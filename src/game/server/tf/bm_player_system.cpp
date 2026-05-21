//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "bm_player_system.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_weaponbase.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_bm_cell_size( "tf_bm_cell_size", "64", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: grid cell size in Hammer units." );
ConVar tf_bm_grid_origin( "tf_bm_grid_origin", "0 0 0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: world origin of cell (0,0). Auto-set from team spawns on map load." );
ConVar tf_bm_move_speed( "tf_bm_move_speed", "320", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: movement speed along one axis." );
ConVar tf_bm_snap_move_interval( "tf_bm_snap_move_interval", "0.12", FCVAR_REPLICATED | FCVAR_NOTIFY, "Bomberman: seconds between grid position snaps while moving." );

static bool s_bBMGridAligned = false;

//-----------------------------------------------------------------------------
void BM_ResetGridAlign( void )
{
	s_bBMGridAligned = false;
}

//-----------------------------------------------------------------------------
static void BM_FindFloorAtXY( const Vector &vecXY, float flRefZ, CTFPlayer *pPlayer, Vector &vecFloor )
{
	Vector vecStart( vecXY.x, vecXY.y, flRefZ + 512.0f );
	Vector vecEnd( vecXY.x, vecXY.y, flRefZ - 4096.0f );

	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, VEC_HULL_MIN, VEC_HULL_MAX, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER, &trace );

	vecFloor = vecXY;
	vecFloor.z = trace.DidHit() ? ( trace.endpos.z + 8.0f ) : flRefZ;
}

//-----------------------------------------------------------------------------
void BM_AutoAlignGridFromSpawns( void )
{
	float flMinX = FLT_MAX;
	float flMinY = FLT_MAX;
	float flMaxX = -FLT_MAX;
	float flMaxY = -FLT_MAX;
	float flMaxZ = -FLT_MAX;
	int nSpawns = 0;

	for ( CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, "info_player_teamspawn" );
		pEnt != NULL;
		pEnt = gEntList.FindEntityByClassname( pEnt, "info_player_teamspawn" ) )
	{
		const Vector &vecOrigin = pEnt->GetAbsOrigin();
		flMinX = Min( flMinX, vecOrigin.x );
		flMinY = Min( flMinY, vecOrigin.y );
		flMaxX = Max( flMaxX, vecOrigin.x );
		flMaxY = Max( flMaxY, vecOrigin.y );
		flMaxZ = Max( flMaxZ, vecOrigin.z );
		++nSpawns;
	}

	if ( nSpawns == 0 )
	{
		Warning( "BM: no info_player_teamspawn on this map — set tf_bm_grid_origin manually (getpos).\n" );
		return;
	}

	const float flCell = Max( 32.0f, tf_bm_cell_size.GetFloat() );
	Vector vecGridOrigin;
	vecGridOrigin.x = floorf( flMinX / flCell ) * flCell;
	vecGridOrigin.y = floorf( flMinY / flCell ) * flCell;

	Vector vecFloor;
	BM_FindFloorAtXY( Vector( ( flMinX + flMaxX ) * 0.5f, ( flMinY + flMaxY ) * 0.5f, 0.0f ), flMaxZ, NULL, vecFloor );
	vecGridOrigin.z = vecFloor.z;

	char szOrigin[64];
	Q_snprintf( szOrigin, sizeof( szOrigin ), "%.1f %.1f %.1f", vecGridOrigin.x, vecGridOrigin.y, vecGridOrigin.z );
	tf_bm_grid_origin.SetValue( szOrigin );
	s_bBMGridAligned = true;

	Msg( "BM: grid aligned from %d spawns — origin %s cell %.0f\n", nSpawns, szOrigin, flCell );
	UTIL_ClientPrintAll( HUD_PRINTTALK, CFmtStr( "Frog Bomber: grid set from team spawns (%s).", szOrigin ) );
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

	BM_AutoAlignGridFromSpawns();

	UTIL_ClientPrintAll( HUD_PRINTTALK, "Frog Bomber: top-down grid movement (Phase 0). Bombs coming next!" );
	Msg( "BM: match started — cell=%.0f origin=(%s)\n", tf_bm_cell_size.GetFloat(), tf_bm_grid_origin.GetString() );
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
		if ( pPlayer && pPlayer->IsConnected() )
		{
			pPlayer->ForceRespawn();
		}
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
		pPlayer->ForceRespawn();
		return;
	}

	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBase *pWpn = assert_cast<CTFWeaponBase *>( pPlayer->GetWeapon( i ) );
		if ( pWpn )
		{
			pPlayer->Weapon_Detach( pWpn );
			UTIL_Remove( pWpn );
		}
	}

	const float flSpeed = tf_bm_move_speed.GetFloat();
	pPlayer->SetMaxSpeed( flSpeed );
	pPlayer->m_flBMNextGridSnapTime = gpGlobals->curtime;

	if ( !s_bBMGridAligned )
	{
		BM_AutoAlignGridFromSpawns();
	}

	if ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING )
	{
		BM_SnapPlayerToGrid( pPlayer );
	}

	if ( !pPlayer->IsBot() )
	{
		ClientPrint( pPlayer, HUD_PRINTCENTER, "Frog Bomber — WASD moves on the grid. Top-down view." );
	}
}

