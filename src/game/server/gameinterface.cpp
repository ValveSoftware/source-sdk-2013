//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: encapsulates and implements all the accessing of the game dll from external
//			sources (only the engine at the time of writing)
//			This files ONLY contains functions and data necessary to build an interface
//			to external modules
//===========================================================================//

#include "cbase.h"
#include "gamestringpool.h"
#include "mapentities_shared.h"
#include "game.h"
#include "entityapi.h"
#include "client.h"
#include "saverestore.h"
#include "entitylist.h"
#include "gamerules.h"
#include "soundent.h"
#include "player.h"
#include "server_class.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_saverestore.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "ivoiceserver.h"
#include <stdarg.h>
#include "movehelper_server.h"
#include "networkstringtable_gamedll.h"
#include "filesystem.h"
#include "func_areaportalwindow.h"
#include "igamesystem.h"
#include "init_factory.h"
#include "vstdlib/random.h"
#include "env_wind_shared.h"
#include "engine/IEngineSound.h"
#include "ispatialpartition.h"
#include "textstatsmgr.h"
#include "bitbuf.h"
#include "saverestoretypes.h"
#include "physics_saverestore.h"
#include "achievement_saverestore.h"
#include "tier0/vprof.h"
#include "effect_dispatch_data.h"
#include "engine/IStaticPropMgr.h"
#include "TemplateEntities.h"
#include "ai_speech.h"
#include "soundenvelope.h"
#include "usermessages.h"
#include "physics.h"
#include "igameevents.h"
#include "EventLog.h"
#include "datacache/idatacache.h"
#include "engine/ivdebugoverlay.h"
#include "shareddefs.h"
#include "props.h"
#include "timedeventmgr.h"
#include "gameinterface.h"
#include "eventqueue.h"
#include "hltvdirector.h"
#if defined( REPLAY_ENABLED )
#include "replay/iserverreplaycontext.h"
#endif
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "AI_ResponseSystem.h"
#include "saverestore_stringtable.h"
#include "util.h"
#include "tier0/icommandline.h"
#include "datacache/imdlcache.h"
#include "engine/iserverplugin.h"
#ifdef _WIN32
#include "ienginevgui.h"
#endif
#include "ragdoll_shared.h"
#include "toolframework/iserverenginetools.h"
#include "sceneentity.h"
#include "appframework/IAppSystemGroup.h"
#include "scenefilecache/ISceneFileCache.h"
#include "tier2/tier2.h"
#include "particles/particles.h"
#include "gamestats.h"
#include "ixboxsystem.h"
#include "engine/imatchmaking.h"
#include "hl2orange.spa.h"
#include "particle_parse.h"
#ifndef NO_STEAM
#include "steam/steam_gameserver.h"
#endif
#include "tier3/tier3.h"
#include "serverbenchmark_base.h"
#include "querycache.h"
#include "player_voice_listener.h"

#ifdef TF_DLL
#include "gc_clientsystem.h"
#include "econ_item_inventory.h"
#include "steamworks_gamestats.h"
#include "tf/tf_gc_server.h"
#include "tf_gamerules.h"
#include "player_vs_environment/tf_population_manager.h"
#include "workshop/maps_workshop.h"

extern ConVar tf_mm_trusted;
extern ConVar tf_mm_servermode;
#endif

#ifdef USE_NAV_MESH
#include "nav_mesh.h"
#endif

#ifdef NEXT_BOT
#include "NextBotManager.h"
#endif

#ifdef USES_ECON_ITEMS
#include "econ_item_system.h"
#endif // USES_ECON_ITEMS

#ifdef CSTRIKE_DLL // BOTPORT: TODO: move these ifdefs out
#include "bot/bot.h"
#endif

#ifdef PORTAL
#include "prop_portal_shared.h"
#include "portal_player.h"
#endif

#if defined( REPLAY_ENABLED )
#include "replay/ireplaysystem.h"
#endif

extern IToolFrameworkServer *g_pToolFrameworkServer;
extern IParticleSystemQuery *g_pParticleSystemQuery;

extern ConVar commentary;

#ifndef NO_STEAM
// this context is not available on dedicated servers
// WARNING! always check if interfaces are available before using
static CSteamAPIContext s_SteamAPIContext;	
CSteamAPIContext *steamapicontext = &s_SteamAPIContext;

// this context is not available on a pure client connected to a remote server.
// WARNING! always check if interfaces are available before using
static CSteamGameServerAPIContext s_SteamGameServerAPIContext;
CSteamGameServerAPIContext *steamgameserverapicontext = &s_SteamGameServerAPIContext;
#endif

IUploadGameStats *gamestatsuploader = NULL;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CTimedEventMgr g_NetworkPropertyEventMgr;

ISaveRestoreBlockHandler *GetEventQueueSaveRestoreBlockHandler();
ISaveRestoreBlockHandler *GetCommentarySaveRestoreBlockHandler();

CUtlLinkedList<CMapEntityRef, unsigned short> g_MapEntityRefs;

// Engine interfaces.
IVEngineServer	*engine = NULL;
IVoiceServer	*g_pVoiceServer = NULL;
#if !defined(_STATIC_LINKED)
IFileSystem		*filesystem = NULL;
#else
extern IFileSystem *filesystem;
#endif
INetworkStringTableContainer *networkstringtable = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IUniformRandomStream *random = NULL;
IEngineSound *enginesound = NULL;
ISpatialPartition *partition = NULL;
IVModelInfo *modelinfo = NULL;
IEngineTrace *enginetrace = NULL;
IGameEventManager2 *gameeventmanager = NULL;
IDataCache *datacache = NULL;
IVDebugOverlay * debugoverlay = NULL;
ISoundEmitterSystemBase *soundemitterbase = NULL;
IServerPluginHelpers *serverpluginhelpers = NULL;
IServerEngineTools *serverenginetools = NULL;
ISceneFileCache *scenefilecache = NULL;
IXboxSystem *xboxsystem = NULL;	// Xbox 360 only
IMatchmaking *matchmaking = NULL;	// Xbox 360 only
IScriptManager *scriptmanager = NULL;
#if defined( REPLAY_ENABLED )
IReplaySystem *g_pReplay = NULL;
IServerReplayContext *g_pReplayServerContext = NULL;
#endif

IGameSystem *SoundEmitterSystem();

bool ModelSoundsCacheInit();
void ModelSoundsCacheShutdown();

void SceneManager_ClientActive( CBasePlayer *player );

class IMaterialSystem;
class IStudioRender;

#ifdef _DEBUG
static ConVar s_UseNetworkVars( "UseNetworkVars", "1", FCVAR_CHEAT, "For profiling, toggle network vars." );
#endif

extern ConVar sv_noclipduringpause;
ConVar sv_massreport( "sv_massreport", "0" );
ConVar sv_force_transmit_ents( "sv_force_transmit_ents", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Will transmit all entities to client, regardless of PVS conditions (will still skip based on transmit flags, however)." );

ConVar sv_autosave( "sv_autosave", "1", 0, "Set to 1 to autosave game on level transition. Does not affect autosave triggers." );
ConVar *sv_maxreplay = NULL;
static ConVar  *g_pcv_commentary = NULL;
static ConVar *g_pcv_ThreadMode = NULL;
static ConVar *g_pcv_hideServer = NULL;

// String tables
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;
INetworkStringTable *g_pStringTableEffectDispatch = NULL;
INetworkStringTable *g_pStringTableVguiScreen = NULL;
INetworkStringTable *g_pStringTableMaterials = NULL;
INetworkStringTable *g_pStringTableInfoPanel = NULL;
INetworkStringTable *g_pStringTableClientSideChoreoScenes = NULL;
INetworkStringTable *g_pStringTableServerMapCycle = NULL;

#ifdef TF_DLL
INetworkStringTable *g_pStringTableServerPopFiles = NULL;
INetworkStringTable *g_pStringTableServerMapCycleMvM = NULL;
#endif

CStringTableSaveRestoreOps g_VguiScreenStringOps;

// Holds global variables shared between engine and game.
CGlobalVars *gpGlobals;
edict_t *g_pDebugEdictBase = 0;
static int		g_nCommandClientIndex = 0;

// The chapter number of the current
static int		g_nCurrentChapterIndex = -1;

#ifdef _DEBUG
static ConVar sv_showhitboxes( "sv_showhitboxes", "-1", FCVAR_CHEAT, "Send server-side hitboxes for specified entity to client (NOTE:  this uses lots of bandwidth, use on listen server only)." );
#endif

void PrecachePointTemplates();

static ClientPutInServerOverrideFn g_pClientPutInServerOverride = NULL;
static void UpdateChapterRestrictions( const char *mapname );

static void UpdateRichPresence ( void );


#if !defined( _XBOX ) // Don't doubly define this symbol.
CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;

#endif

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

const IChangeInfoAccessor *CBaseEdict::GetChangeAccessor() const
{
	return engine->GetChangeAccessor( (const edict_t *)this );
}

const char *GetHintTypeDescription( CAI_Hint *pHint );

void ClientPutInServerOverride( ClientPutInServerOverrideFn fn )
{
	g_pClientPutInServerOverride = fn;
}

ConVar ai_post_frame_navigation( "ai_post_frame_navigation", "0" );
class CPostFrameNavigationHook;
extern CPostFrameNavigationHook *PostFrameNavigationSystem( void );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_GetCommandClientIndex( void )
{
	// -1 == unknown,dedicated server console
	// 0  == player 1

	// Convert to 1 based offset
	return (g_nCommandClientIndex+1);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBasePlayer
//-----------------------------------------------------------------------------
CBasePlayer *UTIL_GetCommandClient( void )
{
	int idx = UTIL_GetCommandClientIndex();
	if ( idx > 0 )
	{
		return UTIL_PlayerByIndex( idx );
	}

	// HLDS console issued command
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves the MOD directory for the active game (ie. "hl2")
//-----------------------------------------------------------------------------

bool UTIL_GetModDir( char *lpszTextOut, unsigned int nSize )
{
	// Must pass in a buffer at least large enough to hold the desired string
	const char *pGameDir = CommandLine()->ParmValue( "-game", "hl2" );
	Assert( strlen(pGameDir) <= nSize );
	if ( strlen(pGameDir) > nSize )
		return false;

	Q_strncpy( lpszTextOut, pGameDir, nSize );
	if ( Q_strnchr( lpszTextOut, '/', nSize ) || Q_strnchr( lpszTextOut, '\\', nSize ) )
	{
		// Strip the last directory off (which will be our game dir)
		Q_StripLastDir( lpszTextOut, nSize );
		
		// Find the difference in string lengths and take that difference from the original string as the mod dir
		int dirlen = Q_strlen( lpszTextOut );
		Q_strncpy( lpszTextOut, pGameDir + dirlen, Q_strlen( pGameDir ) - dirlen + 1 );
	}

	return true;
}

extern void InitializeCvars( void );

CBaseEntity*	FindPickerEntity( CBasePlayer* pPlayer );
CAI_Node*		FindPickerAINode( CBasePlayer* pPlayer, NodeType_e nNodeType );
CAI_Link*		FindPickerAILink( CBasePlayer* pPlayer );
float			GetFloorZ(const Vector &origin);
void			UpdateAllClientData( void );
void			DrawMessageEntities();

#include "ai_network.h"

// For now just using one big AI network
extern ConVar think_limit;


#if 0
//-----------------------------------------------------------------------------
// Purpose: Draw output overlays for any measure sections
// Input  : 
//-----------------------------------------------------------------------------
void DrawMeasuredSections(void)
{
	int		row = 1;
	float	rowheight = 0.025;

	CMeasureSection *p = CMeasureSection::GetList();
	while ( p )
	{
		char str[256];
		Q_snprintf(str,sizeof(str),"%s",p->GetName());
		NDebugOverlay::ScreenText( 0.01,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );
		
		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.28,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );

		Q_snprintf(str,sizeof(str),"%5.2f\n",p->GetMaxTime().GetMillisecondsF());
		//Q_snprintf(str,sizeof(str),"%3.3f\n",p->GetTime().GetSeconds() * 100.0 / engine->Time());
		NDebugOverlay::ScreenText( 0.34,0.51+(row*rowheight),str, 255,255,255,255, 0.0 );


		row++;

		p = p->GetNext();
	}

	bool sort_reset = false;

	// Time to redo sort?
	if ( measure_resort.GetFloat() > 0.0 &&
		engine->Time() >= CMeasureSection::m_dNextResort )
	{
		// Redo it
		CMeasureSection::SortSections();
		// Set next time
		CMeasureSection::m_dNextResort = engine->Time() + measure_resort.GetFloat();
		// Flag to reset sort accumulator, too
		sort_reset = true;
	}

	// Iterate through the sections now
	p = CMeasureSection::GetList();
	while ( p )
	{
		// Update max 
		p->UpdateMax();

		// Reset regular accum.
		p->Reset();
		// Reset sort accum less often
		if ( sort_reset )
		{
			p->SortReset();
		}
		p = p->GetNext();
	}

}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void DrawAllDebugOverlays( void ) 
{
	// If in debug select mode print the selection entities name or classname
	if (CBaseEntity::m_bInDebugSelect)
	{
		CBasePlayer* pPlayer =  UTIL_PlayerByIndex( CBaseEntity::m_nDebugPlayer );

		if (pPlayer)
		{
			// First try to trace a hull to an entity
			CBaseEntity *pEntity = FindPickerEntity( pPlayer );

			if ( pEntity ) 
			{
				pEntity->DrawDebugTextOverlays();
				pEntity->DrawBBoxOverlay();
				pEntity->SendDebugPivotOverlay();
			}
		}
	}

	// --------------------------------------------------------
	//  Draw debug overlay lines 
	// --------------------------------------------------------
	UTIL_DrawOverlayLines();

	// ------------------------------------------------------------------------
	// If in wc_edit mode draw a box to highlight which node I'm looking at
	// ------------------------------------------------------------------------
	if (engine->IsInEditMode())
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex( CBaseEntity::m_nDebugPlayer );
		if (pPlayer) 
		{
			if (g_pAINetworkManager->GetEditOps()->m_bLinkEditMode)
			{
				CAI_Link* pAILink = FindPickerAILink(pPlayer);
				if (pAILink)
				{
					// For now just using one big AI network
					Vector startPos = g_pBigAINet->GetNode(pAILink->m_iSrcID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
					Vector endPos	= g_pBigAINet->GetNode(pAILink->m_iDestID)->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
					Vector linkDir	= startPos-endPos;
					float linkLen = VectorNormalize( linkDir );
					
					// Draw in green if link that's been turned off
					if (pAILink->m_LinkInfo & bits_LINK_OFF)
					{
						NDebugOverlay::BoxDirection(startPos, Vector(-4,-4,-4), Vector(-linkLen,4,4), linkDir, 0,255,0,40,0);
					}
					else
					{
						NDebugOverlay::BoxDirection(startPos, Vector(-4,-4,-4), Vector(-linkLen,4,4), linkDir, 255,0,0,40,0);
					}
				}
			}
			else
			{
				CAI_Node* pAINode;
				if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
				{
					pAINode = FindPickerAINode(pPlayer,NODE_AIR);
				}
				else
				{
					pAINode = FindPickerAINode(pPlayer,NODE_GROUND);
				}

				if (pAINode)
				{
					Vector vecPos = pAINode->GetPosition(g_pAINetworkManager->GetEditOps()->m_iHullDrawNum);
					NDebugOverlay::Box( vecPos, Vector(-8,-8,-8), Vector(8,8,8), 255,0,0,40,0);

					if ( pAINode->GetHint() )
					{
						CBaseEntity *pEnt = (CBaseEntity *)pAINode->GetHint();
						if ( pEnt->GetEntityName() != NULL_STRING )
						{
							NDebugOverlay::Text( vecPos + Vector(0,0,6), STRING(pEnt->GetEntityName()), false, 0 );
						}
						NDebugOverlay::Text( vecPos, GetHintTypeDescription( pAINode->GetHint() ), false, 0 );
					}
				}
			}
			// ------------------------------------
			// If in air edit mode draw guide line
			// ------------------------------------
			if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
			{
				UTIL_DrawPositioningOverlay(g_pAINetworkManager->GetEditOps()->m_flAirEditDistance);
			}
			else
			{
				NDebugOverlay::DrawGroundCrossHairOverlay();
			}
		}
	}

	// For not just using one big AI Network
	if ( g_pAINetworkManager )
	{
		g_pAINetworkManager->GetEditOps()->DrawAINetworkOverlay();
	}

	// PERFORMANCE: only do this in developer mode
	if ( g_pDeveloper->GetInt() && !engine->IsDedicatedServer() )
	{
		// iterate through all objects for debug overlays
		const CEntInfo *pInfo = gEntList.FirstEntInfo();

		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
			// HACKHACK: to flag off these calls
			if ( ent->m_debugOverlays || ent->m_pTimedOverlay )
			{
				MDLCACHE_CRITICAL_SECTION();
				ent->DrawDebugGeometryOverlays();
			}
		}
	}

	if ( sv_massreport.GetInt() )
	{
		// iterate through all objects for debug overlays
		const CEntInfo *pInfo = gEntList.FirstEntInfo();

		for ( ;pInfo; pInfo = pInfo->m_pNext )
		{
			CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
			if (!ent->VPhysicsGetObject())
				continue;

			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"%s: Mass: %.2f kg / %.2f lb (%s)", 
				STRING( ent->GetModelName() ), ent->VPhysicsGetObject()->GetMass(), 
				kg2lbs(ent->VPhysicsGetObject()->GetMass()), 
				GetMassEquivalent(ent->VPhysicsGetObject()->GetMass()));
			ent->EntityText(0, tempstr, 0);
		}
	}

	// A hack to draw point_message entities w/o developer required
	DrawMessageEntities();
}

