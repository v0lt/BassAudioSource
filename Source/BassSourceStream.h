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

//BassSourceStream.h

#include "BassDecoder.h"

#pragma once

#define BASS_BLOCK_SIZE               2048
#define MSEC_REFTIME_FACTOR           10000


class BassSourceStream : public CSourceStream, public IMediaSeeking
{
	friend class BassSource;
private:
	BassDecoder* decoder;
	double rateSeeking;
	DWORD seekingCaps;
	LONGLONG duration;
	REFERENCE_TIME start;
	REFERENCE_TIME stop;
	bool discontinuity;
	REFERENCE_TIME sampleTime;
	REFERENCE_TIME mediaTime;
	CCritSec* lock;
	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();
	void UpdateFromSeek();

public:
	BassSourceStream(LPCWSTR objectName, HRESULT& hr, CSource* filter, LPCWSTR name, LPCWSTR filename,
		ShoutcastEvents* shoutcastEvents,
		int buffersizeMS, int prebufferMS);
	~BassSourceStream();

	HRESULT GetMediaType(CMediaType* pMediaType);
	HRESULT FillBuffer(IMediaSample* pSamp);
	HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* ppropInputRequest);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);
	HRESULT OnThreadStartPlay();

	DECLARE_IUNKNOWN
	// IMediaSeeking methods
	STDMETHODIMP GetCapabilities(DWORD* pCapabilities);
	STDMETHODIMP CheckCapabilities(DWORD* pCapabilities);
	STDMETHODIMP IsFormatSupported(const GUID* pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID* pFormat);
	STDMETHODIMP GetTimeFormat(GUID* pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID* pFormat);
	STDMETHODIMP SetTimeFormat(const GUID* pFormat);
	STDMETHODIMP GetDuration(LONGLONG* pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG* pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG* pCurrent);
	STDMETHODIMP ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat);
	STDMETHODIMP SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags);
	STDMETHODIMP GetPositions(LONGLONG* pCurrent, LONGLONG* pStop);
	STDMETHODIMP GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double* pdRate);
	STDMETHODIMP GetPreroll(LONGLONG* pllPreroll);
};
