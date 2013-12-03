//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_link.h"
#include "ai_navtype.h"
#include "ai_waypoint.h"
#include "ai_pathfinder.h"
#include "ai_navgoaltype.h"
#include "ai_routedist.h"
#include "ai_route.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SIMPLE_DATADESC(CAI_Path)
	//					m_Waypoints	(reconsititute on load)
	DEFINE_FIELD( m_goalTolerance,	FIELD_FLOAT ),
	DEFINE_CUSTOM_FIELD( m_activity,	ActivityDataOps() ),
	DEFINE_FIELD( m_target,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_sequence,		FIELD_INTEGER ),
	DEFINE_FIELD( m_vecTargetOffset,	FIELD_VECTOR ),
	DEFINE_FIELD( m_waypointTolerance, FIELD_FLOAT ),
	DEFINE_CUSTOM_FIELD( m_arrivalActivity,	ActivityDataOps() ),
	DEFINE_FIELD( m_arrivalSequence,		FIELD_INTEGER ),
	//					m_iLastNodeReached
	DEFINE_FIELD( m_bGoalPosSet,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_goalPos,			FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_bGoalTypeSet,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_goalType,			FIELD_INTEGER ),
	DEFINE_FIELD( m_goalFlags,		FIELD_INTEGER ),
	DEFINE_FIELD( m_routeStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_goalDirection,	FIELD_VECTOR ),
	DEFINE_FIELD( m_goalDirectionTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_goalSpeed,	FIELD_FLOAT ),
	DEFINE_FIELD( m_goalSpeedTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_goalStoppingDistance, FIELD_FLOAT ),
END_DATADESC()

//-----------------------------------------------------------------------------
AI_Waypoint_t CAI_Path::gm_InvalidWaypoint( Vector(0,0,0), 0, NAV_NONE, 0, 0 );

//-----------------------------------------------------------------------------

void CAI_Path::SetWaypoints(AI_Waypoint_t* route, bool fSetGoalFromLast) 
{ 
	m_Waypoints.Set(route);

	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		pLast->flPathDistGoal = -1;
		if ( fSetGoalFromLast )
		{
			if ( pLast )
			{
				m_bGoalPosSet = false;
				pLast->ModifyFlags( bits_WP_TO_GOAL, true );
				SetGoalPosition(pLast->GetPos());
			}
		}
	}

	AssertRouteValid( m_Waypoints.GetFirst() );
}

//-----------------------------------------------------------------------------

void CAI_Path::PrependWaypoints( AI_Waypoint_t *pWaypoints ) 
{ 
	m_Waypoints.PrependWaypoints( pWaypoints ); 
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	pLast->flPathDistGoal = -1;

	AssertRouteValid( m_Waypoints.GetFirst() );
}

//-----------------------------------------------------------------------------

void CAI_Path::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags ) 
{ 
	m_Waypoints.PrependWaypoint( newPoint, navType, waypointFlags ); 
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	pLast->flPathDistGoal = -1;

	AssertRouteValid( m_Waypoints.GetFirst() );
}

//-----------------------------------------------------------------------------

float CAI_Path::GetPathLength()
{
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast && pLast->flPathDistGoal == -1 )
	{
		ComputeRouteGoalDistances( pLast );
	}
	AI_Waypoint_t *pCurrent = GetCurWaypoint();
	return ( ( pCurrent  ) ? pCurrent->flPathDistGoal : 0 );
}

//-----------------------------------------------------------------------------

float CAI_Path::GetPathDistanceToGoal( const Vector &startPos )
{
	AI_Waypoint_t *pCurrent = GetCurWaypoint();
	if ( pCurrent )
	{
		return ( GetPathLength() + ComputePathDistance(pCurrent->NavType(), startPos, pCurrent->GetPos()) );
	}
	return 0;
}

//-----------------------------------------------------------------------------

Activity CAI_Path::SetMovementActivity(Activity activity)
{ 
	Assert( activity != ACT_RESET && activity != ACT_INVALID );
	//Msg("Set movement to %s\n", ActivityList_NameForIndex(activity) );

	m_sequence = ACT_INVALID;
	return (m_activity = activity);	
}

//-----------------------------------------------------------------------------

Activity CAI_Path::GetArrivalActivity( ) const
{
	if ( !m_Waypoints.IsEmpty() )
	{
		return m_arrivalActivity;
	}
	return ACT_INVALID;
}

//-----------------------------------------------------------------------------

void CAI_Path::SetArrivalActivity(Activity activity)
{
	m_arrivalActivity = activity;
	m_arrivalSequence = ACT_INVALID;
}

//-----------------------------------------------------------------------------

