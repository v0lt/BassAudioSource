/*
 *  Copyright (C) 2022-2025 v0lt
 *  Based on the following code:
 *  DC-Bass Source filter - http://www.dsp-worx.de/index.php?n=15
 *  DC-Bass Source Filter C++ porting - https://github.com/frafv/DCBassSource
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include "ID3v2Tag.h"

const wchar_t* BassErrorToStr(const int er);

LPCWSTR GetBassTypeStr(const DWORD ctype);

struct ContentTags
{
	std::wstring Title;
	std::wstring AuthorName;
	std::wstring Description;
	std::wstring StationName;

	void Clear() {
		Title.clear();
		AuthorName.clear();
		Description.clear();
	}

	bool Empty() {
		return Title.empty() && AuthorName.empty() && Description.empty();
	}
};

// BASS_TAG_APE, BASS_TAG_MP4, BASS_TAG_WMA
void ReadTagsCommon(const char* p, ContentTags& tags);

// BASS_TAG_OGG
void ReadTagsOgg(const char* p, ContentTags& tags, std::unique_ptr<std::list<DSMResource>>& pResources);

// BASS_TAG_ID3V2
void ReadTagsID3v2(const char* p, ContentTags& tags, std::unique_ptr<std::list<DSMResource>>& pResources);

// BASS_TAG_ID3
void ReadTagsID3v1(const char* p, ContentTags& tags);

// BASS_TAG_ICY, BASS_TAG_HTTP
void ReadTagsICYheaders(const char* p, ContentTags& tags);

// BASS_TAG_META
void ReadTagsICYStreamTitle(const char* p, std::wstring& title);
