//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICLIENTPANEL_H
#define ICLIENTPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

#ifdef GetClassName
#undef GetClassName
#endif

class KeyValues;

namespace vgui
{

class Panel;
class SurfaceBase;

enum EInterfaceID
{
	ICLIENTPANEL_STANDARD_INTERFACE = 0,
};

//-----------------------------------------------------------------------------
// Purpose: Interface from vgui panels -> Client panels
//			This interface cannot be changed without rebuilding all vgui projects
//			Primarily this interface handles dispatching messages from core vgui to controls
//			The additional functions are all their for debugging or optimization reasons
//			To add to this later, use QueryInterface() to see if they support new interfaces
//-----------------------------------------------------------------------------
class IClientPanel
{
public:
	virtual VPANEL GetVPanel() = 0;

	// straight interface to Panel functions
	virtual void Think() = 0;
	virtual void PerformApplySchemeSettings() = 0;
	virtual void PaintTraverse(bool forceRepaint, bool allowForce) = 0;
	virtual void Repaint() = 0;
	virtual VPANEL IsWithinTraverse(int x, int y, bool traversePopups) = 0;
	virtual void GetInset(int &top, int &left, int &right, int &bottom) = 0;
	virtual void GetClipRect(int &x0, int &y0, int &x1, int &y1) = 0;
	virtual void OnChildAdded(VPANEL child) = 0;
	virtual void OnSizeChanged(int newWide, int newTall) = 0;

	virtual void InternalFocusChanged(bool lost) = 0;
	virtual bool RequestInfo(KeyValues *outputData) = 0;
	virtual void RequestFocus(int direction) = 0;
	virtual bool RequestFocusPrev(VPANEL existingPanel) = 0;
	virtual bool RequestFocusNext(VPANEL existingPanel) = 0;
	virtual void OnMessage(const KeyValues *params, VPANEL ifromPanel) = 0;
	virtual VPANEL GetCurrentKeyFocus() = 0;
	virtual int GetTabPosition() = 0;

	// for debugging purposes
	virtual const char *GetName() = 0;
	virtual const char *GetClassName() = 0;

	// get scheme handles from panels
	virtual HScheme GetScheme() = 0;
	// gets whether or not this panel should scale with screen resolution
	virtual bool IsProportional() = 0;
	// auto-deletion
	virtual bool IsAutoDeleteSet() = 0;
	// deletes this
	virtual void DeletePanel() = 0;

	// interfaces
	virtual void *QueryInterface(EInterfaceID id) = 0;

	// returns a pointer to the vgui controls baseclass Panel *
	virtual Panel *GetPanel() = 0;

	// returns the name of the module this panel is part of
	virtual const char *GetModuleName() = 0;

	virtual void OnTick() = 0;
};

} // namespace vgui


#endif // ICLIENTPANEL_H
