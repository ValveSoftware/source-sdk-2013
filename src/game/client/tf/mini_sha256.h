/* Crypto/Sha256.c -- SHA-256 Hash
2010-06-11 : Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

/* define it for speed optimization */
/* #define _SHA256_UNROLL */
/* #define _SHA256_UNROLL2 */

static constexpr uint32_t k_unSha256DigestSize = 32;

struct Sha256_t
{
	uint32 state[8];
	uint64 count;
	uint8 buffer[64];
};

using SHA256Digest_t = uint8[k_unSha256DigestSize];

#ifdef _MSC_VER

#include <stdlib.h>

#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)

#define rotlFixed(x, n) _rotl((x), (n))
#define rotrFixed(x, n) _rotr((x), (n))

#else

#define rotlFixed(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define rotrFixed(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#endif

inline void Sha256_Init( Sha256_t* p )
{
	p->state[0] = 0x6a09e667;
	p->state[1] = 0xbb67ae85;
	p->state[2] = 0x3c6ef372;
	p->state[3] = 0xa54ff53a;
	p->state[4] = 0x510e527f;
	p->state[5] = 0x9b05688c;
	p->state[6] = 0x1f83d9ab;
	p->state[7] = 0x5be0cd19;
	p->count = 0;
	memset( p->buffer, 0, sizeof( p->buffer ) );
}

#define S0(x) (rotrFixed(x, 2) ^ rotrFixed(x,13) ^ rotrFixed(x, 22))
#define S1(x) (rotrFixed(x, 6) ^ rotrFixed(x,11) ^ rotrFixed(x, 25))
#define s0(x) (rotrFixed(x, 7) ^ rotrFixed(x,18) ^ (x >> 3))
#define s1(x) (rotrFixed(x,17) ^ rotrFixed(x,19) ^ (x >> 10))

#define blk0(i) (W[i] = data[i])
#define blk2(i) (W[i&15] += s1(W[(i-2)&15]) + W[(i-7)&15] + s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))

#define a(i) T[(0-(i))&7]
#define b(i) T[(1-(i))&7]
#define c(i) T[(2-(i))&7]
#define d(i) T[(3-(i))&7]
#define e(i) T[(4-(i))&7]
#define f(i) T[(5-(i))&7]
#define g(i) T[(6-(i))&7]
#define h(i) T[(7-(i))&7]

#define R(i) h(i) += S1(e(i)) + Ch(e(i),f(i),g(i)) + K[i+j] + (j?blk2(i):blk0(i));\
  d(i) += h(i); h(i) += S0(a(i)) + Maj(a(i), b(i), c(i))


static constexpr uint32 K[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline void Sha256_Transform( uint32* state, const uint32* data )
{
	uint32 W[16];

	uint32 T[8];
	for ( unsigned j = 0; j < 8; j++ )
	{
		T[j] = state[j];
	}

	for ( unsigned j = 0; j < 64; j += 16 )
	{
		for ( unsigned i = 0; i < 16; i++ )
		{
			R( i );
		}
	}


	for ( unsigned j = 0; j < 8; j++ )
		state[j] += T[j];

	/* Wipe variables */
	/* memset(W, 0, sizeof(W)); */
	/* memset(T, 0, sizeof(T)); */
}

#undef S0
#undef S1
#undef s0
#undef s1

inline static void Sha256_WriteByteBlock( Sha256_t* p )
{
	uint32 data32[16];
	for ( unsigned i = 0; i < 16; i++ )
	{
		data32[i] =
			( ( uint32 )( p->buffer[i * 4] ) << 24 ) +
			( ( uint32 )( p->buffer[i * 4 + 1] ) << 16 ) +
			( ( uint32 )( p->buffer[i * 4 + 2] ) << 8 ) +
			( ( uint32 )( p->buffer[i * 4 + 3] ) );
	}
	Sha256_Transform( p->state, data32 );
}

inline void Sha256_Update( Sha256_t* p, const uint8* data, size_t size )
{
	uint32 curBufferPos = ( uint32 )p->count & 0x3F;
	while ( size > 0 )
	{
		p->buffer[curBufferPos++] = *data++;
		p->count++;
		size--;
		if ( curBufferPos == 64 )
		{
			curBufferPos = 0;
			Sha256_WriteByteBlock( p );
		}
	}
}

inline void Sha256_Final( Sha256_t* p, uint8* digest )
{
	uint64 lenInBits = ( p->count << 3 );
	uint32 curBufferPos = ( uint32 )p->count & 0x3F;

	p->buffer[curBufferPos++] = 0x80;
	while ( curBufferPos != ( 64 - 8 ) )
	{
		curBufferPos &= 0x3F;
		if ( curBufferPos == 0 )
			Sha256_WriteByteBlock( p );
		p->buffer[curBufferPos++] = 0;
	}
	for ( int i = 0; i < 8; i++ )
	{
		p->buffer[curBufferPos++] = ( uint8 )( lenInBits >> 56 );
		lenInBits <<= 8;
	}
	Sha256_WriteByteBlock( p );

	for ( int i = 0; i < 8; i++ )
	{
		*digest++ = ( uint8 )( p->state[i] >> 24 );
		*digest++ = ( uint8 )( p->state[i] >> 16 );
		*digest++ = ( uint8 )( p->state[i] >> 8 );
		*digest++ = ( uint8 )( p->state[i] );
	}
	Sha256_Init( p );
}
