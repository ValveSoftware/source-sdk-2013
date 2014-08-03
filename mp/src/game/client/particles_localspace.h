//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PARTICLES_LOCALSPACE_H
#define PARTICLES_LOCALSPACE_H
#ifdef _WIN32
#pragma once
#endif

#include "particles_simple.h"

#define	FLE_VIEWMODEL	0x00000001

class CLocalSpaceEmitter : public CSimpleEmitter
{
public:

	DECLARE_CLASS( CLocalSpaceEmitter, CParticleEffect );
	
	static CSmartPtr<CLocalSpaceEmitter> Create( const char *pDebugName, ClientEntityHandle_t hEntity, int nAttachment, int flags = 0 );

	virtual void RenderParticles( CParticleRenderIterator *pIterator );
	virtual void SimulateParticles( CParticleSimulateIterator *pIterator );

	virtual void SetupTransformMatrix( void );
	virtual void Update( float flTimeDelta );

	const matrix3x4_t& GetTransformMatrix() const;	

protected:

	CLocalSpaceEmitter( const char *pDebugName );

	ClientEntityHandle_t m_hEntity;
	int	m_nAttachment;
	int m_fFlags;

private:

	CLocalSpaceEmitter( const CLocalSpaceEmitter & ); // not defined, not accessible

	// This is stored in the ParticleEffectBinding now.
	//matrix3x4_t	m_matTransform;

	//FIXME: Bones here as well...
};

#endif // PARTICLES_LOCALSPACE_H
