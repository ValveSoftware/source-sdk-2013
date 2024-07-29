//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity to control screen overlays on a player
//
//=============================================================================

#include "cbase.h"
#include "shareddefs.h"
#include "lights.h"
#include "tier1/utlstring.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define ENV_CASCADE_STARTON			(1<<0)

//distance
static ConVar defdist("csm_default_distance", "1000", FCVAR_DEVELOPMENTONLY, "Default Z distance. Used for some fov calculations. Please dont change");
ConVar curdist("csm_current_distance", "100000", 0, "Current Z distance. You can change it.");

//fov things
static ConVar defFOV("csm_default_fov", "15", FCVAR_DEVELOPMENTONLY, "Default FOV. Used for some fov calculations. Please dont change");
ConVar curFOV("csm_current_fov", "15", 0, "Current FOV. You can change it");
ConVar csm_second_fov("csm_second_fov", "360", FCVAR_NONE, "FOV of the second csm.");
ConVar csm_third_fov("csm_third_fov", "2800", FCVAR_NONE, "FOV of the second csm.");

//farz and nearz
ConVar csm_nearz("csm_nearz", "90000");
ConVar csm_farz("csm_farz", "200000");




class CLightOrigin : public CPointEntity
{
	DECLARE_CLASS(CLightOrigin, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void Spawn();

	CLightOrigin();

	bool angFEnv = false;
	
	void InitialThink(void);

private:

	CNetworkVector(LightEnvVector);
};

LINK_ENTITY_TO_CLASS(csm_origin, CLightOrigin);

BEGIN_DATADESC(CLightOrigin)
DEFINE_FIELD(LightEnvVector, FIELD_VECTOR),
DEFINE_THINKFUNC(InitialThink)
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CLightOrigin, DT_LightOrigin)
SendPropVector(SENDINFO(LightEnvVector))
END_SEND_TABLE()

CLightOrigin::CLightOrigin() 
{
}

void CLightOrigin::Spawn()
{
	if (angFEnv)
	{
		CBaseEntity* pEntity = NULL;
		pEntity = gEntList.FindEntityByClassname(pEntity, "light_environment");
		if (pEntity)
		{
			CEnvLight* pEnv = dynamic_cast<CEnvLight*>(pEntity);
			
			SetAbsAngles(QAngle(-90 - pEnv->m_iPitch, pEnv->GetAbsAngles().y, pEnv->GetAbsAngles().z));
			DevMsg("[FAKE CSM]		light_environment Founded!\n");
		}
		else
		{
			//Msg("What the fuck? Map dont have light_environment with targetname!");
			DevMsg("[FAKE CSM]		Cant find light_environment with targetname!\n");
		}
	}
}

void CLightOrigin::InitialThink()
{
}


//-----------------------------------------------------------------------------
// Purpose: third csm
//-----------------------------------------------------------------------------

