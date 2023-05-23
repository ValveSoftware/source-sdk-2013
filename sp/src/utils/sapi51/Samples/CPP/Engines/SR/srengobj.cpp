/******************************************************************************
*   srengobj.cpp 
*       This file contains the implementation of the CSrEngine class.
*       This implements ISpSREngine and ISpObjectWithToken.
*       This is the main recognition object
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "sreng.h"
#include "srengobj.h"
#include "SpHelper.h"
#ifndef _WIN32_WCE
#include "shfolder.h"
#endif

static const WCHAR DICT_WORD[] = L"Blah"; // This is the default word the sample engine uses for dictation
static const WCHAR ALT_WORD[] = L"Alt"; // This is the default word used for alternates

/****************************************************************************
* CSrEngine::FinalConstruct *
*---------------------------*
*   Description:
*       The ATL FinalConstruct method. Called after the standard C++ constructor
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
HRESULT CSrEngine::FinalConstruct()
{
    SPDBG_FUNC("Sample Engine - FINALCONSTRUCT");
    HRESULT hr = S_OK;

    // This event is used to indicate when space is available for data to be given to the recognition thread
    m_hQueueHasRoom = ::CreateEvent(NULL, TRUE, TRUE, NULL);
    m_FrameQueue.SetSpaceAvailEvent(m_hQueueHasRoom);

    // Create a thread control which will be used for the recognition thread
    CComPtr<ISpTaskManager> cpTaskMgr;    
    hr = cpTaskMgr.CoCreateInstance(CLSID_SpResourceManager);
    if (SUCCEEDED(hr))
    {
        hr = cpTaskMgr->CreateThreadControl(this, this, THREAD_PRIORITY_NORMAL, &m_cpDecoderThread);
    }

    if(SUCCEEDED(hr))
    {
        // Create the SAPI lexicon which holds user and app pronounciations
        hr = m_cpLexicon.CoCreateInstance(CLSID_SpLexicon);
    }
    return hr;
}


/****************************************************************************
* CSrEngine::FinalRelease *
*---------------------------*
*   Description:
*       The ATL FinalRelease method. Clean up any resources not automatically destructed.
*   Return: 
*       S_OK
*****************************************************************************/   
HRESULT CSrEngine::FinalRelease()
{
    SPDBG_FUNC("Sample Engine - FINALRELEASE");
    ::CloseHandle(m_hQueueHasRoom);
    return S_OK;
}

