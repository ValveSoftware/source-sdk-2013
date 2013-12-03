//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHONEMEEXTRACTOR_H
#define PHONEMEEXTRACTOR_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

class CSentence;

typedef enum
{
	SPEECH_API_SAPI = 0,
	SPEECH_API_LIPSINC,
} PE_APITYPE;

typedef enum
{
	SR_RESULT_NORESULT = 0,
	SR_RESULT_ERROR,
	SR_RESULT_SUCCESS,
	SR_RESULT_FAILED
} SR_RESULT;

abstract_class IPhonemeExtractor
{
public:
	virtual PE_APITYPE	GetAPIType() const = 0;

	// Used for menus, etc
	virtual char const *GetName() const = 0;

	virtual SR_RESULT Extract( 
		const char *wavfile,
		int numsamples,
		void (*pfnPrint)( PRINTF_FORMAT_STRING const char *fmt, ... ),
		CSentence& inwords,
		CSentence& outwords ) = 0;
};

#define VPHONEME_EXTRACTOR_INTERFACE		"PHONEME_EXTRACTOR_001"

#endif // PHONEMEEXTRACTOR_H
