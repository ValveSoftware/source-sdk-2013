//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_match_description.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_matchmaking_dashboard_event.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_partyclient.h"
#include "tf_pvp_rank_panel.h"
#include "tf_badge_panel.h"

using namespace vgui;
using namespace GCSDK;

class CTFDashboardEventPanel : public CMatchMakingDashboardSidePanel
							 , public CGameEventListener
{

	DECLARE_CLASS_SIMPLE( CTFDashboardEventPanel, CMatchMakingDashboardSidePanel );

	CTFDashboardEventPanel( Panel *parent, const char *panelName )
		: BaseClass( parent, panelName, "resource/ui/MatchMakingDashboardEventMatch.res", k_eSideRight )
	{
		ListenForGameEvent( "party_criteria_changed" );
		ListenForGameEvent( "party_queue_state_changed" );
		ListenForGameEvent( "world_status_changed" );
	}

	virtual ~CTFDashboardEventPanel()
	{}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		UpdateFromWorldStatus();
	}

	virtual void OnCommand( const char *command ) OVERRIDE
	{
		if ( FStrEq( "show_explanations", command ) )
		{
			ShowDashboardExplanation( "CompIntro" );
			return;
		}
		else if ( FStrEq( command, "find_game" ) )
		{
			ETFMatchGroup eEventMatchGroup = GTFGCClientSystem()->WorldStatus().event_match_group();
			auto pMatchGroup = GetMatchGroupDescription( eEventMatchGroup );
			// Match groups that have strict abandons pop up a dialog to confirm that you want to queue.  This helps
			// inform players that they need to be ready to commit a lot of time.
			if ( pMatchGroup && pMatchGroup->BUsesStrictAbandons() )
			{
				ShowConfirmDialog( "#TF_MM_Disconnect_Title",
								   "#TF_Matchmaking_AbandonQueuePrompt",
								   "#TF_OK",
								   "#Cancel",
								   []( bool bConfirmed, void* pContext )
				{
					if ( bConfirmed )
					{
						ETFMatchGroup eEventMatchGroup = GTFGCClientSystem()->WorldStatus().event_match_group();
						GTFPartyClient()->RequestQueueForMatch( eEventMatchGroup );
					}
				} );
			}
			else
			{
				GTFPartyClient()->RequestQueueForMatch( eEventMatchGroup );
			}

			return;
		}

		BaseClass::OnCommand( command );
	}

	void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		if ( FStrEq( event->GetName(), "party_criteria_changed" ) )
		{
			InvalidateLayout();
		}
		else if ( FStrEq( event->GetName(), "party_queue_state_changed" ) )
		{
			InvalidateLayout();
		}
		else if ( FStrEq( event->GetName(), "world_status_changed" ) )
		{
			UpdateFromWorldStatus();
		}
	}

private:

	void UpdateFromWorldStatus()
	{
		auto& msgWorldStatus = GTFGCClientSystem()->WorldStatus();
		ETFMatchGroup eEventMatchGroup = msgWorldStatus.event_match_group();
		CRTime rtimeExpireTime( msgWorldStatus.event_expire_time() );

		if ( eEventMatchGroup == k_eTFMatchGroup_Invalid )
			return;

		if ( rtimeExpireTime < CRTime::RTime32TimeCur() )
			return;

		auto pMatchGroup = GetMatchGroupDescription( eEventMatchGroup );
		if ( !pMatchGroup )
			return;

		auto pUIData = pMatchGroup->GetPlayListEntryData();
		if ( !pUIData )
			return;

		SetupQueueButton( eEventMatchGroup );
		SetDialogVariable( "title", g_pVGuiLocalize->Find( pUIData->m_pszSidePanelTitle ) );

		Label* pDescLabel = FindControl< Label >( "ModeDesc", true );
		if ( pDescLabel )
		{
			// Hack?  Are we really ever going to have more?
			if ( eEventMatchGroup == k_eTFMatchGroup_Event_Placeholder )
			{
				const SchemaGameCategory_t* pCategory = GetItemSchema()->GetGameCategory( kGameCategory_Competitive_12v12 );

				wchar_t wszMaps[512];
				wszMaps[0] = 0;
				wchar_t wszLocString[4096];
				wszLocString[0] = 0;

				FOR_EACH_VEC( pCategory->m_vecEnabledMaps, idxMap )
				{
					if ( idxMap != 0 )
					{
						V_wcscat_safe( wszMaps, L"\n" );
					}

					V_wcscat_safe( wszMaps, L"    - " );

					const MapDef_t* pMap = pCategory->m_vecEnabledMaps[ idxMap ];
					V_wcscat_safe( wszMaps, g_pVGuiLocalize->Find( pMap->pszMapNameLocKey ) );
				}

				g_pVGuiLocalize->ConstructString_safe( wszLocString, g_pVGuiLocalize->Find( pUIData->m_pszSidePanelDesc ), 1, wszMaps );
				pDescLabel->SetText( wszLocString );
			}
			else
			{
				pDescLabel->SetText( pUIData->m_pszSidePanelDesc );
			}

			pDescLabel->SizeToContents();
		}

		ImagePanel* pModeImage = FindControl< ImagePanel >( "ModeImage", true );
		if ( pModeImage && pUIData->m_pszSidePanelImage )
		{
			pModeImage->SetImage( pUIData->m_pszSidePanelImage );
		}

		// Update the badge panels with what the event match group is
		CPvPRankPanel* pRankPanel = FindControl< CPvPRankPanel >( "RankPanel", true );
		if ( pRankPanel )
		{
			pRankPanel->SetMatchGroup( eEventMatchGroup );
		}

		CTFLocalPlayerBadgePanel* pBadgePanel = FindControl< CTFLocalPlayerBadgePanel >( "RankImage", true );
		if ( pBadgePanel )
		{
			pBadgePanel->SetMatchGroup( eEventMatchGroup );
		}
	}
};

Panel* GetEventMatchPanel()
{
	Panel* pPanel = new CTFDashboardEventPanel( NULL, "EventMatch" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetEventMatchPanel, k_eEventMatch );
