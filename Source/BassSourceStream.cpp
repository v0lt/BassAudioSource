/*
 *  Copyright (C) 2022 v0lt
 *  Based on the following code:
 *  DC-Bass Source filter - http://www.dsp-worx.de/index.php?n=15
 *  DC-Bass Source Filter C++ porting - https://github.com/frafv/DCBassSource
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 */

#include "StdAfx.h"
#include "Common.h"
#include "BassDecoder.h"
#include "BassSource.h"
#include "BassSourceStream.h"
#include "BassAudioSource.h"
#include <MMReg.h>

/*
implementation

{*** TBassSource **************************************************************}
{*** IFileSourceFilter ********************************************************}
(*** ISpecifyPropertyPages ****************************************************)
(*** IBassAudioSource ************************************************************)
(*** IDispatch ****************************************************************)
(*** IAMMediaContent **********************************************************)
{*** TBassSourceStream ********************************************************}
*/

/*** TBassSourceStream ********************************************************/

BassSourceStream::BassSourceStream(LPCWSTR objectName, HRESULT &hr, CSource* filter, LPCWSTR name, 
  LPCWSTR filename, ShoutcastEvents* shoutcastEvents, int buffersizeMS, int prebufferMS)
  : decoder(NULL), lock(NULL), CSourceStream(objectName, &hr, filter, name) /*
constructor TBassSourceStream.Create(const ObjectName: string; out hr: HRESULT; Filter: TBCSource; const Name: WideString; Filename: PWideChar; AMetaDataCallback: TShoutcastMetaDataCallback; ABufferCallback: TShoutcastBufferCallback; ABuffersizeMS: Integer; APrebufferMS: Integer);
*/{//begin
  //inherited Create(ObjectName,hr,Filter,Name);

  if (FAILED(hr))
    return;

  this->decoder = new BassDecoder(shoutcastEvents, buffersizeMS, prebufferMS);
  if (!this->decoder->Load(filename))
  {
    hr = E_FAIL;
    return;
  }

  this->rateSeeking = 1.0;
  this->lock = new CCritSec();
  this->seekingCaps = AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards |
                  AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanGetStopPos | AM_SEEKING_CanGetDuration;

  this->stop = this->decoder->DurationMS * MSEC_REFTIME_FACTOR;
  // If Duration = 0 then it's most likely a Shoutcast Stream
  if (this->stop == 0)
    this->stop = MSEC_REFTIME_FACTOR * 50;
  this->duration = this->stop;
  this->start = 0;

  this->discontinuity = false;
  this->sampleTime = 0;
  this->mediaTime = 0;
}//end;

BassSourceStream::~BassSourceStream() /*
destructor TBassSourceStream.Destroy;
*/{//begin
  if (this->decoder)
    delete this->decoder;

  if (this->lock)
    delete this->lock;

  //inherited Destroy;
}//end;

STDMETHODIMP BassSourceStream::NonDelegatingQueryInterface(REFIID iid, void **ppv) /*
function TBassSourceStream.NonDelegatingQueryInterface(const IID: TGUID; out Obj): HResult;
*/{//begin
  if (IsEqualIID(iid, IID_IMediaSeeking))
  {
    if (!this->decoder->IsShoutcast && SUCCEEDED(GetInterface((LPUNKNOWN)(IMediaSeeking*)this, ppv)))
      return S_OK;
    else return E_NOINTERFACE;
  } else
  {
    return CSourceStream::NonDelegatingQueryInterface(iid, ppv);
  }
}

HRESULT BassSourceStream::DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * ppropInputRequest) /*
function TBassSourceStream.DecideBufferSize(Allocator: IMemAllocator; Properties: PAllocatorProperties): HRESULT;
*/{
  ALLOCATOR_PROPERTIES actual;//: ALLOCATOR_PROPERTIES;
  HRESULT result = S_OK; //begin
  CheckPointer(pAlloc, E_POINTER);
  CheckPointer(ppropInputRequest, E_POINTER);

  this->m_pFilter->pStateLock()->Lock();
  __try {
    ppropInputRequest->cBuffers = 1;
    // Set the Buffersize to our Buffersize
    ppropInputRequest->cbBuffer = BASS_BLOCK_SIZE * 2; // Double Size, just in case we receive more than needed

    HRESULT result = pAlloc->SetProperties(ppropInputRequest, &actual);
    if (SUCCEEDED(result))
    {

      // Is this allocator unsuitable?
      if (actual.cbBuffer < ppropInputRequest->cbBuffer)
        result = E_FAIL;
      else result = S_OK;
    }
  }
  __finally {
    this->m_pFilter->pStateLock()->Unlock();
  }
  return result;
}//end;

