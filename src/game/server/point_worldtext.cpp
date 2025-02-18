//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "point_worldtext.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(point_worldtext, CPointWorldText);

BEGIN_DATADESC( CPointWorldText )
	DEFINE_FIELD( m_szText, FIELD_STRING ),
	DEFINE_FIELD( m_colTextColor, FIELD_COLOR32 ), 
	DEFINE_KEYFIELD( m_flTextSize,	FIELD_FLOAT, "textsize" ),
	DEFINE_KEYFIELD( m_flTextSpacingX,	FIELD_FLOAT, "textspacingX" ),
	DEFINE_KEYFIELD( m_flTextSpacingY,	FIELD_FLOAT, "textspacingY" ),
	DEFINE_KEYFIELD( m_nOrientation, FIELD_INTEGER, "orientation" ), 
	DEFINE_KEYFIELD( m_nFont, FIELD_INTEGER, "font" ), 
	DEFINE_KEYFIELD( m_bRainbow, FIELD_BOOLEAN, "rainbow" ), 

	DEFINE_INPUTFUNC( FIELD_STRING, "SetText", InputSetText ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTextSize", InputSetTextSize ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTextSpacingX", InputSetTextSpacingX ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTextSpacingY", InputSetTextSpacingY ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetColor", InputSetColor ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetOrientation", InputSetOrientation ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetFont", InputSetFont ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetRainbow", InputSetRainbow ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointWorldText, DT_PointWorldText )
	SendPropString( SENDINFO( m_szText ) ),
	SendPropInt( SENDINFO( m_colTextColor ), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flTextSize ) ),
	SendPropFloat( SENDINFO( m_flTextSpacingX ) ),
	SendPropFloat( SENDINFO( m_flTextSpacingY ) ),
	SendPropInt( SENDINFO( m_nOrientation ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nFont ), 16, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bRainbow ) ),
END_SEND_TABLE()

CPointWorldText::CPointWorldText() : 
	CBaseEntity()
{
	V_memset( m_szText.GetForModify(), 0, sizeof( m_szText ) );
	color32 tmp = { 255, 255, 255, 255 };
	m_colTextColor = tmp;
	m_flTextSize = 10.0f;
	m_flTextSpacingX = 0.0f;
	m_flTextSpacingY = 0.0f;
	m_nOrientation = 0;
	m_nFont = 0;
	m_bRainbow = false;
}

CPointWorldText::~CPointWorldText()
{
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CPointWorldText::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

bool CPointWorldText::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "message" ) )
	{
		V_strncpy( m_szText.GetForModify(), szValue, sizeof(m_szText) );
		return true;
	}
	if ( FStrEq( szKeyName, "color" ) )
	{
		int r = 255, g = 255, b = 255, a = 255;
		int nCount = sscanf( szValue, "%d %d %d %d", &r, &g, &b, &a );
		color32 color = { (byte)r, (byte)g, (byte)b, (byte)a };
		byte* pColor = &color.r;
		for (int i = nCount; i < 4; i++)
			pColor[i] = 255;
		m_colTextColor = color;
		return true;
	}
	return BaseClass::KeyValue( szKeyName, szValue );
}

void CPointWorldText::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
	BaseClass::Spawn();
}
