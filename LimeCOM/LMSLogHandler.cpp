// LMSLogHandler.cpp : Implementation of CLMSLogHandler

#include "stdafx.h"
#include "LMSLogHandler.h"


// CLMSLogHandler

STDMETHODIMP CLMSLogHandler::LogHandler(LMS_LOG_LEVEL lvl, BSTR msg)
{
	return S_OK;
}
