//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include "hudelement.h"
//#include "view.h"
//#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudVguiScreenCursor : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudVguiScreenCursor, vgui::Panel );
public:
	CHudVguiScreenCursor( const char *pElementName );

	virtual bool	ShouldDraw();

protected:
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();

private:
	// Cursor sprite and color
	CHudTexture		*m_pCursor;
	Color			m_clrCrosshair;
};

DECLARE_HUDELEMENT( CHudVguiScreenCursor );

CHudVguiScreenCursor::CHudVguiScreenCursor( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "VguiScreenCursor" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pCursor = 0;

	m_clrCrosshair = Color( 0, 0, 0, 0 );

	SetHiddenBits( HIDEHUD_PLAYERDEAD );
}

void CHudVguiScreenCursor::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_clrCrosshair = scheme->GetColor( "VguiScreenCursor", Color( 255, 255, 255, 255 ) );
	m_pCursor = gHUD.GetIcon( "arrow" );

	SetPaintBackgroundEnabled( false );

	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudVguiScreenCursor::ShouldDraw( void )
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsInViewModelVGuiInputMode() )
	{
		return false;
	}

	if ( !pPlayer->IsInVGuiInputMode() )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}


void CHudVguiScreenCursor::Paint( void )
{
	if ( !m_pCursor )
		return;

	float x, y;
	x = ScreenWidth()/2;
	y = ScreenHeight()/2;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsInViewModelVGuiInputMode() )
	{
		int iX, iY;
		vgui::input()->GetCursorPos(iX, iY);
		x = (float)iX;
		y = (float)iY;
	}

	m_pCursor->DrawSelf( 
		x - 0.5f * m_pCursor->Width(), 
		y - 0.5f * m_pCursor->Height(),
		m_clrCrosshair );
}