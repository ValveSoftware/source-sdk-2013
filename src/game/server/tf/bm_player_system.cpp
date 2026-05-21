//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_player_system.h"
#include "bm_grid.h"
#include "tf_bm_bomb.h"
#include "bm_arena.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_weaponbase.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_bm_bomb_fuse;
extern ConVar tf_bm_bomb_range;
extern ConVar tf_bm_max_bombs;

ConVar tf_bm_cell_size( "tf_bm_cell_size", "64", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: grid cell size in Hammer units." );
ConVar tf_bm_grid_origin( "tf_bm_grid_origin", "0 0 0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: world origin of cell (0,0). Auto-set from team spawns on map load." );
ConVar tf_bm_sky_arena( "tf_bm_sky_arena", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: play on a flat sky layer (no terrain traces)." );
ConVar tf_bm_sky_height( "tf_bm_sky_height", "3072", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: sky play plane height above map reference Z." );
ConVar tf_bm_play_z_offset( "tf_bm_play_z_offset", "8", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: player feet offset above grid origin Z." );
ConVar tf_bm_move_speed( "tf_bm_move_speed", "320", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: movement speed along one axis." );
ConVar tf_bm_snap_move_interval( "tf_bm_snap_move_interval", "0.12", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: seconds between grid position snaps while moving." );

static bool s_bBMGridAligned = false;

//-----------------------------------------------------------------------------
void BM_MarkGridAligned( void )
{
	s_bBMGridAligned = true;
}

//-----------------------------------------------------------------------------
void BM_GetGridOrigin( Vector &vecGridOrigin )
{
	sscanf( tf_bm_grid_origin.GetString(), "%f %f %f", &vecGridOrigin.x, &vecGridOrigin.y, &vecGridOrigin.z );
}

//-----------------------------------------------------------------------------
float BM_GetCellSize( void )
{
	return Max( 32.0f, tf_bm_cell_size.GetFloat() );
}

//-----------------------------------------------------------------------------
bool BM_UseSkyPlayPlane( void )
{
	return tf_bm_sky_arena.GetBool();
}

//-----------------------------------------------------------------------------
float BM_GetPlayPlaneZ( void )
{
	Vector vecGridOrigin;
	BM_GetGridOrigin( vecGridOrigin );
	return vecGridOrigin.z + tf_bm_play_z_offset.GetFloat();
}

//-----------------------------------------------------------------------------
void BM_WorldToCell( const Vector &vecWorld, int &iCellX, int &iCellY )
{
	Vector vecGridOrigin;
	BM_GetGridOrigin( vecGridOrigin );
	const float flCell = BM_GetCellSize();

	iCellX = Floor2Int( ( vecWorld.x - vecGridOrigin.x ) / flCell );
	iCellY = Floor2Int( ( vecWorld.y - vecGridOrigin.y ) / flCell );
}

//-----------------------------------------------------------------------------
void BM_CellToWorldCenter( int iCellX, int iCellY, Vector &vecCenter )
{
	Vector vecGridOrigin;
	BM_GetGridOrigin( vecGridOrigin );
	const float flCell = BM_GetCellSize();

	vecCenter.x = vecGridOrigin.x + ( iCellX + 0.5f ) * flCell;
	vecCenter.y = vecGridOrigin.y + ( iCellY + 0.5f ) * flCell;
	vecCenter.z = vecGridOrigin.z + tf_bm_play_z_offset.GetFloat();

	if ( !BM_UseSkyPlayPlane() )
	{
		BM_FindFloorAtXY( vecCenter, vecGridOrigin.z, NULL, vecCenter );
	}
}

//-----------------------------------------------------------------------------
void BM_FindFloorAtXY( const Vector &vecXY, float flRefZ, CTFPlayer *pPlayer, Vector &vecFloor )
{
	if ( BM_UseSkyPlayPlane() && TFGameRules() && TFGameRules()->IsBombermanMode() )
	{
		vecFloor = vecXY;
		vecFloor.z = flRefZ + tf_bm_play_z_offset.GetFloat();
		return;
	}

	Vector vecStart( vecXY.x, vecXY.y, flRefZ + 512.0f );
	Vector vecEnd( vecXY.x, vecXY.y, flRefZ - 4096.0f );

	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER, &trace );

	vecFloor = vecXY;
	vecFloor.z = trace.DidHit() ? ( trace.endpos.z + 8.0f ) : flRefZ;
}

//-----------------------------------------------------------------------------
CTFBMBomb *BM_FindBombAtCell( int iCellX, int iCellY )
{
	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "tf_bm_bomb" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "tf_bm_bomb" ) )
	{
		CTFBMBomb *pBomb = assert_cast<CTFBMBomb *>( pEnt );
		if ( pBomb && pBomb->m_iCellX == iCellX && pBomb->m_iCellY == iCellY )
		{
			return pBomb;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
bool BM_IsBlastBlockedToCell( int iFromCellX, int iFromCellY, int iToCellX, int iToCellY, CTFPlayer *pPlayer )
{
	Vector vecFrom;
	Vector vecTo;
	BM_CellToWorldCenter( iFromCellX, iFromCellY, vecFrom );
	BM_CellToWorldCenter( iToCellX, iToCellY, vecTo );

	Vector vecStart = vecFrom + Vector( 0, 0, 32.0f );
	Vector vecEnd = vecTo + Vector( 0, 0, 32.0f );

	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ), MASK_SOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &trace );

	return ( trace.fraction < 0.95f );
}

//-----------------------------------------------------------------------------
void BM_ResetGridAlign( void )
{
	s_bBMGridAligned = false;
}

//-----------------------------------------------------------------------------
void BM_AutoAlignGridFromSpawns( void )
{
	// Arena build sets grid origin; this early pass only helps before the round starts.
	if ( BM_IsArenaActive() )
	{
		BM_MarkGridAligned();
		return;
	}

	BM_BuildArena();
}

//-----------------------------------------------------------------------------
void BM_ConfigureMatch( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	extern ConVar tf_bot_quota;
	extern ConVar mp_autoteambalance;
	tf_bot_quota.SetValue( 0 );
	mp_autoteambalance.SetValue( 0 );

	BM_BuildArena();

	UTIL_ClientPrintAll( HUD_PRINTTALK, "Frog Bomber: classic arena — hard walls, soft crates!" );
	Msg( "BM: match started — cell=%.0f origin=(%s) fuse=%.1fs range=%d\n",
		BM_GetCellSize(), tf_bm_grid_origin.GetString(), tf_bm_bomb_fuse.GetFloat(), tf_bm_bomb_range.GetInt() );
}

//-----------------------------------------------------------------------------
void BM_RespawnAllPlayers( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsConnected() || pPlayer->GetTeamNumber() < FIRST_GAME_TEAM )
		{
			continue;
		}

		pPlayer->m_iBMActiveBombs = 0;
		if ( pPlayer->IsAlive() )
		{
			if ( BM_IsArenaActive() )
			{
				BM_WarpPlayerToArenaSpawn( pPlayer );
			}
		}
		else
		{
			pPlayer->ForceRespawn();
		}
	}
}

//-----------------------------------------------------------------------------
void BM_FixMatch( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		Warning( "bm_fix: not in bomber mode.\n" );
		return;
	}

	BM_BuildArena();
	BM_RespawnAllPlayers();
	UTIL_ClientPrintAll( HUD_PRINTTALK, "Frog Bomber: arena rebuilt — pick RED/BLU if you are spectating." );
	Msg( "BM: bm_fix — arena rebuilt.\n" );
}

static void CC_BM_Fix( const CCommand &args )
{
	BM_FixMatch();
}

static ConCommand bm_fix( "bm_fix", CC_BM_Fix, "Rebuild bomber arena and warp/respawn players.", FCVAR_GAMEDLL );

//-----------------------------------------------------------------------------
static bool BM_PlayerReadyForGameplay( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return false;
	}

	return ( pPlayer->GetTeamNumber() >= FIRST_GAME_TEAM );
}

