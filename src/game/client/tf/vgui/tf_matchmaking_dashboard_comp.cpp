//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_comp_stats.h"
#include "tf_matchmaking_dashboard_comp.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_matchmaking_dashboard_comp_rank_tooltip.h"
#include "tf_partyclient.h"

using namespace vgui;
using namespace GCSDK;

class CTFDashboardCompPanel : public CMatchMakingDashboardSidePanel
							, public CGameEventListener
{

	DECLARE_CLASS_SIMPLE( CTFDashboardCompPanel, CMatchMakingDashboardSidePanel);



	CTFDashboardCompPanel( Panel *parent, const char *panelName )
		: BaseClass( parent, panelName, "resource/ui/MatchMakingDashboardComp.res", k_eSideRight )
	{
		m_pCriteria = new CCompStatsPanel( this, "stats" );

		ListenForGameEvent( "party_criteria_changed" );
		ListenForGameEvent( "party_queue_state_changed" );
		ListenForGameEvent( "party_updated" );
		ListenForGameEvent( "world_status_changed" );
	}

	~CTFDashboardCompPanel()
	{
		m_pCriteria->MarkForDeletion();
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		SetupQueueButton( k_eTFMatchGroup_Ladder_6v6 );

		Panel* pRankToolTip = FindChildByName( "RankTooltipPanel" );
		if ( pRankToolTip )
		{
			pRankToolTip->SetTooltip( GetCompRanksTooltip(), (const char*)nullptr );
		}
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
			ShowConfirmDialog( "#TF_MM_Disconnect_Title",
							   "#TF_Matchmaking_AbandonQueuePrompt",
							   "#TF_OK",
							   "#Cancel",
							   []( bool bConfirmed, void* pContext )
			{
				if ( bConfirmed )
				{
					GTFPartyClient()->RequestQueueForMatch( k_eTFMatchGroup_Ladder_6v6 );
				}
			} );

			return;
		}

		BaseClass::OnCommand( command );
	}

	void FireGameEvent( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "party_criteria_changed" ) ||
			 FStrEq( event->GetName(), "party_queue_state_changed" ) ||
			 FStrEq( event->GetName(), "world_status_changed" ) ||
			 FStrEq( event->GetName(), "party_updated" ) ) 
		{
			InvalidateLayout();
		}
	}

private:

	CCompStatsPanel* m_pCriteria;
};

Panel* GetCompPanel()
{
	Panel* pPanel = new CTFDashboardCompPanel( NULL, "CompStats" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetCompPanel, k_eCompetitive );
