/*
 *  Copyright (C) 2022 v0lt
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

#include "stdafx.h"
#include "Utils/StringUtil.h"
#include "BassHelper.h"

#include <../Include/bass.h>
#include <../Include/bass_aac.h>
#include <../Include/bass_mpc.h>
#include <../Include/bass_ofr.h>
#include <../Include/bass_spx.h>
#include <../Include/bass_tta.h>
#include <../Include/bassalac.h>
#include <../Include/bassape.h>
#pragma warning(push)
#pragma warning(disable: 4200)
#include <../Include/bassdsd.h>
#pragma warning(pop)
#include <../Include/bassflac.h>
#include <../Include/bassopus.h>
#include <../Include/basswma.h>
#include <../Include/basswv.h>
#include <../Include/basszxtune.h>


LPCWSTR GetBassTypeStr(const DWORD ctype)
{
	if (ctype & BASS_CTYPE_STREAM) {
		switch (ctype) {
		case BASS_CTYPE_STREAM_VORBIS:   return L"Vorbis";
		case BASS_CTYPE_STREAM_MP1:      return L"MP1";
		case BASS_CTYPE_STREAM_MP2:      return L"MP2";
		case BASS_CTYPE_STREAM_MP3:      return L"MP3";
		case BASS_CTYPE_STREAM_AIFF:     return L"AIFF";
		case BASS_CTYPE_STREAM_MF:       return L"Media Foundation codec stream";
		case BASS_CTYPE_STREAM_WMA:      return L"WMA";
		case BASS_CTYPE_STREAM_AAC:      return L"AAC";
		case BASS_CTYPE_STREAM_MP4:      return L"MP4-AAC";
		case BASS_CTYPE_STREAM_MPC:      return L"Musepack";
		case BASS_CTYPE_STREAM_OFR:      return L"OptimFROG";
		case BASS_CTYPE_STREAM_SPX:      return L"Speex";
		case BASS_CTYPE_STREAM_TTA:      return L"TTA";
		case BASS_CTYPE_STREAM_ALAC:     return L"ALAC";
		case BASS_CTYPE_STREAM_APE:      return L"APE";
		case BASS_CTYPE_STREAM_DSD:      return L"DSD";
		case BASS_CTYPE_STREAM_FLAC:     return L"FLAC";
		case BASS_CTYPE_STREAM_FLAC_OGG: return L"Ogg FLAC";
		case BASS_CTYPE_STREAM_OPUS:     return L"Opus";
		case BASS_CTYPE_STREAM_WV:       return L"WavPack";
		}
	}
	if (ctype & BASS_CTYPE_STREAM_WAV) {
		switch (ctype) {
		case BASS_CTYPE_STREAM_WAV_PCM:   return L"WAV-PCM";
		case BASS_CTYPE_STREAM_WAV_FLOAT: return L"WAV-Float";
		}
	}

	if (ctype & BASS_CTYPE_MUSIC_MOD) {
		switch (ctype) {
		case BASS_CTYPE_MUSIC_MOD: return L"Mod";
		case BASS_CTYPE_MUSIC_MTM: return L"Mod-MTM";
		case BASS_CTYPE_MUSIC_S3M: return L"Mod-S3M";
		case BASS_CTYPE_MUSIC_XM:  return L"Mod-XM";
		case BASS_CTYPE_MUSIC_IT:  return L"Mod-IT";
		case BASS_CTYPE_MUSIC_MOD | BASS_CTYPE_MUSIC_MO3: return L"Mo3";
		case BASS_CTYPE_MUSIC_MTM | BASS_CTYPE_MUSIC_MO3: return L"Mo3-MTM";
		case BASS_CTYPE_MUSIC_S3M | BASS_CTYPE_MUSIC_MO3: return L"Mo3-S3M";
		case BASS_CTYPE_MUSIC_XM  | BASS_CTYPE_MUSIC_MO3: return L"Mo3-XM";
		case BASS_CTYPE_MUSIC_IT  | BASS_CTYPE_MUSIC_MO3: return L"Mo3-IT";
		}
	}

	if (ctype == BASS_CTYPE_MUSIC_ZXTUNE) {
		return L"ZXTune";
	}

	return L"Unknown";
}

void ReadTagsCommon(const char* p, ContentTags& tags)
{
	while (p && *p) {
		std::string_view str(p);
		const size_t k = str.find('=');
		if (k > 0 && k < str.size()) {
			// convert the field name to lowercase to make it easier to recognize
			// examples:"Title", "TITLE", "title"
			std::string field_name(k, '\0');
			std::transform(str.begin(), str.begin() + field_name.size(),
				field_name.begin(), [](unsigned char c) { return std::tolower(c); });

			if (field_name.compare("title") == 0) {
				tags.Title = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("artist") == 0 || field_name.compare("author") == 0) {
				tags.AuthorName = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("comment") == 0 || field_name.compare("description") == 0) {
				tags.Description = ConvertUtf8ToWide(p + k + 1);
				str_trim_end(tags.Description, L' ');
			}
		}

		p += str.size() + 1;
	}

	int gg = 0;
}

void ReadTagsID3v2(const char* p, ContentTags& tags, std::list<ID3v2Pict>* pPictList)
{
	if (p) {
		std::list<ID3v2Frame> id3v2Frames;
		if (ParseID3v2Tag((const BYTE*)p, id3v2Frames)) {
			for (const auto& frame : id3v2Frames) {
				switch (frame.id) {
				case 'TIT2':
				case '\0TT2':
					tags.Title = GetID3v2FrameText(frame);
					break;
				case 'TPE1':
				case '\0TP1':
					tags.AuthorName = GetID3v2FrameText(frame);
					break;
				case 'COMM':
				case '\0COM':
					tags.Description = GetID3v2FrameComment(frame);
					break;
				case 'APIC':
				case '\0PIC':
					if (pPictList) {
						ID3v2Pict id3v2Pict;
						ParseID3v2PictFrame(frame, id3v2Pict);
						if (id3v2Pict.size) {
							pPictList->emplace_back(id3v2Pict);
						}
					}
					break;
 				}
			}
		}
	}
}

void ReadTagsID3v1(const char* p, ContentTags& tags)
{
	if (p && std::string_view(p).compare(0, 3, "TAG") == 0) {
		p += 3;
		std::string str;

		auto id3v1_truncate = [](std::string& s) {
			for (auto it = s.crbegin(); it != s.crend(); ++it) {
				if (*it != 0 && *it != 0x20) {
					s.resize(std::distance(it, s.crend()));
					break;
				}
			}
		};

		str.assign(p, 30);
		id3v1_truncate(str);
		tags.Title = ConvertAnsiToWide(str);
		p += 30;

		str.assign(p, 30);
		id3v1_truncate(str);
		tags.AuthorName = ConvertAnsiToWide(str);
		p += 30 + 30 + 4;

		str.assign(p, p[28] == 0 ? 28 : 30);
		id3v1_truncate(str);
		tags.Description = ConvertAnsiToWide(str);
	}
}
