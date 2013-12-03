//========= Copyright Valve Corporation, All rights reserved. ============//
//
// To give an NPC the ability to shoot while moving:
//
// - In the NPC's Spawn function, add:
//		CapabilitiesAdd( bits_CAP_MOVE_SHOOT );
//
// - The NPC must either have a weapon (return non-NULL from GetActiveWeapon)
//	  or must have bits_CAP_INNATE_RANGE_ATTACK1 or bits_CAP_INNATE_RANGE_ATTACK2.
//
// - Support the activities ACT_WALK_AIM and/or ACT_RUN_AIM in the NPC.
//
// - Support the activity ACT_GESTURE_RANGE_ATTACK1 as a gesture that plays
//	 over ACT_WALK_AIM and ACT_RUN_AIM.
//
//=============================================================================

#include "cbase.h"

#include "ai_moveshoot.h"
#include "ai_basenpc.h"
#include "ai_navigator.h"
#include "ai_memory.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_MoveAndShootOverlay )
	DEFINE_FIELD( m_bMovingAndShooting, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNoShootWhileMove, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_initialDelay, FIELD_FLOAT ),
	DEFINE_FIELD( m_flSuspendUntilTime, FIELD_TIME ),
END_DATADESC()

#define MOVESHOOT_DO_NOT_SUSPEND	-1.0f

//-------------------------------------

CAI_MoveAndShootOverlay::CAI_MoveAndShootOverlay() : m_bMovingAndShooting(false), m_initialDelay(0)
{
	m_flSuspendUntilTime = MOVESHOOT_DO_NOT_SUSPEND;
	m_bNoShootWhileMove = false;
}

//-------------------------------------

void CAI_MoveAndShootOverlay::NoShootWhileMove()
{
	m_bNoShootWhileMove = true;
}

//-------------------------------------

bool CAI_MoveAndShootOverlay::HasAvailableRangeAttack()
{
	return ( ( GetOuter()->GetActiveWeapon() != NULL ) ||
			( GetOuter()->CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK1 ) ||
			( GetOuter()->CapabilitiesGet() & bits_CAP_INNATE_RANGE_ATTACK2 ) );
}

//-------------------------------------

void CAI_MoveAndShootOverlay::StartShootWhileMove()
{
	if ( GetOuter()->GetState() == NPC_STATE_SCRIPT || 
		 !HasAvailableRangeAttack() ||
		 !GetOuter()->HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_WALK_AIM ) ) ||
		 !GetOuter()->HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_RUN_AIM ) ) )
	{
		NoShootWhileMove();
		return;
	}
	
	GetOuter()->GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + m_initialDelay );
	m_bNoShootWhileMove = false;
}

//-------------------------------------

bool CAI_MoveAndShootOverlay::CanAimAtEnemy()
{
	CAI_BaseNPC *pOuter = GetOuter();
	bool result = false;
	bool resetConditions = false;
	CAI_ScheduleBits savedConditions;

	if ( !GetOuter()->ConditionsGathered() )
	{
		savedConditions = GetOuter()->AccessConditionBits();
		GetOuter()->GatherEnemyConditions( GetOuter()->GetEnemy() );
	}
	
	if ( pOuter->HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		result = true;
	}
	else if ( !pOuter->HasCondition( COND_ENEMY_DEAD ) &&
			  !pOuter->HasCondition( COND_TOO_FAR_TO_ATTACK ) &&
			  !pOuter->HasCondition( COND_ENEMY_TOO_FAR ) &&
			  !pOuter->HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		result = true;
	}

	// If we don't have a weapon, stop
	// This catches NPCs who holster their weapons while running
	if ( !HasAvailableRangeAttack() )
	{
		result = false;
	}

	if ( resetConditions )
	{
		GetOuter()->AccessConditionBits() = savedConditions;
	}
		
	return result;
}

//-------------------------------------

void CAI_MoveAndShootOverlay::UpdateMoveShootActivity( bool bMoveAimAtEnemy )
{
	// FIXME: should be able to query that transition/state is happening
	// FIXME: needs to not try to shoot if the movement type isn't understood
	Activity curActivity = GetOuter()->GetNavigator()->GetMovementActivity();
	Activity newActivity = curActivity;

	if (bMoveAimAtEnemy)
	{
		switch( curActivity )
		{
		case ACT_WALK:
			newActivity = ACT_WALK_AIM;
			break;
		case ACT_RUN:
			newActivity = ACT_RUN_AIM;
			break;
		}
	}
	else
	{
		switch( curActivity )
		{
		case ACT_WALK_AIM:
			newActivity = ACT_WALK;
			break;
		case ACT_RUN_AIM:
			newActivity = ACT_RUN;
			break;
		}
	}

	if ( curActivity != newActivity )
	{
		// Transitioning, wait a bit
		GetOuter()->GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + 0.3f );
		GetOuter()->GetNavigator()->SetMovementActivity( newActivity );
	}
}

