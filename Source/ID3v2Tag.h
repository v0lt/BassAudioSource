/*
 *  Copyright (C) 2022 v0lt
 */

#pragma once

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

struct ID3v2Frame
{
	uint32_t       id;
	uint32_t       flags;
	const uint8_t* data;
	uint32_t       size;
};

struct ID3v2Pict
{
	uint8_t text_encoding   = 0;
	uint8_t picture_type    = 0;
	const char* mime_type   = nullptr;
	const char* description = nullptr;
	const uint8_t* data     = nullptr;
	uint32_t size           = 0;
};

bool ParseID3v2Tag(const BYTE* buf, std::list<ID3v2Frame>& id3v2Frames);

const uint8_t* DecodeString(const int encoding, const uint8_t* str, const uint8_t* end, std::wstring& wstr);

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame);
std::wstring GetID3v2FrameComment(const ID3v2Frame& id3v2Frame);

void ParseID3v2PictFrame(const ID3v2Frame& id3v2Frame, ID3v2Pict& id3v2Pict);
