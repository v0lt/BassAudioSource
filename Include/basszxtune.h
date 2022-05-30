/*************************************************************************}
{ basszxtune.h - BASS sound library plugin                                }
{                                                                         }
{ (c) Alexey Parfenov, 2014                                               }
{                                                                         }
{ e-mail: zxed@alkatrazstudio.net                                         }
{                                                                         }
{ This program is free software; you can redistribute it and/or           }
{ modify it under the terms of the GNU General Public License             }
{ as published by the Free Software Foundation; either version 3 of       }
{ the License, or (at your option) any later version.                     }
{                                                                         }
{ This program is distributed in the hope that it will be useful,         }
{ but WITHOUT ANY WARRANTY; without even the implied warranty of          }
{ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        }
{ General Public License for more details.                                }
{                                                                         }
{ You may read GNU General Public License at:                             }
{   http://www.gnu.org/copyleft/gpl.html                                  }
{*************************************************************************/

#pragma once

#include "bass.h"
#include "consts.h"

#ifdef ANDROID
    #include <jni.h>
#else
    #define JNICALL
    #define JNIEXPORT
#endif

#ifdef __cplusplus
    #define BASSZXTUNE_API_PART_EXTERNC extern "C"
#else
    #define BASSZXTUNE_API_PART_EXTERNC
#endif
#ifdef _WIN32
    #ifdef BASSZXTUNE_LIBRARY
        #define BASSZXTUNE_API_PART_DECLSPEC __declspec(dllexport)
    #else
        #define BASSZXTUNE_API_PART_DECLSPEC __declspec(dllimport)
    #endif
#else
    #define BASSZXTUNE_API_PART_DECLSPEC JNIEXPORT
#endif
#ifdef BASSZXTUNE_LIBRARY
    #if __GNUC__ >= 4
        #define BASSZXTUNE_API_PART_VISIBILITY __attribute__ ((visibility("default")))
    #else
        #define BASSZXTUNE_API_PART_VISIBILITY
    #endif
#else
    #define BASSZXTUNE_API_PART_VISIBILITY
#endif
#ifdef _WIN32
    #define BASSZXTUNE_API_PART_STDCALL __stdcall
#else
    #define BASSZXTUNE_API_PART_STDCALL JNICALL
#endif

#define BASSZXTUNE_API(type) \
    BASSZXTUNE_API_PART_EXTERNC \
    BASSZXTUNE_API_PART_DECLSPEC \
    BASSZXTUNE_API_PART_VISIBILITY \
	type \
    BASSZXTUNE_API_PART_STDCALL

BASSZXTUNE_API(HSTREAM) BASS_ZXTUNE_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags);
BASSZXTUNE_API(HSTREAM) BASS_ZXTUNE_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user);
