//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_NAVIGATOR_H
#define AI_NAVIGATOR_H

#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "ai_component.h"
#include "ai_navgoaltype.h"
#include "ai_navtype.h"
#include "ai_motor.h"

class CAI_BaseNPC;
class CAI_Motor;
class CAI_Route;
class CAI_Path;
class CAI_Pathfinder;
class CAI_LocalNavigator;
struct AI_Waypoint_t;
class CAI_WaypointList;
class CAI_Network;
struct AIMoveTrace_t;
struct AILocalMoveGoal_t;
typedef intp AI_TaskFailureCode_t;

//-----------------------------------------------------------------------------
// Debugging tools
//-----------------------------------------------------------------------------

#define DEBUG_AI_NAVIGATION 1
#ifdef DEBUG_AI_NAVIGATION
extern ConVar ai_debug_nav;
#define DbgNav() ai_debug_nav.GetBool()
#define DbgNavMsg( pAI, pszMsg ) \
	do \
	{ \
		if (DbgNav()) \
			DevMsg( pAI, "[Nav] %s", static_cast<const char *>(pszMsg) ); \
	} while (0)
#define DbgNavMsg1( pAI, pszMsg, a ) DbgNavMsg( pAI, CFmtStr(static_cast<const char *>(pszMsg), (a) ) )
#define DbgNavMsg2( pAI, pszMsg, a, b ) DbgNavMsg( pAI, CFmtStr(static_cast<const char *>(pszMsg), (a), (b) ) )
#else
#define DbgNav() false
#define DbgNavMsg( pAI, pszMsg ) ((void)0)
#define DbgNavMsg1( pAI, pszMsg, a ) ((void)0)
#define DbgNavMsg2( pAI, pszMsg, a, b ) ((void)0)
#endif


//-----------------------------------------------------------------------------
// STRUCTURES & ENUMERATIONS
//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE( AI_PathNode_t );

//-------------------------------------
// Purpose: Constants used to specify the properties of a requested navigation
//			goal.
//-------------------------------------

// Navigator should use the default or previously set tolerance
const float AIN_DEF_TOLERANCE  = -1.0;

// Navigator should use the hull size as the tolerance
const float AIN_HULL_TOLERANCE = -2.0;

// Goal does not specify a new activity
const Activity AIN_DEF_ACTIVITY = ACT_INVALID;

// Goal has no target
CBaseEntity * const AIN_NO_TARGET  = NULL;

// Goal does not specify a new target, use the existing one, if any
CBaseEntity * const AIN_DEF_TARGET = (AIN_NO_TARGET + 1);

// Goal does not specify a vector location
extern const Vector AIN_NO_DEST;

// Goal does not specify a node location
#define AIN_NO_NODE ((AI_PathNode_t)-1)


//-------------------------------------

enum AI_NavGoalFlags_t
{
	// While navigating, try to face the destination point
	AIN_YAW_TO_DEST			= 0x01,
	
	// If I'm a goal of type GOALTYPE_TARGETENT, update my goal position every time I think
	AIN_UPDATE_TARGET_POS	= 0x02,

	// If navigating on a designer placed path, don't use pathfinder between waypoints, just do it
	AIN_NO_PATHCORNER_PATHFINDING = 0x04,

	AIN_DEF_FLAGS			= 0,
};

//-------------------------------------

enum AI_NavSetGoalFlags_t
{
	// Reset the navigator's navigation to the default state
	AIN_CLEAR_PREVIOUS_STATE 	= 0x01,
	
	// Clear out the target entity, while retaining other settings
	AIN_CLEAR_TARGET			= 0x02,
	
	// If the navigate fails, return navigation to the default state
	AIN_DISCARD_IF_FAIL			= 0x04,

	// Don't signal TaskFail() if the pathfind fails, just return the result
	AIN_NO_PATH_TASK_FAIL		= 0x08,
};

//-------------------------------------

enum AI_NpcBlockHandling_t
{
	AISF_BLOCK,
	AISF_AVOID,
	AISF_IGNORE,
};

