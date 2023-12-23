/*
 *  Copyright (C) 2023 v0lt
 */

#pragma once

struct Settings_t {
	std::wstring sMidiSoundFontDefault;

	Settings_t() {
		SetDefault();
	}

	void SetDefault() {
		sMidiSoundFontDefault.clear();
	}
};

interface __declspec(uuid("153B5D50-39C6-4251-A135-C6070EC7A3B0"))
IBassSource : public IUnknown {
	STDMETHOD_(bool, GetActive()) PURE;
	STDMETHOD(GetInfo) (std::wstring& str) PURE;
};
