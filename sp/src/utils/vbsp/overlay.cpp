//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "disp_vbsp.h"
#include "builddisp.h"
#include "mathlib/vmatrix.h"

void Overlay_BuildBasisOrigin( doverlay_t *pOverlay );

// Overlay list.
CUtlVector<mapoverlay_t> g_aMapOverlays;
CUtlVector<mapoverlay_t> g_aMapWaterOverlays;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int Overlay_GetFromEntity( entity_t *pMapEnt )
{
	int iAccessorID = -1;

	// Allocate the new overlay.
	int iOverlay = g_aMapOverlays.AddToTail();
	mapoverlay_t *pMapOverlay = &g_aMapOverlays[iOverlay];

	// Get the overlay data.
	pMapOverlay->nId = g_aMapOverlays.Count() - 1;

	if ( ValueForKey( pMapEnt, "targetname" )[ 0 ] != '\0' )
	{
		// Overlay has a name, remember it's ID for accessing
		iAccessorID = pMapOverlay->nId;
	}

	pMapOverlay->flU[0] = FloatForKey( pMapEnt, "StartU" );
	pMapOverlay->flU[1] = FloatForKey( pMapEnt, "EndU" );
	pMapOverlay->flV[0] = FloatForKey( pMapEnt, "StartV" );
	pMapOverlay->flV[1] = FloatForKey( pMapEnt, "EndV" );

	pMapOverlay->flFadeDistMinSq = FloatForKey( pMapEnt, "fademindist" );
	if ( pMapOverlay->flFadeDistMinSq > 0 )
	{
		pMapOverlay->flFadeDistMinSq *= pMapOverlay->flFadeDistMinSq;
	}

	pMapOverlay->flFadeDistMaxSq = FloatForKey( pMapEnt, "fademaxdist" );
	if ( pMapOverlay->flFadeDistMaxSq > 0 )
	{
		pMapOverlay->flFadeDistMaxSq *= pMapOverlay->flFadeDistMaxSq;
	}

	GetVectorForKey( pMapEnt, "BasisOrigin", pMapOverlay->vecOrigin );

	pMapOverlay->m_nRenderOrder = IntForKey( pMapEnt, "RenderOrder" );
	if ( pMapOverlay->m_nRenderOrder < 0 || pMapOverlay->m_nRenderOrder >= OVERLAY_NUM_RENDER_ORDERS )
		Error( "Overlay (%s) at %f %f %f has invalid render order (%d).\n", ValueForKey( pMapEnt, "material" ),
				pMapOverlay->vecOrigin.x, pMapOverlay->vecOrigin.y, pMapOverlay->vecOrigin.z,
				pMapOverlay->m_nRenderOrder );

	GetVectorForKey( pMapEnt, "uv0", pMapOverlay->vecUVPoints[0] );
	GetVectorForKey( pMapEnt, "uv1", pMapOverlay->vecUVPoints[1] );
	GetVectorForKey( pMapEnt, "uv2", pMapOverlay->vecUVPoints[2] );
	GetVectorForKey( pMapEnt, "uv3", pMapOverlay->vecUVPoints[3] );

	GetVectorForKey( pMapEnt, "BasisU", pMapOverlay->vecBasis[0] );
	GetVectorForKey( pMapEnt, "BasisV", pMapOverlay->vecBasis[1] );
	GetVectorForKey( pMapEnt, "BasisNormal", pMapOverlay->vecBasis[2] );

	const char *pMaterialName = ValueForKey( pMapEnt, "material" );
	Assert( strlen( pMaterialName ) < OVERLAY_MAP_STRLEN );
	if ( strlen( pMaterialName ) >= OVERLAY_MAP_STRLEN )
	{
		Error( "Overlay Material Name (%s) too long! > OVERLAY_MAP_STRLEN (%d)", pMaterialName, OVERLAY_MAP_STRLEN );
		return -1;
	}
	strcpy( pMapOverlay->szMaterialName, pMaterialName );	

	// Convert the sidelist to side id(s).
	const char *pSideList = ValueForKey( pMapEnt, "sides" );
	char *pTmpList = ( char* )_alloca( strlen( pSideList ) + 1 );
	strcpy( pTmpList, pSideList );
	const char *pScan = strtok( pTmpList, " " );
	if ( !pScan )
		return iAccessorID;

	pMapOverlay->aSideList.Purge();
	pMapOverlay->aFaceList.Purge();

	do
	{
		int nSideId;
		if ( sscanf( pScan, "%d", &nSideId ) == 1 )
		{
			pMapOverlay->aSideList.AddToTail( nSideId );
		}
	} while ( ( pScan = strtok( NULL, " " ) ) );

	return iAccessorID;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
side_t *GetSide( int nSideId )
{
	for( int iSide = 0; iSide < g_LoadingMap->nummapbrushsides; ++iSide )
	{
		if ( g_LoadingMap->brushsides[iSide].id == nSideId )
			return &g_LoadingMap->brushsides[iSide];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_UpdateSideLists( int StartIndex )
{
	int nMapOverlayCount = g_aMapOverlays.Count();
	for( int iMapOverlay = StartIndex; iMapOverlay < nMapOverlayCount; ++iMapOverlay )
	{
		mapoverlay_t *pMapOverlay = &g_aMapOverlays.Element( iMapOverlay );
		if ( pMapOverlay )
		{
			int nSideCount = pMapOverlay->aSideList.Count();
			for( int iSide = 0; iSide < nSideCount; ++iSide )
			{
				side_t *pSide = GetSide( pMapOverlay->aSideList[iSide] );
				if ( pSide )
				{
					if ( pSide->aOverlayIds.Find( pMapOverlay->nId ) == -1 )
					{
						pSide->aOverlayIds.AddToTail( pMapOverlay->nId );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void OverlayTransition_UpdateSideLists( int StartIndex )
{
	int nOverlayCount = g_aMapWaterOverlays.Count();
	for( int iOverlay = StartIndex; iOverlay < nOverlayCount; ++iOverlay )
	{
		mapoverlay_t *pOverlay = &g_aMapWaterOverlays.Element( iOverlay );
		if ( pOverlay )
		{
			int nSideCount = pOverlay->aSideList.Count();
			for( int iSide = 0; iSide < nSideCount; ++iSide )
			{
				side_t *pSide = GetSide( pOverlay->aSideList[iSide] );
				if ( pSide )
				{
					if ( pSide->aWaterOverlayIds.Find( pOverlay->nId ) == -1 )
					{
						pSide->aWaterOverlayIds.AddToTail( pOverlay->nId );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_AddFaceToLists( int iFace, side_t *pSide )
{
	int nOverlayIdCount = pSide->aOverlayIds.Count();
	for( int iOverlayId = 0; iOverlayId < nOverlayIdCount; ++iOverlayId )
	{
		mapoverlay_t *pMapOverlay = &g_aMapOverlays.Element( pSide->aOverlayIds[iOverlayId] );
		if ( pMapOverlay )
		{
			if( pMapOverlay->aFaceList.Find( iFace ) == -1 )
			{
				pMapOverlay->aFaceList.AddToTail( iFace );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void OverlayTransition_AddFaceToLists( int iFace, side_t *pSide )
{
	int nOverlayIdCount = pSide->aWaterOverlayIds.Count();
	for( int iOverlayId = 0; iOverlayId < nOverlayIdCount; ++iOverlayId )
	{
		mapoverlay_t *pMapOverlay = &g_aMapWaterOverlays.Element( pSide->aWaterOverlayIds[iOverlayId] - ( MAX_MAP_OVERLAYS + 1 ) );
		if ( pMapOverlay )
		{
			if( pMapOverlay->aFaceList.Find( iFace ) == -1 )
			{
				pMapOverlay->aFaceList.AddToTail( iFace );
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Overlay_EmitOverlayFace( mapoverlay_t *pMapOverlay )
{
 	Assert( g_nOverlayCount < MAX_MAP_OVERLAYS );
	if ( g_nOverlayCount >= MAX_MAP_OVERLAYS )
	{
		Error ( "Too Many Overlays!\nMAX_MAP_OVERLAYS = %d", MAX_MAP_OVERLAYS );
		return;
	}

	doverlay_t *pOverlay = &g_Overlays[g_nOverlayCount];
	doverlayfade_t *pOverlayFade = &g_OverlayFades[g_nOverlayCount];

	g_nOverlayCount++;

	// Conver the map overlay into a .bsp overlay (doverlay_t).
	if ( pOverlay )
	{
		pOverlay->nId = pMapOverlay->nId;

		pOverlay->flU[0] = pMapOverlay->flU[0];
		pOverlay->flU[1] = pMapOverlay->flU[1];
		pOverlay->flV[0] = pMapOverlay->flV[0];
		pOverlay->flV[1] = pMapOverlay->flV[1];

		VectorCopy( pMapOverlay->vecUVPoints[0], pOverlay->vecUVPoints[0] );
		VectorCopy( pMapOverlay->vecUVPoints[1], pOverlay->vecUVPoints[1] );
		VectorCopy( pMapOverlay->vecUVPoints[2], pOverlay->vecUVPoints[2] );
		VectorCopy( pMapOverlay->vecUVPoints[3], pOverlay->vecUVPoints[3] );

		VectorCopy( pMapOverlay->vecOrigin, pOverlay->vecOrigin );

		VectorCopy( pMapOverlay->vecBasis[2], pOverlay->vecBasisNormal );

		pOverlay->SetRenderOrder( pMapOverlay->m_nRenderOrder );

		// Encode the BasisU into the unused z component of the vecUVPoints 0, 1, 2
		pOverlay->vecUVPoints[0].z = pMapOverlay->vecBasis[0].x;
		pOverlay->vecUVPoints[1].z = pMapOverlay->vecBasis[0].y;
		pOverlay->vecUVPoints[2].z = pMapOverlay->vecBasis[0].z;

		// Encode whether or not the v axis should be flipped.
		Vector vecCross = pMapOverlay->vecBasis[2].Cross( pMapOverlay->vecBasis[0] );
		if ( vecCross.Dot( pMapOverlay->vecBasis[1] ) < 0.0f )
		{
			pOverlay->vecUVPoints[3].z = 1.0f;
		}

		// Texinfo.
		texinfo_t texInfo;
		texInfo.flags = 0;
		texInfo.texdata = FindOrCreateTexData( pMapOverlay->szMaterialName );
		for( int iVec = 0; iVec < 2; ++iVec )
		{
			for( int iAxis = 0; iAxis < 3; ++iAxis )
			{
				texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][iAxis] = 0.0f;
				texInfo.textureVecsTexelsPerWorldUnits[iVec][iAxis] = 0.0f;
			}

			texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][3] = -99999.0f;
			texInfo.textureVecsTexelsPerWorldUnits[iVec][3] = -99999.0f;			
		}
		pOverlay->nTexInfo = FindOrCreateTexInfo( texInfo );

		// Face List
		int nFaceCount = pMapOverlay->aFaceList.Count();
		Assert( nFaceCount < OVERLAY_BSP_FACE_COUNT );
		if ( nFaceCount >= OVERLAY_BSP_FACE_COUNT )
		{
			Error( "Overlay touching too many faces (touching %d, max %d)\nOverlay %s at %.1f %.1f %.1f", nFaceCount, OVERLAY_BSP_FACE_COUNT, pMapOverlay->szMaterialName, pMapOverlay->vecOrigin.x, pMapOverlay->vecOrigin.y, pMapOverlay->vecOrigin.z );
			return;
		}

		pOverlay->SetFaceCount( nFaceCount );
		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			pOverlay->aFaces[iFace] = pMapOverlay->aFaceList.Element( iFace );
		}
	}

	// Convert the map overlay fade data into a .bsp overlay fade (doverlayfade_t).
	if ( pOverlayFade )
	{
		pOverlayFade->flFadeDistMinSq = pMapOverlay->flFadeDistMinSq;
		pOverlayFade->flFadeDistMaxSq = pMapOverlay->flFadeDistMaxSq;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void OverlayTransition_EmitOverlayFace( mapoverlay_t *pMapOverlay )
{
	Assert( g_nWaterOverlayCount < MAX_MAP_WATEROVERLAYS );
	if ( g_nWaterOverlayCount >= MAX_MAP_WATEROVERLAYS )
	{
		Error ( "Too many water overlays!\nMAX_MAP_WATEROVERLAYS = %d", MAX_MAP_WATEROVERLAYS );
		return;
	}

	dwateroverlay_t *pOverlay = &g_WaterOverlays[g_nWaterOverlayCount];
	g_nWaterOverlayCount++;

	// Conver the map overlay into a .bsp overlay (doverlay_t).
	if ( pOverlay )
	{
		pOverlay->nId = pMapOverlay->nId;

		pOverlay->flU[0] = pMapOverlay->flU[0];
		pOverlay->flU[1] = pMapOverlay->flU[1];
		pOverlay->flV[0] = pMapOverlay->flV[0];
		pOverlay->flV[1] = pMapOverlay->flV[1];

		VectorCopy( pMapOverlay->vecUVPoints[0], pOverlay->vecUVPoints[0] );
		VectorCopy( pMapOverlay->vecUVPoints[1], pOverlay->vecUVPoints[1] );
		VectorCopy( pMapOverlay->vecUVPoints[2], pOverlay->vecUVPoints[2] );
		VectorCopy( pMapOverlay->vecUVPoints[3], pOverlay->vecUVPoints[3] );

		VectorCopy( pMapOverlay->vecOrigin, pOverlay->vecOrigin );

		VectorCopy( pMapOverlay->vecBasis[2], pOverlay->vecBasisNormal );

		pOverlay->SetRenderOrder( pMapOverlay->m_nRenderOrder );

		// Encode the BasisU into the unused z component of the vecUVPoints 0, 1, 2
		pOverlay->vecUVPoints[0].z = pMapOverlay->vecBasis[0].x;
		pOverlay->vecUVPoints[1].z = pMapOverlay->vecBasis[0].y;
		pOverlay->vecUVPoints[2].z = pMapOverlay->vecBasis[0].z;

		// Encode whether or not the v axis should be flipped.
		Vector vecCross = pMapOverlay->vecBasis[2].Cross( pMapOverlay->vecBasis[0] );
		if ( vecCross.Dot( pMapOverlay->vecBasis[1] ) < 0.0f )
		{
			pOverlay->vecUVPoints[3].z = 1.0f;
		}

		// Texinfo.
		texinfo_t texInfo;
		texInfo.flags = 0;
		texInfo.texdata = FindOrCreateTexData( pMapOverlay->szMaterialName );
		for( int iVec = 0; iVec < 2; ++iVec )
		{
			for( int iAxis = 0; iAxis < 3; ++iAxis )
			{
				texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][iAxis] = 0.0f;
				texInfo.textureVecsTexelsPerWorldUnits[iVec][iAxis] = 0.0f;
			}

			texInfo.lightmapVecsLuxelsPerWorldUnits[iVec][3] = -99999.0f;
			texInfo.textureVecsTexelsPerWorldUnits[iVec][3] = -99999.0f;			
		}
		pOverlay->nTexInfo = FindOrCreateTexInfo( texInfo );

		// Face List
		int nFaceCount = pMapOverlay->aFaceList.Count();
		Assert( nFaceCount < WATEROVERLAY_BSP_FACE_COUNT );
		if ( nFaceCount >= WATEROVERLAY_BSP_FACE_COUNT )
		{
			Error( "Water Overlay touching too many faces (touching %d, max %d)\nOverlay %s at %.1f %.1f %.1f", nFaceCount, OVERLAY_BSP_FACE_COUNT, pMapOverlay->szMaterialName, pMapOverlay->vecOrigin.x, pMapOverlay->vecOrigin.y, pMapOverlay->vecOrigin.z );
			return;
		}

		pOverlay->SetFaceCount( nFaceCount );
		for( int iFace = 0; iFace < nFaceCount; ++iFace )
		{
			pOverlay->aFaces[iFace] = pMapOverlay->aFaceList.Element( iFace );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Overlay_EmitOverlayFaces( void )
{
	int nMapOverlayCount = g_aMapOverlays.Count();
	for( int iMapOverlay = 0; iMapOverlay < nMapOverlayCount; ++iMapOverlay )
	{
		Overlay_EmitOverlayFace( &g_aMapOverlays.Element( iMapOverlay ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void OverlayTransition_EmitOverlayFaces( void )
{
	int nMapOverlayCount = g_aMapWaterOverlays.Count();
	for( int iMapOverlay = 0; iMapOverlay < nMapOverlayCount; ++iMapOverlay )
	{
		OverlayTransition_EmitOverlayFace( &g_aMapWaterOverlays.Element( iMapOverlay ) );
	}
}


//-----------------------------------------------------------------------------
// These routines were mostly stolen from MapOverlay.cpp in Hammer
//-----------------------------------------------------------------------------
#define OVERLAY_BASIS_U					0
#define OVERLAY_BASIS_V					1
#define OVERLAY_BASIS_NORMAL			2	
#define OVERLAY_HANDLES_COUNT			4


inline void TransformPoint( const VMatrix& matrix, Vector &point )
{
	Vector orgVector = point;
	matrix.V3Mul( orgVector, point );
}


inline bool fequal( float value, float target, float delta) { return ( (value<(target+delta))&&(value>(target-delta)) ); }


//-----------------------------------------------------------------------------
// Purpose: this function translate / rotate an overlay.
// Input  : pOverlay - the overlay to be translated
//			OriginOffset - the translation
//			AngleOffset - the rotation
//			Matrix - the translation / rotation matrix
// Output : none
//-----------------------------------------------------------------------------
void Overlay_Translate( mapoverlay_t *pOverlay, Vector &OriginOffset, QAngle &AngleOffset, matrix3x4_t &Matrix )
{
	VMatrix tmpMatrix( Matrix );

	Vector	temp = pOverlay->vecOrigin;
	VectorTransform( temp, Matrix, pOverlay->vecOrigin );

	// erase move component
	tmpMatrix.SetTranslation( vec3_origin );

	// check if matrix would still change something 
	if ( !tmpMatrix.IsIdentity() )
	{
		// make sure axes are normalized (they should be anyways)
		pOverlay->vecBasis[OVERLAY_BASIS_U].NormalizeInPlace();
		pOverlay->vecBasis[OVERLAY_BASIS_V].NormalizeInPlace();

		Vector vecU = pOverlay->vecBasis[OVERLAY_BASIS_U];
		Vector vecV = pOverlay->vecBasis[OVERLAY_BASIS_V];
		Vector vecNormal = pOverlay->vecBasis[OVERLAY_BASIS_NORMAL];

		TransformPoint( tmpMatrix, vecU );
		TransformPoint( tmpMatrix, vecV );
		TransformPoint( tmpMatrix, vecNormal );

		float fScaleU = vecU.Length();
		float fScaleV = vecV.Length();
		float flScaleNormal = vecNormal.Length();

		bool bIsUnit = ( fequal( fScaleU, 1.0f, 0.0001 ) && fequal( fScaleV, 1.0f, 0.0001 ) && fequal( flScaleNormal, 1.0f, 0.0001 ) );
		bool bIsPerp = ( fequal( DotProduct( vecU, vecV ), 0.0f, 0.0025 ) && fequal( DotProduct( vecU, vecNormal ), 0.0f, 0.0025 ) && fequal( DotProduct( vecV, vecNormal ), 0.0f, 0.0025 ) );

		//		if ( fequal(fScaleU,1,0.0001) && fequal(fScaleV,1,0.0001) && fequal(DotProduct( vecU, vecV ),0,0.0025) )
		if ( bIsUnit && bIsPerp )
		{
			// transformation doesnt scale or shear anything, so just update base axes
			pOverlay->vecBasis[OVERLAY_BASIS_U] = vecU;
			pOverlay->vecBasis[OVERLAY_BASIS_V] = vecV;
			pOverlay->vecBasis[OVERLAY_BASIS_NORMAL] = vecNormal;
		}
		else
		{
			// more complex transformation, move UV coordinates, but leave base axes 
			for ( int iHandle=0; iHandle<OVERLAY_HANDLES_COUNT;iHandle++)
			{
				Vector vecUV = pOverlay->vecUVPoints[iHandle];
				Vector vecPos = ( vecUV.x * pOverlay->vecBasis[OVERLAY_BASIS_U] + vecUV.y * pOverlay->vecBasis[OVERLAY_BASIS_V] );

				// to transform in world space
				TransformPoint( tmpMatrix, vecPos );

				vecUV.x = pOverlay->vecBasis[OVERLAY_BASIS_U].Dot( vecPos );
				vecUV.y = pOverlay->vecBasis[OVERLAY_BASIS_V].Dot( vecPos );

				pOverlay->vecUVPoints[iHandle] = vecUV;
			}
		}
	}

}
