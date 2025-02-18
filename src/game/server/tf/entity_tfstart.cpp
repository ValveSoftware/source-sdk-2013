//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf/tf_shareddefs.h"
#include "entity_tfstart.h"
#include "team_control_point.h"
#include "team_control_point_round.h"
#include "team_objectiveresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// CTFTeamSpawn tables.
//
BEGIN_DATADESC( CTFTeamSpawn )

	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszControlPointName, FIELD_STRING, "controlpoint" ),
	DEFINE_KEYFIELD( m_iszRoundBlueSpawn, FIELD_STRING, "round_bluespawn" ),
	DEFINE_KEYFIELD( m_iszRoundRedSpawn, FIELD_STRING, "round_redspawn" ),
	DEFINE_KEYFIELD( m_nSpawnMode, FIELD_INTEGER, "SpawnMode" ),
	DEFINE_KEYFIELD( m_nMatchSummaryType, FIELD_INTEGER, "MatchSummary" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	// Outputs.

END_DATADESC()

IMPLEMENT_AUTO_LIST( ITFTeamSpawnAutoList );

LINK_ENTITY_TO_CLASS( info_player_teamspawn, CTFTeamSpawn );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFTeamSpawn::CTFTeamSpawn()
{
	m_bDisabled = false;
	m_nMatchSummaryType = PlayerTeamSpawn_MatchSummary_None;
	m_bAlreadyUsedForMatchSummary = false;

	AddEFlags( EFL_FORCE_ALLOW_MOVEPARENT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamSpawn::Activate( void )
{
	BaseClass::Activate();

	Vector mins = VEC_HULL_MIN;
	Vector maxs = VEC_HULL_MAX;

	trace_t trace;
	UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin(), mins, maxs, MASK_PLAYERSOLID, NULL, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	bool bClear = ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
	if ( !bClear )
	{
		DevMsg("Spawnpoint at (%.2f %.2f %.2f) is not clear.\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		// m_debugOverlays |= OVERLAY_TEXT_BIT;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CTFTeamSpawn::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"TeamNumber: %d", GetTeamNumber() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		color32 teamcolor = g_aTeamColors[ GetTeamNumber() ];
		teamcolor.a = 0;

		if ( m_bDisabled )
		{
			Q_snprintf(tempstr,sizeof(tempstr),"DISABLED" );
			EntityText(text_offset,tempstr,0);
			text_offset++;

			teamcolor.a = 255;
		}

		// Make sure it's empty
		Vector mins = VEC_HULL_MIN;
		Vector maxs = VEC_HULL_MAX;

		Vector vTestMins = GetAbsOrigin() + mins;
		Vector vTestMaxs = GetAbsOrigin() + maxs;

		// First test the starting origin.
		if ( UTIL_IsSpaceEmpty( NULL, vTestMins, vTestMaxs ) )
		{
			NDebugOverlay::Box( GetAbsOrigin(), mins, maxs, teamcolor.r, teamcolor.g, teamcolor.b, teamcolor.a, 0.1);
		}
		else
		{
			NDebugOverlay::Box( GetAbsOrigin(), mins, maxs, 0,255,0, 0, 0.1);
		}

		if ( m_hControlPoint )
		{
			NDebugOverlay::Line( GetAbsOrigin(), m_hControlPoint->GetAbsOrigin(), teamcolor.r, teamcolor.g, teamcolor.b, false, 0.1 );
		}
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamSpawn::InputRoundSpawn( inputdata_t &input )
{
	if ( m_iszControlPointName != NULL_STRING )
	{
		// We need to re-find our control point, because they're recreated over round restarts
		m_hControlPoint = dynamic_cast<CTeamControlPoint*>( gEntList.FindEntityByName( NULL, m_iszControlPointName ) );
		if ( !m_hControlPoint )
		{
			Warning("%s failed to find control point named '%s'\n", GetClassname(), STRING(m_iszControlPointName) );
		}
	}

	if ( m_iszRoundBlueSpawn != NULL_STRING )
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundBlueSpawn = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, m_iszRoundBlueSpawn ) );
		if ( !m_hRoundBlueSpawn )
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundBlueSpawn) );
		}
	}

	if ( m_iszRoundRedSpawn != NULL_STRING )
	{
		// We need to re-find our control point round, because they're recreated over round restarts
		m_hRoundRedSpawn = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, m_iszRoundRedSpawn ) );
		if ( !m_hRoundRedSpawn )
		{
			Warning("%s failed to find control point round named '%s'\n", GetClassname(), STRING(m_iszRoundRedSpawn) );
		}
	}
}
