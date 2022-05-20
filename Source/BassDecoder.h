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

/*
interface
*/

#include <DShow.h>
#include <../Include/bass.h>

#pragma once

class ShoutcastEvents {
public:
/*
  TShoutcastMetaDataCallback = procedure(AText: String) of Object;
*/
  virtual void STDMETHODCALLTYPE OnShoutcastMetaDataCallback(LPCWSTR text) = 0;
/*
  TShoutcastBufferCallback   = procedure(ABuffer: PByte; ASize: Integer) of Object;
*/
  virtual void STDMETHODCALLTYPE OnShoutcastBufferCallback(const void *buffer, DWORD size) = 0;
};

class BassDecoder {/*
  TBassDecoder = class
*/protected:
    //BASS_Init:                  function(device: Integer; freq, flags: DWORD; win: HWND; clsid: PGUID): BOOL; stdcall;
    //BASS_Free:                  function: BOOL; stdcall;
    //BASS_PluginLoad:            function(filename: Pointer; flags: DWORD): Cardinal; stdcall;
    //BASS_MusicLoad:             function(mem: BOOL; f: Pointer; offset: Int64; length, flags, freq: DWORD): Cardinal; stdcall;
    //BASS_StreamCreateURL:       function(url: Pointer; offset: DWORD; flags: DWORD; proc: Pointer; user: Pointer): Cardinal; stdcall;
    //BASS_StreamCreateFile:      function(mem: BOOL; f: Pointer; offset, length: Int64; flags: DWORD): Cardinal; stdcall;
    //BASS_MusicFree:             function(handle: Cardinal): BOOL; stdcall;
    //BASS_StreamFree:            function(handle: Cardinal): BOOL; stdcall;
    //BASS_ChannelGetData:        function(handle: DWORD; buffer: Pointer; length: DWORD): DWORD; stdcall;
    //BASS_ChannelGetInfo:        function(handle: DWORD; var info: BASS_CHANNELINFO): BOOL;stdcall;
    //BASS_ChannelGetLength:      function(handle, mode: DWORD): Int64; stdcall;
    //BASS_ChannelSetPosition:    function(handle: DWORD; pos: Int64; mode: DWORD): BOOL; stdcall;
    //BASS_ChannelGetPosition:    function(handle, mode: DWORD): Int64; stdcall;
    //BASS_ChannelSetSync:        function(handle: DWORD; type_: DWORD; param: int64; proc: Pointer; user: Pointer): DWORD; stdcall;
    //BASS_ChannelRemoveSync:     function(handle: DWORD; sync: DWORD): BOOL; stdcall;
    //BASS_ChannelGetTags:        function(handle: DWORD; tags: DWORD): PChar; stdcall;
    //BASS_SetConfig:             function(option, value: DWORD): DWORD; stdcall;

    //Use shoutcastEvents instead of FMetaDataCallback and FBufferCallback
    ShoutcastEvents* shoutcastEvents;
    int buffersizeMS;//: Integer;
    int prebufferMS;//: Integer;

    //FLibrary: THandle;
    HMODULE optimFROGDLL;//: THandle;
    HSTREAM stream;//: Cardinal;
    HSYNC sync;//: Cardinal;
    bool isMOD;//: Boolean;
    bool isURL;//: Boolean;

    int channels;//: Integer;
    int sampleRate;//: Integer;
    int bytesPerSample;//: Integer;
    bool _float;//: Boolean;
    LONGLONG mSecConv;//: Int64;
    bool isShoutcast;//: Boolean;
    DWORD type;//: Cardinal;

    void LoadBASS();
    void UnloadBASS();
    void LoadPlugins();

    bool GetStreamInfos();
    void GetHTTPInfos();
    void GetNameTag(LPCSTR string);

    public: LONGLONG GetDuration();
    public: LONGLONG GetPosition();
    public: void SetPosition(LONGLONG positionMS);
    public: LPCWSTR GetExtension();
  public:
    BassDecoder(ShoutcastEvents* shoutcastEvents, int buffersizeMS, int prebufferMS);
    ~BassDecoder();

    bool Load(LPCWSTR fileName);
    void Close();

    int GetData(void *buffer, int size);
  public:
    __declspec(property(get = GetDuration)) LONGLONG DurationMS;
    __declspec(property(get = GetPosition, put = SetPosition)) LONGLONG PositionMS;

    __declspec(property(get = GetChannels)) int Channels;
    __declspec(property(get = GetSampleRate)) int SampleRate;
    __declspec(property(get = GetBytesPerSample)) int BytesPerSample;
    __declspec(property(get = GetFloat)) bool Float;
    __declspec(property(get = GetMSecConv)) LONGLONG MSecConv;
    __declspec(property(get = GetIsShoutcast)) bool IsShoutcast;
    __declspec(property(get = GetExtension)) LPWSTR Extension;
  public:
    inline int GetChannels() { return this->channels; }
    inline int GetSampleRate() { return this->sampleRate; }
    inline int GetBytesPerSample() { return this->bytesPerSample; }
    inline bool GetFloat() { return this->_float; }
    inline LONGLONG GetMSecConv() { return this->mSecConv; }
    inline bool GetIsShoutcast() { return this->isShoutcast; }
    friend void CALLBACK OnMetaData(HSYNC handle, DWORD channel, DWORD data, void *user);
    friend void CALLBACK OnShoutcastData(const void *buffer, DWORD length, void *user);
};//end;

LPWSTR GetFilterDirectory(LPWSTR folder);
