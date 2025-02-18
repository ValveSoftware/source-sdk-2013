//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ping_panel.h"

#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/cvartogglecheckbutton.h"
#include "vgui_controls/ComboBox.h"

#include "tf_gc_client.h"
#include "tf_partyclient.h"

#include "clientmode_tf.h"

#include "tf_matchmaking_shared.h"
#include "tf_matchmaking_dashboard.h"

Panel* GetDashboardPingPanel()
{
	// Force to 12v12.  It's got the most players
	CTFPingPanel* pPanel = new CTFPingPanel( NULL, "PingPanel", k_eTFMatchGroup_Casual_12v12 );
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}

REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetDashboardPingPanel, k_eMMSettings );


CTFPingPanel::CTFPingPanel( Panel* pPanel, const char *pszName, ETFMatchGroup eMatchGroup )
	: CMatchMakingDashboardSidePanel( pPanel, pszName, "resource/ui/MatchMakingPingPanel.res", k_eSideLeft ),
	m_eMatchGroup( eMatchGroup )
{
	SetProportional( true );

	m_pCurrentPingLabel = new Label( this, "CurrentPingLabel", "" );
	m_pPingSlider = new CCvarSlider( this, "PingSlider" );
	m_pInviteModeComboBox = new ComboBox( this, "InviteModeComboBox", 3, false );
	m_pInviteModeComboBox->AddItem( "#TF_MM_InviteMode_Open", new KeyValues( NULL, "mode", CTFPartyClient::k_ePartyJoinRequestMode_OpenToFriends ) );
	m_pInviteModeComboBox->AddItem( "#TF_MM_InviteMode_Invite", new KeyValues( NULL, "mode", CTFPartyClient::k_ePartyJoinRequestMode_FriendsCanRequestToJoin ) );
	m_pInviteModeComboBox->AddItem( "#TF_MM_InviteMode_Closed", new KeyValues( NULL, "mode", CTFPartyClient::k_ePartyJoinRequestMode_ClosedToFriends ) );
	m_pInviteModeComboBox->SilentActivateItemByRow( GTFPartyClient()->GetPartyJoinRequestMode() );
	m_pInviteModeComboBox->SetEditable( false );

	ListenForGameEvent( "ping_updated" );
	ListenForGameEvent( "mmstats_updated" );
	ListenForGameEvent( "party_pref_changed" );
}