class CEnvCascadeLightThird : public CPointEntity
{
	DECLARE_CLASS(CEnvCascadeLightThird, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvCascadeLightThird();
	bool KeyValue(const char* szKeyName, const char* szValue);

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Activate(void);

	void InitialThink(void);

	CNetworkHandle(CBaseEntity, m_hTargetEntity);
	CNetworkVar(float, m_flNearZ);
	CNetworkVar(float, m_flFarZ);

private:

	CNetworkVar(bool, m_bState);
	CNetworkVar(float, m_flLightFOV);
	CNetworkVar(bool, m_bEnableShadows);
	CNetworkVar(bool, m_bLightOnlyTarget);
	CNetworkVar(bool, m_bLightWorld);
	CNetworkVar(bool, m_bCameraSpace);
	CNetworkVar(float, m_flAmbient);
	CNetworkString(m_SpotlightTextureName, MAX_PATH);
	CNetworkVar(int, m_nSpotlightTextureFrame);
	CNetworkVar(int, m_nShadowQuality);


};

LINK_ENTITY_TO_CLASS(csm_third, CEnvCascadeLightThird);

BEGIN_DATADESC(CEnvCascadeLightThird)
DEFINE_FIELD(m_hTargetEntity, FIELD_EHANDLE),
DEFINE_FIELD(m_bState, FIELD_BOOLEAN),
DEFINE_KEYFIELD(m_flLightFOV, FIELD_FLOAT, "lightfov"),
DEFINE_KEYFIELD(m_bEnableShadows, FIELD_BOOLEAN, "enableshadows"),
DEFINE_KEYFIELD(m_bLightOnlyTarget, FIELD_BOOLEAN, "lightonlytarget"),
DEFINE_KEYFIELD(m_bLightWorld, FIELD_BOOLEAN, "lightworld"),
DEFINE_KEYFIELD(m_bCameraSpace, FIELD_BOOLEAN, "cameraspace"),
DEFINE_KEYFIELD(m_flAmbient, FIELD_FLOAT, "ambient"),
DEFINE_AUTO_ARRAY_KEYFIELD(m_SpotlightTextureName, FIELD_CHARACTER, "texturename"),
DEFINE_KEYFIELD(m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe"),
DEFINE_KEYFIELD(m_flNearZ, FIELD_FLOAT, "nearz"),
DEFINE_KEYFIELD(m_flFarZ, FIELD_FLOAT, "farz"),
DEFINE_KEYFIELD(m_nShadowQuality, FIELD_INTEGER, "shadowquality"),
DEFINE_THINKFUNC(InitialThink),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CEnvCascadeLightThird, DT_EnvCascadeLightThird)
SendPropEHandle(SENDINFO(m_hTargetEntity)),
SendPropBool(SENDINFO(m_bState)),
SendPropFloat(SENDINFO(m_flLightFOV)),
SendPropBool(SENDINFO(m_bEnableShadows)),
SendPropBool(SENDINFO(m_bLightOnlyTarget)),
SendPropBool(SENDINFO(m_bLightWorld)),
SendPropBool(SENDINFO(m_bCameraSpace)),
SendPropFloat(SENDINFO(m_flAmbient)),
SendPropString(SENDINFO(m_SpotlightTextureName)),
SendPropInt(SENDINFO(m_nSpotlightTextureFrame)),
SendPropFloat(SENDINFO(m_flNearZ), 16, SPROP_ROUNDDOWN, 0.0f, 500.0f),
SendPropFloat(SENDINFO(m_flFarZ), 18, SPROP_ROUNDDOWN, 0.0f, 1500.0f),
SendPropInt(SENDINFO(m_nShadowQuality), 1, SPROP_UNSIGNED)  // Just one bit for now
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvCascadeLightThird::CEnvCascadeLightThird(void)
{
	m_bState = true;
	m_flLightFOV = 45.0f;
	m_bEnableShadows = true;
	m_bLightOnlyTarget = false;
	m_bLightWorld = true;
	m_bCameraSpace = false;

	Q_strcpy(m_SpotlightTextureName.GetForModify(), "tools\\fakecsm\\mask_ring");
	m_nSpotlightTextureFrame = 0;
	m_flAmbient = 0.0f;
	m_flNearZ = csm_nearz.GetFloat();
	m_flFarZ = csm_farz.GetFloat();
	m_nShadowQuality = 0;
}

void UTIL_ColorStringToLinearFloatColorCSMFakeThird(Vector& color, const char* pString)
{
	float tmp[4];
	UTIL_StringToFloatArray(tmp, 4, pString);
	if (tmp[3] <= 0.0f)
	{
		tmp[3] = 255.0f;
	}
	tmp[3] *= (1.0f / 255.0f);
	color.x = GammaToLinear(tmp[0] * (1.0f / 255.0f)) * tmp[3];
	color.y = GammaToLinear(tmp[1] * (1.0f / 255.0f)) * tmp[3];
	color.z = GammaToLinear(tmp[2] * (1.0f / 255.0f)) * tmp[3];
}

bool CEnvCascadeLightThird::KeyValue(const char* szKeyName, const char* szValue)
{

	return BaseClass::KeyValue(szKeyName, szValue);

}

void CEnvCascadeLightThird::Activate(void)
{
	if (GetSpawnFlags() & ENV_CASCADE_STARTON)
	{
		m_bState = true;
	}

	SetThink(&CEnvCascadeLightThird::InitialThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	BaseClass::Activate();
}

void CEnvCascadeLightThird::InitialThink(void)
{
	float bibigon = defdist.GetFloat() / curdist.GetFloat();
	m_flLightFOV = csm_third_fov.GetFloat() * bibigon;
	m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target);
}

int CEnvCascadeLightThird::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}


