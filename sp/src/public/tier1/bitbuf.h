//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// NOTE: bf_read is guaranteed to return zeros if it overflows.

#ifndef BITBUF_H
#define BITBUF_H

#ifdef _WIN32
#pragma once
#endif


#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "basetypes.h"
#include "tier0/dbg.h"


#if _DEBUG
#define BITBUF_INLINE inline
#else
#define BITBUF_INLINE FORCEINLINE
#endif

//-----------------------------------------------------------------------------
// Forward declarations.
//-----------------------------------------------------------------------------

class Vector;
class QAngle;

//-----------------------------------------------------------------------------
// You can define a handler function that will be called in case of 
// out-of-range values and overruns here.
//
// NOTE: the handler is only called in debug mode.
//
// Call SetBitBufErrorHandler to install a handler.
//-----------------------------------------------------------------------------

typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE=0,		// Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,				// Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;


typedef void (*BitBufErrorHandler)( BitBufErrorType errorType, const char *pDebugName );


#if defined( _DEBUG )
	extern void InternalBitBufErrorHandler( BitBufErrorType errorType, const char *pDebugName );
	#define CallErrorHandler( errorType, pDebugName ) InternalBitBufErrorHandler( errorType, pDebugName );
#else
	#define CallErrorHandler( errorType, pDebugName )
#endif


// Use this to install the error handler. Call with NULL to uninstall your error handler.
void SetBitBufErrorHandler( BitBufErrorHandler fn );


//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------

inline int BitByte( int bits )
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}

//-----------------------------------------------------------------------------
// namespaced helpers
//-----------------------------------------------------------------------------
namespace bitbuf
{
	// ZigZag Transform:  Encodes signed integers so that they can be
	// effectively used with varint encoding.
	//
	// varint operates on unsigned integers, encoding smaller numbers into
	// fewer bytes.  If you try to use it on a signed integer, it will treat
	// this number as a very large unsigned integer, which means that even
	// small signed numbers like -1 will take the maximum number of bytes
	// (10) to encode.  ZigZagEncode() maps signed integers to unsigned
	// in such a way that those with a small absolute value will have smaller
	// encoded values, making them appropriate for encoding using varint.
	//
	//       int32 ->     uint32
	// -------------------------
	//           0 ->          0
	//          -1 ->          1
	//           1 ->          2
	//          -2 ->          3
	//         ... ->        ...
	//  2147483647 -> 4294967294
	// -2147483648 -> 4294967295
	//
	//        >> encode >>
	//        << decode <<

	inline uint32 ZigZagEncode32(int32 n) 
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 31);
	}

	inline int32 ZigZagDecode32(uint32 n) 
	{
		return(n >> 1) ^ -static_cast<int32>(n & 1);
	}

	inline uint64 ZigZagEncode64(int64 n) 
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 63);
	}

	inline int64 ZigZagDecode64(uint64 n) 
	{
		return(n >> 1) ^ -static_cast<int64>(n & 1);
	}

	const int kMaxVarintBytes = 10;
	const int kMaxVarint32Bytes = 5;
}

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------

class bf_write
{
public:
	bf_write();
					
					// nMaxBits can be used as the number of bits in the buffer. 
					// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write( void *pData, int nBytes, int nMaxBits = -1 );
	bf_write( const char *pDebugName, void *pData, int nBytes, int nMaxBits = -1 );

	// Start writing to the specified buffer.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartWriting( void *pData, int nBytes, int iStartBit = 0, int nMaxBits = -1 );

	// Restart buffer writing.
	void			Reset();

	// Get the base pointer.
	unsigned char*	GetBasePointer() { return (unsigned char*) m_pData; }

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow( bool bAssert );

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName();
	void			SetDebugName( const char *pDebugName );


// Seek to a specific position.
public:
	
	void			SeekToBit( int bitPos );


// Bit functions.
public:

	void			WriteOneBit(int nValue);
	void			WriteOneBitNoCheck(int nValue);
	void			WriteOneBitAt( int iBit, int nValue );
	
	// Write signed or unsigned. Range is only checked in debug.
	void			WriteUBitLong( unsigned int data, int numbits, bool bCheckRange=true );
	void			WriteSBitLong( int data, int numbits );
	
	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void			WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool			WriteBits(const void *pIn, int nBits);

	// writes an unsigned integer with variable bit length
	void			WriteUBitVar( unsigned int data );

