//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//

#include "cbase.h"
#include "tf_logic_halloween_2014.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
	#include "particle_parse.h"
	#include "halloween/tf_weapon_spellbook.h"
	#include "tf_weapon_sniperrifle.h"
	#include "ai_activity.h"
	#include "halloween/halloween_base_boss.h"
	#include "halloween/tf_weapon_spellbook.h"
	#include "engine/IEngineSound.h"
	#include "tf_props.h"
#endif

IMPLEMENT_AUTO_LIST( IMinigameAutoList );

#ifdef GAME_DLL
extern ConVar tf_teleporter_fov_time;
extern ConVar tf_teleporter_fov_start;

ConVar tf_fortune_teller_warning_time( "tf_fortune_teller_warning_time", "2", FCVAR_CHEAT, "Warning time (in second) before fortune teller tells a fortune." );
ConVar tf_fortune_teller_interval_time( "tf_fortune_teller_interval_time", "120", FCVAR_CHEAT, "Time until the next fortune teller event (in second)." );
ConVar tf_fortune_teller_fortune_duration( "tf_fortune_teller_fortune_duration", "30", FCVAR_CHEAT, "Duration of the fortune time." );

ConVar tf_minigame_suddendeath_time( "tf_minigame_suddendeath_time", "-1", FCVAR_CHEAT, "Override Sudden Death Time." );

BEGIN_DATADESC( CTFMiniGame )

	DEFINE_KEYFIELD( m_iszYourTeamScoreSound, FIELD_STRING, "your_team_score_sound" ),
	DEFINE_KEYFIELD( m_iszEnemyTeamScoreSound, FIELD_STRING, "enemy_team_score_sound" ),
	DEFINE_KEYFIELD( m_iszHudResFile, FIELD_STRING, "hud_res_file" ),
	DEFINE_KEYFIELD( m_pszTeamSpawnPoint[ TF_TEAM_RED ], FIELD_STRING, "RedSpawn" ),
	DEFINE_KEYFIELD( m_pszTeamSpawnPoint[ TF_TEAM_BLUE ], FIELD_STRING, "BlueSpawn" ),
	DEFINE_KEYFIELD( m_bMinigameAllowedInRamdomPool, FIELD_BOOLEAN, "InRandomPool" ),
	DEFINE_KEYFIELD( m_nMaxScoreForMiniGame, FIELD_INTEGER, "MaxScore" ),
	DEFINE_KEYFIELD( m_eScoringType, FIELD_INTEGER, "ScoreType" ),
	DEFINE_KEYFIELD( m_flSuddenDeathTime, FIELD_FLOAT, "SuddenDeathTime" ),

	DEFINE_OUTPUT( m_OnRedHitMaxScore, "OnRedHitMaxScore" ),
	DEFINE_OUTPUT( m_OnBlueHitMaxScore, "OnBlueHitMaxScore" ),
	DEFINE_OUTPUT( m_OnTeleportToMinigame, "OnTeleportToMinigame" ),
	DEFINE_OUTPUT( m_OnReturnFromMinigame, "OnReturnFromMinigame" ),
	DEFINE_OUTPUT( m_OnAllRedDead, "OnAllRedDead" ),
	DEFINE_OUTPUT( m_OnAllBlueDead, "OnAllBlueDead" ),
	DEFINE_OUTPUT( m_OnSuddenDeathStart, "OnSuddenDeathStart" ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "ScoreTeamRed", InputScoreTeamRed ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ScoreTeamBlue", InputScoreTeamBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeHudResFile", InputChangeHudResFile ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_base_minigame, CTFMiniGame );
IMPLEMENT_NETWORKCLASS_ALIASED( TFMiniGame, DT_TFMinigame )

BEGIN_NETWORK_TABLE_NOBASE( CTFMiniGame, DT_TFMinigame )
#ifdef CLIENT_DLL
	RecvPropArray3( RECVINFO_ARRAY( m_nMinigameTeamScore ), RecvPropInt( RECVINFO( m_nMinigameTeamScore[0] ) ) ),
	RecvPropInt( RECVINFO( m_nMaxScoreForMiniGame ) ),
	RecvPropString( RECVINFO( m_pszHudResFile ) ),
	RecvPropInt( RECVINFO( m_eScoringType ) ),
