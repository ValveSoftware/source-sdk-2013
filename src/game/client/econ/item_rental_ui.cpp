//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "rtime.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IInput.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "item_rental_ui.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_gamestats.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Confirm item preview.
//-----------------------------------------------------------------------------
class CConfirmItemPreviewDialog : public CBaseToolUsageDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmItemPreviewDialog, CBaseToolUsageDialog );

public:
	CConfirmItemPreviewDialog( vgui::Panel *pParent, CEconItemView *pPreviewItem );
	~CConfirmItemPreviewDialog();

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );

private:

	CEconItemView* m_pPreviewItem;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmItemPreviewDialog::CConfirmItemPreviewDialog( vgui::Panel *parent, CEconItemView *pPreviewItem ) : CBaseToolUsageDialog( parent, "ConfirmItemPreviewDialog", pPreviewItem, pPreviewItem )
{
	m_pPreviewItem = pPreviewItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmItemPreviewDialog::~CConfirmItemPreviewDialog()
{
	delete m_pPreviewItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmItemPreviewDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmItemPreviewDialog.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pTitleLabel = dynamic_cast<vgui::Label*>( FindChildByName("TitleLabel") );
	if ( m_pTitleLabel )
	{
		wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "ItemPreviewDialogTitle" );
		if ( pszBaseString )
		{
			wchar_t	wTemp[256];
			g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, 1, m_pToolModelPanel->GetItem()->GetItemName() );
			m_pTitleLabel->SetText( wTemp );
			m_pTitleLabel->GetTextImage()->ClearColorChangeStream();
		}
	}

	m_pSubjectModelPanel->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmItemPreviewDialog::Apply( void )
{
	// Notify the GC that the player wants to preview this item.
	GCSDK::CGCMsg< MsgGCItemPreviewRequest_t > msg( k_EMsgGCItemPreviewRequest );

	msg.Body().m_unItemDefIndex = m_pToolModelPanel->GetItem()->GetItemDefIndex();

	// OGS LOGGING HERE

	GCClientSystem()->BSendMessage( msg );

	EconUI()->SetPreventClosure( false );
}

CEconPreviewNotification::CEconPreviewNotification( uint64 ulSteamID, uint32 iItemDef ) 
	: CEconNotification() 
{
	SetSteamID( ulSteamID );
	SetLifetime( 20.0f );

	m_pItemDef = GetItemSchema()->GetItemDefinition( iItemDef );
	if ( !m_pItemDef )
		return;

	AddStringToken( "item_name",  g_pVGuiLocalize->Find(m_pItemDef->GetItemBaseName()) );
}

void CEconPreviewExpiredNotification::Trigger()
{
}