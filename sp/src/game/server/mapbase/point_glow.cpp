//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase off-shoot of tf_glow (created using SDK code only)
//
//=============================================================================

#include "cbase.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointGlow : public CPointEntity
{
	DECLARE_CLASS( CPointGlow, CPointEntity );
public:

	int UpdateTransmitState( void ) { return SetTransmitState( FL_EDICT_ALWAYS ); }
	
	void Spawn( void );

	void SetGlowTarget( CBaseEntity *pActivator, CBaseEntity *pCaller ) { m_hGlowTarget = gEntList.FindEntityByName(NULL, m_target, this, pActivator, pCaller); }

	// Inputs
	void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); SetGlowTarget( inputdata.pActivator, inputdata.pCaller ); }

	void InputEnable( inputdata_t &inputdata ) { m_bGlowDisabled = false; SetGlowTarget( inputdata.pActivator, inputdata.pCaller ); }
	void InputDisable( inputdata_t &inputdata ) { m_bGlowDisabled = true; }
	void InputToggle( inputdata_t &inputdata ) { m_bGlowDisabled ? InputEnable(inputdata) : InputDisable(inputdata); }

	void InputSetGlowColor( inputdata_t &inputdata ) { m_GlowColor = inputdata.value.Color32(); }

	CNetworkHandle( CBaseEntity, m_hGlowTarget );
	CNetworkColor32( m_GlowColor );
	CNetworkVar( bool, m_bGlowDisabled );

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};

LINK_ENTITY_TO_CLASS( point_glow, CPointGlow );


BEGIN_DATADESC( CPointGlow )

	// Keys
	DEFINE_KEYFIELD( m_GlowColor, FIELD_COLOR32, "GlowColor" ),
	DEFINE_FIELD( m_hGlowTarget, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_bGlowDisabled, FIELD_BOOLEAN, "StartDisabled" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "SetGlowColor", InputSetGlowColor ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointGlow, DT_PointGlow )
	SendPropEHandle( SENDINFO( m_hGlowTarget ) ),
	SendPropInt( SENDINFO( m_GlowColor ), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ),
	SendPropBool( SENDINFO( m_bGlowDisabled ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointGlow::Spawn()
{
	m_hGlowTarget = gEntList.FindEntityByName( NULL, m_target, this );

	BaseClass::Spawn();
}
