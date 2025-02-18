//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"
#include "tf_gc_client.h"
#include "tf_partyclient.h"
#include "tf_matchcriteria.h"
#include "tf_party.h"

#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "vgui_avatarimage.h"
#include "tf_leaderboardpanel.h"
#include "tf_lobbypanel_casual.h"
#include "tf_hud_mainmenuoverride.h"

#include "tf_lobby_container_frame_casual.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar tf_matchmaking_join_in_progress;

//-----------------------------------------------------------------------------
// Purpose: Override of the generic messagebox dialog to provide a welcome-to-competitive message
//-----------------------------------------------------------------------------
ConVar tf_casual_welcome_hide_forever( "tf_casual_welcome_hide_forever", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN );
ConVar tf_casual_welcome_hide( "tf_casual_welcome_hide", "0", FCVAR_HIDDEN );
class CTFCasualWelcomeDialog : public CTFMessageBoxDialog
{
	DECLARE_CLASS_SIMPLE( CTFCasualWelcomeDialog, CTFMessageBoxDialog );
public:
	CTFCasualWelcomeDialog()
		: CTFMessageBoxDialog( NULL, ( const char * )NULL, NULL, NULL, NULL )
	{
	}

	virtual ~CTFCasualWelcomeDialog() {};

	virtual void OnCommand( const char *command )
	{
		if ( FStrEq( "hideforever", command ) )
		{
			tf_casual_welcome_hide_forever.SetValue( 1 );
			return;
		}
		else if ( FStrEq( "show_explanations", command ) )
		{
			//CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
			// TODO BRETT: Show this elsewhere
	//		pMMOverride->GetCasualLobbyPanel()->OnCommand( command );
			OnCommand( "confirm" );
			return;
		}

		BaseClass::OnCommand( command );
	}

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel )
	{
		CheckButton* pNeverAskAgainCheckBox = FindControl< CheckButton >( "NeverShowAgainCheckBox" );
		if ( panel == pNeverAskAgainCheckBox )
		{
			tf_casual_welcome_hide_forever.SetValue( pNeverAskAgainCheckBox->IsSelected() );
		}
	}

	virtual const char *GetResFile() OVERRIDE
	{
		// FIXME controller?
		return "Resource/UI/CasualWelcomeDialog.res";
	}
};

//-----------------------------------------------------------------------------
CLobbyContainerFrame_Casual::CLobbyContainerFrame_Casual()
	: CBaseLobbyContainerFrame( "LobbyContainerFrame" )
{
	// Our internal lobby panel
	m_pContents = new CLobbyPanel_Casual( this, this );
	m_pContents->AddActionSignalTarget( this );
	AddPage( m_pContents, "#TF_Matchmaking_HeaderCasual" );
	GetPropertySheet()->SetNavToRelay( m_pContents->GetName() );
	m_pContents->SetVisible( true );

	m_pToolTip = new CMainMenuToolTip( this );
	vgui::EditablePanel* pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "Tooltip_CasualLobby" );
	pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );
}

//-----------------------------------------------------------------------------
CLobbyContainerFrame_Casual::~CLobbyContainerFrame_Casual( void )
{
}

void CLobbyContainerFrame_Casual::ShowPanel( bool bShow )
{
	if ( bShow )
	{
		if ( tf_casual_welcome_hide.GetBool() == false
		     && tf_casual_welcome_hide_forever.GetBool() == false
		     && !GTFPartyClient()->BInQueue() )
		{
			CTFCasualWelcomeDialog *pDialog = vgui::SETUP_PANEL( new CTFCasualWelcomeDialog() );

			if ( pDialog )
			{
				tf_casual_welcome_hide.SetValue( 1 );
				pDialog->Show();
			}
			else
			{
				Warning( "Failed to create CasualWelcomeDialog.  Outdated HUD?\n" );
			}
		}

		// Slam to true for casual
		tf_matchmaking_join_in_progress.SetValue( true );
		GTFPartyClient()->MutLocalGroupCriteria().SetLateJoin( true );
	}

	BaseClass::ShowPanel( bShow );
}

void CLobbyContainerFrame_Casual::OnCommand( const char *command )
{
	if ( FStrEq( command, "next" ) )
	{
		StartSearch();
		return;
	}
	else if ( FStrEq( command, "show_explanations" ) )
	{
		CExplanationPopup *pPopup = FindControl<CExplanationPopup>( "StartExplanation" );
		if ( pPopup )
		{
			pPopup->Popup();
		}
		return;
	}
	else if ( FStrEq( command, "show_maps_details_explanation" ) )
	{
		CExplanationPopup *pPopup = FindControl<CExplanationPopup>( "MapSelectionDetailsExplanation" );
		if ( pPopup )
		{
			pPopup->Popup();
		}
		return;
	}
	else if ( FStrEq( command, "restore_search_criteria" ) )
	{
		if ( GTFGCClientSystem() )
		{
			GTFPartyClient()->LoadSavedCasualCriteria();
		}
		return;
	}
	else if ( FStrEq( command, "save_search_criteria" ) )
	{
		if ( GTFGCClientSystem() )
		{
			GTFPartyClient()->SaveCasualCriteria();
		}
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
void CLobbyContainerFrame_Casual::WriteControls()
{
	// Make sure we want to be in matchmaking.  (If we don't, the frame should hide us pretty quickly.)
	// We might get an event or something right at the transition point occasionally when the UI should
	// not be visible
	if ( !GTFGCClientSystem()->BUserInModalMMUI() )
		{ return; }

	const char *pszBackButtonText = "#TF_Matchmaking_Back";
	const char *pszNextButtonText = NULL;

	if ( GCClientSystem()->BConnectedtoGC() )
	{
		if ( GTFPartyClient()->BIsPartyLeader()  )
		{
			if ( GTFPartyClient()->BInQueue() )
			{
				pszBackButtonText = "#TF_Matchmaking_CancelSearch";
			}
			else
			{
				pszBackButtonText = "#TF_Matchmaking_Back";
				pszNextButtonText = "#TF_Matchmaking_StartSearch";
			}
		}
		else
		{
			pszBackButtonText = "#TF_Matchmaking_LeaveParty";
			m_pNextButton->SetEnabled( false );
		}
	}

	m_pBackButton->SetText( pszBackButtonText );
	m_pNextButton->SetText( pszNextButtonText );
	m_pNextButton->SetVisible( pszNextButtonText != NULL );

	BaseClass::WriteControls();
}

//-----------------------------------------------------------------------------
/* static */ bool CLobbyContainerFrame_Casual::TypeCanHandleMatchGroup( ETFMatchGroup eMatchGroup )
{
	// All we know about
	return eMatchGroup == k_eTFMatchGroup_Casual_12v12;
}

//-----------------------------------------------------------------------------
bool CLobbyContainerFrame_Casual::VerifyPartyAuthorization() const
{
	// For now, there's no additional restrictions for playing casual
	return true;
}

//-----------------------------------------------------------------------------
void CLobbyContainerFrame_Casual::HandleBackPressed()
{
	if ( GTFPartyClient()->BInQueue() )
	{
		GTFPartyClient()->SetLocalUIState( k_eTFMatchmakingSyncedUIState_Configuring_Mode );
	}
	else
	{
		GTFGCClientSystem()->EndModalMM();
		ShowPanel( false );
	}

	BaseClass::HandleBackPressed();
}
