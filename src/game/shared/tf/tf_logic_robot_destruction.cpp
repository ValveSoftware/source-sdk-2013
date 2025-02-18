//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//

#include "cbase.h"
#include "tf_logic_robot_destruction.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#ifdef  GAME_DLL
	#include "tf_objective_resource.h"
	#include "entity_bonuspack.h"
	#include "pathtrack.h"
	#include "tf_gamestats.h"
#endif

#ifdef GAME_DLL
void cc_tf_rd_max_points_override( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	ConVarRef var( pConVar );
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
			CTFRobotDestructionLogic::GetRobotDestructionLogic()->DBG_SetMaxPoints( var.GetInt() );
}
ConVar tf_rd_max_points_override( "tf_rd_max_points_override", "0", FCVAR_GAMEDLL, "When changed, overrides the current max points", cc_tf_rd_max_points_override );

#if defined( STAGING_ONLY ) || defined( DEBUG )
void cc_tf_rd_score_blue_points( const CCommand &args )
{
	int nPoints = args.ArgC() > 1 ? atoi(args[1]) : 0;
	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( TF_TEAM_BLUE
																		 , nPoints
																		 , SCORE_CORES_COLLECTED
																		 , NULL );
}
ConCommand tf_rd_score_blue_points( "tf_rd_score_blue_points", cc_tf_rd_score_blue_points, "Give blue points.", FCVAR_CHEAT );

void cc_tf_rd_score_red_points( const CCommand &args )
{
	int nPoints = args.ArgC() > 1 ? atoi(args[1]) : 0;

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( TF_TEAM_RED
																		 , nPoints
																		 , SCORE_CORES_COLLECTED
																		 , NULL );
}
ConCommand tf_rd_score_red_points( "tf_rd_score_red_points", cc_tf_rd_score_red_points, "Give red points.", FCVAR_CHEAT );
#endif // STAGING_ONLY
#endif

ConVar tf_rd_robot_attack_notification_cooldown( "tf_rd_robot_attack_notification_cooldown", "10", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_rd_steal_rate( "tf_rd_steal_rate", "0.5", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_rd_points_per_steal( "tf_rd_points_per_steal", "5", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_rd_points_approach_interval( "tf_rd_points_approach_interval", "0.1f", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_rd_points_per_approach( "tf_rd_points_per_approach", "5", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );
ConVar tf_rd_min_points_to_steal( "tf_rd_min_points_to_steal", "25", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY );

#ifdef CLIENT_DLL
ConVar tf_rd_finale_beep_time( "tf_rd_finale_beep_time", "10", FCVAR_ARCHIVE );
#endif

extern RobotData_t* g_RobotData[ NUM_ROBOT_TYPES ];

#define GROUP_RESPAWN_CONTEXT "group_respawn_context"
#define ADD_POINTS_CONTEXT "add_points_context"
#define UPDATE_STOLEN_POINTS_THINK "stolen_points_think"
#define APPROACH_POINTS_THINK "approach_points_think"


IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotDestruction_RobotSpawn, DT_TFRobotDestructionRobotSpawn )

BEGIN_NETWORK_TABLE_NOBASE( CTFRobotDestruction_RobotSpawn, DT_TFRobotDestructionRobotSpawn )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_robot_destruction_robot_spawn, CTFRobotDestruction_RobotSpawn );

BEGIN_DATADESC( CTFRobotDestruction_RobotSpawn )
#ifdef GAME_DLL
	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnRobot", InputSpawnRobot ),

	// Keyfields
	DEFINE_KEYFIELD( m_spawnData.m_eType, FIELD_INTEGER, "type" ),
	DEFINE_KEYFIELD( m_spawnData.m_nRobotHealth, FIELD_INTEGER, "health" ),
	DEFINE_KEYFIELD( m_spawnData.m_nPoints, FIELD_INTEGER, "points" ),
	DEFINE_KEYFIELD( m_spawnData.m_pszGroupName, FIELD_STRING, "spawngroup" ),
	DEFINE_KEYFIELD( m_spawnData.m_nNumGibs, FIELD_INTEGER, "gibs" ),
	DEFINE_KEYFIELD( m_spawnData.m_pszPathName, FIELD_STRING, "startpath" ),

	DEFINE_OUTPUT( m_OnRobotKilled, "OnRobotKilled" ),
#endif
END_DATADESC()

CTFRobotDestruction_RobotSpawn::CTFRobotDestruction_RobotSpawn()
{}

void CTFRobotDestruction_RobotSpawn::Spawn()
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	SetSolid( SOLID_NONE );

	Precache();
#endif
}

void CTFRobotDestruction_RobotSpawn::Activate()
{
	BaseClass::Activate();
#ifdef GAME_DLL
	if ( !m_spawnData.m_pszGroupName || !m_spawnData.m_pszGroupName[0] )
	{
		Assert(0);
		Warning( "%s has no spawn group defined!", STRING(GetEntityName()) );
		return;
	}

	// Make sure the group exists
	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, m_spawnData.m_pszGroupName );
	CTFRobotDestruction_RobotGroup *pGroup  = dynamic_cast<CTFRobotDestruction_RobotGroup*>( pEnt );
	if ( pEnt != pGroup )
	{
		const char *pszMsg = CFmtStr( "%s specified '%s' as its group, but %s is a %s"
									  , STRING( GetEntityName() )
									  , m_spawnData.m_pszGroupName
									  , m_spawnData.m_pszGroupName
									  , pEnt->GetClassname() );
		AssertMsg( false, "%s", pszMsg );
		Warning( "%s", pszMsg );
	}
	
	if ( pGroup )
	{
		// Make sure there's not two with the same name
		Assert( gEntList.FindEntityByName( pGroup, m_spawnData.m_pszGroupName ) == NULL );
		pGroup->AddToGroup( this );
	}
	else
	{
		Assert(0);
		Warning( "Couldn't find robot destruction spawn group named '%s'!\n", m_spawnData.m_pszGroupName );
	}

	// Make sure the path exists
	pEnt = gEntList.FindEntityByName( NULL, m_spawnData.m_pszPathName );
	CPathTrack *pPath = dynamic_cast< CPathTrack * >( pEnt );
	if ( pPath != pEnt )
	{
		const char *pszMsg = CFmtStr( "%s specified '%s' as its first path, but %s is a %s"
									  , STRING( GetEntityName() )
									  , m_spawnData.m_pszPathName
									  , m_spawnData.m_pszPathName
									  , pEnt->GetClassname() );
		AssertMsg( 0, "%s", pszMsg );
		Warning( "%s", pszMsg );
	}
	else if ( pEnt == NULL )
	{ 
		const char *pszMsg = CFmtStr( "%s specified '%s' as its first path, but %s doesn't exist"
									, STRING( GetEntityName() )
									, m_spawnData.m_pszPathName
									, m_spawnData.m_pszPathName );
		AssertMsg( 0, "%s", pszMsg );
		Warning( "%s", pszMsg );
	}
#endif
}


#ifdef GAME_DLL

