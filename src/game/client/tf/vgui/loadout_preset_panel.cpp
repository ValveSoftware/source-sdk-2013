//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"
#include "loadout_preset_panel.h"
#include "tf_item_inventory.h"
#include "econ/econ_item_preset.h"
#include "econ/econ_item_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

DECLARE_BUILD_FACTORY( CLoadoutPresetPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLoadoutPresetPanel::CLoadoutPresetPanel( vgui::Panel *pParent, const char *pName )
:	EditablePanel( pParent, "loadout_preset_panel" )
{
	V_memset( m_pPresetButtons, 0, sizeof( m_pPresetButtons ) );

	m_iClass = TF_CLASS_UNDEFINED;
	m_pPresetButtonKv = NULL;
	m_bDisplayVertical = false;

	// Create all buttons
	for ( int i = 0; i < MAX_PRESETS; ++i )
	{
		CFmtStr fmtTokenName( "TF_ItemPresetName%i", i );
		CFmtStr fmtButtonName( "LoadPresetButton%i", i );
		wchar_t *pwszPresetName = g_pVGuiLocalize->Find( fmtTokenName.Access() );
		m_pPresetButtons[i] = new CExButton( this, fmtButtonName.Access(), pwszPresetName, this );
	}

	m_pClassLoadoutPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/LoadoutPresetPanel.res" );

	m_aDefaultColors[LOADED][FG][DEFAULT] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetDefaultColorFg", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[LOADED][FG][ARMED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetArmedColorFg", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[LOADED][FG][DEPRESSED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetDepressedColorFg", Color( 255, 255, 255, 255 ) );

	m_aDefaultColors[LOADED][BG][DEFAULT] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetDefaultColorBg", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[LOADED][BG][ARMED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetArmedColorBg", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[LOADED][BG][DEPRESSED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Econ.Button.PresetDepressedColorBg", Color( 255, 255, 255, 255 ) );

	m_aDefaultColors[NOTLOADED][FG][DEFAULT] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.TextColor", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[NOTLOADED][FG][ARMED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.ArmedTextColor", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[NOTLOADED][FG][DEPRESSED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.DepressedTextColor", Color( 255, 255, 255, 255 ) );

	m_aDefaultColors[NOTLOADED][BG][DEFAULT] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.BgColor", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[NOTLOADED][BG][ARMED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.ArmedBgColor", Color( 255, 255, 255, 255 ) );
	m_aDefaultColors[NOTLOADED][BG][DEPRESSED] = vgui::scheme()->GetIScheme( GetScheme() )->GetColor( "Button.DepressedBgColor", Color( 255, 255, 255, 255 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	KeyValues *pPresetButtonKv = pInResourceData->FindKey( "presetbutton_kv" );
	if ( pPresetButtonKv && !m_pPresetButtonKv )
	{
		m_pPresetButtonKv = new KeyValues( "presetbutton_kv" );
		pPresetButtonKv->CopySubkeys( m_pPresetButtonKv );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::PerformLayout() 
{
	BaseClass::PerformLayout();

	if ( !m_pPresetButtons[0] )
		return;

	const int nBuffer = XRES( 2 );

	for ( int i = 0; i < MAX_PRESETS; ++i )
	{
		if ( m_pPresetButtonKv )
		{
			m_pPresetButtons[i]->ApplySettings( m_pPresetButtonKv );
		}

		// Display buttons vertically or horizontally?
		// NOTE: Button width and height will be valid here, since we've just applied settings
		if ( m_bDisplayVertical )
		{
			const int nButtonHeight = m_pPresetButtons[0]->GetTall();
			m_pPresetButtons[i]->SetPos( 0, i * ( nButtonHeight + nBuffer ) );
		}
		else
		{
			const int nButtonWidth = m_pPresetButtons[0]->GetWide();
			const int nStartX = 0.5f * ( GetWide() - MAX_PRESETS * ( nButtonWidth + nBuffer ) );
			m_pPresetButtons[i]->SetPos( nStartX + i * ( nButtonWidth + nBuffer ), 0 );
		}
		m_pPresetButtons[i]->SetVisible( true );
	}

	vgui::ivgui()->AddTickSignal( GetVPanel(), 150 );

	UpdatePresetButtonStates();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::SetClass( int iClass )
{
	m_iClass = iClass;

	if ( iClass != TF_CLASS_UNDEFINED )
	{
		UpdatePresetButtonStates();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::EnableVerticalDisplay( bool bVertical )
{
	m_bDisplayVertical = bVertical;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::LoadPreset( int iPresetIndex )
{
	TFInventoryManager()->LoadPreset( m_iClass, iPresetIndex );

	if (m_pClassLoadoutPanel)
	{
		m_pClassLoadoutPanel->UpdateModelPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::OnCommand( const char *command )
{
	if ( !V_strnicmp( command, "loadpreset_", 11 ) )
	{
		const int iPresetIndex = atoi( command + 11 );
		LoadPreset( iPresetIndex );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::OnTick()
{
	UpdatePresetButtonStates();
}

//-----------------------------------------------------------------------------
// Purpose: Processes some keypresses for the loadout panel
//-----------------------------------------------------------------------------
bool CLoadoutPresetPanel::HandlePresetKeyPressed( vgui::KeyCode code )
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if( nButtonCode == KEY_XBUTTON_LEFT_SHOULDER )
	{
		if( GetSelectedPresetID() > 0 )
			LoadPreset( GetSelectedPresetID() - 1 );
		return true;
	}
	else if( nButtonCode == KEY_XBUTTON_RIGHT_SHOULDER )
	{
		if( GetSelectedPresetID() < MAX_PRESETS - 1 )
			LoadPreset( GetSelectedPresetID() + 1 );
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
equipped_preset_t CLoadoutPresetPanel::GetSelectedPresetID() const
{
	if ( !InventoryManager()->GetLocalInventory() )
		return INVALID_PRESET_INDEX;

	const uint32 unAccountID = InventoryManager()->GetLocalInventory()->GetOwner().GetAccountID();
	const CEconItemPerClassPresetData soSearch( unAccountID, m_iClass );

	GCSDK::CSharedObjectCache *pSOCache = InventoryManager()->GetLocalInventory()->GetSOC();
	if ( !pSOCache )
		return INVALID_PRESET_INDEX;

	const CEconItemPerClassPresetData *pExistingPerClassData = assert_cast<CEconItemPerClassPresetData *>( pSOCache->FindSharedObject( soSearch ) );
	if ( !pExistingPerClassData )
		return INVALID_PRESET_INDEX;

	return pExistingPerClassData->GetActivePreset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLoadoutPresetPanel::UpdatePresetButtonStates()
{
	equipped_preset_t unEquippedPresetID = GetSelectedPresetID();

	CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
	CTFPlayerInventory *pInv = TFInventoryManager()->GetInventoryForPlayer(localSteamID);
	if (pInv) {
		unEquippedPresetID = pInv->GetActiveLocalPreset(m_iClass);
	}

	for ( int i = 0; i < MAX_PRESETS; ++i )
	{
		if ( i == unEquippedPresetID )
		{
			m_pPresetButtons[i]->SetDefaultColor( m_aDefaultColors[LOADED][FG][DEFAULT], m_aDefaultColors[LOADED][BG][DEFAULT] );
			m_pPresetButtons[i]->SetArmedColor( m_aDefaultColors[LOADED][FG][ARMED], m_aDefaultColors[LOADED][BG][ARMED] );
			m_pPresetButtons[i]->SetDepressedColor( m_aDefaultColors[LOADED][FG][DEPRESSED], m_aDefaultColors[LOADED][BG][DEPRESSED] );
		}
		else
		{
			m_pPresetButtons[i]->SetDefaultColor( m_aDefaultColors[NOTLOADED][FG][DEFAULT], m_aDefaultColors[NOTLOADED][BG][DEFAULT] );
			m_pPresetButtons[i]->SetArmedColor( m_aDefaultColors[NOTLOADED][FG][ARMED], m_aDefaultColors[NOTLOADED][BG][ARMED] );
			m_pPresetButtons[i]->SetDepressedColor( m_aDefaultColors[NOTLOADED][FG][DEPRESSED], m_aDefaultColors[NOTLOADED][BG][DEPRESSED] );
		}

		CFmtStr fmtCmd( "loadpreset_%i", i );
		m_pPresetButtons[i]->SetCommand( fmtCmd.Access() );
	}
}
