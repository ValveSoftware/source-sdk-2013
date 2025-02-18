//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MERASMUS_H
#define MERASMUS_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "merasmus_body.h"
#include "Path/NextBotPathFollow.h"
#include "../halloween_base_boss.h"


extern ConVar tf_merasmus_health_base;
extern ConVar tf_merasmus_health_per_player;
extern ConVar tf_merasmus_min_player_count;

extern ConVar tf_merasmus_speed;
extern ConVar tf_merasmus_attack_range;
extern ConVar tf_merasmus_speed_recovery_rate;
extern ConVar tf_merasmus_speed_penalty;
extern ConVar tf_merasmus_chase_duration;
extern ConVar tf_merasmus_chase_range;

extern ConVar tf_merasmus_health_regen_rate;

extern ConVar tf_merasmus_bomb_head_duration;
extern ConVar tf_merasmus_bomb_head_per_team;

class CTFPlayer;
class CWheelOfDoom;
class CMerasmus;
class CTFWeaponBaseGrenadeProj;
class CTFMerasmusTrickOrTreatProp;
class CMonsterResource;

//----------------------------------------------------------------------------
class CMerasmusSWStats
{
public:

	void ResetStats () 
	{
		V_memset( m_arrClassDamage, 0, sizeof( m_arrClassDamage ) );
		m_flPropHuntTime1 = 0;
		m_flPropHuntTime2 = 0;
		m_flLifeTime = 0;
		m_nBombKills = 0;
		m_nStaffKills = 0;
		m_nPvpKills = 0;
	}

	int m_arrClassDamage[ TF_LAST_NORMAL_CLASS ];
	float m_flPropHuntTime1;
	float m_flPropHuntTime2;
	float m_flLifeTime;
	int m_nBombKills;
	int m_nStaffKills;
	int m_nPvpKills;
};

//----------------------------------------------------------------------------
class CMerasmusLocomotion : public NextBotGroundLocomotion
{
public:
	CMerasmusLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { }
	virtual ~CMerasmusLocomotion() { }

	virtual void Update( void );							// (EXTEND) update internal state

	virtual float GetRunSpeed( void ) const;			// get maximum running speed
	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump

	/**
	 * Should we collide with this entity?
	 */
	virtual bool ShouldCollideWith( const CBaseEntity *object ) const;

private:
	virtual float GetMaxYawRate( void ) const;				// return max rate of yaw rotation
};


//----------------------------------------------------------------------------
class CMerasmusFlyingLocomotion : public ILocomotion
{
public:
	CMerasmusFlyingLocomotion( INextBot *bot );
	virtual ~CMerasmusFlyingLocomotion();

	virtual void Reset( void );								// (EXTEND) reset to initial state
	virtual void Update( void );							// (EXTEND) update internal state

	virtual void Approach( const Vector &goalPos, float goalWeight = 1.0f );	// (EXTEND) move directly towards the given position

	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual void SetDesiredAltitude( float height );		// how high above our Approach goal do we float?
	virtual float GetDesiredAltitude( void ) const;

	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity
	void SetVelocity( const Vector &velocity );

	virtual bool ShouldCollideWith( const CBaseEntity *object ) const;

	virtual void FaceTowards( const Vector &target );		// rotate body to face towards "target"

protected:
	float m_currentSpeed;
	Vector m_forward;

	float m_desiredAltitude;
	void MaintainAltitude( void );

	Vector m_velocity;
	Vector m_acceleration;
};

inline const Vector &CMerasmusFlyingLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );

	return up;
}

inline const Vector &CMerasmusFlyingLocomotion::GetVelocity( void ) const
{
	return m_velocity;
}

inline void CMerasmusFlyingLocomotion::SetVelocity( const Vector &velocity )
{
	m_velocity = velocity;
}


//----------------------------------------------------------------------------
class CMerasmusIntention : public IIntention
{
public:
	CMerasmusIntention( CMerasmus *me );
	virtual ~CMerasmusIntention();

	virtual void Reset( void );
	virtual void Update( void );

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }

private:
	Behavior< CMerasmus > *m_behavior;
};

DECLARE_AUTO_LIST( IMerasmusAutoList );

//----------------------------------------------------------------------------
class CMerasmus : public CHalloweenBaseBoss, public CGameEventListener, public IMerasmusAutoList
{
public:
	DECLARE_CLASS( CMerasmus, CHalloweenBaseBoss );
	DECLARE_SERVERCLASS();

	CMerasmus();
	virtual ~CMerasmus();

	static void PrecacheMerasmus();
	virtual void Precache();
	virtual void Spawn( void );
	virtual void UpdateOnRemove();

	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	// CGameEventListener
	virtual void FireGameEvent( IGameEvent *event );

	// INextBot
	virtual CMerasmusIntention *GetIntentionInterface( void ) const		{ return m_intention; }
	virtual ILocomotion	*GetLocomotionInterface( void ) const			{ if ( m_isFlying ) return m_flyingLocomotor; return m_locomotor; }
	virtual CMerasmusBody *GetBodyInterface( void ) const				{ return m_body; }

	virtual void Update( void );

	const Vector &GetHomePosition( void ) const;

	Vector GetCastPosition() const;

	bool IsRevealed() const { return m_bRevealed; }
	void OnRevealed(bool bPlaySound = true);
	bool ShouldReveal() const;
	bool IsNextKilledPropMerasmus() const;
	void SetRevealer( CTFPlayer* pPlayer ) { m_hMerasmusRevealer = pPlayer; }

	void OnDisguise();
	bool ShouldDisguise() const;

