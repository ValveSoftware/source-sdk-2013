//========== Copyright � 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_server.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "eventqueue.h"
#include "GameEventListener.h"
#include "gameinterface.h"
#include "functorutils.h"
#include "mapentities.h"
#include "characterset.h"
#include "sceneentity.h"		// for exposing scene precache function
#include "isaverestore.h"
#include "gamerules.h"
#include "particle_parse.h"
#include "usermessages.h"
#include "engine/IEngineSound.h"
#include "vscript_utils.h"
#include "netpropmanager.h"
#include "client.h"
#include "tier0/vcrmode.h"
#include "in_buttons.h"
#include "coordsize.h"
#include "team.h"

#ifdef TF_DLL
#include "tf/tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/NextBotLocomotionInterface.h"
#include "bot/tf_bot.h"
#endif

#if defined( _WIN32 ) || defined( POSIX )
#include "vscript_server_nut.h"
#endif

#if defined( PORTAL2_PUZZLEMAKER )
#include "matchmaking/imatchframework.h"
#include "portal2_research_data_tracker.h"
#endif // PORTAL2_PUZZLEMAKER

#ifdef DOTA_DLL
#include "dota_animation.h"
#endif

extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

extern CServerGameDLL g_ServerGameDLL;

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START float debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

ConVar script_connect_debugger_on_mapspawn( "script_connect_debugger_on_mapspawn", "0" );

ConVar script_attach_debugger_at_startup( "script_attach_debugger_at_startup", "0" );
ConVar script_break_in_native_debugger_on_error( "script_break_in_native_debugger_on_error", "0" );

#define VSCRIPT_CONVAR_ALLOWLIST_NAME "cfg/vscript_convar_allowlist.txt"

/// Exposes convars to script
class CScriptConvarAccessor : public CAutoGameSystem
{
public:
	ScriptVariant_t GetBool( const char *cvar );
	ScriptVariant_t GetInt( const char *cvar );
	ScriptVariant_t GetFloat( const char *cvar );
	ScriptVariant_t GetStr( const char *cvar );
	const char *GetClientConvarValue( const char *cvar, int entindex );
	void SetValue( const char *cvar, ScriptVariant_t value );

	void LevelInitPreEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE;

	bool IsConVarOnAllowList( const char *cvar );

	CUtlSymbolTable m_AllowedConVars;
};
CScriptConvarAccessor g_ScriptConvars;

#define FCVAR_SCRIPT_NONO ( FCVAR_PROTECTED | FCVAR_SERVER_CANNOT_QUERY )

ScriptVariant_t CScriptConvarAccessor::GetBool( const char *cvar )
{
	if ( !cvar || !*cvar )
		return ScriptVariant_t();

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetBool();
	}
	else
	{
		return ScriptVariant_t(); // default ctor is NULL
	}
}

ScriptVariant_t CScriptConvarAccessor::GetInt( const char *cvar )
{
	if ( !cvar || !*cvar )
		return ScriptVariant_t();

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetInt();
	}
	else
	{
		return ScriptVariant_t(); // default ctor is NULL
	}
}

ScriptVariant_t CScriptConvarAccessor::GetFloat( const char *cvar )
{
	if ( !cvar || !*cvar )
		return ScriptVariant_t();

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		return cref.GetFloat();
	}
	else
	{
		return ScriptVariant_t(); // default ctor is NULL
	}
}

ScriptVariant_t CScriptConvarAccessor::GetStr( const char *cvar )
{
	if ( !cvar || !*cvar )
		return ScriptVariant_t();

	ConVarRef cref( cvar );
	if ( cref.IsValid() )
	{
		if ( cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
		{
			// the funny.
			return "hunter2";
		}
		return cref.GetString();
	}
	else
	{
		return ScriptVariant_t(); // default ctor is NULL
	}
}

const char *CScriptConvarAccessor::GetClientConvarValue( const char *cvar, int entindex )
{
	if ( !cvar || !*cvar )
		return "";

	return engine->GetClientConVarValue( entindex, cvar );
}

void CScriptConvarAccessor::SetValue( const char *cvar, ScriptVariant_t value )
{
	if ( !cvar || !*cvar )
		return;

	if ( !IsConVarOnAllowList( cvar ) )
	{
		DevMsg( "Convar %s was not in " VSCRIPT_CONVAR_ALLOWLIST_NAME "\n", cvar );
		return;
	}

	ConVarRef cref( cvar );
	if ( cref.IsValid() && !cref.IsFlagSet( FCVAR_SCRIPT_NONO ) )
	{
		bool bSave = true;
		switch( value.GetType() )
		{
		case FIELD_BOOLEAN:
			cref.SetValue( (bool)value );
			break;
		case FIELD_INTEGER:
			cref.SetValue( (int)value );
			break;
		case FIELD_FLOAT:
			cref.SetValue( (float)value );
			break;
		case FIELD_CSTRING:
			cref.SetValue( (const char *)value );
			break;
		default:
			Warning( "%s.SetValue() unsupported value type %s\n", cvar, ScriptFieldTypeName( value.GetType() ) );
			bSave = false;
			break;
		}

		if ( bSave )
		{
			GameRules()->SaveConvar( cref );
		}
	}
}

void CScriptConvarAccessor::LevelInitPreEntity()
{
	m_AllowedConVars.RemoveAll();

	KeyValues *kv = new KeyValues( "vscript_convar_allowlist" );
	bool bLoaded = kv->LoadFromFile( g_pFullFileSystem, VSCRIPT_CONVAR_ALLOWLIST_NAME, "MOD" );
	if ( bLoaded )
	{
		for ( KeyValues *pCurItem = kv->GetFirstValue(); pCurItem; pCurItem = pCurItem->GetNextValue() )
		{
			const char *pName = pCurItem->GetName();
			const char *pValue = pCurItem->GetString();

			if ( !V_stricmp( pValue, "allowed" ) )
				m_AllowedConVars.AddString( pName );
		}
	}
	
	if ( !bLoaded )
		Warning( "Error loading " VSCRIPT_CONVAR_ALLOWLIST_NAME "\n" );
	kv->deleteThis();
}

void CScriptConvarAccessor::LevelShutdownPostEntity()
{
	m_AllowedConVars.RemoveAll();
}

bool CScriptConvarAccessor::IsConVarOnAllowList( const char *cvar )
{
	if ( !cvar || !*cvar )
		return false;

	return m_AllowedConVars.Find( cvar ) != UTL_INVAL_SYMBOL;
}

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptConvarAccessor, "Convars", SCRIPT_SINGLETON "Access to convar functions" )
DEFINE_SCRIPTFUNC( GetBool, "GetBool(name) : returns the convar as a bool. May return null if no such convar." )
DEFINE_SCRIPTFUNC( GetInt, "GetInt(name) : returns the convar as an int. May return null if no such convar." )
DEFINE_SCRIPTFUNC( GetFloat, "GetFloat(name) : returns the convar as a float. May return null if no such convar." )
DEFINE_SCRIPTFUNC( GetStr, "GetStr(name) : returns the convar as a string. May return null if no such convar." )
DEFINE_SCRIPTFUNC( GetClientConvarValue, "GetClientConvarValue(name) : returns the convar value for the entindex as a string." )
DEFINE_SCRIPTFUNC( SetValue, "SetValue(name, value) : sets the value of the convar. The convar must be in " VSCRIPT_CONVAR_ALLOWLIST_NAME " to be set. Supported types are bool, int, float, string." )
DEFINE_SCRIPTFUNC( IsConVarOnAllowList, "IsConVarOnAllowList(name) : checks if the convar is allowed to be used and is in " VSCRIPT_CONVAR_ALLOWLIST_NAME ". Please be nice with this and use it for *compatibility* if you need check support and NOT to force server owners to allow hostname to be set... or else this will simply lie and return true in future. ;-) You have been warned!"  )
END_SCRIPTDESC()


//-----------------------------------------------------------------------------
class CScriptEntityOutputs
{
public:
	int GetNumElements( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity )
			return -1;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return -1;

		return pOutput->NumberOfElements();
	}

	void GetOutputTable( HSCRIPT hEntity, const char *szOutputName, HSCRIPT hOutputTable, int element )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity || !hOutputTable || element < 0 )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( pOutput )
		{
			int iCount = 0;
			CEventAction *pAction = pOutput->GetFirstAction();
			while ( pAction )
			{
				if ( iCount == element )
				{
					g_pScriptVM->SetValue( hOutputTable, "target", STRING( pAction->m_iTarget ) );
					g_pScriptVM->SetValue( hOutputTable, "input", STRING( pAction->m_iTargetInput ) );
					g_pScriptVM->SetValue( hOutputTable, "parameter", STRING( pAction->m_iParameter ) );
					g_pScriptVM->SetValue( hOutputTable, "delay", pAction->m_flDelay );
					g_pScriptVM->SetValue( hOutputTable, "times_to_fire", pAction->m_nTimesToFire );
					break;
				}
				else
				{
					iCount++;
					pAction = pAction->m_pNext;
				}
			}
		}
	}

	bool HasOutput( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity )
			return false;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return false;

		return true;
	}

	bool HasAction( HSCRIPT hEntity, const char *szOutputName )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity )
			return false;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( pOutput )
		{
			CEventAction *pAction = pOutput->GetFirstAction();
			if ( pAction )
				return true;
		}

		return false;
	}

	void AddOutput( HSCRIPT hEntity, const char *szOutputName, const char *szTarget, const char *szTargetInput, const char *szParameter, float flDelay, int iTimesToFire )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return;

		CEventAction *pAction = new CEventAction( NULL );
		pAction->m_iTarget = AllocPooledString( szTarget );
		pAction->m_iTargetInput = AllocPooledString( szTargetInput );
		pAction->m_iParameter = AllocPooledString( szParameter );
		pAction->m_flDelay = flDelay;
		pAction->m_nTimesToFire = iTimesToFire;
		pOutput->AddEventAction( pAction );
	}

	void RemoveOutput( HSCRIPT hEntity, const char *szOutputName, const char *szTarget, const char *szTargetInput, const char *szParameter )
	{
		CBaseEntity *pBaseEntity = ToEnt( hEntity );
		if ( !pBaseEntity )
			return;

		CBaseEntityOutput *pOutput = pBaseEntity->FindNamedOutput( szOutputName );
		if ( !pOutput )
			return;

		if ( V_strcmp( szTarget, "" ) == 0 )
			pOutput->DeleteAllElements();
		else
		{
			CEventAction *pAction = pOutput->GetFirstAction();
			pOutput->ScriptRemoveEventAction( pAction, szTarget, szTargetInput, szParameter );
		}
	}
} g_ScriptEntityOutputs;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptEntityOutputs, "CScriptEntityOutputs", SCRIPT_SINGLETON "Used to access entity output data" )
DEFINE_SCRIPTFUNC( GetNumElements, "Arguments: ( entity, outputName ) - returns the number of array elements" )
DEFINE_SCRIPTFUNC( GetOutputTable, "Arguments: ( entity, outputName, table, arrayElement ) - returns a table of output information" )
DEFINE_SCRIPTFUNC( HasOutput, "Arguments: ( entity, outputName ) - returns true if the output exists" )
DEFINE_SCRIPTFUNC( HasAction, "Arguments: ( entity, outputName ) - returns true if an action exists for the output" )
DEFINE_SCRIPTFUNC( AddOutput, "Arguments: ( entity, outputName, targetName, inputName, parameter, delay, timesToFire ) - add a new output to the entity" )
DEFINE_SCRIPTFUNC( RemoveOutput, "Arguments: ( entity, outputName, targetName, inputName, parameter ) - remove an output from the entity" )
END_SCRIPTDESC();


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScrollingScreenOverlay
{
public:
	CScrollingScreenOverlay( float x = 0.01, float y = 0.0, float duration = FLT_MAX, int iFirstLine = 1, int nLines = 50, int r = 255, int g = 255, int b = 255 );

	void SetXY( float x, float y );
	void SetTextDuration( float duration );
	void SetFirstLine( int iFirstLine );
	void SetNumLines( int nLines );

	void AddText( const char *pszText, int r, int g, int b );
	void AddText( const char *pszText );

	void Clear();

	void Draw();

private:
	struct TextLine_t
	{
		CUtlString m_text;
		float m_time;
		int m_r, m_g, m_b;
	};

	CUtlLinkedList< TextLine_t > m_Text;
	float m_x, m_y;
	float m_duration;
	int m_iFirstLine;
	int m_nLines;
	int m_r, m_g, m_b;
};

CScrollingScreenOverlay::CScrollingScreenOverlay( float x, float y, float duration, int iFirstLine, int nLines, int r, int g, int b ) :
	m_x( x ),
	m_y( y ),
	m_duration( duration ),
	m_iFirstLine( iFirstLine ),
	m_nLines( nLines ), 
	m_r( r ), 
	m_g( g ),
	m_b( b )
{
}

void CScrollingScreenOverlay::SetXY( float x, float y )
{
	m_x = x;
	m_y = y;
}

void CScrollingScreenOverlay::SetTextDuration( float duration )
{
	m_duration = duration;
}

void CScrollingScreenOverlay::SetFirstLine( int iFirstLine )
{
	m_iFirstLine = iFirstLine;
}

void CScrollingScreenOverlay::SetNumLines( int nLines )
{
	m_nLines = nLines;
}

void CScrollingScreenOverlay::AddText( const char *pszText, int r, int g, int b )
{
	while ( m_Text.Count() && m_Text.Count() >= m_nLines )
	{
		m_Text.Remove( m_Text.Head() );
	}
	int iNew = m_Text.AddToTail();
	m_Text[iNew].m_text = pszText;
	m_Text[iNew].m_time = gpGlobals->curtime;
	m_Text[iNew].m_r = r;
	m_Text[iNew].m_g = g;
	m_Text[iNew].m_b = b;
}

void CScrollingScreenOverlay::AddText( const char *pszText )
{
	AddText( pszText, m_r, m_g, m_b );
}

void CScrollingScreenOverlay::Clear()
{
	m_Text.RemoveAll();
}

void CScrollingScreenOverlay::Draw()
{
	if ( developer.GetBool() )
	{
		int line = m_iFirstLine;
		int i;
		int alpha;
		float age;

		while ( ( i = m_Text.Head() ) != m_Text.InvalidIndex() )
		{
			age = gpGlobals->curtime - m_Text[i].m_time;

			if ( age >= m_duration )
			{
				m_Text.Remove( m_Text.Head() );
			}
			else
			{
				break;
			}
		}

		CFmtStrN<1024> msg;
		float msgTime;
		for ( int i = m_Text.Head(); i != m_Text.InvalidIndex(); i = m_Text.Next( i ) )
		{
			msgTime = m_Text[i].m_time;
			age = gpGlobals->curtime - msgTime;
			if ( age <= m_duration - 1.0f )
			{
				alpha = 255;
			}
			else
			{
				alpha = 255 * ( m_duration - age );
			}
			
			msg.sprintf( "(%0.2f): %s", msgTime, m_Text[i].m_text.operator const char *() );
			NDebugOverlay::ScreenTextLine( m_x, m_y, line++, msg, m_Text[i].m_r, m_Text[i].m_g, m_Text[i].m_b, alpha, NDEBUG_PERSIST_TILL_NEXT_SERVER );
		}
	}
}


CScrollingScreenOverlay g_ScriptErrorScreenOverlay( 0.01, 0.0, 20.0f, 14, 30, 255, 0, 0 );

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool VScriptServerScriptErrorFunc( ScriptErrorLevel_t /*eLevel*/, const char *pszText )
{
	if ( script_break_in_native_debugger_on_error.GetBool() )
	{
		DebuggerBreakIfDebugging();
		script_break_in_native_debugger_on_error.SetValue( "0" );
	}
	if ( developer.GetBool() )
	{
		char szTemp[1024];
		V_strncpy( szTemp, pszText, ARRAYSIZE(szTemp) );

		char *pszCurrent = szTemp;
		char *pszNewline = pszCurrent;

		while ( *pszCurrent )
		{
			while ( *pszNewline )
			{
				if ( *pszNewline == '\n' )
				{
					*pszNewline++ = 0;
					break;
				}
				pszNewline++;
			}
			g_ScriptErrorScreenOverlay.AddText( pszCurrent );
			pszCurrent = pszNewline;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
class CScriptEntityIterator : public IEntityFindFilter
{
public:
	HSCRIPT First() { return Next(NULL); }

	HSCRIPT Next( HSCRIPT hStartEntity )
	{
		return ToHScript( gEntList.NextEnt( ToEnt( hStartEntity ) ) );
	}

	HSCRIPT CreateByClassname( const char *className )
	{
		return ToHScript( CreateEntityByName( className ) );
	}

	HSCRIPT FindByClassname( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByClassname( ToEnt( hStartEntity ), szName, this ) );
	}

	HSCRIPT FindByName( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByName( ToEnt( hStartEntity ), szName, NULL, NULL, NULL, this ) );
	}

	HSCRIPT FindInSphere( HSCRIPT hStartEntity, const Vector &vecCenter, float flRadius )
	{
		return ToHScript( gEntList.FindEntityInSphere( ToEnt( hStartEntity ), vecCenter, flRadius, this ) );
	}

	HSCRIPT FindByTarget( HSCRIPT hStartEntity, const char *szName )
	{
		return ToHScript( gEntList.FindEntityByTarget( ToEnt( hStartEntity ), szName, this ) );
	}

	HSCRIPT FindByModel( HSCRIPT hStartEntity, const char *szModelName )
	{
		return ToHScript( gEntList.FindEntityByModel( ToEnt( hStartEntity ), szModelName, this ) );
	}

	HSCRIPT FindByNameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius, NULL, NULL, NULL, this ) );
	}

	HSCRIPT FindByNameWithin( HSCRIPT hStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByNameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius, NULL, NULL, NULL, this ) );
	}

	HSCRIPT FindByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius, this ) );
	}

	HSCRIPT FindByClassnameWithin( HSCRIPT hStartEntity , const char *szName, const Vector &vecSrc, float flRadius )
	{
		return ToHScript( gEntList.FindEntityByClassnameWithin( ToEnt( hStartEntity ), szName, vecSrc, flRadius, this ) );
	}

	void DispatchSpawn( HSCRIPT hEntity )
	{
		::DispatchSpawn( ToEnt( hEntity ), false );
	}

	bool ShouldFindEntity( CBaseEntity *pEntity )
	{
		if ( !pEntity )
			return true;

		if ( pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = assert_cast< CBasePlayer * >( pEntity );
			if ( pPlayer->IsHLTV() )
				return false;
		}

		return true;
	}

	CBaseEntity *GetFilterResult( void )
	{
		return NULL;
	}

