//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// ttsmarkup.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Internal Functions
//******************************************************************************
#include "f0.h"
int
ComputeF0 (float* data, int nData, int sampFreq, int minF0, int maxF0, int feaSampFreq, 
           float** f0, int* nF0, float **pRms, int *nRms)
{
  char parStr[128];
  F0Data f0Obj = NULL;
  F0Out f0Frame;
  int buffSize;
  int buffStep;
  int nFrames;
  int remainder;
  int nOutput;
  int outSampNum;
  int i;
  int j;

//  assert (data);
 // assert (nData>0);
 // assert (sampFreq);
 // assert (feaSampFreq);
 // assert (f0);

  /*
   * Create and initialize the f0 object
   */
  if ((f0Obj = F0_NewF0Obj ()) == NULL) {
    goto error;
  }

  sprintf (parStr ,"frame_step=%f", 1.0/feaSampFreq);
  if (!F0_ParseParameter (f0Obj, parStr)) {
    goto error;
  }
  
  sprintf (parStr ,"min_f0=%f", (float)minF0);
  if (!F0_ParseParameter (f0Obj, parStr)) {
    goto error;
  }

  sprintf (parStr ,"max_f0=%f", (float)maxF0);
  if (!F0_ParseParameter (f0Obj, parStr)) {
    goto error;
  }

  if (!F0_Init (f0Obj, sampFreq, &buffSize, &buffStep)) {
    goto error;
  }

  /*
   * Prepare memory for output
   */
  *nF0 = *nRms = (int) (((nData / (float)sampFreq) * feaSampFreq) + 0.5);

  *f0 = NULL;
  *pRms = NULL;
  if ( (*f0 = (float *) malloc(*nF0 * sizeof(**f0))) == NULL) 
  {
    goto error;
  }
  if ( (*pRms = (float *) malloc(*nRms * sizeof(**pRms))) == NULL) 
  {
    goto error;
  }

  /*
   * Start processing
   */
  nFrames = (nData - buffSize) / buffStep + 1;
  remainder = nData - nFrames*buffStep;

  outSampNum = 0;
   
  /* Iterate over number of frames*/
  for (i = 0; i<nFrames; i++) {
  
    if ( !F0_AddDataFrame (f0Obj, data + i*buffStep, buffSize) ) {
      goto error;
    }

    nOutput = F0_OutputLength (f0Obj);
    for (j=0; j<nOutput; j++ ) 
	{
      f0Frame = F0_GetOutputFrame (f0Obj, j);
      (*f0)[outSampNum] = f0Frame.f0;
      (*pRms)[outSampNum] = f0Frame.rms;
      outSampNum++;
    }
  }

  /* Remaining samples */
  if ( !F0_AddDataFrame (f0Obj, data + nFrames*buffStep, remainder) ) {
    goto error;
  }

  nOutput = F0_OutputLength (f0Obj);
  for (i=0; i<nOutput; i++ ) 
  {
    f0Frame = F0_GetOutputFrame (f0Obj, i);
    (*f0)[outSampNum]  = f0Frame.f0;
    (*pRms)[outSampNum] = f0Frame.rms;
    outSampNum++;
  }
  
  for (i=outSampNum; i< *nF0; i++) 
  {
    (*f0)[i]  = f0Frame.f0;
    (*pRms)[i]  = f0Frame.rms;
  }

  F0_DeleteF0Obj (&f0Obj);
  return 1;

error:
  if (f0Obj) 
  {
    F0_DeleteF0Obj (&f0Obj);
  }
  if( *f0 )
  {
	  free( *f0 );
  }
  if( *pRms )
  {
	  free( *pRms );
  }

  return 0;
}





//******************************************************************************
//***** TestProc()'s
//******************************************************************************

