//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include <KeyValues.h>
#include "tf_weapon_parse.h"
#include "tf_shareddefs.h"
#include "tf_playerclass_shared.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CTFWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponInfo::CTFWeaponInfo()
{
	m_WeaponData[0].Init();
	m_WeaponData[1].Init();

	m_bGrenade = false;
	m_flDamageRadius = 0.0f;
	m_flPrimerTime = 0.0f;
	m_bSuppressGrenTimer = false;
	m_bLowerWeapon = false;

	m_bHasTeamSkins_Viewmodel = false;
	m_bHasTeamSkins_Worldmodel = false;

	m_szMuzzleFlashModel[0] = '\0';
	m_flMuzzleFlashModelDuration = 0;
	m_szMuzzleFlashParticleEffect[0] = '\0';

	m_szTracerEffect[0] = '\0';

	m_szBrassModel[0] = '\0';
	m_bDoInstantEjectBrass = true;

	m_szExplosionSound[0] = '\0';
	m_szExplosionEffect[0] = '\0';
	m_szExplosionPlayerEffect[0] = '\0';
	m_szExplosionWaterEffect[0] = '\0';

	m_iWeaponType = TF_WPN_TYPE_PRIMARY;
}

CTFWeaponInfo::~CTFWeaponInfo()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	int i;

	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Primary fire mode.
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage				= pKeyValuesData->GetInt( "Damage", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange				= pKeyValuesData->GetFloat( "Range", 8192.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot		= pKeyValuesData->GetInt( "BulletsPerShot", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread				= pKeyValuesData->GetFloat( "Spread", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle			= pKeyValuesData->GetFloat( "PunchAngle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay		= pKeyValuesData->GetFloat( "TimeFireDelay", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "TimeIdle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty		= pKeyValuesData->GetFloat( "TimeIdleEmpy", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "TimeReloadStart", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload			= pKeyValuesData->GetFloat( "TimeReload", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "DrawCrosshair", 1 ) > 0;
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot			= pKeyValuesData->GetInt( "AmmoPerShot", 1 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "UseRapidFireCrits", 0 ) != 0 );

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = TF_PROJECTILE_NONE;
	const char *pszProjectileType = pKeyValuesData->GetString( "ProjectileType", "projectile_none" );

	for ( i=0;i<TF_NUM_PROJECTILES;i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = i;
			break;
		}
	}	 

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flProjectileSpeed	= pKeyValuesData->GetFloat( "ProjectileSpeed", 0.0f );

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSmackDelay			= pKeyValuesData->GetFloat( "SmackDelay", 0.2f );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSmackDelay		= pKeyValuesData->GetFloat( "Secondary_SmackDelay", 0.2f );

	m_bDoInstantEjectBrass = ( pKeyValuesData->GetInt( "DoInstantEjectBrass", 1 ) != 0 );
	const char *pszBrassModel = pKeyValuesData->GetString( "BrassModel", NULL );
	if ( pszBrassModel )
	{
		Q_strncpy( m_szBrassModel, pszBrassModel, sizeof( m_szBrassModel ) );
	}

	// Secondary fire mode.
	// Inherit from primary fire mode
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nDamage			= pKeyValuesData->GetInt( "Secondary_Damage", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flRange			= pKeyValuesData->GetFloat( "Secondary_Range", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nBulletsPerShot	= pKeyValuesData->GetInt( "Secondary_BulletsPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSpread			= pKeyValuesData->GetFloat( "Secondary_Spread", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flPunchAngle		= pKeyValuesData->GetFloat( "Secondary_PunchAngle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeFireDelay	= pKeyValuesData->GetFloat( "Secondary_TimeFireDelay", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "Secondary_TimeIdle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdleEmpty	= pKeyValuesData->GetFloat( "Secondary_TimeIdleEmpy", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "Secondary_TimeReloadStart", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload		= pKeyValuesData->GetFloat( "Secondary_TimeReload", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "Secondary_DrawCrosshair", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair ) > 0;
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iAmmoPerShot		= pKeyValuesData->GetInt( "Secondary_AmmoPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "Secondary_UseRapidFireCrits", 0 ) != 0 );

	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile;
	pszProjectileType = pKeyValuesData->GetString( "Secondary_ProjectileType", "projectile_none" );

	for ( i=0;i<TF_NUM_PROJECTILES;i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = i;
			break;
		}
	}	

	const char *pszWeaponType = pKeyValuesData->GetString( "WeaponType" );

	if ( !Q_strcmp( pszWeaponType, "primary" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_PRIMARY;
	}
	else if ( !Q_strcmp( pszWeaponType, "secondary" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_SECONDARY;
	}
	else if ( !Q_strcmp( pszWeaponType, "melee" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_MELEE;
	}
	else if ( !Q_strcmp( pszWeaponType, "grenade" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_GRENADE;
	}
	else if ( !Q_strcmp( pszWeaponType, "building" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_BUILDING;
	}
	else if ( !Q_strcmp( pszWeaponType, "pda" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_PDA;
	}
	else if ( !Q_strcmp( pszWeaponType, "item1" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_ITEM1;
	}
	else if ( !Q_strcmp( pszWeaponType, "item2" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_ITEM2;
	}

	// Grenade data.
	m_bGrenade				= ( pKeyValuesData->GetInt( "Grenade", 0 ) != 0 );
	m_flDamageRadius		= pKeyValuesData->GetFloat( "DamageRadius", 0.0f );
	m_flPrimerTime			= pKeyValuesData->GetFloat( "PrimerTime", 0.0f );
	m_bSuppressGrenTimer	= ( pKeyValuesData->GetInt( "PlayGrenTimer", 1 ) <= 0 );

	m_bLowerWeapon			= ( pKeyValuesData->GetInt( "LowerMainWeapon", 0 ) != 0 );
	m_bHasTeamSkins_Viewmodel	= ( pKeyValuesData->GetInt( "HasTeamSkins_Viewmodel", 0 ) != 0 );
	m_bHasTeamSkins_Worldmodel	= ( pKeyValuesData->GetInt( "HasTeamSkins_Worldmodel", 0 ) != 0 );

	// Model muzzleflash
	const char *pszMuzzleFlashModel = pKeyValuesData->GetString( "MuzzleFlashModel", NULL );

	if ( pszMuzzleFlashModel )
	{
		Q_strncpy( m_szMuzzleFlashModel, pszMuzzleFlashModel, sizeof( m_szMuzzleFlashModel ) );
	}

	m_flMuzzleFlashModelDuration = pKeyValuesData->GetFloat( "MuzzleFlashModelDuration", 0.2 );

	const char *pszMuzzleFlashParticleEffect = pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL );

	if ( pszMuzzleFlashParticleEffect )
	{
		Q_strncpy( m_szMuzzleFlashParticleEffect, pszMuzzleFlashParticleEffect, sizeof( m_szMuzzleFlashParticleEffect ) );
	}

	// Tracer particle effect
	const char *pszTracerEffect = pKeyValuesData->GetString( "TracerEffect", NULL );

	if ( pszTracerEffect )
	{
		Q_strncpy( m_szTracerEffect, pszTracerEffect, sizeof( m_szTracerEffect ) );
	}


	// Explosion effects (used for grenades)
	const char *pszSound = pKeyValuesData->GetString( "ExplosionSound", NULL );
	if ( pszSound )
	{
		Q_strncpy( m_szExplosionSound, pszSound, sizeof( m_szExplosionSound ) );
	}

	const char *pszEffect = pKeyValuesData->GetString( "ExplosionEffect", NULL );
	if ( pszEffect )
	{
		Q_strncpy( m_szExplosionEffect, pszEffect, sizeof( m_szExplosionEffect ) );
	}

	pszEffect = pKeyValuesData->GetString( "ExplosionPlayerEffect", NULL );
	if ( pszEffect )
	{
		Q_strncpy( m_szExplosionPlayerEffect, pszEffect, sizeof( m_szExplosionPlayerEffect ) );
	}

	pszEffect = pKeyValuesData->GetString( "ExplosionWaterEffect", NULL );
	if ( pszEffect )
	{
		Q_strncpy( m_szExplosionWaterEffect, pszEffect, sizeof( m_szExplosionWaterEffect ) );
	}

	m_bDontDrop = ( pKeyValuesData->GetInt( "DontDrop", 0 ) > 0 );
}
