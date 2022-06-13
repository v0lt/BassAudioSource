
#include "stdafx.h"
#include "ID3v2Tag.h"
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
