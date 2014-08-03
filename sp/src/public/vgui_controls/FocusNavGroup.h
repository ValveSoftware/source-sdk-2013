//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef FOCUSNAVGROUP_H
#define FOCUSNAVGROUP_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{

class Panel;

//-----------------------------------------------------------------------------
// Purpose: Handles navigation through a set of panels, with tab order & hotkeys
//-----------------------------------------------------------------------------
class FocusNavGroup
{
public:
	FocusNavGroup(Panel *panel);
	~FocusNavGroup();
	virtual Panel *GetDefaultPanel();	// returns a pointer to the panel with the default focus

	virtual void SetDefaultButton(Panel *panel);	// sets which panel should receive input when ENTER is hit
	virtual VPANEL GetDefaultButton();				    // panel which receives default input when ENTER is hit, if current focused item cannot accept ENTER
	virtual VPANEL GetCurrentDefaultButton();			// panel which receives input when ENTER is hit
	virtual Panel *FindPanelByHotkey(wchar_t key);		// finds the panel which is activated by the specified key
	virtual bool RequestFocusPrev(VPANEL panel = NULL); // if panel is NULL, then the tab increment is based last known panel that had key focus
	virtual bool RequestFocusNext(VPANEL panel = NULL);	

	virtual Panel *GetCurrentFocus();
	virtual VPANEL SetCurrentFocus(VPANEL panel, VPANEL defaultPanel);  // returns the Default panel

	// sets the panel that owns this FocusNavGroup to be the root in the focus traversal heirarchy
	// focus change via KEY_TAB will only travel to children of this main panel
	virtual void SetFocusTopLevel(bool state);

    virtual void SetCurrentDefaultButton(VPANEL panel, bool sendCurrentDefaultButtonMessage = true);  
private:
	bool CanButtonBeDefault(VPANEL panel);

	VPanelHandle _defaultButton;
	VPanelHandle _currentDefaultButton;
	VPanelHandle _currentFocus;

	Panel *_mainPanel;
	bool _topLevelFocus;
};

} // namespace vgui

#endif // FOCUSNAVGROUP_H
