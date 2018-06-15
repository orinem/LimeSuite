// LMSChannel.cpp : Implementation of CLMSChannel

#include "stdafx.h"
#include "LMSChannel.h"
#include "LMSDevice.h"
#include <vector>

// CLMSChannel

lms_device_t * CLMSChannel::GetLMSDevice()
{
	if ( m_ILMSDevice )
	{
		CLMSDevice * pCOMDevice = dynamic_cast<CLMSDevice *>((ILMSDevice *)m_ILMSDevice);
		if ( pCOMDevice != nullptr )
			return pCOMDevice->m_device;
	}

	return nullptr;
}


STDMETHODIMP CLMSChannel::get_dir_tx(VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (!m_ILMSDevice) return OLE_E_BLANK;

	*pVal = m_dir_tx;

	return S_OK;
}


STDMETHODIMP CLMSChannel::get_channel(ULONG* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (!m_ILMSDevice) return OLE_E_BLANK;

	*pVal = m_channel;

	return S_OK;
}


STDMETHODIMP CLMSChannel::get_device(ILMSDevice ** pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (!m_ILMSDevice) return OLE_E_BLANK;

	*pVal = nullptr;
	return m_ILMSDevice.QueryInterface(pVal);
}

STDMETHODIMP CLMSChannel::EnableChannel(VARIANT_BOOL enabled, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_EnableChannel(device, m_dir_tx, m_channel, (enabled == VARIANT_TRUE)) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetSampleRate(DOUBLE* host_Hz, DOUBLE* rf_Hz, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	// host_Hz and rf_Hz are allowed to be NULL by LMS_GetSampleRate() so no need to check

	*pVal = VARIANT_FALSE;
	if (LMS_GetSampleRate(device, m_dir_tx, m_channel, host_Hz, rf_Hz) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetLOFrequency(DOUBLE frequency, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_SetLOFrequency(device, m_dir_tx, m_channel, frequency) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetLOFrequency(DOUBLE* frequency, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (frequency == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_GetLOFrequency(device, m_dir_tx, m_channel, frequency) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetAntennaList(SAFEARRAY ** list)
{
	if (list == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*list = nullptr;

	HRESULT hr = S_OK;
	int nDevices = LMS_GetAntennaList(device, m_dir_tx, m_channel, NULL);
	if (nDevices < 0) nDevices = 0;
	try {
		std::vector<lms_name_t> antennaStrings(nDevices);
		if (nDevices > 0)
		{
			if (LMS_GetAntennaList(device, m_dir_tx, m_channel, antennaStrings.data()) != (int)antennaStrings.size())
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


STDMETHODIMP CLMSChannel::SetAntenna(LMS_PATH index, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	if (LMS_SetAntenna(device, m_dir_tx, m_channel, index) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetAntenna(LMS_PATH * index, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (index == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if ( device == nullptr ) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;
	int i = LMS_GetAntenna(device, m_dir_tx, m_channel);
	if (i >= 0)
	{
		*index = (LMS_PATH)i;
		*pVal = VARIANT_TRUE;
	}

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetLPF(VARIANT_BOOL enable, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetLPF(device, m_dir_tx, m_channel, (enable == VARIANT_TRUE)) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetGFIRLPF(VARIANT_BOOL enable, DOUBLE bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetGFIRLPF(device, m_dir_tx, m_channel, (enable == VARIANT_TRUE), bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::Calibrate(DOUBLE bandwidth, ULONG flags, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_Calibrate(device, m_dir_tx, m_channel, bandwidth, flags) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetLPFBW(DOUBLE bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetLPFBW(device, m_dir_tx, m_channel, bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetLPFBW(DOUBLE * bandwidth, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (bandwidth == nullptr) return E_POINTER;
	if (!m_ILMSDevice) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;
	lms_device_t * device = dynamic_cast<lms_device_t *>((ILMSDevice *)m_ILMSDevice);

	if (LMS_GetLPFBW(device, m_dir_tx, m_channel, bandwidth) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetNormalizedGain(DOUBLE gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetNormalizedGain(device, m_dir_tx, m_channel, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetNormalizedGain(DOUBLE * gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (gain == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_GetNormalizedGain(device, m_dir_tx, m_channel, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::SetGaindB(ULONG gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetGaindB(device, m_dir_tx, m_channel, gain) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetGaindB(ULONG * gain, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (gain == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	unsigned uGain;
	if (LMS_GetGaindB(device, m_dir_tx, m_channel, &uGain) == LMS_SUCCESS)
	{
		*gain = uGain;
		*pVal = VARIANT_TRUE;
	}
	return S_OK;
}


STDMETHODIMP CLMSChannel::SetTestSignal(LMS_TESTSIG sig, USHORT dc_i, USHORT dc_q, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
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
	if (LMS_SetTestSignal(device, m_dir_tx, m_channel, lmsSig, dc_i, dc_q) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::GetTestSignal(LMS_TESTSIG * sig, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	if (sig == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	lms_testsig_t lmsSig;
	if (LMS_GetTestSignal(device, m_dir_tx, m_channel, &lmsSig) == LMS_SUCCESS)
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


STDMETHODIMP CLMSChannel::GetNCOFrequency(SAFEARRAY ** frequency, DOUBLE* pho, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr || frequency == nullptr || pho == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	HRESULT hr = S_OK;
	*frequency = SafeArrayCreateVector(VT_R8, 0, (ULONG)LMS_NCO_VAL_COUNT);
	if (*frequency == nullptr)
		hr = E_OUTOFMEMORY;
	else
	{
		DOUBLE * pArrayData;
		hr = SafeArrayAccessData(*frequency, (void **)&pArrayData);
		if (SUCCEEDED(hr))
		{
			if (LMS_GetNCOFrequency(device, m_dir_tx, m_channel, pArrayData, pho) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
			SafeArrayUnaccessData(*frequency);
		}
	}

	return hr;
}


STDMETHODIMP CLMSChannel::SetNCOFrequency(SAFEARRAY * frequency, DOUBLE pho, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	VARTYPE vt;
	LONG lBound, uBound;
	DOUBLE * pArrayData;
	HRESULT hr = E_INVALIDARG;
	if (frequency != nullptr &&
		SafeArrayGetDim(frequency) == 1 &&
		SUCCEEDED(SafeArrayGetVartype(frequency, &vt)) && vt == VT_R8 &&
		SUCCEEDED(SafeArrayGetLBound(frequency, 1, &lBound)) &&
		SUCCEEDED(SafeArrayGetUBound(frequency, 1, &uBound)) &&
		uBound - lBound + 1 >= LMS_NCO_VAL_COUNT &&
		SUCCEEDED(SafeArrayAccessData(frequency, (void **)&pArrayData)))
	{
		hr = S_OK;
		if (LMS_SetNCOFrequency(device, m_dir_tx, m_channel, pArrayData, pho) == LMS_SUCCESS)
			*pVal = VARIANT_TRUE;
		SafeArrayUnaccessData(frequency);
	}

	return hr;
}


STDMETHODIMP CLMSChannel::GetNCOPhase(SAFEARRAY ** phases, DOUBLE* fcw, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr || phases == nullptr || fcw == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	HRESULT hr = S_OK;
	*phases = SafeArrayCreateVector(VT_R8, 0, (ULONG)LMS_NCO_VAL_COUNT);
	if (*phases == nullptr)
		hr = E_OUTOFMEMORY;
	else
	{
		DOUBLE * pArrayData;
		hr = SafeArrayAccessData(*phases, (void **)&pArrayData);
		if (SUCCEEDED(hr))
		{
			if (LMS_GetNCOPhase(device, m_dir_tx, m_channel, pArrayData, fcw) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
			SafeArrayUnaccessData(*phases);
		}
	}

	return hr;
}


STDMETHODIMP CLMSChannel::SetNCOPhase(SAFEARRAY * phases, DOUBLE fcw, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	VARTYPE vt;
	LONG lBound, uBound;
	DOUBLE * pArrayData;
	HRESULT hr = E_INVALIDARG;
	if (phases != nullptr &&
		SafeArrayGetDim(phases) == 1 &&
		SUCCEEDED(SafeArrayGetVartype(phases, &vt)) && vt == VT_R8 &&
		SUCCEEDED(SafeArrayGetLBound(phases, 1, &lBound)) &&
		SUCCEEDED(SafeArrayGetUBound(phases, 1, &uBound)) &&
		uBound - lBound + 1 >= LMS_NCO_VAL_COUNT &&
		SUCCEEDED(SafeArrayAccessData(phases, (void **)&pArrayData)))
	{
		hr = S_OK;
		if (LMS_SetNCOPhase(device, m_dir_tx, m_channel, pArrayData, fcw) == LMS_SUCCESS)
			*pVal = VARIANT_TRUE;
		SafeArrayUnaccessData(phases);
	}

	return hr;
}


STDMETHODIMP CLMSChannel::SetNCOIndex(LONG index, VARIANT_BOOL downconv, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;
	*pVal = VARIANT_FALSE;

	if (LMS_SetNCOIndex(device, m_dir_tx, m_channel, index, downconv == VARIANT_TRUE) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::get_NCOIndex(LONG * pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = LMS_GetNCOIndex(device, m_dir_tx, m_channel);

	return S_OK;
}


STDMETHODIMP CLMSChannel::get_NCO_VAL_COUNT(ULONG * pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = LMS_NCO_VAL_COUNT;
	return S_OK;
}


STDMETHODIMP CLMSChannel::SetGFIRCoeff(LMS_GFIR filt, SAFEARRAY * coef, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;

	size_t maxCount = 40;
	lms_gfir_t lmsFilt = LMS_GFIR1;
	switch (filt)
	{
	case GFIR1:
		break;
	case GFIR2:
		lmsFilt = LMS_GFIR2;
		break;
	case GFIR3:
		lmsFilt = LMS_GFIR3;
		maxCount = 120;
		break;
	default:
		return E_INVALIDARG;
	}

	VARTYPE vt;
	LONG lBound, uBound;
	DOUBLE * pArrayData;
	HRESULT hr = E_INVALIDARG;
	if (coef != nullptr &&
		SafeArrayGetDim(coef) == 1 &&
		SUCCEEDED(SafeArrayGetVartype(coef, &vt)) && vt == VT_R8 &&
		SUCCEEDED(SafeArrayGetLBound(coef, 1, &lBound)) &&
		SUCCEEDED(SafeArrayGetUBound(coef, 1, &uBound)) &&
		uBound - lBound + 1 <= maxCount &&
		SUCCEEDED(SafeArrayAccessData(coef, (void **)&pArrayData)))
	{
		hr = S_OK;
		if (LMS_SetGFIRCoeff(device, m_dir_tx, m_channel, lmsFilt, pArrayData, uBound - lBound + 1) == LMS_SUCCESS)
			*pVal = VARIANT_TRUE;
		SafeArrayUnaccessData(coef);
	}

	return hr;
}


STDMETHODIMP CLMSChannel::GetGFIRCoeff(LMS_GFIR filt, SAFEARRAY ** coef, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;

	size_t maxCount = 40;
	lms_gfir_t lmsFilt = LMS_GFIR1;
	switch (filt)
	{
	case GFIR1:
		break;
	case GFIR2:
		lmsFilt = LMS_GFIR2;
		break;
	case GFIR3:
		lmsFilt = LMS_GFIR3;
		maxCount = 120;
		break;
	default:
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;
	*coef = SafeArrayCreateVector(VT_R8, 0, (ULONG)maxCount);
	if (*coef == nullptr)
		hr = E_OUTOFMEMORY;
	else
	{
		DOUBLE * pArrayData;
		hr = SafeArrayAccessData(*coef, (void **)&pArrayData);
		if (SUCCEEDED(hr))
		{
			if (LMS_GetGFIRCoeff(device, m_dir_tx, m_channel, lmsFilt, pArrayData) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
			SafeArrayUnaccessData(*coef);
		}
	}

	return hr;
}


STDMETHODIMP CLMSChannel::SetGFIR(LMS_GFIR filt, VARIANT_BOOL enabled, VARIANT_BOOL* pVal)
{
	if (pVal == nullptr) return E_POINTER;
	lms_device_t * device = GetLMSDevice();
	if (device == nullptr) return OLE_E_BLANK;

	*pVal = VARIANT_FALSE;

	lms_gfir_t lmsFilt = LMS_GFIR1;
	switch (filt)
	{
	case GFIR1:
		break;
	case GFIR2:
		lmsFilt = LMS_GFIR2;
		break;
	case GFIR3:
		lmsFilt = LMS_GFIR3;
		break;
	default:
		return E_INVALIDARG;
	}

	if (LMS_SetGFIR(device, m_dir_tx, m_channel, lmsFilt, enabled == VARIANT_TRUE) == LMS_SUCCESS)
		*pVal = VARIANT_TRUE;

	return S_OK;
}


STDMETHODIMP CLMSChannel::get_GFIRCoeffCount(LMS_GFIR filt, ULONG * pVal)
{
	if (pVal == nullptr) return E_POINTER;

	switch (filt)
	{
	case GFIR1:
	case GFIR2:
		*pVal = 40;
		break;
	case GFIR3:
		*pVal = 120;
		break;
	default:
		return E_INVALIDARG;
	}

	return S_OK;
}

