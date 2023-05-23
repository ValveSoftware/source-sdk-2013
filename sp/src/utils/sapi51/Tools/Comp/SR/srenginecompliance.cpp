//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// srenginecompliance.cpp
// 
// Test module for srenginecompliance
//******************************************************************************
#include "SRTests1.h"
#include "resource.h"
#include "srenginecompliance.h"
#include "sphelper.h"
#include "sapiddk.h"
#include <time.h>
#include <stdio.h>
#include <wchar.h>
#include <Commdlg.h>

//******************************************************************************
//***** Globals
//******************************************************************************
extern SPS_SHELL_INFO *g_pShellInfo;
extern CRITICAL_SECTION g_csProcess;


//******************************************************************************
//***** Internal Functions and helper classes
//******************************************************************************

// called before dll is unloaded
void CleanupTest(void)
{
	// nothing to do at this time	
};









// get string from resource
void GetWStrFromRes(UINT id, WCHAR* szwStr)
{
	EnterCriticalSection (&g_csProcess);

    HRSRC       hResInfo = NULL;
    HANDLE      hStringSeg = NULL;
    LPWSTR      lpsz = NULL;
    int         cch = 0;
	int	        cchBufferMax = MAX_PATH;

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceEx(g_pShellInfo->hLib, (LPCTSTR)RT_STRING,
                                    (LPCTSTR)((LONG)(((USHORT)id >> 4) + 1)), 
                                    ::GetDefaultEnginePrimaryLangId())) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(g_pShellInfo->hLib, hResInfo);

        // Lock the resource.        
        if (hStringSeg && (lpsz = (LPWSTR)LockResource(hStringSeg)) != NULL) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            id &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (id-- == 0) break;
                lpsz += cch;                // Step to start if next string
            }

            // Account for the NULL                
            cchBufferMax--;

            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax-1;

            // Copy the string into the buffer.                
            CopyMemory(szwStr, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            szwStr[cch] = 0;
        }
    }

	LeaveCriticalSection (&g_csProcess);

}

inline WCHAR* GetDefaultPronunciation(const WCHAR* szSymbol = NULL, LANGID LangID = ::GetDefaultEnginePrimaryLangId())
{


	HRESULT hr = S_OK;

    WCHAR szPronSymbol[MAX_PATH] = L"";
	CComPtr<ISpPhoneConverter> cpPhoneConverter;
	static SPPHONEID pphoneId[MAX_PATH] = L"";

    if (szSymbol)
    {
        wcscpy(szPronSymbol, szSymbol);
    }
    else
    {
    	GetWStrFromRes(IDS_RECO_NEWWORD_PRON, szPronSymbol);
    }


	hr = SpCreatePhoneConverter(LangID, NULL, NULL, &cpPhoneConverter);

	if (SUCCEEDED(hr))
	{
		hr = cpPhoneConverter->PhoneToId(CSpDynamicString(szPronSymbol), pphoneId);
	}


	return pphoneId;
	
}



HRESULT GetFullPath(WCHAR* wszFileName, WCHAR* wszPathName)
{
	WCHAR wszLocation[MAX_PATH] = L"";

	if (wszFileName && _waccess(wszFileName, 0) == 0) // file exists
	{
		wcscpy(wszPathName, wszFileName);
		return S_OK;
	}
	else
	{
		GetWStrFromRes(IDS_RESOURCE_LOCATION1, wszLocation);
		wcscat(wszLocation, wszFileName);
		if (wszLocation && _waccess(wszLocation, 0) == 0)
		{
			wcscpy(wszPathName, wszLocation);
			return S_OK;
		}
		else
		{
			GetWStrFromRes(IDS_RESOURCE_LOCATION2, wszLocation);
			wcscat(wszLocation, wszFileName);
			if (wszLocation && _waccess(wszLocation, 0) == 0)
			{
				wcscpy(wszPathName, wszLocation);
				return S_OK;
			}
		}
	}

	return E_FAIL;

}

// open wav file
HRESULT OpenWavFile(UINT uID, ISpStream ** ppStream, ULONGLONG ullEventInterest = SPFEI_ALL_EVENTS)
{
	EnterCriticalSection (&g_csProcess);

	HRESULT hr = S_OK;
	WCHAR wszLocation[MAX_PATH] = L"";
	WCHAR wszWavFile[MAX_PATH] = L"";

	GetWStrFromRes(uID, wszWavFile);

    // check location of process

    //check the customized location first
    if (_tcslen(ptszCustomizedDirectory))
    {
        wcscpy(wszLocation, CSpDynamicString(ptszCustomizedDirectory));
		wcscat(wszLocation, wszWavFile);
	    hr = SPBindToFile(wszLocation, SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);
    }
    else
    {
        hr = SPERR_NOT_FOUND;
    }

    if (FAILED(hr))
    {
    	hr = SPBindToFile(wszWavFile, SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);
    }

	// check other resource locations
	if (FAILED(hr))
	{
		GetWStrFromRes(IDS_RESOURCE_LOCATION1, wszLocation);
		wcscat(wszLocation, wszWavFile);
    	hr = SPBindToFile(wszLocation, SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);
	}

	// check second location
	if (FAILED(hr))
	{
		GetWStrFromRes(IDS_RESOURCE_LOCATION2, wszLocation);
		wcscat(wszLocation, wszWavFile);
    	hr = SPBindToFile(wszLocation, SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);
	}

	// check third location
	if (FAILED(hr))
	{
		GetWStrFromRes(IDS_RESOURCE_LOCATION3, wszLocation);
		wcscat(wszLocation, wszWavFile);
    	hr = SPBindToFile(wszLocation, SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);
	}

    if (FAILED(hr))
    {
        TCHAR tszFileName[MAX_PATH] = _T("");
        TCHAR tszFilterName[MAX_PATH] = _T("");
        TCHAR* tszTem = NULL;
        OPENFILENAME ofn;
        CSpTString<MAX_PATH> SpTString(wszWavFile);
        ZeroMemory(&ofn, sizeof(ofn));


        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn. hwndOwner = g_pShellInfo->hWnd;
        ofn.hInstance = g_pShellInfo->hInstance;

        //construct the filter, like "Compliance Wave Input (tag_l.wav)\0tag_l.wav\0\0"
        _tcscpy(tszFilterName, _T("Compliance Wave Input ("));
        _tcscat(tszFilterName, SpTString);
        _tcscat(tszFilterName, _T(")"));
        tszTem = _tcscpy(tszFilterName + _tcslen(tszFilterName) + 1, SpTString);
        _tcscpy(tszTem + _tcslen(tszTem) + 1, _T(""));

        ofn.lpstrFilter = tszFilterName;

        _tcscpy(tszFileName, SpTString);
        ofn.lpstrFile = tszFileName;
        ofn.nMaxFile = MAX_PATH;

        if (_tcslen(ptszCustomizedDirectory))
            ofn.lpstrInitialDir =  ptszCustomizedDirectory;
        else
            ofn.lpstrInitialDir = NULL;

        ofn.lpstrTitle = _T("Please choose the resource directory");
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_READONLY;

        BOOL fSuccess = GetOpenFileName(&ofn);
        if (fSuccess)
        {
            _tcscpy(ptszCustomizedDirectory, ofn.lpstrFile);
        	hr = SPBindToFile(CSpDynamicString(ptszCustomizedDirectory), SPFM_OPEN_READONLY, ppStream, NULL, NULL, ullEventInterest);

            TCHAR* tszTem = _tcsrchr(ptszCustomizedDirectory, _T('\\'));
            *(tszTem+1) = NULL;
        }
        else
        {
            //the user cancels the dialog box
        }
    }

	LeaveCriticalSection (&g_csProcess);

	return hr;
};

void Comment(UINT uID)
{
    USES_CONVERSION;
	EnterCriticalSection (&g_csProcess);

	// for string from resource and comment
	static WCHAR wszMessage[MAX_PATH] = L"";

	GetWStrFromRes(uID, wszMessage);

	g_pKato->Comment(5, W2A(wszMessage));

	LeaveCriticalSection (&g_csProcess);

};

void Comment(WCHAR* wszMessage)
{
    USES_CONVERSION;
	EnterCriticalSection (&g_csProcess);

	// for string from resource and comment

    if (wszMessage)
    	g_pKato->Comment(5, W2A(wszMessage));

	LeaveCriticalSection (&g_csProcess);

};

BOOL IsMatchingAttribute(WCHAR* pwszAttribute, WCHAR* pwszValue = NULL)
{
	CComPtr<ISpResourceManager> cpResMgr;
	CComPtr<ISpObjectToken> cpObjectToken;
	BOOL IsMatching = FALSE;

	HRESULT hr = S_OK;
	hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpObjectToken, FALSE);
	if (hr == S_OK)
	{
		CComPtr<ISpDataKey> cpAttributeKey;
		hr = cpObjectToken->OpenKey(L"Attributes", &cpAttributeKey);
		if (hr == S_OK)
		{
			CSpDynamicString dsValue;
			hr = cpAttributeKey->GetStringValue(pwszAttribute, &dsValue);
			if (hr == S_OK)
			{
                if (pwszValue)
                {
                    const WCHAR pwszTem[] = L" ;";
                    const WCHAR* pwszTemToken = wcstok(dsValue, pwszTem);
                    while (pwszTemToken && (!IsMatching))
                    {
                        if (!wcscmp(pwszTemToken, pwszValue))
                        {
                            IsMatching = TRUE;
                        }
                        else
                        {
                            pwszTemToken = wcstok(NULL, pwszTem);
                        }
                    }
                }
                else
                //If pwszValue == NULL, it means we don't care about the value. We are just interested in the attribute.
                {
    				IsMatching = TRUE;
                }
			}
		}
	}

	return IsMatching;

}

LANGID GetDefaultEnginePrimaryLangId()
{
    LANGID LangId = 0;
    CComPtr<ISpRecognizer> cpRecognizer;
    HRESULT hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);
    if (SUCCEEDED(hr))
    {
        SPRECOGNIZERSTATUS SpRecognizerStatus;
        ZeroMemory(&SpRecognizerStatus, sizeof(SpRecognizerStatus));
        hr = cpRecognizer->GetStatus(&SpRecognizerStatus);
        if (SUCCEEDED(hr) && SpRecognizerStatus.cLangIDs)
        {
            LangId = SpRecognizerStatus.aLangID[0];
        }
    }
    return LangId;
}

SPWORDPRONUNCIATIONLIST spPronList;
SPWORDPRONUNCIATION* psppron = NULL;

// internal helper function for the Lexicon test cases
HRESULT HlpAddCustomWord(CComPtr<ISpLexicon> &cpLexicon, WCHAR *wszCustomWord, WCHAR *wszPron = NULL)
{
	HRESULT hr;
	WCHAR szPronunciation[MAX_PATH] = L"";
    LANGID LangID = ::GetDefaultEnginePrimaryLangId();

	// first, delete the word if it exists
	ZeroMemory(&spPronList, sizeof(spPronList));
    hr = cpLexicon->GetPronunciations(wszCustomWord, LangID, eLEXTYPE_USER, &spPronList);

	if (hr == SPERR_NOT_IN_LEX || hr == SP_WORD_EXISTS_WITHOUT_PRONUNCIATION)
	{
		hr = S_OK;
	}
	else if (SUCCEEDED(hr))
	{
		//Comment(IDS_ERR_LEX_EXPECTNOWORD);
		hr = cpLexicon->RemovePronunciation(wszCustomWord, LangID, SPPS_Unknown, NULL);
	}


	// next, get pronunciation from resource string and make it official
	wcscpy(szPronunciation, GetDefaultPronunciation(wszPron));

	if ( wcslen(szPronunciation) )
	{
		hr = cpLexicon->AddPronunciation(wszCustomWord, LangID, SPPS_Noun, szPronunciation);
	}
	


	return hr;
}

// internal helper function for the Lexicon test cases
HRESULT HlpRemoveCustomWord(CComPtr<ISpLexicon> &cpLexicon, WCHAR *wszCustomWord)
{
	HRESULT hr;
	WCHAR szPronunciation[MAX_PATH] = L"";
    LANGID LangID = ::GetDefaultEnginePrimaryLangId();

	// first, delete the word if it exists
	hr = cpLexicon->RemovePronunciation(wszCustomWord, LangID, SPPS_Unknown, NULL);


	//Comment(IDS_ERR_LEX_EXPECTNOWORD);
	for(psppron = spPronList.pFirstWordPronunciation; psppron && SUCCEEDED(hr) && (hr != SP_ALREADY_IN_LEX); psppron = psppron->pNextWordPronunciation)
	{
			hr = cpLexicon->RemovePronunciation(wszCustomWord, psppron->LangID, psppron->ePartOfSpeech, psppron->szPronunciation);
	}

	if (spPronList.pvBuffer) ::CoTaskMemFree(spPronList.pvBuffer);

	return hr;
}



HRESULT HlpCheckResult(RESULTVECTOR& vector_RecoResult, WCHAR* pszText)
{
	HRESULT hr = E_FAIL;
	int 	count = vector_RecoResult.size();
	bool bFound = false;

	for (int i=0; i<count; i++)
	{
		CSpDynamicString wpszTem;
		hr = vector_RecoResult[i]->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &wpszTem, NULL);
		if (SUCCEEDED(hr) && wcsstr(wpszTem, pszText))
		{
			bFound = true;
			break;
		}
	}

	(bFound) ? hr = S_OK : hr = S_FALSE;


	return hr;
}

