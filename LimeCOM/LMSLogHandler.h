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