/****************************************************************************
* CSrEngine::SetObjectToken *
*---------------------------*
*   Description:
*       This method is called by SAPI immediately after the engine is created.
*       It can be used to get registry information specific to this engine.
*       The engine can recover from the token file paths stored there during installation.
*       The engine also can recover user set defaults for accuracy, rejection etc.
*       Also could have different engines sharing the same code base (CLSID) but having different registry info
*       e.g. if engine supports different languages
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("Sample Engine - SETOBJECTTOKEN");
    HRESULT hr = S_OK;

    // Helper function that copies the token reference.
    hr = SpGenericSetObjectToken(pToken, m_cpEngineObjectToken);
    if(FAILED(hr))
    {
        return hr;
    }

    // Read attribute information from the token in the registry
    CComPtr<ISpDataKey> cpAttribKey;
    hr = pToken->OpenKey(L"Attributes", &cpAttribKey);

    if(SUCCEEDED(hr))
    {
        WCHAR *psz = NULL;
        hr = cpAttribKey->GetStringValue(L"Desktop", &psz);
        ::CoTaskMemFree(psz);
        if(SUCCEEDED(hr))
        {
            // This instance of the engine is for doing desktop recognition
        }
        else if(hr == SPERR_NOT_FOUND)
        {
            hr = cpAttribKey->GetStringValue(L"Telephony", &psz);
            ::CoTaskMemFree(psz);
            if(SUCCEEDED(hr))
            {
                // This instance of the engine is for doing telephony recognition
            }
        }
    }

    // Read what language is set in the registry
    if(SUCCEEDED(hr))
    {
        WCHAR *pszLangID = NULL;
        hr = cpAttribKey->GetStringValue(L"Language", &pszLangID);
        if(SUCCEEDED(hr))
        {
            // We could use this language id in recognition.
        }
        else if(hr == SPERR_NOT_FOUND)
        {
            pszLangID = L"409"; // Default language (US English)
            hr = S_OK;
        }
        m_LangID = (unsigned short)wcstol(pszLangID, NULL, 16);
        ::CoTaskMemFree(pszLangID);
    }    

    // Read data-file location
    WCHAR *pszPath = NULL;

    // Looks for the file path stored in the registry
    hr = pToken->GetStorageFileName(CLSID_SampleSREngine, L"SampleEngDataFile", NULL, 0, &pszPath);
    // Could now load engine data-files from this given path
    
    ::CoTaskMemFree(pszPath);

    return hr;
}

/****************************************************************************
* CSrEngine::GetObjectToken *
*---------------------------*
*   Description:
*       This method is called if SAPI wants to find which object token this engine is using
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::GetObjectToken(ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("Sample Engine - GETOBJECTTOKEN");
    // Generic helper function
    return SpGenericGetObjectToken(ppToken, m_cpEngineObjectToken);
}


/****************************************************************************
* CSrEngine::SetSite *
*---------------------------*
*   Description:
*       This is called to give the engine a reference to the ISpSREngineSite.
*       The engine uses this to call back to SAPI.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetSite(ISpSREngineSite *pSite)
{
    m_cpSite = pSite;
    return S_OK;
}


/****************************************************************************
* CSrEngine::SetRecoProfile *
*---------------------------*
*   Description:
*       The RecoProfile is an object token holding information on the current
*       user and enrollment session. The engine can store whatever information here
*       it likes. It should store this in a key under the RecoProfile key named with the engine class id.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetRecoProfile(ISpObjectToken *pProfile)
{
    SPDBG_FUNC("Sample Engine - SETRECOPROFILE");

    // First find if our engine already has info in this profile
    HRESULT hr = S_OK;
    WCHAR *pszCLSID, *pszPath = NULL;
    CComPtr<ISpDataKey> dataKey;

    m_cpUserObjectToken = pProfile;

    hr = ::StringFromCLSID(CLSID_SampleSREngine, &pszCLSID);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = pProfile->OpenKey(pszCLSID, &dataKey);
    if(hr == SPERR_NOT_FOUND)
    {
        // We haven't seen this user profile before, so create a new registry key to hold info for it
        hr = pProfile->CreateKey(pszCLSID, &dataKey);

        // Now we can set some default values
        if(SUCCEEDED(hr))
        {
            hr = dataKey->SetStringValue(L"GENDER", L"UNKNOWN");        
        }
        if(SUCCEEDED(hr))
        {
            hr = dataKey->SetStringValue(L"AGE", L"UNKNOWN");        
        }

        // Now we can create some temporary file storage (e.g. for trained models)
        if(SUCCEEDED(hr))
        {
            hr = pProfile->GetStorageFileName(CLSID_SampleSREngine, L"SampleEngTrainingFile", NULL, CSIDL_FLAG_CREATE | CSIDL_LOCAL_APPDATA, &pszPath);
        }

        // Now we request a UI for user training (or properties - SPDUI_RecoProfileProperties)
        // The engine cannot directly create UI it must request it.
        hr = AddEventString(SPEI_REQUEST_UI, 0, SPDUI_UserTraining);

    }
    else if(SUCCEEDED(hr))
    {
        // We've already seen this profile so read values
        WCHAR *pszGender = NULL, *pszAge = NULL;
        hr = dataKey->GetStringValue(L"GENDER", &pszGender);        
        if(SUCCEEDED(hr))
        {
            hr = dataKey->GetStringValue(L"AGE", &pszAge);        
        }

        // Now we could read training file
        if(SUCCEEDED(hr))
        {
            hr = pProfile->GetStorageFileName(CLSID_SampleSREngine, L"SampleEngTrainingFile", NULL, 0, &pszPath);
        }

        ::CoTaskMemFree(pszGender);
        ::CoTaskMemFree(pszAge);
    }

    ::CoTaskMemFree(pszPath);
    ::CoTaskMemFree(pszCLSID);
    return hr;
}


/****************************************************************************
* CSrEngine::OnCreateRecoContext *
*---------------------------*
*   Description:
*       This method is called each time a new reco context is created in 
*       an application using this engine.
*       This sample engine does not strictly need info about reco contexts
*       but for reference we will a keep list of them.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::OnCreateRecoContext(SPRECOCONTEXTHANDLE hSapiContext, void ** ppvDrvCtxt)
{
    SPDBG_FUNC("Sample Engine - CREATERECOCONTEXT");

    CContext * pContext = new CContext(hSapiContext);

    // Store a reference to the CContext structure
    *ppvDrvCtxt = pContext;
    m_ContextList.InsertHead(pContext);
    
    return S_OK;
}

/****************************************************************************
* CSrEngine::OnDeleteRecoContext *
*---------------------------*
*   Description:
*       This method is called each time a reco context is deleted.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::OnDeleteRecoContext(void * pvDrvCtxt)
{
    SPDBG_FUNC("Sample Engine - DELETERECOCONTEXT");

    CContext * pContext = (CContext *) pvDrvCtxt;
    m_ContextList.Remove(pContext);
    delete pContext;

    return S_OK;
}

/****************************************************************************
* CSrEngine::OnCreateGrammar *
*---------------------------*
*   Description:
*       This method is called each time a new reco grammar is created in 
*       an application using this engine.
*       We keep a list of grammars - storing a pointer to the list entry in ppvEngineGrammar.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::OnCreateGrammar(void * pvEngineRecoContext, SPGRAMMARHANDLE hSapiGrammar, void ** ppvEngineGrammar)
{
    SPDBG_FUNC("Sample Engine - ONCREATEGRAMMAR");

    // Each grammar will be associated with a context
    CContext * pContext = (CContext *) pvEngineRecoContext;
    SPDBG_ASSERT(m_ContextList.Find(pContext->m_hSapiContext));

    // Keep a list of grammars
    CDrvGrammar * pGrammar = new CDrvGrammar(hSapiGrammar);
    // Store a reference to the CDrvGrammar structure
    *ppvEngineGrammar = pGrammar;
    m_GrammarList.InsertHead(pGrammar);
    
    return S_OK;
}


/****************************************************************************
* CSrEngine::OnDeleteGrammar *
*---------------------------*
*   Description:
*       This method is called each time a reco grammar is deleted.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::OnDeleteGrammar(void * pvDrvGrammar)
{
    SPDBG_FUNC("Sample Engine - ONDELETEGRAMMAR");

    CDrvGrammar * pGrammar = (CDrvGrammar *)pvDrvGrammar;
    m_GrammarList.Remove(pGrammar);
    delete pGrammar;

    return S_OK;
}


/****************************************************************************
* CSrEngine::WordNotify *
*---------------------------*
*   Description:
*       This method is called by SAPI to inform the engine of the words in
*       command & control (C&C) grammars. When words are added or removed (e.g. by
*       the application loading or unloading grammars) this method is called.
*       Here we examine the word text, see if it has an associated pronunciation,
*       and see if there is a pronunciation in the lexicon.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::WordNotify(SPCFGNOTIFY Action, ULONG cWords, const SPWORDENTRY * pWords)
{
    SPDBG_FUNC("Sample Engine - WORDNOTIFY");
    HRESULT hr = S_OK;
    ULONG i;
    WCHAR *wordPron;

    switch(Action){
    case SPCFGN_ADD:
        SPWORDENTRY WordEntry;
        for(i = 0; SUCCEEDED(hr) && i < cWords; i++)
        {
            WordEntry = pWords[i];
            hr = m_cpSite->GetWordInfo(&WordEntry, SPWIO_WANT_TEXT);
            if(SUCCEEDED(hr) && WordEntry.aPhoneId)
            {
                // Word entry contains a specific pronounciation from CFG
                // Engine should use this in recognition
                wordPron = new WCHAR[wcslen(WordEntry.aPhoneId)];
                wcscpy(wordPron, WordEntry.aPhoneId);
                ::CoTaskMemFree((void*)WordEntry.aPhoneId);
            }
            else
            {
                // See if word is in lexicon
                SPWORDPRONUNCIATIONLIST PronList;
                PronList.pFirstWordPronunciation = 0;
                PronList.pvBuffer = 0;
                PronList.ulSize = 0;
                hr = m_cpLexicon->GetPronunciations(WordEntry.pszLexicalForm, eLEXTYPE_APP | eLEXTYPE_USER, pWords[i].LangID, &PronList);
                if(SUCCEEDED(hr))
                {
                    // Pronounciation(s) found in SAPI lexicon
                    // Engines should use these prons in recognition and also language and POS info
                    // For sample just copy first pron
                    wordPron = new WCHAR[wcslen(PronList.pFirstWordPronunciation->szPronunciation)];
                    wcscpy(wordPron, PronList.pFirstWordPronunciation->szPronunciation);
                    ::CoTaskMemFree(PronList.pvBuffer);
                }
                else if(hr == SPERR_NOT_IN_LEX)
                {
                    // No pronunciation present in SAPI lexicon.
                    // Engine should generate its own pronounciation or fail
                    // Here we generate a default NULL pronunciation
                    wordPron = NULL;
                    hr = S_OK;
                }
                else
                {
                    break; // Unexpected error - break;
                }

                // Associate the pronunciation information with the SAPI word handle so it can be recovered later
                // An engine can store any arbitrary pointer with each word.
                hr = m_cpSite->SetWordClientContext(WordEntry.hWord, wordPron);
            }

            if (SUCCEEDED(hr))
            {
                // When calling GetWordInfo SAPI allocates the strings which we must free
                ::CoTaskMemFree((void*)WordEntry.pszDisplayText);
                ::CoTaskMemFree((void*)WordEntry.pszLexicalForm);
            }
        }
        break;
    case SPCFGN_REMOVE:
        for(i = 0; i < cWords; i++)
        {
            WordEntry = pWords[i];
            // Client context already on word entry
            wordPron = (WCHAR *) WordEntry.pvClientContext;
            if(wordPron)
            {
                delete wordPron;
            }
        }
        break;
    }

    return hr;
}

/****************************************************************************
* CSrEngine::RuleNotify *
*---------------------------*
*   Description:
*       This method is called by SAPI to inform the engine of the rules in
*       command & control grammars. The order or actions of CFG grammars is of this form:
*       WordNotify(SPCFGN_ADD) - add words
*       RuleNotify(SPCFGN_ADD) - add rules
*       RuleNotify(SPCFGN_ACTIVATE) - activate rules to indicate they are to be used for recognition
*       RuleNotify(SPCFGN_INVALIDATE) - if a rule gets edited by the app then this is called
*       RuleNotify(SPCFGN_DEACTIVE) - deactivate rules
*       RuleNotify(SPCFGN_REMOVE) - remove rules
*       WordNotify(SPCFGN_REMOVE) - remove words
*
*       The engine can call GetRuleInfo to find the initial state in the rule, and
*       then GetStateInfo to find the information about subsequent states and transitions in the rule.
*       If a rule is edited then SPCFGN_INVALIDATE is called to indicate rule has changed so the engine 
*       must reparse the rule information.
*
*       The engine can obtain all the information about the rule either before or during recognition.
*       In this sample engine we just keep a list of rules initially and then wait 
*       until we want to generate a result and then find a random path through the rule.
*
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::RuleNotify(SPCFGNOTIFY Action, ULONG cRules, const SPRULEENTRY * pRules)
{
    SPDBG_FUNC("Sample Engine - RULENOTIFY");
    ULONG i;
    CRuleEntry *pRuleEntry;

    switch (Action)
    {
    case SPCFGN_ADD:
        for (i = 0; i < cRules; i++)
        {
            // Obtain information on the rule and store in a CRuleEntry structure
            pRuleEntry = new CRuleEntry;
            pRuleEntry->m_hRule = pRules[i].hRule;
            pRuleEntry->m_fTopLevel = (pRules[i].Attributes & SPRAF_TopLevel);
            pRuleEntry->m_fActive = (pRules[i].Attributes & SPRAF_Active);

            // Keep a list of rules
            m_RuleList.InsertHead(pRuleEntry);

            // Engine can store information associated with rule handle if desired
            m_cpSite->SetRuleClientContext(pRules[i].hRule, (void *)pRuleEntry);
        }
        break;
    case SPCFGN_REMOVE:
        for (i = 0; i < cRules; i++)
        {
            pRuleEntry = m_RuleList.Find(pRules[i].hRule);
            SPDBG_ASSERT(pRuleEntry); // The rule must have been added before being removed
            m_RuleList.Remove(pRuleEntry);
            delete pRuleEntry;
        }
        break;
    case SPCFGN_ACTIVATE:
        for (i = 0; i < cRules; i++)
        {
            pRuleEntry = m_RuleList.Find(pRules[i].hRule);
            // Only top-level rules can be activated
            SPDBG_ASSERT(pRuleEntry && !pRuleEntry->m_fActive && pRuleEntry->m_fTopLevel);
            pRuleEntry->m_fActive = TRUE;
        }
        break;
    case SPCFGN_DEACTIVATE:
        for (i = 0; i < cRules; i++)
        {
            pRuleEntry = m_RuleList.Find(pRules[i].hRule);
            SPDBG_ASSERT(pRuleEntry && pRuleEntry->m_fActive && pRuleEntry->m_fTopLevel);
            pRuleEntry->m_fActive = FALSE;
        }
        break;
    case SPCFGN_INVALIDATE:
        for (i = 0; i < cRules; i++)
        {
            pRuleEntry = m_RuleList.Find(pRules[i].hRule);
            SPDBG_ASSERT(pRuleEntry);
            pRuleEntry->m_fTopLevel = (pRules[i].Attributes & SPRAF_TopLevel);
            pRuleEntry->m_fActive = (pRules[i].Attributes & SPRAF_Active);
            // Don't need to do anything here as we don't start parsing the rule until recognition time
        }
        break;
    }

    return S_OK;
}

/****************************************************************************
* CSrEngine::LoadSLM *
*---------------------------*
*   Description:
*       Called when SAPI wants the engine to load a dictaion language model (SLM).
*       For each reco gramar one dictation as well as C&C rules can be loaded.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::LoadSLM(void * pvEngineGrammar, const WCHAR * pszTopicName)
{
    SPDBG_FUNC("Sample Engine - LOADSLM");

    if (pszTopicName)
    {
        // Engines should load the named language model - e.g. Spelling using
        //  this parameter. If NULL load the default dictation model.
        // Since just a sample ignore this parameter.
    }
    
    // pvEngineGrammar is the pointer ppvEngineGrammar we set in OnCreateGrammar
    // Use this to find out on which grammar the SLM is being asked for.
    CDrvGrammar * pGrammar = (CDrvGrammar *)pvEngineGrammar;
    pGrammar->m_SLMLoaded = TRUE;

    return S_OK;
}

/****************************************************************************
* CSrEngine::UnloadSLM *
*---------------------------*
*   Description:
*       Called when SAPI wants the engine to delete an SLM.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::UnloadSLM(void *pvEngineGrammar)
{
    SPDBG_FUNC("Sample Engine - UNLOADSLM");

    CDrvGrammar * pGrammar = (CDrvGrammar *)pvEngineGrammar;
    pGrammar->m_SLMLoaded = FALSE;

    return S_OK;
}

/****************************************************************************
* CSrEngine::SetSLMState *
*---------------------------*
*   Description:
*       Called to activate or deactivate an SLM for recognition.
*       NewState is either SPRS_ACTIVE or SPRS_INACTIVE.
*   Return: 
*       S_OK
*****************************************************************************/   
HRESULT CSrEngine::SetSLMState(void * pvDrvGrammar, SPRULESTATE NewState)
{
    SPDBG_FUNC("Sample Engine - SETSLMSTATE");

    // pvDrvGrammar is the pointer ppvEngineGrammar we set in OnCreateGrammar
    CDrvGrammar * pGrammar = (CDrvGrammar *)pvDrvGrammar;
    if (NewState != SPRS_INACTIVE)
    {
        pGrammar->m_SLMActive = TRUE;
    }
    else
    {
        pGrammar->m_SLMActive = FALSE;
    }
    
    return S_OK;
}