//-------------------------------------

enum AI_NavPathProgress_t
{
	AINPP_NO_CHANGE,
	AINPP_ADVANCED,
	AINPP_COMPLETE,
	AINPP_BLOCKED,
};

//-------------------------------------
// Purpose: Describes a navigation request. The various constructors simply
//			allow ease of use in the common cases.
//-------------------------------------

struct AI_NavGoal_t
{
	// Goal is unspecifed, or not a specific location
	AI_NavGoal_t( GoalType_t    type	  = GOALTYPE_INVALID,
				  Activity		activity  = AIN_DEF_ACTIVITY, 
				  float			tolerance = AIN_DEF_TOLERANCE,
				  unsigned 		flags     = AIN_DEF_FLAGS,
				  CBaseEntity * pTarget   = AIN_DEF_TARGET);
	
	// Goal is a specific location, and GOALTYPE_LOCATION
	AI_NavGoal_t( const Vector &dest,
				  Activity		activity  = AIN_DEF_ACTIVITY, 
				  float			tolerance = AIN_DEF_TOLERANCE,
				  unsigned 		flags     = AIN_DEF_FLAGS,
				  CBaseEntity * pTarget   = AIN_DEF_TARGET);

	// Goal is a specific location and goal type
	AI_NavGoal_t( GoalType_t 	type,
				  const Vector &dest,
				  Activity		activity  = AIN_DEF_ACTIVITY, 
				  float			tolerance = AIN_DEF_TOLERANCE,
				  unsigned 		flags     = AIN_DEF_FLAGS,
				  CBaseEntity * pTarget   = AIN_DEF_TARGET);

	// Goal is a specific node, and GOALTYPE_LOCATION
	AI_NavGoal_t( AI_PathNode_t	destNode,
				  Activity		activity  = AIN_DEF_ACTIVITY, 
				  float			tolerance = AIN_DEF_TOLERANCE,
				  unsigned 		flags     = AIN_DEF_FLAGS,
				  CBaseEntity *	pTarget   = AIN_DEF_TARGET);
				  
	// Goal is a specific location and goal type
	AI_NavGoal_t( GoalType_t	type,
				  AI_PathNode_t	destNode,
				  Activity		activity  = AIN_DEF_ACTIVITY, 
				  float			tolerance = AIN_DEF_TOLERANCE,
				  unsigned 		flags     = AIN_DEF_FLAGS,
				  CBaseEntity *	pTarget   = AIN_DEF_TARGET);
				  
	//----------------------------------
	
	// What type of goal is this
	GoalType_t 		type;

	// The destination, either as a vector, or as a path node
	Vector 			dest;
	AI_PathNode_t	destNode;

	// The activity to use, or none if a previosly set activity should be used
	Activity		activity;
	
	// The predicted activity used after arrival
	Activity		arrivalActivity;
	int				arrivalSequence;

	// The tolerance of success, or none if a previosly set tolerance should be used
	float			tolerance;

	// How far to permit an initial simplification of path
	// (will use default if this value is less than the default)
	float			maxInitialSimplificationDist;

	// Optional flags specifying
	unsigned		flags;

	// The target of the navigation, primarily used to ignore the entity in hull and line traces
	CBaseEntity *	pTarget;
};

//-------------------------------------
// Purpose: Used to describe rules for advance on a (fly) path. There's nothing
// 			specifically "flying" about it, other than it came from an attempte
//			to consolodate duplicated code in the various fliers. It may serve
//			a more general purpose in the future. The constructor takes those
//			arguments that can usually be specified just once (as in a 
//			local static constructor)
//-------------------------------------

