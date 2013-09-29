//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: SecobModportal2s act as targets for other NPC's to attack and to trigger
//			events 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "decals.h"
#include "filters.h"
#include "npc_SecobModportal2.h"
#include "hl2mp_player.h"
#include "collisionutils.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CSecobModportal2List : public CAutoGameSystem
{
public:
	CSecobModportal2List( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual void LevelShutdownPostEntity() 
	{
		Clear();
	}

	void Clear()
	{
		m_list.Purge();
	}

	void AddToList( CNPC_SecobModportal2 *pSecobModportal2 );
	void RemoveFromList( CNPC_SecobModportal2 *pSecobModportal2 );

	CUtlVector< CNPC_SecobModportal2 * >	m_list;
};

void CSecobModportal2List::AddToList( CNPC_SecobModportal2 *pSecobModportal2 )
{
	m_list.AddToTail( pSecobModportal2 );
}

void CSecobModportal2List::RemoveFromList( CNPC_SecobModportal2 *pSecobModportal2 )
{
	int index = m_list.Find( pSecobModportal2 );
	if ( index != m_list.InvalidIndex() )
	{
		m_list.FastRemove( index );
	}
}

CSecobModportal2List g_SecobModportal2List( "CSecobModportal2List" );

int FindSecobModportal2sInCone( CBaseEntity **pList, int listMax, const Vector &coneOrigin, const Vector &coneAxis, float coneAngleCos, float coneLength )
{
	if ( listMax <= 0 )
		return 0;

	int count = 0;

	for ( int i = g_SecobModportal2List.m_list.Count() - 1; i >= 0; --i )
	{
		CNPC_SecobModportal2 *pTest = g_SecobModportal2List.m_list[i];

		if ( IsPointInCone( pTest->GetAbsOrigin(), coneOrigin, coneAxis, coneAngleCos, coneLength ) )
		{
			pList[count] = pTest;
			count++;
			if ( count >= listMax )
				break;
		}
	}

	return count;
}


ConVar	sk_SecobModportal2_health( "sk_SecobModportal2_health","0");

BEGIN_DATADESC( CNPC_SecobModportal2 )

	DEFINE_FIELD( m_hPainPartner, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_fAutoaimRadius, FIELD_FLOAT, "autoaimradius" ),
	DEFINE_KEYFIELD( m_flFieldOfView, FIELD_FLOAT, "minangle" ),
	DEFINE_KEYFIELD( m_flMinDistValidEnemy, FIELD_FLOAT, "mindist" ),
	// DEFINE_FIELD( m_bPerfectAccuracy, FIELD_BOOLEAN ),	// Don't save

	// Function Pointers
	DEFINE_THINKFUNC( SecobModportal2Think ),

	DEFINE_INPUTFUNC( FIELD_VOID, "InputTargeted", InputTargeted ),
	DEFINE_INPUTFUNC( FIELD_VOID, "InputReleased", InputReleased ),
	// Outputs
	DEFINE_OUTPUT( m_OnTargeted, "OnTargeted"),
	DEFINE_OUTPUT( m_OnReleased, "OnReleased"),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_SecobModportal2, CNPC_SecobModportal2 );



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CNPC_SecobModportal2::CNPC_SecobModportal2( void )
{
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= sk_SecobModportal2_health.GetFloat();
	m_hPainPartner	= NULL;
	g_SecobModportal2List.AddToList( this );
	m_flFieldOfView = 360;
	m_flMinDistValidEnemy = 0;
}