CServerGameDLL g_ServerGameDLL;
// INTERFACEVERSION_SERVERGAMEDLL_VERSION_8 is compatible with the latest since we're only adding things to the end, so expose that as well.
//EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameDLL, IServerGameDLL008, INTERFACEVERSION_SERVERGAMEDLL_VERSION_8, g_ServerGameDLL );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameDLL, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL, g_ServerGameDLL);

// When bumping the version to this interface, check that our assumption is still valid and expose the older version in the same way
COMPILE_TIME_ASSERT( INTERFACEVERSION_SERVERGAMEDLL_INT == 12 );

bool CServerGameDLL::DLLInit( CreateInterfaceFn appSystemFactory, 
		CreateInterfaceFn physicsFactory, CreateInterfaceFn fileSystemFactory, 
		CGlobalVars *pGlobals)
{
	ConnectTier1Libraries( &appSystemFactory, 1 );
	ConnectTier2Libraries( &appSystemFactory, 1 );
	ConnectTier3Libraries( &appSystemFactory, 1 );

	// Connected in ConnectTier1Libraries
	if ( cvar == NULL )
		return false;

#ifndef _X360
	s_SteamAPIContext.Init();
	s_SteamGameServerAPIContext.Init();
#endif

	// init each (seperated for ease of debugging)
	if ( (engine = (IVEngineServer*)appSystemFactory(INTERFACEVERSION_VENGINESERVER, NULL)) == NULL )
		return false;
	if ( (g_pVoiceServer = (IVoiceServer*)appSystemFactory(INTERFACEVERSION_VOICESERVER, NULL)) == NULL )
		return false;
	if ( (networkstringtable = (INetworkStringTableContainer *)appSystemFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL)) == NULL )
		return false;
	if ( (staticpropmgr = (IStaticPropMgrServer *)appSystemFactory(INTERFACEVERSION_STATICPROPMGR_SERVER,NULL)) == NULL )
		return false;
	if ( (random = (IUniformRandomStream *)appSystemFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (enginesound = (IEngineSound *)appSystemFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (::partition = (ISpatialPartition *)appSystemFactory(INTERFACEVERSION_SPATIALPARTITION, NULL)) == NULL )
		return false;
	if ( (modelinfo = (IVModelInfo *)appSystemFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL)) == NULL )
		return false;
	if ( (enginetrace = (IEngineTrace *)appSystemFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL)) == NULL )
		return false;
	if ( (filesystem = (IFileSystem *)fileSystemFactory(FILESYSTEM_INTERFACE_VERSION,NULL)) == NULL )
		return false;
	if ( (gameeventmanager = (IGameEventManager2 *)appSystemFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL)) == NULL )
		return false;
	if ( (datacache = (IDataCache*)appSystemFactory(DATACACHE_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (soundemitterbase = (ISoundEmitterSystemBase *)appSystemFactory(SOUNDEMITTERSYSTEM_INTERFACE_VERSION, NULL)) == NULL )
		return false;
#ifndef _XBOX
	if ( (gamestatsuploader = (IUploadGameStats *)appSystemFactory( INTERFACEVERSION_UPLOADGAMESTATS, NULL )) == NULL )
		return false;
#endif
	if ( !mdlcache )
		return false;
	if ( (serverpluginhelpers = (IServerPluginHelpers *)appSystemFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL)) == NULL )
		return false;
	if ( (scenefilecache = (ISceneFileCache *)appSystemFactory( SCENE_FILE_CACHE_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( IsX360() && (xboxsystem = (IXboxSystem *)appSystemFactory( XBOXSYSTEM_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( IsX360() && (matchmaking = (IMatchmaking *)appSystemFactory( VENGINE_MATCHMAKING_VERSION, NULL )) == NULL )
		return false;

	if ( !CommandLine()->CheckParm( "-noscripting") )
	{
		scriptmanager = (IScriptManager *)appSystemFactory( VSCRIPT_INTERFACE_VERSION, NULL );
	}

	// If not running dedicated, grab the engine vgui interface
	if ( !engine->IsDedicatedServer() )
	{
#ifdef _WIN32
		// This interface is optional, and is only valid when running with -tools
		serverenginetools = ( IServerEngineTools * )appSystemFactory( VSERVERENGINETOOLS_INTERFACE_VERSION, NULL );
#endif
	}

	// Yes, both the client and game .dlls will try to Connect, the soundemittersystem.dll will handle this gracefully
	if ( !soundemitterbase->Connect( appSystemFactory ) )
		return false;

	// cache the globals
	gpGlobals = pGlobals;

	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();
	
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

	// save these in case other system inits need them
	factorylist_t factories;
	factories.engineFactory = appSystemFactory;
	factories.fileSystemFactory = fileSystemFactory;
	factories.physicsFactory = physicsFactory;
	FactoryList_Store( factories );

	// load used game events  
	gameeventmanager->LoadEventsFromFile("resource/gameevents.res");

	// init the cvar list first in case inits want to reference them
	InitializeCvars();
	
	// Initialize the particle system
	if ( !g_pParticleSystemMgr->Init( g_pParticleSystemQuery ) )
	{
		return false;
	}

	sv_cheats = g_pCVar->FindVar( "sv_cheats" );
	if ( !sv_cheats )
		return false;

	g_pcv_commentary = g_pCVar->FindVar( "commentary" );
	g_pcv_ThreadMode = g_pCVar->FindVar( "host_thread_mode" );
	g_pcv_hideServer = g_pCVar->FindVar( "hide_server" );

	sv_maxreplay = g_pCVar->FindVar( "sv_maxreplay" );

	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetEntitySaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetPhysSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetAISaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetTemplateSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetDefaultResponseSystemSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetCommentarySaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetEventQueueSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->AddBlockHandler( GetAchievementSaveRestoreBlockHandler() );

	// The string system must init first + shutdown last
	IGameSystem::Add( GameStringSystem() );

	// Physics must occur before the sound envelope manager
	IGameSystem::Add( PhysicsGameSystem() );
	
	// Used to service deferred navigation queries for NPCs
	IGameSystem::Add( (IGameSystem *) PostFrameNavigationSystem() );

	// Add game log system
	IGameSystem::Add( GameLogSystem() );
#ifndef _XBOX
	// Add HLTV director 
	IGameSystem::Add( HLTVDirectorSystem() );
#endif
	// Add sound emitter
	IGameSystem::Add( SoundEmitterSystem() );

	// load Mod specific game events ( MUST be before InitAllSystems() so it can pickup the mod specific events)
	gameeventmanager->LoadEventsFromFile("resource/ModEvents.res");

#ifdef CSTRIKE_DLL // BOTPORT: TODO: move these ifdefs out
	InstallBotControl();
#endif

	if ( !IGameSystem::InitAllSystems() )
		return false;

#if defined( REPLAY_ENABLED )
	if ( gameeventmanager->LoadEventsFromFile( "resource/replayevents.res" ) <= 0 )
	{
		Warning( "\n*\n* replayevents.res MISSING.\n*\n\n" );
		return false;
	}
#endif

	// Due to dependencies, these are not autogamesystems
	if ( !ModelSoundsCacheInit() )
	{
		return false;
	}

	InvalidateQueryCache();

	// try to get debug overlay, may be NULL if on HLDS
	debugoverlay = (IVDebugOverlay *)appSystemFactory( VDEBUG_OVERLAY_INTERFACE_VERSION, NULL );

#ifndef _XBOX
#ifdef USE_NAV_MESH
	// create the Navigation Mesh interface
	TheNavMesh = NavMeshFactory();
#endif

	// init the gamestatsupload connection
	gamestatsuploader->InitConnection();
#endif

	return true;
}

void CServerGameDLL::PostInit()
{
	IGameSystem::PostInitAllSystems();
}

void CServerGameDLL::DLLShutdown( void )
{

	// Due to dependencies, these are not autogamesystems
	ModelSoundsCacheShutdown();

	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetAchievementSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetCommentarySaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetEventQueueSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetDefaultResponseSystemSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetTemplateSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetAISaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetPhysSaveRestoreBlockHandler() );
	g_pGameSaveRestoreBlockSet->RemoveBlockHandler( GetEntitySaveRestoreBlockHandler() );

	char *pFilename = g_TextStatsMgr.GetStatsFilename();
	if ( !pFilename || !pFilename[0] )
	{
		g_TextStatsMgr.SetStatsFilename( "stats.txt" );
	}
	g_TextStatsMgr.WriteFile( filesystem );

	IGameSystem::ShutdownAllSystems();

#ifdef CSTRIKE_DLL // BOTPORT: TODO: move these ifdefs out
	RemoveBotControl();
#endif

#ifndef _XBOX
#ifdef USE_NAV_MESH
	// destroy the Navigation Mesh interface
	if ( TheNavMesh )
	{
		delete TheNavMesh;
		TheNavMesh = NULL;
	}
#endif
	// reset (shutdown) the gamestatsupload connection
	gamestatsuploader->InitConnection();
#endif

#ifndef _X360
	s_SteamAPIContext.Clear(); // Steam API context shutdown
	s_SteamGameServerAPIContext.Clear();
#endif	

	gameeventmanager = NULL;
	
	DisconnectTier3Libraries();
	DisconnectTier2Libraries();
	ConVar_Unregister();
	DisconnectTier1Libraries();
}

bool CServerGameDLL::ReplayInit( CreateInterfaceFn fnReplayFactory )
{
#if defined( REPLAY_ENABLED )
	if ( !IsPC() )
		return false;
	if ( (g_pReplay = ( IReplaySystem *)fnReplayFactory( REPLAY_INTERFACE_VERSION, NULL )) == NULL )
		return false;
	if ( (g_pReplayServerContext = g_pReplay->SV_GetContext()) == NULL )
		return false;
	return true;
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: See shareddefs.h for redefining this.  Don't even think about it, though, for HL2.  Or you will pay.  ywb 9/22/03
// Output : float
//-----------------------------------------------------------------------------
float CServerGameDLL::GetTickInterval( void ) const
{
	float tickinterval = DEFAULT_TICK_INTERVAL;

//=============================================================================
// HPE_BEGIN:
// [Forrest] For Counter-Strike, set default tick rate of 66 and removed -tickrate command line parameter.
//=============================================================================
// Ignoring this for now, server ops are abusing it
#if !defined( TF_DLL ) && !defined( CSTRIKE_DLL ) && !defined( DOD_DLL )
//=============================================================================
// HPE_END
//=============================================================================
	// override if tick rate specified in command line
	if ( CommandLine()->CheckParm( "-tickrate" ) )
	{
		float tickrate = CommandLine()->ParmValue( "-tickrate", 0 );
		if ( tickrate > 10 )
			tickinterval = 1.0f / tickrate;
	}
#endif

	return tickinterval;
}

// This is called when a new game is started. (restart, map)
bool CServerGameDLL::GameInit( void )
{
	ResetGlobalState();
	engine->ServerCommand( "exec game.cfg\n" );
	engine->ServerExecute( );
	CBaseEntity::sm_bAccurateTriggerBboxChecks = true;

	IGameEvent *event = gameeventmanager->CreateEvent( "game_init" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}

	return true;
}

// This is called when a game ends (server disconnect, death, restart, load)
// NOT on level transitions within a game
void CServerGameDLL::GameShutdown( void )
{
	ResetGlobalState();
}

static bool g_OneWayTransition = false;
void Game_SetOneWayTransition( void )
{
	g_OneWayTransition = true;
}

static CUtlVector<EHANDLE> g_RestoredEntities;
// just for debugging, assert that this is the only time this function is called
static bool g_InRestore = false;

void AddRestoredEntity( CBaseEntity *pEntity )
{
	Assert(g_InRestore);
	if ( !pEntity )
		return;

	g_RestoredEntities.AddToTail( EHANDLE(pEntity) );
}

void EndRestoreEntities()
{
	if ( !g_InRestore )
		return;
		
	// The entire hierarchy is restored, so we can call GetAbsOrigin again.
	//CBaseEntity::SetAbsQueriesValid( true );

	// Call all entities' OnRestore handlers
	for ( int i = g_RestoredEntities.Count()-1; i >=0; --i )
	{
		CBaseEntity *pEntity = g_RestoredEntities[i].Get();
		if ( pEntity && !pEntity->IsDormant() )
		{
			MDLCACHE_CRITICAL_SECTION();
			pEntity->OnRestore();
		}
	}

	g_RestoredEntities.Purge();

	IGameSystem::OnRestoreAllSystems();

	g_InRestore = false;
	gEntList.CleanupDeleteList();

	// HACKHACK: UNDONE: We need to redesign the main loop with respect to save/load/server activate
	g_ServerGameDLL.ServerActivate( NULL, 0, 0 );
	CBaseEntity::SetAllowPrecache( false );
}

void BeginRestoreEntities()
{
	if ( g_InRestore )
	{
		DevMsg( "BeginRestoreEntities without previous EndRestoreEntities.\n" );
		gEntList.CleanupDeleteList();
	}
	g_RestoredEntities.Purge();
	g_InRestore = true;

	CBaseEntity::SetAllowPrecache( true );

	// No calls to GetAbsOrigin until the entire hierarchy is restored!
	//CBaseEntity::SetAbsQueriesValid( false );
}

//-----------------------------------------------------------------------------
// Purpose: This prevents sv.tickcount/gpGlobals->tickcount from advancing during restore which
//  would cause a lot of the NPCs to fast forward their think times to the same
//  tick due to some ticks being elapsed during restore where there was no simulation going on
//-----------------------------------------------------------------------------
bool CServerGameDLL::IsRestoring()
{
	return g_InRestore;
}

float g_flServerCurTime = 0.0f;

// Called any time a new level is started (after GameInit() also on level transitions within a game)
bool CServerGameDLL::LevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background )
{
	VPROF("CServerGameDLL::LevelInit");

	g_flServerCurTime = gpGlobals->curtime;

#ifdef USES_ECON_ITEMS
	GameItemSchema_t *pItemSchema = ItemSystem()->GetItemSchema();
	if ( pItemSchema )
	{
		pItemSchema->BInitFromDelayedBuffer();
	}
#endif // USES_ECON_ITEMS

	ResetWindspeed();
	UpdateChapterRestrictions( pMapName );

	if ( IsX360() && !background && (gpGlobals->maxClients == 1) && (g_nCurrentChapterIndex >= 0) )
	{
		// Single player games tell xbox live what game & chapter the user is playing
		UpdateRichPresence();
	}

	// Parse the particle manifest file & register the effects within it
	ParseParticleEffects( false, false );

	//Tony; parse custom manifest if exists!
	ParseParticleEffectsMap( pMapName, false );

	// IGameSystem::LevelInitPreEntityAllSystems() is called when the world is precached
	// That happens either in LoadGameState() or in MapEntity_ParseAllEntities()
	if ( loadGame )
	{
		if ( pOldLevel )
		{
			gpGlobals->eLoadType = MapLoad_Transition;
		}
		else
		{
			gpGlobals->eLoadType = MapLoad_LoadGame;
		}

		BeginRestoreEntities();
		if ( !engine->LoadGameState( pMapName, 1 ) )
		{
			if ( pOldLevel )
			{
				MapEntity_ParseAllEntities( pMapEntities );
			}
			else
			{
				// Regular save load case
				return false;
			}
		}

		if ( pOldLevel )
		{
			engine->LoadAdjacentEnts( pOldLevel, pLandmarkName );
		}

		if ( g_OneWayTransition )
		{
			engine->ClearSaveDirAfterClientLoad();
		}

		if ( pOldLevel && sv_autosave.GetBool() == true )
		{
			// This is a single-player style level transition.
			// Queue up an autosave one second into the level
			CBaseEntity *pAutosave = CBaseEntity::Create( "logic_autosave", vec3_origin, vec3_angle, NULL );
			if ( pAutosave )
			{
				g_EventQueue.AddEvent( pAutosave, "Save", 1.0, NULL, NULL );
				g_EventQueue.AddEvent( pAutosave, "Kill", 1.1, NULL, NULL );
			}
		}
	}
	else
	{
		if ( background )
		{
			gpGlobals->eLoadType = MapLoad_Background;
		}
		else
		{
			gpGlobals->eLoadType = MapLoad_NewGame;
		}

		// Clear out entity references, and parse the entities into it.
		g_MapEntityRefs.Purge();
		CMapLoadEntityFilter filter;
		MapEntity_ParseAllEntities( pMapEntities, &filter );

		g_pServerBenchmark->StartBenchmark();

		// Now call the mod specific parse
		LevelInit_ParseAllEntities( pMapEntities );
	}

	// Check low violence settings for this map
	g_RagdollLVManager.SetLowViolence( pMapName );

	// Now that all of the active entities have been loaded in, precache any entities who need point_template parameters
	//  to be parsed (the above code has loaded all point_template entities)
	PrecachePointTemplates();

	// load MOTD from file into stringtable
	LoadMessageOfTheDay();

	// Sometimes an ent will Remove() itself during its precache, so RemoveImmediate won't happen.
	// This makes sure those ents get cleaned up.
	gEntList.CleanupDeleteList();

	g_AIFriendliesTalkSemaphore.Release();
	g_AIFoesTalkSemaphore.Release();
	g_OneWayTransition = false;

	// clear any pending autosavedangerous
	m_fAutoSaveDangerousTime = 0.0f;
	m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: called after every level change and load game, iterates through all the
//			active entities and gives them a chance to fix up their state
//-----------------------------------------------------------------------------
#ifdef DEBUG
bool g_bReceivedChainedActivate;
bool g_bCheckForChainedActivate;
#define BeginCheckChainedActivate() if (0) ; else { g_bCheckForChainedActivate = true; g_bReceivedChainedActivate = false; }
#define EndCheckChainedActivate( bCheck )	\
	if (0) ; else \
	{ \
		if ( bCheck ) \
		{ \
			AssertMsg( g_bReceivedChainedActivate, "Entity (%i/%s/%s) failed to call base class Activate()\n", pClass->entindex(), pClass->GetClassname(), STRING( pClass->GetEntityName() ) );	\
		} \
		g_bCheckForChainedActivate = false; \
	}
#else
#define BeginCheckChainedActivate()			((void)0)
#define EndCheckChainedActivate( bCheck )	((void)0)
#endif

void CServerGameDLL::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	// HACKHACK: UNDONE: We need to redesign the main loop with respect to save/load/server activate
	if ( g_InRestore )
		return;

	if ( gEntList.ResetDeleteList() != 0 )
	{
		Msg( "%s", "ERROR: Entity delete queue not empty on level start!\n" );
	}

	for ( CBaseEntity *pClass = gEntList.FirstEnt(); pClass != NULL; pClass = gEntList.NextEnt(pClass) )
	{
		if ( pClass && !pClass->IsDormant() )
		{
			MDLCACHE_CRITICAL_SECTION();

			BeginCheckChainedActivate();
			pClass->Activate();
			
			// We don't care if it finished activating if it decided to remove itself.
			EndCheckChainedActivate( !( pClass->GetEFlags() & EFL_KILLME ) ); 
		}
	}

	IGameSystem::LevelInitPostEntityAllSystems();
	// No more precaching after PostEntityAllSystems!!!
	CBaseEntity::SetAllowPrecache( false );

	// only display the think limit when the game is run with "developer" mode set
	if ( !g_pDeveloper->GetInt() )
	{
		think_limit.SetValue( 0 );
	}

#ifndef _XBOX
#ifdef USE_NAV_MESH
	// load the Navigation Mesh for this map
	TheNavMesh->Load();
	TheNavMesh->OnServerActivate();
#endif
#endif

#ifdef CSTRIKE_DLL // BOTPORT: TODO: move these ifdefs out
	TheBots->ServerActivate();
#endif

#ifdef NEXT_BOT
	TheNextBots().OnMapLoaded();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after the steam API has been activated post-level startup
//-----------------------------------------------------------------------------
void CServerGameDLL::GameServerSteamAPIActivated( void )
{
#ifndef NO_STEAM
	steamgameserverapicontext->Clear();
	steamgameserverapicontext->Init();
	if ( steamgameserverapicontext->SteamGameServer() && engine->IsDedicatedServer() )
	{
		steamgameserverapicontext->SteamGameServer()->GetGameplayStats();
	}
#endif

#ifdef TF_DLL
	GCClientSystem()->GameServerActivate();
	InventoryManager()->GameServerSteamAPIActivated();
	TFMapsWorkshop()->GameServerSteamAPIActivated();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called after the steam API has been activated post-level startup
//-----------------------------------------------------------------------------
void CServerGameDLL::GameServerSteamAPIShutdown( void )
{
#if !defined( NO_STEAM )
	if ( steamgameserverapicontext )
	{
		steamgameserverapicontext->Clear();
	}
#endif
#ifdef TF_DLL
	GCClientSystem()->Shutdown();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Called at the start of every game frame
//-----------------------------------------------------------------------------
ConVar  trace_report( "trace_report", "0" );

void CServerGameDLL::GameFrame( bool simulating )
{
	VPROF( "CServerGameDLL::GameFrame" );

	// Don't run frames until fully restored
	if ( g_InRestore )
		return;

	if ( CBaseEntity::IsSimulatingOnAlternateTicks() )
	{
		// only run simulation on even numbered ticks
		if ( gpGlobals->tickcount & 1 )
		{
			UpdateAllClientData();
			return;
		}
		// If we're skipping frames, then the frametime is 2x the normal tick
		gpGlobals->frametime *= 2.0f;
	}

	g_flServerCurTime = gpGlobals->curtime;
	float oldframetime = gpGlobals->frametime;

#ifdef _DEBUG
	// For profiling.. let them enable/disable the networkvar manual mode stuff.
	g_bUseNetworkVars = s_UseNetworkVars.GetBool();
#endif

	extern void GameStartFrame( void );
	extern void ServiceEventQueue( void );
	extern void Physics_RunThinkFunctions( bool simulating );

	// Delete anything that was marked for deletion
	//  outside of server frameloop (e.g., in response to concommand)
	gEntList.CleanupDeleteList();

	IGameSystem::FrameUpdatePreEntityThinkAllSystems();
	GameStartFrame();

#ifndef _XBOX
#ifdef USE_NAV_MESH
	TheNavMesh->Update();
#endif

#ifdef NEXT_BOT
	TheNextBots().Update();
#endif

	gamestatsuploader->UpdateConnection();
#endif

	UpdateQueryCache();
	g_pServerBenchmark->UpdateBenchmark();

	Physics_RunThinkFunctions( simulating );
	
	IGameSystem::FrameUpdatePostEntityThinkAllSystems();

	// UNDONE: Make these systems IGameSystems and move these calls into FrameUpdatePostEntityThink()
	// service event queue, firing off any actions whos time has come
	ServiceEventQueue();

	// free all ents marked in think functions
	gEntList.CleanupDeleteList();

	// FIXME:  Should this only occur on the final tick?
	UpdateAllClientData();

	if ( g_pGameRules )
	{
		g_pGameRules->EndGameFrame();
	}

	if ( trace_report.GetBool() )
	{
		int total = 0, totals[3];
		for ( int i = 0; i < 3; i++ )
		{
			totals[i] = enginetrace->GetStatByIndex( i, true );
			if ( totals[i] > 0 )
			{
				total += totals[i];
			}
		}

		if ( total )
		{
			Msg("Trace: %d, contents %d, enumerate %d\n", totals[0], totals[1], totals[2] );
		}
	}

	// Any entities that detect network state changes on a timer do it here.
	g_NetworkPropertyEventMgr.FireEvents();

	gpGlobals->frametime = oldframetime;
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame even if not ticking
// Input  : simulating - 
//-----------------------------------------------------------------------------
void CServerGameDLL::PreClientUpdate( bool simulating )
{
	if ( !simulating )
		return;

	/*
	if (game_speeds.GetInt())
	{
		DrawMeasuredSections();
	}
	*/

//#ifdef _DEBUG  - allow this in release for now
	DrawAllDebugOverlays();
//#endif
	
	IGameSystem::PreClientUpdateAllSystems();

#ifdef _DEBUG
	if ( sv_showhitboxes.GetInt() == -1 )
		return;

	if ( sv_showhitboxes.GetInt() == 0 )
	{
		// assume it's text
		CBaseEntity *pEntity = NULL;

		while (1)
		{
			pEntity = gEntList.FindEntityByName( pEntity, sv_showhitboxes.GetString() );
			if ( !pEntity )
				break;

			CBaseAnimating *anim = dynamic_cast< CBaseAnimating * >( pEntity );

			if (anim)
			{
				anim->DrawServerHitboxes();
			}
		}
		return;
	}

	CBaseAnimating *anim = dynamic_cast< CBaseAnimating * >( CBaseEntity::Instance( engine->PEntityOfEntIndex( sv_showhitboxes.GetInt() ) ) );
	if ( !anim )
		return;

	anim->DrawServerHitboxes();
#endif
}

void CServerGameDLL::Think( bool finalTick )
{
	if ( m_fAutoSaveDangerousTime != 0.0f && m_fAutoSaveDangerousTime < gpGlobals->curtime )
	{
		// The safety timer for a dangerous auto save has expired
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );

		if ( pPlayer && ( pPlayer->GetDeathTime() == 0.0f || pPlayer->GetDeathTime() > gpGlobals->curtime )
			&& !pPlayer->IsSinglePlayerGameEnding()
			)
		{
			if( pPlayer->GetHealth() >= m_fAutoSaveDangerousMinHealthToCommit )
			{
				// The player isn't dead, so make the dangerous auto save safe
				engine->ServerCommand( "autosavedangerousissafe\n" );
			}
		}

		m_fAutoSaveDangerousTime = 0.0f;
		m_fAutoSaveDangerousMinHealthToCommit = 0.0f;
	}
}

void CServerGameDLL::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
}


// Called when a level is shutdown (including changing levels)
void CServerGameDLL::LevelShutdown( void )
{
#ifndef NO_STEAM
	IGameSystem::LevelShutdownPreClearSteamAPIContextAllSystems();

	steamgameserverapicontext->Clear();
#endif

	g_pServerBenchmark->EndBenchmark();

	MDLCACHE_CRITICAL_SECTION();
	IGameSystem::LevelShutdownPreEntityAllSystems();

	// YWB:
	// This entity pointer is going away now and is corrupting memory on level transitions/restarts
	CSoundEnt::ShutdownSoundEnt();

	gEntList.Clear();

	InvalidateQueryCache();

	IGameSystem::LevelShutdownPostEntityAllSystems();

	// In case we quit out during initial load
	CBaseEntity::SetAllowPrecache( false );

	// Josh: Uncache all the particle systems on level shutdown
	// otherwise we leak them constantly on changelevel in the
	// particle precache stringtable list.
	g_pParticleSystemMgr->UncacheAllParticleSystems();
	g_pParticleSystemMgr->RecreateDictionary();

	g_nCurrentChapterIndex = -1;

#ifndef _XBOX
#ifdef USE_NAV_MESH
	// reset the Navigation Mesh
	if ( TheNavMesh )
	{
		TheNavMesh->Reset();
	}
#endif
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : ServerClass*
//-----------------------------------------------------------------------------
ServerClass* CServerGameDLL::GetAllServerClasses()
{
	return g_pServerClassHead;
}


const char *CServerGameDLL::GetGameDescription( void )
{
	return ::GetGameDescription();
}

void CServerGameDLL::CreateNetworkStringTables( void )
{
	// Create any shared string tables here (and only here!)
	// E.g.:  xxx = networkstringtable->CreateStringTable( "SceneStrings", 512 );
	g_pStringTableParticleEffectNames = networkstringtable->CreateStringTable( "ParticleEffectNames", MAX_PARTICLESYSTEMS_STRINGS );
	g_pStringTableEffectDispatch = networkstringtable->CreateStringTable( "EffectDispatch", MAX_EFFECT_DISPATCH_STRINGS );
	g_pStringTableVguiScreen = networkstringtable->CreateStringTable( "VguiScreen", MAX_VGUI_SCREEN_STRINGS );
	g_pStringTableMaterials = networkstringtable->CreateStringTable( "Materials", MAX_MATERIAL_STRINGS );
	g_pStringTableInfoPanel = networkstringtable->CreateStringTable( "InfoPanel", MAX_INFOPANEL_STRINGS );
	g_pStringTableClientSideChoreoScenes = networkstringtable->CreateStringTable( "Scenes", MAX_CHOREO_SCENES_STRINGS );
	g_pStringTableServerMapCycle = networkstringtable->CreateStringTable( "ServerMapCycle", 128 );

#ifdef TF_DLL
	g_pStringTableServerPopFiles = networkstringtable->CreateStringTable( "ServerPopFiles", 128 );
	g_pStringTableServerMapCycleMvM = networkstringtable->CreateStringTable( "ServerMapCycleMvM", 128 );
#endif

	bool bPopFilesValid = true;
	(void)bPopFilesValid; // Avoid unreferenced variable warning

#ifdef TF_DLL
	bPopFilesValid = ( g_pStringTableServerPopFiles != NULL );
#endif

	Assert( g_pStringTableParticleEffectNames &&
			g_pStringTableEffectDispatch &&
			g_pStringTableVguiScreen &&
			g_pStringTableMaterials &&
			g_pStringTableInfoPanel &&
			g_pStringTableClientSideChoreoScenes &&
			g_pStringTableServerMapCycle && 
			bPopFilesValid
			);

	// Need this so we have the error material always handy
	PrecacheMaterial( "debug/debugempty" );
	Assert( GetMaterialIndex( "debug/debugempty" ) == 0 );

	PrecacheParticleSystem( "error" );	// ensure error particle system is handy
	Assert( GetParticleSystemIndex( "error" ) == 0 );

	CreateNetworkStringTables_GameRules();

	// Set up save/load utilities for string tables
	g_VguiScreenStringOps.Init( g_pStringTableVguiScreen );
}

CSaveRestoreData *CServerGameDLL::SaveInit( int size )
{
	return ::SaveInit(size);
}

//-----------------------------------------------------------------------------
// Purpose: Saves data from a struct into a saverestore object, to be saved to disk
// Input  : *pSaveData - the saverestore object
//			char *pname - the name of the data to write
//			*pBaseData - the struct into which the data is to be read
//			*pFields - pointer to an array of data field descriptions
//			fieldCount - the size of the array (number of field descriptions)
//-----------------------------------------------------------------------------
void CServerGameDLL::SaveWriteFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pMap, pFields, fieldCount );
}


//-----------------------------------------------------------------------------
// Purpose: Reads data from a save/restore block into a structure
// Input  : *pSaveData - the saverestore object
//			char *pname - the name of the data to extract from
//			*pBaseData - the struct into which the data is to be restored
//			*pFields - pointer to an array of data field descriptions
//			fieldCount - the size of the array (number of field descriptions)
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

void CServerGameDLL::SaveReadFields( CSaveRestoreData *pSaveData, const char *pname, void *pBaseData, datamap_t *pMap, typedescription_t *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pMap, pFields, fieldCount );
}

//-----------------------------------------------------------------------------

void CServerGameDLL::SaveGlobalState( CSaveRestoreData *s )
{
	::SaveGlobalState(s);
}

void CServerGameDLL::RestoreGlobalState(CSaveRestoreData *s)
{
	::RestoreGlobalState(s);
}

void CServerGameDLL::Save( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->Save( &saveHelper );
}

void CServerGameDLL::Restore( CSaveRestoreData *s, bool b)
{
	CRestore restore(s);
	g_pGameSaveRestoreBlockSet->Restore( &restore, b );
	g_pGameSaveRestoreBlockSet->PostRestore();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_type - 
//			*name - 
//			size - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------

bool CServerGameDLL::GetUserMessageInfo( int msg_type, char *name, int maxnamelength, int& size )
{
	if ( !usermessages->IsValidIndex( msg_type ) )
		return false;

	Q_strncpy( name, usermessages->GetUserMessageName( msg_type ), maxnamelength );
	size = usermessages->GetUserMessageSize( msg_type );
	return true;
}

CStandardSendProxies* CServerGameDLL::GetStandardSendProxies()
{
	return &g_StandardSendProxies;
}

int	CServerGameDLL::CreateEntityTransitionList( CSaveRestoreData *s, int a)
{
	CRestore restoreHelper( s );
	// save off file base
	int base = restoreHelper.GetReadPos();

	int movedCount = ::CreateEntityTransitionList(s, a);
	if ( movedCount )
	{
		g_pGameSaveRestoreBlockSet->CallBlockHandlerRestore( GetPhysSaveRestoreBlockHandler(), base, &restoreHelper, false );
		g_pGameSaveRestoreBlockSet->CallBlockHandlerRestore( GetAISaveRestoreBlockHandler(), base, &restoreHelper, false );
	}

	GetPhysSaveRestoreBlockHandler()->PostRestore();
	GetAISaveRestoreBlockHandler()->PostRestore();

	return movedCount;
}

void CServerGameDLL::PreSave( CSaveRestoreData *s )
{
	g_pGameSaveRestoreBlockSet->PreSave( s );
}

#include "client_textmessage.h"

// This little hack lets me marry BSP names to messages in titles.txt
typedef struct
{
	const char *pBSPName;
	const char *pTitleName;
} TITLECOMMENT;

// this list gets searched for the first partial match, so some are out of order
static TITLECOMMENT gTitleComments[] =
{
#ifdef HL1_DLL
	{ "t0a0", "#T0A0TITLE" },
	{ "c0a0", "#HL1_Chapter1_Title" },
	{ "c1a0", "#HL1_Chapter2_Title" },
	{ "c1a1", "#HL1_Chapter3_Title" },
	{ "c1a2", "#HL1_Chapter4_Title" },
	{ "c1a3", "#HL1_Chapter5_Title" },
	{ "c1a4", "#HL1_Chapter6_Title" },
	{ "c2a1", "#HL1_Chapter7_Title" },
	{ "c2a2", "#HL1_Chapter8_Title" },
	{ "c2a3", "#HL1_Chapter9_Title" },
	{ "c2a4d", "#HL1_Chapter11_Title" },	// These must appear before "C2A4" so all other map names starting with C2A4 get that title
	{ "c2a4e", "#HL1_Chapter11_Title" },
	{ "c2a4f", "#HL1_Chapter11_Title" },
	{ "c2a4g", "#HL1_Chapter11_Title" },
	{ "c2a4", "#HL1_Chapter10_Title" },
	{ "c2a5", "#HL1_Chapter12_Title" },
	{ "c3a1", "#HL1_Chapter13_Title" },
	{ "c3a2", "#HL1_Chapter14_Title" },
	{ "c4a1a", "#HL1_Chapter17_Title"  },	// Order is important, see above
	{ "c4a1b", "#HL1_Chapter17_Title"  },
	{ "c4a1c", "#HL1_Chapter17_Title"  },
	{ "c4a1d", "#HL1_Chapter17_Title"  },
	{ "c4a1e", "#HL1_Chapter17_Title"  },
	{ "c4a1", "#HL1_Chapter15_Title" },
	{ "c4a2", "#HL1_Chapter16_Title"  },
	{ "c4a3", "#HL1_Chapter18_Title"  },
	{ "c5a1", "#HL1_Chapter19_Title"  },
#elif defined PORTAL
	{ "testchmb_a_00",			"#Portal_Chapter1_Title"  },
	{ "testchmb_a_01",			"#Portal_Chapter1_Title"  },
	{ "testchmb_a_02",			"#Portal_Chapter2_Title"  },
	{ "testchmb_a_03",			"#Portal_Chapter2_Title"  },
	{ "testchmb_a_04",			"#Portal_Chapter3_Title"  },
	{ "testchmb_a_05",			"#Portal_Chapter3_Title"  },
	{ "testchmb_a_06",			"#Portal_Chapter4_Title"  },
	{ "testchmb_a_07",			"#Portal_Chapter4_Title"  },
	{ "testchmb_a_08_advanced",	"#Portal_Chapter5_Title"  },
	{ "testchmb_a_08",			"#Portal_Chapter5_Title"  },
	{ "testchmb_a_09_advanced",	"#Portal_Chapter6_Title"  },
	{ "testchmb_a_09",			"#Portal_Chapter6_Title"  },
	{ "testchmb_a_10_advanced",	"#Portal_Chapter7_Title"  },
	{ "testchmb_a_10",			"#Portal_Chapter7_Title"  },
	{ "testchmb_a_11_advanced",	"#Portal_Chapter8_Title"  },
	{ "testchmb_a_11",			"#Portal_Chapter8_Title"  },
	{ "testchmb_a_13_advanced",	"#Portal_Chapter9_Title"  },
	{ "testchmb_a_13",			"#Portal_Chapter9_Title"  },
	{ "testchmb_a_14_advanced",	"#Portal_Chapter10_Title"  },
	{ "testchmb_a_14",			"#Portal_Chapter10_Title"  },
	{ "testchmb_a_15",			"#Portal_Chapter11_Title"  },
	{ "escape_",				"#Portal_Chapter11_Title"  },
	{ "background2",			"#Portal_Chapter12_Title"  },
#else
	{ "intro", "#HL2_Chapter1_Title" },

	{ "d1_trainstation_05", "#HL2_Chapter2_Title" },
	{ "d1_trainstation_06", "#HL2_Chapter2_Title" },
	
	{ "d1_trainstation_", "#HL2_Chapter1_Title" },

	{ "d1_canals_06", "#HL2_Chapter4_Title" },
	{ "d1_canals_07", "#HL2_Chapter4_Title" },
	{ "d1_canals_08", "#HL2_Chapter4_Title" },
	{ "d1_canals_09", "#HL2_Chapter4_Title" },
	{ "d1_canals_1", "#HL2_Chapter4_Title" },
	
	{ "d1_canals_0", "#HL2_Chapter3_Title" },

	{ "d1_eli_", "#HL2_Chapter5_Title" },

	{ "d1_town_", "#HL2_Chapter6_Title" },

	{ "d2_coast_09", "#HL2_Chapter8_Title" },
	{ "d2_coast_1", "#HL2_Chapter8_Title" },
	{ "d2_prison_01", "#HL2_Chapter8_Title" },

	{ "d2_coast_", "#HL2_Chapter7_Title" },

	{ "d2_prison_06", "#HL2_Chapter9a_Title" },
	{ "d2_prison_07", "#HL2_Chapter9a_Title" },
	{ "d2_prison_08", "#HL2_Chapter9a_Title" },

	{ "d2_prison_", "#HL2_Chapter9_Title" },

	{ "d3_c17_01", "#HL2_Chapter9a_Title" },
	{ "d3_c17_09", "#HL2_Chapter11_Title" },
	{ "d3_c17_1", "#HL2_Chapter11_Title" },

	{ "d3_c17_", "#HL2_Chapter10_Title" },

	{ "d3_citadel_", "#HL2_Chapter12_Title" },

	{ "d3_breen_", "#HL2_Chapter13_Title" },
	{ "credits", "#HL2_Chapter14_Title" },

	{ "ep1_citadel_00", "#episodic_Chapter1_Title" },
	{ "ep1_citadel_01", "#episodic_Chapter1_Title" },
	{ "ep1_citadel_02b", "#episodic_Chapter1_Title" },
	{ "ep1_citadel_02", "#episodic_Chapter1_Title" },
	{ "ep1_citadel_03", "#episodic_Chapter2_Title" },
	{ "ep1_citadel_04", "#episodic_Chapter2_Title" },
	{ "ep1_c17_00a", "#episodic_Chapter3_Title" },
	{ "ep1_c17_00", "#episodic_Chapter3_Title" },
	{ "ep1_c17_01", "#episodic_Chapter4_Title" },
	{ "ep1_c17_02b", "#episodic_Chapter4_Title" },
	{ "ep1_c17_02", "#episodic_Chapter4_Title" },
	{ "ep1_c17_05", "#episodic_Chapter5_Title" },
	{ "ep1_c17_06", "#episodic_Chapter5_Title" },

	{ "ep2_outland_01a", "#ep2_Chapter1_Title" },
	{ "ep2_outland_01", "#ep2_Chapter1_Title" },
	{ "ep2_outland_02", "#ep2_Chapter2_Title" },
	{ "ep2_outland_03", "#ep2_Chapter2_Title" },
	{ "ep2_outland_04", "#ep2_Chapter2_Title" },
	{ "ep2_outland_05", "#ep2_Chapter3_Title" },
	
	{ "ep2_outland_06a", "#ep2_Chapter4_Title" },
	{ "ep2_outland_06", "#ep2_Chapter3_Title" },

	{ "ep2_outland_07", "#ep2_Chapter4_Title" },
	{ "ep2_outland_08", "#ep2_Chapter4_Title" },
	{ "ep2_outland_09", "#ep2_Chapter5_Title" },
	
	{ "ep2_outland_10a", "#ep2_Chapter5_Title" },
	{ "ep2_outland_10", "#ep2_Chapter5_Title" },

	{ "ep2_outland_11a", "#ep2_Chapter6_Title" },
	{ "ep2_outland_11", "#ep2_Chapter6_Title" },
	
	{ "ep2_outland_12a", "#ep2_Chapter7_Title" },
	{ "ep2_outland_12", "#ep2_Chapter6_Title" },
#endif
};

#ifdef _XBOX
void CServerGameDLL::GetTitleName( const char *pMapName, char* pTitleBuff, int titleBuffSize )
{
	// Try to find a matching title comment for this mapname
	for ( int i = 0; i < ARRAYSIZE(gTitleComments); i++ )
	{
		if ( !Q_strnicmp( pMapName, gTitleComments[i].pBSPName, strlen(gTitleComments[i].pBSPName) ) )
		{
			Q_strncpy( pTitleBuff, gTitleComments[i].pTitleName, titleBuffSize );
			return;
		}
	}
	Q_strncpy( pTitleBuff, pMapName, titleBuffSize );
}
#endif

void CServerGameDLL::GetSaveComment( char *text, int maxlength, float flMinutes, float flSeconds, bool bNoTime )
{
	char comment[64];
	const char	*pName;
	int		i;

	char const *mapname = STRING( gpGlobals->mapname );

	pName = NULL;

	// Try to find a matching title comment for this mapname
	for ( i = 0; i < ARRAYSIZE(gTitleComments) && !pName; i++ )
	{
		if ( !Q_strnicmp( mapname, gTitleComments[i].pBSPName, strlen(gTitleComments[i].pBSPName) ) )
		{
			// found one
			int j;

			// Got a message, post-process it to be save name friendly
			Q_strncpy( comment, gTitleComments[i].pTitleName, sizeof( comment ) );
			pName = comment;
			j = 0;
			// Strip out CRs
			while ( j < 64 && comment[j] )
			{
				if ( comment[j] == '\n' || comment[j] == '\r' )
					comment[j] = 0;
				else
					j++;
			}
			break;
		}
	}
	
	// If we didn't get one, use the designer's map name, or the BSP name itself
	if ( !pName )
	{
		pName = mapname;
	}

	if ( bNoTime )
	{
		Q_snprintf( text, maxlength, "%-64.64s", pName );
	}
	else
	{
		int minutes = flMinutes;
		int seconds = flSeconds;

		// Wow, this guy/gal must suck...!
		if ( minutes >= 1000 )
		{
			minutes = 999;
			seconds = 59;
		}

		int minutesAdd = ( seconds / 60 );
		seconds %= 60;

		// add the elapsed time at the end of the comment, for the ui to parse out
		Q_snprintf( text, maxlength, "%-64.64s %03d:%02d", pName, (minutes + minutesAdd), seconds );
	}
}

void CServerGameDLL::WriteSaveHeaders( CSaveRestoreData *s )
{
	CSave saveHelper( s );
	g_pGameSaveRestoreBlockSet->WriteSaveHeaders( &saveHelper );
	g_pGameSaveRestoreBlockSet->PostSave();
}

void CServerGameDLL::ReadRestoreHeaders( CSaveRestoreData *s )
{
	CRestore restoreHelper( s );
	g_pGameSaveRestoreBlockSet->PreRestore();
	g_pGameSaveRestoreBlockSet->ReadRestoreHeaders( &restoreHelper );
}

void CServerGameDLL::PreSaveGameLoaded( char const *pSaveName, bool bInGame )
{
	gamestats->Event_PreSaveGameLoaded( pSaveName, bInGame );
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the game DLL wants the server not to be made public.
//			Used by commentary system to hide multiplayer commentary servers from the master.
//-----------------------------------------------------------------------------
bool CServerGameDLL::ShouldHideServer( void )
{
	if ( g_pcv_commentary && g_pcv_commentary->GetBool() )
		return true;

	if ( g_pcv_hideServer && g_pcv_hideServer->GetBool() )
		return true;

	if ( gpGlobals->eLoadType == MapLoad_Background )
		return true;

	#if defined( TF_DLL )
		if ( GTFGCClientSystem()->ShouldHideServer() )
			return true;
	#endif
	return false;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CServerGameDLL::InvalidateMdlCache()
{
	CBaseAnimating *pAnimating;
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		pAnimating = dynamic_cast<CBaseAnimating *>(pEntity);
		if ( pAnimating )
		{
			pAnimating->InvalidateMdlCache();
		}
	}
}

// interface to the new GC based lobby system
IServerGCLobby *CServerGameDLL::GetServerGCLobby()
{
#ifdef TF_DLL
	return GTFGCClientSystem();
#else	
	return NULL;
#endif
}


void CServerGameDLL::SetServerHibernation( bool bHibernating )
{
	m_bIsHibernating = bHibernating;

#ifdef INFESTED_DLL
	if ( engine && engine->IsDedicatedServer() && m_bIsHibernating && ASWGameRules() )
	{
		ASWGameRules()->OnServerHibernating();
	}
#endif

#ifdef TF_DLL
	GTFGCClientSystem()->SetHibernation( bHibernating );
#endif
}

const char *CServerGameDLL::GetServerBrowserMapOverride()
{
#ifdef TF_DLL
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		const char *pszFilenameShort = g_pPopulationManager ? g_pPopulationManager->GetPopulationFilenameShort() : NULL;
		if ( pszFilenameShort && pszFilenameShort[0] )
		{
			return pszFilenameShort;
		}
	}

	static char maptmp[256];
	return GetCleanMapName( STRING( gpGlobals->mapname ), maptmp );
#endif
	return NULL;
}

const char *CServerGameDLL::GetServerBrowserGameData()
{
	CUtlString sResult;

#ifdef TF_DLL
	sResult.Format( "tf_mm_trusted:%d,tf_mm_servermode:%d", tf_mm_trusted.GetInt(), tf_mm_servermode.GetInt() );

	CMatchInfo *pMatch = GTFGCClientSystem()->GetMatch();
	if ( !pMatch )
	{
		sResult.Append( ",lobby:0" );
	}
	else
	{
		sResult.Append( CFmtStr( ",lobby:%016llx", pMatch->m_nLobbyID ) );
	}
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		bool bMannup = pMatch && pMatch->m_eMatchGroup == k_eTFMatchGroup_MvM_MannUp;
		sResult.Append( CFmtStr( ",mannup:%d", (int)bMannup ) );
	}
#endif

	static char rchResult[2048];
	V_strcpy_safe( rchResult, sResult );
	return rchResult;
}

//-----------------------------------------------------------------------------
void CServerGameDLL::Status( void (*print) (const char *fmt, ...) )
{
	if ( g_pGameRules )
	{
		g_pGameRules->Status( print );
	}
}

//-----------------------------------------------------------------------------
void CServerGameDLL::PrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
                                            /* in/out */ char *pszMapFile, size_t nMapFileSize )
{
#ifdef TF_DLL
	TFMapsWorkshop()->PrepareLevelResources( pszMapName, nMapNameSize, pszMapFile, nMapFileSize );
#endif // TF_DLL
}

//-----------------------------------------------------------------------------
IServerGameDLL::ePrepareLevelResourcesResult
CServerGameDLL::AsyncPrepareLevelResources( /* in/out */ char *pszMapName, size_t nMapNameSize,
                                            /* in/out */ char *pszMapFile, size_t nMapFileSize,
                                            float *flProgress /* = NULL */ )
{
#ifdef TF_DLL
	return TFMapsWorkshop()->AsyncPrepareLevelResources( pszMapName, nMapNameSize, pszMapFile, nMapFileSize, flProgress );
#endif // TF_DLL

	if ( flProgress )
	{
		*flProgress = 1.f;
	}
	return IServerGameDLL::ePrepareLevelResources_Prepared;
}

//-----------------------------------------------------------------------------
IServerGameDLL::eCanProvideLevelResult CServerGameDLL::CanProvideLevel( /* in/out */ char *pMapName, int nMapNameMax )
{
#ifdef TF_DLL
	return TFMapsWorkshop()->OnCanProvideLevel( pMapName, nMapNameMax );
#endif // TF_DLL
	return IServerGameDLL::eCanProvideLevel_CannotProvide;
}

//-----------------------------------------------------------------------------
bool CServerGameDLL::IsManualMapChangeOkay( const char **pszReason )
{
	if ( GameRules() )
	{
		return GameRules()->IsManualMapChangeOkay( pszReason );
	}

	return true;
}

//-----------------------------------------------------------------------------
bool CServerGameDLL::GetWorkshopMap( uint32 uIndex, WorkshopMapDesc_t *pDesc )
{
#ifdef TF_DLL
	return TFMapsWorkshop()->GetWorkshopMapDesc( uIndex, pDesc );
#endif // TF_DLL
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Called during a transition, to build a map adjacency list
//-----------------------------------------------------------------------------
void CServerGameDLL::BuildAdjacentMapList( void )
{
	// retrieve the pointer to the save data
	CSaveRestoreData *pSaveData = gpGlobals->pSaveData;

	if ( pSaveData )
		pSaveData->levelInfo.connectionCount = BuildChangeList( pSaveData->levelInfo.levelList, MAX_LEVEL_CONNECTIONS );
}

//-----------------------------------------------------------------------------
// Purpose: Sanity-check to verify that a path is a relative path inside the game dir
// Taken From: engine/cmd.cpp
//-----------------------------------------------------------------------------
static bool IsValidPath( const char *pszFilename )
{
	if ( !pszFilename )
	{
		return false;
	}

	if ( Q_strlen( pszFilename ) <= 0    ||
		 Q_IsAbsolutePath( pszFilename ) || // to protect absolute paths
		 Q_strstr( pszFilename, ".." ) )    // to protect relative paths
	{
		return false;
	}

	return true;
}

static void ValidateMOTDFilename( IConVar *pConVar, const char *oldValue, float flOldValue )
{
	ConVarRef var( pConVar );
	if ( !IsValidPath( var.GetString() ) )
	{
		var.SetValue( var.GetDefault() );
	}
}

static ConVar motdfile( "motdfile", "motd.txt", 0, "The MOTD file to load.", ValidateMOTDFilename );
static ConVar motdfile_text( "motdfile_text", "motd_text.txt", 0, "The text-only MOTD file to use for clients that have disabled HTML MOTDs.", ValidateMOTDFilename );
void CServerGameDLL::LoadMessageOfTheDay()
{
	LoadSpecificMOTDMsg( motdfile, "motd" );
	LoadSpecificMOTDMsg( motdfile_text, "motd_text" );
}

void CServerGameDLL::LoadSpecificMOTDMsg( const ConVar &convar, const char *pszStringName )
{
#ifndef _XBOX
	CUtlBuffer buf;

	// Generate preferred filename, which is in the cfg folder.
	char szPreferredFilename[ MAX_PATH ];
	V_sprintf_safe( szPreferredFilename, "cfg/%s", convar.GetString() );

	// Check the preferred filename first
	char szResolvedFilename[ MAX_PATH ];
	V_strcpy_safe( szResolvedFilename, szPreferredFilename );
	bool bFound = filesystem->ReadFile( szResolvedFilename, "GAME", buf );

	// Not found?  Try in the root, which is the old place it used to go.
	if ( !bFound )
	{

		V_strcpy_safe( szResolvedFilename, convar.GetString() );
		bFound = filesystem->ReadFile( szResolvedFilename, "GAME", buf );
	}

	// Still not found?  See if we can try the default.
	if ( !bFound && !V_stricmp( convar.GetString(), convar.GetDefault() ) )
	{
		V_strcpy_safe( szResolvedFilename, szPreferredFilename );
		char *dotTxt = V_stristr( szResolvedFilename, ".txt" );
		Assert ( dotTxt != NULL );
		if ( dotTxt ) V_strcpy( dotTxt, "_default.txt" );
		bFound = filesystem->ReadFile( szResolvedFilename, "GAME", buf );
	}

	if ( !bFound )
	{
		Msg( "'%s' not found; not loaded\n", szPreferredFilename );
		return;
	}

	if ( buf.TellPut() > 2048 )
	{
		Warning("'%s' is too big; not loaded\n", szResolvedFilename );
		return;
	}
	buf.PutChar( '\0' );

	if ( V_stricmp( szPreferredFilename, szResolvedFilename ) == 0)
	{
		Msg( "Set %s from file '%s'\n", pszStringName, szResolvedFilename );
	}
	else
	{
		Msg( "Set %s from file '%s'.  ('%s' was not found.)\n", pszStringName, szResolvedFilename, szPreferredFilename );
	}

	g_pStringTableInfoPanel->AddString( CBaseEntity::IsServer(), pszStringName, buf.TellPut(), buf.Base() );
#endif
}

// keeps track of which chapters the user has unlocked
ConVar sv_unlockedchapters( "sv_unlockedchapters", "1", FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

//-----------------------------------------------------------------------------
// Purpose: Updates which chapters are unlocked
//-----------------------------------------------------------------------------
void UpdateChapterRestrictions( const char *mapname )
{
	// look at the chapter for this map
	char chapterTitle[64];
	chapterTitle[0] = 0;
	for ( int i = 0; i < ARRAYSIZE(gTitleComments); i++ )
	{
		if ( !Q_strnicmp( mapname, gTitleComments[i].pBSPName, strlen(gTitleComments[i].pBSPName) ) )
		{
			// found
			Q_strncpy( chapterTitle, gTitleComments[i].pTitleName, sizeof( chapterTitle ) );
			int j = 0;
			while ( j < 64 && chapterTitle[j] )
			{
				if ( chapterTitle[j] == '\n' || chapterTitle[j] == '\r' )
					chapterTitle[j] = 0;
				else
					j++;
			}

			break;
		}
	}

	if ( !chapterTitle[0] )
		return;

	// make sure the specified chapter title is unlocked
	strlwr( chapterTitle );
	
	// Get our active mod directory name
	char modDir[MAX_PATH];
	if ( UTIL_GetModDir( modDir, sizeof(modDir) ) == false )
		return;

	char chapterNumberPrefix[64];
	Q_snprintf(chapterNumberPrefix, sizeof(chapterNumberPrefix), "#%s_chapter", modDir);

	const char *newChapterNumber = strstr( chapterTitle, chapterNumberPrefix );
	if ( newChapterNumber )
	{
		// cut off the front
		newChapterNumber += strlen( chapterNumberPrefix );
		char newChapter[32];
		Q_strncpy( newChapter, newChapterNumber, sizeof(newChapter) );

		// cut off the end
		char *end = strstr( newChapter, "_title" );
		if ( end )
		{
			*end = 0;
		}

		int nNewChapter = atoi( newChapter );

		// HACK: HL2 added a zany chapter "9a" which wreaks
		//       havoc in this stupid atoi-based chapter code.
		if ( !Q_stricmp( modDir, "hl2" ) )
		{
			if ( !Q_stricmp( newChapter, "9a" ) )
			{
				nNewChapter = 10;
			}
			else if ( nNewChapter > 9 )
			{
				nNewChapter++;
			}
		}

		// ok we have the string, see if it's newer
		const char *unlockedChapter = sv_unlockedchapters.GetString();
		int nUnlockedChapter = atoi( unlockedChapter );

		if ( nUnlockedChapter < nNewChapter )
		{
			// ok we're at a higher chapter, unlock
			sv_unlockedchapters.SetValue( nNewChapter );

			// HACK: Call up through a better function than this? 7/23/07 - jdw
			if ( IsX360() )
			{
				engine->ServerCommand( "host_writeconfig\n" );
			}
		}

		g_nCurrentChapterIndex = nNewChapter;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update xbox live data for the user's presence
//-----------------------------------------------------------------------------
void UpdateRichPresence ( void )
{
	// This assumes we're playing a single player game
	Assert ( gpGlobals->maxClients == 1 );

	// Shouldn't get here unless we're playing a map and we've updated sv_unlockedchapters
	Assert ( g_nCurrentChapterIndex >= 0 );

	// Get our active mod directory name
	char modDir[MAX_PATH];
	if ( UTIL_GetModDir( modDir, sizeof(modDir) ) == false )
		return;

	// Get presence data based on the game we're playing
	uint iGameID, iChapterIndex, iChapterID, iGamePresenceID;
	iGameID = iChapterIndex = iChapterID = iGamePresenceID = 0;
	if ( Q_stristr( modDir, "hl2" ) )
	{
		iGameID			= CONTEXT_GAME_GAME_HALF_LIFE_2;
		iChapterID		= CONTEXT_CHAPTER_HL2;
		iChapterIndex	= g_nCurrentChapterIndex - 1;
		iGamePresenceID = CONTEXT_PRESENCE_HL2_INGAME;
	}
	else if ( Q_stristr( modDir, "episodic" ) )
	{
		iGameID			= CONTEXT_GAME_GAME_EPISODE_ONE;
		iChapterID		= CONTEXT_CHAPTER_EP1;
		iChapterIndex	= g_nCurrentChapterIndex - 1;
		iGamePresenceID = CONTEXT_PRESENCE_EP1_INGAME;
	}
	else if ( Q_stristr( modDir, "ep2" ) )
	{
		iGameID			= CONTEXT_GAME_GAME_EPISODE_TWO;
		iChapterID		= CONTEXT_CHAPTER_EP2;
		iChapterIndex	= g_nCurrentChapterIndex - 1;
		iGamePresenceID = CONTEXT_PRESENCE_EP2_INGAME;
	}
	else if ( Q_stristr( modDir, "portal" ) )
	{
		iGameID			= CONTEXT_GAME_GAME_PORTAL;
		iChapterID		= CONTEXT_CHAPTER_PORTAL;
		iChapterIndex	= g_nCurrentChapterIndex - 1;
		iGamePresenceID = CONTEXT_PRESENCE_PORTAL_INGAME;
	}
	else
	{
		Warning( "UpdateRichPresence failed in GameInterface. Didn't recognize -game parameter." );
	}

#if defined( _X360 )

	// Set chapter context based on mapname
	if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), iChapterID, iChapterIndex, true ) )
	{
		Warning( "GameInterface: UserSetContext failed.\n" );
	}

	if ( commentary.GetBool() )
	{
		// Set presence to show the user is playing developer commentary
		if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_COMMENTARY, true ) )
		{
			Warning( "GameInterface: UserSetContext failed.\n" );
		}
	}
	else
	{
		// Set presence to show the user is in-game
		if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_PRESENCE, iGamePresenceID, true ) )
		{
			Warning( "GameInterface: UserSetContext failed.\n" );
		}
	}
	
	// Set which game the user is playing
	if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), CONTEXT_GAME, iGameID, true ) )
	{
		Warning( "GameInterface: UserSetContext failed.\n" );
	}

	if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD, true ) )
	{
		Warning( "GameInterface: UserSetContext failed.\n" );
	}

	if ( !xboxsystem->UserSetContext( XBX_GetPrimaryUserId(), X_CONTEXT_GAME_MODE, CONTEXT_GAME_MODE_SINGLEPLAYER, true ) )
	{
		Warning( "GameInterface: UserSetContext failed.\n" );
	}
