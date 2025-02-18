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

#include "vguicenterprint.h"
#include "tf_controls.h"
#include "basemodelpanel.h"
#include "tf_arenateammenu.h"
#include <convar.h>
#include "IGameUIFuncs.h" // for key bindings
#include "hud.h" // for gEngfuncs
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "c_team.h"
#include "tf_hud_notification_panel.h"
#include "inputsystem/iinputsystem.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFArenaTeamMenu::CTFArenaTeamMenu( IViewPort *pViewPort ) : CTeamMenu( pViewPort )
{
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetVisible( false );
	SetKeyBoardInputEnabled( true );

	m_iTeamMenuKey = BUTTON_CODE_INVALID;

	m_pAutoTeamButton = new CTFTeamButton( this, "teambutton2" );
	m_pSpecTeamButton = new CTFTeamButton( this, "teambutton3" );
	m_pSpecLabel = new CExLabel( this, "TeamMenuSpectate", "" );

#ifdef _X360
	m_pFooter = new CTFFooter( this, "Footer" );
#else
	m_pCancelButton = new CExButton( this, "CancelButton", "#TF_Cancel" );
	m_pJoinAutoHintIcon = m_pJoinSpectatorsHintIcon = m_pCancelHintIcon = nullptr;
#endif

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_bRedDisabled = false;
	m_bBlueDisabled = false;

	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/HudArenaTeamMenu_SC.res" );
	}
	else
	{
		LoadControlSettings( "Resource/UI/HudArenaTeamMenu.res" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFArenaTeamMenu::~CTFArenaTeamMenu()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( ::input->IsSteamControllerActive() )
	{
		LoadControlSettings( "Resource/UI/HudArenaTeamMenu_SC.res" );

		m_pCancelHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "CancelHintIcon" ) );
		m_pJoinAutoHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinAutoHintIcon" ) );
		m_pJoinSpectatorsHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "JoinSpectatorsHintIcon" ) );

		SetMouseInputEnabled( false );
	}
	else
	{
		LoadControlSettings( "Resource/UI/HudArenaTeamMenu.res" );
		SetMouseInputEnabled( true );

		m_pCancelHintIcon = m_pJoinAutoHintIcon = m_pJoinSpectatorsHintIcon = nullptr;
	}

	Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::ShowPanel( bool bShow )
{
	if ( BaseClass::IsVisible() == bShow )
		return;
	
	if ( !gameuifuncs || !gViewPortInterface || !engine )
		return;

	if ( bShow )
	{
		if ( !C_TFPlayer::GetLocalTFPlayer() )
			return;

		if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
			 C_TFPlayer::GetLocalTFPlayer() && 
			 C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TFGameRules()->GetWinningTeam()
			 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_SPECTATOR 
	  		 && C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() != TEAM_UNASSIGNED )
		{
			SetVisible( false );

			CHudNotificationPanel *pNotifyPanel = GET_HUDELEMENT( CHudNotificationPanel );
			if ( pNotifyPanel )
			{
				pNotifyPanel->SetupNotifyCustom( "#TF_CantChangeTeamNow", "ico_notify_flag_moving", C_TFPlayer::GetLocalTFPlayer()->GetTeamNumber() );
			}

			return;
		}

		gViewPortInterface->ShowPanel( PANEL_CLASS_RED, false );
		gViewPortInterface->ShowPanel( PANEL_CLASS_BLUE, false );

		engine->CheckPoint( "TeamMenu" );

		InvalidateLayout( true, true );

		Activate();

		// get key bindings if shown
		m_iTeamMenuKey = gameuifuncs->GetButtonCodeForBind( "changeteam" );
		m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		GetFocusNavGroup().SetCurrentFocus( m_pAutoTeamButton->GetVPanel(), m_pAutoTeamButton->GetVPanel() );
		ActivateSelectIconHint( GetFocusNavGroup().GetCurrentFocus() ? GetFocusNavGroup().GetCurrentFocus()->GetTabPosition() : -1 );
	}
	else
	{
		SetVisible( false );

		if ( IsConsole() )
		{
			// Close the door behind us
			CTFArenaTeamMenu *pButton = dynamic_cast< CTFArenaTeamMenu *> ( GetFocusNavGroup().GetCurrentFocus() );
			if ( pButton )
			{
				pButton->OnCursorExited();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Activate the right selection hint icon, depending on the focus group number selected
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::ActivateSelectIconHint( int focus_group_number )
{
	if ( m_pJoinAutoHintIcon ) m_pJoinAutoHintIcon->SetVisible( false );
	if ( m_pJoinSpectatorsHintIcon ) m_pJoinSpectatorsHintIcon->SetVisible( false );

	CSCHintIcon* icon = nullptr;
	switch ( focus_group_number )
	{
	case 1: icon = m_pJoinAutoHintIcon; break;
	case 2: icon = m_pJoinSpectatorsHintIcon; break;
	}

	if ( icon )
	{
		icon->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: called to update the menu with new information
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::Update( void )
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
void CTFArenaTeamMenu::Join_Team( const CCommand &args )
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
void CTFArenaTeamMenu::LoadMapPage( const char *mapName )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::OnKeyCodePressed( KeyCode code )
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
void CTFArenaTeamMenu::OnCommand( const char *command )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( Q_stricmp( command, "vguicancel" ) )
	{
		// we're selecting a team, so make sure it's not the team we're already on before sending to the server
		if ( pLocalPlayer && ( Q_strstr( command, "jointeam " ) ) )
		{
			engine->ClientCmd( command );
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

//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CTFArenaTeamMenu::OnTick()
{
	// update the number of players on each team

	// enable or disable buttons based on team limit

	C_Team *pRed = GetGlobalTeam( TF_TEAM_RED );
	C_Team *pBlue = GetGlobalTeam( TF_TEAM_BLUE );

	if ( !pRed || !pBlue )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	CTFGameRules *pRules = TFGameRules();

	if ( !pRules )
		return;
	
	if ( m_pSpecTeamButton && m_pSpecLabel )
	{
		{
			if ( mp_allowspectators.GetBool() )
			{
				if ( !m_pSpecTeamButton->IsVisible() )
				{
					m_pSpecTeamButton->SetVisible( true );
					m_pSpecLabel->SetVisible( true );
				}
			}
			else
			{
				if ( m_pSpecTeamButton->IsVisible() )
				{
					m_pSpecTeamButton->SetVisible( false );
					m_pSpecLabel->SetVisible( false );
				}
			}
		}
	}
}