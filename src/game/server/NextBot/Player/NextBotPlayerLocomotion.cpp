// NextBotPlayerLocomotion.cpp
// Implementation of Locomotion interface for CBasePlayer-derived classes
// Author: Michael Booth, November 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "nav_mesh.h"
#include "in_buttons.h"
#include "NextBot.h"
#include "NextBotUtil.h"
#include "NextBotPlayer.h"
#include "NextBotPlayerLocomotion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar NextBotPlayerMoveDirect( "nb_player_move_direct", "0" );

//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::PlayerLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	m_player = NULL;
	Reset();
}


//-----------------------------------------------------------------------------------------------------
/**
 * Reset locomotor to initial state
 */
void PlayerLocomotion::Reset( void )
{
	m_player = static_cast< CBasePlayer * >( GetBot()->GetEntity() );
	
	m_isJumping = false;
	m_isClimbingUpToLedge = false;
	m_isJumpingAcrossGap = false;
	m_hasLeftTheGround = false;
	m_desiredSpeed = 0.0f;


	m_ladderState = NO_LADDER;
	m_ladderInfo = NULL;
	m_ladderDismountGoal = NULL;
	m_ladderTimer.Invalidate();

	m_minSpeedLimit = 0.0f;
	m_maxSpeedLimit = 9999999.9f;

	BaseClass::Reset();
}


//-----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::TraverseLadder( void )
{
	switch( m_ladderState )
	{
	case APPROACHING_ASCENDING_LADDER:
		m_ladderState = ApproachAscendingLadder();
		return true;

	case APPROACHING_DESCENDING_LADDER:
		m_ladderState = ApproachDescendingLadder();
		return true;

	case ASCENDING_LADDER:
		m_ladderState = AscendLadder();
		return true;

	case DESCENDING_LADDER:
		m_ladderState = DescendLadder();
		return true;

	case DISMOUNTING_LADDER_TOP:
		m_ladderState = DismountLadderTop();
		return true;

	case DISMOUNTING_LADDER_BOTTOM:
		m_ladderState = DismountLadderBottom();
		return true;

	case NO_LADDER:
	default:
		m_ladderInfo = NULL;

		if ( GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER )
		{
			// on ladder and don't want to be
			GetBot()->GetEntity()->SetMoveType( MOVETYPE_WALK );
		}
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------------------
/**
 * We're close, but not yet on, this ladder - approach it
 */
PlayerLocomotion::LadderState PlayerLocomotion::ApproachAscendingLadder( void )
{
	if ( m_ladderInfo == NULL )
	{
		return NO_LADDER;
	}

	// sanity check - are we already at the end of this ladder?
	if ( GetFeet().z >= m_ladderInfo->m_top.z - GetStepHeight() )
	{
		m_ladderTimer.Start( 2.0f );
		return DISMOUNTING_LADDER_TOP;
	}

	// sanity check - are we too far below this ladder to reach it?
	if ( GetFeet().z <= m_ladderInfo->m_bottom.z - GetMaxJumpHeight() )
	{
		return NO_LADDER;
	}

	FaceTowards( m_ladderInfo->m_bottom );

	// it is important to approach precisely, so use a very large weight to wash out all other Approaches
	Approach( m_ladderInfo->m_bottom, 9999999.9f );

	if ( GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER )
	{
		// we're on the ladder
		return ASCENDING_LADDER;
	}

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::EntityText( GetBot()->GetEntity()->entindex(), 0, "Approach ascending ladder", 0.1f, 255, 255, 255, 255 );
	}

	return APPROACHING_ASCENDING_LADDER;
}


//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::LadderState PlayerLocomotion::ApproachDescendingLadder( void )
{
	if ( m_ladderInfo == NULL )
	{
		return NO_LADDER;
	}

	// sanity check - are we already at the end of this ladder?
	if ( GetFeet().z <= m_ladderInfo->m_bottom.z + GetMaxJumpHeight() )
	{
		m_ladderTimer.Start( 2.0f );
		return DISMOUNTING_LADDER_BOTTOM;
	}

	Vector mountPoint = m_ladderInfo->m_top + 0.25f * GetBot()->GetBodyInterface()->GetHullWidth() * m_ladderInfo->GetNormal();
	Vector to = mountPoint - GetFeet();
	to.z = 0.0f;

	float mountRange = to.NormalizeInPlace();
	Vector moveGoal;

	const float veryClose = 10.0f;
	if ( mountRange < veryClose )
	{
		// we're right at the ladder - just keep moving forward until we grab it
		const Vector &forward = GetMotionVector();
		moveGoal = GetFeet() + 100.0f * forward;
	}
	else
	{
		if ( DotProduct( to, m_ladderInfo->GetNormal() ) < 0.0f )
		{
			// approaching front of downward ladder
			//     ##
			// ->+ ##
			//   | ##
			//   | ##
			//   | ##
			// <-+ ##
			// ######
			//
			moveGoal = m_ladderInfo->m_top - 100.0f * m_ladderInfo->GetNormal();
		}
		else
		{
			// approaching back of downward ladder
			//
			// ->+
			// ##|
			// ##|
			// ##+-->
			// ######
			//
			moveGoal = m_ladderInfo->m_top + 100.0f * m_ladderInfo->GetNormal();
		}
	}

	FaceTowards( moveGoal );

	// it is important to approach precisely, so use a very large weight to wash out all other Approaches
	Approach( moveGoal, 9999999.9f );

	if ( GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER )
	{
		// we're on the ladder
		return DESCENDING_LADDER;
	}

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::EntityText( GetBot()->GetEntity()->entindex(), 0, "Approach descending ladder", 0.1f, 255, 255, 255, 255 );
	}

	return APPROACHING_DESCENDING_LADDER;
}


