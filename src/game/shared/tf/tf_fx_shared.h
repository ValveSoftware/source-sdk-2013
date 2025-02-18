//========= Copyright Valve Corporation, All rights reserved. ============//
//
//  
//
//=============================================================================
#ifndef TF_FX_SHARED_H
#define TF_FX_SHARED_H
#ifdef _WIN32
#pragma once
#endif

// Client specific.
#ifdef CLIENT_DLL
#include "c_tf_player.h"
// Server specific.
#else
#include "tf_player.h"
#endif

void FX_WeaponSound ( int iPlayer, WeaponSound_t soundType, const Vector &vecOrigin, CTFWeaponInfo *pWeaponInfo );
void StartGroupingSounds( void );
void EndGroupingSounds( void );
bool IsFixedWeaponSpreadEnabled( CTFWeaponBase *pWeapon = NULL );

// This runs on both the client and the server.
// On the server, it only does the damage calculations.
// On the client, it does all the effects.
void FX_FireBullets( CTFWeaponBase *pWpn, int iPlayer, const Vector &vecOrigin, const QAngle &vecAngles,
					 int iWeapon, int iMode, int iSeed, float flSpread, float flDamage = -1.0f, bool bCritical = false );

#endif // TF_FX_SHARED_H
