//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_populator.cpp
// KeyValues driven procedural population system
// Michael Booth, April 2011

#include "cbase.h"

#include "tf_population_manager.h"
#include "tf_team.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_shareddefs.h"
#include "filesystem.h"
#include "tf_obj_sentrygun.h"
#include "tf_objective_resource.h"
#include "econ_entity_creation.h"
#include "econ_wearable.h"
#include "tf_upgrades.h"
#include "tf_item_powerup_bottle.h"
#include "tf_gc_server.h"
#include "vote_controller.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"
#include "econ_item_schema.h"
#include "tf_upgrades_shared.h"

#include "etwprof.h"

extern ConVar tf_mvm_skill;
extern ConVar tf_mm_trusted;
extern ConVar tf_mvm_respec_limit;
extern ConVar tf_mvm_respec_credit_goal;
extern ConVar tf_mvm_buybacks_method;
extern ConVar tf_mvm_buybacks_per_wave;

void MvMMissionCycleFileChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->LoadMissionCycleFile();
	}
}

ConVar tf_mvm_missioncyclefile( "tf_mvm_missioncyclefile", "tf_mvm_missioncycle.res", FCVAR_NONE, "Name of the .res file used to cycle mvm misisons", MvMMissionCycleFileChangedCallback );

ConVar tf_populator_debug( "tf_populator_debug", "0", TF_MVM_FCVAR_CHEAT );
ConVar tf_populator_active_buffer_range( "tf_populator_active_buffer_range", "3000", FCVAR_CHEAT, "Populate the world this far ahead of lead raider, and this far behind last raider" );

ConVar tf_mvm_default_sentry_buster_damage_dealt_threshold( "tf_mvm_default_sentry_buster_damage_dealt_threshold", "3000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_mvm_default_sentry_buster_kill_threshold( "tf_mvm_default_sentry_buster_kill_threshold", "15", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );


void MinibossScaleChangedCallBack( IConVar *pVar, const char *pOldString, float flOldValue )
{
	ConVarRef cVarRef( pVar );
	// Change the scale of all the minibosses
	for( int iPlayerIndex = 1 ; iPlayerIndex <= MAX_PLAYERS; iPlayerIndex++ )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->IsMiniBoss() )
		{
			pPlayer->SetModelScale( cVarRef.GetFloat(), 1.0f );
		}
	}
}
ConVar tf_mvm_miniboss_scale( "tf_mvm_miniboss_scale", "1.75", FCVAR_REPLICATED | TF_MVM_FCVAR_CHEAT, "Full body scale for minibosses.", MinibossScaleChangedCallBack );

ConVar tf_mvm_disconnect_on_victory( "tf_mvm_disconnect_on_victory", "0", FCVAR_REPLICATED, "Enable to Disconnect Players after completing MvM" );
ConVar tf_mvm_victory_reset_time( "tf_mvm_victory_reset_time", "60.0", FCVAR_REPLICATED, "Seconds to wait after MvM victory before cycling to the next mission.  (Only used if tf_mvm_disconnect_on_victory is false.)" );
ConVar tf_mvm_victory_disconnect_time( "tf_mvm_victory_disconnect_time", "180.0", FCVAR_REPLICATED, "Seconds to wait after MvM victory before kicking players.  (Only used if tf_mvm_disconnect_on_victory is true.)" );

ConVar tf_mvm_endless_force_on( "tf_mvm_endless_force_on", "0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Force MvM Endless mode on" );
ConVar tf_mvm_endless_wait_time( "tf_mvm_endless_wait_time", "5.0f", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
ConVar tf_mvm_endless_bomb_reset( "tf_mvm_endless_bomb_reset", "5", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Number of Waves to Complete before bomb reset" );
ConVar tf_mvm_endless_bot_cash( "tf_mvm_endless_bot_cash", "120", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "In Endless, number of credits bots get per wave" );
ConVar tf_mvm_endless_tank_boost( "tf_mvm_endless_tank_boost", "0.2", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "In Endless, amount of extra health for the tank per wave" );


ConVar tf_populator_health_multiplier( "tf_populator_health_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar tf_populator_damage_multiplier( "tf_populator_damage_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_REPLICATED | FCVAR_CHEAT );

static bool HaveMap( const char *pszMapName )
{
	char szCanonName[64] = { 0 };
	V_strncpy( szCanonName, pszMapName, sizeof( szCanonName ) );
	IVEngineServer::eFindMapResult eResult = engine->FindMap( szCanonName, sizeof( szCanonName ) );

	switch ( eResult )
	{
	case IVEngineServer::eFindMap_Found:
	case IVEngineServer::eFindMap_NonCanonical:
		return true;
	case IVEngineServer::eFindMap_NotFound:
	case IVEngineServer::eFindMap_FuzzyMatch:
		// Maps that are contingent on just-in-time preparation should probably not be baked into cycle files... yet?
	case IVEngineServer::eFindMap_PossiblyAvailable:
		return false;
	}

	AssertMsg( false, "Unhandled engine->FindMap return value\n" );
	return false;
}

void MVMSkillChangedCallback( IConVar *pVar, const char *pOldString, float flOldValue )
{
	ConVarRef cVarRef( pVar );
	// Testing the effects of skill setting
	float flHealth = 1.f;	// Health modifier for bots
	float flDamage = 1.f;	// Damage modifier in bot vs player

	switch ( cVarRef.GetInt() )
	{
	case 1:
		{
			flHealth = 0.75f;
			flDamage = 0.75f;
			break;
		}
	case 2:
		{
			flHealth = 0.9f;
			flDamage = 0.9f;
			break;
		}
		// "Normal"
	case 3:
		{
			flHealth = 1.f;
			flDamage = 1.f;
			break;
		}
	case 4:
		{
			flHealth = 1.10f;
			flDamage = 1.10f;
			break;
		}
	case 5:
		{
			flHealth = 1.25f;
			flDamage = 1.25f;
			break;
		}
	}

	tf_populator_health_multiplier.SetValue( flHealth );
	tf_populator_damage_multiplier.SetValue( flDamage );
}

ConVar tf_mvm_skill( "tf_mvm_skill", "3", FCVAR_DONTRECORD | FCVAR_REPLICATED | TF_MVM_FCVAR_CHEAT, "Sets the challenge level of the invading bot army. 1 = easiest, 3 = normal, 5 = hardest", true, 1, true, 5, MVMSkillChangedCallback );

//-------------------------------------------------------------------------
// Console command to cheat and force victory.  (To test econ work, etc)
CON_COMMAND_F( tf_mvm_nextmission, "Load the next mission", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->CycleMission();
	}
}

// Console command to cheat and force victory.  (To test econ work, etc)
CON_COMMAND_F( tf_mvm_force_victory, "Force immediate victory.", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->JumpToWave( g_pPopulationManager->GetTotalWaveCount() - 1 );
		g_pPopulationManager->WaveEnd( true );
		g_pPopulationManager->MvMVictory();
	}
}

//-------------------------------------------------------------------------
CON_COMMAND_F( tf_mvm_checkpoint, "Save a checkpoint snapshot", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->SetCheckpoint( -1 );
	}
}

//-------------------------------------------------------------------------
CON_COMMAND_F( tf_mvm_checkpoint_clear, "Clear the saved checkpoint", FCVAR_CHEAT )
{
	if ( g_pPopulationManager )
	{
		g_pPopulationManager->ClearCheckpoint();
	}
}

//-------------------------------------------------------------------------
CON_COMMAND_F( tf_mvm_jump_to_wave, "Jumps directly to the given Mann Vs Machine wave number", FCVAR_CHEAT )
{
	if ( args.ArgC() <= 1 )
	{
		Msg( "Missing wave number\n" );
		return;
	}

	float fCleanMoneyPercent = -1.0f;
	if ( args.ArgC() >= 3 )
	{
		fCleanMoneyPercent = atof( args.Arg(2) );
	}

	// find the population manager
	CPopulationManager *manager = (CPopulationManager *)gEntList.FindEntityByClassname( NULL, "info_populator" );
	if ( !manager )
	{
		Msg( "No Population Manager found in the map\n" );
		return;
	}

	uint32 desiredWave = (uint32)Max( atoi( args.Arg(1) ) - 1, 0) ;
	manager->JumpToWave( desiredWave, fCleanMoneyPercent );
}

//-------------------------------------------------------------------------
CON_COMMAND_F( tf_mvm_debugstats, "Dumpout MvM Data", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( g_pPopulationManager )
	{
		g_pPopulationManager->DebugWaveStats();
	}
}

//-------------------------------------------------------------------------
// CPopulationManager
//-------------------------------------------------------------------------

BEGIN_DATADESC( CPopulationManager )
	DEFINE_THINKFUNC( Update ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_populator, CPopulationManager );
PRECACHE_REGISTER( info_populator );

CPopulationManager *g_pPopulationManager = NULL;

// initialized to zero (1st wave), and not reset unless game won or map change event received.
int CPopulationManager::m_checkpointWaveIndex = 0;
CUtlVector< CPopulationManager::CheckpointSnapshotInfo * > CPopulationManager::m_checkpointSnapshot;
int CPopulationManager::m_nNumConsecutiveWipes = 0;

// Mission Cycle Vars
static int s_iLastKnownMissionCategory = 1;
static int s_iLastKnownMission = 1;

//-------------------------------------------------------------------------
// CPopulationManager
//-------------------------------------------------------------------------
CPopulationManager::CPopulationManager( void )
{
	m_bIsInitialized = false;
	m_bAllocatedBots = false;
	m_popfileFull[ 0 ] = '\0';
	m_popfileShort[ 0 ] = '\0';
	m_nStartingCurrency = 0;
	m_nLobbyBonusCurrency = 0;
	m_canBotsAttackWhileInSpawnRoom = true;
	m_pTemplates = NULL;
	m_isRestoringCheckpoint = false;
	m_nRespawnWaveTime = 10;
	m_bFixedRespawnWaveTime = false;
	m_sentryBusterDamageDealtThreshold = tf_mvm_default_sentry_buster_damage_dealt_threshold.GetInt();
	m_sentryBusterKillThreshold = tf_mvm_default_sentry_buster_kill_threshold.GetInt();
	m_bCheckForCurrencyAchievement = true;
	m_bEndlessOn = false;
	m_bIsWaveJumping = false;
	m_bSpawningPaused = false;

	m_iCurrentWaveIndex = 0;
	m_nNumConsecutiveWipes = 0;
	m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;
	
	SetThink( &CPopulationManager::Update );
	SetNextThink( gpGlobals->curtime );

	g_pPopulationManager = this;
	m_pMVMStats = MannVsMachineStats_GetInstance();

	m_pKvpMvMMapCycle = NULL;
	
	ListenForGameEvent( "pve_win_panel" );

	// Endless
	m_randomizer.SetSeed( 0 );
	m_EndlessSeeds.Purge();
	for ( int i = 0; i < 27; i++ )
	{
		m_EndlessSeeds.AddToTail( m_randomizer.RandomInt( 0, INT_MAX ) );
	}
	EndlessParseBotUpgrades();
	m_bShouldResetFlag = false;

	m_bBonusRound = false;
	m_hBonusBoss = NULL;

	m_nRespecsAwarded = 0;
	m_nRespecsAwardedInWave = 0;
	m_nCurrencyCollectedForRespec = 0;
	m_PlayerRespecPoints.SetLessFunc( DefLessFunc (uint64) );
	m_PlayerRespecPoints.EnsureCapacity( MAX_PLAYERS );

	m_PlayerBuybackPoints.SetLessFunc( DefLessFunc( uint64 ) );
	m_PlayerBuybackPoints.EnsureCapacity( MAX_PLAYERS );
}

