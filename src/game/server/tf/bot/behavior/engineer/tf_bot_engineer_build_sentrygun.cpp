//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_engineer_build_sentrygun.cpp
// Engineer building his Sentry gun
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
#include "bot/behavior/engineer/tf_bot_engineer_move_to_build.h"
#include "bot/behavior/engineer/tf_bot_engineer_build_sentrygun.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/map_entities/tf_bot_hint_sentrygun.h"

extern ConVar tf_bot_path_lookahead_range;


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuildSentryGun::CTFBotEngineerBuildSentryGun( void )
{
	m_sentryBuildHint = NULL;
}


//---------------------------------------------------------------------------------------------
CTFBotEngineerBuildSentryGun::CTFBotEngineerBuildSentryGun( CTFBotHintSentrygun *sentryBuildHint )
{
	m_sentryBuildHint = sentryBuildHint;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildSentryGun::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_sentryTriesLeft = 5;
	m_giveUpTimer.Invalidate();

	m_searchTimer.Invalidate();
	m_wanderWay = 1;
	m_needToAimSentry = true;

	m_sentryBuildLocation = ( m_sentryBuildHint == NULL ) ? me->GetAbsOrigin() : m_sentryBuildHint->GetAbsOrigin();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEngineerBuildSentryGun::Update( CTFBot *me, float interval )
{
	if ( me->GetTimeSinceLastInjury() < 1.0f )
	{
		return Done( "Ouch! I'm under attack" );
	}

	CObjectSentrygun *mySentry = (CObjectSentrygun *)me->GetObjectOfType( OBJ_SENTRYGUN );
	if ( mySentry )
	{
		return Done( "Sentry built" );
	}

	// collect metal as we move to our build location
	if ( me->CanBuild( OBJ_SENTRYGUN ) == CB_NEED_RESOURCES )
	{
		if ( m_getAmmoTimer.IsElapsed() && CTFBotGetAmmo::IsPossible( me ) )
		{
			// need more metal - get some
			m_getAmmoTimer.Start( 1.0f );
			return SuspendFor( new CTFBotGetAmmo, "Need more metal to build my Sentry" );
		}
	}

	// various interruptions could mean we're away from our build location - move to it
	if ( me->IsRangeGreaterThan( m_sentryBuildLocation, 25.0f ) )
	{
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Compute( me, m_sentryBuildLocation, cost );
		}

		m_path.Update( me );

		if ( !m_path.IsValid() )
		{
			return Done( "Path failed" );
		}

		return Continue();
	}

	// we are at our build location
	if ( m_sentryTriesLeft <= 0 )
	{
		// couldn't build here
		return Done( "Couldn't find a place to build" );
	}

	// attempt to build a Sentry here
	if ( m_sentryBuildHint != NULL )
	{
		// directly create a sentry gun at the precise position and orientation desired
		CObjectSentrygun *mySentry = (CObjectSentrygun *)CreateEntityByName( "obj_sentrygun" );
		if ( mySentry )
		{
			m_sentryBuildHint->IncrementUseCount();

			mySentry->SetAbsOrigin( m_sentryBuildHint->GetAbsOrigin() );
			mySentry->SetAbsAngles( QAngle( 0, m_sentryBuildHint->GetAbsAngles().y, 0 ) );
			mySentry->Spawn();

			mySentry->StartPlacement( me );
			mySentry->StartBuilding( me );
		}
	}
	else
	{
		// no precise build location - go through the normal build process

		CTFWeaponBuilder *builder = dynamic_cast< CTFWeaponBuilder * >( me->GetActiveTFWeapon() );
		if ( !builder || builder->GetType() != OBJ_SENTRYGUN || builder->m_hObjectBeingBuilt == NULL )
		{
			// at home position, build a sentry (switch to the sentry builder "gun")
			me->StartBuildingObjectOfType( OBJ_SENTRYGUN );
			return Continue();
		}

		// orient sentry towards where enemies enter this region
		if ( m_needToAimSentry )
		{
			CTFNavArea *myArea = (CTFNavArea *)me->GetLastKnownArea();
			if ( myArea )
			{
				CUtlVector< CTFNavArea * > invasionVector;
				myArea->GetEnemyInvasionAreaVector( me->GetTeamNumber() );

				if ( invasionVector.Count() > 0 )
				{
					// orient sentry towards where enemies enter this region
					int which = RandomInt( 0, invasionVector.Count()-1 );
					me->GetBodyInterface()->AimHeadTowards( invasionVector[ which ]->GetCenter(), IBody::CRITICAL, 1.0f, NULL, "Sentry build orientation" );
					m_needToAimSentry = false;
				}
			}
		}

		if ( me->GetBodyInterface()->IsHeadSteady() )
		{
			if ( builder->IsValidPlacement() )
			{
				// build the sentry
				me->PressFireButton();
			}
			else
			{
				// move around a bit to find valid spot
				if ( m_searchTimer.IsElapsed() )
				{
					m_wanderWay = RandomInt( 0, 3 );
					m_needToAimSentry = true;
					m_searchTimer.Start( RandomFloat( 0.1f, 0.25f ) );
					--m_sentryTriesLeft;
				}

				switch( m_wanderWay )
				{
				case 0:	me->PressForwardButton();	break;
				case 1:	me->PressBackwardButton();	break;
				case 2: me->PressRightButton();		break;
				case 3: me->PressLeftButton();		break;
				}
			}
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEngineerBuildSentryGun::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();

	return Continue();
}