	// writes a varint encoded integer
	void			WriteVarInt32( uint32 data );
	void			WriteVarInt64( uint64 data );
	void			WriteSignedVarInt32( int32 data );
	void			WriteSignedVarInt64( int64 data );
	int				ByteSizeVarInt32( uint32 data );
	int				ByteSizeVarInt64( uint64 data );
	int				ByteSizeSignedVarInt32( int32 data );
	int				ByteSizeSignedVarInt64( int64 data );

	// Copy the bits straight out of pIn. This seeks pIn forward by nBits.
	// Returns an error if this buffer or the read buffer overflows.
	bool			WriteBitsFromBuffer( class bf_read *pIn, int nBits );
	
	void			WriteBitAngle( float fAngle, int numbits );
	void			WriteBitCoord (const float f);
	void			WriteBitCoordMP( const float f, bool bIntegral, bool bLowPrecision );
	void			WriteBitFloat(float val);
	void			WriteBitVec3Coord( const Vector& fa );
	void			WriteBitNormal( float f );
	void			WriteBitVec3Normal( const Vector& fa );
	void			WriteBitAngles( const QAngle& fa );


// Byte functions.
public:

	void			WriteChar(int val);
	void			WriteByte(int val);
	void			WriteShort(int val);
	void			WriteWord(int val);
	void			WriteLong(long val);
	void			WriteLongLong(int64 val);
	void			WriteFloat(float val);
	bool			WriteBytes( const void *pBuf, int nBytes );

	// Returns false if it overflows the buffer.
	bool			WriteString(const char *pStr);


// Status.
public:

	// How many bytes are filled in?
	int				GetNumBytesWritten() const;
	int				GetNumBitsWritten() const;
	int				GetMaxNumBits();
	int				GetNumBitsLeft();
	int				GetNumBytesLeft();
	unsigned char*	GetData();
	const unsigned char*	GetData() const;

	// Has the buffer overflowed?
	bool			CheckForOverflow(int nBits);
	inline bool		IsOverflowed() const {return m_bOverflow;}

	void			SetOverflowFlag();


public:
	// The current buffer.
	unsigned long* RESTRICT m_pData;
	int				m_nDataBytes;
	int				m_nDataBits;
	
	// Where we are in the buffer.
	int				m_iCurBit;
	
private:

	// Errors?
	bool			m_bOverflow;

	bool			m_bAssertOnOverflow;
	const char		*m_pDebugName;
};


//-----------------------------------------------------------------------------
// Inlined methods
//-----------------------------------------------------------------------------

// How many bytes are filled in?
inline int bf_write::GetNumBytesWritten() const	
{
	return BitByte(m_iCurBit);
}

inline int bf_write::GetNumBitsWritten() const	
{
	return m_iCurBit;
}

inline int bf_write::GetMaxNumBits()		
{
	return m_nDataBits;
}

inline int bf_write::GetNumBitsLeft()	
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_write::GetNumBytesLeft()	
{
	return GetNumBitsLeft() >> 3;
}

inline unsigned char* bf_write::GetData()			
{
	return (unsigned char*) m_pData;
}

inline const unsigned char* bf_write::GetData()	const
{
	return (unsigned char*) m_pData;
}

BITBUF_INLINE bool bf_write::CheckForOverflow(int nBits)
{
	if ( m_iCurBit + nBits > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
	}
	
	return m_bOverflow;
}

BITBUF_INLINE void bf_write::SetOverflowFlag()
{
#ifdef DBGFLAG_ASSERT
	if ( m_bAssertOnOverflow )
	{
		Assert( false );
	}
#endif
	m_bOverflow = true;
}

BITBUF_INLINE void bf_write::WriteOneBitNoCheck(int nValue)
{
#if __i386__
	if(nValue)
		m_pData[m_iCurBit >> 5] |= 1u << (m_iCurBit & 31);
	else
		m_pData[m_iCurBit >> 5] &= ~(1u << (m_iCurBit & 31));
#else
	extern unsigned long g_LittleBits[32];
	if(nValue)
		m_pData[m_iCurBit >> 5] |= g_LittleBits[m_iCurBit & 31];
	else
		m_pData[m_iCurBit >> 5] &= ~g_LittleBits[m_iCurBit & 31];
#endif

	++m_iCurBit;
}

inline void bf_write::WriteOneBit(int nValue)
{
	if( m_iCurBit >= m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return;
	}
	WriteOneBitNoCheck( nValue );
}


