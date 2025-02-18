//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "in_buttons.h"
#include "movehelper_server.h"
#include "tf_playerclass_shared.h"
#include "datacache/imdlcache.h"
#include "serverbenchmark_base.h"
#include "econ_entity_creation.h"
#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "player.h"


void InitBotTrig( void );
void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CTFPlayer *pBot );

ConVar bot_debug( "bot_debug", "0", FCVAR_CHEAT, "Bot debugging." );
#define DbgBotMsg( pszMsg ) \
	do \
	{  \
		if ( bot_debug.GetBool() ) \
			DevMsg( "[Bot] %s", static_cast<const char *>(pszMsg) ); \
	} while (0)


ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack( "bot_forceattack", "0", 0, "When on, all bots fire their guns." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattack_down( "bot_forceattack_down", "1", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_dontmove( "bot_dontmove", "0", FCVAR_CHEAT );
ConVar bot_saveme( "bot_saveme", "0", FCVAR_CHEAT );
static ConVar bot_mimic_inverse( "bot_mimic_inverse", "0", 0, "Bot uses usercmd of player by index." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "180", 0, "Offsets the bot yaw." );
ConVar bot_selectweaponslot( "bot_selectweaponslot", "-1", FCVAR_CHEAT, "set to weapon slot that bot should switch to." );
ConVar bot_randomnames( "bot_randomnames", "0", FCVAR_CHEAT );
ConVar bot_jump( "bot_jump", "0", FCVAR_CHEAT, "Force all bots to repeatedly jump." );
ConVar bot_crouch( "bot_crouch", "0", FCVAR_CHEAT, "Force all bots to crouch." );

ConVar bot_nav_turnspeed( "bot_nav_turnspeed", "5", FCVAR_CHEAT, "Rate at which bots turn to face their targets." );
ConVar bot_nav_wpdistance( "bot_nav_wpdistance", "16", FCVAR_CHEAT, "Distance to a waypoint within which a bot considers as having reached it." );
ConVar bot_nav_wpdeceldistance( "bot_nav_wpdeceldistance", "128", FCVAR_CHEAT, "Distance to a waypoint to which a bot starts to decelerate to reach it." );
ConVar bot_nav_simplifypaths( "bot_nav_simplifypaths", "1", FCVAR_CHEAT, "If set, bots will skip waypoints if they already see the waypoint post." );
ConVar bot_nav_useoffsetpaths( "bot_nav_useoffsetpaths", "1", FCVAR_CHEAT, "If set, bots will generate waypoints on both sides of portals between waypoints when building paths." );
ConVar bot_nav_offsetpathinset( "bot_nav_offsetpathinset", "20", FCVAR_CHEAT, "Distance into an area that waypoints should be generated when pathfinding through portals." );
ConVar bot_nav_usefeelers( "bot_nav_usefeelers", "1", FCVAR_CHEAT, "If set, bots will extend feelers to their sides to find & avoid upcoming collisions." );
ConVar bot_nav_recomputetime( "bot_nav_recomputetime", "0.5", FCVAR_CHEAT, "Delay before bots recompute their path to targets that have moved when moving to them." );

ConVar bot_com_meleerange( "bot_com_meleerange", "80", FCVAR_CHEAT, "Distance to a target that a melee bot wants to be within to attack." );
ConVar bot_com_wpnrange( "bot_com_wpnrange", "400", FCVAR_CHEAT, "Distance to a target that a ranged bot wants to be within to attack." );
ConVar bot_com_viewrange( "bot_com_viewrange", "2000", FCVAR_CHEAT, "Distance within which bots looking for any enemies will find them." );

extern ConVar bot_mimic;
extern ConVar sv_usercmd_custom_random_seed;

// Utility function to get the specified bot from the command arguments
void GetBotsFromCommand( const CCommand &args, int iArgsRequired, const char *pszUsage, CUtlVector< CTFPlayer* > *botVector )
{
	if ( !botVector)
		return;

	if ( args.ArgC() < iArgsRequired )
	{
		Msg( "Too few parameters. %s\n", pszUsage );
		return;
	}

	const char *pszBotName = args[1];

	if ( FStrEq( pszBotName, "all" ) )
	{
		CUtlVector< CTFPlayer* > playerVector;
		CollectPlayers( &playerVector );
		for ( int i=0; i<playerVector.Count(); ++i )
		{
			if ( playerVector[i]->IsFakeClient() )
			{
				botVector->AddToTail( playerVector[i] );
			}
		}
		return;
	}

	// get the bot's player object
	CTFPlayer *pBot = ToTFPlayer( UTIL_PlayerByName( pszBotName ) );
	if ( !pBot || !pBot->IsFakeClient() )
	{
		Msg( "No bot with name %s\n", args[1] );
		return;
	}

	botVector->AddToTail( pBot );
}

static int BotNumber = 1;
static int g_iNextBotTeam = -1;
static int g_iNextBotClass = -1;

//===================================================================================================================
// Purpose: Mapmaker bot control entity. Used by mapmakers to add & script bot behaviors.
class CTFBotController : public CPointEntity 
{
	DECLARE_CLASS( CTFBotController, CPointEntity );
public:
	DECLARE_DATADESC();

	// Input.
	void InputCreateBot( inputdata_t &inputdata );
	void InputRespawnBot( inputdata_t &inputdata );
	void InputBotAddCommandMoveToEntity( inputdata_t &inputdata );
	void InputBotAddCommandAttackEntity( inputdata_t &inputdata );
	void InputBotAddCommandSwitchWeapon( inputdata_t &inputdata );
	void InputBotAddCommandDefend( inputdata_t &inputdata );
	void InputBotSetIgnoreHumans( inputdata_t &inputdata );
	void InputBotPreventMovement( inputdata_t &inputdata );
	void InputBotClearQueue( inputdata_t &inputdata );

public:
	COutputEvent			m_outputOnCommandFinished;	// Fired when the entity is done respawning the players.
	CHandle<CTFPlayer>		m_hBot;
	string_t				m_iszBotName;
	int						m_iBotClass;
};

enum botcommands_t
{
	BC_NONE,
	BC_MOVETO_ENTITY,
	BC_MOVETO_POINT,
	BC_ATTACK,
	BC_SWITCH_WEAPON,
	BC_DEFEND,
};

typedef struct botdata_t
{
	CHandle<CTFPlayer>	m_hBot;

	bool			backwards;

	float			nextturntime;
	bool			lastturntoright;

	float			nextstrafetime;
	float			sidemove;

	QAngle			forwardAngle;
	QAngle			lastAngles;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	bool			m_bChoseWeapon;

	bool			m_bWasDead;
	float			m_flDeadTime;

	//------------------------------------------------------
	// Input into the next command
	unsigned short	buttons;

	//------------------------------------------------------
	// Base thinking
	void Bot_AliveThink( QAngle *vecAngles, Vector *vecMove );
	void Bot_AliveWeaponThink( QAngle *vecAngles, Vector *vecMove );
	void Bot_AliveMovementThink( QAngle *vecAngles, Vector *vecMove );
	void Bot_AliveMovementThink_ExtendFeelers( QAngle *vecAngles, Vector *vecMove, Vector *vecCurVelocity );
	void Bot_DeadThink( QAngle *vecAngles, Vector *vecMove );

	void Bot_ItemTestingThink( QAngle *vecAngles, Vector *vecMove );

	//------------------------------------------------------
	// Navigation
	bool FindPathTo( Vector vecTarget );
	bool ComputePathPositions( void );
	bool UpdatePath( void );
	void AdvancePath( void )
	{
		m_iPathIndex++;

		if ( m_iPathIndex >= m_iPathLength )
		{
			if ( RunningMovementCommand() )
			{
				// We're not done if we're moving to an entity
				if ( !CommandHasATarget() && ShouldFinishCommandOnArrival() )
				{
					ResetPath();
					FinishCommand();
				}
				else
				{
					// We finished our path, so find another one.
					m_flRecomputePathAt = 0;
					if ( !UpdatePath() )
					{
						FinishCommand();
					}
				}
			}
		}
	}
	bool HasPath( void )
	{
		return (m_iPathLength > 0);
	}

	void ResetPath( void )
	{
		m_iPathIndex = 0;
		m_iPathLength = 0;
	}

	Vector					m_vecFaceToTarget;
	enum { MAX_PATH_LENGTH = 256 };
	struct ConnectInfo
	{
		CNavArea			*area;										///< the area along the path
		NavTraverseType		how;										///< how to enter this area from the previous one
		Vector				pos;										///< our movement goal position at this point in the path
		float				flOffset;
	};
	ConnectInfo				m_path[MAX_PATH_LENGTH];
	int						m_iPathLength;
	int						m_iPathIndex;
	CNavArea				*m_lastKnownArea;
	float					m_flRecomputePathAt;
	Vector					m_vecLastPathTarget;

	//------------------------------------------------------
	// Sensing
	void					SetIgnoreHumans( bool bIgnore ) { m_bIgnoreHumans = bIgnore; }
	void					SetFrozen( bool bFrozen ) { if ( bFrozen ) m_hBot->AddEFlags( EFL_BOT_FROZEN ); else m_hBot->RemoveEFlags( EFL_BOT_FROZEN );  }
	bool					FindEnemyTarget( void );
	bool					ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd );
	bool					ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd );

	EHANDLE					m_hEnemy;
	bool					m_bIgnoreHumans;

	//------------------------------------------------------
	// Command Queue
	void UpdatePlanForCommand( void );
	void StartNewCommand( void );
	void FinishCommand( void );
	void RunAttackPlan( void );
	void RunDefendPlan( void );

	void AddAttackCommand( CBaseEntity *pTarget );
	void AddMoveToCommand( CBaseEntity *pTarget, Vector vecTarget );
	void AddSwitchWeaponCommand( int iSlot );
	void AddDefendCommand( float flRange );
	void ClearQueue( void );

	bool RunningMovementCommand( void );
	bool CommandHasATarget( void );
	bool ShouldFinishCommandOnArrival( void );

	CBaseEntity *GetCommandTarget( void ) { return m_Commands.Count() ? m_Commands[0].pTarget : NULL; }
	Vector GetCommandPosition( void ) { return m_Commands.Count() ? m_Commands[0].vecTarget : vec3_origin; }

	struct CommandInfo
	{
		botcommands_t		iCommand;
		EHANDLE				pTarget;
		Vector				vecTarget;
		float				flData;
	};
	CUtlVector<CommandInfo>	m_Commands;
	bool					m_bStartedCommand;
	CHandle<CTFBotController>	m_hBotController;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];

