//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "bspfile.h"
#include "strtools.h"
#include "filesystem.h"
#include "lumpfiles.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Generate a lump file name for a given bsp & index
// Input  : *bspfilename - 
//			*lumpfilename - 
//			iIndex - 
//-----------------------------------------------------------------------------
void GenerateLumpFileName( const char *bspfilename, char *lumpfilename, int iBufferSize, int iIndex )
{
	char lumppre[MAX_PATH];
	V_StripExtension( bspfilename, lumppre, MAX_PATH );
	Q_snprintf( lumpfilename, iBufferSize, "%s_l_%d.lmp", lumppre, iIndex );
}

