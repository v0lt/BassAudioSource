#include "stdafx.h"
#include "Common.h"
#include <wchar.h>

#pragma unmanaged

LPCSTR FromWideToANSI(LPCWSTR text, LPSTR convertBuffer, int len)
{
	WideCharToMultiByte(CP_ACP, 0, text, -1, convertBuffer, len, nullptr, nullptr);
	return convertBuffer;
}

LPCWSTR FromAnsiToWide(LPCSTR text, LPWSTR buf, int len)
{
	MultiByteToWideChar(CP_ACP, 0, text, -1, buf, len);
	return buf;
}

LPCWSTR FromUtf8ToWide(LPCSTR text, LPWSTR buf, int len)
{
	MultiByteToWideChar(CP_UTF8, 0, text, -1, buf, len);
	return buf;
}

//

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

bool CommandParamExist(LPCWSTR command, LPCWSTR opt) {
	size_t len = wcslen(opt);
	LPCWSTR pos = wcsstr(command, opt);
	if (!pos) {
		return false;
	}
	pos -= 1;
	if (*pos != ' ' && *pos != '"') {
		return false;
	}
	pos += len+1;
	if (*pos != ' ' && *pos != '"' && *pos) {
		return false;
	}
	return true;
}

void TraceException(LPCWSTR method) {
	WCHAR textBuffer[1024];

	_snwprintf(textBuffer, std::size(textBuffer), L"Exception in %s\r\n", method);
	OutputDebugStringW(textBuffer);
}
