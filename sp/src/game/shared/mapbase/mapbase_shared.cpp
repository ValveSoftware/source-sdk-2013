//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Carries the Mapbase CAutoGameSystem that loads manifest among other things.
//			Also includes code that does not fit anywhere else.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tier0/icommandline.h"
#include "tier1/mapbase_con_groups.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>
#include "saverestore_utlvector.h"
#include "props_shared.h"
#include "utlbuffer.h"
#include "usermessages.h"
#ifdef CLIENT_DLL
#include "hud_closecaption.h"
#include "panelmetaclassmgr.h"
#include "c_soundscape.h"
#include "hud_macros.h"
#include "clientmode_shared.h"
#else
#include "soundscape_system.h"
#include "AI_ResponseSystem.h"
#include "mapbase/SystemConvarMod.h"
#include "gameinterface.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GENERIC_MANIFEST_FILE "scripts/mapbase_default_manifest.txt"

#ifdef CLIENT_DLL
#define AUTOLOADED_MANIFEST_FILE VarArgs("maps/%s_manifest.txt", g_MapName)
#else
#define AUTOLOADED_MANIFEST_FILE UTIL_VarArgs("maps/%s_manifest.txt", g_MapName)
#endif

const char *g_MapName;

extern ISoundEmitterSystemBase *soundemitterbase;

ConVar mapbase_load_default_manifest("mapbase_load_default_manifest", "1", FCVAR_ARCHIVE, "Should we automatically load our default manifest file? (\"maps/%mapname%_manifest.txt\")");

#ifdef GAME_DLL
// This constant should change with each Mapbase update
ConVar mapbase_version( "mapbase_version", MAPBASE_VERSION, FCVAR_NONE, "The version of Mapbase currently being used in this mod's server.dll" );

ConVar mapbase_flush_talker("mapbase_flush_talker", "1", FCVAR_NONE, "Normally, when a map with custom talker files is unloaded, the response system resets to rid itself of the custom file(s). Turn this convar off to prevent that from happening.");

extern void MapbaseGameLog_Init();

extern void ParseCustomActbusyFile(const char *file);

extern bool LoadResponseSystemFile(const char *scriptfile);
extern void ReloadResponseSystem();

// Reloads the response system when the map changes to avoid custom talker leaking
static bool g_bMapContainsCustomTalker;
#else
// This constant should change with each Mapbase update
ConVar mapbase_version_client( "mapbase_version_client", MAPBASE_VERSION, FCVAR_NONE, "The version of Mapbase currently being used in this mod's client.dll" );

// This is from the vgui_controls library
extern vgui::HScheme g_iCustomClientSchemeOverride;

bool g_bUsingCustomHudAnimations = false;
bool g_bUsingCustomHudLayout = false;
#endif

extern void AddSurfacepropFile( const char *pFileName, IPhysicsSurfaceProps *pProps, IFileSystem *pFileSystem );

// Indicates this is a core Mapbase mod and not a mod using its code.
static bool g_bMapbaseCore;

// The game's name found in gameinfo.txt. Mostly used for Discord RPC.
char g_iszGameName[128];

#ifdef GAME_DLL
// Default player configuration
char g_szDefaultPlayerModel[MAX_PATH];
bool g_bDefaultPlayerDrawExternally;

char g_szDefaultHandsModel[MAX_PATH];
int g_iDefaultHandsSkin;
int g_iDefaultHandsBody;
#endif

enum
{
	MANIFEST_SOUNDSCRIPTS,
	//MANIFEST_PROPDATA,
	//MANIFEST_SOUNDSCAPES,
	MANIFEST_LOCALIZATION,
	MANIFEST_SURFACEPROPS,
#ifdef CLIENT_DLL
	MANIFEST_CLOSECAPTION,
	MANIFEST_VGUI,
	MANIFEST_CLIENTSCHEME,
	MANIFEST_HUDANIMATIONS,
	MANIFEST_HUDLAYOUT,
#else
	MANIFEST_TALKER,
	//MANIFEST_SENTENCES,
	MANIFEST_ACTBUSY,
#endif
#ifdef MAPBASE_VSCRIPT
	MANIFEST_VSCRIPT,
#endif

