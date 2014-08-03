//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef WIZARDSUBPANEL_H
#define WIZARDSUBPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Base panel for use in Wizards and in property sheets
//-----------------------------------------------------------------------------
class WizardSubPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( WizardSubPanel, EditablePanel );

public:
	// constructor
	WizardSubPanel(Panel *parent, const char *panelName);
	~WizardSubPanel();

	// called when the subpanel is displayed
	// All controls & data should be reinitialized at this time
	virtual void OnDisplayAsNext() {}

	// called anytime the panel is first displayed, whether the user is moving forward or back
	// called immediately after OnDisplayAsNext/OnDisplayAsPrev
	virtual void OnDisplay() {}

	// called when displayed as previous
	virtual void OnDisplayAsPrev() {}

	// called when one of the wizard buttons are pressed
	// returns true if the wizard should advance, false otherwise
	virtual bool OnNextButton() { return true; }
	virtual bool OnPrevButton() { return true; }
	virtual bool OnFinishButton() { return true; }
	virtual bool OnCancelButton() { return true; }

	// returns true if this panel should be displayed, or if we should just skip over it
	virtual bool ShouldDisplayPanel()	{ return true; }

	// return true if this subpanel doesn't need the next/prev/finish/cancel buttons or will do it itself
	virtual bool isNonWizardPanel() { return false; }

	// returns a pointer to the next subpanel that should be displayed
	virtual WizardSubPanel *GetNextSubPanel()  = 0;

	// returns a pointer to the panel to return to
	// it must be a panel that is already in the wizards panel history
	// returning NULL tells it to use the immediate previous panel in the history
	virtual WizardSubPanel *GetPrevSubPanel() { return NULL; }

	virtual WizardPanel *GetWizardPanel() { return _wizardPanel; }
	virtual void SetWizardPanel(WizardPanel *wizardPanel) { _wizardPanel = wizardPanel; }

	// returns a pointer to the wizard's doc
	virtual KeyValues *GetWizardData();

	// returns a pointer
	virtual WizardSubPanel *GetSiblingSubPanelByName(const char *pageName);

	// gets the size this subpanel would like the wizard to be
	// returns true if it has a desired size
	virtual bool GetDesiredSize(int &wide, int &tall);

protected:
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void GetSettings( KeyValues *outResourceData );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual const char *GetDescription();

private:
	WizardPanel *_wizardPanel;
	int m_iDesiredWide, m_iDesiredTall;
};

} // namespace vgui


#endif // WIZARDSUBPANEL_H	   
