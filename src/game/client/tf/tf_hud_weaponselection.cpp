//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"
#include "hud_macros.h"

#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/EditablePanel.h>

#include "vgui/ILocalize.h"

#include <string.h>
#include "baseobject_shared.h"
#include "tf_imagepanel.h"
#include "item_model_panel.h"
#include "c_tf_player.h"
#include "c_tf_weapon_builder.h"
#include "tf_spectatorgui.h"
#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "inputsystem/iinputsystem.h"

#ifndef WIN32
#define _cdecl
#endif

#define SELECTION_TIMEOUT_THRESHOLD		2.5f	// Seconds
#define SELECTION_FADEOUT_TIME			3.0f

#define FASTSWITCH_DISPLAY_TIMEOUT		0.5f
#define FASTSWITCH_FADEOUT_TIME			0.5f

ConVar tf_weapon_select_demo_start_delay( "tf_weapon_select_demo_start_delay", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Delay after spawning to start the weapon bucket demo." );
ConVar tf_weapon_select_demo_time( "tf_weapon_select_demo_time", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Time to pulse each weapon bucket upon spawning as a new class. 0 to turn off." );

//-----------------------------------------------------------------------------
// Purpose: tf weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection( const char *pElementName );
	virtual ~CHudWeaponSelection( void );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( C_BaseCombatWeapon *pWeapon );
	virtual void SwitchToLastWeapon( void ) OVERRIDE;
	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual C_BaseCombatWeapon *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );

	virtual C_BaseCombatWeapon	*GetSelectedWeapon( void );

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void Init();
	virtual void LevelInit();
	virtual void LevelShutdown( void );

	virtual void FireGameEvent( IGameEvent *event );

	virtual void Reset(void)
	{
		CBaseHudWeaponSelection::Reset();

		// selection time is a little farther back so we don't show it when we spawn
		m_flSelectionTime = gpGlobals->curtime - ( FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME + 0.1 );
	}

	virtual void SelectSlot( int iSlot );

	void _cdecl UserCmd_Slot11( void );
	void _cdecl UserCmd_Slot12( void );

protected:
	struct SlotLayout_t
	{
		float x, y;
		float wide, tall;

	};
	void ComputeSlotLayout( SlotLayout_t *rSlot, int nActiveSlot, int nSelectionMode );

	virtual void OnThink();
	virtual void PerformLayout( void );
	virtual void PostChildPaint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	void		 DrawSelection( C_BaseCombatWeapon *pSelectedWeapon );

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

private:
	C_BaseCombatWeapon *FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);
	C_BaseCombatWeapon *FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);

	void FastWeaponSwitch( int iWeaponSlot );
	void PlusTypeFastWeaponSwitch( int iWeaponSlot, bool *pbPlaySwitchSound );
	int GetNumVisibleSlots();
	bool ShouldDrawInternal();

	virtual	void SetSelectedWeapon( C_BaseCombatWeapon *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	virtual	void SetSelectedSlot( int slot ) 
	{ 
		m_iSelectedSlot = slot;
	}

	void DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter = false );
	void DrawWeaponTexture( C_TFPlayer *pPlayer, C_BaseCombatWeapon *pWeapon, int xpos, int ypos, float flLargeBoxWide, float flLargeBoxTall );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionText" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxWide, "SmallBoxWide", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSmallBoxTall, "SmallBoxTall", "21", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxWide, "PlusStyleBoxWide", "120", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxTall, "PlusStyleBoxTall", "84", "proportional_float" );
	CPanelAnimationVar( float, m_flPlusStyleExpandPercent, "PlusStyleExpandSelected", "0.3" )

	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flRightMargin, "RightMargin", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flIconXPos, "IconXPos", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flIconYPos, "IconYPos", "8", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "54", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flErrorYPos, "ErrorYPos", "60", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "SelectionEmptyBoxBg" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxClor", "SelectionSelectedBoxBg" );

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	CPanelAnimationVar( int, m_iMaxSlots, "MaxSlots", "6" );
	CPanelAnimationVar( bool, m_bPlaySelectionSounds, "PlaySelectSounds", "1" );

	CTFImagePanel *m_pActiveWeaponBG;

	CItemModelPanel	*m_pModelPanels[MAX_WEAPON_SLOTS];


	float m_flDemoStartTime;
	float m_flDemoModeChangeTime;
	int m_iDemoModeSlot;

	// HUDTYPE_PLUS weapon display
	int						m_iSelectedBoxPosition;		// in HUDTYPE_PLUS, the position within a slot
	int						m_iSelectedSlot;			// in HUDTYPE_PLUS, the slot we're currently moving in
	CPanelAnimationVar( float, m_flHorizWeaponSelectOffsetPoint, "WeaponBoxOffset", "0" );

	int m_iActiveSlot;  // used to store the active slot to refresh the layout when using hud_fastswitch
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

