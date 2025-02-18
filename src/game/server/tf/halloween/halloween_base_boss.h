//========= Copyright Valve Corporation, All rights reserved. ============//
// halloween_boss_base.h
// Shared code for the Halloween Bosses
// Michael Booth, October 2011

#ifndef HALLOWEEN_BOSS_BASE_H
#define HALLOWEEN_BOSS_BASE_H

#include "tf_shareddefs.h"
#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "headless_hatman_body.h"
#include "Path/NextBotPathFollow.h"


class CTFPlayer;


//----------------------------------------------------------------------------
class CHalloweenBaseBoss : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CHalloweenBaseBoss, NextBotCombatCharacter );

	CHalloweenBaseBoss();
	virtual ~CHalloweenBaseBoss();

	virtual void Spawn( void );
	virtual int OnTakeDamage( const CTakeDamageInfo &rawInfo ) OVERRIDE;
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info ) OVERRIDE;
	virtual void UpdateOnRemove();

	virtual void Update( void );

	void Break( void );		// bust into gibs

	struct AttackerInfo
	{
		CHandle< CTFPlayer > m_attacker;
		float m_timestamp;
		bool m_wasLastHitFromMeleeWeapon;
	};
	const CUtlVector< AttackerInfo > &GetAttackerVector( void ) const;
	void RememberAttacker( CTFPlayer *player, bool wasMeleeHit, float damage );

	bool WasSpawnedByCheats( void ) const;

	virtual float GetCritInjuryMultiplier( void ) const;	// when we are hit by a crit, damage is mutiplied by this

	float GetInjuryRate( void ) const;					// return average recent damage taken per second
	float GetMaxInjuryRate( void ) const;				// return maximum damage taken per second

	virtual int GetLevel() const { return 0; }

	virtual HalloweenBossType GetBossType() const { return HALLOWEEN_BOSS_INVALID; }
	static CHalloweenBaseBoss* SpawnBossAtPos( HalloweenBossType bossType, const Vector& vSpawnPos, int nTeam = TF_TEAM_HALLOWEEN, CBaseEntity* pOwner = NULL );
	
	bool IsSpell() const { return GetTeamNumber() != TF_TEAM_HALLOWEEN; }

	enum HalloweenStatsEventType
	{
		HALLOWEEN_EVENT_BOSS_SPAWN = 0,
	};

private:
	CUtlVector< AttackerInfo > m_attackerVector;		// list of everyone who injured me, and when

	void UpdateDamagePerSecond( void );
	struct DamageRateInfo
	{
		float m_timestamp;
		float m_damage;
	};
	CUtlVector< DamageRateInfo > m_damageVector;

	float m_damagePerSecond;
	float m_maxDamagePerSecond;

	bool m_wasSpawnedByCheats;
};

inline float CHalloweenBaseBoss::GetInjuryRate( void ) const
{
	return m_damagePerSecond;
}

inline float CHalloweenBaseBoss::GetMaxInjuryRate( void ) const
{
	return m_maxDamagePerSecond;
}

inline float CHalloweenBaseBoss::GetCritInjuryMultiplier( void ) const
{
	return TF_DAMAGE_CRIT_MULTIPLIER;
}

inline bool CHalloweenBaseBoss::WasSpawnedByCheats( void ) const
{
	return m_wasSpawnedByCheats;
}

inline const CUtlVector< CHalloweenBaseBoss::AttackerInfo > &CHalloweenBaseBoss::GetAttackerVector( void ) const
{
	return m_attackerVector;
}

#endif // HALLOWEEN_BOSS_BASE_H