HRESULT BassSourceStream::FillBuffer(IMediaSample *pSamp) /*
function TBassSourceStream.FillBuffer(Samp: IMediaSample): HRESULT;
*/{
  BYTE* buffer;//: PByte;
  int received;//: integer;
  REFERENCE_TIME timeStart, timeStop;//: Int64;
  LONGLONG sampleTime;//: Int64;
  HRESULT result = S_OK; //begin
  this->lock->Lock();
  __try {
    if (this->mediaTime >= this->stop && !this->decoder->IsShoutcast)
    {
      result = S_FALSE;
    } else { //Exit;

    pSamp->GetPointer(&buffer);

    received = this->decoder->GetData(buffer, BASS_BLOCK_SIZE);

    if (received <= 0)
    {
      if (this->decoder->IsShoutcast)
      {
        received = BASS_BLOCK_SIZE;
        memset(buffer, BASS_BLOCK_SIZE, 0);
      } else
      {
        result = S_FALSE;
      }
    }}
    if (SUCCEEDED(result)) {

    if (this->decoder->MSecConv > 0)
      sampleTime = (LONGLONG(received) * 1000LL * 10000LL) / this->decoder->MSecConv;
    else sampleTime = 1024LL; // Dummy Value .. should never happen though ...

    pSamp->SetActualDataLength(received);

    timeStart = this->sampleTime;
    this->sampleTime += sampleTime;
    timeStop = this->sampleTime;
    pSamp->SetTime(&timeStart, &timeStop);

    timeStart = this->mediaTime;
    this->mediaTime += sampleTime;
    timeStop = this->mediaTime;
    pSamp->SetMediaTime(&timeStart, &timeStop);

    pSamp->SetSyncPoint(true);

    if (this->discontinuity)
    {
      pSamp->SetDiscontinuity(true);
      this->discontinuity = false;
    }}

  }
  __finally {
    this->lock->Unlock();
  }
  return result;
}//end;

HRESULT BassSourceStream::GetMediaType(CMediaType *pMediaType) /*
function TBassSourceStream.GetMediaType(MediaType: PAMMediaType): HRESULT;
*/{
  bool useExtensible;//: Boolean;
//begin
  if (!pMediaType)
  {
    return E_FAIL;
    //Exit;
  }

  this->m_pFilter->pStateLock()->Lock();
  __try {
    pMediaType->majortype            = MEDIATYPE_Audio;
    pMediaType->subtype              = MEDIASUBTYPE_PCM;
    pMediaType->formattype           = FORMAT_WaveFormatEx;
    pMediaType->lSampleSize          = this->decoder->Channels * this->decoder->BytesPerSample;
    pMediaType->bFixedSizeSamples    = true;
    pMediaType->bTemporalCompression = false;

    useExtensible = this->decoder->Channels > 2 || this->decoder->Float;

    if (useExtensible)
      pMediaType->cbFormat = sizeof(WAVEFORMATEXTENSIBLE);
    else pMediaType->cbFormat = sizeof(WAVEFORMATEX);

    pMediaType->pbFormat = (BYTE*)CoTaskMemAlloc(pMediaType->cbFormat);

    PWAVEFORMATEX wf = PWAVEFORMATEX(pMediaType->pbFormat);
    {
      wf->wFormatTag      = WAVE_FORMAT_PCM;
      wf->nChannels       = this->decoder->Channels;
      wf->nSamplesPerSec  = this->decoder->SampleRate;
      wf->wBitsPerSample  = this->decoder->BytesPerSample * 8;
      wf->nBlockAlign     = this->decoder->Channels * this->decoder->BytesPerSample;
      wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
      wf->cbSize          = 0;
    }

    if (useExtensible)
    {
      PWAVEFORMATEXTENSIBLE wfe = PWAVEFORMATEXTENSIBLE(pMediaType->pbFormat);
      {
        wfe->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfe->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
      }

      wfe->Samples.wValidBitsPerSample = this->decoder->BytesPerSample * 8;
      wfe->dwChannelMask = 0;

      if (this->decoder->Float)
        wfe->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
      else wfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }

  }
  __finally {
    this->m_pFilter->pStateLock()->Unlock();
  }
  return S_OK;
}//end;

