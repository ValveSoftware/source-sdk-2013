//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: General matching functions for things like wildcards and !=.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPBASE_MATCHERS_H
#define MAPBASE_MATCHERS_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/mapbase_matchers_base.h"

// Compares with != and the like. Basically hijacks the response system matching.
// This also loops back around to Matcher_NamesMatch.
// pszQuery = The value that should have the operator(s) at the beginning.
// szValue = The value tested against the criterion.
bool Matcher_Match( const char *pszQuery, const char *szValue );
bool Matcher_Match( const char *pszQuery, int iValue );
bool Matcher_Match( const char *pszQuery, float flValue );

// Deprecated; do not use
//static inline bool Matcher_Compare( const char *pszQuery, const char *szValue ) { return Matcher_Match( pszQuery, szValue ); }

#endif
