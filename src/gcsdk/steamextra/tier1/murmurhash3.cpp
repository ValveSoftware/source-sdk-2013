//======= Copyright © Valve Corporation, All rights reserved. =================
//
// Public domain MurmurHash3 by Austin Appleby is a very solid general-purpose
// hash with a 32-bit output. References:
// http://code.google.com/p/smhasher/ (home of MurmurHash3)
// https://sites.google.com/site/murmurhash/avalanche
// http://www.strchr.com/hash_functions 
//
//=============================================================================

#include <stdafx.h>
#include "murmurhash3.h"

//-----------------------------------------------------------------------------

uint32 MurmurHash3_32( const void * key, size_t len, uint32 seed, bool bCaselessStringVariant )
{
	const uint8 * data = (const uint8*)key;
	const ptrdiff_t nblocks = len / 4;
	uint32 uSourceBitwiseAndMask = 0xDFDFDFDF | ((uint32)bCaselessStringVariant - 1);

	uint32 h1 = seed;

	//----------
	// body

	const uint32 * blocks = (const uint32 *)(data + nblocks*4);

	for(ptrdiff_t i = -nblocks; i; i++)
	{
		uint32 k1 = LittleDWord(blocks[i]);
		k1 &= uSourceBitwiseAndMask;

		k1 *= 0xcc9e2d51;
		k1 = (k1 << 15) | (k1 >> 17);
		k1 *= 0x1b873593;

		h1 ^= k1;
		h1 = (h1 << 13) | (h1 >> 19);
		h1 = h1*5+0xe6546b64;
	}

	//----------
	// tail

	const uint8 * tail = (const uint8*)(data + nblocks*4);

	uint32 k1 = 0;

	switch(len & 3)
	{
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
		k1 &= uSourceBitwiseAndMask;
		k1 *= 0xcc9e2d51;
		k1 = (k1 << 15) | (k1 >> 17);
		k1 *= 0x1b873593;
		h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;

	return h1;
} 