int CAI_Path::GetArrivalSequence( ) const
{
	if ( !m_Waypoints.IsEmpty() )
	{
		return m_arrivalSequence;
	}
	return ACT_INVALID;
}

//-----------------------------------------------------------------------------

void CAI_Path::SetArrivalSequence( int sequence )
{
	m_arrivalSequence = sequence;
}


//-----------------------------------------------------------------------------

void CAI_Path::SetGoalDirection( const Vector &goalDirection )
{
	m_goalDirectionTarget = NULL;
	m_goalDirection = goalDirection;
	VectorNormalize( m_goalDirection );
	/*
	AI_Waypoint_t *pLast = m_Waypoints.GetLast();
	if ( pLast )
	{
		NDebugOverlay::Box( pLast->vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,0,255, 0, 2.0 );
		NDebugOverlay::Line( pLast->vecLocation, pLast->vecLocation + m_goalDirection * 32, 0,0,255, true, 2.0 );
	}
	*/
}


//-----------------------------------------------------------------------------

void CAI_Path::SetGoalDirection( CBaseEntity *pTarget )
{
	m_goalDirectionTarget = pTarget;

	if (pTarget)
	{
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
		{
			m_goalDirection = pTarget->GetAbsOrigin() - pLast->vecLocation;
			VectorNormalize( m_goalDirection );
			/*
			NDebugOverlay::Box( pLast->vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,0,255, 0, 2.0 );
			NDebugOverlay::Line( pLast->vecLocation, pLast->vecLocation + m_goalDirection * 32, 0,0,255, true, 2.0 );
			*/
		}
	}
}

//-----------------------------------------------------------------------------

Vector CAI_Path::GetGoalDirection( const Vector &startPos )
{
	if (m_goalDirectionTarget)
	{
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
		{
			AI_Waypoint_t *pPrev = pLast->GetPrev();
			if (pPrev)
			{
				Vector goalDirection = m_goalDirectionTarget->GetAbsOrigin() - pPrev->vecLocation;
				VectorNormalize( goalDirection );
				return goalDirection;
			}
			else
			{
				Vector goalDirection = m_goalDirectionTarget->GetAbsOrigin() - startPos;
				VectorNormalize( goalDirection );
				return goalDirection;
			}
		}
	}
	else if (m_goalDirection == vec3_origin)
	{
		// Assert(0); // comment out the default directions in SetGoal() to find test cases for missing initialization
		AI_Waypoint_t *pLast = m_Waypoints.GetLast();
		if ( pLast )
		{
			AI_Waypoint_t *pPrev = pLast->GetPrev();
			if (pPrev)
			{
				Vector goalDirection = pLast->vecLocation - pPrev->vecLocation;
				VectorNormalize( goalDirection );
				return goalDirection;
			}
			else
			{
				Vector goalDirection =pLast->vecLocation - startPos;
				VectorNormalize( goalDirection );
				return goalDirection;
			}
		}
	}

	return m_goalDirection;
}

//-----------------------------------------------------------------------------

void CAI_Path::SetGoalSpeed( float flSpeed )
{
	m_goalSpeed = flSpeed;
}


//-----------------------------------------------------------------------------

void CAI_Path::SetGoalSpeed( CBaseEntity *pTarget )
{
	m_goalSpeedTarget = pTarget;
}

//-----------------------------------------------------------------------------

float CAI_Path::GetGoalSpeed( const Vector &startPos )
{
	if (m_goalSpeedTarget)
	{
		Vector goalDirection = GetGoalDirection( startPos );
		Vector targetVelocity = m_goalSpeedTarget->GetSmoothedVelocity();
		float dot = DotProduct( goalDirection, targetVelocity );
		dot = MAX( 0.0f, dot );
		// return a relative impact speed of m_goalSpeed
		if (m_goalSpeed > 0.0)
		{
			return dot + m_goalSpeed;
		}
		return dot;
	}
	return m_goalSpeed;
}



//-----------------------------------------------------------------------------

void CAI_Path::SetGoalStoppingDistance( float flDistance )
{
	m_goalStoppingDistance = flDistance;
}

//-----------------------------------------------------------------------------

float CAI_Path::GetGoalStoppingDistance( ) const
{
	return m_goalStoppingDistance;
}


//-----------------------------------------------------------------------------
const Vector &CAI_Path::CurWaypointPos() const	
{ 
	if ( GetCurWaypoint() )
		return GetCurWaypoint()->GetPos(); 
	AssertMsg(0, "Invalid call to CurWaypointPos()");
	return gm_InvalidWaypoint.GetPos();
}

