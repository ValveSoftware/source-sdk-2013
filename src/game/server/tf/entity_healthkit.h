//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#ifndef ENTITY_HEALTHKIT_H
#define ENTITY_HEALTHKIT_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "tf_gamerules.h"

#define TF_MEDKIT_SMALL_BDAY	"models/items/medkit_small_bday.mdl"
#define TF_MEDKIT_MEDIUM_BDAY	"models/items/medkit_medium_bday.mdl"
#define TF_MEDKIT_LARGE_BDAY	"models/items/medkit_large_bday.mdl"

#define TF_MEDKIT_SMALL_HALLOWEEN	"models/props_halloween/halloween_medkit_small.mdl"
#define TF_MEDKIT_MEDIUM_HALLOWEEN  "models/props_halloween/halloween_medkit_medium.mdl"
#define TF_MEDKIT_LARGE_HALLOWEEN	"models/props_halloween/halloween_medkit_large.mdl"

//=============================================================================
//
// CTF HealthKit class.
//

DECLARE_AUTO_LIST( IHealthKitAutoList );

class CHealthKit : public CTFPowerup, public IHealthKitAutoList
{
public:
	DECLARE_CLASS( CHealthKit, CTFPowerup );

	void	Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_MEDKIT_LARGE_BDAY ) );
			SetModelIndexOverride( VISION_MODE_HALLOWEEN, modelinfo->GetModelIndex( TF_MEDKIT_LARGE_HALLOWEEN ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{ 
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_MEDKIT_LARGE_BDAY;
		}
		
		return "models/items/medkit_large.mdl";
	}

	powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }
	virtual const char *GetHealthKitName( void ) { return "medkit_large"; }

	virtual float	GetRespawnDelay( void );
};

class CHealthKitSmall : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitSmall, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetHealthKitName( void ) { return "medkit_small"; }

	virtual void Precache( void )
	{
		PrecacheModel( TF_MEDKIT_SMALL_BDAY ); // always precache this for PyroVision
		PrecacheModel( TF_MEDKIT_SMALL_HALLOWEEN ); // always precache this for Halloween Vision
		BaseClass::Precache();
	}

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_MEDKIT_SMALL_BDAY ) );
			SetModelIndexOverride( VISION_MODE_HALLOWEEN, modelinfo->GetModelIndex( TF_MEDKIT_SMALL_HALLOWEEN ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_MEDKIT_SMALL_BDAY;
		}

		return "models/items/medkit_small.mdl"; 
	}
};

class CHealthKitMedium : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthKitMedium, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetHealthKitName( void ) { return "medkit_medium"; }

	virtual void Precache( void )
	{
		PrecacheModel( TF_MEDKIT_MEDIUM_BDAY ); // always precache this for PyroVision
		PrecacheModel( TF_MEDKIT_MEDIUM_HALLOWEEN ); // always precache this for Halloween Vision
		BaseClass::Precache();
	}

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_MEDKIT_MEDIUM_BDAY ) );
			SetModelIndexOverride( VISION_MODE_HALLOWEEN, modelinfo->GetModelIndex( TF_MEDKIT_MEDIUM_HALLOWEEN ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_MEDKIT_MEDIUM_BDAY;
		}

		return "models/items/medkit_medium.mdl"; 
	}
};

//=============================================================================
//
// CTF HealthAmmoKit class. Combo health and ammo kit.
//
// In choosing whether to derive from HealthKit, AmmoPack, or neither, it seemed
// like there was more special behavior for health kits that we want to inherit (in particular
// considering a Heavy's sandvich to be a health kit) than ammo kit behavior. Deriving from
// health kit created the minimum code churn.
//

class CHealthAmmoKit : public CHealthKit
{
public:
	DECLARE_CLASS( CHealthAmmoKit, CHealthKit );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetHealthKitName( void ) { return "healthammokit"; }

	virtual bool MyTouch( CBasePlayer *pPlayer );

	virtual void Precache( void )
	{
		PrecacheModel( TF_MEDKIT_MEDIUM_BDAY ); // always precache this for PyroVision
		PrecacheModel( TF_MEDKIT_MEDIUM_HALLOWEEN ); // always precache this for Halloween Vision
		BaseClass::Precache();
	}

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_MEDKIT_MEDIUM_BDAY ) );
			SetModelIndexOverride( VISION_MODE_HALLOWEEN, modelinfo->GetModelIndex( TF_MEDKIT_MEDIUM_HALLOWEEN ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_MEDKIT_MEDIUM_BDAY;
		}

		return "models/items/medkit_medium.mdl";
	}
};

#endif // ENTITY_HEALTHKIT_H


