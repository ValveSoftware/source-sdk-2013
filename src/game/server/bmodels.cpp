//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Spawn, think, and use functions for common brush entities.
//
//=============================================================================//

#include "cbase.h"
#include "doors.h"
#include "mathlib/mathlib.h"
#include "physics.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include "globals.h"
#include "filters.h"

/// XXX(JohnS): This was never fully implemented, and thus func_rotating was broken in TF2 from 2007 - 2018.  Bandwidth
//              it intended to save is less significant now, but ideally it could be seamlessly clientside (with work -
//              c_funcrotating is a stub that does nothing).  As few official maps use it at all, favoring it working in
//              TF2 at all over small bandwidth savings.
// #define CFUNCROTATING_CLIENTSIDE_SUPPORT

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define		SF_BRUSH_ACCDCC	16// brush should accelerate and decelerate when toggled
#define		SF_BRUSH_HURT		32// rotating brush that inflicts pain based on rotation speed
#define		SF_ROTATING_NOT_SOLID	64	// some special rotating objects are not solid.

// =================== FUNC_WALL ==============================================
class CFuncWall : public CBaseEntity
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncWall, CBaseEntity );
	void	Spawn( void );
	bool	CreateVPhysics( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int		m_nState;
};

LINK_ENTITY_TO_CLASS( func_wall, CFuncWall );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CFuncWall )

	DEFINE_FIELD( m_nState,	FIELD_INTEGER ),

END_DATADESC()

void CFuncWall::Spawn( void )
{
	SetLocalAngles( vec3_angle );
	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
	SetModel( STRING( GetModelName() ) );
	
	// If it can't move/go away, it's really part of the world
	AddFlag( FL_WORLDBRUSH );

	// set manual mode
	CreateVPhysics();
}


bool CFuncWall::CreateVPhysics( void )
{
	SetSolid( SOLID_BSP );
	IPhysicsObject *pPhys = VPhysicsInitStatic();
	if ( pPhys )
	{
		int contents = modelinfo->GetModelContents( GetModelIndex() );
		if ( ! (contents & (MASK_SOLID|MASK_PLAYERSOLID|MASK_NPCSOLID)) )
		{
			// leave the physics shadow there in case it has stuff constrained to it
			// but disable collisions with it
			pPhys->EnableCollisions( false );
		}
	}

	return true;
}


void CFuncWall::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, m_nState ) )
	{
		m_nState = 1 - m_nState;
	}
}


#define SF_WALL_START_OFF		0x0001

class CFuncWallToggle : public CFuncWall
{
public:
	DECLARE_CLASS( CFuncWallToggle, CFuncWall );

	DECLARE_DATADESC();

	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void	InputToggle( inputdata_t &inputdata );

	void	TurnOff( void );
	void	TurnOn( void );
	bool	IsOn( void );
};

BEGIN_DATADESC( CFuncWallToggle )

	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_wall_toggle, CFuncWallToggle );

void CFuncWallToggle::Spawn( void )
{
	BaseClass::Spawn();
	if ( HasSpawnFlags( SF_WALL_START_OFF ) )
		TurnOff();
	
	SetMoveType( MOVETYPE_PUSH );
}


void CFuncWallToggle::TurnOff( void )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( false );
	}
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}


void CFuncWallToggle::TurnOn( void )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( true );
	}
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	RemoveEffects( EF_NODRAW );
}


bool CFuncWallToggle::IsOn( void )
{
	if ( IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return false;
	return true;
}

void CFuncWallToggle::InputToggle( inputdata_t &inputdata )
{
	int status = IsOn();

	if ( ShouldToggle( USE_TOGGLE, status ) )
	{
		if ( status )
			TurnOff();
		else
			TurnOn();
	}
}

//Adrian - Is this function needed at all?
void CFuncWallToggle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int status = IsOn();

	if ( ShouldToggle( useType, status ) )
	{
		if ( status )
			TurnOff();
		else
			TurnOn();
	}
}

//============================== FUNC_VEHICLECLIP =====================================
class CFuncVehicleClip : public CBaseEntity
{
public:
	DECLARE_CLASS( CFuncVehicleClip, CBaseEntity );
	DECLARE_DATADESC();

	void Spawn();
	bool CreateVPhysics( void );

	void InputEnable( inputdata_t &data );
	void InputDisable( inputdata_t &data );

private:
};

BEGIN_DATADESC( CFuncVehicleClip )

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_vehicleclip, CFuncVehicleClip );

void CFuncVehicleClip::Spawn()
{

	SetLocalAngles( vec3_angle );
	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
	SetModel( STRING( GetModelName() ) );
	
	// It's part of the world
	AddFlag( FL_WORLDBRUSH );

	CreateVPhysics();

	AddEffects( EF_NODRAW );		// make entity invisible

	SetCollisionGroup( COLLISION_GROUP_VEHICLE_CLIP );
}

