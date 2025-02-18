//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_escort_squad_leader.cpp
// Escort the squad leader to their destination
// Michael Booth, Octoboer 2011

#include "cbase.h"

#include "bot/tf_bot.h"
#include "bot/behavior/squad/tf_bot_escort_squad_leader.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_fetch_flag.h"

ConVar tf_bot_squad_escort_range( "tf_bot_squad_escort_range", "500", FCVAR_CHEAT );
ConVar tf_bot_formation_debug( "tf_bot_formation_debug", "0", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CTFBotEscortSquadLeader::CTFBotEscortSquadLeader( Action< CTFBot > *actionToDoAfterSquadDisbands ) // : m_path( ChasePath::LEAD_SUBJECT )
{
	m_actionToDoAfterSquadDisbands = actionToDoAfterSquadDisbands;
	m_formationPath.SetGoalTolerance( 0.0f );
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotEscortSquadLeader::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_formationForward = vec3_origin;

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotEscortSquadLeader::Update( CTFBot *me, float interval )
{
	if ( interval <= 0.0f )
	{
		return Continue();
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	CTFBotSquad *squad = me->GetSquad();
	if ( !squad )
	{
		if ( m_actionToDoAfterSquadDisbands )
		{
			return ChangeTo( m_actionToDoAfterSquadDisbands, "Not in a Squad" );
		}

		return Done( "Not in a Squad" );
	}

	// we need to update every tick to smoothly move in formation
	me->FlagForUpdate();

	CTFBot *leader = squad->GetLeader();
	if ( !leader || !leader->IsAlive() )
	{
		me->LeaveSquad();

		if ( m_actionToDoAfterSquadDisbands )
		{
			return ChangeTo( m_actionToDoAfterSquadDisbands, "Squad leader is dead" );
		}

		return Done( "Squad leader is dead" );
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && leader == me )
	{
		const char* pszNowLeader = "I'm now the squad leader! Going for the flag!";
		if ( me->HasAttribute( CTFBot::AGGRESSIVE ) )
		{
			// push for the point first, then attack
			return ChangeTo( new CTFBotPushToCapturePoint( new CTFBotFetchFlag ), pszNowLeader );
		}

		// capture the flag
		return ChangeTo( new CTFBotFetchFlag, pszNowLeader );
	}

	// if we're using a melee weapon, close and attack with it while staying near the leader
	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
	if ( myWeapon && myWeapon->IsMeleeWeapon() )
	{
		if ( me->IsRangeLessThan( leader, tf_bot_squad_escort_range.GetFloat() ) && me->IsLineOfSightClear( leader ) )
		{
			ActionResult< CTFBot > result = m_meleeAttackAction.Update( me, interval );

			if ( result.IsContinue() )
			{
				// we have a melee target, and we're still reasonably close to the flag leader
				return Continue();
			}
		}
	}

	CUtlVector< CTFBot * > rawMemberVector;
	squad->CollectMembers( &rawMemberVector );

	// cull out the medics - they do their own thing
	CUtlVector< CTFBot * > memberVector;
	for( int m=0; m<rawMemberVector.Count(); ++m )
	{
		if ( !rawMemberVector[m]->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			memberVector.AddToTail( rawMemberVector[m] );
		}
	}

	const PathFollower *leaderPath = leader->GetCurrentPath();
	if ( !leaderPath || !leaderPath->GetCurrentGoal() )
	{
		// no path, no formation
		me->SetSquadFormationError( 0.0f );
		me->SetBrokenFormation( false );
		return Continue();
	}

	const Path::Segment *leaderSegment = leaderPath->GetCurrentGoal();

	Vector leaderForward = leaderSegment->pos - leader->GetAbsOrigin();

	// if the leader is very close to the goal, use the next goal to ensure 
	// the forward vector stays forward
	const float atGoal = 25.0f;
	if ( leaderForward.IsLengthLessThan( atGoal ) )
	{
		const Path::Segment *nextSegment = leaderPath->NextSegment( leaderSegment );
		if ( nextSegment )
		{
			leaderForward = nextSegment->pos - leader->GetAbsOrigin();
		}
	}

	leaderForward.NormalizeInPlace();

	if ( m_formationForward.IsZero() )
	{
		m_formationForward = leaderForward;
	}
	else
	{
		// limit rate of change of leader forward vector to keep formation coherent
		float maxRotation = 30.0f;	// degrees/second

		float leaderForwardYaw = UTIL_VecToYaw( leaderForward );
		float formationYaw = UTIL_VecToYaw( m_formationForward );

		float angleDiff = UTIL_AngleDiff( leaderForwardYaw, formationYaw );

		float deltaYaw = maxRotation * interval;

		if ( angleDiff < -deltaYaw )
		{
			formationYaw -= deltaYaw;
		}
		else if ( angleDiff > deltaYaw )
		{
			formationYaw += deltaYaw;
		}
		else
		{
			formationYaw += angleDiff;
		}

		FastSinCos( formationYaw * M_PI / 180.0f, &m_formationForward.y, &m_formationForward.x );
		m_formationForward.z = 0.0f;
	}


	const float maxSeparationAngle = 30.0f * M_PI / 180.0f;
	
	float formationRadius = 125.0f;
	if ( squad->GetFormationSize() > 0.0f )
	{
		formationRadius = squad->GetFormationSize();
	}

	Vector myFormationSpot;
	Vector formationForward = vec3_origin;
	float s, c;

	// where am I in the roster
	int which;
	for( which=0; which<memberVector.Count(); ++which )
	{
		if ( me->IsSelf( memberVector[which] ) )
		{
			break;
		}
	}

	// subtract one since the leader is always first
	--which;

	// my formation spot is assigned via my position in the roster array
	int slot = ( which + 1 ) /2;

	float formationAngle = slot * maxSeparationAngle;

	if ( which & 0x1 )
	{
		formationAngle = -formationAngle;
	}

	FastSinCos( formationAngle, &s, &c );
	formationForward.x = m_formationForward.x * c - m_formationForward.y * s;
	formationForward.y = m_formationForward.y * c + m_formationForward.x * s;

	myFormationSpot = leader->GetAbsOrigin() + formationRadius * formationForward;

	trace_t result;
	CTraceFilterIgnoreTeammates filter( me, COLLISION_GROUP_NONE, me->GetTeamNumber() );
	UTIL_TraceLine( leader->GetAbsOrigin() + Vector( 0, 0, HalfHumanHeight ), myFormationSpot + Vector( 0, 0, HalfHumanHeight ), MASK_PLAYERSOLID, &filter, &result );

	if ( result.DidHitWorld() )
	{
		myFormationSpot = result.endpos - Vector( 0, 0, HalfHumanHeight ) + 0.6f * me->GetBodyInterface()->GetHullWidth() * result.plane.normal;
	}


	if ( tf_bot_formation_debug.GetBool() )
	{
		NDebugOverlay::Circle( myFormationSpot, 16.0f, 0, 255, 0, 255, true, 0.1f );

		CFmtStr msg;
		NDebugOverlay::Text( myFormationSpot, msg.sprintf( "%d", which ), false, 0.1f );
	}

	// match speed with leader if I'm at/near my formation position
	Vector to = myFormationSpot - me->GetAbsOrigin();
	float error = to.Length2D();
	const float maxError = 100.0f;	// 50

	float normalizedError = 1.0f;
	if ( error < maxError )
	{
		normalizedError = error / maxError;
	}

	// this error term is used in CTFPlayer::TeamFortress_CalculateMaxSpeed() to 
	// modulate our speed
	// 0 = in position (no error)
	// 1 = far out of position (max error)
	me->SetSquadFormationError( normalizedError );
	
	// move to my formation spot
	if ( error < 50.0f )
	{
		// if we're ahead of where we want to be, just wait
		if ( DotProduct( to, formationForward ) > 0.0f )
		{
			// very close - just directly approach to avoid pathing jaggies
			me->GetLocomotionInterface()->Approach( myFormationSpot );
		}
		else
		{
			// we're in position
			me->SetSquadFormationError( 0.0f );
		}
	}
	else
	{
		if ( m_pathTimer.IsElapsed() )
		{
			m_pathTimer.Start( RandomFloat( 0.1f, 0.2f ) );

			me->SetBrokenFormation( false );

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			if ( m_formationPath.Compute( me, myFormationSpot, cost ) == false )
			{
				// no path back to formation
				me->SetBrokenFormation( true );
			}

			// if we have a long path to get back in formation, we've broken ranks
			const float tooFar = 750.0f;
			if ( m_formationPath.GetLength() > tooFar )
			{
				me->SetBrokenFormation( true );
			}
		}

		m_formationPath.Update( me );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotEscortSquadLeader::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotWaitForOutOfPositionSquadMember::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_waitTimer.Start( 2.0f );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotWaitForOutOfPositionSquadMember::Update( CTFBot *me, float interval )
{
	if ( m_waitTimer.IsElapsed() )
	{
		return Done( "Timeout" );
	}

	if ( !me->IsInASquad() || !me->GetSquad()->IsLeader( me ) )
	{
		return Done( "No squad" );
	}

	if ( me->GetSquad()->IsInFormation() )
	{
		// Everyone is in position
		return Done( "Everyone is in formation. Moving on." );
	}

	return Continue();
}
