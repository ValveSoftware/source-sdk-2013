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

IScriptVM * g_pScriptVM;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

#ifdef MAPBASE_VSCRIPT
// This is to ensure a dependency exists between the vscript library and the game DLLs
extern int vscript_token;
int vscript_token_hack = vscript_token;
#endif



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
		CGWarning( 0, CON_GROUP_VSCRIPT, "Script file type does not match VM type\n" );
		return NULL;
	}

	CFmtStr scriptPath;
	if ( pszIncomingExtension )
	{
		scriptPath.sprintf( "scripts/vscripts/%s", pszScriptName );
	}
	else
	{	
		scriptPath.sprintf( "scripts/vscripts/%s%s", pszScriptName,  pszVMExtension );
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

#ifdef MAPBASE_VSCRIPT
		if ( !bResult && bWarnMissing )
#else
		if( !bResult )
#endif
		{
			CGWarning( 0, CON_GROUP_VSCRIPT, "Script not found (%s) \n", scriptPath.operator const char *() );
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
		CGWarning( 0, CON_GROUP_VSCRIPT, "FAILED to compile and execute script file named %s\n", scriptPath.operator const char *() );
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
		CGWarning( 0, CON_GROUP_VSCRIPT, "Cannot run script: NULL script name\n" );
		return false;
	}

	// Prevent infinite recursion in VM
	if ( g_ScriptServerRunScriptDepth > 16 )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "IncludeScript stack overflow\n" );
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
			Warning( "Error running script named %s\n", pszScriptName );
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
	if ( !*args[1] )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "No function name specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
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


CON_COMMAND_SHARED( script_execute, "Run a vscript file" )
{
	if ( !*args[1] )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "No script specified\n" );
		return;
	}

	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}

	VScriptRunScript( args[1], true );
}

CON_COMMAND_SHARED( script_debug, "Connect the vscript VM to the script debugger" )
{
	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND_SHARED( script_help, "Output help for script functions, optionally with a search string" )
{
	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}
	const char *pszArg1 = "*";
	if ( *args[1] )
	{
		pszArg1 = args[1];
	}

	g_pScriptVM->Run( CFmtStr( "PrintHelp( \"%s\" );", pszArg1 ) );
}

CON_COMMAND_SHARED( script_dump_all, "Dump the state of the VM to the console" )
{
	if ( !g_pScriptVM )
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->DumpState();
}

#ifdef MAPBASE_VSCRIPT
//-----------------------------------------------------------------------------
// Purpose: An entity that acts as a container for game scripts.
//-----------------------------------------------------------------------------

#define MAX_SCRIPT_GROUP_CLIENT 8

class CLogicScriptClient : public CBaseEntity
{
public:
	DECLARE_CLASS( CLogicScriptClient, CBaseEntity );
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();

#ifdef CLIENT_DLL
	void OnDataChanged( DataUpdateType_t type )
	{
		BaseClass::OnDataChanged( type );
	
		if ( !m_ScriptScope.IsInitialized() )
		{
			RunVScripts();
		}
	}
#else
	int		UpdateTransmitState() { return SetTransmitState( FL_EDICT_ALWAYS ); }
#endif

	bool KeyValue( const char *szKeyName, const char *szValue )
	{
		if ( FStrEq( szKeyName, "vscripts" ) )
		{
			Q_strcpy( m_iszClientScripts.GetForModify(), szValue );
		}

		return BaseClass::KeyValue( szKeyName, szValue );
	}

	void RunVScripts()
	{
#ifdef CLIENT_DLL
		if (m_iszClientScripts == NULL_STRING)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s has no client scripts", GetDebugName() );
			return;
		}

		if (g_pScriptVM == NULL)
		{
			return;
		}

		ValidateScriptScope();

		// All functions we want to have call chained instead of overwritten
		// by other scripts in this entities list.
		static const char* sCallChainFunctions[] =
		{
			"OnPostSpawn",
			"Precache"
		};

		ScriptLanguage_t language = g_pScriptVM->GetLanguage();

