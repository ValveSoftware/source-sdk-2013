//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PROTECTED_THINGS_H
#define PROTECTED_THINGS_H
#ifdef _WIN32
#pragma once
#endif


// This header tries to prevent people from using potentially dangerous functions
// (like the notorious non-null-terminating strncpy) and functions that will break
// VCR mode (like time, input, registry, etc).
//
// This header should be included by ALL of our source code.

// Eventually, ALL of these should be protected, but one man can only accomplish so much in
// one day AND work on features too!
#if defined( PROTECTED_STRINGS_ENABLE ) && !defined(DISABLE_PROTECTED_STRINGS)

	#if defined( printf )
		#undef printf
	#endif
	#define printf				printf__HEY_YOU__USE_VSTDLIB
	
	#if defined( wprintf )
		#undef wprintf
	#endif
	#define wprintf				wprintf__HEY_YOU__USE_VSTDLIB
	
	#if defined( strcmp )
		#undef strcmp
	#endif
	#define strcmp				strcmp__HEY_YOU__USE_VSTDLIB
	
	#if defined( wcscmp )
		#undef wcscmp
	#endif
	#define wcscmp				wcscmp__HEY_YOU__USE_VSTDLIB
	
	#if defined( strncpy )
		#undef strncpy
	#endif
	#define strncpy				strncpy__HEY_YOU__USE_VSTDLIB
	
	#if defined( wcsncpy )
		#undef wcsncpy
	#endif
	#define wcsncpy				wcsncpy__HEY_YOU__USE_VSTDLIB
	
	#if defined( strlen )
		#undef strlen
	#endif
	#define strlen				strlen__HEY_YOU__USE_VSTDLIB
	
	#if defined( wcslen )
		#undef wcslen
	#endif
	#define wcslen				wcslen__HEY_YOU__USE_VSTDLIB
	
	#if defined( Q_strlen )
		#undef Q_strlen
	#endif
	#define Q_strlen			Q_strlen__HEY_YOU__USE_VSTDLIB
	
	#if defined( _snprintf )
		#undef _snprintf
	#endif
	#define _snprintf			snprintf__HEY_YOU__USE_VSTDLIB
	
	#if defined( _snwprintf )
		#undef _snwprintf
	#endif
	#define _snwprintf			snwprintf__HEY_YOU__USE_VSTDLIB
	
	#if defined( sprintf )
		#undef sprintf
	#endif
	#define sprintf				sprintf__HEY_YOU__USE_VSTDLIB

	#if defined( swprintf )
		#undef swprintf
	#endif
	#define swprintf			swprintf__HEY_YOU__USE_VSTDLIB

	#if defined( vsprintf )
		#undef vsprintf
	#endif
	#define vsprintf			vsprintf__HEY_YOU__USE_VSTDLIB

	#if defined( vswprintf )
		#undef vswprintf
	#endif
	#define vswprintf			vswprintf__HEY_YOU__USE_VSTDLIB

	#if defined( _vsnprintf )
		#undef _vsnprintf
	#endif
	#define _vsnprintf			vsnprintf__HEY_YOU__USE_VSTDLIB
	
	#if defined( _vsnwprintf )
		#undef _vsnwprintf
	#endif
	#define _vsnwprintf			vsnwprintf__HEY_YOU__USE_VSTDLIB
	
	#if defined( strcat )
		#undef strcat
	#endif
	#define strcat				strcat__HEY_YOU__USE_VSTDLIB

	#if defined( wcscat )
		#undef wcscat
	#endif
	#define wcscat				wcscat__HEY_YOU__USE_VSTDLIB

	#if defined( strncat )
		#undef strncat
	#endif
	#define strncat				strncat__HEY_YOU__USE_VSTDLIB

	#if defined( wcsncat )
		#undef wcsncat
	#endif
	#define wcsncat				wcsncat__HEY_YOU__USE_VSTDLIB

#endif