bool CFuncVehicleClip::CreateVPhysics( void )
{
	SetSolid( SOLID_BSP );
	VPhysicsInitStatic();

	return true;
}

void CFuncVehicleClip::InputEnable( inputdata_t &data )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( true );
	}
	RemoveSolidFlags( FSOLID_NOT_SOLID );
}

void CFuncVehicleClip::InputDisable( inputdata_t &data )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		pPhys->EnableCollisions( false );
	}
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//============================= FUNC_CONVEYOR =======================================

#define SF_CONVEYOR_VISUAL		0x0001
#define SF_CONVEYOR_NOTSOLID	0x0002

class CFuncConveyor : public CFuncWall
{
public:
	DECLARE_CLASS( CFuncConveyor, CFuncWall );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CFuncConveyor();

	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	UpdateSpeed( float flNewSpeed );

	void	GetGroundVelocityToApply( Vector &vecGroundVel );

	// Input handlers.
	void	InputToggleDirection( inputdata_t &inputdata );
	void	InputSetSpeed( inputdata_t &inputdata );

private:

	Vector m_vecMoveDir;
	CNetworkVar( float, m_flConveyorSpeed );
};

LINK_ENTITY_TO_CLASS( func_conveyor, CFuncConveyor );

BEGIN_DATADESC( CFuncConveyor )

	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleDirection", InputToggleDirection ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetSpeed", InputSetSpeed ),

	DEFINE_KEYFIELD( m_vecMoveDir, FIELD_VECTOR, "movedir" ),
	DEFINE_FIELD( m_flConveyorSpeed, FIELD_FLOAT ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CFuncConveyor, DT_FuncConveyor)
	SendPropFloat( SENDINFO(m_flConveyorSpeed), 0, SPROP_NOSCALE ),
END_SEND_TABLE()


CFuncConveyor::CFuncConveyor()
{
	m_flConveyorSpeed = 0.0;
}

void CFuncConveyor::Spawn( void )
{
	// Convert movedir from angles to a vector
	QAngle angMoveDir = QAngle( m_vecMoveDir.x, m_vecMoveDir.y, m_vecMoveDir.z );
	AngleVectors( angMoveDir, &m_vecMoveDir );

	BaseClass::Spawn();

	if ( !HasSpawnFlags(SF_CONVEYOR_VISUAL) )
		AddFlag( FL_CONVEYOR );

	// HACKHACK - This is to allow for some special effects
	if ( HasSpawnFlags( SF_CONVEYOR_NOTSOLID ) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	if ( m_flSpeed == 0 )
		m_flSpeed = 100;

	UpdateSpeed( m_flSpeed );
}


void CFuncConveyor::UpdateSpeed( float flNewSpeed )
{
	m_flConveyorSpeed = flNewSpeed;
}


void CFuncConveyor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    m_flSpeed = -m_flSpeed;
	UpdateSpeed( m_flSpeed );
}


void CFuncConveyor::InputToggleDirection( inputdata_t &inputdata )
{
	Use( inputdata.pActivator, inputdata.pCaller, USE_TOGGLE, 0 );
}


