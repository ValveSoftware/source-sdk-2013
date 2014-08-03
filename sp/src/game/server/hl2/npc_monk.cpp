//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Father Grigori, a benevolent monk who is the last remaining human
//			in Ravenholm. He keeps to the rooftops and uses a big ole elephant
//			gun to send his zombified former friends to a peaceful death.
//
//=============================================================================//

#include "cbase.h"
#include "ai_baseactor.h"
#include "ai_hull.h"
#include "ammodef.h"
#include "gamerules.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "ai_behavior.h"
#include "ai_behavior_assault.h"
#include "ai_behavior_lead.h"
#include "npcevent.h"
#include "ai_playerally.h"
#include "ai_senses.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar monk_headshot_freq( "monk_headshot_freq", "2" );

//-----------------------------------------------------------------------------
// Activities.
//-----------------------------------------------------------------------------
int ACT_MONK_GUN_IDLE;

class CNPC_Monk : public CAI_PlayerAlly
{
	DECLARE_CLASS( CNPC_Monk, CAI_PlayerAlly );

public:

	CNPC_Monk() {}
	void Spawn();
	void Precache();

	bool CreateBehaviors();
	int GetSoundInterests();
	void BuildScheduleTestBits( void );
	Class_T	Classify( void );

	bool ShouldBackAway();

	bool IsValidEnemy( CBaseEntity *pEnemy );

	int	TranslateSchedule( int scheduleType );
	int	SelectSchedule ();

	void HandleAnimEvent( animevent_t *pEvent );
	Activity NPC_TranslateActivity( Activity eNewActivity );

	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	
	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );
	Vector GetActualShootPosition( const Vector &shootOrigin );
	Vector GetActualShootTrajectory( const Vector &shootOrigin );

	void PrescheduleThink();

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void GatherConditions();

	bool PassesDamageFilter( const CTakeDamageInfo &info );
	void OnKilledNPC( CBaseCombatCharacter *pKilled );

	bool IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	DECLARE_DATADESC();

private:
	//-----------------------------------------------------
	// Conditions, Schedules, Tasks
	//-----------------------------------------------------
	enum
	{
		SCHED_MONK_RANGE_ATTACK1 = BaseClass::NEXT_SCHEDULE,
		SCHED_MONK_BACK_AWAY_FROM_ENEMY,
		SCHED_MONK_BACK_AWAY_AND_RELOAD,
		SCHED_MONK_NORMAL_RELOAD,
	};

	/*enum
	{
		//TASK_MONK_FIRST_TASK = BaseClass::NEXT_TASK,
	};*/

	DEFINE_CUSTOM_AI;

	// Inputs
	void	InputPerfectAccuracyOn( inputdata_t &inputdata );
	void	InputPerfectAccuracyOff( inputdata_t &inputdata );
	
	CAI_AssaultBehavior		m_AssaultBehavior;
	CAI_LeadBehavior		m_LeadBehavior;
	int						m_iNumZombies;
	int						m_iDangerousZombies;
	bool					m_bPerfectAccuracy;
	bool					m_bMournedPlayer;

};

