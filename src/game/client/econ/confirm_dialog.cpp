//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#include "confirm_dialog.h"

#include "ienginevgui.h"
#include "econ_controls.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/CheckButton.h"
#include "econ_ui.h"
#include "store/store_panel.h"
#ifdef TF_CLIENT_DLL
#include "tf_playerpanel.h"
#include "item_ad_panel.h"
#endif // TF_CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

/*static const wchar_t* GetSCGlyph( const char* action )
{
	auto origin = g_pInputSystem->GetSteamControllerActionOrigin( action, GAME_ACTION_SET_FPSCONTROLS );
	return g_pInputSystem->GetSteamControllerFontCharacterForActionOrigin( origin );
}*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmDialog::CConfirmDialog( vgui::Panel *parent )
:	BaseClass( parent, "ConfirmDialog" ),
	m_pCancelButton( NULL ),
	m_pConfirmButton( NULL ),
	m_pIcon( NULL )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( GetResFile(), "GAME" );

	SetBorder( pScheme->GetBorder("EconItemBorder") );

	// Cache off button ptrs
	m_pConfirmButton = dynamic_cast< CExButton* >( FindChildByName( "ConfirmButton" ) );
	m_pCancelButton = dynamic_cast< CExButton* >( FindChildByName( "CancelButton" ) );
	m_pIcon = dynamic_cast< vgui::ImagePanel* >( FindChildByName( "Icon" ) );

	SetDialogVariable( "text", GetText() );

	auto confirmHint = dynamic_cast<CSCHintIcon*>( FindChildByName( "ConfirmButtonHintIcon", true ) );
	auto cancelHint = dynamic_cast<CSCHintIcon*>( FindChildByName( "CancelButtonHintIcon", true ) );

	if ( confirmHint )
		confirmHint->SetVisible( ::input->IsSteamControllerActive() );
	if ( cancelHint )
		cancelHint->SetVisible( ::input->IsSteamControllerActive() );

	if ( ::input->IsSteamControllerActive() )
	{
		if ( confirmHint )
		{
			confirmHint->SetAction( GetConfirmActionName(), GetActionSet() );
		}

		if ( cancelHint )
		{
			confirmHint->SetAction( GetCancelActionName(), GetActionSet() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	Label* pBody = FindControl< Label >( "ExplanationLabel" );
	if ( pBody )
	{
		pBody->SizeToContents();
		SetTall( pBody->GetYPos() + pBody->GetTall() + m_pConfirmButton->GetTall() + YRES(15) + YRES( 15 ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::Show( bool bMakePopup )
{
	SetVisible( true );
	if ( bMakePopup )
	{
		MakePopup();
	}
	MoveToFront();
	SetKeyBoardInputEnabled( true );

	InvalidateLayout( true, true );

	if ( ::input->IsSteamControllerActive() )
	{
		SetMouseInputEnabled( false );
	}
	else
	{
		SetMouseInputEnabled( true );
	}

	TFModalStack()->PushModal( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::SetIconImage( const char *pszIcon )
{
	Assert( m_pIcon );
	if ( m_pIcon )
	{
		m_pIcon->SetImage( pszIcon );
		m_pIcon->SetVisible( ( pszIcon ? true : false ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnCommand( const char *command )
{
	if ( !Q_strnicmp( command, "cancel", 6 ) )
	{
		FinishUp();
		PostMessage( GetParent(), new KeyValues( "ConfirmDlgResult", "confirmed", 0 ) );
	}
	else if ( !Q_strnicmp( command, "confirm", 7 ) )
	{
		FinishUp();
		PostMessage( GetParent(), new KeyValues( "ConfirmDlgResult", "confirmed", 1 ) );
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
void CConfirmDialog::OnKeyCodeTyped( vgui::KeyCode code )
{
	if( code == KEY_ESCAPE )
	{
		OnCommand( "cancel" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

///-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnKeyCodePressed( vgui::KeyCode code )
{
	ButtonCode_t nButtonCode = GetBaseButtonCode( code );

	// We map the voting action buttons to the pseudo-buttons F1/F2 so that players can use them to interact with dialogs on the fly
	if( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_F2 || nButtonCode == STEAMCONTROLLER_B )
	{
		OnCommand( "cancel" );
	}
	else if ( nButtonCode == KEY_ENTER || nButtonCode == KEY_SPACE || nButtonCode == KEY_XBUTTON_A || nButtonCode == STEAMCONTROLLER_F1  || nButtonCode == STEAMCONTROLLER_A )
	{
		OnCommand( "confirm" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CConfirmDialog::GetResFile()
{
	if ( ::input->IsSteamControllerActive() )
	{
		return "Resource/UI/econ/ConfirmDialog_SC.res";
	}
	else
	{
		return "Resource/UI/econ/ConfirmDialog.res";
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hide the panel, mark for deletion, remove from modal stack.
//-----------------------------------------------------------------------------
void CConfirmDialog::FinishUp()
{
	SetVisible( false );
	TFModalStack()->PopModal( this );
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDialog::OnSizeChanged( int nNewWide, int nNewTall )
{
	int nX, nY;

	m_pConfirmButton = dynamic_cast< CExButton* >( FindChildByName( "ConfirmButton" ) );
	m_pCancelButton = dynamic_cast< CExButton* >( FindChildByName( "CancelButton" ) );

	// Shift buttons up
	if ( m_pCancelButton )
	{
		m_pCancelButton->GetPos( nX, nY );
		m_pCancelButton->SetPos( nX, nNewTall - m_pCancelButton->GetTall() - YRES(15) );
	}

	if ( m_pConfirmButton )
	{
		m_pConfirmButton->GetPos( nX, nY );
		m_pConfirmButton->SetPos( nX, nNewTall - m_pConfirmButton->GetTall() - YRES(15) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::CTFGenericConfirmDialog( const char *pTitle, const char *pTextKey,
												  const char *pConfirmBtnText, const char *pCancelBtnText,
												  GenericConfirmDialogCallback callback, vgui::Panel *pParent ) 
:	BaseClass( pParent ),
	m_pTextKey( pTextKey )
{
	CommonInit( pTitle, pConfirmBtnText, pCancelBtnText, callback, pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::CTFGenericConfirmDialog( const char *pTitle, const wchar_t *pText,
												  const char *pConfirmBtnText, const char *pCancelBtnText,
												  GenericConfirmDialogCallback callback, vgui::Panel *pParent )
:	BaseClass( pParent ),
	m_pTextKey( NULL )
{
	CommonInit( pTitle, pConfirmBtnText, pCancelBtnText, callback, pParent );

	V_wcsncpy( m_wszBuffer, pText, sizeof( m_wszBuffer ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::CommonInit( const char *pTitle, const char *pConfirmBtnText, const char *pCancelBtnText,
				 GenericConfirmDialogCallback callback, vgui::Panel *pParent )
{
	if ( pParent == NULL )
	{
		vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
		SetScheme(scheme);
		SetProportional( true );
	}

	m_pTitle = pTitle;
	m_pConfirmBtnText = pConfirmBtnText;
	m_pCancelBtnText = pCancelBtnText;
	m_pCallback = callback;
	m_pContext = NULL;
	m_pKeyValues = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog::~CTFGenericConfirmDialog()
{
	if ( m_pKeyValues )
	{
		m_pKeyValues->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const wchar_t *CTFGenericConfirmDialog::GetText()
{
	if ( m_pTextKey )
	{
		g_pVGuiLocalize->ConstructString_safe( m_wszBuffer, m_pTextKey, m_pKeyValues );
		return m_wszBuffer;
	}

	return m_wszBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pConfirmButton && m_pConfirmBtnText )
	{
		m_pConfirmButton->SetText( m_pConfirmBtnText );
	}

	if ( m_pCancelButton && m_pCancelBtnText )
	{
		m_pCancelButton->SetText (m_pCancelBtnText );
	}

	SetXToRed( m_pConfirmButton );
	SetXToRed( m_pCancelButton );

	CExLabel *pTitle = dynamic_cast< CExLabel* >( FindChildByName( "TitleLabel" ) );
	if ( pTitle )
	{
		pTitle->SetText( m_pTitle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	// Center it, keeping requested size
	int x, y, ww, wt, wide, tall;
	vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
	GetSize(wide, tall);
	SetPos(x + ((ww - wide) / 2), y + ((wt - tall) / 2));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::OnCommand( const char *command )
{
	bool bFinishUp = false;
	bool bConfirmed = false;

	if ( !Q_strnicmp( command, "cancel", 6 ) )
	{
		bConfirmed = false;
		bFinishUp = true;
	}
	else if ( !Q_strnicmp( command, "confirm", 7 ) )
	{
		bConfirmed = true;
		bFinishUp = true;
	}

	if ( bFinishUp )
	{
		FinishUp();
		if ( m_pCallback )
		{
			m_pCallback( bConfirmed, m_pContext );
		}
		return;
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::SetStringTokens( KeyValues *pKeyValues )
{
	if ( m_pKeyValues != NULL )
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = pKeyValues->MakeCopy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::AddStringToken( const char* pToken, const wchar_t* pValue )
{
	if ( m_pKeyValues == NULL )
	{
		m_pKeyValues = new KeyValues( "GenericConfirmDialog" );
	}
	m_pKeyValues->SetWString( pToken, pValue );
	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmDialog::SetContext( void *pContext )
{
	m_pContext = pContext;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmOptOutDialog::CTFGenericConfirmOptOutDialog( const char *pTitle, 
															  const char *pText, 
															  const char *pConfirmBtnText, 
															  const char *pCancelBtnText,
															  const char *pOptOutText,
															  const char *pOptOutConVarName, 
															  GenericConfirmDialogCallback callback, 
															  vgui::Panel *parent ) : 
															  CTFGenericConfirmDialog( pTitle, pText, pConfirmBtnText, pCancelBtnText, callback, parent )
{
	m_optOutText = pOptOutText;
	m_optOutCheckbox = NULL;
	m_optOutConVarName = pOptOutConVarName;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmOptOutDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_optOutCheckbox = dynamic_cast< vgui::CheckButton * >( FindChildByName( "OptOutCheckbox" ) );

	if ( m_optOutCheckbox && m_optOutText )
	{
		m_optOutCheckbox->SetMouseInputEnabled( true );
		m_optOutCheckbox->SetText( m_optOutText );

		// center horizontally
		vgui::Panel *parent = m_optOutCheckbox->GetParent();
		if ( parent )
		{
			float parentWidth = parent->GetWide();

			int checkBoxWidth, checkBoxHeight;
			m_optOutCheckbox->GetContentSize( checkBoxWidth, checkBoxHeight );

			// fudge in checkbox width
			checkBoxWidth += 34.0f;

			int checkX, checkY;
			m_optOutCheckbox->GetPos( checkX, checkY );

			m_optOutCheckbox->SetPos( ( parentWidth - checkBoxWidth ) / 2.0f, checkY );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFGenericConfirmOptOutDialog::GetResFile()
{
  return "Resource/UI/econ/ConfirmDialogOptOut.res";
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFGenericConfirmOptOutDialog::OnButtonChecked( KeyValues *pData )
{
	UIConVarRef var( g_pVGui->GetVGUIEngine(), m_optOutConVarName );
	if ( !var.IsValid() )
		return;

	if ( !m_optOutCheckbox )
		return;

	var.SetValue( m_optOutCheckbox->IsSelected() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFUpgradeBoxDialog::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "upgrade" ) )
	{
		FinishUp();

		// Open the store, and show the upgrade advice
		EconUI()->CloseEconUI();
		EconUI()->OpenStorePanel( STOREPANEL_SHOW_UPGRADESTEPS, false );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFGenericConfirmDialog *ShowConfirmDialog( const char *pTitle, const char *pText, const char *pConfirmBtnText, const char *pCancelBtnText, GenericConfirmDialogCallback callback,
										    vgui::Panel *parent/*=NULL*/, void *pContext/*=NULL*/, const char *pSound/*=NULL*/ )
{
	CTFGenericConfirmDialog *pDialog = vgui::SETUP_PANEL(
		new CTFGenericConfirmDialog(
			pTitle, pText,
			pConfirmBtnText, pCancelBtnText,
			callback, parent
		)
	);

	if ( pDialog )
	{
		pDialog->Show();

		// Play a sound, if one was supplied.
		if ( pSound && pSound[0] )
		{
			vgui::surface()->PlaySound( pSound );
		}
	}

	if ( pContext )
	{
		pDialog->SetContext( pContext );
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pTitle, const char *pText, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent, void *pContext )
{
	return	ShowMessageBox( pTitle, pText, NULL, pConfirmBtnText, callback, parent, pContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pTitle, const wchar_t *pText, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent , void *pContext)
{
	CTFMessageBoxDialog *pDialog = vgui::SETUP_PANEL(
		new CTFMessageBoxDialog(
			pTitle, pText,
			pConfirmBtnText,
			callback, parent
		)
	);

	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowMessageBox( const char *pTitle, const char *pText, KeyValues *pKeyValues, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent , void *pContext)
{
	CTFMessageBoxDialog *pDialog = vgui::SETUP_PANEL( new CTFMessageBoxDialog( pTitle, pText,
		pConfirmBtnText,
		callback, parent ) );

	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		if ( pKeyValues )
		{
			pDialog->SetStringTokens( pKeyValues );
			pDialog->SetDialogVariable( "text", pDialog->GetText() );
		}

		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: Pop up a model yes/no dialog with an "opt out" checkbox that persists via a ConVar
//-----------------------------------------------------------------------------
CTFGenericConfirmOptOutDialog *ShowConfirmOptOutDialog( const char *pTitle, const char *pText, const char *pConfirmBtnText, const char *pCancelBtnText, const char *pOptOutText, const char *pOptOutConVarName, GenericConfirmDialogCallback callback, vgui::Panel *parent)
{
	CTFGenericConfirmOptOutDialog *pDialog = vgui::SETUP_PANEL( new CTFGenericConfirmOptOutDialog( pTitle, pText,
																								   pConfirmBtnText, pCancelBtnText,
																								   pOptOutText, pOptOutConVarName,
																								   callback, parent ) );
	if ( pDialog )
	{
		pDialog->Show();
	}

	return pDialog;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowUpgradeMessageBox( const char *pTitle, const char *pText,
										   const char *pConfirmBtnText, 
										   GenericConfirmDialogCallback callback,
										   vgui::Panel *parent, void *pContext )
{
	CTFMessageBoxDialog *pDialog = vgui::SETUP_PANEL(
		new CTFUpgradeBoxDialog(
		pTitle, pText,
		pConfirmBtnText, callback, parent
		)
		);

	if ( pDialog )
	{
		pDialog->SetContext( pContext );
		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: Pop up a dialog prompting the player to go to the store to upgrade
//-----------------------------------------------------------------------------
CTFMessageBoxDialog *ShowUpgradeMessageBox( const char *pTitle, const char *pText )
{
	return ShowUpgradeMessageBox( pTitle, pText, "#GameUI_OK", NULL, NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialogWithSound *ShowMessageBoxWithSound( const char *pTitle, const char *pText, const char *pszSound, float flDelay, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent, void *pContext )
{
	return ShowMessageBoxWithSound( pTitle, pText, NULL, pszSound, flDelay, pConfirmBtnText, callback, parent, pContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialogWithSound *ShowMessageBoxWithSound( const char *pTitle, const wchar_t *pText, const char *pszSound, float flDelay, const char *pConfirmBtnText , GenericConfirmDialogCallback callback, vgui::Panel *parent, void *pContext )
{
	CTFMessageBoxDialogWithSound *pDialog = vgui::SETUP_PANEL( new CTFMessageBoxDialogWithSound( pTitle, pText, pszSound, flDelay, pConfirmBtnText, callback, parent ) );

	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialogWithSound *ShowMessageBoxWithSound( const char *pTitle, const char *pText, KeyValues *pKeyValues, const char *pszSound, float flDelay, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent, void *pContext )
{
	CTFMessageBoxDialogWithSound *pDialog = vgui::SETUP_PANEL( new CTFMessageBoxDialogWithSound( pTitle, pText, pszSound, flDelay, pConfirmBtnText, callback, parent ) );

	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		if ( pKeyValues )
		{
			pDialog->SetStringTokens( pKeyValues );
			pDialog->SetDialogVariable( "text", pDialog->GetText() );
		}

		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialogWithSound::CTFMessageBoxDialogWithSound( const char *pTitle, const char *pText, const char *pszSound, float flDelay, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent ) 
	: CTFMessageBoxDialog( pTitle, pText, pConfirmBtnText, callback, parent )
{
	m_szSound[0] = 0;

	if ( pszSound )
	{
		V_strcpy_safe( m_szSound, pszSound );
	}

	m_flSoundTime = gpGlobals->curtime + flDelay;
	m_bPlayedSound = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMessageBoxDialogWithSound::CTFMessageBoxDialogWithSound( const char *pTitle, const wchar_t *pText, const char *pszSound, float flDelay, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent ) 
	: CTFMessageBoxDialog( pTitle, pText, pConfirmBtnText, callback, parent )
{
	m_szSound[0] = 0;

	if ( pszSound )
	{
		V_strcpy_safe( m_szSound, pszSound );
	}

	m_flSoundTime = gpGlobals->curtime + flDelay;
	m_bPlayedSound = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMessageBoxDialogWithSound::OnTick()
{
	BaseClass::OnTick();

	if ( !m_bPlayedSound && ( m_flSoundTime < gpGlobals->curtime ) )
	{
		m_bPlayedSound = true;

		if ( Q_strlen( m_szSound ) > 0 )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( m_szSound );
			}
		}
	}
}

#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFReviveDialog::CTFReviveDialog( const char *pTitle, const char *pText, const char *pConfirmBtnText, GenericConfirmDialogCallback callback, vgui::Panel *parent )
: CTFMessageBoxDialog( pTitle, pText, pConfirmBtnText, callback, parent )
{
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pTargetHealth->SetAllowAnimations( false );
	m_pTargetHealth->HideHealthBonusImage();
	
	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
	OnTick();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveDialog::PerformLayout()
{
	// Skipping base class
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFReviveDialog::OnTick()
{
	BaseClass::OnTick();

	if ( !m_pTargetHealth )
		return;

	if ( !m_hEntity )
		return;

	float flHealth = m_hEntity->GetHealth();
	if ( flHealth != m_flPrevHealth )
	{
		float flMaxHealth = m_hEntity->GetMaxHealth();
		m_pTargetHealth->SetHealth( flHealth, flMaxHealth, flMaxHealth );
		m_flPrevHealth = flHealth;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFReviveDialog::SetOwner( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		m_hEntity = pEntity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: In-game dialog that avoids the crosshair area and is much smaller
//-----------------------------------------------------------------------------
CTFReviveDialog *ShowRevivePrompt( CBaseEntity *pOwner,
								   const char *pTitle, 
								   const char *pText,
								   const char *pConfirmBtnText, 
								   GenericConfirmDialogCallback callback,
								   vgui::Panel *parent, void *pContext )
{
	CTFReviveDialog *pDialog = vgui::SETUP_PANEL( new CTFReviveDialog( pTitle, pText, pConfirmBtnText, callback, parent ) );
	if ( pDialog )
	{
		if ( pContext )
		{
			pDialog->SetContext( pContext );
		}

		pDialog->SetOwner( pOwner );
		pDialog->Show();
	}

	return pDialog;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconRequirementDialog::CEconRequirementDialog( const char *pTitle, const char *pTextKey, const char *pItemDefName )
	: CTFGenericConfirmDialog( pTitle, pTextKey, NULL, NULL, NULL, NULL )
	, m_hItemDef( pItemDefName )
{
	m_pItemAd = new CCyclingAdContainerPanel( this, "CyclingAd" );

	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CEconRequirementDialog::GetResFile()
{
	return "Resource/UI/MvMEconRequirementDialog.res";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconRequirementDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValuesAD pKVItemAd( "items" ); // The panel will copy these
	KeyValues* pKVItem = pKVItemAd->CreateNewKey();
	pKVItem->SetName( "0" );
	pKVItem->SetString( "item", m_hItemDef->GetItemDefinitionName() );
	pKVItem->SetBool( "show_market", false );
	m_pItemAd->BSetItemKVs( pKVItemAd );

	m_pItemAd->InvalidateLayout( true, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconRequirementDialog::PerformLayout()
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ShowEconRequirementDialog( const char *pTitle, const char *pText, const char *pItemDefName )
{
	CEconRequirementDialog *pDialog = vgui::SETUP_PANEL( new CEconRequirementDialog( pTitle, pText, pItemDefName ) );
	if ( pDialog )
	{
		pDialog->Show();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the correct res file to use (depends on Steam Controller state)
//-----------------------------------------------------------------------------
const char* CTFMessageBoxDialog::GetResFile()
{
	if ( ::input->IsSteamControllerActive() )
	{
		return "Resource/UI/econ/MessageBoxDialog_SC.res";
	}
	else
	{
		return "Resource/UI/econ/MessageBoxDialog.res";
	}
}

#endif // TF_CLIENT_DLL
