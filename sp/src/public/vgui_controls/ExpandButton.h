//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A button with no borders that shows a left-pointing or down-pointing triangle
//
// $NoKeywords: $
//===========================================================================//

#ifndef EXPANDBUTTON_H
#define EXPANDBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/ToggleButton.h>


namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: A button with no borders that shows a left-pointing or down-pointing arrow
//-----------------------------------------------------------------------------
class ExpandButton : public ToggleButton
{
	DECLARE_CLASS_SIMPLE( ExpandButton, ToggleButton );

public:
	ExpandButton( Panel *parent, const char *panelName );
	~ExpandButton();

	// Expand the button (selected == expanded)
	virtual void SetSelected( bool bExpand );

	// sets whether or not the state of the check can be changed
	// if this is set to false, then no input in the code or by the user can change it's state
	void SetExpandable(bool state);

	virtual void Paint();

protected:
	virtual void ApplySchemeSettings(IScheme *pScheme);
	MESSAGE_FUNC_PTR( OnExpanded, "Expanded", panel );

	virtual IBorder *GetBorder(bool depressed, bool armed, bool selected, bool keyfocus);

	/* MESSAGES SENT
		"Expanded" - sent when the expand button state is changed
			"state"	- button state: 1 is expanded, 0 is unexpanded
	*/

private:
	bool m_bExpandable;
	HFont m_hFont;
	Color m_Color;
};

} // namespace vgui

#endif // EXPANDBUTTON_H
