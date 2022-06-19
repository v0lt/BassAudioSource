
#include "stdafx.h"
#include "ID3v2Tag.h"
#include "Utils/Util.h"
#include "Utils/StringUtil.h"

// based on https://github.com/Aleksoid1978/MPC-BE/blob/master/src/DSUtil/ID3Tag.cpp

//
// CID3v2Tag
//

CID3v2Tag::CID3v2Tag(BYTE major/* = 0*/, BYTE flags/* = 0*/)
	: m_major(major)
	, m_flags(flags)
{
}

CID3v2Tag::~CID3v2Tag()
{
	Clear();
}

void CID3v2Tag::Clear()
{
	for (auto& item : TagItems) {
		if (item) {
			delete item;
		};
	}
	TagItems.clear();
}

// text encoding:
// 0 - ISO-8859-1
// 1 - UCS-2 (UTF-16 encoded Unicode with BOM)
// 2 - UTF-16BE encoded Unicode without BOM
// 3 - UTF-8 encoded Unicode

enum ID3v2Encoding {
	ISO8859  = 0,
	UTF16BOM = 1,
	UTF16BE  = 2,
	UTF8     = 3,
};

std::wstring CID3v2Tag::ReadText(ByteReader& gb, UINT size, const BYTE encoding)
{
	std::wstring str;
	std::string strA;

	BOOL bUTF16LE = FALSE;
	BOOL bUTF16BE = FALSE;
	switch (encoding) {
		case ID3v2Encoding::ISO8859:
		case ID3v2Encoding::UTF8:
			strA.assign(size, '\0');
			gb.ReadBytes(strA.data(), size);
			str = (encoding == ID3v2Encoding::ISO8859)
				? ConvertAnsiToWide(strA)
				: ConvertUtf8ToWide(strA);
			break;
		case ID3v2Encoding::UTF16BOM:
			if (size > 2) {
				WORD bom = (WORD)gb.Read16Be();
				size -= 2;
				if (bom == 0xfffe) {
					bUTF16LE = TRUE;
				} else if (bom == 0xfeff) {
					bUTF16BE = TRUE;
				}
			}
			break;
		case ID3v2Encoding::UTF16BE:
			if (size >= 2) {
				bUTF16BE = TRUE;
			}
			break;
	}

	if (bUTF16LE || bUTF16BE) {
		size /= 2;
		str.assign(size, '\0');
		gb.ReadBytes((BYTE*)str.data(), size * 2);
		if (bUTF16BE) {
			for (size_t i = 0, j = str.size(); i < j; i++) {
				str[i] = (str[i] << 8) | (str[i] >> 8);
			}
		}
	}

	str.erase(std::find(str.begin(), str.end(), '\0'), str.end());
	str_trim(str);

	return str;
}

std::wstring CID3v2Tag::ReadField(ByteReader& gb, UINT &size, const BYTE encoding)
{
	size_t pos = gb.GetPos();

	UINT fieldSize = 0;

	const UINT fieldseparatorSize = (encoding == ID3v2Encoding::ISO8859 || encoding == ID3v2Encoding::UTF8) ? 1 : 2;

	if (fieldseparatorSize == 1) {
		while (size >= 1) {
			size--;
			fieldSize++;
			if (gb.ReadByte() == 0) {
				break;
			}
		}
	}
	else {
		while (size >= 2) {
			size -=2;
			fieldSize +=2;
			if (gb.Read16Le() == 0) {
				break;
			}
		}
	}

	std::wstring str;
	if (fieldSize) {
		gb.Seek(pos);
		str = ReadText(gb, fieldSize, encoding);
	};

	// TODO ???
	/*
	while (size >= fieldseparatorSize
			&& gb.BitRead(8 * fieldseparatorSize, true) == 0) {
		size -= fieldseparatorSize;
		gb.Skip(fieldseparatorSize);
	}
	*/

	if (size && size < fieldseparatorSize) {
		size = 0;
		gb.Skip(size);
	}

	return str;
}

static void ReadLang(ByteReader &gb, UINT &size)
{
	// Language
	/*
	CHAR lang[3] = { 0 };
	gb.ReadBytes((BYTE*)lang, 3);
	UNREFERENCED_PARAMETER(lang);
	*/
	gb.Skip(3);
	size -= 3;
}

