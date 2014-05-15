//========= Copyright Valve Corporation, All rights reserved. ============//
//
// $Header: $
// $NoKeywords: $
//
// Serialization buffer
//===========================================================================//

#pragma warning (disable : 4514)

#include "utlbuffer.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include "tier1/strtools.h"
#include "tier1/characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
			    

//-----------------------------------------------------------------------------
// Character conversions for C strings
//-----------------------------------------------------------------------------
class CUtlCStringConversion : public CUtlCharConversion
{
public:
	CUtlCStringConversion( char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray );

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion( const char *pString, int *pLength );

private:
	char m_pConversion[256];
};


//-----------------------------------------------------------------------------
// Character conversions for no-escape sequence strings
//-----------------------------------------------------------------------------
class CUtlNoEscConversion : public CUtlCharConversion
{
public:
	CUtlNoEscConversion( char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray ) :
		CUtlCharConversion( nEscapeChar, pDelimiter, nCount, pArray ) {}

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion( const char *pString, int *pLength ) { *pLength = 0; return 0; }
};


//-----------------------------------------------------------------------------
// List of character conversions
//-----------------------------------------------------------------------------
BEGIN_CUSTOM_CHAR_CONVERSION( CUtlCStringConversion, s_StringCharConversion, "\"", '\\' )
	{ '\n', "n" },
	{ '\t', "t" },
	{ '\v', "v" },
	{ '\b', "b" },
	{ '\r', "r" },
	{ '\f', "f" },
	{ '\a', "a" },
	{ '\\', "\\" },
	{ '\?', "\?" },
	{ '\'', "\'" },
	{ '\"', "\"" },
END_CUSTOM_CHAR_CONVERSION( CUtlCStringConversion, s_StringCharConversion, "\"", '\\' )

CUtlCharConversion *GetCStringCharConversion()
{
	return &s_StringCharConversion;
}

BEGIN_CUSTOM_CHAR_CONVERSION( CUtlNoEscConversion, s_NoEscConversion, "\"", 0x7F )
	{ 0x7F, "" },
END_CUSTOM_CHAR_CONVERSION( CUtlNoEscConversion, s_NoEscConversion, "\"", 0x7F )

