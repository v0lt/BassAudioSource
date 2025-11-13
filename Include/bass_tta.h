#ifndef BASSTTA_H
#define BASSTTA_H

#include "bass.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSTTADEF
#define BASSTTADEF(f) WINAPI f
#endif

// BASS_CHANNELINFO type
#define BASS_CTYPE_STREAM_TTA	0x10f00


HSTREAM BASSTTADEF(BASS_TTA_StreamCreateFile)(DWORD filetype, const void *file, QWORD offset, QWORD length, DWORD flags);
HSTREAM BASSTTADEF(BASS_TTA_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSOVERLOADS)
static inline HSTREAM BASS_TTA_StreamCreateFile(DWORD filetype, const WCHAR *file, QWORD offset, QWORD length, DWORD flags)
{
	return BASS_TTA_StreamCreateFile(filetype, (const void*)file, offset, length, flags | BASS_UNICODE);
}
#endif
#endif

#endif
