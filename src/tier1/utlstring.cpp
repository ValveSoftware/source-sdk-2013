//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "tier1/utlstring.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"
#include <ctype.h>

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

static const int64 k_nMillion = 1000000;

//-----------------------------------------------------------------------------
// Purpose: Helper: Find s substring
//-----------------------------------------------------------------------------
static ptrdiff_t IndexOf( const char *pstrToSearch, const char *pstrTarget )
{
	const char *pstrHit = Q_strstr( pstrToSearch, pstrTarget );
	if ( pstrHit == NULL )
	{
		return -1;	// Not found.
	}
	return ( pstrHit - pstrToSearch );
}


//-----------------------------------------------------------------------------
// Purpose: Helper: kill all whitespace.
//-----------------------------------------------------------------------------
static size_t RemoveWhitespace( char *pszString )
{
	if ( pszString == NULL )
		return 0;

	char *pstrDest = pszString;
	size_t cRemoved = 0;
	for ( char *pstrWalker = pszString; *pstrWalker != 0; pstrWalker++ )
	{
		if ( !V_isspace( (unsigned char)*pstrWalker ) ) 
		{
			*pstrDest = *pstrWalker;
			pstrDest++;
		}
		else
			cRemoved += 1;
	}
	*pstrDest = 0;

	return cRemoved;
}


//-----------------------------------------------------------------------------
// Simple string class. 
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Either allocates or reallocates memory to the length
//
// Allocated space for length characters.  It automatically adds space for the 
// nul and the cached length at the start of the memory block.  Will adjust
// m_pString and explicitly set the nul at the end before returning.
void *CUtlString::AllocMemory( uint32 length )
{
	void *pMemoryBlock;
	if ( m_pString )
	{
		pMemoryBlock = realloc( m_pString, length + 1 );
	}
	else
	{
		pMemoryBlock = malloc( length + 1 );
	}
	m_pString = (char*)pMemoryBlock;
	m_pString[ length ] = 0;

	return pMemoryBlock;
}

//-----------------------------------------------------------------------------
void CUtlString::SetDirect( const char *pValue, int nChars )
{
	if ( pValue && nChars > 0 )
	{
		if ( pValue == m_pString )
		{
			AssertMsg( nChars == Q_strlen(m_pString), "CUtlString::SetDirect does not support resizing strings in place." );
			return; // Do nothing. Realloc in AllocMemory might move pValue's location resulting in a bad memcpy.
		}

		Assert( nChars <= Min<int>( strnlen(pValue, nChars) + 1, nChars ) );
		AllocMemory( nChars );
		Q_memcpy( m_pString, pValue, nChars );
	}
	else
	{
		Purge();
	}

}


void CUtlString::Set( const char *pValue )
{
	int length = pValue ? V_strlen( pValue ) : 0;
	SetDirect( pValue, length );
}

// Sets the length (used to serialize into the buffer )
void CUtlString::SetLength( int nLen )
{
	if ( nLen > 0 )
	{
#ifdef _DEBUG
		int prevLen = m_pString ? Length() : 0;
#endif
		AllocMemory( nLen );
#ifdef _DEBUG
		if ( nLen > prevLen )
		{
			V_memset( m_pString + prevLen, 0xEB, nLen - prevLen );
		}
#endif
	}
	else
	{
		Purge();
	}
}

const char *CUtlString::Get( ) const
{
	if (!m_pString)
	{
		return "";
	}
	return m_pString;
}

char *CUtlString::GetForModify()
{
	if ( !m_pString )
	{
		// In general, we optimise away small mallocs for empty strings
		// but if you ask for the non-const bytes, they must be writable
		// so we can't return "" here, like we do for the const version - jd
		void *pMemoryBlock = malloc( 1 );
		m_pString = (char *)pMemoryBlock;
		*m_pString = 0;
	}

	return m_pString;
}

char CUtlString::operator[]( int i ) const
{
	if ( !m_pString )
		return '\0';

	if ( i >= Length() )
	{
		return '\0';
	}

	return m_pString[i];
}

void CUtlString::Clear()
{
	Purge();
}

void CUtlString::Purge()
{
    free( m_pString );
    m_pString = NULL;
}

bool CUtlString::IsEqual_CaseSensitive( const char *src ) const
{
	if ( !src )
	{
		return (Length() == 0);
	}
	return ( V_strcmp( Get(), src ) == 0 );
}

