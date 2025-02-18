//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
// @TODO (toml 06-26-02): The entry points in this file need to be organized
//=============================================================================//

#include "cbase.h"

#include <float.h> // for FLT_MAX

#include "animation.h"		// for NOMOTION
#include "collisionutils.h"
#include "ndebugoverlay.h"
#include "isaverestore.h"
#include "saverestore_utlvector.h"

#include "ai_navigator.h"
#include "ai_node.h"
#include "ai_route.h"
#include "ai_routedist.h"
#include "ai_waypoint.h"
#include "ai_pathfinder.h"
#include "ai_link.h"
#include "ai_memory.h"
#include "ai_motor.h"
#include "ai_localnavigator.h"
#include "ai_moveprobe.h"
#include "ai_hint.h"
#include "BasePropDoor.h"
#include "props.h"
#include "physics_npc_solver.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


const char * g_ppszGoalTypes[] =
{
	"GOALTYPE_NONE",
	"GOALTYPE_TARGETENT",
	"GOALTYPE_ENEMY",
	"GOALTYPE_PATHCORNER",
	"GOALTYPE_LOCATION",
	"GOALTYPE_LOCATION_NEAREST_NODE",
	"GOALTYPE_FLANK",
	"GOALTYPE_COVER",
};

#define AIGetGoalTypeText(type)	(g_ppszGoalTypes[(type)])

ConVar ai_vehicle_avoidance("ai_vehicle_avoidance", "1", FCVAR_CHEAT );

#ifdef DEBUG_AI_NAVIGATION
ConVar ai_debug_nav("ai_debug_nav", "0");
#endif

#ifdef DEBUG
ConVar ai_test_nav_failure_handling("ai_test_nav_failure_handling", "0");
int g_PathFailureCounter;
int g_MoveFailureCounter;
#define ShouldTestFailPath() ( ai_test_nav_failure_handling.GetBool() && g_PathFailureCounter++ % 20 == 0 )
#define ShouldTestFailMove() ( ai_test_nav_failure_handling.GetBool() && g_MoveFailureCounter++ % 100 == 0 )
#else
#define ShouldTestFailPath() (0)
#define ShouldTestFailMove() (0)
#endif

//-----------------------------------------------------------------------------

#ifdef DEBUG
bool g_fTestSteering = 0;
#endif

//-----------------------------------------------------------------------------

class CAI_NavInHintGroupFilter : public INearestNodeFilter
{
public:
	CAI_NavInHintGroupFilter( string_t iszGroup = NULL_STRING ) :
	  m_iszGroup( iszGroup )
	  {
	  }

	  bool IsValid( CAI_Node *pNode )
	  {
		  if ( !pNode->GetHint() )
		  {
			  return false;
		  }

		  if ( pNode->GetHint()->GetGroup() != m_iszGroup )
		  {
			  return false;
		  }

		  return true;
	  }

	  bool ShouldContinue()
	  {
		  return true;
	  }

	  string_t m_iszGroup;

};

//-----------------------------------------------------------------------------

const Vector AIN_NO_DEST( FLT_MAX, FLT_MAX, FLT_MAX );
#define NavVecToString(v) ((v == AIN_NO_DEST) ? "AIN_NO_DEST" : VecToString(v))

#define FLIER_CUT_CORNER_DIST		16 // 8 means the npc's bounding box is contained without the box of the node in WC

#define NAV_STOP_MOVING_TOLERANCE	6	// Goal tolerance for TASK_STOP_MOVING stopping paths

//-----------------------------------------------------------------------------
// class CAI_Navigator
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_Navigator )

	DEFINE_FIELD( m_navType,					FIELD_INTEGER ),
	//								m_pMotor
	//								m_pMoveProbe
	//								m_pLocalNavigator
	//								m_pAINetwork
	DEFINE_EMBEDDEDBYREF( m_pPath ),
	//								m_pClippedWaypoints	(not saved)
	//								m_flTimeClipped		(not saved)
	//								m_PreviousMoveActivity (not saved)
	//								m_PreviousArrivalActivity (not saved)
	DEFINE_FIELD( m_bValidateActivitySpeed,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCalledStartMove,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fNavComplete,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNotOnNetwork,				FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLastNavFailed,				FIELD_BOOLEAN ),
  	DEFINE_FIELD( m_flNextSimplifyTime,			FIELD_TIME ),
	DEFINE_FIELD( m_bForcedSimplify,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLastSuccessfulSimplifyTime, FIELD_TIME ),
	DEFINE_FIELD( m_flTimeLastAvoidanceTriangulate, FIELD_TIME ),
  	DEFINE_FIELD( m_timePathRebuildMax,			FIELD_FLOAT ),
  	DEFINE_FIELD( m_timePathRebuildDelay,		FIELD_FLOAT ),
  	DEFINE_FIELD( m_timePathRebuildFail,		FIELD_TIME ),
  	DEFINE_FIELD( m_timePathRebuildNext,		FIELD_TIME ),
	DEFINE_FIELD( m_fRememberStaleNodes,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNoPathcornerPathfinds,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bLocalSucceedOnWithinTolerance, FIELD_BOOLEAN ),
	// 								m_fPeerMoveWait		(think transient)
	//								m_hPeerWaitingOn	(peer move fields do not need to be saved, tied to current schedule and path, which are not saved)
	//								m_PeerWaitMoveTimer	(ibid)
	//								m_PeerWaitClearTimer(ibid)
	//								m_NextSidestepTimer	(ibid)
	DEFINE_FIELD( m_hBigStepGroundEnt,			FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLastBlockingEnt,			FIELD_EHANDLE ),
	//								m_vPosBeginFailedSteer (reset on load)
	//								m_timeBeginFailedSteer (reset on load)
	//								m_nNavFailCounter  (reset on load)
	//								m_flLastNavFailTime  (reset on load)

END_DATADESC()


//-----------------------------------------------------------------------------

