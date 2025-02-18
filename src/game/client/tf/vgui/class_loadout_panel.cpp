//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "class_loadout_panel.h"
#include "c_tf_player.h"
#include "vgui_controls/CheckButton.h"
#include "econ_gcmessages.h"
#include "gc_clientsystem.h"
#include "tf_item_system.h"
#include "loadout_preset_panel.h"
#include "econ_item_description.h"
#include "item_style_select_dialog.h"
#include "vgui/IInput.h"
#include "vgui_controls/PanelListPanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern ConVar tf_respawn_on_loadoutchanges;

ConVar tf_show_preset_explanation_in_class_loadout( "tf_show_preset_explanation_in_class_loadout", "1", FCVAR_HIDDEN | FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar tf_show_taunt_explanation_in_class_loadout( "tf_show_taunt_explanation_in_class_loadout", "1", FCVAR_HIDDEN | FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

void ParticleSlider_UpdateRequest( int iLoadoutPosition, float value )
{
	CClassLoadoutPanel *pPanel = g_pClassLoadoutPanel;
	if ( !pPanel )
		return;

	CEconItemView *pHat = pPanel->GetItemInSlot( iLoadoutPosition );
	if ( !pHat )
		return;

	// does this hat even have a particle effect
	static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
	uint32 iHasEffect = 0;
	if ( !pHat->FindAttribute( pAttrDef_AttachParticleEffect, &iHasEffect ) )
		return;

	// Check for use head toggle
	static CSchemaAttributeDefHandle pAttrDef_UseHeadOrigin( "particle effect use head origin" );
	uint32 iUseHead = 0;
	if ( !pHat->FindAttribute( pAttrDef_UseHeadOrigin, &iUseHead ) || iUseHead == 0 )
		return;

	// Look for the attribute and request to change it
	static CSchemaAttributeDefHandle pAttrDef_VerticalOffset( "particle effect vertical offset" );
	uint32 iOffSet = 0;
	if ( !pHat->FindAttribute( pAttrDef_VerticalOffset, &iOffSet ) && value == 0 )
	{
		return;
	}
	else
	{
		const float& flAttrValue = (float&)iOffSet;
		if ( value == flAttrValue )
			return; // no change do nothing
	}

	// Send a message to the GC to request a change
	GCSDK::CProtoBufMsg<CMsgSetItemEffectVerticalOffset> msg( k_EMsgGCSetItemEffectVerticalOffset );
	msg.Body().set_item_id( pHat->GetItemID() );
	msg.Body().set_offset( value );
	GCClientSystem()->BSendMessage( msg );
}

void HatOffset_Callback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	ConVarRef cVarRef( pConVar );
	ParticleSlider_UpdateRequest( LOADOUT_POSITION_HEAD, cVarRef.GetFloat() );
}
ConVar tf_hat_effect_offset( "tf_hat_effect_offset", "0", FCVAR_HIDDEN, "Adjust the position of the unusual effect for your hat.", true, -8.0f, true, 8.0f, HatOffset_Callback );

void Misc1Offset_Callback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	ConVarRef cVarRef( pConVar );
	ParticleSlider_UpdateRequest( LOADOUT_POSITION_MISC, cVarRef.GetFloat() );
}
ConVar tf_misc1_effect_offset( "tf_misc1_effect_offset", "0", FCVAR_HIDDEN, "Adjust the position of the unusual effect for your hat.", true, -8.0f, true, 8.0f, Misc1Offset_Callback );

void Misc2Offset_Callback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	ConVarRef cVarRef( pConVar );
	ParticleSlider_UpdateRequest( LOADOUT_POSITION_MISC2, cVarRef.GetFloat() );
}
ConVar tf_misc2_effect_offset( "tf_misc2_effect_offset", "0", FCVAR_HIDDEN, "Adjust the position of the unusual effect for your hat.", true, -8.0f, true, 8.0f, Misc2Offset_Callback );


// Hacky solution to different classes wanting different slots visible in their loadouts, and in different positions
struct LoadoutPanelPositioningInstance
{
	int m_iPos[NUM_ITEM_PANELS_IN_LOADOUT];
};

bool IsTauntPanelPosition( int iButtonPos )
{
	return iButtonPos >= 9 && iButtonPos <= 16;
}

const LoadoutPanelPositioningInstance g_DefaultLoadoutPanelPositioning =
{
	{
		1,	// LOADOUT_POSITION_PRIMARY = 0,
		2,	// LOADOUT_POSITION_SECONDARY,
		3,	// LOADOUT_POSITION_MELEE,
		0,	// LOADOUT_POSITION_UTILITY,  // STAGING ONLY
		0,	// LOADOUT_POSITION_BUILDING,
		0,	// LOADOUT_POSITION_PDA,
		0,	// LOADOUT_POSITION_PDA2,
		5,	// LOADOUT_POSITION_HEAD,
		6,	// LOADOUT_POSITION_MISC,
		8,	// LOADOUT_POSITION_ACTION,
		7,	// LOADOUT_POSITION_MISC2,
		9,	// LOADOUT_POSITION_TAUNT,
		10,	// LOADOUT_POSITION_TAUNT2,
		11,	// LOADOUT_POSITION_TAUNT3,
		12,	// LOADOUT_POSITION_TAUNT4,
		13,	// LOADOUT_POSITION_TAUNT5,
		14,	// LOADOUT_POSITION_TAUNT6,
		15,	// LOADOUT_POSITION_TAUNT7,
		16,	// LOADOUT_POSITION_TAUNT8,

	}
};

