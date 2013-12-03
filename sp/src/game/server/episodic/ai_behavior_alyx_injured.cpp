//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: FIXME: This will ultimately become a more generic implementation
//
//=============================================================================

#include "cbase.h"
#include "ai_memory.h"
#include "ai_speech.h"
#include "ai_behavior.h"
#include "ai_navigator.h"
#include "ai_playerally.h"
#include "ai_behavior_follow.h"
#include "ai_moveprobe.h"

#include "ai_behavior_alyx_injured.h"

ConVar g_debug_injured_follow( "g_debug_injured_follow", "0" );
ConVar injured_help_plee_range( "injured_help_plee_range", "256" );

#define	TLK_INJURED_FOLLOW_TOO_FAR	"TLK_INJURED_FOLLOW_TOO_FAR"

BEGIN_DATADESC( CAI_BehaviorAlyxInjured )
	DEFINE_FIELD( m_flNextWarnTime,	FIELD_TIME ),
	// m_ActivityMap

END_DATADESC();

Activity	ACT_INJURED_COWER;
Activity	ACT_GESTURE_INJURED_COWER_FLINCH;

#define	COVER_DISTANCE	128.0f			// Distance behind target to find cover
#define	MIN_ENEMY_MOB	3				// Number of enemies considerd overwhelming
#define	MAX_DIST_FROM_FOLLOW_TARGET	256	// If the follow target is farther than this, the NPC will run to it

//=============================================================================

CAI_BehaviorAlyxInjured::CAI_BehaviorAlyxInjured( void ) : m_flNextWarnTime( 0.0f )
{
	SetDefLessFunc( m_ActivityMap );
}

