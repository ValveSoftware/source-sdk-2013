//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef C_SDK_PLAYER_H
#define C_SDK_PLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "sdk_playeranimstate.h"
#include "c_baseplayer.h"
#include "sdk_shareddefs.h"
#include "baseparticleentity.h"


class C_SDKPlayer : public C_BasePlayer, public ISDKPlayerAnimStateHelpers
{
public:
	DECLARE_CLASS( C_SDKPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_SDKPlayer();
	~C_SDKPlayer();

	static C_SDKPlayer* GetLocalSDKPlayer();

	virtual const QAngle& GetRenderAngles();
	virtual void UpdateClientSideAnimation();
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );


// Called by shared code.
public:
	
	// ISDKPlayerAnimState overrides.
	virtual CWeaponSDKBase* SDKAnim_GetActiveWeapon();
	virtual bool SDKAnim_CanMove();

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	bool ShouldDraw();

	ISDKPlayerAnimState *m_PlayerAnimState;

	QAngle	m_angEyeAngles;
	CInterpolatedVar< QAngle >	m_iv_angEyeAngles;

	CNetworkVar( int, m_iThrowGrenadeCounter );	// used to trigger grenade throw animations.
	CNetworkVar( int, m_iShotsFired );	// number of shots fired recently

	EHANDLE	m_hRagdoll;

	CWeaponSDKBase *GetActiveSDKWeapon() const;

	C_BaseAnimating *BecomeRagdollOnClient();
	IRagdoll* C_SDKPlayer::GetRepresentativeRagdoll() const;

	void FireBullet( 
		Vector vecSrc, 
		const QAngle &shootAngles, 
		float vecSpread, 
		int iDamage, 
		int iBulletType,
		CBaseEntity *pevAttacker,
		bool bDoEffects,
		float x,
		float y );

private:
	C_SDKPlayer( const C_SDKPlayer & );
};


inline C_SDKPlayer* ToSDKPlayer( CBaseEntity *pPlayer )
{
	Assert( dynamic_cast< C_SDKPlayer* >( pPlayer ) != NULL );
	return static_cast< C_SDKPlayer* >( pPlayer );
}


#endif // C_SDK_PLAYER_H