inline void	bf_write::WriteOneBitAt( int iBit, int nValue )
{
	if( iBit >= m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return;
	}

#if __i386__
	if(nValue)
		m_pData[iBit >> 5] |= 1u << (iBit & 31);
	else
		m_pData[iBit >> 5] &= ~(1u << (iBit & 31));
#else
	extern unsigned long g_LittleBits[32];
	if(nValue)
		m_pData[iBit >> 5] |= g_LittleBits[iBit & 31];
	else
		m_pData[iBit >> 5] &= ~g_LittleBits[iBit & 31];
#endif
}

BITBUF_INLINE void bf_write::WriteUBitLong( unsigned int curData, int numbits, bool bCheckRange ) RESTRICT
{
#ifdef _DEBUG
	// Make sure it doesn't overflow.
	if ( bCheckRange && numbits < 32 )
	{
		if ( curData >= (unsigned long)(1 << numbits) )
		{
			CallErrorHandler( BITBUFERROR_VALUE_OUT_OF_RANGE, GetDebugName() );
		}
	}
	Assert( numbits >= 0 && numbits <= 32 );
#endif

	if ( GetNumBitsLeft() < numbits )
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return;
	}

	int iCurBitMasked = m_iCurBit & 31;
	int iDWord = m_iCurBit >> 5;
	m_iCurBit += numbits;

	// Mask in a dword.
	Assert( (iDWord*4 + sizeof(long)) <= (unsigned int)m_nDataBytes );
	unsigned long * RESTRICT pOut = &m_pData[iDWord];

	// Rotate data into dword alignment
	curData = (curData << iCurBitMasked) | (curData >> (32 - iCurBitMasked));

	// Calculate bitmasks for first and second word
	unsigned int temp = 1 << (numbits-1);
	unsigned int mask1 = (temp*2-1) << iCurBitMasked;
	unsigned int mask2 = (temp-1) >> (31 - iCurBitMasked);
	
	// Only look beyond current word if necessary (avoid access violation)
	int i = mask2 & 1;
	unsigned long dword1 = LoadLittleDWord( pOut, 0 );
	unsigned long dword2 = LoadLittleDWord( pOut, i );
	
	// Drop bits into place
	dword1 ^= ( mask1 & ( curData ^ dword1 ) );
	dword2 ^= ( mask2 & ( curData ^ dword2 ) );

	// Note reversed order of writes so that dword1 wins if mask2 == 0 && i == 0
	StoreLittleDWord( pOut, i, dword2 );
	StoreLittleDWord( pOut, 0, dword1 );
}

// writes an unsigned integer with variable bit length
BITBUF_INLINE void bf_write::WriteUBitVar( unsigned int data )
{
	/* Reference:
	if ( data < 0x10u )
		WriteUBitLong( 0, 2 ), WriteUBitLong( data, 4 );
	else if ( data < 0x100u )
		WriteUBitLong( 1, 2 ), WriteUBitLong( data, 8 );
	else if ( data < 0x1000u )
		WriteUBitLong( 2, 2 ), WriteUBitLong( data, 12 );
	else
		WriteUBitLong( 3, 2 ), WriteUBitLong( data, 32 );
	*/
	// a < b ? -1 : 0 translates into a CMP, SBB instruction pair
	// with no flow control. should also be branchless on consoles.
	int n = (data < 0x10u ? -1 : 0) + (data < 0x100u ? -1 : 0) + (data < 0x1000u ? -1 : 0);
	WriteUBitLong( data*4 + n + 3, 6 + n*4 + 12 );
	if ( data >= 0x1000u )
	{
		WriteUBitLong( data >> 16, 16 );
	}
}

// write raw IEEE float bits in little endian form
BITBUF_INLINE void bf_write::WriteBitFloat(float val)
{
	long intVal;

	Assert(sizeof(long) == sizeof(float));
	Assert(sizeof(float) == 4);

	intVal = *((long*)&val);
	WriteUBitLong( intVal, 32 );
}

//-----------------------------------------------------------------------------
// This is useful if you just want a buffer to write into on the stack.
//-----------------------------------------------------------------------------

template<int SIZE>
class old_bf_write_static : public bf_write
{
public:
	inline old_bf_write_static() : bf_write(m_StaticData, SIZE) {}

	char	m_StaticData[SIZE];
};



//-----------------------------------------------------------------------------
// Used for unserialization
//-----------------------------------------------------------------------------

class bf_read
{
public:
	bf_read();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_read( const void *pData, int nBytes, int nBits = -1 );
	bf_read( const char *pDebugName, const void *pData, int nBytes, int nBits = -1 );

	// Start reading from the specified buffer.
	// pData's start address must be dword-aligned.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartReading( const void *pData, int nBytes, int iStartBit = 0, int nBits = -1 );

