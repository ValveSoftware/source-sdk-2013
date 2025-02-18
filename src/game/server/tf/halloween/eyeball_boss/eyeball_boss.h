//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_H
#define EYEBALL_BOSS_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc/bot_npc_body.h"
#include "../halloween_base_boss.h"

#define EYEBALL_RADIUS 100.0f

#define EYEBALL_NORMAL_SKIN 0
#define EYEBALL_RED_SKIN 1
#define EYEBALL_TEAM_RED 2
#define EYEBALL_TEAM_BLUE 3

#define PURGATORY_Z -1152

#define EYEBALL_ANGRY	2
#define EYEBALL_GRUMPY	1
#define EYEBALL_CALM	0

extern ConVar tf_eyeball_boss_debug;
extern ConVar tf_eyeball_boss_debug_orientation;
extern ConVar tf_eyeball_boss_lifetime;
extern ConVar tf_eyeball_boss_lifetime_spell;
extern ConVar tf_eyeball_boss_speed;
extern ConVar tf_eyeball_boss_hover_height;
extern ConVar tf_eyeball_boss_acceleration;
extern ConVar tf_eyeball_boss_horiz_damping;
extern ConVar tf_eyeball_boss_vert_damping;
extern ConVar tf_eyeball_boss_attack_range;
extern ConVar tf_eyeball_boss_health_base;
extern ConVar tf_eyeball_boss_health_per_player;
extern ConVar tf_halloween_bot_min_player_count;


//----------------------------------------------------------------------------
class CEyeballBossBody : public CBotNPCBody
{
public:
	CEyeballBossBody( INextBot *bot );
	virtual ~CEyeballBossBody() { }

	virtual void Update( void );

	virtual void AimHeadTowards( const Vector &lookAtPos, 
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// aim the bot's head towards the given goal
	virtual void AimHeadTowards( CBaseEntity *subject,
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// continually aim the bot's head towards the given subject

	virtual float GetMaxHeadAngularVelocity( void ) const			// return max turn rate of head in degrees/second
	{
		return 3000.0f;
	}

private:
	int m_leftRightPoseParameter;
	int m_upDownPoseParameter;

	Vector m_lookAtSpot;
};


//----------------------------------------------------------------------------
// Bypass vision system
class CDisableVision : public IVision
{
public:
	CDisableVision( INextBot *bot ) : IVision( bot ) { }
	virtual ~CDisableVision() { }

	virtual void Reset( void )	{ }
	virtual void Update( void ) { }
};


//----------------------------------------------------------------------------
class CEyeballBossLocomotion : public ILocomotion
{
public:
	CEyeballBossLocomotion( INextBot *bot );
	virtual ~CEyeballBossLocomotion();

	virtual void Reset( void );								// (EXTEND) reset to initial state
	virtual void Update( void );							// (EXTEND) update internal state

	virtual void Approach( const Vector &goalPos, float goalWeight = 1.0f );	// (EXTEND) move directly towards the given position

	virtual void SetDesiredSpeed( float speed );			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual void SetDesiredAltitude( float height );		// how high above our Approach goal do we float?
	virtual float GetDesiredAltitude( void ) const;

	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity
	void SetVelocity( const Vector &velocity );

	virtual void FaceTowards( const Vector &target );		// rotate body to face towards "target"

	// return position of "feet" - the driving point where the bot contacts the ground
	// for this floating boss, "feet" refers to the ground directly underneath him
	virtual const Vector &GetFeet( void ) const;			

protected:
	float m_desiredSpeed;
	float m_currentSpeed;
	Vector m_forward;

	float m_desiredAltitude;
	void MaintainAltitude( void );

	Vector m_velocity;
	Vector m_acceleration;
};


inline float CEyeballBossLocomotion::GetStepHeight( void ) const
{
	return 50.0f;
}

inline float CEyeballBossLocomotion::GetMaxJumpHeight( void ) const
{
	return 100.0f;
}

inline float CEyeballBossLocomotion::GetDeathDropHeight( void ) const
{
	return 999.9f;
}

inline const Vector &CEyeballBossLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );

	return up;
}

inline const Vector &CEyeballBossLocomotion::GetVelocity( void ) const
{
	return m_velocity;
}

inline void CEyeballBossLocomotion::SetVelocity( const Vector &velocity )
{
	m_velocity = velocity;
}

DECLARE_AUTO_LIST( IEyeballBossAutoList );

