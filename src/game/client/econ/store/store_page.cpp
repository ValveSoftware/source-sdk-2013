//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/store_page.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "gamestringpool.h"
#include "econ_item_inventory.h"
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "store/store_panel.h"
#include "store/store_preview_item.h"
#include "store/store_viewcart.h"
#include "rtime.h"
#include "econ_ui.h"
#include "store/store_page_new.h"
#include "gc_clientsystem.h"
#include "confirm_dialog.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_gamestats.h"
#include "c_tf_freeaccount.h"
#endif // TF_CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef TF_CLIENT_DLL
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
#endif

DECLARE_BUILD_FACTORY( CStorePreviewItemIcon );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStoreItemControlsPanel::CStoreItemControlsPanel( vgui::Panel *pParent, const char *pPanelName, CItemModelPanel *pItemModelPanel ) 
:	vgui::EditablePanel( pParent, pPanelName ),
	m_pItemModelPanel( pItemModelPanel ),
	m_pEntry( NULL ),
	m_bItemPanelEntered( false ),
	m_bButtonsVisible( false )
{
}

void CStoreItemControlsPanel::SetMouseHoverHandler( Panel *pHandler )
{
	m_pMouseHoverHandler = pHandler;
}

void CStoreItemControlsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings(
		ShouldUseNewStore() ?
			"Resource/UI/econ/store/v2/StoreItemControls.res" :
			"Resource/UI/econ/store/v1/StoreItemControls.res"
	);
}

const econ_store_entry_t *CStoreItemControlsPanel::GetItem() const 
{
	return m_pEntry;
}

void CStoreItemControlsPanel::SetItem( const econ_store_entry_t *pEntry )
{
	m_pEntry = pEntry;		
}

void CStoreItemControlsPanel::SetButtonsVisible( bool bVisible )
{
	m_bButtonsVisible = bVisible;

	for ( int i = 0; i < GetChildCount(); ++i )
	{
		CExButton *pButton = dynamic_cast< CExButton* >( GetChild( i ) );
		if ( pButton )
		{
			pButton->SetVisible( bVisible );
			pButton->SetArmed( false );
		}
	}
}

void CStoreItemControlsPanel::OnCursorEntered()
{
	BaseClass::OnCursorEntered();

	if ( m_pItemModelPanel && m_pItemModelPanel->HasItem() )
	{
		SetButtonsVisible( true );
	}	
}

void CStoreItemControlsPanel::OnCursorExited()
{
	BaseClass::OnCursorExited();
}

void CStoreItemControlsPanel::OnItemPanelEntered()
{
	m_bItemPanelEntered = true;
	SetButtonsVisible( true );
}

void CStoreItemControlsPanel::OnItemPanelExited()
{
	m_bItemPanelEntered = false;
}

void CStoreItemControlsPanel::OnThink()
{
	if ( !m_bItemPanelEntered )
	{
		if ( !IsCursorOver() )
		{
			SetButtonsVisible( false );
		}
	}

	if ( m_pMouseHoverHandler.Get() )
	{
		KeyValues *pMsg = new KeyValues( "StoreItemControlsPanelHover", "entered", m_bButtonsVisible );
		pMsg->SetPtr( "entry", (void *)m_pEntry );
		PostMessage( m_pMouseHoverHandler.Get(), pMsg );
	}
}

void CStoreItemControlsPanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "addtocart", 9 ) )
	{
		PostActionSignal( new KeyValues( "ItemAddToCart" ) );
	}
	else if ( !Q_strnicmp( command, "preview_item", 12 ) )
	{
		PostActionSignal( new KeyValues( "ItemPreview" ) );
	}
	else if ( !Q_strnicmp( command, "details", 7 ) )
	{
		PostActionSignal( new KeyValues( "ItemDetails" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemIcon::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	PostActionSignal(new KeyValues("ItemIconSelected", "icon", m_iIconIndex));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel::CStorePricePanel( vgui::Panel *pParent, const char *pPanelName ) 
	: vgui::EditablePanel( pParent, pPanelName )
{
	m_bOldDiscountVisibility = false;
	m_pPrice = NULL;
	m_pDiscount = NULL;
	m_pNew = NULL;
	m_pSale = NULL;
	m_pSaleBorder = NULL;
	m_pOGPrice = NULL;
	m_pCrossout = NULL;
	m_pLimited = NULL;
	m_pHighlighted = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel::~CStorePricePanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CStorePricePanel::GetPanelResFile()
{
	return "Resource/UI/econ/store/v1/StorePrice.res";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
		
	LoadControlSettings( GetPanelResFile() );

	m_pPrice = dynamic_cast< CExLabel* >( FindChildByName( "Price" ) );
	m_pDiscount = dynamic_cast< CExLabel* >( FindChildByName( "Discount" ) );
	m_pHighlighted = dynamic_cast< CExLabel* >( FindChildByName( "Highlighted" ) );
	m_pNew = dynamic_cast< CExLabel* >( FindChildByName( "NewLarge" ) );
	if ( !m_pNew )
	{
		m_pNew = dynamic_cast< CExLabel* >( FindChildByName( "New" ) );
	}
	m_pSale = dynamic_cast< CExLabel* >( FindChildByName( "Sale" ) );
	m_pSaleBorder = dynamic_cast< vgui::EditablePanel* >( FindChildByName( "StorePriceBorder" ) );
	m_pOGPrice = dynamic_cast< CExLabel* >( FindChildByName( "OG_Price" ) );
	m_pCrossout = FindChildByName( "OG_Price_CrossOut" );
	
	// Only support one "limited"
	m_pLimited = FindChildByName( "LimitedLarge" );
	if ( !m_pLimited )
	{
		m_pLimited = FindChildByName( "Limited" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pPrice )
	{
		int contentWidth, contentHeight;
		m_pPrice->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pPrice->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pPrice->SetWide( contentWidth + iTextInsetX );
		m_pPrice->SetPos( GetWide() - m_pPrice->GetWide(), GetTall() - m_pPrice->GetTall() );
	}

	if ( m_pPrice && m_pDiscount && m_pOGPrice )
	{
		int contentWidth, contentHeight;
		m_pDiscount->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pDiscount->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pDiscount->SetWide( contentWidth + iTextInsetX );
		m_pDiscount->SetPos( 0, GetTall() - m_pDiscount->GetTall() );

		// Place original price in bottom-right corner, above the price label
		int aPricePos[2];
		m_pPrice->GetPos( aPricePos[0], aPricePos[1] );
		m_pOGPrice->SetWide( GetWide() );
		m_pOGPrice->GetContentSize( contentWidth, contentHeight );
		int aOGPricePos[2] = { 0, aPricePos[1] - contentHeight };
		m_pOGPrice->SetPos( aOGPricePos[0], aOGPricePos[1] );

		// Place crossout over original price, halfway down from its vertical starting position
		m_pCrossout->SetBounds(
			aOGPricePos[0] + m_pOGPrice->GetWide() - contentWidth,
			aOGPricePos[1] + contentHeight/2, contentWidth, m_pCrossout->GetTall()
		);
	}

	if ( m_pNew )
	{
		int contentWidth, contentHeight;
		m_pNew->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pNew->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pNew->SetWide( contentWidth + iTextInsetX );
		int iPosX, iPosY;
		m_pNew->GetPos( iPosX, iPosY );
		m_pNew->SetPos( GetWide() - m_pNew->GetWide(), iPosY );
	}

	if ( m_pHighlighted )
	{
		int contentWidth, contentHeight;
		m_pHighlighted->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pHighlighted->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pHighlighted->SetWide( contentWidth + iTextInsetX );
		int iPosX, iPosY;
		m_pHighlighted->GetPos( iPosX, iPosY );
		m_pHighlighted->SetPos( GetWide() - m_pHighlighted->GetWide(), iPosY );
	}

	if ( m_pSale )
	{
		int contentWidth, contentHeight;
		m_pSale->GetContentSize( contentWidth, contentHeight );
		int iTextInsetX, iTextInsetY;
		m_pSale->GetTextInset( &iTextInsetX, &iTextInsetY );
		m_pSale->SetWide( contentWidth + iTextInsetX );
		int iPosX, iPosY;
		m_pSale->GetPos( iPosX, iPosY );
		m_pSale->SetPos( GetWide() - m_pSale->GetWide(), iPosY );
	}

	if ( m_pLimited )
	{
		int iPosX, iPosY;
		Panel *pRefPanel = ( m_pSale && m_pSale->IsVisible() ) ? m_pSale : ( m_pNew && m_pNew->IsVisible() ) ? m_pNew : NULL;
		if ( pRefPanel && pRefPanel->IsVisible() )
		{
			pRefPanel->GetPos( iPosX, iPosY );
			m_pLimited->SetPos( GetWide() - m_pLimited->GetWide() - XRES( 3 ), iPosY + pRefPanel->GetTall() + YRES( 3 ) );
		}
		else
		{
			m_pLimited->GetPos( iPosX, iPosY );
			m_pLimited->SetPos( GetWide() - m_pLimited->GetWide() - XRES( 3 ), iPosY );
		}
	}

	if ( m_pSaleBorder )
	{
		m_pSaleBorder->SetSize( GetWide(), GetTall() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel::SetPriceText( int iPrice, const char *pVariable, const econ_store_entry_t *pEntry )
{
	if ( iPrice == 0 )
	{
		if ( pEntry->m_bIsMarketItem )
		{
			SetDialogVariable( pVariable, g_pVGuiLocalize->Find( "#Store_Market" ) );	
		}
		else
		{
			SetDialogVariable( pVariable, "" );
		}
		return;
	}

	wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
	MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), iPrice, EconUI()->GetStorePanel()->GetCurrency() );
	
	if ( pEntry->m_bIsMarketItem )
	{	
		wchar_t wzMarketString[96];
		g_pVGuiLocalize->ConstructString_safe(
			wzMarketString,
			LOCCHAR( "%s1 %s2" ),
			2,
			g_pVGuiLocalize->Find( "#Store_Market" ),
			wzLocalizedPrice );

		SetDialogVariable( pVariable, wzMarketString );
	}
	else
	{
		SetDialogVariable( pVariable, wzLocalizedPrice );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool IsItemPreviewed( const econ_store_entry_t *pEntry, ECurrency eCurrency )
{
	return (pEntry->GetItemDefinitionIndex() == InventoryManager()->GetLocalInventory()->GetPreviewItemDef())
		&& !pEntry->IsOnSale( eCurrency );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AddItemToCartHelper( const char *pszContext, const econ_store_entry_t *pEntry, ECartItemType eSelectedCartItemType )
{
	Assert( pEntry );

	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

	// If this is the item we've previewing *and* it's the first one we've added
	// to the cart then we note that it's a preview item purchase and so we may
	// get a discount.
	ECartItemType eCartItemType = eSelectedCartItemType == kCartItem_Purchase && IsItemPreviewed( pEntry, eCurrency ) && !pCart->ContainsItemDefinition( pEntry->GetItemDefinitionIndex() )
								? kCartItem_TryOutUpgrade
								: eSelectedCartItemType;

	pCart->AddToCart( pEntry, pszContext, eCartItemType );
	EconUI()->GetStorePanel()->OnAddToCart();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void AddItemToCartHelper( const char *pszContext, item_definition_index_t unItemDef, ECartItemType eSelectedCartItemType )
{
	const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( unItemDef );
	if ( pEntry )
	{
		AddItemToCartHelper( pszContext, pEntry, eSelectedCartItemType );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel::SetItem( const econ_store_entry_t *pEntry )
{
	const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();

	item_price_t unPrice = pEntry->GetCurrentPrice( eCurrency );

	const bool bIsItemPreviewed = IsItemPreviewed( pEntry, eCurrency );

	if ( bIsItemPreviewed )
	{
		// Make sure we're doing the math we think we're doing -- the item isn't on sale and so
		// we'll be setting a new price based on the base price.
		Assert( pEntry->GetCurrentPrice( eCurrency ) == pEntry->GetBasePrice( eCurrency ) );
		Assert( unPrice == pEntry->GetBasePrice( eCurrency ) );

		// Apply the preview period discount.
		unPrice = econ_store_entry_t::CalculateSalePrice( unPrice, eCurrency, EconUI()->GetStorePanel()->GetPriceSheet()->GetPreviewPeriodDiscount() );
	}

	SetPriceText( unPrice, "price", pEntry );

	item_price_t unBasePrice;
	const bool bIsDiscounted = pEntry->HasDiscount( eCurrency, &unBasePrice );
	
	if ( m_pDiscount && m_pOGPrice )
	{
		// and discount
		if ( !bIsDiscounted )
		{
			m_pDiscount->SetVisible( false );
			m_pOGPrice->SetVisible( false );
		}
		else
		{
			SetPriceText( unBasePrice, "og_price", pEntry );

			// set the discount and size
			float flDiscountPercentage = 1.0f - ( float(unPrice) / float(unBasePrice) );
			wchar_t wszDiscount[16];
			_snwprintf( wszDiscount, ARRAYSIZE( wszDiscount ), L"-%.0f%%", flDiscountPercentage * 100.0f );
			m_pDiscount->SetText( wszDiscount );

			m_pDiscount->SetVisible( true );
			m_pOGPrice->SetVisible( true );
		}
	}

	if ( m_pCrossout && m_pOGPrice )
	{
		m_pCrossout->SetVisible( bIsDiscounted );
	}		

	if ( m_pNew )
	{
		m_pNew->SetVisible( pEntry->m_bNew );
	}

	if ( m_pHighlighted )
	{
		m_pHighlighted->SetVisible( pEntry->m_bHighlighted );


	}

	if ( m_pSale )
	{
		bool bSaleVisible = false;

		// We don't check explicitly for "is on sale" here because other things like item previews can
		// adjust the price we're going to display to the user without adjusting the actual store entry.
		if ( unPrice != pEntry->GetBasePrice( eCurrency ) && ( m_pNew == NULL || !m_pNew->IsVisible() ) )
		{
			if ( bIsItemPreviewed )
			{
				m_pSale->SetText( "#TF_PreviewDiscount" );
			}

			m_pSale->SetVisible( true );
			bSaleVisible = true;
		}
		else
		{
			m_pSale->SetVisible( false );
		}

		if ( m_pSaleBorder )
		{
			m_pSaleBorder->SetVisible( !ShouldUseNewStore() && bSaleVisible );
		}
	}

	if ( m_pLimited )
	{
		if ( pEntry->m_bLimited )
		{
			m_pLimited->SetVisible( true );
		}
		else
		{
			m_pLimited->SetVisible( false );
		}
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePricePanel::OnStoreItemControlsPanelHover( KeyValues *data )
{
	// We don't care if there's no discount label to deal with
	if ( !m_pDiscount )
		return;

	// Should the discount label be visible?
	const econ_store_entry_t *pEntry = (const econ_store_entry_t *)data->GetPtr( "entry" );
	if ( !pEntry )
		return;

	ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
	if ( !pEntry->HasDiscount( eCurrency, NULL ) )
		return;

	bool bEntered = data->GetInt( "entered" ) == 1;
	m_pDiscount->SetVisible( !bEntered );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePage::CStorePage(Panel *parent, const CEconStoreCategoryManager::StoreCategory_t *pPageData, const char *pPreviewItemResFile ) : vgui::PropertyPage(parent, "StorePage")
{
	m_pPageData = pPageData;

	m_pItemModelPanelKVs = NULL;
	m_pModelPanelLabelsKVs = NULL;
	m_pCartModelPanelKVs = NULL;
	m_pCartQuantityLabelKVs = NULL;
	
	m_pFeaturedItemPanel = NULL;
		
	m_pItemBackdropPanel = new EditablePanel( this, "ItemBackdrop" );
	m_pMouseOverItemPanel = new CItemModelPanel( this, "mouseoveritempanel" );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );
	m_pMouseOverTooltip->SetPositioningStrategy( IPTTP_BOTTOM_SIDE );

	if ( IsHomePage() )
	{
		if ( !ShouldUseNewStore() )
		{
			m_pFeaturedItemPanel = new CItemModelPanel( this, "featured_item_panel" );
			m_pFeaturedItemPanel->SetActAsButton( true, true );
			m_pFeaturedItemPanel->SetTooltip( m_pMouseOverTooltip, "" );
		}

		m_pFilterComboBox = NULL;
	}
	else
	{
		m_pFilterComboBox = new vgui::ComboBox( this, "ClassFilterComboBox", 11, false );
		m_pFilterComboBox->SetVisible( false );
		m_pFilterComboBox->AddActionSignalTarget( this );
	}

	m_pPreviewItemResFile = pPreviewItemResFile;
	m_pPreviewPanel = NULL;
	m_pSelectedPanel = NULL;
	m_pNextPageButton = NULL;
	m_pPrevPageButton = NULL;
	m_pCheckoutButton = NULL;
	m_pPreviewItemButton = NULL;
	m_pAddToCartButtonPanel = NULL;
	m_iCurrentFilter = 0;
	m_pCartButton = NULL;
	m_pBackpackLabel = NULL;
	m_iSelectedItemDef = 0;
	m_iSelectDefOnPageShow = 0;
	m_iSelectPageOnPageShow = 0;
	m_iOldSelectedItemDef = 0;
	m_bShouldDeletePreviewPanel = false;
	m_bFilterDirty = true;

	ListenForGameEvent( "cart_updated" );

	REGISTER_COLOR_AS_OVERRIDABLE( m_colItemPanelBG, "item_panel_bgcolor" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colItemPanelBGMouseover, "item_panel_bgcolor_mouseover" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_colItemPanelBGSelected, "item_panel_bgcolor_selected" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePage::~CStorePage()
{
	if ( m_pItemModelPanelKVs )
	{
		m_pItemModelPanelKVs->deleteThis();
		m_pItemModelPanelKVs = NULL;
	}
	if ( m_pCartModelPanelKVs )
	{
		m_pCartModelPanelKVs->deleteThis();
		m_pCartModelPanelKVs = NULL;
	}
	if ( m_pCartQuantityLabelKVs )
	{
		m_pCartQuantityLabelKVs->deleteThis();
		m_pCartQuantityLabelKVs = NULL;
	}
	if ( m_pModelPanelLabelsKVs )
	{
		m_pModelPanelLabelsKVs->deleteThis();
		m_pModelPanelLabelsKVs = NULL;
	}
	if ( m_bShouldDeletePreviewPanel && m_pPreviewPanel )
	{
		delete m_pPreviewPanel;
		m_pPreviewPanel = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnPostCreate()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CStorePage::GetPageResFile( void ) 
{ 
	return m_pPageData->m_pchPageRes;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePreviewItemPanel *CStorePage::CreatePreviewPanel( void )
{
	return new CStorePreviewItemPanel( this, m_pPreviewItemResFile, "storepreviewitem", this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	// First time through, create our preview panel
	if ( ( ShouldUseNewStore() || !IsHomePage() ) && !m_pPreviewPanel )
	{
		m_pPreviewPanel = CreatePreviewPanel();

		// Force it to load it's scheme now, because it needs to be done before we set it's visibility below
		m_pPreviewPanel->InvalidateLayout( false, true );
		m_pPreviewPanel->SetVisible( false );
	}

	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;
 #ifdef TF_CLIENT_DLL
	const char *pszHoliday = UTIL_GetActiveHolidayString();
	if ( pszHoliday && pszHoliday[0] )
	{
		pConditions = new KeyValues( "conditions" );

		char szCondition[64];
		Q_snprintf( szCondition, sizeof( szCondition ), "if_%s", pszHoliday );
		AddSubKeyNamed( pConditions, szCondition );
	}
#endif

	LoadControlSettings( GetPageResFile(), NULL, NULL, pConditions );	

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_bReapplyItemKVs = true;
	FOR_EACH_VEC( m_vecItemPanels, i )
	{
		SetBorderForItem( m_vecItemPanels[i].m_pItemModelPanel, false );
	}

	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	m_pNextPageButton = dynamic_cast<CExButton*>( FindChildByName("NextPageButton") );
	m_pPrevPageButton = dynamic_cast<CExButton*>( FindChildByName("PrevPageButton") );
	m_pCheckoutButton = dynamic_cast<CExButton*>( FindChildByName("CheckoutButton") );
	m_pPreviewItemButton = dynamic_cast<CExButton*>( FindChildByName("PreviewItemButton") );
	m_pAddToCartButtonPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("AddToCartButton") );
	if ( m_pAddToCartButtonPanel )
	{
		CExButton *pButton = dynamic_cast<CExButton*>( m_pAddToCartButtonPanel->FindChildByName("SubButton") );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( GetVPanel() );
		}
	}
	m_pCurPageLabel = dynamic_cast<vgui::Label*>( FindChildByName("CurPageLabel") );
	m_pCartButton = dynamic_cast<CExButton*>( FindChildByName("CartButton") );
	m_pBackpackLabel = dynamic_cast<vgui::Label*>( FindChildByName("BackpackSpaceLabel") );
	if ( m_pBackpackLabel )
	{
		m_colBackpackOrg = m_pBackpackLabel->GetFgColor();
	}

	m_pItemDetailsButtonPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("ItemDetailsButton") );
	if ( m_pItemDetailsButtonPanel )
	{
		CExButton *pButton = dynamic_cast<CExButton*>( m_pItemDetailsButtonPanel->FindChildByName("SubButton") );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( GetVPanel() );
		}
	}
	m_pItemPreviewButtonPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("ItemPreviewButton") );
	if ( m_pItemPreviewButtonPanel )
	{
		CExButton *pButton = dynamic_cast<CExButton*>( m_pItemPreviewButtonPanel->FindChildByName("SubButton") );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( GetVPanel() );
		}
	}
	
	m_pCartFeaturedItemImage = dynamic_cast<vgui::ImagePanel*>( FindChildByName("CartFeaturedItemSymbol") );
	if ( m_pCartFeaturedItemImage )
	{
		m_pCartFeaturedItemImage->SetMouseInputEnabled( false );
		m_pCartFeaturedItemImage->SetKeyBoardInputEnabled( false );
	}

	vgui::Panel *pPanel = FindChildByName("CartImage");
	if ( pPanel )
	{
		pPanel->SetMouseInputEnabled( false );
		pPanel->SetKeyBoardInputEnabled( false );
	}
	pPanel = FindChildByName("FeaturedItemSymbol");
	if ( pPanel )
	{
		pPanel->SetMouseInputEnabled( false );
		pPanel->SetKeyBoardInputEnabled( false );
	}
	pPanel = FindChildByName("FeaturedItemLabel");
	if ( pPanel )
	{
		pPanel->SetMouseInputEnabled( false );
		pPanel->SetKeyBoardInputEnabled( false );
	}
	pPanel = FindChildByName("FeaturedItemPrice");
	if ( pPanel )
	{
		pPanel->SetMouseInputEnabled( false );
		pPanel->SetKeyBoardInputEnabled( false );
	}

	if ( m_pFilterComboBox )
	{
		vgui::HFont hFont = pScheme->GetFont( "HudFontSmallestBold", true );
		m_pFilterComboBox->SetFont( hFont );
		UpdateFilteredItems();
		UpdateFilterComboBox();

		// Move to "All items" selected
		m_pFilterComboBox->SilentActivateItemByRow( 0 );
	}

	if ( m_pItemBackdropPanel )
	{
		m_pItemBackdropPanel->SetBgColor( m_colItemBackdropPanel );
	}

	SetDetailsVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_colItemBackdropPanel = inResourceData->GetColor( "item_backdrop_color" );

	KeyValues *pItemKV = inResourceData->FindKey( "modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pItemModelPanelKVs )
		{
			m_pItemModelPanelKVs->deleteThis();
		}
		m_pItemModelPanelKVs = new KeyValues("modelpanels_kv");
		pItemKV->CopySubkeys( m_pItemModelPanelKVs );
	}

	pItemKV = inResourceData->FindKey( "modelpanel_labels_kv" );
	if ( pItemKV )
	{
		if ( m_pModelPanelLabelsKVs )
		{
			m_pModelPanelLabelsKVs->deleteThis();
		}
		m_pModelPanelLabelsKVs = new KeyValues("modelpanel_labels_kv");
		pItemKV->CopySubkeys( m_pModelPanelLabelsKVs );
	}

	pItemKV = inResourceData->FindKey( "cart_modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pCartModelPanelKVs )
		{
			m_pCartModelPanelKVs->deleteThis();
		}
		m_pCartModelPanelKVs = new KeyValues("cart_modelpanels_kv");
		pItemKV->CopySubkeys( m_pCartModelPanelKVs );
	}

	pItemKV = inResourceData->FindKey( "cart_labels_kv" );
	if ( pItemKV )
	{
		if ( m_pCartQuantityLabelKVs )
		{
			m_pCartQuantityLabelKVs->deleteThis();
		}
		m_pCartQuantityLabelKVs = new KeyValues("cart_labels_kv");
		pItemKV->CopySubkeys( m_pCartQuantityLabelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::PerformLayout( void )
{
	if ( m_bReapplyItemKVs )
	{
		m_bReapplyItemKVs = false;

		FOR_EACH_VEC( m_vecItemPanels, i )
		{
			if ( m_pItemModelPanelKVs )
			{
				m_vecItemPanels[i].m_pItemModelPanel->ApplySettings( m_pItemModelPanelKVs );
				SetBorderForItem( m_vecItemPanels[i].m_pItemModelPanel, false );
				m_vecItemPanels[i].m_pItemModelPanel->InvalidateLayout();
			}
			m_vecItemPanels[i].m_pStorePricePanel->InvalidateLayout();
		}

		if ( m_pCartModelPanelKVs )
		{
			FOR_EACH_VEC( m_pCartModelPanels, i )
			{
				m_pCartModelPanels[i]->ApplySettings( m_pCartModelPanelKVs );
				SetBorderForItem( m_pCartModelPanels[i], false );
				m_pCartModelPanels[i]->InvalidateLayout();
			}
		}

		if ( m_pCartQuantityLabelKVs )
		{
			FOR_EACH_VEC( m_pCartQuantityLabels, i )
			{
				m_pCartQuantityLabels[i]->ApplySettings( m_pCartQuantityLabelKVs );
				m_pCartQuantityLabels[i]->InvalidateLayout();
			}
		}

		if ( m_pModelPanelLabelsKVs )
		{
			FOR_EACH_VEC( m_pCartQuantityLabels, i )
			{
				m_pCartQuantityLabels[i]->ApplySettings( m_pModelPanelLabelsKVs );
				m_pCartQuantityLabels[i]->InvalidateLayout();
			}
		}
	}

	BaseClass::PerformLayout();

	FOR_EACH_VEC( m_vecItemPanels, i )
	{
		CItemModelPanel *pItemModelPanel = m_vecItemPanels[i].m_pItemModelPanel;
		CStorePricePanel *pItemPricePanel = m_vecItemPanels[i].m_pStorePricePanel;
		CStoreItemControlsPanel *pItemControlsPanel = m_vecItemPanels[i].m_pItemControlsPanel;
		pItemModelPanel->SetVisible( true );
		pItemModelPanel->SetNoItemText( "#SelectNoItemSlot" );

		PositionItemPanel(pItemModelPanel, i );
		
		int iX,iY,iW,iH;
		pItemModelPanel->GetBounds( iX, iY, iW, iH );
		// Position our price label and controls
		pItemPricePanel->SetVisible( pItemModelPanel->HasItem() );
		pItemPricePanel->SetBounds( iX, iY, iW, iH );

		pItemPricePanel->InvalidateLayout( true );

		pItemControlsPanel->SetPos( iX + m_iItemControlsXOffset, iY + iH - pItemControlsPanel->GetTall() - m_iItemControlsYOffset );
	}

	if ( m_pItemBackdropPanel && m_vecItemPanels.Count() >= 2 )
	{
		CItemModelPanel *pTopLeftPanel = m_vecItemPanels.Head().m_pItemModelPanel;
		CItemModelPanel *pBottomRightPanel = m_vecItemPanels.Tail().m_pItemModelPanel;

		int aItemBackdropBounds[4];
		if ( pTopLeftPanel && pBottomRightPanel )
		{
			int nX, nY;
			pTopLeftPanel->GetPos( nX, nY );

			aItemBackdropBounds[0] = nX - m_iItemBackdropLeftMargin;
			aItemBackdropBounds[1] = nY - m_iItemBackdropTopMargin;

			pBottomRightPanel->GetPos( nX, nY );
			aItemBackdropBounds[2] = nX + pBottomRightPanel->GetWide() + m_iItemBackdropRightMargin - aItemBackdropBounds[0];
			aItemBackdropBounds[3] = nY + pBottomRightPanel->GetTall() + m_iItemBackdropBottomMargin - aItemBackdropBounds[1];

			m_pItemBackdropPanel->SetBounds( aItemBackdropBounds[0], aItemBackdropBounds[1], aItemBackdropBounds[2], aItemBackdropBounds[3] );

			m_pItemBackdropPanel->SetPaintBackgroundType( m_iItemBackdropPaintBackgroundType );
			m_pItemBackdropPanel->SetZPos( m_iItemBackdropZPos );
		}
	}

	if ( m_pCartModelPanels.Count() > 0 )
	{
		bool bFeaturedImagePanelVisible = false;
		CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();

		int iCartX, iCartY;
		m_pCartButton->GetPos( iCartX, iCartY );
		int iCartModelWide = m_pCartModelPanels[0]->GetWide();
		FOR_EACH_VEC( m_pCartModelPanels, i )
		{
			if ( m_pCartModelPanels[i]->HasItem() )
			{
				m_pCartModelPanels[i]->SetVisible( true );
				m_pCartQuantityLabels[i]->SetVisible( true );

				int iX = iCartX + m_pCartButton->GetWide() + (XRES(4) * (i+1)) + (iCartModelWide * i);
				m_pCartModelPanels[i]->SetPos( iX, iCartY );
				int iY = iCartY + m_pCartModelPanels[i]->GetTall() - m_pCartQuantityLabels[i]->GetTall();
				m_pCartQuantityLabels[i]->SetPos( iX + iCartModelWide - m_pCartQuantityLabels[i]->GetWide(), iY );

				// If we're the featured item, show it
				cart_item_t *pCartItem = pCart->GetItem(i);
				if ( pCartItem && ( pCartItem->pEntry == EconUI()->GetStorePanel()->GetFeaturedEntry() ) )
				{
					bFeaturedImagePanelVisible = true;

					if ( m_pCartFeaturedItemImage )
					{
						m_pCartFeaturedItemImage->SetPos( iX - XRES(4), iY - YRES(10) );
					}
				}
			}
		}

		if ( m_pCartFeaturedItemImage && m_pCartFeaturedItemImage->IsVisible() != bFeaturedImagePanelVisible )
		{
			m_pCartFeaturedItemImage->SetVisible( bFeaturedImagePanelVisible );
		}
	}

	if ( m_pCurPageLabel )
	{
		bool bMultiplePages = (GetNumPages() > 1);
		m_pCurPageLabel->SetVisible( bMultiplePages );
		m_pNextPageButton->SetVisible( bMultiplePages );
		m_pPrevPageButton->SetVisible( bMultiplePages );
		if ( bMultiplePages )
		{
			m_pNextPageButton->SetEnabled( m_iCurrentPage < (GetNumPages()-1) );
			m_pPrevPageButton->SetEnabled( m_iCurrentPage > 0 );
		}
	}

	if ( IsHomePage() )
	{
		const store_promotion_spend_for_free_item_t *pPromotion = EconUI()->GetStorePanel()->GetPriceSheet()->GetStorePromotion_SpendForFreeItem();
		wchar_t wszText[1024];
		wchar_t wszPriceThreshold[ kLocalizedPriceSizeInChararacters ];
		ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
		AssertMsg( eCurrency >= k_ECurrencyUSD && eCurrency < k_ECurrencyMax, "Invalid currency!" );

		int iPriceThreshold = pPromotion->m_rgusPriceThreshold[ eCurrency ];
		MakeMoneyString( wszPriceThreshold, ARRAYSIZE( wszPriceThreshold ), iPriceThreshold, EconUI()->GetStorePanel()->GetCurrency() );
		bool bIsFreeTrial = false;
#ifdef TF_CLIENT_DLL
		bIsFreeTrial = IsFreeTrialAccount();
#endif
		const char *pszLocString = bIsFreeTrial ? "#Store_FreeTrial_BonusText" : "#Store_Promotion_SpendForGift";
		const char *pszElementName = bIsFreeTrial ? "BonusTextLabel" : "PromotionLabel_BonusItem";
				
		g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( pszLocString ), 1, wszPriceThreshold );
		CExLabel *pPromotionText = dynamic_cast< CExLabel* >( FindChildByName( pszElementName, true ) );
		if ( pPromotionText )
		{
			pPromotionText->SetText( wszText );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CStorePage::PositionItemPanel( CItemModelPanel *pPanel, int iIndex )
{
	CItemModelPanel *pRealPanel = m_vecItemPanels[iIndex].m_pItemModelPanel;

	int iOffsetIndex = iIndex;
	int iYPosOffset = 0;
	int iCenter = GetWide() * 0.5;
	int iButtonX = (iOffsetIndex % GetNumColumns());
	int iButtonY = (iOffsetIndex / GetNumColumns());
	int iXPos = m_iItemXPos + (iCenter + m_iItemOffcenterX) + (iButtonX * pRealPanel->GetWide()) + (m_iItemXDelta * iButtonX);
	int iYPos = m_iItemYPos + (iButtonY * pRealPanel->GetTall() ) + (m_iItemYDelta * iButtonY) + iYPosOffset;

	pRealPanel->SetPos( iXPos, iYPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnPageShow( void )
{
	m_iCurrentPage = m_iSelectPageOnPageShow;
	m_iSelectPageOnPageShow = 0;

	// !KLUDGE!
	SetDetailsVisible( !ShouldUseNewStore() );

	m_bReapplyItemKVs = true;
	BaseClass::OnPageShow();

	if ( !IsHomePage() )
	{
		EconUI()->Gamestats_Store( IE_STORE_TAB_CHANGED, NULL, GetPageName() );
	}

	m_pMouseOverItemPanel->SetVisible( false );

	CreateItemPanels();
	
	if ( m_pFilterComboBox )
	{
		SetFilter( 0 );
		m_pFilterComboBox->SilentActivateItemByRow( 0 );
//		m_pFilterComboBox->SetVisible( !IsHomePage() );
	}

	// Setup sort by newest
	if ( m_pPageData && !ShouldUseNewStore() )
	{
		eEconStoreSortType iSortType = kEconStoreSortType_DateNewest;
		CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheetForEdit();
		pPriceSheet->SetEconStoreSortType( iSortType );

		CEconStoreCategoryManager::StoreCategory_t *pPageData = const_cast< CEconStoreCategoryManager::StoreCategory_t * >( m_pPageData );
		pPageData->m_vecEntries.SetLessContext( pPriceSheet );
		pPageData->m_vecEntries.RedoSort( true );

		UpdateFilteredItems();
	}

	UpdateModelPanels();

	if ( m_pCheckoutButton )
	{
		m_pCheckoutButton->RequestFocus();
	}

	if ( m_iSelectDefOnPageShow )
	{
		m_iSelectDefOnPageShow = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePricePanel* CStorePage::CreatePricePanel( int iIndex )
{
	if ( m_pPageData && !Q_strcmp( m_pPageData->m_pchPageClass, "CStorePage_Popular" ) )
		return vgui::SETUP_PANEL( new CStorePricePanel_Popular( this, "StorePrice", iIndex + 1 ) );

	if ( m_pPageData && !Q_strcmp( m_pPageData->m_pchPageClass, "CStorePage_New" ) )
		return vgui::SETUP_PANEL( new CStorePricePanel_New( this, "StorePrice" ) );

	if ( m_pPageData && !Q_strcmp( m_pPageData->m_pchPageClass, "CStorePage_Bundles" ) )
		return vgui::SETUP_PANEL( new CStorePricePanel_Bundles( this, "StorePrice" ) );

	return vgui::SETUP_PANEL( new CStorePricePanel( this, "StorePrice" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OrderItemsForDisplay( CUtlVector<const econ_store_entry_t *>& vecItems ) const
{
	/*
	// See how I tread upon all the holy concepts of OOP.
	if ( m_pPageData &&
		 !Q_strcmp( m_pPageData->m_pchPageClass, "CStorePage_Bundles" ) &&
		 !ShouldUseNewStore() )
	{
		vecItems.Sort( &ItemDisplayOrderSort_UseSortOverride );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::CreateItemPanels( void )
{
	int iNumPanels = GetNumItemPanels();
	if ( m_pPageData && m_vecItemPanels.Count() < iNumPanels )
	{
		for ( int i = m_vecItemPanels.Count(); i < iNumPanels; i++ )
		{
			int idx = m_vecItemPanels.AddToTail();
			item_panel &itempanel = m_vecItemPanels[idx];
			CItemModelPanel *pPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, VarArgs("modelpanel%d", i) ) );
			pPanel->SetShowQuantity( true );
			pPanel->SetActAsButton( true, true );
			itempanel.m_pItemModelPanel = pPanel;

			pPanel->SetTooltip( m_pMouseOverTooltip, "" );

			// Create our price panel too
			CStorePricePanel *pPricePanel = CreatePricePanel( i );
			pPricePanel->SetMouseInputEnabled( false );
			pPricePanel->SetKeyBoardInputEnabled( false );
			itempanel.m_pStorePricePanel = pPricePanel;

			// and controls
			CStoreItemControlsPanel *pControlsPanel = vgui::SETUP_PANEL( new CStoreItemControlsPanel( this, "StoreItemControls", pPanel ) );
			//pControlsPanel->AddActionSignalTarget( this );
			if ( ShouldUseNewStore() )
			{
				pControlsPanel->SetMouseHoverHandler( pPricePanel );
			}
			itempanel.m_pItemControlsPanel = pControlsPanel;
		}

		m_EntryIndices.SetCountNonDestructively( m_vecItemPanels.Count() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "nextpage", 8 ) )
	{
		if ( m_iCurrentPage < (GetNumPages()-1) )
		{
			m_iCurrentPage++;
			UpdateModelPanels();
		}
		return;
	}
	else if ( !Q_strnicmp( command, "prevpage", 8 ) )
	{
		if ( m_iCurrentPage > 0 )
		{
			m_iCurrentPage--;
			UpdateModelPanels();
		}
		return;
	}
	else if ( !Q_strnicmp( command, "preview_item", 12 ) )
	{
		PreviewSelectionItem();
		return;
	}
	else if ( !Q_strnicmp( command, "addtocart", 9 ) )
	{
		AddSelectionToCart();
		return;
	}
	else if ( !Q_strnicmp( command, "viewcart", 8 ) )
	{
		OpenStoreViewCartPanel();
		return;
	}
	else if ( !Q_strnicmp( command, "startshopping", 8 ) )
	{
		PostMessage( EconUI()->GetStorePanel(), new KeyValues("StartShopping") );
		return;
	}
	else if ( !Q_strnicmp( command, "checkout", 8 ) )
	{
		EconUI()->GetStorePanel()->InitiateCheckout( false );
		return;
	}
	else if ( !Q_stricmp( command, "show_details" ) )
	{
		if ( m_pSelectedPanel )
		{
			CEconItemView *pItem = m_pSelectedPanel->GetItem();
			if ( pItem )
			{
				SetDetailsVisible( true );
			}
		}
		return;
	}
	else if ( !Q_stricmp( command, "show_preview" ) )
	{
		SetDetailsVisible( false );
		return;
	}
	else if ( !Q_strnicmp( command, "marketplace", 8 ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://steamcommunity.com/market/search?appid=440" );
		}
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( false, true );
		SetVisible( true );
		UpdateSelectionInfoPanel();
		return;
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "cart_updated") == 0 )
	{
		UpdateCart();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnMouseWheeled( int delta )
{
	if ( m_vecItemPanels.Count() == 0 )
	{
		// on home page, likely
		return;
	}

	int oldSelectionIndex = -1;
	int currentSelectionIndex = -1;

	// deselect everything
	FOR_EACH_VEC( m_vecItemPanels, i )
	{
		if ( m_vecItemPanels[i].m_pItemModelPanel->IsSelected() )
		{
			oldSelectionIndex = i;
			m_vecItemPanels[i].m_pItemModelPanel->SetSelected( false );
			SetBorderForItem( m_vecItemPanels[i].m_pItemModelPanel, false );
		}
	}

	// step selection ahead
	if ( delta < 0 )
	{
		currentSelectionIndex = oldSelectionIndex+1;

		if ( currentSelectionIndex >= m_vecItemPanels.Count() )
		{
			if ( m_iCurrentPage < (GetNumPages()-1) )
			{
				currentSelectionIndex = 0;
				m_iCurrentPage++;
				UpdateModelPanels();
			}
			else
			{
				currentSelectionIndex = m_vecItemPanels.Count();
			}
		}
		else if ( !m_vecItemPanels[ currentSelectionIndex ].m_pItemModelPanel->HasItem() )
		{
			// don't move into empty slots
			currentSelectionIndex = oldSelectionIndex;
		}
	} 
	else if ( delta > 0 )
	{
		currentSelectionIndex = oldSelectionIndex-1;

		if ( currentSelectionIndex < 0 )
		{
			if ( m_iCurrentPage > 0 )
			{
				currentSelectionIndex = m_vecItemPanels.Count()-1;
				m_iCurrentPage--;
				UpdateModelPanels();
			}
			else
			{
				currentSelectionIndex = 0;
			}
		}
		else if ( !m_vecItemPanels[ currentSelectionIndex ].m_pItemModelPanel->HasItem() )
		{
			// don't move into empty slots
			currentSelectionIndex = oldSelectionIndex;
		}
	}
	else
	{
		// no actual wheel movement
		return;
	}

	// sanity check
	currentSelectionIndex = clamp( currentSelectionIndex, 0, m_vecItemPanels.Count()-1 );

	m_pSelectedPanel = m_vecItemPanels[ currentSelectionIndex ].m_pItemModelPanel;
	m_pSelectedPanel->SetSelected( ShouldUseNewStore() );
	SetBorderForItem( m_pSelectedPanel, false );
	UpdateSelectionInfoPanel();

	if ( currentSelectionIndex != oldSelectionIndex )
	{
		vgui::surface()->PlaySound( "ui/buttonclick.wav" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CStorePage::AssignItemToPanel( CItemModelPanel *pPanel, int iIndex )
{
	iIndex += (m_iCurrentPage * GetNumItemPanels());
	if ( iIndex >= 0 && iIndex < m_FilteredEntries.Count() )
	{
		CEconItemView ItemData;
		ItemData.Init( m_FilteredEntries[iIndex]->GetItemDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		ItemData.SetItemQuantity( m_FilteredEntries[iIndex]->GetQuantity() );
		ItemData.SetClientItemFlags( kEconItemFlagClient_Preview | kEconItemFlagClient_StoreItem );
		pPanel->SetItem( &ItemData );

		return iIndex;
	}

	pPanel->SetItem( NULL );
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CStorePage::GetNumPages( void )
{
	return ceil( (float)m_FilteredEntries.Count() / (float)GetNumItemPanels() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/* static */ int CStorePage::ItemDisplayOrderSort_UseSortOverride( const econ_store_entry_t *const *ppA, const econ_store_entry_t *const *ppB )
{
	static CSchemaAttributeDefHandle pAttribDef_StoreSortOverride( "store sort override" );

	const GameItemDefinition_t *pDefA = ItemSystem()->GetStaticDataForItemByDefIndex( (*ppA)->GetItemDefinitionIndex() ),
							   *pDefB = ItemSystem()->GetStaticDataForItemByDefIndex( (*ppB)->GetItemDefinitionIndex() );

	// We expect only items with valid definition indices to make it into the list to
	// be sorted.
	Assert( pDefA );
	Assert( pDefB );

	// Sort based on: our sort key if we have one; otherwise our definition index.
	float flValue;
	int unSortKeyA = ( pAttribDef_StoreSortOverride && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pDefA, pAttribDef_StoreSortOverride, &flValue ) )
				   ? (int)flValue
				   : pDefA->GetDefinitionIndex(),
		unSortKeyB = ( pAttribDef_StoreSortOverride && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pDefB, pAttribDef_StoreSortOverride, &flValue ) )
				   ? (int)flValue
				   : pDefB->GetDefinitionIndex();

	return unSortKeyA - unSortKeyB;
}

//-----------------------------------------------------------------------------
// Purpose: Update our internal list of entries based on our filters, and count items in each filter
//-----------------------------------------------------------------------------
void CStorePage::UpdateFilteredItems( void )
{
	if ( !m_bFilterDirty )
		return;

	m_FilteredEntries.Purge();
	m_vecFilterCounts.SetCount( GetNumPrimaryFilters() );
	if ( !m_vecFilterCounts.Count() )
		return;

	FOR_EACH_VEC( m_vecFilterCounts, i )
	{
		m_vecFilterCounts[i] = 0;
	}

	for ( int i = 0; i < m_pPageData->m_vecEntries.Count(); i++ )
	{
		const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( m_pPageData->m_vecEntries[i] );
		GameItemDefinition_t *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( pEntry->GetItemDefinitionIndex() );
		if ( pDef )
		{
			// Get a list of applicable filters for the current item definition
			CUtlVector<int> filterList;
			GetFiltersForDef( pDef, &filterList );

			bool bPassesClassFilter = false;

			FOR_EACH_VEC( filterList, iFL )
			{
				int iFilter = filterList[iFL];

				m_vecFilterCounts[iFilter]++;

				if ( m_iCurrentFilter == iFilter )
				{
					bPassesClassFilter = true;
				}
			}

			// If the item passes both filters, add it.
			// NOTE: DoesEntryFilterPassSecondaryFilter() returns true by default.
			if ( bPassesClassFilter && DoesEntryFilterPassSecondaryFilter( pEntry ) )
			{
				m_FilteredEntries.AddToTail( pEntry );
			}
		}
	}

	// Sort our full list of entries however this store page wants it.
	OrderItemsForDisplay( m_FilteredEntries );

	m_bFilterDirty = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::UpdateModelPanels( void )
{
	DeSelectAllItemPanels();
	UpdateSelectionInfoPanel();
	UpdateCart();

	if ( m_pPageData != NULL )
	{
		UpdateFilteredItems();
		UpdateFilterComboBox();

		FOR_EACH_VEC( m_vecItemPanels, i )
		{
			CItemModelPanel *pItemModelPanel = m_vecItemPanels[i].m_pItemModelPanel;
			pItemModelPanel->SetShowEquipped( true );
			m_EntryIndices[i] = AssignItemToPanel( pItemModelPanel, i );
			SetBorderForItem( pItemModelPanel, false );

			int iEntry = m_EntryIndices[i];
			if ( iEntry >= 0 && iEntry < m_FilteredEntries.Count() )
			{
				// Set the price label
				m_vecItemPanels[i].m_pStorePricePanel->SetItem( m_FilteredEntries[ iEntry ] );
				m_vecItemPanels[i].m_pItemControlsPanel->SetItem( m_FilteredEntries[ iEntry ] );
			}
		}
	}

	char szTmp[16];
	Q_snprintf(szTmp, 16, "%d/%d", m_iCurrentPage+1, GetNumPages() );
	SetDialogVariable( "backpackpage", szTmp );

	UpdateBackpackLabel();

	// Now layout again to position our item buttons 
	InvalidateLayout();

	if ( m_pFilterComboBox )
	{
		m_pFilterComboBox->GetComboButton()->SetFgColor( Color( 117,107,94,255 ) ); 
		m_pFilterComboBox->GetComboButton()->SetDefaultColor( Color( 117,107,94,255), Color( 0,0,0,0) );
		m_pFilterComboBox->GetComboButton()->SetArmedColor( Color( 117,107,94,255), Color( 0,0,0,0) );
		m_pFilterComboBox->GetComboButton()->SetDepressedColor( Color( 117,107,94,255), Color( 0,0,0,0) );
	}

	// If we're not the home page, start with the first item selected already
	if ( m_vecItemPanels.Count() )
	{
		ToggleSelectItemPanel( m_vecItemPanels[m_iSelectDefOnPageShow].m_pItemModelPanel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && pItemPanel->HasItem() )
	{
		if ( IsHomePage() )
		{
			// On the homepage, they've clicked the featured item. Find it in a store tab and move to it.
			PostMessage( EconUI()->GetStorePanel(), new KeyValues("FindAndSelectFeaturedItem") );
		}
		else if ( !pItemPanel->IsSelected() )
		{
			ToggleSelectItemPanel( pItemPanel );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnItemPanelMouseDoublePressed( vgui::Panel *panel )
{
	if ( IsHomePage() )
	{
		// On the homepage, they've clicked the featured item. Find it in a store tab and move to it.
		PostMessage( EconUI()->GetStorePanel(), new KeyValues("FindAndSelectFeaturedItem") );
		return;
	}

	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() && pItemPanel->HasItem() )
	{
		// Make sure this panel is selected
		if ( !pItemPanel->IsSelected() )
		{
			ToggleSelectItemPanel( pItemPanel );
		}

		// Double clicking on an item in the cart takes you to the view cart page
		FOR_EACH_VEC( m_pCartModelPanels, i )
		{
			if ( m_pCartModelPanels[i] == pItemPanel )
			{
				OpenStoreViewCartPanel();
				return;
			}
		}

		// Not a cart panel, so add to cart.
		OnCommand("addtocart");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::DeSelectAllItemPanels( void )
{
	FOR_EACH_VEC( m_vecItemPanels, i )
	{
		if ( m_vecItemPanels[i].m_pItemModelPanel->IsSelected() )
		{
			m_vecItemPanels[i].m_pItemModelPanel->SetSelected( false );
			SetBorderForItem( m_vecItemPanels[i].m_pItemModelPanel, false );
		}
	}
	FOR_EACH_VEC( m_pCartModelPanels, i )
	{
		if ( m_pCartModelPanels[i]->IsSelected() )
		{
			m_pCartModelPanels[i]->SetSelected( false );
			SetBorderForItem( m_pCartModelPanels[i], false );
		}
	}

	m_pSelectedPanel = NULL;
	if ( m_pFeaturedItemPanel && m_pFeaturedItemPanel->IsSelected() )
	{
		m_pFeaturedItemPanel->SetSelected( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::ToggleSelectItemPanel( CItemModelPanel *pPanel )
{
	if ( ShouldUseNewStore() )
		return;

	if ( pPanel->IsSelected() || !pPanel->HasItem() )
	{
		pPanel->SetSelected( false );
		m_pSelectedPanel = NULL;
	}
	else
	{
		DeSelectAllItemPanels();
		pPanel->SetSelected( true );
		m_pSelectedPanel = pPanel;
	}
	SetBorderForItem( pPanel, false );
	UpdateSelectionInfoPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::SelectItemPanel( CItemModelPanel *pPanel )
{
	DeSelectAllItemPanels();
	pPanel->SetSelected( true );
	m_pSelectedPanel = pPanel;
	SetBorderForItem( pPanel, false );
	UpdateSelectionInfoPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		CEconItemView *pItem = pItemPanel->GetItem();
		if ( !pItemPanel->IsSelected() )
		{
			SetBorderForItem( pItemPanel, pItem != NULL );
		}
		if ( pItemPanel->HasItem() )
		{
			// make related controls visible
			FOR_EACH_VEC( m_vecItemPanels, i )
			{
				item_panel &itempanel = m_vecItemPanels[i];
				if ( itempanel.m_pItemModelPanel == pItemPanel )
				{
					itempanel.m_pItemControlsPanel->OnItemPanelEntered();
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		if ( !pItemPanel->IsSelected() )
		{
			SetBorderForItem( pItemPanel, false );
		}
		if ( pItemPanel->HasItem() )
		{
			// make related controls visible
			FOR_EACH_VEC( m_vecItemPanels, i )
			{
				item_panel &itempanel = m_vecItemPanels[i];
				if ( itempanel.m_pItemModelPanel == pItemPanel )
				{
					itempanel.m_pItemControlsPanel->OnItemPanelExited();
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnItemAddToCart( vgui::Panel *panel )
{
	CStoreItemControlsPanel *pControlsPanel = dynamic_cast< CStoreItemControlsPanel * >( panel );
	if ( pControlsPanel )
	{
		const econ_store_entry_t *pEntry = pControlsPanel->GetItem();
		if ( pEntry )
		{
			if ( !ShouldUseNewStore() )
			{
				SelectItemPanel( pControlsPanel->GetItemModelPanel() );
			}
			else
			{
#if defined( TF_CLIENT_DLL )
				C_CTFGameStats::ImmediateWriteInterfaceEvent( "store_add_to_cart", "minibutton" );
#endif
			}
			AddItemToCartHelper( GetPageName(), pEntry, kCartItem_Purchase );
			UpdateCart();
		}

		// Turn the free slots indicator red if we can't fit everything.
		UpdateBackpackLabel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	if ( !pItemPanel || pItemPanel == m_pFeaturedItemPanel )
		return;

	// Store panels use backgrounds instead of borders
	pItemPanel->SetBorder( NULL );
	pItemPanel->SetPaintBackgroundEnabled( true );

	if ( pItemPanel->IsSelected() )
	{
		pItemPanel->SetBgColor( m_colItemPanelBGSelected );
	}
	else if ( bMouseOver )
	{
		pItemPanel->SetBgColor( m_colItemPanelBGMouseover );
	}
	else
	{
		pItemPanel->SetBgColor( m_colItemPanelBG );
	}

	const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();

	if ( pItemPanel->GetItem() && pPriceSheet )
	{ 
		const econ_store_entry_t  *pEntry = pPriceSheet->GetEntry( pItemPanel->GetItem()->GetItemDefIndex() );

		if (pEntry && pEntry->m_bHighlighted && !bMouseOver )
		{
			pItemPanel->SetBorder( vgui::scheme()->GetIScheme( GetScheme() )->GetBorder( "StoreHighlightedBackgroundBorder" ) );
			pItemPanel->SetPaintBorderEnabled( true );
			pItemPanel->SetPaintBackgroundEnabled( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::CalculateItemButtonPos( CItemModelPanel *pItemPanel, int x, int y, int *iXPos, int *iYPos )
{
	*iXPos = x;
	*iYPos = (y + pItemPanel->GetTall() + YRES(4));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::UpdateSelectionInfoPanel( void )
{
	// Home page doesn't support item selections
	if ( IsHomePage() )
		return;

	if ( m_pPreviewItemButton )
	{
		m_pPreviewItemButton->SetVisible( false );
	}

	if ( m_pSelectedPanel )
	{
		const econ_store_entry_t *pEntry = GetSelectedEntry();
		CEconItemView *pItem = m_pSelectedPanel->GetItem();
		if ( pItem && pEntry )
		{
			if ( m_pPreviewItemButton )
			{
				m_pPreviewItemButton->SetVisible( pEntry->CanPreview() );
			}

			m_iOldSelectedItemDef = m_iSelectedItemDef;
			m_iSelectedItemDef = pItem->GetItemDefIndex();

			if ( m_iSelectedItemDef != m_iOldSelectedItemDef )
			{
				EconUI()->Gamestats_Store( IE_STORE_ITEM_SELECTED, pItem, GetPageName() );
			}

			CEconItemDefinition *pItemData = pItem->GetStaticData();
			if ( pItemData )
			{
				ShowPreview( 0, pEntry );
				InvalidateLayout();

				wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
				int iPrice = pEntry->GetCurrentPrice( EconUI()->GetStorePanel()->GetCurrency() );
				MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), iPrice, EconUI()->GetStorePanel()->GetCurrency() );
				SetDialogVariable("selectionprice", wzLocalizedPrice );

				if ( m_pAddToCartButtonPanel )
				{
					m_pAddToCartButtonPanel->SetVisible( true );
				}

				return;
			}
		}
	}

	SetDialogVariable("selectionprice", "" );

	if ( m_pAddToCartButtonPanel )
	{
		m_pAddToCartButtonPanel->SetVisible( false );
	}
	m_iSelectedItemDef = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CStorePage::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>( pPanel );

	m_bFilterDirty = true;

	if ( pComboBox == m_pFilterComboBox )
	{
		// the class selection combo box changed, update class details
		KeyValues *pUserData = m_pFilterComboBox->GetActiveItemUserData();
		if ( !pUserData )
			return;

		int iFilter = pUserData->GetInt( "filter", 0 );

		// If there are no items for that class, refuse to switch
		if ( iFilter && m_vecFilterCounts[iFilter] <= 0 )
		{
			m_pFilterComboBox->ActivateItemByRow( m_iCurrentFilter ? m_iCurrentFilter+1 : 0 );
			return;
		}

		SetFilter( iFilter );
		m_iCurrentPage = 0;
		UpdateModelPanels();

		m_pCheckoutButton->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::SetFilter( int iFilter )
{
	if ( iFilter != m_iCurrentFilter )
		m_bFilterDirty = true;

	m_iCurrentFilter = iFilter;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::ShowPreview( int iClass, const econ_store_entry_t* pEntry )
{
	if ( !m_pPreviewPanel )
		return;

	CEconItemView itemData;
	itemData.Init( m_iSelectedItemDef, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
	itemData.SetClientItemFlags( kEconItemFlagClient_Preview );

	m_pPreviewPanel->PreviewItem( iClass, &itemData, pEntry );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::SetDetailsVisible( bool bVisible )
{
	if ( m_pPreviewPanel )
	{
		m_pPreviewPanel->SetState( bVisible ? PS_DETAILS : PS_ITEM );
	}

	if ( m_pItemPreviewButtonPanel && m_pItemDetailsButtonPanel )
	{
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		if ( bVisible )
		{
			m_pItemPreviewButtonPanel->SetBorder( pScheme->GetBorder("StorePreviewTabUnselected") );
			m_pItemDetailsButtonPanel->SetBorder( pScheme->GetBorder("StorePreviewTabSelected") );
		}
		else
		{
			m_pItemPreviewButtonPanel->SetBorder( pScheme->GetBorder("StorePreviewTabSelected") );
			m_pItemDetailsButtonPanel->SetBorder( pScheme->GetBorder("StorePreviewTabUnselected") );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CStorePage::FindAndSelectEntry( const econ_store_entry_t *pEntry )
{
	// We can't search if we haven't created our item panels & filtered.
	CreateItemPanels();
	SetFilter( FILTER_ALL_ITEMS );
	UpdateFilteredItems();

	FOR_EACH_VEC( m_FilteredEntries, i )
	{
		if ( m_FilteredEntries[i]->GetItemDefinitionIndex() == pEntry->GetItemDefinitionIndex() )
		{
			// Figure out what page it'll be on
			int iPage = floor( (float)i / (float)GetNumItemPanels() );

			// Switch to that page
			m_iCurrentPage = iPage;
			UpdateModelPanels();
			m_iSelectPageOnPageShow = iPage;

			// Then select the item model panel for this item
			FOR_EACH_VEC( m_vecItemPanels, p )
			{
				CEconItemView *pItem = m_vecItemPanels[p].m_pItemModelPanel->GetItem();
				if ( pItem && pItem->GetItemDefIndex() == pEntry->GetItemDefinitionIndex() )
				{
					// We can't select here, because the pageshow will stomp it.
					// Remember that this is the panel we'd like to have selected.
					m_iSelectDefOnPageShow = p;
					break;
				}
			}
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const econ_store_entry_t *CStorePage::GetSelectedEntry( void )
{
	// Get the entry for the panel.
	int iEntry = -1;

	if ( m_pFeaturedItemPanel == m_pSelectedPanel )
		return EconUI()->GetStorePanel()->GetFeaturedEntry();

	FOR_EACH_VEC( m_vecItemPanels, i )
	{
		if ( m_vecItemPanels[i].m_pItemModelPanel == m_pSelectedPanel )
		{
			iEntry = m_EntryIndices[i];
			if ( iEntry >= 0 && iEntry < m_FilteredEntries.Count() )
				return m_FilteredEntries[iEntry];
		}
	}

	// It's probably something already in our cart.
	FOR_EACH_VEC( m_pCartModelPanels, i )
	{
		if ( m_pCartModelPanels[i] == m_pSelectedPanel )
		{
			CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
			if ( i < pCart->GetNumEntries() )
			{
				cart_item_t *pCartItem = pCart->GetItem(i);
				return pCartItem->pEntry;
			}
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::AddSelectionToCart( void )
{
	if ( !m_pSelectedPanel )
		return;

	// Get the entry for the panel.
	const econ_store_entry_t *pEntry = GetSelectedEntry();
	if ( pEntry )
	{
		AddItemToCartHelper( GetPageName(), pEntry, kCartItem_Purchase );
		UpdateCart();
	}

	// Turn the free slots indicator red if we can't fit everything.
	UpdateBackpackLabel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::UpdateCart( void )
{
	if ( !IsVisible() || ( !ShouldUseNewStore() && IsHomePage() ) )
		return;

	CStoreCart *pCart = EconUI()->GetStorePanel()->GetCart();
	int iNumEntriesInCart = pCart->GetNumEntries();

	// Now update the item icons next to the cart.
	if ( m_pCartModelPanels.Count() < iNumEntriesInCart )
	{
		// Support a max of 10 items in the cart quickview right now
		for ( int i = m_pCartModelPanels.Count(); (i < iNumEntriesInCart) && (i < m_iMaxCartModelPanels); i++ )
		{
			CItemModelPanel *pPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, VarArgs("cartmodelpanel%d", i) ) );
			pPanel->SetActAsButton( true, true );
			pPanel->ApplySettings( m_pCartModelPanelKVs );
			SetBorderForItem( pPanel, false );
			m_pCartModelPanels.AddToTail( pPanel );

			pPanel->SetTooltip( m_pMouseOverTooltip, "" );

			CExLabel *pLabel = vgui::SETUP_PANEL( new CExLabel( this, VarArgs("cartquantitylabel%d", i), "X" ) );
			pLabel->ApplySettings( m_pCartQuantityLabelKVs );
			pLabel->SetMouseInputEnabled( false );
			pLabel->SetKeyBoardInputEnabled( false );
			m_pCartQuantityLabels.AddToTail( pLabel );
		}
	}

	UpdateBackpackLabel();

	InvalidateLayout();

	CEconItemView *pItemData = new CEconItemView();

	// Assign the items in the cart to the panels
	FOR_EACH_VEC( m_pCartModelPanels, i )
	{
		if ( i >= iNumEntriesInCart )
		{
			m_pCartModelPanels[i]->SetItem( NULL );
			m_pCartModelPanels[i]->SetVisible( false );
			m_pCartQuantityLabels[i]->SetVisible( false );
			continue;
		}

		cart_item_t *pCartItem = pCart->GetItem(i);
		pItemData->Init( pCartItem->pEntry->GetItemDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
		pItemData->SetClientItemFlags( kEconItemFlagClient_Preview | kEconItemFlagClient_StoreItem );
		m_pCartModelPanels[i]->SetItem( pItemData );
		m_pCartModelPanels[i]->SetVisible( true );

		m_pCartQuantityLabels[i]->SetVisible( true );
		m_pCartQuantityLabels[i]->SetText( VarArgs("%d",pCartItem->iQuantity) );
	}

	delete pItemData;

	// Update the item count
	wchar_t wszCount[16];
	wchar_t wzLocalized[512];
	_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%d", pCart->GetTotalItems() );
	g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#Store_Cart" ), 1, wszCount );
	SetDialogVariable("storecart", wzLocalized );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar econ_never_show_items_in_cart_count( "econ_never_show_items_in_cart_count", "1", FCVAR_DEVELOPMENTONLY );

void CStorePage::UpdateBackpackLabel( void )
{
	wchar_t wszBackpackSlotCount[16];
	wchar_t wszLocalized[512];

	// How many slots do we have free in our current backpack? This won't take into
	// consideration expanders, account upgrades, etc.
	const int iMaxItemCount = InventoryManager()->GetLocalInventory()->GetMaxItemCount(),
			  iCurItemCount = InventoryManager()->GetLocalInventory()->GetItemCount();
	// misyl: This triggers when you have a bunch of pending items to acknowledge from drops, disabling this assert for now.
	//AssertMsg( iMaxItemCount - iCurItemCount >= 0, "You have a negative number of backpack slots available - fix me!" );
	const int iBaseFreeSlots = MAX( 0, iMaxItemCount - iCurItemCount );
	_snwprintf( wszBackpackSlotCount, ARRAYSIZE( wszBackpackSlotCount ), L"%d", iBaseFreeSlots );

	// Breaking out bundles into individual items, etc., how many backpack slots will the
	// items in our cart take up?
	const int iItemsInCart = EconUI()->GetStorePanel()->GetCart()->GetTotalConcreteItems();

	if ( iItemsInCart == 0 || econ_never_show_items_in_cart_count.GetBool() )
	{
		g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#Store_FreeBackpackSpace" ), 1, wszBackpackSlotCount );
	}
	else
	{
		wchar_t wszCartCount[16];
		_snwprintf( wszCartCount, ARRAYSIZE( wszCartCount ), L"%d", iItemsInCart );

#if defined( TF_CLIENT_DLL )
		if ( IsFreeTrialAccount() )
		{
			wchar_t wszUpgradeSlotCount[16];
			_snwprintf( wszUpgradeSlotCount, ARRAYSIZE( wszUpgradeSlotCount ), L"%d", DEFAULT_NUM_BACKPACK_SLOTS - DEFAULT_NUM_BACKPACK_SLOTS_FREE_TRIAL_ACCOUNT );

			// We're a free trial account so we show the number of backpack slots we really have,
			// the number of slots we get as a bonus when purchasing, and then the number of items
			// in our cart.
			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#Store_FreeBackpackSpace_WithCartItems_WithUpgrade" ), 3, wszBackpackSlotCount, wszCartCount, wszUpgradeSlotCount );
		}
		else
#endif // defined( TF_CLIENT_DLL )
		{
			// We aren't a free trial account, so there is no account upgrade included in
			// this purchase, so fall back to showing the number of items in our cart.
			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#Store_FreeBackpackSpace_WithCartItems" ), 2, wszBackpackSlotCount, wszCartCount );
		}
	}
	
	SetDialogVariable( "freebackpackspace", wszLocalized );

	if ( m_pBackpackLabel )
	{
		const Color clrTooMany = ShouldUseNewStore() ? Color(200,80,60,255) : Color(255,0,0,255);
		m_pBackpackLabel->SetFgColor( InventoryManager()->GetLocalInventory()->CanPurchaseItems( iItemsInCart ) ? m_colBackpackOrg : clrTooMany );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::UpdateFilterComboBox( void )
{
	if ( !m_pFilterComboBox )
		return;

	m_pFilterComboBox->RemoveAll();

	// All items
	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "filter", FILTER_ALL_ITEMS );
	m_pFilterComboBox->AddItem( "#Store_ClassFilter_None", pKeyValues );

	pKeyValues->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::PreviewSelectionItem( void )
{
	if ( !m_pSelectedPanel )
		return;

	// Get the entry for the panel.
	const econ_store_entry_t *pEntry = GetSelectedEntry();
	if ( !pEntry )
		return;

	if ( !pEntry->CanPreview() )
		return;

	DoPreviewItem( pEntry->GetItemDefinitionIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::DoPreviewItem( item_definition_index_t usItemDef )
{
#ifdef TF_CLIENT_DLL
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "do_try_out_item", CFmtStr( "%i", usItemDef ).Access() );
#endif

	if ( usItemDef == InventoryManager()->GetLocalInventory()->GetPreviewItemDef() )
	{
		ShowMessageBox( "#ItemPreview_AlreadyPreviewTitle", "#ItemPreview_AlreadyPreviewText", "#GameUI_OK" );
		return;
	}

	// Send a message to the GC asking if this player can preview an item.
	GCSDK::CGCMsg< MsgGCCheckItemPreviewStatus_t > msg( k_EMsgGCItemPreviewCheckStatus );
	msg.Body().m_unItemDefIndex = usItemDef;

	// OGS LOGGING HERE

	GCClientSystem()->BSendMessage( msg );

	// Response is handled in item_rental_ui.cpp.
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePage::OnPreviewItem( KeyValues *pData )
{
	DoPreviewItem( pData->GetInt( "item_def_index" ) );
}
