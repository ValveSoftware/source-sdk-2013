//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_arena.h"
#include "bm_grid.h"
#include "bm_shareddefs.h"
#include "tf_bm_crate.h"
#include "tf_bm_wall.h"
#include "tf_bm_floor.h"
#include "bm_props.h"
#include "tf_gamerules.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_arena_width( "tf_bm_arena_width", "15", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena width in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "13", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena height in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_soft_fill( "tf_bm_arena_soft_fill", "0.35", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: chance to place a soft crate in empty interior cells." );
ConVar tf_bm_arena_lift( "tf_bm_arena_lift", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: legacy relative lift above spawns." );
ConVar tf_bm_arena_offset( "tf_bm_arena_offset", "2048 2048", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: XY offset from map spawns for floating arena (stock maps)." );
ConVar tf_bm_void_arena( "tf_bm_void_arena", "0", FCVAR_REPLICATED | FCVAR_NOTIFY,
	"Bomberman: park arena in empty space (+8192 on itemtest). Off = build on map floor (recommended)." );
ConVar tf_bm_platform_height( "tf_bm_platform_height", "512", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: platform height above highest map spawn Z (void arena only)." );
ConVar tf_bm_platform_z( "tf_bm_platform_z", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: absolute platform Z override (0 = use platform_height above spawns)." );

extern ConVar tf_bm_grid_origin;
extern ConVar tf_bm_sky_arena;
extern ConVar tf_bm_sky_height;
extern ConVar tf_bm_play_z_offset;

static bool s_bArenaActive = false;
static int s_iArenaWidth = 0;
static int s_iArenaHeight = 0;
static CBaseEntity *s_pBMSkySpawn = NULL;

static bool BM_IsNearSpawnCell( int iCellX, int iCellY );
static bool BM_GetTeamSpawnAnchors( Vector &vecRed, Vector &vecBlue );
static bool BM_AlignGridOriginFromAnchors( const Vector &vecRed, const Vector &vecBlue, int iWidth, int iHeight, float flCell, Vector &vecGridOrigin );
static CBaseEntity *BM_FindMapTeamSpawn( CTFPlayer *pPlayer );

//-----------------------------------------------------------------------------
bool BM_IsDedicatedArenaMap( void )
{
	const char *pszMap = STRING( gpGlobals->mapname );
	return ( pszMap && Q_stricmp( pszMap, "bm_arena" ) == 0 );
}

//-----------------------------------------------------------------------------
// bm_arena BSP and itemtest fallback: align grid to team spawns on real map geometry (sky + lighting).
//-----------------------------------------------------------------------------
static bool BM_IsMapFloorArena( void )
{
	const char *pszMap = STRING( gpGlobals->mapname );
	if ( !pszMap )
	{
		return false;
	}

	return ( Q_stricmp( pszMap, "bm_arena" ) == 0 || Q_stricmp( pszMap, "itemtest" ) == 0 );
}

//-----------------------------------------------------------------------------
static void BM_GetVoidPlatformOffset( float &flOffX, float &flOffY )
{
	flOffX = 2048.0f;
	flOffY = 2048.0f;
	sscanf( tf_bm_arena_offset.GetString(), "%f %f", &flOffX, &flOffY );

	if ( tf_bm_void_arena.GetBool() && flOffX == 0.0f && flOffY == 0.0f )
	{
		const char *pszMap = STRING( gpGlobals->mapname );
		if ( pszMap && Q_stricmp( pszMap, "itemtest" ) == 0 )
		{
			flOffX = 8192.0f;
			flOffY = 8192.0f;
		}
	}
}

//-----------------------------------------------------------------------------
float BM_GetEffectiveArenaLift( void )
{
	if ( tf_bm_sky_arena.GetBool() || BM_IsMapFloorArena() )
	{
		return 0.0f;
	}

	return Max( 256.0f, tf_bm_arena_lift.GetFloat() );
}

//-----------------------------------------------------------------------------
bool BM_UseVoidArenaPlatform( void )
{
	return ( !BM_IsMapFloorArena() && !tf_bm_sky_arena.GetBool() );
}

//-----------------------------------------------------------------------------
static int BM_GetPlayerSpawnSlot( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return 0;
	}

	int iSlot = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pOther || !pOther->IsConnected() || pOther == pPlayer )
		{
			continue;
		}

		if ( pOther->GetTeamNumber() == pPlayer->GetTeamNumber() && pOther->GetUserID() < pPlayer->GetUserID() )
		{
			++iSlot;
		}
	}

	return clamp( iSlot, 0, BM_MAX_SPAWN_SLOTS_PER_TEAM - 1 );
}

//-----------------------------------------------------------------------------
static void BM_GetArenaSpawnCell( CTFPlayer *pPlayer, int &iCellX, int &iCellY )
{
	iCellX = 1;
	iCellY = 1;

	if ( !pPlayer || !s_bArenaActive )
	{
		return;
	}

	const int iSlot = BM_GetPlayerSpawnSlot( pPlayer );
	const bool bBlueTeam = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE );
	BM_GetSpawnCellForSlot( bBlueTeam, iSlot, s_iArenaWidth, s_iArenaHeight, iCellX, iCellY );
}