CTFPingPanel::~CTFPingPanel()
{
	CleanupPingPanels();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pCustomPingCheckBox = FindControl< CvarToggleCheckButton<UIConVarRef> >( "CustomPingCheckButton", true );
	m_pIgnoreInvitesCheckBox = FindControl< CvarToggleCheckButton<UIConVarRef> >( "IgnorePartyInvites", true );
	m_pKeepTeamTogetherCheckBox = FindControl< CvarToggleCheckButton<UIConVarRef> >( "KeepPartyOnSameTeam", true );

	if ( m_pKeepTeamTogetherCheckBox )
	{
		m_pKeepTeamTogetherCheckBox->SetSelected( true );
		m_pKeepTeamTogetherCheckBox->SetEnabled( false );
		m_pKeepTeamTogetherCheckBox->SetTooltip( GetDashboardTooltip( k_eMediumFont ), "#TF_MM_ComingSoon" );
	}

	RegeneratePingPanels();
	m_pPingSlider->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
void CTFPingPanel::RegeneratePingPanels()
{
	CleanupPingPanels();
	CScrollableList *pDataCenterList = FindControl< CScrollableList >( "DataCenterList", true );

	static const wchar_t *s_pwszPingFormat = L"%ls (%d ms)";

	if ( pDataCenterList && GTFGCClientSystem()->BHavePingData() )
	{
		auto pingData = GTFGCClientSystem()->GetPingData();
		const auto& dictDataCenterPopulations = GTFGCClientSystem()->GetDataCenterPopulationRatioDict( m_eMatchGroup );

		bool bTesting = false;

		// for each ping data, check for intersection with data center population from MMStats
		for ( int iPing=0; iPing<pingData.pingdata_size(); ++iPing )
		{
			auto pingEntry = pingData.pingdata( iPing );
			if ( pingEntry.ping_status() != CMsgGCDataCenterPing_Update_Status_Normal )
				continue;

			const char *pszPingFromDataCenterName = pingEntry.name().c_str();
			auto dictIndex = dictDataCenterPopulations.Find( pszPingFromDataCenterName );
			// found intersection. add a population health panel
			if ( dictIndex != dictDataCenterPopulations.InvalidIndex() || bTesting)
			{
				// Load control settings
				EditablePanel* pDataCenterPopulationPanel = new EditablePanel( pDataCenterList, "DataCenterPopulationPanel" );
				pDataCenterPopulationPanel->LoadControlSettings( "resource/ui/MatchMakingDataCenterPopulationPanel.res" );
				pDataCenterPopulationPanel->SetAutoDelete( false );
				pDataCenterList->ResetScrollAmount();
				pDataCenterList->InvalidateLayout();

				// Update label
				wchar_t wszDataCenterName[ 128 ];
				wchar_t* pwszLocalizedDataCenterName = g_pVGuiLocalize->Find( CFmtStr( "#TF_DataCenter_%s", pszPingFromDataCenterName ) );
				if ( pwszLocalizedDataCenterName )
				{
					V_wcsncpy( wszDataCenterName, pwszLocalizedDataCenterName, sizeof( wszDataCenterName ) );
				}
				else
				{
					// Fallback is no token.  If you hit this, go add a string for this data center in tf_english!
					Assert( false );
					g_pVGuiLocalize->ConvertANSIToUnicode( pszPingFromDataCenterName, wszDataCenterName, sizeof( wszDataCenterName ) );
				}

				wchar_t wszLabelText[ 128 ];
				V_snwprintf( wszLabelText, V_ARRAYSIZE( wszLabelText ), s_pwszPingFormat, wszDataCenterName, pingEntry.ping() );

				pDataCenterPopulationPanel->SetDialogVariable( "datacenter_name", wszLabelText );

				PingPanelInfo panelInfo;
				panelInfo.m_pPanel = pDataCenterPopulationPanel;
				panelInfo.m_flPopulationRatio = bTesting ? RandomFloat( 0.f, 1.f ) : dictDataCenterPopulations[dictIndex];
				panelInfo.m_nPing = pingEntry.ping();
				m_vecDataCenterPingPanels.AddToTail( panelInfo );
			}
		}
	}

	struct PingPanelInfoSorter
	{
		static int SortPingPanelInfo( const PingPanelInfo* a, const PingPanelInfo* b )
		{
			return a->m_nPing - b->m_nPing;
		}
	};
	m_vecDataCenterPingPanels.Sort( &PingPanelInfoSorter::SortPingPanelInfo );

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	FOR_EACH_VEC( m_vecDataCenterPingPanels, i )
	{
		const PingPanelInfo& info = m_vecDataCenterPingPanels[i];
		int iTall = info.m_pPanel->GetTall();
		int iYGap = i > 0 ? m_iDataCenterYSpace : 0;
		int iXPos = info.m_pPanel->GetXPos();
		info.m_pPanel->SetPos( iXPos, m_iDataCenterY + iYGap + i * iTall );

		// Update bars with latest health data
		ProgressBar* pProgress = info.m_pPanel->FindControl< ProgressBar >( "HealthProgressBar", true );
		if ( pProgress )
		{
			auto healthData = GTFGCClientSystem()->GetHealthBracketForRatio( info.m_flPopulationRatio );

			pProgress->MakeReadyForUse();
			pProgress->SetProgress( healthData.m_flRatio );
			pProgress->SetFgColor( healthData.m_colorBar );
		}
	}

	UpdateCurrentPing();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "close" ) )
	{
		MarkForDeletion();
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
void CTFPingPanel::OnThink()
{
	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( FStrEq( pszEventName, "ping_updated" ) || FStrEq( pszEventName, "mmstats_updated") )
	{
		RegeneratePingPanels();
	}
	else if ( FStrEq( pszEventName, "party_pref_changed" ) )
	{
		// Party preferences changed, make sure UI is in sync (they can change by e.g. manual convar setting)
		//
		// The checkboxes are the magical bound-to-the-underlying-convar things so we don't need to touch them.
		if ( m_pInviteModeComboBox )
			{ m_pInviteModeComboBox->SilentActivateItemByRow( GTFPartyClient()->GetPartyJoinRequestMode() ); }
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::CleanupPingPanels()
{
	FOR_EACH_VEC( m_vecDataCenterPingPanels, i )
	{
		m_vecDataCenterPingPanels[i].m_pPanel->MarkForDeletion();
	}

	m_vecDataCenterPingPanels.Purge();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::UpdateCurrentPing()
{
	uint32_t unCustomPingLimit = GTFPartyClient()->GetLocalGroupCriteria().GetCustomPingTolerance();
	bool bUsePingLimit = ( unCustomPingLimit > 0 );
	int nLowestDataCenterPing = m_vecDataCenterPingPanels.Count() ? m_vecDataCenterPingPanels[0].m_nPing : 0;
	int nCurrentPingLimit = MAX( (int)m_pPingSlider->GetSliderValue(), nLowestDataCenterPing );
	m_pCurrentPingLabel->SetText( bUsePingLimit
								  ? LocalizeNumberWithToken( "#TF_MM_PingSetting", nCurrentPingLimit )
								  : g_pVGuiLocalize->Find( "#TF_MM_PingSetting_Auto" ) );

	FOR_EACH_VEC( m_vecDataCenterPingPanels, i )
	{
		const PingPanelInfo& info = m_vecDataCenterPingPanels[i];

		int bHighLight = !bUsePingLimit || info.m_nPing <= nCurrentPingLimit;
		if ( bHighLight )
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( info.m_pPanel, "HealthProgressBar_NotSelected" );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( info.m_pPanel, "HealthProgressBar_Selected" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( info.m_pPanel, "HealthProgressBar_Selected" );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( info.m_pPanel, "HealthProgressBar_NotSelected" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::OnTextChanged( vgui::Panel *panel )
{
	if ( panel == m_pInviteModeComboBox )
	{
		using EPartyJoinRequestMode = CTFPartyClient::EPartyJoinRequestMode;
		EPartyJoinRequestMode eMode = (EPartyJoinRequestMode)m_pInviteModeComboBox->GetActiveItemUserData()->GetInt( "mode" );
		GTFPartyClient()->SetPartyJoinRequestMode( eMode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::OnCheckButtonChecked( vgui::Panel *panel )
{
	if ( m_pCustomPingCheckBox == panel )
		m_pCustomPingCheckBox->ApplyChanges();
	if ( m_pIgnoreInvitesCheckBox == panel )
		m_pIgnoreInvitesCheckBox->ApplyChanges();
	if ( m_pKeepTeamTogetherCheckBox == panel )
		m_pKeepTeamTogetherCheckBox->ApplyChanges();

	m_pPingSlider->SetVisible( GTFPartyClient()->GetLocalGroupCriteria().GetCustomPingTolerance() > 0 );

	UpdateCurrentPing();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPingPanel::OnSliderMoved()
{
	UpdateCurrentPing();
}
