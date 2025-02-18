//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: The TF Game rules object
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#ifndef TF_GAMERULES_H
#define TF_GAMERULES_H

#ifdef _WIN32
#pragma once
#endif


#include "teamplayroundbased_gamerules.h"
#include "convar.h"
#include "gamevars_shared.h"
#include "GameEventListener.h"
#include "tf_gamestats_shared.h"
#include "tf_match_description.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "entity_soldier_statue.h"
#endif

#ifdef CLIENT_DLL
	
	#define CTFGameRules C_TFGameRules
	#define CTFGameRulesProxy C_TFGameRulesProxy
	#define CBonusRoundLogic C_BonusRoundLogic
#else
	extern CUtlString s_strNextMvMPopFile;

	extern BOOL no_cease_fire_text;
	extern BOOL cease_fire;

	class CHealthKit;
	class CTrainingModeLogic;
	class CTFHolidayEntity;
	class CTFNavArea;
	class CTFBot;
	class CTFBotRoster;
	class CMedievalLogic;
	class CCPTimerLogic;
	class CPopulationManager;
	class CCompetitiveLogic;
#endif

class CBonusRoundLogic;
class CTeamTrainWatcher;
class CPhysicsProp;
class CObjectSentrygun;
class CGhost;
class CUpgrades;

extern ConVar	tf_spec_xray;
extern ConVar	tf_avoidteammates;
extern ConVar	tf_avoidteammates_pushaway;
extern ConVar	mp_tournament_blueteamname;
extern ConVar	mp_tournament_redteamname;
extern ConVar	tf_arena_force_class;
extern ConVar	tf_arena_change_limit;
extern ConVar	tf_ctf_bonus_time;
extern ConVar	tf_mvm_respec_enabled;
extern ConVar	tf_spawn_glows_duration;

#ifdef GAME_DLL
extern ConVar mp_tournament_prevent_team_switch_on_readyup;
#endif

#ifdef TF_RAID_MODE

class CRaidLogic;
class CBossBattleLogic;

extern ConVar tf_gamemode_raid;
extern ConVar tf_gamemode_creep_wave;
extern ConVar tf_gamemode_boss_battle;

#endif // TF_RAID_MODE

class CMannVsMachineLogic;
class CMannVsMachineUpgrades;

//extern ConVar tf_populator_health_multiplier;
//extern ConVar tf_populator_damage_multiplier;

extern ConVar tf_mvm_defenders_team_size;

const int kLadder_TeamSize_6v6 = 6;
const int kLadder_TeamSize_9v9 = 9;
const int kLadder_TeamSize_12v12 = 12;

//#define TF_MVM_FCVAR_CHEAT 0 /* Cheats enabled */
#define TF_MVM_FCVAR_CHEAT FCVAR_CHEAT /* Cheats disabled */

extern bool TF_IsHolidayActive( /*EHoliday*/ int eHoliday );
#ifdef CLIENT_DLL
bool BInEndOfMatch();
#endif

//=============================================================================
// HPE_BEGIN
// [msmith] Used for the client to tell the server that we're whatching a movie or not
//			and weather or not we're ready to transition to the next map.
//=============================================================================
// Training mode cvars
extern ConVar	tf_training_client_message;
enum {
	TRAINING_CLIENT_MESSAGE_NONE = 0,
	TRAINING_CLIENT_MESSAGE_WATCHING_INTRO_MOVIE,
	TRAINING_CLIENT_MESSAGE_IN_SUMMARY_SCREEN,
	TRAINING_CLIENT_MESSAGE_NEXT_MAP,
	TRAINING_CLIENT_MESSAGE_REPLAY,
	TRAINING_CLIENT_MESSAGE_MAX,
};

// How many achievements we show in the summary screen.
#define MAX_SHOWN_ACHIEVEMENTS 6
//=============================================================================
// HPE_END
//=============================================================================


extern Vector g_TFClassViewVectors[];

#define NO_CLASS_LIMIT -1

enum {
	STOPWATCH_CAPTURE_TIME_NOT_SET = 0,
	STOPWATCH_RUNNING,
	STOPWATCH_OVERTIME,
};

class CTFGameRulesProxy : public CTeamplayRoundBasedRulesProxy, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFGameRulesProxy, CTeamplayRoundBasedRulesProxy );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();

	CTFGameRulesProxy();

	void	InputSetRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddRedTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputAddBlueTeamRespawnWaveTime( inputdata_t &inputdata );
	void	InputSetRedTeamGoalString( inputdata_t &inputdata );
	void	InputSetBlueTeamGoalString( inputdata_t &inputdata );
	void	InputSetRedTeamRole( inputdata_t &inputdata );
	void	InputSetBlueTeamRole( inputdata_t &inputdata );
	void	InputSetRequiredObserverTarget( inputdata_t &inputdata );
	void	InputAddRedTeamScore( inputdata_t &inputdata );
	void	InputAddBlueTeamScore( inputdata_t &inputdata );
	void	InputSetRedKothClockActive( inputdata_t &inputdata );
	void	InputSetBlueKothClockActive( inputdata_t &inputdata );
	void	InputSetCTFCaptureBonusTime( inputdata_t &inputdata );
	void	InputPlayVORed( inputdata_t &inputdata );
	void	InputPlayVOBlue( inputdata_t &inputdata );
	void	InputPlayVO( inputdata_t &inputdata );
	void	InputHandleMapEvent( inputdata_t &inputdata );
	void	InputSetCustomUpgradesFile( inputdata_t &inputdata );
	void	InputSetRoundRespawnFreezeEnabled( inputdata_t &inputdata );
	void	InputSetMapForcedTruceDuringBossFight( inputdata_t &inputdata );

	void	TeamPlayerCountChanged( CTFTeam *pTeam );
	void	PowerupTeamImbalance( int nTeam );
	void	StateEnterRoundRunning( void );
	void	StateEnterBetweenRounds( void );
	void	StateEnterPreRound( void );
	void	StateExitPreRound( void );
	void	MatchSummaryStart( void );
	void	TruceStart( void );
	void	TruceEnd( void );

	COutputEvent m_OnWonByTeam1;
	COutputEvent m_OnWonByTeam2;
	COutputInt m_Team1PlayersChanged;
	COutputInt m_Team2PlayersChanged;
	COutputEvent m_OnPowerupImbalanceTeam1;
	COutputEvent m_OnPowerupImbalanceTeam2;
	COutputEvent m_OnPowerupImbalanceMeasuresOver;
	COutputEvent m_OnStateEnterRoundRunning;
	COutputEvent m_OnStateEnterBetweenRounds;
	COutputEvent m_OnStateEnterPreRound;
	COutputEvent m_OnStateExitPreRound;
	COutputEvent m_OnMatchSummaryStart;
	COutputEvent m_OnTruceStart;
	COutputEvent m_OnTruceEnd;

	virtual void Activate();

private:

//=============================================================================
// HPE_BEGIN:
// [msmith]	hud type so the game type and hud type can be separate.  Used for
//			training missions.
//=============================================================================
	int		m_nHudType;
//=============================================================================
// HPE_END
//=============================================================================


	bool	m_bOvertimeAllowedForCTF;
	bool	m_bRopesHolidayLightsAllowed;
#endif

public: // IGameEventListener Interface
	virtual void FireGameEvent( IGameEvent * event );
};

class CTFRadiusDamageInfo
{
	DECLARE_CLASS_NOBASE( CTFRadiusDamageInfo );
public:
	CTFRadiusDamageInfo( CTakeDamageInfo *pInfo, const Vector &vecSrcIn, float flRadiusIn, CBaseEntity *pIgnore = NULL, float flRJRadiusIn = 0, float flForceScaleIn = 1.0f )
	{
		dmgInfo = pInfo;
		vecSrc = vecSrcIn;
		flRadius = flRadiusIn;
		pEntityIgnore = pIgnore;
		flRJRadius = flRJRadiusIn;
		flFalloff = 0;
		m_flForceScale = flForceScaleIn;
		m_pEntityTarget = NULL;

		CalculateFalloff();
	}

	void CalculateFalloff( void );
	int ApplyToEntity( CBaseEntity *pEntity );

public:
	// Fill these in & call RadiusDamage()
	CTakeDamageInfo	*dmgInfo;
	Vector			vecSrc;
	float			flRadius;
	CBaseEntity		*pEntityIgnore;
	float			flRJRadius;	// Radius to use to calculate RJ, to maintain RJs when damage/radius changes on a RL
	float			m_flForceScale;
	CBaseEntity		*m_pEntityTarget;		// Target being direct hit if any
private:
	// These are used during the application of the RadiusDamage 
	float			flFalloff;
};

struct PlayerRoundScore_t
{
	int iPlayerIndex;	// player index
	int iRoundScore;	// how many points scored this round
	int	iTotalScore;	// total points scored across all rounds
};

struct PlayerArenaRoundScore_t
{
	int iPlayerIndex;	// player index
	int	iTotalDamage;	// damage done this round
	int iTotalHealing;	// healing done this round
	int iTimeAlive;		// time alive (in seconds)
	int iKillingBlows;	// killing blows this round
	int iScore;
};

