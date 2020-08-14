//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Mapbase's RPC implementation.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#ifdef CLIENT_DLL

#ifdef STEAM_RPC
#include "clientsteamcontext.h"
#include "steam/steamclientpublic.h"
#endif

#ifdef DISCORD_RPC
#include "discord_rpc.h"
#include <time.h>
#include "c_world.h"
#endif

#include "filesystem.h"
#include "c_playerresource.h"
#include <vgui_controls/Controls.h> 
#include <vgui/ILocalize.h>

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// From mapbase_shared.cpp
extern const char *g_MapName;

// The game's name found in gameinfo.txt. Mostly used for Discord RPC.
extern char g_iszGameName[128];

#ifdef MAPBASE_RPC
void MapbaseRPC_CVarToggle( IConVar *var, const char *pOldString, float flOldValue );

ConVar mapbase_rpc_enabled("mapbase_rpc_enabled", "1", FCVAR_ARCHIVE, "Controls whether Mapbase's RPC stuff is enabled on this client.", MapbaseRPC_CVarToggle);

//-----------------------------------------------------------------------------
// RPC Stuff
// 
// Mapbase has some special "RPC" integration stuff for things like Discord.
// There's a section that goes into more detail below.
//-----------------------------------------------------------------------------

void MapbaseRPC_Init();
void MapbaseRPC_Shutdown();

void MapbaseRPC_Update( int iType, const char *pMapName );
void MapbaseRPC_Update( int iRPCMask, int iType, const char *pMapName );

#ifdef STEAM_RPC
void MapbaseRPC_UpdateSteam( int iType, const char *pMapName );
#endif

#ifdef DISCORD_RPC
void MapbaseRPC_UpdateDiscord( int iType, const char *pMapName );
void MapbaseRPC_GetDiscordParameters( DiscordRichPresence &discordPresence, int iType, const char *pMapName );
#endif

enum RPCClients_t
{
	RPC_STEAM,
	RPC_DISCORD,

	NUM_RPCS,
};

static const char *g_pszRPCNames[] = {
	"Steam",
	"Discord",
};

// This is a little dodgy, but it stops us from having to add spawnflag definitions for each RPC.
#define RPCFlag(rpc) (1 << rpc)

// The global game_metadata entity.
// There can be only one...for each RPC.
static EHANDLE g_Metadata[NUM_RPCS];

// Don't update constantly
#define RPC_UPDATE_COOLDOWN 5.0f

// How long to wait before updating in case multiple variables are changing
#define RPC_UPDATE_WAIT 0.25f
#endif

#ifdef CLIENT_DLL
#define CMapbaseMetadata C_MapbaseMetadata
#endif

class CMapbaseMetadata : public CBaseEntity
{
public:
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif
	DECLARE_NETWORKCLASS();
	DECLARE_CLASS( CMapbaseMetadata, CBaseEntity );

#ifdef MAPBASE_RPC
#ifdef CLIENT_DLL
	~C_MapbaseMetadata()
	{
		for (int i = 0; i < NUM_RPCS; i++)
		{
			if (g_Metadata[i] == this)
			{
				g_Metadata[i] = NULL;
			}
		}
	}

	void OnDataChanged( DataUpdateType_t updateType )
	{
		if (updateType == DATA_UPDATE_CREATED)
		{
			for (int i = 0; i < NUM_RPCS; i++)
			{
				// See if we're updating this RPC.
				if (m_spawnflags & RPCFlag(i))
				{
					if (g_Metadata[i])
					{
						Warning("Warning: Metadata entity for %s already exists, replacing with new one\n", g_pszRPCNames[i]);

						// Inherit their update timer
						m_flRPCUpdateTimer = static_cast<C_MapbaseMetadata*>(g_Metadata[i].Get())->m_flRPCUpdateTimer;

						g_Metadata[i].Get()->Remove();
					}

					DevMsg("Becoming metadata entity for %s\n", g_pszRPCNames[i]);
					g_Metadata[i] = this;
				}
			}
		}

		// Avoid spamming updates
		if (gpGlobals->curtime > (m_flRPCUpdateTimer - RPC_UPDATE_WAIT))
		{
			// Multiple variables might be changing, wait until they're probably all finished
			m_flRPCUpdateTimer = gpGlobals->curtime + RPC_UPDATE_WAIT;
		}

		DevMsg("Metadata changed; updating in %f\n", m_flRPCUpdateTimer - gpGlobals->curtime);

		// Update when the cooldown is over
		SetNextClientThink( m_flRPCUpdateTimer );
	}

	void ClientThink()
	{
		// NOTE: Client thinking should be limited by the update timer!
		UpdateRPCThink();

		// Wait until our data is changed again
		SetNextClientThink( CLIENT_THINK_NEVER );
	}