private:
} g_ScriptEntityIterator;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptEntityIterator, "CEntities", SCRIPT_SINGLETON "The global list of entities" )
	DEFINE_SCRIPTFUNC( First, "Begin an iteration over the list of entities" )
	DEFINE_SCRIPTFUNC( Next, "Continue an iteration over the list of entities, providing reference to a previously found entity" )
	DEFINE_SCRIPTFUNC( CreateByClassname, "Creates an entity by classname" )
	DEFINE_SCRIPTFUNC( FindByClassname, "Find entities by class name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByName, "Find entities by name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindInSphere, "Find entities within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByTarget, "Find entities by targetname. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByModel, "Find entities by model name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByNameNearest, "Find entities by name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByNameWithin, "Find entities by name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByClassnameNearest, "Find entities by class name nearest to a point."  )
	DEFINE_SCRIPTFUNC( FindByClassnameWithin, "Find entities by class name within a radius. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( DispatchSpawn, "Dispatches spawn of an entity!" )
END_SCRIPTDESC();

CVScriptGameEventListener g_VScriptGameEventListener;

void CVScriptGameEventListener::Init()
{
	m_RunGameEventCallbacksFunc = INVALID_HSCRIPT;
	m_CollectGameEventCallbacksFunc = INVALID_HSCRIPT;
	m_ScriptHookCallbacksFunc = INVALID_HSCRIPT;
}
void CVScriptGameEventListener::FireGameEvent( IGameEvent *event )
{
	// Pass all keyvales as a table of parameters
	HSCRIPT paramsTable = ScriptTableFromKeyValues( g_pScriptVM, event->GetDataKeys() );
	RunGameEventCallbacks( event->GetName(), paramsTable );
}

void CVScriptGameEventListener::ListenForScriptHook( const char* szName )
{
	m_ScriptHooks.AddString( szName );
}

void CVScriptGameEventListener::ClearAllScriptHooks()
{
	m_ScriptHooks.RemoveAll();
}

bool CVScriptGameEventListener::HasScriptHook( const char *szName )
{
	if ( !szName || !*szName )
		return false;

	return m_ScriptHooks.Find( szName ).IsValid();
}

bool CVScriptGameEventListener::FireScriptHook( const char *pszHookName, HSCRIPT params )
{
	if ( !HasScriptHook( pszHookName ) )
		return false;

	RunScriptHookCallbacks( pszHookName, params );
	return true;
}

// Calls a squirrel func (see vscript_server.nut) to call each
// registered script function associated with this game event.
void CVScriptGameEventListener::RunGameEventCallbacks( const char* szName, HSCRIPT params )
{
	Assert( szName );
	if ( !szName )
		return;

	if ( m_RunGameEventCallbacksFunc == INVALID_HSCRIPT )
		m_RunGameEventCallbacksFunc = g_pScriptVM->LookupFunction( "__RunGameEventCallbacks" );

	if ( m_RunGameEventCallbacksFunc )
	{
		g_pScriptVM->Call( m_RunGameEventCallbacksFunc, NULL, true, NULL, szName, params );
	}
}

void CVScriptGameEventListener::RunScriptHookCallbacks( const char* szName, HSCRIPT params )
{
	Assert( szName );
	if ( !szName )
		return;

	if ( m_ScriptHookCallbacksFunc == INVALID_HSCRIPT )
		m_ScriptHookCallbacksFunc = g_pScriptVM->LookupFunction( "__RunScriptHookCallbacks" );

	if ( m_ScriptHookCallbacksFunc )
	{
		g_pScriptVM->Call( m_ScriptHookCallbacksFunc, NULL, true, NULL, szName, params );
	}
}

void CVScriptGameEventListener::CollectGameEventCallbacksInScope( HSCRIPT scope )
{
	if ( m_CollectGameEventCallbacksFunc == INVALID_HSCRIPT )
		m_CollectGameEventCallbacksFunc = g_pScriptVM->LookupFunction( "__CollectGameEventCallbacks" );

	if ( m_CollectGameEventCallbacksFunc )
	{
		g_pScriptVM->Call( m_CollectGameEventCallbacksFunc, NULL, true, NULL, scope );
	}
}

void RegisterScriptGameEventListener( const char* pszEventName )
{
	if ( !pszEventName || !*pszEventName )
	{
		Log_Warning( LOG_VScript, "No event name specified\n" );
		return;
	}

	g_VScriptGameEventListener.ListenForGameEvent( pszEventName );
}

void RegisterScriptHookListener( const char* pszEventName )
{
	if ( !pszEventName || !*pszEventName )
	{
		Log_Warning( LOG_VScript, "No event name specified\n" );
		return;
	}

	g_VScriptGameEventListener.ListenForScriptHook( pszEventName );
}

void CollectGameEventCallbacksInScope( HSCRIPT scope )
{
	g_VScriptGameEventListener.CollectGameEventCallbacksInScope( scope );
}

void ClearScriptGameEventListeners( void )
{
	g_VScriptGameEventListener.StopListeningForAllEvents();
	g_VScriptGameEventListener.ClearAllScriptHooks();
}

ConVar vscript_script_hooks( "vscript_script_hooks", "1" );

bool ScriptHooksEnabled()
{
	return g_pScriptVM && vscript_script_hooks.GetBool();
}

bool ScriptHookEnabled( const char *pszName )
{
	if ( !ScriptHooksEnabled() )
		return false;

	if ( !pszName || !*pszName )
	{
		Log_Warning( LOG_VScript, "No event name specified\n" );
		return false;
	}

	return g_VScriptGameEventListener.HasScriptHook( pszName );
}

bool RunScriptHook( const char *pszHookName, HSCRIPT params )
{
	if ( !pszHookName || !*pszHookName )
	{
		Log_Warning( LOG_VScript, "No event name specified\n" );
		return false;
	}

	return g_VScriptGameEventListener.FireScriptHook( pszHookName, params );
}

CNetPropManager g_ScriptNetPropManager;

BEGIN_SCRIPTDESC_ROOT_NAMED( CNetPropManager, "CNetPropManager", SCRIPT_SINGLETON "Used to get/set entity network fields" )
	DEFINE_SCRIPTFUNC( GetPropInt, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropFloat, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropVector, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropEntity, "Arguments: ( entity, propertyName ) - returns an entity" )
	DEFINE_SCRIPTFUNC( GetPropString, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( SetPropInt, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropFloat, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropVector, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropEntity, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropString, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( GetPropIntArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropFloatArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropVectorArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropEntityArray, "Arguments: ( entity, propertyName, arrayElement ) - returns an entity" )
	DEFINE_SCRIPTFUNC( GetPropStringArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropIntArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropFloatArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropVectorArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropEntityArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropStringArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropArraySize, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( HasProp, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropType, "Arguments: ( entity, propertyName ) - return the prop type as a string" )
	DEFINE_SCRIPTFUNC( GetPropBool, "Arguments: ( entity, propertyName )" )
	DEFINE_SCRIPTFUNC( GetPropBoolArray, "Arguments: ( entity, propertyName, arrayElement )" )
	DEFINE_SCRIPTFUNC( SetPropBool, "Arguments: ( entity, propertyName, value )" )
	DEFINE_SCRIPTFUNC( SetPropBoolArray, "Arguments: ( entity, propertyName, value, arrayElement )" )
	DEFINE_SCRIPTFUNC( GetPropInfo, "Arguments: ( entity, propertyName, arrayElement, table ) - Fills in a passed table with property info for the provided entity" )
	DEFINE_SCRIPTFUNC( GetTable, "Arguments: ( entity, iPropType, table ) - Fills in a passed table with all props of a specified type for the provided entity (set iPropType to 0 for SendTable or 1 for DataMap)" )
END_SCRIPTDESC()

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#if 0 // From Desolation.
class CScriptPanorama
{
public:

	void DispatchEvent( const char *pszEventName, const char *pszMessage )
	{
		CBroadcastRecipientFilter filter;
		filter.MakeReliable();

		CCSUsrMsg_PanoramaDispatchEvent msg;
		msg.set_event( pszEventName );
		msg.set_message( pszMessage );
		SendUserMessage( filter, CS_UM_PanoramaDispatchEvent, msg );
	}

private:
} g_ScriptPanorama;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptPanorama, "CPanorama", SCRIPT_SINGLETON "Panorama VScript Interface" )
	DEFINE_SCRIPTFUNC( DispatchEvent, "Trigger a panorama event to the vscript event handler" )
END_SCRIPTDESC();
#endif

// ----------------------------------------------------------------------------
// KeyValues access - CBaseEntity::ScriptGetKeyFromModel returns root KeyValues
// ----------------------------------------------------------------------------

BEGIN_SCRIPTDESC_ROOT( CScriptKeyValues, "Wrapper class over KeyValues instance" )
	DEFINE_SCRIPT_CONSTRUCTOR()	
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindKey, "FindKey", "Given a KeyValues object and a key name, find a KeyValues object associated with the key name" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstSubKey, "GetFirstSubKey", "Given a KeyValues object, return the first sub key object" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNextKey, "GetNextKey", "Given a KeyValues object, return the next key object in a sub key group" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueInt, "GetKeyInt", "Given a KeyValues object and a key name, return associated integer value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueFloat, "GetKeyFloat", "Given a KeyValues object and a key name, return associated float value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueBool, "GetKeyBool", "Given a KeyValues object and a key name, return associated bool value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetKeyValueString, "GetKeyString", "Given a KeyValues object and a key name, return associated string value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsKeyValueEmpty, "IsKeyEmpty", "Given a KeyValues object and a key name, return true if key name has no value" );
	DEFINE_SCRIPTFUNC_NAMED( ScriptReleaseKeyValues, "ReleaseKeyValues", "Given a root KeyValues object, release its contents" );
END_SCRIPTDESC();

HSCRIPT CScriptKeyValues::ScriptFindKey( const char *pszName )
{
	KeyValues *pKeyValues = m_pKeyValues->FindKey(pszName);
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetFirstSubKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetFirstSubKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

HSCRIPT CScriptKeyValues::ScriptGetNextKey( void )
{
	KeyValues *pKeyValues = m_pKeyValues->GetNextKey();
	if ( pKeyValues == NULL )
		return NULL;

	CScriptKeyValues *pScriptKey = new CScriptKeyValues( pKeyValues );

	// UNDONE: who calls ReleaseInstance on this??
	HSCRIPT hScriptInstance = g_pScriptVM->RegisterInstance( pScriptKey );
	return hScriptInstance;
}

int CScriptKeyValues::ScriptGetKeyValueInt( const char *pszName )
{
	int i = m_pKeyValues->GetInt( pszName );
	return i;
}

float CScriptKeyValues::ScriptGetKeyValueFloat( const char *pszName )
{
	float f = m_pKeyValues->GetFloat( pszName );
	return f;
}

const char *CScriptKeyValues::ScriptGetKeyValueString( const char *pszName )
{
	const char *psz = m_pKeyValues->GetString( pszName );
	return psz;
}

bool CScriptKeyValues::ScriptIsKeyValueEmpty( const char *pszName )
{
	bool b = m_pKeyValues->IsEmpty( pszName );
	return b;
}

bool CScriptKeyValues::ScriptGetKeyValueBool( const char *pszName )
{
	bool b = m_pKeyValues->GetBool( pszName );
	return b;
}

void CScriptKeyValues::ScriptReleaseKeyValues( )
{
	m_pKeyValues->deleteThis();
	m_pKeyValues = NULL;
}


// constructors
CScriptKeyValues::CScriptKeyValues( KeyValues *pKeyValues )
{
	m_pKeyValues = pKeyValues;
}

// destructor
CScriptKeyValues::~CScriptKeyValues( )
{
	if (m_pKeyValues)
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = NULL;
}




//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float Time()
{
	return gpGlobals->curtime;
}

static float MaxClients()
{
	return gpGlobals->maxClients;
}

static float FrameTime()
{
	return gpGlobals->frametime;
}

static void SendToConsole( const char *pszCommand )
{
	if ( !pszCommand )
		return;

	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
	{
		DevMsg ("Cannot execute \"%s\", no player\n", pszCommand );
		return;
	}

	engine->ClientCommand( pPlayer->edict(), "%s", pszCommand );
}

static void SendToServerConsole( const char *pszCommand )
{
	if ( !pszCommand )
		return;

	bool bAllowed = ( sAllowPointServerCommand == eAllowAlways );
#ifdef TF_DLL
	if ( sAllowPointServerCommand == eAllowOfficial )
	{
		bAllowed = TFGameRules() && TFGameRules()->IsValveMap();
	}
#endif // TF_DLL

	if ( !bAllowed )
	{
		return;
	}

	engine->ServerCommand( UTIL_VarArgs( "%s\n", pszCommand ) );
}

static const char *GetMapName()
{
	return STRING( gpGlobals->mapname );
}

static const char *DoUniqueString( const char *pszBase )
{
	static char szBuf[512];
	g_pScriptVM->GenerateUniqueKey( pszBase, szBuf, ARRAYSIZE(szBuf) );
	return szBuf;
}

static void DoEntFire( const char *pszTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
{
	const char *target = "", *action = "Use";
	variant_t value;

	if ( pszTarget && *pszTarget )
		target = STRING( AllocPooledString( pszTarget ) );

	// Don't allow them to run anything on a point_servercommand unless they're the host player. Otherwise they can ent_fire
	// and run any command on the server. Admittedly, they can only do the ent_fire if sv_cheats is on, but 
	// people complained about users resetting the rcon password if the server briefly turned on cheats like this:
	//    give point_servercommand
	//    ent_fire point_servercommand command "rcon_password mynewpassword"
	if ( gpGlobals->maxClients > 1 && V_stricmp( target, "point_servercommand" ) == 0 )
	{
		return;
	}

	if ( *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	g_EventQueue.AddEvent( target, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}

// Some game events pass entity's by their entindex. This lets scripts translate that
// into a handle to the entity's script instance.
HSCRIPT EntIndexToHScript( int entityIndex )
{
	return ToHScript( UTIL_EntityByIndex( entityIndex ) );
}

HSCRIPT PlayerInstanceFromIndex( int idx )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( idx );

	if ( !pPlayer )
		return NULL;

	if ( pPlayer->IsHLTV() )
		return NULL;

	return ToHScript( pPlayer );
}

// Fires a game event from a script file to any listening script hooks.
// NOTE: this only goes from script, to script. No C code game event listeners
// will be notified.
bool ScriptFireGameEvent( const char* szName, HSCRIPT params )
{
	if ( !szName || !*szName )
		return false;

	g_VScriptGameEventListener.RunGameEventCallbacks( szName, params );
	return true;
}

bool ScriptFireScriptHook( const char* szName, HSCRIPT params )
{
	if ( !szName || !*szName )
		return false;

	return g_VScriptGameEventListener.FireScriptHook( szName, params );
}

bool ScriptSendGlobalGameEvent( const char *szName, HSCRIPT params )
{
	if ( !szName || !*szName )
		return false;

	IGameEvent *event = gameeventmanager->CreateEvent( szName );
	if ( !event )
		return false;

	// Josh: This wasn't as bad as I thought it would be!
	{
		int nIter = 0;
		int nEntries = g_pScriptVM->GetNumTableEntries( params );
		for ( int i = 0; i < nEntries; i++ )
		{
			ScriptVariant_t vKey, vValue;
			nIter = g_pScriptVM->GetKeyValue( params, nIter, &vKey, &vValue );

			if ( vKey.GetType() != FIELD_CSTRING )
			{
				Log_Msg( LOG_VScript, "VSCRIPT: ScriptSendRealGameEvent: Key must be a FIELD_CSTRING" );
				continue;
			}
			const char *pszKeyName = (const char *)vKey;
			switch ( vValue.GetType() )
			{
				case FIELD_BOOLEAN: event->SetBool  ( pszKeyName, (bool)vValue );         break;
				case FIELD_FLOAT:   event->SetFloat ( pszKeyName, (float)vValue );        break;
				case FIELD_INTEGER: event->SetInt   ( pszKeyName, (int)vValue );          break;
				case FIELD_CSTRING: event->SetString( pszKeyName, (const char *)vValue ); break;
				case FIELD_UINT64:  event->SetUint64( pszKeyName, (uint64)vValue );       break;
				default:
				{
					Log_Msg( LOG_VScript, "VSCRIPT: ScriptSendRealGameEvent: Don't understand FIELD_TYPE of value for key %s.", pszKeyName );
					break;
				}
			}
		}
	}

	gameeventmanager->FireEvent( event );
	
	return true;
}

const Vector &RotatePosition( const Vector rotateOrigin, const QAngle rotateAngles, const Vector position )
{
	VMatrix rotationMatrix;
	static Vector vecRotated;
	rotationMatrix.SetupMatrixOrgAngles( rotateOrigin, rotateAngles );
	vecRotated = rotationMatrix.ApplyRotation( position );
	return vecRotated;
}

QAngle RotateOrientation( QAngle posAngles, QAngle entAngles )
{
	matrix3x4_t posMat;
	AngleMatrix( posAngles, posMat );

	matrix3x4_t entMat;
	AngleMatrix( entAngles, entMat );

	matrix3x4_t outMat;
	ConcatTransforms( posMat, entMat, outMat );
	
	QAngle outAngles;
	MatrixAngles( outMat, outAngles );
	return outAngles;
}

//-----------------------------------------------------------------------------
static void ParseTable( CBaseEntity *pEntity, HSCRIPT spawn_table, const char *pKeyNameOverride = NULL )
{
	ScriptVariant_t vKey, vValue;
	int nIter = 0;
	int num_entries = g_pScriptVM->GetNumTableEntries( spawn_table );

	for( int i = 0; i < num_entries; i++ )
	{
		nIter = g_pScriptVM->GetKeyValue( spawn_table, nIter, &vKey, &vValue);
		const char *pKeyName = pKeyNameOverride ? pKeyNameOverride : (const char *)vKey;

		switch ( vValue.GetType() )
		{
		case FIELD_QANGLE:
		case FIELD_VECTOR:
			pEntity->KeyValueFromVector( pKeyName, vValue );
			break;
		case FIELD_INTEGER:
			pEntity->KeyValueFromInt( pKeyName, (int)vValue );
			break;
		case FIELD_FLOAT:
			pEntity->KeyValueFromFloat( pKeyName, vValue );
			break;
		case FIELD_CSTRING:
			pEntity->KeyValueFromString( pKeyName, vValue );
			break;
		case FIELD_BOOLEAN:
			pEntity->KeyValueFromInt( pKeyName, (vValue ? 1 : 0) );
			break;
		case FIELD_HSCRIPT:				
			ParseTable( pEntity, vValue, vKey );
			break;
		default:
			Warning( "Unsupported KeyValue type for key %s (type %s)\n", (const char *)vKey, VariantFieldTypeName( vValue.GetType() ) );
			break;
		}

		g_pScriptVM->ReleaseValue( vKey );	
		g_pScriptVM->ReleaseValue( vValue );
	}
}

//// @todo: rewrite there 4 to not need classname everywhere since it is always in the spawntable!
//-----------------------------------------------------------------------------
static CBaseEntity *VScript_ParseEntity( const char *pszClassname, HSCRIPT hSpawnTable )
{
	if ( !pszClassname || !*pszClassname )
		return NULL;

	CBaseEntity *pEntity = CreateEntityByName( pszClassname );
	if ( !pEntity )
	{
		Warning( "Cannot spawn entity %s\n", pszClassname );
		return NULL;
	}

	ParseTable( pEntity, hSpawnTable );
	return pEntity;
}

//-----------------------------------------------------------------------------
CBaseEntity *ScriptCreateEntityFromTable( const char *pszClassname, HSCRIPT hSpawnTable )
{
	if ( !pszClassname || !*pszClassname )
		return NULL;

	CBaseEntity *pEntity = VScript_ParseEntity( pszClassname, hSpawnTable );

#ifdef TERROR
	TheDirector->GetChallengeMode()->RecordScriptSpawnedEntity( pEntity );
#endif

	return pEntity;
}

//-----------------------------------------------------------------------------
static HSCRIPT Script_SpawnEntityFromTable( const char *pszName, HSCRIPT spawn_table )
{
	if ( !pszName || !*pszName )
		return NULL;

	CBaseEntity *pEntity = ScriptCreateEntityFromTable( pszName, spawn_table );
	if ( !pEntity )
		return 0;

	pEntity->Precache();
	DispatchSpawn( pEntity );
	pEntity->Activate();

	return ToHScript( pEntity );
}

//-----------------------------------------------------------------------------
static bool Script_SpawnEntityGroupFromTable( HSCRIPT groupSpawnTables )
{
	int nIter = 0;
	ScriptVariant_t vKey, vValue, vClassname, vSpawnTable;
	int nEntities = g_pScriptVM->GetNumTableEntries( groupSpawnTables );

	HierarchicalSpawn_t *pSpawnList = (HierarchicalSpawn_t*)stackalloc( nEntities * sizeof(HierarchicalSpawn_t) );

	int numEnts = 0;
	for ( int i = 0; i < nEntities; ++i )
	{
		nIter = g_pScriptVM->GetKeyValue( groupSpawnTables, nIter, &vKey, &vValue );

		Assert( vValue.GetType() == FIELD_HSCRIPT );
		g_pScriptVM->GetKeyValue( vValue, 0, &vClassname, &vSpawnTable );

		// Create the entity from the spawn table
		CBaseEntity *pEntity = ScriptCreateEntityFromTable( vClassname, vSpawnTable );
		if ( pEntity == NULL )
		{
			Msg( "Failed to spawn group entity %s\n", (const char *)vClassname );
			continue;
		}

		pSpawnList[numEnts].m_hEntity = pEntity;
		pSpawnList[numEnts].m_nDepth = 0;
		pSpawnList[numEnts].m_pDeferredParent = NULL;
		++numEnts;

 		g_pScriptVM->ReleaseValue( vKey );	
 		g_pScriptVM->ReleaseValue( vValue );
 		g_pScriptVM->ReleaseValue( vClassname );	
 		g_pScriptVM->ReleaseValue( vSpawnTable );
	}

	SpawnHierarchicalList( numEnts, pSpawnList, true );

	return true;
}

//-----------------------------------------------------------------------------
static void Script_EmitSoundOn( const char *pszSoundName, HSCRIPT hEnt )
{
	if ( !pszSoundName || !*pszSoundName )
		return;

	CBaseEntity *pEnt = ToEnt( hEnt );
	if ( pEnt )
	{
		pEnt->EmitSound( pszSoundName );
	}
}


//-----------------------------------------------------------------------------
static void Script_StopSoundOn( const char *pszSoundName, HSCRIPT hEnt )
{
	if ( !pszSoundName || !*pszSoundName )
		return;

	CBaseEntity *pEnt = ToEnt( hEnt );
	if ( pEnt )
	{
		pEnt->StopSound( pszSoundName );
	}
}


//-----------------------------------------------------------------------------
static void Script_EmitSoundOnClient( const char *pszSoundName, HSCRIPT hEnt )
{
	if ( !pszSoundName || !*pszSoundName )
		return;

	CBaseEntity *pEnt = ToEnt( hEnt );
	if ( pEnt && pEnt->IsPlayer() )
	{
		CSingleUserRecipientFilter filter( ToBasePlayer( pEnt ) );
		pEnt->EmitSound( filter, pEnt->entindex(), pszSoundName );
	}
}

enum EScriptRecipientFilter
{
	RECIPIENT_FILTER_DEFAULT,
	RECIPIENT_FILTER_PAS_ATTENUATION,
	RECIPIENT_FILTER_PAS,
	RECIPIENT_FILTER_PVS,
	RECIPIENT_FILTER_SINGLE_PLAYER,
	RECIPIENT_FILTER_GLOBAL,
	RECIPIENT_FILTER_TEAM,
};

typedef CPASAttenuationFilter CScriptRecipientFilter;

void SetupScriptRecipientFilter( CScriptRecipientFilter& filter, EScriptRecipientFilter type, int nParam, Vector *pOrigin = NULL, CBaseEntity *pEntity = NULL, const char *pszSoundName = NULL )
{
	Vector vecOrigin = vec3_origin;
	if ( pEntity )
		vecOrigin = pszSoundName ? pEntity->GetSoundEmissionOrigin() : pEntity->GetAbsOrigin();
	if ( pOrigin )
		vecOrigin = *pOrigin;

	switch ( type )
	{
		default:
		case RECIPIENT_FILTER_DEFAULT:
		{
			if ( pEntity && pszSoundName )
				filter = CPASAttenuationFilter( pEntity, pszSoundName );
			else if ( pEntity || pOrigin )
				filter.AddRecipientsByPVS( vecOrigin );
			else
				filter.AddAllPlayers();

			break;
		}

		case RECIPIENT_FILTER_PAS_ATTENUATION:
		{
			if ( pEntity && pszSoundName )
				filter = CPASAttenuationFilter( pEntity, pszSoundName );
			break;
		}

		case RECIPIENT_FILTER_PAS:
		{
			filter.AddRecipientsByPAS( vecOrigin );
			break;
		}

		case RECIPIENT_FILTER_PVS:
		{
			filter.AddRecipientsByPVS( vecOrigin );
			break;
		}

		case RECIPIENT_FILTER_SINGLE_PLAYER:
		{
			CBasePlayer *pPlayer = ToBasePlayer( pEntity );
			if ( pPlayer )
				filter.AddRecipient( pPlayer );
			break;
		}

		case RECIPIENT_FILTER_GLOBAL:
		{
			filter.AddAllPlayers();
			break;
		}

		case RECIPIENT_FILTER_TEAM:
		{
			CTeam *pTeam = GetGlobalTeam( nParam );
			if ( pTeam )
				filter.AddRecipientsByTeam( pTeam );

			break;
		}
	}
}

static void Script_EmitSoundEx( HSCRIPT params )
{
	if ( !params )
		return;

	CBaseEntity *pOutputEntity = NULL;

	int nFilterParam = -1;
	bool bSetOrigin = false;
	Vector vecOriginCache = vec3_origin;
	EScriptRecipientFilter eFilter = RECIPIENT_FILTER_DEFAULT;
	CUtlString szSoundName;

	EmitSound_t soundParams;

	IScriptVM *pVM = g_pScriptVM;
	pVM->IfHas<int>			( params, "channel",		[&](int value)			{ soundParams.m_nChannel = value; } );
	pVM->IfHas<CUtlString>	( params, "sound_name",		[&](CUtlString value)	{ szSoundName = std::move( value ); soundParams.m_pSoundName = szSoundName.Get(); });
	pVM->IfHas<float>		( params, "volume",			[&](float value)		{ soundParams.m_flVolume = value; } );
	pVM->IfHas<int>			( params, "sound_level",	[&](int value)			{ soundParams.m_SoundLevel = soundlevel_t(value); } );
	pVM->IfHas<int>			( params, "flags",			[&](int value)			{ soundParams.m_nFlags = value; } );
	pVM->IfHas<int>			( params, "pitch",			[&](int value)			{ soundParams.m_nPitch = value; } );
	pVM->IfHas<int>			( params, "special_dsp",	[&](int value)			{ soundParams.m_nSpecialDSP = value; } );
	pVM->IfHas<Vector>		( params, "origin",			[&](Vector value)		{ vecOriginCache = value; bSetOrigin = true; soundParams.m_pOrigin = &vecOriginCache; } );
	pVM->IfHas<float>		( params, "delay",			[&](float value)		{ soundParams.m_flSoundTime = gpGlobals->curtime + value; } );
	pVM->IfHas<float>		( params, "sound_time",		[&](float value)		{ soundParams.m_flSoundTime = value; } );
	pVM->IfHas<HSCRIPT>		( params, "speaker_entity",	[&](HSCRIPT value)		{ CBaseEntity *pEntity = ToEnt( value ); if ( pEntity ) soundParams.m_nSpeakerEntity = pEntity->entindex(); });
	pVM->IfHas<HSCRIPT>		( params, "entity",			[&](HSCRIPT value)		{ pOutputEntity = ToEnt( value ); });
	pVM->IfHas<int>			( params, "filter_type",	[&](int value)			{ eFilter = EScriptRecipientFilter( value ); });
	pVM->IfHas<int>			( params, "filter_param",	[&](int value)			{ nFilterParam = value; });
	soundParams.m_bWarnOnMissingCloseCaption = false;
	soundParams.m_bWarnOnDirectWaveReference = false;

	if ( !soundParams.m_pSoundName || !*soundParams.m_pSoundName )
		return;

	CScriptRecipientFilter filter;
	SetupScriptRecipientFilter( filter, eFilter, nFilterParam, bSetOrigin ? &vecOriginCache : NULL, pOutputEntity, soundParams.m_pSoundName );

	CBaseEntity::EmitSound( filter, pOutputEntity ? pOutputEntity->entindex() : -1, soundParams );
}

//-----------------------------------------------------------------------------
static bool Script_PrecacheItemFromTable( HSCRIPT hSpawnTable )
{
	ScriptVariant_t vClassname;
	if ( !g_pScriptVM->GetValue( hSpawnTable, "classname", &vClassname ) )
	{
		DevWarning( "Hey - your spawntable doesn't have a classname");
		return false;
	}
		//	CBaseEntity *pEntity = VScript_ParseEntity( pszClassname, spawn_table );
	CBaseEntity *pEntity = VScript_ParseEntity( vClassname, hSpawnTable );
	if ( pEntity )
	{
		DispatchSpawn( pEntity, false );
		UTIL_Remove( pEntity );
		return true;
	}
	return false;
}

static float Script_GetFriction( HSCRIPT hEnt )
{
	CBaseEntity *pEnt = ToEnt( hEnt );
	if ( !pEnt )
		return 0.0f;
	return pEnt->GetFriction();
}

static void Script_PickupObject( HSCRIPT hPlayer, HSCRIPT hEnt)
{
	CBaseEntity *pEnt  = ToEnt( hEnt );
	CBaseEntity *pBaseP = ToEnt( hPlayer );
	CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >(pBaseP);
	if (pPlayer)
		pPlayer->PickupObject( pEnt, false );
}

static void Script_OverlayCircle( Vector vCenter, Vector vCol, float flAlpha, float flRad, bool bZTest, float flDur )
{
	NDebugOverlay::Circle( vCenter, Vector( 1, 0, 0 ), Vector( 0, -1, 0 ), flRad, vCol.x, vCol.y, vCol.z, flAlpha, bZTest, flDur);
}

static void Script_OverlayLine_vCol( Vector vStart, Vector vEnd, Vector vCol, bool bZTest, float flDur )
{
	NDebugOverlay::Line( vStart, vEnd, vCol.x, vCol.y, vCol.z, bZTest, flDur );
}

static void Script_OverlayBoxDirection( Vector vCenter, Vector vMin, Vector vMax, Vector vForward, Vector vCol, float flAlpha, float flDur )
{
	NDebugOverlay::BoxDirection( vCenter, vMin, vMax, vForward, vCol.x, vCol.y, vCol.z, flAlpha, flDur);
}

static void Script_OverlayBoxAngles( Vector vCenter, Vector vMin, Vector vMax, QAngle qAngles, Vector vCol, float flAlpha, float flDur )
{
	NDebugOverlay::BoxAngles( vCenter, vMin, vMax, qAngles, vCol.x, vCol.y, vCol.z, flAlpha, flDur);
}

static void Script_OverlayClear( void )
{
	if (debugoverlay)
		debugoverlay->ClearAllOverlays();
	else
		DevMsg("No debugoverlay to clear with\n");
}

static const Vector& Script_GetPhysVelocity( HSCRIPT hEnt )
{
	DevMsg("Using legacy GetPhysVelocity.\nDo not use me! Use GetPhysVelocity on the entity instead.\n");
	CBaseEntity *pEnt = ToEnt(hEnt);
	if ( !pEnt )
		return vec3_origin;

	if ( pEnt->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		static Vector vVel;
		if (pEnt->VPhysicsGetObject())
			pEnt->VPhysicsGetObject()->GetVelocity( &vVel, NULL );
		return vVel;
	}
	else
		return pEnt->GetAbsVelocity();
}

static const Vector& Script_GetPhysAngularVelocity( HSCRIPT hEnt )
{
	DevMsg("Using legacy GetPhysAngularVelocity.\nDo not use me! Use GetPhysAngularVelocity on the entity instead.\n");
	CBaseEntity *pEnt = ToEnt(hEnt);
	if ( !pEnt )
		return vec3_origin;

	static Vector vAng = Vector(0,0,0);
	if ( pEnt->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		if (pEnt->VPhysicsGetObject())
			pEnt->VPhysicsGetObject()->GetVelocity( NULL, &vAng );
	}
	else
	{
		QAngle qAng = pEnt->GetLocalAngularVelocity();
		vAng.x = qAng.x; vAng.y = qAng.y; vAng.z = qAng.z;
	}
	return vAng;
}

// the prototype in client.h is actually wrong
extern void Host_Say( edict_t *pEdict, const CCommand &args, bool teamonly );

static void Script_Say( HSCRIPT srcEnt, const char *pszStr, bool bTeamOnly = false )
{
	CCommand tmpArg;
	CBaseEntity *pEntity = ToEnt( srcEnt );

	// char tmp[1024];
	// V_snprintf(tmp,sizeof(tmp),"say%s %s",bT)
	
	tmpArg.Tokenize(pszStr);
	if ( pEntity )
	{
		//if (( pPlayer->LastTimePlayerTalked() + TALK_INTERVAL ) < gpGlobals->curtime) 
		Host_Say( pEntity->edict(), tmpArg, bTeamOnly);
		// pPlayer->NotePlayerTalked();
	}
	else
	{
		Host_Say( NULL, tmpArg, 0 );
	}
}

static ConVarRef ref_script_think_interval("sv_script_think_interval", "0.1");
static void Script_AddThinkToEnt( HSCRIPT srcEnt, const char *pszThinkFunc )
{
	CBaseEntity *pEntity = ToEnt( srcEnt );
	if ( !pEntity )
		return;

	// this is horrifying
	if (pszThinkFunc)
	{
		pEntity->m_iszScriptThinkFunction = AllocPooledString( pszThinkFunc );
		pEntity->SetContextThink(&CBaseEntity::ScriptThink, gpGlobals->curtime + ref_script_think_interval.GetFloat(), "ScriptThink" );
	}
	else 
		pEntity->SetContextThink(NULL, 0, "ScriptThink" );  		//pEntity->m_iszScriptThinkFunction = NULL_STRING;
}

static HSCRIPT Script_GetPlayerFromUserID( int userID )
{
	CBasePlayer *pPlayer = UTIL_PlayerByUserId( userID );     // returns NULL if nonexistant
	// DevMsg("p.s. id %d player %x is %s\n", userID, pPlayer, engine->IsUserIDInUse( userID ) ? "InUse" : "NotInUse" );
	if ( !pPlayer )
		return NULL;

	if ( pPlayer->IsHLTV() )
		return NULL;

	return ToHScript( pPlayer );
}

static bool Script_IsPlayerABot( HSCRIPT hEnt )
{
	CBasePlayer *pPlayer = ToBasePlayer( ToEnt( hEnt ) );
	return pPlayer ? pPlayer->IsBot() : false;
}

// Users have requested the ability to write to subdirectories of /scriptdata/, so some extra
// validation will be needed to prevent writing to any locations outside the scriptdata directory
template <size_t maxLenInChars> 
static bool CreateAndValidateFileLocation( char (&pDest)[maxLenInChars], const char *pFileName )
{
	// establish the scriptdata directory
	char szFullSCRIPTDATAPath[MAX_PATH];
	V_MakeAbsolutePath( szFullSCRIPTDATAPath, sizeof( szFullSCRIPTDATAPath ), "scriptdata" );

	// Get the filepath, clean it up, and make it a subdir of /scriptdata/
	char szPath[MAX_PATH];
	char szFullFilePath[MAX_PATH];
	char szFixedPathName[MAX_PATH];
	V_FixupPathName( szFixedPathName, sizeof( szFixedPathName ), pFileName );
	V_snprintf( szPath, sizeof( szPath ), "scriptdata%c%s", CORRECT_PATH_SEPARATOR, szFixedPathName );
	V_MakeAbsolutePath( szFullFilePath, sizeof( szFullFilePath ), szPath );

	// Get the relative path, if possible
	char szRelativePath[MAX_PATH];
	bool bSuccess = V_MakeRelativePath( szFullFilePath, szFullSCRIPTDATAPath, szRelativePath, sizeof( szRelativePath ) );

	// Don't allow users to move outside the scriptdata folder
	if ( !bSuccess || V_stristr( szRelativePath, ".." ) )
	{
		Warning( "Invalid file location: %s\n", szFullFilePath );
		return false;
	}

	// Now build the final path
	char szDirHierarchy[ MAX_PATH ];
	V_ExtractFilePath( szPath, szDirHierarchy, sizeof( szDirHierarchy ) );
	g_pFullFileSystem->CreateDirHierarchy( szDirHierarchy, "DEFAULT_WRITE_PATH" );
	V_snprintf( pDest, maxLenInChars, "scriptdata%c%s", CORRECT_PATH_SEPARATOR, szRelativePath );
	return true;
}

static bool Script_StringToFile( const char *pszFileName, const char *pszTmp )
{
	if ( !pszFileName || !*pszFileName )
	{
		Log_Warning( LOG_VScript, "Script_StringToFile: NULL/empty file name\n" );
		return false;
	}

	if ( V_strstr( pszFileName, "..") )
	{
		Log_Warning( LOG_VScript, "StringToFile() file name cannot contain '..'\n" );
		return false;
	}

	char szFilePath[MAX_PATH];
	if ( !CreateAndValidateFileLocation( szFilePath, pszFileName ) )
		return false;

	FileHandle_t hFile = g_pFullFileSystem->Open( szFilePath, "wt", "DEFAULT_WRITE_PATH" );  // is this failing and not allowing a rewrite?
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		Warning("Couldn't open %s (as %p) to write out the table\n", szFilePath, pszFileName );
		return false;
	}
//	int rval = 
	g_pFullFileSystem->Write( pszTmp, V_strlen(pszTmp)+1, hFile );
//	DevMsg("Think we wrote to %s rval is %d and size is %d\n", pszFileName, rval, V_strlen(pszTmp)+1 );
	g_pFullFileSystem->Close( hFile );
	return true;
}

#define FILE_TO_STRING_BUF_SIZE 16384
static char fileReadBuf[ FILE_TO_STRING_BUF_SIZE + 1 ];
static const char *Script_FileToString( const char *pszFileName )
{
	if ( !pszFileName || !*pszFileName )
	{
		Log_Warning( LOG_VScript, "Script_FileToString: NULL/empty file name\n" );
		return NULL;
	}

	if ( V_strstr( pszFileName, "..") )
	{
		Log_Warning( LOG_VScript, "FileToString() file name cannot contain '..'\n" );
		return NULL;
	}

	char szFilePath[MAX_PATH];
	if ( !CreateAndValidateFileLocation( szFilePath, pszFileName ) )
		return NULL;

	FileHandle_t hFile = g_pFullFileSystem->Open( szFilePath, "r", "DEFAULT_WRITE_PATH" );
	if (hFile == FILESYSTEM_INVALID_HANDLE )
		return NULL;

	g_pFullFileSystem->Seek( hFile, 0, FILESYSTEM_SEEK_TAIL );
	uint iFLen = g_pFullFileSystem->Tell( hFile );
	if ( iFLen > FILE_TO_STRING_BUF_SIZE )
	{
		Warning("File %s (from %s) is len %d too long for a ScriptFileRead\n", szFilePath, pszFileName, iFLen );
		return NULL;
	}
	g_pFullFileSystem->Seek( hFile, 0, FILESYSTEM_SEEK_HEAD );
	uint rval = g_pFullFileSystem->Read( fileReadBuf, iFLen, hFile);
	fileReadBuf[ rval ] = 0;  // null terminate the thing we just read!
	g_pFullFileSystem->Close( hFile );
//	DevMsg("Think we loaded, rval was %d and iflen was %d for file %s\n", rval, iFLen, pszFileName );
	return fileReadBuf;
}

void TraceToScriptVM( HSCRIPT hTable, Vector vStart, Vector vEnd, trace_t& tr )
{
	g_pScriptVM->SetValue( hTable, "fraction", tr.fraction );
	g_pScriptVM->SetValue( hTable, "hit", (tr.fraction != 1.0 || tr.startsolid ));
	g_pScriptVM->SetValue( hTable, "plane_normal", tr.plane.normal );
	g_pScriptVM->SetValue( hTable, "plane_dist", tr.plane.dist );
	g_pScriptVM->SetValue( hTable, "contents", tr.contents );
	if (tr.allsolid)
		g_pScriptVM->SetValue( hTable, "allsolid", tr.allsolid );
	if (tr.m_pEnt)
		g_pScriptVM->SetValue( hTable, "enthit", ToHScript(tr.m_pEnt));
	if (tr.startsolid)
		g_pScriptVM->SetValue( hTable, "startsolid", tr.startsolid);
	Vector vPos = vStart + ( tr.fraction * (vEnd - vStart));
	g_pScriptVM->SetValue( hTable, "pos", vPos );
	g_pScriptVM->SetValue( hTable, "startpos", tr.startpos );
	g_pScriptVM->SetValue( hTable, "endpos", tr.endpos );
	g_pScriptVM->SetValue( hTable, "surface_name", tr.surface.name ? tr.surface.name : "" );
	g_pScriptVM->SetValue( hTable, "surface_flags", tr.surface.flags );
	g_pScriptVM->SetValue( hTable, "surface_props", tr.surface.surfaceProps );
}

// Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, startsolid
static bool Script_TraceLineEx( HSCRIPT hTable )
{
	int mask = MASK_VISIBLE_AND_NPCS;
	int coll = COLLISION_GROUP_NONE;
	ScriptVariant_t rval;
	Vector vStart, vEnd;
	CBaseEntity *pIgnoreEnt = NULL;
	bool bNoParams = false;

	if (g_pScriptVM->GetValue( hTable, "start", &rval ) )
		vStart = rval;
	else
		bNoParams = true;
	if (g_pScriptVM->GetValue( hTable, "end", &rval ) )
		vEnd = rval;
	else
		bNoParams = true;
	if (g_pScriptVM->GetValue( hTable, "mask", &rval ))
		mask = rval;
	if (bNoParams)
	{
		Warning("Didnt supply start and end to Script TraceLine call, failing, setting called to false\n");
		DevMsg("Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, startsolid");
		return false;
	}
	if (g_pScriptVM->GetValue( hTable, "ignore", &rval ))
	{
		pIgnoreEnt = ToEnt((HSCRIPT)rval);
	}
	trace_t tr;
	UTIL_TraceLine( vStart, vEnd, mask, pIgnoreEnt, coll, &tr );

	TraceToScriptVM( hTable, vStart, vEnd, tr );

	return true;
}

// Inputs: start, end, hullmin, hullmax, mask, ignore  -- outputs: pos, fraction, hit, enthit, startsolid
static bool Script_TraceHull( HSCRIPT hTable )
{
	int mask = MASK_VISIBLE_AND_NPCS;
	int coll = COLLISION_GROUP_NONE;
	ScriptVariant_t rval;
	Vector vStart, vEnd;
	Vector vHullMin, vHullMax;
	CBaseEntity* pIgnoreEnt = NULL;
	bool bNoParams = false;

	if (g_pScriptVM->GetValue( hTable, "start", &rval ) )
		vStart = rval;
	else
		bNoParams = true;

	if (g_pScriptVM->GetValue( hTable, "end", &rval ) )
		vEnd = rval;
	else
		bNoParams = true;

	if (g_pScriptVM->GetValue( hTable, "hullmin", &rval ) )
		vHullMin = rval;
	else
		bNoParams = true;

	if (g_pScriptVM->GetValue( hTable, "hullmax", &rval ) )
		vHullMax = rval;
	else
		bNoParams = true;

	if (bNoParams)
	{
		Warning("Didnt supply start end, hullmin and hullmax to Script TraceHull call, failing, setting called to false\n");
		DevMsg("Inputs: ");
		return false;
	}

	if (g_pScriptVM->GetValue( hTable, "mask", &rval ))
	{
		mask = rval;
	}

	if (g_pScriptVM->GetValue( hTable, "ignore", &rval ))
	{
		pIgnoreEnt = ToEnt((HSCRIPT)rval);
	}

	trace_t tr;
	UTIL_TraceHull(vStart, vEnd, vHullMin, vHullMax, mask, pIgnoreEnt, coll, &tr);

	TraceToScriptVM( hTable, vStart, vEnd, tr );

	return true;
}

static int Script_GetFrameCount( void )
{
	return gpGlobals->framecount;
}

static void Script_ClientPrint( HSCRIPT hPlayer, int iDest, const char *pText )
{
	CBaseEntity *pBaseEntity = ToEnt( hPlayer );
	if ( pBaseEntity )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pBaseEntity );
		if ( pPlayer )
		{
			ClientPrint( pPlayer, iDest, pText );
		}
	}
	else
	{
		UTIL_ClientPrintAll( iDest, pText );
	}
}

static void ScriptEmitAmbientSoundOn( const char *soundname, float volume, int soundlevel, int pitch, HSCRIPT entity )
{
	if ( !soundname || !*soundname )
		return;

	CBaseEntity *pEntity = ToEnt( entity );
	if ( !pEntity )
		return;

	UTIL_EmitAmbientSound( pEntity->GetSoundSourceIndex(), pEntity->GetAbsOrigin(), soundname, volume, (soundlevel_t)soundlevel, 131, pitch, 0.0f, 0 );
}

static void ScriptStopAmbientSoundOn( const char *soundname, HSCRIPT entity )
{
	if ( !soundname || !*soundname )
		return;

	CBaseEntity *pEntity = ToEnt( entity );
	if ( !pEntity )
		return;

	UTIL_EmitAmbientSound( pEntity->GetSoundSourceIndex(), pEntity->GetAbsOrigin(), soundname, 0.0f, SNDLVL_NONE, 4, 0, 0.0f, 0 );
}

static void Script_SetFakeClientConVarValue( HSCRIPT fakeclient, const char *cvar, const char *value )
{
	if ( !cvar || !*cvar )
		return;

	if ( !value )
		return;

	CBaseEntity *pBaseEntity = ToEnt( fakeclient );
	if ( pBaseEntity )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pBaseEntity );
		if ( !pPlayer )
			return;

		if ( !pPlayer->IsFakeClient() )
			return;

		edict_t *pFakeClient = pPlayer->edict();
		if ( pFakeClient )
		{
			engine->SetFakeClientConVarValue( pFakeClient, cvar, value );
		}
	}
}

static void Script_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, int eCommand, bool bAirShake )
{
	UTIL_ScreenShake( center, amplitude, frequency, duration, radius, (ShakeCommand_t)eCommand, bAirShake );
}

