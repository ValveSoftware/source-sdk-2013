//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store_page.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "gamestringpool.h"
#include "econ_item_inventory.h"
#include "econ_item_system.h"
#include "store_preview_item.h"
#include "item_model_panel.h"
#include "econ_ui.h"
#include "store/store_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CPreviewRotButton, CPreviewRotButton );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePreviewItemPanel::CStorePreviewItemPanel( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner )
:	EditablePanel( pParent, "storepreviewitem" )
{								   
	m_pOwner = pOwner;
	m_pResFile = pResFile != NULL ? pResFile : ( ShouldUseNewStore() ? "Resource/UI/econ/store/v2/StorePreviewItemPanel.res" : "Resource/UI/econ/store/v1/StorePreviewItemPanel.res" );
	m_pDataTextRichText = NULL;
	m_iCurrentIconPosition = 0;
	m_iState = PS_ITEM;
	m_pIconsMoveLeftButton = NULL;
	m_pIconsMoveRightButton = NULL;

	m_pItemFullImage = new CItemModelPanel( this, "PreviewItemModelPanel" );

	SetDialogVariable("selectiontitle", g_pVGuiLocalize->Find("#TF_NoSelection") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStorePreviewItemPanel::~CStorePreviewItemPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( m_pResFile );

	// Apply attribute changes to CItemModelPanel
	m_pItemFullImage->UpdatePanels();

	m_pIconsMoveLeftButton = dynamic_cast<CExButton*>( FindChildByName("IconsMoveLeftButton") );
	if ( m_pIconsMoveLeftButton )
	{
		m_pIconsMoveLeftButton->AddActionSignalTarget( this );
	}
	m_pIconsMoveRightButton = dynamic_cast<CExButton*>( FindChildByName("IconsMoveRightButton") );
	if ( m_pIconsMoveRightButton )
	{
		m_pIconsMoveRightButton->AddActionSignalTarget( this );
	}

	m_pDataTextRichText = dynamic_cast<CEconItemDetailsRichText*>( FindChildByName( "DetailsRichText" ) );
	if ( m_pDataTextRichText )
	{
		m_pDataTextRichText->SetURLClickedHandler( EconUI()->GetStorePanel() );
		m_pDataTextRichText->AllowItemSetLinks( true );
	}

	// Then find all our item icons
	m_pItemIcons.Purge();
	CStorePreviewItemIcon *pItemIcon = NULL;
	int iIcon = 1;
	do 
	{
		pItemIcon = dynamic_cast<CStorePreviewItemIcon*>( FindChildByName( VarArgs("ItemIcon%d",iIcon)) );
		if ( pItemIcon )
		{
			m_pItemIcons.AddToTail( pItemIcon );
			if ( m_pOwner )
			{
				pItemIcon->GetItemPanel()->SetTooltip( m_pOwner->GetItemTooltip(), "" );
			}
		}
		iIcon++;
	} while ( pItemIcon );

	// Update our item icons. Hide them all first. The code below will unhide ones used.
	for ( int i = 0; i < m_pItemIcons.Count(); i++ )
	{
		m_pItemIcons[i]->SetVisible( false );
	}

	// Start with the item itself showing
	SetState( PS_ITEM );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// center the icons
	int iNumItemIcons = 0;
	FOR_EACH_VEC( m_pItemIcons, i )
	{
		if ( m_pItemIcons[i]->IsVisible() )
		{
			++iNumItemIcons;
		}
	}
	if ( iNumItemIcons )
	{
		int iCenterX = GetWide() / 2;
		int interval = XRES(2);
		int totalWidth = (iNumItemIcons * m_pItemIcons[0]->GetWide()) + (interval * (iNumItemIcons - 1));
		int iX = iCenterX - ( totalWidth / 2 );

		int posX, posY;
		m_pItemIcons[0]->GetPos( posX, posY );

		int iButton = 0;
		for ( int i = 0; i < m_pItemIcons.Count(); i++ )
		{
			if ( m_pItemIcons[i]->IsVisible() )
			{
				m_pItemIcons[i]->SetPos( iX, posY );
				iX += m_pItemIcons[i]->GetWide() + interval;

				iButton++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "close", 5 ) )
	{
		PostActionSignal(new KeyValues("HidePreview"));
		SetVisible( false );
		return;
	}
	else if ( !Q_stricmp( command, "icons_left" ) )
	{
		m_iCurrentIconPosition = MAX( m_iCurrentIconPosition - 1, 0 );
		UpdateIcons();
	}
	else if ( !Q_stricmp( command, "icons_right" ) )
	{
		// It's only visible if we can still move right.
		m_iCurrentIconPosition++;
		UpdateIcons();
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
void CStorePreviewItemPanel::OnRotButtonDown( KeyValues *data )
{
	int iRotDelta = data->GetInt( "rot", 0 );
	m_iCurrentRotation = iRotDelta;
	vgui::ivgui()->AddTickSignal( GetVPanel(), 33 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::OnRotButtonUp( void )
{
	m_iCurrentRotation = 0;
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry /*= NULL*/ )
{
	m_iCurrentIconPosition = 0;
	m_item = *pItem;

	if ( m_item.IsValid() )
	{
		m_pItemFullImage->SetItem( &m_item );
		if ( m_pDataTextRichText )
		{
			m_pDataTextRichText->SetLimitedItem( pEntry && pEntry->m_bLimited );
			m_pDataTextRichText->UpdateDetailsForItem( m_item.GetItemDefinition() );
		}

		SetDialogVariable("selectiontitle", m_item.GetItemName() );

		CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName( "AddToCartButton" ) );
		if ( pButton )
		{
			const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();
			if ( pPriceSheet )
			{
				const econ_store_entry_t *pStoreEntry = pPriceSheet->GetEntry( pItem->GetItemDefIndex() );
				if ( pStoreEntry->m_bIsMarketItem )
				{
					SetDialogVariable( "storeaddtocart", g_pVGuiLocalize->Find( "#Store_ViewMarket" ) );
				}
				else
				{
					SetDialogVariable( "storeaddtocart", g_pVGuiLocalize->Find( "#Store_AddToCart" ) );
				}
			}
		}

	}

	InvalidateLayout();
	UpdateIcons();

	if ( m_iState == PS_PLAYER )
	{
		SetState( PS_ITEM );
	}

	Panel *pAddToCart = FindChildByName( "AddToCartButton" );
	if ( pAddToCart )
	{
		pAddToCart->RequestFocus();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::SetState( preview_state_t iState )
{
	// Only reset the position when moving from to items/details
	if ( iState == PS_DETAILS || iState == PS_ITEM )
	{
		m_iCurrentIconPosition = 0;
	}

	m_iState = iState;

	if ( m_pDataTextRichText )
	{
		m_pDataTextRichText->SetVisible( m_iState == PS_DETAILS );
	}
	m_pItemFullImage->SetVisible( m_iState == PS_ITEM );

	UpdateIcons();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::UpdateIcons( void )
{
	bool bAdditionalIcons = false;

	// Do the item icons first
	if ( m_iState == PS_DETAILS )
	{
		// Show as many of the items in the bundle as possible
		const CEconItemDefinition *pItemData = m_item.GetItemDefinition();
		if ( pItemData ) 
		{
			const bundleinfo_t *pBundleInfo = pItemData->GetBundleInfo();
			if ( pBundleInfo )
			{
				FOR_EACH_VEC( m_pItemIcons, i )
				{
					// If we haven't scrolled, the first item is the bundle itself
					if ( m_iCurrentIconPosition == 0 && i == 0 )
					{
						m_pItemIcons[0]->SetItem( 0, &m_item );
						continue;
					}

					int iItemPos = (i - 1 + m_iCurrentIconPosition); 
					if ( pBundleInfo->vecItemDefs.Count() > iItemPos && pBundleInfo->vecItemDefs[iItemPos] )
					{
						m_pItemIcons[i]->SetItem( i, pBundleInfo->vecItemDefs[iItemPos]->GetDefinitionIndex() );
						m_pItemIcons[i]->SetVisible( true );
					}
					else
					{
						m_pItemIcons[i]->SetVisible( false );
					}
				}

				bAdditionalIcons = (m_iCurrentIconPosition + m_pItemIcons.Count()) <= pBundleInfo->vecItemDefs.Count();
			}
			else if ( m_pItemIcons.Count() > 0 )
			{
				m_pItemIcons[0]->SetVisible( true );
				m_pItemIcons[0]->SetItem( 0, &m_item );
				FOR_EACH_VEC( m_pItemIcons, i )
				{
					if ( i != 0 )
					{
						m_pItemIcons[i]->SetVisible( false );
					}
				}
			}
		}
	}
	else
	{
		// Hide all item icons first (but not the first if we haven't scrolled)
		FOR_EACH_VEC( m_pItemIcons, i )
		{
			m_pItemIcons[i]->SetVisible( m_iCurrentIconPosition == 0 && i == 0 );
		}

		// First icon is always the store entry (item/bundle), if we haven't scrolled right
		if ( m_iCurrentIconPosition == 0 && m_pItemIcons.Count() )
		{
			m_pItemIcons[0]->SetItem( 0, &m_item );
		}
	}

	if( m_pIconsMoveLeftButton )
		m_pIconsMoveLeftButton->SetVisible( (m_iCurrentIconPosition > 0) );
	if( m_pIconsMoveRightButton )
		m_pIconsMoveRightButton->SetVisible( bAdditionalIcons );

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( !IsVisible() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CStorePreviewItemPanel::OnItemIconSelected( KeyValues *data )
{
	if ( m_iState == PS_DETAILS )
	{
		int iIcon = data->GetInt( "icon", 0 );
		CEconItemView *pItem = m_pItemIcons[iIcon]->GetItemPanel()->GetItem();
		if ( pItem )
		{
			if ( m_pDataTextRichText )
			{
				m_pDataTextRichText->UpdateDetailsForItem( pItem->GetStaticData() );
			}
			SetDialogVariable("selectiontitle", pItem->GetItemName() );
		}
	}
	else
	{
		SetState( PS_ITEM );
	}
}

//================================================================================================================
// PREVIEW ROT BUTTON
//================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPreviewRotButton::OnMousePressed(vgui::MouseCode code)
{
	BaseClass::OnMousePressed( code );

	if ( IsSelected() )
	{
		KeyValues *pCommand = GetCommand();
		PostActionSignal(new KeyValues("RotButtonDown", "rot", pCommand->GetString("command", "0") ));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPreviewRotButton::OnMouseReleased(vgui::MouseCode code)
{
	if ( IsSelected() )
	{
		PostActionSignal(new KeyValues("RotButtonUp"));
	}

	BaseClass::OnMouseReleased( code );
}
