//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_ARENA_H
#define BM_ARENA_H

#ifdef SOURCESDK

bool BM_IsArenaActive( void );
bool BM_IsInsideArenaCell( int iCellX, int iCellY );
bool BM_IsHardWallCell( int iCellX, int iCellY );
CBaseEntity *BM_GetSkySpawnEntity( CTFPlayer *pPlayer );
void BM_RemoveAllBombs( void );

#endif // SOURCESDK

#endif // BM_ARENA_H
