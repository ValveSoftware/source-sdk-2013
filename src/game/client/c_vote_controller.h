//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client VoteController
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_VoteController_H
#define C_VoteController_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"
#include "GameEventListener.h"

class C_VoteController : public C_BaseEntity, public CGameEventListener
{
	DECLARE_CLASS( C_VoteController, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_VoteController();
	virtual ~C_VoteController();

	virtual void	Spawn( void );
	virtual void	ClientThink( void );

	static void		RecvProxy_VoteType( const CRecvProxyData *pData, void *pStruct, void *pOut );
	static void		RecvProxy_VoteOption( const CRecvProxyData *pData, void *pStruct, void *pOut );

	void			FireGameEvent( IGameEvent *event );
protected:
	void			ResetData();

	int				m_iActiveIssueIndex;
	int				m_iOnlyTeamToVote;
	int				m_nVoteOptionCount[MAX_VOTE_OPTIONS];
	int				m_iVoteChoiceIndex;
	int				m_nPotentialVotes;
	bool			m_bVotesDirty;	// Received a vote, so remember to tell the Hud
	bool			m_bTypeDirty;	// Vote type changed, so show or hide the Hud
	bool			m_bIsYesNoVote;
	int				m_nVoteIdx;
};

#endif // C_VoteController_H
