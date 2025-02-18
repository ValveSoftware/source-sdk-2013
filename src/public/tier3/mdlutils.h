//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef MDLUTILS_H
#define MDLUTILS_H

#if defined( _WIN32 )
#pragma once
#endif

#include "datacache/imdlcache.h"
#include "mathlib/vector.h"
#include "Color.h"
#include "studio.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CJiggleBones;
struct matrix3x4_t;

struct MDLSquenceLayer_t
{
	int		m_nSequenceIndex;
	float	m_flWeight;
	bool	m_bNoLoop;
	float	m_flCycleBeganAt;
};

//-----------------------------------------------------------------------------
// Class containing simplistic MDL state for use in rendering
//-----------------------------------------------------------------------------
class CMDL
{
public:
	CMDL();
	~CMDL();

	void SetMDL( MDLHandle_t h );
	MDLHandle_t GetMDL() const;

	// Simple version of drawing; sets up bones for you
	void Draw( const matrix3x4_t& rootToWorld );

	// NOTE: This version of draw assumes you've filled in the bone to world
	// matrix yourself by calling IStudioRender::LockBoneMatrices. The pointer
	// returned by that method needs to be passed into here
	void Draw( const matrix3x4_t& rootToWorld, const matrix3x4_t *pBoneToWorld );


	void SetUpBones( const matrix3x4_t& shapeToWorld, int nMaxBoneCount, matrix3x4_t *pOutputMatrices, const float *pPoseParameters = NULL, MDLSquenceLayer_t *pSequenceLayers = NULL, int nNumSequenceLayers = 0 );
	void SetupBonesWithBoneMerge( const CStudioHdr *pMergeHdr, matrix3x4_t *pMergeBoneToWorld, 
		const CStudioHdr *pFollow, const matrix3x4_t *pFollowBoneToWorld, const matrix3x4_t &matModelToWorld );
	
	studiohdr_t *GetStudioHdr();

private:
	void UnreferenceMDL();

public:
	MDLHandle_t	m_MDLHandle;
	Color		m_Color;
	int			m_nSkin;
	int			m_nBody;
	int			m_nSequence;
	int			m_nLOD;
	float		m_flPlaybackRate;
	float		m_flTime;
	float		m_pFlexControls[ MAXSTUDIOFLEXCTRL * 4 ];
	Vector		m_vecViewTarget;
	bool		m_bWorldSpaceViewTarget;
	void		*m_pProxyData;

	CJiggleBones *m_pJiggleBones;
};


//-----------------------------------------------------------------------------
// Returns the bounding box for the model
//-----------------------------------------------------------------------------
void GetMDLBoundingBox( Vector *pMins, Vector *pMaxs, MDLHandle_t h, int nSequence );

//-----------------------------------------------------------------------------
// Returns the radius of the model as measured from the origin
//-----------------------------------------------------------------------------
float GetMDLRadius( MDLHandle_t h, int nSequence );

//-----------------------------------------------------------------------------
// Returns a more accurate bounding sphere
//-----------------------------------------------------------------------------
void GetMDLBoundingSphere( Vector *pVecCenter, float *pRadius, MDLHandle_t h, int nSequence );


#endif // MDLUTILS_H

