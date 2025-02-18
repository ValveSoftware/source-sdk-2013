//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "vgui/ISurface.h"
#include "c_tf_player.h"
#include "gamestringpool.h"
#include "iclientmode.h"
#include "tf_item_inventory.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ComboBox.h"
#include "vgui/IInput.h"
#include "item_model_panel.h"
#include "hudelement.h"
#include "item_quickswitch.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"
#include "loadout_preset_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define MAX_QUICKSWITCH_SLOTS 11

extern ConVar tf_respawn_on_loadoutchanges;

extern const char *g_szEquipSlotHeader[CLASS_LOADOUT_POSITION_COUNT];
int g_SlotsToLoadoutSlotsPerClass[TF_LAST_NORMAL_CLASS][MAX_QUICKSWITCH_SLOTS] =
{
	//TF_CLASS_UNDEFINED = 0,
	{ 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_SCOUT,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_SNIPER,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_SOLDIER,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_DEMOMAN,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_MEDIC,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_HEAVYWEAPONS,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_PYRO,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_SPY,
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_PDA,
		LOADOUT_POSITION_PDA2,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION,
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_INVALID,
	},

	// TF_CLASS_ENGINEER,		
	{
		LOADOUT_POSITION_INVALID,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE,
		LOADOUT_POSITION_PDA,
		LOADOUT_POSITION_PDA2,
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_ACTION, 
		LOADOUT_POSITION_INVALID, 
		LOADOUT_POSITION_INVALID,
	},
};

DECLARE_HUDELEMENT( CItemQuickSwitchPanel );

void IN_QuickSwitchDown( const CCommand &args )
{
	// quickswitch disabled in training
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		return;
	}

	CItemQuickSwitchPanel *pQSPanel = GET_HUDELEMENT( CItemQuickSwitchPanel );
	if ( pQSPanel )
	{
		pQSPanel->OpenQS();
	}
}

void IN_QuickSwitchUp( const CCommand &args )
{
	// quickswitch disabled in training
	if ( TFGameRules() && TFGameRules()->IsInTraining() )
	{
		return;
	}

	CItemQuickSwitchPanel *pQSPanel = GET_HUDELEMENT( CItemQuickSwitchPanel );
	if ( pQSPanel )
	{
		pQSPanel->CloseQS();
	}
}