bool CUtlString::IsEqual_CaseInsensitive( const char *src ) const
{
	if ( !src )
	{
		return (Length() == 0);
	}
	return ( V_stricmp( Get(), src ) == 0 );
}


void CUtlString::ToLower()
{
	if ( !m_pString )
	{
		return;
	}

	V_strlower( m_pString );
}

void CUtlString::ToUpper()
{
	if ( !m_pString )
	{
		return;
	}

	V_strupr( m_pString );
}

CUtlString &CUtlString::operator=( const CUtlString &src )
{
	SetDirect( src.Get(), src.Length() );
	return *this;
}

CUtlString &CUtlString::operator=( const char *src )
{
	Set( src );
	return *this;
}

bool CUtlString::operator==( const CUtlString &src ) const
{
	if ( IsEmpty() )
	{
		if ( src.IsEmpty() )
		{
			return true;
		}

		return false;
	}
	else
	{
		if ( src.IsEmpty() )
		{
			return false;
		}

		return Q_strcmp( m_pString, src.m_pString ) == 0;
	}
}

CUtlString &CUtlString::operator+=( const CUtlString &rhs )
{
	const int lhsLength( Length() );
	const int rhsLength( rhs.Length() );

	if (!rhsLength)
	{
		return *this;
	}

	const int requestedLength( lhsLength + rhsLength );

	AllocMemory( requestedLength );
	Q_memcpy( m_pString + lhsLength, rhs.m_pString, rhsLength );

	return *this;
}

CUtlString &CUtlString::operator+=( const char *rhs )
{
	const int lhsLength( Length() );
	const int rhsLength( V_strlen( rhs ) );
	const int requestedLength( lhsLength + rhsLength );

	if (!requestedLength)
	{
		return *this;
	}

	AllocMemory( requestedLength );
	Q_memcpy( m_pString + lhsLength, rhs, rhsLength );

	return *this;
}

CUtlString &CUtlString::operator+=( char c )
{
	const int lhsLength( Length() );

	AllocMemory( lhsLength + 1 );
	m_pString[ lhsLength ] = c;

	return *this;
}

CUtlString &CUtlString::operator+=( int rhs )
{
	Assert( sizeof( rhs ) == 4 );

	char tmpBuf[ 12 ];	// Sufficient for a signed 32 bit integer [ -2147483648 to +2147483647 ]
	V_snprintf( tmpBuf, sizeof( tmpBuf ), "%d", rhs );
	tmpBuf[ sizeof( tmpBuf ) - 1 ] = '\0';

	return operator+=( tmpBuf );
}

CUtlString &CUtlString::operator+=( double rhs )
{
	char tmpBuf[ 256 ];	// How big can doubles be???  Dunno.
	V_snprintf( tmpBuf, sizeof( tmpBuf ), "%lg", rhs );
	tmpBuf[ sizeof( tmpBuf ) - 1 ] = '\0';

	return operator+=( tmpBuf );
}

