//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "vbsp.h"
#include "disp_vbsp.h"
#include "builddisp.h"
#include "disp_common.h"
#include "ivp.h"
#include "disp_ivp.h"
#include "vphysics_interface.h"
#include "vphysics/virtualmesh.h"
#include "utlrbtree.h"
#include "tier1/utlbuffer.h"
#include "materialpatch.h"

struct disp_grid_t
{
	int				gridIndex;
	CUtlVector<int> dispList;
};

static CUtlVector<disp_grid_t> gDispGridList;



disp_grid_t &FindOrInsertGrid( int gridIndex )
{
	// linear search is slow, but only a few grids will be present
	for ( int i = gDispGridList.Count()-1; i >= 0; i-- )
	{
		if ( gDispGridList[i].gridIndex == gridIndex )
		{
			return gDispGridList[i];
		}
	}
	int index = gDispGridList.AddToTail();
	gDispGridList[index].gridIndex = gridIndex;

	// must be empty
	Assert( gDispGridList[index].dispList.Count() == 0 );

	return gDispGridList[index];
}

// UNDONE: Tune these or adapt them to map size or triangle count?
#define DISP_GRID_SIZEX	4096
#define DISP_GRID_SIZEY	4096
#define DISP_GRID_SIZEZ 8192

int Disp_GridIndex( CCoreDispInfo *pDispInfo )
{
	// quick hash the center into the grid and put the whole terrain in that grid
	Vector mins, maxs;
	pDispInfo->GetNode(0)->GetBoundingBox( mins, maxs );
	Vector center;
	center = 0.5 * (mins + maxs);
	// make sure it's positive
	center += Vector(MAX_COORD_INTEGER,MAX_COORD_INTEGER,MAX_COORD_INTEGER);
	int gridX = center.x / DISP_GRID_SIZEX;
	int gridY = center.y / DISP_GRID_SIZEY;
	int gridZ = center.z / DISP_GRID_SIZEZ;

	gridX &= 0xFF;
	gridY &= 0xFF;
	gridZ &= 0xFF;
	return MAKEID( gridX, gridY, gridZ, 0 );
}

void AddToGrid( int gridIndex, int dispIndex )
{
	disp_grid_t &grid = FindOrInsertGrid( gridIndex );
	grid.dispList.AddToTail( dispIndex );
}

MaterialSystemMaterial_t GetMatIDFromDisp( mapdispinfo_t *pMapDisp )
{
	texinfo_t *pTexInfo = &texinfo[pMapDisp->face.texinfo];
	dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
	MaterialSystemMaterial_t matID = FindOriginalMaterial( TexDataStringTable_GetString( pTexData->nameStringTableID ), NULL, true );
	return matID;
}

// check this and disable virtual mesh if in use
bool Disp_HasPower4Displacements()
{
	for ( int i = 0; i < g_CoreDispInfos.Count(); i++ )
	{
		if ( g_CoreDispInfos[i]->GetPower() > 3 )
		{
			return true;
		}
	}
	return false;
}

