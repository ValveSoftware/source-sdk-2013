//========= Copyright Valve Corporation, All rights reserved. ============//
// Overwatch-style ability + ultimate HUD.
//=============================================================================
#include "cbase.h"

#ifdef SOURCESDK

#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "tf_gamerules.h"
#include "ow_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
class CHudOWAbilities : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudOWAbilities, vgui::Panel );

public:
	CHudOWAbilities( const char *pElementName );

	virtual bool ShouldDraw( void );
	virtual void Paint( void );

private:
	void DrawCooldownBox( int x, int y, int w, int h, float flEndTime, const char *pszKey );
	void DrawUltRing( int cx, int cy, int radius, float flCharge );
};

DECLARE_HUDELEMENT( CHudOWAbilities );

//-----------------------------------------------------------------------------
CHudOWAbilities::CHudOWAbilities( const char *pElementName )
	: CHudElement( pElementName ), BaseClass( NULL, "HudOWAbilities" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
bool CHudOWAbilities::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
	{
		return false;
	}

	if ( !TFGameRules() || !TFGameRules()->IsOverwatchMode() )
	{
		return false;
	}

	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	return pLocal && pLocal->IsAlive() && pLocal->GetTeamNumber() > LAST_SHARED_TEAM;
}

//-----------------------------------------------------------------------------
void CHudOWAbilities::DrawCooldownBox( int x, int y, int w, int h, float flEndTime, const char *pszKey )
{
	const float flNow = gpGlobals->curtime;
	const bool bReady = flNow >= flEndTime;

	surface()->DrawSetColor( bReady ? 80 : 40, bReady ? 200 : 80, 255, 220 );
	surface()->DrawOutlinedRect( x, y, x + w, y + h );

	if ( !bReady && flEndTime > flNow )
	{
		const float flTotal = 12.0f;
		const float flFrac = Clamp( ( flEndTime - flNow ) / flTotal, 0.0f, 1.0f );
		const int iFillH = (int)( h * flFrac );
		surface()->DrawSetColor( 30, 60, 120, 200 );
		surface()->DrawFilledRect( x + 2, y + h - iFillH, x + w - 2, y + h - 2 );
	}

	surface()->DrawSetTextColor( 255, 255, 255, 255 );
	surface()->DrawSetTextFont( g_hFontTrebuchet24 );
	surface()->DrawSetTextPos( x + 8, y + 8 );
	wchar_t wszKey[32];
	g_pVGuiLocalize->ConvertANSIToUnicode( pszKey, wszKey, sizeof( wszKey ) );
	surface()->DrawPrintText( wszKey, V_wcslen( wszKey ) );
}

//-----------------------------------------------------------------------------
void CHudOWAbilities::DrawUltRing( int cx, int cy, int radius, float flCharge )
{
	const float flFrac = Clamp( flCharge / OW_ULT_CHARGE_MAX, 0.0f, 1.0f );
	surface()->DrawSetColor( 255, 200, 40, 255 );
	for ( int i = 0; i < 32; ++i )
	{
		const float a0 = ( i / 32.0f ) * 6.28318f;
		const float a1 = ( ( i + 1 ) / 32.0f ) * 6.28318f;
		if ( ( i / 32.0f ) > flFrac )
		{
			break;
		}
		int x0 = cx + (int)( cosf( a0 ) * radius );
		int y0 = cy + (int)( sinf( a0 ) * radius );
		int x1 = cx + (int)( cosf( a1 ) * radius );
		int y1 = cy + (int)( sinf( a1 ) * radius );
		surface()->DrawLine( x0, y0, x1, y1 );
	}
}

//-----------------------------------------------------------------------------
void CHudOWAbilities::Paint( void )
{
	C_TFPlayer *pLocal = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocal )
	{
		return;
	}

	const int sw = ScreenWidth();
	const int baseX = sw / 2 - 120;
	const int baseY = ScreenHeight() - 100;

	DrawCooldownBox( baseX, baseY, 48, 48, pLocal->m_flOWCooldown0, "Q" );
	DrawCooldownBox( baseX + 56, baseY, 48, 48, pLocal->m_flOWCooldown1, "E" );
	DrawCooldownBox( baseX + 112, baseY, 48, 48, pLocal->m_flOWCooldown2, "Shift" );
	DrawUltRing( baseX + 200, baseY + 24, 28, pLocal->m_flOWUltCharge );

	surface()->DrawSetTextColor( 255, 255, 255, 200 );
	surface()->DrawSetTextFont( g_hFontTrebuchet24 );
	surface()->DrawSetTextPos( baseX + 168, baseY + 12 );
	surface()->DrawPrintText( L"ULT R", 5 );
}

#endif // SOURCESDK