//-------------------------------------------------------------------------
CPopulationManager::~CPopulationManager()
{
	Reset();

	m_populatorVector.PurgeAndDeleteElements();
	m_waveVector.RemoveAll();

	if ( m_pTemplates )
	{
		m_pTemplates->deleteThis();
		m_pTemplates = NULL;
	}

	g_pPopulationManager = NULL;
}

//-------------------------------------------------------------------------
// Purpose : CPointEntity Override
//-------------------------------------------------------------------------
void CPopulationManager::Spawn( void )
{
	BaseClass::Spawn();
	Initialize();
}

//-------------------------------------------------------------------------
// Purpose : CGameEventListener
//-------------------------------------------------------------------------
void CPopulationManager::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	if ( V_strcmp( "pve_win_panel", pEventName ) == 0 )
	{
		// Always release people even if the match isn't ending
		// XXX(JohnS): This is just how the code was, but why wouldn't the match be ending?
		MarkAllCurrentPlayersSafeToLeave();
	}
}

//-------------------------------------------------------------------------
// Purpose :
//-------------------------------------------------------------------------
void CPopulationManager::PlayerDoneViewingLoot( const CTFPlayer* pPlayer )
{
	CUtlVector< const CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );

	if ( m_donePlayers.Find( pPlayer ) == m_donePlayers.InvalidIndex()
		&& playerVector.Find( pPlayer ) != playerVector.InvalidIndex() )
	{
		m_donePlayers.AddToTail( pPlayer );
			
		float flTimeRemaining = m_flMapRestartTime - gpGlobals->curtime;
		const float flMinTime = 15.f;
		
		if ( flTimeRemaining > flMinTime )
		{
			// Figure out if this is restart or kick to lobby time
			float flReduceTimeBy = ( tf_mm_trusted.GetBool() == true || tf_mvm_disconnect_on_victory.GetBool() == true ) 
								 ? tf_mvm_victory_disconnect_time.GetFloat()
								 : tf_mvm_victory_reset_time.GetFloat();

			// Each player can reduce the clock by a certain amount, based on how
			// many players there are
			flReduceTimeBy = ( flReduceTimeBy * 0.8 ) / playerVector.Count();
			flTimeRemaining -= flReduceTimeBy ;
			flTimeRemaining = Max( flTimeRemaining, flMinTime );

			m_flMapRestartTime = gpGlobals->curtime + flTimeRemaining;

			// Notify Users of new remaining time
			CBroadcastRecipientFilter filter;
			filter.MakeReliable();
			UserMessageBegin( filter, "MVMServerKickTimeUpdate" );
			WRITE_BYTE((uint8)flTimeRemaining);
			MessageEnd();
		}
	}
}

//-------------------------------------------------------------------------
// Purpose : Full Clear of Population Manager State and Data
//-------------------------------------------------------------------------
void CPopulationManager::Reset( void )
{	
	m_nStartingCurrency = 0;
	m_canBotsAttackWhileInSpawnRoom = true;
	m_nRespawnWaveTime = 10;
	m_bFixedRespawnWaveTime = false;
	m_sentryBusterDamageDealtThreshold = tf_mvm_default_sentry_buster_damage_dealt_threshold.GetInt();
	m_sentryBusterKillThreshold = tf_mvm_default_sentry_buster_kill_threshold.GetInt();
	m_bAdvancedPopFile = false;
	m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;
	m_bSpawningPaused = false;
	m_donePlayers.Purge();
	m_nRespecsAwardedInWave = 0;

	// don't clobber this value if we're wave jumping
	if ( !m_bIsWaveJumping )
	{
		m_iCurrentWaveIndex = 0;
	}

	m_defaultEventChangeAttributesName = "Default";
}

//-------------------------------------------------------------------------
// Purpose : Restart Population Manager at Current Wave
//-------------------------------------------------------------------------
bool CPopulationManager::Initialize( void )
{
	if ( ( TheNavMesh == NULL ) || ( TheNavMesh->GetNavAreaCount() <= 0 ) )
	{
		Warning( "No Nav Mesh CPopulationManager::Initialize for %s\n", m_popfileFull );
		return false;
	}

	Reset();

	if ( !Parse() )
	{
		Warning( "Parse Failed in CPopulationManager::Initialize for %s\n", m_popfileFull );
		return false;
	}


	if ( TFGameRules()->State_Get() == GR_STATE_PREGAME )
	{
		// new game
		ClearCheckpoint();
		m_iCurrentWaveIndex = 0;
		m_nNumConsecutiveWipes = 0;
		m_pMVMStats->SetCurrentWave( m_iCurrentWaveIndex );
	}
	else
	{
 		RestoreCheckpoint();

		// Restore Check Point is being called on RoundStart so this check is currently needed
		// Report loss to Stats
		if ( m_bIsInitialized && ( m_iCurrentWaveIndex > 0 || m_nNumConsecutiveWipes > 1 ) )
		{
			m_pMVMStats->RoundEvent_WaveEnd( false );
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "mvm_wave_failed" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}
	}

	if ( IsInEndlessWaves() )
	{
		EndlessRollEscalation();
	}

	m_bIsInitialized = true;
	UpdateObjectiveResource();
	DebugWaveStats();
	PostInitialize();

	// Space respecs based on the number allowed
	if ( tf_mvm_respec_limit.GetBool() )
	{
		int nAmount = tf_mvm_respec_limit.GetInt() + 1;
		tf_mvm_respec_credit_goal.SetValue( GetTotalPopFileCurrency() / nAmount );
	}

	m_nRespecsAwardedInWave = 0;

	return true;
}

//-------------------------------------------------------------------------
// Purpose : Precache data for PopulationManager, typically sounds
//-------------------------------------------------------------------------
void CPopulationManager::Precache( void )
{
	PrecacheScriptSound( "music.mvm_end_wave" );
	PrecacheScriptSound( "music.mvm_end_tank_wave" );
	PrecacheScriptSound( "music.mvm_end_mid_wave" );
	PrecacheScriptSound( "music.mvm_end_last_wave" );
	PrecacheScriptSound( "MVM.PlayerUpgraded" );
	PrecacheScriptSound( "MVM.PlayerBoughtIn" );
	PrecacheScriptSound( "MVM.PlayerUsedPowerup" );
	PrecacheScriptSound( "MVM.PlayerDied" );
	PrecacheScriptSound( "MVM.PlayerDiedScout" );
	PrecacheScriptSound( "MVM.PlayerDiedSniper" );
	PrecacheScriptSound( "MVM.PlayerDiedSoldier" );
	PrecacheScriptSound( "MVM.PlayerDiedDemoman" );
	PrecacheScriptSound( "MVM.PlayerDiedMedic" );
	PrecacheScriptSound( "MVM.PlayerDiedHeavy" );
	PrecacheScriptSound( "MVM.PlayerDiedPyro" );
	PrecacheScriptSound( "MVM.PlayerDiedSpy" );
	PrecacheScriptSound( "MVM.PlayerDiedEngineer" );

	BaseClass::Precache();
}

//-------------------------------------------------------------------------
bool CPopulationManager::FindPopulationFileByShortName( const char *pShortName, CUtlString &outFullName )
{
	// Form full path
	char szFullPath[MAX_PATH] = { 0 };
	V_sprintf_safe( szFullPath, MVM_POP_FILE_PATH "/%s.pop", pShortName );

	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		outFullName = szFullPath;
		return true;
	}

	// Check mapname_shorthand.pop
	V_sprintf_safe( szFullPath, MVM_POP_FILE_PATH "/%s_%s.pop", STRING( gpGlobals->mapname ), pShortName );
	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		outFullName = szFullPath;
		return true;
	}

	// If using special name "normal", check just scripts/population/mapname.pop as last resort
	V_sprintf_safe( szFullPath, MVM_POP_FILE_PATH "/%s.pop", STRING( gpGlobals->mapname ) );
	if ( g_pFullFileSystem->FileExists( szFullPath, "GAME" ) )
	{
		if ( !FStrEq( pShortName, "normal" ) )
		{
			Msg( "Population file '%s' not found, falling back to %s.pop\n", pShortName, STRING( gpGlobals->mapname ) );
		}
		outFullName = szFullPath;
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------
void CPopulationManager::FindDefaultPopulationFileShortNames( CUtlVector< CUtlString > &outVecShortNames )
{
	// Search for all loose pop files that are prefixed with the current map name
	char szBaseName[MAX_PATH] = { 0 };
	V_snprintf( szBaseName, sizeof( szBaseName ), MVM_POP_FILE_PATH "/%s*.pop", STRING(gpGlobals->mapname) );

	FileFindHandle_t popHandle;
	const char *pPopFileName = filesystem->FindFirstEx( szBaseName, "GAME", &popHandle );

	while ( pPopFileName && pPopFileName[ 0 ] != '\0' )
	{
		// Skip it if it's a directory or is the folder info
		if ( filesystem->FindIsDirectory( popHandle ) )
		{
			pPopFileName = filesystem->FindNext( popHandle );
			continue;
		}

		const char *pchPopPostfix = StringAfterPrefix( pPopFileName, STRING(gpGlobals->mapname) );
		if ( pchPopPostfix )
		{
			char szShortName[MAX_PATH] = { 0 };
			V_strncpy( szShortName, ( ( pchPopPostfix[ 0 ] == '_' ) ? ( pchPopPostfix + 1 ) : "normal" ), sizeof( szShortName ) ); // skip the '_'
			V_StripExtension( szShortName, szShortName, sizeof( szShortName ) );

			if ( outVecShortNames.Find( szShortName ) == outVecShortNames.InvalidIndex() )
			{
				outVecShortNames.AddToTail( szShortName );
			}
		}

		pPopFileName = filesystem->FindNext( popHandle );
	}

	filesystem->FindClose( popHandle );

	// Search for all pop files in the BSP next. Note that loose files override these (by short name)
	FileFindHandle_t popHandleBSP;
	const char *pPopFileNameBSP = filesystem->FindFirstEx( MVM_POP_FILE_PATH "/*.pop", "BSP", &popHandleBSP );

	while ( pPopFileNameBSP && pPopFileNameBSP[ 0 ] != '\0' )
	{
		// Skip it if it's a directory or is the folder info
		if ( filesystem->FindIsDirectory( popHandleBSP ) )
		{
			pPopFileNameBSP = filesystem->FindNext( popHandleBSP );
			continue;
		}

		char szShortName[MAX_PATH] = { 0 };
		V_strncpy( szShortName, pPopFileNameBSP, sizeof( szShortName ) );
		V_StripExtension( szShortName, szShortName, sizeof( szShortName ) );

		// Legacy: Prior to proper support for in-BSP pop files, maps could jankily match their popfile to their exact
		// map name in the BSP. Map this to "normal"
		if ( V_stricmp( szShortName, STRING(gpGlobals->mapname) ) == 0 )
		{
			V_strncpy( szShortName, "normal", sizeof( szShortName ) );
		}

		if ( outVecShortNames.Find( szShortName ) == outVecShortNames.InvalidIndex() )
		{
			outVecShortNames.AddToTail( szShortName );
		}

		pPopFileNameBSP = filesystem->FindNext( popHandleBSP );
	}

	filesystem->FindClose( popHandleBSP );

	// Always treat "normal" as the default pop-file
	int normalIdx = outVecShortNames.Find( "normal" );
	if ( normalIdx != outVecShortNames.InvalidIndex() && normalIdx != 0 )
	{
		outVecShortNames.Remove( normalIdx );
		outVecShortNames.AddToHead( "normal" );
	}
}

//-------------------------------------------------------------------------
const char *CPopulationManager::GetPopulationFilename( void )
{
	return m_popfileFull;
}

//-------------------------------------------------------------------------
const char *CPopulationManager::GetPopulationFilenameShort( void )
{
	return m_popfileShort;
}

//-------------------------------------------------------------------------
void CPopulationManager::SetPopulationFilename( const char *populationFile )
{
	m_bIsInitialized = false;
	V_strcpy_safe( m_popfileFull, populationFile );
	V_FileBase( m_popfileFull, m_popfileShort, sizeof( m_popfileShort ) );

	MannVsMachineStats_SetPopulationFile( m_popfileFull );
	ResetMap();

	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetMannVsMachineChallengeIndex( GetItemSchema()->FindMvmMissionByName( m_popfileFull ) );
		TFObjectiveResource()->SetMvMPopfileName( MAKE_STRING( m_popfileFull ) );
	}
}