	// Must always be kept below
	MANIFEST_NUM_TYPES,
};

struct ManifestType_t
{
	ManifestType_t( const char *_string, const char *cvarname, const char *cvardesc ) : cvar( cvarname, "1", FCVAR_ARCHIVE, cvardesc )
	{
		string = _string;
	}

	//int type;
	const char *string;
	ConVar cvar;
};

#define DECLARE_MANIFEST_TYPE(name, cvar, desc) { #name,		ConVar(#cvar, "1", FCVAR_ARCHIVE, #desc) }

// KEEP THS IN SYNC WITH THE ENUM!
static const ManifestType_t gm_szManifestFileStrings[MANIFEST_NUM_TYPES] = {
	{ "soundscripts",	"mapbase_load_soundscripts",	"Should we load map-specific soundscripts? e.g. \"maps/<mapname>_level_sounds.txt\"" },
	//{ "propdata",		"mapbase_load_propdata",		"Should we load map-specific soundscripts? e.g. \"maps/<mapname>_level_sounds.txt\"" },
	//{ "soundscapes",	"mapbase_load_soundscapes",		"Should we load map-specific soundscapes? e.g. \"maps/<mapname>_soundscapes.txt\"" },
	{ "localization",	"mapbase_load_localization",	"Should we load map-specific localized text files? e.g. \"maps/<mapname>_english.txt\"" },
	{ "surfaceprops",	"mapbase_load_surfaceprops",	"Should we load map-specific surfaceproperties files? e.g. \"maps/<mapname>_surfaceproperties.txt\"" },
#ifdef CLIENT_DLL
	{ "closecaption",	"mapbase_load_closecaption",	"Should we load map-specific closed captioning? e.g. \"maps/<mapname>_closecaption_english.txt\" and \"maps/<mapname>_closecaption_english.dat\"" },
	{ "vgui",			"mapbase_load_vgui",			"Should we load map-specific VGUI screens? e.g. \"maps/<mapname>_screens.txt\"" },
	{ "clientscheme",	"mapbase_load_clientscheme",	"Should we load map-specific ClientScheme.res overrides? e.g. \"maps/<mapname>_clientscheme.res\"" },
	{ "hudanimations",	"mapbase_load_hudanimations",	"Should we load map-specific HUD animation overrides? e.g. \"maps/<mapname>_hudanimations.txt\"" },
	{ "hudlayout",		"mapbase_load_hudlayout",		"Should we load map-specific HUD layout overrides? e.g. \"maps/<mapname>_hudlayout.res\"" },
#else
	{ "talker",			"mapbase_load_talker",			"Should we load map-specific talker files? e.g. \"maps/<mapname>_talker.txt\"" },
	//{ "sentences",	"mapbase_load_sentences",		"Should we load map-specific sentences? e.g. \"maps/<mapname>_sentences.txt\"" },
	{ "actbusy",		"mapbase_load_actbusy",			"Should we load map-specific actbusy files? e.g. \"maps/<mapname>_actbusy.txt\"" },
#endif
#ifdef MAPBASE_VSCRIPT
	{ "vscript",		"mapbase_load_vscript",			"Should we load map-specific VScript map spawn files? e.g. \"maps/<mapname>_mapspawn.nut\"" },
#endif
};

//-----------------------------------------------------------------------------
// Purpose: System used to load map-specific files, etc.
//-----------------------------------------------------------------------------
class CMapbaseSystem : public CAutoGameSystem
{
public:
	DECLARE_DATADESC();

	CMapbaseSystem() : CAutoGameSystem( "CMapbaseSystem" )
	{
	}

	inline bool GetGameInfoKeyValues(KeyValues *pKeyValues)
	{
		return pKeyValues->LoadFromFile( filesystem, "gameinfo.txt", "MOD" );
	}

	virtual bool Init()
	{
		InitConsoleGroups( g_pFullFileSystem );

		// Checks gameinfo.txt for additional command line options
		KeyValues *gameinfo = new KeyValues("GameInfo");
		if (GetGameInfoKeyValues(gameinfo))
		{
			KeyValues *pCommandLineList = gameinfo->FindKey("CommandLine", false);
			if (pCommandLineList)
			{
				for (KeyValues *pKey = pCommandLineList->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey())
				{
					CommandLine()->AppendParm( pKey->GetName(), pKey->GetString() );
				}
			}
		}
		gameinfo->deleteThis();

#ifdef CLIENT_DLL
		InitializeRTs();
#endif

		return true;
	}

