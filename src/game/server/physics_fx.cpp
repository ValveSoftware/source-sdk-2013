//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "physics.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static int BestAxisMatchingNormal( matrix3x4_t &matrix, const Vector &normal )
{
	float bestDot = -1;
	int best = 0;
	for ( int i = 0; i < 3; i++ )
	{
		Vector tmp;
		MatrixGetColumn( matrix, i, tmp );
		float dot = fabs(DotProduct( tmp, normal ));
		if ( dot > bestDot )
		{
			bestDot = dot;
			best = i;
		}
	}

	return best;
}

void PhysicsSplash( IPhysicsFluidController *pFluid, IPhysicsObject *pObject, CBaseEntity *pEntity )
{
	Vector normal;
	float dist;
	pFluid->GetSurfacePlane( &normal, &dist );

	matrix3x4_t &matrix = pEntity->EntityToWorldTransform();
	
	// Find the local axis that best matches the water surface normal
	int bestAxis = BestAxisMatchingNormal( matrix, normal );

	Vector tangent, binormal;
	MatrixGetColumn( matrix, (bestAxis+1)%3, tangent );
	binormal = CrossProduct( normal, tangent );
	VectorNormalize( binormal );
	tangent = CrossProduct( binormal, normal );
	VectorNormalize( tangent );

	// Now we have a basis tangent to the surface that matches the object's local orientation as well as possible
	// compute an OBB using this basis
	
	// Get object extents in basis
	Vector tanPts[2], binPts[2];
	tanPts[0] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), -tangent );
	tanPts[1] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), tangent );
	binPts[0] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), -binormal );
	binPts[1] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), binormal );

	// now compute the centered bbox
	float mins[2], maxs[2], center[2], extents[2];
	mins[0] = DotProduct( tanPts[0], tangent );
	maxs[0] = DotProduct( tanPts[1], tangent );

	mins[1] = DotProduct( binPts[0], binormal );
	maxs[1] = DotProduct( binPts[1], binormal );

	center[0] = 0.5 * (mins[0] + maxs[0]);
	center[1] = 0.5 * (mins[1] + maxs[1]);

	extents[0] = maxs[0] - center[0];
	extents[1] = maxs[1] - center[1];

	Vector centerPoint = center[0] * tangent + center[1] * binormal + dist * normal;

	Vector axes[2];
	axes[0] = (maxs[0] - center[0]) * tangent;
	axes[1] = (maxs[1] - center[1]) * binormal;

	// visualize OBB hit
	/*
	Vector corner1 = centerPoint - axes[0] - axes[1];
	Vector corner2 = centerPoint + axes[0] - axes[1];
	Vector corner3 = centerPoint + axes[0] + axes[1];
	Vector corner4 = centerPoint - axes[0] + axes[1];
	NDebugOverlay::Line( corner1, corner2, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner2, corner3, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner3, corner4, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner4, corner1, 0, 0, 255, false, 10 );
	*/

	Vector	corner[4];

	corner[0] = centerPoint - axes[0] - axes[1];
	corner[1] = centerPoint + axes[0] - axes[1];
	corner[2] = centerPoint + axes[0] + axes[1];
	corner[3] = centerPoint - axes[0] + axes[1];

	CEffectData	data;

	if ( pObject->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL )
	{
		/*
		data.m_vOrigin = centerPoint;
		data.m_vNormal = normal;
		VectorAngles( normal, data.m_vAngles );
		data.m_flScale = random->RandomFloat( 8, 10 );

		DispatchEffect( "watersplash", data );
		
		int		splashes = 4;
		Vector	point;

		for ( int i = 0; i < splashes; i++ )
		{
			point = RandomVector( -32.0f, 32.0f );
			point[2] = 0.0f;

			point += corner[i];

			data.m_vOrigin = point;
			data.m_vNormal = normal;
			VectorAngles( normal, data.m_vAngles );
			data.m_flScale = random->RandomFloat( 4, 6 );

			DispatchEffect( "watersplash", data );
		}
		*/

		//FIXME: This code will not work correctly given how the ragdoll/fluid collision is acting currently
		return;
	}

	Vector vel;
	pObject->GetVelocity( &vel, NULL );
	float rawSpeed = -DotProduct( normal, vel );

	// proportional to cross-sectional area times velocity squared (fluid pressure)
	float speed = rawSpeed * rawSpeed * extents[0] * extents[1] * (1.0f / 2500000.0f) * pObject->GetMass() * (0.01f);

	speed = clamp( speed, 0.f, 50.f );

	bool bRippleOnly = false;

	// allow the entity to perform a custom splash effect
	if ( pEntity->PhysicsSplash( centerPoint, normal, rawSpeed, speed ) )
		return;

	//Deny really weak hits
	//FIXME: We still need to ripple the surface in this case
	if ( speed <= 0.35f )
	{
		if ( speed <= 0.1f )
			return;

		bRippleOnly = true;
	}

	float size = RemapVal( speed, 0.35, 50, 8, 18 );

	//Find the surface area
	float	radius = extents[0] * extents[1];
	//int	splashes = clamp ( radius / 128.0f, 1, 2 );	//One splash for every three square feet of area

	//Msg( "Speed: %.2f, Size: %.2f\n, Radius: %.2f, Splashes: %d", speed, size, radius, splashes );

	Vector point;

	data.m_fFlags = 0;
	data.m_vOrigin = centerPoint;
	data.m_vNormal = normal;
	VectorAngles( normal, data.m_vAngles );
	data.m_flScale = size + random->RandomFloat( 0, 2 );
	if ( pEntity->GetWaterType() & CONTENTS_SLIME )
	{
		data.m_fFlags |= FX_WATER_IN_SLIME;
	}

	if ( bRippleOnly )
	{
		DispatchEffect( "waterripple", data );
	}
	else
	{
		DispatchEffect( "watersplash", data );
	}

	if ( radius > 500.0f )
	{
		int splashes = random->RandomInt( 1, 4 );

		for ( int i = 0; i < splashes; i++ )
		{
			point = RandomVector( -4.0f, 4.0f );
			point[2] = 0.0f;

			point += corner[i];

			data.m_fFlags = 0;
			data.m_vOrigin = point;
			data.m_vNormal = normal;
			VectorAngles( normal, data.m_vAngles );
			data.m_flScale = size + random->RandomFloat( -3, 1 );
 			if ( pEntity->GetWaterType() & CONTENTS_SLIME )
			{
				data.m_fFlags |= FX_WATER_IN_SLIME;
			}

			if ( bRippleOnly )
			{
				DispatchEffect( "waterripple", data );
			}
			else
			{
				DispatchEffect( "watersplash", data );
			}
		}
	}

	/*
	for ( i = 0; i < splashes; i++ )
	{
		point = RandomVector( -8.0f, 8.0f );
		point[2] = 0.0f;

		point += centerPoint + axes[0] * random->RandomFloat( -1, 1 ) + axes[1] * random->RandomFloat( -1, 1 );

		data.m_vOrigin = point;
		data.m_vNormal = normal;
		VectorAngles( normal, data.m_vAngles );
		data.m_flScale = size + random->RandomFloat( -2, 4 );

		DispatchEffect( "watersplash", data );
	}
	*/
}
