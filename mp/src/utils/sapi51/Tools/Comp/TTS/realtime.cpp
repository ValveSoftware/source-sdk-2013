//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// realtime.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Helper class to slow down writes to memory stream. It simply forwards all
//***** calls onto IStream. However these tests write output to memory which is
//***** too fast to get events. The write method here has a Sleep(100) statement
//***** in it to make it closer to real time.
//******************************************************************************

class CTestStream : public ISpStream
{  

    /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
	CTestStream(CSpStreamFormat	&Fmt, HRESULT * phr)
		: m_ulRef(1)
	{ 
		*phr = SPCreateStreamOnHGlobal( NULL, true, Fmt.FormatId(), Fmt.WaveFormatExPtr(), &m_cpStream );
	}
	~CTestStream() { SPDBG_ASSERT(m_ulRef == 0); }
    
    //--- IUnknown ----------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == __uuidof(ISpStream) ||
			riid == __uuidof(ISpStreamFormat) || 
            riid == IID_ISequentialStream ||     // Note:  __uuidof() wont work on Windows CE
            riid == __uuidof(IUnknown))
        {
            *ppv = (ISpStream *)this;
            m_ulRef++;
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++m_ulRef;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if(--m_ulRef == 0)
        {
			delete this;
        }
        return m_ulRef;
    }

    //--- ISequentialStream -------------------------------------
    STDMETHOD(Read)( void * pv, ULONG cb, ULONG * pcbRead ) 
                        { return m_cpStream->Read(pv, cb, pcbRead); }
    STDMETHOD(Write)( void const* pv, ULONG cb, ULONG * pcbWritten ) 
    {
        ::Sleep( 2 );
		return m_cpStream->Write(pv, cb, pcbWritten);
	}
    
	//--- ISpStream -------------------------------------
	STDMETHOD (GetFormat)(GUID * pFmtId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
		{ return m_cpStream->GetFormat(pFmtId, ppCoMemWaveFormatEx);};

    STDMETHOD (SetBaseStream)(IStream * pStream, REFGUID rguidFormat, const WAVEFORMATEX * pWaveFormatEx)
		{ return m_cpStream->SetBaseStream(pStream, rguidFormat, pWaveFormatEx);};
    
	STDMETHOD (GetBaseStream)(IStream ** ppStream)
		{ return m_cpStream->GetBaseStream(ppStream);};
    STDMETHOD (BindToFile)(const WCHAR * pszFileName, SPFILEMODE eMode,
                            const GUID * pguidFormatId, const WAVEFORMATEX * pWaveformatEx,
                            ULONGLONG ullEventInterest)
		{ return m_cpStream->BindToFile(pszFileName, eMode, pguidFormatId, pWaveformatEx, ullEventInterest);};
    STDMETHOD (Close)()
		{ return m_cpStream->Close();};

    //--- IStream --------------------------------------
    STDMETHOD(Seek)( LARGE_INTEGER dlibMove, DWORD dwOrigin, 
                        ULARGE_INTEGER * plibNewPosition ) 
                        { return m_cpStream->Seek(dlibMove, dwOrigin, plibNewPosition); }
    STDMETHOD(SetSize)( ULARGE_INTEGER libNewSize ) 
                        { return m_cpStream->SetSize(libNewSize); }
    STDMETHOD(CopyTo)( IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead,
                        ULARGE_INTEGER * pcbWritten ) 
                        { return m_cpStream->CopyTo(pstm, cb, pcbRead, pcbWritten); }
    STDMETHOD(Commit)( DWORD grfCommitFlags ) 
                        { return m_cpStream->Commit(grfCommitFlags); }
    STDMETHOD(Revert)( void ) 
                        { return m_cpStream->Revert(); }
    STDMETHOD(LockRegion)( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, 
                        DWORD dwLockType ) 
                        { return m_cpStream->LockRegion(libOffset, cb, dwLockType); }
    STDMETHOD(UnlockRegion)( ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, 
                        DWORD dwLockType ) 
                        { return m_cpStream->UnlockRegion( libOffset, cb, dwLockType); }
    STDMETHOD(Stat)( STATSTG * pstatstg, DWORD grfStatFlag  ) 
                        { return m_cpStream->Stat(pstatstg, grfStatFlag); }
    STDMETHOD(Clone)( IStream ** ppstm ) 
                        { return m_cpStream->Clone(ppstm); }

	/*=== Member Data ===*/
  protected:
    CComPtr<ISpStream>    m_cpStream;
	ULONG				  m_ulRef;
};