#else
	SendPropArray3( SENDINFO_ARRAY3( m_nMinigameTeamScore ), SendPropInt( SENDINFO_ARRAY( m_nMinigameTeamScore ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropInt( SENDINFO( m_nMaxScoreForMiniGame ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_pszHudResFile ) ),
	SendPropInt( SENDINFO( m_eScoringType ), -1, SPROP_VARINT | SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#define NO_TEAM_ADVANTAGE -1


CTFMiniGame::CTFMiniGame()
{
	m_nMaxScoreForMiniGame = 0;
	m_nMinigameTeamScore.Set( TF_TEAM_RED, 0 );
	m_nMinigameTeamScore.Set( TF_TEAM_BLUE, 0 );
#ifdef GAME_DLL
	m_bMinigameAllowedInRamdomPool = true;
	m_bIsActive = false;
	m_pszTeamSpawnPoint[ TF_TEAM_RED ] = NULL;
	m_pszTeamSpawnPoint[ TF_TEAM_BLUE ] = NULL;
	m_eMinigameType = MINIGAME_GENERIC;
	m_iszYourTeamScoreSound = NULL_STRING;
	m_iszEnemyTeamScoreSound = NULL_STRING;
	m_flSuddenDeathTime = -1.0f; // No sudden death.
	m_iAdvantagedTeam = NO_TEAM_ADVANTAGE;

	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_turned_to_ghost" );
	ListenForGameEvent( "player_disconnect" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_spawn" );
#endif
}

#ifdef GAME_DLL
void CTFMiniGame::Spawn()
{
	Precache();

	BaseClass::Spawn();
	V_strncpy( m_pszHudResFile.GetForModify(), STRING( m_iszHudResFile ), MAX_PATH );
}

void CTFMiniGame::Precache()
{
	BaseClass::Precache();

	if ( m_iszYourTeamScoreSound.ToCStr() )
	{
		PrecacheScriptSound( m_iszYourTeamScoreSound.ToCStr() );
	}
	if ( m_iszEnemyTeamScoreSound.ToCStr() )
	{
		PrecacheScriptSound( m_iszEnemyTeamScoreSound.ToCStr() );
	}
}

void CTFMiniGame::FireGameEvent( IGameEvent * pEvent )
{
	// Only look for dead players when the round is running or else we'll get
	// victories while in the spawn room as we respawn
	if ( ( TFGameRules() && TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) || !m_bIsActive ) 
		return;

	const char *pszEventName = pEvent->GetName();
	
	if ( !V_strcmp( pszEventName, "player_turned_to_ghost" ) 
		|| !V_strcmp( pszEventName, "player_disconnect" )
		|| !V_strcmp( pszEventName, "player_team" ) 
		|| !V_strcmp( pszEventName, "player_death" )
		|| !V_strcmp( pszEventName, "player_spawn" ) )
	{
		bool bCanWin = true;
		// Blue gets the chance to win first
		UpdateDeadPlayers( TF_TEAM_RED, m_OnBlueHitMaxScore, m_OnAllRedDead, bCanWin );
		UpdateDeadPlayers( TF_TEAM_BLUE, m_OnRedHitMaxScore, m_OnAllBlueDead, bCanWin );
	}
}

void CTFMiniGame::TeleportAllPlayers()
{
	// remove all projectiles and objects before we go to minigame
	TFGameRules()->RemoveAllProjectilesAndBuildings();

	CUtlVector< CTFPlayer* > vecTeleportedPlayers;
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_RED, m_pszTeamSpawnPoint[ TF_TEAM_RED ], &vecTeleportedPlayers );
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_BLUE, m_pszTeamSpawnPoint[ TF_TEAM_BLUE ], &vecTeleportedPlayers );

	FOR_EACH_VEC( vecTeleportedPlayers, i )
	{
		OnTeleportPlayerToMinigame( vecTeleportedPlayers[i] );
	}
	m_iAdvantagedTeam = NO_TEAM_ADVANTAGE; // reset advantage
	m_bIsActive = true;

	m_OnTeleportToMinigame.FireOutput( this, this );

	if ( tf_minigame_suddendeath_time.GetFloat() != -1 )
	{
		m_flSuddenDeathTime = tf_minigame_suddendeath_time.GetFloat();
		DevMsg( "Setting m_flSuddenDeathTime to %f\n", m_flSuddenDeathTime );
	}

	// If we've got a sudden death start time, trigger a think function callback.
	if ( m_flSuddenDeathTime >= 0.0f )
	{
		SetContextThink( &CTFHalloweenMinigame::SuddenDeathTimeStartThink, gpGlobals->curtime + m_flSuddenDeathTime, "SuddenDeathTimeStart" );
	}
}

void CTFMiniGame::OnTeleportPlayerToMinigame( CTFPlayer *pPlayer )
{
	// Do a zoom effect
	pPlayer->SetFOV( pPlayer, tf_teleporter_fov_start.GetInt() );
	pPlayer->SetFOV( pPlayer, 0, 1.f, tf_teleporter_fov_start.GetInt() );

	// Screen flash
	color32 fadeColor = {255,255,255,100};
	UTIL_ScreenFade( pPlayer, fadeColor, 0.25, 0.4, FFADE_IN );
}

void CTFMiniGame::ReturnAllPlayers()
{
	// Send everyone back
	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, TEAM_ANY, false );
	FOR_EACH_VEC( vecPlayers, i )
	{
		vecPlayers[ i ]->ForceRespawn();
	}

	m_nMinigameTeamScore.Set( TF_TEAM_RED, 0 );
	m_nMinigameTeamScore.Set( TF_TEAM_BLUE, 0 );

	m_bIsActive = false;

	m_OnReturnFromMinigame.FireOutput( this, this );
}

void CTFMiniGame::ScorePointsForTeam( int nTeamNum, int nPoints )
{
	// Don't tally more points if a team already hit the max
	if ( !m_bIsActive || ( m_nMinigameTeamScore.Get( TF_TEAM_RED ) == m_nMaxScoreForMiniGame ) || ( m_nMinigameTeamScore.Get( TF_TEAM_BLUE ) == m_nMaxScoreForMiniGame ) )
	{
		return;
	}

	// Are we playing sudden death right now?
	bool bInSuddenDeath = ( m_flSuddenDeathTime == 0.0f );

	// Increment score for the appropriate team
	auto& nTeamPoints = m_nMinigameTeamScore.GetForModify( nTeamNum );
	nTeamPoints += nPoints;
	nTeamPoints = clamp( nTeamPoints, 0, m_nMaxScoreForMiniGame );

	// If they went to the max score or we're in sudden death, fire winning output.
	if ( ( nTeamPoints == m_nMaxScoreForMiniGame ) || bInSuddenDeath )
	{
		auto& eventMaxScoreHit = ( nTeamNum == TF_TEAM_RED ) ? m_OnRedHitMaxScore : m_OnBlueHitMaxScore;
		eventMaxScoreHit.FireOutput( this, this );

		CUtlVector<CTFPlayer *> vecPlayers;
		CollectPlayers( &vecPlayers, nTeamNum );

		for ( auto pPlayer : vecPlayers )
		{
			HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( pPlayer, kKillEaterEvent_Halloween_MinigamesWon );

			IGameEvent *pEvent = gameeventmanager->CreateEvent( "minigame_won" );
			if ( pEvent )
			{
				pEvent->SetInt( "player", pPlayer->GetUserID() );
				pEvent->SetInt( "game", GetMinigameType() );
				gameeventmanager->FireEvent( pEvent, true );
			}
		}
	}

	// Can not be specified, and we dont want to do anything in that case
	if ( m_iszYourTeamScoreSound.ToCStr() && *m_iszYourTeamScoreSound.ToCStr() 
		 && m_iszEnemyTeamScoreSound.ToCStr() && *m_iszEnemyTeamScoreSound.ToCStr() )
	{
		// Get everyone
		CUtlVector< CTFPlayer* > vecPlayer;
		CollectPlayers( &vecPlayer );
	
		// Play a sound based on if the scoring team is the player's team
		for( CTFPlayer *pPlayer : vecPlayer )
		{
			EmitSound_t params;
			float soundlen = 0;
			params.m_flSoundTime = 0;
			params.m_pSoundName = NULL;
			params.m_pflSoundDuration = &soundlen;
			params.m_pSoundName = pPlayer->GetTeamNumber() == nTeamNum ? m_iszYourTeamScoreSound.ToCStr() : m_iszEnemyTeamScoreSound.ToCStr();
			params.m_nPitch = RemapValClamped( nTeamPoints, m_nMaxScoreForMiniGame * 0.75, m_nMaxScoreForMiniGame, 100, 120 );
			params.m_nFlags |= SND_CHANGE_PITCH;
			params.m_flVolume = 0.25f; // Pretty quiet
			params.m_nFlags |= SND_CHANGE_VOL;

			// Play in the player's ears
			CSingleUserRecipientFilter filter( pPlayer );
			filter.MakeReliable();
			pPlayer->StopSound( params.m_pSoundName );
			pPlayer->EmitSound( filter, pPlayer->entindex(), params );
		}
	}
}

void CTFMiniGame::InputScoreTeamRed( inputdata_t &inputdata )
{
	ScorePointsForTeam( TF_TEAM_RED, inputdata.value.Int() );
	InternalHandleInputScore( inputdata );
}

void CTFMiniGame::InputScoreTeamBlue( inputdata_t &inputdata )
{
	ScorePointsForTeam( TF_TEAM_BLUE, inputdata.value.Int() );
	InternalHandleInputScore( inputdata );
}

void CTFMiniGame::InputChangeHudResFile( inputdata_t &inputdata )
{
	const char *resFile = inputdata.value.String();

	Assert( resFile && resFile[ 0 ] );
	if ( resFile && resFile[ 0 ] )
	{
		V_strncpy( m_pszHudResFile.GetForModify(), resFile, MAX_PATH );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: Find spawn point entity for specified team
//-----------------------------------------------------------------------------
const char *CTFMiniGame::GetTeamSpawnPointName( int nTeamNum ) const
{
	if ( !IsValidTFTeam( nTeamNum ) )
		return NULL;

	return m_pszTeamSpawnPoint[ nTeamNum ];
}

void CTFMiniGame::UpdateDeadPlayers( int nTeam, COutputEvent& eventWin, COutputEvent& eventAllDead, bool& bCanWin )
{
	// Update the score for a team
	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, nTeam, true );
	int nNumDead = 0;
	FOR_EACH_VEC( vecPlayers, i )
	{
		// Tally the number dead/ghosts
		if ( vecPlayers[i]->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) || vecPlayers[i]->IsDead() )
			++nNumDead;
	}

	// Only update the score if this is the SCORING_TYPE_PLAYERS_ALIVE mode
	if ( m_eScoringType == SCORING_TYPE_PLAYERS_ALIVE )
	{
		auto& nScore = m_nMinigameTeamScore.GetForModify( nTeam );
		nScore = vecPlayers.Count() - nNumDead;
	}

	// Everyone is dead
	if ( nNumDead == vecPlayers.Count() && !vecPlayers.IsEmpty() )
	{
		m_bIsActive = false;

		// If everyone is dead, and we're allowed to win, fire the win event
		if ( bCanWin && m_eScoringType == SCORING_TYPE_PLAYERS_ALIVE )
		{
			eventWin.FireOutput( this, this );
			bCanWin = false;
		}
			
		// Fire the team dead event
		eventAllDead.FireOutput( this, this );

		CUtlVector<CTFPlayer *> vecEnemyPlayers;
		CollectPlayers( &vecEnemyPlayers, GetEnemyTeam( nTeam ) );

		for ( auto pPlayer : vecEnemyPlayers )
		{
			HatAndMiscEconEntities_OnOwnerKillEaterEventNoParter( pPlayer, kKillEaterEvent_Halloween_MinigamesWon );

			IGameEvent *pEvent = gameeventmanager->CreateEvent( "minigame_won" );
			if ( pEvent )
			{
				pEvent->SetInt( "player", pPlayer->GetUserID() );
				pEvent->SetInt( "game", GetMinigameType() );
				gameeventmanager->FireEvent( pEvent, true );
			}
		}
	}
}

// "SuddenDeathTimeStart"
void CTFMiniGame::SuddenDeathTimeStartThink()
{
	// If we're active, fire the sudden death time start event.
	if ( m_bIsActive )
	{
		m_flSuddenDeathTime = 0.0f; // In sudden death!
		m_OnSuddenDeathStart.FireOutput( this, this );
	}
}

#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFMiniGame::GetScoreForTeam( int nTeamNum ) const
{
	if ( !IsValidTFTeam( nTeamNum ) )
		return 0;

	return m_nMinigameTeamScore[ nTeamNum ];
}
#endif // GAME_DLL


#ifdef GAME_DLL
BEGIN_DATADESC( CTFHalloweenMinigame )

	DEFINE_KEYFIELD( m_eMinigameType, FIELD_INTEGER, "MinigameType" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "KartWinAnimationRed", InputKartWinAnimationRed ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KartWinAnimationBlue", InputKartWinAnimationBlue ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KartLoseAnimationRed", InputKartLoseAnimationRed ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KartLoseAnimationBlue", InputKartLoseAnimationBlue ),
	DEFINE_INPUTFUNC( FIELD_STRING, "EnableSpawnBoss", InputEnableSpawnBoss ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableSpawnBoss", InputDisableSpawnBoss ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_halloween_minigame, CTFHalloweenMinigame );
IMPLEMENT_NETWORKCLASS_ALIASED( TFHalloweenMinigame, DT_TFHalloweenMinigame )

BEGIN_NETWORK_TABLE( CTFHalloweenMinigame, DT_TFHalloweenMinigame )
END_NETWORK_TABLE()

#ifdef GAME_DLL
CTFHalloweenMinigame::CTFHalloweenMinigame()
{
	m_hBossSpawnPoint = NULL;
	ListenForGameEvent( "pumpkin_lord_killed" );
}


void CTFHalloweenMinigame::Spawn()
{
	BaseClass::Spawn();
}


void CTFHalloweenMinigame::FireGameEvent( IGameEvent * event )
{
	BaseClass::FireGameEvent( event );

	if ( FStrEq( event->GetName(), "pumpkin_lord_killed" ) )
	{
		if ( m_hBossSpawnPoint && ( !m_hHalloweenBoss || m_hHalloweenBoss->IsMarkedForDeletion() ) )
		{
			m_hHalloweenBoss = CHalloweenBaseBoss::SpawnBossAtPos( HALLOWEEN_BOSS_HHH, m_hBossSpawnPoint->GetAbsOrigin() );
		}
	}
}

void CTFHalloweenMinigame::InternalHandleInputScore( inputdata_t &inputdata )
{
	CPropSoccerBall *pSoccerBall = dynamic_cast< CPropSoccerBall* >( inputdata.pActivator );
	if ( pSoccerBall )
	{
		CTFPlayer *pTFPlayer = pSoccerBall->GetLastToucher();
		if ( pTFPlayer && TFGameRules() && TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_HALLOWEEN_DOOMSDAY_SCORE_GOALS );
		}
	}
}

void CTFHalloweenMinigame::TeleportAllPlayers()
{
	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, TEAM_ANY, false );

	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];
		// Only do these effects if the player is alive
		if ( !pPlayer->IsAlive() )
			continue;

		// Fade to white
		color32 fadeColor = {255,255,255,255};
		UTIL_ScreenFade( pPlayer, fadeColor, 2.f, 0.5f, FFADE_OUT | FFADE_PURGE );

		// Do a zoom in effect
		pPlayer->SetFOV( pPlayer, 10.f, 2.5f, 0.f );
		// Rumble like something important happened
		UTIL_ScreenShake(pPlayer->GetAbsOrigin(), 100.f, 150, 4.f, 0.f, SHAKE_START, true );
	}

	// Play a sound for all players
	TFGameRules()->BroadcastSound( 255, "Halloween.hellride" );

	SetContextThink( &CTFHalloweenMinigame::TeleportAllPlayersThink, gpGlobals->curtime + 2.0f, "TeleportToHell" );
}