	// Restart buffer reading.
	void			Reset();

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow( bool bAssert );

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName() const { return m_pDebugName; }
	void			SetDebugName( const char *pName );

	void			ExciseBits( int startbit, int bitstoremove );


// Bit functions.
public:
	
	// Returns 0 or 1.
	int				ReadOneBit();


protected:

	unsigned int	CheckReadUBitLong(int numbits);		// For debugging.
	int				ReadOneBitNoCheck();				// Faster version, doesn't check bounds and is inlined.
	bool			CheckForOverflow(int nBits);


public:

	// Get the base pointer.
	const unsigned char*	GetBasePointer() { return m_pData; }

	BITBUF_INLINE int TotalBytesAvailable( void ) const
	{
		return m_nDataBytes;
	}

	// Read a list of bits in.
	void            ReadBits(void *pOut, int nBits);
	// Read a list of bits in, but don't overrun the destination buffer.
	// Returns the number of bits read into the buffer. The remaining
	// bits are skipped over.
	int             ReadBitsClamped_ptr(void *pOut, size_t outSizeBytes, size_t nBits);
	// Helper 'safe' template function that infers the size of the destination
	// array. This version of the function should be preferred.
	// Usage: char databuffer[100];
	//        ReadBitsClamped( dataBuffer, msg->m_nLength );
	template <typename T, size_t N>
	int             ReadBitsClamped( T (&pOut)[N], size_t nBits )
	{
		return ReadBitsClamped_ptr( pOut, N * sizeof(T), nBits );
	}
	
	float			ReadBitAngle( int numbits );

	unsigned int	ReadUBitLong( int numbits ) RESTRICT;
	unsigned int	ReadUBitLongNoInline( int numbits ) RESTRICT;
	unsigned int	PeekUBitLong( int numbits );
	int				ReadSBitLong( int numbits );

	// reads an unsigned integer with variable bit length
	unsigned int	ReadUBitVar();
	unsigned int	ReadUBitVarInternal( int encodingType );

	// reads a varint encoded integer
	uint32			ReadVarInt32();
	uint64			ReadVarInt64();
	int32			ReadSignedVarInt32();
	int64			ReadSignedVarInt64();

	// You can read signed or unsigned data with this, just cast to 
	// a signed int if necessary.
	unsigned int	ReadBitLong(int numbits, bool bSigned);

	float			ReadBitCoord();
	float			ReadBitCoordMP( bool bIntegral, bool bLowPrecision );
	float			ReadBitFloat();
	float			ReadBitNormal();
	void			ReadBitVec3Coord( Vector& fa );
	void			ReadBitVec3Normal( Vector& fa );
	void			ReadBitAngles( QAngle& fa );

	// Faster for comparisons but do not fully decode float values
	unsigned int	ReadBitCoordBits();
	unsigned int	ReadBitCoordMPBits( bool bIntegral, bool bLowPrecision );

// Byte functions (these still read data in bit-by-bit).
public:
	
	BITBUF_INLINE int	ReadChar() { return (char)ReadUBitLong(8); }
	BITBUF_INLINE int	ReadByte() { return ReadUBitLong(8); }
	BITBUF_INLINE int	ReadShort() { return (short)ReadUBitLong(16); }
	BITBUF_INLINE int	ReadWord() { return ReadUBitLong(16); }
	BITBUF_INLINE long ReadLong() { return ReadUBitLong(32); }
	int64			ReadLongLong();
	float			ReadFloat();
	bool			ReadBytes(void *pOut, int nBytes);

	// Returns false if bufLen isn't large enough to hold the
	// string in the buffer.
	//
	// Always reads to the end of the string (so you can read the
	// next piece of data waiting).
	//
	// If bLine is true, it stops when it reaches a '\n' or a null-terminator.
	//
	// pStr is always null-terminated (unless bufLen is 0).
	//
	// pOutNumChars is set to the number of characters left in pStr when the routine is 
	// complete (this will never exceed bufLen-1).
	//
	bool			ReadString( char *pStr, int bufLen, bool bLine=false, int *pOutNumChars=NULL );

	// Reads a string and allocates memory for it. If the string in the buffer
	// is > 2048 bytes, then pOverflow is set to true (if it's not NULL).
	char*			ReadAndAllocateString( bool *pOverflow = 0 );

	// Returns nonzero if any bits differ
	int				CompareBits( bf_read * RESTRICT other, int bits ) RESTRICT;
	int				CompareBitsAt( int offset, bf_read * RESTRICT other, int otherOffset, int bits ) RESTRICT;

// Status.
public:
	int				GetNumBytesLeft();
	int				GetNumBytesRead();
	int				GetNumBitsLeft();
	int				GetNumBitsRead() const;

