//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#include "cbase.h"
#include "vscript_client.h"
#include "icommandline.h"
#include "tier1/utlbuffer.h"
#include "tier1/fmtstr.h"
#include "filesystem.h"
#include "characterset.h"
#include "isaverestore.h"
#include "gamerules.h"
#include "vscript_client.nut"
#ifdef MAPBASE_VSCRIPT
#include "view.h"
#include "c_world.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "mapbase/matchers.h"
#include "mapbase/vscript_singletons.h"
#include "mapbase/vscript_vgui.h"
#endif

extern IScriptManager *scriptmanager;
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
static ScriptHook_t g_Hook_OnEntityCreated;
static ScriptHook_t g_Hook_OnEntityDeleted;

//-----------------------------------------------------------------------------
// Purpose: A clientside variant of CScriptEntityIterator.
//-----------------------------------------------------------------------------
class CScriptClientEntityIterator : public IClientEntityListener
{
public:
	HSCRIPT GetLocalPlayer()
	{
		return ToHScript( C_BasePlayer::GetLocalPlayer() );
	}

	HSCRIPT First() { return Next(NULL); }

	HSCRIPT Next( HSCRIPT hStartEntity )
	{
		return ToHScript( ClientEntityList().NextBaseEntity( ToEnt( hStartEntity ) ) );
	}

	HSCRIPT CreateByClassname( const char *className )
	{
		return ToHScript( CreateEntityByName( className ) );
	}

	HSCRIPT FindByClassname( HSCRIPT hStartEntity, const char *szName )
	{
		const CEntInfo *pInfo = hStartEntity ? ClientEntityList().GetEntInfoPtr( ToEnt( hStartEntity )->GetRefEHandle() )->m_pNext : ClientEntityList().FirstEntInfo();
		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pEntity;
			if ( !ent )
				continue;

			if ( Matcher_Match( szName, ent->GetClassname() ) )
				return ToHScript( ent );
		}

		return NULL;
	}

	HSCRIPT FindByName( HSCRIPT hStartEntity, const char *szName )
	{
		const CEntInfo *pInfo = hStartEntity ? ClientEntityList().GetEntInfoPtr( ToEnt( hStartEntity )->GetRefEHandle() )->m_pNext : ClientEntityList().FirstEntInfo();
		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			C_BaseEntity *ent = (C_BaseEntity *)pInfo->m_pEntity;
			if ( !ent )
				continue;

			if ( Matcher_Match( szName, ent->GetEntityName() ) )
				return ToHScript( ent );
		}

		return NULL;
	}

	void EnableEntityListening()
	{
		// Start getting entity updates!
		ClientEntityList().AddListenerEntity( this );
	}

	void DisableEntityListening()
	{
		// Stop getting entity updates!
		ClientEntityList().RemoveListenerEntity( this );
	}

	void OnEntityCreated( CBaseEntity *pEntity )
	{
		if ( g_pScriptVM && GetScriptHookManager().IsEventHooked( "OnEntityCreated" ) )
		{
			// entity
			ScriptVariant_t args[] = { ScriptVariant_t( pEntity->GetScriptInstance() ) };
			g_Hook_OnEntityCreated.Call( NULL, NULL, args );
		}
	};

	void OnEntityDeleted( CBaseEntity *pEntity )
	{
		if ( g_pScriptVM && GetScriptHookManager().IsEventHooked( "OnEntityDeleted" ) )
		{
			// entity
			ScriptVariant_t args[] = { ScriptVariant_t( pEntity->GetScriptInstance() ) };
			g_Hook_OnEntityDeleted.Call( NULL, NULL, args );
		}
	};