//-------------------------------------------------------------------------
// Invoked when we are spawned at round (re)start
void CPopulationManager::SetupOnRoundStart( void )
{
	Initialize();
}

//-------------------------------------------------------------------------
// Continuously invoked to modify population over time
//-------------------------------------------------------------------------
void CPopulationManager::Update( void )
{
	VPROF_BUDGET( "CMissionPopulator::Update", "NextBot" );

	SetNextThink( gpGlobals->curtime );

	m_isRestoringCheckpoint = false;

	// update populators
	for( int i=0; i<m_populatorVector.Count(); ++i )
	{
		m_populatorVector[i]->Update();
	}

	// Update Current Wave
	CWave * pWave = GetCurrentWave();
	if ( pWave )
	{
		pWave->Update();
	}

	// Check for GAMEOVER for MapReset
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER )
	{
		if ( m_flMapRestartTime < gpGlobals->curtime )
		{
			if ( tf_mvm_disconnect_on_victory.GetBool() )
			{
				// Shut down the managed match now, ask GC to return players to lobbies.
				if ( !TFGameRules()->IsManagedMatchEnded() )
				{
					TFGameRules()->EndManagedMvMMatch( /* bKickPlayersToParties */ true );
				}
			}
			else
			{
				CycleMission();
			}
		}

		// If players haven't left via the GC returning them to parties by now, due to connection issues/GC down, etc,
		// kick them with a thanks-for-playing.
		if ( tf_mvm_disconnect_on_victory.GetBool() == true && m_flMapRestartTime + 5.0f < gpGlobals->curtime )
		{
			Log( "Kicking all players\n" );
			engine->ServerCommand( "kickall #TF_PVE_Disconnect\n" );
			CycleMission();
		}

		// See if the team got the bonus on every wave
		if ( m_bCheckForCurrencyAchievement )
		{
			if ( ( MannVsMachineStats_GetDroppedCredits() > 0 ) && ( MannVsMachineStats_GetMissedCredits() == 0 ) )
			{
				const char *pszName = IsAdvancedPopFile() ? "mvm_creditbonus_all_advanced" : "mvm_creditbonus_all";
				IGameEvent *event = gameeventmanager->CreateEvent( pszName );
				if ( event )
				{
					gameeventmanager->FireEvent( event );
				}

				m_bCheckForCurrencyAchievement = false;
			}
		}
	}
	else if ( TFGameRules()->State_Get() == GR_STATE_STARTGAME )
	{
		AllocateBots();
	}
}

//-------------------------------------------------------------------------
// Purpose: Invoked by the gamerules think to give us a chance to behave
//          like a sub-gamerules-thing.  This will always run when
//          gamerules thinks, unlike Update which runs in the entity-update
//          phase and doesn't run when we're hibernating etc..
//-------------------------------------------------------------------------
void CPopulationManager::GameRulesThink( void )
{
	// If we reach zero players in managed match mode, drop the match (but otherwise just hang out in our current state,
	// in bootcamp servers ad-hoc players may want to rejoin and keep playing/etc.., server hibernation will handle
	// shutting down the game if desired)
	CMatchInfo *pLiveMatch = GTFGCClientSystem()->GetLiveMatch();
	if ( pLiveMatch && !TFGameRules()->IsManagedMatchEnded() && pLiveMatch->GetNumActiveMatchPlayers() == 0 )
	{
		Log( "No players remaining, ending managed MvM\n" );
		TFGameRules()->EndManagedMvMMatch( /* bSendVictory */ false );
	}
}

//-------------------------------------------------------------------------
// Purpose : 
//-------------------------------------------------------------------------
void CPopulationManager::UpdateObjectiveResource( void )
{
	if ( m_waveVector.Count() == 0 || !TFObjectiveResource() )
	{
		return;
	}

	TFObjectiveResource()->SetMannVsMachineEventPopfileType( m_nMvMEventPopfileType );
	
	if ( IsInEndlessWaves() )
	{
		TFObjectiveResource()->SetMannVsMachineMaxWaveCount( 0 );
	}
	else
	{
		TFObjectiveResource()->SetMannVsMachineMaxWaveCount( m_waveVector.Count() );
	}

	TFObjectiveResource()->SetMannVsMachineWaveCount( m_iCurrentWaveIndex + 1 );

	const CWave *wave = GetCurrentWave();
	if ( wave )
	{
		TFObjectiveResource()->SetMannVsMachineWaveEnemyCount( wave->GetEnemyCount() );
		TFObjectiveResource()->ClearMannVsMachineWaveClassFlags();

		int i = 0;
		bool bHasEngineer = false;
		for ( i; i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW && i < wave->GetNumClassTypes(); ++i )
		{
			if ( !bHasEngineer )
			{
				const char* pszClassIconName = wave->GetClassIconName( i ).ToCStr();
				bHasEngineer |= FStrEq( pszClassIconName, "engineer" );
			}
			TFObjectiveResource()->SetMannVsMachineWaveClassName( i, wave->GetClassIconName( i ) );
			TFObjectiveResource()->SetMannVsMachineWaveClassCount( i, wave->GetClassCount( i ) );
			TFObjectiveResource()->AddMannVsMachineWaveClassFlags( i, wave->GetClassFlags( i ) );
		}

		if ( bHasEngineer )
		{
			if ( i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW )
			{
				TFObjectiveResource()->SetMannVsMachineWaveClassName( i, TFObjectiveResource()->GetTeleporterString() );
				TFObjectiveResource()->SetMannVsMachineWaveClassCount( i, 0 );
				TFObjectiveResource()->AddMannVsMachineWaveClassFlags( i, MVM_CLASS_FLAG_MISSION ); // only mission will flash
				i++;
			}
			else
			{
				AssertMsg( 0, "Failed to add teleporter icon to TFObjectiveResource" );
			}
		}

		for ( i; i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW; ++i )
		{
			TFObjectiveResource()->SetMannVsMachineWaveClassCount( i, 0 );
			TFObjectiveResource()->SetMannVsMachineWaveClassName( i, NULL_STRING );
			TFObjectiveResource()->AddMannVsMachineWaveClassFlags( i, MVM_CLASS_FLAG_NONE );
		}
	}
}

//-------------------------------------------------------------------------
// Purpose : Reset Players, Stats, CheckPoint
//-------------------------------------------------------------------------
void CPopulationManager::ResetMap( void )
{
	// Reset Scores
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

		if ( !pPlayer )
			continue;

		if ( FNullEnt( pPlayer->edict() ) )
			continue;

		if ( pPlayer->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS )
			continue;

		pPlayer->ResetScores();
	}

	// Reset Stats and go to wave 0 clean
	m_pMVMStats->ResetStats( );
	ResetRespecPoints();
	ClearCheckpoint();

	for ( int i = 1; i <= MAX_PLAYERS; ++i )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer )
			continue;

		if ( pTFPlayer->IsBot() )
			continue;

		pTFPlayer->ResetRefundableUpgrades();
	}

	JumpToWave( 0, 0 );
}

//-------------------------------------------------------------------------
void CPopulationManager::CycleMission ( void )
{
	bool isLoaded = true;
	if ( !m_pKvpMvMMapCycle )
	{
		isLoaded = LoadMissionCycleFile();
	}

	const char * pCurrentMap = STRING( gpGlobals->mapname );
	char szCurrentPopfile[MAX_PATH];
	V_FileBase( m_popfileFull, szCurrentPopfile, sizeof( szCurrentPopfile ) );

	//engine->GetMap
	if ( isLoaded )
	{
		int iMaxCat = m_pKvpMvMMapCycle->GetInt( "categories", 0 );

		for ( int iCat = 1; iCat <= iMaxCat; iCat++ )
		{
			KeyValues *pCategory = m_pKvpMvMMapCycle->FindKey( UTIL_VarArgs( "%d", iCat ), false );

			if ( pCategory )
			{
				int iMapCount = pCategory->GetInt( "count", 0 );
				for ( int iMap = 1; iMap <= iMapCount; ++iMap )
				{
					KeyValues *pMission = pCategory->FindKey( UTIL_VarArgs( "%d", iMap ), false );
					if ( pMission )
					{
						const char * pMap = pMission->GetString( "map", "" );
						const char * pPopfile = pMission->GetString( "popfile", "" );
						
						if ( !Q_strcmp( pCurrentMap, pMap ) && !Q_strcmp( szCurrentPopfile, pPopfile ) )
						{
							// match, advance to the next entry and use those values
							int nextMap = (iMap % iMapCount) + 1;

							KeyValues *pNextMission = pCategory->FindKey( UTIL_VarArgs( "%d", nextMap ), false );
							if ( LoadMvMMission( pNextMission ) )
							{
								s_iLastKnownMission = nextMap;
								s_iLastKnownMissionCategory = iCat;
								return;
							}
							// Next map is invalid, load last known
							LoadLastKnownMission();
							return;
						}
					}
				} // for ( int iMap = 1; iMap <= iMapCount; ++iMap )
			}
		}  // for ( int iCat = 1; iCat <= iMaxCat; iCat++ )
	}  

	// Unable to load mvm mapcycle, load last known
	LoadLastKnownMission();
}

//-------------------------------------------------------------------------
bool CPopulationManager::LoadMissionCycleFile( void )
{
	if ( m_pKvpMvMMapCycle )
	{
		m_pKvpMvMMapCycle->deleteThis();
	}

	m_pKvpMvMMapCycle = new KeyValues( tf_mvm_missioncyclefile.GetString() );

	return m_pKvpMvMMapCycle->LoadFromFile( g_pFullFileSystem, tf_mvm_missioncyclefile.GetString(), "MOD" );
}

