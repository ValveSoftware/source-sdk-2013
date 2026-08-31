//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_shared.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "characterset.h"
#include "isaverestore.h"
#include "gamerules.h"

#if defined(CLIENT_DLL) && defined(PANORAMA_ENABLE)
#include "panorama/uijsregistration.h"
#endif

IScriptVM * g_pScriptVM;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_VScript, "VScript", 0, LS_MESSAGE, Color( 245, 175, 238, 255 ) );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START double debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

#ifdef CLIENT_DLL
/*class CPanoramaVScript
{
public:
	void RegisterVariable( const char *pszName, const char *pszInitialValue, const char *pszDesc )
	{
		KeyValues* pKey = m_KeyValues.CreateKey( pszName );
		pKey->SetStringValue(pszInitialValue);

		panorama::RegisterJSAccessorReadOnly(pszName, PANORAMA_DELEGATE(delegate), pszDesc);
	}

	void SetValue( const char *pszName, const char *pszValue )
	{
		m_KeyValues.SetBool( pszName, pszValue );
	}
private:
	KeyValues m_KeyValues;
};*/
#endif

#define SCRIPT_PATH "scripts/vscripts"

static const char *g_pszScriptExtensions[] =
{
	"",		// SL_NONE
	".gm",	// SL_GAMEMONKEY
	".nut",	// SL_SQUIRREL
	".lua", // SL_LUA
	".py",  // SL_PYTHON
};

HSCRIPT VScriptCompileScript( const char *pszScriptName, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return NULL;
	}

	const char *pszVMExtension = g_pszScriptExtensions[g_pScriptVM->GetLanguage()];
	const char *pszIncomingExtension = V_strrchr( pszScriptName , '.' );
	if ( pszIncomingExtension && V_strcmp( pszIncomingExtension, pszVMExtension ) != 0 )
	{
		Log_Warning( LOG_VScript, "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath.sprintf( SCRIPT_PATH "/%s", pszScriptName );
	}
	else
	{	
		scriptPath.sprintf( SCRIPT_PATH "/%s%s", pszScriptName,  pszVMExtension );
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
		bool bResult = filesystem->ReadFile( scriptPath, "GAME", bufferScript );

		if( !bResult )
		{
			Log_Warning( LOG_VScript, "Script not found (%s) \n", scriptPath.operator const char *() );
			Assert( "Error running script" );
		}

		pBase = (const char *) bufferScript.Base();

		if ( !pBase || !*pBase )
		{
			return NULL;
		}
	}


	const char *pszFilename = V_strrchr( scriptPath, '/' );
	pszFilename++;
	HSCRIPT hScript = g_pScriptVM->CompileScript( pBase, pszFilename );
	if ( !hScript )
	{
		Log_Warning( LOG_VScript, "FAILED to compile and execute script file named %s\n", scriptPath.operator const char *() );
		Assert( "Error running script" );
	}
	return hScript;
}

static int g_ScriptServerRunScriptDepth;

bool VScriptRunScript( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM )
	{
		return false;
	}

	if ( !pszScriptName || !*pszScriptName )
	{
		Log_Warning( LOG_VScript, "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptServerRunScriptDepth > 16 )
	{
		Log_Warning( LOG_VScript, "IncludeScript stack overflow\n" );
		return false;
	}

	g_ScriptServerRunScriptDepth++;
	HSCRIPT	hScript = VScriptCompileScript( pszScriptName, bWarnMissing );
	bool bSuccess = false;
	if ( hScript )
	{
#ifdef GAME_DLL
		if ( gpGlobals->maxClients == 1 )
		{
			CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				g_pScriptVM->SetValue( "player", pPlayer->GetScriptInstance() );
			}
		}
#endif
		bSuccess = ( g_pScriptVM->Run( hScript, hScope ) != SCRIPT_ERROR );
		if ( !bSuccess )
		{
			Log_Warning( LOG_VScript, "Error running script named %s\n", pszScriptName );
			Assert( "Error running script" );
		}
	}
	g_ScriptServerRunScriptDepth--;
	return bSuccess;
}

