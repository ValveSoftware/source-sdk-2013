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
#include "testitem_dialog.h"
#include "tf_controls.h"
#include "c_playerresource.h"
#include "gcsdk/gcmsg.h"
#include "tf_gcmessages.h"
#include "econ_item_inventory.h"
#include "econ_gcmessages.h"
#include "ienginevgui.h"
#include "filesystem.h"
#include "vgui_controls/FileOpenDialog.h"
#include "econ_item_system.h"
#include "testitem_root.h"
#include "econ_item_tools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern const char *g_TeamVisualSections[TEAM_VISUAL_SECTIONS];

static const char *g_pszTestItemHideBodygroup[] =
{
	"hat",			// 	TI_HIDEBG_HAT,
	"headphones",	// 	TI_HIDEBG_HEADPHONES,
	"medal",		// 	TI_HIDEBG_MEDALS,
	"grenades",		// 	TI_HIDEBG_GRENADES,
	"bullets",		//	TI_HIDEBG_BULLETS
	"arrows",		//	TI_HIDEBG_ARROWS
	"rightarm",		//	TI_HIDEBG_RIGHTARM
	"shoes_socks",	//	TI_HIDEBG_SHOES_SOCKS
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_pszTestItemHideBodygroup ) == TI_HIDEBG_COUNT );

