//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vrad.h"
#include "vmpi.h"
#ifdef MPI
#include "messbuf.h"
static MessageBuffer mb;
#endif

#define	HALFBIT

extern char		source[MAX_PATH];
extern char		vismatfile[_MAX_PATH];
extern char		incrementfile[_MAX_PATH];
extern qboolean	incremental;

/*
===================================================================

VISIBILITY MATRIX

Determine which patches can see each other
Use the PVS to accelerate if available
===================================================================
*/

#define TEST_EPSILON	0.1
#define PLANE_TEST_EPSILON  0.01 // patch must be this much in front of the plane to be considered "in front"
#define PATCH_FACE_OFFSET  0.1 // push patch origins off from the face by this amount to avoid self collisions

#define STREAM_SIZE 512

class CTransferMaker
{
public:

	CTransferMaker( transfer_t *all_transfers );
	~CTransferMaker();

	FORCEINLINE void TestMakeTransfer( Vector start, Vector stop, int ndxShooter, int ndxReciever )
	{
		g_RtEnv.AddToRayStream( m_RayStream, start, stop, &m_pResults[m_nTests] );
		m_pShooterPatches[m_nTests] = ndxShooter;
		m_pRecieverPatches[m_nTests] = ndxReciever;
		++m_nTests;
	}

	void Finish();

private:

	int m_nTests;
	RayTracingSingleResult *m_pResults;
	int *m_pShooterPatches;
	int *m_pRecieverPatches;
	RayStream m_RayStream;
	transfer_t *m_AllTransfers;
};

CTransferMaker::CTransferMaker( transfer_t *all_transfers ) :
	m_AllTransfers( all_transfers ), m_nTests( 0 )
{
	m_pResults = (RayTracingSingleResult *)calloc( 1, MAX_PATCHES * sizeof ( RayTracingSingleResult ) );
	m_pShooterPatches = (int *)calloc( 1, MAX_PATCHES * sizeof( int ) );
	m_pRecieverPatches = (int *)calloc( 1, MAX_PATCHES * sizeof( int ) );
}

CTransferMaker::~CTransferMaker()
{
	free ( m_pResults );
	free ( m_pShooterPatches );
	free (m_pRecieverPatches );
}

void CTransferMaker::Finish()
{
	g_RtEnv.FinishRayStream( m_RayStream );
	for ( int i = 0; i < m_nTests; ++i )
	{
		if ( m_pResults[i].HitID == -1 || m_pResults[i].HitDistance >= m_pResults[i].ray_length )
		{
			MakeTransfer( m_pShooterPatches[i], m_pRecieverPatches[i], m_AllTransfers );
		}
	}
	m_nTests = 0;
}


dleaf_t* PointInLeaf (int iNode, Vector const& point)
{
	if ( iNode < 0 )
		return &dleafs[ (-1-iNode) ];

	dnode_t *node = &dnodes[iNode];
	dplane_t *plane = &dplanes[ node->planenum ];

	float dist = DotProduct (point, plane->normal) - plane->dist;
	if ( dist > TEST_EPSILON )
	{
		return PointInLeaf( node->children[0], point );
	}
	else if ( dist < -TEST_EPSILON )
	{
		return PointInLeaf( node->children[1], point );
	}
	else
	{
		dleaf_t *pTest = PointInLeaf( node->children[0], point );
		if ( pTest->cluster != -1 )
			return pTest;

		return PointInLeaf( node->children[1], point );
	}
}


int ClusterFromPoint( Vector const& point )
{
	dleaf_t *leaf = PointInLeaf( 0, point );
	
	return leaf->cluster;
}

void PvsForOrigin (Vector& org, byte *pvs)
{
	int visofs;
	int cluster;

	if (!visdatasize)
	{
		memset (pvs, 255, (dvis->numclusters+7)/8 );
		return;
	}

	cluster = ClusterFromPoint( org );
	if ( cluster < 0 )
	{
		visofs = -1;
	}
	else
	{
		visofs = dvis->bitofs[ cluster ][DVIS_PVS];
	}

	if (visofs == -1)
		Error ("visofs == -1");

	DecompressVis (&dvisdata[visofs], pvs);
}