//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::LadderState PlayerLocomotion::AscendLadder( void )
{
	if ( m_ladderInfo == NULL )
	{
		return NO_LADDER;
	}

	if ( GetBot()->GetEntity()->GetMoveType() != MOVETYPE_LADDER )
	{
		// slipped off ladder
		m_ladderInfo = NULL;
		return NO_LADDER;
	}

	if ( GetFeet().z >= m_ladderInfo->m_top.z )
	{
		// reached top of ladder
		m_ladderTimer.Start( 2.0f );
		return DISMOUNTING_LADDER_TOP;
	}

	// climb up this ladder - look up
	Vector goal = GetFeet() + 100.0f * ( -m_ladderInfo->GetNormal() + Vector( 0, 0, 2 ) );

	GetBot()->GetBodyInterface()->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, NULL, "Ladder" );

	// it is important to approach precisely, so use a very large weight to wash out all other Approaches
	Approach( goal, 9999999.9f );

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::EntityText( GetBot()->GetEntity()->entindex(), 0, "Ascend", 0.1f, 255, 255, 255, 255 );
	}

	return ASCENDING_LADDER;
}


//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::LadderState PlayerLocomotion::DescendLadder( void )
{
	if ( m_ladderInfo == NULL )
	{
		return NO_LADDER;
	}

	if ( GetBot()->GetEntity()->GetMoveType() != MOVETYPE_LADDER )
	{
		// slipped off ladder
		m_ladderInfo = NULL;
		return NO_LADDER;
	}

	if ( GetFeet().z <= m_ladderInfo->m_bottom.z + GetBot()->GetLocomotionInterface()->GetStepHeight() )
	{
		// reached bottom of ladder
		m_ladderTimer.Start( 2.0f );
		return DISMOUNTING_LADDER_BOTTOM;
	}

	// climb down this ladder - look down 
	Vector goal = GetFeet() + 100.0f * ( m_ladderInfo->GetNormal() + Vector( 0, 0, -2 ) );

	GetBot()->GetBodyInterface()->AimHeadTowards( goal, IBody::MANDATORY, 0.1f, NULL, "Ladder" );

	// it is important to approach precisely, so use a very large weight to wash out all other Approaches
	Approach( goal, 9999999.9f );

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::EntityText( GetBot()->GetEntity()->entindex(), 0, "Descend", 0.1f, 255, 255, 255, 255 );
	}

	return DESCENDING_LADDER;
}


