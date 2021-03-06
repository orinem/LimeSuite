/*
* Copyright (C) 2018 Orin Eman
*
* Based on the LimeSuite API which is Copyright(C) 2016 Lime Microsystems
* and can be found at https://github.com/myriadrf/LimeSuite
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
// LMSChannel.h : Declaration of the CLMSChannel

#pragma once
#include "resource.h"       // main symbols
#include "LimeCOM_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CLMSChannel

class ATL_NO_VTABLE CLMSChannel :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLMSChannel, &CLSID_LMSChannel>,
	public IDispatchImpl<ILMSChannel, &IID_ILMSChannel, &LIBID_LimeCOMLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CLMSChannel()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LMSCHANNEL)

DECLARE_NOT_AGGREGATABLE(CLMSChannel)

BEGIN_COM_MAP(CLMSChannel)
	COM_INTERFACE_ENTRY(ILMSChannel)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_GET_CONTROLLING_UNKNOWN()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
		m_ILMSDevice.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

	ULONG				m_channel;
	bool				m_dir_tx;
	CComPtr<ILMSDevice>  m_ILMSDevice;

	lms_device_t * GetLMSDevice();
public:
	STDMETHOD(get_dir_tx)(VARIANT_BOOL* pVal);
	STDMETHOD(get_channel)(ULONG* pVal);
	STDMETHOD(get_device)(ILMSDevice ** pVal);
	STDMETHOD(EnableChannel)(VARIANT_BOOL enabled, VARIANT_BOOL* pVal);
	STDMETHOD(GetSampleRate)(DOUBLE* host_Hz, DOUBLE* rf_Hz, VARIANT_BOOL* pVal);
	STDMETHOD(SetLOFrequency)(DOUBLE frequency, VARIANT_BOOL* pVal);
	STDMETHOD(GetLOFrequency)(DOUBLE* frequency, VARIANT_BOOL* pVal);
	STDMETHOD(GetAntennaList)(SAFEARRAY ** list);
	STDMETHOD(SetAntenna)(LMS_PATH index, VARIANT_BOOL* pVal);
	STDMETHOD(GetAntenna)(LMS_PATH* index, VARIANT_BOOL* pVal);
	STDMETHOD(SetLPF)(VARIANT_BOOL enable, VARIANT_BOOL* pVal);
	STDMETHOD(SetGFIRLPF)(VARIANT_BOOL enable, DOUBLE bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(Calibrate)(DOUBLE bandwidth, ULONG flags, VARIANT_BOOL* pVal);
	STDMETHOD(SetLPFBW)(DOUBLE bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(GetLPFBW)(DOUBLE * bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(SetNormalizedGain)(DOUBLE gain, VARIANT_BOOL* pVal);
	STDMETHOD(GetNormalizedGain)(DOUBLE * gain, VARIANT_BOOL* pVal);
	STDMETHOD(SetGaindB)(ULONG gain, VARIANT_BOOL* pVal);
	STDMETHOD(GetGaindB)(ULONG * gain, VARIANT_BOOL* pVal);
	STDMETHOD(SetTestSignal)(LMS_TESTSIG sig, USHORT dc_i, USHORT dc_q, VARIANT_BOOL* pVal);
	STDMETHOD(GetTestSignal)(LMS_TESTSIG * sig, VARIANT_BOOL* pVal);
	STDMETHOD(SetNCOFrequency)(SAFEARRAY * frequency, DOUBLE pho, VARIANT_BOOL* pVal);
	STDMETHOD(GetNCOFrequency)(SAFEARRAY ** frequency, DOUBLE* pho, VARIANT_BOOL* pVal);
	STDMETHOD(SetNCOPhase)(SAFEARRAY * phases, DOUBLE fcw, VARIANT_BOOL* pVal);
	STDMETHOD(GetNCOPhase)(SAFEARRAY ** phases, DOUBLE* fcw, VARIANT_BOOL* pVal);
	STDMETHOD(SetNCOIndex)(LONG index, VARIANT_BOOL downconv, VARIANT_BOOL* pVal);
	STDMETHOD(get_NCOIndex)(LONG * pVal);
	STDMETHOD(get_NCO_VAL_COUNT)(ULONG * pVal);
	STDMETHOD(SetGFIRCoeff)(LMS_GFIR filt, SAFEARRAY * coef, VARIANT_BOOL* pVal);
	STDMETHOD(GetGFIRCoeff)(LMS_GFIR filt, SAFEARRAY ** coef, VARIANT_BOOL* pVal);
	STDMETHOD(SetGFIR)(LMS_GFIR filt, VARIANT_BOOL enabled, VARIANT_BOOL* pVal);
	STDMETHOD(get_GFIRCoeffCount)(LMS_GFIR filt, ULONG * pVal);
};

OBJECT_ENTRY_AUTO(__uuidof(LMSChannel), CLMSChannel)