struct AI_ProgressFlyPathParams_t
{
	AI_ProgressFlyPathParams_t( unsigned _collisionMask, 
							   	float _strictPointTolerance = 32.0, float _blockTolerance = 0.0,
							   	float _waypointTolerance = 100, float _goalTolerance = 12,
							   	AI_NpcBlockHandling_t _blockHandling = AISF_BLOCK )
	 :	collisionMask( _collisionMask ),
		strictPointTolerance( _strictPointTolerance ),
		blockTolerance( _blockTolerance ),
		waypointTolerance( _waypointTolerance ),
		goalTolerance( _goalTolerance ),
		blockHandling( _blockHandling ),
		pTarget( NULL ),
		bTrySimplify( true )
	{
	}

	void SetCurrent( const CBaseEntity *pNewTarget, bool bNewTrySimplify = true )
	{
		pTarget 	 = pNewTarget;
		bTrySimplify = bNewTrySimplify;
	}

	//----------------------------------
	
	// Fields that tend to stay constant
	unsigned 				collisionMask;
	float 					strictPointTolerance;
	float 					blockTolerance; 		// @TODO (toml 07-03-02): rename "blockTolerance". This is specifically the "simplify" block tolerance. See SimplifyFlyPath()
	float 					waypointTolerance;
	float 					goalTolerance;			// @TODO (toml 07-03-02): goalTolerance appears to have come into existence because
													// noone had set a good tolerance in the path itself. It is therefore redundant,
													// and more than likely should be excised
	AI_NpcBlockHandling_t 	blockHandling;			// @TODO (toml 07-03-02): rename "blockHandling". This is specifically the "simplify" block handling. See SimplifyFlyPath()

	// Fields that tend to change
	const CBaseEntity *		pTarget;
	bool 					bTrySimplify;
};

//-----------------------------------------------------------------------------
// CAI_Navigator
//
// Purpose: Implements pathing and path navigaton logic
//-----------------------------------------------------------------------------

