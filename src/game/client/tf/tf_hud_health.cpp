//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation of CHudHealth class.
//
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "ConVar.h"
#include "c_tf_player.h"

//=============================================================================
//
// TF Health Hud
//
class CHudHealth : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudHealth, CHudNumericDisplay );

public:

	CHudHealth( const char *pElementName );
	virtual void Init( void );
	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void Reset( void );

	virtual void OnThink();

	virtual bool ShouldDraw();	
	virtual void Paint( void );

private:

	int				m_nHealth;

	CHudTexture		*m_pHealthIcon;
	float			m_flHealthIconWidth;
	float			m_flHealthIconHeight;

	CPanelAnimationVar( vgui::HFont, m_hHealthFont, "NumberFont", "HudNumbers" );
	CPanelAnimationVarAliasType( float, health_xpos, "digit_xpos", "50", "proportional_float" );
	CPanelAnimationVarAliasType( float, health_ypos, "digit_ypos", "2", "proportional_float" );
};	

DECLARE_HUDELEMENT( CHudHealth );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealth::CHudHealth( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay( NULL, "HudHealth" )
{
	m_nHealth = 0;

	m_pHealthIcon = NULL;
	m_flHealthIconHeight = 0;
	m_flHealthIconWidth	= 0;

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Init()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return;

	m_nHealth = pTFPlayer->GetHealth();
	SetDisplayValue( m_nHealth );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::ApplySchemeSettings( IScheme *scheme )
{
	// Fall back to base.
	BaseClass::ApplySchemeSettings( scheme );

	// Setup the health icon.
	if( !m_pHealthIcon )
	{
		m_pHealthIcon = gHUD.GetIcon( "health_icon" );
	}	

	if( m_pHealthIcon )
	{
		m_flHealthIconHeight = GetTall() - YRES( 2 );
		float flScale = m_flHealthIconHeight / static_cast<float>( m_pHealthIcon->Height() );
		m_flHealthIconWidth = flScale * static_cast<float>( m_pHealthIcon->Width() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: reset health to normal color at round restart
//-----------------------------------------------------------------------------
void CHudHealth::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthRestored" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::OnThink()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return; 

	// Never below zero.
	int nHealth = MAX( pPlayer->GetHealth(), 0 );

	// Only update the fade if we've changed health
	if ( nHealth == m_nHealth )
		return;

	// We are badly injured.
	if ( nHealth <= 25 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthLow" );
	}
	// Got hit.
	else if( nHealth < m_nHealth )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthTookDamage" );
	}
	// Gained health.
	else if( nHealth > m_nHealth )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HealthRestored" );
	}

	m_nHealth = nHealth;
//	SetDisplayValue( m_nHealth );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHealth::ShouldDraw()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return false;

	return !pTFPlayer->IsObserver();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealth::Paint( void )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return;

	// Get the team color.
	Color teamColor;
	pTFPlayer->GetTeamColor( teamColor );

	// Draw the health icon.
	if( m_pHealthIcon )
	{
		m_pHealthIcon->DrawSelf( 0, 2, m_flHealthIconWidth, m_flHealthIconHeight, teamColor );
	}

	// Primary grenade count.
	vgui::surface()->DrawSetTextColor( teamColor );
	PaintNumbers( m_hHealthFont, health_xpos, health_ypos, m_nHealth );

	PaintLabel();
}

//=============================================================================
//
// TF Armor Hud
//
class CHudArmor : public CHudElement, public CHudNumericDisplay
{
public:

	DECLARE_CLASS_SIMPLE( CHudArmor, CHudNumericDisplay );

	CHudArmor( const char *pName );
	virtual void Init();
	virtual void ApplySchemeSettings( IScheme *scheme );

	virtual void OnThink();

	virtual bool ShouldDraw();	
	virtual void Paint();

private:

	int				m_nArmor;

	CHudTexture		*m_pArmorIcon;
	float			m_flArmorIconWidth;
	float			m_flArmorIconHeight;
};


DECLARE_HUDELEMENT( CHudArmor );

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CHudArmor::CHudArmor( const char *pName ) : CHudNumericDisplay( NULL, "HudArmor" ), CHudElement( pName )
{
	m_nArmor = 0;

	m_pArmorIcon = NULL;
	m_flArmorIconHeight = 0;
	m_flArmorIconWidth = 0;

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudArmor::Init()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return;

	m_nArmor = pTFPlayer->ArmorValue();
	SetDisplayValue( m_nArmor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudArmor::ApplySchemeSettings( IScheme *scheme )
{
	// Fall back to base.
	BaseClass::ApplySchemeSettings( scheme );

	// Setup the health icon.
	if( !m_pArmorIcon )
	{
		m_pArmorIcon = gHUD.GetIcon( "shield_bright" );
	}	

	if( m_pArmorIcon )
	{
		m_flArmorIconHeight = GetTall() - YRES( 2 );
		float flScale = m_flArmorIconHeight / static_cast<float>( m_pArmorIcon->Height() );
		m_flArmorIconWidth = flScale * static_cast<float>( m_pArmorIcon->Width() );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudArmor::OnThink()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return;

	m_nArmor = pTFPlayer->ArmorValue();
	SetDisplayValue( m_nArmor );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudArmor::ShouldDraw()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return false;

	// Running the experiment - no armor!
	return ( pTFPlayer->ArmorValue() != 0 );

	return !pTFPlayer->IsObserver();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudArmor::Paint()
{
	// Draw the armor icon.
	if( m_pArmorIcon )
	{
		m_pArmorIcon->DrawSelf( 0, 2, m_flArmorIconWidth, m_flArmorIconHeight, GetFgColor() );
	}

	// Base class paint.
	BaseClass::Paint();
}