//-----------------------------------------------------------------------------
void BM_GetArenaSize( int &iWidth, int &iHeight )
{
	iWidth = s_iArenaWidth;
	iHeight = s_iArenaHeight;
}

//-----------------------------------------------------------------------------
bool BM_IsArenaActive( void )
{
	return s_bArenaActive;
}

//-----------------------------------------------------------------------------
bool BM_IsInsideArenaCell( int iCellX, int iCellY )
{
	if ( !s_bArenaActive )
	{
		return false;
	}

	return ( iCellX >= 0 && iCellY >= 0 && iCellX < s_iArenaWidth && iCellY < s_iArenaHeight );
}

//-----------------------------------------------------------------------------
bool BM_IsHardWallCell( int iCellX, int iCellY )
{
	if ( !s_bArenaActive || !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		return false;
	}

	if ( iCellX == 0 || iCellY == 0 || iCellX == s_iArenaWidth - 1 || iCellY == s_iArenaHeight - 1 )
	{
		return true;
	}

	if ( ( iCellX % 2 ) == 0 && ( iCellY % 2 ) == 0 )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool BM_IsSpawnSafeCell( int iCellX, int iCellY )
{
	if ( !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		return false;
	}

	return BM_IsNearSpawnCell( iCellX, iCellY );
}

//-----------------------------------------------------------------------------
static bool BM_IsNearSpawnCell( int iCellX, int iCellY )
{
	if ( !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		return false;
	}

	const int iMaxY = s_iArenaHeight - 2;
	const int iMaxX = s_iArenaWidth - 2;
	const int iMargin = 3;

	// RED corner (low X, high Y)
	if ( iCellX <= 1 + iMargin && iCellY >= iMaxY - iMargin )
	{
		return true;
	}

	// BLU corner (high X, low Y)
	if ( iCellX >= iMaxX - iMargin && iCellY <= 1 + iMargin )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool BM_CellBlocksMovement( int iCellX, int iCellY )
{
	if ( BM_IsHardWallCell( iCellX, iCellY ) )
	{
		return true;
	}

	if ( BM_FindCrateAtCell( iCellX, iCellY ) != NULL )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool BM_CellBlocksBlast( int iCellX, int iCellY )
{
	if ( !s_bArenaActive )
	{
		return false;
	}

	if ( !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		return true;
	}

	return BM_IsHardWallCell( iCellX, iCellY );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static CBaseEntity *BM_FindMapTeamSpawn( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !BM_IsMapFloorArena() )
	{
		return NULL;
	}

	const int iTeam = pPlayer->GetTeamNumber();
	if ( iTeam != TF_TEAM_RED && iTeam != TF_TEAM_BLUE )
	{
		return NULL;
	}

	CUtlVector<CBaseEntity *> vecSpawns;
	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" ) )
	{
		if ( pEnt->GetTeamNumber() == iTeam )
		{
			vecSpawns.AddToTail( pEnt );
		}
	}

	if ( vecSpawns.Count() == 0 )
	{
		return NULL;
	}

	for ( int i = 0; i < vecSpawns.Count(); ++i )
	{
		for ( int j = i + 1; j < vecSpawns.Count(); ++j )
		{
			const Vector &a = vecSpawns[i]->GetAbsOrigin();
			const Vector &b = vecSpawns[j]->GetAbsOrigin();
			if ( a.x > b.x || ( a.x == b.x && a.y > b.y ) )
			{
				V_swap( vecSpawns[i], vecSpawns[j] );
			}
		}
	}

	const int iSlot = BM_GetPlayerSpawnSlot( pPlayer );
	return vecSpawns[ iSlot % vecSpawns.Count() ];
}

//-----------------------------------------------------------------------------
CBaseEntity *BM_GetSkySpawnEntity( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return NULL;
	}

	if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		return NULL;
	}

	if ( !s_bArenaActive )
	{
		BM_BuildArena( false );
	}

	if ( !s_bArenaActive )
	{
		return NULL;
	}

	CBaseEntity *pMapSpawn = BM_FindMapTeamSpawn( pPlayer );
	if ( pMapSpawn )
	{
		return pMapSpawn;
	}

	if ( !s_pBMSkySpawn )
	{
		s_pBMSkySpawn = CreateEntityByName( "info_target" );
		if ( s_pBMSkySpawn )
		{
			s_pBMSkySpawn->AddEffects( EF_NODRAW );
			DispatchSpawn( s_pBMSkySpawn );
			s_pBMSkySpawn->Activate();
		}
	}

	if ( !s_pBMSkySpawn )
	{
		return NULL;
	}

	int iCellX = 0;
	int iCellY = 0;
	BM_GetArenaSpawnCell( pPlayer, iCellX, iCellY );

	Vector vecDest;
	BM_CellToWorldCenter( iCellX, iCellY, vecDest );
	s_pBMSkySpawn->SetAbsOrigin( vecDest );
	s_pBMSkySpawn->SetAbsAngles( vec3_angle );
	s_pBMSkySpawn->ChangeTeam( pPlayer->GetTeamNumber() );

	return s_pBMSkySpawn;
}

//-----------------------------------------------------------------------------
void BM_WarpPlayerToArenaSpawn( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !s_bArenaActive )
	{
		return;
	}

	Vector vecDest;
	CBaseEntity *pSpawn = BM_GetSkySpawnEntity( pPlayer );
	if ( pSpawn )
	{
		vecDest = pSpawn->GetAbsOrigin();
	}
	else
	{
		int iCellX = 0;
		int iCellY = 0;
		BM_GetArenaSpawnCell( pPlayer, iCellX, iCellY );
		BM_CellToWorldCenter( iCellX, iCellY, vecDest );
	}

	if ( BM_IsMapFloorArena() )
	{
		Vector vecFloor;
		BM_FindFloorAtXY( vecDest, vecDest.z + 256.0f, pPlayer, vecFloor );
		vecDest.z = vecFloor.z;
	}
	else
	{
		// If still embedded in world brushes, step up onto the platform.
		for ( int iNudge = 0; iNudge < 24; ++iNudge )
		{
			trace_t trace;
			UTIL_TraceHull( vecDest, vecDest + Vector( 0, 0, 2 ), VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER, &trace );
			if ( !trace.startsolid )
			{
				break;
			}

			vecDest.z += 32.0f;
		}
	}

	QAngle angFace( 0.0f, 0.0f, 0.0f );
	if ( pSpawn )
	{
		angFace = pSpawn->GetAbsAngles();
	}
	else if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
	{
		angFace.y = 45.0f;
	}
	else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
	{
		angFace.y = 225.0f;
	}

	pPlayer->Teleport( &vecDest, &angFace, &vec3_origin );
	pPlayer->SetLocalOrigin( vecDest );
	pPlayer->SetAbsOrigin( vecDest );
	pPlayer->SnapEyeAngles( angFace );

	if ( tf_bm_sky_arena.GetBool() )
	{
		extern void BM_ApplySkyPlayMovement( CTFPlayer *pPlayer );
		BM_ApplySkyPlayMovement( pPlayer );
	}
	else
	{
		pPlayer->SetMoveType( MOVETYPE_WALK );
		pPlayer->SetGravity( 1.0f );
		pPlayer->RemoveFlag( FL_FLY );
	}
}

