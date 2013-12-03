//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws the death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <game/client/iviewport.h>
#include <igameevents.h>
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudDeathNotice, vgui::Panel );
public:
	CHudDeathNotice( const char *pElementName );
	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void Paint();

	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	void FireGameEvent( KeyValues * event);

private:
	CHudTexture			*m_iconD_skull;  // sprite index of skull icon

	vgui::HFont		m_hTextFont;
	Color			m_clrText;
};

using namespace vgui;

//
//-----------------------------------------------------
//

DECLARE_HUDELEMENT( CHudDeathNotice );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudDeathNotice::CHudDeathNotice( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

void CHudDeathNotice::ApplySchemeSettings( IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_hTextFont = scheme->GetFont( "HudNumbersSmall" );
	m_clrText = scheme->GetColor( "FgColor", GetFgColor() );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------
struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH];
	char szVictim[MAX_PLAYER_NAME_LENGTH];
	CHudTexture *iconDeath;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	float flDisplayTime;
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

// Robin HACKHACK: HL2 doesn't use deathmsgs, so I just forced these down below our minimap.
// It should be positioned by TF2/HL2 separately, and TF2 should position it according to the minimap position
#define DEATHNOTICE_TOP		YRES( 140 )	// Was: 20

DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Init( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::VidInit( void )
{
	m_iconD_skull = gHUD.GetIcon( "d_skull" );
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( rgDeathNoticeList[0].iconDeath ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudDeathNotice::Paint()
{
	int x, y;

	surface()->DrawSetTextFont( m_hTextFont );
	surface()->DrawSetTextColor( m_clrText );

	for ( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		// we've gone through them all
		if ( rgDeathNoticeList[i].iconDeath == NULL )
			break;  

		// display time has expired
		// remove the current item from the list
		if ( rgDeathNoticeList[i].flDisplayTime < gpGlobals->curtime )
		{ 
			Q_memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
			// continue on the next item;  stop the counter getting incremented
			i--;  
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = MIN( rgDeathNoticeList[i].flDisplayTime, gpGlobals->curtime + DEATHNOTICE_DISPLAY_TIME );

		// Draw the death notice
		y = DEATHNOTICE_TOP + (20 * i) + 100;  //!!!

		CHudTexture *icon = rgDeathNoticeList[i].iconDeath;
		if ( !icon )
			continue;

		wchar_t victim[ 256 ];
		wchar_t killer[ 256 ];

		g_pVGuiLocalize->ConvertANSIToUnicode( rgDeathNoticeList[i].szVictim, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( rgDeathNoticeList[i].szKiller, killer, sizeof( killer ) );

		int len = UTIL_ComputeStringWidth( m_hTextFont, victim );

		x = ScreenWidth() - len - icon->Width() - 5;

		if ( !rgDeathNoticeList[i].iSuicide )
		{
			int lenkiller = UTIL_ComputeStringWidth( m_hTextFont, killer );

			x -= (5 + lenkiller );

			// Draw killer's name
			surface()->DrawSetTextPos( x, y );
			surface()->DrawUnicodeString( killer );
			surface()->DrawGetTextPos( x, y );
			x += 5;
		}

		Color iconColor( 255, 80, 0, 255 );

		if ( rgDeathNoticeList[i].iTeamKill )
		{
			// display it in sickly green
			iconColor = Color( 10, 240, 10, 255 );
		}

		// Draw death weapon
		icon->DrawSelf( x, y, iconColor );

		x += icon->Width();

		// Draw victims name
		surface()->DrawSetTextPos( x, y );
		surface()->DrawUnicodeString( victim );
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CHudDeathNotice::FireGameEvent( KeyValues * event)
{
	// Got message during connection
	if ( !g_PR )
		return;

	int killer = engine->GetPlayerForUserID( event->GetInt("killer") ); 
	int victim = engine->GetPlayerForUserID( event->GetInt("victim") );

	char killedwith[32];
	Q_snprintf( killedwith, sizeof( killedwith ), "d_%s", event->GetString("weapon") );

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iconDeath == NULL )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		Q_memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	// Get the names of the players
	const char *killer_name = g_PR->GetPlayerName( killer );
	const char *victim_name = g_PR->GetPlayerName( victim );
	if ( !killer_name )
		killer_name = "";
	if ( !victim_name )
		victim_name = "";

	Q_strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );

	if ( killer == victim || killer == 0 )
		rgDeathNoticeList[i].iSuicide = true;

	if ( !strcmp( killedwith, "d_teammate" ) )
		rgDeathNoticeList[i].iTeamKill = true;

	// try and find the death identifier in the icon list
	rgDeathNoticeList[i].iconDeath = gHUD.GetIcon( killedwith );
	if ( !rgDeathNoticeList[i].iconDeath )
	{
		// can't find it, so use the default skull & crossbones icon
		rgDeathNoticeList[i].iconDeath = m_iconD_skull;
	}

	DEATHNOTICE_DISPLAY_TIME = hud_deathnotice_time.GetFloat();

	rgDeathNoticeList[i].flDisplayTime = gpGlobals->curtime + DEATHNOTICE_DISPLAY_TIME;

	// record the death notice in the console
	if ( rgDeathNoticeList[i].iSuicide )
	{
		Msg( "%s", rgDeathNoticeList[i].szVictim );

		if ( !strcmp( killedwith, "d_world" ) )
		{
			Msg( " died" );
		}
		else
		{
			Msg( " killed self" );
		}
	}
	else if ( rgDeathNoticeList[i].iTeamKill )
	{
		Msg( "%s", rgDeathNoticeList[i].szKiller );
		Msg( " killed his teammate " );
		Msg( "%s", rgDeathNoticeList[i].szVictim );
	}
	else
	{
		Msg( "%s", rgDeathNoticeList[i].szKiller );
		Msg( " killed " );
		Msg( "%s", rgDeathNoticeList[i].szVictim );
	}

	if ( killedwith && *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "d_world" ) && !rgDeathNoticeList[i].iTeamKill )
	{
		Msg( " with " );

		// replace the code names with the 'real' names
		if ( !strcmp( killedwith+2, "egon" ) )
			Q_strncpy( killedwith, "d_gluon gun", sizeof( killedwith ) );
		if ( !strcmp( killedwith+2, "gauss" ) )
			Q_strncpy( killedwith, "d_tau cannon", sizeof( killedwith ) );

		Msg( "%s", killedwith+2 ); // skip over the "d_" part
	}

	Msg( "\n" );
}




