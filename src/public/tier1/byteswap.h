//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Low level byte swapping routines.
//
// $NoKeywords: $
//=============================================================================
#ifndef BYTESWAP_H
#define BYTESWAP_H
#if defined(_WIN32)
#pragma once
#endif

#include "datamap.h"	// Needed for typedescription_t.  Note datamap.h is tier1 as well.

class CByteswap
{
public:
	CByteswap() 
	{
		// Default behavior sets the target endian to match the machine native endian (no swap).
		SetTargetBigEndian( IsMachineBigEndian() );
	}

	//-----------------------------------------------------------------------------
	// Write a single field.
	//-----------------------------------------------------------------------------
	void SwapFieldToTargetEndian( void* pOutputBuffer, void *pData, typedescription_t *pField );

	//-----------------------------------------------------------------------------
	// Write a block of fields.  Works a bit like the saverestore code.  
	//-----------------------------------------------------------------------------
	void SwapFieldsToTargetEndian( void *pOutputBuffer, void *pBaseData, datamap_t *pDataMap );

	// Swaps fields for the templated type to the output buffer.
	template<typename T> inline void SwapFieldsToTargetEndian( T* pOutputBuffer, void *pBaseData, unsigned int objectCount = 1 )
	{
		for ( unsigned int i = 0; i < objectCount; ++i, ++pOutputBuffer )
		{
			SwapFieldsToTargetEndian( (void*)pOutputBuffer, pBaseData, &T::m_DataMap );
			pBaseData = (byte*)pBaseData + sizeof(T);
		}
	}

	// Swaps fields for the templated type in place.
	template<typename T> inline void SwapFieldsToTargetEndian( T* pOutputBuffer, unsigned int objectCount = 1 )
	{
		SwapFieldsToTargetEndian<T>( pOutputBuffer, (void*)pOutputBuffer, objectCount );
	}

	//-----------------------------------------------------------------------------
	// True if the current machine is detected as big endian. 
	// (Endienness is effectively detected at compile time when optimizations are
	// enabled)
	//-----------------------------------------------------------------------------
	static bool IsMachineBigEndian()
	{
		short nIsBigEndian = 1;

		// if we are big endian, the first byte will be a 0, if little endian, it will be a one.
		return (bool)(0 ==  *(char *)&nIsBigEndian );
	}

	//-----------------------------------------------------------------------------
	// Sets the target byte ordering we are swapping to or from.
	//
	// Braindead Endian Reference:
	//		x86 is LITTLE Endian
	//		PowerPC is BIG Endian
	//-----------------------------------------------------------------------------
	inline void SetTargetBigEndian( bool bigEndian )
	{
		m_bBigEndian = bigEndian;
		m_bSwapBytes = IsMachineBigEndian() != bigEndian;
	}

	// Changes target endian
	inline void FlipTargetEndian( void )	
	{
		m_bSwapBytes = !m_bSwapBytes;
		m_bBigEndian = !m_bBigEndian;
	}

	// Forces byte swapping state, regardless of endianess
	inline void ActivateByteSwapping( bool bActivate )	
	{
		SetTargetBigEndian( IsMachineBigEndian() != bActivate );
	}

	//-----------------------------------------------------------------------------
	// Returns true if the target machine is the same as this one in endianness.
	//
	// Used to determine when a byteswap needs to take place.
	//-----------------------------------------------------------------------------
	inline bool IsSwappingBytes( void )	// Are bytes being swapped?
	{
		return m_bSwapBytes;
	}

	inline bool IsTargetBigEndian( void )	// What is the current target endian?
	{
		return m_bBigEndian;
	}

	//-----------------------------------------------------------------------------
	// IsByteSwapped()
	//
	// When supplied with a chunk of input data and a constant or magic number
	// (in native format) determines the endienness of the current machine in
	// relation to the given input data.
	//
	// Returns:
	//		1  if input is the same as nativeConstant.
	//		0  if input is byteswapped relative to nativeConstant.
	//		-1 if input is not the same as nativeConstant and not byteswapped either.
	//
	// ( This is useful for detecting byteswapping in magic numbers in structure 
	// headers for example. )
	//-----------------------------------------------------------------------------
	template<typename T> inline int SourceIsNativeEndian( T input, T nativeConstant )
	{
		// If it's the same, it isn't byteswapped:
		if( input == nativeConstant )
			return 1;

		int output;
		LowLevelByteSwap<T>( &output, &input );
		if( output == nativeConstant )
			return 0;

		assert( 0 );		// if we get here, input is neither a swapped nor unswapped version of nativeConstant.
		return -1;
	}

