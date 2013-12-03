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
typedef unsigned long  ulong;

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
typedef unsigned int VPANEL;

// handles to vgui objects
// NULL values signify an invalid value
typedef unsigned long HScheme;
// Both -1 and 0 are used for invalid textures. Be careful.
typedef unsigned long HTexture;
typedef unsigned long HCursor;
typedef unsigned long HPanel;
const HPanel INVALID_PANEL = 0xffffffff;
typedef unsigned long HFont;
const HFont INVALID_FONT = 0; // the value of an invalid font handle
}

#include "tier1/strtools.h"

#if defined( OSX ) // || defined( LINUX )
// Set to 1 to use GetKernedCharWidth() instead of GetCharABCwide(). Alfred
//  initially started using that code on the Mac because it did better
//  kerning, but he was a leery about switching win32 over. I enabled this
//  for Linux, but it causes some strings to look different than Windows. So
//  I've disabled it for now. mikesart - 12/2012.
#define USE_GETKERNEDCHARWIDTH 1
#else
#define USE_GETKERNEDCHARWIDTH 0
#endif


#endif // VGUI_H