void CFuncConveyor::InputSetSpeed( inputdata_t &inputdata )
{
    m_flSpeed = inputdata.value.Float();
	UpdateSpeed( m_flSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Returns the velocity imparted to players standing on us.
//-----------------------------------------------------------------------------
void CFuncConveyor::GetGroundVelocityToApply( Vector &vecGroundVel )
{
	vecGroundVel = m_vecMoveDir * m_flSpeed;
}


// =================== FUNC_ILLUSIONARY ==============================================
// A simple entity that looks solid but lets you walk through it.
class CFuncIllusionary : public CBaseEntity 
{
	DECLARE_CLASS( CFuncIllusionary, CBaseEntity );
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( func_illusionary, CFuncIllusionary );

void CFuncIllusionary::Spawn( void )
{
	SetLocalAngles( vec3_angle );
	SetMoveType( MOVETYPE_NONE );  
	SetSolid( SOLID_NONE );
	SetModel( STRING( GetModelName() ) );
}


//-----------------------------------------------------------------------------
// Purpose: A rotating brush entity.
//
//			You need to have an origin brush as part of this entity.  The  
//			center of that brush will be the point around which it is rotated.
//
//			It will rotate around the Z axis by default. Spawnflags can be set
//			to make it rotate around the X or Y axes.
//
//			The direction of rotation is also controlled by a spawnflag.
//-----------------------------------------------------------------------------
class CFuncRotating : public CBaseEntity
{
	DECLARE_CLASS( CFuncRotating, CBaseEntity );
public:
	// basic functions
	void Spawn( void  );
	void Precache( void  );
	bool CreateVPhysics( void );
	void SpinUpMove( void );
	void SpinDownMove( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void HurtTouch ( CBaseEntity *pOther );
	void RotatingUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void RotateMove( void );
	void ReverseMove( void );
	void RampPitchVol( void );
	void Blocked( CBaseEntity *pOther );
	void SetTargetSpeed( float flSpeed );
	void UpdateSpeed( float flNewSpeed );
	
	int	 DrawDebugTextOverlays(void);

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

protected:
	bool SpinDown( float flTargetSpeed );
	float GetMoveSpeed( float flSpeed );

	float GetNextMoveInterval() const;

	// Input handlers
	void InputSetSpeed( inputdata_t &inputdata );
	void InputStart( inputdata_t &inputdata );
	void InputStop( inputdata_t &inputdata );
	void InputStartForward( inputdata_t &inputdata );
	void InputStartBackward( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );
	void InputReverse( inputdata_t &inputdata );
	void InputStopAtStartPos( inputdata_t &inputdata );

	QAngle	m_vecMoveAng;

	float m_flFanFriction;
	float m_flAttenuation;
	float m_flVolume;
	float m_flTargetSpeed;			// Target value for m_flSpeed, used for spinning up and down.
	float m_flMaxSpeed;				// Maximum value for m_flSpeed, used for ramping sound effects.
	float m_flBlockDamage;			// Damage inflicted when blocked.
	string_t m_NoiseRunning;
	bool m_bReversed;

	QAngle	m_angStart;
	bool m_bStopAtStartPos;

	bool m_bSolidBsp;				// Brush is SOLID_BSP

public:
	Vector m_vecClientOrigin;
	QAngle m_vecClientAngles;
};

LINK_ENTITY_TO_CLASS( func_rotating, CFuncRotating );


BEGIN_DATADESC( CFuncRotating )

	DEFINE_FIELD( m_vecMoveAng, FIELD_VECTOR ),
	DEFINE_FIELD( m_flFanFriction, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAttenuation, FIELD_FLOAT ),
	DEFINE_FIELD( m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( m_flTargetSpeed, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flMaxSpeed, FIELD_FLOAT, "maxspeed" ),
	DEFINE_KEYFIELD( m_flBlockDamage, FIELD_FLOAT, "dmg" ),
	DEFINE_KEYFIELD( m_NoiseRunning, FIELD_SOUNDNAME, "message" ),
	DEFINE_FIELD( m_bReversed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_angStart, FIELD_VECTOR ),
	DEFINE_FIELD( m_bStopAtStartPos, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_bSolidBsp, FIELD_BOOLEAN, "solidbsp" ),

	// Function Pointers
	DEFINE_FUNCTION( SpinUpMove ),
	DEFINE_FUNCTION( SpinDownMove ),
	DEFINE_FUNCTION( HurtTouch ),
	DEFINE_FUNCTION( RotatingUse ),
	DEFINE_FUNCTION( RotateMove ),
	DEFINE_FUNCTION( ReverseMove ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStart ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Reverse", InputReverse ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartForward", InputStartForward ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartBackward", InputStartBackward ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopAtStartPos", InputStopAtStartPos ),

END_DATADESC()

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_FuncRotatingOrigin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
#ifdef CFUNCROTATING_CLIENTSIDE_SUPPORT
	CFuncRotating *entity = (CFuncRotating*)pStruct;
	Assert( entity );

	if ( entity->HasSpawnFlags(SF_BRUSH_ROTATE_CLIENTSIDE) )
	{
		const Vector *v = &entity->m_vecClientOrigin;
		pOut->m_Vector[ 0 ] = v->x;
		pOut->m_Vector[ 1 ] = v->y;
		pOut->m_Vector[ 2 ] = v->z;
		return;
	}
#endif

	SendProxy_Origin( pProp, pStruct, pData, pOut, iElement, objectID );
}

/*
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_FuncRotatingAngles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CFuncRotating *entity = (CFuncRotating*)pStruct;
	Assert( entity );
	if ( entity->HasSpawnFlags(SF_BRUSH_ROTATE_CLIENTSIDE) )
	{
		const QAngle *a = &entity->m_vecClientAngles;
		pOut->m_Vector[ 0 ] = anglemod( a->x );
		pOut->m_Vector[ 1 ] = anglemod( a->y );
		pOut->m_Vector[ 2 ] = anglemod( a->z );
		return;
	}

	SendProxy_Angles( pProp, pStruct, pData, pOut, iElement, objectID );
}
*/
void SendProxy_FuncRotatingAngle( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID)
{
	CFuncRotating *entity = (CFuncRotating*)pStruct;
	Assert( entity );

	vec_t const *qa = (vec_t *)pData;
	vec_t const *ea = entity->GetLocalAngles().Base();
	NOTE_UNUSED(ea);
	// Assert its actually an index into m_angRotation if not this won't work

	Assert( (uintp)qa >= (uintp)ea && (uintp)qa < (uintp)ea + sizeof( QAngle ));

#ifdef CFUNCROTATING_CLIENTSIDE_SUPPORT
	if ( entity->HasSpawnFlags(SF_BRUSH_ROTATE_CLIENTSIDE) )
	{
		const QAngle *a = &entity->m_vecClientAngles;

		pOut->m_Float = anglemod( (*a)[ qa - ea ] );
		return;
	}
#endif

	pOut->m_Float = anglemod( *qa );

	Assert( IsFinite( pOut->m_Float ) );
}


extern void SendProxy_SimulationTime( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
void SendProxy_FuncRotatingSimulationTime( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
#ifdef CFUNCROTATING_CLIENTSIDE_SUPPORT
	CFuncRotating *entity = (CFuncRotating*)pStruct;
	Assert( entity );

	if ( entity->HasSpawnFlags(SF_BRUSH_ROTATE_CLIENTSIDE) )
	{
		pOut->m_Int = 0;
		return;
	}
#endif

	SendProxy_SimulationTime( pProp, pStruct, pVarData, pOut, iElement, objectID );
}

IMPLEMENT_SERVERCLASS_ST(CFuncRotating, DT_FuncRotating)
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_flSimulationTime" ),

	SendPropVector(SENDINFO(m_vecOrigin), -1,  SPROP_COORD|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_FuncRotatingOrigin ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 13, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 13, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 13, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),

	SendPropInt(SENDINFO(m_flSimulationTime), SIMULATION_TIME_WINDOW_BITS, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN|SPROP_ENCODED_AGAINST_TICKCOUNT, SendProxy_FuncRotatingSimulationTime),
END_SEND_TABLE()



//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues from the BSP. Called before spawning.
//-----------------------------------------------------------------------------
bool CFuncRotating::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "fanfriction"))
	{
		m_flFanFriction = atof(szValue)/100;
	}
	else if (FStrEq(szKeyName, "Volume"))
	{
		m_flVolume = atof(szValue) / 10.0;
		m_flVolume = clamp(m_flVolume, 0.0f, 1.0f);
	}
	else
	{ 
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been set.
//-----------------------------------------------------------------------------
void CFuncRotating::Spawn( )
{
#if defined( CFUNCROTATING_CLIENTSIDE_SUPPORT )
	AddSpawnFlags( SF_BRUSH_ROTATE_CLIENTSIDE );
#endif

	//
	// Maintain compatibility with previous maps.
	//
	if (m_flVolume == 0.0)
	{
		m_flVolume = 1.0;
	}

	//
	// If the designer didn't set a sound attenuation, default to one.
	//
	if ( HasSpawnFlags(SF_BRUSH_ROTATE_SMALLRADIUS) )
	{
		m_flAttenuation = ATTN_IDLE;
	}
	else if ( HasSpawnFlags(SF_BRUSH_ROTATE_MEDIUMRADIUS) )
	{
		m_flAttenuation = ATTN_STATIC;
	}
	else if ( HasSpawnFlags(SF_BRUSH_ROTATE_LARGERADIUS) )
	{
		m_flAttenuation = ATTN_NORM;
	}
	else
	{
		m_flAttenuation = ATTN_NORM;
	}

	//
	// Prevent divide by zero if level designer forgets friction!
	//
	if ( m_flFanFriction == 0 )
	{
		m_flFanFriction = 1;
	}
	
	//
	// Build the axis of rotation based on spawnflags.
	//
	if ( HasSpawnFlags(SF_BRUSH_ROTATE_Z_AXIS) )
	{
		m_vecMoveAng = QAngle(0,0,1);
	}
	else if ( HasSpawnFlags(SF_BRUSH_ROTATE_X_AXIS) )
	{
		m_vecMoveAng = QAngle(1,0,0);
	}
	else
	{
		m_vecMoveAng = QAngle(0,1,0);	// y-axis
	}

	//
	// Check for reverse rotation.
	//
	if ( HasSpawnFlags(SF_BRUSH_ROTATE_BACKWARDS) )
	{
		m_vecMoveAng = m_vecMoveAng * -1;
	}

	SetSolid( SOLID_VPHYSICS );

	//
	// Some rotating objects like fake volumetric lights will not be solid.
	//
	if ( HasSpawnFlags(SF_ROTATING_NOT_SOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_PUSH );
	}
	else
	{
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		SetMoveType( MOVETYPE_PUSH );
	}

	SetModel( STRING( GetModelName() ) );

	SetUse( &CFuncRotating::RotatingUse );

	//
	// Did level designer forget to assign a maximum speed? Prevent a divide by
	// zero in RampPitchVol as well as allowing the rotator to work.
	//
	m_flMaxSpeed = fabs( m_flMaxSpeed );
	if (m_flMaxSpeed == 0)
	{
		m_flMaxSpeed = 100;
	}

	//
	// If the brush should be initially rotating, use it in a little while.
	//
	if ( HasSpawnFlags(SF_BRUSH_ROTATE_START_ON) )
	{		
		SetThink( &CFuncRotating::SUB_CallUseToggle );
		SetNextThink( gpGlobals->curtime + .2 );	// leave a magic delay for client to start up
	}

	//
	// Can this brush inflict pain?
	//
	if ( HasSpawnFlags(SF_BRUSH_HURT) )
	{
		SetTouch( &CFuncRotating::HurtTouch );
	}

	//
	// Set speed to 0 in case there's an old "speed" key lying around.
	//
	m_flSpeed = 0;
	
	Precache( );
	CreateVPhysics();

	m_angStart = GetLocalAngles();
	
	// Slam the object back to solid - if we really want it to be solid.
	if ( m_bSolidBsp )
	{
		SetSolid( SOLID_BSP );
	}

#ifdef CFUNCROTATING_CLIENTSIDE_SUPPORT
	if ( HasSpawnFlags(SF_BRUSH_ROTATE_CLIENTSIDE) )
	{
		m_vecClientOrigin = GetLocalOrigin();
		m_vecClientAngles = GetLocalAngles();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncRotating::CreateVPhysics( void )
{
	if ( !IsSolidFlagSet( FSOLID_NOT_SOLID ))
	{
		VPhysicsInitShadow( false, false );
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncRotating::Precache( void )
{
	//
	// Set up rotation sound.
	//
	char *szSoundFile = ( char * )STRING( m_NoiseRunning );
	if ( !m_NoiseRunning || strlen( szSoundFile ) == 0 )
	{
		// No sound set up, use the null sound.
		m_NoiseRunning = AllocPooledString("DoorSound.Null");
	}
	PrecacheScriptSound( STRING( m_NoiseRunning ) );
	
	if (GetLocalAngularVelocity() != vec3_angle )
	{
		//
		// If fan was spinning, and we went through transition or save/restore,
		// make sure we restart the sound.  1.5 sec delay is a magic number.
		//
		SetMoveDone( &CFuncRotating::SpinUpMove );
		SetMoveDoneTime( 1.5 );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Will hurt others based on how fast the brush is spinning.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CFuncRotating::HurtTouch ( CBaseEntity *pOther )
{
	// we can't hurt this thing, so we're not concerned with it
	if ( !pOther->m_takedamage )
		return;

	// calculate damage based on rotation speed
	m_flBlockDamage = GetLocalAngularVelocity().Length() / 10;

#ifdef HL1_DLL
	if( m_flBlockDamage > 0 )
#endif
	{
		pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
	
		Vector vecNewVelocity = pOther->GetAbsOrigin() - WorldSpaceCenter();
		VectorNormalize(vecNewVelocity);
		vecNewVelocity *= m_flBlockDamage;
		pOther->SetAbsVelocity( vecNewVelocity );
	}
}


#define FANPITCHMIN		30
#define FANPITCHMAX		100


//-----------------------------------------------------------------------------
// Purpose: Ramp pitch and volume up to maximum values, based on the difference
//			between how fast we're going vs how fast we can go.
//-----------------------------------------------------------------------------
void CFuncRotating::RampPitchVol( void )
{
	//
	// Calc volume and pitch as % of maximum vol and pitch.
	//
	float fpct = fabs(m_flSpeed) / m_flMaxSpeed;
	float fvol = clamp(m_flVolume * fpct, 0.f, 1.f);			  // slowdown volume ramps down to 0

	float fpitch = FANPITCHMIN + (FANPITCHMAX - FANPITCHMIN) * fpct;	
	
	int pitch = clamp(FastFloatToSmallInt(fpitch), 0, 255);
	if (pitch == PITCH_NORM)
	{
		pitch = PITCH_NORM - 1;
	}

	//
	// Update the fan's volume and pitch.
	//
	CPASAttenuationFilter filter( GetAbsOrigin(), m_flAttenuation );
	filter.MakeReliable();

	EmitSound_t ep;
	ep.m_nChannel = CHAN_STATIC;
	ep.m_pSoundName = STRING(m_NoiseRunning);
	ep.m_flVolume = fvol;
	ep.m_SoundLevel = ATTN_TO_SNDLVL( m_flAttenuation );
	ep.m_nFlags = SND_CHANGE_PITCH | SND_CHANGE_VOL;
	ep.m_nPitch = pitch;

	EmitSound( filter, entindex(), ep );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CFuncRotating::GetNextMoveInterval() const
{
	if ( m_bStopAtStartPos )
	{
		return TICK_INTERVAL;
	}
	return 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current speed to the given value and manages the sound effects.
// Input  : flNewSpeed - New speed in degrees per second.
//-----------------------------------------------------------------------------
void CFuncRotating::UpdateSpeed( float flNewSpeed )
{
	float flOldSpeed = m_flSpeed;
	m_flSpeed = clamp( flNewSpeed, -m_flMaxSpeed, m_flMaxSpeed );

	if ( m_bStopAtStartPos )
	{
		int checkAxis = 2;
		// See if we got close to the starting orientation
		if ( m_vecMoveAng[0] != 0 )
		{
			checkAxis = 0;
		}
		else if ( m_vecMoveAng[1] != 0 )
		{
			checkAxis = 1;
		}

		float angDelta = anglemod( GetLocalAngles()[ checkAxis ] - m_angStart[ checkAxis ] );
		if ( angDelta > 180.0f )
		{
			angDelta -= 360.0f;
		}

		if ( flNewSpeed < 100 )
		{
			if ( flNewSpeed <= 25 && fabs( angDelta ) < 1.0f )
			{
				m_flTargetSpeed = 0;
				m_bStopAtStartPos = false;
				m_flSpeed = 0.0f;

				SetLocalAngles( m_angStart );
			}
			else if ( fabs( angDelta ) > 90.0f )
			{
				// Keep rotating at same speed for now
				m_flSpeed = flOldSpeed;
			}
			else
			{
				float minSpeed =  fabs( angDelta );
				if ( minSpeed < 20 )
					minSpeed = 20;
	
				m_flSpeed = flOldSpeed > 0.0f ? minSpeed : -minSpeed;
			}
		}
	}


	if ( ( flOldSpeed == 0 ) && ( m_flSpeed != 0 ) )
	{
		// Starting to move - emit the sound.
		CPASAttenuationFilter filter( GetAbsOrigin(), m_flAttenuation );
		filter.MakeReliable();
	
		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = STRING(m_NoiseRunning);
		ep.m_flVolume = 0.01;
		ep.m_SoundLevel = ATTN_TO_SNDLVL( m_flAttenuation );
		ep.m_nPitch = FANPITCHMIN;

		EmitSound( filter, entindex(), ep );
		RampPitchVol();
	}
	else if ( ( flOldSpeed != 0 ) && ( m_flSpeed == 0 ) )
	{
		// Stopping - stop the sound.
		StopSound( entindex(), CHAN_STATIC, STRING(m_NoiseRunning) );
		
	}
	else
	{
		// Changing speed - adjust the pitch and volume.
		RampPitchVol();
	}

	SetLocalAngularVelocity( m_vecMoveAng * m_flSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Think function. Accelerates a func_rotating to a higher angular velocity.
//-----------------------------------------------------------------------------
void CFuncRotating::SpinUpMove( void )
{
	//
	// Calculate our new speed.
	//
	bool bSpinUpDone = false;
	float flNewSpeed = fabs( m_flSpeed ) + 0.2 * m_flMaxSpeed * m_flFanFriction;
	if ( fabs( flNewSpeed ) >=  fabs( m_flTargetSpeed ) )
	{
		// Reached our target speed.
		flNewSpeed = m_flTargetSpeed;
		bSpinUpDone = !m_bStopAtStartPos;
	}
	else if ( m_flTargetSpeed < 0 )
	{
		// Spinning up in reverse - negate the speed.
		flNewSpeed *= -1;
	}

	//
	// Apply the new speed, adjust sound pitch and volume.
	//
	UpdateSpeed( flNewSpeed );

	//
	// If we've met or exceeded target speed, stop spinning up.
	//
	if ( bSpinUpDone )
	{
		SetMoveDone( &CFuncRotating::RotateMove );
		RotateMove();
	} 

	SetMoveDoneTime( GetNextMoveInterval() );
}


//-----------------------------------------------------------------------------
// Purpose: Decelerates the rotator from a higher speed to a lower one.
// Input  : flTargetSpeed - Speed to spin down to.
// Output : Returns true if we reached the target speed, false otherwise.
//-----------------------------------------------------------------------------
bool CFuncRotating::SpinDown( float flTargetSpeed )
{
	//
	// Bleed off a little speed due to friction.
	//
	bool bSpinDownDone = false;
	float flNewSpeed = fabs( m_flSpeed ) - 0.1 * m_flMaxSpeed * m_flFanFriction;
	if ( flNewSpeed < 0 )
	{
		flNewSpeed = 0;
	}

	if ( fabs( flNewSpeed ) <= fabs( flTargetSpeed ) )
	{
		// Reached our target speed.
		flNewSpeed = flTargetSpeed;
		bSpinDownDone = !m_bStopAtStartPos;
	}
	else if ( m_flSpeed < 0 )
	{
		// Spinning down in reverse - negate the speed.
		flNewSpeed *= -1;
	}

	//
	// Apply the new speed, adjust sound pitch and volume.
	//
	UpdateSpeed( flNewSpeed );

	//
	// If we've met or exceeded target speed, stop spinning down.
	//
	return bSpinDownDone;
}


//-----------------------------------------------------------------------------
// Purpose: Think function. Decelerates a func_rotating to a lower angular velocity.
//-----------------------------------------------------------------------------
void CFuncRotating::SpinDownMove( void )
{
	//
	// If we've met or exceeded target speed, stop spinning down.
	//
	if ( SpinDown( m_flTargetSpeed ) )
	{
		SetMoveDone( &CFuncRotating::RotateMove );
		RotateMove();
	}
	else
	{
		SetMoveDoneTime( GetNextMoveInterval() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function for reversing directions. Spins down to zero, then
//			starts spinning up to the target speed.
//-----------------------------------------------------------------------------
void CFuncRotating::ReverseMove( void )
{
	if ( SpinDown( 0 ) )
	{
		// We've reached zero - spin back up to the target speed.
		SetTargetSpeed( m_flTargetSpeed );
	}
	else
	{
		SetMoveDoneTime( GetNextMoveInterval() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Think function. Called while rotating at a constant angular velocity.
//-----------------------------------------------------------------------------
void CFuncRotating::RotateMove( void )
{
	SetMoveDoneTime( 10 );

	if ( m_bStopAtStartPos )
	{
		SetMoveDoneTime( GetNextMoveInterval() );
		int checkAxis = 2;

		// See if we got close to the starting orientation
		if ( m_vecMoveAng[0] != 0 )
		{
			checkAxis = 0;
		}
		else if ( m_vecMoveAng[1] != 0 )
		{
			checkAxis = 1;
		}

		float angDelta = anglemod( GetLocalAngles()[ checkAxis ] - m_angStart[ checkAxis ] );
		if ( angDelta > 180.0f )
			angDelta -= 360.0f;

		QAngle avel = GetLocalAngularVelocity();
		// Delta per tick
		QAngle avelpertick = avel * TICK_INTERVAL;

		if ( fabs( angDelta ) < fabs( avelpertick[ checkAxis ] ) )
		{
			SetTargetSpeed( 0 );
			SetLocalAngles( m_angStart );
			m_bStopAtStartPos = false;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Used for debug output. Returns the given speed considering our current
//			direction of rotation, so that positive values are forward and negative
//			values are backward.
// Input  : flSpeed - Angular speed in degrees per second.
//-----------------------------------------------------------------------------
float CFuncRotating::GetMoveSpeed( float flSpeed )
{
	if ( m_vecMoveAng[0] != 0 )
	{
		return flSpeed * m_vecMoveAng[0];
	}

	if ( m_vecMoveAng[1] != 0 )
	{
		return flSpeed * m_vecMoveAng[1];
	}

	return flSpeed * m_vecMoveAng[2];
}


//-----------------------------------------------------------------------------
// Purpose: Sets a new angular velocity to achieve.
// Input  : flSpeed - Target angular velocity in degrees per second.
//-----------------------------------------------------------------------------
void CFuncRotating::SetTargetSpeed( float flSpeed )
{
	//
	// Make sure the sign is correct - positive for forward rotation,
	// negative for reverse rotation.
	//
	flSpeed = fabs( flSpeed );
	if ( m_bReversed )
	{
		flSpeed *= -1;
	}

	m_flTargetSpeed = flSpeed;

	//
	// If we don't accelerate, change to the new speed instantly.
	//
	if ( !HasSpawnFlags(SF_BRUSH_ACCDCC ) )
	{
		UpdateSpeed( m_flTargetSpeed );
		SetMoveDone( &CFuncRotating::RotateMove );
	}
	//
	// Otherwise deal with acceleration/deceleration:
	//
	else
	{
		//
		// Check for reversing directions.
		//
		if ((( m_flSpeed > 0 ) && ( m_flTargetSpeed < 0 )) || 
			(( m_flSpeed < 0 ) && ( m_flTargetSpeed > 0 )))
		{
			SetMoveDone( &CFuncRotating::ReverseMove );
		}
		//
		// If we are below the new target speed, spin up to the target speed.
		//
		else if ( fabs( m_flSpeed ) < fabs( m_flTargetSpeed ) )
		{
			SetMoveDone( &CFuncRotating::SpinUpMove );
		}
		//
		// If we are above the new target speed, spin down to the target speed.
		//
		else if ( fabs( m_flSpeed ) > fabs( m_flTargetSpeed ) )
		{
			SetMoveDone( &CFuncRotating::SpinDownMove );
		}
		//
		// We are already at the new target speed. Just keep rotating.
		//
		else
		{
			SetMoveDone( &CFuncRotating::RotateMove );
		}
	}

	SetMoveDoneTime( GetNextMoveInterval() );
}


//-----------------------------------------------------------------------------
// Purpose: Called when a rotating brush is used by the player.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CFuncRotating::RotatingUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//
	// If the rotator is spinning, stop it.
	//
	if ( m_flSpeed != 0 )
	{
		SetTargetSpeed( 0 );
	}
	//
	// Rotator is not moving, so start it.
	//
	else
	{
		SetTargetSpeed( m_flMaxSpeed );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that reverses the direction of rotation.
//-----------------------------------------------------------------------------
void CFuncRotating::InputReverse( inputdata_t &inputdata )
{
	m_bStopAtStartPos = false;
	m_bReversed = !m_bReversed;
	SetTargetSpeed( m_flSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the speed of the rotator.
// Input  : Float target angular velocity as a ratio of maximum speed [0, 1].
//-----------------------------------------------------------------------------
void CFuncRotating::InputSetSpeed( inputdata_t &inputdata )
{
	m_bStopAtStartPos = false;
	float flSpeed = inputdata.value.Float();
	m_bReversed = flSpeed < 0 ? true : false;
	flSpeed = fabs(flSpeed);
	SetTargetSpeed( clamp( flSpeed, 0.f, 1.f ) * m_flMaxSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to start the rotator spinning.
//-----------------------------------------------------------------------------
void CFuncRotating::InputStart( inputdata_t &inputdata )
{
	m_bStopAtStartPos = false;
	SetTargetSpeed( m_flMaxSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to start the rotator spinning.
//-----------------------------------------------------------------------------
void CFuncRotating::InputStartForward( inputdata_t &inputdata )
{
	m_bReversed = false;
	SetTargetSpeed( m_flMaxSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to start the rotator spinning.
//-----------------------------------------------------------------------------
void CFuncRotating::InputStartBackward( inputdata_t &inputdata )
{
	m_bStopAtStartPos = false;
	m_bReversed = true;
	SetTargetSpeed( m_flMaxSpeed );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to stop the rotator from spinning.
//-----------------------------------------------------------------------------
void CFuncRotating::InputStop( inputdata_t &inputdata )
{
	m_bStopAtStartPos = false;
	SetTargetSpeed( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncRotating::InputStopAtStartPos( inputdata_t &inputdata )
{
	m_bStopAtStartPos = true;
	SetTargetSpeed( 0 );
	SetMoveDoneTime( GetNextMoveInterval() );
}

//-----------------------------------------------------------------------------
// Purpose: Starts the rotator if it is still, stops it if it is spinning.
//-----------------------------------------------------------------------------
void CFuncRotating::InputToggle( inputdata_t &inputdata )
{	
	if (m_flSpeed > 0)
	{
		SetTargetSpeed( 0 );
	}
	else
	{
		SetTargetSpeed( m_flMaxSpeed );
	}
}


//-----------------------------------------------------------------------------
// Purpose: An entity has blocked the brush.
// Input  : pOther - 
//-----------------------------------------------------------------------------
void CFuncRotating::Blocked( CBaseEntity *pOther )
{
#ifdef HL1_DLL
	if( m_flBlockDamage > 0 )
#endif
		pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFuncRotating::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof( tempstr ),"Speed cur (target): %3.2f (%3.2f)", GetMoveSpeed( m_flSpeed ), GetMoveSpeed( m_flTargetSpeed ) );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;

}


class CFuncVPhysicsClip : public CBaseEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncVPhysicsClip, CBaseEntity );

public:
	void Spawn();
	void Activate();
	bool CreateVPhysics( void );

	bool EntityPassesFilter( CBaseEntity *pOther );
	bool ForceVPhysicsCollide( CBaseEntity *pEntity );

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

private:

	string_t						m_iFilterName;
	CHandle<CBaseFilter>			m_hFilter;
	bool							m_bDisabled;
};

// Global Savedata for base trigger
BEGIN_DATADESC( CFuncVPhysicsClip )

	// Keyfields
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD( m_hFilter,	FIELD_EHANDLE ),
	DEFINE_FIELD( m_bDisabled,	FIELD_BOOLEAN ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( func_clip_vphysics, CFuncVPhysicsClip );

void CFuncVPhysicsClip::Spawn( void )
{
	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
	SetSolid( SOLID_VPHYSICS );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	CreateVPhysics();
	VPhysicsGetObject()->EnableCollisions( !m_bDisabled );
}


bool CFuncVPhysicsClip::CreateVPhysics( void )
{
	VPhysicsInitStatic();
	return true;
}


void CFuncVPhysicsClip::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}
	BaseClass::Activate();
}

bool CFuncVPhysicsClip::EntityPassesFilter( CBaseEntity *pOther )
{
	CBaseFilter* pFilter = (CBaseFilter*)(m_hFilter.Get());

	if ( pFilter )
		return pFilter->PassesFilter( this, pOther );

	if ( pOther->GetMoveType() == MOVETYPE_VPHYSICS && pOther->VPhysicsGetObject()->IsMoveable() )
		return true;
	
	return false;
}


bool CFuncVPhysicsClip::ForceVPhysicsCollide( CBaseEntity *pEntity )
{
	return EntityPassesFilter(pEntity);
}

void CFuncVPhysicsClip::InputEnable( inputdata_t &inputdata )
{
	VPhysicsGetObject()->EnableCollisions(true);
	m_bDisabled = false;
}

void CFuncVPhysicsClip::InputDisable( inputdata_t &inputdata )
{
	VPhysicsGetObject()->EnableCollisions(false);
	m_bDisabled = true;
}
