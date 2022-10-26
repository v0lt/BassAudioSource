/*
 *  Copyright (C) 2022 v0lt
 */

#include "stdafx.h"
#include "ID3v2Tag.h"
#include "Utils/Util.h"
#include "Utils/StringUtil.h"

// specifications
// https://id3.org/id3v2-00
// https://id3.org/id3v2.3.0
// https://id3.org/id3v2.4.0-structure

#define ID3v2_FLAG_UNSYNC 0x80
#define ID3v2_FLAG_EXTHDR 0x40
#define ID3v2_FLAG_EXPERI 0x20
#define ID3v2_FLAG_FOOTER 0x10 // only for ID3v2.4

#define ID3v2_FRAME_FLAG_DATALEN 0x0001
#define ID3v2_FRAME_FLAG_UNSYNCH 0x0002

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


bool ParseID3v2Tag(const BYTE* buf, ID3v2TagInfo& tagInfo, std::list<ID3v2Frame>& id3v2Frames)
{
	id3v2Frames.clear();

	if (buf[0] != 'I' || buf[1] != 'D' || buf[2] != '3') {
		return false;
	}

	tagInfo = { buf[3], buf[4], buf[5] };

	DLog(L"Parsing ID3v2.%d", tagInfo.ver);
	if (tagInfo.ver < 2 || tagInfo.ver > 4) {
		DLog(L"ID3v2: unsupported version!");
		return false;
	}

	if (tagInfo.rev == 0xff) {
		DLog(L"ID3v2: invalid revision!");
		return false;
	}

	if (tagInfo.flags & ~(ID3v2_FLAG_UNSYNC | ID3v2_FLAG_EXTHDR | ID3v2_FLAG_EXPERI)) {
		return false;
	}
	DLogIf(tagInfo.flags & ID3v2_FLAG_UNSYNC, L"ID3v2 uses unsynchronisation scheme");

	uint32_t u32 = *(uint32_t*)&buf[6];
	if (u32 & 0x80808080) {
		DLog(L"ID3v2: invalid tag size!");
		return false;
	}

	const uint32_t tag_size = get_id3v2_size(&buf[6]);

	const BYTE* p = &buf[10];
	const BYTE* end = p + tag_size;

	if (tagInfo.flags & ID3v2_FLAG_EXTHDR) {
		// Extended header present, skip it
		uint32_t extlen = get_id3v2_size(p);

		if (tagInfo.ver == 4) {
			p += extlen;
		}
		else {
			p += 4 + extlen;
		}
		DLog(L"ID3v2 skip extended header");
	}

	if (tagInfo.ver == 2) {
		while (p + 6 < end) {
			uint32_t frame_id = read3bytes(p);
			if (frame_id == 0) {
				break;
			}
			uint32_t frame_size = read3bytes(p);

			ID3v2Frame frame = { frame_id, 0, p, frame_size };
			id3v2Frames.emplace_back(frame);

			p += frame_size;
		}
	}
	else {
		while (p + 10 < end) {
			uint32_t frame_id = read4bytes(p);
			if (frame_id == 0) {
				break;
			}
			uint32_t frame_size = (tagInfo.ver == 4) ? readframesize(p) : read4bytes(p);
			int frame_flags = read2bytes(p);

			ID3v2Frame frame = { frame_id, frame_flags, p, frame_size };
			id3v2Frames.emplace_back(frame);

			p += frame_size;
		}
	}

	DLog(L"ID3v2 found %u frames", (unsigned)id3v2Frames.size());

	return (id3v2Frames.size() > 0);
}