//-------------------------------------------------------------------------
void CPopulationManager::LoadLastKnownMission( void )
{
	// 
	//
	bool isLoaded = true;
	if ( !m_pKvpMvMMapCycle )
	{
		isLoaded = LoadMissionCycleFile();
	}

	if ( !isLoaded )
	{
		ResetMap();
		return;
	}

	// Grab the category
	KeyValues *pCategory = m_pKvpMvMMapCycle->FindKey( UTIL_VarArgs( "%d", s_iLastKnownMissionCategory ), false );
	if ( pCategory )
	{
		KeyValues *pNextMission = pCategory->FindKey( UTIL_VarArgs( "%d", s_iLastKnownMission ), false );
		if ( LoadMvMMission( pNextMission ) )
		{
			return;
		}
	}

	// Did not succeed
	// Attempt to load the first Category / Mission instead
	pCategory = m_pKvpMvMMapCycle->FindKey( UTIL_VarArgs( "%d", 1 ), false );
	if ( pCategory )
	{
		KeyValues *pNextMission = pCategory->FindKey( UTIL_VarArgs( "%d", 1 ), false );
		if ( LoadMvMMission( pNextMission ) )
		{
			s_iLastKnownMissionCategory = 1;
			s_iLastKnownMission = 1;
			return;
		}
	}

	// if 1,1 does not exist (likely due to a bad .res file) just reset map instead
	ResetMap();
}

//-------------------------------------------------------------------------
// Returns True if the mission was successfully found and loaded
//
bool CPopulationManager::LoadMvMMission ( KeyValues *pNextMission ) 
{
	if ( !pNextMission )
		return false;

	const char * pNextMap = pNextMission->GetString( "map", NULL );
	const char * pNextPopfile = pNextMission->GetString( "popfile", NULL );

	if ( pNextMap && pNextPopfile )
	{
		char szPopFileName[MAX_PATH];
		Q_snprintf( szPopFileName, sizeof( szPopFileName ), MVM_POP_FILE_PATH "/%s.pop", pNextPopfile );
		if ( g_pFullFileSystem->FileExists( szPopFileName, "MOD" ) && HaveMap( pNextMap ) )
		{
			engine->ChangeLevel( pNextMap, NULL );
			TFGameRules()->SetNextMvMPopfile( pNextPopfile );
			return true;
		}
	}
	return false;
}

//-------------------------------------------------------------------------
bool CPopulationManager::IsValidMvMMap( const char *pszMapName )
{
	if ( !m_pKvpMvMMapCycle )
	{
		LoadMissionCycleFile();
	}

	if ( pszMapName && m_pKvpMvMMapCycle )
	{
		int iMaxCat = m_pKvpMvMMapCycle->GetInt( "categories", 0 );
		for ( int iCat = 1; iCat <= iMaxCat; iCat++ )
		{
			KeyValues *pCategory = m_pKvpMvMMapCycle->FindKey( UTIL_VarArgs( "%d", iCat ), false );
			if ( pCategory )
			{
				int iMapCount = pCategory->GetInt( "count", 0 );
				for ( int iMap = 1; iMap <= iMapCount; ++iMap )
				{
					KeyValues *pMission = pCategory->FindKey( UTIL_VarArgs( "%d", iMap ), false );
					if ( pMission )
					{
						const char *pszMap = pMission->GetString( "map", "" );
						if ( Q_strcmp( pszMapName, pszMap ) == 0 )
						{
							// Valid?
							return ( HaveMap( pszMapName ) ? true : false );
						}
					}
				}
			}
		}
	}

	return false;
}
//-------------------------------------------------------------------------
void CPopulationManager::ShowNextWaveDescription( void )
{
	UpdateObjectiveResource();
}

//-------------------------------------------------------------------------
void CPopulationManager::StartCurrentWave( void )
{
	if ( TFObjectiveResource() )
	{
		TFObjectiveResource()->SetMannVsMachineNextWaveTime( 0 );
		TFObjectiveResource()->SetMannVsMachineBetweenWaves( false );
	}

	UpdateObjectiveResource();
	m_pMVMStats->RoundEvent_WaveStart();

	TFGameRules()->State_Transition( GR_STATE_RND_RUNNING );


	m_nRespecsAwardedInWave = 0;

	FOR_EACH_MAP( m_PlayerBuybackPoints, i )
	{
		m_PlayerBuybackPoints[i] = tf_mvm_buybacks_per_wave.GetInt();
	}
}

//-------------------------------------------------------------------------
CWave * CPopulationManager::GetCurrentWave( void )
{
	if ( !m_bIsInitialized || m_waveVector.Count() == 0 )
		return NULL;

	// Wrap for Infinite MVM
	if ( IsInEndlessWaves() )
	{
		return m_waveVector[m_iCurrentWaveIndex % m_waveVector.Count() ];
	}
	else if ( (int)m_iCurrentWaveIndex < m_waveVector.Count() )
	{
		return m_waveVector[m_iCurrentWaveIndex];
	}
	
	return NULL;
}

