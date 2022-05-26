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

//BassDecoder.h

#include <DShow.h>
#include <../Include/bass.h>

#pragma once

class ShoutcastEvents
{
public:
	virtual void STDMETHODCALLTYPE OnShoutcastMetaDataCallback(LPCWSTR text) = 0;
	virtual void STDMETHODCALLTYPE OnShoutcastBufferCallback(const void* buffer, DWORD size) = 0;
};

class BassDecoder
{
protected:
	//Use shoutcastEvents instead of FMetaDataCallback and FBufferCallback
	ShoutcastEvents* m_shoutcastEvents;
	int m_buffersizeMS;
	int m_prebufferMS;

	HMODULE m_optimFROGDLL = nullptr;
	HSTREAM m_stream = 0;
	HSYNC m_sync = 0;
	bool m_isMOD = false;
	bool m_isURL = false;
	bool m_isShoutcast = false;

	int m_channels = 0;
	int m_sampleRate = 0;
	int m_bytesPerSample = 0;
	bool m_float = false;
	LONGLONG m_mSecConv = 0;

	DWORD m_type = 0;

	void LoadBASS();
	void UnloadBASS();
	void LoadPlugins();

	bool GetStreamInfos();
	void GetHTTPInfos();
	void GetNameTag(LPCSTR string);
public:
	LONGLONG GetDuration();
	LONGLONG GetPosition();
	void SetPosition(LONGLONG positionMS);
	LPCWSTR GetExtension();

public:
	BassDecoder(ShoutcastEvents* shoutcastEvents, int buffersizeMS, int prebufferMS);
	~BassDecoder();

	bool Load(LPCWSTR fileName);
	void Close();

	int GetData(void* buffer, int size);

	inline int GetChannels()       { return m_channels; }
	inline int GetSampleRate()     { return m_sampleRate; }
	inline int GetBytesPerSample() { return m_bytesPerSample; }
	inline bool GetFloat()         { return m_float; }
	inline LONGLONG GetMSecConv()  { return m_mSecConv; }
	inline bool GetIsShoutcast()   { return m_isShoutcast; }
	friend void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void* user);
	friend void CALLBACK OnShoutcastData(const void* buffer, DWORD length, void* user);
};

LPWSTR GetFilterDirectory(LPWSTR folder);