class CAI_Navigator : public CAI_Component,
					  public CAI_DefMovementSink
{
	typedef CAI_Component BaseClass;
public:
	// --------------------------------
	
	CAI_Navigator(CAI_BaseNPC *pOuter);
	virtual ~CAI_Navigator();

	virtual void Init( CAI_Network *pNetwork );
	
	// --------------------------------

	void SetPathcornerPathfinding( bool fNewVal)					{ m_bNoPathcornerPathfinds = !fNewVal; }
	void SetRememberStaleNodes( bool fNewVal)						{ m_fRememberStaleNodes = fNewVal; }
	void SetValidateActivitySpeed( bool bValidateActivitySpeed )	{ m_bValidateActivitySpeed = bValidateActivitySpeed; }
	void SetLocalSucceedOnWithinTolerance( bool fNewVal )			{ m_bLocalSucceedOnWithinTolerance = fNewVal; }

	// --------------------------------

	void Save( ISave &save );
	void Restore( IRestore &restore );
	
	// --------------------------------
	// Methods to issue movement directives
	// --------------------------------

	// Simple pathfind
	virtual bool 		SetGoal( const AI_NavGoal_t &goal, unsigned flags = 0 );
	
	// Change the target of the path
	virtual bool		SetGoalTarget( CBaseEntity *pEntity, const Vector &offset );
	
	// Fancy pathing
	bool				SetRadialGoal( const Vector &destination, const Vector &center, float radius, float arc, float stepDist, bool bClockwise, bool bAirRoute = false );
	bool 				SetRandomGoal( float minPathLength, const Vector &dir = vec3_origin );
	bool 				SetRandomGoal( const Vector &from, float minPathLength, const Vector &dir = vec3_origin );
	bool				SetDirectGoal( const Vector &goalPos, Navigation_t navType = NAV_GROUND );
	
	bool				SetWanderGoal( float minRadius, float maxRadius );
	bool				SetVectorGoal( const Vector &dir, float targetDist, float minDist = 0, bool fShouldDeflect = false );
	bool				SetVectorGoalFromTarget( const Vector &goalPos, float minDist = 0, bool fShouldDeflect = false );

	bool				FindVectorGoal( Vector *pResult, const Vector &dir, float targetDist, float minDist = 0, bool fShouldDeflect = false );
	
	// Path manipulation
	bool 				PrependLocalAvoidance( float distObstacle, const AIMoveTrace_t &directTrace );
	void 				PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags = 0 );
	
	// Query or change the movement activity
	Activity 			GetMovementActivity() const;
	Activity 			SetMovementActivity(Activity activity);
	int		 			GetMovementSequence();
	void				SetMovementSequence( int sequence );

	// Query or change the Arrival activity
	Activity 			GetArrivalActivity() const;
	void				SetArrivalActivity( Activity activity );
	int					GetArrivalSequence( int curSequence );
	void				SetArrivalSequence( int sequence );

	// Set the facing direction at arrival
	void				SetArrivalDirection( const Vector &goalDirection );
	void				SetArrivalDirection( const QAngle &goalAngle );
	void				SetArrivalDirection( CBaseEntity *pTarget );
	Vector				GetArrivalDirection( );

	// Set the speed to reach at arrival (
	void				SetArrivalSpeed( float flSpeed );
	float				GetArrivalSpeed();

	// Set the estimated distance to stop before the actual goal
	void				SetArrivalDistance( float flDistance );
	float				GetArrivalDistance( ) const;

	// Query or change the goal tolerance
	float				GetGoalTolerance() const;
	void				SetGoalTolerance(float tolerance);

	GoalType_t			GetGoalType() const;
	const Vector &		GetGoalPos() const;
	CBaseEntity *		GetGoalTarget();
	int					GetGoalFlags() const;
	
	const Vector &		GetCurWaypointPos() const;
	int 				GetCurWaypointFlags() const;

	bool				CurWaypointIsGoal() const;

	bool				GetPointAlongPath( Vector *pResult, float distance, bool fReducibleOnly = false );

	float				GetPathDistanceToGoal();
	float				GetPathTimeToGoal();

	// Query if there is a current goal
	bool				IsGoalSet() const;

	// Query if the current goal is active, meaning the navigator has a path in can progress on
	bool				IsGoalActive() const;

	// Update the goal position to reflect current conditions
	bool 				RefindPathToGoal( bool fSignalTaskStatus = true, bool bDontIgnoreBadLinks = false );
	bool 				UpdateGoalPos( const Vector & );
	
	// Wrap up current locomotion
	void				StopMoving( bool bImmediate = true );

	// Discard the current goal, use StopMoving() if just executing a normal stop
	bool 				ClearGoal();

	// --------------------------------

	void				SetAllowBigStep( CBaseEntity *pEntToStepOff )	{ if ( !pEntToStepOff || !pEntToStepOff->IsWorld() ) m_hBigStepGroundEnt = pEntToStepOff; }

	// --------------------------------
	bool				SetGoalFromStoppingPath();
	void				IgnoreStoppingPath();
	
	// --------------------------------
	// Navigation mode
	// --------------------------------
	Navigation_t		GetNavType() const					{ return m_navType; }
	void				SetNavType( Navigation_t navType );

	bool 				IsInterruptable() const				{ return ( m_navType != NAV_CLIMB && m_navType != NAV_JUMP ); }
	
	// --------------------------------
	// Pathing
	// --------------------------------
	
	AI_NavPathProgress_t ProgressFlyPath( const AI_ProgressFlyPathParams_t &params); // note: will not return "blocked"

	AI_PathNode_t		GetNearestNode();
	Vector				GetNodePos( AI_PathNode_t );

	CAI_Network *		GetNetwork()						{ return m_pAINetwork; }
	const CAI_Network *	GetNetwork() const					{ return m_pAINetwork; }
	void 				SetNetwork( CAI_Network *pNetwork ) { m_pAINetwork = pNetwork; }
	
	CAI_Path *			GetPath()							{ return m_pPath; }
	const CAI_Path *	GetPath() const						{ return m_pPath; }

	void				AdvancePath();

	virtual bool		SimplifyPath( bool bFirstForPath = false, float maxDist = -1 );
	void				SimplifyFlyPath( unsigned collisionMask, const CBaseEntity *pTarget, 
										 float strictPointTolerance = 32.0, float blockTolerance = 0.0,
										 AI_NpcBlockHandling_t blockHandling = AISF_BLOCK);
	bool				SimplifyFlyPath(  const AI_ProgressFlyPathParams_t &params );
	
	bool				CanFitAtNode(int nodeNum, unsigned int collisionMask = MASK_NPCSOLID_BRUSHONLY); 
	float				MovementCost( int moveType, Vector &vecStart, Vector &vecEnd );

	bool				CanFitAtPosition( const Vector &vStartPos, unsigned int collisionMask, bool bIgnoreTransients = false, bool bAllowPlayerAvoid = true );
	bool				IsOnNetwork() const			{ return !m_bNotOnNetwork; }

	void				SetMaxRouteRebuildTime(float time) { m_timePathRebuildMax = time;			}

	// --------------------------------
	void				DrawDebugRouteOverlay( void );

	// --------------------------------
	// Miscellany
	// --------------------------------

	float				CalcYawSpeed();
	float				GetStepDownMultiplier();
	CBaseEntity *		GetNextPathcorner( CBaseEntity *pPathCorner );
	virtual void		OnScheduleChange();
	
	// --------------------------------
	
	// See comments at CAI_BaseNPC::Move()
	virtual bool		Move( float flInterval = 0.1 );

	// --------------------------------

	CBaseEntity *		GetBlockingEntity()	{ return m_hLastBlockingEnt; }
	