	void RefreshCustomTalker()
	{
#ifdef GAME_DLL
		if (g_bMapContainsCustomTalker && mapbase_flush_talker.GetBool())
		{
			CGMsg( 1, CON_GROUP_MAPBASE_MISC, "Mapbase: Reloading response system to flush custom talker\n" );
			ReloadResponseSystem();
			g_bMapContainsCustomTalker = false;
		}
#endif
	}

	virtual void LevelInitPreEntity()
	{
#ifdef GAME_DLL
		CGMsg( 0, CON_GROUP_MAPBASE_MISC, "Mapbase system loaded\n" );
#endif

		// Checks gameinfo.txt for Mapbase-specific options
		KeyValues *gameinfo = new KeyValues("GameInfo");
		if (GetGameInfoKeyValues(gameinfo))
		{
			// Indicates this is a core Mapbase mod and not a mod using its code
			g_bMapbaseCore = gameinfo->GetBool("mapbase_core", false);

			if (!gameinfo->GetBool("hide_mod_name", false))
			{
				// Store the game's name
				const char *pszGameName = gameinfo->GetString("game_rpc", NULL);
				if (pszGameName == NULL)
					pszGameName = gameinfo->GetString("game");

				Q_strncpy(g_iszGameName, pszGameName, sizeof(g_iszGameName));
			}

#ifdef GAME_DLL
			Q_strncpy( g_szDefaultPlayerModel, gameinfo->GetString( "player_default_model", "models/player.mdl" ), sizeof( g_szDefaultPlayerModel ) );
			g_bDefaultPlayerDrawExternally = gameinfo->GetBool( "player_default_draw_externally", false );

			Q_strncpy( g_szDefaultHandsModel, gameinfo->GetString( "player_default_hands", "models/weapons/v_hands.mdl" ), sizeof( g_szDefaultHandsModel ) );
			g_iDefaultHandsSkin = gameinfo->GetInt( "player_default_hands_skin", 0 );
			g_iDefaultHandsBody = gameinfo->GetInt( "player_default_hands_body", 0 );
#endif
		}
		gameinfo->deleteThis();

		RefreshMapName();

		// Shared Mapbase scripts to avoid overwriting mod files
		g_pVGuiLocalize->AddFile( "resource/mapbase_%language%.txt" );
#ifdef CLIENT_DLL
		PanelMetaClassMgr()->LoadMetaClassDefinitionFile( "scripts/vgui_screens_mapbase.txt" );
#endif
	}

	virtual void OnRestore()
	{
		RefreshMapName();
	}

	virtual void LevelInitPostEntity()
	{
		// Check for a generic "mapname_manifest.txt" file and load it.
		if (filesystem->FileExists( AUTOLOADED_MANIFEST_FILE, "GAME" ))
		{
			AddManifestFile( AUTOLOADED_MANIFEST_FILE );
		}
		else
		{
			// Load the generic script instead.
			ParseGenericManifest();
		}

#ifdef GAME_DLL
		MapbaseGameLog_Init();
#endif
	}

	virtual void LevelShutdownPreEntity()
	{
		// How would we make sure they don't last between maps?
		// TODO: Investigate ReloadLocalizationFiles()
		//g_pVGuiLocalize->RemoveAll();
		//g_pVGuiLocalize->ReloadLocalizationFiles();
	}