DECLARE_HUD_COMMAND_NAME( CHudWeaponSelection, Slot11, "CHudWeaponSelection");
DECLARE_HUD_COMMAND_NAME( CHudWeaponSelection, Slot12, "CHudWeaponSelection");

HOOK_COMMAND( slot11, Slot11 );
HOOK_COMMAND( slot12, Slot12 );

void CHudWeaponSelection::UserCmd_Slot11(void)
{
	SelectSlot( 11 );
}
void CHudWeaponSelection::UserCmd_Slot12(void)
{
	SelectSlot( 12 );
}

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection( pElementName ), EditablePanel( NULL, "HudWeaponSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPostChildPaintEnabled( true );

	m_flDemoStartTime = -1;
	m_flDemoModeChangeTime = 0;
	m_iDemoModeSlot = -1;
	m_iActiveSlot = -1;

	ListenForGameEvent( "localplayer_changeclass" );

	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		m_pModelPanels[i] = new CItemModelPanel( this, VarArgs( "modelpanel%d", i ) );
	}
}

CHudWeaponSelection::~CHudWeaponSelection( void )
{
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
void CHudWeaponSelection::OnThink()
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;
	if ( hud_fastswitch.GetBool() || (::input->IsSteamControllerActive()) )
	{
		flSelectionTimeout = FASTSWITCH_DISPLAY_TIMEOUT;
		flSelectionFadeoutTime = FASTSWITCH_FADEOUT_TIME;
	}

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		// close
		if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			HideSelection();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	bool bShouldDraw = ShouldDrawInternal();

	if ( !bShouldDraw && m_pActiveWeaponBG && m_pActiveWeaponBG->IsVisible() )
	{
		m_pActiveWeaponBG->SetVisible( false );
	}

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		bShouldDraw = false;

	}

	if ( TFGameRules() && TFGameRules()->ShowMatchSummary() )
	{
		bShouldDraw = false;
	}

	if ( CTFMinigameLogic::GetMinigameLogic() && CTFMinigameLogic::GetMinigameLogic()->GetActiveMinigame() )
	{
		bShouldDraw = false;
	}

	return bShouldDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDrawInternal()
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

	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	if ( pPlayer->IsAlive() == false )
		return false;

	// we only show demo mode in hud_fastswitch 0
	if ( hud_fastswitch.GetInt() == 0 && !::input->IsSteamControllerActive() && ( m_iDemoModeSlot >= 0 || m_flDemoStartTime > 0 ) )
	{
		return true;
	}

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	// draw weapon selection a little longer if in fastswitch so we can see what we've selected
	if ( (hud_fastswitch.GetBool() || ::input->IsSteamControllerActive()) && ( gpGlobals->curtime - m_flSelectionTime ) < (FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME) )
		return true;

	return ( m_bSelectionVisible ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::Init()
{
	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();
	
	m_iMaxSlots = clamp( m_iMaxSlots, 0, MAX_WEAPON_SLOTS );

	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		m_pModelPanels[i]->SetVisible( false );
	}
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelShutdown( void )
{
	CHudElement::LevelShutdown();

	// Clear out our weaponry on level change
	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		if ( m_pModelPanels[i] )
		{
			m_pModelPanels[i]->SetItem( NULL );
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: Calculates how many weapons slots need to be displayed
//-------------------------------------------------------------------------
int CHudWeaponSelection::GetNumVisibleSlots()
{
	int nCount = 0;

	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		if ( GetFirstPos( i ) )
		{
			nCount++;
		}
	}

	return nCount;
}


//-----------------------------------------------------------------------------
// Purpose: Figure out where to put the item model panels for this weapon
//			selection slot layout
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ComputeSlotLayout( SlotLayout_t *rSlot, int nActiveSlot, int nSelectionMode )
{
	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	switch( nSelectionMode )
	{
	case HUDTYPE_CAROUSEL:
	case HUDTYPE_BUCKETS:
	case HUDTYPE_FASTSWITCH:
		{
			// calculate where to start drawing
			int nTotalHeight = ( nNumSlots - 1 ) * ( m_flSmallBoxTall + m_flBoxGap ) + m_flLargeBoxTall;
			int xStartPos = GetWide() - m_flBoxGap - m_flRightMargin;
			int ypos = ( GetTall() - nTotalHeight ) / 2;

			// iterate over all the weapon slots
			for ( int i = 0; i < m_iMaxSlots; i++ )
			{
				if ( i == nActiveSlot )
				{
					rSlot[i].wide = m_flLargeBoxWide;
					rSlot[i].tall = m_flLargeBoxTall;
				}
				else
				{
					rSlot[i].wide = m_flSmallBoxWide;
					rSlot[i].tall = m_flSmallBoxTall;
				}

				rSlot[i].x = xStartPos - ( rSlot[i].wide + m_flBoxGap );
				rSlot[i].y = ypos;

				ypos += ( rSlot[i].tall + m_flBoxGap );	
			}
		}
		break;

	case HUDTYPE_PLUS:
		{
			// bucket style
			int screenCenterX = GetWide() / 2;
			int screenCenterY = GetTall() / 2; // Height isn't quite screen height, so adjust for center alignement

			// Modifiers for the four directions. Used to change the x and y offsets
			// of each box based on which bucket we're drawing. Bucket directions are
			// 0 = UP, 1 = RIGHT, 2 = DOWN, 3 = LEFT

			int xModifiers[] = { 0, 1, 0, -1, -1, 1 };
			int yModifiers[] = { -1, 0, 1, 0, 1, 1 };

			int boxWide = m_flPlusStyleBoxWide;
			int boxTall = m_flPlusStyleBoxTall;
			int boxWideSelected = m_flPlusStyleBoxWide * ( 1.f + m_flPlusStyleExpandPercent );
			int boxTallSelected = m_flPlusStyleBoxTall * ( 1.f + m_flPlusStyleExpandPercent );

			// Draw the four buckets
			for ( int i = 0; i < m_iMaxSlots; ++i )
			{
				if( i == nActiveSlot )
				{
					rSlot[i].wide = boxWideSelected;
					rSlot[i].tall = boxTallSelected;
				}
				else
				{
					rSlot[i].wide = boxWide;
					rSlot[i].tall = boxTall;
				}

				// Set the top left corner so the first box would be centered in the screen.
				int xPos = screenCenterX -( rSlot[i].wide / 2 );
				int yPos = screenCenterY -( rSlot[i].tall / 2 );

				// Offset the box position
				rSlot[ i ].x = xPos + ( rSlot[i].wide + 5 ) * xModifiers[ i ];
				rSlot[ i ].y = yPos + ( rSlot[i].tall + 5 ) * yModifiers[ i ];
			}
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = NULL;
	int fastswitch = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		fastswitch = HUDTYPE_FASTSWITCH;
	}

	switch ( fastswitch )
	{
	case HUDTYPE_FASTSWITCH:
		pSelectedWeapon = pPlayer->GetActiveWeapon();
		break;
	default:
		pSelectedWeapon = GetSelectedWeapon();
		break;
	}
	if ( !pSelectedWeapon )
		return;


	// calculate where to start drawing

	int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

	SlotLayout_t rSlot[ MAX_WEAPON_SLOTS ];
	ComputeSlotLayout( rSlot, iActiveSlot, fastswitch );

	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		m_pModelPanels[i]->SetVisible( false );

		if ( i == iActiveSlot )
		{
			for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
			{
				C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos);
				if ( !pWeapon )
					continue;

				if ( !pWeapon->VisibleInWeaponSelection() )
					continue;

				m_pModelPanels[i]->SetItem( pWeapon->GetAttributeContainer()->GetItem() );

				m_pModelPanels[i]->SetSize( rSlot[i].wide, rSlot[ i ].tall );

				vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
				if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
				{
					m_pModelPanels[i]->SetBorder( pScheme->GetBorder("TFFatLineBorderBlueBG") );
				}
				else
				{
					m_pModelPanels[i]->SetBorder( pScheme->GetBorder("TFFatLineBorderRedBG") );
				}

				m_pModelPanels[i]->SetPos( rSlot[i].x, rSlot[ i ].y );
				m_pModelPanels[i]->SetVisible( true );
			}
		}
		else
		{
			// check to see if there is a weapons in this bucket
			if ( GetFirstPos( i ) )
			{
				C_BaseCombatWeapon *pWeapon = GetFirstPos( i );
				if ( !pWeapon )
					continue;

				m_pModelPanels[i]->SetItem( pWeapon->GetAttributeContainer()->GetItem() );
				
				m_pModelPanels[i]->SetSize( rSlot[i].wide, rSlot[ i ].tall );
				vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
				m_pModelPanels[i]->SetBorder( pScheme->GetBorder("TFFatLineBorder") );
				m_pModelPanels[i]->SetVisible( true );
				m_pModelPanels[i]->SetPos( rSlot[i].x, rSlot[ i ].y );
			}
		}
	}
}

//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::PostChildPaint()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	int fastswitch = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		fastswitch = HUDTYPE_FASTSWITCH;
	}

	if ( fastswitch == 0 )
	{
		// See if we should start the bucket demo
		if ( m_flDemoStartTime > 0 && m_flDemoStartTime < gpGlobals->curtime )
		{
			float flDemoTime = tf_weapon_select_demo_time.GetFloat();

			if ( flDemoTime > 0 )
			{
				m_iDemoModeSlot = 0;
				m_flDemoModeChangeTime = gpGlobals->curtime + flDemoTime;
				gHUD.LockRenderGroup( gHUD.LookupRenderGroupIndexByName( "weapon_selection" ) );
			}

			m_flDemoStartTime = -1;
			m_iSelectedSlot = m_iDemoModeSlot;

			InvalidateLayout();
		}

		// scroll through the slots for demo mode
		if ( m_iDemoModeSlot >= 0 && m_flDemoModeChangeTime < gpGlobals->curtime )
		{
			// Keep iterating until we find a slot that has a weapon in it
			while ( !GetFirstPos( ++m_iDemoModeSlot ) && m_iDemoModeSlot < m_iMaxSlots )
			{
				// blank
			}			
			m_flDemoModeChangeTime = gpGlobals->curtime + tf_weapon_select_demo_time.GetFloat();
			InvalidateLayout();
		}

		if ( m_iDemoModeSlot >= m_iMaxSlots )
		{
			m_iDemoModeSlot = -1;
			gHUD.UnlockRenderGroup( gHUD.LookupRenderGroupIndexByName( "weapon_selection" ) );
		}
	}	

	// find and display our current selection
	C_BaseCombatWeapon *pSelectedWeapon = NULL;
	switch ( fastswitch )
	{
	case HUDTYPE_FASTSWITCH:
		pSelectedWeapon = pPlayer->GetActiveWeapon();
		break;
	default:
		pSelectedWeapon = GetSelectedWeapon();
		break;
	}
	if ( !pSelectedWeapon )
		return;

	if ( fastswitch == 0 )
	{
		if ( m_iDemoModeSlot > -1 )
		{
			pSelectedWeapon = GetWeaponInSlot( m_iDemoModeSlot, 0 );
			m_iSelectedSlot = m_iDemoModeSlot;
			m_iSelectedBoxPosition = 0;
		}
	}

	if ( m_pActiveWeaponBG )
	{
		m_pActiveWeaponBG->SetVisible( fastswitch != HUDTYPE_PLUS && pSelectedWeapon != NULL );
	}

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	DrawSelection( pSelectedWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: Draws the vertical style weapon selection buckets, for PC/mousewheel controls
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawSelection( C_BaseCombatWeapon *pSelectedWeapon )
{
	// if we're not supposed to draw the selection, the don't draw the selection
	if( !m_bSelectionVisible )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	// calculate where to start drawing
	int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	if ( nFastswitchMode == HUDTYPE_FASTSWITCH )
	{
		if ( m_iActiveSlot != iActiveSlot )
		{
			m_iActiveSlot = iActiveSlot;
			InvalidateLayout( true );
		}
	}

	// draw the bucket set
	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		int xpos, ypos;
		m_pModelPanels[i]->GetPos( xpos, ypos );
		
		int wide, tall;
		m_pModelPanels[i]->GetSize( wide, tall );

		if ( i == iActiveSlot )
		{
			bool bFirstItem = true;
			for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
			{
				C_BaseCombatWeapon *pWeapon = GetWeaponInSlot(i, slotpos);
				if ( !pWeapon )
					continue;

				if ( !pWeapon->VisibleInWeaponSelection() )
					continue;

				if ( !pWeapon->CanBeSelected() )
				{
					int msgX = xpos + ( m_flLargeBoxWide * 0.5 );
					int msgY = ypos + (int)m_flErrorYPos;
					Color ammoColor = Color( 255, 0, 0, 255 );
					wchar_t *pText = g_pVGuiLocalize->Find( "#TF_OUT_OF_AMMO" );
					DrawString( pText, msgX, msgY, ammoColor, true );
				}

				if ( pWeapon == pSelectedWeapon || ( m_iDemoModeSlot == i ) )
				{
					// draw the number
					int shortcut = bFirstItem ? i + 1 : -1;
					if ( IsPC() && shortcut >= 0 && nFastswitchMode != HUDTYPE_PLUS )
					{
						Color numberColor = m_NumberColor;
						numberColor[3] *= m_flSelectionAlphaOverride / 255.0f;
						surface()->DrawSetTextColor(numberColor);
						surface()->DrawSetTextFont(m_hNumberFont);
						wchar_t wch = '0' + shortcut;
						surface()->DrawSetTextPos( xpos + wide - XRES(5) - m_flSelectionNumberXPos, ypos + YRES(5) + m_flSelectionNumberYPos );
						surface()->DrawUnicodeChar(wch);
					}
				}
				bFirstItem = false;
			}
		}
		else
		{
			// check to see if there is a weapons in this bucket
			if ( GetFirstPos( i ) )
			{
				C_BaseCombatWeapon *pWeapon = GetFirstPos( i );
				if ( !pWeapon )
					continue;

				// draw the number
				if ( IsPC() && nFastswitchMode != HUDTYPE_PLUS )
				{
					int x = xpos + XRES(5);
					int y = ypos + YRES(5);

					Color numberColor = m_NumberColor;
					numberColor[3] *= m_flAlphaOverride / 255.0f;
					surface()->DrawSetTextColor(numberColor);
					surface()->DrawSetTextFont(m_hNumberFont);
					wchar_t wch = '0' + i + 1;
					surface()->DrawSetTextPos(x + m_flSmallBoxWide - XRES(10) - m_flSelectionNumberXPos, y + m_flSelectionNumberYPos);
					surface()->DrawUnicodeChar(wch);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawWeaponTexture( C_TFPlayer *pPlayer, C_BaseCombatWeapon *pWeapon, int xpos, int ypos, float flLargeBoxWide, float flLargeBoxTall )
{
	// draw icon
	const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
	if ( pPlayer )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
		{
			pTexture = pWeapon->GetSpriteActive();
		}
	}

	if ( pTexture )
	{
		Color col( 255, 255, 255, 255 );
		pTexture->DrawSelf( xpos, ypos, flLargeBoxWide, flLargeBoxTall, col );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter )
{
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextFont( m_hTextFont );

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
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

	int x = xpos;

	if ( bCenter )
	{
		x = xpos - slen * 0.5;
	}

	surface()->DrawSetTextPos( x, ypos );
	// adjust the charCount by the scan amount
	charCount *= m_flTextScan;
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos( x + ((m_flLargeBoxWide - slen) / 2), ypos + (surface()->GetFontTall(m_hTextFont) * 1.1f));
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
	SetBounds(0, 0, screenWide, screenTall);

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSelection.res" );

	m_pActiveWeaponBG = dynamic_cast<CTFImagePanel*>( FindChildByName("ActiveWeapon") );
	if ( m_pActiveWeaponBG )
	{
		m_pActiveWeaponBG->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	InvalidateLayout();

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
	for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
	{
		if ( m_pModelPanels[i] )
		{
			m_pModelPanels[i]->SetVisible( false );
		}
	}

	m_flSelectionTime = 0;
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
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

		if ( pWeapon->VisibleInWeaponSelection() )
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

		if ( pWeapon->VisibleInWeaponSelection() )
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

	if ( pPlayer->IsAlive() == false )
		return;

	// PASSTIME don't CycleToNextWeapon if it's not allowed
	if ( !pPlayer->IsAllowedToSwitchWeapons() )
		return;

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

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		InvalidateLayout();

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
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

	if ( pPlayer->IsAlive() == false )
		return;

	// PASSTIME don't CycleToNextWeapon if it's not allowed
	if ( !pPlayer->IsAllowedToSwitchWeapons() )
		return;

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

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		InvalidateLayout();

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
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

C_BaseCombatWeapon *CHudWeaponSelection::GetSelectedWeapon( void )
{ 
	if ( hud_fastswitch.GetInt() == 0 && !::input->IsSteamControllerActive() && m_iDemoModeSlot >= 0 )
	{
		C_BaseCombatWeapon *pWeapon = GetFirstPos( m_iDemoModeSlot );
		return pWeapon;
	}
	else
	{
		return m_hSelectedWeapon;
	}
}

void CHudWeaponSelection::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "localplayer_changeclass") == 0 )
	{
		for ( int i = 0; i < MAX_WEAPON_SLOTS; i++ )
		{
			if ( m_pModelPanels[i] )
			{
				m_pModelPanels[i]->SetVisible( false );
			}
		}

		int nUpdateType = event->GetInt( "updateType" );
		bool bIsCreationUpdate = ( nUpdateType == DATA_UPDATE_CREATED );
		// Don't demo selection in minmode
		ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
		if ( !cl_hud_minmode.IsValid() || cl_hud_minmode.GetBool() == false )
		{
			if ( !bIsCreationUpdate )
			{
				m_flDemoStartTime = gpGlobals->curtime + tf_weapon_select_demo_start_delay.GetFloat();
			}
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
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

	// kill any fastswitch display
	m_flSelectionTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PlusTypeFastWeaponSwitch( int iWeaponSlot, bool *pbPlaySwitchSound )
{
	// get the slot the player's weapon is in
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

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
			*pbPlaySwitchSound = false;
			return;
		}
	}

	// Select the weapon in this position
	bool bWeaponSelected = false;
	C_BaseCombatWeapon *pActiveWeapon = pPlayer->GetActiveWeapon();
	C_BaseCombatWeapon *pWeapon = GetWeaponInSlot( m_iSelectedSlot, m_iSelectedBoxPosition );
	if ( pWeapon && CanBeSelectedInHUD( pWeapon ) )
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

	bool bPlaySwitchSound = true;
	int nFastswitchMode = hud_fastswitch.GetInt();
	if ( ::input->IsSteamControllerActive() )
	{
		nFastswitchMode = HUDTYPE_FASTSWITCH;
	}

	switch( nFastswitchMode )
	{
	case HUDTYPE_FASTSWITCH:
		{
			FastWeaponSwitch( iSlot );
			return;
		}

	case HUDTYPE_PLUS:
		PlusTypeFastWeaponSwitch( iSlot, &bPlaySwitchSound );

		// ------------------------------------------------------
		// FALLTHROUGH! Plus and buckets both use the item model
		// panels so fix them up in both cases.
		// ------------------------------------------------------


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

				InvalidateLayout();

				// Mark the change
				SetSelectedWeapon( pActiveWeapon );
				m_iDemoModeSlot = -1;
				m_flDemoStartTime = -1;
			}
		}
		break;

	default:
		break;
	}

	if( m_bPlaySelectionSounds && bPlaySwitchSound )
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}

//-----------------------------------------------------------------------------
// Purpose: Menu Selection Code
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectSlot( int iSlot )
{
	// A menu may be overriding weapon selection commands
	if ( HandleHudMenuInput( iSlot ) )
	{
		return;
	}

	// If we're in observer mode, see if the spectator GUI wants to use it
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsObserver() )
	{
		CTFSpectatorGUI *pPanel = (CTFSpectatorGUI*)gViewPortInterface->FindPanelByName( PANEL_SPECGUI );
		if ( pPanel )
		{
			pPanel->SelectSpec( iSlot );
		}
		return;
	}

	// If we're not allowed to draw, ignore weapon selections
	if ( !CHudElement::ShouldDraw() )
	{
		return;
	}

	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	if ( !IsInSelectionMode() && ( iSlot - 1 >= MAX_WEAPON_SLOTS ) )
	{
		OpenSelection();
	}

	UpdateSelectionTime();
	SelectWeaponSlot( iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: Menu Selection Code
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SwitchToLastWeapon()
{
	C_TFPlayer *pTFPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pTFPlayer )
		return;

	if (TFGameRules() && TFGameRules()->IsPasstimeMode() && pTFPlayer->m_Shared.HasPasstimeBall() )
		return;

	CBaseHudWeaponSelection::SwitchToLastWeapon();
}
