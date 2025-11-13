#ifndef BASSSPX_H
#define BASSSPX_H

#include "bass.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSSPXDEF
#define BASSSPXDEF(f) WINAPI f
#endif

// BASS_CHANNELINFO type
#define BASS_CTYPE_STREAM_SPX	0x10c00


HSTREAM BASSSPXDEF(BASS_SPX_StreamCreateFile)(DWORD filetype, const void *file, QWORD offset, QWORD length, DWORD flags);
HSTREAM BASSSPXDEF(BASS_SPX_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user);
HSTREAM BASSSPXDEF(BASS_SPX_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSOVERLOADS)
static inline HSTREAM BASS_SPX_StreamCreateFile(DWORD filetype, const WCHAR *file, QWORD offset, QWORD length, DWORD flags)
{
	return BASS_SPX_StreamCreateFile(filetype, (const void*)file, offset, length, flags | BASS_UNICODE);
}

static inline HSTREAM BASS_SPX_StreamCreateURL(const WCHAR *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user)
{
	return BASS_SPX_StreamCreateURL((const char*)url, offset, flags | BASS_UNICODE, proc, user);
}
#endif
#endif

#endif
