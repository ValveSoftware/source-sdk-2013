
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
// Authors:
//
// Target restrictions:
//
// Tool restrictions:
//
// Things to do:
//
//		
//
//*****************************************************************************

#ifndef INCLUDED_COMMON_SIMPLEBITSTRING_H
#define INCLUDED_COMMON_SIMPLEBITSTRING_H

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif


//*****************************************************************************
//
// Include files required by this header.
//
// Note: Do NOT place any 'using' directives or declarations in header files - 
// put them at the top of the source files that require them.  
// Use fully-qualified names in header files. 
//
//*****************************************************************************

// All precompiled headers (Stable.h) include this first to give use a common 
// place for including order-dependent library headers (things that need to go first).





class CSimpleBitString
{
public:
	explicit CSimpleBitString( uint32 ReserveNumBits = 0 )
		:
		m_uNumBits( 0 ),
		m_vecU8()
	{
		m_vecU8.EnsureCapacity( (ReserveNumBits / 8) + 1 );
		m_vecU8[ m_vecU8.AddToTail() ] = 0x00;	// always need 1 byte
	}

	void clear()
	{
		m_uNumBits = 0;
		m_vecU8.RemoveAll();
                m_vecU8[ m_vecU8.AddToTail() ] = 0x00;  // always need 1 byte
	}
		
	void AppendBits( uint64 data, uint32 NumSignificantLowBitsOfData );
	void AppendBits( const uint8 * pData, uint32 NumBitsOfData );

	void ReversiblyObfusticateBitsFromStart( uint32 NumBits, const uint8 * pObfusticationData, size_t uSizeOfObfusticationData );
	uint8	 GetByteChecksumFromStart( uint32 NumBits ) const;

	uint GetCurrNumBits() const
	{
		return m_uNumBits;
	}

	const uint8 * data() const
	{
		return & m_vecU8[0];
	}

	size_t size() const
	{
		return m_vecU8.Count();
	}

private:
	uint32				m_uNumBits;
	CUtlVector<uint8>	m_vecU8;

public:

	// Iterator class for retrieving bits
	class iterator
	{
	public:
		explicit iterator( const CSimpleBitString & bs )
			:
			m_rSimpleBitString( bs ),
			m_uNextBitIdx( 0 )
		{
		}

		void ResetToStart()
		{
			m_uNextBitIdx = 0;
		}

		uint32 GetNumConsumedBits() const
		{
			return m_uNextBitIdx;
		}

		uint32 GetNumRemainingBits() const
		{
			return m_rSimpleBitString.m_uNumBits - m_uNextBitIdx;
		}

		uint32 GetNextBits( uint32 NumBitsToGet );
		uint64 GetNextBits64( uint32 NumBitsToGet );

		void SkipNextBits( uint32 NumBitsToSkip )
		{
			if ( m_uNextBitIdx + NumBitsToSkip > m_rSimpleBitString.m_uNumBits )
			{
				AssertMsg( false, "Not enough bits in CSimpleBitString" );
				NumBitsToSkip = 0;
			}

			m_uNextBitIdx += NumBitsToSkip;
		}

		bool operator ==( const iterator & other ) const
		{
			return		(& m_rSimpleBitString == & other.m_rSimpleBitString)
					&&	m_uNextBitIdx == other.m_uNextBitIdx;
		}

		void	DoAssertClassInvariant() const;

	private:
		const CSimpleBitString &	m_rSimpleBitString;	//lint !e1725 reference
		uint32						m_uNextBitIdx;
	};

	friend class iterator;

};





#endif