	virtual void LevelShutdownPostEntity()
	{
		g_MapName = NULL;

		RefreshCustomTalker();

#ifdef CLIENT_DLL
		CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
		FOR_EACH_VEC( m_CloseCaptionFileNames, i )
		{
			hudCloseCaption->RemoveCaptionDictionary( m_CloseCaptionFileNames[i] );
		}
		m_CloseCaptionFileNames.RemoveAll();

		if (g_iCustomClientSchemeOverride != 0 || g_bUsingCustomHudAnimations || g_bUsingCustomHudLayout)
		{
			CGMsg( 1, CON_GROUP_MAPBASE_MISC, "Mapbase: Reloading client mode and viewport scheme\n" );

			// TODO: We currently have no way of actually cleaning up custom schemes upon level unload.
			// That may or may not be sustainable if there's a ton of custom schemes loaded at once
			g_iCustomClientSchemeOverride = 0;

			g_bUsingCustomHudAnimations = false;
			g_bUsingCustomHudLayout = false;

			// Reload scheme
			ClientModeShared *mode = ( ClientModeShared * )GetClientModeNormal();
			if ( mode )
			{
				mode->ReloadScheme();

				// We need to reload default values, so load a special "hudlayout_mapbase.res" file that only contains
				// default Mapbase definitions identical to the defaults in the code
				CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>(g_pClientMode->GetViewport());
				if (pViewport)
				{
					KeyValuesAD pConditions( "conditions" );
					g_pClientMode->ComputeVguiResConditions( pConditions );

					// reload the .res file from disk
					pViewport->LoadControlSettings( "scripts/hudlayout_mapbase.res", NULL, NULL, pConditions );
				}
			}
		}
#endif
	}

	bool RefreshMapName()
	{
#ifdef GAME_DLL
		const char *pszMapName = STRING(gpGlobals->mapname);
#else
		//char mapname[128];
		//Q_StripExtension(MapName(), mapname, sizeof(mapname));
		const char *pszMapName = MapName();
#endif

		if (g_MapName == NULL || !FStrEq(pszMapName, g_MapName))
		{
			g_MapName = pszMapName;
			return true;
		}

		return false;
	}

#ifdef CLIENT_DLL
	//-----------------------------------------------------------------------------
	// Initialize custom RT textures if necessary
	//-----------------------------------------------------------------------------
	void InitializeRTs()
	{
		if (!m_bInitializedRTs)
		{
			int iNumCameras = CommandLine()->ParmValue( "-numcameratextures", 3 );

			materials->BeginRenderTargetAllocation();

			for (int i = 0; i < iNumCameras; i++)
			{
				char szName[32];
				Q_snprintf( szName, sizeof(szName), "_rt_Camera%i", i );

				int iRefIndex = m_CameraTextures.AddToTail();

				//m_CameraTextures[iRefIndex].InitRenderTarget(
				//	256, 256, RT_SIZE_DEFAULT,
				//	g_pMaterialSystem->GetBackBufferFormat(),
				//	MATERIAL_RT_DEPTH_SHARED, true, szName );

				m_CameraTextures[iRefIndex].Init( g_pMaterialSystem->CreateNamedRenderTargetTextureEx2(
					szName,
					256, 256, RT_SIZE_DEFAULT,
					g_pMaterialSystem->GetBackBufferFormat(),
					MATERIAL_RT_DEPTH_SHARED,
					0,
					CREATERENDERTARGETFLAGS_HDR ) );
			}

			materials->EndRenderTargetAllocation();

			m_bInitializedRTs = true;
		}
	}

	void Shutdown()
	{
		if (m_bInitializedRTs)
		{
			for (int i = 0; i < m_CameraTextures.Count(); i++)
			{
				m_CameraTextures[i].Shutdown();
			}
			m_bInitializedRTs = false;
		}
	}
#endif

	// Get a generic, hardcoded manifest with hardcoded names.
	void ParseGenericManifest()
	{
		if (!mapbase_load_default_manifest.GetBool())
			return;

		KeyValues *pKV = new KeyValues("DefaultManifest");
		pKV->LoadFromFile(filesystem, GENERIC_MANIFEST_FILE);

		AddManifestFile(pKV, g_MapName/*, true*/);

		pKV->deleteThis();
	}
	
	void AddManifestFile( const char *file )
	{
		KeyValues *pKV = new KeyValues(file);
		if ( !pKV->LoadFromFile( filesystem, file ) )
		{
			Warning("Mapbase Manifest: \"%s\" is unreadable or missing (can't load KV, check for syntax errors)\n", file);
			pKV->deleteThis();
			return;
		}

		CGMsg( 1, CON_GROUP_MAPBASE_MISC, "===== Mapbase Manifest: Loading manifest file %s =====\n", file );

		AddManifestFile(pKV, g_MapName, false);

		CGMsg( 1, CON_GROUP_MAPBASE_MISC, "==============================================================================\n" );

		pKV->deleteThis();
	}

