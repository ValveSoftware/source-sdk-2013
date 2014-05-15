//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A simple class for performing safe and in-expression sprintf-style
//			string formatting
//
// $NoKeywords: $
//=============================================================================//

#ifndef FMTSTR_H
#define FMTSTR_H

#include <stdarg.h>
#include <stdio.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier1/strtools.h"

#if defined( _WIN32 )
#pragma once
#endif
#if defined(POSIX)
#pragma GCC visibility push(hidden)
#endif

//=============================================================================

// using macro to be compatable with GCC
#define FmtStrVSNPrintf( szBuf, nBufSize, bQuietTruncation, ppszFormat, nPrevLen, lastArg ) \
	do \
	{ \
		int     result; \
		va_list arg_ptr; \
		bool bTruncated = false; \
		static int scAsserted = 0; \
	\
		va_start(arg_ptr, lastArg); \
		result = V_vsnprintfRet( (szBuf), (nBufSize)-1, (*(ppszFormat)), arg_ptr, &bTruncated ); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
		if ( bTruncated && !(bQuietTruncation) && scAsserted < 5 ) \
		{ \
			Warning( "FmtStrVSNPrintf truncated to %d without QUIET_TRUNCATION specified!\n", ( int )( nBufSize ) ); \
			AssertMsg( 0, "FmtStrVSNPrintf truncated without QUIET_TRUNCATION specified!\n" ); \
			scAsserted++; \
		} \
		m_nLength = nPrevLen + result; \
	} \
	while (0)

// using macro to be compatable with GCC
#define FmtStrVSNPrintfNoLengthFixup( szBuf, nBufSize, bQuietTruncation, ppszFormat, nPrevLen, lastArg ) \
	do \
	{ \
		int     result; \
		va_list arg_ptr; \
		bool bTruncated = false; \
		static int scAsserted = 0; \
	\
		va_start(arg_ptr, lastArg); \
		result = V_vsnprintfRet( (szBuf), (nBufSize)-1, (*(ppszFormat)), arg_ptr, &bTruncated ); \
		va_end(arg_ptr); \
	\
		(szBuf)[(nBufSize)-1] = 0; \
		if ( bTruncated && !(bQuietTruncation) && scAsserted < 5 ) \
		{ \
			Warning( "FmtStrVSNPrintf truncated to %d without QUIET_TRUNCATION specified!\n", ( int )( nBufSize ) ); \
			AssertMsg( 0, "FmtStrVSNPrintf truncated without QUIET_TRUNCATION specified!\n" ); \
			scAsserted++; \
		} \
	} \
	while (0)

//-----------------------------------------------------------------------------
//
// Purpose: String formatter with specified size
//

template <int SIZE_BUF, bool QUIET_TRUNCATION = false >
class CFmtStrN
{
public:
	CFmtStrN()	
	{ 
		InitQuietTruncation();
		m_szBuf[0] = 0; 
		m_nLength = 0;
	}
	
