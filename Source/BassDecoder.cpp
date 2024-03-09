/*
 *  Copyright (C) 2022-2024 v0lt
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
#include "BassDecoder.h"
#include "BassSource.h"
#include <../Include/bass_aac.h>
#include <../Include/bassflac.h>
#include <../Include/basswma.h>
#include "Utils/Util.h"
#include "Utils/StringUtil.h"
#include "dllmain.h"

/*** Utilities ****************************************************************/

std::wstring GetFilterDirectory()
{
	std::wstring path(MAX_PATH + 1, '\0');

	DWORD res = GetModuleFileNameW(HInstance, path.data(), (DWORD)(path.size()-1));
	if (res) {
		path.resize(path.rfind('\\', res) + 1);
	}
	else {
		path.clear();
	}

	return path;
}

bool IsMODFile(const std::wstring_view& path)
{
	static LPCWSTR mod_exts[] = { L".it", L".mo3", L".mod", L".mptm", L".mtm", L".s3m", L".umx", L".xm"};

	auto ext = std::filesystem::path(path).extension();

	for (const auto& mod_ext : mod_exts) {
		if (lstrcmpiW(mod_ext, ext.c_str()) == 0) {
			return true;
		}
	}

	return false;
}

bool IsMIDIFile(const std::wstring_view& path)
{
	static LPCWSTR midi_exts[] = { L".midi", L".mid", L".rmi", L".kar" };

	auto ext = std::filesystem::path(path).extension();

	for (const auto& midi_ext : midi_exts) {
		if (lstrcmpiW(midi_ext, ext.c_str()) == 0) {
			return true;
		}
	}

	return false;
}

bool IsURLPath(const std::wstring_view& path)
{
	return path.compare(0, 7, L"http://") == 0
		|| path.compare(0, 8, L"https://") == 0
		|| path.compare(0, 6, L"ftp://") == 0;
}


const wchar_t* BassErrorToStr(const int er)
{
#define UNPACK_VALUE(VALUE) case VALUE: return L#VALUE;
	switch (er) {
		UNPACK_VALUE(BASS_OK);
		UNPACK_VALUE(BASS_ERROR_MEM);
		UNPACK_VALUE(BASS_ERROR_FILEOPEN);
		UNPACK_VALUE(BASS_ERROR_DRIVER);
		UNPACK_VALUE(BASS_ERROR_BUFLOST);
		UNPACK_VALUE(BASS_ERROR_HANDLE);
		UNPACK_VALUE(BASS_ERROR_FORMAT);
		UNPACK_VALUE(BASS_ERROR_POSITION);
		UNPACK_VALUE(BASS_ERROR_INIT);
		UNPACK_VALUE(BASS_ERROR_START);
		UNPACK_VALUE(BASS_ERROR_SSL);
		UNPACK_VALUE(BASS_ERROR_REINIT);
		UNPACK_VALUE(BASS_ERROR_ALREADY);
		UNPACK_VALUE(BASS_ERROR_NOTAUDIO);
		UNPACK_VALUE(BASS_ERROR_NOCHAN);
		UNPACK_VALUE(BASS_ERROR_ILLTYPE);
		UNPACK_VALUE(BASS_ERROR_ILLPARAM);
		UNPACK_VALUE(BASS_ERROR_NO3D);
		UNPACK_VALUE(BASS_ERROR_NOEAX);
		UNPACK_VALUE(BASS_ERROR_DEVICE);
		UNPACK_VALUE(BASS_ERROR_NOPLAY);
		UNPACK_VALUE(BASS_ERROR_FREQ);
		UNPACK_VALUE(BASS_ERROR_NOTFILE);
		UNPACK_VALUE(BASS_ERROR_NOHW);
		UNPACK_VALUE(BASS_ERROR_EMPTY);
		UNPACK_VALUE(BASS_ERROR_NONET);
		UNPACK_VALUE(BASS_ERROR_CREATE);
		UNPACK_VALUE(BASS_ERROR_NOFX);
		UNPACK_VALUE(BASS_ERROR_NOTAVAIL);
		UNPACK_VALUE(BASS_ERROR_DECODE);
		UNPACK_VALUE(BASS_ERROR_DX);
		UNPACK_VALUE(BASS_ERROR_TIMEOUT);
		UNPACK_VALUE(BASS_ERROR_FILEFORM);
		UNPACK_VALUE(BASS_ERROR_SPEAKER);
		UNPACK_VALUE(BASS_ERROR_VERSION);
		UNPACK_VALUE(BASS_ERROR_CODEC);
		UNPACK_VALUE(BASS_ERROR_ENDED);
		UNPACK_VALUE(BASS_ERROR_BUSY);
		UNPACK_VALUE(BASS_ERROR_UNSTREAMABLE);
		UNPACK_VALUE(BASS_ERROR_PROTOCOL);
		UNPACK_VALUE(BASS_ERROR_DENIED);
	default:
		UNPACK_VALUE(BASS_ERROR_UNKNOWN);
	};
#undef UNPACK_VALUE
}

