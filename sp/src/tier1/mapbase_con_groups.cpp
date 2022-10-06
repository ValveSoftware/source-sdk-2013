//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 =================
//
// Purpose: Mapbase classifies certain types of console messages into groups with specific colors.
// 
//			This is inspired by similar groups seen in CS:GO and Source 2 games.
//
// $NoKeywords: $
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basetypes.h"
#include "tier1.h"
#include "utldict.h"
#include "Color.h"
#include "mapbase_con_groups.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "mapbase_matchers_base.h"

struct ConGroup_t
{
	ConGroup_t( const char *_pszName, const char *_pszDescription )
	{
		pszName = _pszName;
		pszDescription = _pszDescription;
		_clr.SetColor( 224, 224, 224, 255 ); // Default to a shade of gray
	}

	const Color &GetColor()
	{
		return _clr;
	}

	const char *pszName;
	const char *pszDescription;
	Color _clr;

	bool bDisabled;
};

// TODO: Something more reliable?
static bool g_bIncludeConGroupNames = false;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//#define DEFINE_CON_GROUP(id, name, codename) { name, &con_group_##codename##_color }
#define DEFINE_CON_GROUP(id, name, description) { name, description }

ConGroup_t g_ConGroups[CON_GROUP_MAX] = {

	// General
	DEFINE_CON_GROUP( CON_GROUP_MAPBASE_MISC, "Mapbase misc.", "Messages from misc. Mapbase functions, like map-specific files." ),
	DEFINE_CON_GROUP( CON_GROUP_PHYSICS, "Physics", "Messages from physics-related events." ),
	DEFINE_CON_GROUP( CON_GROUP_IO_SYSTEM, "Entity IO", "Messages from I/O events. (these display in developer 2)" ),
	DEFINE_CON_GROUP( CON_GROUP_RESPONSE_SYSTEM, "Response System", "Messages from the Response System, a library primarily used for NPC speech." ),

	// Game
	DEFINE_CON_GROUP( CON_GROUP_NPC_AI, "NPC AI", "Messages from NPC AI, etc. which display at various verbose levels." ),
	DEFINE_CON_GROUP( CON_GROUP_NPC_SCRIPTS, "NPC scripts", "Messages from scripted_sequence, etc. (these display in developer 2)" ),
	DEFINE_CON_GROUP( CON_GROUP_SPEECH_AI, "Speech AI", "Messages from response expressers. (these display in developer 1, 2, etc.)" ),
	DEFINE_CON_GROUP( CON_GROUP_CHOREO, "Choreo", "Messages from choreographed scenes. (these display in developer 1, 2, etc.)" ),

	// VScript
	DEFINE_CON_GROUP( CON_GROUP_VSCRIPT, "VScript", "Internal messages from VScript not produced by actual scripts." ),
	DEFINE_CON_GROUP( CON_GROUP_VSCRIPT_PRINT, "VScript print", "Messages from VScript's 'print' function." ),

};

int FindConGroup( const char *pszName )
{
	for (int i = 0; i < CON_GROUP_MAX; i++)
	{
		if (Q_stricmp( pszName, g_ConGroups[i].pszName ) == 0)
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Loads console groups
//-----------------------------------------------------------------------------
void LoadConsoleGroupsFromFile( IBaseFileSystem *filesystem, const char *pszFileName, const char *pathID )
{
	KeyValues *pGroupRoot = new KeyValues( "ConsoleGroups" );

	pGroupRoot->LoadFromFile( filesystem, pszFileName, pathID );

	KeyValues *pGroup = NULL;
	for ( pGroup = pGroupRoot->GetFirstTrueSubKey(); pGroup; pGroup = pGroup->GetNextTrueSubKey() )
	{
		int index = FindConGroup( pGroup->GetName() );
		if (index != -1)
		{
			Color msgClr = pGroup->GetColor( "MessageColor" );

			// Make sure the color isn't 0,0,0,0 before assigning
			if (msgClr.GetRawColor() != 0)
				g_ConGroups[index]._clr = msgClr;

			g_ConGroups[index].bDisabled = pGroup->GetBool( "Disabled", false );
		}
		else
		{
			Warning( "Invalid console group %s (new groups should be defined in the code)\n", pGroup->GetName() );
		}
	}

	pGroupRoot->deleteThis();
}

void InitConsoleGroups( IBaseFileSystem *filesystem )
{
	LoadConsoleGroupsFromFile( filesystem, "scripts/mapbase_con_groups.txt", "MOD" );
	LoadConsoleGroupsFromFile( filesystem, "scripts/mod_con_groups.txt", "MOD" );
}

void PrintAllConsoleGroups()
{
	Msg( "============================================================\n" );
	for (int i = 0; i < CON_GROUP_MAX; i++)
	{
		ConColorMsg( g_ConGroups[i].GetColor(), "	# %s", g_ConGroups[i].pszName );

		if (g_ConGroups[i].bDisabled)
			Msg(" [DISABLED]");

		Msg( " - %s ", g_ConGroups[i].pszDescription );

		Msg("\n");
	}
	Msg( "============================================================\n" );
}

void ToggleConsoleGroups( const char *pszQuery )
{
	bool bMatched = false;

	for (int i = 0; i < ARRAYSIZE( g_ConGroups ); i++)
	{
		if (Matcher_NamesMatch( pszQuery, g_ConGroups[i].pszName ))
		{
			Msg( "%s is now %s\n", g_ConGroups[i].pszName, g_ConGroups[i].bDisabled ? "enabled" : "disabled" );
			g_ConGroups[i].bDisabled = !g_ConGroups[i].bDisabled;
			bMatched = true;
		}
	}

	if (!bMatched)
		Msg( "No groups matching \"%s\"\n", pszQuery );
}

void SetConsoleGroupIncludeNames( bool bToggle )
{
	g_bIncludeConGroupNames = bToggle;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CGMsg( int level, ConGroupID_t nGroup, const tchar* pMsg, ... )
{
	// Return early if we're not at this level
	if (!IsSpewActive("developer", level))
		return;

	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, pMsg );
	Q_vsnprintf( string, sizeof(string), pMsg, argptr );
	va_end( argptr );

	Assert( nGroup >= 0 );
	Assert( nGroup < CON_GROUP_MAX );

	ConGroup_t *pGroup = &g_ConGroups[nGroup];

	if (pGroup->bDisabled)
	{
		// Do nothing
	}
	else if (g_bIncludeConGroupNames)
	{
		ConColorMsg(level, pGroup->GetColor(), "[%s] %s", pGroup->pszName, string);
	}
	else
	{
		ConColorMsg(level, pGroup->GetColor(), "%s", string);
	}
}