	void LoadFromValue( const char *value, int type, bool bDontWarn )
	{
		if (type != MANIFEST_VSCRIPT && !filesystem->FileExists(value, "MOD"))
		{
			if (!bDontWarn)
			{
				Warning("Mapbase Manifest: WARNING! \"%s\" does not exist!\n", value);
			}
			return;
		}

		switch (type)
		{
			case MANIFEST_SOUNDSCRIPTS: { soundemitterbase->AddSoundOverrides(value); } break;
			//case MANIFEST_PROPDATA: { g_PropDataSystem.ParsePropDataFile(value); } break;
			case MANIFEST_LOCALIZATION: { g_pVGuiLocalize->AddFile( value, "MOD", true ); } break;
			case MANIFEST_SURFACEPROPS: { AddSurfacepropFile( value, physprops, filesystem ); } break;
#ifdef CLIENT_DLL
			case MANIFEST_CLOSECAPTION: { ManifestLoadCustomCloseCaption( value ); } break;
			case MANIFEST_VGUI:			{ PanelMetaClassMgr()->LoadMetaClassDefinitionFile( value ); } break;
			case MANIFEST_CLIENTSCHEME:	{ ManifestLoadCustomScheme( value ); } break;
			case MANIFEST_HUDANIMATIONS:	{ ManifestLoadCustomHudAnimations( value ); } break;
			case MANIFEST_HUDLAYOUT:	{ ManifestLoadCustomHudLayout( value ); } break;
			//case MANIFEST_SOUNDSCAPES: { Soundscape_AddFile(value); } break;
#else
			case MANIFEST_TALKER: {
					g_bMapContainsCustomTalker = true;
					LoadResponseSystemFile(value); //PrecacheCustomResponseSystem( value );
				} break;
			//case MANIFEST_SOUNDSCAPES: { g_SoundscapeSystem.AddSoundscapeFile(value); } break;
			//case MANIFEST_SENTENCES: { engine->PrecacheSentenceFile(value); } break;
			case MANIFEST_ACTBUSY: { ParseCustomActbusyFile(value); } break;
#endif
#ifdef MAPBASE_VSCRIPT
			case MANIFEST_VSCRIPT:		{ VScriptRunScript(value, false); } break;
#endif
		}
	}

	// This doesn't call deleteThis()!
	void AddManifestFile(KeyValues *pKV, const char *pszMapName, bool bDontWarn = false)
	{
		char value[MAX_PATH];
		const char *name;
		for (KeyValues *pKey = pKV->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey())
		{
			value[0] = '\0';
			name = pKey->GetName();

			// Parse %mapname%, etc.
			bool inparam = false;
			CUtlStringList outStrings;
			V_SplitString( pKey->GetString(), "%", outStrings );
			FOR_EACH_VEC( outStrings, i )
			{
				if (inparam)
				{
					if (FStrEq( outStrings[i], "mapname" ))
					{
						Q_strncat( value, pszMapName, sizeof( value ) );
					}
					else if (FStrEq( outStrings[i], "language" ))
					{
#ifdef CLIENT_DLL
						char uilanguage[64];
						engine->GetUILanguage(uilanguage, sizeof(uilanguage));
						Q_strncat( value, uilanguage, sizeof( value ) );
#else
						// Give up, use English
						Q_strncat( value, "english", sizeof( value ) );
#endif
					}
				}
				else
				{
					Q_strncat( value, outStrings[i], sizeof( value ) );
				}

				inparam = !inparam;
			}

			outStrings.PurgeAndDeleteElements();

			if (FStrEq(name, "NoErrors"))
			{
				bDontWarn = pKey->GetBool();
			}

			for (int i = 0; i < MANIFEST_NUM_TYPES; i++)
			{
				if (FStrEq(name, gm_szManifestFileStrings[i].string))
				{
					if (gm_szManifestFileStrings[i].cvar.GetBool())
					{
						LoadFromValue(value, i, bDontWarn);
					}
					break;
				}
			}
		}
	}

private:

#ifdef CLIENT_DLL
	void ManifestLoadCustomCloseCaption( const char *pszFile )
	{
		if (GET_HUDELEMENT( CHudCloseCaption ))
			(GET_HUDELEMENT( CHudCloseCaption ))->AddCustomCaptionFile( pszFile, m_CloseCaptionFileNames );
	}