struct ActivityMap_t
{
	Activity		activity;
	Activity		translation;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::PopulateActivityMap( void )
{
	// Maps one activity to a translated one
	ActivityMap_t map[] =
	{
		// Runs
		{ ACT_RUN,					ACT_RUN_HURT },
		{ ACT_RUN_AIM,				ACT_RUN_AIM },	// FIMXE: No appropriate temp anim right now!
		{ ACT_RUN_CROUCH,			ACT_RUN_HURT },
		{ ACT_RUN_CROUCH_AIM,		ACT_RUN_HURT },
		{ ACT_RUN_PROTECTED,		ACT_RUN_HURT },
		{ ACT_RUN_RELAXED,			ACT_RUN_HURT },
		{ ACT_RUN_STIMULATED,		ACT_RUN_HURT },
		{ ACT_RUN_AGITATED,			ACT_RUN_HURT },
		{ ACT_RUN_AIM_RELAXED,		ACT_RUN_AIM_RELAXED },		// FIMXE: No appropriate temp anim right now!
		{ ACT_RUN_AIM_STIMULATED,	ACT_RUN_AIM_STIMULATED },	// FIMXE: No appropriate temp anim right now!
		{ ACT_RUN_AIM_AGITATED,		ACT_RUN_AIM_AGITATED },		// FIMXE: No appropriate temp anim right now!
		{ ACT_RUN_HURT,				ACT_RUN_HURT },
		
		// Walks
		{ ACT_WALK,					ACT_WALK_HURT },
		{ ACT_WALK_AIM,				ACT_WALK_HURT },
		{ ACT_WALK_CROUCH,			ACT_WALK_HURT },
		{ ACT_WALK_CROUCH_AIM,		ACT_WALK_HURT },
		{ ACT_WALK_RELAXED,			ACT_WALK_HURT },
		{ ACT_WALK_STIMULATED,		ACT_WALK_HURT },
		{ ACT_WALK_AGITATED,		ACT_WALK_HURT },
		{ ACT_WALK_AIM_RELAXED,		ACT_WALK_HURT },
		{ ACT_WALK_AIM_STIMULATED,	ACT_WALK_HURT },
		{ ACT_WALK_AIM_AGITATED,	ACT_WALK_HURT },
		{ ACT_WALK_HURT,			ACT_WALK_HURT },

		{ ACT_IDLE,					ACT_IDLE_HURT },
		{ ACT_COVER_LOW,			ACT_INJURED_COWER },
		{ ACT_COWER,				ACT_INJURED_COWER },
	};

	// Clear the map
	m_ActivityMap.RemoveAll();

	// Add all translations
	for ( int i = 0; i < ARRAYSIZE( map ); i++ )
	{
		Assert( m_ActivityMap.Find( map[i].activity ) == m_ActivityMap.InvalidIndex() );
		m_ActivityMap.Insert( map[i].activity, map[i].translation );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Populate the list after save/load
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::OnRestore( void )
{
	PopulateActivityMap();
}

//-----------------------------------------------------------------------------
// Purpose: Populate the list on spawn
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::Spawn( void )
{
	PopulateActivityMap();
}

//-----------------------------------------------------------------------------
// Purpose: Get the flinch activity for us to play
// Input  : bHeavyDamage - 
//			bGesture - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CAI_BehaviorAlyxInjured::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	// 
	if ( ( bGesture == false ) || ( GetOuter()->GetActivity() != ACT_COWER ) )
		return BaseClass::GetFlinchActivity( bHeavyDamage, bGesture );

	// Translate the flinch if we're cowering
	return ACT_GESTURE_INJURED_COWER_FLINCH;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nActivity - 
//-----------------------------------------------------------------------------
Activity CAI_BehaviorAlyxInjured::NPC_TranslateActivity( Activity nActivity )
{
	// Find out what the base class wants to do with the activity
	Activity nNewActivity = BaseClass::NPC_TranslateActivity( nActivity );

	// Look it up in the translation map
	int nIndex = m_ActivityMap.Find( nNewActivity );
	
	if ( m_ActivityMap.IsValidIndex( nIndex ) )
		return m_ActivityMap[nIndex];

	return nNewActivity;
}

//-----------------------------------------------------------------------------
// Purpose: Determines if Alyx should run away from enemies or stay put
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_BehaviorAlyxInjured::ShouldRunToCover( void )
{
	Vector	vecRetreatPos;
	float	flRetreatRadius = 128.0f;
	
	// See how far off from our cover position we are
	if ( FindCoverFromEnemyBehindTarget( GetFollowTarget(), flRetreatRadius, &vecRetreatPos ) )
	{
		float flDestDistSqr = ( GetOuter()->WorldSpaceCenter() - vecRetreatPos ).LengthSqr();
		if ( flDestDistSqr > Square( flRetreatRadius ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: See if we need to follow our goal
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_BehaviorAlyxInjured::ShouldRunToFollowGoal( void )
{
	// If we're too far from our follow target, we need to chase after them
	float flDistToFollowGoalSqr = ( GetOuter()->GetAbsOrigin() - GetFollowTarget()->GetAbsOrigin() ).LengthSqr();
	if ( flDistToFollowGoalSqr > Square(MAX_DIST_FROM_FOLLOW_TARGET) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Translate base schedules into overridden forms
//-----------------------------------------------------------------------------
int CAI_BehaviorAlyxInjured::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_RUN_FROM_ENEMY:
	case SCHED_RUN_FROM_ENEMY_MOB:
		{
			// Get under cover if we're able to
			if ( ShouldRunToCover() )
				return SCHED_INJURED_RUN_FROM_ENEMY;

			// Run to our follow goal if we're too far away from it
			if ( ShouldRunToFollowGoal() )
				return SCHED_FOLLOW;

			// Cower if surrounded
			if ( HasCondition( COND_INJURED_OVERWHELMED ) )
				return SCHED_INJURED_COWER;

			// Face our enemies
			return SCHED_INJURED_FEAR_FACE;
		}
		break;

	case SCHED_RUN_FROM_ENEMY_FALLBACK:
		return SCHED_INJURED_COWER;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//-----------------------------------------------------------------------------
// Purpose: Pick up failure cases and handle them
//-----------------------------------------------------------------------------
int CAI_BehaviorAlyxInjured::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	// Failed schedules
	switch( failedSchedule )
	{
	case SCHED_RUN_FROM_ENEMY:
	case SCHED_RUN_FROM_ENEMY_MOB:
	case SCHED_FOLLOW:
		return SCHED_INJURED_COWER;
	}

	// Failed tasks
	switch( failedTask )
	{
	case TASK_FIND_COVER_FROM_ENEMY:
	case TASK_FIND_INJURED_COVER_FROM_ENEMY:
		
		// Only cower if we're already near enough to our follow target
		float flDistToFollowTargetSqr = ( GetOuter()->GetAbsOrigin() - GetFollowTarget()->GetAbsOrigin() ).LengthSqr();
		if (  flDistToFollowTargetSqr > Square( 256 ) )
			return SCHED_FOLLOW;

		return SCHED_INJURED_COWER;
		break;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-----------------------------------------------------------------------------
// Purpose: Find the general direction enemies are coming towards us at
//-----------------------------------------------------------------------------
bool CAI_BehaviorAlyxInjured::FindThreatDirection2D( const Vector &vecSource, Vector *vecOut )
{
	// Find the general direction our threat is coming from
	bool bValid = false;
	Vector vecScratch;
	AIEnemiesIter_t	iter;

	// Iterate through all known enemies
	for( AI_EnemyInfo_t *pMemory = GetOuter()->GetEnemies()->GetFirst(&iter); pMemory != NULL; pMemory = GetOuter()->GetEnemies()->GetNext(&iter) )
	{
		if ( pMemory == NULL || pMemory->hEnemy == NULL )
			continue;

		vecScratch = ( vecSource - pMemory->hEnemy->WorldSpaceCenter() );
		VectorNormalize( vecScratch	);
		
		(*vecOut) += vecScratch;
		bValid = true;
	}

	// Find the general direction
	(*vecOut).z = 0.0f;
	VectorNormalize( (*vecOut) );
	return bValid;
}

//-----------------------------------------------------------------------------
// Purpose: Find a position that hides us from our threats while interposing the
//			target entity between us and the threat
// Input  : pTarget - entity to hide behind
//			flRadius - Radius around the target to search
//			*vecOut - position
//-----------------------------------------------------------------------------
bool CAI_BehaviorAlyxInjured::FindCoverFromEnemyBehindTarget( CBaseEntity *pTarget, float flRadius, Vector *vecOut )
{
	if ( pTarget == NULL )
		return false;

	Vector	vecTargetPos = pTarget->GetAbsOrigin();
	Vector	vecThreatDir = vec3_origin;
	
	// Find our threat direction and base our cover on that
	if ( FindThreatDirection2D( vecTargetPos, &vecThreatDir ) )
	{
		// Get a general location for taking cover
		Vector vecTestPos = vecTargetPos + ( vecThreatDir * flRadius );

		if ( g_debug_injured_follow.GetBool() )
		{
			NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecTestPos, 8.0f, 255, 255, 0, 32, true, 2.0f );
		}

		// Make sure we never move towards our threat to get to cover!
		Vector vecMoveDir = GetOuter()->GetAbsOrigin() - vecTestPos;
		VectorNormalize( vecMoveDir );
		float flDotToCover = DotProduct( vecMoveDir, vecThreatDir );
		if ( flDotToCover > 0.0f )
		{
			if ( g_debug_injured_follow.GetBool() )
			{
				NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecTestPos, 8.0f, 255, 0, 0, 32, true, 2.0f );
			}

			return false;
		}

		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit(	NAV_GROUND, 
			GetOuter()->GetAbsOrigin(), 
			vecTestPos, 
			MASK_SOLID_BRUSHONLY, 
			NULL, 
			0, 
			&moveTrace );

		bool bWithinRangeToGoal = ( moveTrace.vEndPosition - vecTestPos ).Length2DSqr() < Square( GetOuter()->GetHullWidth() * 3.0f );
		bool bCanStandAtGoal = GetOuter()->GetMoveProbe()->CheckStandPosition( moveTrace.vEndPosition, MASK_SOLID_BRUSHONLY );

		if ( bWithinRangeToGoal == false || bCanStandAtGoal == false )
		{
			if ( g_debug_injured_follow.GetBool() )
			{
				NDebugOverlay::SweptBox( GetOuter()->GetAbsOrigin(), vecTestPos, GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), vec3_angle, 255, 0, 0, 0, 2.0f );
			}

			return false;
		}

		// Accept it
		*vecOut =  moveTrace.vEndPosition;

		if ( g_debug_injured_follow.GetBool() )
		{
			NDebugOverlay::SweptBox( GetOuter()->GetAbsOrigin(),  (*vecOut), GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), vec3_angle, 0, 255, 0, 0, 2.0f );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_FIND_COVER_FROM_ENEMY:
		{
			CBaseEntity *pLeader = GetFollowTarget();
			if ( !pLeader )
			{
				BaseClass::StartTask( pTask );
				break;
			}

			// Find a position behind our follow target
			Vector coverPos = vec3_invalid;
			if ( FindCoverFromEnemyBehindTarget( pLeader, COVER_DISTANCE, &coverPos ) )
			{
				AI_NavGoal_t goal( GOALTYPE_LOCATION, coverPos, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS );
				GetOuter()->GetNavigator()->SetGoal( goal );		
				GetOuter()->m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
				TaskComplete();
				return;
			}

			// Couldn't find anything
			TaskFail( FAIL_NO_COVER );
			break;
		}

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Whether or not Alyx is injured
//-----------------------------------------------------------------------------
bool CAI_BehaviorAlyxInjured::IsInjured( void ) const 
{ 
	return IsAlyxInInjuredMode();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Always stomp over this
	ClearCondition( COND_INJURED_TOO_FAR_FROM_PLAYER );
	ClearCondition( COND_INJURED_OVERWHELMED );

	// See if we're overwhelmed by foes
	if ( NumKnownEnemiesInRadius( GetOuter()->GetAbsOrigin(), COVER_DISTANCE ) >= MIN_ENEMY_MOB )
	{
		SetCondition( COND_INJURED_OVERWHELMED );
	}

	// Determines whether we consider ourselves in danger
	bool bInDanger = (  HasCondition( COND_LIGHT_DAMAGE ) || 
						HasCondition( COND_HEAVY_DAMAGE ) || 
						HasCondition( COND_INJURED_OVERWHELMED ) );

	// See if we're too far away from the player and in danger
	if ( AI_IsSinglePlayer() && bInDanger )
	{
		bool bWarnPlayer = false;

		// This only works in single-player
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
		if ( pPlayer != NULL )
		{
			// FIXME: This distance may need to be the length of the shortest walked path between the follower and the target

			// Get our approximate distance to the player
			float flDistToPlayer = UTIL_DistApprox2D( GetOuter()->GetAbsOrigin(), pPlayer->GetAbsOrigin() );
			if ( flDistToPlayer > injured_help_plee_range.GetFloat() )
			{
				bWarnPlayer = true;
			}
			else if ( flDistToPlayer > (injured_help_plee_range.GetFloat()*0.5f) && HasCondition( COND_SEE_PLAYER ) == false )
			{
				// Cut our distance in half if we can't see the player
				bWarnPlayer = true;
			}
		}

		// Yell for help!
		if ( bWarnPlayer )
		{
			// FIXME: This should be routed through the normal speaking code with a system to emit from the player's suit.
			
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
			//float flPlayerDistSqr = ( GetOuter()->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).LengthSqr();

			// If the player is too far away or we can't see him
			//if ( HasCondition( COND_SEE_PLAYER ) == false || flPlayerDistSqr > Square( 128 ) )
			{
				if ( m_flNextWarnTime < gpGlobals->curtime )
				{
					pPlayer->EmitSound( "npc_alyx.injured_too_far" );
					m_flNextWarnTime = gpGlobals->curtime + random->RandomFloat( 3.0f, 5.0f );
				}
			}
			/*
			else
			{
				SpeakIfAllowed( TLK_INJURED_FOLLOW_TOO_FAR );
				m_flNextWarnTime = gpGlobals->curtime + random->RandomFloat( 3.0f, 5.0f );
			}
			*/

			SetCondition( COND_INJURED_TOO_FAR_FROM_PLAYER );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Speak a concept if we're able to
//-----------------------------------------------------------------------------
void CAI_BehaviorAlyxInjured::SpeakIfAllowed( AIConcept_t concept )
{
	CAI_Expresser *pExpresser = GetOuter()->GetExpresser();
	if ( pExpresser == NULL )
		return;

	// Must be able to speak the concept
	if ( pExpresser->CanSpeakConcept( concept ) )
	{
		pExpresser->Speak( concept );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of known enemies within a radius to a point
//-----------------------------------------------------------------------------
int CAI_BehaviorAlyxInjured::NumKnownEnemiesInRadius( const Vector &vecSource, float flRadius )
{
	int	nNumEnemies = 0;
	float flRadiusSqr = Square( flRadius );

	AIEnemiesIter_t	iter;

	// Iterate through all known enemies
	for( AI_EnemyInfo_t *pMemory = GetEnemies()->GetFirst(&iter); pMemory != NULL; pMemory = GetEnemies()->GetNext(&iter) )
	{
		if ( pMemory == NULL || pMemory->hEnemy == NULL )
			continue;

		// Must hate or fear them
		if ( GetOuter()->IRelationType( pMemory->hEnemy ) != D_HT && GetOuter()->IRelationType( pMemory->hEnemy ) != D_FR )
			continue;

		// Count only the enemies I've seen recently
		if ( gpGlobals->curtime - pMemory->timeLastSeen > 0.5f )
			continue;

		// Must be within the radius we've specified
		float flEnemyDistSqr = ( vecSource - pMemory->hEnemy->GetAbsOrigin() ).Length2DSqr();
		if ( flEnemyDistSqr < flRadiusSqr )
		{
			nNumEnemies++;
		}
	}

	return nNumEnemies;
}

// ----------------------------------------------
// Custom AI declarations
// ----------------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_BehaviorAlyxInjured )
{
	DECLARE_ACTIVITY( ACT_GESTURE_INJURED_COWER_FLINCH )
	DECLARE_ACTIVITY( ACT_INJURED_COWER )

	DECLARE_CONDITION( COND_INJURED_TOO_FAR_FROM_PLAYER )
	DECLARE_CONDITION( COND_INJURED_OVERWHELMED )

	DECLARE_TASK( TASK_FIND_INJURED_COVER_FROM_ENEMY )

	DEFINE_SCHEDULE
	(
		SCHED_INJURED_COWER,

		"	Tasks"
		// TOOD: Announce cower
		"		TASK_PLAY_SEQUENCE	ACTIVITY:ACT_COWER"
		"		TASK_WAIT			2"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_PLAYER_PUSHING"
	)

	DEFINE_SCHEDULE
	(
		SCHED_INJURED_FEAR_FACE,

		"	Tasks"
		"		 TASK_STOP_MOVING			0"
		"		 TASK_SET_ACTIVITY			ACTIVITY:ACT_IDLE" // FIXME: Scared idle?
		"		 TASK_FACE_ENEMY			0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_PLAYER_PUSHING"
	);

	DEFINE_SCHEDULE
	(
		SCHED_INJURED_RUN_FROM_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_INJURED_COWER"
		"		TASK_STOP_MOVING				0"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		""
		"	Interrupts"
	);

	AI_END_CUSTOM_SCHEDULE_PROVIDER()
}

//-----------------------------------------------------------------------------
// CAI_InjuredFollowGoal
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CAI_InjuredFollowGoal )
END_DATADESC()

LINK_ENTITY_TO_CLASS( ai_goal_injured_follow, CAI_InjuredFollowGoal );

//-------------------------------------

void CAI_InjuredFollowGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_BehaviorAlyxInjured *pBehavior;
	if ( !pAI->GetBehavior( &pBehavior ) )
		return;

	if ( GetGoalEntity() == NULL )
		return;
	
	pBehavior->SetFollowGoal( this );
}

//-------------------------------------

void CAI_InjuredFollowGoal::DisableGoal( CAI_BaseNPC *pAI  )
{ 
	CAI_BehaviorAlyxInjured *pBehavior;
	if ( !pAI->GetBehavior( &pBehavior ) )
		return;

	pBehavior->ClearFollowGoal( this );
}