/****************************************************************************
* CSrEngine::SetWordSequenceData *
*---------------------------*
*   Description:
*       If the app submits a text buffer to SAPI this method is called.
*       The text buffer supplied here can either be used in CFGs with the text buffer transition,
*       or in dictation to supply information to the engine about the prior text visible on screen.
*       This sample engine is just using the text buffer with the text buffer transition.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetWordSequenceData(void *pvEngineGrammar, const WCHAR *pText, ULONG cchText, const SPTEXTSELECTIONINFO *pInfo)
{
    SPDBG_FUNC("Sample Engine - SETWORDSEQUENCE");

    //For each grammar object going to be released, SAPI would call SetWordSequenceData(NULL, 0, NULL).

    // Recover the data we have associated with this grammar.
    CDrvGrammar * pGrammar = (CDrvGrammar*)pvEngineGrammar;

    // Delete previous grammar text buffer 
    if(pGrammar->m_pWordSequenceText)
    {
        delete pGrammar->m_pWordSequenceText;
    }

    // Make a copy of the text data
    if(cchText)
    {
        pGrammar->m_pWordSequenceText = new WCHAR[cchText];
        memcpy((void *)pGrammar->m_pWordSequenceText, pText, sizeof(WCHAR) * cchText);
        pGrammar->m_cchText = cchText;
    }
    else
    {
        pGrammar->m_pWordSequenceText = NULL;
        pGrammar->m_cchText = NULL;
    }

    // Engines can use the SPTEXTSELECTIONINFO to determine which
    // parts of the text buffer are visible on screen and / or selected.
    // This sample engine is using the active text selection only.
    SetTextSelection(pvEngineGrammar, pInfo);

    
    return S_OK;
}

/****************************************************************************
* CSrEngine::SetTextSelection *
*---------------------------*
*   Description:
*       This method tells engines if the SPTEXTSELECTIONINFO structure
*       has been updated. This sample engine is using only fields ulStartActiveOffset and cchActiveChars of SPTEXTSELECTIONINFO.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetTextSelection(void * pvEngineGrammar, const SPTEXTSELECTIONINFO * pInfo)
{
    SPDBG_FUNC("Sample Engine - SETTEXTSELECTION");

    // Recover the data we have associated with this grammar.
    CDrvGrammar * pGrammar = (CDrvGrammar*)pvEngineGrammar;
    
    if (pGrammar->m_pInfo)
    {
        delete pGrammar->m_pInfo;
    }

    if (pInfo)
    {
        pGrammar->m_pInfo = new SPTEXTSELECTIONINFO(*pInfo);    
    }
    else
    {
        pGrammar->m_pInfo = NULL;
    }

    return S_OK;
}

/****************************************************************************
* CSrEngine::IsPronounceable *
*---------------------------*
*   Description:
*       Engines should return whether it has or will be able to 
*       generate a pronounciation for this word.
*       In this sample engine, this is always true.
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::IsPronounceable(void * pDrvGrammar, const WCHAR * pszWord, SPWORDPRONOUNCEABLE * pWordPronounceable)
{
    SPDBG_FUNC("Sample Engine - ISPRONOUNCEABLE");
    *pWordPronounceable = SPWP_KNOWN_WORD_PRONOUNCEABLE;
    return S_OK;
}

/****************************************************************************
* CSrEngine::SetAdaptationData *
*---------------------------*
*   Description:
*       This method can be used by the app to give text data to the engine
*       for language model adaptation etc. This method can only be called
*       by the app after if has received an SPEI_ADAPTATION event. Since
*       we never fire that event this method should never be called.

*   Return:
*       E_UNEXPECTED 
*****************************************************************************/   
STDMETHODIMP CSrEngine::SetAdaptationData(void * pvEngineCtxtCookie, const WCHAR *pAdaptationData, const ULONG cch)
{
    SPDBG_FUNC("Sample Engine - SETADAPTATIONDATA");

    SPDBG_ASSERT(0); // This method should never be called
    return E_UNEXPECTED;
}

