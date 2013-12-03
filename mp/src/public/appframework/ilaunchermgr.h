//========= Copyright Valve Corporation, All rights reserved. ============//
//
//	ilaunchermgr.h
//
//==================================================================================================
#ifndef ILAUNCHERMGR_H
#define ILAUNCHERMGR_H

#ifdef _WIN32
#pragma once
#endif

#if defined( USE_SDL )

#include "tier0/threadtools.h"
#include "appframework/IAppSystem.h"

#if defined( DX_TO_GL_ABSTRACTION )
	#if defined( LINUX ) || defined( _WIN32 )
		#include "togl/linuxwin/glmgrbasics.h"
		#include "togl/linuxwin/glmdisplay.h"
	#elif defined( OSX )
		#include "togl/osx/glmgrbasics.h"
		#include "togl/osx/glmdisplay.h"
	#endif

class GLMDisplayDB;
class CShowPixelsParams;
#endif

// if you rev this version also update materialsystem/cmaterialsystem.cpp CMaterialSystem::Connect as it defines the string directly
#define SDLMGR_INTERFACE_VERSION "SDLMgrInterface001"

class CCocoaEvent;
class CStackCrawlParams;

typedef struct SDL_Cursor SDL_Cursor;

class ILauncherMgr : public IAppSystem
{
public:
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;
	
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;
	
	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
	
	// Create the window.
	virtual bool CreateGameWindow( const char *pTitle, bool bWindowed, int width, int height ) = 0;
	
	virtual void IncWindowRefCount() = 0;
	virtual void DecWindowRefCount() = 0;
		
	// Get the next N events. The function returns the number of events that were filled into your array.
	virtual int GetEvents( CCocoaEvent *pEvents, int nMaxEventsToReturn, bool debugEvents = false ) = 0;
#ifdef LINUX
	virtual int PeekAndRemoveKeyboardEvents( bool *pbEsc, bool *pbReturn, bool *pbSpace, bool debugEvents = false ) = 0;
#endif

	// Set the mouse cursor position.
	virtual void SetCursorPosition( int x, int y ) = 0;
	
	virtual void SetWindowFullScreen( bool bFullScreen, int nWidth, int nHeight ) = 0;
	virtual bool IsWindowFullScreen() = 0;
	virtual void MoveWindow( int x, int y ) = 0;
	virtual void SizeWindow( int width, int tall ) = 0;
	virtual void PumpWindowsMessageLoop() = 0;
		
	virtual void DestroyGameWindow() = 0;
	virtual void SetApplicationIcon( const char *pchAppIconFile ) = 0;
	
	virtual void GetMouseDelta( int &x, int &y, bool bIgnoreNextMouseDelta = false ) = 0;

	virtual void GetNativeDisplayInfo( int nDisplay, uint &nWidth, uint &nHeight, uint &nRefreshHz ) = 0; // Retrieve the size of the monitor (desktop)
	virtual void RenderedSize( uint &width, uint &height, bool set ) = 0;	// either set or retrieve rendered size value (from dxabstract)
	virtual void DisplayedSize( uint &width, uint &height ) = 0;			// query backbuffer size (window size whether FS or windowed)
	
#if defined( DX_TO_GL_ABSTRACTION )
	virtual PseudoGLContextPtr	GetMainContext() = 0;
	// Get the NSGLContext for a window's main view - note this is the carbon windowref as an argument
	virtual PseudoGLContextPtr GetGLContextForWindow( void* windowref ) = 0;
	virtual PseudoGLContextPtr CreateExtraContext() = 0;
	virtual void DeleteContext( PseudoGLContextPtr hContext ) = 0;
	virtual bool MakeContextCurrent( PseudoGLContextPtr hContext ) = 0;
	virtual GLMDisplayDB *GetDisplayDB( void ) = 0;
	virtual void GetDesiredPixelFormatAttribsAndRendererInfo( uint **ptrOut, uint *countOut, GLMRendererInfoFields *rendInfoOut ) = 0;
	virtual void ShowPixels( CShowPixelsParams *params ) = 0;
#endif

	virtual void GetStackCrawl( CStackCrawlParams *params ) = 0;	
	
	virtual void WaitUntilUserInput( int msSleepTime ) = 0;
	
	virtual void *GetWindowRef() = 0;

	virtual void SetMouseVisible( bool bState ) = 0;
	virtual void SetMouseCursor( SDL_Cursor *hCursor ) = 0;
	virtual void SetForbidMouseGrab( bool bForbidMouseGrab ) = 0;
	virtual void OnFrameRendered() = 0;

	virtual void SetGammaRamp( const uint16 *pRed, const uint16 *pGreen, const uint16 *pBlue ) = 0;

	virtual double GetPrevGLSwapWindowTime() = 0;
};

extern ILauncherMgr *g_pLauncherMgr;

enum CocoaEventType_t
{
	CocoaEvent_KeyDown,
	CocoaEvent_KeyUp,
	CocoaEvent_MouseButtonDown,
	CocoaEvent_MouseMove,
	CocoaEvent_MouseButtonUp,
	CocoaEvent_AppActivate,
	CocoaEvent_MouseScroll,
	CocoaEvent_AppQuit,
	CocoaEvent_Deleted, // Event was one of the above, but has been handled and should be ignored now.
};

// enum values need to match bit-shifting logic in CInputSystem::UpdateMouseButtonState and 
// the codes from NSEvent pressedMouseButtons, turns out the two are in agreement right now
enum CocoaMouseButton_t
{
	COCOABUTTON_LEFT = 1 << 0,
	COCOABUTTON_RIGHT = 1 << 1,
	COCOABUTTON_MIDDLE = 1 << 2,
	COCOABUTTON_4 = 1 << 3,	
	COCOABUTTON_5 = 1 << 4,	
};

enum ECocoaKeyModifier
{
	eCapsLockKey,
	eShiftKey,
	eControlKey,
	eAltKey,		// aka option
	eCommandKey
};

class CCocoaEvent
{
public:
	CocoaEventType_t m_EventType;
	int m_VirtualKeyCode;
	wchar_t m_UnicodeKey;
	wchar_t m_UnicodeKeyUnmodified;
	uint m_ModifierKeyMask;		// 
	int m_MousePos[2];
	int m_MouseButtonFlags;	// Current state of the mouse buttons. See COCOABUTTON_xxxx.
	uint m_nMouseClickCount;
	int m_MouseButton; // which of the CocoaMouseButton_t buttons this is for from above
};

#endif // defined( USE_SDL )

#endif // ILAUNCHERMGR_H
