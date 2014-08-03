//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_BASEZOMBIE_H
#define NPC_BASEZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#include "ai_basenpc.h"
#include "ai_blended_movement.h"
#include "soundenvelope.h"
#include "ai_behavior_actbusy.h"

#define ZOM_ATTN_FOOTSTEP ATTN_IDLE

#define	ENVELOPE_CONTROLLER		(CSoundEnvelopeController::GetController())

#define ZOMBIE_MELEE_REACH	55

extern int AE_ZOMBIE_ATTACK_RIGHT;
extern int AE_ZOMBIE_ATTACK_LEFT;
extern int AE_ZOMBIE_ATTACK_BOTH;
extern int AE_ZOMBIE_SWATITEM;
extern int AE_ZOMBIE_STARTSWAT;
extern int AE_ZOMBIE_STEP_LEFT;
extern int AE_ZOMBIE_STEP_RIGHT;
extern int AE_ZOMBIE_SCUFF_LEFT;
extern int AE_ZOMBIE_SCUFF_RIGHT;
extern int AE_ZOMBIE_ATTACK_SCREAM;
extern int AE_ZOMBIE_GET_UP;
extern int AE_ZOMBIE_POUND;

#define ZOMBIE_BODYGROUP_HEADCRAB	1	// The crab on our head

// Pass these to claw attack so we know where to draw the blood.
#define ZOMBIE_BLOOD_LEFT_HAND		0
#define ZOMBIE_BLOOD_RIGHT_HAND		1
#define ZOMBIE_BLOOD_BOTH_HANDS		2
#define ZOMBIE_BLOOD_BITE			3
	

enum HeadcrabRelease_t
{
	RELEASE_NO,
	RELEASE_IMMEDIATE,		// release the headcrab right now!
	RELEASE_SCHEDULED,		// release the headcrab through the AI schedule.
	RELEASE_VAPORIZE,		// just destroy the crab.	
	RELEASE_RAGDOLL,		// release a dead crab
	RELEASE_RAGDOLL_SLICED_OFF	// toss the crab up a bit
};


//=========================================================
// schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_CHASE_ENEMY = LAST_SHARED_SCHEDULE,
	SCHED_ZOMBIE_MOVE_SWATITEM,
	SCHED_ZOMBIE_SWATITEM,
	SCHED_ZOMBIE_ATTACKITEM,
	SCHED_ZOMBIE_RELEASECRAB,
	SCHED_ZOMBIE_MOVE_TO_AMBUSH,
	SCHED_ZOMBIE_WAIT_AMBUSH,
	SCHED_ZOMBIE_WANDER_MEDIUM,	// medium range wandering behavior.
	SCHED_ZOMBIE_WANDER_FAIL,
	SCHED_ZOMBIE_WANDER_STANDOFF,
	SCHED_ZOMBIE_MELEE_ATTACK1,
	SCHED_ZOMBIE_POST_MELEE_WAIT,

	LAST_BASE_ZOMBIE_SCHEDULE,
};

//=========================================================
// tasks
//=========================================================
enum 
{
	TASK_ZOMBIE_DELAY_SWAT = LAST_SHARED_TASK,
	TASK_ZOMBIE_GET_PATH_TO_PHYSOBJ,
	TASK_ZOMBIE_SWAT_ITEM,
	TASK_ZOMBIE_DIE,
	TASK_ZOMBIE_RELEASE_HEADCRAB,
	TASK_ZOMBIE_WAIT_POST_MELEE,

	LAST_BASE_ZOMBIE_TASK,
};


//=========================================================
// Zombie conditions
//=========================================================
enum Zombie_Conds
{
	COND_ZOMBIE_CAN_SWAT_ATTACK = LAST_SHARED_CONDITION,
	COND_ZOMBIE_RELEASECRAB,
	COND_ZOMBIE_LOCAL_MELEE_OBSTRUCTION,

	LAST_BASE_ZOMBIE_CONDITION,
};



typedef CAI_BlendingHost< CAI_BehaviorHost<CAI_BaseNPC> > CAI_BaseZombieBase;

