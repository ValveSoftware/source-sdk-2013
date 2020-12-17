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
#include "tier1/tier1.h"
#include "tier1/utldict.h"
#include "Color.h"
#include "tier1/mapbase_con_groups.h"

void CV_ColorChanged( IConVar *pConVar, const char *pOldString, float flOldValue );

struct ConGroup_t
{
	ConGroup_t( const char *_pszName, ConVar *pCvar )
	{
		pszName = _pszName;
		cvar = pCvar;
	}

	const Color &GetColor()
	{
		if (_clr.a() == 0)
		{
			// Read the cvar
			int clr[3];
			sscanf( cvar->GetString(), "%i %i %i", &clr[0], &clr[1], &clr[2] );
			_clr.SetColor( clr[0], clr[1], clr[2], 255 );
		}
		return _clr;
	}

	const char *pszName;
	Color _clr;
	ConVar *cvar;

	//bool bDisabled;
};

//-----------------------------------------------------------------------------
// To define a console group, 
//-----------------------------------------------------------------------------

#define DEFINE_CON_GROUP_CVAR(name, def, desc) static ConVar con_group_##name##_color( "con_group_" #name "_color", def, FCVAR_ARCHIVE | FCVAR_REPLICATED, desc, CV_ColorChanged )

DEFINE_CON_GROUP_CVAR( mapbase_misc, "192 128 224", "Messages from misc. Mapbase functions, like map-specific files." );
DEFINE_CON_GROUP_CVAR( physics, "159 209 159", "Messages from physics-related events." );

DEFINE_CON_GROUP_CVAR( inputoutput, "220 170 220", "Messages from I/O events. (these display in developer 2)" );
DEFINE_CON_GROUP_CVAR( npc_ai, "240 160 200", "Messages from NPC AI, etc. which display at various verbse levels." );
DEFINE_CON_GROUP_CVAR( npc_scripts, "255 115 215", "Messages from scripted_sequence, etc. (these display in developer 2)" );
DEFINE_CON_GROUP_CVAR( choreo, "240 224 180", "Messages from choreographed scenes and response expressers. (these display in developer 1, 2, etc.)" );

DEFINE_CON_GROUP_CVAR( vscript, "192 224 240", "Internal messages from VScript not produced by the user's scripts." );
DEFINE_CON_GROUP_CVAR( vscript_print, "80 186 255", "Messages from VScript's 'print' function." );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define DEFINE_CON_GROUP(name, codename) { name, &con_group_##codename##_color }

ConGroup_t g_ConGroups[] = {

	// General
	DEFINE_CON_GROUP( CON_GROUP_MAPBASE_MISC, mapbase_misc ),
	DEFINE_CON_GROUP( CON_GROUP_PHYSICS, physics ),

	// Server
	DEFINE_CON_GROUP( CON_GROUP_IO_SYSTEM, inputoutput ),
	DEFINE_CON_GROUP( CON_GROUP_NPC_AI, npc_ai ),
	DEFINE_CON_GROUP( CON_GROUP_NPC_SCRIPTS, npc_scripts ),
	DEFINE_CON_GROUP( CON_GROUP_CHOREO, choreo ),

	// VScript
	DEFINE_CON_GROUP( CON_GROUP_VSCRIPT, vscript ),
	DEFINE_CON_GROUP( CON_GROUP_VSCRIPT_PRINT, vscript_print ),

};

void CV_ColorChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	for (int i = 0; i < ARRAYSIZE( g_ConGroups ); i++)
	{
		// Reset the alpha to indicate it needs to be refreshed
		g_ConGroups[i]._clr[3] = 0;
	}
}

ConGroup_t *FindConGroup( const char *pszGroup )
{
	for (int i = 0; i < ARRAYSIZE( g_ConGroups ); i++)
	{
		if (V_strcmp(pszGroup, g_ConGroups[i].pszName) == 0)
			return &g_ConGroups[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

ConVar con_group_include_name( "con_group_include_name", "0", FCVAR_NONE, "Includes groups when printing." );

CON_COMMAND( con_group_list, "Prints a list of all console groups." )
{
	Msg( "============================================================\n" );
	for (int i = 0; i < ARRAYSIZE( g_ConGroups ); i++)
	{
		Msg( "	# " );
		ConColorMsg( g_ConGroups[i].GetColor(), "%s", g_ConGroups[i].pszName );
		Msg( " - %s ", g_ConGroups[i].cvar->GetHelpText() );

		//if (g_ConGroups[i].bDisabled)
		//	Msg("(DISABLED)");

		Msg("\n");
	}
	Msg( "============================================================\n" );
}

// TODO: Figure out how this can be done without server/client disparity issues
/*
CON_COMMAND( con_group_toggle, "Toggles a console group." )
{
	const char *pszGroup = args.Arg( 1 );
	for (int i = 0; i < ARRAYSIZE( g_ConGroups ); i++)
	{
		if (V_stricmp(pszGroup, g_ConGroups[i].pszName) == 0)
		{
			Msg( "%s is now %s\n", g_ConGroups[i].pszName, g_ConGroups[i].bDisabled ? "enabled" : "disabled" );
			g_ConGroups[i].bDisabled = !g_ConGroups[i].bDisabled;
			return;
		}
	}

	Msg( "No group named \"%s\"\n", pszGroup );
}
*/

void CGMsg( int level, const char *pszGroup, const tchar* pMsg, ... )
{
	// Return early if we're not at this level
	if (!IsSpewActive("developer", level))
		return;

	char string[ 2048 ];
	va_list argptr;
	va_start( argptr, pMsg );
	Q_vsnprintf( string, sizeof(string), pMsg, argptr );
	va_end( argptr );

	ConGroup_t *pGroup = FindConGroup( pszGroup );
	if (pGroup)
	{
		/*if (pGroup->bDisabled)
		{
			// Do nothing
		}
		else*/ if (con_group_include_name.GetBool())
		{
			ConColorMsg( level, pGroup->GetColor(), "[%s] %s", pGroup->pszName, string );
		}
		else
		{
			ConColorMsg( level, pGroup->GetColor(), string );
		}
	}
	else
		DevMsg( level, string );
}
