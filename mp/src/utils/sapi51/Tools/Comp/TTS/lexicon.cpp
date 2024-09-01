//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// lexicon.cpp
//
//******************************************************************************

#include "TTSComp.h"

//******************************************************************************
//***** Internal Functions
//******************************************************************************

//******************************************************************************
//***** TestProc()'s
//******************************************************************************

//******************************************************************************
TESTPROCAPI t_UserLexiconTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//this procedure tests the user lexicon by 
	// . Add a new word pron
	// . Remove the word pron


	// Message check
    if (uMsg != TPM_EXECUTE) 
	{ 
        return TPR_NOT_HANDLED;
    }
	 
	HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;
	CComPtr<ISpStream>			    cpStream1;
    CComPtr<ISpStream>				cpStream2;
	CComPtr<ISpVoice>               cpVoice;
    CComPtr<ISpContainerLexicon>	cpConlexicon;
	LANGID							LangID;
    ULONG							cSamples1=0, cSamples2=0;
	ULONG							cDiff=0;
	WCHAR                           szwNewWord[MAX_PATH]=L"";
	WCHAR                           szwPronStr[MAX_PATH]=L"";
	WCHAR                           szPronunciation[MAX_PATH]=L"";
	STATSTG							Stat;

	const char						* TEST_TOPIC = "";

	//======================================================================
    	TEST_TOPIC = "User lexicon test: Initialization";
	//======================================================================
	CSpStreamFormat					InFormat(SPSF_22kHz16BitMono, &hr); 
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING84, TEST_TOPIC);

	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//get new word and its pronounciation
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING76, szwNewWord ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING94, szwPronStr ););

	// Create the SAPI voice
    DOCHECKHRGOTO(hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); );
	DOCHECKHRGOTO (hr = cpConlexicon.CoCreateInstance(CLSID_SpLexicon););

	// Create stream #1
    hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream1 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC); 
     
    // Set the output format
    hr = cpVoice->SetOutput( cpStream1, TRUE);
  	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	//remove its pron if it exists already in the User lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//Speak the new word
	hr = cpVoice->Speak(szwNewWord, 0, 0);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);

    hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );
      
	//save to cSamples1 for later comparation
    cSamples1 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
   	CHECKASSERTGOTOIdEx((cSamples1 > 0 ), tpr, IDS_STRING75, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "User lexicon test: Test #1";
	//======================================================================
	//
	//test #1 - Add a word pron
	//
	cpStream2.Release();
	cSamples2 = 0;

	//create stream #2
	hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC); 

	// Set the output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
  	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStr, szPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  

	//remove its pron if it exists already in the User lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//add the pronounciation of the new word to User's lexicon as verb and noun
    hr = cpConlexicon->AddPronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING87,  TEST_TOPIC);

	hr = cpConlexicon->AddPronunciation(szwNewWord, LangID, SPPS_Verb, szPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING87,  TEST_TOPIC);

	hr = cpVoice->Speak(szwNewWord,  0, 0);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
       
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING90, TEST_TOPIC );

    cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
   	CHECKASSERTGOTOId((cSamples2 > 0 ), tpr, IDS_STRING75);

	//The test assume that cSamples2 should be twice greater than cSamples1
  	CHECKASSERTIdEx(( cSamples2 >  2 * cSamples1), tpr, IDS_STRING75, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "User lexicon test: Test #2";
	//======================================================================

	//
	//Test #2 - Remove the word pron
	//
	cpStream2.Release();
	cSamples2 = 0;

	//remove all pronounciation of the word 
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING89,  TEST_TOPIC);

	hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream2 );
    CHECKHRGOTOIdEx( hr, tpr, IDS_STRING81,  TEST_TOPIC); 

	// Set the output format
    hr = cpVoice->SetOutput( cpStream2, TRUE);
  	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING60,  TEST_TOPIC);

	hr = cpVoice->Speak(szwNewWord,  0, 0);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13,  TEST_TOPIC);
       
	hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING90,  TEST_TOPIC);

    cSamples2 = (ULONG)Stat.cbSize.QuadPart / sizeof(SHORT);
   	CHECKASSERTGOTOId((cSamples2 > 0 ), tpr, IDS_STRING75);

	//cSamples1 and cSamples2 should be very close
	cDiff = abs(cSamples2 - cSamples1);
  	CHECKASSERTIdEx(( 5 * cDiff < cSamples2), tpr, IDS_STRING75, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "User lexicon test: Test #3";
	//======================================================================

	//Choose a common word, "Computer", which IS a noun, and it may exist 
	//in app or vendor lexicion. Now we add its pron to the user lexicon 
	//as Noun word. Engine should use the pron in the user lexicon.

	cSamples1=0, cSamples2=0;

	//get new word and its pronounciation
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING64, szwNewWord ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING71, szwPronStr ););
	
	//this Speak uses the default pronunciation
	hr = t_SpeakTwoStreams(cpVoice, szwNewWord, szwNewWord, cSamples1, cSamples1);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC ); 

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStr, szPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  

	//remove its pronounciation if it exists already in the user lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//add the pronounciation of the new word to User's lexicon 
    hr = cpConlexicon->AddPronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING87,  TEST_TOPIC);

	//This Speak should pick up the new pronounciation in the user lexicon
	hr = t_SpeakTwoStreams(cpVoice, szwNewWord, szwNewWord, cSamples2, cSamples2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC ); 

	//remove it if it exists already
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//The test assume that cSamples2 should be twice greater than cSamples1
	CHECKASSERTIdEx(( 2 * cSamples1 <cSamples2), tpr, IDS_STRING75, TEST_TOPIC);

	//======================================================================
    	TEST_TOPIC = "User lexicon test: Test #4 Case Sensitivity";
	//======================================================================
	{
	//Choose a common word, "Computer", and assign String121 as its pronunciation
	//Change "Computer" to all low case, "computer" and assign a new pronounciation
	//stored in String 122, Speak both and check the result.
	
	WCHAR                           szwPronStrUpper[MAX_PATH]=L"";
	WCHAR                           szwPronStrLower[MAX_PATH]=L"";
	WCHAR                           szwNewWordLower[MAX_PATH]=L"";
	WCHAR                           szPronunciationLower[MAX_PATH]=L"";
	cSamples1=0, cSamples2=0;

	//get new word and its pronounciation
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING64, szwNewWord ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING64, szwNewWordLower ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING121, szwPronStrUpper ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING122, szwPronStrLower ););
	
	
	//--- Convert the word to lower case
    WCHAR* pLower = szwNewWordLower;

	_wcslwr( pLower );

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStrUpper, szPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  

	//remove its pronounciation if it exists already in the user lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStrLower, szPronunciationLower);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC );  

	//remove its pronounciation if it exists already in the user lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWordLower, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//add the pronounciation of the new word to User's lexicon 
    hr = cpConlexicon->AddPronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING87,  TEST_TOPIC);

	//add the pronounciation of the new word to User's lexicon 
    hr = cpConlexicon->AddPronunciation(szwNewWordLower, LangID, SPPS_Noun, szPronunciationLower);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING87,  TEST_TOPIC);

	//This Speak should pick up the new pronounciation in the user lexicon
	hr = t_SpeakTwoStreams(cpVoice, szwNewWord, szwNewWordLower, cSamples1, cSamples2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC ); 

	//remove it if it exists already
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//remove it if it exists already
	hr = cpConlexicon->RemovePronunciation(szwNewWordLower, LangID, SPPS_Noun, szPronunciationLower);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//For Chinese/Japanese TTS Engines, the test ignores result check since 
	//No case Sensitivity in Chinese language
	if ( ( LangID != 2052 ) && ( LangID != 1041 ))
	{
		//The test assume that cSamples1(upper case) should be twice greater than cSamples2 (lower case)
		CHECKASSERTIdEx(( 2 * cSamples2 <cSamples1), tpr, IDS_STRING75, TEST_TOPIC);
	}
	}
