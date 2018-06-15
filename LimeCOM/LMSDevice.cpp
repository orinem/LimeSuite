// LMSDevice.cpp : Implementation of CLMSDevice

#include "stdafx.h"
#include "LMSDevice.h"
#include "LMSChannel.h"
#include <vector>


// CLMSDevice

STDMETHODIMP CLMSDevice::Close(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = VARIANT_FALSE;

	HRESULT hr = S_OK;

	Lock();	// Protect m_device

	if (m_device == nullptr)
	{
		hr = OLE_E_BLANK;
	}
	else if (LMS_Close(m_device) == LMS_SUCCESS)
	{
		*pVal = VARIANT_TRUE;
		m_device = nullptr;
	}

	Unlock();

	return hr;
}


STDMETHODIMP CLMSDevice::Reset(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_Reset(m_device) == LMS_SUCCESS) *pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::Init(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_Init(m_device) == LMS_SUCCESS) *pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::get_NumChannels(VARIANT_BOOL dir_tx, LONG* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = LMS_GetNumChannels(m_device, dir_tx == VARIANT_TRUE);

	return S_OK;
}


STDMETHODIMP CLMSDevice::EnableChannel(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enabled, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_EnableChannel(m_device, (dir_tx == VARIANT_TRUE), chan, (enabled == VARIANT_TRUE)) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetSampleRate(DOUBLE rate, ULONG oversample, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_SetSampleRate(m_device, rate, oversample) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetSampleRate(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE* host_Hz, DOUBLE* rf_Hz, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	// host_Hz and rf_Hz are allowed to be NULL by LMS_GetSampleRate() so no need to check

	*pVal = VARIANT_FALSE;
	if (LMS_GetSampleRate(m_device, (dir_tx == VARIANT_TRUE), chan, host_Hz, rf_Hz) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetLOFrequency(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE frequency, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_SetLOFrequency(m_device, (dir_tx == VARIANT_TRUE), chan, frequency) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetLOFrequency(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE* frequency, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (frequency == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_GetLOFrequency(m_device, (dir_tx == VARIANT_TRUE), chan, frequency) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetAntennaList(VARIANT_BOOL dir_tx, ULONG chan, SAFEARRAY ** list)
{
	if (list == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*list = nullptr;

	HRESULT hr = S_OK;
	int nDevices = LMS_GetAntennaList(m_device, (dir_tx == VARIANT_TRUE), chan, NULL);
	if (nDevices < 0) nDevices = 0;
	try {
		std::vector<lms_name_t> antennaStrings(nDevices);
		if (nDevices > 0)
		{
			if (LMS_GetAntennaList(m_device, (dir_tx == VARIANT_TRUE), chan, antennaStrings.data()) != (int)antennaStrings.size())
			{
				return E_UNEXPECTED;	// We are so doomed!
			}
		}
		*list = SafeArrayCreateVector(VT_BSTR, 0, (ULONG)antennaStrings.size());
		if (*list == nullptr)
			hr = E_OUTOFMEMORY;
		else if (antennaStrings.size() > 0)
		{
			BSTR * pArrayData;
			hr = SafeArrayAccessData(*list, (void **)&pArrayData);
			if (SUCCEEDED(hr))
			{
				for (size_t i = 0; i < antennaStrings.size(); ++i)
				{
					int nUTF8 = (int)strlen(antennaStrings[i]);
					int nWide = MultiByteToWideChar(CP_UTF8, 0, antennaStrings[i], nUTF8, nullptr, 0);
					if (nWide > 0)
					{
						pArrayData[i] = SysAllocStringLen(nullptr, nWide);
						if (pArrayData[i] == nullptr)
							hr = E_OUTOFMEMORY;
						else
							MultiByteToWideChar(CP_UTF8, 0, antennaStrings[i], nUTF8, pArrayData[i], nWide);
					}
				}
				SafeArrayUnaccessData(*list);
			}
		}
	}
	catch (std::exception &)
	{
		hr = E_OUTOFMEMORY;
		if (list != nullptr)
		{
			SafeArrayDestroy(*list);
			*list = nullptr;
		}
	}

	return hr;
}


STDMETHODIMP CLMSDevice::SetAntenna(VARIANT_BOOL dir_tx, ULONG chan, LMS_PATH index, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_SetAntenna(m_device, (dir_tx == VARIANT_TRUE), chan, index) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetAntenna(VARIANT_BOOL dir_tx, ULONG chan, LMS_PATH * index, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (index == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	int i = LMS_GetAntenna(m_device, (dir_tx == VARIANT_TRUE), chan);
	if (i >= 0)
	{
		*index = (LMS_PATH)i;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetLPFBWRange(VARIANT_BOOL dir_tx, LMSRange * range, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (range == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	lms_range_t lmsRange;
	if (LMS_GetLPFBWRange(m_device, (dir_tx == VARIANT_TRUE), &lmsRange) == LMS_SUCCESS)
	{
		range->min = lmsRange.min;
		range->max = lmsRange.max;
		range->step = lmsRange.step;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetLPF(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enable, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetLPF(m_device, (dir_tx == VARIANT_TRUE), chan, (enable == VARIANT_TRUE)) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetGFIRLPF(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enable, DOUBLE bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetGFIRLPF(m_device, (dir_tx == VARIANT_TRUE), chan, (enable == VARIANT_TRUE), bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::Calibrate(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE bandwidth, ULONG flags, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_Calibrate(m_device, (dir_tx == VARIANT_TRUE), chan, bandwidth, flags) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetLPFBW(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetLPFBW(m_device, (dir_tx == VARIANT_TRUE), chan, bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetLPFBW(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE * bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (bandwidth == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_GetLPFBW(m_device, (dir_tx == VARIANT_TRUE), chan, bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetNormalizedGain(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetNormalizedGain(m_device, (dir_tx == VARIANT_TRUE), chan, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetNormalizedGain(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE * gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (gain == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_GetNormalizedGain(m_device, (dir_tx == VARIANT_TRUE), chan, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::SetGaindB(VARIANT_BOOL dir_tx, ULONG chan, ULONG gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetGaindB(m_device, (dir_tx == VARIANT_TRUE), chan, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetGaindB(VARIANT_BOOL dir_tx, ULONG chan, ULONG * gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (gain == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	unsigned uGain;
	if (LMS_GetGaindB(m_device, (dir_tx == VARIANT_TRUE), chan, &uGain) == LMS_SUCCESS)
	{
		*gain = uGain;
		*pVal = VARIANT_TRUE;
	}
	return S_OK;
}


STDMETHODIMP CLMSDevice::SetTestSignal(VARIANT_BOOL dir_tx, ULONG chan, LMS_TESTSIG sig, USHORT dc_i, USHORT dc_q, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	lms_testsig_t lmsSig;
	switch (sig)
	{
	case LMS_TESTSIG::TESTSIG_NONE:
		lmsSig = LMS_TESTSIG_NONE;
		break;
	case LMS_TESTSIG::TESTSIG_NCODIV8:
		lmsSig = LMS_TESTSIG_NCODIV8;
		break;
	case LMS_TESTSIG::TESTSIG_NCODIV4:
		lmsSig = LMS_TESTSIG_NCODIV4;
		break;
	case LMS_TESTSIG::TESTSIG_NCODIV8F:
		lmsSig = LMS_TESTSIG_NCODIV8F;
		break;
	case LMS_TESTSIG::TESTSIG_NCODIV4F:
		lmsSig = LMS_TESTSIG_NCODIV4F;
		break;
	case LMS_TESTSIG::TESTSIG_DC:
		lmsSig = LMS_TESTSIG_DC;
		break;
	}
	if (LMS_SetTestSignal(m_device, (dir_tx == VARIANT_TRUE), chan, lmsSig, dc_i, dc_q) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSDevice::GetTestSignal(VARIANT_BOOL dir_tx, ULONG chan, LMS_TESTSIG * sig, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (sig == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	lms_testsig_t lmsSig;
	if (LMS_GetTestSignal(m_device, (dir_tx == VARIANT_TRUE), chan, &lmsSig) == LMS_SUCCESS)
	{
		switch (lmsSig)
		{
		case LMS_TESTSIG_NONE:
			*sig = LMS_TESTSIG::TESTSIG_NONE;
			break;
		case LMS_TESTSIG_NCODIV8:
			*sig = LMS_TESTSIG::TESTSIG_NCODIV8;
			break;
		case LMS_TESTSIG_NCODIV4:
			*sig = LMS_TESTSIG::TESTSIG_NCODIV4;
			break;
		case LMS_TESTSIG_NCODIV8F:
			*sig = LMS_TESTSIG::TESTSIG_NCODIV8F;
			break;
		case LMS_TESTSIG_NCODIV4F:
			*sig = LMS_TESTSIG::TESTSIG_NCODIV4F;
			break;
		case LMS_TESTSIG_DC:
			*sig = LMS_TESTSIG::TESTSIG_DC;
			break;
		}
		*pVal = VARIANT_TRUE;
	}
	return S_OK;
}


STDMETHODIMP CLMSDevice::LoadConfig(BSTR filename, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	int nWide = SysStringLen(filename); // Excludes terminating null
	if (nWide > 0)
	{
		int nMulti = WideCharToMultiByte(CP_UTF8, 0, filename, nWide, nullptr, 0, nullptr, nullptr);
		if (nMulti > 0)
		{
			std::vector<char> fileNameUTF8(nMulti+1);	// Include room for terminating null
			WideCharToMultiByte(CP_UTF8, 0, filename, nWide, fileNameUTF8.data(), nMulti, nullptr, nullptr);
			fileNameUTF8[nMulti] = 0;	// Add terminating null
			if (LMS_LoadConfig(m_device, fileNameUTF8.data()) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
		}
	}
	return S_OK;
}


STDMETHODIMP CLMSDevice::SaveConfig(BSTR filename, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	int nWide = SysStringLen(filename);
	if (nWide > 0)
	{
		int nMulti = WideCharToMultiByte(CP_UTF8, 0, filename, nWide, nullptr, 0, nullptr, nullptr);
		if (nMulti > 0)
		{
			std::vector<char> fileNameUTF8(nMulti+1);
			WideCharToMultiByte(CP_UTF8, 0, filename, nWide, fileNameUTF8.data(), nMulti, nullptr, nullptr);
			fileNameUTF8[nMulti] = 0;
			if (LMS_SaveConfig(m_device, fileNameUTF8.data()) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
		}
	}
	return S_OK;
}


STDMETHODIMP CLMSDevice::SetupStream(ILMSStream * stream, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (stream == nullptr) return E_INVALIDARG;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	lms_stream_t * lms_stream = dynamic_cast<lms_stream_t *>(stream);	// Warning, danger here!
																		// This only works for ILMSStream objects we create.
																		// Better to keep a set of valid ILMSStream objects.
	if (lms_stream == nullptr) return E_INVALIDARG;

	if (LMS_SetupStream(m_device, lms_stream) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;
	return S_OK;
}


STDMETHODIMP CLMSDevice::DestroyStream(ILMSStream * stream, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (stream == nullptr) return E_INVALIDARG;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	lms_stream_t * lms_stream = dynamic_cast<lms_stream_t *>(stream);	// Warning, danger here!
																		// This only works for ILMSStream objects we create.
																		// Better to keep a set of valid ILMSStream objects.
	if (lms_stream == nullptr) return E_INVALIDARG;

	if (LMS_DestroyStream(m_device, lms_stream) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;
	return S_OK;
}


STDMETHODIMP CLMSDevice::GetChannel(VARIANT_BOOL dir_tx, ULONG chan, ILMSChannel ** pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;

	CComObject<CLMSChannel> *pChannel;
	HRESULT hr = CComObject<CLMSChannel>::CreateInstance(&pChannel);
	if (SUCCEEDED(hr))
	{
		pChannel->AddRef();
		pChannel->m_dir_tx = (dir_tx == VARIANT_TRUE);
		pChannel->m_channel = chan;
		pChannel->m_ILMSDevice = CComPtr<ILMSDevice>(this);

		hr = pChannel->QueryInterface(pVal);
		pChannel->Release();
	}

	return hr;
}


STDMETHODIMP CLMSDevice::SetSampleRateDir(VARIANT_BOOL dir_tx, DOUBLE rate, ULONG oversample, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (m_device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetSampleRateDir(m_device, dir_tx == VARIANT_TRUE, rate, oversample) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}
