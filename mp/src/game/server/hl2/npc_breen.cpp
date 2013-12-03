//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dr. Breen, the oft maligned genius, heroically saving humanity from 
//			its own worst enemy, itself.
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

// Spawnflags
#define SF_BREEN_BACKGROUND_TALK		( 1 << 16 )		// 65536 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_Breen : public CAI_BaseActor
{
public:
	DECLARE_CLASS( CNPC_Breen, CAI_BaseActor );

	void	Spawn( void );
	void	Precache( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );
	bool	UseSemaphore( void );
};

LINK_ENTITY_TO_CLASS( npc_breen, CNPC_Breen );

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Breen::Classify ( void )
{
	return	CLASS_NONE;
}



//-----------------------------------------------------------------------------
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//-----------------------------------------------------------------------------
void CNPC_Breen::HandleAnimEvent( animevent_t *pEvent )
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
int CNPC_Breen::GetSoundInterests ( void )
{
	return	NULL;
}

//-----------------------------------------------------------------------------
// Spawn
//-----------------------------------------------------------------------------
void CNPC_Breen::Spawn()
{
	// Breen is allowed to use multiple models, because he has a torso version for monitors.
	// He defaults to his normal model.
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		szModel = "models/breen.mdl";
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
void CNPC_Breen::Precache()
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}	

bool CNPC_Breen::UseSemaphore( void )	
{ 
	if ( HasSpawnFlags( SF_BREEN_BACKGROUND_TALK ) )
		return false;

	return BaseClass::UseSemaphore();
}

//-----------------------------------------------------------------------------
// AI Schedules Specific to this NPC
//-----------------------------------------------------------------------------

