//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// FIXME: Should we just pass the Particle draw members directly as
// arguments to IParticleEffect::SimulateAndRender?

// This file defines and implements the ParticleDraw class, which is used
// by ParticleEffects to render particles. It simply stores render + simulation
// state
//

#ifndef PARTICLEDRAW_H
#define PARTICLEDRAW_H


class IMaterial;
class CMeshBuilder;
class CParticleSubTexture;


class ParticleDraw
{
friend class CParticleEffectBinding;

public:

					ParticleDraw();

	void			Init( CMeshBuilder *pMeshBuilder, IMaterial *pMaterial, float fTimeDelta );

	// Time delta..
	float			GetTimeDelta() const;

	// Get the material being used (mostly useful for getting the tcoord padding).
	//IMaterial*		GetPMaterial();

	// This can return NULL if the particle system is only being simulated.
	CMeshBuilder*	GetMeshBuilder();

	CParticleSubTexture	*m_pSubTexture;

private:
	CMeshBuilder	*m_pMeshBuilder;
	IMaterial		*m_pMaterial;
	float			m_fTimeDelta;
};



// ------------------------------------------------------------------------- //
// Inlines
// ------------------------------------------------------------------------- //

inline ParticleDraw::ParticleDraw()
{
	m_pMaterial = 0;
}

inline void ParticleDraw::Init( CMeshBuilder *pMeshBuilder, IMaterial *pMaterial, float fTimeDelta )
{
	m_pMeshBuilder = pMeshBuilder;
	m_pMaterial = pMaterial;
	m_fTimeDelta = fTimeDelta;
}

inline float ParticleDraw::GetTimeDelta() const
{
	return m_fTimeDelta;
}

inline CMeshBuilder* ParticleDraw::GetMeshBuilder()
{
	return m_pMeshBuilder;
}

#endif