/*** Callbacks ****************************************************************/

void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	BassDecoder* decoder = (BassDecoder*)user;

	if (!decoder->m_shoutcastEvents) {
		return;
	}

	ContentTags tags;

	if (handle == decoder->m_syncMeta) {
		DLog(L"OnMetaData() - BASS_SYNC_META");
		LPCSTR metaTagsUtf8 = BASS_ChannelGetTags(channel, BASS_TAG_META);
		if (metaTagsUtf8) {
			std::wstring metaTags = ConvertUtf8ToWide(metaTagsUtf8);
			DLog(L"Received Meta Tag: {}", metaTags.c_str());

			size_t k1 = metaTags.find(L"StreamTitle='");
			if (k1 != metaTags.npos) {
				k1 += 13;
				size_t k2 = metaTags.find(L'\'', k1);
				if (k2 != metaTags.npos) {
					tags.Title = metaTags.substr(k1, k2 - k1);
					decoder->m_shoutcastEvents->OnMetaDataCallback(&tags);
				}
			}
			else if ((k1 = metaTags.find(L"TITLE=") != metaTags.npos) ||
				(k1 = metaTags.find(L"Title=") != metaTags.npos) ||
				(k1 = metaTags.find(L"title=") != metaTags.npos)) {
				k1 += 6;
				tags.Title = metaTags.substr(k1);
				decoder->m_shoutcastEvents->OnMetaDataCallback(&tags);
			}
		}
		return;
	}

	if (handle == decoder->m_syncOggChange) {
		DLog(L"OnMetaData() - BASS_SYNC_OGG_CHANGE");
		LPCSTR p = BASS_ChannelGetTags(channel, BASS_TAG_OGG);
		if (p) {
			ReadTagsCommon(p, tags);
			decoder->m_shoutcastEvents->OnMetaDataCallback(&tags);
		}
		return;
	}
}

void CALLBACK OnDownloadData(const void* buffer, DWORD length, void* user)
{
	BassDecoder* decoder = (BassDecoder*)user;
	if (buffer && decoder->m_shoutcastEvents) {
		decoder->m_shoutcastEvents->OnShoutcastBufferCallback(buffer, length);
	}
}

//
// BassDecoder
//

BassDecoder::BassDecoder(ShoutcastEvents* shoutcastEvents, Settings_t& sets)
	: m_shoutcastEvents(shoutcastEvents)
	, m_midiSoundFontDefault(sets.sMidiSoundFontDefault)
{
	const std::wstring ddlPath = GetFilterDirectory().append(L"OptimFROG.dll");
	m_optimFROGDLL = LoadLibraryW(ddlPath.c_str());

	LoadBASS();
	LoadPlugins();
}

BassDecoder::~BassDecoder()
{
	Close();

	if (m_optimFROGDLL) {
		FreeLibrary(m_optimFROGDLL);
	}
}

void BassDecoder::LoadBASS()
{
	BASS_Init(0, 44100, 0, GetDesktopWindow(), nullptr);

	BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, LABEL_BassAudioSource);

	BASS_SetConfig(BASS_CONFIG_MF_VIDEO, FALSE);
	BASS_SetConfig(BASS_CONFIG_MP4_VIDEO, FALSE);
	BASS_SetConfig(BASS_CONFIG_WMA_VIDEO, FALSE);
}

