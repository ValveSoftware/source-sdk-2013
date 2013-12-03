//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "hud_base_account.h"

using namespace vgui;

CHudBaseAccount::CHudBaseAccount( const char *pName ) :
	CHudNumericDisplay( NULL, pName ), CHudElement( pName )
{
	SetHiddenBits( HIDEHUD_PLAYERDEAD );
	SetIndent( false ); // don't indent small numbers in the drawing code - we're doing it manually
}


void CHudBaseAccount::LevelInit( void )
{
	m_iPreviousAccount = -1;
	m_iPreviousDelta = 0;
	m_flLastAnimationEnd = 0.0f;
	m_pszLastAnimationName = NULL;
	m_pszQueuedAnimationName = NULL;

	GetAnimationController()->StartAnimationSequence("AccountMoneyInvisible");
}

void CHudBaseAccount::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_clrRed	= pScheme->GetColor( "HudIcon_Red", Color( 255, 16, 16, 255 ) );
	m_clrGreen	= pScheme->GetColor( "HudIcon_Green", Color( 16, 255, 16, 255 ) );

	m_pAccountIcon = gHUD.GetIcon( "dollar_sign" );
	m_pPlusIcon = gHUD.GetIcon( "plus_sign" );
	m_pMinusIcon = gHUD.GetIcon( "minus_sign" );

	if( m_pAccountIcon )
	{
		icon_tall = ( GetTall() / 2 ) - YRES(2);
		float scale = icon_tall / (float)m_pAccountIcon->Height();
		icon_wide = ( scale ) * (float)m_pAccountIcon->Width();
	}
}


bool CHudBaseAccount::ShouldDraw()
{
	// Deriving classes must implement
	Assert( 0 );
	return false;
}


void CHudBaseAccount::Reset( void )
{
	// Round is restarting
	if ( m_flLastAnimationEnd > gpGlobals->curtime && m_pszLastAnimationName )
	{
		// if we had an animation in progress, queue it to be kicked it off again
		m_pszQueuedAnimationName = m_pszLastAnimationName;
	}
}


void CHudBaseAccount::Paint()
{
	int account  = GetPlayerAccount();

	//don't show delta on initial money give
	if( m_iPreviousAccount < 0 )
		m_iPreviousAccount = account;

	if( m_iPreviousAccount != account )
	{
		m_iPreviousDelta = account - m_iPreviousAccount;
		m_pszQueuedAnimationName = NULL;

		if( m_iPreviousDelta < 0 )
		{
			m_pszLastAnimationName = "AccountMoneyRemoved";
		}
		else 
		{
			m_pszLastAnimationName = "AccountMoneyAdded";
		}
		GetAnimationController()->StartAnimationSequence( m_pszLastAnimationName );
		m_flLastAnimationEnd = gpGlobals->curtime + GetAnimationController()->GetAnimationSequenceLength( m_pszLastAnimationName );

		m_iPreviousAccount = account;
	}
	else if ( m_pszQueuedAnimationName )
	{
		GetAnimationController()->StartAnimationSequence( m_pszQueuedAnimationName );
		m_pszQueuedAnimationName = NULL;
	}

	if( m_pAccountIcon )
	{
		m_pAccountIcon->DrawSelf( icon_xpos, icon_ypos, icon_wide, icon_tall, GetFgColor() );
	}

	int xpos = digit_xpos - GetNumberWidth( m_hNumberFont, account );

	// draw current account
	vgui::surface()->DrawSetTextColor(GetFgColor());
	PaintNumbers( m_hNumberFont, xpos, digit_ypos, account );

	//draw account additions / subtractions
	if( m_iPreviousDelta < 0 )
	{
		if( m_pMinusIcon )
		{
			m_pMinusIcon->DrawSelf( icon2_xpos, icon2_ypos, icon_wide, icon_tall, m_Ammo2Color );
		}
	}
	else
	{
		if( m_pPlusIcon )
		{
			m_pPlusIcon->DrawSelf( icon2_xpos, icon2_ypos, icon_wide, icon_tall, m_Ammo2Color );
		}
	}

	int delta = abs(m_iPreviousDelta);

	xpos = digit2_xpos - GetNumberWidth( m_hNumberFont, delta );

	// draw delta
	vgui::surface()->DrawSetTextColor(m_Ammo2Color);
	PaintNumbers( m_hNumberFont, xpos, digit2_ypos, delta );
}

int CHudBaseAccount::GetNumberWidth(HFont font, int number)
{
	int width = 0;

	surface()->DrawSetTextFont(font);
	wchar_t unicode[6];
	V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d", number);

	for (wchar_t *ch = unicode; *ch != 0; ch++)
	{
		width += vgui::surface()->GetCharacterWidth( font, *ch );
	}

	return width;
}