/****************************************************************************
* CSrEngine::AddEvent *
*---------------------------*
*   Description:
*       Internal helper method to send an event to SAPI.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
HRESULT CSrEngine::AddEvent(SPEVENTENUM eEventId, ULONGLONG ullStreamPos, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("Sample Engine - AddEvent");
    HRESULT hr = S_OK;

    SPEVENT Event;
    Event.eEventId = eEventId;
    Event.elParamType = SPET_LPARAM_IS_UNDEFINED;
    Event.ulStreamNum = 0; // Always set this to zero - SAPI fills this in
    Event.ullAudioStreamOffset = ullStreamPos;
    Event.wParam = wParam;
    Event.lParam = lParam;

    hr = m_cpSite->AddEvent(&Event, NULL);

    return hr;
}

/****************************************************************************
* CSrEngine::AddEventString *
*---------------------------*
*   Description:
*       Internal helper method to send an event with a string LParam to SAPI.
*       Request UI is the only event in the sample to need this.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
HRESULT CSrEngine::AddEventString(SPEVENTENUM eEventId, ULONGLONG ullStreamPos, const WCHAR * psz, WPARAM wParam)
{
    SPDBG_FUNC("Sample Engine - AddEventString");
    HRESULT hr = S_OK;

    SPEVENT Event;
    Event.eEventId = eEventId;
    Event.elParamType = SPET_LPARAM_IS_STRING;
    Event.ulStreamNum = 0; // Always set this to zero - SAPI fills this in
    Event.ullAudioStreamOffset = ullStreamPos;
    Event.wParam = wParam;
    Event.lParam = (LPARAM)psz;

    hr = m_cpSite->AddEvent(&Event, NULL);

    return hr;
}


#define BLOCKSIZE 220       // 1/100 of a second

/****************************************************************************
* CSrEngine::RecognizeStream *
*---------------------------*
*   Description:
*       This is the method that SAPI calls for recognition to take place.
*       Engines must only return from this method after they have read all the data
*       and completed all the recognition they are going to do on this stream.
*       Thus this method is giving a thread to the engine to do recognition on,
*       and engines may create additional threads.
*
*       In this sample we constantly read data using this thread, and then perform
*       very basic speech detection and pass data to a recognizer thread, which
*       generates hypotheses and results.
*
*       Parameters:
*
*        - REFGUID rguidFormatId - this is the GUID of the input audio format
*        - const WAVEFORMATEX * pWaveFormatEx - this is the extended wav format information of the audio format
*        - HANDLE hRequestSync - this Win32 event is used to indicate that there are pending tasks
* and the engine should call Synchronize() for SAPI to process these.
*        - HANDLE hDataAvailable - this Win32 event is used to tell the engine that data is available to be read.    
* The frequency this is set can be controlled by the SetBufferNotifySize method.
*        - HANDLE hExit - this Win32 event indicates the engine is being closed down and should exit immediately.
*        - BOOL fNewAudioStream - this indicates this is a new input stream
* e.g. the app has done a new SetInput call rather than just restarting the previous stream.
*        - BOOL fRealTimeAudio - this indicates the input is from a real-time ISpAudio stream, rather than, say, a file
*        - ISpObjectToken * pAudioObjectToken - this is the object token representing the audio input device
* the engine may want to query this.
*
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::RecognizeStream(REFGUID rguidFormatId,
                                        const WAVEFORMATEX * pWaveFormatEx,
                                        HANDLE hRequestSync,
                                        HANDLE hDataAvailable,
                                        HANDLE hExit,
                                        BOOL fNewAudioStream,
                                        BOOL fRealTimeAudio,
                                        ISpObjectToken * pAudioObjectToken)
{
    SPDBG_FUNC("Sample Engine - RECOGNIZESTREAM");
    HRESULT hr = S_OK;
    
    m_hRequestSync = hRequestSync;

    // Start the recognition thread
    hr = m_cpDecoderThread->StartThread(0, NULL);

    if (SUCCEEDED(hr))
    {
        const HANDLE aWait[] = { hExit, m_hQueueHasRoom };

        while (TRUE) // sit in this loop until there is no more data
        {
            // The Read method is used to read data. This will block until the required 
            // amount of data is available. If the stream has ended either a fail code
            // will be returned or the amount read will be less than the amount asked for.
            // To see how much data is available to be read without blocking the DataAvailable
            // method can be used or hDataAvailable event.
            BYTE aData[BLOCKSIZE];
            ULONG cbRead;
            hr = m_cpSite->Read(aData, sizeof(aData), &cbRead);
            if (hr != S_OK || cbRead < sizeof(aData))
            {
                break;
            }

            // Decide if the frame of data is noise or silence with a simple level detector
            BOOL bNoiseDetected = FALSE;
            SHORT * pBuffer = (SHORT *)aData;
            for (ULONG i = 0; i < cbRead; i += 2, pBuffer++)
            {
                if (*pBuffer < (SHORT)-3000 || *pBuffer > (SHORT)3000)
                {
                    bNoiseDetected = TRUE;
                    break;
                }
            }

            // If there no space on the frame queue then wait
            BOOL bBlock = m_FrameQueue.IsFull();
            if(bBlock)
            {
                // Wait for space to appear on the queue to be passed to the recognizer.
                // Real engines should not wait - the data reading should be as real-time as possible
                // Also detect if the hExit event is set to indicate we should stop processing.
                if (::WaitForMultipleObjects(sp_countof(aWait), aWait, FALSE, INFINITE) == WAIT_OBJECT_0)
                {
                    break;
                }
            }

            // Add the frame to the queue and notify the decoder thread
            m_FrameQueue.InsertTail(bNoiseDetected);
            m_cpDecoderThread->Notify();

        }

        // Once we've stopped reading data we must wait for the recognizer thread to finish
        // All processing must be done before returning from the RecognizeStream method.
        m_cpDecoderThread->WaitForThreadDone(TRUE, &hr, 30 * 1000);
    }

    m_hRequestSync = NULL;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSrEngine::ThreadProc *
*-----------------------*
*   Description:
*       This is the main thread for the recognition process.
*       This hExitThreadEvent indicates this thread should complete, 
*       and the hNotifyEvent indicates the RecognizeStream thread is notifying
*       that data is available for processing. The m_hRequestSync indicates
*       SAPI is requesting the engine should call Synchronize.
*
*   Return:
*       S_OK
*****************************************************************************/
STDMETHODIMP CSrEngine::ThreadProc(void *, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwndWorker, volatile const BOOL * pfContinueProcessing)
{
    SPDBG_FUNC("Sample Engine - Recognition ThreadProc");
    HRESULT hr = S_OK;
    const HANDLE aWait[] = { hExitThreadEvent, hNotifyEvent, m_hRequestSync };
    ULONG block = 0;
    ULONG silenceafternoise = 0;
    DWORD waitres;

    m_bSoundStarted = FALSE;
    m_bPhraseStarted = FALSE;
    m_cBlahBlah = 0;

    m_ullStart = 0;
    m_ullEnd = 0;
    while (*pfContinueProcessing) // sit in this loop until exit
    {
        ULONG cEvents = sp_countof(aWait);
        // Respond to the request sync event only if speech not detected,
        // as we don't want to allow grammar changes during recognition.
        // However always respond after several seconds.
        if(m_bPhraseStarted && (block - m_ullStart / BLOCKSIZE) < 5 * 100)
        {
            --cEvents;
        }
        waitres = ::WaitForMultipleObjects(cEvents, aWait, FALSE, INFINITE);
        switch (waitres)
        {

        case WAIT_OBJECT_0:     // Exit thread
            break;

        case WAIT_OBJECT_0 + 1: // Notify (data is available)

            // Engines should regularly call UpdateRecoPos to indicate how far through the
            // stream they have recognized.
            m_cpSite->UpdateRecoPos((ULONGLONG)block * BLOCKSIZE);

            if (m_ullStart == 0 && !m_bPhraseStarted)
            {
                // Engines should also call Synchronize to indicate they are ready
                // to be notified about grammar changes and other tasks. Within Synchronize 
                // if grammars have changed WordNotify, RuleNotify etc. will be called before Synchronize returns.
                // Here Synchronize is called only if processing silence, not speech
                // to avoid having to deal with grammar changes during recognition.

                m_cpSite->Synchronize((ULONGLONG)block * BLOCKSIZE);
                // A return code of S_FALSE from synchronize means the engine can stop recognizing
                // This engine ignores this.
            }
            while (m_FrameQueue.HasData())
            {
                BOOL bNoise = m_FrameQueue.RemoveHead();
                block++; // Update the position in stream
                if (bNoise)
                {
                    // Found some speech - update start and end positions
                    silenceafternoise = 0;
                    if (m_ullStart == 0)
                    {
                        m_ullStart = (ULONGLONG)block * BLOCKSIZE;
                    }
                    m_ullEnd = (ULONGLONG)block * BLOCKSIZE;
                    _CheckRecognition(); // this will generate hypotheses and events
                }
                else
                {
                    // Found some silence
                    silenceafternoise++;
                    if (silenceafternoise > 50)
                    {
                        if (m_bSoundStarted)
                        {
                            // We've heard 1/2 sec of silence since the last noise, so send the
                            // final recognition if we had previously started a phrase
                            if (m_bPhraseStarted)
                            {
                                _NotifyRecognition(FALSE, m_cBlahBlah);
                            }
                            AddEvent(SPEI_SOUND_END, m_ullEnd); // send the sound end event
                            m_bSoundStarted = FALSE;
                            m_bPhraseStarted = FALSE;
                        }
                        m_ullStart = 0;
                        m_ullEnd = 0;
                    }
                }

                //--- Generate random interference start at 20 seconds into the stream
                //    and saying it's gone at 22 seconds
                if (block % (100 * 30) == 20 * 100)
                {
                    const SPINTERFERENCE rgspi[] = 
                    { SPINTERFERENCE_NOISE, SPINTERFERENCE_NOSIGNAL, SPINTERFERENCE_TOOLOUD, SPINTERFERENCE_TOOQUIET };

                    AddEvent(SPEI_INTERFERENCE, block*BLOCKSIZE, 0, rgspi[rand() % 4]);
                }
                else if (block % (100 * 30) == 22 * 100)
                {
                    AddEvent(SPEI_INTERFERENCE, block*BLOCKSIZE, 0, SPINTERFERENCE_NONE);
                }
                // Ask for UI at 10 seconds into the stream
                //  and cancel request one second later
                else if (block == 10 * 100) 
                {
                    AddEventString(SPEI_REQUEST_UI, block * BLOCKSIZE, SPDUI_UserTraining);
                } 
                else if (block == 11 * 100) // Cancle the UI request at 11 seconds into the stream
                {
                    AddEventString(SPEI_REQUEST_UI, block * BLOCKSIZE, NULL);
                }
            }
            break;

        case WAIT_OBJECT_0 + 2: 
            // SAPI has explicitly requested we call Synchronize
            hr = m_cpSite->Synchronize((ULONGLONG)block * BLOCKSIZE);
            // Once synchronize is called the engine cannot fire events prior to that stream position
            // so update the stored stream start position
            m_ullStart = block * BLOCKSIZE;
            if(m_ullEnd < m_ullStart)
            {
                m_ullEnd = m_ullStart;
            }
            break;

        default:
            SPDBG_ASSERT(FALSE);
            // Something strange!
            break;
        }
    }

    // Before exiting we must make sure every phrase start has been paired with a recognition,
    // and that every sound start event has a corresponding sound end.
    if (m_bPhraseStarted)
    {
        _NotifyRecognition(FALSE, m_cBlahBlah);
    }
    if (m_bSoundStarted)
    {
        AddEvent(SPEI_SOUND_END, m_ullEnd);
    }

    return S_OK;
}


