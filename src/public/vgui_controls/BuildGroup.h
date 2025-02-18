//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_BUILDGROUP_H
#define VGUI_BUILDGROUP_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlvector.h"
#include "tier1/utlsymbol.h"
#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <vgui/Cursor.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/PHandle.h>
#include "tier1/utlhandletable.h"

class KeyValues;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: a BuildGroup is a list of panels contained in a window (the contextPanel)
//			Members of this group are viewable and editable in Build Mode, via the BuildModeDialog wizard
//-----------------------------------------------------------------------------
class BuildGroup
{
	DECLARE_HANDLES( BuildGroup, 20 );

public:
	BuildGroup(Panel *parentPanel, Panel *contextPanel);
	~BuildGroup();

	// Toggle build mode on/off
	virtual void SetEnabled(bool state);

	// Check if buildgroup is enabled
	virtual bool IsEnabled();

	// Return the currently selected panel
	virtual Panel *GetCurrentPanel();

	// Load the control settings from file
	virtual void LoadControlSettings(const char *controlResourceName, const char *pathID = NULL, KeyValues *pPreloadedKeyValues = NULL, KeyValues *pConditions = NULL);

	// Reload the control settings from file
	void ReloadControlSettings();

	// changes which control settings are currently loaded
	void ChangeControlSettingsFile(const char *controlResourceName);

	// Save control settings from file, using the same resource 
	// name as what LoadControlSettings() was called with
	virtual bool SaveControlSettings();

	// Serialize settings from a resource data container
	virtual void ApplySettings(KeyValues *resourceData);

	// Serialize settings to a resource data container
	virtual void GetSettings(KeyValues *resourceData);

	// Remove all objects in the current control group
	virtual void RemoveSettings();

	// Get a new unique fieldname for a new control
	void GetNewFieldName(char *newFieldName, int newFieldNameSize, Panel *newPanel);

	// Check if a control name is already taken
	Panel *FieldNameTaken(const char *fieldName);

	// Add a new control (via the BuildModeDialog)
	Panel *NewControl( KeyValues *controlKeys, int x=0, int y=0);
	Panel *NewControl( const char *name, int x=0, int y=0);

	// Set the panel from which the build group gets all it's object creation information
	virtual void SetContextPanel(Panel *contextPanel);

	//Get the panel that build group is pointed at.
	virtual Panel *GetContextPanel();

	// Get the list of panels in the buildgroup
	CUtlVector<PHandle> *GetPanelList(); 

	// Get the resource file name used
	virtual const char *GetResourceName(void) { return m_pResourceName; }

	virtual void PanelAdded(Panel* panel);

	virtual bool MousePressed(MouseCode code,Panel* panel);
	virtual bool MouseReleased(MouseCode code,Panel* panel);

	// Get the list of panels that are currently selected
	virtual CUtlVector<PHandle> *GetControlGroup();

	// Toggle ruler display on/off
	virtual void ToggleRulerDisplay();
	
	// Toggle visibility of ruler number labels
	virtual void SetRulerLabelsVisible(bool state);

	// Check if ruler display is activated
	virtual bool HasRulersOn();

	// Draw Rulers on screen 
	virtual void DrawRulers();

	// registers that a control settings file may be loaded
	// use when the dialog may have multiple states and the editor will need to be able to switch between them
	void RegisterControlSettingsFile(const char *controlResourceName, const char *pathID = NULL);

	// iterator for registered files
	int GetRegisteredControlSettingsFileCount();
	const char *GetRegisteredControlSettingsFileByIndex(int index);

	// dialog variables
	KeyValues *GetDialogVariables();

	// conditional keys for selectively reading keyvalues
	void ProcessConditionalKeys( KeyValues *pDat, KeyValues *pConditions );

protected:
	virtual bool CursorMoved(int x, int y, Panel *panel);
	virtual bool MouseDoublePressed(MouseCode code, Panel *panel);
	virtual bool KeyCodeTyped(KeyCode code, Panel *panel);
	virtual bool KeyCodeReleased(KeyCode code, Panel *panel );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual bool KeyTyped( wchar_t unichar, Panel *panel );

	virtual HCursor GetCursor(Panel *panel);

private:	
	void ApplySnap(Panel* panel);
	Panel *CreateBuildDialog();
	void ActivateBuildDialog();
	void DeleteAllControlsCreatedByControlSettingsFile();
	
	bool      _enabled;
	int       _snapX;
	int       _snapY;
	HCursor   _cursor_sizenwse;
	HCursor   _cursor_sizenesw;
	HCursor   _cursor_sizewe;
	HCursor   _cursor_sizens;
	HCursor   _cursor_sizeall;
	bool      _dragging;
	MouseCode _dragMouseCode;
	int       _dragStartPanelPos[2];
	int       _dragStartCursorPos[2];
	int		  _dragStartPanelSize[ 2 ];
	Panel   * _currentPanel;
	CUtlVector<PHandle> _panelDar;
	char	*m_pResourceName;
	char	*m_pResourcePathID;
	PHandle	 m_hBuildDialog;
	Panel   *m_pBuildContext;  // the panel from which the build dialog gets all the information it needs
	Panel   *m_pParentPanel;   // panel to create new controls in
	CUtlVector<PHandle> _controlGroup; // grouped panels
	CUtlVector<int> _groupDeltaX;	   // x offsets of panels in group from the selected panel
	CUtlVector<int> _groupDeltaY;	   // y offsets of panels in group from the selected panel
	Label	*_rulerNumber[4];  // 4 numbers to label rulers with
	bool	_showRulers;	   // toggles ruler display
	CUtlVector<CUtlSymbol> m_RegisteredControlSettingsFiles;

	friend class Panel;
};


//-----------------------------------------------------------------------------
// Handle to a build group
//-----------------------------------------------------------------------------
typedef CUtlHandle<BuildGroup> HBuildGroup;


} // namespace vgui

#endif // VGUI_BUILDGROUP_H