//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderTop( void )
{
	if ( m_ladderInfo == NULL || m_ladderTimer.IsElapsed() )
	{
		m_ladderInfo = NULL;
		return NO_LADDER;
	}

	IBody *body = GetBot()->GetBodyInterface();
	Vector toGoal = m_ladderDismountGoal->GetCenter() - GetFeet();
	toGoal.z = 0.0f;
	float range = toGoal.NormalizeInPlace();
	toGoal.z = 1.0f;
	
	body->AimHeadTowards( body->GetEyePosition() + 100.0f * toGoal, IBody::MANDATORY, 0.1f, NULL, "Ladder dismount" );

	// it is important to approach precisely, so use a very large weight to wash out all other Approaches
	Approach( GetFeet() + 100.0f * toGoal, 9999999.9f );

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::EntityText( GetBot()->GetEntity()->entindex(), 0, "Dismount top", 0.1f, 255, 255, 255, 255 );
		NDebugOverlay::HorzArrow( GetFeet(), m_ladderDismountGoal->GetCenter(), 5.0f, 255, 255, 0, 255, true, 0.1f );
	}

	// test 2D vector here in case nav area is under the geometry a bit
	const float tolerance = 10.0f;
	if ( GetBot()->GetEntity()->GetLastKnownArea() == m_ladderDismountGoal && range < tolerance )
	{
		// reached dismount goal
		m_ladderInfo = NULL;
		return NO_LADDER;
	}

	return DISMOUNTING_LADDER_TOP;
}


//-----------------------------------------------------------------------------------------------------
PlayerLocomotion::LadderState PlayerLocomotion::DismountLadderBottom( void )
{
	if ( m_ladderInfo == NULL || m_ladderTimer.IsElapsed() )
	{
		m_ladderInfo = NULL;
		return NO_LADDER;
	}

	if ( GetBot()->GetEntity()->GetMoveType() == MOVETYPE_LADDER )
	{
		// near the bottom - just let go
		GetBot()->GetEntity()->SetMoveType( MOVETYPE_WALK );
		m_ladderInfo = NULL;
	}

	return NO_LADDER;
}


//-----------------------------------------------------------------------------------------------------
/**
 * Update internal state
 */
void PlayerLocomotion::Update( void )
{
	if ( TraverseLadder() )
	{
		return BaseClass::Update();
	}

	if ( m_isJumpingAcrossGap || m_isClimbingUpToLedge )
	{
		// force a run
		SetMinimumSpeedLimit( GetRunSpeed() );

		Vector toLanding = m_landingGoal - GetFeet();
		toLanding.z = 0.0f;
		toLanding.NormalizeInPlace();

		if ( m_hasLeftTheGround )
		{
			// face into the jump/climb
			GetBot()->GetBodyInterface()->AimHeadTowards( GetBot()->GetEntity()->EyePosition() + 100.0 * toLanding, IBody::MANDATORY, 0.25f, NULL, "Facing impending jump/climb" );

			if ( IsOnGround() )
			{
				// back on the ground - jump is complete
				m_isClimbingUpToLedge = false;
				m_isJumpingAcrossGap = false;
				SetMinimumSpeedLimit( 0.0f );
			}
		}
		else
		{
			// haven't left the ground yet - just starting the jump

			if ( !IsClimbingOrJumping() )
			{
				Jump();
			}

			Vector vel = GetBot()->GetEntity()->GetAbsVelocity();

			if ( m_isJumpingAcrossGap )
			{
				// cheat and max our velocity in case we were stopped at the edge of this gap
				vel.x = GetRunSpeed() * toLanding.x;
				vel.y = GetRunSpeed() * toLanding.y;
				// leave vel.z unchanged
			}

			GetBot()->GetEntity()->SetAbsVelocity( vel );

			if ( !IsOnGround() )
			{
				// jump has begun
				m_hasLeftTheGround = true;
			}
		}

		
		Approach( m_landingGoal );
	}

	BaseClass::Update();
}


