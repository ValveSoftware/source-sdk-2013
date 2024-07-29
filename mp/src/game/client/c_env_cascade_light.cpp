//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"
#include "vguicenterprint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar scissor("r_flashlightscissor", "0");

//intensity
ConVar csm_intensity("csm_intensity", "200");
ConVar csm_second_intensity("csm_second_intensity", "200");
ConVar csm_third_intensity("csm_third_intensity", "200");
ConVar csm_brightness_general("csm_brightness_general", "1");
ConVar csm_color_r("csm_color_r", "255");
ConVar csm_color_g("csm_color_g", "255");
ConVar csm_color_b("csm_color_b", "255");
ConVar csm_color_a("csm_color_a", "255");

ConVar csm_round_pos("csm_round_pos", "1");

ConVar csm_filter("csm_filter", "1");
ConVar csm_enable("csm_enable", "1");

//-----------------------------------------------------------------------------
// Purpose: main point for change angle of the light
//-----------------------------------------------------------------------------


class C_LightOrigin : public C_BaseEntity
{
	DECLARE_CLASS(C_LightOrigin, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();

	void Update();

	virtual void Simulate();

private:
	Vector LightEnvVector;
	//QAngle LightEnvAngle;
};

IMPLEMENT_CLIENTCLASS_DT(C_LightOrigin, DT_LightOrigin, CLightOrigin)
RecvPropVector(RECVINFO(LightEnvVector))
END_RECV_TABLE()

void C_LightOrigin::Update()
{
	Vector pos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();

	pos.x = ((int)(pos.x / csm_round_pos.GetInt())) * csm_round_pos.GetInt();
	pos.y = ((int)(pos.y / csm_round_pos.GetInt())) * csm_round_pos.GetInt();
	pos.z = ((int)(pos.z / csm_round_pos.GetInt())) * csm_round_pos.GetInt();

	SetAbsOrigin(pos);
}


void C_LightOrigin::Simulate()
{
	Update();
	BaseClass::Simulate();
	ConVarRef("r_farz").SetValue("210000");
}


//-----------------------------------------------------------------------------
// Purpose: main csm code	
//-----------------------------------------------------------------------------

class C_EnvCascadeLight : public C_BaseEntity
{
	DECLARE_CLASS(C_EnvCascadeLight, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	void	ShutDownLightHandle(void);

	virtual void Simulate();

	void	UpdateLight(bool bForceUpdate);

	C_EnvCascadeLight();
	~C_EnvCascadeLight();


private:

	ClientShadowHandle_t m_LightHandle;

	EHANDLE	m_hTargetEntity;
	//color32	m_LightColor;

#ifdef MAPBASE
	float m_flBrightnessScale;
	float m_flCurrentBrightnessScale;
#endif
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;
	bool	m_bState;
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	//Vector	m_LinearFloatLightColor;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[MAX_PATH];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
};

IMPLEMENT_CLIENTCLASS_DT(C_EnvCascadeLight, DT_EnvCascadeLight, CEnvCascadeLight)
	RecvPropEHandle( RECVINFO( m_hTargetEntity )	),
	RecvPropBool(	 RECVINFO( m_bState )			),
	RecvPropFloat(	 RECVINFO( m_flLightFOV )		),
	RecvPropBool(	 RECVINFO( m_bEnableShadows )	),
	RecvPropBool(	 RECVINFO( m_bLightOnlyTarget ) ),
	//RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_IntToColor32),
	RecvPropBool(	 RECVINFO( m_bLightWorld )		),
	RecvPropBool(	 RECVINFO( m_bCameraSpace )		),
	//RecvPropVector(	 RECVINFO( m_LinearFloatLightColor )		),
	RecvPropFloat(	 RECVINFO( m_flAmbient )		),
	RecvPropString(  RECVINFO( m_SpotlightTextureName ) ),
	RecvPropInt(	 RECVINFO( m_nSpotlightTextureFrame ) ),
	RecvPropFloat(	 RECVINFO( m_flNearZ )	),
	RecvPropFloat(	 RECVINFO( m_flFarZ )	),
	RecvPropInt(	 RECVINFO( m_nShadowQuality )	)
END_RECV_TABLE()

C_EnvCascadeLight::C_EnvCascadeLight( void )
{
	
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}


C_EnvCascadeLight::~C_EnvCascadeLight( void )
{
	ShutDownLightHandle();
}

void C_EnvCascadeLight::ShutDownLightHandle( void )
{
	if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_LightHandle);
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - how do you increase the light's height?
//-----------------------------------------------------------------------------
void C_EnvCascadeLight::OnDataChanged( DataUpdateType_t updateType )
{
	UpdateLight( true );

	BaseClass::OnDataChanged( updateType );
}


void C_EnvCascadeLight::UpdateLight( bool bForceUpdate )
{

	if (m_bState == false)
	{
		if(m_LightHandle!=CLIENTSHADOW_INVALID_HANDLE)
		ShutDownLightHandle();
		return;
	}

	Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
	FlashlightState_t state;
	state.m_flShadowFilterSize = csm_filter.GetFloat();
	

	if (m_hTargetEntity != NULL)
	{
		if (m_bCameraSpace)
		{
			const QAngle& angles = GetLocalAngles();

			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (pPlayer)
			{
				const QAngle playerAngles = pPlayer->GetAbsAngles();

				Vector vPlayerForward, vPlayerRight, vPlayerUp;
				AngleVectors(playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp);

				matrix3x4_t	mRotMatrix;
				AngleMatrix(angles, mRotMatrix);

				VectorITransform(vPlayerForward, mRotMatrix, vForward);
				VectorITransform(vPlayerRight, mRotMatrix, vRight);
				VectorITransform(vPlayerUp, mRotMatrix, vUp);

				float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
				vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

				VectorNormalize(vForward);
				VectorNormalize(vRight);
				VectorNormalize(vUp);
			}
		}
		else
		{
			vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(vForward);

			Assert(0);

		}
	}
	else
	{
		AngleVectors(GetAbsAngles(), &vForward, &vRight, &vUp);
	}
	

	Vector vLinearFloatLightColor(csm_color_r.GetInt(), csm_color_g.GetInt(), csm_color_b.GetInt());
	float flLinearFloatLightAlpha = csm_color_a.GetInt();

#ifdef MAPBASE
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		if (m_flColorTransitionTime != 0.0f)
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
			m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
			//m_flCurrentBrightnessScale = Approach(m_flBrightnessScale, m_flCurrentBrightnessScale, flColorTransitionSpeed);
		}
		else
		{
			// Just do it instantly
			m_CurrentLinearFloatLightColor.x = vLinearFloatLightColor.x;
			m_CurrentLinearFloatLightColor.y = vLinearFloatLightColor.y;
			m_CurrentLinearFloatLightColor.z = vLinearFloatLightColor.z;
			m_flCurrentLinearFloatLightAlpha = flLinearFloatLightAlpha;
			//m_flCurrentBrightnessScale = m_flBrightnessScale;
		}
	}
#else
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
		m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
	}