//******************************************************************************
//***** TestProc()'s
//******************************************************************************

/*****************************************************************************/
TESTPROCAPI t_RealTimeRateChange(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test changes the rate of a TTS driver on the fly and checks to see if
// it actually sped up.
// NOTE - this test uses the SAPI Event Model to get bookmark event and will fail 
//if engines have not implemented bookmark event.

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

	tpr = t_RealTimeRateChange_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}
/*****************************************************************************/
TESTPROCAPI t_RealTimeRateChange_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) 

{

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

   
    STATSTG                         Stat;
    ULONG                           cSamples1=0, cSamples2=0, cSamples3=0;
	WCHAR                           szwSpeakStr[MAX_PATH]=L"";
	const char						* TEST_TOPIC = "";


	CSpStreamFormat					NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOId( hr, tpr, IDS_STRING84);

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING53, szwSpeakStr ););


  	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

	//set interest events
	DOCHECKHRGOTO (hr = cpVoice->SetInterest( SPFEI(SPEI_TTS_BOOKMARK), 0););

	//set the rate to the minimum value
	DOCHECKHRGOTO (hr = cpVoice->SetRate( -10 ););
	
    //======================================================================
    	TEST_TOPIC = "t_RealTimeRateChange test: First stream";
	//======================================================================
    {
		g_pKato->Comment(5, "Real time SetRate to -5 test - Thread Id: %x", GetCurrentThreadId());	
		//*** First stream with rate set to -5  below default

		// Create memory stream #1
		CComPtr<CTestStream> cpMemStream1;
		cpMemStream1.p = new CTestStream(NewFmt, &hr);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81, TEST_TOPIC);

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the output to memory
		hr = cpVoice->SetOutput( cpMemStream1, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
   
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
		if (bCalledByMulti )
		{	
				//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{	
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the rate. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetRate( -5 );
		CHECKHRIdEx( hr, tpr, IDS_STRING16,  TEST_TOPIC);       
  
		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}
		
		// Get stream size
		hr = cpMemStream1->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);
		 
		cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
	}
    //======================================================================
    	TEST_TOPIC = "t_RealTimeRateChange test: Second stream";
	//======================================================================
	{
		g_pKato->Comment(5, "Real time SetRate to 0 test - Thread Id: %x", GetCurrentThreadId());	
		//*** Second stream with rate set to default

		CComPtr<CTestStream> cpMemStream2;
		cpMemStream2.p = new CTestStream(NewFmt, &hr);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81, TEST_TOPIC);

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the output to memory
		hr = cpVoice->SetOutput( cpMemStream2, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
   
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
	
		if (bCalledByMulti )
		{
			//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the rate. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetRate( 0 );
		CHECKHRIdEx( hr, tpr, IDS_STRING16,  TEST_TOPIC);       
  
		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}
		
		// Get stream size
		hr = cpMemStream2->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);
		 
		cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
	}

	//======================================================================
    	TEST_TOPIC = "t_RealTimeRateChange test: third stream";
	//======================================================================
	{
		g_pKato->Comment(5, "Real time SetRate to 10 test - Thread Id: %x", GetCurrentThreadId());	
		//*** third stream with rate set to 10 above default

		// Create memory stream #3
		CComPtr<CTestStream> cpMemStream3;
		cpMemStream3.p = new CTestStream(NewFmt, &hr);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81, TEST_TOPIC);

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the output to memory
		hr = cpVoice->SetOutput(cpMemStream3, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);
   
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
		if (bCalledByMulti )
		{
			//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the rate. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetRate( 10 );
		CHECKHRIdEx( hr, tpr, IDS_STRING16,  TEST_TOPIC);       
  
		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}
		
		// Get stream size
		hr = cpMemStream3->Stat( &Stat, STATFLAG_NONAME );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);
		 
		cSamples3 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
	}

	//In Multithreaded case, the test only checks to ensure the system does not fail
	if (!bCalledByMulti )
	{
		// Now compare the values from the 3 streams
		// Note - The higher rate, the shorter the overall stream length
		//
		CHECKASSERTGOTOId(( cSamples1 > 0 ), tpr, IDS_STRING54);
		CHECKASSERTGOTOId(( cSamples2 > 0 ), tpr, IDS_STRING54);
		CHECKASSERTGOTOId(( cSamples3 > 0 ), tpr, IDS_STRING54);
		
		CHECKASSERTId(( cSamples1 > cSamples2 ), tpr, IDS_STRING54);
		CHECKASSERTId(( cSamples2 > cSamples3 ), tpr, IDS_STRING54);
	}

