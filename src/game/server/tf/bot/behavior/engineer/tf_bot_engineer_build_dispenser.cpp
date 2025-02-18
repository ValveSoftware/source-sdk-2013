//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_dispenser.cpp
// Engineer building his Dispenser near his Sentry
// Michael Booth, May 2010

#include "cbase.h"
#include "nav_mesh.h"
#include "tf_player.h"
#include "tf_obj.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"
#include "tf_gamerules.h"
#include "tf_weapon_builder.h"
#include "bot/tf_bot.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_dispenser.h"
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/tf_bot_get_ammo.h"


extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildDispenser::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_placementTriesLeft = 3;
	return Continue();
}


//---------------------------------------------------------------------------------------------
class PressFireButtonIfValidBuildPositionReply : public INextBotReply
{
public:
	PressFireButtonIfValidBuildPositionReply( void )
	{
		m_builder = NULL;
	}
		
	void SetBuilder( CTFWeaponBuilder *builder )
	{
		m_builder = builder;
	}

	// invoked when process completed successfully
	virtual void OnSuccess( INextBot *bot )
	{
		if ( m_builder != NULL && m_builder->IsValidPlacement() )
		{
			INextBotPlayerInput *playerInput = dynamic_cast< INextBotPlayerInput * >( bot->GetEntity() );
			if ( playerInput )
			{
				playerInput->PressFireButton();
			}
		}
	}

	CTFWeaponBuilder *m_builder;
};


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildDispenser::Update( CTFBot *me, float interval )
{
	if ( me->GetTimeSinceLastInjury() < 1.0f )
	{
		return Done( "Ouch! I'm under attack" );
	}

	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	if ( !mySentry )
	{
		return Done( "No Sentry" );
	}

	if ( mySentry->GetTimeSinceLastInjury() < 1.0f || mySentry->GetHealth() < mySentry->GetMaxHealth() )
	{
		return Done( "Need to repair my Sentry" );
	}

	CObjectDispenser *myDispenser = (CObjectDispenser *)me->GetObjectOfType( OBJ_DISPENSER );
	if ( myDispenser )
	{
		return Done( "Dispenser built" );
	}

	if ( m_placementTriesLeft <= 0 )
	{
		return Done( "Can't find a place to build a Dispenser" );
	}

	if ( me->CanBuild( OBJ_DISPENSER ) == CB_NEED_RESOURCES )
	{
		if ( m_getAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			// need more metal - get some
			m_getAmmoTimer.Start( 1.0f );
			return SuspendFor( new CTFBotGetAmmo, "Need more metal to build" );
		}
/*
		else
		{
			// work on my sentry while I wait for ammo to show up
			me->GetBodyInterface()->AimHeadTowards( mySentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Work on sentry while I wait for ammo to show up" );
			me->PressFireButton();
			return Continue();
		}
*/
	}


/*
	// if my sentry is under attack, forgo building a dispenser - focus on keeping the sentry alive
	if ( mySentry->GetTimeSinceLastInjury() < 1.0f )
	{
		CBaseCombatWeapon *wrench = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
		if ( wrench )
		{
			me->Weapon_Switch( wrench );
		}

		me->GetBodyInterface()->AimHeadTowards( mySentry->WorldSpaceCenter(), IBody::CRITICAL, 1.0f, NULL, "Focusing on keeping my besieged sentry alive" );
		me->PressFireButton();

		return Continue();
	}
*/


	// move behind the Sentry (our chosen build location)
	Vector buildSpot = mySentry->GetAbsOrigin() - 75.0f * mySentry->BodyDirection2D();

	// the ground might be steeply sloped (ie: stairs), so find the actual ground
	buildSpot.z += HumanHeight;
	TheNavMesh->GetSimpleGroundHeight( buildSpot, &buildSpot.z );

	if ( me->IsDistanceBetweenLessThan( buildSpot, 100.0f ) )
	{
		// crouch as we get close so we slow down and hit our mark
		me->PressCrouchButton();
	}

	// if too far away from our build location, move closer
	if ( me->IsDistanceBetweenGreaterThan( buildSpot, 25.0f ) )
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, buildSpot, cost );
		}

		m_path.Update( me );

		return Continue();
	}

	// we're at our build spot behind our sentry now - build a Dispenser
	CTFWeaponBuilder *builder = dynamic_cast< CTFWeaponBuilder * >( me->GetActiveTFWeapon() );
	if ( !builder || builder->GetType() != OBJ_DISPENSER || builder->m_hObjectBeingBuilt == NULL )
	{
		// at home position, build the object
		me->StartBuildingObjectOfType( OBJ_DISPENSER );
	}
	else if ( m_searchTimer.IsElapsed() )
	{
		// rotate to find valid spot
		Vector toSentry = mySentry->GetAbsOrigin() - me->GetAbsOrigin();
		toSentry.NormalizeInPlace();

		Vector forward = -toSentry;

		float angle = RandomFloat( -M_PI/2.0f, M_PI/2.0f );
		float s, c;
		FastSinCos( angle, &s, &c );

		forward.x = toSentry.x * c - toSentry.y * s;
		forward.y = toSentry.x * s + toSentry.y * c;
		forward.z = 0.0f;

		static PressFireButtonIfValidBuildPositionReply buildReply;

		buildReply.SetBuilder( builder );
		me->GetBodyInterface()->AimHeadTowards( me->EyePosition() - 100.0f * forward, IBody::CRITICAL, 1.0f, &buildReply, "Trying to place my dispenser" );

		m_searchTimer.Start( 1.0f );

		--m_placementTriesLeft;
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotEngineerBuildDispenser::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->GetBodyInterface()->ClearPendingAimReply();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEngineerBuildDispenser::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	me->GetBodyInterface()->ClearPendingAimReply();

	return Continue();
}

