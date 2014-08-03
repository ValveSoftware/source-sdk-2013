//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base class for many flying NPCs
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc_flyer_new.h"
#include "ai_route.h"
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FLYER_ROUTE_REBUILD_TIME	3.0		// Time between route rebuilds


// NOTE: Never instantiate ai_base_npc_flyer_new directly!!
//IMPLEMENT_CUSTOM_AI( ai_base_npc_flyer_new, CAI_BaseNPCFlyerNew);


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_BaseNPCFlyerNew::CAI_BaseNPCFlyerNew()
{
}


//------------------------------------------------------------------------------
// Used to set up a flyer
//------------------------------------------------------------------------------
void CAI_BaseNPCFlyerNew::SpawnFlyer()
{
	SetNavType( NAV_FLY );
	AddFlag(  FL_FLY );
	SetMoveType( MOVETYPE_STEP );			
	CapabilitiesAdd( bits_CAP_MOVE_FLY );
}


/*
void CAI_BaseNPCFlyerNew::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CAI_BaseNPCFlyerNew);

	ADD_CUSTOM_CONDITION(CAI_BaseNPCFlyerNew,	COND_FLYER_MOVE_BLOCKED);
	ADD_CUSTOM_CONDITION(CAI_BaseNPCFlyerNew,	COND_FLYER_MOVE_IMPOSSIBLE);
}
*/

//------------------------------------------------------------------------------
// Should be called during Select Schedule (BLEAH!) 
//------------------------------------------------------------------------------
void CAI_BaseNPCFlyerNew::ClearFlyerConditions(void)
{
//	ClearCondition( COND_FLYER_MOVE_BLOCKED );
//	ClearCondition( COND_FLYER_MOVE_IMPOSSIBLE );
}



//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
float CAI_BaseNPCFlyerNew::MinGroundDist(void)
{
	return 0;
}


//-----------------------------------------------------------------------------
// Sets the ground speed appropriately: 
//-----------------------------------------------------------------------------
float CAI_BaseNPCFlyerNew::GetIdealSpeed( )	const
{
	return m_flSpeed;
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_BaseNPCFlyerNew::StartTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{	
		// Activity is just idle (have no run)
		case TASK_RUN_PATH:
		{
			GetNavigator()->SetMovementActivity(ACT_IDLE);
			TaskComplete();
			break;
		}

		// Don't check for run/walk activity
		case TASK_SCRIPT_RUN_TO_TARGET:
		case TASK_SCRIPT_WALK_TO_TARGET:
		{
			if (GetTarget() == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else 
			{
				if (!GetNavigator()->SetGoal( GOALTYPE_TARGETENT ) )
				{
					TaskFail(FAIL_NO_ROUTE);
					GetNavigator()->ClearGoal();
				}
			}
			TaskComplete();
			break;
		}

		default:
		{
			BaseClass::StartTask(pTask);
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BaseNPCFlyerNew::RunTask( const Task_t *pTask )
{
	BaseClass::RunTask(pTask);
}


