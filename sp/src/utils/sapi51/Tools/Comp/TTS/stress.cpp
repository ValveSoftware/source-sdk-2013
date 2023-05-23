//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// stress.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Internal Functions
//******************************************************************************
extern CRITICAL_SECTION				g_csProcess;

#define	NUM_STRESS_RUNS				20
//******************************************************************************
//***** TestProc()'s
//******************************************************************************


/*****************************************************************************/
TESTPROCAPI t_MultiInstanceTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
// This test randomly calls all the other functions contained in this dll. This
// function will be called on multiple threads and each thread has its own voice
// Object.


	// Message check
	if (uMsg == TPM_QUERY_THREAD_COUNT)
	{
		CleanupVoiceAndEngine();
		((LPTPS_QUERY_THREAD_COUNT)tpParam)->dwThreadCount = 4;
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

    HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;
	int								iRand, i;
	CComPtr<ISpVoice>				cpLocalVoice; 


	//create a SAPI voice, each thread has its own voice
	hr = cpLocalVoice.CoCreateInstance( CLSID_SpVoice ); 
	CHECKHRGOTOId( hr, tpr, IDS_STRING85 );	

	for( i=0; i<NUM_STRESS_RUNS;i++ )
	{
		iRand = GET_RANDOM( 0, 18);
	
		switch( iRand )
		{
		case 0:
            g_pKato->Comment(5, TEXT("Test #0  - Enter t_ISpTTSEngine_Speak - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_ISpTTSEngine_Speak_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #0  - Exit t_ISpTTSEngine_Speak - Thread Id: %x"), GetCurrentThreadId());
			break;

        case 1:
            g_pKato->Comment(5, TEXT("Test #1  - Enter t_ISpTTSEngine_Skip - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_ISpTTSEngine_Skip_Test ( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #1  - Exit t_ISpTTSEngine_Skip - Thread Id: %x"), GetCurrentThreadId());

			break;
			
		case 2:
            g_pKato->Comment(5, TEXT("Test #2  - Enter t_ISpTTSEngine_GetOutputFormat - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_ISpTTSEngine_GetOutputFormat_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #2  - Exit t_ISpTTSEngine_GetOutputFormat - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 3:
            g_pKato->Comment(5, TEXT("Test #3  - Enter t_ISpTTSEngine_SetRate - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_ISpTTSEngine_SetRate_Test( uMsg, tpParam, NULL , cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #3  - Exit t_ISpTTSEngine_SetRate - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 4:
            g_pKato->Comment(5, TEXT("Test #4  - Enter t_ISpTTSEngine_SetVolume - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_ISpTTSEngine_SetVolume_Test( uMsg, tpParam, NULL , cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #4  - Exit t_ISpTTSEngine_SetVolume - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 5:
            g_pKato->Comment(5, TEXT("Test #5  - Enter t_CheckEventsSAPI - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_CheckEventsSAPI_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #5  - Exit t_CheckEventsSAPI - Thread Id: %x"), GetCurrentThreadId());
			break;
		
		case 6:
            g_pKato->Comment(5, TEXT("Test #6  - Enter t_XMLBookmarkTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLBookmark_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #6  - Exit t_XMLBookmarkTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 7:
            g_pKato->Comment(5, TEXT("Test #7  - Enter t_XMLSilenceTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLSilence_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #7  - Exit t_XMLSilenceTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 8:
            g_pKato->Comment(5, TEXT("Test #8 - Enter t_XMLSpellTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLSpell_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #8 - Exit t_XMLSpellTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 9:
            g_pKato->Comment(5, TEXT("Test #9 - Enter t_XMLPronounceTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLPronounce_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #9 - Exit t_XMLPronounceTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 10:
            g_pKato->Comment(5, TEXT("Test #10 - Enter t_XMLRateTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLRate_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #10 - Exit t_XMLRateTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 11:
            g_pKato->Comment(5, TEXT("Test #11 - Enter t_XMLVolumeTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLVolume_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #11 - Exit t_XMLVolumeTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 12:
            g_pKato->Comment(5, TEXT("Test #12 - Enter t_XMLPitchTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLPitch_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #12 - Exit t_XMLPitchTest - Thread Id: %x"), GetCurrentThreadId());
			break;
	
        case 13:
            g_pKato->Comment(5, TEXT("Test #13 - Enter t_RealTimeRateChangeTest - Thread Id: %x"), GetCurrentThreadId());
            tpr = t_RealTimeRateChange_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #13 - Exit t_RealTimeRateChangeTest - Thread Id: %x"), GetCurrentThreadId());
			break;

        case 14:
            g_pKato->Comment(5, TEXT("Test #14 - Enter t_RealTimeVolumeChangeTest - Thread Id: %x"), GetCurrentThreadId());
            tpr = t_RealTimeVolumeChange_Test( uMsg, tpParam, NULL , cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #14 - Exit t_RealTimeVolumeChangeTest - Thread Id: %x"), GetCurrentThreadId());
			break;

		case 15:
            g_pKato->Comment(5, TEXT("Test #15 - Enter t_SpeakStop_Test - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_SpeakStop_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #15 - Exit t_SpeakStop_Test - Thread Id: %x"), GetCurrentThreadId());
			break;	

		case 16:
			g_pKato->Comment(5, TEXT("Test #16 - Enter t_LexiconMultiTest Test - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_LexiconMulti_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #16 - Exit t_LexiconMultiTest Test - Thread Id: %x"), GetCurrentThreadId());
			break;
			
		case 17:
			g_pKato->Comment(5, TEXT("Test #17 - Enter t_XMLSAPIMarkupTest - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLSAPIMarkup_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #17 - Exit t_XMLSAPIMarkupTest - Thread Id: %x"), GetCurrentThreadId());
			break;	

		case 18:
			g_pKato->Comment(5, TEXT("Test #18 - Enter t_XMLContext_Test - Thread Id: %x"), GetCurrentThreadId());
			tpr = t_XMLContext_Test( uMsg, tpParam, NULL, cpLocalVoice, true );
			g_pKato->Comment(5, TEXT("Test #18 - Exit t_XMLContext_Test - Thread Id: %x"), GetCurrentThreadId());
			break;		
		}
		
		if( tpr != TPR_PASS )
		{
			return tpr;
		}
	}

EXIT:
    return tpr;
}
