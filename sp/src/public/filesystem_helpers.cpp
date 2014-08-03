//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "filesystem.h"
#include "filesystem_helpers.h"
#include "characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// wordbreak parsing set
static characterset_t	g_BreakSet, g_BreakSetIncludingColons;

static void InitializeCharacterSets()
{
	static bool s_CharacterSetInitialized = false;
	if (!s_CharacterSetInitialized)
	{
		CharacterSetBuild( &g_BreakSet, "{}()'" );
		CharacterSetBuild( &g_BreakSetIncludingColons, "{}()':" );
		s_CharacterSetInitialized = true;
	}
}


const char* ParseFileInternal( const char* pFileBytes, char* pTokenOut, bool* pWasQuoted, characterset_t *pCharSet, size_t nMaxTokenLen )
{
	pTokenOut[0] = 0;

	if (pWasQuoted)
		*pWasQuoted = false;

	if (!pFileBytes)
		return 0;

	InitializeCharacterSets();

	// YWB:  Ignore colons as token separators in COM_Parse
	static bool com_ignorecolons = false;  
	characterset_t& breaks = pCharSet ? *pCharSet : (com_ignorecolons ? g_BreakSet : g_BreakSetIncludingColons);
	
	int c;
	unsigned int len = 0;
	
// skip whitespace
skipwhite:

	while ( (c = *pFileBytes) <= ' ')
	{
		if (c == 0)
			return 0;                    // end of file;
		pFileBytes++;
	}
	
// skip // comments
	if (c=='/' && pFileBytes[1] == '/')
	{
		while (*pFileBytes && *pFileBytes != '\n')
			pFileBytes++;
		goto skipwhite;
	}
	
// skip c-style comments
	if (c=='/' && pFileBytes[1] == '*' )
	{
		// Skip "/*"
		pFileBytes += 2;

		while ( *pFileBytes  )
		{
			if ( *pFileBytes == '*' &&
				 pFileBytes[1] == '/' )
			{
				pFileBytes += 2;
				break;
			}

			pFileBytes++;
		}

		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		if (pWasQuoted)
			*pWasQuoted = true;

		pFileBytes++;
		while (1)
		{
			c = *pFileBytes++;
			if (c=='\"' || !c)
			{
				pTokenOut[len] = 0;
				return pFileBytes;
			}
			pTokenOut[len] = c;
			len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		}
	}

// parse single characters
	if ( IN_CHARACTERSET( breaks, c ) )
	{
		pTokenOut[len] = c;
		len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		pTokenOut[len] = 0;
		return pFileBytes+1;
	}

// parse a regular word
	do
	{
		pTokenOut[len] = c;
		pFileBytes++;
		len += ( len < nMaxTokenLen-1 ) ? 1 : 0;
		c = *pFileBytes;
		if ( IN_CHARACTERSET( breaks, c ) )
			break;
	} while (c>32);
	
	pTokenOut[len] = 0;
	return pFileBytes;
}
