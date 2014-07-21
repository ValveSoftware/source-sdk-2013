//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef RADIOBUTTON_H
#define RADIOBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/ToggleButton.h>
#include <vgui_controls/TextImage.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Radio buton image
//-----------------------------------------------------------------------------
class RadioImage : public TextImage
{
public:
	RadioImage(RadioButton *radioButton) : TextImage( "n" )
	{
		_radioButton = radioButton;

		SetSize(20, 13);
	}

	virtual void Paint();

	virtual void SetColor(Color color)
	{
		_borderColor1 = color;
		_borderColor2 = color;
		_checkColor = color;
	}

	Color _borderColor1;
	Color _borderColor2;
	Color _checkColor;

	Color _bgColor;

private:
	RadioButton *_radioButton;
};

//-----------------------------------------------------------------------------
// Purpose: Radio buttons are automatically selected into groups by who their
//			parent is. At most one radio button is active at any time.
//-----------------------------------------------------------------------------
class RadioButton : public ToggleButton
{
	DECLARE_CLASS_SIMPLE( RadioButton, ToggleButton );

public:
	RadioButton(Panel *parent, const char *panelName, const char *text);
	~RadioButton();

	// Set the radio button checked. When a radio button is checked, a 
	// message is sent to all other radio buttons in the same group so
	// they will become unchecked.
	virtual void SetSelected(bool state);

	// Get the tab position of the radio button with the set of radio buttons
	// A group of RadioButtons must have the same TabPosition, with [1, n] subtabpositions
	virtual int GetSubTabPosition();
	virtual void SetSubTabPosition(int position);

	// Return the RadioButton's real tab position (its Panel one changes)
	virtual int GetRadioTabPosition();

	// Set the selection state of the radio button, but don't fire
	// any action signals or messages to other radio buttons
	virtual void SilentSetSelected(bool state);

protected:
	virtual void DoClick();

	virtual void Paint();
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_INT( OnRadioButtonChecked, "RadioButtonChecked", tabposition);
	virtual void OnKeyCodeTyped(KeyCode code);

	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings(KeyValues *outResourceData);
	virtual const char *GetDescription();
	virtual void PerformLayout();

	RadioButton *FindBestRadioButton(int direction);

private:
	RadioImage *_radioBoxImage;
	int _oldTabPosition;
	Color _selectedFgColor;

	int _subTabPosition;	// tab position with the radio button list

	void InternalSetSelected(bool state, bool bFireEvents);
};

}; // namespace vgui

#endif // RADIOBUTTON_H