static ConCommand openquickswitch( "+quickswitch", IN_QuickSwitchDown );
static ConCommand closequickswitch( "-quickswitch", IN_QuickSwitchUp );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemQuickSwitchPanel::CItemQuickSwitchPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "ItemQuickSwitchPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( false );

	m_pItemContainer = vgui::SETUP_PANEL( new vgui::EditablePanel( this, "itemcontainer" ) );
	m_pItemContainerScroller = vgui::SETUP_PANEL( new vgui::ScrollableEditablePanel( this, m_pItemContainer, "itemcontainerscroller" ) );
	m_pItemKV = NULL;
	m_pWeaponLabel = NULL;
	m_pEquipYourClassLabel = NULL;
	m_pLoadoutPresetPanel = NULL;
	m_iClass = TF_CLASS_UNDEFINED;
	m_iSlot = 0;

	SetVisible( false );

	ListenForGameEvent( "inventory_updated" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemQuickSwitchPanel::~CItemQuickSwitchPanel()
{
	if ( m_pItemKV )
	{
		m_pItemKV->deleteThis();
		m_pItemKV = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemQuickSwitchPanel::IsValid( void )
{ 
	return ( m_iClass >= TF_FIRST_NORMAL_CLASS && m_iClass < TF_LAST_NORMAL_CLASS ) && 
		   ( m_iSlot > LOADOUT_POSITION_INVALID && m_iSlot < CLASS_LOADOUT_POSITION_COUNT ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CItemQuickSwitchPanel::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( !IsVisible() )
		return 1; // key not handled

	if ( !down )
		return 1; // key not handled

	int iSlot = 0;

	// convert slot1, slot2 etc to 1,2,3,4
	if ( pszCurrentBinding && ( !Q_strncmp( pszCurrentBinding, "slot", 4 ) && Q_strlen( pszCurrentBinding ) > 4 ) )
	{
		const char *pszNum = pszCurrentBinding + 4;
		iSlot = atoi( pszNum );

		if ( ( iSlot < 1 ) || ( iSlot >= MAX_QUICKSWITCH_SLOTS ) )
		{
			// invalid bind
			iSlot = 0;
		}
	}

	if ( iSlot > 0 )
	{
		int iLoadoutSlot = g_SlotsToLoadoutSlotsPerClass[m_iClass][iSlot];
		if ( iLoadoutSlot != LOADOUT_POSITION_INVALID )
		{
			// is it the slot we're already viewing?
			if ( iLoadoutSlot != m_iSlot )
			{
				m_iSlot = iLoadoutSlot;

				if ( IsValid() )
				{
					UpdateModelPanels();
					InvalidateLayout( true );
				}
			}
		}
		else
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( "Player.DenyWeaponSelection" );
			}
		}

		return 0;
	}

	return 1; // key not handled
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemQuickSwitchPanel::CalculateClassAndSlot( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	// Get the current class
	m_iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	if ( m_iClass < TF_FIRST_NORMAL_CLASS || m_iClass >= TF_LAST_NORMAL_CLASS )
		return false;

	if ( m_pLoadoutPresetPanel )
	{
		m_pLoadoutPresetPanel->SetClass( m_iClass );
	}

	if ( pPlayer->IsAlive() )
	{
		// Get the current weapon slot
		CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
		if ( !pWpn )
			return false;

		m_iSlot = pWpn->GetAttributeContainer()->GetItem()->GetStaticData()->GetLoadoutSlot( m_iClass );
		if ( m_iSlot == LOADOUT_POSITION_INVALID )
			return false;
	}
	else
	{
		m_iClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
		m_iSlot = g_SlotsToLoadoutSlotsPerClass[m_iClass][1]; // use the first slot if we're dead
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OpenQS( void )
{
	if ( !CalculateClassAndSlot() )
		return;

	m_bLoadoutHasChanged = false;

	UpdateModelPanels();
	SetVisible( true );
	RequestFocus();
	MakePopup();
	SetKeyBoardInputEnabled( false );

	// Force layout now so that we can position the cursor properly
	InvalidateLayout( true );

	// It takes a few frames before this panel appears the first time, which means
	// we need to delay until it appears before we can pop the mouse to the right spot.
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OnTick( void )
{
	BaseClass::OnTick();

	// Move the mouse cursor onto the first entry in the panel that's not our active weapon
	if ( vgui::surface()->IsCursorVisible() )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );

		int x,y,w,h;
		if ( m_pItemPanels.Count() > 1 )
		{
			vgui::ipanel()->GetAbsPos( m_pItemPanels[1]->GetVPanel(), x, y );
			m_pItemPanels[1]->GetSize( w, h );
		}
		else
		{
			vgui::ipanel()->GetAbsPos( GetVPanel(), x, y );
			GetSize( w, h );
		}
		::input->SetFullscreenMousePos( x + (w * 0.5), y + (h * 0.5) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::CloseQS( void )
{
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	SetVisible( false );

	if ( m_bLoadoutHasChanged )
	{
		if ( tf_respawn_on_loadoutchanges.GetBool() )
		{
			// Tell the GC to tell server that we should respawn if we're in a respawn room
		}

		// Send the preset panel a msg so it can save the change
		CEconItemView *pCurItemData = TFInventoryManager()->GetItemInLoadoutForClass( m_iClass, m_iSlot );
		if ( pCurItemData )
		{
			KeyValues *pLoadoutChangedMsg = new KeyValues( "LoadoutChanged" );
			pLoadoutChangedMsg->SetInt( "slot", m_iSlot );
			pLoadoutChangedMsg->SetUint64( "itemid", pCurItemData->GetItemID() );
			PostMessage( m_pLoadoutPresetPanel, pLoadoutChangedMsg );
		}

		m_bLoadoutHasChanged = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ItemQuickSwitch.res" );

	m_pWeaponLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemSlotLabel") );
	m_pEquipYourClassLabel = dynamic_cast<vgui::Label*>( FindChildByName("EquipLabel") );
	m_pNoItemsToEquipLabel = dynamic_cast<vgui::Label*>( FindChildByName("NoItemsLabel") );
	m_pEquippedLabel = dynamic_cast<CExLabel*>( m_pItemContainer->FindChildByName("CurrentlyEquippedBackground") );
	m_pLoadoutPresetPanel = dynamic_cast<CLoadoutPresetPanel*>( FindChildByName( "loadout_preset_panel" ) );

	if ( m_pEquippedLabel )
	{
		m_pEquippedLabel->SetMouseInputEnabled( false );
	}

	if ( m_pLoadoutPresetPanel )
	{
		m_pLoadoutPresetPanel->EnableVerticalDisplay( true );
	}

	m_pItemContainerScroller->GetScrollbar()->SetAutohideButtons( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "itemskv" );
	if ( pItemKV )
	{
		if ( m_pItemKV )
		{
			m_pItemKV->deleteThis();
		}
		m_pItemKV = new KeyValues( "itemkv" );
		pItemKV->CopySubkeys( m_pItemKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	if ( !IsValid() )
		return;

	// Need to lay these out before we start making item panels inside them
	m_pItemContainer->InvalidateLayout( true );
	m_pItemContainerScroller->InvalidateLayout( true );

	// Position the item panels
	for ( int i = 0; i < m_pItemPanels.Count(); i++ )
	{
		if ( m_pItemKV )
		{
			m_pItemPanels[i]->ApplySettings( m_pItemKV );
			m_pItemPanels[i]->InvalidateLayout();
		} 

		int iYDelta = m_pItemPanels[0]->GetTall() + m_iItemPanelYDelta;

		// Once we've setup our first item, we know how large to make the container
		if ( i == 0 )
		{
			m_pItemContainer->SetSize( m_pItemContainer->GetWide(), iYDelta * m_pItemPanels.Count() );
		}

		// Always indent the top one to make it look better.
		m_pItemPanels[i]->SetPos( m_iItemPanelXPos, m_iItemPanelYDelta + (iYDelta * i) );
	}

	// Now that the container has been sized, tell the scroller to re-evaluate
	m_pItemContainerScroller->InvalidateLayout();
	m_pItemContainerScroller->GetScrollbar()->InvalidateLayout();

	// Force the class label to layout & resize, so we can align our title
	if ( m_pEquipYourClassLabel && m_pWeaponLabel )
	{
		m_pWeaponLabel->InvalidateLayout( true );
		m_pWeaponLabel->SizeToContents();
		int iXPos, iYPos;
		m_pWeaponLabel->GetPos( iXPos, iYPos );
		iXPos = ( GetWide() - m_pWeaponLabel->GetWide() ) * 0.5;
		m_pWeaponLabel->SetPos( iXPos, m_pWeaponLabel->GetTall() );
		m_pEquipYourClassLabel->SetPos( iXPos, iYPos - m_pEquipYourClassLabel->GetTall() );
	}

	// If it's visible, put the no items to equip at the bottom
	if ( m_pNoItemsToEquipLabel->IsVisible() )
	{
		int iYDelta = 0;
		if ( m_pItemPanels.Count() )
		{
			iYDelta = m_pItemPanels[0]->GetTall() + m_iItemPanelYDelta;
		}
		m_pNoItemsToEquipLabel->SetPos( m_iItemPanelXPos, m_iItemPanelYDelta + (iYDelta * (m_pItemPanels.Count()+1)) );
	}

	UpdateEquippedItem();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::UpdateEquippedItem( void )
{
	if ( !m_pEquippedLabel )
		return;

	bool bEquipped = false;

	CEconItemView *pCurItemData = TFInventoryManager()->GetItemInLoadoutForClass( m_iClass, m_iSlot );
	if ( pCurItemData )
	{
		if ( pCurItemData->IsValid() )
		{
			for ( int i = 0; i < m_pItemPanels.Count(); i++ )
			{
				CEconItemView *pItem = m_pItemPanels[i]->GetItem();
				if ( pItem && ( *pItem == *pCurItemData ) )
				{
					int x,y;
					m_pItemPanels[i]->GetPos( x, y );
					m_pEquippedLabel->SetPos( x + XRES(3), y + YRES(2) );

					bEquipped = true;
				}
			}
		}
		else if ( !TFInventoryManager()->SlotContainsBaseItems( GEconItemSchema().GetEquipTypeFromClassIndex( m_iClass ), m_iSlot ) )
		{
			for ( int i = 0; i < m_pItemPanels.Count(); i++ )
			{
				CEconItemView *pItem = m_pItemPanels[i]->GetItem();
				if ( !pItem )
				{
					int x,y;
					m_pItemPanels[i]->GetPos( x, y );
					m_pEquippedLabel->SetPos( x + XRES(3), y + YRES(2) );

					bEquipped = true;
				}
			}
		}
	}

	if ( m_pEquippedLabel->IsVisible() != bEquipped )
	{
		m_pEquippedLabel->SetVisible( bEquipped );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::UpdateModelPanels( void )
{
	if ( !IsValid() )
		return;

	TFPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
	SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( pData->m_szLocalizableName ) );

	if ( m_pWeaponLabel )
	{
		m_pWeaponLabel->SetText( g_szEquipSlotHeader[m_iSlot] );
	}

	// What items can go in this slot?
	extern equip_region_mask_t GenerateEquipRegionConflictMask( int iClass, int iUpToSlot, int iIgnoreSlot );
	const equip_region_mask_t unUsedEquipRegionMask = GenerateEquipRegionConflictMask( m_iClass, m_iSlot, LOADOUT_POSITION_INVALID );

	CEquippableItemsForSlotGenerator equippableItems( m_iClass, m_iSlot, unUsedEquipRegionMask, CEquippableItemsForSlotGenerator::kSlotGenerator_None );

	int iButton = 0;
	FOR_EACH_VEC( equippableItems.GetDisplayItems(), i )
	{
		// For quick-switch, only show items that are equippable and would show up as such in our regular
		// loadout.
		if ( equippableItems.GetDisplayItems()[i].m_eDisplayType == CEquippableItemsForSlotGenerator::kSlotDisplay_Normal )
		{
			SetButtonToItem( iButton++, equippableItems.GetDisplayItems()[i].m_pEconItemView );
		}
	}

	if ( !TFInventoryManager()->SlotContainsBaseItems( GEconItemSchema().GetEquipTypeFromClassIndex( m_iClass ), m_iSlot ) )
	{
		SetButtonToItem( iButton++, NULL );
	}

	if ( m_pNoItemsToEquipLabel )
	{
		m_pNoItemsToEquipLabel->SetVisible( iButton == 0 );
	}

	// Delete excess items
	for ( int i = m_pItemPanels.Count() - 1; i >= iButton; i-- )
	{
		m_pItemPanels[i]->MarkForDeletion();
		m_pItemPanels.Remove( i	);
	}

	InvalidateLayout();

	// Move the scrollbar to the top
	m_pItemContainerScroller->GetScrollbar()->SetValue( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::SetButtonToItem( int iButton, CEconItemView *pItem )
{
	CItemModelPanel *pItemPanel;
	if ( iButton < m_pItemPanels.Count() )
	{
		pItemPanel = m_pItemPanels[iButton];
	}
	else
	{
		const char *pszCommand = VarArgs( "itempanel%d", iButton );
		pItemPanel = new CItemModelPanel( m_pItemContainer, pszCommand );
		if ( m_pItemKV )
		{
			pItemPanel->ApplySettings( m_pItemKV );
		} 
		pItemPanel->MakeReadyForUse();
		pItemPanel->SetActAsButton( true, true );
		pItemPanel->SendPanelEnterExits( true );

		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		pItemPanel->SetBorder( pScheme->GetBorder( "EconItemBorder" ) );

		m_pItemPanels.AddToTail( pItemPanel );
	}

	pItemPanel->SetNoItemText( "#SelectNoItemSlot" );
	pItemPanel->SetItem( pItem );
	pItemPanel->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OnItemPanelEntered( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel )
	{
		pItemPanel->SetPaintBorderEnabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OnItemPanelExited( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( pItemPanel )
	{
		pItemPanel->SetPaintBorderEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OnIPMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );
	if ( !pItemPanel )
		return;

	itemid_t iIndex = INVALID_ITEM_ID;

	CEconItemView *pItemData = pItemPanel->GetItem();
	if ( pItemData && pItemData->IsValid() )
	{
		iIndex = pItemData->GetItemID();
	}

	TFInventoryManager()->EquipItemInLoadout( m_iClass, m_iSlot, iIndex );

	m_bLoadoutHasChanged = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::FireGameEvent( IGameEvent *event )
{
	if ( !IsVisible() )
		return;

	const char * type = event->GetName();

	if ( Q_strcmp( type, "inventory_updated" ) == 0 )
	{
		UpdateEquippedItem();
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemQuickSwitchPanel::OnItemPresetLoaded()
{
	m_bLoadoutHasChanged = true;
}
