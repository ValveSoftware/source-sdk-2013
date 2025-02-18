//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "crate_detail_panels.h"
#include "vgui_controls/TextImage.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"
#include "econ_ui.h"
#include <vgui/ISurface.h>
#include "econ_item_inventory.h"
#include "econ/tool_items/tool_items.h"

#define SHUFFLE_TIME 5.f

float CInputStringForItemBackpackOverlayDialog::m_sflNextShuffleTime = 0.f;
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CInputStringForItemBackpackOverlayDialog::CInputStringForItemBackpackOverlayDialog( vgui::Panel *pParent, CEconItemView *pItem, CEconItemView *pChosenKey )
	: vgui::EditablePanel( pParent, "InputStringForItemBackpackOverlayDialog" )
	, m_Item( *pItem )
	, m_pPreviewModelPanel( NULL )
	, m_pTextEntry( NULL )
	, m_pItemModelPanelKVs( NULL )
	, m_bUpdateRecieved( false )
{
	if ( pChosenKey )
	{
		m_UseableKey = *pChosenKey;
	}
	m_pPreviewModelPanel = new CItemModelPanel( this, "preview_model" );
	m_pTextEntry = new vgui::TextEntry( this, "TextEntryControl" );
	m_pShuffleButton = new CExButton( this, "ShuffleButton", "Shuffle" );
	m_pRareLootLabel = new CExLabel( this, "RareLootLabel", "#Econ_Revolving_Loot_List_Rare_Item" );
	m_pProgressBar = new vgui::ProgressBar( this, "ShuffleProgress" );
	m_pGetKeyButton = new CExButton( this, "GetKeyButton", "getkey" );
	m_pUseKeyButton = new CExButton( this, "UseKeyButton", "usekey" );

	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );
	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );

	ListenForGameEvent( "inventory_updated" );
}

CInputStringForItemBackpackOverlayDialog::~CInputStringForItemBackpackOverlayDialog()
{
	if ( m_pItemModelPanelKVs )
	{
		m_pItemModelPanelKVs->deleteThis();
		m_pItemModelPanelKVs = NULL;
	}

	m_vecContentsPanels.PurgeAndDeleteElements();
}

void CInputStringForItemBackpackOverlayDialog::FireGameEvent( IGameEvent *event )
{
	// If we're not visible, ignore all events
	if ( !IsVisible() )
		return;

	// Something caused our inventory to update.  Assuming it was from our shuffle
	// then we need to update ourselves.
	const char *type = event->GetName();
	if ( Q_strcmp( "inventory_updated", type ) == 0 )
	{
		m_bUpdateRecieved = true;
	}
}

void CInputStringForItemBackpackOverlayDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/InputStringForItemBackpackOverlayDialog.res" );

	CCrateLootListWrapper itemWrapper( &m_Item );
	const IEconLootList *pLootList = itemWrapper.GetEconLootList();
	// Set the crate footer text.  The crate itself specifies what to use.
	if ( pLootList->GetLootListFooterLocalizationKey() )
	{
		m_pRareLootLabel->SetText( pLootList->GetLootListFooterLocalizationKey() );
	}
	else
	{
		const char *pszRareLootListFooterLocalizationKey = m_Item.GetItemDefinition()->GetDefinitionString( "loot_list_rare_item_footer", "#Econ_Revolving_Loot_List_Rare_Item" );
		m_pRareLootLabel->SetText( pszRareLootListFooterLocalizationKey );
	}

	// Use the gradient border for the tooltip
	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );

	m_pPreviewModelPanel->SetItem( &m_Item );
	m_pPreviewModelPanel->SetActAsButton( false, false ); // Dont mess around with the mouse

	m_pTextEntry->RequestFocus();	
}

void CInputStringForItemBackpackOverlayDialog::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	// Pull out the model panel KVs for this panel
	KeyValues *pItemKV = inResourceData->FindKey( "modelpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pItemModelPanelKVs )
		{
			m_pItemModelPanelKVs->deleteThis();
		}
		m_pItemModelPanelKVs = new KeyValues( "modelpanels_kv" );
		pItemKV->CopySubkeys( m_pItemModelPanelKVs );
	}

	CreateItemPanels();
}

