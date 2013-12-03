//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef BITVEC_H
#define BITVEC_H
#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include "tier0/dbg.h"
#include "tier0/basetypes.h"


class CBitVecAccessor
{
public:
				CBitVecAccessor(uint32 *pDWords, int iBit);

	void		operator=(int val);
				operator uint32();

private:
	uint32 *m_pDWords;
	int	m_iBit;
};
	

//-----------------------------------------------------------------------------
// Support functions
//-----------------------------------------------------------------------------

#define LOG2_BITS_PER_INT	5
#define BITS_PER_INT		32

#if _WIN32 && !defined(_X360)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

inline int FirstBitInWord( unsigned int elem, int offset )
{
#if _WIN32
	if ( !elem )
		return -1;
#if defined( _X360 )
	// this implements CountTrailingZeros() / BitScanForward()
	unsigned int mask = elem-1;
	unsigned int comp = ~elem;
	elem = mask & comp;
	return (32 - _CountLeadingZeros(elem)) + offset;
#else
	unsigned long out;
	_BitScanForward(&out, elem);
	return out + offset;
#endif

#else
	static unsigned firstBitLUT[256] = 
	{
		0,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,6,0,1,0,2,0,1,0,
		3,0,1,0,2,0,1,0,4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
		4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
	};
	unsigned elemByte;

	elemByte = (elem & 0xFF);
	if ( elemByte )
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if ( elemByte )
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if ( elemByte )
		return offset + firstBitLUT[elemByte];

	elem >>= 8;
	offset += 8;
	elemByte = (elem & 0xFF);
	if ( elemByte )
		return offset + firstBitLUT[elemByte];

	return -1;
#endif
}

//-------------------------------------

inline unsigned GetEndMask( int numBits ) 
{ 
	static unsigned bitStringEndMasks[] = 
	{
		0xffffffff,
		0x00000001,
		0x00000003,
		0x00000007,
		0x0000000f,
		0x0000001f,
		0x0000003f,
		0x0000007f,
		0x000000ff,
		0x000001ff,
		0x000003ff,
		0x000007ff,
		0x00000fff,
		0x00001fff,
		0x00003fff,
		0x00007fff,
		0x0000ffff,
		0x0001ffff,
		0x0003ffff,
		0x0007ffff,
		0x000fffff,
		0x001fffff,
		0x003fffff,
		0x007fffff,
		0x00ffffff,
		0x01ffffff,
		0x03ffffff,
		0x07ffffff,
		0x0fffffff,
		0x1fffffff,
		0x3fffffff,
		0x7fffffff,
	};

	return bitStringEndMasks[numBits % BITS_PER_INT]; 
}


inline int GetBitForBitnum( int bitNum ) 
{ 
	static int bitsForBitnum[] = 
	{
		( 1 << 0 ),
		( 1 << 1 ),
		( 1 << 2 ),
		( 1 << 3 ),
		( 1 << 4 ),
		( 1 << 5 ),
		( 1 << 6 ),
		( 1 << 7 ),
		( 1 << 8 ),
		( 1 << 9 ),
		( 1 << 10 ),
		( 1 << 11 ),
		( 1 << 12 ),
		( 1 << 13 ),
		( 1 << 14 ),
		( 1 << 15 ),
		( 1 << 16 ),
		( 1 << 17 ),
		( 1 << 18 ),
		( 1 << 19 ),
		( 1 << 20 ),
		( 1 << 21 ),
		( 1 << 22 ),
		( 1 << 23 ),
		( 1 << 24 ),
		( 1 << 25 ),
		( 1 << 26 ),
		( 1 << 27 ),
		( 1 << 28 ),
		( 1 << 29 ),
		( 1 << 30 ),
		( 1 << 31 ),
	};

	return bitsForBitnum[ (bitNum) & (BITS_PER_INT-1) ]; 
}

inline int GetBitForBitnumByte( int bitNum ) 
{ 
	static int bitsForBitnum[] = 
	{
		( 1 << 0 ),
		( 1 << 1 ),
		( 1 << 2 ),
		( 1 << 3 ),
		( 1 << 4 ),
		( 1 << 5 ),
		( 1 << 6 ),
		( 1 << 7 ),
	};

	return bitsForBitnum[ bitNum & 7 ]; 
}

inline int CalcNumIntsForBits( int numBits )	{ return (numBits + (BITS_PER_INT-1)) / BITS_PER_INT; }

