/*
	BASSWEBM 2.4 C/C++ header file
	Copyright (c) 2018-2025 Un4seen Developments Ltd.

	See the BASSWEBM.CHM file for more detailed documentation
*/

#ifndef BASSWEBM_H
#define BASSWEBM_H

#include "bass.h"

#if BASSVERSION!=0x204
#error conflicting BASS and BASSWEBM versions
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSWEBMDEF
#define BASSWEBMDEF(f) WINAPI f
#else
#define NOBASSWEBMOVERLOADS
#endif

// Additional error codes returned by BASS_ErrorGetCode
#define BASS_ERROR_TRACK	13	// invalid track number

// Additional tag types
#define BASS_TAG_WEBM			0x15000 // file tags : series of null-terminated UTF-8 strings
#define BASS_TAG_WEBM_TRACK		0x15001 // track tags : series of null-terminated UTF-8 strings

// Additional attributes
#define BASS_ATTRIB_WEBM_TRACK	0x16000
#define BASS_ATTRIB_WEBM_TRACKS	0x16001
#define BASS_ATTRIB_WEBM		0x16002

// Additional BASS_ChannelGetLength/GetPosition/SetPosition mode
#define BASS_POS_TRACK			4 // track number

HSTREAM BASSWEBMDEF(BASS_WEBM_StreamCreateFile)(DWORD filetype, const void *file, QWORD offset, QWORD length, DWORD flags, DWORD track);
HSTREAM BASSWEBMDEF(BASS_WEBM_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user, DWORD track);
HSTREAM BASSWEBMDEF(BASS_WEBM_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user, DWORD track);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSWEBMOVERLOADS)
static inline HSTREAM BASS_WEBM_StreamCreateFile(DWORD filetype, const WCHAR *file, QWORD offset, QWORD length, DWORD flags, DWORD track)
{
	return BASS_WEBM_StreamCreateFile(filetype, (const void*)file, offset, length, flags|BASS_UNICODE, track);
}

static inline HSTREAM BASS_WEBM_StreamCreateURL(const WCHAR *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user, DWORD track)
{
	return BASS_WEBM_StreamCreateURL((const char*)url, offset, flags|BASS_UNICODE, proc, user, track);
}
#endif
#endif

#endif
