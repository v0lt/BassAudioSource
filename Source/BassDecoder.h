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
	ShoutcastEvents* shoutcastEvents;
	int buffersizeMS;
	int prebufferMS;

	HMODULE optimFROGDLL;
	HSTREAM stream = 0;
	HSYNC sync = 0;
	bool isMOD;
	bool isURL;

	int channels;
	int sampleRate;
	int bytesPerSample;
	bool _float;
	LONGLONG mSecConv = 0;
	bool isShoutcast;
	DWORD type;

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

	__declspec(property(get = GetDuration)) LONGLONG DurationMS;
	__declspec(property(get = GetPosition, put = SetPosition)) LONGLONG PositionMS;

	__declspec(property(get = GetChannels)) int Channels;
	__declspec(property(get = GetSampleRate)) int SampleRate;
	__declspec(property(get = GetBytesPerSample)) int BytesPerSample;
	__declspec(property(get = GetFloat)) bool Float;
	__declspec(property(get = GetMSecConv)) LONGLONG MSecConv;
	__declspec(property(get = GetIsShoutcast)) bool IsShoutcast;
	__declspec(property(get = GetExtension)) LPWSTR Extension;

	inline int GetChannels() { return this->channels; }
	inline int GetSampleRate() { return this->sampleRate; }
	inline int GetBytesPerSample() { return this->bytesPerSample; }
	inline bool GetFloat() { return this->_float; }
	inline LONGLONG GetMSecConv() { return this->mSecConv; }
	inline bool GetIsShoutcast() { return this->isShoutcast; }
	friend void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void* user);
	friend void CALLBACK OnShoutcastData(const void* buffer, DWORD length, void* user);
};

LPWSTR GetFilterDirectory(LPWSTR folder);
