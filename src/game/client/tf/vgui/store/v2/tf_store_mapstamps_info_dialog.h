//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MAPS_INFO_DIALOG_H
#define TF_MAPS_INFO_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFMapStampsInfoDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFMapStampsInfoDialog, vgui::EditablePanel );
public:
	CTFMapStampsInfoDialog( vgui::Panel *pParent, const char *pName = "" );

	void			DoClose();

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	OnCommand( const char *command );
	virtual void	PerformLayout( void );
	virtual void	OnMouseReleased(vgui::MouseCode code);

	Panel			*m_pBgPanel;
	EditablePanel	*m_pDlgFrame;
};

#endif // TF_MAPS_INFO_DIALOG_H