const LoadoutPanelPositioningInstance g_LoadoutPanelPositioning_Spy =
{
	{
		0,	// LOADOUT_POSITION_PRIMARY = 0,
		1,	// LOADOUT_POSITION_SECONDARY,
		2,	// LOADOUT_POSITION_MELEE,
		0,	// LOADOUT_POSITION_UTILITY,  // STAGING ONLY
		4,	// LOADOUT_POSITION_BUILDING,		// sapper
		0,	// LOADOUT_POSITION_PDA,			// disguise kit (Hidden)
		3,	// LOADOUT_POSITION_PDA2,			// Watch
		5,	// LOADOUT_POSITION_HEAD,
		6,	// LOADOUT_POSITION_MISC,
		8,	// LOADOUT_POSITION_ACTION,
		7,	// LOADOUT_POSITION_MISC2,
		9,	// LOADOUT_POSITION_TAUNT,
		10,	// LOADOUT_POSITION_TAUNT2,
		11,	// LOADOUT_POSITION_TAUNT3,
		12,	// LOADOUT_POSITION_TAUNT4,
		13,	// LOADOUT_POSITION_TAUNT5,
		14,	// LOADOUT_POSITION_TAUNT6,
		15,	// LOADOUT_POSITION_TAUNT7,
		16,	// LOADOUT_POSITION_TAUNT8,
	}
};

const LoadoutPanelPositioningInstance g_LoadoutPanelPositioning_Engineer =
{
	{
		1,	// LOADOUT_POSITION_PRIMARY = 0,
		2,	// LOADOUT_POSITION_SECONDARY,
		3,	// LOADOUT_POSITION_MELEE,
		0,	// LOADOUT_POSITION_UTILITY,  // STAGING ONLY
		0,	// LOADOUT_POSITION_BUILDING,
		4,	// LOADOUT_POSITION_PDA,
		0,	// LOADOUT_POSITION_PDA2,
		5,	// LOADOUT_POSITION_HEAD,
		6,	// LOADOUT_POSITION_MISC,
		8,	// LOADOUT_POSITION_ACTION,
		7,	// LOADOUT_POSITION_MISC2,
		9,	// LOADOUT_POSITION_TAUNT,
		10,	// LOADOUT_POSITION_TAUNT2,
		11,	// LOADOUT_POSITION_TAUNT3,
		12,	// LOADOUT_POSITION_TAUNT4,
		13,	// LOADOUT_POSITION_TAUNT5,
		14,	// LOADOUT_POSITION_TAUNT6,
		15,	// LOADOUT_POSITION_TAUNT7,
		16,	// LOADOUT_POSITION_TAUNT8,
	}
};

const LoadoutPanelPositioningInstance *g_VisibleLoadoutSlotsPerClass[] =
{
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_UNDEFINED
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_SCOUT
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_SNIPER
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_SOLDIER
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_DEMOMAN
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_MEDIC
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_HEAVYWEAPONS
	&g_DefaultLoadoutPanelPositioning,			// TF_CLASS_PYRO
	&g_LoadoutPanelPositioning_Spy,				// TF_CLASS_SPY
	&g_LoadoutPanelPositioning_Engineer,		// TF_CLASS_ENGINEER
};

COMPILE_TIME_ASSERT( ARRAYSIZE( g_VisibleLoadoutSlotsPerClass ) == TF_LAST_NORMAL_CLASS );

