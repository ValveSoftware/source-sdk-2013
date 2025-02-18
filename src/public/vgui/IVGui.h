//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IVGUI_H
#define IVGUI_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include <vgui/VGUI.h>

#include "appframework/IAppSystem.h"

class KeyValues;
class IVEngineClient;

namespace vgui
{

// safe handle to a panel - can be converted to and from a VPANEL
typedef uint32 HPanel;
typedef uint32 HContext;

enum : vgui::HContext
{
	DEFAULT_VGUI_CONTEXT = ((vgui::HContext)~0)
};

//-----------------------------------------------------------------------------
// Purpose: Interface to core vgui components
//-----------------------------------------------------------------------------
class IVGui : public IAppSystem
{
public:
	// activates vgui message pump
	virtual void Start() = 0;

	// signals vgui to Stop running
	virtual void Stop() = 0;

	// returns true if vgui is current active
	virtual bool IsRunning() = 0;

	// runs a single frame of vgui
	virtual void RunFrame() = 0;

	// broadcasts "ShutdownRequest" "id" message to all top-level panels in the app
	virtual void ShutdownMessage(unsigned int shutdownID) = 0;

	// panel allocation
	virtual VPANEL AllocPanel() = 0;
	virtual void FreePanel(VPANEL panel) = 0;
	
	// debugging prints
	virtual void DPrintf(PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void DPrintf2(PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void SpewAllActivePanelNames() = 0;
	
	// safe-pointer handle methods
	virtual HPanel PanelToHandle(VPANEL panel) = 0;
	virtual VPANEL HandleToPanel(HPanel index) = 0;
	virtual void MarkPanelForDeletion(VPANEL panel) = 0;

	// makes panel receive a 'Tick' message every frame (~50ms, depending on sleep times/framerate)
	// panel is automatically removed from tick signal list when it's deleted
	virtual void AddTickSignal(VPANEL panel, int intervalMilliseconds = 0 ) = 0;
	virtual void RemoveTickSignal(VPANEL panel) = 0;

	// message sending
	virtual void PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delaySeconds = 0.0f) = 0;

	// Creates/ destroys vgui contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual HContext CreateContext() = 0;
	virtual void DestroyContext( HContext context ) = 0; 

	// Associates a particular panel with a vgui context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithContext( HContext context, VPANEL pRoot ) = 0;

	// Activates a particular context, use DEFAULT_VGUI_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateContext( HContext context ) = 0;

	// whether to sleep each frame or not, true = sleep
	virtual void SetSleep( bool state) = 0; 

	// data accessor for above
	virtual bool GetShouldVGuiControlSleep() = 0;

	// enables VR mode
	virtual void SetVRMode( bool bVRMode ) = 0;
	virtual bool GetVRMode() = 0;

	// add a tick signal like above, but to the head of the list of tick signals
	virtual void AddTickSignalToHead( VPANEL panel, int intervalMilliseconds = 0 ) = 0;

	virtual IVEngineClient *GetVGUIEngine() = 0;

	virtual void InvalidateMdlCache() = 0;
	virtual uint32 GetMdlCacheSerial() = 0;
};

#define VGUI_IVGUI_INTERFACE_VERSION "VGUI_ivgui008"

};


#endif // IVGUI_H