bool CUtlString::MatchesPattern( const CUtlString &Pattern, int nFlags ) const
{
	const char *pszSource = String();
	const char *pszPattern = Pattern.String();
	bool	bExact = true;

	while( 1 )
	{
		if ( ( *pszPattern ) == 0 )
		{
			return ( (*pszSource ) == 0 );
		}

		if ( ( *pszPattern ) == '*' )
		{
			pszPattern++;

			if ( ( *pszPattern ) == 0 )
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while( ( *pszPattern ) != '*' && ( *pszPattern ) != 0 )
		{
			nLength++;
			pszPattern++;
		}

		while( 1 )
		{
			const char *pszStartPattern = pszPattern - nLength;
			const char *pszSearch = pszSource;

			for( int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++ )
			{
				if ( ( *pszSearch ) == 0 )
				{
					return false;
				}

				if ( ( *pszSearch ) != ( *pszStartPattern ) )
				{
					break;
				}
			}

			if ( pszSearch - pszSource == nLength )
			{
				break;
			}

			if ( bExact == true )
			{
				return false;
			}

			if ( ( nFlags & PATTERN_DIRECTORY ) != 0 )
			{
				if ( ( *pszPattern ) != '/' && ( *pszSource ) == '/' )
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}


int CUtlString::Format( const char *pFormat, ... )
{
	va_list marker;

	va_start( marker, pFormat );
	int len = FormatV( pFormat, marker );
	va_end( marker );

	return len;
}

//--------------------------------------------------------------------------------------------------
// This can be called from functions that take varargs.
//--------------------------------------------------------------------------------------------------

int CUtlString::FormatV( const char *pFormat, va_list marker )
{
	char tmpBuf[ 4096 ];	//< Nice big 4k buffer, as much memory as my first computer had, a Radio Shack Color Computer

	//va_start( marker, pFormat );
	int len = V_vsprintf_safe( tmpBuf, pFormat, marker );
	//va_end( marker );
	Set( tmpBuf );
	return len;
}

//-----------------------------------------------------------------------------
// Strips the trailing slash
//-----------------------------------------------------------------------------
void CUtlString::StripTrailingSlash()
{
	if ( IsEmpty() )
		return;

	int nLastChar = Length() - 1;
	char c = m_pString[ nLastChar ];
	if ( c == '\\' || c == '/' )
	{
		SetLength( nLastChar );
	}
}

void CUtlString::FixSlashes( char cSeparator/*=CORRECT_PATH_SEPARATOR*/ )
{
	if ( m_pString )
	{
		V_FixSlashes( m_pString, cSeparator );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Indicates if the target string exists in this instance.
//			The index is negative if the target string is not found, otherwise it is the index in the string.
//-----------------------------------------------------------------------------
ptrdiff_t CUtlString::IndexOf( const char *pstrTarget ) const
{
	return ::IndexOf( String(), pstrTarget );
}

//-----------------------------------------------------------------------------
// Trim functions
//-----------------------------------------------------------------------------
void CUtlString::TrimLeft( char cTarget )
{
	int nIndex = 0;

	if ( IsEmpty() )
	{
		return;
	}

	while( m_pString[nIndex] == cTarget )
	{
		++nIndex;
	}

	// We have some whitespace to remove
	if ( nIndex > 0 )
	{
		memcpy( m_pString, &m_pString[nIndex], Length() - nIndex );
		SetLength( Length() - nIndex );
	}
}


void CUtlString::TrimLeft( const char *szTargets )
{
	int i;

	if ( IsEmpty() )
	{
		return;
	}

	for( i = 0; m_pString[i] != 0; i++ )
	{
		bool bWhitespace = false;

		for( int j = 0; szTargets[j] != 0; j++ )
		{
			if ( m_pString[i] == szTargets[j] )
			{
				bWhitespace = true;
				break;
			}
		}

		if ( !bWhitespace )
		{
			break;
		}
	}

	// We have some whitespace to remove
	if ( i > 0 )
	{
		memcpy( m_pString, &m_pString[i], Length() - i );
		SetLength( Length() - i );
	}
}


void CUtlString::TrimRight( char cTarget )
{
	const int nLastCharIndex = Length() - 1;
	int nIndex = nLastCharIndex;

	while ( nIndex >= 0 && m_pString[nIndex] == cTarget )
	{
		--nIndex;
	}

	// We have some whitespace to remove
	if ( nIndex < nLastCharIndex )
	{
		m_pString[nIndex + 1] = 0;
		SetLength( nIndex + 2 );
	}
}


void CUtlString::TrimRight( const char *szTargets )
{
	const int nLastCharIndex = Length() - 1;
	int i;

	for( i = nLastCharIndex; i > 0; i-- )
	{
		bool bWhitespace = false;

		for( int j = 0; szTargets[j] != 0; j++ )
		{
			if ( m_pString[i] == szTargets[j] )
			{
				bWhitespace = true;
				break;
			}
		}

		if ( !bWhitespace )
		{
			break;
		}
	}

	// We have some whitespace to remove
	if ( i < nLastCharIndex )
	{
		m_pString[i + 1] = 0;
		SetLength( i + 2 );
	}
}


void CUtlString::Trim( char cTarget )
{
	TrimLeft( cTarget );
	TrimRight( cTarget );
}


void CUtlString::Trim( const char *szTargets )
{
	TrimLeft( szTargets );
	TrimRight( szTargets );
}


CUtlString CUtlString::Slice( int32 nStart, int32 nEnd ) const
{
	int length = Length();
	if ( length == 0 )
	{
		return CUtlString();
	}

	if ( nStart < 0 )
		nStart = length - (-nStart % length);
	else if ( nStart >= length )
		nStart = length;

	if ( nEnd == INT32_MAX )
		nEnd = length;
	else if ( nEnd < 0 )
		nEnd = length - (-nEnd % length);
	else if ( nEnd >= length )
		nEnd = length;
	
	if ( nStart >= nEnd )
		return CUtlString();

	const char *pIn = String();

	CUtlString ret;
	ret.SetDirect( pIn + nStart, nEnd - nStart );
	return ret;
}

// Grab a substring starting from the left or the right side.
CUtlString CUtlString::Left( int32 nChars ) const
{
	return Slice( 0, nChars );
}

CUtlString CUtlString::Right( int32 nChars ) const
{
	return Slice( -nChars );
}

CUtlString CUtlString::Replace( char cFrom, char cTo ) const
{
	if (!m_pString)
	{
		return CUtlString();
	}

	CUtlString ret = *this;
	int len = ret.Length();
	for ( int i=0; i < len; i++ )
	{
		if ( ret.m_pString[i] == cFrom )
			ret.m_pString[i] = cTo;
	}

	return ret;
}

CUtlString CUtlString::Replace( const char *pchFrom, const char *pchTo, bool bCaseSensitive /*= false*/ ) const
{
	if ( !pchTo )
	{
		return Remove( pchFrom, bCaseSensitive );
	}

	int nTextToReplaceLength = pchFrom ? V_strlen( pchFrom ) : 0;
	CUtlString outputString;
	const char *pSrc = Get();
	if ( pSrc )
	{
		while ( *pSrc )
		{
			char const *pNextOccurrence = bCaseSensitive ? V_strstr( pSrc, pchFrom ) : V_stristr( pSrc, pchFrom );
			if ( !pNextOccurrence )
			{
				// append remaining string
				outputString += pSrc;
				break;
			}

			int nNumCharsToCopy = pNextOccurrence - pSrc;
			if ( nNumCharsToCopy )
			{
				// append up to the undesired substring
				CUtlString temp = pSrc;
				temp = temp.Left( nNumCharsToCopy );
				outputString += temp;
			}

			// Append the replacement
			outputString += pchTo;

			// skip past undesired substring
			pSrc = pNextOccurrence + nTextToReplaceLength;
		}
	}

	return outputString;
}

void CUtlString::RemoveDotSlashes(char separator)
{
	V_RemoveDotSlashes(GetForModify(), separator);
}

// Get a string with the specified substring removed
CUtlString CUtlString::Remove( char const *pTextToRemove, bool bCaseSensitive ) const
{
	int nTextToTemoveLength = pTextToRemove ? V_strlen( pTextToRemove ) : 0;
	CUtlString outputString;
	const char *pSrc = m_pString;
	if ( pSrc )
	{
		while ( *pSrc )
		{
			char const *pNextOccurrence = bCaseSensitive ? V_strstr( pSrc, pTextToRemove ) : V_stristr( pSrc, pTextToRemove );
			if ( !pNextOccurrence )
			{
				// append remaining string
				outputString += pSrc;
				break;
			}

			int nNumCharsToCopy = pNextOccurrence - pSrc;
			if ( nNumCharsToCopy )
			{
				// append up to the undesired substring
				outputString.Append( pSrc, nNumCharsToCopy );
			}

			// skip past undesired substring
			pSrc = pNextOccurrence + nTextToTemoveLength;
		}
	}

	return outputString;
}

CUtlString CUtlString::AbsPath( const char *pStartingDir ) const
{
	char szNew[MAX_PATH];
	V_MakeAbsolutePath( szNew, sizeof( szNew ), this->String(), pStartingDir );
	return CUtlString( szNew );
}

CUtlString CUtlString::UnqualifiedFilename() const
{
	const char *pFilename = V_UnqualifiedFileName( this->String() );
	return CUtlString( pFilename );
}

CUtlString CUtlString::DirName() const
{
	CUtlString ret( this->String() );
	V_StripLastDir( (char*)ret.Get(), ret.Length() + 1 );
	V_StripTrailingSlash( (char*)ret.Get() );
	return ret;
}

CUtlString CUtlString::StripExtension() const
{
	char szTemp[MAX_PATH];
	V_StripExtension( String(), szTemp, sizeof( szTemp ) );
	return CUtlString( szTemp );
}

CUtlString CUtlString::StripFilename() const
{
	const char *pFilename = V_UnqualifiedFileName( Get() ); // NOTE: returns 'Get()' on failure, never NULL
	int nCharsToCopy = pFilename - Get();
	CUtlString result;
	result.SetDirect( Get(), nCharsToCopy );
	result.StripTrailingSlash();
	return result;
}

CUtlString CUtlString::GetBaseFilename() const
{
	char szTemp[MAX_PATH];
	V_FileBase( String(), szTemp, sizeof( szTemp ) );
	return CUtlString( szTemp );
}

CUtlString CUtlString::GetExtension() const
{
	char szTemp[MAX_PATH];
	V_ExtractFileExtension( String(), szTemp, sizeof( szTemp ) );
	return CUtlString( szTemp );
}


CUtlString CUtlString::PathJoin( const char *pStr1, const char *pStr2 )
{
	char szPath[MAX_PATH];
	V_ComposeFileName( pStr1, pStr2, szPath, sizeof( szPath ) );
	return CUtlString( szPath );
}

CUtlString CUtlString::operator+( const char *pOther ) const
{
	CUtlString s = *this;
	s += pOther;
	return s;
}

CUtlString CUtlString::operator+( const CUtlString &other ) const
{
	CUtlString s = *this;
	s += other;
	return s;
}

CUtlString CUtlString::operator+( int rhs ) const
{
	CUtlString ret = *this;
	ret += rhs;
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: concatenate the provided string to our current content
//-----------------------------------------------------------------------------
void CUtlString::Append( const char *pchAddition )
{
	(*this) += pchAddition;
}

void CUtlString::Append( const char *pchAddition, int nChars )
{
	nChars = Min<int>( nChars, V_strlen(	pchAddition ) );

	const int lhsLength( Length() );
	const int rhsLength( nChars );
	const int requestedLength( lhsLength + rhsLength );

	AllocMemory( requestedLength );
	const int allocatedLength( requestedLength );
	const int copyLength( allocatedLength - lhsLength < rhsLength ? allocatedLength - lhsLength : rhsLength );
	memcpy( GetForModify() + lhsLength, pchAddition, copyLength );
	m_pString[ allocatedLength ] = '\0';
}

// Shared static empty string.
const CUtlString &CUtlString::GetEmptyString()
{
	static const CUtlString s_emptyString;

	return s_emptyString;
}

//-----------------------------------------------------------------------------
// Purpose: spill routine for making sure our buffer is big enough for an
//			incoming string set/modify.
//-----------------------------------------------------------------------------
char *CUtlStringBuilder::InternalPrepareBuffer( size_t nChars, bool bCopyOld, size_t nMinCapacity )
{
	Assert( nMinCapacity > Capacity() );
	Assert( nMinCapacity >= nChars );
	// Don't use this class if you want a single 2GB+ string.
	static const size_t k_nMaxStringSize = 0x7FFFFFFFu;
	Assert( nMinCapacity <= k_nMaxStringSize );

	if ( nMinCapacity > k_nMaxStringSize )
	{
		SetError();
		return NULL;
	}

	bool bWasHeap = m_data.IsHeap();
	// add this to whatever we are going to grow so we don't start out too slow
	char *pszString = NULL;
	if ( nMinCapacity > MAX_STACK_STRLEN )
	{
		// Allocate 1.5 times what is requested, plus a small initial ramp
		// value so we don't spend too much time re-allocating tiny buffers.
		// A good allocator will prevent this anyways, but this makes it safer.
		// We cap it at +1 million to not get crazy.  Code actually avoides
		// computing power of two numbers since allocations almost always
		// have header/bookkeeping overhead. Don't do the dynamic sizing
		// if the user asked for a specific capacity.
		static const int k_nInitialMinRamp = 32;
		size_t nNewSize;
		if ( nMinCapacity > nChars )
			nNewSize = nMinCapacity;
		else
			nNewSize = nChars + Min<size_t>( (nChars >> 1) + k_nInitialMinRamp, k_nMillion );

		char *pszOld = m_data.Access();
		size_t nLenOld = m_data.Length();

		// order of operations is very important per comment
		// above. Make sure we copy it before changing m_data
		// in any way
		if ( bWasHeap && bCopyOld )
		{
			// maybe we'll get lucky and get the same buffer back.
			pszString = (char*) realloc( pszOld, nNewSize + 1 );
			if ( !pszString )
			{
				SetError();
				return NULL;
			}
		}
		else // Either it's already on the stack; or we don't need to copy
		{
			// if the current pointer is on the heap, we aren't doing a copy
			// (or we would have used the previous realloc code. So
			// if we aren't doing a copy, don't use realloc since it will
			// copy the data if it needs to make a new allocation.
			if ( bWasHeap )
				free( pszOld );

			pszString = (char*) malloc( nNewSize + 1 );
			if ( !pszString )
			{
				SetError();
				return NULL;
			}

			// still need to do the copy if we are going from small buffer to large
			if ( bCopyOld )
				memcpy( pszString, pszOld, nLenOld ); // null will be added at end of func.
		}

		// just in case the user grabs .Access() and scribbles over the terminator at
		// 'length', make sure they don't run off the rails as long as they obey Capacity.
		// We don't offer this protection for the 'on stack' string.
		pszString[nNewSize] = '\0';

		m_data.Heap.m_pchString = pszString;
		m_data.Heap.m_nCapacity = (uint32)nNewSize; // capacity is the max #chars, not including the null.
		m_data.Heap.m_nLength = (uint32)nChars;
		m_data.Heap.sentinel = STRING_TYPE_SENTINEL;
	}
	else
	{
		// Rare case. Only happens if someone did a SetPtr with a length
		// less than MAX_STACK_STRLEN, or maybe a .Replace() shrunk the
		// length down.
		pszString = m_data.Stack.m_szString;
		m_data.Stack.SetBytesLeft( MAX_STACK_STRLEN - (uint8)nChars );

		if ( bWasHeap )
		{
			char *pszOldString = m_data.Heap.m_pchString;
			if ( bCopyOld )
				memcpy( pszString, pszOldString, nChars ); // null will be added at end of func.

			free( pszOldString );
		}
	}

	pszString[nChars] = '\0';
	return pszString;
}

char *_V_simple_strstr( const char *s1, const char *search )
{
#if defined( _X360 )
	return (char *)strstr( (char *)s1, search );
#else
	return (char *)strstr( s1, search );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: replace all occurrences of one string with another
//			replacement string may be NULL or "" to remove target string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::Replace( const char *pstrTarget, const char *pstrReplacement )
{
	return ReplaceInternal( pstrTarget, pstrReplacement, (const char *(*)(const char *,const char *))_V_simple_strstr );
}


//-----------------------------------------------------------------------------
// Purpose: replace all occurrences of one string with another
//			replacement string may be NULL or "" to remove target string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::ReplaceCaseless( const char *pstrTarget, const char *pstrReplacement )
{
	return ReplaceInternal( pstrTarget, pstrReplacement, Q_stristr );
}

//-----------------------------------------------------------------------------
// Purpose: replace all occurrences of one string with another
//			replacement string may be NULL or "" to remove target string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::ReplaceFastCaseless( const char *pstrTarget, const char *pstrReplacement )
{
	return ReplaceInternal( pstrTarget, pstrReplacement, V_stristr_fast );
}

//-----------------------------------------------------------------------------
// Purpose: replace all occurrences of one string with another
//			replacement string may be NULL or "" to remove target string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::ReplaceInternal( const char *pstrTarget, const char *pstrReplacement, const char *pfnCompare(const char*, const char*)  )
{
	if ( HasError() )
		return 0;

	if ( pstrReplacement == NULL )
		pstrReplacement = "";

	size_t nTargetLength = Q_strlen( pstrTarget );
	size_t nReplacementLength = Q_strlen( pstrReplacement );

	CUtlVector<const char *> vecMatches;
	vecMatches.EnsureCapacity( 8 );

	if ( !IsEmpty() && pstrTarget && *pstrTarget )
	{
		char *pszString = Access();

		// walk the string counting hits
		const char *pstrHit = pszString;
		for ( pstrHit = pfnCompare( pstrHit, pstrTarget ); pstrHit != NULL && *pstrHit != 0; /* inside */ )
		{
			vecMatches.AddToTail( pstrHit );
			// look for the next target and keep looping
			pstrHit = pfnCompare( pstrHit + nTargetLength, pstrTarget );
		}

		// if we didn't miss, get to work
		if ( vecMatches.Count() > 0 )
		{
			// reallocate only once; how big will we need?
			size_t nOldLength = Length();
			size_t nNewLength = nOldLength + ( vecMatches.Count() * ( nReplacementLength - nTargetLength ) );

			if ( nNewLength == 0 )
			{
				// shortcut simple case, even if rare
				m_data.Clear();
			}
			else if ( nNewLength > nOldLength )
			{
				// New string will be bigger than the old, but don't re-alloc unless
				// it is also larger than capacity.  If it fits in capacity, we will
				// be adjusting the string 'in place'.  The replacement string is larger
				// than the target string, so if we copied front to back we would screw up
				// the existing data in the 'in place' case.
				char *pstrNew;
				if ( nNewLength > Capacity() )
				{
					pstrNew = (char*) malloc( nNewLength + 1 );
					if ( !pstrNew )
					{
						SetError();
						return 0;
					}
				}
				else
				{
					pstrNew = PrepareBuffer( nNewLength );
					Assert( pstrNew == pszString );
				}

				const char *pstrPreviousHit = pszString + nOldLength; // end of original string
				char *pstrDestination = pstrNew + nNewLength; // end of target
				*pstrDestination = '\0';
				// Go backwards as noted above.
				FOR_EACH_VEC_BACK( vecMatches, i )
				{
					pstrHit = vecMatches[i];
					size_t nRemainder = pstrPreviousHit - (pstrHit + nTargetLength);
					// copy the bit after the match, back up the destination and move forward from the hit
					memmove( pstrDestination - nRemainder, pstrPreviousHit-nRemainder, nRemainder );
					pstrDestination -= ( nRemainder + nReplacementLength );

					// push the replacement string in
					memcpy( pstrDestination, pstrReplacement, nReplacementLength );
					pstrPreviousHit = pstrHit;
				}

				// copy trailing stuff
				size_t nRemainder = pstrPreviousHit - pszString;
				pstrDestination -= nRemainder;
				if ( pstrDestination != pszString )
				{
					memmove( pstrDestination, pszString, nRemainder );
				}
				
				Assert( pstrNew == pstrDestination );

				// Need to set the pointer if we did were larger than capacity.
				if ( pstrNew != pszString )
					SetPtr( pstrNew, nNewLength );
			}
			else // new is shorter than or same length as old, move in place
			{
				char *pstrNew = Access();
				char *pstrPreviousHit = pstrNew;
				char *pstrDestination = pstrNew;
				FOR_EACH_VEC( vecMatches, i )
				{
					pstrHit = vecMatches[i];
					if ( pstrDestination != pstrPreviousHit )
					{
						// memmove very important as it is ok with overlaps.
						memmove( pstrDestination, pstrPreviousHit, pstrHit - pstrPreviousHit );
					}
					pstrDestination += ( pstrHit - pstrPreviousHit );
					memcpy( pstrDestination, pstrReplacement, nReplacementLength );
					pstrDestination += nReplacementLength;
					pstrPreviousHit = const_cast<char*>(pstrHit) + nTargetLength;
				}

				// copy trailing stuff
				if ( pstrDestination != pstrPreviousHit )
				{
					// memmove very important as it is ok with overlaps.
					size_t nRemainder = (pstrNew + nOldLength) - pstrPreviousHit;
					memmove( pstrDestination, pstrPreviousHit, nRemainder );
				}

				const char *pPtr = PrepareBuffer( nNewLength );
				(void)(pPtr);
				Assert( pPtr == pstrNew );
			}

		}
	}

	return vecMatches.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Indicates if the target string exists in this instance.
//			The index is negative if the target string is not found, otherwise it is the index in the string.
//-----------------------------------------------------------------------------
ptrdiff_t CUtlStringBuilder::IndexOf( const char *pstrTarget ) const
{
	return ::IndexOf( String(), pstrTarget );
}


//-----------------------------------------------------------------------------
// Purpose: 
//			remove whitespace -- anything that is isspace() -- from the string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::RemoveWhitespace( )
{
	if ( HasError() )
		return 0;

	char *pstrDest = m_data.Access();
	size_t cRemoved = ::RemoveWhitespace(pstrDest);

	size_t nNewLength = m_data.Length() - cRemoved;

	if ( cRemoved )
		m_data.SetLength( nNewLength );

	Assert( pstrDest[nNewLength] == '\0' ); // SetLength should have set this

	return cRemoved;
}


//-----------------------------------------------------------------------------
// Purpose:	Allows setting the size to anything under the current
//			capacity.  Typically should not be used unless there was a specific
//			reason to scribble on the string. Will not touch the string contents,
//			but will append a NULL. Returns true if the length was changed.
//-----------------------------------------------------------------------------
bool CUtlStringBuilder::SetLength( size_t nLen )
{
	return m_data.SetLength( nLen ) != NULL;
}


//-----------------------------------------------------------------------------
// Purpose:	Convert to heap string if needed, and give it away.
//-----------------------------------------------------------------------------
char *CUtlStringBuilder::TakeOwnership( size_t *pnLen, size_t *pnCapacity )
{
	size_t nLen = 0;
	size_t nCapacity = 0;
	char *psz = m_data.TakeOwnership( nLen, nCapacity );

	if ( pnLen )
		*pnLen = nLen;

	if ( pnCapacity )
		*pnCapacity = nCapacity;

	return psz;
}


//-----------------------------------------------------------------------------
// Purpose: 
//			trim whitespace from front and back of string
//-----------------------------------------------------------------------------
size_t CUtlStringBuilder::TrimWhitespace()
{
	if ( HasError() )
		return 0;

	char *pchString = m_data.Access();
	int cChars = Q_StrTrim( pchString );

	if ( cChars )
		m_data.SetLength( cChars );

	return cChars;
}

//-----------------------------------------------------------------------------
// Purpose: adjust length and add null terminator, within capacity bounds
//-----------------------------------------------------------------------------
char *CUtlStringBuilder::Data::SetLength( size_t nChars )
{
	// heap/stack must be set correctly, and will not
	// be changed by this routine.
	if ( IsHeap() )
	{
		if ( !Heap.m_pchString || nChars > Heap.m_nCapacity )
			return NULL;
		Heap.m_nLength = (uint32)nChars;
		Heap.m_pchString[nChars] = '\0';
		return Heap.m_pchString;
	}
	if ( nChars > MAX_STACK_STRLEN )
		return NULL;
	Stack.m_szString[nChars] = '\0';
	Stack.SetBytesLeft( MAX_STACK_STRLEN - (uint8)nChars );
	return Stack.m_szString;
}

//-----------------------------------------------------------------------------
// Purpose:	Allows setting the raw pointer and taking ownership
//-----------------------------------------------------------------------------
void CUtlStringBuilder::Data::SetPtr( char *pchString, size_t nLength )
{
	// We don't care about the error state since we are totally replacing
	// the string.

	// ok, length may be small enough to fit in our short buffer
	// but we've already got a dynamically allocated string, so let
	// it be in the heap buffer anyways.
	Heap.m_pchString = pchString;
	Heap.m_nCapacity = (uint32)nLength;
	Heap.m_nLength = (uint32)nLength;
	Heap.sentinel = STRING_TYPE_SENTINEL;

	// their buffer must have room for the null
	Heap.m_pchString[nLength] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose:	Enable the error state, moving the string to the heap if
//			it isn't there.
//-----------------------------------------------------------------------------
void CUtlStringBuilder::Data::SetError( bool bEnableAssert )
{
	if ( HasError() )
		return;

	// This is not meant to be used as a status bit. Setting the error state should
	// mean something very unexpected happened that you would want a call stack for.
	// That is why this asserts unconditionally when the state is being flipped.
	if ( bEnableAssert )
		AssertMsg( false, "Error State on string being set." );

	MoveToHeap();

	Heap.sentinel = ( STRING_TYPE_SENTINEL | STRING_TYPE_ERROR );
}


//-----------------------------------------------------------------------------
// Purpose:	Set string to empty state
//-----------------------------------------------------------------------------
void CUtlStringBuilder::Data::ClearError()
{
	if ( HasError() )
	{
		Heap.sentinel = STRING_TYPE_SENTINEL;
		Clear();
	}
}


//-----------------------------------------------------------------------------
// Purpose:	If the string is on the stack, move it to the heap.
//			create a null heap string if memory can't be allocated.
//			Callers of this /need/ the string to be in the heap state
//			when done.
//-----------------------------------------------------------------------------
bool CUtlStringBuilder::Data::MoveToHeap()
{
	bool bSuccess = true;

	if ( !IsHeap() )
	{
		// try to recover the string at the point of failure, to help with debugging
		size_t nLen = Length();
		char *pszHeapString = (char*)malloc( nLen+1 );
		if ( pszHeapString )
		{
			// get the string copy before corrupting the stack union
			char *pszStackString = Access();
			memcpy( pszHeapString, pszStackString, nLen	);
			pszHeapString[nLen] = 0;

			Heap.m_pchString = pszHeapString;
			Heap.m_nLength = (uint32)nLen;
			Heap.m_nCapacity = (uint32)nLen;
			Heap.sentinel = STRING_TYPE_SENTINEL;
		}
		else
		{
			Heap.m_pchString = NULL;
			Heap.m_nLength = 0;
			Heap.m_nCapacity = 0;
			bSuccess = false;
			Heap.sentinel = ( STRING_TYPE_SENTINEL | STRING_TYPE_ERROR );
		}

	}

	return bSuccess;
}

/*
#if defined(POSIX) && !defined(NO_MALLOC_OVERRIDE)

#include "tier0/memdbgoff.h"

// Some places call CRT routines that return malloc'ed memory,
// so we need a way to call the real CRT free.
void real_free( void *pMem )
{
    free( pMem );
}

#endif
*/