	void UpdateRPCThink()
	{
		DevMsg("Global metadata entity: %s\n", g_Metadata != NULL ? "Valid" : "Invalid!?");

		MapbaseRPC_Update(m_spawnflags, RPCSTATE_UPDATE, g_MapName);

		m_flRPCUpdateTimer = gpGlobals->curtime + RPC_UPDATE_COOLDOWN;
	}
#else
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
#endif
#endif

#ifdef CLIENT_DLL
	char m_iszRPCState[128];
	char m_iszRPCDetails[128];

#ifdef MAPBASE_RPC
	// Built-in update spam limiter
	float m_flRPCUpdateTimer = RPC_UPDATE_COOLDOWN;

	int		m_spawnflags;
#endif
#else
	CNetworkVar( string_t, m_iszRPCState );
	CNetworkVar( string_t, m_iszRPCDetails );
#endif

	// TODO: Player-specific control
	//CNetworkVar( int, m_iLimitingID );
};

LINK_ENTITY_TO_CLASS( game_metadata, CMapbaseMetadata );

IMPLEMENT_NETWORKCLASS_ALIASED(MapbaseMetadata, DT_MapbaseMetadata)

BEGIN_NETWORK_TABLE_NOBASE(CMapbaseMetadata, DT_MapbaseMetadata)

#ifdef MAPBASE_RPC
#ifdef CLIENT_DLL
	RecvPropString(RECVINFO(m_iszRPCState)),
	RecvPropString(RECVINFO(m_iszRPCDetails)),
	RecvPropInt( RECVINFO( m_spawnflags ) ),
#else
	SendPropStringT(SENDINFO(m_iszRPCState) ),
	SendPropStringT(SENDINFO(m_iszRPCDetails) ),
	SendPropInt( SENDINFO(m_spawnflags), 8, SPROP_UNSIGNED ),
#endif
#endif

END_NETWORK_TABLE()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CMapbaseMetadata )

	// Inputs
	DEFINE_INPUT( m_iszRPCState, FIELD_STRING, "SetRPCState" ),
	DEFINE_INPUT( m_iszRPCDetails, FIELD_STRING, "SetRPCDetails" ),

END_DATADESC()
#endif

#ifdef MAPBASE_RPC
//-----------------------------------------------------------------------------
// Purpose: Mapbase's special integration with rich presence clients, most notably Discord.
// 
// This only has Discord and crude groundwork for Steam as of writing,
//-----------------------------------------------------------------------------

//-----------------------------------------
// !!! FOR MODS !!!
// 
// Create your own Discord "application" if you want to change what info/images show up, etc.
// You can change the app ID in "scripts/mapbase_rpc.txt". It's located in the shared content VPK and the mod templates.
// You could override that file in your mod to change it to your own app ID.
// 
// This code automatically shows the mod's title in the details, but it's easy to change this code if you want things to be chapter-specific, etc.
// 
//-----------------------------------------

// Changing the default value of the convars below will not work.
// Use "scripts/mapbase_rpc.txt" instead.
static ConVar cl_discord_appid("cl_discord_appid", "582595088719413250", FCVAR_NONE);
static ConVar cl_discord_largeimage("cl_discord_largeimage", "mb_logo_episodic", FCVAR_NONE);
static ConVar cl_discord_largeimage_text("cl_discord_largeimage_text", "Half-Life 2", FCVAR_NONE);
static int64_t startTimestamp = time(0);

//

int MapbaseRPC_GetPlayerCount()
{
	int iNumPlayers = 0;

	if (g_PR)
	{
		for (; iNumPlayers <= gpGlobals->maxClients; iNumPlayers++)
		{
			if (!g_PR->IsConnected( iNumPlayers ))
				break;
		}
	}

	return iNumPlayers;
}

