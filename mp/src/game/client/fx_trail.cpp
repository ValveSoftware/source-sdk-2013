//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "fx_trail.h"

C_ParticleTrail::C_ParticleTrail( void )
{
}

C_ParticleTrail::~C_ParticleTrail( void )
{
	if ( m_pParticleMgr )
	{
		m_pParticleMgr->RemoveEffect( &m_ParticleEffect );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the attachment point to spawn at
//-----------------------------------------------------------------------------
void C_ParticleTrail::GetAimEntOrigin( IClientEntity *pAttachedTo, Vector *pAbsOrigin, QAngle *pAbsAngles )
{
	C_BaseEntity *pEnt = pAttachedTo->GetBaseEntity();

	if ( pEnt && (m_nAttachment > 0) )
	{
		pEnt->GetAttachment( m_nAttachment, *pAbsOrigin, *pAbsAngles );
		return;
	}

	BaseClass::GetAimEntOrigin( pAttachedTo, pAbsOrigin, pAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the emission of particles 
//-----------------------------------------------------------------------------
void C_ParticleTrail::SetEmit( bool bEmit )
{
	m_bEmit = bEmit;
}

//-----------------------------------------------------------------------------
// Purpose: Set the spawn rate of the effect
//-----------------------------------------------------------------------------
void C_ParticleTrail::SetSpawnRate( float rate )
{
	m_SpawnRate = rate;
	m_ParticleSpawn.Init( rate );
}

//-----------------------------------------------------------------------------
// Purpose: First sent down from the server
//-----------------------------------------------------------------------------
void C_ParticleTrail::OnDataChanged(DataUpdateType_t updateType)
{
	C_BaseEntity::OnDataChanged(updateType);

	if ( updateType == DATA_UPDATE_CREATED )
	{
		Start( ParticleMgr(), NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ParticleTrail::Start( CParticleMgr *pParticleMgr, IPrototypeArgAccess *pArgs )
{
	if( pParticleMgr->AddEffect( &m_ParticleEffect, this ) == false )
		return;

	m_pParticleMgr = pParticleMgr;
}

