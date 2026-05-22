//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "tf_bm_bomb.h"
#include "bm_grid.h"
#include "bm_props.h"
#include "props.h"
#include "tf_bm_crate.h"
#include "bm_arena.h"
#include "bm_player_system.h"
#include "tf_gamerules.h"
#include "tf_player.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_bm_bomb, CTFBMBomb );

#define BM_BOMB_STATUE_MODEL "models/soldier_statue/soldier_statue.mdl"

static const char *const g_BMBombModels[] = {
	BM_BOMB_STATUE_MODEL,
	"models/props_gameplay/orange_cone001.mdl",
	"models/props_halloween/pumpkin_loot.mdl",
	"models/props_farm/wooden_barrel.mdl",
	"models/props_c17/oildrum001.mdl",
};

ConVar tf_bm_bomb_fuse( "tf_bm_bomb_fuse", "2.5", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: seconds until a placed bomb explodes." );
ConVar tf_bm_bomb_range( "tf_bm_bomb_range", "2", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: blast length in grid cells (each arm, not counting center)." );
ConVar tf_bm_max_bombs( "tf_bm_max_bombs", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: max active bombs per player." );
ConVar tf_bm_bomb_damage( "tf_bm_bomb_damage", "500", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: blast damage to players." );
ConVar tf_bm_bomb_visible( "tf_bm_bomb_visible", "1", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: spawn a networked prop_dynamic at each bomb (visible on clients)." );
ConVar tf_bm_bomb_scale( "tf_bm_bomb_scale", "0.18", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: scale for bomb prop (soldier statue default; ~0.18 fits a 64u cell)." );

//-----------------------------------------------------------------------------
static float BM_GetBombVisualBaseScale( void )
{
	return clamp( tf_bm_bomb_scale.GetFloat(), 0.05f, 1.0f );
}

//-----------------------------------------------------------------------------
CTFBMBomb::CTFBMBomb()
{
	m_iCellX = 0;
	m_iCellY = 0;
	m_flDetonateTime = 0.0f;
	m_iBlastRange = 2;
	m_bDetonating = false;
	m_hBombVisual.Set( NULL );
}

//-----------------------------------------------------------------------------
void CTFBMBomb::Precache( void )
{
	BM_PrecacheModelCandidates( g_BMBombModels, ARRAYSIZE( g_BMBombModels ) );
	PrecacheScriptSound( "Weapon_Grenade.Tick" );
	PrecacheScriptSound( "BaseGrenade.Explode" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
void CTFBMBomb::Spawn( void )
{
	Precache();

	BM_ApplyPropModelOrHidden( assert_cast<CBaseAnimating *>( this ), g_BMBombModels, ARRAYSIZE( g_BMBombModels ), 1.0f );
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NOSHADOW );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	BaseClass::Spawn();

	SpawnBombVisual();

	SetThink( &CTFBMBomb::BombThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
void CTFBMBomb::SpawnBombVisual( void )
{
	RemoveBombVisual();

	if ( !tf_bm_bomb_visible.GetBool() )
	{
		return;
	}

	BM_PrecacheModelCandidates( g_BMBombModels, ARRAYSIZE( g_BMBombModels ) );
	const char *pszModel = BM_SelectModel( g_BMBombModels, ARRAYSIZE( g_BMBombModels ) );
	if ( !pszModel )
	{
		return;
	}

	CDynamicProp *pProp = dynamic_cast<CDynamicProp *>( CreateEntityByName( "prop_dynamic_override" ) );
	if ( !pProp )
	{
		return;
	}

	const float flBaseScale = BM_GetBombVisualBaseScale();

	pProp->SetModel( pszModel );
	Vector vecOrigin = GetAbsOrigin();
	// Statue origin is at feet; nudge down so the mini statue sits on the grid plane.
	if ( pszModel && Q_stristr( pszModel, "soldier_statue" ) != NULL )
	{
		vecOrigin.z -= BM_GetCellSize() * 0.12f * flBaseScale;
		pProp->SetSequence( 0 );
	}
	pProp->SetAbsOrigin( vecOrigin );
	pProp->SetAbsAngles( vec3_angle );
	pProp->SetModelScale( flBaseScale );
	pProp->SetSolid( SOLID_NONE );
	pProp->SetMoveType( MOVETYPE_NONE );
	pProp->Spawn();
	pProp->Activate();

	m_hBombVisual.Set( pProp );
}

//-----------------------------------------------------------------------------
void CTFBMBomb::RemoveBombVisual( void )
{
	CBaseEntity *pVisual = m_hBombVisual.Get();
	if ( pVisual )
	{
		UTIL_Remove( pVisual );
	}
	m_hBombVisual.Set( NULL );
}

//-----------------------------------------------------------------------------
void CTFBMBomb::BombThink( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		UTIL_Remove( this );
		return;
	}

	if ( !m_bDetonating && gpGlobals->curtime >= m_flDetonateTime )
	{
		Detonate();
		return;
	}

	const float flPulse = 0.85f + 0.15f * sinf( gpGlobals->curtime * 8.0f );
	SetModelScale( flPulse, 0.0f );

	const float flBaseScale = BM_GetBombVisualBaseScale();
	CBaseAnimating *pVisual = dynamic_cast<CBaseAnimating *>( m_hBombVisual.Get() );
	if ( pVisual )
	{
		pVisual->SetModelScale( flBaseScale * flPulse, 0.0f );
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
CTFBMBomb *CTFBMBomb::GetBombAtCell( int iCellX, int iCellY )
{
	return BM_FindBombAtCell( iCellX, iCellY );
}

//-----------------------------------------------------------------------------
CTFBMBomb *CTFBMBomb::PlaceAtCell( CTFPlayer *pOwner, int iCellX, int iCellY )
{
	if ( !pOwner || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return NULL;
	}

	if ( GetBombAtCell( iCellX, iCellY ) != NULL )
	{
		return NULL;
	}

	Vector vecCenter;
	BM_CellToWorldCenter( iCellX, iCellY, vecCenter );

	CTFBMBomb *pBomb = assert_cast<CTFBMBomb *>( CreateEntityByName( "tf_bm_bomb" ) );
	if ( !pBomb )
	{
		return NULL;
	}

	pBomb->m_iCellX = iCellX;
	pBomb->m_iCellY = iCellY;
	pBomb->m_hOwnerPlayer = pOwner;
	pBomb->m_iBlastRange = clamp( tf_bm_bomb_range.GetInt(), 1, 8 );
	pBomb->m_flDetonateTime = gpGlobals->curtime + Max( 0.5f, tf_bm_bomb_fuse.GetFloat() );

	pBomb->SetAbsOrigin( vecCenter );
	pBomb->SetAbsAngles( vec3_angle );

	DispatchSpawn( pBomb );
	pBomb->Activate();

	pOwner->m_iBMActiveBombs++;

	pBomb->EmitSound( "Weapon_Grenade.Tick" );

	return pBomb;
}

//-----------------------------------------------------------------------------
static void BM_HurtPlayersAtCell( int iCellX, int iCellY, CTFPlayer *pOwner, CTFBMBomb *pBomb )
{
	Vector vecCenter;
	BM_CellToWorldCenter( iCellX, iCellY, vecCenter );

	const float flCell = BM_GetCellSize();
	const float flRadius = flCell * 0.55f;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			continue;
		}

		Vector vecDelta = pPlayer->GetAbsOrigin() - vecCenter;
		vecDelta.z = 0.0f;
		if ( vecDelta.LengthSqr() > flRadius * flRadius )
		{
			continue;
		}

		if ( pOwner && pPlayer == pOwner )
		{
			continue;
		}

		CTakeDamageInfo info( pBomb, pOwner, tf_bm_bomb_damage.GetFloat(), DMG_BLAST );
		pPlayer->TakeDamage( info );
	}
}

//-----------------------------------------------------------------------------
void CTFBMBomb::Detonate( void )
{
	if ( m_bDetonating )
	{
		return;
	}

	m_bDetonating = true;
	RemoveBombVisual();

	CTFPlayer *pOwner = ToTFPlayer( m_hOwnerPlayer.Get() );
	if ( pOwner && pOwner->m_iBMActiveBombs > 0 )
	{
		pOwner->m_iBMActiveBombs--;
	}

	Vector vecCenter;
	BM_CellToWorldCenter( m_iCellX, m_iCellY, vecCenter );

	ExplosionCreate( vecCenter, vec3_angle, this, 120, 180, true, 0.0f, false, false, DMG_BLAST );
	EmitSound( "BaseGrenade.Explode" );

	static const int s_aiDirs[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };

	BM_DestroyCrateAtCell( m_iCellX, m_iCellY );
	BM_HurtPlayersAtCell( m_iCellX, m_iCellY, pOwner, this );

	for ( int iDir = 0; iDir < 4; ++iDir )
	{
		const int iDirX = s_aiDirs[iDir][0];
		const int iDirY = s_aiDirs[iDir][1];

		for ( int iDist = 1; iDist <= m_iBlastRange; ++iDist )
		{
			const int iCellX = m_iCellX + iDirX * iDist;
			const int iCellY = m_iCellY + iDirY * iDist;

			if ( BM_CellBlocksBlast( iCellX, iCellY ) ||
				 BM_IsBlastBlockedToCell( m_iCellX + iDirX * ( iDist - 1 ), m_iCellY + iDirY * ( iDist - 1 ), iCellX, iCellY, pOwner ) )
			{
				break;
			}

			BM_DestroyCrateAtCell( iCellX, iCellY );

			CTFBMBomb *pOther = GetBombAtCell( iCellX, iCellY );
			if ( pOther && pOther != this && !pOther->m_bDetonating )
			{
				pOther->Detonate();
			}

			BM_HurtPlayersAtCell( iCellX, iCellY, pOwner, this );

			if ( BM_IsBlastBlockedToCell( iCellX, iCellY, iCellX + iDirX, iCellY + iDirY, pOwner ) )
			{
				break;
			}
		}
	}

	UTIL_Remove( this );
}

#endif // SOURCESDK