HRESULT BassSourceStream::OnThreadStartPlay() /*
function TBassSourceStream.OnThreadStartPlay: HRESULT;
*/{//begin
  this->discontinuity = true;
  return DeliverNewSegment(this->start, this->stop, this->rateSeeking);
}//end;

HRESULT BassSourceStream::ChangeStart() /*
function TBassSourceStream.ChangeStart: HRESULT;
*/{//begin
  this->sampleTime = 0LL;
  this->mediaTime = this->start;
  UpdateFromSeek();
  return S_OK;
}//end;

HRESULT BassSourceStream::ChangeStop() /*
function TBassSourceStream.ChangeStop: HRESULT;
*/{//begin
  UpdateFromSeek();
  return S_OK;
}//end;

HRESULT BassSourceStream::ChangeRate() /*
function TBassSourceStream.ChangeRate: HRESULT;
*/{ HRESULT result = S_OK; //begin
  this->lock->Lock();
  __try {
    if (this->rateSeeking <= 0.0)
    {
      this->rateSeeking = 1.0;
      result = E_FAIL;
      //Exit;
    }
  }
  __finally {
    this->lock->Unlock();
  }

  if (SUCCEEDED(result)) UpdateFromSeek();

  return result;
}//end;

void BassSourceStream::UpdateFromSeek() /*
procedure TBassSourceStream.UpdateFromSeek;
*/{//begin
  if (this->ThreadExists())
  {
    DeliverBeginFlush();
    Stop();
    this->decoder->PositionMS = this->start / MSEC_REFTIME_FACTOR;
    DeliverEndFlush();
    Run();
  } else
  {
    this->decoder->PositionMS = this->start / MSEC_REFTIME_FACTOR;
  }
}//end;

/*** IMediaSeeking ************************************************************/

STDMETHODIMP BassSourceStream::GetCapabilities(DWORD *pCapabilities) /*
function TBassSourceStream.GetCapabilities(out pCapabilities: DWORD): HResult;
*/{ CheckPointer(pCapabilities, E_POINTER); //begin
  *pCapabilities = this->seekingCaps;
  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::CheckCapabilities(DWORD *pCapabilities) /*
function TBassSourceStream.CheckCapabilities(var pCapabilities: DWORD): HResult;
*/{ CheckPointer(pCapabilities, E_POINTER); //begin
  if ((~this->seekingCaps) & *pCapabilities)
    return S_FALSE;
  else return S_OK;
}//end;

STDMETHODIMP BassSourceStream::IsFormatSupported(const GUID *pFormat) /*
function TBassSourceStream.IsFormatSupported(const pFormat: TGUID): HResult;
*/{ CheckPointer(pFormat, E_POINTER); //begin
  if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat))
    return S_OK;
  else return S_FALSE;
}//end;

STDMETHODIMP BassSourceStream::QueryPreferredFormat(GUID *pFormat) /*
function TBassSourceStream.QueryPreferredFormat(out pFormat: TGUID): HResult;
*/{ CheckPointer(pFormat, E_POINTER); //begin
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::GetTimeFormat(GUID *pFormat) /*
function TBassSourceStream.GetTimeFormat(out pFormat: TGUID): HResult;
*/{ CheckPointer(pFormat, E_POINTER); //begin
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::IsUsingTimeFormat(const GUID *pFormat) /*
function TBassSourceStream.IsUsingTimeFormat(const pFormat: TGUID): HResult;
*/{ CheckPointer(pFormat, E_POINTER); //begin
  if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat))
    return S_OK;
  else return S_FALSE;
}//end;

STDMETHODIMP BassSourceStream::SetTimeFormat(const GUID *pFormat) /*
function TBassSourceStream.SetTimeFormat(const pFormat: TGUID): HResult;
*/{ CheckPointer(pFormat, E_POINTER); //begin
  if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat))
    return S_OK;
  else return E_INVALIDARG;
}//end;

