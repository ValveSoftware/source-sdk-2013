//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_behavior_functank.h"
#include "ai_navigator.h"
#include "ai_memory.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// How long to fire a func tank before running schedule selection again.
#define FUNCTANK_FIRE_TIME	5.0f

BEGIN_DATADESC( CAI_FuncTankBehavior )
	DEFINE_FIELD( m_hFuncTank, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bMounted, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flBusyTime, FIELD_TIME ),
	DEFINE_FIELD( m_bSpottedPlayerOutOfCover, FIELD_BOOLEAN ),
END_DATADESC();

//-----------------------------------------------------------------------------
// Purpose: Constructor
//--k---------------------------------------------------------------------------
CAI_FuncTankBehavior::CAI_FuncTankBehavior()
{
	m_hFuncTank = NULL;
	m_bMounted = false;
	m_flBusyTime = 0.0f;
	m_bSpottedPlayerOutOfCover = false;
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
CAI_FuncTankBehavior::~CAI_FuncTankBehavior()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CAI_FuncTankBehavior::CanSelectSchedule()
{
	// If we don't have a func_tank do not bother with conditions, schedules, etc.
	if ( !m_hFuncTank )
		return false;

	// Are you alive, in a script?
	if ( !GetOuter()->IsInterruptable() )
		return false;
	
	// Commander is giving you orders?
	if ( GetOuter()->HasCondition( COND_RECEIVED_ORDERS ) )
		return false;
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::BeginScheduleSelection()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::EndScheduleSelection()
{
	if ( m_bMounted )
	{
		Dismount();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::PrescheduleThink()
{
	BaseClass::PrescheduleThink();

	if ( !HasCondition(COND_SEE_PLAYER) )
	{
		m_bSpottedPlayerOutOfCover = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int	CAI_FuncTankBehavior::SelectSchedule()
{
	// This shouldn't get called with an m_hFuncTank, see CanSelectSchedule.
	Assert( m_hFuncTank );

	// If we've been told to dismount, or we are out of ammo - dismount.
	if ( HasCondition( COND_FUNCTANK_DISMOUNT ) || m_hFuncTank->GetAmmoCount() == 0 )
	{
		if ( m_bMounted )
		{
			Dismount();
		}

		return BaseClass::SelectSchedule();
	}

	// If we are not mounted to a func_tank look for one.
	if ( !IsMounted() )
	{
		return SCHED_MOVE_TO_FUNCTANK;
	}

	// If we have an enemy, it's in the viewcone & we have LOS to it
	if ( GetEnemy() )
	{
		// Tell the func tank whenever we see the player for the first time since not seeing him for a while
		if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy()->IsPlayer() && !m_bSpottedPlayerOutOfCover )
		{
			m_bSpottedPlayerOutOfCover = true;
			m_hFuncTank->NPC_JustSawPlayer( GetEnemy() );
		}

		// Fire at the enemy.
		return SCHED_FIRE_FUNCTANK;
	}
	else
	{
		// Scan for enemies.
		return SCHED_SCAN_WITH_FUNCTANK;
	}

	return SCHED_IDLE_STAND;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CAI_FuncTankBehavior::NPC_TranslateActivity( Activity activity )
{
	// If I'm on the gun, I play the idle manned gun animation
	if ( m_bMounted ) 
		return ACT_IDLE_MANNEDGUN;

	return BaseClass::NPC_TranslateActivity( activity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::Dismount( void )
{
	SetBusy( gpGlobals->curtime + AI_FUNCTANK_BEHAVIOR_BUSYTIME );

	Assert( m_hFuncTank );

	if ( m_hFuncTank )
	{
		GetOuter()->SpeakSentence( FUNCTANK_SENTENCE_DISMOUNTING );

		Assert( m_hFuncTank->IsMarkedForDeletion() || m_hFuncTank->GetController() == GetOuter() );
		
		m_hFuncTank->NPC_SetInRoute( false );
		if ( m_hFuncTank->GetController() == GetOuter() )
			m_hFuncTank->StopControl();
		SetFuncTank( NULL );
	}

	GetOuter()->SetDesiredWeaponState( DESIREDWEAPONSTATE_UNHOLSTERED );

	m_bMounted = false;

	// Set this condition to force breakout of any func_tank behavior schedules
	SetCondition( COND_FUNCTANK_DISMOUNT );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_FuncTankBehavior::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	int iResult = BaseClass::OnTakeDamage_Alive( info );
	if ( !iResult )
		return 0;

	// If we've been hit by the player, and the player's not targetable 
	// by our func_tank, get off the tank.
	CBaseEntity *pAttacker = info.GetAttacker();
	bool bValidDismountAttacker = (pAttacker && pAttacker->IsPlayer());

#ifdef HL2_EPISODIC 
	bValidDismountAttacker = true;
#endif

	if ( m_hFuncTank && bValidDismountAttacker == true )
	{
		if ( !m_hFuncTank->IsEntityInViewCone( pAttacker ) )
		{
			SetCondition( COND_FUNCTANK_DISMOUNT );
		}
	}

	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FUNCTANK_ANNOUNCE_SCAN:
		{
			if ( random->RandomInt( 0, 3 ) == 0 )
			{
				GetOuter()->SpeakSentence( FUNCTANK_SENTENCE_SCAN_FOR_ENEMIES );
			}
			TaskComplete();
		}
		break;

	case TASK_GET_PATH_TO_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET  );
				return;
			}

			Vector vecManPos;
			m_hFuncTank->NPC_FindManPoint( vecManPos );
			AI_NavGoal_t goal( vecManPos );
			goal.pTarget = m_hFuncTank;
			if ( GetNavigator()->SetGoal( goal ) )
			{
				GetNavigator()->SetArrivalDirection( m_hFuncTank->GetAbsAngles() );
				TaskComplete();
			}
			else
			{
				TaskFail("NO PATH");

				// Don't try and use me again for a while
				SetBusy( gpGlobals->curtime + AI_FUNCTANK_BEHAVIOR_BUSYTIME );
			}
			break;
		}		
	case TASK_FACE_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}
			
			// Ensure we've reached the func_tank
			Vector vecManPos;
			m_hFuncTank->NPC_FindManPoint( vecManPos );

			// More leniency in Z.
			Vector vecDelta = (vecManPos - GetAbsOrigin());
			if ( fabs(vecDelta.x) > 16 || fabs(vecDelta.y) > 16 || fabs(vecDelta.z) > 48 )
			{
				TaskFail( "Not correctly on func_tank man point" );
				m_hFuncTank->NPC_InterruptRoute();
				return;
			}

			GetMotor()->SetIdealYawToTarget( m_hFuncTank->GetAbsOrigin() );
			GetOuter()->SetTurnActivity(); 
			break;
		}

	case TASK_HOLSTER_WEAPON:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			if ( GetOuter()->IsWeaponHolstered() || !GetOuter()->CanHolsterWeapon() )
			{
				GetOuter()->SpeakSentence( FUNCTANK_SENTENCE_JUST_MOUNTED );

				// We are at the correct position and facing for the func_tank, mount it.
				m_hFuncTank->StartControl( GetOuter() );
				GetOuter()->ClearEnemyMemory();
				m_bMounted = true;
				TaskComplete();

				GetOuter()->SetIdealActivity( ACT_IDLE_MANNEDGUN );
			}
			else
			{
				GetOuter()->SetDesiredWeaponState( DESIREDWEAPONSTATE_HOLSTERED );
			}
			break;
		}

	case TASK_FIRE_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}
			GetOuter()->m_flWaitFinished = gpGlobals->curtime + FUNCTANK_FIRE_TIME;
			break;
		}
	case TASK_SCAN_LEFT_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			GetMotor()->SetIdealYawToTarget( m_hFuncTank->GetAbsOrigin() );

			float flCenterYaw = m_hFuncTank->YawCenterWorld();
			float flYawRange = m_hFuncTank->YawRange();
			float flScanAmount = random->RandomFloat( 0, flYawRange );
			QAngle vecTargetAngles( 0, UTIL_AngleMod( flCenterYaw + flScanAmount ), 0 );

			/*
			float flCenterPitch = m_hFuncTank->YawCenterWorld();
			float flPitchRange = m_hFuncTank->PitchRange();
			float flPitch = random->RandomFloat( -flPitchRange, flPitchRange );
			QAngle vecTargetAngles( flCenterPitch + flPitch, UTIL_AngleMod( flCenterYaw + flScanAmount ), 0 );
			*/

			Vector vecTargetForward;
			AngleVectors( vecTargetAngles, &vecTargetForward );
			Vector vecTarget = GetOuter()->EyePosition() + (vecTargetForward * 256);
			GetOuter()->AddLookTarget( vecTarget, 1.0, 2.0, 0.2 );

			m_hFuncTank->NPC_SetIdleAngle( vecTarget );

			break;
		}
	case TASK_SCAN_RIGHT_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}

			GetMotor()->SetIdealYawToTarget( m_hFuncTank->GetAbsOrigin() );

			float flCenterYaw = m_hFuncTank->YawCenterWorld();
			float flYawRange = m_hFuncTank->YawRange();
			float flScanAmount = random->RandomFloat( 0, flYawRange );
			QAngle vecTargetAngles( 0, UTIL_AngleMod( flCenterYaw - flScanAmount ), 0 );

			/*
			float flCenterPitch = m_hFuncTank->YawCenterWorld();
			float flPitchRange = m_hFuncTank->PitchRange();
			float flPitch = random->RandomFloat( -flPitchRange, flPitchRange );
			QAngle vecTargetAngles( flCenterPitch + flPitch, UTIL_AngleMod( flCenterYaw - flScanAmount ), 0 );
			*/

			Vector vecTargetForward;
			AngleVectors( vecTargetAngles, &vecTargetForward );
			Vector vecTarget = GetOuter()->EyePosition() + (vecTargetForward * 256);
			GetOuter()->AddLookTarget( vecTarget, 1.0, 2.0, 0.2 );

			m_hFuncTank->NPC_SetIdleAngle( vecTarget );

			break;
		}
	case TASK_FORGET_ABOUT_FUNCTANK:
		{
			if ( !m_hFuncTank )
			{
				TaskFail( FAIL_NO_TARGET );
				return;
			}
			break;
		}
	default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FACE_FUNCTANK:
		{
			Assert( m_hFuncTank );

			GetMotor()->UpdateYaw();

			if ( GetOuter()->FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_HOLSTER_WEAPON:
		{
			Assert( m_hFuncTank );

			if ( GetOuter()->IsWeaponHolstered() )
			{
				GetOuter()->SpeakSentence( FUNCTANK_SENTENCE_JUST_MOUNTED );

				// We are at the correct position and facing for the func_tank, mount it.
				m_hFuncTank->StartControl( GetOuter() );
				GetOuter()->ClearEnemyMemory();
				m_bMounted = true;
				TaskComplete();

				GetOuter()->SetIdealActivity( ACT_IDLE_MANNEDGUN );
			}

			break;
		}
	case TASK_FIRE_FUNCTANK:
		{
			Assert( m_hFuncTank );

			if( GetOuter()->m_flWaitFinished < gpGlobals->curtime )
			{
				TaskComplete();
			}

			if ( m_hFuncTank->NPC_HasEnemy() )
			{
				GetOuter()->SetLastAttackTime( gpGlobals->curtime );
				m_hFuncTank->NPC_Fire();

				// The NPC may have decided to stop using the func_tank, because it's out of ammo.
				if ( !m_hFuncTank )
				{
					TaskComplete();
					break;
				}
			}
			else
			{
				TaskComplete();
			}
			
			Assert( m_hFuncTank );

			if ( m_hFuncTank->GetAmmoCount() == 0 )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_SCAN_LEFT_FUNCTANK:
	case TASK_SCAN_RIGHT_FUNCTANK:
		{
			GetMotor()->UpdateYaw();
			if ( GetOuter()->FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_FORGET_ABOUT_FUNCTANK:
		{
			m_hFuncTank->NPC_InterruptRoute();
			SetBusy( gpGlobals->curtime + AI_FUNCTANK_BEHAVIOR_BUSYTIME );
			TaskComplete();
			break;
		}
	default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_hFuncTank )
	{
		Dismount();
	}
	Assert( !m_hFuncTank );

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::UpdateOnRemove( void )
{
	if ( m_hFuncTank )
	{
		Dismount();
	}

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::SetFuncTank( CHandle<CFuncTank> hFuncTank )			
{ 
	if ( m_hFuncTank && !hFuncTank )
	{
		SetBusy( gpGlobals->curtime + AI_FUNCTANK_BEHAVIOR_BUSYTIME );
		SetCondition( COND_FUNCTANK_DISMOUNT );
	}

	m_hFuncTank = hFuncTank; 
	GetOuter()->ClearSchedule( "Setting a new func_tank" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::AimGun( void )
{
	if ( m_bMounted && m_hFuncTank)
	{
		Vector vecForward;
		AngleVectors( m_hFuncTank->GetAbsAngles(), &vecForward );
		GetOuter()->SetAim( vecForward );
		return;
	}

	BaseClass::AimGun();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_FuncTankBehavior::GatherConditions()
{
	BaseClass::GatherConditions();

	// Since we can't pathfind, if we can't see the enemy, he's eluded us
	// So we deliberately ignore unreachability 
	if ( GetEnemy() && !HasCondition(COND_SEE_ENEMY) )
	{
		if ( gpGlobals->curtime - GetOuter()->GetEnemyLastTimeSeen() >= 3.0f )
		{
			GetOuter()->MarkEnemyAsEluded();
		}
	}

	if ( !m_hFuncTank )
	{
		m_bMounted = false;
		GetOuter()->SetDesiredWeaponState( DESIREDWEAPONSTATE_UNHOLSTERED );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseEntity *CAI_FuncTankBehavior::BestEnemy( void )
{
	// Only use this BestEnemy call when we are on the manned gun.
	if ( !m_hFuncTank ||!IsMounted() )
		return BaseClass::BestEnemy();

	CBaseEntity *pBestEnemy	= NULL;
	int	iBestDistSq	= MAX_COORD_RANGE * MAX_COORD_RANGE;	// so first visible entity will become the closest.
	int	iBestPriority = -1000;
	bool bBestUnreachable = false;							// Forces initial check
	bool bBestSeen = false;
	bool bUnreachable = false;
	int	iDistSq;

	AIEnemiesIter_t iter;

	// Get the current npc for checking from.
	CAI_BaseNPC *pNPC = GetOuter();
	if ( !pNPC )
		return NULL;

	for( AI_EnemyInfo_t *pEMemory = GetEnemies()->GetFirst( &iter ); pEMemory != NULL; pEMemory = GetEnemies()->GetNext( &iter ) )
	{
		CBaseEntity *pEnemy = pEMemory->hEnemy;
		if ( !pEnemy || !pEnemy->IsAlive() )
			continue;
		
		// UNDONE: Move relationship checks into IsValidEnemy?
		if ( ( pEnemy->GetFlags() & FL_NOTARGET ) || 
			 ( pNPC->IRelationType( pEnemy ) != D_HT && pNPC->IRelationType( pEnemy ) != D_FR ) ||
			 !IsValidEnemy( pEnemy ) )
			continue;

		if ( pEMemory->timeLastSeen < pNPC->GetAcceptableTimeSeenEnemy() )
			continue;

		if ( pEMemory->timeValidEnemy > gpGlobals->curtime )
			continue;

		// Skip enemies that have eluded me to prevent infinite loops
		if ( GetEnemies()->HasEludedMe( pEnemy ) )
			continue;

		// Establish the reachability of this enemy
		bUnreachable = pNPC->IsUnreachable( pEnemy );

		// Check view cone of the view tank here.
		bUnreachable = !m_hFuncTank->IsEntityInViewCone( pEnemy );
		if ( !bUnreachable )
		{
			// It's in the viewcone. Now make sure we have LOS to it.
			bUnreachable = !m_hFuncTank->HasLOSTo( pEnemy );
		}

		// If best is reachable and current is unreachable, skip the unreachable enemy regardless of priority
		if ( !bBestUnreachable && bUnreachable )
			continue;

		//  If best is unreachable and current is reachable, always pick the current regardless of priority
		if ( bBestUnreachable && !bUnreachable )
		{
			bBestSeen = ( pNPC->GetSenses()->DidSeeEntity( pEnemy ) || pNPC->FVisible( pEnemy ) ); // @TODO (toml 04-02-03): Need to optimize CanSeeEntity() so multiple calls in frame do not recalculate, rather cache
			iBestPriority = pNPC->IRelationPriority( pEnemy );
			iBestDistSq = (pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
			pBestEnemy = pEnemy;
			bBestUnreachable = bUnreachable;
		}
		// If both are unreachable or both are reachable, chose enemy based on priority and distance
		else if ( pNPC->IRelationPriority( pEnemy ) > iBestPriority )
		{
			// this entity is disliked MORE than the entity that we
			// currently think is the best visible enemy. No need to do
			// a distance check, just get mad at this one for now.
			iBestPriority = pNPC->IRelationPriority ( pEnemy );
			iBestDistSq = ( pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
			pBestEnemy = pEnemy;
			bBestUnreachable = bUnreachable;
		}
		else if ( pNPC->IRelationPriority( pEnemy ) == iBestPriority )
		{
			// this entity is disliked just as much as the entity that
			// we currently think is the best visible enemy, so we only
			// get mad at it if it is closer.
			iDistSq = ( pEnemy->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();

			bool bCloser = ( iDistSq < iBestDistSq ) ;

			if ( bCloser || !bBestSeen )
			{
				// @TODO (toml 04-02-03): Need to optimize FVisible() so multiple calls in frame do not recalculate, rather cache
				bool fSeen = ( pNPC->GetSenses()->DidSeeEntity( pEnemy ) || pNPC->FVisible( pEnemy ) );
				if ( ( bCloser && ( fSeen || !bBestSeen ) ) || ( !bCloser && !bBestSeen && fSeen ) )
				{
					bBestSeen = fSeen;
					iBestDistSq = iDistSq;
					iBestPriority = pNPC->IRelationPriority( pEnemy );
					pBestEnemy = pEnemy;
					bBestUnreachable = bUnreachable;
				}
			}
		}
	}
	return pBestEnemy;
}

//=============================================================================
//
// Custom AI schedule data
//

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_FuncTankBehavior )

	DECLARE_TASK( TASK_GET_PATH_TO_FUNCTANK )
	DECLARE_TASK( TASK_FACE_FUNCTANK )
	DECLARE_TASK( TASK_HOLSTER_WEAPON )
	DECLARE_TASK( TASK_FIRE_FUNCTANK )
	DECLARE_TASK( TASK_SCAN_LEFT_FUNCTANK )
	DECLARE_TASK( TASK_SCAN_RIGHT_FUNCTANK )
	DECLARE_TASK( TASK_FORGET_ABOUT_FUNCTANK )
	DECLARE_TASK( TASK_FUNCTANK_ANNOUNCE_SCAN )

	DECLARE_CONDITION( COND_FUNCTANK_DISMOUNT )

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_MOVE_TO_FUNCTANK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE: SCHED_FAIL_MOVE_TO_FUNCTANK"
		"		TASK_GET_PATH_TO_FUNCTANK	0"
		"		TASK_SPEAK_SENTENCE			1000"	// FUNCTANK_SENTENCE_MOVE_TO_MOUNT
		"		TASK_RUN_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_FUNCTANK			0"
		"		TASK_HOLSTER_WEAPON			0"
		"	"
		"	Interrupts"
		"		COND_FUNCTANK_DISMOUNT"
	)

	//=========================================================
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FIRE_FUNCTANK,

		"	Tasks"
		"		TASK_ANNOUNCE_ATTACK	1"	// 1 = primary attack
		"       TASK_FIRE_FUNCTANK      0"
		"   "
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
		"		COND_ENEMY_OCCLUDED"
		"		COND_WEAPON_BLOCKED_BY_FRIEND"
		"		COND_WEAPON_SIGHT_OCCLUDED"
		"		COND_FUNCTANK_DISMOUNT"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_SCAN_WITH_FUNCTANK,

		"	Tasks"
		"		TASK_FUNCTANK_ANNOUNCE_SCAN	0"
		"		TASK_STOP_MOVING			0"
		"		TASK_WAIT					4"
		"		TASK_SCAN_LEFT_FUNCTANK		0"
		"		TASK_WAIT					4"
		"		TASK_SCAN_RIGHT_FUNCTANK	0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_PROVOKED"
		"		COND_FUNCTANK_DISMOUNT"
	)

	DEFINE_SCHEDULE 
	(
		SCHED_FAIL_MOVE_TO_FUNCTANK,

		"	Tasks"
		"		TASK_FORGET_ABOUT_FUNCTANK		0"
		""
		"	Interrupts"
	)	

AI_END_CUSTOM_SCHEDULE_PROVIDER()
