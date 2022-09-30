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
#include "BassDecoder.h"
#include "BassSource.h"
#include <../Include/bass.h>
#include "Utils/Util.h"
#include "Utils/StringUtil.h"
#include "dllmain.h"
#include "BassHelper.h"

#include "ID3v2Tag.h"

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

bool IsURLPath(const std::wstring_view& path)
{
	return path.compare(0, 7, L"http://") == 0
		|| path.compare(0, 8, L"https://") == 0
		|| path.compare(0, 6, L"ftp://") == 0;
}

/*** Callbacks ****************************************************************/

void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	BassDecoder* decoder = (BassDecoder*)user;

	if (decoder->m_shoutcastEvents) {
		LPCSTR metaTagsUtf8 = BASS_ChannelGetTags(channel, BASS_TAG_META);
		if (metaTagsUtf8) {
			std::wstring metaTags = ConvertUtf8ToWide(metaTagsUtf8);
			DLog(L"Received Meta Tag: %s", metaTags.c_str());

			size_t k1 = metaTags.find(L"StreamTitle='");
			if (k1 != metaTags.npos) {
				k1 += 13;
				size_t k2 = metaTags.find(L'\'', k1);
				if (k2 != metaTags.npos) {
					const std::wstring title = metaTags.substr(k1, k2 - k1);
					decoder->m_shoutcastEvents->OnShoutcastMetaDataCallback(title.c_str());
				}
			}
			else if ((k1 = metaTags.find(L"TITLE=") != metaTags.npos) ||
				(k1 = metaTags.find(L"Title=") != metaTags.npos) ||
				(k1 = metaTags.find(L"title=") != metaTags.npos)) {
				k1 += 6;
				decoder->m_shoutcastEvents->OnShoutcastMetaDataCallback(metaTags.c_str() + k1);
			}
		}
	}
}

void CALLBACK OnShoutcastData(const void* buffer, DWORD length, void* user)
{
	BassDecoder* decoder = (BassDecoder*)user;
	if (buffer && decoder->m_shoutcastEvents) {
		decoder->m_shoutcastEvents->OnShoutcastBufferCallback(buffer, length);
	}
}

//
// BassDecoder
//

BassDecoder::BassDecoder(ShoutcastEvents* shoutcastEvents, int buffersizeMS, int prebufferMS)
	: m_shoutcastEvents(shoutcastEvents)
	, m_buffersizeMS(buffersizeMS)
	, m_prebufferMS(prebufferMS)
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

	if (m_prebufferMS == 0) {
		m_prebufferMS = 1;
	}

	BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, LABEL_BassAudioSource);
	BASS_SetConfig(BASS_CONFIG_NET_BUFFER, m_buffersizeMS);
	BASS_SetConfig(BASS_CONFIG_NET_PREBUF, m_buffersizeMS * 100 / m_prebufferMS);
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
		L"basshls.dll",
		L"bassopus.dll",
		L"basswma.dll",
		L"basswv.dll",
		L"basszxtune.dll",
	};

	const std::wstring filterDir = GetFilterDirectory();

	for (const auto pligin : BassPlugins) {
		const std::wstring pluginPath = filterDir + pligin;
		BASS_PluginLoad(LPCSTR(pluginPath.c_str()), BASS_UNICODE);
	}
}

void BassDecoder::ReadTagsÑommon(LPCSTR p)
{
	while (p && *p) {
		std::string_view str(p);
		const size_t k = str.find('=');
		if (k > 0 && k < str.size()) {
			// convert the field name to lowercase to make it easier to recognize
			// examples:"Title", "TITLE", "title"
			std::string field_name(k, '\0');
			std::transform(str.begin(), str.begin()+ field_name.size(),
				field_name.begin(), [](unsigned char c) { return std::tolower(c); });

			if (field_name.compare("title") == 0) {
				m_tagTitle = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("artist") == 0 || field_name.compare("author") == 0) {
				m_tagArtist = ConvertUtf8ToWide(p + k + 1);
			}
			else if (field_name.compare("comment") == 0 || field_name.compare("description") == 0) {
				m_tagComment = ConvertUtf8ToWide(p + k + 1);
				str_trim_end(m_tagComment, L' ');
			}
		}

		p += str.size() + 1;
	}
}

void BassDecoder::ReadTagsID3v2(LPCSTR p)
{
	if (p) {
		std::list<ID3v2Frame> id3v2Frames;
		if (ParseID3v2Tag((const BYTE*)p, id3v2Frames)) {
			for (const auto& frame : id3v2Frames) {
				switch (frame.id) {
				case 'TIT2':
				case '\0TT2':
					m_tagTitle = GetID3v2FrameText(frame);
					break;
				case 'TPE1':
				case '\0TP1':
					m_tagArtist = GetID3v2FrameText(frame);
					break;
				case 'COMM':
				case '\0COM':
					m_tagComment = GetID3v2FrameText(frame);
					break;
				}
			}
		}
	}
}

void BassDecoder::ReadTagsID3v1(LPCSTR p)
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
		m_tagTitle = ConvertAnsiToWide(str);
		p += 30;

		str.assign(p, 30);
		id3v1_truncate(str);
		m_tagArtist = ConvertAnsiToWide(str);
		p += 30 + 30 + 4;

		str.assign(p, p[28] == 0 ? 28 : 30);
		id3v1_truncate(str);
		m_tagComment = ConvertAnsiToWide(str);
	}
}

