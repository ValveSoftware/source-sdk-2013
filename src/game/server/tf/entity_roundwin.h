//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTeamplayRoundWin Entity
//
//=============================================================================//
#ifndef ENTITY_ROUND_WIN_H
#define ENTITY_ROUND_WIN_H

#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// CTeamplayRoundWin Entity.
//

class CTeamplayRoundWin : public CPointEntity 
{
public:
	DECLARE_CLASS( CTeamplayRoundWin, CPointEntity );

	CTeamplayRoundWin();

	// Input
	void InputRoundWin( inputdata_t &inputdata );

private:

	void RoundWin( void );

private:

	bool m_bForceMapReset;
	bool m_bSwitchTeamsOnWin;
	int	 m_iWinReason;

	COutputEvent m_outputOnRoundWin; // Fired when the entity tells the game rules a team has won the round

	DECLARE_DATADESC();
};

#endif // ENTITY_ROUND_WIN_H