		// Make a call chainer for each in this entities scope
		for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
		{

			if (language == SL_PYTHON)
			{
				// UNDONE - handle call chaining in python
				;
			}
			else if (language == SL_SQUIRREL)
			{
				//TODO: For perf, this should be precompiled and the %s should be passed as a parameter
				HSCRIPT hCreateChainScript = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain <- CSimpleCallChainer(\"%s\", self.GetScriptScope(), true)", sCallChainFunctions[j], sCallChainFunctions[j] ) );
				g_pScriptVM->Run( hCreateChainScript, (HSCRIPT)m_ScriptScope );
			}
		}

		char szScriptsList[255];
		Q_strcpy( szScriptsList, m_iszClientScripts.Get() );
		CUtlStringList szScripts;

		V_SplitString( szScriptsList, " ", szScripts );

		for (int i = 0; i < szScripts.Count(); i++)
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "%s executing script: %s\n", GetDebugName(), szScripts[i] );

			RunScriptFile( szScripts[i], IsWorld() );

			for (int j = 0; j < ARRAYSIZE( sCallChainFunctions ); ++j)
			{
				if (language == SL_PYTHON)
				{
					// UNDONE - handle call chaining in python
					;
				}
				else if (language == SL_SQUIRREL)
				{
					//TODO: For perf, this should be precompiled and the %s should be passed as a parameter.
					HSCRIPT hRunPostScriptExecute = g_pScriptVM->CompileScript( CFmtStr( "%sCallChain.PostScriptExecute()", sCallChainFunctions[j] ) );
					g_pScriptVM->Run( hRunPostScriptExecute, (HSCRIPT)m_ScriptScope );
				}
			}
		}
#else
		// Avoids issues from having m_iszVScripts set without actually having a script scope
		ValidateScriptScope();

		if (m_bRunOnServer)
		{
			BaseClass::RunVScripts();
		}
#endif
	}

#ifndef CLIENT_DLL
	void InputCallScriptFunctionClient( inputdata_t &inputdata )
	{
		// TODO: Support for specific players?
		CBroadcastRecipientFilter filter;
		filter.MakeReliable();

		const char *pszFunction = inputdata.value.String();
		if (strlen( pszFunction ) > 64)
		{
			Msg("%s CallScriptFunctionClient: \"%s\" is too long at %i characters, must be 64 or less\n", GetDebugName(), pszFunction, strlen(pszFunction));
			return;
		}

		UserMessageBegin( filter, "CallClientScriptFunction" );
			WRITE_STRING( pszFunction ); // function
			WRITE_SHORT( entindex() ); // entity
		MessageEnd();
	}
#endif

	//CNetworkArray( string_t, m_iszGroupMembers, MAX_SCRIPT_GROUP_CLIENT );
	CNetworkString( m_iszClientScripts, 128 );

	bool m_bRunOnServer;
};

LINK_ENTITY_TO_CLASS( logic_script_client, CLogicScriptClient );

BEGIN_DATADESC( CLogicScriptClient )

	//DEFINE_KEYFIELD( m_iszGroupMembers[0], FIELD_STRING, "Group00"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[1], FIELD_STRING, "Group01"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[2], FIELD_STRING, "Group02"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[3], FIELD_STRING, "Group03"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[4], FIELD_STRING, "Group04"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[5], FIELD_STRING, "Group05"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[6], FIELD_STRING, "Group06"),
	//DEFINE_KEYFIELD( m_iszGroupMembers[7], FIELD_STRING, "Group07"),

	DEFINE_KEYFIELD( m_bRunOnServer, FIELD_BOOLEAN, "RunOnServer" ),

#ifndef CLIENT_DLL
	DEFINE_INPUTFUNC( FIELD_STRING, "CallScriptFunctionClient", InputCallScriptFunctionClient ),
#endif

END_DATADESC()

IMPLEMENT_NETWORKCLASS_DT( CLogicScriptClient, DT_LogicScriptClient )

#ifdef CLIENT_DLL
	//RecvPropArray( RecvPropString( RECVINFO( m_iszGroupMembers[0] ) ), m_iszGroupMembers ),
	RecvPropString( RECVINFO( m_iszClientScripts ) ),
#else
	//SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_iszGroupMembers ) ), m_iszGroupMembers ),
	SendPropString( SENDINFO( m_iszClientScripts ) ),
#endif

END_NETWORK_TABLE()
#endif
