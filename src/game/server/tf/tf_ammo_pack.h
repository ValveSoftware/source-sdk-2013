//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_AMMO_PACK_H
#define TF_AMMO_PACK_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"

typedef enum
{	
	AP_NORMAL = 0,
	AP_HALLOWEEN,
	AP_CHRISTMAS,

} AmmoPackType_t;

class CTFAmmoPack : public CItem
{
public:
	DECLARE_CLASS( CTFAmmoPack, CItem );
	DECLARE_SERVERCLASS();

	CTFAmmoPack()
	{
		m_PackType = AP_NORMAL;
	}

	virtual void Spawn();
	virtual void Precache();		

	void EXPORT DropSoundThink( void );
	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	void InitAmmoPack( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, int nSkin, bool bEmpty, bool bIsSuicide, float flAmmoRatio = 0.5f );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	int GiveAmmo( int iCount, int iAmmoType );

	void MakeEmptyPack( void ) { m_bEmptyPack = true; }
	void MakeHolidayPack( void );
	void SetBonusScale( float flBonusScale = 1.f );
	void SetPickupThinkTime( float flNewThinkTime );

	static CTFAmmoPack *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName );

	float GetCreationTime( void ) { return m_flCreationTime; }
	void  SetInitialVelocity( Vector &vecVelocity );
	void  SetHealthInstead( bool bHealth ) { m_bHealthInstead = bHealth; }

	const char* MakeHolidayAmmoPack( const char* inModelName, CBaseEntity *pOwner, const CTakeDamageInfo &info );

	bool m_bObjGib;

private:
	int m_iAmmo[TF_AMMO_COUNT];

	float m_flCreationTime;
	float m_flAmmoRatio;

	bool m_bEmptyPack;		// If true, the pack gives nothing when picked up.

	bool m_bHealthInstead;	// If true, the pack gives health instead of ammo
	bool m_bAllowOwnerPickup;
	bool m_bNoPickup;
	float m_flBonusScale;
	AmmoPackType_t m_PackType;
	CNetworkVector( m_vecInitialVelocity );

private:
	CTFAmmoPack( const CTFAmmoPack & );

	DECLARE_DATADESC();
};

#endif //TF_AMMO_PACK_H
