//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// func_passtime_goal - based on func_capture_zone
#ifndef FUNC_PASSTIME_GOAL_H
#define FUNC_PASSTIME_GOAL_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CPasstimeBall;

//-----------------------------------------------------------------------------
// This class is to get around the fact that DEFINE_FUNCTION doesn't like multiple inheritance
// TODO: make AutoList work without inheritance
class CFuncPasstimeGoalShim : public CBaseTrigger
{
public:
	virtual void StartTouch(CBaseEntity *pOther) OVERRIDE { CBaseTrigger::StartTouch(pOther); ShimStartTouch(pOther); }
	virtual void EndTouch(CBaseEntity *pOther) OVERRIDE { CBaseTrigger::EndTouch(pOther); ShimEndTouch(pOther); }
	
private:
	virtual void ShimStartTouch( CBaseEntity* pOther ) = 0;
	virtual void ShimEndTouch( CBaseEntity* pOther ) = 0;
};

//-----------------------------------------------------------------------------
class CFuncPasstimeGoal : public CFuncPasstimeGoalShim, public TAutoList< CFuncPasstimeGoal >
{
public:
	DECLARE_CLASS( CFuncPasstimeGoal, CFuncPasstimeGoalShim );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	CFuncPasstimeGoal();
	virtual void Spawn() OVERRIDE;
	virtual int	UpdateTransmitState() OVERRIDE;
	void OnScore( int team );
	int Points() const { return m_iPoints; }
	bool IsDisabled() const { return m_bTriggerDisabled; }

	enum SpawnFlags 
	{
		WIN_ON_SCORE = 1,
		DISABLE_BALL_SCORE = 2,
		ENABLE_PLAYER_SCORE = 4,
		TYPE_TOWER_GOAL = 8,
	};

	// FIXME: this is copypasta with c_func_passtime_goal
	enum GoalType
	{
		TYPE_HOOP,
		TYPE_ENDZONE,
		TYPE_TOWER,
	};

	bool BWinOnScore() const { return (GetSpawnFlags() & (CFuncPasstimeGoal::WIN_ON_SCORE << 24)) != 0; }
	bool BDisableBallScore() const { return (GetSpawnFlags() & (CFuncPasstimeGoal::DISABLE_BALL_SCORE << 24)) != 0; }
	bool BEnablePlayerScore() const { return (GetSpawnFlags() & (CFuncPasstimeGoal::ENABLE_PLAYER_SCORE << 24)) != 0; }
	bool BTowerGoal() const { return (GetSpawnFlags() & (CFuncPasstimeGoal::TYPE_TOWER_GOAL << 24)) != 0; }

private:
	virtual void ShimStartTouch( CBaseEntity *pOther ) OVERRIDE;
	virtual void ShimEndTouch( CBaseEntity *pOther ) OVERRIDE; 
	bool CanTouchMe( CBaseEntity *pOther );
	void GoalThink();

	COutputEvent m_onScoreBlu;
	COutputEvent m_onScoreRed;
	int m_iPoints;
	CNetworkVar( bool, m_bTriggerDisabled );
	CNetworkVar( int, m_iGoalType );
};

#endif // FUNC_PASSTIME_GOAL_H  
