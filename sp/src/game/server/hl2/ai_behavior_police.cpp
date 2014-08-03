//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

#include "ai_behavior_police.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "collisionutils.h"
#include "npc_metropolice.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAI_PolicingBehavior )

	DEFINE_FIELD( m_bEnabled,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bStartPolicing,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hPoliceGoal,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_flNextHarassTime,	FIELD_TIME ),
	DEFINE_FIELD( m_flAggressiveTime,	FIELD_TIME ),
	DEFINE_FIELD( m_nNumWarnings,		FIELD_INTEGER ),
	DEFINE_FIELD( m_bTargetIsHostile,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTargetHostileTime,FIELD_TIME ),

END_DATADESC();

CAI_PolicingBehavior::CAI_PolicingBehavior( void )
{
	m_bEnabled = false;
	m_nNumWarnings = 0;
	m_bTargetIsHostile = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PolicingBehavior::TargetIsHostile( void )
{
	if ( ( m_flTargetHostileTime < gpGlobals->curtime ) && ( !m_bTargetIsHostile ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pGoal - 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::Enable( CAI_PoliceGoal *pGoal )
{
	m_hPoliceGoal = pGoal;
	m_bEnabled = true;

	m_bStartPolicing = true;

	// Update ourselves immediately
	GetOuter()->ClearSchedule( "Enable police behavior" );
	//NotifyChangeBehaviorStatus( GetOuter()->IsInAScript() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::Disable( void )
{
	m_hPoliceGoal = NULL;
	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PolicingBehavior::CanSelectSchedule( void )
{
	// Must be activated and valid
	if ( IsEnabled() == false || !m_hPoliceGoal || !m_hPoliceGoal->GetTarget() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : false - 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::HostSetBatonState( bool state )
{
	// If we're a cop, turn the baton on
	CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(GetOuter());

	if ( pCop != NULL )
	{
		pCop->SetBatonState( state );
		pCop->SetTarget( m_hPoliceGoal->GetTarget() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : false - 
//-----------------------------------------------------------------------------
bool CAI_PolicingBehavior::HostBatonIsOn( void )
{
	// If we're a cop, turn the baton on
	CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(GetOuter());
	if ( pCop )
		return pCop->BatonActive();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::HostSpeakSentence( const char *pSentence, SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria )
{
	// If we're a cop, turn the baton on
	CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(GetOuter());

	if ( pCop != NULL )
	{
		CAI_Sentence< CNPC_MetroPolice > *pSentences = pCop->GetSentences();

		pSentences->Speak( pSentence, nSoundPriority, nCriteria );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::BuildScheduleTestBits( void )
{
	if ( IsCurSchedule( SCHED_IDLE_STAND ) || IsCurSchedule( SCHED_ALERT_STAND ) )
	{
		if ( m_flNextHarassTime < gpGlobals->curtime )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_POLICE_TARGET_TOO_CLOSE_HARASS ) );
		}

		GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Mapmaker may have removed our goal while we're running our schedule
	if ( !m_hPoliceGoal )
	{
		Disable();
		return;
	}

	ClearCondition( COND_POLICE_TARGET_TOO_CLOSE_HARASS );
	ClearCondition( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS );

	CBaseEntity *pTarget = m_hPoliceGoal->GetTarget();

	if ( pTarget == NULL )
	{
		DevMsg( "ai_goal_police with NULL target entity!\n" );
		return;
	}

	// See if we need to knock out our target immediately
	if ( ShouldKnockOutTarget( pTarget ) )
	{
		SetCondition( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS );
	}

	float flDistSqr = ( m_hPoliceGoal->WorldSpaceCenter() - pTarget->WorldSpaceCenter() ).Length2DSqr();
	float radius = ( m_hPoliceGoal->GetRadius() * PATROL_RADIUS_RATIO );
	float zDiff = fabs( m_hPoliceGoal->WorldSpaceCenter().z - pTarget->WorldSpaceCenter().z );

	// If we're too far away, don't bother
	if ( flDistSqr < (radius*radius) && zDiff < 32.0f )
	{
		SetCondition( COND_POLICE_TARGET_TOO_CLOSE_HARASS );

		if ( flDistSqr < (m_hPoliceGoal->GetRadius()*m_hPoliceGoal->GetRadius()) )
		{
			SetCondition( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS );
		}
	}

	// If we're supposed to stop chasing (aggression over), return
	if ( m_bTargetIsHostile && m_flAggressiveTime < gpGlobals->curtime && IsCurSchedule(SCHED_CHASE_ENEMY) )
	{
		// Force me to re-evaluate my schedule
		GetOuter()->ClearSchedule( "Stopped chasing, aggression over" );
	}
}

//-----------------------------------------------------------------------------
// We're taking cover from danger
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::AnnouncePolicing( void )
{
	// We're policing
	static const char *pWarnings[3] = 
	{
		"METROPOLICE_MOVE_ALONG_A",
		"METROPOLICE_MOVE_ALONG_B",
		"METROPOLICE_MOVE_ALONG_C",
	};

	if ( m_nNumWarnings <= 3 )
	{
		HostSpeakSentence( pWarnings[ m_nNumWarnings - 1 ], SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
	}
	else 
	{
		// We loop at m_nNumWarnings == 4 for players who aren't moving 
		// but still pissing us off, and we're not allowed to do anything about it. (i.e. can't leave post)
		// First two sentences sound pretty good, so randomly pick one of them.
		int iSentence = RandomInt( 0, 1 );
		HostSpeakSentence( pWarnings[ iSentence ], SENTENCE_PRIORITY_MEDIUM, SENTENCE_CRITERIA_NORMAL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : scheduleType - 
// Output : int
//-----------------------------------------------------------------------------
int CAI_PolicingBehavior::TranslateSchedule( int scheduleType )
{
	if ( scheduleType == SCHED_CHASE_ENEMY )
	{
		if ( m_hPoliceGoal->ShouldRemainAtPost() && !MaintainGoalPosition() )
			return BaseClass::TranslateSchedule( SCHED_COMBAT_FACE );
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : newActivity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CAI_PolicingBehavior::NPC_TranslateActivity( Activity newActivity )
{
	// See which harassment to play
	if ( newActivity == ACT_POLICE_HARASS1 )
	{
		switch( m_nNumWarnings )
		{
		case 1:
			return (Activity) ACT_POLICE_HARASS1;
			break;

		default:
		case 2:
			return (Activity) ACT_POLICE_HARASS2;
			break;
		}
	}

	return BaseClass::NPC_TranslateActivity( newActivity );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CAI_PolicingBehavior::GetGoalTarget( void )
{
	if ( m_hPoliceGoal == NULL )
	{
		//NOTENOTE: This has been called before the behavior is actually active, or the goal has gone invalid
		Assert(0);
		return NULL;
	}

	return m_hPoliceGoal->GetTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::SetTargetHostileDuration( float time )
{
	m_flTargetHostileTime = gpGlobals->curtime + time;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_POLICE_GET_PATH_TO_HARASS_GOAL:
		{
			Vector	harassDir = ( m_hPoliceGoal->GetTarget()->WorldSpaceCenter() - WorldSpaceCenter() );
			float	flDist = VectorNormalize( harassDir );

			// See if we're already close enough
			if ( flDist < pTask->flTaskData )
			{
				TaskComplete();
				break;
			}

			float	flInter1, flInter2;
			Vector	harassPos = GetAbsOrigin() + ( harassDir * ( flDist - pTask->flTaskData ) );

			// Find a point on our policing radius to stand on
			if ( IntersectInfiniteRayWithSphere( GetAbsOrigin(), harassDir, m_hPoliceGoal->GetAbsOrigin(), m_hPoliceGoal->GetRadius(), &flInter1, &flInter2 ) )
			{
				Vector vPos = m_hPoliceGoal->GetAbsOrigin() + harassDir * ( MAX( flInter1, flInter2 ) );

				// See how far away the default one is
				float testDist = UTIL_DistApprox2D( m_hPoliceGoal->GetAbsOrigin(), harassPos );
				
				// If our other goal is closer, choose it
				if ( testDist > UTIL_DistApprox2D( m_hPoliceGoal->GetAbsOrigin(), vPos ) )
				{
					harassPos = vPos;
				}
			}

			if ( GetNavigator()->SetGoal( harassPos, pTask->flTaskData ) )
			{
				GetNavigator()->SetMovementActivity( (Activity) ACT_WALK_ANGRY );
				GetNavigator()->SetArrivalDirection( m_hPoliceGoal->GetTarget() );
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;
	
	case TASK_POLICE_GET_PATH_TO_POLICE_GOAL:
		{
			if ( GetNavigator()->SetGoal( m_hPoliceGoal->GetAbsOrigin(), pTask->flTaskData ) )
			{
				GetNavigator()->SetArrivalDirection( m_hPoliceGoal->GetAbsAngles() );
				TaskComplete();
			}
			else
			{
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_POLICE_ANNOUNCE_HARASS:
		{
			AnnouncePolicing();

			// Randomly say this again in the future
			m_flNextHarassTime = gpGlobals->curtime + random->RandomInt( 4, 6 );

			// Scatter rubber-neckers
			CSoundEnt::InsertSound( SOUND_MOVE_AWAY, GetAbsOrigin(), 256.0f, 2.0f, GetOuter() );
		}
		TaskComplete();
		break;

	case TASK_POLICE_FACE_ALONG_GOAL:
		{
			// We may have lost our police goal in the 2 seconds we wait before this task
			if ( m_hPoliceGoal )
			{
				GetMotor()->SetIdealYaw( m_hPoliceGoal->GetAbsAngles().y );
				GetOuter()->SetTurnActivity(); 
			}
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::RunTask( const Task_t *pTask )		
{ 
	switch ( pTask->iTask )
	{
	case TASK_POLICE_FACE_ALONG_GOAL:
		{
   			GetMotor()->UpdateYaw();
   
   			if ( GetOuter()->FacingIdeal() )
   			{
   				TaskComplete();
   			}
   			break;
		}

	default:
		BaseClass::RunTask( pTask);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PolicingBehavior::MaintainGoalPosition( void )
{
	Vector vecOrg = GetAbsOrigin();
	Vector vecTarget = m_hPoliceGoal->GetAbsOrigin();

	// Allow some slop on Z
	if ( fabs(vecOrg.z - vecTarget.z) > 64 )
		return true;

	// Need to be very close on X/Y
	if ( (vecOrg - vecTarget).Length2D() > 16 )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_PolicingBehavior::ShouldKnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_hPoliceGoal == NULL )
	{
		//NOTENOTE: This has been called before the behavior is actually active, or the goal has gone invalid
		Assert(0);
		return false;
	}

	bool bVisible = GetOuter()->FVisible( pTarget );
	return m_hPoliceGoal->ShouldKnockOutTarget( pTarget->WorldSpaceCenter(), bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAI_PolicingBehavior::KnockOutTarget( CBaseEntity *pTarget )
{
	if ( m_hPoliceGoal == NULL )
	{
		//NOTENOTE: This has been called before the behavior is actually active, or the goal has gone invalid
		Assert(0);
		return;
	}

	m_hPoliceGoal->KnockOutTarget( pTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_PolicingBehavior::SelectSuppressSchedule( void )
{
	CBaseEntity *pTarget = m_hPoliceGoal->GetTarget();

	m_flAggressiveTime = gpGlobals->curtime + 4.0f;

	if ( m_bTargetIsHostile == false )
	{
		// Mark this as a valid target
		m_bTargetIsHostile = true;
		
		// Attack the target
		GetOuter()->SetEnemy( pTarget );
		GetOuter()->SetState( NPC_STATE_COMBAT );
		GetOuter()->UpdateEnemyMemory( pTarget, pTarget->GetAbsOrigin() );

		HostSetBatonState( true );
		
		// Remember that we're angry with the target
		m_nNumWarnings = POLICE_MAX_WARNINGS;

		// We need to let the system pickup the new enemy and deal with it on the next frame
		return SCHED_COMBAT_FACE;
	}

	// If we're supposed to stand still, then we need to show aggression
	if ( m_hPoliceGoal->ShouldRemainAtPost() )
	{
		// If we're off our mark, fight to it
		if ( MaintainGoalPosition() )
		{
			return SCHED_CHASE_ENEMY;
		}

		//FIXME: This needs to be a more aggressive warning to the player
		if ( m_flNextHarassTime < gpGlobals->curtime )
		{
			return SCHED_POLICE_WARN_TARGET;
		}
		else
		{
			return SCHED_COMBAT_FACE;
		}
	}

	return SCHED_CHASE_ENEMY;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_PolicingBehavior::SelectHarassSchedule( void )
{
	CBaseEntity *pTarget = m_hPoliceGoal->GetTarget();

	m_flAggressiveTime = gpGlobals->curtime + 4.0f;

	// If we just started to police, make sure we're on our mark
	if ( MaintainGoalPosition() )
		return SCHED_POLICE_RETURN_FROM_HARASS;
	
	// Look at the target if they're too close
	GetOuter()->AddLookTarget( pTarget, 0.5f, 5.0f );

	// Say something if it's been long enough
	if ( m_flNextHarassTime < gpGlobals->curtime )
	{
		// Gesture the player away
		GetOuter()->SetTarget( pTarget );
		
		// Send outputs for each level of warning
		if ( m_nNumWarnings == 0 )
		{
			m_hPoliceGoal->FireWarningLevelOutput( 1 );
		}
		else if ( m_nNumWarnings == 1 )
		{
			m_hPoliceGoal->FireWarningLevelOutput( 2 );
		}

		if ( m_nNumWarnings < POLICE_MAX_WARNINGS )
		{
			m_nNumWarnings++;
		}

		// If we're over our limit, just suppress the offender
		if ( m_nNumWarnings >= POLICE_MAX_WARNINGS )
		{
			if ( m_bTargetIsHostile == false )
			{
				// Mark the target as a valid target
				m_bTargetIsHostile = true;

				GetOuter()->SetEnemy( pTarget );
				GetOuter()->SetState( NPC_STATE_COMBAT );
				GetOuter()->UpdateEnemyMemory( pTarget, pTarget->GetAbsOrigin() );
				HostSetBatonState( true );
				
				m_hPoliceGoal->FireWarningLevelOutput( 4 );

				return SCHED_COMBAT_FACE;
			}

			if ( m_hPoliceGoal->ShouldRemainAtPost() == false )
				return SCHED_CHASE_ENEMY;
		}
			
		// On our last warning, approach the target
		if ( m_nNumWarnings == (POLICE_MAX_WARNINGS-1) )
		{
			m_hPoliceGoal->FireWarningLevelOutput( 3 );

			GetOuter()->SetTarget( pTarget );
			
			HostSetBatonState( true );

			if ( m_hPoliceGoal->ShouldRemainAtPost() == false )
				return SCHED_POLICE_HARASS_TARGET;
		}

		// Otherwise just verbally warn him
		return SCHED_POLICE_WARN_TARGET;
	}

	return SCHED_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_PolicingBehavior::SelectSchedule( void )
{
	CBaseEntity *pTarget = m_hPoliceGoal->GetTarget();

	// Validate our target
	if ( pTarget == NULL )
	{
		DevMsg( "ai_goal_police with NULL target entity!\n" );
		
		// Turn us off
		Disable();
		return SCHED_NONE;
	}

	// Attack if we're supposed to
	if ( ( m_flAggressiveTime >= gpGlobals->curtime ) && HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	// See if we should immediately begin to attack our target
	if ( HasCondition( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS ) )
	{
		return SelectSuppressSchedule();
	}
	
	int newSchedule = SCHED_NONE;

	// See if we're harassing
	if ( HasCondition( COND_POLICE_TARGET_TOO_CLOSE_HARASS ) )
	{
		newSchedule = SelectHarassSchedule();
	}

	// Return that schedule if it was found
	if ( newSchedule != SCHED_NONE )
		return newSchedule;

	// If our enemy is set, fogeda'bout it!
	if ( m_flAggressiveTime < gpGlobals->curtime )
	{
		// Return to your initial spot
		if ( GetEnemy() )
		{
			GetOuter()->SetEnemy( NULL );
			GetOuter()->SetState( NPC_STATE_ALERT );
			GetOuter()->GetEnemies()->RefreshMemories();
		}

		HostSetBatonState( false );
		m_bTargetIsHostile = false;
	}	

	// If we just started to police, make sure we're on our mark
	if ( MaintainGoalPosition() )
		return SCHED_POLICE_RETURN_FROM_HARASS;

	// If I've got my baton on, keep looking at the target
	if ( HostBatonIsOn() )
		return SCHED_POLICE_TRACK_TARGET;

	// Re-align myself to the goal angles if I've strayed
	if ( fabs(UTIL_AngleDiff( GetAbsAngles().y, m_hPoliceGoal->GetAbsAngles().y )) > 15 )
		return SCHED_POLICE_FACE_ALONG_GOAL;

	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CAI_PolicingBehavior::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule == SCHED_CHASE_ENEMY )
	{
		// We've failed to chase our enemy, return to where we were came from
		if ( MaintainGoalPosition() )
			return SCHED_POLICE_RETURN_FROM_HARASS;

		return SCHED_POLICE_WARN_TARGET;
	}
	
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_PolicingBehavior )

	DECLARE_CONDITION( COND_POLICE_TARGET_TOO_CLOSE_HARASS );
	DECLARE_CONDITION( COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS );

	DECLARE_TASK( TASK_POLICE_GET_PATH_TO_HARASS_GOAL );
	DECLARE_TASK( TASK_POLICE_GET_PATH_TO_POLICE_GOAL );
	DECLARE_TASK( TASK_POLICE_ANNOUNCE_HARASS );
	DECLARE_TASK( TASK_POLICE_FACE_ALONG_GOAL );

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_WARN_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_TARGET					0"
		"		TASK_POLICE_ANNOUNCE_HARASS	0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_POLICE_HARASS1"
		""
		"	Interrupts"
		"		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS"
	);

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_HARASS_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_FACE_TARGET							0"
		"		TASK_POLICE_GET_PATH_TO_HARASS_GOAL			64"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_POLICE_ANNOUNCE_HARASS			0"
		"		TASK_PLAY_SEQUENCE							ACTIVITY:ACT_POLICE_HARASS1"
		""
		"	Interrupts"
		"		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS"
	);

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_SUPPRESS_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_TARGET					0"
		"		TASK_POLICE_ANNOUNCE_HARASS	0"
		"		TASK_PLAY_SEQUENCE					ACTIVITY:ACT_POLICE_HARASS1"
		""
		"	Interrupts"
	);

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_RETURN_FROM_HARASS,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_POLICE_GET_PATH_TO_POLICE_GOAL			16"
		"		TASK_WALK_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_STOP_MOVING							0"
		""
		"	Interrupts"
		"		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS"
	);

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_TRACK_TARGET,

		"	Tasks"
		"		TASK_FACE_TARGET					0"
		""
		"	Interrupts"
		"		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS"
	);

	DEFINE_SCHEDULE
	(
		SCHED_POLICE_FACE_ALONG_GOAL,

		"	Tasks"
		"		TASK_WAIT_RANDOM							2"
		"		TASK_POLICE_FACE_ALONG_GOAL					0"
		""
		"	Interrupts"
		"		COND_POLICE_TARGET_TOO_CLOSE_SUPPRESS"
	);

AI_END_CUSTOM_SCHEDULE_PROVIDER()
