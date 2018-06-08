// LMSStream.cpp : Implementation of CLMSStream

#include "stdafx.h"
#include "LMSStream.h"

// CLMSStream

STDMETHODIMP CLMSStream::get_isTx(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = (isTx ? VARIANT_TRUE : VARIANT_FALSE);
	return S_OK;
}


STDMETHODIMP CLMSStream::put_isTx(VARIANT_BOOL newVal)
{
	isTx = (newVal == VARIANT_TRUE);
	return S_OK;
}


STDMETHODIMP CLMSStream::get_channel(ULONG* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = channel;
	return S_OK;
}


STDMETHODIMP CLMSStream::put_channel(ULONG newVal)
{
	channel = newVal;
	return S_OK;
}


STDMETHODIMP CLMSStream::get_fifoSize(ULONG* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = fifoSize;
	return S_OK;
}


STDMETHODIMP CLMSStream::put_fifoSize(ULONG newVal)
{
	fifoSize = newVal;
	return S_OK;
}


STDMETHODIMP CLMSStream::get_throughputVsLatency(FLOAT* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = throughputVsLatency;
	return S_OK;
}


STDMETHODIMP CLMSStream::put_throughputVsLatency(FLOAT newVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}


STDMETHODIMP CLMSStream::get_dataFmt(LMS_FMT* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	// Defend against lms_stream_t::DataFmt changing
	switch (dataFmt)
	{
	case LMS_FMT_F32:
		*pVal = FMT_F32;
		break;
	case LMS_FMT_I16:
		*pVal = FMT_I16;
		break;
	case LMS_FMT_I12:
		*pVal = FMT_I12;
		break;
	}
	return S_OK;
}


STDMETHODIMP CLMSStream::put_dataFmt(LMS_FMT newVal)
{
	switch (newVal)
	{
	case FMT_F32:
		dataFmt = LMS_FMT_F32;
		break;
	case FMT_I16:
		dataFmt = LMS_FMT_I16;
		break;
	case FMT_I12:
		dataFmt = LMS_FMT_I12;
		break;
	}
	return S_OK;
}


STDMETHODIMP CLMSStream::Start(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;

	*pVal = VARIANT_FALSE;
	if (LMS_StartStream(this) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSStream::Stop(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;

	*pVal = VARIANT_FALSE;
	if (LMS_StartStream(this) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSStream::Recv(void *samples, ULONG * sample_count, ULONG timeout_ms, VARIANT_BOOL * pVal)
{
	return RecvWithMeta(samples, sample_count, NULL, timeout_ms, pVal);
}


STDMETHODIMP CLMSStream::RecvWithMeta(void *samples, ULONG * sample_count, LMSStreamMeta * meta, ULONG timeout_ms, VARIANT_BOOL * pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (samples == nullptr) return E_POINTER;
	if (sample_count == nullptr) return E_POINTER;
	*pVal = VARIANT_FALSE;

	lms_stream_meta_t lmsMeta, *pMeta = nullptr;
	if (meta != nullptr)
	{
		pMeta = &lmsMeta;
		lmsMeta.timestamp = meta->timestamp;
		lmsMeta.waitForTimestamp = (meta->timestamp == VARIANT_TRUE);
		lmsMeta.flushPartialPacket = (meta->flushPartialPacket == VARIANT_TRUE);
	}
	*sample_count = LMS_RecvStream(this, samples, *sample_count, pMeta, timeout_ms);
	if (*sample_count > 0)
	{
		if (pMeta)
			meta->timestamp = pMeta->timestamp;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}


STDMETHODIMP CLMSStream::GetStatus(LMSStreamStatus * status, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (status == nullptr) return E_POINTER;
	*pVal = VARIANT_FALSE;

	lms_stream_status_t lmsStatus;
	if (LMS_GetStreamStatus(this, &lmsStatus) == LMS_SUCCESS)
	{
		status->active = lmsStatus.active;
		status->fifoFilledCount = lmsStatus.fifoFilledCount;
		status->fifoSize = lmsStatus.fifoSize;
		status->underrun = lmsStatus.underrun;
		status->overrun = lmsStatus.overrun;
		status->droppedPackets = lmsStatus.droppedPackets;
		status->sampleRate = lmsStatus.sampleRate;
		status->linkRate = lmsStatus.linkRate;
		status->timestamp = lmsStatus.timestamp;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}


STDMETHODIMP CLMSStream::Send(void *samples, ULONG * sample_count, ULONG timeout_ms, VARIANT_BOOL * pVal)
{
	return SendWithMeta(samples, sample_count, NULL, timeout_ms, pVal);
}


STDMETHODIMP CLMSStream::SendWithMeta(void *samples, ULONG * sample_count, LMSStreamMeta * meta, ULONG timeout_ms, VARIANT_BOOL * pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (samples == nullptr) return E_POINTER;
	if (sample_count == nullptr) return E_POINTER;
	*pVal = VARIANT_FALSE;

	lms_stream_meta_t lmsMeta, *pMeta = nullptr;
	if (meta != nullptr)
	{
		pMeta = &lmsMeta;
		lmsMeta.timestamp = meta->timestamp;
		lmsMeta.waitForTimestamp = (meta->timestamp == VARIANT_TRUE);
		lmsMeta.flushPartialPacket = (meta->flushPartialPacket == VARIANT_TRUE);
	}
	*sample_count = LMS_SendStream(this, samples, *sample_count, pMeta, timeout_ms);
	if (*sample_count > 0)
	{
		if (pMeta)
			meta->timestamp = pMeta->timestamp;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}
