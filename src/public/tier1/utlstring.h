//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef UTLSTRING_H
#define UTLSTRING_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlmemory.h"
#include "tier1/strtools.h"
#include "limits.h"

#if defined( OSX )
inline wchar_t *wcsdup(const wchar_t *pString)
{
	wchar_t *pMemory;

	if (!pString)
		return NULL;

	size_t len = (wcslen(pString) + 1);
	if ((pMemory = (wchar_t *)malloc(len * sizeof(wchar_t))) != NULL)
	{
		return wcscpy( pMemory, pString );
	}

	return NULL;
}

inline size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return (p ? p - s : n);
}

#endif

//-----------------------------------------------------------------------------
// Simple string class. 
// NOTE: This is *not* optimal! Use in tools, but not runtime code
//-----------------------------------------------------------------------------
class CUtlString
{
public:
	typedef enum
	{
		PATTERN_NONE		= 0x00000000,
		PATTERN_DIRECTORY	= 0x00000001
	} TUtlStringPattern;

public:
	CUtlString();
	CUtlString( const char *pString );
	CUtlString( const char *pString, int length );
	CUtlString( const CUtlString& string );

#ifdef MOVE_CONSTRUCTOR_SUPPORT
	// Support moving of CUtlString objects. Long live C++11
	// This move constructor will get called when appropriate, such as when
	// returning objects from functions, or otherwise copying from temporaries
	// which are about to be destroyed. It can also be explicitly invoked with
	// std::move().
	// Move constructor:
	CUtlString( CUtlString&& rhs )
	{
		// Move the string pointer from the source to this -- be sure to
		// zero out the source to avoid double frees.
		m_pString = rhs.m_pString;
		rhs.m_pString = 0;
	}
	// Move assignment operator:
	CUtlString& operator=( CUtlString&& rhs )
	{
		// Move the string pointer from the source to this -- be sure to
		// zero out the source to avoid double frees.
		m_pString = rhs.m_pString;
		rhs.m_pString = 0;
		return *this;
	}
#endif

	~CUtlString();

	const char	*Get( ) const;
	void		Set( const char *pValue );
	operator const char*() const;

	// Set directly and don't look for a null terminator in pValue.
	// nChars does not include the nul and this will only copy
	// at most nChars (even if pValue is longer).  If nChars
	// is >strlen(pValue) it will copy past the end, don't do it
	// Does nothing if pValue == String()
	void		SetDirect( const char *pValue, int nChars );

	// for compatibility switching items from UtlSymbol
	const char  *String() const { return Get(); }

	// Returns strlen
	int			Length() const;
	// IsEmpty() is more efficient than Length() == 0
	bool		IsEmpty() const;

	// Sets the length (used to serialize into the buffer )
	// Note: If nLen != 0, then this adds an extra byte for a null-terminator.	
	void		SetLength( int nLen );
	char		*GetForModify();
	void		Clear();
	void		Purge();

	// Case Change
	void		ToLower();
	void		ToUpper();
	void		Append( const char *pAddition, int nChars );

	void		Append( const char *pchAddition );
	void		Append( const char chAddition ) { char temp[2] = { chAddition, 0 }; Append( temp ); }
	// Strips the trailing slash
	void		StripTrailingSlash();
	void		FixSlashes( char cSeparator = CORRECT_PATH_SEPARATOR );

	// Trim whitespace
	void		TrimLeft( char cTarget );
	void		TrimLeft( const char *szTargets = "\t\r\n " );
	void		TrimRight( char cTarget );
	void		TrimRight( const char *szTargets = "\t\r\n " );
	void		Trim( char cTarget );
	void		Trim( const char *szTargets = "\t\r\n " );

	bool		IsEqual_CaseSensitive( const char *src ) const;
	bool		IsEqual_CaseInsensitive( const char *src ) const;

	CUtlString &operator=( const CUtlString &src );
	CUtlString &operator=( const char *src );

	// Test for equality
	bool operator==( const CUtlString &src ) const;
	bool operator!=( const CUtlString &src ) const { return !operator==( src ); }

