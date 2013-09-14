//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: seco7_portal. The seco7 portal. 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "baseentity.h"
#include "triggers.h"
#include "modelentities.h"
#include "saverestore_utlvector.h"
#include "player_pickup.h"
#include "KeyValues.h"
#include "props.h"
#include "vphysics/friction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TRIGGER_DISABLED_THINK			"PortalDisabledThink"

//////////////////////////////////////////////////////////////////////////
// Cseco7_Portal
// Moves touched entity to a target location, changing the model's orientation
// to match the exit target. It differs from CTriggerTeleport in that it
// reorients physics and has inputs to enable/disable its function.
//////////////////////////////////////////////////////////////////////////
class Cseco7_Portal : public CBaseAnimating
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( Cseco7_Portal, CBaseAnimating );

	virtual void Spawn( void );
	virtual void Precache( void );

	void Touch( CBaseEntity *pOther );
	void EndTouch(CBaseEntity *pOther);
	void DisableForIncomingEntity( CBaseEntity *pEntity );
	bool IsTouchingPortal( CBaseEntity *pEntity );
	
	void DisabledThink( void );

private:
	string_t					m_strRemotePortal;
	CNetworkHandle( Cseco7_Portal, m_hRemotePortal );
	CUtlVector<EHANDLE>			m_hDisabledForEntities;

	// Input for setting remote portal entity (for teleporting to it)
	void SetRemotePortal ( const char* strRemotePortalName );
};

LINK_ENTITY_TO_CLASS( seco7_portal, Cseco7_Portal );

