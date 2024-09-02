#include "SRTests1.h"

// BASE is a unique value assigned to a given tester or component.  This value,
// when combined with each of the following test's unique IDs, allows every 
// test case within the entire team to be uniquely identified.

#define BASE_REQUIRED 0x00010000
#define BASE_OPTIONAL 0x00020000

TCHAR ptszCustomizedDirectory[MAX_PATH] = _T("");

// Our function table that we pass to Tux
FUNCTION_TABLE_ENTRY g_lpFTE[] = {
    TEXT("Required"																), 0,   0,				 0, NULL,
    TEXT("Events"																), 1,   0,				 0, NULL,
    TEXT("Single Event"																), 2,   0,				 0, NULL,
    TEXT(   "SoundStart"											), 3,   0,				 BASE_REQUIRED + 0x0101, t_CheckEvent_SoundStart,
    TEXT(   "SoundEnd"												), 3,   0,				 BASE_REQUIRED + 0x0102, t_CheckEvent_SoundEnd,
	TEXT(   "FalseRecognition"								  	), 3,   0,				 BASE_REQUIRED + 0x0103, t_CheckEvent_FalseRecognition,
	TEXT(   "PhraseStart"											), 3,   0,				 BASE_REQUIRED + 0x0104, t_CheckEvent_PhraseStart,
	TEXT(   "Recognition"											), 3,   0,				 BASE_REQUIRED + 0x0105, t_CheckEvent_Recognition,
    TEXT("Multi Events"																), 2,   0,				 0, NULL,
	TEXT(   "SoundStart -> SoundEnd order"							), 3,   0,				 BASE_REQUIRED + 0x0106, t_CheckEvent_SoundStartEnd,
	TEXT(   "PhraseStart -> Recognition order"					), 3,   0,				 BASE_REQUIRED + 0x0107, t_CheckEvent_PhraseStartRecognitionOrder,
	TEXT(   "Events Offset"								), 3,   0,				 BASE_REQUIRED + 0x0108, t_CheckEvent_EventsSequences,

    TEXT("Lexicon"																), 1,   0,				 0, NULL,
    TEXT("User Lexicon"																), 2,   0,				 0, NULL,
	TEXT(   "Synchronize before loading Command & Control grammar"			), 3,   0,				 BASE_REQUIRED + 0x0201, t_UserLexSynchBeforeCfgLoad,
	TEXT(   "Synchronize Command & Control grammar after loading engine"		), 3,   0,				 BASE_REQUIRED + 0x0202, t_UserLexSynchAfterGrammarLoad,
	TEXT(   "Application Lexicon for Command & Control"												), 2,   0,				 BASE_REQUIRED + 0x0203, t_AppLex,
	TEXT(   "Uses user lexicon before application lexicon for Command & Control"		), 2,   0,				 BASE_REQUIRED + 0x0204, t_UserLexBeforeAppLex,
	TEXT(   "Case sensitive lexicon"		), 2,   0,				 BASE_REQUIRED + 0x0205, t_CaseSensitiveLexicon,

    TEXT("Grammar"																), 1,   0,				 0, NULL,
	TEXT(   "L tag"												), 2,   0,				 BASE_REQUIRED + 0x0301, t_GrammarListTag,
	TEXT(   "Expected Rule"										), 2,   0,				 BASE_REQUIRED + 0x0302, t_GrammarExpRuleTag,
	TEXT(   "P[hrase] tag"												), 2,   0,				 BASE_REQUIRED + 0x0303, t_GrammarPTag,
	TEXT(   "O[ptional] tag"												), 2,   0,				 BASE_REQUIRED + 0x0304, t_GrammarOTag,
	TEXT(   "RULE and RULEREF tags"								), 2,   0,				 BASE_REQUIRED + 0x0305, t_GrammarRuleTag,
	TEXT(   "/Disp/lex/pron"										), 2,   0,				 BASE_REQUIRED + 0x0306, t_CustomPron,
	TEXT(   "Case sensitive grammar"										), 2,   0,				 BASE_REQUIRED + 0x0307, t_CaseSensitiveGrammar,

    TEXT("Other"																	), 1,   0,				 0, NULL,
	TEXT(   "SpPhraseElements"													), 2,   0,				 BASE_REQUIRED + 0x0401, t_SpPhraseElements,
	TEXT(   "Automatically pause engine on recognition"															), 2,   0,				 BASE_REQUIRED + 0x0402, t_AutoPause,
	TEXT(   "Invalidate top level rule"											), 2,   0,				 BASE_REQUIRED + 0x0403, t_InvalidateToplevelRule,
	TEXT(   "Invalidate non-top level rule"										), 2,   0,				 BASE_REQUIRED + 0x0404, t_InvalidateNonToplevelRule,
	TEXT(   "Multi instances"													), 2,   0,				 BASE_REQUIRED + 0x0405, t_MultiInstances,
	TEXT(   "Multiple application contexts [ISpRecoContext]"									), 2,   0,				 BASE_REQUIRED + 0x0406, t_MultipleRecoContext,


    TEXT("Optional"																), 0,   0,				0, NULL,
    TEXT("Events"																), 1,   0,				 0, NULL,
	TEXT(   "Get: Hypothesis"											), 2,   0,				 BASE_OPTIONAL + 0x1100, t_CheckEvent_Hypothesis,
	TEXT(   "Get: Interference"											), 2,   0,				 BASE_OPTIONAL + 0x1101, t_CheckEvent_Interference,

    TEXT("Dictation"																), 1,   0,				 0, NULL,
    TEXT("User Lexicon"																), 2,   0,				 0, NULL,
	TEXT(   "Synchronize before loading dictation grammar"			), 3,   0,				 BASE_OPTIONAL + 0x1200, t_UserLexSynchBeforeDicLoad,
	TEXT(   "Synchronize Dictation grammar after loading engine"	), 3,   0,				 BASE_OPTIONAL + 0x1201, t_UserLexSynchAfterDictationLoad,
	TEXT(   "DICTATION Tag"										), 2,   0,				 BASE_OPTIONAL + 0x1202, t_DictationTag,
	TEXT(   "Dictation Alternates"											), 2,   0,				 BASE_OPTIONAL + 0x001203, t_Alternates_Dictation,

    TEXT("Grammar"																), 1,   0,				 0, NULL,
    TEXT("Tags"																), 2,   0,				 0, NULL,
	TEXT(   "CFGTextBuffer"														), 3,   0,				 BASE_OPTIONAL + 0x1301, t_CFGTextBuffer,
	TEXT(   "WILDCARD Tag"											), 3,   0,				 BASE_OPTIONAL + 0x1303, t_Wildcard,
	TEXT(   "Use correct grammar with unambiguous rules"											), 2,   0,				 BASE_OPTIONAL + 0x1304, t_PickGrammar,
	TEXT(   "Use most recently activated grammar with ambiguous rules"									), 2,   0,				 BASE_OPTIONAL + 0x1305, t_UseLastActivatedGrammar,


    TEXT("Other"																	), 1,   0,				 0, NULL,
	TEXT(   "Recognition with Inverse Text Normalization"													), 2,   0,				 BASE_OPTIONAL + 0x1500, t_GetITNResult,
	TEXT(   "Engine Numeric Properties"														), 2,   0,				 BASE_OPTIONAL + 0x1501, t_RequiredPropertyNum,
	TEXT(   "Engine Text Properties"														), 2,   0,				 BASE_OPTIONAL + 0x1502, t_RequiredPropertyString,
	TEXT(   "Command&Control Alternates"									), 2,   0,				 BASE_OPTIONAL + 0x1503, t_Alternates_Cfg,



	NULL									                                     , 0,   0,              0, NULL  // marks end of list
};

HRESULT SetCurrentDirToDllDir()
{
	TCHAR tszPath[MAX_PATH];
	HRESULT hr = E_FAIL;
	if (::GetModuleFileName(g_pShellInfo->hLib, tszPath, MAX_PATH))
	{
		TCHAR *psLast = ::_tcsrchr(tszPath, '\\');
		if (psLast)
			*psLast = _T('\0');

		if (::SetCurrentDirectory(tszPath))
		{

			hr = S_OK;
		}
	}

	return hr;

}

HRESULT PreTestSetup(void) {
    return SetCurrentDirToDllDir();
}

HRESULT PostTestCleanup(void) {
    return S_OK;
}
