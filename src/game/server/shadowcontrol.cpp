//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shadow control entity.
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
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class CShadowControl : public CBaseEntity
{
public:
	DECLARE_CLASS( CShadowControl, CBaseEntity );

	CShadowControl();

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	int  UpdateTransmitState();
	void InputSetAngles( inputdata_t &inputdata );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	CNetworkVector( m_shadowDirection );
	CNetworkColor32( m_shadowColor );
	CNetworkVar( float, m_flShadowMaxDist );
	CNetworkVar( bool, m_bDisableShadows );
};

LINK_ENTITY_TO_CLASS(shadow_control, CShadowControl);

BEGIN_DATADESC( CShadowControl )

	DEFINE_KEYFIELD( m_flShadowMaxDist, FIELD_FLOAT, "distance" ),
	DEFINE_KEYFIELD( m_bDisableShadows, FIELD_BOOLEAN, "disableallshadows" ),

	// Inputs
	DEFINE_INPUT( m_shadowColor,		FIELD_COLOR32, "color" ),
	DEFINE_INPUT( m_shadowDirection,	FIELD_VECTOR, "direction" ),
	DEFINE_INPUT( m_flShadowMaxDist,	FIELD_FLOAT, "SetDistance" ),
	DEFINE_INPUT( m_bDisableShadows,	FIELD_BOOLEAN, "SetShadowsDisabled" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetAngles", InputSetAngles ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST_NOBASE(CShadowControl, DT_ShadowControl)
	SendPropVector(SENDINFO(m_shadowDirection), -1,  SPROP_NOSCALE ),
	SendPropInt(SENDINFO(m_shadowColor),	32, SPROP_UNSIGNED),
	SendPropFloat(SENDINFO(m_flShadowMaxDist), 0, SPROP_NOSCALE ),
	SendPropBool(SENDINFO(m_bDisableShadows)),
END_SEND_TABLE()


CShadowControl::CShadowControl()
{
	m_shadowDirection.Init( 0.2, 0.2, -2 );
	m_flShadowMaxDist = 50.0f;
	m_shadowColor.Init( 64, 64, 64, 0 );
	m_bDisableShadows = false;
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CShadowControl::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


bool CShadowControl::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "color" ) )
	{
		color32 tmp;
		UTIL_StringToColor32( &tmp, szValue );
		m_shadowColor = tmp;
		return true;
	}

	if ( FStrEq( szKeyName, "angles" ) )
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

	// For backward compatibility...
	if ( FStrEq( szKeyName, "direction" ) )
	{
		// Only use this if angles haven't been set...
		if ( fabs(m_shadowDirection->LengthSqr() - 1.0f) > 1e-3 )
		{
			Vector vTemp;
			UTIL_StringToVector( vTemp.Base(), szValue );
			m_shadowDirection = vTemp;
		}
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CShadowControl::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
}

//------------------------------------------------------------------------------
// Input values
//------------------------------------------------------------------------------
void CShadowControl::InputSetAngles( inputdata_t &inputdata )
{
	const char *pAngles = inputdata.value.String();

	QAngle angles;
	UTIL_StringToVector( angles.Base(), pAngles );

	Vector vTemp;
	AngleVectors( angles, &vTemp );
	m_shadowDirection = vTemp;
}