#endif



	state.m_fHorizontalFOVDegrees = m_flLightFOV;
	state.m_fVerticalFOVDegrees = m_flLightFOV;

	state.m_vecLightOrigin = vPos;
	BasisToQuaternion(vForward, vRight, vUp, state.m_quatOrientation);

	state.m_fQuadraticAtten = 0.0;
	state.m_fLinearAtten = 100;
	state.m_fConstantAtten = 0.0f;
	//state.m_Color[0] = m_LinearFloatLightColor.x;
	//state.m_Color[1] = m_LinearFloatLightColor.y;
	//state.m_Color[2] = m_LinearFloatLightColor.z;

#ifdef MAPBASE
	float flAlpha = m_flCurrentLinearFloatLightAlpha * (1.0f / 255.0f);
	state.m_Color[0] = (m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha) * csm_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[1] = (m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha) * csm_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[2] = (m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha) * csm_intensity.GetFloat() * csm_brightness_general.GetFloat();
#else
	state.m_Color[0] = m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * csm_intensity.GetFloat();
	state.m_Color[1] = m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * csm_intensity.GetFloat();
	state.m_Color[2] = m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * csm_intensity.GetFloat();
#endif

	state.m_Color[3] = m_flAmbient; // fixme: need to make ambient work m_flAmbient;
	state.m_NearZ = m_flNearZ;
	state.m_FarZ = m_flFarZ;
	state.m_flShadowSlopeScaleDepthBias = ConVarRef("mat_slopescaledepthbias_shadowmap").GetFloat();//csm_slopescaledepthbias_shadowmap.GetFloat();
	state.m_flShadowDepthBias = ConVarRef("mat_depthbias_shadowmap").GetFloat();//csm_depthbias_shadowmap.GetFloat();
	state.m_bEnableShadows = m_bEnableShadows;
	state.m_pSpotlightTexture = materials->FindTexture(m_SpotlightTextureName, TEXTURE_GROUP_OTHER, false);
	state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

	state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

	if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
	{
		m_LightHandle = g_pClientShadowMgr->CreateFlashlight(state);
	}
	else
	{
		if (m_hTargetEntity != NULL || bForceUpdate == true)
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, state);
		}
	}

	if (m_bLightOnlyTarget)
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, m_hTargetEntity);
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, NULL);
	}

	g_pClientShadowMgr->SetFlashlightLightWorld(m_LightHandle, m_bLightWorld);


	g_pClientShadowMgr->UpdateProjectedTexture(m_LightHandle, true);


}

