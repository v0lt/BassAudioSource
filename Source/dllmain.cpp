/*
 *  Copyright (C) 2022-2023 v0lt
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
#include <InitGuid.h>
#include "BassSource.h"
#include "dllmain.h"

#define STR_GUID_REGISTRY "{FFFB1509-D0C1-4E23-8DAC-4BF554615BB6}" // need a large enough value to be at the end of the list

HMODULE HInstance;

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
	CUnknown* punk = new(std::nothrow) T(lpunk, phr);
	if (punk == nullptr) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinType = {
	&MEDIATYPE_Audio,       // Major type
	&MEDIASUBTYPE_PCM       // Minor type
};

const AMOVIESETUP_PIN sudOpPin = {
	(LPWSTR)L"Output",      // Pin string name
	FALSE,                  // Is it rendered
	TRUE,                   // Is it an output
	FALSE,                  // Can we have none
	FALSE,                  // Can we have many
	&CLSID_NULL,            // Connects to filter
	nullptr,                // Connects to pin
	1,                      // Number of types
	&sudOpPinType           // Pin details
};

const AMOVIESETUP_FILTER sudFilter = {
	&__uuidof(BassSource),  // Filter CLSID
	LABEL_BassAudioSource,  // String name
	MERIT_UNLIKELY,         // Filter merit
	1,                      // Number pins
	&sudOpPin,              // Pin details
	CLSID_LegacyAmFilterCategory
};

// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
	{ sudFilter.strName, sudFilter.clsID, CreateInstance<BassSource>, nullptr, &sudFilter },
};
int g_cTemplates = (int)std::size(g_Templates);

void ClearRegistry()
{
	HKEY hKey;
	LONG ec = ::RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}", 0, KEY_ALL_ACCESS, &hKey);
	if (ec == ERROR_SUCCESS) {
		ec = ::RegDeleteKeyW(hKey, _CRT_WIDE(STR_CLSID_BassAudioSource)); // purge registration of old versions
		ec = ::RegDeleteKeyW(hKey, _CRT_WIDE(STR_GUID_REGISTRY));
		::RegCloseKey(hKey);
	}
}

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
	ClearRegistry();

	HKEY hKey;
	LONG ec = ::RegCreateKeyExW(HKEY_CLASSES_ROOT, L"Media Type\\{e436eb83-524f-11ce-9f53-0020af0ba770}\\" STR_GUID_REGISTRY, 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, 0);
	if (ec == ERROR_SUCCESS) {
		const LPCWSTR value_data0 = _CRT_WIDE(STR_CLSID_BassAudioSource);
		const LPCWSTR value_data1 = L"0,1,00,00"; // connect to any data

		ec = ::RegSetValueExW(hKey, L"Source Filter", 0, REG_SZ,
			reinterpret_cast<BYTE*>(const_cast<LPWSTR>(value_data0)),
			(DWORD)(wcslen(value_data0) + 1) * sizeof(WCHAR));

		ec = ::RegSetValueExW(hKey, L"1", 0, REG_SZ,
			reinterpret_cast<BYTE*>(const_cast<LPWSTR>(value_data1)),
			(DWORD)(wcslen(value_data1) + 1) * sizeof(WCHAR));

		::RegCloseKey(hKey);;
	}

	return AMovieDllRegisterServer2(TRUE);
}

//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	ClearRegistry();

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