private:
} g_ScriptEntityIterator;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptClientEntityIterator, "CEntities", SCRIPT_SINGLETON "The global list of entities" )
	DEFINE_SCRIPTFUNC( GetLocalPlayer, "Get local player" )
	DEFINE_SCRIPTFUNC( First, "Begin an iteration over the list of entities" )
	DEFINE_SCRIPTFUNC( Next, "Continue an iteration over the list of entities, providing reference to a previously found entity" )
	DEFINE_SCRIPTFUNC( CreateByClassname, "Creates an entity by classname" )
	DEFINE_SCRIPTFUNC( FindByClassname, "Find entities by class name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )
	DEFINE_SCRIPTFUNC( FindByName, "Find entities by name. Pass 'null' to start an iteration, or reference to a previously found entity to continue a search"  )

	DEFINE_SCRIPTFUNC( EnableEntityListening, "Enables the 'OnEntity' hooks. This function must be called before using them." )
	DEFINE_SCRIPTFUNC( DisableEntityListening, "Disables the 'OnEntity' hooks." )

	BEGIN_SCRIPTHOOK( g_Hook_OnEntityCreated, "OnEntityCreated", FIELD_VOID, "Called when an entity is created. Requires EnableEntityListening() to be fired beforehand." )
		DEFINE_SCRIPTHOOK_PARAM( "entity", FIELD_HSCRIPT )
	END_SCRIPTHOOK()

	BEGIN_SCRIPTHOOK( g_Hook_OnEntityDeleted, "OnEntityDeleted", FIELD_VOID, "Called when an entity is deleted. Requires EnableEntityListening() to be fired beforehand." )
		DEFINE_SCRIPTHOOK_PARAM( "entity", FIELD_HSCRIPT )
	END_SCRIPTHOOK()
END_SCRIPTDESC();

//-----------------------------------------------------------------------------
// Purpose: A base class for VScript-utilizing clientside classes which can persist
//			across levels, requiring their scripts to be shut down manually.
//-----------------------------------------------------------------------------
abstract_class IClientScriptPersistable
{
public:
	virtual void TermScript() = 0;
};

CUtlVector<IClientScriptPersistable*> g_ScriptPersistableList;

#define SCRIPT_MAT_PROXY_MAX_VARS 8

//-----------------------------------------------------------------------------
// Purpose: A material proxy which runs a VScript and allows it to read/write
//			to material variables.
//-----------------------------------------------------------------------------
class CScriptMaterialProxy : public IMaterialProxy, public IClientScriptPersistable
{
public:
	CScriptMaterialProxy();
	virtual ~CScriptMaterialProxy();

	virtual void Release( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pRenderable );
	virtual IMaterial *GetMaterial() { return NULL; }

	// Proxies can persist across levels and aren't bound to a loaded map.
	// The VM, however, is bound to the loaded map, so the proxy's script variables persisting
	// causes problems when they're used in a new level with a new VM.
	// As a result, we call InitScript() and TermScript() during OnBind and when the level is unloaded respectively.
	bool InitScript();
	void TermScript();

	bool ValidateIndex(int i)
	{
		if (i > SCRIPT_MAT_PROXY_MAX_VARS || i < 0)
		{
			CGWarning( 0, CON_GROUP_VSCRIPT, "VScriptProxy: %i out of range", i );
			return false;
		}

		return true;
	}

	const char *GetVarString( int i );
	int GetVarInt( int i );
	float GetVarFloat( int i );
	const Vector& GetVarVector( int i );

	void SetVarString( int i, const char *value );
	void SetVarInt( int i, int value );
	void SetVarFloat( int i, float value );
	void SetVarVector( int i, const Vector &value );

	const char *GetVarName( int i );

private:
	IMaterialVar *m_MaterialVars[SCRIPT_MAT_PROXY_MAX_VARS];

	// Save the keyvalue string for InitScript()
	char m_szFilePath[MAX_PATH];

	CScriptScope	m_ScriptScope;
	HSCRIPT			m_hScriptInstance;
	HSCRIPT			m_hFuncOnBind;
};

class CMaterialProxyScriptInstanceHelper : public IScriptInstanceHelper
{
	bool ToString( void *p, char *pBuf, int bufSize );
	void *BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId );
};

