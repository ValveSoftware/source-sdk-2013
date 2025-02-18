//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A slow-moving, once-human headcrab victim with only melee attacks.
//
//=============================================================================//

#include "cbase.h"

#include "doors.h"

#include "simtimer.h"
#include "npc_BaseZombie.h"
#include "ai_hull.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "gib.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ACT_FLINCH_PHYSICS


ConVar	sk_zombie_health( "sk_zombie_health","0");

envelopePoint_t envZombieMoanVolumeFast[] =
{
	{	7.0f, 7.0f,
		0.1f, 0.1f,
	},
	{	0.0f, 0.0f,
		0.2f, 0.3f,
	},
};

envelopePoint_t envZombieMoanVolume[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.1f,
	},
	{	1.0f, 1.0f,
		0.2f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanVolumeLong[] =
{
	{	1.0f, 1.0f,
		0.3f, 0.5f,
	},
	{	1.0f, 1.0f,
		0.6f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.3f, 0.4f,
	},
};

envelopePoint_t envZombieMoanIgnited[] =
{
	{	1.0f, 1.0f,
		0.5f, 1.0f,
	},
	{	1.0f, 1.0f,
		30.0f, 30.0f,
	},
	{	0.0f, 0.0f,
		0.5f, 1.0f,
	},
};


//=============================================================================
//=============================================================================

class CZombie : public CAI_BlendingHost<CNPC_BaseZombie>
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CZombie, CAI_BlendingHost<CNPC_BaseZombie> );

public:
	CZombie()
	 : m_DurationDoorBash( 2, 6),
	   m_NextTimeToStartDoorBash( 3.0 )
	{
	}

	void Spawn( void );
	void Precache( void );

	void SetZombieModel( void );
	void MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize );
	bool ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold );
	bool CanBecomeLiveTorso() { return !m_fIsHeadless; }

	void GatherConditions( void );

	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	int TranslateSchedule( int scheduleType );

#ifndef HL2_EPISODIC
	void CheckFlinches() {} // Zombie has custom flinch code
#endif // HL2_EPISODIC

	Activity NPC_TranslateActivity( Activity newActivity );

	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	virtual const char *GetLegsModel( void );
	virtual const char *GetTorsoModel( void );
	virtual const char *GetHeadcrabClassname( void );
	virtual const char *GetHeadcrabModel( void );

	virtual bool OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, 
								 CBaseDoor *pDoor,
								 float distClear, 
								 AIMoveResult_t *pResult );

	Activity SelectDoorBash();

	void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	void Extinguish();
	int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	bool IsHeavyDamage( const CTakeDamageInfo &info );
	bool IsSquashed( const CTakeDamageInfo &info );
	void BuildScheduleTestBits( void );

	void PrescheduleThink( void );
	int SelectSchedule ( void );

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot );

	const char *GetMoanSound( int nSound );
	
public:
	DEFINE_CUSTOM_AI;

protected:
	static const char *pMoanSounds[];


private:
	CHandle< CBaseDoor > m_hBlockingDoor;
	float				 m_flDoorBashYaw;
	
	CRandSimTimer 		 m_DurationDoorBash;
	CSimTimer 	  		 m_NextTimeToStartDoorBash;

	Vector				 m_vPositionCharged;
};

LINK_ENTITY_TO_CLASS( npc_zombie, CZombie );
LINK_ENTITY_TO_CLASS( npc_zombie_torso, CZombie );

//---------------------------------------------------------
//---------------------------------------------------------
const char *CZombie::pMoanSounds[] =
{
	 "NPC_BaseZombie.Moan1",
	 "NPC_BaseZombie.Moan2",
	 "NPC_BaseZombie.Moan3",
	 "NPC_BaseZombie.Moan4",
};

//=========================================================
// Conditions
//=========================================================
enum
{
	COND_BLOCKED_BY_DOOR = LAST_BASE_ZOMBIE_CONDITION,
	COND_DOOR_OPENED,
	COND_ZOMBIE_CHARGE_TARGET_MOVED,
};

