//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Depth of field controller entity
//
//=============================================================================

#include "cbase.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


extern bool  g_bDOFEnabled;
extern float g_flDOFNearBlurDepth;
extern float g_flDOFNearFocusDepth;
extern float g_flDOFFarFocusDepth;
extern float g_flDOFFarBlurDepth;
extern float g_flDOFNearBlurRadius;
extern float g_flDOFFarBlurRadius;

EHANDLE g_hDOFControllerInUse = NULL;

class C_EnvDOFController : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvDOFController, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_EnvDOFController();
	~C_EnvDOFController();
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:
	bool  m_bDOFEnabled;
	float m_flNearBlurDepth;
	float m_flNearFocusDepth;
	float m_flFarFocusDepth;
	float m_flFarBlurDepth;
	float m_flNearBlurRadius;
	float m_flFarBlurRadius;

private:
	C_EnvDOFController( const C_EnvDOFController & );
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvDOFController, DT_EnvDOFController, CEnvDOFController )
	RecvPropInt( RECVINFO(m_bDOFEnabled) ),
	RecvPropFloat( RECVINFO(m_flNearBlurDepth) ),
	RecvPropFloat( RECVINFO(m_flNearFocusDepth) ),
	RecvPropFloat( RECVINFO(m_flFarFocusDepth) ),
	RecvPropFloat( RECVINFO(m_flFarBlurDepth) ),
	RecvPropFloat( RECVINFO(m_flNearBlurRadius) ),
	RecvPropFloat( RECVINFO(m_flFarBlurRadius) )
END_RECV_TABLE()

C_EnvDOFController::C_EnvDOFController()
:	m_bDOFEnabled( true ),
	m_flNearBlurDepth( 20.0f ),
	m_flNearFocusDepth( 100.0f ),
	m_flFarFocusDepth( 250.0f ),
	m_flFarBlurDepth( 1000.0f ),
	m_flNearBlurRadius( 0.0f ),		// no near blur by default
	m_flFarBlurRadius( 5.0f )
{
}

C_EnvDOFController::~C_EnvDOFController()
{
	if ( g_hDOFControllerInUse == this )
	{
		g_bDOFEnabled = false;
	}
}

void C_EnvDOFController::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	g_bDOFEnabled = m_bDOFEnabled && ( ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f ) );
	g_flDOFNearBlurDepth	= m_flNearBlurDepth;
	g_flDOFNearFocusDepth	= m_flNearFocusDepth;
	g_flDOFFarFocusDepth	= m_flFarFocusDepth;
	g_flDOFFarBlurDepth		= m_flFarBlurDepth;
	g_flDOFNearBlurRadius	= m_flNearBlurRadius;
	g_flDOFFarBlurRadius	= m_flFarBlurRadius;

	g_hDOFControllerInUse = this;
}
