//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "convar.h"
#include "tier0/dbg.h"
#include "player.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Test_CreateEntity( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	// Require a player entity or that the command was entered from the dedicated server console
	if ( !pPlayer && UTIL_GetCommandClientIndex() > 0 )
	{
		return;
	}

	if ( args.ArgC() < 2 )
	{
		Error( "Test_CreateEntity: requires entity classname argument." );
	}

	const char *pClassName = args[ 1 ];

	// Don't allow regular users to create point_servercommand entities for the same reason as blocking ent_fire
	if ( pPlayer && !Q_stricmp( pClassName, "point_servercommand" ) )
	{
		if ( engine->IsDedicatedServer() )
		{
			// We allow people with disabled autokick to do it, because they already have rcon.
			if ( pPlayer->IsAutoKickDisabled() == false )
				return;
		}
		else if ( gpGlobals->maxClients > 1 )
		{
			// On listen servers with more than 1 player, only allow the host to create point_servercommand.
			CBasePlayer *pHostPlayer = UTIL_GetListenServerHost();
			if ( pPlayer != pHostPlayer )
				return;
		}
	}

	if ( !CreateEntityByName( pClassName ) )
	{
		Error( "Test_CreateEntity( %s ) failed.", pClassName );
	}
}


void Test_RandomPlayerPosition()
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	CWorld *pWorld = GetWorldEntity();
	if ( !pPlayer )
	{
		Error( "Test_RandomPlayerPosition: no local player entity." );
	}
	else if ( !pWorld )
	{
		Error( "Test_RandomPlayerPosition: no world entity." );
	}

	
	
	Vector vMin, vMax;
	pWorld->GetWorldBounds( vMin, vMax );

	Vector vecOrigin;
	vecOrigin.x = RandomFloat( vMin.x, vMax.x );
	vecOrigin.y = RandomFloat( vMin.y, vMax.y );
	vecOrigin.z = RandomFloat( vMin.z, vMax.z );
	pPlayer->ForceOrigin( vecOrigin );
}


ConCommand cc_Test_CreateEntity( "Test_CreateEntity", Test_CreateEntity, 0, FCVAR_CHEAT );
ConCommand cc_Test_RandomPlayerPosition( "Test_RandomPlayerPosition", Test_RandomPlayerPosition, 0, FCVAR_CHEAT );


