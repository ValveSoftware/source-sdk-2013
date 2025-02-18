//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_matchmaking_dashboard_mvm_criteria.h"
#include "tf_mvm_criteria.h"
#include "tf_partyclient.h"
#include "tf_party.h"
#include "vgui_controls/CheckButton.h"

using namespace vgui;
using namespace GCSDK;

Panel* GetMvMModeSelectPanel()
{
	class CTFDashboardMvMModePanel : public CMatchMakingDashboardSidePanel,
									 public CGameEventListener
	{
	public:
		DECLARE_CLASS_SIMPLE( CTFDashboardMvMModePanel, CMatchMakingDashboardSidePanel );
		CTFDashboardMvMModePanel()
			: CMatchMakingDashboardSidePanel( NULL, "MVMModeSelect", "resource/ui/MatchMakingDashboardMvMModeSelect.res", k_eSideRight )
		{
			EditablePanel* pMannUpBox = new EditablePanel( this, "MannUpGroupBox" );
			m_pPlayMannUpButton = new CExButton( pMannUpBox, "PlayNowButton", (const char*)nullptr );

			EditablePanel* pBootCampBox = new EditablePanel( this, "PracticeGroupBox" );
			m_pPlayBootCampButton = new CExButton( pBootCampBox, "PracticeButton", (const char*)nullptr );

			ListenForGameEvent( "party_updated" );
		}

		virtual void FireGameEvent( IGameEvent *event )
		{
			if ( FStrEq( event->GetName(), "party_updated" ) )
			{
				InvalidateLayout();
			}
		}

		virtual void PerformLayout() OVERRIDE
		{
			BaseClass::PerformLayout();

			//bool bLeader = GTFPartyClient()->BControllingPartyActions();

			//m_pPlayMannUpButton->SetEnabled( bLeader );
			//m_pPlayMannUpButton->SetTooltip( bLeader ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );

			//m_pPlayBootCampButton->SetEnabled( bLeader );
			//m_pPlayBootCampButton->SetTooltip( bLeader ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );
		}

		virtual void OnCommand( const char* pszCommand ) OVERRIDE
		{
			if ( FStrEq( pszCommand, "mannup" ) ) 
			{
				PostActionSignal( new KeyValues( "PlayMvM_MannUp" ) );
				return;
			}
			else if ( FStrEq( pszCommand, "bootcamp" ) )
			{
				PostActionSignal( new KeyValues( "PlayMvM_BootCamp" ) );
				return;
			}

			BaseClass::OnCommand( pszCommand );
		}

	private:

		CExButton *m_pPlayMannUpButton;
		CExButton *m_pPlayBootCampButton;
	};

	Panel* pPanel = new CTFDashboardMvMModePanel();
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetMvMModeSelectPanel, k_eMvM_Mode_Select );

Panel* GetMvMCriteriaPanel()
{
	Panel* pPanel = new CTFDashboardMvMPanel( NULL, "MVMCriteria" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetMvMCriteriaPanel, k_eMvM_Mode_Configure );

CTFDashboardMvMPanel::CTFDashboardMvMPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName, "resource/ui/MatchMakingDashboardMvMCriteria.res", k_eSideRight )
{
	m_pCriteria = new CMVMCriteriaPanel( this, "criteria" );
	m_pMannUpQueueButton = new CExButton( this, "MannUpQueueButton", (const char*)nullptr );
	m_pBootCampQueueButton = new CExButton( this, "BootCampQueueButton", (const char*)nullptr );
	m_pLateJoinCheckButton = new CheckButton( this, "JoinLateCheckButton", (const char*)nullptr );

	ListenForGameEvent( "party_criteria_changed" );
	ListenForGameEvent( "party_queue_state_changed" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "world_status_changed" );
}

CTFDashboardMvMPanel::~CTFDashboardMvMPanel()
{
	m_pCriteria->MarkForDeletion();
}

void CTFDashboardMvMPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "party_updated" ) ||
		 FStrEq( event->GetName(), "party_criteria_changed" ) ||
		 FStrEq( event->GetName(), "party_queue_state_changed" ) ||
		 FStrEq( event->GetName(), "world_status_changed" ) )
	{
		InvalidateLayout();
	}
}

void CTFDashboardMvMPanel::SetAsActive( bool bActive )
{
	BaseClass::SetAsActive( bActive );

	if ( bActive )
	{
		InvalidateLayout();
	}
}

