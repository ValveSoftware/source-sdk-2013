//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: represent a canonical list of the languages we support, 
//
//=============================================================================

#ifndef LANG_H
#define LANG_H
#ifdef _WIN32
#pragma once
#endif

// if you change this enum also change language.cpp:s_LanguageNames
enum ELanguage
{
	k_Lang_None = -1,
	k_Lang_First = 0,
	k_Lang_English = 0,
	k_Lang_German,
	k_Lang_French,
	k_Lang_Italian,
	k_Lang_Korean,
	k_Lang_Spanish,
	k_Lang_Simplified_Chinese,
	k_Lang_Traditional_Chinese,
	k_Lang_Russian,
	k_Lang_Thai,
	k_Lang_Japanese,
	k_Lang_Portuguese,
	k_Lang_Polish,
	k_Lang_Danish,
	k_Lang_Dutch,
	k_Lang_Finnish,
	k_Lang_Norwegian,
	k_Lang_Swedish,
	k_Lang_Romanian,
	k_Lang_Turkish,
	k_Lang_Hungarian,
	k_Lang_Czech,
	k_Lang_Brazilian,
	k_Lang_Bulgarian,
	k_Lang_Greek,
	k_Lang_Ukrainian,
	k_Lang_Latam_Spanish,
	k_Lang_MAX
};

#define FOR_EACH_LANGUAGE( eLang )		for ( int eLang = (int)k_Lang_First; eLang < k_Lang_MAX; ++eLang )

ELanguage PchLanguageToELanguage(const char *pchShortName, ELanguage eDefault = k_Lang_English);
ELanguage PchLanguageICUCodeToELanguage( const char *pchICUCode, ELanguage eDefault = k_Lang_English );
const char *GetLanguageShortName( ELanguage eLang );
const char *GetLanguageICUName( ELanguage eLang );
const char *GetLanguageVGUILocalization( ELanguage eLang );
const char *GetLanguageName( ELanguage eLang );

#endif /* LANG_H */
