//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponPistol C_WeaponPistol
	#include "c_sdk_player.h"

#else

	#include "sdk_player.h"

#endif


class CWeaponPistol : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponPistol, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponPistol();

	virtual SDKWeaponID GetWeaponID( void ) const		{ return SDK_WEAPON_PISTOL; }
	virtual int GetFireMode() const { return FM_SEMIAUTOMATIC; }

private:

	CWeaponPistol( const CWeaponPistol & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE( CWeaponPistol, DT_WeaponPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pistol, CWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_pistol );



CWeaponPistol::CWeaponPistol()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponPistol::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_DOD_STAND_IDLE_PISTOL,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_DOD_CROUCH_IDLE_PISTOL,				false },
	{ ACT_MP_PRONE_IDLE,					ACT_DOD_PRONE_AIM_PISTOL,				false },

	{ ACT_MP_RUN,							ACT_DOD_RUN_AIM_PISTOL,					false },
	{ ACT_MP_WALK,							ACT_DOD_WALK_AIM_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,					ACT_DOD_CROUCHWALK_AIM_PISTOL,			false },
	{ ACT_MP_PRONE_CRAWL,					ACT_DOD_PRONEWALK_IDLE_PISTOL,			false },
	{ ACT_SPRINT,							ACT_DOD_SPRINT_IDLE_PISTOL,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PISTOL,			false },
	{ ACT_MP_ATTACK_PRONE_PRIMARYFIRE,		ACT_DOD_PRIMARYATTACK_PRONE_PISTOL,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_DOD_RELOAD_PISTOL,					false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_DOD_RELOAD_CROUCH_PISTOL,			false },
	{ ACT_MP_RELOAD_PRONE,					ACT_DOD_RELOAD_PRONE_PISTOL,			false },
};

IMPLEMENT_ACTTABLE( CWeaponPistol );

