#include "cbase.h"
#include <algorithm>
#include <cstdio>
#include "p4ssutils.h"
#include "dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
using namespace vgui;

namespace P4ss
{
	void ColorTextP4ss(vgui::TextImage *textImage, const wchar_t *text,  const int team)
	{
		textImage->ClearColorChangeStream();
		Color color;
		Color secondaryColor;
		if ( team == TF_TEAM_BLUE )
		{
			color = P4SS_BLUE;
			secondaryColor = P4SS_RED;
		}
		else if ( team == TF_TEAM_RED ) 
		{
			color = P4SS_RED;
			secondaryColor = P4SS_BLUE;
		}
		else {
			color = COLOR_TF_SPECTATOR;
			secondaryColor = P4SS_RED;
		}
		// We change the title's text color to match the colors of the matching
		// model panel backgrounds
		const wchar_t *txt = text;
		int iWChars = 0;
		while ( txt && *txt )
		{
			switch ( *txt )
			{
			case 0x06: // Assists
				textImage->AddColorChange( Color( 59, 196, 143, 255 ), iWChars );
				break;
			case 0x07: // Saves
				textImage->AddColorChange( Color( 255, 255, 0, 255 ), iWChars );
				break;
			case 0x08: // Intercepts
				textImage->AddColorChange( Color( 255, 0, 255, 255 ), iWChars );
				break;
			case 0x15: // Deathbomb
				textImage->AddColorChange( Color( 151, 224, 67, 255 ), iWChars );
				break;
			case 0x17: // Winstrat & Panacea
				textImage->AddColorChange( Color(77, 247, 4, 255), iWChars );
				break;
			case 0x14: // Steals
				textImage->AddColorChange( Color( 255, 128, 0, 255 ), iWChars );
				break;
			case 0x0F: // Splashes
				textImage->AddColorChange( Color( 91, 212, 180, 255 ), iWChars );
				break;
			case 0x13: // PRIMARY team color
				textImage->AddColorChange( color, iWChars );
				break;
			case 0x11: // SECONDARY team color
				textImage->AddColorChange( secondaryColor, iWChars );
				break;
			case 0x12: // Goals
				textImage->AddColorChange( Color( 59, 196, 59, 255 ), iWChars );
				break;
			case 0x01:
				textImage->AddColorChange( Color( 224, 217, 197, 255), iWChars );
				break;
			default:
				break;
			}

			txt++;
			iWChars++;
		}
	}
}