CNPC_SecobModportal2::~CNPC_SecobModportal2( void )
{
	g_SecobModportal2List.RemoveFromList( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::Spawn( void )
{
	Precache();

	// This is a dummy model that is never used!
	UTIL_SetSize(this, Vector(-26,-26,-26), Vector(26,26,26));

	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( BLOOD_COLOR_RED );
	ClearEffects();
	SetGravity( 0.0 );
	
//PrecacheModel( "models/obco_portal2.mdl" );
//SetModel( "models/obco_portal2.mdl" );    // set size and link into world
	
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	m_flFieldOfView = cos( DEG2RAD(m_flFieldOfView) / 2.0 );

	//Got blood?
	if ( m_spawnflags & SF_SecobModportal2_BLEED )
	{
		SetBloodColor(BLOOD_COLOR_RED);
	}
	else
	{
		SetBloodColor(DONT_BLEED);
	}

	AddFlag( FL_NPC );
	AddEFlags( EFL_NO_DISSOLVE );

	SetThink( &CNPC_SecobModportal2::SecobModportal2Think );
	SetNextThink( gpGlobals->curtime + 0.1f );

	SetSolid( SOLID_BBOX );
	if( m_spawnflags & SF_SecobModportal2_NONSOLID )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}
	
	if ( m_spawnflags & SF_SecobModportal2_VPHYSICSSHADOW )
	{
		VPhysicsInitShadow( false, false );
	}
	
	if( m_spawnflags & SF_SecobModportal2_NODAMAGE )
	{
		m_takedamage = DAMAGE_NO;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}
	//AddEffects( EF_NODRAW );

	//Check our water level
	PhysicsCheckWater();

	CapabilitiesAdd( bits_CAP_SIMPLE_RADIUS_DAMAGE );

	m_iMaxHealth = GetHealth();

	if( m_fAutoaimRadius > 0.0f )
	{
		// Make this an aimtarget, since it has some autoaim influence.
		AddFlag(FL_AIMTARGET);
	}	
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::Activate( void )
{
	BaseClass::Activate();

	if ( m_spawnflags & SF_SecobModportal2_PERFECTACC )
	{
		m_bPerfectAccuracy = true;
	}
	else
	{
		m_bPerfectAccuracy = false;
	}
}


//------------------------------------------------------------------------------
// Purpose : Override so doesn't fall to ground when killed
//------------------------------------------------------------------------------
void CNPC_SecobModportal2::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	if( GetParent() )
	{
		if( GetParent()->ClassMatches("prop_combine_ball") )
		{
			// If this SecobModportal2 is parented to a combine ball, explode the combine ball
			// and remove this SecobModportal2.
			variant_t emptyVariant;
			GetParent()->AcceptInput( "explode", this, this, emptyVariant, 0 );

			// Unhook.
			SetParent(NULL);

			UTIL_Remove(this);
			return;
		}
	}

	SetMoveType( MOVETYPE_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	UTIL_SetSize(this, vec3_origin, vec3_origin );

	SetNextThink( gpGlobals->curtime + 0.1f );
	SetThink( &CBaseEntity::SUB_Remove );
}

//------------------------------------------------------------------------------
// Purpose : Override base implimentation to let decals pass through
//			 me onto the surface beneath
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_SecobModportal2::DecalTrace( trace_t *pOldTrace, char const *decalName )
{
	int index = decalsystem->GetDecalIndexForName( decalName );
	if ( index < 0 )
		return;

	// Get direction of original trace
	Vector vTraceDir = pOldTrace->endpos - pOldTrace->startpos;
	VectorNormalize(vTraceDir);

	// Create a new trace that passes through me
	Vector vStartTrace	= pOldTrace->endpos - (1.0 * vTraceDir);
	Vector vEndTrace	= pOldTrace->endpos + (MAX_TRACE_LENGTH * vTraceDir);

	trace_t pNewTrace;
	AI_TraceLine(vStartTrace, vEndTrace, MASK_SHOT, this, COLLISION_GROUP_NONE, &pNewTrace);

	CBroadcastRecipientFilter filter;
	te->Decal( filter, 0.0, &pNewTrace.endpos, &pNewTrace.startpos,
		ENTINDEX( pNewTrace.m_pEnt ), pNewTrace.hitbox, index );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	// Get direction of original trace
	Vector vTraceDir = pTrace->endpos - pTrace->startpos;
	VectorNormalize(vTraceDir);

	// Create a new trace that passes through me
	Vector vStartTrace	= pTrace->endpos - (1.0 * vTraceDir);
	Vector vEndTrace	= pTrace->endpos + (MAX_TRACE_LENGTH * vTraceDir);

	trace_t pNewTrace;
	AI_TraceLine(vStartTrace, vEndTrace, MASK_SHOT, this, COLLISION_GROUP_NONE, &pNewTrace);

	CBaseEntity	*pEntity = pNewTrace.m_pEnt;

	// Only do this for BSP model entities
	if ( ( pEntity ) && ( pEntity->IsBSPModel() == false ) )
		return;

	BaseClass::ImpactTrace( pTrace, iDamageType, pCustomImpactName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_SecobModportal2::Classify( void )
{
	return	CLASS_SecobModportal2;
}

void CNPC_SecobModportal2::OnRestore( void )
{
	if ( m_spawnflags & SF_SecobModportal2_VPHYSICSSHADOW )
	{
		IPhysicsObject *pObject = VPhysicsGetObject();

		if ( pObject == NULL )
		{
			VPhysicsInitShadow( false, false );
		}
	}

	BaseClass::OnRestore();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::SecobModportal2Think( void )
{
	ClearCondition( COND_LIGHT_DAMAGE  );
	ClearCondition( COND_HEAVY_DAMAGE );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_SecobModportal2::CanBecomeRagdoll()
{
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_SecobModportal2::CanBeAnEnemyOf( CBaseEntity *pEnemy )
{
	static const float flFullFov = cos( DEG2RAD(360) / 2.0 );
	if ( fabsf( m_flFieldOfView - flFullFov ) > .01 )
	{
		if ( !FInViewCone( pEnemy ) )
		{
			return false;
		}
	}

	if ( m_flMinDistValidEnemy > 0 )
	{
		float distSq = ( GetAbsOrigin().AsVector2D() - pEnemy->GetAbsOrigin().AsVector2D() ).LengthSqr();
		if ( distSq < Square( m_flMinDistValidEnemy ) )
		{
			return false;
		}
	}
	return BaseClass::CanBeAnEnemyOf( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: SecobModportal2s should always report light damage if any amount of damage is taken
// Input  : fDamage - amount of damage
//			bitsDamageType - damage type
//-----------------------------------------------------------------------------
bool CNPC_SecobModportal2::IsLightDamage( const CTakeDamageInfo &info )
{
	return ( info.GetDamage() > 0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAttacker - 
//			flDamage - 
//			&vecDir - 
//			*ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	//If specified, we must be the enemy of the target
	if ( m_spawnflags & SF_SecobModportal2_ENEMYDAMAGEONLY )
	{
		CAI_BaseNPC *pInstigator = info.GetAttacker()->MyNPCPointer();

		if ( pInstigator == NULL )
			return;

		if ( pInstigator->GetEnemy() != this )
			return;
	}

	//We can bleed if we want to, we can leave decals behind...
	if ( ( m_spawnflags & SF_SecobModportal2_BLEED ) && ( m_takedamage == DAMAGE_NO ) )
	{
		TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_SecobModportal2::OnTakeDamage( const CTakeDamageInfo &info )
{
	SetNextThink( gpGlobals->curtime );

	//If specified, we must be the enemy of the target
	if ( m_spawnflags & SF_SecobModportal2_ENEMYDAMAGEONLY )
	{
		CAI_BaseNPC *pInstigator = info.GetAttacker()->MyNPCPointer();

		if ( pInstigator == NULL )
			return 0;

		if ( pInstigator->GetEnemy() != this )
			return 0;
	}
	
	//If we're a pain proxy, send the damage through
	if ( m_hPainPartner != NULL )
	{
		m_hPainPartner->TakeDamage( info );
		
		//Fire all pain indicators but take no real damage
		CTakeDamageInfo subInfo = info;
		subInfo.SetDamage( 0 );
		return BaseClass::OnTakeDamage( subInfo );
	}

	return BaseClass::OnTakeDamage( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CNPC_SecobModportal2::SetPainPartner( CBaseEntity *pOther )
{
	m_hPainPartner = pOther;
}

void CNPC_SecobModportal2::InputTargeted( inputdata_t &inputdata )
{
	m_OnTargeted.FireOutput( inputdata.pActivator, inputdata.pCaller, 0 );
}

void CNPC_SecobModportal2::InputReleased( inputdata_t &inputdata )
{
	m_OnReleased.FireOutput( inputdata.pActivator, inputdata.pCaller, 0 );
}

//SecobMod__Information  On a player touching this, portalize them!
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

void CNPC_SecobModportal2::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );
	
	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
	
		const char *PlayerSteamID = engine->GetPlayerNetworkIDString(pOther->edict()); //Finds the current players Steam ID.	
		if( PlayerSteamID == NULL)
		return;
		char Portal1Name[ 512 ];
		Q_strncpy( Portal1Name, "Portal1_" ,sizeof(Portal1Name));
		Q_strncat( Portal1Name, PlayerSteamID,sizeof(Portal1Name), COPY_ALL_CHARACTERS );
		
		// We look for Portal1 for our teleport point because we are Portal2.
		CBaseEntity *pEnt = NULL;
		pEnt = gEntList.FindEntityByName( pEnt, Portal1Name, NULL, pOther, pOther );
		if (!pEnt )
		{
			return;
		}
	
	EmitSound( "PortalPlayer.EnterPortal" );
	
	//PORTAL TRIGGER CODE.
			// Don't touch entities that came through us and haven't left us yet.
		/*EHANDLE hHandle;
		hHandle = pOther;
		if ( m_hDisabledForEntities.Find(hHandle) != m_hDisabledForEntities.InvalidIndex() )
		{
			Msg("    IGNORED\n", GetDebugName(), pOther->GetDebugName() );
			return;
		}
		Pickup_ForcePlayerToDropThisObject( pOther );*/
	
		pOther->SetGroundEntity( NULL );
	
	// Build a this --> remote transformation
		VMatrix matMyModelToWorld, matMyInverse;
		matMyModelToWorld = pOther->EntityToWorldTransform();
		MatrixInverseGeneral ( matMyModelToWorld, matMyInverse );

		// Teleport our object
		VMatrix matRemotePortalTransform = pEnt->EntityToWorldTransform();
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


		// place player at the new destination

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
		this->TestCollision( ray, pOther->PhysicsSolidMaskForEntity(), tr );

		// Teleportation caused us to hit something, deal with it.
		if ( tr.DidHit() )
		{
			
		}
		
		EmitSound( "PortalPlayer.ExitPortal" );
		
		Vector forward, right, up;

		Vector oldorigin = pOther->GetAbsOrigin();

		AngleVectors ( pOther->GetAbsAngles(), &forward, &right, &up);
		
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
								pOther->TakeDamage( CTakeDamageInfo( this, this, 1000, DMG_DISSOLVE ) );
							}
						}
					}
				}
			}
		}
	pOther->SetAbsOrigin( oldorigin );
	
	}
}