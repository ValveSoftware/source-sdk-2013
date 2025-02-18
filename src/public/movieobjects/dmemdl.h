//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing an MDL
//
//=============================================================================

#ifndef DMEMDL_H
#define DMEMDL_H

#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"
#include "movieobjects/dmeshape.h"
#include "datacache/imdlcache.h"
#include "tier3/mdlutils.h"

#include "mathlib/vector.h"
#include <string>
#include <vector>


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeTransform;
class IMorph;
class IMaterial;
struct SubdivMesh_t;
class IMesh;
class CDmeDrawSettings;
struct matrix3x4_t;
class CStudioHdr;


//-----------------------------------------------------------------------------
// A class representing an MDL
//-----------------------------------------------------------------------------
class CDmeMDL : public CDmeShape
{
	DEFINE_ELEMENT( CDmeMDL, CDmeShape );

public:
	virtual void Draw( const matrix3x4_t& shapeToWorld, CDmeDrawSettings *pDrawSettings = NULL );

	void DrawInEngine( bool bDrawInEngine );
	bool IsDrawingInEngine() const;

	void SetMDL( MDLHandle_t handle );
	MDLHandle_t GetMDL( ) const;
	float GetRadius() const; // NOTE: This radius is one that is centered at the origin
	void GetBoundingSphere( Vector &vecCenter, float &flRadius );
	void GetBoundingBox( Vector *pMins, Vector *pMaxs ) const;

	// Computes bone-to-world transforms
	void SetUpBones( const matrix3x4_t& shapeToWorld, int nMaxBoneCount, matrix3x4_t *pOutputMatrices );

public:
	CDmaColor m_Color;
	CDmaVar<int> m_nSkin;
	CDmaVar<int> m_nBody;
	CDmaVar<int> m_nSequence;
	CDmaVar<int> m_nLOD;
	CDmaVar<float> m_flPlaybackRate;
	CDmaVar<float> m_flTime;
	CDmaVar<Vector> m_vecViewTarget;
	CDmaVar<bool> m_bWorldSpaceViewTarget;

private:
	void UpdateMDL();

	CMDL m_MDL;
	bool m_bDrawInEngine;
};

#endif // DMEMDL_H