inline botdata_t *BotData( CBasePlayer *pBot )
{
	return &g_BotData[ pBot->entindex() - 1 ];
}

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bTargetDummy, bool bFrozen, int iTeam, int iClass, const char *pszCustomName )
{
	InitBotTrig();

	g_iNextBotTeam = iTeam;
	g_iNextBotClass = iClass;

	char botname[ 64 ];
	if ( pszCustomName && pszCustomName[0] )
	{
		Q_strncpy( botname, pszCustomName, sizeof( botname ) );
	}
	else if ( bot_randomnames.GetBool() )
	{
		switch( g_pServerBenchmark->RandomInt(0,5) )
		{
		case 0: Q_snprintf( botname, sizeof( botname ), "Bot" ); break;
		case 1: Q_snprintf( botname, sizeof( botname ), "This is a medium Bot" ); break;
		case 2: Q_snprintf( botname, sizeof( botname ), "This is a super long bot name that is too long for the game to allow" ); break;
		case 3: Q_snprintf( botname, sizeof( botname ), "Another bot" ); break;
		case 4: Q_snprintf( botname, sizeof( botname ), "Yet more Bot names, medium sized" ); break;
		default: Q_snprintf( botname, sizeof( botname ), "B" ); break;
		}
	}
	else
	{
		Q_snprintf( botname, sizeof( botname ), "Bot%02i", BotNumber );
	}

	edict_t *pEdict = engine->CreateFakeClient( botname );
	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a CBasePlayer for the bot, and call spawn
	//ClientPutInServer( pEdict, botname );
	CTFPlayer *pPlayer = ((CTFPlayer *)CBaseEntity::Instance( pEdict ));
	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
	pPlayer->SetPlayerType( CTFPlayer::TEMP_BOT );

	if ( bFrozen )
	{
		pPlayer->AddEFlags( EFL_BOT_FROZEN );
	}

	if ( bTargetDummy )
	{
		pPlayer->SetTargetDummy();
	}

	BotNumber++;

	botdata_t *pBot = BotData(pPlayer);
	pBot->m_hBot = pPlayer;
	pBot->m_bWasDead = false;
	pBot->m_WantedTeam = iTeam;
	pBot->m_WantedClass = iClass;
	pBot->m_bChoseWeapon = false;
	pBot->m_flJoinTeamTime = gpGlobals->curtime + 0.3;
	pBot->m_vecFaceToTarget.Zero();
	pBot->m_lastKnownArea = NULL;
	pBot->m_flRecomputePathAt = 0;
	pBot->ResetPath();
	pBot->m_bIgnoreHumans = false;
	pBot->m_bStartedCommand = false;

	return pPlayer;
}

//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( bot_kick, "Remove a bot by name, or an entire team (\"red\" or \"blue\"), or all bots (\"all\").", FCVAR_CHEAT )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() != 2 )
	{
		DevMsg( "%s <bot name>, \"red\", \"blue\", or \"all\"\n", args.Arg(0) );
		return;
	}

	for( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

		if ( !player )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( player->GetFlags() & FL_FAKECLIENT )
		{
			if ( FStrEq( args.Arg( 1 ), "all" ) || 
				  FStrEq( args.Arg( 1 ), player->GetPlayerName() ) ||
				( FStrEq( args.Arg( 1 ), "red" ) && player->GetTeamNumber() == TF_TEAM_RED ) ||
				( FStrEq( args.Arg( 1 ), "blue" ) && player->GetTeamNumber() == TF_TEAM_BLUE ) )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", player->GetUserID() ) );
			}
		}
	}
}

// Handler for the "bot" command.
CON_COMMAND_F( bot, "Add a bot.", FCVAR_NONE )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	// Allow bots in item testing mode
	if ( !sv_cheats->GetBool() && !TFGameRules()->IsInItemTestingMode() )
		return;

	//CDODPlayer *pPlayer = CDODPlayer::Instance( UTIL_GetCommandClientIndex() );

	// The bot command uses switches like command-line switches.
	// -count <count> tells how many bots to spawn.
	// -team <index> selects the bot's team. Default is -1 which chooses randomly.
	//	Note: if you do -team !, then it 
	// -class <index> selects the bot's class. Default is -1 which chooses randomly.
	// -frozen prevents the bots from running around when they spawn in.
	// -name sets the bot's name
	// -targetdummy sets their health to 1,000,000.  Useful for testing damage via hud_damagemeter.
	// -teleport teleports the bot to the location the player is pointing at

	// Look at -count.
	int count = args.FindArgInt( "-count", 1 );
	count = clamp( count, 1, 16 );

	if ( args.FindArg( "-all" ) )
	{
		count = 9;
	}

	// Look at -frozen.
	bool bFrozen = !!args.FindArg( "-frozen" );
	bool bTargetDummy = !!args.FindArg( "-targetdummy" );
	bool bTeleport = !!args.FindArg( "-teleport" );
		
	// Ok, spawn all the bots.
	while ( --count >= 0 )
	{
		// What class do they want?
		int iClass = g_pServerBenchmark->RandomInt( 1, TF_CLASS_COUNT-2 ); // -2 so we skip the Civilian class
		char const *pVal = args.FindArg( "-class" );
		if ( pVal )
		{
			for ( int i=1; i < TF_CLASS_COUNT_ALL; i++ )
			{
				if ( !Q_stricmp(pVal, g_aPlayerClassNames_NonLocalized[i] ) )
				{
					iClass = i;
					break;
				}
			}
		}
		if ( args.FindArg( "-all" ) )
		{
			iClass = 9 - count ;
		}

		int iTeam = TEAM_UNASSIGNED;
		pVal = args.FindArg( "-team" );
		if ( pVal )
		{
			if ( stricmp( pVal, "red" ) == 0 )
			{
				iTeam = TF_TEAM_RED;
			}
			else if ( stricmp( pVal, "spectator" ) == 0 )
			{
				iTeam = TEAM_SPECTATOR;
			}
			else if ( stricmp( pVal, "random" ) == 0 )
			{
				iTeam = g_pServerBenchmark->RandomInt( 0, 100 ) < 50 ? TF_TEAM_BLUE : TF_TEAM_RED;
			}
			else
			{
				iTeam = TF_TEAM_BLUE;
			}
		}

		char const *pName = args.FindArg( "-name" );

		CBasePlayer *pTemp = BotPutInServer( bTargetDummy, bFrozen, iTeam, iClass, pName );

		if ( bTeleport && pTemp )
		{
			CTFPlayer *pBot = ToTFPlayer( pTemp );
			CBasePlayer *pPlayer = UTIL_GetCommandClient();
			if ( pPlayer && pBot )
			{
				trace_t tr;
				Vector forward;

				pPlayer->EyeVectors( &forward );
				UTIL_TraceLine( pPlayer->EyePosition(),
					pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
					pPlayer, COLLISION_GROUP_NONE, &tr );

				if ( tr.fraction != 1.0 )
				{
					botdata_t *pBotData = BotData( pBot );
					if ( pBotData )
					{
						pBotData->m_flJoinTeamTime = gpGlobals->curtime - 0.1;
					}

					const char *pszTeam = "auto";
					switch ( pBotData->m_WantedTeam )
					{
					case TF_TEAM_RED:
						pszTeam = "red";
						break;
					case TF_TEAM_BLUE:
						pszTeam = "blue";
						break;
					case TEAM_SPECTATOR:
						pszTeam = "spectator";
						break;
					default:
						pszTeam = "auto";
						break;
					}
					pBot->HandleCommand_JoinTeam( pszTeam );

					const char *pszClassname = ( pBotData->m_WantedClass == TF_CLASS_RANDOM ) ? "random" : GetPlayerClassData( pBotData->m_WantedClass )->m_szClassName;
					pBot->HandleCommand_JoinClass( pszClassname );

					pBot->ForceRespawn();
					pBot->Teleport( &tr.endpos, NULL, NULL );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
CON_COMMAND_F( bot_hurt, "Hurt a bot by team, or all bots (\"all\").", FCVAR_GAMEDLL )
{
	// Listenserver host or rcon access only!
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		DevMsg( "%s (-team <red/blue/all>>) (-name <name>) (-damage <int>) (-burn)\n", args.Arg(0) );
		return;
	}

	int nDamage = 50;
	const char* pszDamage = args.FindArg( "-damage" );
	if( pszDamage )
	{
		nDamage = atoi(pszDamage);
	}

	bool bBurn = args.FindArg( "-burn" ) != nullptr;

	// Figure out which team if specified
	int iTeam = TEAM_UNASSIGNED;
	const char* pVal = args.FindArg( "-team" );
	if ( pVal )
	{
		if ( stricmp( pVal, "red" ) == 0 )
		{
			iTeam = TF_TEAM_RED;
		}
		else if ( stricmp( pVal, "blue" ) == 0 )
		{
			iTeam = TF_TEAM_BLUE;
		}
		else if ( stricmp( pVal, "all" ) == 0 )
		{
			iTeam = TEAM_ANY;
		}
		else
		{
			iTeam = TEAM_ANY;
		}
	}

	// Get the name if the put one in
	pVal = args.FindArg( "-name" );
	const char *pPlayerName = "";
	if( pVal )
	{
		pPlayerName = pVal;
	}

	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>( UTIL_PlayerByIndex( i ) );

		if ( !player )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( player->GetFlags() & FL_FAKECLIENT )
		{
			if ( iTeam == TEAM_ANY ||
				 FStrEq( pPlayerName, player->GetPlayerName() ) ||
				 ( player->GetTeamNumber() == iTeam ) ||
				 ( player->GetTeamNumber() == iTeam ) )
			{
				player->m_iHealth -= nDamage;

				if ( bBurn )
				{
					CTFPlayer *pBot = ToTFPlayer( player );
					pBot->m_Shared.Burn( nullptr, nullptr, 10.0f );
				}
			}
		}
	}
}

inline bool isTempBot( CTFPlayer* pPlayer )
{
	return ( pPlayer && ( pPlayer->GetFlags() & FL_FAKECLIENT ) && pPlayer->GetPlayerType() == CTFPlayer::TEMP_BOT );
}

//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( isTempBot( pPlayer ) && pPlayer->MyNextBotPointer() == NULL )
		{
			Bot_Think( pPlayer );
		}
	}
}

