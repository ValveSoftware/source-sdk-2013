//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <assert.h>

#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IPanel.h>
#include <vgui/VGUI.h>
#include <KeyValues.h>
#include <tier0/dbg.h>

#include <vgui_controls/Controls.h>
#include <vgui_controls/FocusNavGroup.h>
#include <vgui_controls/Panel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  : *panel - parent panel
//-----------------------------------------------------------------------------
FocusNavGroup::FocusNavGroup(Panel *panel) : _mainPanel(panel)
{
	_currentFocus = NULL;
	_topLevelFocus = false;
	_defaultButton = NULL;
    _currentDefaultButton = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
FocusNavGroup::~FocusNavGroup()
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets the focus to the previous panel in the tab order
// Input  : *panel - panel currently with focus
//-----------------------------------------------------------------------------
bool FocusNavGroup::RequestFocusPrev(VPANEL panel)
{
	if(panel==0)
		return false;

	_currentFocus = NULL;
	int newPosition = 9999999;
	if (panel)
	{
		newPosition = ipanel()->GetTabPosition(panel);
	}

	bool bFound = false;
	bool bRepeat = true;
	Panel *best = NULL;
	while (1)
	{
		newPosition--;
		if (newPosition > 0)
		{
			int bestPosition = 0;

			// look for the next tab position
			for (int i = 0; i < _mainPanel->GetChildCount(); i++)
			{
				Panel *child = _mainPanel->GetChild(i);
				if (child && child->IsVisible() && child->IsEnabled() && child->GetTabPosition())
				{
					int tabPosition = child->GetTabPosition();
					if (tabPosition == newPosition)
					{
						// we've found the right tab
						best = child;
						bestPosition = newPosition;

						// don't loop anymore since we've found the correct panel
						break;
					}
					else if (tabPosition < newPosition && tabPosition > bestPosition)
					{
						// record the match since this is the closest so far
						bestPosition = tabPosition;
						best = child;
					}
				}
			}

			if (!bRepeat)
				break;

			if (best)
				break;
		}
		else
		{
			// reset new position for next loop
			newPosition = 9999999;
		}

		// haven't found an item

		if (!_topLevelFocus)
		{
			// check to see if we should push the focus request up
			if (_mainPanel->GetVParent() && _mainPanel->GetVParent() != surface()->GetEmbeddedPanel())
			{
				// we're not a top level panel, so forward up the request instead of looping
				if (ipanel()->RequestFocusPrev(_mainPanel->GetVParent(), _mainPanel->GetVPanel()))
				{
					bFound = true;
					SetCurrentDefaultButton(NULL);
					break;
				}
			}
		}

		// not found an item, loop back
		newPosition = 9999999;
		bRepeat = false;
	}

	if (best)
	{
		_currentFocus = best->GetVPanel();
		best->RequestFocus(-1);
		bFound = true;

        if (!CanButtonBeDefault(best->GetVPanel()))
        {
            if (_defaultButton)
            {
                SetCurrentDefaultButton(_defaultButton);
            }
			else
			{
				SetCurrentDefaultButton(NULL);

				// we need to ask the parent to set its default button
				if (_mainPanel->GetVParent())
				{
					ivgui()->PostMessage(_mainPanel->GetVParent(), new KeyValues("FindDefaultButton"), NULL);
				}
			}
        }
        else
        {
            SetCurrentDefaultButton(best->GetVPanel());
        }
	}
	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the focus to the previous panel in the tab order
// Input  : *panel - panel currently with focus
//-----------------------------------------------------------------------------
bool FocusNavGroup::RequestFocusNext(VPANEL panel)
{
	// basic recursion guard, in case user has set up a bad focus hierarchy
	static int stack_depth = 0;
	stack_depth++;

	_currentFocus = NULL;
	int newPosition = 0;
	if (panel)
	{
		newPosition = ipanel()->GetTabPosition(panel);
	}

	bool bFound = false;
	bool bRepeat = true;
	Panel *best = NULL;
	while (1)
	{
		newPosition++;
		int bestPosition = 999999;

		// look for the next tab position
		for (int i = 0; i < _mainPanel->GetChildCount(); i++)
		{
			Panel *child = _mainPanel->GetChild(i);
			if ( !child )
				continue;

			if (child && child->IsVisible() && child->IsEnabled() && child->GetTabPosition())
			{
				int tabPosition = child->GetTabPosition();
				if (tabPosition == newPosition)
				{
					// we've found the right tab
					best = child;
					bestPosition = newPosition;

					// don't loop anymore since we've found the correct panel
					break;
				}
				else if (tabPosition > newPosition && tabPosition < bestPosition)
				{
					// record the match since this is the closest so far
					bestPosition = tabPosition;
					best = child;
				}
			}
		}

		if (!bRepeat)
			break;

		if (best)
			break;

		// haven't found an item

		// check to see if we should push the focus request up
		if (!_topLevelFocus)
		{
			if (_mainPanel->GetVParent() && _mainPanel->GetVParent() != surface()->GetEmbeddedPanel())
			{
				// we're not a top level panel, so forward up the request instead of looping
				if (stack_depth < 15)
				{
					if (ipanel()->RequestFocusNext(_mainPanel->GetVParent(), _mainPanel->GetVPanel()))
					{
						bFound = true;
						SetCurrentDefaultButton(NULL);
						break;
					}

					// if we find one then we break, otherwise we loop
				}
			}
		}
		
		// loop back
		newPosition = 0;
		bRepeat = false;
	}

	if (best)
	{
		_currentFocus = best->GetVPanel();
		best->RequestFocus(1);
		bFound = true;

        if (!CanButtonBeDefault(best->GetVPanel()))
        {
            if (_defaultButton)
			{
                SetCurrentDefaultButton(_defaultButton);
			}
			else
			{
				SetCurrentDefaultButton(NULL);

				// we need to ask the parent to set its default button
				if (_mainPanel->GetVParent())
				{
					ivgui()->PostMessage(_mainPanel->GetVParent(), new KeyValues("FindDefaultButton"), NULL);
				}
			}
        }
        else
        {
            SetCurrentDefaultButton(best->GetVPanel());
        }
	}

	stack_depth--;
	return bFound;
}

//-----------------------------------------------------------------------------
// Purpose: sets the panel that owns this FocusNavGroup to be the root in the focus traversal heirarchy
//-----------------------------------------------------------------------------
void FocusNavGroup::SetFocusTopLevel(bool state)
{
	_topLevelFocus = state;
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
//-----------------------------------------------------------------------------
void FocusNavGroup::SetDefaultButton(Panel *panel)
{
	VPANEL vpanel = panel ? panel->GetVPanel() : NULL;
	if ( vpanel == _defaultButton.Get() )
		return;

//	Assert(CanButtonBeDefault(vpanel));

	_defaultButton = vpanel;
	SetCurrentDefaultButton(_defaultButton);
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
//-----------------------------------------------------------------------------
void FocusNavGroup::SetCurrentDefaultButton(VPANEL panel, bool sendCurrentDefaultButtonMessage)
{
	if (panel == _currentDefaultButton.Get())
		return;

	if ( sendCurrentDefaultButtonMessage && _currentDefaultButton.Get() != 0)
	{
		ivgui()->PostMessage(_currentDefaultButton, new KeyValues("SetAsCurrentDefaultButton", "state", 0), NULL);
	}

	_currentDefaultButton = panel;

	if ( sendCurrentDefaultButtonMessage && _currentDefaultButton.Get() != 0)
	{
		ivgui()->PostMessage(_currentDefaultButton, new KeyValues("SetAsCurrentDefaultButton", "state", 1), NULL);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
//-----------------------------------------------------------------------------
VPANEL FocusNavGroup::GetCurrentDefaultButton()
{
	return _currentDefaultButton;
}

//-----------------------------------------------------------------------------
// Purpose: sets panel which receives input when ENTER is hit
//-----------------------------------------------------------------------------
VPANEL FocusNavGroup::GetDefaultButton()
{
	return _defaultButton;
}

//-----------------------------------------------------------------------------
// Purpose: finds the panel which is activated by the specified key
// Input  : code - the keycode of the hotkey
// Output : Panel * - NULL if no panel found
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::FindPanelByHotkey(wchar_t key)
{
	for (int i = 0; i < _mainPanel->GetChildCount(); i++)
	{
		Panel *child = _mainPanel->GetChild(i);
		if ( !child )
			continue;

		Panel *hot = child->HasHotkey(key);
		if (hot && hot->IsVisible() && hot->IsEnabled())
		{
			return hot;
		}
	}
	
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetDefaultPanel()
{
	for (int i = 0; i < _mainPanel->GetChildCount(); i++)
	{
		Panel *child = _mainPanel->GetChild(i);
		if ( !child )
			continue;

		if (child->GetTabPosition() == 1)
		{
			return child;
		}
	}

	return NULL;	// no specific panel set
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Panel *FocusNavGroup::GetCurrentFocus()
{
	return _currentFocus ? ipanel()->GetPanel(_currentFocus, vgui::GetControlsModuleName()) : NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the current focus
//-----------------------------------------------------------------------------
VPANEL FocusNavGroup::SetCurrentFocus(VPANEL focus, VPANEL defaultPanel)
{
	_currentFocus = focus;

    // if we haven't found a default panel yet, let's see if we know of one
    if (defaultPanel == 0)
    {
        // can this focus itself by the default
        if (CanButtonBeDefault(focus))
        {
            defaultPanel = focus;
        }
        else if (_defaultButton)        // do we know of a default button
        {
            defaultPanel = _defaultButton;
        }
    }

    SetCurrentDefaultButton(defaultPanel);
    return defaultPanel;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the specified panel can be the default
//-----------------------------------------------------------------------------
bool FocusNavGroup::CanButtonBeDefault(VPANEL panel)
{
	if( panel == 0 )
		return false;

	KeyValues *data = new KeyValues("CanBeDefaultButton");

	bool bResult = false;
	if (ipanel()->RequestInfo(panel, data))
	{
		bResult = (data->GetInt("result") == 1);
	}
	data->deleteThis();
	return bResult;
}