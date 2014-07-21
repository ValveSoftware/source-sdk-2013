//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Mirrors the movement of a camera about a given point.
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "modelentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//////////////////////////////////////////////////////////////////////////
// CLogicMirrorMovement
// This will record the vector offset of an entity's center from a given reference point 
// (most likely the center of a mirror or portal) and place an entity (most likely a point camera)
// at a the same offset, mirrored about the reference point and orientation.
//////////////////////////////////////////////////////////////////////////
class CLogicMirrorMovement : public CLogicalEntity
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CLogicMirrorMovement, CLogicalEntity );

private:
	void SetMirrorTarget( const char *pName );				// Set entity to watch and mirror (ex. the player)
	void SetTarget( const char *pName );					// Set entity to move based on the Mirror Target entity (ex. a point_camera)
	void SetMirrorRelative( const char* pName );			// Set the point about which to measure an offset to orient based upon (ex. portal 1)
	void SetRemoteTarget ( const char *pName );				// Entity's orientation/location from which to offset the movement target (ex. portal 2)
	void SetDrawingSurface ( const char *pName );

	void InputSetMirrorTarget( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata );
	void InputSetRemoteTarget ( inputdata_t &inputdata );
	void InputSetMirrorRelative ( inputdata_t &inputdata );

	void Think();
	virtual void Activate();

	string_t m_strMirrorTarget;
	string_t m_strRemoteTarget;
	string_t m_strMirrorRelative;

	EHANDLE m_hRemoteTarget;
	EHANDLE m_hMirrorTarget;
	EHANDLE m_hMovementTarget;
	EHANDLE m_hMirrorRelative;
	
};

LINK_ENTITY_TO_CLASS( logic_mirror_movement, CLogicMirrorMovement );


BEGIN_DATADESC( CLogicMirrorMovement )

DEFINE_KEYFIELD( m_strMirrorTarget, FIELD_STRING, "MirrorTarget" ),
DEFINE_KEYFIELD( m_strRemoteTarget, FIELD_STRING, "RemoteTarget" ),
DEFINE_KEYFIELD( m_strMirrorRelative, FIELD_STRING, "MirrorRelative" ),

DEFINE_FIELD( m_hMirrorTarget, FIELD_EHANDLE ),
DEFINE_FIELD( m_hMovementTarget, FIELD_EHANDLE ),
DEFINE_FIELD( m_hRemoteTarget, FIELD_EHANDLE ),
DEFINE_FIELD( m_hMirrorRelative, FIELD_EHANDLE ),

DEFINE_INPUTFUNC( FIELD_STRING, "SetMirrorTarget", InputSetMirrorTarget ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetTarget", InputSetTarget ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetRemoteTarget", InputSetRemoteTarget ),
DEFINE_INPUTFUNC( FIELD_STRING, "SetMirrorRelative", InputSetMirrorRelative ),

DEFINE_THINKFUNC( Think ),

END_DATADESC()


void CLogicMirrorMovement::Activate()
{
	BaseClass::Activate();

	SetMirrorTarget( STRING(m_strMirrorTarget) );
	SetTarget( STRING(m_target) );
	SetRemoteTarget( STRING(m_strRemoteTarget ) );
	SetMirrorRelative( STRING( m_strMirrorRelative) );

	SetThink( &CLogicMirrorMovement::Think );
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}


void CLogicMirrorMovement::SetMirrorTarget( const char *pName )
{
	m_hMirrorTarget = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hMirrorTarget )
	{
		if ( Q_strnicmp( STRING(m_strMirrorTarget), "!player", 8 ) )
		{
			Warning("logic_mirror_movement: Unable to find mirror target entity %s\n", pName );
		}
	}
}

void CLogicMirrorMovement::SetTarget( const char *pName )
{
	m_hMovementTarget = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hMovementTarget )
	{
		Warning("logic_mirror_movement: Unable to find movement target entity %s\n", pName );
	}
}

void CLogicMirrorMovement::SetRemoteTarget(const char *pName )
{
	m_hRemoteTarget = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hRemoteTarget )
	{
		Warning("logic_mirror_movement: Unable to find remote target entity %s\n", pName );
	}
}

void CLogicMirrorMovement::SetMirrorRelative(const char* pName )
{
	m_hMirrorRelative = gEntList.FindEntityByName( NULL, pName );
	if ( !m_hMirrorRelative )
	{
		Warning("logic_mirror_movement: Unable to find mirror relative entity %s\n", pName );
	}
}

void CLogicMirrorMovement::InputSetMirrorTarget( inputdata_t &inputdata )
{
	m_strMirrorTarget = AllocPooledString( inputdata.value.String() );
	SetMirrorTarget( inputdata.value.String() );
}

void CLogicMirrorMovement::InputSetTarget( inputdata_t &inputdata )
{
	m_target = AllocPooledString( inputdata.value.String() );
	SetTarget( inputdata.value.String() );
}

void CLogicMirrorMovement::InputSetRemoteTarget(inputdata_t &inputdata )
{
	m_strRemoteTarget = AllocPooledString( inputdata.value.String() );
	SetRemoteTarget( inputdata.value.String() );
}

void CLogicMirrorMovement::InputSetMirrorRelative(inputdata_t &inputdata )
{
	m_strMirrorRelative = AllocPooledString ( inputdata.value.String() );
	SetMirrorRelative( inputdata.value.String() );
}


void CLogicMirrorMovement::Think()
{
	// Attempt to get the player's handle because it didn't exist at Activate time
	if ( !m_hMirrorTarget.Get() )
	{
		// If we will never find a target, we don't have a use... shutdown
		if ( m_strMirrorTarget == NULL_STRING )
			SetNextThink ( NULL );

		//BUGBUG: If m_strSetMirrorTarget doesn't exist in ent list, we get per-think searches with no results ever...
		SetMirrorTarget ( STRING(m_strMirrorTarget) );
	}
 
	// Make sure all entities are valid
	if ( m_hMirrorTarget.Get() && m_hMovementTarget.Get() && m_hRemoteTarget.Get() && m_hMirrorRelative.Get() )
	{	
		// Get our two portal's world transforms transforms
		VMatrix matPortal1ToWorldInv, matPortal2ToWorld;
		MatrixInverseGeneral( m_hMirrorRelative->EntityToWorldTransform(), matPortal1ToWorldInv );
		matPortal2ToWorld = m_hRemoteTarget->EntityToWorldTransform();

		VMatrix matTransformToRemotePortal = matPortal1ToWorldInv * matPortal2ToWorld;

		// Get our scene camera's current orientation
		Vector ptCameraPosition, vCameraLook, vCameraRight, vCameraUp;
		ptCameraPosition		= m_hMirrorTarget->EyePosition();
		m_hMirrorTarget->GetVectors ( &vCameraLook, &vCameraRight, &vCameraUp );

        // map this position and orientation to the remote portal, mirrored (invert the result)
		Vector ptNewPosition, vNewLook;
		ptNewPosition	= matPortal1ToWorldInv * ptCameraPosition;
		ptNewPosition	= matPortal2ToWorld*( Vector( -ptNewPosition.x, -ptNewPosition.y, ptNewPosition.z ) );

		vNewLook		= matPortal1ToWorldInv.ApplyRotation( vCameraLook );
		vNewLook		= matPortal2ToWorld.ApplyRotation( Vector( -vNewLook.x, -vNewLook.y, vNewLook.z) );

		// Set the point camera to the new location/orientation
		QAngle qNewAngles;
		VectorAngles( vNewLook, qNewAngles );
		m_hMovementTarget->Teleport( &ptNewPosition, &qNewAngles, NULL );
	}

	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}


