//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/tf_store_panel_base.h"
#include "vgui/IInput.h"
#include "iclientmode.h"
#include "econ_item_system.h"
#include "econ_notifications.h"
#include "c_tf_freeaccount.h"
#include <vgui_controls/AnimationController.h>
#include "charinfo_armory_subpanel.h"
#include "backpack_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

class CServerNotConnectedToSteamDialog;
CServerNotConnectedToSteamDialog *OpenServerNotConnectedToSteamDialog( vgui::Panel *pParent );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseStorePanel::CTFBaseStorePanel( Panel *parent ) : CStorePanel(parent)
{
	m_pArmoryPanel = new CArmoryPanel( this, "armory_panel" );
	m_pNotificationsPresentPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pNotificationsPresentPanel = FindChildByName( "NotificationsPresentPanel" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::OnArmoryOpened( KeyValues *data )
{
	int iItemDef = data->GetInt( "itemdef", 0 );

	// If it's a bundle, open the armory to a custom page showing all the items in the bundle
	CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( iItemDef );
	if ( pDef )
	{
		const bundleinfo_t *pBundleInfo = pDef->GetBundleInfo();
		if ( pBundleInfo )
		{
			CUtlVector<item_definition_index_t> vecItems;
			FOR_EACH_VEC( pBundleInfo->vecItemDefs, j )
			{
				if ( pBundleInfo->vecItemDefs[j] )
				{
					vecItems.AddToTail( pBundleInfo->vecItemDefs[j]->GetDefinitionIndex() );
				}
			}

			m_pArmoryPanel->ShowPanel( pDef->GetItemBaseName(), &vecItems );
			m_pArmoryPanel->MoveToFront();
			return;
		}
	}

	m_pArmoryPanel->ShowPanel( iItemDef );
	m_pArmoryPanel->MoveToFront();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::OnArmoryClosed( void )
{
	PostMessage( m_pArmoryPanel, new KeyValues("Closing") );
	m_pArmoryPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::OnThink()
{
	bool bShouldBeVisible = NotificationQueue_GetNumNotifications() != 0;

	bShouldBeVisible = false;

	if ( m_pNotificationsPresentPanel != NULL && m_pNotificationsPresentPanel->IsVisible() != bShouldBeVisible )
	{
		m_pNotificationsPresentPanel->SetVisible( bShouldBeVisible );
		if ( bShouldBeVisible )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "NotificationsPresentBlink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "NotificationsPresentBlinkStop" );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::PostTransactionCompleted( void )
{
	// pop this dialog up
	if ( NeedsToChooseMostHelpfulFriend() )
	{
		// update main menu
		IGameEvent *event = gameeventmanager->CreateEvent( "store_pricesheet_updated" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}

	EconUI()->GetBackpackPanel()->CheckForQuickOpenKey();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseStorePanel::SetTransactionID( uint64 inID )
{
	BaseClass::SetTransactionID( inID );

	EconUI()->GetBackpackPanel()->SetCurrentTransactionID( inID );
}