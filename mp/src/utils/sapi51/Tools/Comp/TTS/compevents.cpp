//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// compevents.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Internal Functions
//******************************************************************************


//******************************************************************************
//***** TestProc()'s
//******************************************************************************

TESTPROCAPI t_CheckEventsSAPI(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{	
	//This is a SAPI5 required test. It checks if the bookmark, word, and sentence
	// boundary events are fired by TTS engine.

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

	tpr = t_CheckEventsSAPI_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_CheckEventsSAPI_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) 
{
	//This is a SAPI5 required test. It checks if the bookmark, word, and sentence
	// boundary events are fired by TTS engine.

	// Message check
    if (uMsg != TPM_EXECUTE) 
	{ 
        return TPR_NOT_HANDLED;
    }

	HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;

	ULONGLONG						ullFlags =  SPFEI(SPEI_WORD_BOUNDARY) | 
                                        SPFEI(SPEI_SENTENCE_BOUNDARY) |
										SPFEI(SPEI_TTS_BOOKMARK); 
	BOOL							bGetEvents = true;
	WCHAR							szwSpeakStr[MAX_PATH]=L"";
    WCHAR                           szwBMarkStr[MAX_PATH]=L"";
    LONGLONG						ullEventsRetrieved = 0;
	LANGID							LangID;


	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING20, szwSpeakStr ););

	//get the bookmark string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING21, szwBMarkStr ););

	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

	//set interest events
	DOCHECKHRGOTO (hr = cpVoice->SetInterest( ullFlags, ullFlags ); );

	//clean up the event queue
	while( hr == S_OK  )
	{
		SPEVENT			Event;
						
		hr = cpVoice->GetEvents (1, &Event, NULL);
	}
	CHECKHRId( hr, tpr, IDS_STRING120 );

	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC |SPF_IS_XML , NULL );
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);
	
	hr = cpVoice->WaitUntilDone(TTS_WAITTIME);
	CHECKASSERTGOTOId( (hr == S_OK), tpr, IDS_STRING61);

	while( hr == S_OK  )
	{
		SPEVENT			Event;
						
		hr = cpVoice->GetEvents (1, &Event, NULL);
	
		if( hr == S_OK )
		{
			// save the events retrieved 
			ullEventsRetrieved |= SPFEI(Event.eEventId);
			if ( Event.eEventId == SPEI_TTS_BOOKMARK)
			{		
				//verify bookmark string (lparam and wparam)
				CHECKASSERTId(( !wcscmp( (WCHAR*)Event.lParam, szwBMarkStr ) ), tpr, IDS_STRING9); 
				CHECKASSERTId(( Event.wParam == _wtol(szwBMarkStr) ), tpr, IDS_STRING9); 						
				continue;
			}
			
			switch (LangID)
			{
			case 2052:
			if ( Event.eEventId == SPEI_WORD_BOUNDARY)
				{				
					//position will be 1 (leading space), 3, 29, and 30
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 3 ) \
						|| ( Event.lParam == 29 ) || ( Event.lParam == 30 )),
								tpr, IDS_STRING9);
					//length of word will be 1, or 2 if "ceshi" as one word
					CHECKASSERTId( ( Event.wParam == 1 ) ||
						( Event.wParam == 2 ), tpr, IDS_STRING9); 
				}
				else if ( Event.eEventId == SPEI_SENTENCE_BOUNDARY)
				{
					//position will 1 (leading space), 3, and  29
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 3 ) \
						|| ( Event.lParam == 29 )), tpr, IDS_STRING9); 
					//length of sentence will be either 2 including period, and
					//3 if "ce shi" as a word.
					CHECKASSERTId( ( Event.wParam == 2 ) || ( Event.wParam == 3 ),
						tpr, IDS_STRING9); 

				}
				break;
			
				case 1041:
				if ( Event.eEventId == SPEI_WORD_BOUNDARY)
				{					
					//lparam position:1, 4, 32
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 4 ) \
						|| ( Event.lParam == 32 ) ),
								tpr, IDS_STRING9); 
					//wparm length of word: 3 or 6 
					CHECKASSERTId( ( Event.wParam == 3 ) || ( Event.wParam == 6 )
						, tpr, IDS_STRING9); 
				}
				else if ( Event.eEventId == SPEI_SENTENCE_BOUNDARY)
				{				
					//lparam position:1, 32, 
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 32 )),
								tpr, IDS_STRING9); 
					//wparm length: 7 and 4 
					//Note: punctuation should be included
					CHECKASSERTId( (( Event.wParam == 7 ) || ( Event.wParam == 4)), 
						tpr, IDS_STRING9); 
				}
			
				break;

			case 1033:	
			default:

				if ( Event.eEventId == SPEI_WORD_BOUNDARY)
				{
					//position will be 1 for book (leading space), 6 for mark, 36 for test1.
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 7 ) \
						|| ( Event.lParam == 36 ) ),
								tpr, IDS_STRING9); 

					//lengths of word are either 4 ( book, mark) or 5 (test1)
					CHECKASSERTId( ( Event.wParam == 5 ) || ( Event.wParam == 4 ), tpr, IDS_STRING9); 
	
				}
				else if ( Event.eEventId == SPEI_SENTENCE_BOUNDARY)
				{

					//position will be 1 for book (leading spaces), 7 for mark), 
					//and 36 for test1
					CHECKASSERTId((( Event.lParam == 1 ) || ( Event.lParam == 7 ) 
						|| ( Event.lParam == 36 )), tpr, IDS_STRING9); 

					//length will be 5 for (book. and mark.) and 6 for (test1.)
					CHECKASSERTId( (( Event.wParam == 5 ) || ( Event.wParam == 6)), 
						tpr, IDS_STRING9); 

				}
			
				break;
			}
		}
	}
	
	// Logging info
	CHECKHRGOTOId( hr, tpr, IDS_STRING9 );

    // Make sure correct events were retrieved
	CHECKASSERTId(((ullEventsRetrieved & ullFlags) == ullFlags), tpr, IDS_STRING9 );
