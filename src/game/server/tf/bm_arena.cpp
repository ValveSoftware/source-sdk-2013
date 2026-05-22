//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_arena.h"
#include "bm_grid.h"
#include "bm_player_system.h"
#include "bm_shareddefs.h"
#include "tf_bm_crate.h"
#include "tf_bm_wall.h"
#include "tf_bm_floor.h"
#include "bm_props.h"
#include "tf_gamerules.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_arena_width( "tf_bm_arena_width", "11", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena width in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "35", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman arena height in grid cells (odd, includes border walls)." );
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
extern ConVar tf_ff_game_mode;

static bool s_bArenaActive = false;
static bool s_bBMPostMapArenaReady = false;
static int s_iArenaWidth = 0;
static int s_iArenaHeight = 0;
static CBaseEntity *s_pBMSkySpawn = NULL;

//-----------------------------------------------------------------------------
// Arena lifecycle (single source of truth — do not stack rebuilds):
//   1) FF_TickPostMapSetup pass 0: exec mode_bomber.cfg
//   2) pass 1: BM_BuildArena( force ) — only authoritative itemtest placement
//   3) gameplay: BM_EnsureArenaBuilt() — reuse grid, no ClearArena
//   4) bm_fix: BM_BuildArena( warp, force )
// itemtest XY always from BM_GetItemtestPlayRoomBounds (never team spawns).
//-----------------------------------------------------------------------------

// itemtest Hammer room fallback (override with tf_bm_room_*). Active arena uses grid footprint.
static const float BM_ITEMTEST_ROOM_MIN_X = 1304.03125f;
static const float BM_ITEMTEST_ROOM_MIN_Y = -2535.97412f;
static const float BM_ITEMTEST_ROOM_MAX_X = 2023.96875f;
static const float BM_ITEMTEST_ROOM_MAX_Y = -280.03979f;

static bool BM_IsNearSpawnCell( int iCellX, int iCellY );
static CBaseEntity *BM_FindMapTeamSpawn( CTFPlayer *pPlayer );
static bool BM_IsArenaConfigValid( void );
static void BM_GetItemtestPlayRoomBounds( float &flMinX, float &flMinY, float &flMaxX, float &flMaxY, float &flCenterX, float &flCenterY );
static void BM_GetArenaDimensions( int &iWidth, int &iHeight );
static void BM_GetItemtestFitArenaDimensions( int &iWidth, int &iHeight );
static void BM_ComputeItemtestExpectedGridOrigin( int iWidth, int iHeight, float flCell, Vector &vecGridOrigin, Vector &vecCenter );

//-----------------------------------------------------------------------------
// itemtest only: align grid to team spawns on map floor geometry.
//-----------------------------------------------------------------------------
bool BM_IsMapFloorArena( void )
{
	const char *pszMap = STRING( gpGlobals->mapname );
	return ( pszMap && Q_stricmp( pszMap, "itemtest" ) == 0 );
}

//-----------------------------------------------------------------------------
void BM_GetPlayAreaWorldBounds( float &flMinX, float &flMinY, float &flMaxX, float &flMaxY )
{
	if ( s_bArenaActive && s_iArenaWidth > 0 && s_iArenaHeight > 0 )
	{
		Vector vecGridOrigin;
		BM_GetGridOrigin( vecGridOrigin );
		const float flCell = BM_GetCellSize();
		flMinX = vecGridOrigin.x;
		flMinY = vecGridOrigin.y;
		flMaxX = vecGridOrigin.x + s_iArenaWidth * flCell;
		flMaxY = vecGridOrigin.y + s_iArenaHeight * flCell;
		return;
	}

	float flCenterX = 0.0f;
	float flCenterY = 0.0f;
	BM_GetItemtestPlayRoomBounds( flMinX, flMinY, flMaxX, flMaxY, flCenterX, flCenterY );
}