#endif
}

//-----------------------------------------------------------------------------
// Precaches a vgui screen overlay material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName )
{
	Assert( pMaterialName && pMaterialName[0] );
	g_pStringTableMaterials->AddString( CBaseEntity::IsServer(), pMaterialName );
}


//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName )
{
	if (pMaterialName)
	{
		int nIndex = g_pStringTableMaterials->FindStringIndex( pMaterialName );
		
		if (nIndex != INVALID_STRING_INDEX )
		{
			return nIndex;
		}
		else
		{
			DevMsg("Warning! GetMaterialIndex: couldn't find material %s\n ", pMaterialName );
			return 0;
		}
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nMaterialIndex )
{
	return g_pStringTableMaterials->GetString( nMaterialIndex );
}


//-----------------------------------------------------------------------------
// Precaches a vgui screen overlay material
//-----------------------------------------------------------------------------
void PrecacheParticleSystem( const char *pParticleSystemName )
{
	Assert( pParticleSystemName && pParticleSystemName[0] );
	g_pStringTableParticleEffectNames->AddString( CBaseEntity::IsServer(), pParticleSystemName );
}


//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetParticleSystemIndex( const char *pParticleSystemName )
{
	if ( pParticleSystemName )
	{
		int nIndex = g_pStringTableParticleEffectNames->FindStringIndex( pParticleSystemName );
		if (nIndex != INVALID_STRING_INDEX )
			return nIndex;

		DevWarning("Server: Missing precache for particle system \"%s\"!\n", pParticleSystemName );
	}

	// This is the invalid string index
	return 0;
}

//-----------------------------------------------------------------------------
// Converts a previously precached material index into a string
//-----------------------------------------------------------------------------
const char *GetParticleSystemNameFromIndex( int nMaterialIndex )
{
	if ( nMaterialIndex < g_pStringTableParticleEffectNames->GetMaxStrings() )
		return g_pStringTableParticleEffectNames->GetString( nMaterialIndex );
	return "error";
}

//-----------------------------------------------------------------------------
// Returns true if host_thread_mode is set to non-zero (and engine is running in threaded mode)
//-----------------------------------------------------------------------------
bool IsEngineThreaded()
{
	if ( g_pcv_ThreadMode )
	{
		return g_pcv_ThreadMode->GetBool();
	}
	return false;
}

class CServerGameEnts : public IServerGameEnts
{
public:
	virtual void			SetDebugEdictBase(edict_t *base);
	virtual void			MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 );
	virtual void			FreeContainingEntity( edict_t * ); 
	virtual edict_t*		BaseEntityToEdict( CBaseEntity *pEnt );
	virtual CBaseEntity*	EdictToBaseEntity( edict_t *pEdict );
	virtual void			CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts );
};
EXPOSE_SINGLE_INTERFACE(CServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMEENTS);

