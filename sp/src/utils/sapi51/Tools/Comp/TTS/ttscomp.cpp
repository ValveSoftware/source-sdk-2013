//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// ttscomp.cpp
//
//******************************************************************************

#include "TTSComp.h"
 
// BASE is a unique value assigned to a given tester or component.  This value,
// when combined with each of the following test's unique IDs, allows every 
// test case within the entire team to be uniquely identified.

#define BASE 0x000A4000


// Our function table that we pass to Tux
FUNCTION_TABLE_ENTRY g_lpFTE[] = {
TEXT("TTS Compliance Test"							), 0,   0,              0, NULL,
  TEXT("ISpTTSEngine"								), 1,   0,              0, NULL,
    TEXT(   "Speak"                                 ), 2,   0,        BASE+ 1, t_ISpTTSEngine_Speak, 
    TEXT(   "Skip"                                  ), 2,   0,        BASE+ 2, t_ISpTTSEngine_Skip, 
    TEXT(   "GetOutputFormat"                       ), 2,   0,        BASE+ 3, t_ISpTTSEngine_GetOutputFormat, 
    TEXT(   "SetRate"                               ), 2,   0,        BASE+ 4, t_ISpTTSEngine_SetRate, 
    TEXT(   "SetVolume"                             ), 2,   0,        BASE+ 5, t_ISpTTSEngine_SetVolume, 
  TEXT(	"Eventing"									), 1,   0,              0, NULL,
    TEXT(   "Check SAPI required Events"            ), 2,   0,        BASE+ 101, t_CheckEventsSAPI, 
  TEXT("TTS XML Markup"								), 1,   0,              0, NULL,
	TEXT(   "Bookmark"					            ), 2,   0,        BASE+ 201, t_XMLBookmark, 
	TEXT(   "Silence"						        ), 2,   0,        BASE+ 202, t_XMLSilence, 
	TEXT(   "Spell"						            ), 2,   0,        BASE+ 203, t_XMLSpell, 
	TEXT(   "Pronounce"					            ), 2,   0,        BASE+ 204, t_XMLPronounce, 
	TEXT(   "Rate"								    ), 2,   0,        BASE+ 205, t_XMLRate, 
	TEXT(   "Volume"					            ), 2,   0,        BASE+ 206, t_XMLVolume, 
	TEXT(   "Pitch"						            ), 2,   0,        BASE+ 207, t_XMLPitch, 
	TEXT(   "Non-SAPI tags"				            ), 2,   0,        BASE+ 208, t_XMLNonSapiTagsTest, 
	TEXT(   "Context"  	     				        ), 2,   0,        BASE+ 212, t_XMLContext,
  TEXT("Real Time Rate/Vol Tests"					), 1,   0,              0, NULL,
	TEXT(   "Real time rate change"		            ), 2,   0,        BASE+ 301, t_RealTimeRateChange, 
	TEXT(   "Real time volume change"		        ), 2,   0,        BASE+ 302, t_RealTimeVolumeChange, 
  TEXT("Audio State Tests"							), 1,   0,              0, NULL,
	TEXT(   "Speak Stop"							), 2,   0,        BASE+ 402, t_SpeakStop,
	TEXT(   "Speak Destroy" 			            ), 2,   0,        BASE+ 403, t_SpeakDestroy,
  TEXT("Lexicon Tests"								), 1,   0,              0, NULL,
	TEXT(   "User Lexicon Test"						), 2,   0,        BASE+ 501, t_UserLexiconTest,
	TEXT(   "App Lexicon Test"						), 2,   0,        BASE+ 502, t_AppLexiconTest,
  TEXT("Multiple Instance Test"								), 1,   0,              0, NULL,
	TEXT(   "Multiple-Instance Test"				), 2,   0,        BASE+ 601, t_MultiInstanceTest,
  TEXT("Features"								), 0,   0,              0, NULL,
	TEXT(   "Emph"						            ), 1,   0,        BASE+ 801, t_XMLEmphTest, 
	TEXT(   "Phoneme & Viseme Events"		        ), 1,   0,        BASE+ 805, t_CheckEventsNotRequire,
	TEXT(   "PartOfSp"					            ), 1,   0,        BASE+ 806, t_XMLPartOfSpTest, 

	NULL                                             , 0,   0,              0, NULL  // marks end of list
};


// Stub function for cleaning up globals before dll is unloaded
void CleanupTest()
{
    CleanupVoiceAndEngine();
}

HRESULT PreTestSetup(void) {
    return S_OK;
}

HRESULT PostTestCleanup(void) {
    return S_OK;
}
