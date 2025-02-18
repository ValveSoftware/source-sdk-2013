//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PropertyDialog::PropertyDialog(Panel *parent, const char *panelName) : Frame(parent, panelName)
{
	// create the property sheet
	_propertySheet = new PropertySheet(this, "Sheet");
	_propertySheet->AddActionSignalTarget(this);
	_propertySheet->SetTabPosition(1);

	// add the buttons
	_okButton = new Button(this, "OKButton", "#PropertyDialog_OK");
	_okButton->AddActionSignalTarget(this);
	_okButton->SetTabPosition(2);
	_okButton->SetCommand("OK");
	GetFocusNavGroup().SetDefaultButton(_okButton);

	_cancelButton = new Button(this, "CancelButton", "#PropertyDialog_Cancel");
	_cancelButton->AddActionSignalTarget(this);
	_cancelButton->SetTabPosition(3);
	_cancelButton->SetCommand("Cancel");

	_applyButton = new Button(this, "ApplyButton", "#PropertyDialog_Apply");
	_applyButton->AddActionSignalTarget(this);
	_applyButton->SetTabPosition(4);
	_applyButton->SetVisible(false);		// default to not visible
    _applyButton->SetEnabled(false);        // default to not enabled
	_applyButton->SetCommand("Apply");

	SetSizeable(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PropertyDialog::~PropertyDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the PropertySheet this dialog encapsulates
// Output : PropertySheet *
//-----------------------------------------------------------------------------
PropertySheet *PropertyDialog::GetPropertySheet()
{
	return _propertySheet;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a pointer to the currently active page.
// Output : Panel
//-----------------------------------------------------------------------------
Panel *PropertyDialog::GetActivePage()
{
	return _propertySheet->GetActivePage();
}

//-----------------------------------------------------------------------------
// Purpose: Wrapped function
//-----------------------------------------------------------------------------
void PropertyDialog::AddPage(Panel *page, const char *title)
{
	_propertySheet->AddPage(page, title);
}

//-----------------------------------------------------------------------------
// Purpose: reloads the data in all the property page
//-----------------------------------------------------------------------------
void PropertyDialog::ResetAllData()
{
	_propertySheet->ResetAllData();
}

//-----------------------------------------------------------------------------
// Purpose: Applies any changes
//-----------------------------------------------------------------------------
void PropertyDialog::ApplyChanges()
{
	OnCommand("Apply");
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the sheet
//-----------------------------------------------------------------------------
void PropertyDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	int iBottom = QuickPropScale( m_iSheetInsetBottom );

	int x, y, wide, tall;
	GetClientArea(x, y, wide, tall);
	_propertySheet->SetBounds(x, y, wide, tall - iBottom);


	// move the buttons to the bottom-right corner
	int xpos = x + wide - QuickPropScale( 80 );
	int ypos = tall + y - QuickPropScale( 28 );

	if (_applyButton->IsVisible())
	{
		_applyButton->SetBounds(xpos, ypos, QuickPropScale( 72 ), QuickPropScale( 24 ) );
		xpos -= QuickPropScale( 80 );
	}

	if (_cancelButton->IsVisible())
	{
		_cancelButton->SetBounds(xpos, ypos, QuickPropScale( 72 ), QuickPropScale( 24 ) );
		xpos -= QuickPropScale( 80 );
	}

	_okButton->SetBounds(xpos, ypos, QuickPropScale( 72 ), QuickPropScale( 24 ) );

	_propertySheet->InvalidateLayout(); // tell the propertysheet to redraw!
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Handles command text from the buttons
//-----------------------------------------------------------------------------
void PropertyDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "OK"))
	{
		if ( OnOK(false) )
		{
			OnCommand("Close");
		}
		_applyButton->SetEnabled(false);
	}
	else if (!stricmp(command, "Cancel"))
	{
		OnCancel();
		Close();
	}
	else if (!stricmp(command, "Apply"))
	{
		OnOK(true);
		_applyButton->SetEnabled(false);
		InvalidateLayout();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when the Cancel button is pressed
//-----------------------------------------------------------------------------
void PropertyDialog::OnCancel()
{
	// designed to be overridden
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : code - 
//-----------------------------------------------------------------------------
void PropertyDialog::OnKeyCodeTyped(KeyCode code)
{
	// this has been removed, since it conflicts with how we use the escape key in the game
//	if (code == KEY_ESCAPE)
//	{
//		OnCommand("Cancel");
//	}
//	else
	{
		BaseClass::OnKeyCodeTyped(code);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
bool PropertyDialog::OnOK(bool applyOnly)
{
    // the sheet should have the pages apply changes before we tell the world
	_propertySheet->ApplyChanges();

    // this should tell anybody who's watching us that we're done
	PostActionSignal(new KeyValues("ApplyChanges"));

	// default to closing
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Overrides build mode so it edits the sub panel
//-----------------------------------------------------------------------------
void PropertyDialog::ActivateBuildMode()
{
	// no subpanel, no build mode
	EditablePanel *panel = dynamic_cast<EditablePanel *>(GetActivePage());
	if (!panel)
		return;

	panel->ActivateBuildMode();
}

//-----------------------------------------------------------------------------
// Purpose: sets the text on the OK/Cancel buttons, overriding the default
//-----------------------------------------------------------------------------
void PropertyDialog::SetOKButtonText(const char *text)
{
	_okButton->SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: sets the text on the OK/Cancel buttons, overriding the default
//-----------------------------------------------------------------------------
void PropertyDialog::SetCancelButtonText(const char *text)
{
	_cancelButton->SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: sets the text on the apply buttons, overriding the default
//-----------------------------------------------------------------------------
void PropertyDialog::SetApplyButtonText(const char *text)
{
	_applyButton->SetText(text);
}

//-----------------------------------------------------------------------------
// Purpose: changes the visibility of the buttons
//-----------------------------------------------------------------------------
void PropertyDialog::SetOKButtonVisible(bool state)
{
	_okButton->SetVisible(state);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: changes the visibility of the buttons
//-----------------------------------------------------------------------------
void PropertyDialog::SetCancelButtonVisible(bool state)
{
	_cancelButton->SetVisible(state);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: changes the visibility of the buttons
//-----------------------------------------------------------------------------
void PropertyDialog::SetApplyButtonVisible(bool state)
{
	_applyButton->SetVisible(state);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: when a sheet changes, enable the apply button
//-----------------------------------------------------------------------------
void PropertyDialog::OnApplyButtonEnable()
{
	if (_applyButton->IsEnabled())
		return;

	EnableApplyButton(true);
}

//-----------------------------------------------------------------------------
// Purpose: enable/disable the apply button
//-----------------------------------------------------------------------------
void PropertyDialog::EnableApplyButton(bool bEnable)
{
	_applyButton->SetEnabled(bEnable);
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PropertyDialog::RequestFocus(int direction)
{
    _propertySheet->RequestFocus(direction);
}