void CTFHalloweenMinigame::TeleportAllPlayersThink()
{
	RemoveAll2013HalloweenTeleportSpellsInMidFlight();

	CUtlVector< CTFPlayer * > vecPlayers;
	CollectPlayers( &vecPlayers, TEAM_ANY, false );

	BaseClass::TeleportAllPlayers();

	FOR_EACH_VEC( vecPlayers, i )
	{
		CTFPlayer *pPlayer = vecPlayers[i];

		pPlayer->CancelEurekaTeleport();

		// Fade from white
		color32 fadeColor = {255,255,255,255};
		UTIL_ScreenFade( pPlayer, fadeColor, 1.f, 0.2f, FFADE_IN );
	}

	// Set this flag.  Lets us check elsewhere if it's hell time
	// HACK:  Should we be doing this for 2014?!?
	if ( TFGameRules() )
	{
		TFGameRules()->SetPlayersInHell( true );
	}
}

void CTFHalloweenMinigame::OnTeleportPlayerToMinigame( CTFPlayer *pPlayer )
{
	BaseClass::OnTeleportPlayerToMinigame( pPlayer );

	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_IN_HELL );
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_KART );

	const float flCageTime = 3.f;
	pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_KART_CAGE, flCageTime );
	pPlayer->m_Shared.AddCond( TF_COND_FREEZE_INPUT, flCageTime );

	pPlayer->SetAbsVelocity( vec3_origin );

	pPlayer->EmitSound( "BumperCar.Spawn" );

	// if its set
	if ( m_iAdvantagedTeam != NO_TEAM_ADVANTAGE )
	{
		if ( pPlayer->GetTeamNumber() != m_iAdvantagedTeam )
		{
			pPlayer->AddKartDamage( 66 );
		}
	}
}

