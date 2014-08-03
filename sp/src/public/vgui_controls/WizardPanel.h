//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WIZARDPANEL_H
#define WIZARDPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>

namespace vgui
{

class WizardSubPanel;

//-----------------------------------------------------------------------------
// Purpose: Type of dialog that supports moving back and forth through a series
//			of sub-dialogs, WizardSubPanels
//-----------------------------------------------------------------------------
class WizardPanel : public Frame
{
	DECLARE_CLASS_SIMPLE( WizardPanel, Frame );

public:
	WizardPanel(Panel *parent, const char *panelName);
	~WizardPanel();

	// Start the wizard, starting with the startPanel
	virtual void Run(WizardSubPanel *startPanel);

	// Called when the buttons are pressed
	// WizardSubPanels can also call these functions to simulate a button being pressed
	MESSAGE_FUNC( OnNextButton, "NextButton" );
	MESSAGE_FUNC( OnPrevButton, "PrevButton" );
	MESSAGE_FUNC( OnFinishButton, "FinishButton" );
	MESSAGE_FUNC( OnCancelButton, "CancelButton" );

	// sets whether or not a button is enabled
	// this state is managed, and will be reset whenever going to a new page
	virtual void SetNextButtonEnabled(bool state);
	virtual void SetPrevButtonEnabled(bool state);
	virtual void SetFinishButtonEnabled(bool state);
	virtual void SetCancelButtonEnabled(bool state);

	// sets whether or not a button is visible
	// this state is unmanaged, the user needs to ensure that the buttons state
	// is correct when going both back and prev through the wizard
	virtual void SetNextButtonVisible(bool state);
	virtual void SetPrevButtonVisible(bool state);
	virtual void SetFinishButtonVisible(bool state);
	virtual void SetCancelButtonVisible(bool state);

	// sets the text for a button
	// setting the text to be NULL resets the text to it's default state
	// this state is unmanaged, the user needs to ensure that the buttons state
	// is correct when going both back and prev through the wizard
	virtual void SetNextButtonText(const char *text);
	virtual void SetPrevButtonText(const char *text);
	virtual void SetFinishButtonText(const char *text);
	virtual void SetCancelButtonText(const char *text);

	// general wizard state for all the subpanels to access
	virtual KeyValues *GetWizardData();

	// recalculates where the key focus should be in the wizard
	virtual void ResetKeyFocus();
	virtual void ResetDefaultButton();

	// resets the sub panel history for the control
	virtual void ResetHistory(); 

	// returns a page by name
	virtual WizardSubPanel *GetSubPanelByName(const char *pageName);
	
	virtual void ShowButtons(bool state);
	virtual void GetClientArea(int &x, int &y, int &wide, int &tall);

protected:
	MESSAGE_FUNC_PTR( InternalActivateNextSubPanel, "ActivateNextSubPanel", panel )
	{
		ActivateNextSubPanel( (WizardSubPanel *)panel );
	}

	virtual void ActivateNextSubPanel(WizardSubPanel *subPanel);
	virtual void ActivatePrevSubPanel();
	virtual void CreateButtons();
	virtual void RecalculateTabOrdering();
	virtual vgui::WizardSubPanel *GetCurrentSubPanel()	{ return _currentSubPanel; }

	// overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(IScheme *pScheme);

	// reroute build messages to the currently active sub panel
	virtual void ActivateBuildMode();

	// close maps to the cancel button
	virtual void OnClose();
	virtual void OnCommand(const char *command);
	virtual void OnCloseFrameButtonPressed();

private:
	WizardSubPanel *FindNextValidSubPanel(WizardSubPanel *currentPanel);

	Button *_prevButton;
	Button *_nextButton;
	Button *_cancelButton;
	Button *_finishButton;

	WizardSubPanel *_currentSubPanel;
	KeyValues *_currentData;

	Dar<WizardSubPanel *> _subPanelStack;  // contains a list of all the subpanels (not including the current one)

	bool _showButtons;
};

} // namespace vgui


#endif // WIZARDPANEL_H
