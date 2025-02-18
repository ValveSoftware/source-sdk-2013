//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Frequently used global functions.
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "doors.h"
#include "entitylist.h"
#include "globals.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Landmark class
void CPointEntity::Spawn( void )
{
	SetSolid( SOLID_NONE );
//	UTIL_SetSize(this, vec3_origin, vec3_origin);
}


class CNullEntity : public CBaseEntity
{
public:
	DECLARE_CLASS( CNullEntity, CBaseEntity );

	void Spawn( void );
};


// Null Entity, remove on startup
void CNullEntity::Spawn( void )
{
	UTIL_Remove( this );
}
LINK_ENTITY_TO_CLASS(info_null,CNullEntity);

class CBaseDMStart : public CPointEntity
{
public:
	DECLARE_CLASS( CBaseDMStart, CPointEntity );

	bool IsTriggered( CBaseEntity *pEntity );

	DECLARE_DATADESC();

	string_t m_Master;

private:
};

BEGIN_DATADESC( CBaseDMStart )

	DEFINE_KEYFIELD( m_Master, FIELD_STRING, "master" ),

END_DATADESC()


// These are the new entry points to entities. 
LINK_ENTITY_TO_CLASS(info_player_deathmatch,CBaseDMStart);
LINK_ENTITY_TO_CLASS(info_player_start,CPointEntity);
LINK_ENTITY_TO_CLASS(info_landmark,CPointEntity);

bool CBaseDMStart::IsTriggered( CBaseEntity *pEntity )
{
	bool master = UTIL_IsMasterTriggered( m_Master, pEntity );

	return master;
}


// Convenient way to delay removing oneself
void CBaseEntity::SUB_Remove( void )
{
	if (m_iHealth > 0)
	{
		// this situation can screw up NPCs who can't tell their entity pointers are invalid.
		m_iHealth = 0;
		DevWarning( 2, "SUB_Remove called on entity with health > 0\n");
	}

	UTIL_Remove( this );
}


// Convenient way to explicitly do nothing (passed to functions that require a method)
void CBaseEntity::SUB_DoNothing( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Finds all active entities with the given targetname and calls their
//			'Use' function.
// Input  : targetName - Target name to search for.
//			pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = NULL;
	if ( !targetName || !targetName[0] )
		return;

	DevMsg( 2, "Firing: (%s)\n", targetName );

	for (;;)
	{
		CBaseEntity *pSearchingEntity = pActivator;
		pTarget = gEntList.FindEntityByName( pTarget, targetName, pSearchingEntity, pActivator, pCaller );
		if ( !pTarget )
			break;

		if (!pTarget->IsMarkedForDeletion() )	// Don't use dying ents
		{
			DevMsg( 2, "[%03d] Found: %s, firing (%s)\n", gpGlobals->tickcount%1000, pTarget->GetDebugName(), targetName );
			pTarget->Use( pActivator, pCaller, useType, value );
		}
	}
}

enum togglemovetypes_t
{
	MOVE_TOGGLE_NONE = 0,
	MOVE_TOGGLE_LINEAR = 1,
	MOVE_TOGGLE_ANGULAR = 2,
};

