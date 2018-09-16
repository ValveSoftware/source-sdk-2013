/******************************************************************************
*   srengui.cpp 
*       This file contains the declaration of the CSrEngineUI class.
*       This implements ISpTokenUI. This is used by the app to display UI.
*       The methods here can either be called by the app directly from ISpObjectToken,
*       or they can be called from the reco instance, in which case the methods
*       are able to make a private call back to the main engine object.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#pragma once

#include "resource.h"

class ATL_NO_VTABLE CSrEngineUI : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSrEngineUI, &CLSID_SampleSREngineUI>,
    public ISpTokenUI
{
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_SRENGUI)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSrEngineUI)
    	COM_INTERFACE_ENTRY(ISpTokenUI)
    END_COM_MAP()

public:

    STDMETHODIMP IsUISupported(
                    const WCHAR * pszTypeOfUI,
                    void * pvExtraData,
                    ULONG cbExtraData,
                    IUnknown * punkObject,
                    BOOL *pfSupported);
    STDMETHODIMP DisplayUI(
                    HWND hwndParent,
                    const WCHAR * pszTitle,
                    const WCHAR * pszTypeOfUI,
                    void * pvExtraData,
                    ULONG cbExtraData,
                    ISpObjectToken * pToken,
                    IUnknown * punkObject);
};
