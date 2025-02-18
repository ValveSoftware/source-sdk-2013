//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_boss_battle_logic.cpp
// Boss battle game mode singleton manager
// Michael Booth, April 2011

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "tf_team.h"
#include "bot/map_entities/tf_bot_generator.h"
#include "player_vs_environment/tf_boss_battle_logic.h"

CBossBattleLogic *g_pBossBattleLogic = NULL;


//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
BEGIN_DATADESC( CBossBattleLogic )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_logic_boss_battle, CBossBattleLogic );


ConVar tf_bot_npc_minion_wave_launch_count( "tf_bot_npc_minion_wave_launch_count", "0"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_wave_initial_launch_interval( "tf_bot_npc_minion_wave_initial_launch_interval", "15"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_minion_wave_launch_interval( "tf_bot_npc_minion_wave_launch_interval", "30"/*, FCVAR_CHEAT*/ );

ConVar tf_bot_npc_grunt_wave_launch_count( "tf_bot_npc_grunt_wave_launch_count", "2"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grunt_wave_initial_launch_interval( "tf_bot_grunt_wave_initial_launch_interval", "25"/*, FCVAR_CHEAT*/ );
ConVar tf_bot_npc_grunt_wave_launch_interval( "tf_bot_npc_grunt_wave_launch_interval", "30"/*, FCVAR_CHEAT*/ );


//-------------------------------------------------------------------------
CBossBattleLogic::CBossBattleLogic()
{
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_round_start" );
}


//-------------------------------------------------------------------------
CBossBattleLogic::~CBossBattleLogic()
{
	g_pBossBattleLogic = NULL;
}


//-------------------------------------------------------------------------
void CBossBattleLogic::Spawn( void )
{
	BaseClass::Spawn();

	Reset();

	SetThink( &CBossBattleLogic::Update );
	SetNextThink( gpGlobals->curtime );

	g_pBossBattleLogic = this;
}


//-------------------------------------------------------------------------
void CBossBattleLogic::Reset( void )
{
	// unspawn entire red team
	CTeam *defendingTeam = GetGlobalTeam( TF_TEAM_RED );
	int i;
	for( i=0; i<defendingTeam->GetNumPlayers(); ++i )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", defendingTeam->GetPlayer(i)->GetUserID() ) );
	}

	// remove all minions
	CBaseEntity *minion = NULL;
	while( ( minion = gEntList.FindEntityByClassname( minion, "bot_npc_minion" ) ) != NULL )
	{
		UTIL_Remove( minion );
	}
}


//-------------------------------------------------------------------------
void CBossBattleLogic::OnRoundStart( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBossBattleMode() )
		return;

	Reset();
}


//--------------------------------------------------------------------------------------------------------
void CBossBattleLogic::FireGameEvent( IGameEvent *event )
{
	const char *eventName = event->GetName();

	if ( !Q_strcmp( eventName, "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == TF_TEAM_RED )
		{
		}
	}
	else if ( !Q_strcmp( eventName, "teamplay_round_start" ) )
	{
		OnRoundStart();
	}
}


//--------------------------------------------------------------------------------------------------------
void CBossBattleLogic::Update( void )
{
	VPROF_BUDGET( "CBossBattleLogic::Update", "Game" );

	const float deltaT = 0.33f;
	SetNextThink( gpGlobals->curtime + deltaT );

	if ( !TFGameRules()->IsBossBattleMode() )
		return;
}


#endif // TF_RAID_MODE
