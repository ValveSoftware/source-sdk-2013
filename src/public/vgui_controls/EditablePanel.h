//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EDITABLEPANEL_H
#define EDITABLEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/FocusNavGroup.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Panel that supports editing via the build dialog
//-----------------------------------------------------------------------------
class EditablePanel : public Panel
{
	DECLARE_CLASS_SIMPLE( EditablePanel, Panel );

public:
	EditablePanel(Panel *parent, const char *panelName);
	EditablePanel(Panel *parent, const char *panelName, HScheme hScheme);

	virtual ~EditablePanel();

	// Load the control settings - should be done after all the children are added
	// If you pass in pPreloadedKeyValues, it won't actually load the file. That way, you can cache
	// the keyvalues outside of here if you want to prevent file accesses in the middle of the game.
	virtual void LoadControlSettings(const char *dialogResourceName, const char *pathID = NULL, KeyValues *pPreloadedKeyValues = NULL, KeyValues *pConditions = NULL);
	virtual void ApplySettings(KeyValues *inResourceData);

	// sets the name of this dialog so it can be saved in the user config area
	// use dialogID to differentiate multiple instances of the same dialog
	virtual void LoadUserConfig(const char *configName, int dialogID = 0);
	virtual void SaveUserConfig();

	// combines both of the above, LoadControlSettings & LoadUserConfig
	virtual void LoadControlSettingsAndUserConfig(const char *dialogResourceName, int dialogID = 0);

	// Override to change how build mode is activated
	virtual void ActivateBuildMode();

	// Return the buildgroup that this panel is part of.
	virtual BuildGroup *GetBuildGroup();

	// Virtual factory for control creation
	// controlName is a string which is the same as the class name
	virtual Panel *CreateControlByName(const char *controlName);

	// Shortcut function to set data in child controls
	virtual void SetControlString(const char *controlName, const char *string);
	// Shortcut function to set data in child controls
	virtual void SetControlString(const char *controlName, const wchar_t *string);
	// Shortcut function to set data in child controls
	virtual void SetControlInt(const char *controlName, int state);
	// Shortcut function to get data in child controls
	virtual int GetControlInt(const char *controlName, int defaultState);
	// Shortcut function to get data in child controls
	// Returns a maximum of 511 characters in the string
	virtual const char *GetControlString(const char *controlName, const char *defaultString = "");
	// as above, but copies the result into the specified buffer instead of a static buffer
	virtual void GetControlString(const char *controlName, char *buf, int bufSize, const char *defaultString = "");
	// sets the enabled state of a control
	virtual void SetControlEnabled(const char *controlName, bool enabled);
	virtual void SetControlVisible(const char *controlName, bool visible);

	// localization variables (used in constructing UI strings)
	// after the variable is set, causes all the necessary sub-panels to update
	virtual void SetDialogVariable(const char *varName, const char *value);
	virtual void SetDialogVariable(const char *varName, const wchar_t *value);
	virtual void SetDialogVariable(const char *varName, int value);
	virtual void SetDialogVariable(const char *varName, float value);

	// Focus handling
	// Delegate focus to a sub panel
	virtual void RequestFocus(int direction = 0);
	virtual bool RequestFocusNext(VPANEL panel);
	virtual bool RequestFocusPrev(VPANEL panel);
	// Pass the focus down onto the last used panel
	virtual void OnSetFocus();
	// Update focus info for navigation
	virtual void OnRequestFocus(VPANEL subFocus, VPANEL defaultPanel);
	// Get the panel that currently has keyfocus
	virtual VPANEL GetCurrentKeyFocus();
	// Get the panel with the specified hotkey
	virtual Panel *HasHotkey(wchar_t key);

	virtual void OnKeyCodePressed( KeyCode code );

	// Handle information requests
	virtual bool RequestInfo(KeyValues *data);
	/* INFO HANDLING
		"BuildDialog"
			input:
				"BuildGroupPtr" - pointer to the panel/dialog to edit
			returns:
				"PanelPtr" - pointer to a new BuildModeDialog()

		"ControlFactory"
			input:
				"ControlName" - class name of the control to create
			returns:
				"PanelPtr" - pointer to the newly created panel, or NULL if no such class exists
	*/	
	// registers a file in the list of control settings, so the vgui dialog can choose between them to edit
	virtual void RegisterControlSettingsFile(const char *dialogResourceName, const char *pathID = NULL);

	// localization variables - only use this if you need to iterate the variables, use the SetLoc*() to set them
	KeyValues *GetDialogVariables();

protected:
	virtual void PaintBackground();

	// nav group access
	virtual FocusNavGroup &GetFocusNavGroup();

	// called when default button has been set
	MESSAGE_FUNC_HANDLE( OnDefaultButtonSet, "DefaultButtonSet", button );
	// called when the current default button has been set
	MESSAGE_FUNC_HANDLE( OnCurrentDefaultButtonSet, "CurrentDefaultButtonSet", button );
    MESSAGE_FUNC( OnFindDefaultButton, "FindDefaultButton" );

	// overrides
	virtual void OnChildAdded(VPANEL child);
	virtual void OnSizeChanged(int wide, int tall);
	virtual void OnClose();

	// user configuration settings
	// this is used for any control details the user wants saved between sessions
	// eg. dialog positions, last directory opened, list column width
	virtual void ApplyUserConfigSettings(KeyValues *userConfig);

	// returns user config settings for this control
	virtual void GetUserConfigSettings(KeyValues *userConfig);

	// optimization for text rendering, returns true if text should be rendered immediately after Paint()
	// disabled for now
	// virtual bool ShouldFlushText();

private:
	void ForceSubPanelsToUpdateWithNewDialogVariables();

	BuildGroup *_buildGroup;
	FocusNavGroup m_NavGroup;
	KeyValues *m_pDialogVariables;

	// the wide and tall to which all controls are locked - used for autolayout deltas
	char *m_pszConfigName;
	int m_iConfigID;
	bool m_bShouldSkipAutoResize;
};

} // namespace vgui

#endif // EDITABLEPANEL_H
