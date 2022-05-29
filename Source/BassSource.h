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

//BassSource.h

#include <InitGuid.h>
#include <qnetwork.h>
#include "BassSourceStream.h"

#pragma once

#define PREBUFFER_MIN_SIZE 100
#define PREBUFFER_MAX_SIZE 5000

#define LABEL_BassAudioSource L"Bass Audio Source"

#define STR_CLSID_BassAudioSource "{A351970E-4601-4BEC-93DE-CEE7AF64C636}"

class __declspec(uuid(STR_CLSID_BassAudioSource))
	BassSource
	: public CSource
	, protected ShoutcastEvents
	, public IFileSourceFilter
	, public IAMMediaContent
{
protected:
	CCritSec* m_metaLock = nullptr;
	BassSourceStream* m_pin = nullptr;
	std::wstring m_fileName;
	std::wstring m_currentTag;
	int m_buffersizeMS = 0;
	int m_preBufferMS = 0;

	void STDMETHODCALLTYPE OnShoutcastMetaDataCallback(LPCWSTR text);
	void STDMETHODCALLTYPE OnShoutcastBufferCallback(const void* buffer, DWORD size);
	void LoadSettings();
	void SaveSettings();

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
	STDMETHODIMP get_AuthorName(THIS_ BSTR FAR* pbstrAuthorName) { return E_NOTIMPL; }
	STDMETHODIMP get_Title(THIS_ BSTR FAR* pbstrTitle);
	STDMETHODIMP get_Rating(THIS_ BSTR FAR* pbstrRating) { return E_NOTIMPL; }
	STDMETHODIMP get_Description(THIS_ BSTR FAR* pbstrDescription) { return E_NOTIMPL; }
	STDMETHODIMP get_Copyright(THIS_ BSTR FAR* pbstrCopyright) { return E_NOTIMPL; }
	STDMETHODIMP get_BaseURL(THIS_ BSTR FAR* pbstrBaseURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoURL(THIS_ BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_LogoIconURL(THIS_ BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_WatermarkURL(THIS_ BSTR FAR* pbstrWatermarkURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoURL(THIS_ BSTR FAR* pbstrMoreInfoURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerImage(THIS_ BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoBannerURL(THIS_ BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
	STDMETHODIMP get_MoreInfoText(THIS_ BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }
};


//var
extern volatile LONG InstanceCount;
