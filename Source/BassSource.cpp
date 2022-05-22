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

volatile LONG InstanceCount = 0;

//
// BassSource
//

BassSource::BassSource(LPCWSTR name, IUnknown* unk, REFCLSID clsid, HRESULT& hr)
	: CSource(name, unk, clsid, &hr)
	, pin(nullptr)
	, currentTag(nullptr)
	, fileName(nullptr)
{
	Init();
}

void BassSource::Init()
{
	this->metaLock = new CCritSec();

	this->buffersizeMS = PREBUFFER_MAX_SIZE;
	this->preBufferMS = this->buffersizeMS * 75 / 100;

	LoadSettings();

	InterlockedIncrement(&InstanceCount);
}

BassSource::BassSource(CFactoryTemplate* factory, LPUNKNOWN controller)
	: CSource(factory->m_Name, controller, CLSID_BassAudioSource, nullptr)
	, pin(nullptr)
	, currentTag(nullptr)
	, fileName(nullptr)
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

	if (this->currentTag) {
		free((void*)this->currentTag);
	}
	if (this->fileName) {
		free((void*)this->fileName);
	}

	SaveSettings();
}

void BassSource::SetCurrentTag(LPCWSTR tag)
{
	if (this->currentTag) {
		free((void*)this->currentTag);
	}
	this->currentTag = _wcsdup(tag);
}

void STDMETHODCALLTYPE BassSource::OnShoutcastMetaDataCallback(LPCWSTR text)
{
	this->metaLock->Lock();
	__try {
		CurrentTag = text;
	}
	__finally {
		this->metaLock->Unlock();
	}
}

void STDMETHODCALLTYPE BassSource::OnShoutcastBufferCallback(const void* buffer, DWORD size)
{
}

bool RegReadInteger(HKEY key, LPCWSTR name, int* value)
{
	BYTE buf[MAX_PATH];
	memset(buf, 0, MAX_PATH);
	DWORD type;
	DWORD len = MAX_PATH;

	if (RegQueryValueExW(key, name, nullptr, &type, buf, &len) != ERROR_SUCCESS) {
		return false;
	}
	if (!value) {
		return true;
	}

	switch (type) {
	case REG_DWORD:
		*value = *((int*)buf);
		return true;
	case REG_BINARY:
	case REG_QWORD:
		*value = (int)*((LONGLONG*)buf);
		return true;
	case REG_EXPAND_SZ:
	case REG_SZ:
		*value = _wtoi((LPCWSTR)buf);
		return true;
	default:
		return false;
	}
}

void RegWriteInteger(HKEY key, LPCWSTR name, int value)
{
	BYTE buf[MAX_PATH];
	DWORD type;
	DWORD len = MAX_PATH;
	if (RegQueryValueExW(key, name, nullptr, &type, nullptr, nullptr) == ERROR_FILE_NOT_FOUND) {
		type = REG_DWORD;
	}

	switch (type) {
	case REG_DWORD:
	case REG_BINARY:
		*((DWORD*)buf) = value;
		RegSetValueExW(key, name, 0, type, buf, sizeof(DWORD));
		break;
	case REG_QWORD:
		*((LONGLONG*)buf) = value;
		RegSetValueExW(key, name, 0, type, buf, sizeof(LONGLONG));
		break;
	case REG_EXPAND_SZ:
	case REG_SZ:
		_itow(value, (LPWSTR)buf, 10);
		RegSetValueExW(key, name, 0, type, buf, DWORD((wcslen((LPWSTR)buf) + 1) * sizeof(WCHAR)));
		break;
	}
}

bool RegReadBool(HKEY key, LPCWSTR name, bool* value)
{
	LONGLONG buf = 0;
	DWORD type;
	DWORD len = sizeof(LONGLONG);
	LPCWSTR s;

	if (RegQueryValueExW(key, name, nullptr, &type, (LPBYTE)&buf, &len) != ERROR_SUCCESS) {
		return false;
	}
	if (!value) {
		return true;
	}

	switch (type) {
	case REG_DWORD:
		*value = !!*((int*)buf);
		return true;
	case REG_BINARY:
	case REG_QWORD:
		*value = !!*((LONGLONG*)buf);
		return true;
	case REG_EXPAND_SZ:
	case REG_SZ:
		s = (LPCWSTR)buf;
		*value = *s != '0';
		return wcslen(s) == 1 && (*s == '0' || *s == '1');
	default:
		return false;
	}
}

