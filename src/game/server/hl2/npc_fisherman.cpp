//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_squad.h"
#include "ai_senses.h"
#include "ai_navigator.h"
#include "ai_motor.h"
#include "ai_behavior.h"
#include "ai_baseactor.h"
#include "ai_behavior_lead.h"
#include "ai_behavior_follow.h"
#include "ai_behavior_standoff.h"
#include "ai_behavior_assault.h"
#include "npc_playercompanion.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "sceneentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FISHERMAN_MODEL "models/lostcoast/fisherman/fisherman.mdl"

//=========================================================
// Fisherman activities
//=========================================================

Activity ACT_FISHERMAN_HAT_UP;
Activity ACT_FISHERMAN_HAT_DOWN;

//=========================================================
// animation events
//=========================================================
int AE_FISHERMAN_HAT_UP;
int AE_FISHERMAN_HAT_DOWN;
int AE_FISHERMAN_HAT_ON;
int AE_FISHERMAN_HAT_OFF;

//---------------------------------------------------------
//
//---------------------------------------------------------

class CNPC_Fisherman : public CNPC_PlayerCompanion
{
public:
	DECLARE_CLASS( CNPC_Fisherman, CNPC_PlayerCompanion );
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache()
	{
		// Prevents a warning
		SelectModel( );
		BaseClass::Precache();

		PrecacheScriptSound( "NPC_Fisherman.FootstepLeft" );
		PrecacheScriptSound( "NPC_Fisherman.FootstepRight" );
		PrecacheScriptSound( "NPC_Fisherman.Die" );

		PrecacheInstancedScene( "scenes/Expressions/FishermanIdle.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/FishermanAlert.vcd" );
		PrecacheInstancedScene( "scenes/Expressions/FishermanCombat.vcd" );
	}

	virtual void Activate()
	{
		BaseClass::Activate();

		if (m_iHatState == -1)
		{
			m_iHatState = ACT_FISHERMAN_HAT_DOWN;
		}

		// allocate layer, start with the hat down
		m_iHatLayer = AddGesture( (Activity)m_iHatState, false );
	}

	void	Spawn( void );
	void	SelectModel();
	Class_T Classify( void );

	void HandleAnimEvent( animevent_t *pEvent );

	bool ShouldLookForBetterWeapon() { return false; }
	virtual bool	IgnorePlayerPushing( void ) { return true; }
	void	DeathSound( const CTakeDamageInfo &info );

	int m_iHatLayer;	// overlay layer for hat, don't save/restore.
	int m_iHatState;	// hat state, persistant.

	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS( npc_fisherman, CNPC_Fisherman );

//---------------------------------------------------------
// 
//---------------------------------------------------------
/*
IMPLEMENT_SERVERCLASS_ST(CNPC_Fisherman, DT_NPC_Fisherman)
END_SEND_TABLE()
*/

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Fisherman )

	// DEFINE_FIELD( m_iHatLayer, FIELD_INT ),
	DEFINE_FIELD( m_iHatState, FIELD_INTEGER ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Fisherman::SelectModel()
{
	SetModelName( AllocPooledString( FISHERMAN_MODEL ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Fisherman::Spawn( void )
{
	m_iHatLayer = -1;
	m_iHatState = -1;

	Precache();

	m_iHealth = 80;

//	m_iszIdleExpression = MAKE_STRING("scenes/Expressions/FishermanIdle.vcd");
//	m_iszAlertExpression = MAKE_STRING("scenes/Expressions/FishermanAlert.vcd");
//	m_iszCombatExpression = MAKE_STRING("scenes/Expressions/FishermanCombat.vcd");

	BaseClass::Spawn();

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Fisherman::Classify( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Fisherman::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == NPC_EVENT_LEFTFOOT )
	{
		EmitSound( "NPC_Fisherman.FootstepLeft", pEvent->eventtime );
	}
	else if ( pEvent->event == NPC_EVENT_RIGHTFOOT )
	{
		EmitSound( "NPC_Fisherman.FootstepRight", pEvent->eventtime );
	}
	else if ( pEvent->event == AE_FISHERMAN_HAT_UP )
	{
		if (m_iHatLayer != -1)
		{
			RemoveLayer( m_iHatLayer, 0.2, 0.2 );
			m_iHatLayer = -1;
		}

		m_iHatState = ACT_FISHERMAN_HAT_UP;
		m_iHatLayer = AddGesture( (Activity)m_iHatState, false );
	}
	else if ( pEvent->event == AE_FISHERMAN_HAT_DOWN )
	{
		if (m_iHatLayer != -1)
		{
			RemoveLayer( m_iHatLayer, 0.2, 0.2 );
			m_iHatLayer = -1;
		}

		m_iHatState = ACT_FISHERMAN_HAT_DOWN;
		m_iHatLayer = AddGesture( (Activity)m_iHatState, false );
	}
	else if ( pEvent->event == AE_FISHERMAN_HAT_ON )
	{
		m_iHatLayer = AddGesture( (Activity)m_iHatState, false );
	}
	else if ( pEvent->event == AE_FISHERMAN_HAT_OFF )
	{
		if (m_iHatLayer != -1)
		{
			RemoveLayer( m_iHatLayer, 0.2, 0.2 );
			m_iHatLayer = -1;
		}
	}
	else
	{
		BaseClass::HandleAnimEvent( pEvent );
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Fisherman::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_fisherman, CNPC_Fisherman )

	DECLARE_ACTIVITY( ACT_FISHERMAN_HAT_UP )
	DECLARE_ACTIVITY( ACT_FISHERMAN_HAT_DOWN )

	DECLARE_ANIMEVENT( AE_FISHERMAN_HAT_UP )
	DECLARE_ANIMEVENT( AE_FISHERMAN_HAT_DOWN )
	DECLARE_ANIMEVENT( AE_FISHERMAN_HAT_ON )
	DECLARE_ANIMEVENT( AE_FISHERMAN_HAT_OFF )

AI_END_CUSTOM_NPC()
