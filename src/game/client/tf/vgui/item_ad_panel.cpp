//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "item_ad_panel.h"
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "econ_store.h"
#include "econ_ui.h"
#include "store/store_panel.h"
#include "tf_controls.h"
#include "econ_item_description.h"
#include "vgui/IInput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseAdPanel::CBaseAdPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAdPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_flPresentTime = inResourceData->GetFloat( "present_time", 10.f );
}

bool CBaseAdPanel::CheckForRequiredSteamComponents( const char* pszSteamRequried, const char* pszOverlayRequired )
{
	// Make sure we've got the appropriate connections to Steam
	if ( !steamapicontext || !steamapicontext->SteamUtils() )
	{
		OpenStoreStatusDialog( NULL, pszSteamRequried, true, false );
		return false;
	}

	if ( !steamapicontext->SteamUtils()->IsOverlayEnabled() )
	{
		OpenStoreStatusDialog( NULL, pszOverlayRequired, true, false );
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemAdPanel::CItemAdPanel( Panel *parent, const char *panelName, item_definition_index_t itemDefIndex )
	: BaseClass( parent, panelName )
	, m_bShowMarketButton( true )
{
	m_item.Init( itemDefIndex, AE_UNIQUE, 1, 1 );
	SetDialogVariable( "price", "..." );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bLoadingControls = true;
	LoadControlSettings( GetItemDef()->GetAdResFile() );
	m_bLoadingControls = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	if ( !m_bLoadingControls )
	{
		m_bShowItemName = inResourceData->GetBool( "show_name", true );
		m_bShowAdText = inResourceData->GetBool( "show_ad_text", true );
		m_bShowBackground = inResourceData->GetBool( "show_background", true );
		m_bShowMarketButton = inResourceData->GetBool( "show_market", true ); // Default to showing market
	}

	if ( !m_bShowMarketButton )
	{
		// Tick every second as we try to get our price from the store
		vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	const CTFItemDefinition* pItemDef = GetItemDef();

	// Get the ad text for the item.  If it's not there, juse use the description text.
	SetDialogVariable( "item_name", m_item.GetItemName() );
	const char* pszAdtext = pItemDef->GetAdTextToken() ? pItemDef->GetAdTextToken() : pItemDef->GetItemDesc();
	CExScrollingEditablePanel* pScrollableItemText = FindControl< CExScrollingEditablePanel >( "ScrollableItemText", true );
	if ( pszAdtext && pScrollableItemText )
	{
		pScrollableItemText->SetDialogVariable( "item_ad_text", g_pVGuiLocalize->Find( pszAdtext ) );

		Label* pAdLabel = pScrollableItemText->FindControl< Label >( "ItemAdText", true );
		if ( pAdLabel )
		{
			int nWide, nTall;
			pAdLabel->InvalidateLayout( true );
			pAdLabel->GetContentSize( nWide, nTall );
			pAdLabel->SetTall( nTall );
		}

		pScrollableItemText->InvalidateLayout( true );
	}

	CExButton* pBuyButton = FindControl< CExButton >( "BuyButton", true );
	CExButton* pMarketButton = FindControl< CExButton >( "MarketButton", true );
	if ( pBuyButton && pMarketButton )
	{
		
		pBuyButton->SetVisible( !m_bShowMarketButton );
		pMarketButton->SetVisible( m_bShowMarketButton );
	}

	CExLabel* pNameLabel = FindControl< CExLabel >( "ItemName", true );
	int nNameLabelTall = 0;
	if ( pNameLabel )
	{
		// Set the name to the quality color
		// Rarity Econ Colorization
		const char* pszRarityColor = GetItemSchema()->GetRarityColor( m_item.GetRarity() );
		EEconItemQuality eQuality = (EEconItemQuality)m_item.GetItemQuality();
		if ( pszRarityColor && eQuality != AE_SELFMADE )
		{
			pNameLabel->SetColorStr( pszRarityColor );
		}

		pNameLabel->SizeToContents();
		nNameLabelTall = pNameLabel->GetTall();
		pNameLabel->SetTall( nNameLabelTall + YRES( 2 ) );
	}

	if ( pScrollableItemText && pNameLabel )
	{
		pScrollableItemText->SetPos( pScrollableItemText->GetXPos(), pNameLabel->GetYPos() + nNameLabelTall );
	}

	SetControlVisible( "ItemName", m_bShowItemName );
	SetControlVisible( "ScrollableItemText", m_bShowAdText );
	SetControlVisible( "Background", m_bShowBackground );

	SetupItemPanel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::SetupItemPanel()
{
	CItemModelPanel* pItemImage = FindControl< CItemModelPanel >( "ItemIcon" );
	if ( pItemImage )
	{

		pItemImage->InvalidateLayout( true, true );
		pItemImage->SetItem( &m_item );

		KeyValuesAD modelpanelKV( "modelpanel_kv" );
		KeyValues *itemKV = new KeyValues( "itemmodelpanel" );
		itemKV->SetBool( "inventory_image_type", true );
		itemKV->SetBool( "use_item_rendertarget", false );
		itemKV->SetBool( "allow_rot", false );

		modelpanelKV->AddSubKey( itemKV );
		pItemImage->ApplySettings( modelpanelKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::OnTick()
{
	const CTFItemDefinition* pItemDef = GetItemDef();
	bool bStoreIsReady = EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetPriceSheet() && EconUI()->GetStorePanel()->GetCart() && steamapicontext && steamapicontext->SteamUser() && pItemDef;
	if ( bStoreIsReady )
	{
		// Get the price of the item
		const ECurrency eCurrency = EconUI()->GetStorePanel()->GetCurrency();
		const econ_store_entry_t *pEntry = EconUI()->GetStorePanel()->GetPriceSheet()->GetEntry( pItemDef->GetDefinitionIndex() );
		if ( pEntry )
		{
			item_price_t unPrice = pEntry->GetCurrentPrice( eCurrency );
			// Set that price into the button
			wchar_t wzLocalizedPrice[ kLocalizedPriceSizeInChararacters ];
			MakeMoneyString( wzLocalizedPrice, ARRAYSIZE( wzLocalizedPrice ), unPrice, eCurrency );
			SetDialogVariable( "price", wzLocalizedPrice );

			// Don't need to tick anymore
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const CTFItemDefinition* CItemAdPanel::GetItemDef() const
{
	return (CTFItemDefinition*)m_item.GetItemDefinition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::SetItemTooltip( CItemModelPanelToolTip* pItemToolTip )
{
	CItemModelPanel* pItemImage = FindControl< CItemModelPanel >( "ItemIcon" );
	if ( pItemImage )
	{
		pItemImage->SetTooltip( pItemToolTip, NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemAdPanel::OnCommand( const char *command )
{
	if ( FStrEq( "purchase", command ) )
	{
		if ( !CheckForRequiredSteamComponents( "#StoreUpdate_SteamRequired", "#MMenu_OverlayRequired" ) )
			return;

		const CTFItemDefinition* pItemDef = GetItemDef();
		if ( pItemDef )
		{
			if ( EconUI()->GetStorePanel() && EconUI()->GetStorePanel()->GetPriceSheet() && EconUI()->GetStorePanel()->GetCart() && steamapicontext && steamapicontext->SteamUser() )
			{
				// Add a the item to the users cart and checkout
				EconUI()->GetStorePanel()->GetCart()->EmptyCart();
				AddItemToCartHelper( NULL, pItemDef->GetDefinitionIndex(), kCartItem_Purchase );
				EconUI()->GetStorePanel()->InitiateCheckout( true );
			}
		}
	}
	else if ( FStrEq( "market", command ) ) 
	{
		if ( !CheckForRequiredSteamComponents( "#StoreUpdate_SteamRequired", "#MMenu_OverlayRequired" ) )
			return;

		const CTFItemDefinition* pItemDef = GetItemDef();
		if ( pItemDef && steamapicontext && steamapicontext->SteamFriends() )
		{
			const char *pszPrefix = "";
			if ( GetUniverse() == k_EUniverseBeta )
			{
				pszPrefix = "beta.";
			}

			static char pszItemName[256];
			g_pVGuiLocalize->ConvertUnicodeToANSI( g_pVGuiLocalize->Find ( pItemDef->GetItemBaseName() ) , pszItemName, sizeof(pszItemName) );

			char szURL[512];
			V_snprintf( szURL, sizeof(szURL), "http://%ssteamcommunity.com/market/listings/%d/%s", pszPrefix, engine->GetAppID(), pszItemName );
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
		}
	}
}



DECLARE_BUILD_FACTORY( CCyclingAdContainerPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCyclingAdContainerPanel::CCyclingAdContainerPanel( Panel *parent, const char *panelName )
	: BaseClass( parent, panelName )
	, m_pAdsContainer( NULL )
	, m_pKVItems( NULL )
	, m_nCurrentIndex( 0 )
	, m_nXPos( 0 )
	, m_nTargetIndex( 0 )
	, m_nTransitionStartOffsetX( 0 )
	, m_bTransitionRight( true )
	, m_bSettingsApplied( false )
	, m_bNeedsToCreatePanels( false )
{
	m_pAdsContainer = new EditablePanel( this, "AdsContainer" );
	m_pFadePanel = new EditablePanel( this, "FadeTransition" );
	m_pNextButton = new CExButton( this, "NextButton", ">", this, "next" );
	m_pPrevButton = new CExButton( this, "PrevButton", "<", this, "prev" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCyclingAdContainerPanel::~CCyclingAdContainerPanel()
{
	if ( m_pKVItems )
	{
		m_pKVItems->deleteThis();
		m_pKVItems = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/CyclingAdContainer.res" );

	m_bSettingsApplied = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues* pKVItems = inResourceData->FindKey( "items" );
	if ( pKVItems )
	{
		BSetItemKVs( pKVItems );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::CreatePanels()
{
	if ( ItemSystem()->GetItemSchema()->GetVersion() == 0 )
		return;

	FOR_EACH_VEC( m_vecPossibleAds, i )
	{
		m_vecPossibleAds[ i ].m_pAdPanel->MarkForDeletion();
	}

	m_vecPossibleAds.Purge();
	m_nCurrentIndex = 0;

	FOR_EACH_TRUE_SUBKEY( m_pKVItems, pKVItem )
	{
		const char* pszItemName = pKVItem->GetString( "item" );
		const CEconItemDefinition *pDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName( pszItemName );
		if ( pDef )
		{
			AdData_t& adData = m_vecPossibleAds[ m_vecPossibleAds.AddToTail() ];
			adData.m_pAdPanel = new CItemAdPanel( m_pAdsContainer, "ad", pDef->GetDefinitionIndex() );

			adData.m_pAdPanel->InvalidateLayout( true, true );	// Default settings
			adData.m_pAdPanel->ApplySettings( pKVItem );		
			adData.m_pAdPanel->InvalidateLayout();		
		}
		else
		{
			AssertMsg( 0, "Invalid item def '%s'!", pszItemName );
		}
	}

	// Queue up a transition
	if ( m_vecPossibleAds.Count() )
	{
		m_ShowTimer.Start( m_vecPossibleAds[ m_nCurrentIndex ].m_pAdPanel->GetPresentTime() );
	}

	m_bNeedsToCreatePanels = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	PresentIndex( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::OnThink() 
{
	BaseClass::OnThink();

	if ( m_bNeedsToCreatePanels && m_bSettingsApplied )
	{
		CreatePanels();
		PresentIndex( 0 );
	}

	UpdateAdPanelPositions();

	// See if it's time to auto-cycle to the next ad
	if ( m_ShowTimer.HasStarted() && m_ShowTimer.IsElapsed() && m_vecPossibleAds.Count() > 1 )
	{
		m_ShowTimer.Invalidate();
		PresentIndex( m_nTargetIndex + 1 );
	}

	int nMouseX, nMouseY;
	vgui::input()->GetCursorPos( nMouseX, nMouseY );
	bool bControlsVisible = IsWithin( nMouseX, nMouseY ) && m_vecPossibleAds.Count() > 1;
	m_pPrevButton->SetVisible( bControlsVisible );
	m_pNextButton->SetVisible( bControlsVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCyclingAdContainerPanel::BSetItemKVs( KeyValues* pKVItems )
{
	// Go through all of our panels and see if there's any changes.  Making item model panels
	// is *very* expensive so let's not if possible.
	bool bAllFound = true;
	FOR_EACH_TRUE_SUBKEY( pKVItems, pKVItem )
	{
		bool bItemFound = false;
		const char* pszItemName = pKVItem->GetString( "item" );
		const CEconItemDefinition *pDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName( pszItemName );
		if ( pDef )
		{
			FOR_EACH_VEC( m_vecPossibleAds, i )
			{
				CItemAdPanel* pAdPanel = assert_cast< CItemAdPanel* >( m_vecPossibleAds[ i ].m_pAdPanel );
				if ( pAdPanel && pAdPanel->GetItemDef() == pDef )
				{
					bItemFound = true;
				}
			}
		}

		if ( !bItemFound )
			bAllFound = false;
	}

	// We've already got all these items.  Bail.
	if ( bAllFound )
		return false;

	if ( pKVItems )
	{
		if ( m_pKVItems )
		{
			m_pKVItems->deleteThis();
			m_pKVItems = NULL;
		}
		m_pKVItems = pKVItems->MakeCopy();
	}

	m_bNeedsToCreatePanels = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::OnCommand( const char *command )
{
	if ( FStrEq( "next", command ) )
	{
		PresentIndex( m_nTargetIndex + 1 );
	}
	else if ( FStrEq( "prev", command ) )
	{
		PresentIndex( m_nTargetIndex - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::PresentIndex( int nIndex )
{
	if ( m_vecPossibleAds.IsEmpty() )
		return;

	if ( m_nCurrentIndex == nIndex )
		return;

	// Figure out which way we want to transition
	m_bTransitionRight = nIndex > m_nCurrentIndex;

	// Wrap if needed
	if ( nIndex >= m_vecPossibleAds.Count() )
	{
		nIndex = 0;
	}
	else if ( nIndex < 0 )
	{
		nIndex = m_vecPossibleAds.Count() - 1;
	}

	m_nTargetIndex = nIndex;

	// If they click more times while transitioning out, just change the target.  If we're
	// into transitioning in to the next panel, then we need to start the whole thing over.
	if ( !IsTransitioningOut() )
	{
		m_nTransitionStartOffsetX = m_nXPos;
		float flTransitionTime = 0.6f;
		m_TransitionTimer.Start( flTransitionTime );

		m_ShowTimer.Start( flTransitionTime + m_vecPossibleAds[ m_nCurrentIndex ].m_pAdPanel->GetPresentTime() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCyclingAdContainerPanel::UpdateAdPanelPositions()
{
	// Figure out how far along a transition we are
	float flPercent = Clamp( m_TransitionTimer.GetElapsedTime() / m_TransitionTimer.GetCountdownDuration(), 0.f, 1.f );
	flPercent = Gain( flPercent, 0.8f );

	// At a certain point, we're no longer transitioning out the old -- we're transitioning in the new
	const float flTransitionCutOff = m_TransitionTimer.GetCountdownDuration() / 2.f;
	bool bTransitionOut = flPercent < flTransitionCutOff;

	int nStartX = 0;
	int nTargetX = 0;
	float flFadeAmount = 0.f;

	if ( bTransitionOut )
	{
		nStartX = m_nTransitionStartOffsetX;
		nTargetX = m_bTransitionRight ? -100 : 100;
		flFadeAmount = RemapValClamped( flPercent, 0.f, flTransitionCutOff * 0.75f, 0.f, 255.f );
	}
	else
	{
		// Once we've passed the middle, show the target
		m_nCurrentIndex = m_nTargetIndex;
		nStartX = m_bTransitionRight ? 100 : -100;
		nTargetX = 0;
		flFadeAmount = RemapValClamped( flPercent, flTransitionCutOff * 1.25f, 1.f, 255.f, 0.f );
	}

	// Alpha fades up entirely near the middle to cover the swap
	m_pFadePanel->SetAlpha( flFadeAmount );

	m_nXPos = RemapVal( flPercent, 0.f, 1.f, nStartX, nTargetX );
	FOR_EACH_VEC( m_vecPossibleAds, i )
	{
		m_vecPossibleAds[i].m_pAdPanel->SetPos( m_nXPos, m_vecPossibleAds[i].m_pAdPanel->GetYPos() );
		m_vecPossibleAds[i].m_pAdPanel->SetVisible( i == m_nCurrentIndex );
	}
}

float CCyclingAdContainerPanel::GetTransitionProgress() const
{
	float flPercent = Clamp( m_TransitionTimer.GetElapsedTime() / m_TransitionTimer.GetCountdownDuration(), 0.f, 1.f );
	return Gain( flPercent, 0.8f );
}

bool CCyclingAdContainerPanel::IsTransitioningOut() const
{
	const float flTransitionCutOff = m_TransitionTimer.GetCountdownDuration() / 2.f;
	return GetTransitionProgress() < flTransitionCutOff;
}