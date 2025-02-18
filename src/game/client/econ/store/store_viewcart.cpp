//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store_viewcart.h"
#include "vgui/IInput.h"
#include "baseviewport.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "econ_item_inventory.h"
#include <vgui/ILocalize.h>
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "vgui_controls/ScrollBarSlider.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DECLARE_BUILD_FACTORY( CCartViewItemEntry );

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CStoreViewCartPanel::CStoreViewCartPanel( Panel *parent ) : Frame(parent, "store_viewcart_panel")
{
	// Store is parented to the game UI panel
	vgui::VPANEL gameuiPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	SetParent( gameuiPanel );

	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );

	SetMoveable( false );
	SetSizeable( false );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );
	ListenForGameEvent( "cart_updated" );

	m_pItemEntryKVs = NULL;
	m_pClientArea = new EditablePanel(this, "ClientArea");
	m_pItemListContainer = new vgui::EditablePanel( this, "ItemListContainer" );
	m_pItemListContainerScroller = new vgui::ScrollableEditablePanel( m_pClientArea, m_pItemListContainer, "ItemListContainerScroller" );
	m_pPurchaseFooter = new EditablePanel(m_pItemListContainer, "PurchaseFooter");
	m_pEmptyCartLabel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStoreViewCartPanel::~CStoreViewCartPanel()
{
	if ( m_pItemEntryKVs )
	{
		m_pItemEntryKVs->deleteThis();
		m_pItemEntryKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( ShouldUseNewStore() ? "Resource/UI/econ/store/v2/StoreViewCartPanel.res" : "Resource/UI/econ/store/v1/StoreViewCartPanel.res" );
	m_bReapplyItemKVs = true;

	m_pItemListContainerScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pEmptyCartLabel = dynamic_cast<vgui::Label*>( m_pClientArea->FindChildByName("EmptyCartLabel") );

	m_pFeaturedItemImage = dynamic_cast<vgui::ImagePanel*>( m_pItemListContainer->FindChildByName("FeaturedItemSymbol") );
	if ( m_pFeaturedItemImage )
	{
		m_pFeaturedItemImage->SetMouseInputEnabled( false );
		m_pFeaturedItemImage->SetKeyBoardInputEnabled( false );
	}

	CExButton *pCheckoutButton = dynamic_cast<CExButton*>( m_pClientArea->FindChildByName("CheckoutButton") );
	if ( pCheckoutButton )
	{
		pCheckoutButton->AddActionSignalTarget( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "item_entry_kv" );
	if ( pItemKV )
	{
		if ( m_pItemEntryKVs )
		{
			m_pItemEntryKVs->deleteThis();
		}
		m_pItemEntryKVs = new KeyValues("item_entry_kv");
		pItemKV->CopySubkeys( m_pItemEntryKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::PerformLayout( void ) 
{
	if ( GetVParent() )
	{
		int w,h;
		vgui::ipanel()->GetSize( GetVParent(), w, h );
		SetBounds(0,0,w,h);
	}

	if ( m_bReapplyItemKVs )
	{
		m_bReapplyItemKVs = false;

		if ( m_pItemEntryKVs )
		{
			FOR_EACH_VEC( m_pItemEntries, i )
			{
				m_pItemEntries[i]->ApplySettings( m_pItemEntryKVs );
				m_pItemEntries[i]->InvalidateLayout();
			}
		}
	}

	BaseClass::PerformLayout();

	if ( m_pItemEntries.Count() )
	{
		int iTall = m_pItemEntries[0]->GetTall();
		m_pItemListContainer->SetSize( m_pItemListContainer->GetWide(), (iTall * m_pItemEntries.Count()) + m_pPurchaseFooter->GetTall() );
		m_pItemListContainerScroller->InvalidateLayout( true );
		m_pItemListContainerScroller->GetScrollbar()->InvalidateLayout( true );

		int iX,iY;
		m_pItemEntries[0]->GetPos( iX, iY );
		FOR_EACH_VEC( m_pItemEntries, i )
		{
			iY = (iTall * i);
			m_pItemEntries[i]->SetPos( iX, iY );
		}

		m_pPurchaseFooter->SetVisible( true );
		m_pPurchaseFooter->SetPos( 0, iTall * m_pItemEntries.Count() );
	}
	else
	{
		m_pItemListContainer->SetSize( m_pItemListContainer->GetWide(), 100 );
		m_pItemListContainer->InvalidateLayout( true );
		m_pItemListContainerScroller->InvalidateLayout( true );
		m_pPurchaseFooter->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::ShowPanel(bool bShow)
{
	if ( bShow )
	{
		InvalidateLayout( false, true );
		Activate();

		CExButton *pCloseButton = dynamic_cast<CExButton*>( FindChildByName("CloseButton") );
		if ( pCloseButton )
		{
			pCloseButton->RequestFocus();
		}

		// don't display the WA sales tax outside of the US
		vgui::Panel *pPanel = FindChildByName( "WashingtonStateSalesTaxLabel", true );
		if ( pPanel )
		{
			pPanel->SetVisible( FStrEq( EconUI()->GetStorePanel()->GetCountryCode(), "US" ) == true );
		}
	}

	SetVisible( bShow );

	if ( bShow )
	{
		UpdateCartItemList();
		m_pItemListContainerScroller->GetScrollbar()->SetValue( 0 );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		ShowPanel( false );
	}
	else if ( Q_strcmp(type, "cart_updated") == 0 )
	{
		UpdateCartItemList();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::UpdateCartItemList( void )
{
	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	int iNumEntriesInCart = pCart->GetNumEntries();

	// Update the item count
	wchar_t wszCount[16];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", pCart->GetTotalItems() );
	wchar_t wzLocalized[32];
	g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_CartItems" ), 1, wszCount );
	m_pClientArea->SetDialogVariable("storecart", wzLocalized );

	// Create / Update all the item entries
	if ( m_pItemEntries.Count() < iNumEntriesInCart )
	{
		for ( int i = m_pItemEntries.Count(); i < iNumEntriesInCart; i++ )
		{
			CCartViewItemEntry *pPanel = vgui::SETUP_PANEL( new CCartViewItemEntry( m_pItemListContainer, VarArgs("itementry%d", i) ) );
			pPanel->ApplySettings( m_pItemEntryKVs );
			m_pItemEntries.AddToTail( pPanel );
		}
	}
	else
	{
		for ( int i = m_pItemEntries.Count()-1; i >= iNumEntriesInCart; i-- )
		{
			m_pItemEntries[i]->MarkForDeletion();
			m_pItemEntries.Remove( i );
		}
	}

	if ( m_pEmptyCartLabel )
	{
		m_pEmptyCartLabel->SetVisible( iNumEntriesInCart == 0 );
	}

	InvalidateLayout( true );

	if ( !iNumEntriesInCart )
		return;

	bool bFeaturedImagePanelVisible = false;

	// Set all the entries up
	FOR_EACH_VEC( m_pItemEntries, i )
	{
		if ( i >= iNumEntriesInCart )
		{
			m_pItemEntries[i]->SetVisible( false );
			continue;
		}

		cart_item_t *pCartItem = pCart->GetItem(i);
		m_pItemEntries[i]->SetEntry( pCartItem, i );
		m_pItemEntries[i]->SetVisible( true );

		// If we're the featured item, show it
		if ( pCartItem && pCartItem->pEntry == EconUI()->GetStorePanel()->GetFeaturedEntry() )
		{
			bFeaturedImagePanelVisible = true;
			int iX, iY;
			m_pItemEntries[i]->GetPos( iX, iY );
			m_pFeaturedItemImage->SetPos( iX, iY + m_pItemEntries[i]->GetTall() - m_pFeaturedItemImage->GetTall() );
		}
	}

	if ( m_pFeaturedItemImage->IsVisible() != bFeaturedImagePanelVisible )
	{
		m_pFeaturedItemImage->SetVisible( bFeaturedImagePanelVisible );
	}

	// Update total price
	item_price_t unTotalPrice = pCart->GetTotalPrice();
	wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
	MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), unTotalPrice, EconUI()->GetStorePanel()->GetCurrency() );
	m_pPurchaseFooter->SetDialogVariable( "totalprice", wzLocalizedPrice );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStoreViewCartPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "close" ) )
	{
		ShowPanel( false );
	}
	else if ( !Q_strnicmp( command, "remove", 6 ) )
	{
		int iIndex = atoi(command+6);
		if ( iIndex >= 0 && iIndex < m_pItemEntries.Count() )
		{
			CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
			pCart->RemoveFromCart( iIndex );
		}
		return;
	}
	else if ( !Q_stricmp( command, "checkout" ) )
	{
		EconUI()->GetStorePanel()->InitiateCheckout( false );
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

static vgui::DHANDLE<CStoreViewCartPanel> g_StoreViewCartPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStoreViewCartPanel *OpenStoreViewCartPanel( void )
{
	if (!g_StoreViewCartPanel.Get())
	{
		g_StoreViewCartPanel = vgui::SETUP_PANEL( new CStoreViewCartPanel( NULL ) );
		g_StoreViewCartPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_StoreViewCartPanel->ShowPanel( true );

	return g_StoreViewCartPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStoreViewCartPanel *GetStoreViewCartPanel( void )
{
	return g_StoreViewCartPanel.Get();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCartViewItemEntry::SetEntry( cart_item_t *pEntry, int iEntryIndex )
{
	m_pEntry = pEntry;
	SetDialogVariable( "quantity", pEntry->iQuantity );

	int iSubTotal = pEntry->GetDisplayPrice();

	wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
	MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), iSubTotal, EconUI()->GetStorePanel()->GetCurrency() );
	SetDialogVariable("price", wzLocalizedPrice );

	CItemModelPanel *pItemPanel = dynamic_cast<CItemModelPanel*>( FindChildByName("itempanel") );
	if ( pItemPanel )
	{
		CEconItemView ItemData;
		ItemData.Init( pEntry->pEntry->GetItemDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		pItemPanel->SetItem( &ItemData );
	}

	CExButton *pRemoveButton = dynamic_cast<CExButton*>( FindChildByName("RemoveButton") );
	if ( pRemoveButton )
	{
		pRemoveButton->SetCommand( VarArgs("remove%d",iEntryIndex) );
		pRemoveButton->AddActionSignalTarget( GetStoreViewCartPanel() );
	}

	wchar_t *pwzPreviewItem = L"";
	if ( pEntry->bPreviewItem )
	{
		pwzPreviewItem = g_pVGuiLocalize->Find( "#Econ_Store_PurchaseType_PreviewItem" );
	}

	SetDialogVariable( "preview_item", pwzPreviewItem ? pwzPreviewItem : L"" );
}
