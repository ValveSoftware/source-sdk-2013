//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// ispttsengine.cpp
//
//******************************************************************************

#include "TTSComp.h"
#include <limits.h>

//******************************************************************************
//***** Globals
//******************************************************************************

//******************************************************************************
//***** Internal Functions and helper classes
//******************************************************************************

//******************************************************************************
//***** TestProc()'s
//******************************************************************************

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_Speak(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test tests the ISpTTSEngine::Speak through the ISpVoice::Speak method.
// It does simple, normal usage tests.

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

	tpr = t_ISpTTSEngine_Speak_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_Speak_Test(UINT uMsg, 
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

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
    CComPtr<ISpStream>			cpStream;

    ULONG                       ulStreamNum;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
    WCHAR                       szwDebug[MAX_PATH]=L"";
    WCHAR                       szwInitDebug[MAX_PATH]=L"";
    WCHAR                       szwNonAscii[] = {0x65e5, 0x672c, 0}; // non-ascii text  
	CSpStreamFormat				InFmt; 

  

    if( SUCCEEDED( hr ) )
    {
		hr = InFmt.AssignFormat(SPSF_22kHz16BitMono);
	}

    if( SUCCEEDED( hr ) )
    {
        hr = SPCreateStreamOnHGlobal( NULL, true, InFmt.FormatId(), InFmt.WaveFormatExPtr(), &cpStream );
    }
    
    // Set the output to memory so we don't hear every speak call
    if( SUCCEEDED( hr ) )
    {
        hr = cpVoice->SetOutput( cpStream, FALSE);
    }

	if( SUCCEEDED( hr ) )
    {
         // Logging info
		 hr =  GetWStrFromRes( IDS_STRING13, szwDebug );

    }

    // Logging info
    CHECKHRGOTOId( hr, tpr, IDS_STRING12 );

    /******* Valid Cases *******/
 
	// 
    // Test #1 - Normal usage
    //
    GetWStrFromRes( IDS_STRING10, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr, SPF_DEFAULT, &ulStreamNum );
    CHECKHRId( hr, tpr, IDS_STRING13 );
    // 
    // Test #2 - Speak empty string
    //
    hr = cpVoice->Speak( L"",  NULL, 0 );
    CHECKHRId( hr, tpr, IDS_STRING13 );

    // 
    // Test #3 - Speak a really long string
    //
    GetWStrFromRes( IDS_STRING11, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr, NULL, 0 );
    CHECKHRId( hr, tpr, IDS_STRING13 );

    // 
    // Test #5 - Speak asyncronously
    //
    GetWStrFromRes( IDS_STRING10, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC, &ulStreamNum );
    CHECKHRId( hr, tpr, IDS_STRING13 );

	hr = cpVoice->WaitUntilDone( TTS_WAITTIME );

	if (!bCalledByMulti )
	{ 
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, " in Test #5");
	}
	else
	{
		CHECKHRIdEx( hr, tpr, IDS_STRING61, " in Test #5");
	}

    // 
    // Test #6 - Purge before speaking
    //
	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC, 0 );
    CHECKHRIdEx( hr, tpr, IDS_STRING13, " in Test #6, SPF_ASYNC");

    hr = cpVoice->Speak( szwSpeakStr, SPF_PURGEBEFORESPEAK, 0 );
    CHECKHRIdEx( hr, tpr, IDS_STRING13, " in Test #6, SPF_PURGEBEFORESPEAK");

    // 
    // Test #7 - Speak the xml tags
    //
    GetWStrFromRes( IDS_STRING8, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr, SPF_IS_XML, 0 );
    CHECKHRId( hr, tpr, IDS_STRING13 );

    // 
    // Test #9 - Speak punctuation
    //
	GetWStrFromRes( IDS_STRING8, szwSpeakStr );
    hr = cpVoice->Speak( szwSpeakStr, SPF_NLP_SPEAK_PUNC, 0 );
    CHECKHRId( hr, tpr, IDS_STRING13 );

    //
    // Test#10 - Speak some non-ASCII text
    //
    hr = cpVoice->Speak( szwNonAscii, SPF_DEFAULT, NULL );
    CHECKHRId( hr, tpr, IDS_STRING13 );

