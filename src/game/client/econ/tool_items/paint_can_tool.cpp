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
#include "paint_can_tool.h"
#include "econ_ui.h"
#include "gc_clientsystem.h"

#ifdef TF_CLIENT_DLL
#include "tf_shareddefs.h"
#endif // TF_CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

enum
{
#ifdef TF_CLIENT_DLL
	kTeamID0 = TF_TEAM_RED,
	kTeamID1 = TF_TEAM_BLUE,
#else // !defined( TF_CLIENT_DLL )
	kTeamID0 = -1,
	kTeamID1 = -1,
#endif // TF_CLIENT_DLL
};

//-----------------------------------------------------------------------------
// Purpose: Confirm / abort paint application
//-----------------------------------------------------------------------------
class CConfirmApplyPaintCanBaseDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmApplyPaintCanBaseDialog, CBaseToolUsageDialog );

protected:
	CConfirmApplyPaintCanBaseDialog( vgui::Panel *pParent, const char *szType, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CBaseToolUsageDialog( pParent, szType, pTool, pToolSubject )
	{
		//
	}

public:
	virtual void Apply( void )
	{
		// Send the apply request to the GC
		GCSDK::CGCMsg< MsgGCPaintItem_t > msg( k_EMsgGCPaintItem );

		msg.Body().m_unToolItemID = m_pToolModelPanel->GetItem()->GetItemID();
		msg.Body().m_unSubjectItemID = m_pSubjectModelPanel->GetItem()->GetItemID();

		EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "painted_item" );

		GCClientSystem()->BSendMessage( msg );
	}

protected:
	// Set up a particular panel for a certain item with a certain preview color.
	static void SetupPaintModelPanel( CItemModelPanel *pPaintModelPanel, CEconItemView *pToolSubjectView, int iTeam, int iPaintRGB )
	{
		static CSchemaAttributeDefHandle pAttrDef_ItemTintRGB( "set item tint RGB" );

		if ( !pAttrDef_ItemTintRGB )
			return;

		// Fake-paint the demonstration items.
		pPaintModelPanel->SetItem( pToolSubjectView );
		pPaintModelPanel->GetItem()->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_ItemTintRGB, iPaintRGB );

		// Set the appropriate skins for each item given the team and the style selected.
		int iStyleSkin = pToolSubjectView->GetSkin( iTeam );

		if ( iStyleSkin == -1 )
		{
			// Fallback case: rely on default skins.
			pPaintModelPanel->SetSkin( iTeam == kTeamID0 ? 0 : 1 );
		}
		else
		{
			pPaintModelPanel->SetSkin( iStyleSkin );
		}

		pPaintModelPanel->SetActAsButton( true, false );
		pPaintModelPanel->GetItem()->InvalidateColor();
		pPaintModelPanel->GetItem()->InvalidateOverrideColor();
	}

	CItemModelPanel	*m_pPaintModelPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Preview a single-color paint
//-----------------------------------------------------------------------------
class CConfirmApplyPaintCanDialog : public CConfirmApplyPaintCanBaseDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmApplyPaintCanDialog, CConfirmApplyPaintCanBaseDialog );

public:
	CConfirmApplyPaintCanDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CConfirmApplyPaintCanBaseDialog( pParent, "ConfirmApplyPaintCanDialog", pTool, pToolSubject )
	{
		m_pPaintModelPanel = new CItemModelPanel( this, "paint_model" );
		m_pPaintModelPanel->SetItem( pToolSubject );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmApplyPaintCanDialog.res" );
		
		BaseClass::ApplySchemeSettings( pScheme );

		SetupPaintModelPanel( m_pPaintModelPanel, m_pSubjectModelPanel->GetItem(), kTeamID0, m_pToolModelPanel->GetItem()->GetModifiedRGBValue( false ) );
	}

private:
	CItemModelPanel	*m_pPaintModelPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Preview team-colored paint
//-----------------------------------------------------------------------------
class CConfirmApplyTeamColorPaintCanDialog : public CConfirmApplyPaintCanBaseDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmApplyTeamColorPaintCanDialog, CConfirmApplyPaintCanBaseDialog );

public:
	CConfirmApplyTeamColorPaintCanDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject )
		: CConfirmApplyPaintCanBaseDialog( pParent, "ConfirmApplyTeamColorPaintCanDialog", pTool, pToolSubject )
	{
		m_pPaintModelPanel_Red = new CItemModelPanel( this, "paint_model_red" );
		m_pPaintModelPanel_Red->SetItem( pToolSubject );
		m_pPaintModelPanel_Blue = new CItemModelPanel( this, "paint_model_blue" );
		m_pPaintModelPanel_Blue->SetItem( pToolSubject );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		LoadControlSettings( "Resource/UI/econ/ConfirmApplyTeamColorPaintCanDialog.res" );

		BaseClass::ApplySchemeSettings( pScheme );

		SetupPaintModelPanel( m_pPaintModelPanel_Red, m_pSubjectModelPanel->GetItem(), kTeamID0, m_pToolModelPanel->GetItem()->GetModifiedRGBValue( false ) );
		SetupPaintModelPanel( m_pPaintModelPanel_Blue, m_pSubjectModelPanel->GetItem(), kTeamID1, m_pToolModelPanel->GetItem()->GetModifiedRGBValue( true ) );

		// @note Tom Bui: we need to change the global index so that the material does not get
		// re-used for each item.  Should be ok to change the high bit.
		Assert( m_pPaintModelPanel_Red->GetItem() );
		m_pPaintModelPanel_Red->GetItem()->SetItemID( m_pPaintModelPanel_Red->GetItem()->GetItemID() | ( 1LL << 63 ) );
	}

private:
	CItemModelPanel	*m_pPaintModelPanel_Red;
	CItemModelPanel	*m_pPaintModelPanel_Blue;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_PaintCan::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	// Bizarro logic: we have no way of finding out whether an item has two different
	//				  colors of paint applied so instead we ask whether both colors are
	//				  the same.
	CBaseToolUsageDialog *dialog = NULL;
	if ( pTool->GetModifiedRGBValue( true ) != pTool->GetModifiedRGBValue( false ) )
	{
		dialog = new CConfirmApplyTeamColorPaintCanDialog( pParent, pTool, pSubject );
	}
	else
	{
		dialog = new CConfirmApplyPaintCanDialog( pParent, pTool, pSubject );
	}
	vgui::SETUP_PANEL( dialog );
	MakeModalAndBringToFront( dialog );
}

//-----------------------------------------------------------------------------
// Purpose: GC Msg handler to receive the paint can response
//-----------------------------------------------------------------------------
class CGCPaintItemResponse : public GCSDK::CGCClientJob
{
public:
	CGCPaintItemResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CGCMsg<MsgGCStandardResponse_t> msg( pNetPacket );
		InventoryManager()->ShowItemsPickedUp( true );
		return true;
	}

};

GC_REG_JOB( GCSDK::CGCClient, CGCPaintItemResponse, "CGCPaintItemResponse", k_EMsgGCPaintItemResponse, GCSDK::k_EServerTypeGCClient );
