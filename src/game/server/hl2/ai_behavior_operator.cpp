//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "ai_navigator.h"
#include "ai_behavior_operator.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//=============================================================================
// >OPERATOR BEHAVIOR
//=============================================================================
//=============================================================================
BEGIN_DATADESC( CAI_OperatorBehavior )
	DEFINE_FIELD( m_hGoalEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPositionEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hContextTarget, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_WatchSeeEntity ),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_OperatorBehavior::CAI_OperatorBehavior()
{
	m_hPositionEnt.Set(NULL);
	m_hGoalEntity.Set(NULL);
	m_hContextTarget.Set(NULL);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define POSITION_ENT_ALWAYS_SEE_DIST	Square(120)
bool CAI_OperatorBehavior::CanSeePositionEntity()
{
	CAI_BaseNPC *pOuter = GetOuter();

	Assert( m_hPositionEnt.Get() != NULL );

	// early out here.
	if( !pOuter->QuerySeeEntity(m_hPositionEnt) )
	{
		m_WatchSeeEntity.Stop();
		return false;
	}

	bool bSpotted = (pOuter->EyePosition().DistToSqr(m_hPositionEnt->GetAbsOrigin()) <= POSITION_ENT_ALWAYS_SEE_DIST);
	if ( !bSpotted )
	{
		bSpotted = ( pOuter->FInViewCone(m_hPositionEnt) && pOuter->FVisible(m_hPositionEnt) );
	}

	if (bSpotted )
	{
		// If we haven't seen it up until now, start a timer. If we have seen it, wait for the
		// timer to finish. This prevents edge cases where turning on the flashlight makes
		// NPC spot the position entity a frame before she spots an enemy.
		if ( !m_WatchSeeEntity.IsRunning() )
		{
			m_WatchSeeEntity.Start( 0.3,0.31 );
			return false;
		}

		if ( !m_WatchSeeEntity.Expired() )
			return false;

		return true;
	}

	m_WatchSeeEntity.Stop();
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_OperatorBehavior::IsAtPositionEntity()
{
	Vector myPos = GetAbsOrigin();
	Vector objectPos = m_hPositionEnt->GetAbsOrigin();

	Vector vecDir = objectPos - myPos;

	return (vecDir.Length2D() <= 12.0f);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_OperatorBehavior::GatherConditionsNotActive()
{
	if( m_hPositionEnt )
	{
		// If we're not currently the active behavior, we have a position ent, and the 
		// NPC can see it, coax the AI out of IDLE/ALERT schedules with this condition.
		if( CanSeePositionEntity() )
		{
			SetCondition( COND_IDLE_INTERRUPT );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_OperatorBehavior::GatherConditions( void )
{
	if( GetGoalEntity() )
	{
		if( GetGoalEntity()->GetState() == OPERATOR_STATE_FINISHED )
		{
			if( IsCurSchedule(SCHED_OPERATOR_OPERATE) )
			{
				// Break us out of the operator schedule if the operation completes.
				SetCondition(COND_PROVOKED);
			}

			m_hGoalEntity.Set(NULL);
			m_hPositionEnt.Set(NULL);
		}
		else
		{
			if( CanSeePositionEntity() )
			{
				ClearCondition( COND_OPERATOR_LOST_SIGHT_OF_POSITION );
			}
			else
			{
				SetCondition( COND_OPERATOR_LOST_SIGHT_OF_POSITION );
			}
		}
	}

	BaseClass::GatherConditions();

	// Ignore player pushing.
	ClearCondition( COND_PLAYER_PUSHING );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_OperatorBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_OPERATOR_OPERATE:
		{
			// Fire the appropriate output!
			switch( GetGoalEntity()->GetState() )
			{
			case OPERATOR_STATE_NOT_READY:
				GetGoalEntity()->m_OnMakeReady.FireOutput(NULL, NULL, 0);
				break;

			case OPERATOR_STATE_READY:
				GetGoalEntity()->m_OnBeginOperating.FireOutput(NULL, NULL, 0);
				break;

			default:
				//!!!HACKHACK
				Assert(0);
				break;
			}
		}
		TaskComplete();
		break;

	case TASK_OPERATOR_START_PATH:
		{
			ChainStartTask(TASK_WALK_PATH);
		}
		break;

	case TASK_OPERATOR_GET_PATH_TO_POSITION:
		{
			CBaseEntity *pGoal = m_hPositionEnt;

			if( !pGoal )
			{
				TaskFail("ai_goal_operator has no location entity\n");
				break;
			}

			AI_NavGoal_t goal( pGoal->GetAbsOrigin() );
			goal.pTarget = pGoal;

			if ( GetNavigator()->SetGoal( goal ) == false )
			{
				TaskFail( "Can't build path\n" );
				/*
				// Try and get as close as possible otherwise
				AI_NavGoal_t nearGoal( GOALTYPE_LOCATION_NEAREST_NODE, m_hTargetObject->GetAbsOrigin(), AIN_DEF_ACTIVITY, 256 );
				if ( GetNavigator()->SetGoal( nearGoal, AIN_CLEAR_PREVIOUS_STATE ) )
				{
					//FIXME: HACK! The internal pathfinding is setting this without our consent, so override it!
					ClearCondition( COND_TASK_FAILED );
					GetNavigator()->SetArrivalDirection( m_hTargetObject->GetAbsAngles() );
					TaskComplete();
					return;
				}
				*/
			}

			GetNavigator()->SetArrivalDirection( pGoal->GetAbsAngles() );
		}
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
void CAI_OperatorBehavior::RunTask( const Task_t *pTask )
{
/*
	switch( pTask->iTask )
	{
	default:
		BaseClass::RunTask( pTask );
		break;
	}
*/
	BaseClass::RunTask( pTask );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CAI_OperatorGoal *CAI_OperatorBehavior::GetGoalEntity()
{
	CAI_OperatorGoal *pGoal = dynamic_cast<CAI_OperatorGoal*>(m_hGoalEntity.Get());

	// NULL is OK.
	return pGoal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_OperatorBehavior::IsGoalReady()
{
	if( GetGoalEntity()->GetState() == OPERATOR_STATE_READY )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_OperatorBehavior::SetParameters( CAI_OperatorGoal *pGoal, CBaseEntity *pPositionEnt, CBaseEntity *pContextTarget )
{
	m_hGoalEntity.Set( pGoal );
	m_hPositionEnt.Set( pPositionEnt );
	m_hContextTarget.Set( pContextTarget );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_OperatorBehavior::CanSelectSchedule()
{
	if ( m_hGoalEntity.Get() == NULL )
		return false;

	if ( m_hPositionEnt.Get() == NULL ) 
		return false;

	if( GetGoalEntity()->GetState() == OPERATOR_STATE_FINISHED )
	{
		m_hGoalEntity.Set(NULL);
		m_hPositionEnt.Set(NULL);
		return false;
	}

	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->m_NPCState == NPC_STATE_COMBAT || GetOuter()->m_NPCState == NPC_STATE_SCRIPT )
		return false;

	// Don't grab NPCs who have been in combat recently
	if ( GetOuter()->GetLastEnemyTime() && (gpGlobals->curtime - GetOuter()->GetLastEnemyTime()) < 3.0 )
		return false;

	if( !CanSeePositionEntity() )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_OperatorBehavior::SelectSchedule()
{
	if( !IsAtPositionEntity() )
	{
		GetGoalEntity()->m_OnBeginApproach.FireOutput( GetOuter(), GetOuter(), 0 );
		return SCHED_OPERATOR_APPROACH_POSITION;
	}

	if( GetGoalEntity() && GetGoalEntity()->GetState() != OPERATOR_STATE_FINISHED)
	{
		if( GetOuter()->GetActiveWeapon() && !GetOuter()->IsWeaponHolstered() )
		{
			GetOuter()->SetDesiredWeaponState( DESIREDWEAPONSTATE_HOLSTERED );
			return SCHED_OPERATOR_WAIT_FOR_HOLSTER;
		}

		return SCHED_OPERATOR_OPERATE;
	}

	return BaseClass::SelectSchedule();
}


//=============================================================================
//=============================================================================
// >AI_GOAL_OPERATOR
//=============================================================================
//=============================================================================
LINK_ENTITY_TO_CLASS( ai_goal_operator, CAI_OperatorGoal );

BEGIN_DATADESC( CAI_OperatorGoal )
	DEFINE_KEYFIELD( m_iState, FIELD_INTEGER, "state" ),
	DEFINE_KEYFIELD( m_iMoveTo, FIELD_INTEGER, "moveto" ),
	DEFINE_KEYFIELD( m_iszContextTarget, FIELD_STRING, "contexttarget" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetStateReady", InputSetStateReady ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetStateFinished", InputSetStateFinished ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

	// Outputs
	DEFINE_OUTPUT( m_OnBeginApproach, "OnBeginApproach" ),
	DEFINE_OUTPUT( m_OnMakeReady, "OnMakeReady" ),
	DEFINE_OUTPUT( m_OnBeginOperating, "OnBeginOperating" ),
	DEFINE_OUTPUT( m_OnFinished, "OnFinished" ),
END_DATADESC()

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_OperatorGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_OperatorBehavior *pBehavior;

	if ( !pAI->GetBehavior( &pBehavior ) )
	{
		return;
	}

	CBaseEntity *pPosition = gEntList.FindEntityByName(NULL, m_target);

	if( !pPosition )
	{
		DevMsg("ai_goal_operator called %s with invalid position ent!\n", GetDebugName() );
		return;
	}

	
	CBaseEntity *pContextTarget = NULL;
	
	if( m_iszContextTarget != NULL_STRING )
	{
		pContextTarget = gEntList.FindEntityByName( NULL, m_iszContextTarget );
	}

	pBehavior->SetParameters(this, pPosition, pContextTarget);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_OperatorGoal::InputActivate( inputdata_t &inputdata )
{
	BaseClass::InputActivate( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAI_OperatorGoal::InputDeactivate( inputdata_t &inputdata )
{
	BaseClass::InputDeactivate( inputdata );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_OperatorGoal::InputSetStateReady( inputdata_t &inputdata )
{
	m_iState = OPERATOR_STATE_READY;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_OperatorGoal::InputSetStateFinished( inputdata_t &inputdata )
{
	m_iState = OPERATOR_STATE_FINISHED;
	m_OnFinished.FireOutput( NULL, NULL, 0 );
}

//=============================================================================
//=============================================================================
// >SCHEDULES
//=============================================================================
//=============================================================================
AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_OperatorBehavior )

DECLARE_TASK( TASK_OPERATOR_GET_PATH_TO_POSITION )
DECLARE_TASK( TASK_OPERATOR_START_PATH )
DECLARE_TASK( TASK_OPERATOR_OPERATE )

DECLARE_CONDITION( COND_OPERATOR_LOST_SIGHT_OF_POSITION )

//=========================================================
//=========================================================
DEFINE_SCHEDULE 
(
 SCHED_OPERATOR_APPROACH_POSITION,
 "	Tasks"
 "		TASK_OPERATOR_GET_PATH_TO_POSITION	0"
 "		TASK_OPERATOR_START_PATH			0"
 "		TASK_WAIT_FOR_MOVEMENT				0"
 "		TASK_STOP_MOVING					0"

 "	"
 "	Interrupts"
 "		COND_NEW_ENEMY"
 "		COND_HEAR_DANGER"
 "		COND_OPERATOR_LOST_SIGHT_OF_POSITION"
 )

 //=========================================================
 //=========================================================
 DEFINE_SCHEDULE 
 (
 SCHED_OPERATOR_OPERATE,
 "	Tasks"
 "		TASK_WAIT					0.2" // Allow pending entity I/O to settle
 "		TASK_OPERATOR_OPERATE		0"
 "		TASK_WAIT_INDEFINITE		0"
 "	"
 "	Interrupts"
 "		COND_PROVOKED"
 )

//=========================================================
//=========================================================
DEFINE_SCHEDULE 
(
 SCHED_OPERATOR_WAIT_FOR_HOLSTER,
 "	Tasks"
 "		TASK_WAIT					1.0" 
 "	"
 "	Interrupts"
 "	"
 )

 AI_END_CUSTOM_SCHEDULE_PROVIDER()
