//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: implementation of player info manager
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "playerinfomanager.h"
#include "edict.h"

extern CGlobalVars *gpGlobals;
static CPlayerInfoManager s_PlayerInfoManager;
static CPluginBotManager s_BotManager;

namespace
{

	// 
	//  Old version support
	//
	abstract_class IPlayerInfo_V1
	{
	public:
		// returns the players name (UTF-8 encoded)
		virtual const char *GetName() = 0;
		// returns the userid (slot number)
		virtual int		GetUserID() = 0;
		// returns the string of their network (i.e Steam) ID
		virtual const char *GetNetworkIDString() = 0;
		// returns the team the player is on
		virtual int GetTeamIndex() = 0;
		// changes the player to a new team (if the game dll logic allows it)
		virtual void ChangeTeam( int iTeamNum ) = 0;
		// returns the number of kills this player has (exact meaning is mod dependent)
		virtual int	GetFragCount() = 0;
		// returns the number of deaths this player has (exact meaning is mod dependent)
		virtual int	GetDeathCount() = 0;
		// returns if this player slot is actually valid
		virtual bool IsConnected() = 0;
		// returns the armor/health of the player (exact meaning is mod dependent)
		virtual int	GetArmorValue() = 0;
	};
	
	abstract_class IPlayerInfoManager_V1
	{
	public:
		virtual IPlayerInfo_V1 *GetPlayerInfo( edict_t *pEdict ) = 0;
	};


	class CPlayerInfoManager_V1: public IPlayerInfoManager_V1
	{
	public:
		virtual IPlayerInfo_V1 *GetPlayerInfo( edict_t *pEdict );
	};

	static CPlayerInfoManager_V1 s_PlayerInfoManager_V1;


	IPlayerInfo_V1 *CPlayerInfoManager_V1::GetPlayerInfo( edict_t *pEdict )
	{
		CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
		if ( pPlayer )
		{
			return (IPlayerInfo_V1 *)pPlayer->GetPlayerInfo();
		}
		else
		{
			return NULL;
		}
	}

	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPlayerInfoManager_V1, IPlayerInfoManager_V1, "PlayerInfoManager001", s_PlayerInfoManager_V1);
}

IPlayerInfo *CPlayerInfoManager::GetPlayerInfo( edict_t *pEdict )
{
	CBasePlayer *pPlayer = ( ( CBasePlayer * )CBaseEntity::Instance( pEdict ));
	if ( pPlayer )
	{
		return pPlayer->GetPlayerInfo();
	}
	else
	{
		return NULL;
	}
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
	pPlayer->RemoveAllItems( true );
	pPlayer->Spawn();

	return pEdict;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPlayerInfoManager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER, s_PlayerInfoManager);
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPluginBotManager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER, s_BotManager);