//-----------------------------------------------------------------------------
// Discord RPC handlers
//-----------------------------------------------------------------------------
static void HandleDiscordReady(const DiscordUser* connectedUser)
{
	DevMsg("Discord: Connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId);
}

static void HandleDiscordDisconnected(int errcode, const char* message)
{
	DevMsg("Discord: Disconnected (%d: %s)\n", errcode, message);
}

static void HandleDiscordError(int errcode, const char* message)
{
	DevMsg("Discord: Error (%d: %s)\n", errcode, message);
}

static void HandleDiscordJoin(const char* secret)
{
	// Not implemented
}

static void HandleDiscordSpectate(const char* secret)
{
	// Not implemented
}

static void HandleDiscordJoinRequest(const DiscordUser* request)
{
	// Not implemented
}

void MapbaseRPC_Init()
{
	// Only init if RPC is enabled
	if (mapbase_rpc_enabled.GetInt() <= 0)
		return;

	// First, load the config
	// (we need its values immediately)
	KeyValues *pKV = new KeyValues( "MapbaseRPC" );
	if (pKV->LoadFromFile( filesystem, "scripts/mapbase_rpc.txt" ))
	{
		const char *szAppID = pKV->GetString("discord_appid", cl_discord_appid.GetString());
		cl_discord_appid.SetValue(szAppID);

		const char *szLargeImage = pKV->GetString("discord_largeimage", cl_discord_largeimage.GetString());
		cl_discord_largeimage.SetValue(szLargeImage);

		const char *szLargeImageText = pKV->GetString("discord_largeimage_text", cl_discord_largeimage_text.GetString());
		cl_discord_largeimage_text.SetValue( szLargeImageText );
	}
	pKV->deleteThis();

	// Steam RPC
	if (steamapicontext)
	{
		if (steamapicontext->SteamFriends())
			steamapicontext->SteamFriends()->ClearRichPresence();
	}

	// Discord RPC
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	
	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	char appid[255];
	sprintf(appid, "%d", engine->GetAppID());
	Discord_Initialize(cl_discord_appid.GetString(), &handlers, 1, appid);

	if (!g_bTextMode)
	{
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));

		MapbaseRPC_GetDiscordParameters(discordPresence, RPCSTATE_INIT, NULL);

		discordPresence.startTimestamp = startTimestamp;

		Discord_UpdatePresence(&discordPresence);
	}
}

void MapbaseRPC_Shutdown()
{
	// Discord RPC
	Discord_ClearPresence();
	Discord_Shutdown();

	// Steam RPC
	if (steamapicontext)
	{
		if (steamapicontext->SteamFriends())
			steamapicontext->SteamFriends()->ClearRichPresence();
	}
}

void MapbaseRPC_Update( int iType, const char *pMapName )
{
	// All RPCs
	MapbaseRPC_Update( INT_MAX, iType, pMapName );
}

void MapbaseRPC_Update( int iRPCMask, int iType, const char *pMapName )
{
	// Only update if RPC is enabled
	if (mapbase_rpc_enabled.GetInt() <= 0)
		return;

	if (iRPCMask & RPCFlag(RPC_STEAM))
		MapbaseRPC_UpdateSteam(iType, pMapName);
	if (iRPCMask & RPCFlag(RPC_DISCORD))
		MapbaseRPC_UpdateDiscord(iType, pMapName);
}

#ifdef STEAM_RPC
void MapbaseRPC_UpdateSteam( int iType, const char *pMapName )
{
	// No Steam
	if (!steamapicontext || !steamapicontext->SteamFriends())
		return;

	const char *pszStatus = NULL;

	if (g_Metadata[RPC_STEAM] != NULL)
	{
		C_MapbaseMetadata *pMetadata = static_cast<C_MapbaseMetadata*>(g_Metadata[RPC_STEAM].Get());

		if (pMetadata->m_iszRPCDetails[0] != NULL)
			pszStatus = pMetadata->m_iszRPCDetails;
		else if (pMetadata->m_iszRPCState[0] != NULL)
			pszStatus = pMetadata->m_iszRPCState;
		else
		{
			if (engine->IsLevelMainMenuBackground())
				pszStatus = VarArgs("Main Menu (%s)", pMapName ? pMapName : "N/A");
			else
				pszStatus = VarArgs("Map: %s", pMapName ? pMapName : "N/A");
		}
	}
	else
	{
		switch (iType)
		{
			case RPCSTATE_INIT:
			case RPCSTATE_LEVEL_SHUTDOWN:
				{
					pszStatus = "Main Menu";
				} break;
			case RPCSTATE_LEVEL_INIT:
			default:
				{
					// Say we're in the main menu if it's a background map
					if (engine->IsLevelMainMenuBackground())
					{
						pszStatus = VarArgs("Main Menu (%s)", pMapName ? pMapName : "N/A");
					}
					else
					{
						pszStatus = VarArgs("Map: %s", pMapName ? pMapName : "N/A");
					}
				} break;
		}
	}

	DevMsg( "Updating Steam\n" );

	if (pszStatus)
	{
		steamapicontext->SteamFriends()->SetRichPresence( "gamestatus", pszStatus );
		steamapicontext->SteamFriends()->SetRichPresence( "steam_display", "#SteamRPC_Status" );

		if (gpGlobals->maxClients > 1)
		{
			// Players in server
			const CSteamID *serverID = serverengine->GetGameServerSteamID();
			if (serverID)
			{
				char szGroupID[32];
				Q_snprintf(szGroupID, sizeof(szGroupID), "%i", serverID->GetAccountID());

				char szGroupSize[8];
				Q_snprintf(szGroupSize, sizeof(szGroupSize), "%i", MapbaseRPC_GetPlayerCount());

				steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group", szGroupID );
				steamapicontext->SteamFriends()->SetRichPresence( "steam_player_group_size", szGroupSize );
			}
			else
			{
				DevWarning("Steam RPC cannot update player count (no server ID)\n");
			}
		}
	}
}
#endif

