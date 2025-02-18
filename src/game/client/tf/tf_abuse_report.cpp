//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_abuse_report.h"
#include "abuse_report_ui.h"
#include "game/client/iviewport.h"
#include "tf_shareddefs.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_gcmessages.h"
#include "c_tf_player.h"
#include "tf_quickplay_shared.h"
#include "tf_gc_client.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


/// Declare singleton object
CTFAbuseReportManager theAbuseReportManager;

CTFAbuseReportManager::CTFAbuseReportManager() {}
CTFAbuseReportManager::~CTFAbuseReportManager() {}

bool CTFAbuseReportManager::CreateAndPopulateIncident()
{
	if ( !CAbuseReportManager::CreateAndPopulateIncident() )
	{
		return false;
	}

	if ( m_bTestReport )
	{
		return true;
	}

	for ( int iPlayer = 0 ; iPlayer < m_pIncidentData->m_vecPlayers.Count() ; ++iPlayer )
	{

		AbuseIncidentData_t::PlayerData_t *p = &m_pIncidentData->m_vecPlayers[iPlayer];

		CPlayerInventory *pInv = InventoryManager()->GetInventoryForAccount( p->m_steamID.GetAccountID() );
		//C_TFPlayer *pTFPlayer = dynamic_cast<C_TFPlayer *> ( UTIL_PlayerByIndex( p->m_iClientIndex ) );
		//if ( pTFPlayer == NULL )
		//{
		//	Assert( !p->m_bHasEntity );
		//	continue;
		//}
		//CTFPlayerInventory *pInv = pTFPlayer->Inventory();
		if ( pInv == NULL )
		{
			Warning( "No inventory data for player '%s'; we won't be able to report this person for inappropriate custom images\n", p->m_sPersona.String() );
			continue;
		}

		for ( int i = 0 ; i < pInv->GetItemCount() ; ++i)
		{
			CEconItemView *pItem = pInv->GetItem( i );

			// Get custom texture ID, if any
			uint64 hCustomtextureID = pItem->GetCustomUserTextureID();

			// Most items won't have a custom texture
			if ( hCustomtextureID == 0 )
			{
				continue;
			}

			// Discard duplicates, as it makes the UI confusing
			bool bDup = false;
			for ( int j = 0 ; j < p->m_vecImages.Count() ; ++j )
			{
				if ( p->m_vecImages[ j ].m_hUGCHandle == hCustomtextureID )
				{
					bDup = true;
					break;
				}
			}
			if ( bDup )
			{
				continue;
			}

			AbuseIncidentData_t::PlayerImage_t img;
			img.m_eType = AbuseIncidentData_t::k_PlayerImageType_UGC;
			img.m_hUGCHandle = hCustomtextureID;
			p->m_vecImages.AddToTail( img );
		}
	}

	// Check if we can report abuse against the game server.
	if (m_pIncidentData->m_adrGameServer.IsValid() &&
	    !GTFGCClientSystem()->BIsIPRecentMatchServer( m_pIncidentData->m_adrGameServer ) )
	{
		m_pIncidentData->m_bCanReportGameServer = true;
	}

	return true;
}

void CTFAbuseReportManager::ActivateSubmitReportUI()
{
	Assert( g_AbuseReportDlg.Get() == NULL );
	Assert( m_pIncidentData != NULL );

	IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	engine->ExecuteClientCmd("gameui_activate");
	vgui::SETUP_PANEL( new CAbuseReportDlg( (CHudMainMenuOverride*)pMMOverride, m_pIncidentData ) );
	Assert( g_AbuseReportDlg.Get() != NULL );
	g_AbuseReportDlg->MakeModal();
}

