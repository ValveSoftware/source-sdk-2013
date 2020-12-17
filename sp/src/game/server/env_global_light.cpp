//========= Copyright 1996-2010, Valve Corporation, All rights reserved. ============//
//
// Purpose: global dynamic light. Ported from Insolence's port of Alien Swarm's env_global_light.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Sunlight shadow control entity
//------------------------------------------------------------------------------
class CGlobalLight : public CBaseEntity
{
public:
	DECLARE_CLASS( CGlobalLight, CBaseEntity );

	CGlobalLight();

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );
	int  UpdateTransmitState();

	// Inputs
	void	InputSetAngles( inputdata_t &inputdata );
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputSetTexture( inputdata_t &inputdata );
	void	InputSetEnableShadows( inputdata_t &inputdata );
	void	InputSetLightColor( inputdata_t &inputdata );
#ifdef MAPBASE
	void	InputSetBrightness( inputdata_t &inputdata );
	void	InputSetColorTransitionTime( inputdata_t &inputdata );
	void	InputSetXOffset( inputdata_t &inputdata ) { m_flEastOffset = inputdata.value.Float(); }
	void	InputSetYOffset( inputdata_t &inputdata ) { m_flForwardOffset = inputdata.value.Float(); }
	void	InputSetOrthoSize( inputdata_t &inputdata ) { m_flOrthoSize = inputdata.value.Float(); }
	void	InputSetDistance( inputdata_t &inputdata ) { m_flSunDistance = inputdata.value.Float(); }
	void	InputSetFOV( inputdata_t &inputdata ) { m_flFOV = inputdata.value.Float(); }
	void	InputSetNearZDistance( inputdata_t &inputdata ) { m_flNearZ = inputdata.value.Float(); }
	void	InputSetNorthOffset( inputdata_t &inputdata ) { m_flNorthOffset = inputdata.value.Float(); }
#endif

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	CNetworkVector( m_shadowDirection );

	CNetworkVar( bool, m_bEnabled );

	CNetworkString( m_TextureName, MAX_PATH );
#ifdef MAPBASE
	CNetworkVar( int, m_nSpotlightTextureFrame );
#endif
	CNetworkColor32( m_LightColor );
#ifdef MAPBASE
	CNetworkVar( float, m_flBrightnessScale );
#endif
	CNetworkVar( float, m_flColorTransitionTime );
	CNetworkVar( float, m_flSunDistance );
	CNetworkVar( float, m_flFOV );
	CNetworkVar( float, m_flNearZ );
	CNetworkVar( float, m_flNorthOffset );
#ifdef MAPBASE
	CNetworkVar( float, m_flEastOffset ); // xoffset
	CNetworkVar( float, m_flForwardOffset ); // yoffset
	CNetworkVar( float, m_flOrthoSize );
#endif
	CNetworkVar( bool, m_bEnableShadows );
};

LINK_ENTITY_TO_CLASS(env_global_light, CGlobalLight);

BEGIN_DATADESC( CGlobalLight )

	DEFINE_KEYFIELD( m_bEnabled,		FIELD_BOOLEAN, "enabled" ),
	DEFINE_AUTO_ARRAY_KEYFIELD( m_TextureName, FIELD_CHARACTER, "texturename" ),
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_nSpotlightTextureFrame, FIELD_INTEGER, "textureframe" ),
#endif
	DEFINE_KEYFIELD( m_flSunDistance,	FIELD_FLOAT, "distance" ),
	DEFINE_KEYFIELD( m_flFOV,	FIELD_FLOAT, "fov" ),
	DEFINE_KEYFIELD( m_flNearZ,	FIELD_FLOAT, "nearz" ),
	DEFINE_KEYFIELD( m_flNorthOffset,	FIELD_FLOAT, "northoffset" ),
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_flEastOffset,	FIELD_FLOAT, "eastoffset" ),
	DEFINE_KEYFIELD( m_flForwardOffset,	FIELD_FLOAT, "forwardoffset" ),
	DEFINE_KEYFIELD( m_flOrthoSize,	FIELD_FLOAT, "orthosize" ),
