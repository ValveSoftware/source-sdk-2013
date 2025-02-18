//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "item_confirm_delete_dialog.h"
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConfirmDeleteItemDialog::CConfirmDeleteItemDialog( vgui::Panel *parent, bool bMultiItem ) : BaseClass(parent)
{
	m_bMultiItem = bMultiItem;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const wchar_t *CConfirmDeleteItemDialog::GetText()
{
	return m_bMultiItem ? g_pVGuiLocalize->Find("MultiDeleteItemConfirmText") : g_pVGuiLocalize->Find("DeleteItemConfirmText");
}