// HlpGetRecoResult() - called by several of the test functions that follow;
// it activates a given grammar and waits for the recognition results to
// come in and generate the recoreusult vector.
HRESULT HlpGetRecoResult(
        CComPtr<ISpRecoContext>	&		    cpRecoCtxt,
		RESULTVECTOR& vector_RecoResult
   )
{
    USES_CONVERSION;
	HRESULT hr = S_OK;

    // Clear out the list of results from any previous test
    vector_RecoResult.clear();

    bool bDone = false;
	// get and check result
    while (!bDone && (cpRecoCtxt->WaitForNotifyEvent(TESTEVENT_TIMEOUT)) == S_OK)
    {
		CSpEvent event;

        while (event.GetFrom(cpRecoCtxt) == S_OK)
        {

            if (event.eEventId == SPEI_END_SR_STREAM)
                bDone = true;
            else if (event.eEventId == SPEI_RECOGNITION)
            {
				vector_RecoResult.push_back(event.RecoResult());
            } // else if (event.eEventId == SPEI_RECO)
        } // while

    }

	if ( !vector_RecoResult.size())
		hr = E_FAIL;

    return hr;
}
	



/*
CEventList is a class used to record the events generated by a speech
recognition pass over a wave file.  It uses the CEventRecord class for
its nodes; it records the event type, the stream offset, and the rule
name in case of recognition events.

These two classes are used by the HlpGatherEvents function.
*/
class CEventRecord
{
public:
    SPEVENTENUM m_eEventId;
    ULONGLONG m_ullOffset;
    CSpDynamicString m_dstrRuleName;
    CSpDynamicString m_dstrRecoResult;
    CEventRecord * m_pNext;

    CEventRecord() { m_pNext = NULL; }
    ~CEventRecord() { m_dstrRuleName.Clear(); }
    CEventRecord(CSpEvent &e)
    {
        m_eEventId = e.eEventId;
        m_ullOffset = e.ullAudioStreamOffset;
        if (m_eEventId == SPEI_RECOGNITION)
        {
            SPPHRASE *pPhrase = NULL;
            e.RecoResult()->GetPhrase(&pPhrase);
            m_dstrRuleName = pPhrase->Rule.pszName;
            if (pPhrase) CoTaskMemFree(pPhrase);
			e.RecoResult()->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &m_dstrRecoResult, NULL);
        }
        m_pNext = NULL;
    }
};

class CEventList
{
public:
    CEventRecord *m_pHead;
    CEventList() { m_pHead = NULL; }
    void Append(CSpEvent &e)
    {
        CEventRecord **p = &m_pHead;
        while (*p)
            p = &((*p)->m_pNext);
        *p = new CEventRecord(e);
    }
    void Release()
    {
        CEventRecord *q, *p = m_pHead;
        while (q = p)
        {
            p = q->m_pNext;
            delete(q);
        }
        m_pHead = NULL;
    }
    ~CEventList() { Release(); }
};

const char *HlpEventToString(SPEVENTENUM eId)
{
    switch (eId)
    {
    case SPEI_UNDEFINED : return "?";
	case SPEI_END_SR_STREAM : return "EndStrm";
	case SPEI_SOUND_START : return "SndSt";
	case SPEI_SOUND_END : return "SndEnd";
	case SPEI_PHRASE_START : return "PhrSt";
	case SPEI_RECOGNITION : return "Reco";
	case SPEI_HYPOTHESIS : return "Hyp";
	case SPEI_FALSE_RECOGNITION : return "FReco";
	case SPEI_INTERFERENCE : return "Intf";
    default : return "Misc";
    }
}

HRESULT HlpGatherEvents(ULONGLONG Events, ULONGLONG QueuedEvents,
                    UINT uWaveFileId, CEventList &EventList, UINT idGrammar = 0)
{
	HRESULT hr = S_OK;

	// speech
	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpStream>					cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(
            Events | SPFEI(SPEI_END_SR_STREAM),
            QueuedEvents | SPFEI(SPEI_END_SR_STREAM));


	// open wav file for sound start test
	if (SUCCEEDED(hr))
		hr = OpenWavFile(uWaveFileId, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return E_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

    // load and activate the default grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(GRAM_ID, &cpGrammar);

    if (!idGrammar)
	{
		if (SUCCEEDED(hr))
			hr = cpGrammar->LoadDictation(/* topic name */ NULL, SPLO_STATIC);

		if (SUCCEEDED(hr))
			hr = cpGrammar->SetDictationState(SPRS_ACTIVE);
	}
	else
	{
		if (SUCCEEDED(hr))
			hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
				(const WCHAR*)MAKEINTRESOURCE(idGrammar),
				L"SRGRAMMAR", GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

		if (SUCCEEDED(hr))
			hr = cpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}





    bool bDone = false;
    DWORD dwWait = 0;

	while ( !bDone && (hr = cpRecoCtxt->WaitForNotifyEvent(WAIT_TIME)) == S_OK )
	{
		CSpEvent event;
        int numEvents = 0;

		while (event.GetFrom(cpRecoCtxt) == S_OK)
		{
            numEvents++;
			if (event.eEventId == SPEI_END_SR_STREAM)
				bDone = true;
            EventList.Append(event);
		} // while GetEvents
	}


	return hr;
}


TESTPROCAPI t_CheckEvent_SoundStart(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;



	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_SOUND_START), SPFEI(SPEI_SOUND_START),
        IDS_WAV_SOUNDSTART, eventList, IDR_L_GRAMMAR);

	//stream positions
    CEventRecord *peSoundStart = NULL;
    CEventRecord *peStreamEnd = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_SOUND_START:
					if (!peSoundStart)
						peSoundStart = p;
					break;
				case SPEI_END_SR_STREAM:
                    peStreamEnd = p;
					break;
				case SPEI_RECOGNITION:
					//recognition is set as a default event by SAPI
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peSoundStart)
		{
			Comment(IDS_ERR_SOUNDSTART_NOEVENT);
			tpr = TPR_FAIL;
		}

		if (!peStreamEnd)
		{
			Comment(IDS_ERR_STREAMEND_NOEVENT);
			tpr = TPR_FAIL;
		}

		// check ullAudioStreamOffset for position
		if (peSoundStart && peStreamEnd)
		{
			// check that sound start comes before stream end
			if (peSoundStart->m_ullOffset > peStreamEnd->m_ullOffset)
			{
				Comment(IDS_ERR_SOUNDSTART_BADPOSITION1);
				tpr = TPR_FAIL;
			}
		}
    }
	else
	// cleanup
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	return tpr;
}

TESTPROCAPI t_CheckEvent_Interference(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	HRESULT hr = S_OK;
	int	tpr = TPR_SUPPORTED;
    CEventList eventList;

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;



	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_INTERFERENCE), SPFEI(SPEI_INTERFERENCE),
        IDS_WAV_INTERFERENCE, eventList, IDR_L_GRAMMAR);

	//stream positions
    CEventRecord *peInterference = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_INTERFERENCE:
					if (!peInterference)
						peInterference = p;
					break;
				case SPEI_END_SR_STREAM:
 				case SPEI_RECOGNITION:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_UNSUPPORTED;
			}
		}
		
		if (!peInterference)
		{
			Comment(IDS_ERR_INTERFERENCE_NOEVENT);
			tpr = TPR_UNSUPPORTED;
		}

    }
	else
	// cleanup
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_UNSUPPORTED;
	}

	return tpr;
}

TESTPROCAPI t_GetITNResult(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

	HRESULT hr = S_OK;
	int	tpr = TPR_SUPPORTED;
    CEventList eventList;
    CComPtr<ISpRecognizer> cpRecognizer;
    CComPtr<ISpRecoContext>  cpRecoContext;

    CComPtr<ISpRecoGrammar> cpRecoGrammar;
    CComPtr<ISpStream>                cpStream;
	RESULTVECTOR vector_RecoResult;
	WCHAR pwszText[MAX_PATH] = L"";



	GetWStrFromRes(IDS_RECO_GETITNRESULT, pwszText);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_GETITNRESULT, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	if (SUCCEEDED(hr))
			hr = cpRecoGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
				(const WCHAR*)MAKEINTRESOURCE(IDR_RULE_GRAMMAR),
				L"SRGRAMMAR", GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

	if (vector_RecoResult.size())
	{
		CSpDynamicString dsText;
		hr = vector_RecoResult[0]->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dsText, NULL);
		if (SUCCEEDED(hr) &&!wcscmp(dsText, pwszText))
		{
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
		}
	}
	else
	{
		tpr = TPR_UNSUPPORTED;
	}

	return tpr;
}


TESTPROCAPI t_CheckEvent_SoundEnd(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_SOUND_END), SPFEI(SPEI_SOUND_END),
        IDS_WAV_SOUNDEND, eventList, IDR_L_GRAMMAR);

	//stream positions
    CEventRecord *peSoundEnd = NULL;
    CEventRecord *peStreamEnd = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_SOUND_END:
                    peSoundEnd = p;
					break;
				case SPEI_END_SR_STREAM:
                    peStreamEnd = p;
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peSoundEnd)
		{
			Comment(IDS_ERR_SOUNDEND_NOEVENT);
			tpr = TPR_FAIL;
		}

		if (!peStreamEnd)
		{
			Comment(IDS_ERR_STREAMEND_NOEVENT);
			tpr = TPR_FAIL;
		}

		// check ullAudioStreamOffset for position
		if (peSoundEnd && peStreamEnd)
		{
			// check that sound start comes before stream end
			if (peSoundEnd->m_ullOffset > peStreamEnd->m_ullOffset)
			{
				Comment(IDS_ERR_SOUNDEND_BADPOSITION1);
				tpr = TPR_FAIL;
			}
		}
    }
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	return tpr;
}


TESTPROCAPI t_CheckEvent_SoundStartEnd(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END),
        SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END),
        IDS_WAV_SOUNDSTARTEND, eventList, IDR_L_GRAMMAR);

	//stream positions
	BOOL fInSound = FALSE;
    CEventRecord *peSoundStart = NULL;
    CEventRecord *peSoundEnd = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_SOUND_START:
					{
						if (!fInSound)
						{
							peSoundStart = p;
							fInSound = TRUE;
						}
						else
						{
							tpr = TPR_FAIL;
							peSoundStart = NULL;
						}
					}
					break;
				case SPEI_SOUND_END:
					{
						if (fInSound)
						{
							fInSound = FALSE;
	                        peSoundEnd = p;
						}
						else
						{
							tpr = TPR_FAIL;
							peSoundEnd = NULL;
						}
					}
					break;
				case SPEI_END_SR_STREAM:
				case SPEI_RECOGNITION:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}

		//We need to make sure that sound start and sound end are pairs of events
		if (!(peSoundStart && peSoundEnd && (!fInSound)))
		{
			Comment(IDS_ERR_SOUNDSTARTEND_WRONGORDER);
			tpr = TPR_FAIL;
		}
    }
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

	return tpr;
}


TESTPROCAPI t_CheckEvent_PhraseStart(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;



	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_PHRASE_START), SPFEI(SPEI_PHRASE_START),
        IDS_WAV_PHRASESTART, eventList, IDR_L_GRAMMAR);

	//stream positions
    CEventRecord *pePhraseStart = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_PHRASE_START:
				    pePhraseStart = p;
					break;
				case SPEI_END_SR_STREAM:
				case SPEI_RECOGNITION:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!pePhraseStart)
		{
			Comment(IDS_ERR_PHRASESTART_NOEVENT);
			tpr = TPR_FAIL;
		}
    }
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	return tpr;
}


TESTPROCAPI t_CheckEvent_Recognition(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION),
        IDS_WAV_RECOGNITION_1, eventList, IDR_L_GRAMMAR);

    CEventRecord *peRecognition = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_RECOGNITION:
				    peRecognition = p;
					break;
				case SPEI_END_SR_STREAM:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peRecognition)
		{
			Comment(IDS_ERR_RECOGNITION_NOEVENT);
			tpr = TPR_FAIL;
		}
    }
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

	// repeat with wav file with silence and insure that no recognition occurs.
	eventList.Release();


	return tpr;
}


TESTPROCAPI t_CheckEvent_PhraseStartRecognitionOrder(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION),
											    SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_FALSE_RECOGNITION),
												IDS_WAV_RECOGNITION_1, eventList, IDR_L_GRAMMAR);

	//stream positions
	BOOL fInPhrase = FALSE;
    CEventRecord *pePhraseStart = NULL;
    CEventRecord *peRecognition = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_PHRASE_START:
					{
						if (!fInPhrase)
						{
							pePhraseStart = p;
							fInPhrase = TRUE;
						}
						else
						{
							tpr = TPR_FAIL;
							pePhraseStart = NULL;
						}
					}
					break;
				case SPEI_RECOGNITION:
				case SPEI_FALSE_RECOGNITION:
					{
						if (fInPhrase)
						{
							fInPhrase = FALSE;
	                        peRecognition = p;
						}
						else
						{
							tpr = TPR_FAIL;
							peRecognition = NULL;
						}
					}
					break;
				case SPEI_END_SR_STREAM:
   					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!(pePhraseStart && peRecognition && (!fInPhrase)))
		{
			Comment(L"incorrect phrasestart and recognition/false recognition event order");
			tpr = TPR_FAIL;
		}
    }
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	// repeat with wav file with silence and insure that no recognition occurs.
	eventList.Release();


	return tpr;
}



TESTPROCAPI t_CheckEvent_FalseRecognition(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_FALSE_RECOGNITION), SPFEI(SPEI_FALSE_RECOGNITION),
        IDS_WAV_RECOGNITION_1, eventList, IDR_RULE_GRAMMAR);

	//stream positions
    CEventRecord *peFalseRecognition = NULL;
    CEventRecord *peStreamEnd = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_RECOGNITION:
					Comment(L"Recognition event shouldn't be recieved");
					tpr = TPR_FAIL;
					break;
				case SPEI_FALSE_RECOGNITION:
				    peFalseRecognition = p;
					break;
				case SPEI_END_SR_STREAM:
                    peStreamEnd = p;
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peFalseRecognition)
		{
			Comment(IDS_ERR_FALSERECOGNITION_NOEVENT);
			tpr = TPR_FAIL;
		}

		if (!peStreamEnd)
		{
			Comment(IDS_ERR_STREAMEND_NOEVENT);
			tpr = TPR_FAIL;
		}
    }

	// repeat with wav file with silence and insure that no recognition occurs.
	eventList.Release();




	return tpr;
}



