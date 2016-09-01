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
//SDK2013CE DynamicPropGlow
#ifdef GLOWS_ENABLE
#include "glow_outline_effect.h"
#endif // GLOWS_ENABLE
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

	//SDK2013CE DynamicPropGlow
#ifdef GLOWS_ENABLE
	virtual void	OnPreDataChanged(DataUpdateType_t updateType);
	virtual void	OnDataChanged(DataUpdateType_t updateType);
#endif // GLOWS_ENABLE

	void GetRenderBounds( Vector& theMins, Vector& theMaxs );
	unsigned int ComputeClientSideAnimationFlags();
	bool TestBoneFollowers( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	bool TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	//SDK2013CE DynamicPropGlow
#ifdef GLOWS_ENABLE
	CGlowObject			*GetGlowObject(void){ return m_pGlowEffect; }
	virtual void		GetGlowEffectColor(float *r, float *g, float *b);
#endif // GLOWS_ENABLE
private:
	C_DynamicProp( const C_DynamicProp & );

	bool	m_bUseHitboxesForRenderBox;
	int		m_iCachedFrameCount;
	Vector	m_vecCachedRenderMins;
	Vector	m_vecCachedRenderMaxs;
	//SDK2013CE DynamicPropGlow
#ifdef GLOWS_ENABLE
	bool				m_bGlowEnabled;
	bool				m_bOldGlowEnabled;
	CGlowObject			*m_pGlowEffect;
#endif // GLOWS_ENABLE
protected:
	//SDK2013CE DynamicPropGlow
#ifdef GLOWS_ENABLE	
	virtual void		UpdateGlowEffect(void);
	virtual void		DestroyGlowEffect(void);
#endif // GLOWS_ENABLE
};

#endif // C_PROPS_H
