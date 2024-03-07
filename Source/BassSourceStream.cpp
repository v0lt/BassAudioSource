/*
 *  Copyright (C) 2022-2024 v0lt
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

#include "stdafx.h"
#include "BassSourceStream.h"
#include <MMReg.h>

//
// BassSourceStream
//

BassSourceStream::BassSourceStream(
	LPCWSTR objectName, HRESULT& hr, CSource* filter, LPCWSTR name,
	LPCWSTR filename, ShoutcastEvents* shoutcastEvents, Settings_t& sets
)
	: CSourceStream(objectName, &hr, filter, name)
{
	if (FAILED(hr)) {
		return;
	}

	m_decoder = new BassDecoder(shoutcastEvents, sets);
	if (!m_decoder->Load(filename)) {
		hr = E_FAIL;
		return;
	}

	m_lock = new CCritSec();
	m_seekingCaps = AM_SEEKING_CanSeekForwards | AM_SEEKING_CanSeekBackwards |
		AM_SEEKING_CanSeekAbsolute | AM_SEEKING_CanGetStopPos | AM_SEEKING_CanGetDuration;

	m_stop = m_decoder->GetDuration() * MSEC_REFTIME_FACTOR;
	// If Duration = 0 then it's most likely a Shoutcast Stream
	if (m_stop == 0) {
		m_stop = MSEC_REFTIME_FACTOR * 50;
	}
	m_duration = m_stop;
}

BassSourceStream::~BassSourceStream()
{
	if (m_decoder) {
		delete m_decoder;
	}

	if (m_lock) {
		delete m_lock;
	}
}

STDMETHODIMP BassSourceStream::NonDelegatingQueryInterface(REFIID iid, void** ppv)
{
	if (IsEqualIID(iid, IID_IMediaSeeking)) {
		if (!m_decoder->GetIsLiveStream() && SUCCEEDED(GetInterface((LPUNKNOWN)(IMediaSeeking*)this, ppv))) {
			return S_OK;
		}
		else {
			return E_NOINTERFACE;
		}
	}
	else {
		return CSourceStream::NonDelegatingQueryInterface(iid, ppv);
	}
}

HRESULT BassSourceStream::DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* ppropInputRequest)
{
	ALLOCATOR_PROPERTIES actual;
	HRESULT result = S_OK;
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(ppropInputRequest, E_POINTER);

	m_pFilter->pStateLock()->Lock();

	__try {
		ppropInputRequest->cBuffers = 1;
		// Set the Buffersize to our Buffersize
		ppropInputRequest->cbBuffer = BASS_BLOCK_SIZE * 2; // Double Size, just in case we receive more than needed

		HRESULT result = pAlloc->SetProperties(ppropInputRequest, &actual);
		if (SUCCEEDED(result)) {
			// Is this allocator unsuitable?
			if (actual.cbBuffer < ppropInputRequest->cbBuffer) {
				result = E_FAIL;
			}
			else {
				result = S_OK;
			}
		}
	}
	__finally {
		m_pFilter->pStateLock()->Unlock();
	}

	return result;
}

HRESULT BassSourceStream::FillBuffer(IMediaSample* pSamp)
{
	BYTE* buffer;
	int received = 0;
	REFERENCE_TIME timeStart, timeStop;
	LONGLONG sampleTime;
	HRESULT result = S_OK;

	m_lock->Lock();

	__try {
		if (m_mediaTime >= m_stop && !m_decoder->GetIsLiveStream()) {
			result = S_FALSE;
		}
		else {
			pSamp->GetPointer(&buffer);
			received = m_decoder->GetData(buffer, BASS_BLOCK_SIZE);

			if (received <= 0) {
				if (m_decoder->GetIsLiveStream()) {
					received = BASS_BLOCK_SIZE;
					memset(buffer, 0, BASS_BLOCK_SIZE);
				}
				else {
					result = S_FALSE;
				}
			}
		}
		if (SUCCEEDED(result)) {
			if (m_decoder->GetMSecConv() > 0) {
				sampleTime = (LONGLONG(received) * 1000LL * 10000LL) / m_decoder->GetMSecConv();
			}
			else {
				sampleTime = 1024LL; // Dummy Value .. should never happen though ...
			}

			pSamp->SetActualDataLength(received);

			timeStart = m_sampleTime;
			m_sampleTime += sampleTime;
			timeStop = m_sampleTime;
			pSamp->SetTime(&timeStart, &timeStop);

			timeStart = m_mediaTime;
			m_mediaTime += sampleTime;
			timeStop = m_mediaTime;
			pSamp->SetMediaTime(&timeStart, &timeStop);

			pSamp->SetSyncPoint(true);

			if (m_discontinuity) {
				pSamp->SetDiscontinuity(true);
				m_discontinuity = false;
			}
		}

	}
	__finally {
		m_lock->Unlock();
	}

	return result;
}

HRESULT BassSourceStream::GetMediaType(CMediaType* pMediaType)
{
	bool useExtensible;

	if (!pMediaType) {
		return E_FAIL;
	}

	m_pFilter->pStateLock()->Lock();

	__try {
		pMediaType->majortype = MEDIATYPE_Audio;
		pMediaType->subtype = MEDIASUBTYPE_PCM;
		pMediaType->formattype = FORMAT_WaveFormatEx;
		pMediaType->lSampleSize = m_decoder->GetChannels() * m_decoder->GetBytesPerSample();
		pMediaType->bFixedSizeSamples = true;
		pMediaType->bTemporalCompression = false;

		useExtensible = m_decoder->GetChannels() > 2 || m_decoder->GetFloat();

		if (useExtensible) {
			pMediaType->cbFormat = sizeof(WAVEFORMATEXTENSIBLE);
		} else {
			pMediaType->cbFormat = sizeof(WAVEFORMATEX);
		}

		pMediaType->pbFormat = (BYTE*)CoTaskMemAlloc(pMediaType->cbFormat);

		PWAVEFORMATEX wf = PWAVEFORMATEX(pMediaType->pbFormat);
		{
			wf->wFormatTag = WAVE_FORMAT_PCM;
			wf->nChannels = m_decoder->GetChannels();
			wf->nSamplesPerSec = m_decoder->GetSampleRate();
			wf->wBitsPerSample = m_decoder->GetBytesPerSample() * 8;
			wf->nBlockAlign = m_decoder->GetChannels() * m_decoder->GetBytesPerSample();
			wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
			wf->cbSize = 0;
		}

		if (useExtensible) {
			PWAVEFORMATEXTENSIBLE wfe = PWAVEFORMATEXTENSIBLE(pMediaType->pbFormat);
			{
				wfe->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
				wfe->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			}

			wfe->Samples.wValidBitsPerSample = m_decoder->GetBytesPerSample() * 8;
			wfe->dwChannelMask = 0;

			if (m_decoder->GetFloat()) {
				wfe->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
			} else {
				wfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			}
		}

	}
	__finally {
		m_pFilter->pStateLock()->Unlock();
	}

	return S_OK;
}

HRESULT BassSourceStream::OnThreadStartPlay()
{
	m_discontinuity = true;

	return DeliverNewSegment(m_start, m_stop, m_rateSeeking);
}

HRESULT BassSourceStream::ChangeStart()
{
	m_sampleTime = 0LL;
	m_mediaTime = m_start;
	UpdateFromSeek();

	return S_OK;
}

HRESULT BassSourceStream::ChangeStop()
{
	UpdateFromSeek();

	return S_OK;
}

HRESULT BassSourceStream::ChangeRate()
{
	HRESULT result = S_OK;

	m_lock->Lock();

	__try {
		if (m_rateSeeking <= 0.0) {
			m_rateSeeking = 1.0;
			result = E_FAIL;
		}
	}
	__finally {
		m_lock->Unlock();
	}

	if (SUCCEEDED(result)) {
		UpdateFromSeek();
	}

	return result;
}

void BassSourceStream::UpdateFromSeek()
{
	if (ThreadExists()) {
		DeliverBeginFlush();
		Stop();
		m_decoder->SetPosition(m_start / MSEC_REFTIME_FACTOR);
		DeliverEndFlush();
		Run();
	}
	else {
		m_decoder->SetPosition(m_start / MSEC_REFTIME_FACTOR);
	}
}

// IMediaSeeking

STDMETHODIMP BassSourceStream::GetCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);
	*pCapabilities = m_seekingCaps;

	return S_OK;
}

STDMETHODIMP BassSourceStream::CheckCapabilities(DWORD* pCapabilities)
{
	CheckPointer(pCapabilities, E_POINTER);

	if ((~m_seekingCaps) & *pCapabilities) {
		return S_FALSE;
	}
	else {
		return S_OK;
	}
}

STDMETHODIMP BassSourceStream::IsFormatSupported(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat)) {
		return S_OK;
	}
	else {
		return S_FALSE;
	}
}

STDMETHODIMP BassSourceStream::QueryPreferredFormat(GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP BassSourceStream::GetTimeFormat(GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);
	*pFormat = TIME_FORMAT_MEDIA_TIME;

	return S_OK;
}

STDMETHODIMP BassSourceStream::IsUsingTimeFormat(const GUID* pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat)) {
		return S_OK;
	}
	else {
		return S_FALSE;
	}
}

STDMETHODIMP BassSourceStream::SetTimeFormat(const GUID *pFormat)
{
	CheckPointer(pFormat, E_POINTER);

	if (IsEqualGUID(TIME_FORMAT_MEDIA_TIME, *pFormat)) {
		return S_OK;
	}
	else {
		return E_INVALIDARG;
	}
}

STDMETHODIMP BassSourceStream::GetDuration(LONGLONG* pDuration)
{
	CheckPointer(pDuration, E_POINTER);

	m_lock->Lock();

	__try {
		*pDuration = m_duration;
	}
	__finally {
		m_lock->Unlock();
	}

	return S_OK;
}

STDMETHODIMP BassSourceStream::GetStopPosition(LONGLONG* pStop)
{
	CheckPointer(pStop, E_POINTER);

	m_lock->Lock();

	__try {
		*pStop = m_stop;
	}
	__finally {
		m_lock->Unlock();
	}

	return S_OK;
}

STDMETHODIMP BassSourceStream::GetCurrentPosition(LONGLONG* pCurrent)
{
	return E_NOTIMPL;
}

STDMETHODIMP BassSourceStream::ConvertTimeFormat(LONGLONG* pTarget, const GUID* pTargetFormat, LONGLONG Source, const GUID* pSourceFormat)
{
	CheckPointer(pTarget, E_POINTER);
	if ((!pTargetFormat || IsEqualGUID(*pTargetFormat, TIME_FORMAT_MEDIA_TIME)) &&
			(!pSourceFormat || IsEqualGUID(*pSourceFormat, TIME_FORMAT_MEDIA_TIME))) {
		*pTarget = Source;

		return S_OK;
	}
	else {
		return E_INVALIDARG;
	}
}

STDMETHODIMP BassSourceStream::SetPositions(LONGLONG* pCurrent, DWORD dwCurrentFlags, LONGLONG* pStop, DWORD dwStopFlags)
{
	DWORD stopPosBits;
	DWORD startPosBits;
	CheckPointer(pCurrent, E_POINTER); CheckPointer(pStop, E_POINTER);
	stopPosBits = dwStopFlags & AM_SEEKING_PositioningBitsMask;
	startPosBits = dwCurrentFlags & AM_SEEKING_PositioningBitsMask;

	if (dwStopFlags > 0) {
		if (stopPosBits != dwStopFlags) {
			return E_INVALIDARG;
		}
	}

	if (dwCurrentFlags > 0) {
		if (startPosBits != AM_SEEKING_AbsolutePositioning && startPosBits != AM_SEEKING_RelativePositioning) {
			return E_INVALIDARG;
		}
	}

	m_lock->Lock();

	__try {
		if (startPosBits == AM_SEEKING_AbsolutePositioning) {
			m_start = *pCurrent;
		}
		else if (startPosBits == AM_SEEKING_RelativePositioning) {
			m_start += *pCurrent;
		}

		if (stopPosBits == AM_SEEKING_AbsolutePositioning) {
			m_stop = *pStop;
		}
		else if (stopPosBits == AM_SEEKING_IncrementalPositioning) {
			m_stop = m_start + *pStop;
		}
		else if (stopPosBits == AM_SEEKING_RelativePositioning) {
			m_stop += *pStop;
		}
	}
	__finally {
		m_lock->Unlock();
	}

	HRESULT result = S_OK;

	if (SUCCEEDED(result) && stopPosBits > 0) {
		result = ChangeStop();
	}

	if (startPosBits > 0) {
		result = ChangeStart();
	}

	return result;
}

STDMETHODIMP BassSourceStream::GetPositions(LONGLONG* pCurrent, LONGLONG* pStop)
{
	CheckPointer(pCurrent, E_POINTER);
	CheckPointer(pStop, E_POINTER);

	*pCurrent = m_start;
	*pStop = m_stop;

	return S_OK;
}

STDMETHODIMP BassSourceStream::GetAvailable(LONGLONG* pEarliest, LONGLONG* pLatest)
{
	CheckPointer(pEarliest, E_POINTER);
	CheckPointer(pLatest, E_POINTER);

	*pEarliest = 0LL;

	m_lock->Lock();

	__try {
		*pLatest = m_duration;
	}
	__finally {
		m_lock->Unlock();
	}

	return S_OK;
}

STDMETHODIMP BassSourceStream::SetRate(double dRate)
{
	m_lock->Lock();

	__try {
		m_rateSeeking = dRate;
	}
	__finally {
		m_lock->Unlock();
	}

	return ChangeRate();
}

STDMETHODIMP BassSourceStream::GetRate(double* pdRate)
{
	CheckPointer(pdRate, E_POINTER);

	m_lock->Lock();

	__try {
		*pdRate = m_rateSeeking;
	}
	__finally {
		m_lock->Unlock();
	}

	return S_OK;
}

STDMETHODIMP BassSourceStream::GetPreroll(LONGLONG* pllPreroll)
{
	CheckPointer(pllPreroll, E_POINTER);

	*pllPreroll = 0LL;

	return S_OK;
}
