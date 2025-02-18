//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"

#include "animation.h"

#include "modelimagepanel.h"
#include "tf_badge_panel.h"
#include "tf_match_description.h"
#include "tf_rating_data.h"
#include "tf_ladder_data.h"
#include "tf_progression.h"
#include "tf_controls.h"

extern const char *s_pszMatchGroups[];

DECLARE_BUILD_FACTORY( CTFBadgePanel );

CTFBadgePanel::CTFBadgePanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_pBadgePanel = new CModelImagePanel( this, "BadgePanel" );
	m_nPrevLevel = 0;
}


void CTFBadgePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pBadgePanel->LoadControlSettings( "resource/ui/BadgePanel.res" );
}

void CTFBadgePanel::SetupDummyBadge( uint32 nLevel, bool bInPlacement )
{
	{
		if ( !m_pBadgePanel )
			return;

		int nLevelIndex = nLevel - 1;
		//int nSkin = nLevelIndex / 25;
		int nStarsBodyGroup = ( ( nLevelIndex ) % 5 ) + 1;
		int nBulletsBodyGroup = 0;
		int nPlatesBodyGroup = 0;
		int nBannerBodyGroup = 0;

		switch ( ( ( nLevelIndex ) / 5 ) % 5 )
		{
		case 0:
			nBulletsBodyGroup = 0;
			nPlatesBodyGroup = 0;
			nBannerBodyGroup = 0;
			break;
		case 1:
			nBulletsBodyGroup = 1;
			nPlatesBodyGroup = 0;
			nBannerBodyGroup = 0;
			break;
		case 2:
			nBulletsBodyGroup = 2;
			nPlatesBodyGroup = 1;
			nBannerBodyGroup = 0;
			break;
		case 3:
			nBulletsBodyGroup = 3;
			nPlatesBodyGroup = 2;
			nBannerBodyGroup = 1;
			break;
		case 4:
			nBulletsBodyGroup = 4;
			nPlatesBodyGroup = 3;
			nBannerBodyGroup = 1;
			break;
		}

		CUtlString strBadgeName = "models/vgui/12v12_badge.mdl";

		studiohdr_t* pHDR = m_pBadgePanel->GetStudioHdr();
		if ( !pHDR || !( CUtlString( pHDR->name ).UnqualifiedFilename() == strBadgeName.UnqualifiedFilename() ) )
		{
			m_pBadgePanel->SetMDL( strBadgeName );
		}

		int nBody = 0;
		CStudioHdr studioHDR( m_pBadgePanel->GetStudioHdr(), g_pMDLCache );

		::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "bullets" ), nBulletsBodyGroup );
		::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "plates" ), nPlatesBodyGroup );
		::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "banner" ), nBannerBodyGroup );
		::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "stars" ), nStarsBodyGroup );

		int nLogoValue = 0;
		::SetBodygroup( &studioHDR, nBody, ::FindBodygroupByName( &studioHDR, "logo" ), nLogoValue );

		m_pBadgePanel->SetBody( nBody );
		m_pBadgePanel->SetSkin( nLevel / 25 );
	}

	if ( m_nPrevLevel != nLevel || m_bPreviouslyInPlacement != bInPlacement )
	{
		m_nPrevLevel = nLevel;
		m_bPreviouslyInPlacement = bInPlacement;
		m_pBadgePanel->InvalidateImage();
	}
}

void CTFBadgePanel::SetupBadge( const IMatchGroupDescription* pMatchDesc, const LevelInfo_t& levelInfo, const CSteamID& steamID, bool bInPlacement )
{
	if ( !pMatchDesc || !pMatchDesc->m_pProgressionDesc )
		return;

	pMatchDesc->SetupBadgePanel( m_pBadgePanel, levelInfo, steamID, bInPlacement );

	if ( m_nPrevLevel != levelInfo.m_nLevelNum || m_bPreviouslyInPlacement != bInPlacement )
	{
		m_nPrevLevel = levelInfo.m_nLevelNum;
		m_bPreviouslyInPlacement = bInPlacement;
		m_pBadgePanel->InvalidateImage();
	}
}

void CTFBadgePanel::SetupBadge( const IMatchGroupDescription* pMatchDesc, const LevelInfo_t& levelInfo, const CSteamID& steamID )
{
	if ( !pMatchDesc || !pMatchDesc->m_pProgressionDesc )
		return;

	bool bInPlacement = pMatchDesc->BPlayerIsInPlacement( steamID );
	SetupBadge( pMatchDesc, levelInfo, steamID, bInPlacement );
}