const uint8_t* DecodeString(const int encoding, const uint8_t* str, const uint8_t* end, std::wstring& wstr)
{
	auto p = str;
	int len = 0;
	uint16_t bom = 0;

	switch (encoding) {
	case ID3v2Encoding::ISO8859:
	case ID3v2Encoding::UTF8:
		while (p < end && *p) {
			p++;
		}
		len = p - str;
		if (len) {
			wstr = (encoding == ID3v2Encoding::ISO8859)
				? ConvertAnsiToWide((char*)str, len)
				: ConvertUtf8ToWide((char*)str, len);
		}
		if (p < end && *p == 0) {
			p++;
		}
		return p;
	case UTF16BOM:
		if (str + 2 < end) {
			bom = read2bytes(str);
			p = str;
		}
		break;
	case UTF16BE:
		bom = 0xfeff;
		break;
	}

	if (bom == 0xfffe || bom == 0xfeff) {
		while ((p+1) < end && *(uint16_t*)p) {
			p += 2;
		}
		wstr.assign((p - str) / 2, '\0');
		if (bom == 0xfffe) {
			memcpy(wstr.data(), str, wstr.size() * 2);
		}
		else { //if (bom == 0xfeff)
			auto src = (const wchar_t*)str;
			auto dst = wstr.data();
			for (size_t i = 0; i < wstr.size(); i++) {
				*dst++ = _byteswap_ushort(*src++);
			}
		}
		if ((p + 1) < end && *(uint16_t*)p == 0) {
			p += 2;
		}
	}

	return p;
}

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame)
{
	std::wstring wstr;

	if (id3v2Frame.data && id3v2Frame.size >= 2) {
		const uint8_t* p = id3v2Frame.data;
		const uint8_t* end = p + id3v2Frame.size;

		const int encoding = *p++;

		p = DecodeString(encoding, p, end, wstr);

		str_trim(wstr);
	}

	return wstr;
}

std::wstring GetID3v2FrameComment(const ID3v2Frame& id3v2Frame)
{
	std::wstring wstr;

	if (id3v2Frame.data && id3v2Frame.size >= 5) {
		const uint8_t* p = id3v2Frame.data;
		const uint8_t* end = p + id3v2Frame.size;

		const int encoding = *p++;
		const uint32_t lang = read3bytes(p);

		std::wstring content_desc;
		p = DecodeString(encoding, p, end, content_desc);

		if (content_desc.empty()) {
			// ignore comments with content description (usually there is unknown technical information)
			p = DecodeString(encoding, p, end, wstr);
			str_trim(wstr);
		}
	}

	return wstr;
}

bool GetID3v2FramePicture(const ID3v2TagInfo& tagInfo, const ID3v2Frame& id3v2Frame, DSMResource& resource)
{
	if (id3v2Frame.data && id3v2Frame.size > 4) {
		const uint8_t* p = id3v2Frame.data;
		const uint8_t* end = p + id3v2Frame.size;

		uint32_t datalen = 0;
		if (id3v2Frame.flags & ID3v2_FRAME_FLAG_DATALEN) {
			datalen = readframesize(p);
		}

		const int encoding = *p++;

		if (tagInfo.ver == 2) {
			uint32_t img_fmt = read3bytes(p);
			if (img_fmt == 'JPG') {
				resource.mime = L"image/jpeg";
			}
			else if (img_fmt == 'PNG') {
				resource.mime = L"image/png";
			}
		}
		else {
			p = DecodeString(encoding, p, end, resource.mime);
		}

		if (p < end) {
			const int picture_type = *p++;

			p = DecodeString(encoding, p, end, resource.desc);

			if (p < end) {
				const uint32_t len = end - p;
				if ((id3v2Frame.flags & ID3v2_FRAME_FLAG_DATALEN)==0 || datalen > len) {
					datalen = len;
				}

				resource.data.resize(datalen);

				if (id3v2Frame.flags & ID3v2_FRAME_FLAG_UNSYNCH) {
					uint32_t i = 0;
					for (; i < datalen && p < end; i++) {
						resource.data[i] = *p++;
						if (resource.data[i] == 0xff && p < end && *p == 0x00) {
							p++;
						}
					}
					resource.data.resize(i);
				}
				else {
					memcpy(resource.data.data(), p, datalen);
				}

				return true;
			}
		}
	}

	return false;
}