EXIT:
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_Skip(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	// This test tests the ISpTTSEngine::Skip through the ISpVoice::Skip method.
	//  0 ulNumItems -> Start at the beginning of current sentence
	//	1 ulNumItems -> Start at the next sentence
	//	-1 ulNumItems -> Start at the previous sentence
	//	Skip type: sentence

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
	tpr = t_ISpTTSEngine_Skip_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_Skip_Test(UINT uMsg, 
									TPPARAM tpParam, 
									LPFUNCTION_TABLE_ENTRY lpFTE,
									ISpVoice *cpVoice,
									bool bCalledByMulti) 
{
	//this method is called by t_ISpTTSEngine_Skip() and Multiple Instance test.
	//For Multiple Instance test, we do not check audio output.

	   // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;

	
	CComPtr<ISpStream>			cpStream;
	STATSTG						Stat;
	ULONG						sample1 = 0, sample2 = 0, ulNoSkipSample = 0;
	ULONG						NumSkipped = 0;
	const char					* TEST_TOPIC = "";
	WCHAR                       szwSpeakStr[MAX_PATH]=L"";

	//select output format
	CSpStreamFormat				InFmt(SPSF_22kHz16BitMono, &hr); 
	CHECKHRGOTOId( hr, tpr, IDS_STRING84);

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING63, szwSpeakStr ););



	//======================================================================
    	TEST_TOPIC = "Test #1 - Skip forward";
	//======================================================================
	g_pKato->Comment(5, "Skip forward test - Thread Id: %x", GetCurrentThreadId());	
	// Create a stream
    hr = SPCreateStreamOnHGlobal( NULL, true, InFmt.FormatId(), InFmt.WaveFormatExPtr(), &cpStream );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);
	
    // Set the output to the stream
	hr = cpVoice->SetOutput( cpStream, TRUE);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	//initialization
	sample1 = 0, sample2 = 0, ulNoSkipSample = 0;

	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC |SPF_IS_XML, NULL );
	CHECKHRGOTOId( hr, tpr, IDS_STRING13);

	//skip 7 sentences
	hr = cpVoice->Skip( L"SENTENCE", 7, NULL);
	CHECKHRId( hr, tpr, IDS_STRING59);

	hr = cpVoice->WaitUntilDone(TTS_WAITTIME);
	if (!bCalledByMulti )
	{ 
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
	}
	else
	{
		CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
	}
	
	if (!bCalledByMulti )
	{
		hr = cpStream->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);

		sample1 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);
		
		//Speak 
		hr = t_SpeakTwoStreams(cpVoice, szwSpeakStr, szwSpeakStr, sample2, sample2);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC );  

		//save the sample for later comparision
		ulNoSkipSample = sample2;

		//The test will abort if the outputs =  0
		CHECKASSERTGOTOIdEx( ( sample2 > 0 ), tpr, IDS_STRING59, TEST_TOPIC );
		CHECKASSERTGOTOIdEx( ( sample1 > 0 ), tpr, IDS_STRING59, TEST_TOPIC );

		//compare the output streams. The output stream without Skip should be much
		//longer than one with Skip ( 7 )

		CHECKASSERTIdEx( ( ulNoSkipSample > sample1), tpr, IDS_STRING59, TEST_TOPIC );
	}
	cpStream.Release();

	//======================================================================
    	TEST_TOPIC = "Test #2 - Skip backward";
	//======================================================================
	g_pKato->Comment(5, "Skip backward test - Thread Id: %x", GetCurrentThreadId());	

	//initialization
	sample1 = 0, sample2 = 0;

	// Create stream
    hr = SPCreateStreamOnHGlobal( NULL, true, InFmt.FormatId(), InFmt.WaveFormatExPtr(), &cpStream );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the output format
	hr = cpVoice->SetOutput( cpStream, TRUE);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	
	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

	//set interest events
	DOCHECKHRGOTO (hr = cpVoice->SetInterest(SPFEI(SPEI_TTS_BOOKMARK), 0););

	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC |SPF_IS_XML , NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC);
	
	//for multithreaded case, the event may not come
	hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
	if (bCalledByMulti )
	{	
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
	}
	else
	{
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
	}

	//skip backward 7 sentences
	hr = cpVoice->Skip( L"SENTENCE", -7, &NumSkipped);
	CHECKHRIdEx( hr, tpr, IDS_STRING59, TEST_TOPIC);

	hr = cpVoice->WaitUntilDone(TTS_WAITTIME);
	if (!bCalledByMulti )
	{ 
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
	}
	else
	{
		CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
	}

	if (!bCalledByMulti )
	{
		hr = cpStream->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90, TEST_TOPIC);

		sample1 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);
		CHECKASSERTIdEx((sample1 != 0), tpr, IDS_STRING59, TEST_TOPIC );

		//The test will abort if the outputs =  0
		CHECKASSERTGOTOIdEx( ( sample1 > 0 ), tpr, IDS_STRING59, TEST_TOPIC );

		//compare the output streams. The output stream without Skip should be much
		//short than one with Skip ( -7 )

		CHECKASSERTIdEx( ( ulNoSkipSample < sample1), tpr, IDS_STRING59, TEST_TOPIC );
	}

	cpStream.Release();

	//======================================================================
    	TEST_TOPIC = "Test #3 - Skip 0";
	//======================================================================
	g_pKato->Comment(5, "Skip 0 test - Thread Id: %x", GetCurrentThreadId());	

	//initialization
	sample1 = 0, sample2 = 0;

	//clean up the event queue
	while(S_OK == cpVoice->WaitForNotifyEvent(0));

	// Create stream
    hr = SPCreateStreamOnHGlobal( NULL, true, InFmt.FormatId(), InFmt.WaveFormatExPtr(), &cpStream );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);  
	
    // Set the output format
	hr = cpVoice->SetOutput( cpStream, TRUE);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
	
	hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC |SPF_IS_XML , NULL );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC);
	
	//for multithreaded case, the event may not come
	hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
	if (bCalledByMulti )
	{	
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
	}
	else
	{
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
	}
	//skip 0 sentence
	hr = cpVoice->Skip( L"SENTENCE", 0, &NumSkipped);
	CHECKHRIdEx( hr, tpr, IDS_STRING59, TEST_TOPIC);

	hr = cpVoice->WaitUntilDone(TTS_WAITTIME);
	if (!bCalledByMulti )
	{ 
		CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
	}
	else
	{
		CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
	}


	if (!bCalledByMulti )
	{
		hr = cpStream->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90, TEST_TOPIC);

		sample2 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);

		//The test will abort if the outputs =  0
		CHECKASSERTIdEx( (sample2 >= ulNoSkipSample ), tpr, IDS_STRING59, TEST_TOPIC );
	}