TESTPROCAPI t_CheckEvent_EventsSequences(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_RECOGNITION)  | SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_PHRASE_START),
						 SPFEI(SPEI_RECOGNITION)  | SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) | SPFEI(SPEI_PHRASE_START),
				         IDS_WAV_RECOGNITION_1, eventList, IDR_L_GRAMMAR);

	//stream positions
    CEventRecord *peSoundStart = NULL;
    CEventRecord *pePhraseStart = NULL;
    CEventRecord *peRecognition = NULL;
    CEventRecord *peSoundEnd = NULL;
    CEventRecord *peStreamEnd = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_SOUND_START:
					peSoundStart = p;
					break;
				case SPEI_PHRASE_START:
					pePhraseStart = p;
					break;
				case SPEI_RECOGNITION:
					peRecognition = p;
					break;
				case SPEI_SOUND_END:
					peSoundEnd = p;
					break;
				case SPEI_END_SR_STREAM:
                    peStreamEnd = p;
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (peSoundStart && pePhraseStart && peRecognition && peSoundEnd && peStreamEnd)
		{
			if (peSoundStart->m_ullOffset <= pePhraseStart->m_ullOffset &&
				pePhraseStart->m_ullOffset <= peRecognition->m_ullOffset &&
				peRecognition->m_ullOffset <= peSoundEnd->m_ullOffset &&
				peSoundEnd->m_ullOffset <= peStreamEnd->m_ullOffset)
			{
			}
			else
			{
				Comment(L"incorrect event sequence");
				tpr = TPR_FAIL;
			}
		}
		else
		{
			Comment(L"failed to receive some events.");
			tpr = TPR_FAIL;
		}

    }
	else
	// cleanup
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	return tpr;
}




TESTPROCAPI t_SpPhraseElements(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;



	HRESULT hr = S_OK;
	int tpr = TPR_PASS;
	// speech
	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpStream>					cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;
	CComPtr<ISpRecoResult>				cpRecoResult;

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));


	// open wav file for sound start test
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_RULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

    // load and activate the default grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(0, &cpGrammar);
    
    if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(hr))
			hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
				(const WCHAR*)MAKEINTRESOURCE(IDR_RULE_GRAMMAR),
				L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);
	}

    if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);






    bool bDone = false;
    DWORD dwWait = 0;

	while ((hr = cpRecoCtxt->WaitForNotifyEvent(WAIT_TIME)) == S_OK)
	{
		CSpEvent event;
        int numEvents = 0;

		while (event.GetFrom(cpRecoCtxt) == S_OK)
		{
            numEvents++;
			if (event.eEventId == SPEI_RECOGNITION)
			{
				bDone = true;
				break;
			}
		} // while GetEvents

		if (bDone)
		{
			cpRecoResult = event.RecoResult();
			if (cpRecoResult)
			{
				SPPHRASE* pPhrase;
				hr = cpRecoResult->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr))
				{
					for (ULONG i=0; i<pPhrase->Rule.ulCountOfElements-1; i++)
					{
						if (pPhrase->pElements[i].ulAudioStreamOffset >= pPhrase->pElements[i+1].ulAudioStreamOffset)
						{
							tpr = TPR_FAIL;
							break;
						}
					}
				}
			}
			break;
		}
    }



	return tpr;
}


TESTPROCAPI t_CheckEvent_Hypothesis(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

    if (!IsMatchingAttribute(L"Hypotheses"))
    {
        return TPR_UNSUPPORTED;
    }

	HRESULT hr = S_OK;
	int	tpr = TPR_SUPPORTED;
    CEventList eventList;


	// get events
	hr = HlpGatherEvents(SPFEI(SPEI_HYPOTHESIS), SPFEI(SPEI_HYPOTHESIS),
        IDS_WAV_HYPOTHESIS, eventList, IDR_EXPRULE_GRAMMAR);

	//stream positions
    CEventRecord *peHypothesis = NULL;
    CEventRecord *peStreamEnd = NULL;

    if (SUCCEEDED(hr))
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_HYPOTHESIS:
				    peHypothesis = p;
					break;
				case SPEI_END_SR_STREAM:
                    peStreamEnd = p;
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_UNSUPPORTED;
			}
		}
		
		if (!peHypothesis)
		{
			Comment(IDS_ERR_HYPOTHESIS_NOEVENT);
			tpr = TPR_UNSUPPORTED;
		}

		if (!peStreamEnd)
		{
			Comment(IDS_ERR_STREAMEND_NOEVENT);
			tpr = TPR_UNSUPPORTED;
		}
    }

	// cleanup
	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_UNSUPPORTED;
	}


	return tpr;
}



class CAppLexicon {
private:
	SPWORDPRONUNCIATIONLIST			spPronList; 
	HRESULT hr;
	CComPtr<ISpContainerLexicon>	cpConlexicon;
	CSpDynamicString dsWord;

public:
	CAppLexicon (WCHAR* wszWord, WCHAR* wszAppSymbol, WCHAR* wszUserSymbol, HRESULT* phr)
	{
		CComPtr<ISpLexicon>				cpLexiconApp;
		hr = S_OK;
		dsWord = CSpDynamicString(wszWord);
		LANGID LangID = ::GetDefaultEnginePrimaryLangId();

        RemoveTestAppLexicon(L"SRCompliance;AppLexTest");

		if (SUCCEEDED(hr))
		{
			//Create a test app lexicon
			hr = CreateAppLexicon(
				L"App lex test",
				LangID,
				L"App Lex test",
				L"SRCompliance;AppLexTest",
				&cpLexiconApp);
		}

		if (SUCCEEDED(hr))
		{
			// add a new word pronunciation to the new created app lexicon
			hr = cpLexiconApp->AddPronunciation(dsWord, LangID, SPPS_Noun, GetDefaultPronunciation(wszAppSymbol));
		}

		cpLexiconApp.Release();


		//the following is to erase the pronunciations of the new word in User's lexicon
		memset(&spPronList, 0, sizeof(spPronList) );

		if (SUCCEEDED(hr))
			hr = cpConlexicon.CoCreateInstance(CLSID_SpLexicon);

		if (SUCCEEDED(hr))
		{
			//get the pronounciation of the new word from the User's lexicon 
			hr = cpConlexicon->GetPronunciations(dsWord, LangID, eLEXTYPE_USER, &spPronList);
			if (SUCCEEDED(hr))
			{
				//remove all the pronunciations of the new word in User lexicon
				hr = cpConlexicon->RemovePronunciation(dsWord, LangID, SPPS_Unknown, NULL);
			}
			else if (hr == SPERR_NOT_IN_LEX || hr == SP_WORD_EXISTS_WITHOUT_PRONUNCIATION)
			{
				hr = S_OK;
			}
		}

		if (SUCCEEDED(hr) && wszUserSymbol)
		{
			hr = cpConlexicon->AddPronunciation(dsWord, LangID, SPPS_Noun, GetDefaultPronunciation(wszUserSymbol));
		}


		*phr = hr;
	}

	~CAppLexicon()
	{
		SPWORDPRONUNCIATION				*psppron = NULL;

		if (cpConlexicon)
		{
			//remove the pronounciation. We don't care about the HRESULT.
			hr = cpConlexicon->RemovePronunciation(dsWord, ::GetDefaultEnginePrimaryLangId(), SPPS_Unknown, NULL);
		}

		//restore the pronounciation of the new word to User's lexicon 
		for(psppron = spPronList.pFirstWordPronunciation; psppron && SUCCEEDED(hr) && (hr != SP_ALREADY_IN_LEX); psppron = psppron->pNextWordPronunciation)
		{
			hr = cpConlexicon -> AddPronunciation(dsWord, psppron->LangID, psppron->ePartOfSpeech, psppron->szPronunciation );
		}
		::CoTaskMemFree(spPronList.pvBuffer);

        cpConlexicon.Release();

        hr = RemoveTestAppLexicon(L"SRCompliance;AppLexTest");

	}
};

enum UserlexTestType
{
    ItUserLexSynchBeforeDictLoad,
	ItUserLexSynchBeforeCfgLoad,
    ItUserLexSynchAfterDictLoad,
    ItUserLexSynchAfterCfgLoad,
};

int HlpLexiconTest(UserlexTestType lt)
{
	int tpr = TPR_PASS;
	HRESULT hr = S_OK;
	
	// it is assumed that this work is in no vendors lexicon and the pronunciation for it is unigue
	WCHAR wszNewWord[MAX_PATH] = L"";
  
	CComPtr<ISpLexicon>					cpLexicon;
	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpObjectToken>				cpProfileToken;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;
	CComPtr<ISpStream>  				cpStream;
    UINT                                idWavFile;
	RESULTVECTOR vector_RecoResult;

	// get the word from the resource file
    switch (lt)
    {
    case ItUserLexSynchBeforeDictLoad :
	case ItUserLexSynchBeforeCfgLoad:
	    GetWStrFromRes(IDS_RECO_SYNCH_BEFORE_LOAD, wszNewWord);
        idWavFile = IDS_WAV_SYNCH_BEFORE_LOAD;
        break;
    case ItUserLexSynchAfterDictLoad :
	    GetWStrFromRes(IDS_RECO_SYNCH_AFTER_DICT, wszNewWord);
        idWavFile = IDS_WAV_SYNCH_AFTER_DICT;
        break;
    case ItUserLexSynchAfterCfgLoad :
	    GetWStrFromRes(IDS_RECO_SYNCH_AFTER_GRAM, wszNewWord);
        idWavFile = IDS_WAV_SYNCH_AFTER_GRAM;
        break;
    }
	

	// create a new token object
	if (SUCCEEDED(hr))
		hr = SpCreateNewToken(SPCAT_RECOPROFILES, L"LexProfile", &cpProfileToken);
	
	// create a random user name
	if (SUCCEEDED(hr))
    {
		WCHAR wszName[MAX_PATH] = L"";
		srand(GetCurrentThreadId() + GetTickCount());
		UINT uiRandom = rand();
		swprintf(wszName, L"TestName:%d", uiRandom);

		// set token to this new user
		SpSetDescription(cpProfileToken, wszName);
	}

	// add word to lexicon
    if (lt == ItUserLexSynchBeforeDictLoad || lt == ItUserLexSynchBeforeCfgLoad)
    {
	    // Get the lexicon
        if (SUCCEEDED(hr))
		    hr = cpLexicon.CoCreateInstance(CLSID_SpLexicon);

	    if (SUCCEEDED(hr))
		    hr = HlpAddCustomWord(cpLexicon, wszNewWord);
    }

	// load the engine and make sure new word is recognized
	// create engine
	if (SUCCEEDED(hr))
		hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// point to our user
	if (SUCCEEDED(hr))
        hr = cpRecognizer->SetRecoProfile(cpProfileToken);

	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(idWavFile, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

    // load and activate the default grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(GRAM_ID, &cpGrammar);

    if ((lt == ItUserLexSynchBeforeDictLoad) || (lt == ItUserLexSynchAfterDictLoad))
    {
        if (SUCCEEDED(hr))
            hr = cpGrammar->LoadDictation(NULL, SPLO_STATIC);
    }
    else
    {
        if (SUCCEEDED(hr))
            hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib, 
                (const WCHAR*)MAKEINTRESOURCE(IDR_SNORK_GRAMMAR),
                L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);
    }

	// add word to lexicon
    if ((lt == ItUserLexSynchAfterDictLoad) || (lt == ItUserLexSynchAfterCfgLoad))
    {
	    // Get the lexicon
        hr = cpLexicon.CoCreateInstance(CLSID_SpLexicon);

	    if (SUCCEEDED(hr))
		    hr = HlpAddCustomWord(cpLexicon, wszNewWord);
    }

	// activate dictation or cfg grammar
    if ((lt == ItUserLexSynchBeforeDictLoad) || (lt == ItUserLexSynchAfterDictLoad))
    {
	    if (SUCCEEDED(hr))
		    hr = cpGrammar->SetDictationState(SPRS_ACTIVE);
    }
    else
    {
	    if (SUCCEEDED(hr))
		    hr = cpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
    }

	// perform SR; get results and see if our word is inside the result string 
    CSpDynamicString dstrText;
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if (hr == S_OK)
	{
	    if (HlpCheckResult(vector_RecoResult, wszNewWord) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
	}

	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

	hr = HlpRemoveCustomWord(cpLexicon, wszNewWord);

	// remove the random user that was created for this test
	if (cpProfileToken && SUCCEEDED(hr))
	{
		hr = cpProfileToken->Remove(NULL);
		if (hr == SPERR_TOKEN_IN_USE)
		{
			//we can't remove the token because SR Engine is using the token
			//we try to set the original default token to be the current token
			CComPtr<ISpObjectToken> cpDefaultProfileToken;
			hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOPROFILES, &cpDefaultProfileToken);
			if (SUCCEEDED(hr))
			{
				hr = cpRecognizer->SetRecoProfile(cpDefaultProfileToken);
			}

			if (SUCCEEDED(hr))
			{
				hr = cpProfileToken->Remove(NULL);
			}
		}
		else if (FAILED(hr))
		{
			Comment(IDS_ERR_LEX_REMOVEUSER);
		}
	}

    if (lt == ItUserLexSynchBeforeDictLoad || lt == ItUserLexSynchAfterDictLoad)
    {
        tpr = (tpr == TPR_PASS) ? TPR_SUPPORTED: TPR_UNSUPPORTED;
    }

	return tpr;
}