#ifdef CLIENT_DLL
CON_COMMAND( script_client, "Run the text as a script" )
#else
CON_COMMAND( script, "Run the text as a script" )
#endif
{
#ifdef CLIENT_DLL
	if ( !engine->IsClientLocalToActiveServer() )
		return;
#else
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !*args[1] )
	{
		Log_Warning( LOG_VScript, "No function name specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args.GetCommandString();

#ifdef CLIENT_DLL
	pszScript += 13;
#else
	pszScript += 6;
#endif
	
	while ( *pszScript == ' ' )
	{
		pszScript++;
	}

	if ( !*pszScript )
	{
		return;
	}

	if ( *pszScript != '\"' )
	{
		g_pScriptVM->Run( pszScript );
	}
	else
	{
		pszScript++;
		const char *pszEndQuote = pszScript;
		while ( *pszEndQuote !=  '\"' )
		{
			pszEndQuote++;
		}
		if ( !*pszEndQuote )
		{
			return;
		}
		*((char *)pszEndQuote) = 0;
		g_pScriptVM->Run( pszScript );
		*((char *)pszEndQuote) = '\"';
	}
}

static void ScriptFileAutocompleteRecursive( const char* pDirectory, const char* pExtension, 
	const char* pMatch, int& nMatches, const char* pCommandName, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	char szDirectory[MAX_PATH];
    V_snprintf( szDirectory, sizeof(szDirectory), "%s/*", pDirectory );

	FileFindHandle_t hFile;
	const char *pFileName = filesystem->FindFirst( szDirectory, &hFile );
	while ( pFileName )
	{
		if ( *pFileName != '.' )
		{
			char szCompleteFilename[MAX_PATH];
			V_snprintf( szCompleteFilename, sizeof(szCompleteFilename), "%s/%s", pDirectory, pFileName );
			if ( g_pFullFileSystem->FindIsDirectory( hFile ) )
			{
				ScriptFileAutocompleteRecursive( szCompleteFilename, pExtension, pMatch, nMatches, pCommandName, commands );
			}
			else
			{
				const char *pFileExtension = V_strrchr( pFileName, '.' );
				if ( pFileExtension && !V_stricmp( pFileExtension, pExtension ) )
				{
					if ( V_strstr( pFileName, pMatch ) )
						V_snprintf( commands[nMatches++], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", 
							pCommandName, szCompleteFilename + sizeof(SCRIPT_PATH) );
				}
			}
		}

		if ( nMatches >= COMMAND_COMPLETION_MAXITEMS )
			break;

		pFileName = filesystem->FindNext( hFile );
	}
	filesystem->FindClose( hFile );
}

static int script_execute_autocomplete( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int nMatches = 0;
	if ( g_pScriptVM )
	{
		const char* pExtension = g_pszScriptExtensions[ g_pScriptVM->GetLanguage() ];
#ifdef CLIENT_DLL
		const char* pCommandName = "script_execute_client";
#else
		const char* pCommandName = "script_execute";
#endif
		const char* pMatch = partial + V_strlen( pCommandName ) + 1;

		ScriptFileAutocompleteRecursive( SCRIPT_PATH, pExtension, pMatch, nMatches, pCommandName, commands );
	}
	return nMatches;
}

#ifdef CLIENT_DLL
CON_COMMAND_F_COMPLETION( script_execute_client, "Run a vscript file", FCVAR_NONE, script_execute_autocomplete )
#else
CON_COMMAND_F_COMPLETION( script_execute, "Run a vscript file", FCVAR_NONE, script_execute_autocomplete )
#endif
{
#ifdef CLIENT_DLL
	if ( !engine->IsClientLocalToActiveServer() )
		return;
#else
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !*args[1] )
	{
		Log_Warning( LOG_VScript, "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	VScriptRunScript( args[1], true );
}

#ifdef CLIENT_DLL
CON_COMMAND( script_debug_client, "Connect the vscript VM to the script debugger" )
#else
CON_COMMAND( script_debug, "Connect the vscript VM to the script debugger" )
#endif
{
#ifdef CLIENT_DLL
	if ( !engine->IsClientLocalToActiveServer() )
		return;
#else
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

#ifdef CLIENT_DLL
CON_COMMAND( script_help_client, "Output help for script functions, optionally with a search string" )
#else
CON_COMMAND( script_help, "Output help for script functions, optionally with a search string" )
#endif
{
#ifdef CLIENT_DLL
	if ( !engine->IsClientLocalToActiveServer() )
		return;
#else
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	const char *pszArg1 = "*";
	if ( *args[1] )
	{
		pszArg1 = args[1];
	}

	g_pScriptVM->Run( CFmtStr( "PrintHelp( \"%s\" );", pszArg1 ) );
}

#ifdef CLIENT_DLL
CON_COMMAND( script_dump_all_client, "Dump the state of the VM to the console" )
#else
CON_COMMAND( script_dump_all, "Dump the state of the VM to the console" )
#endif
{
#ifdef CLIENT_DLL
	if ( !engine->IsClientLocalToActiveServer() )
		return;
#else
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;
#endif

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->DumpState();
}