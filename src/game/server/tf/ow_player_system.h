//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef OW_PLAYER_SYSTEM_H
#define OW_PLAYER_SYSTEM_H

class CTFPlayer;

#ifdef SOURCESDK

void OW_PlayerSystem_Init( void );
void OW_OnPlayerSpawn( CTFPlayer *pPlayer );
void OW_OnPlayerPostThink( CTFPlayer *pPlayer );
void OW_ApplyHeroFromClass( CTFPlayer *pPlayer );
void OW_SetPlayerHero( CTFPlayer *pPlayer, int iHeroId );
bool OW_UseAbility( CTFPlayer *pPlayer, int iSlot );
bool OW_UseUltimate( CTFPlayer *pPlayer );
void OW_TickUltCharge( CTFPlayer *pPlayer, float flDamageDealt, float flDamageTaken, float flHealing );
void OW_PrintPlayerStatus( CTFPlayer *pPlayer );
void OW_EnsureAllBotsHaveAI( void );
void OW_OnHumanChangedTeam( CTFPlayer *pPlayer, int iNewTeam, int iOldTeam );

#endif

#endif