CMaterialProxyScriptInstanceHelper g_MaterialProxyScriptInstanceHelper;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptMaterialProxy, "CScriptMaterialProxy", "Material proxy for VScript" )
	DEFINE_SCRIPT_INSTANCE_HELPER( &g_MaterialProxyScriptInstanceHelper )
	DEFINE_SCRIPTFUNC( GetVarString, "Gets a material var's string value" )
	DEFINE_SCRIPTFUNC( GetVarInt, "Gets a material var's int value" )
	DEFINE_SCRIPTFUNC( GetVarFloat, "Gets a material var's float value" )
	DEFINE_SCRIPTFUNC( GetVarVector, "Gets a material var's vector value" )
	DEFINE_SCRIPTFUNC( SetVarString, "Sets a material var's string value" )
	DEFINE_SCRIPTFUNC( SetVarInt, "Sets a material var's int value" )
	DEFINE_SCRIPTFUNC( SetVarFloat, "Sets a material var's float value" )
	DEFINE_SCRIPTFUNC( SetVarVector, "Sets a material var's vector value" )

	DEFINE_SCRIPTFUNC( GetVarName, "Gets a material var's name" )
END_SCRIPTDESC();

CScriptMaterialProxy::CScriptMaterialProxy()
{
	m_hScriptInstance = NULL;
	m_hFuncOnBind = NULL;

	V_memset( m_MaterialVars, 0, sizeof(m_MaterialVars) );
}

CScriptMaterialProxy::~CScriptMaterialProxy()
{
}


//-----------------------------------------------------------------------------
// Cleanup
//-----------------------------------------------------------------------------
void CScriptMaterialProxy::Release( void )
{ 
	if ( m_hScriptInstance && g_pScriptVM )
	{
		g_pScriptVM->RemoveInstance( m_hScriptInstance );
		m_hScriptInstance = NULL;
	}

	delete this; 
}

bool CScriptMaterialProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	for (KeyValues *pKey = pKeyValues->GetFirstSubKey(); pKey != NULL; pKey = pKey->GetNextKey())
	{
		// Get each variable we're looking for
		if (Q_strnicmp( pKey->GetName(), "var", 3 ) == 0)
		{
			int index = atoi(pKey->GetName() + 3);
			if (index > SCRIPT_MAT_PROXY_MAX_VARS)
			{
				Warning("VScript material proxy only supports 8 vars (not %i)\n", index);
				continue;
			}
			
			bool foundVar;
			m_MaterialVars[index] = pMaterial->FindVar( pKey->GetString(), &foundVar );

			// Don't init if we didn't find the var
			if (!foundVar)
				return false;
		}
		else if (FStrEq( pKey->GetName(), "scriptfile" ))
		{
			Q_strncpy( m_szFilePath, pKey->GetString(), sizeof( m_szFilePath ) );
		}
	}

	return true;
}

bool CScriptMaterialProxy::InitScript()
{
	if (!m_ScriptScope.IsInitialized())
	{
		if (scriptmanager == NULL)
		{
			ExecuteOnce(DevMsg("Cannot execute script because scripting is disabled (-scripting)\n"));
			return false;
		}

		if (g_pScriptVM == NULL)
		{
			ExecuteOnce(DevMsg(" Cannot execute script because there is no available VM\n"));
			return false;
		}

		char* iszScriptId = (char*)stackalloc( 1024 );
		g_pScriptVM->GenerateUniqueKey("VScriptProxy", iszScriptId, 1024);

		m_hScriptInstance = g_pScriptVM->RegisterInstance( GetScriptDescForClass( CScriptMaterialProxy ), this );
		g_pScriptVM->SetInstanceUniqeId( m_hScriptInstance, iszScriptId );

		bool bResult = m_ScriptScope.Init( iszScriptId );

		if (!bResult)
		{
			CGMsg( 1, CON_GROUP_VSCRIPT, "VScriptProxy couldn't create ScriptScope!\n" );
			return false;
		}

		g_pScriptVM->SetValue( m_ScriptScope, "self", m_hScriptInstance );
	}

	// Don't init if we can't run the script
	if (!VScriptRunScript( m_szFilePath, m_ScriptScope, true ))
		return false;
	
	m_hFuncOnBind = m_ScriptScope.LookupFunction( "OnBind" );

	if (!m_hFuncOnBind)
	{
		// Don't init if we can't find our func
		Warning("VScript material proxy can't find OnBind function\n");
		return false;
	}

	g_ScriptPersistableList.AddToTail( this );
	return true;
}