void CTFRobotDestruction_RobotSpawn::SpawnRobot()
{
	if ( m_hGroup.Get() == NULL )
	{
		Assert(0);
		Warning( "Spawnpoint '%s' tried to spawn a robot, but group name '%s' didnt find any groups!\n", STRING(GetEntityName()), m_spawnData.m_pszGroupName );
		return;
	}

	if ( m_hRobot == NULL )
	{
		m_hRobot = assert_cast< CTFRobotDestruction_Robot* >( CreateEntityByName( "tf_robot_destruction_robot" ) );
		m_hRobot->SetModel( g_RobotData[ m_spawnData.m_eType ]->GetStringData( RobotData_t::MODEL_KEY ) );
		m_hRobot->ChangeTeam( m_hGroup->GetTeamNumber() );
		m_hRobot->SetHealth( m_spawnData.m_nRobotHealth );
		m_hRobot->SetMaxHealth( m_spawnData.m_nRobotHealth );
		m_hRobot->SetGroup( m_hGroup.Get() );	
		m_hRobot->SetSpawn( this );
		m_hRobot->SetRobotSpawnData( m_spawnData );
		m_hRobot->SetName( AllocPooledString(CFmtStr( "%s_robot", STRING(GetEntityName())) ) );
		DispatchSpawn( m_hRobot );

		m_hRobot->SetAbsOrigin( GetAbsOrigin() );
		m_hRobot->SetAbsAngles( GetAbsAngles() );
	}
}

void CTFRobotDestruction_RobotSpawn::InputSpawnRobot( inputdata_t &inputdata )
{
	SpawnRobot();
}

void CTFRobotDestruction_RobotSpawn::OnRobotKilled()
{
	Assert( m_hRobot.Get() );
	ClearRobot();
	m_OnRobotKilled.FireOutput( this, this );
}

void CTFRobotDestruction_RobotSpawn::ClearRobot()
{
	m_hRobot = NULL;
}

void CTFRobotDestruction_RobotSpawn::Precache()
{
	BaseClass::Precache();

	CTFRobotDestruction_Robot::StaticPrecache();

	PrecacheModel( g_RobotData[ m_spawnData.m_eType ]->GetStringData( RobotData_t::MODEL_KEY ) );
	PrecacheModel( g_RobotData[ m_spawnData.m_eType ]->GetStringData( RobotData_t::DAMAGED_MODEL_KEY ) );
}

bool CTFRobotDestruction_RobotSpawn::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}


#endif

IMPLEMENT_AUTO_LIST( IRobotDestructionGroupAutoList );

BEGIN_DATADESC( CTFRobotDestruction_RobotGroup )
#ifdef GAME_DLL
	DEFINE_KEYFIELD( m_iszHudIcon, FIELD_STRING, "hud_icon" ),
	DEFINE_KEYFIELD( m_flRespawnTime, FIELD_FLOAT, "respawn_time" ),
	DEFINE_KEYFIELD( m_nGroupNumber, FIELD_INTEGER, "group_number" ),
	DEFINE_KEYFIELD( m_nTeamNumber, FIELD_INTEGER, "team_number" ),
	DEFINE_KEYFIELD( m_flTeamRespawnReductionScale, FIELD_FLOAT, "respawn_reduction_scale" ),

	DEFINE_OUTPUT( m_OnRobotsRespawn, "OnRobotsRespawn" ),
	DEFINE_OUTPUT( m_OnAllRobotsDead, "OnAllRobotsDead" ),
#endif
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_robot_destruction_spawn_group, CTFRobotDestruction_RobotGroup );
IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotDestruction_RobotGroup, DT_TFRobotDestruction_RobotGroup )

BEGIN_NETWORK_TABLE_NOBASE( CTFRobotDestruction_RobotGroup, DT_TFRobotDestruction_RobotGroup )
#ifdef CLIENT_DLL
	RecvPropString( RECVINFO( m_pszHudIcon ) ),
	RecvPropInt( RECVINFO( m_iTeamNum ) ),
	RecvPropInt( RECVINFO( m_nGroupNumber ) ),
	RecvPropInt( RECVINFO( m_nState ) ),
	RecvPropFloat( RECVINFO( m_flRespawnStartTime ) ),
	RecvPropFloat( RECVINFO( m_flRespawnEndTime ) ),
	RecvPropFloat( RECVINFO( m_flLastAttackedTime ) ),
#else
	SendPropString( SENDINFO( m_pszHudIcon ) ),
	SendPropInt( SENDINFO( m_iTeamNum ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nGroupNumber ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nState ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flRespawnStartTime ), -1, SPROP_NOSCALE  ),
	SendPropFloat( SENDINFO( m_flRespawnEndTime ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flLastAttackedTime ), -1, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

CTFRobotDestruction_RobotGroup::~CTFRobotDestruction_RobotGroup()
{
#ifdef CLIENT_DLL
	IGameEvent *event = gameeventmanager->CreateEvent( "rd_rules_state_changed" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
#endif
}

#ifdef GAME_DLL

float CTFRobotDestruction_RobotGroup::m_sflNextAllowedAttackAlertTime[TF_TEAM_COUNT] = { 0.f, 0.f, 0.f, 0.f };

CTFRobotDestruction_RobotGroup::CTFRobotDestruction_RobotGroup()
	: m_flRespawnTime( 0.f )
	, m_nTeamNumber( 0 )
{
	m_nState.Set( ROBOT_STATE_DEAD );
	m_nGroupNumber.Set( 0 );
	m_flRespawnStartTime.Set( 0.f );
	m_flRespawnEndTime.Set( 1.f );
}

void CTFRobotDestruction_RobotGroup::Spawn()
{
	V_strncpy( m_pszHudIcon.GetForModify(), STRING( m_iszHudIcon ), MAX_PATH );
}

void CTFRobotDestruction_RobotGroup::Activate()
{
	BaseClass::Activate();
	ChangeTeam( m_nTeamNumber );

	memset( m_sflNextAllowedAttackAlertTime, 0.f, sizeof( m_sflNextAllowedAttackAlertTime ) );

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->AddRobotGroup( this );
	}
}

void CTFRobotDestruction_RobotGroup::AddToGroup( CTFRobotDestruction_RobotSpawn * pSpawn )
{
	Assert( m_vecSpawns.Find( pSpawn ) == m_vecSpawns.InvalidIndex() );
	
	pSpawn->SetGroup( this );
	m_vecSpawns.AddToTail( pSpawn );
}

void CTFRobotDestruction_RobotGroup::RemoveFromGroup( CTFRobotDestruction_RobotSpawn * pSpawn )
{
	Assert( m_vecSpawns.Find( pSpawn ) != m_vecSpawns.InvalidIndex() );

	pSpawn->SetGroup( NULL );
	m_vecSpawns.FindAndRemove( pSpawn );
}

void CTFRobotDestruction_RobotGroup::UpdateState()
{
	bool bShielded = false;

	int nAlive = 0;
	FOR_EACH_VEC( m_vecSpawns, i )
	{
		CTFRobotDestruction_Robot* pRobot = m_vecSpawns[ i ]->GetRobot();
		if ( !pRobot )
			continue;

		if ( pRobot->m_lifeState != LIFE_DEAD )
		{
			++nAlive;
			bShielded |= m_vecSpawns[ i ]->GetRobot()->GetShieldedState();
		}
	}

	eRobotUIState eState = ROBOT_STATE_INACIVE;
	if ( bShielded )
	{
		eState = ROBOT_STATE_SHIELDED;
	}
	else if ( nAlive > 0 )
	{
		eState = ROBOT_STATE_ACTIVE;
	}
	else
	{
		eState = ROBOT_STATE_DEAD;
	}

	m_nState.Set( (int)eState );
	m_flRespawnEndTime = GetNextThink( GROUP_RESPAWN_CONTEXT );
}

void CTFRobotDestruction_RobotGroup::OnRobotAttacked()
{
	float& flNextAlertTime = m_sflNextAllowedAttackAlertTime[ GetTeamNumber() ];

	if ( gpGlobals->curtime >= flNextAlertTime )
	{
		flNextAlertTime = gpGlobals->curtime + tf_rd_robot_attack_notification_cooldown.GetFloat();

		CTeamRecipientFilter filter( GetTeamNumber(), true );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_RD_ROBOT_UNDER_ATTACK );
	}

	m_flLastAttackedTime = gpGlobals->curtime;
}