//-------------------------------------------------------------------------
void CPopulationManager::JumpToWave( uint32 waveNumber, float fCleanMoneyPercent /*= -1.0f*/  )
{
	if ( !IsInEndlessWaves() && (waveNumber >= (uint32)m_waveVector.Count() ) )
	{
		if ( m_waveVector.Count() > 0 )
		{
			Warning( "Invalid wave number\n" );
		}
		return;
	}

	CWave * pWave = GetCurrentWave();
	if ( pWave )
	{
		pWave->ForceFinish();	
	}
	m_bIsWaveJumping = true;

	m_iCurrentWaveIndex = waveNumber;

	// Set Money for New Wave
	if ( fCleanMoneyPercent != -1.0f )
	{
		ClearCheckpoint();

		Initialize();
		m_pMVMStats->ResetStats( );

		for ( m_iCurrentWaveIndex = 0; m_iCurrentWaveIndex < waveNumber; ++m_iCurrentWaveIndex )
		{
			pWave = GetCurrentWave();
			if ( pWave )
			{
				int nCurrency = pWave->GetTotalCurrency();
				m_pMVMStats->SetCurrentWave( m_iCurrentWaveIndex );

				if ( m_iCurrentWaveIndex < waveNumber )
				{
					m_pMVMStats->RoundEvent_CreditsDropped( m_iCurrentWaveIndex, nCurrency );
					m_pMVMStats->RoundEvent_AcquiredCredits( m_iCurrentWaveIndex, nCurrency * fCleanMoneyPercent, false );
				}
			}
		}
	}

	// Reset the new wave
	m_iCurrentWaveIndex = waveNumber;
	pWave = GetCurrentWave();
	if ( pWave )
	{
		pWave->ForceReset();
	}
	m_pMVMStats->SetCurrentWave( m_iCurrentWaveIndex );
	if ( IsInEndlessWaves() )
	{
		EndlessRollEscalation();
	}
	UpdateObjectiveResource();

	SetCheckpoint( -1 );
	TFGameRules()->SetAllowBetweenRounds( true );
	TFGameRules()->State_Transition( GR_STATE_PREROUND );
	TFGameRules()->PlayerReadyStatus_ResetState();
	TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
	RestorePlayerCurrency();
	m_bIsWaveJumping = false;

	CTF_GameStats.ResetRoundStats();
	IGameEvent *event = gameeventmanager->CreateEvent( "mvm_reset_stats" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	ResetRespecPoints();
}

//-------------------------------------------------------------------------
// Report that a wave has been completed
void CPopulationManager::WaveEnd( bool bSuccess ) 
{
	m_pMVMStats->RoundEvent_WaveEnd( bSuccess );

	// Save off round stats before we reset them
	IGameEvent *event = gameeventmanager->CreateEvent( "scorestats_accumulated_update" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	// Treat completed waves as rounds for the purposes of TF stats
	CTF_GameStats.ResetRoundStats();

	// Completing any wave removes everyone's obligation to stay in MvM matches.  Because that's how it was when I got
	// here.
	MarkAllCurrentPlayersSafeToLeave();

	if ( bSuccess )
	{
		if ( m_bBonusRound )
		{
			if ( m_hBonusBoss )
			{
				UTIL_Remove( m_hBonusBoss );
				m_hBonusBoss = NULL;
			}
			m_bBonusRound = false;
		}
		else
		{
			m_iCurrentWaveIndex++;
		}
		m_pMVMStats->SetCurrentWave( m_iCurrentWaveIndex );

		// get Current Wave
		CWave *nextWave = GetCurrentWave();
		if ( nextWave )
		{
			// we've reached a checkpoint
			SetCheckpoint( -1 );

			// display the upcoming wave's description
			ShowNextWaveDescription();
			nextWave->StartUpgradesAlertTimer( 3.0f );

			if ( IsInEndlessWaves() )
			{
				EndlessRollEscalation();
				nextWave->ForceReset();

				// (Double time between waves on reset waves)
				float flTime = gpGlobals->curtime + tf_mvm_endless_wait_time.GetFloat();
				if ( m_iCurrentWaveIndex % tf_mvm_endless_bomb_reset.GetInt() == 0 )
				{
					flTime += tf_mvm_endless_wait_time.GetFloat();
					m_bShouldResetFlag = true;
				}

				nextWave->SetStartTime( flTime );
			}
		}

		if ( (int)m_iCurrentWaveIndex >= m_waveVector.Count() && !IsInEndlessWaves() )
		{
			// Restart the Map after a time delay
			if ( tf_mm_trusted.GetBool() == true || tf_mvm_disconnect_on_victory.GetBool() == true )
			{
				m_flMapRestartTime = gpGlobals->curtime + tf_mvm_victory_disconnect_time.GetFloat();
			}
			else
			{
				m_flMapRestartTime = gpGlobals->curtime + tf_mvm_victory_reset_time.GetFloat();
			}

			TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
			TFGameRules()->State_Transition( GR_STATE_GAME_OVER );
			return;
		}
	}

	if ( !IsInEndlessWaves() )
	{
		TFGameRules()->State_Transition( GR_STATE_BETWEEN_RNDS );
		TFObjectiveResource()->SetMannVsMachineBetweenWaves( true );
	}
}

//-------------------------------------------------------------------------
// Save the current wave as a checkpoint.
// When the scenario restarts from a loss, it will restart at the checkpoint.
void CPopulationManager::SetCheckpoint( int waveNumber )
{
	// No checkpoints for Endless
	if ( IsInEndlessWaves() )
		return;

	// Auto SetCheckPoint from ConsoleCommand
	if ( waveNumber < 0 )
	{
		waveNumber = m_iCurrentWaveIndex;
	}

	if ( waveNumber < 0 || waveNumber >= m_waveVector.Count() )
	{
		Warning( "Warning: SetCheckpoint() called with invalid wave number %d\n", waveNumber );
		return;
	}

	m_nNumConsecutiveWipes = 0;

	m_checkpointWaveIndex = waveNumber;

	DevMsg( "Checkpoint Saved\n" );

	// snapshot each player's state
	// Save off all upgrades and purge it, copy back in existing players
	for( int i=0; i<m_playerUpgrades.Count(); ++i )
	{
		// Get this players check point
		CheckpointSnapshotInfo *snapshot = FindCheckpointSnapshot( m_playerUpgrades[i]->m_steamId );
		if ( snapshot == NULL )
		{
			// New SnapshotInfo, save the player id
			snapshot = new CheckpointSnapshotInfo;
			snapshot->m_steamId = m_playerUpgrades[i]->m_steamId;
			m_checkpointSnapshot.AddToTail( snapshot );
		}

		// Save the Player upgrade history
		snapshot->m_currencySpent = m_playerUpgrades[i]->m_currencySpent;
		snapshot->m_upgradeVector.RemoveAll();
		// copy in to upgrade history
		for( int j = 0; j < m_playerUpgrades[i]->m_upgradeVector.Count(); ++j )
		{
			snapshot->m_upgradeVector.AddToTail( m_playerUpgrades[i]->m_upgradeVector[j]);
		}
	}
}

//-------------------------------------------------------------------------
void CPopulationManager::RestoreItemToCheckpointState( CTFPlayer *player, CEconItemView *item )
{
	CheckpointSnapshotInfo *snapshot = FindCheckpointSnapshot( player );
	
	if ( !snapshot )
		return;

	if ( !player->Inventory() )
		return;

	if ( !item || !item->IsValid() )
		return;

	player->BeginPurchasableUpgrades();

	// restore the item's upgrade(s)
	for( int u=0; u<snapshot->m_upgradeVector.Count(); ++u )
	{
		if ( item->GetItemDefIndex() == snapshot->m_upgradeVector[u].m_itemDefIndex )
		{
			if ( player->GetPlayerClass()->GetClassIndex() == snapshot->m_upgradeVector[u].m_iPlayerClass )
			{
				if ( g_hUpgradeEntity->ApplyUpgradeToItem( player, item, snapshot->m_upgradeVector[u].m_upgrade, snapshot->m_upgradeVector[u].m_nCost ) )
				{
					if ( tf_populator_debug.GetBool() )
					{
						const char *upgradeName = g_hUpgradeEntity->GetUpgradeAttributeName( snapshot->m_upgradeVector[u].m_upgrade );
						DevMsg( "%3.2f: CHECKPOINT_RESTORE_ITEM: Player '%s', item '%s', upgrade '%s'\n", 
								gpGlobals->curtime, 
								player->GetPlayerName(), 
								item->GetStaticData()->GetItemBaseName(),
								upgradeName ? upgradeName : "<self>" );
					}
				}
			}
		}
	}

	player->EndPurchasableUpgrades();
}

//-------------------------------------------------------------------------
void CPopulationManager::ForgetOtherBottleUpgrades ( CTFPlayer *player, CEconItemView *pItem, int upgradeToKeep )
{
	PlayerUpgradeHistory *history = FindOrAddPlayerUpgradeHistory( player );
	
	// This only applies to the current class, skip bottle upgrades for other classes
	int iClass = player->GetPlayerClass()->GetClassIndex();
	for( int i = 0; i < history->m_upgradeVector.Count(); ++i )
	{
		if ( iClass != history->m_upgradeVector[i].m_iPlayerClass )
		{
			continue;
		}

		// remove upgrades that do NOT match the target
		if ( history->m_upgradeVector[i].m_itemDefIndex == pItem->GetItemDefIndex() && history->m_upgradeVector[i].m_upgrade != upgradeToKeep )		// item upgrade
		{
			history->m_upgradeVector.FastRemove( i );
			--i;
		}
	}
}

// ----------------------------------------------------------------------------------
void CPopulationManager::RestorePlayerCurrency ()
{
	// Set the players money on round start
	int nRoundCurrency = m_pMVMStats->GetAcquiredCredits( -1 );
	nRoundCurrency += GetStartingCurrency();

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		// deduct any cash that has already been spent
		int spentCurrency = GetPlayerCurrencySpent( playerVector[i] );
		playerVector[i]->SetCurrency( nRoundCurrency - spentCurrency );
	}
}

// ----------------------------------------------------------------------------------
// Purpose : Get the player's upgrade list
// ----------------------------------------------------------------------------------
CUtlVector< CUpgradeInfo > *CPopulationManager::GetPlayerUpgradeHistory ( CTFPlayer *player )
{
	PlayerUpgradeHistory *history = FindOrAddPlayerUpgradeHistory ( player );

	if ( !history )
		return NULL;

	return &(history->m_upgradeVector);
}

// ----------------------------------------------------------------------------------
// Purpose : Get the player's currency history
// ----------------------------------------------------------------------------------
int CPopulationManager::GetPlayerCurrencySpent ( CTFPlayer *player )
{
	PlayerUpgradeHistory *history = FindOrAddPlayerUpgradeHistory ( player );
	if ( !history )
		return 0;

	return history->m_currencySpent;
}

// ----------------------------------------------------------------------------------
// Purpose : Get the player's currency history
// ----------------------------------------------------------------------------------
void CPopulationManager::AddPlayerCurrencySpent ( CTFPlayer *player, int cost )
{
	PlayerUpgradeHistory *history = FindOrAddPlayerUpgradeHistory ( player );
	if ( !history )
		return;

	history->m_currencySpent += cost;
}

// ----------------------------------------------------------------------------------
//  Purpose : Send the player their upgrades
void CPopulationManager::SendUpgradesToPlayer ( CTFPlayer *player )
{
	CUtlVector< CUpgradeInfo > *upgrades = GetPlayerUpgradeHistory ( player );
	m_pMVMStats->SendUpgradesToPlayer( player, upgrades );
}

//-----------------------------------------------------------------------------------
void CPopulationManager::RestoreCheckpoint( void )
{
	// No checkpoints for Endless

	m_isRestoringCheckpoint = true;

	if ( !IsInEndlessWaves() )
	{
		m_iCurrentWaveIndex = m_checkpointWaveIndex;
	}

	// Purge all player upgrades
	// Set them to the checkpoint state
	m_playerUpgrades.PurgeAndDeleteElements();

	// We must clear each player's upgrade history to get rid of upgrades they
	// purchased since the last checkpoint. The history will be rebuilt
	// as the checkpoint snapshot is restored.
	for( int i=0; i<m_checkpointSnapshot.Count(); ++i )
	{
		CheckpointSnapshotInfo *snapshot = m_checkpointSnapshot[i];

		// Create new Entry since we Purged the list
		PlayerUpgradeHistory *history = FindOrAddPlayerUpgradeHistory( snapshot->m_steamId );
		history->m_currencySpent = snapshot->m_currencySpent;
		
		for (int j = 0; j < snapshot->m_upgradeVector.Count(); ++j )
		{
			history->m_upgradeVector.AddToTail( snapshot->m_upgradeVector[j] );
		}
	}

	// Iterate over play
	// clear Bottles and Sentry danger and send their upgrades
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		// Clear sentry danger
		player->ResetAccumulatedSentryGunDamageDealt();
		player->ResetAccumulatedSentryGunKillCount();

		// Bottles must be purged separately, charges will be restored with other items
		CTFWearable *pWearable = player->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
		CTFPowerupBottle *pPowerupBottle = dynamic_cast< CTFPowerupBottle* >( pWearable );
		if ( pPowerupBottle )
		{
			pPowerupBottle->Reset();
		}

		SendUpgradesToPlayer( player );
	}

	m_nNumConsecutiveWipes++;

	// players are restored to their checkpoint state after they spawn
	m_pMVMStats->SetCurrentWave( m_iCurrentWaveIndex );

	UpdateObjectiveResource();

	TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Get_To_Upgrade" );

	m_nRespecsAwardedInWave = 0;
}


//-------------------------------------------------------------------------
void CPopulationManager::ClearCheckpoint( void )
{
	if ( tf_populator_debug.GetBool() )
	{
		DevMsg( "%3.2f: CHECKPOINT_CLEAR\n", gpGlobals->curtime );
	}

	m_nNumConsecutiveWipes = 0;
	m_checkpointWaveIndex = 0;
	m_checkpointSnapshot.PurgeAndDeleteElements();

	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *player = playerVector[i];

		player->ClearUpgradeHistory();
	}

}

//-------------------------------------------------------------------------
void CPopulationManager::OnPlayerKilled( CTFPlayer *corpse )
{
	for( int i=0; i<m_populatorVector.Count(); ++i )
	{
		m_populatorVector[i]->OnPlayerKilled( corpse );
	}

	CWave * pWave = GetCurrentWave();
	if ( pWave )
	{
		pWave->OnPlayerKilled( corpse );
	}
}

//-------------------------------------------------------------------------
void CPopulationManager::OnCurrencyPackFade( void )
{
}


//-------------------------------------------------------------------------
void CPopulationManager::OnCurrencyCollected( int nAmount, bool bCountAsDropped, bool bIsBonus )
{
	// Store how much money players collect between waves so we can update the checkpoint
	int iWaveNumber = GetWaveNumber();
	if ( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() )
	{
		// Decrement WaveNumber if between waves, money was for previous wave
		iWaveNumber--;
	}

	if ( bCountAsDropped ) 
	{
		m_pMVMStats->RoundEvent_CreditsDropped( iWaveNumber, nAmount );
	}
	m_pMVMStats->RoundEvent_AcquiredCredits( iWaveNumber, nAmount, bIsBonus );

	// Respec
	int nRespecLimit = tf_mvm_respec_limit.GetInt();
	if ( nRespecLimit )
	{
		bool bAtLimit = m_nRespecsAwarded >= nRespecLimit;
		if ( !bAtLimit )
		{
			m_nCurrencyCollectedForRespec += nAmount;

			// It's possible to earn multiple respecs from a large cash award
			int nCreditGoal = tf_mvm_respec_credit_goal.GetInt();
			while ( m_nCurrencyCollectedForRespec >= nCreditGoal && !bAtLimit )
			{
				++m_nRespecsAwarded;
				++m_nRespecsAwardedInWave;

				// Award each player a respec
				CUtlVector< CTFPlayer* > playerVector;
				CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
				FOR_EACH_VEC( playerVector, i )
				{
					AddRespecToPlayer( playerVector[i] );
				}

				// Allowed to earn another?
				bAtLimit = m_nRespecsAwarded >= nRespecLimit;
				if ( !bAtLimit )
				{
					m_nCurrencyCollectedForRespec -= nCreditGoal;
				}
				else
				{
					// If we're at the limit, peg this value for client UI.
					// i.e. "Respec Goal: 100 of 100"
					m_nCurrencyCollectedForRespec = nCreditGoal;
				}
			}

			// Send down to clients
			CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
			if ( pStats )
			{
				pStats->SetNumRespecsEarnedInWave( m_nRespecsAwardedInWave );
				pStats->SetAcquiredCreditsForRespec( m_nCurrencyCollectedForRespec );
			}
		}
	}
}

//-------------------------------------------------------------------------
int CPopulationManager::GetTotalPopFileCurrency( void )
{
	uint32 nTotalPopCurrency = 0;

	FOR_EACH_VEC( m_waveVector, i )
	{
		nTotalPopCurrency += m_waveVector[i]->GetTotalCurrency();
	}

	return nTotalPopCurrency;
}

