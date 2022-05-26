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

#include "StdAfx.h"
#include "Common.h"
#include "BassDecoder.h"
#include "BassSource.h"
#include "BassAudioSource.h"
#include <MMReg.h>
#include "Utils/StringUtil.h"

#define OPT_REGKEY_BassAudioSource L"Software\\MPC-BE Filters\\BassAudioSource"
#define OPT_BuffersizeMS           L"BuffersizeMS"
#define OPT_PreBufferMS            L"PreBufferMS"

volatile LONG InstanceCount = 0;

//
// BassSource
//

BassSource::BassSource(LPCWSTR name, IUnknown* unk, REFCLSID clsid, HRESULT& hr)
	: CSource(name, unk, clsid, &hr)
{
	Init();
}

BassSource::BassSource(CFactoryTemplate* factory, LPUNKNOWN controller)
	: CSource(factory->m_Name, controller, CLSID_BassAudioSource, nullptr)
{
	Init();
}

BassSource::~BassSource()
{
	InterlockedDecrement(&InstanceCount);

	if (this->pin) {
		delete this->pin;
		this->pin = nullptr;
	}

	delete this->metaLock;

	if (this->fileName) {
		free((void*)this->fileName);
	}

	SaveSettings();
}

void BassSource::Init()
{
	this->metaLock = new CCritSec();

	this->buffersizeMS = PREBUFFER_MAX_SIZE;
	this->preBufferMS = this->buffersizeMS * 75 / 100;

	LoadSettings();

	InterlockedIncrement(&InstanceCount);
}

void STDMETHODCALLTYPE BassSource::OnShoutcastMetaDataCallback(LPCWSTR text)
{
	this->metaLock->Lock();
	__try {
		m_currentTag = text;
	}
	__finally {
		this->metaLock->Unlock();
	}
}

void STDMETHODCALLTYPE BassSource::OnShoutcastBufferCallback(const void* buffer, DWORD size)
{
}

bool RegReadDword(HKEY key, LPCWSTR name, DWORD& dwValue)
{
	DWORD dwType;
	ULONG nBytes = sizeof(DWORD);

	LONG lRes = ::RegQueryValueExW(key, name, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &nBytes);
	
	return (lRes == ERROR_SUCCESS && dwType == REG_DWORD);
}

void RegWriteDword(HKEY key, LPCWSTR name, DWORD dwValue)
{
	LONG lRes = ::RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));

	return;
}

void BassSource::LoadSettings()
{
	HKEY reg;
	DWORD num;

	if (RegOpenKeyW(HKEY_CURRENT_USER, OPT_REGKEY_BassAudioSource, &reg) == ERROR_SUCCESS)
	{
		__try {
			if (RegReadDword(reg, OPT_BuffersizeMS, num)) {
				this->buffersizeMS = std::clamp<int>(num, PREBUFFER_MIN_SIZE, PREBUFFER_MAX_SIZE);
			}

			if (RegReadDword(reg, OPT_PreBufferMS, num)) {
				this->preBufferMS = std::clamp<int>(num, PREBUFFER_MIN_SIZE, this->buffersizeMS);
			}
		}
		__finally {
			RegCloseKey(reg);
		}
	}
}

void BassSource::SaveSettings()
{
	HKEY reg;

	if (RegCreateKeyW(HKEY_CURRENT_USER, OPT_REGKEY_BassAudioSource, &reg) == ERROR_SUCCESS)
	{
		__try {
			RegWriteDword(reg, OPT_BuffersizeMS, this->buffersizeMS);
			RegWriteDword(reg, OPT_PreBufferMS, this->preBufferMS);
		}
		__finally {
			RegCloseKey(reg);
		}
	}
}

STDMETHODIMP BassSource::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
	if (IsEqualIID(iid, IID_IFileSourceFilter)/* || IsEqualIID(iid, IID_ISpecifyPropertyPages)*/) {
		if (SUCCEEDED(GetInterface((LPUNKNOWN)(IFileSourceFilter*)this, ppv))) {
			return S_OK;
		} else {
			return E_NOINTERFACE;
		}
	}
	else if (IsEqualIID(iid, IID_IAMMediaContent)) {
		if (SUCCEEDED(GetInterface((LPUNKNOWN)(IAMMediaContent*)this, ppv))) {
			return S_OK;
		} else {
			return E_NOINTERFACE;
		}
	}
	else {
		return CSource::NonDelegatingQueryInterface(iid, ppv);
	}
}

// IFileSourceFilter

STDMETHODIMP BassSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	static LPCSTR bass_exts[] = {
		// bass.dll
		"mp3", "mp2", "mp1", "ogg", "wav", "aif", "aiff",
		"it", "mod", "mptm", "mtm", "s3m", "umx", "xm", "mo3",
		// bass_aac.dll
		"aac", "m4a",
		// bass_mpc.dll
		"mpc",
		// bass_ofr.dll
		"ofr",
		// bass_tta.dll
		"tta",
		// bassalac.dll
		"alac"
		// bassape.dll
		"ape",
		// bassflac.dll
		"flac",
		// bassopus.dll
		"opus",
		// basswv.dll
		"wv",
		// basszxtune.dll
		"ahx", "ay", "nsf", "pt2", "pt3", "sap", "sid", "spc", "stc", "v2m", "ym"
	};


	HRESULT hr;

	if (GetPinCount() > 0) {
		return VFW_E_ALREADY_CONNECTED;
	}

	if (pszFileName) {
		WCHAR PathBuffer[MAX_PATH + 1] = {};

		GetFileExt(pszFileName, PathBuffer);
		if (PathBuffer[0] == L'.') {
			std::string ext = ConvertWideToANSI(std::wstring(&PathBuffer[1]));

			size_t i = 0;
			for (; i < std::size(bass_exts); i++) {
				if (ext.compare(bass_exts[i]) == 0) {
					break;
				}
			}

			if (i == std::size(bass_exts)) {
				return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
			}
		}
	}

	this->pin = new BassSourceStream(L"Bass Source Stream", hr, this, L"Output", pszFileName, this, this->buffersizeMS, this->preBufferMS);
	if (FAILED(hr) || !this->pin) {
		return hr;
	}

	this->fileName = _wcsdup(pszFileName ? pszFileName : L"");

	if (!this->pin->decoder->GetIsShoutcast()) {
		WCHAR PathBuffer[MAX_PATH + 1];
		m_currentTag = GetFileName(this->fileName, PathBuffer);
	}

	return S_OK;
}

STDMETHODIMP BassSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	return AMGetWideString(this->fileName, ppszFileName);
}

/*
(*** ISpecifyPropertyPages ****************************************************)
(*** IBassAudioSource ************************************************************)
(*** IDispatch ****************************************************************)
*/

// IAMMediaContent

STDMETHODIMP BassSource::get_Title(THIS_ BSTR FAR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	this->metaLock->Lock();

	__try {
		*pbstrTitle = SysAllocString(m_currentTag.c_str());
	}
	__finally {
		this->metaLock->Unlock();
	}

	if (!*pbstrTitle) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}
