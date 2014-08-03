//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BUILDMODEDIALOG_H
#define BUILDMODEDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

struct PanelItem_t;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in build mode editing
//-----------------------------------------------------------------------------
class BuildModeDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( BuildModeDialog, Frame );

public:
	BuildModeDialog( BuildGroup *buildGroup );
	~BuildModeDialog();

	// Set the current control to edit
	MESSAGE_FUNC_PTR( SetActiveControl, "SetActiveControl", panelPtr );

	// Update the current control with the current resource settings.
	MESSAGE_FUNC_PTR( UpdateControlData, "UpdateControlData", panel );

	// Store the current settings of all panels in the build group.
	virtual KeyValues *StoreSettings();

	// Store the current settings of the current panel 
	MESSAGE_FUNC( StoreUndoSettings, "StoreUndo" );

	/* CUSTOM MESSAGE HANDLING
		"SetActiveControl"
			input:	"PanelPtr"	- panel to set active control to edit to
	*/	

	MESSAGE_FUNC( OnShowNewControlMenu, "ShowNewControlMenu" );

protected:
	virtual void PerformLayout();
	virtual void OnClose();
	virtual void OnCommand( const char *command );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual bool IsBuildGroupEnabled();

private:
	void CreateControls();
	
	void OnKeyCodeTyped(KeyCode code);
	MESSAGE_FUNC( ApplyDataToControls, "ApplyDataToControls" );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC( OnDeletePanel, "DeletePanel" );
	void ExitBuildMode();
	Panel *OnNewControl(const char *name, int x = 0, int y = 0);
	MESSAGE_FUNC( DoUndo, "Undo" );
	MESSAGE_FUNC( DoCopy, "Copy" );
	MESSAGE_FUNC( DoPaste, "Paste" );
	MESSAGE_FUNC( EnableSaveButton, "EnableSaveButton" );
	void RevertToSaved();
	void ShowHelp();
	MESSAGE_FUNC( ShutdownBuildMode, "Close" );
	MESSAGE_FUNC( OnPanelMoved, "PanelMoved" );
	MESSAGE_FUNC( OnTextKillFocus, "TextKillFocus" );
	MESSAGE_FUNC( OnReloadLocalization, "ReloadLocalization" );
	MESSAGE_FUNC_CHARPTR( OnCreateNewControl, "CreateNewControl", text );

	MESSAGE_FUNC_CHARPTR( OnSetClipboardText, "SetClipboardText", text );

	MESSAGE_FUNC_INT( OnChangeChild, "OnChangeChild", direction );

	Panel *m_pCurrentPanel;
	BuildGroup *m_pBuildGroup;
	Label *m_pStatusLabel;
	ComboBox *m_pFileSelectionCombo;
	Divider *m_pDivider;

	class PanelList;
	PanelList *m_pPanelList;

	Button *m_pSaveButton;
	Button *m_pApplyButton;
	Button *m_pExitButton;
	Button *m_pDeleteButton;
	Button *m_pReloadLocalization;
	MenuButton *m_pVarsButton;

	bool _autoUpdate;

	ComboBox *m_pAddNewControlCombo; // combo box for adding new controls
	KeyValues *_undoSettings; // settings for the Undo command
	KeyValues *_copySettings; // settings for the Copy/Paste command
	char _copyClassName[255];
	int			m_nClick[ 2 ];

	void RemoveAllControls( void );
	void UpdateEditControl(PanelItem_t &panelItem, const char *datstring);

	enum {
		TYPE_STRING,
		TYPE_INTEGER,
		TYPE_COLOR,
		TYPE_ALIGNMENT,
		TYPE_AUTORESIZE,
		TYPE_CORNER,
		TYPE_LOCALIZEDSTRING,
	};

	vgui::DHANDLE< Menu >	m_hContextMenu;

	ComboBox	*m_pEditableParents;
	ComboBox	*m_pEditableChildren;

	Button		*m_pNextChild;
	Button		*m_pPrevChild;

	friend class PanelList;
};

} // namespace vgui


#endif // BUILDMODEDIALOG_H

