//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a brush model entity that moves along a linear path.
//			Water whose level can be changed is implemented using the same entity.
//
//=============================================================================//

#include "cbase.h"
#include "func_movelinear.h"
#include "entitylist.h"
#include "locksounds.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include "physics_saverestore.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------
//  SPAWN_FLAGS
// -------------------------------
#define SF_MOVELINEAR_NOTSOLID		8

LINK_ENTITY_TO_CLASS( func_movelinear, CFuncMoveLinear );
LINK_ENTITY_TO_CLASS( momentary_door, CFuncMoveLinear );	// For backward compatibility

//
// func_water_analog is implemented as a linear mover so we can raise/lower the water level.
//
LINK_ENTITY_TO_CLASS( func_water_analog, CFuncMoveLinear );


BEGIN_DATADESC( CFuncMoveLinear )

	DEFINE_KEYFIELD( m_vecMoveDir,		 FIELD_VECTOR, "movedir" ),
	DEFINE_KEYFIELD( m_soundStart,		 FIELD_SOUNDNAME, "StartSound" ),
	DEFINE_KEYFIELD( m_soundStop,		 FIELD_SOUNDNAME, "StopSound" ),
	DEFINE_FIELD( m_currentSound, FIELD_SOUNDNAME ),
	DEFINE_KEYFIELD( m_flBlockDamage,	 FIELD_FLOAT,	"BlockDamage"),
	DEFINE_KEYFIELD( m_flStartPosition, FIELD_FLOAT,	"StartPosition"),
	DEFINE_KEYFIELD( m_flMoveDistance,  FIELD_FLOAT,	"MoveDistance"),