static LPCWSTR picture_types[] = {
	L"Other",
	L"32x32 pixels(file icon)",
	L"Other file icon",
	L"Cover (front)",
	L"Cover (back)",
	L"Leaflet page",
	L"Media (lable side of CD)",
	L"Lead artist(lead performer)",
	L"Artist(performer)",
	L"Conductor",
	L"Band(Orchestra)",
	L"Composer",
	L"Lyricist(text writer)",
	L"Recording Location",
	L"During recording",
	L"During performance",
	L"Movie(video) screen capture",
	L"A bright coloured fish",
	L"Illustration",
	L"Band(artist) logo",
	L"Publisher(Studio) logo"
};

void CID3v2Tag::ReadTag(const UINT tag, ByteReader& gbData, UINT &size, CID3TagItem** item)
{
	BYTE encoding = gbData.ReadByte();
	size--;

	if (tag == 'APIC' || tag == '\0PIC') {
		WCHAR mime[64] = {};
		size_t mime_len = 0;

		while (size-- && mime_len < std::size(mime) && (mime[mime_len++] = gbData.ReadByte()) != 0);

		if (mime_len == std::size(mime)) {
			gbData.Skip(size);
			size = 0;
			return;
		}

		BYTE pict_type = gbData.ReadByte();
		size--;
		std::wstring pictStr(L"cover");
		if (pict_type < sizeof(picture_types)) {
			pictStr = picture_types[pict_type];
		}

		if (tag == 'APIC') {
			std::wstring Desc = ReadField(gbData, size, encoding);
			UNREFERENCED_PARAMETER(Desc);
		}

		std::wstring mimeStrLower(mime);
		str_tolower(mimeStrLower);

		std::wstring mimeStr(L"image/");

		if (mimeStrLower == L"jpg" || mimeStrLower == L"png") {
			mimeStr.append(mimeStrLower);
		}
		else if (mimeStrLower.compare(0, 6, L"image/") != 0) {
			mimeStr.append(mime);
		} else {
			mimeStr = mime;
		}

		std::vector<BYTE> data;
		data.resize(size);
		gbData.ReadBytes(data.data(), size);

		*item = new CID3TagItem(tag, data, mimeStr, pictStr);
	} else {
		if (tag == 'COMM' || tag == '\0ULT' || tag == 'USLT') {
			ReadLang(gbData, size);
			std::wstring Desc = ReadField(gbData, size, encoding);
			UNREFERENCED_PARAMETER(Desc);
		}

		std::wstring text;
		if (((char*)&tag)[3] == 'T') {
			while (size) {
				std::wstring field = ReadField(gbData, size, encoding);
				if (!field.empty()) {
					if (text.empty()) {
						text = field;
					} else {
						text.append(L"; ");
						text.append(field);
					}
				}
			}
		} else {
			text = ReadText(gbData, size, encoding);
		}

		if (!text.empty()) {
			*item = new CID3TagItem(tag, text);
		}
	}
}

void CID3v2Tag::ReadChapter(ByteReader& gbData, UINT &size)
{
	std::wstring chapterName;
	std::wstring element = ReadField(gbData, size, ID3v2Encoding::ISO8859);

	UINT start = gbData.Read32Be();
	UINT end   = gbData.Read32Be();
	gbData.Skip(8);

	size -= 16;

	size_t pos = gbData.GetPos();
	while (size > 10) {
		gbData.Seek(pos);

		UINT tag   = (UINT)gbData.Read32Be();
		UINT len   = (UINT)gbData.Read32Be();
		UINT flags = (UINT)gbData.Read16Be(); UNREFERENCED_PARAMETER(flags);
		size -= 10;

		pos += gbData.GetPos() + len;

		if (len > size || tag == 0) {
			break;
		}

		if (!len) {
			continue;
		}

		size -= len;

		if (((char*)&tag)[3] == 'T') {
			CID3TagItem* item = nullptr;
			ReadTag(tag, gbData, len, &item);
			if (item && item->GetType() == ID3Type::ID3_TYPE_STRING) {
				if (chapterName.empty()) {
					chapterName = item->GetValue();
				} else {
					chapterName += L" / " + item->GetValue();
				}
				delete item;
			}
		}
	}

	if (chapterName.empty()) {
		chapterName = element;
	}

	ChaptersList.emplace_back(chapterName, MILLISECONDS_TO_100NS_UNITS(start));
}

#define ID3v2_FLAG_DATALEN 0x0001
#define ID3v2_FLAG_UNSYNCH 0x0002