//-----------------------------------------------------------------------------
bool BM_IsInsideItemtestPlayRoom( const Vector &vecPos )
{
	if ( !BM_IsMapFloorArena() )
	{
		return false;
	}

	float flMinX = 0.0f;
	float flMinY = 0.0f;
	float flMaxX = 0.0f;
	float flMaxY = 0.0f;
	float flCenterX = 0.0f;
	float flCenterY = 0.0f;
	BM_GetItemtestPlayRoomBounds( flMinX, flMinY, flMaxX, flMaxY, flCenterX, flCenterY );

	const float flMargin = 8.0f;
	return ( vecPos.x >= flMinX + flMargin && vecPos.x <= flMaxX - flMargin
		&& vecPos.y >= flMinY + flMargin && vecPos.y <= flMaxY - flMargin );
}

//-----------------------------------------------------------------------------
static void BM_GetItemtestFitArenaDimensions( int &iWidth, int &iHeight )
{
	float flMinX = 0.0f;
	float flMinY = 0.0f;
	float flMaxX = 0.0f;
	float flMaxY = 0.0f;
	float flCenterX = 0.0f;
	float flCenterY = 0.0f;
	BM_GetItemtestPlayRoomBounds( flMinX, flMinY, flMaxX, flMaxY, flCenterX, flCenterY );

	const float flCell = BM_GetCellSize();
	int w = (int)floorf( ( flMaxX - flMinX ) / flCell );
	int h = (int)floorf( ( flMaxY - flMinY ) / flCell );
	if ( w % 2 == 0 )
	{
		++w;
	}
	if ( h % 2 == 0 )
	{
		++h;
	}

	iWidth = clamp( w, 7, 51 );
	iHeight = clamp( h, 7, 51 );
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
static void BM_GetArenaDimensions( int &iWidth, int &iHeight )
{
	if ( s_bArenaActive && s_iArenaWidth > 0 && s_iArenaHeight > 0 )
	{
		iWidth = s_iArenaWidth;
		iHeight = s_iArenaHeight;
		return;
	}

	iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	if ( iWidth % 2 == 0 )
	{
		++iWidth;
	}
	if ( iHeight % 2 == 0 )
	{
		++iHeight;
	}
}

//-----------------------------------------------------------------------------
static void BM_GetArenaSpawnCell( CTFPlayer *pPlayer, int &iCellX, int &iCellY )
{
	iCellX = 1;
	iCellY = 1;

	if ( !pPlayer )
	{
		return;
	}

	int iWidth = 0;
	int iHeight = 0;
	BM_GetArenaDimensions( iWidth, iHeight );

	const int iSlot = BM_GetPlayerSpawnSlot( pPlayer );
	if ( BM_IsFreeForAll() )
	{
		BM_GetSpawnCellForCorner( iSlot % 4, iSlot, iWidth, iHeight, iCellX, iCellY );
	}
	else
	{
		const bool bBlueTeam = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE );
		BM_GetSpawnCellForSlot( bBlueTeam, iSlot, iWidth, iHeight, iCellX, iCellY );
	}
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

	// Keep crates clear of all four play-room corners on itemtest.
	if ( BM_IsMapFloorArena() )
	{
		if ( iCellX <= 1 + iMargin && iCellY >= iMaxY - iMargin )
		{
			return true;
		}
		if ( iCellX >= iMaxX - iMargin && iCellY >= iMaxY - iMargin )
		{
			return true;
		}
		if ( iCellX >= iMaxX - iMargin && iCellY <= 1 + iMargin )
		{
			return true;
		}
		if ( iCellX <= 1 + iMargin && iCellY <= 1 + iMargin )
		{
			return true;
		}
		return false;
	}

	if ( iCellX <= 1 + iMargin && iCellY >= iMaxY - iMargin )
	{
		return true;
	}

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
//-----------------------------------------------------------------------------
static void BM_ComputeItemtestExpectedGridOrigin( int iWidth, int iHeight, float flCell, Vector &vecGridOrigin, Vector &vecCenter )
{
	float flMinX = 0.0f;
	float flMinY = 0.0f;
	float flMaxX = 0.0f;
	float flMaxY = 0.0f;
	BM_GetItemtestPlayRoomBounds( flMinX, flMinY, flMaxX, flMaxY, vecCenter.x, vecCenter.y );
	vecGridOrigin.x = vecCenter.x - ( iWidth * flCell ) * 0.5f;
	vecGridOrigin.y = vecCenter.y - ( iHeight * flCell ) * 0.5f;
	vecGridOrigin.z = BM_GetItemtestPlayFloorGridZ();
	vecCenter.z = vecGridOrigin.z;
}

//-----------------------------------------------------------------------------
static bool BM_IsArenaConfigValid( void )
{
	if ( !s_bArenaActive || s_iArenaWidth <= 0 || s_iArenaHeight <= 0 )
	{
		return false;
	}

	int iWantW = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	int iWantH = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	if ( BM_IsMapFloorArena() )
	{
		BM_GetItemtestFitArenaDimensions( iWantW, iWantH );
	}
	else
	{
		if ( iWantW % 2 == 0 )
		{
			++iWantW;
		}
		if ( iWantH % 2 == 0 )
		{
			++iWantH;
		}
	}

	if ( iWantW != s_iArenaWidth || iWantH != s_iArenaHeight )
	{
		return false;
	}

	if ( BM_IsMapFloorArena() )
	{
		Vector vecExpectedOrigin;
		Vector vecExpectedCenter;
		BM_ComputeItemtestExpectedGridOrigin( s_iArenaWidth, s_iArenaHeight, BM_GetCellSize(), vecExpectedOrigin, vecExpectedCenter );

		Vector vecCurrentOrigin;
		BM_GetGridOrigin( vecCurrentOrigin );

		const float flXYTol = BM_GetCellSize() * 0.25f;
		if ( fabsf( vecCurrentOrigin.x - vecExpectedOrigin.x ) > flXYTol
			|| fabsf( vecCurrentOrigin.y - vecExpectedOrigin.y ) > flXYTol
			|| fabsf( vecCurrentOrigin.z - vecExpectedOrigin.z ) > 2.0f )
		{
			Warning( "BM arena: grid origin (%.0f %.0f %.0f) != play room (%.0f %.0f %.0f) — needs rebuild.\n",
				vecCurrentOrigin.x, vecCurrentOrigin.y, vecCurrentOrigin.z,
				vecExpectedOrigin.x, vecExpectedOrigin.y, vecExpectedOrigin.z );
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
bool BM_EnsureArenaBuilt( void )
{
	if ( !BM_IsBomberGameplayActive() )
	{
		return false;
	}

	if ( BM_IsArenaConfigValid() )
	{
		return true;
	}

	if ( !s_bBMPostMapArenaReady )
	{
		return false;
	}

	BM_BuildArena( false, false );
	return s_bArenaActive;
}

//-----------------------------------------------------------------------------
static void BM_GetItemtestPlayRoomBounds( float &flMinX, float &flMinY, float &flMaxX, float &flMaxY, float &flCenterX, float &flCenterY )
{
	flMinX = BM_ITEMTEST_ROOM_MIN_X;
	flMinY = BM_ITEMTEST_ROOM_MIN_Y;
	flMaxX = BM_ITEMTEST_ROOM_MAX_X;
	flMaxY = BM_ITEMTEST_ROOM_MAX_Y;

	// Optional cfg override (FindVar — safe if ConVar lives in another TU).
	if ( g_pCVar )
	{
		ConVar *pMinX = g_pCVar->FindVar( "tf_bm_room_min_x" );
		ConVar *pMinY = g_pCVar->FindVar( "tf_bm_room_min_y" );
		ConVar *pMaxX = g_pCVar->FindVar( "tf_bm_room_max_x" );
		ConVar *pMaxY = g_pCVar->FindVar( "tf_bm_room_max_y" );
		if ( pMinX && pMinY && pMaxX && pMaxY )
		{
			const float flCfgMinX = pMinX->GetFloat();
			const float flCfgMinY = pMinY->GetFloat();
			const float flCfgMaxX = pMaxX->GetFloat();
			const float flCfgMaxY = pMaxY->GetFloat();
			if ( flCfgMaxX > flCfgMinX + 128.0f && flCfgMaxY > flCfgMinY + 128.0f )
			{
				flMinX = flCfgMinX;
				flMinY = flCfgMinY;
				flMaxX = flCfgMaxX;
				flMaxY = flCfgMaxY;
			}
		}
	}

	flCenterX = 0.5f * ( flMinX + flMaxX );
	flCenterY = 0.5f * ( flMinY + flMaxY );
}

//-----------------------------------------------------------------------------
CBaseEntity *BM_GetSkySpawnEntity( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !BM_IsBomberGameplayActive() )
	{
		return NULL;
	}

	if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		return NULL;
	}

	Vector vecSkySpawn;
	if ( !BM_ComputeArenaSpawnWorldPos( pPlayer, vecSkySpawn ) )
	{
		return NULL;
	}

	// Map teamspawns on itemtest sit in the roof — always use grid cells for XY/Z.
	if ( !BM_IsMapFloorArena() )
	{
		CBaseEntity *pMapSpawn = BM_FindMapTeamSpawn( pPlayer );
		if ( pMapSpawn )
		{
			return pMapSpawn;
		}
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

	if ( BM_IsMapFloorArena() )
	{
		BM_ClearHullFromWorld( vecSkySpawn, pPlayer );
	}
	s_pBMSkySpawn->SetAbsOrigin( vecSkySpawn );
	s_pBMSkySpawn->SetAbsAngles( vec3_angle );
	s_pBMSkySpawn->ChangeTeam( pPlayer->GetTeamNumber() );

	return s_pBMSkySpawn;
}

//-----------------------------------------------------------------------------
bool BM_IsBomberGameplayActive( void )
{
	if ( !TFGameRules() )
	{
		return false;
	}

	if ( tf_ff_game_mode.GetInt() == TF_FF_MODE_BOMBERMAN )
	{
		return true;
	}

	return TFGameRules()->IsBombermanMode();
}

//-----------------------------------------------------------------------------
static float s_flBMSpawnApplyTime[MAX_PLAYERS + 1];

//-----------------------------------------------------------------------------
void BM_ResetArenaSpawnDebounce( CTFPlayer *pPlayer )
{
	if ( !pPlayer )
	{
		return;
	}

	const int iIndex = pPlayer->entindex();
	if ( iIndex >= 0 && iIndex <= MAX_PLAYERS )
	{
		s_flBMSpawnApplyTime[iIndex] = 0.0f;
	}
}

//-----------------------------------------------------------------------------
bool BM_ComputeArenaSpawnWorldPos( CTFPlayer *pPlayer, Vector &vecDest )
{
	if ( !pPlayer || !BM_IsBomberGameplayActive() )
	{
		return false;
	}

	if ( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE )
	{
		return false;
	}

	if ( BM_IsMapFloorArena() )
	{
		int iWidth = 0;
		int iHeight = 0;
		BM_GetArenaDimensions( iWidth, iHeight );

		int iCellX = 0;
		int iCellY = 0;
		const int iSlot = BM_GetPlayerSpawnSlot( pPlayer );
		if ( BM_IsFreeForAll() )
		{
			BM_GetSpawnCellForCorner( iSlot % 4, iSlot, iWidth, iHeight, iCellX, iCellY );
		}
		else
		{
			const bool bBlueTeam = ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE );
			BM_GetSpawnCellForSlot( bBlueTeam, iSlot, iWidth, iHeight, iCellX, iCellY );
		}

		BM_CellToWorldCenter( iCellX, iCellY, vecDest );
		return true;
	}

	if ( !s_bArenaActive )
	{
		return false;
	}

	int iCellX = 0;
	int iCellY = 0;
	BM_GetArenaSpawnCell( pPlayer, iCellX, iCellY );
	BM_CellToWorldCenter( iCellX, iCellY, vecDest );
	return true;
}

//-----------------------------------------------------------------------------
bool BM_ApplyArenaSpawnToPlayer( CTFPlayer *pPlayer )
{
	if ( !pPlayer || BM_IsPlayerMovementUnlocked( pPlayer ) )
	{
		return false;
	}

	const int iIndex = pPlayer->entindex();
	if ( iIndex >= 0 && iIndex <= MAX_PLAYERS && gpGlobals->curtime < s_flBMSpawnApplyTime[iIndex] )
	{
		return false;
	}

	Vector vecDest;
	if ( !BM_ComputeArenaSpawnWorldPos( pPlayer, vecDest ) )
	{
		return false;
	}

	if ( BM_IsMapFloorArena() )
	{
		vecDest.z = BM_GetPlayPlaneZ();
		BM_ClearHullFromWorld( vecDest, pPlayer );
		vecDest.z = BM_GetPlayPlaneZ();
	}
	else if ( !BM_UseVoidArenaPlatform() )
	{
		const float flPlayZ = BM_GetPlayPlaneZ();
		trace_t trace;
		Vector vecTraceStart( vecDest.x, vecDest.y, flPlayZ + 96.0f );
		Vector vecTraceEnd( vecDest.x, vecDest.y, flPlayZ - 256.0f );
		UTIL_TraceHull( vecTraceStart, vecTraceEnd, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER, &trace );
		vecDest.z = trace.DidHit() ? trace.endpos.z : flPlayZ;
		BM_ClearHullFromWorld( vecDest, pPlayer );
	}

	pPlayer->SetGroundEntity( NULL );

	const QAngle angEyes = pPlayer->EyeAngles();
	pPlayer->Teleport( &vecDest, &angEyes, &vec3_origin );
	pPlayer->SetLocalOrigin( vecDest );
	pPlayer->SetAbsOrigin( vecDest );

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

	if ( iIndex >= 0 && iIndex <= MAX_PLAYERS )
	{
		s_flBMSpawnApplyTime[iIndex] = gpGlobals->curtime + 0.5f;
	}

	int iLogCellX = 0;
	int iLogCellY = 0;
	BM_WorldToCell( vecDest, iLogCellX, iLogCellY );
	Msg( "BM spawn: %s -> %.0f %.0f %.0f (grid cell %d,%d)\n",
		pPlayer->GetPlayerName(), vecDest.x, vecDest.y, vecDest.z, iLogCellX, iLogCellY );
	return true;
}

//-----------------------------------------------------------------------------
void BM_WarpPlayerToArenaSpawn( CTFPlayer *pPlayer )
{
	BM_EnsureArenaBuilt();
	BM_ApplyArenaSpawnToPlayer( pPlayer );
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
		float flMinX = 0.0f;
		float flMinY = 0.0f;
		float flMaxX = 0.0f;
		float flMaxY = 0.0f;
		float flBoxCenterX = 0.0f;
		float flBoxCenterY = 0.0f;
		BM_ComputeItemtestExpectedGridOrigin( iWidth, iHeight, flCell, vecGridOrigin, vecCenter );
		BM_GetItemtestPlayRoomBounds( flMinX, flMinY, flMaxX, flMaxY, flBoxCenterX, flBoxCenterY );

		Msg( "BM arena: itemtest PLAY ROOM (%.0f,%.0f)-(%.0f,%.0f) center (%.0f %.0f) floor Z=%.0f origin (%.0f %.0f %.0f).\n",
			flMinX, flMinY, flMaxX, flMaxY,
			vecCenter.x, vecCenter.y, vecGridOrigin.z + tf_bm_play_z_offset.GetFloat(),
			vecGridOrigin.x, vecGridOrigin.y, vecGridOrigin.z );
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
	s_bBMPostMapArenaReady = false;
	s_iArenaWidth = 0;
	s_iArenaHeight = 0;
}

//-----------------------------------------------------------------------------
void BM_BuildArena( bool bWarpAllPlayers, bool bForceRebuild )
{
	if ( !BM_IsBomberGameplayActive() )
	{
		return;
	}

	if ( !bForceRebuild && BM_IsArenaConfigValid() )
	{
		if ( bWarpAllPlayers )
		{
			BM_WarpAllPlayersToArenaSpawns();
		}
		return;
	}

	if ( !bForceRebuild && !s_bBMPostMapArenaReady )
	{
		return;
	}

	BM_ClearArena();

	if ( BM_IsMapFloorArena() )
	{
		BM_GetItemtestFitArenaDimensions( s_iArenaWidth, s_iArenaHeight );
	}
	else
	{
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
	tf_bm_arena_width.SetValue( s_iArenaWidth );
	tf_bm_arena_height.SetValue( s_iArenaHeight );

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

	s_bBMPostMapArenaReady = true;

	if ( bWarpAllPlayers )
	{
		BM_WarpAllPlayersToArenaSpawns();
	}
}

#endif // SOURCESDK
