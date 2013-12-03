//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_SHARED_H
#define PHYSICS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

class IPhysics;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
class IPhysicsCollision;
class IPhysicsObject;
class IPhysicsObjectPairHash;
class CSoundPatch;


extern IPhysicsObject		*g_PhysWorldObject;
extern IPhysics				*physics;
extern IPhysicsCollision	*physcollision;
extern IPhysicsEnvironment	*physenv;
#ifdef PORTAL
extern IPhysicsEnvironment	*physenv_main;
#endif
extern IPhysicsSurfaceProps *physprops;
extern IPhysicsObjectPairHash *g_EntityCollisionHash;

extern const objectparams_t g_PhysDefaultObjectParams;

// Compute enough energy of a reference mass travelling at speed
// makes numbers more intuitive
#define MASS_SPEED2ENERGY(mass, speed)	((speed)*(speed)*(mass))

// energy of a 10kg mass moving at speed
#define MASS10_SPEED2ENERGY(speed)	MASS_SPEED2ENERGY(10,speed)

#define MASS_ENERGY2SPEED(mass,energy)	(FastSqrt((energy)/mass))

#define ENERGY_VOLUME_SCALE		(1.0f / 15500.0f)

#define FLUID_TIME_MAX					2.0f // keep track of last time hitting fluid for up to 2 seconds 

// VPHYSICS object game-specific flags
#define FVPHYSICS_DMG_SLICE				0x0001		// does slice damage, not just blunt damage
#define FVPHYSICS_CONSTRAINT_STATIC		0x0002		// object is constrained to the world, so it should behave like a static
#define FVPHYSICS_PLAYER_HELD			0x0004		// object is held by the player, so have a very inelastic collision response
#define FVPHYSICS_PART_OF_RAGDOLL		0x0008		// object is part of a client or server ragdoll
#define FVPHYSICS_MULTIOBJECT_ENTITY	0x0010		// object is part of a multi-object entity
#define FVPHYSICS_HEAVY_OBJECT			0x0020		// HULK SMASH! (Do large damage even if the mass is small)
#define FVPHYSICS_PENETRATING			0x0040		// This object is currently stuck inside another object
#define FVPHYSICS_NO_PLAYER_PICKUP		0x0080		// Player can't pick this up for some game rule reason
#define	FVPHYSICS_WAS_THROWN			0x0100		// Player threw this object
#define FVPHYSICS_DMG_DISSOLVE			0x0200		// does dissolve damage, not just blunt damage
#define FVPHYSICS_NO_IMPACT_DMG			0x0400		// don't do impact damage to anything
#define FVPHYSICS_NO_NPC_IMPACT_DMG		0x0800		// Don't do impact damage to NPC's. This is temporary for NPC's shooting combine balls (sjb)
#define FVPHYSICS_NO_SELF_COLLISIONS	0x8000		// don't collide with other objects that are part of the same entity

//-----------------------------------------------------------------------------
// Purpose: A little cache of current objects making noises
//-----------------------------------------------------------------------------
struct friction_t
{
	CSoundPatch	*patch;
	CBaseEntity	*pObject;
	float		flLastUpdateTime;
	float		flLastEffectTime;
};

enum
{
	TOUCH_START=0,
	TOUCH_END,
};

struct touchevent_t
{
	CBaseEntity *pEntity0;
	CBaseEntity *pEntity1;
	int			touchType;
	Vector		endPoint;	//sv
	Vector		normal;		//sv
};

struct fluidevent_t
{
	EHANDLE			hEntity;
	float			impactTime;
};

void PhysFrictionSound( CBaseEntity *pEntity, IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit );
void PhysFrictionSound( CBaseEntity *pEntity, IPhysicsObject *pObject, const char *pSoundName, HSOUNDSCRIPTHANDLE& handle, float flVolume );
void PhysCleanupFrictionSounds( CBaseEntity *pEntity );
void PhysFrictionEffect( Vector &vecPos, Vector vecVel, float energy, int surfaceProps, int surfacePropsHit );

// Convenience routine
// ORs gameFlags with the physics object's current game flags
inline unsigned short PhysSetGameFlags( IPhysicsObject *pPhys, unsigned short gameFlags )
{
	unsigned short flags = pPhys->GetGameFlags();
	flags |= gameFlags;
	pPhys->SetGameFlags( flags );
	
	return flags;
}
// mask off gameFlags
inline unsigned short PhysClearGameFlags( IPhysicsObject *pPhys, unsigned short gameFlags )
{
	unsigned short flags = pPhys->GetGameFlags();
	flags &= ~gameFlags;
	pPhys->SetGameFlags( flags );
	
	return flags;
}


// Create a vphysics object based on a model
IPhysicsObject *PhysModelCreate( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles, solid_t *pSolid = NULL );

IPhysicsObject *PhysModelCreateBox( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, bool isStatic );
IPhysicsObject *PhysModelCreateOBB( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, const QAngle &angle, bool isStatic );

// Create a vphysics object based on a BSP model (unmoveable)
IPhysicsObject *PhysModelCreateUnmoveable( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles );

// Create a vphysics object based on an existing collision model
IPhysicsObject *PhysModelCreateCustom( CBaseEntity *pEntity, const CPhysCollide *pModel, const Vector &origin, const QAngle &angles, const char *pName, bool isStatic, solid_t *pSolid = NULL );

// Create a bbox collision model (these may be shared among entities, they are auto-deleted at end of level. do not manage)
CPhysCollide *PhysCreateBbox( const Vector &mins, const Vector &maxs );

// Create a vphysics sphere object
IPhysicsObject *PhysSphereCreate( CBaseEntity *pEntity, float radius, const Vector &origin, solid_t &solid );

// Destroy a physics object created using PhysModelCreate...()
void PhysDestroyObject( IPhysicsObject *pObject, CBaseEntity *pEntity = NULL );

void PhysDisableObjectCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );
void PhysDisableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );
void PhysDisableEntityCollisions( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
void PhysEnableObjectCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );
void PhysEnableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );
void PhysEnableEntityCollisions( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );
bool PhysEntityCollisionsAreDisabled( CBaseEntity *pEntity0, CBaseEntity *pEntity1 );

// create the world physics objects
IPhysicsObject *PhysCreateWorld_Shared( CBaseEntity *pWorld, vcollide_t *pWorldCollide, const objectparams_t &defaultParams );

// parse the parameters for a single solid from the model's collision data
bool PhysModelParseSolid( solid_t &solid, CBaseEntity *pEntity, int modelIndex );
// parse the parameters for a solid matching a particular index
bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, int modelIndex, int solidIndex );

void PhysParseSurfaceData( class IPhysicsSurfaceProps *pProps, class IFileSystem *pFileSystem );

// fill out this solid_t with the AABB defaults (high inertia/no rotation)
void PhysGetDefaultAABBSolid( solid_t &solid );

// Compute an output velocity based on sliding along the current contact points 
// in the closest direction toward inputVelocity.
void PhysComputeSlideDirection( IPhysicsObject *pPhysics, const Vector &inputVelocity, const AngularImpulse &inputAngularVelocity, 
							   Vector *pOutputVelocity, Vector *pOutputAngularVelocity, float minMass );

void PhysForceClearVelocity( IPhysicsObject *pPhys );
bool PhysHasContactWithOtherInDirection( IPhysicsObject *pPhysics, const Vector &dir );

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* PhysicsGameSystem();

#endif // PHYSICS_SHARED_H