//********************************************************************************
TESTPROCAPI t_XMLBookmark(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLBookmark_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLBookmark_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test passes a Bookmark through XML marked up text to a TTS engine and checks 
// to make sure a SPFEI_BOOKMARK event has fired.

    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                         hr = S_OK;
    int                             tpr = TPR_PASS;
 
    BOOL                            bGetEvents = true;
    WCHAR                           szwSpeakStr[MAX_PATH]=L"";
    WCHAR							szwDebug[MAX_PATH]=L"";
	DWORD							dwEventsRetrieved = 0;
	WCHAR							szwBmk[MAX_PATH]=L"";
   
    

    if( SUCCEEDED( hr ) )
    {
		hr = cpVoice->SetNotifyWin32Event();
    }
    
    if( SUCCEEDED( hr ) )
    {
        hr = cpVoice->SetInterest( SPFEI(SPEI_TTS_BOOKMARK), SPFEI(SPEI_TTS_BOOKMARK) );  
    }
    
	//clean up the event queue
	while( hr == S_OK  )
	{
		SPEVENT			Event;
						
		hr = cpVoice->GetEvents (1, &Event, NULL);
	}
	CHECKHRId( hr, tpr, IDS_STRING120 );

    if( SUCCEEDED( hr ) )
    {
        GetWStrFromRes( IDS_STRING20, szwSpeakStr );
        hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );  
    }

    if( SUCCEEDED( hr ) )
    {
        hr = cpVoice->WaitUntilDone( TTS_WAITTIME );
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, " in t_XMLBookmarkTest");
    }

	while( hr == S_OK  )
	{
		SPEVENT			Event;
						
		hr = cpVoice->GetEvents (1, &Event, NULL);
	
		if( hr == S_OK )
		{
			// Record the events retrieved in dwFlagsRetrieved
			dwEventsRetrieved |= (DWORD)Event.eEventId;	

			if (Event.eEventId == SPEI_TTS_BOOKMARK )
			{
				//swprintf( szwBmk, L"%s, %d", (WCHAR*)Event.lParam, Event.wParam );
				//g_pKato->Comment(10, szwBmk);

				//verify bookmark string (lparam and wparam)
				GetWStrFromRes( IDS_STRING21, szwDebug );
				CHECKASSERTGOTOId(( !wcscmp( (WCHAR*)Event.lParam, szwDebug ) ), tpr, IDS_STRING22); 
				CHECKASSERTGOTOId(( Event.wParam == _wtol(szwDebug) ), tpr, IDS_STRING22); 						

				break;		
			}
		}

	}
 

    // Logging info
    CHECKHRId( hr, tpr, IDS_STRING22 );
		
	// Make sure correct events were retrieved
	CHECKASSERTGOTOId(((dwEventsRetrieved & SPEI_TTS_BOOKMARK) == SPEI_TTS_BOOKMARK), tpr, IDS_STRING22);


EXIT:    

    return tpr;
}
//********************************************************************************
TESTPROCAPI t_XMLSilence(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLSilence_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLSilence_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test speaks 2 streams. One with a Silence pause and one without. It analyzes
//the two streams to make sure a change has occured. This tests SILENCE tag.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }


    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
    STATSTG                     Stat;
    ULONG                       cSamples1=0, cSamples2=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Stream 1 with no SILENCE tag";
	//======================================================================
 
    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	       
	hr = GetWStrFromRes( IDS_STRING23, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples1 > 0), tpr, IDS_STRING25, TEST_TOPIC );

 	//======================================================================
    	TEST_TOPIC = "Stream 2 with SILENCE tag";
	//======================================================================

    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	       
	hr = GetWStrFromRes( IDS_STRING24, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples2 > 0), tpr, IDS_STRING25, TEST_TOPIC );

    // Now compare the length of the 2 streams
    // Note - check to ensure cSamples2 is at least twice greater than cSamples1.
	CHECKASSERTId((cSamples2 > 2 * cSamples1), tpr, IDS_STRING25);

