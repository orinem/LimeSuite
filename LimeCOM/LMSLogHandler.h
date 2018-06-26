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
// LMSLogHandler.h : Declaration of the CLMSLogHandler

#pragma once
#include "resource.h"       // main symbols
#include "LimeCOM_i.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;

// CLMSLogHandler

class ATL_NO_VTABLE CLMSLogHandler :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLMSLogHandler, &CLSID_LMSLogHandler>,
	public IDispatchImpl<ILMSLogHandler, &IID_ILMSLogHandler, &LIBID_LimeCOMLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CLMSLogHandler()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_LMSLOGHANDLER)

DECLARE_NOT_AGGREGATABLE(CLMSLogHandler)

BEGIN_COM_MAP(CLMSLogHandler)
	COM_INTERFACE_ENTRY(ILMSLogHandler)
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
	STDMETHOD(LogHandler)(LMS_LOG_LEVEL lvl, BSTR msg);
};

OBJECT_ENTRY_AUTO(__uuidof(LMSLogHandler), CLMSLogHandler)