#ifdef _X360
#define BitVec_Bit( bitNum ) GetBitForBitnum( bitNum )
#define BitVec_BitInByte( bitNum ) GetBitForBitnumByte( bitNum )
#else
#define BitVec_Bit( bitNum ) ( 1 << ( (bitNum) & (BITS_PER_INT-1) ) )
#define BitVec_BitInByte( bitNum ) ( 1 << ( (bitNum) & 7 ) )
#endif
#define BitVec_Int( bitNum ) ( (bitNum) >> LOG2_BITS_PER_INT )


//-----------------------------------------------------------------------------
// template CBitVecT
//
// Defines the operations relevant to any bit array. Simply requires a base
// class that implements GetNumBits(), Base(), GetNumDWords() & ValidateOperand()
//
// CVarBitVec and CBitVec<int> are the actual classes generally used
// by clients
//

template <class BASE_OPS>
class CBitVecT : public BASE_OPS
{
public:
	CBitVecT();
	CBitVecT(int numBits);			// Must be initialized with the number of bits

	void	Init(int val = 0);

	// Access the bits like an array.
	CBitVecAccessor	operator[](int i);

	// Do NOT override bitwise operators (see note in header)
	void	And(const CBitVecT &andStr, CBitVecT *out) const;
	void	Or(const CBitVecT &orStr, CBitVecT *out) const;
	void	Xor(const CBitVecT &orStr, CBitVecT *out) const;
	
	void	Not(CBitVecT *out) const;
	
	void	CopyTo(CBitVecT *out) const;
	void	Copy( const CBitVecT<BASE_OPS> &other, int nBits=-1 );
	bool	Compare( const CBitVecT<BASE_OPS> &other, int nBits=-1 ) const;

	bool	IsAllClear(void) const;		// Are all bits zero?
	bool	IsAllSet(void) const;		// Are all bits one?

	uint32	Get( uint32 bitNum ) const;
	bool 	IsBitSet( int bitNum ) const;
	void 	Set( int bitNum );
	void 	Set( int bitNum, bool bNewVal );
	void 	Clear(int bitNum);

	bool	TestAndSet(int bitNum);

	void 	Set( uint32 offset, uint32 mask );
	void 	Clear( uint32 offset, uint32 mask );
	uint32	Get( uint32 offset, uint32 mask );

	void	SetAll(void);			// Sets all bits
	void	ClearAll(void);			// Clears all bits

	uint32	GetDWord(int i) const;
	void	SetDWord(int i, uint32 val);

	CBitVecT<BASE_OPS>&	operator=(const CBitVecT<BASE_OPS> &other)	{ other.CopyTo( this ); return *this; }
	bool			operator==(const CBitVecT<BASE_OPS> &other)		{ return Compare( other ); }
	bool			operator!=(const CBitVecT<BASE_OPS> &other)		{ return !operator==( other ); }

	static void GetOffsetMaskForBit( uint32 bitNum, uint32 *pOffset, uint32 *pMask )	{ *pOffset = BitVec_Int( bitNum ); *pMask = BitVec_Bit( bitNum ); }
};

//-----------------------------------------------------------------------------
// class CVarBitVecBase
//
// Defines the operations necessary for a variable sized bit array
template <typename BITCOUNTTYPE>
class CVarBitVecBase
{
public:
	bool	IsFixedSize() const			{ return false; }
	int		GetNumBits(void) const		{ return m_numBits; }
	void	Resize( int numBits, bool bClearAll = false );		// resizes bit array
	
	int 	GetNumDWords() const		{ return m_numInts; }
	uint32 *Base()						{ return m_pInt;	}
	const uint32 *Base() const			{ return m_pInt;	}

	void Attach( uint32 *pBits, int numBits );
	bool Detach( uint32 **ppBits, int *pNumBits );

	int		FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CVarBitVecBase();
	CVarBitVecBase(int numBits);
	CVarBitVecBase( const CVarBitVecBase<BITCOUNTTYPE> &from );
	CVarBitVecBase &operator=( const CVarBitVecBase<BITCOUNTTYPE> &from );
	~CVarBitVecBase(void);
	
	void 		ValidateOperand( const CVarBitVecBase<BITCOUNTTYPE> &operand ) const	{ Assert(GetNumBits() == operand.GetNumBits()); }

	unsigned	GetEndMask() const		{ return ::GetEndMask( GetNumBits() ); }

private:

	BITCOUNTTYPE	m_numBits;					// Number of bits in the bitstring
	BITCOUNTTYPE	m_numInts;					// Number of ints to needed to store bitstring
	uint32			m_iBitStringStorage;		// If the bit string fits in one int, it goes here
	uint32 *		m_pInt;					// Array of ints containing the bitstring

	void	AllocInts( int numInts );	// Free the allocated bits
	void	ReallocInts( int numInts );
	void	FreeInts( void );			// Free the allocated bits
};

