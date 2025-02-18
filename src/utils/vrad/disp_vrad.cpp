//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "disp_vrad.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "iscratchpad3d.h"
#include "scratchpadutils.h"


//#define USE_SCRATCHPAD
#if defined( USE_SCRATCHPAD )
	static IScratchPad3D *g_pPad = 0;
#endif


int FindNeighborCornerVert( CCoreDispInfo *pDisp, const Vector &vTest )
{
	CDispUtilsHelper *pDispHelper = pDisp;

	int iClosest = 0;
	float flClosest = 1e24;
	for ( int iCorner=0; iCorner < 4; iCorner++ )
	{
		// Has it been touched?
		CVertIndex cornerVert = pDispHelper->GetPowerInfo()->GetCornerPointIndex( iCorner );
		int iCornerVert = pDispHelper->VertIndexToInt( cornerVert );
		const Vector &vCornerVert = pDisp->GetVert( iCornerVert );

		float flDist = vCornerVert.DistTo( vTest );
		if ( flDist < flClosest )
		{
			iClosest = iCorner;
			flClosest = flDist;
		}
	}

	if ( flClosest <= 0.1f )
		return iClosest;
	else
		return -1;
}


int GetAllNeighbors( const CCoreDispInfo *pDisp, int (&iNeighbors)[512] )
{
	int nNeighbors = 0;

	// Check corner neighbors.
	for ( int iCorner=0; iCorner < 4; iCorner++ )
	{
		const CDispCornerNeighbors *pCorner = pDisp->GetCornerNeighbors( iCorner );

		for ( int i=0; i < pCorner->m_nNeighbors; i++ )
		{
			if ( nNeighbors < ARRAYSIZE( iNeighbors ) )
				iNeighbors[nNeighbors++] = pCorner->m_Neighbors[i];
		}
	}

	for ( int iEdge=0; iEdge < 4; iEdge++ )
	{
		const CDispNeighbor *pEdge = pDisp->GetEdgeNeighbor( iEdge );

		for ( int i=0; i < 2; i++ )
		{
			if ( pEdge->m_SubNeighbors[i].IsValid() )
				if ( nNeighbors < 512 )
					iNeighbors[nNeighbors++] = pEdge->m_SubNeighbors[i].GetNeighborIndex();
		}
	}

	return nNeighbors;
}


void BlendCorners( CCoreDispInfo **ppListBase, int listSize )
{
	CUtlVector<int> nbCornerVerts;

	for ( int iDisp=0; iDisp < listSize; iDisp++ )
	{
		CCoreDispInfo *pDisp = ppListBase[iDisp];

		int iNeighbors[512];
		int nNeighbors = GetAllNeighbors( pDisp, iNeighbors );

		// Make sure we have room for all the neighbors.
		nbCornerVerts.RemoveAll();
		nbCornerVerts.EnsureCapacity( nNeighbors );
		nbCornerVerts.AddMultipleToTail( nNeighbors );
		
		// For each corner.
		for ( int iCorner=0; iCorner < 4; iCorner++ )
		{
			// Has it been touched?
			CVertIndex cornerVert = pDisp->GetCornerPointIndex( iCorner );
			int iCornerVert = pDisp->VertIndexToInt( cornerVert );
			const Vector &vCornerVert = pDisp->GetVert( iCornerVert );

			// For each displacement sharing this corner..
			Vector vAverage = pDisp->GetNormal( iCornerVert );

			for ( int iNeighbor=0; iNeighbor < nNeighbors; iNeighbor++ )
			{
				int iNBListIndex = iNeighbors[iNeighbor];
				CCoreDispInfo *pNeighbor = ppListBase[iNBListIndex];
				
				// Find out which vert it is on the neighbor.
				int iNBCorner = FindNeighborCornerVert( pNeighbor, vCornerVert );
				if ( iNBCorner == -1 )
				{
					nbCornerVerts[iNeighbor] = -1; // remove this neighbor from the list.
				}
				else
				{
					CVertIndex viNBCornerVert = pNeighbor->GetCornerPointIndex( iNBCorner );
					int iNBVert = pNeighbor->VertIndexToInt( viNBCornerVert );
					nbCornerVerts[iNeighbor] = iNBVert;
					vAverage += pNeighbor->GetNormal( iNBVert );
				}
			}


			// Blend all the neighbor normals with this one.
			VectorNormalize( vAverage );
			pDisp->SetNormal( iCornerVert, vAverage );

#if defined( USE_SCRATCHPAD )
			ScratchPad_DrawArrowSimple( 
				g_pPad, 
				pDisp->GetVert( iCornerVert ), 
				pDisp->GetNormal( iCornerVert ), 
				Vector( 0, 0, 1 ),
				25 );
#endif

			for ( int iNeighbor=0; iNeighbor < nNeighbors; iNeighbor++ )
			{
				int iNBListIndex = iNeighbors[iNeighbor];
				if ( nbCornerVerts[iNeighbor] == -1 )
					continue;

				CCoreDispInfo *pNeighbor = ppListBase[iNBListIndex];
				pNeighbor->SetNormal( nbCornerVerts[iNeighbor], vAverage );
			}
		}
	}
}


