//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/BuildGroup.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/WizardPanel.h>
#include <vgui_controls/WizardSubPanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
WizardPanel::WizardPanel(Panel *parent, const char *panelName) : Frame(parent, panelName)
{
	_currentSubPanel = NULL;
	_currentData = new KeyValues("WizardData");
	_showButtons = true;


	SetSizeable(false);

	CreateButtons();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
WizardPanel::~WizardPanel()
{
	if (_currentData)
	{
		_currentData->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	// resize the sub panel to fit in the Client area
	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);

	if (_currentSubPanel && _currentSubPanel->isNonWizardPanel())
	{
		// just have the subpanel cover the full size
		_currentSubPanel->SetBounds(x, y, wide, tall);
		_cancelButton->SetVisible(false);
		_prevButton->SetVisible(false);
		_nextButton->SetVisible(false);
		_finishButton->SetVisible(false);
	}
	else
	{
		// make room for the buttons at bottom
		if (_currentSubPanel) 
		{
			if( _showButtons )
			{
				_currentSubPanel->SetBounds(x, y, wide, tall - 35);
			}
			else
			{
				_currentSubPanel->SetBounds(x, y, wide, tall);
			}
		}

		// align the buttons to the right hand side
		GetSize(wide, tall);


		int bwide, btall;
		_cancelButton->GetSize(bwide, btall);
		
		x = wide - (20 + bwide);
		y = tall - (12 + btall);

		_cancelButton->SetPos(x, y);
		x -= (20 + bwide);

		// only display one of the next or finish buttons (and only if both are visible)
		if ( _showButtons )
		{
			if (_finishButton->IsEnabled() )
			{
				_nextButton->SetVisible(false);
				_finishButton->SetVisible(true);
				_finishButton->SetPos(x, y);
			}
			else
			{
				_nextButton->SetVisible(true);
				_finishButton->SetVisible(false);
				_nextButton->SetPos(x, y);
			}
		}

		x -= (1 + bwide);
		_prevButton->SetPos(x, y);

		ResetDefaultButton();
	}
}


//-----------------------------------------------------------------------------
// Purpose: if we don't show buttons then let the sub panel occupy the whole screen
//-----------------------------------------------------------------------------
void WizardPanel::GetClientArea(int &x, int &y, int &wide, int &tall)
{
	if( _showButtons )
	{
		BaseClass::GetClientArea( x, y, wide, tall );
	}
	else
	{
		x = 0;
		y = 0;
		GetSize( wide, tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::Run(WizardSubPanel *startPanel)
{
	// skip over sub panels if they don't want to be displayed
	startPanel = FindNextValidSubPanel(startPanel);

	// show it
	ActivateNextSubPanel(startPanel);

	// make sure we're set up and Run the first panel
	Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::ActivateBuildMode()
{
	// no subpanel, no build mode
	if (!_currentSubPanel)
		return;

	_currentSubPanel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::ResetDefaultButton()
{
	// work out which is the default button
	if (_nextButton->IsEnabled())
	{
		_nextButton->SetAsDefaultButton(true);
	}
	else if (_finishButton->IsEnabled())
	{
		_finishButton->SetAsDefaultButton(true);
	}
	else if (_prevButton->IsEnabled())
	{
		_prevButton->SetAsDefaultButton(true);
	}
	/* Don't ever set the cancel button as the default, as it is too easy for users to quit the wizard without realizing
	else if (_cancelButton->IsEnabled())
	{
		_cancelButton->SetAsDefaultButton(true);
	}
	*/
	
	// reset them all (this may not be necessary)
	_nextButton->InvalidateLayout();
	_prevButton->InvalidateLayout();
	_cancelButton->InvalidateLayout();
	_finishButton->InvalidateLayout();

	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::ResetKeyFocus()
{
	// set the focus on the default
	FocusNavGroup &navGroup = GetFocusNavGroup();
	Panel *def = navGroup.GetDefaultPanel();
	if (def)
	{
		if (def->IsEnabled() && def->IsVisible())
		{
			def->RequestFocus();
		}
		else
		{
			def->RequestFocusNext();
		}
	}

	ResetDefaultButton();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::CreateButtons()
{
	_prevButton = new Button(this, "PrevButton", "");
	_nextButton = new Button(this, "NextButton", "");
	_cancelButton = new Button(this, "CancelButton", "");
	_finishButton = new Button(this, "FinishButton", "");

	_prevButton->SetCommand(new KeyValues("PrevButton"));
	_nextButton->SetCommand(new KeyValues("NextButton"));
	_cancelButton->SetCommand(new KeyValues("CancelButton"));
	_finishButton->SetCommand(new KeyValues("FinishButton"));

	SetNextButtonText(NULL);
	SetPrevButtonText(NULL);
	SetFinishButtonText(NULL);
	SetCancelButtonText(NULL);

	_prevButton->SetSize(82, 24);
	_nextButton->SetSize(82, 24);
	_cancelButton->SetSize(82, 24);
	_finishButton->SetSize(82, 24);
}

//-----------------------------------------------------------------------------
// Purpose: clears all previous history
//-----------------------------------------------------------------------------
void WizardPanel::ResetHistory()
{
	_subPanelStack.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::ActivateNextSubPanel(WizardSubPanel *subPanel)
{
	// get rid of previous panel
	WizardSubPanel *prevPanel = _currentSubPanel;
	if (prevPanel && prevPanel->ShouldDisplayPanel())
	{
		// hide
		prevPanel->SetVisible(false);

		// push onto history stack
		_subPanelStack.AddElement(_currentSubPanel);
	}

	// reenable all buttons, returning them to their default state
	_prevButton->SetEnabled(true);
	_nextButton->SetEnabled(true);
	_cancelButton->SetEnabled(true);
	_finishButton->SetEnabled(true);
	if ( _showButtons ) 
	{
		_prevButton->SetVisible(true);
		_cancelButton->SetVisible(true);
	}

	// set up new subpanel
	_currentSubPanel = subPanel;
	_currentSubPanel->SetParent(this);
	_currentSubPanel->SetVisible(true);

	_currentSubPanel->SetWizardPanel(this);
	_currentSubPanel->OnDisplayAsNext();
	_currentSubPanel->OnDisplay();
	_currentSubPanel->InvalidateLayout(false);

	SETUP_PANEL( _currentSubPanel );
	int wide, tall;
	if ( _currentSubPanel->GetDesiredSize(wide, tall) )
	{
		SetSize(wide, tall);
	}

	if (!prevPanel)
	{
		// no previous panel, so disable the back button
		_prevButton->SetEnabled(false);		
	}

	_currentSubPanel->RequestFocus();

	RecalculateTabOrdering();
	InvalidateLayout(false);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Pops the last panel off the stack and runs it
//-----------------------------------------------------------------------------
void WizardPanel::ActivatePrevSubPanel()
{
	_currentSubPanel->SetVisible(false);

	WizardSubPanel *prevPanel = NULL;
	if (_subPanelStack.GetCount())
	{
		// check to see if we need to jump back to a previous sub panel
		WizardSubPanel *searchPanel = _currentSubPanel->GetPrevSubPanel();
		if (searchPanel && _subPanelStack.HasElement(searchPanel))
		{
			// keep poping the stack till we find it
			while (_subPanelStack.GetCount() && prevPanel != searchPanel)
			{
				prevPanel = _subPanelStack[_subPanelStack.GetCount() - 1];
				_subPanelStack.RemoveElementAt(_subPanelStack.GetCount() - 1);
			}
		}
		else
		{
			// just get the last one
			prevPanel = _subPanelStack[_subPanelStack.GetCount() - 1];
			_subPanelStack.RemoveElementAt(_subPanelStack.GetCount() - 1);
		}
	}

	if (!prevPanel)
	{
		ivgui()->DPrintf2("Error: WizardPanel::ActivatePrevSubPanel(): no previous panel to go back to\n");
		return;
	}

	// hide old panel
	_currentSubPanel->SetVisible(false);
	
	// reenable all buttons, returning them to their default state
	_prevButton->SetEnabled(true);
	_nextButton->SetEnabled(true);
	_cancelButton->SetEnabled(true);
	_finishButton->SetEnabled(true);

	// Activate new panel
	_currentSubPanel = prevPanel;
	_currentSubPanel->RequestFocus();
	_currentSubPanel->SetWizardPanel(this);
	_currentSubPanel->OnDisplayAsPrev();
	_currentSubPanel->OnDisplay();
	_currentSubPanel->InvalidateLayout(false);

	SETUP_PANEL( _currentSubPanel );
	int wide, tall;
	if ( _currentSubPanel->GetDesiredSize(wide, tall) )
	{
		SetSize(wide, tall);
	}

	// show the previous panel, but don't Activate it (since it should show just what it was previously)
	_currentSubPanel->SetVisible(true);

	if (!_subPanelStack.GetCount())
	{
		// no previous panel, so disable the back button
		_prevButton->SetEnabled(false);		
	}

	RecalculateTabOrdering();
	InvalidateLayout(false);
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the new tab ordering
//-----------------------------------------------------------------------------
void WizardPanel::RecalculateTabOrdering()
{
	if (_currentSubPanel)
	{
		_currentSubPanel->SetTabPosition(1);
	}
	_prevButton->SetTabPosition(2);
	_nextButton->SetTabPosition(3);
	_finishButton->SetTabPosition(4);
	_cancelButton->SetTabPosition(5);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetNextButtonEnabled(bool state)
{
	if (_nextButton->IsEnabled() != state)
	{
		_nextButton->SetEnabled(state);
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetPrevButtonEnabled(bool state)
{
	if (_prevButton->IsEnabled() != state)
	{
		_prevButton->SetEnabled(state);
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetFinishButtonEnabled(bool state)
{
	if (_finishButton->IsEnabled() != state)
	{
		_finishButton->SetEnabled(state);
		InvalidateLayout(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetCancelButtonEnabled(bool state)
{
	if (_cancelButton->IsEnabled() != state)
	{
		_cancelButton->SetEnabled(state);
		InvalidateLayout(false);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetNextButtonVisible(bool state)
{
	_nextButton->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetPrevButtonVisible(bool state)
{
	_prevButton->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetFinishButtonVisible(bool state)
{
	_finishButton->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetCancelButtonVisible(bool state)
{
	_cancelButton->SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetNextButtonText(const char *text)
{
	if (text)
	{
		_nextButton->SetText(text);
	}
	else
	{
		_nextButton->SetText("#WizardPanel_Next");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetPrevButtonText(const char *text)
{
	if (text)
	{
		_prevButton->SetText(text);
	}
	else
	{
		_prevButton->SetText("#WizardPanel_Back");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetFinishButtonText(const char *text)
{
	if (text)
	{
		_finishButton->SetText(text);
	}
	else
	{
		_finishButton->SetText("#WizardPanel_Finish");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::SetCancelButtonText(const char *text)
{
	if (text)
	{
		_cancelButton->SetText(text);
	}
	else
	{
		_cancelButton->SetText("#WizardPanel_Cancel");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Finds the next panel that wants to be shown
//-----------------------------------------------------------------------------
WizardSubPanel *WizardPanel::FindNextValidSubPanel(WizardSubPanel *currentPanel)
{
	// skip over sub panels if they don't want to be displayed
	while (currentPanel)
	{
		currentPanel->SetWizardPanel(this);
		if (currentPanel->ShouldDisplayPanel())
			break;
		
		// ok the panel wants to be skipped, so skip ahead
		currentPanel = currentPanel->GetNextSubPanel();
	}

	return currentPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Advances to the next panel
//-----------------------------------------------------------------------------
void WizardPanel::OnNextButton()
{
	if (_currentSubPanel)
	{
		bool shouldAdvance = _currentSubPanel->OnNextButton();
		if (shouldAdvance)
		{
			WizardSubPanel *nextPanel = FindNextValidSubPanel(_currentSubPanel->GetNextSubPanel());

			if (nextPanel)
			{
				KeyValues *kv = new KeyValues("ActivateNextSubPanel");
				kv->SetPtr("panel", nextPanel);
				ivgui()->PostMessage(GetVPanel(), kv, GetVPanel());
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Retreats to the previous panel
//-----------------------------------------------------------------------------
void WizardPanel::OnPrevButton()
{
	bool shouldRetreat = true;
	if (_currentSubPanel)
	{
		shouldRetreat = _currentSubPanel->OnPrevButton();
	}

	if (shouldRetreat)
	{
		ActivatePrevSubPanel();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::OnFinishButton()
{
	if (_currentSubPanel && _currentSubPanel->OnFinishButton())
	{
		// hide ourselves away
		BaseClass::OnClose();

		// automatically delete ourselves if marked to do so
		if (IsAutoDeleteSet())
		{
			MarkForDeletion();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void WizardPanel::OnCancelButton()
{
	if (_currentSubPanel && _currentSubPanel->OnCancelButton())
	{
		// hide ourselves away
		BaseClass::OnClose();
		if (IsAutoDeleteSet())
		{
			MarkForDeletion();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: command handler for catching escape key presses
//-----------------------------------------------------------------------------
void WizardPanel::OnCommand(const char *command)
{
	if (!stricmp(command, "Cancel"))
	{
		if (_cancelButton->IsEnabled())
		{
			_cancelButton->DoClick();
		}
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Maps close button to cancel button
//-----------------------------------------------------------------------------
void WizardPanel::OnClose()
{
	if (_cancelButton->IsEnabled())
	{
		_cancelButton->DoClick();
	}
	else if (_finishButton->IsEnabled())
	{
		_finishButton->DoClick();
	}

	// don't chain back
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *WizardPanel::GetWizardData()
{
	return _currentData;
}


//-----------------------------------------------------------------------------
// Purpose: whether to show the next,prev,finish and cancel buttons
//-----------------------------------------------------------------------------
void WizardPanel::ShowButtons(bool state)
{ 
	_showButtons = state; 	// hide the wizard panel buttons
	SetNextButtonVisible( state );
	SetPrevButtonVisible( state );
	SetFinishButtonVisible( state );
	SetCancelButtonVisible( state );
}

//-----------------------------------------------------------------------------
// Purpose: filters close buttons
//-----------------------------------------------------------------------------
void WizardPanel::OnCloseFrameButtonPressed()
{
	// only allow close if the cancel button is enabled
	if (_cancelButton->IsEnabled())
	{
		BaseClass::OnCloseFrameButtonPressed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns a page by name
//-----------------------------------------------------------------------------
WizardSubPanel *WizardPanel::GetSubPanelByName(const char *pageName)
{
	return dynamic_cast<WizardSubPanel *>(FindChildByName(pageName));
}
