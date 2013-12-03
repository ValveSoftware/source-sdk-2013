//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
/*

===== tf_client.cpp ========================================================

  HL2 client/server game specific stuff

*/

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "shake.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "sdk_player.h"
#include "sdk_gamerules.h"
#include "tier0/vprof.h"
#include "sdk_bot_temp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool			g_fGameOver;


void FinishClientPutInServer( CSDKPlayer *pPlayer )
{
	pPlayer->InitialSpawn();
	pPlayer->Spawn();

	if (!pPlayer->IsBot())
	{
		// When the player first joins the server, they
		pPlayer->m_takedamage = DAMAGE_YES;
		pPlayer->pl.deadflag = false;
		pPlayer->m_lifeState = LIFE_ALIVE;
		pPlayer->RemoveEffects( EF_NODRAW );
		pPlayer->ChangeTeam( TEAM_UNASSIGNED );
		pPlayer->SetThink( NULL );
	}

	char sName[128];
	Q_strncpy( sName, pPlayer->GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );
}

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	// Allocate a CBaseTFPlayer for pev, and call spawn
	CSDKPlayer *pPlayer = CSDKPlayer::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CSDKPlayer *pPlayer = ToSDKPlayer( CBaseEntity::Instance( pEdict ) );
	FinishClientPutInServer( pPlayer );
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "CounterStrike";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	// Materials used by the client effects
	CBaseEntity::PrecacheModel( "sprites/white.vmt" );
	CBaseEntity::PrecacheModel( "sprites/physbeam.vmt" );
}


// called by ClientKill and DeadThink
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			dynamic_cast< CBasePlayer* >( pEdict )->CreateCorpse();
		}

		// respawn player
		pEdict->Spawn();
	}
	else
	{       // restart the entire server
		engine->ServerCommand("reload\n");
	}
}

void GameStartFrame( void )
{
	VPROF( "GameStartFrame" );

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.GetInt() ? true : false;
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CSDKGameRules" );
}
