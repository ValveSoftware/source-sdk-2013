//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "physics_impact_damage.h"
#include "shareddefs.h"
#include "vphysics/friction.h"
#include "vphysics/player_controller.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//==============================================================================================
// PLAYER PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t playerLinearTable[] =
{
	{ 150*150, 5 },
	{ 250*250, 10 },
	{ 450*450, 20 },
	{ 550*550, 50 },
	{ 700*700, 100 },
	{ 1000*1000, 500 },
};

static impactentry_t playerAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 20 },
	{ 200*200, 50 },
	{ 300*300, 500 },
};

impactdamagetable_t gDefaultPlayerImpactDamageTable =
{
	playerLinearTable,
	playerAngularTable,

	ARRAYSIZE(playerLinearTable),
	ARRAYSIZE(playerAngularTable),

	24*24.0f,	// minimum linear speed
	360*360.0f,	// minimum angular speed
	2.0f,		// can't take damage from anything under 2kg

	5.0f,		// anything less than 5kg is "small"
	5.0f,		// never take more than 5 pts of damage from anything under 5kg
	36*36.0f,	// <5kg objects must go faster than 36 in/s to do damage

	0.0f,		// large mass in kg (no large mass effects)
	1.0f,		// large mass scale
	2.0f,		// large mass falling scale
	320.0f,		// min velocity for player speed to cause damage

};

//==============================================================================================
// PLAYER-IN-VEHICLE PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t playerVehicleLinearTable[] =
{
	{ 450*450, 5 },
	{ 600*600, 10 },
	{ 700*700, 25 },
	{ 1000*1000, 50 },
	{ 1500*1500, 100 },
	{ 2000*2000, 500 },
};

static impactentry_t playerVehicleAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 20 },
	{ 200*200, 50 },
	{ 300*300, 500 },
};

impactdamagetable_t gDefaultPlayerVehicleImpactDamageTable =
{
	playerVehicleLinearTable,
	playerVehicleAngularTable,

	ARRAYSIZE(playerVehicleLinearTable),
	ARRAYSIZE(playerVehicleAngularTable),

	24*24,		// minimum linear speed
	360*360,	// minimum angular speed
	80,			// can't take damage from anything under 80 kg

	150,		// anything less than 150kg is "small"
	5,			// never take more than 5 pts of damage from anything under 150kg
	36*36,		// <150kg objects must go faster than 36 in/s to do damage

	0,			// large mass in kg (no large mass effects)
	1.0f,		// large mass scale
	1.0f,		// large mass falling scale
	0.0f,		// min vel
};


//==============================================================================================
// NPC PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t npcLinearTable[] =
{
	{ 150*150, 5 },
	{ 250*250, 10 },
	{ 350*350, 50 },
	{ 500*500, 100 },
	{ 1000*1000, 500 },
};

static impactentry_t npcAngularTable[] =
{
	{ 100*100, 10 },
	{ 150*150, 25 },
	{ 200*200, 50 },
	{ 250*250, 500 },
};

impactdamagetable_t gDefaultNPCImpactDamageTable =
{
	npcLinearTable,
	npcAngularTable,
	
	ARRAYSIZE(npcLinearTable),
	ARRAYSIZE(npcAngularTable),

	24*24,		// minimum linear speed squared
	360*360,	// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	2,			// can't take damage from anything under 2kg

	5,			// anything less than 5kg is "small"
	5,			// never take more than 5 pts of damage from anything under 5kg
	36*36,		// <5kg objects must go faster than 36 in/s to do damage

	VPHYSICS_LARGE_OBJECT_MASS,		// large mass in kg 
	4,			// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
	5,			// large mass falling scale (emphasize falling/crushing damage over sideways impacts since the stress will kill you anyway)
	0.0f,		// min vel
};

//==============================================================================================
// GLASS DAMAGE TABLE
//==============================================================================================
static impactentry_t glassLinearTable[] =
{
	{ 25*25, 10 },
	{ 50*50, 20 },
	{ 100*100, 50 },
	{ 200*200, 75 },
	{ 500*500, 100 },
	{ 250*250, 500 },
};

static impactentry_t glassAngularTable[] =
{
	{ 50*50, 25 },
	{ 100*100, 50 },
	{ 200*200, 100 },
	{ 250*250, 500 },
};