/****************************************************************************
* CSrEngine::_CheckRecognition *
*---------------------------*
*   Description:
*       This internal method decides when to fire sound start events, 
*       phrase start events and hypotheses.
*****************************************************************************/   
void CSrEngine::_CheckRecognition()
{
    SPDBG_FUNC("Sample Engine - _NotifyRecognition");
    ULONG duration, blahs;

    if (m_ullEnd > m_ullStart)
    {
        duration = (ULONG)(m_ullEnd - m_ullStart);
        if (duration >= BLOCKSIZE * 100 * 1 / 8)
        {
            if (!m_bSoundStarted)
            {
                // Heard something that was longer than 1/8th second, so do sound start
                AddEvent(SPEI_SOUND_START, m_ullStart);
                m_bSoundStarted = TRUE;
                m_cBlahBlah = 0;
            }
            if (duration >= BLOCKSIZE * 100 * 1 / 4)
            {
                // Heard something that was longer than 1/4 second, so do phrase start
                // and then generate a hypothesis every 1/4 second after that.
                blahs = duration / (BLOCKSIZE * 100 * 1 / 4);
                if (blahs != m_cBlahBlah)
                {
                    m_cBlahBlah = blahs;
                    if (!m_bPhraseStarted)
                    {
                        m_bPhraseStarted = TRUE;
                        AddEvent(SPEI_PHRASE_START, m_ullStart);
                    }
                    _NotifyRecognition(TRUE, blahs);
                }
            }
        }
    }
}


/****************************************************************************
* CSrEngine::_NotifyRecognition *
*-------------------------------*
*   Description:
*       This internal method is used to generate a recognition result. The contents
*   are random since we don't actually know how to recognize anything.*
****************************************************************************/
void CSrEngine::_NotifyRecognition( BOOL fHypothesis, ULONG nWords )
{
    SPDBG_FUNC("Sample Engine - _NotifyRecognition");
    HRESULT hr = S_OK;

    // First count the active CFG rules
    ULONG cActiveCFGRules = 0;
    CRuleEntry * pRule = m_RuleList.GetHead();        
    for(; pRule; pRule = m_RuleList.GetNext(pRule))
    {
        if( pRule->m_fActive )
        {
            cActiveCFGRules++;
        }
    }

    // Then count all the grammars with active dictation
    ULONG cActiveSLM = 0;
    CDrvGrammar * pGram = m_GrammarList.GetHead();        
    for(; pGram; pGram = m_GrammarList.GetNext(pGram))
    {
        if(pGram->m_SLMActive)
        {
            cActiveSLM++;
        }
    }

    // If both CFG and dictation are active, randomly do one or the other
    if(cActiveCFGRules && cActiveSLM)
    {
        if(rand() % 2)
        {
            cActiveSLM = 0;
        }
        else
        {
            cActiveCFGRules = 0;
        }
    }

    //--- Compose reco result info
    SPRECORESULTINFO Result;
    memset(&Result, 0, sizeof(SPRECORESULTINFO));
    Result.cbSize               = sizeof(SPRECORESULTINFO);
    Result.fHypothesis           = fHypothesis;
    Result.ullStreamPosStart     = m_ullStart;
    Result.ullStreamPosEnd       = m_ullEnd;

    if( cActiveCFGRules )
    {
        // Generate a CFG result phrase
        hr = WalkCFGRule(&Result, cActiveCFGRules, fHypothesis, nWords, m_ullStart, (ULONG)(m_ullEnd - m_ullStart));
        if( SUCCEEDED(hr) )
        {
            // Pass the results info to SAPI
            hr = m_cpSite->Recognition(&Result);
            // Cleanup any memory allocated for alternates and release phrases
            for(ULONG i = 0; i < Result.ulNumAlts; i++)
            {
                Result.aPhraseAlts[i].pPhrase->Release();
            }
            Result.pPhrase->Release();
            delete[] Result.aPhraseAlts;
        }
    }
    else if(cActiveSLM)
    {
        // Generate a dictation result phrase
        hr = WalkSLM(&Result, cActiveSLM, nWords, m_ullStart, (ULONG)(m_ullEnd - m_ullStart));
        if( SUCCEEDED(hr) )
        {
            // Pass the results info to SAPI
            hr = m_cpSite->Recognition(&Result);
            // Release the result phrase
            Result.pPhrase->Release();
            delete[] Result.pvEngineData;
        }
	}
    else if(!fHypothesis)
    {
        // No rules were active - return a false recognition
        // RecognizeStream can still be called if no rules are active
        // - the engine is free to do anything it wants with the speech data.
        Result.eResultType = SPRT_FALSE_RECOGNITION;
        hr = m_cpSite->Recognition(&Result);
    }

}

