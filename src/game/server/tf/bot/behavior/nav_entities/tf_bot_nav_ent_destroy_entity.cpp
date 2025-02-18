//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_nav_ent_destroy_entity.h
// Destroy the given entity, under nav entity control
// Michael Booth, September 2009

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_destroy_entity.h"

extern ConVar tf_bot_path_lookahead_range;

//---------------------------------------------------------------------------------------------
CTFBotNavEntDestroyEntity::CTFBotNavEntDestroyEntity( const CFuncNavPrerequisite *prereq )
{
	m_prereq = prereq;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotNavEntDestroyEntity::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed before we started" );
	}

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_wasIgnoringEnemies = me->HasAttribute( CTFBot::IGNORE_ENEMIES );

	m_isReadyToLaunchSticky = true;

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotNavEntDestroyEntity::DetonateStickiesWhenSet( CTFBot *me, CTFPipebombLauncher *stickyLauncher ) const
{
	if ( !stickyLauncher )
		return;

	if ( stickyLauncher->GetPipeBombCount() >= 8 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
		// stickies laid - detonate them once they are on the ground
		const CUtlVector< CHandle< CTFGrenadePipebombProjectile > > &pipeVector = stickyLauncher->GetPipeBombVector();

		int i;
		for( i=0; i<pipeVector.Count(); ++i )
		{
			if ( pipeVector[i].Get() && !pipeVector[i]->m_bTouched )
			{
				break;
			}
		}

		if ( i == pipeVector.Count() )
		{
			// stickies are on the ground
			me->PressAltFireButton();
		}
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotNavEntDestroyEntity::Update( CTFBot *me, float interval )
{
	if ( m_prereq == NULL )
	{
		return Done( "Prerequisite has been removed" );
	}

	CBaseEntity *target = m_prereq->GetTaskEntity();
	if ( target == NULL )
	{
		return Done( "Target entity is NULL" );
	}

	float attackRange = me->GetMaxAttackRange();
		
	if ( m_prereq->GetTaskValue() > 0.0f ) 
	{
		attackRange = MIN( attackRange, m_prereq->GetTaskValue() );
	}

	if ( me->IsDistanceBetweenLessThan( target, attackRange ) && me->GetVisionInterface()->IsLineOfSightClearToEntity( target ) )
	{
		me->SetAttribute( CTFBot::IGNORE_ENEMIES );

		me->GetBodyInterface()->AimHeadTowards( target->WorldSpaceCenter(), IBody::CRITICAL, 0.2f, NULL, "Aiming at target we need to destroy to progress" );

		if ( me->GetBodyInterface()->IsHeadAimingOnTarget() )
		{
			// attack
			if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
			{
				// demomen use stickybombs to destroy the barrier
				CTFWeaponBase *myCurrentWeapon = me->m_Shared.GetActiveTFWeapon();
				CTFPipebombLauncher *stickyLauncher = dynamic_cast< CTFPipebombLauncher * >( me->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY ) );

				if ( myCurrentWeapon && myCurrentWeapon->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER )
				{
					me->Weapon_Switch( stickyLauncher );
				}

				if ( m_isReadyToLaunchSticky )
				{
					me->PressFireButton();
				}

				m_isReadyToLaunchSticky = !m_isReadyToLaunchSticky;

				DetonateStickiesWhenSet( me, stickyLauncher );

				return Continue();
			}

			me->EquipBestWeaponForThreat( NULL );
			me->PressFireButton();
		}

		return Continue();
	}


	if ( !m_wasIgnoringEnemies )
	{
		me->ClearAttribute( CTFBot::IGNORE_ENEMIES );
	}

	// move into view of our target
	if ( m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, target->GetAbsOrigin(), cost );
	}

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotNavEntDestroyEntity::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	if ( !m_wasIgnoringEnemies )
	{
		me->ClearAttribute( CTFBot::IGNORE_ENEMIES );
	}
}
