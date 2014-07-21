//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "clientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================
// Global startup and shutdown functions for game code in the DLL.
//================================================================

class CViewportClientSystem : public IGameSystem
{
public:
	CViewportClientSystem()
	{
	}

	virtual char const *Name() { return "CViewportClientSystem"; }
	virtual bool IsPerFrame() { return false; }

	// Init, shutdown
	virtual bool Init()
	{
		g_pClientMode->Layout();
		return true;
	}
	virtual void PostInit() {}
	virtual void Shutdown() {}
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}
	virtual void SafeRemoveIfDesired() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}

};

static CViewportClientSystem g_ViewportClientSystem;

IGameSystem *ViewportClientSystem()
{
	return &g_ViewportClientSystem;
}
