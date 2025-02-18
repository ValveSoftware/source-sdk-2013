//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entities for use in the Robot Destruction TF2 game mode.
//
//=========================================================================//
#ifndef TF_LOGIC_HALLOWEEN_2014_H
#define TF_LOGIC_HALLOWEEN_2014_H

#include "cbase.h"
#include "tf_shareddefs.h"
#ifdef GAME_DLL
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
#endif


#ifdef CLIENT_DLL
	#define CTFMinigameLogic C_TFMinigameLogic
	#define CTFMiniGame C_TFMiniGame
	#define CTFHalloweenMinigame C_TFHalloweenMinigame
	#define CTFHalloweenMinigame_FallingPlatforms C_TFHalloweenMinigame_FallingPlatforms
#endif

#define MINIGAME_INVALID -1

DECLARE_AUTO_LIST( IMinigameAutoList );

class CTFMiniGame : public CBaseEntity
#ifdef GAME_DLL
	, public CGameEventListener
#endif
	, public IMinigameAutoList
{
public:
	enum EScoringType
	{
		SCORING_TYPE_POINTS = 0,
		SCORING_TYPE_PLAYERS_ALIVE,

		NUM_SCORING_TYPES
	};

	enum EMinigameType
	{
		MINIGAME_GENERIC = 0,
		MINIGAME_HALLOWEEN2014_COLLECTION,
		MINIGAME_HALLOWEEN2014_PLATFORMS,
		MINIGAME_HALLOWEEN2014_SOCCER,
		// don't change the order of these first three types because the TF_HALLOWEEN_DOOMSDAY_WIN_MINIROUNDS achievement depends on it

		NUM_MINIGAME_TYPES
	};

	DECLARE_CLASS( CTFMiniGame, CBaseEntity )
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFMiniGame();
	
#ifdef GAME_DLL
	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_ALWAYS ); }

	void InputScoreTeamRed( inputdata_t &inputdata );
	void InputScoreTeamBlue( inputdata_t &inputdata );
	void InputChangeHudResFile( inputdata_t &inputdata );

	virtual void ScorePointsForTeam( int nTeamNum, int nPoints );
	virtual void TeleportAllPlayers();
	virtual void OnTeleportPlayerToMinigame( CTFPlayer *pPlayer );
	virtual void ReturnAllPlayers();
	const char *GetTeamSpawnPointName( int nTeamNum ) const;
	const bool AllowedInRandom() const { return m_bMinigameAllowedInRamdomPool; }
	virtual void UpdateDeadPlayers( int nTeam, COutputEvent& eventWin, COutputEvent& eventAllDead, bool& bCanWin );
	EMinigameType GetMinigameType() const { return m_eMinigameType; }
	void SetAdvantagedTeam ( int iAdvantageTeam ) { m_iAdvantagedTeam = iAdvantageTeam; }
#else
	const char *GetResFile() const { return m_pszHudResFile; }
	int GetMaxScore( void ) const { return m_nMaxScoreForMiniGame; }
	int GetScoreForTeam( int nTeamNum ) const;
#endif

protected:

#ifdef GAME_DLL
	virtual void InternalHandleInputScore( inputdata_t &inputdata ){}
	virtual void SuddenDeathTimeStartThink();

	COutputEvent m_OnRedHitMaxScore;
	COutputEvent m_OnBlueHitMaxScore;
	COutputEvent m_OnTeleportToMinigame;
	COutputEvent m_OnReturnFromMinigame;
	COutputEvent m_OnAllRedDead;
	COutputEvent m_OnAllBlueDead;
	COutputEvent m_OnSuddenDeathStart;

	const char *m_pszTeamSpawnPoint[ TF_TEAM_COUNT ];
	bool m_bMinigameAllowedInRamdomPool;
	bool m_bIsActive;
	string_t m_iszHudResFile;
	EMinigameType m_eMinigameType;
	string_t m_iszYourTeamScoreSound;
	string_t m_iszEnemyTeamScoreSound;
	float m_flSuddenDeathTime; // -1: No sudden death, 0: In sudden death, >0: Sudden death time.
	int m_iAdvantagedTeam; 
#endif

	CNetworkString( m_pszHudResFile, MAX_PATH );
	CNetworkVar( int, m_nMaxScoreForMiniGame );
	CNetworkArray( int, m_nMinigameTeamScore, TF_TEAM_COUNT );
	CNetworkVar( EScoringType, m_eScoringType );
};


class CTFHalloweenMinigame : public CTFMiniGame
{
	DECLARE_CLASS( CTFHalloweenMinigame, CTFMiniGame )
	DECLARE_NETWORKCLASS();
public:
	
#ifdef GAME_DLL
	CTFHalloweenMinigame();

	DECLARE_DATADESC();

	virtual void Spawn() OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;