bool RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	if ( bot_mimic_inverse.GetBool() )
	{
		cmd.forwardmove *= -1;
		cmd.sidemove *= -1;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			sidemove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CTFPlayer *fakeclient, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, float frametime )
{
	if ( !fakeclient )
		return;

	CUserCmd cmd;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	Q_memset( &cmd, 0, sizeof( cmd ) );

	if ( !RunMimicCommand( cmd ) )
	{
		VectorCopy( viewangles, cmd.viewangles );
		cmd.forwardmove = forwardmove;
		cmd.sidemove = sidemove;
		cmd.upmove = upmove;
		cmd.buttons = buttons;
		cmd.impulse = impulse;
		cmd.random_seed = g_pServerBenchmark->RandomInt( 0, 0x7fffffff );
		if ( sv_usercmd_custom_random_seed.GetBool() )
		{
			float fltTimeNow = float( Plat_FloatTime() * 1000.0 );
			cmd.server_random_seed = *reinterpret_cast<int*>( (char*)&fltTimeNow );
		}
		else
		{
			cmd.server_random_seed = cmd.random_seed;
		}
	}

	if ( bot_dontmove.GetBool() )
	{
		cmd.forwardmove = 0;
		cmd.sidemove = 0;
		cmd.upmove = 0;
	}
	else
	{
		float flStunAmount = fakeclient->m_Shared.GetAmountStunned( TF_STUN_MOVEMENT );
		if ( flStunAmount )
		{
			cmd.forwardmove *= (1.0 - flStunAmount);
			cmd.sidemove *= (1.0 - flStunAmount);
		}
	}

	if ( fakeclient->m_Shared.IsControlStunned() || fakeclient->m_Shared.IsLoserStateStunned() )
	{
		cmd.weaponselect = 0;
		cmd.buttons = 0;
		if ( fakeclient->m_Shared.IsControlStunned() )
		{
			cmd.forwardmove = 0;
			cmd.sidemove = 0;
			cmd.upmove = 0;
		}
	}

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CTFPlayer *pBot )
{
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );

	botdata_t *botdata = BotData(pBot);

	Vector vecMove( 0, 0, 0 );
	byte  impulse = 0;
	float frametime = gpGlobals->frametime;
	QAngle vecViewAngles = pBot->EyeAngles();
	botdata->buttons = 0;

	MDLCACHE_CRITICAL_SECTION();

	// Create some random values
	if ( pBot->GetTeamNumber() == TEAM_UNASSIGNED && gpGlobals->curtime > botdata->m_flJoinTeamTime )
	{
		const char *pszTeam = NULL;
		switch ( botdata->m_WantedTeam )
		{
		case TF_TEAM_RED:
			pszTeam = "red";
			break;
		case TF_TEAM_BLUE:
			pszTeam = "blue";
			break;
		case TEAM_SPECTATOR:
			pszTeam = "spectator";
			break;
		default:
			pszTeam = "auto";
			break;
		}
		pBot->HandleCommand_JoinTeam( pszTeam );
	}
	else if ( pBot->GetTeamNumber() != TEAM_UNASSIGNED && pBot->GetPlayerClass()->IsClass( TF_CLASS_UNDEFINED ) )
	{
		// If they're on a team but haven't picked a class, choose a random class..
		const char *pszClassname = ( botdata->m_WantedClass == TF_CLASS_RANDOM ) ? "random" : GetPlayerClassData( botdata->m_WantedClass )->m_szClassName;
		pBot->HandleCommand_JoinClass( pszClassname );
	}
	else if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
	{
		botdata->Bot_AliveThink( &vecViewAngles, &vecMove );
	}
	else
	{
		botdata->Bot_DeadThink( &vecViewAngles, &vecMove );
	}

	RunPlayerMove( pBot, vecViewAngles, vecMove[0], vecMove[1], vecMove[2], botdata->buttons, impulse, frametime );
}