EXIT:
	return tpr;
}

//******************************************************************************
TESTPROCAPI t_AppLexiconTest(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE) 
{
	//this procedure tests the App lexicon.
	
	// Message check
    if (uMsg != TPM_EXECUTE) 
	{ 
        return TPR_NOT_HANDLED;
    }
	 
	HRESULT							hr = S_OK;
	int								tpr = TPR_PASS;

	CComPtr<ISpVoice>               cpVoice;
    CComPtr<ISpContainerLexicon>	cpConlexicon;
	CComPtr<ISpStream>			    cpStream;
	LANGID							LangID;
	WCHAR                           szwNewWord[MAX_PATH]=L"";
	WCHAR                           szwPronStr[MAX_PATH]=L"";
	WCHAR                           szAppPronunciation[MAX_PATH]=L"";
	ULONG							sample1 = 0, sample2 = 0;
	
	SPWORDPRONUNCIATIONLIST			spPronList; 
	SPWORDPRONUNCIATION				*psppron = NULL;
	CComPtr<ISpLexicon>				cpLexiconApp;

	const char						* TEST_TOPIC = "App lexicon test";

	CSpStreamFormat					InFormat(SPSF_22kHz16BitMono, &hr); 
	CHECKHRGOTOId(hr, tpr, IDS_STRING84);

	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//get new word and its pronounciation
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING76, szwNewWord ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING94, szwPronStr ););
	
	//remove the test app lexicon if it exists from the last test run
	DOCHECKHRGOTO( hr = RemoveTestAppLexicon(););

	//Make a backup of all existing App lexicons 
	hr = RegRecursiveCopyKey(
		HKEY_LOCAL_MACHINE, 
		_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\Tokens"),
		HKEY_LOCAL_MACHINE, 
		_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\TokensBackup"),
		TRUE);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING96, TEST_TOPIC ); 

	//Disenable/delete exsisting app lexicons
	hr = RegRecursiveDeleteKey(
		HKEY_LOCAL_MACHINE, 
		_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\Tokens"),
		TRUE);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING97, TEST_TOPIC ); 

	// Create the SAPI voice
    DOCHECKHRGOTO (hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); );
 
	// Create a lexicon
	DOCHECKHRGOTO (hr = cpConlexicon.CoCreateInstance(CLSID_SpLexicon); );
	
	//the following is to erase the pronunciations of the new word in User's lexicon
	ZeroStruct ( spPronList );

	//get the pronounciation of the new word from the User's lexicon 
    hr = cpConlexicon->GetPronunciations(szwNewWord, LangID, eLEXTYPE_USER, &spPronList);
    CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING86,  TEST_TOPIC);

	//remove all the pronunciations of the new word in User lexicon
	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Unknown, NULL);
	CHECKHRALTIdEx( hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89,  TEST_TOPIC);

	//Speak this word before testing app lexicon and save the result to sample1
	hr = t_SpeakTwoStreams(cpVoice, szwNewWord, szwNewWord, sample1, sample1);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC ); 

	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStr, szAppPronunciation);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING77, TEST_TOPIC ); 

	//Create a test app lexicon
	hr = CreateAppLexicon(
		L"App lex test",
		LangID,
		L"App Lex test",
		L"TTSCompliance;AppLexTest",
		&cpLexiconApp);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING98, TEST_TOPIC ); 

	// add a new word pronunciation to the new created app lexicon
	hr = cpLexiconApp->AddPronunciation(szwNewWord, LangID, SPPS_Noun, szAppPronunciation);
	CHECKHRId( hr,  tpr, IDS_STRING87);

	hr = cpLexiconApp->AddPronunciation(szwNewWord, LangID, SPPS_Verb, szAppPronunciation);
	CHECKHRId( hr,  tpr, IDS_STRING87);

	cpLexiconApp.Release();

	//recreate a voice to make sure that the container lexicon notices the test app leixcon
	cpVoice.Release();
    DOCHECKHRGOTO (hr = cpVoice.CoCreateInstance( CLSID_SpVoice ); );
	
	//The new pronunciation from the test app lexicon should be used here and
	//its output will be saved to sample2
	hr = t_SpeakTwoStreams(cpVoice, szwNewWord, szwNewWord, sample2, sample2);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING88, TEST_TOPIC ); 

	//restore the pronounciation of the new word to User's lexicon 
	for(psppron = spPronList.pFirstWordPronunciation; psppron && SUCCEEDED(hr) && (hr != SP_ALREADY_IN_LEX); psppron = psppron->pNextWordPronunciation)
	{
		hr = cpConlexicon -> AddPronunciation(szwNewWord, psppron->LangID, psppron->ePartOfSpeech, psppron->szPronunciation );
	}
	::CoTaskMemFree(spPronList.pvBuffer);
	CHECKHRALTIdEx( hr, SP_ALREADY_IN_LEX, tpr, IDS_STRING87, TEST_TOPIC);

	cpConlexicon.Release();
	cpVoice.Release();

	//remove the test app lexicon if it exists from the last test run
	DOCHECKHR( hr = RemoveTestAppLexicon(););

	//Now, the test is done, so delete the test app lexicon
	// and restore the previous app lexicons
	hr = RegRecursiveCopyKey(
	HKEY_LOCAL_MACHINE, 
	_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\TokensBackup"),
	HKEY_LOCAL_MACHINE, 
	_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\Tokens"),
	TRUE);
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING96, TEST_TOPIC ); 

	//delete the backups
	hr = RegRecursiveDeleteKey(
		HKEY_LOCAL_MACHINE, 
		_T("SOFTWARE\\Microsoft\\Speech\\AppLexicons\\TokensBackup"));
	CHECKHRGOTOIdEx(hr, tpr, IDS_STRING97, TEST_TOPIC ); 

	CHECKASSERTGOTOIdEx( ( sample1 > 0 ), tpr, IDS_STRING74, TEST_TOPIC );
	CHECKASSERTGOTOIdEx( ( sample2 > 0 ), tpr, IDS_STRING74, TEST_TOPIC );

	// Now compare the values from the 2 streams
	// check  to ensure the voice uses the new pronounciation in the test app lexicon
	// which generates bigger samples than normal.
	CHECKASSERTIdEx( (( 2 * sample2) > (3 * sample1)), tpr, IDS_STRING74, TEST_TOPIC );

