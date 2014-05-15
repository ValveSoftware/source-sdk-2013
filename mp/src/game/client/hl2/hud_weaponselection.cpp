//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"
#include "input.h"
#include "../hud_crosshair.h"

#include "VGuiMatSurface/IMatSystemSurface.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>

#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar hud_showemptyweaponslots( "hud_showemptyweaponslots", "1", FCVAR_ARCHIVE, "Shows slots for missing weapons when recieving weapons out of order" );

#define SELECTION_TIMEOUT_THRESHOLD		0.5f	// Seconds
#define SELECTION_FADEOUT_TIME			0.75f

#define PLUS_DISPLAY_TIMEOUT			0.5f	// Seconds
#define PLUS_FADEOUT_TIME				0.75f

#define FASTSWITCH_DISPLAY_TIMEOUT		1.5f
#define FASTSWITCH_FADEOUT_TIME			1.5f

#define CAROUSEL_SMALL_DISPLAY_ALPHA	200.0f
#define FASTSWITCH_SMALL_DISPLAY_ALPHA	160.0f

#define MAX_CAROUSEL_SLOTS				5

//-----------------------------------------------------------------------------
// Purpose: hl2 weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void )
	{ 
		return m_hSelectedWeapon;
	}

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void LevelInit();

protected:
	virtual void OnThink();
	virtual void Paint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

	virtual void SetWeaponSelected()
	{
		CBaseHudWeaponSelection::SetWeaponSelected();

		switch( hud_fastswitch.GetInt() )
		{
		case HUDTYPE_FASTSWITCH:
		case HUDTYPE_CAROUSEL:
			ActivateFastswitchWeaponDisplay( GetSelectedWeapon() );
			break;
		case HUDTYPE_PLUS:
			ActivateWeaponHighlight( GetSelectedWeapon() );
			break;
		default:
			// do nothing
			break;
		}
	}

