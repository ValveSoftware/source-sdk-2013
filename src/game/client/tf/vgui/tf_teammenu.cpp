//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui/IScheme.h>
#include <game/client/iviewport.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>
#include <filesystem.h>
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "clientmode_shared.h"
#include "inputsystem/iinputsystem.h"

#include "vguicenterprint.h"
#include "tf_controls.h"
#include "basemodelpanel.h"
#include "tf_teammenu.h"
#include <convar.h>
#include "IGameUIFuncs.h" // for key bindings
#include "hud.h" // for gEngfuncs
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "c_team.h"
#include "tf_hud_notification_panel.h"
#include "iinput.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTeamButton::CTFTeamButton( vgui::Panel *parent, const char *panelName ) : CExButton( parent, panelName, "" )
{
	m_szModelPanel[0] = '\0';
	m_iTeam = TEAM_UNASSIGNED;
	m_flHoverTimeToWait = -1;
	m_flHoverTime = -1;
	m_bMouseEntered = false;
	m_bTeamDisabled = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy( m_szModelPanel, inResourceData->GetString( "associated_model", "" ), sizeof( m_szModelPanel ) );
	m_iTeam = inResourceData->GetInt( "team", TEAM_UNASSIGNED );
	m_flHoverTimeToWait = inResourceData->GetFloat( "hover", -1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetDefaultColor( GetFgColor(), Color( 0, 0, 0, 0 ) );
	SetArmedColor( GetButtonFgColor(), Color( 0, 0, 0, 0 ) );
	SetDepressedColor( GetButtonFgColor(), Color( 0, 0, 0, 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::SendAnimation( const char *pszAnimation )
{
	Panel *pParent = GetParent();
	if ( pParent )
	{
		CModelPanel *pModel = dynamic_cast< CModelPanel* >( pParent->FindChildByName( m_szModelPanel ) );
		if ( pModel )
		{
			KeyValues *kvParms = new KeyValues( "SetAnimation" );
			if ( kvParms )
			{
				kvParms->SetString( "animation", pszAnimation );
				PostMessage( pModel, kvParms );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::SetDefaultAnimation( const char *pszName )
{
	Panel *pParent = GetParent();
	if ( pParent )
	{
		CModelPanel *pModel = dynamic_cast< CModelPanel* >( pParent->FindChildByName( m_szModelPanel ) );
		if ( pModel )
		{
			pModel->SetDefaultAnimation( pszName );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTeamButton::IsTeamFull()
{
	if ( TFGameRules() && TFGameRules()->IsInArenaMode() == true && tf_arena_use_queue.GetBool() == true )
		return false;

	bool bRetVal = false;

	if ( ( m_iTeam > TEAM_UNASSIGNED ) && GetParent() )
	{
		CTFTeamMenu *pTeamMenu = dynamic_cast< CTFTeamMenu* >( GetParent() );
		if ( pTeamMenu )
		{
			bRetVal = ( m_iTeam == TF_TEAM_BLUE ) ? pTeamMenu->IsBlueTeamDisabled() : pTeamMenu->IsRedTeamDisabled();
		}
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	SetMouseEnteredState( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::OnCursorExited()
{
	BaseClass::OnCursorExited();

	SetMouseEnteredState( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::SetMouseEnteredState( bool state )
{
	if ( state )
	{
		m_bMouseEntered = true;

		if ( m_flHoverTimeToWait > 0 )
		{
			m_flHoverTime = gpGlobals->curtime + m_flHoverTimeToWait;
		}
		else
		{
			m_flHoverTime = -1;
		}

		if ( m_bTeamDisabled )
		{
			SendAnimation( "enter_disabled" );
		}
		else
		{
			SendAnimation( "enter_enabled" );
		}
	}
	else
	{
		m_bMouseEntered = false;
		m_flHoverTime = -1;

		if ( m_bTeamDisabled )
		{
			SendAnimation( "exit_disabled" );
		}
		else
		{
			SendAnimation( "exit_enabled" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamButton::OnTick()
{
	if ( GetParent() && !GetParent()->IsVisible() )
		return; 

	// check to see if our state has changed
	bool bDisabled = IsTeamFull();

	if ( bDisabled != m_bTeamDisabled )
	{
		m_bTeamDisabled = bDisabled;

		if ( m_bMouseEntered )
		{
			// something has changed, so reset our state
			SetMouseEnteredState( true );
		}
		else
		{
			// the mouse isn't currently over the button, but we should update the status
			if ( m_bTeamDisabled )
			{
				SendAnimation( "idle_disabled" );
			}
			else
			{
				SendAnimation( "idle_enabled" );
			}
		}
	}

	if ( ( m_flHoverTime > 0 ) && ( m_flHoverTime < gpGlobals->curtime ) )
	{
		m_flHoverTime = -1;

		if ( m_bTeamDisabled )
		{
			SendAnimation( "hover_disabled" );
		}
		else
		{
			SendAnimation( "hover_enabled" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTeamMenu::CTFTeamMenu( IViewPort *pViewPort ) : CTeamMenu( pViewPort )
{
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );

	m_iTeamMenuKey = BUTTON_CODE_INVALID;

	m_pBlueTeamButton = new CTFTeamButton( this, "teambutton0" );
	m_pRedTeamButton = new CTFTeamButton( this, "teambutton1" );
	m_pAutoTeamButton = new CTFTeamButton( this, "teambutton2" );
	m_pSpecTeamButton = new CTFTeamButton( this, "teambutton3" );
	m_pSpecLabel = new CExLabel( this, "TeamMenuSpectate", "" );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pCancelButton = new CExButton( this, "CancelButton", "#TF_Cancel" );

	m_pHighlanderLabel = new CExLabel( this, "HighlanderLabel", ""  );
	m_pHighlanderLabelShadow = new CExLabel( this, "HighlanderLabelShadow", ""  );
	m_pTeamsFullLabel = new CExLabel( this, "TeamsFullLabel", ""  );
	m_pTeamsFullLabelShadow = new CExLabel( this, "TeamsFullLabelShadow", "" );
	m_pTeamsFullArrow = new CTFImagePanel( this, "TeamsFullArrow" );

#endif

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );

	m_bRedDisabled = false;
	m_bBlueDisabled = false;

	if ( g_pInputSystem && ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/Teammenu_SC.res" );
		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/Teammenu.res" );
		SetMouseInputEnabled( true );
	}

	ListenForGameEvent( "server_spawn" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFTeamMenu::~CTFTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
void CTFTeamMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/Teammenu_SC.res" );
		m_pCancelHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "CancelHintIcon" ) );
		m_pJoinAutoHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinAutoHintIcon" ) );
		m_pJoinBluHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinBluHintIcon" ) );
		m_pJoinRedHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinRedHintIcon" ) );
		m_pJoinSpectatorsHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinSpectatorsHintIcon" ) );

		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/Teammenu.res" );

		m_pCancelHintIcon = nullptr;
		m_pJoinAutoHintIcon = m_pJoinRedHintIcon = m_pJoinBluHintIcon = m_pJoinSpectatorsHintIcon = nullptr;

		SetMouseInputEnabled( true );
	}

	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;
	
	if ( !gameuifuncs || !gViewPortInterface || !engine )
		return;

	if ( bShow )
	{
		if ( !C_TFPlayer::GetLocalTFPlayer() )
			return;

		bool bDisallowChange = false;
		if ( C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() >= FIRST_GAME_TEAM )
		{
			const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
			if ( pMatchDesc && !pMatchDesc->BAllowTeamChange() )
			{
				bDisallowChange = true;
			}
		}

		if ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN
			   && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			   && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
	  		   && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED )
			 || TFGameRules()->State_Get() == GR_STATE_GAME_OVER
			 // [msmith] Don't allow the player to switch teams when in training.
			 || TFGameRules()->IsInTraining() 
			 // or if they are coaching
			 || C_TFPlayer::GetLocalTFPlayer()->m_bIsCoaching
			 || bDisallowChange
			)
		{
			SetVisible( false );

			CHudNotificationPanel *pNotifyPanel = GET_HUDELEMENT( CHudNotificationPanel );
			if ( pNotifyPanel )
			{
				pNotifyPanel->SetupNotifyCustom( "#TF_CantChangeTeamNow", "ico_notify_flag_moving", C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() );
			}

			return;
		}

		extern void Coaching_CheckIfEligibleForCoaching();
		Coaching_CheckIfEligibleForCoaching();

		gViewPortInterface->ShowPanel( PANEL_CLASS_RED, false );
		gViewPortInterface->ShowPanel( PANEL_CLASS_BLUE, false );

		engine->CheckPoint( "TeamMenu" );

		// Force us to reload our scheme, in case Steam Controller stuff has changed.
		InvalidateLayout( true, true );

		Activate();

		// get key bindings if shown
		m_iTeamMenuKey = gameuifuncs->GetButtonCodeForBind( "changeteam" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		switch ( C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			if ( ::input->EnableJoystickMode() )
			{
				m_pBlueTeamButton->OnCursorEntered();
				m_pBlueTeamButton->SetDefaultAnimation( "enter_enabled" );
			}
			GetFocusNavGroup().SetCurrentFocus( m_pBlueTeamButton->GetVPanel(), m_pBlueTeamButton->GetVPanel() );
			break;

		case TF_TEAM_RED:
			if ( ::input->EnableJoystickMode() )
			{
				m_pRedTeamButton->OnCursorEntered();
				m_pRedTeamButton->SetDefaultAnimation( "enter_enabled" );
			}
			GetFocusNavGroup().SetCurrentFocus( m_pRedTeamButton->GetVPanel(), m_pRedTeamButton->GetVPanel() );
			break;

		default:
			if ( ::input->EnableJoystickMode() )
			{
				m_pAutoTeamButton->OnCursorEntered();
				m_pAutoTeamButton->SetDefaultAnimation( "enter_enabled" );
			}
			GetFocusNavGroup().SetCurrentFocus( m_pAutoTeamButton->GetVPanel(), m_pAutoTeamButton->GetVPanel() );
			break;
		}

		ActivateSelectIconHint( GetFocusNavGroup().GetCurrentFocus() ? GetFocusNavGroup().GetCurrentFocus()->GetTabPosition() : -1 );
	}
	else
	{
		SetVisible( false );

		SetHighlanderTeamsFullPanels( false, true );

		if ( ::input->EnableJoystickMode() )
		{
			// Close the door behind us
			CTFTeamButton *pButton = dynamic_cast< CTFTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
			if ( pButton )
			{
				pButton->OnCursorExited();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: called to update the menu with new information
//-----------------------------------------------------------------------------
void CTFTeamMenu::Update( void )
{
	BaseClass::Update();

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) )
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", true );
		}
#else
		if ( m_pCancelButton )
		{
			m_pCancelButton->SetVisible( true );
			if ( m_pCancelHintIcon )
			{
				m_pCancelHintIcon->SetVisible( true );
			}
		}
#endif
	}
	else
	{
#ifdef _X360
		if ( m_pFooter )
		{
			m_pFooter->ShowButtonLabel( "cancel", false );
		}
#else
		if ( m_pCancelButton && m_pCancelButton->IsVisible() )
		{
			m_pCancelButton->SetVisible( false );
			if ( m_pCancelHintIcon )
			{
				m_pCancelHintIcon->SetVisible( false );
			}
		}
#endif
	}
}

#ifdef _X360
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::Join_Team( const CCommand &args )
{
	if ( args.ArgC() > 1 )
	{
		char cmd[256];
		Q_snprintf( cmd, sizeof( cmd ), "jointeam_nomenus %s", args.Arg( 1 ) );
		OnCommand( cmd );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: chooses and loads the text page to display that describes mapName map
//-----------------------------------------------------------------------------
void CTFTeamMenu::LoadMapPage( const char *mapName )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnKeyCodePressed( KeyCode code )
{
	if ( ( m_iTeamMenuKey != BUTTON_CODE_INVALID && m_iTeamMenuKey == code ) ||
		   code == KEY_XBUTTON_BACK || 
		   code == KEY_XBUTTON_B ||
		   code == STEAMCONTROLLER_B )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if ( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() != TEAM_UNASSIGNED ) )
		{
			ShowPanel( false );
		}
	}
	else if( code == KEY_SPACE || code == STEAMCONTROLLER_Y )
	{
		engine->ClientCmd( "jointeam auto" );

		ShowPanel( false );
		OnClose();
	}
	else if( code == KEY_XBUTTON_A || code == KEY_XBUTTON_RTRIGGER || code == STEAMCONTROLLER_A )
	{
		// select the active focus
		if ( GetFocusNavGroup().GetCurrentFocus() )
		{
			ipanel()->SendMessage( GetFocusNavGroup().GetCurrentFocus()->GetVPanel(), new KeyValues( "PressButton" ), GetVPanel() );
		}
	}
	else if( code == KEY_XBUTTON_RIGHT || code == KEY_XSTICK1_RIGHT || code == STEAMCONTROLLER_DPAD_RIGHT )
	{
		CTFTeamButton *pButton;
			
		pButton = dynamic_cast< CTFTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorExited();
			GetFocusNavGroup().RequestFocusNext( pButton->GetVPanel() );
		}
		else
		{
			GetFocusNavGroup().RequestFocusNext( NULL );
		}

		pButton = dynamic_cast< CTFTeamButton * > ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}

		ActivateSelectIconHint( GetFocusNavGroup().GetCurrentFocus() ? GetFocusNavGroup().GetCurrentFocus()->GetTabPosition() : -1 );
	}
	else if( code == KEY_XBUTTON_LEFT || code == KEY_XSTICK1_LEFT || code == STEAMCONTROLLER_DPAD_LEFT )
	{
		CTFTeamButton *pButton;

		pButton = dynamic_cast< CTFTeamButton *> ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorExited();
			GetFocusNavGroup().RequestFocusPrev( pButton->GetVPanel() );
		}
		else
		{
			GetFocusNavGroup().RequestFocusPrev( NULL );
		}

		pButton = dynamic_cast< CTFTeamButton * > ( GetFocusNavGroup().GetCurrentFocus() );
		if ( pButton )
		{
			pButton->OnCursorEntered();
		}

		ActivateSelectIconHint( GetFocusNavGroup().GetCurrentFocus() ? GetFocusNavGroup().GetCurrentFocus()->GetTabPosition() : -1 );
	}
	else if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a team
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnCommand( const char *command )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( Q_stricmp( command, "vguicancel" ) )
	{
		// we're selecting a team, so make sure it's not the team we're already on before sending to the server
		if ( pLocalPlayer && ( Q_strstr( command, "jointeam " ) ) )
		{
			const char *pTeam = command + Q_strlen( "jointeam " );
			int iTeam = TEAM_INVALID;

			if ( Q_stricmp( pTeam, "spectate" ) == 0 )
			{
				iTeam = TEAM_SPECTATOR;
			}
			else if ( Q_stricmp( pTeam, "red" ) == 0 )
			{
				iTeam = TF_TEAM_RED;
			}
			else if ( Q_stricmp( pTeam, "blue" ) == 0 )
			{
				iTeam = TF_TEAM_BLUE;
			}

			if ( iTeam == TF_TEAM_RED && m_bRedDisabled )
			{
				return;
			}

			if ( iTeam == TF_TEAM_BLUE && m_bBlueDisabled )
			{
				return;
			}

			// are we selecting the team we're already on?
			if ( pLocalPlayer->GetTeamNumber() != iTeam )
			{
				engine->ClientCmd( command );
			}
		}
		else if ( pLocalPlayer && ( Q_strstr( command, "jointeam_nomenus " ) ) )
		{
			engine->ClientCmd( command );
		}
	}

	BaseClass::OnCommand( command );
	ShowPanel( false );
	OnClose();
}

void CTFTeamMenu::OnClose()
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		// Clear the HIDEHUD_HEALTH bit we hackily added. Turns out prediction
		//  was restoring these bits every frame. Unfortunately, prediction
		//  is off for karts which means the spell hud item would disappear if you
		//  brought up this menu and returned.
		pLocalPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_HEALTH;
	}

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Activate the right selection hint icon, depending on the focus group number selected
//-----------------------------------------------------------------------------
void CTFTeamMenu::ActivateSelectIconHint( int focus_group_number )
{
	if ( m_pJoinAutoHintIcon ) m_pJoinAutoHintIcon->SetVisible( false );
	if ( m_pJoinBluHintIcon ) m_pJoinBluHintIcon->SetVisible( false );
	if ( m_pJoinRedHintIcon ) m_pJoinRedHintIcon->SetVisible( false );
	if ( m_pJoinSpectatorsHintIcon ) m_pJoinSpectatorsHintIcon->SetVisible( false );

	CSCHintIcon* icon = nullptr;
	switch ( focus_group_number )
	{
		case 1: icon = m_pJoinAutoHintIcon; break;
		case 2: icon = m_pJoinSpectatorsHintIcon; break;
		case 3: icon = m_pJoinBluHintIcon; break;
		case 4: icon = m_pJoinRedHintIcon; break;
	}

	if ( icon )
	{
		icon->SetVisible( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamMenu::SetHighlanderTeamsFullPanels( bool bTeamsFull, bool bForce /* = false */ )
{
	if ( m_pTeamsFullLabel )
	{
		if ( bForce || ( m_pTeamsFullLabel->IsVisible() != bTeamsFull ) )
		{
			m_pTeamsFullLabel->SetVisible( bTeamsFull );
		}
	}

	if ( m_pTeamsFullLabelShadow )
	{
		if ( bForce || ( m_pTeamsFullLabelShadow->IsVisible() != bTeamsFull ) )
		{
			m_pTeamsFullLabelShadow->SetVisible( bTeamsFull );
		}
	}

	if ( !mp_allowspectators.GetBool() )
	{
		// don't show the arrow if the server doesn't allow spectators
		bTeamsFull = false;
	}

	if ( m_pTeamsFullArrow )
	{
		if ( bForce || ( m_pTeamsFullArrow->IsVisible() != bTeamsFull ) )
		{
			m_pTeamsFullArrow->SetVisible( bTeamsFull );

			if ( bTeamsFull )
			{
				// turn on animation
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "TeamsFullArrowAnimate" );
			}
			else
			{
				// turn off animation
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "TeamsFullArrowAnimateEnd" );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CTFTeamMenu::OnTick()
{
	// update the number of players on each team

	// enable or disable buttons based on team limit

	if ( !IsVisible() )
		return;

	C_Team *pRed = GetGlobalTeam( TF_TEAM_RED );
	C_Team *pBlue = GetGlobalTeam( TF_TEAM_BLUE );

	if ( !pRed || !pBlue )
		return;

	// set our team counts
	SetDialogVariable( "bluecount", pBlue->Get_Number_Players() );
	SetDialogVariable( "redcount", pRed->Get_Number_Players() );

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	CTFGameRules *pRules = TFGameRules();

	if ( !pRules )
		return;

	bool bHighlander = pRules->IsInHighlanderMode();

	if ( m_pHighlanderLabel )
	{
		if ( m_pHighlanderLabel->IsVisible() != bHighlander )
		{
			m_pHighlanderLabel->SetVisible( bHighlander );
		}
	}

	if ( m_pHighlanderLabelShadow )
	{
		if ( m_pHighlanderLabelShadow->IsVisible() != bHighlander )
		{
			m_pHighlanderLabelShadow->SetVisible( bHighlander );
		}
	}

	// check if teams are unbalanced
	m_bRedDisabled = m_bBlueDisabled = false;

	int iHeavyTeam, iLightTeam;

	bool bUnbalanced = pRules->AreTeamsUnbalanced( iHeavyTeam, iLightTeam );
	
	int iCurrentTeam = pLocalPlayer->GetTeamNumber();

	if ( ( bUnbalanced && iHeavyTeam == TF_TEAM_RED ) || 
		 ( pRules->WouldChangeUnbalanceTeams( TF_TEAM_RED, iCurrentTeam ) ) ||
		 ( bHighlander && GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) ||
		 ( pRules->IsMannVsMachineMode() && ( GetGlobalTeam( TF_TEAM_RED )->GetNumPlayers() >= tf_mvm_defenders_team_size.GetInt() ) )	 )
	{
		m_bRedDisabled = true;
	}

	if ( ( bUnbalanced && iHeavyTeam == TF_TEAM_BLUE ) || 
		 ( pRules->WouldChangeUnbalanceTeams( TF_TEAM_BLUE, iCurrentTeam ) ) ||
		 ( bHighlander && GetGlobalTeam( TF_TEAM_BLUE )->GetNumPlayers() >= TF_LAST_NORMAL_CLASS - 1 ) ||
		 ( pRules->IsMannVsMachineMode() ) )
	{
		m_bBlueDisabled = true;
	}

	bool bTeamsFull = m_bRedDisabled && m_bBlueDisabled;
	SetHighlanderTeamsFullPanels( bHighlander && bTeamsFull );

	if ( m_pSpecTeamButton && m_pSpecLabel && m_pAutoTeamButton )
	{
		{
			if ( mp_allowspectators.GetBool() )
			{
				if ( !m_pSpecTeamButton->IsVisible() )
				{
					m_pSpecTeamButton->SetVisible( true );
					m_pSpecLabel->SetVisible( true );
				}

				if ( !m_pAutoTeamButton->IsVisible() )
				{
					m_pAutoTeamButton->SetVisible( true );
				}
			}
			else
			{
				if ( m_pSpecTeamButton->IsVisible() )
				{
					m_pSpecTeamButton->SetVisible( false );
					m_pSpecLabel->SetVisible( false );
				}

				if ( bHighlander )
				{
					if ( bTeamsFull )
					{
						if ( m_pAutoTeamButton->IsVisible() )
						{
							m_pAutoTeamButton->SetVisible( false );
						}
					}
					else
					{
						if ( !m_pAutoTeamButton->IsVisible() )
						{
							m_pAutoTeamButton->SetVisible( true );
						}
					}
				}
			}
		}
	}
}

void CTFTeamMenu::OnThink()
{
	//Always hide the health... this needs to be done every frame because a message from the server keeps resetting this.
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		pLocalPlayer->m_Local.m_iHideHUD |= HIDEHUD_HEALTH;
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFTeamMenu::FireGameEvent( IGameEvent *event )
{
	// when we are changing levels
	if ( FStrEq( event->GetName(), "server_spawn" ) )
	{
		SetHighlanderTeamsFullPanels( false, true );
	}
}
