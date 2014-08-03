//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================

#include "cbase.h"
#include "vehicle_jeep_episodic.h"
#include "collisionutils.h"
#include "npc_alyx_episodic.h"
#include "particle_parse.h"
#include "particle_system.h"
#include "hl2_player.h"
#include "in_buttons.h"
#include "vphysics/friction.h"
#include "vphysicsupdateai.h"
#include "physics_npc_solver.h"
#include "Sprite.h"
#include "weapon_striderbuster.h"
#include "npc_strider.h"
#include "vguiscreen.h"
#include "hl2_vehicle_radar.h"
#include "props.h"
#include "ai_dynamiclink.h"

extern ConVar phys_upimpactforcescale;

ConVar jalopy_blocked_exit_max_speed( "jalopy_blocked_exit_max_speed", "50" );

#define JEEP_AMMOCRATE_HITGROUP		5
#define	JEEP_AMMO_CRATE_CLOSE_DELAY	2.0f

// Bodygroups
#define JEEP_RADAR_BODYGROUP	1
#define JEEP_HOPPER_BODYGROUP	2
#define JEEP_CARBAR_BODYGROUP   3

#define RADAR_PANEL_MATERIAL	"vgui/screens/radar"
#define RADAR_PANEL_WRITEZ		"engine/writez"

static const char *s_szHazardSprite = "sprites/light_glow01.vmt";

enum
{
	RADAR_MODE_NORMAL	= 0,
	RADAR_MODE_STICKY,
};

//=========================================================
//=========================================================
class CRadarTarget : public CPointEntity
{
	DECLARE_CLASS( CRadarTarget, CPointEntity );

public:
	void	Spawn();

	bool	IsDisabled()	{ return m_bDisabled; }
	int		GetType()		{ return m_iType; }
	int		GetMode()		{ return m_iMode; }
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	int		ObjectCaps();

private:
	bool	m_bDisabled;
	int		m_iType;
	int		m_iMode;

public:
	float	m_flRadius;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( info_radar_target, CRadarTarget );

BEGIN_DATADESC( CRadarTarget )
	DEFINE_KEYFIELD( m_bDisabled,	FIELD_BOOLEAN,	"StartDisabled" ),
	DEFINE_KEYFIELD( m_flRadius,	FIELD_FLOAT,	"radius" ),
	DEFINE_KEYFIELD( m_iType,		FIELD_INTEGER,	"type" ),
	DEFINE_KEYFIELD( m_iMode,		FIELD_INTEGER,	"mode" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",	InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",InputDisable ),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRadarTarget::Spawn()
{
	BaseClass::Spawn();
	
	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRadarTarget::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRadarTarget::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CRadarTarget::ObjectCaps()
{
	return BaseClass::ObjectCaps() | FCAP_ACROSS_TRANSITION;
}




//
// Trigger which detects entities placed in the cargo hold of the jalopy
//

class CVehicleCargoTrigger : public CBaseEntity
{
	DECLARE_CLASS( CVehicleCargoTrigger, CBaseEntity );

public:

	// 
	// Creates a trigger with the specified bounds

	static CVehicleCargoTrigger *Create( const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, CBaseEntity *pOwner )
	{
		CVehicleCargoTrigger *pTrigger = (CVehicleCargoTrigger *) CreateEntityByName( "trigger_vehicle_cargo" );
		if ( pTrigger == NULL )
			return NULL;

		UTIL_SetOrigin( pTrigger, vecOrigin );
		UTIL_SetSize( pTrigger, vecMins, vecMaxs );		
		pTrigger->SetOwnerEntity( pOwner );
		pTrigger->SetParent( pOwner );

		pTrigger->Spawn();

		return pTrigger;
	}

	//
	// Handles the trigger touching its intended quarry

	void CargoTouch( CBaseEntity *pOther )
	{
		// Cannot be ignoring touches
		if ( ( m_hIgnoreEntity == pOther ) || ( m_flIgnoreDuration >= gpGlobals->curtime ) )
			return;

		// Make sure this object is being held by the player
		if ( pOther->VPhysicsGetObject() == NULL || (pOther->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD) == false )
			return;

		if ( StriderBuster_NumFlechettesAttached( pOther ) > 0 )
			return;

		AddCargo( pOther );
	}

	bool AddCargo( CBaseEntity *pOther )
	{
		// For now, only bother with strider busters
		if ( (FClassnameIs( pOther, "weapon_striderbuster" ) == false) &&
			(FClassnameIs( pOther, "npc_grenade_magna" ) == false)
			)
			return false;

		// Must be a physics prop
		CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pOther);
		if ( pOther == NULL )
			return false;

		CPropJeepEpisodic *pJeep = dynamic_cast< CPropJeepEpisodic * >( GetOwnerEntity() );
		if ( pJeep == NULL )
			return false;

		// Make the player release the item
		Pickup_ForcePlayerToDropThisObject( pOther );

		// Stop colliding with things
		pOther->VPhysicsDestroyObject();
		pOther->SetSolidFlags( FSOLID_NOT_SOLID );
		pOther->SetMoveType( MOVETYPE_NONE );

		// Parent the object to our owner
		pOther->SetParent( GetOwnerEntity() );

		// The car now owns the entity
		pJeep->AddPropToCargoHold( pProp );

		// Notify the buster that it's been added to the cargo hold.		
		StriderBuster_OnAddToCargoHold( pProp );

		// Stop touching this item
		Disable();

		return true;
	}

	//
	// Setup the entity

	void Spawn( void )
	{
		BaseClass::Spawn();

		SetSolid( SOLID_BBOX );
		SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );

		SetTouch( &CVehicleCargoTrigger::CargoTouch );
	}

	void Activate()
	{
		BaseClass::Activate();
		SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID ); // Fixes up old savegames
	}

	//
	// When we've stopped touching this entity, we ignore it

	void EndTouch( CBaseEntity *pOther )
	{
		if ( pOther == m_hIgnoreEntity )
		{
			m_hIgnoreEntity = NULL;
		}

		BaseClass::EndTouch( pOther );
	}

	//
	// Disables the trigger for a set duration

	void IgnoreTouches( CBaseEntity *pIgnoreEntity )
	{
		m_hIgnoreEntity = pIgnoreEntity;
		m_flIgnoreDuration = gpGlobals->curtime + 0.5f;
	}

	void Disable( void )
	{
		SetTouch( NULL );
	}

	void Enable( void )
	{
		SetTouch( &CVehicleCargoTrigger::CargoTouch );
	}

protected:

	float					m_flIgnoreDuration;
	CHandle	<CBaseEntity>	m_hIgnoreEntity;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( trigger_vehicle_cargo, CVehicleCargoTrigger );

BEGIN_DATADESC( CVehicleCargoTrigger )
	DEFINE_FIELD( m_flIgnoreDuration, FIELD_TIME ),
	DEFINE_FIELD( m_hIgnoreEntity, FIELD_EHANDLE ),
	DEFINE_ENTITYFUNC( CargoTouch ),
END_DATADESC();

//
// Transition reference point for the vehicle
//