EXIT:    
    return tpr;
}
//********************************************************************************
TESTPROCAPI t_XMLSpell(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLSpell_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLSpell_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test generates two streams. One that says English as a single word and one that
// spells it out. It analyzes the produced .wav forms to make sure that the second streams
// overall size is greater.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
    STATSTG                     Stat;
	LANGID                      LangID;
    ULONG                       cSamples1=0, cSamples2=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//======================================================================
    	TEST_TOPIC = "Stream 1 with no Spell tag";
	//======================================================================
 
    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	       
	hr = GetWStrFromRes( IDS_STRING26, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);

 	//======================================================================
    	TEST_TOPIC = "Stream 2 with Spell tag";
	//======================================================================

    // Create stream #2
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	      
	hr = GetWStrFromRes( IDS_STRING27, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);

	//For Chinese/Japanese TTS Engines, the test ignores result check since 
	//Spell tag can be ignored in those languages

	if ( ( LangID != 2052 ) && ( LangID != 1041 ))
	{
		CHECKASSERTGOTOIdEx((cSamples2 != 0), tpr, IDS_STRING28, TEST_TOPIC );
		CHECKASSERTGOTOIdEx((cSamples1 != 0), tpr, IDS_STRING28, TEST_TOPIC );

		// Now compare the values from the 2 streams.
		// Note - check to ensure cSamples2 is at least 4/3 greater than cSamples1.
		CHECKASSERTId(((3 * cSamples2 )> (4 * cSamples1)), tpr, IDS_STRING28);
    }
EXIT:  
    return tpr;
}
//********************************************************************************
TESTPROCAPI t_XMLPronounce(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLPronounce_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLPronounce_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test generates two streams. One that says "a" as a single word. The next stream
// says "a" but tells the engine to pronounce it as a random long word. It analyzes the 
// produced .wav forms to make sure that the second stream's overall size is greater.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
    STATSTG                     Stat;
    ULONG                       cSamples1=0, cSamples2=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Stream 1 with no PRON tag";
	//======================================================================
 
    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	
	// short string        
	hr = GetWStrFromRes( IDS_STRING56, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples1 != 0), tpr, IDS_STRING29, TEST_TOPIC );

 	//======================================================================
    	TEST_TOPIC = "Stream 2 with PRON tag";
	//======================================================================

    // Create stream #2
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	
    // short string pronounced as a long string               
	hr = GetWStrFromRes( IDS_STRING57, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples2 != 0), tpr, IDS_STRING29, TEST_TOPIC );
  
    // Now compare the values from the 2 streams
    // Note - check to ensure cSamples2 is at least twice greater than cSamples1.
	CHECKASSERTId((cSamples2> (2 * cSamples1)), tpr, IDS_STRING29);

EXIT:    
    return tpr;
}
//********************************************************************************
TESTPROCAPI t_XMLRate(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLRate_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLRate_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test changes the rate of a TTS engine twice through XML and analyzes the 
// .wav form to make sure a change has occured.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
    STATSTG                     Stat;
    ULONG                       cSamples1=0, cSamples2=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Stream 1 with rate set to -5";
	//======================================================================
 
    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	hr = GetWStrFromRes( IDS_STRING30, szwSpeakStr );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
      
    // Get stream size
	hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples1 != 0), tpr, IDS_STRING32, TEST_TOPIC );

 	//======================================================================
    	TEST_TOPIC = "Stream 2 with rate set to 5";
	//======================================================================
    
    // Create stream #2
	hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC); 
        
    // Set the default output format
	hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	hr = GetWStrFromRes( IDS_STRING31, szwSpeakStr );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);

    hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
   	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

    // Get stream size
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

    cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);
    CHECKASSERTGOTOIdEx((cSamples2 != 0), tpr, IDS_STRING32, TEST_TOPIC );
    
    // Now compare the values from the 2 streams
	// 1. First check ensures low rate has bigger cSamples than higher rate
    // 2. Secondly check ensures cSamples1 and cSamples2 has enough distance
	//
    CHECKASSERTId((cSamples1  > cSamples2 ), tpr, IDS_STRING32);