	CUtlString &operator+=( const CUtlString &rhs );
	CUtlString &operator+=( const char *rhs );
	CUtlString &operator+=( char c );
	CUtlString &operator+=( int rhs );
	CUtlString &operator+=( double rhs );

	CUtlString operator+( const char *pOther ) const;
	CUtlString operator+( const CUtlString &other ) const;
	CUtlString operator+( int rhs ) const;

	bool MatchesPattern( const CUtlString &Pattern, int nFlags = 0 ) const;		// case SENSITIVE, use * for wildcard in pattern string

	char operator[]( int i ) const;

#if ! defined(SWIG)
	// Don't let SWIG see the PRINTF_FORMAT_STRING attribute or it will complain.
	int Format( PRINTF_FORMAT_STRING const char *pFormat, ... )  FMTFUNCTION( 2, 3 );
	int FormatV( PRINTF_FORMAT_STRING const char *pFormat, va_list marker );
#else
	int Format( const char *pFormat, ... );
	int FormatV( const char *pFormat, va_list marker );
#endif

	// Defining AltArgumentType_t hints that associative container classes should
	// also implement Find/Insert/Remove functions that take const char* params.
	typedef const char *AltArgumentType_t;

	// Get a copy of part of the string.
	// If you only specify nStart, it'll go from nStart to the end.
	// You can use negative numbers and it'll wrap around to the start.
	CUtlString Slice( int32 nStart=0, int32 nEnd=INT_MAX ) const;

	// Get a substring starting from the left or the right side.
	CUtlString Left( int32 nChars ) const;
	CUtlString Right( int32 nChars ) const;

	// Get a string with all instances of one character replaced with another.
	CUtlString Replace( char cFrom, char cTo ) const;

	// Replace all instances of specified string with another.
	CUtlString Replace( const char *pszFrom, const char *pszTo ) const;

	// Get this string as an absolute path (calls right through to V_MakeAbsolutePath).
	CUtlString AbsPath( const char *pStartingDir=NULL ) const;	

	// Gets the filename (everything except the path.. c:\a\b\c\somefile.txt -> somefile.txt).
	CUtlString UnqualifiedFilename() const;
	
	// Gets a string with one directory removed. Uses V_StripLastDir but strips the last slash also!
	CUtlString DirName() const;

	// Get a string with the extension removed (with V_StripExtension).
	CUtlString StripExtension() const;

	// Get a string with the filename removed (uses V_UnqualifiedFileName and also strips the last slash)
	CUtlString StripFilename() const;

	// Get a string with the base filename (with V_FileBase).
	CUtlString GetBaseFilename() const;

	// Get a string with the file extension (with V_FileBase).
	CUtlString GetExtension() const;

	// Works like V_ComposeFileName.
	static CUtlString PathJoin( const char *pStr1, const char *pStr2 );

	// These can be used for utlvector sorts.
	static int __cdecl SortCaseInsensitive( const CUtlString *pString1, const CUtlString *pString2 );
	static int __cdecl SortCaseSensitive( const CUtlString *pString1, const CUtlString *pString2 );

	// Empty string for those times when you need to return an empty string and
	// either don't want to pay the construction cost, or are returning a
	// const CUtlString& and cannot just return "".
	static const CUtlString &GetEmptyString();

private:
	// INTERNALS
	// AllocMemory allocates enough space for length characters plus a terminating zero.
	// Previous characters are preserved, the buffer is null-terminated, but new characters
	// are not touched.
	void *AllocMemory( uint32 length );

	// If m_pString is not NULL, it points to the start of the string, and the memory allocation.
	char *m_pString;
};

//	// If these are not defined, CUtlConstString as rhs will auto-convert
//	// to const char* and do logical operations on the raw pointers. Ugh.
//	inline friend bool operator<( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) > 0; }
//	inline friend bool operator==( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) == 0; }
//	inline friend bool operator!=( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) != 0; }

