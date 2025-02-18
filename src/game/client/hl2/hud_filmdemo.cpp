//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include "cbase.h"
#include "c_baseentity.h"
#include "hud.h"
#include "hudelement.h"
#include "clientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudFilmDemo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudFilmDemo, vgui::Panel );
public:
	CHudFilmDemo( const char *name );

	// vgui overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme );
	virtual void Paint( void );
	virtual bool ShouldDraw( void );

	void SetFilmDemoActive( bool bActive );

	void SetLeftStringID( const char *id );
	void SetRightStringID( const char *id );

private:
	bool	m_bFilmDemoActive;

	char	m_pLeftStringID[ 256 ];
	char	m_pRightStringID[ 256 ];

	// Painting
	//CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "0" );
	CPanelAnimationVar( Color, m_BorderColor, "BorderColor", "0 0 0 255" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "255 255 255 255" );
	CPanelAnimationVarAliasType( int, m_iBorderLeft, "BorderLeft", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderRight, "BorderRight", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderTop, "BorderTop", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderBottom, "BorderBottom", "48", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderCenter, "BorderCenter", "8", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iLeftY, "LeftTitleY", "440", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRightY, "RightTitleY", "440", "proportional_int" );
};

DECLARE_HUDELEMENT( CHudFilmDemo );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudFilmDemo::CHudFilmDemo( const char *name ) : vgui::Panel( NULL, "HudHDRDemo" ), CHudElement( name )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	m_bFilmDemoActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudFilmDemo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudFilmDemo::Paint()
{
	int x, y, wide, tall;
  	GetBounds( x, y, wide, tall );

	// Draw the borders
	vgui::surface()->DrawSetColor( m_BorderColor );
  	vgui::surface()->DrawFilledRect( 0, 0, m_iBorderLeft, tall );			// Left
	vgui::surface()->DrawFilledRect( wide-m_iBorderRight, 0, wide, tall );	// Right
  	vgui::surface()->DrawFilledRect( m_iBorderLeft, 0, wide-m_iBorderRight, m_iBorderTop );			// Top
	vgui::surface()->DrawFilledRect( m_iBorderLeft, tall-m_iBorderBottom, wide-m_iBorderRight, tall ); // Bottom
	vgui::surface()->DrawFilledRect( ((wide-m_iBorderCenter)/2), m_iBorderTop, ((wide+m_iBorderCenter)/2), tall-m_iBorderBottom ); // Center
 
	// Get our scheme and font information
	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "MenuTitle" );
	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextColor( m_TextColor ); 

 	wchar_t *tempString = g_pVGuiLocalize->Find( m_pLeftStringID );
	if( tempString )
	{
		int iLength = 0;
		for ( wchar_t *wch = tempString; *wch != 0; wch++ )
		{
			iLength += vgui::surface()->GetCharacterWidth( hFont, *wch );
		}
		vgui::surface()->DrawSetTextPos( floor(wide * 0.25) - (iLength / 2), m_iLeftY );
		vgui::surface()->DrawPrintText( tempString, wcslen(tempString) );
	}

 	tempString = g_pVGuiLocalize->Find( m_pRightStringID );
	if( tempString )
	{
		int iLength = 0;
		for ( wchar_t *wch = tempString; *wch != 0; wch++ )
		{
			iLength += vgui::surface()->GetCharacterWidth( hFont, *wch );
		}
 		vgui::surface()->DrawSetTextPos( ceil(wide * 0.75) - (iLength / 2), m_iRightY );
		vgui::surface()->DrawPrintText( tempString, wcslen(tempString) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudFilmDemo::ShouldDraw()
{
	return ( m_bFilmDemoActive ); //&& m_flAlphaOverride > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CHudFilmDemo::SetFilmDemoActive( bool bActive ) 
{ 
	if ( bActive && !m_bFilmDemoActive )
	{
		ConVarRef hideHud( "hidehud" );
		hideHud.SetValue( 15 );
	}
	else if ( !bActive && m_bFilmDemoActive )
	{
		ConVarRef hideHud( "hidehud" );
		hideHud.SetValue( 0 );
	}

	m_bFilmDemoActive = bActive; 
}

void CHudFilmDemo::SetLeftStringID( const char *id )
{
	Q_strcpy( m_pLeftStringID, id );
}

void CHudFilmDemo::SetRightStringID( const char *id )
{
	Q_strcpy( m_pRightStringID, id );
}

void EnableHUDFilmDemo( bool bEnable, const char *left_string_id, const char *right_string_id )
{
	CHudFilmDemo *pHudDemo = (CHudFilmDemo*)GET_HUDELEMENT( CHudFilmDemo );
	if ( pHudDemo )
	{
		if( left_string_id )
		{
			pHudDemo->SetLeftStringID( left_string_id );
		}

		if( right_string_id )
		{
			pHudDemo->SetRightStringID( right_string_id );
		}

		pHudDemo->SetFilmDemoActive( bEnable );
	}
}


