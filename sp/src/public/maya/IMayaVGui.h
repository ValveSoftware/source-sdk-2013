//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Interface for dealing with vgui focus issues across all plugins
//
// $NoKeywords: $
//===========================================================================//

#ifndef IMAYAVGUI_H
#define IMAYAVGUI_H

#ifdef _WIN32
#pragma once
#endif


#include "tier0/platform.h"
#include "appframework/iappsystem.h"
#include "vgui_controls/Frame.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class EditablePanel;
}

class CVsVGuiWindowBase;


//-----------------------------------------------------------------------------
// Factory for creating vgui windows
//-----------------------------------------------------------------------------
abstract_class IMayaVguiWindowFactory
{
public:
	virtual void CreateVguiWindow( const char *pPanelName ) = 0; 
	virtual void DestroyVguiWindow( const char *pPanelName ) = 0; 
	virtual vgui::Frame *GetVGuiPanel( const char *pPanelName = NULL ) = 0;
	virtual CVsVGuiWindowBase *GetVGuiWindow( const char *pPanelName = NULL ) = 0;
};


//-----------------------------------------------------------------------------
// Interface for dealing with vgui focus issues across all plugins
//-----------------------------------------------------------------------------
#define MAYA_VGUI_INTERFACE_VERSION "VMayaVGui001"
abstract_class IMayaVGui : public IAppSystem
{
public:
	virtual void InstallVguiWindowFactory( const char *pWindowTypeName, IMayaVguiWindowFactory *pFactory ) = 0;
	virtual void RemoveVguiWindowFactory( const char *pWindowTypeName, IMayaVguiWindowFactory *pFactory ) = 0;
	virtual void SetFocus( void *hWnd, int hVGuiContext ) = 0;
	virtual bool HasFocus( void *hWnd ) = 0;

	// In this mode, maya's in a strange re-entrant mode waiting for a modal dialog
	// We still get WM_PAINT messages, but we're in the middle of a callstack
	// deep in the bowels of VGUI
	virtual void SetModalMode( bool bEnable ) = 0;
	virtual bool IsInModalMode( ) const = 0;
};

extern IMayaVGui* g_pMayaVGui;


#endif // IMAYAVGUI_H