//-------------------------------------------------------------------------
void CPopulationManager::AdjustMinPlayerSpawnTime( void )
{
	enum { kMvMRespawnTimeAddPerWave = 2, };

	int iWaveNum = GetWaveNumber() + 1;

	float flTime = 1.0f;
	if ( IsInEndlessWaves() )
	{
		flTime = iWaveNum / 3.0f;
	}
	else
	{
		flTime = m_bFixedRespawnWaveTime ? m_nRespawnWaveTime : 
			MIN( m_nRespawnWaveTime, float( iWaveNum * kMvMRespawnTimeAddPerWave ) );
	}
	TFGameRules()->SetTeamRespawnWaveTime( TF_TEAM_PVE_DEFENDERS, flTime );
}

//-------------------------------------------------------------------------
void CPopulationManager::MarkAllCurrentPlayersSafeToLeave()
{
	// If we have a match, mark everyone currently in the match safe to leave.
	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( !pMatch )
		{ return; }

	// Mark everyone that was meant to be in the match safe to leave now, not just those who were actually present,
	// mirroring old behavior (we are usually using this to release every current player from obligations to stay)
	int total = pMatch->GetNumTotalMatchPlayers();
	for ( int idx = 0; idx < total; idx++ )
	{
		pMatch->GetMatchDataForPlayer( idx )->MarkAlwaysSafeToLeave();
	}
}

//-------------------------------------------------------------------------
void CPopulationManager::MvMVictory()
{
	// Give "Bonus_Time Buff"
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pTFPlayer || !pTFPlayer->IsPlayer() )
			continue;

		if ( pTFPlayer->IsAlive() )
		{
			pTFPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_BONUS_TIME, 10.0 );
		}
	}

	// Set Game state
	TFGameRules()->BroadcastSound( 255, "Game.YourTeamWon" );

	m_pMVMStats->RoundOver( true );
	ClearCheckpoint();

	// Notify Players of Victory
	CBroadcastRecipientFilter filter;
	filter.MakeReliable();
	UserMessageBegin( filter, "MVMVictory" );

	bool bIsKicking = tf_mvm_disconnect_on_victory.GetBool();
	WRITE_BYTE( (uint8)bIsKicking );

	if ( bIsKicking )
	{
		WRITE_BYTE((uint8)tf_mvm_victory_disconnect_time.GetFloat());
	}
	else
	{
		WRITE_BYTE((uint8)tf_mvm_victory_reset_time.GetFloat());
	}
	MessageEnd();

	// Note that because MvM is weird, we can have multiple victories per one match, as players can keep going
	GTFGCClientSystem()->SendMvMVictoryResult();
}

//-------------------------------------------------------------------------
void CPopulationManager::GetSentryBusterDamageAndKillThreshold( int &nDamage, int &nKills ) const
{
	const int nSentryThreshold = 2;
	
	int nSentries = 0;
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
		if ( pObj->ObjectType() == OBJ_SENTRYGUN )
		{
			// Disposable sentries are not valid targets
			if ( pObj->IsDisposableBuilding() )
				continue;

			if ( pObj->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )
			{
				nSentries++;
			}
		}
	}

	// Adjust damage and kill threshold based on number of sentries in the world
	// otherwise players trivially handle the spawn rate with raw damage
	float flScale = RemapValClamped( nSentries, 1, 6, 1.f, 0.5f );
	nDamage = ( nSentries >= nSentryThreshold ) ? m_sentryBusterDamageDealtThreshold * flScale : m_sentryBusterDamageDealtThreshold;
	nKills = ( nSentries >= nSentryThreshold ) ? m_sentryBusterKillThreshold * flScale : m_sentryBusterKillThreshold;
}

//-------------------------------------------------------------------------
bool CPopulationManager::IsInEndlessWaves ( void )
{
	return (m_bEndlessOn || tf_mvm_endless_force_on.GetBool() ) && m_waveVector.Count() > 0;
}

//-------------------------------------------------------------------------
float CPopulationManager::GetHealthMultiplier ( bool bIsTank /*= false*/ )
{
	if ( !IsInEndlessWaves() || !bIsTank )
		return tf_populator_health_multiplier.GetFloat();

	// Calculate how much health the tank should get per wave
	return tf_populator_health_multiplier.GetFloat() + m_iCurrentWaveIndex * tf_mvm_endless_tank_boost.GetFloat();
}

//-------------------------------------------------------------------------
float CPopulationManager::GetDamageMultiplier ()
{
	//if ( !IsInEndlessWaves() )
		return tf_populator_damage_multiplier.GetFloat();

	// Find out how many times over t
	// Floor of the result, ie 9 / 7 returns 1, 15 / 7 returns 2;
	//int nRepeatCount = m_iCurrentWaveIndex / tf_mvm_endless_scale_rate.GetInt();	
	//return tf_populator_damage_multiplier.GetFloat() + tf_mvm_endless_damage_boost_rate.GetFloat() * nRepeatCount;
}

//-------------------------------------------------------------------------
void CPopulationManager::EndlessParseBotUpgrades ()
{
	// Don't do anything if we don't have raid mode.
	m_BotUpgradesList.RemoveAll();

	KeyValues *pKV = new KeyValues( "Upgrades" );

	if ( !pKV->LoadFromFile( filesystem, "scripts/items/mvm_botupgrades.txt", "MOD" ) )
	{
		Warning( "Can't open scripts/items/mvm_botupgrades.txt\n" );
		pKV->deleteThis();
		return;
	}

	for ( KeyValues *pData = pKV->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		const char *pszAttrib = pData->GetString( "attribute" );
		int iAttribIndex = 0;
		bool bIsBotAttr = pData->GetBool( "IsBotAttr" );
		bool bIsSkillAttr = pData->GetBool( "IsSkillAttr" );

		float	flValue = pData->GetFloat( "value" );
		float	flMax = pData->GetFloat( "max" );
		int		nCost = pData->GetFloat( "cost", 100 );
		int		nWeight = pData->GetInt( "weight", 1 );

		// Normal econ attr
		if ( !bIsBotAttr && !bIsSkillAttr )
		{
			CEconItemSchema *pSchema = ItemSystem()->GetItemSchema();
			if ( pSchema )
			{
				// If we can't find a matching attribute, continue
				const CEconItemAttributeDefinition *pAttr = pSchema->GetAttributeDefinitionByName( pszAttrib );
				if ( !pAttr )
				{
					DevMsg( "Unable to Find Attribute %s when parsing EndlessParseBotUpgrades \n", pszAttrib );
					continue;
				}

				iAttribIndex = pAttr->GetDefinitionIndex();
			}
		}

		int index = m_BotUpgradesList.AddToTail();

		for ( int i = 0; i < nWeight; ++i )
		{
			CMvMBotUpgrade *pUpgrade = &( m_BotUpgradesList[ index ] );

			// load her up
			V_strncpy( pUpgrade->szAttrib, pszAttrib, sizeof( pUpgrade->szAttrib ) );

			pUpgrade->flValue = flValue;
			pUpgrade->flMax = flMax;
			pUpgrade->nCost = nCost;
			pUpgrade->bIsBotAttr = bIsBotAttr;
			pUpgrade->bIsSkillAttr = bIsSkillAttr;
			pUpgrade->iAttribIndex = iAttribIndex;
		}
	}

	pKV->deleteThis();
}

//-------------------------------------------------------------------------
void CPopulationManager::EndlessRollEscalation( void )
{
	// Get the wave and calculate the amount of "money" the bots have
	
	// for now
	int nBotCash = ( m_iCurrentWaveIndex * tf_mvm_endless_bot_cash.GetFloat() );

	m_EndlessActiveBotUpgrades.Purge();

	// Create a list of items that can be purchased
	CUtlVector< CMvMBotUpgrade > vecAvailableUpgrades;

	FOR_EACH_VEC( m_BotUpgradesList, i )
	{
		if ( m_BotUpgradesList[i].nCost <= nBotCash )
		{
			vecAvailableUpgrades.AddToTail( m_BotUpgradesList[i] );
		}
	}

	CUniformRandomStream rRandom;
	rRandom.SetSeed( m_EndlessSeeds[ m_iCurrentWaveIndex % m_EndlessSeeds.Count() ] );

	while ( nBotCash >= 100 && vecAvailableUpgrades.Count() > 0 )
	{
		int index = rRandom.RandomInt( 0, vecAvailableUpgrades.Count() - 1);

		CMvMBotUpgrade upgrade = vecAvailableUpgrades[index];

		// Scan the existing list and append the value if it does, otherwise add new entry
		bool bUpgradeFound = false;
		FOR_EACH_VEC( m_EndlessActiveBotUpgrades, iUpgrade )
		{
			if ( !V_strcmp( m_EndlessActiveBotUpgrades[iUpgrade].szAttrib, upgrade.szAttrib) )
			{
				bUpgradeFound = true;
				// increment the value
				m_EndlessActiveBotUpgrades[iUpgrade].flValue += upgrade.flValue;
				nBotCash -= upgrade.nCost;

				// remove this upgrade if its been max
				if ( ( upgrade.flMax > 0 && upgrade.flMax <= m_EndlessActiveBotUpgrades[iUpgrade].flValue )
				  || ( upgrade.flMax < 0 && upgrade.flMax >= m_EndlessActiveBotUpgrades[iUpgrade].flValue ) )
				{
					vecAvailableUpgrades.FastRemove( index );
				}
				break;
			}
		}

		if ( !bUpgradeFound )
		{
			m_EndlessActiveBotUpgrades.AddToTail( upgrade );
			nBotCash -= upgrade.nCost;
			// remove this upgrade if its been max
			if ( ( upgrade.flMax > 0 && upgrade.flMax <= upgrade.flValue )
				|| ( upgrade.flMax < 0 && upgrade.flMax >= upgrade.flValue ) )
			{
				vecAvailableUpgrades.FastRemove( index );
			}
		}

		// Scan available upgrades and remove any that we can't cover
		if ( nBotCash > 0 )
		{
			FOR_EACH_VEC_BACK( vecAvailableUpgrades, iUpgrade )
			{
				if ( vecAvailableUpgrades[iUpgrade].nCost > nBotCash )
				{
					vecAvailableUpgrades.FastRemove( iUpgrade );
				}
			}
		}
	}

	char msg[255];
	V_strcpy_safe( msg, "***  Bot Upgrades\n" );
	FOR_EACH_VEC( m_EndlessActiveBotUpgrades, iUpgrade )
	{
		char line[255];
		V_sprintf_safe( line, "-%s %.1f\n", m_EndlessActiveBotUpgrades[iUpgrade].szAttrib, m_EndlessActiveBotUpgrades[iUpgrade].flValue );
		V_strcat_safe( msg, line );
	}
	
	UTIL_CenterPrintAll( msg );
	UTIL_ClientPrintAll( HUD_PRINTCONSOLE, msg );
}