//-------------------------------------

void CAI_MoveAndShootOverlay::RunShootWhileMove()
{
	if ( m_bNoShootWhileMove )
		return;

	if ( gpGlobals->curtime < m_flSuspendUntilTime )
		return;

	m_flSuspendUntilTime = MOVESHOOT_DO_NOT_SUSPEND;

	CAI_BaseNPC *pOuter = GetOuter();

	// keep enemy if dead but try to look for a new one
	if (!pOuter->GetEnemy() || !pOuter->GetEnemy()->IsAlive())
	{
		CBaseEntity *pNewEnemy = pOuter->BestEnemy();

		if( pNewEnemy != NULL )
		{
			//New enemy! Clear the timers and set conditions.
			pOuter->SetEnemy( pNewEnemy );
			pOuter->SetState( NPC_STATE_COMBAT );
		}
		else
		{
			pOuter->ClearAttackConditions();
		}
		// SetEnemy( NULL );
	}

	if( !pOuter->GetNavigator()->IsGoalActive() )
		return;

	if ( GetEnemy() == NULL )
	{
		if ( pOuter->GetAlternateMoveShootTarget() )
		{
			// Aim at this other thing if I can't aim at my enemy.
			pOuter->AddFacingTarget( pOuter->GetAlternateMoveShootTarget(), pOuter->GetAlternateMoveShootTarget()->GetAbsOrigin(), 1.0, 0.2 );
		}

		return;
	}

	bool bMoveAimAtEnemy = CanAimAtEnemy();
	UpdateMoveShootActivity( bMoveAimAtEnemy );
	if ( !bMoveAimAtEnemy )
	{
		EndShootWhileMove();
		return;
	}

	Assert( HasAvailableRangeAttack() ); // This should have been caught at task start

	Activity activity;
	bool bIsReloading = false;

	if ( ( activity = pOuter->TranslateActivity( ACT_GESTURE_RELOAD ) ) != ACT_INVALID )
	{
		bIsReloading = pOuter->IsPlayingGesture( activity );
	}

	if ( !bIsReloading && HasAvailableRangeAttack() )
	{
		// time to fire?
		if ( pOuter->HasCondition( COND_CAN_RANGE_ATTACK1, false ) )
		{
			if ( pOuter->GetShotRegulator()->IsInRestInterval() )
			{
				EndShootWhileMove();
			}
			else if ( pOuter->GetShotRegulator()->ShouldShoot() )
			{
				if ( m_bMovingAndShooting || pOuter->OnBeginMoveAndShoot() )
				{
					m_bMovingAndShooting = true;
					pOuter->OnRangeAttack1();

					activity = pOuter->TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 );
					Assert( activity != ACT_INVALID );

					pOuter->RestartGesture( activity );

					// FIXME: this seems a bit wacked
					pOuter->Weapon_SetActivity( pOuter->Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );
				}
			}
		}
		else if ( pOuter->HasCondition( COND_NO_PRIMARY_AMMO, false ) )
		{
			if ( pOuter->GetNavigator()->GetPathTimeToGoal() > 1.0 )
			{
				activity = pOuter->TranslateActivity( ACT_GESTURE_RELOAD );
				if ( activity != ACT_INVALID && GetOuter()->HaveSequenceForActivity( activity ) )
					pOuter->AddGesture( activity );
			}
		}
	}

	// try to keep facing towards the last known position of the enemy
	Vector vecEnemyLKP = pOuter->GetEnemyLKP();
	pOuter->AddFacingTarget( pOuter->GetEnemy(), vecEnemyLKP, 1.0, 0.8 );
}


//-------------------------------------

void CAI_MoveAndShootOverlay::EndShootWhileMove()
{
	if ( m_bMovingAndShooting )
	{
		// Reset the shot regulator so that we always start the next motion with a new burst
		if ( !GetOuter()->GetShotRegulator()->IsInRestInterval() )
		{
			GetOuter()->GetShotRegulator()->Reset( false );
		}

		m_bMovingAndShooting = false;
		GetOuter()->OnEndMoveAndShoot();
	}
}

//-------------------------------------

void CAI_MoveAndShootOverlay::SuspendMoveAndShoot( float flDuration )
{
	EndShootWhileMove();
	m_flSuspendUntilTime = gpGlobals->curtime + flDuration;
}

//-------------------------------------

void CAI_MoveAndShootOverlay::SetInitialDelay( float delay )
{
	m_initialDelay = delay;
}

//-----------------------------------------------------------------------------