protected:
	// --------------------------------
	//
	// Common services provided by CAI_BaseNPC
	//
	CBaseEntity *		GetNavTargetEntity();
	void				TaskMovementComplete();
	float				MaxYawSpeed();
	void				SetSpeed( float );

	// --------------------------------

	CAI_Motor *			GetMotor()			{ return m_pMotor; }
	const CAI_Motor *	GetMotor() const	{ return m_pMotor; }

	CAI_MoveProbe *		GetMoveProbe()		{ return m_pMoveProbe; }
	const CAI_MoveProbe *GetMoveProbe() const { return m_pMoveProbe; }

	CAI_LocalNavigator *GetLocalNavigator()				{ return m_pLocalNavigator; }
	const CAI_LocalNavigator *GetLocalNavigator() const { return m_pLocalNavigator; }

	CAI_Pathfinder *	GetPathfinder();
	const CAI_Pathfinder *GetPathfinder() const;

	virtual void		OnClearPath(void);

	// --------------------------------
	
	virtual void OnNewGoal();
	virtual void OnNavComplete();
			void OnNavFailed( bool bMovement = false );
			void OnNavFailed( AI_TaskFailureCode_t code, bool bMovement = false );
			void OnNavFailed( const char *pszGeneralFailText, bool bMovement = false );
	
	// --------------------------------
	
 	virtual AIMoveResult_t MoveNormal();

	// Navigation execution
	virtual AIMoveResult_t MoveClimb();
	virtual AIMoveResult_t MoveJump();

	// --------------------------------
	
	virtual AIMoveResult_t MoveEnact( const AILocalMoveGoal_t &baseMove );
		
protected:
	// made this virtual so strider can implement hover behavior with a navigator
	virtual void 		MoveCalcBaseGoal(  AILocalMoveGoal_t *pMoveGoal);
	
