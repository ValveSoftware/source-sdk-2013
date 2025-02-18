//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: String Tools
//
//===========================================================================//

// These are redefined in the project settings to prevent anyone from using them.
// We in this module are of a higher caste and thus are privileged in their use.
#ifdef strncpy
	#undef strncpy
#endif

#ifdef _snprintf
	#undef _snprintf
#endif

#if defined( sprintf )
	#undef sprintf
#endif

#if defined( vsprintf )
	#undef vsprintf
#endif

#ifdef _vsnprintf
#ifdef _WIN32
	#undef _vsnprintf
#endif
#endif

#ifdef vsnprintf
#ifndef _WIN32
	#undef vsnprintf
#endif
#endif

#if defined( strcat )
	#undef strcat
#endif

#ifdef strncat
	#undef strncat
#endif

// NOTE: I have to include stdio + stdarg first so vsnprintf gets compiled in
#include <stdio.h>
#include <stdarg.h>

#ifdef POSIX
#include <iconv.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#define _getcwd getcwd
#elif _WIN32
#include <direct.h>
#if !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#endif

#ifdef _WIN32
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#endif
#include "tier0/dbg.h"
#include "tier1/strtools.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tier0/basetypes.h"
#include "tier1/utldict.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"
#include "tier1/fmtstr.h"
#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif
#include "tier0/memdbgon.h"

FORCEINLINE char TOLOWERC( char x )
{
	return ( x >= 'A' && x <= 'Z' ) ? (char)( x + 32 ) : x;
}
FORCEINLINE char TOUPPERC( char x )
{
	return ( x >= 'a' && x <= 'z' ) ? (char)( x - 32 ) : x;
}
static int FastToLower( char c )
{
	int i = (unsigned char) c;
	if ( i < 0x80 )
	{
		// Brutally fast branchless ASCII tolower():
		i += (((('A'-1) - i) & (i - ('Z'+1))) >> 26) & 0x20;
	}
	else
	{
		i += isupper( i ) ? 0x20 : 0;
	}
	return i;
}

void _V_memset (const char* file, int line, void *dest, int fill, int count)
{
	Assert( count >= 0 );
	AssertValidWritePtr( dest, count );

	memset(dest,fill,count);
}

void _V_memcpy (const char* file, int line, void *dest, const void *src, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( src, count );
	AssertValidWritePtr( dest, count );

	memcpy( dest, src, count );
}

void _V_memmove(const char* file, int line, void *dest, const void *src, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( src, count );
	AssertValidWritePtr( dest, count );

	memmove( dest, src, count );
}

int _V_memcmp (const char* file, int line, const void *m1, const void *m2, int count)
{
	Assert( count >= 0 );
	AssertValidReadPtr( m1, count );
	AssertValidReadPtr( m2, count );

	return memcmp( m1, m2, count );
}

int	_V_strlen(const char* file, int line, const char *str)
{
	AssertValidStringPtr(str);
	return (int)strlen( str );
}

void _V_strcpy (const char* file, int line, char *dest, const char *src)
{
	AssertValidWritePtr(dest);
	AssertValidStringPtr(src);

	strcpy( dest, src );
}

int	_V_wcslen(const char* file, int line, const wchar_t *pwch)
{
	return (int)wcslen( pwch );
}

char *_V_strrchr(const char* file, int line, const char *s, char c)
{
	AssertValidStringPtr( s );
    int len = V_strlen(s);
    s += len;
    while (len--)
	if (*--s == c) return (char *)s;
    return 0;
}

int _V_strcmp (const char* file, int line, const char *s1, const char *s2)
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	return strcmp( s1, s2 );
}

int _V_wcscmp (const char* file, int line, const wchar_t *s1, const wchar_t *s2)
{
	AssertValidReadPtr( s1 );
	AssertValidReadPtr( s2 );

	while ( *s1 == *s2 )
	{
		if ( !*s1 )
			return 0;			// strings are equal

		s1++;
		s2++;
	}

	return *s1 > *s2 ? 1 : -1;	// strings not equal
}


char *_V_strstr(const char* file, int line,  const char *s1, const char *search )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( search );

#if defined( _X360 )
	return (char *)strstr( (char *)s1, search );
#else
	return (char *)strstr( s1, search );
#endif
}

wchar_t *_V_wcsupr (const char* file, int line, wchar_t *start)
{
	return _wcsupr( start );
}


wchar_t *_V_wcslower (const char* file, int line, wchar_t *start)
{
	return _wcslwr(start);
}



char *V_strupr( char *start )
{
	unsigned char *str = (unsigned char*)start;
	while( *str )
	{
		if ( (unsigned char)(*str - 'a') <= ('z' - 'a') )
			*str -= 'a' - 'A';
		else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
			*str = toupper( *str );
		str++;
	}
	return start;
}

char *V_strtitlecase( char *start )
{
	bool bSeparator = true;
	unsigned char *str = (unsigned char*)start;
	while( *str )
	{
		if ( V_isspace( *str ) )
		{
			bSeparator = true;
		}
		else if ( bSeparator )
		{
			// Upcase
			if ( (unsigned char)(*str - 'a') <= ('z' - 'a') )
				*str -= 'a' - 'A';
			else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
				*str = toupper( *str );

			bSeparator = false;
		}
		else
		{
			// Downcase
			if ( (unsigned char)(*str - 'A') <= ('Z' - 'A') )
				*str += 'a' - 'A';
			else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
				*str = tolower( *str );
		}
		str++;
	}
	return start;
}

char *V_strlower( char *start )
{
	unsigned char *str = (unsigned char*)start;
	while( *str )
	{
		if ( (unsigned char)(*str - 'A') <= ('Z' - 'A') )
			*str += 'a' - 'A';
		else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
			*str = tolower( *str );
		str++;
	}
	return start;
}

char *V_strnlwr(char *s, size_t count)
{
	// Assert( count >= 0 ); tautology since size_t is unsigned
	AssertValidStringPtr( s, count );

	char* pRet = s;
	if ( !s || !count )
		return s;

	while ( -- count > 0 )
	{
		if ( !*s )
			return pRet; // reached end of string

		*s = tolower( *s );
		++s;
	}

	*s = 0; // null-terminate original string at "count-1"
	return pRet;
}

int V_stricmp( const char *str1, const char *str2 )
{
	// It is not uncommon to compare a string to itself. See
	// VPanelWrapper::GetPanel which does this a lot. Since stricmp
	// is expensive and pointer comparison is cheap, this simple test
	// can save a lot of cycles, and cache pollution.
	if ( str1 == str2 )
	{
		return 0;
	}
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; *s1; ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return stricmp( (const char*)s1, (const char*)s2 );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return *s2 ? -1 : 0;
}

int V_strnicmp( const char *str1, const char *str2, int n )
{
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; n > 0 && *s1; --n, ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return strnicmp( (const char*)s1, (const char*)s2, n );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return (n > 0 && *s2) ? -1 : 0;
}

int V_strncmp( const char *s1, const char *s2, int count )
{
	Assert( count >= 0 );
	AssertValidStringPtr( s1, count );
	AssertValidStringPtr( s2, count );

	while ( count > 0 )
	{
		if ( *s1 != *s2 )
			return (unsigned char)*s1 < (unsigned char)*s2 ? -1 : 1; // string different
		if ( *s1 == '\0' )
			return 0; // null terminator hit - strings the same
		s1++;
		s2++;
		count--;
	}

	return 0; // count characters compared the same
}


int V_stricmp_fast( const char *s1, const char *s2 )
{
	// This routine has historically allowed nullptr
	// but that's a bit ill-defined as it may mean
	// the calling code has a bug.  Make calls with
	// nullptr visible for fixing.
	Assert( s1 != nullptr && s2 != nullptr );
	
	// A string is always equal to itself. This optimization is
	// surprisingly valuable.
	if ( s1 == s2 )
		return 0;

	if ( s1 == nullptr )
		return -1;
	if ( s2 == nullptr )
		return 1;

	uint8 const *pS1 = ( uint8 const * ) s1;
	uint8 const *pS2 = ( uint8 const * ) s2;
	for(;;)
	{
		signed char c1 = *( pS1++ );
		signed char c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			if ( !c1 ) return 0;
		}
		else
		{
			if ( ! c2 )
			{
				return c1 - c2;
			}
			c1 = TOLOWERC( c1 );
			c2 = TOLOWERC( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
		c1 = *( pS1++ );
		c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			if ( !c1 ) return 0;
		}
		else
		{
			if ( ! c2 )
			{
				return c1 - c2;
			}
			c1 = TOLOWERC( c1 );
			c2 = TOLOWERC( c2 );
			if ( c1 != c2 )
			{
				return c1 - c2;
			}
		}
	}
}

const char *StringAfterPrefix( const char *str, const char *prefix )
{
	AssertValidStringPtr( str );
	AssertValidStringPtr( prefix );
	do
	{
		if ( !*prefix )
			return str;
	}
	while ( FastToLower( *str++ ) == FastToLower( *prefix++ ) );
	return NULL;
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------

const char *StringAfterPrefixCaseSensitive( const char *str, const char *prefix )
{
	AssertValidStringPtr( str );
	AssertValidStringPtr( prefix );
	do
	{
		if ( !*prefix )
			return str;
	}
	while ( *str++ == *prefix++ );
	return NULL;
}


int64 V_atoi64( const char *str )
{
	AssertValidStringPtr( str );

	int64             val;
	int64             sign;
	int64             c;
	
	Assert( str );
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		sign = 1;
		str++;
	}
	else
	{
		sign = 1;
	}
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}
	
	return 0;
}

uint64 V_atoui64( const char *str )
{
	AssertValidStringPtr( str );

	uint64             val;
	uint64             c;

	Assert( str );

	val = 0;

	//
	// check for hex
	//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val;
		}
	}

	//
	// check for character
	//
	if (str[0] == '\'')
	{
		return str[1];
	}

	//
	// assume decimal
	//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val;
		val = val*10 + c - '0';
	}

	return 0;
}

int V_atoi( const char *str )
{ 
	return (int)V_atoi64( str );
}

float V_atof (const char *str)
{
	AssertValidStringPtr( str );
	double			val;
	int             sign;
	int             c;
	int             decimal, total;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		sign = 1;
		str++;
	}
	else
	{
		sign = 1;
	}

	val = 0;

	//
	// check for hex
	//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

	//
	// check for character
	//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}

	//
	// assume decimal
	//
	decimal = -1;
	total = 0;
	int exponent = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			if ( decimal != -1 )
			{
				break;
			}

			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
		{
			if ( c == 'e' || c == 'E' )
			{
				exponent = V_atoi(str);
			}
			break;
		}
		val = val*10 + c - '0';
		total++;
	}

	if ( exponent != 0 )
	{
		val *= pow( 10.0, exponent );
	}
	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

//-----------------------------------------------------------------------------
// Normalizes a float string in place.  
//
// (removes leading zeros, trailing zeros after the decimal point, and the decimal point itself where possible)
//-----------------------------------------------------------------------------
void V_normalizeFloatString( char* pFloat )
{
	// If we have a decimal point, remove trailing zeroes:
	if( strchr( pFloat,'.' ) )
	{
		int len = V_strlen(pFloat);

		while( len > 1 && pFloat[len - 1] == '0' )
		{
			pFloat[len - 1] = '\0';
			len--;
		}

		if( len > 1 && pFloat[ len - 1 ] == '.' )
		{
			pFloat[len - 1] = '\0';
			len--;
		}
	}

	// TODO: Strip leading zeros

}


//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------
char const* V_stristr( char const* pStr, char const* pSearch )
{
	Assert( pStr );
	Assert( pSearch );
	if (!pStr || !pSearch) 
		return 0;

	const char* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if ( FastASCIIToLower( *pLetter ) == FastASCIIToLower( *pSearch) )
		{
			// Check for match
			const char* pMatch = pLetter + 1;
			const char* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if ( FastASCIIToLower( *pMatch) != FastASCIIToLower( *pTest ) )
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if ( *pTest == 0 )
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

char* V_stristr( char* pStr, char const* pSearch )
{
	AssertValidStringPtr( pStr );
	AssertValidStringPtr( pSearch );

	return (char*)V_stristr( (char const*)pStr, pSearch );
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test w/ length validation
//-----------------------------------------------------------------------------

char const* V_strnistr( char const* pStr, char const* pSearch, int n )
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		if ( n <= 0 )
			return 0;

		// Skip over non-matches
		if (FastToLower(*pLetter) == FastToLower(*pSearch))
		{
			int n1 = n - 1;

			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				if ( n1 <= 0 )
					return 0;

				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (FastToLower(*pMatch) != FastToLower(*pTest))
					break;

				++pMatch;
				++pTest;
				--n1;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
		--n;
	}

	return 0;
}

const char* V_strnchr( const char* pStr, char c, int n )
{
	char const* pLetter = pStr;
	char const* pLast = pStr + n;

	// Check the entire string
	while ( (pLetter < pLast) && (*pLetter != 0) )
	{
		if (*pLetter == c)
			return pLetter;
		++pLetter;
	}
	return NULL;
}

void V_strncpy( char *pDest, char const *pSrc, int maxLen )
{
	Assert( maxLen >= sizeof( *pDest ) );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pSrc );

	strncpy( pDest, pSrc, maxLen );
	if ( maxLen > 0 )
	{
		pDest[maxLen-1] = 0;
	}
}

// warning C6053: Call to 'wcsncpy' might not zero-terminate string 'pDest'
// warning C6059: Incorrect length parameter in call to 'strncat'. Pass the number of remaining characters, not the buffer size of 'argument 1'
// warning C6386: Buffer overrun: accessing 'argument 1', the writable size is 'destBufferSize' bytes, but '1000' bytes might be written
// These warnings were investigated through code inspection and writing of tests and they are
// believed to all be spurious.
#ifdef _PREFAST_
#pragma warning( push )
#pragma warning( disable : 6053 6059 6386 )
#endif

void V_wcsncpy( wchar_t *pDest, wchar_t const *pSrc, int maxLenInBytes )
{
	Assert( maxLenInBytes >= sizeof( *pDest ) );
	AssertValidWritePtr( pDest, maxLenInBytes );
	AssertValidReadPtr( pSrc );

	int maxLen = maxLenInBytes / sizeof(wchar_t);

	wcsncpy( pDest, pSrc, maxLen );
	if( maxLen )
	{
		pDest[maxLen-1] = 0;
	}
}



int V_snwprintf( wchar_t *pDest, int maxLen, const wchar_t *pFormat, ... )
{
	Assert( maxLen > 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidReadPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
#ifdef _WIN32
	int len = _vsnwprintf( pDest, maxLen, pFormat, marker );
#elif POSIX
	int len = vswprintf( pDest, maxLen, pFormat, marker );
#else
#error "define vsnwprintf type."
#endif
	va_end( marker );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLen )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}
	
	return len;
}


int V_vsnwprintf( wchar_t *pDest, int maxLen, const wchar_t *pFormat, va_list params )
{
	Assert( maxLen > 0 );

#ifdef _WIN32
	int len = _vsnwprintf( pDest, maxLen, pFormat, params );
#elif POSIX
	int len = vswprintf( pDest, maxLen, pFormat, params );
#else
#error "define vsnwprintf type."
#endif

	// Len < 0 represents an overflow
	// Len == maxLen represents exactly fitting with no NULL termination
	// Len >= maxLen represents overflow on POSIX
	if ( len < 0 || len >= maxLen )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}