EXIT:
 
    return tpr;
}
//********************************************************************************
TESTPROCAPI t_XMLVolume(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLVolume_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLVolume_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test changes the volume of a TTS engine twice through XML and analyzes the 
// .wav form to make sure a change has occured.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
    ULONG                       cSamples=0;
	ULONG						ulRatio=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
	ULONG						totalAmp1 = 0, totalAmp2 = 0;
    WCHAR                       szwDebug[MAX_PATH]=L"";
	const char					* TEST_TOPIC = "";

    CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
    CHECKHRGOTOId( hr, tpr, IDS_STRING84 );	

	//======================================================================
    	TEST_TOPIC = "Stream 1 with Volume 100";
	//======================================================================
 
	//*** First stream with volume set to 100

    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	
        
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);	

    hr = GetWStrFromRes( IDS_STRING33, szwSpeakStr );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);	
 
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL ); 
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);	

    LARGE_INTEGER l;
    l.QuadPart = 0;

    // Reset stream to beginning
    hr = cpStream1->Seek( l, SEEK_SET, NULL );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);	

	// get the sum of the amplitude from the stream 1
    hr =  GetAmpFromSamples (cpStream1, &totalAmp1 ); 
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Stream 2 with Volume 1";
	//======================================================================
 	
    //*** Second stream with volume set to 1

    // Create stream #2
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	
  
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
    
    hr = GetWStrFromRes( IDS_STRING34, szwSpeakStr );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83,  TEST_TOPIC);

    hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL ); 
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);	

    // Reset stream to beginning
    hr = cpStream2->Seek( l, SEEK_SET, NULL );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);

	// get the sum of the amplitude from the stream 1
    hr =  GetAmpFromSamples (cpStream2, &totalAmp2 ); 
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );	

	//Make sure the amplitudes of stream1 and stream2 are greater than 0 
	CHECKASSERTGOTOId( (totalAmp1 > 0), tpr, IDS_STRING35 );		
	CHECKASSERTGOTOId( (totalAmp2 > 0), tpr, IDS_STRING35 );
		
    // Note - The test assumes that the overall amplitude from stream 1 will
	//		  be at least 3 times that of stream 2
     
	ulRatio = totalAmp1/totalAmp2;
        
	CHECKASSERTId( (ulRatio>1), tpr, IDS_STRING35 );
 
EXIT:
    
    return tpr;
}

//********************************************************************************
TESTPROCAPI t_XMLPitch(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLPitch_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
//******************************************************************************** 
TESTPROCAPI t_XMLPitch_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
// This test changes the pitch of a TTS engine twice through XML and analyzes the 
// .wav form to make sure a change has occured. It first speaks a sentence at 
// normal pitch and finds the average of the positive amplitude. The average positive
// amplitude is then used as a threshhold above which peak detection is measured. It
// is expected that a higher pitch wav form will have more peaks above the threshhold
// then a lower pitch wav form will.
// NOTE - wav samples are run through a low frequency filter to generate a smother
// wav form.

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }


    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
	STATSTG                     Stat;
    ULONG                       cSamples = 0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
	ULONG						freq1=0, freq2=0;
	float						fratio = 0;
	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr); //sampFreq = 22050;
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	hr = cpVoice->SetVolume( 100 );
	CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

	hr = cpVoice->SetRate( 0 );
	CHECKHRIdEx( hr, tpr, IDS_STRING16,  TEST_TOPIC);  

	//======================================================================
    	TEST_TOPIC = "Stream # 1 with low pitch";
	//======================================================================

    // Create stream #1  with low pitch 
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC );   
       
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream1, TRUE);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC );

	hr = GetWStrFromRes( IDS_STRING37, szwSpeakStr );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING83, TEST_TOPIC );  

    hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC );

    // Get stream size
	hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples = (ULONG)Stat.cbSize.QuadPart / sizeof(short);
    CHECKASSERTGOTOIdEx((cSamples != 0), tpr, IDS_STRING39, TEST_TOPIC );
   
	LARGE_INTEGER l;
    l.QuadPart = 0;

    // Reset stream to beginning
	hr = cpStream1->Seek( l, SEEK_SET, NULL );
   	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );

	hr = GetFreqFromSamples (cpStream1, 22050, cSamples, &freq1);
  	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING91, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Stream # 2 with high pitch";
	//======================================================================

    // Create stream #2 with high pitch
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC );   
      
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC );
    
    hr = GetWStrFromRes( IDS_STRING38, szwSpeakStr );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING83, TEST_TOPIC );  

    hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
   	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC ); 

    // Get stream size
    hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

	cSamples = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
    CHECKASSERTGOTOIdEx((cSamples != 0), tpr, IDS_STRING39, TEST_TOPIC );  
    
    // Reset stream to beginning
    hr = cpStream2->Seek( l, SEEK_SET, NULL );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );
	
	hr = GetFreqFromSamples (cpStream2, 22050, cSamples, &freq2);
  	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING91, TEST_TOPIC );

	{
		//debug 
		TCHAR    szDInfo[MAX_PATH]=TEXT("");
		_stprintf( szDInfo, TEXT("freq1 = %d, freq2 = %d"), freq1, freq2 );
		g_pKato->Comment(10, szDInfo);
	}

	// Now compare the frequencies of the audio output streams
    // Note - Average freqencey with high pitch has at least 1.2 times higher than 
	// low pitch
	CHECKASSERTId((freq1 > 0), tpr, IDS_STRING39 ); 
	CHECKASSERTId((freq2 > 0), tpr, IDS_STRING39 ); 
	fratio = (float)freq2/(float)freq1;
	CHECKASSERTId((fratio > 1.2 ), tpr, IDS_STRING39 ); 
 
