//========== Copyright (c) 2008, Valve Corporation, All rights reserved. ========
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
#include "vscript_client_nut.h"
#include "gameui/gameui_interface.h"

#ifdef PANORAMA_ENABLE
#include "panorama/panorama.h"
#include "panorama/uijsregistration.h"
#endif

#include "usermessages.h"
#include "hud_macros.h"

#if defined( PORTAL2_PUZZLEMAKER )
#include "matchmaking/imatchframework.h"
#endif // PORTAL2_PUZZLEMAKER

extern IScriptManager *scriptmanager;
extern ScriptClassDesc_t * GetScriptDesc( CBaseEntity * );

// #define VMPROFILE 1

#ifdef VMPROFILE

#define VMPROF_START double debugStartTime = Plat_FloatTime();
#define VMPROF_SHOW( funcname, funcdesc  ) DevMsg("***VSCRIPT PROFILE***: %s %s: %6.4f milliseconds\n", (##funcname), (##funcdesc), (Plat_FloatTime() - debugStartTime)*1000.0 );

#else // !VMPROFILE

#define VMPROF_START
#define VMPROF_SHOW

#endif // VMPROFILE

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#ifdef PANORAMA_ENABLE

DECLARE_PANORAMA_EVENT2( VScriptTrigger, const char *, const char * );
DEFINE_PANORAMA_EVENT( VScriptTrigger );

class CScriptPanorama
{
public:

	void DispatchEvent( const char *pszEventName, const char *pszMessage )
	{
		panorama::DispatchEvent( VScriptTrigger(), nullptr, pszEventName, pszMessage );
	}


private:
} g_ScriptPanorama;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptPanorama, "CPanorama", SCRIPT_SINGLETON "Panorama VScript Interface" )
	DEFINE_SCRIPTFUNC( DispatchEvent, "Trigger a panorama event" )
END_SCRIPTDESC();

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

#if defined( PORTAL2_PUZZLEMAKER )
void RequestMapRating( void )
{
	g_pMatchFramework->GetEventsSubscription()->BroadcastEvent( new KeyValues( "OnRequestMapRating" ) );		
}

//
//  Hack solution for the moment
//

void OpenVoteDialog( void )
{
	RequestMapRating();
}

ConCommand cm_open_vote_dialog( "cm_open_vote_dialog", OpenVoteDialog, "Opens the map voting dialog for testing purposes" );
#endif // PORTAL2_PUZZLEMAKER

int GetDeveloperLevel()
{
	return developer.GetInt();
}

bool VScriptClientInit()
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
				Log_Msg( LOG_VScript, "VSCRIPT: Started VScript virtual machine using script language '%s'\n", g_pScriptVM->GetLanguageName() );
				ScriptRegisterFunction( g_pScriptVM, GetMapName, "Get the name of the map.");
				ScriptRegisterFunction( g_pScriptVM, Time, "Get the current server time" );
				ScriptRegisterFunction( g_pScriptVM, DoIncludeScript, "Execute a script (internal)" );
				ScriptRegisterFunction( g_pScriptVM, GetDeveloperLevel, "Gets the level of 'develoer'" );
#if defined( PORTAL2_PUZZLEMAKER )
				ScriptRegisterFunction( g_pScriptVM, RequestMapRating, "Pops up the map rating dialog for user input" );
#endif // PORTAL2_PUZZLEMAKER
				
				if ( GameRules() )
				{
					GameRules()->RegisterScriptFunctions();
				}

#ifdef PANORAMA_ENABLE
				g_pScriptVM->RegisterInstance( &g_ScriptPanorama, "Panorama" );
#endif

				if ( scriptLanguage == SL_SQUIRREL )
				{
					g_pScriptVM->Run( g_Script_vscript_client );
				}

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
		Log_Msg( LOG_VScript, "\nVSCRIPT: Scripting is disabled.\n" );
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
		// <sergiy> Note: we may need script VM garbage collection at this point in the future. Currently, VM does not persist 
		//          across level boundaries. GC is not necessary because our scripts are supposed to never create circular references
		//          and everything else is handled with ref counting. For the case of bugs creating circular references, the plan is to add
		//          diagnostics that detects such loops and warns the developer.

		m_bAllowEntityCreationInScripts = true;
		VScriptClientInit();
	}

	virtual void LevelInitPostEntity( void )
	{
		m_bAllowEntityCreationInScripts = false;
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

//
// Slart: These were Portal 2 only, now they're not
//

bool __MsgFunc_SetMixLayerTriggerFactor(const CCSUsrMsg_SetMixLayerTriggerFactor &msg)
{
	int iLayerID = engine->GetMixLayerIndex(msg.layer().c_str());
	if (iLayerID < 0)
	{
		Warning("Invalid mix layer passed to SetMixLayerTriggerFactor: '%s'\n", msg.layer().c_str());
		return true;
	}
	int iGroupID = engine->GetMixGroupIndex(msg.group().c_str());
	if (iGroupID < 0)
	{
		Warning("Invalid mix group passed to SetMixLayerTriggerFactor: '%s'\n", msg.group().c_str());
		return true;
	}

	engine->SetMixLayerTriggerFactor(iLayerID, iGroupID, msg.factor());
	return true;
}

class CSetMixLayerTriggerHelper : public CAutoGameSystem 
{
	virtual bool Init()
	{
		for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
		{
			ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
			HOOK_MESSAGE( SetMixLayerTriggerFactor );
		}
		return true;
	}

	CUserMessageBinder m_UMCMsgSetMixLayerTriggerFactor;
};

static CSetMixLayerTriggerHelper g_SetMixLayerTriggerHelper;

#ifdef PANORAMA_ENABLE

bool __MsgFunc_PanoramaDispatchEvent( const CCSUsrMsg_PanoramaDispatchEvent &msg )
{
	g_ScriptPanorama.DispatchEvent( msg.event().c_str(), msg.message().c_str() );
	return true;
}

class CVScriptPanoramaHelper : public CAutoGameSystem 
{
	virtual bool Init()
	{
		for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
		{
			ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
			HOOK_MESSAGE( PanoramaDispatchEvent );
		}
		return true;
	}

	CUserMessageBinder m_UMCMsgPanoramaDispatchEvent;
};

static CVScriptPanoramaHelper g_VScriptPanoramaHelper;

#endif
