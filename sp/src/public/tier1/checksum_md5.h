//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic MD5 hashing algo
//
//=============================================================================//

#ifndef CHECKSUM_MD5_H
#define CHECKSUM_MD5_H

#ifdef _WIN32
#pragma once
#endif

// 16 bytes == 128 bit digest
#define MD5_DIGEST_LENGTH 16  
#define MD5_BIT_LENGTH ( MD5_DIGEST_LENGTH * sizeof(unsigned char) )
struct MD5Value_t
{
	unsigned char bits[MD5_DIGEST_LENGTH];

	void Zero();
	bool IsZero() const;

	bool operator==( const MD5Value_t &src ) const;
	bool operator!=( const MD5Value_t &src ) const;

};

// MD5 Hash
typedef struct
{
	unsigned int	buf[4];
    unsigned int	bits[2];
    unsigned char	in[64];
} MD5Context_t;

void MD5Init( MD5Context_t *context );
void MD5Update( MD5Context_t *context, unsigned char const *buf, unsigned int len );
void MD5Final( unsigned char digest[ MD5_DIGEST_LENGTH ], MD5Context_t *context );

char *MD5_Print(unsigned char *digest, int hashlen );

/// Convenience wrapper to calculate the MD5 for a buffer, all in one step, without
/// bothering with the context object.
void MD5_ProcessSingleBuffer( const void *p, int len, MD5Value_t &md5Result );

unsigned int MD5_PseudoRandom(unsigned int nSeed);

/// Returns true if the values match.
bool MD5_Compare( const MD5Value_t &data, const MD5Value_t &compare );

inline bool MD5Value_t::operator==( const MD5Value_t &src ) const
{
	return MD5_Compare( *this, src );
}

inline bool MD5Value_t::operator!=( const MD5Value_t &src ) const
{
	return !MD5_Compare( *this, src );
}

#endif // CHECKSUM_MD5_H
