//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Material editor
//=============================================================================

#ifndef VGUIMATSYSAPP_H
#define VGUIMATSYSAPP_H

#ifdef _WIN32
#pragma once
#endif


#include "appframework/tier3app.h"


//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CVguiMatSysApp : public CVguiSteamApp
{
	typedef CVguiSteamApp BaseClass;

public:
	CVguiMatSysApp();

	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual void PostShutdown();
	virtual void Destroy();

	// Returns the window handle (HWND in Win32)
	void* GetAppWindow();

	// Gets the window size
	int GetWindowWidth() const;
	int GetWindowHeight() const;

protected:
	void AppPumpMessages();

	// Sets the video mode
	bool SetVideoMode( );

	// Sets up the game path
	bool SetupSearchPaths( const char *pStartingDir, bool bOnlyUseStartingDir, bool bIsTool );

private:
	// Returns the app name
	virtual const char *GetAppName() = 0;
	virtual bool AppUsesReadPixels() { return false; }

	// Creates the app window
	virtual void *CreateAppWindow( char const *pTitle, bool bWindowed, int w, int h );

	void *m_HWnd;
	int m_nWidth;
	int m_nHeight;
};


#endif // VGUIMATSYSAPP_H
