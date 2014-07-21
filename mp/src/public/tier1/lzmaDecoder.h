//
//	LZMA Decoder. Designed for run time decoding.
//
//	LZMA SDK 4.43 Copyright (c) 1999-2006 Igor Pavlov (2006-05-01)
//	http://www.7-zip.org/
//
//=====================================================================================//

#ifndef _LZMADECODER_H
#define _LZMADECODER_H
#pragma once

#if !defined( _X360 )
#define LZMA_ID				(('A'<<24)|('M'<<16)|('Z'<<8)|('L'))
#else
#define LZMA_ID				(('L'<<24)|('Z'<<16)|('M'<<8)|('A'))
#endif

// bind the buffer for correct identification
#pragma pack(1)
struct lzma_header_t
{
	unsigned int	id;
	unsigned int	actualSize;		// always little endian
	unsigned int	lzmaSize;		// always little endian
	unsigned char	properties[5];
};
#pragma pack()

class CLZMA
{
public:
	unsigned int	Uncompress( unsigned char *pInput, unsigned char *pOutput );
	bool			IsCompressed( unsigned char *pInput );
	unsigned int	GetActualSize( unsigned char *pInput );

private:
};

#endif

