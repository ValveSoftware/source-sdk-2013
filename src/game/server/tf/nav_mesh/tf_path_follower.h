//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_path_follower.h
// Simplified path following for TF2
// Author: Michael Booth, November 2010

#ifndef TF_PATH_FOLLOWER_H
#define TF_PATH_FOLLOWER_H

#include "nav_mesh.h"
#include "nav_pathfind.h"
#include "Path/NextBotPathFollow.h"

class INextBot;
class ILocomotion;


//--------------------------------------------------------------------------------------------------------
/**
 * This is a simplified path follower that doesn't care about ladders, climbing, hindrances, etc.
 */
class CTFPathFollower : public PathFollower
{
public:
	CTFPathFollower( void );
	virtual ~CTFPathFollower();

	virtual void Invalidate( void );										// (EXTEND) cause the path to become invalid
	virtual void OnPathChanged( INextBot *bot, Path::ResultType result );	// invoked when the path is (re)computed (path is valid at the time of this call)

	virtual void Update( INextBot *bot );									// move bot along path

	virtual const Path::Segment *GetCurrentGoal( void ) const;				// return current goal along the path we are trying to reach

	virtual void SetMinLookAheadDistance( float value );					// minimum range movement goal must be along path
	
private:
	const Path::Segment *m_goal;					// our current goal along the path
	float m_minLookAheadRange;

// 	bool CheckProgress( INextBot *bot );
// 	bool IsAtGoal( INextBot *bot ) const;			// return true if reached current path goal
};


inline const Path::Segment *CTFPathFollower::GetCurrentGoal( void ) const
{
	return m_goal;
}


inline void CTFPathFollower::SetMinLookAheadDistance( float value )
{
	m_minLookAheadRange = value;
}

#endif // TF_PATH_FOLLOWER_H


