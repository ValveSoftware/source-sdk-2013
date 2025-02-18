//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#undef strncpy // we use std::string below that needs a good strncpy define
#undef sprintf // "
#include "cbase.h"

#include "ai_behavior_lead.h"

#include "ai_goalentity.h"
#include "ai_navigator.h"
#include "ai_speech.h"
#include "ai_senses.h"
#include "ai_playerally.h"
#include "ai_route.h"
#include "ai_pathfinder.h"
#include "sceneentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Minimum time between leader nags
#define LEAD_NAG_TIME		3.0

#define LEAD_MIN_RETRIEVEDIST_OFFSET		24

//-----------------------------------------------------------------------------
// class CAI_LeadBehavior
//
// Purpose:
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( AI_LeadArgs_t )
	// Only the flags needs saving
	DEFINE_FIELD(		flags,			FIELD_INTEGER ),

	//DEFINE_FIELD(		pszGoal,		FIELD_STRING ),
	//DEFINE_FIELD(		pszWaitPoint,	FIELD_STRING ),
	//DEFINE_FIELD(		flWaitDistance,	FIELD_FLOAT ),
	//DEFINE_FIELD(		flLeadDistance,	FIELD_FLOAT ),
	//DEFINE_FIELD(		flRetrieveDistance,	FIELD_FLOAT ),
	//DEFINE_FIELD(		flSuccessDistance,	FIELD_FLOAT ),
	//DEFINE_FIELD(		bRun,			FIELD_BOOLEAN ),
	//DEFINE_FIELD(		bDontSpeakStart,			FIELD_BOOLEAN ),
	//DEFINE_FIELD(		bGagLeader,			FIELD_BOOLEAN ),

	DEFINE_FIELD(		iRetrievePlayer,			FIELD_INTEGER ),
	DEFINE_FIELD(		iRetrieveWaitForSpeak,		FIELD_INTEGER ),
	DEFINE_FIELD(		iComingBackWaitForSpeak,	FIELD_INTEGER ),
	DEFINE_FIELD(		bStopScenesWhenPlayerLost,	FIELD_BOOLEAN ),
	DEFINE_FIELD(		bLeadDuringCombat,			FIELD_BOOLEAN ),

END_DATADESC();


