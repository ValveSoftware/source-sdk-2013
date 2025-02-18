//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//
#ifndef LOGIC_ROBOT_DESTRUCTION_H
#define LOGIC_ROBOT_DESTRUCTION_H
#pragma once

#include "cbase.h"

#ifdef GAME_DLL
	#include "triggers.h"
	#include "tf_shareddefs.h"
	#include "GameEventListener.h"
	#include "entity_capture_flag.h"
#else
	#include "c_tf_player.h"
#endif


#include "tf_robot_destruction_robot.h"

#ifdef CLIENT_DLL
	#define CTFRobotDestructionLogic C_TFRobotDestructionLogic
	#define CTFRobotDestruction_RobotSpawn C_TFRobotDestruction_RobotSpawn
	#define CTFRobotDestruction_RobotGroup C_TFRobotDestruction_RobotGroup
#endif

#include "props_shared.h"

#define RD_POINTS_STOLEN_PER_TICK 2

//-----------------------------------------------------------------------------
class CTFRobotDestruction_RobotSpawn : public CBaseEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFRobotDestruction_RobotSpawn, CBaseEntity )
	DECLARE_NETWORKCLASS();

	CTFRobotDestruction_RobotSpawn();

	virtual void Spawn() OVERRIDE;
	virtual void Activate() OVERRIDE;

#ifdef GAME_DLL
	virtual void Precache() OVERRIDE;
	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const OVERRIDE;

	CTFRobotDestruction_Robot* GetRobot() const { return m_hRobot.Get(); }
	void OnRobotKilled();
	void ClearRobot();
	void SpawnRobot();
	void SetGroup( class CTFRobotDestruction_RobotGroup* pGroup ) { m_hGroup.Set( pGroup ); }
	// Inputs
	void InputSpawnRobot( inputdata_t &inputdata );
	
#endif
private:
	CHandle< CTFRobotDestruction_Robot > m_hRobot;
#ifdef GAME_DLL
	CHandle< class CTFRobotDestruction_RobotGroup > m_hGroup;
	RobotSpawnData_t m_spawnData;
	COutputEvent m_OnRobotKilled;
#endif
};

//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IRobotDestructionGroupAutoList );
class CTFRobotDestruction_RobotGroup : public CBaseEntity, public IRobotDestructionGroupAutoList
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CTFRobotDestruction_RobotGroup, CBaseEntity )
	DECLARE_NETWORKCLASS();
public:
	virtual ~CTFRobotDestruction_RobotGroup();
#ifdef GAME_DLL

	CTFRobotDestruction_RobotGroup();

	virtual int		UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_ALWAYS ); }
	virtual void	Spawn() OVERRIDE;
	virtual void	Activate() OVERRIDE;
	void	AddToGroup( CTFRobotDestruction_RobotSpawn * pSpawn );
	void	RemoveFromGroup( CTFRobotDestruction_RobotSpawn * pSpawn );
	void	UpdateState();
	void	RespawnRobots();
	int		GetNumAliveBots() const;
	float	GetTeamRespawnScale() const	{ return m_flTeamRespawnReductionScale; }

	// Respawn functions
	void StopRespawnTimer();
	void StartRespawnTimerIfNeeded( CTFRobotDestruction_RobotGroup *pMasterGroup );
	void RespawnCountdownFinish();

	void EnableUberForGroup();
	void DisableUberForGroup();

	void OnRobotAttacked();
	void OnRobotKilled();
	void OnRobotSpawned();
#else
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;
	virtual int GetTeamNumber( void ) const OVERRIDE { return m_iTeamNum; }
	virtual void SetDormant( bool bDormant ) OVERRIDE;
#endif
	const char *GetHUDIcon() const		{ return m_pszHudIcon; }
	int		GetGroupNumber() const		{ return m_nGroupNumber; }
	int		GetState() const			{ return m_nState; }
	float	GetRespawnStartTime() const	{ return m_flRespawnStartTime; }
	float	GetRespawnEndTime() const	{ return m_flRespawnEndTime; }
	float	GetLastAttackedTime() const	{ return m_flLastAttackedTime; }

private:

#ifdef GAME_DLL
	CUtlVector< CTFRobotDestruction_RobotSpawn* > m_vecSpawns;
	int m_nTeamNumber;
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iTeamNum );
	float m_flRespawnTime;
	static float m_sflNextAllowedAttackAlertTime[ TF_TEAM_COUNT ];
	string_t m_iszHudIcon;
	float m_flTeamRespawnReductionScale;

	COutputEvent m_OnRobotsRespawn;
	COutputEvent m_OnAllRobotsDead;
#else
	int m_iTeamNum;
#endif
	CNetworkString( m_pszHudIcon, MAX_PATH );
	CNetworkVar( int, m_nGroupNumber );
	CNetworkVar( int, m_nState );
	CNetworkVar( float, m_flRespawnStartTime );
	CNetworkVar( float, m_flRespawnEndTime );
	CNetworkVar( float, m_flLastAttackedTime );
};

struct RateLimitedSound_t
{
	RateLimitedSound_t( float flPause )
	{
		m_mapNextAllowedTime.SetLessFunc( DefLessFunc( const CBaseEntity* ) );
		m_flPause = flPause;
	}

	float m_flPause;
	CUtlMap< const CBaseEntity*, float > m_mapNextAllowedTime;
};

struct TeamSound_t
{
	const char *m_pszYourTeam;
	const char *m_pszTheirTeam;
};

//-----------------------------------------------------------------------------
class CTFRobotDestructionLogic : public CBaseEntity
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFRobotDestructionLogic, CBaseEntity )
	DECLARE_NETWORKCLASS();
public:

	enum EType
	{
		TYPE_ROBOT_DESTRUCTION,
		TYPE_PLAYER_DESTRUCTION,
	};

	virtual EType GetType() const { return TYPE_ROBOT_DESTRUCTION; }

	CTFRobotDestructionLogic();
	virtual ~CTFRobotDestructionLogic();
	static CTFRobotDestructionLogic* GetRobotDestructionLogic();

	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;

	float	GetRespawnScaleForTeam( int nTeam ) const;
	int		GetScore( int nTeam ) const;
	int		GetTargetScore( int nTeam ) const;
	int		GetMaxPoints() const { return m_nMaxPoints.Get(); }
	float	GetFinaleWinTime( int nTeam ) const;
	float	GetFinaleLength() const { return m_flFinaleLength; }
	void	PlaySoundInfoForScoreEvent( CTFPlayer* pPlayer, bool bPositive, int nNewScore, int nTeam, RDScoreMethod_t eMethod = SCORE_UNDEFINED );
	RDScoreMethod_t GetLastScoreMethod( int nTeam ) const { return (RDScoreMethod_t)m_eWinningMethod[ nTeam ]; }

#ifdef CLIENT_DLL

	virtual void OnDataChanged( DataUpdateType_t type ) OVERRIDE;
	virtual void ClientThink() OVERRIDE;
	const char* GetResFile() const { return STRING( m_szResFile ); }

#else
	DECLARE_DATADESC();

	virtual void Activate() OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;

	virtual int UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_ALWAYS ); }

	CTFRobotDestruction_Robot * IterateRobots( CTFRobotDestruction_Robot * ) const;
	void	RobotCreated( CTFRobotDestruction_Robot *pRobot );
	void	RobotRemoved( CTFRobotDestruction_Robot *pRobot );
	void	RobotAttacked( CTFRobotDestruction_Robot *pRobot );
	float	GetScoringInterval() const { return m_flRobotScoreInterval; }
	void	ScorePoints( int nTeam, int nPoints, RDScoreMethod_t eMethod, CTFPlayer *pPlayer );
	void	AddRobotGroup( CTFRobotDestruction_RobotGroup* pGroup );
	void	ManageGameState();
	void	FlagCreated( int nTeam );
	void	FlagDestroyed( int nTeam );
	
	void	DBG_SetMaxPoints( int nNewMax ) { m_nMaxPoints.Set( nNewMax ); }
	void	InputRoundActivate( inputdata_t &inputdata );
	virtual int GetHealDistance( void ) { return 64; }
#endif

	virtual void SetCountdownEndTime( float flTime ){ m_flCountdownEndTime = flTime; }
	virtual float GetCountdownEndTime(){ return m_flCountdownEndTime; }
	virtual CTFPlayer *GetTeamLeader( int iTeam ) const { return NULL; }
	virtual string_t GetCountdownImage( void ) { return NULL_STRING; }
	virtual bool IsUsingCustomCountdownImage( void ) { return false; }

