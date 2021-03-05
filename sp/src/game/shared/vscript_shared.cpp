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
#ifdef MAPBASE_VSCRIPT
#include "mapbase/vscript_singletons.h"
#endif

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
		// player is not yet spawned, this block is always skipped.
		// It is registered in CBasePlayer instead.
#ifndef MAPBASE
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

	g_pScriptVM->Run( CFmtStr( "__Documentation.PrintHelp( \"%s\" );", pszArg1 ) );
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

//-----------------------------------------------------------------------------

static short VSCRIPT_SERVER_SAVE_RESTORE_VERSION = 2;

//-----------------------------------------------------------------------------

class CVScriptSaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	CVScriptSaveRestoreBlockHandler() :
		m_InstanceMap( DefLessFunc(const char *) )
	{
	}
	const char *GetBlockName()
	{
#ifdef CLIENT_DLL
		return "VScriptClient";
#else
		return "VScriptServer";
#endif
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->StartBlock();

		int temp = g_pScriptVM != NULL;
		pSave->WriteInt( &temp );
		if ( g_pScriptVM )
		{
			temp = g_pScriptVM->GetLanguage();
			pSave->WriteInt( &temp );
			CUtlBuffer buffer;
			g_pScriptVM->WriteState( &buffer );
			temp = buffer.TellPut();
			pSave->WriteInt( &temp );
			if ( temp > 0 )
			{
				pSave->WriteData( (const char *)buffer.Base(), temp );
			}
		}

		pSave->EndBlock();
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == VSCRIPT_SERVER_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( !m_fDoLoad && g_pScriptVM )
		{
			return;
		}
#ifdef CLIENT_DLL
		C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity();
#else
		CBaseEntity *pEnt = gEntList.FirstEnt();
#endif
		while ( pEnt )
		{
			if ( pEnt->m_iszScriptId != NULL_STRING )
			{
#ifndef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterClass( pEnt->GetScriptDesc() );
#endif
				m_InstanceMap.Insert( STRING( pEnt->m_iszScriptId ), pEnt );
			}
#ifdef CLIENT_DLL
			pEnt = ClientEntityList().NextBaseEntity( pEnt );
#else
			pEnt = gEntList.NextEnt( pEnt );
#endif
		}

		pRestore->StartBlock();
		if ( pRestore->ReadInt() && pRestore->ReadInt() == g_pScriptVM->GetLanguage() )
		{
			int nBytes = pRestore->ReadInt();
			if ( nBytes > 0 )
			{
				CUtlBuffer buffer;
				buffer.EnsureCapacity( nBytes );
				pRestore->ReadData( (char *)buffer.AccessForDirectRead( nBytes ), nBytes, 0 );
				g_pScriptVM->ReadState( &buffer );
			}
		}
		pRestore->EndBlock();
	}

	void PostRestore( void )
	{
		for ( int i = m_InstanceMap.FirstInorder(); i != m_InstanceMap.InvalidIndex(); i = m_InstanceMap.NextInorder( i ) )
		{
			CBaseEntity *pEnt = m_InstanceMap[i];
			if ( pEnt->m_hScriptInstance )
			{
				ScriptVariant_t variant;
				if ( g_pScriptVM->GetValue( STRING(pEnt->m_iszScriptId), &variant ) && variant.m_type == FIELD_HSCRIPT )
				{
					pEnt->m_ScriptScope.Init( variant.m_hScript, false );
#ifndef CLIENT_DLL
					pEnt->RunPrecacheScripts();
#endif
				}
			}
			else
			{
				// Script system probably has no internal references
				pEnt->m_iszScriptId = NULL_STRING;
			}
		}
		m_InstanceMap.Purge();

#if defined(MAPBASE_VSCRIPT) && defined(CLIENT_DLL)
		VScriptSaveRestoreUtil_OnVMRestore();
#endif
	}


	CUtlMap<const char *, CBaseEntity *> m_InstanceMap;

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CVScriptSaveRestoreBlockHandler g_VScriptSaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetVScriptSaveRestoreBlockHandler()
{
	return &g_VScriptSaveRestoreBlockHandler;
}

bool CBaseEntityScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CBaseEntity *pEntity = (CBaseEntity *)p;
#ifdef CLIENT_DLL
	if ( pEntity->GetEntityName() && pEntity->GetEntityName()[0] )
#else
	if ( pEntity->GetEntityName() != NULL_STRING )
#endif
	{
		V_snprintf( pBuf, bufSize, "([%d] %s: %s)", pEntity->entindex(), pEntity->GetClassname(), STRING( pEntity->GetEntityName() ) );
	}
	else
	{
		V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), pEntity->GetClassname() );
	}
	return true; 
}

void *CBaseEntityScriptInstanceHelper::BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
{
	int iEntity = g_VScriptSaveRestoreBlockHandler.m_InstanceMap.Find( pszId );
	if ( iEntity != g_VScriptSaveRestoreBlockHandler.m_InstanceMap.InvalidIndex() )
	{
		CBaseEntity *pEnt = g_VScriptSaveRestoreBlockHandler.m_InstanceMap[iEntity];
		pEnt->m_hScriptInstance = hInstance;
		return pEnt;
	}
	return NULL;
}


CBaseEntityScriptInstanceHelper g_BaseEntityScriptInstanceHelper;
