//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
class CTFGlow : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFGlow, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn() OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE;
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetGlowColor( inputdata_t &inputdata );

private:
	CNetworkVar( int, m_iMode );
	CNetworkVar( color32, m_glowColor );
	CNetworkVar( bool, m_bDisabled );
	CNetworkHandle( CBaseEntity, m_hTarget );
};

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTFGlow )
	DEFINE_KEYFIELD( m_iMode, FIELD_INTEGER, "Mode" ),
	DEFINE_KEYFIELD( m_glowColor, FIELD_COLOR32, "GlowColor" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),
END_DATADESC()

//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CTFGlow, DT_TFGlow )
	SendPropInt( SENDINFO( m_glowColor ), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropEHandle( SENDINFO( m_hTarget ) ),
	SendPropInt( SENDINFO( m_iMode ), -1, SPROP_UNSIGNED ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_glow, CTFGlow );

//-----------------------------------------------------------------------------
void CTFGlow::Spawn()
{
	CBaseEntity *pEnt = gEntList.FindEntityByName( nullptr, m_target );
	if ( !pEnt ) 
	{
		Warning( "tf_glow: failed to find target %s\n", m_target.ToCStr() );
		UTIL_Remove( this );
		return;
	}

	m_hTarget = pEnt;

	if ( gEntList.FindEntityByName( pEnt, m_target ) )
	{
		Warning( "tf_glow: only one target is supported (%s)\n", m_target.ToCStr() );
	}
}

//-----------------------------------------------------------------------------
void CTFGlow::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false; // clients will take action
}

//-----------------------------------------------------------------------------
void CTFGlow::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true; // clients will take action
}

//-----------------------------------------------------------------------------
void CTFGlow::InputSetGlowColor( inputdata_t &inputdata )
{
	m_glowColor = inputdata.value.Color32(); // clients will take action
}

//-----------------------------------------------------------------------------
int CTFGlow::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