protected:


#ifdef GAME_DLL
	virtual void OnRedScoreChanged() {}
	virtual void OnBlueScoreChanged() {}
	void	ApproachTargetScoresThink();
	int		ApproachTeamTargetScore( int nTeam, int nApproachScore, int nCurrentScore );
	void	PlaySoundInPlayersEars( CTFPlayer* pPlayer, const EmitSound_t& params ) const;
	void	RedTeamWin();
	void	BlueTeamWin();
	virtual void	TeamWin( int nTeam );

	typedef CUtlMap< int, CTFRobotDestruction_RobotGroup* > RobotSpawnMap_t;
	CUtlVector< CTFRobotDestruction_Robot* > m_vecRobots;
	CUtlVector< CTFRobotDestruction_RobotGroup * > m_vecSpawnGroups;
	float m_flLoserRespawnBonusPerBot;
	float m_flRobotScoreInterval;
	float m_flNextRedRobotAttackedAlertTime;
	float m_flNextBlueRobotAttackedAlertTime;
	int m_nNumFlagsOut[ TF_TEAM_COUNT ];
	bool m_bEducateNewConnectors;
	string_t m_iszResFile;

	TeamSound_t m_AnnouncerProgressSound;
	CUtlMap< const char *, RateLimitedSound_t * > m_mapRateLimitedSounds;

	CUtlVector< CTFPlayer* > m_vecEducatedPlayers;

	// Outputs
	COutputEvent m_OnRedFinalePeriodEnd;
	COutputEvent m_OnBlueFinalePeriodEnd;
	COutputEvent m_OnBlueHitZeroPoints;
	COutputEvent m_OnRedHitZeroPoints;
	COutputEvent m_OnBlueHasPoints;
	COutputEvent m_OnRedHasPoints;
	COutputEvent m_OnBlueHitMaxPoints;
	COutputEvent m_OnRedHitMaxPoints;
	COutputEvent m_OnBlueLeaveMaxPoints;
	COutputEvent m_OnRedLeaveMaxPoints;

	COutputEvent m_OnRedFirstFlagStolen;
	COutputEvent m_OnRedFlagStolen;
	COutputEvent m_OnRedLastFlagReturned;
	COutputEvent m_OnBlueFirstFlagStolen;
	COutputEvent m_OnBlueFlagStolen;
	COutputEvent m_OnBlueLastFlagReturned;
#else
	float		m_flLastTickSoundTime;
#endif
	static CTFRobotDestructionLogic* m_sCTFRobotDestructionLogic;
	CNetworkVar( int, m_nMaxPoints );
	CNetworkVar( float, m_flFinaleLength );
	CNetworkVar( float, m_flBlueFinaleEndTime );
	CNetworkVar( float, m_flRedFinaleEndTime );
	CNetworkVar( int, m_nBlueScore );
	CNetworkVar( int, m_nRedScore );
	CNetworkVar( int, m_nBlueTargetPoints );
	CNetworkVar( int, m_nRedTargetPoints );
	CNetworkVar( float, m_flBlueTeamRespawnScale );
	CNetworkVar( float, m_flRedTeamRespawnScale );
	CNetworkString( m_szResFile, MAX_PATH );
	CNetworkArray( int, m_eWinningMethod, TF_TEAM_COUNT );
	CNetworkVar( float, m_flCountdownEndTime ); // used for player destruction countdown timers
};

#ifdef GAME_DLL
class CRobotDestructionVaultTrigger : public CBaseTrigger
{
	DECLARE_CLASS( CRobotDestructionVaultTrigger, CBaseTrigger );
	DECLARE_DATADESC();

public:
	CRobotDestructionVaultTrigger();
	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;

	virtual bool PassesTriggerFilters( CBaseEntity *pOther ) OVERRIDE;
	virtual void StartTouch(CBaseEntity *pOther) OVERRIDE;
	virtual void EndTouch(CBaseEntity *pOther) OVERRIDE;

private:
	void StealPointsThink();
	int StealPoints( CTFPlayer *pPlayer );

	bool m_bIsStealing;
	COutputEvent m_OnPointsStolen;
	COutputEvent m_OnPointsStartStealing;
	COutputEvent m_OnPointsEndStealing;
};

#endif// GAME_DLL
#endif// LOGIC_ROBOT_DESTRUCTION_H
