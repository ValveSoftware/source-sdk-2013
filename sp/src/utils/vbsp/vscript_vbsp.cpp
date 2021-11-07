//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase's implementation of VScript in VBSP, allowing users to modify map compilation behavior with scripts.
//
// $NoKeywords: $
//=============================================================================//

#include "tier1/KeyValues.h"
#include "tier1/fmtstr.h"

#include "vbsp.h"
#include "map.h"
#include "fgdlib/fgdlib.h"
#include "convar.h"

#include "vscript_vbsp.h"
#include "vscript_vbsp.nut"
#include "vscript_funcs_vmfs.h"
#include "vscript_funcs_vis.h"

IScriptVM *g_pScriptVM;
IScriptManager *scriptmanager = NULL;

extern ScriptLanguage_t	g_iScripting;

extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

// This is to ensure a dependency exists between the vscript library and the game DLLs
extern int vscript_token;
int vscript_token_hack = vscript_token;

// HACKHACK: VScript library relies on developer convar existing
ConVar developer( "developer", "1", 0, "Set developer message level." ); // developer mode

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return NULL;
	}

	static const char *pszExtensions[] =
	{
		"",		// SL_NONE
		".gm",	// SL_GAMEMONKEY
		".nut",	// SL_SQUIRREL
		".lua", // SL_LUA
		".py",  // SL_PYTHON
	};

	const char *pszVMExtension = pszExtensions[g_pScriptVM->GetLanguage()];
	const char *pszIncomingExtension = V_strrchr( pszScriptName , '.' );
	if ( pszIncomingExtension && V_strcmp( pszIncomingExtension, pszVMExtension ) != 0 )
	{
		Warning( "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath = pszScriptName;
	}
	else
	{	
		scriptPath.sprintf( "%s%s", pszScriptName,  pszVMExtension );
	}

	const char *pBase;
	CUtlBuffer bufferScript;

	if ( g_pScriptVM->GetLanguage() == SL_PYTHON )
	{
		// python auto-loads raw or precompiled modules - don't load data here
		pBase = NULL;
	}
	else
	{
		/*
		FileFindHandle_t handle = NULL;
		const char *file = g_pFullFileSystem->FindFirst( "*", &handle );
		while (file)
		{
			Msg( "File in this directory: %s\n", file );
			file = g_pFullFileSystem->FindNext(handle);
		}

		Msg( "File exists: %d\n", g_pFullFileSystem->FileExists( scriptPath ) );
		*/


		bool bResult = g_pFullFileSystem->ReadFile( scriptPath, NULL, bufferScript );

		if ( !bResult && bWarnMissing )
		{
			Warning( "Script not found (%s) \n", scriptPath.operator const char *() );
			Assert( "Error running script" );
		}

		pBase = (const char *) bufferScript.Base();

		if ( !pBase || !*pBase )
		{
			return NULL;
		}
	}


	const char *pszFilename = V_strrchr( scriptPath, '\\' );
	pszFilename++;
	HSCRIPT hScript = g_pScriptVM->CompileScript( pBase, pszFilename );
	if ( !hScript )
	{
		Warning( "FAILED to compile and execute script file named %s\n", scriptPath.operator const char *() );
		Assert( "Error running script" );
	}
	return hScript;
}

static int g_ScriptVBSPRunScriptDepth;

bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	if ( !pszScriptName || !*pszScriptName )
	{
		Warning( "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptVBSPRunScriptDepth > 16 )
	{
		Warning( "IncludeScript stack overflow\n" );
		return false;
	}

	g_ScriptVBSPRunScriptDepth++;
	HSCRIPT	hScript = VScriptCompileScript( pszScriptName, bWarnMissing );
	bool bSuccess = false;
	if ( hScript )
	{
		bSuccess = ( g_pScriptVM->Run( hScript, hScope ) != SCRIPT_ERROR );
		if ( !bSuccess )
		{
			Warning( "Error running script named %s\n", pszScriptName );
			Assert( "Error running script" );
		}
	}
	g_ScriptVBSPRunScriptDepth--;
	return bSuccess;
}

ScriptHook_t	CMapFile::g_Hook_OnMapLoaded;

BEGIN_SCRIPTDESC_ROOT( CMapFile, "Map file" )

	DEFINE_SCRIPTFUNC( GetMins, "Get the map's mins." )
	DEFINE_SCRIPTFUNC( GetMaxs, "Get the map's maxs." )

	DEFINE_SCRIPTFUNC( GetEntityOrigin, "Get the origin of the entity with the specified index." )
	DEFINE_SCRIPTFUNC( GetEntityFirstBrush, "Get the first brush ID of the entity with the specified index." )
	DEFINE_SCRIPTFUNC( GetEntityNumBrushes, "Get the number of brushes in the entity with the specified index." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetEntityKeyValues, "GetEntityKeyValues", "Export an entity's keyvalues to two arrays." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptAddSimpleEntityKV, "AddSimpleEntityKV", "Add a simple entity from a keyvalue table." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddInstance, "AddInstance", "Add an instance to the map." )

	DEFINE_SCRIPTFUNC( GetNumEntities, "Get the number of entities in the map." )

	// 
	// Hooks
	// 
	DEFINE_SIMPLE_SCRIPTHOOK( CMapFile::g_Hook_OnMapLoaded, "OnMapLoaded", FIELD_VOID, "Called when the NPC is deciding whether to hear a CSound or not." )

END_SCRIPTDESC();


static float cvar_getf( const char* sz )
{
	ConVarRef cvar(sz);
	if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
		return NULL;
	return cvar.GetFloat();
}

static bool cvar_setf( const char* sz, float val )
{
	ConVarRef cvar(sz);
	if ( !cvar.IsValid() )
		return false;

	if ( cvar.IsFlagSet( FCVAR_SERVER_CANNOT_QUERY ) )
		return false;

	cvar.SetValue(val);
	return true;
}

static const char *GetSource()
{
	return source;
}

static const char *GetMapBase()
{
	return mapbase;
}

static HSCRIPT GetMainMap()
{
	return g_MainMap ? g_MainMap->GetScriptInstance() : NULL;
}

static HSCRIPT GetLoadingMap()
{
	return g_LoadingMap ? g_LoadingMap->GetScriptInstance() : NULL;
}

static const char *DoUniqueString( const char *pszBase )
{
	static char szBuf[512];
	g_pScriptVM->GenerateUniqueKey( pszBase, szBuf, ARRAYSIZE(szBuf) );
	return szBuf;
}

bool DoIncludeScript( const char *pszScript, HSCRIPT hScope )
{
	if ( !VScriptRunScript( pszScript, hScope, true ) )
	{
		g_pScriptVM->RaiseException( CFmtStr( "Failed to include script \"%s\"", ( pszScript ) ? pszScript : "unknown" ) );
		return false;
	}
	return true;
}

extern qboolean	glview;
extern qboolean	onlyents;
extern bool		onlyprops;
extern qboolean	noleaktest;
extern qboolean	verboseentities;
extern qboolean	g_bLowPriority;
extern bool		g_bKeepStaleZip;
extern bool		g_bNoDefaultCubemaps;
extern bool		g_bSkyboxCubemaps;
extern int		g_iDefaultCubemapSize;

bool VScriptVBSPInit()
{
	VMPROF_START

	if( g_iScripting != SL_NONE && scriptmanager != NULL )
	{
		if ( g_pScriptVM == NULL )
			g_pScriptVM = scriptmanager->CreateVM( g_iScripting );

		if( g_pScriptVM )
		{
			Log( "VSCRIPT VBSP: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );

			ScriptRegisterFunction( g_pScriptVM, cvar_getf, "Gets the value of the given cvar, as a float." );
			ScriptRegisterFunction( g_pScriptVM, cvar_setf, "Sets the value of the given cvar, as a float." );

			ScriptRegisterFunction( g_pScriptVM, GetSource, "Gets the base directory of the first map loaded." );
			ScriptRegisterFunction( g_pScriptVM, GetMapBase, "Gets the base name of the first map loaded." );
			ScriptRegisterFunction( g_pScriptVM, GetMainMap, "Gets the first map loaded." );
			ScriptRegisterFunction( g_pScriptVM, GetLoadingMap, "Gets the map which is currently loading (e.g. an instance)." );

			ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string. Useful for adding data to tables when not sure what keys are already in use in that table." ) );
			ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );

			ScriptRegisterConstantNamed( g_pScriptVM, GameData::NAME_FIXUP_PREFIX, "NAME_FIXUP_PREFIX", "Prefix name fixup" );
			ScriptRegisterConstantNamed( g_pScriptVM, GameData::NAME_FIXUP_POSTFIX, "NAME_FIXUP_PREFIX", "Postfix name fixup" );
			ScriptRegisterConstantNamed( g_pScriptVM, GameData::NAME_FIXUP_NONE, "NAME_FIXUP_NONE", "No name fixup" );

			ScriptRegisterConstant( g_pScriptVM, microvolume, "" );
			ScriptRegisterConstant( g_pScriptVM, noprune, "" );
			ScriptRegisterConstant( g_pScriptVM, glview, "" );
			ScriptRegisterConstant( g_pScriptVM, nodetail, "" );
			ScriptRegisterConstant( g_pScriptVM, fulldetail, "" );
			ScriptRegisterConstant( g_pScriptVM, onlyents, "" );
			ScriptRegisterConstant( g_pScriptVM, onlyprops, "" );
			ScriptRegisterConstant( g_pScriptVM, nomerge, "" );
			ScriptRegisterConstant( g_pScriptVM, nomergewater, "" );
			ScriptRegisterConstant( g_pScriptVM, nowater, "" );
			ScriptRegisterConstant( g_pScriptVM, nocsg, "" );
			ScriptRegisterConstant( g_pScriptVM, noweld, "" );
			ScriptRegisterConstant( g_pScriptVM, noshare, "" );
			ScriptRegisterConstant( g_pScriptVM, nosubdiv, "" );
			ScriptRegisterConstant( g_pScriptVM, notjunc, "" );
			ScriptRegisterConstant( g_pScriptVM, noopt, "" );
			ScriptRegisterConstant( g_pScriptVM, noleaktest, "" );
			ScriptRegisterConstant( g_pScriptVM, verboseentities, "" );
			ScriptRegisterConstant( g_pScriptVM, dumpcollide, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bLowPriority, "" );
			ScriptRegisterConstant( g_pScriptVM, g_DumpStaticProps, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bSkyVis, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bLightIfMissing, "" );
			ScriptRegisterConstant( g_pScriptVM, g_snapAxialPlanes, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bKeepStaleZip, "" );
			ScriptRegisterConstant( g_pScriptVM, g_NodrawTriggers, "" );
			ScriptRegisterConstant( g_pScriptVM, g_DisableWaterLighting, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bAllowDetailCracks, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bNoVirtualMesh, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bNoHiddenManifestMaps, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bNoDefaultCubemaps, "" );
			ScriptRegisterConstant( g_pScriptVM, g_bSkyboxCubemaps, "" );
			ScriptRegisterConstant( g_pScriptVM, g_iDefaultCubemapSize, "" );

			if (g_iScripting == SL_SQUIRREL)
			{
				g_pScriptVM->Run( g_Script_vscript_vbsp );
			}

			RegisterVMFScriptFunctions();

			// Run the map's script
			char script[96];
			Q_snprintf( script, sizeof(script), "%s_vbsp", source );
			//Msg("VBSP script: \"%s\"\n", script);
			VScriptRunScript( script, true );

			VMPROF_SHOW( g_iScripting, "virtual machine startup" );

			return true;
		}
		else
		{
			DevWarning("VM Did not start!\n");
		}
	}
	else
	{
		Log( "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptVBSPTerm()
{
	if( g_pScriptVM != NULL )
	{
		if( g_pScriptVM )
		{
			scriptmanager->DestroyVM( g_pScriptVM );
			g_pScriptVM = NULL;
		}
	}
}