//-----------------------------------------------------------------------------
bool BM_TryPlaceBomb( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return false;
	}

	if ( !BM_PlayerReadyForGameplay( pPlayer ) )
	{
		if ( !pPlayer->IsBot() )
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "Join RED or BLU to play Frog Bomber." );
		}
		return false;
	}

	const int iMaxBombs = Max( 1, tf_bm_max_bombs.GetInt() );
	if ( pPlayer->m_iBMActiveBombs >= iMaxBombs )
	{
		return false;
	}

	int iCellX = 0;
	int iCellY = 0;
	BM_WorldToCell( pPlayer->GetAbsOrigin(), iCellX, iCellY );

	if ( BM_IsArenaActive() && !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		BM_WarpPlayerToArenaSpawn( pPlayer );
		BM_WorldToCell( pPlayer->GetAbsOrigin(), iCellX, iCellY );
	}

	if ( !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		if ( !pPlayer->IsBot() )
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "Stand inside the bomber arena (cyan grid)." );
		}
		return false;
	}

	if ( CTFBMBomb::GetBombAtCell( iCellX, iCellY ) != NULL )
	{
		return false;
	}

	if ( BM_CellBlocksMovement( iCellX, iCellY ) && BM_FindCrateAtCell( iCellX, iCellY ) != NULL )
	{
		if ( !pPlayer->IsBot() )
		{
			ClientPrint( pPlayer, HUD_PRINTCENTER, "Can't place a bomb on a crate." );
		}
		return false;
	}

	if ( BM_IsHardWallCell( iCellX, iCellY ) )
	{
		return false;
	}

	CTFBMBomb *pBomb = CTFBMBomb::PlaceAtCell( pPlayer, iCellX, iCellY );
	if ( !pBomb )
	{
		return false;
	}

	if ( !pPlayer->IsBot() )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Bomb placed!" );
	}

	return true;
}

