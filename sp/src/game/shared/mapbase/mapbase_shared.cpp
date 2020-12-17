//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Carries the Mapbase CAutoGameSystem that loads manifest among other things.
//			Also includes code that does not fit anywhere else.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>
#include "saverestore_utlvector.h"
#include "props_shared.h"
#include "utlbuffer.h"
#ifdef CLIENT_DLL
#include "hud_closecaption.h"
#include "panelmetaclassmgr.h"
#include "c_soundscape.h"
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

ConVar mapbase_load_soundscripts("mapbase_load_soundscripts", "1", FCVAR_ARCHIVE, "Should we load map-specific soundscripts? e.g. \"maps/mapname_level_sounds.txt\"");

//ConVar mapbase_load_propdata("mapbase_load_propdata", "1", FCVAR_ARCHIVE, "Should we load map-specific propdata files? e.g. \"maps/mapname_propdata.txt\"");

//ConVar mapbase_load_soundscapes("mapbase_load_soundscapes", "1", FCVAR_ARCHIVE, "Should we load map-specific soundscapes? e.g. \"maps/mapname_soundscapes.txt\"");

ConVar mapbase_load_localization("mapbase_load_localization", "1", FCVAR_ARCHIVE, "Should we load map-specific localized text files? e.g. \"maps/mapname_english.txt\"");

#ifdef CLIENT_DLL

//ConVar mapbase_load_cc("mapbase_load_cc", "1", FCVAR_ARCHIVE, "Should we load map-specific closed captioning? e.g. \"maps/mapname_closecaption_english.txt\" and \"maps/mapname_closecaption_english.dat\"");

#else

ConVar mapbase_load_sentences("mapbase_load_sentences", "1", FCVAR_ARCHIVE, "Should we load map-specific sentences? e.g. \"maps/mapname_sentences.txt\"");

ConVar mapbase_load_talker("mapbase_load_talker", "1", FCVAR_ARCHIVE, "Should we load map-specific talker files? e.g. \"maps/mapname_talker.txt\"");
ConVar mapbase_flush_talker("mapbase_flush_talker", "1", FCVAR_NONE, "Normally, when a map with custom talker files is unloaded, the response system resets to rid itself of the custom file(s). Turn this convar off to prevent that from happening.");

ConVar mapbase_load_actbusy("mapbase_load_actbusy", "1", FCVAR_ARCHIVE, "Should we load map-specific actbusy files? e.g. \"maps/mapname_actbusy.txt\"");

#endif

#ifdef GAME_DLL
// This cvar should change with each Mapbase update
ConVar mapbase_version( "mapbase_version", "6.1", FCVAR_NONE, "The version of Mapbase currently being used in this mod." );

extern void MapbaseGameLog_Init();

extern void ParseCustomActbusyFile(const char *file);

extern bool LoadResponseSystemFile(const char *scriptfile);
extern void ReloadResponseSystem();

// Reloads the response system when the map changes to avoid custom talker leaking
static bool g_bMapContainsCustomTalker;
#endif

// Indicates this is a core Mapbase mod and not a mod using its code.
static bool g_bMapbaseCore;

// The game's name found in gameinfo.txt. Mostly used for Discord RPC.
char g_iszGameName[128];

enum
{
	MANIFEST_SOUNDSCRIPTS,
	//MANIFEST_PROPDATA,
	//MANIFEST_SOUNDSCAPES,
	MANIFEST_LOCALIZATION,
#ifdef CLIENT_DLL
	//MANIFEST_CLOSECAPTION,
	MANIFEST_VGUI,
#else
	MANIFEST_TALKER,
	MANIFEST_SENTENCES,
	MANIFEST_ACTBUSY,
#endif

	// Must always be kept below
	MANIFEST_NUM_TYPES,
};

struct ManifestType_t
{
	//int type;
	const char *string;
	ConVar *cvar;
};

// KEEP THS IN SYNC WITH THE ENUM!
static const ManifestType_t gm_szManifestFileStrings[MANIFEST_NUM_TYPES] = {
	{ "soundscripts",		&mapbase_load_soundscripts },
	//{ "propdata",			&mapbase_load_propdata },
	//{ "soundscapes",		&mapbase_load_soundscapes },
	{ "localization",		&mapbase_load_localization },
#ifdef CLIENT_DLL
	//{ "closecaption",		&mapbase_load_cc },
	{ "vgui",				NULL },
#else
	{ "talker",				&mapbase_load_talker },
	{ "sentences",			&mapbase_load_sentences },
	{ "actbusy",			&mapbase_load_actbusy },
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
			CGMsg( 1, "Mapbase Misc.", "Mapbase: Reloading response system to flush custom talker\n" );
			ReloadResponseSystem();
			g_bMapContainsCustomTalker = false;
		}