void CTFHalloweenMinigame::ReturnAllPlayers()
{
	// Set this flag.  Lets us check elsewhere if it's hell time
	// HACK:  Should we be doing this for 2014?!?
	if ( TFGameRules() )
	{
		TFGameRules()->SetPlayersInHell( false );
	}

	BaseClass::ReturnAllPlayers();
}

void CTFHalloweenMinigame::InputKartWinAnimationRed( inputdata_t &inputdata )
{
	CUtlVector< CTFPlayer* > players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	for ( int i=0; i<players.Count(); ++i )
	{
		if ( players[i]->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			players[i]->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_GESTURE_POSITIVE );
		}
	}
}

void CTFHalloweenMinigame::InputKartWinAnimationBlue( inputdata_t &inputdata )
{
	CUtlVector< CTFPlayer* > players;
	CollectPlayers( &players, TF_TEAM_BLUE, true );
	for ( int i=0; i<players.Count(); ++i )
	{
		if ( players[i]->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			players[i]->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_GESTURE_POSITIVE );
		}
	}
}

void CTFHalloweenMinigame::InputKartLoseAnimationRed( inputdata_t &inputdata )
{
	CUtlVector< CTFPlayer* > players;
	CollectPlayers( &players, TF_TEAM_RED, true );
	for ( int i=0; i<players.Count(); ++i )
	{
		if ( players[i]->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			players[i]->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_GESTURE_NEGATIVE );
		}
	}
}

void CTFHalloweenMinigame::InputKartLoseAnimationBlue( inputdata_t &inputdata )
{
	CUtlVector< CTFPlayer* > players;
	CollectPlayers( &players, TF_TEAM_BLUE, true );
	for ( int i=0; i<players.Count(); ++i )
	{
		if ( players[i]->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
		{
			players[i]->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_KART_GESTURE_NEGATIVE );
		}
	}
}

