//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implementation of player info manager
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "playerinfomanager.h"
#include "edict.h"

#if defined( TF_DLL )
#include "tf_shareddefs.h"
#elif defined( CSTRIKE_DLL )
#include "weapon_csbase.h"
#elif defined( DOD_DLL )
#include "weapon_dodbase.h"
#elif defined( SDK_DLL )
#include "weapon_sdkbase.h"
#endif

extern CGlobalVars *gpGlobals;
static CPlayerInfoManager s_PlayerInfoManager;
static CPluginBotManager s_BotManager;

namespace
{

	// 
	//  Old version support
	//

	//Tony; pulled out version 1 and 2 support for orange box, we're starting fresh now with v3 player and v2 of the bot interface.
}

IPlayerInfo *CPlayerInfoManager::GetPlayerInfo( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
	if ( pPlayer )
		return pPlayer->GetPlayerInfo();
	else
		return NULL;
}
IPlayerInfo *CPlayerInfoManager::GetPlayerInfo( int index )
{
	return GetPlayerInfo( engine->PEntityOfEntIndex( index ) );
}

// Games implementing advanced bot support should override this.
int CPlayerInfoManager::AliasToWeaponId(const char *weaponName)
{
	//Tony; TF doesn't support this. Should it?
#if defined ( CSTRIKE_DLL ) || defined ( DOD_DLL ) || defined ( SDK_DLL )
	return AliasToWeaponID(weaponName);
#endif
	return -1;
}

// Games implementing advanced bot support should override this.
const char *CPlayerInfoManager::WeaponIdToAlias(int weaponId)
{
#if defined( TF_DLL )
	return WeaponIdToAlias(weaponId);
#elif defined ( CSTRIKE_DLL ) || defined ( DOD_DLL ) || defined ( SDK_DLL )
	return WeaponIDToAlias(weaponId);
#endif
	return "MOD_DIDNT_IMPLEMENT_ME";
}
CGlobalVars *CPlayerInfoManager::GetGlobalVars()
{
	return gpGlobals;
}



IBotController *CPluginBotManager::GetBotController( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
	if ( pPlayer && pPlayer->IsBot() )
	{
		return pPlayer->GetBotController();
	}
	else
	{
		return NULL;
	}
}

edict_t *CPluginBotManager::CreateBot( const char *botname )
{	
	edict_t *pEdict = engine->CreateFakeClient( botname );
	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a player entity for the bot, and call spawn
	CBasePlayer *pPlayer = ((CBasePlayer*)CBaseEntity::Instance( pEdict ));

	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );
	pPlayer->ChangeTeam( TEAM_UNASSIGNED );
	pPlayer->AddEFlags( EFL_PLUGIN_BASED_BOT );		// Mark it as a plugin based bot
	pPlayer->RemoveAllItems( true );
	pPlayer->Spawn();

	return pEdict;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPlayerInfoManager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER, s_PlayerInfoManager);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginBotManager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER, s_BotManager);