impactdamagetable_t gGlassImpactDamageTable =
{
	glassLinearTable,
	glassAngularTable,
	
	ARRAYSIZE(glassLinearTable),
	ARRAYSIZE(glassAngularTable),

	8*8,		// minimum linear speed squared
	360*360,	// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	2,			// can't take damage from anything under 2kg

	1,			// anything less than 1kg is "small"
	10,			// never take more than 10 pts of damage from anything under 1kg
	8*8,		// <1kg objects must go faster than 8 in/s to do damage

	50,			// large mass in kg 
	4,			// large mass scale (anything over 50kg does 4X as much energy to read from damage table)
	0.0f,		// min vel
};

//==============================================================================================
// PHYSICS TABLE NAMES
//==============================================================================================
struct damagetable_t
{
	const char			*pszTableName;
	impactdamagetable_t	*pTable;
};

static damagetable_t gDamageTableRegistry[] =
{
	{
		"player",
		&gDefaultPlayerImpactDamageTable,
	},
	{
		"player_vehicle",
		&gDefaultPlayerVehicleImpactDamageTable,
	},
	{
		"npc",
		&gDefaultNPCImpactDamageTable,
	},
	{
		"glass",
		&gGlassImpactDamageTable,
	},
};

//==============================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float ReadDamageTable( impactentry_t *pTable, int tableCount, float impulse, bool bDebug )
{
	if ( pTable )
	{
		int i;
		for ( i = 0; i < tableCount; i++ )
		{
			if ( impulse < pTable[i].impulse )
				break;
		}
		if ( i > 0 )
		{
			i--;
			if ( bDebug )
			{
				Msg("Damage %.0f, energy %.0f\n", pTable[i].damage, FastSqrt(impulse) );
			}
			return pTable[i].damage;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CalculatePhysicsImpactDamage( int index, gamevcollisionevent_t *pEvent, const impactdamagetable_t &table, float energyScale, bool allowStaticDamage, int &damageType, bool bDamageFromHeldObjects )
{
	damageType = DMG_CRUSH;
	int otherIndex = !index;

	// UNDONE: Expose a flag for self-inflicted damage?  Can't think of a valid case so far.
	if ( pEvent->pEntities[0] == pEvent->pEntities[1] )
		return 0;

	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_NO_NPC_IMPACT_DMG )
	{
		if( pEvent->pEntities[index]->IsNPC() || pEvent->pEntities[index]->IsPlayer() )
		{
			return 0;
		}
	}

	// use implicit velocities on ragdolls since they may have high constraint velocities that aren't actually executed, just pushed through contacts
	if (( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL) && pEvent->pEntities[index]->IsPlayer() )
	{
		pEvent->pObjects[otherIndex]->GetImplicitVelocity( &pEvent->preVelocity[otherIndex], &pEvent->preAngularVelocity[otherIndex] );
	}

	// Dissolving impact damage results in death always.
	if ( ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_DMG_DISSOLVE ) && 
			!pEvent->pEntities[index]->IsEFlagSet(EFL_NO_DISSOLVE) )
	{
		damageType |= DMG_DISSOLVE;
		return 1000;
	}

	if ( energyScale <= 0.0f )
		return 0;

	const int gameFlagsNoDamage = FVPHYSICS_CONSTRAINT_STATIC | FVPHYSICS_NO_IMPACT_DMG;

	// NOTE: Crushing damage is handled by stress calcs in vphysics update functions, this is ONLY impact damage
	// this is a non-moving object due to a constraint - no damage
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & gameFlagsNoDamage )
		return 0;

	// If it doesn't take damage from held objects and the object is being held - no damage
	if ( !bDamageFromHeldObjects && ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
	{
		// If it doesn't take damage from held objects - no damage
		if ( !bDamageFromHeldObjects )
			return 0;
	}

	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY )
	{
		// UNDONE: Add up mass here for car wheels and prop_ragdoll pieces?
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = pEvent->pEntities[otherIndex]->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i]->GetGameFlags() & gameFlagsNoDamage )
				return 0;
		}
	}

	if ( pEvent->pObjects[index]->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		// players can't damage held objects
		if ( pEvent->pEntities[otherIndex]->IsPlayer() )
			return 0;

		allowStaticDamage = false;
	}

#if 0
	{
		PhysGetDamageInflictorVelocityStartOfFrame( pEvent->pObjects[otherIndex], pEvent->preVelocity[otherIndex], pEvent->preAngularVelocity[otherIndex] );
	}
