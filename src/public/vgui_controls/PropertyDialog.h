//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROPERTYDIALOG_H
#define PROPERTYDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Simple frame that holds a property sheet
//-----------------------------------------------------------------------------
class PropertyDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( PropertyDialog, Frame );

public:
	PropertyDialog(Panel *parent, const char *panelName);
	~PropertyDialog();

	// returns a pointer to the PropertySheet this dialog encapsulates 
	virtual PropertySheet *GetPropertySheet();

	// wrapper for PropertySheet interface
	virtual void AddPage(Panel *page, const char *title);
	virtual Panel *GetActivePage();
	virtual void ResetAllData();
	virtual void ApplyChanges();

	// sets the text on the OK/Cancel buttons, overriding the default
	void SetOKButtonText(const char *text);
	void SetCancelButtonText(const char *text);
	void SetApplyButtonText(const char *text);

	// changes the visibility of the buttons
	void SetOKButtonVisible(bool state);
	void SetCancelButtonVisible(bool state);
	void SetApplyButtonVisible(bool state);

	/* MESSAGES SENT
		"ResetData"			- sent when page is loaded.  Data should be reloaded from document into controls.
		"ApplyChanges"		- sent when the OK / Apply button is pressed.  Changed data should be written into document.
	*/

protected:
	// Called when the OK button is pressed.  Simply closes the dialog.
	virtual bool OnOK(bool applyOnly);

	// called when the Cancel button is pressed
	virtual void OnCancel();

	// vgui overrides
	virtual void PerformLayout();
	virtual void OnCommand(const char *command);
	virtual void ActivateBuildMode();
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void RequestFocus(int direction = 0);

	MESSAGE_FUNC( OnApplyButtonEnable, "ApplyButtonEnable" );
	void EnableApplyButton(bool bEnable);
	
private:
	PropertySheet *_propertySheet;
	Button *_okButton;
	Button *_cancelButton;
	Button *_applyButton;

	CPanelAnimationVar( int, m_iSheetInsetBottom, "sheetinset_bottom", "32" );
};

}; // vgui

#endif // PROPERTYDIALOG_H
