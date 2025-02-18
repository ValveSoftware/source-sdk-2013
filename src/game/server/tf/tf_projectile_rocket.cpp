
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Rocket
//
//=============================================================================
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_projectile_rocket.h"
#include "tf_player.h"

//=============================================================================
//
// TF Rocket functions (Server specific).
//
#define ROCKET_MODEL "models/weapons/w_models/w_rocket.mdl"

LINK_ENTITY_TO_CLASS( tf_projectile_rocket, CTFProjectile_Rocket );
PRECACHE_REGISTER( tf_projectile_rocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Rocket, DT_TFProjectile_Rocket )

BEGIN_NETWORK_TABLE( CTFProjectile_Rocket, DT_TFProjectile_Rocket )
	SendPropBool( SENDINFO( m_bCritical ) ),
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Rocket *CTFProjectile_Rocket::Create( CBaseEntity *pLauncher, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_Rocket *pRocket = static_cast<CTFProjectile_Rocket*>( CTFBaseRocket::Create( pLauncher, "tf_projectile_rocket", vecOrigin, vecAngles, pOwner ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
		pRocket->SetEyeBallRocket( false );
		pRocket->SetSpell( false );

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pLauncher );
		bool bDirectHit = pWeapon ? ( pWeapon->GetWeaponID() == TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT ) : false;
		pRocket->SetDirectHit( bDirectHit );
	}

	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Spawn()
{
	SetModel( ROCKET_MODEL );
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Precache()
{
	int iModel = PrecacheModel( ROCKET_MODEL );
	PrecacheGibsForModel( iModel );
	PrecacheParticleSystem( "critical_rocket_blue" );
	PrecacheParticleSystem( "critical_rocket_red" );
	PrecacheParticleSystem( "eyeboss_projectile" );
	PrecacheParticleSystem( "rockettrail" );
	PrecacheParticleSystem( "rockettrail_RocketJumper" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Rocket::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Rocket::GetDamageType() 
{ 
	int iDmgType = BaseClass::GetDamageType();
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Rocket::GetDamageCustom()
{
	if ( m_bDirectHit )
	{
		return TF_DMG_CUSTOM_ROCKET_DIRECTHIT;
	}
	else if ( m_bEyeBallRocket )
	{
		return TF_DMG_CUSTOM_EYEBALL_ROCKET;
	}
	else if ( m_bSpell )
	{
		return TF_DMG_CUSTOM_SPELL_MONOCULUS;
	}
	else
		return BaseClass::GetDamageCustom();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::RocketTouch( CBaseEntity *pOther )
{
	BaseClass::RocketTouch( pOther );
		
	if (m_bCritical && pOther && pOther->IsPlayer())
	{		
		CTFPlayer *pHitPlayer = ToTFPlayer( pOther );
		int iHitPlayerTeamNumber = pHitPlayer->GetTeamNumber();
		int iRocketTeamNumber = BaseClass::GetTeamNumber();

		if (pHitPlayer->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) && !pHitPlayer->m_Shared.InCond( TF_COND_INVULNERABLE)
			&& pHitPlayer->IsAlive() && iHitPlayerTeamNumber != iRocketTeamNumber)
		{
			pHitPlayer->AwardAchievement( ACHIEVEMENT_TF_HEAVY_SURVIVE_CROCKET );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Rocket was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Rocket::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	ChangeTeam( pTFDeflector->GetTeamNumber() );
	SetLauncher( pTFDeflector->GetActiveWeapon() );

	CTFPlayer* pOldOwner = ToTFPlayer( GetOwnerEntity() );
	SetOwnerEntity( pTFDeflector );

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	if ( pTFDeflector->m_Shared.IsCritBoosted() )
	{
		SetCritical( true );
	}

	CTFWeaponBase::SendObjectDeflectedEvent( pTFDeflector, pOldOwner, GetWeaponID(), this );

	IncrementDeflected();
	m_nSkin = ( GetTeamNumber() == TF_TEAM_BLUE ) ? 1 : 0;
}
