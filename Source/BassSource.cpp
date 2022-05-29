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
#include <MMReg.h>
#include "Utils/StringUtil.h"

#define OPT_REGKEY_BassAudioSource L"Software\\MPC-BE Filters\\BassAudioSource"
#define OPT_BuffersizeMS           L"BuffersizeMS"
#define OPT_PreBufferMS            L"PreBufferMS"

volatile LONG InstanceCount = 0;

//
// BassSource
//

BassSource::BassSource(LPUNKNOWN lpunk, HRESULT* phr)
	: CSource(LABEL_BassAudioSource, lpunk, __uuidof(this))
{
	Init();
}

BassSource::BassSource(CFactoryTemplate* factory, LPUNKNOWN controller)
	: CSource(factory->m_Name, controller, __uuidof(BassSource))
{
	Init();
}

BassSource::~BassSource()
{
	InterlockedDecrement(&InstanceCount);

	if (m_pin) {
		delete m_pin;
		m_pin = nullptr;
	}

	delete m_metaLock;

	SaveSettings();
}

void BassSource::Init()
{
	m_metaLock = new CCritSec();

	m_buffersizeMS = PREBUFFER_MAX_SIZE;
	m_preBufferMS = m_buffersizeMS * 75 / 100;

	LoadSettings();

	InterlockedIncrement(&InstanceCount);
}

void STDMETHODCALLTYPE BassSource::OnShoutcastMetaDataCallback(LPCWSTR text)
{
	m_metaLock->Lock();
	__try {
		m_currentTag = text;
	}
	__finally {
		m_metaLock->Unlock();
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
				m_buffersizeMS = std::clamp<int>(num, PREBUFFER_MIN_SIZE, PREBUFFER_MAX_SIZE);
			}

			if (RegReadDword(reg, OPT_PreBufferMS, num)) {
				m_preBufferMS = std::clamp<int>(num, PREBUFFER_MIN_SIZE, m_buffersizeMS);
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
			RegWriteDword(reg, OPT_BuffersizeMS, m_buffersizeMS);
			RegWriteDword(reg, OPT_PreBufferMS, m_preBufferMS);
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

// simple file system path detector
bool IsLikelyFilePath(const std::wstring_view str)
{
	if (str.size() >= 4) {
		auto s = str.data();

		// local file path
		if (s[1] == ':' && s[2] == '\\' &&
			(s[0] >= 'A' && s[0] <= 'Z' || s[0] >= 'a' && s[0] <= 'z')) {
			return true;
		}

		// net file path
		if (str.size() >= 7 && s[0] == '\\' && s[1] == '\\' &&
			(s[2] == '-' || s[2] >= '0' && s[2] <= '9' || s[2] >= 'A' && s[2] <= 'Z' || s[2] >= 'a' && s[2] <= 'z')) {
			return true;
		}
	}

	return false;
}

// IFileSourceFilter

STDMETHODIMP BassSource::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt)
{
	if (GetPinCount() > 0) {
		return VFW_E_ALREADY_CONNECTED;
	}

	CheckPointer(pszFileName, E_POINTER);

	static LPCSTR bass_exts[] = {
		// bass.dll
		"mp3", "mp2", "mp1", "ogg", "oga", "wav", "aif", "aiff",
		"it", "mod", "mptm", "mtm", "s3m", "umx", "xm", "mo3",
		// bass_aac.dll
		"aac", "m4a",
		// bass_mpc.dll
		"mpc",
		// bass_ofr.dll
		"ofr",
		// bass_spx.dll
		"spx",
		// bass_tta.dll
		"tta",
		// bassalac.dll
		"alac",
		// bassape.dll
		"ape",
		// bassdsd.dll
		"dsf",
		// bassflac.dll
		"flac",
		// bassopus.dll
		"opus",
		// basswv.dll
		"wv",
		// basszxtune.dll
		"ahx", "ay", "gbs", "nsf", "pt2", "pt3",
		"sap", "sid", "spc", "stc",
		"v2m", "vgm", "vgz", "vtx", "ym",
	};

	if (IsLikelyFilePath(pszFileName)) {
		std::wstring wext = std::filesystem::path(pszFileName).extension();

		if (wext.size() <= 1) {
			return VFW_E_CANNOT_LOAD_SOURCE_FILTER;
		}

		ASSERT(wext[0] == L'.');
		wext.erase(0, 1);
		std::string ext = ConvertWideToANSI(wext);

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

	HRESULT hr;
	m_pin = new BassSourceStream(L"Bass Source Stream", hr, this, L"Output", pszFileName, this, m_buffersizeMS, m_preBufferMS);
	if (FAILED(hr) || !m_pin) {
		return hr;
	}

	m_fileName = pszFileName;

	if (!m_pin->m_decoder->GetIsShoutcast()) {
		m_currentTag = std::filesystem::path(m_fileName).filename();
	}

	return S_OK;
}

STDMETHODIMP BassSource::GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt)
{
	CheckPointer(ppszFileName, E_POINTER);

	return AMGetWideString(m_fileName.c_str(), ppszFileName);
}

/*
(*** ISpecifyPropertyPages ****************************************************)
(*** IBassAudioSource ************************************************************)
(*** IDispatch ****************************************************************)
*/

// IAMMediaContent

STDMETHODIMP BassSource::get_AuthorName(THIS_ BSTR FAR* pbstrAuthorName)
{
	CheckPointer(pbstrAuthorName, E_POINTER);

	auto tag = m_pin->m_decoder->GetTagArtist();
	if (tag.empty()) {
		return VFW_E_NOT_FOUND;
	}

	*pbstrAuthorName = SysAllocString(tag.c_str());

	if (!*pbstrAuthorName) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP BassSource::get_Title(THIS_ BSTR FAR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	if (m_pin->m_decoder->GetIsShoutcast()) {
		m_metaLock->Lock();

		*pbstrTitle = SysAllocString(m_currentTag.c_str());
		
		m_metaLock->Unlock();
	}
	else {
		auto tag = m_pin->m_decoder->GetTagTitle();
		if (tag.empty()) {
			return VFW_E_NOT_FOUND;
		}

		*pbstrTitle = SysAllocString(tag.c_str());
	}

	if (!*pbstrTitle) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}

STDMETHODIMP BassSource::get_Description(THIS_ BSTR FAR* pbstrDescription)
{
	CheckPointer(pbstrDescription, E_POINTER);

	auto tag = m_pin->m_decoder->GetTagComment();
	if (tag.empty()) {
		return VFW_E_NOT_FOUND;
	}

	*pbstrDescription = SysAllocString(tag.c_str());

	if (!*pbstrDescription) {
		return E_OUTOFMEMORY;
	}

	return S_OK;
}
