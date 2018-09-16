/******************************************************************************
*   srengalt.h 
*       This file contains the implementation of the CSrEngineAlternates class.
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

#include "stdafx.h"
#include "srengalt.h"

/****************************************************************************
* CSrEngineAlternates::GetAlternates *
*---------------------------*
*   Description:
*       This method generates alternate phrases when SAPI requests them.
*       The method reads the extra information returned from the SR engine
*       inside the results object. This gets returned
*       as alternates phrases to SAPI. In addition this method can find a private interface to engine
*       from the context and query the engine for additional result information.
*       
*       The engine must have returned info serialised within the results object
*       allowing us to produce alternatives. This is the case in the sample engine.
*
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngineAlternates::GetAlternates(SPPHRASEALTREQUEST *pAltRequest,
    SPPHRASEALT **ppAlts, ULONG *pcAlts)
{
    HRESULT hr = S_OK;

    // We will just produce one alternate
    // This will replace the words in the original phrase on a one-to-one basis
    // Real alternates may have different numbers of words to original,
    // and replace only parts of the original.
    *pcAlts = 1;
    *ppAlts = (SPPHRASEALT *)::CoTaskMemAlloc(sizeof(SPPHRASEALT));
    (*ppAlts)[0].ulStartElementInParent = pAltRequest->ulStartElement;
    (*ppAlts)[0].cElementsInParent = pAltRequest->cElements;
    (*ppAlts)[0].cElementsInAlternate = pAltRequest->cElements;
    (*ppAlts)[0].pvAltExtra = NULL;
    (*ppAlts)[0].cbAltExtra = 0;

    // Create and fill an SPPHRASE structure
    SPPHRASE phrase;
    memset(&phrase, 0, sizeof(phrase));
    phrase.cbSize = sizeof(phrase);
    // An alternates analyzer should really query its SR engine to find it's lang id. 
    //  For the sample engine we will just hard-code this
    phrase.LangID = 1033; 
    
    WCHAR *pAlts = (WCHAR *) pAltRequest->pvResultExtra;
    ULONG nAltChars = pAltRequest->cbResultExtra / sizeof(WCHAR);
    ULONG nWord = 0;

    // Count words in alternate data
    for(ULONG i = 0; i < nAltChars; i++)
    {
        if(iswspace(pAlts[i]) || pAlts[i] == '\0')
        {
            nWord++;
        }
    }

    // Allocate elements
	SPPHRASEELEMENT* pElements = (SPPHRASEELEMENT*)_alloca(sizeof(SPPHRASEELEMENT) * nWord);
    memset(pElements, 0, sizeof(SPPHRASEELEMENT)*nWord);

    // Add words in alternate to elements
    ULONG cW = 0;
    nWord = 0;
    for(i = 0; i < nAltChars; i++)
    {
        if(iswspace(pAlts[i]) || pAlts[i] == '\0')
        {
        	pElements[nWord].bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
            WCHAR *pszWord = (WCHAR *)_alloca(sizeof(WCHAR) * (i- cW));
            wcsncpy(pszWord, &pAlts[cW], i - cW);
            pszWord[i - cW] = '\0';
			pElements[nWord].pszDisplayText =  pszWord;

            cW = i + 1;
            nWord++;
        }
    }
        
    // Add elements to phrase
    phrase.Rule.ulCountOfElements = nWord;
    phrase.pElements = pElements;

    // Make phrase builder and add phrase info
    CComPtr<ISpPhraseBuilder> cpBuilder;
    hr = cpBuilder.CoCreateInstance(CLSID_SpPhraseBuilder);
    if(SUCCEEDED(hr))
    {
        hr = cpBuilder->InitFromPhrase(&phrase);
    }
    if(SUCCEEDED(hr))
    {
        (*ppAlts)[0].pPhrase = cpBuilder;
        (*ppAlts)[0].pPhrase->AddRef();
    }

    // Alternates class can also query enginethrough private interface
    CComPtr<ISampleSRExtension> m_cpExt;
    hr = pAltRequest->pRecoContext->QueryInterface(&m_cpExt);
    if(SUCCEEDED(hr))
    {
        hr = m_cpExt->ExamplePrivateEngineCall();
    }
    return hr;
}
    
/****************************************************************************
* CSrEngineAlternates::Commit *
*---------------------------*
*   Description:
*       Here the engine could use the information from the application 
*       about which was the correct alternate in order to do supervised
*       adaptation.
*       In this sample we do nothing here.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngineAlternates::Commit(SPPHRASEALTREQUEST *pAltRequest,
    SPPHRASEALT *pAlt, void **ppvResultExtra, ULONG *pcbResultExtra)
{
    return S_OK;
}