void CInputStringForItemBackpackOverlayDialog::CreateItemPanels()
{
	CCrateLootListWrapper itemWrapper( &m_Item );
	const IEconLootList *pLootList = itemWrapper.GetEconLootList();

	class CItemDefLootListIterator : public IEconLootList::IEconLootListIterator
	{
	public:
		CItemDefLootListIterator( CUtlVector< item_definition_index_t > *pVecItemDefs  )
			: m_pVecItemDefs( pVecItemDefs )
		{}

		virtual void OnIterate( item_definition_index_t unItemDefIndex ) OVERRIDE
		{
			const CEconItemDefinition *pItemDef = GetItemSchema()->GetItemDefinition( unItemDefIndex );
			if ( pItemDef && pItemDef->BValidForShuffle() )
			{
				m_pVecItemDefs->AddToTail( unItemDefIndex );
			}
		}

	private:
		CUtlVector< item_definition_index_t > * const m_pVecItemDefs;
	};

	// Get the drops from the item
	CUtlVector< item_definition_index_t > vecItemDefs;
	CItemDefLootListIterator it( &vecItemDefs );
	pLootList->EnumerateUserFacingPotentialDrops( &it );

	if ( !m_pItemModelPanelKVs )
		return;

	if ( m_vecContentsPanels.Count() != vecItemDefs.Count() )
	{
		m_vecContentsPanels.PurgeAndDeleteElements();

		FOR_EACH_VEC( vecItemDefs, i )
		{
			// Create new panel
			CItemModelPanel* pItemPanel = m_vecContentsPanels[ m_vecContentsPanels.AddToTail( new CItemModelPanel( this, CFmtStr( "item_preview_%d", i ) ) ) ];
			pItemPanel->ApplySettings( m_pItemModelPanelKVs );
			pItemPanel->InvalidateLayout( true );
			pItemPanel->SetActAsButton( false, true );	// Lets us get mouse enter/exit evens for tooltips
			pItemPanel->SetTooltip( m_pMouseOverTooltip, "" );	// Tooltip panel to use
		}
	}

	// Create the panels and set the items into them
	FOR_EACH_VEC( vecItemDefs, i )
	{
		const item_definition_index_t &itemDef = vecItemDefs[i];
			
		CItemModelPanel* pItemPanel = m_vecContentsPanels[i];

		CEconItemView item;
		item.SetItemDefIndex( itemDef );
		item.SetItemQuality( AE_UNIQUE );	// Unique by default
		item.SetItemLevel( 0 ); // Hide this?
		item.SetInitialized( true );
		item.SetItemOriginOverride( kEconItemOrigin_Invalid );

		pItemPanel->SetItem( &item );
	}
}

void CInputStringForItemBackpackOverlayDialog::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Find out how wide these panels will be side by side
	const int nBuffer = 5;
	int nTotalWide = 0;
	const int nCount = m_vecContentsPanels.Count();
	if ( nCount )
	{
		const int nWide = m_vecContentsPanels.Head()->GetWide();
		nTotalWide = (nCount * nWide) + ( (nCount - 1) * nBuffer );
	}

	// Find out how much space the panels take up within the parent
	int nParentWide = GetWide();
	int nDiff = nParentWide - nTotalWide;
	// How far we need to offset from the left edge
	int nStartOffset = nDiff / 2;

	// Place all the panels side by side
	FOR_EACH_VEC( m_vecContentsPanels, i )
	{
		CItemModelPanel* pItemPanel = m_vecContentsPanels[ i ];

		const int nWide = pItemPanel->GetWide();
		
		pItemPanel->SetPos( nStartOffset + i * (nWide + nBuffer), YRES(150) );
		pItemPanel->SetVisible( true );
	}

	// Which button to show
	m_pUseKeyButton->SetVisible( m_UseableKey.IsValid() );
	m_pGetKeyButton->SetVisible( !m_UseableKey.IsValid() );
}