void C_EnvCascadeLight::Simulate( void )
{
	
	m_bState = csm_enable.GetBool();
	UpdateLight(true);
	BaseClass::Simulate();
}


//-----------------------------------------------------------------------------
// Purpose:	second csm	
//-----------------------------------------------------------------------------

class C_EnvCascadeLightSecond : public C_BaseEntity
{
	DECLARE_CLASS(C_EnvCascadeLightSecond, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	void	ShutDownLightHandle(void);

	virtual void Simulate();

	void	UpdateLight(bool bForceUpdate);
	void updatePos();

	C_EnvCascadeLightSecond();
	~C_EnvCascadeLightSecond();


private:

	ClientShadowHandle_t m_LightHandle;

	//color32	m_LightColor;

#ifdef MAPBASE
	float m_flBrightnessScale;
	float m_flCurrentBrightnessScale;
#endif
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;

	EHANDLE	m_hTargetEntity;
	CBaseEntity* pEntity = NULL;
	bool	firstUpdate = true;
	bool	m_bState;	
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	//Vector	m_LinearFloatLightColor;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[MAX_PATH];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
};

IMPLEMENT_CLIENTCLASS_DT(C_EnvCascadeLightSecond, DT_EnvCascadeLightSecond, CEnvCascadeLightSecond)
//RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_IntToColor32),
RecvPropEHandle(RECVINFO(m_hTargetEntity)),
RecvPropBool(RECVINFO(m_bState)),
RecvPropFloat(RECVINFO(m_flLightFOV)),
RecvPropBool(RECVINFO(m_bEnableShadows)),
RecvPropBool(RECVINFO(m_bLightOnlyTarget)),
RecvPropBool(RECVINFO(m_bLightWorld)),
RecvPropBool(RECVINFO(m_bCameraSpace)),
//RecvPropVector(RECVINFO(m_LinearFloatLightColor)),
RecvPropFloat(RECVINFO(m_flAmbient)),
RecvPropString(RECVINFO(m_SpotlightTextureName)),
RecvPropInt(RECVINFO(m_nSpotlightTextureFrame)),
RecvPropFloat(RECVINFO(m_flNearZ)),
RecvPropFloat(RECVINFO(m_flFarZ)),
RecvPropInt(RECVINFO(m_nShadowQuality))
END_RECV_TABLE()

C_EnvCascadeLightSecond::C_EnvCascadeLightSecond(void)
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}


