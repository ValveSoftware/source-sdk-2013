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
#define CON_GROUP_MAPBASE_MISC "Mapbase Misc."
#define CON_GROUP_PHYSICS "Physics"

// Server
#define CON_GROUP_IO_SYSTEM "I/O System"
#define CON_GROUP_NPC_AI "NPC AI"
#define CON_GROUP_NPC_SCRIPTS "NPC Scripts"
#define CON_GROUP_CHOREO "Choreo"

// VScript
#define CON_GROUP_VSCRIPT "VScript"
#define CON_GROUP_VSCRIPT_PRINT "VScript Print"

// Mapbase console group message.
void CGMsg( int level, const char *pszGroup, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );

#define CGWarning CGMsg

#endif
