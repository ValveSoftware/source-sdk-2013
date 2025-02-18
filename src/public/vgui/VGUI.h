//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic header for using vgui
//
// $NoKeywords: $
//=============================================================================//

#ifndef VGUI_H
#define VGUI_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

#define null 0L

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#pragma warning( disable: 4800 )	// disables 'performance warning converting int to bool'
#pragma warning( disable: 4786 )	// disables 'identifier truncated in browser information' warning
#pragma warning( disable: 4355 )	// disables 'this' : used in base member initializer list
#pragma warning( disable: 4097 )	// warning C4097: typedef-name 'BaseClass' used as synonym for class-name
#pragma warning( disable: 4514 )	// warning C4514: 'Color::Color' : unreferenced inline function has been removed
#pragma warning( disable: 4100 )	// warning C4100: 'code' : unreferenced formal parameter
#pragma warning( disable: 4127 )	// warning C4127: conditional expression is constant

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

#ifndef _WCHAR_T_DEFINED
// DAL - wchar_t is a built in define in gcc 3.2 with a size of 4 bytes
#if !defined( __x86_64__ ) && !defined( __WCHAR_TYPE__  )
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
#endif

// do this in GOLDSRC only!!!
//#define Assert assert

namespace vgui
{
// handle to an internal vgui panel
// this is the only handle to a panel that is valid across dll boundaries
typedef uintp VPANEL;

// handles to vgui objects
// NULL values signify an invalid value
typedef uint32 HScheme;
// Both -1 and 0 are used for invalid textures. Be careful.
typedef uint32 HTexture;
typedef uint32 HCursor;
typedef uint32 HPanel;
const HPanel INVALID_PANEL = 0xffffffff;
typedef uint32 HFont;
const HFont INVALID_FONT = 0; // the value of an invalid font handle
}

#include "tier1/strtools.h"

#if 0 // defined( OSX ) // || defined( LINUX )
// Disabled all platforms. Did a major cleanup of osxfont.cpp, and having this
//  turned off renders much closer to Windows and Linux and also uses the same
//  code paths (which is good).
#define USE_GETKERNEDCHARWIDTH 1
#else
#define USE_GETKERNEDCHARWIDTH 0
#endif


#endif // VGUI_H
