//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "view_shared.h"
#include "cam_thirdperson.h"
#include "input.h"
#include "in_buttons.h"
#include "materialsystem/imaterialsystem.h"
#include "iviewrender.h"
#include "debugoverlay_shared.h"

ConVar tf_bm_cam_height( "tf_bm_cam_height", "900", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera height above the player." );
ConVar tf_bm_cam_pitch( "tf_bm_cam_pitch", "89", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera pitch (89 = straight down)." );
ConVar tf_bm_clip_world( "tf_bm_clip_world", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: experimental roof clip (off — was hiding the whole map)." );
ConVar tf_bm_clip_height( "tf_bm_clip_height", "256", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: only used if tf_bm_clip_world is 1." );
ConVar tf_bm_show_grid( "tf_bm_show_grid", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: draw grid lines on the play floor." );
ConVar tf_bm_arena_width( "tf_bm_arena_width", "15", FCVAR_CLIENTDLL | FCVAR_REPLICATED, "Bomberman arena width (replicated)." );
ConVar tf_bm_arena_height( "tf_bm_arena_height", "13", FCVAR_CLIENTDLL | FCVAR_REPLICATED, "Bomberman arena height (replicated)." );

ConVar tf_bm_cell_size( "tf_bm_cell_size", "64", FCVAR_CLIENTDLL | FCVAR_REPLICATED, "Bomberman: grid cell size (replicated)." );
ConVar tf_bm_grid_origin( "tf_bm_grid_origin", "0 0 0", FCVAR_CLIENTDLL | FCVAR_REPLICATED, "Bomberman: grid origin (replicated)." );

static QAngle s_angBMLockedView( 89.0f, 0.0f, 0.0f );
static int s_nBMClipPushDepth = 0;

//-----------------------------------------------------------------------------
void BM_ClientCreateMove( C_TFPlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( !pLocalPlayer || !pCmd || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	pCmd->viewangles = s_angBMLockedView;
	pCmd->mousedx = 0;
	pCmd->mousedy = 0;

	g_ThirdPersonManager.SetForcedThirdPerson( true );
	g_ThirdPersonManager.SetOverridingThirdPerson( true );
	g_ThirdPersonManager.SetDesiredCameraOffset( Vector( 0, 0, 0 ) );

	if ( !input->CAM_IsThirdPerson() )
	{
		input->CAM_ToThirdPerson();
		pLocalPlayer->ThirdPersonSwitch( true );
	}
}

//-----------------------------------------------------------------------------
void BM_ClientApplyTopDownCamera( C_TFPlayer *pLocalPlayer, CViewSetup *pSetup )
{
	if ( !pLocalPlayer || !pSetup || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	g_ThirdPersonManager.SetForcedThirdPerson( true );
	g_ThirdPersonManager.SetOverridingThirdPerson( true );
	g_ThirdPersonManager.SetDesiredCameraOffset( Vector( 0, 0, 0 ) );

	if ( !input->CAM_IsThirdPerson() )
	{
		input->CAM_ToThirdPerson();
		pLocalPlayer->ThirdPersonSwitch( true );
	}

	const Vector &vecPlayer = pLocalPlayer->GetAbsOrigin();
	const float flHeight = tf_bm_cam_height.GetFloat();
	const float flPitch = tf_bm_cam_pitch.GetFloat();

	pSetup->origin.x = vecPlayer.x;
	pSetup->origin.y = vecPlayer.y;
	pSetup->origin.z = vecPlayer.z + flHeight;

	pSetup->angles.x = flPitch;
	pSetup->angles.y = 0.0f;
	pSetup->angles.z = 0.0f;
	pSetup->m_bOrtho = false;
}

//-----------------------------------------------------------------------------
void BM_ClientCalcView( C_TFPlayer *pLocalPlayer, Vector &eyeOrigin, QAngle &eyeAngles )
{
	if ( !pLocalPlayer || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	const Vector &vecPlayer = pLocalPlayer->GetAbsOrigin();
	eyeOrigin.x = vecPlayer.x;
	eyeOrigin.y = vecPlayer.y;
	eyeOrigin.z = vecPlayer.z + tf_bm_cam_height.GetFloat();

	eyeAngles.x = tf_bm_cam_pitch.GetFloat();
	eyeAngles.y = 0.0f;
	eyeAngles.z = 0.0f;
}

//-----------------------------------------------------------------------------
bool BM_ClientPushWorldClip( void )
{
	if ( !tf_bm_clip_world.GetBool() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return false;
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocal || !pLocal->IsAlive() )
	{
		return false;
	}

	// Hide geometry above the play layer; keep ground/walls at player height.
	// Plane: visible where dot(normal,pos) + w >= 0  =>  z <= flCeilingZ  with normal (0,0,-1).
	const float flCeilingZ = pLocal->GetAbsOrigin().z + tf_bm_clip_height.GetFloat();
	Vector4D plane;
	plane.x = 0.0f;
	plane.y = 0.0f;
	plane.z = 1.0f;
	plane.w = -flCeilingZ;

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
static void BM_ClientDrawGridOverlay( void )
{
	if ( !tf_bm_show_grid.GetBool() || !TFGameRules() || !TFGameRules()->IsBombermanMode() )
	{
		return;
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocal )
	{
		return;
	}

	Vector vecGridOrigin;
	sscanf( tf_bm_grid_origin.GetString(), "%f %f %f", &vecGridOrigin.x, &vecGridOrigin.y, &vecGridOrigin.z );

	const float flCell = Max( 32.0f, tf_bm_cell_size.GetFloat() );
	const int iWidth = clamp( tf_bm_arena_width.GetInt(), 7, 31 );
	const int iHeight = clamp( tf_bm_arena_height.GetInt(), 7, 25 );

	const float flZ = pLocal->GetAbsOrigin().z + 2.0f;

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

			NDebugOverlay::Line( Vector( x0, y0, flZ ), Vector( x1, y0, flZ ), r, g, bCol, false, 0.0f );
			NDebugOverlay::Line( Vector( x1, y0, flZ ), Vector( x1, y1, flZ ), r, g, bCol, false, 0.0f );
			NDebugOverlay::Line( Vector( x1, y1, flZ ), Vector( x0, y1, flZ ), r, g, bCol, false, 0.0f );
			NDebugOverlay::Line( Vector( x0, y1, flZ ), Vector( x0, y0, flZ ), r, g, bCol, false, 0.0f );
		}
	}
}

//-----------------------------------------------------------------------------
void BM_ClientUpdateCameraMode( C_TFPlayer *pLocalPlayer )
{
	if ( !pLocalPlayer || !pLocalPlayer->IsLocalPlayer() )
	{
		return;
	}

	if ( TFGameRules() && TFGameRules()->IsBombermanMode() )
	{
		g_ThirdPersonManager.SetForcedThirdPerson( true );
		g_ThirdPersonManager.SetOverridingThirdPerson( true );
		if ( !input->CAM_IsThirdPerson() )
		{
			input->CAM_ToThirdPerson();
			pLocalPlayer->ThirdPersonSwitch( true );
		}

		BM_ClientDrawGridOverlay();
	}
}

#endif // SOURCESDK
