#include "cbase.h"
#include "c_muzzleflash_effect.h"
#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"

extern ConVar r_flashlightdepthres;

C_MuzzleflashEffect::C_MuzzleflashEffect()
{
	m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;

	m_FlashlightTexture.Init( "effects/muzzleflashlight", TEXTURE_GROUP_OTHER, true );

	m_flHorizontalFOV = 90.0f;
}

C_MuzzleflashEffect::~C_MuzzleflashEffect()
{
	if ( m_FlashlightHandle != CLIENTSHADOW_INVALID_HANDLE )
	{
		g_pClientShadowMgr->DestroyFlashlight( m_FlashlightHandle );
		m_FlashlightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_MuzzleflashEffect::UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flStrength )
{
	FlashlightState_t state;

	Quaternion quat;
	BasisToQuaternion( vecDir, vecRight, vecUp, quat );
	state.m_fLinearAtten = 2500.0f * flStrength;
	state.m_fQuadraticAtten = 500.0f * flStrength;

	state.m_vecLightOrigin = vecPos;
	state.m_quatOrientation = quat;
	state.m_fVerticalFOVDegrees = state.m_fHorizontalFOVDegrees = 100.0f + 40.0f * flStrength;
	state.m_fConstantAtten = 0.0f;
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 0.7f;
	state.m_Color[2] = 0.3f;
	state.m_Color[3] = 0.0f;
	state.m_NearZ = 5.0f;	// Push near plane out so that we don't clip the world when the flashlight pulls back 
	state.m_FarZ = 500.0f;
	state.m_bEnableShadows = true;
	state.m_flShadowMapResolution = r_flashlightdepthres.GetInt();

	m_flHorizontalFOV = state.m_fHorizontalFOVDegrees;

	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_nSpotlightTextureFrame = 0;

	state.m_flShadowAtten = 0.4f;
	state.m_flShadowSlopeScaleDepthBias = 16.0f;
	state.m_flShadowDepthBias = 0.0005f;

	if ( m_FlashlightHandle == CLIENTSHADOW_INVALID_HANDLE )
	{
		m_FlashlightHandle = g_pClientShadowMgr->CreateFlashlight( state );
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState( m_FlashlightHandle, state );
	}
	
	g_pClientShadowMgr->UpdateProjectedTexture( m_FlashlightHandle, true );
}