#endif
	DEFINE_KEYFIELD( m_bEnableShadows, FIELD_BOOLEAN, "enableshadows" ),
	DEFINE_FIELD( m_LightColor, FIELD_COLOR32 ), 
#ifdef MAPBASE
	DEFINE_KEYFIELD( m_flBrightnessScale, FIELD_FLOAT, "brightnessscale" ),
#endif
	DEFINE_KEYFIELD( m_flColorTransitionTime, FIELD_FLOAT, "colortransitiontime" ),

	DEFINE_FIELD( m_shadowDirection, FIELD_VECTOR ),

	// Inputs
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetXOffset", InputSetXOffset ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetYOffset", InputSetYOffset ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetOrthoSize", InputSetOrthoSize ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDistance", InputSetDistance ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFOV", InputSetFOV ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetNearZDistance", InputSetNearZDistance ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetNorthOffset", InputSetNorthOffset ),
#else
	DEFINE_INPUT( m_flSunDistance,		FIELD_FLOAT, "SetDistance" ),
	DEFINE_INPUT( m_flFOV,				FIELD_FLOAT, "SetFOV" ),
	DEFINE_INPUT( m_flNearZ,			FIELD_FLOAT, "SetNearZDistance" ),
	DEFINE_INPUT( m_flNorthOffset,			FIELD_FLOAT, "SetNorthOffset" ),
#endif

	DEFINE_INPUTFUNC( FIELD_COLOR32, "LightColor", InputSetLightColor ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetAngles", InputSetAngles ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTexture", InputSetTexture ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "EnableShadows", InputSetEnableShadows ),
#ifdef MAPBASE
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetBrightness", InputSetBrightness ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetColorTransitionTime", InputSetColorTransitionTime ),
#endif

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST_NOBASE(CGlobalLight, DT_GlobalLight)
	SendPropVector(SENDINFO(m_shadowDirection), -1,  SPROP_NOSCALE ),
	SendPropBool(SENDINFO(m_bEnabled) ),
	SendPropString(SENDINFO(m_TextureName)),
#ifdef MAPBASE
	SendPropInt(SENDINFO(m_nSpotlightTextureFrame)),
#endif
	/*SendPropInt(SENDINFO (m_LightColor ),	32, SPROP_UNSIGNED, SendProxy_Color32ToInt32 ),*/
	SendPropInt(SENDINFO (m_LightColor ),	32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
#ifdef MAPBASE
	SendPropFloat( SENDINFO( m_flBrightnessScale ) ),
#endif
	SendPropFloat( SENDINFO( m_flColorTransitionTime ) ),
	SendPropFloat(SENDINFO(m_flSunDistance), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flFOV), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flNearZ), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flNorthOffset), 0, SPROP_NOSCALE ),
#ifdef MAPBASE
	SendPropFloat(SENDINFO(m_flEastOffset), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flForwardOffset), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flOrthoSize), 0, SPROP_NOSCALE ),
#endif
	SendPropBool( SENDINFO( m_bEnableShadows ) ),
END_SEND_TABLE()


