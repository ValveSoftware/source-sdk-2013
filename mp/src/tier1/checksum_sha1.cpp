//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation of SHA-1
//
//=============================================================================

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
#include "checksum_sha1.h"
#else
//
//	This path is build in the CEG/DRM projects where we require that no CRT references are made !
//
#include <intrin.h>				// memcpy, memset etc... will be inlined.
#include "tier1/checksum_sha1.h"
#endif

#define MAX_FILE_READ_BUFFER 8000

// Rotate x bits to the left
#ifndef ROL32
#define ROL32(_val32, _nBits) (((_val32)<<(_nBits))|((_val32)>>(32-(_nBits))))
#endif

#ifdef SHA1_LITTLE_ENDIAN
	#define SHABLK0(i) (m_block->l[i] = \
		(ROL32(m_block->l[i],24) & 0xFF00FF00) | (ROL32(m_block->l[i],8) & 0x00FF00FF))
#else
	#define SHABLK0(i) (m_block->l[i])
#endif

#define SHABLK(i) (m_block->l[i&15] = ROL32(m_block->l[(i+13)&15] ^ m_block->l[(i+8)&15] \
	^ m_block->l[(i+2)&15] ^ m_block->l[i&15],1))

// SHA-1 rounds
#define _R0(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK0(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R1(v,w,x,y,z,i) { z+=((w&(x^y))^y)+SHABLK(i)+0x5A827999+ROL32(v,5); w=ROL32(w,30); }
#define _R2(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0x6ED9EBA1+ROL32(v,5); w=ROL32(w,30); }
#define _R3(v,w,x,y,z,i) { z+=(((w|x)&y)|(w&x))+SHABLK(i)+0x8F1BBCDC+ROL32(v,5); w=ROL32(w,30); }
#define _R4(v,w,x,y,z,i) { z+=(w^x^y)+SHABLK(i)+0xCA62C1D6+ROL32(v,5); w=ROL32(w,30); }

#ifdef	_MINIMUM_BUILD_
Minimum_CSHA1::Minimum_CSHA1()
#else
CSHA1::CSHA1()
#endif
{
	m_block = (SHA1_WORKSPACE_BLOCK *)m_workspace;

	Reset();
}
#ifdef	_MINIMUM_BUILD_
Minimum_CSHA1::~Minimum_CSHA1()
#else
CSHA1::~CSHA1()
#endif
{
	// Reset();
}
#ifdef	_MINIMUM_BUILD_
void Minimum_CSHA1::Reset() 
#else
void CSHA1::Reset()
#endif
{
	// SHA1 initialization constants
	m_state[0] = 0x67452301;
	m_state[1] = 0xEFCDAB89;
	m_state[2] = 0x98BADCFE;
	m_state[3] = 0x10325476;
	m_state[4] = 0xC3D2E1F0;

	m_count[0] = 0;
	m_count[1] = 0;
}

