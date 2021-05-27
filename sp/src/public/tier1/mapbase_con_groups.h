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

enum ConGroupID_t
{
	// General
	CON_GROUP_MAPBASE_MISC,      // "Mapbase misc."
	CON_GROUP_PHYSICS,           // "Physics"
	CON_GROUP_IO_SYSTEM,         // "Entity I/O"
	CON_GROUP_RESPONSE_SYSTEM,   // "Response System"

	// Game
	CON_GROUP_NPC_AI,            // "NPC AI"
	CON_GROUP_NPC_SCRIPTS,       // "NPC scripts"
	CON_GROUP_SPEECH_AI,         // "Speech AI"
	CON_GROUP_CHOREO,            // "Choreo"

	// VScript
	CON_GROUP_VSCRIPT,           // "VScript"
	CON_GROUP_VSCRIPT_PRINT,     // "VScript print"

	//--------------------------

	// 
	// Mod groups can be inserted here
	// 

	//--------------------------

	CON_GROUP_MAX, // Keep this at the end
};

// Mapbase console group message.
void CGMsg( int level, ConGroupID_t nGroup, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );

#define CGWarning CGMsg

//-----------------------------------------------------------------------------

class IBaseFileSystem;

void InitConsoleGroups( IBaseFileSystem *filesystem );

void PrintAllConsoleGroups();
void ToggleConsoleGroups( const char *pszQuery );
void SetConsoleGroupIncludeNames( bool bToggle );

#endif
