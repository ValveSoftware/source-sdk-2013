//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

// prevent windows macros from messing with the class
#ifdef MessageBox
#undef MessageBox
#endif

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Popup discardable message box
//-----------------------------------------------------------------------------
class MessageBox : public Frame
{
	DECLARE_CLASS_SIMPLE( MessageBox, Frame );

public:
	// title - Text to be displayed in the title bar of the window
	// text - Text message in the message box
	// startMinimized - wether message box starts minimized. Starts invisible by default
	// parent - parent panel of the message box, by default it has no parent. This will keep the box visible until the OK button is pressed. 
	MessageBox(const char *title, const char *text, Panel *parent = NULL);
	MessageBox(const wchar_t *wszTitle, const wchar_t *wszText, Panel *parent = NULL);
	~MessageBox();

	// Put the message box into a modal state
	virtual void DoModal(Frame *pFrameOver = NULL);

	// make the message box appear and in a modeless state
	virtual void ShowWindow(Frame *pFrameOver = NULL);

	// Set a string command to be sent when the OK button is pressed
	// Use AddActionSignalTarget() to mark yourself as a recipient of the command
	virtual void SetCommand(const char *command);
	virtual void SetCommand(KeyValues *command);

	// Set the visibility of the OK button.
	virtual void SetOKButtonVisible(bool state);

	// Set the text on the OK button
	virtual void SetOKButtonText(const char *buttonText);
	virtual void SetOKButtonText(const wchar_t *wszButtonText);

	// Cancel button (off by default)
	void SetCancelButtonVisible(bool state);
 	void SetCancelButtonText(const char *buttonText);
	void SetCancelButtonText(const wchar_t *wszButtonText);
	void SetCancelCommand( KeyValues *command );

	// Toggles visibility of the close box.
	virtual void DisableCloseButton(bool state);

	virtual void OnCommand( const char *pCommand );

	// Shows the message box over the cursor
	void ShowMessageBoxOverCursor( bool bEnable );

protected:
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(IScheme *pScheme);

protected:
	Button				*m_pOkButton;
	Button				*m_pCancelButton;
	Label				*m_pMessageLabel;

private:
	MESSAGE_FUNC( OnShutdownRequest, "ShutdownRequest" );

	void Init();
	
	KeyValues *m_OkCommand;
	KeyValues *m_CancelCommand;
	vgui::Frame *m_pFrameOver;
	bool m_bNoAutoClose : 1;
	bool m_bShowMessageBoxOverCursor : 1;
};

} // namespace vgui


#endif // MESSAGEBOX_H
