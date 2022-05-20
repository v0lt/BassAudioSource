// Common.h

#pragma once

#define finally(Saver) \
catch ( ... ) { \
  Saver \
  throw; \
} \
Saver

LPCSTR ToLPSTR(LPCWSTR text, LPSTR convertBuffer, int len);
#define ToLPWSTR(text, convertBuffer, len) (text)
LPCWSTR FromLPSTR(LPCSTR text, LPWSTR buf, int len);
#define FromLPWSTR(text, buf, len) (text)
#define CopyFromLPSTR(text, buf, len) (LPWSTR)FromLPSTR(text, buf, len)
#define CopyFromLPWSTR(text, buf, len) wcsncpy(buf, text, len)

LPWSTR GetFileName(LPCWSTR path, LPWSTR filename);
LPWSTR GetFilePath(LPCWSTR path, LPWSTR folder);
LPWSTR GetFileExt(LPCWSTR path, LPWSTR ext);
bool CommandParamExist(LPCWSTR command, LPCWSTR opt);
void TraceException(LPCWSTR method);
