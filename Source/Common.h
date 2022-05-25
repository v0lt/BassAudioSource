// Common.h

#pragma once

LPCSTR FromWideToANSI(LPCWSTR text, LPSTR convertBuffer, int len);
LPCWSTR FromAnsiToWide(LPCSTR text, LPWSTR buf, int len);
LPCWSTR FromUtf8ToWide(LPCSTR text, LPWSTR buf, int len);

LPWSTR GetFileName(LPCWSTR path, LPWSTR filename);
LPWSTR GetFilePath(LPCWSTR path, LPWSTR folder);
LPWSTR GetFileExt(LPCWSTR path, LPWSTR ext);
bool CommandParamExist(LPCWSTR command, LPCWSTR opt);
void TraceException(LPCWSTR method);