CUtlCharConversion *GetNoEscCharConversion()
{
	return &s_NoEscConversion;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CUtlCStringConversion::CUtlCStringConversion( char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray ) : 
	CUtlCharConversion( nEscapeChar, pDelimiter, nCount, pArray )
{
	memset( m_pConversion, 0x0, sizeof(m_pConversion) );
	for ( int i = 0; i < nCount; ++i )
	{
		m_pConversion[ (unsigned char) pArray[i].m_pReplacementString[0] ] = pArray[i].m_nActualChar;
	}
}

// Finds a conversion for the passed-in string, returns length
char CUtlCStringConversion::FindConversion( const char *pString, int *pLength )
{
	char c = m_pConversion[ (unsigned char) pString[0] ];
	*pLength = (c != '\0') ? 1 : 0;
	return c;
}



//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CUtlCharConversion::CUtlCharConversion( char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray )
{
	m_nEscapeChar = nEscapeChar;
	m_pDelimiter = pDelimiter;
	m_nCount = nCount;
	m_nDelimiterLength = Q_strlen( pDelimiter );
	m_nMaxConversionLength = 0;

	memset( m_pReplacements, 0, sizeof(m_pReplacements) );

	for ( int i = 0; i < nCount; ++i )
	{
		m_pList[i] = pArray[i].m_nActualChar;
		ConversionInfo_t &info = m_pReplacements[ (unsigned char) m_pList[i] ];
		Assert( info.m_pReplacementString == 0 );
		info.m_pReplacementString = pArray[i].m_pReplacementString;
		info.m_nLength = Q_strlen( info.m_pReplacementString );
		if ( info.m_nLength > m_nMaxConversionLength )
		{
			m_nMaxConversionLength = info.m_nLength;
		}
	}
}


//-----------------------------------------------------------------------------
// Escape character + delimiter
//-----------------------------------------------------------------------------
char CUtlCharConversion::GetEscapeChar() const
{
	return m_nEscapeChar;
}

const char *CUtlCharConversion::GetDelimiter() const
{
	return m_pDelimiter;
}

int CUtlCharConversion::GetDelimiterLength() const
{
	return m_nDelimiterLength;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
const char *CUtlCharConversion::GetConversionString( char c ) const
{
	return m_pReplacements[ (unsigned char) c ].m_pReplacementString;
}

int CUtlCharConversion::GetConversionLength( char c ) const
{
	return m_pReplacements[ (unsigned char) c ].m_nLength;
}

int CUtlCharConversion::MaxConversionLength() const
{
	return m_nMaxConversionLength;
}


//-----------------------------------------------------------------------------
// Finds a conversion for the passed-in string, returns length
//-----------------------------------------------------------------------------
char CUtlCharConversion::FindConversion( const char *pString, int *pLength )
{
	for ( int i = 0; i < m_nCount; ++i )
	{
		if ( !Q_strcmp( pString, m_pReplacements[ (unsigned char) m_pList[i] ].m_pReplacementString ) )
		{
			*pLength = m_pReplacements[ (unsigned char) m_pList[i] ].m_nLength;
			return m_pList[i];
		}
	}

	*pLength = 0;
	return '\0';
}


//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
CUtlBuffer::CUtlBuffer( int growSize, int initSize, int nFlags ) : 
	m_Error(0)
{
	MEM_ALLOC_CREDIT();
	m_Memory.Init( growSize, initSize );
	m_Get = 0;
	m_Put = 0;
	m_nTab = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	if ( (initSize != 0) && !IsReadOnly() )
	{
		m_nMaxPut = -1;
		AddNullTermination();
	}
	else
	{
		m_nMaxPut = 0;
	}
	SetOverflowFuncs( &CUtlBuffer::GetOverflow, &CUtlBuffer::PutOverflow );
}

CUtlBuffer::CUtlBuffer( const void *pBuffer, int nSize, int nFlags ) :
	m_Memory( (unsigned char*)pBuffer, nSize ), m_Error(0)
{
	Assert( nSize != 0 );

	m_Get = 0;
	m_Put = 0;
	m_nTab = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	if ( IsReadOnly() )
	{
		m_nMaxPut = nSize;
	}
	else
	{
		m_nMaxPut = -1;
		AddNullTermination();
	}
	SetOverflowFuncs( &CUtlBuffer::GetOverflow, &CUtlBuffer::PutOverflow );
}


//-----------------------------------------------------------------------------
// Modifies the buffer to be binary or text; Blows away the buffer and the CONTAINS_CRLF value. 
//-----------------------------------------------------------------------------
void CUtlBuffer::SetBufferType( bool bIsText, bool bContainsCRLF )
{
#ifdef _DEBUG
	// If the buffer is empty, there is no opportunity for this stuff to fail
	if ( TellMaxPut() != 0 )
	{
		if ( IsText() )
		{
			if ( bIsText )
			{
				Assert( ContainsCRLF() == bContainsCRLF );
			}
			else
			{
				Assert( ContainsCRLF() );
			}
		}
		else
		{
			if ( bIsText )
			{
				Assert( bContainsCRLF );
			}
		}
	}
#endif

	if ( bIsText )
	{
		m_Flags |= TEXT_BUFFER;
	}
	else
	{
		m_Flags &= ~TEXT_BUFFER;
	}
	if ( bContainsCRLF )
	{
		m_Flags |= CONTAINS_CRLF;
	}
	else
	{
		m_Flags &= ~CONTAINS_CRLF;
	}
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
void CUtlBuffer::SetExternalBuffer( void* pMemory, int nSize, int nInitialPut, int nFlags )
{
	m_Memory.SetExternalBuffer( (unsigned char*)pMemory, nSize );

	// Reset all indices; we just changed memory
	m_Get = 0;
	m_Put = nInitialPut;
	m_nTab = 0;
	m_Error = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	m_nMaxPut = -1;
	AddNullTermination();
}

//-----------------------------------------------------------------------------
// Assumes an external buffer but manages its deletion
//-----------------------------------------------------------------------------
void CUtlBuffer::AssumeMemory( void *pMemory, int nSize, int nInitialPut, int nFlags )
{
	m_Memory.AssumeMemory( (unsigned char*) pMemory, nSize );

	// Reset all indices; we just changed memory
	m_Get = 0;
	m_Put = nInitialPut;
	m_nTab = 0;
	m_Error = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	m_nMaxPut = -1;
	AddNullTermination();
}

//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
void CUtlBuffer::EnsureCapacity( int num )
{
	MEM_ALLOC_CREDIT();
	// Add one extra for the null termination
	num += 1;
	if ( m_Memory.IsExternallyAllocated() )
	{
		if ( IsGrowable() && ( m_Memory.NumAllocated() < num ) )
		{
			m_Memory.ConvertToGrowableMemory( 0 );
		}
		else
		{
			num -= 1;
		}
	}

	m_Memory.EnsureCapacity( num );
}


//-----------------------------------------------------------------------------
// Base get method from which all others derive
//-----------------------------------------------------------------------------
void CUtlBuffer::Get( void* pMem, int size )
{
	if ( size > 0 && CheckGet( size ) )
	{
		int Index = m_Get - m_nOffset;
		Assert( m_Memory.IsIdxValid( Index ) && m_Memory.IsIdxValid( Index + size - 1 ) );

		memcpy( pMem, &m_Memory[ Index ], size );
		m_Get += size;
	}
}


//-----------------------------------------------------------------------------
// This will get at least 1 byte and up to nSize bytes. 
// It will return the number of bytes actually read.
//-----------------------------------------------------------------------------
int CUtlBuffer::GetUpTo( void *pMem, int nSize )
{
	if ( CheckArbitraryPeekGet( 0, nSize ) )
	{
		int Index = m_Get - m_nOffset;
		Assert( m_Memory.IsIdxValid( Index ) && m_Memory.IsIdxValid( Index + nSize - 1 ) );

		memcpy( pMem, &m_Memory[ Index ], nSize );
		m_Get += nSize;
		return nSize;
	}
	return 0;	
}

	
//-----------------------------------------------------------------------------
// Eats whitespace
//-----------------------------------------------------------------------------
void CUtlBuffer::EatWhiteSpace()
{
	if ( IsText() && IsValid() )
	{
		while ( CheckGet( sizeof(char) ) )
		{
			if ( !isspace( *(const unsigned char*)PeekGet() ) )
				break;
			m_Get += sizeof(char);
		}
	}
}


//-----------------------------------------------------------------------------
// Eats C++ style comments
//-----------------------------------------------------------------------------
bool CUtlBuffer::EatCPPComment()
{
	if ( IsText() && IsValid() )
	{
		// If we don't have a a c++ style comment next, we're done
		const char *pPeek = (const char *)PeekGet( 2 * sizeof(char), 0 );
		if ( !pPeek || ( pPeek[0] != '/' ) || ( pPeek[1] != '/' ) )
			return false;

		// Deal with c++ style comments
		m_Get += 2;

		// read complete line
		for ( char c = GetChar(); IsValid(); c = GetChar() )
		{
			if ( c == '\n' )
				break;
		}
		return true;
	}
	return false;
}

	
//-----------------------------------------------------------------------------
// Peeks how much whitespace to eat
//-----------------------------------------------------------------------------
int CUtlBuffer::PeekWhiteSpace( int nOffset )
{
	if ( !IsText() || !IsValid() )
		return 0;

	while ( CheckPeekGet( nOffset, sizeof(char) ) )
	{
		if ( !isspace( *(unsigned char*)PeekGet( nOffset ) ) )
			break;
		nOffset += sizeof(char);
	}

	return nOffset;
}


//-----------------------------------------------------------------------------
// Peek size of sting to come, check memory bound
//-----------------------------------------------------------------------------
int	CUtlBuffer::PeekStringLength()
{
	if ( !IsValid() )
		return 0;

	// Eat preceeding whitespace
	int nOffset = 0;
	if ( IsText() )
	{
		nOffset = PeekWhiteSpace( nOffset );
	}

	int nStartingOffset = nOffset;

	do
	{
		int nPeekAmount = 128;

		// NOTE: Add 1 for the terminating zero!
		if ( !CheckArbitraryPeekGet( nOffset, nPeekAmount ) )
		{
			if ( nOffset == nStartingOffset )
				return 0;
			return nOffset - nStartingOffset + 1;
		}

		const char *pTest = (const char *)PeekGet( nOffset );

		if ( !IsText() )
		{
			for ( int i = 0; i < nPeekAmount; ++i )
			{
				// The +1 here is so we eat the terminating 0
				if ( pTest[i] == 0 )
					return (i + nOffset - nStartingOffset + 1);
			}
		}
		else
		{
			for ( int i = 0; i < nPeekAmount; ++i )
			{
				// The +1 here is so we eat the terminating 0
				if ( isspace((unsigned char)pTest[i]) || (pTest[i] == 0) )
					return (i + nOffset - nStartingOffset + 1);
			}
		}

		nOffset += nPeekAmount;

	} while ( true );
}


//-----------------------------------------------------------------------------
// Peek size of line to come, check memory bound
//-----------------------------------------------------------------------------
int	CUtlBuffer::PeekLineLength()
{
	if ( !IsValid() )
		return 0;

	int nOffset = 0;
	int nStartingOffset = nOffset;

	do
	{
		int nPeekAmount = 128;

		// NOTE: Add 1 for the terminating zero!
		if ( !CheckArbitraryPeekGet( nOffset, nPeekAmount ) )
		{
			if ( nOffset == nStartingOffset )
				return 0;
			return nOffset - nStartingOffset + 1;
		}

		const char *pTest = (const char *)PeekGet( nOffset );

		for ( int i = 0; i < nPeekAmount; ++i )
		{
			// The +2 here is so we eat the terminating '\n' and 0
			if ( pTest[i] == '\n' || pTest[i] == '\r' )
				return (i + nOffset - nStartingOffset + 2);
			// The +1 here is so we eat the terminating 0
			if ( pTest[i] == 0 )
				return (i + nOffset - nStartingOffset + 1);
		}

		nOffset += nPeekAmount;

	} while ( true );
}


//-----------------------------------------------------------------------------
// Does the next bytes of the buffer match a pattern?
//-----------------------------------------------------------------------------
bool CUtlBuffer::PeekStringMatch( int nOffset, const char *pString, int nLen )
{
	if ( !CheckPeekGet( nOffset, nLen ) )
		return false;
	return !Q_strncmp( (const char*)PeekGet(nOffset), pString, nLen );
}


//-----------------------------------------------------------------------------
// This version of PeekStringLength converts \" to \\ and " to \, etc.
// It also reads a " at the beginning and end of the string
//-----------------------------------------------------------------------------
int CUtlBuffer::PeekDelimitedStringLength( CUtlCharConversion *pConv, bool bActualSize )
{
	if ( !IsText() || !pConv )
		return PeekStringLength();

	// Eat preceeding whitespace
	int nOffset = 0;
	if ( IsText() )
	{
		nOffset = PeekWhiteSpace( nOffset );
	}

	if ( !PeekStringMatch( nOffset, pConv->GetDelimiter(), pConv->GetDelimiterLength() ) )
		return 0;

	// Try to read ending ", but don't accept \"
	int nActualStart = nOffset;
	nOffset += pConv->GetDelimiterLength();
	int nLen = 1;	// Starts at 1 for the '\0' termination

	do
	{
		if ( PeekStringMatch( nOffset, pConv->GetDelimiter(), pConv->GetDelimiterLength() ) )
			break;

		if ( !CheckPeekGet( nOffset, 1 ) )
			break;

		char c = *(const char*)PeekGet( nOffset );
		++nLen;
		++nOffset;
		if ( c == pConv->GetEscapeChar() )
		{
			int nLength = pConv->MaxConversionLength();
			if ( !CheckArbitraryPeekGet( nOffset, nLength ) )
				break;

			pConv->FindConversion( (const char*)PeekGet(nOffset), &nLength );
			nOffset += nLength;
		}
	}  while (true);

	return bActualSize ? nLen : nOffset - nActualStart + pConv->GetDelimiterLength() + 1;
}


//-----------------------------------------------------------------------------
// Reads a null-terminated string
//-----------------------------------------------------------------------------
void CUtlBuffer::GetString( char* pString, int nMaxChars )
{
	if (!IsValid())
	{
		*pString = 0;
		return;
	}

	if ( nMaxChars == 0 )
	{
		nMaxChars = INT_MAX;
	}

	// Remember, this *includes* the null character
	// It will be 0, however, if the buffer is empty.
	int nLen = PeekStringLength();

	if ( IsText() )
	{
		EatWhiteSpace();
	}

	if ( nLen == 0 )
	{
		*pString = 0;
		m_Error |= GET_OVERFLOW;
		return;
	}
	
	// Strip off the terminating NULL
	if ( nLen <= nMaxChars )
	{
		Get( pString, nLen - 1 );
		pString[ nLen - 1 ] = 0;
	}
	else
	{
		Get( pString, nMaxChars - 1 );
		pString[ nMaxChars - 1 ] = 0;
		SeekGet( SEEK_CURRENT, nLen - 1 - nMaxChars );
	}

	// Read the terminating NULL in binary formats
	if ( !IsText() )
	{
		VerifyEquals( GetChar(), 0 );
	}
}


//-----------------------------------------------------------------------------
// Reads up to and including the first \n
//-----------------------------------------------------------------------------
void CUtlBuffer::GetLine( char* pLine, int nMaxChars )
{
	Assert( IsText() && !ContainsCRLF() );

	if ( !IsValid() )
	{
		*pLine = 0;
		return;
	}

	if ( nMaxChars == 0 )
	{
		nMaxChars = INT_MAX;
	}

	// Remember, this *includes* the null character
	// It will be 0, however, if the buffer is empty.
	int nLen = PeekLineLength();
	if ( nLen == 0 )
	{
		*pLine = 0;
		m_Error |= GET_OVERFLOW;
		return;
	}
	
	// Strip off the terminating NULL
	if ( nLen <= nMaxChars )
	{
		Get( pLine, nLen - 1 );
		pLine[ nLen - 1 ] = 0;
	}
	else
	{
		Get( pLine, nMaxChars - 1 );
		pLine[ nMaxChars - 1 ] = 0;
		SeekGet( SEEK_CURRENT, nLen - 1 - nMaxChars );
	}
}

	
//-----------------------------------------------------------------------------
// This version of GetString converts \ to \\ and " to \", etc.
// It also places " at the beginning and end of the string
//-----------------------------------------------------------------------------
char CUtlBuffer::GetDelimitedCharInternal( CUtlCharConversion *pConv )
{
	char c = GetChar();
	if ( c == pConv->GetEscapeChar() )
	{
		int nLength = pConv->MaxConversionLength();
		if ( !CheckArbitraryPeekGet( 0, nLength ) )
			return '\0';

		c = pConv->FindConversion( (const char *)PeekGet(), &nLength );
		SeekGet( SEEK_CURRENT, nLength );
	}

	return c;
}

char CUtlBuffer::GetDelimitedChar( CUtlCharConversion *pConv )
{
	if ( !IsText() || !pConv )
		return GetChar( );
	return GetDelimitedCharInternal( pConv );
}

void CUtlBuffer::GetDelimitedString( CUtlCharConversion *pConv, char *pString, int nMaxChars )
{
	if ( !IsText() || !pConv )
	{
		GetString( pString, nMaxChars );
		return;
	}

	if (!IsValid())
	{
		*pString = 0;
		return;
	}

	if ( nMaxChars == 0 )
	{
		nMaxChars = INT_MAX;
	}

	EatWhiteSpace();
	if ( !PeekStringMatch( 0, pConv->GetDelimiter(), pConv->GetDelimiterLength() ) )
		return;

	// Pull off the starting delimiter
	SeekGet( SEEK_CURRENT, pConv->GetDelimiterLength() );

	int nRead = 0;
	while ( IsValid() )
	{
		if ( PeekStringMatch( 0, pConv->GetDelimiter(), pConv->GetDelimiterLength() ) )
		{
			SeekGet( SEEK_CURRENT, pConv->GetDelimiterLength() );
			break;
		}

		char c = GetDelimitedCharInternal( pConv );

		if ( nRead < nMaxChars )
		{
			pString[nRead] = c;
			++nRead;
		}
	}

	if ( nRead >= nMaxChars )
	{
		nRead = nMaxChars - 1;
	}
	pString[nRead] = '\0';
}


//-----------------------------------------------------------------------------
// Checks if a get is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckGet( int nSize )
{
	if ( m_Error & GET_OVERFLOW )
		return false;

	if ( TellMaxPut() < m_Get + nSize )
	{
		m_Error |= GET_OVERFLOW;
		return false;
	}

	if ( ( m_Get < m_nOffset ) || (	m_Memory.NumAllocated() < m_Get - m_nOffset + nSize ) )
	{
		if ( !OnGetOverflow( nSize ) )
		{
			m_Error |= GET_OVERFLOW;
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Checks if a peek get is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckPeekGet( int nOffset, int nSize )
{
	if ( m_Error & GET_OVERFLOW )
		return false;

	// Checking for peek can't set the overflow flag
	bool bOk = CheckGet( nOffset + nSize );
	m_Error &= ~GET_OVERFLOW;
	return bOk;
}


//-----------------------------------------------------------------------------
// Call this to peek arbitrarily long into memory. It doesn't fail unless
// it can't read *anything* new
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckArbitraryPeekGet( int nOffset, int &nIncrement )
{
	if ( TellGet() + nOffset >= TellMaxPut() )
	{
		nIncrement = 0;
		return false;
	}

	if ( TellGet() + nOffset + nIncrement > TellMaxPut() )
	{
		nIncrement = TellMaxPut() - TellGet() - nOffset;
	}

	// NOTE: CheckPeekGet could modify TellMaxPut for streaming files
	// We have to call TellMaxPut again here
	CheckPeekGet( nOffset, nIncrement );
	int nMaxGet = TellMaxPut() - TellGet();
	if ( nMaxGet < nIncrement )
	{
		nIncrement = nMaxGet;
	}
	return (nIncrement != 0);
}


//-----------------------------------------------------------------------------
// Peek part of the butt
//-----------------------------------------------------------------------------
const void* CUtlBuffer::PeekGet( int nMaxSize, int nOffset )
{
	if ( !CheckPeekGet( nOffset, nMaxSize ) )
		return NULL;

	int Index = m_Get + nOffset - m_nOffset;
	Assert( m_Memory.IsIdxValid( Index ) && m_Memory.IsIdxValid( Index + nMaxSize - 1 ) );

	return &m_Memory[ Index ];
}


//-----------------------------------------------------------------------------
// Change where I'm reading
//-----------------------------------------------------------------------------
void CUtlBuffer::SeekGet( SeekType_t type, int offset )	
{
	switch( type )
	{
	case SEEK_HEAD:						 
		m_Get = offset; 
		break;

	case SEEK_CURRENT:
		m_Get += offset;
		break;

	case SEEK_TAIL:
		m_Get = m_nMaxPut - offset;
		break;
	}

	if ( m_Get > m_nMaxPut )
	{
		m_Error |= GET_OVERFLOW;
	}
	else
	{
		m_Error &= ~GET_OVERFLOW;
		if ( m_Get < m_nOffset || m_Get >= m_nOffset + Size() )
		{
			OnGetOverflow( -1 );
		}
	}
}


//-----------------------------------------------------------------------------
// Parse...
//-----------------------------------------------------------------------------

#pragma warning ( disable : 4706 )

int CUtlBuffer::VaScanf( const char* pFmt, va_list list )
{
	Assert( pFmt );
	if ( m_Error || !IsText() )
		return 0;
	
	int numScanned = 0;
	int nLength;
	char c;
	char* pEnd;
	while ( (c = *pFmt++) )
	{
		// Stop if we hit the end of the buffer
		if ( m_Get >= TellMaxPut() )
		{
			m_Error |= GET_OVERFLOW;
			break;
		}

		switch (c)
		{
		case ' ':
			// eat all whitespace
			EatWhiteSpace();
			break;

		case '%':
			{
				// Conversion character... try to convert baby!
				char type = *pFmt++;
				if (type == 0)
					return numScanned;

				switch(type)
				{
				case 'c':
					{
						char* ch = va_arg( list, char * );
						if ( CheckPeekGet( 0, sizeof(char) ) )
						{
							*ch = *(const char*)PeekGet();
							++m_Get;
						}
						else
						{
							*ch = 0;
							return numScanned;
						}
					}
				break;

				case 'i':
				case 'd':
					{
						int* i = va_arg( list, int * );

						// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
						nLength = 128;
						if ( !CheckArbitraryPeekGet( 0, nLength ) )
						{
							*i = 0;
							return numScanned;
						}

						*i = strtol( (char*)PeekGet(), &pEnd, 10 );
						int nBytesRead = (int)( pEnd - (char*)PeekGet() );
						if ( nBytesRead == 0 )
							return numScanned;
						m_Get += nBytesRead;
					}
					break;
				
				case 'x':
					{
						int* i = va_arg( list, int * );

						// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
						nLength = 128;
						if ( !CheckArbitraryPeekGet( 0, nLength ) )
						{
							*i = 0;
							return numScanned;
						}

						*i = strtol( (char*)PeekGet(), &pEnd, 16 );
						int nBytesRead = (int)( pEnd - (char*)PeekGet() );
						if ( nBytesRead == 0 )
							return numScanned;
						m_Get += nBytesRead;
					}
					break;
					
				case 'u':
					{
						unsigned int* u = va_arg( list, unsigned int *);

						// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
						nLength = 128;
						if ( !CheckArbitraryPeekGet( 0, nLength ) )
						{
							*u = 0;
							return numScanned;
						}

						*u = strtoul( (char*)PeekGet(), &pEnd, 10 );
						int nBytesRead = (int)( pEnd - (char*)PeekGet() );
						if ( nBytesRead == 0 )
							return numScanned;
						m_Get += nBytesRead;
					}
					break;
					
				case 'f':
					{
						float* f = va_arg( list, float *);

						// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
						nLength = 128;
						if ( !CheckArbitraryPeekGet( 0, nLength ) )
						{
							*f = 0.0f;
							return numScanned;
						}

						*f = (float)strtod( (char*)PeekGet(), &pEnd );
						int nBytesRead = (int)( pEnd - (char*)PeekGet() );
						if ( nBytesRead == 0 )
							return numScanned;
						m_Get += nBytesRead;
					}
					break;
					
				case 's':
					{
						char* s = va_arg( list, char * );
						GetString( s );
					}
					break;

				default:
					{
						// unimplemented scanf type
						Assert(0);
						return numScanned;
					}
					break;
				}

				++numScanned;
			}
			break;

		default:
			{
				// Here we have to match the format string character
				// against what's in the buffer or we're done.
				if ( !CheckPeekGet( 0, sizeof(char) ) )
					return numScanned;

				if ( c != *(const char*)PeekGet() )
					return numScanned;

				++m_Get;
			}
		}
	}
	return numScanned;
}

#pragma warning ( default : 4706 )

int CUtlBuffer::Scanf( const char* pFmt, ... )
{
	va_list args;

	va_start( args, pFmt );
	int count = VaScanf( pFmt, args );
	va_end( args );

	return count;
}


//-----------------------------------------------------------------------------
// Advance the get index until after the particular string is found
// Do not eat whitespace before starting. Return false if it failed
//-----------------------------------------------------------------------------
bool CUtlBuffer::GetToken( const char *pToken )
{
	Assert( pToken );

	// Look for the token
	int nLen = Q_strlen( pToken );

	int nSizeToCheck = Size() - TellGet() - m_nOffset;

	int nGet = TellGet();
	do
	{
		int nMaxSize = TellMaxPut() - TellGet();
		if ( nMaxSize < nSizeToCheck )
		{
			nSizeToCheck = nMaxSize;
		}
		if ( nLen > nSizeToCheck )
			break;

		if ( !CheckPeekGet( 0, nSizeToCheck ) )
			break;

		const char *pBufStart = (const char*)PeekGet();
		const char *pFoundEnd = Q_strnistr( pBufStart, pToken, nSizeToCheck );
		if ( pFoundEnd )
		{
			size_t nOffset = (size_t)pFoundEnd - (size_t)pBufStart;
			SeekGet( CUtlBuffer::SEEK_CURRENT, nOffset + nLen );
			return true;
		}

		SeekGet( CUtlBuffer::SEEK_CURRENT, nSizeToCheck - nLen - 1 );
		nSizeToCheck = Size() - (nLen-1);

	} while ( true );

	SeekGet( CUtlBuffer::SEEK_HEAD, nGet );
	return false;
}


//-----------------------------------------------------------------------------
// (For text buffers only)
// Parse a token from the buffer:
// Grab all text that lies between a starting delimiter + ending delimiter
// (skipping whitespace that leads + trails both delimiters).
// Note the delimiter checks are case-insensitive.
// If successful, the get index is advanced and the function returns true,
// otherwise the index is not advanced and the function returns false.
//-----------------------------------------------------------------------------
bool CUtlBuffer::ParseToken( const char *pStartingDelim, const char *pEndingDelim, char* pString, int nMaxLen )
{
	int nCharsToCopy = 0;
	int nCurrentGet = 0;

	size_t nEndingDelimLen;

	// Starting delimiter is optional
	char emptyBuf = '\0';
	if ( !pStartingDelim )
	{
		pStartingDelim = &emptyBuf;
	}

	// Ending delimiter is not
	Assert( pEndingDelim && pEndingDelim[0] );
	nEndingDelimLen = Q_strlen( pEndingDelim );

	int nStartGet = TellGet();
	char nCurrChar;
	int nTokenStart = -1;
	EatWhiteSpace( );
	while ( *pStartingDelim )
	{
		nCurrChar = *pStartingDelim++;
		if ( !isspace((unsigned char)nCurrChar) )
		{
			if ( tolower( GetChar() ) != tolower( nCurrChar ) )
				goto parseFailed;
		}
		else
		{
			EatWhiteSpace();
		}
	}

	EatWhiteSpace();
	nTokenStart = TellGet();
	if ( !GetToken( pEndingDelim ) )
		goto parseFailed;

	nCurrentGet = TellGet();
	nCharsToCopy = (nCurrentGet - nEndingDelimLen) - nTokenStart;
	if ( nCharsToCopy >= nMaxLen )
	{
		nCharsToCopy = nMaxLen - 1;
	}

	if ( nCharsToCopy > 0 )
	{
		SeekGet( CUtlBuffer::SEEK_HEAD, nTokenStart );
		Get( pString, nCharsToCopy );
		if ( !IsValid() )
			goto parseFailed;

		// Eat trailing whitespace
		for ( ; nCharsToCopy > 0; --nCharsToCopy )
		{
			if ( !isspace( (unsigned char)pString[ nCharsToCopy-1 ] ) )
				break;
		}
	}
	pString[ nCharsToCopy ] = '\0';

	// Advance the Get index
	SeekGet( CUtlBuffer::SEEK_HEAD, nCurrentGet );
	return true;

parseFailed:
	// Revert the get index
	SeekGet( SEEK_HEAD, nStartGet );
	pString[0] = '\0';
	return false;
}


//-----------------------------------------------------------------------------
// Parses the next token, given a set of character breaks to stop at
//-----------------------------------------------------------------------------
int CUtlBuffer::ParseToken( characterset_t *pBreaks, char *pTokenBuf, int nMaxLen, bool bParseComments )
{
	Assert( nMaxLen > 0 );
	pTokenBuf[0] = 0;

	// skip whitespace + comments
	while ( true )
	{
		if ( !IsValid() )
			return -1;
		EatWhiteSpace();
		if ( bParseComments )
		{
			if ( !EatCPPComment() )	
				break;
		}
		else
		{
			break;
		}
	}
	
	char c = GetChar();
	
	// End of buffer
	if ( c == 0 )
		return -1;

	// handle quoted strings specially
	if ( c == '\"' )
	{
		int nLen = 0;
		while( IsValid() )
		{
			c = GetChar();
			if ( c == '\"' || !c )
			{
				pTokenBuf[nLen] = 0;
				return nLen;
			}
			pTokenBuf[nLen] = c;
			if ( ++nLen == nMaxLen )
			{
				pTokenBuf[nLen-1] = 0;
				return nMaxLen;
			}
		}

		// In this case, we hit the end of the buffer before hitting the end qoute
		pTokenBuf[nLen] = 0;
		return nLen;
	}

	// parse single characters
	if ( IN_CHARACTERSET( *pBreaks, c ) )
	{
		pTokenBuf[0] = c;
		pTokenBuf[1] = 0;
		return 1;
	}

	// parse a regular word
	int nLen = 0;
	while ( true )
	{
		pTokenBuf[nLen] = c;
		if ( ++nLen == nMaxLen )
		{
			pTokenBuf[nLen-1] = 0;
			return nMaxLen;
		}
		c = GetChar();
		if ( !IsValid() )
			break;

		if ( IN_CHARACTERSET( *pBreaks, c ) || c == '\"' || c <= ' ' )
		{
			SeekGet( SEEK_CURRENT, -1 );
			break;
		}
	}
	
	pTokenBuf[nLen] = 0;
	return nLen;
}


	
//-----------------------------------------------------------------------------
// Serialization
//-----------------------------------------------------------------------------
void CUtlBuffer::Put( const void *pMem, int size )
{
	if ( size && CheckPut( size ) )
	{
		int Index = m_Put - m_nOffset;
		Assert( m_Memory.IsIdxValid( Index ) && m_Memory.IsIdxValid( Index + size - 1 ) );
		if( Index >= 0 )
		{
			memcpy( &m_Memory[ Index ], pMem, size );
			m_Put += size;

			AddNullTermination();
		}
	}
}


//-----------------------------------------------------------------------------
// Writes a null-terminated string
//-----------------------------------------------------------------------------
void CUtlBuffer::PutString( const char* pString )
{
	if (!IsText())
	{
		if ( pString )
		{
			// Not text? append a null at the end.
			size_t nLen = Q_strlen( pString ) + 1;
			Put( pString, nLen * sizeof(char) );
			return;
		}
		else
		{
			PutTypeBin<char>( 0 );
		}
	}
	else if (pString)
	{
		int nTabCount = ( m_Flags & AUTO_TABS_DISABLED ) ? 0 : m_nTab;
		if ( nTabCount > 0 )
		{
			if ( WasLastCharacterCR() )
			{
				PutTabs();
			}

			const char* pEndl = strchr( pString, '\n' );
			while ( pEndl )
			{
				size_t nSize = (size_t)pEndl - (size_t)pString + sizeof(char);
				Put( pString, nSize );
				pString = pEndl + 1;
				if ( *pString )
				{
					PutTabs();
					pEndl = strchr( pString, '\n' );
				}
				else
				{
					pEndl = NULL;
				}
			}
		}
		size_t nLen = Q_strlen( pString );
		if ( nLen )
		{
			Put( pString, nLen * sizeof(char) );
		}
	}
}


//-----------------------------------------------------------------------------
// This version of PutString converts \ to \\ and " to \", etc.
// It also places " at the beginning and end of the string
//-----------------------------------------------------------------------------
inline void CUtlBuffer::PutDelimitedCharInternal( CUtlCharConversion *pConv, char c )
{
	int l = pConv->GetConversionLength( c );
	if ( l == 0 )
	{
		PutChar( c );
	}
	else
	{
		PutChar( pConv->GetEscapeChar() );
		Put( pConv->GetConversionString( c ), l );
	}
}

void CUtlBuffer::PutDelimitedChar( CUtlCharConversion *pConv, char c )
{
	if ( !IsText() || !pConv )
	{
		PutChar( c );
		return;
	}

	PutDelimitedCharInternal( pConv, c );
}

void CUtlBuffer::PutDelimitedString( CUtlCharConversion *pConv, const char *pString )
{
	if ( !IsText() || !pConv )
	{
		PutString( pString );
		return;
	}

	if ( WasLastCharacterCR() )
	{
		PutTabs();
	}
	Put( pConv->GetDelimiter(), pConv->GetDelimiterLength() );

	int nLen = pString ? Q_strlen( pString ) : 0;
	for ( int i = 0; i < nLen; ++i )
	{
		PutDelimitedCharInternal( pConv, pString[i] );
	}

	if ( WasLastCharacterCR() )
	{
		PutTabs();
	}
	Put( pConv->GetDelimiter(), pConv->GetDelimiterLength() );
}


void CUtlBuffer::VaPrintf( const char* pFmt, va_list list )
{
	char temp[2048];
#ifdef DBGFLAG_ASSERT	
	int nLen = 
#endif
		Q_vsnprintf( temp, sizeof( temp ), pFmt, list );
	Assert( nLen < 2048 );
	PutString( temp );
}

void CUtlBuffer::Printf( const char* pFmt, ... )
{
	va_list args;

	va_start( args, pFmt );
	VaPrintf( pFmt, args );
	va_end( args );
}


//-----------------------------------------------------------------------------
// Calls the overflow functions
//-----------------------------------------------------------------------------
void CUtlBuffer::SetOverflowFuncs( UtlBufferOverflowFunc_t getFunc, UtlBufferOverflowFunc_t putFunc )
{
	m_GetOverflowFunc = getFunc;
	m_PutOverflowFunc = putFunc;
}

	
//-----------------------------------------------------------------------------
// Calls the overflow functions
//-----------------------------------------------------------------------------
bool CUtlBuffer::OnPutOverflow( int nSize )
{
	return (this->*m_PutOverflowFunc)( nSize );
}

bool CUtlBuffer::OnGetOverflow( int nSize )
{
	return (this->*m_GetOverflowFunc)( nSize );
}

	
//-----------------------------------------------------------------------------
// Checks if a put is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::PutOverflow( int nSize )
{
	MEM_ALLOC_CREDIT();

	if ( m_Memory.IsExternallyAllocated() )
	{
		if ( !IsGrowable() )
			return false;

		m_Memory.ConvertToGrowableMemory( 0 );
	}

	while( Size() < m_Put - m_nOffset + nSize )
	{
		m_Memory.Grow();
	}

	return true;
}

bool CUtlBuffer::GetOverflow( int nSize )
{
	return false;
}
	

//-----------------------------------------------------------------------------
// Checks if a put is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckPut( int nSize )
{
	if ( ( m_Error & PUT_OVERFLOW ) || IsReadOnly() )
		return false;

	if ( ( m_Put < m_nOffset ) || (	m_Memory.NumAllocated() < m_Put - m_nOffset + nSize ) )
	{
		if ( !OnPutOverflow( nSize ) )
		{
			m_Error |= PUT_OVERFLOW;
			return false;
		}
	}
	return true;
}

void CUtlBuffer::SeekPut( SeekType_t type, int offset )	
{
	int nNextPut = m_Put;
	switch( type )
	{
	case SEEK_HEAD:						 
		nNextPut = offset; 
		break;

	case SEEK_CURRENT:
		nNextPut += offset;
		break;

	case SEEK_TAIL:
		nNextPut = m_nMaxPut - offset;
		break;
	}

	// Force a write of the data
	// FIXME: We could make this more optimal potentially by writing out
	// the entire buffer if you seek outside the current range

	// NOTE: This call will write and will also seek the file to nNextPut.
	OnPutOverflow( -nNextPut-1 );
	m_Put = nNextPut;

	AddNullTermination();
}


void CUtlBuffer::ActivateByteSwapping( bool bActivate )
{
	m_Byteswap.ActivateByteSwapping( bActivate );
}

void CUtlBuffer::SetBigEndian( bool bigEndian )
{
	m_Byteswap.SetTargetBigEndian( bigEndian );
}

bool CUtlBuffer::IsBigEndian( void )
{
	return m_Byteswap.IsTargetBigEndian();
}


//-----------------------------------------------------------------------------
// null terminate the buffer
//-----------------------------------------------------------------------------
void CUtlBuffer::AddNullTermination( void )
{
	if ( m_Put > m_nMaxPut )
	{
		if ( !IsReadOnly() && ((m_Error & PUT_OVERFLOW) == 0)  )
		{
			// Add null termination value
			if ( CheckPut( 1 ) )
			{
				int Index = m_Put - m_nOffset;
				Assert( m_Memory.IsIdxValid( Index ) );
				if( Index >= 0 )
				{
					m_Memory[ Index ] = 0;
				}
			}
			else
			{
				// Restore the overflow state, it was valid before...
				m_Error &= ~PUT_OVERFLOW;
			}
		}
		m_nMaxPut = m_Put;
	}		
}


//-----------------------------------------------------------------------------
// Converts a buffer from a CRLF buffer to a CR buffer (and back)
// Returns false if no conversion was necessary (and outBuf is left untouched)
// If the conversion occurs, outBuf will be cleared.
//-----------------------------------------------------------------------------
bool CUtlBuffer::ConvertCRLF( CUtlBuffer &outBuf )
{
	if ( !IsText() || !outBuf.IsText() )
		return false;

	if ( ContainsCRLF() == outBuf.ContainsCRLF() )
		return false;

	int nInCount = TellMaxPut();

	outBuf.Purge();
	outBuf.EnsureCapacity( nInCount );

	bool bFromCRLF = ContainsCRLF();

	// Start reading from the beginning
	int nGet = TellGet();
	int nPut = TellPut();
	int nGetDelta = 0;
	int nPutDelta = 0;

	const char *pBase = (const char*)Base();
	int nCurrGet = 0;
	while ( nCurrGet < nInCount )
	{
		const char *pCurr = &pBase[nCurrGet];
		if ( bFromCRLF )
		{
			const char *pNext = Q_strnistr( pCurr, "\r\n", nInCount - nCurrGet );
			if ( !pNext )
			{
				outBuf.Put( pCurr, nInCount - nCurrGet );
				break;
			}

			int nBytes = (size_t)pNext - (size_t)pCurr;
			outBuf.Put( pCurr, nBytes );
			outBuf.PutChar( '\n' );
			nCurrGet += nBytes + 2;
			if ( nGet >= nCurrGet - 1 )
			{
				--nGetDelta;
			}
			if ( nPut >= nCurrGet - 1 )
			{
				--nPutDelta;
			}
		}
		else
		{
			const char *pNext = Q_strnchr( pCurr, '\n', nInCount - nCurrGet );
			if ( !pNext )
			{
				outBuf.Put( pCurr, nInCount - nCurrGet );
				break;
			}

			int nBytes = (size_t)pNext - (size_t)pCurr;
			outBuf.Put( pCurr, nBytes );
			outBuf.PutChar( '\r' );
			outBuf.PutChar( '\n' );
			nCurrGet += nBytes + 1;
			if ( nGet >= nCurrGet )
			{
				++nGetDelta;
			}
			if ( nPut >= nCurrGet )
			{
				++nPutDelta;
			}
		}
	}

	Assert(	nPut + nPutDelta <= outBuf.TellMaxPut() );

	outBuf.SeekGet( SEEK_HEAD, nGet + nGetDelta ); 
	outBuf.SeekPut( SEEK_HEAD, nPut + nPutDelta ); 

	return true;
}

//-----------------------------------------------------------------------------
// Fast swap
//-----------------------------------------------------------------------------
void CUtlBuffer::Swap( CUtlBuffer &buf )
{
	V_swap( m_Get, buf.m_Get );
	V_swap( m_Put, buf.m_Put );
	V_swap( m_nMaxPut, buf.m_nMaxPut );
	V_swap( m_Error, buf.m_Error );
	m_Memory.Swap( buf.m_Memory );
}


//-----------------------------------------------------------------------------
// Fast swap w/ a CUtlMemory.
//-----------------------------------------------------------------------------
void CUtlBuffer::Swap( CUtlMemory<uint8> &mem )
{
	m_Get = 0;
	m_Put = mem.Count();
	m_nMaxPut = mem.Count();
	m_Error = 0;
	m_Memory.Swap( mem );
}

//---------------------------------------------------------------------------
// Implementation of CUtlInplaceBuffer
//---------------------------------------------------------------------------

CUtlInplaceBuffer::CUtlInplaceBuffer( int growSize /* = 0 */, int initSize /* = 0 */, int nFlags /* = 0 */ ) :
	CUtlBuffer( growSize, initSize, nFlags )
{
}

bool CUtlInplaceBuffer::InplaceGetLinePtr( char **ppszInBufferPtr, int *pnLineLength )
{
	Assert( IsText() && !ContainsCRLF() );

	int nLineLen = PeekLineLength();
	if ( nLineLen <= 1 )
	{
		SeekGet( SEEK_TAIL, 0 );
		return false;
	}

	-- nLineLen; // because it accounts for putting a terminating null-character

	char *pszLine = ( char * ) const_cast< void * >( PeekGet() );
	SeekGet( SEEK_CURRENT, nLineLen );

	// Set the out args
	if ( ppszInBufferPtr )
		*ppszInBufferPtr = pszLine;
	
	if ( pnLineLength )
		*pnLineLength = nLineLen;

	return true;
}

char * CUtlInplaceBuffer::InplaceGetLinePtr( void )
{
	char *pszLine = NULL;
	int nLineLen = 0;
	
	if ( InplaceGetLinePtr( &pszLine, &nLineLen ) )
	{
		Assert( nLineLen >= 1 );

		switch ( pszLine[ nLineLen - 1 ] )
		{
		case '\n':
		case '\r':
			pszLine[ nLineLen - 1 ] = 0;
			if ( -- nLineLen )
			{
				switch ( pszLine[ nLineLen - 1 ] )
				{
				case '\n':
				case '\r':
					pszLine[ nLineLen - 1 ] = 0;
					break;
				}
			}
			break;
		
		default:
			Assert( pszLine[ nLineLen ] == 0 );
			break;
		}
	}

	return pszLine;
}