static void Script_ScreenFade( HSCRIPT hEntity, int r, int g, int b, int a, float fadeTime, float fadeHold, int flags )
{
	color32 color = { (byte)r, (byte)g, (byte)b, (byte)a };

	CBaseEntity *pEntity = ToEnt( hEntity );
	if ( pEntity )
	{
		UTIL_ScreenFade( pEntity, color, fadeTime, fadeHold, flags );
	}
	else
	{
		UTIL_ScreenFadeAll( color, fadeTime, fadeHold, flags );
	}
}

int Script_PrecacheModel( const char *modelname )
{
	if ( !modelname || !*modelname )
	{
		Log_Warning( LOG_VScript, "Script_PrecacheModel: NULL/empty modelname\n" );
		return -1;
	}

	const bool bPrecacheAllowed = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	int nModelIndex = CBaseEntity::PrecacheModel( modelname );

	CBaseEntity::SetAllowPrecache( bPrecacheAllowed );

	return nModelIndex;
}

extern bool g_bPermitDirectSoundPrecache;

bool Script_PrecacheSound( const char *soundname )
{
	if ( !soundname || !*soundname )
		return false;

	const bool bPrecacheAllowed = CBaseEntity::IsPrecacheAllowed();
	const bool bDirectPrecacheAllowed = g_bPermitDirectSoundPrecache;
	CBaseEntity::SetAllowPrecache( true );
	g_bPermitDirectSoundPrecache = true;

	bool bRet = CBaseEntity::PrecacheSound( soundname );

	CBaseEntity::SetAllowPrecache( bPrecacheAllowed );
	g_bPermitDirectSoundPrecache = bDirectPrecacheAllowed;

	return bRet;
}