//-------------------------------------------------------------------------
void CPopulationManager::EndlessSetAttributesForBot( CTFBot *pBot )
{
	FOR_EACH_VEC( m_EndlessActiveBotUpgrades, i )
	{
		//DevMsg( "   - %s %d ", m_EndlessActiveBotUpgrades[iUpgrade].szAttrib, m_EndlessActiveBotUpgrades[iUpgrade].flValue );
		//pBot->m_AttributeManager
		if ( m_EndlessActiveBotUpgrades[i].bIsBotAttr == true )
		{
			pBot->SetAttribute( (int)m_EndlessActiveBotUpgrades[i].flValue );
		}
		else if ( m_EndlessActiveBotUpgrades[i].bIsSkillAttr == true )
		{
			//switch ( (int)m_EndlessActiveBotUpgrades[i].flValue )
			pBot->SetDifficulty( (CTFBot::DifficultyType)(int)m_EndlessActiveBotUpgrades[i].flValue );
		}
		else
		{
			CEconItemAttributeDefinition *pDef = ItemSystem()->GetItemSchema()->GetAttributeDefinition( m_EndlessActiveBotUpgrades[i].iAttribIndex );
			if ( pDef )
			{
				int iFormat = pDef->GetDescriptionFormat();
				float flValue = m_EndlessActiveBotUpgrades[i].flValue;
				if ( iFormat == ATTDESCFORM_VALUE_IS_PERCENTAGE || iFormat == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE )
				{
					flValue += 1.0;
				}
				Assert( pBot->GetAttributeList() );
				pBot->GetAttributeList()->SetRuntimeAttributeValue( pDef, flValue );
			}
		}
	}
}

//-------------------------------------------------------------------------
bool CPopulationManager::EndlessShouldResetFlag ()
{
	return m_bShouldResetFlag;
}

//-------------------------------------------------------------------------
void CPopulationManager::EndlessFlagHasReset ()
{
	m_bShouldResetFlag = false;
}

//-------------------------------------------------------------------------
// PRIVATE
//-------------------------------------------------------------------------

// -------------------------------------------------------------------------
// Purpose : PostInit
// -------------------------------------------------------------------------
void CPopulationManager::PostInitialize( void )
{
	if ( TheNavMesh->GetNavAreaCount() <= 0 )
	{
		Warning( "Cannot populate - no Navigation Mesh exists.\n" );
		return;
	}

	FOR_EACH_VEC ( m_populatorVector, i )
	{
		m_populatorVector[i]->PostInitialize();
	}

	FOR_EACH_VEC ( m_waveVector, i )
	{
		m_waveVector[i]->PostInitialize();
	}
}

//-------------------------------------------------------------------------
// Purpose : 
//-------------------------------------------------------------------------
bool CPopulationManager::IsValidPopfile( CUtlString fullPath )
{
	const char *pszFullPath = fullPath.Get();

	// known templates that are not valid by themselves
	if ( Q_stristr( pszFullPath, "robot_standard" ) ||
		 Q_stristr( pszFullPath, "robot_giant" ) ||
		 Q_stristr( pszFullPath, "robot_gatebot" ) )
	{
		return false;
	}

	KeyValues *values = new KeyValues( "Population" );

	if ( !values->LoadFromFile( filesystem, pszFullPath, "GAME" ) )
		return false;

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}
		else if ( !Q_stricmp( name, "Wave" ) )
		{
			values->deleteThis();
			return true;
		}
	}

	values->deleteThis();
	return false;
}

//-------------------------------------------------------------------------
// Purpose : Read the target file (m_filename) and populate initial data fields
//-------------------------------------------------------------------------
bool CPopulationManager::Parse( void )
{
	if ( m_popfileFull[ 0 ] == '\0' )
	{
		Warning( "No population file specified.\n" );
		return false;
	}

	//if ( m_bIsInitialized )
//		return true;

	KeyValues *values = new KeyValues( "Population" );
	if ( !values->LoadFromFile( filesystem, m_popfileFull, "GAME" ) )
	{
		Warning( "Can't open %s.\n", m_popfileFull );
		values->deleteThis();
		return false;
	}

	// Clear out existing Data structures
	m_populatorVector.PurgeAndDeleteElements();
	m_waveVector.RemoveAll();
	m_bEndlessOn = false;

	if ( m_pTemplates )
	{
		m_pTemplates->deleteThis();
		m_pTemplates = NULL;
	}

	// find templates first
	KeyValues *pTemplates = values->FindKey( "Templates" );

	if ( pTemplates )
	{
		m_pTemplates = pTemplates->MakeCopy();
	}

	for ( KeyValues *data = values->GetFirstSubKey(); data != NULL; data = data->GetNextKey() )
	{
		const char *name = data->GetName();

		if ( Q_strlen( name ) <= 0 )
		{
			continue;
		}

		if ( !Q_stricmp( name, "StartingCurrency" ) )
		{
			m_nStartingCurrency = data->GetInt();
		}
		else if ( !Q_stricmp( name, "RespawnWaveTime" ) )
		{
			m_nRespawnWaveTime = data->GetInt();
		}
		else if ( !Q_stricmp( name, "EventPopfile" ) )
		{
			if ( !Q_stricmp( data->GetString(), "Halloween" ) )
			{
				m_nMvMEventPopfileType = MVM_EVENT_POPFILE_HALLOWEEN;
			}
			else
			{
				m_nMvMEventPopfileType = MVM_EVENT_POPFILE_NONE;
			}
		}
		else if ( !Q_stricmp( name, "FixedRespawnWaveTime" ) )
		{
			m_bFixedRespawnWaveTime = true;
		}
		else if ( !Q_stricmp( name, "AddSentryBusterWhenDamageDealtExceeds" ) )
		{
			m_sentryBusterDamageDealtThreshold = data->GetInt();
		}
		else if ( !Q_stricmp( name, "AddSentryBusterWhenKillCountExceeds" ) )
		{
			m_sentryBusterKillThreshold = data->GetInt();
		}
		else if ( !Q_stricmp( name, "CanBotsAttackWhileInSpawnRoom" ) )
		{
			if ( !Q_stricmp( data->GetString(), "no" ) || !Q_stricmp( data->GetString(), "false" ) )
			{
				m_canBotsAttackWhileInSpawnRoom = false;
			}
			else
			{
				m_canBotsAttackWhileInSpawnRoom = true;
			}
		}
		else if ( !Q_stricmp( name, "RandomPlacement" ) )
		{
			CRandomPlacementPopulator *randomPopulator = new CRandomPlacementPopulator( this );

			if ( randomPopulator->Parse( data ) == false )
			{
				Warning( "Error reading RandomPlacement definition\n" );
				return false;
			}

			m_populatorVector.AddToTail( randomPopulator );
		}
		else if ( !Q_stricmp( name, "PeriodicSpawn" ) )
		{
			CPeriodicSpawnPopulator *periodicPopulator = new CPeriodicSpawnPopulator( this );

			if ( periodicPopulator->Parse( data ) == false )
			{
				Warning( "Error reading PeriodicSpawn definition\n" );
				return false;
			}

			m_populatorVector.AddToTail( periodicPopulator );
		}
		else if ( !Q_stricmp( name, "Wave" ) )
		{
			CWave *wave = new CWave( this );

			if ( !wave->Parse( data ) )
			{
				Warning( "Error reading Wave definition\n" );
				return false;
			}


			// also keep vector of wave pointers for convenience
			m_waveVector.AddToTail( wave );
		}
		else if ( !Q_stricmp( name, "Mission" ) )
		{
			CMissionPopulator *missionPopulator = new CMissionPopulator( this );

			if ( missionPopulator->Parse( data ) == false )
			{
				Warning( "Error reading Mission definition\n" );
				return false;
			}

			m_populatorVector.AddToTail( missionPopulator );
		}
		else if ( !Q_stricmp( name, "Templates" ) )
		{
			// handled above
		}
		else if ( !Q_stricmp( name, "Advanced" ) )
		{
			m_bAdvancedPopFile = true;
		}
		else if ( !Q_stricmp( name, "IsEndless" ) )
		{
			m_bEndlessOn = true;
		}
		else
		{
			Warning( "Invalid populator '%s'\n", name );
			return false;
		}
	}

	for ( int nPopulator = 0; nPopulator < m_populatorVector.Count(); ++nPopulator )
	{
		CMissionPopulator *pMission = dynamic_cast< CMissionPopulator* >( m_populatorVector[ nPopulator ] );

		if ( pMission )
		{
			// FIXME: Need a way to handle missions that spawn multiple types
			int nStartWave = pMission->BeginAtWave();
			int nStopWave = pMission->StopAtWave();

			if ( pMission->m_spawner && !pMission->m_spawner->IsVarious() )
			{
				for ( int i = nStartWave; i < nStopWave; ++i )
				{
					if ( m_waveVector.IsValidIndex( i ) )
					{
						CWave *pWave = m_waveVector[ i ];
					
						unsigned int iFlags = MVM_CLASS_FLAG_MISSION;
						if ( pMission->m_spawner->IsMiniBoss() )
						{
							iFlags |= MVM_CLASS_FLAG_MINIBOSS;
						}
						if ( pMission->m_spawner->HasAttribute( CTFBot::ALWAYS_CRIT ) )
						{
							iFlags |= MVM_CLASS_FLAG_ALWAYSCRIT;
						}
						pWave->AddClassType( pMission->m_spawner->GetClassIcon(), 0, iFlags );
					}
				}
			}
		}
	}

	values->deleteThis();

	return true;
}


// ----------------------------------------------------------------------------------
// Purpose : Find a Checkpoint info
//-------------------------------------------------------------------------
CPopulationManager::CheckpointSnapshotInfo *CPopulationManager::FindCheckpointSnapshot( CTFPlayer *player ) const
{
	CSteamID steamId;
	if (!player->GetSteamID( &steamId ))
		return NULL;

	return FindCheckpointSnapshot( steamId );
}

// ----------------------------------------------------------------------------------
// Purpose : Find a Checkpoint info
//-------------------------------------------------------------------------
CPopulationManager::CheckpointSnapshotInfo *CPopulationManager::FindCheckpointSnapshot( CSteamID id ) const
{
	for( int i=0; i<m_checkpointSnapshot.Count(); ++i )
	{
		CheckpointSnapshotInfo *snapshot = m_checkpointSnapshot[i];

		if ( id == snapshot->m_steamId )
			return snapshot;
	}

	return NULL;
}

// ----------------------------------------------------------------------------------
// Purpose : Returns the Player's Upgrade History Struct, Adds a new entry if not present
// ----------------------------------------------------------------------------------
CPopulationManager::PlayerUpgradeHistory *CPopulationManager::FindOrAddPlayerUpgradeHistory ( CTFPlayer *player )
{
	CSteamID steamId;
	if (!player->GetSteamID( &steamId ))
	{
		Log( "MvM : Unable to Find SteamID for player %s, unable to locate their upgrade history!", player->GetPlayerName() );
		return NULL;
	}

	return FindOrAddPlayerUpgradeHistory( steamId );
}

// ----------------------------------------------------------------------------------
// Purpose : Returns the Player's Upgrade History Struct, Adds a new entry if not present
// ----------------------------------------------------------------------------------
CPopulationManager::PlayerUpgradeHistory *CPopulationManager::FindOrAddPlayerUpgradeHistory ( CSteamID steamId )
{
	FOR_EACH_VEC( m_playerUpgrades, i )
	{
		if ( steamId == m_playerUpgrades[i]->m_steamId ) 
		{
			return m_playerUpgrades[i];
		}
	}

	PlayerUpgradeHistory *history = new PlayerUpgradeHistory;

	history->m_steamId = steamId; 
	history->m_currencySpent = 0;

	m_playerUpgrades.AddToTail( history );
	return history;
}