	void StartAOEAttack() { m_bIsDoingAOEAttack = true; }
	void StopAOEAttack() { m_bIsDoingAOEAttack = false; }
	bool IsDoingAOEAttack() const { return m_bIsDoingAOEAttack; }

	static CTFWeaponBaseGrenadeProj* CreateMerasmusGrenade( const Vector& vPosition, const Vector& vVelocity, CBaseCombatCharacter* pOwner, float fScale = 1.0f );

	static const char* GetRandomPropModelName();

	void PushPlayer( CTFPlayer* pPlayer, float flPushForce ) const;
	int GetBombHitCount() const { return m_nBombHitCount; }
	void ResetBombHitCount() { m_nBombHitCount = 0; }
	void AddStun( CTFPlayer* pPlayer );
	void OnBeginStun();
	void OnEndStun();
	bool HasStunTimer() const { return !m_stunTimer.IsElapsed(); }
	bool IsStunned() const { return m_bStunned; }

	void AddFakeProp( CTFMerasmusTrickOrTreatProp* pFakeProp );
	void RemoveAllFakeProps();

	void BombHeadMode();

	bool ShouldLeave() const;
	void LeaveWarning();
	void OnLeaveWhileInPropForm();

	void TriggerLogicRelay( const char* pszLogicRelayName, bool bSpawn = false );

	void StartFlying( void ) { m_isFlying = true; }
	void StopFlying( void ) { m_isFlying = false; }
	bool IsFlying( void ) const { return m_isFlying; }
	bool IsHiding( void ) const { return m_isHiding; }

	void PlayLowPrioritySound( IRecipientFilter &filter, const char* pszSoundEntryName );
	void PlayHighPrioritySound( const char* pszSoundEntryName );

	void GainLevel( void );
	void ResetLevel( void );
	static int GetMerasmusLevel() { return m_level; }
	virtual int GetLevel( void ) const OVERRIDE;
	static void DBG_SetLevel( int nLevel );

	virtual HalloweenBossType GetBossType() const { return HALLOWEEN_BOSS_MERASMUS; }

	void StartRespawnTimer() const;

	const CUtlVector< CHandle<CTFPlayer> >& GetStartingAttackers() const;

	static bool Zap( CBaseCombatCharacter *pCaster, const char* pszCastingAttachmentName, float flSpellRange, float flMinDamage, float flMaxDamage, int nMaxTarget, int nTargetTeam = TEAM_ANY );

	// Stats
	void RecordDisguiseTime( );
	void SW_ReportMerasmusStats( void );
private:


	CMerasmusIntention *m_intention;
	CMerasmusLocomotion *m_locomotor;
	CMerasmusFlyingLocomotion *m_flyingLocomotor;
	CMerasmusBody *m_body;

	bool m_isFlying;

	bool m_isHiding;

	CUtlVector< AttackerInfo > m_attackerVector;		// list of everyone who injured me, and when
	CUtlVector< CHandle<CTFPlayer> > m_startingAttackersVector;

	CountdownTimer m_stunTimer;
	int m_nBombHitCount;

	Vector m_homePos;
	int m_damagePoseParameter;

	int m_nRevealedHealth;

	CHandle< CWheelOfDoom > m_wheel;

	CHandle< CMonsterResource > m_hHealthBar;

	CNetworkVar( bool, m_bRevealed );
	CNetworkVar( bool, m_bIsDoingAOEAttack );
	CNetworkVar( bool, m_bStunned );

	CSoundPatch *m_pIdleSound;

	CUtlVector< CHandle< CTFMerasmusTrickOrTreatProp > > m_fakePropVector;

	int m_nDestroyedPropsToReveal;

	SolidType_t m_solidType;
	int m_solidFlags;

	CHandle< CTFPlayer > m_hMerasmusRevealer;

	CountdownTimer m_lifeTimer;
	float m_flLastWarnTime;
	
	// For Stats
	float m_flStartDisguiseTime;
	CMerasmusSWStats m_bossStats;

	static int m_level;
};


inline int CMerasmus::GetLevel( void ) const
{
	return m_level;
}

inline void CMerasmus::GainLevel( void )
{
	++m_level;
}

inline void CMerasmus::ResetLevel( void )
{
	m_level = 1;
}

inline void CMerasmus::DBG_SetLevel( int nLevel )
{
	m_level = nLevel;
}

inline const Vector &CMerasmus::GetHomePosition( void ) const
{
	return m_homePos;
}

inline const CUtlVector< CHandle<CTFPlayer> >& CMerasmus::GetStartingAttackers() const
{
	return m_startingAttackersVector;
}

//--------------------------------------------------------------------------------------------------------------
class CMerasmusPathCost : public IPathCost
{
public:
	CMerasmusPathCost( CMerasmus *me )
	{
		m_me = me;
	}

	// return the cost (weighted distance between) of moving from "fromArea" to "area", or -1 if the move is not allowed
	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			{
				// our locomotor says we can't move here
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				// optimization to avoid recomputing length
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			float cost = dist + fromArea->GetCostSoFar();

			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );
			if ( deltaZ >= m_me->GetLocomotionInterface()->GetStepHeight() )
			{
				if ( deltaZ >= m_me->GetLocomotionInterface()->GetMaxJumpHeight() )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 5.0f;
				cost += jumpPenalty * dist;
			}
			else if ( deltaZ < -m_me->GetLocomotionInterface()->GetDeathDropHeight() )
			{
				// too far to drop
				return -1.0f;
			}

			return cost;
		}
	}

	CMerasmus *m_me;
};


#endif // MERASMUS_H
