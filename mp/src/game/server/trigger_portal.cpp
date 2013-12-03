//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity which teleports touched entities and reorients their physics
//
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "triggers.h"
#include "modelentities.h"
#include "saverestore_utlvector.h"
#include "player_pickup.h"
#include "vphysics/friction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TRIGGER_DISABLED_THINK			"PortalDisabledThink"

ConVar portal_debug( "portal_debug", "0", FCVAR_CHEAT, "Turn on debugging for portal connections." );

//////////////////////////////////////////////////////////////////////////
// CTriggerPortal
// Moves touched entity to a target location, changing the model's orientation
// to match the exit target. It differs from CTriggerTeleport in that it
// reorients physics and has inputs to enable/disable its function.
//////////////////////////////////////////////////////////////////////////
class CTriggerPortal : public CBaseTrigger
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CTriggerPortal, CBaseTrigger );
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void Activate();

	void Touch( CBaseEntity *pOther );
	void EndTouch(CBaseEntity *pOther);
	void DisableForIncomingEntity( CBaseEntity *pEntity );
	bool IsTouchingPortal( CBaseEntity *pEntity );

	void DisabledThink( void );

	// TEMP: Since brushes have no directionality, give this wall a forward face specified in hammer
	QAngle						m_qFaceAngles;

private:
	string_t					m_strRemotePortal;
	CNetworkHandle( CTriggerPortal, m_hRemotePortal );
	CUtlVector<EHANDLE>			m_hDisabledForEntities;

	// Input for setting remote portal entity (for teleporting to it)
	void SetRemotePortal ( const char* strRemotePortalName );
	void InputSetRemotePortal ( inputdata_t &inputdata );

};

LINK_ENTITY_TO_CLASS( trigger_portal, CTriggerPortal );