	// Custom scheme loading
	void ManifestLoadCustomScheme( const char *pszFile )
	{
		g_iCustomClientSchemeOverride = vgui::scheme()->LoadSchemeFromFile( pszFile, "CustomClientScheme" );

		// Reload scheme
		ClientModeShared *mode = ( ClientModeShared * )GetClientModeNormal();
		if ( mode )
		{
			mode->ReloadScheme();
		}
	}

	void ManifestLoadCustomHudAnimations( const char *pszFile )
	{
		CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>(g_pClientMode->GetViewport());
		if (pViewport)
		{
			g_bUsingCustomHudAnimations = true;
			if (!pViewport->LoadCustomHudAnimations( pszFile ))
			{
				g_bUsingCustomHudAnimations = false;
				CGWarning( 0, CON_GROUP_MAPBASE_MISC, "Custom HUD animations file \"%s\" failed to load\n", pszFile );
				pViewport->ReloadHudAnimations();
			}
			else
			{
				CGMsg( 1, CON_GROUP_MAPBASE_MISC, "Loaded custom HUD animations file \"%s\"\n", pszFile );;
			}
		}
	}

	void ManifestLoadCustomHudLayout( const char *pszFile )
	{
		CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>(g_pClientMode->GetViewport());
		if (pViewport)
		{
			g_bUsingCustomHudLayout = true;

			KeyValuesAD pConditions( "conditions" );
			g_pClientMode->ComputeVguiResConditions( pConditions );

			// reload the .res file from disk
			pViewport->LoadControlSettings( pszFile, NULL, NULL, pConditions );

			CGMsg( 1, CON_GROUP_MAPBASE_MISC, "Loaded custom HUD layout file \"%s\"\n", pszFile );;
		}
	}
#endif

public:

	void LoadCustomSoundscriptFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_SOUNDSCRIPTS, false ); }
	void LoadCustomLocalizationFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_LOCALIZATION, false ); }
	void LoadCustomSurfacePropsFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_SURFACEPROPS, false ); }
#ifdef CLIENT_DLL
	void LoadCustomCloseCaptionFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_CLOSECAPTION, false ); }
	void LoadCustomVGUIFile( const char *szScript )				{ LoadFromValue( szScript, MANIFEST_VGUI, false ); }
	void LoadCustomClientSchemeFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_CLIENTSCHEME, false ); }
	void LoadCustomHUDAnimationsFile( const char *szScript )	{ LoadFromValue( szScript, MANIFEST_HUDANIMATIONS, false ); }
	void LoadCustomHUDLayoutFile( const char *szScript )		{ LoadFromValue( szScript, MANIFEST_HUDLAYOUT, false ); }
#else
	void LoadCustomTalkerFile( const char *szScript )			{ LoadFromValue( szScript, MANIFEST_TALKER, false ); }
	void LoadCustomActbusyFile( const char *szScript )			{ LoadFromValue( szScript, MANIFEST_ACTBUSY, false ); }
#endif

	const char *GetModName() { return g_iszGameName; }
	bool IsCoreMapbase() { return g_bMapbaseCore; }

#ifdef MAPBASE_VSCRIPT
	void ScriptAddManifestFile( const char *szScript ) { AddManifestFile( szScript ); }

	virtual void RegisterVScript()
	{
		g_pScriptVM->RegisterInstance( this, "Mapbase" );
	}
#endif

private:

#ifdef CLIENT_DLL
	bool m_bInitializedRTs = false;
	CUtlVector<CTextureReference> m_CameraTextures;

	CUtlVector<CUtlSymbol> m_CloseCaptionFileNames;
#endif
};

CMapbaseSystem	g_MapbaseSystem;

BEGIN_DATADESC_NO_BASE( CMapbaseSystem )

	//DEFINE_UTLVECTOR( m_StoredManifestFiles, FIELD_STRING ),

END_DATADESC()

