//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_BASE_ACCOUNT_H
#define HUD_BASE_ACCOUNT_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "hud_numericdisplay.h"

using namespace vgui;

class CHudBaseAccount : public CHudElement, public CHudNumericDisplay
{
public:
	DECLARE_CLASS_SIMPLE( CHudBaseAccount, CHudNumericDisplay );

	CHudBaseAccount( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void LevelInit( void );
	virtual void Reset( void );

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	int GetNumberWidth(HFont font, int number);

	// How much money does the player have
	virtual int	GetPlayerAccount( void ) { return 0; }

	// Requires game-specific g_pClientMode call, push to derived class
	virtual vgui::AnimationController *GetAnimationController( void ) { Assert( 0 ); return NULL; }

private:
	int m_iPreviousAccount;
	int m_iPreviousDelta;
	CHudTexture *m_pAccountIcon;
	CHudTexture *m_pMinusIcon;
	CHudTexture *m_pPlusIcon;

	Color m_clrRed;
	Color m_clrGreen;
	Color m_clrDeltaColor;

	CPanelAnimationVarAliasType( float, icon_xpos, "icon_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon_ypos, "icon_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon2_xpos, "icon2_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, icon2_ypos, "icon2_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_xpos, "digit_xpos", "50", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_ypos, "digit_ypos", "2", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit2_xpos, "digit2_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit2_ypos, "digit2_ypos", "0", "proportional_float" );
	CPanelAnimationVar( Color, m_Ammo2Color, "Ammo2Color", "0 0 0 0" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudNumbers" );

	float m_flLastAnimationEnd;
	const char *m_pszLastAnimationName;
	const char *m_pszQueuedAnimationName;

	float icon_tall;
	float icon_wide;
};


#endif	// HUD_BASE_ACCOUNT_H