C_EnvCascadeLightSecond::~C_EnvCascadeLightSecond(void)
{
	ShutDownLightHandle();
}

void C_EnvCascadeLightSecond::ShutDownLightHandle(void)
{
	if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_LightHandle);
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_EnvCascadeLightSecond::OnDataChanged(DataUpdateType_t updateType)
{
	UpdateLight(true);
	BaseClass::OnDataChanged(updateType);
}

void C_EnvCascadeLightSecond::UpdateLight(bool bForceUpdate)
{

	if (m_bState == false)
	{
		
		if(m_LightHandle!=CLIENTSHADOW_INVALID_HANDLE)
		ShutDownLightHandle();
		return;
	}



	Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
	FlashlightState_t state;
	state.m_flShadowFilterSize = csm_filter.GetFloat();

	if (m_hTargetEntity != NULL)
	{
		if (m_bCameraSpace)
		{
			const QAngle& angles = GetLocalAngles();

			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (pPlayer)
			{
				const QAngle playerAngles = pPlayer->GetAbsAngles();

				Vector vPlayerForward, vPlayerRight, vPlayerUp;
				AngleVectors(playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp);

				matrix3x4_t	mRotMatrix;
				AngleMatrix(angles, mRotMatrix);

				VectorITransform(vPlayerForward, mRotMatrix, vForward);
				VectorITransform(vPlayerRight, mRotMatrix, vRight);
				VectorITransform(vPlayerUp, mRotMatrix, vUp);

				float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
				vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

				VectorNormalize(vForward);
				VectorNormalize(vRight);
				VectorNormalize(vUp);
			}
		}
		else
		{
			vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(vForward);
			Assert(0);
		}
	}
	else
	{
		AngleVectors(GetAbsAngles(), &vForward, &vRight, &vUp);
	}

	Vector vLinearFloatLightColor(csm_color_r.GetInt(), csm_color_g.GetInt(), csm_color_b.GetInt());
	float flLinearFloatLightAlpha = csm_color_a.GetInt();

#ifdef MAPBASE
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		if (m_flColorTransitionTime != 0.0f)
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
			m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
			//m_flCurrentBrightnessScale = Approach(m_flBrightnessScale, m_flCurrentBrightnessScale, flColorTransitionSpeed);
		}
		else
		{
			// Just do it instantly
			m_CurrentLinearFloatLightColor.x = vLinearFloatLightColor.x;
			m_CurrentLinearFloatLightColor.y = vLinearFloatLightColor.y;
			m_CurrentLinearFloatLightColor.z = vLinearFloatLightColor.z;
			m_flCurrentLinearFloatLightAlpha = flLinearFloatLightAlpha;
			//m_flCurrentBrightnessScale = m_flBrightnessScale;
		}
	}
#else
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
		m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
	}
#endif


	state.m_fHorizontalFOVDegrees = m_flLightFOV;
	state.m_fVerticalFOVDegrees = m_flLightFOV;

	state.m_vecLightOrigin = vPos;
	BasisToQuaternion(vForward, vRight, vUp, state.m_quatOrientation);

	state.m_fQuadraticAtten = 0.0;
	state.m_fLinearAtten = 100;
	state.m_fConstantAtten = 0.0f;

#ifdef MAPBASE
	float flAlpha = m_flCurrentLinearFloatLightAlpha * (1.0f / 255.0f);
	state.m_Color[0] = (m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha) * csm_second_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[1] = (m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha) * csm_second_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[2] = (m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha) * csm_second_intensity.GetFloat() * csm_brightness_general.GetFloat();
#else
	state.m_Color[0] = m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * csm_second_intensity.GetFloat();
	state.m_Color[1] = m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * csm_second_intensity.GetFloat();
	state.m_Color[2] = m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * csm_second_intensity.GetFloat();
