// Common.h

#pragma once

LPCSTR FromWideToANSI(LPCWSTR text, LPSTR convertBuffer, int len);
LPCWSTR FromAnsiToWide(LPCSTR text, LPWSTR buf, int len);
LPCWSTR FromUtf8ToWide(LPCSTR text, LPWSTR buf, int len);

// simple file system path detector
bool IsLikelyFilePath(const std::wstring_view str);

bool CommandParamExist(LPCWSTR command, LPCWSTR opt);
void TraceException(LPCWSTR method);
