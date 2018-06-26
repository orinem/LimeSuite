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
// LMSLogHandler.cpp : Implementation of CLMSLogHandler

#include "stdafx.h"
#include "LMSLogHandler.h"

// CLMSLogHandler

STDMETHODIMP CLMSLogHandler::LogHandler(LMS_LOG_LEVEL lvl, BSTR msg)
{
	return S_OK;
}