TESTPROCAPI t_UserLexSynchBeforeDicLoad(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

    if (!IsMatchingAttribute(L"Dictation"))
    {
        return TPR_UNSUPPORTED;
    }



	// run the test
	int tpr = HlpLexiconTest(ItUserLexSynchBeforeDictLoad);


	return tpr;
}


TESTPROCAPI t_UserLexSynchBeforeCfgLoad(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	// run the test
	int tpr = HlpLexiconTest(ItUserLexSynchBeforeCfgLoad);

	return tpr;
}

TESTPROCAPI t_UserLexSynchAfterDictationLoad(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

    if (!IsMatchingAttribute(L"Dictation"))
    {
        return TPR_FAIL;
    }


	// run the test
	int tpr = HlpLexiconTest(ItUserLexSynchAfterDictLoad);


	return tpr;
}


TESTPROCAPI t_UserLexSynchAfterGrammarLoad(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	// run the test
	int tpr = HlpLexiconTest(ItUserLexSynchAfterCfgLoad);


	return tpr;
}


TESTPROCAPI t_AppLex(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


    int tpr = TPR_PASS;
	HRESULT hr = S_OK;
    CEventList eventList;

	WCHAR wszWord[MAX_PATH] = L"";
	WCHAR wszSymbol[MAX_PATH] = L"";

	GetWStrFromRes(IDS_APPLEX_WORD, wszWord);
	GetWStrFromRes(IDS_APPLEX_PROP, wszSymbol);

	CAppLexicon LexObject(wszWord, wszSymbol, NULL, &hr);

	if (hr == SPERR_APPLEX_READ_ONLY)
	{
		Comment(L"app lexicon already exists as read only, can't add new word.");
		return TPR_FAIL;
	}
    else if (FAILED(hr))
    {
		Comment(L"other app lexicon setup failure.");
		return TPR_FAIL;
    }

	hr = HlpGatherEvents(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION),
        IDS_WAV_APPLEX, eventList, IDR_SNORK_GRAMMAR);


    CEventRecord *peRecognition = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_RECOGNITION:
				    peRecognition = p;
					break;
				case SPEI_END_SR_STREAM:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peRecognition)
		{
			Comment(IDS_ERR_RECOGNITION_NOEVENT);
			tpr = TPR_FAIL;
		}
	}
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

    eventList.Release();

	return tpr;
}


TESTPROCAPI t_UserLexBeforeAppLex(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


    int tpr = TPR_PASS;
	HRESULT hr = S_OK;
    CEventList eventList;

	WCHAR wszWord[MAX_PATH] = L"";
	WCHAR wszUserSymbol[MAX_PATH] = L"";
	WCHAR wszAppSymbol[MAX_PATH] = L"";

	GetWStrFromRes(IDS_USERLEXBEFOREAPPLEX_WORD, wszWord);
	GetWStrFromRes(IDS_USERLEXBEFOREAPPLEX_USERPROP, wszUserSymbol);
	GetWStrFromRes(IDS_USERLEXBEFOREAPPLEX_APPPROP, wszAppSymbol);

	CAppLexicon LexObject(wszWord, wszAppSymbol, wszUserSymbol, &hr);

	if (FAILED(hr))
	{
		Comment(L"app lexicon setup failed.");
		return TPR_FAIL;
	}

	hr = HlpGatherEvents(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION),
        IDS_WAV_USERLEXBEFOREAPPLEX, eventList, IDR_SNORK_GRAMMAR);

    CEventRecord *peRecognition = NULL;

    if (hr == S_OK)
    {
        for (CEventRecord *p = eventList.m_pHead; p != NULL; p = p->m_pNext)
        {
			switch (p->m_eEventId)
			{
				case SPEI_RECOGNITION:
				    peRecognition = p;
					break;
				case SPEI_END_SR_STREAM:
					break;
				default:
					Comment(IDS_UNEXPECTED_EVENT);
					tpr = TPR_FAIL;
			}
		}
		
		if (!peRecognition)
		{
			Comment(IDS_ERR_RECOGNITION_NOEVENT);
			tpr = TPR_FAIL;
		}
	}
	else
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}



	return tpr;
}


// This test uses the P tag data
TESTPROCAPI t_MultipleRecoContext(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


	HRESULT hr = S_OK;
	int	tpr = TPR_PASS;
	bool bSuccess1, bSuccess2;


	// speech
	CComPtr<ISpRecoGrammar>				cpGrammar1;
	CComPtr<ISpRecoGrammar>				cpGrammar2;
	CComPtr<ISpStream>					cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt1;
	CComPtr<ISpRecoContext>				cpRecoCtxt2;

    WCHAR wszWord[MAX_PATH] = L"";
	GetWStrFromRes(IDS_RECO_P_TAG, wszWord);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create first recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt1);

	// create second recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt2);

	// pointer the first reco context at our notify object
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt1->SetNotifyWin32Event();

	// pointer the second reco context at our notify object
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt2->SetNotifyWin32Event();

	// tell the first reco context that we're interested in getting
    // recognition and eos notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt1->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// same as second
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt2->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_MULT_RECO, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
        hr = cpRecognizer->SetInput(cpStream, TRUE);

	// load the first grammar
	if (SUCCEEDED(hr))
        hr = cpRecoCtxt1->CreateGrammar(GRAM_ID, &cpGrammar1);

	if (SUCCEEDED(hr))
        hr = cpGrammar1->LoadCmdFromResource(g_pShellInfo->hLib,
            (const WCHAR*)MAKEINTRESOURCE(IDR_P1_GRAMMAR),
            L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	// load the second grammar
	if (SUCCEEDED(hr))
        hr = cpRecoCtxt2->CreateGrammar(GRAM_ID, &cpGrammar2);

	if (SUCCEEDED(hr))
        hr = cpGrammar2->LoadCmdFromResource(g_pShellInfo->hLib,
            (const WCHAR*)MAKEINTRESOURCE(IDR_P2_GRAMMAR),
            L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	// activate the grammars
    if (SUCCEEDED(hr))
        hr = cpGrammar1->SetRuleState(NULL, NULL, SPRS_ACTIVE);
    if (SUCCEEDED(hr))
        hr = cpGrammar2->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	// wait for EOS
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt1->WaitForNotifyEvent(WAIT_TIME);

	if (SUCCEEDED(hr))
		hr = cpRecoCtxt2->WaitForNotifyEvent(WAIT_TIME);

    bSuccess1 = bSuccess2 = false;

    // process all recognized phrases for first context
    bool bDone = false;
    while (SUCCEEDED(hr) && !bDone)
    {
        CComPtr<ISpRecoResult> pResult;
		CSpEvent event;
		while(event.GetFrom(cpRecoCtxt1) == S_OK)
        {
            if (event.eEventId == SPEI_END_SR_STREAM)
                bDone = true;
            else if (pResult = event.RecoResult())
            {
                CSpDynamicString dstrText;
                if (SUCCEEDED(pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                    &dstrText, NULL)))
			    {
                    if (wcsstr(wszWord, dstrText))
				        bSuccess1 = true;
			    }	
			    pResult.Release();
            }
        }
        cpRecoCtxt1->WaitForNotifyEvent(WAIT_TIME);
    }

	if (SUCCEEDED(hr) && !bSuccess1)
	{
		Comment(IDS_ERR_RESULT_WRONGWORD);
		tpr = TPR_FAIL;
	}

    // process all recognized phrases for second context
    bDone = false;
    while (SUCCEEDED(hr) && !bDone)
    {
        CComPtr<ISpRecoResult> pResult;
		CSpEvent event;
		while(event.GetFrom(cpRecoCtxt2) == S_OK)
        {
            if (event.eEventId == SPEI_END_SR_STREAM)
                bDone = true;
            else if (pResult = event.RecoResult())
            {
                CSpDynamicString dstrText;
                if (SUCCEEDED(pResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                    &dstrText, NULL)))
			    {
				        bSuccess2 = true;
			    }	
			    pResult.Release();
            }
        }
        cpRecoCtxt1->WaitForNotifyEvent(WAIT_TIME);
    }

	if (SUCCEEDED(hr) && !bSuccess2)
	{
		Comment(IDS_ERR_RESULT_WRONGWORD);
		tpr = TPR_FAIL;
	}

	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}


	return tpr;
}


int HlpGrammarTest(UINT idWord, UINT idWavFile, UINT idGrammar, WCHAR* pszRuleName = NULL)
{
    int tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;
	SPSTATEHANDLE hState;

	WCHAR wszWord[MAX_PATH] = L"";
	GetWStrFromRes(idWord, wszWord);
	RESULTVECTOR vector_RecoResult;

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(idWavFile, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

	// load the first grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(GRAM_ID, &cpGrammar);

	if (idGrammar)
	{
		if (SUCCEEDED(hr))
			hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
				(const WCHAR*)MAKEINTRESOURCE(idGrammar),
				L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);
	}
	else
	{
		//Dynamic Grammar
		if (SUCCEEDED(hr))
			hr = cpGrammar->ResetGrammar(::GetDefaultEnginePrimaryLangId());

		if (SUCCEEDED( hr ))
		{
			if (pszRuleName)
			{
				hr = cpGrammar->GetRule(pszRuleName, 0, SPRAF_Dynamic, TRUE, &hState);
			}
			else
			{
				hr = cpGrammar->GetRule(L"color", 0, SPRAF_Dynamic, TRUE, &hState);
			}

		}

		if (SUCCEEDED( hr ))
			hr = cpGrammar->AddWordTransition(hState, NULL, wszWord, L" ", SPWT_LEXICAL, 1.0, NULL);
	}

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(pszRuleName, NULL, SPRS_ACTIVE);

    CSpDynamicString dstrText;
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if (SUCCEEDED(hr))
    {
	    if (HlpCheckResult(vector_RecoResult, wszWord) != S_OK)
        {
            // USES_CONVERSION;
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
    }

	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

	return tpr;
}


TESTPROCAPI t_GrammarListTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


    int tpr = HlpGrammarTest(IDS_RECO_L_TAG, IDS_WAV_L_TAG, IDR_L_GRAMMAR);

	return tpr;
}


TESTPROCAPI t_GrammarExpRuleTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE)
		return TPR_NOT_HANDLED;



	int	tpr = TPR_PASS;
	HRESULT hr = S_OK;

	// words in grammar expected to be recognized
	WCHAR wszFirstRule[MAX_PATH] = L"";
	WCHAR wszSecondRule[MAX_PATH] = L"";

	GetWStrFromRes(IDS_RECO_EXPRULE_FIRSTRULE, wszFirstRule);
	GetWStrFromRes(IDS_RECO_EXPRULE_SECONDRULE, wszSecondRule);

	// speech
	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpStream>			    	cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;
	RESULTVECTOR vector_RecoResult;
	BOOL bFirstRuleReco = FALSE;

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_EXPRULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

	// load the grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(GRAM_ID, &cpGrammar);

	if (SUCCEEDED(hr))
        hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
            (const WCHAR*)MAKEINTRESOURCE(IDR_EXPRULE_GRAMMAR),
            L"SRGRAMMAR",::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	// activate the grammar
    if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(wszFirstRule, NULL, SPRS_ACTIVE);

	// deactivate the second rule
	if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(wszSecondRule, NULL, SPRS_INACTIVE);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if( FAILED(hr) || !vector_RecoResult.size() )
    { 
		Comment(L"failed to get recognition");
		return TPR_FAIL;
    }
    else
    {
		SPPHRASE*  pPhrase = NULL;
		ULONG ulCount = 0;
		hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
		if (SUCCEEDED(hr) && pPhrase)
		{
				if (!wcscmp(pPhrase->Rule.pszName, wszFirstRule))
					bFirstRuleReco = TRUE;
		}

		::CoTaskMemFree(pPhrase);
	}


    if (SUCCEEDED(hr))
    {
	    // The first rule should be used.
	    // The second rule shouldn't be used.
	    if (!bFirstRuleReco)
	    {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
	    }
    }

	// deactivate the rule
	if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(wszFirstRule, NULL, SPRS_INACTIVE);

	// release the input wave file
	if (SUCCEEDED(hr))
		cpStream.Release();

	// use same wave file and make sure that words from first rule are not recognized
	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_EXPRULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

	// activate the other rule
	if (SUCCEEDED(hr))
		hr = cpGrammar->SetRuleState(wszSecondRule, NULL, SPRS_ACTIVE);

	// wait for EOS
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->WaitForNotifyEvent(TESTEVENT_TIMEOUT);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if( FAILED(hr) || !vector_RecoResult.size() )
    { 
		Comment(L"failed to get recognition");
		return TPR_FAIL;
    }
    else
    {
		SPPHRASE*  pPhrase = NULL;
		ULONG ulCount = 0;
		hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
		if (SUCCEEDED(hr) && pPhrase)
		{
				if (!wcscmp(pPhrase->Rule.pszName, wszSecondRule))
					bFirstRuleReco = FALSE;
		}

		::CoTaskMemFree(pPhrase);
	}

    if (SUCCEEDED(hr))
    {
	    // The first rule shouldn't be used.
	    // The second rule should be used.
	    if (bFirstRuleReco)
	    {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
	    }
    }

  	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}

	return tpr;
}


TESTPROCAPI t_GrammarPTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

    int tpr = HlpGrammarTest(IDS_RECO_P_TAG, IDS_WAV_P_TAG, IDR_P1_GRAMMAR);
		
	return tpr;
}