#ifdef MAPBASE_VSCRIPT
BEGIN_SCRIPTDESC_ROOT( CMapbaseSystem, SCRIPT_SINGLETON "All-purpose Mapbase system primarily used for map-specific files." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptAddManifestFile, "AddManifestFile", "Loads a manifest file." )
	DEFINE_SCRIPTFUNC( LoadCustomSoundscriptFile, "Loads a custom soundscript file." )
	DEFINE_SCRIPTFUNC( LoadCustomLocalizationFile, "Loads a custom localization file." )
	DEFINE_SCRIPTFUNC( LoadCustomSurfacePropsFile, "Loads a custom surface properties file." )
#ifdef CLIENT_DLL
	DEFINE_SCRIPTFUNC( LoadCustomCloseCaptionFile, "Loads a custom closed captions file." )
	DEFINE_SCRIPTFUNC( LoadCustomVGUIFile, "Loads a custom VGUI definitions file." )
	DEFINE_SCRIPTFUNC( LoadCustomClientSchemeFile, "Loads a custom ClientScheme.res override file." )
	DEFINE_SCRIPTFUNC( LoadCustomHUDAnimationsFile, "Loads a custom HUD animations override file." )
	DEFINE_SCRIPTFUNC( LoadCustomHUDLayoutFile, "Loads a custom HUD layout override file." )
#else
	DEFINE_SCRIPTFUNC( LoadCustomTalkerFile, "Loads a custom talker file." )
	DEFINE_SCRIPTFUNC( LoadCustomActbusyFile, "Loads a custom actbusy file." )
