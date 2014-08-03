//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>
#include "hud.h"
#include "hud_suitpower.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Shows the flashlight icon
//-----------------------------------------------------------------------------
class CHudFlashlight : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudFlashlight, vgui::Panel );

public:
	CHudFlashlight( const char *pElementName );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

protected:
	virtual void Paint();

private:
	void SetFlashlightState( bool flashlightOn );
	void Reset( void );
	
	bool	m_bFlashlightOn;
	CPanelAnimationVar( vgui::HFont, m_hFont, "Font", "WeaponIconsSmall" );
	CPanelAnimationVarAliasType( float, m_IconX, "icon_xpos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_IconY, "icon_ypos", "4", "proportional_float" );
	
	CPanelAnimationVarAliasType( float, m_flBarInsetX, "BarInsetX", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarInsetY, "BarInsetY", "18", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBarWidth, "BarWidth", "28", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarHeight, "BarHeight", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkWidth, "BarChunkWidth", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flBarChunkGap, "BarChunkGap", "2", "proportional_float" );
};	

using namespace vgui;

#ifdef HL2_EPISODIC
DECLARE_HUDELEMENT( CHudFlashlight );
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudFlashlight::CHudFlashlight( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudFlashlight" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pScheme - 
//-----------------------------------------------------------------------------
void CHudFlashlight::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: Start with our background off
//-----------------------------------------------------------------------------
void CHudFlashlight::Reset( void )
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "SuitFlashlightOn" ); 
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void CHudFlashlight::SetFlashlightState( bool flashlightOn )
{
	if ( m_bFlashlightOn == flashlightOn )
		return;

	m_bFlashlightOn = flashlightOn;
}

#define WCHAR_FLASHLIGHT_ON  169
#define WCHAR_FLASHLIGHT_OFF 174

//-----------------------------------------------------------------------------
// Purpose: draws the flashlight icon
//-----------------------------------------------------------------------------
void CHudFlashlight::Paint()
{
#ifdef HL2_EPISODIC
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Only paint if we're using the new flashlight code
	if ( pPlayer->m_HL2Local.m_flFlashBattery < 0.0f )
	{
		SetPaintBackgroundEnabled( false );
		return;
	}

	bool bIsOn = pPlayer->IsEffectActive( EF_DIMLIGHT );
	SetFlashlightState( bIsOn );

	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (pPlayer->m_HL2Local.m_flFlashBattery * 1.0f/100.0f) + 0.5f );

	Color clrFlashlight;
	clrFlashlight = ( enabledChunks < ( chunkCount / 4 ) ) ? gHUD.m_clrCaution : gHUD.m_clrNormal;
	clrFlashlight[3] = ( bIsOn ) ? 255: 32;

	// Pick the right character given our current state
	wchar_t pState = ( bIsOn ) ? WCHAR_FLASHLIGHT_ON : WCHAR_FLASHLIGHT_OFF;

	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( clrFlashlight );
	surface()->DrawSetTextPos( m_IconX, m_IconY );
	surface()->DrawUnicodeChar( pState );

	// Don't draw the progress bar is we're fully charged
	if ( bIsOn == false && chunkCount == enabledChunks )
		return;

	// draw the suit power bar
	surface()->DrawSetColor( clrFlashlight );
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	
	// Be even less transparent than we already are
	clrFlashlight[3] = clrFlashlight[3] / 8;

	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( clrFlashlight );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
#endif // HL2_EPISODIC
}
