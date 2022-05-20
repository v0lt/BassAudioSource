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

/*
interface
*/

#include "stdafx.h"
#include <Dshow.h>
#include <InitGuid.h>
#include "BassSourceStream.h"
//#include <Dllsetup.h>
#include "BassAudioSource.h"
#include "BassSource.h"
#include "Common.h"

HMODULE HInstance;

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes = {
	&MEDIATYPE_Audio,       // Major type
	&MEDIASUBTYPE_PCM       // Minor type
};

const AMOVIESETUP_PIN sudOpPin = {
	L"Output",              // Pin string name
	FALSE,                  // Is it rendered
	TRUE,                   // Is it an output
	FALSE,                  // Can we have none
	FALSE,                  // Can we have many
	&CLSID_NULL,            // Connects to filter
	NULL,                   // Connects to pin
	1,                      // Number of types
	&sudOpPinTypes          // Pin details
};

const AMOVIESETUP_FILTER sudBassAudioSourceax = {
	&CLSID_BassAudioSource, // Filter CLSID
	LABEL_BassAudioSource,  // String name
	MERIT_UNLIKELY,         // Filter merit
	1,                      // Number pins
	&sudOpPin               // Pin details
};

BassExtension BASS_EXTENSIONS[] = {
	{L".aac",  false, L"bass_aac.dll"},
	{L".alac", false, L"bass_alac.dll"},
	{L".als",  false, L"bass_alac.dll"},
	{L".ape",  false, L"bass_ape.dll"},
	{L".flac", false, L"bassflac.dll"},
	{L".m4a",  false, L"bass_aac.dll"},
	{L".mp4",  false, L"bass_aac.dll"},
	{L".mac",  false, L"bass_ape.dll"},
	{L".mp3",  false, L"bass.dll"},
	{L".ogg",  false, L"bass.dll"},
	{L".mpc",  false, L"bass_mpc.dll"},
	{L".wv",   false, L"basswv.dll"},
#ifndef _WIN64
	{L".tta",  false, L"bass_tta.dll"},
	{L".ofr",  false, L"bass_ofr.dll"},
#endif
	{L".it",   true, L"bass.dll"},
	{L".mo3",  true, L"bass.dll"},
	{L".mod",  true, L"bass.dll"},
	{L".mtm",  true, L"bass.dll"},
	{L".s3m",  true, L"bass.dll"},
	{L".umx",  true, L"bass.dll"},
	{L".xm",   true, L"bass.dll"}
};
const int BASS_EXTENSIONS_COUNT = (int)std::size(BASS_EXTENSIONS);

LPWSTR BASS_PLUGINS[] = {
	L"bass_aac.dll",
	L"bass_alac.dll",
	L"bass_ape.dll",
	L"bassflac.dll",
	L"bass_mpc.dll",
#ifndef _WIN64
	L"bass_tta.dll",
	L"bass_ofr.dll",
#endif
	L"basswv.dll"
};
const int BASS_PLUGINS_COUNT = (int)std::size(BASS_PLUGINS);

// COM global table of objects in this dll

CUnknown * WINAPI CreateBassAudioSourceInstance(LPUNKNOWN lpunk, HRESULT *phr);

CFactoryTemplate g_Templates[] = {
	{ LABEL_BassAudioSource
	, &CLSID_BassAudioSource
	, CreateBassAudioSourceInstance
	, NULL
	, &sudBassAudioSourceax }
};
int g_cTemplates = (int)std::size(g_Templates);


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
//
////////////////////////////////////////////////////////////////////////

bool FileExists(LPCWSTR fileName)
{
	WIN32_FILE_ATTRIBUTE_DATA info;

	return !!GetFileAttributesExW(fileName, GetFileExInfoStandard, &info);
}

/*
(*** DLL Exports **************************************************************)
function DllGetClassObject(const CLSID, IID: TGUID; var Obj): HResult;
function DllCanUnloadNow: HResult;
*/

bool RegisterFormat(LPCWSTR format, bool exist)
{
	LPWSTR fileName;
	WCHAR PathBuffer2[MAX_PATH + 1];

	fileName = wcscat(GetFilterDirectory(PathBuffer2), REGISTER_EXTENSION_FILE);
	if (!FileExists(fileName)) {
		return false;
	}

	if (*format == '.') {
		format++;
	}

	switch (GetPrivateProfileIntW(L"Register", format, 0, fileName)) {
	case 1:
		return true;
	case 2:
		return !exist;
	default:
		return false;
	}
}

void RegWriteString(HKEY key, LPCWSTR name, LPCWSTR value)
{
	RegSetValueExW(key, name, 0, REG_SZ, (BYTE*)value, DWORD((wcslen(value) + 1) * sizeof(WCHAR)));
}