CGlobalLight::CGlobalLight()
{
#if defined( _X360 )
	Q_strcpy( m_TextureName.GetForModify(), "effects/flashlight_border" );
#else
	Q_strcpy( m_TextureName.GetForModify(), "effects/flashlight001" );
#endif
#ifdef MAPBASE
	m_LightColor.Init( 255, 255, 255, 255 );
#else
	m_LightColor.Init( 255, 255, 255, 1 );
#endif
	m_flColorTransitionTime = 0.5f;
	m_flSunDistance = 10000.0f;
	m_flFOV = 5.0f;
	m_bEnableShadows = false;
#ifdef MAPBASE
	m_nSpotlightTextureFrame = 0;
	m_flBrightnessScale = 1.0f;
	m_flOrthoSize = 1000.0f;
#endif
	m_bEnabled = true;
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CGlobalLight::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


bool CGlobalLight::KeyValue( const char *szKeyName, const char *szValue )
{
#ifdef MAPBASE
	if ( FStrEq( szKeyName, "lightcolor" ) || FStrEq( szKeyName, "color" ) )
#else
	if ( FStrEq( szKeyName, "color" ) )
#endif
	{
		float tmp[4];
		UTIL_StringToFloatArray( tmp, 4, szValue );

		m_LightColor.SetR( tmp[0] );
		m_LightColor.SetG( tmp[1] );
		m_LightColor.SetB( tmp[2] );
		m_LightColor.SetA( tmp[3] );
	}
	else if ( FStrEq( szKeyName, "angles" ) )
	{
		QAngle angles;
		UTIL_StringToVector( angles.Base(), szValue );
		if (angles == vec3_angle)
		{
			angles.Init( 80, 30, 0 );
		}
		Vector vForward;
		AngleVectors( angles, &vForward );
		m_shadowDirection = vForward;
		return true;
	}
	else if ( FStrEq( szKeyName, "texturename" ) )
	{
#if defined( _X360 )
		if ( Q_strcmp( szValue, "effects/flashlight001" ) == 0 )
		{
			// Use this as the default for Xbox
			Q_strcpy( m_TextureName.GetForModify(), "effects/flashlight_border" );
		}
		else
		{
			Q_strcpy( m_TextureName.GetForModify(), szValue );
		}
#else
		Q_strcpy( m_TextureName.GetForModify(), szValue );
#endif
	}
	else if ( FStrEq( szKeyName, "StartDisabled" ) )
	{
		m_bEnabled.Set( atoi( szValue ) <= 0 );
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

bool CGlobalLight::GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen )
{
	if ( FStrEq( szKeyName, "color" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%d %d %d %d", m_LightColor.GetR(), m_LightColor.GetG(), m_LightColor.GetB(), m_LightColor.GetA() );
		return true;
	}
	else if ( FStrEq( szKeyName, "texturename" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%s", m_TextureName.Get() );
		return true;
	}
	else if ( FStrEq( szKeyName, "StartDisabled" ) )
	{
		Q_snprintf( szValue, iMaxLen, "%d", !m_bEnabled.Get() );
		return true;
	}
	return BaseClass::GetKeyValue( szKeyName, szValue, iMaxLen );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CGlobalLight::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
}

//------------------------------------------------------------------------------
// Input values
//------------------------------------------------------------------------------
void CGlobalLight::InputSetAngles( inputdata_t &inputdata )
{
	const char *pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector( angles.Base(), pAngles );

	Vector vTemp;
	AngleVectors( angles, &vTemp );
	m_shadowDirection = vTemp;
}

//------------------------------------------------------------------------------
// Purpose : Input handlers
//------------------------------------------------------------------------------
void CGlobalLight::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
}

void CGlobalLight::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
}

void CGlobalLight::InputSetTexture( inputdata_t &inputdata )
{
	Q_strcpy( m_TextureName.GetForModify(), inputdata.value.String() );
}

void CGlobalLight::InputSetEnableShadows( inputdata_t &inputdata )
{
	m_bEnableShadows = inputdata.value.Bool();
}

void CGlobalLight::InputSetLightColor( inputdata_t &inputdata )
{
	m_LightColor = inputdata.value.Color32();
} 

#ifdef MAPBASE
void CGlobalLight::InputSetBrightness( inputdata_t &inputdata )
{
	m_flBrightnessScale = inputdata.value.Float();
}

void CGlobalLight::InputSetColorTransitionTime( inputdata_t &inputdata )
{
	m_flColorTransitionTime = inputdata.value.Float();
}
#endif