#endif

	DEFINE_SCRIPTFUNC( GetModName, "Gets the name of the mod. This is the name which shows up on Steam, RPC, etc." )
	DEFINE_SCRIPTFUNC( IsCoreMapbase, "Indicates whether this is one of the original Mapbase mods or just a separate mod using its code." )

	// Legacy
	DEFINE_SCRIPTFUNC_NAMED( LoadCustomSoundscriptFile, "LoadSoundscriptFile", SCRIPT_HIDE )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC_NAMED( LoadCustomTalkerFile, "LoadTalkerFile", SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC_NAMED( LoadCustomActbusyFile, "LoadActbusyFile", SCRIPT_HIDE )
#endif

END_SCRIPTDESC();
#endif

static void CC_Mapbase_LoadManifestFile( const CCommand& args )
{
	g_MapbaseSystem.AddManifestFile(args[1]);
}

#ifdef CLIENT_DLL
static ConCommand mapbase_loadmanifestfile("mapbase_loadmanifestfile_client", CC_Mapbase_LoadManifestFile, "Loads a Mapbase manifest file on the client. If you don't want this to be saved and found when reloaded, type a '1' after the file path." );
#else
static ConCommand mapbase_loadmanifestfile("mapbase_loadmanifestfile", CC_Mapbase_LoadManifestFile, "Loads a Mapbase manifest file. If you don't want this to be saved and found when reloaded, type a '1' after the file path." );
#endif

#ifdef GAME_DLL
static CUtlVector<MODTITLECOMMENT> g_MapbaseChapterMaps;
static CUtlVector<MODCHAPTER> g_MapbaseChapterList;
CUtlVector<MODTITLECOMMENT> *Mapbase_GetChapterMaps()
{
	if (g_MapbaseChapterMaps.Count() == 0)
	{
		// Check the chapter list
		KeyValues *chapterlist = new KeyValues("ChapterList");
		if (chapterlist->LoadFromFile(filesystem, "scripts/chapters.txt", "MOD"))
		{
			KeyValues *pKey = chapterlist->GetFirstSubKey();
			if (pKey)
			{
				if (Q_stricmp( pKey->GetName(), "Chapters" ) == 0)
				{
					for (KeyValues *pChapters = pKey->GetFirstSubKey(); pChapters; pChapters = pChapters->GetNextKey())
					{
						int index = g_MapbaseChapterList.AddToTail();
						g_MapbaseChapterList[index].iChapter = atoi(pChapters->GetName());
						Q_strncpy(g_MapbaseChapterList[index].pChapterName, pChapters->GetString(), sizeof(g_MapbaseChapterList[index]));
					}
				}

				for (pKey = pKey->GetNextKey(); pKey; pKey = pKey->GetNextKey())
				{
					int index = g_MapbaseChapterMaps.AddToTail();
					Q_strncpy(g_MapbaseChapterMaps[index].pBSPName, pKey->GetName(), sizeof(g_MapbaseChapterMaps[index].pBSPName));
					Q_strncpy(g_MapbaseChapterMaps[index].pTitleName, pKey->GetString(), sizeof(g_MapbaseChapterMaps[index].pTitleName));

					//comment.pBSPName = pKey->GetName();
					//comment.pTitleName = pKey->GetString();
				}
			}
		}
		chapterlist->deleteThis();
	}

	return &g_MapbaseChapterMaps;
}

CUtlVector<MODCHAPTER> *Mapbase_GetChapterList()
{
	return &g_MapbaseChapterList;
}

int Mapbase_GetChapterCount()
{
	return g_MapbaseChapterList.Count();
}

ThreeState_t Flashlight_GetLegacyVersionKey()
{
	KeyValues *gameinfo = new KeyValues( "GameInfo" );
	if (g_MapbaseSystem.GetGameInfoKeyValues( gameinfo ))
	{
		// -1 = default
		int iUseLegacyFlashlight = gameinfo->GetInt( "use_legacy_flashlight", -1 );
		if (iUseLegacyFlashlight > -1)
			return iUseLegacyFlashlight != 0 ? TRS_TRUE : TRS_FALSE;
	}
	gameinfo->deleteThis();

	return TRS_NONE;
}

#define SF_MANIFEST_START_ACTIVATED (1 << 0)

class CMapbaseManifestEntity : public CPointEntity
{
public:
	DECLARE_DATADESC();

	DECLARE_CLASS( CMapbaseManifestEntity, CPointEntity );

	void Spawn( void )
	{
		BaseClass::Spawn();
		if (HasSpawnFlags(SF_MANIFEST_START_ACTIVATED))
		{
			LoadManifestFile();
		}
	}

	void LoadManifestFile( void )
	{
		const char *scriptfile = STRING(m_target);
		if ( filesystem->FileExists( scriptfile, "MOD" ) )
		{
			CGMsg(0, CON_GROUP_MAPBASE_MISC, "Mapbase: Adding manifest file \"%s\"\n", scriptfile);
			g_MapbaseSystem.AddManifestFile(scriptfile);
		}
		else
		{
			Warning("Mapbase: Manifest file \"%s\" does not exist!", scriptfile);
		}
	}

	void InputActivate(inputdata_t &inputdata)
	{
		LoadManifestFile();
	}
};

LINK_ENTITY_TO_CLASS( mapbase_manifest, CMapbaseManifestEntity );

BEGIN_DATADESC( CMapbaseManifestEntity )

	// Needs to be set up in the Activate methods of derived classes
	//DEFINE_CUSTOM_FIELD( m_pInstancedResponseSystem, responseSystemSaveRestoreOps ),

	// Function Pointers
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),

END_DATADESC()
#endif

//-----------------------------------------------------------------------------

void CV_IncludeNameChanged( IConVar *pConVar, const char *pOldString, float flOldValue );

#ifdef CLIENT_DLL
ConVar con_group_include_name_client( "con_group_include_name_client", "0", FCVAR_NONE, "Includes groups when printing on the client.", CV_IncludeNameChanged );

void CV_IncludeNameChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	SetConsoleGroupIncludeNames( con_group_include_name_client.GetBool() );
}
#else
ConVar con_group_include_name( "con_group_include_name", "0", FCVAR_NONE, "Includes groups when printing.", CV_IncludeNameChanged );

void CV_IncludeNameChanged( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	SetConsoleGroupIncludeNames( con_group_include_name.GetBool() );
}
#endif

CON_COMMAND_SHARED( con_group_reload, "Reloads all console groups." )
{
	InitConsoleGroups( g_pFullFileSystem );
}

CON_COMMAND_SHARED( con_group_list, "Prints a list of all console groups." )
{
	PrintAllConsoleGroups();
}

CON_COMMAND_SHARED( con_group_toggle, "Toggles a console group." )
{
	ToggleConsoleGroups( args.Arg( 1 ) );
}
