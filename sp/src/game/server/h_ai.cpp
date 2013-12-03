//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Utility functions used by AI code.
//
//=============================================================================//

#include "cbase.h"
#include "game.h"
#include "vstdlib/random.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define		NUM_LATERAL_CHECKS		13  // how many checks are made on each side of a NPC looking for lateral cover
#define		NUM_LATERAL_LOS_CHECKS		6  // how many checks are made on each side of a NPC looking for lateral cover

#define		TOSS_HEIGHT_MAX		300 // altitude of initial trace done to see how high something can be tossed

//float flRandom = random->RandomFloat(0,1);

bool g_fDrawLines = FALSE;


//=========================================================
// FBoxVisible - a more accurate ( and slower ) version
// of FVisible. 
//
// !!!UNDONE - make this CAI_BaseNPC?
//=========================================================
bool FBoxVisible( CBaseEntity *pLooker, CBaseEntity *pTarget, Vector &vecTargetOrigin, float flSize )
{
	// don't look through water
	if ((pLooker->GetWaterLevel() != 3 && pTarget->GetWaterLevel() == 3) 
		|| (pLooker->GetWaterLevel() == 3 && pTarget->GetWaterLevel() == 0))
		return FALSE;

	trace_t tr;
	Vector	vecLookerOrigin = pLooker->EyePosition();//look through the NPC's 'eyes'
	for (int i = 0; i < 5; i++)
	{
		Vector vecTarget = pTarget->GetAbsOrigin();
		vecTarget.x += random->RandomFloat( pTarget->WorldAlignMins().x + flSize, pTarget->WorldAlignMaxs().x - flSize);
		vecTarget.y += random->RandomFloat( pTarget->WorldAlignMins().y + flSize, pTarget->WorldAlignMaxs().y - flSize);
		vecTarget.z += random->RandomFloat( pTarget->WorldAlignMins().z + flSize, pTarget->WorldAlignMaxs().z - flSize);

		UTIL_TraceLine(vecLookerOrigin, vecTarget, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, &tr);
		
		if (tr.fraction == 1.0)
		{
			vecTargetOrigin = vecTarget;
			return TRUE;// line of sight is valid.
		}
	}
	return FALSE;// Line of sight is not established
}



//-----------------------------------------------------------------------------
// Purpose: Returns the correct toss velocity to throw a given object at a point. 
//			Like the other version of VecCheckToss, but allows you to filter for any 
//			number of entities to ignore.
// Input  : pEntity - The object doing the throwing. Is *NOT* automatically included in the
//					  filter below.
//			pFilter - A trace filter of entities to ignore in the object's collision sweeps. 
//					  It is recommended to include at least the thrower.
//			vecSpot1 - The point from which the object is being thrown.
//			vecSpot2 - The point TO which the object is being thrown.
//			flHeightMaxRatio - A scale factor indicating the maximum ratio of height
//				to total throw distance, measured from the higher of the two endpoints to
//				the apex. -1 indicates that there is no maximum.
//			flGravityAdj - Scale factor for gravity - should match the gravity scale
//				that the object will use in midair.
//			bRandomize - when true, introduces a little fudge to the throw
// Output : Velocity to throw the object with.
//-----------------------------------------------------------------------------
Vector VecCheckToss( CBaseEntity *pEntity, ITraceFilter *pFilter, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins, Vector *vecMaxs )
{
	trace_t			tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecTossVel;
	Vector			vecTemp;
	float			flGravity = GetCurrentGravity() * flGravityAdj;

	if (vecSpot2.z - vecSpot1.z > 500)
	{
		// to high, fail
		return vec3_origin;
	}

	Vector forward, right;
	AngleVectors( pEntity->GetLocalAngles(), &forward, &right, NULL );

	if (bRandomize)
	{
		// toss a little bit to the left or right, not right down on the enemy's bean (head). 
		vecSpot2 += right * ( random->RandomFloat(-8,8) + random->RandomFloat(-16,16) );
		vecSpot2 += forward * ( random->RandomFloat(-8,8) + random->RandomFloat(-16,16) );
	}

	// calculate the midpoint and apex of the 'triangle'
	// UNDONE: normalize any Z position differences between spot1 and spot2 so that triangle is always RIGHT
	// get a rough idea of how high it can be thrown
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,TOSS_HEIGHT_MAX), MASK_SOLID_BRUSHONLY, pFilter, &tr);
	vecMidPoint = tr.endpos;

	if( tr.fraction != 1.0 )
	{
		// (subtract 15 so the object doesn't hit the ceiling)
		vecMidPoint.z -= 15;
	}

	if (flHeightMaxRatio != -1)
	{
		// But don't throw so high that it looks silly. Maximize the height of the
		// throw above the highest of the two endpoints to a ratio of the throw length.
		float flHeightMax = flHeightMaxRatio * (vecSpot2 - vecSpot1).Length();
		float flHighestEndZ = MAX(vecSpot1.z, vecSpot2.z);
		if ((vecMidPoint.z - flHighestEndZ) > flHeightMax)
		{
			vecMidPoint.z = flHighestEndZ + flHeightMax;
		}
	}

	if (vecMidPoint.z < vecSpot1.z || vecMidPoint.z < vecSpot2.z)
	{
		// Not enough space, fail
		return vec3_origin;
	}

	// How high should the object travel to reach the apex
	float distance1 = (vecMidPoint.z - vecSpot1.z);
	float distance2 = (vecMidPoint.z - vecSpot2.z);

	// How long will it take for the object to travel this distance
	float time1 = sqrt( distance1 / (0.5 * flGravity) );
	float time2 = sqrt( distance2 / (0.5 * flGravity) );

	if (time1 < 0.1)
	{
		// too close
		return vec3_origin;
	}

	// how hard to throw sideways to get there in time.
	vecTossVel = (vecSpot2 - vecSpot1) / (time1 + time2);

	// how hard upwards to reach the apex at the right time.
	vecTossVel.z = flGravity * time1;

	// find the apex
	vecApex  = vecSpot1 + vecTossVel * time1;
	vecApex.z = vecMidPoint.z;

	// JAY: Repro behavior from HL1 -- toss check went through gratings
	UTIL_TraceLine(vecSpot1, vecApex, (MASK_SOLID&(~CONTENTS_GRATE)), pFilter, &tr);
	if (tr.fraction != 1.0)
	{
		// fail!
		return vec3_origin;
	}

	// UNDONE: either ignore NPCs or change it to not care if we hit our enemy
	UTIL_TraceLine(vecSpot2, vecApex, (MASK_SOLID_BRUSHONLY&(~CONTENTS_GRATE)), pFilter, &tr); 
	if (tr.fraction != 1.0)
	{
		// fail!
		return vec3_origin;
	}

	if ( vecMins && vecMaxs )
	{
		// Check to ensure the entity's hull can travel the first half of the grenade throw
		UTIL_TraceHull( vecSpot1, vecApex, *vecMins, *vecMaxs, (MASK_SOLID&(~CONTENTS_GRATE)), pFilter, &tr);		
		if ( tr.fraction < 1.0 )
			return vec3_origin;
	}

	return vecTossVel;
}



