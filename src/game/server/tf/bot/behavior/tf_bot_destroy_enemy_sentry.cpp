//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_destroy_enemy_sentry.cpp
// Destroy an enemy sentry gun
// Michael Booth, June 2010

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "tf_weaponbase_gun.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_destroy_enemy_sentry.h"
#include "bot/behavior/tf_bot_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/tf_bot_get_ammo.h"
#include "bot/behavior/demoman/tf_bot_stickybomb_sentrygun.h"

#include "nav_mesh.h"


extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_sticky_base_range;

ConVar tf_bot_debug_destroy_enemy_sentry( "tf_bot_debug_destroy_enemy_sentry", "0", FCVAR_CHEAT );
ConVar tf_bot_max_grenade_launch_at_sentry_range( "tf_bot_max_grenade_launch_at_sentry_range", "1500", FCVAR_CHEAT );
ConVar tf_bot_max_sticky_launch_at_sentry_range( "tf_bot_max_sticky_launch_at_sentry_range", "1500", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
// Search for angle to land grenade near target
bool FindGrenadeAim( CTFBot *me, CBaseEntity *target, float *aimYaw, float *aimPitch )
{
	Vector toTarget = target->WorldSpaceCenter() - me->EyePosition();

	if ( toTarget.IsLengthGreaterThan( tf_bot_max_grenade_launch_at_sentry_range.GetFloat() ) )
	{
		return false;
	}

	QAngle anglesToTarget;
	VectorAngles( toTarget, anglesToTarget );

	// start with current aim, in case we're already on target
	const QAngle &eyeAngles = me->EyeAngles();
	float yaw = eyeAngles.y;
	float pitch = eyeAngles.x;

	const int trials = 10;
	for( int t=0; t<trials; ++t )
	{
		// estimate impact spot
		const float pipebombInitVel = 900.0f;
		Vector impactSpot = me->EstimateProjectileImpactPosition( pitch, yaw, pipebombInitVel );

		// check if impactSpot landed near sentry
		const float explosionRadius = 75.0f;
		if ( ( target->WorldSpaceCenter() - impactSpot ).IsLengthLessThan( explosionRadius ) )
		{
			trace_t trace;
			NextBotTraceFilterIgnoreActors filter( target, COLLISION_GROUP_NONE );

			UTIL_TraceLine( target->WorldSpaceCenter(), impactSpot, MASK_SOLID_BRUSHONLY, &filter, &trace );
			if ( !trace.DidHit() )
			{
				*aimYaw = yaw;
				*aimPitch = pitch;
				return true;
			}
		}

		yaw = anglesToTarget.y + RandomFloat( -30.0f, 30.0f );
		pitch = RandomFloat( -85.0f, 85.0f );
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Search for angle to land sticky near target
bool FindStickybombAim( CTFBot *me, CBaseEntity *target, float *aimYaw, float *aimPitch, float *aimCharge )
{
	Vector toTarget = target->WorldSpaceCenter() - me->EyePosition();

	if ( toTarget.IsLengthGreaterThan( tf_bot_max_sticky_launch_at_sentry_range.GetFloat() ) )
	{
		return false;
	}

	QAngle anglesToTarget;
	VectorAngles( toTarget, anglesToTarget );

	// start with current aim, in case we're already on target
	const QAngle &eyeAngles = me->EyeAngles();

	float yaw = eyeAngles.y;
	float pitch = eyeAngles.x;

	*aimCharge = 1.0f;

	bool hasTarget = false;

	const int trials = 100;
	for( int t=0; t<trials; ++t )
	{
		float charge = 0.0f;
// 		if ( toTarget.IsLengthGreaterThan( tf_bot_sticky_base_range.GetBool() ) )
// 		{
// 			charge = RandomFloat( 0.1f, 1.0f );
// 
// 			// skew towards zero - full charge shots are seldom required
// 			charge *= charge;
// 		}

		// estimate impact spot
		Vector impactSpot = me->EstimateStickybombProjectileImpactPosition( pitch, yaw, charge );

		// check if impactSpot landed near target
		const float explosionRadius = 75.0f;
		if ( ( target->WorldSpaceCenter() - impactSpot ).IsLengthLessThan( explosionRadius ) )
		{
			trace_t trace;
			NextBotTraceFilterIgnoreActors filter( target, COLLISION_GROUP_NONE );

			UTIL_TraceLine( target->WorldSpaceCenter(), impactSpot, MASK_SOLID_BRUSHONLY, &filter, &trace );
			if ( !trace.DidHit() )
			{
				// found target aim - keep one we find with least required
				// charge, because we need to be fast in combat
				if ( charge < (*aimCharge) )
				{
					hasTarget = true;

					*aimCharge = charge;
					*aimYaw = yaw;
					*aimPitch = pitch;

					if ( *aimCharge < 0.01 )
					{
						// as quick as possible - no need to search further
						break;
					}
				}
			}
		}

		yaw = anglesToTarget.y + RandomFloat( -30.0f, 30.0f );
		pitch = RandomFloat( -85.0f, 85.0f );
	}

	return hasTarget;
}





//---------------------------------------------------------------------------------------------
// Return true if this Action has what it needs to perform right now
bool CTFBotDestroyEnemySentry::IsPossible( CTFBot *me )
{
	if ( me->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) ||
		 me->IsPlayerClass( TF_CLASS_SNIPER ) ||
		 me->IsPlayerClass( TF_CLASS_MEDIC ) ||
		 me->IsPlayerClass( TF_CLASS_ENGINEER ) ||
		 me->IsPlayerClass( TF_CLASS_PYRO ) )
	{
		// these classes have no way to kill a sentry at long range
		return false;
	}

	// don't go after a sentry if we're out of ammo
	if ( me->GetAmmoCount( TF_AMMO_PRIMARY ) <= 0 || me->GetAmmoCount( TF_AMMO_SECONDARY ) <= 0 )
	{
		return false;
	}

	// if we're a spy, we have better ways of destroying sentries that shooting at it
	if ( me->IsPlayerClass( TF_CLASS_SPY ) )
	{
		return false;
	}

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( me->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			return false;
		}
	}
#endif

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		if ( me->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
		{
			return false;
		}
	}

	return true;
}


//---------------------------------------------------------------------------------------------
class CFindSafeAttackArea : public ISearchSurroundingAreasFunctor
{
public:
	CFindSafeAttackArea( CTFBot *me )
	{
		m_me = me;
		m_attackSpot = me->GetAbsOrigin();
		m_foundAttackSpot = false;

		CObjectSentrygun *sentry = me->GetEnemySentry();
		if ( sentry )
		{
			sentry->UpdateLastKnownArea();
			m_sentryArea = (CTFNavArea *)sentry->GetLastKnownArea();
		}
		else
		{
			m_sentryArea = NULL;
		}
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
	{
		if ( !m_sentryArea )
		{
			return false;
		}

		if ( area->IsPotentiallyVisible( m_sentryArea ) )
		{
			// try the center first
			m_attackSpot = area->GetCenter();

			const int maxTries = 5;
			for( int i=0; i<maxTries; ++i )
			{
				if ( m_me->IsLineOfFireClear( m_attackSpot + m_me->GetClassEyeHeight(), m_me->GetEnemySentry() ) )
				{
					if ( ( m_attackSpot - m_me->GetEnemySentry()->GetAbsOrigin() ).IsLengthGreaterThan( 1.1f * SENTRY_MAX_RANGE ) )
					{
						// found our attack spot
						m_foundAttackSpot = true;
						return false;
					}
				}

				m_attackSpot = area->GetRandomPoint();
			}
		}

		return true;
	}


	CTFBot *m_me;
	CTFNavArea *m_sentryArea;
	Vector m_attackSpot;
	bool m_foundAttackSpot;

	Vector m_splashFromSpot;
	Vector m_splashToSpot;
	bool m_foundSplashSpot;
};


//---------------------------------------------------------------------------------------------
void CTFBotDestroyEnemySentry::ComputeSafeAttackSpot( CTFBot *me )
{
	m_hasSafeAttackSpot = false;	

	CObjectSentrygun *sentry = me->GetEnemySentry();
	if ( sentry == NULL )
	{
		return;
	}

	sentry->UpdateLastKnownArea();

	CTFNavArea *sentryArea = (CTFNavArea *)sentry->GetLastKnownArea();
	if ( sentryArea == NULL )
	{
		return;
	}

	NavAreaCollector collector( true );
	sentryArea->ForAllPotentiallyVisibleAreas( collector );

	int i;
	CUtlVector< CTFNavArea * > beyondSentryRangeVector;
	for( i=0; i<collector.m_area.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)collector.m_area[i];

		Vector wayOut = ( area->GetCenter() - sentryArea->GetCenter() ) + area->GetCenter();

		Vector farthestFromSentry;
		area->GetClosestPointOnArea( wayOut, &farthestFromSentry ); 

		if ( ( farthestFromSentry - sentry->GetAbsOrigin() ).IsLengthGreaterThan( SENTRY_MAX_RANGE ) )
		{
			// at least some of this area is out of sentry range
			beyondSentryRangeVector.AddToTail( area );

			if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			{
				area->DrawFilled( 0, 255, 0, 255, 60.0f, true, 1.0f );
			}
		}
	}


	CUtlVector< CTFNavArea * > attackSentryVector;
	for( i=0; i<beyondSentryRangeVector.Count(); ++i )
	{
		CTFNavArea *area = beyondSentryRangeVector[i];

		Vector closestToSentry;
		area->GetClosestPointOnArea( sentry->GetAbsOrigin(), &closestToSentry );

		if ( ( closestToSentry - sentry->GetAbsOrigin() ).IsLengthLessThan( 1.5f * SENTRY_MAX_RANGE ) )
		{
			// good attack range
			attackSentryVector.AddToTail( area );

			if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			{
				area->DrawFilled( 100, 255, 0, 255, 60.0f );
			}
		}
	}


	if ( beyondSentryRangeVector.Count() == 0 )
	{
		// no safe areas at all
		m_hasSafeAttackSpot = false;
		return;
	}

	CUtlVector< CTFNavArea * > *safeAreaVector;

	if ( attackSentryVector.Count() == 0 )
	{
		// no good close-in attack areas, choose from farther away set
		safeAreaVector = &beyondSentryRangeVector;
	}
	else
	{
		// for now, just pick a random spot
		safeAreaVector = &attackSentryVector;
	}

	// TODO: find closest and least combat-hot area
	CTFNavArea *safeArea = safeAreaVector->Element( RandomInt( 0, safeAreaVector->Count()-1 ) );

	m_safeAttackSpot = safeArea->GetRandomPoint();
	m_hasSafeAttackSpot = true;

	if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
	{
		safeArea->DrawFilled( 255, 255, 0, 255, 60.0f );
		NDebugOverlay::Cross3D( m_safeAttackSpot, 10.0f, 255, 0, 0, true, 60.0f );
	}
}


//---------------------------------------------------------------------------------------------
class FindSafeSentryApproachAreaScan : public ISearchSurroundingAreasFunctor
{
public:
	FindSafeSentryApproachAreaScan( CTFBot *me )
	{
		m_me = me;

		m_isEscaping = false;

		CTFNavArea *myArea = me->GetLastKnownArea();
		if ( myArea && myArea->IsTFMarked() )
		{
			// I'm standing in a danger area - escape!
			m_isEscaping = true;
		}
	}

	virtual bool operator() ( CNavArea *baseArea, CNavArea *priorArea, float travelDistanceSoFar )
	{
		CTFNavArea *area = (CTFNavArea *)baseArea;

		if ( m_isEscaping )
		{
			if ( !area->IsTFMarked() )
			{
				// found safe area - use it
				m_approachAreaVector.AddToTail( area );
				return false;
			}
		}
		else
		{
			if ( area->IsTFMarked() && priorArea )
			{
				// we just stepped into sentry fire - keep the area one step prior
				m_approachAreaVector.AddToTail( (CTFNavArea *)priorArea );
			}
		}

		return true;
	}

	// return true if 'adjArea' should be included in the ongoing search
	virtual bool ShouldSearch( CNavArea *baseAdjArea, CNavArea *baseCurrentArea, float travelDistanceSoFar ) 
	{
		CTFNavArea *area = (CTFNavArea *)baseCurrentArea;

		if ( !m_isEscaping )
		{
			// don't search beyond sentry danger areas (but step into them)
			if ( area->IsTFMarked() )
			{
				return false;
			}
		}

		return m_me->GetLocomotionInterface()->IsAreaTraversable( baseAdjArea );
	}

	// Invoked after the search has completed
	virtual void PostSearch( void )
	{
		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
		{
			for( int i=0; i<m_approachAreaVector.Count(); ++i )
			{
				m_approachAreaVector[i]->DrawFilled( 0, 255, 0, 255, 60.0f );
			}
		}
	}

	CTFBot *m_me;
	CUtlVector< CTFNavArea * > m_approachAreaVector;
	bool m_isEscaping;
};


//---------------------------------------------------------------------------------------------
void CTFBotDestroyEnemySentry::ComputeCornerAttackSpot( CTFBot *me )
{
	m_safeAttackSpot = vec3_origin;
	m_hasSafeAttackSpot = false;

	CObjectSentrygun *sentry = me->GetEnemySentry();
	if ( !sentry )
	{
		return;
	}
	
	sentry->UpdateLastKnownArea();
	CTFNavArea *sentryArea = (CTFNavArea *)sentry->GetLastKnownArea();

	if ( !sentryArea )
	{
		return;
	}

	// mark all areas this sentry can potentially fire upon
	// need to use completely visible so the partially visible areas are used as corner-fighting spots
	NavAreaCollector sentryDanger;
	sentryArea->ForAllCompletelyVisibleAreas( sentryDanger );

	CTFNavArea::MakeNewTFMarker();
	for( int i=0; i<sentryDanger.m_area.Count(); ++i )
	{
		CTFNavArea *area = (CTFNavArea *)sentryDanger.m_area[i];

		Vector close;
		area->GetClosestPointOnArea( sentry->GetAbsOrigin(), &close );

		if ( ( sentry->GetAbsOrigin() - close ).IsLengthLessThan( SENTRY_MAX_RANGE ) )
		{
			area->TFMark();

			if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
			{
				area->DrawFilled( 255, 0, 0, 255, 60.0f );
			}
		}
	}


	// find nearby area adjacent to area that is in enemy sentry fire field
	FindSafeSentryApproachAreaScan scan( me );
	SearchSurroundingAreas( me->GetLastKnownArea(), scan );

	if ( scan.m_approachAreaVector.Count() > 0 )
	{
		CTFNavArea *safeArea = scan.m_approachAreaVector[ RandomInt( 0, scan.m_approachAreaVector.Count()-1 ) ];

		// try to avoid picking a spot where sentry can attack us
		const int retryCount = 25;
		for( int r=0; r<retryCount; ++r )
		{
			m_safeAttackSpot = safeArea->GetRandomPoint();

			if ( ( sentry->WorldSpaceCenter() - m_safeAttackSpot ).IsLengthGreaterThan( SENTRY_MAX_RANGE ) ||
				 !me->IsLineOfFireClear( sentry->WorldSpaceCenter(), m_safeAttackSpot ) )
			{
				break;
			}
		}

		m_hasSafeAttackSpot = true;

		if ( tf_bot_debug_destroy_enemy_sentry.GetBool() )
		{
			NDebugOverlay::Cross3D( m_safeAttackSpot, 5.0f, 255, 255, 0, true, 60.0f );
		}
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDestroyEnemySentry::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_abandonTimer.Invalidate();

	m_isAttackingSentry = false;
	m_wasUber = false;

/*
	// find a spot to attack the sentry out of its range
	CFindSafeAttackArea find( me );
	SearchSurroundingAreas( me->GetLastKnownArea(), find, 1.5f * SENTRY_MAX_RANGE );

	m_hasSafeAttackSpot = find.m_foundAttackSpot;
	m_safeAttackSpot = find.m_attackSpot;
*/

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		ComputeCornerAttackSpot( me );
	}
	else
	{
		ComputeSafeAttackSpot( me );
	}

/*
	if ( !m_hasSafeAttackSpot )
	{
		return Done( "No safe attack spot found" );
	}
*/

	m_targetSentry = me->GetEnemySentry();

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDestroyEnemySentry::Update( CTFBot *me, float interval )
{
	if ( me->GetEnemySentry() == NULL )
	{
		return Done( "Enemy sentry is destroyed" );
	}

	// if the sentry changes, re-evaluate
	if ( me->GetEnemySentry() != m_targetSentry )
	{
		return ChangeTo( new CTFBotDestroyEnemySentry, "Changed sentry target" );
	}

	if ( me->m_Shared.IsInvulnerable() )
	{
		if ( !m_wasUber )
		{
			m_wasUber = true;

			// we just became uber - are we close enough to rush the sentry?
			const float maxRushDistance = 500.0f;
			CTFBotPathCost cost( me, FASTEST_ROUTE );
			float travelDistance = NavAreaTravelDistance( me->GetLastKnownArea(), 
														  m_targetSentry->GetLastKnownArea(), 
														  cost, maxRushDistance );

			if ( travelDistance >= 0.0f )
			{
				return SuspendFor( new CTFBotUberAttackEnemySentry( m_targetSentry ), "Go get it!" );
			}
		}
	}
	else
	{
		m_wasUber = false;
	}

	if ( !me->HasAttribute( CTFBot::IGNORE_ENEMIES ) )
	{
		const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( threat && threat->IsVisibleInFOVNow() )
		{
			float threatRange = me->GetRangeTo( threat->GetLastKnownPosition() );
			float sentryRange = me->GetRangeTo( me->GetEnemySentry() );

			if ( threatRange < 0.5f * sentryRange )
			{
				return Done( "Enemy near" );
			}
		}
	}

	bool isSentryFiringOnMe = false;
	if ( me->GetEnemySentry()->GetTimeSinceLastFired() < 1.0f )
	{
		Vector sentryForward;
		AngleVectors( me->GetEnemySentry()->GetTurretAngles(), &sentryForward );

		Vector to = me->GetAbsOrigin() - me->GetEnemySentry()->GetAbsOrigin();
		to.NormalizeInPlace();

		if ( DotProduct( to, sentryForward ) > 0.8f )
		{
			isSentryFiringOnMe = true;
		}
	}


	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		// a demoman wants to get close to the sentry but just out of range or line of sight so
		// he can pepper the area with stickies and destroy it
		Vector attackSpot = m_hasSafeAttackSpot ? m_safeAttackSpot : m_targetSentry->GetAbsOrigin();

		// move into position
		if ( !m_path.IsValid() || m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( 1.0f );

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_path.Compute( me, attackSpot, cost );
		}

		float aimPitch, aimYaw, aimCharge;
		if ( isSentryFiringOnMe )
		{
			// the sentry is firing on me - might as well shoot back!
			me->EquipLongRangeWeapon();
			me->PressFireButton();
		}
		else if ( FindStickybombAim( me, m_targetSentry, &aimYaw, &aimPitch, &aimCharge ) )
		{
			// found an opportunistic spot to sticky the sentry from
			return ChangeTo( new CTFBotStickybombSentrygun( me->GetEnemySentry(), aimYaw, aimPitch, aimCharge ), "Destroying sentry with opportunistic sticky shot" );
		}

		// move towards sentry
		if ( m_canMove )
		{
			m_path.Update( me );
		}

		if ( ( me->IsRangeLessThan( attackSpot, 50.0f ) &&
			 ( me->GetAbsOrigin() - attackSpot ).AsVector2D().IsLengthLessThan( 25.0f ) ) ||
			 ( me->IsLineOfFireClear( me->GetEnemySentry() ) && me->IsRangeLessThan( m_targetSentry, 1000.0f ) ) )	// opportunistic shot
		{
			// reached attack spot
			return ChangeTo( new CTFBotStickybombSentrygun( me->GetEnemySentry() ), "Destroying sentry with stickies" );
		}

		if ( me->IsRangeLessThan( attackSpot, 200.0f ) )
		{
#ifdef TF_CREEP_MODE
			if ( m_creepTimer.IsElapsed() )
			{
				m_canMove = !m_canMove;

				if ( m_canMove )
				{
					m_creepTimer.Start( 0.1f );
				}
				else
				{
					m_creepTimer.Start( RandomFloat( 0.2f, 0.5f ) );
				}
			}
#endif
		}
		else
		{
			m_canMove = true;
		}

		return Continue();
	}


	bool isInAttackPosition = ( m_hasSafeAttackSpot && me->IsRangeLessThan( m_safeAttackSpot, 20.0f ) );

	if ( isInAttackPosition )
	{
		if ( !m_abandonTimer.HasStarted() )
		{
			m_abandonTimer.Start( 2.f );
		}

		if ( me->IsLineOfFireClear( me->GetEnemySentry() ) )
		{
			// must look at sentry entity to make use of SelectTargetPoint() 
			me->GetBodyInterface()->AimHeadTowards( me->GetEnemySentry(), IBody::MANDATORY, 1.0f, NULL, "Aiming at enemy sentry" );

			// because sentries are stationary, check if XY is on target to allow SelectTargetPoint() to adjust Z for grenades
			Vector toSentry = me->GetEnemySentry()->WorldSpaceCenter() - me->EyePosition();
			toSentry.NormalizeInPlace();
			Vector forward;
			me->EyeVectors( &forward );

			if ( ( forward.x * toSentry.x + forward.y * toSentry.y ) > 0.95f )
			{
				if ( me->EquipLongRangeWeapon() == false )
				{
					return SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );
				}

				me->PressFireButton();
				m_isAttackingSentry = true;
				m_abandonTimer.Invalidate();
			}
			else
			{
				m_isAttackingSentry = false;
			}

			if ( me->IsRangeGreaterThan( me->GetEnemySentry(), 1.1f * SENTRY_MAX_RANGE ) )
			{
				// safely out of range of the gun - hold here and fire at it
				return Continue();
			}

			// we are in range of the gun - if it is pointed at us and firing, retreat to cover
			if ( me->GetEnemySentry()->GetTimeSinceLastFired() < 1.0f )
			{
				Vector sentryForward;
				AngleVectors( me->GetEnemySentry()->GetTurretAngles(), &sentryForward );

				Vector to = me->GetAbsOrigin() - me->GetEnemySentry()->GetAbsOrigin();
				to.NormalizeInPlace();

				if ( DotProduct( to, sentryForward ) > 0.8f )
				{
					return SuspendFor( new CTFBotRetreatToCover( 0.1f ), "Taking cover from sentry fire" );
				}
			}

			if ( isInAttackPosition )
			{
				// we're at our attack position, hold here
				return Continue();
			}
		}
	}

	// move into position
	if ( !m_path.IsValid() || m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		CTFBotPathCost cost( me, SAFEST_ROUTE );
		Vector moveGoal = m_hasSafeAttackSpot ? m_safeAttackSpot : me->GetEnemySentry()->GetAbsOrigin();

		if ( !m_path.Compute( me, moveGoal, cost ) )
		{
			return Done( "No path" );
		}
	}

	// move along path to vantage point
	m_path.Update( me );

	if ( m_abandonTimer.IsElapsed() )
	{
		return Done( "Can't attack sentry - giving up!" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotDestroyEnemySentry::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_abandonTimer.Invalidate();

	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		ComputeCornerAttackSpot( me );
	}
	else
	{
		ComputeSafeAttackSpot( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotDestroyEnemySentry::ShouldHurry( const INextBot *me ) const
{
	// while killing a sentry we're "hurrying" so we don't dodge
	return m_isAttackingSentry ? ANSWER_YES : ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotDestroyEnemySentry::ShouldRetreat( const INextBot *me ) const
{
	// push in to kill the sentry
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotDestroyEnemySentry::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	// if we're in range to attack the sentry, we handle firing directly
	return m_isAttackingSentry ? ANSWER_NO : ANSWER_UNDEFINED;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFBotUberAttackEnemySentry::CTFBotUberAttackEnemySentry( CObjectSentrygun *sentryTarget )
{
	m_targetSentry = sentryTarget;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotUberAttackEnemySentry::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_wasIgnoringEnemies = me->HasAttribute( CTFBot::IGNORE_ENEMIES );

	me->SetAttribute( CTFBot::IGNORE_ENEMIES );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotUberAttackEnemySentry::Update( CTFBot *me, float interval )
{
	if ( !me->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		return Done( "No longer uber" );
	}

	if ( m_targetSentry == NULL )
	{
		return Done( "Target sentry destroyed" );
	}

	float aimYaw, aimPitch;
	if ( me->IsPlayerClass( TF_CLASS_DEMOMAN ) && FindGrenadeAim( me, m_targetSentry, &aimYaw, &aimPitch ) )
	{
		QAngle aimAngles;
		aimAngles.x = aimPitch;
		aimAngles.y = aimYaw;
		aimAngles.z = 0.0f;

		Vector aimForward;
		AngleVectors( aimAngles, &aimForward );

		// always recompute eye aim target so we can update our view
		Vector eyeAimTarget = me->EyePosition() + 5000.0f * aimForward;
		me->GetBodyInterface()->AimHeadTowards( eyeAimTarget, IBody::CRITICAL, 0.3f, NULL, "Aiming at opportunistic grenade shot" );

		Vector eyeForward;
		me->EyeVectors( &eyeForward );

		if ( DotProduct( aimForward, eyeForward ) > 0.9f )
		{
			if ( me->EquipLongRangeWeapon() == false )
			{
				return SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );
			}

			me->PressFireButton();
		}
	}
	else if ( me->IsLineOfFireClear( m_targetSentry ) )
	{
		// must look at sentry entity to make use of SelectTargetPoint() 
		me->GetBodyInterface()->AimHeadTowards( m_targetSentry, IBody::MANDATORY, 1.0f, NULL, "Aiming at target sentry" );

		// because sentries are stationary, check if XY is on target to allow SelectTargetPoint() to adjust Z for grenades
		Vector toSentry = m_targetSentry->WorldSpaceCenter() - me->EyePosition();
		toSentry.NormalizeInPlace();

		Vector eyeForward;
		me->EyeVectors( &eyeForward );

		if ( ( eyeForward.x * toSentry.x + eyeForward.y * toSentry.y ) > 0.95f )
		{
			if ( me->EquipLongRangeWeapon() == false )
			{
				return SuspendFor( new CTFBotRetreatToCover( 0.1f ), "No suitable range weapon available right now" );
			}

			me->PressFireButton();
		}

		if ( me->IsRangeLessThan( m_targetSentry, 100.0f ) )
		{
			// we have a clear line of fire and are close enough
			return Continue();
		}
	}

	// move into position
	if ( !m_path.IsValid() || m_repathTimer.IsElapsed() )
	{
		m_repathTimer.Start( 1.0f );

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, m_targetSentry->WorldSpaceCenter(), cost );
	}

	m_path.Update( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotUberAttackEnemySentry::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	if ( !m_wasIgnoringEnemies )
	{
		me->ClearAttribute( CTFBot::IGNORE_ENEMIES );
	}
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotUberAttackEnemySentry::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotUberAttackEnemySentry::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotUberAttackEnemySentry::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_YES;
}
