//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_GRID_H
#define BM_GRID_H

#ifdef SOURCESDK

class CTFPlayer;
class CTFBMBomb;
class CTFBMCrate;

void BM_GetGridOrigin( Vector &vecGridOrigin );
float BM_GetCellSize( void );
bool BM_UseSkyPlayPlane( void );
bool BM_UseIsolatedPlayPlane( void );
float BM_GetEffectiveArenaLift( void );
bool BM_UseVoidArenaPlatform( void );
float BM_GetPlayPlaneZ( void );
void BM_WorldToCell( const Vector &vecWorld, int &iCellX, int &iCellY );
void BM_CellToWorldCenter( int iCellX, int iCellY, Vector &vecCenter );
void BM_FindFloorAtXY( const Vector &vecXY, float flRefZ, CTFPlayer *pPlayer, Vector &vecFloor );

CTFBMBomb *BM_FindBombAtCell( int iCellX, int iCellY );
CTFBMCrate *BM_FindCrateAtCell( int iCellX, int iCellY );
bool BM_IsBlastBlockedToCell( int iFromCellX, int iFromCellY, int iToCellX, int iToCellY, CTFPlayer *pPlayer );
void BM_DestroyCrateAtCell( int iCellX, int iCellY );
void BM_BuildArena( bool bWarpAllPlayers = false );
void BM_ClearArena( void );
void BM_RemoveAllBombs( void );
bool BM_CellBlocksMovement( int iCellX, int iCellY );
bool BM_CellBlocksBlast( int iCellX, int iCellY );
void BM_GetArenaSize( int &iWidth, int &iHeight );
void BM_WarpPlayerToArenaSpawn( CTFPlayer *pPlayer );
void BM_WarpAllPlayersToArenaSpawns( void );

#endif // SOURCESDK

#endif // BM_GRID_H