bool Script_PrecacheScriptSound( const char *pszScriptSoundName )
{
	if ( !pszScriptSoundName || !*pszScriptSoundName )
		return false;

	const bool bPrecacheAllowed = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	HSOUNDSCRIPTHANDLE hScriptHandle = CBaseEntity::PrecacheScriptSound( pszScriptSoundName );

	CBaseEntity::SetAllowPrecache( bPrecacheAllowed );

	return ( hScriptHandle != SOUNDEMITTER_INVALID_HANDLE );
}

bool Script_IsModelPrecached( const char *modelname )
{
	if ( !modelname || !*modelname )
		return false;

	return engine->IsModelPrecached( modelname );
}

int Script_GetModelIndex( const char *modelname )
{
	if ( !modelname || !*modelname )
		return false;

	return modelinfo->GetModelIndex( modelname );
}

bool Script_IsDedicatedServer()
{
	return engine->IsDedicatedServer();
}

HSCRIPT Script_GetListenServerHost()
{
	return ToHScript( UTIL_GetLocalPlayerOrListenServerHost() );
}

static float Script_GetSoundDuration( const char *soundname, const char *actormodel )
{
	if ( !soundname || !*soundname )
		return 0.0f;

	return CBaseEntity::GetSoundDuration( soundname, actormodel );
}

static bool Script_IsSoundPrecached( const char *soundname )
{
	if ( !soundname || !*soundname )
		return 0.0f;

	return enginesound->IsSoundPrecached( soundname );
}

static void Script_GetLocalTime( HSCRIPT hTable )
{
	if ( !hTable )
		return;

	tm timeValue;
	VCRHook_LocalTime( &timeValue ); 	// Calls time and gives you localtime's result.
	const tm *pLocalTime = &timeValue;

	g_pScriptVM->SetValue( hTable, "second", pLocalTime->tm_sec );
	g_pScriptVM->SetValue( hTable, "minute", pLocalTime->tm_min );
	g_pScriptVM->SetValue( hTable, "hour", pLocalTime->tm_hour );
	g_pScriptVM->SetValue( hTable, "day", pLocalTime->tm_mday );
	g_pScriptVM->SetValue( hTable, "month", (pLocalTime->tm_mon + 1) );
	g_pScriptVM->SetValue( hTable, "year", (pLocalTime->tm_year + 1900) );
	g_pScriptVM->SetValue( hTable, "dayofweek", pLocalTime->tm_wday );
	g_pScriptVM->SetValue( hTable, "dayofyear", pLocalTime->tm_yday );
	g_pScriptVM->SetValue( hTable, "daylightsavings", pLocalTime->tm_isdst );
	
}

static void Script_FadeClientVolume( HSCRIPT hPlayer, float fadePercent, float fadeOutSeconds, float holdTime, float fadeInSeconds )
{
	CBaseEntity *pBaseEntity = ToEnt( hPlayer );
	if ( pBaseEntity )
	{
		CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pBaseEntity );
		if ( pPlayer )
		{
			engine->FadeClientVolume( pPlayer->edict(), fadePercent, fadeOutSeconds, holdTime, fadeInSeconds );
		}
	}
}

#if 0
static void Script_QueueSpeak( HSCRIPT hEntity, const char *pszConcept, float flDelay, const char *pszCriteria )
{
	CBaseEntity *pBaseEntity = ToEnt( hEntity );
	if ( !pBaseEntity )
		return;

	if ( !pszCriteria )
		pszCriteria = "";

	AI_CriteriaSet criteria;
	if ( V_strlen( pszCriteria ) > 0 )
	{
		criteria.Merge( pszCriteria );
	}

	AIConcept_t concept( pszConcept );
	QueueSpeak( concept, pBaseEntity, flDelay, criteria );
}
#endif

#if 0
static void DoRecordAchievementEvent( const char *pszAchievementname, int iPlayerIndex )
{
	if ( iPlayerIndex < 0 )
	{
		DevWarning( "DoRecordAchievementEvent called with invalid player index (%s, %d)!\n", pszAchievementname, iPlayerIndex );
		return;
	}
	CBasePlayer *pPlayer = NULL;
	if ( iPlayerIndex > 0 )
	{
		pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
		if ( !pPlayer )
		{
			DevWarning( "DoRecordAchievementEvent called with a player index that doesn't resolve to a player (%s, %d)!\n", pszAchievementname, iPlayerIndex );
			return;
		}
	}
	UTIL_RecordAchievementEvent( pszAchievementname, pPlayer );
}
#endif

bool DoIncludeScript( const char *pszScript, HSCRIPT hScope )
{
	if ( !VScriptRunScript( pszScript, hScope, true ) )
	{
		g_pScriptVM->RaiseException( CFmtStr( "Failed to include script \"%s\"", ( pszScript ) ? pszScript : "unknown" ) );
		return false;
	}
	return true;
}

int GetDeveloperLevel()
{
	return developer.GetInt();
}

static void ScriptDispatchParticleEffect( const char *pszParticleName, const Vector &vOrigin, const Vector &vAngle )
{
	QAngle qAngle;
	VectorAngles( vAngle, qAngle );
	DispatchParticleEffect( pszParticleName, vOrigin, qAngle );
}