void BassDecoder::UnloadBASS()
{
	try {
		if (InstanceCount == 0) {
			BASS_Free();
		}
	}
	catch (...) {
		// crashes mplayer2.exe ???
	}
}

void BassDecoder::LoadPlugins()
{
	static LPCWSTR BassPlugins[] = {
		L"bass_aac.dll",
		L"bass_mpc.dll",
		L"bass_ofr.dll",
		L"bass_spx.dll",
		L"bass_tta.dll",
		L"bassalac.dll",
		L"bassape.dll",
		L"bassdsd.dll",
		L"bassflac.dll",
		L"bassopus.dll",
		L"basswma.dll",
		L"basswv.dll",
		L"basszxtune.dll",
		L"bassmidi.dll",
	};

	const std::wstring filterDir = GetFilterDirectory();

	for (const auto pligin : BassPlugins) {
		const std::wstring pluginPath = filterDir + pligin;
		HPLUGIN hPlugin = BASS_PluginLoad(LPCSTR(pluginPath.c_str()), BASS_UNICODE);
#ifdef _DEBUG
		if (hPlugin) {
			std::wstring dbgstr = std::format(L"{}:\n", pligin);
			const BASS_PLUGININFO* pPluginInfo = BASS_PluginGetInfo(hPlugin);
			if (pPluginInfo) {
				for (DWORD i = 0; i < pPluginInfo->formatc; i++) {
					dbgstr += std::format(L"ctype={} name={} exts={}\n",
						pPluginInfo->formats[i].ctype,
						A2WStr(pPluginInfo->formats[i].name),
						A2WStr(pPluginInfo->formats[i].exts)
					);
				}
			}
			DLog(dbgstr);
		}
#endif
	}
}

std::wstring GetNameTag(LPCSTR string)
{
	std::wstring name_tag;

	std::wstring strTags = ConvertAnsiToWide(string);

	LPCWSTR astring = strTags.c_str();
	while (astring && *astring) {
		LPCWSTR tag = astring;
		if (wcsncmp(L"icy-name:", tag, 9) == 0) {
			tag += 9;
			while (*tag && iswspace(*tag)) {
				tag++;
			}
			name_tag = tag;
			break;
		}
		astring += wcslen(astring) + 1;
	}

	return name_tag;
}