void CServerGameEnts::SetDebugEdictBase(edict_t *base)
{
	g_pDebugEdictBase = base;
}

//-----------------------------------------------------------------------------
// Purpose: Marks entities as touching
// Input  : *e1 - 
//			*e2 - 
//-----------------------------------------------------------------------------
void CServerGameEnts::MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 )
{
	CBaseEntity *entity = GetContainingEntity( e1 );
	CBaseEntity *entityTouched = GetContainingEntity( e2 );
	if ( entity && entityTouched )
	{
		// HACKHACK: UNDONE: Pass in the trace here??!?!?
		trace_t tr;
		UTIL_ClearTrace( tr );
		tr.endpos = (entity->GetAbsOrigin() + entityTouched->GetAbsOrigin()) * 0.5;
		entity->PhysicsMarkEntitiesAsTouching( entityTouched, tr );
	}
}

void CServerGameEnts::FreeContainingEntity( edict_t *e )
{
	::FreeContainingEntity(e);
}

edict_t* CServerGameEnts::BaseEntityToEdict( CBaseEntity *pEnt )
{
	if ( pEnt )
		return pEnt->edict();
	else
		return NULL;
}

CBaseEntity* CServerGameEnts::EdictToBaseEntity( edict_t *pEdict )
{
	if ( pEdict )
		return CBaseEntity::Instance( pEdict );
	else
		return NULL;
}


