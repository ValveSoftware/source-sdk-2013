//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef TOGGLEBUTTON_H
#define TOGGLEBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Button.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Type of button that when pressed stays selected & depressed until pressed again
//-----------------------------------------------------------------------------
class ToggleButton : public Button
{
	DECLARE_CLASS_SIMPLE( ToggleButton, Button );

public:
	ToggleButton(Panel *parent, const char *panelName, const char *text);

	virtual void DoClick();

	/* messages sent (get via AddActionSignalTarget()):
		"ButtonToggled"
			int "state"
	*/

protected:
	// overrides
	virtual void OnMouseDoublePressed(MouseCode code);

	virtual Color GetButtonFgColor();
	virtual void ApplySchemeSettings(IScheme *pScheme);

    virtual bool CanBeDefaultButton(void);
    virtual void OnKeyCodePressed(KeyCode code);

private:
	Color _selectedColor;
};

} // namespace vgui

#endif // TOGGLEBUTTON_H
