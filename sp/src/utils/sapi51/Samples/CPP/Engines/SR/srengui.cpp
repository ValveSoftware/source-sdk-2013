/******************************************************************************
*   srengui.cpp 
*       This file contains the implementation of the CSrEngineUI class.
*       This implements ISpTokenUI. This is used by the app to display UI.
*       The methods here can either be called by the app directly from ISpObjectToken,
*       or they can be called from the reco instance, in which case the methods
*       are able to make a private call back to the main engine object.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "sreng.h"
#include "srengui.h"

/****************************************************************************
* CSrEngineUI::IsUISupported *
*----------------------------*
*   Description:  
*       Determine if the UI is supported. A reference to the main SR engine 
*       object (if it has been created), can be obtained from punkObject.
*       If none-NULL this may be an ISpRecoContext, from which an engine
*       extension interface can be obtained.
*
*   Return:
*       S_OK on success
*       E_INVALIDARG on invalid arguments
*****************************************************************************/
STDMETHODIMP CSrEngineUI::IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, IUnknown * punkObject, BOOL *pfSupported)
{
    *pfSupported = FALSE;

    // We can do both engine specific properties as well as default settings (defaults when punkObject == NULL)
    if (wcscmp(pszTypeOfUI, SPDUI_EngineProperties) == 0)
        *pfSupported = TRUE;

    // We can only do user training if we get passed an engine
    if (wcscmp(pszTypeOfUI, SPDUI_UserTraining) == 0 && punkObject != NULL)
        *pfSupported = TRUE;

    // We can only do mic training if we get passed an engine
    if (wcscmp(pszTypeOfUI, SPDUI_MicTraining) == 0 && punkObject != NULL)
        *pfSupported = TRUE;

    return S_OK;
}

/****************************************************************************
* CSrEngineUI::DisplayUI *
*------------------------*
*   Description:  
*       Display the UI requested
*
*   Return:
*       S_OK on success
*****************************************************************************/
STDMETHODIMP CSrEngineUI::DisplayUI(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, ISpObjectToken * pToken, IUnknown * punkObject)
{

    if (wcscmp(pszTypeOfUI, SPDUI_EngineProperties) == 0)
    {
        if (punkObject)
        {
            MessageBoxW(hwndParent, L"Developer Sample Engine: Replace this with real engine properties dialog.", pszTitle, MB_OK);
        }
    }
    
    if (wcscmp(pszTypeOfUI, SPDUI_UserTraining) == 0)
    {
        MessageBoxW(hwndParent, L"Developer Sample Engine: Replace this with real user training wizard / dialog.", pszTitle, MB_OK);
    }
    
    if (wcscmp(pszTypeOfUI, SPDUI_MicTraining) == 0)
    {
        MessageBoxW(hwndParent, L"Developer Sample Engine: Replace this with real microphone training wizard / dialog.", pszTitle, MB_OK);
    }
    
    return S_OK;
}
