//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MENUBAR_H
#define MENUBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <utlvector.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class MenuBar : public Panel
{
	DECLARE_CLASS_SIMPLE( MenuBar, Panel );

public:
	MenuBar(Panel *parent, const char *panelName);
	~MenuBar();

	virtual void AddButton(MenuButton *button); // add button to end of menu list
	virtual void AddMenu( const char *pButtonName, Menu *pMenu );

	virtual void GetContentSize( int& w, int&h );

protected:
	virtual void OnKeyCodeTyped(KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void PerformLayout();
	virtual void Paint();
	MESSAGE_FUNC( OnMenuClose, "MenuClose" );
	MESSAGE_FUNC_INT( OnCursorEnteredMenuButton, "CursorEnteredMenuButton", VPanel);

private:
	CUtlVector<MenuButton *> m_pMenuButtons;
	int						m_nRightEdge;
};

} // namespace vgui

#endif // MENUBAR_H

