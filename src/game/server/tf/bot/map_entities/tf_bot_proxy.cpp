//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_proxy.cpp
// A Hammer entity that spawns a TFBot and relays events to/from it
// Michael Booth, November 2009

#include "cbase.h"

#include "bot/tf_bot.h"
#include "tf_bot_proxy.h"
#include "tf_bot_generator.h"


BEGIN_DATADESC( CTFBotProxy )
	DEFINE_KEYFIELD( m_botName,				FIELD_STRING,	"bot_name" ),
	DEFINE_KEYFIELD( m_className,			FIELD_STRING,	"class" ),
	DEFINE_KEYFIELD( m_teamName,			FIELD_STRING,	"team" ),
	DEFINE_KEYFIELD( m_respawnInterval,		FIELD_FLOAT,	"respawn_interval" ),
	DEFINE_KEYFIELD( m_actionPointName,		FIELD_STRING,	"action_point" ),
	DEFINE_KEYFIELD( m_spawnOnStart,		FIELD_STRING,	"spawn_on_start" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeam", InputSetTeam ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetClass", InputSetClass ),
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

LINK_ENTITY_TO_CLASS( bot_proxy, CTFBotProxy );



//------------------------------------------------------------------------------
CTFBotProxy::CTFBotProxy( void )
{
	V_strcpy_safe( m_botName, "TFBot" );
	V_strcpy_safe( m_teamName, "auto" );
	V_strcpy_safe( m_className, "auto" );
	m_bot = NULL;
	m_moveGoal = NULL;
	SetThink( NULL );
}


//------------------------------------------------------------------------------
void CTFBotProxy::Think( void )
{

}


//------------------------------------------------------------------------------
void CTFBotProxy::InputSetTeam( inputdata_t &inputdata )
{
	const char *teamName = inputdata.value.String();
	if ( teamName && teamName[0] )
	{
		V_strcpy_safe( m_teamName, teamName );

		// if m_bot exists, tell it to change team
		if ( m_bot != NULL )
		{
			m_bot->HandleCommand_JoinTeam( m_teamName );
		}
	}
}


//------------------------------------------------------------------------------
void CTFBotProxy::InputSetClass( inputdata_t &inputdata )
{
	const char *className = inputdata.value.String();
	if ( className && className[0] )
	{
		V_strcpy_safe( m_className, className );		

		// if m_bot exists, tell it to change class
		if ( m_bot != NULL )
		{
			m_bot->HandleCommand_JoinClass( m_className );
		}
	}
}


//------------------------------------------------------------------------------
void CTFBotProxy::InputSetMovementGoal( inputdata_t &inputdata )
{
	const char *entityName = inputdata.value.String();
	if ( entityName && entityName[0] )
	{
		m_moveGoal = dynamic_cast< CTFBotActionPoint * >( gEntList.FindEntityByName( NULL, entityName ) );

		// if m_bot exists, tell it to move to the new action point
		if ( m_bot != NULL )
		{
			m_bot->SetActionPoint( (CTFBotActionPoint *)m_moveGoal.Get() );
		}
	}
}


//------------------------------------------------------------------------------
void CTFBotProxy::InputSpawn( inputdata_t &inputdata )
{
	m_bot = NextBotCreatePlayerBot< CTFBot >( m_botName );
	if ( m_bot != NULL )
	{
		m_bot->SetSpawnPoint( this );
		m_bot->SetAttribute( CTFBot::REMOVE_ON_DEATH );
		m_bot->SetAttribute( CTFBot::IS_NPC );

		m_bot->SetActionPoint( (CTFBotActionPoint *)m_moveGoal.Get() );

		m_bot->HandleCommand_JoinTeam( m_teamName );
		m_bot->HandleCommand_JoinClass( m_className );

		m_onSpawned.FireOutput( m_bot, m_bot );
	}
}


//------------------------------------------------------------------------------
void CTFBotProxy::InputDelete( inputdata_t &inputdata )
{
	if ( m_bot != NULL )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", m_bot->GetUserID() ) );
		m_bot = NULL;
	}
}


//------------------------------------------------------------------------------
void CTFBotProxy::OnInjured( void )
{
	m_onInjured.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CTFBotProxy::OnKilled( void )
{
	m_onKilled.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CTFBotProxy::OnAttackingEnemy( void )
{
	m_onAttackingEnemy.FireOutput( this, this );
}


//------------------------------------------------------------------------------
void CTFBotProxy::OnKilledEnemy( void )
{
	m_onKilledEnemy.FireOutput( this, this );
}