//-----------------------------------------------------------------------------
// Purpose: second csm
//-----------------------------------------------------------------------------

class CEnvCascadeLightSecond : public CPointEntity
{
	DECLARE_CLASS(CEnvCascadeLightSecond, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvCascadeLightSecond();
	bool KeyValue(const char* szKeyName, const char* szValue);

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Activate(void);

	void InitialThink(void);

	CNetworkHandle(CBaseEntity, m_hTargetEntity);
	CNetworkVar(float, m_flNearZ);
	CNetworkVar(float, m_flFarZ);

private:
	
	CNetworkVar(bool, m_bState);
	CNetworkVar(float, m_flLightFOV);
	CNetworkVar(bool, m_bEnableShadows);
	CNetworkVar(bool, m_bLightOnlyTarget);
	CNetworkVar(bool, m_bLightWorld);
	CNetworkVar(bool, m_bCameraSpace);
	CNetworkVar(float, m_flAmbient);
	CNetworkString(m_SpotlightTextureName, MAX_PATH);
	CNetworkVar(int, m_nSpotlightTextureFrame);
	CNetworkVar(int, m_nShadowQuality);


};

LINK_ENTITY_TO_CLASS(csm_second, CEnvCascadeLightSecond);

BEGIN_DATADESC(CEnvCascadeLightSecond)
DEFINE_FIELD(m_hTargetEntity, FIELD_EHANDLE),
DEFINE_FIELD(m_bState, FIELD_BOOLEAN),
DEFINE_KEYFIELD(m_flLightFOV, FIELD_FLOAT, "lightfov"),
DEFINE_KEYFIELD(m_bEnableShadows, FIELD_BOOLEAN, "enableshadows"),
DEFINE_KEYFIELD(m_bLightOnlyTarget, FIELD_BOOLEAN, "lightonlytarget"),
DEFINE_KEYFIELD(m_bLightWorld, FIELD_BOOLEAN, "lightworld"),
DEFINE_KEYFIELD(m_bCameraSpace, FIELD_BOOLEAN, "cameraspace"),
DEFINE_KEYFIELD(m_flAmbient, FIELD_FLOAT, "ambient"),
DEFINE_AUTO_ARRAY_KEYFIELD(m_SpotlightTextureName, FIELD_CHARACTER, "texturename"),
DEFINE_KEYFIELD(m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe"),
DEFINE_KEYFIELD(m_flNearZ, FIELD_FLOAT, "nearz"),
DEFINE_KEYFIELD(m_flFarZ, FIELD_FLOAT, "farz"),
DEFINE_KEYFIELD(m_nShadowQuality, FIELD_INTEGER, "shadowquality"),
DEFINE_THINKFUNC(InitialThink),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CEnvCascadeLightSecond, DT_EnvCascadeLightSecond)
SendPropEHandle(SENDINFO(m_hTargetEntity)),
SendPropBool(SENDINFO(m_bState)),
SendPropFloat(SENDINFO(m_flLightFOV)),
SendPropBool(SENDINFO(m_bEnableShadows)),
SendPropBool(SENDINFO(m_bLightOnlyTarget)),
SendPropBool(SENDINFO(m_bLightWorld)),
SendPropBool(SENDINFO(m_bCameraSpace)),
SendPropFloat(SENDINFO(m_flAmbient)),
SendPropString(SENDINFO(m_SpotlightTextureName)),
SendPropInt(SENDINFO(m_nSpotlightTextureFrame)),
SendPropFloat(SENDINFO(m_flNearZ), 16, SPROP_ROUNDDOWN, 0.0f, 500.0f),
SendPropFloat(SENDINFO(m_flFarZ), 18, SPROP_ROUNDDOWN, 0.0f, 1500.0f),
SendPropInt(SENDINFO(m_nShadowQuality), 1, SPROP_UNSIGNED)  // Just one bit for now
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvCascadeLightSecond::CEnvCascadeLightSecond(void)
{
	m_bState = true;
	m_flLightFOV = 45.0f;
	m_bEnableShadows = true;
	m_bLightOnlyTarget = false;
	m_bLightWorld = true;
	m_bCameraSpace = false;

	Q_strcpy(m_SpotlightTextureName.GetForModify(), "tools\\fakecsm\\mask_ring");
	m_nSpotlightTextureFrame = 0;
	m_flAmbient = 0.0f;
	m_flNearZ = csm_nearz.GetFloat();
	m_flFarZ = csm_farz.GetFloat();
	m_nShadowQuality = 0;
}

void UTIL_ColorStringToLinearFloatColorCSMFakeSecond(Vector& color, const char* pString)
{
	float tmp[4];
	UTIL_StringToFloatArray(tmp, 4, pString);
	if (tmp[3] <= 0.0f)
	{
		tmp[3] = 255.0f;
	}
	tmp[3] *= (1.0f / 255.0f);
	color.x = GammaToLinear(tmp[0] * (1.0f / 255.0f)) * tmp[3];
	color.y = GammaToLinear(tmp[1] * (1.0f / 255.0f)) * tmp[3];
	color.z = GammaToLinear(tmp[2] * (1.0f / 255.0f)) * tmp[3];
}

bool CEnvCascadeLightSecond::KeyValue(const char* szKeyName, const char* szValue)
{

	return BaseClass::KeyValue(szKeyName, szValue);

}

void CEnvCascadeLightSecond::Activate(void)
{
	if (GetSpawnFlags() & ENV_CASCADE_STARTON)
	{
		m_bState = true;
	}

	SetThink(&CEnvCascadeLightSecond::InitialThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	BaseClass::Activate();
}

void CEnvCascadeLightSecond::InitialThink(void)
{
	float bibigon = defdist.GetFloat() / curdist.GetFloat();
	m_flLightFOV = csm_second_fov.GetFloat() * bibigon;
	m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target);
}

int CEnvCascadeLightSecond::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}