bool BassDecoder::Load(std::wstring path) // use copy of path here
{
	Close();

	m_isShoutcast = false;

	if (path.compare(0, 4, L"icyx") == 0) {
		// replace ICYX
		path[0] = 'h';
		path[1] = 't';
		path[2] = 't';
		path[3] = 'p';
	}

	m_isMOD = IsMODFile(path);
	m_isURL = IsURLPath(path);

	if (m_isMOD) {
		if (!m_isURL) {
			m_stream = BASS_MusicLoad(false, (const void*)path.c_str(), 0, 0, BASS_MUSIC_DECODE | BASS_MUSIC_RAMP | BASS_MUSIC_POSRESET | BASS_MUSIC_PRESCAN | BASS_UNICODE, 0);
		}
	}
	else {
		if (m_isURL) {
			m_stream = BASS_StreamCreateURL(LPCSTR(path.c_str()), 0, BASS_STREAM_DECODE | BASS_UNICODE, OnShoutcastData, this);
			m_sync = BASS_ChannelSetSync(m_stream, BASS_SYNC_META, 0, OnMetaData, this);
			m_isShoutcast = GetDuration() == 0;
		}
		else {
			m_stream = BASS_StreamCreateFile(false, (const void*)path.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_UNICODE);
		}
	}

	if (!m_stream) {
		return false;
	}

	if (!GetStreamInfos()) {
		Close();
		return false;
	}

	if (m_isMOD) {
		LPCSTR p = BASS_ChannelGetTags(m_stream, BASS_TAG_MUSIC_NAME);
		if (p) {
			DLog(L"Found Music Name");
			m_tagTitle = ConvertAnsiToWide(p);
		}
	}
	else if (!m_isURL) {
		LPCSTR p = BASS_ChannelGetTags(m_stream, BASS_TAG_APE);
		DLogIf(p, L"Found APE Tag");
		if (!p) {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_OGG);
			DLogIf(p, L"Found OGG Tag");
		}
		if (!p) {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_MP4);
			DLogIf(p, L"Found MP4 Tag");
		}
		if (!p) {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_WMA);
			DLogIf(p, L"Found WMA Tag");
		}

		if (p) {
			ReadTagsÑommon(p);
		}
		else {
			p = BASS_ChannelGetTags(m_stream, BASS_TAG_ID3V2);
			if (p) {
				DLog(L"Found ID3v2 Tag");
				ReadTagsID3v2(p);
			}
			else {
				p = BASS_ChannelGetTags(m_stream, BASS_TAG_ID3);
				if (p) {
					DLog(L"Found ID3v1 Tag");
					ReadTagsID3v1(p);
				}
			}
		}
	}

	return true;
}

void BassDecoder::Close()
{
	if (!m_stream) {
		return;
	}

	if (m_sync) {
		BASS_ChannelRemoveSync(m_stream, m_sync);
	}

	if (m_isMOD) {
		BASS_MusicFree(m_stream);
	} else {
		BASS_StreamFree(m_stream);
	}

	m_channels = 0;
	m_sampleRate = 0;
	m_bytesPerSample = 0;
	m_float = false;
	m_mSecConv = 0;

	m_stream = 0;
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
	m_type       = info.ctype;

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

	DLog(L"Opened '%s' file", GetBassTypeStr(info.ctype));

	GetHTTPInfos();

	return true;
}

void BassDecoder::GetNameTag(LPCSTR string)
{
	if (!m_shoutcastEvents) {
		return;
	}

	std::wstring strTags = ConvertAnsiToWide(string);

	LPCWSTR astring = strTags.c_str();
	while (astring && *astring) {
		LPCWSTR tag = astring;
		if (wcsncmp(L"icy-name:", tag, 9) == 0) {
			tag += 9;
			while (*tag && iswspace(*tag)) {
				tag++;
			}
			//if (m_shoutcastEvents)
			m_shoutcastEvents->OnShoutcastMetaDataCallback(tag);
		}

		astring += wcslen(astring) + 1;
	}
}

void BassDecoder::GetHTTPInfos()
{
	LPCSTR icyTags;
	LPCSTR httpHeaders;

	if (!m_isShoutcast) {
		return;
	}

	icyTags = BASS_ChannelGetTags(m_stream, BASS_TAG_ICY);
	if (icyTags) {
		GetNameTag(icyTags);
	}

	httpHeaders = BASS_ChannelGetTags(m_stream, BASS_TAG_HTTP);
	if (httpHeaders) {
		GetNameTag(httpHeaders);
	}

	LPCSTR metaTagsUtf8 = BASS_ChannelGetTags(m_stream, BASS_TAG_META);
	if (metaTagsUtf8) {
		std::wstring metaTags = ConvertUtf8ToWide(metaTagsUtf8);
		DLog(L"Received Meta Tag: %s", metaTags.c_str());

		size_t k1 = metaTags.find(L"StreamTitle='");
		if (k1 != metaTags.npos) {
			k1 += 13;
			size_t k2 = metaTags.find(L'\'', k1);
			if (k2 != metaTags.npos) {
				const std::wstring title = metaTags.substr(k1, k2 - k1);
				m_shoutcastEvents->OnShoutcastMetaDataCallback(title.c_str());
			}
		}
		else if ((k1 = metaTags.find(L"TITLE=") != metaTags.npos) ||
			(k1 = metaTags.find(L"Title=") != metaTags.npos) ||
			(k1 = metaTags.find(L"title=") != metaTags.npos)) {
			k1 += 6;
			m_shoutcastEvents->OnShoutcastMetaDataCallback(metaTags.c_str() + k1);
		}
	}
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
