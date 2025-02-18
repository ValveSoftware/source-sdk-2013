//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_spawner_boss.cpp
// Entity to spawn a Boss
// Michael Booth, February 2011

#include "cbase.h"

#ifdef OBSOLETE_USE_BOSS_ALPHA

#ifdef TF_RAID_MODE

#include "tf_gamerules.h"
#include "tf_spawner_boss.h"
#include "bot_npc/bot_npc.h"


//------------------------------------------------------------------------------

BEGIN_DATADESC( CTFSpawnerBoss )
	DEFINE_KEYFIELD( m_spawnCount,		FIELD_INTEGER,	"count" ),
	DEFINE_KEYFIELD( m_maxActiveCount,	FIELD_INTEGER,	"maxActive" ),
	DEFINE_KEYFIELD( m_spawnInterval,	FIELD_FLOAT,	"interval" ),
	DEFINE_KEYFIELD( m_teamName,		FIELD_STRING,	"team" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_OUTPUT( m_onSpawned, "OnSpawned" ),
	DEFINE_OUTPUT( m_onExpended, "OnExpended" ),
	DEFINE_OUTPUT( m_onBotKilled, "OnBotKilled" ),
	DEFINE_OUTPUT( m_onBotStunned, "OnBotStunned" ),

	DEFINE_THINKFUNC( SpawnerThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_spawner_boss, CTFSpawnerBoss );


//------------------------------------------------------------------------------
CTFSpawnerBoss::CTFSpawnerBoss( void ) 
{
	m_isExpended = false;
	m_spawnCountRemaining = 0;

	SetThink( NULL );
}

//------------------------------------------------------------------------------
void CTFSpawnerBoss::InputEnable( inputdata_t &inputdata )
{
	if ( m_isExpended )
	{
		return;
	}

	SetThink( &CTFSpawnerBoss::SpawnerThink );

	if ( m_spawnCountRemaining )
	{
		// already generating - don't restart count
		return;
	}
	SetNextThink( gpGlobals->curtime );
	m_spawnCountRemaining = m_spawnCount;
}


//------------------------------------------------------------------------------
void CTFSpawnerBoss::InputDisable( inputdata_t &inputdata )
{
	// just stop thinking
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CTFSpawnerBoss::OnBotKilled( CBotNPC *pBot )
{
	m_onBotKilled.FireOutput( pBot, this );
}


//------------------------------------------------------------------------------
void CTFSpawnerBoss::OnBotStunned( CBotNPC *pBot )
{
	m_onBotStunned.FireOutput( pBot, this );
}


//------------------------------------------------------------------------------
void CTFSpawnerBoss::SpawnerThink( void )
{
	// still waiting for the real game to start?
	gamerules_roundstate_t roundState = TFGameRules()->State_Get();
	if ( roundState >= GR_STATE_TEAM_WIN || roundState < GR_STATE_PREROUND || TFGameRules()->IsInWaitingForPlayers() )
	{
		SetNextThink( gpGlobals->curtime + 1.0f );
		return;
	}

	// remove invalid handles from our collection
	int i = 0;
	while( i < m_spawnedBotVector.Count() )
	{
		CHandle< CBotNPC > hBot = m_spawnedBotVector[i];
		if ( hBot == NULL )
		{
			m_spawnedBotVector.FastRemove(i);
			continue;
		}

		++i;
	}

	if ( m_spawnedBotVector.Count() >= m_maxActiveCount )
	{
		// maximum count reached - can't spawn any more
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	// spawn a bot
	CBotNPC *bot = (CBotNPC *)CreateEntityByName( "bot_boss" );
	if ( bot ) 
	{										   
		m_spawnedBotVector.AddToTail( bot );

		int iTeam = TEAM_UNASSIGNED;
		if ( FStrEq( m_teamName.ToCStr(), "red" ) )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( FStrEq( m_teamName.ToCStr(), "blue" ) )
		{
			iTeam = TF_TEAM_BLUE;
		}
		bot->ChangeTeam( iTeam );

		// match bot facing to that of spawner
		bot->SetAbsAngles( GetAbsAngles() );

		bot->SetAbsOrigin( GetAbsOrigin() );

		bot->SetSpawner( this );

		DispatchSpawn( bot );

		m_onSpawned.FireOutput( bot, this );

		--m_spawnCountRemaining;
		if ( m_spawnCountRemaining )
		{
			SetNextThink( gpGlobals->curtime + m_spawnInterval );
		}
		else
		{
			SetThink( NULL );
			m_onExpended.FireOutput( this, this );
			m_isExpended = true;
		}
	}
}

#endif // TF_RAID_MODE

#endif // #ifdef OBSOLETE_USE_BOSS_ALPHA

