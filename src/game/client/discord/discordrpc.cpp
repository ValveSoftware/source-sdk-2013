//========= Copyright Generalisk, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "discordrpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DISCORDRPC_GAMELOGO_KEY "GameLogo"

#if defined ( TF2_CLIENT_DLL )
#define DISCORDRPC_GAMELOGO_TEXT "Team Fortress 2 Icon"
#elif defined ( HL2MP )
#define DISCORDRPC_GAMELOGO_TEXT "Half-Life 2 Icon"
#else
#define DISCORDRPC_GAMELOGO_TEXT "Game Icon"
#endif


static void HandleDiscordReady(const DiscordUser* connectedUser)
{
	DevMsg( "Discord: Connected to user %s#%s - %s\n",
		connectedUser->username,
		connectedUser->discriminator,
		connectedUser->userId );
}


static void HandleDiscordDisconnected(int errcode, const char* message)
{
	DevMsg( "Discord: Disconnected (%d: %s)\n", errcode, message );
}


static void HandleDiscordError(int errcode, const char* message)
{
	DevMsg( "Discord: Error (%d: %s)\n", errcode, message );
}


static void HandleDiscordJoin(const char* secret)
{
	// TODO: Impliment
}


static void HandleDiscordSpectate(const char* secret)
{
	// TODO: Impliment
}


static void HandleDiscordJoinRequest(const DiscordUser* request)
{
	// TODO: Impliment
}


void DiscordRPC::Init()
{
	DiscordEventHandlers handlers;
	memset( &handlers, 0, sizeof(handlers) );

	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	char appid[255];
	sprintf( appid, "%d", engine->GetAppID() );
	Discord_Initialize( cl_discord_appid.GetString(), &handlers, 1, appid );

	SetStatus_Menu();
}


void DiscordRPC::Shutdown()
{
	Discord_ClearPresence();
	Discord_Shutdown();
}


void DiscordRPC::SetStatus( DiscordRichPresence discordPresence )
{
	if ( g_bTextMode ) return;

	discordPresence.startTimestamp = startTimestamp;
	Discord_UpdatePresence( &discordPresence );
}


void DiscordRPC::SetStatus_Menu()
{
	if ( g_bTextMode ) return;

	DiscordRichPresence discordPresence;
	memset( &discordPresence, 0, sizeof(discordPresence) );

	discordPresence.state = "In-Game";
	discordPresence.details = "Main Menu";
	discordPresence.largeImageKey = DISCORDRPC_GAMELOGO_KEY;
	discordPresence.largeImageText = DISCORDRPC_GAMELOGO_TEXT;

	SetStatus( discordPresence );
}


void DiscordRPC::SetStatus_Game( const char* pMapName )
{
	if (g_bTextMode) return;

	DiscordRichPresence discordPresence;
	memset( &discordPresence, 0, sizeof(discordPresence) );

	char buffer[256];
	discordPresence.state = "In-Game";
	sprintf( buffer, "Map: %s", pMapName );
	discordPresence.details = buffer;
	discordPresence.largeImageKey = pMapName;
	discordPresence.largeImageText = pMapName;
	discordPresence.smallImageKey = DISCORDRPC_GAMELOGO_KEY;
	discordPresence.smallImageText = DISCORDRPC_GAMELOGO_TEXT;

	SetStatus( discordPresence );
}