inline bool operator==( const char *pString, const CUtlString &utlString )
{
	return utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator!=( const char *pString, const CUtlString &utlString )
{	
	return !utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator==( const CUtlString &utlString, const char *pString )
{
	return utlString.IsEqual_CaseSensitive( pString );
}

inline bool operator!=( const CUtlString &utlString, const char *pString )
{
	return !utlString.IsEqual_CaseSensitive( pString );
}



//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CUtlString::CUtlString()
: m_pString( NULL )
{
}

inline CUtlString::CUtlString( const char *pString )
: m_pString( NULL )
{
	Set( pString );
}

inline CUtlString::CUtlString( const char *pString, int length )
: m_pString( NULL )
{
	SetDirect( pString, length );
}

inline CUtlString::CUtlString( const CUtlString& string )
: m_pString( NULL )
{
	Set( string.Get() );
}

inline CUtlString::~CUtlString()
{
	Purge();
}

inline int CUtlString::Length() const
{
	if (m_pString)
	{
		return V_strlen( m_pString );
	}
	return 0;
}

inline bool CUtlString::IsEmpty() const
{
	return !m_pString || m_pString[0] == 0;
}

inline int __cdecl CUtlString::SortCaseInsensitive( const CUtlString *pString1, const CUtlString *pString2 )
{
	return V_stricmp( pString1->String(), pString2->String() );
}

inline int __cdecl CUtlString::SortCaseSensitive( const CUtlString *pString1, const CUtlString *pString2 )
{
	return V_strcmp( pString1->String(), pString2->String() );
}

// Converts to c-strings
inline CUtlString::operator const char*() const
{
	return Get();
}



//-----------------------------------------------------------------------------
// Purpose: Implementation of low-level string functionality for character types.
//-----------------------------------------------------------------------------

template < typename T >
class StringFuncs
{
public:
	static T		*Duplicate( const T *pValue );
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void		 Copy( T *out_pOut, const T *pIn, int iLengthInChars );
	static int		 Compare( const T *pLhs, const T *pRhs );
	static int		 CaselessCompare( const T *pLhs, const T *pRhs );
	static int		 Length( const T *pValue );
	static const T  *FindChar( const T *pStr, const T cSearch );
	static const T	*EmptyString();
	static const T	*NullDebugString();
};

template < >
class StringFuncs<char>
{
public:
	static char		  *Duplicate( const char *pValue ) { return strdup( pValue ); }
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void		   Copy( OUT_CAP(iLengthInChars) char *out_pOut, const char *pIn, int iLengthInChars ) { strncpy( out_pOut, pIn, iLengthInChars ); }
	static int		   Compare( const char *pLhs, const char *pRhs ) { return strcmp( pLhs, pRhs ); }
	static int		   CaselessCompare( const char *pLhs, const char *pRhs ) { return Q_strcasecmp( pLhs, pRhs ); }
	static int		   Length( const char *pValue ) { return (int)strlen( pValue ); }
	static const char *FindChar( const char *pStr, const char cSearch ) { return strchr( pStr, cSearch ); }
	static const char *EmptyString() { return ""; }
	static const char *NullDebugString() { return "(null)"; }
};

template < >
class StringFuncs<wchar_t>
{
public:
	static wchar_t		 *Duplicate( const wchar_t *pValue ) { return wcsdup( pValue ); }
	// Note that this function takes a character count, and does not guarantee null-termination.
	static void			  Copy( OUT_CAP(iLengthInChars) wchar_t *out_pOut, const wchar_t  *pIn, int iLengthInChars ) { wcsncpy( out_pOut, pIn, iLengthInChars ); }
	static int			  Compare( const wchar_t *pLhs, const wchar_t *pRhs ) { return wcscmp( pLhs, pRhs ); }
	static int			  CaselessCompare( const wchar_t *pLhs, const wchar_t *pRhs ); // no implementation?
	static int			  Length( const wchar_t *pValue ) { return (int)wcslen( pValue ); }
	static const wchar_t *FindChar( const wchar_t *pStr, const wchar_t cSearch ) { return wcschr( pStr, cSearch ); }
	static const wchar_t *EmptyString() { return L""; }
	static const wchar_t *NullDebugString() { return L"(null)"; }
};

//-----------------------------------------------------------------------------
// Dirt-basic auto-release string class. Not intended for manipulation,
// can be stored in a container or forwarded as a functor parameter.
// Note the benefit over CUtlString: sizeof(CUtlConstString) == sizeof(char*).
// Also note: null char* pointers are treated identically to empty strings.
//-----------------------------------------------------------------------------

template < typename T = char >
class CUtlConstStringBase
{
public:
	CUtlConstStringBase() : m_pString( NULL ) {}
	explicit CUtlConstStringBase( const T *pString ) : m_pString( NULL ) { Set( pString ); }
	CUtlConstStringBase( const CUtlConstStringBase& src ) : m_pString( NULL ) { Set( src.m_pString ); }
	~CUtlConstStringBase() { Set( NULL ); }

	void Set( const T *pValue );
	void Clear() { Set( NULL ); }

	const T *Get() const { return m_pString ? m_pString : StringFuncs<T>::EmptyString(); }
	operator const T*() const { return m_pString ? m_pString : StringFuncs<T>::EmptyString(); }

	bool IsEmpty() const { return m_pString == NULL; } // Note: empty strings are never stored by Set

	int Compare( const T *rhs ) const;

	// Logical ops
	bool operator<( const T *rhs ) const { return Compare( rhs ) < 0; }
	bool operator==( const T *rhs ) const { return Compare( rhs ) == 0; }
	bool operator!=( const T *rhs ) const { return Compare( rhs ) != 0; }
	bool operator<( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) < 0; }
	bool operator==( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) == 0; }
	bool operator!=( const CUtlConstStringBase &rhs ) const { return Compare( rhs.m_pString ) != 0; }

	// If these are not defined, CUtlConstString as rhs will auto-convert
	// to const char* and do logical operations on the raw pointers. Ugh.
	inline friend bool operator<( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) > 0; }
	inline friend bool operator==( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) == 0; }
	inline friend bool operator!=( const T *lhs, const CUtlConstStringBase &rhs ) { return rhs.Compare( lhs ) != 0; }

	CUtlConstStringBase &operator=( const T *src ) { Set( src ); return *this; }
	CUtlConstStringBase &operator=( const CUtlConstStringBase &src ) { Set( src.m_pString ); return *this; }

	// Defining AltArgumentType_t is a hint to containers that they should
	// implement Find/Insert/Remove functions that take const char* params.
	typedef const T *AltArgumentType_t;

