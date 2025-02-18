//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "bot/hl2mp_bot.h"
#include "hl2mp_bot_proxy.h"
#include "hl2mp_bot_generator.h"


BEGIN_DATADESC( CHL2MPBotProxy )
	DEFINE_KEYFIELD( m_botName,				FIELD_STRING,	"bot_name" ),
	DEFINE_KEYFIELD( m_teamName,			FIELD_STRING,	"team" ),
	DEFINE_KEYFIELD( m_respawnInterval,		FIELD_FLOAT,	"respawn_interval" ),
	DEFINE_KEYFIELD( m_actionPointName,		FIELD_STRING,	"action_point" ),
	DEFINE_KEYFIELD( m_spawnOnStart,		FIELD_STRING,	"spawn_on_start" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeam", InputSetTeam ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMovementGoal", InputSetMovementGoal ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Spawn", InputSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Delete", InputDelete ),

	DEFINE_OUTPUT( m_onSpawned, "OnSpawned" ),
	DEFINE_OUTPUT( m_onInjured, "OnInjured" ),
	DEFINE_OUTPUT( m_onKilled, "OnKilled" ),
	DEFINE_OUTPUT( m_onAttackingEnemy, "OnAttackingEnemy" ),
	DEFINE_OUTPUT( m_onKilledEnemy, "OnKilledEnemy" ),

	DEFINE_THINKFUNC( Think ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_proxy, CHL2MPBotProxy );



//------------------------------------------------------------------------------
CHL2MPBotProxy::CHL2MPBotProxy( void )
{
	V_strcpy_safe( m_botName, "HL2MPBot" );
	V_strcpy_safe( m_teamName, "auto" );
	m_bot = NULL;
	m_moveGoal = NULL;
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::Think( void )
{

}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::InputSetTeam( inputdata_t &inputdata )
{
	const char *teamName = inputdata.value.String();
	if ( teamName && teamName[0] )
	{
		V_strcpy_safe( m_teamName, teamName );

		// if m_bot exists, tell it to change team
		if ( m_bot != NULL )
		{
			m_bot->HandleCommand_JoinTeam( Bot_GetTeamByName( m_teamName ) );
		}
	}
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::InputSetMovementGoal( inputdata_t &inputdata )
{
	const char *entityName = inputdata.value.String();
	if ( entityName && entityName[0] )
	{
		m_moveGoal = dynamic_cast< CHL2MPBotActionPoint * >( gEntList.FindEntityByName( NULL, entityName ) );

		// if m_bot exists, tell it to move to the new action point
		if ( m_bot != NULL )
		{
			m_bot->SetActionPoint( (CHL2MPBotActionPoint *)m_moveGoal.Get() );
		}
	}
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::InputSpawn( inputdata_t &inputdata )
{
	m_bot = NextBotCreatePlayerBot< CHL2MPBot >( m_botName );
	if ( m_bot != NULL )
	{
		m_bot->SetSpawnPoint( this );
		m_bot->SetAttribute( CHL2MPBot::REMOVE_ON_DEATH );
		m_bot->SetAttribute( CHL2MPBot::IS_NPC );

		m_bot->SetActionPoint( (CHL2MPBotActionPoint *)m_moveGoal.Get() );

		m_bot->HandleCommand_JoinTeam( Bot_GetTeamByName( m_teamName ) );

		m_onSpawned.FireOutput( m_bot, m_bot );
	}
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::InputDelete( inputdata_t &inputdata )
{
	if ( m_bot != NULL )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", m_bot->GetUserID() ) );
		m_bot = NULL;
	}
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::OnInjured( void )
{
	m_onInjured.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::OnKilled( void )
{
	m_onKilled.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::OnAttackingEnemy( void )
{
	m_onAttackingEnemy.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CHL2MPBotProxy::OnKilledEnemy( void )
{
	m_onKilledEnemy.FireOutput( this, this );
}