void CTFRobotDestruction_RobotGroup::OnRobotKilled()
{
	UpdateState();

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->ManageGameState();
	}

	// If all our robots are dead, fire the corresponding output
	if ( GetNumAliveBots() == 0 )
	{
		m_OnAllRobotsDead.FireOutput( this, this );
	}
}

void CTFRobotDestruction_RobotGroup::OnRobotSpawned()
{
	UpdateState();
}

void CTFRobotDestruction_RobotGroup::RespawnRobots()
{
	// Clear out our think
	StopRespawnTimer();

	FOR_EACH_VEC( m_vecSpawns, i )
	{
		m_vecSpawns[ i ]->SpawnRobot();
	}

	if ( CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		CTFRobotDestructionLogic::GetRobotDestructionLogic()->ManageGameState();
	}

	m_OnRobotsRespawn.FireOutput( this, this );
}

int CTFRobotDestruction_RobotGroup::GetNumAliveBots() const
{
	int nNumAlive = 0;
	FOR_EACH_VEC( m_vecSpawns, i )
	{
		CTFRobotDestruction_RobotSpawn* pSpawn = m_vecSpawns[i];
		CTFRobotDestruction_Robot *pRobot = pSpawn->GetRobot();
		if ( pRobot && pRobot->m_lifeState != LIFE_DEAD )
		{
			++nNumAlive;
		}
	}

	return nNumAlive;
}

void CTFRobotDestruction_RobotGroup::StopRespawnTimer()
{
	SetContextThink( NULL, TICK_NEVER_THINK, GROUP_RESPAWN_CONTEXT );
}

void CTFRobotDestruction_RobotGroup::StartRespawnTimerIfNeeded( CTFRobotDestruction_RobotGroup *pMasterGroup )
{
	bool bIsMaster = pMasterGroup == this || pMasterGroup == NULL;

	// We're already thinking and we're the master
	if ( GetNextThink( GROUP_RESPAWN_CONTEXT ) != TICK_NEVER_THINK && bIsMaster )
	{
		return;
	}

	// We dont have dead bots
	if ( GetNumAliveBots() != 0 )
	{
		return;
	}

	// Use the master's time if one got passed in
	float flRespawnTime = bIsMaster ? gpGlobals->curtime + m_flRespawnTime : pMasterGroup->GetNextThink( GROUP_RESPAWN_CONTEXT );

	// If this respawn time is different, then mark this time as the respawn start time.  This can
	// get multiple times with the same value, and we dont want to update every time if we dont have to.
	if ( !AlmostEqual( flRespawnTime, GetNextThink( GROUP_RESPAWN_CONTEXT ) ) )
	{
		// Mark this time
		m_flRespawnStartTime = gpGlobals->curtime;
	}

	SetContextThink( &CTFRobotDestruction_RobotGroup::RespawnCountdownFinish, flRespawnTime, GROUP_RESPAWN_CONTEXT );
	m_flRespawnEndTime = flRespawnTime;
}

void CTFRobotDestruction_RobotGroup::RespawnCountdownFinish()
{
	RespawnRobots();
	// Do other stuff?
}

void CTFRobotDestruction_RobotGroup::EnableUberForGroup()
{
	FOR_EACH_VEC( m_vecSpawns, i )
	{
		CTFRobotDestruction_Robot *pRobot = m_vecSpawns[ i ]->GetRobot();
		if ( pRobot )
		{
			pRobot->EnableUber();
		}
	}
}

void CTFRobotDestruction_RobotGroup::DisableUberForGroup()
{
	FOR_EACH_VEC( m_vecSpawns, i )
	{
		CTFRobotDestruction_Robot *pRobot = m_vecSpawns[ i ]->GetRobot();
		if ( pRobot )
		{
			pRobot->DisableUber();
		}
	}
}
#else //GAME_DLL

void CTFRobotDestruction_RobotGroup::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "rd_rules_state_changed" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
}