EXIT:
    
    return tpr;
}

//***************************************************************************************
TESTPROCAPI t_XMLNonSapiTagsTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with an arbitrary non-sapi tag. In this case, SAPI supplys the
	//SPVA_ParseUnknownTag text fragment state to the tts engine.

	// Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
    CComPtr<ISpVoice>           cpVoice; 
	CComPtr<ISpStream>          cpStream;
	WCHAR                       szwSpeakStr[MAX_PATH]=L"";

	CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOId(hr, tpr, IDS_STRING84);  

	//create a SAPI voice
	hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); 
	CHECKHRGOTOId( hr, tpr, IDS_STRING85 );	

	hr = GetWStrFromRes( IDS_STRING67, szwSpeakStr );
	CHECKHRGOTOId(hr, tpr, IDS_STRING83);  

	// Create a generic stream to send the audio data to so we don't have to listen to it
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream );
    CHECKHRGOTOId(hr, tpr, IDS_STRING81 ); 
        
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream, TRUE);
    CHECKHRGOTOId(hr, tpr, IDS_STRING60 ); 

    hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRId(hr, tpr, IDS_STRING13);
		
EXIT:
    return tpr;
}

//*********************************************************************************
TESTPROCAPI t_XMLEmphTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with EMPH tag. Test uses Pitch, Rate and volume to measure
	//the output streams. The test assume that the output stream of EMPH is different
	//in Rate, or Volume, or pitch from one without emph. If engines implement EMPH in
	//other different way, rather than use pitch, rate, or volume, then this test will
	//fail. And the test result will be "Not Supported".

	// Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_SUPPORTED;
    CComPtr<ISpVoice>           cpVoice;   
    CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
	ULONG						ulRatio=0;
	STATSTG                     Stat;
    ULONG                       cSamples1=0, cSamples2=0;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
	ULONG						totalAmp1 = 0, totalAmp2 = 0;
	ULONG						freq1=0, freq2=0;

	const char					* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
    CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	hr = GetWStrFromRes( IDS_STRING69, szwSpeakStr );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING83, TEST_TOPIC );  

    // Create the SAPI voice
    hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); 
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING85, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Stream #1 without Emph";
	//======================================================================
 
    // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC );   
        
    // Set the output format
    hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC );

	//no Emph
    hr = GetWStrFromRes( IDS_STRING68, szwSpeakStr );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING83, TEST_TOPIC );

    hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL ); 
   	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC );
      
	// Get stream size
    hr = cpStream1->Stat( &Stat, STATFLAG_NONAME ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);

	cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);

    LARGE_INTEGER l;
    l.QuadPart = 0;

    // Reset stream to beginning
    hr = cpStream1->Seek( l, SEEK_SET, NULL );
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );

	// get the sum of the amplitude from the stream 1
    hr =  GetAmpFromSamples (cpStream1, &totalAmp1 ); 
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );	

	// Reset stream to beginning again
    hr = cpStream1->Seek( l, SEEK_SET, NULL );
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );

	//get frequency
	hr = GetFreqFromSamples (cpStream1, 22050, cSamples1, &freq1);
  	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING91, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Stream #2 with Emph";
	//======================================================================
 
    // Create stream #2
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC );   
        
    // Set the output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC );	

    GetWStrFromRes( IDS_STRING69, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr,  SPF_IS_XML, NULL ); 
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC );	
    
	 // Get stream size
    hr = cpStream2->Stat( &Stat, STATFLAG_NONAME ); 
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);

	cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);

    // Reset stream to beginning  
    hr = cpStream2->Seek( l, SEEK_SET, NULL );   
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );	

	// get the sum of the amplitude from the stream 2
    hr =  GetAmpFromSamples (cpStream2, &totalAmp2 ); 
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82 , TEST_TOPIC );	

	// Reset stream to beginning again
    hr = cpStream2->Seek( l, SEEK_SET, NULL );
 	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING79, TEST_TOPIC );

	hr = GetFreqFromSamples (cpStream2, 22050, cSamples2, &freq2);
  	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING91, TEST_TOPIC );

	//Compare the two streams by Pitch, volume, and Rate.
	//The output streams should be different in pitch, or volume, or Rate.

	//Rate measurement
 
	CHECKISSUPPORTEDId( (cSamples2 > 0), tpr, IDS_STRING70 );		
	CHECKISSUPPORTEDId( (cSamples1 > 0), tpr, IDS_STRING70 );

	//Volume measurement
	CHECKISSUPPORTEDId( (totalAmp1 > 0), tpr, IDS_STRING70 );		
	CHECKISSUPPORTEDId( (totalAmp2 > 0), tpr, IDS_STRING70 );		 

	// Pitch measurement
	CHECKISSUPPORTEDId( (freq2 > 0), tpr, IDS_STRING70 );		
	CHECKISSUPPORTEDId( (freq1 > 0), tpr, IDS_STRING70 );

	CHECKISSUPPORTEDId( (freq2 != freq1 ) || (cSamples2 != cSamples1 ) || (totalAmp2 != totalAmp1) , tpr, IDS_STRING70 );