//-----------------------------------------------------------------------------
void BM_EnsurePlayerInArena( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	if ( !BM_IsArenaActive() )
	{
		BM_BuildArena();
	}

	if ( !BM_IsArenaActive() )
	{
		return;
	}

	int iCellX = 0;
	int iCellY = 0;
	BM_WorldToCell( pPlayer->GetAbsOrigin(), iCellX, iCellY );

	const bool bInsideCell = BM_IsInsideArenaCell( iCellX, iCellY );
	const float flPlaneZ = BM_GetPlayPlaneZ();
	const bool bOnPlayPlane = ( fabsf( pPlayer->GetAbsOrigin().z - flPlaneZ ) <= 32.0f );

	if ( !bInsideCell || !bOnPlayPlane )
	{
		BM_WarpPlayerToArenaSpawn( pPlayer );
	}
}

//-----------------------------------------------------------------------------
void BM_OnPlayerSpawn( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	if ( pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_SCOUT )
	{
		pPlayer->SetDesiredPlayerClassIndex( TF_CLASS_SCOUT );
		if ( pPlayer->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED )
		{
			pPlayer->ForceRespawn();
		}
		return;
	}

	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBase *pWpn = assert_cast<CTFWeaponBase *>( pPlayer->GetWeapon( i ) );
		if ( !pWpn || pWpn->GetTFWpnData().m_iWeaponType == TF_WPN_TYPE_MELEE )
		{
			continue;
		}

		pPlayer->Weapon_Detach( pWpn );
		UTIL_Remove( pWpn );
	}

	CTFWeaponBase *pMelee = dynamic_cast<CTFWeaponBase *>( pPlayer->Weapon_GetSlot( TF_WPN_TYPE_MELEE ) );
	if ( pMelee )
	{
		pPlayer->Weapon_Switch( pMelee );
	}

	const float flSpeed = tf_bm_move_speed.GetFloat();
	pPlayer->SetMaxSpeed( flSpeed );
	pPlayer->m_flBMNextGridSnapTime = gpGlobals->curtime;

	BM_EnsurePlayerInArena( pPlayer );

	if ( !pPlayer->IsBot() )
	{
		extern ConVar tf_bm_build_id;
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Frog Bomber [%s] — you are on the sky arena (not map spawns).", tf_bm_build_id.GetString() );
	}
}

