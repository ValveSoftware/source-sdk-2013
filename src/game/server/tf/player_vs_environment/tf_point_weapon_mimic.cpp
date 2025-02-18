//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_projectile_rocket.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_grenade_pipebomb.h"

class CTFPointWeaponMimic : public CPointEntity
{
	DECLARE_CLASS( CTFPointWeaponMimic, CPointEntity );
public:
	DECLARE_DATADESC();

	CTFPointWeaponMimic();
	~CTFPointWeaponMimic();
	virtual void Spawn();

	void InputFireOnce( inputdata_t& inputdata );
	void InputFireMultiple( inputdata_t& inputdata );
	void DetonateStickies( inputdata_t& inputdata );
private:
	void Fire();

	void FireRocket();
	void FireGrenade();
	void FireArrow();
	void FireStickyGrenade();

	enum eWeaponType
	{
		WEAPON_STANDARD_ROCKET,
		WEAPON_STANDARD_GRENADE,
		WEAPON_STANDARD_ARROW,
		WEAPON_STICKY_GRENADE,

		WEAPON_TYPES
	};

	QAngle GetFiringAngles() const;
	float GetSpeed() const;

	int m_nWeaponType;
	bool m_bContinousFire;

	// Effects for firing
	const char* m_pzsFireSound;
	const char* m_pzsFireParticles;

	// Override/defaults for the projectile/bullets
	const char* m_pzsModelOverride;
	float		m_flModelScale;
	float		m_flSpeedMin;
	float		m_flSpeedMax;
	float		m_flDamage;
	float		m_flSplashRadius;
	float		m_flSpreadAngle;
	bool		m_bCrits;

	// List of active pipebombs
	typedef CHandle<CTFGrenadePipebombProjectile>	PipebombHandle;
	CUtlVector<PipebombHandle>		m_Pipebombs;
};

LINK_ENTITY_TO_CLASS( tf_point_weapon_mimic, CTFPointWeaponMimic );

