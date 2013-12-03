//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Variant Pearson Hash general purpose hashing algorithm described
//			by Cargill in C++ Report 1994. Generates a 16-bit result.
//
//=============================================================================

#ifndef GENERICHASH_H
#define GENERICHASH_H

#if defined(_WIN32)
#pragma once
#endif

//-----------------------------------------------------------------------------

unsigned FASTCALL HashString( const char *pszKey );
unsigned FASTCALL HashStringCaseless( const char *pszKey );
unsigned FASTCALL HashStringCaselessConventional( const char *pszKey );
unsigned FASTCALL Hash4( const void *pKey );
unsigned FASTCALL Hash8( const void *pKey );
unsigned FASTCALL Hash12( const void *pKey );
unsigned FASTCALL Hash16( const void *pKey );
unsigned FASTCALL HashBlock( const void *pKey, unsigned size );

unsigned FASTCALL HashInt( const int key );

// hash a uint32 into a uint32
FORCEINLINE uint32 HashIntAlternate( uint32 n)
{
	n = ( n + 0x7ed55d16 ) + ( n << 12 );
	n = ( n ^ 0xc761c23c ) ^ ( n >> 19 );
	n = ( n + 0x165667b1 ) + ( n << 5 );
	n = ( n + 0xd3a2646c ) ^ ( n << 9 );
	n = ( n + 0xfd7046c5 ) + ( n << 3 );
	n = ( n ^ 0xb55a4f09 ) ^ ( n >> 16 );
	return n;
}

inline unsigned HashIntConventional( const int n ) // faster but less effective
{
	// first byte
	unsigned hash = 0xAAAAAAAA + (n & 0xFF);
	// second byte
	hash = ( hash << 5 ) + hash + ( (n >> 8) & 0xFF );
	// third byte
	hash = ( hash << 5 ) + hash + ( (n >> 16) & 0xFF );
	// fourth byte
	hash = ( hash << 5 ) + hash + ( (n >> 24) & 0xFF );

	return hash;

	/* this is the old version, which would cause a load-hit-store on every
	   line on a PowerPC, and therefore took hundreds of clocks to execute!
	  
	byte *p = (byte *)&n;
	unsigned hash = 0xAAAAAAAA + *p++;
	hash = ( ( hash << 5 ) + hash ) + *p++;
	hash = ( ( hash << 5 ) + hash ) + *p++;
	return ( ( hash << 5 ) + hash ) + *p;
	*/
}

//-----------------------------------------------------------------------------

template <typename T>
inline unsigned HashItem( const T &item )
{
	// TODO: Confirm comiler optimizes out unused paths
	if ( sizeof(item) == 4 )
		return Hash4( &item );
	else if ( sizeof(item) == 8 )
		return Hash8( &item );
	else if ( sizeof(item) == 12 )
		return Hash12( &item );
	else if ( sizeof(item) == 16 )
		return Hash16( &item );
	else
		return HashBlock( &item, sizeof(item) );
}

template <> inline unsigned HashItem<int>(const int &key )
{
	return HashInt( key );
}

template <> inline unsigned HashItem<unsigned>(const unsigned &key )
{
	return HashInt( (int)key );
}

template<> inline unsigned HashItem<const char *>(const char * const &pszKey )
{
	return HashString( pszKey );
}

template<> inline unsigned HashItem<char *>(char * const &pszKey )
{
	return HashString( pszKey );
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Murmur hash
//-----------------------------------------------------------------------------
uint32 MurmurHash2( const void * key, int len, uint32 seed );

// return murmurhash2 of a downcased string
uint32 MurmurHash2LowerCase( char const *pString, uint32 nSeed );

uint64 MurmurHash64( const void * key, int len, uint32 seed );


#endif /* !GENERICHASH_H */