//-----------------------------------------------------------------------------
void BM_PlayerRunCommand( CTFPlayer *pPlayer, CUserCmd *ucmd )
{
	if ( !pPlayer || !ucmd || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	ucmd->upmove = 0;
	ucmd->buttons &= ~( IN_JUMP | IN_DUCK );

	const float flFwd = ucmd->forwardmove;
	const float flSide = ucmd->sidemove;
	if ( fabsf( flFwd ) < 1.0f && fabsf( flSide ) < 1.0f )
	{
		ucmd->forwardmove = 0.0f;
		ucmd->sidemove = 0.0f;
		return;
	}

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

	if ( !s_bBMGridAligned )
	{
		BM_AutoAlignGridFromSpawns();
	}

	const float flCell = Max( 32.0f, tf_bm_cell_size.GetFloat() );
	Vector vecGridOrigin;
	sscanf( tf_bm_grid_origin.GetString(), "%f %f %f", &vecGridOrigin.x, &vecGridOrigin.y, &vecGridOrigin.z );

	Vector vecOrigin = pPlayer->GetAbsOrigin();
	const int iCellX = Floor2Int( ( vecOrigin.x - vecGridOrigin.x ) / flCell );
	const int iCellY = Floor2Int( ( vecOrigin.y - vecGridOrigin.y ) / flCell );

	Vector vecSnapped;
	vecSnapped.x = vecGridOrigin.x + ( iCellX + 0.5f ) * flCell;
	vecSnapped.y = vecGridOrigin.y + ( iCellY + 0.5f ) * flCell;

	BM_FindFloorAtXY( vecSnapped, vecOrigin.z, pPlayer, vecSnapped );

	pPlayer->SetAbsOrigin( vecSnapped );
	pPlayer->SnapEyeAngles( QAngle( 0, pPlayer->EyeAngles().y, 0 ) );
}

//-----------------------------------------------------------------------------
void BM_TickMatch( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() || TFGameRules()->State_Get() != GR_STATE_RND_RUNNING )
	{
		return;
	}

	const float flInterval = Max( 0.05f, tf_bm_snap_move_interval.GetFloat() );

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() )
		{
			continue;
		}

		if ( gpGlobals->curtime >= pPlayer->m_flBMNextGridSnapTime )
		{
			pPlayer->m_flBMNextGridSnapTime = gpGlobals->curtime + flInterval;
			BM_SnapPlayerToGrid( pPlayer );
		}
	}
}

#endif // SOURCESDK
