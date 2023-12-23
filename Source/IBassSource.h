/*
 *  Copyright (C) 2023 v0lt
 */

#pragma once

#define PREBUFFER_MIN_SIZE 100
#define PREBUFFER_MAX_SIZE 5000

struct Settings_t {
	int iBuffersizeMS;
	std::wstring sMidiSoundFontDefault;

	Settings_t() {
		SetDefault();
	}

	void SetDefault() {
		iBuffersizeMS = PREBUFFER_MAX_SIZE;
		sMidiSoundFontDefault.clear();
	}
};

interface __declspec(uuid("153B5D50-39C6-4251-A135-C6070EC7A3B0"))
IBassSource : public IUnknown {
	STDMETHOD_(bool, GetActive()) PURE;
	STDMETHOD(GetInfo) (std::wstring& str) PURE;
};