//-----------------------------------------------------------------------------
// Particle Effect Slider
//-----------------------------------------------------------------------------
CLoadoutItemOptionsPanel::CLoadoutItemOptionsPanel( Panel *parent, const char *pName ) : vgui::EditablePanel( parent, pName )
{
	m_pHatParticleSlider = NULL;
	m_pHatParticleUseHeadButton = NULL;

	m_iCurrentClassIndex = -1;
	m_eItemSlot = LOADOUT_POSITION_INVALID;

	m_pListPanel = new vgui::PanelListPanel( this, "PanelListPanel" );
	m_pListPanel->SetFirstColumnWidth( 0 );
	m_pHatParticleSlider = new CCvarSlider( m_pListPanel, "HatParticleSlider" );
	m_pHatParticleSlider->AddActionSignalTarget( this );
	m_pHatParticleUseHeadButton = new vgui::CheckButton( m_pListPanel, "HatUseHeadCheckButton", "#GameUI_ParticleHatUseHead" );
	m_pHatParticleUseHeadButton->AddActionSignalTarget( this );
	m_pSetStyleButton = new CExButton( m_pListPanel, "SetStyleButton", "#TF_Item_SelectStyle" );
	m_pSetStyleButton->AddActionSignalTarget( this );
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/ItemOptionsPanel.res" );
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();
	m_pHatParticleSlider->SetTickColor( Color( 235, 226, 202, 255 ) ); // tanlight
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "particle_button_clicked" ) )
	{
		UpdateItemOptionsUI();
		return;
	}
	else if ( FStrEq( command, "particle_use_head_clicked" ) )
	{
		// Grab current hat
		CEconItemView *pHat = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, m_eItemSlot );
		if ( !pHat )
			return;

		// does this hat even have a particle effect
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
		uint32 iHasEffect = 0;
		if ( !pHat->FindAttribute( pAttrDef_AttachParticleEffect, &iHasEffect ) )
			return;

		// Send a message to the GC to request a change
		GCSDK::CProtoBufMsg<CMsgSetHatEffectUseHeadOrigin> msg( k_EMsgGCSetHatEffectUseHeadOrigin );
		msg.Body().set_item_id( pHat->GetItemID() );
		msg.Body().set_use_head( m_pHatParticleUseHeadButton->IsSelected() );
		GCClientSystem()->BSendMessage( msg );
		return;
	}
	else if ( FStrEq( command, "set_style" ) )
	{
		CEconItemView *pHat = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, m_eItemSlot );
		CStyleSelectDialog *pStyle = vgui::SETUP_PANEL( new CStyleSelectDialog( GetParent(), pHat ) );
		if ( pStyle )
		{
			pStyle->Show();
		}
	}
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::OnMessage( const KeyValues* pParams, vgui::VPANEL hFromPanel )
{
	if ( FStrEq( pParams->GetName(), "SliderDragEnd" ) )
	{
		m_pHatParticleSlider->ApplyChanges();
	}

	BaseClass::OnMessage( pParams, hFromPanel );
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::SetItemSlot( loadout_positions_t eItemSlot, int iClassIndex )
{
	m_eItemSlot = eItemSlot;
	m_iCurrentClassIndex = iClassIndex;
	// Init the Slider based on the slot
	const char * pszConVarName = NULL;

	switch ( eItemSlot )
	{
	case LOADOUT_POSITION_HEAD :
		pszConVarName = "tf_hat_effect_offset";
		break;
	case LOADOUT_POSITION_MISC :
		pszConVarName = "tf_misc1_effect_offset";
		break;
	case LOADOUT_POSITION_MISC2 :
		pszConVarName = "tf_misc2_effect_offset";
		break;
	default:
		break;
	}
	
	if ( pszConVarName )
	{
		m_pHatParticleSlider->SetupSlider( -8, 8, pszConVarName, false );
	}
	
	m_pHatParticleSlider->SetTickColor( Color( 235, 226, 202, 255 ) ); // tanlight
	m_pHatParticleSlider->SetTickCaptions( "", "" );
	
	UpdateItemOptionsUI();
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::UpdateItemOptionsUI()
{
	if ( m_eItemSlot == LOADOUT_POSITION_INVALID )
		return;

	m_pListPanel->RemoveAll();

	// Add controls for various item options
	AddControlsParticleEffect();
	AddControlsSetStyle();

	// Bail if no controls added
	if ( m_pListPanel->GetItemCount() == 0 )
	{
		// We should have some controls if we get to this point.
		Assert( 0 );
		SetVisible( false );
		return;
	}

	// Resize the background and list panel to contain all the controls
	int nVertPixels = m_pListPanel->ComputeVPixelsNeeded();
	int nMinTall = YRES( 200 );
	int nNewTall = Min( nMinTall, nVertPixels );
	m_pListPanel->SetTall( nNewTall );
	SetTall( nNewTall );
	InvalidateLayout( true, false );
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::AddControlsParticleEffect( void ) const
{
	m_pHatParticleUseHeadButton->SetVisible( false );
	m_pHatParticleSlider->SetVisible( false );

	CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, m_eItemSlot );
	if ( pItem )
	{
		// does this hat even have a particle effect
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
		uint32 iValue = 0;
		if ( pItem->FindAttribute( pAttrDef_AttachParticleEffect, &iValue ) )
		{
			m_pHatParticleUseHeadButton->SetVisible( true );
			m_pListPanel->AddItem( NULL, m_pHatParticleUseHeadButton );
			m_pListPanel->AddItem( NULL, m_pHatParticleSlider );

			// Check for use head toggle
			static CSchemaAttributeDefHandle pAttrDef_UseHeadOrigin( "particle effect use head origin" );
			uint32 iUseHead = 0;
			if ( pItem->FindAttribute( pAttrDef_UseHeadOrigin, &iUseHead ) && iUseHead > 0 )
			{
				m_pHatParticleSlider->SetVisible( true );

				m_pHatParticleUseHeadButton->SetSelected( true );
				m_pHatParticleSlider->SetTickColor( Color( 235, 226, 202, 255 ) ); // tanlight
				m_pHatParticleSlider->Repaint();

				// Get offset if it exists
				static CSchemaAttributeDefHandle pAttrDef_VerticalOffset( "particle effect vertical offset" );
				uint32 iOffset = 0;
				if ( pItem->FindAttribute( pAttrDef_VerticalOffset, &iOffset ) )
				{
					m_pHatParticleSlider->SetSliderValue( (float&)iOffset );
				}
			}
			else
			{
				m_pHatParticleUseHeadButton->SetSelected( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CLoadoutItemOptionsPanel::AddControlsSetStyle( void ) const
{
	m_pSetStyleButton->SetVisible( false );

	CEconItemView *pItem = GetItem();
	if ( pItem && pItem->GetStaticData()->GetNumStyles() )
	{
		m_pSetStyleButton->SetVisible( true );
		m_pListPanel->AddItem( NULL, m_pSetStyleButton );
	}
}

//-----------------------------------------------------------------------------
CEconItemView* CLoadoutItemOptionsPanel::GetItem( void ) const
{ 
	return TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, m_eItemSlot );
}

CClassLoadoutPanel *g_pClassLoadoutPanel = NULL;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CClassLoadoutPanel::CClassLoadoutPanel( vgui::Panel *parent ) 
	: CBaseLoadoutPanel( parent, "class_loadout_panel" )
	, m_pItemOptionPanelKVs( NULL )
{
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED;
	m_iCurrentTeamIndex = TF_TEAM_RED;
	m_iCurrentSlotIndex = -1;
	m_pPlayerModelPanel = NULL;
	m_pSelectionPanel = NULL;
	m_pTauntHintLabel = NULL;
	m_pTauntLabel = NULL;
	m_pTauntCaratLabel = NULL;
	m_pPassiveAttribsLabel = NULL;
	m_pLoadoutPresetPanel = NULL;
	m_pPresetsExplanationPopup = NULL;
	m_pTauntsExplanationPopup = NULL;
	m_pBuildablesButton = NULL;
	
	m_pCharacterLoadoutButton = NULL;
	m_pTauntLoadoutButton = NULL;

	m_bInTauntLoadoutMode = false;

	g_pClassLoadoutPanel = this;

	m_pItemOptionPanel = new CLoadoutItemOptionsPanel( this, "ItemOptionsPanel" );
}

CClassLoadoutPanel::~CClassLoadoutPanel()
{
	if ( m_pItemOptionPanelKVs )
	{
		m_pItemOptionPanelKVs->deleteThis();
		m_pItemOptionPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/ClassLoadoutPanel.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pPlayerModelPanel = dynamic_cast<CTFPlayerModelPanel*>( FindChildByName("classmodelpanel") );
	m_pTauntHintLabel = dynamic_cast<vgui::Label*>( FindChildByName("TauntHintLabel") );
	m_pTauntLabel = dynamic_cast<CExLabel*>( FindChildByName("TauntLabel") );
	m_pTauntCaratLabel = dynamic_cast<CExLabel*>( FindChildByName("TauntCaratLabel") );
	m_pBuildablesButton = dynamic_cast<CExButton*>( FindChildByName("BuildablesButton") );
	m_pCharacterLoadoutButton = dynamic_cast<CExImageButton*>( FindChildByName("CharacterLoadoutButton") );
	m_pTauntLoadoutButton = dynamic_cast<CExImageButton*>( FindChildByName("TauntLoadoutButton") );
	m_pPassiveAttribsLabel = dynamic_cast<CExLabel*>( FindChildByName("PassiveAttribsLabel") );
	m_pLoadoutPresetPanel = dynamic_cast<CLoadoutPresetPanel*>( FindChildByName( "loadout_preset_panel" ) );
	if (m_pLoadoutPresetPanel)
	{
		m_pLoadoutPresetPanel->SetClassLoadoutPanel(this);
	}
	m_pPresetsExplanationPopup = dynamic_cast<CExplanationPopup*>( FindChildByName( "PresetsExplanation" ) );
	m_pTauntsExplanationPopup = dynamic_cast<CExplanationPopup*>( FindChildByName( "TauntsExplanation" ) );
	m_pTopLinePanel = FindChildByName( "TopLine" );
	if ( m_pPassiveAttribsLabel )
	{
		m_pPassiveAttribsLabel->SetMouseInputEnabled( false );
	}

	m_pMouseOverTooltip->SetPositioningStrategy( IPTTP_BOTTOM_SIDE );

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


void CClassLoadoutPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "itemoptionpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pItemOptionPanelKVs )
		{
			m_pItemOptionPanelKVs->deleteThis();
		}
		m_pItemOptionPanelKVs = new KeyValues("itemoptionpanels_kv");
		pItemKV->CopySubkeys( m_pItemOptionPanelKVs );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::PerformLayout( void ) 
{
	BaseClass::PerformLayout();

	// This is disabled by default in res file. IF we turn it on again, uncomment this.
	/*if ( m_pPassiveAttribsLabel )
	{
		m_pPassiveAttribsLabel->SetVisible( !m_bInTauntLoadoutMode );
	}*/

	if ( m_pTauntHintLabel )
	{
		m_pTauntHintLabel->SetVisible( m_bInTauntLoadoutMode );
		
		const char *key = engine->Key_LookupBinding( "taunt" );
		if ( !key )
		{
			key = "< not bound >";
		}
		SetDialogVariable( "taunt", key );
	}

	if ( m_pTauntLabel )
	{
		m_pTauntLabel->SetVisible( m_bInTauntLoadoutMode );
	}
	if ( m_pTauntCaratLabel )
	{
		m_pTauntCaratLabel->SetVisible( m_bInTauntLoadoutMode );
	}
	if ( m_pCharacterLoadoutButton )
	{
		UpdatePageButtonColor( m_pCharacterLoadoutButton, !m_bInTauntLoadoutMode );
	}
	if ( m_pTauntLoadoutButton )
	{
		UpdatePageButtonColor( m_pTauntLoadoutButton, m_bInTauntLoadoutMode );
	}

	FOR_EACH_VEC( m_vecItemOptionButtons, i )
	{
		m_vecItemOptionButtons[i]->SetVisible( false );
	}
	
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		// Viewing a class loadout. Layout the buttons & the class image.
		if ( i >= NUM_ITEM_PANELS_IN_LOADOUT )
		{
			m_pItemModelPanels[i]->SetVisible( false );
			continue;
		}

		int iButtonPos = 0;
		if ( m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
		{
			iButtonPos = g_VisibleLoadoutSlotsPerClass[m_iCurrentClassIndex]->m_iPos[i];
		}

		bool bIsVisible = false;
		if ( iButtonPos > 0 )
		{
			bIsVisible = m_bInTauntLoadoutMode ? IsTauntPanelPosition( iButtonPos ) : !IsTauntPanelPosition( iButtonPos );
		}
		m_pItemModelPanels[i]->SetVisible( bIsVisible );

		if ( bIsVisible )
		{
			if ( m_bInTauntLoadoutMode )
			{
				iButtonPos -= g_VisibleLoadoutSlotsPerClass[m_iCurrentClassIndex]->m_iPos[LOADOUT_POSITION_TAUNT];
			}
			else
			{
				iButtonPos--;
			}
			

			m_pItemModelPanels[i]->SetNoItemText( ItemSystem()->GetItemSchema()->GetLoadoutStringsForDisplay( EEquipType_t::EQUIP_TYPE_CLASS )[i] );

			int iCenter = GetWide() * 0.5;
			int iColumnHeight = 4;
			int iColumn = iButtonPos / iColumnHeight;
			int iYButtonPos = iButtonPos % iColumnHeight;
			
			int iOffset = iColumn == 0 ? m_iItemXPosOffcenterA : m_iItemXPosOffcenterB + ((iColumn - 1) * 200);
			int	iXPos = iCenter + iOffset;
			int	iYPos = m_iItemYPos + (m_iItemYDelta * iYButtonPos);
			m_pItemModelPanels[i]->SetPos( iXPos, iYPos );

			// Update position and visibility of the item option buttons
			if ( i < m_vecItemOptionButtons.Count() )
			{
				// Place the button just inside the item model panel
				CExButton* pItemOptionsPanel = m_vecItemOptionButtons[iButtonPos];
				int iButtonWide = m_pItemModelPanels[i]->GetWide();
				int iMyWide = pItemOptionsPanel->GetWide();
				int iOptionsXPos = iColumn == 0 
								 ? iXPos + iButtonWide - iMyWide
								 : iXPos;
				pItemOptionsPanel->SetPos( iOptionsXPos, iYPos );

				CEconItemView *pItemData = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, i );
				// Enable or disable the item options button for this item model panel
				pItemOptionsPanel->SetVisible( !m_bInTauntLoadoutMode && AnyOptionsAvailableForItem( pItemData ) );
				pItemOptionsPanel->SetCommand( CFmtStr( "options%d", i ) );
			}
		}

		m_pItemModelPanels[ i ]->SetSelected( false );
	}

	if ( m_pLoadoutPresetPanel )
	{
		m_pLoadoutPresetPanel->SetPos( ( ScreenWidth() - m_pLoadoutPresetPanel->GetWide() ) / 2, m_iItemYPos );
	}

	LinkModelPanelControllerNavigation( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// See if the preset control uses this key
	if( m_pLoadoutPresetPanel->HandlePresetKeyPressed( code ) )
	{
		return;
	}

	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	if (nButtonCode == KEY_XBUTTON_LEFT || 
		nButtonCode == KEY_XSTICK1_LEFT ||
		nButtonCode == KEY_XSTICK2_LEFT ||
		nButtonCode == STEAMCONTROLLER_DPAD_LEFT ||
		code == KEY_LEFT ||
		nButtonCode == KEY_XBUTTON_RIGHT || 
		nButtonCode == KEY_XSTICK1_RIGHT ||
		nButtonCode == KEY_XSTICK2_RIGHT ||
		nButtonCode == STEAMCONTROLLER_DPAD_RIGHT ||
		code == KEY_RIGHT ||
		nButtonCode == KEY_XBUTTON_UP || 
		nButtonCode == KEY_XSTICK1_UP ||
		nButtonCode == KEY_XSTICK2_UP ||
		nButtonCode == STEAMCONTROLLER_DPAD_UP ||
		code == KEY_UP ||
		nButtonCode == KEY_XBUTTON_DOWN || 
		nButtonCode == KEY_XSTICK1_DOWN ||
		nButtonCode == KEY_XSTICK2_DOWN ||
		nButtonCode == STEAMCONTROLLER_DPAD_DOWN ||
		code == KEY_DOWN )
	{
		// just eat all navigation keys so we don't 
		// end up with undesirable navigation behavior bubbling from 
		// one item model panel to another
	}
	else if( nButtonCode == KEY_XBUTTON_A || code == KEY_ENTER || nButtonCode == STEAMCONTROLLER_A )
	{
		// show the current loadout slot
		int nSelected = GetFirstSelectedItemIndex( true );
		if( nSelected != -1 )
		{
			OnCommand( VarArgs("change%d", nSelected ) );
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnNavigateTo( const char* panelName )
{
	CItemModelPanel *pChild = dynamic_cast<CItemModelPanel *>( FindChildByName( panelName ) );
	if( !pChild )
		return;

	pChild->SetSelected( true );
	SetBorderForItem( pChild, false );
	pChild->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnNavigateFrom( const char* panelName )
{
	CItemModelPanel *pChild = dynamic_cast<CItemModelPanel *>( FindChildByName( panelName ) );
	if( !pChild )
		return;

	pChild->SetSelected( false );
	SetBorderForItem( pChild, false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnShowPanel( bool bVisible, bool bReturningFromArmory )
{
	if ( bVisible )
	{
		// always start in character loadout page
		SetLoadoutPage( CHARACTER_LOADOUT_PAGE );

		if ( m_pSelectionPanel )
		{
			m_pSelectionPanel->SetVisible( false );
			m_pSelectionPanel->MarkForDeletion();
			m_pSelectionPanel = NULL;
		}

		m_iCurrentSlotIndex = TF_WPN_TYPE_PRIMARY;
		if( m_pItemModelPanels.Count() && m_pItemModelPanels[0] )
		{
			m_pItemModelPanels[0]->SetSelected( true );
			SetBorderForItem( m_pItemModelPanels[0], false );
		}

		m_bLoadoutHasChanged = false;

		if ( tf_show_preset_explanation_in_class_loadout.GetBool() && m_pPresetsExplanationPopup )
		{
			m_pPresetsExplanationPopup->Popup();
			tf_show_preset_explanation_in_class_loadout.SetValue( 0 );
		}
		else if ( tf_show_taunt_explanation_in_class_loadout.GetBool() && m_pTauntsExplanationPopup )
		{
			m_pTauntsExplanationPopup->Popup();
			tf_show_taunt_explanation_in_class_loadout.SetValue( 0 );
		}

		ClearItemOptionsMenu();
	}
	else
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->ClearCarriedItems();
		}
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::PostShowPanel( bool bVisible )
{
	if ( bVisible )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->SetVisible( true );
		}

		if ( m_pBuildablesButton )
		{
			m_pBuildablesButton->SetVisible( m_iCurrentClassIndex == TF_CLASS_ENGINEER );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::SetClass( int iClass )
{
	m_iCurrentClassIndex = iClass;

	if ( m_pLoadoutPresetPanel )
	{
		m_pLoadoutPresetPanel->SetClass( m_iCurrentClassIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::SetTeam( int iTeam )
{
	Assert( IsValidTFTeam( iTeam ) );
	m_iCurrentTeamIndex = iTeam;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CClassLoadoutPanel::GetNumRelevantSlots() const
{
	return m_pItemModelPanels.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CClassLoadoutPanel::GetItemInSlot( int iSlot )
{
	if( iSlot >= 0 && iSlot < m_pItemModelPanels.Count() )
	{
		return m_pItemModelPanels[iSlot]->GetItem();
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::FireGameEvent( IGameEvent *event )
{
	// If we're not visible, ignore all events
	if ( !IsVisible() )
		return;

	BaseClass::FireGameEvent( event );

	// We need to update ourselves after the base has done it, so our item models have been updated
	const char *type = event->GetName();
	if ( Q_strcmp( "inventory_updated", type ) == 0 )
	{
		if ( m_pPlayerModelPanel )
		{
			m_pPlayerModelPanel->HoldItemInSlot( m_iCurrentSlotIndex );
		}
	}
}

void CClassLoadoutPanel::AddNewItemPanel( int iPanelIndex )
{
	BaseClass::AddNewItemPanel( iPanelIndex );

	m_vecItemOptionButtons[ m_vecItemOptionButtons.AddToTail() ] = new CExButton( this,
																				  CFmtStr( "item_options_button%d", iPanelIndex ),
																				  "+",
																				  this,
																				  CFmtStr( "options%d", iPanelIndex ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::UpdateModelPanels( void )
{
	// We're showing the loadout for a specific class.
	TFPlayerClassData_t *pData = GetPlayerClassData( m_iCurrentClassIndex );
	if ( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->ClearCarriedItems();
		m_pPlayerModelPanel->SetToPlayerClass( m_iCurrentClassIndex );
		m_pPlayerModelPanel->SetTeam( m_iCurrentTeamIndex );
	}

	// For now, fill them out with the local player's currently wielded items
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, i );
		m_pItemModelPanels[i]->SetItem( pItemData );
		m_pItemModelPanels[i]->SetShowQuantity( true );
		m_pItemModelPanels[i]->SetSelected( false );
		SetBorderForItem( m_pItemModelPanels[i], false );

		if ( m_pPlayerModelPanel && pItemData && pItemData->IsValid() )
		{
			m_pPlayerModelPanel->AddCarriedItem( pItemData );
		}
	}

	if ( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->HoldItemInSlot( m_iCurrentSlotIndex );
	}

	SetDialogVariable( "loadoutclass", g_pVGuiLocalize->Find( pData->m_szLocalizableName ) );

	UpdatePassiveAttributes();

	// Now layout again to position our item buttons 
	InvalidateLayout();

	if ( m_pItemOptionPanel->IsVisible() )
	{
		m_pItemOptionPanel->UpdateItemOptionsUI();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnItemPanelMouseReleased( vgui::Panel *panel )
{
	CItemModelPanel *pItemPanel = dynamic_cast < CItemModelPanel * > ( panel );

	if ( pItemPanel && IsVisible() )
	{
		for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
		{
			if ( m_pItemModelPanels[i] == pItemPanel  )
			{
				OnCommand( VarArgs("change%d", i) );
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnSelectionReturned( KeyValues *data )
{
	if ( data )
	{
		uint64 ulIndex = data->GetUint64( "itemindex", INVALID_ITEM_ID );

		// ulIndex implies do nothing (escape key)
		if ( ulIndex != 0 )
		{
			TFInventoryManager()->EquipItemInLoadout( m_iCurrentClassIndex, m_iCurrentSlotIndex, ulIndex );

			m_bLoadoutHasChanged = true;

			UpdateModelPanels();

			// Send the preset panel a msg so it can save the change
			KeyValues *pLoadoutChangedMsg = new KeyValues( "LoadoutChanged" );
			pLoadoutChangedMsg->SetInt( "slot", m_iCurrentSlotIndex );
			pLoadoutChangedMsg->SetUint64( "itemid", ulIndex );
			PostMessage( m_pLoadoutPresetPanel, pLoadoutChangedMsg );
		}
	}

	PostMessage( GetParent(), new KeyValues("SelectionEnded") );

	// It'll have deleted itself, so we don't need to clean it up
	m_pSelectionPanel = NULL;
	OnCancelSelection();

	// find the selected item and give it the focus
	CItemModelPanel *pSelection = GetFirstSelectedItemModelPanel( true );
	if( !pSelection )
	{
		m_pItemModelPanels[0]->SetSelected( true );
		pSelection = m_pItemModelPanels[0];
	}

	pSelection->RequestFocus();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnCancelSelection( void )
{
	if ( m_pSelectionPanel )
	{
		m_pSelectionPanel->SetVisible( false );
		m_pSelectionPanel->MarkForDeletion();
		m_pSelectionPanel = NULL;
	}

	if ( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::RespawnPlayer()
{
	if ( tf_respawn_on_loadoutchanges.GetBool() )
	{
		// Tell the GC to tell server that we should respawn if we're in a respawn room
		GCSDK::CGCMsg< MsgGCEmpty_t > msg( k_EMsgGCRespawnPostLoadoutChange );
		GCClientSystem()->BSendMessage( msg );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply KVs to the item option buttons
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::ApplyKVsToItemPanels( void )
{
	BaseClass::ApplyKVsToItemPanels();

	if ( m_pItemOptionPanelKVs )
	{
		for ( int i = 0; i < m_vecItemOptionButtons.Count(); i++ )
		{
			m_vecItemOptionButtons[i]->ApplySettings( m_pItemOptionPanelKVs );
			m_vecItemOptionButtons[i]->InvalidateLayout();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnClosing( void )
{
	if ( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->ClearCarriedItems();
	}

	if ( m_bLoadoutHasChanged )
	{
		RespawnPlayer();

		m_bLoadoutHasChanged = false;
	}
}

extern const char *g_szItemBorders[AE_MAX_TYPES][5];
extern ConVar cl_showbackpackrarities;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver )
{
	if ( !pItemPanel )
		return;

	const char *pszBorder = NULL;

	if ( pItemPanel->IsGreyedOut() )
	{
		pszBorder = "EconItemBorder";
	}
	else
	{
		int iRarity = 0;
		if ( pItemPanel->HasItem() && cl_showbackpackrarities.GetBool() )
		{
			iRarity = pItemPanel->GetItem()->GetItemQuality();

			uint8 nRarity = pItemPanel->GetItem()->GetRarity();
			if ( ( nRarity != k_unItemRarity_Any ) && ( iRarity != AE_SELFMADE ) && ( iRarity != AE_UNUSUAL ) )
			{
				// translate this quality to rarity
				iRarity = nRarity + AE_RARITY_DEFAULT;
			}

			if ( iRarity > 0 )
			{
				if ( bMouseOver || pItemPanel->IsSelected() )
				{
					pszBorder = g_szItemBorders[iRarity][1];
				}
				else
				{
					pszBorder = g_szItemBorders[iRarity][0];
				}
			}
		}
		
		
		if ( iRarity == 0 )
		{
			if ( bMouseOver || pItemPanel->IsSelected() )
			{
				pszBorder = "LoadoutItemMouseOverBorder";
			}
			else
			{
				pszBorder = "EconItemBorder";
			}
		}
	}

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	pItemPanel->SetBorder( pScheme->GetBorder( pszBorder ) );
}

//-----------------------------------------------------------------------------
// Purpose: Clear the item options menu and reset the button that summoned it
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::ClearItemOptionsMenu( void )
{
	SetOptionsButtonText( m_pItemOptionPanel->GetItemSlot(), "+" );
	m_pItemOptionPanel->SetItemSlot( LOADOUT_POSITION_INVALID, m_iCurrentClassIndex );
	m_pItemOptionPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: Safely set the text for a button
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::SetOptionsButtonText( int nIndex, const char* pszText )
{
	if ( nIndex >= 0 && nIndex < m_vecItemOptionButtons.Count() )
	{
		m_vecItemOptionButtons[ m_pItemOptionPanel->GetItemSlot() ]->SetText( pszText );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return if the passed in item has any options
//-----------------------------------------------------------------------------
bool CClassLoadoutPanel::AnyOptionsAvailableForItem( const CEconItemView *pItem )
{
	if ( !pItem )
		return false;
		
	// Styles!
	if ( pItem->GetStaticData()->GetNumSelectableStyles() > 1 )
		return true;

	// Unusual particle effect! For Cosmetics only
	static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
	if ( pItem->FindAttribute( pAttrDef_AttachParticleEffect ) && pItem->GetItemDefinition()->GetLoadoutSlot( 0 ) >= LOADOUT_POSITION_HEAD )
		return true;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::SetLoadoutPage( classloadoutpage_t loadoutPage )
{
	ClearItemOptionsMenu();
	switch ( loadoutPage )
	{
		case CHARACTER_LOADOUT_PAGE:
		{
			m_bInTauntLoadoutMode = false;
		}
		break;
		case TAUNT_LOADOUT_PAGE:
		{
			m_bInTauntLoadoutMode = true;
		}
		break;
		default:
		{
			// Unhandled loadout page
			Assert( 0 );
		}
	}
	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "characterloadout" ) )
	{
		SetLoadoutPage( CHARACTER_LOADOUT_PAGE );
		return;
	}
	else if ( FStrEq( command, "tauntloadout" ) )
	{
		SetLoadoutPage( TAUNT_LOADOUT_PAGE );
		return;
	}
	else if ( !V_strnicmp( command, "change", 6 ) )
	{
		const char *pszNum = command+6;
		if ( pszNum && pszNum[0] )
		{
			int iSlot = atoi(pszNum);
			if ( iSlot >= 0 && iSlot < CLASS_LOADOUT_POSITION_COUNT && m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
			{
				if ( m_iCurrentSlotIndex != iSlot )
				{
					m_iCurrentSlotIndex = iSlot;
				}

				// Create the selection screen. It removes itself on close.
				m_pSelectionPanel = new CEquipSlotItemSelectionPanel( this, m_iCurrentClassIndex, iSlot );
				m_pSelectionPanel->ShowPanel( 0, true );

				if ( m_pPlayerModelPanel )
				{
					m_pPlayerModelPanel->SetVisible( false );
				}

				ClearItemOptionsMenu();

				PostMessage( GetParent(), new KeyValues("SelectionStarted") );
			}
		}

		return;
	}
	else if ( !V_strnicmp( command, "options", 7 ) )
	{
		const char *pszNum = command + 7;
		if( pszNum && pszNum[0] )
		{
			int iSlot = atoi( pszNum );
			//iSlot = g_VisibleLoadoutSlotsPerClass[m_iCurrentClassIndex]->m_iPos[iSlot - 1];
			if ( iSlot >= 0 && iSlot < m_vecItemOptionButtons.Count() && m_iCurrentClassIndex != TF_CLASS_UNDEFINED )
			{
				// Change the button we're coming from to be a "+"
				SetOptionsButtonText( m_pItemOptionPanel->GetItemSlot(), "+" );

				// Update the current slot index for callback from the setstyle button.
				// It will send us a message to change the item the player model is holding
				// and we need this to be updated for that.
				m_iCurrentSlotIndex = iSlot;
				

				// Did they just toggle?
				if ( m_pItemOptionPanel->GetItemSlot() == iSlot )
				{
					m_pItemOptionPanel->SetVisible( !m_pItemOptionPanel->IsVisible() );
				}
				else
				{
					// Set the options panel to have the data for this slot
					m_pItemOptionPanel->SetItemSlot( (loadout_positions_t)iSlot, m_iCurrentClassIndex );
					m_pItemOptionPanel->SetVisible( true );
					// Figure out if this is on the left or right
					int iColumnHeight = 4;
					int iColumn = iSlot / iColumnHeight;
					PinCorner_e myCornerToPin = iColumn == 0 ? PIN_TOPLEFT : PIN_TOPRIGHT;
					PinCorner_e siblingCornerPinTo = iColumn == 0 ? PIN_TOPRIGHT : PIN_TOPLEFT;
					// Pin to the appropriate side
					int iButtonPos = g_VisibleLoadoutSlotsPerClass[m_iCurrentClassIndex]->m_iPos[ iSlot ] - 1;
					m_pItemOptionPanel->PinToSibling( m_vecItemOptionButtons[ iButtonPos ]->GetName(), myCornerToPin, siblingCornerPinTo );
					m_pItemOptionPanel->UpdateItemOptionsUI();
				}

				// Change the button we're going to to be "-" if we're visible, "+" if we're not
				SetOptionsButtonText( m_pItemOptionPanel->GetItemSlot(), m_pItemOptionPanel->IsVisible() ? "-" : "+" );
			}
			return;
		}
	}
	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
void CClassLoadoutPanel::OnMessage( const KeyValues* pParams, vgui::VPANEL hFromPanel )
{
	BaseClass::OnMessage( pParams, hFromPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct passive_attrib_to_print_t
{
	const CEconItemAttributeDefinition *m_pAttrDef;
	attrib_value_t m_value;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAttributeIterator_AddPassiveAttribsToPassiveList : public CEconItemSpecificAttributeIterator
{
public:
	CAttributeIterator_AddPassiveAttribsToPassiveList( CUtlVector<passive_attrib_to_print_t> *pList, bool bForceAdd )
		: m_pList( pList )
		, m_bForceAdd( bForceAdd )
	{
		Assert( m_pList );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
	{
		Assert( pAttrDef );

		if ( pAttrDef->IsHidden() )
			return true;

		if ( !m_bForceAdd )
		{
			const char *pDesc = pAttrDef->GetArmoryDescString();
			if ( !pDesc || !pDesc[0] )
				return true;

			// If we have the "on_wearer" key, we're a passive attribute
			if ( !Q_stristr(pDesc, "on_wearer") )
				return true;
		}

		// Now see if we're already in the list
		FOR_EACH_VEC( (*m_pList), i )
		{
			passive_attrib_to_print_t& passiveAttr = (*m_pList)[i];

			Assert( passiveAttr.m_pAttrDef );

			// We match if our class is the same -- this is a case-sensitive compare!
			if ( Q_strcmp( passiveAttr.m_pAttrDef->GetAttributeClass(), pAttrDef->GetAttributeClass() ) )
				continue;

			// We've found a matching attribute. Collate our values and stomp over the earlier value.
			passiveAttr.m_value = CollateAttributeValues( passiveAttr.m_pAttrDef, passiveAttr.m_value, pAttrDef, value );

			return true;
		}

		// We didn't find it. Add it to the list.
		passive_attrib_to_print_t newPassiveAttr = { pAttrDef, value };
		m_pList->AddToTail( newPassiveAttr );

		return true;
	}

	// Other types are ignored.

private:
	CUtlVector<passive_attrib_to_print_t> *m_pList;
	bool m_bForceAdd;
};

//-----------------------------------------------------------------------------
void CClassLoadoutPanel::UpdatePassiveAttributes( void )
{
	if ( !m_pPassiveAttribsLabel )
		return;

	// We build a list of attributes & associated values by looping through all equipped items.
	// This way we can identify & collate attributes based on the same definition index.
	CUtlVector<passive_attrib_to_print_t> vecAttribsToPrint;

	// Loop through all equipped items
	for ( int i = 0; i < m_pItemModelPanels.Count(); i++ )
	{
		CEconItemView *pItemData = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, i );
		if ( pItemData && pItemData->IsValid() )
		{
			CAttributeIterator_AddPassiveAttribsToPassiveList attrItPassives( &vecAttribsToPrint, false );
			pItemData->IterateAttributes( &attrItPassives );
		}
	}

	// Then add any set bonuses
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID localSteamID = steamapicontext->SteamUser()->GetSteamID();
		CUtlVector<const CEconItemSetDefinition *> pActiveSets;
		TFInventoryManager()->GetActiveSets( &pActiveSets, localSteamID, m_iCurrentClassIndex );

		FOR_EACH_VEC( pActiveSets, set )
		{
			CAttributeIterator_AddPassiveAttribsToPassiveList attrItSetPassives( &vecAttribsToPrint, true );
			pActiveSets[set]->IterateAttributes( &attrItSetPassives );
		}
	}

	// Now build the text
	wchar_t wszPassiveDesc[4096];
	wszPassiveDesc[0] = '\0';
	m_pPassiveAttribsLabel->GetTextImage()->ClearColorChangeStream();

	wchar_t *pHeader = g_pVGuiLocalize->Find( "#TF_PassiveAttribs" );
	if ( pHeader )
	{
		V_wcscpy_safe( wszPassiveDesc, pHeader );
		V_wcscat_safe( wszPassiveDesc, L"\n" );
	}

	if ( vecAttribsToPrint.Count() )
	{
		FOR_EACH_VEC( vecAttribsToPrint, i )
		{
			CEconAttributeDescription AttrDesc( GLocalizationProvider(), vecAttribsToPrint[i].m_pAttrDef, vecAttribsToPrint[i].m_value );
			AddAttribPassiveText( AttrDesc, wszPassiveDesc, ARRAYSIZE(wszPassiveDesc) );
		}
	}
	else
	{
		vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		Color col = pScheme->GetColor( GetColorNameForAttribColor( ATTRIB_COL_NEUTRAL ), Color(255,255,255,255) );
		m_pPassiveAttribsLabel->GetTextImage()->AddColorChange( col, Q_wcslen( wszPassiveDesc ) );

		wchar_t *pNone = g_pVGuiLocalize->Find( "#TF_PassiveAttribs_None" );
		if ( pNone )
		{
			V_wcscat_safe( wszPassiveDesc, pNone );
		}
	}

	m_pPassiveAttribsLabel->SetText( wszPassiveDesc );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::AddAttribPassiveText( const CEconAttributeDescription& AttrDesc, INOUT_Z_CAP(iNumPassiveChars) wchar_t *out_wszPassiveDesc, int iNumPassiveChars )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	Assert( pScheme );

	if ( !AttrDesc.GetDescription().IsEmpty() )
	{
		// Insert the color change at the current position
		Color col = pScheme->GetColor( GetColorNameForAttribColor( AttrDesc.GetDefaultColor() ), Color(255,255,255,255) );
		m_pPassiveAttribsLabel->GetTextImage()->AddColorChange( col, Q_wcslen( out_wszPassiveDesc ) );

		// Now append the text of the attribute
		V_wcsncat( out_wszPassiveDesc, AttrDesc.GetDescription().Get(), iNumPassiveChars );
		V_wcsncat( out_wszPassiveDesc, L"\n", iNumPassiveChars );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CClassLoadoutPanel::UpdatePageButtonColor( CExImageButton *pPageButton, bool bIsActive )
{
	if ( pPageButton )
	{
		int iLoaded = bIsActive ? LOADED : NOTLOADED;
		pPageButton->SetDefaultColor( m_aDefaultColors[iLoaded][FG][DEFAULT], m_aDefaultColors[iLoaded][BG][DEFAULT] );
		pPageButton->SetArmedColor( m_aDefaultColors[iLoaded][FG][ARMED], m_aDefaultColors[iLoaded][BG][ARMED] );
		pPageButton->SetDepressedColor( m_aDefaultColors[iLoaded][FG][DEPRESSED], m_aDefaultColors[iLoaded][BG][DEPRESSED] );
	}
}