void CTFHalloweenMinigame::InputEnableSpawnBoss( inputdata_t &inputdata )
{
	if ( !m_hBossSpawnPoint )
	{
		const char *pszSpawnPoint = inputdata.value.String();
		if ( pszSpawnPoint )
		{
			m_hBossSpawnPoint = gEntList.FindEntityByName( NULL, pszSpawnPoint );
		}
	}

	if ( m_hBossSpawnPoint )
	{
		m_hHalloweenBoss = CHalloweenBaseBoss::SpawnBossAtPos( HALLOWEEN_BOSS_HHH, m_hBossSpawnPoint->GetAbsOrigin() );
	}
}

void CTFHalloweenMinigame::InputDisableSpawnBoss( inputdata_t &inputdata )
{
	m_hBossSpawnPoint = NULL;
}

#endif

#ifdef GAME_DLL
BEGIN_DATADESC( CTFHalloweenMinigame_FallingPlatforms )

	DEFINE_INPUTFUNC( FIELD_VOID, "ChoosePlatform", InputChoosePlatform ),

	DEFINE_OUTPUT( m_OutputSafePlatform, "OutputSafePlatform" ),
	DEFINE_OUTPUT( m_OutputRemovePlatform, "OutputRemovePlatform" ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_halloween_minigame_falling_platforms, CTFHalloweenMinigame_FallingPlatforms );
IMPLEMENT_NETWORKCLASS_ALIASED( TFHalloweenMinigame_FallingPlatforms, DT_TFHalloweenMinigame_FallingPlatforms )

BEGIN_NETWORK_TABLE( CTFHalloweenMinigame_FallingPlatforms, DT_TFHalloweenMinigame_FallingPlatforms )
END_NETWORK_TABLE()

#ifdef GAME_DLL
CTFHalloweenMinigame_FallingPlatforms::CTFHalloweenMinigame_FallingPlatforms()
{
	// Corners
	CUtlVector<int> vecFirstSet;
	vecFirstSet.AddToTail( 1 );
	vecFirstSet.AddToTail( 3 );
	vecFirstSet.AddToTail( 7 );
	vecFirstSet.AddToTail( 9 );
	vecFirstSet.Shuffle();

	// On Axis
	CUtlVector<int> vecSecondSet;
	vecSecondSet.AddToTail( 2 );
	vecSecondSet.AddToTail( 4 );
	vecSecondSet.AddToTail( 6 );
	vecSecondSet.AddToTail( 8 );
	vecSecondSet.Shuffle();

	m_vecRemainingPlatforms.AddVectorToTail( vecFirstSet );
	m_vecRemainingPlatforms.AddVectorToTail( vecSecondSet );

	m_vecRemainingPlatforms.AddToTail( 5 ); // The center
}

void CTFHalloweenMinigame_FallingPlatforms::InputChoosePlatform( inputdata_t &inputdata )
{
	variant_t nVal;
	// If there's more than 1 platforms remaining, mark another to never come back
	if ( m_vecRemainingPlatforms.Count() > 1 )
	{
		nVal.SetInt( m_vecRemainingPlatforms.Head() );
		m_vecRemainingPlatforms.Remove( 0 );
		m_OutputRemovePlatform.FireOutput( nVal, this, this );
	}

	// The which one is supposed to be the safe platform
	int nIndex = RandomInt( 0, m_vecRemainingPlatforms.Count() - 1 );
	int nSafePlatform = m_vecRemainingPlatforms[ nIndex ];
	nVal.SetInt( nSafePlatform );
	m_OutputSafePlatform.FireOutput( nVal, this, this );
}

void CTFHalloweenMinigame_FallingPlatforms::FireGameEvent( IGameEvent * pEvent )
{
	const char *pszEventName = pEvent->GetName();
	
	// In the falling platforms game, we dont want ghosts to get stuck in platforms.
	// This hack sets the collision of ghosts to collide with triggers, but not with
	// other players.  This should allow us to use the relative teleport trigger on the
	// ghosts to prevent stuckage.
	if ( m_bIsActive && !V_strcmp( pszEventName, "player_turned_to_ghost" )  )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( pEvent->GetInt( "userid" ) ) );
		if ( pPlayer )
		{
			pPlayer->SetSolid( SOLID_BBOX );
			pPlayer->SetSolidFlags( FSOLID_NOT_STANDABLE );
			pPlayer->SetCollisionGroup( COLLISION_GROUP_DEBRIS_TRIGGER ); // Dont run into other players
		}
	}

	BaseClass::FireGameEvent( pEvent );
}

#endif



