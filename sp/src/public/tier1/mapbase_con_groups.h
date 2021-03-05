//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: See tier1/mapbase_con_groups.cpp for more information
//
// $NoKeywords: $
//=============================================================================

#ifndef CON_VERBOSE_COLORS_H
#define CON_VERBOSE_COLORS_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//static const Color CON_COLOR_DEV_VERBOSE( 192, 128, 192, 255 );

// General
#define CON_GROUP_MAPBASE_MISC  0 // "Mapbase Misc."
#define CON_GROUP_PHYSICS       1 // "Physics"

// Server
#define CON_GROUP_IO_SYSTEM     2 // "Entity I/O"
#define CON_GROUP_NPC_AI        3 // "NPC AI"
#define CON_GROUP_NPC_SCRIPTS   4 // "NPC Scripts"
#define CON_GROUP_CHOREO        5 // "Choreo"

// VScript
#define CON_GROUP_VSCRIPT       6 // "VScript"
#define CON_GROUP_VSCRIPT_PRINT 7 // "VScript Print"

#define CON_GROUP_MAX           8 // must always be at the end

// Mapbase console group message.
void CGMsg( int level, int nGroup, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );

#define CGWarning CGMsg

#endif