private:
	virtual bool		OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool		OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool		OnFailedSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool 		OnFailedLocalNavigation( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool		OnInsufficientStopDist( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool		OnMoveStalled( const AILocalMoveGoal_t &move );
	virtual bool		OnMoveExecuteFailed( const AILocalMoveGoal_t &move, const AIMoveTrace_t &trace, AIMotorMoveResult_t fMotorResult, AIMoveResult_t *pResult );
	virtual bool		OnMoveBlocked( AIMoveResult_t *pResult );
	
	void ResetCalculations();
	
	// Methods shared between ground and fly movement
	bool 				PreMove();
	virtual bool 		MoveUpdateWaypoint( AIMoveResult_t *pResult );
	bool 				IsMovingOutOfWay( const AILocalMoveGoal_t &moveGoal, float distClear );
	bool 				DelayNavigationFailure( const AIMoveTrace_t &trace );

	static void			CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult );

	// --------------------------------
	// Pathfinding
	// --------------------------------

public:
	float				GetPathDistToCurWaypoint() const;
	float				GetPathDistToGoal() const;
	float				BuildAndGetPathDistToGoal();

	// --------------------------------

	int					GetNavFailCounter() const;
	void 				ClearNavFailCounter();
	float				GetLastNavFailTime() const;
	bool				TeleportAlongPath();

private:
	bool				DoFindPath( void );							// Find a route
	bool				DoFindPathToPathcorner( CBaseEntity *pPathCorner );

protected:
	virtual bool		DoFindPathToPos(void);
	virtual	bool		ShouldOptimizeInitialPathSegment( AI_Waypoint_t * ) { return true; }

private:
	void				ClearPath(void);
	void				SaveStoppingPath( void );

protected:
	virtual bool 		GetStoppingPath( CAI_WaypointList *pClippedWaypoints );

private:
	bool				FindPath( const AI_NavGoal_t &goal, unsigned flags );
	bool				FindPath( bool fSignalTaskStatus = true, bool bDontIgnoreBadLinks = false );
	bool				MarkCurWaypointFailedLink( void );			// Call when route fails

	struct SimplifyForwardScanParams
	{
		float scanDist;
		float radius;
		float increment;
		int maxSamples;
	};

	bool 				ShouldAttemptSimplifyTo( const Vector &pos );
	bool 				ShouldSimplifyTo( bool passedDetour, const Vector &pos );
	bool 				SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params );
	bool				SimplifyPathForwardScan( const SimplifyForwardScanParams &params, AI_Waypoint_t *pCurWaypoint, const Vector &curPoint, float distRemaining, bool skip, bool passedDetour, int *pTestCount );
	bool 				SimplifyPathForward( float maxDist = -1 );
	bool 				SimplifyPathBacktrack();
	bool 				SimplifyPathQuick();
	void				SimplifyPathInsertSimplification( AI_Waypoint_t *pSegmentStart, const Vector &point );

    // ---------------------------------

	static bool			ActivityIsLocomotive( Activity );
    
    // ---------------------------------

	Navigation_t		m_navType;									// My current navigation type (walk,fly)
	bool				m_fNavComplete;
	bool				m_bLastNavFailed;

	// Cached pointers to other components, for efficiency
	CAI_Motor *			m_pMotor;
	CAI_MoveProbe *		m_pMoveProbe;
	CAI_LocalNavigator *m_pLocalNavigator;

    // ---------------------------------

	CAI_Network*		m_pAINetwork;			 					// My current AINetwork
	CAI_Path*			m_pPath;									// My current route

	CAI_WaypointList *	m_pClippedWaypoints;
	float				m_flTimeClipped;
	Activity			m_PreviousMoveActivity;
	Activity			m_PreviousArrivalActivity;

	bool				m_bValidateActivitySpeed;
	bool				m_bCalledStartMove;
	
	bool				m_bNotOnNetwork;							// This NPC has no reachable nodes!

	float				m_flNextSimplifyTime;						// next time we should try to simplify our route
	bool				m_bForcedSimplify;
	float				m_flLastSuccessfulSimplifyTime;

	float				m_flTimeLastAvoidanceTriangulate;

	// --------------

	float				m_timePathRebuildMax;						// How long to try rebuilding path before failing task
	float				m_timePathRebuildDelay;						// How long to wait before trying to rebuild again

	float				m_timePathRebuildFail;						// Current global time when should fail building path
	float				m_timePathRebuildNext;						// Global time to try rebuilding again

	// --------------
	
	bool				m_fRememberStaleNodes;
	bool				m_bNoPathcornerPathfinds;
	bool				m_bLocalSucceedOnWithinTolerance;

	// --------------
	
	bool				m_fPeerMoveWait;
	EHANDLE				m_hPeerWaitingOn;
	CSimTimer 			m_PeerWaitMoveTimer;
	CSimTimer			m_PeerWaitClearTimer;

	CSimTimer			m_NextSidestepTimer;

	// --------------

	EHANDLE				m_hBigStepGroundEnt;
	EHANDLE				m_hLastBlockingEnt;

	// --------------

	Vector				m_vPosBeginFailedSteer;
	float				m_timeBeginFailedSteer;

	// --------------

	int					m_nNavFailCounter;
	float				m_flLastNavFailTime;
