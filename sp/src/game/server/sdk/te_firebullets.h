//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TE_FIREBULLETS_H
#define TE_FIREBULLETS_H
#ifdef _WIN32
#pragma once
#endif


void TE_FireBullets( 
	int	iPlayerIndex,
	const Vector &vOrigin,
	const QAngle &vAngles,
	int	iWeaponID,
	int	iMode,
	int iSeed,
	float flSpread 
	);

#endif // TE_FIREBULLETS_H
