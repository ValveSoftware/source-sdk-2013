#ifndef TE_SHOTGUN_SHOT_H
#define TE_SHOTGUN_SHOT_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

void TE_FireBullets(
    int	iPlayerIndex,
    const Vector &vOrigin,
    const QAngle &vAngles,
    int	iWeaponID,
    int	iMode,
    int iSeed,
    float flSpread
    );

void TE_PlantBomb(int iPlayerIndex, const Vector &vOrigin);


#endif // TE_SHOTGUN_SHOT_H