static void ScriptSetSkyboxTexture( const char* pszSkyboxName )
{
	if ( !pszSkyboxName || !*pszSkyboxName )
	{
		DevMsg( "ScriptSetSkyboxTexture has no skybox specified!\n" );
		return;
	}

	char  name[ MAX_PATH ];
	char *skyboxsuffix[ 6 ] = { "rt", "bk", "lf", "ft", "up", "dn" };
	for ( int i = 0; i < 6; i++ )
	{
		Q_snprintf( name, sizeof( name ), "skybox/%s%s", pszSkyboxName, skyboxsuffix[i] );
		PrecacheMaterial( name );
	}

	static ConVarRef sv_skyname( "sv_skyname", false );
	if ( sv_skyname.IsValid() )
	{
		sv_skyname.SetValue( pszSkyboxName );
	}
}

HSCRIPT CreateProp( const char *pszEntityName, const Vector &vOrigin, const char *pszModelName, int iAnim )
{
	if ( !pszEntityName || !*pszEntityName )
		return NULL;

	CBaseAnimating *pBaseEntity = (CBaseAnimating *)CreateEntityByName( pszEntityName );
	pBaseEntity->SetAbsOrigin( vOrigin );
	if ( pszModelName )
		pBaseEntity->SetModel( pszModelName );
	pBaseEntity->SetPlaybackRate( 1.0f );

	int iSequence = pBaseEntity->SelectWeightedSequence( (Activity)iAnim );

	if ( iSequence != -1 )
	{
		pBaseEntity->SetSequence( iSequence );
	}

	return ToHScript( pBaseEntity );
}

//--------------------------------------------------------------------------------------------------
// Use an entity's script instance to add an entity IO event (used for firing events on unnamed entities from vscript)
//--------------------------------------------------------------------------------------------------
static void DoEntFireByInstanceHandle( HSCRIPT hTarget, const char *pszAction, const char *pszValue, float delay, HSCRIPT hActivator, HSCRIPT hCaller )
{
	const char *action = "Use";
	variant_t value;

	if ( pszAction && *pszAction )
	{
		action = STRING( AllocPooledString( pszAction ) );
	}
	if ( pszValue && *pszValue )
	{
		value.SetString( AllocPooledString( pszValue ) );
	}
	if ( delay < 0 )
	{
		delay = 0;
	}

	CBaseEntity* pTarget = ToEnt(hTarget);

	if ( !pTarget )
	{
		Warning( "VScript error: DoEntFire was passed an invalid entity instance.\n" );
		return;
	}

	g_EventQueue.AddEvent( pTarget, action, value, delay, ToEnt(hActivator), ToEnt(hCaller) );
}

static float ScriptTraceLine( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	// UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, ptr );
	trace_t tr;
	CBaseEntity *pLooker = ToEnt(entIgnore);
	UTIL_TraceLine( vecStart, vecEnd, MASK_NPCWORLDSTATIC, pLooker, COLLISION_GROUP_NONE, &tr);
	if (tr.fractionleftsolid && tr.startsolid)
	{
		return 1.0 - tr.fractionleftsolid;
	}
	else
	{
		return tr.fraction;
	}
}

static float ScriptTraceLinePlayersIncluded( const Vector &vecStart, const Vector &vecEnd, HSCRIPT entIgnore )
{
	// UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, ptr );
	trace_t tr;
	CBaseEntity *pLooker = ToEnt( entIgnore );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pLooker, COLLISION_GROUP_NONE, &tr );
	if ( tr.fractionleftsolid && tr.startsolid )
	{
		return 1.0 - tr.fractionleftsolid;
	}
	else
	{
		return tr.fraction;
	}
}

#include "usermessages.h"

#if defined ( PORTAL2 )

#define LOCAL_MAP_PLAY_ORDER_FILENAME	"scripts/vo_progress.txt"

// Used for scripts playing Cave VO
CUtlVector< PublishedFileId_t >	g_vecLocalMapPlayOrder;

//-----------------------------------------------------------------------------
// Purpose: Load the user's played map order off the disk, disallowing duplicates and allowing us to know what's already been played
// FIXME:	For lack of a better place, I'm putting this here
//-----------------------------------------------------------------------------
bool LoadLocalMapPlayOrder( void )
{
	// Load our keyvalues from disk
	KeyValues *pKV = new KeyValues( "MapPlayOrder" );
	if ( pKV->LoadFromFile( g_pFullFileSystem, LOCAL_MAP_PLAY_ORDER_FILENAME, "MOD" ) == false )
		return false;

	// Grab all of our subkeys (should all be maps)
	for ( KeyValues *sub = pKV->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
	{
		if ( !Q_stricmp( sub->GetName(), "map" ) )
		{
			// Add the map's ID to our list
			PublishedFileId_t mapID = sub->GetUint64();
			if ( g_vecLocalMapPlayOrder.Find( mapID ) == g_vecLocalMapPlayOrder.InvalidIndex() )
			{
				g_vecLocalMapPlayOrder.AddToTail( mapID );
			}
		}
		else
		{
			Warning("Ill-formed parameter found in map progress file!\n" );
		}
	}

	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Load the user's played map order off the disk, disallowing duplicates and allowing us to know what's already been played
// FIXME:	For lack of a better place, I'm putting this here
//-----------------------------------------------------------------------------
bool SaveLocalMapPlayOrder( void )
{
	// Load our keyvalues from disk
	KeyValues *pKV = new KeyValues( "MapPlayOrder" );
	KeyValues *pSubKeyTemplate = new KeyValues( "Map" );
	pSubKeyTemplate->SetUint64( "map", 0 );

	// Take all the maps in our list and dump them to the file
	for ( int i=0; i < g_vecLocalMapPlayOrder.Count(); i++ )
	{
		KeyValues *pNewKey = pSubKeyTemplate->MakeCopy();
		pNewKey->SetUint64( "map", g_vecLocalMapPlayOrder[i] );
		pKV->AddSubKey( pNewKey );
		pKV->ElideSubKey( pNewKey ); // This strips the outer "map"{ } parent and leaves the sub-keys as peers to one another in the final file
	}

	// Serialize it
	if ( pKV->SaveToFile( g_pFullFileSystem, LOCAL_MAP_PLAY_ORDER_FILENAME, "MOD" ) == false )
	{
		Assert( 0 );
		return false;
	}

	pSubKeyTemplate->deleteThis();
	pKV->deleteThis();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Get the order index of the local map (order played) by its published file ID
//-----------------------------------------------------------------------------
int GetLocalMapIndexByPublishedFileID( PublishedFileId_t unFileID )
{
	int nIndex = g_vecLocalMapPlayOrder.Find( unFileID );
	if ( nIndex == g_vecLocalMapPlayOrder.InvalidIndex() )
		return -1;

	return nIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Get the order index of the local map (order played) by its published file ID
//-----------------------------------------------------------------------------
bool SetLocalMapPlayed( PublishedFileId_t unFileID )
{
	// Don't allow dupes
	int nIndex = g_vecLocalMapPlayOrder.Find( unFileID );
	if ( nIndex != g_vecLocalMapPlayOrder.InvalidIndex() )
		return false;

	// New entry, take it
	g_vecLocalMapPlayOrder.AddToTail( unFileID );

	return SaveLocalMapPlayOrder();	// FIXME: We probably don't need to do this every time and right away, but convenient for now
}

static void SetDucking( const char *pszLayerName, const char *pszMixGroupName, float factor )
{
	CReliableBroadcastRecipientFilter filter;

	CCSUsrMsg_SetMixLayerTriggerFactor msg;
	msg.set_layer(pszLayerName);
	msg.set_group(pszMixGroupName);
	msg.set_factor(factor);
	SendUserMessage(filter, CS_UM_SetMixLayerTriggerFactor, msg);
}
#endif

#if defined( PORTAL2_PUZZLEMAKER )
ConVar cm_current_community_map( "cm_current_community_map", "0", FCVAR_HIDDEN | FCVAR_REPLICATED );

void RequestMapRating( void )
{
	// Request this blindly and make the listener decide if it's valid
	g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( new KeyValues( "OnRequestMapRating" ) );
	g_Portal2ResearchDataTracker.Event_LevelCompleted();
}

//
// Get the index of the map in our play order (-1 if not there, -2 if the current map isn't a community map)
//

int GetMapIndexInPlayOrder( void )
{
	PublishedFileId_t nMapID = (uint64) atol(cm_current_community_map.GetString());

	// Coop maps will NOT play dialog.  The hooks remain in the maps, so we can change this in
	// the future if we need to.
	if ( nMapID == 0 || GameRules()->IsCoOp() )
		return -2;	// This map isn't a known community map!

	// Returns -1 if it's not found (but IS a valid community map. The user needs to add it at this point).
	return GetLocalMapIndexByPublishedFileID( nMapID );
}

//
// Get the number of maps the player has played through
//

int GetNumMapsPlayed( void )
{
	return g_vecLocalMapPlayOrder.Count();
}

//
// Set the currently map has having been "played" in the eyes of the VO (-1 denotes a failure, -2 if the current map isn't a community map)
//

int SetMapAsPlayed( void )
{
	PublishedFileId_t nMapID = (uint64) atol(cm_current_community_map.GetString());
	if ( nMapID == 0 )
		return -2;	// This map isn't a known community map!

	// Add our map as being "played" in the eyes of the VO scripts
	if ( SetLocalMapPlayed( nMapID ) )
		return GetLocalMapIndexByPublishedFileID( nMapID );

	return -1;
}
#endif // PORTAL2_PUZZLEMAKER

bool VScriptServerInit()
{
	VMPROF_START

	if( scriptmanager != NULL )
	{
		ScriptLanguage_t scriptLanguage = SL_DEFAULT;

		char const *pszScriptLanguage;
		if ( CommandLine()->CheckParm( "-scriptlang", &pszScriptLanguage ) )
		{
			if( !Q_stricmp(pszScriptLanguage, "gamemonkey") )
			{
				scriptLanguage = SL_GAMEMONKEY;
			}
			else if( !Q_stricmp(pszScriptLanguage, "squirrel") )
			{
				scriptLanguage = SL_SQUIRREL;
			}
			else if( !Q_stricmp(pszScriptLanguage, "python") )
			{
				scriptLanguage = SL_PYTHON;
			}
			else
			{
				DevWarning("-server_script does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}

		}
		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if ( ( script_attach_debugger_at_startup.GetBool() || CommandLine()->CheckParm( "-vscriptdebug" ) ) && g_pScriptVM )
			{
				g_pScriptVM->ConnectDebugger();
			}

			if( g_pScriptVM )
			{
				Log_Msg( LOG_VScript, "VSCRIPT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
				g_pScriptVM->SetErrorCallback( &VScriptServerScriptErrorFunc );

				ScriptRegisterFunctionNamed( g_pScriptVM, UTIL_ShowMessageAll, "ShowMessage", "Print a hud message on all clients" );
				ScriptRegisterFunction( g_pScriptVM, SendToConsole, "Send a string to the console as a command" );
				ScriptRegisterFunction( g_pScriptVM, SendToServerConsole, "Send a string that gets executed on the server as a ServerCommand. Respects sv_allow_point_servercommand." );
				ScriptRegisterFunctionNamed( g_pScriptVM, SendToServerConsole, "SendToConsoleServer", "Copy of SendToServerConsole with another name for compat." );
				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, DoEntFire, SCRIPT_ALIAS( "EntFire", "Generate and entity i/o event" ) );
				ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string. Useful for adding data to tables when not sure what keys are already in use in that table." ) );
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
				ScriptRegisterFunction( g_pScriptVM, RegisterScriptGameEventListener, "Register as a listener for a game event from script." );
				ScriptRegisterFunction( g_pScriptVM, RegisterScriptHookListener, "Register as a listener for a script hook from script." );
				ScriptRegisterFunction( g_pScriptVM, EntIndexToHScript, "Turn an entity index integer to an HScript representing that entity's script instance." );
				ScriptRegisterFunction( g_pScriptVM, PlayerInstanceFromIndex, "Get a script instance of a player by index." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFireGameEvent, "FireGameEvent", "Fire a game event to a listening callback function in script. Parameters are passed in a squirrel table." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptFireScriptHook, "FireScriptHook", "Fire a script hoook to a listening callback function in script. Parameters are passed in a squirrel table." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptSendGlobalGameEvent, "SendGlobalGameEvent", "Sends a real game event to everything. Parameters are passed in a squirrel table." );
				ScriptRegisterFunction( g_pScriptVM, ScriptHooksEnabled, "Returns whether script hooks are currently enabled." );

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_SpawnEntityFromTable, "SpawnEntityFromTable", "Spawn entity from KeyValues in table - 'name' is entity name, rest are KeyValues for spawn." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_SpawnEntityGroupFromTable, "SpawnEntityGroupFromTable", "Hierarchically spawn an entity group from a set of spawn tables." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PrecacheItemFromTable, "PrecacheEntityFromTable", "Precache an entity from KeyValues in table" );

				ScriptRegisterFunction( g_pScriptVM, RotatePosition, "Rotate a Vector around a point." );
				ScriptRegisterFunction( g_pScriptVM, RotateOrientation, "Rotate a QAngle by another QAngle." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_EmitSoundOn, "EmitSoundOn", "Play named sound on Entity. Legacy only, use EmitSoundEx." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_EmitSoundOnClient, "EmitSoundOnClient", "Play named sound only on the client for the passed in player. NOTE: This only supports soundscripts. Legacy only, use EmitSoundEx." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_EmitSoundEx, "EmitSoundEx", "Play a sound. Takes in a script table of params." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_StopSoundOn, "StopSoundOn", "Stop named sound on Entity." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetPhysVelocity, "GetPhysVelocity","Get Velocity for VPHYS or normal object" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetPhysAngularVelocity, "GetPhysAngularVelocity","Get Angular Velocity for VPHYS or normal object" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_Say, "Say", "Have Entity say string, and teamOnly or not" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_AddThinkToEnt, "AddThinkToEnt", "Adds a late bound think function to the C++ think tables for the obj" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetFriction, "GetFriction", "Returns the Friction on a player entity, meaningless if not a player");

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetPlayerFromUserID, "GetPlayerFromUserID", "Given a user id, return the entity, or null");
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsPlayerABot, "IsPlayerABot", "Is this player/entity a bot");

				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::ScreenTextLine, "DebugDrawScreenTextLine", "Draw text with a line offset" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Text, "DebugDrawText", "Draw text in 3d (origin, text, bViewCheck, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Box, "DebugDrawBox", "Draw a debug overlay box" );
				ScriptRegisterFunctionNamed( g_pScriptVM, NDebugOverlay::Line, "DebugDrawLine", "Draw a debug overlay line" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_OverlayCircle, "DebugDrawCircle", "Draw a debug circle (center, rad, vRgb, a, ztest, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_OverlayLine_vCol, "DebugDrawLine_vCol", "Draw a debug line using color vec (start, end, vRgb, a, ztest, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_OverlayBoxDirection, "DebugDrawBoxDirection", "Draw a debug forward box (cent, min, max, forward, vRgb, a, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_OverlayBoxAngles, "DebugDrawBoxAngles", "Draw a debug oriented box (cent, min, max, angles(p,y,r), vRgb, a, duration)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_OverlayClear, "DebugDrawClear", "Try to clear all the debug overlay info" );
				
// Josh: Bring this back if we have response rules and stuff.
#if 0
				ScriptRegisterFunctionNamed( g_pScriptVM, CSpeechScriptBridge::Script_AddDecisionRule, "rr_AddDecisionRule", "Add a rule to the decision database." );
				ScriptRegisterFunctionNamed( g_pScriptVM, CSpeechScriptBridge::Script_FindBestResponse, "rr_QueryBestResponse", "Params: (entity, query) : tests 'query' against entity's response system and returns the best response found (or null if none found)." );
				ScriptRegisterFunctionNamed( g_pScriptVM, CSpeechScriptBridge::Script_CommitAIResponse, "rr_CommitAIResponse", "Commit the result of QueryBestResponse back to the given entity to play. Call with params (entity, airesponse)" );
				ScriptRegisterFunctionNamed( g_pScriptVM, CSpeechScriptBridge::Script_GetExpressers, "rr_GetResponseTargets", "Retrieve a table of all available expresser targets, in the form { name : handle, name: handle }." );
#endif

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PickupObject, "PickupObject", "Have a player pickup a nearby named entity" );

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_StringToFile, "StringToFile", "Store a string to a file for later reading" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_FileToString, "FileToString", "Reads a string from a file to send to script" );

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_TraceLineEx, "TraceLineEx", "Pass table - Inputs: start, end, mask, ignore  -- outputs: pos, fraction, hit, enthit, allsolid, startpos, endpos, startsolid, plane_normal, plane_dist, surface_name, surface_flags, surface_props" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_TraceHull, "TraceHull", "Pass table - Inputs: start, end, hullmin, hullmax, mask, ignore  -- outputs: pos, fraction, hit, enthit, allsolid, startpos, endpos, startsolid, plane_normal, plane_dist, surface_name, surface_flags, surface_props" );

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetFrameCount, "GetFrameCount", "Returns the engines current frame count" );

				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ClientPrint, "ClientPrint", "Print a client message" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptEmitAmbientSoundOn, "EmitAmbientSoundOn", "Play named ambient sound on an entity." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptStopAmbientSoundOn, "StopAmbientSoundOn", "Stop named ambient sound on an entity." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_SetFakeClientConVarValue, "SetFakeClientConVarValue", "Sets a USERINFO client ConVar for a fakeclient" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ScreenShake, "ScreenShake", "Start a screenshake with the following parameters. vecCenter, flAmplitude, flFrequency, flDuration, flRadius, eCommand( SHAKE_START = 0, SHAKE_STOP = 1 ), bAirShake" );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ScreenFade, "ScreenFade", "Start a screenfade with the following parameters. player, red, green, blue, alpha, flFadeTime, flFadeHold, flags" );
//				ScriptRegisterFunctionNamed( g_pScriptVM, Script_ChangeLevel, "ChangeLevel", "Tell engine to change level." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PrecacheModel, "PrecacheModel", "Precache a model. Returns the modelindex." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PrecacheSound, "PrecacheSound", "Precache a sound." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_PrecacheScriptSound, "PrecacheScriptSound", "Precache a sound." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsModelPrecached, "IsModelPrecached", "Checks if the modelname is precached." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetModelIndex, "GetModelIndex", "Returns the index of the named model." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsDedicatedServer, "IsDedicatedServer", "Returns true if this server is a dedicated server." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetListenServerHost, "GetListenServerHost", "Get the local player on a listen server." );
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetSoundDuration, "GetSoundDuration", "Returns float duration of the sound. Takes soundname and optional actormodelname.");
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_IsSoundPrecached, "IsSoundPrecached", "Takes a sound name");
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_GetLocalTime, "LocalTime", "Fills out a table with the local time (second, minute, hour, day, month, year, dayofweek, dayofyear, daylightsavings)" );
#if 0
				ScriptRegisterFunctionNamed( g_pScriptVM, Script_QueueSpeak, "QueueSpeak", "(hEntity, szConcept, flDelay, szCriteria) Queue a speech concept" );
