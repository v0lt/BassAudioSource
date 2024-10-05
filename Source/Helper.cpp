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
	auto IsLatin = [](wchar_t ch) { return (ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z'); };

	if (str.size() >= 4) {
		// local file path 'x:\'
		if (str[1] == ':' && str[2] == '\\' && IsLatin(str[0])) {
			return true;
		}

		if (str.size() >= 7 && str[0] == '\\' && str[1] == '\\') {
			// net file path '\\servername'
			if (IsLatin(str[2]) || str[2] == '-' || str[2] >= '0' && str[2] <= '9') {
				return true;
			}
			// local file path with prefix '\\?\x:\'
			if (str[2] == '?' && str[3] == '\\' && str[5] == ':' && str[6] == '\\' && IsLatin(str[4])) {
				return true;
			}
		}
	}

	return false;
}

LPCWSTR GetNameAndVersion()
{
	return L"Bass Audio Source " _CRT_WIDE(VERSION_STR)
#if VER_RELEASE != 1
		L" (git-" _CRT_WIDE(_CRT_STRINGIZE(REV_DATE)) "-" _CRT_WIDE(_CRT_STRINGIZE(REV_HASH)) ")"
#endif
#ifdef _WIN64
		L" x64"
#endif
#ifdef _DEBUG
		L" DEBUG"
#endif
		;
}