//-----------------------------------------------------------------------------
// Purpose: Handle the Bot AI for a live bot
//-----------------------------------------------------------------------------
void botdata_t::Bot_AliveThink( QAngle *vecAngles, Vector *vecMove )
{
	trace_t trace;

	m_bWasDead = false;

	// In item testing mode, we run custom logic
	if ( TFGameRules()->IsInItemTestingMode() )
	{
		Bot_ItemTestingThink( vecAngles, vecMove );
		return;
	}

	UpdatePlanForCommand();
	Bot_AliveMovementThink( vecAngles, vecMove );
	Bot_AliveWeaponThink( vecAngles, vecMove );

	// Miscellaneous
	if ( bot_saveme.GetInt() > 0 )
	{
		m_hBot->SaveMe();
		bot_saveme.SetValue( bot_saveme.GetInt() - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle movement
//-----------------------------------------------------------------------------
void botdata_t::Bot_AliveMovementThink( QAngle *vecAngles, Vector *vecMove )
{
	if ( m_hBot->IsEFlagSet(EFL_BOT_FROZEN) )
	{
		(*vecMove)[0] = 0;
		(*vecMove)[1] = 0;
		(*vecMove)[2] = 0;
		return;
	}

	if ( bot_jump.GetBool() && m_hBot->GetFlags() & FL_ONGROUND )
	{
		buttons |= IN_JUMP;
	}

	if ( bot_crouch.GetBool() )
	{
		buttons |= IN_DUCK;
	}

	// If we don't have a faceto target, but we're moving to a waypoint, look at that.
	Vector vecFaceTo = m_vecFaceToTarget;
	if ( vecFaceTo == vec3_origin )
	{
		if ( m_hEnemy )
		{
			vecFaceTo = m_hEnemy->EyePosition();
		}
		else if ( GetCommandTarget() )
		{
			vecFaceTo = GetCommandTarget()->EyePosition();
		}
		else if ( HasPath() )
		{
			vecFaceTo = m_path[ m_iPathIndex ].pos;
			vecFaceTo[2] = m_hBot->EyePosition()[2];
		}
	}
	// Should we face something?
	if ( vecFaceTo != vec3_origin )
	{
		Vector vecViewTarget = (vecFaceTo - m_hBot->EyePosition());
		VectorNormalize(vecViewTarget);
		QAngle vecTargetViewAngles;
		VectorAngles( vecViewTarget, Vector(0,0,1), vecTargetViewAngles );
		(*vecAngles)[YAW] = ApproachAngle( vecTargetViewAngles[YAW], (*vecAngles)[YAW], bot_nav_turnspeed.GetFloat() );
	}

	// Are we moving to a waypoint?
	if ( HasPath() )
	{
		m_lastKnownArea = TheNavMesh->GetNavArea( m_hBot->GetAbsOrigin() );
		if ( !UpdatePath() )
		{
			FinishCommand();
			return;
		}

		if ( !m_Commands.Count() )
			return;

		float flDistance = bot_nav_wpdistance.GetFloat();
		CBaseEntity *pTargetEnt = m_Commands[0].pTarget;

		// We might run into our target entity earlier. 
		if ( CommandHasATarget() && ShouldFinishCommandOnArrival() )
		{
			float flEntDistance = pTargetEnt->BoundingRadius() + flDistance;
			flEntDistance *= flEntDistance;
			if ( (pTargetEnt->GetAbsOrigin() - m_hBot->GetAbsOrigin()).Length2DSqr() < flEntDistance )
			{
				ResetPath();
				FinishCommand();
				return;
			}
		}

		// Have we reached the next waypoint?
		flDistance *= flDistance;
		if ( (m_path[ m_iPathIndex ].pos - m_hBot->GetAbsOrigin()).Length2DSqr() < flDistance )
		{
			AdvancePath();
		}
		else if ( bot_nav_simplifypaths.GetBool() )
		{
			// If we can see our next waypoint already, stop moving to this one
			while ( m_iPathLength >= (m_iPathIndex+1) &&
				m_iPathIndex < (m_iPathLength - 1) &&
				m_hBot->FVisible( m_path[ m_iPathIndex+1 ].pos ) )
			{
				AdvancePath();
			}
		}

		if ( bot_debug.GetInt() == 1 )
		{
			for ( int i = 0; i < m_iPathLength-1; i++ )
			{
				NDebugOverlay::Line( m_path[i].pos, m_path[i+1].pos, 0, i == m_iPathIndex ? 255 : 64, 0, true, 0.1 );
				NDebugOverlay::Box( m_path[i].pos, -Vector(3,3,3), Vector(3,3,3), 0, i == m_iPathIndex ? 255 : 64, 0, 0, 0.1 );	
			}
		}

		Vector vecTarget = m_path[ m_iPathIndex ].pos;
		Vector vecDesiredVelocity = (vecTarget - m_hBot->GetAbsOrigin());
		if ( bot_nav_usefeelers.GetBool() && m_iPathIndex < (m_iPathLength - 1) )
		{
			Bot_AliveMovementThink_ExtendFeelers( vecAngles, vecMove, &vecDesiredVelocity );
		}
		flDistance = VectorNormalize( vecDesiredVelocity );

		// If we're approaching our last waypoint, decelerate to the point.
		if ( m_iPathLength == 1 )
		{
			float flDecelDistance = bot_nav_wpdeceldistance.GetFloat();
			float flSpeed = MIN( m_hBot->MaxSpeed() * (flDistance / flDecelDistance), m_hBot->MaxSpeed() );
			vecDesiredVelocity *= flSpeed;
		}
		else
		{
			vecDesiredVelocity *= m_hBot->MaxSpeed();
		}

		if ( bot_debug.GetInt() == 10 )
		{
			NDebugOverlay::HorzArrow( m_hBot->GetAbsOrigin(), m_hBot->GetAbsOrigin() + vecDesiredVelocity, 3, 255, 0, 0, 0, true, 0.1 );
		}

		// Convert the velocity into forward/sidemove
		Vector vecForward, vecRight;
		AngleVectors( *vecAngles, &vecForward, &vecRight, NULL );
		(*vecMove)[0] = DotProduct( vecForward, vecDesiredVelocity );
		(*vecMove)[1] = DotProduct( vecRight, vecDesiredVelocity );
	}
}

//------------------------------------------------------------------------------------------------------------
#define COS_TABLE_SIZE 256
static float cosTable[ COS_TABLE_SIZE ];
static bool bBotTrigInitted = false;

void InitBotTrig( void )
{
	if ( bBotTrigInitted )
		return;
	bBotTrigInitted = true;
	for( int i=0; i<COS_TABLE_SIZE; ++i )
	{
		float angle = (float)(2.0f * M_PI * i / (float)(COS_TABLE_SIZE-1));
		cosTable[i] = (float)cos( angle ); 
	}
}

float BotCOS( float angle )
{
	angle = AngleNormalizePositive( angle );
	int i = (int)( angle * (COS_TABLE_SIZE-1) / 360.0f );
	return cosTable[i];
}

float BotSIN( float angle )
{
	angle = AngleNormalizePositive( angle - 90 );
	int i = (int)( angle * (COS_TABLE_SIZE-1) / 360.0f );
	return cosTable[i];
}

// Find "simple" ground height, treating current nav area as part of the floor
bool GetSimpleGroundHeightWithFloor( CNavArea *pArea, const Vector &pos, float *height, Vector *normal )
{
	if (TheNavMesh->GetSimpleGroundHeight( pos, height, normal ))
	{
		// our current nav area also serves as a ground polygon
		if ( pArea && pArea->IsOverlapping( pos ))
		{
			*height = MAX( (*height), pArea->GetZ( pos ) );
		}

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
// Purpose: Fire feelers out to either side and steer to avoid collisions
//--------------------------------------------------------------------------------------------------------------
void botdata_t::Bot_AliveMovementThink_ExtendFeelers( QAngle *vecAngles, Vector *vecMove, Vector *vecCurVelocity )
{
	VectorNormalize( *vecCurVelocity );

	float flForwardAngle = UTIL_VecToYaw( *vecCurVelocity );

	Vector dir( BotCOS( flForwardAngle ), BotSIN( flForwardAngle ), 0.0f );
	Vector lat( -dir.y, dir.x, 0.0f );

	const float feelerOffset = 20.0f;
	const float feelerLengthRun = 50.0f;	// 100 - too long for tight hallways (cs_747)
	const float feelerHeight = StepHeight + 0.1f;	// if obstacle is lower than StepHeight, we'll walk right over it

	// Feelers must follow floor slope
	float ground;
	Vector normal;
	Vector eye = m_hBot->EyePosition();
	if (GetSimpleGroundHeightWithFloor( m_lastKnownArea, eye, &ground, &normal ) == false)
		return;

	// get forward vector along floor
	dir = CrossProduct( lat, normal );

	// correct the sideways vector
	lat = CrossProduct( dir, normal );

	Vector feet = m_hBot->GetAbsOrigin();
	feet.z += feelerHeight;

	Vector from = feet + feelerOffset * lat;
	Vector to = from + feelerLengthRun * dir;

	bool leftClear = IsWalkableTraceLineClear( from, to, WALK_THRU_DOORS | WALK_THRU_BREAKABLES );
	if ( bot_debug.GetInt() == 3 )
	{
		NDebugOverlay::Line( from, to, leftClear ? 0 : 255, leftClear ? 255 : 0, 0, true, 0.1 );
	}

	from = feet - feelerOffset * lat;
	to = from + feelerLengthRun * dir;

	bool rightClear = IsWalkableTraceLineClear( from, to, WALK_THRU_DOORS | WALK_THRU_BREAKABLES );
	if ( bot_debug.GetInt() == 3 )
	{
		NDebugOverlay::Line( from, to, rightClear ? 0 : 255, rightClear ? 255 : 0, 0, true, 0.1 );
	}

	Vector vecDebug;
	if ( bot_debug.GetInt() == 3 && (!leftClear || !rightClear) )
	{
		vecDebug = (*vecCurVelocity * 100);
		NDebugOverlay::Line( m_hBot->GetAbsOrigin(), m_hBot->GetAbsOrigin() + vecDebug, 0, 0, 255, true, 0.1 );
	}

	const float avoidRange = 300.0f;		// 50, 300
	if (!rightClear)
	{
		if (leftClear)
		{
			// right hit, left clear - veer left
			*vecCurVelocity = (*vecCurVelocity * avoidRange) + avoidRange * lat;
		}
	}
	else if (!leftClear)
	{
		// right clear, left hit - veer right
		*vecCurVelocity = (*vecCurVelocity * avoidRange) - avoidRange * lat;
	}

	if ( bot_debug.GetInt() == 3 && (!leftClear || !rightClear) )
	{
		vecDebug = (*vecCurVelocity);
		VectorNormalize( vecDebug );
		vecDebug *= 100;
		NDebugOverlay::Line( m_hBot->GetAbsOrigin(), m_hBot->GetAbsOrigin() + vecDebug, 64, 64, 255, true, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle weapon switching / firing
//-----------------------------------------------------------------------------
void botdata_t::Bot_AliveWeaponThink( QAngle *vecAngles, Vector *vecMove )
{
	if ( bot_selectweaponslot.GetInt() >= 0 )
	{
		int slot = bot_selectweaponslot.GetInt();

		CBaseCombatWeapon *pWpn = m_hBot->Weapon_GetSlot( slot );

		if ( pWpn )
		{
			m_hBot->Weapon_Switch( pWpn );
		}

		bot_selectweaponslot.SetValue( -1 );
	}

	const char *pszWeapon = bot_forcefireweapon.GetString();
	if ( (pszWeapon && pszWeapon[0]) || g_pServerBenchmark->IsBenchmarkRunning() )
	{
		// If bots are being forced to fire a weapon, see if I have it
		// Manually look through weapons to ignore subtype			
		CBaseCombatWeapon *pWeapon = NULL;

		if ( g_pServerBenchmark->IsBenchmarkRunning() )
		{
			if ( !m_bChoseWeapon )
			{
				m_bChoseWeapon = true;

				// Choose any weapon out of the available ones.
				CUtlVector<CBaseCombatWeapon*> weapons;
				for (int i=0;i<MAX_WEAPONS;i++) 
				{
					if ( m_hBot->GetWeapon(i) )
					{
						weapons.AddToTail( m_hBot->GetWeapon( i ) );
					}
				}

				if ( weapons.Count() > 0 )
				{
					pWeapon = weapons[ g_pServerBenchmark->RandomInt( 0, weapons.Count() - 1 ) ];
				}
			}
		}
		else
		{
			// Look for a specific weapon name here.
			for (int i=0;i<MAX_WEAPONS;i++) 
			{
				if ( m_hBot->GetWeapon(i) && FClassnameIs( m_hBot->GetWeapon(i), pszWeapon ) )
				{
					pWeapon = m_hBot->GetWeapon(i);
					break;
				}
			}
		}

		if ( pWeapon )
		{
			// Switch to it if we don't have it out
			CBaseCombatWeapon *pActiveWeapon = m_hBot->GetActiveWeapon();

			// Switch?
			if ( pActiveWeapon != pWeapon )
			{
				m_hBot->Weapon_Switch( pWeapon );
			}
			else
			{
				// Start firing
				// Some weapons require releases, so randomise firing
				if ( bot_forceattack_down.GetBool() || (g_pServerBenchmark->RandomFloat(0.0,1.0) > 0.5) )
				{
					buttons |= IN_ATTACK;
				}

				if ( bot_forceattack2.GetBool() )
				{
					buttons |= IN_ATTACK2;
				}
			}
		}
	}

	if ( bot_forceattack.GetInt() )
	{
		if ( bot_forceattack_down.GetBool() || (g_pServerBenchmark->RandomFloat(0.0,1.0) > 0.5) )
		{
			buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Handle the Bot AI for a dead bot
//-----------------------------------------------------------------------------
void botdata_t::Bot_DeadThink( QAngle *vecAngles, Vector *vecMove )
{
	// Wait for Reinforcement wave
	if ( !m_hBot->IsAlive() )
	{
		if ( m_bWasDead )
		{
			// Wait for a few seconds before respawning.
			if ( gpGlobals->curtime - m_flDeadTime > 3 )
			{
				// Respawn the bot
				buttons |= IN_JUMP;
			}
		}
		else
		{
			// Start a timer to respawn them in a few seconds.
			m_bWasDead = true;
			m_flDeadTime = gpGlobals->curtime;
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: sends the specified command from a bot
//------------------------------------------------------------------------------
void cc_bot_sendcommand( const CCommand &args )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 3, "Usage: bot_command <bot name> <command string...>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	const char *commandline = args.GetCommandString();

	// find the rest of the command line past the bot index
	commandline = strstr( commandline, args[2] );
	Assert( commandline );

	int iSize = Q_strlen(commandline) + 1;
	char *pBuf = (char *)malloc(iSize);
	Q_snprintf( pBuf, iSize, "%s", commandline );

	if ( pBuf[iSize-2] == '"' )
	{
		pBuf[iSize-2] = '\0';
	}

	// make a command object with the intended command line
	CCommand command;
	command.Tokenize( pBuf );

	// send the command
	FOR_EACH_VEC( botVector, i )
	{
		TFGameRules()->ClientCommand( botVector[i], command );
	}
}
static ConCommand bot_sendcommand( "bot_command", cc_bot_sendcommand, "<bot id> <command string...>.  Sends specified command on behalf of specified bot", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: Kill the specified bot
//------------------------------------------------------------------------------
void cc_bot_kill( const CCommand &args )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "Usage: bot_kill <bot name>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	FOR_EACH_VEC( botVector, i )
	{
		botVector[i]->CommitSuicide();
	}
}

static ConCommand bot_kill( "bot_kill", cc_bot_kill, "Kills a bot. Usage: bot_kill <bot name>", FCVAR_CHEAT );

//------------------------------------------------------------------------------
// Purpose: Force all bots to swap teams
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_changeteams, "Make all bots change teams", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( isTempBot( pPlayer ) )
		{
			int iTeam = pPlayer->GetTeamNumber();			
			if ( TF_TEAM_BLUE == iTeam || TF_TEAM_RED == iTeam )
			{
				// toggle team between red & blue
				pPlayer->ChangeTeam( TF_TEAM_BLUE + TF_TEAM_RED - iTeam );
			}			
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Refill all bot ammo counts
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_refill, "Refill all bot ammo counts", FCVAR_CHEAT )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( isTempBot( pPlayer ) )
		{
			pPlayer->GiveAmmo( 1000, TF_AMMO_PRIMARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_SECONDARY );
			pPlayer->GiveAmmo( 1000, TF_AMMO_METAL );
			pPlayer->TakeHealth( 999, DMG_GENERIC );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Deliver lethal damage from the player to the specified bot
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_whack, "Deliver lethal damage from player to specified bot. Usage: bot_whack <bot name>", FCVAR_CHEAT )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "Usage: bot_whack <bot name>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	FOR_EACH_VEC( botVector, i )
	{
		CTakeDamageInfo info( botVector[i], pTFPlayer, 1000, DMG_BULLET );
		info.SetInflictor( pTFPlayer->GetActiveTFWeapon() );
		botVector[i]->TakeDamage( info );
	}
}

//------------------------------------------------------------------------------
// Purpose: Force the specified bot to teleport to the specified position
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_teleport, "Teleport the specified bot to the specified position & angles.\n\tFormat: bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>", FCVAR_CHEAT )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 8, "Usage: bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	Vector vecPos( atof(args[2]), atof(args[3]), atof(args[4]) );
	QAngle vecAng( atof(args[5]), atof(args[6]), atof(args[7]) );
	FOR_EACH_VEC( botVector, i )
	{
		botVector[i]->Teleport( &vecPos, &vecAng, NULL );
	}
	
}

//------------------------------------------------------------------------------
// Purpose: Force the specified bot to create & equip an item
//------------------------------------------------------------------------------
void BotGenerateAndWearItem( CTFPlayer *pBot, const char *itemName )
{
	if ( !pBot )
		return;

	CItemSelectionCriteria criteria;
	criteria.SetItemLevel( AE_USE_SCRIPT_VALUE );
	criteria.SetQuality( AE_USE_SCRIPT_VALUE );
	criteria.BAddCondition( "name", k_EOperator_String_EQ, itemName, true );

	CBaseEntity *pItem = ItemGeneration()->GenerateRandomItem( &criteria, pBot->GetAbsOrigin(), vec3_angle );
	if ( pItem )
	{
		// If it's a weapon, remove the current one, and give us this one.
		CBaseEntity	*pExisting = pBot->Weapon_OwnsThisType(pItem->GetClassname());
		if ( pExisting )
		{
			CBaseCombatWeapon *pWpn = dynamic_cast<CBaseCombatWeapon *>(pExisting);
			pBot->Weapon_Detach( pWpn );
			UTIL_Remove( pExisting );
		}

		// Fake global id
		static int s_nFakeID = 1;
		static_cast<CEconEntity*>(pItem)->GetAttributeContainer()->GetItem()->SetItemID( s_nFakeID++ );

		DispatchSpawn( pItem );
		static_cast<CEconEntity*>(pItem)->GiveTo( pBot );

		pBot->PostInventoryApplication();
	}
	else
	{
		{
			Msg( "Failed to create an item named %s\n", itemName );
		}
	}
}

void BotGenerateAndWearItem( CTFPlayer *pBot, CEconItemView *pItem )
{
	int iClass = pBot->GetPlayerClass()->GetClassIndex();
	int iItemSlot = pItem->GetStaticData()->GetLoadoutSlot( iClass );
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( pBot->GetEntityForLoadoutSlot( iItemSlot ) );

	// we need to force translating the name here.
	// GiveNamedItem will not translate if we force creating the item
	const char *pTranslatedWeaponName = TranslateWeaponEntForClass( pItem->GetStaticData()->GetItemClass(), iClass );
	CTFWeaponBase *pNewItem = dynamic_cast<CTFWeaponBase*>( pBot->GiveNamedItem( pTranslatedWeaponName, 0, pItem, true ) );
	if ( pNewItem )
	{
		CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder*>( (CBaseEntity*)pNewItem );
		if ( pBuilder )
		{
			pBuilder->SetSubType( pBot->GetPlayerClass()->GetData()->m_aBuildable[0] );
		}

		// make sure we removed our current weapon				
		if ( pWeapon )
		{
			pBot->Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
		}

		pNewItem->MarkAttachedEntityAsValidated();
		pNewItem->GiveTo( pBot );
	}
	else
	{
		BotGenerateAndWearItem( pBot, pItem->GetItemDefinition()->GetDefinitionName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void BotMirrorPlayerClassAndItems( CTFPlayer *pBot, CTFPlayer *pPlayer )
{
	if ( !pBot || !pPlayer )
		return;

	// Force them to be our class
	if ( pPlayer->GetPlayerClass()->GetClassIndex() != pBot->GetPlayerClass()->GetClassIndex() )
	{
		pBot->AllowInstantSpawn();
		pBot->HandleCommand_JoinClass( pPlayer->GetPlayerClass()->GetName() );
	}

	int nLastSlot = LOADOUT_POSITION_MISC2;


	pBot->RemoveAllItems( false );

	for ( int i = 0; i <= nLastSlot; ++i )
	{
		CEconItemView *pPlayerItem = pPlayer->GetLoadoutItem( pPlayer->GetPlayerClass()->GetClassIndex(), i );
		if ( !pPlayerItem )
			continue;

		BotGenerateAndWearItem( pBot, pPlayerItem );
	}

	TFPlayerClassData_t *pData = pBot->GetPlayerClass()->GetData();
	if ( pData )
	{
		pBot->ManageBuilderWeapons( pData );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_mirror, "Forces the specified bot to be the same class, and use the same items, as you.", FCVAR_CHEAT )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pTFPlayer )
		return;

	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "Usage: bot_mirror <bot name>, or bot_mirror all", &botVector );
	FOR_EACH_VEC( botVector, i )
	{
		BotMirrorPlayerClassAndItems( botVector[i], pTFPlayer );
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_changeclass, "Forces the specified bot to change class (e.g. bot_changeclass bot01 soldier).", FCVAR_CHEAT )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_GetCommandClient() );
	if ( !pTFPlayer )
		return;

	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "", &botVector );
	FOR_EACH_VEC( botVector, i )
	{
		botVector[i]->AllowInstantSpawn();
		botVector[i]->HandleCommand_JoinClass( args[2] );
	}
}


//------------------------------------------------------------------------------
// Purpose: Force the specified bot to select a weapon in the specified slot
//------------------------------------------------------------------------------
CON_COMMAND_F( cc_bot_selectweapon, "Force a bot to select a weapon in a slot. Usage: bot_selectweapon <bot name> <weapon slot>", FCVAR_CHEAT )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 3, "Usage: bot_selectweapon <bot name> <weapon slot>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	int slot = atoi( args[2] );

	FOR_EACH_VEC( botVector, i )
	{
		CBaseCombatWeapon *pWpn = botVector[i]->Weapon_GetSlot( slot );
		if ( pWpn )
		{
			botVector[i]->Weapon_Switch( pWpn );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Force the specified bot to drop all his gear.
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_drop, "Force the specified bot to drop his active weapon. Usage: bot_drop <bot name>", FCVAR_CHEAT )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "Usage: bot_drop <bot name>", &botVector );
	
	FOR_EACH_VEC( botVector, i )
	{
		CBaseCombatWeapon* pWpn = botVector[i]->GetActiveWeapon();
		if ( pWpn )
		{
			UTIL_Remove( pWpn );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: Force the specified bot to move to the point under your crosshair
//------------------------------------------------------------------------------
CON_COMMAND_F( bot_moveto, "Force the specified bot to move to the point under your crosshair. Usage: bot_moveto <bot name>", FCVAR_CHEAT )
{
	CUtlVector< CTFPlayer* > botVector;
	GetBotsFromCommand( args, 2, "Usage: bot_moveto <bot name>", &botVector );
	if ( botVector.IsEmpty() )
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	UTIL_TraceLine( pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		FOR_EACH_VEC( botVector, i )
		{
			botdata_t *botdata = BotData( botVector[i] );
			botdata->FindPathTo( tr.endpos );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool botdata_t::FindPathTo( Vector vecTarget )
{
	m_flRecomputePathAt = gpGlobals->curtime + bot_nav_recomputetime.GetFloat();
	m_vecLastPathTarget = vecTarget;

	ShortestPathCost cost;
	CNavArea *pCurrentArea = TheNavMesh->GetNavArea( m_hBot->GetAbsOrigin() );
	if ( !pCurrentArea )
	{
		Warning( "Bot '%s' not on the nav mesh.\n", m_hBot->GetPlayerName() );
		return false;
	}

	CNavArea *pTargetArea = TheNavMesh->GetNavArea( vecTarget );
	if ( !pTargetArea )
	{
		Warning( "Bot '%s' tried to move to a point that isn't on the nav mesh (%.2f %.2f %.2f).\n", m_hBot->GetPlayerName(), vecTarget.x, vecTarget.y, vecTarget.z );
		return false;
	}

	// If the path stays within a single area, build a single waypoint and we're done.
	if ( pTargetArea == pCurrentArea )
	{
		m_path[0].pos = vecTarget;
		m_path[0].area = pCurrentArea;
		m_path[0].how = NUM_TRAVERSE_TYPES;
		m_iPathLength = 1;
		m_iPathIndex = 0;
		return true;
	}

	if ( NavAreaBuildPath( pCurrentArea, pTargetArea, &vecTarget, cost ) == false )
	{
		Warning( "Bot '%s' couldn't find a path from nav mesh %d to nav mesh %d.\n", m_hBot->GetPlayerName(), pCurrentArea->GetID(), pTargetArea->GetID() );
		return false;
	}

	m_iPathIndex = 0;

	// Count the number of areas in the path
	int count = 0;
	CNavArea *area;
	for( area = pTargetArea; area; area = area->GetParent() )
	{
		++count;
	}

	bool bUseOffsetPaths = bot_nav_useoffsetpaths.GetBool();
	if ( bUseOffsetPaths )
	{
		count = (count * 2) - 1;
	}
	m_iPathLength = count;

	// Move the areas into our path
	for( area = pTargetArea; count && area; area = area->GetParent() )
	{
		--count;
		m_path[ count ].area = area;
		m_path[ count ].how = area->GetParentHow();
		m_path[ count ].flOffset = bUseOffsetPaths ? -bot_nav_offsetpathinset.GetFloat() : 0;

		if ( bUseOffsetPaths && count >= 1 )
		{
			--count;
			m_path[ count ].area = m_path[ count+1 ].area;
			m_path[ count ].how = m_path[ count+1 ].how;
			m_path[ count ].flOffset = m_path[ count+1 ].flOffset * -1;
		}
	}

	if ( !ComputePathPositions() )
	{
		ResetPath();
		return false;
	}

	// append path end position
	m_path[ m_iPathLength ].area = pTargetArea;
	m_path[ m_iPathLength ].pos = vecTarget;
	m_path[ m_iPathLength ].how = NUM_TRAVERSE_TYPES;
	m_path[ m_iPathLength ].flOffset = 0;
	++m_iPathLength;
	return true;
}

//------------------------------------------------------------------------------
// Purpose: Given a path of areas, compute waypoints along the path
//------------------------------------------------------------------------------
bool botdata_t::ComputePathPositions( void )
{
	if (m_iPathLength == 0)
		return false;

	m_path[0].pos = m_hBot->GetAbsOrigin();//m_path[0].area->GetCenter();
	m_path[0].how = NUM_TRAVERSE_TYPES;

	for( int i=1; i<m_iPathLength; ++i )
	{
		const ConnectInfo *from = &m_path[ i-1 ];
		ConnectInfo *to = &m_path[ i ];
		Vector vecFrom = from->pos;

		if ( to->flOffset < 0 && i >= 2 )
		{
			from = &m_path[ i-2 ];
		}

		from->area->ComputeClosestPointInPortal( to->area, (NavDirType)to->how, from->pos, &to->pos );
		to->pos.z = from->area->GetZ( to->pos );

		if ( to->flOffset )
		{
			// Create a waypoint just inside our current area
			AddDirectionVector( &to->pos, (NavDirType)to->how, -to->flOffset );
		}

		if ( bot_debug.GetInt() == 2 )
		{
			NDebugOverlay::HorzArrow( vecFrom, to->pos, 3, 0, 96, 0, 0, true, 10.0 );
		}
	}

	return true;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool botdata_t::UpdatePath( void )
{
	// If we're going to an entity, make sure it hasn't moved.
	if ( CommandHasATarget() )
	{
		if ( m_flRecomputePathAt < gpGlobals->curtime && m_Commands[0].pTarget )
		{
			Vector vecNewTarget = m_Commands[0].pTarget->GetAbsOrigin();
			if ( (m_vecLastPathTarget - vecNewTarget).LengthSqr() > (50 * 50) )
				return FindPathTo( vecNewTarget );
		}
	}
	return true;
}


//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::UpdatePlanForCommand( void )
{
	if ( !m_Commands.Count() )
		return;

	if ( !m_bStartedCommand )
	{
		StartNewCommand();
		m_bStartedCommand = true;
	}

	switch ( m_Commands[0].iCommand )
	{
	case BC_ATTACK:
		{
			RunAttackPlan();
		}
		break;
	case BC_MOVETO_ENTITY:
		{
			if ( !HasPath() )
			{
				if ( FindPathTo( m_Commands[0].pTarget->GetAbsOrigin() ) == false )
				{
					FinishCommand();
				}
			}
		}
		break;
	case BC_MOVETO_POINT:
		{
			if ( !HasPath() )
			{
				if ( FindPathTo( GetCommandPosition() ) == false )
				{
					FinishCommand();
				}
			}
		}
		break;
	case BC_SWITCH_WEAPON:
		{
			CBaseCombatWeapon *pWpn = m_hBot->Weapon_GetSlot( (int)m_Commands[0].flData );
			if ( pWpn )
			{
				m_hBot->Weapon_Switch( pWpn );
			}
			FinishCommand();
		}
		break;
	case BC_DEFEND:
		{
			RunDefendPlan();
		}
		break;
	default:
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::RunAttackPlan( void )
{
	CBaseEntity *pEnt = GetCommandTarget();
	if ( !pEnt )
		return;

	if ( !pEnt->IsAlive() )
	{
		FinishCommand();
		return;
	}

	bool bCanSeeTarget = m_hBot->FVisible( pEnt );
	bool bNeedsAPath = !bCanSeeTarget;

	if ( bCanSeeTarget )
	{
		float flDistance = (pEnt->GetAbsOrigin() - m_hBot->GetAbsOrigin()).LengthSqr();

		// If it's a melee weapon, we need to close. Otherwise, stay at a nice looking distance.
		if ( m_hBot->GetActiveTFWeapon() && m_hBot->GetActiveTFWeapon()->IsMeleeWeapon()  )
		{
			float flRange = bot_com_meleerange.GetFloat();
			flRange *= flRange;
			bNeedsAPath = ( flDistance > flRange );
			if ( !bNeedsAPath )
			{
				buttons |= IN_ATTACK;
			}
		}
		else
		{
			float flRange = bot_com_wpnrange.GetFloat();
			flRange *= flRange;
			bNeedsAPath = ( flDistance > (500 * 500) );
			buttons |= IN_ATTACK;
		}
	}

	if ( bNeedsAPath && !HasPath() )
	{
		if ( FindPathTo( m_Commands[0].pTarget->GetAbsOrigin() ) == false )
		{
			FinishCommand();
			return;
		}
	}
	else if ( !bNeedsAPath && HasPath() )
	{
		ResetPath();
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::RunDefendPlan( void )
{
	if ( bot_debug.GetInt() == 5 )
	{
		NDebugOverlay::Line( m_hBot->GetAbsOrigin(), GetCommandPosition(), 0, 128, 0, true, 0.1 );
	}

	if ( !FindEnemyTarget() )
	{
		if ( !HasPath() )
		{
			// If we're far from our defend point, move back to it.
			float flDistance = (GetCommandPosition() - m_hBot->GetAbsOrigin()).LengthSqr();
			if ( flDistance > (32*32) )
			{
				if ( FindPathTo( GetCommandPosition() ) == false )
				{
					// Can't get back to our defend position
					FinishCommand();
					return;
				}
			}
		}
		return;
	}

	Assert( m_hEnemy.Get() );

	if ( bot_debug.GetInt() == 5 )
	{
		NDebugOverlay::Line( m_hBot->GetAbsOrigin(), m_hEnemy->GetAbsOrigin(), 255, 0, 0, true, 0.1 );
	}

	// We've got an enemy.
	float flDefendRange = m_Commands[0].flData * m_Commands[0].flData;
	float flDistanceToEnemy = (m_hEnemy->GetAbsOrigin() - m_hBot->GetAbsOrigin()).LengthSqr();
	float flDistanceToEnemyFromDefend = (m_hEnemy->GetAbsOrigin() - GetCommandPosition()).LengthSqr();

	bool bNeedsAPath = false;

	// If it's a melee weapon, we need to close. Otherwise, stay at a nice looking distance.
	if ( m_hBot->GetActiveTFWeapon() && m_hBot->GetActiveTFWeapon()->IsMeleeWeapon()  )
	{
		// We can only close if we're allowed to move to the target's distance.
		if ( flDistanceToEnemyFromDefend <= flDefendRange )
		{
			float flRange = bot_com_meleerange.GetFloat();
			flRange *= flRange;
			bNeedsAPath = ( flDistanceToEnemy > flRange );
			if ( !bNeedsAPath )
			{
				buttons |= IN_ATTACK;
			}
		}
	}
	else
	{
		buttons |= IN_ATTACK;
	}

	if ( bNeedsAPath && !HasPath() )
	{
		FindPathTo( m_hEnemy->GetAbsOrigin() );
	}
	else if ( !bNeedsAPath && HasPath() )
	{
		ResetPath();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look for a target
//-----------------------------------------------------------------------------
bool botdata_t::FindEnemyTarget( void )
{
	Vector vecEyePosition = m_hBot->EyePosition();

	// find the enemy team
	int iEnemyTeam = ( m_hBot->GetTeamNumber() == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	CTFTeam *pTeam = TFTeamMgr()->GetTeam( iEnemyTeam );
	if ( !pTeam )
		return false;

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = bot_com_viewrange.GetFloat();
	flMinDist2 *= flMinDist2;
	CBaseEntity *pTargetCurrent = NULL;
	CBaseEntity *pTargetOld = m_hEnemy;
	float flOldTargetDist2 = FLT_MAX;

	// Try to target players first, then objects.  However, if the enemy held was an object it will continue
	// to try and attack it first.
	int nTeamCount = pTeam->GetNumPlayers();
	for ( int iPlayer = 0; iPlayer < nTeamCount; ++iPlayer )
	{
		CTFPlayer *pTargetPlayer = static_cast<CTFPlayer*>( pTeam->GetPlayer( iPlayer ) );
		if ( pTargetPlayer == NULL )
			continue;

		// Make sure the player is alive.
		if ( !pTargetPlayer->IsAlive() )
			continue;

		if ( pTargetPlayer->GetFlags() & FL_NOTARGET )
			continue;

		vecTargetCenter = pTargetPlayer->GetAbsOrigin();
		vecTargetCenter += pTargetPlayer->GetViewOffset();
		VectorSubtract( vecTargetCenter, vecEyePosition, vecSegment );
		float flDist2 = vecSegment.LengthSqr();

		// Check to see if the target is closer than the already validated target.
		if ( flDist2 > flMinDist2 )
			continue;

		// It is closer, check to see if the target is valid.
		if ( ValidTargetPlayer( pTargetPlayer, vecEyePosition, vecTargetCenter ) )
		{
			flMinDist2 = flDist2;
			pTargetCurrent = pTargetPlayer;

			// Store the current target distance if we come across it
			if ( pTargetPlayer == pTargetOld )
			{
				flOldTargetDist2 = flDist2;
			}
		}
	}

	// If we already have a target, don't check objects.
	if ( pTargetCurrent == NULL )
	{
		int nTeamObjectCount = pTeam->GetNumObjects();
		for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
		{
			CBaseObject *pTargetObject = pTeam->GetObject( iObject );
			if ( !pTargetObject )
				continue;

			vecTargetCenter = pTargetObject->GetAbsOrigin();
			vecTargetCenter += pTargetObject->GetViewOffset();
			VectorSubtract( vecTargetCenter, vecEyePosition, vecSegment );
			float flDist2 = vecSegment.LengthSqr();

			// Store the current target distance if we come across it
			if ( pTargetObject == pTargetOld )
			{
				flOldTargetDist2 = flDist2;
			}

			// Check to see if the target is closer than the already validated target.
			if ( flDist2 > flMinDist2 )
				continue;

			// It is closer, check to see if the target is valid.
			if ( ValidTargetObject( pTargetObject, vecEyePosition, vecTargetCenter ) )
			{
				flMinDist2 = flDist2;
				pTargetCurrent = pTargetObject;
			}
		}
	}

	// We have a target.
	if ( pTargetCurrent )
	{
		if ( pTargetCurrent != pTargetOld )
		{
			// flMinDist2 is the new target's distance
			// flOldTargetDist2 is the old target's distance
			// Don't switch unless the new target is closer by some percentage
			if ( flMinDist2 < ( flOldTargetDist2 * 0.75f ) )
			{
				m_hEnemy = pTargetCurrent;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool botdata_t::ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd )
{
	if ( m_bIgnoreHumans && !pPlayer->IsFakeClient() )
		return false;

	// Keep shooting at spies that go invisible after we acquire them as a target.
	if ( pPlayer->m_Shared.GetPercentInvisible() > 0.5 )
		return false;

	// Keep shooting at spies that disguise after we acquire them as at a target.
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == m_hBot->GetTeamNumber() && pPlayer != m_hEnemy )
		return false;

	// Not across water boundary.
	if ( ( m_hBot->GetWaterLevel() == 0 && pPlayer->GetWaterLevel() >= 3 ) || ( m_hBot->GetWaterLevel() == 3 && pPlayer->GetWaterLevel() <= 0 ) )
		return false;

	// Ray trace!!!
	return m_hBot->FVisible( pPlayer, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool botdata_t::ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd )
{
	// Ignore objects being placed, they are not real objects yet.
	if ( pObject->IsPlacing() )
		return false;

	// Ignore sappers.
	if ( pObject->MustBeBuiltOnAttachmentPoint() )
		return false;

	// Not across water boundary.
	if ( ( m_hBot->GetWaterLevel() == 0 && pObject->GetWaterLevel() >= 3 ) || ( m_hBot->GetWaterLevel() == 3 && pObject->GetWaterLevel() <= 0 ) )
		return false;

	if ( pObject->GetObjectFlags() & OF_DOESNT_HAVE_A_MODEL )
		return false;

	// Ray trace.
	return m_hBot->FVisible( pObject, MASK_SHOT | CONTENTS_GRATE );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::StartNewCommand( void )
{
	switch ( m_Commands[0].iCommand )
	{
	case BC_DEFEND:
		{
			// Mark our current position as the point we're defending
			m_Commands[0].vecTarget = m_hBot->GetAbsOrigin();
		}
		break;

	case BC_ATTACK:
	case BC_MOVETO_ENTITY:
	case BC_MOVETO_POINT:
	case BC_SWITCH_WEAPON:
	default:
		break;
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::FinishCommand( void )
{
	if ( m_Commands.Count() > 0 )
	{
		m_hBotController->m_outputOnCommandFinished.FireOutput( m_hBot, m_hBot );
		m_Commands.Remove(0);
	}
	m_bStartedCommand = false;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::AddAttackCommand( CBaseEntity *pTarget )
{
	int iIndex = m_Commands.AddToTail();
	m_Commands[iIndex].iCommand = BC_ATTACK;
	m_Commands[iIndex].pTarget = pTarget;
	m_Commands[iIndex].vecTarget = vec3_origin;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::AddMoveToCommand( CBaseEntity *pTarget, Vector vecTarget )
{
	int iIndex = m_Commands.AddToTail();
	m_Commands[iIndex].iCommand = pTarget ? BC_MOVETO_ENTITY : BC_MOVETO_POINT;
	m_Commands[iIndex].pTarget = pTarget;
	m_Commands[iIndex].vecTarget = vecTarget;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::AddSwitchWeaponCommand( int iSlot )
{
	int iIndex = m_Commands.AddToTail();
	m_Commands[iIndex].iCommand = BC_SWITCH_WEAPON;
	m_Commands[iIndex].flData = iSlot;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::AddDefendCommand( float flRange )
{
	int iIndex = m_Commands.AddToTail();
	m_Commands[iIndex].iCommand = BC_DEFEND;
	m_Commands[iIndex].flData = flRange;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void botdata_t::ClearQueue( void )
{
	FinishCommand();
	m_Commands.Purge();
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool botdata_t::RunningMovementCommand( void )
{
	if ( m_Commands.Count() )
		return ( m_Commands[0].iCommand == BC_MOVETO_ENTITY || m_Commands[0].iCommand == BC_MOVETO_POINT );
	return false;
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool botdata_t::CommandHasATarget( void )
{
	return ( m_Commands.Count() && (m_Commands[0].iCommand == BC_MOVETO_ENTITY || m_Commands[0].iCommand == BC_ATTACK) );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
bool botdata_t::ShouldFinishCommandOnArrival( void )
{
	return ( m_Commands.Count() && (m_Commands[0].iCommand == BC_MOVETO_ENTITY || m_Commands[0].iCommand == BC_MOVETO_POINT) );
}

//-----------------------------------------------------------------------------
// Purpose: Handle movement
//-----------------------------------------------------------------------------
void botdata_t::Bot_ItemTestingThink( QAngle *vecAngles, Vector *vecMove )
{
	switch ( TFGameRules()->ItemTesting_GetBotAnim() )
	{
	default:
	case TI_BOTANIM_IDLE:
		break;

	case TI_BOTANIM_CROUCH:
		buttons |= IN_DUCK;
		break;

	case TI_BOTANIM_RUN:
		break;

	case TI_BOTANIM_CROUCH_WALK:
		buttons |= IN_DUCK;
		break;

	case TI_BOTANIM_JUMP:
		if ( m_hBot->GetFlags() & FL_ONGROUND )
		{
			buttons |= IN_JUMP;
		}
		break;
	}

	if ( TFGameRules()->ItemTesting_GetBotForceFire() )
	{
		// Hack to make them not fire faster than the anims being played?
		//if ( !m_hBot->GetAnimState()->IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
		buttons |= IN_ATTACK;
	}

	if ( TFGameRules()->ItemTesting_GetBotTurntable() )
	{
		(*vecAngles)[YAW] += (1.0f * TFGameRules()->ItemTesting_GetBotAnimSpeed());
		if ( (*vecAngles)[YAW] > 360 )
		{
			(*vecAngles)[YAW] = 0;
		}
	}

	(*vecMove)[0] = 0;
	(*vecMove)[1] = 0;
	(*vecMove)[2] = 0;
}

//===================================================================================================================
// Purpose: Mapmaker bot control entity. Used by mapmakers to add & script bot behaviors.
BEGIN_DATADESC( CTFBotController )
	DEFINE_KEYFIELD( m_iszBotName, FIELD_STRING, "bot_name" ),
	DEFINE_KEYFIELD( m_iBotClass, FIELD_INTEGER, "bot_class" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "CreateBot", InputCreateBot ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RespawnBot", InputRespawnBot ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AddCommandMoveToEntity", InputBotAddCommandMoveToEntity ),
	DEFINE_INPUTFUNC( FIELD_STRING, "AddCommandAttackEntity", InputBotAddCommandAttackEntity ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddCommandSwitchWeapon", InputBotAddCommandSwitchWeapon ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "AddCommandDefend", InputBotAddCommandDefend ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetIgnoreHumans", InputBotSetIgnoreHumans ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PreventMovement", InputBotPreventMovement ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearQueue", InputBotClearQueue ),

	DEFINE_OUTPUT( m_outputOnCommandFinished, "OnCommandFinished" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_controller, CTFBotController );

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputCreateBot( inputdata_t &inputdata )
{
	m_hBot = ToTFPlayer( BotPutInServer( false, false, GetTeamNumber(), m_iBotClass ? m_iBotClass : TF_CLASS_RANDOM, STRING(m_iszBotName) ) );
	if ( m_hBot )
	{
		BotData(m_hBot)->m_hBotController = this;
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputRespawnBot( inputdata_t &inputdata )
{
	if ( !m_hBot->GetPlayerClass() || m_hBot->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
	{
		// Allow them to spawn instantly when they do choose
		m_hBot->AllowInstantSpawn();
		return;
	}

	m_hBot->ForceRespawn();
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotAddCommandMoveToEntity( inputdata_t &inputdata )
{
	const char *pszEntName = inputdata.value.String();
	if ( pszEntName && pszEntName[0] )
	{
		CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, pszEntName );
		if ( pEnt )
		{
			BotData(m_hBot)->AddMoveToCommand( pEnt, vec3_origin );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotAddCommandAttackEntity( inputdata_t &inputdata )
{
	const char *pszEntName = inputdata.value.String();
	if ( pszEntName && pszEntName[0] )
	{
		CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, pszEntName );
		if ( pEnt )
		{
			BotData(m_hBot)->AddAttackCommand( pEnt );
		}
	}
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotAddCommandSwitchWeapon( inputdata_t &inputdata )
{
	BotData(m_hBot)->AddSwitchWeaponCommand( inputdata.value.Int() );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotAddCommandDefend( inputdata_t &inputdata )
{
	BotData(m_hBot)->AddDefendCommand( inputdata.value.Float() );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotSetIgnoreHumans( inputdata_t &inputdata )
{
	BotData(m_hBot)->SetIgnoreHumans( inputdata.value.Int() > 0 );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotPreventMovement( inputdata_t &inputdata )
{
	BotData(m_hBot)->SetFrozen( inputdata.value.Bool() );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CTFBotController::InputBotClearQueue( inputdata_t &inputdata )
{
	BotData(m_hBot)->ClearQueue();
}

