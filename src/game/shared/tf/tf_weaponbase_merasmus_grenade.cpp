//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_merasmus_grenade.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseMerasmusGrenade, DT_TFWeaponBaseMerasmusGrenade )
LINK_ENTITY_TO_CLASS( tf_weaponbase_merasmus_grenade, CTFWeaponBaseMerasmusGrenade );

BEGIN_NETWORK_TABLE( CTFWeaponBaseMerasmusGrenade, DT_TFWeaponBaseMerasmusGrenade )
END_NETWORK_TABLE()

int CTFWeaponBaseMerasmusGrenade::GetDamageCustom()
{
	return TF_DMG_CUSTOM_MERASMUS_GRENADE;
}

int CTFWeaponBaseMerasmusGrenade::GetCustomParticleIndex()
{
	return GetParticleSystemIndex( "merasmus_dazed_explosion" );
}

#ifdef CLIENT_DLL

int CTFWeaponBaseMerasmusGrenade::DrawModel( int flags )
{
	float flAliveTime = gpGlobals->curtime - m_flSpawnTime;

	const float flNoDrawTime = 0.1f;
	const float flScaleTime = 0.1f;
	// Don't draw the rocket for the first bit of life
	if( flAliveTime < flNoDrawTime )
	{
		return 0;
	}

	// Scale up as we become visible
	if( flAliveTime < ( flNoDrawTime + flScaleTime ) )
	{
		float flSize = RemapVal( flAliveTime, flNoDrawTime, flNoDrawTime + flScaleTime, 0.f, 1.f );
		SetModelScale( flSize );
	}
	
	return BaseClass::DrawModel( flags );
}

#endif