bool RegReadString(HKEY key, LPCWSTR name, LPWSTR value, int len)
{
	DWORD type;
	DWORD cbuf = len * sizeof(WCHAR);
	if (RegQueryValueExW(key, name, NULL, &type, (LPBYTE)value, &cbuf) != ERROR_SUCCESS) {
		return false;
	}

	switch (type) {
	case REG_EXPAND_SZ:
	case REG_SZ:
		return true;
	default:
		return false;
	}
}

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
	HKEY reg, reg2;
	LPCWSTR ext;
	LPWSTR dllPath;
	LPWSTR plugin;
	WCHAR PathBuffer2[MAX_PATH + 1];

	dllPath = GetFilterDirectory(PathBuffer2);
	plugin = dllPath + wcslen(dllPath);

	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, DIRECTSHOW_SOURCE_FILTER_PATH, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &reg, NULL) == ERROR_SUCCESS)
	{
		__try {
			for (int i = 0; i < BASS_EXTENSIONS_COUNT; i++) {
				ext = BASS_EXTENSIONS[i].Extension;

				if (RegOpenKeyExW(reg, ext, 0, KEY_QUERY_VALUE, &reg2) != ERROR_SUCCESS) {
					reg2 = NULL;
				}
				else {
					RegCloseKey(reg2);
				}

				if (RegisterFormat(ext, reg2 != NULL)) {
					wcscpy(plugin, BASS_EXTENSIONS[i].DLL);

					if (FileExists(dllPath)) {
						//if reg.KeyExists(path)
						RegDeleteKeyW(reg, ext);

						if (RegCreateKeyExW(reg, ext, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &reg2, NULL) == ERROR_SUCCESS) {
							__try {
								RegWriteString(reg2, L"Source Filter", STR_CLSID_BassAudioSource);

								// Special handling of MP3 Files
								if (lstrcmpiW(ext, L".mp3") == 0)
								{
									RegWriteString(reg2, L"Media Type", L"{E436EB83-524F-11CE-9F53-0020AF0BA770}");
									RegWriteString(reg2, L"Subtype", L"{E436EB87-524F-11CE-9F53-0020AF0BA770}");
								}

							}
							__finally {
								RegCloseKey(reg2);
							}
						}
					}
				}
			}
		}
		__finally {
			RegCloseKey(reg);
		}
	}

	return AMovieDllRegisterServer2(TRUE);
}

//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	HKEY reg;
	LPCWSTR ext;
	HKEY reg2;
	WCHAR TextBuffer[1024];
	int TextBufferLength = 1024;

	if (RegOpenKeyW(HKEY_CLASSES_ROOT, DIRECTSHOW_SOURCE_FILTER_PATH, &reg) == ERROR_SUCCESS)
	{
		__try {
			for (int i = 0; i < BASS_EXTENSIONS_COUNT; i++)
			{
				ext = BASS_EXTENSIONS[i].Extension;

				if (RegOpenKeyExW(reg, ext, 0, KEY_QUERY_VALUE, &reg2) != ERROR_SUCCESS) {
					reg2 = NULL;
				}
				else {
					__try {
						if (!RegReadString(reg2, L"Source Filter", TextBuffer, TextBufferLength))
							*TextBuffer = 0;
					}
					__finally {
						RegCloseKey(reg2);
					}

					if (lstrcmpiW(TextBuffer, STR_CLSID_BassAudioSource) != 0) {
						reg2 = NULL;
					}
				}

				if (reg2 != NULL) {
					//if reg.KeyExists(path)
					RegDeleteKeyW(reg, ext);

					// Special handling of MP3 Files
					if (lstrcmpiW(ext, L".mp3") == 0) {
						if (RegCreateKeyW(reg, ext, &reg2)) {
							__try {
								RegWriteString(reg2, L"Source Filter", L"{E436EBB5-524F-11CE-9F53-0020AF0BA770}");
								RegWriteString(reg2, L"Media Type", L"{E436EB83-524F-11CE-9F53-0020AF0BA770}");
								RegWriteString(reg2, L"Subtype", L"{E436EB87-524F-11CE-9F53-0020AF0BA770}");
							}
							__finally {
								RegCloseKey(reg2);
							}
						}
					}
				}
			}

		}
		__finally {
			RegCloseKey(reg);
		}
	}

	return AMovieDllRegisterServer2(FALSE);
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		HInstance = hModule;
	}

	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

CUnknown* WINAPI CreateBassAudioSourceInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	CUnknown* punk = new BassSource(LABEL_BassAudioSource, lpunk, CLSID_BassAudioSource, *phr);
	if (!punk) {
		if (phr) {
			*phr = E_OUTOFMEMORY;
		}
	}

	return punk;
}