#ifdef CLIENT_DLL
const char *GetMapType( const char *mapName );
const char *GetMapDisplayName( const char *mapName, bool bTitleCase = false );
#else

class CKothLogic;

#endif

// Used to sort the players in the list by their bonus score
typedef CTFPlayer *BONUSPLAYERPTR;
class CBonusPlayerListLess
{
public:
	bool Less( const BONUSPLAYERPTR &src1, const BONUSPLAYERPTR &src2, void *pCtx )
	{
		if ( src1->m_Shared.GetItemFindBonus() > src2->m_Shared.GetItemFindBonus() )
			return true;
		return false;
	}
};

#define MAX_TEAMGOAL_STRING		256
#define MAX_TEAMNAME_STRING		6

class CTFGameRules : public CTeamplayRoundBasedRules
{
public:
	DECLARE_CLASS( CTFGameRules, CTeamplayRoundBasedRules );

	CTFGameRules();

	virtual void	LevelInitPostEntity( void );
	virtual float	GetRespawnTimeScalar( int iTeam );
	virtual float	GetRespawnWaveMaxLength( int iTeam, bool bScaleWithNumPlayers = true );

	// Damage Queries.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP:
	virtual int		Damage_GetTimeBased( void );		
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetShouldNotBleed( void );

	int				GetFarthestOwnedControlPoint( int iTeam, bool bWithSpawnpoints );
	virtual bool	TeamMayCapturePoint( int iTeam, int iPointIndex );
	virtual bool	PlayerMayCapturePoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );
	virtual bool	PlayerMayBlockPoint( CBasePlayer *pPlayer, int iPointIndex, char *pszReason = NULL, int iMaxReasonLength = 0 );

	static int		CalcPlayerScore( RoundStats_t *pRoundStats, CTFPlayer *pPlayer );
	static int		CalcPlayerSupportScore( RoundStats_t *pRoundStats, int iPlayerIdx );

	bool			IsBirthday( void ) const;
	bool			IsBirthdayOrPyroVision( void ) const;
	virtual bool	IsHolidayActive( /*EHoliday*/ int eHoliday ) const;

	virtual const unsigned char *GetEncryptionKey( void ) { return GetTFEncryptionKey(); }

	int				GetClassLimit( int iClass );
	bool			CanPlayerChooseClass( CBasePlayer *pPlayer, int iClass );

	virtual bool	ShouldBalanceTeams( void );

	virtual int		GetBonusRoundTime( bool bGameOver = false ) OVERRIDE;

	virtual bool	PointsMayBeCaptured( void ) OVERRIDE;

#ifdef GAME_DLL
public:
	virtual void	Precache( void );

	// Override this to prevent removal of game specific entities that need to persist
	virtual bool	RoundCleanupShouldIgnore( CBaseEntity *pEnt );
	virtual bool	ShouldCreateEntity( const char *pszClassName );
	virtual void	CleanUpMap( void );

	virtual void	FrameUpdatePostEntityThink();

	virtual void	RespawnPlayers( bool bForceRespawn, bool bTeam = false, int iTeam = TEAM_UNASSIGNED ) OVERRIDE;

	// Called when a new round is being initialized
	virtual void	SetupOnRoundStart( void );

	// Called when a new round is off and running
	virtual void	SetupOnRoundRunning( void );

	// Called before a new round is started (so the previous round can end)
	virtual void	PreviousRoundEnd( void );

	// Send the team scores down to the client
	virtual void	SendTeamScoresEvent( void ) { return; }

	// Send the end of round info displayed in the win panel
	virtual void	SendWinPanelInfo( bool bGameOver ) OVERRIDE;
	void			SendArenaWinPanelInfo( void );
	void			SendPVEWinPanelInfo( void );

	// Setup spawn points for the current round before it starts
	virtual void	SetupSpawnPointsForRound( void );

	// Called when a round has entered stalemate mode (timer has run out)
	virtual void	SetupOnStalemateStart( void );
	virtual void	SetupOnStalemateEnd( void );

	virtual void	RecalculateControlPointState( void );

	void			TeamPlayerCountChanged( CTFTeam *pTeam );
	void			PowerupTeamImbalance( int nTeam );
	int				GetAssignedHumanTeam( void );
	virtual void	HandleSwitchTeams( void );
	virtual void	HandleScrambleTeams( void );
	bool			CanChangeClassInStalemate( void );
	bool			CanChangeTeam( int iCurrentTeam ) const;

	virtual void	SetRoundOverlayDetails( void );	
	virtual void	ShowRoundInfoPanel( CTFPlayer *pPlayer = NULL ); // NULL pPlayer means show the panel to everyone

	virtual bool	TimerMayExpire( void );

	virtual void	Activate();

	virtual bool	AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

	void			SetTeamGoalString( int iTeam, const char *pszGoal );
//=============================================================================
// HPE_BEGIN:
// [msmith]	Added a HUD type separate from the game mode so we can do different
//			HUDs for the same mode.  This is used in training maps.
//=============================================================================
	void			SetHUDType( int nHudType );
//=============================================================================
// HPE_END
//=============================================================================

	// Speaking, vcds, voice commands.
	virtual void	InitCustomResponseRulesDicts();
	virtual void	ShutdownCustomResponseRulesDicts();

	virtual bool	HasPassedMinRespawnTime( CBasePlayer *pPlayer );
	virtual bool	ShouldRespawnQuickly( CBasePlayer *pPlayer );

	bool			ShouldScorePerRound( void );

	virtual bool	IsValveMap( void );
	virtual bool 	IsOfficialMap();

	virtual void	PlayTrainCaptureAlert( CTeamControlPoint *pPoint, bool bFinalPointInMap );

	void			SetRequiredObserverTarget( CBaseEntity *pEnt ){ m_hRequiredObserverTarget = pEnt; }
	void			SetObjectiveObserverTarget( CBaseEntity *pEnt ) { m_hObjectiveObserverTarget = pEnt; }
	EHANDLE			GetRequiredObserverTarget( void ){ return m_hRequiredObserverTarget.Get(); }
	EHANDLE			GetObjectiveObserverTarget( void ){ return m_hObjectiveObserverTarget.Get(); }

	virtual void	GetTaggedConVarList( KeyValues *pCvarTagList );

	virtual bool	PointsMayAlwaysBeBlocked(){ return ( GetGameType() == TF_GAMETYPE_ESCORT ); }

	virtual void	PlaySpecialCapSounds( int iCappingTeam, CTeamControlPoint *pPoint );

	virtual CTacticalMissionManager *TacticalMissionManagerFactory( void );

	virtual bool	ShouldSwitchTeams( void );
	virtual bool	ShouldScrambleTeams( void );

	virtual void	ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );

	bool            CanBotChangeClass( CBasePlayer* pPlayer );
	bool            CanBotChooseClass( CBasePlayer *pPlayer, int iClass );

	void SetCTFCaptureBonusTime( float flTime ){ m_flCTFCaptureBonusTime = flTime; }
	float GetCTFCaptureBonusTime( void )
	{ 
		float flRetVal = tf_ctf_bonus_time.GetFloat();
		if ( m_flCTFCaptureBonusTime >= 0.0f )
		{
			flRetVal = m_flCTFCaptureBonusTime;
		}

		return flRetVal; 
	}

	// populate vector with set of control points the player needs to capture
	virtual void CollectCapturePoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *captureVector ) const;

	// populate vector with set of control points the player needs to defend from capture
	virtual void CollectDefendPoints( CBasePlayer *player, CUtlVector< CTeamControlPoint * > *defendVector ) const;

	CObjectSentrygun *FindSentryGunWithMostKills( int team = TEAM_ANY ) const;

	// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );

	virtual bool ShouldSkipAutoScramble( void ) OVERRIDE
	{
		return IsPVEModeActive();
	}

	bool			ShouldMakeChristmasAmmoPack( void );

	void			UpdatePeriodicEvent( CTFPlayer *pPlayer, eEconPeriodicScoreEvents eEvent, uint32 nCount );

	void			HandleMapEvent( inputdata_t &inputdata );

	void			SetCustomUpgradesFile( inputdata_t &inputdata );

	virtual bool	ShouldWaitToStartRecording( void );

	void			SetGravityMultiplier( float flValue ){ m_flGravityMultiplier.Set( flValue ); }

	bool			CanFlagBeCaptured( CBaseEntity *pOther );
	bool			PowerupModeFlagStandoffActive( void );

	void			TeleportPlayersToTargetEntities( int iTeam, const char *pszEntTargetName, CUtlVector< CTFPlayer * > *pTeleportedPlayers );

	virtual void LoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList ) OVERRIDE;

	void			OnWorkshopMapUpdated( PublishedFileId_t nWorkshopID );

	void			RecalculateTruce( void );

	void			SetMapForcedTruceDuringBossFight( bool bState ){ m_bMapForcedTruceDuringBossFight = bState; }
	bool			IsMapForcedTruceDuringBossFight( void ){ return m_bMapForcedTruceDuringBossFight; }

	void			CreateSoldierStatue();

	virtual void	BroadcastSound( int iTeam, const char *sound, int iAdditionalSoundFlags = 0, CBasePlayer *pPlayer = NULL ) override;

	void			RegisterScriptFunctions() override;

	int				GetRoundState() { return (int)State_Get(); }

	bool			InMatchStartCountdown() { return BInMatchStartCountdown(); }

