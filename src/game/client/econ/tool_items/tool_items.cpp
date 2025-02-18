//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/TextImage.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_ui.h"
#include "tool_items.h"
#ifdef TF_CLIENT_DLL
	#include "tf_item_tools.h"
#else
	#include "econ_item_tools.h"
#endif
#include <vgui/ILocalize.h>
#include "gc_clientsystem.h"
#include "item_style_select_dialog.h"			// for CComboBoxBackpackOverlayDialogBase
#include "backpack_panel.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

#ifdef TF_CLIENT_DLL
#include "character_info_panel.h"
#include "c_tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseToolUsageDialog::CBaseToolUsageDialog( vgui::Panel *parent, const char *panelName, CEconItemView *pTool, CEconItemView *pToolSubject ) : vgui::EditablePanel( parent, panelName )
{
	m_pToolModelPanel = new CItemModelPanel( this, "tool_modelpanel" );
	m_pToolModelPanel->SetActAsButton( true, true );
	m_pSubjectModelPanel = new CItemModelPanel( this, "subject_modelpanel" );
	m_pSubjectModelPanel->SetActAsButton( true, true );
	m_pMouseOverItemPanel = vgui::SETUP_PANEL( new CItemModelPanel( this, "mouseoveritempanel" ) );

	m_pMouseOverTooltip = new CItemModelPanelToolTip( this );
	m_pMouseOverTooltip->SetupPanels( this, m_pMouseOverItemPanel );
	m_pToolModelPanel->SetTooltip( m_pMouseOverTooltip, "" );
	m_pSubjectModelPanel->SetTooltip( m_pMouseOverTooltip, "" );

	m_pToolModelPanel->SetItem( pTool );
	m_pToolModelPanel->SetShowEquipped( true );
	m_pSubjectModelPanel->SetItem( pToolSubject );
	m_pSubjectModelPanel->SetShowEquipped( true );

	m_pTitleLabel = NULL;
	m_pszInternalPanelName = panelName ? panelName : "unknown";
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseToolUsageDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetDialogVariable( "oldname", m_pSubjectModelPanel->GetItem()->GetItemName() );

	m_pTitleLabel = dynamic_cast<vgui::Label*>( FindChildByName("TitleLabel") );
	if ( m_pTitleLabel )
	{
		wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "ToolDialogTitle" );
		if ( pszBaseString )
		{
			wchar_t	wTemp[256];
			g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, 2, m_pToolModelPanel->GetItem()->GetItemName(), m_pSubjectModelPanel->GetItem()->GetItemName() );
			m_pTitleLabel->SetText( wTemp );

			// Now go through the string and find the escape characters telling us where the color changes are
			m_pTitleLabel->GetTextImage()->ClearColorChangeStream();

			// We change the title's text color to match the colors of the matching model panel backgrounds
			wchar_t *txt = wTemp;
			int iWChars = 0;
			Color colCustom;
			while ( txt && *txt )
			{
				switch ( *txt )
				{
				case 0x01:	// Normal color
					m_pTitleLabel->GetTextImage()->AddColorChange( Color(235,226,202,255), iWChars );
					break;
				case 0x02:	// Item 1 color
					m_pTitleLabel->GetTextImage()->AddColorChange( Color(112,176,74,255), iWChars );
					break;
				case 0x03:	// Item 2 color
					m_pTitleLabel->GetTextImage()->AddColorChange( Color(71,98,145,255), iWChars );
					break;
				default:
					break;
				}
				txt++;
				iWChars++;
			}
		}
	}

	vgui::Panel *pToolIcon = FindChildByName( "tool_icon" );
	if ( pToolIcon )
	{
		pToolIcon->SetMouseInputEnabled( false );
		pToolIcon->SetKeyBoardInputEnabled( false );
	}
	vgui::Panel *pSubjectIcon = FindChildByName( "subject_icon" );
	if ( pSubjectIcon )
	{
		pSubjectIcon->SetMouseInputEnabled( false );
		pSubjectIcon->SetKeyBoardInputEnabled( false );
	}	

	// @note Tom Bui: because the children have already applied their scheme settings and/or performed their layout before
	// this dialog has had a chance to load its res file, we need to manually invalidate the layout of these children
	// to make sure that the settings are valid with respect to what the parent wants
	m_pToolModelPanel->UpdatePanels();
	m_pSubjectModelPanel->UpdatePanels();
	m_pMouseOverItemPanel->SetBorder( pScheme->GetBorder("LoadoutItemPopupBorder") );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseToolUsageDialog::PerformLayout()
{
	BaseClass::PerformLayout();
	// if ( m_pMouseOverItemPanel->IsVisible() )
	// {
	// 	// The mouseover panel was visible. Fake a panel entry into the original panel to get it to show up again properly.
	// 	if ( m_pItemPanelBeingMousedOver )
	// 	{
	// 		OnItemPanelEntered( m_pItemPanelBeingMousedOver );
	// 	}
	// 	else
	// 	{
	// 		HideMouseOverPanel();
	// 	}
	// }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseToolUsageDialog::OnCommand( const char *command )
{
	// in any case, our dialog is going away
	TFModalStack()->PopModal( this );
	SetVisible( false );
	MarkForDeletion();

#ifdef TF_CLIENT_DLL
	const char *pSubjectBaseName = m_pSubjectModelPanel && m_pSubjectModelPanel->GetItem() && m_pSubjectModelPanel->GetItem()->GetItemDefinition()
								 ? m_pSubjectModelPanel->GetItem()->GetItemDefinition()->GetItemBaseName()
								 : "n/a";
#endif

	if ( !Q_stricmp( command, "apply" ) )
	{
#ifdef TF_CLIENT_DLL
		C_CTFGameStats::ImmediateWriteInterfaceEvent( CFmtStr( "tool_usage_proceed(%s)", m_pszInternalPanelName ).Access(),
													  pSubjectBaseName );
#endif
		// Call before Apply() in case it creates new dialogs that wants to handle prevention
		EconUI()->SetPreventClosure( false );
		Apply();
	}
	else // "cancel"
	{
#ifdef TF_CLIENT_DLL
		C_CTFGameStats::ImmediateWriteInterfaceEvent( CFmtStr( "tool_usage_cancel(%s)", m_pszInternalPanelName ).Access(),
													  pSubjectBaseName );
#endif

		EconUI()->SetPreventClosure( false );

		IGameEvent *event = gameeventmanager->CreateEvent( "inventory_updated" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Utility function
//-----------------------------------------------------------------------------
void	CBaseToolUsageDialog::OnKeyCodeTyped( vgui::KeyCode code )
{
	if( code == KEY_ESCAPE )
	{
		OnCommand( "cancel" );
	}
	else
	{
		BaseClass::OnKeyCodeTyped( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Utility function
//-----------------------------------------------------------------------------
void	CBaseToolUsageDialog::OnKeyCodePressed( vgui::KeyCode code )
{
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
		// eat all the movement keys so the selection doesn't update behind the dialog
	}
	else if( nButtonCode == KEY_XBUTTON_A || code == KEY_ENTER || nButtonCode == STEAMCONTROLLER_A )
	{
		OnCommand( "apply" );
	}
	else if( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_B )
	{
		OnCommand( "cancel" );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Utility function
//-----------------------------------------------------------------------------
void MakeModalAndBringToFront( vgui::EditablePanel *dialog )
{
	dialog->SetVisible( true );
	if ( dialog->GetParent() == NULL )
	{
		dialog->MakePopup();
	}
	dialog->SetZPos( 10000 );
	dialog->MoveToFront();
	dialog->SetKeyBoardInputEnabled( true );
	dialog->SetMouseInputEnabled( true );
	TFModalStack()->PushModal( dialog );

	EconUI()->SetPreventClosure( true );
}

//-----------------------------------------------------------------------------
//
// Given a tool and an item to apply the tool's effects upon,
// gather required information from the user and
// send a change request to the GC.
//
bool ApplyTool( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
{
	if ( !pTool || !pToolSubject )
		return false;

	if ( !CEconSharedToolSupport::ToolCanApplyTo( pTool, pToolSubject ) )
		return false;

	// this tool can be applied to this subject item
	const IEconTool *pEconTool = pTool->GetStaticData()->GetEconTool();
	if ( !pEconTool )
		return false;

	pEconTool->OnClientApplyTool( pTool, pToolSubject, pParent );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: STRANGE COUNT TRANSFER
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CConfirmStrangeCountTransferApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmStrangeCountTransferApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmStrangeCountTransferApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmApplyStrangeCountTransferDialog", pTool, pToolSubject )
	{
		m_pItemSrc = NULL;
		m_pItemDest = NULL;
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmApplyDuckTokenDialog.res" ); // fix me

		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyStrangeCountTransfer> msg( k_EMsgGCApplyStrangeCountTransfer );

		if ( !m_pItemSrc || !m_pItemDest )
			return;

		msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_item_src_item_id( m_pItemSrc->GetItemID() );
		msg.Body().set_item_dest_item_id( m_pItemDest->GetItemID() );
		GCClientSystem()->BSendMessage( msg );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_strangecounttransfer", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );
	}

	bool SetItems( CEconItemView *pItemSrc, CEconItemView *pItemDest )
	{
		if ( !pItemSrc || !pItemDest )
			return false;

		if ( CEconTool_StrangeCountTransfer::AreItemsEligibleForStrangeCountTransfer( pItemSrc, pItemDest ) )
		{
			m_pItemSrc = pItemSrc;
			m_pItemDest = pItemDest;

			return true;
		}

		return false;
	}

private:
	CEconItemView *m_pItemSrc;
	CEconItemView *m_pItemDest;
};

/*static */bool CEconTool_StrangeCountTransfer::SetItems( CEconItemView *pItemSrc, CEconItemView *pItemDest )
{
	if ( !pItemSrc || !pItemDest )
		return false;

	if ( CEconTool_StrangeCountTransfer::AreItemsEligibleForStrangeCountTransfer( pItemSrc, pItemDest ) )
	{
		CEconTool_StrangeCountTransfer::m_pItemSrc = pItemSrc;
		CEconTool_StrangeCountTransfer::m_pItemDest = pItemDest;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_StrangeCountTransfer::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmStrangeCountTransferApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmStrangeCountTransferApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}


// ****************************************************************************
// STRANGE PARTS
//-----------------------------------------------------------------------------
// Purpose: Confirm / abort strange part application
//-----------------------------------------------------------------------------
class CConfirmStrangePartApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmStrangePartApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmStrangePartApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	Apply( void );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmStrangePartApplicationDialog::CConfirmStrangePartApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
	: CBaseToolUsageDialog( pParent, "ConfirmApplyStrangePartApplicationDialog", pTool, pToolSubject )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmStrangePartApplicationDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmApplyStrangePartApplicationDialog.res" );

	int iRemainingStrangePartSlots = 0;
	for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
	{
		if ( !GetKillEaterAttr_IsUserCustomizable( i ) )
			continue;

		if ( !m_pSubjectModelPanel->GetItem()->FindAttribute( GetKillEaterAttr_Score( i ) ) )
			++iRemainingStrangePartSlots;
	}

	SetDialogVariable( "remaining_strange_part_slots", iRemainingStrangePartSlots );
	SetDialogVariable( "maximum_strange_part_slots", GetKillEaterAttrCount_UserCustomizable() );
	SetDialogVariable( "subject_item_def_name", GLocalizationProvider()->Find( m_pSubjectModelPanel->GetItem()->GetItemDefinition()->GetItemBaseName() ) );
	SetDialogVariable( "slot_singular_plural", GLocalizationProvider()->Find( iRemainingStrangePartSlots == 1 ? "#Econ_FreeSlot_Singular" : "#Econ_FreeSlot_Plural" ) );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmStrangePartApplicationDialog::Apply()
{
	GCSDK::CProtoBufMsg<CMsgApplyStrangePart> msg( k_EMsgGCApplyStrangePart );

	msg.Body().set_strange_part_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
	msg.Body().set_item_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_strange_part", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_StrangePart::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmStrangePartApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmStrangePartApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort strange restriction application
//-----------------------------------------------------------------------------
class CConfirmStrangeRestrictionApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmStrangeRestrictionApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmStrangeRestrictionApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject, int iStrangeSlot, const char *pszStatLocalizationToken );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	Apply( void );

private:
	int m_iStrangeSlot;
	const char *m_pszStatLocalizationToken;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmStrangeRestrictionApplicationDialog::CConfirmStrangeRestrictionApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject, int iStrangeSlot, const char *pszStatLocalizationToken )
	: CBaseToolUsageDialog( pParent, "ConfirmApplyStrangeRestrictionApplicationDialog", pTool, pToolSubject )
	, m_iStrangeSlot( iStrangeSlot )
	, m_pszStatLocalizationToken( pszStatLocalizationToken )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmStrangeRestrictionApplicationDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmApplyStrangeRestrictionApplicationDialog.res" );

	SetDialogVariable( "stat_name", GLocalizationProvider()->Find( m_pszStatLocalizationToken ) );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmStrangeRestrictionApplicationDialog::Apply()
{
	GCSDK::CProtoBufMsg<CMsgApplyStrangeRestriction> msg( k_EMsgGCApplyStrangeRestriction );

	msg.Body().set_strange_part_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
	msg.Body().set_item_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );
	msg.Body().set_strange_attr_index( m_iStrangeSlot );

	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_strange_restriction", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

	GCClientSystem()->BSendMessage( msg );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSelectStrangePartToRestrictDialog : public CComboBoxBackpackOverlayDialogBase
{
public:
	DECLARE_CLASS_SIMPLE( CSelectStrangePartToRestrictDialog, CComboBoxBackpackOverlayDialogBase );

public:
	CSelectStrangePartToRestrictDialog( vgui::Panel *pParent, CEconItemView *pToolItem, CEconItemView *pSubjectItem )
		: CComboBoxBackpackOverlayDialogBase( pParent, pSubjectItem )
		, m_ToolItem( *pToolItem )
		, m_SubjectItem( *pSubjectItem )
	{
		//
	}

private:
	virtual void PopulateComboBoxOptions()
	{
		const wchar_t *pLocBase = GLocalizationProvider()->Find( "#ApplyStrangeRestrictionCombo" );

		const CEconTool_StrangePartRestriction *pToolRestriction = m_ToolItem.GetItemDefinition()->GetTypedEconTool<CEconTool_StrangePartRestriction>();
		Assert( pToolRestriction );

		KeyValues *pKeyValues = new KeyValues( "data" );
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			uint32 unScoreType;
			if ( !GetItemSchema()->BCanStrangeFilterApplyToStrangeSlotInItem( pToolRestriction->GetRestrictionType(), pToolRestriction->GetRestrictionValue(), &m_SubjectItem, i, &unScoreType ) )
				continue;

			const char *pszTypeLocKey = GetItemSchema()->GetKillEaterScoreTypeLocString( unScoreType );
			if ( !pszTypeLocKey )
				continue;

			pKeyValues->SetInt( "data", i );
			pKeyValues->SetString( "token", pszTypeLocKey );

			GetComboBox()->AddItem( CConstructLocalizedString( pLocBase, GLocalizationProvider()->Find( pszTypeLocKey ) ), pKeyValues );
		}
		pKeyValues->deleteThis();

		Assert( GetComboBox()->GetItemCount() > 0 );

		GetComboBox()->ActivateItemByRow( 0 );
	}

	virtual void OnComboBoxApplication()
	{
		KeyValues *pKVActiveUserData = GetComboBox()->GetActiveItemUserData();
		int iIndex = pKVActiveUserData ? pKVActiveUserData->GetInt( "data", -1 ) : -1;
		if ( iIndex < 0 )
			return;
		
		// FIXME: CConfirmStrangePartApplicationDialog is wrong class
		CConfirmStrangeRestrictionApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmStrangeRestrictionApplicationDialog( GetParent(), &m_ToolItem, &m_SubjectItem, iIndex, pKVActiveUserData ? pKVActiveUserData->GetString( "token", NULL ) : NULL ) );
		MakeModalAndBringToFront( pDialog );
	}

	virtual const char *GetTitleLabelLocalizationToken() const { return "#ApplyStrangeRestrictionPartTitle"; }

private:
	CEconItemView m_ToolItem;
	CEconItemView m_SubjectItem;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_StrangePartRestriction::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	if ( EconUI()->GetBackpackPanel() )
	{
		EconUI()->GetBackpackPanel()->SetComboBoxOverlaySelectionItem( pSubject );
	}

	CSelectStrangePartToRestrictDialog *pDialog = vgui::SETUP_PANEL( new CSelectStrangePartToRestrictDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
class CWaitingDialog : public CGenericWaitingDialog
{
public:
	CWaitingDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		vgui::surface()->PlaySound( "misc/achievement_earned.wav" );

		// Show them the result item.
		InventoryManager()->ShowItemsPickedUp( true );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort card upgrade tool application
//-----------------------------------------------------------------------------
class CConfirmApplyStrangifierDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmApplyStrangifierDialog, CBaseToolUsageDialog );

public:
	CConfirmApplyStrangifierDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject, const char *pszPromptLocToken, const char *pszTransactionReason, const char *pszUpdatingText = "" )
		: CBaseToolUsageDialog( pParent, "ConfirmApplyStrangifierDialog", pTool, pToolSubject )
		, m_sPromptLocToken( pszPromptLocToken )
		, m_sTransactionReason( pszTransactionReason )
		, m_sUpdatingText( pszUpdatingText )
		, m_resultItem( *pToolSubject )
	{
		m_pModelInspectPanel = new CEmbeddedItemModelPanel( this, "ModelInspectionPanel" );
	}

	virtual const char* GetResFile() const { return "Resource/UI/econ/ConfirmApplyStrangifierDialog.res"; }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( GetResFile() );
		BaseClass::ApplySchemeSettings( pScheme );

		CExLabel* pTextLabel = dynamic_cast<CExLabel*>( FindChildByName( "ConfirmLabel" ) );
		if( pTextLabel && m_pToolModelPanel )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( m_sPromptLocToken );
			wchar_t wTempFinalString[1024] = { 0 };
			if ( pszBaseString )
			{
				V_wcscpy_safe( wTempFinalString, pszBaseString );
			}

			// If the strangifier is untradable, add an extra warning in the prompt to let the user know
			if( m_pToolModelPanel->GetItem() && !m_pToolModelPanel->GetItem()->IsTradable() &&
				m_pSubjectModelPanel && m_pSubjectModelPanel->GetItem() )
			{
				wchar_t *pszUntradableString = g_pVGuiLocalize->Find( "ToolStrangifierUntradableWarning" );

				// Stick the names of the items into the string
				wchar_t	wTempUntradable[1024] = { 0 };
				g_pVGuiLocalize->ConstructString_safe( wTempUntradable, pszUntradableString, 2, m_pToolModelPanel->GetItem()->GetItemName(), m_pSubjectModelPanel->GetItem()->GetItemName() );

				// Concat onto the the original string
				V_wcscat_safe( wTempFinalString, wTempUntradable, sizeof( wTempUntradable ) );
			}

			pTextLabel->SetText( wTempFinalString );
		}

		SetupItem();
	}

	virtual void Apply( void )
	{
		if ( m_pSubjectModelPanel->GetItem()->GetItemID() != INVALID_ITEM_ID )
		{
			GCSDK::CProtoBufMsg<CMsgApplyToolToItem> msg( k_EMsgGCApplyXifier );
			msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
			msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );
			GCClientSystem()->BSendMessage( msg );
		}
		else
		{
			GCSDK::CProtoBufMsg<CMsgApplyToolToBaseItem> msg( k_EMsgGCApplyBaseItemXifier );
			msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
			msg.Body().set_baseitem_def_index( m_pSubjectModelPanel->GetItem()->GetStaticData()->GetDefinitionIndex() );
			GCClientSystem()->BSendMessage( msg );
		}

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), m_sTransactionReason.String() );

		if ( *m_sUpdatingText.String() )
		{
			vgui::surface()->PlaySound( "ui/item_gift_wrap_use.wav" );
			ShowWaitingDialog( new CWaitingDialog( NULL ), m_sUpdatingText.String(), true, false, 5.0f );
		}
	}

	// Be default don't actually do anything.  Can be overridden for specific tools since the client
	// doesn't know what happens when a tool is applied to an item
	virtual void SetupItem() 
	{
		m_pModelInspectPanel->SetVisible( false );
	}

protected:
	CEmbeddedItemModelPanel *m_pModelInspectPanel;
	CEconItemView m_resultItem;

private:
	CUtlString m_sPromptLocToken;
	CUtlString m_sTransactionReason;
	CUtlString m_sUpdatingText;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_Strangifier::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmApplyStrangifierDialog *pDialog = vgui::SETUP_PANEL( new CConfirmApplyStrangifierDialog( pParent, pTool, pSubject, "ToolStrangifierConfirm", "strangified_item" ) );
	MakeModalAndBringToFront( pDialog );
}
//-----------------------------------------------------------------------------
void CEconTool_KillStreakifier::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmApplyStrangifierDialog *pDialog = vgui::SETUP_PANEL( new CConfirmApplyStrangifierDialog( pParent, pTool, pSubject, "ToolKillStreakifierConfirm", "killstreakified_item" ) );
	MakeModalAndBringToFront( pDialog );
}
//-----------------------------------------------------------------------------
void CEconTool_Festivizer::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmApplyStrangifierDialog *pDialog = vgui::SETUP_PANEL( new CConfirmApplyStrangifierDialog( pParent, pTool, pSubject, "ToolFestivizerConfirm", "festivized_item", "#ToolFestivizerInProgress" ) );
	MakeModalAndBringToFront( pDialog );
}
//-----------------------------------------------------------------------------
void CEconTool_Unusualifier::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmApplyStrangifierDialog *pDialog = vgui::SETUP_PANEL( new CConfirmApplyStrangifierDialog( pParent, pTool, pSubject, "ToolUnusualifierConfirm", "unusualified_item", "#ToolUnusualifierInProgress" ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
class CConfirmUseItemEaterRechargerDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmUseItemEaterRechargerDialog, CBaseToolUsageDialog );

public:
	CConfirmUseItemEaterRechargerDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmUseItemEaterRechargerDialog", pTool, pToolSubject )
	{
		
	} 

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmUseItemEaterRechargerDialog.res" );

		// Find the number of charges to add by looking at item tool rescritions.
		const CEconTool_ItemEaterRecharger *pTool = m_pToolModelPanel->GetItem()->GetItemDefinition()->GetTypedEconTool<CEconTool_ItemEaterRecharger>();
		if ( pTool )
		{
			int iCharges = pTool->GetChargesForItemDefId( m_pSubjectModelPanel->GetItem()->GetItemDefIndex() );
			SetDialogVariable( "charges_added", iCharges );
		}

		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyToolToItem> msg( k_EMsgGCItemEaterRecharger );
		msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "recharging_item" );

		GCClientSystem()->BSendMessage( msg );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_ItemEaterRecharger::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmUseItemEaterRechargerDialog *pDialog = vgui::SETUP_PANEL( new CConfirmUseItemEaterRechargerDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort card upgrade tool application
//-----------------------------------------------------------------------------
class CConfirmCardUpgradeApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmCardUpgradeApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmCardUpgradeApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmApplyCardUpgradeApplicationDialog", pTool, pToolSubject )
	{
		//
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmApplyCardUpgradeApplicationDialog.res" );

		// See how many card upgrades have already been applied
		CCountUserGeneratedAttributeIterator countIterator;
		m_pSubjectModelPanel->GetItem()->IterateAttributes( &countIterator );

		int iRemainingStrangePartSlots = GetMaxCardUpgradesPerItem() - countIterator.GetCount();

		SetDialogVariable( "remaining_upgrade_card_slots", iRemainingStrangePartSlots );
		SetDialogVariable( "subject_item_def_name", GLocalizationProvider()->Find( m_pSubjectModelPanel->GetItem()->GetItemDefinition()->GetItemBaseName() ) );
		SetDialogVariable( "slot_singular_plural", GLocalizationProvider()->Find( iRemainingStrangePartSlots == 1 ? "#Econ_FreeSlot_Singular" : "#Econ_FreeSlot_Plural" ) );

		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyUpgradeCard> msg( k_EMsgGCApplyUpgradeCard );

		msg.Body().set_upgrade_card_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_upgrade_card", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );
	}
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_UpgradeCard::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmCardUpgradeApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmCardUpgradeApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CConfirmTransmogrifyApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmTransmogrifyApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmTransmogrifyApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmTransmogrifyApplicationDialog", pTool, pToolSubject )
	{
		//
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmTransmogrifyApplicationDialog.res" );

		const CEconTool_ClassTransmogrifier *pTool = m_pToolModelPanel->GetItem()->GetItemDefinition()->GetTypedEconTool<CEconTool_ClassTransmogrifier>();
		Assert( pTool );

		if ( pTool )
		{
			int iOutputClass = pTool->GetOutputClass();
			if ( iOutputClass > 0 && iOutputClass < LOADOUT_COUNT )
			{
				SetDialogVariable( "output_class", GLocalizationProvider()->Find( g_aPlayerClassNames[ iOutputClass ] ) );
			}
		}
		
		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyToolToItem> msg( k_EMsgGCApplyClassTransmogrifier );

		msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_transmogrifier", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_ClassTransmogrifier::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmTransmogrifyApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmTransmogrifyApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

#ifdef TF_CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CConfirmSpellbookPageApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmSpellbookPageApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmSpellbookPageApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmSpellbookPageApplicationDialog", pTool, pToolSubject )
	{
		//
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmSpellbookPageApplicationDialog.res" );

		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyToolToItem> msg( k_EMsgGCApplyHalloweenSpellbookPage );

		msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_spellbook_page", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_TFSpellbookPage::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmSpellbookPageApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmSpellbookPageApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}

//-----------------------------------------------------------------------------
class CConfirmDuckTokenApplicationDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDuckTokenApplicationDialog, CBaseToolUsageDialog );

public:
	CConfirmDuckTokenApplicationDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, "ConfirmApplyDuckTokenDialog", pTool, pToolSubject )
	{
		//
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmApplyDuckTokenDialog.res" );

		BaseClass::ApplySchemeSettings( pScheme );
	}

	virtual void Apply( void )
	{
		GCSDK::CProtoBufMsg<CMsgApplyToolToItem> msg( k_EMsgGCApplyDuckToken );

		msg.Body().set_tool_item_id( m_pToolModelPanel->GetItem()->GetItemID() );
		msg.Body().set_subject_item_id( m_pSubjectModelPanel->GetItem()->GetItemID() );

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "applied_ducktoken", m_pToolModelPanel->GetItem()->GetItemDefIndex() );

		GCClientSystem()->BSendMessage( msg );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_DuckToken::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmDuckTokenApplicationDialog *pDialog = vgui::SETUP_PANEL( new CConfirmDuckTokenApplicationDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( pDialog );
}


#endif // TF_CLIENT_DLL
