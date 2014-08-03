//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_IMPACT_DAMAGE_H
#define PHYSICS_IMPACT_DAMAGE_H
#ifdef _WIN32
#pragma once
#endif


struct impactentry_t
{
	float	impulse;
	float	damage;
};

// UNDONE: Add a flag to turn off aggregation of mass in object systems (e.g. ragdolls, vehicles)?
struct impactdamagetable_t
{
	impactentry_t	*linearTable;
	impactentry_t	*angularTable;
	int			linearCount;	// array size of linearTable
	int			angularCount;	// array size of angularTable

	float		minSpeedSqr;	// minimum squared impact speed for damage
	float		minRotSpeedSqr;
	float		minMass;		// minimum mass to do damage

	// filter out reall small objects, set all to zero to disable
	float		smallMassMax;
	float		smallMassCap;
	float		smallMassMinSpeedSqr;

	// exaggerate the effects of really large objects, set all to 1 to disable
	float		largeMassMin;
	float		largeMassScale;
	float		largeMassFallingScale;	// emphasize downward impacts so that this will kill instead of stress (we have more information here than there)
	float		myMinVelocity;			// filter out any energy lost by me unless my velocity is greater than this
};



extern impactdamagetable_t gDefaultNPCImpactDamageTable;
extern impactdamagetable_t gDefaultPlayerImpactDamageTable;
extern impactdamagetable_t gDefaultPlayerVehicleImpactDamageTable;

// NOTE Default uses default NPC table
float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageTypeOut, string_t iszDamageTableName = NULL_STRING, bool bDamageFromHeldObjects = false );

// use passes in the table
float CalculatePhysicsImpactDamage( int index, gamevcollisionevent_t *pEvent, const impactdamagetable_t &table, float energyScale, bool allowStaticDamage, int &damageTypeOut, bool bDamageFromHeldObjects = false );

struct vphysics_objectstress_t
{
	float		exertedStress;
	float		receivedStress;
	bool		hasNonStaticStress;
	bool		hasLargeObjectContact;
};

float CalculateObjectStress( IPhysicsObject *pObject, CBaseEntity *pOwnerEntity, vphysics_objectstress_t *pOutput );

#endif // PHYSICS_IMPACT_DAMAGE_H