#if defined( PROTECTED_THINGS_ENABLE ) && !defined( _X360 ) && !defined(DISABLE_PROTECTED_THINGS)

	#if defined( GetTickCount )
		#undef GetTickCount
	#endif
	#define GetTickCount		GetTickCount__USE_VCR_MODE
	
	
	#if defined( timeGetTime )
		#undef timeGetTime
	#endif
	#define timeGetTime			timeGetTime__USE_VCR_MODE
	#if defined( clock )
		#undef clock
	#endif
	#define time				time__USE_VCR_MODE
	
	
	#if defined( recvfrom )
		#undef recvfrom
	#endif
	#define recvfrom			recvfrom__USE_VCR_MODE


	#if defined( GetCursorPos )
		#undef GetCursorPos
	#endif
	#define GetCursorPos		GetCursorPos__USE_VCR_MODE
	
	
	#if defined( ScreenToClient )
		#undef ScreenToClient
	#endif
	#define ScreenToClient		ScreenToClient__USE_VCR_MODE
	
	
	#if defined( GetCommandLine )
		#undef GetCommandLine
	#endif
	#define GetCommandLine		GetCommandLine__USE_VCR_MODE
	
	
	#if defined( RegOpenKeyEx )
		#undef RegOpenKeyEx
	#endif
	#define RegOpenKeyEx		RegOpenKeyEx__USE_VCR_MODE
	
	
	#if defined( RegOpenKey )
		#undef RegOpenKey
	#endif
	#define RegOpenKey			RegOpenKey__USE_VCR_MODE
	
	
	#if defined( RegSetValueEx )
		#undef RegSetValueEx
	#endif
	#define RegSetValueEx		RegSetValueEx__USE_VCR_MODE
	
	
	#if defined( RegSetValue )
		#undef RegSetValue
	#endif
	#define RegSetValue			RegSetValue__USE_VCR_MODE
	
	
	#if defined( RegQueryValueEx )
		#undef RegQueryValueEx
	#endif		
	#define RegQueryValueEx		RegQueryValueEx__USE_VCR_MODE
	
	
	#if defined( RegQueryValue )
		#undef RegQueryValue
	#endif		
	#define RegQueryValue		RegQueryValue__USE_VCR_MODE
	
	
	#if defined( RegCreateKeyEx )
		#undef RegCreateKeyEx
	#endif
	#define RegCreateKeyEx		RegCreateKeyEx__USE_VCR_MODE
	
	
	#if defined( RegCreateKey )
		#undef RegCreateKey
	#endif
	#define RegCreateKey		RegCreateKey__USE_VCR_MODE
	
	
	#if defined( RegCloseKey )
		#undef RegCloseKey
	#endif
	#define RegCloseKey			RegCloseKey__USE_VCR_MODE
	
	
	#if defined( GetNumberOfConsoleInputEvents )
		#undef GetNumberOfConsoleInputEvents
	#endif
	#define GetNumberOfConsoleInputEvents	GetNumberOfConsoleInputEvents__USE_VCR_MODE
	
	
	#if defined( ReadConsoleInput )
		#undef ReadConsoleInput
	#endif
	#define ReadConsoleInput	ReadConsoleInput__USE_VCR_MODE


	#if defined( GetAsyncKeyState )
		#undef GetAsyncKeyState
	#endif
	#define GetAsyncKeyState	GetAsyncKeyState__USE_VCR_MODE

	
	#if defined( GetKeyState )
		#undef GetKeyState
	#endif
	#define GetKeyState			GetKeyState__USE_VCR_MODE


	#if defined( CreateThread )
		#undef CreateThread
	#endif
	#define CreateThread		CreateThread__USE_VCR_MODE

	#if defined( WaitForSingleObject )
		#undef WaitForSingleObject
	#endif
	#define WaitForSingleObject	WaitForSingleObject__USE_VCR_MODE

	#if defined( EnterCriticalSection )
		#undef EnterCriticalSection
	#endif
	#define EnterCriticalSection EnterCriticalSection__USE_VCR_MODE

#endif


#endif // PROTECTED_THINGS_H