//-----------------------------------------------------------------------------
// Purpose: main csm code
//-----------------------------------------------------------------------------
class CEnvCascadeLight : public CPointEntity
{
	DECLARE_CLASS(CEnvCascadeLight, CPointEntity);
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CEnvCascadeLight();
	bool KeyValue(const char* szKeyName, const char* szValue);

	// Always transmit to clients
	virtual int UpdateTransmitState();
	virtual void Activate(void);
	void Spawn();
	void Preparation();

	//Inputs
	void InputSetEnableShadows(inputdata_t& inputdata);
	void InputSetLightColor( inputdata_t &inputdata );
	void InputSetSpotlightTexture(inputdata_t& inputdata);
	void InputSetAmbient(inputdata_t& inputdata);
	void InputSetAngles(inputdata_t& inputdata);
	void InputAddAngles(inputdata_t& inputdata);
	void InputResetAngles(inputdata_t& inputdata);

	void InitialThink(void);



	CNetworkHandle(CBaseEntity, m_hTargetEntity);

private:
	CLightOrigin* csm_origin;
	CEnvCascadeLightSecond* SecondCSM;
	CEnvCascadeLightThird* ThirdCSM;
	CNetworkVar(bool, m_bState);
	CNetworkVar(float, m_flLightFOV);
	CNetworkVar(bool, m_bEnableAngleFromEnv);
	CNetworkVar(bool, m_bEnableShadows);
	CNetworkVar(bool, m_bLightOnlyTarget);
	CNetworkVar(bool, m_bLightWorld);
	CNetworkVar(bool, m_bCameraSpace);
	CNetworkVar(float, m_flAmbient);
	CNetworkString(m_SpotlightTextureName, MAX_PATH);
	CNetworkVar(int, m_nSpotlightTextureFrame);
	CNetworkVar(float, m_flNearZ);
	CNetworkVar(float, m_flFarZ);
	CNetworkVar(int, m_nShadowQuality);
	float m_flBrightnessScale;

