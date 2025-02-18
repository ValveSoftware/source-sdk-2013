//====== Copyright Valve Corporation, All rights reserved. =======

#ifndef UTLSTRING_H
#define UTLSTRING_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlmemory.h"
#include "tier1/strtools.h"
#include "limits.h"

// Matched with the memdbgoff at end of header
#include "memdbgon.h"

#if defined( OSX )
#ifndef wcsdup
// The mem override tools may provide a copy of this if active, otherwise it is not available in OS X's libc due to
// being introduced in POSIX-20008
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
#endif

inline size_t strnlen(const char *s, size_t n)
{
	const char *p = (const char *)memchr(s, 0, n);
	return (p ? p - s : n);
}

#endif

#define MOVE_CONSTRUCTOR_SUPPORT

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
		// zero out the source to avoid double frees.  SetPtr will free our existing
		// value if needed.
		SetPtr( rhs.m_pString );
		rhs.m_pString = 0;
		return *this;
	}

	void SetPtr( char *pszPtr )
	{
		if ( pszPtr == m_pString )
			return;

		Purge();
		m_pString = pszPtr;
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

	char *Access() { return GetForModify(); }
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

	ptrdiff_t IndexOf( const char *pstrTarget ) const;

	// Trim whitespace
	void		TrimLeft( char cTarget );
	void		TrimLeft( const char *szTargets = "\t\r\n " );
	void		TrimRight( char cTarget );
	void		TrimRight( const char *szTargets = "\t\r\n " );
	void		Trim( char cTarget );
	void		Trim( const char *szTargets = "\t\r\n " );

	bool		IsEqual_CaseSensitive( const char *src ) const;
	bool		IsEqual_CaseInsensitive( const char *src ) const;

	void RemoveDotSlashes(char separator = CORRECT_PATH_SEPARATOR);

	CUtlString AbsPath(const char *pStartingDir, bool bLowercaseName) const
	{
		CUtlString result = AbsPath(pStartingDir);
		if (bLowercaseName)
		{
			result.ToLower();
		}
		return result;
	}

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

	// is valid?
	bool IsValid() const { return m_pString != NULL; }

#if ! defined(SWIG)
	// Don't let SWIG see the PRINTF_FORMAT_STRING attribute or it will complain.
	int Format( PRINTF_FORMAT_STRING const char *pFormat, ... )  FMTFUNCTION( 2, 3 );
	int FormatV( PRINTF_FORMAT_STRING const char *pFormat, va_list marker );
#else
	int Format( const char *pFormat, ... );
	int FormatV( const char *pFormat, va_list marker );
#endif

	void Truncate( int nChars );

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
	CUtlString Replace( const char *pszFrom, const char *pszTo, bool bCaseSensitive = false ) const;

	// Get a string with the specified substring removed
	CUtlString Remove( char const *pTextToRemove, bool bCaseSensitive = false ) const;

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

//-----------------------------------------------------------------------------
// Purpose: Truncates the string to the specified number of characters
//-----------------------------------------------------------------------------
inline void CUtlString::Truncate( int nChars )
{
	if ( !m_pString )
		return;

	int nLen = V_strlen( m_pString );
	if ( nLen <= nChars )
		return;

	m_pString[nChars] = '\0';
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

#include "memdbgoff.h"



//-----------------------------------------------------------------------------
// Purpose: General purpose string class good for when it
//			is rarely expected to be empty, and/or will undergo
//			many modifications/appends.
//-----------------------------------------------------------------------------
class CUtlStringBuilder
{
public:
	CUtlStringBuilder();
	CUtlStringBuilder( const char *pchString );
	CUtlStringBuilder( CUtlStringBuilder const &string );
	explicit CUtlStringBuilder( size_t nPreallocateBytes );
	//CUtlStringBuilder( const CUtlStringResult &strMoveSource );
	//explicit CUtlStringBuilder( IFillStringFunctor& func );
	~CUtlStringBuilder();

	// operator=
	CUtlStringBuilder &operator=( const CUtlStringBuilder &src );
	CUtlStringBuilder &operator=( const char *pchString );
	//CUtlStringBuilder &operator=( const CUtlStringResult &strMoveSource );

	// operator==
	bool operator==( CUtlStringBuilder const &src ) const;
	bool operator==( const char *pchString ) const;

	// operator!=
	bool operator!=( CUtlStringBuilder const &src ) const;
	bool operator!=( const char *pchString ) const;

	// operator </>, performs case sensitive comparison
	bool operator<( const CUtlStringBuilder &val ) const;
	bool operator<( const char *pchString ) const;
	bool operator>( const CUtlStringBuilder &val ) const;
	bool operator>( const char *pchString ) const;

	// operator+=
	CUtlStringBuilder &operator+=( const char *rhs );

	// is valid?
	bool IsValid() const;

	// gets the string
	// never returns NULL, use IsValid() to see if it's never been set
	const char *String() const;
	const char *Get() const { return String(); }
	operator const char *() const { return String(); }

	// returns the string directly (could be NULL)
	// useful for doing inline operations on the string
	char *Access() 
	{
		// aggressive warning that there has been a bad error;
		// probably should not be retrieving the string by pointer.
		Assert(!m_data.HasError());

		if ( !IsValid() || ( Capacity() == 0 ) ) 
			return NULL;

		return m_data.Access(); 
	}

	// return false if capacity can't be set. If false is returned,
	// the error state is set.
	bool EnsureCapacity( size_t nLength );

	// append in-place, causing a re-allocation
	void Append( const char *pchAddition );
	void Append( const char *pchAddition, size_t cbLen );
	void Append( const CUtlStringBuilder &str ) { Append( str.String(), str.Length() ); }
	//void Append( IFillStringFunctor& func );
	void AppendChar( char ch ) { Append( &ch, 1 ); }
	void AppendRepeat( char ch, int cCount );

	// sets the string
	void SetValue( const char *pchString );
	void Set( const char *pchString );
	void Clear() { m_data.Clear(); }
	void SetPtr( char *pchString );
	void SetPtr( char *pchString, size_t nLength );
	void Swap( CUtlStringBuilder &src );

	// If you want to take ownership of the ptr, you can use this. So for instance if you had 
	// a CUtlString you wanted to move it to a CUtlStringConst without making a copy, you 
	// could do: CUtlStringConst strConst( strUtl.Detach() );
	// Also used for fast temporaries when a string that does not need to be retained is being 
	// returned from a func. ie: return (str.Detach());
	// All strings in this file can take a CUtlStringResult as a constructor parm and will take ownership directly.
	//CUtlStringResult Detach();

	// Set directly and don't look for a null terminator in pValue.
	// nChars is the string length. "abcd" nChars==3 would copy and null
	// terminate "abc" in the string object.
	void SetDirect( const char *pchString, size_t nChars );

	// Get the length of the string in characters.
	size_t Length() const;
	bool IsEmpty() const;
	size_t Capacity() const { return m_data.Capacity(); } // how much room is there to scribble

	// Format like sprintf.
	size_t Format( PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 2, 3 );
	// format, then append what we crated in the format
	size_t AppendFormat( PRINTF_FORMAT_STRING const char *pFormat, ... ) FMTFUNCTION( 2, 3 );

	// replace a single character with another, returns hit count
	size_t Replace( char chTarget, char chReplacement );

	// replace a string with another string, returns hit count
	// replacement string might be NULL or "" to remove target substring
	size_t Replace( const char *pstrTarget, const char *pstrReplacement );
	size_t ReplaceCaseless( const char *pstrTarget, const char *pstrReplacement );

	size_t ReplaceFastCaseless( const char* pstrTarget, const char* pstrReplacement );

    // replace a sequence of characters at a given point with a new string
	// replacement string might be NULL or "" to remove target substring
    bool ReplaceAt( size_t nIndex, size_t nOldChars, const char *pNewStr, size_t nNewChars = SIZE_MAX );
    
	ptrdiff_t IndexOf( const char *pstrTarget ) const;

	// remove whitespace from the string; anything that is isspace()
	size_t RemoveWhitespace( );

	// trim whitepace from the beginning and end of the string
	size_t TrimWhitespace( );

	// Allows setting the size to anything under the current
	// capacity.  Typically should not be used unless there was a specific
	// reason to scribble on the string. Will not touch the string contents,
	// but will append a NULL. Returns true if the length was changed.
	bool SetLength( size_t nLen );

	// Take responsibility for the string. May cause a heap alloc.
	char *TakeOwnership( size_t *pnLen, size_t *pnCapacity );

	// For operations that are long and/or complex - if something fails
	// along the way, the error will be set and can be queried at the end.
	// The string is undefined in the error state, but will likely hold the
	// last value before the error occurred.  The string is cleared
	// if ClearError() is called.  The error can be set be the user, and it
	// will also be set if a dynamic allocation fails in string operations
	// where it needs to grow the capacity.
	void SetError()			{ m_data.SetError(true); }
	void ClearError()		{ m_data.ClearError(); }
	bool HasError() const	{ return m_data.HasError(); }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );	// validate our internal structures
#endif // DBGFLAG_VALIDATE

	size_t VFormat( const char *pFormat, va_list args );
	size_t VAppendFormat( const char *pFormat, va_list args );
	void Truncate( size_t nChars );

	// Access() With no assertion check - should only be used for tests
	char *AccessNoAssert() 
	{
		if ( !IsValid() ) 
			return NULL;
		return m_data.Access(); 
	}

	// SetError() With no assertion check - should only be used for tests
	void SetErrorNoAssert() { m_data.SetError(false); }

private:
	size_t ReplaceInternal( const char *pstrTarget, const char *pstrReplacement, const char *pfnCompare(const char*, const char*)  );
	operator bool () const	{ return IsValid(); }

	// nChars is the number of characters you want, NOT including the null
	char *PrepareBuffer( size_t nChars, bool bCopyOld = false, size_t nMinCapacity = 0 )
	{
		char *pszString = NULL;
		size_t nCapacity = m_data.Capacity();
		if ( ( nChars <= nCapacity ) && ( nMinCapacity <= nCapacity ) )
		{
			// early out leaving it all alone, just update the length,
			// even if it shortens an existing heap string to a width
			// that would fit in the stack buffer.
			pszString = m_data.SetLength( nChars );

			// SetLength will have added the null. Pointer might
			// be NULL if there is an error state and no buffer
			Assert( !pszString ||  pszString[nChars] == '\0' );

			return pszString;
		}

		if ( HasError() )
			return NULL;

		// Need to actually adjust the capacity
		return InternalPrepareBuffer( nChars, bCopyOld, Max( nChars, nMinCapacity ) );
	}

	char *InternalPrepareBuffer( size_t nChars, bool bCopyOld, size_t nMinCapacity );

	template <typename T>
	void Swap( T &p1, T &p2 )
	{
		T t = p1;
		p1 = p2;
		p2 = t;
	}

	// correct for 32 or 64 bit
	static const uint8 MAX_STACK_STRLEN = (sizeof(void*)==4 ? 15 : 19 );
	enum 
	{  
		// Note: If it's ever desired to have the embedded string be larger than
		// 63 (0x40-1), just make the sentinal 0xFF, and the error something (0xFF also
		// is fine).  Then shrink the scrap size by 1 and add a uint8 for the error state.
		// The error byte is only valid if the heap is on (already have that restriction).
		// and then embedded strings can get back to being up to 254.  It's not done this
		// way now just to make the tests for IsHeap()/HasError() faster since they
		// are often both tested together the compiler can do nice bit test optimizations.
		STRING_TYPE_SENTINEL = 0x80,
		STRING_TYPE_ERROR = 0x40
	}; // if Data.Stack.BytesLeft() or Data.Heap.sentinel == this value, data is in heap

	union Data
	{
		struct _Heap
		{	// 16 on 32 bit, sentinel == 0xff if Heap is the active union item
		private:
			char *m_pchString;
			uint32 m_nLength;
			uint32 m_nCapacity; // without trailing null; ie: m_pchString[m_nCapacity] = '\0' is not out of bounds
			uint8  scrap[3];
			uint8  sentinel;
		public:
			friend union Data;
			friend char *CUtlStringBuilder::InternalPrepareBuffer(size_t, bool, size_t);
		} Heap;

		struct _Stack
		{
		private:
			// last byte is doing a hack double duty.  It holds how many bytes 
			// are left in the string; so when the string is 'full' it will be
			// '0' and thus suffice as the terminating null.  This is why
			// we hold remaining chars instead of 'string length'
			char m_szString[MAX_STACK_STRLEN+1];
		public:
			uint8 BytesLeft() const { return (uint8)(m_szString[MAX_STACK_STRLEN]); }
			void SetBytesLeft( char n ) { m_szString[MAX_STACK_STRLEN] = n; }

			friend char *CUtlStringBuilder::InternalPrepareBuffer(size_t, bool, size_t);
			friend union Data;
		} Stack;

		// set to a clear state without looking at the current state
		void Construct() 
		{
			Stack.m_szString[0] = '\0'; 
			Stack.SetBytesLeft( MAX_STACK_STRLEN ); 
		}

		// If we have heap allocated data, free it
		void FreeHeap()
		{
			if ( IsHeap() && Heap.m_pchString ) 
				free( Heap.m_pchString );
		}

		// Back to a clean state, but retain the error state.
		void Clear() 
		{
			if ( HasError() )
				return;

			FreeHeap();
			Heap.m_pchString = NULL;

			Construct();
		}

		bool IsHeap() const { return ( (Heap.sentinel & STRING_TYPE_SENTINEL) != 0 ); }

		char *Access() { return IsHeap() ? Heap.m_pchString : Stack.m_szString; }
		const char *String() const { return IsHeap() ? Heap.m_pchString : Stack.m_szString; }

		size_t Length() const { return IsHeap() ? Heap.m_nLength : ( MAX_STACK_STRLEN - Stack.BytesLeft() ); }
		bool IsEmpty() const 
		{
			if ( IsHeap() )
				return Heap.m_nLength == 0;
			else
				return Stack.BytesLeft() == MAX_STACK_STRLEN; // empty if all the bytes are available
		}

		size_t Capacity() const { return IsHeap() ? Heap.m_nCapacity : MAX_STACK_STRLEN; }

		// Internally the code often needs the char * after setting the length, so
		// just return it from here for conveniences.
		char *SetLength( size_t nChars );

		// Give the string away and set to an empty state
		char *TakeOwnership( size_t &nLen, size_t &nCapacity )
		{
			MoveToHeap();

			if ( HasError() )
			{
				nLen = 0;
				nCapacity = 0;
				return NULL;
			}

			nLen = Heap.m_nLength;
			nCapacity = Heap.m_nCapacity;
			char *psz = Heap.m_pchString;
			Construct();
			return psz;
		}

		void SetPtr( char *pchString, size_t nLength );

		// Set the string to an error state
		void SetError( bool bEnableAssert );

		// clear the error state and reset the string
		void ClearError();

		// If string is in the heap and the error bit is set in the sentinel
		// the error state is true.
		bool HasError() const { return IsHeap() && ( (Heap.sentinel & STRING_TYPE_ERROR) != 0 ); }

		// If it's stack based, get it to the heap and return if it is
		// successfully on the heap (or already was)
		bool MoveToHeap();

	private:
		//-----------------------------------------------------------------------------
		// Purpose: Needed facts for string class to work
		//-----------------------------------------------------------------------------
		void StaticAssertTests()
		{
			// If this fails when the heap sentinel and where the stack string stores its bytes left
			// aren't aliases.  This is needed so that regardless of how the 'sentinel' to mark
			// that the string is on the heap is set, it is set as expected on both sides of the union.
			COMPILE_TIME_ASSERT( offsetof(_Heap, sentinel) == (offsetof(_Stack, m_szString) + MAX_STACK_STRLEN ) );

			// Lots of code assumes it can look at m_data.Stack.m_nBytesLeft for an empty string; which
			// means that it will equal MAX_STACK_STRLEN.  Therefor it must be a different value than
			// the STRING_TYPE_SENTINEL which will be set if the string is in the heap.
			COMPILE_TIME_ASSERT( MAX_STACK_STRLEN < STRING_TYPE_SENTINEL );
			COMPILE_TIME_ASSERT( MAX_STACK_STRLEN < STRING_TYPE_ERROR );

			// this is a no brainer, and I don't know anywhere in the world this isn't true,
			// but this code does take this dependency.
			COMPILE_TIME_ASSERT( 0 == '\0');
		}
	};

private: // data
	Data m_data;

};

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
inline CUtlStringBuilder::CUtlStringBuilder()
{
	m_data.Construct();
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
inline CUtlStringBuilder::CUtlStringBuilder( size_t nPreallocateBytes ) 
{
	if ( nPreallocateBytes <= MAX_STACK_STRLEN )
	{
		m_data.Construct();
	}
	else
	{
		m_data.Construct();
		PrepareBuffer( 0, false, nPreallocateBytes );
	}
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
inline CUtlStringBuilder::CUtlStringBuilder( const char *pchString )
{
	m_data.Construct();
	SetDirect( pchString, pchString ? strlen( pchString ) : 0 );
}


//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
inline CUtlStringBuilder::CUtlStringBuilder( CUtlStringBuilder const &string )
{
	m_data.Construct();
	SetDirect( string.String(), string.Length() );

	// attempt the copy before checking for error. On the off chance there
	// is data there that can be set, it will help with debugging.
	if ( string.HasError() )
		m_data.SetError( false );
}


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
inline CUtlStringBuilder::~CUtlStringBuilder()
{
	m_data.FreeHeap();
}


//-----------------------------------------------------------------------------
// Purpose: Pre-Widen a string to an expected length
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::EnsureCapacity( size_t nLength )
{
	return PrepareBuffer( Length(), true, nLength ) != NULL;
}


//-----------------------------------------------------------------------------
// Purpose: ask if the string has anything in it
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::IsEmpty() const
{
	return m_data.IsEmpty();
}

//-----------------------------------------------------------------------------
// Purpose: assignment
//-----------------------------------------------------------------------------
inline CUtlStringBuilder &CUtlStringBuilder::operator=( const char *pchString )
{
	SetDirect( pchString, pchString ? strlen( pchString ) : 0 );
	return *this;
}

//-----------------------------------------------------------------------------
// Purpose: assignment
//-----------------------------------------------------------------------------
inline CUtlStringBuilder &CUtlStringBuilder::operator=( CUtlStringBuilder const &src )
{
	if ( &src != this )
	{
		SetDirect( src.String(), src.Length() );
		// error propagate
		if ( src.HasError() )
			m_data.SetError( false );
	}

	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator==( CUtlStringBuilder const &src ) const
{
	return !Q_strcmp( String(), src.String() );
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator==( const char *pchString ) const
{
	return !Q_strcmp( String(), pchString );
}

//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator!=( CUtlStringBuilder const &src ) const
{
	return !( *this == src );
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator!=( const char *pchString ) const
{
	return !( *this == pchString );
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator<( CUtlStringBuilder const &val ) const
{
	return operator<( val.String() );
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator<( const char *pchString ) const
{
	return Q_strcmp( String(), pchString ) < 0;
}

//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator>( CUtlStringBuilder const &val ) const
{
	return Q_strcmp( String(), val.String() ) > 0;
}


//-----------------------------------------------------------------------------
// Purpose: comparison
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::operator>( const char *pchString ) const
{
	return Q_strcmp( String(), pchString ) > 0;
}


//-----------------------------------------------------------------------------
// Return a string with this string and rhs joined together.
inline CUtlStringBuilder& CUtlStringBuilder::operator+=( const char *rhs )
{
	Append( rhs );
	return *this;
}


//-----------------------------------------------------------------------------
// Purpose: returns true if the string is not null
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::IsValid() const
{
	return !HasError();
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
inline const char *CUtlStringBuilder::String() const
{
	const char *pszString = m_data.String();
	if ( pszString )
		return pszString;

	// pszString can be NULL in the error state. For const char*
	// never return NULL.
	return "";
}


//-----------------------------------------------------------------------------
// Purpose: Sets the string to be the new value, taking a copy of it
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::SetValue( const char *pchString )
{
	size_t nLen = ( pchString ? strlen( pchString ) : 0 );
	SetDirect( pchString, nLen );
}


//-----------------------------------------------------------------------------
// Purpose: Set directly and don't look for a null terminator in pValue.
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::SetDirect( const char *pchSource, size_t nChars )
{
	if ( HasError() )
		return;

	if ( m_data.IsHeap() && Get() == pchSource )
		return;

	if ( !pchSource || !nChars )
	{
		m_data.Clear();
		return;
	}

	char *pszString = PrepareBuffer( nChars );

	if ( pszString )
	{
		memcpy( pszString, pchSource, nChars );
		// PrepareBuffer already allocated space for the terminating null, 
		// and inserted it for us. Make sure we didn't clobber it.
		// Also assign it anyways so we don't risk the caller having a buffer
		// running into random bytes.
#ifdef _DEBUG
		// Suppress a bogus noisy warning:
		// warning C6385: Invalid data: accessing 'pszString', the readable size is 'nChars' bytes, but '1001' bytes might be read
		ANALYZE_SUPPRESS(6385);
		Assert( pszString[nChars] == '\0' );
		pszString[nChars] = '\0';
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the string to be the new value, taking a copy of it
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::Set( const char *pchString )
{
	SetValue( pchString );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the string to be the new value, taking ownership of the pointer
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::SetPtr( char *pchString )
{
	size_t nLength = pchString ? strlen( pchString ) : 0;
	SetPtr( pchString, nLength );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the string to be the new value, taking ownership of the pointer
//			This API will clear the error state if it was set.
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::SetPtr( char *pchString, size_t nLength )
{
	m_data.Clear();

	if ( !pchString || !nLength )
	{
		if ( pchString )
			free( pchString ); // we don't hang onto empty strings.
		return;
	}

	m_data.SetPtr( pchString, nLength );
}

//-----------------------------------------------------------------------------
// Purpose: return the conceptual 'strlen' of the string.
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::Length() const
{
	return m_data.Length();
}


//-----------------------------------------------------------------------------
// Purpose: format something sprintf() style, and take it as the new value of this CUtlStringBuilder
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::Format( const char *pFormat, ... )
{
	va_list args;
	va_start( args, pFormat );
	size_t nLen = VFormat( pFormat, args );
	va_end( args );
	return nLen;
}


//-----------------------------------------------------------------------------
// Purpose: Helper for Format() method
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::VFormat( const char *pFormat, va_list args )
{
	if ( HasError() )
		return 0;

	int len = 0;
#ifdef _WIN32
	// how much space will we need?
	len = _vscprintf( pFormat, args );
#else

	// ISO spec defines the NULL/0 case as being valid and will return the
	// needed length. Verified on PS3 as well.  Ignore that bsd/linux/mac
	// have vasprintf which will allocate a buffer. We'd rather have the
	// self growing buffer management ourselves. Even the best implementations
	// There does not seem to be a magic vasprintf that is significantly
	// faster than 2 passes (some guess and get lucky).

    // Scope ReuseArgs.
    {
        CReuseVaList ReuseArgs( args );
        len = vsnprintf( NULL, 0, pFormat, ReuseArgs.m_ReuseList );
    }
#endif
	if ( len > 0 )
	{
		// get it
		char *pszString = PrepareBuffer( len, true );
		if ( pszString )
			len = _vsnprintf( pszString, len + 1, pFormat, args );
		else
			len = 0;
	}
	
	Assert( len > 0 || HasError() );
	return len;
}


//-----------------------------------------------------------------------------
// format a string and append the result to the string we hold
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::AppendFormat( const char *pFormat, ... )
{
	va_list args;
	va_start( args, pFormat );
	size_t nLen = VAppendFormat( pFormat, args );
	va_end( args );
	return nLen;
}



//-----------------------------------------------------------------------------
// Purpose: implementation helper for AppendFormat()
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::VAppendFormat( const char *pFormat, va_list args )
{
	if ( HasError() )
		return 0;

	int len = 0;
#ifdef _WIN32
	// how much space will we need?
	len = _vscprintf( pFormat, args );
#else

	// ISO spec defines the NULL/0 case as being valid and will return the
	// needed length. Verified on PS3 as well.  Ignore that bsd/linux/mac
	// have vasprintf which will allocate a buffer. We'd rather have the
	// self growing buffer management ourselves. Even the best implementations
	// There does not seem to be a magic vasprintf that is significantly
	// faster than 2 passes (some guess and get lucky).

    // Scope ReuseArgs.
    {
        CReuseVaList ReuseArgs( args );
        len = vsnprintf( NULL, 0, pFormat, ReuseArgs.m_ReuseList );
    }
#endif
	size_t nOldLen = Length();

	if ( len > 0 )
	{
		// get it

		char *pszString = PrepareBuffer( nOldLen + len, true );
		if ( pszString )
			len = _vsnprintf( &pszString[nOldLen], len + 1, pFormat, args );
		else
			len = 0;
	}

	Assert( len > 0 || HasError() );
	return nOldLen + len;
}

//-----------------------------------------------------------------------------
// Purpose: concatenate the provided string to our current content
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::Append( const char *pchAddition )
{
	if ( pchAddition && pchAddition[0] )
	{
		size_t cchLen = strlen( pchAddition );
		Append( pchAddition, cchLen );
	}
}


//-----------------------------------------------------------------------------
// Purpose: concatenate the provided string to our current content
//			when the additional string length is known
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::Append( const char *pchAddition, size_t cbLen )
{
	if ( pchAddition && pchAddition[0] && cbLen )
	{
		if ( IsEmpty() )
		{
			SetDirect( pchAddition, cbLen );
		}
		else
		{
			size_t cbOld = Length();
			char *pstrNew = PrepareBuffer( cbOld + cbLen, true );

			// make sure we use raw memcpy to get intrinsic
			if ( pstrNew )
				memcpy( pstrNew + cbOld, pchAddition, cbLen );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: append a repeated series of a single character
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::AppendRepeat( char ch, int cCount )
{
	size_t cbOld = Length();
	char *pstrNew = PrepareBuffer( cbOld + cCount, true );
	if ( pstrNew )
		memset( pstrNew + cbOld, ch, cCount );
}

//-----------------------------------------------------------------------------
// Purpose: Swaps string contents
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::Swap( CUtlStringBuilder &src )
{
	// swapping m_data instead of '*this' prevents having to
	// copy dynamic strings. Important that m_data doesn't know
	// any lifetime rules about its members (ie: it should not have
	// a destructor that frees the dynamic string pointer).
	Swap( m_data, src.m_data );
}


//-----------------------------------------------------------------------------
// Purpose: replace all occurrences of one character with another
//-----------------------------------------------------------------------------
inline size_t CUtlStringBuilder::Replace( char chTarget, char chReplacement )
{
	size_t cReplacements = 0;

	if ( !IsEmpty() && !HasError() )
	{
		char *pszString = Access();
		for ( char *pstrWalker = pszString; *pstrWalker != 0; pstrWalker++ )
		{
			if ( *pstrWalker == chTarget )
			{
				*pstrWalker = chReplacement;
				cReplacements++;
			}
		}
	}

	return cReplacements;
}


//-----------------------------------------------------------------------------
// replace a sequence of characters at a given point with a new string
// replacement string might be NULL or "" to remove target substring
//-----------------------------------------------------------------------------
inline bool CUtlStringBuilder::ReplaceAt( size_t nIndex, size_t nOldChars, const char *pNewStr, size_t nNewChars )
{
    Assert( nIndex < Length() && nIndex + nOldChars <= Length() );

    if ( nNewChars == SIZE_MAX )
    {
        nNewChars = pNewStr ? V_strlen( pNewStr ) : 0;
    }

    size_t nOldLength = Length();
    ptrdiff_t nDelta = nNewChars - nOldChars;
    if ( nDelta < 0 )
    {
        char *pBuf = Access();
        memmove( pBuf + nIndex + nNewChars, pBuf + nIndex + nOldChars, nOldLength - nIndex - nOldChars );
        SetLength( nOldLength + nDelta );
    }
    else if ( nDelta > 0 )
    {
        char *pBuf = PrepareBuffer( nOldLength + nDelta, true );
        if ( !pBuf )
        {
            return false;
        }

        memmove( pBuf + nIndex + nNewChars, pBuf + nIndex + nOldChars, nOldLength - nIndex - nOldChars );
    }

    if ( nNewChars )
    {
        memcpy( Access() + nIndex, pNewStr, nNewChars );
    }

    return true;
}


//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Truncates the string to the specified number of characters
//-----------------------------------------------------------------------------
inline void CUtlStringBuilder::Truncate( size_t nChars )
{
	if ( IsEmpty() || HasError() )
		return;

	size_t nLen = Length();
	if ( nLen <= nChars )
		return;

	// we may be shortening enough to fit in the small buffer, but
	// the external buffer is already allocated, so just keep using it.
	m_data.SetLength( nChars );
}


//-----------------------------------------------------------------------------
// Data and memory validation
//-----------------------------------------------------------------------------
#ifdef DBGFLAG_VALIDATE
inline void CUtlStringBuilder::Validate( CValidator &validator, const char *pchName )
{
#ifdef _WIN32
	validator.Push( typeid(*this).raw_name(), this, pchName );
#else
	validator.Push( typeid(*this).name(), this, pchName );
#endif

	if ( m_data.IsHeap() )
		validator.ClaimMemory( Access() );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE




#endif // UTLSTRING_H
