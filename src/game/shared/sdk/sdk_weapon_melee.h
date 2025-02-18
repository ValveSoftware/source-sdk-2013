//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base code for any melee based weapon
//
//=====================================================================================//

#ifndef SDK_WEAPON_MELEE_H
#define SDK_WEAPON_MELEE_H

#ifdef _WIN32
#pragma once
#endif


#if defined( CLIENT_DLL )
#define CWeaponSDKMelee C_WeaponSDKMelee
#endif

//=========================================================
// CBaseHLBludgeonWeapon 
//=========================================================
class CWeaponSDKMelee : public CWeaponSDKBase
{
	DECLARE_CLASS( CWeaponSDKMelee, CWeaponSDKBase );
public:
	CWeaponSDKMelee();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual	void	Spawn( void );
	virtual	void	Precache( void );
	
	//Attack functions
	virtual	void	PrimaryAttack( void );
	virtual	void	SecondaryAttack( void );
	
	virtual void	ItemPostFrame( void );

	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity( void )	{	return	ACT_VM_HITCENTER;	}
	virtual Activity	GetSecondaryAttackActivity( void )	{	return	ACT_VM_HITCENTER2;	}

	virtual float	GetRange( void )								{	return	32.0f;	}
	virtual	float	GetDamageForActivity( Activity hitActivity )	{	return	GetSDKWpnData().m_iDamage;	}

	CWeaponSDKMelee( const CWeaponSDKMelee & );

protected:
	virtual	void	ImpactEffect( trace_t &trace );

private:
	bool			ImpactWater( const Vector &start, const Vector &end );
	void			Swing( int bIsSecondary );
	void			Hit( trace_t &traceHit, Activity nHitActivity );
	Activity		ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CSDKPlayer *pOwner );
};


#endif // SDK_WEAPON_MELEE_H