/* Yuck.. ideally this would be in CServerNetworkProperty's header, but it requires CBaseEntity and
// inlining it gives a nice speedup.
inline void CServerNetworkProperty::CheckTransmit( CCheckTransmitInfo *pInfo )
{
	// If we have a transmit proxy, let it hook our ShouldTransmit return value.
	if ( m_pTransmitProxy )
	{
		nShouldTransmit = m_pTransmitProxy->ShouldTransmit( pInfo, nShouldTransmit );
	}

	if ( m_pOuter->ShouldTransmit( pInfo ) )
	{
		m_pOuter->SetTransmit( pInfo );
	}
} */

void CServerGameEnts::CheckTransmit( CCheckTransmitInfo *pInfo, const unsigned short *pEdictIndices, int nEdicts )
{
	// NOTE: for speed's sake, this assumes that all networkables are CBaseEntities and that the edict list
	// is consecutive in memory. If either of these things change, then this routine needs to change, but
	// ideally we won't be calling any virtual from this routine. This speedy routine was added as an
	// optimization which would be nice to keep.
	edict_t *pBaseEdict = engine->PEntityOfEntIndex( 0 );

	// get recipient player's skybox:
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	Assert( pRecipientEntity && pRecipientEntity->IsPlayer() );
	if ( !pRecipientEntity )
		return;
	
	MDLCACHE_CRITICAL_SECTION();
	CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );
	const int skyBoxArea = pRecipientPlayer->m_Local.m_skybox3d.area;