BEGIN_DATADESC( Cseco7_Portal )
	DEFINE_KEYFIELD( m_strRemotePortal, FIELD_STRING, "RemotePortal" ),

	DEFINE_FIELD( m_hRemotePortal, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hDisabledForEntities, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( DisabledThink ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void Cseco7_Portal::Precache( void )
{
	PrecacheModel ( "models/obco_portal1.mdl" );
	PrecacheModel ( "models/obco_portal2.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cseco7_Portal::Spawn( void )
{
	Precache();
	SetModel ( "models/obco_portal1.mdl" );
	//Player portal is bigger to make it easier to fall into one below you.
	UTIL_SetSize( this, -Vector(40,40,40), Vector(40,40,40) );
	SetSolid( SOLID_BBOX );
	AddSolidFlags ( FSOLID_TRIGGER|FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	
	//We cheated and set our portals current name to that of the players steamID.
	//Now we turn this into something we can reference for our code.
	const char *PlayerSteamID = GetDebugName();
	
	char Portal1Name[ 512 ];
                Q_strncpy( Portal1Name, "Portal1_" ,sizeof(Portal1Name));
                Q_strncat( Portal1Name, PlayerSteamID,sizeof(Portal1Name), COPY_ALL_CHARACTERS );
				
				char Portal2Name[ 512 ];
                Q_strncpy( Portal2Name, "Portal2_" ,sizeof(Portal2Name));
                Q_strncat( Portal2Name, PlayerSteamID,sizeof(Portal2Name), COPY_ALL_CHARACTERS );
				
				//KeyValue( "targetname", Portal1Name );
				//SetRemotePortal(Portal2Name);
               
    //We need to check that a Portal doesn't exsist already, and if it does - remove it.
   /* CBaseEntity *pPortal1 = gEntList.FindEntityByName( NULL, Portal1Name );
		if ( !pPortal1 )
		{
		    //We didn't find a Portal1, so we'll set our name to Portal1Name.
			KeyValue( "targetname", Portal1Name );
			//And finally set our remote destination to Portal1
			SetRemotePortal(Portal2Name);
		}
		else if ( pPortal1 )
		{
			//We must remove any that already exsist here.
			UTIL_Remove( pPortal1 );
			//We found Portal2 so we'll set our name to Portal1Name/
			KeyValue( "targetname", Portal1Name );
			//And finally set our remote destination to Portal2
			SetRemotePortal(Portal2Name);					
		}
		else
		{
		return;
		}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strRemotePortalName - 
//-----------------------------------------------------------------------------
void Cseco7_Portal::SetRemotePortal(const char *strRemotePortalName)
{

	m_hRemotePortal = dynamic_cast<Cseco7_Portal*> (gEntList.FindEntityByName( NULL, strRemotePortalName, NULL, NULL, NULL ));
	if ( m_hRemotePortal == NULL )
	{
		Msg ( "trigger_portal: Cannot find remote portal entity named %s\n", m_hRemotePortal );
	}
}

//--------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void Cseco7_Portal::EndTouch(CBaseEntity *pOther)
{
	BaseClass::EndTouch(pOther);

	EHANDLE hHandle;
	hHandle = pOther;
	m_hDisabledForEntities.FindAndRemove( hHandle );
}

//------------------------------------------------------------------------------
// A small wrapper around SV_Move that never clips against the supplied entity.
//------------------------------------------------------------------------------
static bool TestEntityPosition ( CBaseEntity *pOther )
{	
	trace_t	trace;
	UTIL_TraceEntity( pOther, pOther->GetAbsOrigin(), pOther->GetAbsOrigin(), MASK_PLAYERSOLID, &trace );
	return (trace.startsolid == 0);
}


//------------------------------------------------------------------------------
// Searches along the direction ray in steps of "step" to see if 
// the entity position is passible.
// Used for putting the player in valid space when toggling off noclip mode.
//------------------------------------------------------------------------------
static int FindPassableSpace( CBaseEntity *pOther, const Vector& direction, float step, Vector& oldorigin )
{
	int i;
	for ( i = 0; i < 100; i++ )
	{
		Vector origin = pOther->GetAbsOrigin();
		VectorMA( origin, step, direction, origin );
		pOther->SetAbsOrigin( origin );
		if ( TestEntityPosition( pOther ) )
		{
			VectorCopy( pOther->GetAbsOrigin(), oldorigin );
			return 1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Upon touching a non-filtered entity, Cseco7_Portal teleports them to it's
//			remote portal location.
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void Cseco7_Portal::Touch( CBaseEntity *pOther )
{
		// If we somehow lost our pointer to the remote portal, get a new one
		if ( m_hRemotePortal == NULL )
		{
			Msg ( "trigger_portal: Cannot find remote portal entity named %s\n", m_hRemotePortal );
			return;
		}


		// Don't touch entities that came through us and haven't left us yet.
		EHANDLE hHandle;
		hHandle = pOther;
		if ( m_hDisabledForEntities.Find(hHandle) != m_hDisabledForEntities.InvalidIndex() )
		{
			//Msg("    IGNORED\n", GetDebugName(), pOther->GetDebugName() );
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
			
			// Start getting our player ready for noclip.
			pOther->SetParent( NULL );
			pOther->SetMoveType( MOVETYPE_NOCLIP );
			pOther->AddEFlags( EFL_NOCLIP_ACTIVE );
		}

		Vector vecOldPos = pOther->WorldSpaceCenter();

		// place player at the new destination
		Cseco7_Portal *pPortal = m_hRemotePortal.Get();
		pPortal->DisableForIncomingEntity( pOther );
		pOther->Teleport( &ptNewOrigin, &qNewAngles, &vVelocity );

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
		
		if ( pOther->IsPlayer() )
		{
		pOther->RemoveEFlags( EFL_NOCLIP_ACTIVE );
		pOther->SetMoveType( MOVETYPE_WALK );
		
		CBasePlayer *pPlayer = ToBasePlayer( pOther );
		
		CPlayerState *pl = pPlayer->PlayerData();
		Assert( pl );

			Vector oldorigin = pOther->GetAbsOrigin();
			if ( !TestEntityPosition( pOther ) )
			{
				Vector forward, right, up;

				AngleVectors ( pl->v_angle, &forward, &right, &up);
		
				// Try to move into the world
				if ( !FindPassableSpace( pOther, forward, 1, oldorigin ) )
				{
					if ( !FindPassableSpace( pOther, right, 1, oldorigin ) )
					{
						if ( !FindPassableSpace( pOther, right, -1, oldorigin ) )		// left
						{
							if ( !FindPassableSpace( pOther, up, 1, oldorigin ) )	// up
							{
								if ( !FindPassableSpace( pOther, up, -1, oldorigin ) )	// down
								{
									if ( !FindPassableSpace( pOther, forward, -1, oldorigin ) )	// back
									{
										//Kill the player because we're stuck in a wall.
										pPlayer->CommitSuicide();
									}
								}
							}
						}
					}
				}

				pOther->SetAbsOrigin( oldorigin );
			}
		}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cseco7_Portal::DisableForIncomingEntity( CBaseEntity *pEntity )
{
	EHANDLE hHandle;
	hHandle = pEntity;
	Assert( m_hDisabledForEntities.Find(hHandle) == m_hDisabledForEntities.InvalidIndex() );
	m_hDisabledForEntities.AddToTail( hHandle );

	// Start thinking, and remove the other as soon as it's not touching me.
	// Needs to be done in addition to EndTouch, because entities may move fast
	// enough through the portal to come out not touching the other portal.

	SetContextThink( &Cseco7_Portal::DisabledThink, gpGlobals->curtime + 0.1, TRIGGER_DISABLED_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Cseco7_Portal::DisabledThink( void )
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
	SetContextThink( &Cseco7_Portal::DisabledThink, gpGlobals->curtime + 0.1, TRIGGER_DISABLED_THINK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool Cseco7_Portal::IsTouchingPortal( CBaseEntity *pEntity )
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