void TestPatchToPatch( int ndxPatch1, int ndxPatch2, int head, transfer_t *transfers, CTransferMaker &transferMaker, int iThread )
{
	Vector tmp;

	//
	// get patches
	//
	if( ndxPatch1 == g_Patches.InvalidIndex() || ndxPatch2 == g_Patches.InvalidIndex() )
		return;

	CPatch *patch = &g_Patches.Element( ndxPatch1 );
	CPatch *patch2 = &g_Patches.Element( ndxPatch2 );

	if (patch2->child1 != g_Patches.InvalidIndex() )
	{
		// check to see if we should use a child node instead

		VectorSubtract( patch->origin, patch2->origin, tmp );
		// SQRT( 1/4 )
		// FIXME: should be based on form-factor (ie. include visible angle, etc)
		if ( DotProduct(tmp, tmp) * 0.0625 < patch2->area )
		{
			TestPatchToPatch( ndxPatch1, patch2->child1, head, transfers, transferMaker, iThread );
			TestPatchToPatch( ndxPatch1, patch2->child2, head, transfers, transferMaker, iThread );
			return;
		}
	}

	// check vis between patch and patch2
	// if bit has not already been set
	//  && v2 is not behind light plane
	//  && v2 is visible from v1
	if ( DotProduct( patch2->origin, patch->normal ) > patch->planeDist + PLANE_TEST_EPSILON )
	{
		// push out origins from face so that don't intersect their owners
		Vector p1, p2;
		VectorAdd( patch->origin, patch->normal, p1 );
		VectorAdd( patch2->origin, patch2->normal, p2 );
		transferMaker.TestMakeTransfer( p1, p2, ndxPatch1, ndxPatch2 );
	}
}


/*
==============
TestPatchToFace

Sets vis bits for all patches in the face
==============
*/
void TestPatchToFace (unsigned patchnum, int facenum, int head, transfer_t *transfers, CTransferMaker &transferMaker, int iThread )
{
	if( faceParents.Element( facenum ) == g_Patches.InvalidIndex() || patchnum == g_Patches.InvalidIndex() )
		return;

	CPatch	*patch = &g_Patches.Element( patchnum );
	CPatch *patch2 = &g_Patches.Element( faceParents.Element( facenum ) );

	// if emitter is behind that face plane, skip all patches

	CPatch *pNextPatch;

	if ( patch2 && DotProduct(patch->origin, patch2->normal) > patch2->planeDist + PLANE_TEST_EPSILON )
	{
		// we need to do a real test
		for( ; patch2; patch2 = pNextPatch )
		{
			// next patch
			pNextPatch = NULL;
			if( patch2->ndxNextParent != g_Patches.InvalidIndex() )
			{
				pNextPatch = &g_Patches.Element( patch2->ndxNextParent );
			}

			/*
			// skip patches too far away
			VectorSubtract( patch->origin, patch2->origin, tmp );
			if (DotProduct( tmp, tmp ) > 512 * 512)
				continue;
			*/

			int ndxPatch2 = patch2 - g_Patches.Base();
			TestPatchToPatch( patchnum, ndxPatch2, head, transfers, transferMaker, iThread );
		}
	}
}


struct ClusterDispList_t
{
	CUtlVector<int>	dispFaces;
};

static CUtlVector<ClusterDispList_t> g_ClusterDispFaces;

//-----------------------------------------------------------------------------
// Helps us find all displacements associated with a particular cluster
//-----------------------------------------------------------------------------
void AddDispsToClusterTable( void )
{
	g_ClusterDispFaces.SetCount( g_ClusterLeaves.Count() );

	//
	// add displacement faces to the cluster table
	//
	for( int ndxFace = 0; ndxFace < numfaces; ndxFace++ )
	{
		// search for displacement faces
		if( g_pFaces[ndxFace].dispinfo == -1 )
			continue;

		//
		// get the clusters associated with the face
		//
		if( g_FacePatches.Element( ndxFace ) != g_FacePatches.InvalidIndex() )
		{
			CPatch *pNextPatch = NULL;
			for( CPatch *pPatch = &g_Patches.Element( g_FacePatches.Element( ndxFace ) ); pPatch; pPatch = pNextPatch )
			{
				// next patch
				pNextPatch = NULL;
				if( pPatch->ndxNext != g_Patches.InvalidIndex() )
				{
					pNextPatch = &g_Patches.Element( pPatch->ndxNext );
				}

				if( pPatch->clusterNumber != g_Patches.InvalidIndex() )
				{
					int ndxDisp = g_ClusterDispFaces[pPatch->clusterNumber].dispFaces.Find( ndxFace );
					if( ndxDisp == -1 )
					{
						ndxDisp = g_ClusterDispFaces[pPatch->clusterNumber].dispFaces.AddToTail();
						g_ClusterDispFaces[pPatch->clusterNumber].dispFaces[ndxDisp] = ndxFace;
					}
				}
			}
		}
	}
}