//-----------------------------------------------------------------------------
void BM_PlayerRunCommand( CTFPlayer *pPlayer, CUserCmd *ucmd )
{
	if ( !pPlayer || !ucmd || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	if ( !pPlayer->IsAlive() || !BM_PlayerReadyForGameplay( pPlayer ) )
	{
		return;
	}

	const int iPlaceButtons = IN_ATTACK | IN_ATTACK2 | IN_USE;
	const bool bPlacePressed = ( ucmd->buttons & iPlaceButtons ) && !( pPlayer->m_nBMPreviousButtons & iPlaceButtons );

	if ( !BM_IsArenaActive() )
	{
		static float s_flNextArenaBuildTry = 0.0f;
		if ( gpGlobals->curtime >= s_flNextArenaBuildTry )
		{
			s_flNextArenaBuildTry = gpGlobals->curtime + 1.0f;
			BM_BuildArena();
		}

		if ( bPlacePressed )
		{
			BM_TryPlaceBomb( pPlayer );
		}

		pPlayer->m_nBMPreviousButtons = ucmd->buttons;
		return;
	}

	BM_EnsurePlayerInArena( pPlayer );

	Vector vecVelocity = pPlayer->GetAbsVelocity();
	if ( BM_UseSkyPlayPlane() && vecVelocity.z != 0.0f )
	{
		vecVelocity.z = 0.0f;
		pPlayer->SetAbsVelocity( vecVelocity );
	}

	ucmd->upmove = 0;
	ucmd->buttons &= ~( IN_JUMP | IN_DUCK );

	if ( bPlacePressed )
	{
		BM_TryPlaceBomb( pPlayer );
	}

	const float flFwd = ucmd->forwardmove;
	const float flSide = ucmd->sidemove;
	const bool bMoving = ( fabsf( flFwd ) >= 1.0f || fabsf( flSide ) >= 1.0f );

	if ( !bMoving )
	{
		ucmd->forwardmove = 0.0f;
		ucmd->sidemove = 0.0f;

		if ( pPlayer->m_bBMWasMoving )
		{
			BM_SnapPlayerToGrid( pPlayer );
		}

		pPlayer->m_bBMWasMoving = false;
		pPlayer->m_nBMPreviousButtons = ucmd->buttons;
		return;
	}

	int iCellX = 0;
	int iCellY = 0;
	BM_WorldToCell( pPlayer->GetAbsOrigin(), iCellX, iCellY );

	if ( BM_IsArenaActive() && !BM_IsInsideArenaCell( iCellX, iCellY ) )
	{
		BM_EnsurePlayerInArena( pPlayer );
		pPlayer->m_nBMPreviousButtons = ucmd->buttons;
		return;
	}

	int iTargetX = iCellX;
	int iTargetY = iCellY;
	if ( flFwd > 1.0f )
	{
		++iTargetY;
	}
	else if ( flFwd < -1.0f )
	{
		--iTargetY;
	}
	else if ( flSide > 1.0f )
	{
		++iTargetX;
	}
	else if ( flSide < -1.0f )
	{
		--iTargetX;
	}

	if ( !BM_IsInsideArenaCell( iTargetX, iTargetY ) || BM_CellBlocksMovement( iTargetX, iTargetY ) )
	{
		ucmd->forwardmove = 0.0f;
		ucmd->sidemove = 0.0f;
		pPlayer->m_bBMWasMoving = false;
		pPlayer->m_nBMPreviousButtons = ucmd->buttons;
		return;
	}

		pPlayer->m_bBMWasMoving = true;

	pPlayer->m_nBMPreviousButtons = ucmd->buttons;

	const float flSpeed = tf_bm_move_speed.GetFloat();
	if ( fabsf( flFwd ) >= fabsf( flSide ) )
	{
		ucmd->sidemove = 0.0f;
		ucmd->forwardmove = ( flFwd > 0.0f ) ? flSpeed : -flSpeed;
	}
	else
	{
		ucmd->forwardmove = 0.0f;
		ucmd->sidemove = ( flSide > 0.0f ) ? flSpeed : -flSpeed;
	}
}

//-----------------------------------------------------------------------------
void BM_SnapPlayerToGrid( CTFPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return;
	}

	if ( !s_bBMGridAligned && !BM_IsArenaActive() )
	{
		BM_AutoAlignGridFromSpawns();
	}

	const float flCell = BM_GetCellSize();
	Vector vecGridOrigin;
	BM_GetGridOrigin( vecGridOrigin );

	Vector vecOrigin = pPlayer->GetAbsOrigin();
	const int iCellX = Floor2Int( ( vecOrigin.x - vecGridOrigin.x ) / flCell );
	const int iCellY = Floor2Int( ( vecOrigin.y - vecGridOrigin.y ) / flCell );

	Vector vecSnapped;
	vecSnapped.x = vecGridOrigin.x + ( iCellX + 0.5f ) * flCell;
	vecSnapped.y = vecGridOrigin.y + ( iCellY + 0.5f ) * flCell;

	BM_FindFloorAtXY( vecSnapped, vecOrigin.z, pPlayer, vecSnapped );

	Vector vecVelocity = pPlayer->GetAbsVelocity();
	vecVelocity.z = 0.0f;
	if ( vecVelocity.LengthSqr() > 1.0f )
	{
		pPlayer->SetAbsVelocity( Vector( 0, 0, pPlayer->GetAbsVelocity().z ) );
	}

	pPlayer->SetAbsOrigin( vecSnapped );

	// Yaw only — camera handles pitch on the client.
	float flYaw = pPlayer->EyeAngles().y;
	if ( pPlayer->m_bBMWasMoving )
	{
		Vector vecVel = pPlayer->GetAbsVelocity();
		vecVel.z = 0.0f;
		if ( vecVel.LengthSqr() > 1.0f )
		{
			flYaw = RAD2DEG( atan2f( vecVel.y, vecVel.x ) );
		}
	}
	pPlayer->SnapEyeAngles( QAngle( 0, flYaw, 0 ) );
}

//-----------------------------------------------------------------------------
void BM_TickMatch( void )
{
	// Bombs tick via CTFBMBomb::BombThink. Grid snap runs on stop, not every frame.
}

#endif // SOURCESDK
