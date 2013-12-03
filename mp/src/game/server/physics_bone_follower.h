//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_BONE_FOLLOWER_H
#define PHYSICS_BONE_FOLLOWER_H
#ifdef _WIN32
#pragma once
#endif

class CBoneFollower;

//
// To use bone followers in an entity, contain a CBoneFollowerManager in it. Then:
//		- Call InitBoneFollowers() in the entity's CreateVPhysics().
//		- Call UpdateBoneFollowers() after you move your bones.
//		- Call DestroyBoneFollowers() when your entity's removed

struct physfollower_t
{
	DECLARE_SIMPLE_DATADESC();
	int boneIndex;
	CHandle<CBoneFollower> hFollower;
};

struct vcollide_t;

// create a manager and a list of followers directly from a ragdoll
void CreateBoneFollowersFromRagdoll( CBaseAnimating *pEntity, class CBoneFollowerManager *pManager, vcollide_t *pCollide );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBoneFollowerManager
{
	DECLARE_SIMPLE_DATADESC();
public:
	CBoneFollowerManager();
	~CBoneFollowerManager();

	// Use either of these to create the bone followers in your entity's CreateVPhysics()
	void InitBoneFollowers( CBaseAnimating *pParentEntity, int iNumBones, const char **pFollowerBoneNames );
	void AddBoneFollower( CBaseAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid = NULL );	// Adds a single bone follower

	// Call this after you move your bones
	void UpdateBoneFollowers( CBaseAnimating *pParentEntity );

	// Call this when your entity's removed
	void DestroyBoneFollowers( void );

	physfollower_t *GetBoneFollower( int iFollowerIndex );
	int				GetBoneFollowerIndex( CBoneFollower *pFollower );
	int				GetNumBoneFollowers( void ) const { return m_iNumBones; }

private:
	bool CreatePhysicsFollower( CBaseAnimating *pParentEntity, physfollower_t &follow, const char *pBoneName, solid_t *pSolid );

private:
	int							m_iNumBones;
	CUtlVector<physfollower_t>	m_physBones;
};


class CBoneFollower : public CBaseEntity
{
	DECLARE_CLASS( CBoneFollower, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
public:
	// CBaseEntity
	void VPhysicsUpdate( IPhysicsObject *pPhysics );
	int  UpdateTransmitState(void);

	// NOTE: These are forwarded to the parent object!
	void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	void VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );
	void VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );

	bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	int	 ObjectCaps( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	void Touch( CBaseEntity *pOther );

	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	// locals
	bool Init( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );
	void UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval );
	void SetTraceData( int physicsBone, int hitGroup );

	// factory
	static CBoneFollower *Create( CBaseEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );

private:
	CNetworkVar( int, m_modelIndex );
	CNetworkVar( int, m_solidIndex );
	int		m_physicsBone;
	int		m_hitGroup;
};

#endif // PHYSICS_BONE_FOLLOWER_H