	//-----------------------------------------------------------------------------
	// Swaps an input buffer full of type T into the given output buffer.
	//
	// Swaps [count] items from the inputBuffer to the outputBuffer.
	// If inputBuffer is omitted or NULL, then it is assumed to be the same as
	// outputBuffer - effectively swapping the contents of the buffer in place.
	//-----------------------------------------------------------------------------
	template<typename T> inline void SwapBuffer( T* outputBuffer, T* inputBuffer = NULL, int count = 1 )
	{
		assert( count >= 0 );
		assert( outputBuffer );

		// Fail gracefully in release:
		if( count <=0 || !outputBuffer )
			return;

		// Optimization for the case when we are swapping in place.
		if( inputBuffer == NULL )
		{
			inputBuffer = outputBuffer;
		}

		// Swap everything in the buffer:
		for( int i = 0; i < count; i++ )
		{
			LowLevelByteSwap<T>( &outputBuffer[i], &inputBuffer[i] );
		}
	}

	//-----------------------------------------------------------------------------
	// Swaps an input buffer full of type T into the given output buffer.
	//
	// Swaps [count] items from the inputBuffer to the outputBuffer.
	// If inputBuffer is omitted or NULL, then it is assumed to be the same as
	// outputBuffer - effectively swapping the contents of the buffer in place.
	//-----------------------------------------------------------------------------
	template<typename T> inline void SwapBufferToTargetEndian( T* outputBuffer, T* inputBuffer = NULL, int count = 1 )
	{
		assert( count >= 0 );
		assert( outputBuffer );

		// Fail gracefully in release:
		if( count <=0 || !outputBuffer )
			return;

		// Optimization for the case when we are swapping in place.
		if( inputBuffer == NULL )
		{
			inputBuffer = outputBuffer;
		}

		// Are we already the correct endienness? ( or are we swapping 1 byte items? )
		if( !m_bSwapBytes || ( sizeof(T) == 1 ) )
		{
			// If we were just going to swap in place then return.
			if( !inputBuffer )
				return;
		
			// Otherwise copy the inputBuffer to the outputBuffer:
			memcpy( outputBuffer, inputBuffer, count * sizeof( T ) );
			return;

		}

		// Swap everything in the buffer:
		for( int i = 0; i < count; i++ )
		{
			LowLevelByteSwap<T>( &outputBuffer[i], &inputBuffer[i] );
		}
	}

private:
	//-----------------------------------------------------------------------------
	// The lowest level byte swapping workhorse of doom.  output always contains the 
	// swapped version of input.  ( Doesn't compare machine to target endianness )
	//-----------------------------------------------------------------------------
	template<typename T> static void LowLevelByteSwap( T *output, const T *input )
	{
		T temp = *output;
#if defined( _X360 )
		// Intrinsics need the source type to be fixed-point
		DWORD* word = (DWORD*)input;
		switch( sizeof(T) )
		{
		case 8:
			{
			__storewordbytereverse( *word, 0, &temp );
			__storewordbytereverse( *(word+1), 4, &temp );
			}
			break;

		case 4:
			__storewordbytereverse( *word, 0, &temp );
			break;

		case 2:
			__storeshortbytereverse( *input, 0, &temp );
			break;

		default:
			Assert( "Invalid size in CByteswap::LowLevelByteSwap" && 0 );
		}
#else
		for( auto i = 0; i < sizeof(T); i++ )
		{
			unsigned char *pByteOut = (unsigned char *) &temp;
			const unsigned char *pByteIn = (const unsigned char *) input;
			pByteOut[i] = pByteIn[ sizeof( T ) - ( i + 1 ) ]; 
		}
#endif
		Q_memcpy( output, &temp, sizeof(T) );
	}

	unsigned int m_bSwapBytes : 1;
	unsigned int m_bBigEndian : 1;
};

#endif /* !BYTESWAP_H */