#endif
	}

	virtual void LevelInitPreEntity()
	{
#ifdef GAME_DLL
		CGMsg( 0, "Mapbase Misc.", "Mapbase system loaded\n" );
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
		}
		gameinfo->deleteThis();

		RefreshMapName();

		// Shared Mapbase localization file
		g_pVGuiLocalize->AddFile( "resource/mapbase_%language%.txt" );
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
	bool m_bInitializedRTs = false;
	CUtlVector<CTextureReference> m_CameraTextures;

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

		AddManifestFile(pKV/*, true*/);

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

		CGMsg( 1, "Mapbase Misc.", "===== Mapbase Manifest: Loading manifest file %s =====\n", file );

		AddManifestFile(pKV, false);

		CGMsg( 1, "Mapbase Misc.", "==============================================================================\n" );

		pKV->deleteThis();
	}

	void LoadFromValue( const char *value, int type, bool bDontWarn )
	{
		if (!filesystem->FileExists(value, "MOD"))
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
#ifdef CLIENT_DLL
			//case MANIFEST_CLOSECAPTION: { todo } break;
			case MANIFEST_VGUI: { PanelMetaClassMgr()->LoadMetaClassDefinitionFile( value ); } break;
			//case MANIFEST_SOUNDSCAPES: { Soundscape_AddFile(value); } break;
#else
			case MANIFEST_TALKER: {
					g_bMapContainsCustomTalker = true;
					LoadResponseSystemFile(value); //PrecacheCustomResponseSystem( value );
				} break;
			//case MANIFEST_SOUNDSCAPES: { g_SoundscapeSystem.AddSoundscapeFile(value); } break;
			case MANIFEST_SENTENCES: { engine->PrecacheSentenceFile(value); } break;
			case MANIFEST_ACTBUSY: { ParseCustomActbusyFile(value); } break;
#endif
		}
	}

	// This doesn't call deleteThis()!
	void AddManifestFile(KeyValues *pKV, bool bDontWarn = false)
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
						Q_strncat( value, g_MapName, sizeof( value ) );
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
					if (!gm_szManifestFileStrings[i].cvar || gm_szManifestFileStrings[i].cvar->GetBool())
					{
						LoadFromValue(value, i, bDontWarn);
					}
					break;
				}
			}
		}
	}

#ifdef MAPBASE_VSCRIPT
	void ScriptAddManifestFile( const char *szScript ) { AddManifestFile( szScript ); }

	void LoadSoundscriptFile( const char *szScript ) { LoadFromValue(szScript, MANIFEST_SOUNDSCRIPTS, false); }
#ifndef CLIENT_DLL
	void LoadTalkerFile( const char *szScript ) { LoadFromValue( szScript, MANIFEST_TALKER, false ); }
	void LoadActbusyFile( const char *szScript ) { LoadFromValue( szScript, MANIFEST_ACTBUSY, false ); }
#endif

	const char *GetModName() { return g_iszGameName; }
	bool IsCoreMapbase() { return g_bMapbaseCore; }

	virtual void RegisterVScript()
	{
		g_pScriptVM->RegisterInstance( this, "Mapbase" );
	}
#endif
};

CMapbaseSystem	g_MapbaseSystem;

BEGIN_DATADESC_NO_BASE( CMapbaseSystem )

	//DEFINE_UTLVECTOR( m_StoredManifestFiles, FIELD_STRING ),

END_DATADESC()

#ifdef MAPBASE_VSCRIPT
BEGIN_SCRIPTDESC_ROOT( CMapbaseSystem, SCRIPT_SINGLETON "All-purpose Mapbase system primarily used for map-specific files." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptAddManifestFile, "AddManifestFile", "Loads a manifest file." )
	DEFINE_SCRIPTFUNC( LoadSoundscriptFile, "Loads a custom soundscript file." )
#ifndef CLIENT_DLL
	DEFINE_SCRIPTFUNC( LoadTalkerFile, "Loads a custom talker file." )
	DEFINE_SCRIPTFUNC( LoadActbusyFile, "Loads a custom actbusy file." )
#endif
	DEFINE_SCRIPTFUNC( GetModName, "Gets the name of the mod. This is the name which shows up on Steam, RPC, etc." )
	DEFINE_SCRIPTFUNC( IsCoreMapbase, "Indicates whether this is one of the original Mapbase mods or just a separate mod using its code." )
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
			CGMsg(0, "Mapbase Misc.", "Mapbase: Adding manifest file \"%s\"\n", scriptfile);
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