//-----------------------------------------------------------------------------------------------------
void PlayerLocomotion::AdjustPosture( const Vector &moveGoal )
{
	// This function has no effect if we're not standing or crouching
	IBody *body = GetBot()->GetBodyInterface();
	if ( !body->IsActualPosture( IBody::STAND ) && !body->IsActualPosture( IBody::CROUCH ) )
		return;

	// not all games have auto-crouch, so don't assume it here
	BaseClass::AdjustPosture( moveGoal );
}


//-----------------------------------------------------------------------------------------------------
/**
 * Build a user command to move this player towards the goal position
 */
void PlayerLocomotion::Approach( const Vector &pos, float goalWeight )
{
	VPROF_BUDGET( "PlayerLocomotion::Approach", "NextBot" );

	BaseClass::Approach( pos );

	AdjustPosture( pos );

	if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
	{
		NDebugOverlay::Line( GetFeet(), pos, 255, 255, 0, true, 0.1f );
	}

	INextBotPlayerInput *playerButtons = dynamic_cast< INextBotPlayerInput * >( GetBot() );

	if ( !playerButtons )
	{
		DevMsg( "PlayerLocomotion::Approach: No INextBotPlayerInput\n " );
		return;
	}

	Vector forward3D;
	m_player->EyeVectors( &forward3D );
	
	Vector2D forward( forward3D.x, forward3D.y );
	forward.NormalizeInPlace();
		
	Vector2D right( forward.y, -forward.x );

	// compute unit vector to goal position
	Vector2D to = ( pos - GetFeet() ).AsVector2D();
	float goalDistance = to.NormalizeInPlace();

	float ahead = to.Dot( forward );
	float side = to.Dot( right );

#ifdef NEED_TO_INTEGRATE_MOTION_CONTROLLED_CODE_FROM_L4D_PLAYERS
	// If we're climbing ledges, we need to stay crouched to prevent player movement code from messing
	// with our origin.
	CTerrorPlayer *player = ToTerrorPlayer(m_player);
	if ( player && player->IsMotionControlledZ( player->GetMainActivity() ) )
	{
		playerButtons->PressCrouchButton();
		return;
	}
#endif

	if ( m_player->IsOnLadder() && IsUsingLadder() && ( m_ladderState == ASCENDING_LADDER || m_ladderState == DESCENDING_LADDER ) )
	{
		// we are on a ladder and WANT to be on a ladder.
		playerButtons->PressForwardButton();

		// Stay in center of ladder.  The gamemovement will autocenter us in most cases, but this is needed in case it doesn't.
		if ( m_ladderInfo )
		{
			Vector posOnLadder;
			CalcClosestPointOnLine( GetFeet(), m_ladderInfo->m_bottom, m_ladderInfo->m_top, posOnLadder );

			Vector alongLadder = m_ladderInfo->m_top - m_ladderInfo->m_bottom;
			alongLadder.NormalizeInPlace();

			Vector rightLadder = CrossProduct( alongLadder, m_ladderInfo->GetNormal() );

			Vector away = GetFeet() - posOnLadder;

			// we only want error in plane of ladder
			float error = DotProduct( away, rightLadder );
			away.NormalizeInPlace();

			const float tolerance = 5.0f + 0.25f * GetBot()->GetBodyInterface()->GetHullWidth();
			if ( error > tolerance )
			{
				if ( DotProduct( away, rightLadder ) > 0.0f )
				{
					playerButtons->PressLeftButton();
				}
				else
				{
					playerButtons->PressRightButton();
				}
			}
		}
	}
	else
	{
		const float epsilon = 0.25f;
		if ( NextBotPlayerMoveDirect.GetBool() )
		{
			if ( goalDistance > epsilon )
			{
				playerButtons->SetButtonScale( ahead, side );
			}
		}

		if ( ahead > epsilon )
		{
			playerButtons->PressForwardButton();

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::HorzArrow( m_player->GetAbsOrigin(), m_player->GetAbsOrigin() + 50.0f * Vector( forward.x, forward.y, 0.0f ), 15.0f, 0, 255, 0, 255, true, 0.1f );
			}
		}
		else if ( ahead < -epsilon )
		{
			playerButtons->PressBackwardButton();

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::HorzArrow( m_player->GetAbsOrigin(), m_player->GetAbsOrigin() - 50.0f * Vector( forward.x, forward.y, 0.0f ), 15.0f, 255, 0, 0, 255, true, 0.1f );
			}
		}

		if ( side <= -epsilon )
		{
			playerButtons->PressLeftButton();

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::HorzArrow( m_player->GetAbsOrigin(), m_player->GetAbsOrigin() - 50.0f * Vector( right.x, right.y, 0.0f ), 15.0f, 255, 0, 255, 255, true, 0.1f );
			}
		}
		else if ( side >= epsilon )
		{
			playerButtons->PressRightButton();

			if ( GetBot()->IsDebugging( NEXTBOT_LOCOMOTION ) )
			{
				NDebugOverlay::HorzArrow( m_player->GetAbsOrigin(), m_player->GetAbsOrigin() + 50.0f * Vector( right.x, right.y, 0.0f ), 15.0f, 0, 255, 255, 255, true, 0.1f );
			}
		}
	}

	if ( !IsRunning() )
	{
		playerButtons->PressWalkButton();
	}
}