protected:

	virtual void LoadMapCycleFile( void ) OVERRIDE;
	void TrackWorkshopMapsInMapCycle( void );

	virtual const char* GetStalemateSong( int nTeam ) OVERRIDE;
	virtual const char* WinSongName( int nTeam ) OVERRIDE;
	virtual const char* LoseSongName( int nTeam ) OVERRIDE;

	virtual void	InitTeams( void );

	virtual void	RoundRespawn( void );
	virtual void	RespawnTeam( int iTeam );

	virtual void	InternalHandleTeamWin( int iWinningTeam );
	
	static int		PlayerRoundScoreSortFunc( const PlayerRoundScore_t *pRoundScore1, const PlayerRoundScore_t *pRoundScore2 );
	static int		PlayerArenaRoundScoreSortFunc( const PlayerArenaRoundScore_t *pRoundScore1, const PlayerArenaRoundScore_t *pRoundScore2 );

	virtual void FillOutTeamplayRoundWinEvent( IGameEvent *event );

	virtual bool CanChangelevelBecauseOfTimeLimit( void );
	virtual bool CanGoToStalemate( void );

	virtual void RestoreActiveTimer( void );

	void BroadcastDrawLine( CTFPlayer *pTFPlayer, KeyValues *pKeyValues );

#endif // GAME_DLL

public:
	// Bonus round handling
#ifdef GAME_DLL
	virtual bool	ShouldGoToBonusRound( void );
	virtual void	SetupOnBonusStart( void );
	virtual void	SetupOnBonusEnd( void );
	virtual void	BonusStateThink( void );
	void			BonusStateAbort( void );
	void			SetBonusItem( itemid_t iItemID );

	// Between rounds handling
	virtual void	BetweenRounds_Start( void );
	virtual void	BetweenRounds_End( void );
	virtual void	BetweenRounds_Think( void );
	virtual void	PreRound_Start( void ) OVERRIDE;
	virtual void	PreRound_End( void ) OVERRIDE;
#endif

public:
	// Return the value of this player towards capturing a point
	virtual int		GetCaptureValueForPlayer( CBasePlayer *pPlayer );

	// Collision and Damage rules.
	virtual bool	ShouldCollide( int collisionGroup0, int collisionGroup1 );
	
	int GetTimeLeft( void );

	// Get the view vectors for this mod.
	virtual const CViewVectors *GetViewVectors() const;

	virtual void FireGameEvent( IGameEvent *event );

	virtual const char *GetGameTypeName( void );
	virtual int GetGameType( void ){ return m_nGameType; }

	virtual void ClientSpawned( edict_t * pPlayer );

	virtual void OnFileReceived( const char * fileName, unsigned int transferID );

	virtual bool FlagsMayBeCapped( void );

	void	RunPlayerConditionThink ( void );

	const char *GetTeamGoalString( int iTeam );

	int		GetStopWatchState( void ) { return m_nStopWatchState; }
	
	// Game Modes
	virtual bool IsInArenaMode( void ) const OVERRIDE;
	virtual bool IsInKothMode( void ) const OVERRIDE { return m_bPlayingKoth; }
	bool IsInMedievalMode( void ) const { return m_bPlayingMedieval; }
	bool IsHolidayMap( int nHoliday ) const { return m_nMapHolidayType == nHoliday; }
	
#ifdef TF_RAID_MODE
	bool IsRaidMode( void ) const;
	bool IsBossBattleMode( void ) const;
#endif // TF_RAID_MODE

#ifdef TF_CREEP_MODE
bool IsCreepWaveMode( void ) const;
#endif

	bool IsMannVsMachineMode( void ) const { return m_bPlayingMannVsMachine; }

	void SetMannVsMachineAlarmStatus( bool bStatus ){ m_bMannVsMachineAlarmStatus.Set( bStatus ); }
	bool GetMannVsMachineAlarmStatus( void ){ return m_bMannVsMachineAlarmStatus; }
	
	bool IsQuickBuildTime( void );

	bool GameModeUsesUpgrades( void );
	bool GameModeUsesCurrency( void ) { return GameModeUsesUpgrades(); }
	bool GameModeUsesMiniBosses( void ) { return IsMannVsMachineMode() || IsBountyMode(); }
	bool GameModeUsesEscortPushLogic( void );

	bool IsPasstimeMode() const { return m_nGameType == TF_GAMETYPE_PASSTIME; }

	bool IsMannVsMachineRespecEnabled( void ) { return IsMannVsMachineMode() && tf_mvm_respec_enabled.GetBool(); }
	bool CanPlayerUseRespec( CTFPlayer *pTFPlayer );
	bool IsPowerupMode( void ) { return m_bPowerupMode; }
	void SetPowerupMode( bool bValue );

#ifdef GAME_DLL
	// Managed competitive matches should go through the End/StopCompetitiveMatch path
	void EndManagedMvMMatch( bool bKickPlayersToParties );
#endif

	// Competitive games
	bool IsCommunityGameMode( void ) const;
	bool IsCompetitiveMode( void ) const;			// means we're using competitive/casual matchmaking
	bool IsMatchTypeCasual( void ) const;
	bool IsMatchTypeCompetitive( void ) const;
	// Are we showing the match-start-countdown doors right now
	bool BInMatchStartCountdown() const;
#ifdef GAME_DLL
	void SyncMatchSettings();
	// ! Check return
	bool StartManagedMatch();
	void SetCompetitiveMode( bool bValue );
#endif
	void StartCompetitiveMatch( void );
	void StopCompetitiveMatch( CMsgGC_Match_Result_Status nCode );
	void EndCompetitiveMatch( void );
	void ManageCompetitiveMode( void );
	bool ReportMatchResultsToGC( CMsgGC_Match_Result_Status nCode );
	bool MatchmakingShouldUseStopwatchMode( void );
	bool IsAttackDefenseMode( void );

	ETFMatchGroup GetCurrentMatchGroup() const;
	bool IsManagedMatchEnded() const;

	bool UsePlayerReadyStatusMode( void );
	bool PlayerReadyStatus_HaveMinPlayersToEnable( void );
#ifdef GAME_DLL
	bool PlayerReadyStatus_ArePlayersOnTeamReady( int iTeam );
	bool PlayerReadyStatus_ShouldStartCountdown( void );
	void PlayerReadyStatus_ResetState( void );
	void PlayerReadyStatus_UpdatePlayerState( CTFPlayer *pTFPlayer, bool bState );
#endif // GAME_DLL

	bool IsDefaultGameMode( void );		// The absence of arena, mvm, tournament mode, etc

	// Upgrades
	int	GetCostForUpgrade( CMannVsMachineUpgrades *pUpgrade, int iItemSlot, int nClass, CTFPlayer *pPurchaser = NULL );
	bool CanUpgradeWithAttrib( CTFPlayer *pPlayer, int iWeaponSlot, attrib_definition_index_t iAttribIndex, CMannVsMachineUpgrades *pUpgrade );
	int GetUpgradeTier( int iUpgrade );
	bool IsUpgradeTierEnabled( CTFPlayer *pTFPlayer, int iItemSlot, int iUpgrade );
	bool IsPVEModeActive( void ) const;						// return true if we are playing a PvE mode
	bool IsPVEModeControlled( CBaseEntity *who ) const;		// return true for PvE opponents (ie: enemy bot team)
	const char*		GetCustomUpgradesFile() { return m_pszCustomUpgradesFile.Get(); }

//=============================================================================
// HPE_BEGIN:
// [msmith]	Training Status. And HUD type.
//=============================================================================
	bool IsInTraining( void ){ return m_bIsInTraining; }
	bool AllowTrainingAchievements() { return m_bAllowTrainingAchievements; }
	void SetAllowTrainingAchievements( bool bAllow) { m_bAllowTrainingAchievements = bAllow; }
	bool IsWaitingForTrainingContinue() { return m_bIsWaitingForTrainingContinue; }
	void SetIsWaitingForTrainingContinue( bool bWaiting ) { m_bIsWaitingForTrainingContinue = bWaiting; }
	int GetHUDType( void ){ return m_nHudType; }
