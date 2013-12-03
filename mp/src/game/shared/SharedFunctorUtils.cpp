// SharedFunctorUtils.cpp
// Useful functors
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "SharedFunctorUtils.h"
#include "collisionutils.h"
#ifdef CLIENT_DLL
	#include "ClientTerrorPlayer.h"
#else
	#include "TerrorPlayer.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------
bool AvoidActors::operator()( CBaseCombatCharacter *obj )
{
	if ( !obj || ( obj == m_owner ) || ( obj->GetTeamNumber() != m_owner->GetTeamNumber() ) )
		return true;

#ifdef CLIENT_DLL
	if ( obj->IsDormant() )
		return true;
#endif

	if ( obj->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return true;

	CTerrorPlayer *player = ToTerrorPlayer( obj );
	if ( !player )
		return true;

	if ( player->IsIncapacitatedRevivable() )
		return true;

	if ( player->IsGhost() )
		return true;

	Vector objOrigin = obj->GetAbsOrigin();
	Vector vObjMins = objOrigin + obj->WorldAlignMins();
	Vector vObjMaxs = objOrigin + obj->WorldAlignMaxs();
	Vector vOwnerMins = *m_dest + m_owner->WorldAlignMins();
	Vector vOwnerMaxs = *m_dest + m_owner->WorldAlignMaxs();
	if ( !IsBoxIntersectingBox( vOwnerMins, vOwnerMaxs, vObjMins, vObjMaxs ) )
		return true;

	float objWidth = vObjMaxs.x - vObjMins.x;
	float ownerWidth = vOwnerMaxs.x - vOwnerMins.x;
	float idealDistance = (objWidth + ownerWidth) * 0.5f * m_scale;

	Vector vDelta = objOrigin - *m_dest;
	vDelta.z = 0;
	float fDist = vDelta.NormalizeInPlace();
	if ( fDist > idealDistance )
		return true;

	Vector rayOrigin = m_origin;
	Vector rayDelta = *m_dest - m_origin;
	Vector sphereCenter = objOrigin;
	float sphereRadius = idealDistance;
	rayOrigin.z = rayDelta.z = sphereCenter.z = 0.0f;
	float t1, t2;
	if ( IntersectRayWithSphere( rayOrigin, rayDelta, sphereCenter, sphereRadius, &t1, &t2 ) )
	{
		Vector sphereToDest = *m_dest - sphereCenter;
		sphereToDest.z = 0.0f;
		if ( !sphereToDest.IsZero() )
		{
			float radius = sphereToDest.NormalizeInPlace();
			sphereToDest *= (idealDistance - radius);
			*m_dest += sphereToDest;
			m_avoidedActors.AddToTail( obj );
		}
	}

	return true;
}


//--------------------------------------------------------------------------------------------------------
