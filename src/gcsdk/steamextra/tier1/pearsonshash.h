//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	PearsonsHash.h
//
//	This file contains implementation definitions of 'Pearson's' Hash'
//	The generic implementation is a template, plus a couple of specializations
//	for commonly used types.
//
//	Take a look at http://en.wikipedia.org/wiki/Pearson_hashing for more info.
//
//=============================================================================

#ifndef _PEARSONSHASH_H_
#define	_PEARSONSHASH_H_

#if defined(_WIN32) || defined(_WIN64)
#pragma once
#endif

extern const unsigned char g_CTHashRandomValues[256] ; 

template<typename T> 
struct PearsonsHashFunctor
{
	typedef	uint32  TargetType ; 
	TargetType	operator()(const T& unKey) const
	{
		// This is a pearsons hash variant that returns a maximum of 32 bits
		size_t size = sizeof(T);
		const uint8 *	k    = (const uint8 *) &unKey;
		uint32 byte_one = 0, byte_two = 0, byte_three = 0, byte_four = 0, n;
		while (size)
		{
			--size;
			n    = *k++;
			byte_one = g_CTHashRandomValues[byte_one ^ n];

			if (size)
			{
				--size;
				n   = *k++;
				byte_two = g_CTHashRandomValues[byte_two ^ n];
			}
			else
				break;

			if (size)
			{
				--size;
				n   = *k++;
				byte_three = g_CTHashRandomValues[byte_three ^ n];
			}
			else
				break;

			if (size)
			{
				--size;
				n   = *k++;
				byte_four = g_CTHashRandomValues[byte_four ^ n];
			}
			else
				break;
		}

		TargetType idx = ( byte_four << 24 ) | ( byte_three << 16 ) | ( byte_two << 8 ) | byte_one;
		return ( idx ); 
	}
};

//
//	We use this specialization for pointer types - it allows somebody 
//	to define a specialization for some complicated type, and then use a 
//	pointer to that type as a key, and have that automatically go to the 
//	specialization for the complicated type !
//
template<typename T>
struct PearsonsHashFunctor<T*>	
{
	typedef	uint32	TargetType ; 
	TargetType operator()(const T* key) const 
	{
		PearsonsHashFunctor<T>	functor ; 
		return functor(*key) ; 
	}
};

//
//	This functor specializes for unsigned 32 bit integers, a commonly used type in Steam.
//	It should return the exact same result as the unspecialized version on Intel Architecture machines.
//
template<>
struct PearsonsHashFunctor<uint32>
{
	typedef	uint32 TargetType ; 
	TargetType	operator()(const uint32 unKey) const
	{
		uint32 byte_one =	g_CTHashRandomValues[(unKey>>0) & 0xff];
		uint32 byte_two =	g_CTHashRandomValues[(unKey>>8) & 0xff];
		uint32 byte_three = g_CTHashRandomValues[(unKey>>16) & 0xff];
		uint32 byte_four =	g_CTHashRandomValues[(unKey>>24)&0xff];
		return	( byte_four << 24 ) | ( byte_three << 16 ) | ( byte_two << 8 ) | byte_one; 
	}
};

//
//	This functor specializes for unsigned 64 bit integers, another commonly used type in Steam.
//	It should return the exact same result as the unspecialized version on Intel Architecture machines.
//
template<>
struct PearsonsHashFunctor<uint64>
{
	typedef	uint32 TargetType ; 
	TargetType	operator()(const uint64 unKey) const
	{
		//
		//	Note that we pull apart the 64 bits in Intel's endian order.
		//
		uint32 n; 
		//
		//	On Intel Machines, to make this return the exact same result as the generic version 
		//	we have to go from least significant byte to most significant byte ! 
		//
		n = static_cast<uint32>((unKey >> (0)) & 0xff)  ; 
		uint32 byte_one =	g_CTHashRandomValues[n];
		n = static_cast<uint32>((unKey >> (8)) & 0xff) ; 
		uint32 byte_two =	g_CTHashRandomValues[n];
		n = static_cast<uint32>((unKey >> (16)) & 0xff) ; 
		uint32 byte_three = g_CTHashRandomValues[n];
		n = static_cast<uint32>((unKey >> (24)) & 0xff)  ; 		
		uint32 byte_four =	g_CTHashRandomValues[n];
		n = static_cast<uint32>((unKey >> (32)) & 0xff)  ; 
		byte_one =			g_CTHashRandomValues[n ^ byte_one];
		n = static_cast<uint32>((unKey >> (8+32)) & 0xff)  ; 
		byte_two =			g_CTHashRandomValues[n ^ byte_two];
		n = static_cast<uint32>((unKey >> (16+32)) & 0xff)  ; 
		byte_three =		g_CTHashRandomValues[n ^ byte_three];
		n = static_cast<uint32>((unKey >> (24+32)) & 0xff)  ; 
		byte_four =			g_CTHashRandomValues[n ^ byte_four];
		return	( byte_four << 24 ) | ( byte_three << 16 ) | ( byte_two << 8 ) | byte_one; 
	}
};

