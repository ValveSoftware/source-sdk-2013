//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Unique ID generation
//=============================================================================//

#include "tier0/platform.h"

#ifdef IS_WINDOWS_PC
#include <windows.h> // UUIDCreate
#else
#include "checksum_crc.h"
#endif
#include "tier1/uniqueid.h"
#include "tier1/utlbuffer.h"

//-----------------------------------------------------------------------------
// Creates a new unique id
//-----------------------------------------------------------------------------
void CreateUniqueId( UniqueId_t *pDest )
{
#ifdef IS_WINDOWS_PC
	Assert( sizeof( UUID ) == sizeof( *pDest ) );
	UuidCreate( (UUID *)pDest );
#else
	// X360/linux TBD: Need a real UUID Implementation
	Q_memset( pDest, 0, sizeof( UniqueId_t ) );
#endif
}


//-----------------------------------------------------------------------------
// Creates a new unique id from a string representation of one
//-----------------------------------------------------------------------------
bool UniqueIdFromString( UniqueId_t *pDest, const char *pBuf, int nMaxLen )
{
	if ( nMaxLen == 0 )
	{
		nMaxLen = Q_strlen( pBuf );
	}

	char *pTemp = (char*)stackalloc( nMaxLen + 1 );
	V_strncpy( pTemp, pBuf, nMaxLen + 1 );
	--nMaxLen;
	while( (nMaxLen >= 0) && isspace( pTemp[nMaxLen] ) )
	{
		--nMaxLen;
	}
	pTemp[ nMaxLen + 1 ] = 0;

	while( *pTemp && isspace( *pTemp ) )
	{
		++pTemp;
	}

#ifdef IS_WINDOWS_PC
	Assert( sizeof( UUID ) == sizeof( *pDest ) );

	if ( RPC_S_OK != UuidFromString( (unsigned char *)pTemp, (UUID *)pDest ) )
	{
		InvalidateUniqueId( pDest );
		return false;
	}
#else
	// X360TBD: Need a real UUID Implementation
	// For now, use crc to generate a unique ID from the UUID string.
	Q_memset( pDest, 0, sizeof( UniqueId_t ) );
	if ( nMaxLen > 0 )
	{
		CRC32_t crc;
		CRC32_Init( &crc );
		CRC32_ProcessBuffer( &crc, pBuf, nMaxLen );
		CRC32_Final( &crc );
		Q_memcpy( pDest, &crc, sizeof( CRC32_t ) );
	}
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Sets an object ID to be an invalid state
//-----------------------------------------------------------------------------
void InvalidateUniqueId( UniqueId_t *pDest )
{
	Assert( pDest );
	memset( pDest, 0, sizeof( UniqueId_t ) );
}

bool IsUniqueIdValid( const UniqueId_t &id )
{
	UniqueId_t invalidId;
	memset( &invalidId, 0, sizeof( UniqueId_t ) );
	return !IsUniqueIdEqual( invalidId, id );
}

bool IsUniqueIdEqual( const UniqueId_t &id1, const UniqueId_t &id2 )
{
	return memcmp( &id1, &id2, sizeof( UniqueId_t ) ) == 0; 
}

void UniqueIdToString( const UniqueId_t &id, char *pBuf, int nMaxLen )
{
	pBuf[ 0 ] = 0;

// X360TBD: Need a real UUID Implementation
#ifdef IS_WINDOWS_PC
	UUID *self = ( UUID * )&id;

	unsigned char *outstring = NULL;

	UuidToString( self, &outstring );
	if ( outstring && *outstring )
	{
		Q_strncpy( pBuf, (const char *)outstring, nMaxLen );
		RpcStringFree( &outstring );
	}
#endif
}

void CopyUniqueId( const UniqueId_t &src, UniqueId_t *pDest )
{
	memcpy( pDest, &src, sizeof( UniqueId_t ) );
}

bool Serialize( CUtlBuffer &buf, const UniqueId_t &src )
{
// X360TBD: Need a real UUID Implementation
#ifdef IS_WINDOWS_PC
	if ( buf.IsText() )
	{
		UUID *pId = ( UUID * )&src;

		unsigned char *outstring = NULL;

		UuidToString( pId, &outstring );
		if ( outstring && *outstring )
		{
			buf.PutString( (const char *)outstring );
			RpcStringFree( &outstring );
		}
		else
		{
			buf.PutChar( '\0' );
		}
	}
	else
	{
		buf.Put( &src, sizeof(UniqueId_t) );
	}
	return buf.IsValid();
#else
	return false;
#endif
}

bool Unserialize( CUtlBuffer &buf, UniqueId_t &dest )
{
	if ( buf.IsText() )
	{
		int nTextLen = buf.PeekStringLength();
		char *pBuf = (char*)stackalloc( nTextLen );
		buf.GetString( pBuf, nTextLen );
		UniqueIdFromString( &dest, pBuf, nTextLen );
	}
	else
	{
		buf.Get( &dest, sizeof(UniqueId_t) );
	}
	return buf.IsValid();
}



