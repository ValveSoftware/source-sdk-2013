//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "tf_bm_floor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_bm_floor, CTFBMFloor );

//-----------------------------------------------------------------------------
void CTFBMFloor::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	AddEffects( EF_NODRAW );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
void CTFBMFloor::RemoveAllFloors( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_floor" ) ) != NULL )
	{
		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
CTFBMFloor *CTFBMFloor::CreateForArena( const Vector &vecCenter, float flWidth, float flDepth, float flTopZ )
{
	CTFBMFloor *pFloor = assert_cast<CTFBMFloor *>( CreateEntityByName( "tf_bm_floor" ) );
	if ( !pFloor )
	{
		return NULL;
	}

	const float flHalfW = Max( flWidth * 0.5f, 256.0f );
	const float flHalfD = Max( flDepth * 0.5f, 256.0f );
	const float flThickness = 24.0f;

	Vector vecOrigin( vecCenter.x, vecCenter.y, flTopZ - flThickness * 0.5f );
	pFloor->SetAbsOrigin( vecOrigin );
	pFloor->SetAbsAngles( vec3_angle );

	UTIL_SetSize( pFloor, Vector( -flHalfW, -flHalfD, -flThickness * 0.5f ), Vector( flHalfW, flHalfD, flThickness * 0.5f ) );

	DispatchSpawn( pFloor );
	pFloor->Activate();

	return pFloor;
}

#endif // SOURCESDK
