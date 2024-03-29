/*
 *  Copyright (C) 2022 v0lt
 */

#pragma once

#include "DSMResource.h"

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

struct ID3v2TagInfo {
	uint32_t ver   : 8;
	uint32_t rev   : 8;
	uint32_t flags : 8;
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

bool ParseID3v2Tag(const BYTE* buf, ID3v2TagInfo& tagInfo, std::list<ID3v2Frame>& id3v2Frames);

std::wstring GetID3v2FrameText(const ID3v2Frame& id3v2Frame);
std::wstring GetID3v2FrameComment(const ID3v2Frame& id3v2Frame);

bool GetID3v2FramePicture(const ID3v2TagInfo& tagInfo, const ID3v2Frame& id3v2Frame, DSMResource& resource);