EXIT:
    return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_GetOutputFormat(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	// This test tests the ISpTTSEngine::GetOutputFormat method. 
	// GetOutputFormat() is tested directly and also via SAPI

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
	tpr = t_ISpTTSEngine_GetOutputFormat_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_GetOutputFormat_Test(UINT uMsg, 
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

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
    WCHAR                       szwSpeakStr[MAX_PATH]=L"";
	CSpStreamFormat			    OutputFmt;
	CSpStreamFormat			    InputFmt;
	WAVEFORMATEX				Fmtex;

	const char					* TEST_TOPIC = "";	
	CComPtr<ISpStream>			cpStream;

	//Initialize the waveformat structure, this user's format is supported by SAPI5
	Fmtex.wFormatTag			= WAVE_FORMAT_PCM;
	Fmtex.nSamplesPerSec		= 2000; // arbitrary number
	Fmtex.wBitsPerSample		= 8;	
	Fmtex.nChannels				= 1;	
	Fmtex.nBlockAlign			= 1;
	Fmtex.nAvgBytesPerSec		= 2000; // arbitrary number
	Fmtex.cbSize				= 0;

	CComPtr<ISpObjectToken>     cpVoiceToken;
	CComPtr<ISpTTSEngine>       cpTTSEngine;


	DOCHECKHRGOTO(hr=SpGetDefaultTokenFromCategoryId( SPCAT_VOICES, &cpVoiceToken ););
   
    DOCHECKHRGOTO(hr = SpCreateObjectFromToken( cpVoiceToken, &cpTTSEngine ););

	DOCHECKHRGOTO (hr = InputFmt.AssignFormat(SPSF_22kHz16BitMono););
		

	//======================================================================
    	TEST_TOPIC = "Test #1 - Pass in NULL";
	//======================================================================
    hr = cpTTSEngine->GetOutputFormat(
		NULL, 
		NULL, 
		&OutputFmt.m_guidFormatId, 
		&OutputFmt.m_pCoMemWaveFormatEx );
	CHECKHRIdEx( hr, tpr, IDS_STRING14, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Test #2 - Text format";
	//======================================================================
    hr = cpTTSEngine->GetOutputFormat(
		&SPDFID_Text, 
		NULL, 
		&OutputFmt.m_guidFormatId, 
		&OutputFmt.m_pCoMemWaveFormatEx );
	CHECKHRIdEx( hr, tpr, IDS_STRING14, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Test #3 - wave format";
	//======================================================================

	hr = cpTTSEngine->GetOutputFormat(
		&SPDFID_WaveFormatEx, 
		&Fmtex, 
		&OutputFmt.m_guidFormatId, 
		&OutputFmt.m_pCoMemWaveFormatEx );
	CHECKHRIdEx( hr, tpr, IDS_STRING14, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Test #4 - Normal usage";
	//======================================================================
 
    hr = cpTTSEngine->GetOutputFormat( &InputFmt.m_guidFormatId, 
		InputFmt.m_pCoMemWaveFormatEx , 
		&OutputFmt.m_guidFormatId, 
		&OutputFmt.m_pCoMemWaveFormatEx );
	CHECKHRIdEx( hr, tpr, IDS_STRING14, TEST_TOPIC );

 	//======================================================================
    	TEST_TOPIC = "Test #5 - test GetOutputFormat through SetOutPut";
	//======================================================================
	hr = cpVoice->SetOutput( NULL, TRUE);
	CHECKHRIdEx( hr, tpr, IDS_STRING60, TEST_TOPIC );

 	//======================================================================
    	TEST_TOPIC = "Test #6 - test GetOutputFormat through Speak";
	//======================================================================
	GetWStrFromRes( IDS_STRING65, szwSpeakStr );
	hr = cpVoice->Speak( szwSpeakStr, 0, 0 ); 
	CHECKHRIdEx( hr, tpr, IDS_STRING13, TEST_TOPIC );

	//======================================================================
    	TEST_TOPIC = "Test #7 - test GetOutputFormat with user's format";
	//======================================================================

    hr = SPCreateStreamOnHGlobal( NULL, true, SPDFID_WaveFormatEx, &Fmtex, &cpStream );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);
    
    // Set the output to cpStream
	hr = cpVoice->SetOutput( cpStream, TRUE);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
   	
	//Speak calls GetOutputFormat() and passs in the above defined junk wave format 
	//as the first two paramters 
    hr = cpVoice->Speak( szwSpeakStr, 0, 0 );
    CHECKHRIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

EXIT:   
    return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_SetRate(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test tests the ISpTTSEngine::SetRate method through the ISpVoice::SetRate method.
// It does simple, normal usage tests.

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
	tpr = t_ISpTTSEngine_SetRate_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_SetRate_Test(UINT uMsg, 
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

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
    WCHAR                       szwDebug[MAX_PATH]=L"";
	WCHAR                       szwSpeakStr[MAX_PATH]=L"";
	ULONG						cSamples1=0, cSamples2=0, ulHightRateSample=0;


    // Logging info and abort the test if fails here
    CHECKHRGOTOId( hr, tpr, IDS_STRING12 );

	//get the debug string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING16, szwDebug ););
    
	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING10, szwSpeakStr ););
	
    // 
    // Test #1 - Set a rate to 5
    //
	cSamples1 = 0, cSamples2 = 0;

    hr = cpVoice->SetRate( 5 ); 
    CHECKHRId( hr, tpr, IDS_STRING16 );

	//Speak
	hr = t_SpeakTwoStreams(cpVoice, szwSpeakStr, szwSpeakStr, cSamples1, cSamples2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, "SetRate to 5");  
  	
	//save the output for later comparision
	ulHightRateSample = cSamples1;

    // 
    // Test #2 - SetRate to -5
    //
	cSamples1 = 0, cSamples2 = 0;

	hr = cpVoice->SetRate( -5 ); 
    CHECKHRId( hr, tpr, IDS_STRING16 );

	//Speak 
	hr = t_SpeakTwoStreams(cpVoice, szwSpeakStr, szwSpeakStr, cSamples1, cSamples2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, "SetRate to -5" );  

	//In multithreaded case, we do not check audio output
	if (!bCalledByMulti )
	{
		//The test will abort if the outputs =  0
		CHECKASSERTGOTOId( ( ulHightRateSample > 0 ), tpr, IDS_STRING16);
		CHECKASSERTGOTOId( ( cSamples1 > 0 ), tpr, IDS_STRING16);

		//compare the output streams. The output stream when the rate = -5 should be much
		//longer than one when the rate = 5.

		CHECKASSERTId( ( ulHightRateSample < cSamples1), tpr, IDS_STRING16);
	}

    // 
    // Test #3 - SetRate to 0
    //

	//set the rate back to the default
    hr = cpVoice->SetRate( 0 ); 
    CHECKHRId( hr, tpr, IDS_STRING16 );

EXIT:
    return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_SetVolume(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test tests the ISpTTSEngine::SetVolume method through the ISpVoice::SetVolume method.
// It does simple, normal usage tests.

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
	tpr = t_ISpTTSEngine_SetVolume_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
/*****************************************************************************/
TESTPROCAPI t_ISpTTSEngine_SetVolume_Test(UINT uMsg, 
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

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;

	WCHAR                       szwSpeakStr[MAX_PATH]=L"";
	CComPtr<ISpStream>          cpStream1;
    CComPtr<ISpStream>          cpStream2;
	CSpStreamFormat				NewFmt; 
	ULONG						totalAmp1 = 0, totalAmp2 = 0;
	const char					* TEST_TOPIC = "";

  
    if( SUCCEEDED( hr ) )
    {
		hr = NewFmt.AssignFormat(SPSF_22kHz16BitMono);
	}

    // Logging info
    CHECKHRGOTOId( hr, tpr, IDS_STRING12 );

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING10, szwSpeakStr ););

	//======================================================================
    	TEST_TOPIC = "Test #1: SetVolume to 50";
	//======================================================================
 	
    // 
    // Test #1 - SetVolume to 50
    //

	//set volume to 50
    hr = cpVoice->SetVolume( 50 ); 
    CHECKHRIdEx( hr, tpr, IDS_STRING18, TEST_TOPIC);	

   // Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream1 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	
        
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream1, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);	
	
	hr = cpVoice->Speak( szwSpeakStr, 0, 0 ); 
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
    	TEST_TOPIC = "Test #2: SetVolume to 0";
	//======================================================================
 	
    // 
    // Test #2 - SetVolume to 0
    //
    hr = cpVoice->SetVolume( 0 ); 
    CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "Test #3: SetVolume to 100";
	//======================================================================
 		
	// 
    // Test #3 - SetVolume to 100
    //
    hr = cpVoice->SetVolume( 100 ); 
    CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

	// Create stream #2
    hr = SPCreateStreamOnHGlobal( NULL, true, NewFmt.FormatId(), NewFmt.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	
  
    // Set the default output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
    
    hr = cpVoice->Speak( szwSpeakStr, 0, 0 ); 
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);	

		//In multithreaded case, we do not check audio output
	if (!bCalledByMulti )
	{
		// Reset stream to beginning
		hr = cpStream2->Seek( l, SEEK_SET, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);

		// get the sum of the amplitude from the stream 1
		hr =  GetAmpFromSamples (cpStream2, &totalAmp2 ); 
 		CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );	

		//Make sure the amplitudes of stream1 and stream2 are greater than 0 
		CHECKASSERTGOTOId( (totalAmp1 > 0), tpr, IDS_STRING35 );		
		CHECKASSERTGOTOId( (totalAmp2 > 0), tpr, IDS_STRING35 );
		
		//compare the output streams. The amplitude of output stream when the volume = 100
		//should be higher than one when the volume = 50

		CHECKASSERTId( ( totalAmp1 < totalAmp2), tpr, IDS_STRING18);
	}

EXIT:
    return tpr;
}


// Release global objects
void CleanupVoiceAndEngine()
{
}
