//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "team_objectiveresource.h"
#include "team_control_point_master.h"
#include "teamplayroundbased_gamerules.h"

#if defined ( TF_DLL )
#include "tf_gamerules.h"
#endif

BEGIN_DATADESC( CTeamControlPointMaster )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszCapLayoutInHUD, FIELD_STRING, "caplayout" ),
	DEFINE_KEYFIELD( m_iInvalidCapWinner, FIELD_INTEGER, "cpm_restrict_team_cap_win" ),
	DEFINE_KEYFIELD( m_bSwitchTeamsOnWin, FIELD_BOOLEAN, "switch_teams" ),
	DEFINE_KEYFIELD( m_bScorePerCapture, FIELD_BOOLEAN, "score_style" ),
	DEFINE_KEYFIELD( m_bPlayAllRounds, FIELD_BOOLEAN, "play_all_rounds" ),

	DEFINE_KEYFIELD( m_flPartialCapturePointsRate, FIELD_FLOAT, "partial_cap_points_rate" ),

	DEFINE_KEYFIELD( m_flCustomPositionX, FIELD_FLOAT, "custom_position_x" ),
	DEFINE_KEYFIELD( m_flCustomPositionY, FIELD_FLOAT, "custom_position_y" ),

//	DEFINE_FIELD( m_ControlPoints, CUtlMap < int , CTeamControlPoint * > ),
//	DEFINE_FIELD( m_bFoundPoints, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_ControlPointRounds, CUtlVector < CTeamControlPointRound * > ),
//	DEFINE_FIELD( m_iCurrentRoundIndex, FIELD_INTEGER ),
//	DEFINE_ARRAY( m_iszTeamBaseIcons, FIELD_STRING, MAX_TEAMS ),
//	DEFINE_ARRAY( m_iTeamBaseIcons, FIELD_INTEGER, MAX_TEAMS ),
//	DEFINE_FIELD( m_bFirstRoundAfterRestart, FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetWinner", InputSetWinner ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetWinnerAndForceCaps", InputSetWinnerAndForceCaps ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundSpawn", InputRoundSpawn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetCapLayout", InputSetCapLayout ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCapLayoutCustomPositionX", InputSetCapLayoutCustomPositionX ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetCapLayoutCustomPositionY", InputSetCapLayoutCustomPositionY ),

	DEFINE_FUNCTION( CPMThink ),

	DEFINE_OUTPUT( m_OnWonByTeam1,	"OnWonByTeam1" ),
	DEFINE_OUTPUT( m_OnWonByTeam2,	"OnWonByTeam2" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( team_control_point_master, CTeamControlPointMaster );

ConVar mp_time_between_capscoring( "mp_time_between_capscoring", "30", FCVAR_GAMEDLL, "Delay between scoring of owned capture points.", true, 1, false, 0 );

// sort function for the list of control_point_rounds (we're sorting them by priority...highest first)
int ControlPointRoundSort( CTeamControlPointRound* const *p1, CTeamControlPointRound* const *p2 )
{
	// check the priority
	if ( (*p2)->GetPriorityValue() > (*p1)->GetPriorityValue() )
	{
		return 1;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: init
//-----------------------------------------------------------------------------
CTeamControlPointMaster::CTeamControlPointMaster()
{
	m_flPartialCapturePointsRate = 0.0f;
	m_flCustomPositionX = -1.f;
	m_flCustomPositionY = -1.f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::Spawn( void )
{
	Precache();

	SetTouch( NULL );
	m_bFoundPoints = false;
	SetDefLessFunc( m_ControlPoints );

	m_iCurrentRoundIndex = -1;
  	m_bFirstRoundAfterRestart = true;
	m_flLastOwnershipChangeTime = -1;

	BaseClass::Spawn();

	if ( g_hControlPointMasters.Find(this) == g_hControlPointMasters.InvalidIndex() )
	{
		g_hControlPointMasters.AddToTail( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	g_hControlPointMasters.FindAndRemove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::KeyValue( const char *szKeyName, const char *szValue )
{	
	if ( !Q_strncmp( szKeyName, "team_base_icon_", 15 ) )
	{
		int iTeam = atoi(szKeyName+15);
		Assert( iTeam >= 0 && iTeam < MAX_TEAMS );

		m_iszTeamBaseIcons[iTeam] = AllocPooledString(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::Precache( void )
{
	for ( int i = 0; i < MAX_TEAMS; i++ )
	{
		if ( m_iszTeamBaseIcons[i] != NULL_STRING )
		{
			PrecacheMaterial( STRING( m_iszTeamBaseIcons[i] ) );
			m_iTeamBaseIcons[i] = GetMaterialIndex( STRING( m_iszTeamBaseIcons[i] ) );
			Assert( m_iTeamBaseIcons[i] != 0 );
		}
	}

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::Activate( void )
{
	BaseClass::Activate();

	// Find control points right away. This allows client hud elements to know the
	// number & starting state of control points before the game actually starts.
	FindControlPoints();
	FindControlPointRounds();

	SetBaseControlPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::RoundRespawn( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::Reset( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::FindControlPoints( void )
{
	//go through all the points
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, GetControlPointName() );

	int numFound = 0;
	
	while( pEnt )
	{
		CTeamControlPoint *pPoint = assert_cast<CTeamControlPoint *>(pEnt);

		if( pPoint->IsActive() && !pPoint->IsMarkedForDeletion() )
		{
			int index = pPoint->GetPointIndex();

			Assert( index >= 0 );

			if( m_ControlPoints.Find( index ) == m_ControlPoints.InvalidIndex())
			{
				DevMsg( 2, "**** Adding control point %s with index %d to control point master\n", pPoint->GetName(), index );
				m_ControlPoints.Insert( index, pPoint );
				numFound++;
			}
			else
			{
				Warning( "!!!!\nMultiple control points with the same index, duplicates ignored\n!!!!\n" );
				UTIL_Remove( pPoint );
			}
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, GetControlPointName() );
	}

	if( numFound > MAX_CONTROL_POINTS )
	{
		Warning( "Too many control points! Max is %d\n", MAX_CONTROL_POINTS );
	}

	//Remap the indeces of the control points so they are 0-based
	//======================
	unsigned int j;

	bool bHandled[MAX_CONTROL_POINTS];
	memset( bHandled, 0, sizeof(bHandled) );

	unsigned int numPoints = m_ControlPoints.Count();
	unsigned int newIndex = 0;

	while( newIndex < numPoints )
	{
		//Find the lowest numbered, unhandled point
		int lowestIndex = -1;
		int lowestValue = 999;

		//find the lowest unhandled index
		for( j=0; j<numPoints; j++ )
		{
			if( !bHandled[j] && m_ControlPoints[j]->GetPointIndex() < lowestValue )
			{
				lowestIndex = j;
				lowestValue = m_ControlPoints[j]->GetPointIndex();
			}
		}

		//Don't examine this point again
		bHandled[lowestIndex] = true;

		//Give it its new index
		m_ControlPoints[lowestIndex]->SetPointIndex( newIndex );
		newIndex++;
	}
	
	if( m_ControlPoints.Count() == 0 ) 
	{
		Warning( "Error! No control points found in map!\n");
		return false;	
	}

	// Now setup the objective resource
	ObjectiveResource()->SetNumControlPoints( m_ControlPoints.Count() );
	for ( unsigned int i = 0; i < m_ControlPoints.Count(); i++ )
	{
		CTeamControlPoint *pPoint = m_ControlPoints[i];

		int iPointIndex = m_ControlPoints[i]->GetPointIndex();

		ObjectiveResource()->SetOwningTeam( iPointIndex, pPoint->GetOwner() );
		ObjectiveResource()->SetCPVisible( iPointIndex, pPoint->PointIsVisible() );
		ObjectiveResource()->SetCPPosition( iPointIndex, pPoint->GetAbsOrigin() );
		ObjectiveResource()->SetWarnOnCap( iPointIndex, pPoint->GetWarnOnCap() );
		ObjectiveResource()->SetWarnSound( iPointIndex, pPoint->GetWarnSound() );
		ObjectiveResource()->SetCPGroup( iPointIndex, pPoint->GetCPGroup() );
		for ( int team = 0; team < GetNumberOfTeams(); team++ )
		{
			ObjectiveResource()->SetCPIcons( iPointIndex, team, pPoint->GetHudIconIndexForTeam(team) );
			ObjectiveResource()->SetCPOverlays( iPointIndex, team, pPoint->GetHudOverlayIndexForTeam(team) );
			for ( int prevpoint = 0; prevpoint < MAX_PREVIOUS_POINTS; prevpoint++ )
			{
				ObjectiveResource()->SetPreviousPoint( iPointIndex, team, prevpoint, pPoint->GetPreviousPointForTeam(team, prevpoint) );
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::SetBaseControlPoints( void )
{
	for ( int team = 0; team < GetNumberOfTeams(); team++ )
	{
		ObjectiveResource()->SetTeamBaseIcons( team, m_iTeamBaseIcons[team] );
		ObjectiveResource()->SetBaseCP( GetBaseControlPoint(team), team );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::FindControlPointRounds( void )
{
	bool bFoundRounds = false;

	m_ControlPointRounds.RemoveAll();
	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, GetControlPointRoundName() );

	while( pEnt )
	{
		CTeamControlPointRound *pRound = assert_cast<CTeamControlPointRound *>( pEnt );

		if( pRound && ( m_ControlPointRounds.Find( pRound ) == m_ControlPointRounds.InvalidIndex() ) )
		{
			DevMsg( 2, "**** Adding control point round %s to control point master\n", pRound->GetEntityName().ToCStr() );
			m_ControlPointRounds.AddToHead( pRound );
		}

		pEnt = gEntList.FindEntityByClassname( pEnt, GetControlPointRoundName() );
	}

	if ( m_ControlPointRounds.Count() > 0 ) 
	{
		// sort them in our list by priority (highest priority first)
		m_ControlPointRounds.Sort( ControlPointRoundSort );
		bFoundRounds = true;
	}

	if ( g_pObjectiveResource )
	{
		g_pObjectiveResource->SetPlayingMiniRounds( bFoundRounds );
		g_pObjectiveResource->SetCapLayoutInHUD( STRING(m_iszCapLayoutInHUD) );
		g_pObjectiveResource->SetCapLayoutCustomPosition( m_flCustomPositionX, m_flCustomPositionY );
	}

	return bFoundRounds;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::IsInRound( CTeamControlPoint *pPoint )
{
	// are we playing a round and is this point in the round?
	if ( m_ControlPointRounds.Count() > 0 && m_iCurrentRoundIndex != -1 )
	{
		return m_ControlPointRounds[m_iCurrentRoundIndex]->IsControlPointInRound( pPoint );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTeamControlPointMaster::NumPlayableControlPointRounds( void )
{
	int nRetVal = 0;

	for ( int i = 0 ; i < m_ControlPointRounds.Count() ; ++i )
	{
		CTeamControlPointRound *pRound = m_ControlPointRounds[i];

		if ( pRound )
		{
			if ( pRound->IsPlayable() )
			{
				// we found one that's playable
				nRetVal++;
			}
		}
	}

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::SelectSpecificRound( void )
{
	CTeamControlPointRound *pRound = NULL;
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );

	if ( pRules )
	{
		if ( pRules->GetRoundToPlayNext() != NULL_STRING )
		{
			// do we have the name of a round?
			pRound = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, STRING( pRules->GetRoundToPlayNext() ) ) );

			if ( pRound )
			{
				if ( ( m_ControlPointRounds.Find( pRound )== m_ControlPointRounds.InvalidIndex() ) || 
					 ( !pRound->IsPlayable() && !pRound->MakePlayable() ) )
				{
					pRound = NULL;
				}
			}

			pRules->SetRoundToPlayNext( NULL_STRING );
		}
	}

	// do we have a round to play?
	if ( pRound )
	{
		m_iCurrentRoundIndex = m_ControlPointRounds.Find( pRound );
		m_ControlPointRounds[m_iCurrentRoundIndex]->SelectedToPlay();
		
		if ( pRules )
		{
			pRules->SetRoundOverlayDetails();
		}

		FireRoundStartOutput();
		DevMsg( 2, "**** Selected round %s to play\n", m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName().ToCStr() );

		if ( !pRules->IsInWaitingForPlayers() )
		{
			UTIL_LogPrintf( "World triggered \"Mini_Round_Selected\" (round \"%s\")\n", m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName().ToCStr() );
			UTIL_LogPrintf( "World triggered \"Mini_Round_Start\"\n" );
		}
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::RegisterRoundBeingPlayed( void )
{
	// let the game rules know what round we're playing
	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	if ( pRules )
	{
		string_t iszEntityName = m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName();

		pRules->AddPlayedRound( iszEntityName );

		if ( m_bFirstRoundAfterRestart )
		{
			pRules->SetFirstRoundPlayed( iszEntityName );
			m_bFirstRoundAfterRestart = false;
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_round_selected" );	
	if ( event )
	{
		event->SetString( "round", m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName().ToCStr() );
		gameeventmanager->FireEvent( event );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::GetControlPointRoundToPlay( void )
{
	int i = 0;

	// are we trying to pick a specific round?
	if ( SelectSpecificRound() )
	{
		SetBaseControlPoints();
		RegisterRoundBeingPlayed();
		return true;
	}

	// rounds are sorted with the higher priority rounds first
	for ( i = 0 ; i < m_ControlPointRounds.Count() ; ++i )
	{
		CTeamControlPointRound *pRound = m_ControlPointRounds[i];

		if ( pRound )
		{
			if ( pRound->IsPlayable() )
			{
				// we found one that's playable
				break;
			}
		}
	}

	if ( i >= m_ControlPointRounds.Count() || m_ControlPointRounds[i] == NULL )
	{
		// we didn't find one to play
		m_iCurrentRoundIndex = -1;
		return false;
	}

    // we have a priority value, now we need to randomly pick a round with this priority that's playable
	int nPriority = m_ControlPointRounds[i]->GetPriorityValue();
	CUtlVector<int> nRounds;

	CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
	string_t iszLastRoundPlayed = pRules ? pRules->GetLastPlayedRound() : NULL_STRING;
	int iLastRoundPlayed = -1;

	string_t iszFirstRoundPlayed = pRules ? pRules->GetFirstRoundPlayed() : NULL_STRING;
	int iFirstRoundPlayed = -1; // after a full restart

	// loop through and find the rounds with this priority value
	for ( i = 0 ; i < m_ControlPointRounds.Count() ; ++i )
	{
		CTeamControlPointRound *pRound = m_ControlPointRounds[i];

		if ( pRound )
		{
			string_t iszRoundName = pRound->GetEntityName();

			if ( pRound->IsPlayable() && pRound->GetPriorityValue() == nPriority )
			{
				if ( iszLastRoundPlayed == iszRoundName ) // is this the last round we played?
				{
					iLastRoundPlayed = i;
				}

				if ( m_bFirstRoundAfterRestart )
				{
					// is this the first round we played after the last full restart?
					if ( ( iszFirstRoundPlayed != NULL_STRING ) && ( iszFirstRoundPlayed == iszRoundName ) )
					{
						iFirstRoundPlayed = i;
					}
				}

				nRounds.AddToHead(i);
			}
		}
	}

	if ( nRounds.Count() <= 0 )
	{
		// we didn't find one to play
		m_iCurrentRoundIndex = -1;
		return false;
	}

	// if we have more than one and the last played round is in our list, remove it
	if ( nRounds.Count() > 1 )
	{
		if ( iLastRoundPlayed != -1 )
		{
			int elementIndex = nRounds.Find( iLastRoundPlayed );
			nRounds.Remove( elementIndex );
		}
	}

	// if this is the first round after a full restart, we still have more than one round in our list, 
	// and the first played round (after the last full restart) is in our list, remove it
	if ( m_bFirstRoundAfterRestart )
	{
		if ( nRounds.Count() > 1 )
		{
			if ( iFirstRoundPlayed != -1 )
			{
				int elementIndex = nRounds.Find( iFirstRoundPlayed );
				nRounds.Remove( elementIndex );
			}
		}
	}

	// pick one to play but try to avoid picking one that we have recently played if there are other rounds to play
	int index = random->RandomInt( 0, nRounds.Count() - 1 );
	
	// only need to check this if we have more than one round with this priority value
	if ( pRules && nRounds.Count() > 1 )
	{
		// keep picking a round until we find one that's not a previously played round
		// or until we don't have any more rounds to choose from
		while ( pRules->IsPreviouslyPlayedRound( m_ControlPointRounds[ nRounds[ index ] ]->GetEntityName() ) && 
				nRounds.Count() > 1 )
		{
			nRounds.Remove( index ); // we have played this round recently so get it out of the list
			index = random->RandomInt( 0, nRounds.Count() - 1 );
		}
	}

	// pick one to play and fire its OnSelected output
	m_iCurrentRoundIndex = nRounds[ index ];
	m_ControlPointRounds[m_iCurrentRoundIndex]->SelectedToPlay();

	if ( pRules )
	{
		pRules->SetRoundOverlayDetails();
	}

	FireRoundStartOutput();
	DevMsg( 2, "**** Selected round %s to play\n", m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName().ToCStr() );

	if ( !pRules->IsInWaitingForPlayers() )
	{
		UTIL_LogPrintf( "World triggered \"Mini_Round_Selected\" (round \"%s\")\n", m_ControlPointRounds[m_iCurrentRoundIndex]->GetEntityName().ToCStr() );
		UTIL_LogPrintf( "World triggered \"Mini_Round_Start\"\n" );
	}

	SetBaseControlPoints();
	RegisterRoundBeingPlayed();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called every 0.1 seconds and checks the status of all the control points
// if one team owns them all, it gives points and resets
// Think also gives the time based points at the specified time intervals
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::CPMThink( void )
{
	if ( m_bDisabled || !TeamplayGameRules()->PointsMayBeCaptured() )
	{
		SetContextThink( &CTeamControlPointMaster::CPMThink, gpGlobals->curtime + 0.2, CPM_THINK );
		return;
	}

	// If we call this from team_control_point, this function should never 
	// trigger a win. but we'll leave it here just in case.
	CheckWinConditions();

	// the next time we 'think'
	SetContextThink( &CTeamControlPointMaster::CPMThink, gpGlobals->curtime + 0.2, CPM_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::CheckWinConditions( void )
{
	if ( m_bDisabled )
		return;

	if ( m_ControlPointRounds.Count() > 0 )
	{
		if ( m_iCurrentRoundIndex != -1 )
		{
			// Check the current round to see if one team is a winner yet
			int iWinners = m_ControlPointRounds[m_iCurrentRoundIndex]->CheckWinConditions();
			if ( iWinners != -1 && iWinners >= FIRST_GAME_TEAM )
			{
				bool bForceMapReset = ( NumPlayableControlPointRounds() == 0 ); // are there any more rounds to play?

				if ( !bForceMapReset )
				{
					// we have more rounds to play
					TeamplayGameRules()->SetWinningTeam( iWinners, WINREASON_ALL_POINTS_CAPTURED, bForceMapReset );
				}
				else
				{
					// we have played all of the available rounds
					TeamplayGameRules()->SetWinningTeam( iWinners, WINREASON_ALL_POINTS_CAPTURED, bForceMapReset, m_bSwitchTeamsOnWin );
				}

				FireTeamWinOutput( iWinners );
			}
		}
	}
	else
	{
		// Check that the points aren't all held by one team...if they are
		// this will reset the round and will reset all the points
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
				TeamplayGameRules()->SetWinningTeam( iWinners, WINREASON_ALL_POINTS_CAPTURED, true, m_bSwitchTeamsOnWin );
				FireTeamWinOutput( iWinners );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputSetWinner( inputdata_t &input )
{
	int iTeam = input.value.Int();
	InternalSetWinner( iTeam );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputSetWinnerAndForceCaps( inputdata_t &input )
{
	int iTeam = input.value.Int();

	// Set all cap points in the current round to be owned by the winning team
	for ( unsigned int i = 0; i < m_ControlPoints.Count(); i++ )
	{
		CTeamControlPoint *pPoint = m_ControlPoints[i];
		if ( pPoint && (!PlayingMiniRounds() || ObjectiveResource()->IsInMiniRound(pPoint->GetPointIndex()) ) )
		{
			pPoint->ForceOwner( iTeam );
		}
	}

	InternalSetWinner( iTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InternalSetWinner( int iTeam )
{
	bool bForceMapReset = true;

	if ( m_ControlPointRounds.Count() > 0 )
	{
		// if we're playing rounds and there are more to play, don't do a full reset
		bForceMapReset = ( NumPlayableControlPointRounds() == 0 );
	}

	if ( iTeam == TEAM_UNASSIGNED )
	{
		TeamplayGameRules()->SetStalemate( STALEMATE_TIMER, bForceMapReset );
	}
	else
	{
		if ( !bForceMapReset )
		{
			TeamplayGameRules()->SetWinningTeam( iTeam, WINREASON_ALL_POINTS_CAPTURED, bForceMapReset );
		}
		else
		{
			TeamplayGameRules()->SetWinningTeam( iTeam, WINREASON_ALL_POINTS_CAPTURED, bForceMapReset, m_bSwitchTeamsOnWin );
		}

		FireTeamWinOutput( iTeam );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::HandleRandomOwnerControlPoints( void )
{
	CUtlVector<CTeamControlPoint*> vecPoints; 
	CUtlVector<int> vecTeams; 

	int i = 0;

	// loop through and find all of the points that want random owners after a full restart
	for ( i = 0 ; i < (int)m_ControlPoints.Count() ; i++ )
	{
		CTeamControlPoint *pPoint = m_ControlPoints[i];

		if ( pPoint && pPoint->RandomOwnerOnRestart() )
		{
			vecPoints.AddToHead( pPoint );
			vecTeams.AddToHead( pPoint->GetTeamNumber() );
		}
	}

	// now loop through and mix up the owners (if we found any points with this flag set)
	for ( i = 0 ; i < vecPoints.Count() ; i++ )
	{
		CTeamControlPoint *pPoint = vecPoints[i];

		if ( pPoint )
		{
			int index = random->RandomInt( 0, vecTeams.Count() - 1 );
			pPoint->ForceOwner( vecTeams[index] );

			vecTeams.Remove( index );
		}
	}

	vecPoints.RemoveAll();
	vecTeams.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputRoundSpawn( inputdata_t &input )
{
	//clear out old control points
	m_ControlPoints.RemoveAll();

	//find the control points, and if successful, do CPMThink
	if ( FindControlPoints() )
	{
/*		if ( m_bFirstRoundAfterRestart )
		{
			CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
			if ( pRules && ( pRules->GetRoundToPlayNext() == NULL_STRING ) )
			{
				// we only want to handle the random points if we don't have a specific round to play next
				// (prevents points being randomized again after "waiting for players" has finished and we're going to play the same round)
				HandleRandomOwnerControlPoints();
			}
		}
*/
		SetContextThink( &CTeamControlPointMaster::CPMThink, gpGlobals->curtime + 0.1, CPM_THINK );
	}

	// clear out the old rounds
	m_ControlPointRounds.RemoveAll();

	// find the rounds (if the map has any)
	FindControlPointRounds();

	SetBaseControlPoints();
	
	ObjectiveResource()->ResetControlPoints();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputRoundActivate( inputdata_t &input )
{
	// if we're using mini-rounds and haven't picked one yet, find one to play
	if ( PlayingMiniRounds() && GetCurrentRound() == NULL )
	{
		GetControlPointRoundToPlay();
	}

	if ( PlayingMiniRounds() )
	{
		// Tell the objective resource what control points are in use in the selected mini-round
		CTeamControlPointRound *pRound = GetCurrentRound();
		if ( pRound )
		{
			for ( unsigned int i = 0; i < m_ControlPoints.Count(); i++ )
			{
				CTeamControlPoint *pPoint = m_ControlPoints[i];
				if ( pPoint )
				{
					ObjectiveResource()->SetInMiniRound( pPoint->GetPointIndex(), pRound->IsControlPointInRound( pPoint ) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputSetCapLayout( inputdata_t &inputdata )
{
	m_iszCapLayoutInHUD = inputdata.value.StringID();
	g_pObjectiveResource->SetCapLayoutInHUD( STRING(m_iszCapLayoutInHUD) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputSetCapLayoutCustomPositionX( inputdata_t &inputdata )
{
	m_flCustomPositionX = inputdata.value.Float();
	g_pObjectiveResource->SetCapLayoutCustomPosition( m_flCustomPositionX, m_flCustomPositionY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputSetCapLayoutCustomPositionY( inputdata_t &inputdata )
{
	m_flCustomPositionY = inputdata.value.Float();
	g_pObjectiveResource->SetCapLayoutCustomPosition( m_flCustomPositionX, m_flCustomPositionY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::FireTeamWinOutput( int iWinningTeam )
{
	// Remap team so that first game team = 1
	switch( iWinningTeam - FIRST_GAME_TEAM+1 )
	{
	case 1:
		m_OnWonByTeam1.FireOutput(this,this);
		break;
	case 2:
		m_OnWonByTeam2.FireOutput(this,this);
		break;
	default:
		Assert(0);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::FireRoundStartOutput( void )
{
	CTeamControlPointRound *pRound = GetCurrentRound();

	if ( pRound )
	{
		pRound->FireOnStartOutput();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::FireRoundEndOutput( void )
{
	CTeamControlPointRound *pRound = GetCurrentRound();

	if ( pRound )
	{
		pRound->FireOnEndOutput();
		m_iCurrentRoundIndex = -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CTeamControlPointRound* CTeamControlPointMaster::GetRoundByIndex( int nIndex ) const
{
	if ( nIndex < 0 || nIndex >= m_ControlPointRounds.Count() )
	{
		Assert( false );
		return 0;
	}

	return m_ControlPointRounds[ nIndex ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamControlPointMaster::PointLastContestedAt( int point )
{
	CTeamControlPoint *pPoint = GetControlPoint(point);
	if ( pPoint )
		return pPoint->LastContestedAt();

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: This function returns the team that owns all the cap points. 
//			If its not the case that one team owns them all, it returns 0.
//			CPs are broken into groups. A team can win by owning all flags within a single group.
//			
//			Can be passed an overriding team. If this is not null, the passed team
//			number will be used for that cp. Used to predict if that CP changing would
//			win the game.
//-----------------------------------------------------------------------------
int CTeamControlPointMaster::TeamOwnsAllPoints( CTeamControlPoint *pOverridePoint /* = NULL */, int iOverrideNewTeam /* = TEAM_UNASSIGNED */ )
{
	unsigned int i;

	int iWinningTeam[MAX_CONTROL_POINT_GROUPS];

	for( i=0;i<MAX_CONTROL_POINT_GROUPS;i++	)
	{
		iWinningTeam[i] = TEAM_INVALID;
	}

	// if TEAM_INVALID, haven't found a flag for this group yet
	// if TEAM_UNASSIGNED, the group is still being contested 

	// for each control point
	for( i=0;i<m_ControlPoints.Count();i++ )
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
	for ( i=0;i<MAX_CONTROL_POINT_GROUPS;i++ )
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
bool CTeamControlPointMaster::WouldNewCPOwnerWinGame( CTeamControlPoint *pPoint, int iNewOwner )
{
	return ( TeamOwnsAllPoints( pPoint, iNewOwner ) == iNewOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTeamControlPointMaster::IsBaseControlPoint( int iPointIndex )
{
	bool retVal = false;

	for ( int iTeam = LAST_SHARED_TEAM+1; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		if ( GetBaseControlPoint( iTeam ) == iPointIndex )
		{
			retVal = true;
			break;
		}
	}

	return retVal;
}

//-----------------------------------------------------------------------------
// Purpose: Get the control point for the specified team that's at their end of
//			the control point chain.
//-----------------------------------------------------------------------------
int	CTeamControlPointMaster::GetBaseControlPoint( int iTeam )
{
	int iRetVal = -1;
	int nLowestValue = 999;
	int nHighestValue = -1;
	CTeamControlPoint *pLowestPoint = NULL;
	CTeamControlPoint *pHighestPoint = NULL;

	for( unsigned int i = 0 ; i < m_ControlPoints.Count() ; i++ )
	{
		CTeamControlPoint *pPoint = m_ControlPoints[i];

		if ( !PlayingMiniRounds() || ( IsInRound( pPoint ) && ( iTeam > LAST_SHARED_TEAM ) ) )
		{
			int nTempValue = pPoint->GetPointIndex();

			if ( nTempValue > nHighestValue )
			{
				nHighestValue = nTempValue;
				pHighestPoint = pPoint;
			}

			if ( nTempValue < nLowestValue )
			{
				nLowestValue = nTempValue;
				pLowestPoint = pPoint;
			}
		}
	}

	if ( pLowestPoint && pHighestPoint )
	{
		// which point is owned by this team?
		if ( ( pLowestPoint->GetDefaultOwner() == iTeam && pHighestPoint->GetDefaultOwner() == iTeam ) || // if the same team owns both, take the highest value to be the last point
				( pHighestPoint->GetDefaultOwner() == iTeam ) )
		{
			iRetVal = nHighestValue;
		}
		else if ( pLowestPoint->GetDefaultOwner() == iTeam )
		{
			iRetVal = nLowestValue;
		}
	}
	
	return iRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputEnable( inputdata_t &input )
{ 
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTeamControlPointMaster::InputDisable( inputdata_t &input )
{ 
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------	
int CTeamControlPointMaster::GetNumPointsOwnedByTeam( int iTeam )
{
	int nCount = 0;

	for( int i = 0 ; i < (int)m_ControlPoints.Count() ; i++ )
	{
		CTeamControlPoint *pPoint = m_ControlPoints[i];

		if ( pPoint && ( pPoint->GetTeamNumber() == iTeam ) )
		{
			nCount++;
		}
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Purpose: returns how many more mini-rounds it will take for specified team
//			to win, if they keep winning every mini-round
//-----------------------------------------------------------------------------	
int CTeamControlPointMaster::CalcNumRoundsRemaining( int iTeam )
{
	// To determine how many rounds remain for a given team if it consistently wins mini-rounds, we have to 
	// simulate forward each mini-round and track the control point ownership that would result

	// vector of control points the team owns in our forward-simulation
	CUtlVector<CTeamControlPoint *> vecControlPointsOwned;
	
	// start with all the control points the team currently owns 
	FOR_EACH_MAP_FAST( m_ControlPoints, iControlPoint )
	{
		if ( m_ControlPoints[iControlPoint]->GetOwner() == iTeam )
		{
			vecControlPointsOwned.AddToTail( m_ControlPoints[iControlPoint] );
		}
	}

	int iRoundsRemaining = 0;

	// keep simulating what will happen next if this team keeps winning, until
	// it owns all the control points in the map 
	while ( vecControlPointsOwned.Count() < (int) m_ControlPoints.Count() )
	{
		iRoundsRemaining++;

		// choose the next highest-priority round that is playable 
		for ( int i = 0 ; i < m_ControlPointRounds.Count() ; ++i )
		{
			CTeamControlPointRound *pRound = m_ControlPointRounds[i];
			if ( !pRound )
				continue;

			// see if one team owns all control points in this round
			int iRoundOwningTeam = TEAM_INVALID;
			int iControlPoint;
			for ( iControlPoint = 0; iControlPoint < pRound->m_ControlPoints.Count(); iControlPoint++ )
			{
				CTeamControlPoint *pControlPoint = pRound->m_ControlPoints[iControlPoint];
				int iControlPointOwningTeam = TEAM_INVALID;

				// determine who owns this control point.
				// First, check our simulated ownership
				if ( vecControlPointsOwned.InvalidIndex() != vecControlPointsOwned.Find( pControlPoint ) )
				{
					// This team has won this control point in forward simulation
					iControlPointOwningTeam = iTeam;
				}
				else
				{
					// use actual control point ownership
					iControlPointOwningTeam = pControlPoint->GetOwner();
				}
				
				if ( 0 == iControlPoint )
				{
					// if this is the first control point, assign ownership to the team that owns this control point
					iRoundOwningTeam = iControlPointOwningTeam;
				}
				else
				{
					// for all other control points, if the control point ownership does not match other control points, reset
					// round ownership to no team
					if ( iRoundOwningTeam != iControlPointOwningTeam )
					{
						iRoundOwningTeam = TEAM_INVALID;
					}
				}
			}
			// this round is playable if all control points are not owned by one team (or owned by a team that can't win by capping them)
			bool bPlayable = ( ( iRoundOwningTeam < FIRST_GAME_TEAM ) || ( pRound->GetInvalidCapWinner() == 1 ) || ( iRoundOwningTeam == pRound->GetInvalidCapWinner() ) );
			if ( !bPlayable )
				continue;

			// Pretend this team played and won this round.  It now owns all control points from this round.  Add all the
			// control points from this round that are not already own the owned list to the owned list
			int iNewControlPointsOwned = 0;
			FOR_EACH_VEC( pRound->m_ControlPoints, iControlPoint )
			{
				CTeamControlPoint *pControlPoint = pRound->m_ControlPoints[iControlPoint];
				if ( vecControlPointsOwned.InvalidIndex() == vecControlPointsOwned.Find( pControlPoint ) )
				{
					vecControlPointsOwned.AddToTail( pControlPoint );
					iNewControlPointsOwned++;
				}
			}
			// sanity check: team being simulated should be owning at least one more new control point per round, or they're not making progress
			Assert( iNewControlPointsOwned > 0 );	

			// now go back and pick the next playable round (if any) given the control points this team now owns,
			// repeat until all control points are owned.  The number of iterations it takes is the # of rounds remaining
			// for this team to win.
			break;
		}				
	}

	return iRoundsRemaining;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTeamControlPointMaster::GetPartialCapturePointRate( void )
{
	return m_flPartialCapturePointsRate;
}

//-----------------------------------------------------------------------------
void CTeamControlPointMaster::ListRounds( void )
{
	if ( PlayingMiniRounds() )
	{
		ConMsg( "Rounds in this map:\n\n" );

		for ( int i = 0; i < m_ControlPointRounds.Count() ; ++i )
		{
			CTeamControlPointRound* pRound = m_ControlPointRounds[i];

			if ( pRound )
			{
				const char *pszName = STRING( pRound->GetEntityName() );
				ConMsg( "%s\n", pszName );
			}
		}
	}
	else
	{
		ConMsg( "* No rounds in this map *\n" );
	}
}

//-----------------------------------------------------------------------------
void cc_ListRounds( void )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		{ return; }

	CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;
	if ( pMaster )
	{
		pMaster->ListRounds();
	}
}

static ConCommand tf_listrounds( "tf_listrounds", cc_ListRounds, "List the rounds for the current map", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
void cc_PlayRound( const CCommand& args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		{ return; }

	if ( args.ArgC() > 1 )
	{
		CTeamplayRoundBasedRules *pRules = dynamic_cast<CTeamplayRoundBasedRules*>( GameRules() );
		CTeamControlPointMaster *pMaster = g_hControlPointMasters.Count() ? g_hControlPointMasters[0] : NULL;

		if ( pRules && pMaster )
		{
			if ( pMaster->PlayingMiniRounds() )
			{
				// did we get the name of a round?
				CTeamControlPointRound *pRound = dynamic_cast<CTeamControlPointRound*>( gEntList.FindEntityByName( NULL, args[1] ) );

				if ( pRound )
				{
					pRules->SetRoundToPlayNext( pRound->GetEntityName() );
					mp_restartgame.SetValue( 5 );
				}
				else
				{
					ConMsg( "* Round \"%s\" not found in this map *\n", args[1] );
				}
			}
		}
	}
	else
	{
		ConMsg( "Usage:  tf_playround < round name >\n" );
	}
}

static ConCommand tf_playround( "tf_playround", cc_PlayRound, "Play the selected round\n\tArgument: {round name given by \"tf_listrounds\" command}", FCVAR_CHEAT );