TESTPROCAPI t_GrammarOTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

	// Message check
    if (uMsg != TPM_EXECUTE)
		return TPR_NOT_HANDLED;


	USES_CONVERSION;
	int	tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpGrammar;
	CComPtr<ISpStream>			    	cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoCtxt;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszFirstOptionalWord[MAX_PATH] = L"";
	WCHAR wszWord[MAX_PATH] = L"";
	WCHAR wszSecondOptionalWord[MAX_PATH] = L"";

	GetWStrFromRes(IDS_RECO_O_TAG_1, wszFirstOptionalWord);
	GetWStrFromRes(IDS_RECO_O_TAG_2, wszWord);
	GetWStrFromRes(IDS_RECO_O_TAG_3, wszSecondOptionalWord);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create first recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoCtxt);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoCtxt->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_O_TAG_1, &cpStream,  NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

	// load the grammar
    if (SUCCEEDED(hr))
        hr = cpRecoCtxt->CreateGrammar(GRAM_ID, &cpGrammar);

	if (SUCCEEDED(hr))
        hr = cpGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
            (const WCHAR*)MAKEINTRESOURCE(IDR_O_GRAMMAR),
            L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

    // perform SR; get results and compare to our word
 	if (SUCCEEDED(hr))
        hr = cpGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if (SUCCEEDED(hr))
	    if (HlpCheckResult(vector_RecoResult, wszFirstOptionalWord) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }

	// part 2: check that second optional word is recognized
	if (SUCCEEDED(hr))
		cpStream.Release();

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_O_TAG_2, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

    // perform SR; get results and compare to our word
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if (SUCCEEDED(hr))
	    if (HlpCheckResult(vector_RecoResult, wszSecondOptionalWord) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }

	// part 3: check that only main word is recognized
	if (SUCCEEDED(hr))
		cpStream.Release();

	// open wav file
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_O_TAG_3, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);

    // perform SR; get results and compare to our word
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoCtxt, vector_RecoResult);

    if (SUCCEEDED(hr))
	    if (HlpCheckResult(vector_RecoResult, wszWord) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }

	// end test
	if (FAILED(hr))
	{
		Comment(IDS_UNEXPECTED_ERR);
		tpr = TPR_FAIL;
	}
		
	return tpr;
}


TESTPROCAPI t_GrammarRuleTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;

    int tpr = HlpGrammarTest(IDS_RECO_RULE_TAG, IDS_WAV_RULE_TAG, IDR_RULE_GRAMMAR);
		
	return tpr;
}






TESTPROCAPI t_Alternates_Dictation(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    if (!IsMatchingAttribute(L"Alternates", L"Dictation"))
    {
        return TPR_UNSUPPORTED;
    }


    HRESULT hr;
    int tpr = TPR_SUPPORTED;
    CComPtr<ISpRecognizer> cpRecognizer;
    CComPtr<ISpRecoContext>  cpRecoContext;

    CComPtr<ISpRecoResult>  cpRecoResult;
    CComPtr<ISpRecoGrammar> cpRecoGrammar;
    CComPtr<ISpStream>                cpStream;
	CComPtr<ISpPhraseAlt> cpPhraseAlt;
	RESULTVECTOR vector_RecoResult;

    hr = cpRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer );

	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	hr = OpenWavFile(IDS_WAV_EXPRULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_UNSUPPORTED;

	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->LoadDictation(NULL, SPLO_STATIC);
	}

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoContext->SetNotifyWin32Event();
    }

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoContext->SetInterest(SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM), 
																		 SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
    }

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoGrammar->SetDictationState(SPRS_ACTIVE);
    }




 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if( FAILED(hr) || !vector_RecoResult.size() )
    { 
		Comment(L"failed to get recognition");
		return TPR_UNSUPPORTED;
    }
    else
    {
		SPPHRASE*  pPhrase = NULL;
		ULONG ulCount = 0;
		hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
		if (SUCCEEDED(hr) && pPhrase)
		{
			hr = vector_RecoResult[0]->GetAlternates(pPhrase->Rule.ulFirstElement, 
											 pPhrase->Rule.ulCountOfElements, 
											 1,
											 &cpPhraseAlt,
											 &ulCount);
			if (hr== S_OK && cpPhraseAlt && ulCount ==1)
			{
				CSpDynamicString dsString;
				hr = cpPhraseAlt->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                &dsString, NULL);
				if (SUCCEEDED(hr))
				{
					hr = cpPhraseAlt->Commit();
				}
			}
			::CoTaskMemFree(pPhrase);
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
		}
	}

	if (FAILED(hr))
	{
		tpr = TPR_UNSUPPORTED;
	}


    return tpr;
}

TESTPROCAPI t_Alternates_Cfg(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    if (!IsMatchingAttribute(L"Alternates", L"CC"))
    {
        return TPR_UNSUPPORTED;
    }

    HRESULT hr;
    int tpr = TPR_SUPPORTED;
    CComPtr<ISpRecognizer> cpRecognizer;
    CComPtr<ISpRecoContext>  cpRecoContext;

    CComPtr<ISpRecoResult>  cpRecoResult;
    CComPtr<ISpRecoGrammar> cpRecoGrammar;
    CComPtr<ISpStream>                cpStream;
	CComPtr<ISpPhraseAlt> cpPhraseAlt;

	WCHAR wszAlternates[3][MAX_PATH] = {L""};
	WCHAR wszWords[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";
	RESULTVECTOR vector_RecoResult;

	SPSTATEHANDLE hInit;
	SPSTATEHANDLE hAfterAlternates;

	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_ALTERNATESCFG_BESTWORD, wszAlternates[0]);
	GetWStrFromRes(IDS_ALTERNATESCFG_ALTERNATE1, wszAlternates[1]);
	GetWStrFromRes(IDS_ALTERNATESCFG_ALTERNATE2, wszAlternates[2]);
	GetWStrFromRes(IDS_ALTERNATESCFG_WORDS, wszWords);
	GetWStrFromRes(IDS_ALTERNATESCFG_RULE, wszRule);

    hr = cpRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer );

	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	hr = OpenWavFile(IDS_WAV_ALTERMATESCFG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetMaxAlternates(1);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hInit);
			if (FAILED(hr))
			{
					Comment(L"create the first dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->CreateNewState(hInit, &hAfterAlternates);
	}

	for (ULONG ul=0; ul<sizeof(wszAlternates)/sizeof(wszAlternates[0]); ul++)
	{
		if (SUCCEEDED(hr))
		{
				hr = cpRecoGrammar->AddWordTransition(hInit, hAfterAlternates, wszAlternates[ul] , L" ", SPWT_LEXICAL, 1.0, NULL);
		}
	}


	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hAfterAlternates, NULL, wszWords , L" ", SPWT_LEXICAL, 1.0, NULL);
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->Commit(NULL);
	}

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoContext->SetNotifyWin32Event();
    }

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoContext->SetInterest(SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM), 
																		 SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
    }

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
    }

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if( FAILED(hr) || !vector_RecoResult.size() )
    { 
		Comment(L"failed to get recognition");
		return TPR_UNSUPPORTED;
    }
    else
    {
		SPPHRASE*  pPhrase = NULL;
		ULONG ulCount = 0;
		hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
		if (SUCCEEDED(hr) && pPhrase)
		{
			hr = vector_RecoResult[0]->GetAlternates(pPhrase->Rule.ulFirstElement, 
											 pPhrase->Rule.ulCountOfElements, 
											 1,
											 &cpPhraseAlt,
											 &ulCount);
			if (hr== S_OK && cpPhraseAlt && ulCount ==1)
			{
				CSpDynamicString dsString;
				hr = cpPhraseAlt->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                &dsString, NULL);
				if (SUCCEEDED(hr))
					hr = cpPhraseAlt->Commit();
			}
			else
			{
				tpr = TPR_UNSUPPORTED;
			}
			::CoTaskMemFree(pPhrase);
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
		}
	}

		if (FAILED(hr))
		{
			tpr = TPR_UNSUPPORTED;
			Comment(L"Alternate failed.");
		}




    return tpr;
}

TESTPROCAPI t_PickGrammar(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpFirstGrammar;
	CComPtr<ISpRecoGrammar>				cpSecondGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;

	WCHAR wszWord[MAX_PATH] = L"";
	GetWStrFromRes(IDS_RECO_RULE_TAG, wszWord);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);

	// get event notification
	if (SUCCEEDED(hr))
		hr = cpRecoContext->SetNotifyWin32Event();

	// specify events we want to see
	if (SUCCEEDED(hr))
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_RULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);



	// We pause so that we can load the grammar atomically.
	if (SUCCEEDED(hr))
		hr = cpRecoContext->Pause(NULL);

	// load the first grammar
    if (SUCCEEDED(hr))
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpFirstGrammar);

	if (SUCCEEDED(hr))
		hr = cpFirstGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
			(const WCHAR*)MAKEINTRESOURCE(IDR_L_GRAMMAR),
			L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpFirstGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	// load the second grammar
    if (SUCCEEDED(hr))
        hr = cpRecoContext->CreateGrammar(GRAM_ID+1, &cpSecondGrammar);

	if (SUCCEEDED(hr))
	hr = cpSecondGrammar->LoadCmdFromResource(g_pShellInfo->hLib,
		(const WCHAR*)MAKEINTRESOURCE(IDR_RULE_GRAMMAR),
		L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpSecondGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	// We resume after we finish.
	if (SUCCEEDED(hr))
		hr = cpRecoContext->Resume(NULL);




    while ((hr = cpRecoContext->WaitForNotifyEvent(TESTEVENT_TIMEOUT)) == S_OK)
	{
        CSpEvent event;
        while ((hr = event.GetFrom(cpRecoContext)) == S_OK)
		{
			if (event.eEventId == SPEI_RECOGNITION)
			{
				cpRecoResult = event.RecoResult();
			}
		}
    }

    if( !cpRecoResult )
    { 
        hr = E_FAIL;
    }
    else
    {
		CSpDynamicString dsString;
		hr = cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, 
            &dsString, NULL);
		if (SUCCEEDED(hr) && wcsstr(dsString, wszWord))
		{
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
			Comment(L"The first word is not right.");
		}
	}
	cpRecoResult.Release();


	return tpr;

}



TESTPROCAPI t_UseLastActivatedGrammar(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE)
		return TPR_NOT_HANDLED;


    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpGrammar[NUMOFRECOCONTEXTS];
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext[NUMOFRECOCONTEXTS];
	CComPtr<ISpRecoResult>				cpRecoResult;
	BOOL fSuccess[NUMOFRECOCONTEXTS];

	WCHAR wszWord[MAX_PATH] = L"";
	GetWStrFromRes(IDS_RECO_RULE_TAG, wszWord);
	ULONG ul;



	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// open wav file, it is correspondent to IDR_RULE_GRAMMAR.
	if (SUCCEEDED(hr))
		hr = OpenWavFile(IDS_WAV_RULE_TAG, &cpStream, NULL);

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
		hr = cpRecognizer->SetInput(cpStream, TRUE);




    if (SUCCEEDED(hr))
    {
	    for (ul=0; ul < NUMOFRECOCONTEXTS; ul++)
	    {
		    fSuccess[ul] = TRUE;
		    // create recognition context
		    if (SUCCEEDED(hr))
			    hr = cpRecognizer->CreateRecoContext(&cpRecoContext[ul]);
	    }
    }

	// We pause so that we can load the grammar atomically.
	if (SUCCEEDED(hr))
		hr = cpRecoContext[0]->Pause(NULL);

    if (SUCCEEDED(hr))
    {
	    for (ul=0; ul < NUMOFRECOCONTEXTS; ul++)
	    {

		    // get event notification
		    if (SUCCEEDED(hr))
			    hr = cpRecoContext[ul]->SetNotifyWin32Event();

		    // specify events we want to see
		    if (SUCCEEDED(hr))
			    hr = cpRecoContext[ul]->SetInterest(
				    SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
				    SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));

		    // load the first grammar
		    if (SUCCEEDED(hr))
			    hr = cpRecoContext[ul]->CreateGrammar(GRAM_ID+ul, &cpGrammar[ul]);

		    if (SUCCEEDED(hr))
			    hr = cpGrammar[ul]->LoadCmdFromResource(g_pShellInfo->hLib,
				    (const WCHAR*)MAKEINTRESOURCE(IDR_RULE_GRAMMAR),
				    L"SRGRAMMAR", ::GetDefaultEnginePrimaryLangId(), SPLO_STATIC);

		    // perform SR; get results and see if our word is inside the result string 
 		    if (SUCCEEDED(hr))
			    hr = cpGrammar[ul]->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	    }
    }

	// We resume after we finish.
	if (SUCCEEDED(hr))
		hr = cpRecoContext[0]->Resume(NULL);



    if (SUCCEEDED(hr))
    {
	    for (ul = 0; ul < NUMOFRECOCONTEXTS; ul++)
	    {

		    while ((hr = cpRecoContext[ul]->WaitForNotifyEvent(TESTEVENT_TIMEOUT)) == S_OK)
		    {
			    CSpEvent event;
			    while ((hr = event.GetFrom(cpRecoContext[ul])) == S_OK)
			    {
				    if (event.eEventId == SPEI_RECOGNITION)
				    {
					    if (ul != NUMOFRECOCONTEXTS - 1)
						    fSuccess[ul] = FALSE;
					    else
						    cpRecoResult = event.RecoResult();
				    }
			    }
		    }
	    }
    }

    if( !cpRecoResult )
    { 
        tpr = TPR_UNSUPPORTED;
    }
    else
    {
		CSpDynamicString dsString;
		hr = cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, 
            &dsString, NULL);
		if (SUCCEEDED(hr) && wcsstr(dsString, wszWord))
		{
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
			Comment(L"The first word is not right.");
		}
	}
	cpRecoResult.Release();

	for (ul=0; ul<NUMOFRECOCONTEXTS; ul++)
	{
		if (!fSuccess[ul])
			tpr = TPR_UNSUPPORTED;
	}

	return tpr;

}


