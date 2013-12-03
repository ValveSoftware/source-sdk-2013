//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation of SHA-1
//
//=============================================================================

#ifndef CHECKSUM_SHA1_H
#define CHECKSUM_SHA1_H

#if defined( _WIN32 )
#pragma once
#endif

/*
	100% free public domain implementation of the SHA-1
	algorithm by Dominik Reichl <dominik.reichl@t-online.de>


	=== Test Vectors (from FIPS PUB 180-1) ===

	SHA1("abc") =
		A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D

	SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
		84983E44 1C3BD26E BAAE4AA1 F95129E5 E54670F1

	SHA1(A million repetitions of "a") =
		34AA973C D4C4DAA4 F61EEB2B DBAD2731 6534016F
*/

#if !defined(_MINIMUM_BUILD_)
#include <stdio.h>  // Needed for file access
#if defined( _PS3 )
#include <sys/memory.h>
#else
#include <memory.h>
#endif
#include <string.h> // Needed for strcat and strcpy
#endif

// If you're compiling big endian, just comment out the following line
#define SHA1_LITTLE_ENDIAN

typedef union
{
	unsigned char c[64];
	unsigned long l[16];
} SHA1_WORKSPACE_BLOCK;

// SHA1 hash
const unsigned int k_cubHash = 20;
const unsigned int k_cchHash = 41; // k_cubHash * 2, plus 1 for terminator
#pragma pack( push, 1 )
typedef	unsigned char SHADigest_t[ k_cubHash ];
#pragma pack( pop )

#if !defined(_MINIMUM_BUILD_)
class CSHA1
#else 
class Minimum_CSHA1 
#endif
{
public:
	// Two different formats for ReportHash(...)
	enum
	{
		REPORT_HEX = 0,
		REPORT_DIGIT = 1
	};

	// Constructor and Destructor
#if !defined(_MINIMUM_BUILD_) 
	CSHA1();
	virtual ~CSHA1() ;
#else
	Minimum_CSHA1() ; 
	~Minimum_CSHA1() ;	// no virtual destructor's in the minimal builds !
#endif	

	unsigned long m_state[5];
	unsigned long m_count[2];
	unsigned char m_buffer[64];
	unsigned char m_digest[k_cubHash];

	void Reset();

	// Update the hash value
	void Update(unsigned char *data, unsigned int len);
#if !defined(_MINIMUM_BUILD_) 
	bool HashFile(char *szFileName);
#endif

	// Finalize hash and report
	void Final();
#if !defined(_MINIMUM_BUILD_) 
	void ReportHash(char *szReport, unsigned char uReportType = REPORT_HEX);
#endif
	void GetHash(unsigned char *uDest);

private:
	// Private SHA-1 transformation
	void Transform(unsigned long state[5], unsigned char buffer[64]);

	// Member variables
	unsigned char m_workspace[64];
	SHA1_WORKSPACE_BLOCK *m_block; // SHA1 pointer to the byte array above
};

#define GenerateHash( hash, pubData, cubData ) { CSHA1 sha1; sha1.Update( (byte *)pubData, cubData ); sha1.Final(); sha1.GetHash( hash ); } 

#if !defined(_MINIMUM_BUILD_)
// hash comparison function, for use with CUtlMap/CUtlRBTree
bool HashLessFunc( SHADigest_t const &lhs, SHADigest_t const &rhs );

// utility class for manipulating SHA1 hashes in their compact form
struct CSHA
{
public:
	SHADigest_t m_shaDigest;

	CSHA()
	{
		memset( m_shaDigest, 0, k_cubHash );
	}

	explicit CSHA( const SHADigest_t rhs )
	{
		memcpy( m_shaDigest, rhs, k_cubHash );
	}

	SHADigest_t &SHADigest()
	{
		return m_shaDigest;
	}

	bool operator<( const CSHA &rhs ) const
	{
		return memcmp( m_shaDigest, rhs.m_shaDigest, k_cubHash ) < 0;
	}

	bool operator==( const CSHA &rhs ) const
	{
		return memcmp( m_shaDigest, rhs.m_shaDigest, k_cubHash ) == 0;
	}

	bool operator!=( const CSHA &rhs ) const
	{
		return !(*this == rhs);
	}

	bool operator==( const SHADigest_t &rhs ) const
	{
		return memcmp( m_shaDigest, rhs, k_cubHash ) == 0;
	}

	bool operator!=( const SHADigest_t &rhs ) const
	{
		return !(*this == rhs);
	}

	CSHA &operator=( const SHADigest_t rhs )
	{
		memcpy( m_shaDigest, rhs, k_cubHash );
		return *this;
	}

	void AssignTo( SHADigest_t rhs ) const
	{
		memcpy( rhs, m_shaDigest, k_cubHash );
	}
};
#endif // _MINIMUM_BUILD_

#endif // CHECKSUM_SHA1_H
