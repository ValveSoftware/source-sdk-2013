//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "tf_bm_crate.h"
#include "bm_grid.h"
#include "bm_props.h"
#include "tf_gamerules.h"
#include "tf_player.h"
#include "ndebugoverlay.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_bm_crate, CTFBMCrate );

static const char *const g_BMCrateModels[] = {
	"models/props_gameplay/orange_cone001.mdl",
	"models/props_farm/wooden_barrel.mdl",
	"models/props_farm/concrete_block001.mdl",
	"models/props_halloween/pumpkin_loot.mdl",
	"models/props_c17/oildrum001.mdl",
};

ConVar tf_bm_crate_density( "tf_bm_crate_density", "0.28", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: chance each arena cell gets a breakable crate." );
ConVar tf_bm_crate_radius( "tf_bm_crate_radius", "7", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: half-size of crate arena in grid cells." );

//-----------------------------------------------------------------------------
CTFBMCrate::CTFBMCrate()
{
	m_iCellX = 0;
	m_iCellY = 0;
}

//-----------------------------------------------------------------------------
void CTFBMCrate::Precache( void )
{
	BM_PrecacheModelCandidates( g_BMCrateModels, ARRAYSIZE( g_BMCrateModels ) );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
void CTFBMCrate::Spawn( void )
{
	Precache();

	BM_ApplyPropModelOrHidden( assert_cast<CBaseAnimating *>( this ), g_BMCrateModels, ARRAYSIZE( g_BMCrateModels ), 0.85f );
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NOSHADOW );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
CTFBMCrate *CTFBMCrate::GetCrateAtCell( int iCellX, int iCellY )
{
	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_bm_crate" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_crate" ) )
	{
		CTFBMCrate *pCrate = assert_cast<CTFBMCrate *>( pEnt );
		if ( pCrate && pCrate->m_iCellX == iCellX && pCrate->m_iCellY == iCellY )
		{
			return pCrate;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
void CTFBMCrate::RemoveAllCrates( void )
{
	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_crate" ) ) != NULL )
	{
		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
CTFBMCrate *CTFBMCrate::CreateAtCell( int iCellX, int iCellY )
{
	if ( GetCrateAtCell( iCellX, iCellY ) != NULL )
	{
		return NULL;
	}

	if ( BM_FindBombAtCell( iCellX, iCellY ) != NULL )
	{
		return NULL;
	}

	Vector vecCenter;
	BM_CellToWorldCenter( iCellX, iCellY, vecCenter );

	CTFBMCrate *pCrate = assert_cast<CTFBMCrate *>( CreateEntityByName( "tf_bm_crate" ) );
	if ( !pCrate )
	{
		return NULL;
	}

	pCrate->m_iCellX = iCellX;
	pCrate->m_iCellY = iCellY;
	pCrate->SetAbsOrigin( vecCenter );
	pCrate->SetAbsAngles( QAngle( 0, RandomFloat( 0, 360 ), 0 ) );

	DispatchSpawn( pCrate );
	pCrate->Activate();

	return pCrate;
}

//-----------------------------------------------------------------------------
CTFBMCrate *BM_FindCrateAtCell( int iCellX, int iCellY )
{
	return CTFBMCrate::GetCrateAtCell( iCellX, iCellY );
}

//-----------------------------------------------------------------------------
void BM_DestroyCrateAtCell( int iCellX, int iCellY )
{
	CTFBMCrate *pCrate = CTFBMCrate::GetCrateAtCell( iCellX, iCellY );
	if ( !pCrate )
	{
		return;
	}

	Vector vecOrigin = pCrate->GetAbsOrigin();
	ExplosionCreate( vecOrigin, vec3_angle, pCrate, 40, 64, false, 0.0f, false, true, DMG_BLAST );
	UTIL_ScreenShake( vecOrigin, 8.0f, 120.0f, 0.4f, 256.0f, SHAKE_START );
	UTIL_Remove( pCrate );
}

#endif // SOURCESDK