// Data Description
BEGIN_DATADESC( CTFPointWeaponMimic )

	// Keyfields
	DEFINE_KEYFIELD( m_nWeaponType, FIELD_INTEGER, "WeaponType" ),
	DEFINE_KEYFIELD( m_pzsFireSound, FIELD_SOUNDNAME, "FireSound" ),
	DEFINE_KEYFIELD( m_pzsFireParticles, FIELD_STRING, "ParticleEffect" ),
	DEFINE_KEYFIELD( m_pzsModelOverride, FIELD_MODELNAME, "ModelOverride" ),
	DEFINE_KEYFIELD( m_flModelScale, FIELD_FLOAT, "ModelScale" ),
	DEFINE_KEYFIELD( m_flSpeedMin, FIELD_FLOAT, "SpeedMin" ),
	DEFINE_KEYFIELD( m_flSpeedMax, FIELD_FLOAT, "SpeedMax" ),
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),
	DEFINE_KEYFIELD( m_flSplashRadius, FIELD_FLOAT, "SplashRadius" ),
	DEFINE_KEYFIELD( m_flSpreadAngle, FIELD_FLOAT, "SpreadAngle" ),
	DEFINE_KEYFIELD( m_bCrits, FIELD_BOOLEAN, "Crits" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "FireOnce", InputFireOnce ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "FireMultiple", InputFireMultiple ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DetonateStickies", DetonateStickies ),
END_DATADESC()



CTFPointWeaponMimic::CTFPointWeaponMimic()
: m_pzsModelOverride( NULL )
{
}

CTFPointWeaponMimic::~CTFPointWeaponMimic()
{
}


void CTFPointWeaponMimic::Spawn()
{
	BaseClass::Spawn();

	if( m_pzsModelOverride )
	{
		PrecacheModel( m_pzsModelOverride );
	}

	ChangeTeam( TF_TEAM_BLUE );
}


void CTFPointWeaponMimic::InputFireOnce( inputdata_t& inputdata )
{
	Fire();
}

void CTFPointWeaponMimic::InputFireMultiple( inputdata_t& inputdata )
{
	int nNumFires = Max( 1, abs(inputdata.value.Int()) );

	while( nNumFires-- )
	{
		Fire();
	}
}

void CTFPointWeaponMimic::DetonateStickies( inputdata_t& inputdata )
{
	int count = m_Pipebombs.Count();

	for ( int i = 0; i < count; i++ )
	{
		CTFGrenadePipebombProjectile *pTemp = m_Pipebombs[i];
		if ( pTemp )
		{
			//This guy will die soon enough.
			if ( pTemp->IsEffectActive( EF_NODRAW ) )
				continue;
	
			pTemp->Detonate();
		}
	}

	m_Pipebombs.Purge();
}


void CTFPointWeaponMimic::Fire()
{
	Assert( m_nWeaponType >= 0 && m_nWeaponType < WEAPON_TYPES );

	switch( m_nWeaponType )
	{
	case WEAPON_STANDARD_ROCKET:
		FireRocket();
		break;
	case WEAPON_STANDARD_GRENADE:
		FireGrenade();
		break;
	case WEAPON_STANDARD_ARROW:
		FireArrow();
		break;
	case WEAPON_STICKY_GRENADE:
		FireStickyGrenade();
		break;
	}
}

void CTFPointWeaponMimic::FireRocket()
{
	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create( this, GetAbsOrigin(), GetFiringAngles(), this, NULL);

	if ( pProjectile )
	{
		if( m_pzsModelOverride )
		{
			pProjectile->SetModel( m_pzsModelOverride );
		}
		pProjectile->ChangeTeam( TF_TEAM_BLUE );
		pProjectile->SetCritical( m_bCrits );
		pProjectile->SetDamage( m_flDamage );
		Vector vVelocity = pProjectile->GetAbsVelocity().Normalized() * GetSpeed();
		pProjectile->SetAbsVelocity( vVelocity );	
		pProjectile->SetupInitialTransmittedGrenadeVelocity( vVelocity );
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS );
	}
}

void CTFPointWeaponMimic::FireGrenade()
{
	QAngle vFireAngles = GetFiringAngles();
	Vector vForward, vUp;
	AngleVectors( vFireAngles, &vForward, NULL, &vUp );
	Vector vVelocity( vForward * GetSpeed() );

	CTFGrenadePipebombProjectile *pGrenade = static_cast<CTFGrenadePipebombProjectile*>( CBaseEntity::CreateNoSpawn( "tf_projectile_pipe", GetAbsOrigin(), vFireAngles, this ) );
	if ( pGrenade )
	{
		DispatchSpawn( pGrenade );
		if( m_pzsModelOverride )
		{
			pGrenade->SetModel( m_pzsModelOverride );
		}
		pGrenade->InitGrenade( vVelocity, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), NULL, m_flDamage, m_flSplashRadius );
		pGrenade->ChangeTeam( TF_TEAM_BLUE );
		pGrenade->m_nSkin = 1;
		pGrenade->SetDetonateTimerLength( 2.f );
		pGrenade->SetModelScale( m_flModelScale );
		pGrenade->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );  // we want to use collision_group_rockets so we don't ever collide with players
		pGrenade->SetDamage( m_flDamage );
		pGrenade->SetFullDamage( m_flDamage );
		pGrenade->SetDamageRadius( m_flSplashRadius );
		pGrenade->SetCritical( m_bCrits );
		vVelocity = pGrenade->GetAbsVelocity().Normalized() * GetSpeed();
		pGrenade->SetAbsVelocity( vVelocity );	
		pGrenade->SetupInitialTransmittedGrenadeVelocity( vVelocity );
	}
}