protected:
	const T *m_pString;
};

template < typename T >
void CUtlConstStringBase<T>::Set( const T *pValue )
{
	if ( pValue != m_pString )
	{
		free( ( void* ) m_pString );
		m_pString = pValue && pValue[0] ? StringFuncs<T>::Duplicate( pValue ) : NULL;
	}
}

template < typename T >
int CUtlConstStringBase<T>::Compare( const T *rhs ) const
{
	// Empty or null RHS?
	if ( !rhs || !rhs[0] )
		return m_pString ? 1 : 0;

	// Empty *this, non-empty RHS?
	if ( !m_pString )
		return -1;

	// Neither empty
	return StringFuncs<T>::Compare( m_pString, rhs );
}

typedef	CUtlConstStringBase<char>		CUtlConstString;
typedef	CUtlConstStringBase<wchar_t>	CUtlConstWideString;

//-----------------------------------------------------------------------------
// Helper functor objects.
//-----------------------------------------------------------------------------

template < typename T > struct UTLConstStringCaselessStringLessFunctor { bool operator()( const CUtlConstStringBase<T>& a, const char *b ) const { return StringFuncs<T>::CaselessCompare( a.Get(), b ) < 0; } };
template < typename T > struct UTLConstStringCaselessStringEqualFunctor { bool operator()( const CUtlConstStringBase<T>& a, const char *b ) const { return StringFuncs<T>::CaselessCompare( a.Get(), b ) == 0; } };

// Helper function for CUtlMaps with a CUtlString key
inline bool UtlStringLessFunc( const CUtlString &lhs, const CUtlString &rhs ) { return V_strcmp( lhs.Get(), rhs.Get() ) < 0; } 
inline bool UtlStringCaseInsensitiveLessFunc( const CUtlString &lhs, const CUtlString &rhs ) { return V_stricmp( lhs.Get(), rhs.Get() ) < 0; } 

#endif // UTLSTRING_H
