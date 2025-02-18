//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "charinfo_loadout_subpanel.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui/ILocalize.h"
#include "c_tf_freeaccount.h"
#include "c_tf_player.h"
#include "confirm_dialog.h"
#include "gamestringpool.h"
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "tf_item_inventory.h"
#include "trading_start_dialog.h"
#include "gc_clientsystem.h"
#include "tf_item_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


DECLARE_BUILD_FACTORY( CImageButton );


ConVar tf_explanations_charinfopanel( "tf_explanations_charinfopanel", "0", FCVAR_ARCHIVE, "Whether the user has seen explanations for this panel." );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImageButton::CImageButton( vgui::Panel *parent, const char *panelName ) : BaseClass( parent, panelName, "" )
{
	m_pszActiveImageName = NULL;
	m_pszInactiveImageName = NULL;

	m_pActiveImage = NULL;
	m_pInactiveImage = NULL;

	m_ActiveDrawColor = Color(255,255,255,255);
	m_InactiveDrawColor = Color(255,255,255,255);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::ApplySettings( KeyValues *inResourceData )
{
	m_bScaleImage = inResourceData->GetInt( "scaleImage", 0 );

	// Active Image
	delete [] m_pszActiveImageName;
	m_pszActiveImageName = NULL;

	const char *activeImageName = inResourceData->GetString( "activeimage", "" );
	if ( *activeImageName )
	{
		SetActiveImage( activeImageName );
	}

	// Inactive Image
	delete [] m_pszInactiveImageName;
	m_pszInactiveImageName = NULL;

	const char *inactiveImageName = inResourceData->GetString( "inactiveimage", "" );
	if ( *inactiveImageName )
	{
		SetInactiveImage( inactiveImageName );
	}

	const char *pszDrawColor = inResourceData->GetString("activedrawcolor", "");
	if (*pszDrawColor)
	{
		int r = 0, g = 0, b = 0, a = 255;
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_ActiveDrawColor = Color(r, g, b, a);
		}
		else
		{
			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
			m_ActiveDrawColor = pScheme->GetColor(pszDrawColor, Color(0, 0, 0, 0));
		}
	}

	pszDrawColor = inResourceData->GetString("inactivedrawcolor", "");
	if (*pszDrawColor)
	{
		int r = 0, g = 0, b = 0, a = 255;
		if (sscanf(pszDrawColor, "%d %d %d %d", &r, &g, &b, &a) >= 3)
		{
			m_InactiveDrawColor = Color(r, g, b, a);
		}
		else
		{
			vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
			m_InactiveDrawColor = pScheme->GetColor(pszDrawColor, Color(0, 0, 0, 0));
		}
	}

	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pszActiveImageName && strlen( m_pszActiveImageName ) > 0 )
	{
		SetActiveImage(vgui::scheme()->GetImage( m_pszActiveImageName, m_bScaleImage ) );
	}

	if ( m_pszInactiveImageName && strlen( m_pszInactiveImageName ) > 0 )
	{
		SetInactiveImage(vgui::scheme()->GetImage( m_pszInactiveImageName, m_bScaleImage ) );
	}

	vgui::IBorder *pBorder = pScheme->GetBorder( "NoBorder" );
	SetDefaultBorder( pBorder);
	SetDepressedBorder( pBorder );
	SetKeyFocusBorder( pBorder );

	Color defaultFgColor = GetSchemeColor( "Button.TextColor", Color(255, 255, 255, 255), pScheme );
	Color armedFgColor = GetSchemeColor( "Button.ArmedTextColor", Color(255, 255, 255, 255), pScheme );
	Color depressedFgColor = GetSchemeColor( "Button.DepressedTextColor", Color(255, 255, 255, 255), pScheme );

	Color blank(0,0,0,0);
	SetDefaultColor( defaultFgColor, blank );
	SetArmedColor( armedFgColor, blank );
	SetDepressedColor( depressedFgColor, blank );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::SetActiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszActiveImageName = new char[ len ];
	Q_strncpy( m_pszActiveImageName, imagename, len );

	SetActiveImage(vgui::scheme()->GetImage( m_pszActiveImageName, m_bScaleImage ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::SetInactiveImage( const char *imagename )
{
	int len = Q_strlen( imagename ) + 1;
	m_pszInactiveImageName = new char[ len ];
	Q_strncpy( m_pszInactiveImageName, imagename, len );

	SetInactiveImage(vgui::scheme()->GetImage( m_pszInactiveImageName, m_bScaleImage ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::SetActiveImage( vgui::IImage *image )
{
	m_pActiveImage = image;

	if ( m_pActiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage )
		{
			// scaling, force the image size to be our size
			GetSize( wide, tall );
			m_pActiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pActiveImage->GetSize( wide, tall );
			SetSize( wide, tall );
		}
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::SetInactiveImage( vgui::IImage *image )
{
	m_pInactiveImage = image;

	if ( m_pInactiveImage )
	{
		int wide, tall;
		if ( m_bScaleImage)
		{
			// scaling, force the image size to be our size
			GetSize( wide, tall );
			m_pInactiveImage->SetSize( wide, tall );
		}
		else
		{
			// not scaling, so set our size to the image size
			m_pInactiveImage->GetSize( wide, tall );
			SetSize( wide, tall );
		}
	}

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::OnSizeChanged( int newWide, int newTall )
{
	if ( m_bScaleImage )
	{
		// scaling, force the image size to be our size
		if ( m_pActiveImage )
			m_pActiveImage->SetSize( newWide, newTall );

		if ( m_pInactiveImage )
			m_pInactiveImage->SetSize( newWide, newTall );
	}
	BaseClass::OnSizeChanged( newWide, newTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImageButton::Paint()
{
	if ( IsArmed() || _buttonFlags.IsFlagSet( FORCE_DEPRESSED ) )
	{
		// draw the active image
		if ( m_pActiveImage )
		{
			m_pActiveImage->SetColor( m_ActiveDrawColor );
			m_pActiveImage->SetPos( 0, 0 );
			m_pActiveImage->Paint();
		}
	}
	else 
	{
		// draw the inactive image
		if ( m_pInactiveImage )
		{
			m_pActiveImage->SetColor( m_InactiveDrawColor );
			m_pInactiveImage->SetPos( 0, 0 );
			m_pInactiveImage->Paint();
		}
	}

	BaseClass::Paint();
}

const char *g_pszSubButtonNames[CHSB_NUM_BUTTONS] =
{
	"ShowBackpackButton",	// CHSB_BACKPACK,
	"ShowCraftingButton",	// CHSB_CRAFTING,
	"ShowArmoryButton",		// CHSB_ARMORY,
	"ShowTradeButton",		// CHSB_TRADING,
	"ShowPaintkitsButton"	// CHSB_PAINTKITS
};
const char *g_pszSubButtonLabelNames[CHSB_NUM_BUTTONS] =
{
	"ShowBackpackLabel",	// CHSB_BACKPACK,
	"ShowCraftingLabel",	// CHSB_CRAFTING,
	"ShowArmoryLabel",		// CHSB_ARMORY,
	"ShowTradeLabel",		// CHSB_TRADING,
	"ShowPaintkitsLabel",	// CHSB_PAINTKITS
};

int g_nLoadoutClassOrder[] =
{
	TF_CLASS_SCOUT,
	TF_CLASS_SOLDIER,
	TF_CLASS_PYRO,
	TF_CLASS_DEMOMAN,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_ENGINEER,
	TF_CLASS_MEDIC,
	TF_CLASS_SNIPER,
	TF_CLASS_SPY
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCharInfoLoadoutSubPanel::CCharInfoLoadoutSubPanel(Panel *parent) : vgui::PropertyPage(parent, "CharInfoLoadoutSubPanel")
{
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED;
	m_iCurrentTeamIndex = TF_TEAM_RED;
	m_iShowingPanel = CHAP_LOADOUT;
	m_iPrevShowingPanel = CHAP_LOADOUT;

	memset( m_pClassButtons, 0, sizeof( m_pClassButtons ) );

	m_pClassButtons[ TF_CLASS_SCOUT ] = new CImageButton( this, "scout" );
	m_pClassButtons[ TF_CLASS_SOLDIER ] = new CImageButton( this, "soldier" );
	m_pClassButtons[ TF_CLASS_PYRO ] = new CImageButton( this, "pyro" );
	m_pClassButtons[ TF_CLASS_DEMOMAN ] = new CImageButton( this, "demoman" );
	m_pClassButtons[ TF_CLASS_HEAVYWEAPONS ] = new CImageButton( this, "heavyweapons" );
	m_pClassButtons[ TF_CLASS_ENGINEER ] = new CImageButton( this, "engineer" );
	m_pClassButtons[ TF_CLASS_MEDIC ] = new CImageButton( this, "medic" );
	m_pClassButtons[ TF_CLASS_SNIPER ] = new CImageButton( this, "sniper" );
	m_pClassButtons[ TF_CLASS_SPY ] = new CImageButton( this, "spy" );

	for( int i = 0; i < Q_ARRAYSIZE( m_pClassButtons ); i++ )
	{
		if( m_pClassButtons[ i ] )
			m_pClassButtons[ i ]->SetParentNeedsCursorMoveEvents( true );
	}

	for ( int i = 0; i < CHSB_NUM_BUTTONS; i++ )
	{
		m_pSubButtons[i] = new CImageButton( this, g_pszSubButtonNames[i] );
		m_pButtonLabels[i] = new CExLabel( this, g_pszSubButtonLabelNames[i], "" );
	}
	m_iOverSubButton = -1;

	m_pClassLoadoutPanel = new CClassLoadoutPanel( this );
	m_pBackpackPanel = new CBackpackPanel( this, "backpack_panel" );
	m_pCraftingPanel = new CCraftingPanel( this, "crafting_panel" );
	m_pArmoryPanel = new CArmoryPanel( this, "armory_panel" );
	m_pInspectPanel = new CTFItemInspectionPanel( this, "InspectionPanel" );
	m_pArmoryPanel->AllowGotoStore();
	m_pSelectLabel = NULL;
	m_pLoadoutChangesLabel = NULL;
	m_pNoSteamLabel = NULL;
	m_pNoGCLabel = NULL;
	m_pClassLabel = NULL;
	m_pItemsLabel = NULL;
	m_bSnapClassLayout = false;
	m_bClassLayoutDirty = false;
	m_bRequestingInventoryRefresh = false;
	m_flStartExplanationsAt = 0;

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	REGISTER_COLOR_AS_OVERRIDABLE( m_ItemColorNone, "itemcountcolor_noitems" );
	REGISTER_COLOR_AS_OVERRIDABLE( m_ItemColor, "itemcountcolor" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCharInfoLoadoutSubPanel::~CCharInfoLoadoutSubPanel()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/CharInfoLoadoutSubPanel.res" );

	m_pSelectLabel = dynamic_cast<vgui::Label*>( FindChildByName("SelectLabel") );
	m_pLoadoutChangesLabel = dynamic_cast<vgui::Label*>( FindChildByName("LoadoutChangesLabel") );
	m_pNoSteamLabel = dynamic_cast<vgui::Label*>( FindChildByName("NoSteamLabel") );
	m_pNoGCLabel = dynamic_cast<vgui::Label*>( FindChildByName("NoGCLabel") );
	m_pClassLabel = dynamic_cast<vgui::Label*>( FindChildByName("ClassLabel") );
	int ignored;
	if ( m_pClassLabel )
	{
		m_pClassLabel->GetPos( ignored, m_iClassLabelYPos );
	}
	m_pItemsLabel = dynamic_cast<CExLabel*>( FindChildByName("ItemsLabel") );
	if ( m_pItemsLabel )
	{
		m_pItemsLabel->GetPos( ignored, m_iItemLabelYPos );
	}

	// Start classes sized as if the mouse is in the middle of the screen
	m_iMouseXPos = -1;
	m_bSnapClassLayout = true;
	RecalculateTargetClassLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnPageShow( void )
{
	SetVisible( true );

	BaseClass::OnPageShow();

	if( m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
	{
		m_pClassButtons[ m_iCurrentClassIndex ]->GetPos( m_iMouseXPos, m_iMouseYPos );
		m_bClassLayoutDirty = true;
		InvalidateLayout();
	}

	// If this is the first time we've opened the loadout, start the loadout explanations
	if ( !tf_explanations_charinfopanel.GetBool() && ShouldShowExplanations() )
	{
		m_flStartExplanationsAt = engine->Time() + 0.5;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnSelectionStarted( void )
{
	PostActionSignal( new KeyValues("SelectionUpdate", "open", 1 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnSelectionEnded( void )
{
	PostActionSignal( new KeyValues("SelectionUpdate", "open", 0 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnCancelSelection( void )
{
	PostMessage( m_pClassLoadoutPanel, new KeyValues("CancelSelection") );
	PostMessage( m_pBackpackPanel, new KeyValues("CancelSelection") );
	PostMessage( m_pCraftingPanel, new KeyValues("CancelSelection") );
	PostMessage( m_pArmoryPanel, new KeyValues("CancelSelection") );
	RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnCharInfoClosing( void )
{
	switch ( m_iShowingPanel )
	{
	case CHAP_CRAFTING:
		PostMessage( m_pCraftingPanel, new KeyValues("Closing") );
		break;
	case CHAP_BACKPACK:
		break;
	case CHAP_ARMORY:
		PostMessage( m_pArmoryPanel, new KeyValues("Closing") );
		break;
	case CHAP_LOADOUT:
		PostMessage( m_pClassLoadoutPanel, new KeyValues("Closing") );
		break;
	default: // Class loadout.
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnOpenCrafting( void )
{
	OpenToCrafting();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnCraftingClosed( void )
{
	PostMessage( m_pCraftingPanel, new KeyValues("Closing") );
	m_iShowingPanel = CHAP_LOADOUT;
	m_iPrevShowingPanel = CHAP_CRAFTING;
	m_flStartExplanationsAt = 0; 
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED; 
	UpdateModelPanels();
	RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnArmoryClosed( void )
{
	// Return to whatever we were on before opening the armory
	PostMessage( m_pArmoryPanel, new KeyValues("Closing") );
	m_iShowingPanel = m_iPrevShowingPanel;
	m_iPrevShowingPanel = CHAP_ARMORY;
	m_flStartExplanationsAt = 0; 
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED; 
	UpdateModelPanels();
	RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "loadout ", 8 ) )
	{
		// Ignore selection while we don't have a steam connection
		if ( !TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() )
			return;

		m_flStartExplanationsAt = 0;

		const char *pszClass = command+8;
		if ( pszClass[0] != '\0' )
		{
			int nClassIndex = GetClassIndexFromString( pszClass, NUM_CLASSES_IN_LOADOUT_PANEL );
			if ( nClassIndex != TF_CLASS_UNDEFINED && m_iCurrentClassIndex != nClassIndex )
			{
				SetClassIndex( nClassIndex, true );
				return;
			}
		}
	}
	else if ( !Q_strnicmp( command, "backpack", 8 ) )
	{
		OpenToBackpack();
	}
	else if ( !Q_strnicmp( command, "crafting", 8 ) )
	{
		OpenToCrafting();
	}
	else if ( !Q_strnicmp( command, "armory", 6 ) )
	{
		OpenToArmory();
	}
	else if ( !Q_strnicmp( command, "trading", 7 ) )
	{
		OpenTradingStartDialog( this );
	}
	else if ( FStrEq( command, "paintkit_preview" ) )
	{
		// When opening from the charinfo panel, just show a random weapon
		CUtlVector< CTFItemDefinition* > vecWeaponDefs;

		// Dig up all the weapons
		const CEconItemSchema::ItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetItemDefinitionMap();
		FOR_EACH_MAP_FAST( mapItemDefs, i )
		{
			CTFItemDefinition *pData = assert_cast< CTFItemDefinition* >( mapItemDefs[i] );

			if ( pData->GetDefaultLoadoutSlot() == LOADOUT_POSITION_PRIMARY ||
				 pData->GetDefaultLoadoutSlot() == LOADOUT_POSITION_SECONDARY ||
				 pData->GetDefaultLoadoutSlot() == LOADOUT_POSITION_MELEE )
			{

				float flInspect = 0;
				static CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
				if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pData, pAttrib_WeaponAllowInspect, &flInspect ) )
				{
					vecWeaponDefs.AddToTail( pData );
				}
			}
		}

		Assert( vecWeaponDefs.Count() );
		if ( vecWeaponDefs.Count() )
		{
			// Randomly pick one that supports paintkits.  We don't want to do GetValidPaintkits() in the above
			// loop because it causes the CTFItemDefinition to populate its valid paintkits list, which isn't cheap
			int nSafeguard = 0;
			int nRandomIndex = 0;
			do 
			{
				nRandomIndex = RandomInt( 0, vecWeaponDefs.Count() - 1 );
				++nSafeguard;
			} while ( vecWeaponDefs[ nRandomIndex ]->GetValidPaintkits().IsEmpty() && nSafeguard < 100 );

			// Open up the inspection panel with the random weapon we've chosen
			CEconItemView itemTemp;
			itemTemp.Init( vecWeaponDefs[ nRandomIndex ]->GetDefinitionIndex(), AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );

			OpenToPaintkitPreview( &itemTemp, false, false );
		}
	}
	else if ( !Q_stricmp( command, "show_explanations" ) )
	{
		if ( !m_flStartExplanationsAt )
		{
			m_flStartExplanationsAt = engine->Time();
		}
		RequestFocus();
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
void CCharInfoLoadoutSubPanel::RequestInventoryRefresh()
{
	m_bRequestingInventoryRefresh = false;

	// Don't respond to the mouse if we don't have items
	if ( !TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() )
	{
		ShowWaitingDialog( new CGenericWaitingDialog(this), "#NoSteamNoItems_Refresh", true, true, 30.0f );
		if ( !m_bRequestingInventoryRefresh )
		{
			// make sure the local inventory is added as a listener
			TFInventoryManager()->UpdateLocalInventory();
			m_bRequestingInventoryRefresh = true;
			// ask GC for refresh
			GCSDK::CProtoBufMsg< CMsgRequestInventoryRefresh > msg( k_EMsgGCRequestInventoryRefresh );
			GCClientSystem()->BSendMessage( msg );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::SetClassIndex( int iClassIndex, bool bOpenClassLoadout )
{
	Assert(iClassIndex >= TF_CLASS_UNDEFINED && iClassIndex <= NUM_CLASSES_IN_LOADOUT_PANEL);
	m_iCurrentClassIndex = iClassIndex;
	m_iShowingPanel = CHAP_LOADOUT;
	UpdateModelPanels( bOpenClassLoadout );

	RequestInventoryRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::SetTeamIndex( int iTeam )
{
	Assert( IsValidTFTeam( iTeam ) );
	m_iCurrentTeamIndex = iTeam;
}


void CCharInfoLoadoutSubPanel::OpenToPaintkitPreview( CEconItemView* pItem, bool bFixedItem, bool bFixedPaintkit )
{
	if ( m_pInspectPanel->IsLayoutInvalid() )
		m_pInspectPanel->InvalidateLayout( true, true );

	if ( pItem )
	{
		m_pInspectPanel->SetOptions( bFixedItem, bFixedPaintkit, false );
		m_pInspectPanel->SetItemCopy( pItem );
	}
	OpenSubPanel( CHAP_PAINTKIT_PREVIEW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OpenSubPanel( charinfo_activepanels_t iPanel )
{
	m_flStartExplanationsAt = 0; 
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED; 
	m_iPrevShowingPanel = m_iShowingPanel;
	m_iShowingPanel = iPanel; 

	UpdateModelPanels();

	RequestInventoryRefresh();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::UpdateModelPanels( bool bOpenClassLoadout )
{
	int iLabelClassToSet = -1;
	int iClassIndexToSet = 0;

	if ( m_iShowingPanel == CHAP_CRAFTING )
	{
		m_pClassLoadoutPanel->SetVisible( false );
		m_pBackpackPanel->SetVisible( false );
		m_pArmoryPanel->SetVisible( false );
		m_pCraftingPanel->ShowPanel( m_iCurrentClassIndex, true, (m_iPrevShowingPanel == CHAP_ARMORY) );
		m_pInspectPanel->SetVisible( false );
	}
	else if ( m_iShowingPanel == CHAP_BACKPACK )
	{
		m_pClassLoadoutPanel->SetVisible( false );
		m_pCraftingPanel->SetVisible( false );
		m_pArmoryPanel->SetVisible( false );
		m_pBackpackPanel->ShowPanel( m_iCurrentClassIndex, true, (m_iPrevShowingPanel == CHAP_ARMORY) );
		m_pInspectPanel->SetVisible( false );
	}
	else if ( m_iShowingPanel == CHAP_ARMORY )
	{
		m_pClassLoadoutPanel->SetVisible( false );
		m_pCraftingPanel->SetVisible( false );
		m_pBackpackPanel->SetVisible( false );
		m_pArmoryPanel->ShowPanel( m_iArmoryItemDef );
		m_pInspectPanel->SetVisible( false );
	}
	else if ( m_iShowingPanel == CHAP_PAINTKIT_PREVIEW )
	{
		m_pInspectPanel->SetVisible( true );
	}
	else
	{
		iClassIndexToSet = bOpenClassLoadout ? m_iCurrentClassIndex : TF_CLASS_UNDEFINED;
		m_pArmoryPanel->SetVisible( false );
		m_pBackpackPanel->SetVisible( false );
		m_pCraftingPanel->SetVisible( false );
		m_pInspectPanel->SetVisible( false );
		m_pClassLoadoutPanel->SetTeam( m_iCurrentTeamIndex );
		m_pClassLoadoutPanel->SetClass( iClassIndexToSet );
		m_pClassLoadoutPanel->ShowPanel( iClassIndexToSet, false, (m_iPrevShowingPanel == CHAP_ARMORY) );

		iLabelClassToSet = m_iCurrentClassIndex;
	}

	m_iCurrentClassIndex = iClassIndexToSet;

	if( bOpenClassLoadout )
	{
		PostActionSignal( new KeyValues("ClassSelected", "class", m_iCurrentClassIndex ) );
	}
	else
	{
		m_iLabelSetToClass = iLabelClassToSet;
		m_bClassLayoutDirty = true;
		InvalidateLayout();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Show our changes label if we're alive, and hence won't get the changes immediately
	bool bChangesLabel = false;
	if ( engine->IsInGame() )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer->IsAlive() && pLocalPlayer->GetObserverMode() == OBS_MODE_NONE )
		{
			bChangesLabel = true;
		}
	}

	if ( !TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() )
	{
		bool bLoggedIntoSteam = steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUser()->BLoggedOn();
		if ( m_pNoGCLabel ) 
			m_pNoGCLabel->SetVisible( bLoggedIntoSteam );
		if ( m_pNoSteamLabel ) 
			m_pNoSteamLabel->SetVisible( !bLoggedIntoSteam );
		if ( m_pSelectLabel ) 
			m_pSelectLabel->SetVisible( false );
		if ( m_pLoadoutChangesLabel) 
			m_pLoadoutChangesLabel->SetVisible( false );

		for ( int i = 0; i < CHSB_NUM_BUTTONS; i++ )
		{
			m_pSubButtons[i]->SetVisible( false );
			m_pButtonLabels[i]->SetVisible( false );
		}
	}
	else
	{
		if ( m_pNoSteamLabel ) 
			m_pNoSteamLabel->SetVisible( false );
		if ( m_pNoGCLabel )
			m_pNoGCLabel->SetVisible( false );
		if ( m_pSelectLabel )
			m_pSelectLabel->SetVisible( true );

		for ( int i = 0; i < CHSB_NUM_BUTTONS; i++ )
		{
			m_pSubButtons[i]->SetVisible( true );
			m_pButtonLabels[i]->SetVisible( true );
		}
		if ( !bChangesLabel )
		{
			if ( m_pSelectLabel )
				m_pSelectLabel->SetPos( 0, m_iSelectLabelY );
			if ( m_pLoadoutChangesLabel ) 
				m_pLoadoutChangesLabel->SetVisible( false );
		}
		else
		{
			if ( m_pSelectLabel ) 
				m_pSelectLabel->SetPos( 0, m_iSelectLabelOnChangesY );
			if ( m_pLoadoutChangesLabel ) 
				m_pLoadoutChangesLabel->SetVisible( true );
		}
	}

	m_iOverSubButton = -1;
	if ( m_pSelectLabel )
		m_pClassLabel->SetVisible( false );
	if ( m_pItemsLabel )
		m_pItemsLabel->SetVisible( false );

	m_bClassLayoutDirty = false;

	// Now Layout the class images. 
	for ( int iPanel = 0; iPanel < ARRAYSIZE( g_nLoadoutClassOrder ); iPanel++ )
	{
		int i = g_nLoadoutClassOrder[iPanel];

		int iX = m_iClassLayout[i][0];
		int iY = m_iClassLayout[i][1];
		int iWide = m_iClassLayout[i][2];
		int iTall = m_iClassLayout[i][3];

		if ( m_bSnapClassLayout )
		{
			m_pClassButtons[i]->SetBounds( iX, iY, iWide, iTall );
		}
		else
		{
			// Lerp towards the target
			int iCurX, iCurY, iCurWide, iCurTall;
			m_pClassButtons[i]->GetBounds( iCurX, iCurY, iCurWide, iCurTall );
			int iNewX = Lerp( 0.2, iCurX, iX );
			int iNewY = Lerp( 0.2, iCurY, iY );
			int iNewWide = Lerp( 0.2, iCurWide, iWide );
			int iNewTall = Lerp( 0.2, iCurTall, iTall );
			m_pClassButtons[i]->SetBounds( iNewX, iNewY, iNewWide, iNewTall );
			if ( abs(iNewX-iX) > 5 || abs(iNewY-iY) > 5 || abs(iNewWide-iWide) > 5 || abs(iNewTall-iTall) > 5 )
			{
				m_bClassLayoutDirty = true;
			}
		}
	}

	// We need to do our own management of cursor arming in the buttons, because the curserentered/exited code can't
	// deal with the way we resize the buttons without the cursor moving. 
	int iBestButton = -1;
	int iBestZ = 0;
	int x = m_iMouseXPos, y = m_iMouseYPos;

	// only get the actual cursor pos if we don't have a cached cursor pos. THe
	// cached pos might have come from the keyboard.
	if( x < 0 )
		vgui::input()->GetCursorPos(x, y);
	for ( int iPanel = 0; iPanel < ARRAYSIZE( g_nLoadoutClassOrder ); iPanel++ )
	{
		int i = g_nLoadoutClassOrder[iPanel];
		m_pClassButtons[i]->SetArmed( false );

		m_pClassButtons[i]->SetEnabled( TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() );

		if ( m_pClassButtons[i]->IsWithin( x,y ) && iBestZ < m_pClassButtons[i]->GetZPos() )
		{
			iBestButton = i;
			iBestZ = m_pClassButtons[i]->GetZPos();
		}
	}

	if ( iBestButton >= 0 && iBestButton < ARRAYSIZE( m_pClassButtons ) )
	{
		m_pClassButtons[iBestButton]->SetArmed( true );

		if ( m_iLabelSetToClass != iBestButton )
		{
			m_iLabelSetToClass = iBestButton;
		}

		UpdateLabelFromClass( m_iLabelSetToClass );
	}

	m_bSnapClassLayout = false;
}

void CCharInfoLoadoutSubPanel::UpdateLabelFromClass( int nClass )
{
	if ( nClass < 0 )
		return;

	const wchar_t *wszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[nClass] );
	if ( m_pClassLabel )
	{
		m_pClassLabel->SetText( wszClassName );
		m_pClassLabel->SetVisible( true );
	}

	if ( m_pItemsLabel )
	{
		m_pItemsLabel->SetVisible( true );
	}

	CUtlVector<CEconItemView*> pList;
	int iNumItems = TFInventoryManager()->GetAllUsableItemsForSlot( nClass, -1, &pList );

	if ( !iNumItems )
	{
		const wchar_t *wszItemsName = g_pVGuiLocalize->Find( "#NoItemsFoundShort" );
		m_pItemsLabel->SetText( wszItemsName );
		m_pItemsLabel->SetColorStr( m_ItemColorNone );
	}
	else if ( iNumItems == 1 )
	{
		const wchar_t *wszItemsName = g_pVGuiLocalize->Find( "#ItemsFoundShortOne" );
		m_pItemsLabel->SetText( wszItemsName );
		m_pItemsLabel->SetColorStr( m_ItemColor );
	}
	else
	{
		wchar_t wzCount[10];
		_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iNumItems );
		wchar_t	wTemp[32];
		g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find("ItemsFoundShort"), 1, wzCount );
		m_pItemsLabel->SetText( wTemp );
		m_pItemsLabel->SetColorStr( m_ItemColor );
	}

	int iPos = 0;
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= NUM_CLASSES_IN_LOADOUT_PANEL; i++ )
	{
		if ( iRemapIndexToClass[i] == nClass )
		{
			iPos = i;
			break;
		}
	}
	Assert(iPos != 0 );
	int iXLeft = (GetWide() - ((m_iClassWideMin * NUM_CLASSES_IN_LOADOUT_PANEL) + (m_iClassXDelta * (NUM_CLASSES_IN_LOADOUT_PANEL-1)))) * 0.5;
	int iBaseX = iXLeft + ((m_iClassWideMin + m_iClassXDelta) * (iPos-1));
	int iCenterX = iBaseX + (m_iClassWideMin * 0.5);

	m_pClassLabel->SetVisible( true );
	m_pClassLabel->SetPos( iCenterX - (m_pClassLabel->GetWide() * 0.5), m_iClassLabelYPos );
	m_pItemsLabel->SetVisible( true );
	m_pItemsLabel->SetPos( iCenterX - (m_pItemsLabel->GetWide() * 0.5), m_iItemLabelYPos );
}

void CCharInfoLoadoutSubPanel::UpdateLabelFromSubButton( int nButton )
{
	if( nButton < 0 )
		nButton = CHSB_NUM_BUTTONS - 1;
	else if( nButton >= CHSB_NUM_BUTTONS )
		nButton = 0;

	if ( m_iOverSubButton == nButton )
		return;

	m_iOverSubButton = nButton;

	switch ( nButton )
	{
	case CHSB_BACKPACK:
		{
			int iNumItems = TFInventoryManager()->GetLocalTFInventory()->GetItemCount();
			if ( iNumItems == 1 )
			{
				const wchar_t *wszItemsName = g_pVGuiLocalize->Find( "#Loadout_OpenBackpackDesc1" );
				m_pItemsLabel->SetText( wszItemsName );
			}
			else
			{
				wchar_t wzCount[10];
				_snwprintf( wzCount, ARRAYSIZE( wzCount ), L"%d", iNumItems );
				wchar_t	wTemp[32];
				g_pVGuiLocalize->ConstructString_safe( wTemp, g_pVGuiLocalize->Find("Loadout_OpenBackpackDesc"), 1, wzCount );
				m_pItemsLabel->SetText( wTemp );
			}
		}
		break;
	case CHSB_CRAFTING:
		m_pItemsLabel->SetText( g_pVGuiLocalize->Find( "Loadout_OpenCraftingDesc" ) );
		break;
	case CHSB_ARMORY:
		m_pItemsLabel->SetText( g_pVGuiLocalize->Find( "Loadout_OpenArmoryDesc" ) );
		break;
	case CHSB_TRADING:
		m_pItemsLabel->SetText( g_pVGuiLocalize->Find( "Loadout_OpenTradingDesc" ) );
		break;
	case CHSB_PAINTKITS:
		m_pItemsLabel->SetText( g_pVGuiLocalize->Find( "Loadout_OpenPaintkitPreview" ) );
		break;
	default:
		Assert( false );
	}

	int iX, iY;
	m_pSubButtons[nButton]->GetPos( iX, iY );
	iX += (m_pSubButtons[nButton]->GetWide() * 0.5);
	iY += m_pSubButtons[nButton]->GetTall() + YRES(5);

	m_pItemsLabel->SetVisible( true );
	m_pItemsLabel->SetPos( iX - (m_pItemsLabel->GetWide() * 0.5), iY + (m_iItemLabelYPos - m_iClassLabelYPos) );
	m_pItemsLabel->SetColorStr( m_ItemColor );

	for ( int i = 0; i < CHSB_NUM_BUTTONS; i++ )
	{
		m_pSubButtons[i]->SetArmed( false );
	}

	m_pSubButtons[nButton]->SetArmed( true );
	m_pSubButtons[nButton]->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnTick( void )
{
	if ( m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
		return;
	if ( !IsVisible() )
		return;

	if ( m_bRequestingInventoryRefresh && TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() )
	{
		m_bRequestingInventoryRefresh = false;
		CloseWaitingDialog();
		return;
	}

	// if the class layout is dirty, invalidate our layout so that
	// we'll animate the class buttons.
	if ( m_bClassLayoutDirty )
	{
		InvalidateLayout();
	}

	if ( !HasFocus() )
		return;

	// Don't respond to the mouse if we don't have items
	if ( !TFInventoryManager()->GetLocalTFInventory()->RetrievedInventoryFromSteam() )
		return;

	if ( m_flStartExplanationsAt && m_flStartExplanationsAt < engine->Time() )
	{
		m_flStartExplanationsAt = 0;

		if ( ShouldShowExplanations() )
		{
			tf_explanations_charinfopanel.SetValue( 1 );

			CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( FindChildByName("StartExplanation") );
			if ( pPopup )
			{
				pPopup->Popup();
			}
		}
	}

}


//-----------------------------------------------------------------------------
// Purpose: Handles mousing over classes
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::OnCursorMoved( int x, int y )
{
	RecalculateTargetClassLayoutAtPos( x, y );
}


//-----------------------------------------------------------------------------
// Purpose: Handles setting the highlighted class for both mouse and keyboard 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::RecalculateTargetClassLayoutAtPos( int x, int y )
{
	// Ignore mouse movement outside the buttons
	bool bWithin = false;
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= NUM_CLASSES_IN_LOADOUT_PANEL; i++ )
	{
		if ( m_pClassButtons[i]->IsWithin(x,y) )
		{
			bWithin = true;
			break;
		}
	}

	if ( bWithin )
	{
		m_iMouseXPos = x;
		m_iMouseYPos = y;
		RecalculateTargetClassLayout();
		m_bClassLayoutDirty = true;
	}
	else
	{
		// See if we're over a sub button
		bool bOverSubButton = false;
		for ( int i = 0; i < CHSB_NUM_BUTTONS; i++ )
		{
			if ( m_pSubButtons[i]->IsWithin(x,y) )
			{
				bOverSubButton = true;
				UpdateLabelFromSubButton( i );
			}
		}

		if ( !bOverSubButton && m_pClassLabel->IsVisible() )
		{
			// Hide the class label
			if ( m_iMouseXPos != -1 )
			{
				m_iMouseXPos = -1;
				RecalculateTargetClassLayout();
				m_bClassLayoutDirty = true;
			}

			m_iOverSubButton = -1;
			m_iLabelSetToClass = -1;
			m_pClassLabel->SetVisible( false );
			m_pItemsLabel->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCharInfoLoadoutSubPanel::RecalculateTargetClassLayout( void )
{
	// Now Layout the class images. 
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= NUM_CLASSES_IN_LOADOUT_PANEL; i++ )
	{
		int iIndex = GetRemappedMenuIndexForClass(i);

		// Figure out where we'd be unscaled
		int iXLeft = (GetWide() - ((m_iClassWideMin * NUM_CLASSES_IN_LOADOUT_PANEL) + (m_iClassXDelta * (NUM_CLASSES_IN_LOADOUT_PANEL-1)))) * 0.5;
		int iBaseX = iXLeft + ((m_iClassWideMin + m_iClassXDelta) * (iIndex-1));

		// Scale based on distance from the mouse cursor.
		int iCenterX = iBaseX + (m_iClassWideMin * 0.5);

		float flScale = 0.0;
		if ( m_iMouseXPos >= 0 )
		{
			flScale = RemapValClamped( abs(m_iMouseXPos - iCenterX), m_iClassDistanceMin, m_iClassDistanceMax, 1.0, 0.0 );
		}

		float iWide = RemapValClamped( flScale, 0.0, 1.0, m_iClassWideMin, m_iClassWideMax );
		float iTall = RemapValClamped( flScale, 0.0, 1.0, m_iClassTallMin, m_iClassTallMax );

		int iY = m_iClassYPos - ((iTall - m_iClassTallMin) * 0.5);
		int iX = iBaseX - ((iWide - m_iClassWideMin) * 0.5);

		m_pClassButtons[i]->SetZPos( flScale * 100 );

		// Cache off the target bounds for this class button
		m_iClassLayout[i][0] = iX;
		m_iClassLayout[i][1] = iY;
		m_iClassLayout[i][2] = iWide;
		m_iClassLayout[i][3] = iTall;
	}
}

void CCharInfoLoadoutSubPanel::MoveCharacterSelection( int nDirection )
{
	int nCurrent = 0;

	if ( m_iLabelSetToClass != -1 )
	{
		for ( int i = 0; i < ARRAYSIZE( g_nLoadoutClassOrder ); i++ )
		{
			if ( m_iLabelSetToClass == g_nLoadoutClassOrder[ i ] )
			{
				nCurrent = i;
				break;
			}
		}

		nCurrent += nDirection;

		if ( nCurrent < 0 )
		{
			nCurrent = ARRAYSIZE( g_nLoadoutClassOrder ) - 1;
		}
		else if ( nCurrent >= ARRAYSIZE( g_nLoadoutClassOrder ) )
		{
			nCurrent = 0;
		}
	}

	for ( int i = 0; i < ARRAYSIZE( g_nLoadoutClassOrder ); i++ )
	{
		m_pClassButtons[ g_nLoadoutClassOrder[ i ] ]->SetArmed( false );
	}

	// animate the class buttons
	CImageButton *pButton = m_pClassButtons[ g_nLoadoutClassOrder[ nCurrent ] ];
	int x, y, wide, tall;
	pButton->GetBounds( x, y, wide, tall );
	RecalculateTargetClassLayoutAtPos( x + wide/2, y + tall/2 );

	pButton->RequestFocus();
}

void CCharInfoLoadoutSubPanel::OnKeyCodeTyped(vgui::KeyCode code)
{
	// turn off key handling in this panel when we're showing a loadout 
	// for one class
	if ( m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
	{
		// let escape and B (aka "go back") through so we 
		// can actually get out of the loadout screen
		if ( code == KEY_ESCAPE )
		{
			BaseClass::OnKeyCodePressed( code );
		}
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CCharInfoLoadoutSubPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	// turn off key handling in this panel when we're showing a loadout 
	// for one class
	if( m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
	{
		// let escape and B (aka "go back") through so we 
		// can actually get out of the loadout screen
		if ( nButtonCode == KEY_XBUTTON_B )
		{
			BaseClass::OnKeyCodePressed( code );
		}
		return;
	}

	if ( nButtonCode == KEY_XBUTTON_LEFT || 
		 nButtonCode == KEY_XSTICK1_LEFT ||
		 nButtonCode == KEY_XSTICK2_LEFT ||
		 nButtonCode == STEAMCONTROLLER_DPAD_LEFT ||
		 code == KEY_LEFT )
	{
		if ( m_iLabelSetToClass != -1 )
		{
			MoveCharacterSelection( -1 );
		}
		else
		{
			UpdateLabelFromSubButton( m_iOverSubButton - 1 );
		}
		return;
	}
	else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
			  nButtonCode == KEY_XSTICK1_RIGHT ||
			  nButtonCode == KEY_XSTICK2_RIGHT ||
			  nButtonCode == STEAMCONTROLLER_DPAD_RIGHT ||
			  code == KEY_RIGHT )
	{
		if ( m_iLabelSetToClass != -1 )
		{
			MoveCharacterSelection( 1 );
		}
		else
		{
			UpdateLabelFromSubButton( m_iOverSubButton + 1 );
		}
		return;
	}
	else if ( nButtonCode == KEY_XBUTTON_UP || 
			  nButtonCode == KEY_XSTICK1_UP ||
			  nButtonCode == KEY_XSTICK2_UP ||
			  nButtonCode == STEAMCONTROLLER_DPAD_UP ||
			  code == KEY_UP )
	{
		if ( m_iLabelSetToClass == -1 )
		{
			m_iLabelSetToClass = g_nLoadoutClassOrder[ 0 ];
			CImageButton *pButton = m_pClassButtons[ m_iLabelSetToClass ];
			UpdateLabelFromClass( m_iLabelSetToClass );

			int x, y, wide, tall;
			pButton->GetBounds( x, y, wide, tall );
			RecalculateTargetClassLayoutAtPos( x + wide/2, y + tall/2 );
			pButton->RequestFocus();

		}
		return;
	}
	else if ( nButtonCode == KEY_XBUTTON_DOWN || 
			  nButtonCode == KEY_XSTICK1_DOWN ||
			  nButtonCode == KEY_XSTICK2_DOWN ||
			  nButtonCode == STEAMCONTROLLER_DPAD_DOWN ||
			  code == KEY_DOWN )
	{
		if ( m_iLabelSetToClass != -1 )
		{
			m_iLabelSetToClass = -1;
			m_pClassLabel->SetVisible( false );
			m_pItemsLabel->SetVisible( false );

			for ( int iPanel = 0; iPanel < ARRAYSIZE( g_nLoadoutClassOrder ); iPanel++ )
			{
				int i = g_nLoadoutClassOrder[iPanel];
				m_pClassButtons[i]->SetArmed( false );
			}

			UpdateLabelFromSubButton( 0 );
		}
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}