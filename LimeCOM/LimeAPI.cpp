// LimeAPI.cpp : Implementation of CLimeAPI

#include "stdafx.h"
#include "LimeAPI.h"
#include "LMSDevice.h"
#include <vector>

// CLimeAPI

STDMETHODIMP CLimeAPI::Open(ILMSDevice ** device, BSTR info, VARIANT_BOOL* pVal)
{
	if (device == nullptr || pVal == nullptr ) return E_POINTER;

	*pVal = VARIANT_FALSE;
	*device = nullptr;

	// Create the COM container
	CComObject<CLMSDevice> *pDevice;
	HRESULT hr = CComObject<CLMSDevice>::CreateInstance(&pDevice);
	if (SUCCEEDED(hr))
	{
		pDevice->AddRef();

		// Open the device, converting the BSTR if necessary
		if (info == nullptr)
		{
			if (LMS_Open(&pDevice->m_device, nullptr, nullptr) == LMS_SUCCESS)
				*pVal = VARIANT_TRUE;
		}
		else
		{
			// Assuming chars are UTF8...
			lms_info_str_t infoStr;
			if ( WideCharToMultiByte(CP_UTF8, 0, info, -1, &infoStr[0], sizeof(infoStr), nullptr, nullptr) != 0 &&
				 LMS_Open(&pDevice->m_device, infoStr, nullptr) == LMS_SUCCESS )
				*pVal = VARIANT_TRUE;
		}

		// If we succeeded, increment reference count and get ILMSDevice pointer
		if (*pVal == VARIANT_TRUE)
		{
			hr = pDevice->QueryInterface(device);
		}

		pDevice->Release();	// This will delete the LMSDevice object if we failed
	}

	return hr;
}


STDMETHODIMP CLimeAPI::get_DeviceList(SAFEARRAY ** pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = nullptr;

	HRESULT hr = S_OK;
	int nDevices = LMS_GetDeviceList(nullptr);
	if (nDevices < 0) nDevices = 0;
	try {
		std::vector<lms_info_str_t> deviceStrings(nDevices);
		if (nDevices > 0)
		{
			if (LMS_GetDeviceList(deviceStrings.data()) != (int)deviceStrings.size())
			{
				return E_UNEXPECTED;	// We are so doomed!
			}
		}
		*pVal = SafeArrayCreateVector(VT_BSTR, 0, (ULONG)deviceStrings.size());
		if ( *pVal == nullptr )
			hr = E_OUTOFMEMORY;
		else if ( deviceStrings.size() > 0 )
		{
			BSTR * pArrayData;
			hr = SafeArrayAccessData(*pVal, (void **)&pArrayData);
			if (SUCCEEDED(hr))
			{
				for (size_t i = 0; i < deviceStrings.size(); ++i)
				{
					int nUTF8 = (int)strlen(deviceStrings[i]);
					int nWide = MultiByteToWideChar(CP_UTF8, 0, deviceStrings[i], nUTF8, nullptr, 0);
					if (nWide > 0)
					{
						pArrayData[i] = SysAllocStringLen(nullptr, nWide);
						if (pArrayData[i] == nullptr)
							hr = E_OUTOFMEMORY;
						else
							MultiByteToWideChar(CP_UTF8, 0, deviceStrings[i], nUTF8, pArrayData[i], nWide);
					}
				}
				SafeArrayUnaccessData(*pVal);
			}
		}
	}
	catch (std::exception &)
	{
		hr = E_OUTOFMEMORY;
		if (pVal != nullptr)
		{
			SafeArrayDestroy(*pVal);
			*pVal = nullptr;
		}
	}

	return hr;
}


STDMETHODIMP CLimeAPI::get_LibraryVersion(BSTR * pVal)
{
	if (pVal == nullptr) return E_POINTER;
	*pVal = nullptr;
	HRESULT hr = S_OK;

	const char * version = LMS_GetLibraryVersion();

	int nUTF8 = (int)strlen(version);
	int nWide = MultiByteToWideChar(CP_UTF8, 0, version, nUTF8, nullptr, 0);
	if (nWide > 0)
	{
		*pVal = SysAllocStringLen(nullptr, nWide);
		if (*pVal == nullptr)
			hr = E_OUTOFMEMORY;
		else
			MultiByteToWideChar(CP_UTF8, 0, version, nUTF8, *pVal, nWide);
	}

	return hr;
}


CComPtr<ILMSLogHandler>	pMyLogHandler;

void LogHandler(int lvl, const char *msg)
{
	if (pMyLogHandler && msg != nullptr)
	{
		int nUTF8 = (int)strlen(msg);
		int nWide = MultiByteToWideChar(CP_UTF8, 0, msg, nUTF8, nullptr, 0);
		if (nWide > 0)
		{
			BSTR bstrMsg = SysAllocStringLen(nullptr, nWide);
			if (bstrMsg != nullptr)
			{
				MultiByteToWideChar(CP_UTF8, 0, msg, nUTF8, bstrMsg, nWide);
				pMyLogHandler->LogHandler((LMS_LOG_LEVEL)lvl, bstrMsg);
				SysFreeString(bstrMsg);
			}
		}
	}
}


STDMETHODIMP CLimeAPI::RegisterLogHandler(ILMSLogHandler * handler)
{
	if (handler != nullptr)
	{
		pMyLogHandler = handler;
		LMS_RegisterLogHandler(LogHandler);
	}
	else
	{
		LMS_RegisterLogHandler(nullptr);
		pMyLogHandler = nullptr;
	}

	return S_OK;
	}