#endif

	float otherSpeedSqr = pEvent->preVelocity[otherIndex].LengthSqr();
	float otherAngSqr = 0;
	
	// factor in angular for sharp objects
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_DMG_SLICE )
	{
		otherAngSqr = pEvent->preAngularVelocity[otherIndex].LengthSqr();
	}

	float otherMass = pEvent->pObjects[otherIndex]->GetMass();

	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		if ( gpGlobals->maxClients == 1 )
		{
			// if the player is holding the object, use it's real mass (player holding reduced the mass)
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				otherMass = pPlayer->GetHeldObjectMass( pEvent->pObjects[otherIndex] );
			}
		}
	}

	// NOTE: sum the mass of each object in this system for the purpose of damage
	if ( pEvent->pEntities[otherIndex] && (pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY) )
	{
		otherMass = PhysGetEntityMass( pEvent->pEntities[otherIndex] );
	}
	
	if ( pEvent->pObjects[otherIndex]->GetGameFlags() & FVPHYSICS_HEAVY_OBJECT )
	{
		otherMass = table.largeMassMin;
		if ( energyScale < 2.0f )
		{
			energyScale = 2.0f;
		}
	}

	// UNDONE: allowStaticDamage is a hack - work out some method for 
	// breakable props to impact the world and break!!
	if ( !allowStaticDamage )
	{
		if ( otherMass < table.minMass )
			return 0;
		// check to see if the object is small
		if ( otherMass < table.smallMassMax && otherSpeedSqr < table.smallMassMinSpeedSqr )
			return 0;
		
		if ( otherSpeedSqr < table.minSpeedSqr && otherAngSqr < table.minRotSpeedSqr )
			return 0;
	}

	// Add extra oomph for floating objects
	if ( pEvent->pEntities[index]->IsFloating() && !pEvent->pEntities[otherIndex]->IsWorld() )
	{
		if ( energyScale < 3.0f )
		{
			energyScale = 3.0f;
		}
	}

	float damage = 0;
	bool bDebug = false;//(&table == &gDefaultPlayerImpactDamageTable);

	// don't ever take spin damage from slowly spinning objects
	if ( otherAngSqr > table.minRotSpeedSqr )
	{
		Vector otherInertia = pEvent->pObjects[otherIndex]->GetInertia();
		float angularMom = DotProductAbs( otherInertia, pEvent->preAngularVelocity[otherIndex] );
		damage = ReadDamageTable( table.angularTable, table.angularCount, angularMom * energyScale, bDebug );
		if ( damage > 0 )
		{
//			Msg("Spin : %.1f, Damage %.0f\n", FastSqrt(angularMom), damage );
			damageType |= DMG_SLASH;
		}
	}
	
	float deltaV = pEvent->preVelocity[index].Length() - pEvent->postVelocity[index].Length();
	float mass = pEvent->pObjects[index]->GetMass();

	// If I lost speed, and I lost less than min velocity, then filter out this energy
	if ( deltaV > 0 && deltaV < table.myMinVelocity )
	{
		deltaV = 0;
	}
	float eliminatedEnergy = deltaV * deltaV * mass;

	deltaV = pEvent->preVelocity[otherIndex].Length() - pEvent->postVelocity[otherIndex].Length();
	float otherEliminatedEnergy = deltaV * deltaV * otherMass;
	
	// exaggerate the effects of really large objects
	if ( otherMass >= table.largeMassMin )
	{
		otherEliminatedEnergy *= table.largeMassScale;
		float dz = pEvent->preVelocity[otherIndex].z - pEvent->postVelocity[otherIndex].z;

		if ( deltaV > 0 && dz < 0 && pEvent->preVelocity[otherIndex].z < 0 )
		{
			float factor = fabs(dz / deltaV);
			otherEliminatedEnergy *= (1 + factor * (table.largeMassFallingScale - 1.0f));
		}
	}

	eliminatedEnergy += otherEliminatedEnergy;

	// now in units of this character's speed squared
	float invMass = pEvent->pObjects[index]->GetInvMass();
	if ( !pEvent->pObjects[index]->IsMoveable() )
	{
		// inv mass is zero, but impact damage is enabled on this
		// prop, so recompute:
		invMass = 1.0f / pEvent->pObjects[index]->GetMass();
	}
	else if ( pEvent->pObjects[index]->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		if ( gpGlobals->maxClients == 1 )
		{
			// if the player is holding the object, use it's real mass (player holding reduced the mass)
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				float mass = pPlayer->GetHeldObjectMass( pEvent->pObjects[index] );
				if ( mass > 0 )
				{
					invMass = 1.0f / mass;
				}
			}
		}
	}

	eliminatedEnergy *= invMass * energyScale;
	
	damage += ReadDamageTable( table.linearTable, table.linearCount, eliminatedEnergy, bDebug );

	if ( !pEvent->pObjects[otherIndex]->IsStatic() && otherMass < table.smallMassMax && table.smallMassCap > 0 )
	{
		damage = clamp( damage, 0.f, table.smallMassCap );
	}

	return damage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageType, string_t iszDamageTableName, bool bDamageFromHeldObjects )
{
	// If we have a specified damage table, find it and use it instead
	if ( iszDamageTableName != NULL_STRING )
	{
		for ( int i = 0; i < ARRAYSIZE(gDamageTableRegistry); i++ )
		{
			if ( !Q_strcmp( gDamageTableRegistry[i].pszTableName, STRING(iszDamageTableName) ) )
				return CalculatePhysicsImpactDamage( index, pEvent, *(gDamageTableRegistry[i].pTable), energyScale, allowStaticDamage, damageType, bDamageFromHeldObjects );
		}

		Warning("Failed to find custom physics damage table name: %s\n", STRING(iszDamageTableName) );
	}

	return CalculatePhysicsImpactDamage( index, pEvent, gDefaultNPCImpactDamageTable, energyScale, allowStaticDamage, damageType, bDamageFromHeldObjects );
}

