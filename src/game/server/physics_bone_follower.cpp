//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "bone_setup.h"
#include "physics_bone_follower.h"
#include "vcollide_parse.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


BEGIN_SIMPLE_DATADESC( physfollower_t )
DEFINE_FIELD( boneIndex,			FIELD_INTEGER	),
DEFINE_FIELD( hFollower,			FIELD_EHANDLE	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( CBoneFollowerManager )
DEFINE_GLOBAL_FIELD( m_iNumBones,			FIELD_INTEGER	),
DEFINE_GLOBAL_UTLVECTOR( m_physBones,		FIELD_EMBEDDED	),
END_DATADESC()

//================================================================================================================
// BONE FOLLOWER MANAGER
//================================================================================================================
CBoneFollowerManager::CBoneFollowerManager()
{
	m_iNumBones = 0;
}

CBoneFollowerManager::~CBoneFollowerManager()
{
	// if this fires then someone isn't destroying their bonefollowers in UpdateOnRemove
	Assert(m_iNumBones==0);
	DestroyBoneFollowers();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//			iNumBones - 
//			**pFollowerBoneNames - 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::InitBoneFollowers( CBaseAnimating *pParentEntity, int iNumBones, const char **pFollowerBoneNames )
{
	m_iNumBones = iNumBones;
	m_physBones.EnsureCount( iNumBones );

	// Now init all the bones
	for ( int i = 0; i < iNumBones; i++ )
	{
		CreatePhysicsFollower( pParentEntity, m_physBones[i], pFollowerBoneNames[i], NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::AddBoneFollower( CBaseAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid )
{
	m_iNumBones++;

	int iIndex = m_physBones.AddToTail();
	CreatePhysicsFollower( pParentEntity, m_physBones[iIndex], pFollowerBoneName, pSolid );
}

// walk the hitboxes and find the first one that is attached to the physics bone in question
// return the hitgroup of that box
static int HitGroupFromPhysicsBone( CBaseAnimating *pAnim, int physicsBone )
{
	CStudioHdr *pStudioHdr = pAnim->GetModelPtr( );
	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnim->m_nHitboxSet );
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		if ( pStudioHdr->pBone( set->pHitbox(i)->bone )->physicsbone == physicsBone )
		{
			return set->pHitbox(i)->group;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &follow - 
//			*pBoneName - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBoneFollowerManager::CreatePhysicsFollower( CBaseAnimating *pParentEntity, physfollower_t &follow, const char *pBoneName, solid_t *pSolid )
{
	CStudioHdr *pStudioHdr = pParentEntity->GetModelPtr();
	matrix3x4_t boneToWorld;
	solid_t solidTmp;

	Vector bonePosition;
	QAngle boneAngles;

	int boneIndex = Studio_BoneIndexByName( pStudioHdr, pBoneName );

	if ( boneIndex >= 0 )
	{
		mstudiobone_t *pBone = pStudioHdr->pBone( boneIndex );

		int physicsBone = pBone->physicsbone;
		if ( !pSolid )
		{
			if ( !PhysModelParseSolidByIndex( solidTmp, pParentEntity, pParentEntity->GetModelIndex(), physicsBone ) )
				return false;
			pSolid = &solidTmp;
		}

		// fixup in case ragdoll is assigned to a parent of the requested follower bone
		follow.boneIndex = Studio_BoneIndexByName( pStudioHdr, pSolid->name );
		if ( follow.boneIndex < 0 )
		{
			follow.boneIndex = boneIndex;
		}

		pParentEntity->GetBoneTransform( follow.boneIndex, boneToWorld );
		MatrixAngles( boneToWorld, boneAngles, bonePosition );

		follow.hFollower = CBoneFollower::Create( pParentEntity, STRING(pParentEntity->GetModelName()), *pSolid, bonePosition, boneAngles );
		follow.hFollower->SetTraceData( physicsBone, HitGroupFromPhysicsBone( pParentEntity, physicsBone ) );
		follow.hFollower->SetBlocksLOS( pParentEntity->BlocksLOS() );
		return true;
	}
	else
	{
		Warning( "ERROR: Tried to create bone follower on invalid bone %s\n", pBoneName );
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::UpdateBoneFollowers( CBaseAnimating *pParentEntity )
{
	if ( m_iNumBones )
	{
		matrix3x4_t boneToWorld;
		Vector bonePosition;
		QAngle boneAngles;
		for ( int i = 0; i < m_iNumBones; i++ )
		{
			if ( !m_physBones[i].hFollower )
				continue;

			pParentEntity->GetBoneTransform( m_physBones[i].boneIndex, boneToWorld );
			MatrixAngles( boneToWorld, boneAngles, bonePosition );
			m_physBones[i].hFollower->UpdateFollower( bonePosition, boneAngles, 0.1 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollowerManager::DestroyBoneFollowers( void )
{
	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		UTIL_Remove( m_physBones[i].hFollower );
		m_physBones[i].hFollower = NULL;
	}

	m_physBones.Purge();
	m_iNumBones = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
physfollower_t *CBoneFollowerManager::GetBoneFollower( int iFollowerIndex )
{
	Assert( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones );
	if ( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones )
		return &m_physBones[iFollowerIndex];
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieve the index for a supplied bone follower
// Input  : *pFollower - Bone follower to look up
// Output : -1 if not found, otherwise the index of the bone follower
//-----------------------------------------------------------------------------
int CBoneFollowerManager::GetBoneFollowerIndex( CBoneFollower *pFollower )
{
	if ( pFollower == NULL )
		return -1;

	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		if ( m_physBones[i].hFollower == pFollower )
			return i;
	}
	
	return -1;
}

//================================================================================================================
// BONE FOLLOWER
//================================================================================================================

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBoneFollower )

	DEFINE_FIELD( m_modelIndex,	FIELD_MODELINDEX ),
	DEFINE_FIELD( m_solidIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( m_physicsBone,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hitGroup,	FIELD_INTEGER ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CBoneFollower, DT_BoneFollower )
	SendPropModelIndex(SENDINFO(m_modelIndex)),
	SendPropInt(SENDINFO(m_solidIndex), 6, SPROP_UNSIGNED ),
END_SEND_TABLE()


bool CBoneFollower::Init( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	SetOwnerEntity( pOwner );
	UTIL_SetModel( this, pModelName );

	AddEffects( EF_NODRAW ); // invisible

	m_modelIndex = modelinfo->GetModelIndex( pModelName );
	m_solidIndex = solid.index;
	SetAbsOrigin( position );
	SetAbsAngles( orientation );
	SetMoveType( MOVETYPE_PUSH );
	SetSolid( SOLID_VPHYSICS );
	SetCollisionGroup( pOwner->GetCollisionGroup() );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	solid.params.pGameData = (void *)this;
	IPhysicsObject *pPhysics = VPhysicsInitShadow( false, false, &solid );
	if ( !pPhysics )
		return false;

	// we can't use the default model bounds because each entity is only one bone of the model
	// so compute the OBB of the physics model and use that.
	Vector mins, maxs;
	physcollision->CollideGetAABB( &mins, &maxs, pPhysics->GetCollide(), vec3_origin, vec3_angle );
	SetCollisionBounds( mins, maxs );

	pPhysics->SetCallbackFlags( pPhysics->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH );
	pPhysics->EnableGravity( false );
	// This is not a normal shadow controller that is trying to go to a space occupied by an entity in the game physics
	// This entity is not running PhysicsPusher(), so Vphysics is supposed to move it
	// This line of code informs vphysics of that fact
	if ( pOwner->IsNPC() )
	{
		pPhysics->GetShadowController()->SetPhysicallyControlled( true );
	}

	return true;
}

int CBoneFollower::UpdateTransmitState()
{
	// Send to the client for client-side collisions and visualization
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

void CBoneFollower::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	Vector origin;
	QAngle angles;

	pPhysics->GetPosition( &origin, &angles );

	SetAbsOrigin( origin );
	SetAbsAngles( angles );
}

// a little helper class to temporarily change the physics object
// for an entity - and change it back when it goes out of scope.
class CPhysicsSwapTemp
{
public:
	CPhysicsSwapTemp( CBaseEntity *pEntity, IPhysicsObject *pTmpPhysics )
	{
		Assert(pEntity);
		Assert(pTmpPhysics);
		m_pEntity = pEntity;
		m_pPhysics = m_pEntity->VPhysicsGetObject();
		if ( m_pPhysics )
		{
			m_pEntity->VPhysicsSwapObject( pTmpPhysics );
		}
		else
		{
			m_pEntity->VPhysicsSetObject( pTmpPhysics );
		}
	}
	~CPhysicsSwapTemp()
	{
		m_pEntity->VPhysicsSwapObject( m_pPhysics );
	}

private:
	CBaseEntity *m_pEntity;
	IPhysicsObject *m_pPhysics;
};


void CBoneFollower::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pEvent->pObjects[index] );
		pOwner->VPhysicsCollision( index, pEvent );
	}
}

void CBoneFollower::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pEvent->pObjects[index] );
		pOwner->VPhysicsShadowCollision( index, pEvent );
	}
}

void CBoneFollower::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		CPhysicsSwapTemp tmp(pOwner, pObject );
		pOwner->VPhysicsFriction( pObject, energy, surfaceProps, surfacePropsHit );
	}
}

bool CBoneFollower::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
	Assert( pCollide && pCollide->solidCount > m_solidIndex );

	UTIL_ClearTrace( trace );

	physcollision->TraceBox( ray, pCollide->solids[m_solidIndex], GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.fraction >= 1 )
		return false;

	// return owner as trace hit
	trace.m_pEnt = GetOwnerEntity();
	trace.hitgroup = m_hitGroup;
	trace.physicsbone = m_physicsBone;
	return true;
}

void CBoneFollower::UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval )
{
	// UNDONE: Shadow update needs timing info?
	VPhysicsGetObject()->UpdateShadow( position, orientation, false, flInterval );
}

void CBoneFollower::SetTraceData( int physicsBone, int hitGroup )
{
	m_hitGroup = hitGroup;
	m_physicsBone = physicsBone;
}

CBoneFollower *CBoneFollower::Create( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	CBoneFollower *pFollower = (CBoneFollower *)CreateEntityByName( "phys_bone_follower" );
	if ( pFollower )
	{
		pFollower->Init( pOwner, pModelName, solid, position, orientation );
	}
	return pFollower;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBoneFollower::ObjectCaps() 
{ 
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		if( pOwner->m_iGlobalname != NULL_STRING )
		{
			int caps = BaseClass::ObjectCaps() | pOwner->ObjectCaps();
			caps &= ~FCAP_ACROSS_TRANSITION;
			return caps;
		}
	}

	return BaseClass::ObjectCaps();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBoneFollower::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->Use( pActivator, pCaller, useType, value );
		return;
	}

	BaseClass::Use( pActivator, pCaller, useType, value );
}

//-----------------------------------------------------------------------------
// Purpose: Pass on Touch calls to the entity we're following
//-----------------------------------------------------------------------------
void CBoneFollower::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		//TODO: fill in the touch trace with the hitbox number associated with this bone
		pOwner->Touch( pOther );
		return;
	}

	BaseClass::Touch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Pass on trace attack calls to the entity we're following
//-----------------------------------------------------------------------------
void CBoneFollower::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DispatchTraceAttack( info, vecDir, ptr, pAccumulator );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

LINK_ENTITY_TO_CLASS( phys_bone_follower, CBoneFollower );



// create a manager and a list of followers directly from a ragdoll
void CreateBoneFollowersFromRagdoll( CBaseAnimating *pEntity, CBoneFollowerManager *pManager, vcollide_t *pCollide )
{
	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !strcmpi( pBlock, "solid" ) )
		{
			solid_t solid;

			pParse->ParseSolid( &solid, NULL );
			// collisions are off by default, turn them on
			solid.params.enableCollisions = true;
			solid.params.pName = STRING(pEntity->GetModelName());

			pManager->AddBoneFollower( pEntity, solid.name, &solid );
		}
		else
		{
			pParse->SkipBlock();
		}
	}
}
