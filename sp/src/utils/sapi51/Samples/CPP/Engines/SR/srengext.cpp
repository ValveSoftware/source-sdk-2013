/******************************************************************************
*   srengext.cpp 
*       This file contains the implementation of the CSampleSRExtension class.
*       This implements the custom interface ISampleSRExtension.
*       When an app QI's for this from the reco context, SAPI will
*       look for the ExtensionCLSID field in the engine object token, and
*       create this object and then QI for the requested interface.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "srengext.h"

/****************************************************************************
* CSampleSRExtension::ExamplePrivateEngineCall *
*----------------------------------------------*
*   Description:
*       This method shows an example of calling back to the main engine object.
*       When CallEngine is called, the data supplied will get passed by SAPI
*       to the ISpSREngine::PrivateCall method in CSrEngine.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSampleSRExtension::ExamplePrivateEngineCall(void)
{
    // We can use this method to pass data to and from the actual engine class, via the context
    static BYTE Data[4] = { 1, 2, 3, 4 };
    return m_pEngineCall->CallEngine( (void*)Data, sp_countof(Data) );
}