#endif

				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
//				ScriptRegisterFunction( g_pScriptVM, LoopSinglePlayerMaps, "Run the single player maps in a continuous loop.");

				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLine, "TraceLine", "given 2 points & ent to ignore, return fraction along line that hits world or models" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptTraceLinePlayersIncluded, "TraceLinePlayersIncluded", "given 2 points & ent to ignore, return fraction along line that hits world, models, players or npcs" );

				ScriptRegisterFunction( g_pScriptVM, FrameTime, "Get the time spent on the server in the last frame" );
				ScriptRegisterFunction( g_pScriptVM, MaxClients, "Get the current number of max clients set by the maxplayers command." );
				ScriptRegisterFunctionNamed( g_pScriptVM, DoEntFireByInstanceHandle, "EntFireByHandle", "Generate and entity i/o event. First parameter is an entity instance." );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptCreateSceneEntity, "CreateSceneEntity", "Create a scene entity to play the specified scene." );
				ScriptRegisterFunction( g_pScriptVM, CreateProp, "Create a physics prop" );
				//ScriptRegisterFunctionNamed( g_pScriptVM, DoRecordAchievementEvent, "RecordAchievementEvent", "Records achievement event or progress" );
				ScriptRegisterFunction( g_pScriptVM, GetDeveloperLevel, "Gets the level of 'developer'" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptDispatchParticleEffect, "DispatchParticleEffect", "Dispatches a one-off particle system" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptSetSkyboxTexture, "SetSkyboxTexture", "Sets the current skybox texture" );

#if defined ( PORTAL2 )
				ScriptRegisterFunction( g_pScriptVM, SetDucking, "Set the level of an audio ducking channel" );
#if defined( PORTAL2_PUZZLEMAKER )
				ScriptRegisterFunction( g_pScriptVM, RequestMapRating, "Pops up the map rating dialog for user input" );
				ScriptRegisterFunction( g_pScriptVM, GetMapIndexInPlayOrder, "Determines which index (by order played) this map is. Returns -1 if entry is not found. -2 if this is not a known community map." );
				ScriptRegisterFunction( g_pScriptVM, GetNumMapsPlayed, "Returns how many maps the player has played through." );
				ScriptRegisterFunction( g_pScriptVM, SetMapAsPlayed, "Adds the current map to the play order and returns the new index therein. Returns -2 if this is not a known community map." );
#endif	// PORTAL2_PUZZLEMAKER
#endif

				g_pScriptVM->RegisterAllClasses();
				
				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

#ifdef TF_DLL
				g_pScriptVM->RegisterInstance( TheNavMesh, "NavMesh" );
#endif
				g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );
#if 0
				g_pScriptVM->RegisterInstance( &g_ScriptPanorama, "Panorama" );
#endif
				g_pScriptVM->RegisterInstance( &g_ScriptConvars, "Convars" ) ;
				g_pScriptVM->RegisterInstance( &g_ScriptEntityOutputs, "EntityOutputs" );
				g_pScriptVM->RegisterInstance( &g_ScriptNetPropManager, "NetProps" );

				ScriptVariant_t	vConstantsTable;
				g_pScriptVM->CreateTable( vConstantsTable );
#define DECLARE_SCRIPT_CONST_TABLE( x ) ScriptVariant_t	vConstantsTable_##x; g_pScriptVM->CreateTable( vConstantsTable_##x );
#define DECLARE_SCRIPT_CONST_NAMED( type, name, x ) g_pScriptVM->SetValue( (HSCRIPT)vConstantsTable_##type, name, x );
#define DECLARE_SCRIPT_CONST( type, x ) DECLARE_SCRIPT_CONST_NAMED( type, #x, x )
#define REGISTER_SCRIPT_CONST_TABLE( x ) g_pScriptVM->SetValue( (HSCRIPT) vConstantsTable, #x, vConstantsTable_##x );
#ifdef TF_DLL
DECLARE_SCRIPT_CONST_TABLE( EScriptRecipientFilter )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_DEFAULT )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_PAS_ATTENUATION )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_PAS )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_PVS )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_SINGLE_PLAYER )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_GLOBAL )
DECLARE_SCRIPT_CONST( EScriptRecipientFilter, RECIPIENT_FILTER_TEAM )
REGISTER_SCRIPT_CONST_TABLE( EScriptRecipientFilter )

DECLARE_SCRIPT_CONST_TABLE( ETFCond )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVALID )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_AIMING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_ZOOMED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DISGUISING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DISGUISED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_STEALTHED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVULNERABLE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_TELEPORTED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_TAUNTING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVULNERABLE_WEARINGOFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_STEALTHED_BLINK )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SELECTED_TO_TELEPORT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_TMPDAMAGEBONUS )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_FEIGN_DEATH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PHASE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_STUNNED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_OFFENSEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SHIELD_CHARGE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DEMO_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_ENERGY_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RADIUSHEAL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HEALTH_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BURNING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HEALTH_OVERHEALED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_URINE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BLEEDING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DEFENSEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MAD_MILK )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEGAHEAL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_REGENONDAMAGEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MARKEDFORDEATH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_NOHEALINGDAMAGEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SPEED_BOOST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_PUMPKIN )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_USER_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_DEMO_CHARGE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SODAPOPPER_HYPE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_FIRST_BLOOD )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_BONUS_TIME )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_CTF_CAPTURE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_ON_KILL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CANNOT_SWITCH_FROM_MELEE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_REPROGRAMMED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_RAGE_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DEFENSEBUFF_HIGH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SNIPERCHARGE_RAGE_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DISGUISE_WEARINGOFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MARKEDFORDEATH_SILENT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DISGUISED_AS_DISPENSER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SAPPED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVULNERABLE_USER_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_BOMB_HEAD )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_THRILLER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RADIUSHEAL_ON_DAMAGE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_CARD_EFFECT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_INVULNERABLE_CARD_EFFECT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_UBER_BULLET_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_UBER_BLAST_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_UBER_FIRE_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_SMALL_BULLET_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_SMALL_BLAST_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_SMALL_FIRE_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_STEALTHED_USER_BUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MEDIGUN_DEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_STEALTHED_USER_BUFF_FADING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BULLET_IMMUNE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BLAST_IMMUNE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_FIRE_IMMUNE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PREVENT_DEATH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MVM_BOT_STUN_RADIOWAVE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_SPEED_BOOST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_QUICK_HEAL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_GIANT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_TINY )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_IN_HELL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_GHOST_MODE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MINICRITBOOSTED_ON_KILL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_OBSCURED_SMOKE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PARACHUTE_ACTIVE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BLASTJUMPING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_KART )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_KART_DASH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BALLOON_HEAD )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_MELEE_ONLY )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SWIMMING_CURSE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_FREEZE_INPUT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_KART_CAGE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_DONOTUSE_0 )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_STRENGTH )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_HASTE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_REGEN )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_RESIST )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_VAMPIRE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_REFLECT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_PRECISION )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_AGILITY )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLINGHOOK )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLINGHOOK_SAFEFALL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLINGHOOK_LATCHED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLINGHOOK_BLEEDING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_AFTERBURN_IMMUNE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_KNOCKOUT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_IMBALANCE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_CRITBOOSTED_RUNE_TEMP )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PASSTIME_INTERCEPTION )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_SWIMMING_NO_EFFECTS )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PURGATORY )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_KING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_PLAGUE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_RUNE_SUPERNOVA )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PLAGUE )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_KING_BUFFED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_TEAM_GLOWS )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_KNOCKED_INTO_AIR )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_COMPETITIVE_WINNER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_COMPETITIVE_LOSER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HEALING_DEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PASSTIME_PENALTY_DEBUFF )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLED_TO_PLAYER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GRAPPLED_BY_PLAYER )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_PARACHUTE_DEPLOYED )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_GAS )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_BURNING_PYRO )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_ROCKETPACK )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_LOST_FOOTING )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_AIR_CURRENT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_HALLOWEEN_HELL_HEAL )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_POWERUPMODE_DOMINANT )
DECLARE_SCRIPT_CONST( ETFCond, TF_COND_IMMUNE_TO_PUSHBACK )
REGISTER_SCRIPT_CONST_TABLE( ETFCond )

DECLARE_SCRIPT_CONST_TABLE( ECritType )
DECLARE_SCRIPT_CONST_NAMED( ECritType, "CRIT_NONE", CTakeDamageInfo::CRIT_NONE )
DECLARE_SCRIPT_CONST_NAMED( ECritType, "CRIT_MINI", CTakeDamageInfo::CRIT_MINI )
DECLARE_SCRIPT_CONST_NAMED( ECritType, "CRIT_FULL", CTakeDamageInfo::CRIT_FULL )
REGISTER_SCRIPT_CONST_TABLE( ECritType )

DECLARE_SCRIPT_CONST_TABLE( FTFBotAttributeType )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "REMOVE_ON_DEATH", CTFBot::AttributeType::REMOVE_ON_DEATH )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "AGGRESSIVE", CTFBot::AttributeType::AGGRESSIVE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "IS_NPC", CTFBot::AttributeType::IS_NPC )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "SUPPRESS_FIRE", CTFBot::AttributeType::SUPPRESS_FIRE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "DISABLE_DODGE", CTFBot::AttributeType::DISABLE_DODGE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "BECOME_SPECTATOR_ON_DEATH", CTFBot::AttributeType::BECOME_SPECTATOR_ON_DEATH )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "QUOTA_MANANGED", CTFBot::AttributeType::QUOTA_MANANGED )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "RETAIN_BUILDINGS", CTFBot::AttributeType::RETAIN_BUILDINGS )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "SPAWN_WITH_FULL_CHARGE", CTFBot::AttributeType::SPAWN_WITH_FULL_CHARGE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "ALWAYS_CRIT", CTFBot::AttributeType::ALWAYS_CRIT )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "IGNORE_ENEMIES", CTFBot::AttributeType::IGNORE_ENEMIES )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "HOLD_FIRE_UNTIL_FULL_RELOAD", CTFBot::AttributeType::HOLD_FIRE_UNTIL_FULL_RELOAD )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PRIORITIZE_DEFENSE", CTFBot::AttributeType::PRIORITIZE_DEFENSE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "ALWAYS_FIRE_WEAPON", CTFBot::AttributeType::ALWAYS_FIRE_WEAPON )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "TELEPORT_TO_HINT", CTFBot::AttributeType::TELEPORT_TO_HINT )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "MINIBOSS", CTFBot::AttributeType::MINIBOSS )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "USE_BOSS_HEALTH_BAR", CTFBot::AttributeType::USE_BOSS_HEALTH_BAR )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "IGNORE_FLAG", CTFBot::AttributeType::IGNORE_FLAG )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "AUTO_JUMP", CTFBot::AttributeType::AUTO_JUMP )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "AIR_CHARGE_ONLY", CTFBot::AttributeType::AIR_CHARGE_ONLY )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PREFER_VACCINATOR_BULLETS", CTFBot::AttributeType::PREFER_VACCINATOR_BULLETS )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PREFER_VACCINATOR_BLAST", CTFBot::AttributeType::PREFER_VACCINATOR_BLAST )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PREFER_VACCINATOR_FIRE", CTFBot::AttributeType::PREFER_VACCINATOR_FIRE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "BULLET_IMMUNE", CTFBot::AttributeType::BULLET_IMMUNE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "BLAST_IMMUNE", CTFBot::AttributeType::BLAST_IMMUNE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "FIRE_IMMUNE", CTFBot::AttributeType::FIRE_IMMUNE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PARACHUTE", CTFBot::AttributeType::PARACHUTE )
DECLARE_SCRIPT_CONST_NAMED( FTFBotAttributeType, "PROJECTILE_SHIELD", CTFBot::AttributeType::PROJECTILE_SHIELD )
REGISTER_SCRIPT_CONST_TABLE( FTFBotAttributeType )

DECLARE_SCRIPT_CONST_TABLE( ETFBotDifficultyType )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "UNDEFINED", CTFBot::DifficultyType::UNDEFINED )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "EASY", CTFBot::DifficultyType::EASY )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "NORMAL", CTFBot::DifficultyType::NORMAL )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "HARD", CTFBot::DifficultyType::HARD )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "EXPERT", CTFBot::DifficultyType::EXPERT )
DECLARE_SCRIPT_CONST_NAMED( ETFBotDifficultyType, "NUM_DIFFICULTY_LEVELS", CTFBot::DifficultyType::NUM_DIFFICULTY_LEVELS )
REGISTER_SCRIPT_CONST_TABLE( ETFBotDifficultyType )

DECLARE_SCRIPT_CONST_TABLE( ENavDirType )
DECLARE_SCRIPT_CONST( ENavDirType, NORTH )
DECLARE_SCRIPT_CONST( ENavDirType, EAST )
DECLARE_SCRIPT_CONST( ENavDirType, SOUTH )
DECLARE_SCRIPT_CONST( ENavDirType, WEST )
DECLARE_SCRIPT_CONST( ENavDirType, NUM_DIRECTIONS )
REGISTER_SCRIPT_CONST_TABLE( ENavDirType )

DECLARE_SCRIPT_CONST_TABLE( ENavTraverseType )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_NORTH )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_EAST )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_SOUTH )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_WEST ) // life is peaceful there
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_LADDER_UP )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_LADDER_DOWN )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_JUMP )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_ELEVATOR_UP )
DECLARE_SCRIPT_CONST( ENavTraverseType, GO_ELEVATOR_DOWN )
DECLARE_SCRIPT_CONST( ENavTraverseType, NUM_TRAVERSE_TYPES )
REGISTER_SCRIPT_CONST_TABLE( ENavTraverseType )

DECLARE_SCRIPT_CONST_TABLE( ENavCornerType )
DECLARE_SCRIPT_CONST( ENavCornerType, NORTH_WEST )
DECLARE_SCRIPT_CONST( ENavCornerType, NORTH_EAST )
DECLARE_SCRIPT_CONST( ENavCornerType, SOUTH_EAST )
DECLARE_SCRIPT_CONST( ENavCornerType, SOUTH_WEST )
DECLARE_SCRIPT_CONST( ENavCornerType, NUM_CORNERS )
REGISTER_SCRIPT_CONST_TABLE( ENavCornerType )

DECLARE_SCRIPT_CONST_TABLE( ENavRelativeDirType )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, FORWARD )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, RIGHT )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, BACKWARD )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, LEFT )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, UP )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, DOWN )
DECLARE_SCRIPT_CONST( ENavRelativeDirType, NUM_RELATIVE_DIRECTIONS )
REGISTER_SCRIPT_CONST_TABLE( ENavRelativeDirType )

DECLARE_SCRIPT_CONST_TABLE( FNavAttributeType )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_INVALID )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_CROUCH )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_JUMP )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_PRECISE )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_NO_JUMP )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_STOP )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_RUN )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_WALK )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_AVOID )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_TRANSIENT )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_DONT_HIDE )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_STAND )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_NO_HOSTAGES )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_STAIRS )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_NO_MERGE )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_OBSTACLE_TOP )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_CLIFF )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_FIRST_CUSTOM )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_LAST_CUSTOM )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_FUNC_COST )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_HAS_ELEVATOR )
DECLARE_SCRIPT_CONST( FNavAttributeType, NAV_MESH_NAV_BLOCKER )
REGISTER_SCRIPT_CONST_TABLE( FNavAttributeType )

DECLARE_SCRIPT_CONST_TABLE( FButtons )
DECLARE_SCRIPT_CONST( FButtons, IN_ATTACK )
DECLARE_SCRIPT_CONST( FButtons, IN_JUMP )
DECLARE_SCRIPT_CONST( FButtons, IN_DUCK )
DECLARE_SCRIPT_CONST( FButtons, IN_FORWARD )
DECLARE_SCRIPT_CONST( FButtons, IN_BACK )
DECLARE_SCRIPT_CONST( FButtons, IN_USE )
DECLARE_SCRIPT_CONST( FButtons, IN_CANCEL )
DECLARE_SCRIPT_CONST( FButtons, IN_LEFT )
DECLARE_SCRIPT_CONST( FButtons, IN_RIGHT )
DECLARE_SCRIPT_CONST( FButtons, IN_MOVELEFT )
DECLARE_SCRIPT_CONST( FButtons, IN_MOVERIGHT )
DECLARE_SCRIPT_CONST( FButtons, IN_ATTACK2 )
DECLARE_SCRIPT_CONST( FButtons, IN_RUN )
DECLARE_SCRIPT_CONST( FButtons, IN_RELOAD )
DECLARE_SCRIPT_CONST( FButtons, IN_ALT1 )
DECLARE_SCRIPT_CONST( FButtons, IN_ALT2 )
DECLARE_SCRIPT_CONST( FButtons, IN_SCORE )
DECLARE_SCRIPT_CONST( FButtons, IN_SPEED )
DECLARE_SCRIPT_CONST( FButtons, IN_WALK )
DECLARE_SCRIPT_CONST( FButtons, IN_ZOOM )
DECLARE_SCRIPT_CONST( FButtons, IN_WEAPON1 )
DECLARE_SCRIPT_CONST( FButtons, IN_WEAPON2 )
DECLARE_SCRIPT_CONST( FButtons, IN_BULLRUSH )
DECLARE_SCRIPT_CONST( FButtons, IN_GRENADE1 )
DECLARE_SCRIPT_CONST( FButtons, IN_GRENADE2 )
DECLARE_SCRIPT_CONST( FButtons, IN_ATTACK3 )
REGISTER_SCRIPT_CONST_TABLE( FButtons )

