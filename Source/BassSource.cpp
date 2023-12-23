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
#include "BassDecoder.h"
#include "BassSource.h"
#include "PropPage.h"
#include <MMReg.h>
#include "Utils/Util.h"
#include "Utils/StringUtil.h"

#define OPT_REGKEY_BassAudioSource L"Software\\MPC-BE Filters\\BassAudioSource"
#define OPT_MidiSoundFontDefault   L"MIDI_SoundFontDefault"

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
}

void BassSource::Init()
{
#ifdef _DEBUG
	DbgSetModuleLevel(LOG_TRACE, DWORD_MAX);
	DbgSetModuleLevel(LOG_ERROR, DWORD_MAX);
#endif
	DLog(L"BassSource::Init()");

	m_metaLock = new CCritSec();

	LoadSettings();

	InterlockedIncrement(&InstanceCount);
}

void STDMETHODCALLTYPE BassSource::OnMetaDataCallback(ContentTags* pTags)
{
	DLog(L"BassSource::OnMetaDataCallback()");
	if (!pTags) {
		return;
	}

	m_metaLock->Lock();
	__try {
		m_Tags = *pTags;
	}
	__finally {
		m_metaLock->Unlock();
	}
}

void STDMETHODCALLTYPE BassSource::OnResourceDataCallback(std::unique_ptr<std::list<DSMResource>>& pResources)
{
	DLog(L"BassSource::OnResourceDataCallback()");
	if (!pResources) {
		return;
	}

	m_metaLock->Lock();

	try {
		m_pResources = std::move(pResources);
	}
	catch (...) {
		DLog(L"BassSource::OnResourceDataCallback() - FAILED!");
		if (m_pResources) {
			m_pResources->clear();
		}
	}

	m_metaLock->Unlock();
}

void STDMETHODCALLTYPE BassSource::OnShoutcastBufferCallback(const void* buffer, DWORD size)
{
}

void BassSource::LoadSettings()
{
	HKEY key;
	DWORD dwType;
	ULONG nBytes;

	LSTATUS lRes = RegOpenKeyW(HKEY_CURRENT_USER, OPT_REGKEY_BassAudioSource, &key);
	if (lRes == ERROR_SUCCESS) {
		/*
		DWORD dwValue;
		nBytes = sizeof(DWORD);
		lRes = ::RegQueryValueExW(key, OPT_BufferSizeMS, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &nBytes);
		if (lRes == ERROR_SUCCESS && dwType == REG_DWORD) {
			m_Sets.iBuffersizeMS = std::clamp<int>(dwValue, PREBUFFER_MIN_SIZE, PREBUFFER_MAX_SIZE);
		}
		*/

		lRes = ::RegQueryValueExW(key, OPT_MidiSoundFontDefault, nullptr, &dwType, nullptr, &nBytes);
		if (lRes == ERROR_SUCCESS && dwType == REG_SZ) {
			std::wstring str(nBytes, 0);
			lRes = ::RegQueryValueExW(key, OPT_MidiSoundFontDefault, nullptr, &dwType, reinterpret_cast<LPBYTE>(str.data()), &nBytes);
			if (lRes == ERROR_SUCCESS && dwType == REG_SZ) {
				str_truncate_after_null(str);
				m_Sets.sMidiSoundFontDefault = str;
			}
		}

		RegCloseKey(key);
	}
}

void BassSource::SaveSettings()
{
	HKEY key;

	LSTATUS lRes = RegCreateKeyW(HKEY_CURRENT_USER, OPT_REGKEY_BassAudioSource, &key);
	if (lRes == ERROR_SUCCESS) {
		/*
		DWORD dwValue = m_Sets.iBuffersizeMS;
		lRes = ::RegSetValueExW(key, OPT_BufferSizeMS, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
		*/

		std::wstring str(m_Sets.sMidiSoundFontDefault);
		lRes = ::RegSetValueExW(key, OPT_MidiSoundFontDefault, 0, REG_SZ, reinterpret_cast<const BYTE*>(str.c_str()), (DWORD)(str.size()+1));

		RegCloseKey(key);
	}
}