#endif

	state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
	state.m_NearZ = m_flNearZ;
	state.m_FarZ = m_flFarZ;
	state.m_flShadowSlopeScaleDepthBias = ConVarRef("mat_slopescaledepthbias_shadowmap").GetFloat();
	state.m_flShadowDepthBias = ConVarRef("mat_depthbias_shadowmap").GetFloat();
	state.m_bEnableShadows = m_bEnableShadows;
	state.m_pSpotlightTexture = materials->FindTexture(m_SpotlightTextureName, TEXTURE_GROUP_OTHER, false);
	state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

	state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality
	

	if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
	{
		m_LightHandle = g_pClientShadowMgr->CreateFlashlight(state);
	}
	else
	{
		if (m_hTargetEntity != NULL || bForceUpdate == true)
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, state);
		}
	}

	if (m_bLightOnlyTarget)
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, m_hTargetEntity);
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, NULL);
	}

	g_pClientShadowMgr->SetFlashlightLightWorld(m_LightHandle, m_bLightWorld);

	g_pClientShadowMgr->UpdateProjectedTexture(m_LightHandle, true);
	
	scissor.SetValue("0");
}


void C_EnvCascadeLightSecond::Simulate(void)
{
	m_bState = csm_enable.GetBool();
	UpdateLight(true);
	BaseClass::Simulate();
}

//-----------------------------------------------------------------------------
// Purpose:	second csm	
//-----------------------------------------------------------------------------

class C_EnvCascadeLightThird : public C_BaseEntity
{
	DECLARE_CLASS(C_EnvCascadeLightThird, C_BaseEntity);
public:
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	void	ShutDownLightHandle(void);

	virtual void Simulate();

	void	UpdateLight(bool bForceUpdate);
	void updatePos();

	C_EnvCascadeLightThird();
	~C_EnvCascadeLightThird();


private:

	ClientShadowHandle_t m_LightHandle;

	//color32	m_LightColor;

#ifdef MAPBASE
	float m_flBrightnessScale;
	float m_flCurrentBrightnessScale;
#endif
	Vector m_CurrentLinearFloatLightColor;
	float m_flCurrentLinearFloatLightAlpha;
	float m_flColorTransitionTime;

	EHANDLE	m_hTargetEntity;
	CBaseEntity* pEntity = NULL;
	bool	firstUpdate = true;
	bool	m_bState;
	float	m_flLightFOV;
	bool	m_bEnableShadows;
	bool	m_bLightOnlyTarget;
	bool	m_bLightWorld;
	bool	m_bCameraSpace;
	//Vector	m_LinearFloatLightColor;
	float	m_flAmbient;
	float	m_flNearZ;
	float	m_flFarZ;
	char	m_SpotlightTextureName[MAX_PATH];
	int		m_nSpotlightTextureFrame;
	int		m_nShadowQuality;
};

IMPLEMENT_CLIENTCLASS_DT(C_EnvCascadeLightThird, DT_EnvCascadeLightThird, CEnvCascadeLightThird)
//RecvPropInt(RECVINFO(m_LightColor), 0, RecvProxy_IntToColor32),	
RecvPropEHandle(RECVINFO(m_hTargetEntity)),
RecvPropBool(RECVINFO(m_bState)),
RecvPropFloat(RECVINFO(m_flLightFOV)),
RecvPropBool(RECVINFO(m_bEnableShadows)),
RecvPropBool(RECVINFO(m_bLightOnlyTarget)),
RecvPropBool(RECVINFO(m_bLightWorld)),
RecvPropBool(RECVINFO(m_bCameraSpace)),
//RecvPropVector(RECVINFO(m_LinearFloatLightColor)),
RecvPropFloat(RECVINFO(m_flAmbient)),
RecvPropString(RECVINFO(m_SpotlightTextureName)),
RecvPropInt(RECVINFO(m_nSpotlightTextureFrame)),
RecvPropFloat(RECVINFO(m_flNearZ)),
RecvPropFloat(RECVINFO(m_flFarZ)),
RecvPropInt(RECVINFO(m_nShadowQuality))
END_RECV_TABLE()

C_EnvCascadeLightThird::C_EnvCascadeLightThird(void)
{
	m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
}


