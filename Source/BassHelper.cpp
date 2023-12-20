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
#include "Utils/Util.h"
#include "Utils/StringUtil.h"
#include "Utils/ByteReader.h"
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
#include <../Include/bassmidi.h>

#include <wincrypt.h>

struct METADATA_BLOCK_PICTURE {
	uint32_t apic;      // ID3v2 "APIC" picture type
	uint32_t mime_size;
	const char* mime;   // mime type
	uint32_t desc_size;
	const char* desc;   // description
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t colors;
	uint32_t length;    // data length
	const void* data;
};


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
		case BASS_CTYPE_STREAM_MIDI:     return L"MIDI";
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
			// APE/MP4: Title, Artist, Comment
			// WMA: Title, Author, Description
			std::string field_name(str.data(), k);

			if (field_name.compare("Title") == 0) {
				tags.Title = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("Artist") == 0 || field_name.compare("Author") == 0) {
				tags.AuthorName = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("Comment") == 0 || field_name.compare("Description") == 0) {
				tags.Description = ConvertUtf8ToWide(p + k + 1);
				str_trim_end(tags.Description, L' ');
			}
		}

		p += str.size() + 1;
	}
}

void ReadTagsOgg(const char* p, ContentTags& tags, std::unique_ptr<std::list<DSMResource>>& pResources)
{
	while (p && *p) {
		std::string_view str(p);
		const size_t k = str.find('=');
		if (k > 0 && k < str.size()) {
			// Standard Ogg field names are recommended to be written in upper case.
			// But there are some FLACs where this is not respected.
			// Therefore, we will convert all field names to upper case.
			std::string field_name(k, '\0');
			std::transform(str.begin(), str.begin() + field_name.size(),
				field_name.begin(), [](unsigned char c) { return std::toupper(c); });

			if (field_name.compare("TITLE") == 0) {
				tags.Title = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("ARTIST") == 0) {
				tags.AuthorName = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("COMMENT") == 0) {
				tags.Description = ConvertUtf8ToWide(p + k + 1);
				str_trim_end(tags.Description, L' ');
			}
			else if (field_name.compare("METADATA_BLOCK_PICTURE") == 0 && pResources) {
				LPCSTR pstr = p + k + 1;
				DWORD cbBinary = 0;
				BOOL ok = CryptStringToBinaryA(pstr, 0, CRYPT_STRING_BASE64, nullptr, &cbBinary, nullptr, nullptr);
				if (ok && cbBinary > 32) {
					std::vector<uint8_t> binary(cbBinary);
					ok = CryptStringToBinaryA(pstr, 0, CRYPT_STRING_BASE64, binary.data(), &cbBinary, nullptr, nullptr);
					if (ok && cbBinary > 32) {
						ByteReader br(binary.data());
						br.SetSize(binary.size());

						METADATA_BLOCK_PICTURE FlacPict = {};
						FlacPict.apic = br.Read32Be();
						FlacPict.mime_size = br.Read32Be();
						FlacPict.mime = (const char*)br.GetPtr();
						br.Skip(FlacPict.mime_size);
						FlacPict.desc_size = br.Read32Be();
						FlacPict.desc = (const char*)br.GetPtr();
						br.Skip(FlacPict.desc_size);
						FlacPict.width = br.Read32Be();
						FlacPict.height = br.Read32Be();
						FlacPict.depth = br.Read32Be();
						FlacPict.colors = br.Read32Be();
						FlacPict.length = br.Read32Be();
						FlacPict.data   = br.GetPtr();

						if (FlacPict.length && br.GetRemainder() == FlacPict.length) {
							DSMResource resource;
							resource.mime = ConvertAnsiToWide(FlacPict.mime, FlacPict.mime_size);
							resource.data.resize(FlacPict.length);
							memcpy(resource.data.data(), FlacPict.data, FlacPict.length);
							pResources->emplace_back(resource);
						}
					}
				}
			}
		}

		p += str.size() + 1;
	}
}

void ReadTagsID3v2(const char* p, ContentTags& tags, std::unique_ptr<std::list<DSMResource>>& pResources)
{
	if (p) {
		ID3v2TagInfo id3v2tagInfo = {};
		std::list<ID3v2Frame> id3v2Frames;
		if (ParseID3v2Tag((const BYTE*)p, id3v2tagInfo, id3v2Frames)) {
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
					if (tags.Description.empty()) {
						// read only the first relevant comment
						tags.Description = GetID3v2FrameComment(frame);
					}
					break;
				case 'APIC':
				case '\0PIC':
					if (pResources) {
						DSMResource resource;
						if (GetID3v2FramePicture(id3v2tagInfo, frame, resource)) {
							pResources->emplace_back(resource);
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
