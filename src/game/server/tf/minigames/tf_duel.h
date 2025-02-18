//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Duel MiniGame
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_DUEL_H
#define TF_DUEL_H

class CTFPlayer;

bool DuelMiniGame_IsInDuel( CTFPlayer *pPlayer );
int  DuelMiniGame_GetRequiredPlayerClass( CTFPlayer *pPlayer );
void DuelMiniGame_NotifyKill( CTFPlayer *pKiller, CTFPlayer *pVictim );
void DuelMiniGame_NotifyAssist( CTFPlayer *pAssister, CTFPlayer *pVictim );
void DuelMiniGame_NotifyPlayerChangedTeam( CTFPlayer *pPlayer, int iNewTeam, bool bInitiatedByPlayer );
void DuelMiniGame_NotifyPlayerDisconnect( CTFPlayer *pPlayer, bool bKicked = false );
void DuelMiniGame_AssignWinners();
void DuelMiniGame_Stop();
void DuelMiniGame_LevelShutdown();

#endif // TF_DUEL_H