	// Standard C formatting
	CFmtStrN(PRINTF_FORMAT_STRING const char *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{
		InitQuietTruncation();
		FmtStrVSNPrintf( m_szBuf, SIZE_BUF, m_bQuietTruncation, &pszFormat, 0, pszFormat );
	}

	// Use this for pass-through formatting
	CFmtStrN(const char ** ppszFormat, ...)
	{
		InitQuietTruncation();
		FmtStrVSNPrintf( m_szBuf, SIZE_BUF, m_bQuietTruncation, ppszFormat, 0, ppszFormat );
	}

	// Explicit reformat
	const char *sprintf(PRINTF_FORMAT_STRING const char *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{
		InitQuietTruncation();
		FmtStrVSNPrintf(m_szBuf, SIZE_BUF, m_bQuietTruncation, &pszFormat, 0, pszFormat ); 
		return m_szBuf;
	}

	// Use this for va_list formatting
	const char *sprintf_argv(const char *pszFormat, va_list arg_ptr)
	{
		int result; 
		bool bTruncated = false; 
		static int s_nWarned = 0; 

		InitQuietTruncation();
		result = V_vsnprintfRet( m_szBuf, SIZE_BUF - 1, pszFormat, arg_ptr, &bTruncated );
		m_szBuf[SIZE_BUF - 1] = 0; 
		if ( bTruncated && !m_bQuietTruncation && ( s_nWarned < 5 ) ) 
		{ 
			Warning( "CFmtStr truncated to %d without QUIET_TRUNCATION specified!\n", SIZE_BUF ); 
			AssertMsg( 0, "CFmtStr truncated without QUIET_TRUNCATION specified!\n" );
			s_nWarned++; 
		} 
		m_nLength = V_strlen( m_szBuf );
		return m_szBuf;
	}

	// Use this for pass-through formatting
	void VSprintf(const char **ppszFormat, ...)
	{
		InitQuietTruncation();
		FmtStrVSNPrintf( m_szBuf, SIZE_BUF, m_bQuietTruncation, ppszFormat, 0, ppszFormat );
	}

	// Compatible API with CUtlString for converting to const char*
	const char *Get( ) const					{ return m_szBuf; }
	const char *String( ) const					{ return m_szBuf; }
	// Use for access
	operator const char *() const				{ return m_szBuf; }
	char *Access()								{ return m_szBuf; }

	// Access template argument
	static inline int GetMaxLength() { return SIZE_BUF-1; }

	CFmtStrN<SIZE_BUF,QUIET_TRUNCATION> & operator=( const char *pchValue ) 
	{ 
		V_strncpy( m_szBuf, pchValue, SIZE_BUF );
		m_nLength = V_strlen( m_szBuf );
		return *this; 
	}

	CFmtStrN<SIZE_BUF,QUIET_TRUNCATION> & operator+=( const char *pchValue ) 
	{ 
		Append( pchValue ); 
		return *this; 
	}

	int Length() const							{ return m_nLength; }

	void SetLength( int nLength )
	{
		m_nLength = Min( nLength, SIZE_BUF - 1 );
		m_szBuf[m_nLength] = '\0';
	}

	void Clear()								
	{ 
		m_szBuf[0] = 0; 
		m_nLength = 0; 
	}

	void AppendFormat( PRINTF_FORMAT_STRING const char *pchFormat, ... ) 
	{ 
		char *pchEnd = m_szBuf + m_nLength; 
		FmtStrVSNPrintf( pchEnd, SIZE_BUF - m_nLength, m_bQuietTruncation, &pchFormat, m_nLength, pchFormat ); 
	}

	void AppendFormatV( const char *pchFormat, va_list args );
	
	void Append( const char *pchValue )
	{
		// This function is close to the metal to cut down on the CPU cost
		// of the previous incantation of Append which was implemented as
		// AppendFormat( "%s", pchValue ). This implementation, though not
		// as easy to read, instead does a strcpy from the existing end
		// point of the CFmtStrN. This brings something like a 10-20x speedup
		// in my rudimentary tests. It isn't using V_strncpy because that
		// function doesn't return the number of characters copied, which
		// we need to adjust m_nLength. Doing the V_strncpy with a V_strlen
		// afterwards took twice as long as this implementations in tests,
		// so V_strncpy's implementation was used to write this method.
		char *pDest = m_szBuf + m_nLength;
		const int maxLen = SIZE_BUF - m_nLength;
		char *pLast = pDest + maxLen - 1;
		while ( (pDest < pLast) && (*pchValue != 0) )
		{
			*pDest = *pchValue;
			++pDest; ++pchValue;
		}
		*pDest = 0;
		m_nLength = pDest - m_szBuf;
	}

	//optimized version of append for just adding a single character
	void Append( char ch )
	{
		if( m_nLength < SIZE_BUF - 1 )
		{
			m_szBuf[ m_nLength ] = ch;
			m_nLength++;
			m_szBuf[ m_nLength ] = '\0';
		}
	}

	void AppendIndent( uint32 unCount, char chIndent = '\t' );

	void SetQuietTruncation( bool bQuiet ) { m_bQuietTruncation = bQuiet; }

protected:
	virtual void InitQuietTruncation()
	{
		m_bQuietTruncation = QUIET_TRUNCATION; 
	}

	bool m_bQuietTruncation;

private:
	char m_szBuf[SIZE_BUF];
	int m_nLength;
};


// Version which will not assert if strings are truncated

template < int SIZE_BUF >
class CFmtStrQuietTruncationN : public CFmtStrN<SIZE_BUF, true >
{
};


template< int SIZE_BUF, bool QUIET_TRUNCATION >
void CFmtStrN< SIZE_BUF, QUIET_TRUNCATION >::AppendIndent( uint32 unCount, char chIndent )
{
	Assert( Length() + unCount < SIZE_BUF );
	if( Length() + unCount >= SIZE_BUF )
		unCount = SIZE_BUF - (1+Length());
	for ( uint32 x = 0; x < unCount; x++ )
	{
		m_szBuf[ m_nLength++ ] = chIndent;
	}
	m_szBuf[ m_nLength ] = '\0';
}

template< int SIZE_BUF, bool QUIET_TRUNCATION >
void CFmtStrN< SIZE_BUF, QUIET_TRUNCATION >::AppendFormatV( const char *pchFormat, va_list args )
{
	int cubPrinted = V_vsnprintf( m_szBuf+Length(), SIZE_BUF - Length(), pchFormat, args );
	m_nLength += cubPrinted;
}


#if defined(POSIX)
#pragma GCC visibility pop
#endif

//-----------------------------------------------------------------------------
//
// Purpose: Default-sized string formatter
//

#define FMTSTR_STD_LEN 256

typedef CFmtStrN<FMTSTR_STD_LEN> CFmtStr;
typedef CFmtStrQuietTruncationN<FMTSTR_STD_LEN> CFmtStrQuietTruncation;
typedef CFmtStrN<1024> CFmtStr1024;
typedef CFmtStrN<8192> CFmtStrMax;


//-----------------------------------------------------------------------------
// Purpose: Fast-path number-to-string helper (with optional quoting)
//			Derived off of the Steam CNumStr but with a few tweaks, such as
//			trimming off the in-our-cases-unnecessary strlen calls (by not
//			storing the length in the class).
//-----------------------------------------------------------------------------

class CNumStr
{
public:
	CNumStr() { m_szBuf[0] = 0; }

	explicit CNumStr( bool b )		{ SetBool( b ); } 

	explicit CNumStr( int8 n8 )		{ SetInt8( n8 ); }
	explicit CNumStr( uint8 un8 )	{ SetUint8( un8 );  }
	explicit CNumStr( int16 n16 )	{ SetInt16( n16 ); }
	explicit CNumStr( uint16 un16 )	{ SetUint16( un16 );  }
	explicit CNumStr( int32 n32 )	{ SetInt32( n32 ); }
	explicit CNumStr( uint32 un32 )	{ SetUint32( un32 ); }
	explicit CNumStr( int64 n64 )	{ SetInt64( n64 ); }
	explicit CNumStr( uint64 un64 )	{ SetUint64( un64 ); }

#if defined(COMPILER_GCC) && defined(PLATFORM_64BITS)
	explicit CNumStr( lint64 n64 )		{ SetInt64( (int64)n64 ); }
	explicit CNumStr( ulint64 un64 )	{ SetUint64( (uint64)un64 ); }
#endif

	explicit CNumStr( double f )	{ SetDouble( f ); }
	explicit CNumStr( float f )		{ SetFloat( f ); }

	inline void SetBool( bool b )			{ Q_memcpy( m_szBuf, b ? "1" : "0", 2 ); } 

#ifdef _WIN32
	inline void SetInt8( int8 n8 )			{ _itoa( (int32)n8, m_szBuf, 10 ); }
	inline void SetUint8( uint8 un8 )		{ _itoa( (int32)un8, m_szBuf, 10 ); }
	inline void SetInt16( int16 n16 )		{ _itoa( (int32)n16, m_szBuf, 10 ); }
	inline void SetUint16( uint16 un16 )	{ _itoa( (int32)un16, m_szBuf, 10 ); }
	inline void SetInt32( int32 n32 )		{ _itoa( n32, m_szBuf, 10 ); }
	inline void SetUint32( uint32 un32 )	{ _i64toa( (int64)un32, m_szBuf, 10 ); }
	inline void SetInt64( int64 n64 )		{ _i64toa( n64, m_szBuf, 10 ); }
	inline void SetUint64( uint64 un64 )	{ _ui64toa( un64, m_szBuf, 10 ); }
#else
	inline void SetInt8( int8 n8 )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n8 ); }
	inline void SetUint8( uint8 un8 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un8 ); }
	inline void SetInt16( int16 n16 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)n16 ); }
	inline void SetUint16( uint16 un16 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", (int32)un16 ); }
	inline void SetInt32( int32 n32 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%d", n32 ); }
	inline void SetUint32( uint32 un32 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%u", un32 ); }
	inline void SetInt64( int64 n64 )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%lld", n64 ); }
	inline void SetUint64( uint64 un64 )	{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%llu", un64 ); }
#endif

	inline void SetDouble( double f )		{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }
	inline void SetFloat( float f )			{ Q_snprintf( m_szBuf, sizeof(m_szBuf), "%.18g", f ); }

	inline void SetHexUint64( uint64 un64 )	{ Q_binarytohex( (byte *)&un64, sizeof( un64 ), m_szBuf, sizeof( m_szBuf ) ); }

	operator const char *() const { return m_szBuf; }
	const char* String() const { return m_szBuf; }
	
	void AddQuotes()
	{
		Assert( m_szBuf[0] != '"' );
		const int nLength = Q_strlen( m_szBuf );
		Q_memmove( m_szBuf + 1, m_szBuf, nLength );
		m_szBuf[0] = '"';
		m_szBuf[nLength + 1] = '"';
		m_szBuf[nLength + 2] = 0;
	}

protected:
	char m_szBuf[28]; // long enough to hold 18 digits of precision, a decimal, a - sign, e+### suffix, and quotes

};


//=============================================================================

bool BGetLocalFormattedDateAndTime( time_t timeVal, char *pchDate, int cubDate, char *pchTime, int cubTime );
bool BGetLocalFormattedDate( time_t timeVal, char *pchDate, int cubDate );
bool BGetLocalFormattedTime( time_t timeVal, char *pchTime, int cubTime );

#endif // FMTSTR_H