/*
==============
BuildVisRow

Calc vis bits from a single patch
==============
*/
void BuildVisRow (int patchnum, byte *pvs, int head, transfer_t *transfers, CTransferMaker &transferMaker, int iThread )
{
	int		j, k, l, leafIndex;
	CPatch	*patch;
	dleaf_t	*leaf;
	byte	face_tested[MAX_MAP_FACES];
	byte	disp_tested[MAX_MAP_FACES];

	patch = &g_Patches.Element( patchnum );

	memset( face_tested, 0, numfaces ) ;
	memset( disp_tested, 0, numfaces );

	for (j=0; j<dvis->numclusters; j++)
	{
		if ( ! ( pvs[(j)>>3] & (1<<((j)&7)) ) )
		{
			continue;		// not in pvs
		}

		for ( leafIndex = 0; leafIndex < g_ClusterLeaves[j].leafCount; leafIndex++ )
		{
			leaf = dleafs + g_ClusterLeaves[j].leafs[leafIndex];

			for (k=0 ; k<leaf->numleaffaces; k++)
			{
				l = dleaffaces[leaf->firstleafface + k];
				// faces can be marksurfed by multiple leaves, but
				// don't bother testing again
				if (face_tested[l])
				{
					continue;
				}
				face_tested[l] = 1;

				// don't check patches on the same face
				if (patch->faceNumber == l)
					continue;
				TestPatchToFace (patchnum, l, head, transfers, transferMaker, iThread );
			}
		}

		int dispCount = g_ClusterDispFaces[j].dispFaces.Size();
		for( int ndxDisp = 0; ndxDisp < dispCount; ndxDisp++ )
		{
			int ndxFace = g_ClusterDispFaces[j].dispFaces[ndxDisp];
			if( disp_tested[ndxFace] )
				continue;

			disp_tested[ndxFace] = 1;

			// don't check patches on the same face
			if( patch->faceNumber == ndxFace )
				continue;

			TestPatchToFace( patchnum, ndxFace, head, transfers, transferMaker, iThread );
		}
	}


	// Msg("%d) Transfers: %5d\n", patchnum, patch->numtransfers);
}



/*
===========
BuildVisLeafs

  This is run by multiple threads
===========
*/

transfer_t* BuildVisLeafs_Start()
{
	return (transfer_t *)calloc( 1,  MAX_PATCHES * sizeof( transfer_t ) );
}


// If PatchCB is non-null, it is called after each row is generated (used by MPI).
void BuildVisLeafs_Cluster( 
	int threadnum,
	transfer_t *transfers, 
	int iCluster, 
	void (*PatchCB)(int iThread, int patchnum, CPatch *patch)
	)
{
	byte	pvs[(MAX_MAP_CLUSTERS+7)/8];
	CPatch	*patch;
	int		head;
	unsigned	patchnum;
	
	DecompressVis( &dvisdata[ dvis->bitofs[ iCluster ][DVIS_PVS] ], pvs);
	head = 0;

	CTransferMaker transferMaker( transfers );

	// light every patch in the cluster
	if( clusterChildren.Element( iCluster ) != clusterChildren.InvalidIndex() )
	{
		CPatch *pNextPatch;
		for( patch = &g_Patches.Element( clusterChildren.Element( iCluster ) ); patch; patch = pNextPatch )
		{
			//
			// next patch
			//
			pNextPatch = NULL;
			if( patch->ndxNextClusterChild != g_Patches.InvalidIndex() )
			{
				pNextPatch = &g_Patches.Element( patch->ndxNextClusterChild );
			}
			
			patchnum = patch - g_Patches.Base();

			// build to all other world clusters
			BuildVisRow (patchnum, pvs, head, transfers, transferMaker, threadnum );
			transferMaker.Finish();
			
			// do the transfers
			MakeScales( patchnum, transfers );

			// Let MPI aggregate the data if it's being used.
			if ( PatchCB )
				PatchCB( threadnum, patchnum, patch );
		}
	}
}


void BuildVisLeafs_End( transfer_t *transfers )
{
	free( transfers );
}


void BuildVisLeafs( int threadnum, void *pUserData )
{
	transfer_t *transfers = BuildVisLeafs_Start();
	
	while ( 1 )
	{
		//
		// build a minimal BSP tree that only
		// covers areas relevent to the PVS
		//
		// JAY: Now this returns a cluster index
		int iCluster = GetThreadWork();
		if ( iCluster == -1 )
			break;

		BuildVisLeafs_Cluster( threadnum, transfers, iCluster, NULL );
	}
	
	BuildVisLeafs_End( transfers );
}


/*
==============
BuildVisMatrix
==============
*/
void BuildVisMatrix (void)
{
	if ( g_bUseMPI )
	{
		RunMPIBuildVisLeafs();
	}
	else 
	{
		RunThreadsOn (dvis->numclusters, true, BuildVisLeafs);
	}
}

void FreeVisMatrix (void)
{

}