BEGIN_DATADESC( CAI_LeadBehavior )
	DEFINE_EMBEDDED(	m_args ),
	//					m_pSink		(reconnected on load)
	DEFINE_FIELD(		m_hSinkImplementor, FIELD_EHANDLE ),
	DEFINE_FIELD(		m_goal,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD(		m_goalyaw, 		FIELD_FLOAT ),
	DEFINE_FIELD(		m_waitpoint, 	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD(		m_waitdistance, FIELD_FLOAT ),
	DEFINE_FIELD(		m_leaddistance, FIELD_FLOAT ),
	DEFINE_FIELD(		m_retrievedistance, FIELD_FLOAT ),
	DEFINE_FIELD(		m_successdistance, FIELD_FLOAT ),
	DEFINE_FIELD(		m_weaponname,	FIELD_STRING ),
	DEFINE_FIELD(		m_run,			FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_gagleader, FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_hasspokenstart, FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_hasspokenarrival, FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_hasPausedScenes, FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_flSpeakNextNagTime, FIELD_TIME ),
	DEFINE_FIELD(		m_flWeaponSafetyTimeOut, FIELD_TIME ),
	DEFINE_FIELD(		m_flNextLeadIdle, FIELD_TIME ),
	DEFINE_FIELD(		m_bInitialAheadTest, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED(	m_MoveMonitor ),
	DEFINE_EMBEDDED(	m_LostTimer ),
	DEFINE_EMBEDDED(	m_LostLOSTimer ),
END_DATADESC();

//-----------------------------------------------------------------------------


void CAI_LeadBehavior::OnRestore()
{
	CBaseEntity *pSinkImplementor = m_hSinkImplementor;
	if ( pSinkImplementor )
	{
		m_pSink = dynamic_cast<CAI_LeadBehaviorHandler *>(pSinkImplementor);
		if ( !m_pSink )
		{
			DevMsg( "Failed to reconnect to CAI_LeadBehaviorHandler\n" );
			m_hSinkImplementor = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays
// Input  : Previous text offset from the top
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_LeadBehavior::DrawDebugTextOverlays( int text_offset )
{
	char			tempstr[ 512 ];
	int				offset;

	offset = BaseClass::DrawDebugTextOverlays( text_offset );
	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{	
		if ( HasGoal() )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Goal: %s %s", m_args.pszGoal, VecToString( m_goal ) );
			GetOuter()->EntityText( offset, tempstr, 0 );
			offset++;
		}
		else 
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Goal: None" );
			GetOuter()->EntityText( offset, tempstr, 0 );
			offset++;
		}
	}

	return offset;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_LeadBehavior::IsNavigationUrgent( void )
{
#if defined( HL2_DLL )
	if( HasGoal() && !hl2_episodic.GetBool() )
	{
		return (GetOuter()->Classify() == CLASS_PLAYER_ALLY_VITAL);
	}
#endif
	return BaseClass::IsNavigationUrgent();
}

//-------------------------------------

void CAI_LeadBehavior::LeadPlayer( const AI_LeadArgs_t &leadArgs, CAI_LeadBehaviorHandler *pSink )
{
#ifndef CSTRIKE_DLL
	CAI_PlayerAlly *pOuter = dynamic_cast<CAI_PlayerAlly*>(GetOuter());
	if ( pOuter && AI_IsSinglePlayer() )
	{
		pOuter->SetSpeechTarget( UTIL_GetLocalPlayer() );
	}
#endif

	if( SetGoal( leadArgs ) )
	{
		SetCondition( COND_PROVOKED );
		Connect( pSink );
		NotifyChangeBehaviorStatus();
	}
	else
	{
		DevMsg( "*** Warning! LeadPlayer() has a NULL Goal Ent\n" );
	}
}

//-------------------------------------

void CAI_LeadBehavior::StopLeading( void )
{
	ClearGoal();
	m_pSink = NULL;
	NotifyChangeBehaviorStatus();
}

//-------------------------------------

bool CAI_LeadBehavior::CanSelectSchedule()
{
 	if ( !AI_GetSinglePlayer() || AI_GetSinglePlayer()->IsDead() )
		return false;

	bool fAttacked = ( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ) );
	bool fNonCombat = ( GetNpcState() == NPC_STATE_IDLE || GetNpcState() == NPC_STATE_ALERT );
	
	return ( !fAttacked && (fNonCombat || m_args.bLeadDuringCombat) && HasGoal() );
}

//-------------------------------------

void CAI_LeadBehavior::BeginScheduleSelection()
{
	SetTarget( AI_GetSinglePlayer() );
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( pExpresser )
		pExpresser->ClearSpokeConcept( TLK_LEAD_ARRIVAL );
}

//-------------------------------------

bool CAI_LeadBehavior::SetGoal( const AI_LeadArgs_t &args )
{
	CBaseEntity *pGoalEnt;
	pGoalEnt = gEntList.FindEntityByName( NULL, args.pszGoal );
	
	if ( !pGoalEnt )
		return false;

	m_args 		= args;	// @Q (toml 08-13-02): need to copy string?
	m_goal 		= pGoalEnt->GetLocalOrigin();
	m_goalyaw 	= (args.flags & AILF_USE_GOAL_FACING) ? pGoalEnt->GetLocalAngles().y : -1;
	m_waitpoint = vec3_origin;
	m_waitdistance = args.flWaitDistance;
	m_leaddistance = args.flLeadDistance ? args.flLeadDistance : 64;
	m_retrievedistance = args.flRetrieveDistance ? args.flRetrieveDistance : (m_leaddistance + LEAD_MIN_RETRIEVEDIST_OFFSET);
	m_successdistance = args.flSuccessDistance ? args.flSuccessDistance : 0;
	m_run = args.bRun;
	m_gagleader = args.bGagLeader;
	m_hasspokenstart = args.bDontSpeakStart;
	m_hasspokenarrival = false;
	m_hasPausedScenes = false;
	m_flSpeakNextNagTime = 0;
	m_flWeaponSafetyTimeOut = 0;
	m_flNextLeadIdle = gpGlobals->curtime + 10;
	m_bInitialAheadTest = true;

	if ( args.pszWaitPoint && args.pszWaitPoint[0] )
	{
		CBaseEntity *pWaitPoint = gEntList.FindEntityByName( NULL, args.pszWaitPoint );
		if ( pWaitPoint )
		{
			m_waitpoint = pWaitPoint->GetLocalOrigin();
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_LeadBehavior::GetClosestPointOnRoute( const Vector &targetPos, Vector *pVecClosestPoint )
{
	AI_Waypoint_t *waypoint = GetOuter()->GetNavigator()->GetPath()->GetCurWaypoint();
	AI_Waypoint_t *builtwaypoints = NULL;
	if ( !waypoint )
	{
		// We arrive here twice when lead behaviour starts:
		//	- When the lead behaviour is first enabled. We have no schedule. We want to know if the player is ahead of us.
		//	- A frame later when we've chosen to lead the player, but we still haven't built our route. We know that the
		//	  the player isn't lagging, so it's safe to go ahead and simply say he's ahead of us. This avoids building 
		//	  the temp route twice.
		if ( IsCurSchedule( SCHED_LEAD_PLAYER, false ) )
			return true;

		// Build a temp route to the gold and use that
		builtwaypoints = GetOuter()->GetPathfinder()->BuildRoute( GetOuter()->GetAbsOrigin(), m_goal, NULL, GetOuter()->GetDefaultNavGoalTolerance(), GetOuter()->GetNavType(), true );
		if ( !builtwaypoints )
			return false;

		GetOuter()->GetPathfinder()->UnlockRouteNodes( builtwaypoints );
		waypoint = builtwaypoints;
	}

	// Find the nearest node to the target (going forward)
	float		flNearestDist2D	= 999999999;
	float		flNearestDist	= 999999999;
	float		flPathDist, flPathDist2D;

	Vector vecNearestPoint(0, 0, 0);
	Vector vecPrevPos = GetOuter()->GetAbsOrigin();
	for ( ; (waypoint != NULL) ; waypoint = waypoint->GetNext() )
	{
		// Find the closest point on the line segment on the path
		Vector vecClosest;
		CalcClosestPointOnLineSegment( targetPos, vecPrevPos, waypoint->GetPos(), vecClosest );
		/*
		if ( builtwaypoints )
		{
			NDebugOverlay::Line( vecPrevPos, waypoint->GetPos(), 0,0,255,true, 10.0 );
		}
		*/
		vecPrevPos = waypoint->GetPos();

		// Find the distance between this test point and our goal point
		flPathDist2D = vecClosest.AsVector2D().DistToSqr( targetPos.AsVector2D() );
		if ( flPathDist2D > flNearestDist2D )
			continue;

		flPathDist = vecClosest.z - targetPos.z;
		flPathDist *= flPathDist;
		flPathDist += flPathDist2D;
		if (( flPathDist2D == flNearestDist2D ) && ( flPathDist >= flNearestDist ))
			continue;

		flNearestDist2D	= flPathDist2D;
		flNearestDist	= flPathDist;
		vecNearestPoint	= vecClosest;
	}

	if ( builtwaypoints )
	{
		//NDebugOverlay::Line( vecNearestPoint, targetPos, 0,255,0,true, 10.0 );
		DeleteAll( builtwaypoints );
	}

	*pVecClosestPoint = vecNearestPoint;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the player is further ahead on the lead route than I am
//-----------------------------------------------------------------------------
bool CAI_LeadBehavior::PlayerIsAheadOfMe( bool bForce )
{
	// Find the nearest point on our route to the player, and see if that's further 
	// ahead of us than our nearest point. 

	// If we're not leading, our route doesn't lead to the goal, so we can't use it.
	// If we just started leading, go ahead and test, and we'll build a temp route.
	if ( !m_bInitialAheadTest && !IsCurSchedule( SCHED_LEAD_PLAYER, false ) && !bForce )
		return false;

	m_bInitialAheadTest = false;

	Vector vecClosestPoint;
	if ( GetClosestPointOnRoute( AI_GetSinglePlayer()->GetAbsOrigin(), &vecClosestPoint ) )
	{
		// If the closest point is not right next to me, then 
		// the player is somewhere ahead of me on the route.
		if ( (vecClosestPoint - GetOuter()->GetAbsOrigin()).LengthSqr() > (32*32) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_LeadBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( HasGoal() )
	{
		// Fix for bad transition case (to investigate)
		if ( ( WorldSpaceCenter() - m_goal ).LengthSqr() > (64*64) && IsCurSchedule( SCHED_LEAD_AWAIT_SUCCESS, false)  )
		{
			GetOuter()->ClearSchedule( "Lead behavior - bad transition?" );
		}

		// We have to collect data about the person we're leading around.
		CBaseEntity *pFollower = AI_GetSinglePlayer();

		if( pFollower )
		{
			ClearCondition( COND_LEAD_FOLLOWER_VERY_CLOSE );
			ClearCondition( COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME );

			// Check distance to the follower
			float flFollowerDist = ( WorldSpaceCenter() - pFollower->WorldSpaceCenter() ).Length();
			bool bLagging = flFollowerDist > (m_leaddistance*4);
			if ( bLagging )
			{
				if ( PlayerIsAheadOfMe() )
				{
					bLagging = false;
				}
			}

			// Player heading towards me?
			// Only factor this in if you're not too far from them
			if ( flFollowerDist < (m_leaddistance*4) )
			{
				Vector vecVelocity = pFollower->GetSmoothedVelocity();
				if ( VectorNormalize(vecVelocity) > 50 )
				{
					Vector vecToPlayer = (GetAbsOrigin() - pFollower->GetAbsOrigin());
					VectorNormalize( vecToPlayer );
					if ( DotProduct( vecVelocity, vecToPlayer ) > 0.5 )
					{
						SetCondition( COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME );
						bLagging = false;
					}
				}
			}

			// If he's outside our lag range, consider him lagging
			if ( bLagging )
			{
				SetCondition( COND_LEAD_FOLLOWER_LAGGING );
				ClearCondition( COND_LEAD_FOLLOWER_NOT_LAGGING );
			}
			else
			{
				ClearCondition( COND_LEAD_FOLLOWER_LAGGING );
				SetCondition( COND_LEAD_FOLLOWER_NOT_LAGGING );

				// If he's really close, note that
				if ( flFollowerDist < m_leaddistance )
				{
					SetCondition( COND_LEAD_FOLLOWER_VERY_CLOSE );
				}
			}

			// To be considered not lagging, the follower must be visible, and within the lead distance
			if ( GetOuter()->FVisible( pFollower ) && GetOuter()->GetSenses()->ShouldSeeEntity( pFollower ) )
			{
				SetCondition( COND_LEAD_HAVE_FOLLOWER_LOS );
				m_LostLOSTimer.Stop();
			}
			else
			{
				ClearCondition( COND_LEAD_HAVE_FOLLOWER_LOS );

				// We don't have a LOS. But if we did have LOS, don't clear it until the timer is up.
				if ( m_LostLOSTimer.IsRunning() )
				{
					if ( m_LostLOSTimer.Expired() )
					{
						SetCondition( COND_LEAD_FOLLOWER_LAGGING );
						ClearCondition( COND_LEAD_FOLLOWER_NOT_LAGGING );
					}
				}
				else
				{
					m_LostLOSTimer.Start();
				}
			}

			// Now we want to see if the follower is lost. Being lost means being (far away || out of LOS ) 
			// && some time has passed. Also, lagging players are considered lost if the NPC's never delivered
			// the start speech, because it means the NPC should run to the player to start the lead.
			if( HasCondition( COND_LEAD_FOLLOWER_LAGGING ) )
			{
				if ( !m_hasspokenstart )
				{
					SetCondition( COND_LEAD_FOLLOWER_LOST );
				}
				else
				{
					if ( m_args.bStopScenesWhenPlayerLost )
					{
						// Try and stop me speaking my monolog, if I am
						if ( !m_hasPausedScenes && IsRunningScriptedScene( GetOuter() ) )
						{
							//Msg("Stopping scenes.\n");
							PauseActorsScriptedScenes( GetOuter(), false );
							m_hasPausedScenes = true;
						}
					}

					if( m_LostTimer.IsRunning() )
					{
						if( m_LostTimer.Expired() )
						{
							SetCondition( COND_LEAD_FOLLOWER_LOST );
						}
					}
					else
					{
						m_LostTimer.Start();
					}
				}
			}
			else
			{
				// If I was speaking a monolog, resume it
				if ( m_args.bStopScenesWhenPlayerLost && m_hasPausedScenes )
				{
					if ( IsRunningScriptedScene( GetOuter() ) )
					{
						//Msg("Resuming scenes.\n");
						ResumeActorsScriptedScenes( GetOuter(), false );
					}

					m_hasPausedScenes = false;
				}

				m_LostTimer.Stop();
				ClearCondition( COND_LEAD_FOLLOWER_LOST );
			}

			// Evaluate for success
			// Success right now means being stationary, close to the goal, and having the player close by
			if ( !( m_args.flags & AILF_NO_DEF_SUCCESS ) )
			{
				ClearCondition( COND_LEAD_SUCCESS );

				// Check Z first, and only check 2d if we're within that
				bool bWithinZ = fabs(GetLocalOrigin().z - m_goal.z) < 64;
				if ( bWithinZ && (GetLocalOrigin() - m_goal).Length2D() <= 64 )
				{
					if ( HasCondition( COND_LEAD_FOLLOWER_VERY_CLOSE ) )
					{
						SetCondition( COND_LEAD_SUCCESS );
					}
					else if ( m_successdistance )
					{
						float flDistSqr = (pFollower->GetAbsOrigin() - GetLocalOrigin()).Length2DSqr();
						if ( flDistSqr < (m_successdistance*m_successdistance) )
						{
							SetCondition( COND_LEAD_SUCCESS );
						}
					}
				}
			}
			if ( m_MoveMonitor.IsMarkSet() && m_MoveMonitor.TargetMoved( pFollower ) )
				SetCondition( COND_LEAD_FOLLOWER_MOVED_FROM_MARK );
			else
				ClearCondition( COND_LEAD_FOLLOWER_MOVED_FROM_MARK );
		}
	}

	if( m_args.bLeadDuringCombat )
	{
		ClearCondition( COND_LIGHT_DAMAGE );
		ClearCondition( COND_HEAVY_DAMAGE );
	}
}

//-------------------------------------

int CAI_LeadBehavior::SelectSchedule()
{
	if ( HasGoal() )
	{
		if( HasCondition(COND_LEAD_SUCCESS) )
		{
			return SCHED_LEAD_SUCCEED;
		}

		// Player's here, but does he have the weapon we want him to have?
		if ( m_weaponname != NULL_STRING )
		{
			CBasePlayer *pFollower = AI_GetSinglePlayer();
			if ( pFollower && !pFollower->Weapon_OwnsThisType( STRING(m_weaponname) ) )
			{
				// If the safety timeout has run out, just give the player the weapon
				if ( !m_flWeaponSafetyTimeOut || (m_flWeaponSafetyTimeOut > gpGlobals->curtime) )
					return SCHED_LEAD_PLAYERNEEDSWEAPON;

				string_t iszItem = AllocPooledString( "weapon_bugbait" );
				pFollower->GiveNamedItem( STRING(iszItem) );
			}
		}

		// If we have a waitpoint, we want to wait at it for the player.
		if( HasWaitPoint() && !PlayerIsAheadOfMe( true ) )
		{
			bool bKeepWaiting = true;

			// If we have no wait distance, trigger as soon as the player comes in view
			if ( !m_waitdistance )
			{
				if ( HasCondition( COND_SEE_PLAYER ) )
				{
					// We've spotted the player, so stop waiting
					bKeepWaiting = false;
				}
			}
			else
			{
				// We have to collect data about the person we're leading around.
				CBaseEntity *pFollower = AI_GetSinglePlayer();
				if( pFollower )
				{
					float flFollowerDist = ( WorldSpaceCenter() - pFollower->WorldSpaceCenter() ).Length();
					if ( flFollowerDist < m_waitdistance )
					{
						bKeepWaiting = false;
					}
				}
			}

			// Player still not here?
			if ( bKeepWaiting )
				return SCHED_LEAD_WAITFORPLAYER;

			// We're finished waiting
			m_waitpoint = vec3_origin;
			Speak( TLK_LEAD_WAITOVER );

			// Don't speak the start line, because we've said 
			m_hasspokenstart = true;
			return SCHED_WAIT_FOR_SPEAK_FINISH;
		}

		// If we haven't spoken our start speech, do that first
		if ( !m_hasspokenstart )
		{
			if ( HasCondition(COND_LEAD_HAVE_FOLLOWER_LOS) && HasCondition(COND_LEAD_FOLLOWER_VERY_CLOSE) )
				return SCHED_LEAD_SPEAK_START;

			// We haven't spoken to him, and we still need to. Go get him.
			return SCHED_LEAD_RETRIEVE;
		}

		if( HasCondition( COND_LEAD_FOLLOWER_LOST ) )
		{
			if( m_args.iRetrievePlayer )
			{
				// If not, we want to go get the player.
				DevMsg( GetOuter(), "Follower lost. Spoke COMING_BACK.\n");

				Speak( TLK_LEAD_COMINGBACK );
				m_MoveMonitor.ClearMark();

				// If we spoke something, wait for it to finish
				if ( m_args.iComingBackWaitForSpeak && IsSpeaking() )
					return SCHED_LEAD_SPEAK_THEN_RETRIEVE_PLAYER;

				return SCHED_LEAD_RETRIEVE;
			}
			else
			{
				// Just stay right here and wait.
				return SCHED_LEAD_WAITFORPLAYERIDLE;
			}
		}

		if( HasCondition( COND_LEAD_FOLLOWER_LAGGING ) )
		{
			DevMsg( GetOuter(), "Follower lagging. Spoke CATCHUP.\n");

			Speak( TLK_LEAD_CATCHUP );
			return SCHED_LEAD_PAUSE;
		}
		else
		{
			// If we're at the goal, wait for the player to get here
			if ( ( WorldSpaceCenter() - m_goal ).LengthSqr() < (64*64) )
				return SCHED_LEAD_AWAIT_SUCCESS;

			// If we were retrieving the player, speak the resume
			if ( IsCurSchedule( SCHED_LEAD_RETRIEVE, false ) || IsCurSchedule( SCHED_LEAD_WAITFORPLAYERIDLE, false ) )
			{
				Speak( TLK_LEAD_RETRIEVE );

				// If we spoke something, wait for it to finish, if the mapmakers wants us to
				if ( m_args.iRetrieveWaitForSpeak && IsSpeaking() )
					return SCHED_LEAD_SPEAK_THEN_LEAD_PLAYER;
			}

			DevMsg( GetOuter(), "Leading Follower.\n");
			return SCHED_LEAD_PLAYER;
		}
	}
	return BaseClass::SelectSchedule();
}

//-------------------------------------

int CAI_LeadBehavior::TranslateSchedule( int scheduleType )
{
	bool bInCombat = (m_args.bLeadDuringCombat && GetOuter()->GetState() == NPC_STATE_COMBAT);
	switch( scheduleType )
	{
	case SCHED_LEAD_PAUSE:
		if( bInCombat )
			return SCHED_LEAD_PAUSE_COMBAT;
		break;
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

bool CAI_LeadBehavior::IsCurTaskContinuousMove()
{
	const Task_t *pCurTask = GetCurTask();
	if ( pCurTask && pCurTask->iTask == TASK_LEAD_MOVE_TO_RANGE )
		return true;
	return BaseClass::IsCurTaskContinuousMove();
}

//-------------------------------------

void CAI_LeadBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_LEAD_FACE_GOAL:
		{
			if ( m_goalyaw != -1 )
			{
				GetMotor()->SetIdealYaw( m_goalyaw ); 
			}

			TaskComplete();
			break;
		}

		case TASK_LEAD_SUCCEED:
		{
			Speak( TLK_LEAD_SUCCESS );
			NotifyEvent( LBE_SUCCESS );

			break;
		}

		case TASK_LEAD_ARRIVE:
		{
			// Only speak the first time we arrive
			if ( !m_hasspokenarrival )
			{
				Speak( TLK_LEAD_ARRIVAL );
				NotifyEvent( LBE_ARRIVAL );

				m_hasspokenarrival = true;
			}
			else
			{
				TaskComplete();
			}
			
			break;
		}
		
		case TASK_STOP_LEADING:
		{
			ClearGoal();
			TaskComplete();
			break;
		}

		case TASK_GET_PATH_TO_LEAD_GOAL:
		{
			if ( GetNavigator()->SetGoal( m_goal ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("NO PATH");
			}
			break;
		}
		
		case TASK_LEAD_GET_PATH_TO_WAITPOINT:
		{
			if ( GetNavigator()->SetGoal( m_waitpoint ) )
			{
				TaskComplete();
			}
			else
			{
				TaskFail("NO PATH");
			}
			break;
		}

		case TASK_LEAD_WALK_PATH:
		{
			// If we're leading, and we're supposed to run, run instead of walking
			if ( m_run && 
				( IsCurSchedule( SCHED_LEAD_WAITFORPLAYER, false ) || IsCurSchedule( SCHED_LEAD_PLAYER, false ) || IsCurSchedule( SCHED_LEAD_SPEAK_THEN_LEAD_PLAYER, false )|| IsCurSchedule( SCHED_LEAD_RETRIEVE, false ) ) )
			{
				ChainStartTask( TASK_RUN_PATH );
			}
			else
			{
				ChainStartTask( TASK_WALK_PATH );
			}
			break;
		}

		case TASK_LEAD_WAVE_TO_PLAYER:
		{
			// Wave to the player if we can see him. Otherwise, just idle.
			if ( HasCondition( COND_SEE_PLAYER ) )
			{
				Speak( TLK_LEAD_ATTRACTPLAYER );
				if ( HaveSequenceForActivity(ACT_SIGNAL1) )
				{
					SetActivity(ACT_SIGNAL1);
				}
			}
			else
			{
				SetActivity(ACT_IDLE);
			}

			TaskComplete();
			break;
		}

		case TASK_LEAD_PLAYER_NEEDS_WEAPON:
		{
			float flAvailableTime = GetOuter()->GetExpresser()->GetSemaphoreAvailableTime( GetOuter() );

			// if someone else is talking, don't speak
			if ( flAvailableTime <= gpGlobals->curtime )
			{
				Speak( TLK_LEAD_MISSINGWEAPON );
			}

			SetActivity(ACT_IDLE);
			TaskComplete();
			break;
		}

		case TASK_LEAD_SPEAK_START:
		{
			m_hasspokenstart = true;

			Speak( TLK_LEAD_START );
			SetActivity(ACT_IDLE);
			TaskComplete();
			break;
		}

		case TASK_LEAD_MOVE_TO_RANGE:
		{
			// If we haven't spoken our start speech, move closer
			if ( !m_hasspokenstart)
			{
				ChainStartTask( TASK_MOVE_TO_GOAL_RANGE, m_leaddistance - 24 );
			}
			else
			{
				ChainStartTask( TASK_MOVE_TO_GOAL_RANGE, m_retrievedistance );
			}
			break;
		}

		case TASK_LEAD_RETRIEVE_WAIT:
		{
			m_MoveMonitor.SetMark( AI_GetSinglePlayer(), 24 );
			ChainStartTask( TASK_WAIT_INDEFINITE );
			break;
		}

		case TASK_STOP_MOVING:
		{
			BaseClass::StartTask( pTask);

			if ( IsCurSchedule( SCHED_LEAD_PAUSE, false ) && pTask->flTaskData == 1 )
			{
				GetNavigator()->SetArrivalDirection( GetTarget() );
			}
			break;
		}

		case TASK_WAIT_FOR_SPEAK_FINISH:
		{
			BaseClass::StartTask( pTask);

			if( GetOuter()->GetState() == NPC_STATE_COMBAT )
			{
				// Don't stand around jabbering in combat. 
				TaskComplete();
			}

			// If we're not supposed to wait for the player, don't wait for speech to finish.
			// Instead, just wait a wee tad, and then start moving. NPC will speak on the go.
			if ( TaskIsRunning() && !m_args.iRetrievePlayer )
			{
				if ( gpGlobals->curtime - GetOuter()->GetTimeTaskStarted() > 0.3 )
				{
					TaskComplete();
				}
			}
			break;
		}

		default:
			BaseClass::StartTask( pTask);
	}
}

//-------------------------------------

void CAI_LeadBehavior::RunTask( const Task_t *pTask )		
{ 
	switch ( pTask->iTask )
	{
		case TASK_LEAD_SUCCEED:
		{
			if ( !IsSpeaking() )
			{
				TaskComplete();
				NotifyEvent( LBE_DONE );
			}
			break;
		}
		case TASK_LEAD_ARRIVE:
		{
			if ( !IsSpeaking() )
			{
				TaskComplete();
				NotifyEvent( LBE_ARRIVAL_DONE );
			}
			break;
		}

		case TASK_LEAD_MOVE_TO_RANGE:
		{
			// If we haven't spoken our start speech, move closer
 			if ( !m_hasspokenstart)
			{
				ChainRunTask( TASK_MOVE_TO_GOAL_RANGE, m_leaddistance - 24 );
			}
			else
			{
 				ChainRunTask( TASK_MOVE_TO_GOAL_RANGE, m_retrievedistance );

				if ( !TaskIsComplete() )
				{
					// Transition to a walk when we get near the player
					// Check Z first, and only check 2d if we're within that
					Vector vecGoalPos = GetNavigator()->GetGoalPos();
					float distance = fabs(vecGoalPos.z - GetLocalOrigin().z);
					bool bWithinZ = false;
					if ( distance < m_retrievedistance )
					{
						distance = ( vecGoalPos - GetLocalOrigin() ).Length2D();
						bWithinZ = true;
					}

					if ( distance > m_retrievedistance )
					{
						Activity followActivity = ACT_WALK;
						if ( GetOuter()->GetState() == NPC_STATE_COMBAT || ( (!bWithinZ || distance < (m_retrievedistance*4)) && GetOuter()->GetState() != NPC_STATE_COMBAT ) )
						{
							followActivity = ACT_RUN;
						}

						// Don't confuse move and shoot by resetting the activity every think
						Activity curActivity = GetNavigator()->GetMovementActivity();
						switch( curActivity )
						{
						case ACT_WALK_AIM:	curActivity = ACT_WALK;	break;
						case ACT_RUN_AIM:	curActivity = ACT_RUN;	break;
						}
						
						if ( curActivity != followActivity )
						{
							GetNavigator()->SetMovementActivity(followActivity);
						}
						GetNavigator()->SetArrivalDirection( GetOuter()->GetTarget() );
					}
				}
			}
			break;
		}

		case TASK_LEAD_RETRIEVE_WAIT:
		{
			ChainRunTask( TASK_WAIT_INDEFINITE );
			break;
		}

		case TASK_LEAD_WALK_PATH:
		{
			// If we're leading, and we're supposed to run, run instead of walking
			if ( m_run && 
				( IsCurSchedule( SCHED_LEAD_WAITFORPLAYER, false ) || IsCurSchedule( SCHED_LEAD_PLAYER, false ) || IsCurSchedule( SCHED_LEAD_SPEAK_THEN_LEAD_PLAYER, false )|| IsCurSchedule( SCHED_LEAD_RETRIEVE, false ) ) )
			{
				ChainRunTask( TASK_RUN_PATH );
			}
			else
			{
				ChainRunTask( TASK_WALK_PATH );
			}

			// While we're walking
			if ( TaskIsRunning() && IsCurSchedule( SCHED_LEAD_PLAYER, false ) )
			{
				// If we're not speaking, and we haven't tried for a while, try to speak lead idle
				if ( m_flNextLeadIdle < gpGlobals->curtime && !IsSpeaking() )
				{
					m_flNextLeadIdle = gpGlobals->curtime + RandomFloat( 10,15 );

					if ( !m_args.iRetrievePlayer && HasCondition( COND_LEAD_FOLLOWER_LOST ) && HasCondition(COND_SEE_PLAYER) )
					{
						Speak( TLK_LEAD_COMINGBACK );
					}
					else
					{
						Speak( TLK_LEAD_IDLE );
					}
				}
			}

			break;
		}

		default:
			BaseClass::RunTask( pTask);
	}
}

//-------------------------------------

bool CAI_LeadBehavior::Speak( AIConcept_t concept )
{
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( !pExpresser )
		return false;

	// If the leader is gagged, don't speak any lead speech
	if ( m_gagleader )
		return false;

	// If we haven't said the start speech, don't nag
	bool bNag = ( FStrEq(concept,TLK_LEAD_COMINGBACK) || FStrEq(concept, TLK_LEAD_CATCHUP) || FStrEq(concept, TLK_LEAD_RETRIEVE) );
	if ( !m_hasspokenstart && bNag )
		return false;

	if ( hl2_episodic.GetBool() )
	{
		// If we're a player ally, only speak the concept if we're allowed to.
		// This allows the response rules to control it better (i.e. handles respeakdelay)
		// We ignore nag timers for this, because the response rules will control refire rates.
		CAI_PlayerAlly *pAlly = dynamic_cast<CAI_PlayerAlly*>(GetOuter());
		if ( pAlly )
 			return pAlly->SpeakIfAllowed( concept, GetConceptModifiers( concept ) );
	}

	// Don't spam Nags
	if ( bNag )
	{
		if ( m_flSpeakNextNagTime > gpGlobals->curtime )
		{
			DevMsg( GetOuter(), "Leader didn't speak due to Nag timer.\n");
			return false;
		}
	}
	
	if ( pExpresser->Speak( concept, GetConceptModifiers( concept ) ) )
	{
		m_flSpeakNextNagTime = gpGlobals->curtime + LEAD_NAG_TIME;
		return true;
	}

	return false;
}

//-------------------------------------

bool CAI_LeadBehavior::IsSpeaking()
{
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( !pExpresser )
		return false;
		
	return pExpresser->IsSpeaking();
}

//-------------------------------------

bool CAI_LeadBehavior::Connect( CAI_LeadBehaviorHandler *pSink )
{
	m_pSink = pSink;
	m_hSinkImplementor = dynamic_cast<CBaseEntity *>(pSink);

	if ( m_hSinkImplementor == NULL )
		DevMsg( 2, "Note: CAI_LeadBehaviorHandler connected to a sink that isn't an entity. Manual fixup on load will be necessary\n" );

	return true;
}

//-------------------------------------

bool CAI_LeadBehavior::Disconnect( CAI_LeadBehaviorHandler *pSink )
{
	Assert( pSink == m_pSink );
	m_pSink = NULL;
	m_hSinkImplementor = NULL;
	return true;
}

//-------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_LeadBehavior )

	DECLARE_CONDITION( COND_LEAD_FOLLOWER_LOST )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_LAGGING )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_NOT_LAGGING )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_VERY_CLOSE )
	DECLARE_CONDITION( COND_LEAD_SUCCESS )
	DECLARE_CONDITION( COND_LEAD_HAVE_FOLLOWER_LOS )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_MOVED_FROM_MARK )
	DECLARE_CONDITION( COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME )

	//---------------------------------
	//
	// Lead
	//
	DECLARE_TASK( TASK_GET_PATH_TO_LEAD_GOAL )
	DECLARE_TASK( TASK_STOP_LEADING )
	DECLARE_TASK( TASK_LEAD_ARRIVE )
	DECLARE_TASK( TASK_LEAD_SUCCEED )
	DECLARE_TASK( TASK_LEAD_FACE_GOAL )
	DECLARE_TASK( TASK_LEAD_GET_PATH_TO_WAITPOINT )
	DECLARE_TASK( TASK_LEAD_WAVE_TO_PLAYER )
	DECLARE_TASK( TASK_LEAD_PLAYER_NEEDS_WEAPON )
	DECLARE_TASK( TASK_LEAD_MOVE_TO_RANGE )
	DECLARE_TASK( TASK_LEAD_SPEAK_START )
	DECLARE_TASK( TASK_LEAD_RETRIEVE_WAIT )
	DECLARE_TASK( TASK_LEAD_WALK_PATH )

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_RETRIEVE,

		"	Tasks"
		"		TASK_GET_PATH_TO_PLAYER			0"
		"		TASK_LEAD_MOVE_TO_RANGE			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT_FOR_SPEAK_FINISH		1"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_LEAD_RETRIEVE_WAIT"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//-------------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_SPEAK_THEN_RETRIEVE_PLAYER,

		"	Tasks"
		"		TASK_WAIT_FOR_SPEAK_FINISH		1"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_LEAD_RETRIEVE"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//-------------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_RETRIEVE_WAIT,

		"	Tasks"
		"		TASK_LEAD_RETRIEVE_WAIT			0"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_LEAD_FOLLOWER_LAGGING"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME"
		"		COND_LEAD_FOLLOWER_MOVED_FROM_MARK"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_PLAYER,

		"	Tasks"
		"		TASK_WAIT_FOR_SPEAK_FINISH	1"
		"		TASK_GET_PATH_TO_LEAD_GOAL	0"
		"		TASK_LEAD_WALK_PATH			0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_LEAD_FOLLOWER_LAGGING"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_AWAIT_SUCCESS,

		"	Tasks"
		"		TASK_LEAD_FACE_GOAL			0"
		"		TASK_FACE_IDEAL				0"
		"		TASK_LEAD_ARRIVE			0"
		"		TASK_WAIT_INDEFINITE		0"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_LEAD_FOLLOWER_LAGGING"
		"		COND_LEAD_SUCCESS"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	(
		SCHED_LEAD_SUCCEED,

		"	Tasks"
		"		TASK_LEAD_SUCCEED			0"
		"		TASK_STOP_LEADING			0"
		""
	)

	//---------------------------------
	// This is the schedule Odell uses to pause the tour momentarily
	// if the player lags behind. If the player shows up in a 
	// couple of seconds, the tour will resume. Otherwise, Odell
	// moves to retrieve.
	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_PAUSE,

		"	Tasks"
		"		TASK_STOP_MOVING			1"
		"		TASK_FACE_TARGET			0"
		"		TASK_WAIT					5"
		"		TASK_WAIT_RANDOM			5"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_LEAD_RETRIEVE"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME"
		"		COND_LEAD_FOLLOWER_NOT_LAGGING"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	DEFINE_SCHEDULE
	(
		SCHED_LEAD_PAUSE_COMBAT,

		"	Tasks"
		"		TASK_STOP_MOVING			1"
		"		TASK_FACE_TARGET			0"
		"		TASK_WAIT					1"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_LEAD_RETRIEVE"
		""
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LEAD_FOLLOWER_MOVING_TOWARDS_ME"
		"		COND_LEAD_FOLLOWER_NOT_LAGGING"
		"		COND_LEAD_FOLLOWER_LOST"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_WAITFORPLAYER,

		"	Tasks"
		"		TASK_LEAD_GET_PATH_TO_WAITPOINT	0"
		"		TASK_LEAD_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						0.5"
		"		TASK_FACE_TARGET				0"
		"		TASK_LEAD_WAVE_TO_PLAYER		0"
		"		TASK_WAIT						5.0"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_WAITFORPLAYERIDLE,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_WAIT						0.5"
		"		TASK_FACE_TARGET				0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_IDLE"
		"		TASK_WAIT						2"
		"		"
		"	Interrupts"
		"		COND_LEAD_FOLLOWER_VERY_CLOSE"
		"		COND_LIGHT_DAMAGE"
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_PLAYERNEEDSWEAPON,

		"	Tasks"
		"		TASK_FACE_PLAYER				0"
		"		TASK_LEAD_PLAYER_NEEDS_WEAPON	0"
		"		TASK_WAIT_FOR_SPEAK_FINISH		1"
		"		TASK_WAIT						8"
		"		"
		"	Interrupts"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
		SCHED_LEAD_SPEAK_START,

		"	Tasks"
		"		TASK_LEAD_SPEAK_START			0"
		"		TASK_WAIT_FOR_SPEAK_FINISH		1"
		""
		"	Interrupts"
	)

	//---------------------------------

	DEFINE_SCHEDULE
	( 
	SCHED_LEAD_SPEAK_THEN_LEAD_PLAYER,

	"	Tasks"
	"		TASK_STOP_MOVING			0"
	"		TASK_WAIT_FOR_SPEAK_FINISH	1"
	"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_LEAD_PLAYER"
	""
	"	Interrupts"
	"		COND_LEAD_FOLLOWER_LOST"
	"		COND_LEAD_FOLLOWER_LAGGING"
	"		COND_LIGHT_DAMAGE"
	"		COND_NEW_ENEMY"
	"		COND_HEAR_DANGER"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()


//-----------------------------------------------------------------------------
//
// Purpose: A level tool to control the lead behavior. Use is not required
//			in order to use behavior.
//

class CAI_LeadGoal : public CAI_GoalEntity,
					 public CAI_LeadBehaviorHandler
{
	DECLARE_CLASS( CAI_LeadGoal, CAI_GoalEntity );
public:
	CAI_LeadGoal()
	 :	m_fArrived( false )
	{
		// These fields got added after existing levels shipped, so we set
		// the default values here in the constructor.
		m_iRetrievePlayer = 1;
		m_iRetrieveWaitForSpeak = 0;
		m_iComingBackWaitForSpeak = 0;
		m_bStopScenesWhenPlayerLost = false;
		m_bDontSpeakStart = false;
		m_bLeadDuringCombat = false;
		m_bGagLeader = false;
	}

	CAI_LeadBehavior *GetLeadBehavior();

	virtual const char *GetConceptModifiers( const char *pszConcept );

	virtual void InputActivate( inputdata_t &inputdata );
	virtual void InputDeactivate( inputdata_t &inputdata );
	
	DECLARE_DATADESC();
private:

	virtual void OnEvent( int event );
	void InputSetSuccess( inputdata_t &inputdata );
	void InputSetFailure( inputdata_t &inputdata );
	
	bool	 m_fArrived; // @TODO (toml 08-16-02): move arrived tracking onto behavior
	float	 m_flWaitDistance;
	float	 m_flLeadDistance;
	float	 m_flRetrieveDistance;
	float	 m_flSuccessDistance;
	bool	 m_bRun;
	int		 m_iRetrievePlayer;
	int		 m_iRetrieveWaitForSpeak;
	int		 m_iComingBackWaitForSpeak;
	bool	 m_bStopScenesWhenPlayerLost;
	bool	 m_bDontSpeakStart;
	bool	 m_bLeadDuringCombat;
	bool	 m_bGagLeader;

	string_t m_iszWaitPointName;

	string_t m_iszStartConceptModifier;
	string_t m_iszAttractPlayerConceptModifier;
	string_t m_iszWaitOverConceptModifier;
	string_t m_iszArrivalConceptModifier;
	string_t m_iszPostArrivalConceptModifier;
	string_t m_iszSuccessConceptModifier;
	string_t m_iszFailureConceptModifier;
	string_t m_iszRetrieveConceptModifier;
	string_t m_iszComingBackConceptModifier;

	// Output handlers
	COutputEvent	m_OnArrival;
	COutputEvent	m_OnArrivalDone;
	COutputEvent	m_OnSuccess;
	COutputEvent	m_OnFailure;
	COutputEvent	m_OnDone;
};

//-----------------------------------------------------------------------------
//
// CAI_LeadGoal implementation
//

LINK_ENTITY_TO_CLASS( ai_goal_lead, CAI_LeadGoal );

BEGIN_DATADESC( CAI_LeadGoal )

	DEFINE_FIELD( m_fArrived, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD(m_flWaitDistance, 		FIELD_FLOAT, 	"WaitDistance"),
	DEFINE_KEYFIELD(m_iszWaitPointName, 	FIELD_STRING, 	"WaitPointName"),
	DEFINE_KEYFIELD(m_flLeadDistance, 		FIELD_FLOAT, 	"LeadDistance"),
	DEFINE_KEYFIELD(m_flRetrieveDistance, 	FIELD_FLOAT, 	"RetrieveDistance"),
	DEFINE_KEYFIELD(m_flSuccessDistance, 	FIELD_FLOAT, 	"SuccessDistance"),
	DEFINE_KEYFIELD(m_bRun, 				FIELD_BOOLEAN, 	"Run"),
	DEFINE_KEYFIELD(m_iRetrievePlayer,		FIELD_INTEGER,	"Retrieve"),
	DEFINE_KEYFIELD(m_iRetrieveWaitForSpeak,		FIELD_INTEGER,	"RetrieveWaitForSpeak"),
	DEFINE_KEYFIELD(m_iComingBackWaitForSpeak,		FIELD_INTEGER,	"ComingBackWaitForSpeak"),
	DEFINE_KEYFIELD(m_bStopScenesWhenPlayerLost,	FIELD_BOOLEAN,	"StopScenes"),
	DEFINE_KEYFIELD(m_bDontSpeakStart,	FIELD_BOOLEAN,	"DontSpeakStart"),
	DEFINE_KEYFIELD(m_bLeadDuringCombat, FIELD_BOOLEAN, "LeadDuringCombat"),
	DEFINE_KEYFIELD(m_bGagLeader, FIELD_BOOLEAN, "GagLeader"),

	DEFINE_KEYFIELD(m_iszStartConceptModifier,			FIELD_STRING, 	"StartConceptModifier"),
	DEFINE_KEYFIELD(m_iszAttractPlayerConceptModifier,	FIELD_STRING, 	"AttractPlayerConceptModifier"),
	DEFINE_KEYFIELD(m_iszWaitOverConceptModifier, 		FIELD_STRING, 	"WaitOverConceptModifier"),
	DEFINE_KEYFIELD(m_iszArrivalConceptModifier, 		FIELD_STRING, 	"ArrivalConceptModifier"),
	DEFINE_KEYFIELD(m_iszPostArrivalConceptModifier,	FIELD_STRING,	"PostArrivalConceptModifier"),
	DEFINE_KEYFIELD(m_iszSuccessConceptModifier,		FIELD_STRING,	"SuccessConceptModifier"),
	DEFINE_KEYFIELD(m_iszFailureConceptModifier,		FIELD_STRING,	"FailureConceptModifier"),
	DEFINE_KEYFIELD(m_iszRetrieveConceptModifier,		FIELD_STRING,	"RetrieveConceptModifier"),
	DEFINE_KEYFIELD(m_iszComingBackConceptModifier,		FIELD_STRING,	"ComingBackConceptModifier"),

	DEFINE_OUTPUT( m_OnSuccess, 		"OnSuccess" ),
	DEFINE_OUTPUT( m_OnArrival, 		"OnArrival" ),
	DEFINE_OUTPUT( m_OnArrivalDone, 	"OnArrivalDone" ),
	DEFINE_OUTPUT( m_OnFailure, 		"OnFailure" ),
	DEFINE_OUTPUT( m_OnDone,	  		"OnDone" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetSuccess", InputSetSuccess ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetFailure", InputSetFailure ),

END_DATADESC()


//-----------------------------------------------------------------------------

CAI_LeadBehavior *CAI_LeadGoal::GetLeadBehavior()
{
	CAI_BaseNPC *pActor = GetActor();
	if ( !pActor )
		return NULL;

	CAI_LeadBehavior *pBehavior;
	if ( !pActor->GetBehavior( &pBehavior ) )
	{
		return NULL;
	}
	
	return pBehavior;
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputSetSuccess( inputdata_t &inputdata )
{
	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
		return;
		
	// @TODO (toml 02-14-03): Hackly!
	pBehavior->SetCondition( CAI_LeadBehavior::COND_LEAD_SUCCESS);
}


//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputSetFailure( inputdata_t &inputdata )
{
	DevMsg( "SetFailure unimplemented\n" );
}


//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputActivate( inputdata_t &inputdata )
{
	BaseClass::InputActivate( inputdata );
	
	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
	{
		DevMsg( "Lead goal entity activated for an NPC that doesn't have the lead behavior\n" );
		return;
	}
#ifdef HL2_EPISODIC
 	if ( (m_flLeadDistance*4) < m_flRetrieveDistance )
	{
		Warning("ai_goal_lead '%s': lead distance (%.2f) * 4 is < retrieve distance (%.2f). This will make the NPC act stupid. Either reduce the retrieve distance, or increase the lead distance.\n", GetDebugName(), m_flLeadDistance, m_flRetrieveDistance );
	}
#endif

	if ( m_flRetrieveDistance < (m_flLeadDistance + LEAD_MIN_RETRIEVEDIST_OFFSET) )
	{
#ifdef HL2_EPISODIC
		Warning("ai_goal_lead '%s': retrieve distance (%.2f) < lead distance (%.2f) + %d. Retrieve distance should be at least %d greater than the lead distance, or NPC will ping-pong while retrieving.\n", GetDebugName(), m_flRetrieveDistance, m_flLeadDistance, LEAD_MIN_RETRIEVEDIST_OFFSET, LEAD_MIN_RETRIEVEDIST_OFFSET );
#endif
		m_flRetrieveDistance = m_flLeadDistance + LEAD_MIN_RETRIEVEDIST_OFFSET;
	}

	AI_LeadArgs_t leadArgs = { 
		GetGoalEntityName(), 
		STRING(m_iszWaitPointName), 
		m_spawnflags, 
		m_flWaitDistance, 
		m_flLeadDistance, 
		m_flRetrieveDistance, 
		m_flSuccessDistance,
		m_bRun, 
		m_iRetrievePlayer, 
		m_iRetrieveWaitForSpeak, 
		m_iComingBackWaitForSpeak, 
		m_bStopScenesWhenPlayerLost,
		m_bDontSpeakStart,
		m_bLeadDuringCombat,
		m_bGagLeader,
	};
	
	pBehavior->LeadPlayer( leadArgs, this );
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::InputDeactivate( inputdata_t &inputdata )
{
	BaseClass::InputDeactivate( inputdata );

	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( !pBehavior )
		return;
		
	pBehavior->StopLeading();
}

//-----------------------------------------------------------------------------

void CAI_LeadGoal::OnEvent( int event )
{
	COutputEvent *pOutputEvent = NULL;

	switch ( event )
	{
		case LBE_ARRIVAL:		pOutputEvent = &m_OnArrival;		break;
		case LBE_ARRIVAL_DONE:	pOutputEvent = &m_OnArrivalDone;	break;
		case LBE_SUCCESS:		pOutputEvent = &m_OnSuccess;		break;
		case LBE_FAILURE:		pOutputEvent = &m_OnFailure;		break;
		case LBE_DONE:			pOutputEvent = &m_OnDone;			break;
	}
	
	// @TODO (toml 08-16-02): move arrived tracking onto behavior
	if ( event == LBE_ARRIVAL )
		m_fArrived = true;

	if ( pOutputEvent )
		pOutputEvent->FireOutput( this, this );
}

//-----------------------------------------------------------------------------

const char *CAI_LeadGoal::GetConceptModifiers( const char *pszConcept )	
{ 
	if ( m_iszStartConceptModifier != NULL_STRING && *STRING(m_iszStartConceptModifier) && strcmp( pszConcept, TLK_LEAD_START) == 0 )
		return STRING( m_iszStartConceptModifier );

	if ( m_iszAttractPlayerConceptModifier != NULL_STRING && *STRING(m_iszAttractPlayerConceptModifier) && strcmp( pszConcept, TLK_LEAD_ATTRACTPLAYER) == 0 )
		return STRING( m_iszAttractPlayerConceptModifier );

	if ( m_iszWaitOverConceptModifier != NULL_STRING && *STRING(m_iszWaitOverConceptModifier) && strcmp( pszConcept, TLK_LEAD_WAITOVER) == 0 )
		return STRING( m_iszWaitOverConceptModifier );

	if ( m_iszArrivalConceptModifier != NULL_STRING && *STRING(m_iszArrivalConceptModifier) && strcmp( pszConcept, TLK_LEAD_ARRIVAL) == 0 )
		return STRING( m_iszArrivalConceptModifier );
		
	if ( m_iszSuccessConceptModifier != NULL_STRING && *STRING(m_iszSuccessConceptModifier) && strcmp( pszConcept, TLK_LEAD_SUCCESS) == 0 )
		return STRING( m_iszSuccessConceptModifier );
		
	if (m_iszFailureConceptModifier != NULL_STRING && *STRING(m_iszFailureConceptModifier) && strcmp( pszConcept, TLK_LEAD_FAILURE) == 0 )
		return STRING( m_iszFailureConceptModifier );
	
	if (m_iszRetrieveConceptModifier != NULL_STRING && *STRING(m_iszRetrieveConceptModifier) && strcmp( pszConcept, TLK_LEAD_RETRIEVE) == 0 )
		return STRING( m_iszRetrieveConceptModifier );

	if (m_iszComingBackConceptModifier != NULL_STRING && *STRING(m_iszComingBackConceptModifier) && strcmp( pszConcept, TLK_LEAD_COMINGBACK) == 0 )
		return STRING( m_iszComingBackConceptModifier );

	if ( m_fArrived && m_iszPostArrivalConceptModifier != NULL_STRING && *STRING(m_iszPostArrivalConceptModifier) )
		return STRING( m_iszPostArrivalConceptModifier );
	
	return NULL; 
}


//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Purpose: A custom lead goal that waits until the player has a weapon.
//

class CAI_LeadGoal_Weapon : public CAI_LeadGoal
{
	DECLARE_CLASS( CAI_LeadGoal_Weapon, CAI_LeadGoal );
public:

	virtual const char *GetConceptModifiers( const char *pszConcept );
	virtual void InputActivate( inputdata_t &inputdata );

private:
	string_t	m_iszWeaponName;
	string_t	m_iszMissingWeaponConceptModifier;

	DECLARE_DATADESC();
};

//-----------------------------------------------------------------------------
//
// CAI_LeadGoal_Weapon implementation
//

LINK_ENTITY_TO_CLASS( ai_goal_lead_weapon, CAI_LeadGoal_Weapon );

BEGIN_DATADESC( CAI_LeadGoal_Weapon )

	DEFINE_KEYFIELD( m_iszWeaponName, 		FIELD_STRING, 	"WeaponName"),
	DEFINE_KEYFIELD( m_iszMissingWeaponConceptModifier, FIELD_STRING, 	"MissingWeaponConceptModifier"),

END_DATADESC()

//-----------------------------------------------------------------------------

const char *CAI_LeadGoal_Weapon::GetConceptModifiers( const char *pszConcept )	
{ 
	if ( m_iszMissingWeaponConceptModifier != NULL_STRING && *STRING(m_iszMissingWeaponConceptModifier) && strcmp( pszConcept, TLK_LEAD_MISSINGWEAPON) == 0 )
		return STRING( m_iszMissingWeaponConceptModifier );

	return BaseClass::GetConceptModifiers( pszConcept ); 
}


//-----------------------------------------------------------------------------

void CAI_LeadGoal_Weapon::InputActivate( inputdata_t &inputdata )
{
	BaseClass::InputActivate( inputdata );

	CAI_LeadBehavior *pBehavior = GetLeadBehavior();
	if ( pBehavior )
	{
		pBehavior->SetWaitForWeapon( m_iszWeaponName );
	}
}
