//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Cryptographic hash agility helper definitions
//
//=============================================================================

#ifndef PASSWORDHASH_H
#define PASSWORDHASH_H

#if defined( _WIN32 )
#pragma once
#endif


#include "checksum_sha1.h"

typedef unsigned char BigPasswordHash_t[32];
typedef unsigned char PBKDF2Hash_t[32];

//
// A union of all the possible password hash types.
// When adding to this, if the maximum size of the
// structure has changed then the Account table and
// the AccountSecurityHistory table need to be manually
// revised using something like the following SQL:
//
// ALTER TABLE Account ALTER COLUMN PasswordHash binary(32)
// ALTER TABLE AccountSecurityHistory ALTER COLUMN PasswordHashOld binary(32)
// ALTER TABLE AccountSecurityHistory ALTER COLUMN PasswordHashNew binary(32)
//
// Replace 32 with the new size of PasswordHash_t.
//
// If the width of those columns does not match sizeof(PasswordHash_t), many
// database operations will fail.
//
// Note that while this query was correct at the time this code was checked in,
// it may not be sufficient at the time you actually change the size of the
// type, so look around.
//
typedef union
{
	SHADigest_t sha;
	BigPasswordHash_t bigpassword;
	PBKDF2Hash_t pbkdf2;
} PasswordHash_t;


//
// Enum of all available password hash algorithms. These should
// be consecutive and ordinals should never be reused.
//
// k_EHashSHA1: A salted SHA-1 hash, width 20 bytes.
// k_EHashBigPassword: For testing purposes, a salted SHA-1 hash extended to 32 bytes, with 6 bytes of 0x1 on either side.
// k_EHashPBKDF2_1000: A PKCS#5 v2.0 Password-Based Key Derivation Function hash (PBKDF2-HMAC-SHA256) with 1,000 iterations.
//						The digest width is 32 bytes.
// k_EHashPBKDF2_5000: A PKCS#5 v2.0 Password-Based Key Derivation Function hash (PBKDF2-HMAC-SHA256) with 5,000 iterations.
// k_EHashPBKDF2_10000: A PKCS#5 v2.0 Password-Based Key Derivation Function hash (PBKDF2-HMAC-SHA256) with 10,000 iterations.
// k_EHashSHA1WrappedWithPBKDF2_10000: A SHA-1 hash which is then further hashed with PBKDF2 at 10,000 rounds.  Used for
//										strengthening old hashes in the database that haven't been logged in in a long time.
//
// Make sure to update k_EHashMax when adding new hash types.  Also add the length into the k_HashLengths array below.
enum EPasswordHashAlg
{
	k_EHashSHA1 = 0,
	k_EHashBigPassword = 1,
	k_EHashPBKDF2_1000 = 2,
	k_EHashPBKDF2_5000 = 3,
	k_EHashPBKDF2_10000 = 4,
	k_EHashSHA1WrappedWithPBKDF2_10000 = 5,
	k_EHashMax = 5,
};

//
// Hash sizes for the various available hash algorithms,
// indexed by EPasswordHashAlg.
const size_t k_HashLengths[] = {
	sizeof(SHADigest_t),
	sizeof(BigPasswordHash_t),
	sizeof(PBKDF2Hash_t),
	sizeof(PBKDF2Hash_t),
	sizeof(PBKDF2Hash_t),
	sizeof(PBKDF2Hash_t),
};

#if defined(C_ASSERT)
//
// If you're hitting this assert at compile time, it means that you've added a new
// hash type and properly updated k_EHashMax, but you forgot to add the length
// of the new hash type into k_HashLengths.  So do that.
//
C_ASSERT( ( ( sizeof(k_HashLengths) / sizeof(size_t) ) == k_EHashMax + 1 ) );
#endif

#endif // PASSWORDHASH_H