//=========================================================
//=========================================================
abstract_class CNPC_BaseZombie : public CAI_BaseZombieBase
{
	DECLARE_CLASS( CNPC_BaseZombie, CAI_BaseZombieBase );

public:
	CNPC_BaseZombie( void );
	~CNPC_BaseZombie( void );

	void Spawn( void );
	void Precache( void );
	void StartTouch( CBaseEntity *pOther );
	bool CreateBehaviors();
	float MaxYawSpeed( void );
	bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	Class_T Classify( void );
	Disposition_t IRelationType( CBaseEntity *pTarget );
	void HandleAnimEvent( animevent_t *pEvent );

	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	void KillMe( void )
	{
		m_iHealth = 5;
		OnTakeDamage( CTakeDamageInfo( this, this, m_iHealth * 2, DMG_GENERIC ) );
	}

	int MeleeAttack1Conditions ( float flDot, float flDist );
	virtual float GetClawAttackRange() const { return ZOMBIE_MELEE_REACH; }

	// No range attacks
	int RangeAttack1Conditions ( float flDot, float flDist ) { return( 0 ); }
	
	virtual float GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual float	GetReactionDelay( CBaseEntity *pEnemy ) { return 0.0; }

	virtual int SelectSchedule ( void );
	virtual int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual void BuildScheduleTestBits( void );

	virtual int TranslateSchedule( int scheduleType );
	virtual Activity NPC_TranslateActivity( Activity baseAct );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void GatherConditions( void );
	void PrescheduleThink( void );

	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	void StopLoopingSounds();
	virtual void OnScheduleChange( void );

	virtual void PoundSound();

	// Custom damage/death 
	bool ShouldIgnite( const CTakeDamageInfo &info );
	bool ShouldIgniteZombieGib( void );
	virtual bool IsChopped( const CTakeDamageInfo &info );
	virtual bool IsSquashed( const CTakeDamageInfo &info ) { return false; }
	virtual void DieChopped( const CTakeDamageInfo &info );
	virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void CopyRenderColorTo( CBaseEntity *pOther );

	virtual bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold );
	virtual HeadcrabRelease_t ShouldReleaseHeadcrab( const CTakeDamageInfo &info, float flDamageThreshold );

	// Headcrab releasing/breaking apart
	void RemoveHead( void );
	virtual void SetZombieModel( void ) { };
	virtual void BecomeTorso( const Vector &vecTorsoForce, const Vector &vecLegsForce );
	virtual bool CanBecomeLiveTorso() { return false; }
	virtual bool HeadcrabFits( CBaseAnimating *pCrab );
	void ReleaseHeadcrab( const Vector &vecOrigin, const Vector &vecVelocity, bool fRemoveHead, bool fRagdollBody, bool fRagdollCrab = false );
	void SetHeadcrabSpawnLocation( int iCrabAttachment, CBaseAnimating *pCrab );

	// Slumping/sleeping
	bool IsSlumped( void );
	bool IsGettingUp( void );

	// Swatting physics objects
	int GetSwatActivity( void );
	bool FindNearestPhysicsObject( int iMaxMass );
	float DistToPhysicsEnt( void );
	virtual bool CanSwatPhysicsObjects( void ) { return true; }

	// Returns whether we must be very near our enemy to attack them.
	virtual bool MustCloseToAttack(void) { return true; }

	virtual CBaseEntity *ClawAttack( float flDist, int iDamage, QAngle &qaViewPunch, Vector &vecVelocityPunch, int BloodOrigin );

	// Sounds & sound envelope
	virtual bool ShouldPlayFootstepMoan( void );
	virtual void PainSound( const CTakeDamageInfo &info ) = 0;
	virtual void AlertSound( void ) = 0;
	virtual void IdleSound( void ) = 0;
	virtual void AttackSound( void ) = 0;
	virtual void AttackHitSound( void ) = 0;
	virtual void AttackMissSound( void ) = 0;
	virtual void FootstepSound( bool fRightFoot ) = 0;
	virtual void FootscuffSound( bool fRightFoot ) = 0;

	// make a sound Alyx can hear when in darkness mode
	void		 MakeAISpookySound( float volume, float duration = 0.5 );

	virtual bool CanPlayMoanSound();
	virtual void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );
	bool ShouldPlayIdleSound( void ) { return false; }

	virtual const char *GetMoanSound( int nSound ) = 0;
	virtual const char *GetHeadcrabClassname( void ) = 0;
	virtual const char *GetLegsModel( void ) = 0;
	virtual const char *GetTorsoModel( void ) = 0;
	virtual const char *GetHeadcrabModel( void ) = 0;

	virtual Vector BodyTarget( const Vector &posSrc, bool bNoisy );
	virtual Vector HeadTarget( const Vector &posSrc );
	virtual float  GetAutoAimRadius();
	virtual void TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition );

	bool OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	virtual	bool		AllowedToIgnite( void ) { return true; }

public:
	CAI_ActBusyBehavior		m_ActBusyBehavior;



protected:

	CSoundPatch	*m_pMoanSound;

	bool	m_fIsTorso;			// is this is a half-zombie?
	bool	m_fIsHeadless;		// is this zombie headless

	float	m_flNextFlinch;

	bool m_bHeadShot;			// Used to determine the survival of our crab beyond our death.

	//
	// Zombies catch on fire if they take too much burn damage in a given time period.
	//
	float	m_flBurnDamage;				// Keeps track of how much burn damage we've incurred in the last few seconds.
	float	m_flBurnDamageResetTime;	// Time at which we reset the burn damage.

	EHANDLE m_hPhysicsEnt;

	float m_flNextMoanSound;
	float m_flNextSwat;
	float m_flNextSwatScan;
	float m_crabHealth;
	float m_flMoanPitch;

	EHANDLE	m_hObstructor;

	static int g_numZombies;	// counts total number of existing zombies.

	int m_iMoanSound; // each zombie picks one of the 4 and keeps it.

	static int ACT_ZOM_SWATLEFTMID;
	static int ACT_ZOM_SWATRIGHTMID;
	static int ACT_ZOM_SWATLEFTLOW;
	static int ACT_ZOM_SWATRIGHTLOW;
	static int ACT_ZOM_RELEASECRAB;
	static int ACT_ZOM_FALL;

	DECLARE_DATADESC();

	DEFINE_CUSTOM_AI;

private:
	bool m_bIsSlumped;

};

#endif // NPC_BASEZOMBIE_H
