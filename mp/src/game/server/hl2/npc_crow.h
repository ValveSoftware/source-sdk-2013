//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NPC_CROW_H
#define NPC_CROW_H
#ifdef _WIN32
#pragma once
#endif

#define BIRDTYPE_CROW 1
#define BIRDTYPE_PIGEON 2
#define BIRDTYPE_SEAGULL 3

//
// Spawnflags.
//
#define SF_CROW_FLYING		16

#define CROW_TAKEOFF_SPEED		170
#define CROW_AIRSPEED			220 // FIXME: should be about 440, but I need to add acceleration

//
// Custom schedules.
//
enum
{
	SCHED_CROW_IDLE_WALK = LAST_SHARED_SCHEDULE,
	SCHED_CROW_IDLE_FLY,

	//
	// Various levels of wanting to get away from something, selected
	// by current value of m_nMorale.
	//
	SCHED_CROW_WALK_AWAY,
	SCHED_CROW_RUN_AWAY,
	SCHED_CROW_HOP_AWAY,
	SCHED_CROW_FLY_AWAY,

	SCHED_CROW_FLY,
	SCHED_CROW_FLY_FAIL,

	SCHED_CROW_BARNACLED,
};


//
// Custom tasks.
//
enum 
{
	TASK_CROW_FIND_FLYTO_NODE = LAST_SHARED_TASK,
	//TASK_CROW_PREPARE_TO_FLY,
	TASK_CROW_TAKEOFF,
	//TASK_CROW_LAND,
	TASK_CROW_FLY,
	TASK_CROW_FLY_TO_HINT,
	TASK_CROW_PICK_RANDOM_GOAL,
	TASK_CROW_PICK_EVADE_GOAL,
	TASK_CROW_HOP,

	TASK_CROW_FALL_TO_GROUND,
	TASK_CROW_PREPARE_TO_FLY_RANDOM,

	TASK_CROW_WAIT_FOR_BARNACLE_KILL,
};


//
// Custom conditions.
//
enum
{
	COND_CROW_ENEMY_TOO_CLOSE = LAST_SHARED_CONDITION,
	COND_CROW_ENEMY_WAY_TOO_CLOSE,
	COND_CROW_FORCED_FLY,
	COND_CROW_BARNACLED,
};

enum FlyState_t
{
	FlyState_Walking = 0,
	FlyState_Flying,
	FlyState_Falling,
	FlyState_Landing,
};


//-----------------------------------------------------------------------------
// The crow class.
//-----------------------------------------------------------------------------
class CNPC_Crow : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Crow, CAI_BaseNPC );

public:

	//
	// CBaseEntity:
	//
	virtual void Spawn( void );
	virtual void Precache( void );

	virtual Vector BodyTarget( const Vector &posSrc, bool bNoisy = true );

	virtual int DrawDebugTextOverlays( void );

	//
	// CBaseCombatCharacter:
	//
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual bool CorpseGib( const CTakeDamageInfo &info );
	bool	BecomeRagdollOnClient( const Vector &force );

	//
	// CAI_BaseNPC:
	//
	virtual float MaxYawSpeed( void ) { return 120.0f; }
	
	virtual Class_T Classify( void );
	virtual void GatherEnemyConditions( CBaseEntity *pEnemy );

	virtual void HandleAnimEvent( animevent_t *pEvent );
	virtual int GetSoundInterests( void );
	virtual int SelectSchedule( void );
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );

	virtual bool HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sourceEnt );

	virtual void OnChangeActivity( Activity eNewActivity );

	virtual bool OverrideMove( float flInterval );

	virtual bool FValidateHintType( CAI_Hint *pHint );
	virtual Activity GetHintActivity( short sHintType, Activity HintsActivity );

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual void IdleSound( void );
	virtual void AlertSound( void );
	virtual void StopLoopingSounds( void );
	virtual void UpdateEfficiency( bool bInPVS );

	virtual bool QueryHearSound( CSound *pSound );

	void InputFlyAway( inputdata_t &inputdata );
	
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	void StartTargetHandling( CBaseEntity *pTargetEnt );

	DEFINE_CUSTOM_AI;
	DECLARE_DATADESC();

	int			m_iBirdType;
	bool		m_bOnJeep;

protected:
	void SetFlyingState( FlyState_t eState );
	inline bool IsFlying( void ) const { return GetNavType() == NAV_FLY; }

	void Takeoff( const Vector &vGoal );
	void FlapSound( void );

	void MoveCrowFly( float flInterval );
	bool Probe( const Vector &vecMoveDir, float flSpeed, Vector &vecDeflect );

	bool IsDeaf() { return m_bIsDeaf; }

protected:
	float m_flGroundIdleMoveTime;

	float m_flEnemyDist;		// Distance to GetEnemy(), cached in GatherEnemyConditions.
	int m_nMorale;				// Used to determine which avoidance schedule to pick. Degrades as I pick avoidance schedules.
	
	bool m_bReachedMoveGoal;

	float m_flHopStartZ;		// Our Z coordinate when we started a hop. Used to check for accidentally hopping off things.

	bool		m_bPlayedLoopingSound;

private:

	Activity NPC_TranslateActivity( Activity eNewActivity );

	float				m_flSoarTime;
	bool				m_bSoar;
	Vector				m_vLastStoredOrigin;
	float				m_flLastStuckCheck;
	
	float				m_flDangerSoundTime;

	Vector				m_vDesiredTarget;
	Vector				m_vCurrentTarget;

	bool				m_bIsDeaf;
};

//-----------------------------------------------------------------------------
// Purpose: Seagull. Crow with a different model.
//-----------------------------------------------------------------------------
class CNPC_Seagull : public CNPC_Crow
{
	DECLARE_CLASS( CNPC_Seagull, CNPC_Crow );

public:
	
	void Spawn( void )
	{
		SetModelName( AllocPooledString("models/seagull.mdl") );
		BaseClass::Spawn();

		m_iBirdType = BIRDTYPE_SEAGULL;
	}

	void PainSound( const CTakeDamageInfo &info )
	{
		EmitSound( "NPC_Seagull.Pain" );
	}

	void DeathSound( const CTakeDamageInfo &info )
	{
		EmitSound( "NPC_Seagull.Pain" );
	}

	void IdleSound( void )
	{
		EmitSound( "NPC_Seagull.Idle" );
	}
};

//-----------------------------------------------------------------------------
// Purpose: Pigeon. Crow with a different model.
//-----------------------------------------------------------------------------
class CNPC_Pigeon : public CNPC_Crow
{
	DECLARE_CLASS( CNPC_Pigeon, CNPC_Crow );

public:
	void Spawn( void )
	{
		SetModelName( AllocPooledString("models/pigeon.mdl") );
		BaseClass::Spawn();

		m_iBirdType = BIRDTYPE_PIGEON;
	}

	void IdleSound( void )
	{
		EmitSound( "NPC_Pigeon.Idle" );
	}
};

#endif // NPC_CROW_H
