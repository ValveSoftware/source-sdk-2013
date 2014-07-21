//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROPERTYPAGE_H
#define PROPERTYPAGE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/PHandle.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Property page, as held by a set of property sheets
//-----------------------------------------------------------------------------
class PropertyPage : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( PropertyPage, EditablePanel );

public:
	PropertyPage(Panel *parent, const char *panelName);
	~PropertyPage();

	// Called when page is loaded.  Data should be reloaded from document into controls.
	MESSAGE_FUNC( OnResetData, "ResetData" );

	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	MESSAGE_FUNC( OnApplyChanges, "ApplyChanges" );

	// called when the page is shown/hidden
	MESSAGE_FUNC( OnPageShow, "PageShow" );
	MESSAGE_FUNC( OnPageHide, "PageHide" );

	virtual void OnKeyCodeTyped(KeyCode code);
	virtual bool HasUserConfigSettings() { return true; }

	virtual void SetVisible(bool state);

protected:
	// called to be notified of the tab button used to Activate this page
	// if overridden this must be chained back to
	MESSAGE_FUNC_PTR( OnPageTabActivated, "PageTabActivated", panel );

private:
	PHandle _pageTab;
};

} // namespace vgui

#endif // PROPERTYPAGE_H
