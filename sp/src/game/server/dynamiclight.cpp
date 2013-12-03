//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Dynamic light.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "dlight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define NUM_DL_EXPONENT_BITS	8
#define MIN_DL_EXPONENT_VALUE	-((1 << (NUM_DL_EXPONENT_BITS-1)) - 1)
#define MAX_DL_EXPONENT_VALUE	((1 << (NUM_DL_EXPONENT_BITS-1)) - 1)


class CDynamicLight : public CBaseEntity
{
public:
	DECLARE_CLASS( CDynamicLight, CBaseEntity );

	void Spawn( void );
	void DynamicLightThink( void );
	bool KeyValue( const char *szKeyName, const char *szValue );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	// Turn on and off the light
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

public:
	unsigned char m_ActualFlags;
	CNetworkVar( unsigned char, m_Flags );
	CNetworkVar( unsigned char, m_LightStyle );
	bool	m_On;
	CNetworkVar( float, m_Radius );
	CNetworkVar( int, m_Exponent );
	CNetworkVar( float, m_InnerAngle );
	CNetworkVar( float, m_OuterAngle );
	CNetworkVar( float, m_SpotRadius );
};

LINK_ENTITY_TO_CLASS(light_dynamic, CDynamicLight);

BEGIN_DATADESC( CDynamicLight )

	DEFINE_FIELD( m_ActualFlags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_On, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( DynamicLightThink ),

	// Inputs
	DEFINE_INPUT( m_Radius,		FIELD_FLOAT,	"distance" ),
	DEFINE_INPUT( m_Exponent,	FIELD_INTEGER,	"brightness" ),
	DEFINE_INPUT( m_InnerAngle,	FIELD_FLOAT,	"_inner_cone" ),
	DEFINE_INPUT( m_OuterAngle,	FIELD_FLOAT,	"_cone" ),
	DEFINE_INPUT( m_SpotRadius,	FIELD_FLOAT,	"spotlight_radius" ),
	DEFINE_INPUT( m_LightStyle,	FIELD_CHARACTER,"style" ),
	
	// Input functions
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CDynamicLight, DT_DynamicLight)
	SendPropInt( SENDINFO(m_Flags), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_LightStyle), 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_Radius), 0, SPROP_NOSCALE),
	SendPropInt( SENDINFO(m_Exponent), NUM_DL_EXPONENT_BITS),
	SendPropFloat( SENDINFO(m_InnerAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_OuterAngle), 8, 0, 0.0, 360.0f ),
	SendPropFloat( SENDINFO(m_SpotRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDynamicLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "_light" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		SetRenderColor( tmp.r, tmp.g, tmp.b );
	}
	else if ( FStrEq( szKeyName, "pitch" ) )
	{
		float angle = atof(szValue);
		if ( angle )
		{
			QAngle angles = GetAbsAngles();
			angles[PITCH] = -angle;
			SetAbsAngles( angles );
		}
	}
	else if ( FStrEq( szKeyName, "spawnflags" ) )
	{
		m_ActualFlags = m_Flags = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//------------------------------------------------------------------------------
// Turn on and off the light
//------------------------------------------------------------------------------
void CDynamicLight::InputTurnOn( inputdata_t &inputdata )
{
	m_Flags = m_ActualFlags;
	m_On = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CDynamicLight::InputTurnOff( inputdata_t &inputdata )
{
	// This basically shuts it off
	m_Flags = DLIGHT_NO_MODEL_ILLUMINATION | DLIGHT_NO_WORLD_ILLUMINATION;
	m_On = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CDynamicLight::InputToggle( inputdata_t &inputdata )
{
	if (m_On)
	{
		InputTurnOff( inputdata );
	}
	else
	{
		InputTurnOn( inputdata );
	}
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CDynamicLight::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	m_On = true;
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	// If we have a target, think so we can orient towards it
	if ( m_target != NULL_STRING )
	{
		SetThink( &CDynamicLight::DynamicLightThink );
		SetNextThink( gpGlobals->curtime + 0.1 );
	}
	
	int clampedExponent = clamp( (int) m_Exponent, MIN_DL_EXPONENT_VALUE, MAX_DL_EXPONENT_VALUE );
	if ( m_Exponent != clampedExponent )
	{
		Warning( "light_dynamic at [%d %d %d] has invalid exponent value (%d must be between %d and %d).\n",
			(int)GetAbsOrigin().x, (int)GetAbsOrigin().x, (int)GetAbsOrigin().x, 
			m_Exponent.Get(),
			MIN_DL_EXPONENT_VALUE,
			MAX_DL_EXPONENT_VALUE );
		
		m_Exponent = clampedExponent;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicLight::DynamicLightThink( void )
{
	if ( m_target == NULL_STRING )
		return;

	CBaseEntity *pEntity = GetNextTarget();
	if ( pEntity )
	{
		Vector vecToTarget = (pEntity->GetAbsOrigin() - GetAbsOrigin());
		QAngle vecAngles;
		VectorAngles( vecToTarget, vecAngles );
		SetAbsAngles( vecAngles );
	}
	
	SetNextThink( gpGlobals->curtime + 0.1 );
}