private:
	C_BaseCombatWeapon *FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);

	void DrawLargeWeaponBox( C_BaseCombatWeapon *pWeapon, bool bSelected, int x, int y, int wide, int tall, Color color, float alpha, int number );
	void ActivateFastswitchWeaponDisplay( C_BaseCombatWeapon *pWeapon );
	void ActivateWeaponHighlight( C_BaseCombatWeapon *pWeapon );
	float GetWeaponBoxAlpha( bool bSelected );
	int GetLastPosInSlot( int iSlot ) const;
    
	void FastWeaponSwitch( int iWeaponSlot );
	void PlusTypeFastWeaponSwitch( int iWeaponSlot );

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	virtual	void SetSelectedSlot( int slot ) 
	{ 
		m_iSelectedSlot = slot;
	}

	void SetSelectedSlideDir( int dir )
	{
		m_iSelectedSlideDir = dir;
	}

	void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number);

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionNumbers" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );
	CPanelAnimationVar( float, m_flBlur, "Blur", "0" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxSize, "SmallBoxSize", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flMediumBoxWide, "MediumBoxWide", "75", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flMediumBoxTall, "MediumBoxTall", "50", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "54", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "0" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "0" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "SelectionEmptyBoxBg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxColor", "SelectionSelectedBoxBg" );
	CPanelAnimationVar( Color, m_SelectedFgColor, "SelectedFgColor", "FgColor" );
	CPanelAnimationVar( Color, m_BrightBoxColor, "SelectedFgColor", "BgColor" );

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	bool m_bFadingOut;

	// fastswitch weapon display
	struct WeaponBox_t
	{
		int m_iSlot;
		int m_iSlotPos;
   	};
	CUtlVector<WeaponBox_t>	m_WeaponBoxes;
	int						m_iSelectedWeaponBox;
	int						m_iSelectedSlideDir;
	int						m_iSelectedBoxPosition;
	int						m_iSelectedSlot;
	C_BaseCombatWeapon		*m_pLastWeapon;
	CPanelAnimationVar( float, m_flHorizWeaponSelectOffsetPoint, "WeaponBoxOffset", "0" );
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection(pElementName), BaseClass(NULL, "HudWeaponSelection")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bFadingOut = false;
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( C_BaseCombatWeapon *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink( void )
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;
	if ( hud_fastswitch.GetBool() )
	{
		flSelectionTimeout = FASTSWITCH_DISPLAY_TIMEOUT;
		flSelectionFadeoutTime = FASTSWITCH_FADEOUT_TIME;
	}

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		if (!m_bFadingOut)
		{
			// start fading out
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FadeOutWeaponSelectionMenu" );
			m_bFadingOut = true;
		}
		else if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			// finished fade, close
			HideSelection();
		}
	}
	else if (m_bFadingOut)
	{
		// stop us fading out, show the animation again
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "OpenWeaponSelectionMenu" );
		m_bFadingOut = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	// draw weapon selection a little longer if in fastswitch so we can see what we've selected
	if ( hud_fastswitch.GetBool() && ( gpGlobals->curtime - m_flSelectionTime ) < (FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME) )
		return true;

	return ( m_bSelectionVisible ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();

	m_iSelectedWeaponBox = -1;
	m_iSelectedSlideDir  = 0;
	m_pLastWeapon        = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: starts animating the center of the draw point to the newly selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ActivateFastswitchWeaponDisplay( C_BaseCombatWeapon *pSelectedWeapon )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// make sure all our configuration data is read
	MakeReadyForUse();

	m_WeaponBoxes.RemoveAll();
	m_iSelectedWeaponBox = 0;

	// find out where our selected weapon is in the full list
	int cWeapons = 0;
	int iLastSelectedWeaponBox = -1;
	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		for (int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++)
		{
			C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotpos );
			if ( !pWeapon )
				continue;

			WeaponBox_t box = { i, slotpos };
			m_WeaponBoxes.AddToTail( box );

			if ( pWeapon == pSelectedWeapon )
			{
				m_iSelectedWeaponBox = cWeapons;
			}
			if ( pWeapon == m_pLastWeapon )
			{
				iLastSelectedWeaponBox = cWeapons;
			}
			cWeapons++;
		}
	}

	if ( iLastSelectedWeaponBox == -1 )
	{
		// unexpected failure, no last weapon to scroll from, default to snap behavior
		m_pLastWeapon = NULL;
	}

	// calculate where we would have to start drawing for this weapon to slide into center
	float flStart, flStop, flTime;
	if ( !m_pLastWeapon || m_iSelectedSlideDir == 0 || m_flHorizWeaponSelectOffsetPoint != 0 )
	{
		// no previous weapon or weapon selected directly or selection during slide, snap to exact position
		m_pLastWeapon = pSelectedWeapon;
		flStart = flStop = flTime = 0;
	}
	else
	{
		// offset display for a scroll
		// causing selected weapon to slide into position
		// scroll direction based on user's "previous" or "next" selection
		int numIcons = 0;
		int start    = iLastSelectedWeaponBox;
		for (int i=0; i<cWeapons; i++)
		{
			// count icons in direction of slide to destination
			if ( start == m_iSelectedWeaponBox )
				break;
			if ( m_iSelectedSlideDir < 0 )
			{
				start--;
			}
			else
			{
				start++;
			}
			// handle wraparound in either direction
			start = ( start + cWeapons ) % cWeapons;
			numIcons++;
		}

		flStart = numIcons * (m_flLargeBoxWide + m_flBoxGap);
		if ( m_iSelectedSlideDir < 0 )
			flStart *= -1;
		flStop = 0;

		// shorten duration for scrolling when desired weapon is farther away
		// otherwise a large skip in the same duration causes the scroll to fly too fast
		flTime = numIcons * 0.20f;
		if ( numIcons > 1 )
			flTime *= 0.5f;
	}
	m_flHorizWeaponSelectOffsetPoint = flStart;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "WeaponBoxOffset", flStop, 0, flTime, AnimationController::INTERPOLATOR_LINEAR );

	// start the highlight after the scroll completes
	m_flBlur = 7.f;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "Blur", 0, flTime, 0.75f, AnimationController::INTERPOLATOR_DEACCEL );
}

//-----------------------------------------------------------------------------
// Purpose: starts animating the highlight for the selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ActivateWeaponHighlight( C_BaseCombatWeapon *pSelectedWeapon )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// make sure all our configuration data is read
	MakeReadyForUse();

	C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_iSelectedSlot, m_iSelectedBoxPosition );
	if ( !pWeapon )
		return;

	// start the highlight after the scroll completes
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "WeaponHighlight" );
}