//-----------------------------------------------------------------------------
// Purpose: Returns the correct toss velocity to throw a given object at a point.
// Input  : pEntity - The entity that is throwing the object.
//			vecSpot1 - The point from which the object is being thrown.
//			vecSpot2 - The point TO which the object is being thrown.
//			flHeightMaxRatio - A scale factor indicating the maximum ratio of height
//				to total throw distance, measured from the higher of the two endpoints to
//				the apex. -1 indicates that there is no maximum.
//			flGravityAdj - Scale factor for gravity - should match the gravity scale
//				that the object will use in midair.
//			bRandomize - when true, introduces a little fudge to the throw
// Output : Velocity to throw the object with.
//-----------------------------------------------------------------------------
Vector VecCheckToss( CBaseEntity *pEntity, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins, Vector *vecMaxs )
{
	// construct a filter and call through to the other version of this function.
	CTraceFilterSimple traceFilter( pEntity, COLLISION_GROUP_NONE );
	return VecCheckToss( pEntity, &traceFilter, vecSpot1, vecSpot2, 
						 flHeightMaxRatio, flGravityAdj, bRandomize, 
						 vecMins, vecMaxs );
}

//
// VecCheckThrow - returns the velocity vector at which an object should be thrown from vecspot1 to hit vecspot2.
// returns vec3_origin if throw is not feasible.
// 
Vector VecCheckThrow ( CBaseEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj, Vector *vecMins, Vector *vecMaxs )
{
	float			flGravity = GetCurrentGravity() * flGravityAdj;

	Vector vecGrenadeVel = (vecSpot2 - vecSpot1);

	// throw at a constant time
	float time = vecGrenadeVel.Length( ) / flSpeed;
	vecGrenadeVel = vecGrenadeVel * (1.0 / time);

	// adjust upward toss to compensate for gravity loss
	vecGrenadeVel.z += flGravity * time * 0.5;

	Vector vecApex = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	vecApex.z += 0.5 * flGravity * (time * 0.5) * (time * 0.5);

	
	trace_t tr;
	UTIL_TraceLine(vecSpot1, vecApex, MASK_SOLID, pEdict, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction != 1.0)
	{
		// fail!
		//NDebugOverlay::Line( vecSpot1, vecApex, 255, 0, 0, true, 5.0 );
		return vec3_origin;
	}

	//NDebugOverlay::Line( vecSpot1, vecApex, 0, 255, 0, true, 5.0 );

	UTIL_TraceLine(vecSpot2, vecApex, MASK_SOLID_BRUSHONLY, pEdict, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction != 1.0)
	{
		// fail!
		//NDebugOverlay::Line( vecApex, vecSpot2, 255, 0, 0, true, 5.0 );
		return vec3_origin;
	}

	//NDebugOverlay::Line( vecApex, vecSpot2, 0, 255, 0, true, 5.0 );

	if ( vecMins && vecMaxs )
	{
		// Check to ensure the entity's hull can travel the first half of the grenade throw
		UTIL_TraceHull( vecSpot1, vecApex, *vecMins, *vecMaxs, MASK_SOLID, pEdict, COLLISION_GROUP_NONE, &tr);		
		if ( tr.fraction < 1.0 )
		{
			//NDebugOverlay::SweptBox( vecSpot1, tr.endpos, *vecMins, *vecMaxs, vec3_angle, 255, 0, 0, 64, 5.0 );
			return vec3_origin;
		}
	}

	//NDebugOverlay::SweptBox( vecSpot1, vecApex, *vecMins, *vecMaxs, vec3_angle, 0, 255, 0, 64, 5.0 );

	return vecGrenadeVel;
}
