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

#pragma once

#include <../Include/bass.h>
#include <../Include/bassmidi.h>
#include "BassHelper.h"
#include "IBassSource.h"

class ShoutcastEvents
{
public:
	virtual void STDMETHODCALLTYPE OnMetaDataCallback(ContentTags* pTags) = 0;
	virtual void STDMETHODCALLTYPE OnResourceDataCallback(std::unique_ptr<std::list<DSMResource>>& pResources) = 0;
	virtual void STDMETHODCALLTYPE OnShoutcastBufferCallback(const void* buffer, DWORD size) = 0;
};

class BassDecoder
{
	std::vector<HPLUGIN> m_pluggins;

protected:
	//Use shoutcastEvents instead of FMetaDataCallback and FBufferCallback
	ShoutcastEvents* m_shoutcastEvents;

	const std::wstring m_midiSoundFontDefault;

	HMODULE m_optimFROGDLL = nullptr;
	HSTREAM m_stream = 0;
	HSOUNDFONT m_soundFont = 0;
	HSYNC m_syncMeta = 0;
	HSYNC m_syncOggChange = 0;
	bool m_isMOD  = false;
	bool m_isMIDI = false;
	bool m_isURL  = false;
	bool m_isLiveStream = false;

	int m_channels = 0;
	int m_sampleRate = 0;
	int m_bytesPerSample = 0;
	bool m_float = false;
	int m_bytesPerSecond = 0;

	DWORD m_ctype = 0;

	std::wstring m_tagTitle;
	std::wstring m_tagArtist;
	std::wstring m_tagComment;

	void LoadBASS();
	void UnloadBASS();
	void LoadPlugins();

	bool GetStreamInfos();
public:
	LONGLONG GetDuration();
	LONGLONG GetPosition();
	void SetPosition(LONGLONG positionMS);

public:
	BassDecoder(ShoutcastEvents* shoutcastEvents, Settings_t& sets);
	~BassDecoder();

	bool Load(std::wstring path);
	void Close();

	int GetData(void* buffer, int size);

	inline DWORD GetBassCType()    { return m_ctype; }
	inline int GetChannels()       { return m_channels; }
	inline int GetSampleRate()     { return m_sampleRate; }
	inline int GetBytesPerSample() { return m_bytesPerSample; }
	inline int GetBytesPerSecond() { return m_bytesPerSecond; }
	inline bool GetFloat()         { return m_float; }
	inline bool GetIsLiveStream()  { return m_isLiveStream; }

	friend void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void* user);
	friend void CALLBACK OnDownloadData(const void* buffer, DWORD length, void* user);
};

std::wstring GetFilterDirectory();
