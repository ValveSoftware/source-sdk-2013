//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_motor.h"
#include "ai_behavior_fear.h"
#include "ai_hint.h"
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAI_FearBehavior )
	DEFINE_FIELD( m_flTimeToSafety, FIELD_TIME ),
	DEFINE_FIELD( m_flTimePlayerLastVisible, FIELD_TIME ),
	DEFINE_FIELD( m_hSafePlaceHint, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hMovingToHint, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_SafePlaceMoveMonitor ),
	DEFINE_FIELD( m_flDeferUntil, FIELD_TIME ),
END_DATADESC();

#define BEHAVIOR_FEAR_SAFETY_TIME		5
#define FEAR_SAFE_PLACE_TOLERANCE		36.0f
#define FEAR_ENEMY_TOLERANCE_CLOSE_DIST_SQR		Square(300.0f) // (25 feet)
#define FEAR_ENEMY_TOLERANCE_TOO_CLOSE_DIST_SQR	Square( 60.0f ) // (5 Feet)

ConVar ai_enable_fear_behavior( "ai_enable_fear_behavior", "1" );

ConVar ai_fear_player_dist("ai_fear_player_dist", "720" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_FearBehavior::CAI_FearBehavior()
{
	ReleaseAllHints();
	m_SafePlaceMoveMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::Precache( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FEAR_IN_SAFE_PLACE:
		// We've arrived! Lock the hint and set the marker. we're safe for now.
		m_hSafePlaceHint = m_hMovingToHint;
		m_hSafePlaceHint->Lock( GetOuter() );
		m_SafePlaceMoveMonitor.SetMark( GetOuter(), FEAR_SAFE_PLACE_TOLERANCE );
		TaskComplete();
		break;

	case TASK_FEAR_GET_PATH_TO_SAFETY_HINT:
		// Using TaskInterrupt() optimizations. See RunTask().
		break;

	case TASK_FEAR_WAIT_FOR_SAFETY:
		m_flTimeToSafety = gpGlobals->curtime + BEHAVIOR_FEAR_SAFETY_TIME;
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_FearBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FEAR_WAIT_FOR_SAFETY:
		if( HasCondition(COND_SEE_ENEMY) )
		{
			m_flTimeToSafety = gpGlobals->curtime + BEHAVIOR_FEAR_SAFETY_TIME;
		}
		else
		{
			if( gpGlobals->curtime > m_flTimeToSafety )
			{
				TaskComplete();
			}
		}
		break;

	case TASK_FEAR_GET_PATH_TO_SAFETY_HINT:
		{
			switch( GetOuter()->GetTaskInterrupt() )
			{
			case 0:// Find the hint node
				{
					ReleaseAllHints();
					CAI_Hint *pHint = FindFearWithdrawalDest();

					if( pHint == NULL )
					{
						TaskFail("Fear: Couldn't find hint node\n");
						m_flDeferUntil = gpGlobals->curtime + 3.0f;// Don't bang the hell out of this behavior. If we don't find a node, take a short break and run regular AI.
					}
					else
					{
						m_hMovingToHint.Set( pHint );
						GetOuter()->TaskInterrupt();
					}
				}
				break;

			case 1:// Do the pathfinding.
				{
					Assert( m_hMovingToHint != NULL );

					AI_NavGoal_t goal(m_hMovingToHint->GetAbsOrigin());
					goal.pTarget = NULL;
					if( GetNavigator()->SetGoal( goal ) == false )
					{
						m_hMovingToHint.Set( NULL );
						// Do whatever we'd want to do if we can't find a path
						/*
						Msg("Can't path to the Fear Hint!\n");

						AI_NavGoal_t nearGoal( GOALTYPE_LOCATION_NEAREST_NODE, m_hRallyPoint->GetAbsOrigin(), AIN_DEF_ACTIVITY, 256 );
						if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
						{
						//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
						ClearCondition( COND_TASK_FAILED );
						GetNavigator()->SetArrivalDirection( m_hRallyPoint->GetAbsAngles() );
						TaskComplete();
						return;
						}
						*/
					}
					else
					{
						GetNavigator()->SetArrivalDirection( m_hMovingToHint->GetAbsAngles() );
					}
				}
				break;
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : TRUE if I have an enemy and that enemy would attack me if it could
// Notes  : Returns FALSE if the enemy is neutral or likes me.
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::EnemyDislikesMe()
{
	CBaseEntity *pEnemy = GetEnemy();

	if( pEnemy == NULL )
		return false;

	if( pEnemy->MyNPCPointer() == NULL )
		return false;

	Disposition_t disposition = pEnemy->MyNPCPointer()->IRelationType(GetOuter());

	Assert(disposition != D_ER);

	if( disposition >= D_LI )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// This place is definitely no longer safe. Stop picking it for a while.
//-----------------------------------------------------------------------------
void CAI_FearBehavior::MarkAsUnsafe()
{
	Assert( m_hSafePlaceHint );

	// Disable the node to stop anyone from picking it for a while.
	m_hSafePlaceHint->DisableForSeconds( 5.0f );
}

//-----------------------------------------------------------------------------
// Am I in safe place from my enemy?
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::IsInASafePlace()
{
	// No safe place in mind.
	if( !m_SafePlaceMoveMonitor.IsMarkSet() )
		return false;

	// I have a safe place, but I'm not there.
	if( m_SafePlaceMoveMonitor.TargetMoved(GetOuter()) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::SpoilSafePlace()
{
	m_SafePlaceMoveMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::ReleaseAllHints()
{
	if( m_hSafePlaceHint )
	{
		// If I have a safe place, unlock it for others.
		m_hSafePlaceHint->Unlock();

		// Don't make it available right away. I probably left for a good reason.
		// We also don't want to oscillate
		m_hSafePlaceHint->DisableForSeconds( 4.0f );
		m_hSafePlaceHint = NULL;
	}

	if( m_hMovingToHint )
	{
		m_hMovingToHint->Unlock();
		m_hMovingToHint = NULL;
	}

	m_SafePlaceMoveMonitor.ClearMark();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
// Notes  : This behavior runs when I have an enemy that I fear, but who
//			does NOT hate or fear me (meaning they aren't going to fight me)
//-----------------------------------------------------------------------------
bool CAI_FearBehavior::CanSelectSchedule()
{
	if( !GetOuter()->IsInterruptable() )
		return false;

	if( m_flDeferUntil > gpGlobals->curtime )
		return false;

	CBaseEntity *pEnemy = GetEnemy();

	if( pEnemy == NULL )
		return false;

	//if( !HasCondition(COND_SEE_PLAYER) )
	//	return false;

	if( !ai_enable_fear_behavior.GetBool() )
		return false;
	
	if( GetOuter()->IRelationType(pEnemy) != D_FR )
		return false;

	if( !pEnemy->ClassMatches("npc_hunter") )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::GatherConditions()
{
	BaseClass::GatherConditions();

	ClearCondition( COND_FEAR_ENEMY_CLOSE );
	ClearCondition( COND_FEAR_ENEMY_TOO_CLOSE );
	if( GetEnemy() )
	{
		float flEnemyDistSqr = GetAbsOrigin().DistToSqr(GetEnemy()->GetAbsOrigin());

		if( flEnemyDistSqr < FEAR_ENEMY_TOLERANCE_TOO_CLOSE_DIST_SQR )
		{
			SetCondition( COND_FEAR_ENEMY_TOO_CLOSE );
			if( IsInASafePlace() )
			{
				SpoilSafePlace();
			}
		}
		else if( flEnemyDistSqr < FEAR_ENEMY_TOLERANCE_CLOSE_DIST_SQR && GetEnemy()->GetEnemy() == GetOuter() )	
		{
			// Only become scared of an enemy at this range if they're my enemy, too
			SetCondition( COND_FEAR_ENEMY_CLOSE );
			if( IsInASafePlace() )
			{
				SpoilSafePlace();
			}
		}
	}

	ClearCondition(COND_FEAR_SEPARATED_FROM_PLAYER);

	// Check for separation from the player
	//	-The player is farther away than 60 feet
	//  -I haven't seen the player in 2 seconds
	//
	// Here's the distance check:
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	if( pPlayer != NULL && GetAbsOrigin().DistToSqr(pPlayer->GetAbsOrigin()) >= Square( ai_fear_player_dist.GetFloat() * 1.5f )  )
	{
		SetCondition(COND_FEAR_SEPARATED_FROM_PLAYER);
	}

	// Here's the visibility check. We can't skip this because it's time-sensitive
	if( GetOuter()->FVisible(pPlayer) )
	{
		m_flTimePlayerLastVisible = gpGlobals->curtime;
	}
	else
	{
		if( gpGlobals->curtime - m_flTimePlayerLastVisible >= 2.0f )
		{
			SetCondition(COND_FEAR_SEPARATED_FROM_PLAYER);
		}
	}

	if( HasCondition(COND_FEAR_SEPARATED_FROM_PLAYER) )
	{
		//Msg("I am separated from player\n");

		if( IsInASafePlace() )
		{
			SpoilSafePlace();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::BeginScheduleSelection()
{
	if( m_hSafePlaceHint )
	{
		// We think we're safe. Is it true?
		if( !IsInASafePlace() )
		{
			// no! So mark it so.
			ReleaseAllHints();
		}
	}

	m_flTimePlayerLastVisible = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::EndScheduleSelection()
{
	// We don't have to release our hints or markers or anything here. 
	// Just because we ran other AI for a while doesn't mean we aren't still in a safe place.
	//ReleaseAllHints();
}




//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
// Notes  : If fear behavior is running at all, we know we're afraid of our enemy
//-----------------------------------------------------------------------------
int CAI_FearBehavior::SelectSchedule()
{
	bool bInSafePlace = IsInASafePlace();

	if( !HasCondition(COND_HEAR_DANGER) )
	{
		if( !bInSafePlace )
		{
			// Always move to a safe place if we're not running from a danger sound
			return SCHED_FEAR_MOVE_TO_SAFE_PLACE;
		}
		else
		{
			// We ARE in a safe place
			if( HasCondition(COND_CAN_RANGE_ATTACK1) )
				return SCHED_RANGE_ATTACK1;

			return SCHED_FEAR_STAY_IN_SAFE_PLACE;
		}
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_FearBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if( GetOuter()->GetState() != NPC_STATE_SCRIPT )
	{
		// Stop doing ANYTHING if we get scared.
		//GetOuter()->SetCustomInterruptCondition( COND_HEAR_DANGER );

		if( !IsCurSchedule(SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY, false) && !IsCurSchedule(SCHED_FEAR_MOVE_TO_SAFE_PLACE, false) )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal(COND_FEAR_SEPARATED_FROM_PLAYER) );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_FearBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_FEAR_MOVE_TO_SAFE_PLACE:
		if( HasCondition(COND_FEAR_ENEMY_TOO_CLOSE) )
		{
			// If I'm moving to a safe place AND have an enemy too close to me,
			// make the move to safety while ignoring the condition.
			// this stops an oscillation
			// IS THIS CODE EVER EVEN BEING CALLED? (sjb)
			return SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY;
		}
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_Hint *CAI_FearBehavior::FindFearWithdrawalDest()
{
	CAI_Hint *pHint;
	CHintCriteria hintCriteria;
	CAI_BaseNPC *pOuter = GetOuter();

	Assert(pOuter != NULL);

	hintCriteria.AddHintType( HINT_PLAYER_ALLY_FEAR_DEST );
	hintCriteria.SetFlag( bits_HINT_NODE_VISIBLE_TO_PLAYER | bits_HINT_NOT_CLOSE_TO_ENEMY /*| bits_HINT_NODE_IN_VIEWCONE | bits_HINT_NPC_IN_NODE_FOV*/ );
	hintCriteria.AddIncludePosition( AI_GetSinglePlayer()->GetAbsOrigin(), ( ai_fear_player_dist.GetFloat() ) );

	pHint = CAI_HintManager::FindHint( pOuter, hintCriteria );

	if( pHint )
	{
		// Reserve this node while I try to get to it. When I get there I will lock it.
		// Otherwise, if I fail to get there, the node will come available again soon.
		pHint->DisableForSeconds( 4.0f );
	}
#if 0
	else
	{
		Msg("DID NOT FIND HINT\n");
		NDebugOverlay::Cross3D( GetOuter()->WorldSpaceCenter(), 32, 255, 255, 0, false, 10.0f );
	}
#endif

	return pHint;
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_FearBehavior )

	DECLARE_TASK( TASK_FEAR_GET_PATH_TO_SAFETY_HINT )
	DECLARE_TASK( TASK_FEAR_WAIT_FOR_SAFETY )
	DECLARE_TASK( TASK_FEAR_IN_SAFE_PLACE )

	DECLARE_CONDITION( COND_FEAR_ENEMY_CLOSE )
	DECLARE_CONDITION( COND_FEAR_ENEMY_TOO_CLOSE )
	DECLARE_CONDITION( COND_FEAR_SEPARATED_FROM_PLAYER )

	//===============================================
	//===============================================
	DEFINE_SCHEDULE
	(
		SCHED_FEAR_MOVE_TO_SAFE_PLACE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_RUN_FROM_ENEMY"
		"		TASK_FEAR_GET_PATH_TO_SAFETY_HINT	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FEAR_IN_SAFE_PLACE				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FEAR_STAY_IN_SAFE_PLACE"
		""
		"	Interrupts"
		""
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
		"		COND_FEAR_ENEMY_TOO_CLOSE"
	);

	DEFINE_SCHEDULE
		(
		SCHED_FEAR_MOVE_TO_SAFE_PLACE_RETRY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_RUN_FROM_ENEMY"
		"		TASK_FEAR_GET_PATH_TO_SAFETY_HINT	0"
		"		TASK_RUN_PATH						0"
		"		TASK_WAIT_FOR_MOVEMENT				0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FEAR_IN_SAFE_PLACE				0"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FEAR_STAY_IN_SAFE_PLACE"
		""
		"	Interrupts"
		""
		"		COND_HEAR_DANGER"
		"		COND_NEW_ENEMY"
		);

	//===============================================
	//===============================================
	DEFINE_SCHEDULE
		(
		SCHED_FEAR_STAY_IN_SAFE_PLACE,

		"	Tasks"
		"		TASK_FEAR_WAIT_FOR_SAFETY			0"
		""
		"	Interrupts"
		""
		"		COND_NEW_ENEMY"
		"		COND_HEAR_DANGER"
		"		COND_FEAR_ENEMY_CLOSE"
		"		COND_FEAR_ENEMY_TOO_CLOSE"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_FEAR_SEPARATED_FROM_PLAYER"
		);


AI_END_CUSTOM_SCHEDULE_PROVIDER()