bool BassDecoder::Load(std::wstring path) // use copy of path here
{
	Close();
	DLog(L"BassDecoder::Load - \"{}\"", path);

	if (path.compare(0, 4, L"icyx") == 0) {
		// replace ICYX
		path[0] = 'h';
		path[1] = 't';
		path[2] = 't';
		path[3] = 'p';
	}

	if (IsMODFile(path)) {
		m_isMOD = true;
	}
	else if (IsMIDIFile(path)) {
		m_isMIDI = true;
		m_soundFont = BASS_MIDI_FontInit((const void*)m_midiSoundFontDefault.c_str(), BASS_MIDI_FONT_MMAP | BASS_UNICODE);
		if (m_soundFont) {
			BASS_MIDI_FONT sf = { m_soundFont, -1, 0 };
			BOOL ret = BASS_MIDI_StreamSetFonts(0, &sf, 1); // set default soundfont
		} else {
			DLog(L"ERROR: default SoundFont not found!");
			return false;
		}
		
	}
	m_isURL  = IsURLPath(path);

	if (m_isMOD) {
		if (!m_isURL) {
			m_stream = BASS_MusicLoad(false, (const void*)path.c_str(), 0, 0, BASS_MUSIC_DECODE | BASS_MUSIC_RAMP | BASS_MUSIC_POSRESET | BASS_MUSIC_PRESCAN | BASS_UNICODE, 0);
		}
	}
	else if (m_isURL) {
		m_stream = BASS_StreamCreateURL(
			LPCSTR(path.c_str()), 0,
			BASS_STREAM_BLOCK | BASS_STREAM_DECODE | BASS_UNICODE | BASS_STREAM_STATUS,
			OnDownloadData, this
		);
	}
	else {
		m_stream = BASS_StreamCreateFile(false, (const void*)path.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_UNICODE);
	}

	if (!m_stream) {
		const int error_code = BASS_ErrorGetCode();
		DLog(L"BassDecoder::Load - Opening the path failed with error = {}", BassErrorToStr(error_code));
		return false;
	}

	if (m_isURL) {
		m_syncMeta = BASS_ChannelSetSync(m_stream, BASS_SYNC_META, 0, OnMetaData, this);
		m_syncOggChange = BASS_ChannelSetSync(m_stream, BASS_SYNC_OGG_CHANGE, 0, OnMetaData, this);

		m_isLiveStream = GetDuration() == 0;
	}

	if (!GetStreamInfos()) {
		Close();
		return false;
	}

	DLog(L"BassDecoder::Load - '{}', {} Hz, {} ch, {}{}",
		GetBassTypeStr(m_ctype), m_sampleRate, m_channels, m_float ? L"Float" : L"Int", m_bytesPerSample*8);

	ContentTags tags;
	auto pResources = std::make_unique<std::list<DSMResource>>();

	if (m_isMOD) {
		LPCSTR p = BASS_ChannelGetTags(m_stream, BASS_TAG_MUSIC_NAME);
		if (p) {
			DLog(L"Found Music Name");
			m_tagTitle = ConvertAnsiToWide(p);
		}
	}
	else if (m_isLiveStream) {
		LPCSTR p = BASS_ChannelGetTags(m_stream, BASS_TAG_OGG);
		if (p) {
			DLog(L"Found OGG Tag");
			ReadTagsOgg(p, tags, pResources);
		}
	}
	else {
		LPCSTR p = BASS_ChannelGetTags(m_stream, BASS_TAG_APE);
		DLogIf(p, L"Found APE Tag");
		if (!p) {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_MP4);
			DLogIf(p, L"Found MP4 Tag");
		}
		if (!p) {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_WMA);
			DLogIf(p, L"Found WMA Tag");
		}

		if (p) {
			ReadTagsCommon(p, tags);
		}
		else if (p = BASS_ChannelGetTags(m_stream, BASS_TAG_OGG)) {
			DLog(L"Found OGG Tag");
			ReadTagsOgg(p, tags, pResources);
		}
		else if (p = BASS_ChannelGetTags(m_stream, BASS_TAG_ID3V2)) {
			DLog(L"Found ID3v2 Tag");
			ReadTagsID3v2(p, tags, pResources);
		}
		else if (p = BASS_ChannelGetTags(m_stream, BASS_TAG_ID3)) {
			DLog(L"Found ID3v1 Tag");
			ReadTagsID3v1(p, tags);
		}

		TAG_FLAC_PICTURE* pFlacPict = (TAG_FLAC_PICTURE*)BASS_ChannelGetTags(m_stream, BASS_TAG_FLAC_PICTURE);
		if (pFlacPict && pFlacPict->length) {
			DSMResource resource;
			resource.mime = ConvertAnsiToWide(pFlacPict->mime);
			resource.data.resize(pFlacPict->length);
			memcpy(resource.data.data(), pFlacPict->data, pFlacPict->length);
			pResources->emplace_back(resource);
		}
	}

	if (m_isLiveStream && tags.Empty()) {
		LPCSTR icyTags;
		LPCSTR httpHeaders;

		icyTags = BASS_ChannelGetTags(m_stream, BASS_TAG_ICY);
		if (icyTags) {
			tags.Title = GetNameTag(icyTags);
		}

		httpHeaders = BASS_ChannelGetTags(m_stream, BASS_TAG_HTTP);
		if (httpHeaders) {
			tags.Title = GetNameTag(httpHeaders);
		}

		LPCSTR metaTagsUtf8 = BASS_ChannelGetTags(m_stream, BASS_TAG_META);
		if (metaTagsUtf8) {
			std::wstring metaTags = ConvertUtf8ToWide(metaTagsUtf8);
			DLog(L"Received Meta Tag: {}", metaTags.c_str());

			size_t k1 = metaTags.find(L"StreamTitle='");
			if (k1 != metaTags.npos) {
				k1 += 13;
				size_t k2 = metaTags.find(L'\'', k1);
				if (k2 != metaTags.npos) {
					tags.Title = metaTags.substr(k1, k2 - k1);
				}
			}
			else if ((k1 = metaTags.find(L"TITLE=") != metaTags.npos) ||
				(k1 = metaTags.find(L"Title=") != metaTags.npos) ||
				(k1 = metaTags.find(L"title=") != metaTags.npos)) {
				k1 += 6;
				tags.Title = metaTags.substr(k1);
			}
		}
	}

	if (m_shoutcastEvents) {
		if (!tags.Empty()) {
			m_shoutcastEvents->OnMetaDataCallback(&tags);
		}
		if (pResources->size()) {
			m_shoutcastEvents->OnResourceDataCallback(pResources);
		}
	}

	return true;
}

