//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_PLAYER_SYSTEM_H
#define BM_PLAYER_SYSTEM_H

class CTFPlayer;
class CUserCmd;

#ifdef SOURCESDK

void BM_ResetGridAlign( void );
void BM_MarkGridAligned( void );
void BM_AutoAlignGridFromSpawns( void );
void BM_ConfigureMatch( void );
void BM_TickMatch( void );
bool BM_OnPlayerSpawn( CTFPlayer *pPlayer );
void BM_PlayerRunCommand( CTFPlayer *pPlayer, CUserCmd *ucmd );
void BM_SnapPlayerToGrid( CTFPlayer *pPlayer );
void BM_RespawnAllPlayers( void );
void BM_EnsurePlayerInArena( CTFPlayer *pPlayer );
bool BM_UseGridMovement( CTFPlayer *pPlayer );
void BM_ApplySkyPlayMovement( CTFPlayer *pPlayer );
bool BM_IsPlayerMovementUnlocked( CTFPlayer *pPlayer );
void BM_SetPlayerMovementUnlocked( CTFPlayer *pPlayer, bool bUnlocked );
bool BM_IsFreeForAll( void );
int BM_GetPlayerSpawnSlot( CTFPlayer *pPlayer );
void BM_EnsurePlayerJoinedMatch( CTFPlayer *pPlayer );

#endif

#endif
