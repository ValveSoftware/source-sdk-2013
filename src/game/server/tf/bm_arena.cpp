//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_arena.h"
#include "bm_grid.h"
#include "tf_bm_crate.h"
#include "tf_bm_wall.h"
#include "tf_gamerules.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_arena_width( "tf_bm_arena_width", "15", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena width in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "13", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena height in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_soft_fill( "tf_bm_arena_soft_fill", "0.55", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: chance to place a soft crate in empty interior cells." );

extern ConVar tf_bm_grid_origin;
extern ConVar tf_bm_sky_arena;
extern ConVar tf_bm_sky_height;

static bool s_bArenaActive = false;
static int s_iArenaWidth = 0;
static int s_iArenaHeight = 0;

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
		return true;
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

	const int iSpawnCells[4][2] = {
		{ 1, 1 },
		{ 1, s_iArenaHeight - 2 },
		{ s_iArenaWidth - 2, 1 },
		{ s_iArenaWidth - 2, s_iArenaHeight - 2 },
	};

	for ( int i = 0; i < 4; ++i )
	{
		if ( iSpawnCells[i][0] == iCellX && iSpawnCells[i][1] == iCellY )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
static bool BM_IsNearSpawnCell( int iCellX, int iCellY )
{
	if ( !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		return false;
	}

	const int iSpawnCells[4][2] = {
		{ 1, 1 },
		{ 1, s_iArenaHeight - 2 },
		{ s_iArenaWidth - 2, 1 },
		{ s_iArenaWidth - 2, s_iArenaHeight - 2 },
	};

	const int iMargin = 2;
	for ( int i = 0; i < 4; ++i )
	{
		if ( abs( iCellX - iSpawnCells[i][0] ) <= iMargin && abs( iCellY - iSpawnCells[i][1] ) <= iMargin )
		{
			return true;
		}
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
void BM_WarpPlayerToArenaSpawn( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !s_bArenaActive )
	{
		return;
	}

	int iCellX = 1;
	int iCellY = 1;

	if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
	{
		iCellX = s_iArenaWidth - 2;
		iCellY = s_iArenaHeight - 2;
	}
	else if ( pPlayer->GetTeamNumber() == TF_TEAM_RED )
	{
		iCellX = 1;
		iCellY = s_iArenaHeight - 2;
	}

	Vector vecDest;
	BM_CellToWorldCenter( iCellX, iCellY, vecDest );

	const QAngle angFace( 0.0f, 0.0f, 0.0f );
	pPlayer->Teleport( &vecDest, &angFace, &vec3_origin );
	pPlayer->SnapEyeAngles( angFace );
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

//-----------------------------------------------------------------------------
static bool BM_FindArenaCenter( Vector &vecCenter, float &flRefZ )
{
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
void BM_ClearArena( void )
{
	CTFBMWall::RemoveAllWalls();
	CTFBMCrate::RemoveAllCrates();
	s_bArenaActive = false;
	s_iArenaWidth = 0;
	s_iArenaHeight = 0;
}

//-----------------------------------------------------------------------------
void BM_BuildArena( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	BM_ClearArena();

	s_iArenaWidth = clamp( tf_bm_arena_width.GetInt(), 7, 31 );
	if ( s_iArenaWidth % 2 == 0 )
	{
		++s_iArenaWidth;
	}

	s_iArenaHeight = clamp( tf_bm_arena_height.GetInt(), 7, 25 );
	if ( s_iArenaHeight % 2 == 0 )
	{
		++s_iArenaHeight;
	}

	const float flCell = BM_GetCellSize();
	const float flFill = clamp( tf_bm_arena_soft_fill.GetFloat(), 0.0f, 1.0f );

	Vector vecCenter;
	float flRefZ = 0.0f;
	if ( !BM_FindArenaCenter( vecCenter, flRefZ ) )
	{
		return;
	}

	Vector vecGridOrigin;
	vecGridOrigin.x = vecCenter.x - ( s_iArenaWidth * flCell ) * 0.5f;
	vecGridOrigin.y = vecCenter.y - ( s_iArenaHeight * flCell ) * 0.5f;

	if ( tf_bm_sky_arena.GetBool() )
	{
		vecGridOrigin.z = flRefZ + Max( 512.0f, tf_bm_sky_height.GetFloat() );
	}
	else
	{
		Vector vecFloor;
		BM_FindFloorAtXY( vecCenter, flRefZ, NULL, vecFloor );
		vecGridOrigin.z = vecFloor.z;
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

	Msg( "BM arena: %dx%d at %s — %d hard walls, %d soft crates (sky=%d height=%.0f).\n",
		s_iArenaWidth, s_iArenaHeight, szOrigin, nWalls, nCrates, tf_bm_sky_arena.GetInt(), tf_bm_sky_height.GetFloat() );
	if ( tf_bm_sky_arena.GetBool() )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: %dx%d sky arena (Z=%.0f) — flat play layer!", s_iArenaWidth, s_iArenaHeight, vecGridOrigin.z ) );
	}
	else
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber arena %dx%d — borders + crates ready!", s_iArenaWidth, s_iArenaHeight ) );
	}

	BM_WarpAllPlayersToArenaSpawns();
}

#endif // SOURCESDK
