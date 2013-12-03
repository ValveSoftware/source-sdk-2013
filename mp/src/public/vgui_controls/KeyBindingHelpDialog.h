//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYBINDINGHELPDIALOG_H
#define KEYBINDINGHELPDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui/KeyCode.h"

namespace vgui
{

class ListPanel;
class CKeyBoardEditorDialog;

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBindingHelpDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( CKeyBindingHelpDialog, Frame );

public:
	CKeyBindingHelpDialog( Panel *parent, Panel *panelToView, KeyBindingContextHandle_t handle, KeyCode code, int modifiers );
	~CKeyBindingHelpDialog();

	virtual void			OnCommand( char const *cmd );
	virtual void			OnKeyCodeTyped(vgui::KeyCode code);

	// The key originally bound to help was pressed
	void					HelpKeyPressed();
private:

	virtual void			OnTick();

	bool					IsHelpKeyStillBeingHeld();

	void					PopulateList();
	void					GetMappingList( Panel *panel, CUtlVector< PanelKeyBindingMap * >& maps );

	void					AnsiText( char const *token, char *out, size_t buflen );

	vgui::PHandle			m_hPanel;
	KeyBindingContextHandle_t m_Handle;
	KeyCode					m_KeyCode;
	int						m_Modifiers;

	ListPanel				*m_pList;
	double					m_flShowTime;
	bool					m_bPermanent;

	DHANDLE< CKeyBoardEditorDialog >	m_hKeyBindingsEditor;
};

}

#endif // KEYBINDINGHELPDIALOG_H
