//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef ENTITY_AMMOPACK_H
#define ENTITY_AMMOPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "tf_gamerules.h"

#define TF_AMMOPACK_SMALL_BDAY		"models/items/ammopack_small_bday.mdl"
#define TF_AMMOPACK_MEDIUM_BDAY		"models/items/ammopack_medium_bday.mdl"
#define TF_AMMOPACK_LARGE_BDAY		"models/items/ammopack_large_bday.mdl"

//=============================================================================
//
// CTF AmmoPack class.
//

class CAmmoPack : public CTFPowerup
{
public:
	DECLARE_CLASS( CAmmoPack, CTFPowerup );

	void	Spawn( void );
	virtual void Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }
	virtual const char *GetAmmoPackName( void ) { return "ammopack_large"; }

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_AMMOPACK_LARGE_BDAY ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{ 
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_AMMOPACK_LARGE_BDAY;
		}

		return "models/items/ammopack_large.mdl"; 
	}
};

class CAmmoPackSmall : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackSmall, CAmmoPack );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetAmmoPackName( void ) { return "ammopack_small"; }

	virtual void Precache( void )
	{
		PrecacheModel( TF_AMMOPACK_SMALL_BDAY ); // always precache this for PyroVision
		BaseClass::Precache();
	}

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_AMMOPACK_SMALL_BDAY ) );
		}
	}

	virtual const char *GetDefaultPowerupModel( void )
	{ 
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_AMMOPACK_SMALL_BDAY;
		}
		
		return "models/items/ammopack_small.mdl"; 
	}
};

class CAmmoPackMedium : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackMedium, CAmmoPack );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetAmmoPackName( void ) { return "ammopack_medium"; }

	virtual void Precache( void )
	{
		PrecacheModel( TF_AMMOPACK_MEDIUM_BDAY ); // always precache this for PyroVision
		BaseClass::Precache();
	}

	virtual void UpdateModelIndexOverrides( void )
	{
		if ( modelinfo )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( GetPowerupModel() ) );
			SetModelIndexOverride( VISION_MODE_PYRO, modelinfo->GetModelIndex( TF_AMMOPACK_MEDIUM_BDAY ) );
		}
	}
	 
	virtual const char *GetDefaultPowerupModel( void ) 
	{
		if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) )
		{
			return TF_AMMOPACK_MEDIUM_BDAY;
		}
		
		return "models/items/ammopack_medium.mdl"; 
	}
};

#endif // ENTITY_AMMOPACK_H


