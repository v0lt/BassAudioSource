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

// text encoding:
// 0 - ISO-8859-1
// 1 - UCS-2 (UTF-16 encoded Unicode with BOM)
// 2 - UTF-16BE encoded Unicode without BOM
// 3 - UTF-8 encoded Unicode

enum ID3v2Encoding {
	ISO8859 = 0,
	UTF16BOM = 1,
	UTF16BE = 2,
	UTF8 = 3,
};

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
	if (tag_ver < 2 || tag_ver > 4) {
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

	if (tag_ver == 2) {
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
			uint32_t frame_size = (tag_ver == 4) ? readframesize(p) : read4bytes(p);
			int frame_flags = read2bytes(p);

			ID3v2Frame frame = { frame_id, frame_flags, p, frame_size };
			id3v2Frames.emplace_back(frame);

			p += frame_size;
		}
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
