//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYPARTICLETRAIL_H
#define ENTITYPARTICLETRAIL_H

#ifdef _WIN32
#pragma once
#endif

#include "baseparticleentity.h"
#include "entityparticletrail_shared.h"


//-----------------------------------------------------------------------------
// Spawns particles after  the entity
//-----------------------------------------------------------------------------
class CEntityParticleTrail : public CBaseParticleEntity 
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CEntityParticleTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

public:
	static CEntityParticleTrail	*Create( CBaseEntity *pTarget, const EntityParticleTrailInfo_t &info, CBaseEntity *pConstraint );
	static void Destroy( CBaseEntity *pTarget, const EntityParticleTrailInfo_t &info );

	void Spawn();
	virtual void UpdateOnRemove();

	// Force our constraint entity to be trasmitted
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );

	// Clean up when the entity goes away.
	virtual void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );

private:
	void	AttachToEntity( CBaseEntity *pTarget );
	void	IncrementRefCount();
	void	DecrementRefCount();
	
	CNetworkVar( int, m_iMaterialName );
	CNetworkVarEmbedded( EntityParticleTrailInfo_t, m_Info );
	CNetworkHandle( CBaseEntity, m_hConstraintEntity );

	int	m_nRefCount;
};

#endif // ENTITYPARTICLETRAIL_H
