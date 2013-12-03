//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Interface used to construct morph buffers
//=============================================================================

#ifndef IMORPH_H
#define IMORPH_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"
#include <float.h>
#include "tier0/dbg.h"
#include "materialsystem/imaterial.h"


//-----------------------------------------------------------------------------
// Single morph data
//-----------------------------------------------------------------------------
struct MorphVertexInfo_t
{
	int m_nVertexId;		// What vertex is this going to affect?
	int m_nMorphTargetId;	// What morph did it come from?
	Vector m_PositionDelta;	// Positional morph delta
	Vector m_NormalDelta;	// Normal morph delta
	float m_flWrinkleDelta;	// Wrinkle morph delta
	float m_flSpeed;
	float m_flSide;
};


//-----------------------------------------------------------------------------
// Morph weight data
//-----------------------------------------------------------------------------
enum MorphWeightType_t
{
	MORPH_WEIGHT = 0,
	MORPH_WEIGHT_LAGGED,
	MORPH_WEIGHT_STEREO,
	MORPH_WEIGHT_STEREO_LAGGED,

	MORPH_WEIGHT_COUNT,
};

struct MorphWeight_t
{
	float m_pWeight[MORPH_WEIGHT_COUNT];
};


//-----------------------------------------------------------------------------
// Interface to the morph
//-----------------------------------------------------------------------------
abstract_class IMorph
{
public:
	// Locks the morph, destroys any existing contents
	virtual void Lock( float flFloatToFixedScale = 1.0f ) = 0;

	// Adds a morph
	virtual void AddMorph( const MorphVertexInfo_t &info ) = 0;

	// Unlocks the morph
	virtual void Unlock(  ) = 0;
};


//-----------------------------------------------------------------------------
// Morph builders
//-----------------------------------------------------------------------------
class CMorphBuilder
{
public:
	CMorphBuilder();
	~CMorphBuilder();

	// Start building the morph
	void Begin( IMorph *pMorph, float flFloatToFixedScale = 1.0f );

	// End building the morph
	void End();

	void PositionDelta3fv( const float *pDelta );
	void PositionDelta3f( float dx, float dy, float dz );
	void PositionDelta3( const Vector &vec );

	void NormalDelta3fv( const float *pDelta );
	void NormalDelta3f( float dx, float dy, float dz );
	void NormalDelta3( const Vector &vec );

	void WrinkleDelta1f( float flWrinkle );

	// Both are 0-1 values indicating which morph target to use (for stereo morph targets)
	// and how much to blend between using lagged weights vs actual weights
	// Speed: 0 - use lagged, 1 - use actual
	void Speed1f( float flSpeed );
	void Side1f( float flSide );

	void AdvanceMorph( int nSourceVertex, int nMorphTargetId );

private:
	MorphVertexInfo_t m_Info;
	IMorph *m_pMorph;
};


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
inline CMorphBuilder::CMorphBuilder()
{
	m_pMorph = NULL;
}

inline CMorphBuilder::~CMorphBuilder()
{
	// You forgot to call End()!
	Assert( !m_pMorph );
}


//-----------------------------------------------------------------------------
// Start building the morph
//-----------------------------------------------------------------------------
inline void CMorphBuilder::Begin( IMorph *pMorph, float flFloatToFixedScale )
{
	Assert( pMorph && !m_pMorph );
	m_pMorph = pMorph;
	m_pMorph->Lock( flFloatToFixedScale );

#ifdef _DEBUG
	m_Info.m_PositionDelta.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
	m_Info.m_NormalDelta.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
	m_Info.m_flWrinkleDelta = VEC_T_NAN;
	m_Info.m_flSpeed = VEC_T_NAN;
	m_Info.m_flSide = VEC_T_NAN;
#endif
}

// End building the morph
inline void CMorphBuilder::End()
{
	Assert( m_pMorph );
	m_pMorph->Unlock();
	m_pMorph = NULL;
}


//-----------------------------------------------------------------------------
// Set position delta
//-----------------------------------------------------------------------------
inline void CMorphBuilder::PositionDelta3fv( const float *pDelta )
{
	Assert( m_pMorph );
	m_Info.m_PositionDelta.Init( pDelta[0], pDelta[1], pDelta[2] );
}

inline void CMorphBuilder::PositionDelta3f( float dx, float dy, float dz )
{
	Assert( m_pMorph );
	m_Info.m_PositionDelta.Init( dx, dy, dz );
}

inline void CMorphBuilder::PositionDelta3( const Vector &vec )
{
	Assert( m_pMorph );
	m_Info.m_PositionDelta = vec;
}


//-----------------------------------------------------------------------------
// Set normal delta
//-----------------------------------------------------------------------------
inline void CMorphBuilder::NormalDelta3fv( const float *pDelta )
{
	Assert( m_pMorph );
	m_Info.m_NormalDelta.Init( pDelta[0], pDelta[1], pDelta[2] );
}

inline void CMorphBuilder::NormalDelta3f( float dx, float dy, float dz )
{
	Assert( m_pMorph );
	m_Info.m_NormalDelta.Init( dx, dy, dz );
}

inline void CMorphBuilder::NormalDelta3( const Vector &vec )
{
	Assert( m_pMorph );
	m_Info.m_NormalDelta = vec;
}


//-----------------------------------------------------------------------------
// Set wrinkle delta
//-----------------------------------------------------------------------------
inline void CMorphBuilder::WrinkleDelta1f( float flWrinkle )
{
	Assert( m_pMorph );
	m_Info.m_flWrinkleDelta = flWrinkle;
}


//-----------------------------------------------------------------------------
// Set speed,side data
//-----------------------------------------------------------------------------
inline void CMorphBuilder::Speed1f( float flSpeed )
{
	Assert( m_pMorph );
	m_Info.m_flSpeed = flSpeed;
}

inline void CMorphBuilder::Side1f( float flSide )
{
	Assert( m_pMorph );
	m_Info.m_flSide = flSide;
}


//-----------------------------------------------------------------------------
// Advance morph
//-----------------------------------------------------------------------------
inline void CMorphBuilder::AdvanceMorph( int nSourceVertex, int nMorphTargetId )
{
	Assert( m_pMorph );

	m_Info.m_nVertexId = nSourceVertex;
	m_Info.m_nMorphTargetId = nMorphTargetId;

	m_pMorph->AddMorph( m_Info );

#ifdef _DEBUG
	m_Info.m_PositionDelta.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
	m_Info.m_NormalDelta.Init( VEC_T_NAN, VEC_T_NAN, VEC_T_NAN );
	m_Info.m_flWrinkleDelta = VEC_T_NAN;
	m_Info.m_flSpeed = VEC_T_NAN;
	m_Info.m_flSide = VEC_T_NAN;
#endif
}


#endif // IMORPH_H