int V_snprintf( char *pDest, int maxLen, char const *pFormat, ... )
{
	Assert( maxLen > 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
#ifdef _WIN32
	int len = _vsnprintf( pDest, maxLen, pFormat, marker );
#elif POSIX
	int len = vsnprintf( pDest, maxLen, pFormat, marker );
#else
	#error "define vsnprintf type."
#endif
	va_end( marker );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLen )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}


int V_vsnprintf( char *pDest, int maxLen, char const *pFormat, va_list params )
{
	Assert( maxLen > 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	int len = _vsnprintf( pDest, maxLen, pFormat, params );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLen )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}


int V_vsnprintfRet( char *pDest, int maxLen, const char *pFormat, va_list params, bool *pbTruncated )
{
	Assert( maxLen > 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	int len = _vsnprintf( pDest, maxLen, pFormat, params );

	if ( pbTruncated )
	{
		*pbTruncated = ( len < 0 || len >= maxLen );
	}

	if	( len < 0 || len >= maxLen )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}



//-----------------------------------------------------------------------------
// Purpose: If COPY_ALL_CHARACTERS == max_chars_to_copy then we try to add the whole pSrc to the end of pDest, otherwise
//  we copy only as many characters as are specified in max_chars_to_copy (or the # of characters in pSrc if thats's less).
// Input  : *pDest - destination buffer
//			*pSrc - string to append
//			destBufferSize - sizeof the buffer pointed to by pDest
//			max_chars_to_copy - COPY_ALL_CHARACTERS in pSrc or max # to copy
// Output : char * the copied buffer
//-----------------------------------------------------------------------------
char *V_strncat(char *pDest, const char *pSrc, size_t destBufferSize, int max_chars_to_copy )
{
	size_t charstocopy = (size_t)0;

	Assert( (ptrdiff_t)destBufferSize >= 0 );
	AssertValidStringPtr( pDest);
	AssertValidStringPtr( pSrc );
	
	size_t len = strlen(pDest);
	size_t srclen = strlen( pSrc );
	if ( max_chars_to_copy <= COPY_ALL_CHARACTERS )
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)min( max_chars_to_copy, (int)srclen );
	}

	if ( len + charstocopy >= destBufferSize )
	{
		charstocopy = destBufferSize - len - 1;
	}

	if ( (int)charstocopy <= 0 )
	{
		return pDest;
	}

	ANALYZE_SUPPRESS( 6059 ); // warning C6059: : Incorrect length parameter in call to 'strncat'. Pass the number of remaining characters, not the buffer size of 'argument 1'
	char *pOut = strncat( pDest, pSrc, charstocopy );
	return pOut;
}

wchar_t *V_wcsncat( INOUT_Z_CAP(cchDest) wchar_t *pDest, const wchar_t *pSrc, size_t cchDest, int max_chars_to_copy )
{
	size_t charstocopy = (size_t)0;

	Assert( (ptrdiff_t)cchDest >= 0 );
	
	size_t len = wcslen(pDest);
	size_t srclen = wcslen( pSrc );
	if ( max_chars_to_copy <= COPY_ALL_CHARACTERS )
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)min( max_chars_to_copy, (int)srclen );
	}

	if ( len + charstocopy >= cchDest )
	{
		charstocopy = cchDest - len - 1;
	}

	if ( (int)charstocopy <= 0 )
	{
		return pDest;
	}

	ANALYZE_SUPPRESS( 6059 ); // warning C6059: : Incorrect length parameter in call to 'strncat'. Pass the number of remaining characters, not the buffer size of 'argument 1'
	wchar_t *pOut = wcsncat( pDest, pSrc, charstocopy );
	return pOut;
}



