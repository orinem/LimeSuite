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
// LMSStream.h : Declaration of the CLMSStream

#pragma once
#include "resource.h"       // main symbols
#include "LimeCOM_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CLMSStream

class ATL_NO_VTABLE CLMSStream :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLMSStream, &CLSID_LMSStream>,
	public IDispatchImpl<ILMSStream, &IID_ILMSStream, &LIBID_LimeCOMLib, /*wMajor =*/ 1, /*wMinor =*/ 0>,
	public lms_stream_t
{
public:
	CLMSStream()
	{
		handle = 0;
		isTx = false;
		channel = 0;
		fifoSize = 0;
		throughputVsLatency = 0.0;
		dataFmt = lms_stream_t::LMS_FMT_F32;

		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LMSSTREAM)

DECLARE_NOT_AGGREGATABLE(CLMSStream)

BEGIN_COM_MAP(CLMSStream)
	COM_INTERFACE_ENTRY(ILMSStream)
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
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

public:
	STDMETHOD(get_isTx)(VARIANT_BOOL* pVal);
	STDMETHOD(put_isTx)(VARIANT_BOOL newVal);
	STDMETHOD(get_channel)(ULONG* pVal);
	STDMETHOD(put_channel)(ULONG newVal);
	STDMETHOD(get_fifoSize)(ULONG* pVal);
	STDMETHOD(put_fifoSize)(ULONG newVal);
	STDMETHOD(get_throughputVsLatency)(FLOAT* pVal);
	STDMETHOD(put_throughputVsLatency)(FLOAT newVal);
	STDMETHOD(get_dataFmt)(LMS_FMT * pVal);
	STDMETHOD(put_dataFmt)(LMS_FMT newVal);
	STDMETHOD(Start)(VARIANT_BOOL* pVal);
	STDMETHOD(Stop)(VARIANT_BOOL* pVal);
	STDMETHOD(Recv)(void *samples, ULONG * sample_count, ULONG timeout_ms, VARIANT_BOOL * pVal);
	STDMETHOD(RecvWithMeta)(void *samples, ULONG * sample_count, LMSStreamMeta * meta, ULONG timeout_ms, VARIANT_BOOL * pVal);
	STDMETHOD(GetStatus)(LMSStreamStatus * status, VARIANT_BOOL* pVal);
	STDMETHOD(Send)(void *samples, ULONG * sample_count, ULONG timeout_ms, VARIANT_BOOL * pVal);
	STDMETHOD(SendWithMeta)(void *samples, ULONG * sample_count, LMSStreamMeta * meta, ULONG timeout_ms, VARIANT_BOOL * pVal);
};

OBJECT_ENTRY_AUTO(__uuidof(LMSStream), CLMSStream)