EXIT:	
	return tpr;
}

/*****************************************************************************/
TESTPROCAPI t_LexiconMulti_Test(UINT uMsg, 
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

    CComPtr<ISpContainerLexicon>	cpConlexicon;
	CComPtr<ISpStream>			    cpStream;
	LANGID							LangID;
	WCHAR                           szwNewWord[MAX_PATH]=L"";
	WCHAR                           szwPronStr[MAX_PATH]=L"";
	WCHAR                           szPronunciation[MAX_PATH]=L"";


	CSpStreamFormat					InFormat(SPSF_22kHz16BitMono, &hr); 
	CHECKHRGOTOId(hr, tpr, IDS_STRING84);

	DOCHECKHRGOTO( hr = SpGetLanguageIdFromDefaultVoice(&LangID); ); 

	//new word and its pronounciation
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING76, szwNewWord ););
	DOCHECKHRGOTO( hr = GetWStrFromRes( IDS_STRING94, szwPronStr ););

	// Create a lexicon
	DOCHECKHRGOTO (hr = cpConlexicon.CoCreateInstance(CLSID_SpLexicon); );

	//create stream
	hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream );
	CHECKHRGOTOId( hr, tpr, IDS_STRING81); 

	// Set the output format
    hr = cpVoice->SetOutput( cpStream, TRUE);
  	CHECKHRGOTOId( hr, tpr, IDS_STRING60);

	//The new word is stored in szwNewWord and its pronunciation is in szwPronStr.
	//First get the pronounciation of the string stored in szwPronStr,
	//and then assign the pronounciation to szwNewWord
	//
	//get phoneid
	hr = TTSSymToPhoneId ( LangID, szwPronStr, szPronunciation);
	CHECKHRGOTOId(hr, tpr, IDS_STRING77 );  

	//add the pronounciation of the new word to User's lexicon 
    hr = cpConlexicon->AddPronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRALTId( hr, SP_ALREADY_IN_LEX, tpr, IDS_STRING87);

	hr = cpVoice->Speak(szwNewWord, 0, 0);
	CHECKHRGOTOIdEx( hr, tpr, IDS_STRING13, " in t_LexiconMultiTest");

	hr = cpConlexicon->RemovePronunciation(szwNewWord, LangID, SPPS_Noun, szPronunciation);
	CHECKHRALTId(hr, SPERR_NOT_IN_LEX, tpr, IDS_STRING89);   

EXIT:
	return tpr;
}
