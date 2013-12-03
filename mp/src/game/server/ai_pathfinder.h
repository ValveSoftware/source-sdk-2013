//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_PATHFINDER_H
#define AI_PATHFINDER_H

#include "ai_component.h"
#include "ai_navtype.h"

#if defined( _WIN32 )
#pragma once
#endif

struct AIMoveTrace_t;
struct OverlayLine_t;
struct AI_Waypoint_t;
class CAI_Link;
class CAI_Network;
class CAI_Node;


//-----------------------------------------------------------------------------
// The type of route to build
enum RouteBuildFlags_e 
{
	bits_BUILD_GROUND		=			0x00000001, // 
	bits_BUILD_JUMP			=			0x00000002, //
	bits_BUILD_FLY			=			0x00000004, // 
	bits_BUILD_CLIMB		=			0x00000008, //
	bits_BUILD_GIVEWAY		=			0x00000010, //
	bits_BUILD_TRIANG		=			0x00000020, //
	bits_BUILD_IGNORE_NPCS	=			0x00000040, // Ignore collisions with NPCs
	bits_BUILD_COLLIDE_NPCS	=			0x00000080, // Use    collisions with NPCs (redundant for argument clarity)
	bits_BUILD_GET_CLOSE	=			0x00000100, // the route will be built even if it can't reach the destination
};

//-----------------------------------------------------------------------------
// CAI_Pathfinder
//
// Purpose: Executes pathfinds through an associated network.
//
//-----------------------------------------------------------------------------

class CAI_Pathfinder : public CAI_Component
{
public:
	CAI_Pathfinder( CAI_BaseNPC *pOuter )
	 :	CAI_Component(pOuter),
		m_flLastStaleLinkCheckTime( 0 ),
		m_pNetwork( NULL )
	{
	}

	void Init( CAI_Network *pNetwork );
	
	//---------------------------------
	
	int				NearestNodeToNPC();
	int				NearestNodeToPoint( const Vector &vecOrigin );

	AI_Waypoint_t*	FindBestPath		(int startID, int endID);
	AI_Waypoint_t*	FindShortRandomPath	(int startID, float minPathLength, const Vector &vDirection = vec3_origin);

	// --------------------------------

	bool			IsLinkUsable(CAI_Link *pLink, int startID);

	// --------------------------------
	
	AI_Waypoint_t *BuildRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType = NAV_NONE, bool bLocalSucceedOnWithinTolerance = false );
	void UnlockRouteNodes( AI_Waypoint_t * );

	// --------------------------------

	void SetIgnoreBadLinks()		{ m_bIgnoreStaleLinks = true; } // lasts only for the next pathfind

	// --------------------------------
	
	virtual AI_Waypoint_t *BuildNodeRoute( const Vector &vStart, const Vector &vEnd, int buildFlags, float goalTolerance );
	virtual AI_Waypoint_t *BuildLocalRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float goalTolerance);
	virtual AI_Waypoint_t *BuildRadialRoute( const Vector &vStartPos, const Vector &vCenterPos, const Vector &vGoalPos, float flRadius, float flArc, float flStepDist, bool bClockwise, float goalTolerance, bool bAirRoute );	
	
	virtual AI_Waypoint_t *BuildTriangulationRoute( const Vector &vStart, 
													const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID,
													float flYaw, float flDistToBlocker, Navigation_t navType);

	virtual AI_Waypoint_t *BuildOBBAvoidanceRoute(  const Vector &vStart, const Vector &vEnd, 
													const CBaseEntity *pObstruction, const CBaseEntity *pTarget, 
													Navigation_t navType );

	// --------------------------------
	
	bool Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEnd, 
						float flDistToBlocker, CBaseEntity const *pTargetEnt, Vector *pApex );
						
	// --------------------------------
	
	void DrawDebugGeometryOverlays( int m_debugOverlays );

protected:
	virtual bool	CanUseLocalNavigation() { return true; }

private:
	friend class CPathfindNearestNodeFilter;

	//---------------------------------

	AI_Waypoint_t*	RouteToNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance);
	AI_Waypoint_t*	RouteFromNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance);

	AI_Waypoint_t *	BuildNearestNodeRoute( const Vector &vGoal, bool bToNode, int buildFlags, float goalTolerance, int *pNearestNode );

	//---------------------------------
	
	AI_Waypoint_t*	MakeRouteFromParents(int *parentArray, int endID);
	AI_Waypoint_t*	CreateNodeWaypoint( Hull_t hullType, int nodeID, int nodeFlags = 0 );
	
	AI_Waypoint_t*	BuildRouteThroughPoints( Vector *vecPoints, int nNumPoints, int nDirection, int nStartIndex, int nEndIndex, Navigation_t navType, CBaseEntity *pTarget );

	bool			IsLinkStillStale(int moveType, CAI_Link *nodeLink);

	// --------------------------------
	
	// Builds a simple route (no triangulation, no making way)
	AI_Waypoint_t	*BuildSimpleRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd, 
		const CBaseEntity *pTarget, int endFlags, int nodeID, int nodeTargetType, float flYaw);

	// Builds a complex route (triangulation, making way)
	AI_Waypoint_t	*BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
		const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, 
		int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance );

	AI_Waypoint_t	*BuildGroundRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildFlyRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance );
	AI_Waypoint_t	*BuildJumpRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );
	AI_Waypoint_t	*BuildClimbRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity const *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw );

	// Computes the link type
	Navigation_t ComputeWaypointType( CAI_Node **ppNodes, int parentID, int destID );

	// --------------------------------
	
	bool TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
		const Vector &vecApex, const Vector &vecEnd, const CBaseEntity *pTargetEnt, AIMoveTrace_t *pStartTrace );

	// --------------------------------
	
	bool			CheckStaleRoute( const Vector &vStart, const Vector &vEnd, int moveTypes);
	bool			CheckStaleNavTypeRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd );

	// --------------------------------
	
	bool			CanGiveWay( const Vector& vStart, const Vector& vEnd, CBaseEntity *pNPCBlocker );

	// --------------------------------

	bool			UseStrongOptimizations();

	// --------------------------------
	// Debugging fields and functions

	class CTriDebugOverlay
	{
	public:
		CTriDebugOverlay()
		 :	m_debugTriOverlayLine( NULL )
		{
		}
		void AddTriOverlayLines( const Vector &vecStart, const Vector &vecApex, const Vector &vecEnd, const AIMoveTrace_t &startTrace, const AIMoveTrace_t &endTrace, bool bPathClear );
		void ClearTriOverlayLines(void);
		void FadeTriOverlayLines(void);

		void Draw(int npcDebugOverlays);
	private:
		void AddTriOverlayLine(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest);

		OverlayLine_t	**m_debugTriOverlayLine;
	};

	CTriDebugOverlay m_TriDebugOverlay;

	//---------------------------------
	
	float m_flLastStaleLinkCheckTime;	// Last time I check for a stale link
	bool m_bIgnoreStaleLinks;

	//---------------------------------
	
	CAI_Network *GetNetwork()				{ return m_pNetwork; }
	const CAI_Network *GetNetwork() const	{ return m_pNetwork; }
	
	CAI_Network *m_pNetwork;

public:
	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------

#endif // AI_PATHFINDER_H