	virtual void TeleportAllPlayers() OVERRIDE;
	virtual void OnTeleportPlayerToMinigame( CTFPlayer *pPlayer ) OVERRIDE;
	virtual void ReturnAllPlayers() OVERRIDE;

	void InputKartWinAnimationRed( inputdata_t &inputdata );
	void InputKartWinAnimationBlue( inputdata_t &inputdata );
	void InputKartLoseAnimationRed( inputdata_t &inputdata );
	void InputKartLoseAnimationBlue( inputdata_t &inputdata );
	
	void InputEnableSpawnBoss( inputdata_t &inputdata );
	void InputDisableSpawnBoss( inputdata_t &inputdata );

protected:
	virtual void InternalHandleInputScore( inputdata_t &inputdata ) OVERRIDE;

private:

	void TeleportAllPlayersThink();

	EHANDLE m_hBossSpawnPoint;
	EHANDLE m_hHalloweenBoss;
#endif // GAME_DLL
};

class CTFHalloweenMinigame_FallingPlatforms : public CTFHalloweenMinigame
{
	DECLARE_CLASS( CTFHalloweenMinigame_FallingPlatforms, CTFHalloweenMinigame )
	DECLARE_NETWORKCLASS();
public:

#ifdef GAME_DLL
	CTFHalloweenMinigame_FallingPlatforms();

	DECLARE_DATADESC();

	void InputChoosePlatform( inputdata_t &inputdata );
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;

	COutputInt m_OutputSafePlatform;
	COutputInt m_OutputRemovePlatform;

private:
	CCopyableUtlVector< int > m_vecRemainingPlatforms;
#endif
};

class CTFMinigameLogic : public CBaseEntity
{
	DECLARE_CLASS( CTFMinigameLogic, CBaseEntity )
	DECLARE_NETWORKCLASS();
public:
	CTFMinigameLogic();
	virtual ~CTFMinigameLogic();

	static CTFMinigameLogic* GetMinigameLogic() { return m_sMinigameLogic; }
	CTFMiniGame *GetActiveMinigame() const { return m_hActiveMinigame; }
#ifdef GAME_DLL
	DECLARE_DATADESC();
	virtual int UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_ALWAYS ); }

	void InputReturnFromMinigame( inputdata_t &inputdata );
	void InputTeleportToMinigame( inputdata_t &inputdata );
	void InputSetAdvantageTeam( inputdata_t &inputdata );
	void InputTeleportToRandomMinigame( inputdata_t &inputdata );
#endif

protected:

	static CTFMinigameLogic* m_sMinigameLogic;

#ifdef GAME_DLL

	virtual void TeleportToMinigame( int nMiniGameIndex );
	virtual void ReturnFromMinigame();

	int m_iAdvantagedTeam;

#endif
	CNetworkHandle( CTFMiniGame, m_hActiveMinigame );
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CTFHalloweenFortuneTeller : public CBaseAnimating
#ifdef GAME_DLL
	, public CGameEventListener
#endif
{
	DECLARE_CLASS( CTFHalloweenFortuneTeller, CBaseAnimating );
	DECLARE_DATADESC();

	enum ETellerType
	{
		TELLER_TYPE_EVERYBODY = 0,
		TELLER_TYPE_PERSONAL, 

		NUM_TELLER_TYPES
	};

public:
	CTFHalloweenFortuneTeller();
	~CTFHalloweenFortuneTeller();

	virtual void Spawn() OVERRIDE;
	virtual void UpdateOnRemove() OVERRIDE;

#ifdef GAME_DLL
	void InputEnableFortuneTelling( inputdata_t & );
	void InputDisableFortuneTelling( inputdata_t & );
	void InputStartFortuneTelling( inputdata_t & );
	void InputEndFortuneTelling( inputdata_t & );
#endif // GAME_DLL

protected:
	virtual void Precache() OVERRIDE;

#ifdef GAME_DLL
	void FireGameEvent( IGameEvent* pEvent );
	void UpdateFortuneTellerTime();
	void PauseTimer();
	void ResetTimer();

	void StartFortuneWarning();
	void StartFortuneTell();
	void EndFortuneTell();
	void TellFortune();
	void ApplyFortuneEffect();
	void StopTalkingAnim();
	void DanceThink();
	void SpeakThink();
#endif // GAME_DLL

private:
#ifdef GAME_DLL
	COutputEvent m_OnFortuneWarning;
	COutputEvent m_OnFortuneTold;
	COutputEvent m_OnFortuneCurse;
	COutputEvent m_OnFortuneEnd;
	class CConditionFortuneTellerEffect* m_pActiveFortune;

	string_t m_iszRedTeleport;
	string_t m_iszBlueTeleport;

	bool m_bUseTimer;
	bool m_bWasUsingTimer;
	float m_flStartTime;
	float m_flPauseTime;
#endif // GAME_DLL
};

#endif // TF_LOGIC_HALLOWEEN_2014