C_EnvCascadeLightThird::~C_EnvCascadeLightThird(void)
{
	ShutDownLightHandle();
}

void C_EnvCascadeLightThird::ShutDownLightHandle(void)
{
	if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
	{
		g_pClientShadowMgr->DestroyFlashlight(m_LightHandle);
		m_LightHandle = CLIENTSHADOW_INVALID_HANDLE;
	}
}

void C_EnvCascadeLightThird::OnDataChanged(DataUpdateType_t updateType)
{
	UpdateLight(true);
	BaseClass::OnDataChanged(updateType);
}

void C_EnvCascadeLightThird::UpdateLight(bool bForceUpdate)
{

	if (m_bState == false)
	{
		if (m_LightHandle != CLIENTSHADOW_INVALID_HANDLE)
			ShutDownLightHandle();
		return;
	}

	Vector vForward, vRight, vUp, vPos = GetAbsOrigin();
	FlashlightState_t state;
	state.m_flShadowFilterSize = csm_filter.GetFloat();

	if (m_hTargetEntity != NULL)
	{
		if (m_bCameraSpace)
		{
			const QAngle& angles = GetLocalAngles();

			C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
			if (pPlayer)
			{
				const QAngle playerAngles = pPlayer->GetAbsAngles();

				Vector vPlayerForward, vPlayerRight, vPlayerUp;
				AngleVectors(playerAngles, &vPlayerForward, &vPlayerRight, &vPlayerUp);

				matrix3x4_t	mRotMatrix;
				AngleMatrix(angles, mRotMatrix);

				VectorITransform(vPlayerForward, mRotMatrix, vForward);
				VectorITransform(vPlayerRight, mRotMatrix, vRight);
				VectorITransform(vPlayerUp, mRotMatrix, vUp);

				float dist = (m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin()).Length();
				vPos = m_hTargetEntity->GetAbsOrigin() - vForward * dist;

				VectorNormalize(vForward);
				VectorNormalize(vRight);
				VectorNormalize(vUp);
			}
		}
		else
		{
			vForward = m_hTargetEntity->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(vForward);
			Assert(0);
		}
	}
	else
	{
		AngleVectors(GetAbsAngles(), &vForward, &vRight, &vUp);
	}

	Vector vLinearFloatLightColor(csm_color_r.GetInt(), csm_color_g.GetInt(), csm_color_b.GetInt());
	float flLinearFloatLightAlpha = csm_color_a.GetInt();

#ifdef MAPBASE
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		if (m_flColorTransitionTime != 0.0f)
		{
			float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

			m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
			m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
			m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
			//m_flCurrentBrightnessScale = Approach(m_flBrightnessScale, m_flCurrentBrightnessScale, flColorTransitionSpeed);
		}
		else
		{
			// Just do it instantly
			m_CurrentLinearFloatLightColor.x = vLinearFloatLightColor.x;
			m_CurrentLinearFloatLightColor.y = vLinearFloatLightColor.y;
			m_CurrentLinearFloatLightColor.z = vLinearFloatLightColor.z;
			m_flCurrentLinearFloatLightAlpha = flLinearFloatLightAlpha;
			//m_flCurrentBrightnessScale = m_flBrightnessScale;
		}
	}