class CInfoTargetVehicleTransition : public CPointEntity
{
public:
	DECLARE_CLASS( CInfoTargetVehicleTransition, CPointEntity );

	void Enable( void ) { m_bDisabled = false; }
	void Disable( void ) { m_bDisabled = true; }

	bool IsDisabled( void ) const { return m_bDisabled; }

private:

	void InputEnable( inputdata_t &data ) { Enable(); }
	void InputDisable( inputdata_t &data ) { Disable(); }

	bool	m_bDisabled;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CInfoTargetVehicleTransition )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable",	InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable",InputDisable ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( info_target_vehicle_transition, CInfoTargetVehicleTransition );

//
//	CPropJeepEpisodic
//

LINK_ENTITY_TO_CLASS( prop_vehicle_jeep, CPropJeepEpisodic );

BEGIN_DATADESC( CPropJeepEpisodic )

	DEFINE_FIELD( m_bEntranceLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExitLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hCargoProp, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hCargoTrigger, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bAddingCargo, FIELD_BOOLEAN ),
	DEFINE_ARRAY( m_hWheelDust, FIELD_EHANDLE, NUM_WHEEL_EFFECTS ),
	DEFINE_ARRAY( m_hWheelWater, FIELD_EHANDLE, NUM_WHEEL_EFFECTS ),
	DEFINE_ARRAY( m_hHazardLights, FIELD_EHANDLE, NUM_HAZARD_LIGHTS ),
	DEFINE_FIELD( m_flCargoStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_bBlink, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bRadarEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bRadarDetectsEnemies, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hRadarScreen, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLinkControllerFront, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLinkControllerRear, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_bBusterHopperVisible, FIELD_BOOLEAN, "CargoVisible" ),
	// m_flNextAvoidBroadcastTime
	DEFINE_FIELD( m_flNextWaterSound, FIELD_TIME ),
	DEFINE_FIELD( m_flNextRadarUpdateTime, FIELD_TIME ),
	DEFINE_FIELD( m_iNumRadarContacts, FIELD_INTEGER ),
	DEFINE_ARRAY( m_vecRadarContactPos, FIELD_POSITION_VECTOR, RADAR_MAX_CONTACTS ),
	DEFINE_ARRAY( m_iRadarContactType, FIELD_INTEGER, RADAR_MAX_CONTACTS ),

	DEFINE_THINKFUNC( HazardBlinkThink ),

	DEFINE_OUTPUT( m_OnCompanionEnteredVehicle, "OnCompanionEnteredVehicle" ),
	DEFINE_OUTPUT( m_OnCompanionExitedVehicle, "OnCompanionExitedVehicle" ),
	DEFINE_OUTPUT( m_OnHostileEnteredVehicle, "OnHostileEnteredVehicle" ),
	DEFINE_OUTPUT( m_OnHostileExitedVehicle, "OnHostileExitedVehicle" ),
	
	DEFINE_INPUTFUNC( FIELD_VOID, "LockEntrance",				InputLockEntrance ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnlockEntrance",				InputUnlockEntrance ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LockExit",					InputLockExit ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnlockExit",					InputUnlockExit ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableRadar",				InputEnableRadar ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableRadar",				InputDisableRadar ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableRadarDetectEnemies",	InputEnableRadarDetectEnemies ),
	DEFINE_INPUTFUNC( FIELD_VOID, "AddBusterToCargo",			InputAddBusterToCargo ),
	DEFINE_INPUTFUNC( FIELD_VOID, "OutsideTransition",			InputOutsideTransition ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePhysGun",				InputDisablePhysGun ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePhysGun",				InputEnablePhysGun ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CreateLinkController",		InputCreateLinkController ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DestroyLinkController",		InputDestroyLinkController ),

	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetCargoHopperVisibility",   InputSetCargoVisibility ),

END_DATADESC();

IMPLEMENT_SERVERCLASS_ST(CPropJeepEpisodic, DT_CPropJeepEpisodic)
	//CNetworkVar( int, m_iNumRadarContacts );
	SendPropInt( SENDINFO(m_iNumRadarContacts), 8 ),

	//CNetworkArray( Vector, m_vecRadarContactPos, RADAR_MAX_CONTACTS );
	SendPropArray( SendPropVector( SENDINFO_ARRAY(m_vecRadarContactPos), -1, SPROP_COORD), m_vecRadarContactPos ),

	//CNetworkArray( int, m_iRadarContactType, RADAR_MAX_CONTACTS );
	SendPropArray( SendPropInt(SENDINFO_ARRAY(m_iRadarContactType), RADAR_CONTACT_TYPE_BITS ), m_iRadarContactType ),
END_SEND_TABLE()


//=============================================================================
// Episodic jeep