#ifndef _X360
	const bool bIsHLTV = pRecipientPlayer->IsHLTV();
	const bool bIsReplay = pRecipientPlayer->IsReplay();

	// m_pTransmitAlways must be set if HLTV client
	Assert( bIsHLTV == ( pInfo->m_pTransmitAlways != NULL) ||
		    bIsReplay == ( pInfo->m_pTransmitAlways != NULL) );
#endif

	for ( int i=0; i < nEdicts; i++ )
	{
		int iEdict = pEdictIndices[i];

		edict_t *pEdict = &pBaseEdict[iEdict];
		Assert( pEdict == engine->PEntityOfEntIndex( iEdict ) );
		int nFlags = pEdict->m_fStateFlags & (FL_EDICT_DONTSEND|FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_FULLCHECK);

		// entity needs no transmit
		if ( nFlags & FL_EDICT_DONTSEND )
			continue;
		
		// entity is already marked for sending
		if ( pInfo->m_pTransmitEdict->Get( iEdict ) )
			continue;
		
		if ( nFlags & FL_EDICT_ALWAYS )
		{
			// FIXME: Hey! Shouldn't this be using SetTransmit so as 
			// to also force network down dependent entities?
			while ( true )
			{
				// mark entity for sending
				pInfo->m_pTransmitEdict->Set( iEdict );
	
#ifndef _X360
				if ( bIsHLTV || bIsReplay )
				{
					pInfo->m_pTransmitAlways->Set( iEdict );
				}
#endif	
				CServerNetworkProperty *pEnt = static_cast<CServerNetworkProperty*>( pEdict->GetNetworkable() );
				if ( !pEnt )
					break;

				CServerNetworkProperty *pParent = pEnt->GetNetworkParent();
				if ( !pParent )
					break;

				pEdict = pParent->edict();
				iEdict = pParent->entindex();
			}
			continue;
		}

		// FIXME: Would like to remove all dependencies
		CBaseEntity *pEnt = ( CBaseEntity * )pEdict->GetUnknown();
		Assert( dynamic_cast< CBaseEntity* >( pEdict->GetUnknown() ) == pEnt );

		if ( nFlags == FL_EDICT_FULLCHECK )
		{
			// do a full ShouldTransmit() check, may return FL_EDICT_CHECKPVS
			nFlags = pEnt->ShouldTransmit( pInfo );

			Assert( !(nFlags & FL_EDICT_FULLCHECK) );

			if ( nFlags & FL_EDICT_ALWAYS )
			{
				pEnt->SetTransmit( pInfo, true );
				continue;
			}	
		}

		// don't send this entity
		if ( !( nFlags & FL_EDICT_PVSCHECK ) )
			continue;

		CServerNetworkProperty *netProp = static_cast<CServerNetworkProperty*>( pEdict->GetNetworkable() );

#ifndef _X360
		if ( bIsHLTV || bIsReplay )
		{
			// for the HLTV/Replay we don't cull against PVS
			if ( netProp->AreaNum() == skyBoxArea )
			{
				pEnt->SetTransmit( pInfo, true );
			}
			else
			{
				pEnt->SetTransmit( pInfo, false );
			}
			continue;
		}
#endif

		// Always send entities in the player's 3d skybox.
		// Sidenote: call of AreaNum() ensures that PVS data is up to date for this entity
		bool bSameAreaAsSky = netProp->AreaNum() == skyBoxArea;
		if ( bSameAreaAsSky )
		{
			pEnt->SetTransmit( pInfo, true );
			continue;
		}

		bool bInPVS = netProp->IsInPVS( pInfo );
		if ( bInPVS || sv_force_transmit_ents.GetBool() )
		{
			// only send if entity is in PVS
			pEnt->SetTransmit( pInfo, false );
			continue;
		}

		// If the entity is marked "check PVS" but it's in hierarchy, walk up the hierarchy looking for the
		//  for any parent which is also in the PVS.  If none are found, then we don't need to worry about sending ourself
		CBaseEntity *orig = pEnt;
		CServerNetworkProperty *check = netProp->GetNetworkParent();

		// BUG BUG:  I think it might be better to build up a list of edict indices which "depend" on other answers and then
		// resolve them in a second pass.  Not sure what happens if an entity has two parents who both request PVS check?
        while ( check )
		{
			int checkIndex = check->entindex();

			// Parent already being sent
			if ( pInfo->m_pTransmitEdict->Get( checkIndex ) )
			{
				orig->SetTransmit( pInfo, true );
				break;
			}

			edict_t *checkEdict = check->edict();
			int checkFlags = checkEdict->m_fStateFlags & (FL_EDICT_DONTSEND|FL_EDICT_ALWAYS|FL_EDICT_PVSCHECK|FL_EDICT_FULLCHECK);
			if ( checkFlags & FL_EDICT_DONTSEND )
				break;

			if ( checkFlags & FL_EDICT_ALWAYS )
			{
				orig->SetTransmit( pInfo, true );
				break;
			}

			if ( checkFlags == FL_EDICT_FULLCHECK )
			{
				// do a full ShouldTransmit() check, may return FL_EDICT_CHECKPVS
				CBaseEntity *pCheckEntity = check->GetBaseEntity();
				nFlags = pCheckEntity->ShouldTransmit( pInfo );
				Assert( !(nFlags & FL_EDICT_FULLCHECK) );
				if ( nFlags & FL_EDICT_ALWAYS )
				{
					pCheckEntity->SetTransmit( pInfo, true );
					orig->SetTransmit( pInfo, true );
				}
				break;
			}

			if ( checkFlags & FL_EDICT_PVSCHECK )
			{
				// Check pvs
				check->RecomputePVSInformation();
				bool bMoveParentInPVS = check->IsInPVS( pInfo );
				if ( bMoveParentInPVS )
				{
					orig->SetTransmit( pInfo, true );
					break;
				}
			}

			// Continue up chain just in case the parent itself has a parent that's in the PVS...
			check = check->GetNetworkParent();
		}
	}