void BassDecoder::Close()
{
	if (m_stream) {
		if (m_syncMeta) {
			BASS_ChannelRemoveSync(m_stream, m_syncMeta);
		}
		if (m_syncOggChange) {
			BASS_ChannelRemoveSync(m_stream, m_syncOggChange);
		}

		if (m_isMOD) {
			BASS_MusicFree(m_stream);
		}
		else {
			BASS_StreamFree(m_stream);
		}

		m_stream = 0;
	}

	if (m_soundFont) {
		BASS_MIDI_FontFree(m_soundFont);
		m_soundFont = 0;
	}

	m_syncMeta = 0;
	m_syncOggChange = 0;

	m_channels = 0;
	m_sampleRate = 0;
	m_bytesPerSample = 0;
	m_float = false;
	m_mSecConv = 0;
	m_ctype = 0;

	m_isMOD  = false;
	m_isMIDI = false;
	m_isURL  = false;
	m_isLiveStream = false;

	m_tagTitle.clear();
	m_tagArtist.clear();
	m_tagComment.clear();
}

int BassDecoder::GetData(void* buffer, int size)
{
	return BASS_ChannelGetData(m_stream, buffer, size);
}

bool BassDecoder::GetStreamInfos()
{
	BASS_CHANNELINFO info;

	if (!BASS_ChannelGetInfo(m_stream, &info)) {
		return false;
	}

	if (info.chans == 0 || info.freq == 0) {
		return false;
	}

	m_float = (info.flags & BASS_SAMPLE_FLOAT) != 0;
	m_sampleRate = info.freq;
	m_channels   = info.chans;
	m_ctype      = info.ctype;

	if (m_float) {
		m_bytesPerSample = 4;
	}
	else if (info.flags & BASS_SAMPLE_8BITS) {
		m_bytesPerSample = 1;
	}
	else {
		m_bytesPerSample = 2;
	}

	m_mSecConv = m_sampleRate * m_channels * m_bytesPerSample;

	if (m_mSecConv == 0) {
		return false;
	}

	return true;
}

LONGLONG BassDecoder::GetDuration()
{
	if (m_mSecConv == 0) {
		return 0;
	}
	if (!m_stream) {
		return 0;
	}

	// bytes = samplerate * channel * bytes_per_second
	// msecs = (bytes * 1000) / (samplerate * channels * bytes_per_second)

	return BASS_ChannelGetLength(m_stream, BASS_POS_BYTE) * 1000 / m_mSecConv;
}

LONGLONG BassDecoder::GetPosition()
{
	if (m_mSecConv == 0) {
		return 0;
	}
	if (!m_stream) {
		return 0;
	}

	return BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE) * 1000 / m_mSecConv;
}

void BassDecoder::SetPosition(LONGLONG positionMS)
{
	LONGLONG pos;

	if (m_mSecConv == 0) {
		return;
	}
	if (!m_stream) {
		return;
	}

	pos = LONGLONG(positionMS * m_mSecConv) / 1000L;

	BASS_ChannelSetPosition(m_stream, pos, BASS_POS_BYTE);
}
