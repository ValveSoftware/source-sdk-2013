//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef C_PROPS_H
#define C_PROPS_H
#ifdef _WIN32
#pragma once
#endif

#include "c_breakableprop.h"
#include "props_shared.h"

#define CDynamicProp C_DynamicProp

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_DynamicProp : public C_BreakableProp
{
	DECLARE_CLASS( C_DynamicProp, C_BreakableProp );
public:
	DECLARE_NETWORKCLASS();

	// constructor, destructor
	C_DynamicProp( void );
	~C_DynamicProp( void );

	void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	unsigned int ComputeClientSideAnimationFlags();
	bool TestBoneFollowers( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	bool TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

private:
	C_DynamicProp( const C_DynamicProp & );

	bool	m_bUseHitboxesForRenderBox;
	int		m_iCachedFrameCount;
	Vector	m_vecCachedRenderMins;
	Vector	m_vecCachedRenderMaxs;
};

#endif // C_PROPS_H
