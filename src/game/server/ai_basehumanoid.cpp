//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "BasePropDoor.h"

#include "ai_basehumanoid.h"
#include "ai_blended_movement.h"
#include "ai_navigator.h"
#include "ai_memory.h"

#ifdef HL2_DLL
#include "ai_interactions.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose:  This is a generic function (to be implemented by sub-classes) to
//			 handle specific interactions between different types of characters
//			 (For example the barnacle grabbing an NPC)
// Input  :  Constant for the type of interaction
// Output :	 true  - if sub-class has a response for the interaction
//			 false - if sub-class has no response
//-----------------------------------------------------------------------------
bool CAI_BaseHumanoid::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
#ifdef HL2_DLL
	// Annoying to ifdef this out. Copy it into all the HL2 specific humanoid NPC's instead?
	if ( interactionType == g_interactionBarnacleVictimDangle )
	{
		// Force choosing of a new schedule
		ClearSchedule( "Grabbed by a barnacle" );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		// Destroy the entity, the barnacle is going to use the ragdoll that it is releasing
		// as the corpse.
		UTIL_Remove( this );
		return true;
	}
#endif
	return BaseClass::HandleInteraction( interactionType, data, sourceEnt);
}


//-----------------------------------------------------------------------------
// Purpose: check ammo
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::CheckAmmo( void )
{
	BaseClass::CheckAmmo();

	// FIXME: put into GatherConditions()?
	// FIXME: why isn't this a baseclass function?
	if (!GetActiveWeapon())
		return;

	// Don't do this while holstering / unholstering
	if ( IsWeaponStateChanging() )
		return;

	if (GetActiveWeapon()->UsesPrimaryAmmo())
	{
		if (!GetActiveWeapon()->HasPrimaryAmmo() )
		{
			SetCondition(COND_NO_PRIMARY_AMMO);
		}
		else if (GetActiveWeapon()->UsesClipsForAmmo1() && GetActiveWeapon()->Clip1() < (GetActiveWeapon()->GetMaxClip1() / 4 + 1))
		{
			// don't check for low ammo if you're near the max range of the weapon
			SetCondition(COND_LOW_PRIMARY_AMMO);
		}
	}

	if (!GetActiveWeapon()->HasSecondaryAmmo() )
	{
		if ( GetActiveWeapon()->UsesClipsForAmmo2() )
		{
			SetCondition(COND_NO_SECONDARY_AMMO);
		}
	}
}