#ifdef	_MINIMUM_BUILD_
void Minimum_CSHA1::Transform(unsigned long state[5], unsigned char buffer[64])
#else
void CSHA1::Transform(unsigned long state[5], unsigned char buffer[64])
#endif
{
	unsigned long a = 0, b = 0, c = 0, d = 0, e = 0;

	memcpy(m_block, buffer, 64);

	// Copy state[] to working vars
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];

	// 4 rounds of 20 operations each. Loop unrolled.
	_R0(a,b,c,d,e, 0); _R0(e,a,b,c,d, 1); _R0(d,e,a,b,c, 2); _R0(c,d,e,a,b, 3);
	_R0(b,c,d,e,a, 4); _R0(a,b,c,d,e, 5); _R0(e,a,b,c,d, 6); _R0(d,e,a,b,c, 7);
	_R0(c,d,e,a,b, 8); _R0(b,c,d,e,a, 9); _R0(a,b,c,d,e,10); _R0(e,a,b,c,d,11);
	_R0(d,e,a,b,c,12); _R0(c,d,e,a,b,13); _R0(b,c,d,e,a,14); _R0(a,b,c,d,e,15);
	_R1(e,a,b,c,d,16); _R1(d,e,a,b,c,17); _R1(c,d,e,a,b,18); _R1(b,c,d,e,a,19);
	_R2(a,b,c,d,e,20); _R2(e,a,b,c,d,21); _R2(d,e,a,b,c,22); _R2(c,d,e,a,b,23);
	_R2(b,c,d,e,a,24); _R2(a,b,c,d,e,25); _R2(e,a,b,c,d,26); _R2(d,e,a,b,c,27);
	_R2(c,d,e,a,b,28); _R2(b,c,d,e,a,29); _R2(a,b,c,d,e,30); _R2(e,a,b,c,d,31);
	_R2(d,e,a,b,c,32); _R2(c,d,e,a,b,33); _R2(b,c,d,e,a,34); _R2(a,b,c,d,e,35);
	_R2(e,a,b,c,d,36); _R2(d,e,a,b,c,37); _R2(c,d,e,a,b,38); _R2(b,c,d,e,a,39);
	_R3(a,b,c,d,e,40); _R3(e,a,b,c,d,41); _R3(d,e,a,b,c,42); _R3(c,d,e,a,b,43);
	_R3(b,c,d,e,a,44); _R3(a,b,c,d,e,45); _R3(e,a,b,c,d,46); _R3(d,e,a,b,c,47);
	_R3(c,d,e,a,b,48); _R3(b,c,d,e,a,49); _R3(a,b,c,d,e,50); _R3(e,a,b,c,d,51);
	_R3(d,e,a,b,c,52); _R3(c,d,e,a,b,53); _R3(b,c,d,e,a,54); _R3(a,b,c,d,e,55);
	_R3(e,a,b,c,d,56); _R3(d,e,a,b,c,57); _R3(c,d,e,a,b,58); _R3(b,c,d,e,a,59);
	_R4(a,b,c,d,e,60); _R4(e,a,b,c,d,61); _R4(d,e,a,b,c,62); _R4(c,d,e,a,b,63);
	_R4(b,c,d,e,a,64); _R4(a,b,c,d,e,65); _R4(e,a,b,c,d,66); _R4(d,e,a,b,c,67);
	_R4(c,d,e,a,b,68); _R4(b,c,d,e,a,69); _R4(a,b,c,d,e,70); _R4(e,a,b,c,d,71);
	_R4(d,e,a,b,c,72); _R4(c,d,e,a,b,73); _R4(b,c,d,e,a,74); _R4(a,b,c,d,e,75);
	_R4(e,a,b,c,d,76); _R4(d,e,a,b,c,77); _R4(c,d,e,a,b,78); _R4(b,c,d,e,a,79);

	// Add the working vars back into state[]
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;

	// Wipe variables
	a = b = c = d = e = 0;
}

// Use this function to hash in binary data and strings
#ifdef	_MINIMUM_BUILD_
void Minimum_CSHA1::Update(unsigned char *data, unsigned int len)
#else
void CSHA1::Update(unsigned char *data, unsigned int len)
#endif
{
	unsigned long i = 0, j;

	j = (m_count[0] >> 3) & 63;

	if((m_count[0] += len << 3) < (len << 3)) m_count[1]++;

	m_count[1] += (len >> 29);

	if((j + len) > 63)
	{
		memcpy(&m_buffer[j], data, (i = 64 - j));
		Transform(m_state, m_buffer);

		for (; i+63 < len; i += 64)
			Transform(m_state, &data[i]);

		j = 0;
	}
	else i = 0;

	memcpy(&m_buffer[j], &data[i], len - i);
}

