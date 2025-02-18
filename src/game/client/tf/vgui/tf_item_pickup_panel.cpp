//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include "tf_item_pickup_panel.h"
#include "iclientmode.h"
#include "baseviewport.h"
#include "econ_entity.h"
#include "c_baseplayer.h"
#include "gamestringpool.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "econ_item_system.h"
#include "ienginevgui.h"
#include "achievementmgr.h"
#include "fmtstr.h"
#include "tf_item_inventory.h"
#include "item_confirm_delete_dialog.h"
#include "backpack_panel.h"
#include "econ_ui.h"
#include "c_tf_player.h"
#include "character_info_panel.h"
#include "tf_matchmaking_dashboard_parent_manager.h"

ConVar tf_explanations_discardpanel( "tf_explanations_discardpanel", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemPickupPanel::CTFItemPickupPanel( Panel *parent ) : CItemPickupPanel( parent, false )
{
	m_pClassImage = NULL;
	m_pClassImageBG = NULL;
	SetZPos( 10000 );
	GetMMDashboardParentManager()->AddPanel( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemPickupPanel::~CTFItemPickupPanel()
{
	GetMMDashboardParentManager()->RemovePanel( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemPickupPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pClassImage = dynamic_cast<vgui::ImagePanel*>(FindChildByName( "classimage" ));
	m_pClassImageBG = FindChildByName( "classimageoutline" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemPickupPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "changeloadout" ) )
	{
		// We dont want the UI to close -- we're about to change our loadout
		SetReturnToGame( false );
		AcknowledgeItems();

		int iClass = TF_CLASS_UNDEFINED;
		if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
		{
			if ( m_aItems[m_iSelectedItem].pItem.IsValid() && !m_aItems[m_iSelectedItem].bDiscarded )
			{
				// Open the loadout panel with the first class that can use this item (or the base loadout screen if it's an all-class item)
				if ( m_aItems[m_iSelectedItem].pItem.GetStaticData()->CanBeUsedByAllClasses() )
				{
					iClass = TF_CLASS_UNDEFINED;
				}
				else
				{
					for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
					{
						if ( m_aItems[m_iSelectedItem].pItem.GetStaticData()->CanBeUsedByClass(i) )
						{
							iClass = -i;
							break;
						}
					}

					if ( iClass == TF_CLASS_UNDEFINED )
					{
						// Item's not usable by any class. Go to backpack.
						iClass = ECONUI_BACKPACK;
					}
				}
			}
		}

		ShowPanel( false );

		EconUI()->OpenEconUI( iClass, true );
	}
	else 
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemPickupPanel::UpdateModelPanels( void )
{
	BaseClass::UpdateModelPanels();

	if ( m_pClassImage )
	{
		m_pClassImage->SetVisible( false );
		if ( m_pClassImageBG )
		{
			m_pClassImageBG->SetVisible( false );
		}
		if ( m_aModelPanels[2]->HasItem() )
		{
			CEconItemView *pItem = m_aModelPanels[2]->GetItem();
			
			int iClass = -1;
			if ( pItem->GetStaticData()->CanBeUsedByAllClasses() )
			{
				iClass = TF_CLASS_UNDEFINED;
			}
			else
			{
				// Find a class that can use the item, and show that class image
				for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
				{
					if ( pItem->GetStaticData()->CanBeUsedByClass(i) )
					{
						iClass = i;
						break;
					}
				}
			}

			if ( iClass != -1 )
			{
				m_pClassImage->SetImage( g_pszItemClassImagesRed[iClass] );
				m_pClassImage->SetVisible( true );
				if ( m_pClassImageBG )
				{
					m_pClassImageBG->SetVisible( true );
				}
			}
		}
	}

	// Update the loadout button as appropriate
	if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
	{
		bool bDiscarded = false;
		if ( m_iSelectedItem >= 0 && m_iSelectedItem < m_aItems.Count() )
		{
			bDiscarded = m_aItems[m_iSelectedItem].bDiscarded;
		}

		// Open the loadout panel with the first class that can use this item
		if ( m_aItems[m_iSelectedItem].pItem.IsValid() && !bDiscarded )
		{
			if ( m_aItems[m_iSelectedItem].pItem.GetStaticData()->CanBeUsedByAllClasses() )
			{
				SetDialogVariable("loadouttext", g_pVGuiLocalize->Find( "#OpenGeneralLoadout" ) );
			}
			else
			{
				int iClass = TF_CLASS_UNDEFINED;
				for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; i++ )
				{
					if ( m_aItems[m_iSelectedItem].pItem.GetStaticData()->CanBeUsedByClass(i) )
					{
						iClass = i;
						break;
					}
				}

				if ( iClass != TF_CLASS_UNDEFINED )
				{
					wchar_t wzLocalized[128];
					g_pVGuiLocalize->ConstructString_safe( wzLocalized, g_pVGuiLocalize->Find( "#OpenSpecificLoadout" ), 1, g_pVGuiLocalize->Find( g_aPlayerClassNames[iClass] ) );
					SetDialogVariable("loadouttext", wzLocalized );
				}
				else
				{
					SetDialogVariable("loadouttext", g_pVGuiLocalize->Find( "#OpenBackpack" ) );
					m_pOpenLoadoutButton->SetVisible( true );
				}
			}
		}
	}
}

static vgui::DHANDLE<CTFItemPickupPanel> g_TFItemPickupPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemPickupPanel *OpenTFItemPickupPanel( void )
{
	if (!g_TFItemPickupPanel.Get())
	{
		g_TFItemPickupPanel = vgui::SETUP_PANEL( new CTFItemPickupPanel( NULL ) );
		g_TFItemPickupPanel->InvalidateLayout( false, true );
	}

	engine->ClientCmd_Unrestricted( "gameui_activate" );
	g_TFItemPickupPanel->ShowPanel( true );

	return g_TFItemPickupPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemPickupPanel *GetTFItemPickupPanel( void )
{
	return g_TFItemPickupPanel.Get();
}

//=======================================================================================================================================================
// ITEM DISCARD PANEL
//=======================================================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemDiscardPanel::CTFItemDiscardPanel( Panel *parent ) : CItemDiscardPanel( parent )
{
	m_flStartExplanationsAt = 0;
	m_pExplanationALabel = NULL;
	m_pExplanationBLabel = NULL;
	m_pExplanationCaratLabel = NULL;
	SetZPos( 10000 );
	GetMMDashboardParentManager()->AddPanel( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFItemDiscardPanel::~CTFItemDiscardPanel()
{
	GetMMDashboardParentManager()->RemovePanel( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDiscardPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pExplanationALabel = dynamic_cast<vgui::Label*>( FindChildByName("ExplanationLabel") );
	m_pExplanationBLabel = dynamic_cast<vgui::Label*>( FindChildByName("ExplanationLabel2") );
	m_pExplanationCaratLabel = dynamic_cast<vgui::Label*>( FindChildByName("CaratLabel2") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDiscardPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	m_pExplanationALabel->SetVisible( !m_bDiscardedNewItem && !m_bMadeRoom );
	m_pExplanationBLabel->SetVisible( !m_bDiscardedNewItem && !m_bMadeRoom );
	m_pExplanationCaratLabel->SetVisible( !m_bDiscardedNewItem && !m_bMadeRoom );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDiscardPanel::ShowPanel(bool bShow)
{
	BaseClass::ShowPanel( bShow );

	if ( bShow )
	{
		if ( !tf_explanations_discardpanel.GetBool() )
		{
			m_flStartExplanationsAt = engine->Time() + 0.5;
			vgui::ivgui()->AddTickSignal( GetVPanel() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFItemDiscardPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( m_flStartExplanationsAt && m_flStartExplanationsAt < engine->Time() && TFModalStack()->IsEmpty() )
	{
		m_flStartExplanationsAt = 0;

		tf_explanations_discardpanel.SetValue( 1 );

		CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( FindChildByName("StartExplanation") );
		if ( pPopup )
		{
			pPopup->Popup();
		}
	}

	if ( !m_flStartExplanationsAt )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFItemDiscardPanel::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "show_explanations" ) )
	{
		if ( !m_flStartExplanationsAt )
		{
			m_flStartExplanationsAt = engine->Time();
			vgui::ivgui()->AddTickSignal( GetVPanel() );
		}
		RequestFocus();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

#if defined(DEBUG)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Test_ItemPickupPanel( const CCommand &args )
{
	int iClass = TF_CLASS_PYRO;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	}

	CItemPickupPanel *pItemPanel = EconUI()->OpenItemPickupPanel();
	pItemPanel->InvalidateLayout( false, true );

	for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
	{
		CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i );
		if ( pItem && pItem->IsValid() )
		{
			pItemPanel->AddItem( pItem );
		}
	}

	pItemPanel->DebugRandomizePickupMethods();
}
ConCommand test_itempickuppanel( "test_itempickuppanel", Test_ItemPickupPanel, "Debugging tool to test the item pickup panel. Usage: test_itempickuppanel\n", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Test_ItemDiscardPanel( const CCommand &args )
{
	int iClass = TF_CLASS_PYRO;
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();
	}

	CItemDiscardPanel *pItemPanel = EconUI()->OpenItemDiscardPanel();
	pItemPanel->InvalidateLayout( false, true );

	CEconItemView *pItemView = NULL;

	bool bAllItems = (args.ArgC() <= 1);
	for ( int i = bAllItems ? 0 : clamp( atoi(args[1]), 0, 2 ); i <= 2 && !pItemView; i++ )
	{
		pItemView = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i );
	}

	if ( pItemView )
	{
		pItemPanel->SetItem( pItemView );
	}
}

ConCommand test_itemdiscardpanel( "test_itemdiscardpanel", Test_ItemDiscardPanel, "Debugging tool to test the item discard panel. Usage: test_itemdiscardpanel <weapon name>\n   <weapon id>: 0 = primary, 1 = secondary, 2 = melee.", FCVAR_CHEAT );
#endif // defined(DEBUG)