DECLARE_SCRIPT_CONST_TABLE( FHideHUD )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_WEAPONSELECTION )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_FLASHLIGHT )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_ALL )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_HEALTH )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_PLAYERDEAD )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_NEEDSUIT )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_MISCSTATUS )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_CHAT )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_CROSSHAIR )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_VEHICLE_CROSSHAIR )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_INVEHICLE )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_BONUS_PROGRESS )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_BUILDING_STATUS )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_CLOAK_AND_FEIGN )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_PIPES_AND_CHARGE )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_METAL )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_TARGET_ID )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_MATCH_STATUS )
DECLARE_SCRIPT_CONST( FHideHUD, HIDEHUD_BITCOUNT )
REGISTER_SCRIPT_CONST_TABLE( FHideHUD )

DECLARE_SCRIPT_CONST_TABLE( FTaunts )
DECLARE_SCRIPT_CONST( FTaunts, TAUNT_BASE_WEAPON )
DECLARE_SCRIPT_CONST( FTaunts, TAUNT_MISC_ITEM )
DECLARE_SCRIPT_CONST( FTaunts, TAUNT_SHOW_ITEM )
DECLARE_SCRIPT_CONST( FTaunts, TAUNT_LONG )
DECLARE_SCRIPT_CONST( FTaunts, TAUNT_SPECIAL )
REGISTER_SCRIPT_CONST_TABLE( FTaunts )

DECLARE_SCRIPT_CONST_TABLE( EHitGroup )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_GENERIC )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_HEAD )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_CHEST )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_STOMACH )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_LEFTARM )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_RIGHTARM )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_LEFTLEG )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_RIGHTLEG )
DECLARE_SCRIPT_CONST( EHitGroup, HITGROUP_GEAR )
REGISTER_SCRIPT_CONST_TABLE( EHitGroup )

DECLARE_SCRIPT_CONST_TABLE( FDmgType )
DECLARE_SCRIPT_CONST( FDmgType, DMG_GENERIC )
DECLARE_SCRIPT_CONST( FDmgType, DMG_CRUSH )
DECLARE_SCRIPT_CONST( FDmgType, DMG_CLUB )
DECLARE_SCRIPT_CONST( FDmgType, DMG_BULLET )
DECLARE_SCRIPT_CONST( FDmgType, DMG_SLASH )
DECLARE_SCRIPT_CONST( FDmgType, DMG_BURN )
DECLARE_SCRIPT_CONST( FDmgType, DMG_VEHICLE )
DECLARE_SCRIPT_CONST( FDmgType, DMG_FALL )
DECLARE_SCRIPT_CONST( FDmgType, DMG_BLAST )
DECLARE_SCRIPT_CONST( FDmgType, DMG_CLUB )
DECLARE_SCRIPT_CONST( FDmgType, DMG_SHOCK )
DECLARE_SCRIPT_CONST( FDmgType, DMG_SONIC )
DECLARE_SCRIPT_CONST( FDmgType, DMG_ENERGYBEAM )
DECLARE_SCRIPT_CONST( FDmgType, DMG_PREVENT_PHYSICS_FORCE )
DECLARE_SCRIPT_CONST( FDmgType, DMG_NEVERGIB )
DECLARE_SCRIPT_CONST( FDmgType, DMG_ALWAYSGIB )
DECLARE_SCRIPT_CONST( FDmgType, DMG_DROWN )
DECLARE_SCRIPT_CONST( FDmgType, DMG_PARALYZE )
DECLARE_SCRIPT_CONST( FDmgType, DMG_NERVEGAS )
DECLARE_SCRIPT_CONST( FDmgType, DMG_POISON )
DECLARE_SCRIPT_CONST( FDmgType, DMG_RADIATION )
DECLARE_SCRIPT_CONST( FDmgType, DMG_DROWNRECOVER )
DECLARE_SCRIPT_CONST( FDmgType, DMG_ACID )
DECLARE_SCRIPT_CONST( FDmgType, DMG_SLOWBURN )
DECLARE_SCRIPT_CONST( FDmgType, DMG_REMOVENORAGDOLL )
DECLARE_SCRIPT_CONST( FDmgType, DMG_PHYSGUN )
DECLARE_SCRIPT_CONST( FDmgType, DMG_PLASMA )
DECLARE_SCRIPT_CONST( FDmgType, DMG_AIRBOAT )
DECLARE_SCRIPT_CONST( FDmgType, DMG_DISSOLVE )
DECLARE_SCRIPT_CONST( FDmgType, DMG_BLAST_SURFACE )
DECLARE_SCRIPT_CONST( FDmgType, DMG_DIRECT )
DECLARE_SCRIPT_CONST( FDmgType, DMG_BUCKSHOT )
REGISTER_SCRIPT_CONST_TABLE( FDmgType )

DECLARE_SCRIPT_CONST_TABLE( ESpectatorMode )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_NONE )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_DEATHCAM )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_FREEZECAM )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_FIXED )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_IN_EYE )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_CHASE )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_POI )
DECLARE_SCRIPT_CONST( ESpectatorMode, OBS_MODE_ROAMING )
DECLARE_SCRIPT_CONST( ESpectatorMode, NUM_OBSERVER_MODES )
REGISTER_SCRIPT_CONST_TABLE( ESpectatorMode )

DECLARE_SCRIPT_CONST_TABLE( FEntityEFlags )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_KILLME )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DORMANT )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NOCLIP_ACTIVE )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_SETTING_UP_BONES )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_KEEP_ON_RECREATE_ENTITIES )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_HAS_PLAYER_CHILD )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_SHADOWUPDATE )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NOTIFY )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_FORCE_CHECK_TRANSMIT )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_BOT_FROZEN )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_SERVER_ONLY )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_AUTO_EDICT_ATTACH )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_ABSTRANSFORM )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_ABSVELOCITY )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_ABSANGVELOCITY )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DIRTY_SPATIAL_PARTITION )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_FORCE_ALLOW_MOVEPARENT )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_IN_SKYBOX )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_USE_PARTITION_WHEN_NOT_SOLID )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_TOUCHING_FLUID )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_IS_BEING_LIFTED_BY_BARNACLE )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_ROTORWASH_PUSH )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_THINK_FUNCTION )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_GAME_PHYSICS_SIMULATION )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_CHECK_UNTOUCH )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DONTBLOCKLOS )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_DONTWALKON )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_DISSOLVE )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_MEGAPHYSCANNON_RAGDOLL )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_WATER_VELOCITY_CHANGE )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_PHYSCANNON_INTERACTION )
DECLARE_SCRIPT_CONST( FEntityEFlags, EFL_NO_DAMAGE_FORCES )
REGISTER_SCRIPT_CONST_TABLE( FEntityEFlags )

DECLARE_SCRIPT_CONST_TABLE( FSolid )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_CUSTOMRAYTEST )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_CUSTOMBOXTEST )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_NOT_SOLID )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_TRIGGER )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_NOT_STANDABLE )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_VOLUME_CONTENTS )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_FORCE_WORLD_ALIGNED )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_USE_TRIGGER_BOUNDS )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_ROOT_PARENT_ALIGNED )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_TRIGGER_TOUCH_DEBRIS )
DECLARE_SCRIPT_CONST( FSolid, FSOLID_MAX_BITS )
REGISTER_SCRIPT_CONST_TABLE( FSolid )

DECLARE_SCRIPT_CONST_TABLE( ERenderMode )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderNormal )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransColor )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransTexture )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderGlow )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransAlpha )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransAdd )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderEnvironmental )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransAddFrameBlend )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderTransAlphaAdd )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderWorldGlow )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderNone )
DECLARE_SCRIPT_CONST( ERenderMode, kRenderModeCount )
REGISTER_SCRIPT_CONST_TABLE( ERenderMode )

DECLARE_SCRIPT_CONST_TABLE( ERenderFx )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxNone )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxPulseSlow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxPulseFast )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxPulseSlowWide )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxPulseFastWide )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxFadeSlow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxFadeFast )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxSolidSlow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxSolidFast )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxStrobeSlow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxStrobeFast )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxStrobeFaster )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxFlickerSlow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxFlickerFast )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxNoDissipation )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxDistort )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxHologram )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxExplode )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxGlowShell )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxClampMinScale )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxEnvRain )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxEnvSnow )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxSpotlight )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxRagdoll )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxPulseFastWider )
DECLARE_SCRIPT_CONST( ERenderFx, kRenderFxMax )
REGISTER_SCRIPT_CONST_TABLE( ERenderFx )

DECLARE_SCRIPT_CONST_TABLE( ECollisionGroup )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_NONE )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_DEBRIS )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_DEBRIS_TRIGGER )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_INTERACTIVE_DEBRIS )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_INTERACTIVE )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_PLAYER )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_BREAKABLE_GLASS )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_VEHICLE )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_PLAYER_MOVEMENT )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_NPC )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_IN_VEHICLE )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_WEAPON )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_VEHICLE_CLIP )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_PROJECTILE )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_DOOR_BLOCKER )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_PASSABLE_DOOR )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_DISSOLVING )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_PUSHAWAY )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_NPC_ACTOR )
DECLARE_SCRIPT_CONST( ECollisionGroup, COLLISION_GROUP_NPC_SCRIPTED )
DECLARE_SCRIPT_CONST( ECollisionGroup, LAST_SHARED_COLLISION_GROUP )
REGISTER_SCRIPT_CONST_TABLE( ECollisionGroup )

DECLARE_SCRIPT_CONST_TABLE( FPlayer )
DECLARE_SCRIPT_CONST( FPlayer, FL_ONGROUND )
DECLARE_SCRIPT_CONST( FPlayer, FL_DUCKING )
DECLARE_SCRIPT_CONST( FPlayer, FL_ANIMDUCKING )
DECLARE_SCRIPT_CONST( FPlayer, FL_WATERJUMP )
DECLARE_SCRIPT_CONST( FPlayer, FL_ONTRAIN )
DECLARE_SCRIPT_CONST( FPlayer, FL_INRAIN )
DECLARE_SCRIPT_CONST( FPlayer, FL_FROZEN )
DECLARE_SCRIPT_CONST( FPlayer, FL_ATCONTROLS )
DECLARE_SCRIPT_CONST( FPlayer, FL_CLIENT )
DECLARE_SCRIPT_CONST( FPlayer, FL_FAKECLIENT )
DECLARE_SCRIPT_CONST( FPlayer, FL_INWATER )
DECLARE_SCRIPT_CONST( FPlayer, FL_FLY )
DECLARE_SCRIPT_CONST( FPlayer, FL_SWIM )
DECLARE_SCRIPT_CONST( FPlayer, FL_CONVEYOR )
DECLARE_SCRIPT_CONST( FPlayer, FL_NPC )
DECLARE_SCRIPT_CONST( FPlayer, FL_GODMODE )
DECLARE_SCRIPT_CONST( FPlayer, FL_NOTARGET )
DECLARE_SCRIPT_CONST( FPlayer, FL_AIMTARGET )
DECLARE_SCRIPT_CONST( FPlayer, FL_PARTIALGROUND )
DECLARE_SCRIPT_CONST( FPlayer, FL_STATICPROP )
DECLARE_SCRIPT_CONST( FPlayer, FL_GRAPHED )
DECLARE_SCRIPT_CONST( FPlayer, FL_GRENADE )
DECLARE_SCRIPT_CONST( FPlayer, FL_STEPMOVEMENT )
DECLARE_SCRIPT_CONST( FPlayer, FL_DONTTOUCH )
DECLARE_SCRIPT_CONST( FPlayer, FL_BASEVELOCITY )
DECLARE_SCRIPT_CONST( FPlayer, FL_WORLDBRUSH )
DECLARE_SCRIPT_CONST( FPlayer, FL_OBJECT )
DECLARE_SCRIPT_CONST( FPlayer, FL_KILLME )
DECLARE_SCRIPT_CONST( FPlayer, FL_ONFIRE )
DECLARE_SCRIPT_CONST( FPlayer, FL_DISSOLVING )
DECLARE_SCRIPT_CONST( FPlayer, FL_TRANSRAGDOLL )
DECLARE_SCRIPT_CONST( FPlayer, FL_UNBLOCKABLE_BY_PLAYER )
DECLARE_SCRIPT_CONST( FPlayer, PLAYER_FLAG_BITS )
REGISTER_SCRIPT_CONST_TABLE( FPlayer )

DECLARE_SCRIPT_CONST_TABLE( FEntityEffects )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_BONEMERGE )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_BRIGHTLIGHT )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_DIMLIGHT )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_NOINTERP )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_NOSHADOW )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_NODRAW )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_NORECEIVESHADOW )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_BONEMERGE_FASTCULL )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_ITEM_BLINK )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_PARENT_ANIMATES )
DECLARE_SCRIPT_CONST( FEntityEffects, EF_MAX_BITS )
REGISTER_SCRIPT_CONST_TABLE( FEntityEffects )

DECLARE_SCRIPT_CONST_TABLE( ESolidType )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_NONE )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_BSP )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_BBOX )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_OBB )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_OBB_YAW )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_CUSTOM )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_VPHYSICS )
DECLARE_SCRIPT_CONST( ESolidType, SOLID_LAST )
REGISTER_SCRIPT_CONST_TABLE( ESolidType )

DECLARE_SCRIPT_CONST_TABLE( FContents )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_EMPTY )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_SOLID )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_WINDOW )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_AUX )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_GRATE )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_SLIME )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_WATER )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_BLOCKLOS )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_OPAQUE )
DECLARE_SCRIPT_CONST( FContents, LAST_VISIBLE_CONTENTS )
DECLARE_SCRIPT_CONST( FContents, ALL_VISIBLE_CONTENTS )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_TESTFOGVOLUME )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_UNUSED )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_UNUSED6 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_TEAM1 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_TEAM2 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_IGNORE_NODRAW_OPAQUE )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_MOVEABLE )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_AREAPORTAL )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_PLAYERCLIP )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_MONSTERCLIP )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_0 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_90 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_180 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_270 )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_UP )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_CURRENT_DOWN )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_ORIGIN )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_MONSTER )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_DEBRIS )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_DETAIL )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_TRANSLUCENT )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_LADDER )
DECLARE_SCRIPT_CONST( FContents, CONTENTS_HITBOX )
REGISTER_SCRIPT_CONST_TABLE( FContents )

DECLARE_SCRIPT_CONST_TABLE( FSurf )
DECLARE_SCRIPT_CONST( FSurf, SURF_LIGHT )
DECLARE_SCRIPT_CONST( FSurf, SURF_SKY2D )
DECLARE_SCRIPT_CONST( FSurf, SURF_SKY )
DECLARE_SCRIPT_CONST( FSurf, SURF_WARP )
DECLARE_SCRIPT_CONST( FSurf, SURF_TRANS )
DECLARE_SCRIPT_CONST( FSurf, SURF_NOPORTAL )
DECLARE_SCRIPT_CONST( FSurf, SURF_TRIGGER )
DECLARE_SCRIPT_CONST( FSurf, SURF_NODRAW )
DECLARE_SCRIPT_CONST( FSurf, SURF_HINT )
DECLARE_SCRIPT_CONST( FSurf, SURF_SKIP )
DECLARE_SCRIPT_CONST( FSurf, SURF_NOLIGHT )
DECLARE_SCRIPT_CONST( FSurf, SURF_BUMPLIGHT )
DECLARE_SCRIPT_CONST( FSurf, SURF_NOSHADOWS )
DECLARE_SCRIPT_CONST( FSurf, SURF_NODECALS )
DECLARE_SCRIPT_CONST( FSurf, SURF_NOCHOP )
DECLARE_SCRIPT_CONST( FSurf, SURF_HITBOX )
REGISTER_SCRIPT_CONST_TABLE( FSurf )

// MoveType_t
DECLARE_SCRIPT_CONST_TABLE( EMoveType )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_NONE )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_ISOMETRIC )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_WALK )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_STEP )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_FLY )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_FLYGRAVITY )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_VPHYSICS )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_PUSH )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_NOCLIP )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_LADDER )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_OBSERVER )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_CUSTOM )
DECLARE_SCRIPT_CONST( EMoveType, MOVETYPE_LAST )
REGISTER_SCRIPT_CONST_TABLE( EMoveType )

DECLARE_SCRIPT_CONST_TABLE( EMoveCollide )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_DEFAULT )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_FLY_BOUNCE )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_FLY_CUSTOM )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_FLY_SLIDE )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_COUNT )
DECLARE_SCRIPT_CONST( EMoveCollide, MOVECOLLIDE_MAX_BITS )
REGISTER_SCRIPT_CONST_TABLE( EMoveCollide )

// Unnamed enum
DECLARE_SCRIPT_CONST_TABLE( ETFTeam )
DECLARE_SCRIPT_CONST( ETFTeam, TEAM_ANY )
DECLARE_SCRIPT_CONST( ETFTeam, TEAM_INVALID )
DECLARE_SCRIPT_CONST( ETFTeam, TEAM_UNASSIGNED )
DECLARE_SCRIPT_CONST( ETFTeam, TEAM_SPECTATOR )
DECLARE_SCRIPT_CONST( ETFTeam, TEAM_SPECTATOR )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_RED )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_BLUE )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_COUNT )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_PVE_INVADERS )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_PVE_DEFENDERS )
DECLARE_SCRIPT_CONST( ETFTeam, TF_TEAM_PVE_INVADERS_GIANTS )
REGISTER_SCRIPT_CONST_TABLE( ETFTeam )

