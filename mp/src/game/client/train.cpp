//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Train.cpp
//
// implementation of CHudAmmo class
//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CHudTrain: public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudTrain, vgui::Panel );
public:
	CHudTrain( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void MsgFunc_Train(bf_read &msg);

private:
	int m_iPos;

};

//
//-----------------------------------------------------
//

DECLARE_HUDELEMENT( CHudTrain );
DECLARE_HUD_MESSAGE( CHudTrain, Train )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTrain::CHudTrain( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudTrain" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scheme - 
//-----------------------------------------------------------------------------
void CHudTrain::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrain::Init(void)
{
	HOOK_HUD_MESSAGE( CHudTrain, Train );

	m_iPos = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrain::VidInit(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudTrain::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && m_iPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrain::Paint()
{
	// FIXME:  Rewrite using vgui materials if we still do this type of train UI!!!
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTrain::MsgFunc_Train( bf_read &msg )
{
	// update Train data
	m_iPos = msg.ReadByte();
}
