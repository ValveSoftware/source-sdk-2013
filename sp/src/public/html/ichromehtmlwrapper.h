//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICHROMEHTMLWRAPPER_H
#define ICHROMEHTMLWRAPPER_H

#ifdef _WIN32
#pragma once
#endif

#include <html/htmlmessages.h>

class CUtlString;
class IHTMLResponses;
struct HTMLCommandBuffer_t;


//-------------------------------------------------------------	----------------
// Purpose: wrapper for HTML functionality
//-----------------------------------------------------------------------------
class IHTMLChromeController
{
public:
	virtual ~IHTMLChromeController() {}

	virtual bool Init( const char *pchHTMLCacheDir, const char *pchCookiePath ) = 0;
	virtual void Shutdown() = 0;
	virtual bool RunFrame() = 0;


	// set a cookie in the CEF instance. pchPath is typically "/". If nExpires is 0 then it is a session only cookie, other wise it is saved.
	virtual void SetWebCookie( const char *pchHostname, const char *pchKey, const char *pchValue, const char *pchPath, RTime32 nExpires = 0 ) = 0;
	virtual void GetWebCookiesForURL( CUtlString *pstrValue, const char *pchURL, const char *pchName ) = 0;

	virtual void SetClientBuildID( uint64 ulBuildID ) = 0;

	virtual bool BHasPendingMessages() = 0;

	virtual void CreateBrowser( IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier ) = 0;
	virtual void RemoveBrowser( IHTMLResponses *pBrowser ) = 0;

	virtual void WakeThread() = 0;
	virtual HTMLCommandBuffer_t *GetFreeCommandBuffer( EHTMLCommands eCmd, int iBrowser ) = 0;
	virtual void PushCommand( HTMLCommandBuffer_t * ) = 0;

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName ) = 0;

	virtual bool ChromePrepareForValidate() = 0;
	virtual bool ChromeResumeFromValidate() = 0;
#endif

	virtual void SetCefThreadTargetFrameRate( uint32 nFPS ) = 0;
};

#define CHROMEHTML_CONTROLLER_INTERFACE_VERSION "ChromeHTML_Controller_001"


#endif // ICHROMEHTMLWRAPPER_H