CAI_Navigator::CAI_Navigator(CAI_BaseNPC *pOuter)
 :	BaseClass(pOuter)
{
	m_pPath					= new CAI_Path;
	m_pAINetwork			= NULL;
	m_bNotOnNetwork			= false;
	m_flNextSimplifyTime	= 0;

	m_flLastSuccessfulSimplifyTime = -1;

	m_pClippedWaypoints		= new CAI_WaypointList;
	m_flTimeClipped			= -1;

	m_bValidateActivitySpeed = true;
	m_bCalledStartMove		= false;

	// ----------------------------
	
	m_navType = NAV_GROUND;
	m_fNavComplete = false;
	m_bLastNavFailed = false;
	
	// ----------------------------

	m_PeerWaitMoveTimer.Set(0.25); // 2 thinks
	m_PeerWaitClearTimer.Set(3.0);
	m_NextSidestepTimer.Set(5.0);

	m_vPosBeginFailedSteer = vec3_invalid;
	m_timeBeginFailedSteer = FLT_MAX;

	m_flTimeLastAvoidanceTriangulate = -1;

	// ----------------------------

	m_bNoPathcornerPathfinds = false;
	m_bLocalSucceedOnWithinTolerance = false;

	m_fRememberStaleNodes = true;
	
	m_pMotor = NULL;
	m_pMoveProbe = NULL;
	m_pLocalNavigator = NULL;


	m_nNavFailCounter	= 0;
	m_flLastNavFailTime = -1;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::Init( CAI_Network *pNetwork )
{
	m_pMotor = GetOuter()->GetMotor();
	m_pMoveProbe = GetOuter()->GetMoveProbe();
	m_pLocalNavigator = GetOuter()->GetLocalNavigator();
	m_pAINetwork = pNetwork;

}

//-----------------------------------------------------------------------------

CAI_Navigator::~CAI_Navigator()
{
	delete m_pPath;
	m_pClippedWaypoints->RemoveAll();
	delete m_pClippedWaypoints;
}

//-----------------------------------------------------------------------------

const short AI_NAVIGATOR_SAVE_VERSION = 1;

void CAI_Navigator::Save( ISave &save )
{
	save.WriteShort( &AI_NAVIGATOR_SAVE_VERSION );

	CUtlVector<AI_Waypoint_t> minPathArray;

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	if ( pCurWaypoint )
	{
		if ( ( pCurWaypoint->NavType() == NAV_CLIMB || pCurWaypoint->NavType() == NAV_JUMP ) )
		{	
			CAI_WaypointList minCompletionPath;
			if ( GetStoppingPath( &minCompletionPath ) && !minCompletionPath.IsEmpty() )
			{
				AI_Waypoint_t *pCurrent = minCompletionPath.GetLast();
				while ( pCurrent )
				{
					minPathArray.AddToTail( *pCurrent );
					pCurrent = pCurrent->GetPrev();
				}
				minCompletionPath.RemoveAll();
			}
		}
	}

	SaveUtlVector( &save, &minPathArray, FIELD_EMBEDDED );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::Restore( IRestore &restore )
{
	short version = restore.ReadShort();

	if ( version != AI_NAVIGATOR_SAVE_VERSION )
		return;

	CUtlVector<AI_Waypoint_t> minPathArray;
	RestoreUtlVector( &restore, &minPathArray, FIELD_EMBEDDED );

	if ( minPathArray.Count() )
	{
		for ( int i = 0; i < minPathArray.Count(); i++ )
		{
			m_pClippedWaypoints->PrependWaypoint( minPathArray[i].GetPos(), minPathArray[i].NavType(), ( minPathArray[i].Flags() & ~bits_WP_TO_PATHCORNER ), minPathArray[i].flYaw );
		}
		m_flTimeClipped = gpGlobals->curtime + 1000; // time passes between restore and onrestore
	}

}

//-----------------------------------------------------------------------------

bool CAI_Navigator::ActivityIsLocomotive( Activity activity ) 
{
	// FIXME: should be calling HasMovement() for a sequence
	return ( activity > ACT_IDLE ); 
}

//-----------------------------------------------------------------------------

CAI_Pathfinder *CAI_Navigator::GetPathfinder()
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------

const CAI_Pathfinder *CAI_Navigator::GetPathfinder() const
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Navigator::GetNavTargetEntity()
{
	if ( GetGoalType() == GOALTYPE_ENEMY || GetGoalType() == GOALTYPE_TARGETENT )
		return GetOuter()->GetNavTargetEntity();
	return GetPath()->GetTarget();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::TaskMovementComplete()
{
	GetOuter()->TaskMovementComplete();
}

//-----------------------------------------------------------------------------

float CAI_Navigator::MaxYawSpeed()
{
	return GetOuter()->MaxYawSpeed();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetSpeed( float fl )
{
	GetOuter()->m_flSpeed = fl;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::FindPath( const AI_NavGoal_t &goal, unsigned flags )
{
	CAI_Path *pPath = GetPath();

	MARK_TASK_EXPENSIVE();

	// Clear out previous state
	if ( flags & AIN_CLEAR_PREVIOUS_STATE )
		pPath->Clear();
	else if ( flags & AIN_CLEAR_TARGET )
		pPath->ClearTarget();

	// Set the activity
	if ( goal.activity != AIN_DEF_ACTIVITY )
		pPath->SetMovementActivity( goal.activity );
	else if ( pPath->GetMovementActivity() == ACT_INVALID )
		pPath->SetMovementActivity( ( GetOuter()->GetState() == NPC_STATE_COMBAT ) ? ACT_RUN : ACT_WALK );
		
	// Set the tolerance
	if ( goal.tolerance == AIN_HULL_TOLERANCE )
		pPath->SetGoalTolerance( GetHullWidth() );
	else if ( goal.tolerance != AIN_DEF_TOLERANCE )
		pPath->SetGoalTolerance( goal.tolerance );
	else if (pPath->GetGoalTolerance() == 0 )
		pPath->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );

	if (pPath->GetGoalTolerance() < 0.1 )
		DevMsg( GetOuter(), "Suspicious navigation goal tolerance specified\n");

	pPath->SetWaypointTolerance( GetHullWidth() * 0.5 );

	pPath->SetGoalType( GOALTYPE_NONE ); // avoids a spurious warning about setting the goal type twice
	pPath->SetGoalType( goal.type );
	pPath->SetGoalFlags( goal.flags );
		
	CBaseEntity *pPathTarget = goal.pTarget;
	if ((goal.type == GOALTYPE_TARGETENT) || (goal.type == GOALTYPE_ENEMY))
	{
		// Guarantee that the path target 
		if (goal.type == GOALTYPE_TARGETENT)
			pPathTarget = GetTarget(); 
		else
			pPathTarget = GetEnemy();

		Assert( goal.pTarget == AIN_DEF_TARGET || goal.pTarget == pPathTarget );

		// Set the goal offset
		if ( goal.dest != AIN_NO_DEST )
			pPath->SetTargetOffset( goal.dest );

		// We're not setting the goal position here because
		// it's going to be computed + set in DoFindPath.
	}
	else
	{
		// When our goaltype is position based, we have to set
		// the goal position here because it won't get set during DoFindPath().
		if ( goal.dest != AIN_NO_DEST )
			pPath->ResetGoalPosition( goal.dest );
		else if ( goal.destNode != AIN_NO_NODE )
			pPath->ResetGoalPosition( GetNodePos( goal.destNode ) );
	}

	if ( pPathTarget > AIN_DEF_TARGET )
	{
		pPath->SetTarget( pPathTarget );
	}

	pPath->ClearWaypoints();
	bool result = FindPath( ( flags & AIN_NO_PATH_TASK_FAIL ) == 0 );

	if ( result == false )
	{
		if ( flags & AIN_DISCARD_IF_FAIL )
			pPath->Clear();
		else
			pPath->SetGoalType( GOALTYPE_NONE );
	}
	else
	{
		if ( goal.arrivalActivity != AIN_DEF_ACTIVITY && goal.arrivalActivity > ACT_RESET )
		{
			pPath->SetArrivalActivity( goal.arrivalActivity );
		}
		else if ( goal.arrivalSequence != -1 )
		{
			pPath->SetArrivalSequence( goal.arrivalSequence );
		}

		// Initialize goal facing direction
		// FIXME: This is a poor way to initialize the arrival direction, apps calling SetGoal() 
		//		  should do this themselves, and/or it should be part of AI_NavGoal_t
		// FIXME: A number of calls to SetGoal() building routes to their enemy but don't set the flags!
		if (goal.type == GOALTYPE_ENEMY)
		{
			pPath->SetGoalDirection( pPathTarget );
			pPath->SetGoalSpeed( pPathTarget );
		}
		else
		{
			pPath->SetGoalDirection( pPath->ActualGoalPosition() - GetAbsOrigin() );
		}
	}
	
	return result;
}

ConVar ai_navigator_generate_spikes( "ai_navigator_generate_spikes", "0" );
ConVar ai_navigator_generate_spikes_strength( "ai_navigator_generate_spikes_strength", "8" );

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetGoal( const AI_NavGoal_t &goal, unsigned flags )
{
	// Queue this up if we're in the middle of a frame
	if ( PostFrameNavigationSystem()->IsGameFrameRunning() )
	{
		// Send off the query for queuing
		PostFrameNavigationSystem()->EnqueueEntityNavigationQuery( GetOuter(), CreateFunctor( this, &CAI_Navigator::SetGoal, RefToVal( goal ), flags ) );

		// Complete immediately if we're waiting on that
		// FIXME: This will probably cause a lot of subtle little nuisances...
		if ( ( flags & AIN_NO_PATH_TASK_FAIL ) == 0 || GetOuter()->IsCurTaskContinuousMove() )
		{
			TaskComplete();
		}

		// For now, always succeed -- we need to deal with failures on the next frame
		return true;
	}

	CAI_Path *pPath = GetPath();

	OnNewGoal();
		
	// Clear out previous state
	if ( flags & AIN_CLEAR_PREVIOUS_STATE )
		ClearPath();

	if ( GetOuter()->IsCurTaskContinuousMove() || ai_post_frame_navigation.GetBool() )
		flags |= AIN_NO_PATH_TASK_FAIL;

	bool result = FindPath( goal, flags );
	
	if ( result == false )
	{
		DbgNavMsg(  GetOuter(), "Failed to pathfind to nav goal:\n" );
		DbgNavMsg1( GetOuter(), "   Type:      %s\n", AIGetGoalTypeText( goal.type) );
		DbgNavMsg1( GetOuter(), "   Dest:      %s\n", NavVecToString( goal.dest ) );
		DbgNavMsg1( GetOuter(), "   Dest node: %p\n", goal.destNode );
		DbgNavMsg1( GetOuter(), "   Target:    %p\n", goal.pTarget );

		if ( flags & AIN_DISCARD_IF_FAIL )
			ClearPath();
	}
	else
	{
		DbgNavMsg(  GetOuter(), "New goal set:\n" );
		DbgNavMsg1( GetOuter(), "   Type:         %s\n", AIGetGoalTypeText( goal.type) );
		DbgNavMsg1( GetOuter(), "   Dest:         %s\n", NavVecToString( goal.dest ) );
		DbgNavMsg1( GetOuter(), "   Dest node:    %p\n", goal.destNode );
		DbgNavMsg1( GetOuter(), "   Target:       %p\n", goal.pTarget );
		DbgNavMsg1( GetOuter(), "   Tolerance:    %.1f\n", GetPath()->GetGoalTolerance() );
		DbgNavMsg1( GetOuter(), "   Waypoint tol: %.1f\n", GetPath()->GetWaypointTolerance() );
		DbgNavMsg1( GetOuter(), "   Activity:     %s\n", GetOuter()->GetActivityName(GetPath()->GetMovementActivity()) );
		DbgNavMsg1( GetOuter(), "   Arrival act:  %s\n", GetOuter()->GetActivityName(GetPath()->GetArrivalActivity()) );
		DbgNavMsg1( GetOuter(), "   Arrival seq:  %d\n", GetPath()->GetArrivalSequence() );
		DbgNavMsg1( GetOuter(), "   Goal dir:     %s\n", NavVecToString( GetPath()->GetGoalDirection(GetAbsOrigin())) );
	
		// Set our ideal yaw. This has to be done *after* finding the path, 
		// because the goal position may not be set until then
		if ( goal.flags & AIN_YAW_TO_DEST )
		{
			DbgNavMsg( GetOuter(), "   Yaw to dest\n" );
			GetMotor()->SetIdealYawToTarget( pPath->ActualGoalPosition() );
		}
		
		SimplifyPath( true, goal.maxInitialSimplificationDist );
	}
	
	return result;
}


//-----------------------------------------------------------------------------
// Change the target of the path
//-----------------------------------------------------------------------------
bool CAI_Navigator::SetGoalTarget( CBaseEntity *pEntity, const Vector &offset )
{
	OnNewGoal();
	CAI_Path *pPath = GetPath();
	pPath->SetTargetOffset( offset );
	pPath->SetTarget( pEntity );
	pPath->ClearWaypoints();
	return FindPath( !GetOuter()->IsCurTaskContinuousMove() );
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRadialGoal( const Vector &destination, const Vector &center, float radius, float arc, float stepDist, bool bClockwise, bool bAirRoute)
{
	DbgNavMsg( GetOuter(), "Set radial goal\n" );
	OnNewGoal();
	GetPath()->SetGoalType(GOALTYPE_LOCATION);

	GetPath()->SetWaypoints( GetPathfinder()->BuildRadialRoute( GetLocalOrigin(), center, destination, radius, arc, stepDist, bClockwise, GetPath()->GetGoalTolerance(), bAirRoute ), true);			
	GetPath()->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );

	return IsGoalActive();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( const Vector &from, float minPathLength, const Vector &dir )
{
	DbgNavMsg( GetOuter(), "Set random goal\n" );
	OnNewGoal();
	if ( GetNetwork()->NumNodes() <= 0 )
		return false;

	INearestNodeFilter *pFilter = NULL;
	CAI_NavInHintGroupFilter filter;
	if ( GetOuter()->GetHintGroup() != NULL_STRING )
	{
		filter.m_iszGroup = GetOuter()->GetHintGroup();
		pFilter = &filter;
	}
		
	int fromNodeID = GetNetwork()->NearestNodeToPoint( GetOuter(), from, true, pFilter );
	
	if (fromNodeID == NO_NODE)
		return false;
		
	AI_Waypoint_t* pRoute = GetPathfinder()->FindShortRandomPath( fromNodeID, minPathLength, dir );

	if (!pRoute)
		return false;

	GetPath()->SetGoalType(GOALTYPE_LOCATION);
	GetPath()->SetWaypoints(pRoute);
	GetPath()->SetLastNodeAsGoal();
	GetPath()->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );

	SimplifyPath( true );
	
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetDirectGoal( const Vector &goalPos, Navigation_t navType )
{
	DbgNavMsg( GetOuter(), "Set direct goal\n" );
	OnNewGoal();
	ClearPath();
	GetPath()->SetGoalType( GOALTYPE_LOCATION );
	GetPath()->SetWaypoints( new AI_Waypoint_t( goalPos, 0, navType, bits_WP_TO_GOAL, NO_NODE ) );
	GetPath()->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );
	GetPath()->SetGoalPosition( goalPos );
	return true;
}

//-----------------------------------------------------------------------------
// Placeholder implementation for wander goals: cast a few random vectors and
// accept the first one that still lies on the navmesh.
// Side effect: vector goal of navigator is set.
// Returns: true on goal set, false otherwise.
static bool SetWanderGoalByRandomVector(CAI_Navigator *pNav, float minRadius, float maxRadius, int numTries)
{
	while (--numTries >= 0)
	{
		float dist = random->RandomFloat( minRadius, maxRadius );
		Vector dir = UTIL_YawToVector( random->RandomFloat( 0, 359.99 ) );

		if ( pNav->SetVectorGoal( dir, dist, minRadius ) )
			return true;
	}

	return false;
}

bool CAI_Navigator::SetWanderGoal( float minRadius, float maxRadius )
{
	// @Note (toml 11-07-02): this is a bogus placeholder implementation!!!
	// 
	// First try using a random setvector goal, and then try SetRandomGoal().
	// Except, if we have a hint group, first try SetRandomGoal() (which 
	// respects hint groups) and then fall back on the setvector.
	if( !GetOuter()->GetHintGroup() )
	{
		return ( SetWanderGoalByRandomVector( this, minRadius, maxRadius, 5 ) || 
			SetRandomGoal( 1 ) );
	}
	else
	{
		return ( SetRandomGoal(1) ||
			SetWanderGoalByRandomVector( this, minRadius, maxRadius, 5 ) );
	}
}


//-----------------------------------------------------------------------------

void CAI_Navigator::CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult )
{
	Vector temp;
	CrossProduct( dir, normal, temp );
	CrossProduct( normal, temp, *pResult );
	VectorNormalize( *pResult );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoal( const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	DbgNavMsg( GetOuter(), "Set vector goal\n" );

	Vector result;

	if ( FindVectorGoal( &result, dir, targetDist, minDist, fShouldDeflect ) )
		return SetGoal( result );
	
	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoalFromTarget( const Vector &goalPos, float minDist, bool fShouldDeflect )
{
	Vector vDir = goalPos;
	float dist = ComputePathDirection( GetNavType(), GetLocalOrigin(), goalPos, &vDir );
	return SetVectorGoal( vDir, dist, minDist, fShouldDeflect );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::FindVectorGoal( Vector *pResult, const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	AIMoveTrace_t moveTrace;
	float distAchieved = 0;
	
	MARK_TASK_EXPENSIVE();

	Vector testLoc = GetLocalOrigin() + ( dir * targetDist );
	GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
	
	if ( moveTrace.fStatus != AIMR_OK )
	{
		distAchieved = targetDist - moveTrace.flDistObstructed;
		if ( fShouldDeflect && moveTrace.vHitNormal != vec3_origin )
		{
			Vector vecDeflect;
			Vector vecNormal = moveTrace.vHitNormal;
			if ( GetNavType() == NAV_GROUND )
				vecNormal.z = 0;

			CalculateDeflection( moveTrace.vEndPosition, dir, vecNormal, &vecDeflect );

			testLoc = moveTrace.vEndPosition + ( vecDeflect * ( targetDist - distAchieved ) );
	
			Vector temp = moveTrace.vEndPosition;
			GetMoveProbe()->MoveLimit( GetNavType(), temp, testLoc, MASK_NPCSOLID, NULL, &moveTrace );

			distAchieved += ( targetDist - distAchieved ) - moveTrace.flDistObstructed;
		}
		
		if ( distAchieved < minDist + 0.01 )
			return false;
	}

	*pResult = moveTrace.vEndPosition;
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( float minPathLength, const Vector &dir )
{
	return SetRandomGoal( GetLocalOrigin(), minPathLength, dir );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::PrependLocalAvoidance( float distObstacle, const AIMoveTrace_t &directTrace )
{
	if ( AIStrongOpt() )
		return false;

	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	if ( m_flTimeLastAvoidanceTriangulate >= gpGlobals->curtime )
		return false; // Only triangulate once per think at most

	m_flTimeLastAvoidanceTriangulate = gpGlobals->curtime;

	AI_PROFILE_SCOPE(CAI_Navigator_PrependLocalAvoidance);

	AI_Waypoint_t *pAvoidanceRoute = NULL;

	Vector vStart = GetLocalOrigin();

	if ( distObstacle < GetHullWidth() * 0.5 )
	{
		AIMoveTrace_t backawayTrace;
		Vector vTestBackaway = GetCurWaypointPos() - GetLocalOrigin();
		VectorNormalize( vTestBackaway );
		vTestBackaway *= -GetHullWidth();
		vTestBackaway += GetLocalOrigin();

		int flags = ( GetNavType() == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT;

		if ( GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), vTestBackaway, 
													 MASK_NPCSOLID, GetNavTargetEntity(), 
													 100.0, 
													 flags, &backawayTrace ) )
		{
			vStart = backawayTrace.vEndPosition;
			pAvoidanceRoute = new AI_Waypoint_t( vStart, 0, GetNavType(), bits_WP_TO_DETOUR, NO_NODE );
		}
	}

	AI_Waypoint_t *pTriangulation = GetPathfinder()->BuildTriangulationRoute(
		vStart, 
		GetCurWaypointPos(), 
		GetNavTargetEntity(), 
		bits_WP_TO_DETOUR, 
		NO_NODE,
		0.0, 
		distObstacle, 
		GetNavType() );
	
	if ( !pTriangulation )
	{
		delete pAvoidanceRoute;
		return false;
	}

	if ( pAvoidanceRoute )
		pAvoidanceRoute->SetNext( pTriangulation );
	else
		pAvoidanceRoute = pTriangulation;

	// @TODO (toml 02-04-04): it would be better to do this on each triangulation test to
	// improve the odds of success. however, difficult in current structure
	float moveThisInterval = GetMotor()->CalcIntervalMove();
	Vector dir = pAvoidanceRoute->GetPos() - GetLocalOrigin();
	float dist = VectorNormalize( dir );
	Vector testPos;
	if ( dist > moveThisInterval )
	{
		dist = moveThisInterval;
		testPos = GetLocalOrigin() + dir * dist;
	}
	else
	{
		testPos = pAvoidanceRoute->GetPos();
	}

	int flags = ( GetNavType() == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT;

	if ( !GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testPos, 
												 MASK_NPCSOLID, GetNavTargetEntity(), 
												 100.0, 
												 flags ) )
	{
		DeleteAll( pAvoidanceRoute );
		return false;
	}
		
	// FIXME: should the route get simplified? 
	DbgNavMsg( GetOuter(), "Adding triangulation\n" );
	GetPath()->PrependWaypoints( pAvoidanceRoute );
	return true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags )
{
	GetPath()->PrependWaypoint( newPoint, navType, waypointFlags );
}

//-----------------------------------------------------------------------------

const Vector &CAI_Navigator::GetGoalPos() const
{ 
	return GetPath()->BaseGoalPosition(); 
}

//-----------------------------------------------------------------------------

CBaseEntity *CAI_Navigator::GetGoalTarget()
{
	return GetPath()->GetTarget();
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetGoalTolerance() const
{
	return GetPath()->GetGoalTolerance(); 
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetGoalTolerance(float tolerance)
{
	GetPath()->SetGoalTolerance(tolerance);
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::RefindPathToGoal( bool fSignalTaskStatus, bool bDontIgnoreBadLinks )
{
	return FindPath( fSignalTaskStatus, bDontIgnoreBadLinks );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::UpdateGoalPos( const Vector &goalPos )
{
	// Queue this up if we're in the middle of a frame
	if ( PostFrameNavigationSystem()->IsGameFrameRunning() )
	{
		// Send off the query for queuing
		PostFrameNavigationSystem()->EnqueueEntityNavigationQuery( GetOuter(), CreateFunctor( this, &CAI_Navigator::UpdateGoalPos, RefToVal( goalPos ) ) );

		// For now, always succeed -- we need to deal with failures on the next frame
		return true;
	}

	DbgNavMsg( GetOuter(), "Updating goal pos\n" );

	if ( GetNavType() == NAV_JUMP )
	{
		DevMsg( "Updating goal pos while jumping!\n" );
		AssertOnce( 0 );
		return false;
	}

	// FindPath should be finding the goal position if the goal type is
	// one of these two... We could just ignore the suggested position
	// in these two cases I suppose!
	Assert( (GetPath()->GoalType() != GOALTYPE_ENEMY) && (GetPath()->GoalType() != GOALTYPE_TARGETENT) );

	GetPath()->ResetGoalPosition( goalPos );
	if ( FindPath( !GetOuter()->IsCurTaskContinuousMove() ) )
	{
		SimplifyPath( true );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::SetMovementActivity(Activity activity)
{
	return GetPath()->SetMovementActivity( activity );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::StopMoving( bool bImmediate )
{
	DbgNavMsg1( GetOuter(), "CAI_Navigator::StopMoving( %d )\n", bImmediate );
	if ( IsGoalSet() )
	{
		if ( bImmediate || !SetGoalFromStoppingPath() )
		{
			OnNavComplete();
		}
	}
	else
		ClearGoal();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::ClearGoal()
{ 
	DbgNavMsg( GetOuter(), "CAI_Navigator::ClearGoal()\n" );
	ClearPath();
	OnNewGoal();
	return true; 
}

//-----------------------------------------------------------------------------

int CAI_Navigator::GetMovementSequence( )
{
	int sequence = GetPath()->GetMovementSequence( );
	if (sequence == ACT_INVALID)
	{
		Activity activity = GetPath()->GetMovementActivity();
		Assert( activity != ACT_INVALID );

		sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ) );
		if ( sequence == ACT_INVALID )
		{
			DevMsg( GetOuter(), "No appropriate sequence for movement activity %s (%d)\n", GetOuter()->GetActivityName( GetPath()->GetArrivalActivity() ), GetPath()->GetArrivalActivity() );

			if ( activity == ACT_SCRIPT_CUSTOM_MOVE )
			{
				sequence = GetOuter()->GetScriptCustomMoveSequence();
			}
			else
			{
				sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( ACT_WALK ) );
			}
		}
		Assert( sequence != ACT_INVALID );
		GetPath()->SetMovementSequence( sequence );
	}
	return sequence;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetMovementSequence( int sequence )
{
	GetPath()->SetMovementSequence( sequence );
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetMovementActivity() const
{
	return GetPath()->GetMovementActivity();
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalActivity(Activity activity)
{
	GetPath()->SetArrivalActivity(activity);
}


//-----------------------------------------------------------------------------

int CAI_Navigator::GetArrivalSequence( int curSequence )
{
	int sequence = GetPath()->GetArrivalSequence( );
	if (sequence == ACT_INVALID)
	{
		Activity activity = GetOuter()->GetStoppedActivity();

		Assert( activity != ACT_INVALID );
		if (activity == ACT_INVALID)
		{
			activity = ACT_IDLE;
		}

		sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ), curSequence );

		if ( sequence == ACT_INVALID )
		{
			DevMsg( GetOuter(), "No appropriate sequence for arrival activity %s (%d)\n", GetOuter()->GetActivityName( GetPath()->GetArrivalActivity() ), GetPath()->GetArrivalActivity() );
			sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( ACT_IDLE ), curSequence );
		}
		Assert( sequence != ACT_INVALID );
		GetPath()->SetArrivalSequence( sequence );
	}
	return sequence;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalSequence( int sequence )
{
	GetPath()->SetArrivalActivity( ACT_INVALID );
	GetPath()->SetArrivalSequence( sequence );
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetArrivalActivity( ) const
{
	return GetPath()->GetArrivalActivity( );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( const Vector &goalDirection )
{ 
	GetPath()->SetGoalDirection( goalDirection );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( const QAngle &goalAngle ) 
{ 
	Vector goalDirection;

	AngleVectors( goalAngle, &goalDirection );

	GetPath()->SetGoalDirection( goalDirection );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDirection( CBaseEntity * pTarget )
{
	GetPath()->SetGoalDirection( pTarget );
}

//-----------------------------------------------------------------------------

Vector CAI_Navigator::GetArrivalDirection( )
{
	return GetPath()->GetGoalDirection( GetAbsOrigin() );
}


//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalSpeed( float flSpeed )
{
	GetPath()->SetGoalSpeed( flSpeed );
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetArrivalSpeed( void )
{ 
	float flSpeed = GetPath()->GetGoalSpeed( GetAbsOrigin() );

	if (flSpeed >= 0.0)
	{
		return flSpeed;
	}

	int sequence = GetArrivalSequence( ACT_INVALID );

	if (sequence != ACT_INVALID)
	{
		flSpeed = GetOuter()->GetEntryVelocity( sequence );
		SetArrivalSpeed( flSpeed );
	}
	else
	{
		flSpeed = 0.0;
	}

	return flSpeed;
}


//-----------------------------------------------------------------------------

void CAI_Navigator::SetArrivalDistance( float flDistance )
{
	GetPath()->SetGoalStoppingDistance( flDistance );
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetArrivalDistance() const
{
	return GetPath()->GetGoalStoppingDistance();
}

//-----------------------------------------------------------------------------

const Vector &CAI_Navigator::GetCurWaypointPos() const
{
	return GetPath()->CurWaypointPos();
}

//-----------------------------------------------------------------------------

int CAI_Navigator::GetCurWaypointFlags() const
{
	return GetPath()->CurWaypointFlags();
}

//-----------------------------------------------------------------------------

GoalType_t CAI_Navigator::GetGoalType() const
{
	return GetPath()->GoalType();
}

//-----------------------------------------------------------------------------

int	CAI_Navigator::GetGoalFlags() const
{
	return GetPath()->GoalFlags();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::CurWaypointIsGoal() const
{
	return GetPath()->CurWaypointIsGoal();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::IsGoalSet() const
{
	return ( GetPath()->GoalType() != GOALTYPE_NONE );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::IsGoalActive() const
{
	return ( GetPath() && !( const_cast<CAI_Path *>(GetPath())->IsEmpty() ) );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::GetPointAlongPath( Vector *pResult, float distance, bool fReducibleOnly )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	AI_Waypoint_t *pCurWaypoint	 = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pEndPoint 	 = pCurWaypoint;
	float 		   distRemaining = distance;
	float 		   distToNext;
	Vector		   vPosPrev		 = GetLocalOrigin();

	while ( pEndPoint->GetNext() )
	{
		distToNext = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() );
		
		if ( distToNext > distRemaining)
			break;
		
		distRemaining -= distToNext;
		vPosPrev = pEndPoint->GetPos();
		if ( fReducibleOnly && !pEndPoint->IsReducible() )
			break;
		pEndPoint = pEndPoint->GetNext();
	}
	
	Vector &result = *pResult;
	float distToEnd = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() ); 
	if ( distToEnd - distRemaining < 0.1 )
	{
		result = pEndPoint->GetPos();
	}
	else
	{
		result = pEndPoint->GetPos() - vPosPrev;
		VectorNormalize( result );
		result *= distRemaining;
		result += vPosPrev;
	}

	return true;
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathDistanceToGoal()
{
	return GetPath()->GetPathDistanceToGoal(GetAbsOrigin());
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathTimeToGoal()
{
	if ( GetOuter()->m_flGroundSpeed )
		return (GetPathDistanceToGoal() / GetOuter()->m_flGroundSpeed);
	return 0;
}

//-----------------------------------------------------------------------------

AI_PathNode_t CAI_Navigator::GetNearestNode()
{
#ifdef WIN32
	COMPILE_TIME_ASSERT( (intp)AIN_NO_NODE == (intp)NO_NODE );
#endif
	return (AI_PathNode_t)(intp)( GetPathfinder()->NearestNodeToNPC() );
}

//-----------------------------------------------------------------------------

Vector CAI_Navigator::GetNodePos( AI_PathNode_t node )
{
	return GetNetwork()->GetNode((intp)node)->GetPosition(GetHullType());
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnScheduleChange()						
{ 
	DbgNavMsg( GetOuter(), "Schedule change\n" );
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnClearPath(void) 
{
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNewGoal()
{
	DbgNavMsg( GetOuter(), "New Goal\n" );
	ResetCalculations();
	m_fNavComplete = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavComplete()
{
	DbgNavMsg( GetOuter(), "Nav complete\n" );
	ResetCalculations();
	TaskMovementComplete();
	m_fNavComplete = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( bool bMovement )
{
	DbgNavMsg( GetOuter(), "Nav failed\n" );
	if ( bMovement )
		GetOuter()->OnMovementFailed();

#ifdef DEBUG
	if ( CurWaypointIsGoal() )
	{
		float flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->ActualGoalPosition() );
		if ( flWaypointDist < GetGoalTolerance() + 0.1 )
		{
			DevMsg( "Nav failed but NPC was within goal tolerance?\n" );
		}
	}
#endif

	ResetCalculations();
	m_fNavComplete = true;
	m_bLastNavFailed = true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( AI_TaskFailureCode_t code, bool bMovement )
{
	if ( GetOuter()->ShouldFailNav( bMovement ) )
	{
		OnNavFailed( bMovement );
		SetActivity( GetOuter()->GetStoppedActivity() );
		TaskFail(code);
	}
	else
	{
		m_nNavFailCounter++;
		m_flLastNavFailTime = gpGlobals->curtime;
		if ( GetOuter()->ShouldBruteForceFailedNav() )
		{
			if (bMovement)
			{

				m_timeBeginFailedSteer = FLT_MAX;

				// if failing, turn off collisions with the object
				CBaseEntity *pBlocker = GetBlockingEntity();
				// FIXME: change this to only be for MOVETYPE_VPHYSICS?
				if (pBlocker && !pBlocker->IsWorld() && !pBlocker->IsPlayer() && !FClassnameIs( pBlocker, "func_tracktrain" ))
				{
					//pBlocker->DrawBBoxOverlay( 2.0f );
					if (NPCPhysics_CreateSolver( GetOuter(), pBlocker, true, 10.0f ) != NULL)
					{
						ClearNavFailCounter();
					}
				}

				// if still failing, try jumping forward through the route
				if (GetNavFailCounter() > 0)
				{
					if (TeleportAlongPath())
					{
						ClearNavFailCounter();
					}
				}
			}
			else
			{
				CBaseEntity *pBlocker = GetMoveProbe()->GetBlockingEntity();
				if (pBlocker)
				{
					//pBlocker->DrawBBoxOverlay( 2.0f );
					if (NPCPhysics_CreateSolver( GetOuter(), pBlocker, true, 10.0f ) != NULL)
					{
						ClearNavFailCounter();
					}
				}
			}
		}
	}

}
//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( const char *pszGeneralFailText, bool bMovement )	
{ 
	OnNavFailed( MakeFailCode( pszGeneralFailText ), bMovement );
}

//-----------------------------------------------------------------------------

int	CAI_Navigator::GetNavFailCounter() const
{
	return m_nNavFailCounter;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ClearNavFailCounter()
{
	m_nNavFailCounter = 0;
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetLastNavFailTime() const
{
	return m_flLastNavFailTime;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::TeleportAlongPath()
{
	while (GetPath()->GetCurWaypoint())
	{
		Vector vecStart;
		Vector vTestPoint;

		vecStart = GetPath()->CurWaypointPos();
		AdvancePath();

		GetOuter()->GetMoveProbe()->FloorPoint( vecStart, MASK_NPCSOLID, GetOuter()->StepHeight(), -64, &vTestPoint );

		if ( CanFitAtPosition( vTestPoint, MASK_NPCSOLID, false, false ) )
		{
			if ( GetOuter()->GetMoveProbe()->CheckStandPosition( vTestPoint, MASK_NPCSOLID ) )
			{
				GetOuter()->Teleport( &vTestPoint, NULL, NULL );
				// clear ground entity, let normal fall code reestablish what the npc is now standing on
				GetOuter()->SetGroundEntity( NULL );
				GetOuter()->PhysicsTouchTriggers( &vTestPoint );
				return true;
			}
		}

		if (CurWaypointIsGoal())
			break;
	}
	return false;
}


//-----------------------------------------------------------------------------

void CAI_Navigator::ResetCalculations()
{
	m_hPeerWaitingOn = NULL;
	m_PeerWaitMoveTimer.Force();
	m_PeerWaitClearTimer.Force();

	m_hBigStepGroundEnt = NULL;

	m_NextSidestepTimer.Force();

	m_bCalledStartMove = false;

	m_vPosBeginFailedSteer = vec3_invalid;
	m_timeBeginFailedSteer = FLT_MAX;

	m_flLastSuccessfulSimplifyTime = -1;

	GetLocalNavigator()->ResetMoveCalculations();
	GetMotor()->ResetMoveCalculations();
	GetMoveProbe()->ClearBlockingEntity();

	m_nNavFailCounter = 0;
	m_flLastNavFailTime = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Sets navigation type, maintaining any necessary invariants
//-----------------------------------------------------------------------------
void CAI_Navigator::SetNavType( Navigation_t navType )
{ 
	m_navType = navType;
}


//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Navigator::MoveClimb() 
{
	// --------------------------------------------------
	//  CLIMB START
	// --------------------------------------------------
	const Vector &climbDest = GetPath()->CurWaypointPos();
	Vector climbDir = climbDest - GetLocalOrigin();
	float climbDist = VectorNormalize( climbDir );

	if ( GetNavType() != NAV_CLIMB )
	{
		DbgNavMsg( GetOuter(), "Climb start\n" );
		GetMotor()->MoveClimbStart( climbDest, climbDir, climbDist, GetPath()->CurWaypointYaw() );
	}

	SetNavType( NAV_CLIMB );

	// Look for a block by another NPC, and attempt to recover
	AIMoveTrace_t moveTrace;
	if ( climbDist > 0.01 &&
		 !GetMoveProbe()->MoveLimit( NAV_CLIMB, GetLocalOrigin(), GetLocalOrigin() + ( climbDir * MIN(0.1,climbDist - 0.005) ), MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace ) )
	{
		CAI_BaseNPC *pOther = ( moveTrace.pObstruction ) ? moveTrace.pObstruction->MyNPCPointer() : NULL;
		if ( pOther )
		{
			bool bAbort = false;

			if ( !pOther->IsMoving() )
				bAbort = true;
			else if ( pOther->GetNavType() == NAV_CLIMB && climbDir.z <= 0.01 )
			{
				const Vector &otherClimbDest = pOther->GetNavigator()->GetPath()->CurWaypointPos();
				Vector otherClimbDir = otherClimbDest - pOther->GetLocalOrigin();
				VectorNormalize( otherClimbDir );

				if ( otherClimbDir.Dot( climbDir ) < 0 )
				{
					bAbort = true;
					if ( pOther->GetNavigator()->GetStoppingPath( m_pClippedWaypoints ) )
					{
						m_flTimeClipped = gpGlobals->curtime;
						SetNavType(NAV_GROUND); // end of clipped will be on ground
						SetGravity( 1.0 );
						if ( RefindPathToGoal( false ) )
						{
							bAbort = false;
						}
						SetGravity( 0.0 );
						SetNavType(NAV_CLIMB);
					}
				}
			}

			if ( bAbort )
			{
				DbgNavMsg( GetOuter(), "Climb fail\n" );
				GetMotor()->MoveClimbStop();
				SetNavType(NAV_GROUND);
				return AIMR_BLOCKED_NPC;
			}
		}
	}

	// count NAV_CLIMB nodes remaining
	int climbNodesLeft = 0;
	AI_Waypoint_t *pWaypoint = GetPath()->GetCurWaypoint();
	while (pWaypoint && pWaypoint->NavType() == NAV_CLIMB)
	{
		++climbNodesLeft;
		pWaypoint = pWaypoint->GetNext();
	}

	AIMoveResult_t result = GetMotor()->MoveClimbExecute( climbDest, climbDir, climbDist, GetPath()->CurWaypointYaw(), climbNodesLeft );

	if ( result == AIMR_CHANGE_TYPE )
	{
		if ( GetPath()->GetCurWaypoint()->GetNext() )
			AdvancePath();
		else
			OnNavComplete();

		if ( !GetPath()->GetCurWaypoint() || !GetPath()->GetCurWaypoint()->GetNext() || !(GetPath()->CurWaypointNavType() == NAV_CLIMB))
		{
			DbgNavMsg( GetOuter(), "Climb stop\n" );
			GetMotor()->MoveClimbStop();
			SetNavType(NAV_GROUND);
		}
	}
	else if ( result != AIMR_OK )
	{
		DbgNavMsg( GetOuter(), "Climb fail (2)\n" );
		GetMotor()->MoveClimbStop();
		SetNavType(NAV_GROUND);
		return result;
	}

	return result;
}

//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Navigator::MoveJump()
{
	// --------------------------------------------------
	//  JUMPING
	// --------------------------------------------------
	if ( (GetNavType() != NAV_JUMP) && (GetEntFlags() & FL_ONGROUND) )
	{
		// --------------------------------------------------
		//  Now check if I can actually jump this distance?
		// --------------------------------------------------
		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_JUMP, GetLocalOrigin(), GetPath()->CurWaypointPos(), 
			MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );
		if ( IsMoveBlocked( moveTrace ) )
		{
			return moveTrace.fStatus;
		}

		SetNavType(NAV_JUMP);

		DbgNavMsg( GetOuter(), "Jump start\n" );
		GetMotor()->MoveJumpStart( moveTrace.vJumpVelocity );
	}	
	// --------------------------------------------------
	//  LANDING (from jump)
	// --------------------------------------------------
	else if (GetNavType() == NAV_JUMP && (GetEntFlags() & FL_ONGROUND)) 
	{
		// DevMsg( "jump to %f %f %f landed %f %f %f", GetPath()->CurWaypointPos().x, GetPath()->CurWaypointPos().y, GetPath()->CurWaypointPos().z, GetLocalOrigin().x, GetLocalOrigin().y, GetLocalOrigin().z );

		DbgNavMsg( GetOuter(), "Jump stop\n" );
		AIMoveResult_t result = GetMotor()->MoveJumpStop( );

		if (result == AIMR_CHANGE_TYPE)
		{
			SetNavType(NAV_GROUND);

			// --------------------------------------------------
			//  If I've jumped to my goal I'm done
			// --------------------------------------------------
			if (CurWaypointIsGoal())
			{
				OnNavComplete();
				return AIMR_OK;
			}
			// --------------------------------------------------
			//  Otherwise advance my route and walk
			// --------------------------------------------------
			else 
			{
				AdvancePath();
				return AIMR_CHANGE_TYPE;
			}
		}
		return AIMR_OK;
	}
	// --------------------------------------------------
	//  IN-AIR (from jump)
	// --------------------------------------------------
	else
	{
		GetMotor()->MoveJumpExecute( );
	}

	return AIMR_OK;
}


//-----------------------------------------------------------------------------

void CAI_Navigator::MoveCalcBaseGoal( AILocalMoveGoal_t *pMoveGoal )
{
	AI_PROFILE_SCOPE( CAI_Navigator_MoveCalcBaseGoal );

	pMoveGoal->navType			= GetNavType();
	pMoveGoal->target			= GetPath()->CurWaypointPos();
	pMoveGoal->maxDist			= ComputePathDirection( GetNavType(), GetLocalOrigin(), pMoveGoal->target, &pMoveGoal->dir );
	pMoveGoal->facing			= pMoveGoal->dir;
	pMoveGoal->speed			= GetMotor()->GetSequenceGroundSpeed( GetMovementSequence() );
	pMoveGoal->curExpectedDist	= pMoveGoal->speed * GetMotor()->GetMoveInterval();
	pMoveGoal->pMoveTarget		= GetNavTargetEntity();

	if ( pMoveGoal->curExpectedDist > pMoveGoal->maxDist )
		pMoveGoal->curExpectedDist = pMoveGoal->maxDist;

	if ( GetPath()->CurWaypointIsGoal())
	{
		pMoveGoal->flags |= AILMG_TARGET_IS_GOAL;
	}
	else
	{
		AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
		if ( pCurWaypoint->GetNext() && pCurWaypoint->GetNext()->NavType() != pCurWaypoint->NavType() )
			pMoveGoal->flags |= AILMG_TARGET_IS_TRANSITION;
	}

	const Task_t *pCurTask = GetOuter()->GetTask();
	if ( pCurTask && pCurTask->iTask == TASK_STOP_MOVING )
	{
		// If we're running stop moving, don't steer or run avoidance paths
		// This stops the NPC wiggling around as they attempt to reach a stopping
		// path that's pushed right up against geometry. (Tracker #48656)
		pMoveGoal->flags |= ( AILMG_NO_STEER | AILMG_NO_AVOIDANCE_PATHS );
	}

	pMoveGoal->pPath = GetPath();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	if ( GetOuter()->OnCalcBaseMove( pMoveGoal, distClear, pResult ) )
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	bool fTargetIsGoal = ( ( pMoveGoal->flags & AILMG_TARGET_IS_GOAL ) != 0 );
	bool fShouldAttemptHit = false;
	bool fShouldAdvancePath = false;
	float tolerance = 0;

	if ( fTargetIsGoal )
	{
		fShouldAttemptHit = true;
		tolerance = GetPath()->GetGoalTolerance();
	}
	else if ( !( pMoveGoal->flags & AILMG_TARGET_IS_TRANSITION ) )
	{
		fShouldAttemptHit = true;
		fShouldAdvancePath = true;
		tolerance = GetPath()->GetWaypointTolerance();

		// If the immediate goal is close, and the clearance brings into tolerance,
		// just try and move on
		if ( pMoveGoal->maxDist < 4*12 && pMoveGoal->maxDist - distClear < tolerance )
			tolerance = pMoveGoal->maxDist + 1;
	}

	if ( fShouldAttemptHit )
	{
		if ( distClear > pMoveGoal->maxDist )
		{
#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
			if ( distClear < AI_EPS_CASTS ) // needed because vphysics can pull us back up to this far
			{
				DebugNoteMovementFailure();
				*pResult = pMoveGoal->directTrace.fStatus;
				pMoveGoal->maxDist = 0;
				return true;
			}
#endif
			*pResult = AIMR_OK;
			return true;
		}


#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
		if ( pMoveGoal->maxDist + AI_EPS_CASTS < tolerance )
#else
		if ( pMoveGoal->maxDist < tolerance )
#endif
		{
			if ( !fTargetIsGoal ||
				 ( pMoveGoal->directTrace.fStatus != AIMR_BLOCKED_NPC ) ||
				 ( !((CAI_BaseNPC *)pMoveGoal->directTrace.pObstruction)->IsMoving() ) )
			{
				pMoveGoal->maxDist = distClear;
				*pResult = AIMR_OK;

				if ( fShouldAdvancePath )
				{
					AdvancePath();
				}
				else if ( distClear < 0.025 )
				{
					*pResult = pMoveGoal->directTrace.fStatus;
				}
				return true;
			}
		}
	}

#ifdef HL2_EPISODIC
	// Build an avoidance path around a vehicle
	if ( ai_vehicle_avoidance.GetBool() && pMoveGoal->directTrace.pObstruction != NULL && pMoveGoal->directTrace.pObstruction->GetServerVehicle() != NULL )
	{
		//FIXME: This should change into a flag which forces an OBB route to be formed around the entity in question!
		AI_Waypoint_t *pOBB = GetPathfinder()->BuildOBBAvoidanceRoute( GetOuter()->GetAbsOrigin(),
																	   GetGoalPos(),
																	   pMoveGoal->directTrace.pObstruction,
																	   GetNavTargetEntity(), 
																	   GetNavType() );

		// See if we need to complete this navigation
		if ( pOBB == NULL )
		{
			/*
			if ( GetOuter()->ShouldFailNav( true ) == false )
			{
				// Create a physics solver to allow us to pass
				NPCPhysics_CreateSolver( GetOuter(), pMoveGoal->directTrace.pObstruction, true, 5.0f );
				return true;
			}
			*/
		}
		else
		{
			// Otherwise we have a clear path to move around
			GetPath()->PrependWaypoints( pOBB );
			return true;
		}
	}
#endif // HL2_EPISODIC

	// Allow the NPC to override this behavior. Above logic takes priority
	if ( GetOuter()->OnObstructionPreSteer( pMoveGoal, distClear, pResult ) )
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( !m_hBigStepGroundEnt.Get() &&
		 pMoveGoal->directTrace.pObstruction && 
		 distClear < GetHullWidth() && 
		 pMoveGoal->directTrace.pObstruction == GetOuter()->GetGroundEntity() && 
		 ( pMoveGoal->directTrace.pObstruction->IsPlayer() ||
		   dynamic_cast<CPhysicsProp *>( pMoveGoal->directTrace.pObstruction ) ) )
	{
		m_hBigStepGroundEnt = pMoveGoal->directTrace.pObstruction;
		*pResult = AIMR_CHANGE_TYPE;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnInsufficientStopDist( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

#ifdef PHYSICS_NPC_SHADOW_DISCREPENCY
	if ( distClear < AI_EPS_CASTS ) // needed because vphysics can pull us back up to this far
	{
		DebugNoteMovementFailure();
		*pResult = pMoveGoal->directTrace.fStatus;
		pMoveGoal->maxDist = 0;
		return true;
	}
#endif

	if ( !IsMovingOutOfWay( *pMoveGoal, distClear ) )
	{
		float goalDist = ComputePathDistance( GetNavType(), GetAbsOrigin(), GetPath()->ActualGoalPosition() );

		if ( goalDist < GetGoalTolerance() + 0.01 )
		{
			pMoveGoal->maxDist = distClear;
			pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
			OnNavComplete();
			*pResult = AIMR_OK;
			return true;
		}

		if ( m_NextSidestepTimer.Expired() )
		{
			// Try bumping to side
			m_NextSidestepTimer.Reset();

			AIMoveTrace_t moveTrace;
			Vector vDeflection;
			CalculateDeflection( GetLocalOrigin(), pMoveGoal->dir, pMoveGoal->directTrace.vHitNormal, &vDeflection );

			for ( int i = 1; i > -2; i -= 2 )
			{
				Vector testLoc = GetLocalOrigin() + ( vDeflection * GetHullWidth() * 2.0) * i;
				GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
				if ( moveTrace.fStatus == AIMR_OK )
				{
					Vector vNewWaypoint = moveTrace.vEndPosition;
					GetMoveProbe()->MoveLimit( GetNavType(), vNewWaypoint, pMoveGoal->target, MASK_NPCSOLID_BRUSHONLY, NULL, &moveTrace );
					if ( moveTrace.fStatus == AIMR_OK )
					{
						PrependWaypoint( vNewWaypoint, GetNavType(), bits_WP_TO_DETOUR );
						*pResult = AIMR_CHANGE_TYPE;
						return true;
					}
				}
			}
		}


		if ( distClear < 1.0 )
		{
			// too close, nothing happening, I'm screwed
			DebugNoteMovementFailure();
			*pResult = pMoveGoal->directTrace.fStatus;
			pMoveGoal->maxDist = 0;
			return true;
		}
		return false;
	}
	
	*pResult = AIMR_OK;
	pMoveGoal->maxDist = distClear;
	pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnFailedSteer( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( pMoveGoal->flags & AILMG_TARGET_IS_GOAL )
	{
		if ( distClear >= GetPathDistToGoal() )
		{
			*pResult = AIMR_OK;
			return true;
		}

		if ( distClear > pMoveGoal->maxDist - GetPath()->GetGoalTolerance() )
		{
			Assert( CurWaypointIsGoal() && fabs(pMoveGoal->maxDist - GetPathDistToCurWaypoint()) < 0.01 );

			if ( pMoveGoal->maxDist > distClear )
				pMoveGoal->maxDist = distClear;

			if ( distClear < 0.125 )
				OnNavComplete();

			pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
			*pResult = AIMR_OK;

			return true;
		}
	}
		
	if ( !(	pMoveGoal->flags & AILMG_TARGET_IS_TRANSITION ) )
	{
		float distToWaypoint = GetPathDistToCurWaypoint();
		float halfHull 		 = GetHullWidth() * 0.5;
		
		if ( distToWaypoint < halfHull )
		{
			if ( distClear > distToWaypoint + halfHull )
			{
				*pResult = AIMR_OK;
				return true;
			}
		}
	}

#if 0
	if ( pMoveGoal->directTrace.pObstruction->MyNPCPointer() &&
		 !GetOuter()->IsUsingSmallHull() && 
		 GetOuter()->SetHullSizeSmall() )
	{
		*pResult = AIMR_CHANGE_TYPE;
		return true;
	}
#endif

	if ( !TestingSteering() && pMoveGoal->directTrace.fStatus == AIMR_BLOCKED_NPC && pMoveGoal->directTrace.vHitNormal != vec3_origin )
	{
		AIMoveTrace_t moveTrace;
		Vector vDeflection;
		CalculateDeflection( GetLocalOrigin(), pMoveGoal->dir, pMoveGoal->directTrace.vHitNormal, &vDeflection );

		if ( pMoveGoal->dir.AsVector2D().Dot( vDeflection.AsVector2D() ) > 0.7 )
		{
			Vector testLoc = GetLocalOrigin() + ( vDeflection * pMoveGoal->curExpectedDist );
			GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
			if ( moveTrace.fStatus == AIMR_OK )
			{
				pMoveGoal->dir = vDeflection;
				pMoveGoal->maxDist = pMoveGoal->curExpectedDist;
				*pResult = AIMR_OK;
				return true;
			}
		}
	}


	// If fail steer more than once after a second with no measurable progres, fail completely
	// This usually means a sucessful triangulation was not actually a valid avoidance.
	const float MOVE_TOLERANCE = 12.0;
	const float TIME_TOLERANCE = 1.0;

	if ( m_vPosBeginFailedSteer == vec3_invalid || ( m_vPosBeginFailedSteer - GetAbsOrigin() ).LengthSqr() > Square(MOVE_TOLERANCE) )
	{
		m_vPosBeginFailedSteer = GetAbsOrigin();
		m_timeBeginFailedSteer = gpGlobals->curtime;
	}
	else if ( GetNavType() == NAV_GROUND && 
			  gpGlobals->curtime - m_timeBeginFailedSteer > TIME_TOLERANCE && 
			  GetOuter()->m_flGroundSpeed * TIME_TOLERANCE > MOVE_TOLERANCE )
	{
		*pResult = AIMR_ILLEGAL;
		return true;
	}

	if ( !(pMoveGoal->flags & AILMG_NO_AVOIDANCE_PATHS) && distClear < pMoveGoal->maxDist && !TestingSteering() )
	{
		if ( PrependLocalAvoidance( distClear, pMoveGoal->directTrace ) )
		{
			*pResult = AIMR_CHANGE_TYPE;
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnFailedLocalNavigation( pMoveGoal, distClear, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( DelayNavigationFailure( pMoveGoal->directTrace ) )
	{
		*pResult = AIMR_OK;
		pMoveGoal->maxDist = distClear;
		pMoveGoal->flags |= AILMG_CONSUME_INTERVAL;
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::DelayNavigationFailure( const AIMoveTrace_t &trace )
{
	// This code only handles the case of a group of AIs in close proximity, preparing
	// to move mostly as a group, but on slightly different think schedules. Without
	// this patience factor, in the middle or at the rear might fail just because it
	// happened to have its think function called a half cycle before the one
	// in front of it.

	CAI_BaseNPC *pBlocker = trace.pObstruction ? trace.pObstruction->MyNPCPointer() : NULL;
	Assert( m_fPeerMoveWait == false || pBlocker == m_hPeerWaitingOn ); // expected to be cleared each frame, and never call this function twice
	
	if ( !m_fPeerMoveWait || pBlocker != m_hPeerWaitingOn )
	{
		if ( pBlocker )
		{
			if ( m_hPeerWaitingOn != pBlocker || m_PeerWaitClearTimer.Expired() )
			{
				m_fPeerMoveWait = true;
				m_hPeerWaitingOn = pBlocker;
				m_PeerWaitMoveTimer.Reset();
				m_PeerWaitClearTimer.Reset();

				if ( pBlocker->GetGroundEntity() == GetOuter() )
				{
					trace_t bumpTrace;
					pBlocker->GetMoveProbe()->TraceHull( pBlocker->GetAbsOrigin(), 
														 pBlocker->GetAbsOrigin() + Vector(0,0,2.0), 
														 MASK_NPCSOLID, 
														 &bumpTrace );
					if ( bumpTrace.fraction == 1.0  )
					{
						UTIL_SetOrigin(pBlocker, bumpTrace.endpos, true);
					}
				}
			}
			else if ( m_hPeerWaitingOn == pBlocker && !m_PeerWaitMoveTimer.Expired() )
			{
				m_fPeerMoveWait = true;
			}
		}
	}
	
	return m_fPeerMoveWait;
}

//-----------------------------------------------------------------------------

// @TODO (toml 11-12-02): right now, physics can pull the box back pretty far even though a hull
// trace said we could make the move. Jay is looking into it. For now, if the NPC physics shadow
// is active, allow for a larger tolerance
extern ConVar npc_vphysics;

bool test_it = false;

bool CAI_Navigator::MoveUpdateWaypoint( AIMoveResult_t *pResult )
{
	// Note that goal & waypoint tolerances are handled in progress blockage cases (e.g., OnObstructionPreSteer)

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float 		   waypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), pCurWaypoint->GetPos() );
	bool		   bIsGoal		= CurWaypointIsGoal();
	float		   tolerance	= ( npc_vphysics.GetBool() ) ? 0.25 : 0.0625;

	bool fHit = false;

	if ( waypointDist <= tolerance )
	{
		if ( test_it )
		{
			if ( pCurWaypoint->GetNext() && pCurWaypoint->GetNext()->NavType() != pCurWaypoint->NavType() )
			{
				if ( waypointDist < 0.001 )
					fHit = true;
			}
			else
				fHit = true;
		}
		else
			fHit = true;
	}
	
	if ( fHit )
	{
		if ( bIsGoal )
		{
			OnNavComplete();
			*pResult = AIMR_OK;
			
		}
		else
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
		return true;
	}
	
	return false;
}
	 
//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveStalled( const AILocalMoveGoal_t &move )
{
	SetActivity( GetOuter()->GetStoppedActivity() );
	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult )
{
	// Allow the NPC to override this behavior
	if ( GetOuter()->OnMoveExecuteFailed( move, trace, fMotorResult, pResult ))
	{
		DebugNoteMovementFailureIfBlocked( *pResult );
		return true;
	}

	if ( !m_hBigStepGroundEnt.Get() &&
		 trace.pObstruction &&
		 trace.flTotalDist - trace.flDistObstructed < GetHullWidth() && 
		 trace.pObstruction == GetOuter()->GetGroundEntity() && 
		 ( trace.pObstruction->IsPlayer() ||
		   dynamic_cast<CPhysicsProp *>( trace.pObstruction ) ) )
	{
		m_hBigStepGroundEnt = trace.pObstruction;
		*pResult = AIMR_CHANGE_TYPE;
		return true;
	}

	if ( fMotorResult == AIM_PARTIAL_HIT_TARGET )
	{
		OnNavComplete();
		*pResult = AIMR_OK;
	}
	else if ( fMotorResult == AIM_PARTIAL_HIT_NPC && DelayNavigationFailure( trace ) )
	{
		*pResult = AIMR_OK;
	}

	return true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::OnMoveBlocked( AIMoveResult_t *pResult )
{
	if ( *pResult == AIMR_BLOCKED_NPC && 
		 GetPath()->GetCurWaypoint() &&
		 ( GetPath()->GetCurWaypoint()->Flags() & bits_WP_TO_DOOR ) )
	{
		CBasePropDoor *pDoor = (CBasePropDoor *)(CBaseEntity *)GetPath()->GetCurWaypoint()->GetEHandleData();
		if (pDoor != NULL)
		{
			GetOuter()->OpenPropDoorBegin( pDoor );
			*pResult = AIMR_OK;
			return true;
		}
	}


	// Allow the NPC to override this behavior
	if ( GetOuter()->OnMoveBlocked( pResult ))
		return true;

	float flWaypointDist;

	if ( !GetPath()->CurWaypointIsGoal() && GetPath()->GetCurWaypoint()->IsReducible() )
	{
		flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetCurWaypointPos() );
		if ( flWaypointDist < GetHullWidth() )
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
	}

	SetActivity( GetOuter()->GetStoppedActivity() );

	const float EPS = 0.1;
	
	flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->ActualGoalPosition() );

	if ( flWaypointDist < GetGoalTolerance() + EPS )
	{
		OnNavComplete();
		*pResult = AIMR_OK;
		return true;
	}

	return false;
}

//-------------------------------------

AIMoveResult_t CAI_Navigator::MoveEnact( const AILocalMoveGoal_t &baseMove )
{
	AIMoveResult_t result = AIMR_ILLEGAL;
	AILocalMoveGoal_t move = baseMove;

	result = GetLocalNavigator()->MoveCalc( &move, ( m_flLastSuccessfulSimplifyTime == gpGlobals->curtime ) );

	if ( result != AIMR_OK )
		m_hLastBlockingEnt = move.directTrace.pObstruction;
	else
	{
		m_hLastBlockingEnt = NULL;
		GetMoveProbe()->ClearBlockingEntity();
	}
	
	if ( result == AIMR_OK && !m_fNavComplete )
	{
		Assert( GetPath()->GetCurWaypoint() );
		result = GetMotor()->MoveNormalExecute( move );
	}
	else if ( result != AIMR_CHANGE_TYPE )
	{
		GetMotor()->MoveStop();
	}
	
	if ( IsMoveBlocked( result ) )
	{
		OnMoveBlocked( &result );
	}
	
	return result;
}

//-----------------------------------------------------------------------------

AIMoveResult_t CAI_Navigator::MoveNormal()
{
	if (!PreMove())
		return AIMR_ILLEGAL;

	if ( ShouldTestFailMove() )
		return AIMR_ILLEGAL;

	// --------------------------------
	
	AIMoveResult_t result = AIMR_ILLEGAL;
	
	if ( MoveUpdateWaypoint( &result ) )
		return result;
	
	// --------------------------------

	// Set activity to be the Navigation activity
	float		preMoveSpeed		= GetIdealSpeed();
	Activity	preMoveActivity		= GetActivity();
	int			nPreMoveSequence	= GetOuter()->GetSequence(); // this is an unfortunate necessity to ensure setting back the activity picks the right one if it had been sticky
	Vector		vStart				= GetAbsOrigin();

	// --------------------------------

	// FIXME: only need since IdealSpeed isn't based on movement activity but immediate activity!
	SetActivity( GetMovementActivity() );

	if ( m_bValidateActivitySpeed && GetIdealSpeed() <= 0.0f )
	{
		if ( GetActivity() == ACT_TRANSITION )
			return AIMR_OK;
		DevMsg( "%s moving with speed <= 0 (%s)\n", GetEntClassname(), GetOuter()->GetSequenceName( GetSequence() ) );
	}
			
	// --------------------------------
	
	AILocalMoveGoal_t move;
	
	MoveCalcBaseGoal( &move );

	result = MoveEnact( move );
	
	// --------------------------------
	// If we didn't actually move, but it was a success (i.e., blocked but within tolerance), make sure no visual artifact

	// FIXME: only needed because of the above slamming of SetActivity(), which is only needed 
	// because GetIdealSpeed() looks at the current activity instead of the movement activity.

	if ( result == AIMR_OK && preMoveSpeed < 0.01 )
	{
		if ( ( GetAbsOrigin() - vStart ).Length() < 0.01 )
		{
			GetOuter()->SetSequence( nPreMoveSequence );
			SetActivity( preMoveActivity );
		}
	}

	// --------------------------------

	return result;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::PreMove()
{
	Navigation_t goalType = GetPath()->CurWaypointNavType();
	Navigation_t curType  = GetNavType();

	m_fPeerMoveWait = false;
	
	if ( goalType == NAV_GROUND && curType != NAV_GROUND )
	{
		DevMsg( "Warning: %s(%s) appears to have wrong nav type in CAI_Navigator::MoveGround()\n", GetOuter()->GetClassname(), STRING( GetOuter()->GetEntityName() ) );
		switch ( curType )
		{
			case NAV_CLIMB:
			{
				GetMotor()->MoveClimbStop();
				break;
			}
			
			case NAV_JUMP:
			{
				GetMotor()->MoveJumpStop();
				break;
			}
		}

		SetNavType( NAV_GROUND );
	}
	else if ( goalType == NAV_FLY && curType != NAV_FLY )
	{
		AssertMsg( 0, ( "GetNavType() == NAV_FLY" ) );
		return false;
	}

	// --------------------------------
	
	Assert( GetMotor()->GetMoveInterval() > 0 );

	// --------------------------------
	
	SimplifyPath();
	
	
	return true;
}

//--------------------------------------------------------------------------------------------

bool CAI_Navigator::IsMovingOutOfWay( const AILocalMoveGoal_t &moveGoal, float distClear )
{
	// FIXME: We can make this work for regular entities; although the
	// original code was simply looking @ NPCs. I'm reproducing that code now
	// although I want to make it work for both.
	CAI_BaseNPC *pBlocker = moveGoal.directTrace.pObstruction ? moveGoal.directTrace.pObstruction->MyNPCPointer() : NULL;

	// if it's the world, it ain't moving
	if (!pBlocker)
		return false;

	// if they're doing something, assume it'll work out
	if (pBlocker->IsMoving())
	{
		if ( distClear > moveGoal.curExpectedDist * 0.75 )
			return true;

		Vector2D velBlocker = pBlocker->GetMotor()->GetCurVel().AsVector2D();
		Vector2D velBlockerNorm = velBlocker;

		Vector2DNormalize( velBlockerNorm );

		float dot = moveGoal.dir.AsVector2D().Dot( velBlockerNorm );

		if (dot > -0.25 )
		{
			return true;
		}
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Move towards the next waypoint on my route
// Input  :
// Output :
//-----------------------------------------------------------------------------

enum AINavResult_t
{
	AINR_OK,
	AINR_NO_GOAL,
	AINR_NO_ROUTE,
	AINR_BLOCKED,
	AINR_ILLEGAL
};

bool CAI_Navigator::Move( float flInterval ) 
{
	if (flInterval > 1.0)
	{
		// Bound interval so we don't get ludicrous motion when debugging
		// or when framerate drops catostrophically
		flInterval = 1.0;
	}

	if ( !GetOuter()->OverrideMove( flInterval ) )
	{
		// UNDONE: Figure out how much of the timestep was consumed by movement
		// this frame and restart the movement/schedule engine if necessary
		bool bHasGoal = GetGoalType() != GOALTYPE_NONE;
		bool bIsTurning = HasMemory( bits_MEMORY_TURNING );
		if ( bHasGoal )
		{
			if ( bIsTurning )
			{
				if ( gpGlobals->curtime - GetPath()->GetStartTime() > 5 )
				{
					Forget( bits_MEMORY_TURNING );
					bIsTurning = false;
					DbgNavMsg( GetOuter(), "NPC appears stuck turning. Proceeding.\n" );
				}
			}

			if ( ActivityIsLocomotive( m_PreviousMoveActivity ) && !ActivityIsLocomotive( GetMovementActivity() ) )
			{
				SetMovementActivity( GetOuter()->TranslateActivity( m_PreviousMoveActivity ) );
			}
		}
		else
		{
			m_PreviousMoveActivity = ACT_RESET;
			m_PreviousArrivalActivity = ACT_RESET;
		}

		bool fShouldMove = ( bHasGoal && 
							 // !bIsTurning &&
							 ActivityIsLocomotive( GetMovementActivity() ) );
		if ( fShouldMove )
		{
			AINavResult_t result = AINR_OK;
			
			GetMotor()->SetMoveInterval( flInterval );
			
			// ---------------------------------------------------------------------
			// Move should never happen if I don't have a movement goal or route
			// ---------------------------------------------------------------------
			if ( GetPath()->GoalType() == GOALTYPE_NONE )
			{
				DevWarning( "Move requested with no route!\n" );
				result = AINR_NO_GOAL;
			}
			else if (!GetPath()->GetCurWaypoint())
			{
				DevWarning( "Move goal with no route!\n" );
				GetPath()->Clear();
				result = AINR_NO_ROUTE;
			}

			if ( result == AINR_OK )
			{
				// ---------------------------------------------------------------------
				// If I've been asked to wait, let's wait
				// ---------------------------------------------------------------------
				if ( GetOuter()->ShouldMoveWait() )
				{
					GetMotor()->MovePaused();
					return false;
				}
				
				int nLoopCount = 0;

				bool bMoved = false;
				AIMoveResult_t moveResult = AIMR_CHANGE_TYPE;
				m_fNavComplete = false;

				while ( moveResult >= AIMR_OK && !m_fNavComplete )
				{
					if ( GetMotor()->GetMoveInterval() <= 0 )
					{
						moveResult = AIMR_OK;
						break;
					}

					// TODO: move higher up the call chain?
					if ( !m_bCalledStartMove )
					{
						GetMotor()->MoveStart();
						m_bCalledStartMove = true;
					}

					if ( m_hBigStepGroundEnt && m_hBigStepGroundEnt != GetOuter()->GetGroundEntity() )
						m_hBigStepGroundEnt = NULL;
					
					switch (GetPath()->CurWaypointNavType())
					{
					case NAV_CLIMB:
						moveResult = MoveClimb();
						break;

					case NAV_JUMP:
						moveResult = MoveJump();
						break;

					case NAV_GROUND:
					case NAV_FLY:
						moveResult = MoveNormal();
						break;

					default:
						DevMsg( "Bogus route move type!");
						moveResult = AIMR_ILLEGAL;
						break;
					}

					if ( moveResult == AIMR_OK )
						bMoved = true;

					++nLoopCount;
					if ( nLoopCount > 16 )
					{
						DevMsg( "ERROR: %s navigation not terminating. Possibly bad cyclical solving?\n", GetOuter()->GetDebugName() );
						moveResult = AIMR_ILLEGAL;

						switch (GetPath()->CurWaypointNavType())
						{
						case NAV_GROUND:
						case NAV_FLY:
							OnMoveBlocked( &moveResult );
							break;
						}
						break;
					}

				}

				// --------------------------------------------
				//  Update move status
				// --------------------------------------------
				if ( IsMoveBlocked( moveResult ) )
				{
					bool bRecovered = false;
					if (moveResult != AIMR_BLOCKED_NPC || GetNavType() == NAV_CLIMB || GetNavType() == NAV_JUMP || GetPath()->CurWaypointNavType() == NAV_JUMP )
					{
						if ( MarkCurWaypointFailedLink() )
						{
							AI_Waypoint_t *pSavedWaypoints = GetPath()->GetCurWaypoint();
							if ( pSavedWaypoints )
							{
								GetPath()->SetWaypoints( NULL );
								if ( RefindPathToGoal( false, true ) )
								{
									DeleteAll( pSavedWaypoints );
									bRecovered = true;
								}
								else
									GetPath()->SetWaypoints( pSavedWaypoints );

							}
						}

					}

					if ( !bRecovered )
					{
						OnNavFailed( ( moveResult == AIMR_ILLEGAL ) ? FAIL_NO_ROUTE_ILLEGAL : FAIL_NO_ROUTE_BLOCKED, true );
					}
				}
				return bMoved;
			}
			
			static AI_TaskFailureCode_t failures[] =
			{
				NO_TASK_FAILURE,				// AINR_OK (never should happen)
				FAIL_NO_ROUTE_GOAL,				// AINR_NO_GOAL
				FAIL_NO_ROUTE,					// AINR_NO_ROUTE
				FAIL_NO_ROUTE_BLOCKED,			// AINR_BLOCKED
				FAIL_NO_ROUTE_ILLEGAL			// AINR_ILLEGAL
			};
			
			OnNavFailed( failures[result], false );
		}
		else 
		{
			// @TODO (toml 10-30-02): the climb part of this is unacceptable, but needed until navigation can handle commencing
			// 						  a navigation while in the middle of a climb

			if ( GetNavType() == NAV_CLIMB )
			{
				GetMotor()->MoveClimbStop();
				SetNavType( NAV_GROUND );
			}
			GetMotor()->MoveStop();
			AssertMsg( TaskIsRunning() || TaskIsComplete(), ("Schedule stalled!!\n") );
		}
		return false;
	}

	return true; // assume override move handles stopping issues
}


//-----------------------------------------------------------------------------
// Purpose: Returns yaw speed based on what they're doing. 
//-----------------------------------------------------------------------------
float CAI_Navigator::CalcYawSpeed( void )
{
	// Allow the NPC to override this behavior
	float flNPCYaw = GetOuter()->CalcYawSpeed();
	if (flNPCYaw >= 0.0f)
		return flNPCYaw;

	float maxYaw = MaxYawSpeed();

	//return maxYaw;

	if( IsGoalSet() && GetIdealSpeed() != 0.0)
	{
		// ---------------------------------------------------
		// If not moving to a waypoint use a base turing speed
		// ---------------------------------------------------
		if (!GetPath()->GetCurWaypoint()) 
		{
			return maxYaw;
		}
		// --------------------------------------------------------------
		// If moving towards a waypoint, set the turn speed based on the 
		// distance of the waypoint and my forward velocity
		// --------------------------------------------------------------
		if (GetIdealSpeed() > 0) 
		{
			// -----------------------------------------------------------------
			// Get the projection of npc's heading direction on the waypoint dir
			// -----------------------------------------------------------------
			float waypointDist = (GetPath()->CurWaypointPos() - GetLocalOrigin()).Length();

			// If waypoint is close, aim for the waypoint
			if (waypointDist < 100)
			{
				float scale = 1 + (0.01*(100 - waypointDist));
				return (maxYaw * scale);
			}
		}
	}
	return maxYaw;
}

//-----------------------------------------------------------------------------
float CAI_Navigator::GetStepDownMultiplier()
{
	if ( m_hBigStepGroundEnt )
	{
		if ( !m_hBigStepGroundEnt->IsPlayer() )
			return 2.6;
		else
			return 10.0;
	}
	return 1.0;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to advance the route to the next waypoint, triangulating
//			around entities that are in the way
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Navigator::AdvancePath()
{
	DbgNavMsg( GetOuter(), "Advancing path\n" );

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	bool bPassingPathcorner = ( ( pCurWaypoint->Flags() & bits_WP_TO_PATHCORNER ) != 0 );

	if ( bPassingPathcorner )
	{
		Assert( !pCurWaypoint->GetNext() || (pCurWaypoint->GetNext()->Flags() & (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL )) == (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL ));
		CBaseEntity *pEntity = pCurWaypoint->hPathCorner;
		if ( pEntity )
		{
			variant_t emptyVariant;
			pEntity->AcceptInput( "InPass", GetOuter(), pEntity, emptyVariant, 0 );
		}
	}

	if ( GetPath()->CurWaypointIsGoal() )
		return;

	if ( pCurWaypoint->Flags() & bits_WP_TO_DOOR )
	{
		CBasePropDoor *pDoor = (CBasePropDoor *)(CBaseEntity *)pCurWaypoint->GetEHandleData();
		if (pDoor != NULL)
		{
			GetOuter()->OpenPropDoorBegin(pDoor);
		}
		else
		{
			DevMsg("%s trying to open a door that has been deleted!\n", GetOuter()->GetDebugName());
		}
	}

	GetPath()->Advance();

	// If we've just passed a path_corner, advance m_pGoalEnt
	if ( bPassingPathcorner )
	{
		pCurWaypoint = GetPath()->GetCurWaypoint();

		if ( pCurWaypoint )
		{
			Assert( (pCurWaypoint->Flags() & (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL )) == (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL ));

			SetGoalEnt( pCurWaypoint->hPathCorner );
			DoFindPathToPathcorner( pCurWaypoint->hPathCorner );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

#ifdef DEBUG
ConVar ai_disable_path_simplification( "ai_disable_path_simplification","0" );
#define IsSimplifyPathDisabled() ai_disable_path_simplification.GetBool()
#else
#define IsSimplifyPathDisabled() false
#endif

const float MIN_ANGLE_COS_SIMPLIFY = 0.766; // 40 deg left or right

bool CAI_Navigator::ShouldAttemptSimplifyTo( const Vector &pos )
{
	if ( m_bForcedSimplify )
		return true;
		
	Vector vecToPos = ( pos - GetLocalOrigin() );
	vecToPos.z = 0;
	VectorNormalize( vecToPos );

	Vector vecCurrentDirectionOfMovement = ( GetCurWaypointPos() - GetLocalOrigin() );
	vecCurrentDirectionOfMovement.z = 0;
	VectorNormalize( vecCurrentDirectionOfMovement );
	
	float dot = vecCurrentDirectionOfMovement.AsVector2D().Dot( vecToPos.AsVector2D() );
	
	return ( m_bForcedSimplify || dot > MIN_ANGLE_COS_SIMPLIFY );
}

//-------------------------------------

bool CAI_Navigator::ShouldSimplifyTo( bool passedDetour, const Vector &pos )
{
	int flags = AIMLF_QUICK_REJECT;

#ifndef NPCS_BLOCK_SIMPLIFICATION
	if ( !passedDetour )
		flags |= AIMLF_IGNORE_TRANSIENTS;
#endif

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( GetNavType(), 
		GetLocalOrigin(), pos, MASK_NPCSOLID, 
		GetPath()->GetTarget(), 100, flags, &moveTrace );

	return !IsMoveBlocked(moveTrace);
}

//-------------------------------------

void CAI_Navigator::SimplifyPathInsertSimplification( AI_Waypoint_t *pSegmentStart, const Vector &point )
{
	if ( point != pSegmentStart->GetPos() )
	{
		AI_Waypoint_t *pNextWaypoint = pSegmentStart->GetNext();
		Assert( pNextWaypoint );
		Assert( pSegmentStart->NavType() == pNextWaypoint->NavType() );

		AI_Waypoint_t *pNewWaypoint = new AI_Waypoint_t( point, 0, pSegmentStart->NavType(), 0, NO_NODE );

		while ( GetPath()->GetCurWaypoint() != pNextWaypoint )
		{
			Assert( GetPath()->GetCurWaypoint()->IsReducible() );
			GetPath()->Advance();
		}
		pNewWaypoint->SetNext( pNextWaypoint );
		GetPath()->SetWaypoints( pNewWaypoint );
	}
	else
	{
		while ( GetPath()->GetCurWaypoint() != pSegmentStart )
		{
			Assert( GetPath()->GetCurWaypoint()->IsReducible() );
			GetPath()->Advance();
		}
	}

}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params, 
											 AI_Waypoint_t *pCurWaypoint, const Vector &curPoint, 
											 float distRemaining, bool skipTest, bool passedDetour, int *pTestCount )
{
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( !passedDetour )
		passedDetour = ( ( pCurWaypoint->Flags() & bits_WP_TO_DETOUR) != 0 );

	if ( distRemaining > 0)
	{
		if ( pCurWaypoint->IsReducible() )
		{
			// Walk out to test point, or next waypoint
			AI_Waypoint_t *pRecursionWaypoint;
			Vector nextPoint;
			
			float distToNext = ComputePathDistance( GetNavType(), curPoint, pNextWaypoint->GetPos() );
			if (distToNext < params.increment * 1.10 )
			{
				nextPoint = pNextWaypoint->GetPos();
				distRemaining -= distToNext;
				pRecursionWaypoint = pNextWaypoint;
			}
			else
			{
				Vector offset = pNextWaypoint->GetPos() - pCurWaypoint->GetPos();
				VectorNormalize( offset );
				offset *= params.increment;
				nextPoint = curPoint + offset;
				distRemaining -= params.increment;
				pRecursionWaypoint = pCurWaypoint;
			}
			
			bool skipTestNext = ( ComputePathDistance( GetNavType(), GetLocalOrigin(), nextPoint ) > params.radius + 0.1 );

			if ( SimplifyPathForwardScan( params, pRecursionWaypoint, nextPoint, distRemaining, skipTestNext, passedDetour, pTestCount ) )
				return true;
		}
	}
	
	if ( !skipTest && *pTestCount < params.maxSamples && ShouldAttemptSimplifyTo( curPoint ) )
	{
		(*pTestCount)++;

		if ( ShouldSimplifyTo( passedDetour, curPoint ) )
		{
			SimplifyPathInsertSimplification( pCurWaypoint, curPoint );
			return true;
		}
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params )
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float distRemaining = params.scanDist - GetPathDistToCurWaypoint();
	int testCount = 0;

	if ( distRemaining < 0.1 )
		return false;
	
	if ( SimplifyPathForwardScan( params, pCurWaypoint, pCurWaypoint->GetPos(), distRemaining, true, false, &testCount ) )
		return true;
		
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForward( float maxDist )
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( !pNextWaypoint )
		return false;

	AI_PROFILE_SCOPE(CAI_Navigator_SimplifyPathForward);

	static SimplifyForwardScanParams fullScanParams = 
	{
		32 * 12, 	// Distance to move out path
		12 * 12, 	// Radius within which a point must be to be valid
		3 * 12, 	// Increment to move out on
		4, 			// maximum number of point samples
	};

	SimplifyForwardScanParams scanParams = fullScanParams;
	if ( maxDist > fullScanParams.radius )
	{
		float ratio = (maxDist / fullScanParams.radius);

		fullScanParams.radius = maxDist;
		fullScanParams.scanDist *= ratio;
		fullScanParams.increment *= ratio;
	}
	
	if ( SimplifyPathForwardScan( scanParams ) )
		return true;

	if ( ShouldAttemptSimplifyTo( pNextWaypoint->GetPos() ) && 
		 ComputePathDistance( GetNavType(), GetLocalOrigin(), pNextWaypoint->GetPos() ) < scanParams.scanDist && 
		 ShouldSimplifyTo( ( ( pCurWaypoint->Flags() & bits_WP_TO_DETOUR ) != 0 ), pNextWaypoint->GetPos() ) ) // @TODO (toml 04-25-03): need to handle this better. this is here because forward scan may look out so far that a close obvious solution is skipped (due to test limiting)
	{
		delete pCurWaypoint;
		GetPath()->SetWaypoints(pNextWaypoint);
		return true;
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathBacktrack()
{
	AI_PROFILE_SCOPE(CAI_Navigator_SimplifyPathBacktrack);

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();
	
	// ------------------------------------------------------------------------
	// If both waypoints are ground waypoints and my path sends me back tracking 
	// more than 24 inches, try to aim for (roughly) the nearest point on the line 
	// connecting the first two waypoints
	// ------------------------------------------------------------------------
	if (pCurWaypoint->GetNext() &&
		(pNextWaypoint->Flags() & bits_WP_TO_NODE) &&
		(pNextWaypoint->NavType() == NAV_GROUND) &&
		(pCurWaypoint->NavType() == NAV_GROUND)	&&
		(pCurWaypoint->Flags() & bits_WP_TO_NODE) )	
	{

		Vector firstToMe	= (GetLocalOrigin()			   - pCurWaypoint->GetPos());
		Vector firstToNext	= pNextWaypoint->GetPos() - pCurWaypoint->GetPos();
		VectorNormalize(firstToNext);
		firstToMe.z			= 0;
		firstToNext.z		= 0;
		float firstDist	= firstToMe.Length();
		float firstProj	= DotProduct(firstToMe,firstToNext);
		float goalTolerance = GetPath()->GetGoalTolerance();
		if (firstProj>0.5*firstDist) 
		{
			Vector targetPos = pCurWaypoint->GetPos() + (firstProj * firstToNext);

			// Only use a local or jump move
			int buildFlags = 0;
			if (CapabilitiesGet() & bits_CAP_MOVE_GROUND)
				buildFlags |= bits_BUILD_GROUND; 
			if (CapabilitiesGet() & bits_CAP_MOVE_JUMP)
				buildFlags |= bits_BUILD_JUMP;

			// Make sure I can get to the new point
			AI_Waypoint_t *route1 = GetPathfinder()->BuildLocalRoute(GetLocalOrigin(), targetPos, GetPath()->GetTarget(), bits_WP_TO_DETOUR, NO_NODE, buildFlags, goalTolerance);
			if (!route1) 
				return false;

			// Make sure the new point gets me to the target location
			AI_Waypoint_t *route2 = GetPathfinder()->BuildLocalRoute(targetPos, pNextWaypoint->GetPos(), GetPath()->GetTarget(), bits_WP_TO_DETOUR, NO_NODE, buildFlags, goalTolerance);
			if (!route2) 
			{
				DeleteAll(route1);
				return false;
			}

			// Add the two route together
			AddWaypointLists(route1,route2);

			// Now add the rest of the old route to the new one
			AddWaypointLists(route1,pNextWaypoint->GetNext());

			// Now advance the route linked list, putting the finished waypoints back in the waypoint pool
			AI_Waypoint_t *freeMe = pCurWaypoint->GetNext();
			delete pCurWaypoint;
			delete freeMe;

			GetPath()->SetWaypoints(route1);
			return true;
		}
	}
	return false;
}			

//-------------------------------------

bool CAI_Navigator::SimplifyPathQuick()
{
	AI_PROFILE_SCOPE(CAI_Navigator_SimplifyPathQuick);

	static SimplifyForwardScanParams quickScanParams[2] = 
	{ 
		{
			(12.0 * 12.0) - 0.1,	// Distance to move out path
			12 * 12, 				// Radius within which a point must be to be valid
			0.5 * 12, 				// Increment to move out on
			1, 						// maximum number of point samples
		},
		// Strong optimization version
		{
			(6.0 * 12.0) - 0.1,	// Distance to move out path
			8 * 12, 				// Radius within which a point must be to be valid
			1.0 * 12, 				// Increment to move out on
			1, 						// maximum number of point samples
		} 
	};
	
	if ( SimplifyPathForwardScan( quickScanParams[AIStrongOpt()] ) )
		return true;

	return false;
}

//-------------------------------------

// Second entry is the strong opt version
const float ROUTE_SIMPLIFY_TIME_DELAY[2]		= { 0.5, 1.0f };
const float NO_PVS_ROUTE_SIMPLIFY_TIME_DELAY[2]	= { 1.0, 2.0f };
const float QUICK_SIMPLIFY_TIME_DELAY[2]		= { FLT_MIN, 0.3f };

int g_iFrameLastSimplified;

bool CAI_Navigator::SimplifyPath( bool bFirstForPath, float scanDist )
{
	AI_PROFILE_SCOPE(CAI_Navigator_SimplifyPath);
	
	if ( TestingSteering() || IsSimplifyPathDisabled() )
		return false;

	bool bInPVS = GetOuter()->HasCondition( COND_IN_PVS );
	bool bRetVal = false;

	Navigation_t navType = GetOuter()->GetNavType();
	if (navType == NAV_GROUND || navType == NAV_FLY)
	{
		AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
		if ( !pCurWaypoint || !pCurWaypoint->IsReducible() )
			return false;

		//-----------------------------

		bool bFullSimplify;
		
		bFullSimplify = ( m_flNextSimplifyTime <= gpGlobals->curtime );

		if ( bFirstForPath && !bFullSimplify )
		{
			bFullSimplify = bInPVS;
		}

		if ( AIStrongOpt() && bFullSimplify )
		{
			if ( g_iFrameLastSimplified != gpGlobals->framecount )
			{
				g_iFrameLastSimplified = gpGlobals->framecount;
			}
			else
			{
				bFullSimplify = false;
			}
		}

		m_bForcedSimplify = bFirstForPath;

		//-----------------------------

		if ( bFullSimplify )
		{
			float simplifyDelay = ( bInPVS ) ? ROUTE_SIMPLIFY_TIME_DELAY[AIStrongOpt()] : NO_PVS_ROUTE_SIMPLIFY_TIME_DELAY[AIStrongOpt()];
			
			if ( GetOuter()->GetMoveEfficiency() > AIME_NORMAL )
				simplifyDelay *= 2;

			m_flNextSimplifyTime = gpGlobals->curtime + simplifyDelay;

			if ( SimplifyPathForward( scanDist ) )
				bRetVal = true;
			else if ( SimplifyPathBacktrack() )
				bRetVal = true;
			else if ( SimplifyPathQuick() )
				bRetVal = true;
		}
		else if ( bFirstForPath || ( bInPVS && GetOuter()->GetMoveEfficiency() == AIME_NORMAL ) )
		{
			if ( !AIStrongOpt() || gpGlobals->curtime - m_flLastSuccessfulSimplifyTime > QUICK_SIMPLIFY_TIME_DELAY[AIStrongOpt()] )
			{
				if ( SimplifyPathQuick() )
					bRetVal = true;
			}
		}
	}

	if ( bRetVal )
	{
		m_flLastSuccessfulSimplifyTime = gpGlobals->curtime;
		DbgNavMsg( GetOuter(), "Simplified path\n" );
	}

	return bRetVal;
}
//-----------------------------------------------------------------------------

AI_NavPathProgress_t CAI_Navigator::ProgressFlyPath( const AI_ProgressFlyPathParams_t &params )
{
	if ( IsGoalActive() )
	{
		float waypointDist = ( GetCurWaypointPos() - GetLocalOrigin() ).Length();

		if ( CurWaypointIsGoal() )
		{
			float tolerance = MAX( params.goalTolerance, GetPath()->GetGoalTolerance() );
			if ( waypointDist <= tolerance )
				return AINPP_COMPLETE;
		}
		else
		{
			bool bIsStrictWaypoint = ( (GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 );
			float tolerance = (bIsStrictWaypoint) ? params.strictPointTolerance : params.waypointTolerance;
			if ( waypointDist <= tolerance )
			{
				trace_t tr;
				AI_TraceLine( GetAbsOrigin(), GetPath()->GetCurWaypoint()->GetNext()->GetPos(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction == 1.0f )
				{
					AdvancePath();	
					return AINPP_ADVANCED;
				}
			}

			if ( SimplifyFlyPath( params ) )
				return AINPP_ADVANCED;
		}
	}

	return AINPP_NO_CHANGE;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SimplifyFlyPath( unsigned collisionMask, const CBaseEntity *pTarget, 
									 float strictPointTolerance, float blockTolerance,
									 AI_NpcBlockHandling_t blockHandling)
{
	AI_ProgressFlyPathParams_t params( collisionMask, strictPointTolerance, blockTolerance,
									   0, 0, blockHandling );
	params.SetCurrent( pTarget );
	SimplifyFlyPath( params );
}

//-----------------------------------------------------------------------------

#define FLY_ROUTE_SIMPLIFY_TIME_DELAY 0.3
#define FLY_ROUTE_SIMPLIFY_LOOK_DIST (12.0*12.0)

bool CAI_Navigator::SimplifyFlyPath(  const AI_ProgressFlyPathParams_t &params )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	if ( m_flNextSimplifyTime > gpGlobals->curtime)
		return false;

	m_flNextSimplifyTime = gpGlobals->curtime + FLY_ROUTE_SIMPLIFY_TIME_DELAY;

	if ( params.bTrySimplify && SimplifyPathForward( FLY_ROUTE_SIMPLIFY_LOOK_DIST ) )
		return true;

	// don't shorten path_corners
	bool bIsStrictWaypoint = ( !params.bTrySimplify || ( (GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 ) );

	Vector dir = GetCurWaypointPos() - GetLocalOrigin();
	float length = VectorNormalize( dir );
	
	if ( !bIsStrictWaypoint || length < params.strictPointTolerance )
	{
		// FIXME: This seems strange... Why should this condition ever be met?
		// Don't advance your waypoint if you don't have one!
		if (GetPath()->CurWaypointIsGoal())
			return false;

		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), GetPath()->NextWaypointPos(),
			params.collisionMask, params.pTarget, &moveTrace);
		
		if ( moveTrace.flDistObstructed - params.blockTolerance < 0.01 || 
			 ( ( params.blockHandling == AISF_IGNORE) && ( moveTrace.fStatus == AIMR_BLOCKED_NPC ) ) )
		{
			AdvancePath();
			return true;
		}
		else if ( moveTrace.pObstruction && params.blockHandling == AISF_AVOID )
		{
			PrependLocalAvoidance( params.blockTolerance - moveTrace.flDistObstructed, moveTrace );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_Navigator::MovementCost( int moveType, Vector &vecStart, Vector &vecEnd )
{
	float cost;
	
	cost = (vecStart - vecEnd).Length();

	if ( moveType == bits_CAP_MOVE_JUMP || moveType == bits_CAP_MOVE_CLIMB )
	{
		cost *= 2.0;
	}

	// Allow the NPC to override the movement cost
	GetOuter()->MovementCost( moveType, vecStart, vecEnd, &cost );
	
	return cost;
}

//-----------------------------------------------------------------------------
// Purpose: Will the entities hull fit at the given node
// Input  :
// Output : Returns true if hull fits at node
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtNode(int nodeNum, unsigned int collisionMask ) 
{
	// Make sure I have a network
	if (!GetNetwork())
	{
		DevMsg("CanFitAtNode() called with no network!\n");
		return false;
	}

	CAI_Node* pTestNode = GetNetwork()->GetNode(nodeNum);
	Vector startPos		= pTestNode->GetPosition(GetHullType());

	// -------------------------------------------------------------------
	// Check ground nodes for standable bottom
	// -------------------------------------------------------------------
	if (pTestNode->GetType() == NODE_GROUND)
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check climb exit nodes for standable bottom
	// -------------------------------------------------------------------
	if ((pTestNode->GetType() == NODE_CLIMB) &&
		(pTestNode->m_eNodeInfo & (bits_NODE_CLIMB_BOTTOM | bits_NODE_CLIMB_EXIT )))
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check that hull fits at position of node
	// -------------------------------------------------------------------
	if (!CanFitAtPosition( startPos, collisionMask ))
	{
		startPos.z += GetOuter()->StepHeight();
		if (!CanFitAtPosition( startPos, collisionMask ))
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if NPC's hull fits in the given spot with the
//			given collision mask
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtPosition( const Vector &vStartPos, unsigned int collisionMask, bool bIgnoreTransients, bool bAllowPlayerAvoid  )
{
	CTraceFilterNav traceFilter( const_cast<CAI_BaseNPC *>(GetOuter()), bIgnoreTransients, GetOuter(), COLLISION_GROUP_NONE, bAllowPlayerAvoid );

	Vector vEndPos	= vStartPos;
	vEndPos.z += 0.01;
	trace_t tr;
	AI_TraceHull( vStartPos, vEndPos, 
				  GetHullMins(), GetHullMaxs(), 
				  collisionMask, 
				  &traceFilter, 
				  &tr );

	if (tr.startsolid)
	{
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathDistToCurWaypoint() const
{
	return ( GetPath()->GetCurWaypoint() ) ? 
				ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->CurWaypointPos() ) :
				0;
}


//-----------------------------------------------------------------------------
// Computes the distance to our goal, rebuilding waypoint distances if necessary.
// Returns -1 if we still don't have a valid path length after rebuilding.
//
// NOTE: this should really be part of GetPathDistToGoal below, but I didn't
// want to affect OnFailedSteer this close to shipping! (dvs: 8/16/07)
//-----------------------------------------------------------------------------
float CAI_Navigator::BuildAndGetPathDistToGoal()
{
	if ( !GetPath() )
		return -1;
	
	// Make sure it's fresh.	
	GetPath()->GetPathLength();

	if ( GetPath()->GetCurWaypoint() && ( GetPath()->GetCurWaypoint()->flPathDistGoal >= 0 ) )
		return GetPathDistToGoal();
	
	return -1;
}


// FIXME: this ignores the fact that flPathDistGoal might be -1, yielding nonsensical results.
// See BuildAndGetPathDistToGoal above.
float CAI_Navigator::GetPathDistToGoal() const
{
	return ( GetPath()->GetCurWaypoint() ) ? 
				( GetPathDistToCurWaypoint() + GetPath()->GetCurWaypoint()->flPathDistGoal ) : 
				0;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a route
// Input  :
// Output : True if successful / false if fail
//-----------------------------------------------------------------------------
bool CAI_Navigator::FindPath( bool fSignalTaskStatus, bool bDontIgnoreBadLinks )
{
	// Test to see if we're resolving spiking problems via threading
	if ( ai_navigator_generate_spikes.GetBool() )
	{
		unsigned int nLargeCount = (INT_MAX>>(ai_navigator_generate_spikes_strength.GetInt()));
		while ( nLargeCount-- ) {}
	}

	bool bRetrying = (HasMemory(bits_MEMORY_PATH_FAILED) && m_timePathRebuildMax != 0 );
	if ( bRetrying )
	{
		// If I've passed by fail time, fail this task
		if (m_timePathRebuildFail < gpGlobals->curtime)
		{
			if ( fSignalTaskStatus )
				OnNavFailed( FAIL_NO_ROUTE );
			else
				OnNavFailed();
			return false;
		}
		else if ( m_timePathRebuildNext > gpGlobals->curtime )
		{
			return false;
		}
	}

	bool bFindResult = DoFindPath();

	if ( !bDontIgnoreBadLinks && !bFindResult && GetOuter()->IsNavigationUrgent() )
	{
		GetPathfinder()->SetIgnoreBadLinks();
		bFindResult = DoFindPath();
	}

	if (bFindResult)
	{	
		Forget(bits_MEMORY_PATH_FAILED);

		if (fSignalTaskStatus) 
		{
			TaskComplete();
		}
		return true;
	}

	if (m_timePathRebuildMax == 0)
	{
		if ( fSignalTaskStatus )
			OnNavFailed( FAIL_NO_ROUTE );
		else
			OnNavFailed();
		return false;
	}
	else
	{
		if ( !bRetrying )
		{
			Remember(bits_MEMORY_PATH_FAILED);
			m_timePathRebuildFail = gpGlobals->curtime + m_timePathRebuildMax;
		}
		m_timePathRebuildNext = gpGlobals->curtime + m_timePathRebuildDelay;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Called when route fails.  Marks last link on which that failure
//			occured as stale so when then next node route is build that link
//			will be explictly checked for blockages
// Input  :
// Output : 
//-----------------------------------------------------------------------------
bool CAI_Navigator::MarkCurWaypointFailedLink( void )
{
	if ( TestingSteering() )
		return false;

	if ( !m_fRememberStaleNodes )
		return false;

	// Prevent a crash in release
	if( !GetPath() || !GetPath()->GetCurWaypoint() )
		return false;

	bool didMark = false;

	int startID =	GetPath()->GetLastNodeReached();
	int endID	=	GetPath()->GetCurWaypoint()->iNodeID;

	if ( endID != NO_NODE )
	{
		bool bBlockAll = false;

		if ( m_hLastBlockingEnt != NULL && 
			 !m_hLastBlockingEnt->IsPlayer() && !m_hLastBlockingEnt->IsNPC() &&
			 m_hLastBlockingEnt->GetMoveType() == MOVETYPE_VPHYSICS && 
			 m_hLastBlockingEnt->VPhysicsGetObject() && 
			 ( !m_hLastBlockingEnt->VPhysicsGetObject()->IsMoveable() || 
			   m_hLastBlockingEnt->VPhysicsGetObject()->GetMass() > 200 ) )
		{
			// Make sure it's a "large" object
			//		- One dimension is >40
			//		- Other 2 dimensions are >30
			CCollisionProperty *pCollisionProp = m_hLastBlockingEnt->CollisionProp();
			bool bFoundLarge = false;
			bool bFoundSmall = false;
			Vector vecSize = pCollisionProp->OBBMaxs() - pCollisionProp->OBBMins();
			for ( int i = 0; i < 3; i++ )
			{
				if ( vecSize[i] > 40 )
				{
					bFoundLarge = true;
				}

				if ( vecSize[i] < 30 )
				{
					bFoundSmall = true;
					break;
				}
			}

			if ( bFoundLarge && !bFoundSmall )
			{
				Vector vStartPos = GetNetwork()->GetNode( endID )->GetPosition( GetHullType() );
				Vector vEndPos = vStartPos;
				vEndPos.z += 0.01;
				trace_t tr;

				UTIL_TraceModel( vStartPos, vEndPos, GetHullMins(), GetHullMaxs(), m_hLastBlockingEnt, COLLISION_GROUP_NONE, &tr );

				if ( tr.startsolid )
					bBlockAll = true;
			}
		}

		if ( bBlockAll )
		{
			CAI_Node *pDestNode = GetNetwork()->GetNode( endID );
			for ( int i = 0; i < pDestNode->NumLinks(); i++ )
			{
				CAI_Link *pLink = pDestNode->GetLinkByIndex( i );
				pLink->m_LinkInfo |= bits_LINK_STALE_SUGGESTED;
				pLink->m_timeStaleExpires = gpGlobals->curtime + 4.0;
				didMark = true;
			}

		}
		else if ( startID != NO_NODE )
		{
			// Find link and mark it as stale
			CAI_Node *pNode = GetNetwork()->GetNode(startID);
			CAI_Link *pLink = pNode->GetLink( endID );
			if ( pLink )
			{
				pLink->m_LinkInfo |= bits_LINK_STALE_SUGGESTED;
				pLink->m_timeStaleExpires = gpGlobals->curtime + 4.0;
				didMark = true;
			}
		}
	}

	return didMark;
}

//-----------------------------------------------------------------------------
// Purpose: Builds a route to the given vecGoal using either local movement
//			or nodes
// Input  :
// Output : True is route was found, false otherwise
//-----------------------------------------------------------------------------

bool CAI_Navigator::DoFindPathToPos(void)
{
	CAI_Path *		pPath 			= GetPath();
	CAI_Pathfinder *pPathfinder 	= GetPathfinder();
	const Vector &	actualGoalPos 	= pPath->ActualGoalPosition();
	CBaseEntity *	pTarget 		= pPath->GetTarget();
	float 			tolerance 		= pPath->GetGoalTolerance();
	Vector			origin;
	
	if ( gpGlobals->curtime - m_flTimeClipped > 0.11  || m_bLastNavFailed )
		m_pClippedWaypoints->RemoveAll();

	if ( m_pClippedWaypoints->IsEmpty() )
		origin = GetLocalOrigin();
	else
	{
		AI_Waypoint_t *pLastClipped = m_pClippedWaypoints->GetLast();
		origin = pLastClipped->GetPos();
	}

	m_bLastNavFailed = false;

	pPath->ClearWaypoints();

	AI_Waypoint_t *pFirstWaypoint = pPathfinder->BuildRoute( origin, actualGoalPos, pTarget, tolerance, GetNavType(), m_bLocalSucceedOnWithinTolerance );

	if (!pFirstWaypoint)
	{
		//  Sorry no path
		return false;
	}

	pPath->SetWaypoints( pFirstWaypoint );

	if ( ShouldTestFailPath() )
	{
		pPath->ClearWaypoints();
		return false;
	}

	if ( !m_pClippedWaypoints->IsEmpty() )
	{
		AI_Waypoint_t *pFirstClipped = m_pClippedWaypoints->GetFirst();
		m_pClippedWaypoints->Set( NULL );
		pFirstClipped->ModifyFlags( bits_WP_DONT_SIMPLIFY, true );
		pPath->PrependWaypoints( pFirstClipped );
		pFirstWaypoint = pFirstClipped;
	}

	if (  pFirstWaypoint->IsReducible() && pFirstWaypoint->GetNext() && pFirstWaypoint->GetNext()->NavType() == GetNavType() &&
		  ShouldOptimizeInitialPathSegment( pFirstWaypoint ) )
	{
		// If we're seemingly beyond the waypoint, and our hull is over the line, move on
		const float EPS = 0.1;
		Vector vClosest;
		CalcClosestPointOnLineSegment( origin, 
									   pFirstWaypoint->GetPos(), pFirstWaypoint->GetNext()->GetPos(), 
									   vClosest );
		if ( ( pFirstWaypoint->GetPos() - vClosest ).Length() > EPS &&
			 ( origin - vClosest ).Length() < GetHullWidth() * 0.5 )
		{
			pPath->Advance();
		}
	}
	
	return true;
}


//-----------------------------------------------------------------------------

CBaseEntity *CAI_Navigator::GetNextPathcorner( CBaseEntity *pPathCorner )
{
	CBaseEntity *pNextPathCorner = NULL;

	Assert( pPathCorner );
	if ( pPathCorner )
	{
		pNextPathCorner = pPathCorner->GetNextTarget();

		CAI_Hint *pHint;
		if ( !pNextPathCorner && ( pHint = dynamic_cast<CAI_Hint *>( pPathCorner ) ) != NULL )
		{
			int targetNode = pHint->GetTargetNode();
			if ( targetNode != NO_NODE )
			{
				CAI_Node *pTestNode = GetNetwork()->GetNode(targetNode);
				pNextPathCorner = pTestNode->GetHint();
			}
		}
	}

	return pNextPathCorner;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::DoFindPathToPathcorner( CBaseEntity *pPathCorner )
{
// UNDONE: This is broken
// UNDONE: Remove this and change the pathing code to be able to refresh itself and support
// circular paths, etc.
	bool returnCode = false;
	Assert( GetPath()->GoalType() == GOALTYPE_PATHCORNER );

	// NPC is on a path_corner loop
	if ( pPathCorner != NULL )
	{
		// Find path to first pathcorner
		if ( ( GetGoalFlags() & AIN_NO_PATHCORNER_PATHFINDING ) || m_bNoPathcornerPathfinds )
		{
			// HACKHACK: If the starting path_corner has a speed, copy that to the entity
			if ( pPathCorner->m_flSpeed != 0 )
				SetSpeed( pPathCorner->m_flSpeed );

			GetPath()->ClearWaypoints();

			AI_Waypoint_t *pRoute = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), bits_WP_TO_PATHCORNER, NO_NODE );
			pRoute->hPathCorner = pPathCorner;
			AI_Waypoint_t *pLast = pRoute;
			pPathCorner = GetNextPathcorner(pPathCorner);
			if ( pPathCorner )
			{
				pLast = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), bits_WP_TO_PATHCORNER, NO_NODE );
				pLast->hPathCorner = pPathCorner;
				pRoute->SetNext(pLast);
			}
			pLast->ModifyFlags( bits_WP_TO_GOAL, true );
			GetPath()->SetWaypoints( pRoute, true );
			returnCode = true;
		}
		else
		{
			Vector initPos = pPathCorner->GetLocalOrigin();

			TranslateNavGoal( pPathCorner, initPos );

			GetPath()->ResetGoalPosition( initPos );
			
			float tolerance = GetPath()->GetGoalTolerance();
			float outerTolerance = GetOuter()->GetDefaultNavGoalTolerance();
			if ( outerTolerance > tolerance )
			{
				GetPath()->SetGoalTolerance( outerTolerance );
				tolerance = outerTolerance;
			}

			if ( ( returnCode = DoFindPathToPos() ) != false )
			{
				// HACKHACK: If the starting path_corner has a speed, copy that to the entity
				if ( pPathCorner->m_flSpeed != 0 )
				{
					SetSpeed( pPathCorner->m_flSpeed );
				}

				AI_Waypoint_t *lastWaypoint	= GetPath()->GetGoalWaypoint();
				Assert(lastWaypoint);

				lastWaypoint->ModifyFlags( bits_WP_TO_PATHCORNER, true );
				lastWaypoint->hPathCorner = pPathCorner;

				pPathCorner = GetNextPathcorner(pPathCorner); // first already accounted for in pathfind
				if ( pPathCorner )
				{
					// Place a dummy node in that will be used to signal the next pathfind, also prevents
					// animation system from decellerating when approaching a pathcorner
					lastWaypoint->ModifyFlags( bits_WP_TO_GOAL, false );
					// BRJ 10/4/02
					// FIXME: I'm not certain about the navtype here
					AI_Waypoint_t *curWaypoint = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL), NO_NODE );
					Vector waypointPos = curWaypoint->GetPos();
					TranslateNavGoal( pPathCorner, waypointPos );
					curWaypoint->SetPos( waypointPos );
					GetPath()->SetGoalTolerance( tolerance );
					curWaypoint->hPathCorner = pPathCorner;
					lastWaypoint->SetNext(curWaypoint);
					GetPath()->ResetGoalPosition( curWaypoint->GetPos() );
				}
			}
		}
	}
	return returnCode;
}

//-----------------------------------------------------------------------------
// Purpose: Attemps to find a route
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::DoFindPath( void )
{
	AI_PROFILE_SCOPE(CAI_Navigator_DoFindPath);

	DbgNavMsg( GetOuter(), "Finding new path\n" );

	GetPath()->ClearWaypoints();

	bool		returnCode;

	returnCode = false;

	switch( GetPath()->GoalType() )
	{
	case GOALTYPE_PATHCORNER:
		{
			returnCode = DoFindPathToPathcorner( GetGoalEnt() );
		}
		break;

	case GOALTYPE_ENEMY:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CBaseEntity *pEnemy = GetPath()->GetTarget();
			if (pEnemy)
			{
				Assert( pEnemy == GetEnemy() );

				Vector newPos = GetEnemyLKP();

				float tolerance = GetPath()->GetGoalTolerance();
				float outerTolerance = GetOuter()->GetDefaultNavGoalTolerance();
				if ( outerTolerance > tolerance )
				{
					GetPath()->SetGoalTolerance( outerTolerance );
					tolerance = outerTolerance;
				}
				
				TranslateNavGoal( pEnemy, newPos );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				GetPath()->ResetGoalPosition( newPos );
				GetPath()->SetGoalTolerance( tolerance );

				returnCode = DoFindPathToPos();
			}
		}
		break;

	case GOALTYPE_LOCATION:
	case GOALTYPE_FLANK:
	case GOALTYPE_COVER:
		returnCode = DoFindPathToPos();
		break;

	case GOALTYPE_LOCATION_NEAREST_NODE:
		{
			int myNodeID;
			int destNodeID;

			returnCode = false;
			
			myNodeID = GetPathfinder()->NearestNodeToNPC();
			if (myNodeID != NO_NODE)
			{
				destNodeID = GetPathfinder()->NearestNodeToPoint( GetPath()->ActualGoalPosition() );
				if (destNodeID != NO_NODE)
				{
					AI_Waypoint_t *pRoute = GetPathfinder()->FindBestPath( myNodeID, destNodeID );
					
					if ( pRoute != NULL )
					{
						GetPath()->SetWaypoints( pRoute );
						GetPath()->SetLastNodeAsGoal(true);
						returnCode = true;
					}
				}
			}
		}
		break;

	case GOALTYPE_TARGETENT:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CBaseEntity *pTarget = GetPath()->GetTarget();
			
			if ( pTarget )
			{
				Assert( pTarget == GetTarget() );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				
				Vector	initPos = pTarget->GetAbsOrigin();
				TranslateNavGoal( pTarget, initPos );

				GetPath()->ResetGoalPosition( initPos );
				returnCode = DoFindPathToPos();
			}
		}
		break;
	}

	return returnCode;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ClearPath( void )
{
	OnClearPath();

	m_timePathRebuildMax	= 0;					// How long to try rebuilding path before failing task
	m_timePathRebuildFail	= 0;					// Current global time when should fail building path
	m_timePathRebuildNext	= 0;					// Global time to try rebuilding again
	m_timePathRebuildDelay	= 0;					// How long to wait before trying to rebuild again

	Forget( bits_MEMORY_PATH_FAILED );

	AI_Waypoint_t *pWaypoint = GetPath()->GetCurWaypoint();

	if ( pWaypoint )
	{
		SaveStoppingPath();
		m_PreviousMoveActivity = GetMovementActivity();
		m_PreviousArrivalActivity = GetArrivalActivity();

		if( m_pClippedWaypoints && m_pClippedWaypoints->GetFirst() )
		{
			Assert( m_PreviousMoveActivity > ACT_RESET );
		}

		while ( pWaypoint )
		{
			if ( pWaypoint->iNodeID != NO_NODE )
			{
				CAI_Node *pNode = GetNetwork()->GetNode(pWaypoint->iNodeID);
				
				if ( pNode )
				{
					if ( pNode->IsLocked() )
						 pNode->Unlock();
				}
			}
			pWaypoint = pWaypoint->GetNext();
		}
	}

	GetPath()->Clear();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::GetStoppingPath( CAI_WaypointList *	pClippedWaypoints )
{
	pClippedWaypoints->RemoveAll();

	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	if ( pCurWaypoint )
	{
		bool bMustCompleteCurrent = ( pCurWaypoint->NavType() == NAV_CLIMB || pCurWaypoint->NavType() == NAV_JUMP );
		float distRemaining = GetMotor()->MinStoppingDist( 0 );

		if ( bMustCompleteCurrent )
		{
			float distToCurrent = ComputePathDistance( pCurWaypoint->NavType(), GetLocalOrigin(), pCurWaypoint->GetPos() );
			if ( pCurWaypoint->NavType() == NAV_CLIMB )
			{
				if ( pCurWaypoint->GetNext() && pCurWaypoint->GetNext()->NavType() == NAV_CLIMB )
					distToCurrent += ComputePathDistance( NAV_CLIMB, pCurWaypoint->GetPos(), pCurWaypoint->GetNext()->GetPos() );
				distToCurrent += GetHullWidth() * 2.0;
			}

			if ( distToCurrent > distRemaining )
				distRemaining = distToCurrent;
		}

		if ( bMustCompleteCurrent || distRemaining > 0.1 )
		{
			Vector		   vPosPrev		 = GetLocalOrigin();
			AI_Waypoint_t *pNextPoint 	 = pCurWaypoint;
			AI_Waypoint_t *pSavedWaypoints = NULL;
			AI_Waypoint_t *pLastSavedWaypoint = NULL;
			AI_Waypoint_t *pNewWaypoint;
			
			while ( distRemaining > 0.01  && pNextPoint )
			{
				if ( ( pNextPoint->NavType() == NAV_CLIMB || pNextPoint->NavType() == NAV_JUMP ) &&
					 !bMustCompleteCurrent )
				{
					break;
				}

#if PARANOID_NAV_CHECK_ON_MOMENTUM
				if ( !CanFitAtPosition( pNextPoint->GetPos(), MASK_NPCSOLID ) )
				{
					break;
				}
#endif

				if ( pNextPoint->NavType() != NAV_CLIMB || !pNextPoint->GetNext() || pNextPoint->GetNext()->NavType() != NAV_CLIMB )
					bMustCompleteCurrent = false;

				float distToNext = ComputePathDistance( pNextPoint->NavType(), vPosPrev, pNextPoint->GetPos() );

				if ( distToNext <= distRemaining + 0.01 )
				{
					pNewWaypoint = new AI_Waypoint_t(*pNextPoint);

					if ( pNewWaypoint->Flags() & bits_WP_TO_PATHCORNER )
					{
						pNewWaypoint->ModifyFlags( bits_WP_TO_PATHCORNER, false );
						pNewWaypoint->hPathCorner = NULL;
					}

					pNewWaypoint->ModifyFlags( bits_WP_TO_GOAL | bits_WP_TO_NODE, false );
					pNewWaypoint->iNodeID = NO_NODE;

					if ( pLastSavedWaypoint )
						pLastSavedWaypoint->SetNext( pNewWaypoint );
					else
						pSavedWaypoints = pNewWaypoint;
					pLastSavedWaypoint = pNewWaypoint;

					vPosPrev = pNextPoint->GetPos();

//					NDebugOverlay::Cross3D( vPosPrev, 16, 255, 255, 0, false, 10.0f );

					pNextPoint = pNextPoint->GetNext();
					distRemaining -= distToNext;
				}
				else
				{
					Assert( !( pNextPoint->NavType() == NAV_CLIMB || pNextPoint->NavType() == NAV_JUMP ) );
					Vector remainder = pNextPoint->GetPos() - vPosPrev;
					VectorNormalize( remainder );
					float yaw = UTIL_VecToYaw( remainder );
					remainder *= distRemaining;
					remainder += vPosPrev;

					AIMoveTrace_t trace;
					if ( GetMoveProbe()->MoveLimit( pNextPoint->NavType(), vPosPrev, remainder, MASK_NPCSOLID, NULL, 100, AIMLF_DEFAULT | AIMLF_2D, &trace ) )
					{
						pNewWaypoint = new AI_Waypoint_t( trace.vEndPosition, yaw, pNextPoint->NavType(), bits_WP_TO_GOAL, 0);

						if ( pLastSavedWaypoint )
							pLastSavedWaypoint->SetNext( pNewWaypoint );
						else
							pSavedWaypoints = pNewWaypoint;
					}

					distRemaining = 0;
				}

			}

			if ( pSavedWaypoints )
			{
				pClippedWaypoints->Set( pSavedWaypoints );
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
void CAI_Navigator::IgnoreStoppingPath( void )
{
	if( m_pClippedWaypoints && m_pClippedWaypoints->GetFirst() )
	{
		AI_Waypoint_t *pWaypoint = m_pClippedWaypoints->GetFirst();

		if( pWaypoint->NavType() != NAV_JUMP && pWaypoint->NavType() != NAV_CLIMB )
		{
			m_pClippedWaypoints->RemoveAll();
		}
	}
}

//-----------------------------------------------------------------------------

ConVar ai_use_clipped_paths( "ai_use_clipped_paths", "1" );

void CAI_Navigator::SaveStoppingPath( void )
{
	m_flTimeClipped = -1;

	m_pClippedWaypoints->RemoveAll();
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	if ( pCurWaypoint )
	{
		if ( ( pCurWaypoint->NavType() == NAV_CLIMB || pCurWaypoint->NavType() == NAV_JUMP ) || ai_use_clipped_paths.GetBool() )
		{	
			if ( GetStoppingPath( m_pClippedWaypoints ) )
				m_flTimeClipped = gpGlobals->curtime;
		}
	}
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::SetGoalFromStoppingPath()
{
	if ( m_pClippedWaypoints && m_pClippedWaypoints->IsEmpty() )
		SaveStoppingPath();
	if ( m_pClippedWaypoints && !m_pClippedWaypoints->IsEmpty() )
	{
		if ( m_PreviousMoveActivity <= ACT_RESET && GetMovementActivity() <= ACT_RESET  )
		{
			m_pClippedWaypoints->RemoveAll();
			DevWarning( 2, "%s has a stopping path and no valid. Movement activity: %s (prev %s)\n", GetOuter()->GetDebugName(), ActivityList_NameForIndex(GetMovementActivity()), ActivityList_NameForIndex(m_PreviousMoveActivity) );
			return false;
		}

		if ( ( m_pClippedWaypoints->GetFirst()->NavType() == NAV_CLIMB || m_pClippedWaypoints->GetFirst()->NavType() == NAV_JUMP ) )
		{
			const Task_t *pCurTask = GetOuter()->GetTask();
			if ( pCurTask && pCurTask->iTask == TASK_STOP_MOVING )
			{
				// Clipped paths are used for 2 reasons: Prepending movement that must be finished in the case of climbing/jumping,
				// and bringing an NPC to a stop. In the second case, we should never be starting climb or jump movement.
				m_pClippedWaypoints->RemoveAll();
				return false;
			}
		}

		if ( !GetPath()->IsEmpty() )
			GetPath()->ClearWaypoints();
		GetPath()->SetWaypoints( m_pClippedWaypoints->GetFirst(), true );
		m_pClippedWaypoints->Set( NULL );
		GetPath()->SetGoalType( GOALTYPE_NONE );
		GetPath()->SetGoalType( GOALTYPE_LOCATION );
		GetPath()->SetGoalTolerance( NAV_STOP_MOVING_TOLERANCE );
		Assert( GetPath()->GetCurWaypoint() );

		Assert( m_PreviousMoveActivity != ACT_INVALID );
		

		if ( m_PreviousMoveActivity != ACT_RESET )
			GetPath()->SetMovementActivity( m_PreviousMoveActivity );
		if ( m_PreviousArrivalActivity != ACT_RESET )
			GetPath()->SetArrivalActivity( m_PreviousArrivalActivity );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

static Vector GetRouteColor(Navigation_t navType, int waypointFlags)
{
	if (navType == NAV_JUMP)
	{
		return Vector(255,0,0);
	}

	if (waypointFlags & bits_WP_TO_GOAL)
	{
		return Vector(200,0,255);
	}
	
	if (waypointFlags & bits_WP_TO_DETOUR)
	{
		return Vector(0,200,255);
	}
	
	if (waypointFlags & bits_WP_TO_NODE)
	{
		return Vector(0,0,255);
	}

	else //if (waypointBits & bits_WP_TO_PATHCORNER)
	{
		return Vector(0,255,150);
	}
}


//-----------------------------------------------------------------------------
// Returns a color for debugging purposes
//-----------------------------------------------------------------------------
static Vector GetWaypointColor(Navigation_t navType)
{
	switch( navType )
	{
	case NAV_JUMP:
		return Vector(255,90,90);

	case NAV_GROUND:
		return Vector(0,255,255);

	case NAV_CLIMB:
		return Vector(90,255,255);

	case NAV_FLY:
		return Vector(90,90,255);

	default:
		return Vector(255,0,0);
	}
}

//-----------------------------------------------------------------------------

void CAI_Navigator::DrawDebugRouteOverlay(void)
{
	AI_Waypoint_t *waypoint = GetPath()->GetCurWaypoint();

	if (waypoint)
	{
		Vector RGB = GetRouteColor(waypoint->NavType(), waypoint->Flags());
		NDebugOverlay::Line(GetLocalOrigin(), waypoint->GetPos(), RGB[0],RGB[1],RGB[2], true,0);
	}

	while (waypoint) 
	{
		Vector RGB = GetWaypointColor(waypoint->NavType());
		NDebugOverlay::Box(waypoint->GetPos(), Vector(-3,-3,-3),Vector(3,3,3), RGB[0],RGB[1],RGB[2], true,0);

		if (waypoint->GetNext()) 
		{
			Vector RGB = GetRouteColor(waypoint->GetNext()->NavType(), waypoint->GetNext()->Flags());
			NDebugOverlay::Line(waypoint->GetPos(), waypoint->GetNext()->GetPos(),RGB[0],RGB[1],RGB[2], true,0);
		}
		waypoint = waypoint->GetNext();
	}

	if ( GetPath()->GoalType() != GOALTYPE_NONE )
	{
		Vector vecGoalPos = GetPath()->ActualGoalPosition();
		Vector vecGoalDir = GetPath()->GetGoalDirection( GetOuter()->GetAbsOrigin() );
		NDebugOverlay::Line( vecGoalPos, vecGoalPos + vecGoalDir * 32, 0,0,255, true, 2.0 );

		float flYaw = UTIL_VecToYaw( vecGoalDir );
		NDebugOverlay::Text( vecGoalPos, CFmtStr("yaw: %f", flYaw), true, 1 );
	}
}

//-----------------------------------------------------------------------------