//-----------------------------------------------------------------------------
// Purpose: returns an (per frame animating) alpha value for different weapon boxes
//-----------------------------------------------------------------------------
float CHudWeaponSelection::GetWeaponBoxAlpha( bool bSelected )
{
	float alpha;
	if ( bSelected )
	{
		alpha = m_flSelectionAlphaOverride;
	}
	else
	{
		alpha = m_flSelectionAlphaOverride * (m_flAlphaOverride / 255.0f);
	}
	return alpha;
}


//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::Paint()
{
	int width;
	int xpos;
	int ypos;

	if (!ShouldDraw())
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = NULL;
	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
	case HUDTYPE_CAROUSEL:
		pSelectedWeapon = pPlayer->GetActiveWeapon();
		break;
	default:
		pSelectedWeapon = GetSelectedWeapon();
		break;
	}
	if ( !pSelectedWeapon )
		return;

	bool bPushedViewport = false;
	if( hud_fastswitch.GetInt() == HUDTYPE_FASTSWITCH  || hud_fastswitch.GetInt() == HUDTYPE_PLUS )
	{
		CMatRenderContextPtr pRenderContext( materials );
		if( pRenderContext->GetRenderTarget() )
		{
			surface()->PushFullscreenViewport();
			bPushedViewport = true;
		}
	}

	// interpolate the selected box size between the small box size and the large box size
	// interpolation has been removed since there is no weapon pickup animation anymore, so it's all at the largest size
	float percentageDone = 1.0f; //min(1.0f, (gpGlobals->curtime - m_flPickupStartTime) / m_flWeaponPickupGrowTime);
	int largeBoxWide = m_flSmallBoxSize + ((m_flLargeBoxWide - m_flSmallBoxSize) * percentageDone);
	int largeBoxTall = m_flSmallBoxSize + ((m_flLargeBoxTall - m_flSmallBoxSize) * percentageDone);
	Color selectedColor;
	for (int i = 0; i < 4; i++)
	{
		selectedColor[i] = m_BoxColor[i] + ((m_SelectedBoxColor[i] - m_BoxColor[i]) * percentageDone);
	}

	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_CAROUSEL:
		{
			// carousel style - flat line of items
			ypos = 0;
			if ( m_iSelectedWeaponBox == -1 || m_WeaponBoxes.Count() <= 1 )
			{
				// nothing to do
				return;
			}
			else if ( m_WeaponBoxes.Count() < MAX_CAROUSEL_SLOTS )
			{
				// draw the selected weapon as a 1 of n style
				width = (m_WeaponBoxes.Count()-1) * (m_flLargeBoxWide+m_flBoxGap) + m_flLargeBoxWide;
				xpos  = (GetWide() - width)/2;
				for ( int i=0; i<m_WeaponBoxes.Count(); i++ )
				{
					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_WeaponBoxes[i].m_iSlot, m_WeaponBoxes[i].m_iSlotPos );
					if ( !pWeapon )
						break;

					float alpha = GetWeaponBoxAlpha( i == m_iSelectedWeaponBox );
					if ( i == m_iSelectedWeaponBox )
					{
						// draw selected in highlighted style
						DrawLargeWeaponBox( pWeapon, true, xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall, selectedColor, alpha, -1 );
					}
					else
					{
						DrawLargeWeaponBox( pWeapon, false, xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall / 1.5f, m_BoxColor, alpha, -1 );
					}

					xpos += (m_flLargeBoxWide + m_flBoxGap);
				}
			}
			else
			{
				// draw the selected weapon in the center, as a continuous scrolling carosuel
				// draw at center the current selected and all items to its right
				xpos = GetWide()/2 + m_flHorizWeaponSelectOffsetPoint - largeBoxWide/2;
				int i = m_iSelectedWeaponBox;
				while ( 1 )
				{
					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_WeaponBoxes[i].m_iSlot, m_WeaponBoxes[i].m_iSlotPos );
					if ( !pWeapon )
						break;

					float alpha;
					if ( i == m_iSelectedWeaponBox && !m_flHorizWeaponSelectOffsetPoint )
					{
						// draw selected in highlighted style
						alpha = GetWeaponBoxAlpha( true );
						DrawLargeWeaponBox( pWeapon, true, xpos, ypos, largeBoxWide, largeBoxTall, selectedColor, alpha, -1 );
					}
					else
					{
						alpha = GetWeaponBoxAlpha( false );
						DrawLargeWeaponBox( pWeapon, false, xpos, ypos, largeBoxWide, largeBoxTall / 1.5f, m_BoxColor, alpha, -1 );
					}

					// advance until past edge
					xpos += (largeBoxWide + m_flBoxGap);
					if ( xpos >= GetWide() )
						break;

					++i;
					if ( i >= m_WeaponBoxes.Count() )
					{
						// wraparound
						i = 0;
					}
				}

				// draw all items left of center
				xpos = GetWide()/2 + m_flHorizWeaponSelectOffsetPoint - (3*largeBoxWide/2 + m_flBoxGap);
				i = m_iSelectedWeaponBox - 1;
				while ( 1 )
				{
					if ( i < 0 )
					{
						// wraparound
						i = m_WeaponBoxes.Count() - 1;
					}

					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_WeaponBoxes[i].m_iSlot, m_WeaponBoxes[i].m_iSlotPos );
					if ( !pWeapon )
						break;

					float alpha;
					if ( i == m_iSelectedWeaponBox && !m_flHorizWeaponSelectOffsetPoint )
					{
						// draw selected in highlighted style
						alpha = GetWeaponBoxAlpha( true );
						DrawLargeWeaponBox( pWeapon, true, xpos, ypos, largeBoxWide, largeBoxTall, selectedColor, alpha, -1 );
					}
					else
					{
						alpha = GetWeaponBoxAlpha( false );
						DrawLargeWeaponBox( pWeapon, false, xpos, ypos, largeBoxWide, largeBoxTall / 1.5f, m_BoxColor, alpha, -1 );
					}

					// retreat until past edge
					xpos -= (largeBoxWide + m_flBoxGap);
					if ( xpos + largeBoxWide <= 0 )
						break;

					--i;
				}
			}
		}
	break;

	case HUDTYPE_PLUS:
		{
			float fCenterX, fCenterY;
			bool bBehindCamera = false;
			CHudCrosshair::GetDrawPosition( &fCenterX, &fCenterY, &bBehindCamera );

			// if the crosshair is behind the camera, don't draw it
			if( bBehindCamera )
				return;

			// bucket style
			int screenCenterX = (int) fCenterX;
			int screenCenterY = (int) fCenterY - 15; // Height isn't quite screen height, so adjust for center alignment

			// Modifiers for the four directions. Used to change the x and y offsets
			// of each box based on which bucket we're drawing. Bucket directions are
			// 0 = UP, 1 = RIGHT, 2 = DOWN, 3 = LEFT
			int xModifiers[] = { 0, 1, 0, -1, -1, 1 };
			int yModifiers[] = { -1, 0, 1, 0, 1, 1 };

			// Draw the four buckets
			for ( int i = 0; i < MAX_WEAPON_SLOTS; ++i )
			{
				// Set the top left corner so the first box would be centered in the screen.
				int xPos = screenCenterX -( m_flMediumBoxWide / 2 );
				int yPos = screenCenterY -( m_flMediumBoxTall / 2 );

				// Find out how many positions to draw - an empty position should still
				// be drawn if there is an active weapon in any slots past it.
				int lastSlotPos = -1;
				for ( int slotPos = 0; slotPos < MAX_WEAPON_POSITIONS; ++slotPos )
				{
					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotPos );
					if ( pWeapon )
					{
						lastSlotPos = slotPos;
					}
				}

				// Draw the weapons in this bucket
				for ( int slotPos = 0; slotPos <= lastSlotPos; ++slotPos )
				{
					// Offset the box position
					xPos += ( m_flMediumBoxWide + 5 ) * xModifiers[ i ];
					yPos += ( m_flMediumBoxTall + 5 ) * yModifiers[ i ];

					int boxWide = m_flMediumBoxWide;
					int boxTall = m_flMediumBoxTall;
					int x = xPos;
					int y = yPos;

					C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotPos );
					bool selectedWeapon = false;
					if ( i == m_iSelectedSlot && slotPos == m_iSelectedBoxPosition )
					{
						// This is a bit of a misnomer... we really are asking "Is this the selected slot"?
						selectedWeapon = true;
					}

					// Draw the box with the appropriate icon
					DrawLargeWeaponBox( pWeapon, 
										selectedWeapon, 
										x, 
										y, 
										boxWide, 
										boxTall, 
										selectedWeapon ? selectedColor : m_BoxColor, 
										GetWeaponBoxAlpha( selectedWeapon ), 
										-1 );
				}
			}
		}
	break;

	case HUDTYPE_BUCKETS:
		{
			// bucket style
			width = (MAX_WEAPON_SLOTS - 1) * (m_flSmallBoxSize + m_flBoxGap) + largeBoxWide;
			xpos  = (GetWide() - width) / 2;
			ypos  = 0;

			int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

			// draw the bucket set
			// iterate over all the weapon slots
			for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
			{
				if ( i == iActiveSlot )
				{
					bool bDrawBucketNumber = true;
					int iLastPos = GetLastPosInSlot( i );

					for (int slotpos = 0; slotpos <= iLastPos; slotpos++)
					{
						C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( i, slotpos );
						if ( !pWeapon )
						{
							if ( !hud_showemptyweaponslots.GetBool() )
								continue;
							DrawBox( xpos, ypos, largeBoxWide, largeBoxTall, m_EmptyBoxColor, m_flAlphaOverride, bDrawBucketNumber ? i + 1 : -1 );
						}
						else
						{
							bool bSelected = (pWeapon == pSelectedWeapon);
							DrawLargeWeaponBox( pWeapon, 
												bSelected, 
												xpos, 
												ypos, 
												largeBoxWide, 
												largeBoxTall, 
												bSelected ? selectedColor : m_BoxColor, 
												GetWeaponBoxAlpha( bSelected ), 
												bDrawBucketNumber ? i + 1 : -1 );
						}

						// move down to the next bucket
						ypos += (largeBoxTall + m_flBoxGap);
						bDrawBucketNumber = false;
					}

					xpos += largeBoxWide;
				}
				else
				{
					// check to see if there is a weapons in this bucket
					if ( GetFirstPos( i ) )
					{
						// draw has weapon in slot
						DrawBox(xpos, ypos, m_flSmallBoxSize, m_flSmallBoxSize, m_BoxColor, m_flAlphaOverride, i + 1);
					}
					else
					{
						// draw empty slot
						DrawBox(xpos, ypos, m_flSmallBoxSize, m_flSmallBoxSize, m_EmptyBoxColor, m_flAlphaOverride, -1);
					}

					xpos += m_flSmallBoxSize;
				}

				// reset position
				ypos = 0;
				xpos += m_flBoxGap;
			}
		}
	break;

	default:
		{
			// do nothing
		}
		break;
	}

	if( bPushedViewport )
	{
		surface()->PopFullscreenViewport();
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws a single weapon selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawLargeWeaponBox( C_BaseCombatWeapon *pWeapon, bool bSelected, int xpos, int ypos, int boxWide, int boxTall, Color selectedColor, float alpha, int number )
{
	Color col = bSelected ? m_SelectedFgColor : GetFgColor();
	
	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_BUCKETS:
		{
			// draw box for selected weapon
			DrawBox( xpos, ypos, boxWide, boxTall, selectedColor, alpha, number );

			// draw icon
			col[3] *= (alpha / 255.0f);
			if ( pWeapon->GetSpriteActive() )
			{
				// find the center of the box to draw in
				int iconWidth = pWeapon->GetSpriteActive()->Width();
				int iconHeight = pWeapon->GetSpriteActive()->Height();

				int x_offs = (boxWide - iconWidth) / 2;

				int y_offs;
				if ( bSelected && hud_fastswitch.GetInt() != 0 )
				{
					// place the icon aligned with the non-selected version
					y_offs = (boxTall / 1.5f - iconHeight) / 2;
				}
				else
				{
					y_offs = (boxTall - iconHeight) / 2;
				}

				if (!pWeapon->CanBeSelected())
				{
					// unselectable weapon, display as such
					col = Color(255, 0, 0, col[3]);
				}
				else if (bSelected)
				{
					// currently selected weapon, display brighter
					col[3] = alpha;

					// draw an active version over the top
					pWeapon->GetSpriteActive()->DrawSelf( xpos + x_offs, ypos + y_offs, col );
				}
				
				// draw the inactive version
				pWeapon->GetSpriteInactive()->DrawSelf( xpos + x_offs, ypos + y_offs, col );
			}
		}
		break;

	case HUDTYPE_PLUS:
	case HUDTYPE_CAROUSEL:
		{
			if ( !pWeapon )
			{
				// draw red box for an empty bubble
				if( bSelected )
				{
					selectedColor.SetColor( 255, 0, 0, 40 );
				}

				DrawBox( xpos, ypos, boxWide, boxTall, selectedColor, alpha, number );
				return;
			}
			else
			{
				// draw box for selected weapon
				DrawBox( xpos, ypos, boxWide, boxTall, selectedColor, alpha, number );
			}

			int iconWidth;
			int	iconHeight;
			int	x_offs;
			int	y_offs;

			// draw icon
			col[3] *= (alpha / 255.0f);

			if ( pWeapon->GetSpriteInactive() )
			{
				iconWidth = pWeapon->GetSpriteInactive()->Width();
				iconHeight = pWeapon->GetSpriteInactive()->Height();

				x_offs = (boxWide - iconWidth) / 2;
				if ( bSelected && HUDTYPE_CAROUSEL == hud_fastswitch.GetInt() )
				{
					// place the icon aligned with the non-selected version
					y_offs = (boxTall/1.5f - iconHeight) / 2;
				}
				else
				{
					y_offs = (boxTall - iconHeight) / 2;
				}

				if ( !pWeapon->CanBeSelected() )
				{
					// unselectable weapon, display as such
					col = Color(255, 0, 0, col[3]);
				}

				// draw the inactive version
				pWeapon->GetSpriteInactive()->DrawSelf( xpos + x_offs, ypos + y_offs, iconWidth, iconHeight, col );
			}

			if ( bSelected && pWeapon->GetSpriteActive() )
			{
				// find the center of the box to draw in
				iconWidth = pWeapon->GetSpriteActive()->Width();
				iconHeight = pWeapon->GetSpriteActive()->Height();

				x_offs = (boxWide - iconWidth) / 2;
				if ( HUDTYPE_CAROUSEL == hud_fastswitch.GetInt() )
				{
					// place the icon aligned with the non-selected version
					y_offs = (boxTall/1.5f - iconHeight) / 2;
				}
				else
				{
					y_offs = (boxTall - iconHeight) / 2;
				}

				col[3] = 255;
				for (float fl = m_flBlur; fl > 0.0f; fl -= 1.0f)
				{
					if (fl >= 1.0f)
					{
						pWeapon->GetSpriteActive()->DrawSelf( xpos + x_offs, ypos + y_offs, col );
					}
					else
					{
						// draw a percentage of the last one
						col[3] *= fl;
						pWeapon->GetSpriteActive()->DrawSelf( xpos + x_offs, ypos + y_offs, col );
					}
				}
			}
		}
		break;

	default:
		{
			// do nothing
		}
		break;
	}

	if ( HUDTYPE_PLUS == hud_fastswitch.GetInt() )
	{
		// No text in plus bucket method
		return;
	}

	// draw text
	col = m_TextColor;
	const FileWeaponInfo_t &weaponInfo = pWeapon->GetWpnData();

	if ( bSelected )
	{
		wchar_t text[128];
		wchar_t *tempString = g_pVGuiLocalize->Find(weaponInfo.szPrintName);

		// setup our localized string
		if ( tempString )
		{
#ifdef WIN32
			_snwprintf(text, sizeof(text)/sizeof(wchar_t) - 1, L"%s", tempString);
#else
			_snwprintf(text, sizeof(text)/sizeof(wchar_t) - 1, L"%S", tempString);
#endif
			text[sizeof(text)/sizeof(wchar_t) - 1] = 0;
		}
		else
		{
			// string wasn't found by g_pVGuiLocalize->Find()
			g_pVGuiLocalize->ConvertANSIToUnicode(weaponInfo.szPrintName, text, sizeof(text));
		}

		surface()->DrawSetTextColor( col );
		surface()->DrawSetTextFont( m_hTextFont );

		// count the position
		int slen = 0, charCount = 0, maxslen = 0;
		int firstslen = 0;
		{
			for (wchar_t *pch = text; *pch != 0; pch++)
			{
				if (*pch == '\n') 
				{
					// newline character, drop to the next line
					if (slen > maxslen)
					{
						maxslen = slen;
					}
					if (!firstslen)
					{
						firstslen = slen;
					}

					slen = 0;
				}
				else if (*pch == '\r')
				{
					// do nothing
				}
				else
				{
					slen += surface()->GetCharacterWidth( m_hTextFont, *pch );
					charCount++;
				}
			}
		}
		if (slen > maxslen)
		{
			maxslen = slen;
		}
		if (!firstslen)
		{
			firstslen = maxslen;
		}

		int tx = xpos + ((m_flLargeBoxWide - firstslen) / 2);
		int ty = ypos + (int)m_flTextYPos;
		surface()->DrawSetTextPos( tx, ty );
		// adjust the charCount by the scan amount
		charCount *= m_flTextScan;
		for (wchar_t *pch = text; charCount > 0; pch++)
		{
			if (*pch == '\n')
			{
				// newline character, move to the next line
				surface()->DrawSetTextPos( xpos + ((boxWide - slen) / 2), ty + (surface()->GetFontTall(m_hTextFont) * 1.1f));
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				surface()->DrawUnicodeChar(*pch);
				charCount--;
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number)
{
	BaseClass::DrawBox( x, y, wide, tall, color, normalizedAlpha / 255.0f );

	// draw the number
	if (number >= 0)
	{
		Color numberColor = m_NumberColor;
		numberColor[3] *= normalizedAlpha / 255.0f;
		surface()->DrawSetTextColor(numberColor);
		surface()->DrawSetTextFont(m_hNumberFont);
		wchar_t wch = '0' + number;
		surface()->DrawSetTextPos(x + m_flSelectionNumberXPos, y + m_flSelectionNumberYPos);
		surface()->DrawUnicodeChar(wch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);

	if ( hud_fastswitch.GetInt() == HUDTYPE_CAROUSEL )
	{
		// need bounds to be exact width for proper clipping during scroll 
		int width = MAX_CAROUSEL_SLOTS*m_flLargeBoxWide + (MAX_CAROUSEL_SLOTS-1)*m_flBoxGap;
		SetBounds( (screenWide-width)/2, y, width, screenTall - y);
	}
	else
	{
		SetBounds( x, y, screenWide - x, screenTall - y );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
	m_iSelectedBoxPosition = 0;
	m_iSelectedSlot = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
	m_bFadingOut = false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	int iLowestNextPosition = MAX_WEAPON_POSITIONS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition > iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot || (weaponSlot == iLowestNextSlot && weaponPosition < iLowestNextPosition) )
				{
					iLowestNextSlot = weaponSlot;
					iLowestNextPosition = weaponPosition;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return NULL;

	C_BaseCombatWeapon *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	int iLowestPrevPosition = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( CanBeSelectedInHUD( pWeapon ) )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition < iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot || (weaponSlot == iLowestPrevSlot && weaponPosition > iLowestPrevPosition) )
				{
					iLowestPrevSlot = weaponSlot;
					iLowestPrevPosition = weaponPosition;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = pPlayer->GetActiveWeapon();

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection(-1, -1);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlideDir( 1 );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// Play the "cycle to next weapon" sound
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = pPlayer->GetActiveWeapon();

	C_BaseCombatWeapon *pNextWeapon = NULL;
	if ( IsInSelectionMode() )
	{
		// find the next selection spot
		C_BaseCombatWeapon *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection(MAX_WEAPON_SLOTS, MAX_WEAPON_POSITIONS);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );
		SetSelectedSlideDir( -1 );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// Play the "cycle to next weapon" sound
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the # of the last weapon in the specified slot
//-----------------------------------------------------------------------------
int CHudWeaponSelection::GetLastPosInSlot( int iSlot ) const
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	int iMaxSlotPos;

	if ( !player )
		return -1;

	iMaxSlotPos = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);
		
		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() > iMaxSlotPos )
			iMaxSlotPos = pWeapon->GetPosition();
	}

	return iMaxSlotPos;
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
C_BaseCombatWeapon *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		C_BaseCombatWeapon *pWeapon = player->GetWeapon(i);

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = NULL;

	// see where we should start selection
	int iPosition = -1;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	if ( pActiveWeapon && pActiveWeapon->GetSlot() == iWeaponSlot )
	{
		// start after this weapon
		iPosition = pActiveWeapon->GetPosition();
	}

	C_BaseCombatWeapon *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, iPosition);
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, -1);
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );
	}
	else if ( pNextWeapon != pActiveWeapon )
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	if ( HUDTYPE_CAROUSEL != hud_fastswitch.GetInt() )
	{
		// kill any fastswitch display
		m_flSelectionTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PlusTypeFastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	m_pLastWeapon = NULL;
	int newSlot = m_iSelectedSlot;

	// Changing slot number does not necessarily mean we need to change the slot - the player could be
	// scrolling through the same slot but in the opposite direction. Slot pairs are 0,2 and 1,3 - so
	// compare the 0 bits to see if we're within a pair. Otherwise, reset the box to the zero position.
	if ( -1 == m_iSelectedSlot || ( ( m_iSelectedSlot ^ iWeaponSlot ) & 1 ) )
	{
		// Changing vertical/horizontal direction. Reset the selected box position to zero.
		m_iSelectedBoxPosition = 0;
		m_iSelectedSlot = iWeaponSlot;
	}
	else
	{
		// Still in the same horizontal/vertical direction. Determine which way we're moving in the slot.
		int increment = 1;
		if ( m_iSelectedSlot != iWeaponSlot )
		{
			// Decrementing within the slot. If we're at the zero position in this slot, 
			// jump to the zero position of the opposite slot. This also counts as our increment.
			increment = -1;
			if ( 0 == m_iSelectedBoxPosition )
			{
				newSlot = ( m_iSelectedSlot + 2 ) % 4;
				increment = 0;
			}
		}

		// Find out of the box position is at the end of the slot
		int lastSlotPos = -1;
		for ( int slotPos = 0; slotPos < MAX_WEAPON_POSITIONS; ++slotPos )
		{
			C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( newSlot, slotPos );
			if ( pWeapon )
			{
				lastSlotPos = slotPos;
			}
		}

		// Increment/Decrement the selected box position
		if ( m_iSelectedBoxPosition + increment <= lastSlotPos )
		{
			m_iSelectedBoxPosition += increment;
			m_iSelectedSlot = newSlot;
		}
		else
		{
			// error sound
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
			return;
		}
	}

	// Select the weapon in this position
	bool bWeaponSelected = false;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_iSelectedSlot, m_iSelectedBoxPosition );
	if ( pWeapon )
	{
		if ( pWeapon != pActiveWeapon )
		{
			// Select the new weapon
			::input->MakeWeaponSelection( pWeapon );
			SetSelectedWeapon( pWeapon );
			bWeaponSelected = true;
		}
	}

	if ( !bWeaponSelected )
	{
		// Still need to set this to make hud display appear
		SetSelectedWeapon( pPlayer->GetActiveWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot >= MAX_WEAPON_SLOTS )
		return;
	
	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	switch( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
	case HUDTYPE_CAROUSEL:
		{
			FastWeaponSwitch( iSlot );
			return;
		}
		
	case HUDTYPE_PLUS:
		{
			if ( !IsInSelectionMode() )
			{
				// open the weapon selection
				OpenSelection();
			}
				
			PlusTypeFastWeaponSwitch( iSlot );
			ActivateWeaponHighlight( GetSelectedWeapon() );
		}
		break;

	case HUDTYPE_BUCKETS:
		{
			int slotPos = 0;
			C_BaseCombatWeapon *pActiveWeapon = GetSelectedWeapon();

			// start later in the list
			if ( IsInSelectionMode() && pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
			{
				slotPos = pActiveWeapon->GetPosition() + 1;
			}

			// find the weapon in this slot
			pActiveWeapon = GetNextActivePos( iSlot, slotPos );
			if ( !pActiveWeapon )
			{
				pActiveWeapon = GetNextActivePos( iSlot, 0 );
			}
			
			if ( pActiveWeapon != NULL )
			{
				if ( !IsInSelectionMode() )
				{
					// open the weapon selection
					OpenSelection();
				}

				// Mark the change
				SetSelectedWeapon( pActiveWeapon );
				SetSelectedSlideDir( 0 );
			}
		}

	default:
		{
			// do nothing
		}
		break;
	}

	pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}