//-----------------------------------------------------------------------------
// Purpose: Converts value into x.xx MB/ x.xx KB, x.xx bytes format, including commas
// Input  : value - 
//			2 - 
//			false - 
// Output : char
//-----------------------------------------------------------------------------
#define NUM_PRETIFYMEM_BUFFERS 8
char *V_pretifymem( float value, int digitsafterdecimal /*= 2*/, bool usebinaryonek /*= false*/ )
{
	static char output[ NUM_PRETIFYMEM_BUFFERS ][ 32 ];
	static int  current;

	float		onekb = usebinaryonek ? 1024.0f : 1000.0f;
	float		onemb = onekb * onekb;

	char *out = output[ current ];
	current = ( current + 1 ) & ( NUM_PRETIFYMEM_BUFFERS -1 );

	char suffix[ 8 ];

	// First figure out which bin to use
	if ( value > onemb )
	{
		value /= onemb;
		V_snprintf( suffix, sizeof( suffix ), " MB" );
	}
	else if ( value > onekb )
	{
		value /= onekb;
		V_snprintf( suffix, sizeof( suffix ), " KB" );
	}
	else
	{
		V_snprintf( suffix, sizeof( suffix ), " bytes" );
	}

	char val[ 32 ];

	// Clamp to >= 0
	digitsafterdecimal = max( digitsafterdecimal, 0 );

	// If it's basically integral, don't do any decimals
	if ( FloatMakePositive( value - (int)value ) < 0.00001 )
	{
		V_snprintf( val, sizeof( val ), "%i%s", (int)value, suffix );
	}
	else
	{
		char fmt[ 32 ];

		// Otherwise, create a format string for the decimals
		V_snprintf( fmt, sizeof( fmt ), "%%.%if%s", digitsafterdecimal, suffix );
		V_snprintf( val, sizeof( val ), fmt, value );
	}

	// Copy from in to out
	char *i = val;
	char *o = out;

	// Search for decimal or if it was integral, find the space after the raw number
	char *dot = strstr( i, "." );
	if ( !dot )
	{
		dot = strstr( i, " " );
	}

	// Compute position of dot
	int pos = dot - i;
	// Don't put a comma if it's <= 3 long
	pos -= 3;

	while ( *i )
	{
		// If pos is still valid then insert a comma every third digit, except if we would be
		//  putting one in the first spot
		if ( pos >= 0 && !( pos % 3 ) )
		{
			// Never in first spot
			if ( o != out )
			{
				*o++ = ',';
			}
		}

		// Count down comma position
		pos--;

		// Copy rest of data as normal
		*o++ = *i++;
	}

	// Terminate
	*o = 0;

	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a string representation of an integer with commas
//			separating the 1000s (ie, 37,426,421)
// Input  : value -		Value to convert
// Output : Pointer to a static buffer containing the output
//-----------------------------------------------------------------------------
#define NUM_PRETIFYNUM_BUFFERS 8 // Must be a power of two
char *V_pretifynum( int64 inputValue )
{
	static char output[ NUM_PRETIFYMEM_BUFFERS ][ 32 ];
	static int  current;

	// Point to the output buffer.
	char * const out = output[ current ];
	// Track the output buffer end for easy calculation of bytes-remaining.
	const char* const outEnd = out + sizeof( output[ current ] );

	// Point to the current output location in the output buffer.
	char *pchRender = out;
	// Move to the next output pointer.
	current = ( current + 1 ) & ( NUM_PRETIFYMEM_BUFFERS -1 );

	*out = 0;

	// In order to handle the most-negative int64 we need to negate it
	// into a uint64.
	uint64 value;
	// Render the leading minus sign, if necessary
	if ( inputValue < 0 )
	{
		V_snprintf( pchRender, 32, "-" );
		value = (uint64)-inputValue;
		// Advance our output pointer.
		pchRender += V_strlen( pchRender );
	}
	else
	{
		value = (uint64)inputValue;
	}

	// Now let's find out how big our number is. The largest number we can fit
	// into 63 bits is about 9.2e18. So, there could potentially be six
	// three-digit groups.

	// We need the initial value of 'divisor' to be big enough to divide our
	// number down to 1-999 range.
	uint64 divisor = 1;
	// Loop more than six times to avoid integer overflow.
	for ( int i = 0; i < 6; ++i )
	{
		// If our divisor is already big enough then stop.
		if ( value < divisor * 1000 )
			break;

		divisor *= 1000;
	}

	// Print the leading batch of one to three digits.
	int toPrint = value / divisor;
	V_snprintf( pchRender, outEnd - pchRender, "%d", toPrint );

	for (;;)
	{
		// Advance our output pointer.
		pchRender += V_strlen( pchRender );
		// Adjust our value to be printed and our divisor.
		value -= toPrint * divisor;
		divisor /= 1000;
		if ( !divisor )
			break;

		// The remaining blocks of digits always include a comma and three digits.
		toPrint = value / divisor;
		V_snprintf( pchRender, outEnd - pchRender, ",%03d", toPrint );
	}

	return out;
}


//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool Q_IsMeanSpaceW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x0082':	  // BREAK PERMITTED HERE
	case L'\x0083':	  // NO BREAK PERMITTED HERE
	case L'\x00A0':	  // NO-BREAK SPACE
	case L'\x034F':   // COMBINING GRAPHEME JOINER
	case L'\x2000':   // EN QUAD
	case L'\x2001':   // EM QUAD
	case L'\x2002':   // EN SPACE
	case L'\x2003':   // EM SPACE
	case L'\x2004':   // THICK SPACE
	case L'\x2005':   // MID SPACE
	case L'\x2006':   // SIX SPACE
	case L'\x2007':   // figure space
	case L'\x2008':   // PUNCTUATION SPACE
	case L'\x2009':   // THIN SPACE
	case L'\x200A':   // HAIR SPACE
	case L'\x200B':   // ZERO-WIDTH SPACE
	case L'\x200C':   // ZERO-WIDTH NON-JOINER
	case L'\x200D':   // ZERO WIDTH JOINER
	case L'\x200E':	  // LEFT-TO-RIGHT MARK
	case L'\x2028':   // LINE SEPARATOR
	case L'\x2029':   // PARAGRAPH SEPARATOR
	case L'\x202F':   // NARROW NO-BREAK SPACE
	case L'\x2060':   // word joiner
	case L'\xFEFF':   // ZERO-WIDTH NO BREAK SPACE
	case L'\xFFFC':   // OBJECT REPLACEMENT CHARACTER
		bIsMean = true;
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: strips trailing whitespace; returns pointer inside string just past
// any leading whitespace.
//
// bAggresive = true causes this function to also check for "mean" spaces,
// which we don't want in persona names or chat strings as they're disruptive
// to the user experience.
//-----------------------------------------------------------------------------
static wchar_t *StripWhitespaceWorker( int cchLength, wchar_t *pwch, bool *pbStrippedWhitespace, bool bAggressive )
{
	// walk backwards from the end of the string, killing any whitespace
	*pbStrippedWhitespace = false;

	wchar_t *pwchEnd = pwch + cchLength;
	while ( --pwchEnd >= pwch )
	{
		if ( !iswspace( *pwchEnd ) && ( !bAggressive || !Q_IsMeanSpaceW( *pwchEnd ) ) )
			break;

		*pwchEnd = 0;
		*pbStrippedWhitespace = true;
	}

	// walk forward in the string
	while ( pwch < pwchEnd )
	{
		if ( !iswspace( *pwch ) )
			break;

		*pbStrippedWhitespace = true;
		pwch++;
	}

	return pwch;
}

//-----------------------------------------------------------------------------
// Purpose: Strips all evil characters (ie. zero-width no-break space)
//			from a string.
//-----------------------------------------------------------------------------
bool Q_RemoveAllEvilCharacters( char *pch )
{
	// convert to unicode
	int cch = Q_strlen( pch );
	int cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = (wchar_t *)stackalloc( cubDest );
	int cwch = Q_UTF8ToUnicode( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;

	// Walk through and skip over evil characters
	int nWalk = 0;
	for( int i=0; i<cwch; ++i )
	{
		if( !Q_IsMeanSpaceW( pwch[i] ) )
		{
			pwch[nWalk] = pwch[i];
			++nWalk;
		}
		else
		{
			bStrippedWhitespace = true;
		}
	}

	// Null terminate
	pwch[nWalk-1] = L'\0';
	

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_UnicodeToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}


//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_StripPrecedingAndTrailingWhitespaceW( wchar_t *pwch )
{
	int cch = Q_wcslen( pwch );

	// Early out and don't convert if we don't have any chars or leading/trailing ws.
	if ( ( cch < 1 ) || ( !iswspace( pwch[ 0 ] ) && !iswspace( pwch[ cch - 1 ] ) ) )
		return false;

	// duplicate on stack
	int cubDest = ( cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwchT = (wchar_t *)stackalloc( cubDest );
	Q_wcsncpy( pwchT, pwch, cubDest );

	bool bStrippedWhitespace = false;
	pwchT = StripWhitespaceWorker( cch, pwch, &bStrippedWhitespace, false /* not aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_wcsncpy( pwch, pwchT, cubDest );
	}

	return bStrippedWhitespace;
}



//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace,
//		and also strips punctuation and formatting characters with "clear"
//		representations.
//-----------------------------------------------------------------------------
bool Q_AggressiveStripPrecedingAndTrailingWhitespaceW( wchar_t *pwch )
{
	// duplicate on stack
	int cch = Q_wcslen( pwch );
	int cubDest = ( cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwchT = (wchar_t *)stackalloc( cubDest );
	Q_wcsncpy( pwchT, pwch, cubDest );

	bool bStrippedWhitespace = false;
	pwchT = StripWhitespaceWorker( cch, pwch, &bStrippedWhitespace, true /* is aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_wcsncpy( pwch, pwchT, cubDest );
	}

	return bStrippedWhitespace;
}


//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_StripPrecedingAndTrailingWhitespace( char *pch )
{
	int cch = Q_strlen( pch );

	// Early out and don't convert if we don't have any chars or leading/trailing ws.
	if ( ( cch < 1 ) || ( !isspace( (unsigned char)pch[ 0 ] ) && !isspace( (unsigned char)pch[ cch - 1 ] ) ) )
		return false;

	// convert to unicode
	int cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = (wchar_t *)stackalloc( cubDest );
	int cwch = Q_UTF8ToUnicode( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;
	pwch = StripWhitespaceWorker( cwch-1, pwch, &bStrippedWhitespace, false /* not aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_UnicodeToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}

//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_AggressiveStripPrecedingAndTrailingWhitespace( char *pch )
{
	// convert to unicode
	int cch = Q_strlen( pch );
	int cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = (wchar_t *)stackalloc( cubDest );
	int cwch = Q_UTF8ToUnicode( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;
	pwch = StripWhitespaceWorker( cwch-1, pwch, &bStrippedWhitespace, true /* is aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_UnicodeToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a ucs2 string to a unicode (wchar_t) one, no-op on win32
//-----------------------------------------------------------------------------
int _V_UCS2ToUnicode( const ucs2 *pUCS2, wchar_t *pUnicode, int cubDestSizeInBytes )
{
	Assert( cubDestSizeInBytes >= sizeof( *pUnicode ) );
	AssertValidWritePtr(pUnicode);
	AssertValidReadPtr(pUCS2);
	
	pUnicode[0] = 0;
#ifdef _WIN32
	int cchResult = V_wcslen( pUCS2 );
	V_memcpy( pUnicode, pUCS2, cubDestSizeInBytes );
#else
	iconv_t conv_t = iconv_open( "UCS-4LE", "UCS-2LE" );
	int cchResult = -1;
	size_t nLenUnicde = cubDestSizeInBytes;
	size_t nMaxUTF8 = cubDestSizeInBytes;
	char *pIn = (char *)pUCS2;
	char *pOut = (char *)pUnicode;
	if ( conv_t != (iconv_t)-1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );
		iconv_close( conv_t );
		if ( (int)cchResult < 0 )
			cchResult = 0;
		else
			cchResult = nMaxUTF8;
	}
#endif
	pUnicode[(cubDestSizeInBytes / sizeof(wchar_t)) - 1] = 0;
	return cchResult;	

}

#ifdef _PREFAST_
#pragma warning( pop ) // Restore the /analyze warnings
#endif


//-----------------------------------------------------------------------------
// Purpose: Converts a wchar_t string into a UCS2 string -noop on windows
//-----------------------------------------------------------------------------
int _V_UnicodeToUCS2( const wchar_t *pUnicode, int cubSrcInBytes, char *pUCS2, int cubDestSizeInBytes )
{
#ifdef _WIN32
	// Figure out which buffer is smaller and convert from bytes to character
	// counts.
	int cchResult = min( cubSrcInBytes/ ((int)sizeof(wchar_t)), cubDestSizeInBytes/((int)sizeof(wchar_t)) );
	wchar_t *pDest = (wchar_t*)pUCS2;
	wcsncpy( pDest, pUnicode, cchResult );
	// Make sure we NULL-terminate.
	pDest[ cchResult - 1 ] = 0;
#elif defined (POSIX)
	iconv_t conv_t = iconv_open( "UCS-2LE", "UTF-32LE" );
	size_t cchResult = -1;
	size_t nLenUnicde = cubSrcInBytes;
	size_t nMaxUCS2 = cubDestSizeInBytes;
	char *pIn = (char*)pUnicode;
	char *pOut = pUCS2;
	if ( conv_t != (iconv_t)-1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUCS2 );
		iconv_close( conv_t );
		if ( (int)cchResult < 0 )
			cchResult = 0;
		else
			cchResult = cubSrcInBytes / sizeof( wchar_t );
	}
#else
	#error Must be implemented for this platform
#endif
	return cchResult;	
}


//-----------------------------------------------------------------------------
// Purpose: Converts a ucs-2 (windows wchar_t) string into a UTF8 (standard) string
//-----------------------------------------------------------------------------
int _V_UCS2ToUTF8( const ucs2 *pUCS2, char *pUTF8, int cubDestSizeInBytes )
{
	AssertValidStringPtr(pUTF8, cubDestSizeInBytes);
	AssertValidReadPtr(pUCS2);
	
	pUTF8[0] = 0;
#ifdef _WIN32
	// under win32 wchar_t == ucs2, sigh
	int cchResult = WideCharToMultiByte( CP_UTF8, 0, pUCS2, -1, pUTF8, cubDestSizeInBytes, NULL, NULL );
#elif defined(POSIX)
	iconv_t conv_t = iconv_open( "UTF-8", "UCS-2LE" );
	size_t cchResult = -1;

	// pUCS2 will be null-terminated so use that to work out the input
	// buffer size. Note that we shouldn't assume iconv will stop when it
	// finds a zero, and nLenUnicde should be given in bytes, so we multiply
	// it by sizeof( ucs2 ) at the end.
	size_t nLenUnicde = 0;
	while ( pUCS2[nLenUnicde] )
	{
		++nLenUnicde;
	}
	nLenUnicde *= sizeof( ucs2 );

	// Calculate number of bytes we want iconv to write, leaving space
	// for the null-terminator
	size_t nMaxUTF8 = cubDestSizeInBytes - 1;
	char *pIn = (char *)pUCS2;
	char *pOut = (char *)pUTF8;
	if ( conv_t != (iconv_t)-1 )
	{
		const size_t nBytesToWrite = nMaxUTF8;
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );

		// Calculate how many bytes were actually written and use that to
		// null-terminate our output string.
		const size_t nBytesWritten = nBytesToWrite - nMaxUTF8;
		pUTF8[nBytesWritten] = 0;

		iconv_close( conv_t );
		if ( (int)cchResult < 0 )
			cchResult = 0;
		else
			cchResult = nMaxUTF8;
	}
#endif
	pUTF8[cubDestSizeInBytes - 1] = 0;
	return cchResult;	
}


//-----------------------------------------------------------------------------
// Purpose: Converts a UTF8 to ucs-2 (windows wchar_t)
//-----------------------------------------------------------------------------
int _V_UTF8ToUCS2( const char *pUTF8, int cubSrcInBytes, ucs2 *pUCS2, int cubDestSizeInBytes )
{
	Assert( cubDestSizeInBytes >= sizeof(pUCS2[0]) );
	AssertValidStringPtr(pUTF8, cubDestSizeInBytes);
	AssertValidReadPtr(pUCS2);

	pUCS2[0] = 0;
#ifdef _WIN32
	// under win32 wchar_t == ucs2, sigh
	int cchResult = MultiByteToWideChar( CP_UTF8, 0, pUTF8, -1, pUCS2, cubDestSizeInBytes / sizeof(wchar_t) );
#elif defined( _PS3 ) // bugbug JLB
	int cchResult = 0;
	Assert( 0 );
#elif defined(POSIX)
	iconv_t conv_t = iconv_open( "UCS-2LE", "UTF-8" );
	size_t cchResult = -1;
	size_t nLenUnicde = cubSrcInBytes;
	size_t nMaxUTF8 = cubDestSizeInBytes;
	char *pIn = (char *)pUTF8;
	char *pOut = (char *)pUCS2;
	if ( conv_t != (iconv_t)-1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );
		iconv_close( conv_t );
		if ( (int)cchResult < 0 )
			cchResult = 0;
		else
			cchResult = cubSrcInBytes;

	}
#endif
	pUCS2[ (cubDestSizeInBytes/sizeof(ucs2)) - 1] = 0;
	return cchResult;	
}



//-----------------------------------------------------------------------------
// Purpose: Returns the 4 bit nibble for a hex character
// Input  : c - 
// Output : unsigned char
//-----------------------------------------------------------------------------
unsigned char V_nibble( char c )
{
	if ( ( c >= '0' ) &&
		 ( c <= '9' ) )
	{
		 return (unsigned char)(c - '0');
	}

	if ( ( c >= 'A' ) &&
		 ( c <= 'F' ) )
	{
		 return (unsigned char)(c - 'A' + 0x0a);
	}

	if ( ( c >= 'a' ) &&
		 ( c <= 'f' ) )
	{
		 return (unsigned char)(c - 'a' + 0x0a);
	}

	return '0';
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			numchars - 
//			*out - 
//			maxoutputbytes - 
//-----------------------------------------------------------------------------
void V_hextobinary( char const *in, int numchars, byte *out, int maxoutputbytes )
{
	int len = V_strlen( in );
	numchars = min( len, numchars );
	// Make sure it's even
	numchars = ( numchars ) & ~0x1;

	// Must be an even # of input characters (two chars per output byte)
	Assert( numchars >= 2 );

	memset( out, 0x00, maxoutputbytes );

	byte *p;
	int i;

	p = out;
	for ( i = 0; 
		 ( i < numchars ) && ( ( p - out ) < maxoutputbytes ); 
		 i+=2, p++ )
	{
		*p = ( V_nibble( in[i] ) << 4 ) | V_nibble( in[i+1] );		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			inputbytes - 
//			*out - 
//			outsize - 
//-----------------------------------------------------------------------------
void V_binarytohex( const byte *in, int inputbytes, char *out, int outsize )
{
	Assert( outsize >= 1 );
	char doublet[10];
	int i;

	out[0]=0;

	for ( i = 0; i < inputbytes; i++ )
	{
		unsigned char c = in[i];
		V_snprintf( doublet, sizeof( doublet ), "%02x", c );
		V_strncat( out, doublet, outsize, COPY_ALL_CHARACTERS );
	}
}

// Even though \ on Posix (Linux&Mac) isn't techincally a path separator we are
// now counting it as one even Posix since so many times our filepaths aren't actual
// paths but rather text strings passed in from data files, treating \ as a pathseparator
// covers the full range of cases
bool PATHSEPARATOR( char c )
{
	return c == '\\' || c == '/';
}



// A special high-performance case-insensitive compare function
// returns 0 if strings match exactly
// returns >0 if strings match in a case-insensitive way, but do not match exactly
// returns <0 if strings do not match even in a case-insensitive way
int	_V_stricmp_NegativeForUnequal( const char *s1, const char *s2 )
{
	//VPROF_2( "V_stricmp", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, BUDGETFLAG_ALL );

	// It is not uncommon to compare a string to itself. Since stricmp
	// is expensive and pointer comparison is cheap, this simple test
	// can save a lot of cycles, and cache pollution.
	if ( s1 == s2 )
		return 0;

	uint8 const *pS1 = ( uint8 const * ) s1;
	uint8 const *pS2 = ( uint8 const * ) s2;
	int iExactMatchResult = 1;
	for(;;)
	{
		int c1 = *( pS1++ );
		int c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			// strings are case-insensitive equal, coerce accumulated
			// case-difference to 0/1 and return it
			if ( !c1 ) return !iExactMatchResult;
		}
		else
		{
			if ( ! c2 )
			{
				// c2=0 and != c1  =>  not equal
				return -1;
			}
			iExactMatchResult = 0;
			c1 = FastASCIIToLower( c1 );
			c2 = FastASCIIToLower( c2 );
			if ( c1 != c2 )
			{
				// strings are not equal
				return -1;
			}
		}
		c1 = *( pS1++ );
		c2 = *( pS2++ );
		if ( c1 == c2 )
		{
			// strings are case-insensitive equal, coerce accumulated
			// case-difference to 0/1 and return it
			if ( !c1 ) return !iExactMatchResult;
		}
		else
		{
			if ( ! c2 )
			{
				// c2=0 and != c1  =>  not equal
				return -1;
			}
			iExactMatchResult = 0;
			c1 = FastASCIIToLower( c1 );
			c2 = FastASCIIToLower( c2 );
			if ( c1 != c2 )
			{
				// strings are not equal
				return -1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
// Input  : *in - 
//			*out - 
//			maxlen - 
//-----------------------------------------------------------------------------
void V_FileBase( const char *in, char *out, int maxlen )
{
	Assert( maxlen >= 1 );
	Assert( in );
	Assert( out );

	if ( !in || !in[ 0 ] )
	{
		*out = 0;
		return;
	}

	int len, start, end;

	len = V_strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end&& in[end] != '.' && !PATHSEPARATOR( in[end] ) )
	{
		end--;
	}
	
	if ( in[end] != '.' )		// no '.', copy to end
	{
		end = len-1;
	}
	else 
	{
		end--;					// Found ',', copy to left of '.'
	}

	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && !PATHSEPARATOR( in[start] ) )
	{
		start--;
	}

	if ( start < 0 || !PATHSEPARATOR( in[start] ) )
	{
		start = 0;
	}
	else 
	{
		start++;
	}

	// Length of new sting
	len = end - start + 1;

	int maxcopy = min( len + 1, maxlen );

	// Copy partial string
	V_strncpy( out, &in[start], maxcopy );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppath - 
//-----------------------------------------------------------------------------
void V_StripTrailingSlash( char *ppath )
{
	Assert( ppath );

	int len = V_strlen( ppath );
	if ( len > 0 )
	{
		if ( PATHSEPARATOR( ppath[ len - 1 ] ) )
		{
			ppath[ len - 1 ] = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripTrailingWhitespace( char *ppline )
{
	Assert( ppline );

	int len = V_strlen( ppline );
	while ( len > 0 )
	{
		if ( !V_isspace( ppline[ len - 1 ] ) )
			break;
		ppline[ len - 1 ] = 0;
		len--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripLeadingWhitespace( char *ppline )
{
	Assert( ppline );

	// Skip past initial whitespace
	int skip = 0;
	while( V_isspace( ppline[ skip ] ) )
		skip++;
	// Shuffle the rest of the string back (including the NULL-terminator)
	if ( skip )
	{
		while( ( ppline[0] = ppline[skip] ) != 0 )
			ppline++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripSurroundingQuotes( char *ppline )
{
	Assert( ppline );

	int len = V_strlen( ppline ) - 2;
	if ( ( ppline[0] == '"' ) && ( len >= 0 ) && ( ppline[len+1] == '"' ) )
	{
		for ( int i = 0; i < len; i++ )
			ppline[i] = ppline[i+1];
		ppline[len] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			*out - 
//			outSize - 
//-----------------------------------------------------------------------------
void V_StripExtension( const char *in, char *out, int outSize )
{
	// Find the last dot. If it's followed by a dot or a slash, then it's part of a 
	// directory specifier like ../../somedir/./blah.

	// scan backward for '.'
	int end = V_strlen( in ) - 1;
	while ( end > 0 && in[end] != '.' && !PATHSEPARATOR( in[end] ) )
	{
		--end;
	}

	if (end > 0 && !PATHSEPARATOR( in[end] ) && end < outSize)
	{
		int nChars = min( end, outSize-1 );
		if ( out != in )
		{
			memcpy( out, in, nChars );
		}
		out[nChars] = 0;
	}
	else
	{
		// nothing found
		if ( out != in )
		{
			V_strncpy( out, in, outSize );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*extension - 
//			pathStringLength - 
//-----------------------------------------------------------------------------
void V_DefaultExtension( char *path, const char *extension, int pathStringLength )
{
	Assert( path );
	Assert( pathStringLength >= 1 );
	Assert( extension );
	Assert( extension[0] == '.' );

	char    *src;

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	src = path + V_strlen(path) - 1;

	while ( !PATHSEPARATOR( *src ) && ( src > path ) )
	{
		if (*src == '.')
		{
			// it has an extension
			return;                 
		}
		src--;
	}

	// Concatenate the desired extension
	V_strncat( path, extension, pathStringLength, COPY_ALL_CHARACTERS );
}

//-----------------------------------------------------------------------------
// Purpose: Force extension...
// Input  : *path - 
//			*extension - 
//			pathStringLength - 
//-----------------------------------------------------------------------------
void V_SetExtension( char *path, const char *extension, int pathStringLength )
{
	V_StripExtension( path, path, pathStringLength );

	// We either had an extension and stripped it, or didn't have an extension
	// at all. Either way, we need to concatenate our extension now.

	// extension is not required to start with '.', so if it's not there,
	// then append that first.
	if ( extension[0] != '.' )
	{
		V_strncat( path, ".", pathStringLength, COPY_ALL_CHARACTERS );
	}

	V_strncat( path, extension, pathStringLength, COPY_ALL_CHARACTERS );
}

//-----------------------------------------------------------------------------
// Purpose: Remove final filename from string
// Input  : *path - 
// Output : void  V_StripFilename
//-----------------------------------------------------------------------------
void  V_StripFilename (char *path)
{
	int             length;

	length = V_strlen( path )-1;
	if ( length <= 0 )
		return;

	while ( length > 0 && 
		!PATHSEPARATOR( path[length] ) )
	{
		length--;
	}

	path[ length ] = 0;
}

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif POSIX
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

//-----------------------------------------------------------------------------
// Purpose: Changes all '/' or '\' characters into separator
// Input  : *pname - 
//			separator - 
//-----------------------------------------------------------------------------
void V_FixSlashes( char *pname, char separator /* = CORRECT_PATH_SEPARATOR */ )
{
	while ( *pname )
	{
		if ( *pname == INCORRECT_PATH_SEPARATOR || *pname == CORRECT_PATH_SEPARATOR )
		{
			*pname = separator;
		}
		pname++;
	}
}


//-----------------------------------------------------------------------------
// Purpose: This function fixes cases of filenames like materials\\blah.vmt or somepath\otherpath\\ and removes the extra double slash.
//-----------------------------------------------------------------------------
void V_FixDoubleSlashes( char *pStr )
{
	int len = V_strlen( pStr );

	for ( int i=1; i < len-1; i++ )
	{
		if ( (pStr[i] == '/' || pStr[i] == '\\') && (pStr[i+1] == '/' || pStr[i+1] == '\\') )
		{
			// This means there's a double slash somewhere past the start of the filename. That 
			// can happen in Hammer if they use a material in the root directory. You'll get a filename 
			// that looks like 'materials\\blah.vmt'
			V_memmove( &pStr[i], &pStr[i+1], len - i );
			--len;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Strip off the last directory from dirName
// Input  : *dirName - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool V_StripLastDir( char *dirName, int maxlen )
{
	if( dirName[0] == 0 || 
		!V_stricmp( dirName, "./" ) || 
		!V_stricmp( dirName, ".\\" ) )
		return false;
	
	int len = V_strlen( dirName );

	Assert( len < maxlen );

	// skip trailing slash
	if ( PATHSEPARATOR( dirName[len-1] ) )
	{
		len--;
	}

	while ( len > 0 )
	{
		if ( PATHSEPARATOR( dirName[len-1] ) )
		{
			dirName[len] = 0;
			V_FixSlashes( dirName, CORRECT_PATH_SEPARATOR );
			return true;
		}
		len--;
	}

	// Allow it to return an empty string and true. This can happen if something like "tf2/" is passed in.
	// The correct behavior is to strip off the last directory ("tf2") and return true.
	if( len == 0 )
	{
		V_snprintf( dirName, maxlen, ".%c", CORRECT_PATH_SEPARATOR );
		return true;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the beginning of the unqualified file name 
//			(no path information)
// Input:	in - file name (may be unqualified, relative or absolute path)
// Output:	pointer to unqualified file name
//-----------------------------------------------------------------------------
const char * V_UnqualifiedFileName( const char * in )
{
	// back up until the character after the first path separator we find,
	// or the beginning of the string
	const char * out = in + strlen( in ) - 1;
	while ( ( out > in ) && ( !PATHSEPARATOR( *( out-1 ) ) ) )
		out--;
	return out;
}


//-----------------------------------------------------------------------------
// Purpose: Composes a path and filename together, inserting a path separator
//			if need be
// Input:	path - path to use
//			filename - filename to use
//			dest - buffer to compose result in
//			destSize - size of destination buffer
//-----------------------------------------------------------------------------
void V_ComposeFileName( const char *path, const char *filename, char *dest, int destSize )
{
	V_strncpy( dest, path, destSize );
	V_FixSlashes( dest );
	V_AppendSlash( dest, destSize );
	V_strncat( dest, filename, destSize, COPY_ALL_CHARACTERS );
	V_FixSlashes( dest );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFilePath
//-----------------------------------------------------------------------------
bool V_ExtractFilePath (const char *path, char *dest, int destSize )
{
	Assert( destSize >= 1 );
	if ( destSize < 1 )
	{
		return false;
	}

	// Last char
	int len = V_strlen(path);
	const char *src = path + (len ? len-1 : 0);

	// back up until a \ or the start
	while ( src != path && !PATHSEPARATOR( *(src-1) ) )
	{
		src--;
	}

	int copysize = min( (int)((ptrdiff_t)src - (ptrdiff_t)path), destSize - 1 );
	memcpy( dest, path, copysize );
	dest[copysize] = 0;

	return copysize != 0 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFileExtension
//-----------------------------------------------------------------------------
void V_ExtractFileExtension( const char *path, char *dest, int destSize )
{
	*dest = NULL;
	const char * extension = V_GetFileExtension( path );
	if ( NULL != extension )
		V_strncpy( dest, extension, destSize );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the file extension within a file name string
// Input:	in - file name 
// Output:	pointer to beginning of extension (after the "."), or NULL
//				if there is no extension
//-----------------------------------------------------------------------------
const char * V_GetFileExtension( const char * path )
{
	const char    *src;

	const size_t len = strlen(path);
	if (len < 1)
		{ return NULL; }

	src = path + len - 1;


//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.' )
		src--;

	// check to see if the '.' is part of a pathname
	if (src == path || PATHSEPARATOR( *src ) )
	{		
		return NULL;  // no extension
	}

	return src;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the filename part of a path string
// Input:	in - file name 
// Output:	pointer to beginning of filename (after the "/"). If there were no /, 
//          output is identical to input
//-----------------------------------------------------------------------------
const char * V_GetFileName( const char * path )
{
	return V_UnqualifiedFileName( path );
}


bool V_RemoveDotSlashes( char *pFilename, char separator, bool bRemoveDoubleSlashes /* = true */ )
{
	// McJohn - 2021 - I've hand integrated steam's version of this function. 
	// make all slashes the requested separator
	V_FixSlashes( pFilename, separator );

	int len = (int) strlen( pFilename );

	// Fix the double slashes for real.
	int i = 1; // We start at character 1 to skip over intentional UNC paths. Maybe this is never desirable.
	while ( i <= ( len - 2 ) )
	{
		bool bDoubleSlash = pFilename[ i ] == separator
			&& pFilename[ i + 1 ] == separator;
		if ( bDoubleSlash )
		{
			memmove( pFilename + i, pFilename + i + 1, len - i );
			len -= 1;
			continue;
		}

		++i;
	}

	// strip all "/./" and replace by "/"
	i = 0;
	while ( i <= ( len - 3 ) )
	{
		if ( pFilename[ i ] == separator && pFilename[ i + 1 ] == '.' && pFilename[ i + 2 ] == separator )
		{
			memmove( pFilename + i, pFilename + i + 2, len - i - 1 );
			len -= 2; // stripped /. leaving the /
		}
		else
		{
			i++;
		}
	}

	// Replace ./ with / except for if it's the very first character, in which case we remove both (ie:
	//   ./foo -> foo
	//   k./foo -> k/foo
	//   foo./ -> foo/
	//   ../foo -> ../foo
	i = 0;
	while ( i <= ( len - 2 ) )
	{
		if ( i == 0 )
		{
			// ./foo -> foo
			if ( pFilename[ i ] == '.' && pFilename[ i + 1 ] == separator )
			{
				memmove( pFilename, pFilename + 2, len - 1 );
				len -= 2;
				continue;
			}
		}
		else
		{
			bool bIsUselessDotSlash = pFilename[ i - 1 ] != '.'
				&& pFilename[ i ] == '.'
				&& pFilename[ i + 1 ] == separator;

			if ( bIsUselessDotSlash )
			{
				memmove( pFilename + i, pFilename + i + 1, len - i );
				len -= 1; // stripped ./ leaving the /	
			}
		}

		// Advance the read location.
		++i;
	}


	// Get rid of a trailing "/." (needless). 
	// McJohn - Divergence from Steam here: We do not leave the trailing /, because that would change
	// the expectations of the calling code (previously they had a path that looked like /bar/foo/., 
	// and if they wanted to concat to that path they would need to add a / first--but if we 
	// leave behind the / then they don't need that anymore). 
	if ( len > 2 && pFilename[ len - 1 ] == '.' && separator == pFilename[ len - 2 ] )
	{
		pFilename[ len - 2 ] = 0;
		len -= 2;
	}


	// get rid of leading "./"
	if ( len > 1 && pFilename[ 0 ] == '.' && pFilename[ 1 ] == separator )
	{
		memmove( pFilename, pFilename + 2, len - 1 );
		len -= 2;
	}

	// Each time we encounter a "..", back up until we've read the previous directory name,
	// then get rid of it.
	char *pIn = pFilename;
	while ( *pIn )
	{
		if ( pIn[ 0 ] == '.' &&
			pIn[ 1 ] == '.' &&
			( pIn == pFilename || separator == pIn[ -1 ] ) &&	// Preceding character must be a slash.
			( pIn[ 2 ] == 0 || separator == pIn[ 2 ] ) )			// Following character must be a slash or the end of the string.
		{
			// If there is no previous directory we cannot go up
			if ( pIn == pFilename || pIn - 1 == pFilename )
				return false;

			char *pEndOfDots = pIn + 2;
			char *pStart = pIn - 2;

			// Ok, now scan back for the path separator that starts the preceding directory.
			while ( pStart > pFilename && separator != *pStart )
			{
				--pStart;
			}

			// If we hit the beginning of the path, then without this adjustment we will turn a 
			// relative path into an absolute path--but that is not correct.
			if ( pStart == pFilename && *pEndOfDots )
				pEndOfDots += 1;

			// Now slide the string down to get rid of the previous directory and the ".."
			memmove( pStart, pEndOfDots, strlen( pEndOfDots ) + 1 );

			// Start over.
			pIn = pFilename;
		}
		else
		{
			++pIn;
		}
	}

	return true;
}


void V_AppendSlash( char *pStr, int strSize )
{
	int len = V_strlen( pStr );
	if ( len > 0 && !PATHSEPARATOR(pStr[len-1]) )
	{
		if ( len+1 >= strSize )
			Error( "V_AppendSlash: ran out of space on %s.", pStr );
		
		pStr[len] = CORRECT_PATH_SEPARATOR;
		pStr[len+1] = 0;
	}
}


void V_MakeAbsolutePath( char *pOut, int outLen, const char *pPath, const char *pStartingDir )
{
	if ( V_IsAbsolutePath( pPath ) )
	{
		// pPath is not relative.. just copy it.
		V_strncpy( pOut, pPath, outLen );
	}
	else
	{
		// Make sure the starting directory is absolute..
		if ( pStartingDir && V_IsAbsolutePath( pStartingDir ) )
		{
			V_strncpy( pOut, pStartingDir, outLen );
		}
		else
		{
			if ( !_getcwd( pOut, outLen ) )
				Error( "V_MakeAbsolutePath: _getcwd failed." );

			if ( pStartingDir )
			{
				V_AppendSlash( pOut, outLen );
				V_strncat( pOut, pStartingDir, outLen, COPY_ALL_CHARACTERS );
			}
		}

		// Concatenate the paths.
		V_AppendSlash( pOut, outLen );
		V_strncat( pOut, pPath, outLen, COPY_ALL_CHARACTERS );
	}

	if ( !V_RemoveDotSlashes( pOut ) )
		Error( "V_MakeAbsolutePath: tried to \"..\" past the root." );

	//V_FixSlashes( pOut ); - handled by V_RemoveDotSlashes
}


//-----------------------------------------------------------------------------
// Makes a relative path
//-----------------------------------------------------------------------------
bool V_MakeRelativePath( const char *pFullPath, const char *pDirectory, char *pRelativePath, int nBufLen )
{
	pRelativePath[0] = 0;

	const char *pPath = pFullPath;
	const char *pDir = pDirectory;

	// Strip out common parts of the path
	const char *pLastCommonPath = NULL;
	const char *pLastCommonDir = NULL;
	while ( *pPath && ( FastToLower( *pPath ) == FastToLower( *pDir ) || 
						( PATHSEPARATOR( *pPath ) && ( PATHSEPARATOR( *pDir ) || (*pDir == 0) ) ) ) )
	{
		if ( PATHSEPARATOR( *pPath ) )
		{
			pLastCommonPath = pPath + 1;
			pLastCommonDir = pDir + 1;
		}
		if ( *pDir == 0 )
		{
			--pLastCommonDir;
			break;
		}
		++pDir; ++pPath;
	}

	// Nothing in common
	if ( !pLastCommonPath )
		return false;

	// For each path separator remaining in the dir, need a ../
	int nOutLen = 0;
	bool bLastCharWasSeparator = true;
	for ( ; *pLastCommonDir; ++pLastCommonDir )
	{
		if ( PATHSEPARATOR( *pLastCommonDir ) )
		{
			pRelativePath[nOutLen++] = '.';
			pRelativePath[nOutLen++] = '.';
			pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
			bLastCharWasSeparator = true;
		}
		else
		{
			bLastCharWasSeparator = false;
		}
	}

	// Deal with relative paths not specified with a trailing slash
	if ( !bLastCharWasSeparator )
	{
		pRelativePath[nOutLen++] = '.';
		pRelativePath[nOutLen++] = '.';
		pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
	}

	// Copy the remaining part of the relative path over, fixing the path separators
	for ( ; *pLastCommonPath; ++pLastCommonPath )
	{
		if ( PATHSEPARATOR( *pLastCommonPath ) )
		{
			pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
		}
		else
		{
			pRelativePath[nOutLen++] = *pLastCommonPath;
		}

		// Check for overflow
		if ( nOutLen == nBufLen - 1 )
			break;
	}

	pRelativePath[nOutLen] = 0;
	return true;
}


//-----------------------------------------------------------------------------
// small helper function shared by lots of modules
//-----------------------------------------------------------------------------
bool V_IsAbsolutePath( const char *pStr )
{
	bool bIsAbsolute = ( pStr[0] && pStr[1] == ':' ) || pStr[0] == '/' || pStr[0] == '\\';
	if ( IsX360() && !bIsAbsolute )
	{
		bIsAbsolute = ( V_stristr( pStr, ":" ) != NULL );
	}
	return bIsAbsolute;
}


// Copies at most nCharsToCopy bytes from pIn into pOut.
// Returns false if it would have overflowed pOut's buffer.
static bool CopyToMaxChars( char *pOut, int outSize, const char *pIn, int nCharsToCopy )
{
	if ( outSize == 0 )
		return false;

	int iOut = 0;
	while ( *pIn && nCharsToCopy > 0 )
	{
		if ( iOut == (outSize-1) )
		{
			pOut[iOut] = 0;
			return false;
		}
		pOut[iOut] = *pIn;
		++iOut;
		++pIn;
		--nCharsToCopy;
	}
	
	pOut[iOut] = 0;
	return true;
}


//-----------------------------------------------------------------------------
// Fixes up a file name, removing dot slashes, fixing slashes, converting to lowercase, etc.
//-----------------------------------------------------------------------------
void V_FixupPathName( char *pOut, int nOutLen, const char *pPath )
{
	V_strncpy( pOut, pPath, nOutLen );
	V_RemoveDotSlashes( pOut, CORRECT_PATH_SEPARATOR, true );
#ifdef WIN32
	V_strlower( pOut );
#endif
}


// Returns true if it completed successfully.
// If it would overflow pOut, it fills as much as it can and returns false.
bool V_StrSubst( 
	const char *pIn, 
	const char *pMatch,
	const char *pReplaceWith,
	char *pOut,
	int outLen,
	bool bCaseSensitive
	)
{
	int replaceFromLen = V_strlen( pMatch );
	int replaceToLen = V_strlen( pReplaceWith );

	const char *pInStart = pIn;
	char *pOutPos = pOut;
	pOutPos[0] = 0;

	while ( 1 )
	{
		int nRemainingOut = outLen - (pOutPos - pOut);

		const char *pTestPos = ( bCaseSensitive ? strstr( pInStart, pMatch ) : V_stristr( pInStart, pMatch ) );
		if ( pTestPos )
		{
			// Found an occurence of pMatch. First, copy whatever leads up to the string.
			int copyLen = pTestPos - pInStart;
			if ( !CopyToMaxChars( pOutPos, nRemainingOut, pInStart, copyLen ) )
				return false;
			
			// Did we hit the end of the output string?
			if ( copyLen > nRemainingOut-1 )
				return false;

			pOutPos += strlen( pOutPos );
			nRemainingOut = outLen - (pOutPos - pOut);

			// Now add the replacement string.
			if ( !CopyToMaxChars( pOutPos, nRemainingOut, pReplaceWith, replaceToLen ) )
				return false;

			pInStart += copyLen + replaceFromLen;
			pOutPos += replaceToLen;			
		}
		else
		{
			// We're at the end of pIn. Copy whatever remains and get out.
			int copyLen = V_strlen( pInStart );
			V_strncpy( pOutPos, pInStart, nRemainingOut );
			return ( copyLen <= nRemainingOut-1 );
		}
	}
}


char* AllocString( const char *pStr, int nMaxChars )
{
	int allocLen;
	if ( nMaxChars == -1 )
		allocLen = V_strlen( pStr ) + 1;
	else
		allocLen = min( (int)V_strlen(pStr), nMaxChars ) + 1;

	char *pOut = new char[allocLen];
	V_strncpy( pOut, pStr, allocLen );
	return pOut;
}


void V_SplitString2( const char *pString, const char **pSeparators, int nSeparators, CUtlVector<char*> &outStrings )
{
	outStrings.Purge();
	const char *pCurPos = pString;
	while ( 1 )
	{
		int iFirstSeparator = -1;
		const char *pFirstSeparator = 0;
		for ( int i=0; i < nSeparators; i++ )
		{
			const char *pTest = V_stristr( pCurPos, pSeparators[i] );
			if ( pTest && (!pFirstSeparator || pTest < pFirstSeparator) )
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if ( pFirstSeparator )
		{
			// Split on this separator and continue on.
			int separatorLen = V_strlen( pSeparators[iFirstSeparator] );
			if ( pFirstSeparator > pCurPos )
			{
				outStrings.AddToTail( AllocString( pCurPos, pFirstSeparator-pCurPos ) );
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if ( V_strlen( pCurPos ) )
			{
				outStrings.AddToTail( AllocString( pCurPos, -1 ) );
			}
			return;
		}
	}
}

void V_SplitString2( const char *pString, const char * const *pSeparators, int nSeparators, CUtlVector<CUtlString> &outStrings, bool bIncludeEmptyStrings )
{
	outStrings.Purge();
	const char *pCurPos = pString;
	for ( ;; )
	{
		int iFirstSeparator = -1;
		const char *pFirstSeparator = 0;
		for ( int i = 0; i < nSeparators; i++ )
		{
			const char *pTest = V_stristr_fast( pCurPos, pSeparators[i] );
			if ( pTest && (!pFirstSeparator || pTest < pFirstSeparator) )
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if ( pFirstSeparator )
		{
			// Split on this separator and continue on.
			int separatorLen = (int)V_strlen( pSeparators[iFirstSeparator] );
			if ( pFirstSeparator > pCurPos || (pFirstSeparator == pCurPos && bIncludeEmptyStrings) )
			{
				outStrings[outStrings.AddToTail()].SetDirect( pCurPos, (int)( pFirstSeparator - pCurPos ) );
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string, if there's anything there
			if ( pCurPos[0] != 0 )
			{
				outStrings[outStrings.AddToTail()].Set( pCurPos );
			}
			return;
		}
	}
}

void V_SplitString( const char *pString, const char *pSeparator, CUtlVector<char*> &outStrings )
{
	V_SplitString2( pString, &pSeparator, 1, outStrings );
}

void V_SplitString( const char *pString, const char *pSeparator, CUtlVector<CUtlString> &outStrings, bool bIncludeEmptyStrings )
{
	V_SplitString2( pString, &pSeparator, 1, outStrings, bIncludeEmptyStrings );
}


bool V_GetCurrentDirectory( char *pOut, int maxLen )
{
	return _getcwd( pOut, maxLen ) == pOut;
}


bool V_SetCurrentDirectory( const char *pDirName )
{
	return _chdir( pDirName ) == 0;
}


// This function takes a slice out of pStr and stores it in pOut.
// It follows the Python slice convention:
// Negative numbers wrap around the string (-1 references the last character).
// Numbers are clamped to the end of the string.
void V_StrSlice( const char *pStr, int firstChar, int lastCharNonInclusive, char *pOut, int outSize )
{
	if ( outSize == 0 )
		return;
	
	int length = V_strlen( pStr );

	// Fixup the string indices.
	if ( firstChar < 0 )
	{
		firstChar = length - (-firstChar % length);
	}
	else if ( firstChar >= length )
	{
		pOut[0] = 0;
		return;
	}

	if ( lastCharNonInclusive < 0 )
	{
		lastCharNonInclusive = length - (-lastCharNonInclusive % length);
	}
	else if ( lastCharNonInclusive > length )
	{
		lastCharNonInclusive %= length;
	}

	if ( lastCharNonInclusive <= firstChar )
	{
		pOut[0] = 0;
		return;
	}

	int copyLen = lastCharNonInclusive - firstChar;
	if ( copyLen <= (outSize-1) )
	{
		memcpy( pOut, &pStr[firstChar], copyLen );
		pOut[copyLen] = 0;
	}
	else
	{
		memcpy( pOut, &pStr[firstChar], outSize-1 );
		pOut[outSize-1] = 0;
	}
}


void V_StrLeft( const char *pStr, int nChars, char *pOut, int outSize )
{
	if ( nChars == 0 )
	{
		if ( outSize != 0 )
			pOut[0] = 0;

		return;
	}

	V_StrSlice( pStr, 0, nChars, pOut, outSize );
}


void V_StrRight( const char *pStr, int nChars, char *pOut, int outSize )
{
	int len = V_strlen( pStr );
	if ( nChars >= len )
	{
		V_strncpy( pOut, pStr, outSize );
	}
	else
	{
		V_StrSlice( pStr, -nChars, V_strlen( pStr ), pOut, outSize );
	}
}

//-----------------------------------------------------------------------------
// Convert multibyte to wchar + back
//-----------------------------------------------------------------------------
void V_strtowcs( const char *pString, int nInSize, wchar_t *pWString, int nOutSizeInBytes )
{
	Assert( nOutSizeInBytes >= sizeof(pWString[0]) );
#ifdef _WIN32
	int nOutSizeInChars = nOutSizeInBytes / sizeof(pWString[0]);
	int result = MultiByteToWideChar( CP_UTF8, 0, pString, nInSize, pWString, nOutSizeInChars );
	// If the string completely fails to fit then MultiByteToWideChar will return 0.
	// If the string exactly fits but with no room for a null-terminator then MultiByteToWideChar
	// will happily fill the buffer and omit the null-terminator, returning nOutSizeInChars.
	// Either way we need to return an empty string rather than a bogus and possibly not
	// null-terminated result.
	if ( result <= 0 || result >= nOutSizeInChars )
	{
		// If nInSize includes the null-terminator then a result of nOutSizeInChars is
		// legal. We check this by seeing if the last character in the output buffer is
		// a zero.
		if ( result == nOutSizeInChars && pWString[ nOutSizeInChars - 1 ] == 0)
		{
			// We're okay! Do nothing.
		}
		else
		{
			// The string completely to fit. Null-terminate the buffer.
			*pWString = L'\0';
		}
	}
	else
	{
		// We have successfully converted our string. Now we need to null-terminate it, because
		// MultiByteToWideChar will only do that if nInSize includes the source null-terminator!
		pWString[ result ] = 0;
	}
#elif POSIX
	if ( mbstowcs( pWString, pString, nOutSizeInBytes / sizeof(pWString[0]) ) <= 0 )
	{
		*pWString = 0;
	}
#endif
}

void V_wcstostr( const wchar_t *pWString, int nInSize, char *pString, int nOutSizeInChars )
{
#ifdef _WIN32
	int result = WideCharToMultiByte( CP_UTF8, 0, pWString, nInSize, pString, nOutSizeInChars, NULL, NULL );
	// If the string completely fails to fit then MultiByteToWideChar will return 0.
	// If the string exactly fits but with no room for a null-terminator then MultiByteToWideChar
	// will happily fill the buffer and omit the null-terminator, returning nOutSizeInChars.
	// Either way we need to return an empty string rather than a bogus and possibly not
	// null-terminated result.
	if ( result <= 0 || result >= nOutSizeInChars )
	{
		// If nInSize includes the null-terminator then a result of nOutSizeInChars is
		// legal. We check this by seeing if the last character in the output buffer is
		// a zero.
		if ( result == nOutSizeInChars && pWString[ nOutSizeInChars - 1 ] == 0)
		{
			// We're okay! Do nothing.
		}
		else
		{
			*pString = '\0';
		}
	}
	else
	{
		// We have successfully converted our string. Now we need to null-terminate it, because
		// MultiByteToWideChar will only do that if nInSize includes the source null-terminator!
		pString[ result ] = '\0';
	}
#elif POSIX
	if ( wcstombs( pString, pWString, nOutSizeInChars ) <= 0 )
	{
		*pString = '\0';
	}
#endif
}



//--------------------------------------------------------------------------------
// backslashification
//--------------------------------------------------------------------------------

static char s_BackSlashMap[]="\tt\nn\rr\"\"\\\\";

char *V_AddBackSlashesToSpecialChars( char const *pSrc )
{
	// first, count how much space we are going to need
	int nSpaceNeeded = 0;
	for( char const *pScan = pSrc; *pScan; pScan++ )
	{
		nSpaceNeeded++;
		for(char const *pCharSet=s_BackSlashMap; *pCharSet; pCharSet += 2 )
		{
			if ( *pCharSet == *pScan )
				nSpaceNeeded++;								// we need to store a bakslash
		}
	}
	char *pRet = new char[ nSpaceNeeded + 1 ];				// +1 for null
	char *pOut = pRet;
	
	for( char const *pScan = pSrc; *pScan; pScan++ )
	{
		bool bIsSpecial = false;
		for(char const *pCharSet=s_BackSlashMap; *pCharSet; pCharSet += 2 )
		{
			if ( *pCharSet == *pScan )
			{
				*( pOut++ ) = '\\';
				*( pOut++ ) = pCharSet[1];
				bIsSpecial = true;
				break;
			}
		}
		if (! bIsSpecial )
		{
			*( pOut++ ) = *pScan;
		}
	}
	*( pOut++ ) = 0;
	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Helper for converting a numeric value to a hex digit, value should be 0-15.
//-----------------------------------------------------------------------------
char cIntToHexDigit( int nValue )
{
	Assert( nValue >= 0 && nValue <= 15 );
	return "0123456789ABCDEF"[ nValue & 15 ];
}

//-----------------------------------------------------------------------------
// Purpose: Helper for converting a hex char value to numeric, return -1 if the char
//          is not a valid hex digit.
//-----------------------------------------------------------------------------
int iHexCharToInt( char cValue )
{
	int32 iValue = cValue;
	if ( (uint32)( iValue - '0' ) < 10 )
		return iValue - '0';

	iValue |= 0x20;
	if ( (uint32)( iValue - 'a' ) < 6 )
		return iValue - 'a' + 10;

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Internal implementation of encode, works in the strict RFC manner, or
//          with spaces turned to + like HTML form encoding.
//-----------------------------------------------------------------------------
void Q_URLEncodeInternal( char *pchDest, int nDestLen, const char *pchSource, int nSourceLen, bool bUsePlusForSpace )
{
	if ( nDestLen < 3*nSourceLen )
	{
		pchDest[0] = '\0';
		AssertMsg( false, "Target buffer for Q_URLEncode needs to be 3 times larger than source to guarantee enough space\n" );
		return;
	}

	int iDestPos = 0;
	for ( int i=0; i < nSourceLen; ++i )
	{
		// We allow only a-z, A-Z, 0-9, period, underscore, and hyphen to pass through unescaped.
		// These are the characters allowed by both the original RFC 1738 and the latest RFC 3986.
		// Current specs also allow '~', but that is forbidden under original RFC 1738.
		if ( !( pchSource[i] >= 'a' && pchSource[i] <= 'z' ) && !( pchSource[i] >= 'A' && pchSource[i] <= 'Z' ) && !(pchSource[i] >= '0' && pchSource[i] <= '9' )
			&& pchSource[i] != '-' && pchSource[i] != '_' && pchSource[i] != '.'	
			)
		{
			if ( bUsePlusForSpace && pchSource[i] == ' ' )
			{
				pchDest[iDestPos++] = '+';
			}
			else
			{
				pchDest[iDestPos++] = '%';
				uint8 iValue = pchSource[i];
				if ( iValue == 0 )
				{
					pchDest[iDestPos++] = '0';
					pchDest[iDestPos++] = '0';
				}
				else
				{
					char cHexDigit1 = cIntToHexDigit( iValue % 16 );
					iValue /= 16;
					char cHexDigit2 = cIntToHexDigit( iValue );
					pchDest[iDestPos++] = cHexDigit2;
					pchDest[iDestPos++] = cHexDigit1;
				}
			}
		}
		else
		{
			pchDest[iDestPos++] = pchSource[i];
		}
	}

	// Null terminate
	pchDest[iDestPos++] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Internal implementation of decode, works in the strict RFC manner, or
//          with spaces turned to + like HTML form encoding.
//
//			Returns the amount of space used in the output buffer.
//-----------------------------------------------------------------------------
size_t Q_URLDecodeInternal( char *pchDecodeDest, int nDecodeDestLen, const char *pchEncodedSource, int nEncodedSourceLen, bool bUsePlusForSpace )
{
	if ( nDecodeDestLen < nEncodedSourceLen )
	{
		AssertMsg( false, "Q_URLDecode needs a dest buffer at least as large as the source" );
		return 0;
	}

	int iDestPos = 0;
	for( int i=0; i < nEncodedSourceLen; ++i )
	{
		if ( bUsePlusForSpace && pchEncodedSource[i] == '+' )
		{
			pchDecodeDest[ iDestPos++ ] = ' ';
		}
		else if ( pchEncodedSource[i] == '%' )
		{
			// Percent signifies an encoded value, look ahead for the hex code, convert to numeric, and use that

			// First make sure we have 2 more chars
			if ( i < nEncodedSourceLen - 2 )
			{
				char cHexDigit1 = pchEncodedSource[i+1];
				char cHexDigit2 = pchEncodedSource[i+2];

				// Turn the chars into a hex value, if they are not valid, then we'll
				// just place the % and the following two chars direct into the string,
				// even though this really shouldn't happen, who knows what bad clients
				// may do with encoding.
				bool bValid = false;
				int iValue = iHexCharToInt( cHexDigit1 );
				if ( iValue != -1 )
				{
					iValue *= 16;
					int iValue2 = iHexCharToInt( cHexDigit2 );
					if ( iValue2 != -1 )
					{
						iValue += iValue2;
						pchDecodeDest[ iDestPos++ ] = iValue;
						bValid = true;
					}
				}

				if ( !bValid )
				{
					pchDecodeDest[ iDestPos++ ] = '%';
					pchDecodeDest[ iDestPos++ ] = cHexDigit1;
					pchDecodeDest[ iDestPos++ ] = cHexDigit2;
				}
			}

			// Skip ahead
			i += 2;
		}
		else
		{
			pchDecodeDest[ iDestPos++ ] = pchEncodedSource[i];
		}
	}

	// We may not have extra room to NULL terminate, since this can be used on raw data, but if we do
	// go ahead and do it as this can avoid bugs.
	if ( iDestPos < nDecodeDestLen )
	{
		pchDecodeDest[iDestPos] = 0;
	}

	return (size_t)iDestPos;
}

//-----------------------------------------------------------------------------
// Purpose: Encodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version of the call isn't a strict RFC implementation, but uses + for space as is
//          the standard in HTML form encoding, despite it not being part of the RFC.
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//-----------------------------------------------------------------------------
void Q_URLEncode( char *pchDest, int nDestLen, const char *pchSource, int nSourceLen )
{
	return Q_URLEncodeInternal( pchDest, nDestLen, pchSource, nSourceLen, true );
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version of the call isn't a strict RFC implementation, but uses + for space as is
//          the standard in HTML form encoding, despite it not being part of the RFC.
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//			Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//-----------------------------------------------------------------------------
size_t Q_URLDecode( char *pchDecodeDest, int nDecodeDestLen, const char *pchEncodedSource, int nEncodedSourceLen )
{
	return Q_URLDecodeInternal( pchDecodeDest, nDecodeDestLen, pchEncodedSource, nEncodedSourceLen, true );
}


//-----------------------------------------------------------------------------
// Purpose: Encodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version will not encode space as + (which HTML form encoding uses despite not being part of the RFC)
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//-----------------------------------------------------------------------------
void Q_URLEncodeRaw( char *pchDest, int nDestLen, const char *pchSource, int nSourceLen )
{
	return Q_URLEncodeInternal( pchDest, nDestLen, pchSource, nSourceLen, false );
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version will not recognize + as a space (which HTML form encoding uses despite not being part of the RFC)
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//			Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//-----------------------------------------------------------------------------
size_t Q_URLDecodeRaw( char *pchDecodeDest, int nDecodeDestLen, const char *pchEncodedSource, int nEncodedSourceLen )
{
	return Q_URLDecodeInternal( pchDecodeDest, nDecodeDestLen, pchEncodedSource, nEncodedSourceLen, false );
}

#if defined( LINUX ) || defined( _PS3 )
extern "C" void qsort_s( void *base, size_t num, size_t width, int (*compare )(void *, const void *, const void *), void * context );
#endif

void V_qsort_s( void *base, size_t num, size_t width, int ( __cdecl *compare )(void *, const void *, const void *), void * context ) 
{
#if defined OSX
	// the arguments are swapped 'round on the mac - awesome, huh?
	return qsort_r( base, num, width, context, compare );
#else
	return qsort_s( base, num, width, compare, context );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: format the time and/or date with the user's current locale
// If timeVal is 0, gets the current time
//
// This is generally for use with chatroom dialogs, etc. which need to be
// able to say "Last message received: %date% at %time%"
//
// Note that this uses time_t because RTime32 is not hooked-up on the client
//-----------------------------------------------------------------------------
bool BGetLocalFormattedDateAndTime( time_t timeVal, char *pchDate, int cubDate, char *pchTime, int cubTime )
{
	if ( 0 == timeVal || timeVal < 0 )
	{
		// get the current time
		time( &timeVal );
	}

	if ( timeVal )
	{
		// Convert it to our local time
		struct tm tmStruct;
		struct tm tmToDisplay = *( Plat_localtime( ( const time_t* )&timeVal, &tmStruct ) );
#ifdef POSIX
		if ( pchDate != NULL )
		{
			pchDate[ 0 ] = 0;
			if ( 0 == strftime( pchDate, cubDate, "%A %b %d", &tmToDisplay ) )
				return false;
		}

		if ( pchTime != NULL )
		{
			pchTime[ 0 ] = 0;
			if ( 0 == strftime( pchTime, cubTime - 6, "%I:%M ", &tmToDisplay ) )
				return false;

			// append am/pm in lower case (since strftime doesn't have a lowercase formatting option)
			if (tmToDisplay.tm_hour >= 12)
			{
				Q_strcat( pchTime, "p.m.", cubTime );
			}
			else
			{
				Q_strcat( pchTime, "a.m.", cubTime );
			}
		}
#else // WINDOWS
		// convert time_t to a SYSTEMTIME
		SYSTEMTIME st;
		st.wHour = tmToDisplay.tm_hour;
		st.wMinute = tmToDisplay.tm_min;
		st.wSecond = tmToDisplay.tm_sec;
		st.wDay = tmToDisplay.tm_mday;
		st.wMonth = tmToDisplay.tm_mon + 1;
		st.wYear = tmToDisplay.tm_year + 1900;
		st.wDayOfWeek = tmToDisplay.tm_wday;
		st.wMilliseconds = 0;

		WCHAR rgwch[ MAX_PATH ];

		if ( pchDate != NULL )
		{
			pchDate[ 0 ] = 0;
			if ( !GetDateFormatW( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, rgwch, MAX_PATH ) )
				return false;
			Q_strncpy( pchDate, CStrAutoEncode( rgwch ).ToString(), cubDate );
		}

		if ( pchTime != NULL )
		{
			pchTime[ 0 ] = 0;
			if ( !GetTimeFormatW( LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, rgwch, MAX_PATH ) )
				return false;
			Q_strncpy( pchTime, CStrAutoEncode( rgwch ).ToString(), cubTime );
		}
#endif
		return true;
	}

	return false;
}


// And a couple of helpers so people don't have to remember the order of the parameters in the above function
bool BGetLocalFormattedDate( time_t timeVal, char *pchDate, int cubDate )
{
	return BGetLocalFormattedDateAndTime( timeVal, pchDate, cubDate, NULL, 0 );
}
bool BGetLocalFormattedTime( time_t timeVal, char *pchTime, int cubTime )
{
	return BGetLocalFormattedDateAndTime( timeVal, NULL, 0, pchTime, cubTime );
}

// Prints out a memory dump where stuff that's ascii is human readable, etc.
void V_LogMultiline( bool input, char const *label, const char *data, size_t len, CUtlString &output )
{
	static const char HEX[] = "0123456789abcdef";
	const char * direction = (input ? " << " : " >> ");
	const size_t LINE_SIZE = 24;
	char hex_line[LINE_SIZE * 9 / 4 + 2], asc_line[LINE_SIZE + 1];
	while (len > 0) 
	{
		V_memset(asc_line, ' ', sizeof(asc_line));
		V_memset(hex_line, ' ', sizeof(hex_line));
		size_t line_len = MIN(len, LINE_SIZE);
		for (size_t i=0; i<line_len; ++i) {
			unsigned char ch = static_cast<unsigned char>(data[i]);
			asc_line[i] = ( V_isprint(ch) && !V_iscntrl(ch) ) ? data[i] : '.';
			hex_line[i*2 + i/4] = HEX[ch >> 4];
			hex_line[i*2 + i/4 + 1] = HEX[ch & 0xf];
		}
		asc_line[sizeof(asc_line)-1] = 0;
		hex_line[sizeof(hex_line)-1] = 0;
		output += CFmtStr( "%s %s %s %s\n", label, direction, asc_line, hex_line );
		data += line_len;
		len -= line_len;
	}
}


#ifdef WIN32
// Win32 CRT doesn't support the full range of UChar32, has no extended planes
inline int V_iswspace( int c ) { return ( c <= 0xFFFF ) ? iswspace( (wint_t)c ) : 0; }
#else
#define V_iswspace(x) iswspace(x)
#endif


//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
const char *nexttoken(char *token, size_t nMaxTokenLen, const char *str, char sep)
{
	if (nMaxTokenLen < 1)
	{
		Assert(nMaxTokenLen > 0);
		return NULL;
	}

	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return(NULL);
	}

	char *pTokenLast = token + nMaxTokenLen - 1;

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0') && (token < pTokenLast))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return(str);
	}

	return(++str);
}

int V_StrTrim( char *pStr )
{
	char *pSource = pStr;
	char *pDest = pStr;

	// skip white space at the beginning
	while ( *pSource != 0 && V_isspace( *pSource ) )
	{
		pSource++;
	}

	// copy everything else
	char *pLastWhiteBlock = NULL;
	char *pStart = pDest;
	while ( *pSource != 0 )
	{
		*pDest = *pSource++;
		if ( V_isspace( *pDest ) )
		{
			if ( pLastWhiteBlock == NULL )
				pLastWhiteBlock = pDest;
		}
		else
		{
			pLastWhiteBlock = NULL;
		}
		pDest++;
	}
	*pDest = 0;

	// did we end in a whitespace block?
	if ( pLastWhiteBlock != NULL )
	{
		// yep; shorten the string
		pDest = pLastWhiteBlock;
		*pLastWhiteBlock = 0;
	}

	return pDest - pStart;
}

#ifdef _WIN32
int64 V_strtoi64( const char *nptr, char **endptr, int base )
{
	return _strtoi64( nptr, endptr, base );
}

uint64 V_strtoui64( const char *nptr, char **endptr, int base )
{
	return _strtoui64( nptr, endptr, base );
}
#elif POSIX
int64 V_strtoi64( const char *nptr, char **endptr, int base )
{
	return strtoll( nptr, endptr, base );
}

uint64 V_strtoui64( const char *nptr, char **endptr, int base )
{
	return strtoull( nptr, endptr, base );
}
#endif


struct HtmlEntity_t
{
	unsigned short uCharCode;
	const char *pchEntity;
	int nEntityLength;
};

const static HtmlEntity_t g_BasicHTMLEntities[] = {
		{ '"', "&quot;", 6 },
		{ '\'', "&#039;", 6 },
		{ '<', "&lt;", 4 },
		{ '>', "&gt;", 4 },
		{ '&', "&amp;", 5 },
		{ 0, NULL, 0 } // sentinel for end of array
};

const static HtmlEntity_t g_WhitespaceEntities[] = {
		{ ' ', "&nbsp;", 6 },
		{ '\n', "<br>", 4 },
		{ 0, NULL, 0 } // sentinel for end of array
};


struct Tier1FullHTMLEntity_t
{
	uchar32 uCharCode;
	const char *pchEntity;
	int nEntityLength;
};


#pragma warning( push )
#pragma warning( disable : 4428 ) // universal-character-name encountered in source
const Tier1FullHTMLEntity_t g_Tier1_FullHTMLEntities[] =
{
	{ L'"', "&quot;", 6 },
	{ L'\'', "&apos;", 6 },
	{ L'&', "&amp;", 5 },
	{ L'<', "&lt;", 4 },
	{ L'>', "&gt;", 4 },
	{ L' ', "&nbsp;", 6 },
	{ L'\u2122', "&trade;", 7 },
	{ L'\u00A9', "&copy;", 6 },
	{ L'\u00AE', "&reg;", 5 },
	{ L'\u2013', "&ndash;", 7 },
	{ L'\u2014', "&mdash;", 7 },
	{ L'\u20AC', "&euro;", 6 },
	{ L'\u00A1', "&iexcl;", 7 },
	{ L'\u00A2', "&cent;", 6 },
	{ L'\u00A3', "&pound;", 7 },
	{ L'\u00A4', "&curren;", 8 },
	{ L'\u00A5', "&yen;", 5 },
	{ L'\u00A6', "&brvbar;", 8 },
	{ L'\u00A7', "&sect;", 6 },
	{ L'\u00A8', "&uml;", 5 },
	{ L'\u00AA', "&ordf;", 6 },
	{ L'\u00AB', "&laquo;", 7 },
	{ L'\u00AC', "&not;", 8 },
	{ L'\u00AD', "&shy;", 5 },
	{ L'\u00AF', "&macr;", 6 },
	{ L'\u00B0', "&deg;", 5 },
	{ L'\u00B1', "&plusmn;", 8 },
	{ L'\u00B2', "&sup2;", 6 },
	{ L'\u00B3', "&sup3;", 6 },
	{ L'\u00B4', "&acute;", 7 },
	{ L'\u00B5', "&micro;", 7 },
	{ L'\u00B6', "&para;", 6 },
	{ L'\u00B7', "&middot;", 8 },
	{ L'\u00B8', "&cedil;", 7 },
	{ L'\u00B9', "&sup1;", 6 },
	{ L'\u00BA', "&ordm;", 6 },
	{ L'\u00BB', "&raquo;", 7 },
	{ L'\u00BC', "&frac14;", 8 },
	{ L'\u00BD', "&frac12;", 8 },
	{ L'\u00BE', "&frac34;", 8 },
	{ L'\u00BF', "&iquest;", 8 },
	{ L'\u00D7', "&times;", 7 },
	{ L'\u00F7', "&divide;", 8 },
	{ L'\u00C0', "&Agrave;", 8 },
	{ L'\u00C1', "&Aacute;", 8 },
	{ L'\u00C2', "&Acirc;", 7 },
	{ L'\u00C3', "&Atilde;", 8 },
	{ L'\u00C4', "&Auml;", 6 },
	{ L'\u00C5', "&Aring;", 7 },
	{ L'\u00C6', "&AElig;", 7 },
	{ L'\u00C7', "&Ccedil;", 8 },
	{ L'\u00C8', "&Egrave;", 8 },
	{ L'\u00C9', "&Eacute;", 8 },
	{ L'\u00CA', "&Ecirc;", 7 },
	{ L'\u00CB', "&Euml;", 6 },
	{ L'\u00CC', "&Igrave;", 8 },
	{ L'\u00CD', "&Iacute;", 8 },
	{ L'\u00CE', "&Icirc;", 7 },
	{ L'\u00CF', "&Iuml;", 6 },
	{ L'\u00D0', "&ETH;", 5 },
	{ L'\u00D1', "&Ntilde;", 8 },
	{ L'\u00D2', "&Ograve;", 8 },
	{ L'\u00D3', "&Oacute;", 8 },
	{ L'\u00D4', "&Ocirc;", 7 },
	{ L'\u00D5', "&Otilde;", 8 },
	{ L'\u00D6', "&Ouml;", 6 },
	{ L'\u00D8', "&Oslash;", 8 },
	{ L'\u00D9', "&Ugrave;", 8 },
	{ L'\u00DA', "&Uacute;", 8 },
	{ L'\u00DB', "&Ucirc;", 7 },
	{ L'\u00DC', "&Uuml;", 6 },
	{ L'\u00DD', "&Yacute;", 8 },
	{ L'\u00DE', "&THORN;", 7 },
	{ L'\u00DF', "&szlig;", 7 },
	{ L'\u00E0', "&agrave;", 8 },
	{ L'\u00E1', "&aacute;", 8 },
	{ L'\u00E2', "&acirc;", 7 },
	{ L'\u00E3', "&atilde;", 8 },
	{ L'\u00E4', "&auml;", 6 },
	{ L'\u00E5', "&aring;", 7 },
	{ L'\u00E6', "&aelig;", 7 },
	{ L'\u00E7', "&ccedil;", 8 },
	{ L'\u00E8', "&egrave;", 8 },
	{ L'\u00E9', "&eacute;", 8 },
	{ L'\u00EA', "&ecirc;", 7 },
	{ L'\u00EB', "&euml;", 6 },
	{ L'\u00EC', "&igrave;", 8 },
	{ L'\u00ED', "&iacute;", 8 },
	{ L'\u00EE', "&icirc;", 7 },
	{ L'\u00EF', "&iuml;", 6 },
	{ L'\u00F0', "&eth;", 5 },
	{ L'\u00F1', "&ntilde;", 8 },
	{ L'\u00F2', "&ograve;", 8 },
	{ L'\u00F3', "&oacute;", 8 },
	{ L'\u00F4', "&ocirc;", 7 },
	{ L'\u00F5', "&otilde;", 8 },
	{ L'\u00F6', "&ouml;", 6 },
	{ L'\u00F8', "&oslash;", 8 },
	{ L'\u00F9', "&ugrave;", 8 },
	{ L'\u00FA', "&uacute;", 8 },
	{ L'\u00FB', "&ucirc;", 7 },
	{ L'\u00FC', "&uuml;", 6 },
	{ L'\u00FD', "&yacute;", 8 },
	{ L'\u00FE', "&thorn;", 7 },
	{ L'\u00FF', "&yuml;", 6 },
	{ 0, NULL, 0 } // sentinel for end of array
};
#pragma warning( pop )



bool V_BasicHtmlEntityEncode( char *pDest, const int nDestSize, char const *pIn, const int nInSize, bool bPreserveWhitespace /*= false*/ )
{
	Assert( nDestSize == 0 || pDest != NULL );
	int iOutput = 0;
	for ( int iInput = 0; iInput < nInSize; ++iInput )
	{
		bool bReplacementDone = false;
		// See if the current char matches any of the basic entities
		for ( int i = 0; g_BasicHTMLEntities[ i ].uCharCode != 0; ++i )
		{
			if ( pIn[ iInput ] == g_BasicHTMLEntities[ i ].uCharCode )
			{
				bReplacementDone = true;
				for ( int j = 0; j < g_BasicHTMLEntities[ i ].nEntityLength; ++j )
				{
					if ( iOutput >= nDestSize - 1 )
					{
						pDest[ nDestSize - 1 ] = 0;
						return false;
					}
					pDest[ iOutput++ ] = g_BasicHTMLEntities[ i ].pchEntity[ j ];
				}
			}
		}

		if ( bPreserveWhitespace && !bReplacementDone )
		{
			// See if the current char matches any of the basic entities
			for ( int i = 0; g_WhitespaceEntities[ i ].uCharCode != 0; ++i )
			{
				if ( pIn[ iInput ] == g_WhitespaceEntities[ i ].uCharCode )
				{
					bReplacementDone = true;
					for ( int j = 0; j < g_WhitespaceEntities[ i ].nEntityLength; ++j )
					{
						if ( iOutput >= nDestSize - 1 )
						{
							pDest[ nDestSize - 1 ] = 0;
							return false;
						}
						pDest[ iOutput++ ] = g_WhitespaceEntities[ i ].pchEntity[ j ];
					}
				}
			}
		}

		if ( !bReplacementDone )
		{
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}

	// Null terminate the output
	pDest[ iOutput ] = 0;
	return true;
}


bool V_HtmlEntityDecodeToUTF8( char *pDest, const int nDestSize, char const *pIn, const int nInSize )
{
	Assert( nDestSize == 0 || pDest != NULL );
	int iOutput = 0;
	for ( int iInput = 0; iInput < nInSize && iOutput < nDestSize; ++iInput )
	{
		bool bReplacementDone = false;
		if ( pIn[ iInput ] == '&' )
		{
			bReplacementDone = true;

			uchar32 wrgchReplacement[ 2 ] = { 0, 0 };
			char rgchReplacement[ 8 ];
			rgchReplacement[ 0 ] = 0;

			const char *pchEnd = Q_strstr( pIn + iInput + 1, ";" );
			if ( pchEnd )
			{
				if ( iInput + 1 < nInSize && pIn[ iInput + 1 ] == '#' )
				{
					// Numeric
					int iBase = 10;
					int iOffset = 2;
					if ( iInput + 3 < nInSize && pIn[ iInput + 2 ] == 'x' )
					{
						iBase = 16;
						iOffset = 3;
					}

					wrgchReplacement[ 0 ] = (uchar32)V_strtoi64( pIn + iInput + iOffset, NULL, iBase );
					if ( !Q_UTF32ToUTF8( wrgchReplacement, rgchReplacement, sizeof( rgchReplacement ) ) )
					{
						rgchReplacement[ 0 ] = 0;
					}
				}
				else
				{
					// Lookup in map
					const Tier1FullHTMLEntity_t *pFullEntities = g_Tier1_FullHTMLEntities;
					for ( int i = 0; pFullEntities[ i ].uCharCode != 0; ++i )
					{
						if ( nInSize - iInput - 1 >= pFullEntities[ i ].nEntityLength )
						{
							if ( Q_memcmp( pIn + iInput, pFullEntities[ i ].pchEntity, pFullEntities[ i ].nEntityLength ) == 0 )
							{
								wrgchReplacement[ 0 ] = pFullEntities[ i ].uCharCode;
								if ( !Q_UTF32ToUTF8( wrgchReplacement, rgchReplacement, sizeof( rgchReplacement ) ) )
								{
									rgchReplacement[ 0 ] = 0;
								}
								break;
							}
						}
					}
				}

				// make sure we found a replacement. If not, skip
				int cchReplacement = V_strlen( rgchReplacement );
				if ( cchReplacement > 0 )
				{
					if ( (int)cchReplacement + iOutput < nDestSize )
					{
						for ( int i = 0; rgchReplacement[ i ] != 0; ++i )
						{
							pDest[ iOutput++ ] = rgchReplacement[ i ];
						}
					}

					// Skip extra space that we passed
					iInput += pchEnd - ( pIn + iInput );
				}
				else
				{
					bReplacementDone = false;
				}
			}
		}

		if ( !bReplacementDone )
		{
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}

	// Null terminate the output
	if ( iOutput < nDestSize )
	{
		pDest[ iOutput ] = 0;
	}
	else
	{
		pDest[ nDestSize - 1 ] = 0;
	}

	return true;
}

static const char *g_pszSimpleBBCodeReplacements[] = {
	"[b]", "<b>",
	"[/b]", "</b>",
	"[i]", "<i>",
	"[/i]", "</i>",
	"[u]", "<u>",
	"[/u]", "</u>",
	"[s]", "<s>",
	"[/s]", "</s>",
	"[code]", "<pre>",
	"[/code]", "</pre>",
	"[h1]", "<h1>",
	"[/h1]", "</h1>",
	"[list]", "<ul>",
	"[/list]", "</ul>",
	"[*]", "<li>",
	"[/url]", "</a>",
	"[img]", "<img src=\"",
	"[/img]", "\"></img>",
};

// Converts BBCode tags to HTML tags
bool V_BBCodeToHTML( OUT_Z_CAP( nDestSize ) char *pDest, const int nDestSize, char const *pIn, const int nInSize )
{
	Assert( nDestSize == 0 || pDest != NULL );
	int iOutput = 0;

	for ( int iInput = 0; iInput < nInSize && iOutput < nDestSize && pIn[ iInput ]; ++iInput )
	{
		if ( pIn[ iInput ] == '[' )
		{
			// check simple replacements
			bool bFoundReplacement = false;
			for ( int r = 0; r < ARRAYSIZE( g_pszSimpleBBCodeReplacements ); r += 2 )
			{
				int nBBCodeLength = V_strlen( g_pszSimpleBBCodeReplacements[ r ] );
				if ( !V_strnicmp( &pIn[ iInput ], g_pszSimpleBBCodeReplacements[ r ], nBBCodeLength ) )
				{
					int nHTMLReplacementLength = V_strlen( g_pszSimpleBBCodeReplacements[ r + 1 ] );
					for ( int c = 0; c < nHTMLReplacementLength && iOutput < nDestSize; c++ )
					{
						pDest[ iOutput ] = g_pszSimpleBBCodeReplacements[ r + 1 ][ c ];
						iOutput++;
					}
					iInput += nBBCodeLength - 1;
					bFoundReplacement = true;
					break;
				}
			}
			// check URL replacement
			if ( !bFoundReplacement && !V_strnicmp( &pIn[ iInput ], "[url=", 5 ) && nDestSize - iOutput > 9 )
			{
				iInput += 5;
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'a';
				pDest[ iOutput++ ] = ' ';
				pDest[ iOutput++ ] = 'h';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = 'e';
				pDest[ iOutput++ ] = 'f';
				pDest[ iOutput++ ] = '=';
				pDest[ iOutput++ ] = '\"';

				// copy all characters up to the closing square bracket
				while ( pIn[ iInput ] != ']' && iInput < nInSize && iOutput < nDestSize )
				{
					pDest[ iOutput++ ] = pIn[ iInput++ ];
				}
				if ( pIn[ iInput ] == ']' && nDestSize - iOutput > 2 )
				{
					pDest[ iOutput++ ] = '\"';
					pDest[ iOutput++ ] = '>';
				}
				bFoundReplacement = true;
			}
			// otherwise, skip over everything up to the closing square bracket
			if ( !bFoundReplacement )
			{
				while ( pIn[ iInput ] != ']' && iInput < nInSize )
				{
					iInput++;
				}
			}
		}
		else if ( pIn[ iInput ] == '\r' && pIn[ iInput + 1 ] == '\n' )
		{
			// convert carriage return and newline to a <br>
			if ( nDestSize - iOutput > 4 )
			{
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'b';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = '>';
			}
			iInput++;
		}
		else if ( pIn[ iInput ] == '\n' )
		{
			// convert newline to a <br>
			if ( nDestSize - iOutput > 4 )
			{
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'b';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = '>';
			}
		}
		else
		{
			// copy character to destination
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}
	// always terminate string
	if ( iOutput >= nDestSize )
	{
		iOutput = nDestSize - 1;
	}
	pDest[ iOutput ] = 0;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool V_IsMeanUnderscoreW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x005f':	  // low line (normal underscore)
	case L'\xff3f':	  // fullwidth low line
	case L'\x0332':	  // combining low line
		bIsMean = true;
		break;
	default:
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool V_IsMeanSpaceW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x0080':	  // PADDING CHARACTER
	case L'\x0081':	  // HIGH OCTET PRESET
	case L'\x0082':	  // BREAK PERMITTED HERE
	case L'\x0083':	  // NO BREAK PERMITTED HERE
	case L'\x0084':	  // INDEX
	case L'\x0085':	  // NEXT LINE
	case L'\x0086':	  // START OF SELECTED AREA
	case L'\x0087':	  // END OF SELECTED AREA
	case L'\x0088':	  // CHARACTER TABULATION SET
	case L'\x0089':	  // CHARACTER TABULATION WITH JUSTIFICATION
	case L'\x008A':	  // LINE TABULATION SET
	case L'\x008B':	  // PARTIAL LINE FORWARD
	case L'\x008C':	  // PARTIAL LINE BACKWARD
	case L'\x008D':	  // REVERSE LINE FEED
	case L'\x008E':	  // SINGLE SHIFT 2
	case L'\x008F':	  // SINGLE SHIFT 3
	case L'\x0090':	  // DEVICE CONTROL STRING
	case L'\x0091':	  // PRIVATE USE
	case L'\x0092':	  // PRIVATE USE
	case L'\x0093':	  // SET TRANSMIT STATE
	case L'\x0094':	  // CANCEL CHARACTER
	case L'\x0095':	  // MESSAGE WAITING
	case L'\x0096':	  // START OF PROTECTED AREA
	case L'\x0097':	  // END OF PROTECED AREA
	case L'\x0098':	  // START OF STRING
	case L'\x0099':	  // SINGLE GRAPHIC CHARACTER INTRODUCER
	case L'\x009A':	  // SINGLE CHARACTER INTRODUCER
	case L'\x009B':	  // CONTROL SEQUENCE INTRODUCER
	case L'\x009C':	  // STRING TERMINATOR
	case L'\x009D':	  // OPERATING SYSTEM COMMAND
	case L'\x009E':	  // PRIVACY MESSAGE
	case L'\x009F':	  // APPLICATION PROGRAM COMMAND
	case L'\x00A0':	  // NO-BREAK SPACE
	case L'\x034F':   // COMBINING GRAPHEME JOINER
	case L'\x2000':   // EN QUAD
	case L'\x2001':   // EM QUAD
	case L'\x2002':   // EN SPACE
	case L'\x2003':   // EM SPACE
	case L'\x2004':   // THICK SPACE
	case L'\x2005':   // MID SPACE
	case L'\x2006':   // SIX SPACE
	case L'\x2007':   // figure space
	case L'\x2008':   // PUNCTUATION SPACE
	case L'\x2009':   // THIN SPACE
	case L'\x200A':   // HAIR SPACE
	case L'\x200B':   // ZERO-WIDTH SPACE
	case L'\x200C':   // ZERO-WIDTH NON-JOINER
	case L'\x200D':   // ZERO WIDTH JOINER
	case L'\x2028':   // LINE SEPARATOR
	case L'\x2029':   // PARAGRAPH SEPARATOR
	case L'\x202F':   // NARROW NO-BREAK SPACE
	case L'\x2060':   // word joiner
	case L'\xFEFF':   // ZERO-WIDTH NO BREAK SPACE
	case L'\xFFFC':   // OBJECT REPLACEMENT CHARACTER
		bIsMean = true;
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: tell us if a Unicode character is deprecated
//
// See Unicode Technical Report #20: http://www.unicode.org/reports/tr20/
//
// Some characters are difficult or unreliably rendered. These characters eventually
// fell out of the Unicode standard, but are abusable by users. For example,
// setting "RIGHT-TO-LEFT OVERRIDE" without popping or undoing the action causes
// the layout instruction to bleed into following characters in HTML renderings,
// or upset layout calculations in vgui panels.
//
// Many games don't cope with these characters well, and end up providing opportunities
// for griefing others. For example, a user might join a game with a malformed player
// name and it turns out that player name can't be selected or typed into the admin
// console or UI to mute, kick, or ban the disruptive player. 
//
// Ideally, we'd perfectly support these end-to-end but we never realistically will.
// The benefit of doing so far outweighs the cost, anyway.
//-----------------------------------------------------------------------------
bool V_IsDeprecatedW( wchar_t wch )
{
	bool bIsDeprecated = false;

	switch ( wch )
	{
	case L'\x202A':		// LEFT-TO-RIGHT EMBEDDING
	case L'\x202B':		// RIGHT-TO-LEFT EMBEDDING
	case L'\x202C':		// POP DIRECTIONAL FORMATTING
	case L'\x202D':		// LEFT-TO-RIGHT OVERRIDE
	case L'\x202E':		// RIGHT-TO-LEFT OVERRIDE

	case L'\x206A':		// INHIBIT SYMMETRIC SWAPPING
	case L'\x206B':		// ACTIVATE SYMMETRIC SWAPPING
	case L'\x206C':		// INHIBIT ARABIC FORM SHAPING
	case L'\x206D':		// ACTIVATE ARABIC FORM SHAPING
	case L'\x206E':		// NATIONAL DIGIT SHAPES
	case L'\x206F':		// NOMINAL DIGIT SHAPES
		bIsDeprecated = true;
	}

	return bIsDeprecated;
}


//-----------------------------------------------------------------------------
// returns true if the character is allowed in a DNS doman name, false otherwise
//-----------------------------------------------------------------------------
bool V_IsValidDomainNameCharacter( const char *pch, int *pAdvanceBytes )
{
	if ( pAdvanceBytes )
		*pAdvanceBytes = 0;


	// We allow unicode in Domain Names without the an encoding unless it corresponds to 
	// a whitespace or control sequence or something we think is an underscore looking thing.
	// If this character is the start of a UTF-8 sequence, try decoding it.
	unsigned char ch = (unsigned char)*pch;
	if ( ( ch & 0xC0 ) == 0xC0 )
	{
		uchar32 rgch32Buf;
		bool bError = false;
		int iAdvance = Q_UTF8ToUChar32( pch, rgch32Buf, bError );
		if ( bError || iAdvance == 0 )
		{
			// Invalid UTF8 sequence, lets consider that invalid
			return false;
		}

		if ( pAdvanceBytes )
			*pAdvanceBytes = iAdvance;

		if ( iAdvance )
		{
			// Ick. Want uchar32 versions of unicode character classification functions.
			// Really would like Q_IsWhitespace32 and Q_IsNonPrintable32, but this is OK.
			if ( rgch32Buf < 0x10000 && ( V_IsMeanSpaceW( (wchar_t)rgch32Buf ) || V_IsDeprecatedW( (wchar_t)rgch32Buf ) || V_IsMeanUnderscoreW( (wchar_t)rgch32Buf ) ) )
			{
				return false;
			}

			return true;
		}
		else
		{
			// Unreachable but would be invalid utf8
			return false;
		}
	}
	else
	{
		// Was not unicode
		if ( pAdvanceBytes )
			*pAdvanceBytes = 1;

		// The only allowable non-unicode chars are a-z A-Z 0-9 and -
		if ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' )
			return true;

		return false;
	}
}


//-----------------------------------------------------------------------------
// returns true if the character is allowed in a URL, false otherwise
//-----------------------------------------------------------------------------
bool V_IsValidURLCharacter( const char *pch, int *pAdvanceBytes )
{
	if ( pAdvanceBytes )
		*pAdvanceBytes = 0;


	// We allow unicode in URLs unless it corresponds to a whitespace or control sequence.
	// If this character is the start of a UTF-8 sequence, try decoding it.
	unsigned char ch = (unsigned char)*pch;
	if ( ( ch & 0xC0 ) == 0xC0 )
	{
		uchar32 rgch32Buf;
		bool bError = false;
		int iAdvance = Q_UTF8ToUChar32( pch, rgch32Buf, bError );
		if ( bError || iAdvance == 0 )
		{
			// Invalid UTF8 sequence, lets consider that invalid
			return false;
		}

		if ( pAdvanceBytes )
			*pAdvanceBytes = iAdvance;

		if ( iAdvance )
		{
			// Ick. Want uchar32 versions of unicode character classification functions.
			// Really would like Q_IsWhitespace32 and Q_IsNonPrintable32, but this is OK.
			if ( rgch32Buf < 0x10000 && ( V_IsMeanSpaceW( (wchar_t)rgch32Buf ) || V_IsDeprecatedW( (wchar_t)rgch32Buf ) ) )
			{
				return false;
			}

			return true;
		}
		else
		{
			// Unreachable but would be invalid utf8
			return false;
		}
	}
	else
	{
		// Was not unicode
		if ( pAdvanceBytes )
			*pAdvanceBytes = 1;

		// Spaces, control characters, quotes, and angle brackets are not legal URL characters.
		if ( ch <= 32 || ch == 127 || ch == '"' || ch == '<' || ch == '>' )
			return false;

		return true;
	}

}


//-----------------------------------------------------------------------------
// Purpose: helper function to get a domain from a url
//			Checks both standard url and steam://openurl/<url>
//-----------------------------------------------------------------------------
bool V_ExtractDomainFromURL( const char *pchURL, char *pchDomain, int cchDomain )
{
	pchDomain[ 0 ] = 0;

	static const char *k_pchSteamOpenUrl = "steam://openurl/";
	static const char *k_pchSteamOpenUrlExt = "steam://openurl_external/";

	const char *pchOpenUrlSuffix = StringAfterPrefix( pchURL, k_pchSteamOpenUrl );
	if ( pchOpenUrlSuffix == NULL )
		pchOpenUrlSuffix = StringAfterPrefix( pchURL, k_pchSteamOpenUrlExt );

	if ( pchOpenUrlSuffix )
		pchURL = pchOpenUrlSuffix;

	if ( !pchURL || pchURL[ 0 ] == '\0' )
		return false;

	const char *pchDoubleSlash = strstr( pchURL, "//" );

	// Put the domain and everything after into pchDomain.
	// We'll find where to terminate it later.
	if ( pchDoubleSlash )
	{
		// Skip the slashes
		pchDoubleSlash += 2;

		// If that's all there was, then there's no domain here. Bail.
		if ( *pchDoubleSlash == '\0' )
		{
			return false;
		}

		// Skip any extra slashes
		// ex: http:///steamcommunity.com/
		while ( *pchDoubleSlash == '/' )
		{
			pchDoubleSlash++;
		}

		Q_strncpy( pchDomain, pchDoubleSlash, cchDomain );
	}
	else
	{
		// No double slash, so pchURL has no protocol.
		Q_strncpy( pchDomain, pchURL, cchDomain );
	}

	// First character has to be valid
	if ( *pchDomain == '?' || *pchDomain == '\0' )
	{
		return false;
	}

	// terminate the domain after the first non domain char
	int iAdvance = 0;
	int iStrLen = 0;
	char cLast = 0;
	while ( pchDomain[ iStrLen ] )
	{
		if ( !V_IsValidDomainNameCharacter( pchDomain + iStrLen, &iAdvance ) || ( pchDomain[ iStrLen ] == '.' && cLast == '.' ) )
		{
			pchDomain[ iStrLen ] = 0;
			break;
		}

		cLast = pchDomain[ iStrLen ];
		iStrLen += iAdvance;
	}

	return ( pchDomain[ 0 ] != 0 );
}


//-----------------------------------------------------------------------------
// Purpose: helper function to get a domain from a url
//-----------------------------------------------------------------------------
bool V_URLContainsDomain( const char *pchURL, const char *pchDomain )
{
	char rgchExtractedDomain[ 2048 ];
	if ( V_ExtractDomainFromURL( pchURL, rgchExtractedDomain, sizeof( rgchExtractedDomain ) ) )
	{
		// see if the last part of the domain matches what we extracted
		int cchExtractedDomain = V_strlen( rgchExtractedDomain );
		if ( pchDomain[ 0 ] == '.' )
		{
			++pchDomain;		// If the domain has a leading '.', skip it. The test below assumes there is none.
		}
		int cchDomain = V_strlen( pchDomain );

		if ( cchDomain > cchExtractedDomain )
		{
			return false;
		}
		else if ( cchExtractedDomain >= cchDomain )
		{
			// If the actual domain is longer than what we're searching for, the character previous
			// to the domain we're searching for must be a period
			if ( cchExtractedDomain > cchDomain && rgchExtractedDomain[ cchExtractedDomain - cchDomain - 1 ] != '.' )
				return false;

			if ( 0 == V_stricmp( rgchExtractedDomain + cchExtractedDomain - cchDomain, pchDomain ) )
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Strips all HTML tags not specified in rgszPreserveTags
//			Does some additional formatting, like turning <li> into * when not preserving that tag,
//          and auto-closing unclosed tags if they aren't specified in rgszNoCloseTags
//-----------------------------------------------------------------------------
void V_StripAndPreserveHTMLCore( CUtlBuffer *pbuffer, const char *pchHTML, const char **rgszPreserveTags, uint cPreserveTags, const char **rgszNoCloseTags, uint cNoCloseTags, uint cMaxResultSize )
{
	uint cHTMLCur = 0;

	bool bStripNewLines = true;
	if ( cPreserveTags > 0 )
	{
		for ( uint i = 0; i < cPreserveTags; ++i )
		{
			if ( !Q_stricmp( rgszPreserveTags[ i ], "\n" ) )
				bStripNewLines = false;
		}
	}

	//state-
	bool bInStrippedTag = false;
	bool bInStrippedContentTag = false;
	bool bInPreservedTag = false;
	bool bInListItemTag = false;
	bool bLastCharWasWhitespace = true; //set to true to strip leading whitespace
	bool bInComment = false;
	bool bInDoubleQuote = false;
	bool bInSingleQuote = false;
	int nPreTagDepth = 0;
	CUtlVector< const char* > vecTagStack;

	for ( int iContents = 0; pchHTML[ iContents ] != '\0' && cHTMLCur < cMaxResultSize; iContents++ )
	{
		char c = pchHTML[ iContents ];

		// If we are entering a comment, flag as such and skip past the begin comment tag
		const char *pchCur = &pchHTML[ iContents ];
		if ( !Q_strnicmp( pchCur, "<!--", 4 ) )
		{
			bInComment = true;
			iContents += 3;
			continue;
		}

		// If we are in a comment, check if we are exiting
		if ( bInComment )
		{
			if ( !Q_strnicmp( pchCur, "-->", 3 ) )
			{
				bInComment = false;
				iContents += 2;
				continue;
			}
			else
			{
				continue;
			}
		}

		if ( bInStrippedTag || bInPreservedTag )
		{
			// we're inside a tag, keep stripping/preserving until we get to a >
			if ( bInPreservedTag )
				pbuffer->PutChar( c );

			// While inside a tag, ignore ending > properties if they are inside a property value in "" or ''
			if ( c == '"' )
			{
				if ( bInDoubleQuote )
					bInDoubleQuote = false;
				else
					bInDoubleQuote = true;
			}

			if ( c == '\'' )
			{
				if ( bInSingleQuote )
					bInSingleQuote = false;
				else
					bInSingleQuote = true;
			}

			if ( !bInDoubleQuote && !bInSingleQuote && c == '>' )
			{
				if ( bInPreservedTag )
					bLastCharWasWhitespace = false;

				bInPreservedTag = false;
				bInStrippedTag = false;
			}
		}
		else if ( bInStrippedContentTag )
		{
			if ( c == '<' && !Q_strnicmp( pchCur, "</script>", 9 ) )
			{
				bInStrippedContentTag = false;
				iContents += 8;
				continue;
			}
			else
			{
				continue;
			}
		}
		else if ( c & 0x80 && !bInStrippedContentTag )
		{
			// start/continuation of a multibyte sequence, copy to output.
			int nMultibyteRemaining = 0;
			if ( ( c & 0xF8 ) == 0xF0 )	// first 5 bits are 11110
				nMultibyteRemaining = 3;
			else if ( ( c & 0xF0 ) == 0xE0 ) // first 4 bits are 1110
				nMultibyteRemaining = 2;
			else if ( ( c & 0xE0 ) == 0xC0 ) // first 3 bits are 110
				nMultibyteRemaining = 1;

			// cHTMLCur is in characters, so just +1
			cHTMLCur++;
			pbuffer->Put( pchCur, 1 + nMultibyteRemaining );

			iContents += nMultibyteRemaining;

			// Need to determine if we just added whitespace or not
			wchar_t rgwch[ 3 ] = { 0 };
			Q_UTF8CharsToWString( pchCur, 1, rgwch, sizeof( rgwch ) );
			if ( !V_iswspace( rgwch[ 0 ] ) )
				bLastCharWasWhitespace = false;
			else
				bLastCharWasWhitespace = true;
		}
		else
		{
			//not in a multibyte sequence- do our parsing/stripping
			if ( c == '<' )
			{
				if ( !rgszPreserveTags || cPreserveTags == 0 )
				{
					//not preserving any tags, just strip it
					bInStrippedTag = true;
				}
				else
				{
					//look ahead, is this our kind of tag?
					bool bPreserve = false;
					bool bEndTag = false;
					const char *szTagStart = &pchHTML[ iContents + 1 ];
					// if it's a close tag, skip the /
					if ( *szTagStart == '/' )
					{
						bEndTag = true;
						szTagStart++;
					}
					if ( Q_strnicmp( "script", szTagStart, 6 ) == 0 )
					{
						bInStrippedTag = true;
						bInStrippedContentTag = true;
					}
					else
					{
						//see if this tag is one we want to preserve
						for ( uint iTag = 0; iTag < cPreserveTags; iTag++ )
						{
							const char *szTag = rgszPreserveTags[ iTag ];
							int cchTag = Q_strlen( szTag );

							//make sure characters match, and are followed by some non-alnum char 
							//  so "i" can match <i> or <i class=...>, but not <img>
							if ( Q_strnicmp( szTag, szTagStart, cchTag ) == 0 && !V_isalnum( szTagStart[ cchTag ] ) )
							{
								bPreserve = true;
								if ( bEndTag )
								{
									// ending a paragraph tag is optional. If we were expecting to find one, and didn't, skip
									if ( Q_stricmp( szTag, "p" ) != 0 )
									{
										while ( vecTagStack.Count() > 0 && Q_stricmp( vecTagStack[ vecTagStack.Count() - 1 ], "p" ) == 0 )
										{
											vecTagStack.Remove( vecTagStack.Count() - 1 );
										}
									}

									if ( vecTagStack.Count() > 0 && vecTagStack[ vecTagStack.Count() - 1 ] == szTag )
									{
										vecTagStack.Remove( vecTagStack.Count() - 1 );

										if ( Q_stricmp( szTag, "pre" ) == 0 )
										{
											nPreTagDepth--;
											if ( nPreTagDepth < 0 )
											{
												nPreTagDepth = 0;
											}
										}
									}
									else
									{
										// don't preserve this unbalanced tag.  All open tags will be closed at the end of the blurb
										bPreserve = false;
									}
								}
								else
								{
									bool bNoCloseTag = false;
									for ( uint iNoClose = 0; iNoClose < cNoCloseTags; iNoClose++ )
									{
										if ( Q_stricmp( szTag, rgszNoCloseTags[ iNoClose ] ) == 0 )
										{
											bNoCloseTag = true;
											break;
										}
									}

									if ( !bNoCloseTag )
									{
										vecTagStack.AddToTail( szTag );
										if ( Q_stricmp( szTag, "pre" ) == 0 )
										{
											nPreTagDepth++;
										}
									}
								}
								break;
							}
						}
						if ( !bPreserve )
						{
							bInStrippedTag = true;
						}
						else
						{
							bInPreservedTag = true;
							pbuffer->PutChar( c );
						}

					}
				}
				if ( bInStrippedTag )
				{
					const char *szTagStart = &pchHTML[ iContents ];
					if ( Q_strnicmp( szTagStart, "<li>", Q_strlen( "<li>" ) ) == 0 )
					{
						if ( bInListItemTag )
						{
							pbuffer->PutChar( ';' );
							cHTMLCur++;
							bInListItemTag = false;
						}

						if ( !bLastCharWasWhitespace )
						{
							pbuffer->PutChar( ' ' );
							cHTMLCur++;
						}

						pbuffer->PutChar( '*' );
						pbuffer->PutChar( ' ' );
						cHTMLCur += 2;
						bInListItemTag = true;
					}
					else if ( !bLastCharWasWhitespace )
					{

						if ( bInListItemTag )
						{
							char cLastChar = ' ';

							if ( pbuffer->TellPut() > 0 )
							{
								cLastChar = ( ( (char*)pbuffer->Base() ) + pbuffer->TellPut() - 1 )[ 0 ];
							}
							if ( cLastChar != '.' && cLastChar != '?' && cLastChar != '!' )
							{
								pbuffer->PutChar( ';' );
								cHTMLCur++;
							}
							bInListItemTag = false;
						}

						//we're decided to remove a tag, simulate a space in the original text
						pbuffer->PutChar( ' ' );
						cHTMLCur++;
					}
					bLastCharWasWhitespace = true;
				}
			}
			else
			{
				//just a normal character, nothin' special.
				if ( nPreTagDepth == 0 && V_isspace( c ) && ( bStripNewLines || c != '\n' ) )
				{
					if ( !bLastCharWasWhitespace )
					{
						//replace any block of whitespace with a single space
						cHTMLCur++;
						pbuffer->PutChar( ' ' );
						bLastCharWasWhitespace = true;
					}
					// don't put anything for whitespace if the previous character was whitespace 
					//  (effectively trimming all blocks of whitespace down to a single ' ')
				}
				else
				{
					cHTMLCur++;
					pbuffer->PutChar( c );
					bLastCharWasWhitespace = false;
				}
			}
		}
	}
	if ( cHTMLCur >= cMaxResultSize )
	{
		// we terminated because the blurb was full.  Add a '...' to the end
		pbuffer->Put( "...", 3 );
	}
	//close any preserved tags that were open at the end.
	FOR_EACH_VEC_BACK( vecTagStack, iTagStack )
	{
		pbuffer->PutChar( '<' );
		pbuffer->PutChar( '/' );
		pbuffer->Put( vecTagStack[ iTagStack ], Q_strlen( vecTagStack[ iTagStack ] ) );
		pbuffer->PutChar( '>' );
	}

	// Null terminate
	pbuffer->PutChar( '\0' );
}

//-----------------------------------------------------------------------------
// Purpose: Strips all HTML tags not specified in rgszPreserveTags
//			Does some additional formatting, like turning <li> into * when not preserving that tag
//-----------------------------------------------------------------------------
void V_StripAndPreserveHTML( CUtlBuffer *pbuffer, const char *pchHTML, const char **rgszPreserveTags, uint cPreserveTags, uint cMaxResultSize )
{
	const char *rgszNoCloseTags[] = { "br", "img" };
	V_StripAndPreserveHTMLCore( pbuffer, pchHTML, rgszPreserveTags, cPreserveTags, rgszNoCloseTags, V_ARRAYSIZE( rgszNoCloseTags ), cMaxResultSize );
}
