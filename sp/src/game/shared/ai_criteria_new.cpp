//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "ai_criteria.h"

#ifdef GAME_DLL
#include "ai_speech.h"
#endif

#include <keyvalues.h>
#include "engine/ienginesound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>



BEGIN_SIMPLE_DATADESC( AI_ResponseParams )
	DEFINE_FIELD( flags,	FIELD_SHORT ),
	DEFINE_FIELD( odds,	FIELD_SHORT ),	
	DEFINE_FIELD( soundlevel,	FIELD_CHARACTER ),	
	DEFINE_FIELD( delay,	FIELD_INTEGER ),		// These are compressed down to two float16s, so treat as an INT for saverestore
	DEFINE_FIELD( respeakdelay,	FIELD_INTEGER ),	//  
END_DATADESC()

BEGIN_SIMPLE_DATADESC( AI_Response )
	DEFINE_FIELD( m_Type,	FIELD_CHARACTER ),
	DEFINE_ARRAY( m_szResponseName, FIELD_CHARACTER, AI_Response::MAX_RESPONSE_NAME ),	
	DEFINE_ARRAY( m_szMatchingRule, FIELD_CHARACTER, AI_Response::MAX_RULE_NAME ),	
	// DEFINE_FIELD( m_pCriteria, FIELD_??? ), // Don't need to save this probably
	DEFINE_EMBEDDED( m_Params ),
END_DATADESC()