void CTFDashboardMvMPanel::OnCommand( const char *pszCommand )
{
	if ( FStrEq( pszCommand, "select_tour" ) )
	{
		auto uiState = GTFPartyClient()->GetLeaderUIState();
		Assert( GTFPartyClient()->BControllingPartyActions() );
		Assert( uiState.menu_step() == k_eTFSyncedMMMenuStep_MvM_Selecting_Tour );

		if ( GTFPartyClient()->BControllingPartyActions() )
		{
			ETFMatchGroup eMatchGroup = GTFPartyClient()->GetLocalUIState().match_group();
			GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_MvM_Selecting_Missions, eMatchGroup );
			m_pCriteria->WriteControls();
			UpdateFlowControlButtonsVisibility();
		}

		return;
	}
	else if ( FStrEq( pszCommand, "back" ) )
	{
		auto uiState = GTFPartyClient()->GetLeaderUIState();
		Assert( GTFPartyClient()->BControllingPartyActions() );
		Assert( uiState.menu_step() == k_eTFSyncedMMMenuStep_MvM_Selecting_Missions );

		if ( GTFPartyClient()->BControllingPartyActions() )
		{
			ETFMatchGroup eMatchGroup = GTFPartyClient()->GetLocalUIState().match_group();
			GTFPartyClient()->SetLocalUIState( k_eTFSyncedMMMenuStep_MvM_Selecting_Tour, eMatchGroup );
			m_pCriteria->WriteControls();
			UpdateFlowControlButtonsVisibility();
		}

		return;
	}
	else if ( FStrEq( pszCommand, "start_search" ) )
	{
		Assert( GTFPartyClient()->BControllingPartyActions() );
		Assert( GTFPartyClient()->GetLeaderUIState().menu_step() == k_eTFSyncedMMMenuStep_MvM_Selecting_Missions );

		if ( GTFPartyClient()->BControllingPartyActions() )
		{
			ETFMatchGroup eMatchGroup = GTFPartyClient()->GetLocalUIState().match_group();
			GTFPartyClient()->RequestQueueForMatch( eMatchGroup );
			UpdateFlowControlButtonsVisibility();
		}

		return;
	}

	BaseClass::OnCommand( pszCommand );
}

void CTFDashboardMvMPanel::OnSetNextEnabled( KeyValues* pKVParams )
{
	m_bLastHeardNextEnabledCommand = pKVParams->GetBool( "enabled" );
	UpdateNextButton();
}

void CTFDashboardMvMPanel::OnCheckButtonChecked( vgui::Panel *panel )
{
	if ( panel == m_pLateJoinCheckButton )
	{
		if ( GTFPartyClient()->BControllingPartyActions() )
		{
			GTFPartyClient()->MutLocalGroupCriteria().SetLateJoin( m_pLateJoinCheckButton->IsSelected() );
		}

		// The GC will control the true state of this checkbox
		m_pLateJoinCheckButton->SetSilentMode( true );
		m_pLateJoinCheckButton->SetSelected( false );
		m_pLateJoinCheckButton->SetSilentMode( false );
	}
}

void CTFDashboardMvMPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	UpdateFlowControlButtonsVisibility();
	UpdateNextButton();

	m_pCriteria->WriteControls();

	m_pLateJoinCheckButton->SetEnabled( GTFPartyClient()->BControllingPartyActions() );
	m_pLateJoinCheckButton->SetTooltip( GTFPartyClient()->BControllingPartyActions() ? NULL : GetDashboardTooltip( k_eSmallFont ), "#TF_Matchmaking_OnlyLeaderCanChange" );

	// Update selected state of late joins
	m_pLateJoinCheckButton->SetSilentMode( true );
	m_pLateJoinCheckButton->SetSelected( GTFPartyClient()->GetEffectiveGroupCriteria().GetLateJoin() );
	m_pLateJoinCheckButton->SetSilentMode( false );
}

void CTFDashboardMvMPanel::UpdateFlowControlButtonsVisibility()
{
	// Leader state
	ETFSyncedMMMenuStep eMenuStep = GTFPartyClient()->GetLeaderUIState().menu_step();

	// Local match group
	ETFMatchGroup eMatchGroup = GTFPartyClient()->GetLocalUIState().match_group();
	bool bMannUp = eMatchGroup == k_eTFMatchGroup_MvM_MannUp;
	bool bSelectingMissions = ( eMenuStep == k_eTFSyncedMMMenuStep_MvM_Selecting_Missions );
	bool bSelectingTour = ( eMenuStep == k_eTFSyncedMMMenuStep_MvM_Selecting_Tour );

	CUtlVector< CTFPartyClient::QueueEligibilityData_t > vecReasons;
	GTFPartyClient()->BCanQueueForMatch( bMannUp ? k_eTFMatchGroup_MvM_MannUp : k_eTFMatchGroup_MvM_Practice, vecReasons );

	if ( bSelectingMissions )
	{
		SetControlVisible( "BootCampQueueButton", !bMannUp );
		SetControlVisible( "MannUpQueueButton", bMannUp );

		wchar_t* pwszDisabledReason = vecReasons.IsEmpty() ? nullptr : vecReasons.Head().wszCantReason;

		SetupButtonAndTooltip( m_pBootCampQueueButton, FindControl< EditablePanel >( "BootCampToolTipButtonHack" ), pwszDisabledReason );
		SetupButtonAndTooltip( m_pMannUpQueueButton, FindControl< EditablePanel >( "MannUpToolTipButtonHack" ), pwszDisabledReason );

		SetControlVisible( "BootCampToolTipButtonHack", !bMannUp );
		SetControlVisible( "MannUpToolTipButtonHack", bMannUp );
	}
	else
	{
		SetControlVisible( "BootCampQueueButton", false );
		SetControlVisible( "MannUpQueueButton", false );
		SetControlVisible( "BootCampToolTipButtonHack", false );
		SetControlVisible( "MannUpToolTipButtonHack", false );
	}
		
	if ( GTFPartyClient()->BControllingPartyActions() )
	{
		SetControlVisible( "NextButton", bSelectingTour );
		SetControlVisible( "BackButton", bSelectingMissions && bMannUp );
	}
	else
	{
		// Non-leaders can never press these buttons, so don't even show them
		SetControlVisible( "NextButton", false );
		SetControlVisible( "BackButton", false );
	}

	UpdateNextButton();
}

void CTFDashboardMvMPanel::UpdateNextButton()
{
	bool bEnabled = m_bLastHeardNextEnabledCommand;

	SetControlEnabled( "NextButton", bEnabled );
}
