//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
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
#include "materialsystem/imaterialsystemhardwareconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudHDRDemo : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudHDRDemo, vgui::Panel );
public:
	CHudHDRDemo( const char *name );

	// vgui overrides
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme );
	virtual void Paint( void );
	virtual bool ShouldDraw( void );

	void SetHDRDemoActive( bool bActive );

private:
	bool	m_bHDRDemoActive;

	// Painting
	//CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "0" );
	CPanelAnimationVar( Color, m_BorderColor, "BorderColor", "0 0 0 255" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "255 255 255 255" );
	CPanelAnimationVarAliasType( int, m_iBorderLeft, "BorderLeft", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderRight, "BorderRight", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderTop, "BorderTop", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderBottom, "BorderBottom", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBorderCenter, "BorderCenter", "8", "proportional_int" );

	CPanelAnimationVarAliasType( int, m_iLeftY, "LeftTitleY", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRightY, "RightTitleY", "8", "proportional_int" );
};

DECLARE_HUDELEMENT( CHudHDRDemo );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudHDRDemo::CHudHDRDemo( const char *name ) : vgui::Panel( NULL, "HudHDRDemo" ), CHudElement( name )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	m_bHDRDemoActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHDRDemo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetSize( ScreenWidth(), ScreenHeight() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHDRDemo::Paint()
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
	vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "HDRDemoText" );
	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextColor( m_TextColor ); 

	// Left Title
 	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_HDRDEMO_LeftTitle");
	if (tempString)
	{
		int iLength = 0;
		for ( wchar_t *wch = tempString; *wch != 0; wch++ )
		{
			iLength += vgui::surface()->GetCharacterWidth( hFont, *wch );
		}
		vgui::surface()->DrawSetTextPos( floor(wide * 0.25) - (iLength / 2), m_iLeftY );
		vgui::surface()->DrawPrintText(tempString, wcslen(tempString));
	}

	// Right Title
	tempString = g_pVGuiLocalize->Find("#Valve_HDRDEMO_RightTitle");
	if (tempString)
	{
		int iLength = 0;
		for ( wchar_t *wch = tempString; *wch != 0; wch++ )
		{
			iLength += vgui::surface()->GetCharacterWidth( hFont, *wch );
		}
 		vgui::surface()->DrawSetTextPos( ceil(wide * 0.75) - (iLength / 2), m_iRightY );
		vgui::surface()->DrawPrintText(tempString, wcslen(tempString));
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHDRDemo::ShouldDraw()
{
	return ( 
		// no split screen hud if not hdr
		(g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE) &&
		m_bHDRDemoActive 
		); //&& m_flAlphaOverride > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CHudHDRDemo::SetHDRDemoActive( bool bActive ) 
{ 
	if ( bActive && !m_bHDRDemoActive )
	{
		ConVarRef pHideHud( "hidehud" );
		pHideHud.SetValue( 15 );
	}
	else if ( !bActive && m_bHDRDemoActive )
	{
		ConVarRef pHideHud( "hidehud" );
		pHideHud.SetValue( 0 );
	}

	m_bHDRDemoActive = bActive; 
}

//=======================================================================================================
// CONVAR to toggle this hud element
void mat_show_ab_hdr_hudelement_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CHudHDRDemo *pHudDemo = (CHudHDRDemo*)GET_HUDELEMENT( CHudHDRDemo );
	if ( pHudDemo )
	{
		ConVarRef var( pConVar );
		pHudDemo->SetHDRDemoActive( var.GetBool() );
	}
}
ConVar mat_show_ab_hdr_hudelement( "mat_show_ab_hdr_hudelement", "0", FCVAR_CHEAT, "HDR Demo HUD Element toggle.", mat_show_ab_hdr_hudelement_changed );
