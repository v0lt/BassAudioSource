/*
 *  Copyright (C) 2023-2025 v0lt
 */

#pragma once

struct Settings_t {
	bool bMidiEnable;
	bool bWebmEnable;
	std::wstring sMidiSoundFontDefault;

	Settings_t() {
		SetDefault();
	}

	void SetDefault() {
		bMidiEnable = false;
		bWebmEnable = false;
		sMidiSoundFontDefault.clear();
	}
};

interface __declspec(uuid("153B5D50-39C6-4251-A135-C6070EC7A3B0"))
IBassSource : public IUnknown {
	STDMETHOD_(bool, GetActive()) PURE;

	STDMETHOD_(void, GetSettings(Settings_t& setings)) PURE;
	STDMETHOD_(void, SetSettings(const Settings_t setings)) PURE; // use copy of setings here
	STDMETHOD(SaveSettings()) PURE;

	STDMETHOD(GetInfo) (std::wstring& str) PURE;
};