BOOL CID3v2Tag::ReadTagsV2(const BYTE *buf, size_t len)
{
	ByteReader gb(buf);
	gb.SetSize(len);

	UINT tag;
	UINT size;
	WORD flags;
	int tagsize;
	if (m_major == 2) {
		if (m_flags & 0x40) {
			return FALSE;
		}
		flags   = 0;
		tagsize = 6;
	} else {
		if (m_flags & 0x40) {
			// Extended header present, skip it
			UINT extlen = gb.Read32Be();
			extlen = hexdec2uint(extlen);
			if (m_major == 4) {
				if (extlen < 4) {
					return FALSE;
				}
				extlen -= 4;
			}
			if (extlen + 4 > len) {
				return FALSE;
			}

			gb.Skip(extlen);
			len -= extlen + 4;
		}
		tagsize = 10;
	}
	size_t pos = gb.GetPos();

	while (gb.GetRemainder() >= 6) {
		if (m_major == 2) {
			tag   = (UINT)gb.Read24Be();
			size  = (UINT)gb.Read24Be();
		} else {
			tag   = (UINT)gb.Read32Be();
			size  = (UINT)gb.Read32Be();
			flags = (WORD)gb.Read16Be();
			if (m_major == 4) {
				size = hexdec2uint(size);
			}
		}

		if (pos + tagsize + size > len) {
			break;
		}

		pos += tagsize + size;

		if (pos > gb.GetSize() || tag == 0) {
			break;
		}

		if (pos < gb.GetSize()) {
			const size_t save_pos = gb.GetPos();

			gb.Seek(pos);
			while (gb.GetRemainder() && gb.LookByte() == 0) {
				gb.Skip(1);
				pos++;
				size++;
			}

			gb.Seek(save_pos);
		}

		if (!size) {
			gb.Seek(pos);
			continue;
		}

		if (flags & ID3v2_FLAG_DATALEN) {
			if (size < 4) {
				break;
			}
			gb.Skip(4);
			size -= 4;
		}

		std::vector<BYTE> Data;
		BOOL bUnSync = m_flags & 0x80 || flags & ID3v2_FLAG_UNSYNCH;
		if (bUnSync) {
			UINT dwSize = size;
			while (dwSize) {
				BYTE b = gb.ReadByte();
				Data.push_back(b);
				dwSize--;
				if (b == 0xFF && dwSize > 1) {
					b = gb.ReadByte();
					dwSize--;
					if (!b) {
						b = gb.ReadByte();
						dwSize--;
					}
					Data.push_back(b);
				}
			}
		} else {
			Data.resize(size);
			gb.ReadBytes(Data.data(), size);
		}
		ByteReader gbData(Data.data());
		gbData.SetSize(Data.size());
		size = (UINT)Data.size();

		if (tag == 'TIT2'
				|| tag == 'TPE1'
				|| tag == 'TALB' || tag == '\0TAL'
				|| tag == 'TYER'
				|| tag == 'COMM'
				|| tag == 'TRCK'
				|| tag == 'TCOP'
				|| tag == 'TXXX'
				|| tag == '\0TP1'
				|| tag == '\0TT2'
				|| tag == '\0PIC' || tag == 'APIC'
				|| tag == '\0ULT' || tag == 'USLT') {
			CID3TagItem* item = nullptr;
			ReadTag(tag, gbData, size, &item);

			if (item) {
				TagItems.emplace_back(item);
			}
		} else if (tag == 'CHAP') {
			ReadChapter(gbData, size);
		}

		gb.Seek(pos);
	}

	for (const auto& item : TagItems) {
		if (item->GetType() == ID3Type::ID3_TYPE_STRING && item->GetTag()) {
			Tags[item->GetTag()] = item->GetValue();
		}
	}

	return !TagItems.empty() ? TRUE : FALSE;
}


///////////////////////////////////////////////////////////////////
// experiments
// https://id3.org/id3v2-00
// https://id3.org/id3v2.3.0
// https://id3.org/id3v2.4.0-structure

#define ID3v2_FLAG_UNSYNC 0x80
#define ID3v2_FLAG_EXTHDR 0x40
#define ID3v2_FLAG_EXPERI 0x20
#define ID3v2_FLAG_FOOTER 0x10 // only for ID3v2.4

uint32_t get_id3v2_size(const BYTE* buf)
{
	return
		((buf[0] & 0x7f) << 21) +
		((buf[1] & 0x7f) << 14) +
		((buf[2] & 0x7f) << 7) +
		(buf[3] & 0x7f);
}

