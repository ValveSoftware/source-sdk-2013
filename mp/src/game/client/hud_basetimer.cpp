//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws a timer in the format "Minutes:Seconds"
// Seconds are padded with zeros
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include "hud_basetimer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CHudBaseTimer::CHudBaseTimer(vgui::Panel *parent, const char *name) : BaseClass(parent, name)
{
	m_iMinutes = 0;
	m_iSeconds = 0;
	SetLabelText(L"");
}

void CHudBaseTimer::SetMinutes(int minutes)
{	
	m_iMinutes = minutes;
}

void CHudBaseTimer::SetSeconds(int seconds)
{	
	m_iSeconds = seconds;
}

void CHudBaseTimer::PaintTime(HFont font, int xpos, int ypos, int mins, int secs)
{
	surface()->DrawSetTextFont(font);
	wchar_t unicode[6];
	V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d:%.2d", mins, secs);
	
	surface()->DrawSetTextPos(xpos, ypos);
	surface()->DrawUnicodeString( unicode );
}

void CHudBaseTimer::Paint()
{
	float alpha = m_flAlphaOverride / 255;
	Color fgColor = GetFgColor();
	fgColor[3] *= alpha;
	SetFgColor( fgColor );
	
	surface()->DrawSetTextColor(GetFgColor());
	PaintTime( m_hNumberFont, digit_xpos, digit_ypos, m_iMinutes, m_iSeconds );

	// draw the overbright blur
	for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
	{
		if (fl >= 1.0f)
		{
			PaintTime(m_hNumberGlowFont, digit_xpos, digit_ypos, m_iMinutes, m_iSeconds);
		}
		else
		{
			// draw a percentage of the last one
			Color col = GetFgColor();
			col[3] *= fl;
			surface()->DrawSetTextColor(col);
			PaintTime(m_hNumberGlowFont, digit_xpos, digit_ypos, m_iMinutes, m_iSeconds);
		}
	}

	BaseClass::PaintLabel();
}

void CHudBaseTimer::SetToPrimaryColor()
{
	SetFgColor(m_TextColor);
}

void CHudBaseTimer::SetToSecondaryColor()
{
	SetFgColor(m_FlashColor);
}
