//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: General matching functions for things like wildcards and !=.
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "matchers.h"
#include "fmtstr.h"

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