BEGIN_DATADESC( CTriggerPortal )
	DEFINE_KEYFIELD( m_strRemotePortal, FIELD_STRING, "RemotePortal" ),

	DEFINE_FIELD( m_hRemotePortal, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hDisabledForEntities, FIELD_EHANDLE ),

	// TEMP: Only keep this field while portals are still brushes
	DEFINE_FIELD( m_qFaceAngles, FIELD_VECTOR ),

	DEFINE_THINKFUNC( DisabledThink ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetRemotePortal", InputSetRemotePortal ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTriggerPortal, DT_TriggerPortal )
	SendPropEHandle(SENDINFO(m_hRemotePortal)),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerPortal::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTriggerPortal::Activate()
{
	BaseClass::Activate();

	m_qFaceAngles = this->GetAbsAngles();

	// keep the remote portal's pointer at activate time to avoid redundant FindEntity calls
	if ( m_strRemotePortal != NULL_STRING )
	{
		SetRemotePortal( STRING(m_strRemotePortal) );
		m_strRemotePortal = NULL_STRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CTriggerPortal::InputSetRemotePortal(inputdata_t &inputdata )
{
	SetRemotePortal( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strRemotePortalName - 
//-----------------------------------------------------------------------------
void CTriggerPortal::SetRemotePortal(const char *strRemotePortalName )
{
	m_hRemotePortal = dynamic_cast<CTriggerPortal*> (gEntList.FindEntityByName( NULL, strRemotePortalName, NULL, NULL, NULL ));
	if ( m_hRemotePortal == NULL )
	{
		Warning ( "trigger_portal: Cannot find remote portal entity named %s\n", strRemotePortalName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPortal::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch(pOther);

	if ( portal_debug.GetBool() )
	{
		Msg("%s ENDTOUCH: for %s\n", GetDebugName(), pOther->GetDebugName() );
	}

	EHANDLE hHandle;
	hHandle = pOther;
	m_hDisabledForEntities.FindAndRemove( hHandle );
}

//-----------------------------------------------------------------------------
// Purpose: Upon touching a non-filtered entity, CTriggerPortal teleports them to it's
//			remote portal location.
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CTriggerPortal::Touch( CBaseEntity *pOther )
{
	// If we are enabled, and allowed to react to the touched entity
	if ( PassesTriggerFilters(pOther) )
	{
		// If we somehow lost our pointer to the remote portal, get a new one
		if ( m_hRemotePortal == NULL )
		{
			Disable();
			return;
		}

		bool bDebug = portal_debug.GetBool();
		if ( bDebug )
		{
			Msg("%s TOUCH: for %s\n", GetDebugName(), pOther->GetDebugName() );
		}

		// Don't touch entities that came through us and haven't left us yet.
		EHANDLE hHandle;
		hHandle = pOther;
		if ( m_hDisabledForEntities.Find(hHandle) != m_hDisabledForEntities.InvalidIndex() )
		{
			Msg("    IGNORED\n", GetDebugName(), pOther->GetDebugName() );
			return;
		}

		Pickup_ForcePlayerToDropThisObject( pOther );

		// de-ground this entity
        pOther->SetGroundEntity( NULL );

		// Build a this --> remote transformation
		VMatrix matMyModelToWorld, matMyInverse;
		matMyModelToWorld = this->EntityToWorldTransform();
		MatrixInverseGeneral ( matMyModelToWorld, matMyInverse );

		// Teleport our object
		VMatrix matRemotePortalTransform = m_hRemotePortal->EntityToWorldTransform();
		Vector ptNewOrigin, vLook, vRight, vUp, vNewLook;
		pOther->GetVectors( &vLook, &vRight, &vUp );

		// Move origin
		ptNewOrigin = matMyInverse * pOther->GetAbsOrigin();
		ptNewOrigin = matRemotePortalTransform * Vector( ptNewOrigin.x, -ptNewOrigin.y, ptNewOrigin.z );

		// Re-aim camera
		vNewLook	= matMyInverse.ApplyRotation( vLook );
		vNewLook	= matRemotePortalTransform.ApplyRotation( Vector( -vNewLook.x, -vNewLook.y, vNewLook.z ) );

		// Reorient the physics
	 	Vector vVelocity, vOldVelocity;
		pOther->GetVelocity( &vOldVelocity );
		vVelocity = matMyInverse.ApplyRotation( vOldVelocity );
		vVelocity = matRemotePortalTransform.ApplyRotation( Vector( -vVelocity.x, -vVelocity.y, vVelocity.z ) );

		QAngle qNewAngles;
		VectorAngles( vNewLook, qNewAngles );
		
		if ( pOther->IsPlayer() )
		{
			((CBasePlayer*)pOther)->SnapEyeAngles(qNewAngles);
		}

		Vector vecOldPos = pOther->WorldSpaceCenter();
		if ( bDebug )
		{
			NDebugOverlay::Box( pOther->GetAbsOrigin(), pOther->WorldAlignMins(), pOther->WorldAlignMaxs(), 255,0,0, 8, 20 );
			NDebugOverlay::Axis( pOther->GetAbsOrigin(), pOther->GetAbsAngles(), 10.0f, true, 50 );
		}

		// place player at the new destination
		CTriggerPortal *pPortal = m_hRemotePortal.Get();
		pPortal->DisableForIncomingEntity( pOther );
		pOther->Teleport( &ptNewOrigin, &qNewAngles, &vVelocity );

		if ( bDebug )
		{
			NDebugOverlay::Box( pOther->GetAbsOrigin(), pOther->WorldAlignMins(), pOther->WorldAlignMaxs(), 0,255,0, 8, 20 );
			NDebugOverlay::Line( vecOldPos, pOther->WorldSpaceCenter(), 0,255,0, true, 20 );
			NDebugOverlay::Axis( pOther->GetAbsOrigin(), pOther->GetAbsAngles(), 10.0f, true, 50 );

			Msg("%s TELEPORTED: %s\n", GetDebugName(), pOther->GetDebugName() );
		}

		// test collision on the new teleport location
		Vector vMin, vMax, vCenter;
		pOther->CollisionProp()->WorldSpaceAABB( &vMin, &vMax );
		vCenter = (vMin + vMax) * 0.5f;
		vMin -= vCenter;
		vMax -= vCenter;

		Vector vStart, vEnd;
		vStart	= ptNewOrigin;
		vEnd	= ptNewOrigin;

		Ray_t ray;
		ray.Init( vStart, vEnd, vMin, vMax );
		trace_t tr;
		pPortal->TestCollision( ray, pOther->PhysicsSolidMaskForEntity(), tr );

		// Teleportation caused us to hit something, deal with it.
		if ( tr.DidHit() )
		{
			
		}

		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerPortal::DisableForIncomingEntity( CBaseEntity *pEntity )
{
	EHANDLE hHandle;
	hHandle = pEntity;
	Assert( m_hDisabledForEntities.Find(hHandle) == m_hDisabledForEntities.InvalidIndex() );
	m_hDisabledForEntities.AddToTail( hHandle );

	// Start thinking, and remove the other as soon as it's not touching me.
	// Needs to be done in addition to EndTouch, because entities may move fast
	// enough through the portal to come out not touching the other portal.
	SetContextThink( DisabledThink, gpGlobals->curtime + 0.1, TRIGGER_DISABLED_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTriggerPortal::DisabledThink( void )
{
	// If we've got no disabled entities left, we're done
	if ( !m_hDisabledForEntities.Count() )
	{
		SetContextThink( NULL, gpGlobals->curtime, TRIGGER_DISABLED_THINK );
		return;
	}

	for ( int i = m_hDisabledForEntities.Count()-1; i >= 0; i-- )
	{
		CBaseEntity *pEntity = m_hDisabledForEntities[i];
		if ( !pEntity || !IsTouchingPortal(pEntity) )
		{
			m_hDisabledForEntities.Remove(i);
		}
	}

	SetContextThink( DisabledThink, gpGlobals->curtime + 0.1, TRIGGER_DISABLED_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTriggerPortal::IsTouchingPortal( CBaseEntity *pEntity )
{
	// First, check the touchlinks. This will find non-vphysics entities touching us
    touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch == pEntity )
				return true;
		}
	}

	// Then check the friction snapshot. This will find vphysics objects touching us.
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( !pPhysics )
		return false;

	IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();
	bool bFound = false;
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject( 1 );
		if ( ((CBaseEntity *)pOther->GetGameData()) == pEntity )
		{
			bFound = true;
			break;
		}

		pSnapshot->NextFrictionData();
	}
	pPhysics->DestroyFrictionSnapshot( pSnapshot );

	return bFound;
}