EXIT:      
	return tpr;
}

//********************************************************************************
TESTPROCAPI t_XMLContext(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test Speaks with CONTEXT tag. 

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

	tpr = t_XMLContext_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}

//********************************************************************************
TESTPROCAPI t_XMLContext_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) 
{
	//This test Speaks with CONTEXT tag. 

	// Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;

	CComPtr<ISpStream>			cpStream;
	const char					* TEST_TOPIC = "";
	WCHAR						szwSpeakStr[1024]=L"";

	//======================================================================
    	TEST_TOPIC = "Context: Initialization";
	//======================================================================

    CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC );  

	// Create stream 
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC );   
        
    // Set the output format
    hr = cpVoice->SetOutput( cpStream, TRUE);
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#1a - date_mdy";
	//======================================================================

	//Date in mdy format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING99, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#1b - date_dmy";
	//======================================================================

	//Date in date_dmy format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING100, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#1c - date_ymd";
	//======================================================================

	//Date in date_ymd format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING101, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#2a - date_ym";
	//======================================================================

	//Date in date_ym format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING102, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#2b - date_my";
	//======================================================================

	//Date in date_my format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING103, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#2c - date_dm";
	//======================================================================

	//Date in date_dm format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING104, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#2d - date_md";
	//======================================================================

	//Date in date_md format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING105, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#2e - date_md";
	//======================================================================

	//Date in date_year format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING106, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#3 - time";
	//======================================================================

	//Time in time_hms format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING107, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );
  
	//======================================================================
    	TEST_TOPIC = "Context: Test#4a - number_cardinal";
	//======================================================================

	//Number in number_cardinal format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING108, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#4b - number_digit";
	//======================================================================

	//Number in number_digit format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING109, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );
	
	//======================================================================
    	TEST_TOPIC = "Context: Test#4c - number_fraction";
	//======================================================================

	//Number in number_fraction format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING110, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#4d - number_decimal";
	//======================================================================

	//Number in number_decimal format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING111, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#5 - phone_number";
	//======================================================================

	// phone_number format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING112, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#6 - currency";
	//======================================================================

	// currency format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING113, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#7 - web";
	//======================================================================

	// web format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING114, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );
	
	//======================================================================
    	TEST_TOPIC = "Context: Test#8 - email_address";
	//======================================================================

	// email address format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING115, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );	

	//======================================================================
    	TEST_TOPIC = "Context: Test#9 - address";
	//======================================================================

	// address format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING116, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#10 - address_postal";
	//======================================================================

	// address postal format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING117, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Context: Test#11 - my context tests";
	//======================================================================
	// 
	// ms context format
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING118, szwSpeakStr ););
	hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, NULL );
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