//-----------------------------------------------------------------------------
const Vector &CAI_Path::NextWaypointPos() const
{ 
	if ( GetCurWaypoint() && GetCurWaypoint()->GetNext())
		return GetCurWaypoint()->GetNext()->GetPos(); 
	static Vector invalid( 0, 0, 0 );
	AssertMsg(0, "Invalid call to NextWaypointPos()");
	return gm_InvalidWaypoint.GetPos();
}

//-----------------------------------------------------------------------------
float CAI_Path::CurWaypointYaw() const
{ 
	return GetCurWaypoint()->flYaw; 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetGoalPosition(const Vector &goalPos) 
{

#ifdef _DEBUG
	// Make sure goal position isn't set more than once
	if (m_bGoalPosSet == true)
	{
		DevMsg( "GetCurWaypoint Goal Position Set Twice!\n");
	}
#endif

	m_bGoalPosSet = true;
	VectorAdd( goalPos, m_vecTargetOffset, m_goalPos );
}

//-----------------------------------------------------------------------------
// Purpose: Sets last node as goal and goal position
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetLastNodeAsGoal(bool bReset)
{
	#ifdef _DEBUG
		// Make sure goal position isn't set more than once
		if (!bReset && m_bGoalPosSet == true)
		{
			DevMsg( "GetCurWaypoint Goal Position Set Twice!\n");
		}
	#endif	
	
	// Find the last node
	if (GetCurWaypoint()) 
	{
		AI_Waypoint_t* waypoint = GetCurWaypoint();

		while (waypoint)
		{
			if (!waypoint->GetNext())
			{
				m_goalPos = waypoint->GetPos();
				m_bGoalPosSet = true;
				waypoint->ModifyFlags( bits_WP_TO_GOAL, true );
				return;
			}
			waypoint = waypoint->GetNext();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Explicitly change the goal position w/o check
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::ResetGoalPosition(const Vector &goalPos) 
{
	m_bGoalPosSet	= true;
	VectorAdd( goalPos, m_vecTargetOffset, m_goalPos );
}


//-----------------------------------------------------------------------------
// Returns the *base* goal position (without the offset applied) 
//-----------------------------------------------------------------------------
const Vector& CAI_Path::BaseGoalPosition() const
{
#ifdef _DEBUG
	// Make sure goal position was set
	if (m_bGoalPosSet == false)
	{
		DevMsg( "GetCurWaypoint Goal Position Never Set!\n");
	}
#endif

	// FIXME: A little risky; store the base if this becomes a problem
	static Vector vecResult;
	VectorSubtract(	m_goalPos, m_vecTargetOffset, vecResult );
	return vecResult;
}


//-----------------------------------------------------------------------------
	// Returns the *actual* goal position (with the offset applied)
//-----------------------------------------------------------------------------
const Vector & CAI_Path::ActualGoalPosition(void) const
{
#ifdef _DEBUG
	// Make sure goal position was set
	if (m_bGoalPosSet == false)
	{
		DevMsg( "GetCurWaypoint Goal Position Never Set!\n");
	}
#endif

	return m_goalPos;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::SetGoalType(GoalType_t goalType) 
{

#ifdef _DEBUG
	// Make sure goal position isn't set more than once
	if (m_goalType != GOALTYPE_NONE && goalType != GOALTYPE_NONE )
	{
		DevMsg( "GetCurWaypoint Goal Type Set Twice!\n");
	}
#endif

	if (m_goalType != GOALTYPE_NONE)
	{
		m_routeStartTime = gpGlobals->curtime;
		m_bGoalTypeSet	= true;
	}
	else
		m_bGoalTypeSet	= false;

	m_goalType		= goalType;

}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
GoalType_t	CAI_Path::GoalType(void) const
{
	return m_goalType;
}


//-----------------------------------------------------------------------------

void CAI_Path::Advance( void )
{
	if ( CurWaypointIsGoal() )
		return;

	// -------------------------------------------------------
	// If I have another waypoint advance my path
	// -------------------------------------------------------
	if (GetCurWaypoint()->GetNext()) 
	{
		AI_Waypoint_t *pNext = GetCurWaypoint()->GetNext();

		// If waypoint was a node take note of it
		if (GetCurWaypoint()->Flags() & bits_WP_TO_NODE)
		{
			m_iLastNodeReached = GetCurWaypoint()->iNodeID;
		}

		delete GetCurWaypoint();
		SetWaypoints(pNext);

		return;
	}
	// -------------------------------------------------
	//  This is an error catch that should *not* happen
	//  It means a route was created with no goal
	// -------------------------------------------------
	else 
	{
		DevMsg( "!!ERROR!! Force end of route with no goal!\n");
		GetCurWaypoint()->ModifyFlags( bits_WP_TO_GOAL, true );
	}

	AssertRouteValid( m_Waypoints.GetFirst() );
}



//-----------------------------------------------------------------------------
// Purpose: Clears the route and resets all its fields to default values
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Path::Clear( void )
{
	m_Waypoints.RemoveAll();

	m_goalType			= GOALTYPE_NONE;		// Type of goal
	m_goalPos			= vec3_origin;			// Our ultimate goal position
	m_bGoalPosSet		= false;				// Was goal position set
	m_bGoalTypeSet		= false;				// Was goal position set
	m_goalFlags			= false;
	m_vecTargetOffset	= vec3_origin;
	m_routeStartTime	= FLT_MAX;

	m_goalTolerance		= 0.0;					// How close do we need to get to the goal
	// FIXME: split m_goalTolerance into m_buildTolerance and m_moveTolerance, let them be seperatly controllable.

	m_activity			= ACT_INVALID;
	m_sequence			= ACT_INVALID;
	m_target			= NULL;

	m_arrivalActivity = ACT_INVALID;
	m_arrivalSequence = ACT_INVALID;

	m_goalDirectionTarget = NULL;
	m_goalDirection = vec3_origin;

	m_goalSpeedTarget = NULL;
	m_goalSpeed			= -1.0f;	// init to an invalid speed

	m_goalStoppingDistance = 0.0;				// How close to we want to get to the goal
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
Navigation_t CAI_Path::CurWaypointNavType()	const
{
	if (!GetCurWaypoint())
	{
		return NAV_NONE;
	}
	else
	{
		return GetCurWaypoint()->NavType();
	}
}

int CAI_Path::CurWaypointFlags() const
{
	if (!GetCurWaypoint())
	{
		return 0;
	}
	else
	{
		return GetCurWaypoint()->Flags();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Get the goal's flags
// Output : unsigned
//-----------------------------------------------------------------------------
unsigned CAI_Path::GoalFlags( void ) const
{
	return m_goalFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if current waypoint is my goal
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Path::CurWaypointIsGoal( void ) const
{
//	Assert( GetCurWaypoint() );

	if( !GetCurWaypoint() )
		return false;


	if ( GetCurWaypoint()->Flags() & bits_WP_TO_GOAL )
	{
		#ifdef _DEBUG
			if (GetCurWaypoint()->GetNext())
			{
				DevMsg( "!!ERROR!! Goal is not last waypoint!\n");
			}
			if ((GetCurWaypoint()->GetPos() - m_goalPos).Length() > 0.1)
			{
				DevMsg( "!!ERROR!! Last waypoint isn't in goal position!\n");
			}
		#endif
		return true;
	}
	if ( GetCurWaypoint()->Flags() & bits_WP_TO_PATHCORNER )
	{
		// UNDONE: Refresh here or somewhere else?
	}
#ifdef _DEBUG
	if (!GetCurWaypoint()->GetNext())
	{
		DevMsg( "!!ERROR!! GetCurWaypoint has no goal!\n");
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------
// Computes the goal distance for each waypoint along the route
//-----------------------------------------------------------------------------
void CAI_Path::ComputeRouteGoalDistances(AI_Waypoint_t *pGoalWaypoint)
{
	// The goal distance is the distance from any waypoint to the goal waypoint

	// Backup through the list and calculate distance to goal
	AI_Waypoint_t *pPrev;
	AI_Waypoint_t *pCurWaypoint = pGoalWaypoint;
	pCurWaypoint->flPathDistGoal = 0;
	while (pCurWaypoint->GetPrev())
	{
		pPrev = pCurWaypoint->GetPrev();

		float flWaypointDist = ComputePathDistance(pCurWaypoint->NavType(), pPrev->GetPos(), pCurWaypoint->GetPos());
		pPrev->flPathDistGoal = pCurWaypoint->flPathDistGoal + flWaypointDist;
		
		pCurWaypoint = pPrev;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Path::CAI_Path()
{
	m_goalType			= GOALTYPE_NONE;		// Type of goal
	m_goalPos			= vec3_origin;			// Our ultimate goal position
	m_goalTolerance		= 0.0;					// How close do we need to get to the goal
	m_activity			= ACT_INVALID;			// The activity to use during motion
	m_sequence			= ACT_INVALID;
	m_target			= NULL;
	m_goalFlags			= 0;
	m_routeStartTime	= FLT_MAX;
	m_arrivalActivity	= ACT_INVALID;
	m_arrivalSequence	= ACT_INVALID;

	m_iLastNodeReached  = NO_NODE;

	m_waypointTolerance = DEF_WAYPOINT_TOLERANCE;

}

CAI_Path::~CAI_Path()
{
	DeleteAll( GetCurWaypoint() );
}


