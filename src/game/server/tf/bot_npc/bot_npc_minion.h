//========= Copyright Valve Corporation, All rights reserved. ============//
// bot_npc_minion.h
// Minions for the Boss
// Michael Booth, November 2010

#ifndef BOT_NPC_MINION_H
#define BOT_NPC_MINION_H

#ifdef TF_RAID_MODE

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "Path/NextBotPathFollow.h"
#include "bot_npc.h"
#include "bot_npc_body.h"


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
class CNextBotFlyingLocomotion : public ILocomotion
{
public:
	CNextBotFlyingLocomotion( INextBot *bot );
	virtual ~CNextBotFlyingLocomotion();

	virtual void Reset( void );								// (EXTEND) reset to initial state
	virtual void Update( void );							// (EXTEND) update internal state

	virtual void Approach( const Vector &goalPos, float goalWeight = 1.0f );	// (EXTEND) move directly towards the given position

	virtual void SetDesiredSpeed( float speed );			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual void SetDesiredAltitude( float height );		// how high above our Approach goal do we float?
	virtual float GetDesiredAltitude( void ) const;

	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity
	void SetVelocity( const Vector &velocity );

	void Deflect( CBaseEntity *deflector );

protected:
	float m_desiredSpeed;
	float m_currentSpeed;
	Vector m_forward;

	float m_desiredAltitude;
	void MaintainAltitude( void );

	Vector m_velocity;
	Vector m_acceleration;
};

inline const Vector &CNextBotFlyingLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );

	return up;
}

inline const Vector &CNextBotFlyingLocomotion::GetVelocity( void ) const
{
	return m_velocity;
}

inline void CNextBotFlyingLocomotion::SetVelocity( const Vector &velocity )
{
	m_velocity = velocity;
}


//----------------------------------------------------------------------------
class CBotNPCMinion : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CBotNPCMinion, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();

	CBotNPCMinion();
	virtual ~CBotNPCMinion();

	virtual void Precache();
	virtual void Spawn( void );

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	virtual bool IsDeflectable() { return true; }			// for flamethrower
	virtual void Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

	// INextBot
	DECLARE_INTENTION_INTERFACE( CBotNPCMinion );
	virtual CNextBotFlyingLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CBotNPCBody *GetBodyInterface( void ) const							{ return m_body; }
	virtual IVision *GetVisionInterface( void ) const							{ return m_vision; }

	virtual Vector EyePosition( void );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void BecomeAmmoPack( void );

	CTFPlayer *FindTarget( void );	// Find the closest living player not already being targeted by another minion

	void UpdateTarget( void );

	void SetTarget( CTFPlayer *target );
	CTFPlayer *GetTarget( void ) const;
	bool HasTarget( void ) const;
	bool IsTarget( CTFPlayer *target ) const;

	const Vector &GetLastKnownTargetPosition( void ) const;

	void StartStunEffects( CTFPlayer *victim );
	void EndStunEffects( void );

	bool IsAlert( void ) const;
	void BecomeAlert( void );

private:
	CNextBotFlyingLocomotion *m_locomotor;
	CBotNPCBody *m_body;
	CDisableVision *m_vision;

	Vector m_eyeOffset;

	CTFPlayer *m_target;
	Vector m_lastKnownTargetPosition;

	CountdownTimer m_invulnTimer;

	CNetworkHandle( CBaseEntity, m_stunTarget );

	bool m_isAlert;
};

inline bool CBotNPCMinion::IsAlert( void ) const
{
	return m_isAlert;
}

inline Vector CBotNPCMinion::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}

inline bool CBotNPCMinion::HasTarget( void ) const
{
	return m_target == NULL ? false : true;
}

inline bool CBotNPCMinion::IsTarget( CTFPlayer *target ) const
{
	return ( m_target == target ) ? true : false;
}

inline void CBotNPCMinion::SetTarget( CTFPlayer *target )
{
	m_target = target;
}

inline CTFPlayer *CBotNPCMinion::GetTarget( void ) const
{
	return m_target;
}

inline const Vector &CBotNPCMinion::GetLastKnownTargetPosition( void ) const
{
	return m_lastKnownTargetPosition;
}

inline void CBotNPCMinion::StartStunEffects( CTFPlayer *victim )
{
	m_stunTarget = victim;
}

inline void CBotNPCMinion::EndStunEffects( void )
{
	m_stunTarget = NULL;
}


#endif // TF_RAID_MODE

#endif // BOT_NPC_MINION_H