BEGIN_DATADESC( CNPC_Monk )
//					m_AssaultBehavior
//					m_LeadBehavior
	DEFINE_FIELD( m_iNumZombies, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDangerousZombies, FIELD_INTEGER ),
	DEFINE_FIELD( m_bPerfectAccuracy, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMournedPlayer, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "PerfectAccuracyOn", InputPerfectAccuracyOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PerfectAccuracyOff", InputPerfectAccuracyOff ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_monk, CNPC_Monk );

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Monk::CreateBehaviors()
{
	AddBehavior( &m_LeadBehavior );
	AddBehavior( &m_AssaultBehavior );
	
	return BaseClass::CreateBehaviors();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Monk::GetSoundInterests()
{
	return	SOUND_WORLD		|
			SOUND_COMBAT	|
			SOUND_PLAYER	|
			SOUND_DANGER;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::BuildScheduleTestBits( void )
{
	// FIXME: we need a way to make scenes non-interruptible
#if 0
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) || IsCurSchedule( SCHED_SCENE_GENERIC ) )
	{
		ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_NEW_ENEMY );
		ClearCustomInterruptCondition( COND_HEAR_DANGER );
	}
#endif

	// Don't interrupt while shooting the gun
	const Task_t* pTask = GetTask();
	if ( pTask && (pTask->iTask == TASK_RANGE_ATTACK1) )
	{
		ClearCustomInterruptCondition( COND_HEAVY_DAMAGE );
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
		ClearCustomInterruptCondition( COND_HEAR_DANGER );
		ClearCustomInterruptCondition( COND_WEAPON_BLOCKED_BY_FRIEND );
		ClearCustomInterruptCondition( COND_WEAPON_SIGHT_OCCLUDED );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Class_T	CNPC_Monk::Classify( void )
{
	return CLASS_PLAYER_ALLY_VITAL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CNPC_Monk::NPC_TranslateActivity( Activity eNewActivity )
{
	eNewActivity = BaseClass::NPC_TranslateActivity( eNewActivity );

	if ( (m_NPCState == NPC_STATE_COMBAT || m_NPCState == NPC_STATE_ALERT) )
	{
		bool bGunUp = false;

		bGunUp = (gpGlobals->curtime - m_flLastAttackTime < 4);
		bGunUp = bGunUp || (GetEnemy() && !HasCondition( COND_TOO_FAR_TO_ATTACK ));

		if (bGunUp)
		{
			if ( eNewActivity == ACT_IDLE )
			{
				eNewActivity = ACT_IDLE_ANGRY;
			}
			// keep aiming a little longer than normal since the shot takes so long and there's no good way to do a transitions between movement types :/
			else if ( eNewActivity == ACT_WALK )
			{
				eNewActivity = ACT_WALK_AIM;
			}
			else if ( eNewActivity == ACT_RUN )
			{
				eNewActivity = ACT_RUN_AIM;
			}
		}
	}

	// We need these so that we can pick up the shotgun to throw it in the balcony scene
	if ( eNewActivity == ACT_IDLE_ANGRY_SHOTGUN )
	{
		eNewActivity = ACT_IDLE_ANGRY_SMG1;
	}
	else if ( eNewActivity == ACT_WALK_AIM_SHOTGUN )
	{
		eNewActivity = ACT_WALK_AIM_RIFLE;
	}
	else if ( eNewActivity == ACT_RUN_AIM_SHOTGUN )
	{
		eNewActivity = ACT_RUN_AIM_RIFLE;
	}
	else if ( eNewActivity == ACT_RANGE_ATTACK_SHOTGUN_LOW )
	{
		return ACT_RANGE_ATTACK_SMG1_LOW;
	}

	return eNewActivity;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::Precache()
{
	PrecacheModel( "models/Monk.mdl" );
	
	PrecacheScriptSound( "NPC_Citizen.FootstepLeft" );
	PrecacheScriptSound( "NPC_Citizen.FootstepRight" );

	BaseClass::Precache();
}
 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Monk::Spawn()
{
	Precache();

	BaseClass::Spawn();

	SetModel( "models/Monk.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= 100;
	m_flFieldOfView		= m_flFieldOfView = -0.707; // 270`
	m_NPCState			= NPC_STATE_NONE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	CapabilitiesAdd( bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MOVE_GROUND );
	CapabilitiesAdd( bits_CAP_USE_WEAPONS );
	CapabilitiesAdd( bits_CAP_ANIMATEDFACE );
	CapabilitiesAdd( bits_CAP_FRIENDLY_DMG_IMMUNE );
	CapabilitiesAdd( bits_CAP_AIM_GUN );
	CapabilitiesAdd( bits_CAP_MOVE_SHOOT );

	NPCInit();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Monk::PainSound( const CTakeDamageInfo &info )
{
	SpeakIfAllowed( TLK_WOUND );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void CNPC_Monk::DeathSound( const CTakeDamageInfo &info )
{
	// Sentences don't play on dead NPCs
	SentenceStop();

	Speak( TLK_DEATH );
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
WeaponProficiency_t CNPC_Monk::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	return WEAPON_PROFICIENCY_PERFECT;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_Monk::GetActualShootPosition( const Vector &shootOrigin )
{
	return BaseClass::GetActualShootPosition( shootOrigin );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
Vector CNPC_Monk::GetActualShootTrajectory( const Vector &shootOrigin )
{
	if( GetEnemy() && GetEnemy()->Classify() == CLASS_ZOMBIE )
	{
		Vector vecShootDir;

		if( m_bPerfectAccuracy || random->RandomInt( 1, monk_headshot_freq.GetInt() ) == 1 )
		{
			vecShootDir = GetEnemy()->HeadTarget( shootOrigin ) - shootOrigin;
		}
		else
		{
			vecShootDir = GetEnemy()->BodyTarget( shootOrigin ) - shootOrigin;
		}

		VectorNormalize( vecShootDir );
		return vecShootDir;
	}

	return BaseClass::GetActualShootTrajectory( shootOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pEvent - 
//-----------------------------------------------------------------------------
void CNPC_Monk::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
		case NPC_EVENT_LEFTFOOT:
			{
				EmitSound( "NPC_Citizen.FootstepLeft", pEvent->eventtime );
			}
			break;
		case NPC_EVENT_RIGHTFOOT:
			{
				EmitSound( "NPC_Citizen.FootstepRight", pEvent->eventtime );
			}
			break;

		default:
			BaseClass::HandleAnimEvent( pEvent );
			break;
	}
}

//-------------------------------------
// Grigori tries to stand his ground until
// enemies are very close.
//-------------------------------------
#define MONK_STAND_GROUND_HEIGHT	24.0
bool CNPC_Monk::ShouldBackAway()
{
	if( !GetEnemy() )
		return false;

	if( GetAbsOrigin().z - GetEnemy()->GetAbsOrigin().z >= MONK_STAND_GROUND_HEIGHT )
	{
		// This is a fairly special case. Grigori looks better fighting from his assault points in the
		// elevated places of the Graveyard, so we prevent his back away behavior anytime he has a height
		// advantage on his enemy.
		return false;
	}

	float flDist;
	flDist = ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).Length();

	if( flDist <= 180 )
		return true;

	return false;
}

//-------------------------------------

bool CNPC_Monk::IsValidEnemy( CBaseEntity *pEnemy )
{
	if ( BaseClass::IsValidEnemy( pEnemy ) && GetActiveWeapon() )
	{
		float flDist;

		flDist = ( GetAbsOrigin() - pEnemy->GetAbsOrigin() ).Length();
		if( flDist <= GetActiveWeapon()->m_fMaxRange1 )
			return true;
	}
	return false;
}


//-------------------------------------

int CNPC_Monk::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
	case SCHED_MOVE_AWAY_FAIL:
		// Our first method of backing away failed. Try another.
		return SCHED_MONK_BACK_AWAY_FROM_ENEMY;
		break;

	case SCHED_RANGE_ATTACK1:
		{
			if( ShouldBackAway() )
			{
				// Get some room, rely on move and shoot.
				return SCHED_MOVE_AWAY;
			}

			return SCHED_MONK_RANGE_ATTACK1;
		}
		break;

	case SCHED_HIDE_AND_RELOAD:
	case SCHED_RELOAD:
		if( ShouldBackAway() )
		{
			return SCHED_MONK_BACK_AWAY_AND_RELOAD;
		}

		return SCHED_RELOAD;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}


//-------------------------------------

void CNPC_Monk::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
}	

//-------------------------------------

int CNPC_Monk::SelectSchedule()
{
	if( HasCondition( COND_HEAR_DANGER ) )
	{
		SpeakIfAllowed( TLK_DANGER );
		return SCHED_TAKE_COVER_FROM_BEST_SOUND;
	}

	if ( HasCondition( COND_TALKER_PLAYER_DEAD ) && !m_bMournedPlayer && IsOkToSpeak() )
	{
		m_bMournedPlayer = true;
		Speak( TLK_IDLE );
	}

	if( !BehaviorSelectSchedule() )
	{
		if ( HasCondition ( COND_NO_PRIMARY_AMMO ) )
		{
			return SCHED_HIDE_AND_RELOAD;
		}
	}

	return BaseClass::SelectSchedule();
}

//-------------------------------------

void CNPC_Monk::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RELOAD:
		{
			if ( GetActiveWeapon() && GetActiveWeapon()->HasPrimaryAmmo() )
			{
				// Don't reload if you have done so while moving (See BACK_AWAY_AND_RELOAD schedule).
				TaskComplete();
				return;
			}

			if( m_iNumZombies >= 2 && random->RandomInt( 1, 3 ) == 1 )
			{
				SpeakIfAllowed( TLK_ATTACKING );
			}

			Activity reloadGesture = TranslateActivity( ACT_GESTURE_RELOAD );
			if ( reloadGesture != ACT_INVALID && IsPlayingGesture( reloadGesture ) )
			{
				ResetIdealActivity( ACT_IDLE );
				return;
			}

			BaseClass::StartTask( pTask );
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


void CNPC_Monk::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RELOAD:
		{
			Activity reloadGesture = TranslateActivity( ACT_GESTURE_RELOAD );
			if ( GetIdealActivity() != ACT_RELOAD && reloadGesture != ACT_INVALID )
			{
				if ( !IsPlayingGesture( reloadGesture ) )
				{
					if ( GetShotRegulator() )
					{
						GetShotRegulator()->Reset( false );
					}

					TaskComplete();
				}
				return;
			}

			BaseClass::RunTask( pTask );
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Monk::GatherConditions()
{
	BaseClass::GatherConditions();

	// Build my zombie danger index!
	m_iNumZombies = 0;
	m_iDangerousZombies = 0;

	AISightIter_t iter;
	CBaseEntity *pSightEnt;
	pSightEnt = GetSenses()->GetFirstSeenEntity( &iter );
	while( pSightEnt )
	{
		if( pSightEnt->Classify() == CLASS_ZOMBIE && pSightEnt->IsAlive() )
		{
			// Is this zombie coming for me?
			CAI_BaseNPC *pZombie = dynamic_cast<CAI_BaseNPC*>(pSightEnt);
			
			if( pZombie && pZombie->GetEnemy() == this )
			{
				m_iNumZombies++;

				// if this zombie is close enough to attack, add him to the zombie danger!
				float flDist;

				flDist = (pZombie->GetAbsOrigin() - GetAbsOrigin()).Length2DSqr();

				if( flDist <= 128.0f * 128.0f )
				{
					m_iDangerousZombies++;
				}
			}
		}

		pSightEnt = GetSenses()->GetNextSeenEntity( &iter );
	}

	if( m_iDangerousZombies >= 3 || (GetEnemy() && GetHealth() < 25) )
	{
		// I see many zombies, or I'm quite injured.
		SpeakIfAllowed( TLK_HELP_ME );
	}

	// NOTE!!!!!! This code assumes grigori is using annabelle!
	ClearCondition(COND_LOW_PRIMARY_AMMO);
	if ( GetActiveWeapon() )
	{
		if ( GetActiveWeapon()->UsesPrimaryAmmo() )
		{
			if (!GetActiveWeapon()->HasPrimaryAmmo() )
			{
				SetCondition(COND_NO_PRIMARY_AMMO);
			}
			else if ( m_NPCState != NPC_STATE_COMBAT && GetActiveWeapon()->UsesClipsForAmmo1() && GetActiveWeapon()->Clip1() < 2 )
			{
				// Don't send a low ammo message unless we're not in combat.
				SetCondition(COND_LOW_PRIMARY_AMMO);
			}
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Monk::PassesDamageFilter( const CTakeDamageInfo &info )
{
	if ( info.GetAttacker()->ClassMatches( "npc_headcrab_black" ) || info.GetAttacker()->ClassMatches( "npc_headcrab_poison" ) )
		return false;

	return BaseClass::PassesDamageFilter( info );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Monk::OnKilledNPC( CBaseCombatCharacter *pKilled )
{
	if ( !pKilled )
	{
		return;
	}

	if ( pKilled->Classify() == CLASS_ZOMBIE )
	{
		// Don't speak if the gun is empty, cause grigori will want to speak while he's reloading.
		if ( GetActiveWeapon() )
		{
			if ( GetActiveWeapon()->UsesPrimaryAmmo() && !GetActiveWeapon()->HasPrimaryAmmo() )
			{
				// Gun is empty. I'm about to reload.
				if( m_iNumZombies >= 2 )
				{
					// Don't talk about killing a single zombie if there are more coming.
					// the reload behavior will say "come to me, children", etc.
					return;
				}
			}
		}

		if( m_iNumZombies == 1 || random->RandomInt( 1, 3 ) == 1 )
		{
			SpeakIfAllowed( TLK_ENEMY_DEAD );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Monk::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if( failedSchedule == SCHED_MONK_BACK_AWAY_FROM_ENEMY )
	{
		if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
		{
			// Most likely backed into a corner. Just blaze away.
			return SCHED_MONK_RANGE_ATTACK1;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Monk::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	if ( startPos.z - endPos.z < 0 )
		return false;
	return BaseClass::IsJumpLegal( startPos, apex, endPos );
}

//-----------------------------------------------------------------------------
// Every shot's a headshot. Useful for scripted Grigoris
//-----------------------------------------------------------------------------
void CNPC_Monk::InputPerfectAccuracyOn( inputdata_t &inputdata )
{
	m_bPerfectAccuracy = true;
}

//-----------------------------------------------------------------------------
// Turn off perfect accuracy.
//-----------------------------------------------------------------------------
void CNPC_Monk::InputPerfectAccuracyOff( inputdata_t &inputdata )
{
	m_bPerfectAccuracy = false;
}


//-----------------------------------------------------------------------------
//
// CNPC_Monk Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_monk, CNPC_Monk )

	DECLARE_ACTIVITY( ACT_MONK_GUN_IDLE )

	DEFINE_SCHEDULE
	(
		SCHED_MONK_RANGE_ATTACK1,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_FACE_ENEMY			0"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"		TASK_RANGE_ATTACK1		0"
		""
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
		"		COND_ENEMY_OCCLUDED"
		"		COND_HEAR_DANGER"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_MONK_BACK_AWAY_FROM_ENEMY,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
		"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
		"		TASK_WALK_PATH_TIMED						4.0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
	);

	DEFINE_SCHEDULE
	(
		SCHED_MONK_BACK_AWAY_AND_RELOAD,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_MONK_NORMAL_RELOAD"
		"		TASK_STOP_MOVING							0"
		"		TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION	0"
		"		TASK_FIND_BACKAWAY_FROM_SAVEPOSITION		0"
		"		TASK_WALK_PATH_TIMED						2.0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_STOP_MOVING							0"
		"		TASK_RELOAD									0"
		""
		"	Interrupts"
		"		COND_ENEMY_DEAD"
	);

	DEFINE_SCHEDULE
	(
		SCHED_MONK_NORMAL_RELOAD,

		"	Tasks"
		"		TASK_STOP_MOVING							0"
		"		TASK_RELOAD									0"
		""
		"	Interrupts"
		"		COND_HEAR_DANGER"
	);


AI_END_CUSTOM_NPC()