#else
	if (m_CurrentLinearFloatLightColor != vLinearFloatLightColor || m_flCurrentLinearFloatLightAlpha != flLinearFloatLightAlpha)
	{
		float flColorTransitionSpeed = gpGlobals->frametime * m_flColorTransitionTime * 255.0f;

		m_CurrentLinearFloatLightColor.x = Approach(vLinearFloatLightColor.x, m_CurrentLinearFloatLightColor.x, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.y = Approach(vLinearFloatLightColor.y, m_CurrentLinearFloatLightColor.y, flColorTransitionSpeed);
		m_CurrentLinearFloatLightColor.z = Approach(vLinearFloatLightColor.z, m_CurrentLinearFloatLightColor.z, flColorTransitionSpeed);
		m_flCurrentLinearFloatLightAlpha = Approach(flLinearFloatLightAlpha, m_flCurrentLinearFloatLightAlpha, flColorTransitionSpeed);
	}
#endif



	state.m_fHorizontalFOVDegrees = m_flLightFOV;
	state.m_fVerticalFOVDegrees = m_flLightFOV;

	state.m_vecLightOrigin = vPos;
	BasisToQuaternion(vForward, vRight, vUp, state.m_quatOrientation);

	state.m_fQuadraticAtten = 0.0;
	state.m_fLinearAtten = 100;
	state.m_fConstantAtten = 0.0f;
	//state.m_Color[0] = m_LinearFloatLightColor.x;
	//state.m_Color[1] = m_LinearFloatLightColor.y;
	//state.m_Color[2] = m_LinearFloatLightColor.z;

#ifdef MAPBASE
	float flAlpha = m_flCurrentLinearFloatLightAlpha * (1.0f / 255.0f);
	state.m_Color[0] = (m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * flAlpha) * csm_third_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[1] = (m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * flAlpha) * csm_third_intensity.GetFloat() * csm_brightness_general.GetFloat();
	state.m_Color[2] = (m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * flAlpha) * csm_third_intensity.GetFloat() * csm_brightness_general.GetFloat();
#else
	state.m_Color[0] = m_CurrentLinearFloatLightColor.x * (1.0f / 255.0f) * csm_third_intensity.GetFloat();
	state.m_Color[1] = m_CurrentLinearFloatLightColor.y * (1.0f / 255.0f) * csm_third_intensity.GetFloat();
	state.m_Color[2] = m_CurrentLinearFloatLightColor.z * (1.0f / 255.0f) * csm_third_intensity.GetFloat();
#endif

	state.m_Color[3] = 0.0f; // fixme: need to make ambient work m_flAmbient;
	state.m_NearZ = m_flNearZ;
	state.m_FarZ = m_flFarZ;
	state.m_flShadowSlopeScaleDepthBias = ConVarRef("mat_slopescaledepthbias_shadowmap").GetFloat();//csm_slopescaledepthbias_shadowmap.GetFloat();
	state.m_flShadowDepthBias = ConVarRef("mat_depthbias_shadowmap").GetFloat();//csm_depthbias_shadowmap.GetFloat();
	state.m_bEnableShadows = m_bEnableShadows;
	state.m_pSpotlightTexture = materials->FindTexture(m_SpotlightTextureName, TEXTURE_GROUP_OTHER, false);
	state.m_nSpotlightTextureFrame = m_nSpotlightTextureFrame;

	state.m_nShadowQuality = m_nShadowQuality; // Allow entity to affect shadow quality

	if (m_LightHandle == CLIENTSHADOW_INVALID_HANDLE)
	{
		m_LightHandle = g_pClientShadowMgr->CreateFlashlight(state);
	}
	else
	{
		if (m_hTargetEntity != NULL || bForceUpdate == true)
		{
			g_pClientShadowMgr->UpdateFlashlightState(m_LightHandle, state);
		}
	}

	if (m_bLightOnlyTarget)
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, m_hTargetEntity);
	}
	else
	{
		g_pClientShadowMgr->SetFlashlightTarget(m_LightHandle, NULL);
	}

	g_pClientShadowMgr->SetFlashlightLightWorld(m_LightHandle, m_bLightWorld);

#ifdef MAPBASE
	if (state.m_bOrtho)
	{
		bool bSupressWorldLights = false;

		bSupressWorldLights = m_bEnableShadows;

		g_pClientShadowMgr->SetShadowFromWorldLightsEnabled(!bSupressWorldLights);
	}
#endif // MAPBASE

	g_pClientShadowMgr->UpdateProjectedTexture(m_LightHandle, true);

	scissor.SetValue("0");
}


void C_EnvCascadeLightThird::Simulate(void)
{
	m_bState = csm_enable.GetBool();
	UpdateLight(true);
	BaseClass::Simulate();
}