TESTPROCAPI t_RequiredPropertyString(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE)
		return TPR_NOT_HANDLED;



    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

    CComPtr<ISpRecognizer>			cpRecognizer;

	WCHAR wszPropertyName[] = L"Non support property string"; //No engine is going to support this property.
	WCHAR wszPropertyValue[] = L"Non support property value";


	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	hr = cpRecognizer->SetPropertyString(wszPropertyName, wszPropertyValue);
	if (hr == S_FALSE)
	{
		CSpDynamicString dsString;
		hr = cpRecognizer->GetPropertyString(wszPropertyName, &dsString);
		if (hr == S_FALSE)
		{
			tpr = TPR_SUPPORTED;
		}
		else
		{
			tpr = TPR_UNSUPPORTED;
		}
	}
	else
	{
		tpr = TPR_UNSUPPORTED;
	}
	
	return tpr;
}

TESTPROCAPI t_RequiredPropertyNum(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{

    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

    CComPtr<ISpRecognizer>			cpRecognizer;

	BOOL fSuccess = TRUE;

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	for (ULONG ul =0; ul <sizeof(PropertyNumTable)/sizeof(PropertyNumTable[0]); ul++)
	{
		LONG lValueSet = (PropertyNumTable[ul].ulLowLimit + PropertyNumTable[ul].ulHighLimit)/2;
		hr = cpRecognizer->SetPropertyNum(PropertyNumTable[ul].pPropertyNumNames, lValueSet);
		if (hr == S_OK)
		{
			LONG lReturnNum = 0;
			WCHAR pwszResult[MAX_PATH] = L"";
			hr = cpRecognizer->GetPropertyNum(PropertyNumTable[ul].pPropertyNumNames, &lReturnNum);
			if (lReturnNum == lValueSet)
			{
				swprintf(pwszResult, L"SetPropertyNum for %s is successful", PropertyNumTable[ul].pPropertyNumNames);
				Comment(pwszResult);
			}
			else
			{
				swprintf(pwszResult, L"SetPropertyNum for %s fails", PropertyNumTable[ul].pPropertyNumNames);
				Comment(pwszResult);
				fSuccess = FALSE;
			}
		}
	}
	
	if (!fSuccess)
	{
		tpr = TPR_UNSUPPORTED;
	}

	return tpr;
}


HRESULT HlpClearandAddTextBuffer(CComPtr<ISpRecognizer>& cpRecognizer, CComPtr<ISpStream>& cpStream, CComPtr<ISpRecoGrammar>& cpRecoGrammar, SPSTATEHANDLE hInit, WCHAR* wszTextBuffer, ULONG ulSize, SPTEXTSELECTIONINFO* pts = NULL)
{
    HRESULT hr = S_OK;

	LARGE_INTEGER li;
	li.QuadPart = 0;
	hr = cpStream->Seek(li, STREAM_SEEK_SET, NULL);

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

    if (SUCCEEDED(hr))
    {
        hr = cpRecoGrammar->ClearRule(hInit);
    }

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddRuleTransition(hInit, NULL, SPRULETRANS_TEXTBUFFER, 1.0, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->Commit(NULL);
	}

    if (SUCCEEDED(hr))
    {
        hr = cpRecoGrammar->SetWordSequenceData(wszTextBuffer, ulSize, pts);
    }


	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

    return hr;
}

TESTPROCAPI t_CFGTextBuffer(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    if (!IsMatchingAttribute(L"WordSequences"))
    {
        return TPR_UNSUPPORTED;
    }

    HRESULT hr;
    int tpr = TPR_SUPPORTED;
    CComPtr<ISpRecognizer> cpRecognizer;
    CComPtr<ISpRecoContext>  cpRecoContext;

    CComPtr<ISpRecoResult>  cpRecoResult;
    CComPtr<ISpRecoGrammar> cpRecoGrammar;
    CComPtr<ISpStream>                cpStream;
	WCHAR wszExpectedResult[MAX_PATH] = L"";
	WCHAR wszFirstPart[MAX_PATH] = L"";
	WCHAR wszSecondPart[MAX_PATH] = L"";
    WCHAR wszNotInterestedPart[MAX_PATH] = L"";
    WCHAR wszLongWords[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";
	RESULTVECTOR vector_RecoResult;

	SPSTATEHANDLE hInit;
	SPSTATEHANDLE hBeforeTextBuffer;

    SPTEXTSELECTIONINFO ts;
    ZeroMemory(&ts, sizeof(ts));


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_CFGTEXTBUFFER_WORDS, wszFirstPart);
	GetWStrFromRes(IDS_CFGTEXTBUFFER_BUFFERWORD, wszSecondPart);
	GetWStrFromRes(IDS_CFGTEXTBUFFER_RULE, wszRule);
    GetWStrFromRes(IDS_CFGTEXTBUFFER_NOTINTERESTEDWORDS, wszNotInterestedPart);

	wcscpy(wszExpectedResult, wszFirstPart);
	wcscat(wszExpectedResult, wszSecondPart);


    hr = cpRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer );

	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_CFGTEXTBUFFER, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}
	
	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}


    //Scenario 1, whole buffer
   Comment(L"Start scenario 1, whole buffer");
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hInit);
			if (FAILED(hr))
			{
					Comment(L"create dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->CreateNewState(hInit, &hBeforeTextBuffer);
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hInit, hBeforeTextBuffer, wszFirstPart , L" ", SPWT_LEXICAL, 1.0, NULL);
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddRuleTransition(hBeforeTextBuffer, NULL, SPRULETRANS_TEXTBUFFER, 1.0, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->Commit(NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetWordSequenceData(wszSecondPart, wcslen(wszSecondPart) + 2, NULL);
	}

	if (SUCCEEDED(hr))
		hr = cpRecoContext->SetNotifyWin32Event();

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);


 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK )
	{
        if (HlpCheckResult(vector_RecoResult, wszExpectedResult) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_UNSUPPORTED;
        }
        else
        {
            Comment(L"pass");
        }
	}
    else
    {
        tpr = TPR_UNSUPPORTED;
        Comment(L"No recognition");
    }
   Comment(L"End scenario 1");



    //Scenario 2, no recognition across different groups
   Comment(L"Start scenario 2, no recognition across different groups");
    wcscpy(wszFirstPart + wcslen(wszFirstPart) + 1,  wszSecondPart);

    hr = HlpClearandAddTextBuffer(cpRecognizer, cpStream, cpRecoGrammar, hInit, wszFirstPart, wcslen(wszFirstPart) + wcslen(wszSecondPart) + 3);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK && HlpCheckResult(vector_RecoResult, wszExpectedResult) == S_OK)
    {
        Comment(L"failed");
        tpr = TPR_UNSUPPORTED;
    }
    else
    {
        //no receiving any result is compliant
       Comment(L"pass");
        hr = S_OK;
    }
   Comment(L"End start scenario 2");



    //Scenario 3, the expected result is a subset of the second group in the text buffer
   Comment(L"Start scenario 3, the expected result is a subset of the second group in the text buffer");
    wcscpy(wszLongWords, wszNotInterestedPart);
    wcscat(wszLongWords, wszFirstPart);
    wcscat(wszLongWords, wszSecondPart);

    wcscpy(wszNotInterestedPart + wcslen(wszNotInterestedPart) + 1,  wszLongWords);

    hr = HlpClearandAddTextBuffer(cpRecognizer, cpStream, cpRecoGrammar, hInit, wszNotInterestedPart, wcslen(wszNotInterestedPart) + wcslen(wszLongWords) + 3);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
        if (HlpCheckResult(vector_RecoResult, wszExpectedResult) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_UNSUPPORTED;
        }
        else
        {
            Comment(L"pass");
        }
	}
    else
    {
	    tpr = TPR_UNSUPPORTED;
        Comment(L"No recognition");
    }
   Comment(L"End scenario 3");



    //Scenario 4, no recognition for noncontinous words
   Comment(L"Start scenario 4, no recognition for noncontinous words");
    wcscpy(wszLongWords, wszFirstPart);
    wcscat(wszLongWords, wszNotInterestedPart);
    wcscat(wszLongWords, wszSecondPart);

    hr = HlpClearandAddTextBuffer(cpRecognizer, cpStream, cpRecoGrammar, hInit, wszLongWords, wcslen(wszLongWords) + 2);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK && HlpCheckResult(vector_RecoResult, wszExpectedResult) == S_OK)
    {
        Comment(L"Noncontinous words shouldn't be recognized");
        tpr = TPR_UNSUPPORTED;
    }
    else
    {
        //no receiving any result is compliant
       Comment(L"pass");
        hr = S_OK;
    }
   Comment(L"End scenario 4");


    //Scenario 5, only words in textselection are recognizable
   Comment(L"Start scenario 5, only words in textselection are recognizable");
    wcscpy(wszLongWords, wszFirstPart);
    wcscat(wszLongWords, wszSecondPart);


    //only the second part is active
    ts.ulStartActiveOffset = wcslen(wszFirstPart);
    ts.cchActiveChars = wcslen(wszSecondPart);

    hr = HlpClearandAddTextBuffer(cpRecognizer, cpStream, cpRecoGrammar, hInit, wszLongWords, wcslen(wszLongWords) + 2, &ts);

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK && HlpCheckResult(vector_RecoResult, wszExpectedResult) == S_OK)
    {
        Comment(L"Words not in the active textselection shouldn't be recognized");
        tpr = TPR_UNSUPPORTED;
    }
    else
    {
        //no receiving any result is compliant
       Comment(L"pass");
        hr = S_OK;
    }
   Comment(L"End scenario 5");


	if (FAILED(hr))
	{
		tpr = TPR_UNSUPPORTED;
	}







    return tpr;
}

void RunAndReport(TESTPROC pTest, WCHAR* pszLog, int* ptpr)
{

    WCHAR pwszThreadId[MAX_PATH] = L"";
    swprintf(pwszThreadId, L"Thread Id: %x ", ::GetCurrentThreadId());
    CSpDynamicString dsResult;
    dsResult.Clear();
    dsResult.Append(pwszThreadId);
    dsResult.Append(pszLog);
    if (*ptpr != TPR_PASS && *ptpr != TPR_SKIP)
    {
        dsResult.Append(L" skipped as failure in the previous test");
    }
    else
    {
        *ptpr = pTest(TPM_EXECUTE, NULL, NULL);
        dsResult.Append((*ptpr == TPR_PASS) ? L" pass" : (*ptpr == TPR_SKIP) ? L" skip" : L" fail");
    }
	Comment(dsResult);

}