public:
	DECLARE_SIMPLE_DATADESC();
};


//-----------------------------------------------------------------------------
// AI_NavGoal_t inline methods
//-----------------------------------------------------------------------------

inline AI_NavGoal_t::AI_NavGoal_t( GoalType_t   type,
								   Activity		activity, 
								   float		tolerance,
								   unsigned 	flags,
								   CBaseEntity *pTarget)
 :	type(type),
	dest(AIN_NO_DEST),
	destNode(AIN_NO_NODE),
	activity(activity),
	tolerance(tolerance),
	maxInitialSimplificationDist(-1),
	flags(flags),
	pTarget(pTarget),
	arrivalActivity( AIN_DEF_ACTIVITY ),
	arrivalSequence( ACT_INVALID )
{
}

inline AI_NavGoal_t::AI_NavGoal_t( const Vector &dest,
								   Activity		activity, 
								   float		tolerance,
								   unsigned 	flags,
								   CBaseEntity *pTarget)
 :	type(GOALTYPE_LOCATION),
	dest(dest),
	destNode(AIN_NO_NODE),
	activity(activity),
	tolerance(tolerance),
	maxInitialSimplificationDist(-1),
	flags(flags),
	pTarget(pTarget),
	arrivalActivity( AIN_DEF_ACTIVITY ),
	arrivalSequence( ACT_INVALID )
{
}

inline AI_NavGoal_t::AI_NavGoal_t( GoalType_t 	type,
								   const Vector &dest,
								   Activity		activity, 
								   float		tolerance,
								   unsigned 	flags,
								   CBaseEntity *pTarget)
 :	type(type),
	dest(dest),
	destNode(AIN_NO_NODE),
	activity(activity),
	tolerance(tolerance),
	maxInitialSimplificationDist(-1),
	flags(flags),
	pTarget(pTarget),
	arrivalActivity( AIN_DEF_ACTIVITY ),
	arrivalSequence( ACT_INVALID )
{
}

inline AI_NavGoal_t::AI_NavGoal_t( AI_PathNode_t destNode,
								   Activity		 activity, 
								   float		 tolerance,
								   unsigned 	 flags,
								   CBaseEntity * pTarget)
 :	type(GOALTYPE_LOCATION),
	dest(AIN_NO_DEST),
	destNode(destNode),
	activity(activity),
	tolerance(tolerance),
	maxInitialSimplificationDist(-1),
	flags(flags),
	pTarget(pTarget),
	arrivalActivity( AIN_DEF_ACTIVITY ),
	arrivalSequence( ACT_INVALID )
{
}

inline AI_NavGoal_t::AI_NavGoal_t( GoalType_t	 type,
								   AI_PathNode_t destNode,
								   Activity		 activity, 
								   float		 tolerance,
								   unsigned 	 flags,
								   CBaseEntity * pTarget)
 :	type(type),
	dest(AIN_NO_DEST),
	destNode(destNode),
	activity(activity),
	tolerance(tolerance),
	maxInitialSimplificationDist(-1),
	flags(flags),
	pTarget(pTarget),
	arrivalActivity( AIN_DEF_ACTIVITY ),
	arrivalSequence( ACT_INVALID )
{
}

//-----------------------------------------------------------------------------

#endif // AI_NAVIGATOR_H
