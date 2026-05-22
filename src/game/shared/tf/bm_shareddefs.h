//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef BM_SHAREDDEFS_H
#define BM_SHAREDDEFS_H

#ifdef SOURCESDK

#define TF_FF_MODE_STOCK		0
#define TF_FF_MODE_OW			1
#define TF_FF_MODE_RIM			2
#define TF_FF_MODE_BOMBERMAN	3

#define BM_MAX_SPAWN_SLOTS_PER_TEAM	8

struct BM_SpawnOffset_t
{
	int m_iDeltaX;
	int m_iDeltaY;
};

// Corner clusters: RED high-Y corner, BLU low-Y corner (diagonal opponents).
static const BM_SpawnOffset_t g_BMRedSpawnOffsets[BM_MAX_SPAWN_SLOTS_PER_TEAM] = {
	{ 0, 0 }, { 1, 0 }, { 0, -1 }, { 1, -1 }, { 0, -2 }, { 1, -2 }, { 2, 0 }, { 2, -1 },
};

static const BM_SpawnOffset_t g_BMBluSpawnOffsets[BM_MAX_SPAWN_SLOTS_PER_TEAM] = {
	{ 0, 0 }, { -1, 0 }, { 0, 1 }, { -1, 1 }, { 0, 2 }, { -1, 2 }, { -2, 0 }, { -2, 1 },
};

inline void BM_GetSpawnCellForSlot( bool bBlueTeam, int iSlot, int iArenaWidth, int iArenaHeight, int &iCellX, int &iCellY )
{
	const int iMaxY = iArenaHeight - 2;
	const int iMaxX = iArenaWidth - 2;
	const int iClampedSlot = clamp( iSlot, 0, BM_MAX_SPAWN_SLOTS_PER_TEAM - 1 );

	if ( bBlueTeam )
	{
		const BM_SpawnOffset_t &offset = g_BMBluSpawnOffsets[iClampedSlot];
		iCellX = iMaxX + offset.m_iDeltaX;
		iCellY = 1 + offset.m_iDeltaY;
	}
	else
	{
		const BM_SpawnOffset_t &offset = g_BMRedSpawnOffsets[iClampedSlot];
		iCellX = 1 + offset.m_iDeltaX;
		iCellY = iMaxY + offset.m_iDeltaY;
	}

	iCellX = clamp( iCellX, 1, iMaxX );
	iCellY = clamp( iCellY, 1, iMaxY );
}

#endif // SOURCESDK

#endif // BM_SHAREDDEFS_H
