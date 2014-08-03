//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic CRC functions
//
// $NoKeywords: $
//=============================================================================//
#ifndef CHECKSUM_CRC_H
#define CHECKSUM_CRC_H
#ifdef _WIN32
#pragma once
#endif

typedef unsigned int CRC32_t;

void CRC32_Init( CRC32_t *pulCRC );
void CRC32_ProcessBuffer( CRC32_t *pulCRC, const void *p, int len );
void CRC32_Final( CRC32_t *pulCRC );
CRC32_t	CRC32_GetTableEntry( unsigned int slot );

inline CRC32_t CRC32_ProcessSingleBuffer( const void *p, int len )
{
	CRC32_t crc;

	CRC32_Init( &crc );
	CRC32_ProcessBuffer( &crc, p, len );
	CRC32_Final( &crc );

	return crc;
}

#endif // CHECKSUM_CRC_H
