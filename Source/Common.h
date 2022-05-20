// Common.h

#pragma once

LPCSTR ToLPSTR(LPCWSTR text, LPSTR convertBuffer, int len);
LPCWSTR FromLPSTR(LPCSTR text, LPWSTR buf, int len);
#define CopyFromLPSTR(text, buf, len) (LPWSTR)FromLPSTR(text, buf, len)
#define CopyFromLPWSTR(text, buf, len) wcsncpy(buf, text, len)

LPWSTR GetFileName(LPCWSTR path, LPWSTR filename);
LPWSTR GetFilePath(LPCWSTR path, LPWSTR folder);
LPWSTR GetFileExt(LPCWSTR path, LPWSTR ext);
bool CommandParamExist(LPCWSTR command, LPCWSTR opt);
void TraceException(LPCWSTR method);
