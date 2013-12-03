//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_behavior_holster.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAI_HolsterBehavior )
	DEFINE_FIELD( m_bWeaponOut, FIELD_BOOLEAN ),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_HolsterBehavior::CAI_HolsterBehavior()
{
	// m_AssaultCue = CUE_NO_ASSAULT;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_HolsterBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		BaseClass::StartTask( pTask );
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
void CAI_HolsterBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		BaseClass::RunTask( pTask );
		break;
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_HolsterBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
		return false;

	if ( GetEnemy() )
	{
		// make sure weapon is out
		if (!m_bWeaponOut)
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_HolsterBehavior::SelectSchedule()
{
	return BaseClass::SelectSchedule();
}






AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_HolsterBehavior )

	DECLARE_TASK( TASK_HOLSTER_WEAPON )
	DECLARE_TASK( TASK_DRAW_WEAPON )

	// DECLARE_CONDITION( COND_ )

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_HOLSTER_WEAPON,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_HOLSTER_WEAPON				0"
		"	"
		"	Interrupts"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_DRAW_WEAPON,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_DRAW_WEAPON				0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_SCHEDULE_PROVIDER()
