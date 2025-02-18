//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vrad.h"
#include "lightmap.h"

#define SAMPLEHASH_NUM_BUCKETS			65536
#define SAMPLEHASH_GROW_SIZE			0
#define SAMPLEHASH_INIT_SIZE			0

int samplesAdded = 0;
int patchSamplesAdded = 0;
static unsigned short g_PatchIterationKey = 0;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool SampleData_CompareFunc( SampleData_t const &src1, SampleData_t const &src2 )
{
	return ( ( src1.x == src2.x ) &&
		     ( src1.y == src2.y ) &&
			 ( src1.z == src2.z ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int SampleData_KeyFunc( SampleData_t const &src )
{
	return ( src.x + src.y + src.z );
}


CUtlHash<SampleData_t> g_SampleHashTable( SAMPLEHASH_NUM_BUCKETS, 
										  SAMPLEHASH_GROW_SIZE, 
										  SAMPLEHASH_INIT_SIZE, 
										  SampleData_CompareFunc, SampleData_KeyFunc );



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
UtlHashHandle_t SampleData_Find( sample_t *pSample )
{
	SampleData_t sampleData;	
	sampleData.x = ( int )( pSample->pos.x / SAMPLEHASH_VOXEL_SIZE ) * 100;
	sampleData.y = ( int )( pSample->pos.y / SAMPLEHASH_VOXEL_SIZE ) * 10;
	sampleData.z = ( int )( pSample->pos.z / SAMPLEHASH_VOXEL_SIZE );

	return g_SampleHashTable.Find( sampleData );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
UtlHashHandle_t SampleData_InsertIntoHashTable( sample_t *pSample, SampleHandle_t sampleHandle )
{
	SampleData_t sampleData;
	sampleData.x = ( int )( pSample->pos.x / SAMPLEHASH_VOXEL_SIZE ) * 100;
	sampleData.y = ( int )( pSample->pos.y / SAMPLEHASH_VOXEL_SIZE ) * 10;
	sampleData.z = ( int )( pSample->pos.z / SAMPLEHASH_VOXEL_SIZE );

	UtlHashHandle_t handle = g_SampleHashTable.AllocEntryFromKey( sampleData );

	SampleData_t *pSampleData = &g_SampleHashTable.Element( handle );
	pSampleData->x = sampleData.x;
	pSampleData->y = sampleData.y;
	pSampleData->z = sampleData.z;
	pSampleData->m_Samples.AddToTail( sampleHandle );

	samplesAdded++;

	return handle;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
UtlHashHandle_t SampleData_AddSample( sample_t *pSample, SampleHandle_t sampleHandle )
{

	// find the key -- if it doesn't exist add new sample data to the
	// hash table
	UtlHashHandle_t handle = SampleData_Find( pSample );
	if( handle == g_SampleHashTable.InvalidHandle() )
	{
		handle = SampleData_InsertIntoHashTable( pSample, sampleHandle );
	}
	else
	{
		SampleData_t *pSampleData = &g_SampleHashTable.Element( handle );
		pSampleData->m_Samples.AddToTail( sampleHandle );

		samplesAdded++;
	}

	return handle;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void SampleData_Log( void )
{
	if( g_bLogHashData )
	{
		g_SampleHashTable.Log( "samplehash.txt" );
	}
}


//=============================================================================
//=============================================================================
//
// PatchSample Functions
//
//=============================================================================
//=============================================================================

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool PatchSampleData_CompareFunc( PatchSampleData_t const &src1, PatchSampleData_t const &src2 )
{
	return ( ( src1.x == src2.x ) &&
		     ( src1.y == src2.y ) &&
			 ( src1.z == src2.z ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int PatchSampleData_KeyFunc( PatchSampleData_t const &src )
{
	return ( src.x + src.y + src.z );
}


CUtlHash<PatchSampleData_t>	g_PatchSampleHashTable( SAMPLEHASH_NUM_BUCKETS,
												    SAMPLEHASH_GROW_SIZE,
													SAMPLEHASH_INIT_SIZE,
													PatchSampleData_CompareFunc, PatchSampleData_KeyFunc );

void GetPatchSampleHashXYZ( const Vector &vOrigin, int &x, int &y, int &z )
{
	x = ( int )( vOrigin.x / SAMPLEHASH_VOXEL_SIZE );
	y = ( int )( vOrigin.y / SAMPLEHASH_VOXEL_SIZE );
	z = ( int )( vOrigin.z / SAMPLEHASH_VOXEL_SIZE );
}


unsigned short IncrementPatchIterationKey()
{
	if ( g_PatchIterationKey == 0xFFFF )
	{
		g_PatchIterationKey = 1;
		for ( int i=0; i < g_Patches.Count(); i++ )
			g_Patches[i].m_IterationKey = 0;
	}
	else
	{
		g_PatchIterationKey++;
	}
	return g_PatchIterationKey;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PatchSampleData_AddSample( CPatch *pPatch, int ndxPatch )
{
	int patchSampleMins[3], patchSampleMaxs[3];

#if defined( SAMPLEHASH_USE_AREA_PATCHES )
	GetPatchSampleHashXYZ( pPatch->mins, patchSampleMins[0], patchSampleMins[1], patchSampleMins[2] );
	GetPatchSampleHashXYZ( pPatch->maxs, patchSampleMaxs[0], patchSampleMaxs[1], patchSampleMaxs[2] );
#else
	// If not using area patches, just use the patch's origin to add it to the voxels.
	GetPatchSampleHashXYZ( pPatch->origin, patchSampleMins[0], patchSampleMins[1], patchSampleMins[2] );
	memcpy( patchSampleMaxs, patchSampleMins, sizeof( patchSampleMaxs ) );
#endif
	
	// Make sure mins are smaller than maxs so we don't iterate for 4 bil.
	Assert( patchSampleMins[0] <= patchSampleMaxs[0] && patchSampleMins[1] <= patchSampleMaxs[1] && patchSampleMins[2] <= patchSampleMaxs[2] );
	patchSampleMins[0] = min( patchSampleMins[0], patchSampleMaxs[0] );
	patchSampleMins[1] = min( patchSampleMins[1], patchSampleMaxs[1] );
	patchSampleMins[2] = min( patchSampleMins[2], patchSampleMaxs[2] );
	
	int iterateCoords[3];
	for ( iterateCoords[0]=patchSampleMins[0]; iterateCoords[0] <= patchSampleMaxs[0]; iterateCoords[0]++ )
	{
		for ( iterateCoords[1]=patchSampleMins[1]; iterateCoords[1] <= patchSampleMaxs[1]; iterateCoords[1]++ )
		{
			for ( iterateCoords[2]=patchSampleMins[2]; iterateCoords[2] <= patchSampleMaxs[2]; iterateCoords[2]++ )
			{
				// find the key -- if it doesn't exist add new sample data to the
				// hash table
				PatchSampleData_t iteratePatch;
				iteratePatch.x = iterateCoords[0] * 100;
				iteratePatch.y = iterateCoords[1] * 10;
				iteratePatch.z = iterateCoords[2];

				UtlHashHandle_t handle = g_PatchSampleHashTable.Find( iteratePatch );
				if( handle == g_PatchSampleHashTable.InvalidHandle() )
				{
					UtlHashHandle_t handle = g_PatchSampleHashTable.AllocEntryFromKey( iteratePatch );

					PatchSampleData_t *pPatchData = &g_PatchSampleHashTable.Element( handle );
					pPatchData->x = iteratePatch.x;
					pPatchData->y = iteratePatch.y;
					pPatchData->z = iteratePatch.z;
					pPatchData->m_ndxPatches.AddToTail( ndxPatch );

					patchSamplesAdded++;
				}
				else
				{
					PatchSampleData_t *pPatchData = &g_PatchSampleHashTable.Element( handle );
					pPatchData->m_ndxPatches.AddToTail( ndxPatch );

					patchSamplesAdded++;
				}
			}
		}
	}
}