//=============================================================================
// HPE_END
//=============================================================================
	bool IsTrainingHUDVisible( void ) { return IsInTraining() && m_bIsTrainingHUDVisible; }
	void SetTrainingHUDVisible( bool bVisible ) { m_bIsTrainingHUDVisible.Set( bVisible ); }

	virtual bool	IsInItemTestingMode( void ) { return m_bIsInItemTestingMode; }
	void			SetInItemTestingMode( bool bInTesting ) { m_bIsInItemTestingMode.Set( bInTesting ); }
	int				ItemTesting_GetBotAnim( void ) { return m_iItemTesting_BotAnim; }
	float			ItemTesting_GetBotAnimSpeed( void );
	bool			ItemTesting_GetBotForceFire( void ) { return m_bItemTesting_BotForceFire; }
	bool			ItemTesting_GetBotTurntable( void ) { return m_bItemTesting_BotTurntable; }
	bool			ItemTesting_GetBotViewScan( void ) { return m_bItemTesting_BotViewScan; }
	void			ItemTesting_SetupFromKV( KeyValues *pKV );
	
	bool IsPlayingHybrid_CTF_CP( void ) const { return m_bPlayingHybrid_CTF_CP; }
	bool IsPlayingSpecialDeliveryMode( void ) const { return m_bPlayingSpecialDeliveryMode; }
	bool IsPlayingRobotDestructionMode( void ) const { return m_bPlayingRobotDestructionMode; }

	virtual bool AllowThirdPersonCamera( void ) { return ( IsInMedievalMode() || ShowMatchSummary() ); }

	// Bonus rounds
	CBonusRoundLogic *GetBonusLogic( void ) { return m_hBonusLogic.Get(); }
	void		BuildBonusPlayerList( void );

	CTeamRoundTimer *GetRedKothRoundTimer( void ) { return m_hRedKothTimer.Get(); }
	CTeamRoundTimer *GetBlueKothRoundTimer( void ) { return m_hBlueKothTimer.Get(); }

	int		GetStatsMinimumPlayers( void );
	int		GetStatsMinimumPlayedTime( void );

	// BountyMode
	bool IsBountyMode( void ) { return false; }

	float GetGravityMultiplier(  void ){ return m_flGravityMultiplier; }

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );

	void SetPlayersInHell( bool bState ){ m_bHelltowerPlayersInHell.Set( bState ); } // used for Halloween 2013 state of the game (players in the underworld fighting)
	bool ArePlayersInHell( void ) const { return m_bHelltowerPlayersInHell; }
	void SpawnPlayerInHell( CTFPlayer *pPlayer, const char *pszSpawnPointName );
	
	// Halloween 2013 
	void PlayHelltowerAnnouncerVO( int iRedLine, int iBlueLine );

	void SetUsingSpells( bool bState )
	{ 
		m_bIsUsingSpells.Set( bState ); 
	}

	bool IsUsingSpells( void ) const;
	bool IsUsingGrapplingHook( void ) const;

	bool IsTruceActive( void ) const; 

	bool MapHasMatchSummaryStage( void ){ return m_bMapHasMatchSummaryStage; }
	bool PlayersAreOnMatchSummaryStage( void ){ return m_bPlayersAreOnMatchSummaryStage; }

	bool ShowMatchSummary( void ){ return m_bShowMatchSummary; }

	bool HaveStopWatchWinner( void ) { return m_bStopWatchWinner; }

	int GetGameTeamForGCTeam( TF_GC_TEAM nGCTeam );
	TF_GC_TEAM GetGCTeamForGameTeam( int nGameTeam );

	enum ENextMapVotingState
	{
		NEXT_MAP_VOTE_STATE_NONE,
		NEXT_MAP_VOTE_STATE_WAITING_FOR_USERS_TO_VOTE,
		NEXT_MAP_VOTE_STATE_MAP_CHOSEN_PAUSE,
	};

	enum EUserNextMapVote
	{
		USER_NEXT_MAP_VOTE_MAP_0 = 0,
		USER_NEXT_MAP_VOTE_MAP_1,
		USER_NEXT_MAP_VOTE_MAP_2,
		USER_NEXT_MAP_VOTE_UNDECIDED,

		NUM_VOTE_STATES
	};

	EUserNextMapVote GetWinningVote( int (&nVotes)[ EUserNextMapVote::NUM_VOTE_STATES ] ) const;
	EUserNextMapVote PlayerNextMapVoteState( int nIndex ) const { return m_ePlayerWantsRematch.Get( nIndex ); }
	ENextMapVotingState GetCurrentNextMapVotingState() const { return m_eRematchState; }
	MapDefIndex_t GetNextMapVoteOption( int nIndex ) const { return m_nNextMapVoteOptions.Get( nIndex ); }
	
#ifdef GAME_DLL
	void KickPlayersNewMatchIDRequestFailed();

	void CheckAndSetPartyLeader( CTFPlayer *pTFPlayer, int iTeam );
#endif // GAME_DLL


#ifdef GAME_DLL
	void RequestClientInventory( CSteamID steamID );
#endif

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes data tables able to access our private vars.

	virtual ~CTFGameRules();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	HandleOvertimeBegin();

	bool			ShouldShowTeamGoal( void );

	const char *GetVideoFileForMap( bool bWithExtension = true );

	const char *FormatVideoName( const char *videoName, bool bWithExtension = true );

	void			SetUpVisionFilterKeyValues( void );

	bool			UseSillyGibs( void );

	virtual bool	AllowMapParticleEffect( const char *pszParticleEffect );

	virtual bool	AllowWeatherParticles( void );

	virtual bool AllowMapVisionFilterShaders( void );
	virtual const char* TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName );

	virtual void	ModifySentChat( char *pBuf, int iBufSize );

	virtual void	GetTeamGlowColor( int nTeam, float &r, float &g, float &b );

	virtual bool ShouldConfirmOnDisconnect();

	bool ShouldShowPreRoundDoors() const;
	bool RecievedBaseline() const { return m_bRecievedBaseline; }

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes data tables able to access our private vars.
	
	virtual ~CTFGameRules();

	virtual void LevelShutdown();
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void Think();

	void PeriodicHalloweenUpdate();

	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

	bool CheckWinLimit( bool bAllowEnd = true, int nAddValueWhenChecking = 0 ) OVERRIDE;
	bool SetCtfWinningTeam();
	bool SetPasstimeWinningTeam();
	bool CheckCapsPerRound();
	virtual void CheckRespawnWaves();
	virtual void PlayWinSong( int team ) OVERRIDE;

//=============================================================================
// HPE_BEGIN:
// [msmith]	Used in training to load the next training map in sequence.
//=============================================================================
	void LoadNextTrainingMap();
//=============================================================================
// HPE_END
//=============================================================================

	virtual void SetWinningTeam( int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false ) OVERRIDE;
	virtual void SetStalemate( int iReason, bool bForceMapReset = true, bool bSwitchTeams = false );

	void CheckTauntAchievement( CTFPlayer *pAchiever, int nGibs, int *pTauntCamAchievements );

	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );

	// Spawing rules.
	CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer, bool bIgnorePlayers, PlayerTeamSpawnMode_t nSpawndMode = PlayerTeamSpawnMode_Normal );

	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );

	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	void ClientSettingsChanged( CBasePlayer *pPlayer );
	void ChangePlayerName( CTFPlayer *pPlayer, const char *pszNewName );

	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem ); 

	float GetPreMatchEndTime() const;	// Returns the time at which the prematch will be over.
	void GoToIntermission( void );

	virtual int GetAutoAimMode()	{ return AUTOAIM_NONE; }
	void SetSetup( bool bSetup );
	void ManageStopwatchTimer( bool bInSetup );
	virtual void HandleTeamScoreModify( int iTeam, int iScore);

	bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex );

	virtual const char *GetGameDescription( void ){ return "Team Fortress"; }

	virtual void Status( void (*print) (PRINTF_FORMAT_STRING const char *fmt, ...) );

	// Sets up g_pPlayerResource.
	virtual void CreateStandardEntities();

	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void PlayerKilledCheckAchievements( CTFPlayer *pAttacker, CTFPlayer *pVictim );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info, const char* eventName );
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );

	void CalcDominationAndRevenge( CTFPlayer *pAttacker, CBaseEntity *pWeapon, CTFPlayer *pVictim, bool bIsAssist, int *piDeathFlags );

	const char *GetKillingWeaponName( const CTakeDamageInfo &info, CTFPlayer *pVictim, int *iWeaponID );
	CBasePlayer *GetAssister( CBasePlayer *pVictim, CBasePlayer *pScorer, CBaseEntity *pInflictor );
	CTFPlayer *GetRecentDamager( CTFPlayer *pVictim, int iDamager, float flMaxElapsed );

	virtual void ClientDisconnected( edict_t *pClient );

	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	void	RadiusDamage( CTFRadiusDamageInfo &info );

	bool ApplyOnDamageModifyRules( CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, bool bAllowDamage );

	struct DamageModifyExtras_t
	{
		bool bIgniting;
		bool bSelfBlastDmg;
		bool bSendPreFeignDamage;
		bool bPlayDamageReductionSound;
	};
	float ApplyOnDamageAliveModifyRules( const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity, DamageModifyExtras_t& outParams );

	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );

	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }

	virtual bool UseSuicidePenalty() { return false; }

	int		GetPreviousRoundWinners( void ) { return m_iPreviousRoundWinners; }

	void	SendHudNotification( IRecipientFilter &filter, HudNotification_t iType, bool bForceShow = false  );
	void	SendHudNotification( IRecipientFilter &filter, const char *pszText, const char *pszIcon, int iTeam = TEAM_UNASSIGNED );
	void	StopWatchModeThink( void );

	virtual		void RestartTournament( void );

	bool	TFVoiceManager( CBasePlayer *pListener, CBasePlayer *pTalker );

	void	OnNavMeshLoad( void );
	void	OnDispenserBuilt( CBaseEntity *dispenser );
	void	OnDispenserDestroyed( CBaseEntity *dispenser );
	void	OnPlayerSpawned( CTFPlayer *pPlayer );
	void	OnCoachJoining( uint32 unCoachAccountID, uint32 unStudentAccountID );
	void 	OnRemoveCoach( uint32 unCoachAccountID );

	//Arena
	void		AddPlayerToQueue( CTFPlayer *pPlayer );
	void		AddPlayerToQueueHead( CTFPlayer *pPlayer );
	void		RemovePlayerFromQueue( CTFPlayer *pPlayer );
	virtual bool BHavePlayers( void ) OVERRIDE;

	void		Arena_RunTeamLogic( void );
	void		Arena_ResetLosersScore( bool bResetAll );
	void		Arena_PrepareNewPlayerQueue( bool bResetAll );
	int			Arena_PlayersNeededForMatch( void );
	void		Arena_CleanupPlayerQueue( void );
	void		Arena_ClientDisconnect( const char *playername );
	void		Arena_SendPlayerNotifications( void );
	void		Arena_NotifyTeamSizeChange( void );
	float		GetRoundStart( void ) { return m_flRoundStartTime; }

	// Voting
	void		ManageServerSideVoteCreation( void );