void BlendTJuncs( CCoreDispInfo **ppListBase, int listSize )
{
	for ( int iDisp=0; iDisp < listSize; iDisp++ )
	{
		CCoreDispInfo *pDisp = ppListBase[iDisp];

		for ( int iEdge=0; iEdge < 4; iEdge++ )
		{
			CDispNeighbor *pEdge = pDisp->GetEdgeNeighbor( iEdge );

			CVertIndex viMidPoint = pDisp->GetEdgeMidPoint( iEdge );
			int iMidPoint = pDisp->VertIndexToInt( viMidPoint );

			if ( pEdge->m_SubNeighbors[0].IsValid() && pEdge->m_SubNeighbors[1].IsValid() )
			{
				const Vector &vMidPoint = pDisp->GetVert( iMidPoint );

				CCoreDispInfo *pNeighbor1 = ppListBase[pEdge->m_SubNeighbors[0].GetNeighborIndex()];
				CCoreDispInfo *pNeighbor2 = ppListBase[pEdge->m_SubNeighbors[1].GetNeighborIndex()];

				int iNBCorners[2];
				iNBCorners[0] = FindNeighborCornerVert( pNeighbor1, vMidPoint );
				iNBCorners[1] = FindNeighborCornerVert( pNeighbor2, vMidPoint );
				
				if ( iNBCorners[0] != -1 && iNBCorners[1] != -1 )
				{
					CVertIndex viNBCorners[2] = 
					{
						pNeighbor1->GetCornerPointIndex( iNBCorners[0] ),
						pNeighbor2->GetCornerPointIndex( iNBCorners[1] )
					};

					Vector vAverage = pDisp->GetNormal( iMidPoint );
					vAverage += pNeighbor1->GetNormal( viNBCorners[0] );
					vAverage += pNeighbor2->GetNormal( viNBCorners[1] );

					VectorNormalize( vAverage );
					pDisp->SetNormal( iMidPoint, vAverage );
					pNeighbor1->SetNormal( viNBCorners[0], vAverage );
					pNeighbor2->SetNormal( viNBCorners[1], vAverage );

#if defined( USE_SCRATCHPAD )
					ScratchPad_DrawArrowSimple( g_pPad, pDisp->GetVert( iMidPoint ), pDisp->GetNormal( iMidPoint ), Vector( 0, 1, 1 ), 25 );
#endif
				}
			}
		}
	}
}