#ifdef GAME_DLL
BEGIN_DATADESC( CTFMinigameLogic )

	DEFINE_INPUTFUNC( FIELD_INTEGER, "TeleportToMinigame", InputTeleportToMinigame ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetAdvantageTeam", InputSetAdvantageTeam ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TeleportToRandomMinigame", InputTeleportToRandomMinigame ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ReturnFromMinigame", InputReturnFromMinigame ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( tf_logic_minigames, CTFMinigameLogic );
IMPLEMENT_NETWORKCLASS_ALIASED( TFMinigameLogic, DT_TFMinigameLogic )

BEGIN_NETWORK_TABLE_NOBASE( CTFMinigameLogic, DT_TFMinigameLogic )
#ifdef CLIENT_DLL
	RecvPropEHandle( RECVINFO( m_hActiveMinigame ) ),
#else
	SendPropEHandle( SENDINFO( m_hActiveMinigame ) ),
#endif
END_NETWORK_TABLE()

CTFMinigameLogic* CTFMinigameLogic::m_sMinigameLogic = NULL;

CTFMinigameLogic::CTFMinigameLogic()
{
	m_hActiveMinigame = NULL;
	m_sMinigameLogic = this;
#ifdef GAME_DLL
	m_iAdvantagedTeam = NO_TEAM_ADVANTAGE;
#endif
}

CTFMinigameLogic::~CTFMinigameLogic()
{
	if ( m_sMinigameLogic == this )
	{
		m_sMinigameLogic = NULL;
	}
}

#ifdef GAME_DLL

void CTFMinigameLogic::TeleportToMinigame( int nMiniGameIndex )
{
	Assert( m_hActiveMinigame == NULL );
	CTFMiniGame *pMinigame = static_cast< CTFMiniGame* >( IMinigameAutoList::AutoList()[ nMiniGameIndex ] );

	if ( pMinigame )
	{
		m_hActiveMinigame = pMinigame;
		m_hActiveMinigame->SetAdvantagedTeam( m_iAdvantagedTeam );
		m_hActiveMinigame->TeleportAllPlayers( );
		m_iAdvantagedTeam = NO_TEAM_ADVANTAGE;
	}
}

void CTFMinigameLogic::ReturnFromMinigame()
{
	if ( m_hActiveMinigame )
	{
		m_hActiveMinigame->ReturnAllPlayers();
	}

	m_hActiveMinigame = NULL;
}


void CTFMinigameLogic::InputTeleportToMinigame( inputdata_t &inputdata )
{
	int nInput = inputdata.value.Int();

	if ( nInput >= 0 && nInput < IMinigameAutoList::AutoList().Count() )
	{
		TeleportToMinigame( nInput );
	}
}

void CTFMinigameLogic::InputSetAdvantageTeam( inputdata_t &inputdata )
{
	m_iAdvantagedTeam = FStrEq( inputdata.value.String(), "red" ) ? TF_TEAM_RED : TF_TEAM_BLUE;
}

void CTFMinigameLogic::InputReturnFromMinigame( inputdata_t &inputdata )
{
	ReturnFromMinigame();
}

void CTFMinigameLogic::InputTeleportToRandomMinigame( inputdata_t &inputdata )
{
	static int nLastChosenIndex = -1;
	CUtlVector< int > m_vecRandomableMiniGames;
	FOR_EACH_VEC( IMinigameAutoList::AutoList(), i )
	{
		CTFMiniGame *pMinigame = static_cast< CTFMiniGame* >( IMinigameAutoList::AutoList()[ i ] );
		if ( pMinigame->AllowedInRandom() && nLastChosenIndex != i )
		{
			m_vecRandomableMiniGames.AddToTail( i );
		}
	}

	if ( !m_vecRandomableMiniGames.IsEmpty() )
	{
		int nChosenIndex = m_vecRandomableMiniGames[ RandomInt( 0, m_vecRandomableMiniGames.Count() - 1 ) ];
		nLastChosenIndex = nChosenIndex;
		TeleportToMinigame( nChosenIndex );
	}
}

#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CConditionFortuneTellerEffect 
#ifdef GAME_DLL
	: public CGameEventListener
#endif
{
public:

	CConditionFortuneTellerEffect( const char* pszActivateSound, ETFCond eCond )
		: m_pszActivateSound( pszActivateSound )
		, m_eCondition( eCond )
		, m_bUseTimer( false )
	{}

	void OnActivateEffect( bool bUseTimer )
	{
#ifdef GAME_DLL
		m_bUseTimer = bUseTimer;

		float flConditionDuration = m_bUseTimer ? tf_fortune_teller_fortune_duration.GetFloat() : PERMANENT_CONDITION;

		CUtlVector< CTFPlayer* > vecPlayers;
		CollectPlayers<CTFPlayer>( &vecPlayers, TEAM_ANY, true );
		for ( CTFPlayer *pPlayer : vecPlayers )
		{
			// Permanently add condition.  We'll remove it when we're done
			pPlayer->m_Shared.AddCond( m_eCondition, flConditionDuration );
		}

		ListenForGameEvent( "player_spawn" );
#endif
	}

	void OnDeactivateEffect()
	{
#ifdef GAME_DLL
		m_bUseTimer = false;
		StopListeningForAllEvents();
		
		CUtlVector< CTFPlayer* > vecPlayers;
		CollectPlayers<CTFPlayer>( &vecPlayers, TEAM_ANY, true );

		for ( CTFPlayer *pPlayer : vecPlayers )
		{
			// We're done.  Remove this condition
			pPlayer->m_Shared.RemoveCond( m_eCondition );
		}
#endif
	}

#ifdef GAME_DLL
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE
	{
		// don't do anything when players are in hell
		if ( TFGameRules() && TFGameRules()->ArePlayersInHell() )
			return;

		float flConditionDuration = m_bUseTimer ? tf_fortune_teller_fortune_duration.GetFloat() : PERMANENT_CONDITION;

		// Add the condition to anyone who spawns in
		if ( FStrEq( event->GetName(), "player_spawn" ) )
		{
			const int nUserID = event->GetInt( "userid" );
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( nUserID ) );
			if( pPlayer )
			{
				pPlayer->m_Shared.AddCond( m_eCondition, flConditionDuration );
			}
		}
	}
#endif

	const char *GetActivationSound() const
	{
		return m_pszActivateSound;
	}

	const char *m_pszActivateSound;
	ETFCond m_eCondition;
	bool m_bUseTimer;
};

static CConditionFortuneTellerEffect g_FortuneTellerEffect_BalloonHead = { "Announcer.SD_Event_BigHeadCurse", TF_COND_BALLOON_HEAD };							// FIXME: need 2014 sound link
static CConditionFortuneTellerEffect g_FortuneTellerEffect_MeleeOnly = { "Announcer.SD_Event_NoGunsCurse", TF_COND_MELEE_ONLY };
static CConditionFortuneTellerEffect g_FortuneTellerEffect_SwimmingCurse = { "Announcer.SD_Event_SwimmingCurse", TF_COND_SWIMMING_CURSE };			// FIXME: need 2014 sound link

