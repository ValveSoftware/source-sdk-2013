//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_spawner.cpp
// Entity to spawn one or more templatized entities
// Michael Booth, April 2011

#include "cbase.h"

#include "tf_gamerules.h"
#include "bot/map_entities/tf_spawner.h"


//------------------------------------------------------------------------------
BEGIN_DATADESC( CTFSpawner )
	DEFINE_KEYFIELD( m_spawnCount,		FIELD_INTEGER,	"count" ),
	DEFINE_KEYFIELD( m_maxActiveCount,	FIELD_INTEGER,	"maxActive" ),
	DEFINE_KEYFIELD( m_spawnInterval,	FIELD_FLOAT,	"interval" ),
	DEFINE_KEYFIELD( m_templateName,	FIELD_STRING,	"template" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Reset", InputReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_onExpended, "OnExpended" ),
	DEFINE_OUTPUT( m_onSpawned, "OnSpawned" ),
	DEFINE_OUTPUT( m_onKilled, "OnKilled" ),

	DEFINE_THINKFUNC( SpawnerThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_spawner, CTFSpawner );


//------------------------------------------------------------------------------
CTFSpawner::CTFSpawner( void ) 
{
	Reset();
}


//------------------------------------------------------------------------------
void CTFSpawner::Reset( void )
{
	m_bExpended = false;
	m_spawnCountRemaining = 0;
	m_spawnedVector.RemoveAll();
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CTFSpawner::InputReset( inputdata_t &inputdata )
{
	Reset();
}


//------------------------------------------------------------------------------
void CTFSpawner::InputEnable( inputdata_t &inputdata )
{
	if ( m_bExpended )
	{
		return;
	}

	SetThink( &CTFSpawner::SpawnerThink );

	if ( m_spawnCountRemaining )
	{
		// already generating - don't restart count
		return;
	}

	SetNextThink( gpGlobals->curtime );
	m_spawnCountRemaining = m_spawnCount;

	m_template = dynamic_cast< CTFSpawnTemplate * >( gEntList.FindEntityByName( NULL, m_templateName ) );
	if ( m_template == NULL )
	{
		Warning( "%s failed to find template named '%s'\n", GetClassname(), STRING( m_templateName ) );
	}
}


//------------------------------------------------------------------------------
void CTFSpawner::InputDisable( inputdata_t &inputdata )
{
	// just stop thinking
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CTFSpawner::OnKilled( CBaseEntity *dead )
{
	m_onKilled.FireOutput( dead, this );
}


//------------------------------------------------------------------------------
void CTFSpawner::SpawnerThink( void )
{
	// still waiting for the real game to start?
	gamerules_roundstate_t roundState = TFGameRules()->State_Get();
	if ( roundState >= GR_STATE_TEAM_WIN || roundState < GR_STATE_PREROUND ||  TFGameRules()->IsInWaitingForPlayers() )
	{
		SetNextThink( gpGlobals->curtime + 1.0f );
		return;
	}

	// clean up destroyed children
	for ( int i = 0; i < m_spawnedVector.Count(); )
	{
		CHandle< CBaseEntity > child = m_spawnedVector[i];

		if ( child == NULL )
		{
			m_spawnedVector.FastRemove(i);
			m_onKilled.FireOutput( this, this );
			continue;
		}

		++i;
	}

	if ( m_spawnedVector.Count() >= m_maxActiveCount )
	{
		// reached max simultanous active count
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	if ( m_template == NULL )
	{
		// nothing to spawn!
		return;
	}

	// spawn the entity
	CBaseEntity *child = m_template->Instantiate();
	if ( child )
	{
		m_spawnedVector.AddToTail( child );

		child->SetAbsOrigin( GetAbsOrigin() );
		child->SetAbsAngles( GetAbsAngles() );
		child->SetOwnerEntity( this );

		DispatchSpawn( child );
		m_onSpawned.FireOutput( child, this );

		--m_spawnCountRemaining;
		if ( m_spawnCountRemaining )
		{
			SetNextThink( gpGlobals->curtime + m_spawnInterval );
		}
		else
		{
			SetThink( NULL );
			m_onExpended.FireOutput( this, this );
			m_bExpended = true;
		}
	}
}