TESTPROCAPI t_MultiInstances(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	int	tpr = TPR_PASS;
	HRESULT hr = S_OK;
    CSpDynamicString dsResult;

	if (uMsg == TPM_QUERY_THREAD_COUNT)
	{
		((TPS_QUERY_THREAD_COUNT*)tpParam)->dwThreadCount = 3;
		return TPR_HANDLED;
	}

	// Message check
    if (uMsg != TPM_EXECUTE) 
	{ 
		return TPR_NOT_HANDLED;
    }
	else
	{
        srand(reinterpret_cast<LPTPS_EXECUTE>(tpParam)->dwRandomSeed);
    }


	UINT uiCaseWeight[] = {
									1,	// t_CheckEvent_SoundStart
									1,	// t_CheckEvent_SoundEnd
									1,	// t_CheckEvent_PhraseStart
									1,	// t_CheckEvent_Recognition
									1,	// t_UserLexSynchBeforeCfgLoad
									1,	// t_MultipleRecoContext
									1,	// t_GrammarPTag
									1,	// t_InvalidateToplevelRule
									0	// t_GrammarOTag
	};

	
	
	
	// scale Weight array
	for(UINT uiCount=0; uiCount<(sizeof(uiCaseWeight)/sizeof(uiCaseWeight[0]) - 1); uiCount++)
	{
		uiCaseWeight[uiCount+1] += uiCaseWeight[uiCount];
	}

	UINT uiRand; 
	for(uiCount=0; uiCount<sizeof(uiCaseWeight)/sizeof(uiCaseWeight[0]) - 1; uiCount++)
	{
		uiRand = rand() % uiCaseWeight[sizeof(uiCaseWeight)/sizeof(uiCaseWeight[0]) - 1];

		if (uiRand < uiCaseWeight[0])
		{
            RunAndReport(t_CheckEvent_SoundStart,  L"SoundStart Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[0] && uiRand < uiCaseWeight[1])
		{
            RunAndReport(t_CheckEvent_SoundEnd,  L"SoundEnd Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[1] && uiRand < uiCaseWeight[2])
		{
            RunAndReport(t_CheckEvent_PhraseStart,  L"PhraseStart Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[2] && uiRand < uiCaseWeight[3])
		{
            RunAndReport(t_CheckEvent_Recognition,  L"Recognition Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[3] && uiRand < uiCaseWeight[4])
		{
//            RunAndReport(t_UserLexSynchBeforeCfgLoad,  L"t_UserLexSynchBeforeCfgLoad Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[4] && uiRand < uiCaseWeight[5])
		{
            RunAndReport(t_MultipleRecoContext,  L"MultipleRecoContext Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[5] && uiRand < uiCaseWeight[6])
		{
            RunAndReport(t_GrammarPTag,  L"GrammarPTag Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[6] && uiRand < uiCaseWeight[7])
		{
//            RunAndReport(t_InvalidateToplevelRule,  L"InvalidateToplevelRule Test", &tpr);
		}

		if (uiRand >= uiCaseWeight[7] && uiRand < uiCaseWeight[8])
		{
            RunAndReport(t_GrammarOTag,  L"GrammarOTag Test", &tpr);
		}
	
	}
	
	return tpr;
}


TESTPROCAPI t_AutoPause(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;

	WCHAR wszFirstWord[MAX_PATH] = L"";
	WCHAR wszSecondWord[MAX_PATH] = L"";
	WCHAR wszFirstRule[MAX_PATH] = L"";
	WCHAR wszSecondRule[MAX_PATH] = L"";

	SPSTATEHANDLE hFirstInit;
	SPSTATEHANDLE hSecondInit;

    BOOL                     fFirstReco = FALSE;
    BOOL                     fSecondReco = FALSE;


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_AUTOPAUSE_DYNAMICWORD1, wszFirstWord);
	GetWStrFromRes(IDS_AUTOPAUSE_DYNAMICWORD2, wszSecondWord);
	GetWStrFromRes(IDS_AUTOPAUSE_DYNAMICRULE1, wszFirstRule);
	GetWStrFromRes(IDS_AUTOPAUSE_DYNAMICRULE2, wszSecondRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_AUTOPAUSE, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;
		

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//first rule
    if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->GetRule(wszFirstRule, 0, SPRAF_TopLevel, TRUE, &hFirstInit);
		if (FAILED(hr))
		{
				Comment(L"create the first dynamic rule failed");
		}
	}


	//second rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszSecondRule, 0, SPRAF_TopLevel, TRUE, &hSecondInit);
			if (FAILED(hr))
			{
					Comment(L"create the second dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hFirstInit, NULL, wszFirstWord , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L" AddWordTransition on first dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hSecondInit, NULL, wszSecondWord , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L"AddWordTransition on the second dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(wszFirstRule, NULL, SPRS_ACTIVE_WITH_AUTO_PAUSE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}

	while((!fEndofStream)&&(hr = cpRecoContext->WaitForNotifyEvent(TESTEVENT_TIMEOUT)) == S_OK)
	{
        CSpEvent event;
        while((hr = event.GetFrom(cpRecoContext)) == S_OK)
		{
			switch (event.eEventId)
			{
			case SPEI_RECOGNITION:
				{
					cpRecoResult = event.RecoResult();
					//We get the auto pause flag and call resume so that engine can continue to process the audio

					CComPtr<ISpRecoResult> cpRecoResult = event.RecoResult();
					if (cpRecoResult)
					{
						CSpDynamicString dsText;
						cpRecoResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dsText, NULL);
						if (!wcscmp(dsText, wszFirstWord))
						{
								fFirstReco = TRUE;
						}
						else if (!wcscmp(dsText, wszSecondWord))
						{
								fSecondReco = TRUE;
								break;
						}
					}
					if (event.IsPaused())
					{
						hr = cpRecoGrammar->SetRuleState(wszFirstRule, NULL, SPRS_INACTIVE);
						if (SUCCEEDED(hr))
						{
							//activate the second rule
							hr = cpRecoGrammar->SetRuleState( wszSecondRule, NULL, SPRS_ACTIVE);
							if (SUCCEEDED(hr))
									hr = cpRecoContext->Resume(NULL);
						}
					}
					break;
				}
			case SPEI_END_SR_STREAM:
				fEndofStream = TRUE;
			}
		}
	}

	if (!(fFirstReco && fSecondReco))
	{
		tpr = TPR_FAIL;
	}

	return tpr;

}


TESTPROCAPI t_InvalidateToplevelRule(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszWords[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";

	SPSTATEHANDLE hToplevelInit;

    BOOL                     fFirstReco = FALSE;
    BOOL                     fSecondReco = FALSE;


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_INVALIDATETOPLEVEL_DYNAMICWORDS, wszWords);
	GetWStrFromRes(IDS_INVALIDATETOPLEVEL_DYNAMICRULE, wszRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_INVALIDATETOPLEVEL_OLD, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}
	
	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWords , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L" AddWordTransition on first dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}



	/************************************************
	//It is the first rule and using the first wave file before we invalidate the rule by reloading the same rule with different words.
	************************************************/
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
	    if (HlpCheckResult(vector_RecoResult, wszWords) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->ClearRule(hToplevelInit);
	}

	if (SUCCEEDED(hr))
	{
		GetWStrFromRes(IDS_INVALIDATETOPLEVEL_DYNAMICNEWWORDS, wszWords);
		hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWords , L" ", SPWT_LEXICAL, 1.0, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->Commit(NULL);
	}

	if (SUCCEEDED(hr))
	{
		LARGE_INTEGER li;
		li.QuadPart = 0;
		hr = cpStream->Seek(li, STREAM_SEEK_SET, NULL);
	}

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}



	/************************************************
	The wave file and the grammar are not matching. We shouldn't get recognition.
	************************************************/
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
	    for (ULONG ul = 0; ul < vector_RecoResult.size(); ul++)
		{
			CSpDynamicString dsText;
			hr = vector_RecoResult[ul]->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dsText, NULL);
			if (SUCCEEDED(hr))
			{
				dsText .Append(L" shouldn't be received");
				Comment(dsText);
			}
		}
		Comment(IDS_ERR_INVALIDATETOPLEVEL);
		return TPR_FAIL;
	}


	hr = S_OK;

	/************************************************
	Change to a matching wave file for the new grammar and we should receive the recognition event
	************************************************/
	cpStream.Release();
	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_INVALIDATETOPLEVEL_NEW, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
	    if (HlpCheckResult(vector_RecoResult, wszWords) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
	}


	return tpr;

}



TESTPROCAPI t_InvalidateNonToplevelRule(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszRule1[MAX_PATH] = L"";
	WCHAR wszRule2[MAX_PATH] = L"";
	WCHAR wszTopRule[MAX_PATH] = L"";
	WCHAR wszWord1[MAX_PATH] = L"";
	WCHAR wszWord2[MAX_PATH] = L"";
	WCHAR wszTopWords[MAX_PATH] = L"";
	WCHAR wszWords[MAX_PATH] = L"";

	SPSTATEHANDLE hFirstInit;
	SPSTATEHANDLE hSecondInit;
	SPSTATEHANDLE hToplevelInit;
	SPSTATEHANDLE hToplevelAfterRule1;
	SPSTATEHANDLE hToplevelBeforeRule2;

    BOOL                     fFirstReco = FALSE;
    BOOL                     fSecondReco = FALSE;


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_RULE1, wszRule1);
	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_RULE2, wszRule2);
	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_TOPLEVELRULE, wszTopRule);

	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_OLDWORD1, wszWord1);
	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_OLDWORD2, wszWord2);
	GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_TOPLEVELWORDS, wszTopWords);

	wcscpy(wszWords, wszTopWords);
	wcscat(wszWords, wszWord2);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_INVALIDATENONTOPLEVEL_OLD, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule1, 0, 0, TRUE, &hFirstInit);
			if (FAILED(hr))
			{
					Comment(L"create the first dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule2, 0, 0, TRUE, &hSecondInit);
			if (FAILED(hr))
			{
					Comment(L"create the second dynamic rule failed");
			}
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszTopRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//create two new rule states
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->CreateNewState(hToplevelInit, &hToplevelAfterRule1);
			if (FAILED(hr))
			{
					Comment(L"add new rule state failed");
			}
			else
			{
				hr = cpRecoGrammar->CreateNewState(hToplevelInit, &hToplevelBeforeRule2);
				if (FAILED(hr))
				{
						Comment(L"add new rule failed");
				}
			}
	}

	//add rulereference for the first rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddRuleTransition(hToplevelInit, hToplevelAfterRule1, hFirstInit, 1.0, NULL);
	}

	//add words between two rules in the toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hToplevelAfterRule1, hToplevelBeforeRule2, wszTopWords , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L" AddWordTransition on the toplevel dynamic rule failed");
			}
	}

	//add rulereference for the second rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddRuleTransition(hToplevelBeforeRule2, NULL, hSecondInit, 1.0, NULL);
	}


	//Fill in the first rule.

	if (SUCCEEDED(hr))
	{
		//we add the word only when the word is empty
		if (!wcslen(CSpDynamicString(wszWord1).LTrim()))
		{
            //When the word is empty, we need to add epsilon transition.
		    hr = cpRecoGrammar->AddWordTransition(hFirstInit, NULL, NULL , L" ", SPWT_LEXICAL, 1.0, NULL);
        }
        else
        {
		    hr = cpRecoGrammar->AddWordTransition(hFirstInit, NULL, wszWord1 , L" ", SPWT_LEXICAL, 1.0, NULL);
        }
		if (FAILED(hr))
		{
				Comment(L" AddWordTransition on the toplevel dynamic rule failed");
		}
	}


	if (SUCCEEDED(hr))
	{
		if (!wcslen(CSpDynamicString(wszWord2).LTrim()))
		{
            //When the word is empty, we need to add epsilon transition.
		    hr = cpRecoGrammar->AddWordTransition(hSecondInit, NULL, NULL , L" ", SPWT_LEXICAL, 1.0, NULL);
        }
        else
        {
		    hr = cpRecoGrammar->AddWordTransition(hSecondInit, NULL, wszWord2 , L" ", SPWT_LEXICAL, 1.0, NULL);
        }
		if (FAILED(hr))
		{
				Comment(L" AddWordTransition  failed");
		}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}



	/************************************************
	//It is the first rule and using the first wave file before we invalidate the rule by reloading the same rule with different words.
	************************************************/
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
	    if (HlpCheckResult(vector_RecoResult, wszWords) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
	}
    else
    {
	   Comment(IDS_ERR_RESULT_WRONGWORD);
        tpr = TPR_FAIL;
    }
/************************************
************************************/



	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->ClearRule(hFirstInit);
		GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_NEWWORD1, wszWord1);
		if (!wcslen(CSpDynamicString(wszWord1).LTrim()))
		{
            //When the word is empty, we need to add epsilon transition.
		    hr = cpRecoGrammar->AddWordTransition(hFirstInit, NULL, NULL , L" ", SPWT_LEXICAL, 1.0, NULL);
        }
        else
        {
		    hr = cpRecoGrammar->AddWordTransition(hFirstInit, NULL, wszWord1, L" ", SPWT_LEXICAL, 1.0, NULL);
        }
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->ClearRule(hSecondInit);
		if (SUCCEEDED(hr))
		{
			GetWStrFromRes(IDS_INVALIDATENONTOPLEVEL_NEWWORD2, wszWord2);
		    if (!wcslen(CSpDynamicString(wszWord2).LTrim()))
		    {
                //When the word is empty, we need to add epsilon transition.
		        hr = cpRecoGrammar->AddWordTransition(hSecondInit, NULL, NULL , L" ", SPWT_LEXICAL, 1.0, NULL);
            }
            else
            {
				hr = cpRecoGrammar->AddWordTransition(hSecondInit, NULL, wszWord2 , L" ", SPWT_LEXICAL, 1.0, NULL);
            }
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->Commit(NULL);
	}

	wcscpy(wszWords, wszWord1);
	wcscat(wszWords, wszTopWords);
	wcscat(wszWords, CSpDynamicString(wszWord2).RTrim());

	if (SUCCEEDED(hr))
	{
		LARGE_INTEGER li;
		li.QuadPart = 0;
		hr = cpStream->Seek(li, STREAM_SEEK_SET, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}



	/************************************************
	The wave file and the grammar are not matching. We shouldn't get recognition.
	************************************************/
 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
		    Comment(IDS_ERR_INVALIDATETOPLEVEL);
		    return TPR_FAIL;
	}




	/************************************************
	Change to a matching wave file for the new grammar and we should receive the recognition event
	************************************************/
	cpStream.Release();
	hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_INACTIVE);

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_INVALIDATENONTOPLEVEL_NEW, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	if (SUCCEEDED(hr))
	{
		hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
	    if (HlpCheckResult(vector_RecoResult, wszWords) != S_OK)
        {
		    Comment(IDS_ERR_RESULT_WRONGWORD);
		    tpr = TPR_FAIL;
        }
	}


	return tpr;

}




TESTPROCAPI t_DictationTag(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    if (!IsMatchingAttribute(L"DictationInCFG"))
    {
        return TPR_UNSUPPORTED;
    }


    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszWords[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";

	SPSTATEHANDLE hToplevelInit;
	SPSTATEHANDLE hBeforeDictation;

	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_DICTATIONTAG_WORDS, wszWords);
	GetWStrFromRes(IDS_DICTATIONTAG_RULE, wszRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_DICTATIONTAG, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_UNSUPPORTED;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->CreateNewState(hToplevelInit, &hBeforeDictation);
			if (SUCCEEDED(hr))
				hr = cpRecoGrammar->AddWordTransition(hToplevelInit, hBeforeDictation, wszWords , L" ", SPWT_LEXICAL, 1.0, NULL);

			if (SUCCEEDED(hr))
            {
//				hr = cpRecoGrammar->AddWordTransition(hBeforeDictation, NULL, L"*" , L" ", SPWT_LEXICAL, 1.0, NULL);
				hr = cpRecoGrammar->AddRuleTransition(hBeforeDictation, NULL, SPRULETRANS_DICTATION, 1.0, NULL);
            }

			if (FAILED(hr))
			{
					Comment(L" AddWordTransition failed");
			}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_UNSUPPORTED;
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
 	}
	else
	{ 
			Comment(L"no recognition");
			tpr = TPR_UNSUPPORTED;
	}



	return tpr;

}