static const char *g_pszClassSubdirectories[] =
{
	"all_class",	// TF_CLASS_UNDEFINED = 0,
	"scout",		// TF_CLASS_SCOUT,			// TF_FIRST_NORMAL_CLASS
	"sniper",		// TF_CLASS_SNIPER,
	"soldier",		// TF_CLASS_SOLDIER,
	"demo",			// TF_CLASS_DEMOMAN,
	"medic",		// TF_CLASS_MEDIC,
	"heavy",		// TF_CLASS_HEAVYWEAPONS,
	"pyro",			// TF_CLASS_PYRO,
	"spy",			// TF_CLASS_SPY,
	"engineer",		// TF_CLASS_ENGINEER,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemDialog::CTestItemDialog( vgui::Panel *parent, testitem_itemtypes_t iItemType, int iClassUsage, KeyValues *pExistingKVs ) : vgui::EditablePanel( parent, "TestItemDialog" )
{
	// Need to use the clientscheme (we're not parented to a clientscheme'd panel)
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );

	ListenForGameEvent( "gameui_hidden" );

	m_hImportModelDialog = NULL;
	m_pModelLabel = NULL;
	m_pSelectModelLabel = NULL;
	m_pNoItemsToReplaceLabel = NULL;
	m_pSelectModelButton = NULL;
	m_pOkButton = NULL;
	
	m_pItemReplacedPanel = new vgui::EditablePanel( this, "ItemReplacedPanel" );
	m_pItemReplacedComboBox = new vgui::ComboBox( m_pItemReplacedPanel, "ItemReplacedComboBox", 20, false );
	m_pItemReplacedComboBox->AddActionSignalTarget( this );

	m_pExistingItemToTestPanel = new vgui::EditablePanel( this, "ExistingItemToTestPanel" );
	m_pExistingItemComboBox = new vgui::ComboBox( m_pExistingItemToTestPanel, "ExistingItemComboBox", 20, false );
	m_pExistingItemComboBox->AddActionSignalTarget( this );

	m_pBodygroupPanel = new vgui::EditablePanel( this, "BodygroupPanel" );
	for ( int i = 0; i < TI_HIDEBG_COUNT; i++ )
	{
		m_pBodygroupCheckButtons[i] = new vgui::CheckButton( m_pBodygroupPanel, VarArgs("HideBodygroupCheckBox%d",i), "" );
		m_pBodygroupCheckButtons[i]->AddActionSignalTarget( this );
	}

	m_pCustomizationsPanel = new vgui::EditablePanel( this, "CustomizationsPanel" );
	m_pPaintColorComboBox = new vgui::ComboBox( m_pCustomizationsPanel, "PaintColorComboBox", 20, false );
	m_pPaintColorComboBox->AddActionSignalTarget( this );

	m_pUnusualEffectComboBox = new vgui::ComboBox( m_pCustomizationsPanel, "UnusualEffectComboBox", 20, false );
	m_pUnusualEffectComboBox->AddActionSignalTarget( this );

	m_iItemType = iItemType;
	m_iClassUsage = iClassUsage;

	m_szRelativePath[0] = '\0';
	SetDialogVariable("testmodel", g_pVGuiLocalize->Find( "#IT_NoModel" ) );
	SetEntryStep( TI_STEP_MODELNAME );

	// Load our scheme right away so we have all our pieces ready
	MakeReadyForUse();

	SetupPaintColorComboBox();
	SetupUnusualEffectComboBox();

	// Pull the data out of the existing KVs
	if ( pExistingKVs )
	{
		InitializeFromExistingKVs( pExistingKVs );
	}
	else
	{
		for ( int i = 0; i < TI_HIDEBG_COUNT; i++ )
		{
			// Start with the "hat" bodygroup checked (for non-weapons)
			bool bIsHat = ( m_iItemType != TI_TYPE_WEAPON ) && ( i == 0 );
			m_pBodygroupCheckButtons[i]->SetSelected( bIsHat );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::InitializeFromExistingKVs( KeyValues *pExistingKVs )
{
	// If we're testing an existing item, it supercedes everything else
	item_definition_index_t iExistingItemDef = pExistingKVs->GetInt( "existing_itemdef", INVALID_ITEM_DEF_INDEX );
	if ( iExistingItemDef != INVALID_ITEM_DEF_INDEX )
	{
		SetupItemComboBox( m_pExistingItemComboBox );

		// Loop through the entries until we find the specified item def
		for ( int i = 0; i < m_pExistingItemComboBox->GetItemCount(); i++ )
		{
			int iItemID = m_pExistingItemComboBox->GetItemIDFromRow(i);
			KeyValues *pRowKV = m_pExistingItemComboBox->GetItemUserData( iItemID );
			if ( pRowKV && pRowKV->GetInt( "item", INVALID_ITEM_DEF_INDEX ) == iExistingItemDef )
			{
				m_pExistingItemComboBox->SilentActivateItemByRow(i);
				SetEntryStep( TI_STEP_FINISHED );
			}
		}
	}
	else
	{
		const char *pszModel = pExistingKVs->GetString( "model_player", NULL );
		if ( pszModel && pszModel[0] )
		{
			Q_strncpy( m_szRelativePath, pszModel, MAX_PATH );
			SetDialogVariable("testmodel", m_szRelativePath );
			SetEntryStep( TI_STEP_MODELNAME );

			SetEntryStep( TI_STEP_WPN_ITEMREPLACED );
			if ( m_iItemType == TI_TYPE_WEAPON )
			{
				item_definition_index_t iItemDefToReplace = pExistingKVs->GetInt( "item_replace", INVALID_ITEM_DEF_INDEX );
				if ( iItemDefToReplace != INVALID_ITEM_DEF_INDEX )
				{
					SetupItemComboBox( m_pItemReplacedComboBox );

					// Loop through the entries until we find the specified item def
					for ( int i = 0; i < m_pItemReplacedComboBox->GetItemCount(); i++ )
					{
						int iItemID = m_pItemReplacedComboBox->GetItemIDFromRow(i);
						KeyValues *pRowKV = m_pItemReplacedComboBox->GetItemUserData( iItemID );
						if ( pRowKV && pRowKV->GetInt( "item", INVALID_ITEM_DEF_INDEX ) == iItemDefToReplace )
						{
							m_pItemReplacedComboBox->SilentActivateItemByRow(i);
							SetEntryStep( TI_STEP_FINISHED );
						}
					}
				}
			}
			else
			{
				KeyValues *pkvVisuals = pExistingKVs->FindKey( g_TeamVisualSections[0] );
				if ( pkvVisuals )
				{
					KeyValues *pKVEntry = pkvVisuals->GetFirstSubKey();
					while ( pKVEntry )
					{
						if ( !Q_stricmp( pKVEntry->GetName(), "player_bodygroups" ) )
						{
							FOR_EACH_SUBKEY( pKVEntry, pKVSubEntry )
							{
								int iBG = StringFieldToInt( pKVSubEntry->GetName(), g_pszTestItemHideBodygroup, ARRAYSIZE(g_pszTestItemHideBodygroup) );
								if ( iBG >= 0 && iBG < TI_HIDEBG_COUNT )
								{
									m_pBodygroupCheckButtons[iBG]->SetSelected( pKVSubEntry->GetInt() == 0 );
								}
							}
						}

						pKVEntry = pKVEntry->GetNextKey();
					}
				}

				// Start with the right paint can selected
				int iPaintCanIndex = pExistingKVs->GetInt("paintcan_index", 0);
				for ( int i = 0; i < m_pPaintColorComboBox->GetItemCount(); i++ )
				{
					int iItemID = m_pPaintColorComboBox->GetItemIDFromRow(i);
					KeyValues *pRowKV = m_pPaintColorComboBox->GetItemUserData( iItemID );
					if ( pRowKV && pRowKV->GetInt("paintcan_index",0) == iPaintCanIndex )
					{
						m_pPaintColorComboBox->SilentActivateItemByRow(i);
					}
				}

				// Start with the right unusual effect selected
				int iUnusualIndex = pExistingKVs->GetInt("unusual_index", 0);
				for ( int i = 0; i < m_pUnusualEffectComboBox->GetItemCount(); i++ )
				{
					int iItemID = m_pUnusualEffectComboBox->GetItemIDFromRow(i);
					KeyValues *pRowKV = m_pUnusualEffectComboBox->GetItemUserData( iItemID );
					if ( pRowKV && pRowKV->GetInt("unusual_index",0) == iUnusualIndex )
					{
						m_pUnusualEffectComboBox->SilentActivateItemByRow(i);
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTestItemDialog::~CTestItemDialog( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/TestItemDialog.res" );

	m_pModelLabel = dynamic_cast<CExLabel*>( FindChildByName( "ModelLabel" ) );
	m_pSelectModelLabel = dynamic_cast<CExLabel*>( FindChildByName( "SelectModelLabel" ) );
	m_pSelectModelButton = dynamic_cast<CExButton*>( FindChildByName( "SelectModelButton" ) );
	m_pOkButton = dynamic_cast<CExButton*>( FindChildByName( "OkButton" ) );

	m_pNoItemsToReplaceLabel = dynamic_cast<CExLabel*>( m_pItemReplacedPanel->FindChildByName( "NoItemsToReplaceLabel" ) );

	SetEntryStep( m_iEntryStep );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::PerformLayout( void ) 
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::FireGameEvent( IGameEvent *event )
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
void CTestItemDialog::Close( void )
{
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::CloseAndUpdateItem( void )
{
	// We're going to assemble a KV block that describes this test item
	KeyValues *kv = new KeyValues( "SetTestItemKVs" );
	kv->SetInt( "item_type", m_iItemType );
	kv->SetString( "model_player", m_szRelativePath );
	kv->SetBool( "test_existing_item", false );
	kv->SetInt( "attach_to_hands", (m_iItemType == TI_TYPE_WEAPON) );
	
	KeyValues *pKVModels = new KeyValues( "model_player_per_class" );
	kv->AddSubKey( pKVModels );
	const char *pFilename = V_UnqualifiedFileName( m_szRelativePath );
	if ( pFilename)
	{
		for ( int i = TF_FIRST_NORMAL_CLASS; i < ARRAYSIZE( g_pszClassSubdirectories ); i++ )
		{
			if ( m_iClassUsage == 1 || ( m_iClassUsage & (1 << i) ) )
			{
				CFmtStr1024 path( "models/player/items/%s/%s", g_pszClassSubdirectories[i], pFilename );
				if ( g_pFullFileSystem->FileExists( path.Access() ) )
				{
					pKVModels->SetString( ItemSystem()->GetItemSchema()->GetClassUsabilityStrings()[i], path.Access() );
				}
			}
		}
	}			

	KeyValues *pkvVisuals = new KeyValues( g_TeamVisualSections[0] ),
			  *pkvPlayerBodyGroups = new KeyValues( "player_bodygroups" );

	kv->AddSubKey( pkvVisuals );
	pkvVisuals->AddSubKey( pkvPlayerBodyGroups );

	for ( int i = 0; i < TI_HIDEBG_COUNT; i++ )
	{
		KeyValues *pKVBG = new KeyValues( g_pszTestItemHideBodygroup[i] );
		pKVBG->SetInt( NULL, m_pBodygroupCheckButtons[i]->IsSelected() ? 0 : 1 );
		pkvPlayerBodyGroups->AddSubKey( pKVBG );
	}

	// Extract the paint can index
	KeyValues *pPaintComboKV = m_pPaintColorComboBox->GetActiveItemUserData();
	int iPaintCanIndex = pPaintComboKV ? pPaintComboKV->GetInt( "paintcan_index", 0 ) : 0;
	kv->SetInt( "paintcan_index", iPaintCanIndex );

	// Extract the unusual effect index
	KeyValues *pUnusualComboKV = m_pUnusualEffectComboBox->GetActiveItemUserData();
	int iUnusualIndex = pUnusualComboKV ? pUnusualComboKV->GetInt( "unusual_index", 0 ) : 0;
	kv->SetInt( "unusual_index", iUnusualIndex );

	item_definition_index_t iItemDef = INVALID_ITEM_DEF_INDEX;

	// See if we're copying an existing item
	KeyValues *pExistingUserData = m_pExistingItemComboBox->GetActiveItemUserData();
	item_definition_index_t iExistingItemDef = pExistingUserData ? pExistingUserData->GetInt( "item", INVALID_ITEM_DEF_INDEX ) : INVALID_ITEM_DEF_INDEX;
	if ( iExistingItemDef != INVALID_ITEM_DEF_INDEX )
	{
		iItemDef = iExistingItemDef;
		kv->SetInt( "existing_itemdef", iItemDef );
		kv->SetBool( "test_existing_item", true );

		// copy model path from existing items
		GameItemDefinition_t *pItemDef = ItemSystem()->GetStaticDataForItemByDefIndex( iItemDef );
		if ( pItemDef )
		{
			for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
			{
				if ( m_iClassUsage == 1 || ( m_iClassUsage & (1 << iClass) ) )
				{
					const char *pszClassString = ItemSystem()->GetItemSchema()->GetClassUsabilityStrings()[iClass];
					const char *pszModel = pItemDef->GetPlayerDisplayModel( iClass );
					pKVModels->SetString( pszClassString, pszModel );
				}
			}
		}
	}
	else
	{
		KeyValues *pUserData = m_pItemReplacedComboBox->GetActiveItemUserData();
		iItemDef = pUserData ? pUserData->GetInt( "item", INVALID_ITEM_DEF_INDEX ) : INVALID_ITEM_DEF_INDEX;

		// Find the item def we're going to build off
		switch ( m_iItemType )
		{
		case TI_TYPE_WEAPON:
			// Need an item def to replace
			if ( iItemDef == INVALID_ITEM_DEF_INDEX )
				return;
			break;
		case TI_TYPE_HEADGEAR:
			iItemDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName("Football Helmet")->GetDefinitionIndex();
			break;
		case TI_TYPE_MISC1:
			iItemDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName("Employee Badge A")->GetDefinitionIndex();
			break;
		case TI_TYPE_MISC2:
			iItemDef = ItemSystem()->GetItemSchema()->GetItemDefinitionByName("High Five Taunt")->GetDefinitionIndex();
			break;
		}
	}

	// Tell the server what item we're replacing, and what def index we used
	kv->SetInt( "item_replace", iItemDef );

	// Send it to the testing root panel
	PostMessage( GetParent(), kv );
	
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "cancel" ) )
	{
		Close();
		return;
	}
	else if ( !Q_stricmp( command, "ok" ) )
	{
		CloseAndUpdateItem();
		return;
	}
	else if ( !Q_stricmp( command, "reloadscheme" ) )
	{
		InvalidateLayout( false, true );
		return;
	}
	else if ( !Q_stricmp( command, "select_model" ) )
	{
		OpenSelectModelDialog();
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::OpenSelectModelDialog( void )
{
	if (m_hImportModelDialog == NULL)
	{
		m_hImportModelDialog = new vgui::FileOpenDialog( NULL, "#ToolCustomizeTextureTitle", true );
		m_hImportModelDialog->AddFilter( "*.mdl", "#IT_MDL_Files", true );
		m_hImportModelDialog->AddActionSignalTarget( this );
	}

	char szModelsDir[MAX_PATH];
	switch( m_iItemType )
	{
	default:
		break;

	case TI_TYPE_WEAPON:
		m_hImportModelDialog->SetStartDirectory( g_pFullFileSystem->RelativePathToFullPath( "models/weapons/c_models", "MOD", szModelsDir, sizeof(szModelsDir) ) );
		break;

	case TI_TYPE_HEADGEAR:
	case TI_TYPE_MISC1:
	case TI_TYPE_MISC2:
		{
			const char *pszSubDir = NULL;

			// All classes?
			if ( m_iClassUsage == 1 )
			{
				pszSubDir = g_pszClassSubdirectories[0];
			}
			else
			{
				// If we only have one class, jump into that directory
				for ( int i = TF_FIRST_NORMAL_CLASS; i < LOADOUT_COUNT; i++ )
				{
					if ( m_iClassUsage & (1 << i) )
					{
						if ( !pszSubDir )
						{
							pszSubDir = g_pszClassSubdirectories[i];
						}
						else
						{
							// Found multiple classes. Move back up to the base dir.
							pszSubDir = NULL;
							break;
						}
					}
				}
			}

			if ( pszSubDir )
			{
				m_hImportModelDialog->SetStartDirectory( g_pFullFileSystem->RelativePathToFullPath( VarArgs("models/player/items/%s",pszSubDir), "MOD", szModelsDir, sizeof(szModelsDir) ) );
			}
			else
			{
				m_hImportModelDialog->SetStartDirectory( g_pFullFileSystem->RelativePathToFullPath( "models/player/items", "MOD", szModelsDir, sizeof(szModelsDir) ) );
			}
		}
		break;
	}

	m_hImportModelDialog->DoModal( false );
	m_hImportModelDialog->Activate();

	// Base file dialog won't refresh if it's opening to the same directory it was in. Force it to.
	PostMessage( m_hImportModelDialog->GetVPanel(), new KeyValues( "PopulateFileList" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct ComboBoxTestItem_t
{
	const wchar_t *pwszItemName;
	item_definition_index_t itemDef;
};
static int SortComboBoxTestItem( const ComboBoxTestItem_t *a, const ComboBoxTestItem_t *b )
{
	return V_wcscmp( a->pwszItemName, b->pwszItemName );
}

void CTestItemDialog::SetupItemComboBox( vgui::ComboBox *pComboBox )
{
	pComboBox->RemoveAll();

	CUtlVector<item_definition_index_t> vecDefs;
	int iReplacements = ((CTestItemRoot*)GetParent())->FindReplaceableItemsForSelectedClass( &vecDefs, m_iItemType == TI_TYPE_WEAPON );
	if ( iReplacements )
	{
		KeyValues *pKeyValues = new KeyValues( "data" );
		pKeyValues->SetInt( "item", INVALID_ITEM_DEF_INDEX );
		pComboBox->AddItem( "#IT_ItemReplaced_Select", pKeyValues );

		CUtlVector< ComboBoxTestItem_t > testItems;
		FOR_EACH_VEC( vecDefs, i )
		{
			CEconItemDefinition *pDef = ItemSystem()->GetStaticDataForItemByDefIndex( vecDefs[i] );
			if ( pDef )
			{
				const wchar_t *pwszLocalizedItemName = g_pVGuiLocalize->Find( pDef->GetItemBaseName() );
				if ( pwszLocalizedItemName )
				{
					int newIndex = testItems.AddToTail();
					testItems[newIndex].itemDef = vecDefs[i];
					testItems[newIndex].pwszItemName = pwszLocalizedItemName;
				}
			}
		}

		if ( testItems.Count() )
		{
			testItems.Sort( &SortComboBoxTestItem );
			FOR_EACH_VEC( testItems, i )
			{
				pKeyValues = new KeyValues( "data" );
				pKeyValues->SetInt( "item", testItems[i].itemDef );
				pComboBox->AddItem( testItems[i].pwszItemName, pKeyValues );
			}
		}
	}

	// No valid entries?
	if ( pComboBox == m_pItemReplacedComboBox )
	{
		if ( m_pNoItemsToReplaceLabel )
		{
			m_pNoItemsToReplaceLabel->SetVisible( !iReplacements );
		}
		m_pItemReplacedPanel->SetVisible( iReplacements );
	}

	pComboBox->SetItemEnabled( 0, false );
	pComboBox->SilentActivateItemByRow( 0 );
	pComboBox->GetMenu()->SetBgColor( Color(0,0,0,255) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::SetupPaintColorComboBox( void )
{
	m_pPaintColorComboBox->RemoveAll();

	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "paintcan_index", 0 );
	m_pPaintColorComboBox->AddItem( "#IT_PaintNone", pKeyValues );

	// Now loop through all our paints and add them to the list
	const CEconItemSchema::SortedItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetSortedItemDefinitionMap();
	FOR_EACH_MAP( mapItemDefs, i )
	{
		const CEconItemDefinition *pDef = mapItemDefs[i];

		const CEconTool_PaintCan *pEconToolPaintCan = pDef->GetTypedEconTool<CEconTool_PaintCan>();
		if ( !pEconToolPaintCan )
			continue;

		pKeyValues->SetInt( "paintcan_index", pDef->GetDefinitionIndex() );
		m_pPaintColorComboBox->AddItem( g_pVGuiLocalize->Find( pDef->GetItemBaseName() ), pKeyValues );

		// Make sure it has valid colors (to skip the store version of the paint can)
		KeyValues *pAttribs = pDef->GetDefinitionKey( "attributes" );
		if ( !pAttribs )
			continue;

		KeyValues *pRGBAttrib = pAttribs->FindKey( "set_item_tint_rgb" );
		if ( !pRGBAttrib )
			continue;

		int iModifiedRGB = pRGBAttrib->GetInt( "value", -1 );
		if ( iModifiedRGB != -1 )
		{
			m_pPaintColorComboBox->AddItem( g_pVGuiLocalize->Find( pDef->GetItemBaseName() ), pKeyValues );
		}
	}

	m_pPaintColorComboBox->SilentActivateItemByRow( 0 );
	m_pPaintColorComboBox->GetMenu()->SetBgColor( Color(0,0,0,255) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::SetupUnusualEffectComboBox( void )
{
	m_pUnusualEffectComboBox->RemoveAll();

	KeyValues *pKeyValues = new KeyValues( "data" );
	pKeyValues->SetInt( "unusual_index", 0 );
	m_pUnusualEffectComboBox->AddItem( "#IT_UnusualNone", pKeyValues );

	// Now loop through all unusual effects and add them to the list.
	const CEconItemSchema::ParticleDefinitionMap_t& mapParticleDefs = ItemSystem()->GetItemSchema()->GetAttributeControlledParticleSystems();
	FOR_EACH_MAP( mapParticleDefs, i )
	{
		pKeyValues->SetInt( "unusual_index", mapParticleDefs[i].nSystemID );

		char particleNameEntry[128];
		Q_snprintf( particleNameEntry, ARRAYSIZE( particleNameEntry ), "#Attrib_Particle%i", mapParticleDefs[i].nSystemID );
		m_pUnusualEffectComboBox->AddItem( g_pVGuiLocalize->Find( particleNameEntry ), pKeyValues );
	}

	m_pUnusualEffectComboBox->SilentActivateItemByRow( 0 );
	m_pUnusualEffectComboBox->GetMenu()->SetBgColor( Color(0,0,0,255) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::SetEntryStep( testitem_entrysteps_t iStep )
{
	// Skip over the item replacement if we're not a weapon
	if ( iStep == TI_STEP_WPN_ITEMREPLACED && m_iItemType != TI_TYPE_WEAPON )
	{
		iStep = (testitem_entrysteps_t)(iStep+1);
	}

	if ( iStep == TI_STEP_NONWPN_BODYGROUPS || iStep == TI_STEP_OTHER_OPTIONS )
	{
		// Move to "finished" straight away
		iStep = TI_STEP_FINISHED;
	}

	m_iEntryStep = iStep;

	if ( m_pSelectModelButton )
	{
		m_pSelectModelButton->SetVisible( iStep >= TI_STEP_MODELNAME );
		m_pSelectModelLabel->SetVisible( iStep >= TI_STEP_MODELNAME );
		m_pModelLabel->SetVisible( iStep >= TI_STEP_MODELNAME );
	}

	bool bTestingExistingItem = (iStep > TI_STEP_MODELNAME && m_szRelativePath[0] == '\0');

	m_pBodygroupPanel->SetVisible( iStep >= TI_STEP_NONWPN_BODYGROUPS && m_iItemType != TI_TYPE_WEAPON && !bTestingExistingItem );
	m_pExistingItemToTestPanel->SetVisible( iStep == TI_STEP_MODELNAME || bTestingExistingItem );
	m_pItemReplacedPanel->SetVisible( iStep >= TI_STEP_WPN_ITEMREPLACED && m_iItemType == TI_TYPE_WEAPON && !bTestingExistingItem );
	if ( m_pNoItemsToReplaceLabel )
	{
		m_pNoItemsToReplaceLabel->SetVisible( false );
	}

	m_pCustomizationsPanel->SetVisible( (iStep >= TI_STEP_CUSTOMIZATION && m_iItemType != TI_TYPE_WEAPON) );

	if ( m_pOkButton )
	{
		m_pOkButton->SetEnabled( m_iEntryStep >= TI_STEP_FINISHED );
	}

	switch ( m_iEntryStep )
	{
	case TI_STEP_MODELNAME:
		if ( !m_szRelativePath[0] )
		{
			SetDialogVariable("testmodel", g_pVGuiLocalize->Find( "#IT_NoModel" ) );
		}
		SetupItemComboBox( m_pExistingItemComboBox );
		break;
	case TI_STEP_WPN_ITEMREPLACED:
		SetupItemComboBox( m_pItemReplacedComboBox );
		break;
	case TI_STEP_NONWPN_BODYGROUPS:
		break;

	default:
	case TI_STEP_FINISHED:
		break;
	}
	
	SetDialogVariable( "testtitle", g_pVGuiLocalize->Find( VarArgs("#IT_Title_%d",m_iItemType) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr("panel") );
	if ( pPanel == m_pExistingItemComboBox )
	{
		if ( m_iItemType != TI_TYPE_WEAPON )
		{
			SetEntryStep( TI_STEP_OTHER_OPTIONS );
		}
		else
		{
			SetEntryStep( TI_STEP_FINISHED );
		}
	}
	else if ( pPanel == m_pItemReplacedComboBox )
	{
		SetEntryStep( TI_STEP_FINISHED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTestItemDialog::OnFileSelected(const char *fullpath)
{
	m_szRelativePath[0] = '\0';
	if ( g_pFullFileSystem->FullPathToRelativePathEx( fullpath, "GAME", m_szRelativePath, sizeof(m_szRelativePath) ) )
	{
		Q_FixSlashes( m_szRelativePath, '/' );
		SetDialogVariable("testmodel", m_szRelativePath );
		SetEntryStep( TI_STEP_WPN_ITEMREPLACED );
	}
	else
	{
		SetDialogVariable("testmodel", g_pVGuiLocalize->Find( "#IT_NoModel" ) );
	}

	// Nuke the file open dialog
	m_hImportModelDialog->MarkForDeletion();
	m_hImportModelDialog = NULL;
}

