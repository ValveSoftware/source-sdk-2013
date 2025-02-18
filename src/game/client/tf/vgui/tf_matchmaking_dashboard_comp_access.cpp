//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard_side_panel.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_match_description.h"
#include "tf_item_inventory.h"
#include "ienginevgui.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "tf_partyclient.h"
#include "tf_badge_panel.h"
#include "c_tf_freeaccount.h"
#include "tf_ladder_data.h"
#include "tf_rating_data.h"

using namespace vgui;
using namespace GCSDK;

class CCompetitiveAccessInfoPanel : public EditablePanel
								  , public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CCompetitiveAccessInfoPanel, EditablePanel );
public:
	CCompetitiveAccessInfoPanel( Panel* pParent, const char* pszName )
		: EditablePanel( pParent, pszName )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );

		m_pPhoneButton = NULL;
		m_pPremiumButton = NULL;
		m_pRankImage = NULL;
		m_pRankButton = NULL;

		m_pPhoneCheckImage = NULL;
		m_pPremiumCheckImage = NULL;
		m_pRankCheckImage = NULL;
	}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );
		LoadControlSettings( "resource/ui/CompetitiveAccessInfo.res" );

		m_pPhoneButton = FindControl< CExImageButton >( "PhoneButton", true );
		m_pPremiumButton = FindControl< CExImageButton >( "PremiumButton", true );
		m_pRankImage = FindControl< CTFBadgePanel >( "RankImage", true );
		m_pRankButton = FindControl< CExImageButton >( "RankButton", true );

		m_pPhoneCheckImage = FindControl< ImagePanel >( "PhoneCheckImage", true );
		m_pPremiumCheckImage = FindControl< ImagePanel >( "PremiumCheckImage", true );
		m_pRankCheckImage = FindControl< ImagePanel >( "RankCheckImage", true );
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		// premium
		bool bIsFreeAccount = IsFreeTrialAccount();
		if ( m_pPremiumButton )
		{
			m_pPremiumButton->SetEnabled( bIsFreeAccount );
		}
		if ( m_pPremiumCheckImage )
		{
			m_pPremiumCheckImage->SetVisible( !bIsFreeAccount );
		}

		// phone
		bool bIsPhoneVerified = GTFGCClientSystem()->BIsPhoneVerified();
		bool bIsPhoneIdentifying = GTFGCClientSystem()->BIsPhoneIdentifying();
		bool bPhoneReady = bIsPhoneVerified && bIsPhoneIdentifying;
		if ( m_pPhoneButton )
		{
			m_pPhoneButton->SetEnabled( !bPhoneReady );
		}
		if ( m_pPhoneCheckImage )
		{
			m_pPhoneCheckImage->SetVisible( bPhoneReady );
		}

		// rank
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( k_eTFMatchGroup_Casual_12v12 );
		if ( pMatchDesc && pMatchDesc->m_pProgressionDesc )
		{
			if ( m_pRankImage )
			{
				m_pRankImage->SetupBadge( pMatchDesc, SteamUser()->GetSteamID() );
			}

			bool bHighEnoughLevel = false;
			if ( m_pRankCheckImage )
			{
				if ( steamapicontext && steamapicontext->SteamUser() )
				{
					CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
					const auto pRankRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( SteamUser()->GetSteamID(), pMatchDesc->GetCurrentDisplayRank() );
					if ( pRankRating )
					{
						auto level = pMatchDesc->m_pProgressionDesc->GetLevelByNumber( pRankRating->GetRatingData().unRatingPrimary );
						bHighEnoughLevel = level.m_nLevelNum >= k_nMinCasualLevelForCompetitive;
					}
				}
				m_pRankCheckImage->SetVisible( bHighEnoughLevel );
			}

			if ( m_pRankButton )
			{
				m_pRankButton->SetEnabled( !bHighEnoughLevel );
			}
		}
	}

	virtual void OnCommand(  const char *command ) OVERRIDE
	{
		if ( FStrEq( command, "close" ) )
		{
			SetVisible( false );
			return;
		}
		else if ( FStrEq( command, "addphone" ) )
		{
			if ( steamapicontext && steamapicontext->SteamFriends() )
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://support.steampowered.com/kb_article.php?ref=8625-WRAH-9030#addphone" );
			}
			return;
		}
		else if ( FStrEq( command, "addpremium" ) )
		{
			if ( steamapicontext && steamapicontext->SteamFriends() )
			{
				steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "https://steamcommunity.com/sharedfiles/filedetails/?id=143430756" );
			}
			return;
		}
		else if ( FStrEq( command, "open_casual" ) )
		{
			// Defaulting to 12v12
			PostMessage( GetMMDashboard(), new KeyValues( "PlayCasual" ) );
			return;
		}

		BaseClass::OnCommand( command );
	}

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE
	{
		if ( pObject->GetTypeID() != CEconGameAccountClient::k_nTypeID )
			return;

		if ( GTFGCClientSystem()->BHasCompetitiveAccess() )
		{
		//	SetVisible( false );
		}
		else
		{
			InvalidateLayout();
		}
	}

	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE
	{
		if ( pObject->GetTypeID() != CEconGameAccountClient::k_nTypeID && pObject->GetTypeID() != CSOTFLadderData::k_nTypeID )
			return;

		if ( GTFGCClientSystem()->BHasCompetitiveAccess() )
		{
		//	SetVisible( false );
		}
		else
		{
			InvalidateLayout();
		}
	}

private:
	CExImageButton	*m_pPhoneButton;
	CExImageButton	*m_pPremiumButton;
	CTFBadgePanel	*m_pRankImage;
	CExImageButton	*m_pRankButton;

	ImagePanel		*m_pPhoneCheckImage;
	ImagePanel		*m_pPremiumCheckImage;
	ImagePanel		*m_pRankCheckImage;
};

class CCompAccessSlidePanel : public CMatchMakingDashboardSidePanel
{
	DECLARE_CLASS_SIMPLE( CCompAccessSlidePanel, CMatchMakingDashboardSidePanel );
	CCompAccessSlidePanel( Panel* pParent, const char* pszName )
		: CMatchMakingDashboardSidePanel( NULL, pszName, "resource/ui/MatchMakingDashboardCompAccess.res", k_eSideRight )
	{
		m_pCompAccessPanel = new CCompetitiveAccessInfoPanel( this, "CompAccessEmbedded" );
	}

	CCompetitiveAccessInfoPanel* m_pCompAccessPanel;
};

Panel* GetCompAccessPanel()
{
	Panel* pPanel = new CCompAccessSlidePanel( NULL, "CompAccess" );
	pPanel->MakeReadyForUse();
	pPanel->AddActionSignalTarget( GetMMDashboard() );
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetCompAccessPanel, k_eCompAccess );
