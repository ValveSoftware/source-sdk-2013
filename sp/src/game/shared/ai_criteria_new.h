//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_CRITERIA_H
#define AI_CRITERIA_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlrbtree.h"
#include "tier1/utlsymbol.h"
#include "tier1/interval.h"
#include "mathlib/compressed_vector.h"
#include "../../public/responserules/response_types.h"


using ResponseRules::ResponseType_t;

extern const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen, float *duration, const char *entireContext );

#ifndef AI_CriteriaSet
#define AI_CriteriaSet ResponseRules::CriteriaSet 
#endif

typedef ResponseRules::ResponseParams	AI_ResponseParams ;
typedef ResponseRules::CRR_Response		AI_Response;



/*
// An AI response that is dynamically new'ed up and returned from SpeakFindResponse.
class AI_ResponseReturnValue : AI_Response
{

};
*/

#endif // AI_CRITERIA_H
