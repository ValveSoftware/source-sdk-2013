/***
*
//========= Copyright Valve Corporation, All rights reserved. ============//
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
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
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tier0/vprof.h"
#include "tf_bot_temp.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );

extern bool			g_fGameOver;

extern ConVar tf_allow_player_name_change;


void FinishClientPutInServer( CTFPlayer *pPlayer )
{
	{
		bool save = engine->LockNetworkStringTables( false );
	
		pPlayer->InitialSpawn();
		pPlayer->Spawn();
	
		engine->LockNetworkStringTables( save );
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
	if ( !pPlayer->IsFakeClient() )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );

		if ( pPlayer->BHaveChatSuspensionInCurrentMatch() || !tf_allow_player_name_change.GetBool() )
		{
			engine->ServerCommand( UTIL_VarArgs( "lockplayername %d\n", pPlayer->GetUserID() ) );
		}
	}
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
	CTFPlayer *pPlayer = CTFPlayer::CreatePlayer( "player", pEdict );
	pPlayer->SetPlayerName( playername );
}


void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CTFPlayer *pPlayer = ToTFPlayer( CBaseEntity::Instance( pEdict ) );
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
		return "Team Fortress";
}


//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache( void )
{
	const char *pFilename = "scripts/client_precache.txt";
	KeyValues *pValues = new KeyValues( "ClientPrecache" );

	if ( !pValues->LoadFromFile( filesystem, pFilename, "GAME" ) )
	{
		Error( "Can't open %s for client precache info.", pFilename );
		pValues->deleteThis();
		return;
	}

	for ( KeyValues *pData = pValues->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		const char *pszType = pData->GetName();
		const char *pszFile = pData->GetString();

		if ( Q_strlen( pszType ) > 0 &&
			 Q_strlen( pszFile ) > 0 )
		{
			if ( !Q_stricmp( pData->GetName(), "model" ) )
			{
				CBaseEntity::PrecacheModel( pszFile );
			}
			else if ( !Q_stricmp( pData->GetName(), "scriptsound" ) )
			{
				CBaseEntity::PrecacheScriptSound( pszFile );
			}
			else if ( !Q_stricmp( pData->GetName(), "particle" ) )
			{
				PrecacheParticleSystem( pszFile );
			}
		}
	}

	pValues->deleteThis();

// @FD This has been moved into pure_server_consistency.txt
//
//	// particles
//// 	engine->ForceExactFile( "particles/blood_impact.pcf" );			// Don't force consistency on this because of the LV version.
//// 	engine->ForceExactFile( "particles/blood_impact_dx80.pcf" );	// Don't force consistency on this because of the LV version.
//// 	engine->ForceExactFile( "particles/blood_trail.pcf" );			// Don't force consistency on this because of the LV version.
//// 	engine->ForceExactFile( "particles/blood_trail_dx80.pcf" );		// Don't force consistency on this because of the LV version.
////	engine->ForceExactFile( "particles/buildingdamage.pcf" );
//	engine->ForceExactFile( "particles/bullet_tracers.pcf" );
//	engine->ForceExactFile( "particles/burningplayer.pcf" );
//	engine->ForceExactFile( "particles/burningplayer_dx80.pcf" );
//	engine->ForceExactFile( "particles/cig_smoke.pcf" );
//	engine->ForceExactFile( "particles/cig_smoke_dx80.pcf" );
////	engine->ForceExactFile( "particles/cinefx.pcf" );
////	engine->ForceExactFile( "particles/crit.pcf" );
////	engine->ForceExactFile( "particles/default.pcf" );
//	engine->ForceExactFile( "particles/disguise.pcf" );
////	engine->ForceExactFile( "particles/explosion.pcf" );
////	engine->ForceExactFile( "particles/explosion_dx80.pcf" );
////	engine->ForceExactFile( "particles/explosion_dx90_slow.pcf" );
////	engine->ForceExactFile( "particles/explosion_high.pcf" );
//	engine->ForceExactFile( "particles/flag_particles.pcf" );
////	engine->ForceExactFile( "particles/flamethrower.pcf" );
////	engine->ForceExactFile( "particles/flamethrowerTest.pcf" );
////	engine->ForceExactFile( "particles/flamethrower_dx80.pcf" );
////	engine->ForceExactFile( "particles/flamethrower_dx90_slow.pcf" );
////	engine->ForceExactFile( "particles/flamethrower_high.pcf" );
////	engine->ForceExactFile( "particles/impact_fx.pcf" );
////	engine->ForceExactFile( "particles/item_fx.pcf" );
////	engine->ForceExactFile( "particles/medicgun_attrib.pcf" );
////	engine->ForceExactFile( "particles/medicgun_beam.pcf" );
////	engine->ForceExactFile( "particles/medicgun_beam_dx80.pcf" );
////	engine->ForceExactFile( "particles/muzzle_flash.pcf" );
////	engine->ForceExactFile( "particles/muzzle_flash_dx80.pcf" );
////	engine->ForceExactFile( "particles/nailtrails.pcf" );
//	engine->ForceExactFile( "particles/nemesis.pcf" );
//	engine->ForceExactFile( "particles/player_recent_teleport.pcf" );
//	engine->ForceExactFile( "particles/player_recent_teleport_dx80.pcf" );
////	engine->ForceExactFile( "particles/rocketbackblast.pcf" );
////	engine->ForceExactFile( "particles/rocketjumptrail.pcf" );
////	engine->ForceExactFile( "particles/rockettrail.pcf" );
////	engine->ForceExactFile( "particles/rockettrail_dx80.pcf" );
////	engine->ForceExactFile( "particles/rockettrail_dx90_slow.pcf" );
////	engine->ForceExactFile( "particles/shellejection.pcf" );
////	engine->ForceExactFile( "particles/shellejection_dx80.pcf" );
////	engine->ForceExactFile( "particles/shellejection_high.pcf" );
////	engine->ForceExactFile( "particles/smoke_blackbillow.pcf" );
////	engine->ForceExactFile( "particles/smoke_blackbillow_dx80.pcf" );
////	engine->ForceExactFile( "particles/sparks.pcf" );
//	engine->ForceExactFile( "particles/speechbubbles.pcf" );
////	engine->ForceExactFile( "particles/stickybomb.pcf" );
////	engine->ForceExactFile( "particles/stickybomb_dx80.pcf" );
//	engine->ForceExactFile( "particles/teleported_fx.pcf" );
//	engine->ForceExactFile( "particles/teleport_status.pcf" );
//	engine->ForceExactFile( "particles/water.pcf" );
//	engine->ForceExactFile( "particles/water_dx80.pcf" );
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

	Bot_RunAll();
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CTFGameRules" );
}