void CTFBadgePanel::SetupBadge( const IMatchGroupDescription* pMatchDesc, const CSteamID& steamID )
{
	if ( pMatchDesc && pMatchDesc->m_pProgressionDesc && steamID.IsValid() )
	{
		if ( pMatchDesc->GetMatchType() == MATCH_TYPE_CASUAL )
		{
			const CTFRatingData *pRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( steamID, pMatchDesc->GetCurrentDisplayRating() );
			uint32 nCurrentRating = pRating ? pRating->GetRatingData().unRatingPrimary : 0;
			auto& level = pMatchDesc->m_pProgressionDesc->GetLevelForRating( nCurrentRating );
			SetupBadge( pMatchDesc, level, steamID );
		}
		else
		{
			const CTFRatingData *pRating = CTFRatingData::YieldingGetPlayerRatingDataBySteamID( steamID, pMatchDesc->GetCurrentDisplayRank() );
			int nLevel = pRating ? pRating->GetRatingData().unRatingPrimary : 0;
			auto& level = pMatchDesc->m_pProgressionDesc->GetLevelByNumber( nLevel );
			SetupBadge( pMatchDesc, level, steamID );
		}
	}
}

DECLARE_BUILD_FACTORY( CTFLocalPlayerBadgePanel );

CTFLocalPlayerBadgePanel::CTFLocalPlayerBadgePanel( vgui::Panel *pParent, const char *pName )
	: CTFBadgePanel( pParent, pName )
{}

void CTFLocalPlayerBadgePanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char* pszMatchGroup = inResourceData->GetString( "matchgroup", NULL );
	// If a match group is specified, use the local player's info to setup the badge
	if ( pszMatchGroup )
	{
		m_eMatchGroup = (ETFMatchGroup)StringFieldToInt( inResourceData->GetString( "matchgroup" ), s_pszMatchGroups, (int)ETFMatchGroup_ARRAYSIZE, false );
		UpdateBadge();
	}
}


void CTFLocalPlayerBadgePanel::SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFRatingData::k_nTypeID )
		UpdateBadge();
}

void CTFLocalPlayerBadgePanel::SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() == CTFRatingData::k_nTypeID )
		UpdateBadge();
}

void CTFLocalPlayerBadgePanel::SetMatchGroup( ETFMatchGroup eMatchGroup )
{
	m_eMatchGroup = eMatchGroup;
	UpdateBadge();
}

void CTFLocalPlayerBadgePanel::UpdateBadge()
{
	if ( m_eMatchGroup == k_eTFMatchGroup_Invalid )
		return;

	SetupBadge( GetMatchGroupDescription( m_eMatchGroup ), SteamUser()->GetSteamID() );
}

class CTFStaticBadgePanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFStaticBadgePanel, vgui::EditablePanel );

	CTFStaticBadgePanel( vgui::Panel *pParent, const char *pName )
		: EditablePanel( pParent, pName )
	{
		m_pModelPanel = new CBaseModelPanel( this, "BadgePanel" );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE
	{
		BaseClass::ApplySchemeSettings( pScheme );
		LoadControlSettings( "resource/ui/StaticBadgePanel.res" );
	}

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE
	{
		BaseClass::ApplySettings( inResourceData );
		SetLevel( inResourceData->GetInt( "level", 1 ) );
	}

	virtual void PerformLayout() OVERRIDE
	{
		BaseClass::PerformLayout();

		auto pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
		auto pProgression = pMatchDesc->m_pProgressionDesc;
		auto& level = pProgression->GetLevelByNumber( m_nLevel );

		SetDialogVariable( "rank", LocalizeNumberWithToken( "#TF_Competitive_RankNumber", level.m_nLevelNum ) );
		SetDialogVariable( "name", g_pVGuiLocalize->Find( level.m_pszLevelTitle ) );
	}

private:

	void SetLevel( int nLevel )
	{
		auto pMatchDesc = GetMatchGroupDescription( m_eMatchGroup );
		auto pProgression = pMatchDesc->m_pProgressionDesc;
		int nMaxLevel = pProgression->GetNumLevels();
		m_nLevel = clamp( nLevel, 1, nMaxLevel );

		auto& level = pProgression->GetLevelByNumber( m_nLevel );
		pMatchDesc->SetupBadgePanel( m_pModelPanel, level, CSteamID(), false );
		InvalidateLayout();
	}

	ETFMatchGroup m_eMatchGroup = k_eTFMatchGroup_Ladder_6v6;
	int m_nLevel = 1;
	CBaseModelPanel* m_pModelPanel = NULL;
};

DECLARE_BUILD_FACTORY( CTFStaticBadgePanel );