	bool m_bEnableThird;

	QAngle DefaultAngle = QAngle(0, 0, 0);
	QAngle CurrentAngle = QAngle(0, 0, 0);
};

LINK_ENTITY_TO_CLASS(env_cascade_light, CEnvCascadeLight);

BEGIN_DATADESC(CEnvCascadeLight)
DEFINE_FIELD(m_hTargetEntity, FIELD_EHANDLE),
DEFINE_FIELD(m_bState, FIELD_BOOLEAN),
DEFINE_KEYFIELD(m_bEnableShadows, FIELD_BOOLEAN, "enableshadows"),
DEFINE_KEYFIELD(m_bEnableThird, FIELD_BOOLEAN, "enablethird"),
DEFINE_KEYFIELD(m_bLightOnlyTarget, FIELD_BOOLEAN, "lightonlytarget"),
DEFINE_KEYFIELD(m_bLightWorld, FIELD_BOOLEAN, "lightworld"),
DEFINE_KEYFIELD(m_bCameraSpace, FIELD_BOOLEAN, "cameraspace"),
DEFINE_KEYFIELD(m_flAmbient, FIELD_FLOAT, "ambient"),
DEFINE_AUTO_ARRAY_KEYFIELD(m_SpotlightTextureName, FIELD_CHARACTER, "texturename"),
DEFINE_KEYFIELD(m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe"),
DEFINE_KEYFIELD(m_flNearZ, FIELD_FLOAT, "nearz"),
DEFINE_KEYFIELD(m_flFarZ, FIELD_FLOAT, "farz"),
DEFINE_KEYFIELD(m_nShadowQuality, FIELD_INTEGER, "shadowquality"),
DEFINE_KEYFIELD(m_bEnableAngleFromEnv, FIELD_BOOLEAN, "uselightenvangles"),
DEFINE_KEYFIELD(m_flBrightnessScale, FIELD_FLOAT, "brightnessscale"),


//Inputs
DEFINE_INPUTFUNC(FIELD_BOOLEAN, "EnableShadows", InputSetEnableShadows),
DEFINE_INPUTFUNC(FIELD_COLOR32, "LightColor", InputSetLightColor),
DEFINE_INPUTFUNC(FIELD_FLOAT, "Ambient", InputSetAmbient),
DEFINE_INPUTFUNC(FIELD_STRING, "Texture", InputSetSpotlightTexture),
DEFINE_INPUTFUNC(FIELD_STRING, "SetAngles", InputSetAngles),
DEFINE_INPUTFUNC(FIELD_STRING, "AddAngles", InputAddAngles),
DEFINE_INPUTFUNC(FIELD_VOID, "ResetAngles", InputResetAngles),

DEFINE_THINKFUNC(InitialThink),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CEnvCascadeLight, DT_EnvCascadeLight)

	SendPropEHandle(SENDINFO(m_hTargetEntity)),
	SendPropBool(SENDINFO(m_bState)),
	SendPropFloat(SENDINFO(m_flLightFOV)),
	SendPropBool(SENDINFO(m_bEnableShadows)),
	SendPropBool(SENDINFO(m_bLightOnlyTarget)),
	SendPropBool(SENDINFO(m_bLightWorld)),
	SendPropBool(SENDINFO(m_bCameraSpace)),
	SendPropFloat(SENDINFO(m_flAmbient)),
	SendPropString(SENDINFO(m_SpotlightTextureName)),
	SendPropInt(SENDINFO(m_nSpotlightTextureFrame)),
	SendPropFloat(SENDINFO(m_flNearZ), 16, SPROP_ROUNDDOWN, 0.0f, 500.0f),
	SendPropFloat(SENDINFO(m_flFarZ), 18, SPROP_ROUNDDOWN, 0.0f, 1500.0f),
	SendPropInt(SENDINFO(m_nShadowQuality), 1, SPROP_UNSIGNED)  // Just one bit for now
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEnvCascadeLight::CEnvCascadeLight(void)
{
	m_bState = true;
	m_flLightFOV = 45.0f;
	m_bEnableShadows = true;
	m_bLightOnlyTarget = false;
	m_bLightWorld = true;
	m_bCameraSpace = false;
	m_bEnableAngleFromEnv = false;
	m_flBrightnessScale = 1.0f;

	Q_strcpy(m_SpotlightTextureName.GetForModify(), "tools\\fakecsm\\mask_center");
	m_nSpotlightTextureFrame = 0;
	m_flAmbient = 0.0f;
	m_flNearZ = csm_nearz.GetFloat();
	m_flFarZ = csm_farz.GetFloat();
	m_nShadowQuality = 0;

	m_bEnableThird = true;
	
}

