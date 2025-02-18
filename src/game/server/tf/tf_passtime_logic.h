//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PASSTIME_LOGIC_H
#define TF_PASSTIME_LOGIC_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "tf_passtime_ball.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
class CTFPlayer;
class CTFPasstimeBall;
class CPasstimeBallSpawn;
class CFuncPasstimeGoal;
class CCountdownAnnouncer;
class CTrackPath;
struct SetSectionParams;
enum HudNotification_t;

//-----------------------------------------------------------------------------
class CTFPasstimeLogic : public CPointEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFPasstimeLogic, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFPasstimeLogic();
	virtual ~CTFPasstimeLogic();
	virtual void Spawn() OVERRIDE;
	virtual void Precache() OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;

	void LaunchBall( CTFPlayer *pPlayer, const Vector &pos, const Vector &vel );
	void EjectBall( CTFPlayer *pPlayer, CTFPlayer *pAttacker );

	bool BCanPlayerPickUpBall( CTFPlayer *pPlayer, HudNotification_t *pReason = 0 ) const;
	CPasstimeBall *GetBall() const;

	void OnBallCarrierDamaged( CTFPlayer *pPlayer, CTFPlayer *pAttacker, const CTakeDamageInfo& info );
	void OnBallCarrierMeleeHit( CTFPlayer *pPlayer, CTFPlayer *pAttacker );
	void OnPlayerTouchBall( CTFPlayer *pPlayer, CPasstimeBall *pBall );
	void OnEnterGoal( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal );
	void OnEnterGoal( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal );
	void OnExitGoal( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal );
	void OnStayInGoal( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal );
	bool OnBallCollision( CPasstimeBall *pBall, int index, gamevcollisionevent_t *pEvent );
	float GetLastHeldTime( CTFPlayer* pPlayer );
	float GetLastPassTime( CTFPlayer* pPlayer );
	void SetLastPassTime( CTFPlayer* pPlayer );
	void RespawnBall();
	float GetMaxPassRange() const { return m_flMaxPassRange; }
	CTFPlayer *GetBallCarrier() const;
	float GetPackSpeed( CTFPlayer *pPlayer ) const;

	static void AddCondToTeam( ETFCond eCond, int iTeam, float flTime );

private:
	void PostSpawn();
	void InputSetSection( inputdata_t &input );
	bool ParseSetSection( const char *pStr, SetSectionParams &s ) const;
	void InputSpawnBall( inputdata_t &input );
	void InputTimeUp( inputdata_t &input );
	void InputSpeedBoostUsed( inputdata_t &input );
	void InputJumpPadUsed( inputdata_t &input );

	void StopAskForBallEffects();
	void OnBallGet();
	void Score( CTFPlayer *pPlayer, CFuncPasstimeGoal *pGoal );
	void Score( CPasstimeBall *pBall, CFuncPasstimeGoal *pGoal );
	void Score( CTFPlayer *pPlayer, int iTeam, int iPoints, bool iForceWin );
	void SpawnBallAtRandomSpawnerThink();
	void SpawnBallAtRandomSpawner();
	void SpawnBallAtSpawner( CPasstimeBallSpawn *pSpawner );
	void MoveBallToSpawner();
	void StealBall( CTFPlayer *pFrom, CTFPlayer *pTo );
	void ThinkExpiredTimer();
	void EndRoundExpiredTimer();
	void CrowdReactionSound( int iTeam );

	void OneSecStatsUpdateThink();
	void BallHistSampleThink();
	void BallPower_PowerThink();
	void BallPower_PackThink();
	void BallPower_PackHealThink();
	float CalcProgressFrac() const;
	bool AddBallPower( int iPower );
	void ClearBallPower();
	bool ShouldEndOvertime() const;
	void ReplicatePackMemberBits();

	CUtlVector< std::pair<CTFPlayer*, float> > m_ballLastPassTimes;
	CUtlVector< std::pair<CTFPlayer*, float> > m_ballLastHeldTimes;
	CCountdownAnnouncer *m_pRespawnCountdown;
	int m_iBallSpawnCountdownSec;
	float m_flNextCrowdReactionTime;
	uint64 m_nPackMemberBits;
	uint64 m_nPrevPackMemberBits;

	// outputs
	COutputEvent m_onBallFree;
	COutputEvent m_onBallGetRed;
	COutputEvent m_onBallGetBlu;
	COutputEvent m_onBallGetAny;
	COutputEvent m_onBallRemoved;
	COutputEvent m_onScoreRed;
	COutputEvent m_onScoreBlu;
	COutputEvent m_onScoreAny;
	COutputEvent m_onBallPowerUp;
	COutputEvent m_onBallPowerDown;

	// secret room stuff
	void SecretRoom_Spawn();
	void statica( inputdata_t &input ); // SecretRoom_InputStartTouchPlayerSlot
	void staticb( inputdata_t &input ); // SecretRoom_InputEndTouchPlayerSlot
	void staticc( inputdata_t &input ); // SecretRoom_InputPlugDamaged
	void InputRoomTriggerOnTouch( inputdata_t &input );
	void SecretRoom_UpdateTv( int iNumSlotsFilled );
	void SecretRoom_Solve();
	int SecretRoom_CountSlottedPlayers() const;
	CTFPlayer **SecretRoom_GetPlayerSlotInfoForTrigger( const char *pTriggerName, int *piExpectedClass, int *piExpectedTeam );

	CBaseEntity *m_SecretRoom_pTv;
	CSoundPatch* m_SecretRoom_pTvSound;
	enum class SecretRoomState { None, Open, Solved } m_SecretRoom_state;
	CTFPlayer *m_SecretRoom_slottedPlayers[9];
	CUtlVector<CSteamID> m_SecretRoom_playersThatTouchedRoom;
	
	// netvars
	CNetworkHandle( CPasstimeBall, m_hBall );
	CNetworkArray( Vector, m_trackPoints, 16 );
	CNetworkVar( int, m_iNumSections );
	CNetworkVar( int, m_iCurrentSection );
	CNetworkVar( float, m_flMaxPassRange );
	CNetworkVar( int, m_iBallPower );
	CNetworkVar( float, m_flPackSpeed );
	CNetworkArray( int, m_bPlayerIsPackMember, MAX_PLAYERS_ARRAY_SAFE ); // +1 for easy entity index
};

//-----------------------------------------------------------------------------
extern CTFPasstimeLogic *g_pPasstimeLogic;

#endif // TF_PASSTIME_LOGIC_H  