STDMETHODIMP BassSourceStream::GetDuration(LONGLONG *pDuration) /*
function TBassSourceStream.GetDuration(out pDuration: int64): HResult;
*/{ CheckPointer(pDuration, E_POINTER); //begin
  this->lock->Lock();
  __try {
    *pDuration = this->duration;
  }
  __finally {
    this->lock->Unlock();
  }
  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::GetStopPosition(LONGLONG *pStop) /*
function TBassSourceStream.GetStopPosition(out pStop: int64): HResult;
*/{ CheckPointer(pStop, E_POINTER); //begin
  this->lock->Lock();
  __try {
    *pStop = this->stop;
  }
  __finally {
    this->lock->Unlock();
  }
  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::GetCurrentPosition(LONGLONG *pCurrent) /*
function TBassSourceStream.GetCurrentPosition(out pCurrent: int64): HResult;
*/{//begin
  return E_NOTIMPL;
}//end;

STDMETHODIMP BassSourceStream::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat) /*
function TBassSourceStream.ConvertTimeFormat(out pTarget: int64; pTargetFormat: PGUID; Source: int64; pSourceFormat: PGUID): HResult;
*/{ CheckPointer(pTarget, E_POINTER); //begin
  if ((!pTargetFormat || IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME)) &&
      (!pSourceFormat || IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME)))
    {
    *pTarget = Source;
    return S_OK;
  } else
  {
    return E_INVALIDARG;
  }
}//end;

STDMETHODIMP BassSourceStream::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags) /*
function TBassSourceStream.SetPositions(var pCurrent: int64; dwCurrentFlags: DWORD; var pStop: int64; dwStopFlags: DWORD): HResult;
*/{
  DWORD stopPosBits;
  DWORD startPosBits;//: Cardinal;
  CheckPointer(pCurrent, E_POINTER); CheckPointer(pStop, E_POINTER); //begin
  stopPosBits = dwStopFlags & AM_SEEKING_PositioningBitsMask;
  startPosBits = dwCurrentFlags & AM_SEEKING_PositioningBitsMask;

  if (dwStopFlags > 0)
  {
    if (stopPosBits != dwStopFlags)
    {
      return E_INVALIDARG;
      //Exit;
    }
  }

  if (dwCurrentFlags > 0)
  {
    if (startPosBits != AM_SEEKING_AbsolutePositioning && startPosBits != AM_SEEKING_RelativePositioning)
    {
      return E_INVALIDARG;
      //Exit;
    }
  }

  this->lock->Lock();
  __try {
    if (startPosBits == AM_SEEKING_AbsolutePositioning)
    {
      this->start = *pCurrent;
    }
    else if (startPosBits == AM_SEEKING_RelativePositioning)
    {
      this->start += *pCurrent;
    }
    if (stopPosBits == AM_SEEKING_AbsolutePositioning)
    {
      this->stop = *pStop;
    }
    else if (stopPosBits == AM_SEEKING_IncrementalPositioning)
    {
      this->stop = this->start + *pStop;
    }
    else if (stopPosBits == AM_SEEKING_RelativePositioning)
    {
      this->stop += *pStop;
    }
  }
  __finally {
    this->lock->Unlock();
  }

  HRESULT result = S_OK;

  if (SUCCEEDED(result) && stopPosBits > 0)
  {
    result = ChangeStop();
  }

  if(startPosBits > 0)
  {
    result = ChangeStart();
  }
  return result;
}//end;

STDMETHODIMP BassSourceStream::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop) /*
function TBassSourceStream.GetPositions(out pCurrent, pStop: int64): HResult;
*/{ CheckPointer(pCurrent, E_POINTER); CheckPointer(pStop, E_POINTER); //begin
  *pCurrent = this->start;
  *pStop = this->stop;

  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest) /*
function TBassSourceStream.GetAvailable(out pEarliest, pLatest: int64): HResult;
*/{ CheckPointer(pEarliest, E_POINTER); CheckPointer(pLatest, E_POINTER); //begin
  *pEarliest = 0LL;

  this->lock->Lock();
  __try {
    *pLatest = this->duration;
  }
  __finally {
    this->lock->Unlock();
  }

  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::SetRate(double dRate) /*
function TBassSourceStream.SetRate(dRate: double): HResult;
*/{//begin
  this->lock->Lock();
  __try {
    this->rateSeeking = dRate;
  }
  __finally {
    this->lock->Unlock();
  }

  return ChangeRate();
}//end;

STDMETHODIMP BassSourceStream::GetRate(double *pdRate) /*
function TBassSourceStream.GetRate(out pdRate: double): HResult;
*/{ CheckPointer(pdRate, E_POINTER); //begin
  this->lock->Lock();
  __try {
    *pdRate = this->rateSeeking;
  }
  __finally {
    this->lock->Unlock();
  }

  return S_OK;
}//end;

STDMETHODIMP BassSourceStream::GetPreroll(LONGLONG *pllPreroll) /*
function TBassSourceStream.GetPreroll(out pllPreroll: int64): HResult;
*/{ CheckPointer(pllPreroll, E_POINTER); //begin
  *pllPreroll = 0LL;
  return S_OK;
}//end;
