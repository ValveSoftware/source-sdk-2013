//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Powerup Rune.
//
//=============================================================================//
#ifndef ENTITY_RUNE_H
#define ENTITY_RUNE_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "tf_gamerules.h"

#define TF_RUNE_STRENGTH		"models/pickups/pickup_powerup_strength.mdl"
#define TF_RUNE_RESIST			"models/pickups/pickup_powerup_defense.mdl"
#define TF_RUNE_REGEN			"models/pickups/pickup_powerup_regen.mdl"
#define TF_RUNE_HASTE			"models/pickups/pickup_powerup_haste.mdl"
#define TF_RUNE_VAMPIRE			"models/pickups/pickup_powerup_vampire.mdl"
#define TF_RUNE_REFLECT 		"models/pickups/pickup_powerup_reflect.mdl"
#define TF_RUNE_PRECISION 		"models/pickups/pickup_powerup_precision.mdl"
#define TF_RUNE_AGILITY 		"models/pickups/pickup_powerup_agility.mdl"
#define TF_RUNE_KNOCKOUT 		"models/pickups/pickup_powerup_knockout.mdl"
#define TF_RUNE_KING			"models/pickups/pickup_powerup_king.mdl"
#define TF_RUNE_PLAGUE			"models/pickups/pickup_powerup_plague.mdl"
#define TF_RUNE_SUPERNOVA		"models/pickups/pickup_powerup_supernova.mdl"

#define TF_RUNE_TEMP_CRIT		"models/pickups/pickup_powerup_crit.mdl"
#define TF_RUNE_TEMP_UBER		"models/pickups/pickup_powerup_uber.mdl"

DECLARE_AUTO_LIST( IInfoPowerupSpawnAutoList );

//=============================================================================
//
// CTF Rune class. Powerups which last the life of the player and drop when they die
//
//=============================================================================

class CTFRune : public CTFPowerup
{
public:
	DECLARE_CLASS( CTFRune, CTFPowerup );

	CTFRune();
	~CTFRune();

	virtual void	Spawn( void ) OVERRIDE;
	virtual void	Precache( void ) OVERRIDE;
	virtual bool	MyTouch( CBasePlayer *pPlayer );
	static CTFRune*	CreateRune( const Vector &vecOrigin, RuneTypes_t nType, int nTeam, bool bShouldReposition, bool bApplyForce, Vector vecSpawnDirection = vec3_origin );
	static bool		RepositionRune( RuneTypes_t nType, int nTeamNumber );
	float			GetRuneRepositionTime( void );

	virtual const char *GetDefaultPowerupModel( void )
	{
		if ( m_nRuneType == RUNE_STRENGTH )
		{
			return TF_RUNE_STRENGTH;
		}
		else if ( m_nRuneType == RUNE_RESIST )
		{
			return TF_RUNE_RESIST;
		}
		else if ( m_nRuneType == RUNE_REGEN )
		{
			return TF_RUNE_REGEN;
		}
		else if ( m_nRuneType == RUNE_HASTE )
		{
			return TF_RUNE_HASTE;
		}
		else if ( m_nRuneType == RUNE_VAMPIRE )
		{
			return TF_RUNE_VAMPIRE;
		}
		else if ( m_nRuneType == RUNE_REFLECT )
		{
			return TF_RUNE_REFLECT;
		}
		else if ( m_nRuneType == RUNE_PRECISION )
		{
			return TF_RUNE_PRECISION;
		}
		else if ( m_nRuneType == RUNE_AGILITY )
		{
			return TF_RUNE_AGILITY;
		}
		else if ( m_nRuneType == RUNE_KNOCKOUT )
		{
			return TF_RUNE_KNOCKOUT;
		}
		else if ( m_nRuneType == RUNE_KING )
		{
			return TF_RUNE_KING;
		}
		else if ( m_nRuneType == RUNE_PLAGUE )
		{
			return TF_RUNE_PLAGUE;
		}
		else if ( m_nRuneType == RUNE_SUPERNOVA )
		{
			return TF_RUNE_SUPERNOVA;
		}
		return TF_RUNE_STRENGTH;
	}
	
	virtual int		UpdateTransmitState( void ) OVERRIDE;
	virtual	int		ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;

protected:
	bool m_bApplyForce;
	virtual void ComeToRest( void );
	Vector m_vecSpawnDirection;

private:
	void BlinkThink();

	RuneTypes_t m_nRuneType;
	int m_nBlinkCount;
	float m_flKillTime;
	float m_flCanPickupTime;
	int m_nTeam;
	bool m_bShouldReposition;
};

//=============================================================================
//
// CTF Rune Temp class - Powerups whose effect lasts a length of time, then deactivates
//
//=============================================================================

class CTFRuneTemp : public CTFPowerup
{
public:
	DECLARE_CLASS( CTFRuneTemp, CTFPowerup );

	CTFRuneTemp();

	virtual void		Spawn( void ) OVERRIDE;
	virtual void		Precache( void ) OVERRIDE;
	virtual bool		MyTouch( CBasePlayer *pPlayer ) OVERRIDE;
	virtual const char 	*GetDefaultPowerupModel( void ) OVERRIDE { return TF_RUNE_TEMP_CRIT; }
	virtual float		GetRespawnDelay( void ) OVERRIDE;
		
protected:
	void	TempRuneRespawnThink( void );

	int		m_nRuneTempType;
};

//=============================================================================
//
// Temporary Crit boost
//
//=============================================================================

class CTFRuneTempCrit : public CTFRuneTemp
{
public:
	DECLARE_CLASS( CTFRuneTempCrit, CTFRuneTemp );

	CTFRuneTempCrit();
};

//=============================================================================
//
// Temporary Uber boost
//
//=============================================================================

class CTFRuneTempUber : public CTFRuneTemp
{
public:
	DECLARE_CLASS( CTFRuneTempUber, CTFRuneTemp );

	CTFRuneTempUber();

	virtual const char 	*GetDefaultPowerupModel( void ) OVERRIDE { return TF_RUNE_TEMP_UBER; }

};

//=============================================================================
//
// Powerup Spawn point class - location to spawn a powerup at
//
//=============================================================================

class CTFInfoPowerupSpawn : public CPointEntity, public IInfoPowerupSpawnAutoList
{
public:
	DECLARE_CLASS( CTFInfoPowerupSpawn, CPointEntity );
	CTFInfoPowerupSpawn();
	DECLARE_DATADESC();
	
	virtual void Spawn() OVERRIDE;

	bool IsDisabled() const { return m_bDisabled; }
	bool HasRune() const { return m_hRune != NULL; }
	void SetRune( CTFRune *pRune ) { m_hRune = pRune; }

private:
	bool m_bDisabled;
	int m_nTeam;
	CHandle< CTFRune > m_hRune;
};
#endif // ENTITY_RUNE_H