//	Msg("A:%i, N:%i, F: %i, P: %i\n", always, dontSend, fullCheck, PVS );
}


CServerGameClients g_ServerGameClients;
// INTERFACEVERSION_SERVERGAMECLIENTS_VERSION_3 is compatible with the latest since we're only adding things to the end, so expose that as well.
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameClients, IServerGameClients003, INTERFACEVERSION_SERVERGAMECLIENTS_VERSION_3, g_ServerGameClients );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameClients, IServerGameClients004, INTERFACEVERSION_SERVERGAMECLIENTS_VERSION_4, g_ServerGameClients );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS, g_ServerGameClients );


//-----------------------------------------------------------------------------
// Purpose: called when a player tries to connect to the server
// Input  : *pEdict - the new player
//			char *pszName - the players name
//			char *pszAddress - the IP address of the player
//			reject - output - fill in with the reason why
//			maxrejectlen -- sizeof output buffer
//			the player was not allowed to connect.
// Output : Returns TRUE if player is allowed to join, FALSE if connection is denied.
//-----------------------------------------------------------------------------
bool CServerGameClients::ClientConnect( edict_t *pEdict, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{	
	if ( !g_pGameRules )
		return false;
	
	return g_pGameRules->ClientConnected( pEdict, pszName, pszAddress, reject, maxrejectlen );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a player is fully active (i.e. ready to receive messages)
// Input  : *pEntity - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientActive( edict_t *pEdict, bool bLoadGame )
{
	MDLCACHE_CRITICAL_SECTION();
	
	::ClientActive( pEdict, bLoadGame );

	// If we just loaded from a save file, call OnRestore on valid entities
	EndRestoreEntities();

	if ( gpGlobals->eLoadType != MapLoad_LoadGame )
	{
		// notify all entities that the player is now in the game
		for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
		{
			pEntity->PostClientActive();
		}
	}

	// Tell the sound controller to check looping sounds
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	CSoundEnvelopeController::GetController().CheckLoopingSoundsForPlayer( pPlayer );
	SceneManager_ClientActive( pPlayer );

	#if defined( TF_DLL )
		Assert( pPlayer );
		if ( pPlayer && !pPlayer->IsFakeClient() && !pPlayer->IsHLTV() && !pPlayer->IsReplay() )
		{
			CSteamID steamID;
			if ( pPlayer->GetSteamID( &steamID ) )
			{
				GTFGCClientSystem()->ClientActive( steamID );
			}
			else
			{
				if ( !pPlayer->IsReplay() && !pPlayer->IsHLTV() )
				{
					Log("WARNING: ClientActive, but we don't know his SteamID?\n");
				}
			}
		}
	#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSpawned( edict_t *pPlayer )
{
	if ( g_pGameRules )
	{
		g_pGameRules->ClientSpawned( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when a player disconnects from a server
// Input  : *pEdict - the player
//-----------------------------------------------------------------------------
void CServerGameClients::ClientDisconnect( edict_t *pEdict )
{
	extern bool	g_fGameOver;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if ( player )
	{
		if ( !g_fGameOver )
		{
			player->SetMaxSpeed( 0.0f );

			CSound *pSound;
			pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEdict ) );
			{
				// since this client isn't around to think anymore, reset their sound. 
				if ( pSound )
				{
					pSound->Reset();
				}
			}

		// since the edict doesn't get deleted, fix it so it doesn't interfere.
			player->RemoveFlag( FL_AIMTARGET ); // don't attract autoaim
			player->AddFlag( FL_DONTTOUCH );	// stop it touching anything
			player->AddFlag( FL_NOTARGET );	// stop NPCs noticing it
			player->AddSolidFlags( FSOLID_NOT_SOLID );		// nonsolid

			if ( g_pGameRules )
			{
				g_pGameRules->ClientDisconnected( pEdict );
				gamestats->Event_PlayerDisconnected( player );
			}
		}

		// Make sure all Untouch()'s are called for this client leaving
		CBaseEntity::PhysicsRemoveTouchedList( player );
		CBaseEntity::PhysicsRemoveGroundList( player );

#if !defined( NO_ENTITY_PREDICTION )
		// Make sure anything we "own" is simulated by the server from now on
		player->ClearPlayerSimulationList();
#endif
		#if defined( TF_DLL )
			if ( !player->IsFakeClient() )
			{
				CSteamID steamID;
				if ( player->GetSteamID( &steamID ) )
				{
					GTFGCClientSystem()->ClientDisconnected( steamID );
				}
				else
				{
					if ( !player->IsReplay() && !player->IsHLTV() )
					{
						Log("WARNING: ClientDisconnected, but we don't know his SteamID?\n");
					}
				}
			}
		#endif
	}
}

void CServerGameClients::ClientPutInServer( edict_t *pEntity, const char *playername )
{
	if ( g_pClientPutInServerOverride )
		g_pClientPutInServerOverride( pEntity, playername );
	else
		::ClientPutInServer( pEntity, playername );
}

void CServerGameClients::ClientCommand( edict_t *pEntity, const CCommand &args )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetContainingEntity( pEntity ) );
	::ClientCommand( pPlayer, args );
}

//-----------------------------------------------------------------------------
// Purpose: called after the player changes userinfo - gives dll a chance to modify 
//			it before it gets sent into the rest of the engine->
// Input  : *pEdict - the player
//			*infobuffer - their infobuffer
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSettingsChanged( edict_t *pEdict )
{
	// Is the client spawned yet?
	if ( !pEdict->GetUnknown() )
		return;

	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );

	if ( !player )
		return;

	player->ClientSettingsChanged();
	g_pGameRules->ClientSettingsChanged( player );
}


#ifdef PORTAL
//-----------------------------------------------------------------------------
// Purpose: Runs CFuncAreaPortalBase::UpdateVisibility on each portal
// Input  : pAreaPortal - The Area portal to test for visibility from portals
// Output : int - 1 if any portal needs this area portal open, 0 otherwise.
//-----------------------------------------------------------------------------
int TestAreaPortalVisibilityThroughPortals ( CFuncAreaPortalBase* pAreaPortal, edict_t *pViewEntity, unsigned char *pvs, int pvssize  )
{
	int iPortalCount = CProp_Portal_Shared::AllPortals.Count();
	if( iPortalCount == 0 )
		return 0;

	CProp_Portal **pPortals = CProp_Portal_Shared::AllPortals.Base();

	for ( int i = 0; i != iPortalCount; ++i )
	{
		CProp_Portal* pLocalPortal = pPortals[ i ];
		if ( pLocalPortal && pLocalPortal->m_bActivated )
		{
			CProp_Portal* pRemotePortal = pLocalPortal->m_hLinkedPortal.Get();

			// Make sure this portal's linked portal is in the PVS before we add what it can see
			if ( pRemotePortal && pRemotePortal->m_bActivated && pRemotePortal->NetworkProp() && 
				pRemotePortal->NetworkProp()->IsInPVS( pViewEntity, pvs, pvssize ) )
			{
				bool bIsOpenOnClient = true;
				float fovDistanceAdjustFactor = 1.0f;
				Vector portalOrg = pLocalPortal->GetAbsOrigin();
				int iPortalNeedsThisPortalOpen = pAreaPortal->UpdateVisibility( portalOrg, fovDistanceAdjustFactor, bIsOpenOnClient );

				// Stop checking on success, this portal needs to be open
				if ( iPortalNeedsThisPortalOpen )
				{
					return iPortalNeedsThisPortalOpen;
				}
			}
		}
	}
	
	return 0;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
//  view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
//  entity's origin is used.  Either is offset by the m_vecViewOffset to get the eye position.
// From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
//  override the actual PAS or PVS values, or use a different origin.
// NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
// Input  : *pViewEntity - 
//			*pClient - 
//			**pvs - 
//			**pas - 
//-----------------------------------------------------------------------------
void CServerGameClients::ClientSetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char *pvs, int pvssize )
{
	Vector org;

	// Reset the PVS!!!
	engine->ResetPVS( pvs, pvssize );

	g_pToolFrameworkServer->PreSetupVisibility();

	// Find the client's PVS
	CBaseEntity *pVE = NULL;
	if ( pViewEntity )
	{
		pVE = GetContainingEntity( pViewEntity );
		// If we have a viewentity, it overrides the player's origin
		if ( pVE )
		{
			org = pVE->EyePosition();
			engine->AddOriginToPVS( org );
		}
	}

	float fovDistanceAdjustFactor = 1;

	CBasePlayer *pPlayer = ( CBasePlayer * )GetContainingEntity( pClient );
	if ( pPlayer )
	{
		org = pPlayer->EyePosition();
		pPlayer->SetupVisibility( pVE, pvs, pvssize );
		UTIL_SetClientVisibilityPVS( pClient, pvs, pvssize );
		fovDistanceAdjustFactor = pPlayer->GetFOVDistanceAdjustFactorForNetworking();
	}

	unsigned char portalBits[MAX_AREA_PORTAL_STATE_BYTES];
	memset( portalBits, 0, sizeof( portalBits ) );
	
	int portalNums[512];
	int isOpen[512];
	int iOutPortal = 0;

	for( unsigned short i = g_AreaPortals.Head(); i != g_AreaPortals.InvalidIndex(); i = g_AreaPortals.Next(i) )
	{
		CFuncAreaPortalBase *pCur = g_AreaPortals[i];

		bool bIsOpenOnClient = true;
		
		// Update our array of which portals are open and flush it if necessary.		
		portalNums[iOutPortal] = pCur->m_portalNumber;
		isOpen[iOutPortal] = pCur->UpdateVisibility( org, fovDistanceAdjustFactor, bIsOpenOnClient );

#ifdef PORTAL
		// If the client doesn't need this open, test if portals might need this area portal open
		if ( isOpen[iOutPortal] == 0 )
		{
			isOpen[iOutPortal] = TestAreaPortalVisibilityThroughPortals( pCur, pViewEntity, pvs, pvssize );
		}
#endif

		++iOutPortal;
		if ( iOutPortal >= ARRAYSIZE( portalNums ) )
		{
			engine->SetAreaPortalStates( portalNums, isOpen, iOutPortal );
			iOutPortal = 0;
		}

		// Version 0 portals (ie: shipping Half-Life 2 era) are always treated as open
		// for purposes of the m_chAreaPortalBits array on the client.
		if ( pCur->m_iPortalVersion == 0 )
			bIsOpenOnClient = true;

		if ( bIsOpenOnClient )
		{
			if ( pCur->m_portalNumber < 0 )
				continue;
			else if ( pCur->m_portalNumber >= sizeof( portalBits ) * 8 )
				Error( "ClientSetupVisibility: portal number (%d) too large", pCur->m_portalNumber );
			else
				portalBits[pCur->m_portalNumber >> 3] |= (1 << (pCur->m_portalNumber & 7));
		}	
	}

	// Flush the remaining areaportal states.
	engine->SetAreaPortalStates( portalNums, isOpen, iOutPortal );

	if ( pPlayer )
	{
		// Update the area bits that get sent to the client.
		pPlayer->m_Local.UpdateAreaBits( pPlayer, portalBits );

#ifdef PORTAL 
		// *After* the player's view has updated its area bits, add on any other areas seen by portals
		CPortal_Player* pPortalPlayer = dynamic_cast<CPortal_Player*>( pPlayer );
		if ( pPortalPlayer )
		{
			pPortalPlayer->UpdatePortalViewAreaBits( pvs, pvssize );
		}
#endif //PORTAL
	}
}




//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
//			*buf - 
//			numcmds - 
//			totalcmds - 
//			dropped_packets - 
//			ignore - 
//			paused - 
// Output : float
//-----------------------------------------------------------------------------
#define CMD_MAXBACKUP 64

static ConVar sv_max_usercmd_move_magnitude( "sv_max_usercmd_move_magnitude", "1000", 0, "Maximum move magnitude that can be requested by client." );	

float CServerGameClients::ProcessUsercmds( edict_t *player, bf_read *buf, int numcmds, int totalcmds,
	int dropped_packets, bool ignore, bool paused )
{
	int				i;
	CUserCmd		*from, *to;

	// We track last three command in case we drop some 
	//  packets but get them back.
	CUserCmd cmds[ CMD_MAXBACKUP ];  

	CUserCmd		cmdNull;  // For delta compression
	
	Assert( numcmds >= 0 );
	Assert( ( totalcmds - numcmds ) >= 0 );

	CBasePlayer *pPlayer = NULL;
	CBaseEntity *pEnt = CBaseEntity::Instance(player);
	if ( pEnt && pEnt->IsPlayer() )
	{
		pPlayer = static_cast< CBasePlayer * >( pEnt );
	}
	// Too many commands?
	if ( totalcmds < 0 || totalcmds >= ( CMD_MAXBACKUP - 1 ) || numcmds < 0 || numcmds > totalcmds )
	{
		const char *name = "unknown";
		if ( pPlayer )
		{
			name = pPlayer->GetPlayerName();
		}

		Msg("CBasePlayer::ProcessUsercmds: too many cmds %i sent for player %s\n", totalcmds, name );
		// FIXME:  Need a way to drop the client from here
		//SV_DropClient ( host_client, false, "CMD_MAXBACKUP hit" );
		buf->SetOverflowFlag();
		return 0.0f;
	}

	// Initialize for reading delta compressed usercmds
	cmdNull.Reset();
	from = &cmdNull;
	for ( i = totalcmds - 1; i >= 0; i-- )
	{
		to = &cmds[ i ];
		ReadUsercmd( buf, to, from );
		from = to;

		if ( ( fabs( to->forwardmove ) > sv_max_usercmd_move_magnitude.GetFloat() ) ||
			( fabs( to->sidemove ) > sv_max_usercmd_move_magnitude.GetFloat() ) ||
			( fabs( to->upmove ) > sv_max_usercmd_move_magnitude.GetFloat() ) )
		{
			to->forwardmove = 0;
			to->sidemove = 0;
			to->upmove = 0;
		}
	}

	// Client not fully connected or server has gone inactive  or is paused, just ignore
	if ( ignore || !pPlayer )
	{
		return 0.0f;
	}

	MDLCACHE_CRITICAL_SECTION();
	pPlayer->ProcessUsercmds( cmds, numcmds, totalcmds, dropped_packets, paused );

	return TICK_INTERVAL;
}


void CServerGameClients::PostClientMessagesSent_DEPRECIATED( void )
{
}

// Sets the client index for the client who typed the command into his/her console
void CServerGameClients::SetCommandClient( int index )
{
	g_nCommandClientIndex = index;
}

int	CServerGameClients::GetReplayDelay( edict_t *pEdict, int &entity )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );

	if ( !pPlayer )
		return 0;

	entity = pPlayer->GetReplayEntity();

	return pPlayer->GetDelayTicks();
}