//-----------------------------------------------------------------------------
void BM_WarpAllPlayersToArenaSpawns( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() )
		{
			BM_WarpPlayerToArenaSpawn( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
static void BM_AccumulateSpawnOrigin( const Vector &vecOrigin, float &flMinX, float &flMinY, float &flMaxX, float &flMaxY, float &flMaxZ, int &nPoints )
{
	flMinX = Min( flMinX, vecOrigin.x );
	flMinY = Min( flMinY, vecOrigin.y );
	flMaxX = Max( flMaxX, vecOrigin.x );
	flMaxY = Max( flMaxY, vecOrigin.y );
	flMaxZ = Max( flMaxZ, vecOrigin.z );
	++nPoints;
}

static bool BM_FindArenaCenter( Vector &vecCenter, float &flRefZ );

//-----------------------------------------------------------------------------
static bool BM_GetTeamSpawnAnchors( Vector &vecRed, Vector &vecBlue )
{
	vecRed = vec3_origin;
	vecBlue = vec3_origin;
	int nRed = 0;
	int nBlue = 0;
	float flBestRedCorner = -FLT_MAX;
	float flBestBlueCorner = -FLT_MAX;

	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" ) )
	{
		const Vector &vecOrigin = pEnt->GetAbsOrigin();
		const int iTeam = pEnt->GetTeamNumber();
		if ( iTeam == TF_TEAM_RED )
		{
			++nRed;
			// RED bomber corner: low X, high Y.
			const float flCornerScore = vecOrigin.y - vecOrigin.x;
			if ( flCornerScore > flBestRedCorner )
			{
				flBestRedCorner = flCornerScore;
				vecRed = vecOrigin;
			}
		}
		else if ( iTeam == TF_TEAM_BLUE )
		{
			++nBlue;
			// BLU bomber corner: high X, low Y.
			const float flCornerScore = vecOrigin.x - vecOrigin.y;
			if ( flCornerScore > flBestBlueCorner )
			{
				flBestBlueCorner = flCornerScore;
				vecBlue = vecOrigin;
			}
		}
	}

	return ( nRed > 0 && nBlue > 0 );
}

//-----------------------------------------------------------------------------
static bool BM_AlignGridOriginFromAnchors( const Vector &vecRed, const Vector &vecBlue, int iWidth, int iHeight, float flCell, Vector &vecGridOrigin )
{
	const int iRedCellX = 1;
	const int iRedCellY = iHeight - 2;
	const int iBluCellX = iWidth - 2;
	const int iBluCellY = 1;

	const float flOxRed = vecRed.x - ( iRedCellX + 0.5f ) * flCell;
	const float flOyRed = vecRed.y - ( iRedCellY + 0.5f ) * flCell;
	const float flOxBlu = vecBlue.x - ( iBluCellX + 0.5f ) * flCell;
	const float flOyBlu = vecBlue.y - ( iBluCellY + 0.5f ) * flCell;

	vecGridOrigin.x = 0.5f * ( flOxRed + flOxBlu );
	vecGridOrigin.y = 0.5f * ( flOyRed + flOyBlu );
	vecGridOrigin.z = Max( vecRed.z, vecBlue.z ) - tf_bm_play_z_offset.GetFloat();
	return true;
}

//-----------------------------------------------------------------------------
// Stock maps: huge platform offset from spawns (beside map, in PVS — not basement, not Z=4096 void).
//-----------------------------------------------------------------------------
static bool BM_ResolveArenaGridOrigin( Vector &vecGridOrigin, int iWidth, int iHeight, float flCell, Vector &vecCenter )
{
	if ( tf_bm_sky_arena.GetBool() )
	{
		float flRefZ = 0.0f;
		if ( !BM_FindArenaCenter( vecCenter, flRefZ ) )
		{
			return false;
		}

		vecGridOrigin.x = vecCenter.x - ( iWidth * flCell ) * 0.5f;
		vecGridOrigin.y = vecCenter.y - ( iHeight * flCell ) * 0.5f;
		vecGridOrigin.z = flRefZ + Max( 512.0f, tf_bm_sky_height.GetFloat() );
		return true;
	}

	if ( BM_IsMapFloorArena() )
	{
		Vector vecRed;
		Vector vecBlue;
		if ( BM_GetTeamSpawnAnchors( vecRed, vecBlue ) && BM_AlignGridOriginFromAnchors( vecRed, vecBlue, iWidth, iHeight, flCell, vecGridOrigin ) )
		{
			vecCenter.x = vecGridOrigin.x + ( iWidth * flCell ) * 0.5f;
			vecCenter.y = vecGridOrigin.y + ( iHeight * flCell ) * 0.5f;
			vecCenter.z = vecGridOrigin.z;

			CBaseEntity *pAnchor = gEntList.FindEntityByName( NULL, "bm_arena_center" );
			if ( pAnchor )
			{
				vecGridOrigin.z = pAnchor->GetAbsOrigin().z - tf_bm_play_z_offset.GetFloat();
			}
			else
			{
				Vector vecFloor;
				const float flRefZ = Max( vecRed.z, vecBlue.z );
				BM_FindFloorAtXY( vecCenter, flRefZ + 256.0f, NULL, vecFloor );
				vecGridOrigin.z = vecFloor.z - tf_bm_play_z_offset.GetFloat();
			}

			Msg( "BM arena: grid aligned to team spawns (RED %.0f %.0f, BLU %.0f %.0f).\n",
				vecRed.x, vecRed.y, vecBlue.x, vecBlue.y );
			return true;
		}

		float flRefZ = 0.0f;
		if ( !BM_FindArenaCenter( vecCenter, flRefZ ) )
		{
			return false;
		}

		vecGridOrigin.x = vecCenter.x - ( iWidth * flCell ) * 0.5f;
		vecGridOrigin.y = vecCenter.y - ( iHeight * flCell ) * 0.5f;
		Vector vecFloor;
		BM_FindFloorAtXY( vecCenter, flRefZ + 256.0f, NULL, vecFloor );
		vecGridOrigin.z = vecFloor.z - tf_bm_play_z_offset.GetFloat();
		return true;
	}

	Vector vecSpawnCenter;
	float flRefZ = 0.0f;
	if ( !BM_FindArenaCenter( vecSpawnCenter, flRefZ ) )
	{
		vecSpawnCenter = vec3_origin;
		flRefZ = 0.0f;
	}

	float flOffX = 2048.0f;
	float flOffY = 2048.0f;
	BM_GetVoidPlatformOffset( flOffX, flOffY );

	vecCenter.x = vecSpawnCenter.x + flOffX;
	vecCenter.y = vecSpawnCenter.y + flOffY;
	vecGridOrigin.x = vecCenter.x - ( iWidth * flCell ) * 0.5f;
	vecGridOrigin.y = vecCenter.y - ( iHeight * flCell ) * 0.5f;

	if ( tf_bm_platform_z.GetFloat() > 256.0f )
	{
		vecGridOrigin.z = tf_bm_platform_z.GetFloat();
	}
	else
	{
		vecGridOrigin.z = flRefZ + Max( 128.0f, tf_bm_platform_height.GetFloat() );
	}

	return true;
}

//-----------------------------------------------------------------------------
static bool BM_FindArenaCenter( Vector &vecCenter, float &flRefZ )
{
	CBaseEntity *pAnchor = gEntList.FindEntityByName( NULL, "bm_arena_center" );
	if ( pAnchor )
	{
		vecCenter = pAnchor->GetAbsOrigin();
		flRefZ = vecCenter.z;
		return true;
	}

	float flMinX = FLT_MAX;
	float flMinY = FLT_MAX;
	float flMaxX = -FLT_MAX;
	float flMaxY = -FLT_MAX;
	float flMaxZ = -FLT_MAX;
	int nPoints = 0;

	static const char *s_pszSpawnClasses[] = {
		"info_player_teamspawn",
		"info_player_start",
		"info_player_deathmatch",
	};

	for ( int iClass = 0; iClass < ARRAYSIZE( s_pszSpawnClasses ); ++iClass )
	{
		for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, s_pszSpawnClasses[iClass] );
			pEnt != NULL;
			pEnt = gEntList.FindEntityByClassname( pEnt, s_pszSpawnClasses[iClass] ) )
		{
			BM_AccumulateSpawnOrigin( pEnt->GetAbsOrigin(), flMinX, flMinY, flMaxX, flMaxY, flMaxZ, nPoints );
		}
	}

	if ( nPoints == 0 )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pPlayer || !pPlayer->IsConnected() )
			{
				continue;
			}

			BM_AccumulateSpawnOrigin( pPlayer->GetAbsOrigin(), flMinX, flMinY, flMaxX, flMaxY, flMaxZ, nPoints );
		}
	}

	if ( nPoints > 0 )
	{
		vecCenter.x = ( flMinX + flMaxX ) * 0.5f;
		vecCenter.y = ( flMinY + flMaxY ) * 0.5f;
		vecCenter.z = flMaxZ;
		flRefZ = flMaxZ;
		return true;
	}

	// Last resort: trace near world origin (itemtest and other flat maps).
	vecCenter = Vector( 0.0f, 0.0f, 0.0f );
	flRefZ = 0.0f;
	Vector vecFloor;
	BM_FindFloorAtXY( vecCenter, 4096.0f, NULL, vecFloor );
	vecCenter.z = vecFloor.z;
	flRefZ = vecFloor.z;
	Warning( "BM arena: no spawns — using traced floor at (%.0f %.0f %.0f).\n", vecCenter.x, vecCenter.y, vecCenter.z );
	return true;
}

