//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "particles_simple.h"
#include "c_baseentity.h"
#include "baseparticleentity.h"
#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//==================================================
// SporeSmokeEffect
//==================================================

class SporeSmokeEffect : public CSimpleEmitter
{
public:
	SporeSmokeEffect( const char *pDebugName ) : CSimpleEmitter( pDebugName ) {}

	static SporeSmokeEffect* Create( const char *pDebugName );

	virtual void UpdateVelocity( SimpleParticle *pParticle, float timeDelta );
	virtual float UpdateAlpha( const SimpleParticle *pParticle );

private:
	SporeSmokeEffect( const SporeSmokeEffect & );
};


SporeSmokeEffect* SporeSmokeEffect::Create( const char *pDebugName )
{
	return new SporeSmokeEffect( pDebugName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
// Output : float
//-----------------------------------------------------------------------------
float SporeSmokeEffect::UpdateAlpha( const SimpleParticle *pParticle )
{
	//return ( ((float)pParticle->m_uchStartAlpha/255.0f) * sin( M_PI * (pParticle->m_flLifetime / pParticle->m_flDieTime) ) );
	return (pParticle->m_uchStartAlpha/255.0f) + ( (float)(pParticle->m_uchEndAlpha/255.0f) - (float)(pParticle->m_uchStartAlpha/255.0f) ) * (pParticle->m_flLifetime / pParticle->m_flDieTime);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pParticle - 
//			timeDelta - 
//-----------------------------------------------------------------------------
void SporeSmokeEffect::UpdateVelocity( SimpleParticle *pParticle, float timeDelta )
{
}