void CScriptMaterialProxy::TermScript()
{
	if ( m_hScriptInstance )
	{
		g_pScriptVM->RemoveInstance( m_hScriptInstance );
		m_hScriptInstance = NULL;
	}

	m_hFuncOnBind = NULL;

	m_ScriptScope.Term();
}

void CScriptMaterialProxy::OnBind( void *pRenderable )
{
	if (m_hFuncOnBind != NULL)
	{
		C_BaseEntity *pEnt = NULL;
		if (pRenderable)
		{
			IClientRenderable *pRend = (IClientRenderable*)pRenderable;
			pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
			if ( pEnt )
			{
				g_pScriptVM->SetValue( m_ScriptScope, "entity", pEnt->GetScriptInstance() );
			}
		}

		if (!pEnt)
		{
			g_pScriptVM->SetValue( m_ScriptScope, "entity", SCRIPT_VARIANT_NULL );
		}

		m_ScriptScope.Call( m_hFuncOnBind, NULL );
	}
	else
	{
		// The VM might not exist if we do it from Init(), so we have to do it here.
		// TODO: We have no handling for if this fails, how do you cancel a proxy?
		if (InitScript())
			OnBind( pRenderable );
	}
}

const char *CScriptMaterialProxy::GetVarString( int i )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return NULL;

	return m_MaterialVars[i]->GetStringValue();
}

int CScriptMaterialProxy::GetVarInt( int i )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return 0;

	return m_MaterialVars[i]->GetIntValue();
}

float CScriptMaterialProxy::GetVarFloat( int i )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return 0.0f;

	return m_MaterialVars[i]->GetFloatValue();
}

const Vector& CScriptMaterialProxy::GetVarVector( int i )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return vec3_origin;

	if (m_MaterialVars[i]->GetType() != MATERIAL_VAR_TYPE_VECTOR)
		return vec3_origin;

	return *(reinterpret_cast<const Vector*>(m_MaterialVars[i]->GetVecValue()));
}

void CScriptMaterialProxy::SetVarString( int i, const char *value )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return;

	return m_MaterialVars[i]->SetStringValue( value );
}

void CScriptMaterialProxy::SetVarInt( int i, int value )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return;

	return m_MaterialVars[i]->SetIntValue( value );
}

void CScriptMaterialProxy::SetVarFloat( int i, float value )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return;

	return m_MaterialVars[i]->SetFloatValue( value );
}

void CScriptMaterialProxy::SetVarVector( int i, const Vector &value )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return;

	return m_MaterialVars[i]->SetVecValue( value.Base(), 3 );
}

const char *CScriptMaterialProxy::GetVarName( int i )
{
	if (!ValidateIndex( i ) || !m_MaterialVars[i])
		return NULL;

	return m_MaterialVars[i]->GetName();
}

EXPOSE_INTERFACE( CScriptMaterialProxy, IMaterialProxy, "VScriptProxy" IMATERIAL_PROXY_INTERFACE_VERSION );

bool CMaterialProxyScriptInstanceHelper::ToString( void *p, char *pBuf, int bufSize )
{
	CScriptMaterialProxy *pProxy = (CScriptMaterialProxy *)p;
	V_snprintf( pBuf, bufSize, "(proxy: %s)", pProxy->GetMaterial() != NULL ? pProxy->GetMaterial()->GetName() : "<no material>" );
	return true; 
}