//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::BuildScheduleTestBits( )
{
	BaseClass::BuildScheduleTestBits();

	if ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR )
	{
		if ( GetShotRegulator()->IsInRestInterval() )
		{
			ClearCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static bool IsSmall( CBaseEntity *pBlocker )
{
	CCollisionProperty *pCollisionProp = pBlocker->CollisionProp();
	int  nSmaller = 0;
	Vector vecSize = pCollisionProp->OBBMaxs() - pCollisionProp->OBBMins();
	for ( int i = 0; i < 3; i++ )
	{
		if ( vecSize[i] >= 42 )
			return false;

		if ( vecSize[i] <= 30 )
		{
			nSmaller++;
		}
	}

	return ( nSmaller >= 2 );
}

bool CAI_BaseHumanoid::OnMoveBlocked( AIMoveResult_t *pResult )
{
	if ( *pResult != AIMR_BLOCKED_NPC && GetNavigator()->GetBlockingEntity() && !GetNavigator()->GetBlockingEntity()->IsNPC() )
	{
		CBaseEntity *pBlocker = GetNavigator()->GetBlockingEntity();

		float massBonus = ( IsNavigationUrgent() ) ? 40.0 : 0;

		if ( pBlocker->GetMoveType() == MOVETYPE_VPHYSICS && 
			 pBlocker != GetGroundEntity() && 
			 !pBlocker->IsNavIgnored() &&
			 !dynamic_cast<CBasePropDoor *>(pBlocker) &&
			 pBlocker->VPhysicsGetObject() && 
			 pBlocker->VPhysicsGetObject()->IsMoveable() && 
			 ( pBlocker->VPhysicsGetObject()->GetMass() <= 35.0 + massBonus + 0.1 || 
			   ( pBlocker->VPhysicsGetObject()->GetMass() <= 50.0 + massBonus + 0.1 && IsSmall( pBlocker ) ) ) )
		{
			DbgNavMsg1( this, "Setting ignore on object %s", pBlocker->GetDebugName() );
			pBlocker->SetNavIgnore( 2.5 );
		}
#if 0
		else
		{
			CPhysicsProp *pProp = dynamic_cast<CPhysicsProp*>( pBlocker );
			if ( pProp && pProp->GetHealth() && pProp->GetExplosiveDamage() == 0.0 && GetActiveWeapon() && !GetActiveWeapon()->ClassMatches( "weapon_rpg" ) )
			{
				Msg( "!\n" );
				// Destroy!
			}
		}
#endif
	}

	return BaseClass::OnMoveBlocked( pResult );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SNEAK_ATTACK_DIST	360.0f // 30 feet
void CAI_BaseHumanoid::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	bool bSneakAttacked = false;

	if( ptr->hitgroup == HITGROUP_HEAD )
	{
		if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && info.GetAttacker() != GetEnemy() && !IsInAScript() )
		{
			// Shot in the head by a player I've never seen. In this case the player 
			// has gotten the drop on this enemy and such an attack is always lethal (at close range)
			bSneakAttacked = true;

			AIEnemiesIter_t	iter;
			for( AI_EnemyInfo_t *pMemory = GetEnemies()->GetFirst(&iter); pMemory != NULL; pMemory = GetEnemies()->GetNext(&iter) )
			{
				if ( pMemory->hEnemy == info.GetAttacker() )
				{
					bSneakAttacked = false;
					break;
				}
			}

			float flDist;

			flDist = (info.GetAttacker()->GetAbsOrigin() - GetAbsOrigin()).Length();

			if( flDist > SNEAK_ATTACK_DIST )
			{
				bSneakAttacked = false;
			}
		}
	}

	if( bSneakAttacked )
	{
		CTakeDamageInfo newInfo = info;

		newInfo.SetDamage( GetHealth() );
		BaseClass::TraceAttack( newInfo, vecDir, ptr, pAccumulator );
		return;
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}

//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::StartTaskRangeAttack1( const Task_t *pTask )
{
	if ( ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR ) == 0 )
	{
		BaseClass::StartTask( pTask );
		return;
	}

	// Can't shoot if we're in the rest interval; fail the schedule
	if ( GetShotRegulator()->IsInRestInterval() )
	{
		TaskFail( "Shot regulator in rest interval" );
		return;
	}

	if ( GetShotRegulator()->ShouldShoot() )
	{
		OnRangeAttack1();
		ResetIdealActivity( ACT_RANGE_ATTACK1 );
	}
	else
	{
		// This can happen if we start while in the middle of a burst
		// which shouldn't happen, but given the chaotic nature of our AI system,
		// does occasionally happen.
		ResetIdealActivity( ACT_IDLE_ANGRY );
	}
}


//-----------------------------------------------------------------------------
// Starting Tasks
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		StartTaskRangeAttack1( pTask );
		break;

	default:
		BaseClass::StartTask( pTask );
	}
}



//-----------------------------------------------------------------------------
// TASK_RANGE_ATTACK1 / TASK_RANGE_ATTACK2 / etc.
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::RunTaskRangeAttack1( const Task_t *pTask )
{
	if ( ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR ) == 0 )
	{
		BaseClass::RunTask( pTask );
		return;
	}

	AutoMovement( );

	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if( vecEnemyLKP != vec3_origin )
	{
		if ( ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RELOAD ) && 
			 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
			 FInAimCone( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	if ( IsActivityFinished() )
	{
		if ( !GetEnemy() || !GetEnemy()->IsAlive() )
		{
			TaskComplete();
			return;
		}

		if ( !GetShotRegulator()->IsInRestInterval() )
		{
			if ( GetShotRegulator()->ShouldShoot() )
			{
				OnRangeAttack1();
				ResetIdealActivity( ACT_RANGE_ATTACK1 );
			}
			return;
		}
		TaskComplete();
	}
}


//-----------------------------------------------------------------------------
// Running Tasks
//-----------------------------------------------------------------------------
void CAI_BaseHumanoid::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		RunTaskRangeAttack1( pTask );
		break;

	default:
		BaseClass::RunTask( pTask );
	}
}
