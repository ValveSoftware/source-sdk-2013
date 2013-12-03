//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	This file sets all of our debugging flags.  It should be 
//			called before all other header files.
//
// $NoKeywords: $
//=============================================================================//

#ifndef DBGFLAG_H
#define DBGFLAG_H
#ifdef _WIN32
#pragma once
#endif


// Here are all the flags we support:
// DBGFLAG_MEMORY:			Enables our memory debugging system, which overrides malloc & free
// DBGFLAG_MEMORY_NEWDEL:	Enables new / delete tracking for memory debug system.  Requires DBGFLAG_MEMORY to be enabled.
// DBGFLAG_VALIDATE:		Enables our recursive validation system for checking integrity and memory leaks
// DBGFLAG_ASSERT:			Turns Assert on or off (when off, it isn't compiled at all)
// DBGFLAG_ASSERTFATAL:		Turns AssertFatal on or off (when off, it isn't compiled at all)
// DBGFLAG_ASSERTDLG:		Turns assert dialogs on or off and debug breaks on or off when not under the debugger.
//								(Dialogs will always be on when process is being debugged.)
// DBGFLAG_STRINGS:			Turns on hardcore string validation (slow but safe)

#undef DBGFLAG_MEMORY
#undef DBGFLAG_MEMORY_NEWDEL
#undef DBGFLAG_VALIDATE
#undef DBGFLAG_ASSERT
#undef DBGFLAG_ASSERTFATAL
#undef DBGFLAG_ASSERTDLG
#undef DBGFLAG_STRINGS

//-----------------------------------------------------------------------------
// Default flags for debug builds
//-----------------------------------------------------------------------------
#if (defined( _DEBUG ) || defined( RELEASEASSERTS ) )

#define DBGFLAG_MEMORY
#ifdef _SERVER	// only enable new & delete tracking for server; on client it conflicts with CRT mem leak tracking
#define DBGFLAG_MEMORY_NEWDEL	
#endif
#ifdef STEAM
#define DBGFLAG_VALIDATE
#endif
#define DBGFLAG_ASSERT
#define DBGFLAG_ASSERTFATAL
#define DBGFLAG_ASSERTDLG
#define DBGFLAG_STRINGS


//-----------------------------------------------------------------------------
// Default flags for release builds
//-----------------------------------------------------------------------------
#else // _DEBUG

#ifdef STEAM
#define DBGFLAG_ASSERT
#endif
#define DBGFLAG_ASSERTFATAL		// note: fatal asserts are enabled in release builds
#define DBGFLAG_ASSERTDLG

#endif // _DEBUG

#endif // DBGFLAG_H
