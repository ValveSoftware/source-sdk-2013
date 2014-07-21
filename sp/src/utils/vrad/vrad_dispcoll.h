//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VRAD_DISPCOLL_H
#define VRAD_DISPCOLL_H
#pragma once

#include <assert.h>
#include "DispColl_Common.h"

//=============================================================================
//
// VRAD specific collision
//
#define VRAD_QUAD_SIZE			4

struct CPatch;

class CVRADDispColl : public CDispCollTree
{
public:

	// Creation/Destruction Functions
	CVRADDispColl();
	~CVRADDispColl();
	bool Create( CCoreDispInfo *pDisp );

	// Patches.
	bool InitPatch( int iPatch, int iParentPatch, int iChild, Vector *pPoints, int *pIndices, float &flArea );
	bool InitParentPatch( int iPatch, Vector *pPoints, float &flArea );
	float CreateParentPatches( void );
	void CreateChildPatches( int iParentPatch, int nLevel );
	void CreateChildPatchesFromRoot( int iParentPatch, int *pChildPatch );
	void CreateChildPatchesSub( int iParentPatch );

	// Operations Functions
	void BaseFacePlaneToDispUV( Vector const &vecPlanePt, Vector2D &dispUV );
	void DispUVToSurfPoint( Vector2D const &dispUV, Vector &vecPoint, float flPushEps );
	void DispUVToSurfNormal( Vector2D const &dispUV, Vector &vecNormal );

	// Data.
	inline float GetSampleRadius2( void )								{ return m_flSampleRadius2; }
	inline float GetPatchSampleRadius2( void )							{ return m_flPatchSampleRadius2; }

	inline int GetParentIndex( void )									{ return m_iParent; }		
	inline void GetParentFaceNormal( Vector &vecNormal )				{ vecNormal = m_vecStabDir; }

	inline void GetVert( int iVert, Vector &vecVert )					{ Assert( ( iVert >= 0 ) && ( iVert < GetSize() ) ); vecVert = m_aVerts[iVert]; }
	inline void GetVertNormal( int iVert, Vector &vecNormal )			{ Assert( ( iVert >= 0 ) && ( iVert < GetSize() ) ); vecNormal = m_aVertNormals[iVert]; }
	inline Vector2D const& GetLuxelCoord( int iLuxel )					{ Assert( ( iLuxel >= 0 ) && ( iLuxel < GetSize() ) ); return m_aLuxelCoords[iLuxel]; }

	// Raytracing
	void AddPolysForRayTrace( void );

protected:

	void CalcSampleRadius2AndBox( dface_t *pFace );

	// Utility.
	void DispUVToSurf_TriTLToBR( Vector &vecPoint, float flPushEps, float flU, float flV, int nSnapU, int nSnapV, int nWidth, int nHeight );
	void DispUVToSurf_TriBLToTR( Vector &vecPoint, float flPushEps, float flU, float flV, int nSnapU, int nSnapV, int nWidth, int nHeight );
	void GetSurfaceMinMax( Vector &boxMin, Vector &boxMax );
	void GetMinorAxes( Vector const &vecNormal, int &nAxis0, int &nAxis1 );

protected:

	int						m_iParent;								// Parent index
	float					m_flSampleRadius2;						// Sampling radius
	float					m_flPatchSampleRadius2;					// Patch sampling radius (max bound)
	float					m_flSampleWidth;
	float					m_flSampleHeight;
	CUtlVector<Vector2D>	m_aLuxelCoords;							// Lightmap coordinates.
	CUtlVector<Vector>		m_aVertNormals;							// Displacement vertex normals
};

#endif // VRAD_DISPCOLL_H