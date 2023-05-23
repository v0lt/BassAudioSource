/*
 *  Copyright (C) 2023 v0lt
 */

#include "stdafx.h"
#include "../Include/Version.h"
#include "Helper.h"

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