static bool IsPhysicallyControlled( CBaseEntity *pEntity, IPhysicsObject *pPhysics )
{
	bool isPhysical = false;
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		isPhysical = true;
	}
	else
	{
		if ( pPhysics->GetShadowController() )
		{
			isPhysical = pPhysics->GetShadowController()->IsPhysicallyControlled();
		}
	}
	return isPhysical;
}
float CalculateObjectStress( IPhysicsObject *pObject, CBaseEntity *pInputOwnerEntity, vphysics_objectstress_t *pOutput )
{
	CUtlVector< CBaseEntity * > pObjectList;
	CUtlVector< Vector >		objectForce;
	bool hasLargeObject = false;

	// add a slot for static objects
	pObjectList.AddToTail( NULL );
	objectForce.AddToTail( vec3_origin );
	// add a slot for friendly objects
	pObjectList.AddToTail( NULL );
	objectForce.AddToTail( vec3_origin );

	CBaseCombatCharacter *pBCC = pInputOwnerEntity->MyCombatCharacterPointer();

	IPhysicsFrictionSnapshot *pSnapshot = pObject->CreateFrictionSnapshot();
	float objMass = pObject->GetMass();
	while ( pSnapshot->IsValid() )
	{
		float force = pSnapshot->GetNormalForce();
		if ( force > 0.0f )
		{
			IPhysicsObject *pOther = pSnapshot->GetObject(1);
			CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
			if ( !pOtherEntity )
			{
				// object was just deleted, but we still have a contact point this frame...
				// just assume it came from the world.
				pOtherEntity = GetWorldEntity();
			}
			CBaseEntity *pOtherOwner = pOtherEntity;
			if ( pOtherEntity->GetOwnerEntity() )
			{
				pOtherOwner = pOtherEntity->GetOwnerEntity();
			}

			int outIndex = 0;
			if ( !pOther->IsMoveable() )
			{
				outIndex = 0;
			}
			// NavIgnored objects are often being pushed by a friendly
			else if ( pBCC && (pBCC->IRelationType( pOtherOwner ) == D_LI || pOtherEntity->IsNavIgnored()) )
			{
				outIndex = 1;
			}
			// player held objects do no stress
			else if ( pOther->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				outIndex = 1;
			}
			else
			{
				if ( pOther->GetMass() >= VPHYSICS_LARGE_OBJECT_MASS )
				{
					if ( pInputOwnerEntity->GetGroundEntity() != pOtherEntity)
					{
						hasLargeObject = true;
					}
				}
				// moveable, non-friendly
				
				// aggregate contacts over each object to avoid greater stress in multiple contact cases
				// NOTE: Contacts should be in order, so this shouldn't ever search, but just in case
				outIndex = pObjectList.Count();
				for ( int i = pObjectList.Count()-1; i >= 2; --i )
				{
					if ( pObjectList[i] == pOtherOwner )
					{
						outIndex = i;
						break;
					}
				}
				if ( outIndex == pObjectList.Count() )
				{
					pObjectList.AddToTail( pOtherOwner );
					objectForce.AddToTail( vec3_origin );
				}
			}

			if ( outIndex != 0 && pInputOwnerEntity->GetMoveType() != MOVETYPE_VPHYSICS && !IsPhysicallyControlled(pOtherEntity, pOther) )
			{
				// UNDONE: Test this!  This is to remove any shadow/shadow stress.  The game should handle this with blocked/damage
				force = 0.0f;
			}

			Vector normal;
			pSnapshot->GetSurfaceNormal( normal );
			objectForce[outIndex] += normal * force;
		}
		pSnapshot->NextFrictionData();
	}
	pObject->DestroyFrictionSnapshot( pSnapshot );
	pSnapshot = NULL;

	// clear out all friendly force
	objectForce[1].Init();

	float sum = 0;
	Vector negativeForce = vec3_origin;
	Vector positiveForce = vec3_origin;

	Assert( pObjectList.Count() == objectForce.Count() );
	for ( int objectIndex = pObjectList.Count()-1; objectIndex >= 0; --objectIndex )
	{
		sum += objectForce[objectIndex].Length();
		for ( int i = 0; i < 3; i++ )
		{
			if ( objectForce[objectIndex][i] < 0 )
			{
				negativeForce[i] -= objectForce[objectIndex][i];
			}
			else
			{
				positiveForce[i] += objectForce[objectIndex][i];
			}
		}
	}

	// "external" stress is two way (something pushes on the object and something else pushes back)
	// so the set of minimum values per component are the projections of the two-way force
	// "internal" stress is one way (the object is pushing against something OR something pushing back)
	// the momentum must have come from inside the object (gravity, controller, etc)
	Vector internalForce = vec3_origin;
	Vector externalForce = vec3_origin;

	for ( int i = 0; i < 3; i++ )
	{
		if ( negativeForce[i] < positiveForce[i] )
		{
			internalForce[i] = positiveForce[i] - negativeForce[i];
			externalForce[i] = negativeForce[i];
		}
		else
		{
			internalForce[i] = negativeForce[i] - positiveForce[i];
			externalForce[i] = positiveForce[i];
		}
	}

	// sum is kg in / s
	Vector gravVector;
	physenv->GetGravity( &gravVector );
	float gravity = gravVector.Length();
	if ( pInputOwnerEntity->GetMoveType() != MOVETYPE_VPHYSICS && pObject->IsMoveable() )
	{
		Vector lastVel;
		lastVel.Init();
		if ( pObject->GetShadowController() )
		{
			pObject->GetShadowController()->GetLastImpulse( &lastVel );
		}
		else 
		{
			if ( ( pObject->GetCallbackFlags() & CALLBACK_IS_PLAYER_CONTROLLER ) )
			{
				CBasePlayer *pPlayer = ToBasePlayer( pInputOwnerEntity );
				IPhysicsPlayerController *pController = pPlayer ? pPlayer->GetPhysicsController() : NULL;
				if ( pController )
				{
					pController->GetLastImpulse( &lastVel );
				}
			}
		}
		
		// Work in progress...

		// Peek into the controller for this object.  Look at the input velocity and make sure it's all
		// accounted for in the computed stress.  If not, redistribute external to internal as it's 
		// probably being reflected in a way we can't measure here.
		float inputLen = lastVel.Length() * (1.0f / physenv->GetSimulationTimestep()) * objMass;
		if ( inputLen > 0.0f )
		{
			float internalLen = internalForce.Length();
			if ( internalLen < inputLen )
			{
				float ratio = internalLen / inputLen;
				Vector delta = internalForce * (1.0f - ratio);
				internalForce += delta;
				float deltaLen = delta.Length();
				sum -= deltaLen;
				float extLen = VectorNormalize(externalForce) - deltaLen;
				if ( extLen < 0 )
				{
					extLen = 0;
				}
				externalForce *= extLen;
			}
		}
	}

	float invGravity = gravity;
	if ( invGravity <= 0 )
	{
		invGravity = 1.0f;
	}
	else
	{
		invGravity = 1.0f / invGravity;
	}
	sum *= invGravity;
	internalForce *= invGravity;
	externalForce *= invGravity;
	if ( !pObject->IsMoveable() )
	{
		// the above algorithm will see almost all force as internal if the object is not moveable 
		// (it doesn't push on anything else, so nothing is reciprocated)
		// exceptions for friction of a single other object with multiple contact points on this object
		
		// But the game wants to see it all as external because obviously the object can't move, so it can't have
		// internal stress
		externalForce = internalForce;
		internalForce.Init();

		if ( !pObject->IsStatic() )
		{
			sum += objMass;
		}
	}
	else
	{
		// assume object is at rest
		if ( sum > objMass )
		{
			sum = objMass + (sum-objMass) * 0.5;
		}
	}

	if ( pOutput )
	{
		pOutput->exertedStress = internalForce.Length();
		pOutput->receivedStress = externalForce.Length();
		pOutput->hasNonStaticStress = pObjectList.Count() > 2 ? true : false;
		pOutput->hasLargeObjectContact = hasLargeObject;
	}

	// sum is now kg 
	return sum;
}
