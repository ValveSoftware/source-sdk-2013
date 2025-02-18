//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

// for messaging with the GC
#include "tf_gcmessages.h"
#include "tf_item_inventory.h"
#include "econ_game_account_server.h"
#include "gc_clientsystem.h"

//-----------------------------------------------------------------------------

CON_COMMAND_F( cl_gameserver_create_identity, "Creates a new game server account associated with the currently logged in steam account", FCVAR_CLIENTDLL )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_CreateIdentity> msg( k_EMsgGC_GameServer_CreateIdentity );
		msg.Body().set_account_id( steamID.GetAccountID() );
		GCClientSystem()->BSendMessage( msg );
		Msg( "Request to create a game server account sent--please wait.\n" );
	}
	else
	{
		Msg( "You must be logged into Steam to create a game server account.\n" );
	}
}

CON_COMMAND_F( cl_gameserver_list, "List all the game server accounts owned by the currently logged in steam account", FCVAR_CLIENTDLL )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_List > msg( k_EMsgGC_GameServer_List );
		msg.Body().set_account_id( steamID.GetAccountID() );
		GCClientSystem()->BSendMessage( msg );
		Msg( "Request to retrieve owned game server accounts--please wait.\n" );
	}
	else
	{
		Msg( "You must be logged into Steam to get the list of game server accounts.\n" );
	}
}

CON_COMMAND_F( cl_gameserver_reset_identity, "Resets the identity token for a given game server", FCVAR_CLIENTDLL )
{
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		if ( args.ArgC() > 1 )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			GCSDK::CProtoBufMsg< CMsgGC_GameServer_ResetIdentity > msg( k_EMsgGC_GameServer_ResetIdentity );
			msg.Body().set_game_server_account_id( atoi( args[1] ) );
			GCClientSystem()->BSendMessage( msg );
			Msg( "Request to reset owned game server account identity token--please wait.\n" );
		}
		else
		{
			Msg( "Usage: cl_gameserver_reset_identity <game server account id>\n");
		}
	}
	else
	{
		Msg( "You must be logged into Steam to reset a game server's identity token.\n" );
	}
}

class CGC_GameServer_CreateIdentityResponse : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_CreateIdentityResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_CreateIdentityResponse > msg( pNetPacket );

		if ( msg.Body().account_created() )
		{
			Msg( "Game server account created successfully!\n" );
			Msg( "Set these convars on your game server to have it log in and receive benefits:\n" );
			Msg( "tf_server_identity_account_id %u\n", msg.Body().game_server_account_id() );
			Msg( "tf_server_identity_token \"%s\"\n\n", msg.Body().game_server_identity_token().c_str() );
		}
		else
		{
			Msg( "Unable to create a game server account attached to your Steam account.\n" );
			switch ( msg.Body().status() )
			{
				default:
				case CMsgGC_GameServer_CreateIdentityResponse::kStatus_GenericFailure:
					Msg( "Failure code %d.  Contact Steam support.\n", (int)msg.Body().status() );
					break;
				case CMsgGC_GameServer_CreateIdentityResponse::kStatus_TooMany:
					Msg( "Too many game server accounts already registered with your Steam account.\n" );
					break;
				case CMsgGC_GameServer_CreateIdentityResponse::kStatus_NoPrivs:
					Msg( "Your Steam account doesn't have rights to create game server accounts.\n" );
					break;
			}
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_CreateIdentityResponse, "CGC_GameServer_CreateIdentityResponse", k_EMsgGC_GameServer_CreateIdentityResponse, GCSDK::k_EServerTypeGCClient );

class CGC_GameServer_ListResponse : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_ListResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_ListResponse > msg( pNetPacket );

		Msg( "Owned Game Servers: %d\n", msg.Body().owned_game_servers_size() );
		for ( int i = 0; i < msg.Body().owned_game_servers_size(); ++i )
		{
			const CMsgGC_GameServer_ListResponse_GameServerIdentity &identity = msg.Body().owned_game_servers( i );
			const char *pStanding = GameServerAccount_GetStandingString( (eGameServerScoreStanding)identity.game_server_standing() );
			const char *pStandingTrend = GameServerAccount_GetStandingTrendString( (eGameServerScoreStandingTrend)identity.game_server_standing_trend() );
			Msg( "%d - Account ID: %u  Identity Token: %s   Standing: %s  Trend: %s\n", i + 1, identity.game_server_account_id(), identity.game_server_identity_token().c_str(), pStanding, pStandingTrend );
		}
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_ListResponse, "CGC_GameServer_ListResponse", k_EMsgGC_GameServer_ListResponse, GCSDK::k_EServerTypeGCClient );


class CGC_GameServer_ResetIdentityResponse : public GCSDK::CGCClientJob
{
public:
	CGC_GameServer_ResetIdentityResponse( GCSDK::CGCClient *pClient ) : GCSDK::CGCClientJob( pClient ) {}

	virtual bool BYieldingRunGCJob( GCSDK::IMsgNetPacket *pNetPacket )
	{
		GCSDK::CProtoBufMsg< CMsgGC_GameServer_ResetIdentityResponse > msg( pNetPacket );

		if ( msg.Body().game_server_identity_token_reset() )
		{
			Msg( "Game server account identity reset!\n" );
			Msg( "Set these convars on your game server to have it log in and receive benefits:\n" );
			Msg( "tf_server_identity_account_id %u\n", msg.Body().game_server_account_id() );
			Msg( "tf_server_identity_token \"%s\"\n\n", msg.Body().game_server_identity_token().c_str() );
		}
		else
		{
			Msg( "Failed to reset game server account identity--check to make sure the game server account id is correct!\n" );
		}
		
		return true;
	}
};
GC_REG_JOB( GCSDK::CGCClient, CGC_GameServer_ResetIdentityResponse, "CGC_GameServer_ResetIdentityResponse", k_EMsgGC_GameServer_ResetIdentityResponse, GCSDK::k_EServerTypeGCClient );