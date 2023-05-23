//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// audiostate.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Internal Functions
//******************************************************************************
inline HRESULT SpGetLanguageFromVoiceToken(ISpObjectToken * pToken, LANGID * plangid)
{
    
    HRESULT hr = S_OK;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    hr = pToken->OpenKey(L"Attributes", &cpDataKeyAttribs);

    CSpDynamicString dstrLanguage;
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->GetStringValue(L"Language", &dstrLanguage);
    }

 
    LANGID langid;
    if (SUCCEEDED(hr))
    {
        if (!swscanf(dstrLanguage, L"%hx", &langid))
        {
            hr = E_UNEXPECTED;
        }
    }

    if (SUCCEEDED(hr))
    {
        *plangid = langid;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
} /* SpGetLanguageFromVoiceToken */

HRESULT SpGetLanguageIdFromDefaultVoice(LANGID *plangid)
{
	HRESULT                     hr = S_OK;

    CComPtr<ISpObjectToken>     cpVoiceToken;
	LANGID						langid;
    // Find the best voice
    if( SUCCEEDED( hr ) )
    {
        hr = SpGetDefaultTokenFromCategoryId( SPCAT_VOICES, &cpVoiceToken );
    }
	if( SUCCEEDED( hr ) )
	{
        hr = SpGetLanguageFromVoiceToken(cpVoiceToken, &langid);
	}
	*plangid = langid;

	return hr;
}

HRESULT GetWStrFromRes( UINT id, WCHAR* szwStr, int	cchBufferMax )
{
    HRSRC       hResInfo = NULL;
    HANDLE      hStringSeg = NULL;
    LPWSTR      lpsz = NULL;
    int         cch = 0;
	HRESULT		hr = S_OK;
	LANGID		Language;

	hr = SpGetLanguageIdFromDefaultVoice(&Language);  
	if (FAILED (hr ) )
		return hr; 

    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW( g_pShellInfo->hLib, (LPCWSTR)RT_STRING,
                                    MAKEINTRESOURCEW(((USHORT)id >> 4) + 1), 
								    Language) )
    {        
        // Load that segment.        
        hStringSeg = LoadResource( g_pShellInfo->hLib, hResInfo );

        // Lock the resource.        
        if( lpsz = (LPWSTR)LockResource(hStringSeg) ) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            id &= 0x0F;
            while( TRUE ) 
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
            CopyMemory( szwStr, lpsz, cch*sizeof(WCHAR) );

            // Attach Null terminator.
            szwStr[cch] = 0;
        }
    }

	return cch ? S_OK : HRESULT_FROM_WIN32(ERROR_RESOURCE_NAME_NOT_FOUND);
}
//******************************************************************************
//***** TestProc()'s
//******************************************************************************
/*****************************************************************************/

//*************************************************************************************
TESTPROCAPI t_SpeakStop(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test uses the default audio object to speak pause and speak a stream. It checks
// to make sure a TTS engine is paying attention to these states by stopping the stream 
// part way through and counting the number of word events ( these must be less than the
// total number in the string.

    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
	
	CComPtr<ISpVoice>           cpLocalVoice;

	// Create the SAPI voice
    DOCHECKHRGOTO (hr = cpLocalVoice.CoCreateInstance( CLSID_SpVoice ););

	tpr = t_SpeakStop_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
/*****************************************************************************/
TESTPROCAPI t_SpeakStop_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) 

{
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;

	int								cEventsReceived = 0;
	WCHAR							szwSpeakStr[MAX_PATH]=L"";	
	CSpEvent						Event;

	hr = cpVoice->SetOutput(NULL, TRUE);
	CHECKHRGOTOId( hr, tpr, IDS_STRING60);

	//clean up the event queue. 
	while (Event.GetFrom(cpVoice) == S_OK);
	
	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

    DOCHECKHRGOTO (	hr = cpVoice->SetInterest( SPFEI(SPEI_WORD_BOUNDARY), SPFEI(SPEI_WORD_BOUNDARY) ); );

	DOCHECKHRGOTO ( hr = GetWStrFromRes( IDS_STRING1, szwSpeakStr ););

	hr = cpVoice->Speak( szwSpeakStr,  SPF_ASYNC, NULL );  
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);

	//wait for the first word boundry event
	hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
	if (bCalledByMulti )
	{
		//In Multi threaded case
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, " t_SpeakStop_Test");
	}
	else
	{
		//normal case
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, " t_SpeakStop_Test");
	}	


	Sleep (500);

	//purges all data, close the audio and set action flag to SPVES_ABORT  
	hr = cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, 0 );
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);

	hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
	if (!bCalledByMulti )
	{ 
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, " t_SpeakStop_Test");
	}
	else
	{
		CHECKHRIdEx( hr, tpr, IDS_STRING61, " t_SpeakStop_Test");
	}

	while (S_OK == hr )
	{	
		hr = Event.GetFrom(cpVoice);

		if( hr == S_OK )
		{
			// count how many word boundry events we got
			cEventsReceived++; 
		}
	
	}

	//Make sure the entire first speak call did not finish
	//Note - The sentence contains 20 words, so normally we can receive 20 word 
	//boundary events.
	CHECKASSERTId(( cEventsReceived < 20 ), tpr, IDS_STRING5);

	// Logging info
	CHECKHRId( hr, tpr, IDS_STRING5 );

EXIT:    
    return tpr;
}

//*************************************************************************************
TESTPROCAPI t_SpeakDestroy(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test starts a long Speak call and destroys the stream before completion. It 
// expects engines to clean-up correctly and not fault.

    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;
	CComPtr<ISpVoice>				cpVoice;
	WCHAR							szwSpeakStr[MAX_PATH]=L"";
    WCHAR                           szwDebug[MAX_PATH]=L"";
	    

	// Create the SAPI voice
    DOCHECKHRGOTO (hr = cpVoice.CoCreateInstance( CLSID_SpVoice ););

	//get the bookmark string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING6, szwSpeakStr ););

	hr = cpVoice->Speak( szwSpeakStr,  SPF_ASYNC, NULL );  
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);
	
	cpVoice.Release();

	// Logging info
	CHECKHRId( hr, tpr, IDS_STRING7 );

EXIT:
    return tpr;
}

