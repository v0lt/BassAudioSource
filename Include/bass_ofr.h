#ifndef BASSOFR_H
#define BASSOFR_H

#include "bass.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSOFRDEF
#define BASSOFRDEF(f) WINAPI f
#endif

// BASS_CHANNELINFO type
#define BASS_CTYPE_STREAM_OFR	0x10600


HSTREAM BASSOFRDEF(BASS_OFR_StreamCreateFile)(DWORD filetype, const void *file, QWORD offset, QWORD length, DWORD flags);
HSTREAM BASSOFRDEF(BASS_OFR_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSOVERLOADS)
static inline HSTREAM BASS_OFR_StreamCreateFile(DWORD filetype, const WCHAR *file, QWORD offset, QWORD length, DWORD flags)
{
	return BASS_OFR_StreamCreateFile(filetype, (const void*)file, offset, length, flags | BASS_UNICODE);
}
#endif
#endif

#endif
