//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_path_follower.cpp
// Simplified path following for TF2
// Author: Michael Booth, November 2010

#include "cbase.h"

#include "NextBotManager.h"
#include "tf_path_follower.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//--------------------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
CTFPathFollower::CTFPathFollower( void )
{
	m_goal = NULL;
	m_minLookAheadRange = 300.0f;
}


//--------------------------------------------------------------------------------------------------------------
CTFPathFollower::~CTFPathFollower()
{
	// allow bots to detach pointer to me
	CUtlVector< INextBot * > botVector;
	TheNextBots().CollectAllBots( &botVector );
	
	for( int i=0; i<botVector.Count(); ++i )
	{
		botVector[i]->NotifyPathDestruction( this );
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * When the path is invalidated, the follower is also reset
 */
void CTFPathFollower::Invalidate( void )
{
	// extend
	Path::Invalidate();

	m_goal = NULL;
	MoveCursorToStart();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Invoked when the path is (re)computed (path is valid at the time of this call)
 */
void CTFPathFollower::OnPathChanged( INextBot *bot, Path::ResultType result )
{
	// start from the beginning
	m_goal = FirstSegment();
	MoveCursorToStart();
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Move mover along path
 */
void CTFPathFollower::Update( INextBot *bot )
{
	VPROF_BUDGET( "CTFPathFollower::Update", "NextBot" );
	ILocomotion *mover = bot->GetLocomotionInterface();

	// track most recent path followed
	bot->SetCurrentPath( this );

	if ( !IsValid() || m_goal == NULL )
	{
		return;
	}

	// check if we've reached the end of the path
	const float nearRange = 25.0f;
	if ( mover->IsOnGround() && ( GetEndPosition() - mover->GetFeet() ).AsVector2D().IsLengthLessThan( nearRange ) )
	{
		// the end of the path has been reached
		mover->GetBot()->OnMoveToSuccess( this );

		if ( bot->IsDebugging( NEXTBOT_PATH ) )
		{
			DevMsg( "CTFPathFollower: OnMoveToSuccess\n" );
		}

		// don't invalidate if OnMoveToSuccess just recomputed a new path
		if ( GetAge() > 0.0f )
		{
			Invalidate();
		}

		return;
	}

	// move along the path
	MoveCursorToClosestPosition( mover->GetFeet(), SEEK_AHEAD );
	float myCursorPosition = GetCursorPosition();
	const Path::Data &data = GetCursorData();

	if ( !data.segmentPrior )
	{
		// this shouldn't happen
		mover->GetBot()->OnMoveToFailure( this, FAIL_STUCK );
		Invalidate();
		return;
	}

	// set goal to be just ahead of wherever we happen to be on the path
	m_goal = NextSegment( data.segmentPrior );

	if ( !m_goal )
	{
		m_goal = data.segmentPrior;
	}

	// find actual move-to position farther down the path
	Vector moveToPos = m_goal->pos;

	// follow point farther down the path to smooth out our movement
	for( float ahead = m_minLookAheadRange; ahead > 0.0f; ahead -= 50.0f )
	{
		MoveCursor( myCursorPosition, PATH_ABSOLUTE_DISTANCE );
		MoveCursor( ahead, PATH_RELATIVE_DISTANCE );

		// get path data at this lookahead point
		const Path::Data &data = GetCursorData();

		if ( mover->IsPotentiallyTraversable( mover->GetFeet(), data.pos ) )
		{
			moveToPos = data.pos;
			break;
		}
	}

	// move bot along path
	mover->FaceTowards( moveToPos );
	mover->Approach( moveToPos );

	// debug display
	if ( bot->IsDebugging( NEXTBOT_PATH ) )
	{
		Path::Draw();

		NDebugOverlay::Cross3D( moveToPos, 5.0f, 150, 150, 255, true, 0.1f );
		NDebugOverlay::Line( bot->GetEntity()->WorldSpaceCenter(), moveToPos, 255, 255, 0, true, 0.1f );
	}
}

