//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_matchmaking_dashboard_casual_criteria.h"
#include "tf_partyclient.h"

using namespace vgui;
using namespace GCSDK;

class CCasualSidePanel : public CMatchMakingDashboardSidePanel
					   , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CCasualSidePanel, CMatchMakingDashboardSidePanel );
public:
	CCasualSidePanel() : CMatchMakingDashboardSidePanel( NULL, "CasualCriteria", "resource/ui/MatchMakingDashboardCasualCriteria.res", k_eSideRight )
	{
		ListenForGameEvent( "party_criteria_changed" );
		ListenForGameEvent( "party_queue_state_changed" );
		ListenForGameEvent( "party_updated" );
		ListenForGameEvent( "world_status_changed" );
	}

	virtual ~CCasualSidePanel() {}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		SetupQueueButton( k_eTFMatchGroup_Casual_12v12 );
	}

	virtual void OnCommand( const char *command ) OVERRIDE
	{
		if ( FStrEq( command, "find_game" ) )
		{
			GTFPartyClient()->RequestQueueForMatch( k_eTFMatchGroup_Casual_12v12 );
			return;
		}

		BaseClass::OnCommand( command );
	}

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE
	{
		if ( FStrEq( event->GetName(), "party_criteria_changed" ) )
		{
			InvalidateLayout();
		}
		else if ( FStrEq( event->GetName(), "party_queue_state_changed" ) )
		{
			InvalidateLayout();
		}
		else if ( FStrEq( event->GetName(), "party_updated" ) )
		{
			InvalidateLayout();
		}
		else if ( FStrEq( event->GetName(), "world_status_changed" ) ) 
		{
			InvalidateLayout();
		}
	}
};

Panel* GetCasualCriteriaPanel()
{
	Panel* pPanel = new CCasualSidePanel();
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetCasualCriteriaPanel, k_eCasual );
