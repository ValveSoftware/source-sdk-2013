//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TOOL_ITEMS_H
#define TOOL_ITEMS_H
#ifdef _WIN32
#pragma once
#endif

#include "item_model_panel.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CBaseToolUsageDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseToolUsageDialog, vgui::EditablePanel );

public:
	CBaseToolUsageDialog( vgui::Panel *pParent, const char *panelName, CEconItemView *pTool, CEconItemView *pToolSubject );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	PerformLayout();
	virtual void	OnCommand( const char *command );
	virtual void	OnKeyCodeTyped( vgui::KeyCode code ) OVERRIDE;
	virtual void	OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;

	virtual void	Apply( void ) { return; }

	inline CEconItemView *GetToolItem() { return m_pToolModelPanel->GetItem(); };
	inline CEconItemView *GetSubjectItem() { return m_pSubjectModelPanel->GetItem(); };

protected:
	CItemModelPanel			*m_pToolModelPanel;
	CItemModelPanel			*m_pSubjectModelPanel;
	CItemModelPanel			*m_pMouseOverItemPanel;
	CItemModelPanelToolTip	*m_pMouseOverTooltip;
	vgui::Label				*m_pTitleLabel;

	const char				*m_pszInternalPanelName;
};


// Utility function for tool dialogs.
void MakeModalAndBringToFront( vgui::EditablePanel *dialog );

bool ToolCanApplyTo( CEconItemView *pTool, CEconItemView *pToolSubject );

// Given a tool and an item to apply the tool's effects upon,
// gather required information from the user and
// send a change request to the GC.
bool ApplyTool( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject );

#endif // TOOL_ITEMS_H
