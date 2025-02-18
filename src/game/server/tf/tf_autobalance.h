//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_AUTOBALANCE_H
#define TF_AUTOBALANCE_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"

enum autobalance_state_t
{
	AB_STATE_INACTIVE = 0,
	AB_STATE_MONITOR,
	AB_STATE_FORCE_DEAD_CANDIDATES,
	AB_STATE_FORCE_CANDIDATES_SETUP,
	AB_STATE_FORCE_CANDIDATES_EXECUTION,
};

typedef struct
{
	CHandle<CTFPlayer> hPlayer;
	bool bSentForceMessage;
} candidate_info_s;

//-----------------------------------------------------------------------------
class CTFAutobalance : public CAutoGameSystemPerFrame
{
public:

	CTFAutobalance();
	~CTFAutobalance();

	virtual char const *Name() { return "CTFAutobalance"; }

	virtual void Shutdown();
	virtual void LevelShutdownPostEntity();

	// called after entities think
	virtual void FrameUpdatePostEntityThink();

private:
	void Reset();
	bool ShouldBeActive() const;
	bool AreTeamsUnbalanced();
	void ForceDeadCandidates();
	void ForceCandidatesSetup();
	void ForceCandidatesExecution();

	bool IsAlreadyCandidate( CTFPlayer *pTFPlayer ) const;
	double GetTeamAutoBalanceScore( int nTeam ) const;
	double GetPlayerAutoBalanceScore( CTFPlayer *pTFPlayer ) const;
	CTFPlayer *FindNextCandidate();
	bool FindCandidates();
	bool ValidateCandidates();

	bool IsOkayToBalancePlayers();
	void PlayerChangeTeam( CTFPlayer *pTFPlayer, bool bFullBonus );

private:
	autobalance_state_t m_eCurrentState;

	int m_iLightestTeam;
	int m_iHeaviestTeam;
	int m_nNeeded;
	float m_flNextStateChange;

	CUtlVector< candidate_info_s > m_vecCandidates;
};

CTFAutobalance *TFAutoBalance();

#endif // TF_AUTOBALANCE_H