#ifdef TF_RAID_MODE
	// Raid game mode
	CRaidLogic	*GetRaidLogic( void ) const		{ return m_hRaidLogic.Get(); }
#endif // TF_RAID_MODE

	// Currency awarding
	int		CalculateCurrencyAmount_CustomPack( int nAmount );											// If we should drop a custom currency pack, and how much money to put in - 0 means don't drop
	int		CalculateCurrencyAmount_ByType( CurrencyRewards_t nType );									// How much to give players for specific items and events, i.e. cash collection bonus, small packs
	int		DistributeCurrencyAmount( int nAmount, CTFPlayer *pTFPlayer = NULL, bool bShared = true, bool bCountAsDropped = false, bool bIsBonus = false );	// Distributes nAmount to a specific player or team

	virtual bool StopWatchShouldBeTimedWin( void ) OVERRIDE;

public:
	void SetPlayerNextMapVote( int nIndex, EUserNextMapVote eState ) { m_ePlayerWantsRematch.Set( nIndex, eState ); }

	CTrainingModeLogic *GetTrainingModeLogic() { return m_hTrainingModeLogic; }
	CTFHolidayEntity *GetHolidayLogic() const { return m_hHolidayLogic; }

	void	HandleCTFCaptureBonus( int nTeam );
	bool	TournamentModeCanEndWithTimelimit( void ){ return ( GetStopWatchTimer() == NULL ); }

	CTeamRoundTimer *GetKothTeamTimer( int iTeam )
	{
		if ( IsInKothMode() == false )
			return NULL;

		if ( iTeam == TF_TEAM_RED )
		{
			return m_hRedKothTimer.Get();
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			return m_hBlueKothTimer.Get();
		}

		return NULL;
	}

	void SetKothTeamTimer( int iTeam, CTeamRoundTimer *pTimer )
	{
		if ( iTeam == TF_TEAM_RED )
		{
			m_hRedKothTimer.Set( pTimer );
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			m_hBlueKothTimer.Set( pTimer );
		}
	}

	void SetOvertimeAllowedForCTF( bool bAllowed ){ m_bOvertimeAllowedForCTF = bAllowed; }
	bool GetOvertimeAllowedForCTF( void ){ return m_bOvertimeAllowedForCTF; }

	void SetRopesHolidayLightsAllowed( bool bAllowed ) { m_bRopesHolidayLightsAllowed = bAllowed; }

	const CUtlVector< CHandle< CBaseEntity > > &GetHealthEntityVector( void );		// return vector of health entities 
	const CUtlVector< CHandle< CBaseEntity > > &GetAmmoEntityVector( void );		// return vector of ammo entities 

	CHandle< CTeamTrainWatcher > GetPayloadToPush( int pushingTeam ) const;			// return the train watcher for the Payload cart the given team needs to push to win, or NULL if none currently exists
	CHandle< CTeamTrainWatcher > GetPayloadToBlock( int blockingTeam ) const;		// return the train watcher for the Payload cart the given team needs to block from advancing, or NULL if none currently exists

	virtual void ProcessVerboseLogOutput( void );

	void PushAllPlayersAway( const Vector& vFromThisPoint, float flRange, float flForce, int nTeam, CUtlVector< CTFPlayer* > *pPushedPlayers = NULL );

	bool ShouldDropSpellPickup();
	void DropSpellPickup( const Vector& vPosition, int nTier = 0 ) const;
	
	bool ShouldDropBonusDuck( void );
	bool ShouldDropBonusDuckFromPlayer( CTFPlayer *pScorer, CTFPlayer *pVictim );
	void DropBonusDuck( const Vector& vPosition, CTFPlayer *pScorer = NULL, CTFPlayer *pAssistor = NULL, CTFPlayer *pVictim = NULL, bool bCrit = false, bool bObjective = false ) const;

	void DropHalloweenSoulPackToTeam( int nAmount, const Vector& vecPosition, int nTeamNumber, int nSourceTeam );
	void DropHalloweenSoulPack( int nAmount, const Vector& vecSource, CBaseEntity *pTarget, int nSourceTeam );

	void MatchSummaryStart( void );
	void MatchSummaryEnd( void );

	int GetTeamAssignmentOverride( CTFPlayer *pTFPlayer, int iDesiredTeam, bool bAutoBalance = false );
private:

	void ChooseNextMapVoteOptions();

	int DefaultFOV( void ) { return 75; }
	int GetDuckSkinForClass( int nTeam, int nClass ) const;
	void MatchSummaryTeleport();

	void StopWatchShouldBeTimedWin_Calculate( void );
	
	void PowerupTeamImbalance_PlayerChangeTeam( CTFPlayer *pTFPlayer, int nTeam );
	void PowerupTeamImbalance_SwapPlayers( int nLosingTeam );
	
#endif // GAME_DLL

	bool GetRopesHolidayLightsAllowed( void ) { return m_bRopesHolidayLightsAllowed; }

private:

	void ComputeHealthAndAmmoVectors( void );		// compute internal vectors of health and ammo locations
	bool m_areHealthAndAmmoVectorsReady;


#ifdef GAME_DLL

	
	void CheckHelltowerCartAchievement( int iTeam );

	Vector2D	m_vecPlayerPositions[MAX_PLAYERS_ARRAY_SAFE];

	CUtlVector<CHandle<CHealthKit> > m_hDisabledHealthKits;	


	char	m_szMostRecentCappers[MAX_PLAYERS_ARRAY_SAFE];	// list of players who made most recent capture.  Stored as string so it can be passed in events.
	int		m_iNumCaps[TF_TEAM_COUNT];				// # of captures ever by each team during a round

	int SetCurrentRoundStateBitString();
	void SetMiniRoundBitMask( int iMask );
	int m_iPrevRoundState;	// bit string representing the state of the points at the start of the previous miniround
	int m_iCurrentRoundState;
	int m_iCurrentMiniRoundMask;

	CHandle<CTeamRoundTimer>	m_hStopWatchTimer;
	

	CTeamRoundTimer* GetStopWatchTimer( void ) { return (CTeamRoundTimer*)m_hStopWatchTimer.Get(); }

	EHANDLE m_hRequiredObserverTarget;
	EHANDLE m_hObjectiveObserverTarget;

	CHandle<CTFGameRulesProxy> m_hGamerulesProxy;

	//Arena
	bool IsFirstBloodAllowed( void );
	EHANDLE m_hArenaEntity;
	CUtlVector<CHandle<CTFPlayer> > m_hArenaPlayerQueue;	
	int		m_iPreviousTeamSize;
	bool	m_bArenaFirstBlood;

	float	m_flSendNotificationTime;

	// Tournament
	CHandle< CCompetitiveLogic > m_hCompetitiveLogicEntity;
	
	CHandle<CTrainingModeLogic> m_hTrainingModeLogic;
	CHandle<CTFHolidayEntity> m_hHolidayLogic;

	bool	m_bOvertimeAllowedForCTF;

	// for bot rosters
	CHandle<CTFBotRoster> m_hBlueBotRoster;
	CHandle<CTFBotRoster> m_hRedBotRoster;

	// coaching
	typedef CUtlMap<uint32, uint32> tCoachToStudentMap;
	tCoachToStudentMap m_mapCoachToStudentMap;

	// Automatic vote called near the end of a map
	bool	m_bVoteCalled;
	bool	m_bServerVoteOnReset;
	float	m_flVoteCheckThrottle;

	CUtlVector< CHandle< CCPTimerLogic > > m_CPTimerEnts;
	float	m_flCapInProgressBuffer;

	float	m_flMatchSummaryTeleportTime;

#ifdef TF_RAID_MODE
	CHandle< CRaidLogic >		m_hRaidLogic;
	CHandle< CBossBattleLogic > m_hBossBattleLogic;
