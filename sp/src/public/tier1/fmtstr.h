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
		static unsigned int scAsserted = 0; \
	\
		va_start(arg_ptr, lastArg); \
		result = Q_vsnprintfRet( (szBuf), nBufSize, (*(ppszFormat)), arg_ptr, &bTruncated ); \
		va_end(arg_ptr); \
	\
		if ( bTruncated && !(bQuietTruncation) && scAsserted < 5 ) \
		{ \
			Assert( !bTruncated ); \
			scAsserted++; \
		} \
		m_nLength = nPrevLen + result; \
	} \
	while (0)


//-----------------------------------------------------------------------------
//
// Purpose: String formatter with specified size
//

template <int SIZE_BUF>
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
		FmtStrVSNPrintf( m_szBuf, SIZE_BUF, m_bQuietTruncation, &pszFormat, 0, pszFormat ); 
		return m_szBuf;
	}

	// Use this for pass-through formatting
	void VSprintf(const char **ppszFormat, ...)
	{
		FmtStrVSNPrintf( m_szBuf, SIZE_BUF, m_bQuietTruncation, ppszFormat, 0, ppszFormat);
	}

	// Compatible API with CUtlString for converting to const char*
	const char *Get( ) const					{ return m_szBuf; }
	const char *String( ) const					{ return m_szBuf; }
	// Use for access
	operator const char *() const				{ return m_szBuf; }
	char *Access()								{ return m_szBuf; }

	CFmtStrN<SIZE_BUF> & operator=( const char *pchValue ) 
	{ 
		V_strncpy( m_szBuf, pchValue, SIZE_BUF );
		m_nLength = V_strlen( m_szBuf );
		return *this; 
	}

	CFmtStrN<SIZE_BUF> & operator+=( const char *pchValue ) 
	{ 
		Append( pchValue ); 
		return *this; 
	}

	int Length() const							{ return m_nLength; }

	void Clear()								
	{ 
		m_szBuf[0] = 0; 
		m_nLength = 0; 
	}

	void AppendFormat(PRINTF_FORMAT_STRING const char *pchFormat, ... ) FMTFUNCTION( 2, 3 )
	{ 
		char *pchEnd = m_szBuf + m_nLength; 
		FmtStrVSNPrintf( pchEnd, SIZE_BUF - m_nLength, m_bQuietTruncation, &pchFormat, m_nLength, pchFormat ); 
	}

	void AppendFormatV( const char *pchFormat, va_list args );
	void Append( const char *pchValue )			{ AppendFormat( "%s", pchValue ); }
	void AppendIndent( uint32 unCount, char chIndent = '\t' );

protected:
	virtual void InitQuietTruncation()
	{
#ifdef _DEBUG
		m_bQuietTruncation = false; 
#else
		m_bQuietTruncation = true;	// Force quiet for release builds
#endif
	}

	bool m_bQuietTruncation;

private:
	char m_szBuf[SIZE_BUF];
	int m_nLength;
};


// Version which will not assert if strings are truncated

template <int SIZE_BUF>
class CFmtStrQuietTruncationN : public CFmtStrN<SIZE_BUF>
{
protected:
	virtual void InitQuietTruncation() { this->m_bQuietTruncation = true; }
};


template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendIndent( uint32 unCount, char chIndent )
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

template< int SIZE_BUF >
void CFmtStrN<SIZE_BUF>::AppendFormatV( const char *pchFormat, va_list args )
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

//=============================================================================

bool BGetLocalFormattedDateAndTime( time_t timeVal, char *pchDate, int cubDate, char *pchTime, int cubTime );
bool BGetLocalFormattedDate( time_t timeVal, char *pchDate, int cubDate );
bool BGetLocalFormattedTime( time_t timeVal, char *pchTime, int cubTime );

#endif // FMTSTR_H