void CEnvCascadeLight::Preparation()
{
	CreateEntityByName("csm_origin");
	CreateEntityByName("csm_second");
	defFOV.SetValue(m_flLightFOV);
	CBaseEntity* CSMOrigin = NULL;
	CBaseEntity* CSMSecond = NULL;

	CSMOrigin = gEntList.FindEntityByClassname(CSMOrigin, "csm_origin");
	CSMSecond = gEntList.FindEntityByClassname(CSMSecond, "csm_second");

	//if origin is exist
	if (CSMOrigin)
	{

		csm_origin = dynamic_cast<CLightOrigin*>(CSMOrigin);

		//if second csm is exist
		if (CSMSecond)
		{
			SecondCSM = dynamic_cast<CEnvCascadeLightSecond*>(CSMSecond);
			SecondCSM->SetAbsAngles(GetAbsAngles());
			SecondCSM->SetAbsOrigin(GetAbsOrigin());
			SecondCSM->SetParent(GetBaseEntity());

			DispatchSpawn(SecondCSM);
		}

		//if third csm enabled
		if (m_bEnableThird)
		{
			CreateEntityByName("csm_third");
			CBaseEntity* CSMThird = NULL;

			CSMThird = gEntList.FindEntityByClassname(CSMThird, "csm_third");

			//if third csm exist
			if (CSMThird)
			{
				ThirdCSM = dynamic_cast<CEnvCascadeLightThird*>(CSMThird);
				ThirdCSM->SetAbsAngles(GetAbsAngles());
				ThirdCSM->SetAbsOrigin(GetAbsOrigin());
				ThirdCSM->SetParent(GetBaseEntity());

				DispatchSpawn(ThirdCSM);
			}
		}


		SetParent(csm_origin, 1);

		SetAbsOrigin(Vector(csm_origin->GetAbsOrigin().x, csm_origin->GetAbsOrigin().y, csm_origin->GetAbsOrigin().z + curdist.GetInt()));

		if (m_bEnableAngleFromEnv)
			csm_origin->angFEnv = true;
		else
			csm_origin->SetAbsAngles(QAngle((GetLocalAngles().x - 90), GetLocalAngles().y, -GetLocalAngles().z));


		SetLocalAngles(QAngle(90, 0, 0));

		DefaultAngle = csm_origin->GetAbsAngles();
		CurrentAngle = DefaultAngle;


		DispatchSpawn(CSMOrigin);
	}
	else
	{
		DevMsg("[FAKE CSM]		Main csm entity can't find \"csmorigin\" entity!");
	}

	ConVarRef("csm_brightness_general").SetValue(m_flBrightnessScale);

}