static CConditionFortuneTellerEffect *g_GlobalFortuneTellerEffects[] =
{
	&g_FortuneTellerEffect_BalloonHead,
	&g_FortuneTellerEffect_MeleeOnly,
	&g_FortuneTellerEffect_SwimmingCurse,
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static const char *s_pszFortuneTellerSpinSound = "Halloween.WheelofFateQuiet";

// Data Description
BEGIN_DATADESC( CTFHalloweenFortuneTeller )
#ifdef GAME_DLL

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableFortuneTelling",InputEnableFortuneTelling ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableFortuneTelling",InputDisableFortuneTelling ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFortuneTelling", InputStartFortuneTelling ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EndFortuneTelling", InputEndFortuneTelling ),

	DEFINE_OUTPUT( m_OnFortuneWarning, "OnFortuneWarning" ),
	DEFINE_OUTPUT( m_OnFortuneTold, "OnFortuneTold" ),
	DEFINE_OUTPUT( m_OnFortuneCurse, "OnFortuneCurse" ),
	DEFINE_OUTPUT( m_OnFortuneEnd, "OnFortuneEnd" ),

	DEFINE_KEYFIELD( m_iszRedTeleport, FIELD_STRING, "red_teleport" ),
	DEFINE_KEYFIELD( m_iszBlueTeleport, FIELD_STRING, "blue_teleport" ),

#endif // GAME_DLLs
END_DATADESC()


LINK_ENTITY_TO_CLASS( halloween_fortune_teller, CTFHalloweenFortuneTeller );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHalloweenFortuneTeller::CTFHalloweenFortuneTeller()
{
#ifdef GAME_DLL
	m_bUseTimer = false;
	m_bWasUsingTimer = false;
#endif // GAME_DLL
}

