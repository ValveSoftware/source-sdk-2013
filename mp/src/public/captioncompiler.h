//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef CAPTIONCOMPILER_H
#define CAPTIONCOMPILER_H
#ifdef _WIN32
#pragma once
#endif

#include "datamap.h"
#include "checksum_crc.h"

#define MAX_BLOCK_BITS	13

#define MAX_BLOCK_SIZE (1<<MAX_BLOCK_BITS )

#define COMPILED_CAPTION_FILEID			MAKEID( 'V', 'C', 'C', 'D' )
#define COMPILED_CAPTION_VERSION		1

#pragma pack(1)
struct CompiledCaptionHeader_t
{
	DECLARE_BYTESWAP_DATADESC()
	int				magic;
	int				version;
	int				numblocks;
	int				blocksize;
	int				directorysize;
	int				dataoffset;
};

struct CaptionLookup_t
{
	DECLARE_BYTESWAP_DATADESC()
	unsigned int	hash;
	int				blockNum;
	unsigned short	offset;
	unsigned short	length;

	void SetHash( char const *string )
	{
		int len = Q_strlen( string );
		char *tempstr = (char *)_alloca( len + 1 );
		Q_strncpy( tempstr, string, len + 1 );
		Q_strlower( tempstr );
		CRC32_t temp;
		CRC32_Init( &temp );
		CRC32_ProcessBuffer( &temp, tempstr, len );
		CRC32_Final( &temp );

		hash = ( unsigned int )temp;
	}
};
#pragma pack()

class CCaptionLookupLess
{
public:
	bool	Less( const CaptionLookup_t& lhs, const CaptionLookup_t& rhs, void *pContext )
	{
		return lhs.hash < rhs.hash;
	}
};

struct CaptionBlock_t
{
	byte	data[ MAX_BLOCK_SIZE ];
};

// For swapping compiled caption files
bool	SwapClosecaptionFile( void *pData );
int		UpdateOrCreateCaptionFile( const char *pSourceName, char *pTargetName, int targetLen, bool bForce = false );

#endif // CAPTIONCOMPILER_H
