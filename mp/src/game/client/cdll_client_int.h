//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef CDLL_CLIENT_INT_H
#define CDLL_CLIENT_INT_H
#ifdef _WIN32
#pragma once
#endif

#include "iclientnetworkable.h"
#include "utllinkedlist.h"
#include "cdll_int.h"
#include "eiface.h"


class IVModelRender;
class IVEngineClient;
class IVModelRender;
class IVEfx;
class IVRenderView;
class IVDebugOverlay;
class IMaterialSystem;
class IMaterialSystemStub;
class IDataCache;
class IMDLCache;
class IVModelInfoClient;
class IEngineVGui;
class ISpatialPartition;
class IBaseClientDLL;
class ISpatialPartition;
class IFileSystem;
class IStaticPropMgrClient;
class IShadowMgr;
class IEngineSound;
class IMatSystemSurface;
class IMaterialSystemHardwareConfig;
class ISharedGameRules;
class IEngineTrace;
class IGameUIFuncs;
class IGameEventManager2;
class IPhysicsGameTrace;
class CGlobalVarsBase;
class IClientTools;
class C_BaseAnimating;
class IColorCorrectionSystem;
class IInputSystem;
class ISceneFileCache;
class IXboxSystem;	// Xbox 360 only
class IMatchmaking;
class IVideoServices;
class CSteamAPIContext;
class IClientReplayContext;
class IReplayManager;
class IEngineReplay;
class IEngineClientReplay;
class IReplayScreenshotManager;
class CSteamID;

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

class AchievementsAndStatsInterface;

//=============================================================================
// HPE_END
//=============================================================================

extern IVModelRender *modelrender;
extern IVEngineClient	*engine;
extern IVModelRender *modelrender;
extern IVEfx *effects;
extern IVRenderView *render;
extern IVDebugOverlay *debugoverlay;
extern IMaterialSystem *materials;
extern IMaterialSystemStub *materials_stub;
extern IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
extern IDataCache *datacache;
extern IMDLCache *mdlcache;
extern IVModelInfoClient *modelinfo;
extern IEngineVGui *enginevgui;
extern ISpatialPartition* partition;
extern IBaseClientDLL *clientdll;
extern IFileSystem *filesystem;
extern IStaticPropMgrClient *staticpropmgr;
extern IShadowMgr *shadowmgr;
extern IEngineSound *enginesound;
extern IMatSystemSurface *g_pMatSystemSurface;
extern IEngineTrace *enginetrace;
extern IGameUIFuncs *gameuifuncs;
extern IGameEventManager2 *gameeventmanager;
extern IPhysicsGameTrace *physgametrace;
extern CGlobalVarsBase *gpGlobals;
extern IClientTools *clienttools;
extern IInputSystem *inputsystem;
extern ISceneFileCache *scenefilecache;
extern IXboxSystem *xboxsystem;	// Xbox 360 only
extern IMatchmaking *matchmaking;
extern IVideoServices *g_pVideo;
extern IUploadGameStats *gamestatsuploader;
extern CSteamAPIContext *steamapicontext;
extern IReplaySystem *g_pReplay;
extern IClientReplayContext *g_pClientReplayContext;
extern IReplayManager *g_pReplayManager;
extern IReplayScreenshotManager *g_pReplayScreenshotManager;
extern IEngineReplay *g_pEngineReplay;
extern IEngineClientReplay *g_pEngineClientReplay;

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for stats display
//=============================================================================

extern AchievementsAndStatsInterface* g_pAchievementsAndStatsInterface;

//=============================================================================
// HPE_END
//=============================================================================

// Set to true between LevelInit and LevelShutdown.
extern bool	g_bLevelInitialized;
extern bool g_bTextMode;

// Kyle says: this is here to obsfucate our accessing of the g_bTextMode variable and for no other purpose.
//			  See the mess of TF_ANTI_IDLEBOT_VERIFICATION code. If that code doesn't exist anymore, this
//			  probably also doesn't need to exist anymore.
//
//			  On a suggestion from Joe, we also point it to an incomplete type.
extern class IClientPurchaseInterfaceV2 *g_pClientPurchaseInterface;


// Returns true if a new OnDataChanged event is registered for this frame.
bool AddDataChangeEvent( IClientNetworkable *ent, DataUpdateType_t updateType, int *pStoredEvent );

void ClearDataChangedEvent( int iStoredEvent );

//-----------------------------------------------------------------------------
// Precaches a material
//-----------------------------------------------------------------------------
void PrecacheMaterial( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts a previously precached material into an index
//-----------------------------------------------------------------------------
int GetMaterialIndex( const char *pMaterialName );

//-----------------------------------------------------------------------------
// Converts precached material indices into strings
//-----------------------------------------------------------------------------
const char *GetMaterialNameFromIndex( int nIndex );

//-----------------------------------------------------------------------------
// Precache-related methods for particle systems
//-----------------------------------------------------------------------------
void PrecacheParticleSystem( const char *pParticleSystemName );
int GetParticleSystemIndex( const char *pParticleSystemName );
const char *GetParticleSystemNameFromIndex( int nIndex );


//-----------------------------------------------------------------------------
// Called during bone setup to test perf
//-----------------------------------------------------------------------------
void TrackBoneSetupEnt( C_BaseAnimating *pEnt );

bool IsEngineThreaded();

#ifndef NO_STEAM

/// Returns Steam ID, given player index.   Returns an invalid SteamID upon
/// failure
extern CSteamID GetSteamIDForPlayerIndex( int iPlayerIndex );

#endif

#endif // CDLL_CLIENT_INT_H
