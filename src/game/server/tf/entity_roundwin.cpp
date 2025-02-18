//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "entity_roundwin.h"
#include "teamplayroundbased_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTeamplayRoundWin tables.
//
BEGIN_DATADESC( CTeamplayRoundWin )

	DEFINE_KEYFIELD( m_bForceMapReset, FIELD_BOOLEAN, "force_map_reset" ),
	DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "switch_teams" ),
	DEFINE_KEYFIELD( m_iWinReason, FIELD_INTEGER, "win_reason" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTeam", InputSetTeam ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundWin", InputRoundWin ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnRoundWin, "OnRoundWin" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( game_round_win, CTeamplayRoundWin );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTeamplayRoundWin::CTeamplayRoundWin()
{
	// default win reason for map-fired event (map may change it)
	m_iWinReason = WINREASON_DEFEND_UNTIL_TIME_LIMIT;	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTeamplayRoundWin::RoundWin( void )
{
    CTeamplayRoundBasedRules *pGameRules = dynamic_cast<CTeamplayRoundBasedRules *>( GameRules() );

	if ( pGameRules )
	{
		int iTeam = GetTeamNumber();

		if ( iTeam > LAST_SHARED_TEAM )
		{
			if ( !m_bForceMapReset )
			{
				pGameRules->SetWinningTeam( iTeam, m_iWinReason, m_bForceMapReset );
			}
			else
			{
				pGameRules->SetWinningTeam( iTeam, m_iWinReason, m_bForceMapReset, m_bSwitchTeamsOnWin );
			}
		}
		else
		{
			pGameRules->SetStalemate( STALEMATE_TIMER, m_bForceMapReset, m_bSwitchTeamsOnWin );
		}
	}

	// Output.
	m_outputOnRoundWin.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTeamplayRoundWin::InputRoundWin( inputdata_t &inputdata )
{
	RoundWin();
}

