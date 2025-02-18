//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <vgui/ILocalize.h>
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/CheckButton.h"
#include "testitem_root.h"
#include "tf_controls.h"
#include "c_playerresource.h"
#include "gcsdk/gcmsg.h"
#include "tf_gcmessages.h"
#include "econ_item_inventory.h"
#include "econ_gcmessages.h"
#include "ienginevgui.h"
#include "econ_item_system.h"
#include "vgui_controls/FileOpenDialog.h"
#include <filesystem.h>
#include "ai_activity.h"
#include "tf_gamerules.h"
#include "vgui_controls/Slider.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar tf_testitem_recent( "tf_testitem_recent", "", FCVAR_ARCHIVE );

KeyValues	*g_pRootItemTestingKV = NULL;

// Bot animations
const char *g_pszBotAnimStrings[TI_BOTANIM_COUNT] =
{
	"#IT_BotAnim_Idle",			// TI_BOTANIM_IDLE,
	"#IT_BotAnim_Crouch_Idle",	// TI_BOTANIM_CROUCH,
	"#IT_BotAnim_Run",			// TI_BOTANIM_RUN,
	"#IT_BotAnim_Crouch_Walk",	// TI_BOTANIM_CROUCH_WALK
	"#IT_BotAnim_Jump",			// TI_BOTANIM_JUMP
};