uint32_t readframesize(const uint8_t*& p)
{
	uint32_t v = get_id3v2_size(p);
	p += 4;
	return v;
};

uint16_t read2bytes(const uint8_t*& p)
{
	uint16_t v = (p[0] << 8) + (p[1]);
	p += 2;
	return v;
};

uint32_t read3bytes(const uint8_t*& p)
{
	uint32_t v = (p[0] << 16) + (p[1] << 8) + (p[2]);
	p += 3;
	return v;
};

uint32_t read4bytes(const uint8_t*& p)
{
	uint32_t v = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + (p[3]);
	p += 4;
	return v;
};



bool ParseID3v2Tag(const BYTE* buf, std::list<ID3v2Frame>& id3v2Frames)
{
	id3v2Frames.clear();

	if (buf[0] != 'I' || buf[1] != 'D' || buf[2] != '3') {
		return false;
	}

	const int tag_ver = buf[3];
	DLog(L"Parsing ID3v2.%d", tag_ver);
	if (tag_ver < 3 || tag_ver > 4) {
		DLog(L"ID3v2: unsupported version!");
		return false;
	}

	const int tag_rev = buf[4];
	if (tag_rev == 0xff) {
		DLog(L"ID3v2: invalid revision!");
		return false;
	}

	const int tag_flags = buf[5];
	if (tag_flags & ~(ID3v2_FLAG_UNSYNC | ID3v2_FLAG_EXTHDR | ID3v2_FLAG_EXPERI)) {
		return false;
	}
	DLogIf(tag_flags & ID3v2_FLAG_UNSYNC, L"ID3v2 uses unsynchronisation scheme");

	uint32_t u32 = *(uint32_t*)&buf[6];
	if (u32 & 0x80808080) {
		DLog(L"ID3v2: invalid tag size!");
		return false;
	}

	const uint32_t tag_size = get_id3v2_size(&buf[6]);

	const BYTE* p = &buf[10];
	const BYTE* end = p + tag_size;

	if (tag_flags & ID3v2_FLAG_EXTHDR) {
		// Extended header present, skip it
		uint32_t extlen = get_id3v2_size(p);

		if (tag_ver == 4) {
			p += extlen;
		}
		else {
			p += 4 + extlen;
		}
		DLog(L"ID3v2 skip extended header");
	}

	while (p + 10 < end) {
		uint32_t frame_id = read4bytes(p);
		if (frame_id == 0) {
			break;
		}
		uint32_t frame_size = (tag_ver == 4) ? readframesize(p) : read4bytes(p);
		int frame_flags = read2bytes(p);

		ID3v2Frame frame = { frame_id, frame_flags, p, frame_size };
		id3v2Frames.emplace_back(frame);

		p += frame_size;
	}

	DLog(L"ID3v2 found %u frames", (unsigned)id3v2Frames.size());

	return (id3v2Frames.size() > 0);
}

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame)
{
	std::wstring wstr;

	if (id3v2Frame.data && id3v2Frame.size > 2) {

		const uint8_t* p = id3v2Frame.data;
		const uint8_t* end = p + id3v2Frame.size;

		const int encoding = *p++;

		uint16_t bom = 0;

		switch (encoding) {
		case ID3v2Encoding::ISO8859:
		case ID3v2Encoding::UTF8:
			if (p + 1 < end) {
				std::string str(end - p, '\0');
				memcpy(str.data(), p, str.size());
				wstr = (encoding == ID3v2Encoding::ISO8859)
					? ConvertAnsiToWide(str)
					: ConvertUtf8ToWide(str);
			}
			break;
		case UTF16BOM:
			if (p + 4 < end) {
				bom = read2bytes(p);
			}
			break;
		case UTF16BE:
			if (p + 2 < end) {
				bom = 0xfeff;
			}
			break;
		}

		if (bom == 0xfffe || bom == 0xfeff) {
			wstr.assign((end - p) / 2, '\0');
			if (bom == 0xfffe) {
				memcpy(wstr.data(), p, wstr.size() * 2);
			}
			else { //if (bom == 0xfeff)
				auto src = (const uint16_t*)p;
				auto dst = wstr.data();
				for (size_t i = 0; i < wstr.size(); i++) {
					*dst++ = *src++;
				}
			}
		}

		wstr.erase(std::find(wstr.begin(), wstr.end(), '\0'), wstr.end());
		str_trim(wstr);
	}

	return wstr;
}