//-----------------------------------------------------------------------------
// The client's userinfo data lump has changed
//-----------------------------------------------------------------------------
void CServerGameClients::ClientEarPosition( edict_t *pEdict, Vector *pEarOrigin )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if (pPlayer)
	{
		*pEarOrigin = pPlayer->EarPosition();
	}
	else
	{
		// Shouldn't happen
		Assert(0);
		*pEarOrigin = vec3_origin;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *player - 
// Output : CPlayerState
//-----------------------------------------------------------------------------
CPlayerState *CServerGameClients::GetPlayerState( edict_t *player )
{
	// Is the client spawned yet?
	if ( !player || !player->GetUnknown() )
		return NULL;

	CBasePlayer *pBasePlayer = ( CBasePlayer * )CBaseEntity::Instance( player );
	if ( !pBasePlayer )
		return NULL;

	return &pBasePlayer->pl;
}

//-----------------------------------------------------------------------------
// Purpose: Anything this game .dll wants to add to the bug reporter text (e.g., the entity/model under the picker crosshair)
//  can be added here
// Input  : *buf - 
//			buflen - 
//-----------------------------------------------------------------------------
void CServerGameClients::GetBugReportInfo( char *buf, int buflen )
{
	recentNPCSpeech_t speech[ SPEECH_LIST_MAX_SOUNDS ];
	int  num;
	int  i;

	buf[ 0 ] = 0;

	if ( gpGlobals->maxClients == 1 )
	{
		CBaseEntity *ent = FindPickerEntity( UTIL_PlayerByIndex(1) );
		if ( ent )
		{
			Q_snprintf( buf, buflen, "Picker %i/%s - ent %s model %s\n",
				ent->entindex(),
				ent->GetClassname(),
				STRING( ent->GetEntityName() ),
				STRING( ent->GetModelName() ) );
		}

		// get any sounds that were spoken by NPCs recently
		num = GetRecentNPCSpeech( speech );
		if ( num > 0 )
		{
			Q_snprintf( buf, buflen, "%sRecent NPC speech:\n", buf );
			for( i = 0; i < num; i++ )
			{
				Q_snprintf( buf, buflen, "%s   time: %6.3f   sound name: %s   scene: %s\n", buf, speech[ i ].time, speech[ i ].name, speech[ i ].sceneName );
			}
			Q_snprintf( buf, buflen, "%sCurrent time: %6.3f\n", buf, gpGlobals->curtime );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A player sent a voice packet
//-----------------------------------------------------------------------------
void CServerGameClients::ClientVoice( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if (pPlayer)
	{
		pPlayer->OnVoiceTransmit();
		
		// Notify the voice listener that we've spoken
		PlayerVoiceListener().AddPlayerSpeakTime( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: A user has had their network id setup and validated 
//-----------------------------------------------------------------------------
void CServerGameClients::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
}

// The client has submitted a keyvalues command
void CServerGameClients::ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues )
{
	if ( !pKeyValues )
		return;

	if ( g_pGameRules )
	{
		g_pGameRules->ClientCommandKeyValues( pEntity, pKeyValues );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bf_write *g_pMsgBuffer = NULL;

void EntityMessageBegin( CBaseEntity * entity, bool reliable /*= false*/ ) 
{
	Assert( !g_pMsgBuffer );

	Assert ( entity );

	g_pMsgBuffer = engine->EntityMessageBegin( entity->entindex(), entity->GetServerClass(), reliable );
}

void UserMessageBegin( IRecipientFilter& filter, const char *messagename )
{
	Assert( !g_pMsgBuffer );

	Assert( messagename );

	int msg_type = usermessages->LookupUserMessage( messagename );
	
	if ( msg_type == -1 )
	{
		Error( "UserMessageBegin:  Unregistered message '%s'\n", messagename );
	}

	g_pMsgBuffer = engine->UserMessageBegin( &filter, msg_type );
}

void MessageEnd( void )
{
	Assert( g_pMsgBuffer );

	engine->MessageEnd();

	g_pMsgBuffer = NULL;
}

void MessageWriteByte( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_BYTE called with no active message\n" );

	g_pMsgBuffer->WriteByte( iValue );
}

void MessageWriteChar( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_CHAR called with no active message\n" );

	g_pMsgBuffer->WriteChar( iValue );
}

void MessageWriteShort( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WRITE_SHORT called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteWord( int iValue )
{
	if (!g_pMsgBuffer)
		Error( "WRITE_WORD called with no active message\n" );

	g_pMsgBuffer->WriteWord( iValue );
}

void MessageWriteLong( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteLong called with no active message\n" );

	g_pMsgBuffer->WriteLong( iValue );
}

void MessageWriteFloat( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteFloat called with no active message\n" );

	g_pMsgBuffer->WriteFloat( flValue );
}

void MessageWriteAngle( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteAngle called with no active message\n" );

	g_pMsgBuffer->WriteBitAngle( flValue, 8 );
}

void MessageWriteCoord( float flValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteCoord called with no active message\n" );

	g_pMsgBuffer->WriteBitCoord( flValue );
}

void MessageWriteVec3Coord( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Coord called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Coord( rgflValue );
}

void MessageWriteVec3Normal( const Vector& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitVec3Normal( rgflValue );
}

void MessageWriteAngles( const QAngle& rgflValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteVec3Normal called with no active message\n" );

	g_pMsgBuffer->WriteBitAngles( rgflValue );
}

void MessageWriteString( const char *sz )
{
	if (!g_pMsgBuffer)
		Error( "WriteString called with no active message\n" );

	g_pMsgBuffer->WriteString( sz );
}

void MessageWriteEntity( int iValue)
{
	if (!g_pMsgBuffer)
		Error( "WriteEntity called with no active message\n" );

	g_pMsgBuffer->WriteShort( iValue );
}

void MessageWriteEHandle( CBaseEntity *pEntity )
{
	if (!g_pMsgBuffer)
		Error( "WriteEHandle called with no active message\n" );

	long iEncodedEHandle;
	
	if( pEntity )
	{
		EHANDLE hEnt = pEntity;

		int iSerialNum = hEnt.GetSerialNumber() & ( (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1 );
		iEncodedEHandle = hEnt.GetEntryIndex() | (iSerialNum << MAX_EDICT_BITS);
	}
	else
	{
		iEncodedEHandle = INVALID_NETWORKED_EHANDLE_VALUE;
	}
	
	g_pMsgBuffer->WriteLong( iEncodedEHandle );
}

// bitwise
void MessageWriteBool( bool bValue )
{
	if (!g_pMsgBuffer)
		Error( "WriteBool called with no active message\n" );

	g_pMsgBuffer->WriteOneBit( bValue ? 1 : 0 );
}

void MessageWriteUBitLong( unsigned int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteUBitLong called with no active message\n" );

	g_pMsgBuffer->WriteUBitLong( data, numbits );
}

void MessageWriteSBitLong( int data, int numbits )
{
	if (!g_pMsgBuffer)
		Error( "WriteSBitLong called with no active message\n" );

	g_pMsgBuffer->WriteSBitLong( data, numbits );
}

void MessageWriteBits( const void *pIn, int nBits )
{
	if (!g_pMsgBuffer)
		Error( "WriteBits called with no active message\n" );

	g_pMsgBuffer->WriteBits( pIn, nBits );
}

class CServerDLLSharedAppSystems : public IServerDLLSharedAppSystems
{
public:
	CServerDLLSharedAppSystems()
	{
		AddAppSystem( "soundemittersystem" DLL_EXT_STRING, SOUNDEMITTERSYSTEM_INTERFACE_VERSION );
		AddAppSystem( "scenefilecache" DLL_EXT_STRING, SCENE_FILE_CACHE_INTERFACE_VERSION );
	}

	virtual int	Count()
	{
		return m_Systems.Count();
	}
	virtual char const *GetDllName( int idx )
	{
		return m_Systems[ idx ].m_pModuleName;
	}
	virtual char const *GetInterfaceName( int idx )
	{
		return m_Systems[ idx ].m_pInterfaceName;
	}
private:
	void AddAppSystem( char const *moduleName, char const *interfaceName )
	{
		AppSystemInfo_t sys;
		sys.m_pModuleName = moduleName;
		sys.m_pInterfaceName = interfaceName;
		m_Systems.AddToTail( sys );
	}

	CUtlVector< AppSystemInfo_t >	m_Systems;
};

EXPOSE_SINGLE_INTERFACE( CServerDLLSharedAppSystems, IServerDLLSharedAppSystems, SERVER_DLL_SHARED_APPSYSTEMS );


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CServerGameTags::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	if ( pCvarTagList && g_pGameRules )
	{
		g_pGameRules->GetTaggedConVarList( pCvarTagList );
	}
}



#ifndef NO_STEAM

CSteamID GetSteamIDForPlayerIndex( int iPlayerIndex )
{
	const CSteamID *pResult = engine->GetClientSteamIDByPlayerIndex( iPlayerIndex );
	if ( pResult )
		return *pResult;

	// Return a bogus steam ID
	return CSteamID();
}

#endif
