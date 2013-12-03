//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TOOLWINDOW_H
#define TOOLWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Frame.h>

namespace vgui
{

class ToolWindow;

// So that an app can have a "custom" tool window class created during window drag/drop operations on the property sheet
class IToolWindowFactory
{
public:
	virtual ToolWindow *InstanceToolWindow( Panel *parent, bool contextLabel, Panel *firstPage, char const *title, bool contextMenu ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Simple frame that holds a property sheet
//-----------------------------------------------------------------------------
class ToolWindow : public Frame
{
	DECLARE_CLASS_SIMPLE( ToolWindow, Frame );

public:
	ToolWindow(Panel *parent, bool contextLabel, IToolWindowFactory *factory = 0, Panel *page = NULL, char const *title = NULL, bool contextMenu = false, bool inGlobalList = true );

	~ToolWindow();

	virtual bool IsDraggableTabContainer() const;

	// returns a pointer to the PropertySheet this dialog encapsulates 
	PropertySheet *GetPropertySheet();

	// wrapper for PropertySheet interface
	void AddPage(Panel *page, const char *title, bool contextMenu );
	void RemovePage( Panel *page );
	Panel *GetActivePage();
	void SetActivePage( Panel *page );

	void SetToolWindowFactory( IToolWindowFactory *factory );
	IToolWindowFactory *GetToolWindowFactory();

	static int GetToolWindowCount();
	static ToolWindow *GetToolWindow( int index );

	static CUtlVector< ToolWindow * > s_ToolWindows;

	virtual void Grow( int edge = 0, int from_x = -1, int from_y = -1 );
	virtual void GrowFromClick();

protected:
	// vgui overrides
	virtual void PerformLayout();
	virtual void ActivateBuildMode();
	virtual void RequestFocus(int direction = 0);
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);

private:
	PropertySheet		*m_pPropertySheet;
	IToolWindowFactory	*m_pFactory;
};

}; // vgui


#endif // TOOLWINDOW_H