void RegWriteBool(HKEY key, LPCWSTR name, bool value)
{
	LONGLONG buf = 0;
	DWORD type;
	DWORD len = sizeof(LONGLONG);

	if (RegQueryValueExW(key, name, nullptr, &type, nullptr, nullptr) == ERROR_FILE_NOT_FOUND) {
		type = REG_DWORD;
	}

	switch (type) {
	case REG_DWORD:
	case REG_BINARY:
		*((DWORD*)buf) = value ? 1 : 0;
		RegSetValueExW(key, name, 0, type, (const BYTE*)&buf, sizeof(DWORD));
		break;
	case REG_QWORD:
		*((LONGLONG*)buf) = value ? 1L : 0L;
		RegSetValueExW(key, name, 0, type, (const BYTE*)&buf, sizeof(LONGLONG));
		break;
	case REG_EXPAND_SZ:
	case REG_SZ:
		LPWSTR s = (LPWSTR)buf;
		s[0] = value ? '1' : '0';
		s[1] = 0;
		RegSetValueExW(key, name, 0, type, (const BYTE*)&buf, 2 * sizeof(WCHAR));
		break;
	}
}

void BassSource::LoadSettings()
{
	HKEY reg;
	int num;

	if (RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\MPC-BE Filters\\BassAudioSource", &reg) == ERROR_SUCCESS)
	{
		__try {
			if (RegReadInteger(reg, L"BuffersizeMS", &num))
				this->buffersizeMS = std::clamp(num, PREBUFFER_MIN_SIZE, PREBUFFER_MAX_SIZE);

			if (RegReadInteger(reg, L"PreBufferMS", &num))
				this->preBufferMS = std::clamp(num, PREBUFFER_MIN_SIZE, PREBUFFER_MAX_SIZE);

			//if (RegReadBool(reg, L"SplitStream", &flag))
			//  this->splitStream = flag;

		}
		__finally {
			RegCloseKey(reg);
		}
	}
}

void BassSource::SaveSettings()
{
	HKEY reg;

	if (RegCreateKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\MPC-BE Filters\\BassAudioSource", &reg) == ERROR_SUCCESS)
	{
		__try {
			RegWriteInteger(reg, L"BuffersizeMS", this->buffersizeMS);
			RegWriteInteger(reg, L"PreBufferMS", this->preBufferMS);
			//RegWriteBool   (reg, L"SplitStream",  this->splitStream);
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
		}
		else {
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
	HRESULT hr;

	if (GetPinCount() > 0) {
		return VFW_E_ALREADY_CONNECTED;
	}

	if (pszFileName) {
		LPWSTR ext;
		WCHAR PathBuffer[MAX_PATH + 1];

		ext = GetFileExt(pszFileName, PathBuffer);
		
		if (lstrcmpiW(ext, L".avi") == 0
			|| lstrcmpiW(ext, L".mp4") == 0
			|| lstrcmpiW(ext, L".ts") == 0) {

			return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
		}
	}

	this->pin = new BassSourceStream(L"Bass Source Stream", hr, this, L"Output", pszFileName, this, this->buffersizeMS, this->preBufferMS);
	if (FAILED(hr) || !this->pin) {
		return hr;
	}

	this->fileName = _wcsdup(pszFileName ? pszFileName : L"");

	if (!this->pin->decoder->IsShoutcast) {
		WCHAR PathBuffer[MAX_PATH + 1];
		CurrentTag = GetFileName(this->fileName, PathBuffer);
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
		*pbstrTitle = SysAllocString(CurrentTag);
	}
	__finally {
		this->metaLock->Unlock();
	}

	if (!*pbstrTitle) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}
