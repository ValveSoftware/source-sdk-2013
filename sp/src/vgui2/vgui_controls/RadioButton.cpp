//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <stdarg.h>
#include <stdio.h>

#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include <vgui_controls/FocusNavGroup.h>
#include <vgui_controls/Image.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/TextImage.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

enum direction
{
	UP = -1,
	DOWN = 1,
};

//-----------------------------------------------------------------------------
// Purpose: Check box image
//-----------------------------------------------------------------------------
void RadioImage::Paint()
{
	DrawSetTextFont(GetFont());

	// draw background
	if (_radioButton->IsEnabled())
	{
		DrawSetTextColor(_bgColor);
	}
	else
	{
		DrawSetTextColor(_radioButton->GetBgColor());
	}
	DrawPrintChar(0, 1, 'n');

	// draw border circl
	DrawSetTextColor(_borderColor1);
	DrawPrintChar(0, 1, 'j');
	DrawSetTextColor(_borderColor2);
	DrawPrintChar(0, 1, 'k');

	// draw selected check
	if (_radioButton->IsSelected())
	{
		DrawSetTextColor(_checkColor);
		DrawPrintChar(0, 1, 'h');
	}
}

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( RadioButton, RadioButton );

//-----------------------------------------------------------------------------
// Purpose: Create a radio button.
//-----------------------------------------------------------------------------
RadioButton::RadioButton(Panel *parent, const char *panelName, const char *text) : ToggleButton(parent, panelName, text)
{
 	SetContentAlignment(a_west);

	// create the image
	_radioBoxImage = new RadioImage(this);

	_oldTabPosition = 0;
	_subTabPosition = 0;

	SetTextImageIndex(1);
	SetImageAtIndex(0, _radioBoxImage, 0);

	SetButtonActivationType(ACTIVATE_ONPRESSED);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
RadioButton::~RadioButton()
{
	delete _radioBoxImage;
}

//-----------------------------------------------------------------------------
// Purpose: Apply resource file scheme.
//-----------------------------------------------------------------------------
void RadioButton::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	_radioBoxImage->_bgColor = GetSchemeColor("CheckButton.BgColor", Color(150, 150, 150, 0), pScheme);
	_radioBoxImage->_borderColor1 = GetSchemeColor("CheckButton.Border1", Color(20, 20, 20, 0), pScheme);
	_radioBoxImage->_borderColor2 = GetSchemeColor("CheckButton.Border2", Color(90, 90, 90, 0), pScheme);
	_radioBoxImage->_checkColor = GetSchemeColor("CheckButton.Check", Color(20, 20, 20, 0), pScheme);

	SetFgColor(GetSchemeColor("RadioButton.TextColor", pScheme));
	_selectedFgColor = GetSchemeColor("RadioButton.SelectedTextColor", GetSchemeColor("ControlText", pScheme), pScheme);

	SetDefaultColor( GetFgColor(), GetBgColor() );

	SetArmedColor( GetSchemeColor("RadioButton.ArmedTextColor", pScheme), GetButtonArmedBgColor() );

	SetContentAlignment(a_west);

	//  reloading the scheme wipes out lists of images
	HFont hFont = pScheme->GetFont("MarlettSmall", IsProportional());
	if ( hFont == INVALID_FONT )
	{
		// fallback to Marlett if MarlettSmall isn't found
		hFont = pScheme->GetFont("Marlett", IsProportional());
	}
	_radioBoxImage->SetFont( hFont );
	_radioBoxImage->ResizeImageToContent();
	SetImageAtIndex(0, _radioBoxImage, 0);

	// don't draw a background
	SetPaintBackgroundEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: Get the border style of the button, Radio buttons have no border			
//-----------------------------------------------------------------------------
IBorder *RadioButton::GetBorder(bool depressed, bool armed, bool selected, bool keyfocus)
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get the tab position of the radio button with the set of radio buttons
//			A group of RadioButtons must have the same TabPosition, with [1, n] subtabpositions
//-----------------------------------------------------------------------------
int RadioButton::GetSubTabPosition()
{
	return _subTabPosition;
}

//-----------------------------------------------------------------------------
// Purpose: Get the tab position of the radio button with the set of radio buttons
//			A group of RadioButtons must have the same TabPosition, with [1, n] subtabpositions
//-----------------------------------------------------------------------------
void RadioButton::SetSubTabPosition(int position)
{
	_subTabPosition = position;
}

//-----------------------------------------------------------------------------
// Purpose: Return the RadioButton's real tab position (its Panel one changes)
//-----------------------------------------------------------------------------
int RadioButton::GetRadioTabPosition()
{
	return _oldTabPosition;
}

//-----------------------------------------------------------------------------
// Purpose: Set the radio button checked. When a radio button is checked, a 
//			message is sent to all other radio buttons in the same group so
//			they will become unchecked.
//-----------------------------------------------------------------------------
void RadioButton::SetSelected(bool state)
{
	InternalSetSelected( state, true );
}

void RadioButton::InternalSetSelected(bool state, bool bFireEvents)
{
	if (state == true)
	{
		if (!IsEnabled())
			return;

		// restore our tab position
		SetTabPosition(_oldTabPosition);

		// Should we send notifications?
		if ( bFireEvents )
		{
			// send a message
			KeyValues *msg = new KeyValues("RadioButtonChecked");
			msg->SetPtr("panel", this);
			msg->SetInt("tabposition", _oldTabPosition);

			// send a message to all other panels on the same level as heirarchy, 
			// so that other radio buttons know to shut off
			VPANEL radioParent = GetVParent();
			if (radioParent)
			{
				for (int i = 0; i < ipanel()->GetChildCount(radioParent); i++)
				{
					VPANEL child = ipanel()->GetChild(radioParent, i);
					if (child != GetVPanel())
					{
						ivgui()->PostMessage(child, msg->MakeCopy(), GetVPanel());
					}
				}
			}

			RequestFocus();
			PostActionSignal(msg);
		}
	}
	else
	{
		// remove ourselves from the tab order
		if (GetTabPosition())
		{
			_oldTabPosition = GetTabPosition();
		}
		SetTabPosition(0);
	}

	InvalidateLayout();
	Repaint();

	ToggleButton::SetSelected(state);
}

//-----------------------------------------------------------------------------
// Purpose: Set the selection state without firing any events
//-----------------------------------------------------------------------------
void RadioButton::SilentSetSelected(bool state)
{
	InternalSetSelected( state, false );
}

//-----------------------------------------------------------------------------
// Purpose: Set up the text color before doing normal layout
//-----------------------------------------------------------------------------
void RadioButton::PerformLayout()
{
	if (IsSelected())
	{
		SetFgColor(_selectedFgColor);
	}
	else
	{
		SetFgColor(GetButtonFgColor());
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Apply resource settings including button state and text color.
//-----------------------------------------------------------------------------
void RadioButton::ApplySettings(KeyValues *inResourceData)
{
	ToggleButton::ApplySettings(inResourceData);
	SetTextColorState(CS_NORMAL);

	_subTabPosition = inResourceData->GetInt("SubTabPosition");
	_oldTabPosition = inResourceData->GetInt("TabPosition");
	SetImageAtIndex(0, _radioBoxImage, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Get resource settings including button state, text color, and subTabPosition
//-----------------------------------------------------------------------------
void RadioButton::GetSettings(KeyValues *outResourceData)
{
	ToggleButton::GetSettings(outResourceData);
	outResourceData->SetInt("SubTabPosition", _subTabPosition);
	outResourceData->SetInt("TabPosition", GetRadioTabPosition());
}

//-----------------------------------------------------------------------------
// Purpose: Describe editing details
//-----------------------------------------------------------------------------
const char *RadioButton::GetDescription( void )
{
	static char buf[1024];
	Q_snprintf(buf, sizeof(buf), "%s, int SubTabPosition", BaseClass::GetDescription());
	return buf;
}

//-----------------------------------------------------------------------------
// Purpose: When a radio button is checked, all other radio buttons
//			in the same group become unchecked.
//-----------------------------------------------------------------------------
void RadioButton::OnRadioButtonChecked(int tabPosition)
{
	// make sure we're in the same tab group
	if (tabPosition != _oldTabPosition)
		return;

	// wouldn't be sent to us from ourselves, so another radio button has taken over
	SetSelected(false);
}

//-----------------------------------------------------------------------------
// Purpose: Draws the selection rectangle
//-----------------------------------------------------------------------------
void RadioButton::Paint()
{
	BaseClass::Paint();

	/*
	if (HasFocus())
	{
		int tx0, ty0, tx1, ty1;
		_textImage->GetPos(tx0, ty0);
		_textImage->GetSize(tx1, ty1);
		DrawFocusBorder(tx0, ty0, tx0 + tx1, ty0 + ty1);
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void RadioButton::DoClick()
{
	SetSelected(true);
}

//-----------------------------------------------------------------------------
// Purpose: Handle arrow key movement
//-----------------------------------------------------------------------------
void RadioButton::OnKeyCodeTyped(KeyCode code)
{
	switch (code)
	{
		case KEY_ENTER:
		case KEY_SPACE:
		{
			if (!IsSelected())
			{
				SetSelected(true);
			}
			else
			{
				Panel::OnKeyCodeTyped(code);
			}
		}
		break;

		case KEY_DOWN:
		case KEY_RIGHT:
			{
				RadioButton *bestRadio = FindBestRadioButton( DOWN );
				if (bestRadio)
				{
					bestRadio->SetSelected(true);
				}	
			}
			break;
		case KEY_UP:
		case KEY_LEFT:
			{
				RadioButton *bestRadio = FindBestRadioButton( UP );
				if (bestRadio)
				{
					bestRadio->SetSelected(true);
				}	
			}
			break;

		default:
			BaseClass::OnKeyCodeTyped(code);
			break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the correct radio button to move to.
// Input  : direction - the direction we are moving, up or down. 
//-----------------------------------------------------------------------------
RadioButton *RadioButton::FindBestRadioButton(int direction)
{
	RadioButton *bestRadio = NULL;
	int highestRadio = 0;
	Panel *pr = GetParent();
	if (pr)
	{
		// find the radio button to go to next
		for (int i = 0; i < pr->GetChildCount(); i++)
		{
			RadioButton *child = dynamic_cast<RadioButton *>(pr->GetChild(i));
			if (child && child->GetRadioTabPosition() == _oldTabPosition)
			{
				if (child->GetSubTabPosition() == _subTabPosition + direction)
				{
					bestRadio = child;
					break;
				}
				if ( (child->GetSubTabPosition() == 0)  && (direction == DOWN) )
				{
					bestRadio = child;
					continue;
				}
				else if ( (child->GetSubTabPosition() > highestRadio) && (direction == UP) )
				{
					bestRadio = child;
					highestRadio = bestRadio->GetSubTabPosition();
					continue;
				}
				if (!bestRadio)
				{
					bestRadio = child;
				}
			}
		}
		
		if (bestRadio)
		{
			bestRadio->RequestFocus();
		}
		
		InvalidateLayout();
		Repaint();
	}

	return bestRadio;
}
