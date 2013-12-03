//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "bsplib.h"

// input:
// from bsplib.h:
//		numleafs
//		dleafs

void EmitDistanceToWaterInfo( void )
{
	int leafID;
	for( leafID = 0; leafID < numleafs; leafID++ )
	{
		dleaf_t *pLeaf = &dleafs[leafID];
		if( pLeaf->leafWaterDataID == -1 )
		{
			// FIXME: set the distance to water to infinity here just in case.
			continue;
		}

		// Get the vis set for this leaf.
		
	}
}