#endif // TF_RAID_MODE
	
	int		m_nCurrencyAccumulator;
	int		m_iCurrencyPool;

	float	m_flCheckPlayersConnectingTime;

	CountdownTimer m_helltowerTimer;		// used for Halloween 2013 Announcer VO in plr_hightower_event
	CountdownTimer m_doomsdaySetupTimer;	// used for Halloween 2014 Announcer Setup VO in sd_doomsday_event
	CountdownTimer m_doomsdayTicketsTimer;	// Used on sd_doomsday_event to nag players about picking up the tickets

	float m_flNextStrangeEventProcessTime;

	bool m_bMapForcedTruceDuringBossFight;
	float m_flNextHalloweenGiftUpdateTime;
#else

	KeyValues *m_pkvVisionFilterTranslations;
	KeyValues *m_pkvVisionFilterShadersMapWhitelist;

	bool	m_bSillyGibs;

#endif

	CNetworkVar( ETFGameType, m_nGameType ); // Type of game this map is (CTF, CP)
	CNetworkVar( int, m_nStopWatchState );
	CNetworkString( m_pszTeamGoalStringRed, MAX_TEAMGOAL_STRING );
	CNetworkString( m_pszTeamGoalStringBlue, MAX_TEAMGOAL_STRING );
	CNetworkVar( float, m_flCapturePointEnableTime );
	CNetworkVar( int, m_iGlobalAttributeCacheVersion );

//=============================================================================
// HPE_BEGIN:
// [msmith]	Training and HUD status.
//=============================================================================
	CNetworkVar( int, m_nHudType ); // Used by map authors to override the default HUD clients are showing
	CNetworkVar( bool, m_bIsInTraining );
	CNetworkVar( bool, m_bAllowTrainingAchievements );
	CNetworkVar( bool, m_bIsWaitingForTrainingContinue );
//=============================================================================
// HPE_END
//=============================================================================
	CNetworkVar( bool, m_bIsTrainingHUDVisible );

	CNetworkVar( bool, m_bIsInItemTestingMode );
	int		m_iItemTesting_BotAnim;
	float	m_flItemTesting_BotAnimSpeed;
	bool	m_bItemTesting_BotForceFire;
	bool	m_bItemTesting_BotTurntable;
	bool	m_bItemTesting_BotViewScan;

	CNetworkHandle( CBonusRoundLogic, m_hBonusLogic );

	CNetworkVar( bool, m_bPlayingKoth );
	CNetworkVar( bool, m_bPowerupMode );
	CNetworkVar( bool, m_bPlayingRobotDestructionMode );
	CNetworkVar( bool, m_bPlayingMedieval );
	CNetworkVar( bool, m_bPlayingHybrid_CTF_CP );
	CNetworkVar( bool, m_bPlayingSpecialDeliveryMode );

	CNetworkVar( bool, m_bPlayingMannVsMachine );
	CNetworkVar( bool, m_bMannVsMachineAlarmStatus );
	CNetworkVar( bool, m_bHaveMinPlayersToEnableReady );

	CNetworkVar( bool, m_bBountyModeEnabled );
	CNetworkVar( bool, m_bCompetitiveMode );
	CNetworkVar( float, m_flGravityMultiplier );
	CNetworkVar( int, m_nMatchGroupType );
	CNetworkVar( bool, m_bMatchEnded );

	// This is used to check if players are in hell. The name doesn't make sense because we thought this would only be used for Halloween 2013
	// cannot change the name because it's network var which will break demo
	CNetworkVar( bool, 	m_bHelltowerPlayersInHell );

	CNetworkVar( bool, m_bIsUsingSpells );

	CNetworkVar( bool, m_bTruceActive );
	CNetworkVar( bool, m_bTeamsSwitched );

	CNetworkVar( bool, m_bRopesHolidayLightsAllowed );

#ifdef GAME_DLL
	float	m_flNextFlagAlarm;
	float	m_flNextFlagAlert;

	float	m_flSafeToLeaveTimer;

	CBaseEntity *m_pUpgrades;
#endif

	CNetworkHandle( CTeamRoundTimer, m_hRedKothTimer );
	CNetworkHandle( CTeamRoundTimer, m_hBlueKothTimer );

	CNetworkVar( int, m_nMapHolidayType ); // Used by map authors to indicate this is a holiday map

	CNetworkString( m_pszCustomUpgradesFile, MAX_PATH );

	CNetworkVar( bool, m_bShowMatchSummary );
	CNetworkVar( bool, m_bMapHasMatchSummaryStage );
	CNetworkVar( bool, m_bPlayersAreOnMatchSummaryStage );
	CNetworkVar( bool, m_bStopWatchWinner );
	// This is called m_ePlayerWantsRematch because we initially had rematches, but now we
	// let players vote on the next map instead.  Can't rename this variable, so we're just
	// going to use with the wrong name
	CNetworkArray( EUserNextMapVote, m_ePlayerWantsRematch, MAX_PLAYERS_ARRAY_SAFE );
	CNetworkVar( ENextMapVotingState, m_eRematchState );
	CNetworkArray( MapDefIndex_t, m_nNextMapVoteOptions, 3 );

	float		m_flCTFCaptureBonusTime;
public:

	bool m_bControlSpawnsPerTeam[ MAX_TEAMS ][ MAX_CONTROL_POINTS ];
	int	 m_iPreviousRoundWinners;

	float	GetCapturePointTime( void ) { return m_flCapturePointEnableTime; }

	virtual bool ShouldDrawHeadLabels() override;

	enum HalloweenScenarioType
	{
		HALLOWEEN_SCENARIO_NONE = 0,
		HALLOWEEN_SCENARIO_MANN_MANOR,
		HALLOWEEN_SCENARIO_VIADUCT,
		HALLOWEEN_SCENARIO_LAKESIDE,
		HALLOWEEN_SCENARIO_HIGHTOWER,
		HALLOWEEN_SCENARIO_DOOMSDAY,
	};
	HalloweenScenarioType GetHalloweenScenario( void ) const;
	bool IsHalloweenScenario( HalloweenScenarioType scenario ) const;

	bool CanInitiateDuels( void );

#ifdef GAME_DLL

	// Used on sd_doomsday_event to nag players about picking up the tickets
	void StartDoomsdayTicketsTimer( void ) { m_doomsdayTicketsTimer.Start( RandomInt( 30, 60 ) ); }
	void StopDoomsdayTicketsTimer( void ) { m_doomsdayTicketsTimer.Invalidate(); }
	bool DoomsdayTicketTimerElapsed( void ) const { return m_doomsdayTicketsTimer.HasStarted() && m_doomsdayTicketsTimer.IsElapsed(); }

	int GetBossCount() const { return m_activeBosses.Count(); }

	CBaseCombatCharacter *GetActiveBoss( int iBoss = 0 )
	{
		if ( iBoss < 0 || iBoss >= m_activeBosses.Count() )
			return NULL;

		return m_activeBosses[iBoss];
	}

	void AddActiveBoss( CBaseCombatCharacter *boss )
	{
		// don't add the same boss
		if ( m_activeBosses.Find( boss ) != m_activeBosses.InvalidIndex() )
			return;

		m_activeBosses.AddToTail( boss );
	}

	void RemoveActiveBoss( CBaseCombatCharacter *boss )
	{
		m_activeBosses.FindAndRemove( boss );
	}

	CBaseEntity *GetIT( void ) const			// who is the boss chasing
	{
		return m_itHandle;
	}

	void SetIT( CBaseEntity *who );
	void SetBirthdayPlayer( CBaseEntity *pEntity );

	void SetHalloweenEffectStatus( int effect, float duration )		// Update the current Halloween effect on the HUD
	{
		m_nHalloweenEffect = effect;
		m_fHalloweenEffectStartTime = gpGlobals->curtime;
		m_fHalloweenEffectDuration = duration;
	}


	// remove all projectiles in the world
	void RemoveAllProjectiles();

	// remove all buildings in the world
	void RemoveAllBuildings( bool bExplodeBuildings = false );

	// remove all sentry's ammo
	void RemoveAllSentriesAmmo();

	// remove all projectiles and objects
	void RemoveAllProjectilesAndBuildings( bool bExplodeBuildings = false );

#endif // GAME_DLL

	void ClearHalloweenEffectStatus( void )							// Clear the current Halloween effect and hide the HUD display
	{
		m_nHalloweenEffect = -1;
		m_fHalloweenEffectStartTime = -1.0f;
		m_fHalloweenEffectDuration = -1.0f;
	}

	bool IsIT( CBaseEntity *who ) const
	{
		return ( who && who == m_itHandle.Get() );
	}

	CBaseEntity *GetBirthdayPlayer( void ) const
	{
		return m_hBirthdayPlayer.Get();
	}

	bool IsHalloweenEffectStatusActive( void ) const
	{
		return m_nHalloweenEffect >= 0;
	}

	int GetHalloweenEffectStatus( void ) const
	{
		return m_nHalloweenEffect;
	}

	float GetHalloweenEffectTimeLeft( void ) const
	{
		float expireTime = m_fHalloweenEffectStartTime + m_fHalloweenEffectDuration;

		return expireTime - gpGlobals->curtime;
	}

	float GetHalloweenEffectDuration( void ) const
	{
		return m_fHalloweenEffectDuration;
	}

	int GetGlobalAttributeCacheVersion( void ) const
	{
		return m_iGlobalAttributeCacheVersion;
	}

	void FlushAllAttributeCaches( void )
	{
		m_iGlobalAttributeCacheVersion++;
	}

