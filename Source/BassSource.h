/*
 *  Copyright (C) 2022-2024 v0lt
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

#pragma once

#include <qnetwork.h>
#include "BassSourceStream.h"
#include "IBassSource.h"

#define LABEL_BassAudioSource L"Bass Audio Source"

#define STR_CLSID_BassAudioSource "{A351970E-4601-4BEC-93DE-CEE7AF64C636}"

class __declspec(uuid(STR_CLSID_BassAudioSource))
	BassSource
	: public CSource
	, protected ShoutcastEvents
	, public IFileSourceFilter
	, public IAMMediaContent
	, public IDSMResourceBag
	, public ISpecifyPropertyPages
	, public IBassSource
{
protected:
	CCritSec* m_metaLock = nullptr;
	ContentTags m_Tags;
	std::unique_ptr<std::list<DSMResource>> m_pResources;

	BassSourceStream* m_pin = nullptr;
	std::wstring m_filePath;
	Settings_t m_Sets;

	void STDMETHODCALLTYPE OnMetaDataCallback(ContentTags* tags);
	void STDMETHODCALLTYPE OnResourceDataCallback(std::unique_ptr<std::list<DSMResource>>& pResources);
	void STDMETHODCALLTYPE OnShoutcastBufferCallback(const void* buffer, DWORD size);
	void LoadSettings();

	void Init();

public:
	BassSource(LPUNKNOWN lpunk, HRESULT* phr);
	BassSource(CFactoryTemplate* factory, LPUNKNOWN controller);
	~BassSource();

	STDMETHODIMP NonDelegatingQueryInterface(REFIID iid, void** ppv);

	DECLARE_IUNKNOWN
	// IFileSourceFilter
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);
	STDMETHODIMP GetCurFile(LPOLESTR* ppszFileName, AM_MEDIA_TYPE* pmt);

	//IDispatch
	STDMETHODIMP GetTypeInfoCount(UINT FAR* pctinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo) { return E_NOTIMPL; }
	STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgdispid) { return E_NOTIMPL; }
	STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr) { return E_NOTIMPL; }

	// IAMMediaContent
	STDMETHODIMP get_AuthorName(THIS_ BSTR FAR* pbstrAuthorName);
	STDMETHODIMP get_Title(THIS_ BSTR FAR* pbstrTitle);
	STDMETHODIMP get_Rating(THIS_ BSTR FAR* pbstrRating) { return E_NOTIMPL; }
	STDMETHODIMP get_Description(THIS_ BSTR FAR* pbstrDescription);
	STDMETHODIMP get_Copyright(THIS_ BSTR FAR* pbstrCopyright) { return E_NOTIMPL; }
	STDMETHODIMP get_BaseURL(THIS_ BSTR FAR* pbstrBaseURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoURL(THIS_ BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoIconURL(THIS_ BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_WatermarkURL(THIS_ BSTR FAR* pbstrWatermarkURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoURL(THIS_ BSTR FAR* pbstrMoreInfoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerImage(THIS_ BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerURL(THIS_ BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoText(THIS_ BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }

	//IDSMResourceBag
	STDMETHODIMP_(DWORD) ResGetCount();
	STDMETHODIMP ResGet(DWORD iIndex, BSTR* ppName, BSTR* ppDesc, BSTR* ppMime, BYTE** ppData, DWORD* pDataLen, DWORD_PTR* pTag);
	STDMETHODIMP ResSet(DWORD iIndex, LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag) { return E_NOTIMPL; }
	STDMETHODIMP ResAppend(LPCWSTR pName, LPCWSTR pDesc, LPCWSTR pMime, BYTE* pData, DWORD len, DWORD_PTR tag) { return E_NOTIMPL; }
	STDMETHODIMP ResRemoveAt(DWORD iIndex) { return E_NOTIMPL; }
	STDMETHODIMP ResRemoveAll(DWORD_PTR tag) { return E_NOTIMPL; }

	// ISpecifyPropertyPages
	STDMETHODIMP GetPages(CAUUID* pPages) override;

	// IBassSource
	STDMETHODIMP_(bool) GetActive() override;

	STDMETHODIMP_(void) GetSettings(Settings_t& setings) override;
	STDMETHODIMP_(void) SetSettings(const Settings_t setings) override;
	STDMETHODIMP SaveSettings() override;

	STDMETHODIMP GetInfo(std::wstring& str) override;
};


//var
extern volatile LONG InstanceCount;