//----------------------------------------------------------------------------------------------------
/**
 * Move the bot to the precise given position immediately, 
 */
void PlayerLocomotion::DriveTo( const Vector &pos )
{
	BaseClass::DriveTo( pos );

	Approach( pos );
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::IsClimbPossible( INextBot *me, const CBaseEntity *obstacle ) const
{
	// don't jump unless we have to
	const PathFollower *path = GetBot()->GetCurrentPath();
	if ( path )
	{
		const float watchForClimbRange = 75.0f;
		if ( !path->IsDiscontinuityAhead( GetBot(), Path::CLIMB_UP, watchForClimbRange ) )
		{
			// we are not planning on climbing

			// always allow climbing over movable obstacles
			if ( obstacle && !const_cast< CBaseEntity * >( obstacle )->IsWorld() )
			{
				IPhysicsObject *physics = obstacle->VPhysicsGetObject();
				if ( physics && physics->IsMoveable() )
				{
					// movable physics object - climb over it
					return true;
				}
			}

			if ( !GetBot()->GetLocomotionInterface()->IsStuck() )
			{
				// we're not stuck - don't try to jump up yet
				return false;
			}
		}
	}

	return true;
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::ClimbUpToLedge( const Vector &landingGoal, const Vector &landingForward, const CBaseEntity *obstacle )
{
	if ( !IsClimbPossible( GetBot(), obstacle ) )
	{
		return false;
	}

	Jump();

	m_isClimbingUpToLedge = true;
	m_landingGoal = landingGoal;
	m_hasLeftTheGround = false;

	return true;
}


//----------------------------------------------------------------------------------------------------
void PlayerLocomotion::JumpAcrossGap( const Vector &landingGoal, const Vector &landingForward )
{
	Jump();

	// face forward
	GetBot()->GetBodyInterface()->AimHeadTowards( landingGoal, IBody::MANDATORY, 1.0f, NULL, "Looking forward while jumping a gap" );

	m_isJumpingAcrossGap = true;
	m_landingGoal = landingGoal;
	m_hasLeftTheGround = false;
}


//----------------------------------------------------------------------------------------------------
void PlayerLocomotion::Jump( void )
{
	m_isJumping = true;
	m_jumpTimer.Start( 0.5f );

	INextBotPlayerInput *playerButtons = dynamic_cast< INextBotPlayerInput * >( GetBot() );
	if ( playerButtons )
	{
		playerButtons->PressJumpButton();
	}
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::IsClimbingOrJumping( void ) const
{
	if ( !m_isJumping )
		return false;
		
	if ( m_jumpTimer.IsElapsed() && IsOnGround() )
	{
		m_isJumping = false;
		return false;
	}
	
	return true;
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::IsClimbingUpToLedge( void ) const
{
	return m_isClimbingUpToLedge;
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::IsJumpingAcrossGap( void ) const
{
	return m_isJumpingAcrossGap;
}


//----------------------------------------------------------------------------------------------------
/**
 * Return true if standing on something
 */
bool PlayerLocomotion::IsOnGround( void ) const
{
	return (m_player->GetGroundEntity() != NULL);
}


//----------------------------------------------------------------------------------------------------
/**
 * Return the current ground entity or NULL if not on the ground
 */
CBaseEntity *PlayerLocomotion::GetGround( void ) const
{
	return m_player->GetGroundEntity();
}


//----------------------------------------------------------------------------------------------------
/**
 * Surface normal of the ground we are in contact with
 */
const Vector &PlayerLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );
	return up;

	// TODO: Integrate movehelper_server for this:  return m_player->GetGroundNormal();
}


//----------------------------------------------------------------------------------------------------
/**
 * Climb the given ladder to the top and dismount
 */
void PlayerLocomotion::ClimbLadder( const CNavLadder *ladder, const CNavArea *dismountGoal )
{
	// look up and push forward
// 	Vector goal =  GetBot()->GetPosition() + 100.0f * ( Vector( 0, 0, 1.0f ) - ladder->GetNormal() );
// 	Approach( goal );
// 	FaceTowards( goal );
	
	m_ladderState = APPROACHING_ASCENDING_LADDER;
	m_ladderInfo = ladder;
	m_ladderDismountGoal = dismountGoal;
}


//----------------------------------------------------------------------------------------------------
/**
 * Descend the given ladder to the bottom and dismount
 */
void PlayerLocomotion::DescendLadder( const CNavLadder *ladder, const CNavArea *dismountGoal )
{
	// look down and push forward
// 	Vector goal =  GetBot()->GetPosition() + 100.0f * ( Vector( 0, 0, -1.0f ) - ladder->GetNormal() );
// 	Approach( goal );
// 	FaceTowards( goal );

	m_ladderState = APPROACHING_DESCENDING_LADDER;
	m_ladderInfo = ladder;
	m_ladderDismountGoal = dismountGoal;
}


//----------------------------------------------------------------------------------------------------
bool PlayerLocomotion::IsUsingLadder( void ) const
{
	return ( m_ladderState != NO_LADDER );
}


//----------------------------------------------------------------------------------------------------
/**
 * Rotate body to face towards "target"
 */
void PlayerLocomotion::FaceTowards( const Vector &target )
{
	// player body follows view direction
	Vector look( target.x, target.y, GetBot()->GetEntity()->EyePosition().z );

	GetBot()->GetBodyInterface()->AimHeadTowards( look, IBody::BORING, 0.1f, NULL, "Body facing" );
}


//-----------------------------------------------------------------------------------------------------
/**
* Return position of "feet" - point below centroid of bot at feet level
*/
const Vector &PlayerLocomotion::GetFeet( void ) const
{
	return m_player->GetAbsOrigin();
}


//-----------------------------------------------------------------------------------------------------
/**
 * Return current world space velocity
 */
const Vector &PlayerLocomotion::GetVelocity( void ) const
{
	return m_player->GetAbsVelocity();
}


//-----------------------------------------------------------------------------------------------------
float PlayerLocomotion::GetRunSpeed( void ) const
{
	return m_player->MaxSpeed();
}


//-----------------------------------------------------------------------------------------------------
float PlayerLocomotion::GetWalkSpeed( void ) const
{
	return 0.5f * m_player->MaxSpeed();
}

