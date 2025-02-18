//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef UTLSTRINGTOKEN_H
#define UTLSTRINGTOKEN_H

#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include "tier0/threadtools.h"
#include "tier1/mempool.h"
#include "tier1/generichash.h"

#define DEBUG_STRINGTOKENS 0
#define STRINGTOKEN_MURMURHASH_SEED 0x31415926

class CUtlStringToken
{
public:
	uint32 m_nHashCode;
#if DEBUG_STRINGTOKENS
	char const *m_pDebugName;
#endif

	FORCEINLINE bool operator==( CUtlStringToken const &other ) const
	{
		return ( other.m_nHashCode == m_nHashCode );
	}

	FORCEINLINE bool operator!=( CUtlStringToken const &other ) const
	{
		return ( other.m_nHashCode != m_nHashCode );
	}
	
	FORCEINLINE bool operator<( CUtlStringToken const &other ) const
	{
		return ( m_nHashCode < other.m_nHashCode );
	}


	/// access to the hash code for people who need to store thse as 32-bits, regardless of the
	/// setting of DEBUG_STRINGTOKENS (for instance, for atomic operations).
	FORCEINLINE uint32 GetHashCode( void ) const { return m_nHashCode; }

	FORCEINLINE void SetHashCode( uint32 nCode ) { m_nHashCode = nCode; }


#ifndef _DEBUG												// the auto-generated things are too big when not inlined and optimized away
#include "tier1/utlstringtoken_generated_contructors.h"
#else
    CUtlStringToken( char const *pString )					// generate one from a dynamic string 
	{
		m_nHashCode = MurmurHash2LowerCase( pString, STRINGTOKEN_MURMURHASH_SEED );
#if DEBUG_STRINGTOKENS
		m_pDebugName = NULL;
#endif
	}
#endif

	CUtlStringToken()
	{
		m_nHashCode = 0;
#if DEBUG_STRINGTOKENS
		m_pDebugName = NULL;
#endif
	}

	bool IsValid() const
	{
		return m_nHashCode != 0;
	}
};

FORCEINLINE CUtlStringToken MakeStringToken( char const *pString )
{
	CUtlStringToken ret;
	ret.m_nHashCode = MurmurHash2LowerCase( pString, STRINGTOKEN_MURMURHASH_SEED );
	return ret;
}

FORCEINLINE CUtlStringToken MakeStringToken( const char *pString, const char *pStringEnd )
{
	CUtlStringToken ret;
	ret.m_nHashCode = MurmurHash2LowerCase( pString, ( int )( pStringEnd - pString ), STRINGTOKEN_MURMURHASH_SEED );
	return ret;
}

#endif // UTLSTRINGTOKEN_H
