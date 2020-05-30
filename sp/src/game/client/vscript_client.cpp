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
#ifdef _WIN32
//#include "vscript_client_nut.h"
#endif
#ifdef MAPBASE_VSCRIPT
#include "c_world.h"
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
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

#define SCRIPT_MAT_PROXY_MAX_VARS 8

//-----------------------------------------------------------------------------
// Purpose: A material proxy which runs a VScript and allows it to read/write
//			to material variables.
//-----------------------------------------------------------------------------
class CScriptMaterialProxy : public IMaterialProxy
{
public:
	CScriptMaterialProxy();
	virtual ~CScriptMaterialProxy();

	virtual void Release( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pRenderable );
	virtual IMaterial *GetMaterial() { return NULL; }

	// It would be more preferable to init the script stuff in Init(), but
	// the VM isn't usually active by that time, so we have to init it when
	// it's first called in OnBind().
	bool InitScript();

	bool ValidateIndex(int i)
	{
		if (i > SCRIPT_MAT_PROXY_MAX_VARS || i < 0)
		{
			Warning("VScriptProxy: %i out of range", i);
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

private:
	IMaterialVar *m_MaterialVars[SCRIPT_MAT_PROXY_MAX_VARS];

	// Save the keyvalue string for InitScript()
	char m_szFilePath[MAX_PATH];

	CScriptScope	m_ScriptScope;
	HSCRIPT			m_hScriptInstance;
	HSCRIPT			m_hFuncOnBind;
};

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptMaterialProxy, "CScriptMaterialProxy", "Material proxy for VScript" )
	DEFINE_SCRIPTFUNC( GetVarString, "Gets a material var's string value" )
	DEFINE_SCRIPTFUNC( GetVarInt, "Gets a material var's int value" )
	DEFINE_SCRIPTFUNC( GetVarFloat, "Gets a material var's float value" )
	DEFINE_SCRIPTFUNC( GetVarVector, "Gets a material var's vector value" )
	DEFINE_SCRIPTFUNC( SetVarString, "Sets a material var's string value" )
	DEFINE_SCRIPTFUNC( SetVarInt, "Sets a material var's int value" )
	DEFINE_SCRIPTFUNC( SetVarFloat, "Sets a material var's float value" )
	DEFINE_SCRIPTFUNC( SetVarVector, "Sets a material var's vector value" )
END_SCRIPTDESC();

CScriptMaterialProxy::CScriptMaterialProxy()
{
	m_hScriptInstance = NULL;
	m_hFuncOnBind = NULL;
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
			DevMsg("VScriptProxy couldn't create ScriptScope!\n");
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

	return true;
}

void CScriptMaterialProxy::OnBind( void *pRenderable )
{
	if( !pRenderable )
		return;

	if (m_hFuncOnBind != NULL)
	{
		IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
		C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
		if ( pEnt )
		{
			g_pScriptVM->SetValue( m_ScriptScope, "entity", pEnt->GetScriptInstance() );
		}
		else
		{
			// Needs to register as a null value so the script doesn't break if it looks for an entity
			g_pScriptVM->SetValue( m_ScriptScope, "entity", SCRIPT_VARIANT_NULL );
		}

		m_ScriptScope.Call( m_hFuncOnBind, NULL );

		g_pScriptVM->ClearValue( m_ScriptScope, "entity" );
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

	// This is really bad. Too bad!
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

EXPOSE_INTERFACE( CScriptMaterialProxy, IMaterialProxy, "VScriptProxy" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif

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
			else
			{
				DevWarning("-scriptlang does not recognize a language named '%s'. virtual machine did NOT start.\n", pszScriptLanguage );
				scriptLanguage = SL_NONE;
			}

		}
		if( scriptLanguage != SL_NONE )
		{
			if ( g_pScriptVM == NULL )
				g_pScriptVM = scriptmanager->CreateVM( scriptLanguage );

			if( g_pScriptVM )
			{
				Log( "VSCRIPT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
				
				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

				//g_pScriptVM->RegisterInstance( &g_ScriptEntityIterator, "Entities" );

#ifdef MAPBASE_VSCRIPT
				IGameSystem::RegisterVScriptAllSystems();

				RegisterSharedScriptFunctions();
#else
				if ( scriptLanguage == SL_SQUIRREL )
				{
					//g_pScriptVM->Run( g_Script_vscript_client );
				}
#endif

				VScriptRunScript( "mapspawn", false );

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
		Log( "\nVSCRIPT: Scripting is disabled.\n" );
	}
	g_pScriptVM = NULL;
	return false;
}

void VScriptClientTerm()
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
#ifdef MAPBASE_VSCRIPT
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer)
		{
			g_pScriptVM->SetValue( "player", pPlayer->GetScriptInstance() );
		}
#endif
	}

	virtual void LevelShutdownPostEntity( void )
	{
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


