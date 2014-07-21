//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef PARTICLE_SMOKEGRENADE_H
#define PARTICLE_SMOKEGRENADE_H


#include "baseparticleentity.h"


#define PARTICLESMOKEGRENADE_ENTITYNAME	"env_particlesmokegrenade"


class ParticleSmokeGrenade : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( ParticleSmokeGrenade, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

						ParticleSmokeGrenade();

	virtual int			UpdateTransmitState( void );

public:

	// Tell the client entity to start filling the volume.
	void				FillVolume();

	// Set the times it fades out at.
	void				SetFadeTime(float startTime, float endTime);

	// Set time to fade out relative to current time
	void				SetRelativeFadeTime(float startTime, float endTime);


public:
	
	// Stage 0 (default): make a smoke trail that follows the entity it's following.
	// Stage 1          : fill a volume with smoke.
	CNetworkVar( unsigned char, m_CurrentStage );

	CNetworkVar( float, m_flSpawnTime );

	// When to fade in and out.
	CNetworkVar( float, m_FadeStartTime );
	CNetworkVar( float, m_FadeEndTime );
};


#endif