void CInputStringForItemBackpackOverlayDialog::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "cancel", 6 ) )
	{
		TFModalStack()->PopModal( this );

		SetVisible( false );
		MarkForDeletion();
	}
	else if ( !Q_strnicmp( command, "shuffle", 7 ) )
	{
		// let the GC know
		if ( m_pTextEntry && Plat_FloatTime() >= m_sflNextShuffleTime )
		{
			// Set the next time they can send a request to shuffle
			m_sflNextShuffleTime = Plat_FloatTime() + SHUFFLE_TIME;

			enum { kMaxCodeStringSize = 32 };
			char szText[ kMaxCodeStringSize ] = { 0 };
			m_pTextEntry->GetText( &szText[0], sizeof( szText )  );

			GCSDK::CProtoBufMsg<CMsgGCShuffleCrateContents> msg( k_EMsgGCShuffleCrateContents );

			msg.Body().set_crate_item_id( m_Item.GetID() );
			msg.Body().set_user_code_string( szText );

			GCClientSystem()->BSendMessage( msg );

			m_pProgressBar->SetProgress( 0.f );
			m_pProgressBar->SetVisible( true );
			m_pTextEntry->SetVisible( false );

			vgui::surface()->PlaySound( "ui/itemcrate_shuffle.wav" );
		}
	}
	else if ( !Q_strnicmp( command, "getkey", 6 ) )
	{
		static CSchemaAttributeDefHandle pAttrDef_DecodedBy( "decoded by itemdefindex" );
			
		uint32 iDecodableItemDef = 0;
		if ( m_Item.FindAttribute( pAttrDef_DecodedBy, &iDecodableItemDef ) )
		{
			// casting to the proper type since our econ system is dumb
			const float& value_as_float = (float&)iDecodableItemDef;
			EconUI()->CloseEconUI();
			EconUI()->OpenStorePanel( (int)value_as_float, false );

			// close ourselves
			TFModalStack()->PopModal( this );
			SetVisible( false );
			MarkForDeletion();
		}
	}
	else if ( !Q_strnicmp( command, "usekey", 6 ) )
	{
		if ( m_UseableKey.IsValid() )
		{
			// Use the key
			ApplyTool( GetParent(), &m_UseableKey, &m_Item );
			// close ourselves
			TFModalStack()->PopModal( this );
			SetVisible( false );
			MarkForDeletion();
		}
	}
}

void CInputStringForItemBackpackOverlayDialog::FindUsableKey()
{
	static CSchemaAttributeDefHandle pAttrDef_DecodedBy( "decoded by itemdefindex" );
			
	uint32 iDecodableItemDef = 0;
	if ( m_Item.FindAttribute( pAttrDef_DecodedBy, &iDecodableItemDef ) )
	{
		const float& value_as_float = (float&)iDecodableItemDef;
		iDecodableItemDef = (float)value_as_float;
		CPlayerInventory *pInventory = InventoryManager()->GetLocalInventory();
		if ( !pInventory )
			return;

		for ( int i = 0; i < pInventory->GetItemCount(); i++ )
		{
			CEconItemView *pItem = pInventory->GetItem(i);
			if ( pItem->GetItemDefIndex() == iDecodableItemDef )
			{
				m_UseableKey = *pItem;
			}
		}
	}
}

void CInputStringForItemBackpackOverlayDialog::OnThink()
{
	float flDelta = m_sflNextShuffleTime - Plat_FloatTime();

	// If we're ready, show "Shuffle"
	if ( flDelta < 0 )
	{
		// Show the text entry, show the progress bar
		m_pProgressBar->SetVisible( false );
		m_pTextEntry->SetVisible( true );

		// Re-enable the shuffle/use buttons
		m_pShuffleButton->SetEnabled( m_pTextEntry->GetTextLength() != 0 );
		m_pUseKeyButton->SetEnabled( true );
		// Say "Shuffle"
		m_pShuffleButton->SetText( "#ShuffleContents" );

		// We got a inventory update message, update
		if ( m_bUpdateRecieved )
		{
			CreateItemPanels();
			m_bUpdateRecieved = false;
		}
	}
	else
	{
		// Show the progress bar, hide the text field
		m_pProgressBar->SetVisible( true );
		m_pTextEntry->SetVisible( false );

		// Dont allow clicking the shuffle or use key button
		m_pShuffleButton->SetEnabled( false );
		m_pUseKeyButton->SetEnabled( false );
		// Say "Shuffling..."
		m_pShuffleButton->SetText( "#ShufflingContents" );

		// Set progress
		float flProgress = ( SHUFFLE_TIME - flDelta ) / SHUFFLE_TIME;
		m_pProgressBar->SetProgress( flProgress );
	}
}

void CInputStringForItemBackpackOverlayDialog::Show()
{
	SetVisible( true );
	MakePopup();
	MoveToFront();
	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	TFModalStack()->PushModal( this );

	// If a key wasnt passed in, find the first one in the 
	// player's inventory
	if ( !m_UseableKey.IsValid() )
	{
		FindUsableKey();
	}

	// Which button to show
	m_pUseKeyButton->SetVisible( m_UseableKey.IsValid() );
	m_pGetKeyButton->SetVisible( !m_UseableKey.IsValid() );

	// Put the current gen code of the crate into the text field
	static CSchemaAttributeDefHandle pAttrDef_DecodedBy( "crate generation code" );
	const char *pszAttrGenCode;
	if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( &m_Item, pAttrDef_DecodedBy, &pszAttrGenCode ) )
	{
		m_pTextEntry->SetText( pszAttrGenCode );
	}
}


