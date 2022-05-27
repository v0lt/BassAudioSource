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
#include <Dshow.h>
#include <InitGuid.h>
#include "BassSourceStream.h"
//#include <Dllsetup.h>
#include "BassAudioSource.h"
#include "BassSource.h"
#include "Common.h"

#define REGISTERING_FILE_EXTENSIONS 0

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
	nullptr,                // Connects to pin
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
	{L".aac",  L"bass_aac.dll"},
	{L".alac", L"bassalac.dll"},
	{L".als",  L"bassalac.dll"},
	{L".ape",  L"bassape.dll"},
	{L".flac", L"bassflac.dll"},
	{L".m4a",  L"bass_aac.dll"},
	{L".mp4",  L"bass_aac.dll"},
	{L".mac",  L"bassape.dll"},
	{L".mp3",  L"bass.dll"},
	{L".ogg",  L"bass.dll"},
	{L".opus", L"bassopus.dll"},
	{L".mpc",  L"bass_mpc.dll"},
	{L".wv",   L"basswv.dll"},
	{L".tta",  L"bass_tta.dll"},
	{L".ofr",  L"bass_ofr.dll"},
	{L".it",   L"bass.dll"},
	{L".mo3",  L"bass.dll"},
	{L".mod",  L"bass.dll"},
	{L".mtm",  L"bass.dll"},
	{L".s3m",  L"bass.dll"},
	{L".umx",  L"bass.dll"},
	{L".xm",   L"bass.dll"},
	{L".pt2",  L"basszxtune.dll"},
	{L".pt3",  L"basszxtune.dll"},
};
const int BASS_EXTENSIONS_COUNT = (int)std::size(BASS_EXTENSIONS);

LPWSTR BASS_PLUGINS[] = {
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
	L"basswv.dll",
	L"basszxtune.dll",
};
const int BASS_PLUGINS_COUNT = (int)std::size(BASS_PLUGINS);

// COM global table of objects in this dll

CUnknown * WINAPI CreateBassAudioSourceInstance(LPUNKNOWN lpunk, HRESULT *phr);

CFactoryTemplate g_Templates[] = {
	{ LABEL_BassAudioSource
	, &CLSID_BassAudioSource
	, CreateBassAudioSourceInstance
	, nullptr
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

bool RegReadString(HKEY key, LPCWSTR name, LPWSTR value, unsigned len)
{
	DWORD type;
	DWORD cbuf = len * sizeof(WCHAR);

	LONG lRes = ::RegQueryValueExW(key, name, nullptr, &type, (LPBYTE)value, &cbuf);

	if (lRes == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
		return true;
	}

	return false;
}

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
#if REGISTERING_FILE_EXTENSIONS
	HKEY reg, reg2;
	LPCWSTR ext;
	LPWSTR dllPath;
	LPWSTR plugin;
	WCHAR PathBuffer2[MAX_PATH + 1];

	dllPath = GetFilterDirectory(PathBuffer2);
	plugin = dllPath + wcslen(dllPath);

	if (RegCreateKeyExW(HKEY_CLASSES_ROOT, DIRECTSHOW_SOURCE_FILTER_PATH, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &reg, nullptr) == ERROR_SUCCESS)
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

						if (RegCreateKeyExW(reg, ext, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &reg2, nullptr) == ERROR_SUCCESS) {
							__try {
								RegWriteString(reg2, L"Source Filter", STR_CLSID_BassAudioSource);

								// Special handling of MP3 Files
								if (lstrcmpiW(ext, L".mp3") == 0) {
									RegWriteString(reg2, L"Media Type", L"{E436EB83-524F-11CE-9F53-0020AF0BA770}"); // MEDIATYPE_Stream
									RegWriteString(reg2, L"Subtype", L"{E436EB87-524F-11CE-9F53-0020AF0BA770}"); // MEDIASUBTYPE_MPEG1Audio
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
#else
	HKEY hKey;
	LONG ec = ::RegCreateKeyExW(HKEY_CLASSES_ROOT, L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}\\" STR_CLSID_BassAudioSource, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if (ec == ERROR_SUCCESS) {
		const LPCWSTR value_data0 = STR_CLSID_BassAudioSource;
		const LPCWSTR value_data1 = L"0,1,00,00"; // connect to any data

		ec = ::RegSetValueExW(hKey, L"Source Filter", 0, REG_SZ,
			reinterpret_cast<BYTE*>(const_cast<LPWSTR>(value_data0)),
			(DWORD)(wcslen(value_data0) + 1) * sizeof(WCHAR));

		ec = ::RegSetValueExW(hKey, L"1", 0, REG_SZ,
			reinterpret_cast<BYTE*>(const_cast<LPWSTR>(value_data1)),
			(DWORD)(wcslen(value_data1) + 1) * sizeof(WCHAR));

		::RegCloseKey(hKey);;
	}
#endif

	return AMovieDllRegisterServer2(TRUE);
}

//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
#if REGISTERING_FILE_EXTENSIONS
	HKEY reg;
	LPCWSTR ext;
	HKEY reg2;
	WCHAR TextBuffer[1024];

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
						if (!RegReadString(reg2, L"Source Filter", TextBuffer, (unsigned)std::size(TextBuffer)))
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
								RegWriteString(reg2, L"Source Filter", L"{E436EBB5-524F-11CE-9F53-0020AF0BA770}"); // CLSID_AsyncReader
								RegWriteString(reg2, L"Media Type", L"{E436EB83-524F-11CE-9F53-0020AF0BA770}"); // MEDIATYPE_Stream
								RegWriteString(reg2, L"Subtype", L"{E436EB87-524F-11CE-9F53-0020AF0BA770}"); // MEDIASUBTYPE_MPEG1Audio
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
#else
	HKEY hKey;
	LONG ec = ::RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", 0, KEY_ALL_ACCESS, &hKey);
	if (ec == ERROR_SUCCESS) {
		ec = ::RegDeleteKeyW(hKey, STR_CLSID_BassAudioSource);
		::RegCloseKey(hKey);
	}
#endif

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