private:	
#ifdef CLIENT_DLL
	bool m_bRecievedBaseline;
#endif


	CountdownTimer m_botCountTimer;

	CUtlVector< CHandle< CBaseEntity > > m_ammoVector;			// vector of active ammo entities
	bool m_isAmmoVectorReady;									// for lazy evaluation

	CUtlVector< CHandle< CBaseEntity > > m_healthVector;		// vector of active health entities
	bool m_isHealthVectorReady;									// for lazy evaluation

	bool m_bUseMatchHUD;
	bool m_bUsePreRoundDoors;
#ifdef GAME_DLL
	mutable CHandle< CTeamTrainWatcher > m_redPayloadToPush;
	mutable CHandle< CTeamTrainWatcher > m_bluePayloadToPush;
	mutable CHandle< CTeamTrainWatcher > m_redPayloadToBlock;
	mutable CHandle< CTeamTrainWatcher > m_bluePayloadToBlock;

	bool m_hasSpawnedToy;
	void SpawnHalloweenBoss( void );
	CountdownTimer m_halloweenBossTimer;
	CUtlVector< CHandle< CBaseCombatCharacter > > m_activeBosses;
	bool m_bHasSpawnedSoccerBall[TF_TEAM_COUNT];

	CountdownTimer m_ghostTimer;

	void SpawnZombieMob( void );
	CountdownTimer m_zombieMobTimer;
	int m_zombiesLeftToSpawn;
	Vector m_zombieSpawnSpot;

public:
	void BeginHaunting( int nDesiredCount, float flMinDuration, float flMaxDuration );

	void StartHalloweenBossTimer( float flTime, float flVariation = 0.f )
	{
		m_halloweenBossTimer.Start( RandomFloat( flTime - flVariation, flTime + flVariation ) );
	}

	// Recent player stuff
	void PlayerHistory_AddPlayer( CTFPlayer *pTFPlayer );
	PlayerHistoryInfo_t *PlayerHistory_GetPlayerInfo( CTFPlayer *pTFPlayer );
	int PlayerHistory_GetTimeSinceLastSeen( CTFPlayer *pTFPlayer );

	CUtlVector< Vector > *GetHalloweenSpawnLocations() { return &m_halloweenGiftSpawnLocations; }

	bool BAttemptMapVoteRollingMatch();
	bool BIsManagedMatchEndImminent( void );

	float CheckPowerupModeDominantDisconnect( CSteamID steamID );
	void PowerupModeDominantDisconnect( CSteamID steamID, float flRemoveDominantConditionTime );

	void ForceEnableUpgrades( int nState ) { m_nForceUpgrades = nState; }
	void ForceEscortPushLogic( int nState ) { m_nForceEscortPushLogic = nState; }

private:
	CUtlVector< CHandle< CGhost > > m_ghostVector;
	CUtlVector< PlayerHistoryInfo_t > m_vecPlayerHistory;

	struct TeleportLocation_t
	{
		Vector m_vecPosition;
		QAngle m_qAngles;
	};
	CUtlMap< string_t, CUtlVector< TeleportLocation_t >* > m_mapTeleportLocations;

	// Keep track of kills made with powerups
	int		m_nPowerupKillsRedTeam;
	int		m_nPowerupKillsBlueTeam;
	float	m_flTimeToRunImbalanceMeasures;
	float	m_flTimeToStopImbalanceMeasures;
	bool	m_bPowerupImbalanceMeasuresRunning;
	int		m_nLastPowerUpImbalanceTeam = TEAM_UNASSIGNED;
	float	m_flLastPowerUpImbalanceTime = -1.f;
	int		m_nPowerUpImbalanceVictimTeam = TEAM_UNASSIGNED;
	float	m_flPowerUpImbalanceVictimTeamTime = -1.f;

	// Every so often we analyze player kills to determine if any players are dominant
	float	m_flNextPowerupModeKillCountTimer = -1.f;

	void	PowerupModeInitKillCountTimer( void );
	void	PowerupModeKillCountCompare( void );

	struct PowerupModeDominantDisconnect_t
	{
		CSteamID m_steamID;
		float m_flRemoveDominantConditionTime = -1.f;
	};
	CUtlVector< PowerupModeDominantDisconnect_t > m_PowerupModeDominantDisconnect;

	bool	m_bMapCycleNeedsUpdate;

	CUtlVector< Vector > m_halloweenGiftSpawnLocations;		// vector of valid gift spawn locations from the map
	float	m_flCompModeRespawnPlayersAtMatchStart;

	CHandle< CEntitySoldierStatue > m_hSoldierStatue = nullptr;

#endif // GAME_DLL

	// LEGACY BOSS CODE. Keeping this to not break demo
	CNetworkVar( int, m_nBossHealth );
	CNetworkVar( int, m_nMaxBossHealth );
	CNetworkVar( float, m_fBossNormalizedTravelDistance );

	CNetworkHandle( CBaseEntity, m_itHandle );	// entindex of current IT entity (0 = no it)
	CNetworkHandle( CBaseEntity, m_hBirthdayPlayer );	// entindex of current birthday player (0 = none)

	CNetworkVar( int, m_nHalloweenEffect );
	CNetworkVar( float, m_fHalloweenEffectStartTime );
	CNetworkVar( float, m_fHalloweenEffectDuration );
	CNetworkVar( HalloweenScenarioType, m_halloweenScenario );

	CNetworkVar( int, m_nForceUpgrades );
	CNetworkVar( int, m_nForceEscortPushLogic );

// MvM Helpers
#ifdef GAME_DLL
public:
	void SetNextMvMPopfile ( const char * next );
	const char * GetNextMvMPopfile ();

	virtual void BalanceTeams( bool bRequireSwitcheesToBeDead );
#endif
};

//-----------------------------------------------------------------------------
// Gets us at the team fortress game rules
//-----------------------------------------------------------------------------

inline CTFGameRules* TFGameRules()
{
	return static_cast<CTFGameRules*>(g_pGameRules);
}

inline float CTFGameRules::ItemTesting_GetBotAnimSpeed( void ) 
{ 
	static const ConVar *pHostTimescale = NULL;

	if ( !pHostTimescale )
	{
		pHostTimescale = cvar->FindVar( "host_timescale" );
	}

	if ( pHostTimescale )
		return (m_flItemTesting_BotAnimSpeed * pHostTimescale->GetFloat());
	return m_flItemTesting_BotAnimSpeed;
}

#ifdef TF_RAID_MODE

inline bool CTFGameRules::IsRaidMode( void ) const
{
#ifdef GAME_DLL
	return m_hRaidLogic != NULL; 
#else
	return tf_gamemode_raid.GetBool();
#endif
}



inline bool CTFGameRules::IsBossBattleMode( void ) const
{
	return tf_gamemode_boss_battle.GetBool();
}

#endif // TF_RAID_MODE

#ifdef TF_CREEP_MODE

inline bool CTFGameRules::IsCreepWaveMode( void ) const
{
	return tf_gamemode_creep_wave.GetBool();
}

#endif


inline bool CTFGameRules::IsHalloweenScenario( HalloweenScenarioType scenario ) const
{
	return m_halloweenScenario == scenario;
}