// ----------------------------------------------------------------------------------
// Purpose : Remove upgrade tracking tied to the player and their items (ignores bottles, buybacks)
// ----------------------------------------------------------------------------------
void CPopulationManager::RemovePlayerAndItemUpgradesFromHistory( CTFPlayer *pPlayer )
{
	CSteamID steamId;
	if ( !pPlayer->GetSteamID( &steamId ) )
		return;

	// Remove player and item upgrades from snapshots
	FOR_EACH_VEC_BACK( m_checkpointSnapshot, i )
	{
		CheckpointSnapshotInfo *pSnapshot = m_checkpointSnapshot[i];
		if ( steamId != pSnapshot->m_steamId )
			continue;

		FOR_EACH_VEC_BACK( pSnapshot->m_upgradeVector, j )
		{
			int iUpgrade = pSnapshot->m_upgradeVector[j].m_upgrade;
			CMannVsMachineUpgrades *pUpgrade = &(g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ]);
			if ( pUpgrade && ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM || pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER ) )
			{
				pSnapshot->m_currencySpent -= pSnapshot->m_upgradeVector[j].m_nCost;
				pSnapshot->m_upgradeVector.Remove( j );
			}
		}
	}

	// Remove player and item upgrades from current
	FOR_EACH_VEC_BACK( m_playerUpgrades, i )
	{
		if ( steamId != m_playerUpgrades[i]->m_steamId ) 
			continue;

		FOR_EACH_VEC_BACK( m_playerUpgrades[i]->m_upgradeVector, j )
		{
			int iUpgrade = m_playerUpgrades[i]->m_upgradeVector[j].m_upgrade;
			CMannVsMachineUpgrades *pUpgrade = &(g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ]);
			if ( pUpgrade && ( pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM || pUpgrade->nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER ) )
			{
				m_playerUpgrades[i]->m_currencySpent -= m_playerUpgrades[i]->m_upgradeVector[j].m_nCost;
				m_playerUpgrades[i]->m_upgradeVector.Remove( j );
			}
		}
	}

	// Only do this step in MvM
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && m_pMVMStats )
	{
		// This should put us at the right currency, given that we've removed item and player upgrade tracking by this point
		int nTotalAcquiredCurrency = m_pMVMStats->GetAcquiredCredits( -1 ) + GetStartingCurrency();
		int nSpentCurrency = GetPlayerCurrencySpent( pPlayer );
		pPlayer->SetCurrency( nTotalAcquiredCurrency - nSpentCurrency );

		// Reset the stat that tracks upgrade purchases
		m_pMVMStats->ResetUpgradeSpending( pPlayer );
	}
};

//-----------------------------------------------------------------------------
// Purpose: This adds one per call
//-----------------------------------------------------------------------------
void CPopulationManager::AddRespecToPlayer( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;
	
	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerRespecPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerRespecPoints.InvalidIndex() )
		{
			int nCount = m_PlayerRespecPoints[iIndex];
			if ( nCount >= tf_mvm_respec_limit.GetInt() )
				return;

			m_PlayerRespecPoints[iIndex]++;
		}
		else
		{
			m_PlayerRespecPoints.Insert( steamID.ConvertToUint64(), 1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This removes one per call
//-----------------------------------------------------------------------------
void CPopulationManager::RemoveRespecFromPlayer( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Unlimited
	if ( !tf_mvm_respec_limit.GetInt() )
		return;

	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerRespecPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerRespecPoints.InvalidIndex() )
		{
			Assert( m_PlayerRespecPoints[iIndex] );

			m_PlayerRespecPoints[iIndex]--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This stomps whatever value we have
//-----------------------------------------------------------------------------
void CPopulationManager::SetNumRespecsForPlayer( CTFPlayer *pPlayer, int nCount )
{
	if ( !pPlayer )
		return;

	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerRespecPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerRespecPoints.InvalidIndex() )
		{
			m_PlayerRespecPoints[iIndex] = nCount;
		}
		else
		{
			m_PlayerRespecPoints.Insert( steamID.ConvertToUint64(), nCount );
		}
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CPopulationManager::GetNumRespecsAvailableForPlayer( CTFPlayer *pPlayer )
{
	// Unlimited
	if ( !tf_mvm_respec_limit.GetBool() )
		return 1;

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerRespecPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerRespecPoints.InvalidIndex() )
		{
			return m_PlayerRespecPoints[iIndex];
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Slam the value to nCount
//-----------------------------------------------------------------------------
void CPopulationManager::SetBuybackCreditsForPlayer( CTFPlayer *pPlayer, int nCount )
{
	if ( !pPlayer )
		return;

	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerBuybackPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerBuybackPoints.InvalidIndex() )
		{
			m_PlayerBuybackPoints[iIndex] = nCount;
		}
		else
		{
			m_PlayerBuybackPoints.Insert( steamID.ConvertToUint64(), nCount );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This removes one per call
//-----------------------------------------------------------------------------
void CPopulationManager::RemoveBuybackCreditFromPlayer( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	// Unlimited
	if ( !tf_mvm_buybacks_method.GetInt() )
		return;

	CSteamID steamID;
	if ( pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerBuybackPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerBuybackPoints.InvalidIndex() )
		{
			Assert( m_PlayerBuybackPoints[iIndex] );

			m_PlayerBuybackPoints[iIndex]--;
		}
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int CPopulationManager::GetNumBuybackCreditsForPlayer( CTFPlayer *pPlayer )
{
	if ( !tf_mvm_buybacks_method.GetBool() )
		return 1;

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		int iIndex = m_PlayerBuybackPoints.Find( steamID.ConvertToUint64() );
		if ( iIndex != m_PlayerBuybackPoints.InvalidIndex() )
		{
			return m_PlayerBuybackPoints[iIndex];
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CPopulationManager::IsPlayerBeingTrackedForBuybacks( CTFPlayer *pPlayer )
{
	int iIndex = m_PlayerBuybackPoints.InvalidIndex();

	CSteamID steamID;
	if ( pPlayer && pPlayer->GetSteamID( &steamID ) )
	{
		iIndex = m_PlayerBuybackPoints.Find( steamID.ConvertToUint64() );
	}

	return ( iIndex != m_PlayerBuybackPoints.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CPopulationManager::ResetRespecPoints( void )
{
	m_PlayerRespecPoints.RemoveAll();
	m_nRespecsAwarded = 0;
	m_nRespecsAwardedInWave = 0;
	m_nCurrencyCollectedForRespec = 0;

	// Send down to clients
	if ( tf_mvm_respec_limit.GetBool() )
	{
		CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
		if ( pStats )
		{
			pStats->SetNumRespecsEarnedInWave( m_nRespecsAwardedInWave );
			pStats->SetAcquiredCreditsForRespec( m_nCurrencyCollectedForRespec );
		}
	}
}

// ----------------------------------------------------------------------------------
// Purpose : Output useful info to console - Remove before ship
// ----------------------------------------------------------------------------------
void CPopulationManager::DebugWaveStats ()
{
	// map economy stats to spit to the console
	// remove before shipping
	if ( m_waveVector.Count() )
	{
		uint32 nTotalPopCurrency = GetTotalPopFileCurrency();
		uint32 nTotalWaves = GetTotalWaveCount();

		DevMsg( "---\n" );
		DevMsg( "Credits: %d\n", nTotalPopCurrency );
		DevMsg( "Waves: %d ( %3.2f credits per wave )\n", nTotalWaves, float( (float)nTotalPopCurrency / (float)nTotalWaves ) );
		DevMsg( "---\n" );
	}

	if ( m_EndlessActiveBotUpgrades.Count() > 0)
	{
		DevMsg( "*** Endless Bot Upgrades - %.0f Cash *** \n", m_iCurrentWaveIndex * tf_mvm_endless_bot_cash.GetFloat() );
		FOR_EACH_VEC( m_EndlessActiveBotUpgrades, iUpgrade )
		{
			DevMsg( "   - %s %.2f\n", m_EndlessActiveBotUpgrades[iUpgrade].szAttrib, m_EndlessActiveBotUpgrades[iUpgrade].flValue );
		}

		char msg[255];
		V_strcpy_safe( msg, "***  Bot Upgrades\n" );
		FOR_EACH_VEC( m_EndlessActiveBotUpgrades, iUpgrade )
		{
			char line[255];
			V_sprintf_safe( line, "-%s %.1f\n", m_EndlessActiveBotUpgrades[iUpgrade].szAttrib, m_EndlessActiveBotUpgrades[iUpgrade].flValue );
			V_strcat_safe( msg, line );
		}

		UTIL_CenterPrintAll( msg );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, msg );
	}

	DevMsg( "Popfile: %s\n", GetPopulationFilename() );
}


void CPopulationManager::AllocateBots()
{
	if ( m_bAllocatedBots )
	{
		return;
	}

	int nNumEnemyBots = 0;

	CUtlVector<CTFPlayer *> botVector;
	nNumEnemyBots = CollectMvMBots( &botVector );

	if ( botVector.Count() > 0 )
	{
		Assert( botVector.Count() == 0 );
		Warning( "%d bots were already allocated some how before CPopulationManager::AllocateBots was called\n", botVector.Count() );
	}

	for ( int i = nNumEnemyBots; i < MVM_INVADERS_TEAM_SIZE; ++i )
	{
		CTFBot* newBot = NextBotCreatePlayerBot< CTFBot >( "TFBot", false );
		if ( newBot )
		{
			newBot->ChangeTeam( TEAM_SPECTATOR, false, true );
		}
	}

	m_bAllocatedBots = true;
}

void CPopulationManager::PauseSpawning()
{
	DevMsg( "Wave paused\n" );
	m_bSpawningPaused = true;
}

void CPopulationManager::UnpauseSpawning()
{ 
	DevMsg( "Wave unpaused\n" );

	m_bSpawningPaused = false;

	// Some populators need to reset their timers or do other things when we unpause.
	// Go through and let them know we've un-paused.
	FOR_EACH_VEC( m_populatorVector, i )
	{
		m_populatorVector[i]->UnpauseSpawning();
	}
}

bool CPopulationManager::HasEventChangeAttributes( const char* pszEventName ) const
{
	for ( int i=0; i<m_waveVector.Count(); ++i )
	{
		if ( m_waveVector[i]->HasEventChangeAttributes( pszEventName ) )
		{
			return true;
		}
	}

	for ( int i=0; i<m_populatorVector.Count(); ++i )
	{
		if ( m_populatorVector[i]->HasEventChangeAttributes( pszEventName ) )
		{
			return true;
		}
	}

	return false;
}

/*static*/ int CPopulationManager::CollectMvMBots ( CUtlVector< CTFPlayer *> *pBots )
{
	pBots->RemoveAll();

	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CTFPlayer *player = ToTFPlayer( UTIL_PlayerByIndex( i ) );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsBot() )
			continue;

		if ( !player->IsConnected() )
			continue;

		if ( player->GetTeamNumber() == TF_TEAM_PVE_DEFENDERS )	// Want everything but defenders
			continue;

		pBots->AddToTail( player );
	}

	return pBots->Count();
}
