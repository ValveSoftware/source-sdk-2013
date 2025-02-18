//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================//

#ifndef UTLBUFFER_H
#define UTLBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlmemory.h"
#include "tier1/byteswap.h"
#include <stdarg.h>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct characterset_t;

	
//-----------------------------------------------------------------------------
// Description of character conversions for string output
// Here's an example of how to use the macros to define a character conversion
// BEGIN_CHAR_CONVERSION( CStringConversion, '\\' )
//	{ '\n', "n" },
//	{ '\t', "t" }
// END_CHAR_CONVERSION( CStringConversion, '\\' )
//-----------------------------------------------------------------------------
class CUtlCharConversion
{
public:
	struct ConversionArray_t
	{
		char m_nActualChar;
		const char *m_pReplacementString;
	};

	CUtlCharConversion( char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray );
	char GetEscapeChar() const;
	const char *GetDelimiter() const;
	int GetDelimiterLength() const;

	const char *GetConversionString( char c ) const;
	int GetConversionLength( char c ) const;
	int MaxConversionLength() const;

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion( const char *pString, int *pLength );

protected:
	struct ConversionInfo_t
	{
		int m_nLength;
		const char *m_pReplacementString;
	};

	char m_nEscapeChar;
	const char *m_pDelimiter;
	int m_nDelimiterLength;
	int m_nCount;
	int m_nMaxConversionLength;
	char m_pList[256];
	ConversionInfo_t m_pReplacements[256];
};

#define BEGIN_CHAR_CONVERSION( _name, _delimiter, _escapeChar )	\
	static CUtlCharConversion::ConversionArray_t s_pConversionArray ## _name[] = {