//----------------------------------------------------------------------------
class CEyeballBoss : public CHalloweenBaseBoss, public IEyeballBossAutoList
{
public:
	DECLARE_CLASS( CEyeballBoss, CHalloweenBaseBoss );
	DECLARE_SERVERCLASS();

	CEyeballBoss();
	virtual ~CEyeballBoss();

	static void PrecacheEyeballBoss();
	virtual void Precache();
	virtual void Spawn( void );
	virtual void UpdateOnRemove( void );

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual void UpdateLastKnownArea( void );										// invoke this to update our last known nav area (since there is no think method chained to CBaseCombatCharacter)

	// INextBot
	DECLARE_INTENTION_INTERFACE( CEyeballBoss );
	virtual CEyeballBossLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CEyeballBossBody *GetBodyInterface( void ) const				{ return m_body; }
	virtual CDisableVision *GetVisionInterface( void ) const				{ return m_vision; }

	virtual void Update( void );

	virtual Vector EyePosition( void );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual float GetCritInjuryMultiplier( void ) const;		// when we are hit by a crit, damage is mutiplied by this

	const Vector &GetHomePosition( void ) const;

	void BecomeEnraged( float duration );
	bool IsEnraged( void ) const;

	bool IsGrumpy( void ) const;

	void SetLookAtTarget( const Vector &spot );
	
	void SetVictim( CBaseCombatCharacter *victim );
	CBaseCombatCharacter *GetVictim( void ) const;

	bool IsInPurgatory( CBaseEntity *entity ) const;

	CBaseCombatCharacter *FindClosestVisibleVictim( void );

	const Vector &PickNewSpawnSpot( void ) const;

	void JarateNearbyPlayers( float range );

	void SetDamageLimit( int limit );
	void RemoveDamageLimit( void );

	void LogPlayerInteraction( const char *verb, CTFPlayer *player );

	void GainLevel( void );
	void ResetLevel( void );
	virtual int GetLevel( void ) const OVERRIDE;

	virtual HalloweenBossType GetBossType() const { return HALLOWEEN_BOSS_MONOCULUS; }

private:
	CEyeballBossLocomotion *m_locomotor;
	CEyeballBossBody *m_body;
	CDisableVision *m_vision;

	Vector m_eyeOffset;
	Vector m_homePos;

	CTFPlayer *m_target;

	CountdownTimer m_invulnTimer;

	CNetworkVector( m_lookAtSpot );
	CNetworkVar( int, m_attitude );

	CountdownTimer m_rageTimer;

	CHandle< CBaseCombatCharacter > m_victim;

	CUtlVector< CHandle< CBaseEntity > > m_spawnSpotVector;

	int m_damageLimit;

	static int m_level;
};

inline int CEyeballBoss::GetLevel( void ) const
{
	return m_level;
}

inline void CEyeballBoss::GainLevel( void )
{
	++m_level;
}

inline void CEyeballBoss::ResetLevel( void )
{
	m_level = 1;
}

inline void CEyeballBoss::SetDamageLimit( int limit )
{
	m_damageLimit = limit;
}

inline void CEyeballBoss::RemoveDamageLimit( void )
{
	m_damageLimit = -1;
}

inline float CEyeballBoss::GetCritInjuryMultiplier( void ) const
{
	return 2.0f;
}

inline bool CEyeballBoss::IsInPurgatory( CBaseEntity *entity ) const
{
	if ( IsSpell() )
		return false;

	return ( entity->GetAbsOrigin().z < PURGATORY_Z );
}

inline void CEyeballBoss::SetVictim( CBaseCombatCharacter *victim )
{
	m_victim = victim;
}

inline bool CEyeballBoss::IsEnraged( void ) const
{
	// always enrage if I'm a spell
	if ( IsSpell() )
		return true;

	// being near death always makes me mad
	if ( GetHealth() < GetMaxHealth()/3 )
		return true;

	return m_rageTimer.HasStarted() && !m_rageTimer.IsElapsed();
}

inline bool CEyeballBoss::IsGrumpy( void ) const
{
	if ( IsEnraged() )
		return false;

	return ( GetHealth() < 2*GetMaxHealth()/3 );
}

inline const Vector &CEyeballBoss::GetHomePosition( void ) const
{
	return m_homePos;
}

inline Vector CEyeballBoss::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}

inline void CEyeballBoss::SetLookAtTarget( const Vector &spot )
{
	m_lookAtSpot = spot;
}

#endif // EYEBALL_BOSS_H