// ETFDmgCustom
DECLARE_SCRIPT_CONST_TABLE( ETFDmgCustom )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_NONE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_HEADSHOT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BACKSTAB )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BURNING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_WRENCH_FIX )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_MINIGUN )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SUICIDE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_HADOUKEN )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BURNING_FLARE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_GRAND_SLAM )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PENETRATE_MY_TEAM )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PENETRATE_ALL_PLAYERS )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_FENCING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PENETRATE_NONBURNING_TEAMMATE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_ARROW_STAB )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TELEFRAG )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BURNING_ARROW )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_FLYINGBURN )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PUMPKIN_BOMB )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_DECAPITATION )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_GRENADE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BASEBALL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CHARGE_IMPACT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_BARBARIAN_SWING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_AIR_STICKY_BURST )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_DEFENSIVE_STICKY )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PICKAXE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_ROCKET_DIRECTHIT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_UBERSLICE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PLAYER_SENTRY )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_STANDARD_STICKY )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SHOTGUN_REVENGE_CRIT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_GUITAR_SMASH )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BLEEDING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_GOLD_WRENCH )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CARRIED_BUILDING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_COMBO_PUNCH )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_ENGINEER_ARM_KILL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_FISH_KILL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TRIGGER_HURT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_DECAPITATION_BOSS )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_STICKBOMB_EXPLOSION )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_AEGIS_ROUND )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_FLARE_EXPLOSION )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_BOOTS_STOMP )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PLASMA )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PLASMA_CHARGED )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PLASMA_GIB )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_PRACTICE_STICKY )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_EYEBALL_ROCKET )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_HEADSHOT_DECAPITATION )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_ARMAGEDDON )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_FLARE_PELLET )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CLEAVER )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CLEAVER_CRIT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_MERASMUS_PLAYER_BOMB )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_MERASMUS_GRENADE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_MERASMUS_ZAP )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_MERASMUS_DECAPITATION )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CANNONBALL_PUSH )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_ALLCLASS_GUITAR_RIFF )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_THROWABLE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_THROWABLE_KILL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_TELEPORT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_SKELETON )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_MIRV )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_METEOR )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_LIGHTNING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_FIREBALL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_MONOCULUS )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_BLASTJUMP )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_BATS )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SPELL_TINY )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_KART )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_GIANT_HAMMER )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_RUNE_REFLECT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_DRAGONS_FURY_IGNITE )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_DRAGONS_FURY_BONUS_BURNING )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_SLAP_KILL )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_CROC )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_GASBLAST )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_AXTINGUISHER_BOOSTED )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_TAUNTATK_TRICKSHOT )
DECLARE_SCRIPT_CONST( ETFDmgCustom, TF_DMG_CUSTOM_END )
REGISTER_SCRIPT_CONST_TABLE( ETFDmgCustom )

// ETFClass
DECLARE_SCRIPT_CONST_TABLE( ETFClass )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_UNDEFINED )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_SCOUT )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_SNIPER )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_SOLDIER )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_DEMOMAN )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_MEDIC )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_HEAVYWEAPONS )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_PYRO )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_SPY )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_ENGINEER )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_CIVILIAN )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_COUNT_ALL )
DECLARE_SCRIPT_CONST( ETFClass, TF_CLASS_RANDOM )
REGISTER_SCRIPT_CONST_TABLE( ETFClass )

// gamerules_roundstate_t
DECLARE_SCRIPT_CONST_TABLE( ERoundState )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_INIT )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_PREGAME )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_STARTGAME )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_PREROUND )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_RND_RUNNING )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_TEAM_WIN )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_RESTART )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_STALEMATE )
DECLARE_SCRIPT_CONST( ERoundState, GR_STATE_GAME_OVER )
DECLARE_SCRIPT_CONST( ERoundState, GR_NUM_ROUND_STATES )
REGISTER_SCRIPT_CONST_TABLE( ERoundState )

// Unnamed enum
DECLARE_SCRIPT_CONST_TABLE( EStopwatchState )
DECLARE_SCRIPT_CONST( EStopwatchState, STOPWATCH_CAPTURE_TIME_NOT_SET )
DECLARE_SCRIPT_CONST( EStopwatchState, STOPWATCH_RUNNING )
DECLARE_SCRIPT_CONST( EStopwatchState, STOPWATCH_OVERTIME )
REGISTER_SCRIPT_CONST_TABLE( EStopwatchState )

// EHoliday
DECLARE_SCRIPT_CONST_TABLE( EHoliday )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_None )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_TFBirthday )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_Halloween )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_Christmas )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_CommunityUpdate )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_EOTL )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_Valentines )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_MeetThePyro )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_FullMoon )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_HalloweenOrFullMoon )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_HalloweenOrFullMoonOrValentines )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_AprilFools )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_Soldier )
DECLARE_SCRIPT_CONST( EHoliday, kHoliday_Summer )
DECLARE_SCRIPT_CONST( EHoliday, kHolidayCount )
REGISTER_SCRIPT_CONST_TABLE( EHoliday )

// TFNavAttributeType
DECLARE_SCRIPT_CONST_TABLE( FTFNavAttributeType )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_INVALID )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLOCKED )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_SPAWN_ROOM_RED )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_SPAWN_ROOM_BLUE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_SPAWN_ROOM_EXIT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_HAS_AMMO )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_HAS_HEALTH )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_CONTROL_POINT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLUE_SENTRY_DANGER )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_RED_SENTRY_DANGER )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLUE_SETUP_GATE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_RED_SETUP_GATE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLOCKED_AFTER_POINT_CAPTURE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLOCKED_UNTIL_POINT_CAPTURE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BLUE_ONE_WAY_DOOR )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_RED_ONE_WAY_DOOR )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_WITH_SECOND_POINT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_WITH_THIRD_POINT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_WITH_FOURTH_POINT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_WITH_FIFTH_POINT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_SNIPER_SPOT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_SENTRY_SPOT )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_ESCAPE_ROUTE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_ESCAPE_ROUTE_VISIBLE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_NO_SPAWNING )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_RESCUE_CLOSET )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_BOMB_CAN_DROP_HERE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_DOOR_NEVER_BLOCKS )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_DOOR_ALWAYS_BLOCKS )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_UNBLOCKABLE )
DECLARE_SCRIPT_CONST( FTFNavAttributeType, TF_NAV_PERSISTENT_ATTRIBUTES )
REGISTER_SCRIPT_CONST_TABLE( FTFNavAttributeType )

DECLARE_SCRIPT_CONST_TABLE( EHudNotify )
DECLARE_SCRIPT_CONST( EHudNotify, HUD_PRINTNOTIFY )
DECLARE_SCRIPT_CONST( EHudNotify, HUD_PRINTCONSOLE )
DECLARE_SCRIPT_CONST( EHudNotify, HUD_PRINTTALK )
DECLARE_SCRIPT_CONST( EHudNotify, HUD_PRINTCENTER )
REGISTER_SCRIPT_CONST_TABLE( EHudNotify )

DECLARE_SCRIPT_CONST_TABLE( EBotType )
DECLARE_SCRIPT_CONST( EBotType, TF_BOT_TYPE )
REGISTER_SCRIPT_CONST_TABLE( EBotType )

DECLARE_SCRIPT_CONST_TABLE( Math )
DECLARE_SCRIPT_CONST_NAMED( Math, "Epsilon", FLT_EPSILON)
DECLARE_SCRIPT_CONST_NAMED( Math, "Zero", 0.0f)
DECLARE_SCRIPT_CONST_NAMED( Math, "One", 1.0f)
DECLARE_SCRIPT_CONST_NAMED( Math, "E", 2.718281828f)
DECLARE_SCRIPT_CONST_NAMED( Math, "Pi", 3.141592654f)
DECLARE_SCRIPT_CONST_NAMED( Math, "Tau", 6.283185307f )
DECLARE_SCRIPT_CONST_NAMED( Math, "Sqrt2", 1.414213562f)
DECLARE_SCRIPT_CONST_NAMED( Math, "Sqrt3", 1.732050808f)
DECLARE_SCRIPT_CONST_NAMED( Math, "GoldenRatio", 1.618033989f)
REGISTER_SCRIPT_CONST_TABLE( Math )

DECLARE_SCRIPT_CONST_TABLE( Server )
DECLARE_SCRIPT_CONST( Server, MAX_EDICTS )
DECLARE_SCRIPT_CONST( Server, MAX_PLAYERS )
DECLARE_SCRIPT_CONST( Server, DIST_EPSILON )
DECLARE_SCRIPT_CONST_NAMED( Server, "ConstantNamingConvention", "Constants are named as follows: F -> flags, E -> enums, (nothing) -> random values/constants" )
REGISTER_SCRIPT_CONST_TABLE( Server )
#endif
				g_pScriptVM->SetValue( "Constants", vConstantsTable );

				if ( scriptLanguage == SL_SQUIRREL )
				{
					g_pScriptVM->Run( g_Script_vscript_server );
				}
				g_VScriptGameEventListener.Init();

				VScriptRunScript( "mapspawn", false );

				if ( script_connect_debugger_on_mapspawn.GetBool() )
				{
					g_pScriptVM->ConnectDebugger();
				}

				VMPROF_SHOW( pszScriptLanguage, "virtual machine startup" );

				return true;
			}
			else
			{
				DevWarning("VM Did not start!\n");
			}
		}
	}
	else
	{
		Log_Msg( LOG_VScript, "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptServerTerm()
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


bool VScriptServerReplaceClosures( const char *pszScriptName, HSCRIPT hScope, bool bWarnMissing )
{
	if ( !g_pScriptVM || !pszScriptName )
	{
		return false;
	}

	HSCRIPT hReplaceClosuresFunc = g_pScriptVM->LookupFunction( "__ReplaceClosures" );
	if ( !hReplaceClosuresFunc )
	{
		return false;
	}
	HSCRIPT hNewScript =  VScriptCompileScript( pszScriptName, bWarnMissing );
	if ( !hNewScript )
	{
		g_pScriptVM->ReleaseFunction( hReplaceClosuresFunc );
		return false;
	}

	g_pScriptVM->Call( hReplaceClosuresFunc, NULL, true, NULL, hNewScript, hScope );
	g_pScriptVM->ReleaseFunction( hReplaceClosuresFunc );
	g_pScriptVM->ReleaseScript( hNewScript );
	return true;
}

bool g_bVscriptGameDebugEnabled;

void VScriptServerToggleGameDebug()
{
	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	g_bVscriptGameDebugEnabled = !g_bVscriptGameDebugEnabled;
	Msg( "script_debug %s\n", (g_bVscriptGameDebugEnabled) ? "ON" : "OFF" );
	const char *pszFunction = ( g_bVscriptGameDebugEnabled ) ? "BeginScriptDebug" : "EndScriptDebug";
	HSCRIPT hFunction = g_pScriptVM->LookupFunction( pszFunction );
	if ( !hFunction )
		return;
	g_pScriptVM->ExecuteFunction( hFunction, NULL, 0, NULL, NULL, true );
	g_pScriptVM->ReleaseFunction( hFunction );
}

CON_COMMAND_F( script_attach_debugger, "Connect the vscript VM to the script debugger", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND_F( script_debug, "Toggle the in-game script debug features", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( tolower(*args[1]) == 'w'/*atch*/ )
	{
		g_pScriptVM->Run( "ScriptDebugDrawWatchesEnabled = !ScriptDebugDrawWatchesEnabled" );
		return;
	}
	else if ( tolower(*args[1]) == 'm'/*essages*/ )
	{
		g_pScriptVM->Run( "ScriptDebugDrawTextEnabled = !ScriptDebugDrawTextEnabled" );
		return;
	}

	VScriptServerToggleGameDebug();
}

CON_COMMAND_F( script_add_watch, "Add a watch to the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugAddWatch( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_remove_watch, "Remove a watch from the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugRemoveWatch( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_add_watch_pattern, "Add a watch to the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugAddWatchPattern( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_remove_watch_pattern, "Remove a watch from the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugRemoveWatchPattern( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_add_debug_filter, "Add a filter to the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugAddTextFilter( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_remove_debug_filter, "Remove a filter from the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugRemoveTextFilter( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_trace_enable, "Turn on a particular trace output by file or function name", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	if ( !*args[1] )
	{
		Msg( "No key specified\n");
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugAddTrace( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_trace_disable, "Turn off a particular trace output by file or function name", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	if ( !*args[1] )
	{
		Msg( "No key specified\n");
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugRemoveTrace( \"%s\" )", pszScript ) );
}

CON_COMMAND_F( script_trace_enable_key, "Turn on a particular trace output by table/instance", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	if ( !*args[1] )
	{
		Msg( "No key specified\n");
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugAddTrace( %s )", pszScript ) );
}

CON_COMMAND_F( script_trace_disable_key, "Turn off a particular trace output by table/instance", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	if ( !*args[1] )
	{
		Msg( "No key specified\n");
		return;
	}

	const char *pszScript = args[1];
	g_pScriptVM->Run( CFmtStr( "ScriptDebugRemoveTrace( %s )", pszScript ) );
}

CON_COMMAND_F( script_trace_enable_all, "Turn on all trace output", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	g_pScriptVM->Run( "ScriptDebugTraceAll( true )" );
}

CON_COMMAND_F( script_trace_disable_all, "Turn off all trace output", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	g_pScriptVM->Run( "ScriptDebugTraceAll( false )" );
}

CON_COMMAND_F( script_clear_watches, "Clear all watches from the game debug overlay", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	g_pScriptVM->Run( "ScriptDebugClearWatches()" );
}

CON_COMMAND_F( script_find, "Find a key in the VM ", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( !g_pScriptVM )
	{
		Msg( "Scripting disabled or no server running\n" );
		return;
	}

	if ( !*args[1] )
	{
		Msg( "No key specified\n");
		return;
	}

	const char *pszFunction = "ScriptDebugDumpKeys";
	HSCRIPT hFunction = g_pScriptVM->LookupFunction( pszFunction );
	if ( !hFunction )
		return;
	g_pScriptVM->Call( hFunction, NULL, true, NULL, args[1] );
	g_pScriptVM->ReleaseFunction( hFunction );
}

CON_COMMAND( script_reload_code, "Execute a vscript file, replacing existing functions with the functions in the run script" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

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

	VScriptServerReplaceClosures( args[1], NULL, true );
}

CON_COMMAND( script_reload_entity_code, "Execute all of this entity's VScripts, replacing existing functions with the functions in the run scripts" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszVScripts != NULL_STRING )
		{
			char szScriptsList[255];
			V_strcpy_safe( szScriptsList, STRING(pEntity->m_iszVScripts) );
			CUtlStringList szScripts;
			V_SplitString( szScriptsList, " ", szScripts);

			for( int i = 0 ; i < szScripts.Count() ; i++ )
			{
				VScriptServerReplaceClosures( szScripts[i], pEntity->m_ScriptScope, true );
			}
		}
	}
}

CON_COMMAND( script_reload_think, "Execute an activation script, replacing existing functions with the functions in the run script" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

	const char *pszTarget = "";
	if ( *args[1] )
	{
		pszTarget = args[1];
	}

	if ( !g_pScriptVM )
	{
		Log_Warning( LOG_VScript, "Scripting disabled or no server running\n" );
		return;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( !pPlayer )
		return;

	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( pPlayer, pszTarget, pEntity )) != NULL )
	{
		if ( pEntity->m_ScriptScope.IsInitialized() && pEntity->m_iszScriptThinkFunction != NULL_STRING )
		{
			VScriptServerReplaceClosures( STRING(pEntity->m_iszScriptThinkFunction), pEntity->m_ScriptScope, true );
		}
	}
}

class CVScriptGameSystem : public CAutoGameSystemPerFrame
{
public:
	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void )
	{
		ClearScriptGameEventListeners();
		g_ScriptErrorScreenOverlay.Clear();

		// <sergiy> Note: we may need script VM garbage collection at this point in the future. Currently, VM does not persist 
		//          across level boundaries. GC is not necessary because our scripts are supposed to never create circular references
		//          and everything else is handled with ref counting. For the case of bugs creating circular references, the plan is to add
		//          diagnostics that detects such loops and warns the developer.
		
		m_bAllowEntityCreationInScripts = true;
		VScriptServerInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
	}

	virtual void LevelShutdownPostEntity( void )
	{
		ClearScriptGameEventListeners();
		VScriptServerTerm();
	}

	virtual void FrameUpdatePostEntityThink() 
	{ 
		if ( g_pScriptVM )
			g_pScriptVM->Frame( gpGlobals->frametime );

		g_ScriptErrorScreenOverlay.Draw();
	}

	bool m_bAllowEntityCreationInScripts;
};

CVScriptGameSystem g_VScriptGameSystem;

bool IsEntityCreationAllowedInScripts( void )
{
	return g_VScriptGameSystem.m_bAllowEntityCreationInScripts;
}

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
		return "VScriptServer";
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
		CBaseEntity *pEnt = gEntList.FirstEnt();
		while ( pEnt )
		{
			if ( pEnt->m_iszScriptId != NULL_STRING )
			{
				g_pScriptVM->RegisterClass( pEnt->GetScriptDesc() );
				m_InstanceMap.Insert( STRING( pEnt->m_iszScriptId ), pEnt );
			}
			pEnt = gEntList.NextEnt( pEnt );
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
				if ( g_pScriptVM->GetValue( STRING(pEnt->m_iszScriptId), &variant ) && variant.GetType() == FIELD_HSCRIPT)
				{
					pEnt->m_ScriptScope.Init( variant, false );
					pEnt->RunPrecacheScripts();
				}
			}
			else
			{
				// Script system probably has no internal references
				pEnt->m_iszScriptId = NULL_STRING;
			}
		}
		m_InstanceMap.Purge();
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

//-----------------------------------------------------------------------------

bool CBaseEntityScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CBaseEntity *pEntity = (CBaseEntity *)p;
	if ( pEntity->GetEntityName() != NULL_STRING )
	{
		V_snprintf( pBuf, bufSize, "([%d] %s: %s)", pEntity->entindex(), STRING(pEntity->m_iClassname), STRING( pEntity->GetEntityName() ) );
	}
	else
	{
		V_snprintf( pBuf, bufSize, "([%d] %s)", pEntity->entindex(), STRING(pEntity->m_iClassname) );
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

#ifdef TF_DLL
bool CNavAreaScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	CTFNavArea *pArea = (CTFNavArea *)p;
	V_snprintf( pBuf, bufSize, "([%u] Area)", pArea->GetID() );
	return true;
}

CNavAreaScriptInstanceHelper g_NavAreaScriptInstanceHelper;

bool INextBotComponentScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )	
{
	INextBotComponent *pNextBotComponent = (INextBotComponent *)p;
	if ( pNextBotComponent && pNextBotComponent->GetBot() )
		V_snprintf( pBuf, bufSize, "([%d] NextBotComponent)", pNextBotComponent->GetBot()->GetBotId() );
	else
		V_snprintf( pBuf, bufSize, "(Invalid NextBotComponent)" );
	return true;
}

INextBotComponentScriptInstanceHelper g_NextBotComponentScriptInstanceHelper;
#endif