// adds all displacement faces as a series of convex objects
// UNDONE: Only add the displacements for this model?
void Disp_AddCollisionModels( CUtlVector<CPhysCollisionEntry *> &collisionList, dmodel_t *pModel, int contentsMask)
{
	int dispIndex;

	// Add each displacement to the grid hash
	for ( dispIndex = 0; dispIndex < g_CoreDispInfos.Count(); dispIndex++ )	
	{
		CCoreDispInfo *pDispInfo = g_CoreDispInfos[ dispIndex ];
		mapdispinfo_t *pMapDisp = &mapdispinfo[ dispIndex ];

		// not solid for this pass
		if ( !(pMapDisp->contents & contentsMask) )
			continue;

		int gridIndex = Disp_GridIndex( pDispInfo );
		AddToGrid( gridIndex, dispIndex );
	}

	// now make a polysoup for the terrain in each grid
	for ( int grid = 0; grid < gDispGridList.Count(); grid++ )
	{
		int triCount = 0;
		CPhysPolysoup *pTerrainPhysics = physcollision->PolysoupCreate();

		// iterate the displacements in this grid
		for ( int listIndex = 0; listIndex < gDispGridList[grid].dispList.Count(); listIndex++ )
		{
			dispIndex = gDispGridList[grid].dispList[listIndex];
			CCoreDispInfo *pDispInfo = g_CoreDispInfos[ dispIndex ];
			mapdispinfo_t *pMapDisp = &mapdispinfo[ dispIndex ];

			// Get the material id.
			MaterialSystemMaterial_t matID = GetMatIDFromDisp( pMapDisp );

			// Build a triangle list. This shares the tesselation code with the engine.
			CUtlVector<unsigned short> indices;
			CVBSPTesselateHelper helper;
			helper.m_pIndices = &indices;
			helper.m_pActiveVerts = pDispInfo->GetAllowedVerts().Base();
			helper.m_pPowerInfo = pDispInfo->GetPowerInfo();

			::TesselateDisplacement( &helper );

			Assert( indices.Count() > 0 );
			Assert( indices.Count() % 3 == 0 );	// Make sure indices are a multiple of 3.
			int nTriCount = indices.Count() / 3;
			triCount += nTriCount;
			if ( triCount >= 65536 )
			{
				// don't put more than 64K tris in any single collision model
				CPhysCollide *pCollide = physcollision->ConvertPolysoupToCollide( pTerrainPhysics, false );
				if ( pCollide )
				{
					collisionList.AddToTail( new CPhysCollisionEntryStaticMesh( pCollide, NULL ) );	
				}
				// Throw this polysoup away and start over for the remaining triangles
				physcollision->PolysoupDestroy( pTerrainPhysics );
				pTerrainPhysics = physcollision->PolysoupCreate();
				triCount = nTriCount;
			}
			Vector tmpVerts[3];
			for ( int iTri = 0; iTri < nTriCount; ++iTri )
			{
				float flAlphaTotal = 0.0f;
				for ( int iTriVert = 0; iTriVert < 3; ++iTriVert )
				{
					pDispInfo->GetVert( indices[iTri*3+iTriVert], tmpVerts[iTriVert] );
					flAlphaTotal += pDispInfo->GetAlpha( indices[iTri*3+iTriVert] );
				}

				int nProp = g_SurfaceProperties[texinfo[pMapDisp->face.texinfo].texdata];
				if ( flAlphaTotal > DISP_ALPHA_PROP_DELTA )
				{
					int nProp2 = GetSurfaceProperties2( matID, "surfaceprop2" );
					if ( nProp2 != -1 )
					{
						nProp = nProp2;
					}
				}
				int nMaterialIndex = RemapWorldMaterial( nProp );
				physcollision->PolysoupAddTriangle( pTerrainPhysics, tmpVerts[0], tmpVerts[1], tmpVerts[2], nMaterialIndex );
			}
		}

		// convert the whole grid's polysoup to a collide and store in the collision list
		CPhysCollide *pCollide = physcollision->ConvertPolysoupToCollide( pTerrainPhysics, false );
		if ( pCollide )
		{
			collisionList.AddToTail( new CPhysCollisionEntryStaticMesh( pCollide, NULL ) );	
		}
		// now that we have the collide, we're done with the soup
		physcollision->PolysoupDestroy( pTerrainPhysics );
	}
}


class CDispMeshEvent : public IVirtualMeshEvent
{
public:
	CDispMeshEvent( unsigned short *pIndices, int indexCount, CCoreDispInfo *pDispInfo );
	virtual void GetVirtualMesh( void *userData, virtualmeshlist_t *pList );
	virtual void GetWorldspaceBounds( void *userData, Vector *pMins, Vector *pMaxs );
	virtual void GetTrianglesInSphere( void *userData, const Vector &center, float radius, virtualmeshtrianglelist_t *pList );

	CUtlVector<Vector>		m_verts;
	unsigned short			*m_pIndices;
	int						m_indexCount;
};

CDispMeshEvent::CDispMeshEvent( unsigned short *pIndices, int indexCount, CCoreDispInfo *pDispInfo )
{
	m_pIndices = pIndices;
	m_indexCount = indexCount;
	int maxIndex = 0;
	for ( int i = 0; i < indexCount; i++ )
	{
		if ( pIndices[i] > maxIndex )
		{
			maxIndex = pIndices[i];
		}
	}
	for ( int i = 0; i < indexCount/2; i++ )
	{
		V_swap( pIndices[i], pIndices[(indexCount-i)-1] );
	}
	int count = maxIndex + 1;
	m_verts.SetCount( count );
	for ( int i = 0; i < count; i++ )
	{
		m_verts[i] = pDispInfo->GetVert(i);
	}
}

void CDispMeshEvent::GetVirtualMesh( void *userData, virtualmeshlist_t *pList )
{
	Assert(userData==((void *)this));
	pList->pVerts = m_verts.Base();
	pList->indexCount = m_indexCount;
	pList->triangleCount = m_indexCount/3;
	pList->vertexCount = m_verts.Count();
	pList->surfacePropsIndex = 0;	// doesn't matter here, reset at runtime
	pList->pHull = NULL;
	int indexMax = ARRAYSIZE(pList->indices);
	int indexCount = min(m_indexCount, indexMax);
	Assert(m_indexCount < indexMax);
	Q_memcpy( pList->indices, m_pIndices, sizeof(*m_pIndices) * indexCount );
}

