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
// LimeAPI.h : Declaration of the CLimeAPI

#pragma once
#include "resource.h"       // main symbols
#include "LimeCOM_i.h"

using namespace ATL;

// CLimeAPI

class ATL_NO_VTABLE CLimeAPI :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLimeAPI, &CLSID_LimeAPI>,
	public IDispatchImpl<ILimeAPI, &IID_ILimeAPI, &LIBID_LimeCOMLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CLimeAPI()
	{
		m_pUnkMarshaler = nullptr;
	}

DECLARE_REGISTRY_RESOURCEID(106)

DECLARE_NOT_AGGREGATABLE(CLimeAPI)

BEGIN_COM_MAP(CLimeAPI)
	COM_INTERFACE_ENTRY(ILimeAPI)
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
	STDMETHOD(get_LibraryVersion)(BSTR * pVal);
	STDMETHOD(get_DeviceList)(SAFEARRAY ** pVal);
	STDMETHOD(Open)(ILMSDevice ** device, BSTR info, VARIANT_BOOL* pVal);
	STDMETHOD(RegisterLogHandler)(ILMSLogHandler * handler);
};

OBJECT_ENTRY_AUTO(__uuidof(LimeAPI), CLimeAPI)