// Global Savedata for Toggle
BEGIN_DATADESC( CBaseToggle )

	DEFINE_FIELD( m_toggle_state, FIELD_INTEGER ),
	DEFINE_FIELD( m_flMoveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( m_flLip, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecPosition1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPosition2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecMoveAng, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( m_vecAngle1, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( m_vecAngle2, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( m_flHeight, FIELD_FLOAT ),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecFinalDest, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecFinalAngle, FIELD_VECTOR ),
	DEFINE_FIELD( m_sMaster, FIELD_STRING),
	DEFINE_FIELD( m_movementType, FIELD_INTEGER ),	// Linear or angular movement? (togglemovetypes_t)

END_DATADESC()


CBaseToggle::CBaseToggle()
{
#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_vecPosition1.Init();
	m_vecPosition2.Init();
	m_vecAngle1.Init();
	m_vecAngle2.Init();
	m_vecFinalDest.Init();
	m_vecFinalAngle.Init();
#endif
}

bool CBaseToggle::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "lip"))
	{
		m_flLip = atof(szValue);
	}
	else if (FStrEq(szKeyName, "wait"))
	{
		m_flWait = atof(szValue);
	}
	else if (FStrEq(szKeyName, "master"))
	{
		m_sMaster = AllocPooledString(szValue);
	}
	else if (FStrEq(szKeyName, "distance"))
	{
		m_flMoveDistance = atof(szValue);
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Calculate m_vecVelocity and m_flNextThink to reach vecDest from
//			GetOrigin() traveling at flSpeed.
// Input  : Vector	vecDest - 
//			flSpeed - 
//-----------------------------------------------------------------------------
void CBaseToggle::LinearMove( const Vector &vecDest, float flSpeed )
{
	ASSERTSZ(flSpeed != 0, "LinearMove:  no speed is defined!");
	
	m_vecFinalDest = vecDest;

	m_movementType = MOVE_TOGGLE_LINEAR;
	// Already there?
	if (vecDest == GetLocalOrigin())
	{
		MoveDone();
		return;
	}
		
	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - GetLocalOrigin();
	
	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set m_flNextThink to trigger a call to LinearMoveDone when dest is reached
	SetMoveDoneTime( flTravelTime );

	// scale the destdelta vector by the time spent traveling to get velocity
	SetLocalVelocity( vecDestDelta / flTravelTime );
}


void CBaseToggle::MoveDone( void )
{
	switch ( m_movementType )
	{
	case MOVE_TOGGLE_LINEAR:
		LinearMoveDone();
		break;
	case MOVE_TOGGLE_ANGULAR:
		AngularMoveDone();
		break;
	}
	m_movementType = MOVE_TOGGLE_NONE;
	BaseClass::MoveDone();
}
//-----------------------------------------------------------------------------
// Purpose: After moving, set origin to exact final destination, call "move done" function.
//-----------------------------------------------------------------------------
void CBaseToggle::LinearMoveDone( void )
{
	UTIL_SetOrigin( this, m_vecFinalDest);
	SetAbsVelocity( vec3_origin );
	SetMoveDoneTime( -1 );
}


// DVS TODO: obselete, remove?
bool CBaseToggle::IsLockedByMaster( void )
{
	if (m_sMaster != NULL_STRING && !UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return true;
	else
		return false;
}


//-----------------------------------------------------------------------------
// Purpose: Calculate m_vecVelocity and m_flNextThink to reach vecDest from
//			GetLocalOrigin() traveling at flSpeed. Just like LinearMove, but rotational.
// Input  : vecDestAngle - 
//			flSpeed - 
//-----------------------------------------------------------------------------
void CBaseToggle::AngularMove( const QAngle &vecDestAngle, float flSpeed )
{
	ASSERTSZ(flSpeed != 0, "AngularMove:  no speed is defined!");
	
	m_vecFinalAngle = vecDestAngle;

	m_movementType = MOVE_TOGGLE_ANGULAR;
	// Already there?
	if (vecDestAngle == GetLocalAngles())
	{
		MoveDone();
		return;
	}
	
	// set destdelta to the vector needed to move
	QAngle vecDestDelta = vecDestAngle - GetLocalAngles();
	
	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	const float MinTravelTime = 0.01f;
	if ( flTravelTime < MinTravelTime )
	{
		// If we only travel for a short time, we can fail WillSimulateGamePhysics()
		flTravelTime = MinTravelTime;
		flSpeed = vecDestDelta.Length() / flTravelTime;
	}

	// set m_flNextThink to trigger a call to AngularMoveDone when dest is reached
	SetMoveDoneTime( flTravelTime );

	// scale the destdelta vector by the time spent traveling to get velocity
	SetLocalAngularVelocity( vecDestDelta * (1.0 / flTravelTime) );
}


//-----------------------------------------------------------------------------
// Purpose: After rotating, set angle to exact final angle, call "move done" function.
//-----------------------------------------------------------------------------
void CBaseToggle::AngularMoveDone( void )
{
	SetLocalAngles( m_vecFinalAngle );
	SetLocalAngularVelocity( vec3_angle );
	SetMoveDoneTime( -1 );
}


float CBaseToggle::AxisValue( int flags, const QAngle &angles )
{
	if ( FBitSet(flags, SF_DOOR_ROTATE_ROLL) )
		return angles.z;
	if ( FBitSet(flags, SF_DOOR_ROTATE_PITCH) )
		return angles.x;

	return angles.y;
}


void CBaseToggle::AxisDir( void )
{
	if ( m_spawnflags & SF_DOOR_ROTATE_ROLL )
		m_vecMoveAng = QAngle( 0, 0, 1 );	// angles are roll
	else if ( m_spawnflags & SF_DOOR_ROTATE_PITCH )
		m_vecMoveAng = QAngle( 1, 0, 0 );	// angles are pitch
	else
		m_vecMoveAng = QAngle( 0, 1, 0 );		// angles are yaw
}


float CBaseToggle::AxisDelta( int flags, const QAngle &angle1, const QAngle &angle2 )
{
	// UNDONE: Use AngleDistance() here?
	if ( FBitSet (flags, SF_DOOR_ROTATE_ROLL) )
		return angle1.z - angle2.z;
	
	if ( FBitSet (flags, SF_DOOR_ROTATE_PITCH) )
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}


