//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: UNDONE: Rename this to prop_vehicle.cpp !!!
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "vehicle_base.h"
#include "ndebugoverlay.h"
#include "igamemovement.h"
#include "soundenvelope.h"
#include "in_buttons.h"
#include "npc_vehicledriver.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "func_break.h"
#include "physics_impact_damage.h"
#include "entityblocker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_PROP_VEHICLE_ALWAYSTHINK		0x00000001

ConVar g_debug_vehiclebase( "g_debug_vehiclebase", "0", FCVAR_CHEAT );
extern ConVar g_debug_vehicledriver;

// CFourWheelServerVehicle
BEGIN_SIMPLE_DATADESC_( CFourWheelServerVehicle, CBaseServerVehicle )

	DEFINE_EMBEDDED( m_ViewSmoothing ),

END_DATADESC()

// CPropVehicle
BEGIN_DATADESC( CPropVehicle )

	DEFINE_EMBEDDED( m_VehiclePhysics ),

	// These are necessary to save here because the 'owner' of these fields must be the prop_vehicle
	DEFINE_PHYSPTR( m_VehiclePhysics.m_pVehicle ),
	DEFINE_PHYSPTR_ARRAY( m_VehiclePhysics.m_pWheels ),

	DEFINE_FIELD( m_nVehicleType, FIELD_INTEGER ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

#ifdef HL2_EPISODIC
	DEFINE_UTLVECTOR( m_hPhysicsChildren, FIELD_EHANDLE ),
#endif // HL2_EPISODIC

	// Keys
	DEFINE_KEYFIELD( m_vehicleScript, FIELD_STRING, "VehicleScript" ),
	DEFINE_FIELD( m_vecSmoothedVelocity, FIELD_VECTOR ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Throttle", InputThrottle ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Steer", InputSteering ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "Action", InputAction ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HandBrakeOn", InputHandBrakeOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HandBrakeOff", InputHandBrakeOff ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( prop_vehicle, CPropVehicle );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#pragma warning (disable:4355)
CPropVehicle::CPropVehicle() : m_VehiclePhysics( this )
{
	SetVehicleType( VEHICLE_TYPE_CAR_WHEELS );
}
#pragma warning (default:4355)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicle::~CPropVehicle ()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::Spawn( )
{
	CFourWheelServerVehicle *pServerVehicle = dynamic_cast<CFourWheelServerVehicle*>(GetServerVehicle());
	m_VehiclePhysics.SetOuter( this, pServerVehicle );

	// NOTE: The model has to be set before we can spawn vehicle physics
	BaseClass::Spawn();
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	m_VehiclePhysics.Spawn();
	if (!m_VehiclePhysics.Initialize( STRING(m_vehicleScript), m_nVehicleType ))
		return;
	SetNextThink( gpGlobals->curtime );

	m_vecSmoothedVelocity.Init();
}

// this allows reloading the script variables from disk over an existing vehicle state
// This is useful for tuning vehicles or updating old saved game formats
CON_COMMAND(vehicle_flushscript, "Flush and reload all vehicle scripts")
{
	PhysFlushVehicleScripts();
	for ( CBaseEntity *pEnt = gEntList.FirstEnt(); pEnt != NULL; pEnt = gEntList.NextEnt(pEnt) )
	{
		IServerVehicle *pServerVehicle = pEnt->GetServerVehicle();
		if ( pServerVehicle )
		{
			pServerVehicle->ReloadScript();
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: Restore
//-----------------------------------------------------------------------------
int CPropVehicle::Restore( IRestore &restore )
{
	CFourWheelServerVehicle *pServerVehicle = dynamic_cast<CFourWheelServerVehicle*>(GetServerVehicle());
	m_VehiclePhysics.SetOuter( this, pServerVehicle );
	return BaseClass::Restore( restore );
}


//-----------------------------------------------------------------------------
// Purpose: Tell the vehicle physics system whenever we teleport, so it can fixup the wheels.
//-----------------------------------------------------------------------------
void CPropVehicle::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	matrix3x4_t startMatrixInv;

	MatrixInvert( EntityToWorldTransform(), startMatrixInv );
	BaseClass::Teleport( newPosition, newAngles, newVelocity );

	// Calculate the relative transform of the teleport
	matrix3x4_t xform;
	ConcatTransforms( EntityToWorldTransform(), startMatrixInv, xform );
	m_VehiclePhysics.Teleport( xform );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::DrawDebugGeometryOverlays()
{
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{	
		m_VehiclePhysics.DrawDebugGeometryOverlays();
	}
	BaseClass::DrawDebugGeometryOverlays();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropVehicle::DrawDebugTextOverlays()
{
	int nOffset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		nOffset = m_VehiclePhysics.DrawDebugTextOverlays( nOffset );
	}
	return nOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePlayer *CPropVehicle::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Keep track of physgun influence
//-----------------------------------------------------------------------------
void CPropVehicle::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputThrottle( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetThrottle( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputSteering( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetSteering( inputdata.value.Float(), 2*gpGlobals->frametime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::InputAction( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetAction( inputdata.value.Float() );
}

void CPropVehicle::InputHandBrakeOn( inputdata_t &inputdata )
{
	m_VehiclePhysics.SetHandbrake( true );
}

void CPropVehicle::InputHandBrakeOff( inputdata_t &inputdata )
{
	m_VehiclePhysics.ReleaseHandbrake();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::Think()
{
	m_VehiclePhysics.Think();

	// Derived classes of CPropVehicle have their own code to determine how frequently to think.
	// But the prop_vehicle entity native to this class will only think one time, so this flag
	// was added to allow prop_vehicle to always think without affecting the derived classes.
	if( HasSpawnFlags(SF_PROP_VEHICLE_ALWAYSTHINK) )
	{
		SetNextThink(gpGlobals->curtime);
	}
}

#define SMOOTHING_FACTOR 0.9

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicle::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	if ( IsMarkedForDeletion() )
		return;

	Vector	velocity;
	VPhysicsGetObject()->GetVelocity( &velocity, NULL );

	//Update our smoothed velocity
	m_vecSmoothedVelocity = m_vecSmoothedVelocity * SMOOTHING_FACTOR + velocity * ( 1 - SMOOTHING_FACTOR );

	// must be a wheel
	if (!m_VehiclePhysics.VPhysicsUpdate( pPhysics ))
		return;
	
	BaseClass::VPhysicsUpdate( pPhysics );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const Vector
//-----------------------------------------------------------------------------
Vector CPropVehicle::GetSmoothedVelocity( void )
{
	return m_vecSmoothedVelocity;
}

//=============================================================================
#ifdef HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: Add an entity to a list which receives physics callbacks from the vehicle
//-----------------------------------------------------------------------------
void CPropVehicle::AddPhysicsChild( CBaseEntity *pChild )
{
	// Don't add something we already have
	if ( m_hPhysicsChildren.Find( pChild ) != m_hPhysicsChildren.InvalidIndex() )
		return ;

	m_hPhysicsChildren.AddToTail( pChild );
}

//-----------------------------------------------------------------------------
// Purpose: Removes entity from physics callback list
//-----------------------------------------------------------------------------
void CPropVehicle::RemovePhysicsChild( CBaseEntity *pChild )
{
	int elemID = m_hPhysicsChildren.Find( pChild );

	if ( m_hPhysicsChildren.IsValidIndex( elemID ) )
	{
		m_hPhysicsChildren.Remove( elemID );
	}
}

#endif //HL2_EPISODIC
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Player driveable vehicle class
//-----------------------------------------------------------------------------

IMPLEMENT_SERVERCLASS_ST(CPropVehicleDriveable, DT_PropVehicleDriveable)

	SendPropEHandle(SENDINFO(m_hPlayer)),
//	SendPropFloat(SENDINFO_DT_NAME(m_controls.throttle, m_throttle), 8,	SPROP_ROUNDUP,	0.0f,	1.0f),
	SendPropInt(SENDINFO(m_nSpeed),	8),
	SendPropInt(SENDINFO(m_nRPM), 13),
	SendPropFloat(SENDINFO(m_flThrottle), 0, SPROP_NOSCALE ),
	SendPropInt(SENDINFO(m_nBoostTimeLeft), 8),
	SendPropInt(SENDINFO(m_nHasBoost), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nScannerDisabledWeapons), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nScannerDisabledVehicle), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_bEnterAnimOn), 1, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_bExitAnimOn), 1, SPROP_UNSIGNED ),
	SendPropInt(SENDINFO(m_bUnableToFire), 1, SPROP_UNSIGNED ),
	SendPropVector(SENDINFO(m_vecEyeExitEndpoint), -1, SPROP_COORD),
	SendPropBool(SENDINFO(m_bHasGun)),
	SendPropVector(SENDINFO(m_vecGunCrosshair), -1, SPROP_COORD),
END_SEND_TABLE();

BEGIN_DATADESC( CPropVehicleDriveable )
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Lock",	InputLock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unlock",	InputUnlock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn",	InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUT( m_bHasGun, FIELD_BOOLEAN, "EnableGun" ),

	// Outputs
	DEFINE_OUTPUT( m_playerOn, "PlayerOn" ),
	DEFINE_OUTPUT( m_playerOff, "PlayerOff" ),
	DEFINE_OUTPUT( m_pressedAttack, "PressedAttack" ),
	DEFINE_OUTPUT( m_pressedAttack2, "PressedAttack2" ),
	DEFINE_OUTPUT( m_attackaxis, "AttackAxis" ),
	DEFINE_OUTPUT( m_attack2axis, "Attack2Axis" ),
	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),

	DEFINE_EMBEDDEDBYREF( m_pServerVehicle ),
	DEFINE_FIELD( m_nSpeed, FIELD_INTEGER ),
	DEFINE_FIELD( m_nRPM, FIELD_INTEGER ),
	DEFINE_FIELD( m_flThrottle, FIELD_FLOAT ),
	DEFINE_FIELD( m_nBoostTimeLeft, FIELD_INTEGER ),
	DEFINE_FIELD( m_nHasBoost, FIELD_INTEGER ),
	DEFINE_FIELD( m_nScannerDisabledWeapons, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nScannerDisabledVehicle, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bUnableToFire, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecEyeExitEndpoint, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecGunCrosshair, FIELD_VECTOR ),

	DEFINE_FIELD( m_bEngineLocked, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bLocked, FIELD_BOOLEAN, "VehicleLocked" ),
	DEFINE_FIELD( m_flMinimumSpeedToEnterExit, FIELD_FLOAT ),
	DEFINE_FIELD( m_bEnterAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bExitAnimOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTurnOffKeepUpright, FIELD_TIME ),
	//DEFINE_FIELD( m_flNoImpactDamageTime, FIELD_TIME ),

	DEFINE_FIELD( m_hNPCDriver, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hKeepUpright, FIELD_EHANDLE ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( prop_vehicle_driveable, CPropVehicleDriveable );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable::CPropVehicleDriveable( void ) :
	m_pServerVehicle( NULL ),
	m_hKeepUpright( NULL ),
	m_flTurnOffKeepUpright( 0 ),
	m_flNoImpactDamageTime( 0 )
{
	m_vecEyeExitEndpoint.Init();
	m_vecGunCrosshair.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable::~CPropVehicleDriveable( void )
{
	DestroyServerVehicle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::CreateServerVehicle( void )
{
	// Create our server vehicle
	m_pServerVehicle = new CFourWheelServerVehicle();
	m_pServerVehicle->SetVehicle( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DestroyServerVehicle()
{
	if ( m_pServerVehicle )
	{
		delete m_pServerVehicle;
		m_pServerVehicle = NULL;
	}
}

//------------------------------------------------
// Precache
//------------------------------------------------
void CPropVehicleDriveable::Precache( void )
{
	BaseClass::Precache();

	// This step is needed because if we're precaching from a templated instance, we'll miss our vehicle 
	// script sounds unless we do the parse below.  This instance of the vehicle will be nuked when we're actually created.
	if ( m_pServerVehicle == NULL )
	{
		CreateServerVehicle();
	}
	
	// Load the script file and precache our assets
	if ( m_pServerVehicle )
	{
		m_pServerVehicle->Initialize( STRING( m_vehicleScript ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Spawn( void )
{
	// Has to be created before Spawn is called (since that causes Precache to be called)
	DestroyServerVehicle();
	CreateServerVehicle();
	
	// Initialize our vehicle via script
	if ( m_pServerVehicle->Initialize( STRING(m_vehicleScript) ) == false )
	{
		Warning( "Vehicle (%s) unable to properly initialize due to script error in (%s)!\n", GetEntityName().ToCStr(), STRING( m_vehicleScript ) );
		SetThink( &CBaseEntity::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	BaseClass::Spawn();

	m_flMinimumSpeedToEnterExit = 0;
	m_takedamage = DAMAGE_EVENTS_ONLY;
	m_bEngineLocked = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPropVehicleDriveable::Restore( IRestore &restore )
{
	// Has to be created before	we can restore
	// and we can't create it in the constructor because it could be
	// overridden by a derived class.
	DestroyServerVehicle();
	CreateServerVehicle();

	int nRetVal = BaseClass::Restore( restore );
	 
	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Do extra fix-up after restore
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::OnRestore( void )
{
	BaseClass::OnRestore();

	// NOTE: This is necessary to prevent overflow of datatables on level transition
	// since the last exit eyepoint in the last level will have been fixed up
	// based on the level landmarks, resulting in a position that lies outside
	// typical map coordinates. If we're not in the middle of an exit anim, the
	// eye exit endpoint field isn't being used at all.
	if ( !m_bExitAnimOn )
	{
		m_vecEyeExitEndpoint = GetAbsOrigin();
	}

	m_flNoImpactDamageTime = gpGlobals->curtime + 5.0f;

	IServerVehicle *pServerVehicle = GetServerVehicle();
	if ( pServerVehicle != NULL )
	{
		// Restore the passenger information we're holding on to
		pServerVehicle->RestorePassengerInfo();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Vehicles are permanently oriented off angle for vphysics.
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pRight ); 
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: AngleVectors equivalent that accounts for the hacked 90 degree rotation of vehicles
//			BUGBUG: VPhysics is hardcoded so that vehicles must face down Y instead of X like everything else
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::VehicleAngleVectors( const QAngle &angles, Vector *pForward, Vector *pRight, Vector *pUp )
{
	AngleVectors( angles, pRight, pForward, pUp );
	if ( pForward )
	{
	  	*pForward *= -1;
	}
}
  

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( !pPlayer )
		return;

	ResetUseKey( pPlayer );

	m_pServerVehicle->HandlePassengerEntry( pPlayer, (value>0) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPropVehicleDriveable::GetDriver( void ) 
{ 
	if ( m_hNPCDriver ) 
		return m_hNPCDriver; 

	return m_hPlayer; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::EnterVehicle( CBaseCombatCharacter *pPassenger )
{
	if ( pPassenger == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pPassenger	);
	if ( pPlayer != NULL )
	{
		// Remove any player who may be in the vehicle at the moment
		if ( m_hPlayer )
		{
			ExitVehicle( VEHICLE_ROLE_DRIVER );
		}

		m_hPlayer = pPlayer;
		m_playerOn.FireOutput( pPlayer, this, 0 );

		// Don't start the engine if the player's using an entry animation,
		// because we want to start the engine once the animation is done.
		if ( !m_bEnterAnimOn )
		{
			StartEngine();
		}

		// Start Thinking
		SetNextThink( gpGlobals->curtime );

		Vector vecViewOffset = m_pServerVehicle->GetSavedViewOffset();

		// Clear our state
		m_pServerVehicle->InitViewSmoothing( pPlayer->GetAbsOrigin() + vecViewOffset, pPlayer->EyeAngles() );

		m_VehiclePhysics.GetVehicle()->OnVehicleEnter();
	}
	else
	{
		// NPCs are not yet supported - jdw
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::ExitVehicle( int nRole )
{
	CBasePlayer *pPlayer = m_hPlayer;
	if ( !pPlayer )
		return;

	m_hPlayer = NULL;
	ResetUseKey( pPlayer );
	
	m_playerOff.FireOutput( pPlayer, this, 0 );

	// clear out the fire buttons
	m_attackaxis.Set( 0, pPlayer, this );
	m_attack2axis.Set( 0, pPlayer, this );

	m_nSpeed = 0;
	m_flThrottle = 0.0f;

	StopEngine();

	m_VehiclePhysics.GetVehicle()->OnVehicleExit();

	// Clear our state
	m_pServerVehicle->InitViewSmoothing( vec3_origin, vec3_angle );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::ResetUseKey( CBasePlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DriveVehicle( CBasePlayer *pPlayer, CUserCmd *ucmd )
{
	//Lose control when the player dies
	if ( pPlayer->IsAlive() == false )
		return;

	DriveVehicle( TICK_INTERVAL, ucmd, pPlayer->m_afButtonPressed, pPlayer->m_afButtonReleased );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	int iButtons = ucmd->buttons;

	m_VehiclePhysics.UpdateDriverControls( ucmd, flFrameTime );

	m_nSpeed = m_VehiclePhysics.GetSpeed();	//send speed to client
	m_nRPM = clamp( m_VehiclePhysics.GetRPM(), 0, 4095 );
	m_nBoostTimeLeft = m_VehiclePhysics.BoostTimeLeft();
	m_nHasBoost = m_VehiclePhysics.HasBoost();
	m_flThrottle = m_VehiclePhysics.GetThrottle();

	m_nScannerDisabledWeapons = false;		// off for now, change once we have scanners
	m_nScannerDisabledVehicle = false;		// off for now, change once we have scanners

	//
	// Fire the appropriate outputs based on button pressed events.
	//
	// BUGBUG: m_afButtonPressed is broken - check the player.cpp code!!!
	float attack = 0, attack2 = 0;

	if ( iButtonsDown & IN_ATTACK )
	{
		m_pressedAttack.FireOutput( this, this, 0 );
	}
	if ( iButtonsDown & IN_ATTACK2 )
	{
		m_pressedAttack2.FireOutput( this, this, 0 );
	}

	if ( iButtons & IN_ATTACK )
	{
		attack = 1;
	}
	if ( iButtons & IN_ATTACK2 )
	{
		attack2 = 1;
	}

	m_attackaxis.Set( attack, this, this );
	m_attack2axis.Set( attack2, this, this );
}

//-----------------------------------------------------------------------------
// Purpose: Tells whether or not the car has been overturned
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::IsOverturned( void )
{
	Vector	vUp;
	VehicleAngleVectors( GetAbsAngles(), NULL, NULL, &vUp );

	float	upDot = DotProduct( Vector(0,0,1), vUp );

	// Tweak this number to adjust what's considered "overturned"
	if ( upDot < 0.0f )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Think()
{
	BaseClass::Think();

	if ( ShouldThink() )
	{
		SetNextThink( gpGlobals->curtime );
	}

	// If we have an NPC Driver, tell him to drive
	if ( m_hNPCDriver )
	{
		GetServerVehicle()->NPC_DriveVehicle();
	}

	// Keep thinking while we're waiting to turn off the keep upright
	if ( m_flTurnOffKeepUpright )
	{
		SetNextThink( gpGlobals->curtime );

		// Time up?
		if ( m_hKeepUpright != NULL && m_flTurnOffKeepUpright < gpGlobals->curtime )
		{
			variant_t emptyVariant;
			m_hKeepUpright->AcceptInput( "TurnOff", this, this, emptyVariant, USE_TOGGLE );
			m_flTurnOffKeepUpright = 0;

			UTIL_Remove( m_hKeepUpright );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	// If the engine's not active, prevent driving
	if ( !IsEngineOn() || m_bEngineLocked )
		return;

	// If the player's entering/exiting the vehicle, prevent movement
	if ( m_bEnterAnimOn || m_bExitAnimOn )
		return;

	DriveVehicle( player, ucmd );
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from entering / exiting the vehicle
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputLock( inputdata_t &inputdata )
{
	m_bLocked = true;
}

//-----------------------------------------------------------------------------
// Purpose: Allow the player to enter / exit the vehicle
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputUnlock( inputdata_t &inputdata )
{
	m_bLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter the vehicle
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Only drivers are supported
	Assert( pEntity && pEntity->IsPlayer() );

	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	// Can't enter if we're upside-down
	if ( IsOverturned() )
		return false;

	// Prevent entering if the vehicle's locked, or if it's moving too fast.
	return ( !m_bLocked && (m_nSpeed <= m_flMinimumSpeedToEnterExit) );
}

//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to exit the vehicle
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, or if it's moving too fast.
	return ( !m_bEnterAnimOn && !m_bExitAnimOn && !m_bLocked && (m_nSpeed <= m_flMinimumSpeedToEnterExit) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputTurnOn( inputdata_t &inputdata )
{
	m_bEngineLocked = false;

	StartEngine();
	m_VehiclePhysics.SetDisableEngine( false );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::InputTurnOff( inputdata_t &inputdata )
{
	m_bEngineLocked = true;

	StopEngine();
	m_VehiclePhysics.SetDisableEngine( true );
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the engine is on.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::IsEngineOn( void )
{
	return m_VehiclePhysics.IsOn();
}

//-----------------------------------------------------------------------------
// Purpose: Turn on the engine, but only if we're allowed to
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::StartEngine( void )
{
	if ( m_bEngineLocked )
	{
		m_VehiclePhysics.SetHandbrake( true );
		return;
	}

	m_VehiclePhysics.TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::StopEngine( void )
{
	m_VehiclePhysics.TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: // The player takes damage if he hits something going fast enough
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{

//=============================================================================
#ifdef HL2_EPISODIC

	// Notify all children
	for ( int i = 0; i < m_hPhysicsChildren.Count(); i++ )
	{
		if ( m_hPhysicsChildren[i] == NULL )
			continue;

		m_hPhysicsChildren[i]->VPhysicsCollision( index, pEvent );
	}

#endif // HL2_EPISODIC
//=============================================================================

	// Don't care if we don't have a driver
	CBaseCombatCharacter *pDriver = GetDriver() ? GetDriver()->MyCombatCharacterPointer() : NULL;
	if ( !pDriver )
		return;

	// Make sure we don't keep hitting the same entity
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
		return;

	BaseClass::VPhysicsCollision( index, pEvent );

	// if this is a bone follower, promote to the owner entity
	if ( pHitEntity->GetOwnerEntity() && (pHitEntity->GetEffects() & EF_NODRAW) )
	{
		CBaseEntity *pOwner = pHitEntity->GetOwnerEntity();
		// no friendly bone follower damage
		// this allows strider legs to damage the player on impact but not d0g for example
		if ( pDriver->IRelationType( pOwner ) == D_LI )
			return;
	}

	// If we hit hard enough, damage the player
	// Don't take damage from ramming bad guys
	if ( pHitEntity->MyNPCPointer() )
	{
		return;
	}

	// Don't take damage from ramming ragdolls
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL )
		return;

	// Ignore func_breakables
	CBreakable *pBreakable = dynamic_cast<CBreakable *>(pHitEntity);
	if ( pBreakable )
	{
		// ROBIN: Do we want to only do this on func_breakables that are about to die?
		//if ( pBreakable->HasSpawnFlags( SF_PHYSICS_BREAK_IMMEDIATELY ) )
		return;
	}

	// Over our skill's minimum crash level?
	int damageType = 0;
	float flDamage = CalculatePhysicsImpactDamage( index, pEvent, gDefaultPlayerVehicleImpactDamageTable, 1.0, true, damageType );
	if ( flDamage > 0 && m_flNoImpactDamageTime < gpGlobals->curtime )
	{
		Vector damagePos;
		pEvent->pInternalData->GetContactPoint( damagePos );
		Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
		CTakeDamageInfo info( this, GetDriver(), damageForce, damagePos, flDamage, (damageType|DMG_VEHICLE) );
		GetDriver()->TakeDamage( info );
	}
}

int CPropVehicleDriveable::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	return GetPhysics()->VPhysicsGetObjectList( pList, listMax );
}

//-----------------------------------------------------------------------------
// Purpose: Handle trace attacks from the physcannon
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// If we've just been zapped by the physcannon, try and right ourselves
	if ( info.GetDamageType() & DMG_PHYSGUN )
	{
		float flUprightStrength = GetUprightStrength();
		if ( flUprightStrength )
		{
			// Update our strength value if we already have an upright controller
			if ( m_hKeepUpright )
			{
				variant_t limitVariant;
				limitVariant.SetFloat( flUprightStrength );
				m_hKeepUpright->AcceptInput( "SetAngularLimit", this, this, limitVariant, USE_TOGGLE );
			}
			else
			{
				// If we don't have one, create an upright controller for us
				m_hKeepUpright = CreateKeepUpright( GetAbsOrigin(), vec3_angle, this, GetUprightStrength(), false );
			}

			Assert( m_hKeepUpright );
			variant_t emptyVariant;
			m_hKeepUpright->AcceptInput( "TurnOn", this, this, emptyVariant, USE_TOGGLE );

			// Turn off the keepupright after a short time
			m_flTurnOffKeepUpright = gpGlobals->curtime + GetUprightTime();
			SetNextThink( gpGlobals->curtime );
		}

#ifdef HL2_EPISODIC
		// Notify all children
		for ( int i = 0; i < m_hPhysicsChildren.Count(); i++ )
		{
			if ( m_hPhysicsChildren[i] == NULL )
				continue;

			variant_t emptyVariant;
			m_hPhysicsChildren[i]->AcceptInput( "VehiclePunted", info.GetAttacker(), this, emptyVariant, USE_TOGGLE );
		}
#endif // HL2_EPISODIC

	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//=============================================================================
// Passenger carrier

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::NPC_CanEnterVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Always allowed unless a leaf class says otherwise
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::NPC_CanExitVehicle( CAI_BaseNPC *pPassenger, bool bCompanion )
{
	// Always allowed unless a leaf class says otherwise
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::NPC_AddPassenger( CAI_BaseNPC *pPassenger, string_t strRoleName, int nSeatID )
{
	// Must be allowed to enter
	if ( NPC_CanEnterVehicle( pPassenger, true /*FIXME*/ ) == false )
		return false;

	IServerVehicle *pVehicleServer = GetServerVehicle();
	if ( pVehicleServer != NULL )
		return pVehicleServer->NPC_AddPassenger( pPassenger, strRoleName, nSeatID );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPassenger - 
//			bCompanion - 
//-----------------------------------------------------------------------------
bool CPropVehicleDriveable::NPC_RemovePassenger( CAI_BaseNPC *pPassenger )
{
	// Must be allowed to exit
	if ( NPC_CanExitVehicle( pPassenger, true /*FIXME*/ ) == false )
		return false;

	IServerVehicle *pVehicleServer = GetServerVehicle();
	if ( pVehicleServer != NULL )
		return pVehicleServer->NPC_RemovePassenger( pPassenger );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//			&info - 
//-----------------------------------------------------------------------------
void CPropVehicleDriveable::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{ 
	CBaseEntity *pDriver = GetDriver();
	if ( pDriver != NULL )
	{
		pDriver->Event_KilledOther( pVictim, info );
	}

	BaseClass::Event_KilledOther( pVictim, info );
}

//========================================================================================================================================
// FOUR WHEEL PHYSICS VEHICLE SERVER VEHICLE
//========================================================================================================================================
CFourWheelServerVehicle::CFourWheelServerVehicle( void )
{
	// Setup our smoothing data
	memset( &m_ViewSmoothing, 0, sizeof( m_ViewSmoothing ) );

	m_ViewSmoothing.bClampEyeAngles		= true;
	m_ViewSmoothing.bDampenEyePosition	= true;
	m_ViewSmoothing.flPitchCurveZero	= PITCH_CURVE_ZERO;
	m_ViewSmoothing.flPitchCurveLinear	= PITCH_CURVE_LINEAR;
	m_ViewSmoothing.flRollCurveZero		= ROLL_CURVE_ZERO;
	m_ViewSmoothing.flRollCurveLinear	= ROLL_CURVE_LINEAR;
}

#ifdef HL2_EPISODIC
ConVar r_JeepFOV( "r_JeepFOV", "82", FCVAR_CHEAT | FCVAR_REPLICATED );
#else
ConVar r_JeepFOV( "r_JeepFOV", "90", FCVAR_CHEAT | FCVAR_REPLICATED );
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: Setup our view smoothing information
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::InitViewSmoothing( const Vector &vecOrigin, const QAngle &vecAngles )
{
	m_ViewSmoothing.bWasRunningAnim = false;
	m_ViewSmoothing.vecOriginSaved = vecOrigin;
	m_ViewSmoothing.vecAnglesSaved = vecAngles;
	m_ViewSmoothing.flFOV = r_JeepFOV.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::SetVehicle( CBaseEntity *pVehicle )
{
	ASSERT( dynamic_cast<CPropVehicleDriveable*>(pVehicle) );
	BaseClass::SetVehicle( pVehicle );
	
	// Save this for view smoothing
	if ( pVehicle != NULL )
	{
		m_ViewSmoothing.pVehicle = pVehicle->GetBaseAnimating();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*= NULL*/ )
{
	CBaseEntity *pDriver = GetPassenger( nRole );
	if ( pDriver && pDriver->IsPlayer())
	{
		CBasePlayer *pPlayerDriver = ToBasePlayer( pDriver );
		CPropVehicleDriveable *pVehicle = GetFourWheelVehicle();
		SharedVehicleViewSmoothing( pPlayerDriver,
									pAbsOrigin, pAbsAngles,
									pVehicle->IsEnterAnimOn(), pVehicle->IsExitAnimOn(),
									pVehicle->GetEyeExitEndpoint(), 
									&m_ViewSmoothing,
									pFOV );
	}
	else
	{
		// NPCs are not supported
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const vehicleparams_t *CFourWheelServerVehicle::GetVehicleParams( void )
{ 
	return &GetFourWheelVehiclePhysics()->GetVehicleParams(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const vehicle_operatingparams_t	*CFourWheelServerVehicle::GetVehicleOperatingParams( void )
{
	return &GetFourWheelVehiclePhysics()->GetVehicleOperatingParams();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const vehicle_controlparams_t *CFourWheelServerVehicle::GetVehicleControlParams( void )
{
	return &GetFourWheelVehiclePhysics()->GetVehicleControls();
}

IPhysicsVehicleController *CFourWheelServerVehicle::GetVehicleController()
{
	return GetFourWheelVehiclePhysics()->GetVehicleController();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPropVehicleDriveable *CFourWheelServerVehicle::GetFourWheelVehicle( void )
{
	return (CPropVehicleDriveable *)m_pVehicle;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFourWheelVehiclePhysics *CFourWheelServerVehicle::GetFourWheelVehiclePhysics( void )
{
	CPropVehicleDriveable *pVehicle = GetFourWheelVehicle();
	return pVehicle->GetPhysics();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFourWheelServerVehicle::IsVehicleUpright( void )
{ 
	return (GetFourWheelVehicle()->IsOverturned() == false); 
}

bool CFourWheelServerVehicle::IsVehicleBodyInWater() 
{ 
	return GetFourWheelVehicle()->IsVehicleBodyInWater(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFourWheelServerVehicle::IsPassengerEntering( void )
{
	return GetFourWheelVehicle()->IsEnterAnimOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFourWheelServerVehicle::IsPassengerExiting( void )
{
	return GetFourWheelVehicle()->IsExitAnimOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::NPC_SetDriver( CNPC_VehicleDriver *pDriver )
{
	if ( pDriver )
	{
		m_nNPCButtons = 0;
		GetFourWheelVehicle()->m_hNPCDriver = pDriver;
		GetFourWheelVehicle()->StartEngine();
		SetVehicleVolume( 1.0 );	// Vehicles driven by NPCs are louder

		// Set our owner entity to be the NPC, so it can path check without hitting us
		GetFourWheelVehicle()->SetOwnerEntity( pDriver );

		// Start Thinking
		GetFourWheelVehicle()->SetNextThink( gpGlobals->curtime );
	}
	else
	{
		GetFourWheelVehicle()->m_hNPCDriver = NULL;
		GetFourWheelVehicle()->StopEngine();
		GetFourWheelVehicle()->SetOwnerEntity( NULL );
		SetVehicleVolume( 0.5 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFourWheelServerVehicle::NPC_DriveVehicle( void )
{

#ifdef HL2_DLL
	if ( g_debug_vehicledriver.GetInt() )
	{
		if ( m_nNPCButtons )
		{
			Vector vecForward, vecRight;
			GetFourWheelVehicle()->GetVectors( &vecForward, &vecRight, NULL );
			if ( m_nNPCButtons & IN_FORWARD )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() + vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_BACK )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() - vecForward * 200, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVELEFT )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() - vecRight * 200 * -m_flTurnDegrees, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_MOVERIGHT )
			{
				NDebugOverlay::Line( GetFourWheelVehicle()->GetAbsOrigin(), GetFourWheelVehicle()->GetAbsOrigin() + vecRight * 200 * m_flTurnDegrees, 0,255,0, true, 0.1 );
			}
			if ( m_nNPCButtons & IN_JUMP )
			{
				NDebugOverlay::Box( GetFourWheelVehicle()->GetAbsOrigin(), -Vector(20,20,20), Vector(20,20,20), 0,255,0, true, 0.1 );
			}
		}
	}
#endif

	int buttonsChanged = m_nPrevNPCButtons ^ m_nNPCButtons;
	int afButtonPressed = buttonsChanged & m_nNPCButtons;		// The changed ones still down are "pressed"
	int afButtonReleased = buttonsChanged & (~m_nNPCButtons);	// The ones not down are "released"
	CUserCmd fakeCmd;
	fakeCmd.Reset();
	fakeCmd.buttons = m_nNPCButtons;
	fakeCmd.forwardmove += 200.0f * ( m_nNPCButtons & IN_FORWARD );
	fakeCmd.forwardmove -= 200.0f * ( m_nNPCButtons & IN_BACK );
	fakeCmd.sidemove -= 200.0f * ( m_nNPCButtons & IN_MOVELEFT );
	fakeCmd.sidemove += 200.0f * ( m_nNPCButtons & IN_MOVERIGHT );

	GetFourWheelVehicle()->DriveVehicle( gpGlobals->frametime, &fakeCmd, afButtonPressed, afButtonReleased );
	m_nPrevNPCButtons = m_nNPCButtons;

	// NPC's cheat by using analog steering.
	GetFourWheelVehiclePhysics()->SetSteering( m_flTurnDegrees, 0 );

	// Clear out attack buttons each frame
	m_nNPCButtons &= ~IN_ATTACK;
	m_nNPCButtons &= ~IN_ATTACK2;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nWheelIndex - 
//			&vecPos - 
//-----------------------------------------------------------------------------
bool CFourWheelServerVehicle::GetWheelContactPoint( int nWheelIndex, Vector &vecPos )
{
	// Dig through a couple layers to get to our data
	CFourWheelVehiclePhysics  *pVehiclePhysics = GetFourWheelVehiclePhysics();
	if ( pVehiclePhysics )
	{
		IPhysicsVehicleController *pVehicleController = pVehiclePhysics->GetVehicle();
		if ( pVehicleController )
		{
			return pVehicleController->GetWheelContactPoint( nWheelIndex, &vecPos, NULL );
		}
	}
	return false;
}
