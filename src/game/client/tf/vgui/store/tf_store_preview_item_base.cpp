//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "store/tf_store_preview_item_base.h"
#include "store/store_page.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "gamestringpool.h"
#include "tf_item_inventory.h"
#include "tf_playermodelpanel.h"
#include "econ_item_system.h"
#include "item_model_panel.h"
#include "c_tf_gamestats.h"
#include "econ_ui.h"
#include "econ_item_tools.h"
#include "vgui_controls/MenuItem.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFStorePreviewItemPanelBase::CTFStorePreviewItemPanelBase( vgui::Panel *pParent, const char *pResFile, const char *pPanelName, CStorePage *pOwner )
:	CStorePreviewItemPanel( pParent, pResFile, "storepreviewitem", pOwner )
{								   
	ResetHandles();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::ResetHandles( void )
{
	m_pPlayerModelPanel = NULL;
	m_iCurrentClass = TF_CLASS_SCOUT;
	m_iCurrentHeldItem = 0;
	m_pClassIconMouseoverLabel = NULL;
	m_pRotRightButton = NULL;
	m_pRotLeftButton = NULL;
	m_pNextWeaponButton = NULL;
	m_pZoomButton = NULL;
	m_pOptionsButton = NULL;
	m_pTeamButton = NULL;
	m_pDataTextRichText = NULL;
	m_iCurrentIconPosition = 0;
	m_iState = PS_ITEM;
	m_unPaintDef = 0;
	m_unPaintRGB0 = 0;
	m_unPaintRGB1 = 0;
	m_pPaintNameLabel = NULL;
	m_pStyleNameLabel = NULL;
	m_pCustomizeMenu = NULL;
	m_pClassIcons.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetCycleLabelText( vgui::Label *pTargetLabel, const char *pCycleText )
{
	if ( pTargetLabel )
	{
		pTargetLabel->SetText( pCycleText );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	ResetHandles();

	BaseClass::ApplySchemeSettings( pScheme );

	m_pPlayerModelPanel = dynamic_cast<CTFPlayerModelPanel*>( FindChildByName("classmodelpanel") );
	m_pClassIconMouseoverLabel = dynamic_cast<vgui::Label*>( FindChildByName("ClassUsageMouseoverLabel") );
	m_pRotRightButton = dynamic_cast<CExButton*>( FindChildByName("RotRightButton") );
	m_pRotLeftButton = dynamic_cast<CExButton*>( FindChildByName("RotLeftButton") );
	m_pNextWeaponButton = dynamic_cast<CExButton*>( FindChildByName("NextWeaponButton") );
	m_pZoomButton = dynamic_cast<CExButton*>( FindChildByName("ZoomButton") );
	m_pOptionsButton = dynamic_cast<CExButton*>( FindChildByName("OptionsButton") );
	m_pTeamButton = dynamic_cast<CExButton*>( FindChildByName("TeamButton") );
	m_pPaintNameLabel = dynamic_cast<vgui::Label*>( FindChildByName("PaintNameLabel") );
	m_pStyleNameLabel = dynamic_cast<vgui::Label*>( FindChildByName("StyleNameLabel") );

	if ( m_pClassIconMouseoverLabel )
	{
		m_pClassIconMouseoverLabel->SetVisible( false );
	}

	// Find all the class images
	CStorePreviewClassIcon *pClassImage = NULL;
	int iIcon = 1;
	do 
	{
		pClassImage = dynamic_cast<CStorePreviewClassIcon*>( FindChildByName( VarArgs("ClassUsageImage%d",iIcon)) );
		if ( pClassImage )
		{
			m_pClassIcons.AddToTail( pClassImage );
		}
		iIcon++;
	} while ( pClassImage );

	// Update our class icons. Hide them all first. The code below will unhide ones used.
	for ( int i = 0; i < m_pClassIcons.Count(); i++ )
	{
		m_pClassIcons[i]->SetVisible( false );
	}

	SetState( PS_ITEM );

	m_vecPaintCans.Purge();
	const CEconItemSchema::ToolsItemDefinitionMap_t &toolDefs = GetItemSchema()->GetToolsItemDefinitionMap();

	// Store all of the active paint can item defs
	FOR_EACH_MAP_FAST( toolDefs, i )
	{
		const CEconItemDefinition *pItemDef = toolDefs[i];

		// ignore everything that is not a paint can tool
		const IEconTool *pEconTool = pItemDef->GetEconTool();
		if ( pEconTool && !V_strcmp( pEconTool->GetTypeName(), "paint_can" ) ) 
		{
			m_vecPaintCans.AddToTail( pItemDef->GetDefinitionIndex() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// center the icons (we need to redo some of the work of CStorePreviewItemPanel, because we 
	// center the base item icons along with our TF specific class ones)
	int iNumItemIcons = 0;
	FOR_EACH_VEC( m_pItemIcons, i )
	{
		if ( m_pItemIcons[i]->IsVisible() )
		{
			++iNumItemIcons;
		}
	}
	int iNumClassIcons = 0;
	FOR_EACH_VEC( m_pClassIcons, i )
	{
		if ( m_pClassIcons[i]->IsVisible() )
		{
			++iNumClassIcons;
		}
	}
	if ( iNumItemIcons || iNumClassIcons )
	{
		int iCenterX = GetWide() / 2;
		int interval = XRES(2);
		int totalWidth = (iNumItemIcons * m_pItemIcons[0]->GetWide()) + (iNumClassIcons * m_pClassIcons[0]->GetWide()) + (interval * (iNumItemIcons + iNumClassIcons - 1));
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

		for ( int i = 0; i < m_pClassIcons.Count(); i++ )
		{
			if ( m_pClassIcons[i]->IsVisible() )
			{
				m_pClassIcons[i]->SetPos( iX, posY );
				iX += m_pClassIcons[i]->GetWide() + interval;

				iButton++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool IsAnythingPaintable( const CUtlVector<CEconItemView*>& vecItems )
{
	FOR_EACH_VEC( vecItems, i )
	{
		if ( vecItems[i]->GetStaticData()->GetCapabilities() & ITEM_CAP_PAINTABLE )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void UpdatePaintColorsForTeam( CTFPlayerModelPanel *pPlayerModelPanel, uint32 unRGB0, uint32 unRGB1 )
{
	Assert( pPlayerModelPanel );

	static CSchemaAttributeDefHandle pAttrDef_ItemTintRGB( "set item tint RGB" );

	if ( !pAttrDef_ItemTintRGB )
		return;

	const CUtlVector<CEconItemView*> &items = pPlayerModelPanel->GetCarriedItems();
	if ( !IsAnythingPaintable( items ) )
		return;
	
	for ( int i=0; i<items.Count(); ++i )
	{
		if ( items[i]->GetStaticData()->GetCapabilities() & ITEM_CAP_PAINTABLE )
		{
			items[i]->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_ItemTintRGB, pPlayerModelPanel->GetTeam() == TF_TEAM_RED ? unRGB0 : unRGB1 );
			items[i]->InvalidateColor();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "team_toggle", 11 ) )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->SetTeam( m_pPlayerModelPanel->GetTeam() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED );
		}

		// Also toggle team paint color if necessary.
		if ( m_unPaintRGB0 != m_unPaintRGB1 )
		{
			UpdatePaintColorsForTeam( m_pPlayerModelPanel, m_unPaintRGB0, m_unPaintRGB1 );
		}
	}
	else if ( !Q_strnicmp( command, "zoom_toggle", 11 ) )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->ToggleZoom();
		}
	}
	else if ( !Q_strnicmp( command, "paint_toggle", 12 ) )
	{
		CyclePaint();
	}
	else if ( !Q_strnicmp( command, "set_red", 7 ) )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->SetSkin( 0 );
		}
		return;
	}
	else if ( !Q_strnicmp( command, "set_blu", 7 ) )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->SetSkin( 1 );
		}
		return;
	}
	else if ( !Q_strnicmp( command, "next_weapon", 11 ) )
	{
		if ( m_pPlayerModelPanel )
		{
			const CUtlVector<CEconItemView*> &items = m_pPlayerModelPanel->GetCarriedItems();
			int iLastItem = m_iCurrentHeldItem;
			do 
			{
				m_iCurrentHeldItem = ( m_iCurrentHeldItem + 1 ) % items.Count();
			} while ( m_iCurrentHeldItem != iLastItem  && m_pPlayerModelPanel->HoldItem( m_iCurrentHeldItem ) == false );		
		}		
		m_pPlayerModelPanel->SetTeam( m_pPlayerModelPanel->GetTeam() );
		return;
	}
	else if ( !Q_strnicmp( command, "next_style", 10 ) )
	{
		CycleStyle();
		return;
	}
	else if ( V_strncasecmp( command, "SetPaint", V_strlen( "SetPaint" ) ) == 0 )
	{
		item_definition_index_t iItemDef = V_atoi( &command[ V_strlen( "SetPaint" ) ] );
		SetPaint( iItemDef );
	}
	else if ( V_strncasecmp( command, "SetStyle", V_strlen( "SetStyle" ) ) == 0 )
	{
		style_index_t unStyle = V_atoi( &command[ V_strlen( "SetStyle" ) ] );
		SetStyle( unStyle );
	}
	else if ( V_strncasecmp( command, "SetUnusual", V_strlen( "SetUnusual" ) ) == 0 )
	{
		int iUnusual = V_atoi( &command[ V_strlen( "SetUnusual" ) ] );
		SetUnusual( iUnusual );
	}
	else if ( FStrEq( command, "options" ) )
	{
		UpdateCustomizeMenu();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::OnClassIconSelected( KeyValues *data )
{
	int iClass = data->GetInt( "class", 0 );
	if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
	{
		iClass = TF_CLASS_SCOUT;
	}
	m_iCurrentClass = iClass;
	UpdateModelPanel();

	SetState( PS_PLAYER );

//	C_CTF_GameStats.Event_Store( IE_STORE_ITEM_PREVIEWED, NULL, NULL, m_iCurrentClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetPlayerModelVisible( bool bVisible )
{
	if( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->SetVisible( bVisible );
		if ( m_pRotRightButton )
		{
			m_pRotRightButton->SetVisible( bVisible );
		}
		if ( m_pRotLeftButton )
		{
			m_pRotLeftButton->SetVisible( bVisible );
		}
		UpdatePlayerModelButtons();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry )
{
	m_iCurrentClass = 0;
	BaseClass::PreviewItem( iClass, pItem, pEntry );

	UpdateModelPanel();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetState( preview_state_t iState )
{
	BaseClass::SetState( iState );
	SetPlayerModelVisible( m_iState == PS_PLAYER );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFStorePreviewItemPanelBase::GetPreviewTeam() const
{
	return m_pPlayerModelPanel
		 ? m_pPlayerModelPanel->GetTeam()
		 : TF_TEAM_RED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateIcons( void )
{
	// Don't bother calling back to the base class UpdateIcons, because 
	// we'd need to redo it all with the class icons factored in.

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
			else if ( m_pItemIcons.Count() )
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

		// Hide all the class icons
		for ( int i = 0; i < m_pClassIcons.Count(); i++ )
		{
			m_pClassIcons[i]->SetVisible( false );
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

		// Then do the class icons
		const CTFItemDefinition *pItemData = m_item.GetItemDefinition();
		if ( pItemData )
		{
			int iButton = 0;
			int iMaxButtons = m_pClassIcons.Count();
			int iNumClasses = (TF_LAST_NORMAL_CLASS - TF_FIRST_NORMAL_CLASS);
			// we show one less class icon when the item is visible
			if ( iMaxButtons < iNumClasses && m_iCurrentIconPosition == 0 )
			{
				iMaxButtons -= 1;
			}
			for ( int iClass = TF_FIRST_NORMAL_CLASS + m_iCurrentIconPosition; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
			{
				if ( !pItemData->CanBeUsedByClass(iClass) )
					continue;

				// Run out of buttons?
				if ( iButton >= iMaxButtons )
				{
					bAdditionalIcons = true;
					break;
				}

				m_pClassIcons[iButton]->SetVisible( true );
				m_pClassIcons[iButton]->SetClass(iClass);
				iButton++;

				if ( !m_iCurrentClass )
				{
					m_iCurrentClass = iClass;
				}
			}
			for ( ; iButton < m_pClassIcons.Count(); ++iButton )
			{
				m_pClassIcons[iButton]->SetVisible( false );
			}
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
void CTFStorePreviewItemPanelBase::OnTick( void )
{
	BaseClass::OnTick();

	if ( !IsVisible() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		return;
	}

	if ( m_iCurrentRotation )
	{
		m_pPlayerModelPanel->RotateYaw( m_iCurrentRotation );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateModelPanel()
{
	if ( m_pPlayerModelPanel )
	{
		m_iCurrentHeldItem = 0;
		m_pPlayerModelPanel->ClearCarriedItems();
		m_pPlayerModelPanel->SetToPlayerClass( m_iCurrentClass );

		if ( m_item.IsValid() )
		{
			CTFItemDefinition *pItemDef = m_item.GetStaticData();
			if ( pItemDef->GetBundleInfo() != NULL )
			{
				const bundleinfo_t *pBundleInfo = pItemDef->GetBundleInfo();
				FOR_EACH_VEC( pBundleInfo->vecItemDefs, i )
				{
					CTFItemDefinition *pBundledItem = dynamic_cast<CTFItemDefinition *>( pBundleInfo->vecItemDefs[i] );
					if ( pBundledItem && pBundledItem->CanBeUsedByClass( m_iCurrentClass ) )
					{
						CEconItemView bundleItemData;
						bundleItemData.Init( pBundledItem->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
						bundleItemData.SetClientItemFlags( kEconItemFlagClient_Preview );
						int iItemIdx = m_pPlayerModelPanel->AddCarriedItem( &bundleItemData );
						// try to hold it
						if ( m_pPlayerModelPanel->HoldItem( iItemIdx ) ) 
						{
							m_iCurrentHeldItem = iItemIdx;
						}
					}
				}
			}
			else
			{
				m_pPlayerModelPanel->AddCarriedItem( &m_item );

				// Now make sure we're holding it if it's a non-wearable
				int iLoadoutSlot = m_item.GetStaticData()->GetLoadoutSlot( m_iCurrentClass );
				m_pPlayerModelPanel->HoldItemInSlot( iLoadoutSlot );
			}
		}

		UpdatePlayerModelButtons();

		// Fix a problem where changing the class would change the preview mesh but wouldn't
		// update the paint. This is a hack and won't work if we have multi-class styles or
		// just about anything else.
		CyclePaint( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdatePlayerModelButtons()
{
	UpdateOptionsButton();
	UpdateNextWeaponButton();
	UpdateZoomButton();
	UpdateTeamButton();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateCustomizeMenu( void )
{
	if ( m_pCustomizeMenu )
	{
		delete m_pCustomizeMenu;
		m_pCustomizeMenu = NULL;
	}

	if ( !m_pPlayerModelPanel->IsVisible() )
		return;

	if ( !m_pOptionsButton || !m_pOptionsButton->IsVisible() )
		return;

	if ( !m_item.IsValid() )
		return;

	m_pCustomizeMenu = new Menu( this, "CustomizeMenu" );
	MenuBuilder contextMenuBuilder( m_pCustomizeMenu, this );
	const char *pszContextMenuBorder = "NotificationDefault";
	const char *pszContextMenuFont = "HudFontMediumSecondary";
	m_pCustomizeMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
	m_pCustomizeMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );

	// Add paint options sub menu
	{
		Menu *pPaintSubMenu = NULL;
		FOR_EACH_VEC( m_vecPaintCans, i )
		{
			item_definition_index_t paintItemDefIndex = m_vecPaintCans[ i ];
			GameItemDefinition_t * pPaintCanDef = dynamic_cast<GameItemDefinition_t*>( GEconItemSchema().GetItemDefinition( paintItemDefIndex ) );
			if ( !CEconSharedToolSupport::ToolCanApplyToDefinition( dynamic_cast<const GameItemDefinition_t *>( pPaintCanDef ), m_item.GetStaticData() ) )
				continue;

			if ( pPaintSubMenu == NULL )
			{
				pPaintSubMenu = new Menu( this, "PaintSubMenu" );
				pPaintSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
				pPaintSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
				contextMenuBuilder.AddCascadingMenuItem( "#Context_Paint", pPaintSubMenu, "customization" );
			}

			wchar_t wBuff[256] = { 0 };
			V_swprintf_safe( wBuff, L"     %ls", g_pVGuiLocalize->Find( pPaintCanDef->GetItemBaseName() ) );
			int nIndex = pPaintSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "SetPaint%d", paintItemDefIndex ) ), this );
			vgui::MenuItem *pMenuItem = pPaintSubMenu->GetMenuItem( nIndex );
			pMenuItem->SetText( wBuff );
			pMenuItem->InvalidateLayout( true, false );

			uint32 unPaintRGB0 = 0;
			uint32 unPaintRGB1 = 0;

			static CSchemaAttributeDefHandle pAttrDef_PaintRGB( "set item tint RGB" );
			static CSchemaAttributeDefHandle pAttrDef_PaintRGB2( "set item tint RGB 2" );

			float fRGB = 0.0f;
			if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pPaintCanDef, pAttrDef_PaintRGB, &fRGB ) && fRGB != 0.0f )
			{
				unPaintRGB0 = fRGB;

				// We may or may not have a secondary paint color as well. If we don't, we just use the primary
				// paint color to fill both slots.
				if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pPaintCanDef, pAttrDef_PaintRGB2, &fRGB ) )
				{
					unPaintRGB1 = fRGB;
				}
				else
				{
					unPaintRGB1 = unPaintRGB0;
				}
			}

			vgui::HFont hFont = scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() );
			int nWidth, nHeight;
			vgui::surface()->GetTextSize( hFont, L"blah", nWidth, nHeight );

			CItemMaterialCustomizationIconPanel *pCustomPanel = new CItemMaterialCustomizationIconPanel( pMenuItem, "paint" );
			pCustomPanel->SetZPos( -100 );
			pCustomPanel->SetTall( nHeight );
			pCustomPanel->SetWide( nHeight );
			pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( (unPaintRGB0 & 0xFF0000) >> 16, 0, 255 ), clamp( (unPaintRGB0 & 0xFF00) >> 8, 0, 255 ), clamp( (unPaintRGB0 & 0xFF), 0, 255 ), 255 ) );
			pCustomPanel->m_colPaintColors.AddToTail( Color( clamp( (unPaintRGB1 & 0xFF0000) >> 16, 0, 255 ), clamp( (unPaintRGB1 & 0xFF00) >> 8, 0, 255 ), clamp( (unPaintRGB1 & 0xFF), 0, 255 ), 255 ) );
		}
	}
	
	// Add style
	{
		Menu *pStyleSubMenu = NULL;
		if ( m_item.GetStaticData()->GetNumSelectableStyles() > 1 )
		{
			for ( style_index_t unStyle=0; unStyle<m_item.GetStaticData()->GetNumStyles(); ++unStyle )
			{
				const CEconStyleInfo *pStyle = m_item.GetStaticData()->GetStyleInfo( unStyle );
				if ( !pStyle )
					continue;

				if ( !pStyle->IsSelectable() )
					continue;

				if ( pStyleSubMenu == NULL )
				{
					pStyleSubMenu = new Menu( this, "StyleSubMenu" );
					pStyleSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
					pStyleSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
					contextMenuBuilder.AddCascadingMenuItem( "#Context_Style", pStyleSubMenu, "customization" );
				}
				
				int nIndex = pStyleSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "SetStyle%d", unStyle ) ), this );
				vgui::MenuItem *pMenuItem = pStyleSubMenu->GetMenuItem( nIndex );
				pMenuItem->SetText( pStyle->GetName() );
				pMenuItem->InvalidateLayout( true, false );
			}
		}
	}

	// Add unusual
	{
		const CUtlVector< int > *pUnusualList = GetUnusualList();
		if ( pUnusualList )
		{
			Menu *pUnusualSubMenu = NULL;
			for ( int i=0; i<pUnusualList->Count(); ++i )
			{
				if ( pUnusualSubMenu == NULL )
				{
					pUnusualSubMenu = new Menu( this, "UnusualSubMenu" );
					pUnusualSubMenu->SetBorder( scheme()->GetIScheme( GetScheme() )->GetBorder( pszContextMenuBorder ) );
					pUnusualSubMenu->SetFont( scheme()->GetIScheme( GetScheme() )->GetFont( pszContextMenuFont, IsProportional() ) );
					contextMenuBuilder.AddCascadingMenuItem( "#Context_Unusual", pUnusualSubMenu, "customization" );
				}

				int iParticleIndex = pUnusualList->Element( i );
				int nIndex = pUnusualSubMenu->AddMenuItem( "", new KeyValues( "Command", "command", CFmtStr( "SetUnusual%d", iParticleIndex ) ), this );
				vgui::MenuItem *pMenuItem = pUnusualSubMenu->GetMenuItem( nIndex );
				pMenuItem->SetText( GetItemSchema()->GetParticleSystemLocalizedName( iParticleIndex ) );
				pMenuItem->InvalidateLayout( true, false );
			}
		}
	}

	int nX, nY;
	g_pVGuiInput->GetCursorPosition( nX, nY );
	m_pCustomizeMenu->SetPos( nX - 1, nY - 1 );
	
	m_pCustomizeMenu->SetVisible(true);
	m_pCustomizeMenu->AddActionSignalTarget(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateOptionsButton( void )
{
	if ( !m_pOptionsButton )
		return;

	m_pOptionsButton->SetVisible( false );

	if ( !m_pPlayerModelPanel->IsVisible() )
		return;

	if ( !m_item.IsValid() )
		return;

	const CEconItemDefinition *pItemData = m_item.GetItemDefinition();
	if ( !pItemData )
		return;

	bool bVisible = false;

	// Is the selected item paintable
	if ( pItemData->GetCapabilities() & ITEM_CAP_PAINTABLE )
	{
		bVisible = true;
	}
	// has multiple styles?
	else if ( pItemData->GetNumSelectableStyles() > 1 )
	{
		bVisible = true;
	}
	// can have unusual?
	else if ( GetUnusualList() != NULL )
	{
		bVisible = true;
	}

	m_pOptionsButton->SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateZoomButton( void )
{
	if ( !m_pZoomButton )
		return;

	m_pZoomButton->SetVisible( m_pPlayerModelPanel->IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateTeamButton( void )
{
	if ( !m_pTeamButton )
		return;

	m_pTeamButton->SetVisible( m_pPlayerModelPanel->IsVisible() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::UpdateNextWeaponButton( void )
{
	if ( !m_pNextWeaponButton )
		return;

	if ( !m_pPlayerModelPanel->IsVisible() )
	{
		m_pNextWeaponButton->SetVisible( false );
		return;
	}

	bool bShowNextWeaponsButton = false;
	const CUtlVector<CEconItemView*> &items = m_pPlayerModelPanel->GetCarriedItems();
	int iNumItemsArray[CLASS_LOADOUT_POSITION_COUNT];
	memset( iNumItemsArray, 0, sizeof( iNumItemsArray ) );
	FOR_EACH_VEC( items, i )
	{
		CEconItemView *pItem = items[i];
		int iLoadoutSlot = pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClass );
		if ( iLoadoutSlot >= 0 && iLoadoutSlot < CLASS_LOADOUT_POSITION_COUNT )
		{
			++iNumItemsArray[iLoadoutSlot];
		}			
	}
	bShowNextWeaponsButton |= iNumItemsArray[LOADOUT_POSITION_PRIMARY] + iNumItemsArray[LOADOUT_POSITION_SECONDARY] + iNumItemsArray[LOADOUT_POSITION_MELEE] > 1;
																						 
	m_pNextWeaponButton->SetVisible( bShowNextWeaponsButton );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::OnHideClassIconMouseover( void )
{
	if ( m_pClassIconMouseoverLabel )
	{
		m_pClassIconMouseoverLabel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::OnShowClassIconMouseover( KeyValues *data )
{
	if ( m_pClassIconMouseoverLabel )
	{
		const CEconItemDefinition *pItemData = m_item.GetItemDefinition();
		bool bIsABundle = pItemData ? (pItemData->GetBundleInfo() != NULL) : false;

		// Set the text to the correct string
		int iClass = data->GetInt( "class", 0 );
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS )
		{
			wchar_t wzLocalized[256];
			const char *pszLocString = bIsABundle ? "#Store_ClassImageMouseoverBundle" : "#Store_ClassImageMouseover";
			g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( pszLocString ), 1, g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] ) );
			m_pClassIconMouseoverLabel->SetText( wzLocalized );
		}
		else
		{
			const char *pszLocString = bIsABundle ? "#Store_ClassImageMouseoverAllBundle" : "#Store_ClassImageMouseoverAll";
			m_pClassIconMouseoverLabel->SetText( pszLocString );
		}

		m_pClassIconMouseoverLabel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::CycleStyle( void )
{
	if ( !m_pPlayerModelPanel )
		return;

	// Find and cycle the style based on the first item we're previewing that has
	// styles. If we are previewing multiple items at the same time where more than
	// one of them has styles, we'll only cycle through the names/options of the
	// first one in the list.
	CEconItemView *pPreviewItemView = NULL;

	const CUtlVector<CEconItemView*> &vecItems = m_pPlayerModelPanel->GetCarriedItems();
	FOR_EACH_VEC( vecItems, i )
	{
		CEconItemView *pItem = vecItems[i];
		if ( pItem->GetStaticData()->GetNumStyles() && ( pItem->GetFlags() & kEconItemFlagClient_Preview ) )
		{
			pPreviewItemView = pItem;
			break;
		}
	}

	if ( !pPreviewItemView )
		return;

	// Cycle.
	style_index_t unStyleCount = pPreviewItemView->GetStaticData()->GetNumStyles();
	Assert( unStyleCount >= 1 );

	style_index_t unStyle = pPreviewItemView->GetItemStyle();
	// Default to style 0 if we're getting an invalid index
	unStyle = unStyle == INVALID_STYLE_INDEX ? 0 : unStyle;

	// Try to find the next selectable style
	const CEconStyleInfo *pStyle = NULL;
	style_index_t unStartingStyle = unStyle;
	do
	{
		unStyle = (unStyle + 1) % unStyleCount;
		pStyle = pPreviewItemView->GetStaticData()->GetStyleInfo( unStyle );
		Assert( pStyle );
	}
	while ( unStyle != unStartingStyle && ( !pStyle || !pStyle->IsSelectable() ) );

	pPreviewItemView->SetItemStyleOverride( unStyle );
	SetCycleLabelText( m_pStyleNameLabel, pStyle->GetName() );

	// Re-equip our held item.  This causes all of our equipped items to get reloaded, and thus
	// their styles updated
	m_pPlayerModelPanel->SwitchHeldItemTo( m_pPlayerModelPanel->GetHeldItem() );
	m_pPlayerModelPanel->UpdatePreviewVisuals();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetPaint( item_definition_index_t iItemDef )
{
	if ( !m_pPlayerModelPanel )
		return;

	if ( !IsAnythingPaintable( m_pPlayerModelPanel->GetCarriedItems() ) )
		return;

	static CSchemaAttributeDefHandle pAttribDef_Paint( "set item tint RGB" );
	static CSchemaAttributeDefHandle pAttribDef_Paint2( "set item tint RGB 2" );

	// Find the next paint color.
	const CEconItemSchema::SortedItemDefinitionMap_t &mapDefs = GetItemSchema()->GetSortedItemDefinitionMap();

	m_unPaintRGB0 = 0;
	m_unPaintRGB1 = 0;

	if ( iItemDef != INVALID_ITEM_DEF_INDEX )
	{
		int iteratorName = mapDefs.FirstInorder();
		while ( iteratorName != mapDefs.InvalidIndex() )
		{
			// Find the next sub
			int iIndex = mapDefs[iteratorName]->GetDefinitionIndex();
			if ( iIndex == iItemDef )
			{
				// Is this definition something that has paint attributes on it?
				const CEconItemDefinition *pData = mapDefs[iteratorName];

				float fRGB = 0.0f;
				if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pData, pAttribDef_Paint, &fRGB ) && fRGB != 0.0f )
				{
					m_unPaintRGB0 = fRGB;

					// We may or may not have a secondary paint color as well. If we don't, we just use the primary
					// paint color to fill both slots.
					if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pData, pAttribDef_Paint2, &fRGB ) )
					{
						m_unPaintRGB1 = fRGB;
					}
					else
					{
						m_unPaintRGB1 = m_unPaintRGB0;
					}

					m_unPaintDef = pData->GetDefinitionIndex();
					SetCycleLabelText( m_pPaintNameLabel, pData->GetItemBaseName() );
				}
				else
				{
					Warning( "CTFStorePreviewItemPanelBase::SetPaint iItemDef[%d] is not paint item", iItemDef );
				}

				break;
			}

			iteratorName = mapDefs.NextInorder( iteratorName );
		}
	}

	if ( m_unPaintRGB0 == 0 )
	{
		m_unPaintDef = 0;
		SetCycleLabelText( m_pPaintNameLabel, "#Store_NoPaint" );
	}

	UpdatePaintColorsForTeam( m_pPlayerModelPanel, m_unPaintRGB0, m_unPaintRGB1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetStyle( style_index_t unStyle )
{
	if ( !m_pPlayerModelPanel )
		return;

	// Find and cycle the style based on the first item we're previewing that has
	// styles. If we are previewing multiple items at the same time where more than
	// one of them has styles, we'll only cycle through the names/options of the
	// first one in the list.
	CEconItemView *pPreviewItemView = NULL;

	const CUtlVector<CEconItemView*> &vecItems = m_pPlayerModelPanel->GetCarriedItems();
	FOR_EACH_VEC( vecItems, i )
	{
		CEconItemView *pItem = vecItems[i];
		if ( pItem->GetStaticData()->GetNumSelectableStyles() > 1 )
		{
			pPreviewItemView = pItem;
			break;
		}
	}

	if ( !pPreviewItemView )
		return;

	pPreviewItemView->SetItemStyleOverride( unStyle );
	const CEconStyleInfo *pStyle = pPreviewItemView->GetStaticData()->GetStyleInfo( unStyle );
	Assert( pStyle && pStyle->IsSelectable() );
	if ( pStyle )
	{
		SetCycleLabelText( m_pStyleNameLabel, pStyle->GetName() );
	}

	// Re-equip our held item.  This causes all of our equipped items to get reloaded, and thus
	// their styles updated
	m_pPlayerModelPanel->SwitchHeldItemTo( m_pPlayerModelPanel->GetHeldItem() );
	m_pPlayerModelPanel->UpdatePreviewVisuals();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::SetUnusual( uint32 iUnusualIndex )
{
	if ( !m_pPlayerModelPanel )
		return;

	static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
	static CSchemaAttributeDefHandle pAttrDef_TauntAttachParticleIndex( "taunt attach particle index" );
	const CUtlVector<CEconItemView*> &vecItems = m_pPlayerModelPanel->GetCarriedItems();

	FOR_EACH_VEC( vecItems, i )
	{
		CEconItemView *pItem = vecItems[i];
		if ( pItem->GetStaticData()->GetTauntData() )
		{
			const float& value_as_float = (float&)iUnusualIndex;
			pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_TauntAttachParticleIndex, value_as_float );
		}
		else
		{
			pItem->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_AttachParticleEffect, iUnusualIndex );
		}
	}
	m_pPlayerModelPanel->InvalidateParticleEffects();
	m_pPlayerModelPanel->SwitchHeldItemTo( m_pPlayerModelPanel->GetHeldItem() );
}


const CUtlVector< int >	*CTFStorePreviewItemPanelBase::GetUnusualList() const
{
	if ( !AllowUnusualPreview() )
		return NULL;

	int iLoadoutSlot = m_item.GetStaticData()->GetLoadoutSlot( m_iCurrentClass );
	if ( IsValidPickupWeaponSlot( iLoadoutSlot ) )
	{
		return GetItemSchema()->GetWeaponUnusualParticleIndexes();
	}
	// is hat or whole head?
	else if ( m_item.GetItemDefinition()->GetEquipRegionMask() & GetItemSchema()->GetEquipRegionBitMaskByName( "hat" ) || m_item.GetItemDefinition()->GetEquipRegionMask() & GetItemSchema()->GetEquipRegionBitMaskByName( "whole_head" ) )
	{
		return GetItemSchema()->GetCosmeticUnusualParticleIndexes();
	}
	else if ( IsTauntSlot( iLoadoutSlot ) )
	{
		return GetItemSchema()->GetTauntUnusualParticleIndexes();
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStorePreviewItemPanelBase::CyclePaint( bool bActuallyCycle )
{
	if ( !m_pPlayerModelPanel )
		return;

	if ( !IsAnythingPaintable( m_pPlayerModelPanel->GetCarriedItems() ) )
		return;
	
	static CSchemaAttributeDefHandle pAttribDef_Paint( "set item tint RGB" );
	static CSchemaAttributeDefHandle pAttribDef_Paint2( "set item tint RGB 2" );

	// Find the next paint color.
	const CEconItemSchema::SortedItemDefinitionMap_t &mapDefs = GetItemSchema()->GetSortedItemDefinitionMap();
	
	m_unPaintRGB0 = 0;
	m_unPaintRGB1 = 0;
	
	if ( bActuallyCycle || m_unPaintDef > 0 )
	{
		int iteratorName = mapDefs.FirstInorder();
		while ( iteratorName != mapDefs.InvalidIndex() )
		{
			// Find the next sub
			int iIndex = mapDefs[iteratorName]->GetDefinitionIndex();
			if ( bActuallyCycle ? iIndex > m_unPaintDef : iIndex >= m_unPaintDef )
			{
				// Is this definition something that has paint attributes on it?
				const CEconItemDefinition *pData = mapDefs[iteratorName];

				float fRGB = 0.0f;
				if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pData, pAttribDef_Paint, &fRGB ) && fRGB != 0.0f )
				{
					if ( V_strstr( pData->GetItemBaseName(), "Halloween" ) )
					{
						iteratorName = mapDefs.NextInorder( iteratorName );
						continue;
					}

					m_unPaintRGB0 = fRGB;

					// We may or may not have a secondary paint color as well. If we don't, we just use the primary
					// paint color to fill both slots.
					if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pData, pAttribDef_Paint2, &fRGB ) )
					{
						m_unPaintRGB1 = fRGB;
					}
					else
					{
						m_unPaintRGB1 = m_unPaintRGB0;
					}

					m_unPaintDef = pData->GetDefinitionIndex();
					SetCycleLabelText( m_pPaintNameLabel, pData->GetItemBaseName() );
					break;
				}
			}

			iteratorName = mapDefs.NextInorder( iteratorName );
		}
	}

	if ( m_unPaintRGB0 == 0 )
	{
		m_unPaintDef = 0;
		SetCycleLabelText( m_pPaintNameLabel, "#Store_NoPaint" );
	}

	UpdatePaintColorsForTeam( m_pPlayerModelPanel, m_unPaintRGB0, m_unPaintRGB1 );
}