void CEnvCascadeLight::Spawn()
{
	
	Preparation();

}

void UTIL_ColorStringToLinearFloatColorCSMFake(Vector& color, const char* pString)
{
	float tmp[4];
	UTIL_StringToFloatArray(tmp, 4, pString);
	if (tmp[3] <= 0.0f)
	{	
		tmp[3] = 255.0f;
	}
	tmp[3] *= (1.0f / 255.0f);
	color.x = GammaToLinear(tmp[0] * (1.0f / 255.0f)) * tmp[3];
	color.y = GammaToLinear(tmp[1] * (1.0f / 255.0f)) * tmp[3];
	color.z = GammaToLinear(tmp[2] * (1.0f / 255.0f)) * tmp[3];
}

bool CEnvCascadeLight::KeyValue(const char* szKeyName, const char* szValue)
{

	if (FStrEq(szKeyName, "lightcolor") || FStrEq(szKeyName, "color"))
	{
		
		float tmp[4];
		UTIL_StringToFloatArray(tmp, 4, szValue);

		ConVarRef("csm_color_r").SetValue(tmp[0]);
		ConVarRef("csm_color_g").SetValue(tmp[1]);
		ConVarRef("csm_color_b").SetValue(tmp[2]);
		ConVarRef("csm_color_a").SetValue(tmp[3]);

	}
	else
	{
		return BaseClass::KeyValue(szKeyName, szValue);
	}

	return true;
}


void CEnvCascadeLight::InputSetEnableShadows(inputdata_t& inputdata)
{
	m_bEnableShadows = inputdata.value.Bool();
}


void CEnvCascadeLight::InputSetAmbient(inputdata_t& inputdata)
{
	m_flAmbient = inputdata.value.Float();
}

void CEnvCascadeLight::InputSetSpotlightTexture(inputdata_t& inputdata)
{
	Q_strcpy(m_SpotlightTextureName.GetForModify(), inputdata.value.String());
}

void CEnvCascadeLight::Activate(void)
{
	SetThink(&CEnvCascadeLight::InitialThink);
	SetNextThink(gpGlobals->curtime + 0.1f);

	BaseClass::Activate();
}

void CEnvCascadeLight::InitialThink(void)
{
	m_hTargetEntity = gEntList.FindEntityByName(NULL, m_target);
	float bibigon = defdist.GetFloat() / curdist.GetFloat();
	curFOV.SetValue(defFOV.GetFloat() * bibigon);
	m_flLightFOV = curFOV.GetFloat();
}


void CEnvCascadeLight::InputSetAngles(inputdata_t& inputdata)
{
	const char* pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector(angles.Base(), pAngles); 
	
	CurrentAngle = angles;
	csm_origin->SetAbsAngles(CurrentAngle);

}

void CEnvCascadeLight::InputAddAngles(inputdata_t& inputdata)
{
	const char* pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector(angles.Base(), pAngles);

	CurrentAngle = CurrentAngle + angles;
	csm_origin->SetAbsAngles(CurrentAngle);
}

void CEnvCascadeLight::InputResetAngles(inputdata_t& inputdata)
{
	CurrentAngle = DefaultAngle;
	csm_origin->SetAbsAngles(CurrentAngle);
}

void CEnvCascadeLight::InputSetLightColor(inputdata_t& inputdata)
{

	color32 color = inputdata.value.Color32();

	ConVarRef("csm_color_r").SetValue(color.r);
	ConVarRef("csm_color_g").SetValue(color.g);
	ConVarRef("csm_color_b").SetValue(color.b);
	ConVarRef("csm_color_a").SetValue(color.a);
}

int CEnvCascadeLight::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}
