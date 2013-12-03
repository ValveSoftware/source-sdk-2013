//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// This file defines the C_TEParticleSystem class, which handles most of the
// interfacing with the particle manager, simulation, and rendering.

// NOTE: most tempents are singletons.

#ifndef C_TE_PARTICLESYSTEM_H
#define C_TE_PARTICLESYSTEM_H


#include "particlemgr.h"
#include "c_basetempentity.h"
#include "particles_simple.h"


#define SIMSHIFT	10


typedef enum {
	pt_static, 
	pt_grav,
	pt_slowgrav,
	pt_fire,
	pt_explode,
	pt_explode2,
	pt_blob,
	pt_blob2,
	pt_vox_slowgrav,
	pt_vox_grav,
	pt_snow,
	pt_rain,
	pt_clientcustom   // Must have callback function specified
} ptype_t;



class C_TEParticleSystem : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TEParticleSystem, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

					C_TEParticleSystem();


public:

	// particle effect sort origin
	Vector			m_vecOrigin;
};


// This is the class that legacy tempents use to emit particles.
// They use it in this pattern:
//	CTEParticleRenderer *pRen = CTEParticleRenderer::Create();
//	pRen->AddParticle....
//	pRen->Release();

class CTEParticleRenderer : public CParticleEffect
{
public:
	DECLARE_CLASS( CTEParticleRenderer, CParticleEffect );
	virtual						~CTEParticleRenderer();

	// Create a CTEParticleRenderer. Pass in your sort origin (m_vecOrigin).
	static CSmartPtr<CTEParticleRenderer>	Create( const char *pDebugName, const Vector &vOrigin );

	StandardParticle_t*		AddParticle();

	CParticleMgr*			GetParticleMgr();

	void					SetParticleType( StandardParticle_t *pParticle, ptype_t type );
	ptype_t					GetParticleType( StandardParticle_t *pParticle );

	// Get/set lifetime. Note: lifetime here is a counter. You set it to a value and it 
	// counts down and disappears after that long.
	void					SetParticleLifetime( StandardParticle_t *pParticle, float lifetime );
	float					GetParticleLifetime( StandardParticle_t *pParticle );


// IParticleEffect overrides.	
public:
	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

private:
					CTEParticleRenderer( const char *pDebugName );
					CTEParticleRenderer( const CTEParticleRenderer & ); // not defined, not accessible

	int				m_nActiveParticles;
	float			m_ParticleSize;
	PMaterialHandle	m_MaterialHandle;
};



// ------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------ //
inline void CTEParticleRenderer::SetParticleType(StandardParticle_t *pParticle, ptype_t type)
{
	pParticle->m_EffectData = (unsigned char)type;
}

inline ptype_t CTEParticleRenderer::GetParticleType(StandardParticle_t *pParticle)
{
	return (ptype_t)pParticle->m_EffectData;
}

// Get/set lifetime. Note that 
inline void CTEParticleRenderer::SetParticleLifetime(StandardParticle_t *pParticle, float lifetime)
{
	pParticle->m_Lifetime = lifetime;
}

inline float CTEParticleRenderer::GetParticleLifetime(StandardParticle_t *pParticle)
{
	return pParticle->m_Lifetime;
}


#endif