void BlendEdges( CCoreDispInfo **ppListBase, int listSize )
{
	for ( int iDisp=0; iDisp < listSize; iDisp++ )
	{
		CCoreDispInfo *pDisp = ppListBase[iDisp];

		for ( int iEdge=0; iEdge < 4; iEdge++ )
		{
			CDispNeighbor *pEdge = pDisp->GetEdgeNeighbor( iEdge );

			for ( int iSub=0; iSub < 2; iSub++ )
			{
				CDispSubNeighbor *pSub = &pEdge->m_SubNeighbors[iSub];
				if ( !pSub->IsValid() )
					continue;

				CCoreDispInfo *pNeighbor = ppListBase[ pSub->GetNeighborIndex() ];

				int iEdgeDim = g_EdgeDims[iEdge];

				CDispSubEdgeIterator it;
				it.Start( pDisp, iEdge, iSub, true );

				// Get setup on the first corner vert.
				it.Next();
				CVertIndex viPrevPos = it.GetVertIndex();

				while ( it.Next() )
				{
					// Blend the two.
					if ( !it.IsLastVert() )
					{
						Vector vAverage = pDisp->GetNormal( it.GetVertIndex() ) + pNeighbor->GetNormal( it.GetNBVertIndex() );
						VectorNormalize( vAverage );

						pDisp->SetNormal( it.GetVertIndex(), vAverage );
						pNeighbor->SetNormal( it.GetNBVertIndex(), vAverage );

#if defined( USE_SCRATCHPAD )
						ScratchPad_DrawArrowSimple( g_pPad, pDisp->GetVert( it.GetVertIndex() ), pDisp->GetNormal( it.GetVertIndex() ), Vector( 1, 0, 0 ), 25 );
#endif
					}

					// Now blend the in-between verts (if this edge is high-res).
					int iPrevPos = viPrevPos[ !iEdgeDim ];
					int iCurPos = it.GetVertIndex()[ !iEdgeDim ];
					
					for ( int iTween = iPrevPos+1; iTween < iCurPos; iTween++ )
					{
						float flPercent = RemapVal( iTween, iPrevPos, iCurPos, 0, 1 );
						Vector vNormal;
						VectorLerp( pDisp->GetNormal( viPrevPos ), pDisp->GetNormal( it.GetVertIndex() ), flPercent, vNormal );
						VectorNormalize( vNormal );

						CVertIndex viTween;
						viTween[iEdgeDim] = it.GetVertIndex()[ iEdgeDim ];
						viTween[!iEdgeDim] = iTween;
						pDisp->SetNormal( viTween, vNormal );

#if defined( USE_SCRATCHPAD )
						ScratchPad_DrawArrowSimple( g_pPad, pDisp->GetVert( viTween ), pDisp->GetNormal( viTween ), Vector( 1, 0.5, 0 ), 25 );
#endif
					}
			
					viPrevPos = it.GetVertIndex();
				}
			}
		}
	}
}


#if defined( USE_SCRATCHPAD )
	void ScratchPad_DrawOriginalNormals( const CCoreDispInfo *pListBase, int listSize )
	{
		for ( int i=0; i < listSize; i++ )
		{
			const CCoreDispInfo *pDisp = &pListBase[i];
			const CPowerInfo *pPowerInfo = pDisp->GetPowerInfo();

			// Draw the triangles.
			for ( int iTri=0; iTri < pPowerInfo->GetNumTriInfos(); iTri++ )
			{
				const CTriInfo *pTriInfo = pPowerInfo->GetTriInfo( iTri );
			
				for ( int iLine=0; iLine < 3; iLine++ )
				{
					const Vector &v1 = pDisp->GetVert( pTriInfo->m_Indices[iLine] );
					const Vector &v2 = pDisp->GetVert( pTriInfo->m_Indices[(iLine+1)%3] );

					g_pPad->DrawLine( CSPVert( v1 ), CSPVert( v2 ) );
				}
			}

			// Draw the normals.
			CDispCircumferenceIterator it( pPowerInfo->GetSideLength() );
			while ( it.Next() )
			{
				ScratchPad_DrawArrowSimple( 
					g_pPad, 
					pDisp->GetVert( it.GetVertIndex() ), 
					pDisp->GetNormal( it.GetVertIndex() ), 
					Vector( 0, 1, 0 ),
					15 );
			}
		}
	}
#endif


void SmoothNeighboringDispSurfNormals( CCoreDispInfo **ppListBase, int listSize )
{
//#if defined( USE_SCRATCHPAD )
//	g_pPad = ScratchPad3D_Create();
//	ScratchPad_DrawOriginalNormals( pListBase, listSize );
//#endif

	BlendTJuncs( ppListBase, listSize );

	BlendCorners( ppListBase, listSize );

	BlendEdges( ppListBase, listSize );
}