//	DEFINE_PHYSPTR( m_pFluidController ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,  "Open", InputOpen ),
	DEFINE_INPUTFUNC( FIELD_VOID,  "Close", InputClose ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPosition", InputSetPosition ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),

	// Outputs
	DEFINE_OUTPUT( m_OnFullyOpen, "OnFullyOpen" ),
	DEFINE_OUTPUT( m_OnFullyClosed, "OnFullyClosed" ),

	// Functions
	DEFINE_FUNCTION( StopMoveSound ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose: Called before spawning, after keyvalues have been parsed.
//------------------------------------------------------------------------------
void CFuncMoveLinear::Spawn( void )
{
	// Convert movedir from angles to a vector
	QAngle angMoveDir = QAngle( m_vecMoveDir.x, m_vecMoveDir.y, m_vecMoveDir.z );
	AngleVectors( angMoveDir, &m_vecMoveDir );

	SetMoveType( MOVETYPE_PUSH );
	SetModel( STRING( GetModelName() ) );
	
	// Don't allow zero or negative speeds
	if (m_flSpeed <= 0)
	{
		m_flSpeed = 100;
	}
	
	// If move distance is set to zero, use with width of the 
	// brush to determine the size of the move distance
	if (m_flMoveDistance <= 0)
	{
		Vector vecOBB = CollisionProp()->OBBSize();
		vecOBB -= Vector( 2, 2, 2 );
		m_flMoveDistance = DotProductAbs( m_vecMoveDir, vecOBB ) - m_flLip;
	}

	m_vecPosition1 = GetAbsOrigin() - (m_vecMoveDir * m_flMoveDistance * m_flStartPosition);
	m_vecPosition2 = m_vecPosition1 + (m_vecMoveDir * m_flMoveDistance);
	m_vecFinalDest = GetAbsOrigin();

	SetTouch( NULL );

	Precache();

	// It is solid?
	SetSolid( SOLID_VPHYSICS );

	if ( FClassnameIs( this, "func_water_analog" ) )
	{
		AddSolidFlags( FSOLID_VOLUME_CONTENTS );
	}

	if ( !FClassnameIs( this, "func_water_analog" ) && FBitSet (m_spawnflags, SF_MOVELINEAR_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	CreateVPhysics();
}


bool CFuncMoveLinear::ShouldSavePhysics( void )
{
	// don't save physics for func_water_analog, regen
	return !FClassnameIs( this, "func_water_analog" );
		
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncMoveLinear::CreateVPhysics( void )
{
	if ( !FClassnameIs( this, "func_water_analog" ) )
	{
		//normal door
		if ( !IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		{
			VPhysicsInitShadow( false, false );
		}
	}
	else
	{
		// special contents
		AddSolidFlags( FSOLID_VOLUME_CONTENTS );
		//SETBITS( m_spawnflags, SF_DOOR_SILENT );	// water is silent for now

		IPhysicsObject *pPhysics = VPhysicsInitShadow( false, false );
		fluidparams_t fluid;
		
		Assert( CollisionProp()->GetCollisionAngles() == vec3_angle );
		fluid.damping = 0.01f;
		fluid.surfacePlane[0] = 0;
		fluid.surfacePlane[1] = 0;
		fluid.surfacePlane[2] = 1;
		fluid.surfacePlane[3] = CollisionProp()->GetCollisionOrigin().z + CollisionProp()->OBBMaxs().z - 1;
		fluid.currentVelocity.Init(0,0,0);
		fluid.torqueFactor = 0.1f;
		fluid.viscosityFactor = 0.01f;
		fluid.pGameData = static_cast<void *>(this);
		
		//FIXME: Currently there's no way to specify that you want slime
		fluid.contents = CONTENTS_WATER;
		
		m_pFluidController = physenv->CreateFluidController( pPhysics, &fluid );
	}

	return true;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::Precache( void )
{
	if (m_soundStart != NULL_STRING)
	{
		PrecacheScriptSound( (char *) STRING(m_soundStart) );
	}
	if (m_soundStop != NULL_STRING)
	{
		PrecacheScriptSound( (char *) STRING(m_soundStop) );
	}
	m_currentSound = NULL_STRING;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::MoveTo(Vector vPosition, float flSpeed)
{
	if ( flSpeed != 0 )
	{
		if ( m_soundStart != NULL_STRING )
		{
			if (m_currentSound == m_soundStart)
			{
				StopSound(entindex(), CHAN_BODY, (char*)STRING(m_soundStop));
			}
			else
			{
				m_currentSound = m_soundStart;
				CPASAttenuationFilter filter( this );

				EmitSound_t ep;
				ep.m_nChannel = CHAN_BODY;
				ep.m_pSoundName = (char*)STRING(m_soundStart);
				ep.m_flVolume = 1;
				ep.m_SoundLevel = SNDLVL_NORM;

				EmitSound( filter, entindex(), ep );	
			}
		}

		LinearMove( vPosition, flSpeed );

		if ( m_pFluidController )
		{
			m_pFluidController->WakeAllSleepingObjects();
		}

		// Clear think (that stops sounds)
		SetThink(NULL);
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::StopMoveSound( void )
{
	if ( m_soundStart != NULL_STRING && ( m_currentSound == m_soundStart ) )
	{
		StopSound(entindex(), CHAN_BODY, (char*)STRING(m_soundStart) );
	}

	if ( m_soundStop != NULL_STRING && ( m_currentSound != m_soundStop ) )
	{
		m_currentSound = m_soundStop;
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_soundStop);
		ep.m_flVolume = 1;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}

	SetThink(NULL);
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::MoveDone( void )
{
	// Stop sounds at the next think, rather than here as another
	// SetPosition call might immediately follow the end of this move
	SetThink(&CFuncMoveLinear::StopMoveSound);
	SetNextThink( gpGlobals->curtime + 0.1f );
	BaseClass::MoveDone();

	if ( GetAbsOrigin() == m_vecPosition2 )
	{
		m_OnFullyOpen.FireOutput( this, this );
	}
	else if ( GetAbsOrigin() == m_vecPosition1 )
	{
		m_OnFullyClosed.FireOutput( this, this );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )		// Momentary buttons will pass down a float in here
		return;

	if ( value > 1.0 )
		value = 1.0;
	Vector move = m_vecPosition1 + (value * (m_vecPosition2 - m_vecPosition1));
	
	Vector delta = move - GetLocalOrigin();
	float speed = delta.Length() * 10;

	MoveTo(move, speed);
}


//-----------------------------------------------------------------------------
// Purpose: Sets the position as a value from [0..1].
//-----------------------------------------------------------------------------
void CFuncMoveLinear::SetPosition( float flPosition )
{
	Vector vTargetPos = m_vecPosition1 + ( flPosition * (m_vecPosition2 - m_vecPosition1));
	if ((vTargetPos - GetLocalOrigin()).Length() > 0.001)
	{
		MoveTo(vTargetPos, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::InputOpen( inputdata_t &inputdata )
{
	if (GetLocalOrigin() != m_vecPosition2)
	{
		MoveTo(m_vecPosition2, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CFuncMoveLinear::InputClose( inputdata_t &inputdata )
{
	if (GetLocalOrigin() != m_vecPosition1)
	{
		MoveTo(m_vecPosition1, m_flSpeed);
	}
}


//------------------------------------------------------------------------------
// Purpose: Input handler for setting the position from [0..1].
// Input  : Float position.
//-----------------------------------------------------------------------------
void CFuncMoveLinear::InputSetPosition( inputdata_t &inputdata )
{
	SetPosition( inputdata.value.Float() );
}


//-----------------------------------------------------------------------------
// Purpose: Called every frame when the bruch is blocked while moving
// Input  : pOther - The blocking entity.
//-----------------------------------------------------------------------------
void CFuncMoveLinear::Blocked( CBaseEntity *pOther )
{
	// Hurt the blocker 
	if ( m_flBlockDamage )
	{
		if ( pOther->m_takedamage == DAMAGE_EVENTS_ONLY )
		{
			if ( FClassnameIs( pOther, "gib" ) )
				UTIL_Remove( pOther );
		}
		else
			pOther->TakeDamage( CTakeDamageInfo( this, this, m_flBlockDamage, DMG_CRUSH ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncMoveLinear::InputSetSpeed( inputdata_t &inputdata )
{
	// Set the new speed
	m_flSpeed = inputdata.value.Float();

	// FIXME: This is a little questionable.  Do we want to fix the speed, or let it continue on at the old speed?
	float flDistToGoalSqr = ( m_vecFinalDest - GetAbsOrigin() ).LengthSqr();
	if ( flDistToGoalSqr > Square( FLT_EPSILON ) )
	{
		// NOTE: We do NOT want to call sound functions here, just vanilla position changes
		LinearMove( m_vecFinalDest, m_flSpeed );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CFuncMoveLinear::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		float flTravelDist = (m_vecPosition1 - m_vecPosition2).Length();
		float flCurDist	   = (m_vecPosition1 - GetLocalOrigin()).Length();
		Q_snprintf(tempstr,sizeof(tempstr),"Current Pos: %3.3f",flCurDist/flTravelDist);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		float flTargetDist	   = (m_vecPosition1 - m_vecFinalDest).Length();
		Q_snprintf(tempstr,sizeof(tempstr),"Target Pos: %3.3f",flTargetDist/flTravelDist);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
