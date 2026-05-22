//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "bm_shareddefs.h"
#include "view_shared.h"
#include "cam_thirdperson.h"
#include "input.h"
#include "in_buttons.h"
#include "materialsystem/imaterialsystem.h"
#include "iviewrender.h"
#include "debugoverlay_shared.h"

ConVar tf_bm_cam_height( "tf_bm_cam_height", "520", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: height above player (straight down)." );
ConVar tf_bm_cam_pitch( "tf_bm_cam_pitch", "89", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera pitch (89 = top-down)." );
ConVar tf_bm_cam_back( "tf_bm_cam_back", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: horizontal offset (0 = overhead, avoids walls)." );
ConVar tf_bm_clip_world( "tf_bm_clip_world", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: experimental roof clip (off)." );
ConVar tf_bm_clip_height( "tf_bm_clip_height", "256", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: only used if tf_bm_clip_world is 1." );
ConVar tf_bm_show_grid( "tf_bm_show_grid", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: debug grid overlay (0=off, 1=on)." );
ConVar tf_bm_draw_floor( "tf_bm_draw_floor", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: client floor tint overlay when props are missing." );
ConVar tf_bm_client_snap( "tf_bm_client_snap", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: client origin snap (0=server warp only)." );

// Replicated — defaults must match server (bm_arena.cpp / bm_player_system.cpp).
ConVar tf_bm_arena_width( "tf_bm_arena_width", "15", FCVAR_REPLICATED, "Bomberman arena width in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "13", FCVAR_REPLICATED, "Bomberman arena height in grid cells (odd, includes border walls)." );
ConVar tf_bm_cell_size( "tf_bm_cell_size", "64", FCVAR_REPLICATED, "Bomberman: grid cell size in Hammer units." );
ConVar tf_bm_grid_origin( "tf_bm_grid_origin", "0 0 0", FCVAR_REPLICATED, "Bomberman: world origin of cell (0,0). Auto-set from team spawns on map load." );
ConVar tf_bm_play_z_offset( "tf_bm_play_z_offset", "8", FCVAR_REPLICATED, "Bomberman: player feet offset above grid origin Z." );

static QAngle s_angBMLockedView( 89.0f, 0.0f, 0.0f );
static int s_nBMClipPushDepth = 0;
static float s_flBMNextGridDrawTime = 0.0f;

static void BM_ClientDrawGridOverlay( void );
static void BM_ClientDrawArenaFloor( void );
static bool BM_ClientGridReady( void );

//-----------------------------------------------------------------------------
void BM_ClientClearBomberCameraState( C_TFPlayer *pLocalPlayer )
{
	if ( !pLocalPlayer )
	{
		return;
	}

	g_ThirdPersonManager.SetOverridingThirdPerson( false );
	g_ThirdPersonManager.SetForcedThirdPerson( false );
	pLocalPlayer->ForceTempForceDraw( false );
	pLocalPlayer->m_Local.m_bDrawViewmodel = true;

	if ( input->CAM_IsThirdPerson() )
	{
		input->CAM_ToFirstPerson();
		pLocalPlayer->ThirdPersonSwitch( false );
	}
}

//-----------------------------------------------------------------------------
void BM_ClientApplyBomberCameraState( C_TFPlayer *pLocalPlayer )
{
	if ( !pLocalPlayer )
	{
		return;
	}

	// Do not use stock third-person — it places the eye inside map geometry on itemtest.
	g_ThirdPersonManager.SetForcedThirdPerson( false );
	g_ThirdPersonManager.SetOverridingThirdPerson( false );

	pLocalPlayer->ForceTempForceDraw( true );
	pLocalPlayer->m_Local.m_bDrawViewmodel = false;
}

//-----------------------------------------------------------------------------
static void BM_ClientParseGridOrigin( Vector &vecGridOrigin )
{
	sscanf( tf_bm_grid_origin.GetString(), "%f %f %f", &vecGridOrigin.x, &vecGridOrigin.y, &vecGridOrigin.z );
}

//-----------------------------------------------------------------------------
static float BM_ClientGetPlayPlaneZ( void )
{
	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );
	return vecGridOrigin.z + tf_bm_play_z_offset.GetFloat();
}

//-----------------------------------------------------------------------------
// Avoid camera inside world brushes when grid origin is still 0 0 0 but the body is on the arena.
//-----------------------------------------------------------------------------
static float BM_ClientGetEffectivePlayZ( C_TFPlayer *pPlayer )
{
	const float flGridPlaneZ = BM_ClientGetPlayPlaneZ();

	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		return flGridPlaneZ;
	}

	const float flPlayerZ = pPlayer->GetAbsOrigin().z;

	if ( !BM_ClientGridReady() )
	{
		return flPlayerZ;
	}

	if ( fabsf( flPlayerZ - flGridPlaneZ ) > 96.0f )
	{
		return flPlayerZ;
	}

	return flGridPlaneZ;
}

//-----------------------------------------------------------------------------
static void BM_ClientUnstuckCamera( C_TFPlayer *pPlayer, Vector &vecCam )
{
	if ( !pPlayer )
	{
		return;
	}

	trace_t trace;
	const Vector vecHullMins( -8.0f, -8.0f, -8.0f );
	const Vector vecHullMaxs( 8.0f, 8.0f, 8.0f );
	UTIL_TraceHull( vecCam, vecCam, vecHullMins, vecHullMaxs, MASK_SOLID, pPlayer, COLLISION_GROUP_DEBRIS, &trace );
	if ( !trace.startsolid )
	{
		return;
	}

	Vector vecUp = vecCam;
	vecUp.z += 128.0f;
	UTIL_TraceHull( vecCam, vecUp, vecHullMins, vecHullMaxs, MASK_SOLID, pPlayer, COLLISION_GROUP_DEBRIS, &trace );
	if ( !trace.startsolid )
	{
		vecCam = trace.endpos;
		return;
	}

	vecCam.z = pPlayer->GetAbsOrigin().z + tf_bm_cam_height.GetFloat();
}

//-----------------------------------------------------------------------------
static float BM_ClientGetCellSize( void )
{
	return clamp( tf_bm_cell_size.GetFloat(), 32.0f, 128.0f );
}

//-----------------------------------------------------------------------------
static int BM_ClientGetSpawnSlot( C_TFPlayer *pLocalPlayer )
{
	if ( !pLocalPlayer )
	{
		return 0;
	}

	int iSlot = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		C_TFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pOther || pOther == pLocalPlayer || !pOther->IsPlayer() )
		{
			continue;
		}

		if ( pOther->GetTeamNumber() == pLocalPlayer->GetTeamNumber() && pOther->GetUserID() < pLocalPlayer->GetUserID() )
		{
			++iSlot;
		}
	}

	return clamp( iSlot, 0, BM_MAX_SPAWN_SLOTS_PER_TEAM - 1 );
}

//-----------------------------------------------------------------------------
static void BM_ClientGetSpawnCell( C_TFPlayer *pLocalPlayer, int &iCellX, int &iCellY )
{
	int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	if ( iWidth % 2 == 0 )
	{
		++iWidth;
	}
	if ( iHeight % 2 == 0 )
	{
		++iHeight;
	}

	const bool bBlueTeam = ( pLocalPlayer && pLocalPlayer->GetTeamNumber() == TF_TEAM_BLUE );
	const int iSlot = BM_ClientGetSpawnSlot( pLocalPlayer );
	BM_GetSpawnCellForSlot( bBlueTeam, iSlot, iWidth, iHeight, iCellX, iCellY );
}

//-----------------------------------------------------------------------------
static void BM_ClientCellToWorldCenter( int iCellX, int iCellY, Vector &vecCenter )
{
	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );
	const float flCell = BM_ClientGetCellSize();

	vecCenter.x = vecGridOrigin.x + ( iCellX + 0.5f ) * flCell;
	vecCenter.y = vecGridOrigin.y + ( iCellY + 0.5f ) * flCell;
	vecCenter.z = BM_ClientGetPlayPlaneZ();
}

//-----------------------------------------------------------------------------
static bool BM_ClientIsInsideArenaCell( int iCellX, int iCellY )
{
	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	return ( iCellX >= 0 && iCellY >= 0 && iCellX < iWidth && iCellY < iHeight );
}

//-----------------------------------------------------------------------------
static void BM_ClientWorldToCell( const Vector &vecWorld, int &iCellX, int &iCellY )
{
	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );
	const float flCell = BM_ClientGetCellSize();

	iCellX = Floor2Int( ( vecWorld.x - vecGridOrigin.x ) / flCell );
	iCellY = Floor2Int( ( vecWorld.y - vecGridOrigin.y ) / flCell );
}

//-----------------------------------------------------------------------------
static bool BM_ClientGridReady( void )
{
	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );
	if ( vecGridOrigin == vec3_origin )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
static void BM_ClientSyncLocalPlayerToArena( C_TFPlayer *pLocalPlayer )
{
	if ( !tf_bm_client_snap.GetBool() )
	{
		return;
	}

	if ( !pLocalPlayer || !pLocalPlayer->IsAlive() )
	{
		return;
	}

	const int iTeam = pLocalPlayer->GetTeamNumber();
	if ( iTeam != TF_TEAM_RED && iTeam != TF_TEAM_BLUE )
	{
		return;
	}

	if ( !BM_ClientGridReady() )
	{
		return;
	}

	const float flPlayZ = BM_ClientGetPlayPlaneZ();
	Vector vecPos = pLocalPlayer->GetAbsOrigin();

	int iCellX = 0;
	int iCellY = 0;
	BM_ClientWorldToCell( vecPos, iCellX, iCellY );

	const bool bOnPlane = ( fabsf( vecPos.z - flPlayZ ) <= 24.0f );
	const bool bInside = BM_ClientIsInsideArenaCell( iCellX, iCellY );

	Vector vecTarget;
	if ( !bInside )
	{
		BM_ClientGetSpawnCell( pLocalPlayer, iCellX, iCellY );
		BM_ClientCellToWorldCenter( iCellX, iCellY, vecTarget );
	}
	else
	{
		vecTarget = vecPos;
		if ( !bOnPlane )
		{
			vecTarget.z = flPlayZ;
		}
	}

	const float flErrorSqr = ( vecPos - vecTarget ).LengthSqr();
	if ( flErrorSqr > ( 96.0f * 96.0f ) )
	{
		pLocalPlayer->SetAbsOrigin( vecTarget );
		pLocalPlayer->SetLocalOrigin( vecTarget );
		pLocalPlayer->SetAbsVelocity( vec3_origin );
	}
}

//-----------------------------------------------------------------------------
static void BM_ClientGetCameraTargets( C_TFPlayer *pLocalPlayer, Vector &vecCamOrigin, Vector &vecLookAt )
{
	BM_ClientSyncLocalPlayerToArena( pLocalPlayer );

	const Vector vecFeet = pLocalPlayer->GetAbsOrigin();
	const float flHeight = Max( 256.0f, tf_bm_cam_height.GetFloat() );
	const float flBack = tf_bm_cam_back.GetFloat();

	vecLookAt = vecFeet;
	vecLookAt.z = vecFeet.z + 16.0f;

	vecCamOrigin.x = vecFeet.x - flBack;
	vecCamOrigin.y = vecFeet.y;
	vecCamOrigin.z = vecFeet.z + flHeight;

	BM_ClientUnstuckCamera( pLocalPlayer, vecCamOrigin );

	s_angBMLockedView.Init( tf_bm_cam_pitch.GetFloat(), pLocalPlayer->GetAbsAngles().y, 0.0f );
}

//-----------------------------------------------------------------------------
bool BM_ClientShouldControlView( C_TFPlayer *pLocalPlayer )
{
	return ( pLocalPlayer && pLocalPlayer->IsAlive() && pLocalPlayer->GetTeamNumber() >= FIRST_GAME_TEAM
		&& TFGameRules() && TFGameRules()->IsBombermanMode() );
}

//-----------------------------------------------------------------------------
void BM_ClientApplyBomberViewCmd( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( !pLocalPlayer || !pCmd || !BM_ClientShouldControlView( pLocalPlayer ) )
	{
		return;
	}

	pCmd->viewangles = s_angBMLockedView;
	pCmd->mousedx = 0;
	pCmd->mousedy = 0;
}

//-----------------------------------------------------------------------------
void BM_ClientCreateMove( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( !pLocalPlayer || !pCmd || !BM_ClientShouldControlView( pLocalPlayer ) )
	{
		return;
	}

	BM_ClientApplyBomberCameraState( pLocalPlayer );
	BM_ClientSyncLocalPlayerToArena( pLocalPlayer );

	Vector vecCam;
	Vector vecLook;
	BM_ClientGetCameraTargets( pLocalPlayer, vecCam, vecLook );

	BM_ClientApplyBomberViewCmd( pLocalPlayer, pCmd );

	if ( gpGlobals->curtime >= s_flBMNextGridDrawTime )
	{
		s_flBMNextGridDrawTime = gpGlobals->curtime + 0.1f;
		if ( tf_bm_draw_floor.GetBool() )
		{
			BM_ClientDrawArenaFloor();
		}
		if ( tf_bm_show_grid.GetBool() )
		{
			BM_ClientDrawGridOverlay();
		}
	}
}

//-----------------------------------------------------------------------------
void BM_ClientApplyTopDownCamera( C_TFPlayer *pLocalPlayer, CViewSetup *pSetup )
{
	if ( !pLocalPlayer || !pSetup || !BM_ClientShouldControlView( pLocalPlayer ) )
	{
		return;
	}

	BM_ClientApplyBomberCameraState( pLocalPlayer );

	Vector vecCam, vecLook;
	BM_ClientGetCameraTargets( pLocalPlayer, vecCam, vecLook );

	pSetup->origin = vecCam;
	pSetup->angles = s_angBMLockedView;
	pSetup->m_bOrtho = false;

	const float flViewDist = ( vecLook - vecCam ).Length();
	pSetup->zNear = clamp( 1.0f, pSetup->zNear, 8.0f );
	pSetup->zFar = Max( pSetup->zFar, flViewDist + 4096.0f );
}

//-----------------------------------------------------------------------------
void BM_ClientCalcView( C_TFPlayer *pLocalPlayer, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar )
{
	if ( !pLocalPlayer || !BM_ClientShouldControlView( pLocalPlayer ) )
	{
		return;
	}

	Vector vecCam, vecLook;
	BM_ClientGetCameraTargets( pLocalPlayer, vecCam, vecLook );

	eyeOrigin = vecCam;
	eyeAngles = s_angBMLockedView;

	const float flViewDist = ( vecLook - vecCam ).Length();
	zNear = clamp( 1.0f, zNear, 8.0f );
	zFar = Max( zFar, flViewDist + 4096.0f );
}

//-----------------------------------------------------------------------------
bool BM_ClientPushWorldClip( void )
{
	if ( !tf_bm_clip_world.GetBool() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return false;
	}

	if ( !BM_ClientGridReady() )
	{
		return false;
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	const float flCeilingZ = BM_ClientGetEffectivePlayZ( pLocal ) + tf_bm_clip_height.GetFloat();
	Vector4D plane;
	plane.x = 0.0f;
	plane.y = 0.0f;
	plane.z = -1.0f;
	plane.w = flCeilingZ;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PushCustomClipPlane( plane.Base() );
	++s_nBMClipPushDepth;
	return true;
}

//-----------------------------------------------------------------------------
void BM_ClientPopWorldClip( void )
{
	if ( s_nBMClipPushDepth <= 0 )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->PopCustomClipPlane();
	--s_nBMClipPushDepth;
}

//-----------------------------------------------------------------------------
void BM_ClientOnLevelShutdown( void )
{
	while ( s_nBMClipPushDepth > 0 )
	{
		BM_ClientPopWorldClip();
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocal )
	{
		BM_ClientClearBomberCameraState( pLocal );
	}
}

//-----------------------------------------------------------------------------
static void BM_ClientDrawArenaFloor( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() || !BM_ClientGridReady() )
	{
		return;
	}

	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );

	const float flCell = BM_ClientGetCellSize();
	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	const float flZ = BM_ClientGetPlayPlaneZ() + 1.0f;

	const float x0 = vecGridOrigin.x;
	const float y0 = vecGridOrigin.y;
	const float x1 = vecGridOrigin.x + iWidth * flCell;
	const float y1 = vecGridOrigin.y + iHeight * flCell;

	const Vector v00( x0, y0, flZ );
	const Vector v10( x1, y0, flZ );
	const Vector v11( x1, y1, flZ );
	const Vector v01( x0, y1, flZ );

	NDebugOverlay::Triangle( v00, v10, v11, 48, 72, 96, 220, true, 0.0f );
	NDebugOverlay::Triangle( v00, v11, v01, 48, 72, 96, 220, true, 0.0f );
}

//-----------------------------------------------------------------------------
static void BM_ClientDrawCellOutline( float x0, float y0, float x1, float y1, float flZ, int r, int g, int bCol )
{
	NDebugOverlay::Line( Vector( x0, y0, flZ ), Vector( x1, y0, flZ ), r, g, bCol, false, 0.0f );
	NDebugOverlay::Line( Vector( x1, y0, flZ ), Vector( x1, y1, flZ ), r, g, bCol, false, 0.0f );
	NDebugOverlay::Line( Vector( x1, y1, flZ ), Vector( x0, y1, flZ ), r, g, bCol, false, 0.0f );
	NDebugOverlay::Line( Vector( x0, y1, flZ ), Vector( x0, y0, flZ ), r, g, bCol, false, 0.0f );
}

//-----------------------------------------------------------------------------
static void BM_ClientDrawGridOverlay( void )
{
	if ( !tf_bm_show_grid.GetBool() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocal || !BM_ClientGridReady() )
	{
		return;
	}

	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );

	const float flCell = BM_ClientGetCellSize();
	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	const float flZ = BM_ClientGetPlayPlaneZ() + 2.0f;
	const int nCells = iWidth * iHeight;
	const bool bLightweight = ( nCells > 255 );

	if ( bLightweight )
	{
		const float x0 = vecGridOrigin.x;
		const float y0 = vecGridOrigin.y;
		const float x1 = vecGridOrigin.x + iWidth * flCell;
		const float y1 = vecGridOrigin.y + iHeight * flCell;
		BM_ClientDrawCellOutline( x0, y0, x1, y1, flZ, 220, 80, 60 );

		for ( int iCellX = 2; iCellX < iWidth - 1; iCellX += 2 )
		{
			for ( int iCellY = 2; iCellY < iHeight - 1; iCellY += 2 )
			{
				const float px0 = vecGridOrigin.x + iCellX * flCell;
				const float py0 = vecGridOrigin.y + iCellY * flCell;
				BM_ClientDrawCellOutline( px0, py0, px0 + flCell, py0 + flCell, flZ, 140, 140, 200 );
			}
		}
		return;
	}

	for ( int iCellX = 0; iCellX < iWidth; ++iCellX )
	{
		for ( int iCellY = 0; iCellY < iHeight; ++iCellY )
		{
			const float x0 = vecGridOrigin.x + iCellX * flCell;
			const float y0 = vecGridOrigin.y + iCellY * flCell;
			const float x1 = x0 + flCell;
			const float y1 = y0 + flCell;

			const bool bBorder = ( iCellX == 0 || iCellY == 0 || iCellX == iWidth - 1 || iCellY == iHeight - 1 );
			const bool bPillar = ( !bBorder && ( iCellX % 2 ) == 0 && ( iCellY % 2 ) == 0 );
			const int r = bBorder ? 220 : ( bPillar ? 140 : 80 );
			const int g = bBorder ? 80 : ( bPillar ? 140 : 200 );
			const int bCol = bBorder ? 60 : ( bPillar ? 200 : 255 );

			BM_ClientDrawCellOutline( x0, y0, x1, y1, flZ, r, g, bCol );
		}
	}

	if ( tf_bm_client_snap.GetBool() )
	{
		const float flPlayZ = BM_ClientGetPlayPlaneZ();
		const Vector vecFeet = pLocal->GetAbsOrigin();
		if ( fabsf( vecFeet.z - flPlayZ ) > 8.0f )
		{
			NDebugOverlay::Line( vecFeet, Vector( vecFeet.x, vecFeet.y, flPlayZ ), 255, 0, 0, false, 0.0f );
		}
	}
}

#endif // SOURCESDK
