//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: General matching functions for things like wildcards and !=.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "matchers.h"
#include "fmtstr.h"

// glibc (Linux) uses these tokens when including <regex>, so we must not #define them
#undef max
#undef min
#include <regex>
#undef MINMAX_H
#include "minmax.h"

ConVar mapbase_wildcards_enabled("mapbase_wildcards_enabled", "1", FCVAR_NONE, "Toggles Mapbase's '?' wildcard and true '*' features. Useful for maps that have '?' in their targetnames.");
ConVar mapbase_regex_enabled("mapbase_regex_enabled", "1", FCVAR_NONE, "Toggles Mapbase's regex matching handover.");

#ifdef CLIENT_DLL
// FIXME: There is no clientside equivalent to the RS code
static bool ResponseSystemCompare(const char *criterion, const char *value) { return Matcher_NamesMatch(criterion, value); }
#else
extern bool ResponseSystemCompare(const char *criterion, const char *value);
#endif

//=============================================================================
// These are the "matchers" that compare with wildcards ("any*" for text starting with "any")
// and operators (<3 for numbers less than 3).
// 
// Matcher_Match - Matching function using RS operators and NamesMatch wildcards/regex.
// Matcher_Regex - Uses regex functions from the std library.
// Matcher_NamesMatch - Based on Valve's original NamesMatch function, using wildcards and regex.
// 
// AppearsToBeANumber - Response System-based function which checks if the string might be a number.
//=============================================================================

bool Matcher_Match(const char *pszQuery, const char *szValue)
{
	// I wasn't kidding when I said all this did was hijack response system matching.
	return ResponseSystemCompare(pszQuery, szValue);
}

bool Matcher_Match(const char *pszQuery, int iValue) { return Matcher_Match(pszQuery, CNumStr(iValue)); }
bool Matcher_Match(const char *pszQuery, float flValue) { return Matcher_Match(pszQuery, CNumStr(flValue)); }

// -------------------------------------------------------------------------------
// -------------------------------------------------------------------------------

// The recursive part of Mapbase's modified version of Valve's NamesMatch().
bool Matcher_RunCharCompare(const char *pszQuery, const char *szValue)
{
	// This matching model is based off of the ASW SDK
	while ( *szValue && *pszQuery )
	{
		char cName = *szValue;
		char cQuery = *pszQuery;
		if ( cName != cQuery && tolower(cName) != tolower(cQuery) ) // people almost always use lowercase, so assume that first
		{
			// Now we'll try the new and improved Mapbase wildcards!
			switch (*pszQuery)
			{
				case '*':
					{
						// Return true at classic trailing *
						if ( *(pszQuery+1) == 0 )
							return true;

						if (mapbase_wildcards_enabled.GetBool())
						{
							// There's text after this * which we need to test.
							// This recursion allows for multiple wildcards
							int vlen = Q_strlen(szValue);
							++pszQuery;
							for (int i = 0; i < vlen; i++)
							{
								if (Matcher_RunCharCompare(pszQuery, szValue + i))
									return true;
							}
						}
						return false;
					} break;
				case '?':
					// Just skip if we're capable of lazy wildcards
					if (mapbase_wildcards_enabled.GetBool())
						break;
				default:
					return false;
			}
		}
		++szValue;
		++pszQuery;
	}

	// Include a classic trailing * check for when szValue is something like "value" and pszQuery is "value*"
	return ( ( *pszQuery == 0 && *szValue == 0 ) || *pszQuery == '*' );
}

// Regular expressions based off of the std library.
// The C++ is strong in this one.
bool Matcher_Regex(const char *pszQuery, const char *szValue)
{
	std::regex regex;
	
	// Since I can't find any other way to check for valid regex,
	// use a try-catch here to see if it throws an exception.
	try { regex = std::regex(pszQuery); }
	catch (std::regex_error &e)
	{
		Msg("Invalid regex \"%s\" (%s)\n", pszQuery, e.what());
		return false;
	}

	std::match_results<const char*> results;
	bool bMatch = std::regex_match( szValue, results, regex );
	if (!bMatch)
		return false;

	// Only match the *whole* string
	return Q_strlen(results.str(0).c_str()) == Q_strlen(szValue);
}

// The entry point for Mapbase's modified version of Valve's NamesMatch().
bool Matcher_NamesMatch(const char *pszQuery, const char *szValue)
{
	if ( szValue == NULL )
		return (*pszQuery == 0 || *pszQuery == '*');

	// If the pointers are identical, we're identical
	if ( szValue == pszQuery )
		return true;

	// Check for regex
	if ( *pszQuery == '@' && mapbase_regex_enabled.GetBool() )
	{
		// Make sure it has a forward slash
		// (prevents confusion with instance fixup escape)
		if (*(pszQuery+1) == '/')
		{
			return Matcher_Regex( pszQuery+2, szValue );
		}
	}

	return Matcher_RunCharCompare( pszQuery, szValue );
}

bool Matcher_NamesMatch_Classic(const char *pszQuery, const char *szValue)
{
	if ( szValue == NULL )
		return (!pszQuery || *pszQuery == 0 || *pszQuery == '*');

	// If the pointers are identical, we're identical
	if ( szValue == pszQuery )
		return true;

	while ( *szValue && *pszQuery )
	{
		unsigned char cName = *szValue;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		if ( cName == cQuery )
			;
		else if ( cName - 'A' <= (unsigned char)'Z' - 'A' && cName - 'A' + 'a' == cQuery )
			;
		else if ( cName - 'a' <= (unsigned char)'z' - 'a' && cName - 'a' + 'A' == cQuery )
			;
		else
			break;
		++szValue;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *szValue == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' )
		return true;

	return false;
}

bool Matcher_NamesMatch_MutualWildcard(const char *pszQuery, const char *szValue)
{
	if ( szValue == NULL )
		return (!pszQuery || *pszQuery == 0 || *pszQuery == '*');

	if ( pszQuery == NULL )
		return (!szValue || *szValue == 0 || *szValue == '*');

	// If the pointers are identical, we're identical
	if ( szValue == pszQuery )
		return true;

	while ( *szValue && *pszQuery )
	{
		unsigned char cName = *szValue;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		if ( cName == cQuery )
			;
		else if ( cName - 'A' <= (unsigned char)'Z' - 'A' && cName - 'A' + 'a' == cQuery )
			;
		else if ( cName - 'a' <= (unsigned char)'z' - 'a' && cName - 'a' + 'A' == cQuery )
			;
		else
			break;
		++szValue;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *szValue == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' || *szValue == '*' )
		return true;

	return false;
}

// Matcher_Compare is a deprecated alias originally used when Matcher_Match didn't support wildcards.
/*
bool Matcher_Compare(const char *pszQuery, const char *szValue)
{
	return Matcher_Match(pszQuery, szValue);
#if 0
	// I have to do this so wildcards could test *before* the response system comparison.
	// I know it removes the operators twice, but I won't worry about it.
	bool match = Matcher_NamesMatch(Matcher_RemoveOperators(pszQuery), szValue);
	if (match)
		return Matcher_Match(pszQuery, szValue);
	return false;
#endif
}
*/