CTFHalloweenFortuneTeller::~CTFHalloweenFortuneTeller()
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::Precache()
{
	PrecacheScriptSound( s_pszFortuneTellerSpinSound );

	for ( const auto *pEffect : g_GlobalFortuneTellerEffects )
	{
		PrecacheScriptSound( pEffect->GetActivationSound() );
	}

	PrecacheModel( "models/bots/merasmus/merasmas_misfortune_teller.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::Spawn()
{
	Precache();

	SetThink( NULL );

	SetModel( "models/bots/merasmus/merasmas_misfortune_teller.mdl" );

	ResetSequence( LookupSequence( "ref" ) );

#ifdef GAME_DLL
	ResetTimer();

	ListenForGameEvent( "sentry_on_go_active" );
#endif // GAME_DLLFireGameEvent
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::UpdateOnRemove()
{
#ifdef GAME_DLL
	if ( m_pActiveFortune )
	{
		m_pActiveFortune->OnDeactivateEffect();
		m_pActiveFortune = NULL;
	}
#endif // GAME_DLL

	BaseClass::UpdateOnRemove();
}


#ifdef GAME_DLL

void CTFHalloweenFortuneTeller::InputEnableFortuneTelling( inputdata_t & )
{
	m_bWasUsingTimer = m_bUseTimer;
	m_bUseTimer = true;

	if ( m_pActiveFortune )
	{
		m_pActiveFortune->OnDeactivateEffect();
		m_pActiveFortune = NULL;
	}

	if ( !m_bWasUsingTimer )
		UpdateFortuneTellerTime();
}

void CTFHalloweenFortuneTeller::InputDisableFortuneTelling( inputdata_t & )
{
	m_bWasUsingTimer = m_bUseTimer;
	m_bUseTimer = false;

	if ( m_pActiveFortune )
	{
		m_pActiveFortune->OnDeactivateEffect();
		m_pActiveFortune = NULL;
	}

	// keep track of when we pause
	if ( m_bWasUsingTimer )
		PauseTimer();
}

void CTFHalloweenFortuneTeller::InputStartFortuneTelling( inputdata_t & )
{
	// don't manually call this while using timer
	Assert( !m_bUseTimer );
	StartFortuneTell();
}

void CTFHalloweenFortuneTeller::InputEndFortuneTelling( inputdata_t & )
{
	// don't manually call this while using timer
	Assert( !m_bUseTimer );
	EndFortuneTell();
}

void CTFHalloweenFortuneTeller::FireGameEvent( IGameEvent* pEvent )
{
	const char *pszEventName = pEvent->GetName();

	if ( FStrEq( pszEventName, "sentry_on_go_active" ) )
	{
		// While curses are active, no ammo in sentries
		if ( m_pActiveFortune )
		{
			TFGameRules()->RemoveAllSentriesAmmo();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::UpdateFortuneTellerTime()
{
	// unpause time, compute new start time
	m_flStartTime = gpGlobals->curtime - ( m_flPauseTime - m_flStartTime );

	const float flWarningTime = tf_fortune_teller_interval_time.GetFloat() - tf_fortune_teller_warning_time.GetFloat();
	float flTimePast = gpGlobals->curtime - m_flStartTime;
	// warning time
	if ( flTimePast < flWarningTime )
	{
		float flTimeBeforeEvent = flWarningTime - flTimePast;
		SetContextThink( &CTFHalloweenFortuneTeller::StartFortuneWarning, gpGlobals->curtime + flTimeBeforeEvent, "StartFortuneWarning" );
	}
	else
	{
		float flTimeBeforeEvent = tf_fortune_teller_interval_time.GetFloat() - flTimePast;
		SetContextThink( &CTFHalloweenFortuneTeller::StartFortuneTell, gpGlobals->curtime + flTimeBeforeEvent, "StartFortuneTell" );
	}
}

void CTFHalloweenFortuneTeller::PauseTimer()
{
	m_flPauseTime = gpGlobals->curtime;

	// Cancel any fortunes in flight
	SetContextThink( NULL, 0, "StartFortuneWarning" );
	SetContextThink( NULL, 0, "StartFortuneTell" );
	SetContextThink( NULL, 0, "TellFortune" );
	SetContextThink( NULL, 0, "EndFortuneTell" );
}


void CTFHalloweenFortuneTeller::ResetTimer()
{
	m_flStartTime = m_flPauseTime = gpGlobals->curtime;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::StartFortuneWarning()
{
	// disable nagging
	TFGameRules()->StopDoomsdayTicketsTimer();

	m_OnFortuneWarning.FireOutput( this, this );
	SetContextThink( &CTFHalloweenFortuneTeller::StartFortuneTell, gpGlobals->curtime + tf_fortune_teller_warning_time.GetFloat(), "StartFortuneTell" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::StartFortuneTell()
{
	// Common effects.
	m_OnFortuneTold.FireOutput( this, this );

	// Broadcast the spin sound for the team-wide
	TFGameRules()->BroadcastSound( 255, s_pszFortuneTellerSpinSound );

	// Set when to actually perform the fortune telling
	SetContextThink( &CTFHalloweenFortuneTeller::TellFortune, gpGlobals->curtime + 6.1f, "TellFortune" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::EndFortuneTell()
{
	// resume nagging
	TFGameRules()->StartDoomsdayTicketsTimer();

	// Cancel any fortunes in flight
	SetContextThink( NULL, 0, "TellFortune" );

	m_OnFortuneEnd.FireOutput( this, this );

	if ( m_pActiveFortune )
	{
		m_pActiveFortune->OnDeactivateEffect();
		m_pActiveFortune = NULL;
	}

	if ( m_bUseTimer )
	{
		// restart fortune teller time
		ResetTimer();
		UpdateFortuneTellerTime();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFHalloweenFortuneTeller::TellFortune()
{
	if ( m_pActiveFortune )
	{
		m_pActiveFortune->OnDeactivateEffect();
		m_pActiveFortune = NULL;
	}

	m_pActiveFortune = g_GlobalFortuneTellerEffects[ RandomInt( 0, ARRAYSIZE( g_GlobalFortuneTellerEffects ) - 1 ) ];
	if ( !m_pActiveFortune )
		return;

	// prevent stickies trap before the dance off
	TFGameRules()->RemoveAllProjectiles();
	TFGameRules()->RemoveAllSentriesAmmo();

	// Teleport RED
	CUtlVector< CTFPlayer* > vecRedPlayers;
	CollectPlayers<CTFPlayer>( &vecRedPlayers, TF_TEAM_RED, true );
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_RED, m_iszRedTeleport.ToCStr(), &vecRedPlayers );
	// Teleport BLUE
	CUtlVector< CTFPlayer* > vecBluePlayers;
	CollectPlayers<CTFPlayer>( &vecBluePlayers, TF_TEAM_BLUE, true );
	TFGameRules()->TeleportPlayersToTargetEntities( TF_TEAM_BLUE, m_iszBlueTeleport.ToCStr(), &vecBluePlayers );

	CUtlVector< CTFPlayer* > vecPlayers;
	CollectPlayers<CTFPlayer>( &vecPlayers, TEAM_ANY, true );
	// Effects
	for( CTFPlayer * pPlayer : vecPlayers )
	{
		if ( pPlayer->m_Shared.IsCarryingObject() && pPlayer->m_Shared.GetCarriedObject() != NULL)
		{
			pPlayer->m_Shared.GetCarriedObject()->DetonateObject();
		}

		// Do a zoom effect
		pPlayer->SetFOV( pPlayer, tf_teleporter_fov_start.GetInt() );
		pPlayer->SetFOV( pPlayer, 0, 1.f, tf_teleporter_fov_start.GetInt() );

		// Screen flash
		color32 fadeColor = {255,255,255,100};
		UTIL_ScreenFade( pPlayer, fadeColor, 0.25, 0.4, FFADE_IN );

		// Plays music and makes it so Taunt() always performs a thriller
		pPlayer->m_Shared.AddCond( TF_COND_HALLOWEEN_THRILLER, 6.f );
	}

	// Speak after the 1st thriller
	SetContextThink( &CTFHalloweenFortuneTeller::SpeakThink, gpGlobals->curtime + 3.f, "SpeakThink" );
	// Apply the effect after the 2nd thriller
	SetContextThink( &CTFHalloweenFortuneTeller::ApplyFortuneEffect, gpGlobals->curtime + 6.f, "FortuneActivate" );

	const float flDanceTime = 0.5f;
	const float flDanceDuration = 2.75f;

	// Queue up the thrillers
	SetContextThink( &CTFHalloweenFortuneTeller::DanceThink, gpGlobals->curtime + flDanceTime, "DanceThink1" );
	SetContextThink( &CTFHalloweenFortuneTeller::DanceThink, gpGlobals->curtime + flDanceTime + flDanceDuration, "DanceThink2" );
}

void CTFHalloweenFortuneTeller::ApplyFortuneEffect()
{
	m_OnFortuneCurse.FireOutput( this, this );

	// Apply the actual effects.
	if ( m_pActiveFortune )
		m_pActiveFortune->OnActivateEffect( m_bUseTimer );

	if ( m_bUseTimer )
	{
		SetContextThink( &CTFHalloweenFortuneTeller::EndFortuneTell, gpGlobals->curtime + tf_fortune_teller_fortune_duration.GetFloat(), "EndFortuneTell" );
	}
}

void CTFHalloweenFortuneTeller::SpeakThink()
{
	float flSoundDuration = 0.0f;

	if ( m_pActiveFortune )
	{
		// Speak
		const char *pszActivationSound = m_pActiveFortune->GetActivationSound();
		TFGameRules()->BroadcastSound( 255, pszActivationSound );
		// Do speaking anim
		SetSequence( LookupSequence( "jaw_talking" ) );
		//flSoundDuration = enginesound->GetSoundDuration( pszActivationSound );
	}

	// Tell ourselves to stop speaking after awhile
	SetContextThink( &CTFHalloweenFortuneTeller::StopTalkingAnim, gpGlobals->curtime + flSoundDuration, "StopTalkingAnim" );
}

void CTFHalloweenFortuneTeller::StopTalkingAnim()
{
	SetSequence( LookupSequence( "ref" ) );
}

void CTFHalloweenFortuneTeller::DanceThink()
{
	CUtlVector< CTFPlayer* > vecPlayers;
	CollectPlayers<CTFPlayer>( &vecPlayers, TEAM_ANY, true );

	// No mere mortal can resist the magic of Merasmus
	for( CTFPlayer * pPlayer : vecPlayers )
	{
		pPlayer->Taunt();
	}
}

#endif // GAME_DLLs
