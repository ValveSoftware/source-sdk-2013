#include "cbase.h"
#include "mom_player.h"
#include "momentum/mom_gamerules.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say(edict_t *pEdict, bool teamonly);

extern CBaseEntity*	FindPickerEntityClass(CBasePlayer *pPlayer, char *classname);
extern bool			g_fGameOver;

/*
===========
ClientPutInServer

called each time a player is spawned into the game
============
*/
void ClientPutInServer(edict_t *pEdict, const char *playername)
{
    // Allocate a CBasePlayer for pev, and call spawn
    CMomentumPlayer *pPlayer = CMomentumPlayer::CreatePlayer("player", pEdict);
    pPlayer->SetPlayerName(playername);
}


void ClientActive(edict_t *pEdict, bool bLoadGame)
{
    CMomentumPlayer *pPlayer = dynamic_cast< CMomentumPlayer* >(CBaseEntity::Instance(pEdict));
    Assert(pPlayer);

    if (!pPlayer)
    {
        return;
    }

    pPlayer->InitialSpawn();

    if (!bLoadGame)
    {
        pPlayer->Spawn();
    }
}


/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
    if (g_pGameRules) // this function may be called before the world has spawned, and the game rules initialized
        return g_pGameRules->GetGameDescription();
    else
        return "Momentum";
}

//-----------------------------------------------------------------------------
// Purpose: Given a player and optional name returns the entity of that 
//			classname that the player is nearest facing
//			
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity* FindEntity(edict_t *pEdict, char *classname)
{
    // If no name was given set bits based on the picked
    if (FStrEq(classname, ""))
    {
        return (FindPickerEntityClass(static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname));
    }
    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Precache game-specific models & sounds
//-----------------------------------------------------------------------------
void ClientGamePrecache(void)
{
    //MOM_TODO: Precache all of the mod-related sounds here



    /*CBaseEntity::PrecacheModel("models/player.mdl");
    CBaseEntity::PrecacheModel("models/gibs/agibs.mdl");
    CBaseEntity::PrecacheModel("models/weapons/v_hands.mdl");

    CBaseEntity::PrecacheScriptSound("HUDQuickInfo.LowAmmo");
    CBaseEntity::PrecacheScriptSound("HUDQuickInfo.LowHealth");

    CBaseEntity::PrecacheScriptSound("FX_AntlionImpact.ShellImpact");
    CBaseEntity::PrecacheScriptSound("Missile.ShotDown");
    CBaseEntity::PrecacheScriptSound("Bullets.DefaultNearmiss");
    CBaseEntity::PrecacheScriptSound("Bullets.GunshipNearmiss");
    CBaseEntity::PrecacheScriptSound("Bullets.StriderNearmiss");

    CBaseEntity::PrecacheScriptSound("Geiger.BeepHigh");
    CBaseEntity::PrecacheScriptSound("Geiger.BeepLow");*/
}


// called by ClientKill and DeadThink
void respawn(CBaseEntity *pEdict, bool fCopyCorpse)
{
    pEdict->Spawn();
}

void GameStartFrame(void)
{
    VPROF("GameStartFrame()");
    if (g_fGameOver)
        return;

    gpGlobals->teamplay = (teamplay.GetInt() != 0);
}

//=========================================================
// instantiate the proper game rules object
//=========================================================
void InstallGameRules()
{
    CreateGameRulesObject("CMomentum");
}