STDMETHODIMP BassSource::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
	if (IsEqualIID(iid, IID_IFileSourceFilter)) {
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
	else if (IsEqualIID(iid, __uuidof(IDSMResourceBag))) {
		if (SUCCEEDED(GetInterface((LPUNKNOWN)(IDSMResourceBag*)this, ppv))) {
			return S_OK;
		}
		else {
			return E_NOINTERFACE;
		}
	}
	else if (IsEqualIID(iid, IID_ISpecifyPropertyPages)) {
		if (SUCCEEDED(GetInterface((LPUNKNOWN)(ISpecifyPropertyPages*)this, ppv))) {
			return S_OK;
		}
		else {
			return E_NOINTERFACE;
		}
	}
	else if (IsEqualIID(iid, __uuidof(IBassSource))) {
		if (SUCCEEDED(GetInterface((LPUNKNOWN)(IBassSource*)this, ppv))) {
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
		// basswma.dll
		"wma",
		// basswv.dll
		"wv",
		// basszxtune.dll
		"ahx", "ay", "gbs", "nsf", "pt2", "pt3",
		"sap", "sid", "spc", "stc",
		"v2m", "vgm", "vgz", "vtx", "ym",
		// bassmidi.dll
		"mid", "rmi",
	};

	if (IsLikelyFilePath(pszFileName)) {
		std::wstring wext = std::filesystem::path(pszFileName).extension();
		str_tolower(wext);

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
	m_pin = new BassSourceStream(L"Bass Source Stream", hr, this, L"Output", pszFileName, this, m_Sets);
	if (FAILED(hr) || !m_pin) {
		return hr;
	}

	m_fileName = pszFileName;

	if (!m_pin->m_decoder->GetIsLiveStream() && m_Tags.Empty()) {
		m_Tags.Title = std::filesystem::path(m_fileName).filename();
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

	HRESULT hr = S_OK;

	m_metaLock->Lock();

	if (m_Tags.AuthorName.size()) {
		*pbstrAuthorName = SysAllocString(m_Tags.AuthorName.c_str());
	} else {
		hr = VFW_E_NOT_FOUND;
	}

	m_metaLock->Unlock();

	if (!*pbstrAuthorName) {
		return E_OUTOFMEMORY;
	}

	return hr;
}

STDMETHODIMP BassSource::get_Title(THIS_ BSTR FAR* pbstrTitle)
{
	CheckPointer(pbstrTitle, E_POINTER);

	HRESULT hr = S_OK;

	m_metaLock->Lock();

	if (m_Tags.Title.size()) {
		*pbstrTitle = SysAllocString(m_Tags.Title.c_str());
	} else {
		hr = VFW_E_NOT_FOUND;
	}
		
	m_metaLock->Unlock();

	if (!*pbstrTitle) {
		return E_OUTOFMEMORY;
	}

	return hr;
}

STDMETHODIMP BassSource::get_Description(THIS_ BSTR FAR* pbstrDescription)
{
	CheckPointer(pbstrDescription, E_POINTER);

	HRESULT hr = S_OK;

	m_metaLock->Lock();

	if (m_Tags.Description.size()) {
		*pbstrDescription = SysAllocString(m_Tags.Description.c_str());
	} else {
		hr = VFW_E_NOT_FOUND;
	}

	m_metaLock->Unlock();

	if (!*pbstrDescription) {
		return E_OUTOFMEMORY;
	}

	return hr;
}

// IDSMResourceBag

STDMETHODIMP_(DWORD) BassSource::ResGetCount()
{
	return m_pResources ? (DWORD)m_pResources->size() : 0;
}

STDMETHODIMP BassSource::ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag)
{
	if (ppData) {
		CheckPointer(pDataLen, E_POINTER);
	}

	if (!m_pResources || iIndex >= m_pResources->size()) {
		return E_INVALIDARG;
	}

	auto it = m_pResources->cbegin();
	std::advance(it, iIndex);

	auto& r = *it;

	if (ppName) {
		*ppName = SysAllocString(r.name.c_str());
	}
	if (ppDesc) {
		*ppDesc = SysAllocString(r.desc.c_str());
	}
	if (ppMime) {
		*ppMime = SysAllocString(r.mime.c_str());
	}
	if (ppData) {
		*pDataLen = (DWORD)r.data.size();
		*ppData = (BYTE*)CoTaskMemAlloc(*pDataLen);
		if (*ppData) {
			memcpy(*ppData, r.data.data(), *pDataLen);
		}
	}
	if (pTag) {
		*pTag = r.tag;
	}

	return S_OK;
}

// ISpecifyPropertyPages
STDMETHODIMP BassSource::GetPages(CAUUID* pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = reinterpret_cast<GUID*>(CoTaskMemAlloc(sizeof(GUID) * pPages->cElems));
	if (pPages->pElems == nullptr) {
		return E_OUTOFMEMORY;
	}

	pPages->pElems[0] = __uuidof(CBassMainPPage);

	return S_OK;
}

// IBassSource

STDMETHODIMP_(bool) BassSource::GetActive()
{
	return (GetPinCount() > 0);
}

STDMETHODIMP BassSource::GetInfo(std::wstring& str)
{
	if (GetActive() && m_pin && m_pin->m_decoder) {
		union {
			struct {
				BYTE d;
				BYTE c;
				BYTE b;
				BYTE a;
			};
			DWORD value;
		} version;
		version.value = BASS_GetVersion();
		str = std::format(L"BASS {}.{}.{}.{}", version.a, version.b, version.c, version.d);

		auto& d = m_pin->m_decoder;
		str += std::format(L"\nStream: '{}', {} Hz, {} ch, {}{}",
			GetBassTypeStr(d->GetBassCType()),
			d->GetSampleRate(),
			d->GetChannels(),
			d->GetFloat() ? L"Float" : L"Int",
			d->GetBytesPerSample() * 8);
		return S_OK;
	}
	else {
		str.assign(L"filter is not active");
		return S_FALSE;
	}
}
