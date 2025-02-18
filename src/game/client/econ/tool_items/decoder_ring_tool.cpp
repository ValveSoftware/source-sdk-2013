//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "econ_item_tools.h"
#include "tool_items.h"
#include "decoder_ring_tool.h"
#include "vgui/ISurface.h"
#include "econ_controls.h"
#include "econ_ui.h"
#include "gc_clientsystem.h"
#include "collection_crafting_panel.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#define ROBOCRATE_UNLOCKING_SND		"/mvm/mvm_tank_deploy.wav"
#define ROBOCRATE_OPENED_SND		"/mvm/mvm_tank_explode.wav"

int s_iCrateType = CRATETYPE_NORMAL;

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort tool application
//-----------------------------------------------------------------------------
class CConfirmDecodeDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDecodeDialog, CBaseToolUsageDialog );

public:
	CConfirmDecodeDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );
	virtual void	OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmDecodeDialog::CConfirmDecodeDialog( vgui::Panel *parent, CEconItemView *pTool, CEconItemView *pToolSubject ) : CBaseToolUsageDialog( parent, "ConfirmApplyDecodeDialog", pTool, pToolSubject )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDecodeDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmApplyDecodeDialog.res" );

	const CEconItemView* pCrate = m_pSubjectModelPanel->GetItem();

	s_iCrateType = CRATETYPE_NORMAL;
	// Check Crate Type
	if ( pCrate )
	{
		static CSchemaAttributeDefHandle pAttrib_IsWinterCase( "is winter case" );
		uint32 nIsWinterCase = 0;
		if ( pCrate->FindAttribute( pAttrib_IsWinterCase, &nIsWinterCase ) && nIsWinterCase != 0 )
		{
			s_iCrateType = CRATETYPE_WINTER;
		}
		else if ( pCrate->GetItemDefIndex() == 5635 ) // Robo Crate items_tools_crafting
		{
			s_iCrateType = CRATETYPE_ROBO;
		}
	}

	if ( pCrate && V_strstr( pCrate->GetItemDefinition()->GetDefinitionName(), "Case" ) )
	{
		SetDialogVariable( "confirm_text", GLocalizationProvider()->Find("#ToolDecodeConfirmCase") );
	}
	else
	{
		SetDialogVariable( "confirm_text", GLocalizationProvider()->Find("#ToolDecodeConfirm") );
	}

	const locchar_t *loc_Append = NULL;
	if ( pCrate && pCrate->GetItemDefinition() && pCrate->GetItemDefinition()->GetHolidayRestriction() )
	{
		loc_Append = GLocalizationProvider()->Find( "#ToolDecodeConfirm_OptionalAppend_RestrictedContents" );
	}
	if ( !loc_Append )
	{
		loc_Append = LOCCHAR( "" );
	}
	
	SetDialogVariable( "optional_append", loc_Append );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDecodeDialog::OnCommand( const char *command )
{
	if ( FStrEq( "cancel", command ) )
	{
		InventoryManager()->ShowItemsPickedUp( true );
	}

	BaseClass::OnCommand( command );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmDecodeDialog::Apply( void )
{
	static CSchemaAttributeDefHandle pAttrDef_SupplyCrateSeries( "set supply crate series" );

	// Tell the GC to unlock the subject item.
	GCSDK::CGCMsg< MsgGCUnlockCrate_t > msg( k_EMsgGCUnlockCrate );

	msg.Body().m_unToolItemID = m_pToolModelPanel->GetItem()->GetItemID();
	msg.Body().m_unSubjectItemID = m_pSubjectModelPanel->GetItem()->GetItemID();

	int iSeries = 0;
	CEconItemView* pCrate = m_pSubjectModelPanel->GetItem();
	if ( pCrate )
	{
		float fSeries;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pCrate, pAttrDef_SupplyCrateSeries, &fSeries ) )
		{
			iSeries = fSeries;
		}
	}

	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "unlocked_supply_crate", iSeries );

	GCClientSystem()->BSendMessage( msg );

	CCollectionCraftingPanel *pPanel = EconUI()->GetBackpackPanel()->GetCollectionCraftPanel();
	if ( pPanel )
	{
		pPanel->SetWaitingForItem( kEconItemOrigin_FoundInCrate );
	}
}


void CEconTool_CrateKey::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmDecodeDialog *dialog = vgui::SETUP_PANEL( new CConfirmDecodeDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( dialog );
}


class CWaitForCrateDialog : public CGenericWaitingDialog
{
public:
	CWaitForCrateDialog( vgui::Panel *pParent ) : CGenericWaitingDialog( pParent )
	{
	}

protected:
	virtual void OnTimeout()
	{
		// Play an exciting sound!
		if ( s_iCrateType == CRATETYPE_ROBO )
		{
			vgui::surface()->PlaySound( ROBOCRATE_OPENED_SND );
		}
		else
		{
			vgui::surface()->PlaySound( "misc/achievement_earned.wav" );
		}

		// Show them their loot!
		InventoryManager()->ShowItemsPickedUp( true );
	}
};


//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the decoder ring response
//-----------------------------------------------------------------------------
class CGCUnlockCrateResponse : public GCSDK::CGCClientJob
{
public:
	CGCUnlockCrateResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );

		if ( s_iCrateType == CRATETYPE_ROBO )
		{
			vgui::surface()->PlaySound( ROBOCRATE_UNLOCKING_SND );
		}
		else if ( s_iCrateType == CRATETYPE_WINTER )
		{
			vgui::surface()->PlaySound( "ui/item_open_holiday_crate.wav" );
		}
		else
		{
			vgui::surface()->PlaySound( "ui/item_open_crate_short.wav" );
		}

		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCUnlockCrateResponse, "CGCUnlockCrateResponse", k_EMsgGCUnlockCrateResponse, GCSDK::k_EServerTypeGCClient );