#if !defined(_MINIMUM_BUILD_)
// Hash in file contents
bool CSHA1::HashFile(char *szFileName)
{
	unsigned long ulFileSize = 0, ulRest = 0, ulBlocks = 0;
	unsigned long i = 0;
	unsigned char uData[MAX_FILE_READ_BUFFER];
	FILE *fIn = NULL;

	if(szFileName == NULL) return(false);

	if((fIn = fopen(szFileName, "rb")) == NULL) return(false);

	fseek(fIn, 0, SEEK_END);
	ulFileSize = ftell(fIn);
	fseek(fIn, 0, SEEK_SET);

	ulRest = ulFileSize % MAX_FILE_READ_BUFFER;
	ulBlocks = ulFileSize / MAX_FILE_READ_BUFFER;

	for(i = 0; i < ulBlocks; i++)
	{
		fread(uData, 1, MAX_FILE_READ_BUFFER, fIn);
		Update(uData, MAX_FILE_READ_BUFFER);
	}

	if(ulRest != 0)
	{
		fread(uData, 1, ulRest, fIn);
		Update(uData, ulRest);
	}

	fclose(fIn);
	fIn = NULL;

	return(true);
}
#endif

#ifdef	_MINIMUM_BUILD_
void Minimum_CSHA1::Final()
#else
void CSHA1::Final()
#endif
{
	unsigned long i = 0;
	unsigned char finalcount[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	for (i = 0; i < 8; i++)
		finalcount[i] = (unsigned char)((m_count[(i >= 4 ? 0 : 1)]
			>> ((3 - (i & 3)) * 8) ) & 255); // Endian independent

	Update((unsigned char *)"\200", 1);

	while ((m_count[0] & 504) != 448)
		Update((unsigned char *)"\0", 1);

	Update(finalcount, 8); // Cause a SHA1Transform()

	for (i = 0; i < k_cubHash; i++)
	{
		m_digest[i] = (unsigned char)((m_state[i >> 2] >> ((3 - (i & 3)) * 8) ) & 255);
	}

	// Wipe variables for security reasons
	i = 0;
	memset(m_buffer, 0, sizeof(m_buffer) );
	memset(m_state, 0, sizeof(m_state) );
	memset(m_count, 0, sizeof(m_count) );
	memset(finalcount, 0, sizeof( finalcount) );

	Transform(m_state, m_buffer);
}

#if !defined(_MINIMUM_BUILD_)
// Get the final hash as a pre-formatted string
void CSHA1::ReportHash(char *szReport, unsigned char uReportType)
{
	unsigned char i = 0;
	char szTemp[12];

	if(szReport == NULL) return;

	if(uReportType == REPORT_HEX)
	{
		sprintf(szTemp, "%02X", m_digest[0]);
		strcat(szReport, szTemp);

		for(i = 1; i < k_cubHash; i++)
		{
			sprintf(szTemp, " %02X", m_digest[i]);
			strcat(szReport, szTemp);
		}
	}
	else if(uReportType == REPORT_DIGIT)
	{
		sprintf(szTemp, "%u", m_digest[0]);
		strcat(szReport, szTemp);

		for(i = 1; i < k_cubHash; i++)
		{
			sprintf(szTemp, " %u", m_digest[i]);
			strcat(szReport, szTemp);
		}
	}
	else strcpy(szReport, "Error: Unknown report type!");
}
#endif // _MINIMUM_BUILD_

// Get the raw message digest
#ifdef	_MINIMUM_BUILD_
void Minimum_CSHA1::GetHash(unsigned char *uDest)
#else
void CSHA1::GetHash(unsigned char *uDest)
#endif
{
	memcpy(uDest, m_digest, k_cubHash);
}

#ifndef	_MINIMUM_BUILD_
// utility hash comparison function
bool HashLessFunc( SHADigest_t const &lhs, SHADigest_t const &rhs )
{
	int iRes = memcmp( &lhs, &rhs, sizeof( SHADigest_t ) );
	return ( iRes < 0 );
}
#endif