void CTFRobotDestruction_RobotGroup::SetDormant( bool bDormant )
{
	BaseClass::SetDormant( bDormant );

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_rules_state_changed" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}
#endif


#ifdef GAME_DLL

static CTFRobotDestruction_RobotGroup * GetLowestAlive( const CUtlVector < CTFRobotDestruction_RobotGroup * >& vecGroups )
{
	CTFRobotDestruction_RobotGroup *pLowest = NULL;
	FOR_EACH_VEC( vecGroups, i )
	{
		CTFRobotDestruction_RobotGroup *pGroup = vecGroups[i];
		// Must have some bots alive
		if ( pGroup->GetNumAliveBots() == 0 )
			continue;

		if ( pLowest == NULL || pGroup->GetGroupNumber() < pLowest->GetGroupNumber() )
		{
			pLowest = pGroup;
		}
	}

	return pLowest;
}

static CTFRobotDestruction_RobotGroup * GetHighestDead( const CUtlVector < CTFRobotDestruction_RobotGroup * >& vecGroups )
{
	CTFRobotDestruction_RobotGroup *pHighest = NULL;
	FOR_EACH_VEC( vecGroups, i )
	{
		CTFRobotDestruction_RobotGroup *pGroup = vecGroups[i];
		// Must not have any alive bots
		if ( pGroup->GetNumAliveBots() > 0 )
			continue;

		if ( pHighest == NULL || pGroup->GetGroupNumber() > pHighest->GetGroupNumber() )
		{
			pHighest = pGroup;
		}
	}

	return pHighest;
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFRobotDestructionLogic::CTFRobotDestructionLogic()
{
	Assert( m_sCTFRobotDestructionLogic == NULL );
	m_sCTFRobotDestructionLogic = this;
#ifdef GAME_DLL
	m_nBlueTargetPoints = 0.f;
	m_nRedTargetPoints = 0.f;
	m_flBlueFinaleEndTime = FLT_MAX;
	m_flRedFinaleEndTime = FLT_MAX;
	m_flNextRedRobotAttackedAlertTime = 0.f;
	m_flNextBlueRobotAttackedAlertTime = 0.f;
	memset( m_nNumFlagsOut, 0, sizeof( m_nNumFlagsOut ) );
	m_iszResFile = MAKE_STRING( "resource/UI/HudObjectiveRobotDestruction.res" ); // Can get overridden from the map

	ListenForGameEvent( "teamplay_pre_round_time_left" );
	ListenForGameEvent( "player_spawn" );

	m_mapRateLimitedSounds.SetLessFunc( StringLessThan );
	m_mapRateLimitedSounds.Insert( "RD.TeamScoreCore", new RateLimitedSound_t( 0.001f ) );
	m_mapRateLimitedSounds.Insert( "RD.EnemyScoreCore", new RateLimitedSound_t( 0.001f ) );
	m_mapRateLimitedSounds.Insert( "RD.EnemyStealingPoints", new RateLimitedSound_t( 0.45f ) );
	m_mapRateLimitedSounds.Insert( "MVM.PlayerUpgraded", new RateLimitedSound_t( 0.2f ) );

	m_AnnouncerProgressSound = { "Announcer.OurTeamCloseToWinning", "Announcer.EnemyTeamCloseToWinning" };

	for ( int i = 0 ; i < TF_TEAM_COUNT ; i++ )
	{
		m_eWinningMethod.Set( i, SCORE_UNDEFINED );
	}

#else
	m_flLastTickSoundTime = 0.f;
#endif
}

CTFRobotDestructionLogic::~CTFRobotDestructionLogic()
{
	Assert( m_sCTFRobotDestructionLogic == this );
	if ( m_sCTFRobotDestructionLogic == this )
		m_sCTFRobotDestructionLogic = NULL;

#ifdef GAME_DLL
	m_mapRateLimitedSounds.PurgeAndDeleteElements();
#endif
}

void CTFRobotDestructionLogic::Spawn()
{
	BaseClass::Spawn();
	Precache();

#ifdef GAME_DLL
	V_strncpy( m_szResFile.GetForModify(), STRING( m_iszResFile ), MAX_PATH );
#endif
}

void CTFRobotDestructionLogic::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Announcer.HowToPlayRD" );
	PrecacheScriptSound( "RD.TeamScoreCore" );
	PrecacheScriptSound( "RD.EnemyScoreCore" );
	PrecacheScriptSound( "RD.EnemyStealingPoints" );
	PrecacheScriptSound( "RD.FlagReturn" );
	PrecacheScriptSound( "RD.FinaleMusic" );

#ifdef GAME_DLL
	PrecacheScriptSound( m_AnnouncerProgressSound.m_pszTheirTeam );
	PrecacheScriptSound( m_AnnouncerProgressSound.m_pszYourTeam );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFRobotDestructionLogic::GetRespawnScaleForTeam( int nTeam ) const
{
	if ( nTeam == TF_TEAM_RED )
	{
		return m_flRedTeamRespawnScale; 
	}
	else
	{
		return m_flBlueTeamRespawnScale;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the score for a team
//-----------------------------------------------------------------------------
int CTFRobotDestructionLogic::GetScore( int nTeam ) const
{
	Assert( nTeam == TF_TEAM_RED || nTeam == TF_TEAM_BLUE );
	return nTeam == TF_TEAM_RED ? m_nRedScore.Get() : m_nBlueScore.Get();
}

//-----------------------------------------------------------------------------
// Purpose: Return the target score that their real score will approach
//-----------------------------------------------------------------------------
int	CTFRobotDestructionLogic::GetTargetScore( int nTeam ) const
{
	Assert( nTeam == TF_TEAM_RED || nTeam == TF_TEAM_BLUE );
	return nTeam == TF_TEAM_RED ? m_nRedTargetPoints.Get() : m_nBlueTargetPoints.Get();
}

float CTFRobotDestructionLogic::GetFinaleWinTime( int nTeam ) const
{
	Assert( nTeam == TF_TEAM_RED || nTeam == TF_TEAM_BLUE );
	return nTeam == TF_TEAM_RED ? m_flRedFinaleEndTime.Get() : m_flBlueFinaleEndTime.Get();
}
#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose: Have scores approach target score
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::ApproachTargetScoresThink()
{
	// If the round is not in play, dont do anything with points
	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	// Approach
	int nOldRedScore = m_nRedScore;
	m_nRedScore.Set( ApproachTeamTargetScore( TF_TEAM_RED, m_nRedTargetPoints, m_nRedScore.Get() ) );
	if ( nOldRedScore != m_nRedScore )
	{
		OnRedScoreChanged();
	}

	int m_nOldBlueScore = m_nBlueScore;
	m_nBlueScore.Set( ApproachTeamTargetScore( TF_TEAM_BLUE, m_nBlueTargetPoints, m_nBlueScore.Get() ) );
	if ( m_nOldBlueScore != m_nBlueScore )
	{
		OnBlueScoreChanged();
	}

	// Re-think if something is still off
	if ( m_nBlueTargetPoints != m_nBlueScore.Get() || m_nRedTargetPoints != m_nRedScore.Get() )
	{
		SetContextThink( &CTFRobotDestructionLogic::ApproachTargetScoresThink, gpGlobals->curtime + tf_rd_points_approach_interval.GetFloat(), APPROACH_POINTS_THINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Have score approach target score.  Fire events regarding score.
//-----------------------------------------------------------------------------
int CTFRobotDestructionLogic::ApproachTeamTargetScore( int nTeam, int nApproachScore, int nCurrentScore )
{
	if ( nApproachScore != nCurrentScore )
	{
		// Figure out which events we need
		COutputEvent& eventHitZeroPoints = nTeam == TF_TEAM_RED ? m_OnRedHitZeroPoints : m_OnBlueHitZeroPoints;
		COutputEvent& eventHasPoints = nTeam == TF_TEAM_RED ? m_OnRedHasPoints : m_OnBlueHasPoints;
	
		// Approach by 1 per interval
		int nDelta = clamp( nApproachScore - nCurrentScore, -tf_rd_points_per_approach.GetInt(), tf_rd_points_per_approach.GetInt() );
		int nNewScore = nCurrentScore + nDelta;

		// Enable the appropriate team flag if their score went from below to above min to steal
		if ( nCurrentScore < tf_rd_min_points_to_steal.GetInt() && nNewScore >= tf_rd_min_points_to_steal.GetInt() )
		{
			for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
			{
				CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
				if ( pFlag->GetTeamNumber() == nTeam )
				{
					pFlag->SetDisabled( false );
				}
			}
		}

		if ( nNewScore == m_nMaxPoints )
		{
			if ( nTeam == TF_TEAM_RED )
			{
				m_OnRedHitMaxPoints.FireOutput( this, this );
				m_flRedFinaleEndTime = gpGlobals->curtime + m_flFinaleLength;
				SetContextThink( &CTFRobotDestructionLogic::RedTeamWin, m_flRedFinaleEndTime, "RedWin" );

				if ( m_flBlueFinaleEndTime == FLT_MAX && GetType() == TYPE_ROBOT_DESTRUCTION )
				{
					// Announce the state change
					TFGameRules()->BroadcastSound( 255, "RD.FinaleMusic" );
				}
			}
			else
			{
				m_OnBlueHitMaxPoints.FireOutput( this, this );
				m_flBlueFinaleEndTime = gpGlobals->curtime + m_flFinaleLength;
				SetContextThink( &CTFRobotDestructionLogic::BlueTeamWin, m_flBlueFinaleEndTime, "BlueWin" );

				if ( m_flRedFinaleEndTime == FLT_MAX && GetType() == TYPE_ROBOT_DESTRUCTION )
				{
					// Announce the state change
					TFGameRules()->BroadcastSound( 255, "RD.FinaleMusic" );
				}
			}
		}
		else if ( nCurrentScore == m_nMaxPoints && nNewScore < m_nMaxPoints )
		{
			if ( nTeam == TF_TEAM_RED )
			{
				m_OnRedLeaveMaxPoints.FireOutput( this, this );
				m_flRedFinaleEndTime = FLT_MAX;
				SetContextThink( NULL, 0.f, "RedWin" );
				if ( m_flBlueFinaleEndTime == FLT_MAX )
				{
					CUtlVector< CTFPlayer* > vecAllPlayers;
					CollectHumanPlayers( &vecAllPlayers );

					FOR_EACH_VEC( vecAllPlayers, i )
					{
						CTFPlayer *pPlayer = vecAllPlayers[i];
						pPlayer->StopSound( "RD.FinaleMusic" );
					}
				}
			}
			else
			{
				m_OnBlueLeaveMaxPoints.FireOutput( this, this );
				m_flBlueFinaleEndTime = FLT_MAX;
				SetContextThink( NULL, 0.f, "BlueWin" );
				if ( m_flRedFinaleEndTime == FLT_MAX )
				{
					CUtlVector< CTFPlayer* > vecAllPlayers;
					CollectHumanPlayers( &vecAllPlayers );

					FOR_EACH_VEC( vecAllPlayers, i )
					{
						CTFPlayer *pPlayer = vecAllPlayers[i];
						pPlayer->StopSound( "RD.FinaleMusic" );
					}
				}
			}
		}
		else if ( nNewScore == 0 )
		{
			eventHitZeroPoints.FireOutput( this, this );
		}
		else if ( nCurrentScore == 0 && nNewScore > 0 )
		{
			eventHasPoints.FireOutput( this, this );
		}

		return nNewScore;
	}

	return nCurrentScore;
}

//-----------------------------------------------------------------------------
// Purpose: Score nPoints for nTeam.  Check for a victory.
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::ScorePoints( int nTeam, int nPoints, RDScoreMethod_t eMethod, CTFPlayer *pPlayer )
{
	// If the round is not in play, dont do anything with points
	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( nPoints == 0 )
		return;

	Assert( nTeam == TF_TEAM_RED || nTeam == TF_TEAM_BLUE );

	// Set the target score
	int nTargetScore = 0;
	if ( nTeam == TF_TEAM_RED )
	{
		nTargetScore = m_nRedTargetPoints = clamp ( m_nRedTargetPoints + nPoints, 0, m_nMaxPoints.Get() );
	}
	else
	{
		nTargetScore = m_nBlueTargetPoints = clamp ( m_nBlueTargetPoints + nPoints, 0, m_nMaxPoints.Get() );
	}
		
	if ( GetNextThink( APPROACH_POINTS_THINK ) == TICK_NEVER_THINK )
	{
		SetContextThink( &CTFRobotDestructionLogic::ApproachTargetScoresThink, gpGlobals->curtime + tf_rd_points_approach_interval.GetFloat(), APPROACH_POINTS_THINK );
	}
	
	int nOldScore = nTeam == TF_TEAM_RED ? m_nRedScore.Get() : m_nBlueScore.Get();

	// Can't do anything if we're already at max and adding points
	if ( nOldScore == m_nMaxPoints && nPoints > 0 )
	{
		return;
	}

	// Or if at 0 and substracting points
	if ( nOldScore == 0 && nPoints < 0 )
	{
		return;
	}

	// is this going to cause a win? store the method.
	if ( nOldScore != nTargetScore )
	{
		m_eWinningMethod.Set( nTeam, eMethod );
	}

	int nNewScore = Clamp( nOldScore + nPoints, 0, m_nMaxPoints.Get() );

	// We want to play different sounds based on the player's team
	CUtlVector< CTFPlayer* > vecAllPlayers;
	CollectHumanPlayers( &vecAllPlayers );
	FOR_EACH_VEC( vecAllPlayers, i )
	{
		CTFPlayer* pSoundPlayer = vecAllPlayers[i];
		bool bPositive = ( pSoundPlayer->GetTeamNumber() == nTeam && nPoints > 0 ) || ( pSoundPlayer->GetTeamNumber() != nTeam && nPoints < 0 );
		PlaySoundInfoForScoreEvent( pSoundPlayer, bPositive, nPoints, nTeam, eMethod );
	}

	// Earn 1 score point for every 10 bonus points
	if ( pPlayer && nPoints > 0 )
	{
		CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayer, NULL, ( nPoints ) );
	}

	// Possibly have the announcer speak about how close the team is to winning if the
	// score was made by picking up a power core
	const int nCloseToWinningThreshold = (5.f / 6.f) * m_nMaxPoints;
	if ( eMethod == SCORE_CORES_COLLECTED && ( nOldScore < nCloseToWinningThreshold ) && ( nNewScore >= nCloseToWinningThreshold ) && GetType() == TYPE_ROBOT_DESTRUCTION )
	{
		TFGameRules()->BroadcastSound( nTeam, m_AnnouncerProgressSound.m_pszYourTeam );
		TFGameRules()->BroadcastSound( GetEnemyTeam( nTeam ), m_AnnouncerProgressSound.m_pszTheirTeam );
	}

	short nDelta = nNewScore - nOldScore;
	if ( nDelta != 0 )
	{
		const char *pszEventName = "RDTeamPointsChanged";
		CBroadcastRecipientFilter filter;
		filter.MakeReliable();
		UserMessageBegin( filter, pszEventName );
		WRITE_SHORT( nDelta );
		WRITE_BYTE( nTeam );
		WRITE_BYTE( (int)eMethod );
		MessageEnd();

		if ( pPlayer )
		{
			IGameEvent *pScoreEvent = gameeventmanager->CreateEvent( "rd_player_score_points" );
			if ( pScoreEvent )
			{
				pScoreEvent->SetInt( "player", pPlayer->GetUserID() );
				pScoreEvent->SetInt( "method", (int)eMethod );
				pScoreEvent->SetInt( "amount", nDelta );
		
				gameeventmanager->FireEvent( pScoreEvent );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::InputRoundActivate( inputdata_t &/*inputdata*/ )
{
	FOR_EACH_VEC( m_vecSpawnGroups, i )
	{
		m_vecSpawnGroups[ i ]->RespawnRobots();
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Give us the One True Robot Destruction Llgic
//-----------------------------------------------------------------------------
CTFRobotDestructionLogic* CTFRobotDestructionLogic::GetRobotDestructionLogic()
{
	return m_sCTFRobotDestructionLogic;
}

CTFRobotDestructionLogic* CTFRobotDestructionLogic::m_sCTFRobotDestructionLogic = NULL;

							   
void CTFRobotDestructionLogic::PlaySoundInfoForScoreEvent( CTFPlayer* pPlayer, bool bPositive, int nNewScore, int nTeam, RDScoreMethod_t eMethod )
{
	if ( !pPlayer )
		return;

	eMethod = eMethod == SCORE_UNDEFINED ? (RDScoreMethod_t)m_eWinningMethod[ nTeam ] : eMethod;

	EmitSound_t params;
	float soundlen = 0;
	params.m_flSoundTime = 0;
	params.m_pSoundName = NULL;
	params.m_pflSoundDuration = &soundlen;

	switch ( eMethod )
	{
		case SCORE_CORES_COLLECTED:
		{
			params.m_pSoundName = bPositive ? "RD.TeamScoreCore" : "RD.EnemyScoreCore";
			params.m_nPitch = RemapValClamped( nNewScore, m_nMaxPoints * 0.75, m_nMaxPoints, 100, 120 );
			params.m_nFlags |= SND_CHANGE_PITCH;
			params.m_flVolume = 0.25f;
			params.m_nFlags |= SND_CHANGE_VOL;
			
			break;
		}
		case SCORE_REACTOR_CAPTURED:
		case SCORE_REACTOR_RETURNED:
		{
			params.m_pSoundName = "RD.FlagReturn";
			break;
		}
		case SCORE_REACTOR_STEAL:
		{
			params.m_pSoundName = bPositive ? "MVM.PlayerUpgraded" : "RD.EnemyStealingPoints";
			break;
		}
		default:
		{
			// By default nothing
		}
	}

	if ( params.m_pSoundName )
	{
#ifdef GAME_DLL
		PlaySoundInPlayersEars( pPlayer, params );
#else
		pPlayer->StopSound( params.m_pSoundName );
		CBroadcastRecipientFilter filter;
		pPlayer->EmitSound( filter, pPlayer->entindex(), params );
#endif
	}
}
#ifdef CLIENT_DLL


void CTFRobotDestructionLogic::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	float flSoonestFinale = Min( m_flBlueFinaleEndTime.Get(), m_flRedFinaleEndTime.Get() ) - gpGlobals->curtime;
	if ( flSoonestFinale <= m_flFinaleLength && m_flLastTickSoundTime == 0.f )
	{
		float flFirstBeepTime = flSoonestFinale - tf_rd_finale_beep_time.GetFloat();
		SetNextClientThink( gpGlobals->curtime + flFirstBeepTime );
	}
}

void CTFRobotDestructionLogic::ClientThink()
{
	float flSoonestFinale = Min( m_flBlueFinaleEndTime.Get(), m_flRedFinaleEndTime.Get() ) - gpGlobals->curtime;
	if ( flSoonestFinale <= tf_rd_finale_beep_time.GetFloat() && flSoonestFinale > 0.f)
	{
		SetNextClientThink( gpGlobals->curtime + 1.f );

		// Play a beeping sound that gets louder the closer we get to finishing
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer )
		{
			bool bLastTick = flSoonestFinale <= 1.f;
			float flExcitementScale = RemapValClamped( Bias( 1.f - ( flSoonestFinale / tf_rd_finale_beep_time.GetFloat() ), 0.2f ), 0.f, 1.f, 0.3f, 1.f );
			float soundlen = 0;
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = bLastTick ? "Weapon_Grenade_Det_Pack.Timer" : "RD.FinaleBeep";
			params.m_pflSoundDuration = &soundlen;
			params.m_flVolume = flExcitementScale;
			params.m_nPitch = bLastTick ? PITCH_NORM : PITCH_NORM * ( 1.f + flExcitementScale );
			params.m_nFlags |= SND_CHANGE_VOL | SND_CHANGE_PITCH;
			CBroadcastRecipientFilter filter;
			pPlayer->EmitSound( filter, pPlayer->entindex(), params );
		}
	}
}
#endif

#ifdef GAME_DLL
void CTFRobotDestructionLogic::Activate()
{
	BaseClass::Activate();

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_rules_state_changed" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

void CTFRobotDestructionLogic::FireGameEvent( IGameEvent * event )
{
	const char *pszName = event->GetName();
	if( FStrEq( pszName, "teamplay_pre_round_time_left" ) )
	{
		int nTimeLeft = event->GetInt( "time" );
		// The round has started.  After this point, when players connect and spawn we want to play the sound
		if ( nTimeLeft == 0 )
		{
			m_bEducateNewConnectors = true;
		}
		// At the 20 second mark we want to play a sound for all the players
		else if ( nTimeLeft == 20 )
		{
			CUtlVector< CTFPlayer* > vecAllPlayers;
			CollectHumanPlayers( &vecAllPlayers );

			FOR_EACH_VEC( vecAllPlayers, i )
			{
				CTFPlayer *pPlayer = vecAllPlayers[i];

				// Ony play the sound for players that are alive
				if ( !pPlayer->IsAlive() )
				{
					continue;
				}

				// Only play the sound for players who havent heard it
				if ( m_vecEducatedPlayers.Find( pPlayer ) == m_vecEducatedPlayers.InvalidIndex() )
				{
					// Remember who has heard the sound
					m_vecEducatedPlayers.AddToTail( pPlayer );

					float soundlen = 0;
					EmitSound_t params;
					params.m_flSoundTime = 0;
					params.m_pSoundName = "Announcer.HowToPlayRD";
					params.m_pflSoundDuration = &soundlen;
					PlaySoundInPlayersEars( pPlayer, params );
				}
			}
		}
	}
	else if ( FStrEq( pszName, "player_spawn" ) )
	{
		// If we're not telling players yet, then skip
		if ( !m_bEducateNewConnectors )
			return;

		const int nUserID = event->GetInt( "userid" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( nUserID ) );

		// If the just spawned and havent heard the sound, play the sound
		if ( pPlayer && pPlayer->IsAlive() && m_vecEducatedPlayers.Find( pPlayer ) == m_vecEducatedPlayers.InvalidIndex() )
		{
			// Remember who heard the sound
			m_vecEducatedPlayers.AddToTail( pPlayer );

			float soundlen = 0;
			EmitSound_t params;
			params.m_flSoundTime = 0;
			params.m_pSoundName = "Announcer.HowToPlayRD";
			params.m_pflSoundDuration = &soundlen;
			PlaySoundInPlayersEars( pPlayer, params );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Givin a pointer to a robot, give the next in the list. If NULL,
//			return the first in the list.
//-----------------------------------------------------------------------------
CTFRobotDestruction_Robot* CTFRobotDestructionLogic::IterateRobots( CTFRobotDestruction_Robot* pRobot ) const
{
	int nIndex = m_vecRobots.Find( pRobot );
	// Not found?  Return the head
	if ( nIndex == -1 && m_vecRobots.Count() )
		return m_vecRobots.Head();
	// Found, but at the end?  Return NULL
	if ( (nIndex + 1) >= m_vecRobots.Count() )
		return NULL;
	// Return the next
	return m_vecRobots[ nIndex + 1 ];
}

void CTFRobotDestructionLogic::AddRobotGroup( CTFRobotDestruction_RobotGroup* pGroup )
{
	Assert( m_vecSpawnGroups.Find( pGroup ) == m_vecSpawnGroups.InvalidIndex() );
	FOR_EACH_VEC( m_vecSpawnGroups, i )
	{
		Assert( m_vecSpawnGroups[i]->GetGroupNumber() != pGroup->GetGroupNumber() 
			 || m_vecSpawnGroups[i]->GetTeamNumber() != pGroup->GetTeamNumber() );
	}

	m_vecSpawnGroups.AddToTail( pGroup );

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_rules_state_changed" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

void CTFRobotDestructionLogic::ManageGameState()
{
	// Put all the groups into team-based vectors
	CUtlVector< CTFRobotDestruction_RobotGroup * > vecTeamGroups[ TF_TEAM_COUNT ];
	FOR_EACH_VEC( m_vecSpawnGroups, i )
	{
		vecTeamGroups[ m_vecSpawnGroups[i]->GetTeamNumber() ].AddToTail( m_vecSpawnGroups[i] );
	}

	CTFRobotDestruction_RobotGroup *pLowestAlive[ TF_TEAM_COUNT ];
	CTFRobotDestruction_RobotGroup *pHighestDead[ TF_TEAM_COUNT ];

	// Find the highest group-numbered group with no living bots, and the lowest group-numbered group
	// with any alive bots
	for( int i = 0; i < TF_TEAM_COUNT; ++i )
	{
		pLowestAlive[ i ] = GetLowestAlive( vecTeamGroups[ i ] );
		pHighestDead[ i ] = GetHighestDead( vecTeamGroups[ i ] );
	}

	// Reset respawn bonus times to 0.  They'll get updated below
	m_flRedTeamRespawnScale = m_flBlueTeamRespawnScale = 0.f;

	// Go through and change the state of the bots
	for( int nTeam = 0; nTeam < TF_TEAM_COUNT; ++nTeam )
	{
		// Skip empty groups
		if ( vecTeamGroups[ nTeam ].Count() == 0 )
			continue;

		CTFRobotDestruction_RobotGroup *pLowest = pLowestAlive[ nTeam ];
		CTFRobotDestruction_RobotGroup *pHighest = pHighestDead[ nTeam ];
	
		bool bHighestAlreadyRespawning = false;
		// The highest dead group is the master respawning group
		if ( pHighest )
		{
			bHighestAlreadyRespawning = pHighest->GetNextThink( GROUP_RESPAWN_CONTEXT ) != TICK_NEVER_THINK;
			pHighest->StartRespawnTimerIfNeeded( pHighest );
			if ( nTeam == TF_TEAM_RED )
			{
				m_flRedTeamRespawnScale = pHighest->GetTeamRespawnScale();
			}
			else
			{
				m_flBlueTeamRespawnScale = pHighest->GetTeamRespawnScale();
			}
		}
		// The lowest alive group is the only non-uber group
		if ( pLowest )
		{
			pLowest->DisableUberForGroup();
		}

		bool bAllDead = true;
		FOR_EACH_VEC( vecTeamGroups[ nTeam ], i )
		{
			CTFRobotDestruction_RobotGroup *pGroup = vecTeamGroups[ nTeam ][ i ];
			
			bAllDead &= pGroup->GetNumAliveBots() == 0;

			// The non-lowest alive groups are ubered
			if ( pGroup != pLowest )
			{
				pGroup->EnableUberForGroup();
			}

			// The non-highest dead groups respawn when the highest-dead group respawns
			if ( pGroup != pHighest )
			{
				pGroup->StartRespawnTimerIfNeeded( pHighest );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Plays a sound in a player's ears
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::PlaySoundInPlayersEars( CTFPlayer* pPlayer, const EmitSound_t& params ) const
{
	int nIndex = m_mapRateLimitedSounds.Find( params.m_pSoundName );
	if ( nIndex != m_mapRateLimitedSounds.InvalidIndex() )
	{
		RateLimitedSound_t* pSound = m_mapRateLimitedSounds[ nIndex ];
		
		int nPlayerIndex = pSound->m_mapNextAllowedTime.Find( pPlayer );
		if ( nPlayerIndex == pSound->m_mapNextAllowedTime.InvalidIndex() )
		{
			nPlayerIndex = pSound->m_mapNextAllowedTime.Insert( pPlayer );
			pSound->m_mapNextAllowedTime[ nPlayerIndex ] = 0.f;
		}

		float& flNextAllowedTime = pSound->m_mapNextAllowedTime[ nPlayerIndex ];

		// If we're not allowed to play, then return
		if ( flNextAllowedTime > gpGlobals->curtime )
		{
			return;
		}

		// Mark the next time we're allowed to play
		flNextAllowedTime = gpGlobals->curtime + m_mapRateLimitedSounds[ nIndex ]->m_flPause;
	}

	// Play in the player's ears
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	if ( params.m_nFlags & SND_CHANGE_PITCH )
	{
		pPlayer->StopSound( params.m_pSoundName );
	}
	pPlayer->EmitSound( filter, pPlayer->entindex(), params );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::RedTeamWin()
{
	TeamWin( TF_TEAM_RED );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::BlueTeamWin()
{
	TeamWin( TF_TEAM_BLUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::TeamWin( int nTeam )
{
	RDScoreMethod_t eMethod = (RDScoreMethod_t)m_eWinningMethod.Get( nTeam );
		
	if ( TFGameRules() )
	{	
		TFGameRules()->SetWinningTeam( nTeam, ( eMethod == SCORE_REACTOR_CAPTURED ) ? WINREASON_RD_REACTOR_CAPTURED : ( ( eMethod == SCORE_CORES_COLLECTED ) ? WINREASON_RD_CORES_COLLECTED : WINREASON_RD_REACTOR_RETURNED ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::FlagCreated( int nTeam )
{
	if ( nTeam == TF_TEAM_RED )
	{
		m_OnRedFlagStolen.FireOutput( this, this );
		if ( m_nNumFlagsOut[ nTeam ] == 0 )
		{
			m_OnRedFirstFlagStolen.FireOutput( this, this );
		}
	}
	else
	{
		m_OnBlueFlagStolen.FireOutput( this, this );
		if ( m_nNumFlagsOut[ nTeam ] == 0 )
		{
			m_OnBlueFirstFlagStolen.FireOutput( this, this );
		}
	}
	
	++m_nNumFlagsOut[ nTeam ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::FlagDestroyed( int nTeam )
{
	if ( m_nNumFlagsOut[ nTeam ] == 1 )
	{
		if ( nTeam == TF_TEAM_RED )
		{
			m_OnRedLastFlagReturned.FireOutput( this, this );
		}
		else
		{
			m_OnBlueLastFlagReturned.FireOutput( this, this );
		}
	}

	--m_nNumFlagsOut[ nTeam ];
}

//-----------------------------------------------------------------------------
// Purpose: Add a given robot to our list of robots.  Increment our count of
//			robots for the team that the robot is on
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::RobotCreated( CTFRobotDestruction_Robot *pRobot )
{
	m_vecRobots.AddToTail( pRobot );
}

//-----------------------------------------------------------------------------
// Purpose: Remove a robot from our list.  Decrement our count of robots for
//			the team that the robot was on
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::RobotRemoved( CTFRobotDestruction_Robot *pRobot )
{
	m_vecRobots.FindAndRemove( pRobot );
}


//-----------------------------------------------------------------------------
// Purpose: Perform alerts when a robot is attacked
//-----------------------------------------------------------------------------
void CTFRobotDestructionLogic::RobotAttacked( CTFRobotDestruction_Robot *pRobot )
{
	float& flNextAlertTime = ( pRobot->GetTeamNumber() == TF_TEAM_RED ) ? m_flNextRedRobotAttackedAlertTime
																		: m_flNextBlueRobotAttackedAlertTime;

	if ( gpGlobals->curtime >= flNextAlertTime )
	{
		flNextAlertTime = gpGlobals->curtime + tf_rd_robot_attack_notification_cooldown.GetFloat();

		CTeamRecipientFilter filter( pRobot->GetTeamNumber(), true );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_RD_ROBOT_UNDER_ATTACK );
	}
}


BEGIN_DATADESC( CTFRobotDestructionLogic )

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),

	DEFINE_OUTPUT( m_OnRedHitZeroPoints,	"OnRedHitZeroPoints" ),
	DEFINE_OUTPUT( m_OnRedHasPoints,		"OnRedHasPoints" ),
	DEFINE_OUTPUT( m_OnRedFinalePeriodEnd,	"OnRedFinalePeriodEnd" ),

	DEFINE_OUTPUT( m_OnBlueHitZeroPoints,	"OnBlueHitZeroPoints" ),
	DEFINE_OUTPUT( m_OnBlueHasPoints,		"OnBlueHasPoints" ),
	DEFINE_OUTPUT( m_OnBlueFinalePeriodEnd, "OnBlueFinalePeriodEnd" ),

	DEFINE_OUTPUT( m_OnRedFirstFlagStolen,	"OnRedFirstFlagStolen" ),
	DEFINE_OUTPUT( m_OnRedFlagStolen,		"OnRedFlagStolen" ),
	DEFINE_OUTPUT( m_OnRedLastFlagReturned, "OnRedLastFlagReturned" ),
	DEFINE_OUTPUT( m_OnBlueFirstFlagStolen, "OnBlueFirstFlagStolen" ),
	DEFINE_OUTPUT( m_OnBlueFlagStolen,		"OnBlueFlagStolen" ),
	DEFINE_OUTPUT( m_OnBlueLastFlagReturned, "OnBlueLastFlagReturned" ),
	DEFINE_OUTPUT( m_OnBlueLeaveMaxPoints, "OnBlueLeaveMaxPoints" ),
	DEFINE_OUTPUT( m_OnRedLeaveMaxPoints, "OnRedLeaveMaxPoints" ),
	DEFINE_OUTPUT( m_OnBlueHitMaxPoints, "OnBlueHitMaxPoints" ),
	DEFINE_OUTPUT( m_OnRedHitMaxPoints, "OnRedHitMaxPoints" ),

	DEFINE_KEYFIELD( m_flRobotScoreInterval, FIELD_FLOAT, "score_interval" ),
	DEFINE_KEYFIELD( m_flLoserRespawnBonusPerBot, FIELD_FLOAT, "loser_respawn_bonus_per_bot" ),
	DEFINE_KEYFIELD( m_nMaxPoints, FIELD_INTEGER, "max_points" ),
	DEFINE_KEYFIELD( m_flFinaleLength, FIELD_FLOAT, "finale_length" ),
	DEFINE_KEYFIELD( m_iszResFile, FIELD_STRING, "res_file" ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_logic_robot_destruction, CTFRobotDestructionLogic );
IMPLEMENT_NETWORKCLASS_ALIASED( TFRobotDestructionLogic, DT_TFRobotDestructionLogic )

BEGIN_NETWORK_TABLE_NOBASE( CTFRobotDestructionLogic, DT_TFRobotDestructionLogic )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nMaxPoints ) ),
	RecvPropInt( RECVINFO( m_nBlueScore ) ),
	RecvPropInt( RECVINFO( m_nRedScore ) ),
	RecvPropInt( RECVINFO( m_nBlueTargetPoints ) ),
	RecvPropInt( RECVINFO( m_nRedTargetPoints ) ),
	RecvPropFloat( RECVINFO( m_flBlueTeamRespawnScale ) ),
	RecvPropFloat( RECVINFO( m_flRedTeamRespawnScale ) ),
	RecvPropFloat( RECVINFO( m_flBlueFinaleEndTime ) ),
	RecvPropFloat( RECVINFO( m_flRedFinaleEndTime ) ),
	RecvPropFloat( RECVINFO( m_flFinaleLength ) ),
	RecvPropString( RECVINFO( m_szResFile ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_eWinningMethod ), RecvPropInt( RECVINFO( m_eWinningMethod[0] ) ) ),
	RecvPropFloat( RECVINFO( m_flCountdownEndTime ) ),
#else
	SendPropInt( SENDINFO( m_nMaxPoints ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nBlueScore ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nRedScore ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nBlueTargetPoints ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nRedTargetPoints ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flBlueTeamRespawnScale ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flRedTeamRespawnScale ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flBlueFinaleEndTime ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flRedFinaleEndTime ), -1, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flFinaleLength ), -1, SPROP_NOSCALE ),
	SendPropString( SENDINFO( m_szResFile ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_eWinningMethod ), SendPropInt( SENDINFO_ARRAY( m_eWinningMethod ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropFloat( SENDINFO( m_flCountdownEndTime ), -1, SPROP_NOSCALE ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( trigger_rd_vault_trigger, CRobotDestructionVaultTrigger );

BEGIN_DATADESC( CRobotDestructionVaultTrigger )
	DEFINE_OUTPUT( m_OnPointsStolen,	"OnPointsStolen" ),
	DEFINE_OUTPUT( m_OnPointsStartStealing,	"OnPointsStartStealing" ),
	DEFINE_OUTPUT( m_OnPointsEndStealing,	"OnPointsEndStealing" ),
END_DATADESC()

CRobotDestructionVaultTrigger::CRobotDestructionVaultTrigger()
	: m_bIsStealing( false )
{}

void CRobotDestructionVaultTrigger::Spawn()
{
	BaseClass::Spawn();

	InitTrigger();
}

void CRobotDestructionVaultTrigger::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "Cart.WarningSingle" );
}

bool CRobotDestructionVaultTrigger::PassesTriggerFilters( CBaseEntity *pOther )
{
	if ( pOther->GetTeamNumber() == GetTeamNumber() )
		return false;

	// Only allow these entities
	if ( !pOther->ClassMatches( "player" ) )
		return false;

	return true;
}

void CRobotDestructionVaultTrigger::StartTouch(CBaseEntity *pOther)
{
	if ( !PassesTriggerFilters( pOther ) )
		return;

	BaseClass::StartTouch( pOther );

	// This is the first guy to touch us.   Start thinking
	if ( m_hTouchingEntities.Count() == 1 )
	{
		SetContextThink( &CRobotDestructionVaultTrigger::StealPointsThink, gpGlobals->curtime, ADD_POINTS_CONTEXT );
	}
}

void CRobotDestructionVaultTrigger::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch( pOther );

	// Last guy stopped touching us.  Stop thinking
	if ( m_hTouchingEntities.Count() == 0 )
	{
		SetContextThink( NULL, 0, ADD_POINTS_CONTEXT );
	}

	// Force the stealing player to drop the flag if they didnt steal enough points
	CTFPlayer *pPlayer = dynamic_cast< CTFPlayer * >( pOther );
	if ( pPlayer )
	{
		CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * >( pPlayer->GetItem() );
		if ( pFlag )
		{
			if ( pFlag->GetPointValue() < tf_rd_min_points_to_steal.GetInt() )
			{
				pFlag->Drop( pPlayer, true, true );
				pFlag->ResetFlag();

				// TODO: Play negative sound in player's ears
			}
			
			if ( m_bIsStealing )
			{
				// If the flag carrier is leaving us, we're done stealing
				m_bIsStealing = false;
				m_OnPointsEndStealing.FireOutput( this, this );
			}
		}
	}
}

void CRobotDestructionVaultTrigger::StealPointsThink()
{
	// Do it again!
	SetContextThink( &CRobotDestructionVaultTrigger::StealPointsThink, gpGlobals->curtime + tf_rd_steal_rate.GetFloat(), ADD_POINTS_CONTEXT );

	int nNumStolen = 0;

 	FOR_EACH_VEC( m_hTouchingEntities, i )
	{
		CTFPlayer *pPlayer = static_cast< CTFPlayer * >( m_hTouchingEntities[i].Get() );
		if ( pPlayer )
		{
			CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * >( pPlayer->GetItem() );
			if ( pFlag )
			{
				nNumStolen = StealPoints( pPlayer );
			}
		}
	}

	// Check to fire the stealing outputs
	if ( nNumStolen > 0 )
	{
		m_OnPointsStolen.FireOutput( this, this );
	}
	if ( nNumStolen && !m_bIsStealing )
	{
		m_OnPointsStartStealing.FireOutput( this, this );
	}
	else if ( !nNumStolen && m_bIsStealing )
	{
		m_OnPointsEndStealing.FireOutput( this, this );
	}

	m_bIsStealing = nNumStolen != 0;
}

int CRobotDestructionVaultTrigger::StealPoints( CTFPlayer *pPlayer )
{
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pPlayer->GetItem() );
	if ( pFlag && CTFRobotDestructionLogic::GetRobotDestructionLogic() )
	{
		int nEnemyTeamNumber = GetEnemyTeam( pPlayer->GetTeamNumber() );
		int nEnemyPoints = CTFRobotDestructionLogic::GetRobotDestructionLogic()->GetTargetScore( nEnemyTeamNumber );
		if ( nEnemyPoints )
		{
			int nPointsToSteal = Min( nEnemyPoints, tf_rd_points_per_steal.GetInt() );
			pFlag->AddPointValue( nPointsToSteal );
			CTFRobotDestructionLogic::GetRobotDestructionLogic()->ScorePoints( nEnemyTeamNumber
																			 , -nPointsToSteal
																			 , SCORE_REACTOR_STEAL
																			 , pPlayer );

			SetContextThink( &CRobotDestructionVaultTrigger::StealPointsThink, gpGlobals->curtime + tf_rd_steal_rate.GetFloat(), ADD_POINTS_CONTEXT );

			return nPointsToSteal;
		}
	}

	return 0;
}
#endif