//-----------------------------------------------------------------------------
static void BM_ClearSkySpawn( void )
{
	if ( s_pBMSkySpawn )
	{
		UTIL_Remove( s_pBMSkySpawn );
		s_pBMSkySpawn = NULL;
	}
}

//-----------------------------------------------------------------------------
void BM_RemoveAllBombs( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer )
		{
			pPlayer->m_iBMActiveBombs = 0;
		}
	}

	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_bm_bomb" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_bomb" ) )
	{
		UTIL_Remove( pEnt );
	}
}

//-----------------------------------------------------------------------------
void BM_ClearArena( void )
{
	BM_RemoveAllBombs();

	CTFBMWall::RemoveAllWalls();
	CTFBMCrate::RemoveAllCrates();
	CTFBMFloor::RemoveAllFloors();
	BM_RemoveStrayArenaProps();
	BM_ClearSkySpawn();
	s_bArenaActive = false;
	s_iArenaWidth = 0;
	s_iArenaHeight = 0;
}

//-----------------------------------------------------------------------------
void BM_BuildArena( bool bWarpAllPlayers )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	BM_ClearArena();

	s_iArenaWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	if ( s_iArenaWidth % 2 == 0 )
	{
		++s_iArenaWidth;
	}

	s_iArenaHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	if ( s_iArenaHeight % 2 == 0 )
	{
		++s_iArenaHeight;
	}

	if ( BM_IsDedicatedArenaMap() )
	{
		s_iArenaWidth = Min( s_iArenaWidth, 15 );
		s_iArenaHeight = Min( s_iArenaHeight, 13 );
	}

	const float flCell = BM_GetCellSize();
	const float flFill = clamp( tf_bm_arena_soft_fill.GetFloat(), 0.0f, 1.0f );

	Vector vecCenter;
	Vector vecGridOrigin;
	if ( !BM_ResolveArenaGridOrigin( vecGridOrigin, s_iArenaWidth, s_iArenaHeight, flCell, vecCenter ) )
	{
		return;
	}

	char szOrigin[64];
	Q_snprintf( szOrigin, sizeof( szOrigin ), "%.1f %.1f %.1f", vecGridOrigin.x, vecGridOrigin.y, vecGridOrigin.z );
	tf_bm_grid_origin.SetValue( szOrigin );

	extern void BM_ResetGridAlign( void );
	extern void BM_MarkGridAligned( void );
	BM_ResetGridAlign();
	BM_MarkGridAligned();
	s_bArenaActive = true;

	int nWalls = 0;
	int nCrates = 0;

	for ( int iCellX = 0; iCellX < s_iArenaWidth; ++iCellX )
	{
		for ( int iCellY = 0; iCellY < s_iArenaHeight; ++iCellY )
		{
			if ( BM_IsHardWallCell( iCellX, iCellY ) )
			{
				if ( CTFBMWall::CreateAtCell( iCellX, iCellY ) != NULL )
				{
					++nWalls;
				}
				continue;
			}

			if ( BM_IsSpawnSafeCell( iCellX, iCellY ) || BM_IsNearSpawnCell( iCellX, iCellY ) )
			{
				continue;
			}

			if ( flFill > 0.0f && RandomFloat( 0.0f, 1.0f ) <= flFill )
			{
				if ( CTFBMCrate::CreateAtCell( iCellX, iCellY ) != NULL )
				{
					++nCrates;
				}
			}
		}
	}

	const float flArenaW = s_iArenaWidth * flCell;
	const float flArenaD = s_iArenaHeight * flCell;
	Vector vecArenaCenter(
		vecGridOrigin.x + flArenaW * 0.5f,
		vecGridOrigin.y + flArenaD * 0.5f,
		vecGridOrigin.z );
	const float flPlayZ = vecGridOrigin.z + tf_bm_play_z_offset.GetFloat();
	if ( CTFBMFloor::CreateForArena( vecArenaCenter, flArenaW + flCell * 2.0f, flArenaD + flCell * 2.0f, flPlayZ ) != NULL )
	{
		Msg( "BM arena: solid play floor at Z=%.0f.\n", flPlayZ );
	}

	BM_SpawnArenaVisuals( vecArenaCenter, flArenaW, flArenaD, flPlayZ );

	Msg( "BM arena: %dx%d at %s — %d hard walls, %d soft crates (sky=%d).\n",
		s_iArenaWidth, s_iArenaHeight, szOrigin, nWalls, nCrates, tf_bm_sky_arena.GetInt() );
	if ( tf_bm_sky_arena.GetBool() )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: %dx%d sky layer (Z=%.0f) — legacy mode.", s_iArenaWidth, s_iArenaHeight, vecGridOrigin.z ) );
	}
	else if ( BM_UseVoidArenaPlatform() )
	{
		const char *pszMap = STRING( gpGlobals->mapname );
		if ( pszMap && Q_stricmp( pszMap, "itemtest" ) == 0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: void arena at %.0f %.0f Z=%.0f (tf_bm_void_arena 1). Use 0 for itemtest floor.",
				vecCenter.x, vecCenter.y, flPlayZ ) );
		}
		else
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: isolated %dx%d platform at %.0f %.0f Z=%.0f — JOIN RED/BLU Scout!",
				s_iArenaWidth, s_iArenaHeight, vecCenter.x, vecCenter.y, flPlayZ ) );
		}
	}
	else
	{
		const char *pszMap = STRING( gpGlobals->mapname );
		if ( pszMap && Q_stricmp( pszMap, "itemtest" ) == 0 )
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: %dx%d on itemtest floor at %.0f %.0f Z=%.0f — join RED/BLU Scout.",
				s_iArenaWidth, s_iArenaHeight, vecCenter.x, vecCenter.y, flPlayZ ) );
		}
		else
		{
			UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: %dx%d on map floor (Z=%.0f).", s_iArenaWidth, s_iArenaHeight, flPlayZ ) );
		}
	}

	if ( bWarpAllPlayers )
	{
		BM_WarpAllPlayersToArenaSpawns();
	}
}

#endif // SOURCESDK
