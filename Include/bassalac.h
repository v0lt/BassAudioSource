/*
	BASSALAC 2.4 C/C++ header file
	Copyright (c) 2016-2024 Un4seen Developments Ltd.

	See the BASSALAC.CHM file for more detailed documentation
*/

#ifndef BASSALAC_H
#define BASSALAC_H

#include "bass.h"

#if BASSVERSION!=0x204
#error conflicting BASS and BASSALAC versions
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSALACDEF
#define BASSALACDEF(f) WINAPI f
#endif

// BASS_CHANNELINFO type
#define BASS_CTYPE_STREAM_ALAC	0x10e00

HSTREAM BASSALACDEF(BASS_ALAC_StreamCreateFile)(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags);
HSTREAM BASSALACDEF(BASS_ALAC_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user);
HSTREAM BASSALACDEF(BASS_ALAC_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSOVERLOADS)
static inline HSTREAM BASS_ALAC_StreamCreateFile(BOOL mem, const WCHAR *file, QWORD offset, QWORD length, DWORD flags)
{
	return BASS_ALAC_StreamCreateFile(mem, (const void*)file, offset, length, flags | BASS_UNICODE);
}

static inline HSTREAM BASS_ALAC_StreamCreateURL(const WCHAR *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user)
{
	return BASS_ALAC_StreamCreateURL((const char*)url, offset, flags | BASS_UNICODE, proc, user);
}
#endif
#endif

#endif
