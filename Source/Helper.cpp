/*
 *  Copyright (C) 2023-2024 v0lt
 */

#include "stdafx.h"
#include "../Include/Version.h"
#include "Helper.h"
#include "dllmain.h"

std::wstring GetFilterDirectory()
{
	std::wstring path(MAX_PATH + 1, '\0');

	DWORD res = GetModuleFileNameW(HInstance, path.data(), (DWORD)(path.size() - 1));
	if (res) {
		path.resize(path.rfind('\\', res) + 1);
	}
	else {
		path.clear();
	}

	return path;
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

const std::wstring GetVersionStr()
{
	std::wstring version = _CRT_WIDE(VERSION_STR);
#if VER_RELEASE != 1
	version += std::format(L" (git-{}-{})",
		_CRT_WIDE(_CRT_STRINGIZE(REV_DATE)),
		_CRT_WIDE(_CRT_STRINGIZE(REV_HASH))
	);
#endif
#ifdef _WIN64
	version.append(L" x64");
#endif
#ifdef _DEBUG
	version.append(L" DEBUG");
#endif
	return version;
}

LPCWSTR GetNameAndVersion()
{
	static std::wstring version = L"Bass Audio Source " + GetVersionStr();

	return version.c_str();
}
