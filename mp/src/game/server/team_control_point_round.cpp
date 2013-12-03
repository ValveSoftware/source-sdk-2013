//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "team_control_point_master.h"
#include "teamplayroundbased_gamerules.h"
#include "team_control_point_round.h"

#if defined ( TF_DLL )
#include "tf_gamerules.h"
#endif

BEGIN_DATADESC( CTeamControlPointRound )
	DEFINE_KEYFIELD( m_bDisabled,			FIELD_BOOLEAN,	"StartDisabled" ),

	DEFINE_KEYFIELD( m_iszCPNames,			FIELD_STRING,	"cpr_cp_names" ),
	DEFINE_KEYFIELD( m_nPriority,			FIELD_INTEGER,	"cpr_priority" ),
	DEFINE_KEYFIELD( m_iInvalidCapWinner,	FIELD_INTEGER,	"cpr_restrict_team_cap_win" ),
	DEFINE_KEYFIELD( m_iszPrintName,		FIELD_STRING,	"cpr_printname" ),
//	DEFINE_FIELD( m_ControlPoints, CUtlVector < CHandle < CTeamControlPoint > > ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),

	DEFINE_OUTPUT( m_OnStart,		"OnStart" ),
	DEFINE_OUTPUT( m_OnEnd,			"OnEnd" ),
	DEFINE_OUTPUT( m_OnWonByTeam1,	"OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2,	"OnWonByTeam2" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( team_control_point_round, CTeamControlPointRound );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::Spawn( void )
{
	SetTouch( NULL );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::Activate( void )
{
	BaseClass::Activate();

	FindControlPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::FindControlPoints( void )
{
	// Let out control point masters know that the round has started
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

	if ( pMaster )
	{
		// go through all the points
		CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, pMaster->GetControlPointName() );
		
		while( pEnt )
		{
			CTeamControlPoint *pPoint = assert_cast<CTeamControlPoint *>(pEnt);

			if ( pPoint )
			{
				const char *pString = STRING( m_iszCPNames );
				const char *pName = STRING( pPoint->GetEntityName() );

				// HACK to work around a problem with cp_a being returned for an entity name with cp_A
				const char *pos = Q_stristr( pString, pName );
				if ( pos )
				{
					int len = Q_strlen( STRING( pPoint->GetEntityName() ) );
					if ( *(pos + len) == ' ' || *(pos + len) == '\0' )
					{
						if( m_ControlPoints.Find( pPoint ) == m_ControlPoints.InvalidIndex() )
						{
							DevMsg( 2, "Adding control point %s to control point round %s\n", pPoint->GetEntityName().ToCStr(), GetEntityName().ToCStr() );
							m_ControlPoints.AddToHead( pPoint );
						}
					}
				}
			}

			pEnt = gEntList.FindEntityByClassname( pEnt, pMaster->GetControlPointName() );
		}
	}

	if( m_ControlPoints.Count() == 0 ) 
	{
		Warning( "Error! No control points found in map for team_game_round %s!\n", GetEntityName().ToCStr() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check that the points aren't all held by one team if they are 
//			this will reset the round and will reset all the points
//-----------------------------------------------------------------------------
int CTeamControlPointRound::CheckWinConditions( void )
{
	int iWinners = TeamOwnsAllPoints();
	if ( ( m_iInvalidCapWinner != 1 ) &&
		 ( iWinners >= FIRST_GAME_TEAM ) && 
		 ( iWinners != m_iInvalidCapWinner ) )
	{
		bool bWinner = true;

#if defined( TF_DLL)
		if ( TFGameRules() && TFGameRules()->IsInKothMode() )
		{
			CTeamRoundTimer *pTimer = NULL;
			if ( iWinners == TF_TEAM_RED )
			{
				pTimer = TFGameRules()->GetRedKothRoundTimer();
			}
			else if ( iWinners == TF_TEAM_BLUE )
			{
				pTimer = TFGameRules()->GetBlueKothRoundTimer();
			}

			if ( pTimer )
			{
				if ( pTimer->GetTimeRemaining() > 0 || TFGameRules()->TimerMayExpire() == false )
				{
					bWinner = false;
				}
			}
		}
#endif
		if ( bWinner )
		{
			FireTeamWinOutput( iWinners );
			return iWinners;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::FireTeamWinOutput( int iWinningTeam )
{
	// Remap team so that first game team = 1
	switch( iWinningTeam - FIRST_GAME_TEAM+1 )
	{
	case 1:
		m_OnWonByTeam1.FireOutput( this, this );
		break;
	case 2:
		m_OnWonByTeam2.FireOutput( this, this );
		break;
	default:
		Assert(0);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPointRound::GetPointOwner( int point )
{
	Assert( point >= 0 );
	Assert( point < MAX_CONTROL_POINTS );

	CTeamControlPoint *pPoint = m_ControlPoints[point];

	if ( pPoint )
		return pPoint->GetOwner();

	return TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: This function returns the team that owns all the cap points. 
//			If its not the case that one team owns them all, it returns 0.
//			
//			Can be passed an overriding team. If this is not null, the passed team
//			number will be used for that cp. Used to predict if that CP changing would
//			win the game.
//-----------------------------------------------------------------------------
int CTeamControlPointRound::TeamOwnsAllPoints( CTeamControlPoint *pOverridePoint /* = NULL */, int iOverrideNewTeam /* = TEAM_UNASSIGNED */ )
{
	int i;

	int iWinningTeam[MAX_CONTROL_POINT_GROUPS];

	for( i = 0 ; i < MAX_CONTROL_POINT_GROUPS ; i++ )
	{
		iWinningTeam[i] = TEAM_INVALID;
	}

	// if TEAM_INVALID, haven't found a flag for this group yet
	// if TEAM_UNASSIGNED, the group is still being contested 

	// for each control point
	for( i = 0 ; i < m_ControlPoints.Count() ; i++ )
	{
		int group = m_ControlPoints[i]->GetCPGroup();
		int owner = m_ControlPoints[i]->GetOwner();

		if ( pOverridePoint == m_ControlPoints[i] )
		{
			owner = iOverrideNewTeam;
		}

		// the first one we find in this group, set the win to true
		if ( iWinningTeam[group] == TEAM_INVALID )
		{
			iWinningTeam[group] = owner;
		}
		// unassigned means this group is already contested, move on
		else if ( iWinningTeam[group] == TEAM_UNASSIGNED )
		{
			continue;
		}
		// if we find another one in the group that isn't the same owner, set the win to false
		else if ( owner != iWinningTeam[group] )
		{
			iWinningTeam[group] = TEAM_UNASSIGNED;
		}		
	}

	// report the first win we find as the winner
	for ( i = 0 ; i < MAX_CONTROL_POINT_GROUPS ; i++ )
	{
		if ( iWinningTeam[i] >= FIRST_GAME_TEAM )
			return iWinningTeam[i];
	}

	// no wins yet
	return TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointRound::WouldNewCPOwnerWinGame( CTeamControlPoint *pPoint, int iNewOwner )
{
	return ( TeamOwnsAllPoints( pPoint, iNewOwner ) == iNewOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::InputEnable( inputdata_t &input )
{ 
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::InputDisable( inputdata_t &input )
{ 
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::InputRoundSpawn( inputdata_t &input )
{
	// clear out old control points
	m_ControlPoints.RemoveAll();

	// find the control points
	FindControlPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::SetupSpawnPoints( void )
{
	CTeamplayRoundBasedRules *pRules = TeamplayRoundBasedRules();

	if ( pRules )
	{
		pRules->SetupSpawnPointsForRound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::SelectedToPlay( void )
{
	SetupSpawnPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::FireOnStartOutput( void )
{
	m_OnStart.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointRound::FireOnEndOutput( void )
{
	m_OnEnd.FireOutput( this, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointRound::IsControlPointInRound( CTeamControlPoint *pPoint )
{
	if ( !pPoint )
	{
		return false;
	}

	return ( m_ControlPoints.Find( pPoint ) != m_ControlPoints.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointRound::IsPlayable( void )
{
	int iWinners = TeamOwnsAllPoints();

	if (  m_iInvalidCapWinner == 1 ) // neither team can win this round by capping
	{
		return true;
	}

	if ( ( iWinners >= FIRST_GAME_TEAM ) && 
		 ( iWinners != m_iInvalidCapWinner ) )
	{
		return false; // someone has already won this round
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointRound::MakePlayable( void )
{
	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		if ( !IsPlayable() )
		{
			// we need to try switching the owners of the teams to make this round playable
			for ( int iTeam = FIRST_GAME_TEAM ; iTeam < GetNumberOfTeams() ; iTeam++ )
			{
				for ( int iControlPoint = 0 ; iControlPoint < m_ControlPoints.Count() ; iControlPoint++ )
				{
					if ( ( !pMaster->IsBaseControlPoint( m_ControlPoints[iControlPoint]->GetPointIndex() ) ) && // this is NOT the base point for one of the teams (we don't want to assign the base to the wrong team)
						 ( !WouldNewCPOwnerWinGame( m_ControlPoints[iControlPoint], iTeam ) ) ) // making this change would make this round playable
					{
						// need to find the trigger area associated with this point
						for ( int iObj=0; iObj<ITriggerAreaCaptureAutoList::AutoList().Count(); ++iObj )
						{
							CTriggerAreaCapture *pArea = static_cast< CTriggerAreaCapture * >( ITriggerAreaCaptureAutoList::AutoList()[iObj] );	
							if ( pArea->TeamCanCap( iTeam ) )
							{
								CHandle<CTeamControlPoint> hPoint = pArea->GetControlPoint();
								if ( hPoint == m_ControlPoints[iControlPoint] )
								{
									// found! 
									pArea->ForceOwner( iTeam ); // this updates the trigger_area *and* the control_point
									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: returns the first point found that the given team owns
//-----------------------------------------------------------------------------
CHandle<CTeamControlPoint> CTeamControlPointRound::GetPointOwnedBy( int iTeam )
{
	for( int i = 0 ; i < m_ControlPoints.Count() ; i++ )
	{
		if ( m_ControlPoints[i]->GetOwner() == iTeam )
		{
			return m_ControlPoints[i];
		}
	}

	return NULL;
}
