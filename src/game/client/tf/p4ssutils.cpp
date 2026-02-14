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
	void ColorTextP4ss( vgui::TextImage *textImage, wchar_t *text,  const int team )
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
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 59, 196, 143, 255 ), iWChars );
				break;
			case 0x07: // Saves
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 255, 255, 0, 255 ), iWChars );
				break;
			case 0x08: // Intercepts
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 255, 0, 255, 255 ), iWChars );
				break;
			case 0x15: // Deathbomb
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 151, 224, 67, 255 ), iWChars );
				break;
			case 0x17: // Winstrat & Panacea
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color(77, 247, 4, 255), iWChars );
				break;
			case 0x14: // Steals
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 255, 128, 0, 255 ), iWChars );
				break;
			case 0x0F: // Splashes
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 91, 212, 180, 255 ), iWChars );
				break;
			case 0x13: // PRIMARY team color
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( color, iWChars );
				break;
			case 0x11: // SECONDARY team color
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( secondaryColor, iWChars );
				break;
			case 0x12: // Goals
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 59, 196, 59, 255 ), iWChars );
				break;
			case 0x01:
				iWChars--;
				iWChars = Max( 0, iWChars );
				textImage->AddColorChange( Color( 224, 217, 197, 255), iWChars );
				break;
			default:
				break;
			}

			txt++;
			iWChars++;
		}

		// remove all color characters so they dont bleed
		// 0x01-0x08 are safe and dont bleed but we use
		// more than 8 color chars witch makes this needed
		// + allows for more colors if they ever gonna be needed
		RemoveColorChars( text );
	}

	void RemoveColorChars( wchar_t *text )
	{
		const wchar_t *txt = text;
		int nWalk = 0;
		while ( *txt )
		{
			if ( !IsColorChar(*txt) )
			{
				text[nWalk] = *txt;
				++nWalk;
			}

			txt++;
		}

		// Null terminate
		text[nWalk] = L'\0';
	}

	bool IsColorChar( const wchar_t txt )
	{
		switch (txt)
		{
		case 0x01:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x14:
		case 0x0F:
		case 0x13:
		case 0x11:
		case 0x12:
		case 0x15:
		case 0x17:
			return true;
		}

		return false;
	}
}