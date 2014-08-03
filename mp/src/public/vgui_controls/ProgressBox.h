//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

// prevent windows macros from messing with the class
#ifdef ProgressBox
#undef ProgressBox
#endif

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Popup discardable message box
//-----------------------------------------------------------------------------
class ProgressBox : public Frame
{
	DECLARE_CLASS_SIMPLE( ProgressBox, Frame );

public:
	// title - Text to be displayed in the title bar of the window
	// text - Text message in the message box
	// parent - parent panel of the message box, by default it has no parent.
	ProgressBox(const char *title, const char *text, const char *pszUnknownTimeString, Panel *parent = NULL);
	ProgressBox(const wchar_t *wszTitle, const wchar_t *wszText, const wchar_t *wszUnknownTimeString, Panel *parent = NULL);
	~ProgressBox();

	// Put the message box into a modal state
	virtual void DoModal(Frame *pFrameOver = NULL);

	// make the message box appear and in a modeless state
	virtual void ShowWindow(Frame *pFrameOver = NULL);

	// updates progress bar, range [0, 1]
	virtual void SetProgress(float progress);

	// sets the info text
	virtual void SetText(const char *text);

	// toggles visibility of the close box.
	virtual void SetCancelButtonVisible(bool state);

	// toggles the enabled state of the cancel button (for if it needs to be disabled part way through a process)
	virtual void SetCancelButtonEnabled(bool state);

	/* custom messages:

		"ProgressBoxCancelled"
			sent if the user pressed the cancel button (must be enabled & visible for this to happen)

	*/

protected:
	virtual void PerformLayout();
	virtual void OnClose();
	virtual void OnCloseFrameButtonPressed();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnThink();
	virtual void OnCommand(const char *command);
	virtual void OnTick();

	// called when the update has been cancelled
	virtual void OnCancel();

private:
	MESSAGE_FUNC( OnShutdownRequest, "ShutdownRequest" );
	void Init();
	void UpdateTitle();

	Label *m_pMessageLabel;
	ProgressBar *m_pProgressBar;
	Button *m_pCancelButton;

	wchar_t m_wszTitleString[128];
	wchar_t m_wcsInfoString[128];
	wchar_t m_wszUnknownTimeString[128];

	float m_flFirstProgressUpdate;
	float m_flLastProgressUpdate;
	float m_flCurrentProgress;
};

} // namespace vgui


#endif // PROGRESSBOX_H
