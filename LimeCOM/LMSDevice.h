// LMSDevice.h : Declaration of the CLMSDevice

#pragma once
#include "resource.h"       // main symbols
#include "LimeCOM_i.h"

using namespace ATL;

// CLMSDevice

class ATL_NO_VTABLE CLMSDevice :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLMSDevice, &CLSID_LMSDevice>,
	public IDispatchImpl<ILMSDevice, &IID_ILMSDevice, &LIBID_LimeCOMLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CLMSDevice()
	{
		m_device = nullptr;
		m_pUnkMarshaler = nullptr;
	}

DECLARE_REGISTRY_RESOURCEID(107)

DECLARE_NOT_AGGREGATABLE(CLMSDevice)

BEGIN_COM_MAP(CLMSDevice)
	COM_INTERFACE_ENTRY(ILMSDevice)
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

	lms_device_t *m_device;
public:
	STDMETHOD(Close)(VARIANT_BOOL* pVal);
	STDMETHOD(Reset)(VARIANT_BOOL* pVal);
	STDMETHOD(Init)(VARIANT_BOOL* pVal);
	STDMETHOD(get_NumChannels)(VARIANT_BOOL dir_tx, LONG* pVal);
	STDMETHOD(EnableChannel)(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enabled, VARIANT_BOOL* pVal);
	STDMETHOD(SetSampleRate)(DOUBLE rate, ULONG oversample, VARIANT_BOOL* pVal);
	STDMETHOD(GetSampleRate)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE* host_Hz, DOUBLE* rf_Hz, VARIANT_BOOL* pVal);
	STDMETHOD(SetLOFrequency)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE frequency, VARIANT_BOOL* pVal);
	STDMETHOD(GetLOFrequency)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE* frequency, VARIANT_BOOL* pVal);
	STDMETHOD(GetAntennaList)(VARIANT_BOOL dir_tx, ULONG chan, SAFEARRAY ** list);
	STDMETHOD(SetAntenna)(VARIANT_BOOL dir_tx, ULONG chan, LMS_PATH index, VARIANT_BOOL* pVal);
	STDMETHOD(GetAntenna)(VARIANT_BOOL dir_tx, ULONG chan, LMS_PATH* index, VARIANT_BOOL* pVal);
	STDMETHOD(GetLPFBWRange)(VARIANT_BOOL dir_tx, LMSRange * range, VARIANT_BOOL* pVal);
	STDMETHOD(SetLPF)(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enable, VARIANT_BOOL* pVal);
	STDMETHOD(SetGFIRLPF)(VARIANT_BOOL dir_tx, ULONG chan, VARIANT_BOOL enable, DOUBLE bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(Calibrate)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE bandwidth, ULONG flags, VARIANT_BOOL* pVal);
	STDMETHOD(SetLPFBW)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(GetLPFBW)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE * bandwidth, VARIANT_BOOL* pVal);
	STDMETHOD(SetNormalizedGain)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE gain, VARIANT_BOOL* pVal);
	STDMETHOD(GetNormalizedGain)(VARIANT_BOOL dir_tx, ULONG chan, DOUBLE * gain, VARIANT_BOOL* pVal);
	STDMETHOD(SetGaindB)(VARIANT_BOOL dir_tx, ULONG chan, ULONG gain, VARIANT_BOOL* pVal);
	STDMETHOD(GetGaindB)(VARIANT_BOOL dir_tx, ULONG chan, ULONG * gain, VARIANT_BOOL* pVal);
	STDMETHOD(LoadConfig)(BSTR filename, VARIANT_BOOL* pVal);
	STDMETHOD(SaveConfig)(BSTR filename, VARIANT_BOOL* pVal);
	STDMETHOD(SetTestSignal)(VARIANT_BOOL dir_tx, ULONG chan, LMS_TESTSIG sig, USHORT dc_i, USHORT dc_q, VARIANT_BOOL* pVal);
	STDMETHOD(GetTestSignal)(VARIANT_BOOL dir_tx, ULONG chan, LMS_TESTSIG * sig, VARIANT_BOOL* pVal);
	STDMETHOD(SetupStream)(ILMSStream * stream, VARIANT_BOOL* pVal);
	STDMETHOD(DestroyStream)(ILMSStream * stream, VARIANT_BOOL* pVal);
	STDMETHOD(GetChannel)(VARIANT_BOOL dir_tx, ULONG chan, ILMSChannel ** pVal);
	STDMETHOD(SetSampleRateDir)(VARIANT_BOOL dir_tx, DOUBLE rate, ULONG oversample, VARIANT_BOOL* pVal);
	STDMETHOD(VCTCXOWrite)(USHORT VCTXOVal, VARIANT_BOOL* pVal);
	STDMETHOD(VCTCXORead)(USHORT * pVCTXOVal, VARIANT_BOOL* pVal);
};

OBJECT_ENTRY_AUTO(__uuidof(LMSDevice), CLMSDevice)