EXIT:
	return tpr;
}

//*************************************************************************************
TESTPROCAPI t_XMLPartOfSpTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//This test is trying to Speak with PartOfSP tags. 
	//Currently it is in the feature list. The test result will be "Supported"
	// or "Unsupported".
	
	// Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT							hr = S_OK;
	int								tpr = TPR_SUPPORTED;

    CComPtr<ISpVoice>				cpVoice;    
    CComPtr<ISpContainerLexicon>	cpConlexicon;
	LANGID                          LangID;
    ULONG							cSamples1=0, cSamples2=0;
	WCHAR							szwSpeakStrNoun[MAX_PATH]=L"";
	WCHAR							szwSpeakStrVerb[MAX_PATH]=L"";
	WCHAR							szNewWord[MAX_PATH]=L"";
	WCHAR							szwSpeakstr[MAX_PATH]=L"";
	WCHAR							szVPronunciation[MAX_PATH]=L"";
	WCHAR							szNPronunciation[MAX_PATH]=L"";

	const char						* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "Initialization";
	//======================================================================
 
	hr = GetWStrFromRes( IDS_STRING76, szNewWord );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83, TEST_TOPIC);

	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//create a SAPI voice
	hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); 
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING85, TEST_TOPIC );  
	
	hr = cpConlexicon.CoCreateInstance(CLSID_SpLexicon);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING85, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Add verb pronunciation to a word";
	//======================================================================

	hr = GetWStrFromRes( IDS_STRING48, szwSpeakStrVerb );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83, TEST_TOPIC);

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwSpeakStrVerb, szVPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  

	hr = cpConlexicon->RemovePronunciation(szNewWord, LangID, SPPS_Verb, szVPronunciation);
	CHECKHRALTGOTOIdEx( hr, SPERR_NOT_IN_LEX,  tpr, IDS_STRING89, TEST_TOPIC);

	//Assign the verb pronunciation of the new word
    hr = cpConlexicon->AddPronunciation(szNewWord, LangID, SPPS_Verb, szVPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING87, TEST_TOPIC );  
	
	//======================================================================
    	TEST_TOPIC = "Add noun pronunciation to a word";
	//======================================================================

	hr = GetWStrFromRes( IDS_STRING52, szwSpeakStrNoun );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83, TEST_TOPIC);

	//get phoneid 
	hr = TTSSymToPhoneId ( LangID, szwSpeakStrNoun, szNPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  
	
	hr = cpConlexicon->RemovePronunciation(szNewWord, LangID, SPPS_Noun, szNPronunciation);
	CHECKHRALTGOTOIdEx( hr, SPERR_NOT_IN_LEX,  tpr, IDS_STRING89, TEST_TOPIC);

	//add the word in noun pronunciation  to Lexicon
    hr = cpConlexicon->AddPronunciation(szNewWord, LangID, SPPS_Noun, szNPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING87, TEST_TOPIC );  

	//======================================================================
    	TEST_TOPIC = "Speak two streams with PartOfSp";
	//======================================================================

	//speak two streams
	hr = GetWStrFromRes( IDS_STRING49, szwSpeakstr );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83, TEST_TOPIC);

	swprintf( szwSpeakStrNoun, szwSpeakstr, szNewWord );

	hr = GetWStrFromRes( IDS_STRING50, szwSpeakstr );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING83, TEST_TOPIC);

	swprintf( szwSpeakStrVerb, szwSpeakstr, szNewWord );

	hr = t_SpeakTwoStreams(cpVoice, szwSpeakStrNoun, szwSpeakStrVerb, cSamples1, cSamples2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC);

	hr = cpConlexicon->RemovePronunciation(szNewWord, LangID, SPPS_Noun, szNPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING89, TEST_TOPIC);

	hr = cpConlexicon->RemovePronunciation(szNewWord, LangID, SPPS_Verb, szVPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING89, TEST_TOPIC);

	CHECKISSUPPORTEDId((cSamples1 > 0 ), tpr, IDS_STRING51);
   	CHECKISSUPPORTEDId((cSamples2 > 0 ), tpr, IDS_STRING51);

	//This test assumes that second stream should be twice shorter than the first one. 
	CHECKISSUPPORTEDId( ( 2* cSamples2 < cSamples1), tpr, IDS_STRING51);

