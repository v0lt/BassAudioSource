#include "stdafx.h"
#include "Common.h"
#include <wchar.h>

#pragma unmanaged

LPCSTR ToLPSTR(LPCWSTR text, LPSTR convertBuffer, int len) {
	WideCharToMultiByte(CP_ACP, 0, text, -1, convertBuffer, len, NULL, NULL);
	return convertBuffer;
}

LPCWSTR FromLPSTR(LPCSTR text, LPWSTR buf, int len) {
	MultiByteToWideChar(CP_ACP, 0, text, -1, buf, len);
	return buf;
}

//

LPWSTR GetFileName(LPCWSTR path, LPWSTR filename) {
	WCHAR ext[MAX_PATH];
	_wsplitpath(path, NULL, NULL, filename, ext);
	wcscat(filename, ext);
	return filename;
}

LPWSTR GetFilePath(LPCWSTR path, LPWSTR folder) {
	WCHAR dir[MAX_PATH];
	_wsplitpath(path, folder, dir, NULL, NULL);
	wcscat(folder, dir);
	return folder;
}

LPWSTR GetFileExt(LPCWSTR path, LPWSTR ext) {
	_wsplitpath(path, NULL, NULL, NULL, ext);
	return ext;
}

bool CommandParamExist(LPCWSTR command, LPCWSTR opt) {
	size_t len = wcslen(opt);
	LPCWSTR pos = wcsstr(command, opt);
	if (!pos) return false;
	pos -= 1;
	if (*pos != ' ' && *pos != '"') return false;
	pos += len+1;
	if (*pos != ' ' && *pos != '"' && *pos) return false;
	return true;
}

void TraceException(LPCWSTR method) {
	WCHAR textBuffer[1024];
	int textBufferLength = 1024;

	_snwprintf(textBuffer, textBufferLength, L"Exception in %s\r\n", method);
	OutputDebugStringW(textBuffer);
}