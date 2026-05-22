//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "tf_bm_wall.h"
#include "bm_grid.h"
#include "bm_props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_bm_wall, CTFBMWall );

static const char *const g_BMWallModels[] = {
	"models/props_gameplay/orange_cone001.mdl",
	"models/props_junk/wood_crate001a.mdl",
};

//-----------------------------------------------------------------------------
CTFBMWall::CTFBMWall()
{
	m_iCellX = 0;
	m_iCellY = 0;
}

//-----------------------------------------------------------------------------
void CTFBMWall::Precache( void )
{
	BM_PrecacheModelCandidates( g_BMWallModels, ARRAYSIZE( g_BMWallModels ) );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
void CTFBMWall::Spawn( void )
{
	Precache();

	BM_ApplyPropModelOrHidden( assert_cast<CBaseAnimating *>( this ), g_BMWallModels, ARRAYSIZE( g_BMWallModels ), 1.0f );

	// Grid logic blocks movement; SOLID_BBOX here caused invisible hull hits (models are EF_NODRAW).
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NOSHADOW );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
void CTFBMWall::RemoveAllWalls( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_wall" ) ) != NULL )
	{
		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
CTFBMWall *CTFBMWall::CreateAtCell( int iCellX, int iCellY )
{
	Vector vecCenter;
	BM_CellToWorldCenter( iCellX, iCellY, vecCenter );

	CTFBMWall *pWall = assert_cast<CTFBMWall *>( CreateEntityByName( "tf_bm_wall" ) );
	if ( !pWall )
	{
		return NULL;
	}

	pWall->m_iCellX = iCellX;
	pWall->m_iCellY = iCellY;
	pWall->SetAbsOrigin( vecCenter );
	pWall->SetAbsAngles( vec3_angle );
	DispatchSpawn( pWall );
	pWall->Activate();

	return pWall;
}

#endif // SOURCESDK