/****************************************************************************
* CSrEngine::CreatePhraseFromRule *
*---------------------------------*
*   Description:
*       This method is used to produce the result phrase for a CFG result.
*       It selects a random path through an active rule, and
*       then calls ParseFromTransition to generate an ISpPhraseBuilder object.
*
*   Return:
*       S_OK
*       FAIL(hr)
****************************************************************************/
HRESULT CSrEngine::CreatePhraseFromRule( CRuleEntry * pRule, BOOL fHypothesis,
                                         ULONGLONG ullAudioPos, ULONG ulAudioSize,
                                         ISpPhraseBuilder** ppPhrase )
{
    HRESULT hr = S_OK;
    SPRULEENTRY RuleInfo;
    RuleInfo.hRule = pRule->m_hRule;
    hr = m_cpSite->GetRuleInfo(&RuleInfo, SPRIO_NONE);
    if( SUCCEEDED(hr) )
    {
        SPPATHENTRY Path[200];        // Note: Limit of 200 transitions in grammar!
        ULONG cTrans;
        // Recursively generate random path
        hr = RecurseWalk(RuleInfo.hInitialState, Path, &cTrans);

        //Fill in the audio offset and audio size for each element in the path, while each element has equal size and silence in between
        if (cTrans)
        {
            ULONG ulInterval = ulAudioSize/cTrans;
            for (ULONG ul = 0; ul < cTrans; ul++)
            {
                Path[ul].elem.ulAudioStreamOffset = ul * ulInterval;
                Path[ul].elem.ulAudioSizeBytes = ulInterval/2;
            }
        }

        if (SUCCEEDED(hr))
        {
            // generate a SPPARSEINFO structure
            SPPARSEINFO ParseInfo;
            memset(&ParseInfo, 0, sizeof(ParseInfo));
            ParseInfo.cbSize = sizeof(SPPARSEINFO);
            ParseInfo.hRule = pRule->m_hRule;
            ParseInfo.ullAudioStreamPosition = ullAudioPos;
            ParseInfo.ulAudioSize = ulAudioSize;
            ParseInfo.cTransitions = cTrans;
            ParseInfo.pPath = Path;
            ParseInfo.fHypothesis = fHypothesis;
            ParseInfo.SREngineID = CLSID_SampleSREngine;
            ParseInfo.ulSREnginePrivateDataSize = 0;
            ParseInfo.pSREnginePrivateData = NULL;

            // Generate a phrase object from the parse info.
            hr = m_cpSite->ParseFromTransitions(&ParseInfo, ppPhrase );
            if(SUCCEEDED(hr))
            {
                // delete any allocated memory
                for(ULONG i = 0; i < cTrans; i++)
                {
                    if(Path[i].elem.pszDisplayText)
                    {
                        delete const_cast<WCHAR*>(Path[i].elem.pszDisplayText);
                    }
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CSrEngine::FindRule *
*---------------------*
*   Description:
*       This method is used to locate an active rule in the rule list by index
*
****************************************************************************/
CRuleEntry* CSrEngine::FindRule( ULONG ulRuleIndex )
{
    CRuleEntry * pRule = m_RuleList.GetHead();
    ULONG ulRule = 0;
    while( pRule )
    {
        if( pRule->m_fActive && ( ulRule++ == ulRuleIndex ) )
        {
            break;
        }
        pRule = m_RuleList.GetNext( pRule );
    }
    SPDBG_ASSERT(pRule && pRule->m_fActive);
    return pRule;
}

/****************************************************************************
* CSrEngine::NextRuleAlt *
*------------------------*
*   Description:
*       This method is used to locate a rule alternate in the rule list
*
****************************************************************************/
CRuleEntry* CSrEngine::NextRuleAlt( CRuleEntry * pPriRule, CRuleEntry * pLastRule )
{
    CRuleEntry * pRule = (pLastRule)?(pLastRule):(m_RuleList.GetHead());
    for(; pRule; pRule = m_RuleList.GetNext(pRule))
    {
        if( pRule->m_fActive &&
            ( m_cpSite->IsAlternate( pPriRule->m_hRule, pRule->m_hRule ) == S_OK ) )
        {
            break;
        }
    }
    return pRule;
}

/****************************************************************************
* CSrEngine::WalkCFGRule *
*------------------------*
*   Description:
*       This method is used to produce the results information for a CFG result.
*       It creates a result phrase and then some alternates phrases.
*   Return: 
*       S_OK
*       FAILED(hr)
****************************************************************************/
HRESULT CSrEngine::WalkCFGRule( SPRECORESULTINFO * pResult, ULONG cRulesActive, BOOL fHypothesis,
                                ULONG nWords, ULONGLONG ullAudioPos, ULONG ulAudioSize)
{
    HRESULT hr = E_FAIL;
    CRuleEntry * pPriRule = NULL;
    pResult->ulSizeEngineData = 0;
    pResult->pvEngineData     = NULL;
    pResult->eResultType      = SPRT_CFG;
    pResult->hGrammar         = NULL;

    while (hr == E_FAIL)
    {
        // Randomly pick a rule and locate it in the list
        pPriRule = FindRule( rand() % cRulesActive );

        // Create a phrase from the rule
        hr = CreatePhraseFromRule( pPriRule, fHypothesis, ullAudioPos,
                                   ulAudioSize, &pResult->pPhrase );
        // E_FAIL means the random path generated came to a dead end. Most likely an empty
        // dynamic rule. We are not allowed to recognize through empty rules.
        // Hence we choose another random toplevel rule and try again until we succeed.
    }

    if (hr != S_OK)
    {
        SPDBG_ASSERT(FALSE);
    }

    // Get the phrase info
    SPPHRASE* pPriPhraseInfo = NULL;
    if( SUCCEEDED( hr ) )
    {
        hr = pResult->pPhrase->GetPhrase( &pPriPhraseInfo );
    }

    // Ask the site how many alternates to generate
    ULONG ulNumAlts = 0;
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpSite->GetMaxAlternates( pPriRule->m_hRule, &ulNumAlts );
    }

    // Randomly create some alternates
    if( SUCCEEDED( hr ) && ulNumAlts )
    {
        pResult->aPhraseAlts = new SPPHRASEALT[ulNumAlts];
        if( pResult->aPhraseAlts )
        {
            memset( pResult->aPhraseAlts, 0, ulNumAlts * sizeof(SPPHRASEALT) );
            CRuleEntry * pAltRule = NULL;

            for( ULONG i = 0; SUCCEEDED( hr ) && (i < ulNumAlts); ++i )
            {
                // Try to find an alternate rule
                pAltRule = NextRuleAlt( pPriRule, pAltRule );
                if( !pAltRule )
                {
                    ulNumAlts = i;
                    break;
                }

                // Create an alternate phrase from the rule
                hr = CreatePhraseFromRule( pAltRule, fHypothesis, ullAudioPos,
                                           ulAudioSize, &pResult->aPhraseAlts[i].pPhrase );

                // Get the alternate phrase info
                SPPHRASE* pAltPhraseInfo = NULL;
                if( SUCCEEDED( hr ) )
                {
                    hr = pResult->aPhraseAlts[i].pPhrase->GetPhrase( &pAltPhraseInfo );
                }

                if( SUCCEEDED( hr ) )
                {
                    ++pResult->ulNumAlts;

                    // Fill out relationship info
                    pResult->aPhraseAlts[i].cElementsInParent = pPriPhraseInfo->Rule.ulCountOfElements;
                    pResult->aPhraseAlts[i].cElementsInAlternate = pAltPhraseInfo->Rule.ulCountOfElements;

                    // Point to some extra data for testing
                    static BYTE AltData[] = { 0xED, 0xED, 0xED, 0xED };
                    pResult->aPhraseAlts[i].pvAltExtra = &AltData;
                    pResult->aPhraseAlts[i].cbAltExtra = sp_countof( AltData );
                }

                if( pAltPhraseInfo )
                {
                    ::CoTaskMemFree( pAltPhraseInfo );
                }
            }
        }
        else
        {
            E_OUTOFMEMORY;
        }
    }

    // Cleanup main phrase information
    if( pPriPhraseInfo )
    {
        ::CoTaskMemFree( pPriPhraseInfo );
    }

    // Cleanup on failure
    if( FAILED( hr ) )
    {
        if( pResult->pPhrase )
        {
            pResult->pPhrase->Release();
			pResult->pPhrase = NULL;
        }

        for( ULONG i = 0; i < pResult->ulNumAlts; ++i )
        {
            pResult->aPhraseAlts[i].pPhrase->Release();
			pResult->aPhraseAlts[i].pPhrase = NULL;
        }
		pResult->ulNumAlts = 0;
        delete[] pResult->aPhraseAlts;
		pResult->aPhraseAlts = NULL;
    }

    return hr;
}

/****************************************************************************
* CSrEngine::WalkSLM *
*---------------------------*
*   Description:
*       This method is used to produce the results information for a dictation result.
*       It creates a result phrase, and then serializes some alternates information
*       that the CSrEngineAlternates object uses to generate alternates.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
HRESULT CSrEngine::WalkSLM(SPRECORESULTINFO * pResult, ULONG cSLMActive,
                           ULONG nWords, ULONGLONG ullAudioPos, ULONG ulAudioSize)
{
    HRESULT hr = S_OK;

    // If several dictation grammars are active pick one at random
    ULONG ulGramIndex = rand() % cSLMActive;
    CDrvGrammar * pGram = m_GrammarList.GetHead();        
    ULONG nGram = 0;
    for(; pGram; pGram = m_GrammarList.GetNext(pGram))
    {
        if(pGram->m_SLMActive)
        {
            if(nGram == ulGramIndex)
            {
                break;
            }
            nGram++;
        }
    }
    SPDBG_ASSERT(pGram && pGram->m_SLMActive);

    // create and fill SPPHRASE structure
    SPPHRASE phrase;
    memset(&phrase, 0, sizeof(SPPHRASE));
    phrase.cbSize = sizeof(SPPHRASE);
    phrase.LangID = m_LangID;
    phrase.ullAudioStreamPosition = ullAudioPos;
    phrase.ulAudioSizeBytes = ulAudioSize;
    phrase.SREngineID = CLSID_SampleSREngine;

    // allocate elements
    ULONG cb = nWords * sizeof(SPPHRASEELEMENT);
    SPPHRASEELEMENT* pElements = (SPPHRASEELEMENT *)_alloca(cb);
    memset(pElements, 0, cb);

    // fill in word info into elements
	for (ULONG n = 0; n < nWords; n++)
	{
        ULONG ulInterval = ulAudioSize/nWords;

        pElements[n].bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
		pElements[n].pszDisplayText =  DICT_WORD;
        pElements[n].ulAudioStreamOffset = n * ulInterval;
        pElements[n].ulAudioSizeBytes = ulInterval/2;
 
	}

    // add elements to phrase
    phrase.Rule.ulCountOfElements = nWords;
    phrase.pElements = pElements;

    // make phrase builder and add phrase info
    CComPtr<ISpPhraseBuilder> cpBuilder;
    hr = cpBuilder.CoCreateInstance(CLSID_SpPhraseBuilder);
    if (SUCCEEDED(hr))
    {
        hr = cpBuilder->InitFromPhrase(&phrase);
    }

    if (SUCCEEDED(hr))
    {
        // Store alternates string in extra engine data
        // We just serialize an equal number of copies of ALT_WORD as DICT_WORD.
        // Engines can store any information (e.g. serialized lattice) here.
        pResult->ulSizeEngineData = sizeof(ALT_WORD) * nWords;
        pResult->pvEngineData = new WCHAR[(sizeof(ALT_WORD) / sizeof(WCHAR )) * nWords];
        if (pResult->pvEngineData == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        WCHAR *pC = (WCHAR *)pResult->pvEngineData;
        for(ULONG i = 0; i < nWords; i++)
        {
            wcscpy(pC,ALT_WORD);
            pC += wcslen(ALT_WORD);
            *pC = L' ';
            pC++;
        }
        *(--pC) = '\0';

        pResult->eResultType = SPRT_SLM;
        pResult->hGrammar = pGram->m_hSapiGrammar;
    }

    if (SUCCEEDED(hr))
    {
        pResult->pPhrase = cpBuilder.Detach();
    }
    else
    {
        delete[] pResult->pvEngineData;
        pResult->pvEngineData = NULL;
        pResult->ulSizeEngineData = 0;
    }

    return hr;
}

/****************************************************************************
* CSrEngine::WalkTextBuffer *
*---------------------------*
*   Description:
*       This is called when RecurseWalk hits a text-buffer transition.
*       At such a transition the engine should try to recognize any sequence
*       of words from the active text selection the app has supplied.
*       Any word can be the start word and then recognition can continue to
*       the next \0 end of sentence marker or at the end of the active text selection.
*       In this sample we pick a random string of words from the buffer.
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
HRESULT CSrEngine::WalkTextBuffer(void* pvGrammarCookie, SPPATHENTRY * pPath, SPTRANSITIONID hId, ULONG * pcTrans)
{
    HRESULT hr = S_OK;
    // Check if text buffer present
    CDrvGrammar * pGrammar = (CDrvGrammar *) pvGrammarCookie;
    SPDBG_ASSERT(pGrammar->m_pWordSequenceText && pGrammar->m_cchText >= 2); //m_cchText counts the trailing "/0/0"

    *pcTrans = 0;

    // Count sentences
    ULONG nPhrase = 0;
    const WCHAR *cPhrase;
    
    ULONG ulStartActiveOffset = 0; //The default value with text selection
    ULONG cchActiveChars = pGrammar->m_cchText - 2; //The default value with text selection
    ULONG ccChars = 0;

    if (pGrammar->m_pInfo)
    {
        ulStartActiveOffset = pGrammar->m_pInfo->ulStartActiveOffset;
        cchActiveChars = pGrammar->m_pInfo->cchActiveChars;
    }

    for (cPhrase = pGrammar->m_pWordSequenceText + ulStartActiveOffset;
            ccChars < cchActiveChars && (*cPhrase != L'\0' || *(cPhrase + 1) != '\0');
            ccChars++, cPhrase++)
    {
        if(*cPhrase != L'\0' && *(cPhrase + 1) == L'\0')
        {
            nPhrase++;
        }
    }


    SPDBG_ASSERT(nPhrase);

    // Randomly pick a sentence between the first active sentence and the last active sentence
    nPhrase = rand() % nPhrase; //nPhrase would be 0 index
    ULONG nP = 0;
    for(cPhrase = pGrammar->m_pWordSequenceText + ulStartActiveOffset; nP != nPhrase; cPhrase++)
    {
        if(*cPhrase == L'\0')
        {
            nP++;
        }
    }

    // Count words in sentence
    ULONG nWord = 1;
    const WCHAR *cWord;
    for(cWord = cPhrase; *cWord != L'\0' && ULONG(cWord - pGrammar->m_pWordSequenceText) < ulStartActiveOffset + cchActiveChars; cWord++)
    {
        if(iswspace(*cWord))
        {
            while(*(cWord+1) && iswspace(*(cWord+1)) && ULONG(cWord - pGrammar->m_pWordSequenceText) < ulStartActiveOffset + cchActiveChars - 1) {cWord++;}
            nWord++;
        }
    }

    // Pick entry and exit word
    ULONG startWord = rand() % nWord;
    ULONG countWord = rand() % (nWord - startWord) + 1;

    // Find the entry word
    for(nWord = 0, cWord = cPhrase; nWord != startWord; cWord++)
    {
        if(iswspace(*cWord))
        {
            while(*(cWord+1) && iswspace(*(cWord+1)) && ULONG(cWord - pGrammar->m_pWordSequenceText) < ulStartActiveOffset + cchActiveChars - 1) {cWord++;}
            nWord++;
        }
    }

    // Build paths to end word
    const WCHAR *cW = cWord;
    for(nWord = 0; nWord != countWord; cWord++)
    {
        if(*cWord == L'\0' || iswspace(*cWord) || ULONG(cWord - pGrammar->m_pWordSequenceText) == ulStartActiveOffset + cchActiveChars)
        {
            // Build a path entry
            pPath->hTransition = hId;
            memset(&pPath->elem, 0, sizeof(pPath->elem));
            pPath->elem.bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
            WCHAR *pszWord = new WCHAR[cWord - cW + 1];
            wcsncpy(pszWord, cW, cWord - cW);
            pszWord[cWord - cW] = '\0';
            pPath->elem.pszDisplayText = pszWord;

            pPath++;
            (*pcTrans)++;

            while(*(cWord+1) && iswspace(*(cWord+1)) && ULONG(cWord - pGrammar->m_pWordSequenceText) < ulStartActiveOffset + cchActiveChars - 1) {cWord++;}
            cW = cWord + 1; // first char of next word
            nWord++;
        }
    }

    return hr;
}

/****************************************************************************
* CSrEngine::RecurseWalk *
*----------------------*
*   Description:
*       This method produces a random path through an active CFG. If the path contains
*       a rule reference transition RecurseWalk is recursively called to produce a path though
*       sub-rules. The result is an array of SPPATHENTRY elements containing the transitions.
*
*       The initial state in each rule is obtained by calling GetRuleInfo. Then for each
*       state GetStateInfo can be called. This gives an array of SPTRANSITION entries
*       the contain information on the type of transition, the transition id and 
*       the next state the transition goes to. The transition id is the main information
*       included in the SPPATHENTRY. Only for word transitions are SPPATHENTRY created,
*       as this is all that is required by ParseFromTransitions.
*
*   Return:
*       S_OK
*       FAILED(hr)
****************************************************************************/
HRESULT CSrEngine::RecurseWalk(SPSTATEHANDLE hState, SPPATHENTRY * pPath, ULONG * pcTrans)
{
    HRESULT hr = S_OK;

    CSpStateInfo StateInfo;
    *pcTrans = 0;
    while (SUCCEEDED(hr) && hState)
    {
        ULONG cTrans;
        hr = m_cpSite->GetStateInfo(hState, &StateInfo);
        if (SUCCEEDED(hr))
        {
            //  Now randomly decide which transition to take.
            ULONG cTransInState = StateInfo.cEpsilons + StateInfo.cWords + StateInfo.cRules + StateInfo.cSpecialTransitions;
            if (cTransInState == 0)
            {
                // This path is a dead-end. Most likely this is due to an empty dynamic rule.
                hr = E_FAIL;
                break;
            }
            SPTRANSITIONENTRY * pTransEntry = StateInfo.pTransitions + (rand() % cTransInState);

            switch(pTransEntry->Type)
            {
            case SPTRANSEPSILON:
                // Epsilon transition - don't need to create a path entry
                // Advance to the next state.
                break;
            case SPTRANSRULE:
                // Rule transition - we recursively descend into the rule and add onto the path array
                hr = RecurseWalk(pTransEntry->hRuleInitialState, pPath, &cTrans);
                *pcTrans += cTrans;
                pPath += cTrans;
                break;
            case SPTRANSWORD:
            case SPTRANSWILDCARD:
                // For a word transition we complete an SPPATHENTRY structure with the transition id.
                // A wildcard transition indicates the engine should match against any speech, so we do the same thing.
                pPath->hTransition = pTransEntry->ID;
                memset(&pPath->elem, 0, sizeof(pPath->elem));
                pPath->elem.bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
                pPath++;
                (*pcTrans)++;
                break;
            case SPTRANSTEXTBUF:
                // Text Buffer transition - produce a path from WalkTextBuffer
                hr = WalkTextBuffer(pTransEntry->pvGrammarCookie, pPath, pTransEntry->ID, &cTrans);
                *pcTrans += cTrans;
                pPath += cTrans;
                break;
            case SPTRANSDICTATION:
                // Dictation transition - indicating the recognizer should do dictation at
                // this point in the grammar. We generate the DICT_WORD as a path entry.
                // The word text is indicated by setting pszDisplayText, which otherwise can be left NULL.
                pPath->hTransition = pTransEntry->ID;
                memset(&pPath->elem, 0, sizeof(pPath->elem));
                pPath->elem.bDisplayAttributes = SPAF_ONE_TRAILING_SPACE;
                WCHAR *pszWord = new WCHAR[wcslen(DICT_WORD) + 1];
                wcscpy(pszWord, DICT_WORD);
                pPath->elem.pszDisplayText = pszWord;
                pPath++;
                (*pcTrans)++;
                break;
            }

            // Move to the next state - a transition to NULL indicates the end of the rule.
            hState = pTransEntry->hNextState;
        }
    }
    return hr;
}



/****************************************************************************
* CSrEngine::GetInputAudioFormat *
*---------------------------*
*   Description:
*       This method is called for SAPI to find what audio formats the engine
*       can support for recognition.
*       Audio formats a represented by a GUID and an optional WAVEFORMATEX
*       structure for wav file formats. This method may be called
*       either with pSourceFormatId and pSourceWaveFormatEx NULL, in which
*       case the engine should return its most preferred format. It may also
*       be called with these set, in which case the engine should return
*       that format if it can support or the closest it can.
*
*       If the engines returned format is incompatible with the input audio
*       object format SAPI will try and create a format convertor to convert the audio
*       
*   Return: 
*       S_OK
*       FAILED(hr)
*****************************************************************************/   
STDMETHODIMP CSrEngine::GetInputAudioFormat(const GUID * pSourceFormatId, const WAVEFORMATEX * pSourceWaveFormatEx,
                                            GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWFEX)
{
    SPDBG_FUNC( "Sample Engine - GETINPUTAUDIOFORMAT" );

    // Helper function which fills in the sample engine desired format (PCM 11kHz Mono).
    return SpConvertStreamFormatEnum(SPSF_11kHz16BitMono, pDesiredFormatId, ppCoMemDesiredWFEX);
} 


/*****************************************************************************
* CSrEngine::SetPropertyNum *
*---------------------*
*   Description:
*       The following methods are used for the app to send and receive
*       real-time attribute information to the engine. This differs from the
*       ISpObjectWithToken mechanism which is used to query static registry information.
*       If the app tries to set or query an attribute the engine should return
*       okay if it supports that attribute and S_FALSE if it does not.
*       In this method the app is trying to set a numeric property value.
*   Return: 
*       S_FALSE
*****************************************************************************/
STDMETHODIMP CSrEngine::
    SetPropertyNum( SPPROPSRC eSrc, PVOID pvSrcObj, const WCHAR* pName, LONG lValue )
{
    SPDBG_FUNC( "Sample Engine - SETPROPERTYNUM" );
    HRESULT hr = S_OK;

    hr = S_FALSE;   // We don't support any properties

    return hr;
}

/*****************************************************************************
* CSrEngine::GetPropertyNum *
*---------------------*
*   Description:
*       In this method the app is trying to get a numeric property value.
*****************************************************************************/
STDMETHODIMP CSrEngine::
    GetPropertyNum( SPPROPSRC eSrc, PVOID pvSrcObj, const WCHAR* pName, LONG * plValue )
{
    SPDBG_FUNC( "Sample Engine - GETPROPERTYNUM" );
    HRESULT hr = S_OK;

    hr = S_FALSE;
    *plValue = 0;

    return hr;
}

/*****************************************************************************
* CSrEngine::SetPropertyString *
*----------------------*
*   Description:
*       In this method the app is trying to set a string property value.
*****************************************************************************/
STDMETHODIMP CSrEngine::
    SetPropertyString( SPPROPSRC eSrc, PVOID pvSrcObj, const WCHAR* pName, const WCHAR* pValue )
{
    SPDBG_FUNC( "Sample Engine - SETPROPERTYSTRING" );
    HRESULT hr = S_OK;

    hr = S_FALSE;

    return hr;
}

/*****************************************************************************
* CSrEngine::GetPropertyString *
*----------------------*
*   Description:
*       In this method the app is trying to get a string property value.
*****************************************************************************/
STDMETHODIMP CSrEngine::
    GetPropertyString( SPPROPSRC eSrc, PVOID pvSrcObj, const WCHAR* pName, WCHAR** ppCoMemValue )
{
    SPDBG_FUNC( "Sample Engine - GETPROPERTYSTRING" );
    HRESULT hr = S_OK;

    hr = S_FALSE;
    *ppCoMemValue = NULL;

    return hr;
}


/****************************************************************************
* CSrEngine::PrivateCall *
*---------------------------*
*   Description:
*       This method is used for a reco extension to send private, engine-specific information
*       to an engine it knows about. The app QI's off the reco context for a private
*       interface, and SAPI creates the engine' extension object (in this case CSampleSRExtension).
*       The app can then call methods on this interface. If the extension object wants to
*       communicate with the engine it calls CallEngine. The data from that call is then
*       passed to this method on the engine.
*
*       Similarly the alternates class and UI class can also use this mechanism to
*       communicate with their main engine class.
*
*       The single buffer is in/out. This call can update the buffer, and the updated buffer
*       will be returned to the caller of _ISpPrivateEngineCall::CallEngine().  To return
*       variable size buffers, use _ISpPrivateEngineCall::CallEngineEx() and 
*       ISpSREngine::PrivateCallEx().
*
*   Return: 
*       S_OK
*****************************************************************************/   
STDMETHODIMP CSrEngine::PrivateCall(void * pvEngineContext, void * pCallFrame, ULONG ulCallFrameSize)
{
    SPDBG_FUNC( "Sample Engine - PRIVATECALL" );
    // Just an example - do nothing here
    return S_OK;
}

/****************************************************************************
* CSrEngine::PrivateCallEx *
*--------------------------*
*   Description:
*       This method is similar to PrivateCall except that the call frame is an
*       input only parameter.  This function should CoTaskMemAlloc a response block
*       and return it in *ppvCoMemResponse.  You must also return the size of the
*       allocated block (in bytes) in *pcbResponse.
*
*       While this is slightly more work than CallEngine/PrivateCall, it allows for
*       variable size responses, which could be more efficent in some cases.
*
*   Returns:
*       S_OK
*
*****************************************************************************/

STDMETHODIMP CSrEngine::PrivateCallEx(void * pvEngineContext, const void * pInCallFrame, ULONG ulCallFrameSize,
                                      void ** ppvCoMemResponse, ULONG * pcbResponse)
{
    SPDBG_FUNC("SampleEngine - PrivateCallEx");
    HRESULT hr = S_OK;

    *ppvCoMemResponse = NULL;
    *pcbResponse = 0;

    return hr;
}

