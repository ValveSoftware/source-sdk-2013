//========= Copyright Generalisk, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DISCORDRPC_H
#define DISCORDRPC_H
#ifdef _WIN32
#pragma once
#endif

#include "discord_rpc/discord_rpc.h"
#include <time.h>

// TODO: Find out A way for VPC to allow string preprocessors so I don't have to do shit like this again...
#if defined( TF_CLIENT_DLL )
#define DISCORD_APPID "1472369971517718670"
#elif defined( HL2MP )
#define DISCORD_APPID "1472371769250807849"
#else
#define DISCORD_APPID "1472370414486294618"
#endif

static ConVar cl_discord_appid( "cl_discord_appid", DISCORD_APPID, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT );
static int64_t startTimestamp = time(0);



static class DiscordRPC
{
public:
	void Init();
	void Shutdown();

	void SetStatus( DiscordRichPresence discordPresence );
	void SetStatus_Menu();
	void SetStatus_Game( const char* pMapName );
};

#endif // DISCORDRPC_H
