//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef FILESYSTEM_HELPERS_H
#define FILESYSTEM_HELPERS_H

#ifdef _WIN32
#pragma once
#endif

struct characterset_t;

// Don't call this directly. You should (as long as your destination is an array) be
// able to call ParseFile, which is safer as it infers your array size for you.
const char* ParseFileInternal( const char* pFileBytes, OUT_Z_CAP(nMaxTokenLen) char* pTokenOut, bool* pWasQuoted, characterset_t *pCharSet, size_t nMaxTokenLen );

// Call until it returns NULL. Each time you call it, it will parse out a token.

template <size_t count>
const char* ParseFile( const char* pFileBytes, OUT_Z_ARRAY char (&pTokenOut)[count], bool* pWasQuoted, characterset_t *pCharSet = NULL, unsigned int nMaxTokenLen = (unsigned int)-1 )
{
	(void)nMaxTokenLen; // Avoid unreferenced variable warnings.
	return ParseFileInternal( pFileBytes, pTokenOut, pWasQuoted, pCharSet, count );
}

template <size_t count>
char* ParseFile( char* pFileBytes, OUT_Z_ARRAY char (&pTokenOut)[count], bool* pWasQuoted )	// (same exact thing as the const version)
{
	return const_cast<char*>( ParseFileInternal( pFileBytes, pTokenOut, pWasQuoted, NULL, count ) );
}

#endif // FILESYSTEM_HELPERS_H