//-----------------------------------------------------------------------------
// class CFixedBitVecBase
//
// Defines the operations necessary for a fixed sized bit array. 
// 

template <int bits> struct BitCountToEndMask_t { };
template <> struct BitCountToEndMask_t< 0> { enum { MASK = 0xffffffff }; };
template <> struct BitCountToEndMask_t< 1> { enum { MASK = 0x00000001 }; };
template <> struct BitCountToEndMask_t< 2> { enum { MASK = 0x00000003 }; };
template <> struct BitCountToEndMask_t< 3> { enum { MASK = 0x00000007 }; };
template <> struct BitCountToEndMask_t< 4> { enum { MASK = 0x0000000f }; };
template <> struct BitCountToEndMask_t< 5> { enum { MASK = 0x0000001f }; };
template <> struct BitCountToEndMask_t< 6> { enum { MASK = 0x0000003f }; };
template <> struct BitCountToEndMask_t< 7> { enum { MASK = 0x0000007f }; };
template <> struct BitCountToEndMask_t< 8> { enum { MASK = 0x000000ff }; };
template <> struct BitCountToEndMask_t< 9> { enum { MASK = 0x000001ff }; };
template <> struct BitCountToEndMask_t<10> { enum { MASK = 0x000003ff }; };
template <> struct BitCountToEndMask_t<11> { enum { MASK = 0x000007ff }; };
template <> struct BitCountToEndMask_t<12> { enum { MASK = 0x00000fff }; };
template <> struct BitCountToEndMask_t<13> { enum { MASK = 0x00001fff }; };
template <> struct BitCountToEndMask_t<14> { enum { MASK = 0x00003fff }; };
template <> struct BitCountToEndMask_t<15> { enum { MASK = 0x00007fff }; };
template <> struct BitCountToEndMask_t<16> { enum { MASK = 0x0000ffff }; };
template <> struct BitCountToEndMask_t<17> { enum { MASK = 0x0001ffff }; };
template <> struct BitCountToEndMask_t<18> { enum { MASK = 0x0003ffff }; };
template <> struct BitCountToEndMask_t<19> { enum { MASK = 0x0007ffff }; };
template <> struct BitCountToEndMask_t<20> { enum { MASK = 0x000fffff }; };
template <> struct BitCountToEndMask_t<21> { enum { MASK = 0x001fffff }; };
template <> struct BitCountToEndMask_t<22> { enum { MASK = 0x003fffff }; };
template <> struct BitCountToEndMask_t<23> { enum { MASK = 0x007fffff }; };
template <> struct BitCountToEndMask_t<24> { enum { MASK = 0x00ffffff }; };
template <> struct BitCountToEndMask_t<25> { enum { MASK = 0x01ffffff }; };
template <> struct BitCountToEndMask_t<26> { enum { MASK = 0x03ffffff }; };
template <> struct BitCountToEndMask_t<27> { enum { MASK = 0x07ffffff }; };
template <> struct BitCountToEndMask_t<28> { enum { MASK = 0x0fffffff }; };
template <> struct BitCountToEndMask_t<29> { enum { MASK = 0x1fffffff }; };
template <> struct BitCountToEndMask_t<30> { enum { MASK = 0x3fffffff }; };
template <> struct BitCountToEndMask_t<31> { enum { MASK = 0x7fffffff }; };

//-------------------------------------

template <int NUM_BITS>
class CFixedBitVecBase
{
public:
	bool	IsFixedSize() const								{ return true; }
	int		GetNumBits(void) const							{ return NUM_BITS; }
	void	Resize( int numBits, bool bClearAll = false )	{ Assert(numBits == NUM_BITS); if ( bClearAll ) Plat_FastMemset( m_Ints, 0, NUM_INTS * sizeof(uint32) ); }// for syntatic consistency (for when using templates)
	
	int 			GetNumDWords() const					{ return NUM_INTS; }
	uint32 *		Base()									{ return m_Ints;	}
	const uint32 *	Base() const							{ return m_Ints;	}

	int		FindNextSetBit(int iStartBit) const; // returns -1 if no set bit was found

protected:
	CFixedBitVecBase()				{}
	CFixedBitVecBase(int numBits)	{ Assert( numBits == NUM_BITS ); } // doesn't make sense, really. Supported to simplify templates & allow easy replacement of variable 
	
	void 		ValidateOperand( const CFixedBitVecBase<NUM_BITS> &operand ) const	{ } // no need, compiler does so statically

public: // for test code
	unsigned	GetEndMask() const		{ return static_cast<unsigned>( BitCountToEndMask_t<NUM_BITS % BITS_PER_INT>::MASK ); }

private:
	enum
	{
		NUM_INTS = (NUM_BITS + (BITS_PER_INT-1)) / BITS_PER_INT
	};

