//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// srtests1.h
// 
//******************************************************************************
#ifndef __SRTRESTS1_H__
#define __SRTRESTS1_H__

#include <windows.h>
#include <atlbase.h>
#include <tchar.h>
#include <kato.h>
#include <tux.h>
#include "sapi.h"
#include "sapiddk.h"
#include <sphelper.h>

#define GRAM_ID         123

// Added for logging across .cpp files
extern CKato *g_pKato;

extern TCHAR ptszCustomizedDirectory[];

// Engine compliance through SAPI API's
TESTPROCAPI t_CheckEvent_SoundStart				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_SoundEnd				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_SoundStartEnd			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_PhraseStart		    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_Recognition			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_FalseRecognition	    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_CheckEvent_EventsSequences		(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_PhraseStartRecognitionOrder		(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 

TESTPROCAPI t_CheckEvent_Hypothesis				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CheckEvent_Interference		    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 

//TESTPROCAPI t_CheckEvent_PhraseHypReco		(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_UserLexSynchBeforeDicLoad		    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_UserLexSynchBeforeCfgLoad		    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_UserLexSynchAfterDictationLoad	(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_UserLexSynchAfterGrammarLoad		(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_AppLex						    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_UserLexBeforeAppLex			    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_CaseSensitiveLexicon			    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

 
TESTPROCAPI t_GrammarListTag				    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_GrammarExpRuleTag					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_GrammarPTag						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_GrammarOTag						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_GrammarRuleTag					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CustomPron						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_CaseSensitiveGrammar						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
 
TESTPROCAPI t_DictationTag						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_Wildcard							(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_CFGTextBuffer						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

TESTPROCAPI t_MultipleRecoContext				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_GetITNResult					    (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY); 
TESTPROCAPI t_Alternates_Dictation				(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE);
TESTPROCAPI t_Alternates_Cfg					(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE);
TESTPROCAPI t_SpPhraseElements					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_AutoPause							(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_PickGrammar						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_UseLastActivatedGrammar			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_RequiredPropertyString			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_RequiredPropertyNum				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_InvalidateToplevelRule			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_InvalidateNonToplevelRule			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_MultiInstances					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);







#define TESTEVENT_TIMEOUT 20000

extern SPS_SHELL_INFO *g_pShellInfo;  // need this to get handle to dll







#endif // __SRTRESTS1_H__