void	UpdateItemTestKVs( void )
{
	KeyValues *pTmpCopy = g_pRootItemTestingKV->MakeCopy();
	engine->ServerCmdKeyValues( pTmpCopy );

	// Setup any clientside variables to match what we're sending to the server
	TFGameRules()->ItemTesting_SetupFromKV( g_pRootItemTestingKV );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemRoot::CTestItemRoot( vgui::Panel *parent ) : vgui::EditablePanel( parent, "TestItemRoot" )
{
	// Need to use the clientscheme (we're not parented to a clientscheme'd panel)
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	m_hEditItemDialog = NULL;
	m_iClassUsage = 0;
	m_pClassUsagePanel = NULL;
	m_pTestingPanel = NULL;
	m_hImportExportDialog = NULL;
	m_bExporting = false;
	memset( m_pItemTestButtons, 0, sizeof(m_pItemTestButtons) );
	memset( m_pItemRemoveButtons, 0, sizeof(m_pItemRemoveButtons) );
	memset( m_pClassCheckButtons, NULL, sizeof(m_pClassCheckButtons) );
	memset( m_pItemTestKVs, 0, sizeof(m_pItemTestKVs) );

	m_pBotAdditionPanel = new vgui::EditablePanel( this, "BotAdditionPanel" );
	m_pBotSelectionComboBox = new vgui::ComboBox( m_pBotAdditionPanel, "BotSelectionComboBox", 9, false );
	m_pBotSelectionComboBox->AddActionSignalTarget( this );
	m_pAutoAddBotsCheckBox = new vgui::CheckButton( m_pBotAdditionPanel, "AutoAddBotsCheckBox", "" );
	m_pAutoAddBotsCheckBox->AddActionSignalTarget( this );
	m_pAutoAddBotsCheckBox->SetSelected( true );
	m_pBotsOnBlueTeamCheckBox = new vgui::CheckButton( m_pBotAdditionPanel, "BotsOnBlueTeamCheckBox", "" );
	m_pBotsOnBlueTeamCheckBox->AddActionSignalTarget( this );
	m_pBotsOnBlueTeamCheckBox->SetSelected( true );
	m_pAddBotButton = NULL;

	m_pBotControlPanel = new CTestItemBotControls( this );
	m_pBotControlPanel->SetEmbedded( true );

	SetupComboBoxes();

	if ( !g_pRootItemTestingKV )
	{
		g_pRootItemTestingKV = new KeyValues( "TestItems" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemRoot::~CTestItemRoot( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::SetupComboBoxes( void )
{
	// Setup our Bot Selection combo box
	KeyValues *pKeyValues;
	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++ )
	{
		if ( iClass == TF_CLASS_CIVILIAN )
			continue;
		pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "class", iClass );
		m_pBotSelectionComboBox->AddItem( g_aPlayerClassNames[iClass], pKeyValues );
	}
	m_pBotSelectionComboBox->SilentActivateItemByRow( 0 );

	m_pBotControlPanel->SetupComboBoxes();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/TestItemRoot.res" );

	m_pTestingPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "TestingPanel" ) );
	if ( m_pTestingPanel )
	{
		for ( int i = 0; i < TI_TYPE_COUNT; i++ )
		{
			m_pItemTestButtons[i] = dynamic_cast<CExButton*>( m_pTestingPanel->FindChildByName( VarArgs("TestItemButton%d",i) ) );
			m_pItemTestButtons[i]->AddActionSignalTarget( this );
			m_pItemRemoveButtons[i] = dynamic_cast<CExButton*>( m_pTestingPanel->FindChildByName( VarArgs("RemoveItemButton%d",i) ) );
			m_pItemRemoveButtons[i]->AddActionSignalTarget( this );
			m_pItemTestLabels[i] = dynamic_cast<CExLabel*>( m_pTestingPanel->FindChildByName( VarArgs("TestItemEntry%d",i) ) );
		}
	}

	m_pClassUsagePanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "ClassUsagePanel" ) );
	if ( m_pClassUsagePanel )
	{
		for ( int i = 0; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			m_pClassCheckButtons[i] = dynamic_cast<vgui::CheckButton*>( m_pClassUsagePanel->FindChildByName( VarArgs("ClassCheckBox%d",i)) );
			m_pClassCheckButtons[i]->AddActionSignalTarget( this );
		}
	}

	m_pAddBotButton = dynamic_cast<CExButton*>( m_pBotAdditionPanel->FindChildByName( "AddBotButton" ) );
	if ( m_pAddBotButton )
	{
		m_pAddBotButton->AddActionSignalTarget( this );
	}
	CExButton *pKickAllBotsButton = dynamic_cast<CExButton*>( m_pBotAdditionPanel->FindChildByName( "KickAllBotsButton" ) );
	if ( pKickAllBotsButton )
	{
		pKickAllBotsButton->AddActionSignalTarget( this );
	}

	AddChildActionSignalTarget( this, "SteamWorkshopButtonSubButton", this, true );

	UpdateTestItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::PerformLayout( void ) 
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::Close( void )
{
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::OnSetTestItemKVs( KeyValues *pKV )
{
	if ( !pKV )
		return;

	testitem_itemtypes_t iItemType = (testitem_itemtypes_t)pKV->GetInt("item_type");
	if ( iItemType <= TI_TYPE_UNKNOWN || iItemType > TI_TYPE_COUNT )
		return;

	// If we already have KVs for that slot, nuke them
	if ( m_pItemTestKVs[iItemType] )
	{
		g_pRootItemTestingKV->RemoveSubKey( m_pItemTestKVs[iItemType] );
		m_pItemTestKVs[iItemType]->deleteThis();
	}

	// Make our copy, and store it in the root KVs
	m_pItemTestKVs[iItemType] = pKV->MakeCopy();
	m_pItemTestKVs[iItemType]->SetName( VarArgs("Item%d",iItemType) );
	g_pRootItemTestingKV->AddSubKey( m_pItemTestKVs[iItemType] );

	UpdateTestItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::OnButtonChecked( KeyValues *pData )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( pData->GetPtr("panel") );

	if ( pPanel == m_pAutoAddBotsCheckBox )
	{
		if ( m_pAutoAddBotsCheckBox->IsSelected() )
		{
			m_pAddBotButton->SetEnabled( false );
			m_pBotSelectionComboBox->SetEnabled( false );
		}
		else
		{
			m_pAddBotButton->SetEnabled( true );
			m_pBotSelectionComboBox->SetEnabled( true );
		}

		return;
	}

	// If they hit all classes, disable everything else.
	if ( pPanel == m_pClassCheckButtons[0] )
	{
		bool bAllClass = m_pClassCheckButtons[0]->IsSelected();
		for ( int i = 1; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			m_pClassCheckButtons[i]->SetEnabled( !bAllClass );
			if ( bAllClass )
			{
				m_pClassCheckButtons[i]->SetSelected( false );
			}
		}
	}
	else
	{
		// If they've individually checked all boxes, switch to all-classes being checked
		bool bAllChecked = true;
		for ( int i = 1; i < TF_LAST_NORMAL_CLASS; i++ )
		{
			if ( !m_pClassCheckButtons[i]->IsSelected() )
			{
				bAllChecked = false;
				break;
			}
		}

		if ( bAllChecked )
		{
			m_pClassCheckButtons[0]->SetSelected( true );
		}
	}

	m_iClassUsage = 0;
	for ( int i = 0; i < TF_LAST_NORMAL_CLASS; i++ )
	{
		if ( m_pClassCheckButtons[i]->IsSelected() )
		{
			m_iClassUsage |= (1 << i);
		}
	}

	UpdateTestItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::CommitSettingsToKV( void )
{
	g_pRootItemTestingKV->SetInt( "class_usage", m_iClassUsage );
	g_pRootItemTestingKV->SetInt( "auto_add_bots", m_pAutoAddBotsCheckBox->IsSelected() );
	g_pRootItemTestingKV->SetInt( "bots_on_blue_team", m_pBotsOnBlueTeamCheckBox->IsSelected() );

	m_pBotControlPanel->CommitSettingsToKV();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::OnFileSelected(const char *fullpath)
{
	if ( m_bExporting )
	{
		ExportTestSetup( fullpath );
	}
	else
	{
		ImportTestSetup( fullpath );
	}

	// Nuke the file open dialog
	m_hImportExportDialog->MarkForDeletion();
	m_hImportExportDialog = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::ExportTestSetup( const char *pFilename )
{
	if ( !pFilename || !pFilename[0] )
		return;

	CommitSettingsToKV();

	g_pRootItemTestingKV->SaveToFile( g_pFullFileSystem, pFilename );
	tf_testitem_recent.SetValue( pFilename );	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::ImportTestSetup( KeyValues *pKV )
{
	// Setup the class usage checkboxes
	m_iClassUsage = pKV->GetInt( "class_usage", 0 );
	for ( int i = 0; i < TF_LAST_NORMAL_CLASS; i++ )
	{
		m_pClassCheckButtons[i]->SetSelected( (m_iClassUsage & (1<<i)) );
	}

	// Pull out the item KV blocks
	for ( int i = 0; i < TI_TYPE_COUNT; i++ )
	{
		m_pItemTestKVs[i] = pKV->FindKey( VarArgs("Item%d",i) );
	}

	bool bAutoAdd = pKV->GetInt( "auto_add_bots", 1 );
	m_pAutoAddBotsCheckBox->SetSelected(bAutoAdd);
	bool bBlueTeamBots = pKV->GetInt( "bots_on_blue_team", 0 );
	m_pBotsOnBlueTeamCheckBox->SetSelected(bBlueTeamBots);

	m_pBotControlPanel->ImportTestSetup( pKV );

	UpdateTestItems();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::ImportTestSetup( const char *pFilename )
{
	if ( !pFilename || !pFilename[0] )
		return;

	g_pRootItemTestingKV->deleteThis();
	g_pRootItemTestingKV = new KeyValues( "TestItems" );
	if ( g_pRootItemTestingKV->LoadFromFile( g_pFullFileSystem, pFilename ) )
	{
		ImportTestSetup( g_pRootItemTestingKV );
	}
	else
	{
		m_iClassUsage = 0;
		memset( m_pItemTestKVs, 0, sizeof(m_pItemTestKVs) );

		g_pRootItemTestingKV->deleteThis();
		g_pRootItemTestingKV = new KeyValues( "TestItems" );

		UpdateTestItems();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTestItemRoot::FindReplaceableItemsForSelectedClass( CUtlVector<item_definition_index_t> *pItemDefs, bool bWeapons )
{
	// Build our list of checked classes
	bool bClasses[TF_LAST_NORMAL_CLASS];
	for ( int i = 0; i < TF_LAST_NORMAL_CLASS; i++ )
	{
		bClasses[i] = m_iClassUsage & (1 << i);
	}

	int iReplaceableItems = 0;

	// Find all the weapons that can be used by the combination of classes we've checked
	const CEconItemSchema::SortedItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetSortedItemDefinitionMap();
	FOR_EACH_MAP( mapItemDefs, i )
	{
		const CTFItemDefinition *pDef = dynamic_cast<const CTFItemDefinition *>( mapItemDefs[i] );

		// Never show:
		//	- Hidden items
		//	- Items that don't have fixed qualities
		if ( !pDef || pDef->IsHidden() || pDef->GetQuality() == k_unItemQuality_Any )
			continue;

		// Only show in staging (internal dev branch):
		//	- Normal quality items
		//	- Items that haven't asked to be shown in the armory
		static const bool bIsStaging = ( engine->GetAppID() == 810 );
		if ( !bIsStaging )
		{
			if ( pDef->GetQuality() == AE_NORMAL || !pDef->ShouldShowInArmory() )
				continue;
		}

		// Make sure it's the right type of item
		int iDefSlot = pDef->GetDefaultLoadoutSlot();
		bool bValidSlot = false;
		if ( bWeapons )
		{
			bValidSlot = (iDefSlot == LOADOUT_POSITION_PRIMARY || iDefSlot == LOADOUT_POSITION_SECONDARY || iDefSlot == LOADOUT_POSITION_MELEE );
			if ( !bValidSlot )
			{
				bValidSlot = pDef->CanBePlacedInSlot(LOADOUT_POSITION_PRIMARY) || pDef->CanBePlacedInSlot(LOADOUT_POSITION_SECONDARY) || pDef->CanBePlacedInSlot(LOADOUT_POSITION_MELEE);
			}
		}
		else
		{
			bValidSlot = (iDefSlot == LOADOUT_POSITION_HEAD || iDefSlot == LOADOUT_POSITION_MISC );
			if ( !bValidSlot )
			{
				bValidSlot = pDef->CanBePlacedInSlot(LOADOUT_POSITION_HEAD) || pDef->CanBePlacedInSlot(LOADOUT_POSITION_MISC);
			}
		}
		if ( !bValidSlot )
			continue;

		// Make sure it's used by all the checked classes
		bool bUsable = false;
		if ( bClasses[0] )
		{
			bUsable = pDef->CanBeUsedByAllClasses();
		}
		else
		{
			bUsable = true;
			for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
			{
				if ( bClasses[iClass] && !pDef->CanBeUsedByClass(iClass) )
				{
					bUsable = false;
					break;
				}
			}
		}
		if ( !bUsable )
			continue;

		if ( pItemDefs )
		{
			pItemDefs->AddToTail( pDef->GetDefinitionIndex() );
		}
		iReplaceableItems++;
	}

	return iReplaceableItems;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::UpdateTestItems( void )
{
	for ( int i = 0; i < TI_TYPE_COUNT; i++ )
	{
		// Weapon is handled specially, because it's tied to the class usage
		if ( i == TI_TYPE_WEAPON )
		{
			int iValidWeapons = FindReplaceableItemsForSelectedClass( NULL, true );
			m_pItemTestButtons[0]->SetEnabled( iValidWeapons );
			if ( !iValidWeapons )
			{
				m_pItemTestLabels[0]->SetText( g_pVGuiLocalize->Find("#IT_ItemReplaced_Invalid") );
				continue;
			}
		}

		if ( m_pItemTestKVs[i] )
		{
			m_pItemTestButtons[i]->SetText( "#IT_Item_Edit" );

			item_definition_index_t iExistingDef = m_pItemTestKVs[i]->GetInt( "existing_itemdef", INVALID_ITEM_DEF_INDEX );
			if ( iExistingDef != INVALID_ITEM_DEF_INDEX )
			{
				CEconItemDefinition *pDef = ItemSystem()->GetItemSchema()->GetItemDefinition(iExistingDef);
				if ( pDef )
				{
					m_pItemTestLabels[i]->SetText( g_pVGuiLocalize->Find( pDef->GetItemBaseName() ) );
				}
				else
				{
					m_pItemTestLabels[i]->SetText( "#IT_TestingSlot_Empty" );
				}
			}
			else
			{
				const char *pszModel = m_pItemTestKVs[i]->GetString("model_player", "#IT_TestingSlot_Empty");

				char szModel[MAX_PATH+1]="";
				Q_FileBase( pszModel, szModel, ARRAYSIZE( szModel ) );
				m_pItemTestLabels[i]->SetText( szModel );
			}
			m_pItemRemoveButtons[i]->SetEnabled( true );
		}
		else
		{
			m_pItemTestButtons[i]->SetText( "#IT_Item_Add" );
			m_pItemTestLabels[i]->SetText( "#IT_TestingSlot_Empty" );
			m_pItemRemoveButtons[i]->SetEnabled( false );
		}
	}

	// Hide the testing panel if we don't have any classes selected
	if ( m_pTestingPanel )
	{
		m_pTestingPanel->SetVisible( m_iClassUsage != 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::CloseAndTestItem( void )
{
	// Go through and update the schema definitions before we send them off to the server
	for ( int i = 0; i < TI_TYPE_COUNT; i++ )
	{
		if ( !m_pItemTestKVs[i] )
			continue;

		item_definition_index_t iNewDef = TESTITEM_DEFINITIONS_BEGIN_AT + i;
		item_definition_index_t iItemDef = m_pItemTestKVs[i]->GetInt( "item_replace", INVALID_ITEM_DEF_INDEX );
		ItemSystem()->GetItemSchema()->ItemTesting_CreateTestDefinition( iItemDef, iNewDef, m_pItemTestKVs[i] );
		m_pItemTestKVs[i]->SetInt( "item_def", iNewDef );
	}

	// Not connected to a game?
	if ( !TFGameRules() )
		return;

	CommitSettingsToKV();
	g_pRootItemTestingKV->SetName("TestItems");
	UpdateItemTestKVs();
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemRoot::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		Close();
		return;
	}
	else if ( !Q_stricmp( command, "ok" ) )
	{
		CloseAndTestItem();
		return;
	}
	else if ( !Q_stricmp( command, "steamworkshop" ) )
	{
		Close();
		engine->ClientCmd_Unrestricted( "OpenSteamWorkshopDialog;" );
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( false, true );
		return;
	}
	else if ( !Q_strnicmp( command, "item_test", 9 ) )
	{
		int iItemType = atoi( command+9 );
		if ( iItemType >= 0 && iItemType < TI_TYPE_COUNT )
		{
			if (!m_hEditItemDialog.Get())
			{
				m_hEditItemDialog = vgui::SETUP_PANEL( new CTestItemDialog( this, (testitem_itemtypes_t)iItemType, m_iClassUsage, m_pItemTestKVs[iItemType] ) );
			}
			m_hEditItemDialog->InvalidateLayout( false, true );
			m_hEditItemDialog->SetVisible( true );
			m_hEditItemDialog->MoveToFront();
			m_hEditItemDialog->SetKeyBoardInputEnabled(true);
			m_hEditItemDialog->SetMouseInputEnabled(true);
			TFModalStack()->PushModal( m_hEditItemDialog );
		}
		return;
	}
	else if ( !Q_strnicmp( command, "item_remove", 11 ) )
	{
		int iItemType = atoi( command+11 );
		if ( iItemType >= 0 && iItemType < TI_TYPE_COUNT )
		{
			if ( m_pItemTestKVs[iItemType] )
			{
				g_pRootItemTestingKV->RemoveSubKey( m_pItemTestKVs[iItemType] );
				m_pItemTestKVs[iItemType]->deleteThis();
				m_pItemTestKVs[iItemType] = NULL;
			}
			UpdateTestItems();
		}
		return;
	}
	else if ( !Q_stricmp( command, "export" ) || !Q_stricmp( command, "import" ) )
	{
		m_bExporting = ( command[0] == 'e' );

		if (m_hImportExportDialog == NULL)
		{
			m_hImportExportDialog = new vgui::FileOpenDialog( NULL, "#ToolCustomizeTextureTitle", m_bExporting ? vgui::FOD_SAVE : vgui::FOD_OPEN, NULL );
			m_hImportExportDialog->AddFilter( "*.itf", "#IT_TestingFiles", true );
			m_hImportExportDialog->AddActionSignalTarget( this );

			char szModelsDir[MAX_PATH];
			m_hImportExportDialog->SetStartDirectory( g_pFullFileSystem->RelativePathToFullPath( "cfg", "MOD", szModelsDir, sizeof(szModelsDir) ) );
		}
		m_hImportExportDialog->DoModal( false );
		m_hImportExportDialog->Activate();
		return;
	}
	else if ( !Q_stricmp( command, "importrecent" ) )
	{
		ImportTestSetup( tf_testitem_recent.GetString() );
		return;
	}
	else if ( !Q_stricmp( command, "bot_add" ) )
	{
		KeyValues *pKV = m_pBotSelectionComboBox->GetActiveItemUserData();
		int iClass = pKV->GetInt( "class", TF_CLASS_UNDEFINED );
		if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS )
		{
			bool bBlueTeam = m_pBotsOnBlueTeamCheckBox->IsSelected();
			engine->ClientCmd_Unrestricted( VarArgs( "bot -team %s -class %s\n", bBlueTeam ? "blue" : "red", g_aPlayerClassNames_NonLocalized[iClass] ) );
		}
		return;
	}
	else if ( !Q_stricmp( command, "bot_removeall" ) )
	{
		// Kick everyone above the first player
		for ( int i = 2; i <= gpGlobals->maxClients; i++ )
		{
			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer )
			{
				engine->ClientCmd_Unrestricted( VarArgs( "kickid %d\n", pPlayer->GetUserID() ) );
			}
		}
		return;
	}
	

	BaseClass::OnCommand( command );
}


static vgui::DHANDLE<CTestItemRoot> g_hTestItemRoot;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OpenTestItemRoot( void )
{
	if (!g_hTestItemRoot.Get())
	{
		g_hTestItemRoot = vgui::SETUP_PANEL( new CTestItemRoot( NULL ) );
	}

	g_hTestItemRoot->SetVisible( true );
	g_hTestItemRoot->MakePopup();
	g_hTestItemRoot->MoveToFront();
	g_hTestItemRoot->SetKeyBoardInputEnabled(true);
	g_hTestItemRoot->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_hTestItemRoot );

	g_hTestItemRoot->MakeReadyForUse();
	if ( g_pRootItemTestingKV )
	{
		g_hTestItemRoot->ImportTestSetup( g_pRootItemTestingKV );
	}
}
ConCommand testitem( "itemtest", OpenTestItemRoot, "Open the item testing panel.", FCVAR_NONE );



//========================================================================================================================================
// BOT CONTROLS PANEL
//========================================================================================================================================
static vgui::DHANDLE<CTestItemBotControls> g_hTestItemBotControls;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemBotControls::CTestItemBotControls( vgui::Panel *parent ) : vgui::EditablePanel( parent, "TestItemBotControls" )
{
	// Need to use the clientscheme (we're not parented to a clientscheme'd panel)
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	m_pBotAnimationComboBox = new vgui::ComboBox( this, "BotAnimationComboBox", 9, false );
	m_pBotAnimationComboBox->AddActionSignalTarget( this );
	m_pBotForceFireCheckBox = new vgui::CheckButton( this, "BotForceFireCheckBox", "" );
	m_pBotForceFireCheckBox->AddActionSignalTarget( this );
	m_pBotTurntableCheckBox = new vgui::CheckButton( this, "BotTurntableCheckBox", "" );
	m_pBotTurntableCheckBox->AddActionSignalTarget( this );
	m_pBotViewScanCheckBox = new vgui::CheckButton( this, "BotViewScanCheckBox", "" );
	m_pBotViewScanCheckBox->AddActionSignalTarget( this );
	m_pBotAnimationSpeedSlider = new vgui::Slider( this, "BotAnimationSpeedSlider" );
	m_pBotAnimationSpeedSlider->SetRange( 0, 100 );
	m_pBotAnimationSpeedSlider->SetNumTicks( 10 );
	m_pBotAnimationSpeedSlider->AddActionSignalTarget( this );

	m_bEmbedded = false;

	SetupComboBoxes();

	if ( !g_pRootItemTestingKV )
	{
		g_pRootItemTestingKV = new KeyValues( "TestItems" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemBotControls::~CTestItemBotControls( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::SetupComboBoxes( void )
{
	KeyValues *pKeyValues;
	// Setup our bot animation combo box
	for ( int i = 0; i < TI_BOTANIM_COUNT; i++ )
	{
		pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "anim", i );
		m_pBotAnimationComboBox->AddItem( g_pszBotAnimStrings[i], pKeyValues );
	}
	m_pBotAnimationComboBox->SilentActivateItemByRow( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();

	if ( Q_strcmp(type, "gameui_hidden") == 0 )
	{
		Close();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::Close( void )
{
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::ImportTestSetup( KeyValues *pKV )
{
	bool bForceFire = pKV->GetInt( "bot_force_fire", 0 );
	m_pBotForceFireCheckBox->SetSelected(bForceFire);
	bool bViewScan = pKV->GetInt( "bot_view_scan", 0 );
	m_pBotViewScanCheckBox->SetSelected(bViewScan);
	bool bTurnTable = pKV->GetInt( "bot_turntable", 0 );
	m_pBotTurntableCheckBox->SetSelected(bTurnTable);

	int iAnim = g_pRootItemTestingKV->GetInt( "bot_anim", TI_BOTANIM_IDLE );
	m_pBotAnimationComboBox->SilentActivateItemByRow( iAnim );

	int iAnimSpeed = g_pRootItemTestingKV->GetInt( "bot_animspeed", 100 );
	m_pBotAnimationSpeedSlider->SetValue( iAnimSpeed, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/TestItemBotControls.res" );

	// Dumb, but the slider needs to have its scheme forcibly loaded to make it create the left/right text
	m_pBotAnimationSpeedSlider->InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	CExButton *pButton = dynamic_cast<CExButton*>( FindChildByName( "OkButton" ) );
	if ( pButton )
	{
		pButton->SetVisible( !m_bEmbedded );
	}
	pButton = dynamic_cast<CExButton*>( FindChildByName( "CloseButton" ) );
	if ( pButton )
	{
		pButton->SetVisible( !m_bEmbedded );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		Close();
		return;
	}
	else if ( !Q_stricmp( command, "ok" ) )
	{
		UpdateBots();
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( false, true );
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::UpdateBots( void )
{
	// Not connected to a game?
	if ( !TFGameRules() )
		return;

	CommitSettingsToKV();

	g_pRootItemTestingKV->SetName("TestItemsBotUpdate");
	UpdateItemTestKVs();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemBotControls::CommitSettingsToKV( void )
{
	g_pRootItemTestingKV->SetInt( "bot_force_fire", m_pBotForceFireCheckBox->IsSelected() );
	g_pRootItemTestingKV->SetInt( "bot_view_scan", m_pBotViewScanCheckBox->IsSelected() );
	g_pRootItemTestingKV->SetInt( "bot_turntable", m_pBotTurntableCheckBox->IsSelected() );

	KeyValues *pKV = m_pBotAnimationComboBox->GetActiveItemUserData();
	int iAnim = pKV->GetInt( "anim", TI_BOTANIM_IDLE );
	g_pRootItemTestingKV->SetInt( "bot_anim", iAnim );

	int iAnimSpeed = clamp( m_pBotAnimationSpeedSlider->GetValue(), 0, 100 );
	g_pRootItemTestingKV->SetInt( "bot_animspeed", iAnimSpeed );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void OpenTestItemBotControls( void )
{
	if (!g_hTestItemBotControls.Get())
	{
		g_hTestItemBotControls = vgui::SETUP_PANEL( new CTestItemBotControls( NULL ) );
	}

	g_hTestItemBotControls->SetVisible( true );
	g_hTestItemBotControls->MakePopup();
	g_hTestItemBotControls->MoveToFront();
	g_hTestItemBotControls->SetKeyBoardInputEnabled(true);
	g_hTestItemBotControls->SetMouseInputEnabled(true);
	TFModalStack()->PushModal( g_hTestItemBotControls );

	g_hTestItemBotControls->MakeReadyForUse();
	if ( g_pRootItemTestingKV )
	{
		g_hTestItemBotControls->ImportTestSetup( g_pRootItemTestingKV );
		g_hTestItemBotControls->SetEmbedded( false );
	}
}
ConCommand testitem_botcontrols( "itemtest_botcontrols", OpenTestItemBotControls, "Open the item testing bot control panel.", FCVAR_NONE );
