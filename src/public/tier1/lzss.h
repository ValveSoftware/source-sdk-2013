//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	LZSS Codec. Designed for fast cheap gametime encoding/decoding. Compression results
//	are	not aggresive as other alogrithms, but gets 2:1 on most arbitrary uncompressed data.
//
//=====================================================================================//

#ifndef _LZSS_H
#define _LZSS_H
#pragma once

#define LZSS_ID   uint32( BigDWord( ('L'<<24)|('Z'<<16)|('S'<<8)|('S') ) )
#define SNAPPY_ID uint32( BigDWord( ('S'<<24)|('N'<<16)|('A'<<8)|('P') ) )

// bind the buffer for correct identification
struct lzss_header_t
{
	unsigned int	id;
	unsigned int	actualSize;	// always little endian
};

class CUtlBuffer;

#define DEFAULT_LZSS_WINDOW_SIZE 4096

class CLZSS
{
public:
	unsigned char*	Compress( const unsigned char *pInput, int inputlen, unsigned int *pOutputSize );
	unsigned char*	CompressNoAlloc( const unsigned char *pInput, int inputlen, unsigned char *pOutput, unsigned int *pOutputSize );
	unsigned int	Uncompress( const unsigned char *pInput, unsigned char *pOutput );
	//unsigned int	Uncompress( unsigned char *pInput, CUtlBuffer &buf );
	unsigned int	SafeUncompress( const unsigned char *pInput, unsigned int inputlen, unsigned char *pOutput, unsigned int unBufSize );

	static bool			IsCompressed( const unsigned char *pInput );
	static unsigned int	GetActualSize( const unsigned char *pInput );

	// windowsize must be a power of two.
	FORCEINLINE CLZSS( int nWindowSize = DEFAULT_LZSS_WINDOW_SIZE );

private:
	// expected to be sixteen bytes
	struct lzss_node_t
	{
		const unsigned char	*pData;
		lzss_node_t		*pPrev;
		lzss_node_t		*pNext;
		char			empty[4];
	};

	struct lzss_list_t
	{
		lzss_node_t *pStart;
		lzss_node_t *pEnd;
	};

	void			BuildHash( const unsigned char *pData );
	lzss_list_t		*m_pHashTable;	
	lzss_node_t		*m_pHashTarget;
	int             m_nWindowSize;

};

FORCEINLINE CLZSS::CLZSS( int nWindowSize )
{
	m_nWindowSize = nWindowSize;
}
#endif

