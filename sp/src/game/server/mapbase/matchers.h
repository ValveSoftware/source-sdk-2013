//========= Copyright Valve Corporation, All rights reserved. =================
//
// Purpose: General matching functions for things like wildcards and !=.
//
// $NoKeywords: $
//=============================================================================

#define MAPBASE_MATCHERS 1

extern bool ResponseSystemCompare(const char *criterion, const char *value);
//extern const char *ResponseSystemCompare(const char *criterion, const char *value, bool bReturnToken);

//=============================================================================
// These are the "matchers" that compare with wildcards (any*) and operators (<3).
// 
// Matcher_Match - Supports operators and wildcards.
// Matcher_NamesMatch - Only supports wildcards.
// Matcher_Compare - Long story short, Matcher_Match didn't support wildcards before.
// 					 This was used to support both matchers and wildcards.
//					 Now it just redirects to Matcher_Match.
// 
// AppearsToBeANumber - Checks if the string might be a number.
//						Stolen from the Response System, just like the other matchers.
//=============================================================================

// Compares with != and the like. Basically hijacks the response system matching.
// Now supports trailing wildcards (query* == querything)
// pszQuery = The value that should have the operator(s) at the beginning
// szValue = The value tested against the criterion
inline bool Matcher_Match(const char *pszQuery, const char *szValue)
{
	// I wasn't kidding when I said all this did was hijack response system matching.
	return ResponseSystemCompare(pszQuery, szValue);
	// (pszQuery && szValue) ? ResponseSystemCompare(pszQuery, szValue, pass) : FStrEq(pszQuery, szValue);
}

inline bool Matcher_Match(const char *pszQuery, int iValue)
{
	return Matcher_Match(pszQuery, UTIL_VarArgs("%i", iValue));
}

// Compares two strings with trailing wildcards. Nothing more. Use Matcher_Match if you want <, !=, etc.
// pszQuery = The value that should have the wildcard.
// szValue = The value tested against the query.
// It's basically just baseentity.cpp's NamesMatch().
FORCEINLINE bool Matcher_NamesMatch(const char *pszQuery, const char *szValue)
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

// Identical to Matcher_NamesMatch(), but either value could use a wildcard.
// pszQuery = The value that serves as the query. This value can use wildcards.
// szValue = The value tested against the query. This value can use wildcards as well.
FORCEINLINE bool Matcher_NamesMatch_MutualWildcard(const char *pszQuery, const char *szValue)
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

// While Matcher_Match didn't support wildcards,
// this was used to match with both matchers and wildcards.
// Now that Matcher_Match supports wildcards, it just redirects to that,
// but I use this anyway for consistency purposes and force-of-habit.
// pszQuery = The value that should have the operator or wildcard
// szValue = The value tested against the criterion
inline bool Matcher_Compare(const char *pszQuery, const char *szValue)
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

// Yeeted right out of the Response System.
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