void CTFPointWeaponMimic::FireArrow()
{
	CTFProjectile_Arrow *pProjectile = CTFProjectile_Arrow::Create( GetAbsOrigin(), GetFiringAngles(), 2000, 0.7f, TF_PROJECTILE_ARROW, this, NULL );

	if ( pProjectile )
	{
		if( m_pzsModelOverride )
		{
			pProjectile->SetModel( m_pzsModelOverride );
		}
		pProjectile->ChangeTeam( TF_TEAM_BLUE );
		pProjectile->SetCritical( m_bCrits );
		pProjectile->SetDamage( m_flDamage );
		Vector vVelocity = pProjectile->GetAbsVelocity().Normalized() * GetSpeed();
		pProjectile->SetAbsVelocity( vVelocity );	
		pProjectile->SetupInitialTransmittedGrenadeVelocity( vVelocity );
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ROCKET_BUT_NOT_WITH_OTHER_ROCKETS );
	}
}

void CTFPointWeaponMimic::FireStickyGrenade()
{
	QAngle vFireAngles = GetFiringAngles();
	Vector vForward, vUp;
	AngleVectors( vFireAngles, &vForward, NULL, &vUp );
	Vector vVelocity( vForward * GetSpeed() );

	CTFGrenadePipebombProjectile *pGrenade = static_cast<CTFGrenadePipebombProjectile*>( CBaseEntity::CreateNoSpawn( "tf_projectile_pipe", GetAbsOrigin(), vFireAngles, this ) );
	if ( pGrenade )
	{
		pGrenade->m_bDefensiveBomb = true;

		pGrenade->SetPipebombMode( TF_GL_MODE_REMOTE_DETONATE );
		pGrenade->SetModelScale( m_flModelScale );
		pGrenade->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );  // we want to use collision_group_rockets so we don't ever collide with players
		pGrenade->SetCanTakeDamage( false );
		DispatchSpawn( pGrenade );
		if( m_pzsModelOverride )
		{
			pGrenade->SetModel( m_pzsModelOverride );
		}
		else
		{
			pGrenade->SetModel( "models/weapons/w_models/w_stickybomb_d.mdl" );
		}

		pGrenade->InitGrenade( vVelocity, AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ), NULL, m_flDamage, m_flSplashRadius );
		vVelocity = pGrenade->GetAbsVelocity().Normalized() * GetSpeed();
		pGrenade->SetAbsVelocity( vVelocity );	
		pGrenade->SetupInitialTransmittedGrenadeVelocity( vVelocity );

		pGrenade->SetDamage( m_flDamage );
		pGrenade->SetFullDamage( m_flDamage );
		pGrenade->SetDamageRadius( m_flSplashRadius );
		pGrenade->SetCritical( m_bCrits );
		pGrenade->ChangeTeam( TF_TEAM_BLUE );
		pGrenade->m_nSkin = 1;

		m_Pipebombs.AddToTail( pGrenade );
	}
}

QAngle CTFPointWeaponMimic::GetFiringAngles() const
{
	// No spread?  Straight along our angles, then
	QAngle angles = GetAbsAngles();
	if( m_flSpreadAngle == 0 )
		return angles;

	Vector vForward, vRight, vUp;
	AngleVectors( angles, &vForward, &vRight, &vUp );

	// Rotate around up by half the spread input, then rotate around the original forward by +-180
	float flHalfSpread = m_flSpreadAngle / 2.f;
	VMatrix mtxRotateAroundUp		= SetupMatrixAxisRot( vUp,		RandomFloat( -flHalfSpread, flHalfSpread ) );
	VMatrix mtxRotateAroundForward	= SetupMatrixAxisRot( vForward,	RandomFloat( -180, 180 ) );

	// Rotate forward
	VMatrix mtxSpreadRot;
	MatrixMultiply( mtxRotateAroundForward, mtxRotateAroundUp, mtxSpreadRot );
	vForward = mtxSpreadRot * vForward;

	// Back to angles
	VectorAngles( vForward, vUp, angles );

	return angles;

}

float CTFPointWeaponMimic::GetSpeed() const
{
	return RandomFloat( m_flSpeedMin, m_flSpeedMax );
}