//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: General matching functions for things like wildcards and !=.
//
// $NoKeywords: $
//=============================================================================

#ifndef MAPBASE_MATCHERS_BASE_H
#define MAPBASE_MATCHERS_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include <math.h>

#define MAPBASE_MATCHERS 1

// Regular expressions based off of the std library.
// pszQuery = The regex text.
// szValue = The value that should be matched.
bool Matcher_Regex( const char *pszQuery, const char *szValue );

// Compares two strings with support for wildcards or regex. This code is an expanded version of baseentity.cpp's NamesMatch().
// pszQuery = The value that should have the wildcard.
// szValue = The value tested against the query.
// Use Matcher_Match if you want <, !=, etc. as well.
bool Matcher_NamesMatch( const char *pszQuery, const char *szValue );

// Identical to baseentity.cpp's original NamesMatch().
// pszQuery = The value that should have the wildcard.
// szValue = The value tested against the query.
bool Matcher_NamesMatch_Classic( const char *pszQuery, const char *szValue );

// Identical to Matcher_NamesMatch_Classic(), but either value could use a wildcard.
// pszQuery = The value that serves as the query. This value can use wildcards.
// szValue = The value tested against the query. This value can use wildcards as well.
bool Matcher_NamesMatch_MutualWildcard( const char *pszQuery, const char *szValue );

// Returns true if the specified string contains a wildcard character.
bool Matcher_ContainsWildcard( const char *pszQuery );

// Taken from the Response System.
// Checks if the specified string appears to be a number of some sort.
static bool AppearsToBeANumber( char const *token )
{
	if ( atof( token ) != 0.0f )
		return true;

	char const *p = token;
	while ( *p )
	{
		if ( *p != '0' )
			return false;

		p++;
	}

	return true;
}

#endif
