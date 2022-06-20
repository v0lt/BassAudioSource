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
