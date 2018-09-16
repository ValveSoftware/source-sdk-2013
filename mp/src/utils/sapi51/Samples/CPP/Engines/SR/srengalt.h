/******************************************************************************
*   srengalt.h 
*       This file contains the declaration of the CSrEngineAlternates class.
*       This implements the interface ISpSRAlternates.
*       When an app calls GetAlternates or Commit on a result, SAPI will
*       look for the AlternatesCLSID field in the engine object token, and
*       create this object.
*       It will then call the methods here, passing the relevant results information.
*       This includes any serialized data the main engine has returned with
*       the results to allow alternates to be generated off-line.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#pragma once

#include "stdafx.h"
#include "sreng.h"
#include "resource.h"

class ATL_NO_VTABLE CSrEngineAlternates : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSrEngineAlternates, &CLSID_SampleSREngineAlternates>,
    public ISpSRAlternates
{
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SRENGALT)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSrEngineAlternates)
    	COM_INTERFACE_ENTRY(ISpSRAlternates)
    END_COM_MAP()

public:

    STDMETHODIMP GetAlternates( 
        SPPHRASEALTREQUEST *pAltRequest,
        SPPHRASEALT **ppAlts,
        ULONG *pcAlts);
    
    STDMETHODIMP Commit( 
        SPPHRASEALTREQUEST *pAltRequest,
        SPPHRASEALT *pAlt,
        void **ppvResultExtra,
        ULONG *pcbResultExtra);
};
