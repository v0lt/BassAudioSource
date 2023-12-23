// stdafx.cpp : source file that includes just the standard includes
// BassAudioSource.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Crypt32.lib")

#ifdef _WIN64
#pragma comment(lib, "../Lib/x64/bass.lib")
#pragma comment(lib, "../Lib/x64/bassmidi.lib")
#else
#pragma comment(lib, "../Lib/x86/bass.lib")
#pragma comment(lib, "../Lib/x86/bassmidi.lib")
#endif
