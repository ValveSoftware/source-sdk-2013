//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef HUD_BITMAPNUMERICDISPLAY_H
#define HUD_BITMAPNUMERICDISPLAY_H
#ifdef _WIN32
#pragma once
#endif

#include "hud_numericdisplay.h"

class CHudBitmapNumericDisplay : public vgui::Panel
{	
	DECLARE_CLASS_SIMPLE( CHudBitmapNumericDisplay, vgui::Panel );

public:
	CHudBitmapNumericDisplay(vgui::Panel *parent, const char *name);

	void SetDisplayValue(int value);
	void SetShouldDisplayValue(bool state);

protected:
	// vgui overrides
	virtual void PaintBackground( void );
	virtual void Paint();
	void PaintNumbers(int xpos, int ypos, int value, Color col, int numSigDigits);
	virtual void PaintNumbers(int xpos, int ypos, int value, Color col)
	{
		PaintNumbers(xpos, ypos, value, col, 1);
	}

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( Color, m_TextColor, "TextColor", "FgColor" );
	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );

	CPanelAnimationVarAliasType( float, digit_xpos, "digit_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_ypos, "digit_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, digit_height, "digit_height", "16", "proportional_float" );

private:

	CHudTexture *m_pNumbers[10];

	int m_iValue;
	bool m_bDisplayValue;
};

#endif //HUD_BITMAPNUMERICDISPLAY_H
