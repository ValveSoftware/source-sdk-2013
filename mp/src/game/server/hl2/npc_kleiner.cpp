//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dr. Kleiner, a suave ladies man leading the fight against the evil 
//			combine while constantly having to help his idiot assistant Gordon
//
//=============================================================================//


//-----------------------------------------------------------------------------
// Generic NPC - purely for scripted sequence work.
//-----------------------------------------------------------------------------
#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include "ai_baseactor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// NPC's Anim Events Go Here
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Kleiner : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Kleiner, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );
};

LINK_ENTITY_TO_CLASS( npc_kleiner, CNPC_Kleiner );

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Kleiner::Classify ( void )
{
	return	CLASS_PLAYER_ALLY_VITAL;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Kleiner::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//-----------------------------------------------------------------------------
// GetSoundInterests - generic NPC can't hear.
//-----------------------------------------------------------------------------
int CNPC_Kleiner::GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Kleiner::Spawn()
{
	// Allow custom model usage (mostly for monitors)
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/kleiner.mdl";
		SetModelName( AllocPooledString(szModel) );
	}

	Precache();
	SetModel( szModel );

	BaseClass::Spawn();

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS | bits_CAP_ANIMATEDFACE | bits_CAP_TURN_HEAD );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );

	AddEFlags( EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION );

	NPCInit();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Kleiner::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
	
	BaseClass::Precache();
}	

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------
