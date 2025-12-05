#ifndef BASSAAC_H
#define BASSAAC_H

#include "bass.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BASSAACDEF
#define BASSAACDEF(f) WINAPI f
#else
#define NOBASSAACOVERLOADS
#endif

// additional BASS_SetConfig options
#define BASS_CONFIG_MP4_VIDEO	0x10700 // play the audio from MP4 videos
#define BASS_CONFIG_AAC_MP4		0x10701 // support MP4
#define BASS_CONFIG_AAC_PRESCAN	0x10702 // pre-scan ADTS AAC files for seek points and accurate length

// additional BASS_AAC_StreamCreateFile/etc flags
#define BASS_AAC_FRAME960		0x1000 // 960 samples per frame
#define BASS_AAC_STEREO			0x400000 // downmatrix to stereo

// additional BASS_ChannelGetLength/GetPosition/SetPosition mode
#define BASS_POS_TRACK			4 // track number

// BASS_CHANNELINFO type
#define BASS_CTYPE_STREAM_AAC	0x10b00 // AAC
#define BASS_CTYPE_STREAM_MP4	0x10b01 // AAC in MP4 container


HSTREAM BASSAACDEF(BASS_AAC_StreamCreateFile)(DWORD filetype, const void *file, QWORD offset, QWORD length, DWORD flags);
HSTREAM BASSAACDEF(BASS_AAC_StreamCreateURL)(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user);
HSTREAM BASSAACDEF(BASS_AAC_StreamCreateFileUser)(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);

#ifdef __cplusplus
}

#if defined(_WIN32) && !defined(NOBASSAACOVERLOADS)
static inline HSTREAM BASS_AAC_StreamCreateFile(DWORD filetype, const WCHAR *file, QWORD offset, QWORD length, DWORD flags)
{
	return BASS_AAC_StreamCreateFile(filetype, (const void*)file, offset, length, flags | BASS_UNICODE);
}

static inline HSTREAM BASS_AAC_StreamCreateURL(const WCHAR *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user)
{
	return BASS_AAC_StreamCreateURL((const char*)url, offset, flags | BASS_UNICODE, proc, user);
}
#endif
#endif

#endif
