//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef PARTICLE_FIRE_H
#define PARTICLE_FIRE_H


#include "baseparticleentity.h"


class CParticleFire : public CBaseParticleEntity
{
	DECLARE_DATADESC();

public:
	CParticleFire();

	DECLARE_CLASS( CParticleFire, CBaseParticleEntity );

					DECLARE_SERVERCLASS();

	// The client shoots a ray out and starts creating fire where it hits.
	CNetworkVector( m_vOrigin );
	CNetworkVector( m_vDirection );
};


#endif



