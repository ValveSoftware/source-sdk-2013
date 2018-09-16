/******************************************************************************
*   srengext.h 
*       This file contains the declaration of the CSampleSRExtension class.
*       This implements the custom interface ISampleSRExtension.
*       When an app QI's for this from the reco context, SAPI will
*       look for the ExtensionCLSID field in the engine object token, and
*       create this object and then QI for the requested interface.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#pragma once

#include "stdafx.h" 
#include "sreng.h"
#include "resource.h"

class ATL_NO_VTABLE CSampleSRExtension : 
public CComObjectRootEx<CComMultiThreadModel>,
public CComCoClass<CSampleSRExtension, &CLSID_SampleSRExtension>,
public ISampleSRExtension
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_SRENGEXT)
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_PROTECT_FINAL_CONSTRUCT()
        
BEGIN_COM_MAP(CSampleSRExtension)
    COM_INTERFACE_ENTRY(ISampleSRExtension)
END_COM_MAP()
        
    HRESULT FinalConstruct()
    {
        // We can query back to SAPI to find both the reco context, and,
        // an IID__ISpPrivateEngineCall interface which can be used to call
        // back to the main engine object.
        HRESULT hr;
        hr = OuterQueryInterface(IID__ISpPrivateEngineCall, (void **)&m_pEngineCall);
        if(SUCCEEDED(hr))
        {
            hr = OuterQueryInterface(IID_ISpRecoContext, (void **)&m_pRecoCtxt);
            if (SUCCEEDED(hr))
            {
                GetControllingUnknown()->Release();
            }
        }
        return hr;
    }
    
    STDMETHODIMP ExamplePrivateEngineCall(void); // Just a test method

private:
    _ISpPrivateEngineCall *m_pEngineCall;
    ISpRecoContext *m_pRecoCtxt;
};
