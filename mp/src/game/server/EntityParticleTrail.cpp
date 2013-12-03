//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Drops particles where the entity was.
//
//=============================================================================//

#include "cbase.h"
#include "EntityParticleTrail.h"
#include "networkstringtable_gamedll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Used to retire the entity
//-----------------------------------------------------------------------------
static const char *s_pRetireContext = "RetireContext";


//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CEntityParticleTrail )

	DEFINE_FIELD( m_iMaterialName, FIELD_MATERIALINDEX ),
	DEFINE_EMBEDDED( m_Info ),
	DEFINE_FIELD( m_hConstraintEntity, FIELD_EHANDLE ),

	// Think this should be handled by StartTouch/etc.
//	DEFINE_FIELD( m_nRefCount, FIELD_INTEGER ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CEntityParticleTrail, DT_EntityParticleTrail )
	SendPropInt(SENDINFO(m_iMaterialName), MAX_MATERIAL_STRING_BITS, SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( m_Info ), &REFERENCE_SEND_TABLE( DT_EntityParticleTrailInfo ) ),
	SendPropEHandle(SENDINFO(m_hConstraintEntity)),
END_SEND_TABLE()


LINK_ENTITY_TO_CLASS( env_particle_trail, CEntityParticleTrail );


//-----------------------------------------------------------------------------
// Purpose: Creates a flame and attaches it to a target entity.
// Input  : pTarget - 
//-----------------------------------------------------------------------------
CEntityParticleTrail *CEntityParticleTrail::Create( CBaseEntity *pTarget, const EntityParticleTrailInfo_t &info, CBaseEntity *pConstraintEntity )
{
	int iMaterialName = GetMaterialIndex( STRING(info.m_strMaterialName) );

	// Look for other particle trails on the entity + copy state to the new entity
	CEntityParticleTrail *pTrail;
	CBaseEntity *pNext;
	for ( CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		pTrail = dynamic_cast<CEntityParticleTrail*>(pChild);
		if ( pTrail && (pTrail->m_iMaterialName == iMaterialName) )
		{
			// Prevent destruction if it re-enters the field
			pTrail->IncrementRefCount();
			return pTrail;
		}
	}

	pTrail = (CEntityParticleTrail *)CreateEntityByName( "env_particle_trail" );
	if ( pTrail == NULL )
		return NULL;

	pTrail->m_hConstraintEntity = pConstraintEntity;
	pTrail->m_iMaterialName = iMaterialName;
	pTrail->m_Info.CopyFrom(info);
	pTrail->m_nRefCount = 1;
	pTrail->AttachToEntity( pTarget );
	pTrail->Spawn();
	return pTrail;
}


//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CEntityParticleTrail::Spawn()
{
	BaseClass::Spawn();

	/*
	SetThink( &CEntityParticleTrail::BoogieThink );
	SetNextThink( gpGlobals->curtime + 0.01f );

	if ( HasSpawnFlags( SF_RAGDOLL_BOOGIE_ELECTRICAL ) )
	{
		SetContextThink( ZapThink, gpGlobals->curtime + random->RandomFloat( 0.1f, 0.3f ), s_pZapContext ); 
	}
	*/
}


//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CEntityParticleTrail::UpdateOnRemove()
{
	g_pNotify->ClearEntity( this );

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Force our constraint entity to be trasmitted
//-----------------------------------------------------------------------------
void CEntityParticleTrail::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );
	
	// Force our constraint entity to be sent too.
	if ( m_hConstraintEntity )
	{
		m_hConstraintEntity->SetTransmit( pInfo, bAlways );
	}
}


//-----------------------------------------------------------------------------
// Retire
//-----------------------------------------------------------------------------
void CEntityParticleTrail::IncrementRefCount()
{
	if ( m_nRefCount == 0 )
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pRetireContext );
	}
	++m_nRefCount;
}

void CEntityParticleTrail::DecrementRefCount()
{
	--m_nRefCount;
	Assert( m_nRefCount >= 0 );
	if ( m_nRefCount == 0 )
	{
		FollowEntity( NULL );
		g_pNotify->ClearEntity( this );
		SetContextThink( &CEntityParticleTrail::SUB_Remove, gpGlobals->curtime + m_Info.m_flLifetime, s_pRetireContext );
	}
}


//-----------------------------------------------------------------------------
// Clean up when the entity goes away.
//-----------------------------------------------------------------------------
void CEntityParticleTrail::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	BaseClass::NotifySystemEvent( pNotify, eventType, params );
	Assert( pNotify == GetMoveParent() );
	if ( eventType == NOTIFY_EVENT_DESTROY )
	{
		FollowEntity( NULL );
		g_pNotify->ClearEntity( this );
		if ( m_nRefCount != 0 )
		{
			m_nRefCount = 0;
			SetContextThink( &CEntityParticleTrail::SUB_Remove, gpGlobals->curtime + m_Info.m_flLifetime, s_pRetireContext );
		}
	}
}


//-----------------------------------------------------------------------------
// Suppression count
//-----------------------------------------------------------------------------
void CEntityParticleTrail::Destroy( CBaseEntity *pTarget, const EntityParticleTrailInfo_t &info )
{
	int iMaterialName = GetMaterialIndex( STRING(info.m_strMaterialName) );

	// Look for the particle trail attached to this entity + decrease refcount
	CBaseEntity *pNext;
	for ( CBaseEntity *pChild = pTarget->FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		CEntityParticleTrail *pTrail = dynamic_cast<CEntityParticleTrail*>(pChild);
		if ( !pTrail || (pTrail->m_iMaterialName != iMaterialName) )
			continue;

		pTrail->DecrementRefCount();
	}
}


//-----------------------------------------------------------------------------
// Attach to an entity
//-----------------------------------------------------------------------------
void CEntityParticleTrail::AttachToEntity( CBaseEntity *pTarget )
{
	FollowEntity( pTarget );
	g_pNotify->AddEntity( this, pTarget );
}