	uint32 m_Ints[(NUM_BITS + (BITS_PER_INT-1)) / BITS_PER_INT];
};

//-----------------------------------------------------------------------------
//
// The actual classes used
//

// inheritance instead of typedef to allow forward declarations
class CVarBitVec : public CBitVecT< CVarBitVecBase<unsigned short> >
{
public:
	CVarBitVec()
	{
	}
	
	CVarBitVec(int numBits)
	 : CBitVecT< CVarBitVecBase<unsigned short> >(numBits)
	{
	}
};

class CLargeVarBitVec : public CBitVecT< CVarBitVecBase<int> >
{
public:
	CLargeVarBitVec()
	{
	}

	CLargeVarBitVec(int numBits)
		: CBitVecT< CVarBitVecBase<int> >(numBits)
	{
	}
};

//-----------------------------------------------------------------------------

template < int NUM_BITS >
class CBitVec : public CBitVecT< CFixedBitVecBase<NUM_BITS> >
{
public:
	CBitVec()
	{
	}
	
	CBitVec(int numBits)
	 : CBitVecT< CFixedBitVecBase<NUM_BITS> >(numBits)
	{
	}
};


//-----------------------------------------------------------------------------

typedef CBitVec<32> CDWordBitVec;

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase()
{
	Plat_FastMemset( this, 0, sizeof( *this ) );
}

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase(int numBits)
{
	Assert( numBits );
	m_numBits	= numBits;

	// Figure out how many ints are needed
	m_numInts = CalcNumIntsForBits( numBits );
	m_pInt = NULL;
	AllocInts( m_numInts );
}

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::CVarBitVecBase( const CVarBitVecBase<BITCOUNTTYPE> &from )
{
	if ( from.m_numInts )
	{
		m_numBits = from.m_numBits;
		m_numInts = from.m_numInts;
		m_pInt = NULL;
		AllocInts( m_numInts );
		memcpy( m_pInt, from.m_pInt, m_numInts * sizeof(int) );
	}
	else
		memset( this, 0, sizeof( *this ) );
}

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE> &CVarBitVecBase<BITCOUNTTYPE>::operator=( const CVarBitVecBase<BITCOUNTTYPE> &from )
{
	Resize( from.GetNumBits() );
	if ( m_pInt )
		memcpy( m_pInt, from.m_pInt, m_numInts * sizeof(int) );
	return (*this);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline CVarBitVecBase<BITCOUNTTYPE>::~CVarBitVecBase(void)
{
	FreeInts();
}

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::Attach( uint32 *pBits, int numBits )
{
	FreeInts();
	m_numBits = numBits;
	m_numInts = CalcNumIntsForBits( numBits );
	if ( m_numInts > 1 )
	{
		m_pInt = pBits;
	}
	else
	{
		m_iBitStringStorage = *pBits;
		m_pInt = &m_iBitStringStorage;
		free( pBits );
	}
}

//-----------------------------------------------------------------------------

template <typename BITCOUNTTYPE>
inline bool CVarBitVecBase<BITCOUNTTYPE>::Detach( uint32 **ppBits, int *pNumBits )
{
	if ( !m_numBits )
	{
		return false;
	}

	*pNumBits = m_numBits;
	if ( m_numInts > 1 )
	{
		*ppBits = m_pInt;
	}
	else
	{
		*ppBits = (uint32 *)malloc( sizeof(uint32) );
		**ppBits = m_iBitStringStorage;
		free( m_pInt );
	}

	memset( this, 0, sizeof( *this ) );
	return true;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT()
{
	// undef this is ints are not 4 bytes
	// generate a compile error if sizeof(int) is not 4 (HACK: can't use the preprocessor so use the compiler)
	
	COMPILE_TIME_ASSERT( sizeof(int)==4 );
	
	// Initialize bitstring by clearing all bits
	ClearAll();
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline CBitVecT<BASE_OPS>::CBitVecT(int numBits)
 : BASE_OPS( numBits )
{
	// undef this is ints are not 4 bytes
	// generate a compile error if sizeof(int) is not 4 (HACK: can't use the preprocessor so use the compiler)
	
	COMPILE_TIME_ASSERT( sizeof(int)==4 );
	
	// Initialize bitstring by clearing all bits
	ClearAll();
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline CBitVecAccessor CBitVecT<BASE_OPS>::operator[](int i)	
{
	Assert(i >= 0 && i < this->GetNumBits());
	return CBitVecAccessor(this->Base(), i);
}


//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Init( int val )
{
	if ( this->Base() )
		Plat_FastMemset( this->Base(), ( val ) ? 0xff : 0, this->GetNumDWords() * sizeof(int) );
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline uint32 CBitVecT<BASE_OPS>::Get( uint32 bitNum ) const
{
	Assert( bitNum < (uint32)this->GetNumBits() );
	const uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	return ( *pInt & BitVec_Bit( bitNum ) );
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsBitSet( int bitNum ) const
{
	Assert( bitNum >= 0 && bitNum < this->GetNumBits() );
	const uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	return ( ( *pInt & BitVec_Bit( bitNum ) ) != 0 );
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set( int bitNum )			
{
	Assert( bitNum >= 0 && bitNum < this->GetNumBits() );
	uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	*pInt |= BitVec_Bit( bitNum );
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::TestAndSet(int bitNum)
{
	Assert( bitNum >= 0 && bitNum < this->GetNumBits() );
	uint32 bitVecBit = BitVec_Bit( bitNum );
	uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	bool bResult = ( ( *pInt & bitVecBit) != 0 );
	*pInt |= bitVecBit;
	return bResult;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear(int bitNum)		
{
	Assert( bitNum >= 0 && bitNum < this->GetNumBits() );
	uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	*pInt &= ~BitVec_Bit( bitNum );
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set( int bitNum, bool bNewVal )
{
	uint32 *pInt = this->Base() + BitVec_Int( bitNum );
	uint32 bitMask = BitVec_Bit( bitNum );
	if ( bNewVal )
	{
		*pInt |= bitMask;
	}
	else
	{
		*pInt &= ~bitMask;
	}
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Set( uint32 offset, uint32 mask )
{
	uint32 *pInt = this->Base() + offset;
	*pInt |= mask;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Clear( uint32 offset, uint32 mask )
{
	uint32 *pInt = this->Base() + offset;
	*pInt &= ~mask;
}

//-----------------------------------------------------------------------------

template <class BASE_OPS>
inline uint32 CBitVecT<BASE_OPS>::Get( uint32 offset, uint32 mask )
{
	uint32 *pInt = this->Base() + offset;
	return ( *pInt & mask );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::And(const CBitVecT &addStr, CBitVecT *out) const
{
	this->ValidateOperand( addStr );
	this->ValidateOperand( *out );
	
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= this->Base();
	const uint32 *pOperand2	= addStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0 ; --i) 
	{
		pDest[i] = pOperand1[i] & pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Or(const CBitVecT &orStr, CBitVecT *out) const
{
	this->ValidateOperand( orStr );
	this->ValidateOperand( *out );

	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= this->Base();
	const uint32 *pOperand2	= orStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i) 
	{
		pDest[i] = pOperand1[i] | pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Xor(const CBitVecT &xorStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= this->Base();
	const uint32 *pOperand2	= xorStr.Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i) 
	{
		pDest[i] = pOperand1[i] ^ pOperand2[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Not(CBitVecT *out) const
{
	this->ValidateOperand( *out );

	uint32 *	   pDest	= out->Base();
	const uint32 *pOperand	= this->Base();

	for (int i = this->GetNumDWords() - 1; i >= 0; --i) 
	{
		pDest[i] = ~(pOperand[i]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copy a bit string
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::CopyTo(CBitVecT *out) const
{
	out->Resize( this->GetNumBits() );

	this->ValidateOperand( *out );
	Assert( out != this );
	
	memcpy( out->Base(), this->Base(), this->GetNumDWords() * sizeof( int ) );
}

//-----------------------------------------------------------------------------
// Purpose: Are all bits zero?
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllClear(void) const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for zero
	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords()-1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	for (int i = this->GetNumDWords() - 1; i >= 0; --i) 
	{
		if ( this->Base()[i] !=0 ) 
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Are all bits set?
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::IsAllSet(void) const
{
	// Number of available bits may be more than the number
	// actually used, so make sure to mask out unused bits
	// before testing for set bits
	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords()-1] |= ~CBitVecT<BASE_OPS>::GetEndMask();  // external semantics of const retained

	for (int i = this->GetNumDWords() - 1; i >= 0; --i) 
	{
		if ( this->Base()[i] != ~0 ) 
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Sets all bits
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetAll(void)		
{
	if ( this->Base() )
		Plat_FastMemset( this->Base(), 0xff, this->GetNumDWords() * sizeof(int) );
}

//-----------------------------------------------------------------------------
// Purpose: Clears all bits
// Input  :
// Output :
//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::ClearAll(void)		
{
	if ( this->Base() )
		Plat_FastMemset( this->Base(), 0, this->GetNumDWords() * sizeof(int) );
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::Copy( const CBitVecT<BASE_OPS> &other, int nBits )
{
	if ( nBits == - 1 )
	{
		nBits = other.GetNumBits();
	}

	this->Resize( nBits );

	this->ValidateOperand( other );
	Assert( &other != this );

	memcpy( this->Base(), other.Base(), this->GetNumDWords() * sizeof( uint32 ) );
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline bool CBitVecT<BASE_OPS>::Compare( const CBitVecT<BASE_OPS> &other, int nBits ) const
{
	if ( nBits == - 1 )
	{
		if ( other.GetNumBits() != this->GetNumBits() )
		{
			return false;
		}

		nBits = other.GetNumBits();
	}

	if ( nBits > other.GetNumBits() || nBits > this->GetNumBits() )
	{
		return false;
	}

	(const_cast<CBitVecT *>(this))->Base()[this->GetNumDWords()-1] &= CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained
	(const_cast<CBitVecT *>(&other))->Base()[this->GetNumDWords()-1] &= other.CBitVecT<BASE_OPS>::GetEndMask(); // external semantics of const retained

	int nBytes = PAD_NUMBER( nBits, 8 ) >> 3;

	return ( memcmp( this->Base(), other.Base(), nBytes ) == 0 );
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline uint32 CBitVecT<BASE_OPS>::GetDWord(int i) const
{
	Assert(i >= 0 && i < this->GetNumDWords());
	return this->Base()[i];
}

//-----------------------------------------------------------------------------
template <class BASE_OPS>
inline void CBitVecT<BASE_OPS>::SetDWord(int i, uint32 val)
{
	Assert(i >= 0 && i < this->GetNumDWords());
	this->Base()[i] = val;
}

//-----------------------------------------------------------------------------

inline unsigned GetStartBitMask( int startBit )
{
	static unsigned int g_StartMask[32] =
	{
		0xffffffff,
		0xfffffffe,
		0xfffffffc,
		0xfffffff8,
		0xfffffff0,
		0xffffffe0,
		0xffffffc0,
		0xffffff80,
		0xffffff00,
		0xfffffe00,
		0xfffffc00,
		0xfffff800,
		0xfffff000,
		0xffffe000,
		0xffffc000,
		0xffff8000,
		0xffff0000,
		0xfffe0000,
		0xfffc0000,
		0xfff80000,
		0xfff00000,
		0xffe00000,
		0xffc00000,
		0xff800000,
		0xff000000,
		0xfe000000,
		0xfc000000,
		0xf8000000,
		0xf0000000,
		0xe0000000,
		0xc0000000,
		0x80000000,
	};

	return g_StartMask[ startBit & 31 ];
}

template <typename BITCOUNTTYPE>
inline int CVarBitVecBase<BITCOUNTTYPE>::FindNextSetBit( int startBit ) const
{
	if ( startBit < GetNumBits() )
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask( startBit );
		int lastWord = GetNumDWords()-1;

		// handle non dword lengths
		if ( (GetNumBits() % BITS_PER_INT) != 0 )
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if ( wordIndex == lastWord)
			{
				elem &= (GetEndMask());
				// there's a bit remaining in this word
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for ( int i = wordIndex+1; i < lastWord; i++ )
				{
					elem = Base()[i];
					if ( elem )
						return FirstBitInWord(elem, i << 5);
				}
				elem = Base()[lastWord] & GetEndMask();
				if ( elem )
					return FirstBitInWord(elem, lastWord << 5);
			}
		}
		else
		{
			const uint32 * RESTRICT pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do 
			{
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);
				++pCurElem;
				elem = *pCurElem;
				++wordIndex;
			} while( wordIndex <= lastWord );
		}

	}

	return -1;
}

template <int NUM_BITS>
inline int CFixedBitVecBase<NUM_BITS>::FindNextSetBit( int startBit ) const
{
	if ( startBit < NUM_BITS )
	{
		int wordIndex = BitVec_Int(startBit);
		unsigned int startMask = GetStartBitMask( startBit );

		// handle non dword lengths
		if ( (NUM_BITS % BITS_PER_INT) != 0 )
		{
			unsigned int elem = Base()[wordIndex];
			elem &= startMask;
			if ( wordIndex == NUM_INTS-1)
			{
				elem &= (GetEndMask());
				// there's a bit remaining in this word
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);
			}
			else
			{
				// there's a bit remaining in this word
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);

				// iterate the words
				for ( int i = wordIndex+1; i < NUM_INTS-1; i++ )
				{
					elem = Base()[i];
					if ( elem )
						return FirstBitInWord(elem, i << 5);
				}
				elem = Base()[NUM_INTS-1] & GetEndMask();
				if ( elem )
					return FirstBitInWord(elem, (NUM_INTS-1) << 5);
			}
		}
		else
		{
			const uint32 * RESTRICT pCurElem = Base() + wordIndex;
			unsigned int elem = *pCurElem;
			elem &= startMask;
			do 
			{
				if ( elem )
					return FirstBitInWord(elem, wordIndex << 5);
				++pCurElem;
				elem = *pCurElem;
				++wordIndex;
			} while( wordIndex <= NUM_INTS-1);
		}

	}

	return -1;
}

//-----------------------------------------------------------------------------
// Unrolled loops for some common sizes

template<> 
FORCEINLINE_TEMPLATE void CBitVecT< CFixedBitVecBase<256> >::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= Base();
	const uint32 *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
	pDest[4] = pOperand1[4] & pOperand2[4];
	pDest[5] = pOperand1[5] & pOperand2[5];
	pDest[6] = pOperand1[6] & pOperand2[6];
	pDest[7] = pOperand1[7] & pOperand2[7];
}

template<> 
FORCEINLINE_TEMPLATE  bool CBitVecT< CFixedBitVecBase<256> >::IsAllClear(void) const
{
	const uint32 *pInts = Base();
	return ( pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0 && pInts[4] == 0 && pInts[5] == 0 && pInts[6] == 0 && pInts[7] == 0 );
}

template<> 
FORCEINLINE_TEMPLATE  void CBitVecT< CFixedBitVecBase<256> >::CopyTo(CBitVecT *out) const
{
	uint32 *	   pDest = out->Base();
	const uint32 *pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
	pDest[4] = pInts[4];
	pDest[5] = pInts[5];
	pDest[6] = pInts[6];
	pDest[7] = pInts[7];
}

template<> 
FORCEINLINE_TEMPLATE  void CBitVecT< CFixedBitVecBase<128> >::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= Base();
	const uint32 *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
	pDest[3] = pOperand1[3] & pOperand2[3];
}

template<> 
FORCEINLINE_TEMPLATE  bool CBitVecT< CFixedBitVecBase<128> >::IsAllClear(void) const
{
	const uint32 *pInts = Base();
	return ( pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 && pInts[3] == 0 );
}

template<> 
FORCEINLINE_TEMPLATE  void CBitVecT< CFixedBitVecBase<128> >::CopyTo(CBitVecT *out) const
{
	uint32 *	   pDest = out->Base();
	const uint32 *pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
	pDest[3] = pInts[3];
}

template<> 
inline void CBitVecT< CFixedBitVecBase<96> >::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= Base();
	const uint32 *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
	pDest[2] = pOperand1[2] & pOperand2[2];
}

template<> 
inline bool CBitVecT< CFixedBitVecBase<96> >::IsAllClear(void) const
{
	const uint32 *pInts = Base();
	return ( pInts[0] == 0 && pInts[1] == 0 && pInts[2] == 0 );
}

template<> 
inline void CBitVecT< CFixedBitVecBase<96> >::CopyTo(CBitVecT *out) const
{
	uint32 *	   pDest = out->Base();
	const uint32 *pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
	pDest[2] = pInts[2];
}

template<> 
inline void CBitVecT< CFixedBitVecBase<64> >::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= Base();
	const uint32 *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
	pDest[1] = pOperand1[1] & pOperand2[1];
}

template<> 
inline bool CBitVecT< CFixedBitVecBase<64> >::IsAllClear(void) const
{
	const uint32 *pInts = Base();
	return ( pInts[0] == 0 && pInts[1] == 0 );
}

template<> 
inline void CBitVecT< CFixedBitVecBase<64> >::CopyTo(CBitVecT *out) const
{
	uint32 *	   pDest = out->Base();
	const uint32 *pInts = Base();

	pDest[0] = pInts[0];
	pDest[1] = pInts[1];
}

template<> 
inline void CBitVecT< CFixedBitVecBase<32> >::And(const CBitVecT &addStr, CBitVecT *out) const
{
	uint32 *	   pDest		= out->Base();
	const uint32 *pOperand1	= Base();
	const uint32 *pOperand2	= addStr.Base();

	pDest[0] = pOperand1[0] & pOperand2[0];
}

template<> 
inline bool CBitVecT< CFixedBitVecBase<32> >::IsAllClear(void) const
{
	const uint32 *pInts = Base();

	return ( pInts[0] == 0 );
}

template<> 
inline void CBitVecT< CFixedBitVecBase<32> >::CopyTo(CBitVecT *out) const
{
	uint32 *	   pDest = out->Base();
	const uint32 *pInts = Base();

	pDest[0] = pInts[0];
}

//-----------------------------------------------------------------------------

template <>
inline uint32 CBitVecT< CFixedBitVecBase<32> >::Get( uint32 bitNum ) const
{
	return ( *Base() & BitVec_Bit( bitNum ) );
}

//-----------------------------------------------------------------------------

template <>
inline bool CBitVecT< CFixedBitVecBase<32> >::IsBitSet( int bitNum ) const
{
	return ( ( *Base() & BitVec_Bit( bitNum ) ) != 0 );
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Set( int bitNum )			
{
	*Base() |= BitVec_Bit( bitNum );
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Clear(int bitNum)		
{
	*Base() &= ~BitVec_Bit( bitNum );
}

//-----------------------------------------------------------------------------

template <>
inline void CBitVecT< CFixedBitVecBase<32> >::Set( int bitNum, bool bNewVal )
{
	uint32 bitMask = BitVec_Bit( bitNum );
	if ( bNewVal )
	{
		*Base() |= bitMask;
	}
	else
	{
		*Base() &= ~bitMask;
	}
}


//-----------------------------------------------------------------------------

#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Resizes the bit string to a new number of bits
// Input  : resizeNumBits - 
//-----------------------------------------------------------------------------
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::Resize( int resizeNumBits, bool bClearAll )
{
	Assert( resizeNumBits >= 0 && ((BITCOUNTTYPE)resizeNumBits == resizeNumBits) );

	int newIntCount = CalcNumIntsForBits( resizeNumBits );
	if ( newIntCount != GetNumDWords() )
	{
		if ( Base() )
		{
			ReallocInts( newIntCount );
			if ( !bClearAll && resizeNumBits >= GetNumBits() )
			{
				Base()[GetNumDWords() - 1] &= GetEndMask();
				Plat_FastMemset( Base() + GetNumDWords(), 0, (newIntCount - GetNumDWords()) * sizeof(int) );
			}
		}
		else
		{
			// Figure out how many ints are needed
			AllocInts( newIntCount );
			// Initialize bitstring by clearing all bits
			bClearAll = true;
		}

		m_numInts = newIntCount;
	} 
	else if ( !bClearAll && resizeNumBits >= GetNumBits() && Base() )
	{
		Base()[GetNumDWords() - 1] &= GetEndMask();
	}

	if ( bClearAll && Base() )
	{
		Plat_FastMemset( Base(), 0, newIntCount * sizeof(int) );
	}

	// store the new size and end mask
	m_numBits = resizeNumBits;
}

//-----------------------------------------------------------------------------
// Purpose: Allocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::AllocInts( int numInts )
{
	Assert( !m_pInt );

	if ( numInts == 0 )
		return;

	if ( numInts == 1 )
	{
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (uint32 *)malloc( numInts * sizeof(int) );
}


//-----------------------------------------------------------------------------
// Purpose: Reallocate the storage for the ints
// Input  : numInts - 
//-----------------------------------------------------------------------------
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::ReallocInts( int numInts )
{
	Assert( Base() );
	if ( numInts == 0)
	{
		FreeInts();
		return;
	}

	if ( m_pInt == &m_iBitStringStorage )
	{
		if ( numInts != 1 )
		{
			m_pInt = ((uint32 *)malloc( numInts * sizeof(int) ));
			*m_pInt = m_iBitStringStorage;
		}

		return;
	}

	if ( numInts == 1 )
	{
		m_iBitStringStorage = *m_pInt;
		free( m_pInt );
		m_pInt = &m_iBitStringStorage;
		return;
	}

	m_pInt = (uint32 *)realloc( m_pInt,  numInts * sizeof(int) );
}


//-----------------------------------------------------------------------------
// Purpose: Free storage allocated with AllocInts
//-----------------------------------------------------------------------------
template <typename BITCOUNTTYPE>
inline void CVarBitVecBase<BITCOUNTTYPE>::FreeInts( void )
{
	if ( m_numInts > 1 )
	{
		free( m_pInt );
	}
	m_pInt = NULL;
}

#include "tier0/memdbgoff.h"

// ------------------------------------------------------------------------ //
// CBitVecAccessor inlines.
// ------------------------------------------------------------------------ //

inline CBitVecAccessor::CBitVecAccessor(uint32 *pDWords, int iBit)
{
	m_pDWords = pDWords;
	m_iBit = iBit;
}


inline void CBitVecAccessor::operator=(int val)
{
	if(val)
		m_pDWords[m_iBit >> 5] |= (1 << (m_iBit & 31));
	else
		m_pDWords[m_iBit >> 5] &= ~(unsigned long)(1 << (m_iBit & 31));
}

inline CBitVecAccessor::operator uint32()
{
	return m_pDWords[m_iBit >> 5] & (1 << (m_iBit & 31));
}


//=============================================================================

#endif // BITVEC_H