//
//	This functor specializes for C standard NULL terminated strings !
//	It is setup so that if you had a char array containing a NULL terminated string
//	and correctly sized, ie char rgch[16] = { "123456789012345" } and a 
//	null terminated string i.e. char *sz = "123456789012345" these will return identical
//	results, and both include the NULL terminator in the hash calculation.
//
template<>
struct PearsonsHashFunctor<char*>
{
	typedef	uint32  TargetType ; 
	TargetType	operator()(const char* szKey) const
	{
		const uint8 *	k    = (const uint8 *) szKey ;
		uint32 byte_one = 0, byte_two = 0, byte_three = 0, byte_four = 0, n;
		do
		{
			n    = *k++;
			byte_one = g_CTHashRandomValues[byte_one ^ n];
			if (n=='\0')
				break;

			n   = *k++;
			byte_two = g_CTHashRandomValues[byte_two ^ n];
			if (n=='\0')
				break;

			n   = *k++;
			byte_three = g_CTHashRandomValues[byte_three ^ n];
			if (n=='\0')
				break;

			n   = *k++;
			byte_four = g_CTHashRandomValues[byte_four ^ n];
		}	while(n!='\0') ; 

		TargetType idx = ( byte_four << 24 ) | ( byte_three << 16 ) | ( byte_two << 8 ) | byte_one;
		return ( idx ); 
	}
};

//
//	This functor compares two objects of a particular type and returns a result
//	that follows the strcmp/memcmp.
//	If the type doesn't provide an operator== or operator< then you can provide 
//	a type specific specialization to override this defualt functor !
//
template<typename T>
struct	ComparisonFunctor
{
	int	operator()(const T &lhs, const T &rhs) const 
	{
		if( lhs == rhs ) 
			return 0 ; 
		else if( lhs < rhs ) 
			return -1 ; 
		else 
			return 1 ; 
	}

};

//
//	I expect people to build Comparison specializations for full types, 
//	not pointer types - this Specialization should allow the C++ compiler
//	to bind to a specialization for a particular type.
//	We do not want to compare pointers for equality, and a memcmp() may 
//	not be good for a complicated type !
//
template<typename T>
struct ComparisonFunctor<T*>
{
	int operator()(const T *lhs, const T *rhs) const 
	{
		ComparisonFunctor<T>	functor ; 
		return functor(*lhs, *rhs) ; 
	}
};

template<>
struct ComparisonFunctor<int>
{
	int		operator()(const int lhs, const int rhs) const
	{
		return lhs-rhs ; 
	}
};

template<>
struct ComparisonFunctor<unsigned int>
{
	int		operator()(const unsigned int lhs, const unsigned int rhs) const 
	{
		return static_cast<int>(lhs) - static_cast<int>(rhs) ; 
	}
};


template<>
struct ComparisonFunctor<uint64>
{
	int		operator()(const uint64 lhs, const uint64 rhs) const 
	{
		if( lhs < rhs ) 
			return -1 ; 
		else if( lhs == rhs ) 
			return 0 ;
		else
			return 1 ; 
	}
};

template<>
struct ComparisonFunctor<char*>
{
	int		operator()(const char * lhs, const char* rhs) const 
	{
		return	Q_strcmp(lhs, rhs) ; 
	}
};


#endif // _PEARSONSHASH_H_