EXIT:
	return tpr;
}

TESTPROCAPI t_CheckEventsNotRequire(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test creates an Event Sink and asks a TTS engine to forward Events to it. It checks
// to make sure that all the expected Events are fired.

	// Message check
    if (uMsg != TPM_EXECUTE) 
	{ 
        return TPR_NOT_HANDLED;
    }

	HRESULT							hr = S_OK;
	int								tpr = TPR_SUPPORTED;
	CComPtr<ISpVoice>				cpVoice;

	ULONGLONG						ullFlags =  SPFEI(SPEI_PHONEME) | 
										SPFEI(SPEI_VISEME);
	BOOL							bGetEvents = true;
	WCHAR							szwSpeakStr[MAX_PATH]=L"";
    WCHAR                           szwDebug[MAX_PATH]=L"";
    ULONGLONG						ullEventsRetrieved = 0;

	// Create the SAPI voice
	DOCHECKHRGOTO (hr = cpVoice.CoCreateInstance( CLSID_SpVoice ););

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING10, szwSpeakStr ););

	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

	//set interest events
	DOCHECKHRGOTO (hr = cpVoice->SetInterest( ullFlags, ullFlags ); );

	//clean up the event queue
	while( hr == S_OK  )
	{
		SPEVENT			Event;
						
		hr = cpVoice->GetEvents (1, &Event, NULL);
	}
	CHECKHRId( hr, tpr, IDS_STRING120 );

	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC |SPF_IS_XML , NULL );
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);
	
	hr = cpVoice->WaitUntilDone( TTS_WAITTIME );
	CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, "in t_CheckEventsSAPI");


	while( hr == S_OK )
	{
		CSpEvent		Event;
					
		hr = Event.GetFrom(cpVoice);

		if( hr == S_OK )
		{
			// Record the events retrieved in dwFlagsRetrieved
			ullEventsRetrieved |= SPFEI(Event.eEventId);
		}
		
	}

	// Logging info
	CHECKHRGOTOId( hr, tpr, IDS_STRING15 );

    // Make sure correct events were retrieved
	CHECKISSUPPORTEDId(( ullEventsRetrieved == ullFlags ), tpr, IDS_STRING15);
	
EXIT:
	return tpr;
}