//=========================================================
// Schedules
//=========================================================
enum
{
	SCHED_ZOMBIE_BASH_DOOR = LAST_BASE_ZOMBIE_SCHEDULE,
	SCHED_ZOMBIE_WANDER_ANGRILY,
	SCHED_ZOMBIE_CHARGE_ENEMY,
	SCHED_ZOMBIE_FAIL,
};

//=========================================================
// Tasks
//=========================================================
enum
{
	TASK_ZOMBIE_EXPRESS_ANGER = LAST_BASE_ZOMBIE_TASK,
	TASK_ZOMBIE_YAW_TO_DOOR,
	TASK_ZOMBIE_ATTACK_DOOR,
	TASK_ZOMBIE_CHARGE_ENEMY,
};

//-----------------------------------------------------------------------------

int ACT_ZOMBIE_TANTRUM;
int ACT_ZOMBIE_WALLPOUND;

BEGIN_DATADESC( CZombie )

	DEFINE_FIELD( m_hBlockingDoor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flDoorBashYaw, FIELD_FLOAT ),
	DEFINE_EMBEDDED( m_DurationDoorBash ),
	DEFINE_EMBEDDED( m_NextTimeToStartDoorBash ),
	DEFINE_FIELD( m_vPositionCharged, FIELD_POSITION_VECTOR ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( "models/zombie/classic.mdl" );
	PrecacheModel( "models/zombie/classic_torso.mdl" );
	PrecacheModel( "models/zombie/classic_legs.mdl" );

	PrecacheScriptSound( "Zombie.FootstepRight" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombie.ScuffRight" );
	PrecacheScriptSound( "Zombie.ScuffLeft" );
	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
	PrecacheScriptSound( "Zombie.Pain" );
	PrecacheScriptSound( "Zombie.Die" );
	PrecacheScriptSound( "Zombie.Alert" );
	PrecacheScriptSound( "Zombie.Idle" );
	PrecacheScriptSound( "Zombie.Attack" );

	PrecacheScriptSound( "NPC_BaseZombie.Moan1" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan2" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan3" );
	PrecacheScriptSound( "NPC_BaseZombie.Moan4" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::Spawn( void )
{
	Precache();

	if( FClassnameIs( this, "npc_zombie" ) )
	{
		m_fIsTorso = false;
	}
	else
	{
		// This was placed as an npc_zombie_torso
		m_fIsTorso = true;
	}

	m_fIsHeadless = false;

#ifdef HL2_EPISODIC
	SetBloodColor( BLOOD_COLOR_ZOMBIE );
#else
	SetBloodColor( BLOOD_COLOR_GREEN );
#endif // HL2_EPISODIC

	m_iHealth			= sk_zombie_health.GetFloat();
	m_flFieldOfView		= 0.2;

	CapabilitiesClear();

	//GetNavigator()->SetRememberStaleNodes( false );

	BaseClass::Spawn();

	m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 4.0 );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::PrescheduleThink( void )
{
  	if( gpGlobals->curtime > m_flNextMoanSound )
  	{
  		if( CanPlayMoanSound() )
  		{
			// Classic guy idles instead of moans.
			IdleSound();

  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 5.0 );
  		}
  		else
 		{
  			m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 1.0, 2.0 );
  		}
  	}

	BaseClass::PrescheduleThink();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CZombie::SelectSchedule ( void )
{
	if( HasCondition( COND_PHYSICS_DAMAGE ) && !m_ActBusyBehavior.IsActive() )
	{
		return SCHED_FLINCH_PHYSICS;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a footstep
//-----------------------------------------------------------------------------
void CZombie::FootstepSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound(  "Zombie.FootstepRight" );
	}
	else
	{
		EmitSound( "Zombie.FootstepLeft" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sound of a foot sliding/scraping
//-----------------------------------------------------------------------------
void CZombie::FootscuffSound( bool fRightFoot )
{
	if( fRightFoot )
	{
		EmitSound( "Zombie.ScuffRight" );
	}
	else
	{
		EmitSound( "Zombie.ScuffLeft" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack hit sound
//-----------------------------------------------------------------------------
void CZombie::AttackHitSound( void )
{
	EmitSound( "Zombie.AttackHit" );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack miss sound
//-----------------------------------------------------------------------------
void CZombie::AttackMissSound( void )
{
	// Play a random attack miss sound
	EmitSound( "Zombie.AttackMiss" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::PainSound( const CTakeDamageInfo &info )
{
	// We're constantly taking damage when we are on fire. Don't make all those noises!
	if ( IsOnFire() )
	{
		return;
	}

	EmitSound( "Zombie.Pain" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZombie::DeathSound( const CTakeDamageInfo &info ) 
{
	EmitSound( "Zombie.Die" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZombie::AlertSound( void )
{
	EmitSound( "Zombie.Alert" );

	// Don't let a moan sound cut off the alert sound.
	m_flNextMoanSound += random->RandomFloat( 2.0, 4.0 );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a moan sound for this class of zombie.
//-----------------------------------------------------------------------------
const char *CZombie::GetMoanSound( int nSound )
{
	return pMoanSounds[ nSound % ARRAYSIZE( pMoanSounds ) ];
}

//-----------------------------------------------------------------------------
// Purpose: Play a random idle sound.
//-----------------------------------------------------------------------------
void CZombie::IdleSound( void )
{
	if( GetState() == NPC_STATE_IDLE && random->RandomFloat( 0, 1 ) == 0 )
	{
		// Moan infrequently in IDLE state.
		return;
	}

	if( IsSlumped() )
	{
		// Sleeping zombies are quiet.
		return;
	}

	EmitSound( "Zombie.Idle" );
	MakeAISpookySound( 360.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Play a random attack sound.
//-----------------------------------------------------------------------------
void CZombie::AttackSound( void )
{
	EmitSound( "Zombie.Attack" );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the classname (ie "npc_headcrab") to spawn when our headcrab bails.
//-----------------------------------------------------------------------------
const char *CZombie::GetHeadcrabClassname( void )
{
	return "npc_headcrab";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char *CZombie::GetHeadcrabModel( void )
{
	return "models/headcrabclassic.mdl";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CZombie::GetLegsModel( void )
{
	return "models/zombie/classic_legs.mdl";
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
const char *CZombie::GetTorsoModel( void )
{
	return "models/zombie/classic_torso.mdl";
}


//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::SetZombieModel( void )
{
	Hull_t lastHull = GetHullType();

	if ( m_fIsTorso )
	{
		SetModel( "models/zombie/classic_torso.mdl" );
		SetHullType( HULL_TINY );
	}
	else
	{
		SetModel( "models/zombie/classic.mdl" );
		SetHullType( HULL_HUMAN );
	}

	SetBodygroup( ZOMBIE_BODYGROUP_HEADCRAB, !m_fIsHeadless );

	SetHullSizeNormal( true );
	SetDefaultEyeOffset();
	SetActivity( ACT_IDLE );

	// hull changed size, notify vphysics
	// UNDONE: Solve this generally, systematically so other
	// NPCs can change size
	if ( lastHull != GetHullType() )
	{
		if ( VPhysicsGetObject() )
		{
			SetupVPhysicsHull();
		}
	}
}

//---------------------------------------------------------
// Classic zombie only uses moan sound if on fire.
//---------------------------------------------------------
void CZombie::MoanSound( envelopePoint_t *pEnvelope, int iEnvelopeSize )
{
	if( IsOnFire() )
	{
		BaseClass::MoanSound( pEnvelope, iEnvelopeSize );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CZombie::ShouldBecomeTorso( const CTakeDamageInfo &info, float flDamageThreshold )
{
	if( IsSlumped() ) 
	{
		// Never break apart a slouched zombie. This is because the most fun
		// slouched zombies to kill are ones sleeping leaning against explosive
		// barrels. If you break them in half in the blast, the force of being
		// so close to the explosion makes the body pieces fly at ridiculous 
		// velocities because the pieces weigh less than the whole.
		return false;
	}

	return BaseClass::ShouldBecomeTorso( info, flDamageThreshold );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::GatherConditions( void )
{
	BaseClass::GatherConditions();

	static int conditionsToClear[] = 
	{
		COND_BLOCKED_BY_DOOR,
		COND_DOOR_OPENED,
		COND_ZOMBIE_CHARGE_TARGET_MOVED,
	};

	ClearConditions( conditionsToClear, ARRAYSIZE( conditionsToClear ) );

	if ( m_hBlockingDoor == NULL || 
		 ( m_hBlockingDoor->m_toggle_state == TS_AT_TOP || 
		   m_hBlockingDoor->m_toggle_state == TS_GOING_UP )  )
	{
		ClearCondition( COND_BLOCKED_BY_DOOR );
		if ( m_hBlockingDoor != NULL )
		{
			SetCondition( COND_DOOR_OPENED );
			m_hBlockingDoor = NULL;
		}
	}
	else
		SetCondition( COND_BLOCKED_BY_DOOR );

	if ( ConditionInterruptsCurSchedule( COND_ZOMBIE_CHARGE_TARGET_MOVED ) )
	{
		if ( GetNavigator()->IsGoalActive() )
		{
			const float CHARGE_RESET_TOLERANCE = 60.0;
			if ( !GetEnemy() ||
				 ( m_vPositionCharged - GetEnemyLKP()  ).Length() > CHARGE_RESET_TOLERANCE )
			{
				SetCondition( COND_ZOMBIE_CHARGE_TARGET_MOVED );
			}
				 
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

int CZombie::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( HasCondition( COND_BLOCKED_BY_DOOR ) && m_hBlockingDoor != NULL )
	{
		ClearCondition( COND_BLOCKED_BY_DOOR );
		if ( m_NextTimeToStartDoorBash.Expired() && failedSchedule != SCHED_ZOMBIE_BASH_DOOR )
			return SCHED_ZOMBIE_BASH_DOOR;
		m_hBlockingDoor = NULL;
	}

	if ( failedSchedule != SCHED_ZOMBIE_CHARGE_ENEMY && 
		 IsPathTaskFailure( taskFailCode ) &&
		 random->RandomInt( 1, 100 ) < 50 )
	{
		return SCHED_ZOMBIE_CHARGE_ENEMY;
	}

	if ( failedSchedule != SCHED_ZOMBIE_WANDER_ANGRILY &&
		 ( failedSchedule == SCHED_TAKE_COVER_FROM_ENEMY || 
		   failedSchedule == SCHED_CHASE_ENEMY_FAILED ) )
	{
		return SCHED_ZOMBIE_WANDER_ANGRILY;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//---------------------------------------------------------
//---------------------------------------------------------

int CZombie::TranslateSchedule( int scheduleType )
{
	if ( scheduleType == SCHED_COMBAT_FACE && IsUnreachable( GetEnemy() ) )
		return SCHED_TAKE_COVER_FROM_ENEMY;

	if ( !m_fIsTorso && scheduleType == SCHED_FAIL )
		return SCHED_ZOMBIE_FAIL;

	return BaseClass::TranslateSchedule( scheduleType );
}

//---------------------------------------------------------

Activity CZombie::NPC_TranslateActivity( Activity newActivity )
{
	newActivity = BaseClass::NPC_TranslateActivity( newActivity );

	if ( newActivity == ACT_RUN )
		return ACT_WALK;
		
	if ( m_fIsTorso && ( newActivity == ACT_ZOMBIE_TANTRUM ) )
		return ACT_IDLE;

	return newActivity;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::OnStateChange( NPC_STATE OldState, NPC_STATE NewState )
{
	BaseClass::OnStateChange( OldState, NewState );
}

//---------------------------------------------------------
//---------------------------------------------------------

void CZombie::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( random->RandomInt( 1, 4 ) == 2 )
			{
				SetIdealActivity( (Activity)ACT_ZOMBIE_TANTRUM );
			}
			else
			{
				TaskComplete();
			}

			break;
		}

	case TASK_ZOMBIE_YAW_TO_DOOR:
		{
			AssertMsg( m_hBlockingDoor != NULL, "Expected condition handling to break schedule before landing here" );
			if ( m_hBlockingDoor != NULL )
			{
				GetMotor()->SetIdealYaw( m_flDoorBashYaw );
			}
			TaskComplete();
			break;
		}

	case TASK_ZOMBIE_ATTACK_DOOR:
		{
		 	m_DurationDoorBash.Reset();
			SetIdealActivity( SelectDoorBash() );
			break;
		}

	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			if ( !GetEnemy() )
				TaskFail( FAIL_NO_ENEMY );
			else if ( GetNavigator()->SetVectorGoalFromTarget( GetEnemy()->GetLocalOrigin() ) )
			{
				m_vPositionCharged = GetEnemy()->GetLocalOrigin();
				TaskComplete();
			}
			else
				TaskFail( FAIL_NO_ROUTE );
			break;
		}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

void CZombie::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ZOMBIE_ATTACK_DOOR:
		{
			if ( IsActivityFinished() )
			{
				if ( m_DurationDoorBash.Expired() )
				{
					TaskComplete();
					m_NextTimeToStartDoorBash.Reset();
				}
				else
					ResetIdealActivity( SelectDoorBash() );
			}
			break;
		}

	case TASK_ZOMBIE_CHARGE_ENEMY:
		{
			break;
		}

	case TASK_ZOMBIE_EXPRESS_ANGER:
		{
			if ( IsActivityFinished() )
			{
				TaskComplete();
			}
			break;
		}

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

bool CZombie::OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, 
							  float distClear, AIMoveResult_t *pResult )
{
	if ( BaseClass::OnObstructingDoor( pMoveGoal, pDoor, distClear, pResult ) )
	{
		if  ( IsMoveBlocked( *pResult ) && pMoveGoal->directTrace.vHitNormal != vec3_origin )
		{
			m_hBlockingDoor = pDoor;
			m_flDoorBashYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );	
		}
		return true;
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------

Activity CZombie::SelectDoorBash()
{
	if ( random->RandomInt( 1, 3 ) == 1 )
		return ACT_MELEE_ATTACK1;
	return (Activity)ACT_ZOMBIE_WALLPOUND;
}

//---------------------------------------------------------
// Zombies should scream continuously while burning, so long
// as they are alive... but NOT IN GERMANY!
//---------------------------------------------------------
void CZombie::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
 	if( !IsOnFire() && IsAlive() )
	{
		BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

		if ( !UTIL_IsLowViolence() )
		{
			RemoveSpawnFlags( SF_NPC_GAG );

			MoanSound( envZombieMoanIgnited, ARRAYSIZE( envZombieMoanIgnited ) );

			if ( m_pMoanSound )
			{
				ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 120, 1.0 );
				ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 1, 1.0 );
			}
		}
	}
}

//---------------------------------------------------------
// If a zombie stops burning and hasn't died, quiet him down
//---------------------------------------------------------
void CZombie::Extinguish()
{
	if( m_pMoanSound )
	{
		ENVELOPE_CONTROLLER.SoundChangeVolume( m_pMoanSound, 0, 2.0 );
		ENVELOPE_CONTROLLER.SoundChangePitch( m_pMoanSound, 100, 2.0 );
		m_flNextMoanSound = gpGlobals->curtime + random->RandomFloat( 2.0, 4.0 );
	}

	BaseClass::Extinguish();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CZombie::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
#ifndef HL2_EPISODIC
	if ( inputInfo.GetDamageType() & DMG_BUCKSHOT )
	{
		if( !m_fIsTorso && inputInfo.GetDamage() > (m_iMaxHealth/3) )
		{
			// Always flinch if damaged a lot by buckshot, even if not shot in the head.
			// The reason for making sure we did at least 1/3rd of the zombie's max health
			// is so the zombie doesn't flinch every time the odd shotgun pellet hits them,
			// and so the maximum number of times you'll see a zombie flinch like this is 2.(sjb)
			AddGesture( ACT_GESTURE_FLINCH_HEAD );
		}
	}
#endif // HL2_EPISODIC

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CZombie::IsHeavyDamage( const CTakeDamageInfo &info )
{
#ifdef HL2_EPISODIC
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		if ( !m_fIsTorso && info.GetDamage() > (m_iMaxHealth/3) )
			return true;
	}

	// Randomly treat all damage as heavy
	if ( info.GetDamageType() & (DMG_BULLET | DMG_BUCKSHOT) )
	{
		// Don't randomly flinch if I'm melee attacking
		if ( !HasCondition(COND_CAN_MELEE_ATTACK1) && (RandomFloat() > 0.5) )
		{
			// Randomly forget I've flinched, so that I'll be forced to play a big flinch
			// If this doesn't happen, it means I may not fully flinch if I recently flinched
			if ( RandomFloat() > 0.75 )
			{
				Forget(bits_MEMORY_FLINCHED);
			}

			return true;
		}
	}
#endif // HL2_EPISODIC

	return BaseClass::IsHeavyDamage(info);
}

//---------------------------------------------------------
//---------------------------------------------------------
#define ZOMBIE_SQUASH_MASS	300.0f  // Anything this heavy or heavier squashes a zombie good. (show special fx)
bool CZombie::IsSquashed( const CTakeDamageInfo &info )
{
	if( GetHealth() > 0 )
	{
		return false;
	}

	if( info.GetDamageType() & DMG_CRUSH )
	{
		IPhysicsObject *pCrusher = info.GetInflictor()->VPhysicsGetObject();
		if( pCrusher && pCrusher->GetMass() >= ZOMBIE_SQUASH_MASS && info.GetInflictor()->WorldSpaceCenter().z > EyePosition().z )
		{
			// This heuristic detects when a zombie has been squashed from above by a heavy
			// item. Done specifically so we can add gore effects to Ravenholm cartraps.
			// The zombie must take physics damage from a 300+kg object that is centered above its eyes (comes from above)
			return true;
		}
	}

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CZombie::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if( !m_fIsTorso && !IsCurSchedule( SCHED_FLINCH_PHYSICS ) && !m_ActBusyBehavior.IsActive() )
	{
		SetCustomInterruptCondition( COND_PHYSICS_DAMAGE );
	}
}

	
//=============================================================================

AI_BEGIN_CUSTOM_NPC( npc_zombie, CZombie )

	DECLARE_CONDITION( COND_BLOCKED_BY_DOOR )
	DECLARE_CONDITION( COND_DOOR_OPENED )
	DECLARE_CONDITION( COND_ZOMBIE_CHARGE_TARGET_MOVED )

	DECLARE_TASK( TASK_ZOMBIE_EXPRESS_ANGER )
	DECLARE_TASK( TASK_ZOMBIE_YAW_TO_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_ATTACK_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_CHARGE_ENEMY )
	
	DECLARE_ACTIVITY( ACT_ZOMBIE_TANTRUM );
	DECLARE_ACTIVITY( ACT_ZOMBIE_WALLPOUND );

	DEFINE_SCHEDULE
	( 
		SCHED_ZOMBIE_BASH_DOOR,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_ZOMBIE_YAW_TO_DOOR			0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_ZOMBIE_ATTACK_DOOR			0"
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_WANDER_ANGRILY,

		"	Tasks"
		"		TASK_WANDER						480240" // 48 units to 240 units.
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			4"
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_CHARGE_ENEMY,


		"	Tasks"
		"		TASK_ZOMBIE_CHARGE_ENEMY		0"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_ZOMBIE_TANTRUM" /* placeholder until frustration/rage/fence shake animation available */
		""
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
		"		COND_ZOMBIE_CHARGE_TARGET_MOVED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_FAIL,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_ZOMBIE_TANTRUM"
		"		TASK_WAIT				1"
		"		TASK_WAIT_PVS			0"
		""
		"	Interrupts"
		"		COND_CAN_RANGE_ATTACK1 "
		"		COND_CAN_RANGE_ATTACK2 "
		"		COND_CAN_MELEE_ATTACK1 "
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_GIVE_WAY"
		"		COND_DOOR_OPENED"
	)

AI_END_CUSTOM_NPC()

//=============================================================================
