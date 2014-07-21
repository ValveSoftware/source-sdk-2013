//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "text_message.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "KeyValues.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include "IEffects.h"
#include "hudelement.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: HDU Damage indication
//-----------------------------------------------------------------------------
class CHudPoisonDamageIndicator : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudPoisonDamageIndicator, vgui::Panel );

public:
	CHudPoisonDamageIndicator( const char *pElementName );
	void Reset( void );
	virtual bool ShouldDraw( void );

private:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "8", "proportional_float" );
	CPanelAnimationVarAliasType( float, text_ygap, "text_ygap", "14", "proportional_float" );

	bool m_bDamageIndicatorVisible;
};

DECLARE_HUDELEMENT( CHudPoisonDamageIndicator );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudPoisonDamageIndicator::CHudPoisonDamageIndicator( const char *pElementName ) : CHudElement( pElementName ), BaseClass(NULL, "HudPoisonDamageIndicator")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPoisonDamageIndicator::Reset( void )
{
	SetAlpha( 0 );
	m_bDamageIndicatorVisible = false;
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudPoisonDamageIndicator::ShouldDraw( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	bool bNeedsDraw = ( ( pPlayer->IsPoisoned() != m_bDamageIndicatorVisible ) || ( GetAlpha() > 0 ) );

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: updates poison damage
//-----------------------------------------------------------------------------
void CHudPoisonDamageIndicator::OnThink()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// update poison damage indicator
	bool bShouldIndicatorBeVisible = pPlayer->IsPoisoned();
	if (bShouldIndicatorBeVisible == m_bDamageIndicatorVisible)
		return;

	// state change
	m_bDamageIndicatorVisible = bShouldIndicatorBeVisible;

	if (m_bDamageIndicatorVisible)
	{
		SetVisible(true);
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "PoisonDamageTaken" );
	}
	else
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "PoisonDamageCured" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Paints the damage display
//-----------------------------------------------------------------------------
void CHudPoisonDamageIndicator::Paint()
{
	// draw the text
	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( m_TextColor );
	surface()->DrawSetTextPos(text_xpos, text_ypos);
	int ypos = text_ypos;

	const wchar_t *labelText = g_pVGuiLocalize->Find("Valve_HudPoisonDamage");
	Assert( labelText );
	for (const wchar_t *wch = labelText; wch && *wch != 0; wch++)
	{
		if (*wch == '\n')
		{
			ypos += text_ygap;
			surface()->DrawSetTextPos(text_xpos, ypos);
		}
		else
		{
			surface()->DrawUnicodeChar(*wch);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudPoisonDamageIndicator::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}