EXIT:
	//set back to default
    cpVoice->SetRate( 0 );
    return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_RealTimeVolumeChange(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test changes the volume of a TTS driver on the fly and checks to see if
// it actually got louder.
// NOTE - this test uses the SAPI Event Model to get bookmark event and will fail 
//if engines have not implemented bookmark event.

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

	tpr = t_RealTimeVolumeChange_Test(uMsg, tpParam, lpFTE, cpLocalVoice, false);

EXIT:
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_RealTimeVolumeChange_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) 

{

    HRESULT                     hr = S_OK;
    int                         tpr = TPR_PASS;
        
    // Message check
    if (uMsg != TPM_EXECUTE) 
    { 
        return TPR_NOT_HANDLED;
    }

    WCHAR                           szwSpeakStr[MAX_PATH]=L"";
	ULONG							totalAmp1 = 0, totalAmp2 = 0, totalAmp3 = 0;
	const char						* TEST_TOPIC = "";

	CSpStreamFormat					NewFmt(SPSF_22kHz16BitMono, &hr);
	CHECKHRGOTOId( hr, tpr, IDS_STRING84);

	LARGE_INTEGER l;
	l.QuadPart = 0;

	//get the speak string
	DOCHECKHRGOTO (hr = GetWStrFromRes( IDS_STRING53, szwSpeakStr ););

  	DOCHECKHRGOTO (hr = cpVoice->SetNotifyWin32Event(););

	//set interest events
	DOCHECKHRGOTO (hr = cpVoice->SetInterest( SPFEI(SPEI_TTS_BOOKMARK), 0););

	//set the start volume to max
	DOCHECKHRGOTO (hr = cpVoice->SetVolume( 100 ););

	//======================================================================
    	TEST_TOPIC = "t_RealTimeVolumeChangeTest: Stream 1 with Volume 100";
	//======================================================================
	{
		g_pKato->Comment(5, "Real time SetVolume to 90 test - Thread Id: %x", GetCurrentThreadId());	

		// Create memory for stream #1
		CComPtr<CTestStream> cpMemStream1;
		cpMemStream1.p = new CTestStream(NewFmt, &hr);
        CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the default output format
		hr = cpVoice->SetOutput( cpMemStream1, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);		
 
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
		if (bCalledByMulti )
		{
			//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the volume. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetVolume( 90 );
		CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}

		// Reset stream to beginning
		hr = cpMemStream1->Seek( l, SEEK_SET, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);	

		// get the sum of the amplitude from the stream 1
		hr =  GetAmpFromSamples (cpMemStream1, &totalAmp1 ); 
 		CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );
	}
	//======================================================================
    	TEST_TOPIC = "t_RealTimeVolumeChangeTest: Stream 2 with Volume 50";
	//======================================================================
    // Create stream #2
	{
		g_pKato->Comment(5, "Real time SetVolume to 50 test - Thread Id: %x", GetCurrentThreadId());	
		// Create memory for stream #2
		CComPtr<CTestStream> cpMemStream2;
		cpMemStream2.p = new CTestStream(NewFmt, &hr);
        CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the default output format
		hr = cpVoice->SetOutput(cpMemStream2, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);		
 
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
		if (bCalledByMulti )
		{
			//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the volume. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetVolume( 50 );
		CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}

		// Reset stream to beginning
		hr = cpMemStream2->Seek( l, SEEK_SET, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);	

		// get the sum of the amplitude from the stream 2
		hr =  GetAmpFromSamples (cpMemStream2, &totalAmp2 ); 
 		CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );
	}

	//======================================================================
    	TEST_TOPIC = "t_RealTimeVolumeChangeTest: Stream 3 with Volume 1";
	//======================================================================
	{
		g_pKato->Comment(5, "Real time SetVolume to 1 test - Thread Id: %x", GetCurrentThreadId());	
		// Create memory for stream #3
		CComPtr<CTestStream> cpMemStream3;
		cpMemStream3.p = new CTestStream(NewFmt, &hr);
        CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC);	

		//clean up the event queue
		while(S_OK == cpVoice->WaitForNotifyEvent(0));

		// Set the default output format
		hr = cpVoice->SetOutput( cpMemStream3, TRUE);
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);		
 
		hr = cpVoice->Speak( szwSpeakStr, SPF_ASYNC | SPF_IS_XML, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

		//wait for bookmark event
		hr = cpVoice->WaitForNotifyEvent(TTS_WAITTIME);
		if (bCalledByMulti )
		{
			//In Multi threaded case
			CHECKHRGOTOIdEx( hr, tpr, IDS_STRING62, TEST_TOPIC);
		}
		else
		{
			//normal case
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING62, TEST_TOPIC);
		}

		//set the volume. At this time, the engine is busying in processing Speak call
		hr = cpVoice->SetVolume( 1 );
		CHECKHRIdEx( hr, tpr, IDS_STRING18,  TEST_TOPIC);

		hr = cpVoice->WaitUntilDone( TTS_WAITTIME_LONG );
		if (!bCalledByMulti )
		{ 
			CHECKASSERTGOTOIdEx( (hr == S_OK), tpr, IDS_STRING61, TEST_TOPIC);
		}
		else
		{
			CHECKHRIdEx( hr, tpr, IDS_STRING61, TEST_TOPIC);
		}

		// Reset stream to beginning
		hr = cpMemStream3->Seek( l, SEEK_SET, NULL );
		CHECKHRGOTOIdEx( hr, tpr, IDS_STRING79,  TEST_TOPIC);	

		// get the sum of the amplitude from the stream 3
		hr =  GetAmpFromSamples (cpMemStream3, &totalAmp3 ); 
 		CHECKHRGOTOIdEx(hr, tpr, IDS_STRING82, TEST_TOPIC );
	}

	if (!bCalledByMulti )
	{
		// Now compare the values from the 3 streams
		
		CHECKASSERTGOTOId(( totalAmp1 > 0 ), tpr, IDS_STRING55);
		CHECKASSERTGOTOId(( totalAmp2 > 0 ), tpr, IDS_STRING55);
		CHECKASSERTGOTOId(( totalAmp3 > 0 ), tpr, IDS_STRING55);

		CHECKASSERTId(( totalAmp1 > totalAmp2 ), tpr, IDS_STRING55);
		CHECKASSERTId(( totalAmp2 > totalAmp3 ), tpr, IDS_STRING55);
		
	}

EXIT:
	//set back to default
    cpVoice->SetVolume( 100 );
    return tpr;
}