#ifdef GAME_DLL
bool EntityPlacementTest( CBaseEntity *pMainEnt, const Vector &vOrigin, Vector &outPos, bool bDropToGround );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CArenaLogic : public CPointEntity
{
	DECLARE_CLASS( CArenaLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	virtual void ArenaLogicThink( void );
	virtual void Spawn( void );

	COutputEvent	m_OnArenaRoundStart;
	float			m_flTimeToEnableCapPoint;
	COutputEvent	m_OnCapEnabled;
	bool			m_bFiredOutput;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCompetitiveLogic : public CPointEntity
{
	DECLARE_CLASS( CCompetitiveLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	void OnSpawnRoomDoorsShouldLock( void );
	void OnSpawnRoomDoorsShouldUnlock( void );

	COutputEvent	m_OnSpawnRoomDoorsShouldLock;
	COutputEvent	m_OnSpawnRoomDoorsShouldUnlock;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLogicMannPower : public CPointEntity
{
	DECLARE_CLASS( CLogicMannPower, CPointEntity );
public:
	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
// Purpose: New training stuff
//-----------------------------------------------------------------------------
class CTrainingModeLogic : public CPointEntity
{
	DECLARE_CLASS( CTrainingModeLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	void SetupOnRoundStart( void );
	void SetTrainingMsg( const char *msg );
	void SetTrainingObjective( const char *msg );
	void OnPlayerSpawned( CTFPlayer *pPlayer );
	void OnPlayerDied( CTFPlayer *pPlayer, CBaseEntity *pKiller );
	void OnBotDied( CTFPlayer *pPlayer, CBaseEntity *pKiller );
	void OnPlayerSwitchedWeapons( CTFPlayer *pPlayer );
	void OnPlayerWantsToContinue();
	void OnPlayerBuiltBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject );
	void OnPlayerUpgradedBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject );
	void OnPlayerDetonateBuilding( CTFPlayer *pPlayer, CBaseObject *pBaseObject );
	void UpdateHUDObjective();
	const char* GetNextMap();
	const char* GetTrainingEndText();
	int GetDesiredClass() const;

	// Inputs
	void InputForcePlayerSpawnAsClassOutput( inputdata_t &inputdata );
	void InputKickAllBots( inputdata_t &inputdata );
	void InputShowTrainingMsg( inputdata_t &inputdata );
	void InputShowTrainingObjective( inputdata_t &inputdata );
	void InputShowTrainingHUD( inputdata_t &inputdata );
	void InputHideTrainingHUD( inputdata_t &inputdata );
	void InputEndTraining( inputdata_t &inputdata );
	void InputPlaySoundOnPlayer( inputdata_t &inputdata );
	void InputWaitForTimerOrKeypress( inputdata_t &inputdata );
	void InputSetNextMap( inputdata_t &inputdata );
	void InputForcePlayerSwapToWeapon( inputdata_t &inputdata );

protected:
	enum
	{
		kMaxLengthObjectiveText = 128,
	};

	// outputs based on the class the player spawned as
	COutputEvent m_outputOnPlayerSpawnAsScout;
	COutputEvent m_outputOnPlayerSpawnAsSniper;
	COutputEvent m_outputOnPlayerSpawnAsSoldier;
	COutputEvent m_outputOnPlayerSpawnAsDemoman;
	COutputEvent m_outputOnPlayerSpawnAsMedic;
	COutputEvent m_outputOnPlayerSpawnAsHeavy;
	COutputEvent m_outputOnPlayerSpawnAsPyro;
	COutputEvent m_outputOnPlayerSpawnAsSpy;
	COutputEvent m_outputOnPlayerSpawnAsEngineer;
	// outputs based on the weapon the player swapped to
	COutputEvent m_outputOnPlayerSwappedToWeaponSlotPrimary;
	COutputEvent m_outputOnPlayerSwappedToWeaponSlotSecondary;
	COutputEvent m_outputOnPlayerSwappedToWeaponSlotMelee;
	COutputEvent m_outputOnPlayerSwappedToWeaponSlotBuilding;
	COutputEvent m_outputOnPlayerSwappedToWeaponSlotPDA;
	// outputs based on if the player built inside a suggested area
	COutputEvent m_outputOnPlayerBuiltOutsideSuggestedArea;
	// player detonated their own building
	COutputEvent m_outputOnPlayerDetonateBuilding;
	// other outputs
	COutputEvent m_outputOnPlayerDied;
	COutputEvent m_outputOnBotDied;

	CHandle<CBaseEntity> m_waitingForKeypressTimer;
	string_t m_nextMapName;
	char m_objText[kMaxLengthObjectiveText];
	string_t m_endTrainingText;
};

class CMultipleEscort : public CPointEntity
{
	DECLARE_CLASS( CMultipleEscort, CPointEntity );
public:

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

class CMedievalLogic : public CPointEntity
{
	DECLARE_CLASS( CMedievalLogic, CPointEntity );
public:

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

class CHybridMap_CTF_CP : public CPointEntity
{
	DECLARE_CLASS( CHybridMap_CTF_CP, CPointEntity );
public:

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

class CTFHolidayEntity : public CPointEntity, public CGameEventListener
{
	DECLARE_CLASS( CTFHolidayEntity, CPointEntity );
public:
	DECLARE_DATADESC();

	CTFHolidayEntity()
	{ 
		m_nHolidayType = kHoliday_None;
		m_nTauntInHell = 0;
		m_nAllowHaunting = 0;
		ListenForGameEvent( "player_turned_to_ghost" );
		ListenForGameEvent( "player_disconnect" );
		ListenForGameEvent( "player_team" );
	}
	~CTFHolidayEntity()
	{
	}

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
	int GetHolidayType( void ){ return m_nHolidayType; }
	bool ShouldTauntInHell( void ){ return ( m_nTauntInHell > 0 ); }
	bool ShouldAllowHaunting( void ){ return ( m_nAllowHaunting > 0 ); }

	void InputHalloweenSetUsingSpells( inputdata_t &inputdata );
	void InputHalloweenTeleportToHell( inputdata_t &inputdata );

	virtual void FireGameEvent( IGameEvent *event );

	void ResetWinner() { m_nWinningTeam = TF_TEAM_COUNT; }
	int GetWinningTeam() const { return m_nWinningTeam; }
private:

	void HalloweenTeleportToHellDanceThink( void );
	void Teleport();

	CUtlVector< CHandle<CTFPlayer> > m_vecDancers;
	int m_nWinningTeam;

	int m_nHolidayType;
	int m_nTauntInHell;
	int m_nAllowHaunting;
};

class CKothLogic : public CPointEntity
{
	DECLARE_CLASS( CKothLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CKothLogic()
	{ 
		m_nTimerInitialLength = 180; // seconds
		m_nTimeToUnlockPoint = 30; // seconds

		m_hRedTimer = NULL;
		m_hBlueTimer = NULL;
	}
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	int GetInitialTimerLength( void ){ return m_nTimerInitialLength; }
	int GetTimerToUnlockPoint( void ){ return m_nTimeToUnlockPoint; }

	void InputRoundSpawn( inputdata_t &inputdata );
	void InputRoundActivate( inputdata_t &inputdata );
	void InputSetRedTimer( inputdata_t &inputdata );
	void InputSetBlueTimer( inputdata_t &inputdata );
	void InputAddRedTimer( inputdata_t &inputdata );
	void InputAddBlueTimer( inputdata_t &inputdata );

private:
	int m_nTimerInitialLength;
	int m_nTimeToUnlockPoint;

	CHandle< CTeamRoundTimer > m_hRedTimer;
	CHandle< CTeamRoundTimer > m_hBlueTimer;
};

#define CP_TIMER_THINK "CCPTimerLogicThink"
class CCPTimerLogic : public CPointEntity
{
	DECLARE_CLASS( CCPTimerLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CCPTimerLogic()
	{ 
		m_nTimerLength = 60; // seconds
		m_iszControlPointName = NULL_STRING;
		m_hControlPoint = NULL;
		m_bFire15SecRemain = m_bFire10SecRemain = m_bFire5SecRemain = true;
	
		SetContextThink( &CCPTimerLogic::Think, gpGlobals->curtime + 0.15, CP_TIMER_THINK );
	}
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void InputRoundSpawn( inputdata_t &inputdata );
	void Think( void );
	bool TimerMayExpire( void );

private:
	int m_nTimerLength;
	string_t m_iszControlPointName;
	CHandle<CTeamControlPoint> m_hControlPoint;
	CountdownTimer m_pointTimer;

	bool m_bFire15SecRemain;
	bool m_bFire10SecRemain;
	bool m_bFire5SecRemain;

	COutputEvent m_onCountdownStart;
	COutputEvent m_onCountdown15SecRemain;
	COutputEvent m_onCountdown10SecRemain;
	COutputEvent m_onCountdown5SecRemain;
	COutputEvent m_onCountdownEnd;

	int m_nTimerTeam = TF_TEAM_BLUE;
};
#endif

class CBonusRoundLogic : public CBaseEntity
{
	DECLARE_CLASS( CBonusRoundLogic, CBaseEntity );
public:
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	bool		InitBonusRound( void );
	void		SetBonusItem( itemid_t iItemID );
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
#endif

	void		BuildBonusPlayerList( void );
	int			GetNumBonusPlayers( void ) { return m_aBonusPlayerList.Count(); }
	CTFPlayer	*GetBonusPlayer( int i ) { Assert ( i < m_aBonusPlayerList.Count() ); return m_aBonusPlayerList[i]; }
	CTFPlayer	*GetBonusWinner( void ) { return m_hBonusWinner.Get(); }
	void		SetBonusStateAborted( bool bAborted ) { m_bAbortedBonusRound = bAborted; }
	bool		BonusStateAborted( void ) { return m_bAbortedBonusRound; }
	int			GetPlayerBonusRoll( int iPlayer ) { return (iPlayer < m_aBonusPlayerRoll.Count()) ? m_aBonusPlayerRoll[iPlayer] : 0; }
	CEconItemView	*GetBonusItem( void ) { return &m_Item; }

private:
	CUtlSortVector< BONUSPLAYERPTR, CBonusPlayerListLess >	m_aBonusPlayerList;
	CUtlVector<int>			m_aBonusPlayerRoll;
	CNetworkHandle( CTFPlayer, m_hBonusWinner );
	CNetworkVar( bool,		m_bAbortedBonusRound );
	itemid_t			m_iBonusItemID;
	CNetworkVarEmbedded( CEconItemView,	m_Item );
};

#ifdef GAME_DLL
class CSingleUserReliableRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserReliableRecipientFilter( CBasePlayer *player )
	{
		AddRecipient( player );
		MakeReliable();
	}
};
#endif

#endif // TF_GAMERULES_H