	// Has the buffer overflowed?
	inline bool		IsOverflowed() const {return m_bOverflow;}

	inline bool		Seek(int iBit);					// Seek to a specific bit.
	inline bool		SeekRelative(int iBitDelta);	// Seek to an offset from the current position.

	// Called when the buffer is overflowed.
	void			SetOverflowFlag();


public:

	// The current buffer.
	const unsigned char* RESTRICT m_pData;
	int						m_nDataBytes;
	int						m_nDataBits;
	
	// Where we are in the buffer.
	int				m_iCurBit;


private:	
	// Errors?
	bool			m_bOverflow;

	// For debugging..
	bool			m_bAssertOnOverflow;

	const char		*m_pDebugName;
};

//-----------------------------------------------------------------------------
// Inlines.
//-----------------------------------------------------------------------------

inline int bf_read::GetNumBytesRead()	
{
	return BitByte(m_iCurBit);
}

inline int bf_read::GetNumBitsLeft()	
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_read::GetNumBytesLeft()	
{
	return GetNumBitsLeft() >> 3;
}

inline int bf_read::GetNumBitsRead() const
{
	return m_iCurBit;
}

inline bool bf_read::Seek(int iBit)
{
	if(iBit < 0 || iBit > m_nDataBits)
	{
		SetOverflowFlag();
		m_iCurBit = m_nDataBits;
		return false;
	}
	else
	{
		m_iCurBit = iBit;
		return true;
	}
}

// Seek to an offset from the current position.
inline bool	bf_read::SeekRelative(int iBitDelta)		
{
	return Seek(m_iCurBit+iBitDelta);
}	

inline bool bf_read::CheckForOverflow(int nBits)
{
	if( m_iCurBit + nBits > m_nDataBits )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
	}

	return m_bOverflow;
}

inline int bf_read::ReadOneBitNoCheck()
{
#if VALVE_LITTLE_ENDIAN
	unsigned int value = ((unsigned long * RESTRICT)m_pData)[m_iCurBit >> 5] >> (m_iCurBit & 31);
#else
	unsigned char value = m_pData[m_iCurBit >> 3] >> (m_iCurBit & 7);
#endif
	++m_iCurBit;
	return value & 1;
}

inline int bf_read::ReadOneBit()
{
	if( GetNumBitsLeft() <= 0 )
	{
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return 0;
	}
	return ReadOneBitNoCheck();
}

inline float bf_read::ReadBitFloat()
{
	union { uint32 u; float f; } c = { ReadUBitLong(32) };
	return c.f;
}

BITBUF_INLINE unsigned int bf_read::ReadUBitVar()
{
	// six bits: low 2 bits for encoding + first 4 bits of value
	unsigned int sixbits = ReadUBitLong(6);
	unsigned int encoding = sixbits & 3;
	if ( encoding )
	{
		// this function will seek back four bits and read the full value
		return ReadUBitVarInternal( encoding );
	}
	return sixbits >> 2;
}

BITBUF_INLINE unsigned int bf_read::ReadUBitLong( int numbits ) RESTRICT
{
	Assert( numbits > 0 && numbits <= 32 );

	if ( GetNumBitsLeft() < numbits )
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler( BITBUFERROR_BUFFER_OVERRUN, GetDebugName() );
		return 0;
	}

	unsigned int iStartBit = m_iCurBit & 31u;
	int iLastBit = m_iCurBit + numbits - 1;
	unsigned int iWordOffset1 = m_iCurBit >> 5;
	unsigned int iWordOffset2 = iLastBit >> 5;
	m_iCurBit += numbits;
	
#if __i386__
	unsigned int bitmask = (2 << (numbits-1)) - 1;
#else
	extern unsigned long g_ExtraMasks[33];
	unsigned int bitmask = g_ExtraMasks[numbits];
#endif

	unsigned int dw1 = LoadLittleDWord( (unsigned long* RESTRICT)m_pData, iWordOffset1 ) >> iStartBit;
	unsigned int dw2 = LoadLittleDWord( (unsigned long* RESTRICT)m_pData, iWordOffset2 ) << (32 - iStartBit);

	return (dw1 | dw2) & bitmask;
}

BITBUF_INLINE int bf_read::CompareBits( bf_read * RESTRICT other, int numbits ) RESTRICT
{
	return (ReadUBitLong(numbits) != other->ReadUBitLong(numbits));
}


#endif



