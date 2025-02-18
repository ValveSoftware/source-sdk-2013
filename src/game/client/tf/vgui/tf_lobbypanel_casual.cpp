//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#include "cbase.h"

#include "tf_party.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/ScrollableEditablePanel.h"

#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>

#include "tf_lobbypanel_casual.h"
#include "tf_lobby_container_frame_casual.h"
#include "tf_matchmaking_shared.h"
#include "tf_casual_criteria.h"

#include "tf_hud_mainmenuoverride.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern Color s_colorChallengeForegroundEnabled;
extern Color s_colorChallengeForegroundDisabled;
extern Color s_colorChallengeHeader;

extern const char *s_pszMatchGroups[];

CLobbyPanel_Casual::CLobbyPanel_Casual( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer )
	: CBaseLobbyPanel( pParent, pLobbyContainer )
{
	m_pCriteriaPanel = new CCasualCriteriaPanel( this, "criteria" );
}

CLobbyPanel_Casual::~CLobbyPanel_Casual()
{
	m_pCriteriaPanel->MarkForDeletion();
}

void CLobbyPanel_Casual::ApplyChatUserSettings( const CBaseLobbyPanel::LobbyPlayerInfo &player, KeyValues *pKV ) const
{
	pKV->SetInt( "has_ticket", 0 );
	pKV->SetInt( "squad_surplus", 0 );
}

void CLobbyPanel_Casual::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	int nAvatarWidth = ( ( m_iAvatarWidth * 5 / 4 ) + 1 ); 
	int nExtraWidth = ( m_pChatPlayerList->GetWide() - ( 2 * nAvatarWidth ) - m_iPlayerNameWidth - m_iBannedWidth );

	m_pChatPlayerList->AddColumnToSection( 0, "avatar", "#TF_Players", vgui::SectionedListPanel::COLUMN_IMAGE, nAvatarWidth );
	m_pChatPlayerList->AddColumnToSection( 0, "name", "", 0, m_iPlayerNameWidth + nExtraWidth );
	m_pChatPlayerList->AddColumnToSection( 0, "is_banned", "", vgui::SectionedListPanel::COLUMN_IMAGE | vgui::SectionedListPanel::COLUMN_CENTER, m_iBannedWidth );
	m_pChatPlayerList->AddColumnToSection( 0, "rank", "", vgui::SectionedListPanel::COLUMN_IMAGE | vgui::SectionedListPanel::COLUMN_CENTER, nAvatarWidth );
	m_pChatPlayerList->SetDrawHeaders( false );
}