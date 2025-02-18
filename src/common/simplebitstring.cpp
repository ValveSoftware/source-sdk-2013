
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
//*****************************************************************************
//
// Contents:
//
//		CSimpleBitString
//
// Authors:	chrisn
//
// Target restrictions:
//
// Tool restrictions:
//
// Things to do:
//
//*****************************************************************************


//*****************************************************************************
//
// Include files required to build and use this module.
//
//*****************************************************************************

// Precompiled header (must come first - includes project common headers)
//#include "stdafx.h"
#include "tier0/platform.h"
#include "tier1/utlvector.h"
#include "simplebitstring.h"



//*****************************************************************************
//
// Class definitions:
//
//*****************************************************************************

//
// class CSimpleBitString::iterator
//


//-----------------------------------------------------------------------------
// 
// Function:	
// 
//-----------------------------------------------------------------------------
void CSimpleBitString::AppendBits( uint64 u64Data, uint32 NumSignificantLowBitsOfData )
{
	Assert
		( 
		NumSignificantLowBitsOfData <= 64
		);

	while ( NumSignificantLowBitsOfData > 0 )
	{
		// Clear top bits of data
		if ( NumSignificantLowBitsOfData < 64 )
			u64Data &= ( (1ULL << NumSignificantLowBitsOfData) - 1 );	// will fail for 64 bits

		uint32 Idx					= m_uNumBits / 8;
		uint32 NumUsedBitsInLastByte	= m_uNumBits % 8;

		uint32 NumAvailableBitsInLastByte = 8 - NumUsedBitsInLastByte;

		uint32 NumBitsToPutInThisByte 
				= min( NumAvailableBitsInLastByte, NumSignificantLowBitsOfData );

		uint8 BitsForThisByte
				=	( u64Data >> (NumSignificantLowBitsOfData - NumBitsToPutInThisByte) )
						& ( (1ULL << NumBitsToPutInThisByte) - 1 );

		m_vecU8[Idx] |= ( BitsForThisByte 
							<< ( NumAvailableBitsInLastByte - NumBitsToPutInThisByte ) );

		m_uNumBits += NumBitsToPutInThisByte;

		NumAvailableBitsInLastByte -= NumBitsToPutInThisByte;
		if ( NumAvailableBitsInLastByte == 0 )
		{
			m_vecU8[ m_vecU8.AddToTail() ] = 0x00;
		}
		
		// We've used the top N bits of data
		NumSignificantLowBitsOfData -= NumBitsToPutInThisByte;
	}
}


//-----------------------------------------------------------------------------
// 
// Function:	
// 
//-----------------------------------------------------------------------------
void CSimpleBitString::AppendBits( const uint8 * pData, uint32 NumBitsOfData )
{
	Assert( pData );

	uint32 NumBytes = NumBitsOfData / 8;
	for ( uint32 i = 0; i < NumBytes; ++i )
	{
		AppendBits( *(pData++), 8 );
	}
	uint32 NumTailBits = NumBitsOfData % 8;
	AppendBits( (*pData) >> (8U - NumTailBits), NumTailBits );
}



//-----------------------------------------------------------------------------
// 
// Function:	
// 
//-----------------------------------------------------------------------------
void CSimpleBitString::ReversiblyObfusticateBitsFromStart( uint NumBits, const uint8 * pObfusticationData, size_t uSizeOfObfusticationData )
{
	Assert( pObfusticationData );

	if	(	NumBits > m_uNumBits
		||	NumBits > uSizeOfObfusticationData * 8
		)
	{
		AssertMsg( false, "ReversiblyObfusticateBitsFromStart(): Bad NumBits" );
		return; // bugbug taylor bool return
	}

	uint8 * pBits = & m_vecU8[0];

	uint NumBytes = NumBits / 8;
	for ( uint i = 0; i < NumBytes; ++i )
	{
		*(pBits++) ^= *(pObfusticationData++);
	}
	uint NumTailBits = NumBits % 8;
	if ( NumTailBits > 0 )
	{
		*pBits ^= ( *(pObfusticationData++) & (((1U << NumTailBits) - 1) << (8U - NumTailBits) ) );
	}
}


//-----------------------------------------------------------------------------
// 
// Function:	
// 
//-----------------------------------------------------------------------------
uint8 CSimpleBitString::GetByteChecksumFromStart( uint NumBits ) const
{
	if ( NumBits > m_uNumBits )
	{
		AssertMsg( false, "GenerateByteChecksumFromStart(): Bad NumBits" );
		return 0;
	}

	uint8 u8Checksum = 0;
	const uint8 * pBits = & m_vecU8[0];

	uint NumBytes = NumBits / 8;
	for ( uint i = 0; i < NumBytes; ++i )
	{
		u8Checksum += *(pBits++);
	}
	uint NumTailBits = NumBits % 8;
	if ( NumTailBits > 0 )
	{
		u8Checksum += ( *(pBits) & (((1U << NumTailBits) - 1) << (8U - NumTailBits) ) );
	}

	return u8Checksum;
}



//
// class CSimpleBitString::iterator
//
uint32 CSimpleBitString::iterator::GetNextBits( uint32 NumBitsToGet )
{
	Assert
		( 
		NumBitsToGet <= 32
		);

	return static_cast<uint32>( GetNextBits64( NumBitsToGet ) );
}

uint64 CSimpleBitString::iterator::GetNextBits64( uint32 NumBitsToGet )
{
	Assert
		( 
		NumBitsToGet <= 64
		);

	if ( m_uNextBitIdx + NumBitsToGet > m_rSimpleBitString.m_uNumBits )
	{
		AssertMsg( false, "Not enough bits in CSimpleBitString" );
		return 0;
	}

	uint64	u64Data = 0;

	while ( NumBitsToGet > 0 )
	{
		uint32 Idx						= m_uNextBitIdx / 8;
		Assert( Idx < (uint32)m_rSimpleBitString.m_vecU8.Count() );

		uint32 NumConsumedBitsInThisByte	= m_uNextBitIdx % 8;
		uint32 NumAvailableBitsInThisByte = 8 - NumConsumedBitsInThisByte;

		uint32 NumBitsToGetFromThisByte 
				= min( NumAvailableBitsInThisByte, NumBitsToGet );

		uint8 BitsFromThisByte
				=	( m_rSimpleBitString.m_vecU8[Idx] >> (NumAvailableBitsInThisByte - NumBitsToGetFromThisByte) )
						& ( (1UL << NumBitsToGetFromThisByte) - 1 );

		u64Data <<= NumBitsToGetFromThisByte;
		u64Data |= BitsFromThisByte;

		m_uNextBitIdx	+= NumBitsToGetFromThisByte;
		NumBitsToGet	-= NumBitsToGetFromThisByte;
	}

	return u64Data;
}

