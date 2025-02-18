//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_CONFIRM_DELETE_DIALOG_H
#define ITEM_CONFIRM_DELETE_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "confirm_delete_dialog.h"
#include "vgui_controls/EditablePanel.h"
#include "econ_controls.h"
#include "item_pickup_panel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CConfirmDeleteItemDialog : public CConfirmDeleteDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDeleteItemDialog, CConfirmDeleteDialog );
public:
	CConfirmDeleteItemDialog( vgui::Panel *parent, bool bMultiItem = false );

	void	SetMultiItem( bool bMultiItem ) { m_bMultiItem = bMultiItem; }
	virtual const wchar_t *GetText();

private:
	bool	m_bMultiItem;
};

#endif // ITEM_CONFIRM_DELETE_DIALOG_H
