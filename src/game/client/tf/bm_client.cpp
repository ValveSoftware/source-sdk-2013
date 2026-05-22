//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "bm_client.h"
#include "bm_shareddefs.h"
#include "view_shared.h"
#include "cam_thirdperson.h"
#include "input.h"
#include "in_buttons.h"
#include "materialsystem/imaterialsystem.h"
#include "iviewrender.h"
#include "debugoverlay_shared.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui/ILocalize.h"

ConVar tf_bm_camera_mode( "tf_bm_camera_mode", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"Bomberman camera: 0=vanilla first-person, 1=top-down chase (recommended), 2=ortho (experimental)." );
ConVar tf_bm_cam_height( "tf_bm_cam_height", "520", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: height above player (straight down)." );
ConVar tf_bm_cam_pitch( "tf_bm_cam_pitch", "-89", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera pitch (-89 = top-down in TF)." );
ConVar tf_bm_cam_back( "tf_bm_cam_back", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: horizontal offset (0 = overhead, avoids walls)." );
ConVar tf_bm_clip_world( "tf_bm_clip_world", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: experimental roof clip (off)." );
ConVar tf_bm_clip_height( "tf_bm_clip_height", "256", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: only used if tf_bm_clip_world is 1." );
ConVar tf_bm_show_grid( "tf_bm_show_grid", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: client grid overlay (recommended — server props are invisible)." );
ConVar tf_bm_draw_floor( "tf_bm_draw_floor", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: client floor tint overlay when props are missing." );
ConVar tf_bm_client_snap( "tf_bm_client_snap", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: client origin snap (0=server warp only)." );

// Replicated — defaults must match server (bm_arena.cpp / bm_player_system.cpp).
ConVar tf_bm_arena_width( "tf_bm_arena_width", "15", FCVAR_REPLICATED, "Bomberman arena width in grid cells (odd, includes border walls)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "13", FCVAR_REPLICATED, "Bomberman arena height in grid cells (odd, includes border walls)." );
ConVar tf_bm_cell_size( "tf_bm_cell_size", "64", FCVAR_REPLICATED, "Bomberman: grid cell size in Hammer units." );
ConVar tf_bm_grid_origin( "tf_bm_grid_origin", "0 0 0", FCVAR_REPLICATED, "Bomberman: world origin of cell (0,0). Auto-set from team spawns on map load." );
ConVar tf_bm_play_z_offset( "tf_bm_play_z_offset", "8", FCVAR_REPLICATED, "Bomberman: player feet offset above grid origin Z." );

static QAngle s_angBMLockedView( 90.0f, 90.0f, 0.0f );
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

	// First-person overhead eye (no third-person / ForceTempForceDraw — those triggered NULL material binds).
	g_ThirdPersonManager.SetForcedThirdPerson( false );
	g_ThirdPersonManager.SetOverridingThirdPerson( false );

	if ( input->CAM_IsThirdPerson() )
	{
		input->CAM_ToFirstPerson();
		pLocalPlayer->ThirdPersonSwitch( false );
	}

	pLocalPlayer->ForceTempForceDraw( false );
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
static bool BM_ClientGetArenaCenter( Vector &vecCenter, float &flArenaW, float &flArenaD )
{
	if ( !BM_ClientGridReady() )
	{
		return false;
	}

	Vector vecGridOrigin;
	BM_ClientParseGridOrigin( vecGridOrigin );
	const float flCell = BM_ClientGetCellSize();
	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	flArenaW = iWidth * flCell;
	flArenaD = iHeight * flCell;
	vecCenter.x = vecGridOrigin.x + flArenaW * 0.5f;
	vecCenter.y = vecGridOrigin.y + flArenaD * 0.5f;
	vecCenter.z = BM_ClientGetPlayPlaneZ();
	return true;
}

//-----------------------------------------------------------------------------
static void BM_ClientApplyOrthoView( CViewSetup *pSetup, Vector *pEyeOrigin, QAngle *pEyeAngles, float *pZNear, float *pZFar )
{
	Vector vecCenter;
	float flArenaW = 1024.0f;
	float flArenaD = 1024.0f;
	if ( !BM_ClientGetArenaCenter( vecCenter, flArenaW, flArenaD ) )
	{
		C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocal )
		{
			vecCenter = pLocal->GetAbsOrigin();
		}
		flArenaW = 1024.0f;
		flArenaD = 768.0f;
	}

	const float flHeight = Max( 256.0f, tf_bm_cam_height.GetFloat() );
	Vector vecEye = vecCenter;
	vecEye.z += flHeight;

	const QAngle angView( 90.0f, 90.0f, 0.0f );
	s_angBMLockedView = angView;

	const float flHalfW = flArenaW * 0.55f;
	const float flHalfD = flArenaD * 0.55f;

	if ( pSetup )
	{
		pSetup->origin = vecEye;
		pSetup->angles = angView;
		pSetup->m_bOrtho = true;
		pSetup->m_OrthoLeft = -flHalfW;
		pSetup->m_OrthoTop = -flHalfD;
		pSetup->m_OrthoRight = flHalfW;
		pSetup->m_OrthoBottom = flHalfD;
		pSetup->zNear = 1.0f;
		pSetup->zFar = flHeight + 8192.0f;
	}

	if ( pEyeOrigin )
	{
		*pEyeOrigin = vecEye;
	}
	if ( pEyeAngles )
	{
		*pEyeAngles = angView;
	}
	if ( pZNear )
	{
		*pZNear = 1.0f;
	}
	if ( pZFar )
	{
		*pZFar = flHeight + 8192.0f;
	}
}

//-----------------------------------------------------------------------------
bool BM_ClientShouldControlView( C_TFPlayer *pLocalPlayer )
{
	if ( tf_bm_camera_mode.GetInt() <= 0 )
	{
		return false;
	}

	return ( pLocalPlayer && pLocalPlayer->IsAlive() && pLocalPlayer->GetTeamNumber() >= FIRST_GAME_TEAM
		&& TFGameRules() && TFGameRules()->IsBombermanMode() );
}

//-----------------------------------------------------------------------------
static bool BM_ClientUseOrthoCamera( void )
{
	return tf_bm_camera_mode.GetInt() >= 2;
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

	if ( BM_ClientUseOrthoCamera() )
	{
		BM_ClientApplyOrthoView( pSetup, NULL, NULL, NULL, NULL );
		return;
	}

	Vector vecCam;
	Vector vecLook;
	BM_ClientGetCameraTargets( pLocalPlayer, vecCam, vecLook );

	pSetup->origin = vecCam;
	pSetup->angles = s_angBMLockedView;
	pSetup->m_bOrtho = false;
}

//-----------------------------------------------------------------------------
void BM_ClientCalcView( C_TFPlayer *pLocalPlayer, Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar )
{
	if ( !pLocalPlayer || !BM_ClientShouldControlView( pLocalPlayer ) )
	{
		return;
	}

	if ( BM_ClientUseOrthoCamera() )
	{
		BM_ClientApplyOrthoView( NULL, &eyeOrigin, &eyeAngles, &zNear, &zFar );
		return;
	}

	Vector vecCam;
	Vector vecLook;
	BM_ClientGetCameraTargets( pLocalPlayer, vecCam, vecLook );

	eyeOrigin = vecCam;
	eyeAngles = s_angBMLockedView;
	zNear = 1.0f;
	zFar = view ? view->GetZFar() : 32768.0f;
}

//-----------------------------------------------------------------------------
void BM_ClientPaintArenaHUD( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	int nScreenW = 0;
	int nScreenH = 0;
	vgui::surface()->GetScreenSize( nScreenW, nScreenH );

	// Top banner — visible even when the 3D view is black.
	const int nBannerH = 36;
	vgui::surface()->DrawSetColor( 32, 96, 48, 240 );
	vgui::surface()->DrawFilledRect( 0, 0, nScreenW, nBannerH );
	vgui::surface()->DrawSetColor( 180, 255, 120, 255 );
	vgui::surface()->DrawOutlinedRect( 0, 0, nScreenW, nBannerH );

	const int nMapW = 280;
	const int nMapH = 220;
	const int nX0 = nScreenW - nMapW - 16;
	const int nY0 = 48;

	vgui::surface()->DrawSetColor( 24, 36, 52, 235 );
	vgui::surface()->DrawFilledRect( nX0, nY0, nX0 + nMapW, nY0 + nMapH );
	vgui::surface()->DrawSetColor( 200, 220, 255, 255 );
	vgui::surface()->DrawOutlinedRect( nX0, nY0, nX0 + nMapW, nY0 + nMapH );

	vgui::surface()->DrawSetTextColor( 255, 255, 255, 255 );
	if ( g_hFontTrebuchet24 != vgui::INVALID_FONT )
	{
		vgui::surface()->DrawSetTextFont( g_hFontTrebuchet24 );
	}

	wchar_t wszLine[128];
	g_pVGuiLocalize->ConvertANSIToUnicode( "Frog Bomber phase28 — HUD OK", wszLine, sizeof( wszLine ) );
	vgui::surface()->DrawSetTextPos( 12, 8 );
	vgui::surface()->DrawPrintText( wszLine, V_wcslen( wszLine ) );

	g_pVGuiLocalize->ConvertANSIToUnicode( "Frog Bomber arena", wszLine, sizeof( wszLine ) );
	vgui::surface()->DrawSetTextPos( nX0 + 8, nY0 + 6 );
	vgui::surface()->DrawPrintText( wszLine, V_wcslen( wszLine ) );

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	char szStatus[128];
	if ( !pLocal || !pLocal->IsAlive() || pLocal->GetTeamNumber() < FIRST_GAME_TEAM )
	{
		Q_snprintf( szStatus, sizeof( szStatus ), "jointeam red, then Scout" );
	}
	else
	{
		Q_snprintf( szStatus, sizeof( szStatus ), "Team %d  cam %d", pLocal->GetTeamNumber(), tf_bm_camera_mode.GetInt() );
	}
	g_pVGuiLocalize->ConvertANSIToUnicode( szStatus, wszLine, sizeof( wszLine ) );
	vgui::surface()->DrawSetTextPos( nX0 + 8, nY0 + 24 );
	vgui::surface()->DrawPrintText( wszLine, V_wcslen( wszLine ) );

	if ( !BM_ClientGridReady() )
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( "Grid not ready yet...", wszLine, sizeof( wszLine ) );
		vgui::surface()->DrawSetTextPos( nX0 + 8, nY0 + 44 );
		vgui::surface()->DrawPrintText( wszLine, V_wcslen( wszLine ) );
		return;
	}

	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 51 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 51 );
	const int nInnerX = nX0 + 12;
	const int nInnerY = nY0 + 48;
	const int nInnerW = nMapW - 24;
	const int nInnerH = nMapH - 60;
	const float flCellPixW = (float)nInnerW / (float)iWidth;
	const float flCellPixH = (float)nInnerH / (float)iHeight;

	for ( int iCellX = 0; iCellX < iWidth; ++iCellX )
	{
		for ( int iCellY = 0; iCellY < iHeight; ++iCellY )
		{
			const bool bBorder = ( iCellX == 0 || iCellY == 0 || iCellX == iWidth - 1 || iCellY == iHeight - 1 );
			const int r = bBorder ? 200 : 72;
			const int g = bBorder ? 90 : 110;
			const int bCol = bBorder ? 60 : 150;

			const int px0 = nInnerX + (int)( iCellX * flCellPixW );
			const int py0 = nInnerY + (int)( iCellY * flCellPixH );
			const int px1 = nInnerX + (int)( ( iCellX + 1 ) * flCellPixW );
			const int py1 = nInnerY + (int)( ( iCellY + 1 ) * flCellPixH );

			vgui::surface()->DrawSetColor( r, g, bCol, 255 );
			vgui::surface()->DrawFilledRect( px0, py0, px1, py1 );
		}
	}

	if ( pLocal && pLocal->IsAlive() && BM_ClientGridReady() )
	{
		Vector vecGridOrigin;
		BM_ClientParseGridOrigin( vecGridOrigin );
		const float flCell = BM_ClientGetCellSize();
		int iCellX = Floor2Int( ( pLocal->GetAbsOrigin().x - vecGridOrigin.x ) / flCell );
		int iCellY = Floor2Int( ( pLocal->GetAbsOrigin().y - vecGridOrigin.y ) / flCell );
		iCellX = clamp( iCellX, 0, iWidth - 1 );
		iCellY = clamp( iCellY, 0, iHeight - 1 );

		const int px0 = nInnerX + (int)( iCellX * flCellPixW );
		const int py0 = nInnerY + (int)( iCellY * flCellPixH );
		const int px1 = nInnerX + (int)( ( iCellX + 1 ) * flCellPixW );
		const int py1 = nInnerY + (int)( ( iCellY + 1 ) * flCellPixH );
		vgui::surface()->DrawSetColor( 255, 240, 80, 255 );
		vgui::surface()->DrawFilledRect( px0, py0, px1, py1 );
	}
}

//-----------------------------------------------------------------------------
void BM_ClientDrawArenaHUD( const CViewSetup &viewSetup )
{
	(void)viewSetup;
	BM_ClientPaintArenaHUD();
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

	NDebugOverlay::Triangle( v00, v10, v11, 72, 110, 150, 240, true, 0.0f );
	NDebugOverlay::Triangle( v00, v11, v01, 72, 110, 150, 240, true, 0.0f );
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