void *CMaterialProxyScriptInstanceHelper::BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
{
	// TODO: Material proxy save/restore?
	return NULL;
}
#endif // MAPBASE_VSCRIPT

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
static float Time()
{
	return gpGlobals->curtime;
}

static const char *GetMapName()
{
	return engine->GetLevelName();
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

#ifdef MAPBASE_VSCRIPT
int ScriptScreenWidth();
int ScriptScreenHeight();

static float FrameTime()
{
	return gpGlobals->frametime;
}

static bool Con_IsVisible()
{
	return engine->Con_IsVisible();
}

static bool IsWindowedMode()
{
	return engine->IsWindowedMode();
}

// Creates a client-side prop
HSCRIPT CreateProp( const char *pszEntityName, const Vector &vOrigin, const char *pszModelName, int iAnim )
{
	C_BaseAnimating *pBaseEntity = (C_BaseAnimating *)CreateEntityByName( pszEntityName );
	if (!pBaseEntity)
		return NULL;

	pBaseEntity->SetAbsOrigin( vOrigin );
	pBaseEntity->SetModelName( pszModelName );
	if (!pBaseEntity->InitializeAsClientEntity( pszModelName, RENDER_GROUP_OPAQUE_ENTITY ))
	{
		Warning("Can't initialize %s as client entity\n", pszEntityName);
		return NULL;
	}

	pBaseEntity->SetPlaybackRate( 1.0f );

	int iSequence = pBaseEntity->SelectWeightedSequence( (Activity)iAnim );

	if ( iSequence != -1 )
	{
		pBaseEntity->SetSequence( iSequence );
	}

	return ToHScript( pBaseEntity );
}
#endif

bool VScriptClientInit()
{
	VMPROF_START

	if( scriptmanager != NULL )
	{
		ScriptLanguage_t scriptLanguage = SL_DEFAULT;

		char const *pszScriptLanguage;
#ifdef MAPBASE_VSCRIPT
		if (GetClientWorldEntity()->GetScriptLanguage() != SL_NONE)
		{
			// Allow world entity to override script language
			scriptLanguage = GetClientWorldEntity()->GetScriptLanguage();

			// Less than SL_NONE means the script language should literally be none
			if (scriptLanguage < SL_NONE)
				scriptLanguage = SL_NONE;
		}
		else
#endif
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
#ifdef MAPBASE_VSCRIPT
			else if( !Q_stricmp(pszScriptLanguage, "lua") )
			{
				scriptLanguage = SL_LUA;
			}
#endif
			else
			{
				CGWarning( 1, CON_GROUP_VSCRIPT, "-scriptlang does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}

		}
		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if( g_pScriptVM )
			{
#ifdef MAPBASE_VSCRIPT
				CGMsg( 0, CON_GROUP_VSCRIPT, "VSCRIPT CLIENT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
#else
				Log( "VSCRIPT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
#endif

#ifdef MAPBASE_VSCRIPT
				GetScriptHookManager().OnInit();
#endif

				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, DoUniqueString, SCRIPT_ALIAS( "UniqueString", "Generate a string guaranteed to be unique across the life of the script VM, with an optional root string." ) );
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
#ifdef MAPBASE_VSCRIPT
				ScriptRegisterFunction( g_pScriptVM, FrameTime, "Get the time spent on the client in the last frame" );
				ScriptRegisterFunction( g_pScriptVM, Con_IsVisible, "Returns true if the console is visible" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptScreenWidth, "ScreenWidth", "Width of the screen in pixels" );
				ScriptRegisterFunctionNamed( g_pScriptVM, ScriptScreenHeight, "ScreenHeight", "Height of the screen in pixels" );
				ScriptRegisterFunction( g_pScriptVM, IsWindowedMode, "" );

				ScriptRegisterFunction( g_pScriptVM, MainViewOrigin, "" );
				ScriptRegisterFunction( g_pScriptVM, MainViewAngles, "" );
				ScriptRegisterFunction( g_pScriptVM, PrevMainViewOrigin, "" );
				ScriptRegisterFunction( g_pScriptVM, PrevMainViewAngles, "" );
				ScriptRegisterFunction( g_pScriptVM, MainViewForward, "" );
				ScriptRegisterFunction( g_pScriptVM, MainViewRight, "" );
				ScriptRegisterFunction( g_pScriptVM, MainViewUp, "" );

				ScriptRegisterFunction( g_pScriptVM, CurrentViewOrigin, "" );
				ScriptRegisterFunction( g_pScriptVM, CurrentViewAngles, "" );
				ScriptRegisterFunction( g_pScriptVM, CurrentViewForward, "" );
				ScriptRegisterFunction( g_pScriptVM, CurrentViewRight, "" );
				ScriptRegisterFunction( g_pScriptVM, CurrentViewUp, "" );

				ScriptRegisterFunction( g_pScriptVM, CreateProp, "Create an animating prop" );
#endif


				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

#ifdef MAPBASE_VSCRIPT
				g_pScriptVM->RegisterAllClasses();
				g_pScriptVM->RegisterAllEnums();

				g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );

				IGameSystem::RegisterVScriptAllSystems();

				RegisterSharedScriptConstants();
				RegisterSharedScriptFunctions();
				RegisterScriptVGUI();
#else
				//g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );
#endif

				if (scriptLanguage == SL_SQUIRREL)
				{
					g_pScriptVM->Run( g_Script_vscript_client );
				}

				VScriptRunScript( "vscript_client", true );
				VScriptRunScript( "mapspawn", false );

#ifdef MAPBASE_VSCRIPT
				RunAddonScripts();
#endif

				VMPROF_SHOW( pszScriptLanguage, "virtual machine startup" );

				return true;
			}
			else
			{
				CGWarning( 1, CON_GROUP_VSCRIPT, "VM Did not start!\n" );
			}
		}
#ifdef MAPBASE_VSCRIPT
		else
		{
			CGMsg( 0, CON_GROUP_VSCRIPT, "VSCRIPT CLIENT: Not starting because language is set to 'none'\n" );
		}
#endif
	}
	else
	{
		CGWarning( 0, CON_GROUP_VSCRIPT, "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptClientTerm()
{
	if( g_pScriptVM != NULL )
	{
#ifdef MAPBASE_VSCRIPT
		// Things like proxies can persist across levels, so we have to shut down their scripts manually
		for (int i = g_ScriptPersistableList.Count()-1; i >= 0; i--)
		{
			if (g_ScriptPersistableList[i])
			{
				g_ScriptPersistableList[i]->TermScript();
				g_ScriptPersistableList.FastRemove( i );
			}
		}
#endif

		if( g_pScriptVM )
		{
			scriptmanager->DestroyVM( g_pScriptVM );
			g_pScriptVM = NULL;
		}
	}
}


class CVScriptGameSystem : public CAutoGameSystemPerFrame
{
public:
	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void )
	{
		m_bAllowEntityCreationInScripts = true;
		VScriptClientInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
	}

	virtual void LevelShutdownPostEntity( void )
	{
#ifdef MAPBASE_VSCRIPT
		g_ScriptEntityIterator.DisableEntityListening();

		g_ScriptNetMsg->LevelShutdownPreVM();

		GetScriptHookManager().OnShutdown();
#endif
		VScriptClientTerm();
	}

	virtual void FrameUpdatePostEntityThink() 
	{ 
		if ( g_pScriptVM )
			g_pScriptVM->Frame( gpGlobals->frametime );
	}

	bool m_bAllowEntityCreationInScripts;
};

CVScriptGameSystem g_VScriptGameSystem;

bool IsEntityCreationAllowedInScripts( void )
{
	return g_VScriptGameSystem.m_bAllowEntityCreationInScripts;
}


