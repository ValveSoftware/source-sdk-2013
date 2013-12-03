//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef UNICODE_H
#define UNICODE_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if !defined( _X360 )
#include <windows.h>
#endif
#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

class IUnicodeWindows : public IBaseInterface
{
public:
	virtual LRESULT DefWindowProcW
	(
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam 
	) = 0;

	virtual HWND CreateWindowExW
	(          
		DWORD dwExStyle,
	    LPCWSTR lpClassName,
	    LPCWSTR lpWindowName,
	    DWORD dwStyle,
	    int x,
	    int y,
	    int nWidth,
	    int nHeight,
	    HWND hWndParent,
	    HMENU hMenu,
	    HINSTANCE hInstance,
	    LPVOID lpParam
	) = 0;

	virtual ATOM RegisterClassW
	( 
		CONST WNDCLASSW *lpWndClass
	) = 0;

	virtual BOOL UnregisterClassW
	( 
		LPCWSTR lpClassName,
		HINSTANCE hInstance
	) = 0;
};

#define VENGINE_UNICODEINTERFACE_VERSION	"VENGINEUNICODE001"


#endif // UNICODE_H