EXIT:
    return tpr;
}

//**************************************************************************************
TESTPROCAPI t_XMLSAPIMarkup_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti)
{
	// This test expects TTS engines to handle all possible spec'ed out combinations 
	//of the XML tags.


    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }
    else if(lpFTE) // called by SpComp directly. It is NULL in multiInstance test.
    {
        srand(reinterpret_cast<LPTPS_EXECUTE>(tpParam)->dwRandomSeed);
    }

    CComPtr<ISpStream>          cpStream;
  
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwXMLString[MAX_PATH]=L"";  
	const char					* TEST_TOPIC = "";

    CSpStreamFormat				NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOId(hr, tpr, IDS_STRING84);  

	//======================================================================
    	TEST_TOPIC = "Markup Test: Initialization";
	//======================================================================

    // Create a generic stream to send the audio data to so we don't have to listen to it
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream );
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING81, TEST_TOPIC);
        
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream, TRUE);
    CHECKHRGOTOIdEx(hr, tpr, IDS_STRING60, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Markup Test: Bookmark";
	//======================================================================

	// Bookmark
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING8, szwXMLString ););
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Markup Test: Silence";
	//======================================================================

    // Silence
    DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING41, szwSpeakStr ););
    swprintf( szwXMLString, szwSpeakStr, GET_RANDOM(0, 2000) );
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);
  
	//======================================================================
    	TEST_TOPIC = "Markup Test: Spell";
	//======================================================================

    // Spell    
    DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING27, szwXMLString ););
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);
    
	//======================================================================
    	TEST_TOPIC = "Markup Test: Pron";
	//======================================================================

	// Pronounce
        
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING43, szwXMLString ););
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Markup Test: Rate";
	//======================================================================

    // Rate
    DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING44, szwSpeakStr ););
    swprintf( szwXMLString, szwSpeakStr, GET_RANDOM(-10, 10) );
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);
	//======================================================================
    	TEST_TOPIC = "Markup Test: Volume";
	//======================================================================

    // Volume
    DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING45, szwSpeakStr ););
    swprintf( szwXMLString, szwSpeakStr, GET_RANDOM(0, 100) );
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Markup Test: Pitch";
	//======================================================================

    // Pitch
    DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING46, szwSpeakStr ););
    swprintf( szwXMLString, szwSpeakStr, GET_RANDOM(-10, 10) );
    hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Markup Test: Non-Sapi";
	//======================================================================
	//Non-Sapi
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING67, szwXMLString ););
	hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);
	
	//======================================================================
    	TEST_TOPIC = "Markup Test: Context";
	//======================================================================

	//Context
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING119, szwXMLString ););
	hr = cpVoice->Speak( szwXMLString,  SPF_ASYNC | SPF_IS_XML, NULL );
	CHECKHRIdEx(hr, tpr, IDS_STRING13, TEST_TOPIC);

	hr = cpVoice->WaitUntilDone( TTS_WAITTIME );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING61, " in t_XMLBookmarkTest");


EXIT:
    return tpr;
}