#ifdef DISCORD_RPC
void MapbaseRPC_GetDiscordMapInfo( char *pDetails, size_t iSize, const char *pMapName )
{
	if (!pMapName)
		pMapName = "N/A";

	// Say we're in the main menu if it's a background map
	if (engine->IsLevelMainMenuBackground())
	{
		Q_snprintf( pDetails, iSize, "Main Menu (%s)", pMapName );
	}
	else
	{
		// Show the chapter title first
		const char *szChapterTitle = NULL;

		C_World *pWorld = GetClientWorldEntity();
		if ( pWorld && pWorld->m_iszChapterTitle[0] != '\0' )
		{
			szChapterTitle = g_pVGuiLocalize->FindAsUTF8( pWorld->m_iszChapterTitle );
			if (!szChapterTitle || szChapterTitle[0] == '\0')
				szChapterTitle = pWorld->m_iszChapterTitle;
		}

		if (szChapterTitle)
		{
			Q_snprintf( pDetails, iSize, "%s (%s)", szChapterTitle, pMapName );
		}
		else
		{
			Q_snprintf( pDetails, iSize, "%s", pMapName );
		}
	}
}

void MapbaseRPC_GetDiscordParameters( DiscordRichPresence &discordPresence, int iType, const char *pMapName )
{
	static char details[128];
	static char state[128];

	details[0] = '\0';
	state[0] = '\0';

	if (g_Metadata[RPC_DISCORD] != NULL)
	{
		C_MapbaseMetadata *pMetadata = static_cast<C_MapbaseMetadata*>(g_Metadata[RPC_DISCORD].Get());

		if (pMetadata->m_iszRPCState[0] != NULL)
			Q_strncpy( state, pMetadata->m_iszRPCState, sizeof(state) );
		else
			Q_strncpy( state, g_iszGameName, sizeof(state) );

		if (pMetadata->m_iszRPCDetails[0] != NULL)
			Q_strncpy( details, pMetadata->m_iszRPCDetails, sizeof(details) );
		else
		{
			MapbaseRPC_GetDiscordMapInfo( details, sizeof(details), pMapName );
		}
	}
	else
	{
		Q_strncpy( state, g_iszGameName, sizeof(state) );

		switch (iType)
		{
			case RPCSTATE_INIT:
			case RPCSTATE_LEVEL_SHUTDOWN:
				{
					Q_strncpy( details, "Main Menu", sizeof(details) );
				} break;
			case RPCSTATE_LEVEL_INIT:
			default:
				{
					MapbaseRPC_GetDiscordMapInfo( details, sizeof(details), pMapName );
				} break;
		}
	}

	if (gpGlobals->maxClients > 1)
	{
		Q_snprintf( details, sizeof(details), "%s (%i/%i)", details, MapbaseRPC_GetPlayerCount(), gpGlobals->maxClients );
	}

	if (state[0] != '\0')
		discordPresence.state = state;
	if (details[0] != '\0')
		discordPresence.details = details;

	// Generic Mapbase logo. Specific to the Mapbase Discord application.
	discordPresence.smallImageKey = "mb_logo_general";
	discordPresence.smallImageText = "Mapbase";

	discordPresence.largeImageKey = cl_discord_largeimage.GetString();
	discordPresence.largeImageText = cl_discord_largeimage_text.GetString();
}

void MapbaseRPC_UpdateDiscord( int iType, const char *pMapName )
{
	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));

	DevMsg("Updating Discord\n");

	discordPresence.startTimestamp = startTimestamp;

	MapbaseRPC_GetDiscordParameters( discordPresence, iType, pMapName );

	Discord_UpdatePresence(&discordPresence);
}

void MapbaseRPC_CVarToggle( IConVar *var, const char *pOldString, float flOldValue )
{
	if (flOldValue <= 0 && mapbase_rpc_enabled.GetInt() > 0)
	{
		// Turning on
		MapbaseRPC_Init();
		MapbaseRPC_Update( g_MapName != NULL ? RPCSTATE_UPDATE : RPCSTATE_INIT, g_MapName );
	}
	else if (mapbase_rpc_enabled.GetInt() <= 0)
	{
		// Turning off
		MapbaseRPC_Shutdown();
	}
}
#endif

#endif
