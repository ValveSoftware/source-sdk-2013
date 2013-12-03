//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui_controls/Panel.h>
#include <vgui/IVGui.h>


using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HUD_POSTURE_UPDATES_PER_SECOND	10
#define HUD_POSTURE_FADE_TIME 0.4f
#define CROUCHING_CHARACTER_INDEX 92  // index of the crouching dude in the TTF font

//-----------------------------------------------------------------------------
// Purpose: Shows the sprint power bar
//-----------------------------------------------------------------------------
class CHudPosture : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudPosture, vgui::Panel );

public:
	CHudPosture( const char *pElementName );
	bool			ShouldDraw( void );

#ifdef _X360 	// if not xbox 360, don't waste code space on this
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnTick( void );

protected:
	virtual void	Paint();

	float	m_duckTimeout; /// HUD_POSTURE_FADE_TIME after the last known time the player was ducking

private:

	CPanelAnimationVar( vgui::HFont, m_hFont, "Font", "WeaponIconsSmall" );
	CPanelAnimationVarAliasType( float, m_IconX, "icon_xpos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_IconY, "icon_ypos", "4", "proportional_float" );

	enum { NOT_FADING, 
		   FADING_UP, 
		   FADING_DOWN
	} m_kIsFading;
#endif
};	


DECLARE_HUDELEMENT( CHudPosture );


namespace
{
	// Don't pass a null pPlayer. Doesn't check for it.
	inline bool PlayerIsDucking(C_BasePlayer *pPlayer)
	{
		return  pPlayer->m_Local.m_bDucked &&		 // crouching 
				pPlayer->GetGroundEntity() != NULL ; // but not jumping
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudPosture::CHudPosture( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPosture" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	if( IsX360() )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), (1000/HUD_POSTURE_UPDATES_PER_SECOND) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudPosture::ShouldDraw()
{
#ifdef _X360
	return ( m_duckTimeout >= gpGlobals->curtime &&
		CHudElement::ShouldDraw() );
#else
	return false;
#endif
}

#ifdef _X360

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPosture::Init( void )
{
	m_duckTimeout = 0.0f;
	m_kIsFading = NOT_FADING;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPosture::Reset( void )
{
	Init();
}

void CHudPosture::OnTick( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if ( PlayerIsDucking(pPlayer) )
	{
		m_duckTimeout = gpGlobals->curtime + HUD_POSTURE_FADE_TIME; // kick the timer forward
		if (GetAlpha() < 255)
		{
			// if not fully faded in, and not fading in, start fading in.
			if (m_kIsFading != FADING_UP)
			{
				m_kIsFading = FADING_UP;
				GetAnimationController()->RunAnimationCommand( this, "alpha", 255, 0, HUD_POSTURE_FADE_TIME, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
			}
		}
		else
		{
			m_kIsFading = NOT_FADING;
		}
	}	
	else // player is not ducking
	{
		if (GetAlpha() > 0)
		{
			// if not faded out or fading out, fade out.
			if (m_kIsFading != FADING_DOWN)
			{
				m_kIsFading = FADING_DOWN;
				GetAnimationController()->RunAnimationCommand( this, "alpha", 0, 0, HUD_POSTURE_FADE_TIME, vgui::AnimationController::INTERPOLATOR_SIMPLESPLINE );
			}
		}
		else
		{
			m_kIsFading = NOT_FADING;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws the posture elements we want.
//-----------------------------------------------------------------------------
void CHudPosture::Paint()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	SetPaintBackgroundEnabled( true );

	Color clr;
	clr = gHUD.m_clrNormal;
	clr[3] = 255;

	// Pick the duck character
	wchar_t duck_char = CROUCHING_CHARACTER_INDEX;

	surface()->DrawSetTextFont( m_hFont );
	surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextPos( m_IconX, m_IconY );
	surface()->DrawUnicodeChar( duck_char );
}

#endif

