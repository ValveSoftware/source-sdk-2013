//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_gc_client.h"
#include "tf_party.h"

#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/SectionedListPanel.h"
#include "vgui_bitmapimage.h"
#include "vgui_avatarimage.h"
#include "store/store_panel.h"
#include <VGuiMatSurface/IMatSystemSurface.h>
#include <vgui_controls/ImageList.h>

#include "tf_lobbypanel_mvm.h"
#include "tf_lobby_container_frame_mvm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

CLobbyPanel_MvM::CLobbyPanel_MvM( vgui::Panel *pParent, CBaseLobbyContainerFrame* pLobbyContainer )
	: BaseClass( pParent, pLobbyContainer )
{
	m_pCriteria = new CMVMCriteriaPanel( this, "criteria" );
}

void CLobbyPanel_MvM::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_iImageHasTicket = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_ticket_small", true ) );
	m_pImageList->GetImage( m_iImageHasTicket )->SetSize( m_iHasTicketWidth, m_iHasTicketWidth );
	m_iImageNoTicket = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_no_ticket_small", true ) );
	m_pImageList->GetImage( m_iImageNoTicket )->SetSize( m_iHasTicketWidth, m_iHasTicketWidth );
	m_iImageSquadSurplus = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_squad_surplus_small", true ) );
	m_pImageList->GetImage( m_iImageSquadSurplus )->SetSize( m_iSquadSurplusWidth, m_iSquadSurplusWidth );
	m_iImageNoSquadSurplus = m_pImageList->AddImage( vgui::scheme()->GetImage( "pve/mvm_no_squad_surplus_small", true ) );
	m_pImageList->GetImage( m_iImageNoSquadSurplus )->SetSize( m_iSquadSurplusWidth, m_iSquadSurplusWidth );


}

EMatchGroup CLobbyPanel_MvM::GetMatchGroup( void ) const
{
	return GTFGCClientSystem()->GetSearchPlayForBraggingRights() ? k_nMatchGroup_MvM_MannUp : k_nMatchGroup_MvM_Practice;
}

bool CLobbyPanel_MvM::ShouldShowLateJoin() const
{
	TF_Matchmaking_WizardStep eWizardStep = GTFGCClientSystem()->GetWizardStep();
	return 
#ifdef USE_MVM_TOUR
	( eWizardStep == TF_Matchmaking_WizardStep_MVM_TOUR_OF_DUTY ) ||
#endif // USE_MVM_TOUR
		( eWizardStep == TF_Matchmaking_WizardStep_MVM_CHALLENGE ) ||
		( ( eWizardStep == TF_Matchmaking_WizardStep_SEARCHING ) && ( GTFGCClientSystem()->GetSearchMode() == TF_Matchmaking_MVM ) );
}

void CLobbyPanel_MvM::ApplyChatUserSettings( const LobbyPlayerInfo& player, KeyValues* pSettings ) const
{
	if ( GTFGCClientSystem()->GetSearchPlayForBraggingRights() )
	{
		pSettings->SetInt( "has_ticket", player.m_bHasTicket ? m_iImageHasTicket : m_iImageNoTicket );
		pSettings->SetInt( "squad_surplus", player.m_bSquadSurplus ? m_iImageSquadSurplus : m_iImageNoSquadSurplus );
	}
}

void CLobbyPanel_MvM::WriteGameSettingsControls()
{
	m_pCriteria->WriteControls();
}

