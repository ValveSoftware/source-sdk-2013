//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CONFIRM_DELETE_DIALOG_H
#define CONFIRM_DELETE_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "confirm_dialog.h"

//-----------------------------------------------------------------------------
// Purpose: A generic delete confirmation dialog - see CConfirmDialog.
//-----------------------------------------------------------------------------
class CConfirmDeleteDialog : public CConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CConfirmDeleteDialog,CConfirmDialog );
public:
	CConfirmDeleteDialog( vgui::Panel *parent );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
};

#endif // CONFIRM_DELETE_DIALOG_H
