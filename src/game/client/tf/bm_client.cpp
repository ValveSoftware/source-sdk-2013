//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#ifdef SOURCESDK

#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "view_shared.h"
#include "cam_thirdperson.h"
#include "input.h"

ConVar tf_bm_cam_height( "tf_bm_cam_height", "720", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera height above the player." );
ConVar tf_bm_cam_pitch( "tf_bm_cam_pitch", "89", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Bomberman: camera pitch (89 = top-down)." );

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
	pSetup->origin.x = vecPlayer.x;
	pSetup->origin.y = vecPlayer.y;
	pSetup->origin.z = vecPlayer.z + tf_bm_cam_height.GetFloat();

	pSetup->angles.x = tf_bm_cam_pitch.GetFloat();
	pSetup->angles.y = 0.0f;
	pSetup->angles.z = 0.0f;

	pSetup->m_bOrtho = false;
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
		if ( !input->CAM_IsThirdPerson() )
		{
			input->CAM_ToThirdPerson();
			pLocalPlayer->ThirdPersonSwitch( true );
		}
	}
}

#endif // SOURCESDK