CPropJeepEpisodic::CPropJeepEpisodic( void ) : 
m_bEntranceLocked( false ),
m_bExitLocked( false ),
m_bAddingCargo( false ),
m_flNextAvoidBroadcastTime( 0.0f )
{
	m_bHasGun = false;
	m_bUnableToFire = true;
	m_bRadarDetectsEnemies = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	// Kill our wheel dust
	for ( int i = 0; i < NUM_WHEEL_EFFECTS; i++ )
	{
		if ( m_hWheelDust[i] != NULL )
		{
			UTIL_Remove( m_hWheelDust[i] );
		}

		if ( m_hWheelWater[i] != NULL )
		{
			UTIL_Remove( m_hWheelWater[i] );
		}
	}

	DestroyHazardLights();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Precache( void )
{
	PrecacheMaterial( RADAR_PANEL_MATERIAL );
	PrecacheMaterial( RADAR_PANEL_WRITEZ );
	PrecacheModel( s_szHazardSprite );
	PrecacheScriptSound( "JNK_Radar_Ping_Friendly" );
	PrecacheScriptSound( "Physics.WaterSplash" );

	PrecacheParticleSystem( "WheelDust" );
	PrecacheParticleSystem( "WheelSplash" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	BaseClass::EnterVehicle( pPassenger );

	// Turn our hazards off!
	DestroyHazardLights();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Spawn( void )
{
	BaseClass::Spawn();

	SetBlocksLOS( false );

	CBasePlayer	*pPlayer = UTIL_GetLocalPlayer();
	if ( pPlayer != NULL )
	{
		pPlayer->m_Local.m_iHideHUD |= HIDEHUD_VEHICLE_CROSSHAIR;
	}


	SetBodygroup( JEEP_HOPPER_BODYGROUP, m_bBusterHopperVisible ? 1 : 0);
	CreateCargoTrigger();

	// carbar bodygroup is always on
	SetBodygroup( JEEP_CARBAR_BODYGROUP, 1 );

	m_bRadarDetectsEnemies = false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Activate()
{
	m_iNumRadarContacts = 0; // Force first contact tone
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::NPC_FinishedEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// FIXME: This will be moved to the NPCs entering and exiting
	// Fire our outputs
	if ( bCompanion	)
	{
		m_OnCompanionEnteredVehicle.FireOutput( this, pPassenger );
	}
	else
	{
		m_OnHostileEnteredVehicle.FireOutput( this, pPassenger );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::NPC_FinishedExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// FIXME: This will be moved to the NPCs entering and exiting
	// Fire our outputs
	if ( bCompanion	)
	{
		m_OnCompanionExitedVehicle.FireOutput( this, pPassenger );
	}
	else
	{
		m_OnHostileExitedVehicle.FireOutput( this, pPassenger );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::NPC_CanEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Must be unlocked
	if ( bCompanion && m_bEntranceLocked )
		return false;

	return BaseClass::NPC_CanEnterVehicle( pPassenger, bCompanion );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::NPC_CanExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Must be unlocked
	if ( bCompanion && m_bExitLocked )
		return false;

	return BaseClass::NPC_CanExitVehicle( pPassenger, bCompanion );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputLockEntrance( inputdata_t &data )
{
	m_bEntranceLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputUnlockEntrance( inputdata_t &data )
{
	m_bEntranceLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputLockExit( inputdata_t &data )
{
	m_bExitLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputUnlockExit( inputdata_t &data )
{
	m_bExitLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the Jalopy radar device
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputEnableRadar( inputdata_t &data )
{
	if( m_bRadarEnabled )
		return; // Already enabled

	SetBodygroup( JEEP_RADAR_BODYGROUP, 1 );

	SpawnRadarPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Turn off the Jalopy radar device
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputDisableRadar( inputdata_t &data )
{
	if( !m_bRadarEnabled )
		return; // Already disabled

	SetBodygroup( JEEP_RADAR_BODYGROUP, 0 );

	DestroyRadarPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Allow the Jalopy radar to detect Hunters and Striders
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputEnableRadarDetectEnemies( inputdata_t &data )
{
	m_bRadarDetectsEnemies = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputAddBusterToCargo( inputdata_t &data )
{
	if ( m_hCargoProp != NULL)
	{
		ReleasePropFromCargoHold();
		m_hCargoProp = NULL;
	}

	CBaseEntity *pNewBomb = CreateEntityByName( "weapon_striderbuster" );
	if ( pNewBomb )
	{
		DispatchSpawn( pNewBomb );
		pNewBomb->Teleport( &m_hCargoTrigger->GetAbsOrigin(), NULL, NULL );
		m_hCargoTrigger->AddCargo( pNewBomb );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropJeepEpisodic::PassengerInTransition( void )
{
	// FIXME: Big hack - we need a way to bridge this data better
	// TODO: Get a list of passengers we can traverse instead
	CNPC_Alyx *pAlyx = CNPC_Alyx::GetAlyx();
	if ( pAlyx )
	{
		if ( pAlyx->GetPassengerState() == PASSENGER_STATE_ENTERING ||
			 pAlyx->GetPassengerState() == PASSENGER_STATE_EXITING )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Override velocity if our passenger is transitioning or we're upside-down
//-----------------------------------------------------------------------------
Vector CPropJeepEpisodic::PhysGunLaunchVelocity( const Vector &forward, float flMass )
{
	// Disallow
	if ( PassengerInTransition() )
		return vec3_origin;

	Vector vecPuntDir = BaseClass::PhysGunLaunchVelocity( forward, flMass );
	vecPuntDir.z = 150.0f;
	vecPuntDir *= 600.0f;
	return vecPuntDir;
}

//-----------------------------------------------------------------------------
// Purpose: Rolls the vehicle when its trying to upright itself from a punt
//-----------------------------------------------------------------------------
AngularImpulse CPropJeepEpisodic::PhysGunLaunchAngularImpulse( void ) 
{ 
	if ( IsOverturned() )
		return AngularImpulse( 0, 300, 0 );

	// Don't spin randomly, always spin reliably
	return AngularImpulse( 0, 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Get the upright strength based on what state we're in
//-----------------------------------------------------------------------------
float CPropJeepEpisodic::GetUprightStrength( void ) 
{ 
	// Lesser if overturned
	if ( IsOverturned() )
		return 2.0f;
	
	return 0.0f; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::CreateCargoTrigger( void )
{
	if ( m_hCargoTrigger != NULL )
		return;

	int nAttachment = LookupAttachment( "cargo" );
	if ( nAttachment )
	{
		Vector vecAttachOrigin;
		Vector vecForward, vecRight, vecUp;
		GetAttachment( nAttachment, vecAttachOrigin, &vecForward, &vecRight, &vecUp );

		// Approx size of the hold
		Vector vecMins( -8.0, -6.0, 0 );
		Vector vecMaxs( 8.0, 6.0, 4.0 );

		// NDebugOverlay::BoxDirection( vecAttachOrigin, vecMins, vecMaxs, vecForward, 255, 0, 0, 64, 4.0f );

		// Create a trigger that lives for a small amount of time
		m_hCargoTrigger = CVehicleCargoTrigger::Create( vecAttachOrigin, vecMins, vecMaxs, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If the player uses the jeep while at the back, he gets ammo from the crate instead
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Fall back and get in the vehicle instead, skip giving ammo
	BaseClass::BaseClass::Use( pActivator, pCaller, useType, value );
}

#define	MIN_WHEEL_DUST_SPEED	5

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::UpdateWheelDust( void )
{
	// See if this wheel should emit dust
	const vehicleparams_t *vehicleData = m_pServerVehicle->GetVehicleParams();
	const vehicle_operatingparams_t *carState = m_pServerVehicle->GetVehicleOperatingParams();
	bool bAllowDust = vehicleData->steering.dustCloud;
	
	// Car must be active
	bool bCarOn = m_VehiclePhysics.IsOn();

	// Must be moving quickly enough or skidding along the ground
	bool bCreateDust = ( bCarOn &&
						 bAllowDust && 
					   ( m_VehiclePhysics.GetSpeed() >= MIN_WHEEL_DUST_SPEED || carState->skidSpeed > DEFAULT_SKID_THRESHOLD ) );

	// Update our wheel dust
	Vector	vecPos;
	for ( int i = 0; i < NUM_WHEEL_EFFECTS; i++ )
	{
		m_pServerVehicle->GetWheelContactPoint( i, vecPos );
		
		// Make sure the effect is created
		if ( m_hWheelDust[i] == NULL )
		{
			// Create the dust effect in place
			m_hWheelDust[i] = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
			if ( m_hWheelDust[i] == NULL )
				continue;

			// Setup our basic parameters
			m_hWheelDust[i]->KeyValue( "start_active", "0" );
			m_hWheelDust[i]->KeyValue( "effect_name", "WheelDust" );
			m_hWheelDust[i]->SetParent( this );
			m_hWheelDust[i]->SetLocalOrigin( vec3_origin );
			DispatchSpawn( m_hWheelDust[i] );
			if ( gpGlobals->curtime > 0.5f )
				m_hWheelDust[i]->Activate();
		}

		// Make sure the effect is created
		if ( m_hWheelWater[i] == NULL )
		{
			// Create the dust effect in place
			m_hWheelWater[i] = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
			if ( m_hWheelWater[i] == NULL )
				continue;

			// Setup our basic parameters
			m_hWheelWater[i]->KeyValue( "start_active", "0" );
			m_hWheelWater[i]->KeyValue( "effect_name", "WheelSplash" );
			m_hWheelWater[i]->SetParent( this );
			m_hWheelWater[i]->SetLocalOrigin( vec3_origin );
			DispatchSpawn( m_hWheelWater[i] );
			if ( gpGlobals->curtime > 0.5f )
				m_hWheelWater[i]->Activate();
		}

		// Turn the dust on or off
		if ( bCreateDust )
		{
			// Angle the dust out away from the wheels
			Vector vecForward, vecRight, vecUp;
			GetVectors( &vecForward, &vecRight, &vecUp );
			
			const vehicle_controlparams_t *vehicleControls = m_pServerVehicle->GetVehicleControlParams();
			float flWheelDir = ( i & 1 ) ? 1.0f : -1.0f;
			QAngle vecAngles;
			vecForward += vecRight * flWheelDir;
			vecForward += vecRight * (vehicleControls->steering*0.5f) * flWheelDir;
			vecForward += vecUp;
			VectorAngles( vecForward, vecAngles );

			// NDebugOverlay::Axis( vecPos, vecAngles, 8.0f, true, 0.1f );

			if ( m_WaterData.m_bWheelInWater[i] )
			{
				m_hWheelDust[i]->StopParticleSystem();

				// Set us up in the right position
				m_hWheelWater[i]->StartParticleSystem();
				m_hWheelWater[i]->SetAbsAngles( vecAngles );
				m_hWheelWater[i]->SetAbsOrigin( vecPos + Vector( 0, 0, 8 ) );

				if ( m_flNextWaterSound < gpGlobals->curtime )
				{
					m_flNextWaterSound = gpGlobals->curtime + random->RandomFloat( 0.25f, 1.0f );
					EmitSound( "Physics.WaterSplash" );
				}
			}
			else
			{
				m_hWheelWater[i]->StopParticleSystem();

				// Set us up in the right position
				m_hWheelDust[i]->StartParticleSystem();
				m_hWheelDust[i]->SetAbsAngles( vecAngles );
				m_hWheelDust[i]->SetAbsOrigin( vecPos + Vector( 0, 0, 8 ) );
			}
		}
		else
		{
			// Stop emitting
			m_hWheelDust[i]->StopParticleSystem();
			m_hWheelWater[i]->StopParticleSystem();
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ConVar jalopy_radar_test_ent( "jalopy_radar_test_ent", "none" );

//-----------------------------------------------------------------------------
// Purpose: Search for things that the radar detects, and stick them in the
// UTILVector that gets sent to the client for radar display.
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::UpdateRadar( bool forceUpdate )
{
	bool bDetectedDog = false;

	if( !m_bRadarEnabled )
		return;

	if( !forceUpdate && gpGlobals->curtime < m_flNextRadarUpdateTime )
		return;

	// Count the targets on radar. If any more targets come on the radar, we beep.
	int m_iNumOldRadarContacts = m_iNumRadarContacts;

	m_flNextRadarUpdateTime = gpGlobals->curtime + RADAR_UPDATE_FREQUENCY;
	m_iNumRadarContacts = 0;

	CBaseEntity *pEnt = gEntList.FirstEnt();
	string_t iszRadarTarget = FindPooledString( "info_radar_target" );
	string_t iszStriderName = FindPooledString( "npc_strider" );
	string_t iszHunterName = FindPooledString( "npc_hunter" );

	string_t iszTestName = FindPooledString( jalopy_radar_test_ent.GetString() );

	Vector vecJalopyOrigin = WorldSpaceCenter();

	while( pEnt != NULL )
	{
		int type = RADAR_CONTACT_NONE;

		if( pEnt->m_iClassname == iszRadarTarget )
		{
			CRadarTarget *pTarget = dynamic_cast<CRadarTarget*>(pEnt);

			if( pTarget != NULL && !pTarget->IsDisabled() )
			{
				if( pTarget->m_flRadius < 0 || vecJalopyOrigin.DistToSqr(pTarget->GetAbsOrigin()) <= Square(pTarget->m_flRadius) )
				{
					// This item has been detected.
					type = pTarget->GetType();

					if( type == RADAR_CONTACT_DOG )
						bDetectedDog = true;// used to prevent Alyx talking about the radar (see below)

					if( pTarget->GetMode() == RADAR_MODE_STICKY )
					{
						// This beacon was just detected. Now change the radius to infinite
						// so that it will never go off the radar due to distance.
						pTarget->m_flRadius = -1;
					}
				}
			}
		}
		else if ( m_bRadarDetectsEnemies )
		{
			if ( pEnt->m_iClassname == iszStriderName )
			{
				CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider*>(pEnt);

				if( !pStrider || !pStrider->CarriedByDropship() )
				{
					// Ignore striders which are carried by dropships.
					type = RADAR_CONTACT_LARGE_ENEMY;
				}
			}

			if ( pEnt->m_iClassname == iszHunterName )
			{
				type = RADAR_CONTACT_ENEMY;
			}
		}

		if( type != RADAR_CONTACT_NONE )
		{
			Vector vecPos = pEnt->WorldSpaceCenter();

			m_vecRadarContactPos.Set( m_iNumRadarContacts, vecPos );
			m_iRadarContactType.Set( m_iNumRadarContacts, type );
			m_iNumRadarContacts++;

			if( m_iNumRadarContacts == RADAR_MAX_CONTACTS )
				break;
		}

		pEnt = gEntList.NextEnt(pEnt);
	}

	if( m_iNumRadarContacts > m_iNumOldRadarContacts )
	{
		// Play a bleepy sound
		if( !bDetectedDog )
		{
			EmitSound( "JNK_Radar_Ping_Friendly" );
		}

		//Notify Alyx so she can talk about the radar contact
		CNPC_Alyx *pAlyx = CNPC_Alyx::GetAlyx();

		if( !bDetectedDog && pAlyx != NULL && pAlyx->GetVehicle() )
		{
			pAlyx->SpeakIfAllowed( TLK_PASSENGER_NEW_RADAR_CONTACT );
		}
	}

	if( bDetectedDog )
	{
		// Update the radar much more frequently when dog is around.
		m_flNextRadarUpdateTime = gpGlobals->curtime + RADAR_UPDATE_FREQUENCY_FAST;
	}

	//Msg("Server detected %d objects\n", m_iNumRadarContacts );

	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	CSingleUserRecipientFilter filter(pPlayer);
	UserMessageBegin( filter, "UpdateJalopyRadar" );
	WRITE_BYTE( 0 ); // end marker
	MessageEnd();	// send message
}

ConVar jalopy_cargo_anim_time( "jalopy_cargo_anim_time", "1.0" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::UpdateCargoEntry( void )
{
	// Don't bother if we have no prop to move
	if ( m_hCargoProp == NULL )
		return;

	// If we're past our animation point, then we're already done
	if ( m_flCargoStartTime + jalopy_cargo_anim_time.GetFloat() < gpGlobals->curtime )
	{
		// Close the hold immediately if we're finished
		if ( m_bAddingCargo )
		{
			m_flAmmoCrateCloseTime = gpGlobals->curtime;
			m_bAddingCargo = false;
		}

		return;
	}

	// Get our target point
	int nAttachment = LookupAttachment( "cargo" );
	Vector vecTarget, vecOut;
	QAngle vecAngles;
	GetAttachmentLocal( nAttachment, vecTarget, vecAngles );

	// Find where we are in the blend and bias it for a fast entry and slow ease-out
	float flPerc = (jalopy_cargo_anim_time.GetFloat()) ? (( gpGlobals->curtime - m_flCargoStartTime ) / jalopy_cargo_anim_time.GetFloat()) : 1.0f;
	flPerc = Bias( flPerc, 0.75f );
	VectorLerp( m_hCargoProp->GetLocalOrigin(), vecTarget, flPerc, vecOut );

	// Get our target orientation
	CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(m_hCargoProp.Get());
	if ( pProp == NULL )
		return;

	// Slerp our quaternions to find where we are this frame
	Quaternion	qtTarget;
	QAngle qa( 0, 90, 0 );
	qa += pProp->PreferredCarryAngles();
	AngleQuaternion( qa, qtTarget );	// FIXME: Find the real offset to make this sit properly
	Quaternion	qtCurrent;
	AngleQuaternion( pProp->GetLocalAngles(), qtCurrent );

	Quaternion qtOut;
	QuaternionSlerp( qtCurrent, qtTarget, flPerc, qtOut );

	// Put it back to angles
	QuaternionAngles( qtOut, vecAngles );

	// Finally, take these new position
	m_hCargoProp->SetLocalOrigin( vecOut );
	m_hCargoProp->SetLocalAngles( vecAngles );

	// Push the closing out into the future to make sure we don't try and close at the same time
	m_flAmmoCrateCloseTime += gpGlobals->frametime;
}

#define VEHICLE_AVOID_BROADCAST_RATE	0.5f

//-----------------------------------------------------------------------------
// Purpose: This function isn't really what we want
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::CreateAvoidanceZone( void )
{
	if ( m_flNextAvoidBroadcastTime > gpGlobals->curtime )
		return;

	// Only do this when we're stopped
	if ( m_VehiclePhysics.GetSpeed() > 5.0f )
		return;

	float flHullRadius = CollisionProp()->BoundingRadius2D();
	
	Vector	vecPos;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.33f, 0.25f ), &vecPos );
	CSoundEnt::InsertSound( SOUND_MOVE_AWAY, vecPos, (flHullRadius*0.4f), VEHICLE_AVOID_BROADCAST_RATE, this );
	// NDebugOverlay::Sphere( vecPos, vec3_angle, flHullRadius*0.4f, 255, 0, 0, 0, true, VEHICLE_AVOID_BROADCAST_RATE );

	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.66f, 0.25f ), &vecPos );
	CSoundEnt::InsertSound( SOUND_MOVE_AWAY, vecPos, (flHullRadius*0.4f), VEHICLE_AVOID_BROADCAST_RATE, this );
	// NDebugOverlay::Sphere( vecPos, vec3_angle, flHullRadius*0.4f, 255, 0, 0, 0, true, VEHICLE_AVOID_BROADCAST_RATE );

	// Don't broadcast again until these are done
	m_flNextAvoidBroadcastTime = gpGlobals->curtime + VEHICLE_AVOID_BROADCAST_RATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::Think( void )
{
	BaseClass::Think();

	// If our passenger is transitioning, then don't let the player drive off
	CNPC_Alyx *pAlyx = CNPC_Alyx::GetAlyx();
	if ( pAlyx && pAlyx->GetPassengerState() == PASSENGER_STATE_EXITING )
	{
		m_throttleDisableTime = gpGlobals->curtime + 0.25f;		
	}

	// Update our cargo entering our hold
	UpdateCargoEntry();

	// See if the wheel dust should be on or off
	UpdateWheelDust();	

	// Update the radar, of course.
	UpdateRadar();

	if ( m_hCargoTrigger && !m_hCargoProp && !m_hCargoTrigger->m_pfnTouch )
	{
		m_hCargoTrigger->Enable();
	}

	CreateAvoidanceZone();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::AddPropToCargoHold( CPhysicsProp *pProp )
{
	// The hold must be empty to add something to it
	if ( m_hCargoProp != NULL )
	{
		Assert( 0 );
		return;
	}
	
	// Take the prop as our cargo
	m_hCargoProp = pProp;
	m_flCargoStartTime = gpGlobals->curtime;
	m_bAddingCargo = true;
}

//-----------------------------------------------------------------------------
// Purpose: Drops the cargo from the hold
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::ReleasePropFromCargoHold( void )
{
	// Pull the object free!
	m_hCargoProp->SetParent( NULL );
	m_hCargoProp->CreateVPhysics();

	if ( m_hCargoTrigger )
	{
		m_hCargoTrigger->Enable();
		m_hCargoTrigger->IgnoreTouches( m_hCargoProp );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If the player is trying to pull the cargo out of the hold using the physcannon, let him
// Output : Returns the cargo to pick up, if all the conditions are met
//-----------------------------------------------------------------------------
CBaseEntity *CPropJeepEpisodic::OnFailedPhysGunPickup( Vector vPhysgunPos )
{
	// Make sure we're available to open
	if ( m_hCargoProp != NULL )
	{
		// Player's forward direction
		Vector vecPlayerForward;
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer == NULL )
			return NULL;

		pPlayer->EyeVectors( &vecPlayerForward );

		// Origin and facing of the cargo hold
		Vector vecCargoOrigin;
		Vector vecCargoForward;
		GetAttachment( "cargo", vecCargoOrigin, &vecCargoForward );

		// Direction from the cargo to the player's position
		Vector vecPickupDir = ( vecCargoOrigin - vPhysgunPos );
		float flDist = VectorNormalize( vecPickupDir );

		// We need to make sure the player's position is within a cone near the opening and that they're also facing the right way
		bool bInCargoRange = ( (flDist < (15.0f * 12.0f)) && DotProduct( vecCargoForward, vecPickupDir ) < 0.1f );
		bool bFacingCargo = DotProduct( vecPlayerForward, vecPickupDir ) > 0.975f;

		// If we're roughly pulling at the item, pick that up
		if ( bInCargoRange && bFacingCargo )
		{
			// Save this for later
			CBaseEntity *pCargo = m_hCargoProp;
			
			// Drop the cargo
			ReleasePropFromCargoHold();
			
			// Forget the item but pass it back as the object to pick up
			m_hCargoProp = NULL;
			return pCargo;
		}
	}

	return BaseClass::OnFailedPhysGunPickup( vPhysgunPos );
}

// adds a collision solver for any small props that are stuck under the vehicle
static void SolveBlockingProps( CPropJeepEpisodic *pVehicleEntity, IPhysicsObject *pVehiclePhysics )
{
	CUtlVector<CBaseEntity *> solveList;
	float vehicleMass = pVehiclePhysics->GetMass();
	Vector vehicleUp;
	pVehicleEntity->GetVectors( NULL, NULL, &vehicleUp );
	IPhysicsFrictionSnapshot *pSnapshot = pVehiclePhysics->CreateFrictionSnapshot();
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		float otherMass = pOther->GetMass();
		CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
		Assert(pOtherEntity);
		if ( pOtherEntity && pOtherEntity->GetMoveType() == MOVETYPE_VPHYSICS && pOther->IsMoveable() && (otherMass*4.0f) < vehicleMass )
		{
			Vector normal;
			pSnapshot->GetSurfaceNormal(normal);
			// this points down in the car's reference frame, then it's probably trapped under the car
			if ( DotProduct(normal, vehicleUp) < -0.9f )
			{
				Vector point, pointLocal;
				pSnapshot->GetContactPoint(point);
				VectorITransform( point, pVehicleEntity->EntityToWorldTransform(), pointLocal );
				Vector bottomPoint = physcollision->CollideGetExtent( pVehiclePhysics->GetCollide(), vec3_origin, vec3_angle, Vector(0,0,-1) );
				// make sure it's under the bottom of the car
				float bottomPlane = DotProduct(bottomPoint,vehicleUp)+8;	// 8 inches above bottom
				if ( DotProduct( pointLocal, vehicleUp ) <= bottomPlane )
				{
					//Msg("Solved %s\n", pOtherEntity->GetClassname());
					if ( solveList.Find(pOtherEntity) < 0 )
					{
						solveList.AddToTail(pOtherEntity);
					}
				}
			}
		}
		pSnapshot->NextFrictionData();
	}
	pVehiclePhysics->DestroyFrictionSnapshot( pSnapshot );
	if ( solveList.Count() )
	{
		for ( int i = 0; i < solveList.Count(); i++ )
		{
			EntityPhysics_CreateSolver( pVehicleEntity, solveList[i], true, 4.0f );
		}
		pVehiclePhysics->RecheckContactPoints();
	}
}

static void SimpleCollisionResponse( Vector velocityIn, const Vector &normal, float coefficientOfRestitution, Vector *pVelocityOut )
{
	Vector Vn = DotProduct(velocityIn,normal) * normal;
	Vector Vt = velocityIn - Vn;
	*pVelocityOut = Vt - coefficientOfRestitution * Vn;
}

static void KillBlockingEnemyNPCs( CBasePlayer *pPlayer, CBaseEntity *pVehicleEntity, IPhysicsObject *pVehiclePhysics )
{
	Vector velocity;
	pVehiclePhysics->GetVelocity( &velocity, NULL );
	float vehicleMass = pVehiclePhysics->GetMass();

	// loop through the contacts and look for enemy NPCs that we're pushing on
	CUtlVector<CAI_BaseNPC *> npcList;
	CUtlVector<Vector> forceList;
	CUtlVector<Vector> contactList;
	IPhysicsFrictionSnapshot *pSnapshot = pVehiclePhysics->CreateFrictionSnapshot();
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		float otherMass = pOther->GetMass();
		CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
		CAI_BaseNPC *pNPC = pOtherEntity ? pOtherEntity->MyNPCPointer() : NULL;
		// Is this an enemy NPC with a small enough mass?
		if ( pNPC && pPlayer->IRelationType(pNPC) != D_LI && ((otherMass*2.0f) < vehicleMass) )
		{
			// accumulate the stress force for this NPC in the lsit
			float force = pSnapshot->GetNormalForce();
			Vector normal;
			pSnapshot->GetSurfaceNormal(normal);
			normal *= force;
			int index = npcList.Find(pNPC);
			if ( index < 0 )
			{
				vphysicsupdateai_t *pUpdate = NULL;
				if ( pNPC->VPhysicsGetObject() && pNPC->VPhysicsGetObject()->GetShadowController() && pNPC->GetMoveType() == MOVETYPE_STEP )
				{
					if ( pNPC->HasDataObjectType(VPHYSICSUPDATEAI) )
					{
						pUpdate = static_cast<vphysicsupdateai_t *>(pNPC->GetDataObject(VPHYSICSUPDATEAI));
						// kill this guy if I've been pushing him for more than half a second and I'm 
						// still pushing in his direction
						if ( (gpGlobals->curtime - pUpdate->startUpdateTime) > 0.5f && DotProduct(velocity,normal) > 0)
						{
							index = npcList.AddToTail(pNPC);
							forceList.AddToTail( normal );
							Vector pos;
							pSnapshot->GetContactPoint(pos);
							contactList.AddToTail(pos);
						}
					}
					else
					{
						pUpdate = static_cast<vphysicsupdateai_t *>(pNPC->CreateDataObject( VPHYSICSUPDATEAI ));
						pUpdate->startUpdateTime = gpGlobals->curtime;
					}
					// update based on vphysics for the next second
					// this allows the car to push the NPC
					pUpdate->stopUpdateTime = gpGlobals->curtime + 1.0f;
					float maxAngular;
					pNPC->VPhysicsGetObject()->GetShadowController()->GetMaxSpeed( &pUpdate->savedShadowControllerMaxSpeed, &maxAngular );
					pNPC->VPhysicsGetObject()->GetShadowController()->MaxSpeed( 1.0f, maxAngular );
				}
			}
			else
			{
				forceList[index] += normal;
			}
		}
		pSnapshot->NextFrictionData();
	}
	pVehiclePhysics->DestroyFrictionSnapshot( pSnapshot );
	// now iterate the list and check each cumulative force against the threshold
	if ( npcList.Count() )
	{
		for ( int i = npcList.Count(); --i >= 0; )
		{
			Vector damageForce;
			npcList[i]->VPhysicsGetObject()->GetVelocity( &damageForce, NULL );
			Vector vel;
			pVehiclePhysics->GetVelocityAtPoint( contactList[i], &vel );
			damageForce -= vel;
			Vector normal = forceList[i];
			VectorNormalize(normal);
			SimpleCollisionResponse( damageForce, normal, 1.0, &damageForce );
			damageForce += (normal * 300.0f);
			damageForce *= npcList[i]->VPhysicsGetObject()->GetMass();
			float len = damageForce.Length();
			damageForce.z += len*phys_upimpactforcescale.GetFloat();
			Vector vehicleForce = -damageForce;

			CTakeDamageInfo dmgInfo( pVehicleEntity, pVehicleEntity, damageForce, contactList[i], 200.0f, DMG_CRUSH|DMG_VEHICLE );
			npcList[i]->TakeDamage( dmgInfo );
			pVehiclePhysics->ApplyForceOffset( vehicleForce, contactList[i] );
			PhysCollisionSound( pVehicleEntity, npcList[i]->VPhysicsGetObject(), CHAN_BODY, pVehiclePhysics->GetMaterialIndex(), npcList[i]->VPhysicsGetObject()->GetMaterialIndex(), gpGlobals->frametime, 200.0f );
		}
	}
}

void CPropJeepEpisodic::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	/* The car headlight hurts perf, there's no timer to turn it off automatically,
	   and we haven't built any gameplay around it.

	   Furthermore, I don't think I've ever seen a playtester turn it on.
	
	if ( ucmd->impulse == 100 )
	{
		if (HeadlightIsOn())
		{
			HeadlightTurnOff();
		}
		else 
		{
			HeadlightTurnOn();
		}
	}*/
	
	if ( ucmd->forwardmove != 0.0f )
	{
		//Msg("Push V: %.2f, %.2f, %.2f\n", ucmd->forwardmove, carState->engineRPM, carState->speed );
		CBasePlayer *pPlayer = ToBasePlayer(GetDriver());

		if ( pPlayer && VPhysicsGetObject() )
		{
			KillBlockingEnemyNPCs( pPlayer, this, VPhysicsGetObject() );
			SolveBlockingProps( this, VPhysicsGetObject() );
		}
	}
	BaseClass::DriveVehicle(flFrameTime, ucmd, iButtonsDown, iButtonsReleased);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::CreateHazardLights( void )
{
	static const char *s_szAttach[NUM_HAZARD_LIGHTS] =
	{
		"rearlight_r",
		"rearlight_l",
		"headlight_r",
		"headlight_l",
	};

	// Turn on the hazards!	
	for ( int i = 0; i < NUM_HAZARD_LIGHTS; i++ )
	{
		if ( m_hHazardLights[i] == NULL )
		{
			m_hHazardLights[i] = CSprite::SpriteCreate( s_szHazardSprite, GetLocalOrigin(), false );
			if ( m_hHazardLights[i] )
			{
				m_hHazardLights[i]->SetTransparency( kRenderWorldGlow, 255, 220, 40, 255, kRenderFxNoDissipation );
				m_hHazardLights[i]->SetAttachment( this, LookupAttachment( s_szAttach[i] ) );
				m_hHazardLights[i]->SetGlowProxySize( 2.0f );
				m_hHazardLights[i]->TurnOff();
				if ( i < 2 )
				{
					// Rear lights are red
					m_hHazardLights[i]->SetColor( 255, 0, 0 );
					m_hHazardLights[i]->SetScale( 1.0f );
				}
				else
				{
					// Font lights are white
					m_hHazardLights[i]->SetScale( 1.0f );
				}
			}
		}
	}

	// We start off
	m_bBlink = false;

	// Setup our blink
	SetContextThink( &CPropJeepEpisodic::HazardBlinkThink, gpGlobals->curtime + 0.1f, "HazardBlink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::DestroyHazardLights( void )
{
	for ( int i = 0; i < NUM_HAZARD_LIGHTS; i++ )
	{
		if ( m_hHazardLights[i] != NULL )
		{
			UTIL_Remove( m_hHazardLights[i] );
		}
	}

	SetContextThink( NULL, gpGlobals->curtime, "HazardBlink" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nRole - 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::ExitVehicle( int nRole )
{
	BaseClass::ExitVehicle( nRole );

	CreateHazardLights();
}

void CPropJeepEpisodic::SetBusterHopperVisibility(bool visible)
{
	// if we're there already do nothing
	if (visible == m_bBusterHopperVisible)
		return;

	SetBodygroup( JEEP_HOPPER_BODYGROUP, visible ? 1 : 0);
	m_bBusterHopperVisible = visible;
}


void CPropJeepEpisodic::InputSetCargoVisibility( inputdata_t &data )
{
	bool visible = data.value.Bool();

	SetBusterHopperVisibility( visible );
}

//-----------------------------------------------------------------------------
// THIS CODE LIFTED RIGHT OUT OF TF2, to defer the pain of making vgui-on-an-entity
// code available to all CBaseAnimating.
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::SpawnRadarPanel()
{
	// FIXME: Deal with dynamically resizing control panels?

	// If we're attached to an entity, spawn control panels on it instead of use
	CBaseAnimating *pEntityToSpawnOn = this;
	char *pOrgLL = "controlpanel0_ll";
	char *pOrgUR = "controlpanel0_ur";

	Assert( pEntityToSpawnOn );

	// Lookup the attachment point...
	int nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(pOrgLL);

	if (nLLAttachmentIndex <= 0)
	{
		return;
	}

	int nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(pOrgUR);
	if (nURAttachmentIndex <= 0)
	{
		return;
	}

	const char *pScreenName = "jalopy_radar_panel";
	const char *pScreenClassname = "vgui_screen";

	// Compute the screen size from the attachment points...
	matrix3x4_t	panelToWorld;
	pEntityToSpawnOn->GetAttachment( nLLAttachmentIndex, panelToWorld );

	matrix3x4_t	worldToPanel;
	MatrixInvert( panelToWorld, worldToPanel );

	// Now get the lower right position + transform into panel space
	Vector lr, lrlocal;
	pEntityToSpawnOn->GetAttachment( nURAttachmentIndex, panelToWorld );
	MatrixGetColumn( panelToWorld, 3, lr );
	VectorTransform( lr, worldToPanel, lrlocal );

	float flWidth = lrlocal.x;
	float flHeight = lrlocal.y;

	CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, pEntityToSpawnOn, this, nLLAttachmentIndex );
	pScreen->SetActualSize( flWidth, flHeight );
	pScreen->SetActive( true );
	pScreen->SetOverlayMaterial( RADAR_PANEL_WRITEZ );
	pScreen->SetTransparency( true );

	m_hRadarScreen.Set( pScreen );

	m_bRadarEnabled = true;
	m_iNumRadarContacts = 0;
	m_flNextRadarUpdateTime = gpGlobals->curtime - 1.0f;
}

//-----------------------------------------------------------------------------
void CPropJeepEpisodic::DestroyRadarPanel()
{
	Assert( m_hRadarScreen != NULL );
	m_hRadarScreen->SUB_Remove();
	m_bRadarEnabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::HazardBlinkThink( void )
{
	if ( m_bBlink )
	{
		for ( int i = 0; i < NUM_HAZARD_LIGHTS; i++ )
		{
			if ( m_hHazardLights[i] )
			{
				m_hHazardLights[i]->SetBrightness( 0, 0.1f );
			}
		}

		SetContextThink( &CPropJeepEpisodic::HazardBlinkThink, gpGlobals->curtime + 0.25f, "HazardBlink" );
	}
	else
	{
		for ( int i = 0; i < NUM_HAZARD_LIGHTS; i++ )
		{
			if ( m_hHazardLights[i] )
			{
				m_hHazardLights[i]->SetBrightness( 255, 0.1f );
				m_hHazardLights[i]->TurnOn();
			}
		}

		SetContextThink( &CPropJeepEpisodic::HazardBlinkThink, gpGlobals->curtime + 0.5f, "HazardBlink" );
	}

	m_bBlink = !m_bBlink;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::HandleWater( void )
{
	// Only check the wheels and engine in water if we have a driver (player).
	if ( !GetDriver() )
		return;

	// Update our internal state
	CheckWater();

	// Save of data from last think.
	for ( int iWheel = 0; iWheel < JEEP_WHEEL_COUNT; ++iWheel )
	{
		m_WaterData.m_bWheelWasInWater[iWheel] = m_WaterData.m_bWheelInWater[iWheel];
	}
}

//-----------------------------------------------------------------------------
// Purpose: Report our lock state
//-----------------------------------------------------------------------------
int	CPropJeepEpisodic::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		EntityText( text_offset, CFmtStr("Entrance: %s", m_bEntranceLocked ? "Locked" : "Unlocked" ), 0 );
		text_offset++;

		EntityText( text_offset, CFmtStr("Exit: %s", m_bExitLocked ? "Locked" : "Unlocked" ), 0 );
		text_offset++;
	}

	return text_offset;
}

#define TRANSITION_SEARCH_RADIUS	(100*12)

//-----------------------------------------------------------------------------
// Purpose: Teleport the car to a destination that will cause it to transition if it's not going to otherwise
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputOutsideTransition( inputdata_t &inputdata )
{
	// Teleport into the new map
	CBasePlayer *pPlayer = AI_GetSinglePlayer();
	Vector vecTeleportPos;
	QAngle vecTeleportAngles;

	// Get our bounds
	Vector vecSurroundMins, vecSurroundMaxs;
	CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
	vecSurroundMins -= WorldSpaceCenter();
	vecSurroundMaxs -= WorldSpaceCenter();

	Vector vecBestPos;
	QAngle vecBestAngles;

	CInfoTargetVehicleTransition *pEntity = NULL;
	bool bSucceeded = false;

	// Find all entities of the correct name and try and sit where they're at
	while ( ( pEntity = (CInfoTargetVehicleTransition *) gEntList.FindEntityByClassname( pEntity, "info_target_vehicle_transition" ) ) != NULL )
	{
		// Must be enabled
		if ( pEntity->IsDisabled() )
			continue;

		// Must be within range
		if ( ( pEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr() > Square( TRANSITION_SEARCH_RADIUS ) )
			continue;

		vecTeleportPos = pEntity->GetAbsOrigin();
		vecTeleportAngles = pEntity->GetAbsAngles() + QAngle( 0, -90, 0 );	// Vehicle is always off by 90 degrees

		// Rotate to face the destination angles
		Vector vecMins;
		Vector vecMaxs;
		VectorRotate( vecSurroundMins, vecTeleportAngles, vecMins );
		VectorRotate( vecSurroundMaxs, vecTeleportAngles, vecMaxs );

		if ( vecMaxs.x < vecMins.x )
			V_swap( vecMins.x, vecMaxs.x );

		if ( vecMaxs.y < vecMins.y )
			V_swap( vecMins.y, vecMaxs.y );

		if ( vecMaxs.z < vecMins.z )
			V_swap( vecMins.z, vecMaxs.z );

		// Move up
		vecTeleportPos.z += ( vecMaxs.z - vecMins.z );

		trace_t	tr;
		UTIL_TraceHull( vecTeleportPos, vecTeleportPos - Vector( 0, 0, 128 ), vecMins, vecMaxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		if ( tr.startsolid == false && tr.allsolid == false && tr.fraction < 1.0f )
		{
			// Store this off
			vecBestPos = tr.endpos;
			vecBestAngles = vecTeleportAngles;
			bSucceeded = true;

			// If this point isn't visible, then stop looking and use it
			if ( pPlayer->FInViewCone( tr.endpos ) == false )
				break;
		}
	}

	// See if we're finished
	if ( bSucceeded )
	{
		Teleport( &vecTeleportPos, &vecTeleportAngles, NULL );
		return;
	}

	// TODO: We found no valid teleport points, so try to find them dynamically
	Warning("No valid vehicle teleport points!\n");
}

//-----------------------------------------------------------------------------
// Purpose: Stop players punting the car around.
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputDisablePhysGun( inputdata_t &data )
{
	AddEFlags( EFL_NO_PHYSCANNON_INTERACTION );
}
//-----------------------------------------------------------------------------
// Purpose: Return to normal
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputEnablePhysGun( inputdata_t &data )
{
	RemoveEFlags( EFL_NO_PHYSCANNON_INTERACTION );
}

//-----------------------------------------------------------------------------
// Create and parent two radial node link controllers.
//-----------------------------------------------------------------------------
void CPropJeepEpisodic::InputCreateLinkController( inputdata_t &data )
{
	Vector vecFront, vecRear;
	Vector vecWFL, vecWFR;	// Front wheels
	Vector vecWRL, vecWRR;	// Back wheels

	GetAttachment( "wheel_fr", vecWFR );
	GetAttachment( "wheel_fl", vecWFL );

	GetAttachment( "wheel_rr", vecWRR );
	GetAttachment( "wheel_rl", vecWRL );

	vecFront = (vecWFL + vecWFR) * 0.5f;
	vecRear = (vecWRL + vecWRR) * 0.5f;

	float flRadius = ( (vecFront - vecRear).Length() ) * 0.6f;

	CAI_RadialLinkController *pLinkController = (CAI_RadialLinkController *)CreateEntityByName( "info_radial_link_controller" );
	if( pLinkController != NULL && m_hLinkControllerFront.Get() == NULL )
	{
		pLinkController->m_flRadius = flRadius;
		pLinkController->Spawn();
		pLinkController->SetAbsOrigin( vecFront );
		pLinkController->SetOwnerEntity( this );
		pLinkController->SetParent( this );
		pLinkController->Activate();
		m_hLinkControllerFront.Set( pLinkController );

		//NDebugOverlay::Circle( vecFront, Vector(0,1,0), Vector(1,0,0), flRadius, 255, 255, 255, 128, false, 100 );
	}

	pLinkController = (CAI_RadialLinkController *)CreateEntityByName( "info_radial_link_controller" );
	if( pLinkController != NULL && m_hLinkControllerRear.Get() == NULL  )
	{
		pLinkController->m_flRadius = flRadius;
		pLinkController->Spawn();
		pLinkController->SetAbsOrigin( vecRear );
		pLinkController->SetOwnerEntity( this );
		pLinkController->SetParent( this );
		pLinkController->Activate();
		m_hLinkControllerRear.Set( pLinkController );

		//NDebugOverlay::Circle( vecRear, Vector(0,1,0), Vector(1,0,0), flRadius, 255, 255, 255, 128, false, 100 );
	}
}

void CPropJeepEpisodic::InputDestroyLinkController( inputdata_t &data )
{
	if( m_hLinkControllerFront.Get() != NULL )
	{
		CAI_RadialLinkController *pLinkController = dynamic_cast<CAI_RadialLinkController*>(m_hLinkControllerFront.Get());
		if( pLinkController != NULL )
		{
			pLinkController->ModifyNodeLinks(false);
			UTIL_Remove( pLinkController );
			m_hLinkControllerFront.Set(NULL);
		}
	}

	if( m_hLinkControllerRear.Get() != NULL )
	{
		CAI_RadialLinkController *pLinkController = dynamic_cast<CAI_RadialLinkController*>(m_hLinkControllerRear.Get());
		if( pLinkController != NULL )
		{
			pLinkController->ModifyNodeLinks(false);
			UTIL_Remove( pLinkController );
			m_hLinkControllerRear.Set(NULL);
		}
	}
}


bool CPropJeepEpisodic::AllowBlockedExit( CBaseCombatCharacter *pPassenger, int nRole )
{
	// Wait until we've settled down before we resort to blocked exits.
	// This keeps us from doing blocked exits in mid-jump, which can cause mayhem like
	// sticking the player through player clips or into geometry.
	return GetSmoothedVelocity().IsLengthLessThan( jalopy_blocked_exit_max_speed.GetFloat() );
}

