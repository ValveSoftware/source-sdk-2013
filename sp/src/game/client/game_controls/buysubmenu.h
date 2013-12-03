//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BUYSUBMENU_H
#define BUYSUBMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/WizardSubPanel.h>
#include <vgui_controls/Button.h>
#include <utlvector.h>
#include "mouseoverpanelbutton.h"

class CBuyMenu;

//-----------------------------------------------------------------------------
// Purpose: Draws the class menu
//-----------------------------------------------------------------------------
class CBuySubMenu : public vgui::WizardSubPanel
{
private:
	DECLARE_CLASS_SIMPLE( CBuySubMenu, vgui::WizardSubPanel );
	
public:
	CBuySubMenu(vgui::Panel *parent,const char *name = "BuySubMenu");
	~CBuySubMenu();

	virtual void SetVisible( bool state );
	virtual void DeleteSubPanels();
		
protected:

	// command callbacks
	virtual void OnCommand( const char *command );
	virtual vgui::WizardSubPanel *GetNextSubPanel(); // this is the last menu in the list
	virtual vgui::Panel *CreateControlByName(const char *controlName);
	virtual CBuySubMenu* CreateNewSubMenu();
	virtual MouseOverPanelButton* CreateNewMouseOverPanelButton(vgui::EditablePanel *panel);
	
	typedef struct
	{
		char filename[_MAX_PATH];
		CBuySubMenu *panel;
	} SubMenuEntry_t;

	vgui::EditablePanel *m_pPanel;
	MouseOverPanelButton *m_pFirstButton;

	CUtlVector<SubMenuEntry_t> m_SubMenus; // a cache of buy submenus, so we don't need to construct them each time

	vgui::WizardSubPanel *m_NextPanel;
};

#endif // BUYSUBMENU_H