TESTPROCAPI t_Wildcard(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    if (!IsMatchingAttribute(L"WildcardInCFG"))
    {
        return TPR_UNSUPPORTED;
    }

    int tpr = TPR_SUPPORTED;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszWords[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";

	SPSTATEHANDLE hToplevelInit;
	SPSTATEHANDLE hBeforeWildcard;

    BOOL fWildCard = FALSE;

	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_WILDCARD_WORDS, wszWords);
	GetWStrFromRes(IDS_WILDCARD_RULE, wszRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_WILDCARD, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->CreateNewState(hToplevelInit, &hBeforeWildcard);
			if (SUCCEEDED(hr))
				hr = cpRecoGrammar->AddWordTransition(hToplevelInit, hBeforeWildcard, wszWords , L" ", SPWT_LEXICAL, 1.0, NULL);

			if (SUCCEEDED(hr))
				hr = cpRecoGrammar->AddRuleTransition(hBeforeWildcard, NULL, SPRULETRANS_WILDCARD, 1.0, NULL);

			if (FAILED(hr))
			{
				Comment(L"AddWordTransition failed");
			}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_UNSUPPORTED;
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
            SPPHRASE *pPhrase = NULL;
			if (vector_RecoResult.size())
			{
				hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr) && pPhrase)
				{
                    for (ULONG ul = 0; ul < pPhrase->Rule.ulCountOfElements; ul++)
                    {
					    if (!wcscmp(pPhrase->pElements[ul].pszDisplayText, WILDCARDTEXT) &&
                            !wcscmp(pPhrase->pElements[ul].pszLexicalForm, WILDCARDTEXT))
					    {
                            fWildCard = TRUE;
                            break;
					    }
                    }

                    if (!fWildCard)
                    {
                        tpr = TPR_UNSUPPORTED;
                    }
				}
				else
				{
						Comment(L"no phrase");
						tpr = TPR_UNSUPPORTED;
				}
			}
			else
			{
				Comment(L"no recognition");
				tpr = TPR_UNSUPPORTED;
			}

 	}
	else
	{ 
			Comment(L"no recognition");
			tpr = TPR_UNSUPPORTED;
	}



	return tpr;

}



TESTPROCAPI t_CustomPron(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    int tpr = TPR_PASS;
	HRESULT hr = S_OK;

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
    BOOL                     fEndofStream = FALSE;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszSymbol[MAX_PATH] = L"";
	WCHAR wszDisplay[MAX_PATH] = L"";
	WCHAR wszLexicon[MAX_PATH] = L"";
	WCHAR wszWord[MAX_PATH] = L"";
	WCHAR wszRule[MAX_PATH] = L"";

	SPSTATEHANDLE hToplevelInit;


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_CUSTOMPROP_NEWWORD_PRON, wszSymbol);
	GetWStrFromRes(IDS_CUSTOMPROP_NEWWORD_DISP, wszDisplay);
	GetWStrFromRes(IDS_CUSTOMPROP_NEWWORD_LEX, wszLexicon);

	wcscat(wszWord,  L"/");
	wcscat(wszWord, wszDisplay);
	wcscat(wszWord, L"/");
	wcscat(wszWord, wszLexicon);
	wcscat(wszWord, L"/");
	wcscat(wszWord, wszSymbol);
	wcscat(wszWord, L";");

	GetWStrFromRes(IDS_CUSTOMPROP_RULE, wszRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_WAV_CUSTOMPROP, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWord , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L" AddWordTransition on first dynamic rule failed");
			}
	}

	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
            SPPHRASE *pPhrase = NULL;
			if (vector_RecoResult.size())
			{
				hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr) && pPhrase)
				{
					if (wcscmp(pPhrase->pElements[0].pszDisplayText, wszDisplay))
					{
						Comment(L"The display form is incorrect");
						tpr = TPR_FAIL;
					}
					if ( wcscmp(pPhrase->pElements[0].pszLexicalForm, wszLexicon ))
					{
						Comment(L"The lexicon form is incorrect");
						tpr = TPR_FAIL;
					}
				}
				else
				{
						Comment(L"no phrase");
						tpr = TPR_FAIL;
				}
			}
			else
			{
				Comment(L"no recognition");
				tpr = TPR_FAIL;
			}
	}
	else
	{ 
			Comment(L"no recognition");
			tpr = TPR_FAIL;
	}



	return tpr;

}



TESTPROCAPI t_CaseSensitiveGrammar(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


    int tpr = TPR_PASS;
	HRESULT hr = S_OK;
    LANGID LangId;

    if ((LangId = GetDefaultEnginePrimaryLangId()) == 0x411 || LangId == 0x804)
    {
        return tpr;
    }

	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpStream>				    cpStream;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpRecoResult>				cpRecoResult;
	RESULTVECTOR vector_RecoResult;

	WCHAR wszSymbol1[MAX_PATH] = L"";
	WCHAR wszDisplay1[MAX_PATH] = L"";
	WCHAR wszLexicon1[MAX_PATH] = L"";
	WCHAR wszWord1[MAX_PATH] = L"";

	WCHAR wszSymbol2[MAX_PATH] = L"";
	WCHAR wszDisplay2[MAX_PATH] = L"";
	WCHAR wszLexicon2[MAX_PATH] = L"";
	WCHAR wszWord2[MAX_PATH] = L"";

	WCHAR wszRule[MAX_PATH] = L"";
	SPSTATEHANDLE hToplevelInit;


	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_PRON1, wszSymbol1);
	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_DISP1, wszDisplay1);
	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_LEX1, wszLexicon1);

	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_PRON2, wszSymbol2);
	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_DISP2, wszDisplay2);
	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_LEX2, wszLexicon2);

	wcscat(wszWord1,  L"/");
	wcscat(wszWord1, wszDisplay1);
	wcscat(wszWord1, L"/");
	wcscat(wszWord1, wszLexicon1);
	wcscat(wszWord1, L"/");
	wcscat(wszWord1, wszSymbol1);
	wcscat(wszWord1, L";");

	wcscat(wszWord2,  L"/");
	wcscat(wszWord2, wszDisplay2);
	wcscat(wszWord2, L"/");
	wcscat(wszWord2, wszLexicon2);
	wcscat(wszWord2, L"/");
	wcscat(wszWord2, wszSymbol2);
	wcscat(wszWord2, L";");

	GetWStrFromRes(IDS_CASESENSITIVEGRAMMAR_RULENAME, wszRule);

	// create engine
	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);

	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

    //This wave file is for word2
	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_CASESENSITIVEGRAMMAR_WAVE, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWord1 , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L" AddWordTransition failed");
			}
            else
            {
			    hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWord2 , L" ", SPWT_LEXICAL, 1.0, NULL);
			    if (FAILED(hr))
			    {
					    Comment(L"AddWordTransition failed");
			    }
            }
	}


	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
            SPPHRASE *pPhrase = NULL;
			if (vector_RecoResult.size())
			{
				hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr) && pPhrase)
				{
					if (wcscmp(pPhrase->pElements[0].pszDisplayText, wszDisplay2))
					{
						Comment(L"The display form is incorrect");
						tpr = TPR_FAIL;
					}
					if ( wcscmp(pPhrase->pElements[0].pszLexicalForm, wszLexicon2 ))
					{
						Comment(L"The lexicon form is incorrect");
						tpr = TPR_FAIL;
					}
				}
				else
				{
						Comment(L"no phrase");
						tpr = TPR_FAIL;
				}
			}
			else
			{
				Comment(L"no recognition");
				tpr = TPR_FAIL;
			}
	}
	else
	{ 
			Comment(L"no recognition");
			tpr = TPR_FAIL;
	}


    return tpr;
}



TESTPROCAPI t_CaseSensitiveLexicon(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
	// Message check
    if (uMsg != TPM_EXECUTE) 
		return TPR_NOT_HANDLED;


    int tpr = TPR_PASS;
	HRESULT hr = S_OK;
    LANGID LangId;

    if ((LangId = GetDefaultEnginePrimaryLangId()) == 0x411 || LangId == 0x804)
    {
        return tpr;
    }

	CComPtr<ISpLexicon>					cpLexicon;
	CComPtr<ISpRecoGrammar>				cpRecoGrammar;
	CComPtr<ISpObjectToken>				cpProfileToken;
    CComPtr<ISpRecognizer>			cpRecognizer;
    CComPtr<ISpRecoContext>				cpRecoContext;
	CComPtr<ISpStream>  				cpStream;

    WCHAR wszSymbol1[MAX_PATH] = L"";
    WCHAR wszWord1[MAX_PATH] = L"";
    WCHAR wszSymbol2[MAX_PATH] = L"";
    WCHAR wszWord2[MAX_PATH] = L"";
    WCHAR wszRule[MAX_PATH] = L"";
	RESULTVECTOR vector_RecoResult;
	SPSTATEHANDLE hToplevelInit;

	//Get the rule names and words information from the resource
	GetWStrFromRes(IDS_CASESENSITIVELEXICON_SYMBOL1, wszSymbol1);
	GetWStrFromRes(IDS_CASESENSITIVELEXICON_WORD1, wszWord1);

	GetWStrFromRes(IDS_CASESENSITIVELEXICON_SYMBOL2, wszSymbol2);
	GetWStrFromRes(IDS_CASESENSITIVELEXICON_WORD2, wszWord2);


	GetWStrFromRes(IDS_CASESENSITIVELEXICON_RULENAME, wszRule);


    //Set up user lexicon

	// create a new token object
	if (SUCCEEDED(hr))
		hr = SpCreateNewToken(SPCAT_RECOPROFILES, L"LexProfile", &cpProfileToken);
	
	// create a random user name
	if (SUCCEEDED(hr))
    {
		WCHAR wszName[MAX_PATH] = L"";
		srand(GetCurrentThreadId() + GetTickCount());
		UINT uiRandom = rand();
		swprintf(wszName, L"TestName:%d", uiRandom);

		// set token to this new user
		SpSetDescription(cpProfileToken, wszName);
	}

    if (SUCCEEDED(hr))
		hr = cpLexicon.CoCreateInstance(CLSID_SpLexicon);

	if (SUCCEEDED(hr))
		hr = HlpAddCustomWord(cpLexicon, wszWord1, wszSymbol1);

	if (SUCCEEDED(hr))
		hr = HlpAddCustomWord(cpLexicon, wszWord2, wszSymbol2);


	// create engine
    if (SUCCEEDED(hr))
    {
    	hr = cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpRecognizer->SetRecoProfile(cpProfileToken);
    }
	// create recognition context
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->CreateRecoContext(&cpRecoContext);
	}

	// get event notification
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetNotifyWin32Event();
	}

	// specify events we want to see
	if (SUCCEEDED(hr))
	{
		hr = cpRecoContext->SetInterest(
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM),
            SPFEI(SPEI_RECOGNITION) | SPFEI(SPEI_END_SR_STREAM));
	}

    //This wave file is for word2
	if (SUCCEEDED(hr))
	{
		hr = OpenWavFile(IDS_CASESENSITIVELEXICON_WAVE, &cpStream, NULL);
	}

    if (STG_E_FILENOTFOUND == hr)
        return TPR_FAIL;

	// set the input to the wav file stream
	if (SUCCEEDED(hr))
	{
		hr = cpRecognizer->SetInput(cpStream, TRUE);
	}

	// load the first grammar
    if (SUCCEEDED(hr))
	{
        hr = cpRecoContext->CreateGrammar(GRAM_ID, &cpRecoGrammar);
	}

	//toplevel rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->GetRule(wszRule, 0, SPRAF_TopLevel | SPRAF_Active, TRUE, &hToplevelInit);
			if (FAILED(hr))
			{
					Comment(L"create the toplevel dynamic rule failed");
			}
	}

	//fill in the rule
	if (SUCCEEDED(hr))
	{
			hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWord1 , L" ", SPWT_LEXICAL, 1.0, NULL);
			if (FAILED(hr))
			{
					Comment(L"AddWordTransition failed");
			}
            else
            {
			    hr = cpRecoGrammar->AddWordTransition(hToplevelInit, NULL, wszWord2 , L" ", SPWT_LEXICAL, 1.0, NULL);
			    if (FAILED(hr))
			    {
					    Comment(L"AddWordTransition failed");
			    }
            }
	}


	if (SUCCEEDED(hr))
		hr = cpRecoGrammar->Commit(NULL);

	// perform SR; get results and see if our word is inside the result string 
 	if (SUCCEEDED(hr))
        hr = cpRecoGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);

	if (FAILED(hr))
	{
		Comment(L"initialization failed.");
		return TPR_FAIL;
	}

 	if (SUCCEEDED(hr))
        hr = HlpGetRecoResult(cpRecoContext, vector_RecoResult);

    if (hr == S_OK)
	{
            SPPHRASE *pPhrase = NULL;
			if (vector_RecoResult.size())
			{
				hr = vector_RecoResult[0]->GetPhrase(&pPhrase);
				if (SUCCEEDED(hr) && pPhrase)
				{
					if ( wcscmp(pPhrase->pElements[0].pszLexicalForm, wszWord2 ))
					{
						Comment(L"The result is incorrect");
						tpr = TPR_FAIL;
					}
				}
				else
				{
						Comment(L"no phrase");
						tpr = TPR_FAIL;
				}
			}
			else
			{
				Comment(L"no recognition");
				tpr = TPR_FAIL;
			}
	}
	else
	{ 
			Comment(L"no recognition");
			tpr = TPR_FAIL;
	}

    if (SUCCEEDED(hr))
    	hr = HlpRemoveCustomWord(cpLexicon, wszWord1);

    if (SUCCEEDED(hr))
    	HlpRemoveCustomWord(cpLexicon, wszWord2);

	// remove the random user that was created for this test
	if (cpProfileToken && SUCCEEDED(hr))
	{
		hr = cpProfileToken->Remove(NULL);
		if (hr == SPERR_TOKEN_IN_USE)
		{
			//we can't remove the token because SR Engine is using the token
			//we try to set the original default token to be the current token
			CComPtr<ISpObjectToken> cpDefaultProfileToken;
			hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOPROFILES, &cpDefaultProfileToken);
			if (SUCCEEDED(hr))
			{
				hr = cpRecognizer->SetRecoProfile(cpDefaultProfileToken);
			}

			if (SUCCEEDED(hr))
			{
				hr = cpProfileToken->Remove(NULL);
			}
		}
		else if (FAILED(hr))
		{
			Comment(IDS_ERR_LEX_REMOVEUSER);
		}
	}


    return tpr;
}