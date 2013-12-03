//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#ifndef HTMLMANAGER_H
#define HTMLMANAGER_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: helper funcs to pump chrome
//-----------------------------------------------------------------------------
void ChromeInit( const char *pchHTMLCacheDir, const char *pchCookiePath );
void ChromeShutdown();

#ifdef DBGFLAG_VALIDATE
void ChromeValidate( CValidator &validator, const char *pchName );
bool ChromeResumeFromValidate();
bool ChromePrepareForValidate();
#endif

bool ChromeSetWebCookie(  const char *pchHostname, const char *pchName, const char *pchValue, const char *pchPath );
void ChromeSetClientBuildID( uint64 ulBuildID );

enum EMouseState { UP,DOWN,MOVE,DBLCLICK };

#endif // HTMLMANAGER_H
