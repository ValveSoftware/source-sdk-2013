//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#ifndef TF_WEARABLE_ITEM_DEMOSHIELD_H
#define TF_WEARABLE_ITEM_DEMOSHIELD_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_item_wearable.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "tf_viewmodel.h"
#include "bone_setup.h"
#else
#include "tf_player.h"
#endif

#ifdef CLIENT_DLL
#define CTFWearableDemoShield C_TFWearableDemoShield
#define CTFWearableRazorback C_TFWearableRazorback
class C_TFSword;
#endif


//=============================================================================
//
// 
//
class CTFWearableDemoShield : public CTFWearable
{
	DECLARE_CLASS( CTFWearableDemoShield, CTFWearable );

public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFWearableDemoShield();

	virtual void Precache();

	void DoSpecialAction( CTFPlayer *pPlayer );
	void EndSpecialAction( CTFPlayer *pPlayer );

	// Charge
	bool CanCharge( CTFPlayer *pPlayer );
	void DoCharge( CTFPlayer *pPlayer );
	void ShieldBash( CTFPlayer *pPlayer, float flCurrentChargeMeter );

	virtual void Equip( CBasePlayer* pOwner );
	virtual void UnEquip( CBasePlayer* pOwner );

	float CalculateChargeDamage( float flCurrentChargeMeter );
	Vector GetShieldDamageForce( float flCurrentChargeMeter );

private:
#ifdef GAME_DLL
	bool m_bImpactedSomething;
#endif
};

class CTFWearableRazorback : public CTFWearable
{
	DECLARE_CLASS( CTFWearableRazorback, CTFWearable );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFWearableRazorback() {}

	// IHasGenericMeter
	virtual void OnResourceMeterFilled() OVERRIDE;
};


#endif // TF_WEARABLE_ITEM_DEMOSHIELD_H