void CDispMeshEvent::GetWorldspaceBounds( void *userData, Vector *pMins, Vector *pMaxs )
{
	Assert(userData==((void *)this));
	ClearBounds( *pMins, *pMaxs );
	for ( int i = 0; i < m_verts.Count(); i++ )
	{
		AddPointToBounds( m_verts[i], *pMins, *pMaxs );
	}
}

void CDispMeshEvent::GetTrianglesInSphere( void *userData, const Vector &center, float radius, virtualmeshtrianglelist_t *pList )
{
	Assert(userData==((void *)this));
	pList->triangleCount = m_indexCount/3;
	int indexMax = ARRAYSIZE(pList->triangleIndices);
	int indexCount = min(m_indexCount, indexMax);
	Assert(m_indexCount < MAX_VIRTUAL_TRIANGLES*3);
	Q_memcpy( pList->triangleIndices, m_pIndices, sizeof(*m_pIndices) * indexCount );
}

void Disp_BuildVirtualMesh( int contentsMask )
{
	CUtlVector<CPhysCollide *> virtualMeshes;
	virtualMeshes.EnsureCount( g_CoreDispInfos.Count() );
	for ( int i = 0; i < g_CoreDispInfos.Count(); i++ )	
	{
		CCoreDispInfo *pDispInfo = g_CoreDispInfos[ i ];
		mapdispinfo_t *pMapDisp = &mapdispinfo[ i ];

		virtualMeshes[i] = NULL;
		// not solid for this pass
		if ( !(pMapDisp->contents & contentsMask) )
			continue;

		// Build a triangle list. This shares the tesselation code with the engine.
		CUtlVector<unsigned short> indices;
		CVBSPTesselateHelper helper;
		helper.m_pIndices = &indices;
		helper.m_pActiveVerts = pDispInfo->GetAllowedVerts().Base();
		helper.m_pPowerInfo = pDispInfo->GetPowerInfo();

		::TesselateDisplacement( &helper );

		// validate the collision data
		if ( 1 )
		{
			int triCount = indices.Count() / 3;
			for ( int j = 0; j < triCount; j++ )
			{
				int index = j * 3;
				Vector v0 = pDispInfo->GetVert( indices[index+0] );
				Vector v1 = pDispInfo->GetVert( indices[index+1] );
				Vector v2 = pDispInfo->GetVert( indices[index+2] );
				if ( v0 == v1 || v1 == v2 || v2 == v0 )
				{
					Warning( "Displacement %d has bad geometry near %.2f %.2f %.2f\n", i, v0.x, v0.y, v0.z );
					texinfo_t *pTexInfo = &texinfo[pMapDisp->face.texinfo];
					dtexdata_t *pTexData = GetTexData( pTexInfo->texdata );
					const char *pMatName = TexDataStringTable_GetString( pTexData->nameStringTableID );

					Error( "Can't compile displacement physics, exiting.  Texture is %s\n", pMatName );
				}
			}

		}
		CDispMeshEvent meshHandler( indices.Base(), indices.Count(), pDispInfo );
		virtualmeshparams_t params;
		params.buildOuterHull = true;
		params.pMeshEventHandler = &meshHandler;
		params.userData = &meshHandler;
		virtualMeshes[i] = physcollision->CreateVirtualMesh( params );
	}
	unsigned int totalSize = 0;
	CUtlBuffer buf;
	dphysdisp_t header;
	header.numDisplacements = g_CoreDispInfos.Count();
	buf.PutObjects( &header );

	CUtlVector<char> dispBuf;
	for ( int i = 0; i < header.numDisplacements; i++ )
	{
		if ( virtualMeshes[i] )
		{
			unsigned int testSize = physcollision->CollideSize( virtualMeshes[i] );
			totalSize += testSize;
			buf.PutShort( testSize );
		}
		else
		{
			buf.PutShort( -1 );
		}
	}
	for ( int i = 0; i < header.numDisplacements; i++ )
	{
		if ( virtualMeshes[i] )
		{
			unsigned int testSize = physcollision->CollideSize( virtualMeshes[i] );
			dispBuf.RemoveAll();
			dispBuf.EnsureCount(testSize);

			unsigned int outSize = physcollision->CollideWrite( dispBuf.Base(), virtualMeshes[i], false );
			Assert( outSize == testSize );
			buf.Put( dispBuf.Base(), outSize );
		}
	}
	g_PhysDispSize = totalSize + sizeof(dphysdisp_t) + (sizeof(unsigned short) * header.numDisplacements);
	Assert( buf.TellMaxPut() == g_PhysDispSize );
	g_PhysDispSize = buf.TellMaxPut();
	g_pPhysDisp = new byte[g_PhysDispSize];
	Q_memcpy( g_pPhysDisp, buf.Base(), g_PhysDispSize );
}

