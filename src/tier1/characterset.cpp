//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include <string.h>
#include "characterset.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: builds a simple lookup table of a group of important characters
// Input  : *pParseGroup - pointer to the buffer for the group
//			*pGroupString - null terminated list of characters to flag
//-----------------------------------------------------------------------------
void CharacterSetBuild( characterset_t *pSetBuffer, const char *pszSetString )
{
	int i = 0;

	// Test our pointers
	if ( !pSetBuffer || !pszSetString )
		return;

	memset( pSetBuffer->set, 0, sizeof(pSetBuffer->set) );

	while ( pszSetString[i] )
	{
		pSetBuffer->set[ (unsigned)pszSetString[i] ] = 1;
		i++;
	}

}