#define END_CHAR_CONVERSION( _name, _delimiter, _escapeChar ) \
	}; \
	CUtlCharConversion _name( _escapeChar, _delimiter, sizeof( s_pConversionArray ## _name ) / sizeof( CUtlCharConversion::ConversionArray_t ), s_pConversionArray ## _name );

#define BEGIN_CUSTOM_CHAR_CONVERSION( _className, _name, _delimiter, _escapeChar ) \
	static CUtlCharConversion::ConversionArray_t s_pConversionArray ## _name[] = {

#define END_CUSTOM_CHAR_CONVERSION( _className, _name, _delimiter, _escapeChar ) \
	}; \
	_className _name( _escapeChar, _delimiter, sizeof( s_pConversionArray ## _name ) / sizeof( CUtlCharConversion::ConversionArray_t ), s_pConversionArray ## _name );

//-----------------------------------------------------------------------------
// Character conversions for C strings
//-----------------------------------------------------------------------------
CUtlCharConversion *GetCStringCharConversion();

//-----------------------------------------------------------------------------
// Character conversions for quoted strings, with no escape sequences
//-----------------------------------------------------------------------------
CUtlCharConversion *GetNoEscCharConversion();


//-----------------------------------------------------------------------------
// Macro to set overflow functions easily
//-----------------------------------------------------------------------------
#define SetUtlBufferOverflowFuncs( _get, _put )	\
	SetOverflowFuncs( static_cast <UtlBufferOverflowFunc_t>( _get ), static_cast <UtlBufferOverflowFunc_t>( _put ) )


//-----------------------------------------------------------------------------
// Command parsing..
//-----------------------------------------------------------------------------
class CUtlBuffer
{
public:
	enum SeekType_t
	{
		SEEK_HEAD = 0,
		SEEK_CURRENT,
		SEEK_TAIL
	};

	// flags
	enum BufferFlags_t
	{
		TEXT_BUFFER = 0x1,			// Describes how get + put work (as strings, or binary)
		EXTERNAL_GROWABLE = 0x2,	// This is used w/ external buffers and causes the utlbuf to switch to reallocatable memory if an overflow happens when Putting.
		CONTAINS_CRLF = 0x4,		// For text buffers only, does this contain \n or \n\r?
		READ_ONLY = 0x8,			// For external buffers; prevents null termination from happening.
		AUTO_TABS_DISABLED = 0x10,	// Used to disable/enable push/pop tabs
	};

	// Overflow functions when a get or put overflows
	typedef bool (CUtlBuffer::*UtlBufferOverflowFunc_t)( int nSize );

	// Constructors for growable + external buffers for serialization/unserialization
	CUtlBuffer( int growSize = 0, int initSize = 0, int nFlags = 0 );
	CUtlBuffer( const void* pBuffer, int size, int nFlags = 0 );
	// This one isn't actually defined so that we catch contructors that are trying to pass a bool in as the third param.
	CUtlBuffer( const void *pBuffer, int size, bool crap );

	unsigned char	GetFlags() const;

	// NOTE: This will assert if you attempt to recast it in a way that
	// is not compatible. The only valid conversion is binary-> text w/CRLF
	void			SetBufferType( bool bIsText, bool bContainsCRLF );

	// Makes sure we've got at least this much memory
	void			EnsureCapacity( int num );

	// Attaches the buffer to external memory....
	void			SetExternalBuffer( void* pMemory, int nSize, int nInitialPut, int nFlags = 0 );
	bool			IsExternallyAllocated() const;
	// Takes ownership of the passed memory, including freeing it when this buffer is destroyed.
	void			AssumeMemory( void *pMemory, int nSize, int nInitialPut, int nFlags = 0 );

	// copies data from another buffer
	void			CopyBuffer( const CUtlBuffer &buffer );
	void			CopyBuffer( const void *pubData, int cubData );

	void			Swap( CUtlBuffer &buf );
	void			Swap( CUtlMemory<uint8> &mem );

	FORCEINLINE void ActivateByteSwappingIfBigEndian( void )
	{
		if ( IsX360() )
			ActivateByteSwapping( true );
	}


	// Controls endian-ness of binary utlbufs - default matches the current platform
	void			ActivateByteSwapping( bool bActivate );
	void			SetBigEndian( bool bigEndian );
	bool			IsBigEndian( void );

	// Resets the buffer; but doesn't free memory
	void			Clear();

	// Clears out the buffer; frees memory
	void			Purge();

	// Read stuff out.
	// Binary mode: it'll just read the bits directly in, and characters will be
	//		read for strings until a null character is reached.
	// Text mode: it'll parse the file, turning text #s into real numbers.
	//		GetString will read a string until a space is reached
	char			GetChar( );
	unsigned char	GetUnsignedChar( );
	short			GetShort( );
	unsigned short	GetUnsignedShort( );
	int				GetInt( );
	int64			GetInt64( );
	int				GetIntHex( );
	unsigned int	GetUnsignedInt( );
	float			GetFloat( );
	double			GetDouble( );
	void			GetString( char* pString, int nMaxChars = 0 );
	void			Get( void* pMem, int size );
	void			GetLine( char* pLine, int nMaxChars = 0 );

	// Used for getting objects that have a byteswap datadesc defined
	template <typename T> void GetObjects( T *dest, int count = 1 );

	// This will get at least 1 byte and up to nSize bytes. 
	// It will return the number of bytes actually read.
	int				GetUpTo( void *pMem, int nSize );

	// This version of GetString converts \" to \\ and " to \, etc.
	// It also reads a " at the beginning and end of the string
	void			GetDelimitedString( CUtlCharConversion *pConv, char *pString, int nMaxChars = 0 );
	char			GetDelimitedChar( CUtlCharConversion *pConv );

	// This will return the # of characters of the string about to be read out
	// NOTE: The count will *include* the terminating 0!!
	// In binary mode, it's the number of characters until the next 0
	// In text mode, it's the number of characters until the next space.
	int				PeekStringLength();

	// This version of PeekStringLength converts \" to \\ and " to \, etc.
	// It also reads a " at the beginning and end of the string
	// NOTE: The count will *include* the terminating 0!!
	// In binary mode, it's the number of characters until the next 0
	// In text mode, it's the number of characters between "s (checking for \")
	// Specifying false for bActualSize will return the pre-translated number of characters
	// including the delimiters and the escape characters. So, \n counts as 2 characters when bActualSize == false
	// and only 1 character when bActualSize == true
	int				PeekDelimitedStringLength( CUtlCharConversion *pConv, bool bActualSize = true );

	// Just like scanf, but doesn't work in binary mode
	int				Scanf( SCANF_FORMAT_STRING const char* pFmt, ... );
	int				VaScanf( const char* pFmt, va_list list );

	// Eats white space, advances Get index
	void			EatWhiteSpace();

	// Eats C++ style comments
	bool			EatCPPComment();

	// (For text buffers only)
	// Parse a token from the buffer:
	// Grab all text that lies between a starting delimiter + ending delimiter
	// (skipping whitespace that leads + trails both delimiters).
	// If successful, the get index is advanced and the function returns true,
	// otherwise the index is not advanced and the function returns false.
	bool			ParseToken( const char *pStartingDelim, const char *pEndingDelim, char* pString, int nMaxLen );

	// Advance the get index until after the particular string is found
	// Do not eat whitespace before starting. Return false if it failed
	// String test is case-insensitive.
	bool			GetToken( const char *pToken );

	// Parses the next token, given a set of character breaks to stop at
	// Returns the length of the token parsed in bytes (-1 if none parsed)
	int				ParseToken( characterset_t *pBreaks, char *pTokenBuf, int nMaxLen, bool bParseComments = true );

	// Write stuff in
	// Binary mode: it'll just write the bits directly in, and strings will be
	//		written with a null terminating character
	// Text mode: it'll convert the numbers to text versions
	//		PutString will not write a terminating character
	void			PutChar( char c );
	void			PutUnsignedChar( unsigned char uc );
	void			PutUint64( uint64 ub );
	void			PutInt16( int16 s16 );
	void			PutShort( short s );
	void			PutUnsignedShort( unsigned short us );
	void			PutInt( int i );
	void			PutInt64( int64 i );
	void			PutUnsignedInt( unsigned int u );
	void			PutFloat( float f );
	void			PutDouble( double d );
	void			PutString( const char* pString );
	void			Put( const void* pMem, int size );

	// Used for putting objects that have a byteswap datadesc defined
	template <typename T> void PutObjects( T *src, int count = 1 );

	// This version of PutString converts \ to \\ and " to \", etc.
	// It also places " at the beginning and end of the string
	void			PutDelimitedString( CUtlCharConversion *pConv, const char *pString );
	void			PutDelimitedChar( CUtlCharConversion *pConv, char c );

	// Just like printf, writes a terminating zero in binary mode
	void			Printf( PRINTF_FORMAT_STRING const char* pFmt, ... ) FMTFUNCTION( 2, 3 );
	void			VaPrintf( const char* pFmt, va_list list );

	// What am I writing (put)/reading (get)?
	void* PeekPut( int offset = 0 );
	const void* PeekGet( int offset = 0 ) const;
	const void* PeekGet( int nMaxSize, int nOffset );

	// Where am I writing (put)/reading (get)?
	int TellPut( ) const;
	int TellGet( ) const;

	// What's the most I've ever written?
	int TellMaxPut( ) const;

	// How many bytes remain to be read?
	// NOTE: This is not accurate for streaming text files; it overshoots
	int GetBytesRemaining() const;

	// Change where I'm writing (put)/reading (get)
	void SeekPut( SeekType_t type, int offset );
	void SeekGet( SeekType_t type, int offset );

	// Buffer base
	const void* Base() const;
	void* Base();
	// Returns the base as a const char*, only valid in text mode.
	const char *String() const;

	// memory allocation size, does *not* reflect size written or read,
	//	use TellPut or TellGet for that
	int Size() const;

	// Am I a text buffer?
	bool IsText() const;

	// Can I grow if I'm externally allocated?
	bool IsGrowable() const;

	// Am I valid? (overflow or underflow error), Once invalid it stays invalid
	bool IsValid() const;

	// Do I contain carriage return/linefeeds? 
	bool ContainsCRLF() const;

	// Am I read-only
	bool IsReadOnly() const;

	// Converts a buffer from a CRLF buffer to a CR buffer (and back)
	// Returns false if no conversion was necessary (and outBuf is left untouched)
	// If the conversion occurs, outBuf will be cleared.
	bool ConvertCRLF( CUtlBuffer &outBuf );

	// Push/pop pretty-printing tabs
	void PushTab();
	void PopTab();

	// Temporarily disables pretty print
	void EnableTabs( bool bEnable );

protected:
	// error flags
	enum
	{
		PUT_OVERFLOW = 0x1,
		GET_OVERFLOW = 0x2,
		MAX_ERROR_FLAG = GET_OVERFLOW,
	};

	void SetOverflowFuncs( UtlBufferOverflowFunc_t getFunc, UtlBufferOverflowFunc_t putFunc );

	bool OnPutOverflow( int nSize );
	bool OnGetOverflow( int nSize );

protected:
	// Checks if a get/put is ok
	bool CheckPut( int size );
	bool CheckGet( int size );

	void AddNullTermination( );

	// Methods to help with pretty-printing
	bool WasLastCharacterCR();
	void PutTabs();

	// Help with delimited stuff
	char GetDelimitedCharInternal( CUtlCharConversion *pConv );
	void PutDelimitedCharInternal( CUtlCharConversion *pConv, char c );

	// Default overflow funcs
	bool PutOverflow( int nSize );
	bool GetOverflow( int nSize );

	// Does the next bytes of the buffer match a pattern?
	bool PeekStringMatch( int nOffset, const char *pString, int nLen );

	// Peek size of line to come, check memory bound
	int	PeekLineLength();

	// How much whitespace should I skip?
	int PeekWhiteSpace( int nOffset );

	// Checks if a peek get is ok
	bool CheckPeekGet( int nOffset, int nSize );

	// Call this to peek arbitrarily long into memory. It doesn't fail unless
	// it can't read *anything* new
	bool CheckArbitraryPeekGet( int nOffset, int &nIncrement );

	template <typename T> void GetType( T& dest, const char *pszFmt );
	template <typename T> void GetTypeBin( T& dest );
	template <typename T> void GetObject( T *src );

	template <typename T> void PutType( T src, const char *pszFmt );
	template <typename T> void PutTypeBin( T src );
	template <typename T> void PutObject( T *src );

	CUtlMemory<unsigned char> m_Memory;
	int m_Get;
	int m_Put;

	unsigned char m_Error;
	unsigned char m_Flags;
	unsigned char m_Reserved;
#if defined( _X360 )
	unsigned char pad;
#endif

	int m_nTab;
	int m_nMaxPut;
	int m_nOffset;

	UtlBufferOverflowFunc_t m_GetOverflowFunc;
	UtlBufferOverflowFunc_t m_PutOverflowFunc;

	CByteswap	m_Byteswap;
};


// Stream style output operators for CUtlBuffer
inline CUtlBuffer &operator<<( CUtlBuffer &b, char v )
{
	b.PutChar( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, unsigned char v )
{
	b.PutUnsignedChar( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, short v )
{
	b.PutShort( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, unsigned short v )
{
	b.PutUnsignedShort( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, int v )
{
	b.PutInt( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, unsigned int v )
{
	b.PutUnsignedInt( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, float v )
{
	b.PutFloat( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, double v )
{
	b.PutDouble( v );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, const char *pv )
{
	b.PutString( pv );
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, const Vector &v )
{
	b << v.x << " " << v.y << " " << v.z;
	return b;
}

inline CUtlBuffer &operator<<( CUtlBuffer &b, const Vector2D &v )
{
	b << v.x << " " << v.y;
	return b;
}


class CUtlInplaceBuffer : public CUtlBuffer
{
public:
	CUtlInplaceBuffer( int growSize = 0, int initSize = 0, int nFlags = 0 );

	//
	// Routines returning buffer-inplace-pointers
	//
public:
	//
	// Upon success, determines the line length, fills out the pointer to the
	// beginning of the line and the line length, advances the "get" pointer
	// offset by the line length and returns "true".
	//
	// If end of file is reached or upon error returns "false".
	//
	// Note:	the returned length of the line is at least one character because the
	//			trailing newline characters are also included as part of the line.
	//
	// Note:	the pointer returned points into the local memory of this buffer, in
	//			case the buffer gets relocated or destroyed the pointer becomes invalid.
	//
	// e.g.:	-------------
	//
	//			char *pszLine;
	//			int nLineLen;
	//			while ( pUtlInplaceBuffer->InplaceGetLinePtr( &pszLine, &nLineLen ) )
	//			{
	//				...
	//			}
	//
	//			-------------
	//
	// @param	ppszInBufferPtr		on return points into this buffer at start of line
	// @param	pnLineLength		on return holds num bytes accessible via (*ppszInBufferPtr)
	//
	// @returns	true				if line was successfully read
	//			false				when EOF is reached or error occurs
	//
	bool InplaceGetLinePtr( /* out */ char **ppszInBufferPtr, /* out */ int *pnLineLength );

	//
	// Determines the line length, advances the "get" pointer offset by the line length,
	// replaces the newline character with null-terminator and returns the initial pointer
	// to now null-terminated line.
	//
	// If end of file is reached or upon error returns NULL.
	//
	// Note:	the pointer returned points into the local memory of this buffer, in
	//			case the buffer gets relocated or destroyed the pointer becomes invalid.
	//
	// e.g.:	-------------
	//
	//			while ( char *pszLine = pUtlInplaceBuffer->InplaceGetLinePtr() )
	//			{
	//				...
	//			}
	//
	//			-------------
	//
	// @returns	ptr-to-zero-terminated-line		if line was successfully read and buffer modified
	//			NULL							when EOF is reached or error occurs
	//
	char * InplaceGetLinePtr( void );
};


//-----------------------------------------------------------------------------
// Where am I reading?
//-----------------------------------------------------------------------------
inline int CUtlBuffer::TellGet( ) const
{
	return m_Get;
}


//-----------------------------------------------------------------------------
// How many bytes remain to be read?
//-----------------------------------------------------------------------------
inline int CUtlBuffer::GetBytesRemaining() const
{
	return m_nMaxPut - TellGet();
}


//-----------------------------------------------------------------------------
// What am I reading?
//-----------------------------------------------------------------------------
inline const void* CUtlBuffer::PeekGet( int offset ) const
{
	return &m_Memory[ m_Get + offset - m_nOffset ];
}


//-----------------------------------------------------------------------------
// Unserialization
//-----------------------------------------------------------------------------

template <typename T> 
inline void CUtlBuffer::GetObject( T *dest )
{
	if ( CheckGet( sizeof(T) ) )
	{
		if ( !m_Byteswap.IsSwappingBytes() || ( sizeof( T ) == 1 ) )
		{
			*dest = *(T *)PeekGet();
		}
		else
		{
			m_Byteswap.SwapFieldsToTargetEndian<T>( dest, (T*)PeekGet() );
		}
		m_Get += sizeof(T);	
	}
	else
	{
		Q_memset( dest, 0, sizeof(T) );
	}
}


template <typename T> 
inline void CUtlBuffer::GetObjects( T *dest, int count )
{
	for ( int i = 0; i < count; ++i, ++dest )
	{
		GetObject<T>( dest );
	}
}


template <typename T> 
inline void CUtlBuffer::GetTypeBin( T &dest )
{
	if ( CheckGet( sizeof(T) ) )
	{
		if ( !m_Byteswap.IsSwappingBytes() || ( sizeof( T ) == 1 ) )
		{
			dest = *(T *)PeekGet();
		}
		else
		{
			m_Byteswap.SwapBufferToTargetEndian<T>( &dest, (T*)PeekGet() );
		}
		m_Get += sizeof(T);	
	}		
	else
	{
		dest = 0;
	}					
}

template <>
inline void CUtlBuffer::GetTypeBin< float >( float &dest )
{
	if ( CheckGet( sizeof( float ) ) )
	{
		uintptr_t pData = (uintptr_t)PeekGet();
		if ( IsX360() && ( pData & 0x03 ) )
		{
			// handle unaligned read
			((unsigned char*)&dest)[0] = ((unsigned char*)pData)[0];
			((unsigned char*)&dest)[1] = ((unsigned char*)pData)[1];
			((unsigned char*)&dest)[2] = ((unsigned char*)pData)[2];
			((unsigned char*)&dest)[3] = ((unsigned char*)pData)[3];
		}
		else
		{
			// aligned read
			dest = *(float *)pData;
		}
		if ( m_Byteswap.IsSwappingBytes() )
		{
			m_Byteswap.SwapBufferToTargetEndian< float >( &dest, &dest );
		}
		m_Get += sizeof( float );	
	}		
	else
	{
		dest = 0;
	}					
}

template <typename T> 
inline void CUtlBuffer::GetType( T &dest, const char *pszFmt )
{
	if (!IsText())
	{
		GetTypeBin( dest );
	}
	else
	{
		dest = 0;
		Scanf( pszFmt, &dest );
	}
}

inline char CUtlBuffer::GetChar( )
{
	char c;
	GetType( c, "%c" );
	return c;
}

inline unsigned char CUtlBuffer::GetUnsignedChar( )
{
	unsigned char c;
	GetType( c, "%u" );
	return c;
}

inline short CUtlBuffer::GetShort( )
{
	short s;
	GetType( s, "%d" );
	return s;
}

inline unsigned short CUtlBuffer::GetUnsignedShort( )
{
	unsigned short s;
	GetType( s, "%u" );
	return s;
}

inline int CUtlBuffer::GetInt( )
{
	int i;
	GetType( i, "%d" );
	return i;
}

inline int64 CUtlBuffer::GetInt64( )
{
	int64 i;
	GetType( i, "%lld" );
	return i;
}

inline int CUtlBuffer::GetIntHex( )
{
	int i;
	GetType( i, "%x" );
	return i;
}

inline unsigned int CUtlBuffer::GetUnsignedInt( )
{
	unsigned int u;
	GetType( u, "%u" );
	return u;
}

inline float CUtlBuffer::GetFloat( )
{
	float f;
	GetType( f, "%f" );
	return f;
}

inline double CUtlBuffer::GetDouble( )
{
	double d;
	GetType( d, "%f" );
	return d;
}


//-----------------------------------------------------------------------------
// Where am I writing?
//-----------------------------------------------------------------------------
inline unsigned char CUtlBuffer::GetFlags() const
{ 
	return m_Flags; 
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::IsExternallyAllocated() const
{ 
	return m_Memory.IsExternallyAllocated();
}

	
//-----------------------------------------------------------------------------
// Where am I writing?
//-----------------------------------------------------------------------------
inline int CUtlBuffer::TellPut( ) const
{
	return m_Put;
}


//-----------------------------------------------------------------------------
// What's the most I've ever written?
//-----------------------------------------------------------------------------
inline int CUtlBuffer::TellMaxPut( ) const
{
	return m_nMaxPut;
}


//-----------------------------------------------------------------------------
// What am I reading?
//-----------------------------------------------------------------------------
inline void* CUtlBuffer::PeekPut( int offset )
{
	return &m_Memory[m_Put + offset - m_nOffset];
}


//-----------------------------------------------------------------------------
// Various put methods
//-----------------------------------------------------------------------------

template <typename T> 
inline void CUtlBuffer::PutObject( T *src )
{
	if ( CheckPut( sizeof(T) ) )
	{
		if ( !m_Byteswap.IsSwappingBytes() || ( sizeof( T ) == 1 ) )
		{
			*(T *)PeekPut() = *src;
		}
		else
		{
			m_Byteswap.SwapFieldsToTargetEndian<T>( (T*)PeekPut(), src );
		}
		m_Put += sizeof(T);
		AddNullTermination();
	}
}


template <typename T> 
inline void CUtlBuffer::PutObjects( T *src, int count )
{
	for ( int i = 0; i < count; ++i, ++src )
	{
		PutObject<T>( src );
	}
}


template <typename T> 
inline void CUtlBuffer::PutTypeBin( T src )
{
	if ( CheckPut( sizeof(T) ) )
	{
		if ( !m_Byteswap.IsSwappingBytes() || ( sizeof( T ) == 1 ) )
		{
			*(T *)PeekPut() = src;
		}
		else
		{
			m_Byteswap.SwapBufferToTargetEndian<T>( (T*)PeekPut(), &src );
		}
		m_Put += sizeof(T);
		AddNullTermination();
	}
}

template <typename T> 
inline void CUtlBuffer::PutType( T src, const char *pszFmt )
{
	if (!IsText())
	{
		PutTypeBin( src );
	}
	else
	{
		Printf( pszFmt, src );
	}
}

//-----------------------------------------------------------------------------
// Methods to help with pretty-printing
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::WasLastCharacterCR()
{
	if ( !IsText() || (TellPut() == 0) )
		return false;
	return ( *( const char * )PeekPut( -1 ) == '\n' );
}

inline void CUtlBuffer::PutTabs()
{
	int nTabCount = ( m_Flags & AUTO_TABS_DISABLED ) ? 0 : m_nTab;
	for (int i = nTabCount; --i >= 0; )
	{
		PutTypeBin<char>( '\t' );
	}
}


//-----------------------------------------------------------------------------
// Push/pop pretty-printing tabs
//-----------------------------------------------------------------------------
inline void CUtlBuffer::PushTab( )
{
	++m_nTab;
}

inline void CUtlBuffer::PopTab()
{
	if ( --m_nTab < 0 )
	{
		m_nTab = 0;
	}
}


//-----------------------------------------------------------------------------
// Temporarily disables pretty print
//-----------------------------------------------------------------------------
inline void CUtlBuffer::EnableTabs( bool bEnable )
{
	if ( bEnable )
	{
		m_Flags &= ~AUTO_TABS_DISABLED;
	}
	else
	{
		m_Flags |= AUTO_TABS_DISABLED; 
	}
}

inline void CUtlBuffer::PutChar( char c )
{
	if ( WasLastCharacterCR() )
	{
		PutTabs();
	}

	PutTypeBin( c );
}

inline void CUtlBuffer::PutUnsignedChar( unsigned char c )
{
	PutType( c, "%u" );
}

inline void CUtlBuffer::PutUint64( uint64 ub )
{
	PutType( ub, "%llu" );
}

inline void CUtlBuffer::PutInt16( int16 s16 )
{
	PutType( s16, "%d" );
}

inline void  CUtlBuffer::PutShort( short s )
{
	PutType( s, "%d" );
}

inline void CUtlBuffer::PutUnsignedShort( unsigned short s )
{
	PutType( s, "%u" );
}

inline void CUtlBuffer::PutInt( int i )
{
	PutType( i, "%d" );
}

inline void CUtlBuffer::PutInt64( int64 i )
{
	PutType( i, "%llu" );
}

inline void CUtlBuffer::PutUnsignedInt( unsigned int u )
{
	PutType( u, "%u" );
}

inline void CUtlBuffer::PutFloat( float f )
{
	PutType( f, "%f" );
}

inline void CUtlBuffer::PutDouble( double d )
{
	PutType( d, "%f" );
}


//-----------------------------------------------------------------------------
// Am I a text buffer?
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::IsText() const 
{ 
	return (m_Flags & TEXT_BUFFER) != 0; 
}


//-----------------------------------------------------------------------------
// Can I grow if I'm externally allocated?
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::IsGrowable() const 
{ 
	return (m_Flags & EXTERNAL_GROWABLE) != 0; 
}


//-----------------------------------------------------------------------------
// Am I valid? (overflow or underflow error), Once invalid it stays invalid
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::IsValid() const 
{ 
	return m_Error == 0; 
}


//-----------------------------------------------------------------------------
// Do I contain carriage return/linefeeds? 
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::ContainsCRLF() const 
{ 
	return IsText() && ((m_Flags & CONTAINS_CRLF) != 0); 
} 


//-----------------------------------------------------------------------------
// Am I read-only
//-----------------------------------------------------------------------------
inline bool CUtlBuffer::IsReadOnly() const
{
	return (m_Flags & READ_ONLY) != 0; 
}


//-----------------------------------------------------------------------------
// Buffer base and size
//-----------------------------------------------------------------------------
inline const void* CUtlBuffer::Base() const	
{ 
	return m_Memory.Base(); 
}

inline void* CUtlBuffer::Base()
{
	return m_Memory.Base(); 
}

// Returns the base as a const char*, only valid in text mode.
inline const char *CUtlBuffer::String() const
{
	Assert( IsText() );
	return reinterpret_cast<const char*>( m_Memory.Base() );
}

inline int CUtlBuffer::Size() const			
{ 
	return m_Memory.NumAllocated(); 
}


//-----------------------------------------------------------------------------
// Clears out the buffer; frees memory
//-----------------------------------------------------------------------------
inline void CUtlBuffer::Clear()
{
	m_Get = 0;
	m_Put = 0;
	m_Error = 0;
	m_nOffset = 0;
	m_nMaxPut = -1;
	AddNullTermination();
}

inline void CUtlBuffer::Purge()
{
	m_Get = 0;
	m_Put = 0;
	m_nOffset = 0;
	m_nMaxPut = 0;
	m_Error = 0;
	m_Memory.Purge();
}

inline void CUtlBuffer::CopyBuffer( const CUtlBuffer &buffer )
{
	CopyBuffer( buffer.Base(), buffer.TellPut() );
}

inline void	CUtlBuffer::CopyBuffer( const void *pubData, int cubData )
{
	Clear();
	if ( cubData )
	{
		Put( pubData, cubData );
	}
}

#endif // UTLBUFFER_H

