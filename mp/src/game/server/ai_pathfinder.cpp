//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ndebugoverlay.h"

#include "ai_pathfinder.h"

#include "ai_basenpc.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_waypoint.h"
#include "ai_link.h"
#include "ai_routedist.h"
#include "ai_moveprobe.h"
#include "ai_dynamiclink.h"
#include "ai_hint.h"
#include "bitstring.h"

//@todo: bad dependency!
#include "ai_navigator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define NUM_NPC_DEBUG_OVERLAYS	  50

const float MAX_LOCAL_NAV_DIST_GROUND[2] = { (50*12), (25*12) };
const float MAX_LOCAL_NAV_DIST_FLY[2] = { (750*12), (750*12) };

//-----------------------------------------------------------------------------
// CAI_Pathfinder
//

BEGIN_SIMPLE_DATADESC( CAI_Pathfinder )

	//								m_TriDebugOverlay
	//								m_bIgnoreStaleLinks
  	DEFINE_FIELD( m_flLastStaleLinkCheckTime,		FIELD_TIME ),
	//								m_pNetwork

END_DATADESC()

//-----------------------------------------------------------------------------
// Compute move type bits to nav type
//-----------------------------------------------------------------------------
Navigation_t MoveBitsToNavType( int fBits )
{
	switch (fBits)
	{
	case bits_CAP_MOVE_GROUND:
		return NAV_GROUND;

	case bits_CAP_MOVE_FLY:
		return NAV_FLY;

	case bits_CAP_MOVE_CLIMB:
		return NAV_CLIMB;

	case bits_CAP_MOVE_JUMP:
		return NAV_JUMP;

	default:
		// This will only happen if more than one bit is set
		Assert(0);
		return NAV_NONE;
	}
}


//-----------------------------------------------------------------------------

void CAI_Pathfinder::Init( CAI_Network *pNetwork )
{
	Assert( pNetwork );
	m_pNetwork = pNetwork;
}
	
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::UseStrongOptimizations()
{
	if ( !AIStrongOpt() )
	{
		return false;
	}

#ifdef HL2_DLL
	if( GetOuter()->Classify() == CLASS_PLAYER_ALLY_VITAL )
	{
		return false;
	}
#endif//HL2_DLL
	return true;
}

//-----------------------------------------------------------------------------
// Computes the link type
//-----------------------------------------------------------------------------
Navigation_t CAI_Pathfinder::ComputeWaypointType( CAI_Node **ppNodes, int parentID, int destID )
{
	Navigation_t navType = NAV_NONE;

	CAI_Node *pNode = ppNodes[parentID];
	for (int link=0; link < pNode->NumLinks();link++) 
	{
		if (pNode->GetLinkByIndex(link)->DestNodeID(parentID) == destID)
		{
			// BRJ 10/1/02
			// FIXME: pNPC->CapabilitiesGet() is actually the mechanism by which fliers
			// filter out the bitfields in the waypoint type (most importantly, bits_MOVE_CAP_GROUND)
			// that would cause the waypoint distance to be computed in a 2D, as opposed to 3D fashion
			// This is a super-scary weak link if you ask me.
			int linkMoveTypeBits = pNode->GetLinkByIndex(link)->m_iAcceptedMoveTypes[GetHullType()];
			int moveTypeBits = ( linkMoveTypeBits & CapabilitiesGet());
			if ( !moveTypeBits && linkMoveTypeBits == bits_CAP_MOVE_JUMP )
			{
				Assert( pNode->GetHint() && pNode->GetHint()->HintType() == HINT_JUMP_OVERRIDE );
				ppNodes[destID]->Lock(0.3);
				moveTypeBits = linkMoveTypeBits;
			}
			Navigation_t linkType = MoveBitsToNavType( moveTypeBits );

			// This will only trigger if the links disagree about their nav type
			Assert( (navType == NAV_NONE) || (navType == linkType) );
			navType = linkType; 
			break;
		}
	}

	// @TODO (toml 10-15-02): one would not expect to come out of the above logic
	// with NAV_NONE. However, if a graph is newly built, it can contain malformed
	// links that are referred to by the destination node, not the source node.
	// This has to be fixed
	if ( navType == NAV_NONE )
	{
		pNode = ppNodes[destID];
		for (int link=0; link < pNode->NumLinks();link++) 
		{
			if (pNode->GetLinkByIndex(link)->DestNodeID(parentID) == destID)
			{
				int npcMoveBits = CapabilitiesGet();
				int nodeMoveBits = pNode->GetLinkByIndex(link)->m_iAcceptedMoveTypes[GetHullType()];
				int moveTypeBits = ( npcMoveBits & nodeMoveBits );
				Navigation_t linkType = MoveBitsToNavType( moveTypeBits );

				Assert( (navType == NAV_NONE) || (navType == linkType) );
				navType = linkType; 

				DevMsg( "Note: Strange link found between nodes in AI node graph\n" );
				break;
			}
		}
	}

	AssertMsg( navType != NAV_NONE, "Pathfinder appears to have output a path with consecutive nodes thate are not actually connected\n" );

	return navType;
}


//-----------------------------------------------------------------------------
// Purpose: Given an array of parentID's and endID, contruct a linked 
//			list of waypoints through those parents
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::MakeRouteFromParents( int *parentArray, int endID ) 
{
	AI_Waypoint_t *pOldWaypoint = NULL;
	AI_Waypoint_t *pNewWaypoint = NULL;
	int	currentID = endID;

	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	while (currentID != NO_NODE) 
	{
		// Try to link it to the previous waypoint
		int prevID = parentArray[currentID];

		int destID; 
		if (prevID != NO_NODE)
		{
			destID = prevID;
		}
		else
		{		   
			// If we have no previous node, then use the next node
			if ( !pOldWaypoint )
				return NULL;
			destID = pOldWaypoint->iNodeID;
		}

		Navigation_t waypointType = ComputeWaypointType( pAInode, currentID, destID );

		// BRJ 10/1/02
		// FIXME: It appears potentially possible for us to compute waypoints 
		// here which the NPC is not capable of traversing (because 
		// pNPC->CapabilitiesGet() in ComputeWaypointType() above filters it out). 
		// It's also possible if none of the lines have an appropriate DestNodeID.
		// Um, shouldn't such a waypoint not be allowed?!?!?
		Assert( waypointType != NAV_NONE );

		pNewWaypoint = new AI_Waypoint_t( pAInode[currentID]->GetPosition(GetHullType()),
			pAInode[currentID]->GetYaw(), waypointType, bits_WP_TO_NODE, currentID );

		// Link it up...
		pNewWaypoint->SetNext( pOldWaypoint );
		pOldWaypoint = pNewWaypoint;

		currentID = prevID;
	}

	return pOldWaypoint;
}


//------------------------------------------------------------------------------
// Purpose : Test if stale link is no longer stale
//------------------------------------------------------------------------------

bool CAI_Pathfinder::IsLinkStillStale(int moveType, CAI_Link *nodeLink)
{
	if ( m_bIgnoreStaleLinks )
		return false;

	if ( !(nodeLink->m_LinkInfo & bits_LINK_STALE_SUGGESTED ) )
		return false;

	if ( gpGlobals->curtime < nodeLink->m_timeStaleExpires )
		return true;

	// NPC should only check one stale link per think
	if (gpGlobals->curtime == m_flLastStaleLinkCheckTime)
	{
		return true;
	}
	else
	{
		m_flLastStaleLinkCheckTime = gpGlobals->curtime;
	}
	
	// Test movement, if suceeds, clear the stale bit
	if (CheckStaleRoute(GetNetwork()->GetNode(nodeLink->m_iSrcID)->GetPosition(GetHullType()),
		GetNetwork()->GetNode(nodeLink->m_iDestID)->GetPosition(GetHullType()), moveType))
	{
		nodeLink->m_LinkInfo &= ~bits_LINK_STALE_SUGGESTED;
		return false;
	}

	nodeLink->m_timeStaleExpires = gpGlobals->curtime + 1.0;

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int	CAI_Pathfinder::NearestNodeToNPC()
{
	return GetNetwork()->NearestNodeToPoint( GetOuter(), GetAbsOrigin() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CAI_Pathfinder::NearestNodeToPoint( const Vector &vecOrigin )
{
	return GetNetwork()->NearestNodeToPoint( GetOuter(), vecOrigin );
}

//-----------------------------------------------------------------------------
// Purpose: Build a path between two nodes
//-----------------------------------------------------------------------------

AI_Waypoint_t *CAI_Pathfinder::FindBestPath(int startID, int endID) 
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_FindBestPath );
	
	if ( !GetNetwork()->NumNodes() )
		return NULL;

#ifdef AI_PERF_MON
	m_nPerfStatPB++;
#endif

	int nNodes = GetNetwork()->NumNodes();
	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	CVarBitVec	openBS(nNodes);
	CVarBitVec	closeBS(nNodes);

	// ------------- INITIALIZE ------------------------
	float* nodeG = (float *)stackalloc( nNodes * sizeof(float) );
	float* nodeH = (float *)stackalloc( nNodes * sizeof(float) );
	float* nodeF = (float *)stackalloc( nNodes * sizeof(float) );
	int*   nodeP = (int *)stackalloc( nNodes * sizeof(int) );		// Node parent 

	for (int node=0;node<nNodes;node++)
	{
		nodeG[node] = FLT_MAX;
		nodeP[node] = -1;
	}

	nodeG[startID] = 0;

	nodeH[startID] = 0.1*(pAInode[startID]->GetPosition(GetHullType())-pAInode[endID]->GetPosition(GetHullType())).Length(); // Don't want to over estimate
	nodeF[startID] = nodeG[startID] + nodeH[startID];

	openBS.Set(startID);
	closeBS.Set( startID );

	// --------------- FIND BEST PATH ------------------
	while (!openBS.IsAllClear()) 
	{
		int smallestID = CAI_Network::FindBSSmallest(&openBS,nodeF,nNodes);
	
		openBS.Clear(smallestID);

		CAI_Node *pSmallestNode = pAInode[smallestID];
		
		if (GetOuter()->IsUnusableNode(smallestID, pSmallestNode->GetHint()))
			continue;

		if (smallestID == endID) 
		{
			AI_Waypoint_t* route = MakeRouteFromParents(&nodeP[0], endID);
			return route;
		}

		// Check this if the node is immediately in the path after the startNode 
		// that it isn't blocked
		for (int link=0; link < pSmallestNode->NumLinks();link++) 
		{
			CAI_Link *nodeLink = pSmallestNode->GetLinkByIndex(link);
			
			if (!IsLinkUsable(nodeLink,smallestID))
				continue;

			// FIXME: the cost function should take into account Node costs (danger, flanking, etc).
			int moveType = nodeLink->m_iAcceptedMoveTypes[GetHullType()] & CapabilitiesGet();
			int testID	 = nodeLink->DestNodeID(smallestID);

			Vector r1 = pSmallestNode->GetPosition(GetHullType());
			Vector r2 = pAInode[testID]->GetPosition(GetHullType());
			float dist   = GetOuter()->GetNavigator()->MovementCost( moveType, r1, r2 ); // MovementCost takes ref parameters!!

			if ( dist == FLT_MAX )
				continue;

			float new_g  = nodeG[smallestID] + dist;

			if ( !closeBS.IsBitSet(testID) || (new_g < nodeG[testID]) ) 
			{
				nodeP[testID] = smallestID;
				nodeG[testID] = new_g;
				nodeH[testID] = (pAInode[testID]->GetPosition(GetHullType())-pAInode[endID]->GetPosition(GetHullType())).Length();
				nodeF[testID] = nodeG[testID] + nodeH[testID];

				closeBS.Set( testID );
				openBS.Set( testID );
			}
		}
	}

	return NULL;   
}

//-----------------------------------------------------------------------------
// Purpose: Find a short random path of at least pathLength distance.  If
//			vDirection is given random path will expand in the given direction,
//			and then attempt to go generally straight
//-----------------------------------------------------------------------------

AI_Waypoint_t* CAI_Pathfinder::FindShortRandomPath(int startID, float minPathLength, const Vector &directionIn) 
{
	int				pNeighbor[AI_MAX_NODE_LINKS];
	int				pStaleNeighbor[AI_MAX_NODE_LINKS];
	int				numNeighbors		= 1;	// The start node
	int				numStaleNeighbors	= 0;
	int				neighborID			= NO_NODE;

	int nNodes = GetNetwork()->NumNodes();
	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	if ( !nNodes )
		return NULL;
	
	MARK_TASK_EXPENSIVE();

	int *nodeParent	= (int *)stackalloc( sizeof(int) * nNodes );
	CVarBitVec closeBS(nNodes);
	Vector vDirection = directionIn;

	// ------------------------------------------
	// Bail immediately if node has no neighbors
	// ------------------------------------------
	if (pAInode[startID]->NumLinks() == 0)
	{
		return NULL;
	}

	// ------------- INITIALIZE ------------------------
	nodeParent[startID] = NO_NODE;
	pNeighbor[0]		= startID;

	// --------------- FIND PATH ---------------------------------------------------------------
	// Quit when path is long enough, and I've run out of neighbors unless I'm on a climb node
	// in which case I'm not allowed to stop
	// -----------------------------------------------------------------------------------------
	float	pathLength	 = 0;
	int		nSearchCount = 0;
	while ( (pathLength < minPathLength) || 
			(neighborID != NO_NODE && pAInode[neighborID]->GetType() == NODE_CLIMB))
	{
		nSearchCount++;

		// If no neighbors try circling back to last node
		if (neighborID			!= NO_NODE	&&
			numNeighbors		== 0		&& 
			numStaleNeighbors	== 0		)
		{
			// If we dead ended on a climb node we've failed as we
			// aren't allowed to stop on a climb node
			if (pAInode[neighborID]->GetType() == NODE_CLIMB)
			{
				// If no neighbors exist we've failed.
				return NULL;
			}
			// Otherwise accept this path to a dead end
			else
			{
				AI_Waypoint_t* route = MakeRouteFromParents(&nodeParent[0], neighborID);
				return route;
			}
		}

		// ----------------------
		//  Pick a neighbor
		// ----------------------
		int lastID = neighborID;

		// If vDirection is non-zero attempt to expand close to current direction
		if (vDirection != vec3_origin)
		{
			float	bestDot		= -1;
			Vector	vLastPos;

			if (lastID == NO_NODE)
			{
				vLastPos = GetLocalOrigin();
			}
			else
			{
				vLastPos = pAInode[lastID]->GetOrigin();
			}

			// If no neighbors, try using a stale one
			if (numNeighbors == 0)
			{
				neighborID = pStaleNeighbor[random->RandomInt(0,numStaleNeighbors-1)];
			}
			else
			{
				for (int i=0;i<numNeighbors;i++)
				{
					Vector nodeDir = vLastPos - pAInode[pNeighbor[i]]->GetOrigin();
					VectorNormalize(nodeDir);
					float fDotPr = DotProduct(vDirection,nodeDir);
					if (fDotPr > bestDot)
					{
						bestDot = fDotPr;
						neighborID = pNeighbor[i];
					}
				}
			}

			if (neighborID != NO_NODE)
			{
				vDirection = vLastPos - pAInode[neighborID]->GetOrigin();
				VectorNormalize(vDirection);
			}

		}
		// Pick random neighbor 
		else if (numNeighbors != 0)
		{
			neighborID = pNeighbor[random->RandomInt(0,numNeighbors-1)];
		}
		// If no neighbors, try using a stale one
		else
		{
			neighborID = pStaleNeighbor[random->RandomInt(0,numStaleNeighbors-1)];
		}

		// BUGBUG: This routine is totally hosed!
		if ( neighborID < 0 )
			return NULL;

		// Set previous nodes parent
		nodeParent[neighborID] = lastID;
		closeBS.Set(neighborID);

		// Add the new length
		if (lastID != NO_NODE)
		{
			pathLength += (pAInode[lastID]->GetOrigin() - pAInode[neighborID]->GetOrigin()).Length();
		}

		// If path is long enough or we've hit a maximum number of search nodes,
		// we're done unless we've ended on a climb node
		if ((pathLength >= minPathLength || nSearchCount > 20) &&
			pAInode[neighborID]->GetType() != NODE_CLIMB)
		{
			return MakeRouteFromParents(&nodeParent[0], neighborID);
		}

		// Clear neighbors
		numNeighbors		= 0;
		numStaleNeighbors	= 0;

		// Now add in new neighbors, pick links in different order ever time
		pAInode[neighborID]->ShuffleLinks();
		for (int link=0; link < pAInode[neighborID]->NumLinks();link++) 
		{
			if ( numStaleNeighbors == ARRAYSIZE(pStaleNeighbor) )
			{
				AssertMsg( 0, "Array overflow" );
				return NULL;
			}
			if ( numNeighbors == ARRAYSIZE(pStaleNeighbor) )
			{
				AssertMsg( 0, "Array overflow" );
				return NULL;
			}

			CAI_Link*	nodeLink = pAInode[neighborID]->GetShuffeledLink(link);
			int			testID	 = nodeLink->DestNodeID(neighborID);

			// --------------------------------------------------------------------------
			//  Don't loop
			// --------------------------------------------------------------------------
			if (closeBS.IsBitSet(testID))
			{
				continue;
			}

			// --------------------------------------------------------------------------
			// Don't go back to the node I just visited
			// --------------------------------------------------------------------------
			if (testID == lastID)
			{
				continue; 
			}

			// --------------------------------------------------------------------------
			// Make sure link is valid
			// --------------------------------------------------------------------------
			if (!IsLinkUsable(nodeLink,neighborID))
			{
				continue;
			}

			// --------------------------------------------------------------------------
			// If its a stale node add to stale list
			// --------------------------------------------------------------------------
			if (pAInode[testID]->IsLocked())
			{
				pStaleNeighbor[numStaleNeighbors]=testID;
				numStaleNeighbors++;
			}

			// --------------------------------------
			//  Add to list of non-stale neighbors
			// --------------------------------------
			else
			{
				pNeighbor[numNeighbors]=testID;
				numNeighbors++;
			}
		}
	}
	// Failed to get a path of full length, but return what we have
	return MakeRouteFromParents(&nodeParent[0], neighborID);
}

//------------------------------------------------------------------------------
// Purpose : Returns true is link us usable by the given NPC from the
//			 startID node.
//------------------------------------------------------------------------------

bool CAI_Pathfinder::IsLinkUsable(CAI_Link *pLink, int startID)
{
	// --------------------------------------------------------------------------
	// Skip if link turned off
	// --------------------------------------------------------------------------
	if (pLink->m_LinkInfo & bits_LINK_OFF)
	{
		CAI_DynamicLink *pDynamicLink = pLink->m_pDynamicLink;

		if ( !pDynamicLink || pDynamicLink->m_strAllowUse == NULL_STRING )
			return false;

		const char *pszAllowUse = STRING( pDynamicLink->m_strAllowUse );
		if ( pDynamicLink->m_bInvertAllow )
		{
			// Exlude only the specified entity name or classname
			if ( GetOuter()->NameMatches(pszAllowUse) || GetOuter()->ClassMatches( pszAllowUse ) )
				return false;
		}
		else
		{
			// Exclude everything but the allowed entity name or classname
			if ( !GetOuter()->NameMatches( pszAllowUse) && !GetOuter()->ClassMatches( pszAllowUse ) )
				return false;
		}
	}

	// --------------------------------------------------------------------------			
	//  Get the destination nodeID
	// --------------------------------------------------------------------------			
	int endID = pLink->DestNodeID(startID);

	// --------------------------------------------------------------------------
	// Make sure I have the ability to do the type of movement specified by the link
	// --------------------------------------------------------------------------
	int linkMoveTypes = pLink->m_iAcceptedMoveTypes[GetHullType()];
	int moveType = ( linkMoveTypes & CapabilitiesGet() );

	CAI_Node *pStartNode,*pEndNode;

	pStartNode = GetNetwork()->GetNode(startID);
	pEndNode = GetNetwork()->GetNode(endID);

	if ( (linkMoveTypes & bits_CAP_MOVE_JUMP) && !moveType )
	{
		CAI_Hint *pStartHint = pStartNode->GetHint();
		CAI_Hint *pEndHint = pEndNode->GetHint();
		if ( pStartHint && pEndHint )
		{
			if ( pStartHint->HintType() == HINT_JUMP_OVERRIDE && 
				 pEndHint->HintType() == HINT_JUMP_OVERRIDE &&
				 ( ( ( pStartHint->GetSpawnFlags() | pEndHint->GetSpawnFlags() ) & SF_ALLOW_JUMP_UP ) || pStartHint->GetAbsOrigin().z > pEndHint->GetAbsOrigin().z ) )
			{
				if ( !pStartNode->IsLocked() )
				{
					if ( pStartHint->GetTargetNode() == -1 || pStartHint->GetTargetNode() == endID )
						moveType = bits_CAP_MOVE_JUMP;
				}
			}
		}
	}

	if (!moveType)
	{
		return false;
	}

	// --------------------------------------------------------------------------
	// Check if NPC has a reason not to use the desintion node
	// --------------------------------------------------------------------------
	if (GetOuter()->IsUnusableNode(endID, pEndNode->GetHint()))
	{
		return false;
	}	

	// --------------------------------------------------------------------------
	// If a jump make sure the jump is within NPC's legal parameters for jumping
	// --------------------------------------------------------------------------
	if (moveType == bits_CAP_MOVE_JUMP)
	{	
		if (!GetOuter()->IsJumpLegal(pStartNode->GetPosition(GetHullType()), 
									 pEndNode->GetPosition(GetHullType()),
									 pEndNode->GetPosition(GetHullType())))
		{
			return false;
		}
	}

	// --------------------------------------------------------------------------
	// If an NPC suggested that this link is stale and I haven't checked it yet
	// I should make sure the link is still valid before proceeding
	// --------------------------------------------------------------------------
	if (pLink->m_LinkInfo & bits_LINK_STALE_SUGGESTED)
	{
		if (IsLinkStillStale(moveType, pLink))
		{
			return false;
		}
	}
	return true;
}

//-----------------------------------------------------------------------------

static int NPCBuildFlags( CAI_BaseNPC *pNPC, const Vector &vecOrigin )
{
	// If vecOrigin the the npc's position and npc is climbing only climb nodes allowed
	if (pNPC->GetLocalOrigin() == vecOrigin && pNPC->GetNavType() == NAV_CLIMB) 
	{
		return bits_BUILD_CLIMB;
	}
	else if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
	{
		return bits_BUILD_FLY | bits_BUILD_GIVEWAY;
	}
	else if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_GROUND)
	{
		int buildFlags = bits_BUILD_GROUND | bits_BUILD_GIVEWAY;
		if (pNPC->CapabilitiesGet() & bits_CAP_MOVE_JUMP)
		{
			buildFlags |= bits_BUILD_JUMP;
		}

		return buildFlags;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Creates a node waypoint
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::CreateNodeWaypoint( Hull_t hullType, int nodeID, int nodeFlags )
{
	CAI_Node *pNode = GetNetwork()->GetNode(nodeID);

	Navigation_t navType;
	switch(pNode->GetType())
	{
	case NODE_CLIMB:
		navType = NAV_CLIMB;
		break;

	case NODE_AIR:
		navType = NAV_FLY;
		break;

	default:
		navType = NAV_GROUND;
		break;
	}

	return new AI_Waypoint_t( pNode->GetPosition(hullType), pNode->GetYaw(), navType, ( bits_WP_TO_NODE | nodeFlags) , nodeID );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a route to a node for the given npc with the given 
//			build flags
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::RouteToNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_RouteToNode );
	
	buildFlags |= NPCBuildFlags( GetOuter(), vecOrigin );
	buildFlags &= ~bits_BUILD_GET_CLOSE;

	// Check if vecOrigin is already at the smallest node

	// FIXME: an equals check is a bit sloppy, this should be a tolerance
	const Vector &vecNodePosition = GetNetwork()->GetNode(nodeID)->GetPosition(GetHullType());
	if (vecOrigin == vecNodePosition)
	{
		return CreateNodeWaypoint( GetHullType(), nodeID, bits_WP_TO_GOAL );
	}

	// Otherwise try to build a local route to the node
	AI_Waypoint_t *pResult = BuildLocalRoute(vecOrigin, 
		vecNodePosition, NULL, bits_WP_TO_NODE, nodeID, buildFlags, goalTolerance);
	if ( pResult )
		pResult->iNodeID = nodeID;
	return pResult;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a route to a node for the given npc with the given 
//			build flags
//-----------------------------------------------------------------------------

AI_Waypoint_t* CAI_Pathfinder::RouteFromNode(const Vector &vecOrigin, int buildFlags, int nodeID, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_RouteFromNode );
	
	buildFlags |= NPCBuildFlags( GetOuter(), vecOrigin );
	buildFlags |= bits_BUILD_GET_CLOSE;

	// Check if vecOrigin is already at the smallest node
	// FIXME: an equals check is a bit sloppy, this should be a tolerance
	CAI_Node *pNode = GetNetwork()->GetNode(nodeID);
	const Vector &vecNodePosition = pNode->GetPosition(GetHullType());

	if (vecOrigin == vecNodePosition)
	{
		return CreateNodeWaypoint( GetHullType(), nodeID, bits_WP_TO_GOAL );
	}

	// Otherwise try to build a local route from the node
	AI_Waypoint_t* pResult = BuildLocalRoute( vecNodePosition, 
											  vecOrigin, NULL, bits_WP_TO_GOAL, NO_NODE, buildFlags, goalTolerance);

	// Handle case of target hanging over edge near climb dismount
	if ( !pResult &&
		 pNode->GetType() == NODE_CLIMB && 
		 ( vecOrigin - vecNodePosition ).Length2DSqr() < 32.0*32.0 &&
		 GetOuter()->GetMoveProbe()->CheckStandPosition(vecNodePosition, MASK_NPCSOLID_BRUSHONLY ) )
	{
		pResult = new AI_Waypoint_t( vecOrigin, 0, NAV_GROUND, bits_WP_TO_GOAL, nodeID );
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// Builds a simple route (no triangulation, no making way)
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildSimpleRoute( Navigation_t navType, const Vector &vStart, 
	const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, 
	int nodeTargetType, float flYaw )
{
	Assert( navType == NAV_JUMP || navType == NAV_CLIMB ); // this is what this here function is for
	// Only allowed to jump to ground nodes
	if ((nodeID == NO_NODE)	|| (GetNetwork()->GetNode(nodeID)->GetType() == nodeTargetType) )
	{
		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID, pTarget, &moveTrace );

		// If I was able to make the move, or the vEnd is the
		// goal and I'm within tolerance, just move to vEnd
		if (!IsMoveBlocked(moveTrace))
		{
			// It worked so return a route of length one to the endpoint
			return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Builds a complex route (triangulation, making way)
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildComplexRoute( Navigation_t navType, const Vector &vStart, 
	const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, 
	int buildFlags, float flYaw, float goalTolerance, float maxLocalNavDistance )
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildComplexRoute );
	
	float flTotalDist = ComputePathDistance( navType, vStart, vEnd );
	if ( flTotalDist < 0.0625 )
	{
		return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );
	}
	
	unsigned int collideFlags = (buildFlags & bits_BUILD_IGNORE_NPCS) ? MASK_NPCSOLID_BRUSHONLY : MASK_NPCSOLID;

	bool bCheckGround = (GetOuter()->CapabilitiesGet() & bits_CAP_SKIP_NAV_GROUND_CHECK) ? false : true;

	if ( flTotalDist <= maxLocalNavDistance )
	{
		AIMoveTrace_t moveTrace;

		AI_PROFILE_SCOPE_BEGIN( CAI_Pathfinder_BuildComplexRoute_Direct );
	
		GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, collideFlags, pTarget, (bCheckGround) ? 100 : 0, &moveTrace);

		// If I was able to make the move...
		if (!IsMoveBlocked(moveTrace))
		{
			// It worked so return a route of length one to the endpoint
			return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );
		}
	
		// ...or the vEnd is thegoal and I'm within tolerance, just move to vEnd
		if ( (buildFlags & bits_BUILD_GET_CLOSE) && 
			 (endFlags & bits_WP_TO_GOAL) && 
			 moveTrace.flDistObstructed <= goalTolerance ) 
		{
			return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );
		}

		AI_PROFILE_SCOPE_END();

		// -------------------------------------------------------------------
		// Try to triangulate if requested
		// -------------------------------------------------------------------

		AI_PROFILE_SCOPE_BEGIN( CAI_Pathfinder_BuildComplexRoute_Triangulate );
		
		if (buildFlags & bits_BUILD_TRIANG)
		{
			if ( !UseStrongOptimizations() || ( GetOuter()->GetState() == NPC_STATE_SCRIPT || GetOuter()->IsCurSchedule( SCHED_SCENE_GENERIC, false ) ) )
			{
				float flTotalDist = ComputePathDistance( navType, vStart, vEnd );

				AI_Waypoint_t *triangRoute = BuildTriangulationRoute(vStart, vEnd, pTarget, 
					endFlags, nodeID, flYaw, flTotalDist - moveTrace.flDistObstructed, navType);

				if (triangRoute)
				{
					return triangRoute;
				}
			}
		}
		
		AI_PROFILE_SCOPE_END();

		// -------------------------------------------------------------------
		// Try to giveway if requested
		// -------------------------------------------------------------------
		if (moveTrace.fStatus == AIMR_BLOCKED_NPC && (buildFlags & bits_BUILD_GIVEWAY))
		{
			// If I can't get there even ignoring NPCs, don't bother to request a giveway
			AIMoveTrace_t moveTrace2;
			GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID_BRUSHONLY, pTarget, (bCheckGround) ? 100 : 0, &moveTrace2 );

			if (!IsMoveBlocked(moveTrace2))
			{							
				// If I can clear the way return a route of length one to the target location
				if ( CanGiveWay(vStart, vEnd, moveTrace.pObstruction) )
				{
					return new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );
				}
			}
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a jump route between vStart
//			and vEnd, ignoring entity pTarget
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildJumpRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw)
{
	// Only allowed to jump to ground nodes
	return BuildSimpleRoute( NAV_JUMP, vStart, vEnd, pTarget, 
		endFlags, nodeID, NODE_GROUND, flYaw );
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a climb route between vStart
//			and vEnd, ignoring entity pTarget
// Input  :
// Output : Returns a route if sucessful or NULL if no climb route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildClimbRoute(const Vector &vStart, const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw)
{
	// Only allowed to climb to climb nodes
	return BuildSimpleRoute( NAV_CLIMB, vStart, vEnd, pTarget, 
		endFlags, nodeID, NODE_CLIMB, flYaw );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a ground route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if sucessful or NULL if no ground route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildGroundRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance)
{
	return BuildComplexRoute( NAV_GROUND, vStart, vEnd, pTarget, 
		endFlags, nodeID, buildFlags, flYaw, goalTolerance, MAX_LOCAL_NAV_DIST_GROUND[UseStrongOptimizations()] );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a fly route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if sucessful or NULL if no ground route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildFlyRoute(const Vector &vStart, const Vector &vEnd, 
	const CBaseEntity *pTarget, int endFlags, int nodeID, int buildFlags, float flYaw, float goalTolerance)
{
	return BuildComplexRoute( NAV_FLY, vStart, vEnd, pTarget, 
		endFlags, nodeID, buildFlags, flYaw, goalTolerance, MAX_LOCAL_NAV_DIST_FLY[UseStrongOptimizations()] );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a route between vStart and vEnd, requesting the
//			pNPCBlocker to get out of the way
// Input  :
// Output : Returns a route if sucessful or NULL if giveway failed
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CanGiveWay( const Vector& vStart, const Vector& vEnd, CBaseEntity *pBlocker)
{
	// FIXME: make this a CAI_BaseNPC member function
	CAI_BaseNPC *pNPCBlocker = pBlocker->MyNPCPointer();
	if (pNPCBlocker && pNPCBlocker->edict()) 
	{
		Disposition_t eDispBlockerToMe = pNPCBlocker->IRelationType( GetOuter() );
		if ( ( eDispBlockerToMe == D_LI ) || ( eDispBlockerToMe == D_NU ) )
		{
			return true;
		}
		
		return false;

		// FIXME: this is called in route creation, not navigation.  It shouldn't actually make
		// anyone get out of their way, just see if they'll honor the request.
		// things like locked doors, enemies and such should refuse, all others shouldn't.
		// things like breakables should know who is trying to break them, though a door hidden behind
		// some boxes shouldn't be known to the AI even though a route should connect through them but
		// be turned off.

		/*
		Vector moveDir	   = (vEnd - vStart).Normalize();
		Vector blockerDir  = (pNPCBlocker->GetLocalOrigin()  - vStart);
		float  blockerDist = DotProduct(moveDir,blockerDir);
		Vector blockPos	   = vStart + (moveDir*blockerDist);

		if (pNPCBlocker->RequestGiveWay ( m_owner->GetLocalOrigin(), blockPos, moveDir, m_owner->m_eHull))
		{
			return true;
		}
		*/
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a triangulation route between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance and
//			triangulating around a blocking object at blockDist
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildTriangulationRoute(
	  const Vector &vStart, // from where
	  const Vector &vEnd,	// to where
	  const CBaseEntity *pTarget,	// an entity I can ignore
	  int endFlags,			// add these WP flags to the last waypoint
	  int nodeID,			// node id for the last waypoint
	  float flYaw,			// ideal yaw for the last waypoint
	  float flDistToBlocker,// how far away is the obstruction from the start?
	  Navigation_t navType)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildTriangulationRoute );
	
	Vector vApex;
	if (!Triangulate(navType, vStart, vEnd, flDistToBlocker, pTarget, &vApex ))
		return NULL;

	//-----------------------------------------------------------------------------
	// it worked, create a route
	//-----------------------------------------------------------------------------
	AI_Waypoint_t *pWayPoint2 = new AI_Waypoint_t( vEnd, flYaw, navType, endFlags, nodeID );

	// FIXME: Compute a reasonable yaw here
	AI_Waypoint_t *waypoint1 = new AI_Waypoint_t( vApex, 0, navType, bits_WP_TO_DETOUR, NO_NODE );
	waypoint1->SetNext(pWayPoint2);

	return waypoint1;
}

//-----------------------------------------------------------------------------
// Purpose: Get the next node (with wrapping) around a circularly wound path
// Input  : nLastNode - The starting node
//			nDirection - Direction we're moving
//			nNumNodes - Total nodes in the chain
//-----------------------------------------------------------------------------
inline int GetNextPoint( int nLastNode, int nDirection, int nNumNodes )
{
	int nNextNode = nLastNode + nDirection;
	if ( nNextNode > (nNumNodes-1) )
		nNextNode = 0;
	else if ( nNextNode < 0 )
		nNextNode = (nNumNodes-1);

	return nNextNode;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to wind a route through a series of node points in a specified direction.
// Input  : *vecCorners - Points to test between
//			nNumCorners - Number of points to test
//			&vecStart - Starting position
//			&vecEnd - Ending position
// Output : Route through the points
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildRouteThroughPoints( Vector *vecPoints, int nNumPoints, int nDirection, int nStartIndex, int nEndIndex, Navigation_t navType, CBaseEntity *pTarget )
{
	AIMoveTrace_t endTrace;
	endTrace.fStatus = AIMR_OK;

	CAI_MoveProbe *pMoveProbe = GetOuter()->GetMoveProbe();

	AI_Waypoint_t *pFirstRoute = NULL;
	AI_Waypoint_t *pHeadRoute = NULL;

	int nCurIndex = nStartIndex;
	int nNextIndex;

	// FIXME: Must be able to move to the first position (these needs some parameterization) 
	pMoveProbe->MoveLimit( navType, GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], MASK_NPCSOLID, pTarget, &endTrace );
	if ( IsMoveBlocked( endTrace ) )
	{
		// NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], 8.0f, 255, 0, 0, 0, true, 4.0f );
		return NULL;
	}

	// NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecPoints[nStartIndex], 8.0f, 0, 255, 0, 0, true, 4.0f );

	int nRunAwayCount = 0;
	while ( nRunAwayCount++ < nNumPoints )
	{
		// Advance our index in the specified direction
		nNextIndex = GetNextPoint( nCurIndex, nDirection, nNumPoints );

		// Try and build a local route between the current and next point
		pMoveProbe->MoveLimit( navType, vecPoints[nCurIndex], vecPoints[nNextIndex], MASK_NPCSOLID, pTarget, &endTrace );
		if ( IsMoveBlocked( endTrace ) )
		{
			// TODO: Triangulate here if we failed?

			// We failed, so give up
			if ( pHeadRoute )
			{
				DeleteAll( pHeadRoute );
			}

			// NDebugOverlay::HorzArrow( vecPoints[nCurIndex], vecPoints[nNextIndex], 8.0f, 255, 0, 0, 0, true, 4.0f );
			return NULL;
		}

		// NDebugOverlay::HorzArrow( vecPoints[nCurIndex], vecPoints[nNextIndex], 8.0f, 0, 255, 0, 0, true, 4.0f );

		if ( pHeadRoute == NULL )
		{
			// Start a new route head
			pFirstRoute = pHeadRoute = new AI_Waypoint_t( vecPoints[nCurIndex], 0.0f, navType, bits_WP_TO_DETOUR, NO_NODE );
		}
		else
		{
			// Link a new waypoint into the path
			AI_Waypoint_t *pNewNode = new AI_Waypoint_t( vecPoints[nCurIndex], 0.0f, navType, bits_WP_TO_DETOUR|bits_WP_DONT_SIMPLIFY, NO_NODE );
			pHeadRoute->SetNext( pNewNode );
			pHeadRoute = pNewNode;
		}

		// See if we're done
		if ( nNextIndex == nEndIndex )
		{
			AI_Waypoint_t *pNewNode = new AI_Waypoint_t( vecPoints[nEndIndex], 0.0f, navType, bits_WP_TO_DETOUR, NO_NODE );
			pHeadRoute->SetNext( pNewNode );
			pHeadRoute = pNewNode;
			break;
		}

		// Advance one node
		nCurIndex = nNextIndex;
	}

	return pFirstRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Find the closest point in a list of points, to a specified position
// Input  : &vecPosition - Position to test against
//			*vecPoints - List of vectors we'll check
//			nNumPoints - Number of points in the list
// Output : Index to the closest point in the list
//-----------------------------------------------------------------------------
inline int ClosestPointToPosition( const Vector &vecPosition, Vector *vecPoints, int nNumPoints )
{
	int   nBestNode = -1;
	float flBestDistSqr = FLT_MAX;
	float flDistSqr;
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistSqr = ( vecPoints[i] - vecPosition ).LengthSqr();
		if ( flDistSqr < flBestDistSqr )
		{
			flBestDistSqr = flDistSqr;
			nBestNode = i;
		}
	}

	return nBestNode;
}

//-----------------------------------------------------------------------------
// Purpose: Find which winding through a circular list is shortest in physical distance travelled
// Input  : &vecStart - Where we started from
//			nStartPoint - Starting index into the points
//			nEndPoint - Ending index into the points
//			nNumPoints - Number of points in the list
//			*vecPoints - List of vectors making up a list of points
//-----------------------------------------------------------------------------
inline int ShortestDirectionThroughPoints( const Vector &vecStart, int nStartPoint, int nEndPoint, Vector *vecPoints, int nNumPoints )
{
	const int nClockwise = 1;
	const int nCounterClockwise = -1;

	// Find the quickest direction around the object
	int nCurPoint = nStartPoint;
	int nNextPoint = GetNextPoint( nStartPoint, 1, nNumPoints );

	float flStartDistSqr = ( vecStart - vecPoints[nStartPoint] ).LengthSqr();
	float flDistanceSqr = flStartDistSqr;

	// Try going clockwise first
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistanceSqr += ( vecPoints[nCurPoint] - vecPoints[nNextPoint] ).LengthSqr();

		if ( nNextPoint == nEndPoint )
			break;

		nNextPoint = GetNextPoint( nNextPoint, 1, nNumPoints );
	}

	// Save this to test against
	float flBestDistanceSqr = flDistanceSqr;

	// Start from the beginning again
	flDistanceSqr = flStartDistSqr;

	nCurPoint = nStartPoint;
	nNextPoint = GetNextPoint( nStartPoint, -1, nNumPoints );

	// Now go the other way and see if it's shorter to do so
	for ( int i = 0; i < nNumPoints; i++ )
	{
		flDistanceSqr += ( vecPoints[nCurPoint] - vecPoints[nNextPoint] ).LengthSqr();

		// We've gone over our maximum so we can't be shorter
		if ( flDistanceSqr > flBestDistanceSqr )
			break;

		// We hit the end, we're shorter
		if ( nNextPoint == nEndPoint )
			return nCounterClockwise;

		nNextPoint = GetNextPoint( nNextPoint, -1, nNumPoints );
	}

	return nClockwise;
}

//-----------------------------------------------------------------------------
// Purpose: Attempt to build an avoidance route around an object using its OBB
//			Currently this function is meant for NPCs moving around a vehicle, 
//			and is very specialized as such
//
// Output : Returns a route if successful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildOBBAvoidanceRoute(	const Vector &vStart, const Vector &vEnd,
														const CBaseEntity *pObstruction, // obstruction to avoid
														const CBaseEntity *pTarget,		 // target to ignore
														Navigation_t navType )
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildOBBAvoidanceRoute );

	// If the point we're navigating to is within our OBB, then fail
	// TODO: We could potentially also just try to get as near as possible
	if ( pObstruction->CollisionProp()->IsPointInBounds( vEnd ) )
		return NULL;

	// Find out how much we'll need to inflate the collision bounds to let us move past
	Vector vecSize = pObstruction->CollisionProp()->OBBSize();
	float flWidth = GetOuter()->GetHullWidth() * 0.5f;

	float flWidthPercX = ( flWidth / vecSize.x );
	float flWidthPercY = ( flWidth / vecSize.y );

	// Find the points around the object, bloating it by our hull width
	// The ordering of these corners wind clockwise around the object, starting at the top left
	Vector vecPoints[4];
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector(  -flWidthPercX, 1+flWidthPercY, 0.25f ), &vecPoints[0] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector( 1+flWidthPercX, 1+flWidthPercY, 0.25f ), &vecPoints[1] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector( 1+flWidthPercX,  -flWidthPercY, 0.25f ), &vecPoints[2] );
	pObstruction->CollisionProp()->NormalizedToWorldSpace( Vector(  -flWidthPercX,  -flWidthPercY, 0.25f ), &vecPoints[3] );

	// Find the two points nearest our goals
	int nStartPoint = ClosestPointToPosition( vStart, vecPoints, ARRAYSIZE( vecPoints ) );
	int nEndPoint = ClosestPointToPosition( vEnd, vecPoints, ARRAYSIZE( vecPoints ) );

	// We won't be able to build a route if we're moving no distance between points
	if ( nStartPoint == nEndPoint )
		return NULL;

	// Find the shortest path around this wound polygon (direction is how to step through array)
	int nDirection = ShortestDirectionThroughPoints( vStart, nStartPoint, nEndPoint, vecPoints, ARRAYSIZE( vecPoints ) );

	// Attempt to build a route in our direction
	AI_Waypoint_t *pRoute = BuildRouteThroughPoints( vecPoints, ARRAYSIZE(vecPoints), nDirection, nStartPoint, nEndPoint, navType, (CBaseEntity *) pTarget );
	if ( pRoute == NULL )
	{
		// Failed that way, so try the opposite
		pRoute = BuildRouteThroughPoints( vecPoints, ARRAYSIZE(vecPoints), (-nDirection), nStartPoint, nEndPoint, navType, (CBaseEntity *) pTarget );
		if ( pRoute == NULL )
			return NULL;
	}

	return pRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a local route (not using nodes) between vStart
//			and vEnd, ignoring entity pTarget the the given tolerance
// Input  :
// Output : Returns a route if successful or NULL if no local route was possible
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildLocalRoute(const Vector &vStart, const Vector &vEnd, const CBaseEntity *pTarget, int endFlags, int nodeID, int buildFlags, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildLocalRoute );

	// Get waypoint yaw
	float flYaw;
	if (nodeID != NO_NODE)
	{
		flYaw = GetNetwork()->GetNode(nodeID)->GetYaw();
	}
	else
	{
		flYaw = 0;
	}

	// Try a ground route if requested
	if (buildFlags & bits_BUILD_GROUND)
	{
		AI_Waypoint_t *groundRoute = BuildGroundRoute(vStart,vEnd,pTarget,endFlags,nodeID,buildFlags,flYaw,goalTolerance);

		if (groundRoute)
		{
			return groundRoute;
		}
	}

	// Try a fly route if requested
	if ( buildFlags & bits_BUILD_FLY )
	{
		AI_Waypoint_t *flyRoute = BuildFlyRoute(vStart,vEnd,pTarget,endFlags,nodeID,buildFlags,flYaw,goalTolerance);

		if (flyRoute)
		{
			return flyRoute;
		}
	}

	// Try a jump route if NPC can jump and requested
	if ((buildFlags & bits_BUILD_JUMP) && (CapabilitiesGet() & bits_CAP_MOVE_JUMP))
	{
		AI_Waypoint_t *jumpRoute = BuildJumpRoute(vStart,vEnd,pTarget,endFlags,nodeID,buildFlags,flYaw);

		if (jumpRoute)
		{
			return jumpRoute;
		}
	}

	// Try a climb route if NPC can climb and requested
	if ((buildFlags & bits_BUILD_CLIMB)	&& (CapabilitiesGet() & bits_CAP_MOVE_CLIMB))
	{
		AI_Waypoint_t *climbRoute = BuildClimbRoute(vStart,vEnd,pTarget,endFlags,nodeID,buildFlags,flYaw);
		
		if (climbRoute)
		{
			return climbRoute;
		}
	}

	// Everything failed so return a NULL route
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Builds a route to the given vecGoal using either local movement
//			or nodes
//-----------------------------------------------------------------------------

ConVar ai_no_local_paths( "ai_no_local_paths", "0" );

AI_Waypoint_t *CAI_Pathfinder::BuildRoute( const Vector &vStart, const Vector &vEnd, CBaseEntity *pTarget, float goalTolerance, Navigation_t curNavType, bool bLocalSucceedOnWithinTolerance )
{
	int buildFlags = 0;
	bool bTryLocal = !ai_no_local_paths.GetBool();

	// Set up build flags
	if (curNavType == NAV_CLIMB)
	{
		 // if I'm climbing, then only allow routes that are also climb routes
		buildFlags = bits_BUILD_CLIMB;
		bTryLocal = false;
	}
	else if ( (CapabilitiesGet() & bits_CAP_MOVE_FLY) || (CapabilitiesGet() & bits_CAP_MOVE_SWIM) )
	{
		buildFlags = (bits_BUILD_FLY | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
	}
	else if (CapabilitiesGet() & bits_CAP_MOVE_GROUND)
	{
		buildFlags = (bits_BUILD_GROUND | bits_BUILD_GIVEWAY | bits_BUILD_TRIANG);
		if ( CapabilitiesGet() & bits_CAP_MOVE_JUMP )
		{
			buildFlags |= bits_BUILD_JUMP;
		}
	}

	// If our local moves can succeed if we get within the goaltolerance, set the flag
	if ( bLocalSucceedOnWithinTolerance )
	{
		buildFlags |= bits_BUILD_GET_CLOSE;
	}

	AI_Waypoint_t *pResult = NULL;

	//  First try a local route 
	if ( bTryLocal && CanUseLocalNavigation() )
	{
		pResult = BuildLocalRoute(vStart, vEnd, pTarget, 
								  bits_WP_TO_GOAL, NO_NODE, 
								  buildFlags, goalTolerance);
	}

	//  If the fails, try a node route
	if ( !pResult )
	{
		pResult = BuildNodeRoute( vStart, vEnd, buildFlags, goalTolerance );
	}

	m_bIgnoreStaleLinks = false;

	return pResult;
}

void CAI_Pathfinder::UnlockRouteNodes( AI_Waypoint_t *pPath )
{
	CAI_Node *pNode;
	while ( pPath )
	{
		if ( pPath->iNodeID != NO_NODE && ( pNode = GetNetwork()->GetNode(pPath->iNodeID) ) != NULL && pNode->IsLocked() )
			pNode->Unlock();
		pPath = pPath->GetNext();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Attempts to build a radial route around the given center position
//			over a given arc size
//
// Input  : vStartPos	- where route should start from
//			vCenterPos	- the center of the arc
//			vGoalPos	- ultimate goal position
//			flRadius	- radius of the arc
//			flArc		- how long should the path be (in degrees)
//			bClockwise	- the direction we are heading
// Output : The route
//-----------------------------------------------------------------------------
AI_Waypoint_t *CAI_Pathfinder::BuildRadialRoute( const Vector &vStartPos, const Vector &vCenterPos, const Vector &vGoalPos, float flRadius, float flArc, float flStepDist, bool bClockwise, float goalTolerance, bool bAirRoute /*= false*/ )
{
	MARK_TASK_EXPENSIVE();

	// ------------------------------------------------------------------------------
	// Make sure we have a minimum distance between nodes.  For the given 
	// radius, calculate the angular step necessary for this distance.
	// IMPORTANT: flStepDist must be large enough that given the 
	//			  NPC's movment speed that it can come to a stop
	// ------------------------------------------------------------------------------
	float flAngleStep = 2.0f * atan((0.5f*flStepDist)/flRadius);	

	// Flip direction if clockwise
	if ( bClockwise )
	{
		flArc		*= -1;
		flAngleStep *= -1;
	}

	// Calculate the start angle on the arc in world coordinates
	Vector vStartDir = ( vStartPos - vCenterPos );
	VectorNormalize( vStartDir );

	// Get our control angles
	float flStartAngle	= DEG2RAD(UTIL_VecToYaw(vStartDir));
	float flEndAngle	= flStartAngle + DEG2RAD(flArc);

	//  Offset set our first node by one arc step so NPC doesn't run perpendicular to the arc when starting a different radius
	flStartAngle += flAngleStep;

	AI_Waypoint_t*	pHeadRoute	= NULL;	// Pointer to the beginning of the route chains
	AI_Waypoint_t*	pNextRoute	= NULL; // Next leg of the route
	AI_Waypoint_t*  pLastRoute	= NULL; // The last route chain added to the head
	Vector			vLastPos	= vStartPos; // Last position along the arc in worldspace
	int				fRouteBits = ( bAirRoute ) ? bits_BUILD_FLY : bits_BUILD_GROUND; // Whether this is an air route or not
	float			flCurAngle = flStartAngle; // Starting angle
	Vector			vNextPos;
	
	// Make sure that we've got somewhere to go.  This generally means your trying to walk too small an arc.
	Assert( ( bClockwise && flCurAngle > flEndAngle ) || ( !bClockwise && flCurAngle < flEndAngle ) );

	// Start iterating through our arc
	while( 1 )
	{
		// See if we've ended our run
		if ( ( bClockwise && flCurAngle <= flEndAngle ) || ( !bClockwise && flCurAngle >= flEndAngle ) )
			break;
		
		// Get our next position along the arc
		vNextPos	= vCenterPos;
		vNextPos.x	+= flRadius * cos( flCurAngle );
		vNextPos.y	+= flRadius * sin( flCurAngle );

		// Build a route from the last position to the current one
		pNextRoute = BuildLocalRoute( vLastPos, vNextPos, NULL, NULL, NO_NODE, fRouteBits, goalTolerance);
		
		// If we can't find a route, we failed
		if ( pNextRoute == NULL )
			return NULL;

		// Don't simplify the route (otherwise we'll cut corners where we don't want to!
		pNextRoute->ModifyFlags( bits_WP_DONT_SIMPLIFY, true );
			
		if ( pHeadRoute )
		{
			// Tack the routes together
			AddWaypointLists( pHeadRoute, pNextRoute );
		}
		else
		{
			// Otherwise we're now the previous route
			pHeadRoute = pNextRoute;
		}
		
		// Move our position
		vLastPos  = vNextPos;
		pLastRoute = pNextRoute;

		// Move our current angle
		flCurAngle += flAngleStep;
	}

	// NOTE: We could also simply build a local route with no curve, but it's unlikely that's what was intended by the caller
	if ( pHeadRoute == NULL )
		return NULL;

	// Append a path to the final position
	pLastRoute = BuildLocalRoute( vLastPos, vGoalPos, NULL, NULL, NO_NODE, bAirRoute ? bits_BUILD_FLY : bits_BUILD_GROUND, goalTolerance );	
	if ( pLastRoute == NULL )
		return NULL;

	// Allow us to simplify the last leg of the route
	pLastRoute->ModifyFlags( bits_WP_DONT_SIMPLIFY, false );
	pLastRoute->ModifyFlags( bits_WP_TO_GOAL, true );

	// Add them together
	AddWaypointLists( pHeadRoute, pLastRoute );
	
	// Give back the complete route
	return pHeadRoute;
}


//-----------------------------------------------------------------------------
// Checks a stale navtype route 
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CheckStaleNavTypeRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd )
{
	AIMoveTrace_t moveTrace;
	GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID, NULL, 100, AIMLF_IGNORE_TRANSIENTS, &moveTrace);

	// Is the direct route clear?
	if (!IsMoveBlocked(moveTrace))
	{
		return true;
	}

	// Next try to triangulate
	// FIXME: Since blocked dist is an unreliable number, this computation is bogus
	Vector vecDelta;
	VectorSubtract( vEnd, vStart, vecDelta );
	float flTotalDist = vecDelta.Length();

	Vector vApex;
	if (Triangulate( navType, vStart, vEnd, flTotalDist - moveTrace.flDistObstructed, NULL, &vApex ))
	{
		return true;
	}

	// Try a giveway request, if I can get there ignoring NPCs
	if ( moveTrace.pObstruction && moveTrace.pObstruction->MyNPCPointer() )
	{
		GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID_BRUSHONLY, NULL, &moveTrace);

		if (!IsMoveBlocked(moveTrace))
		{
			return true;
		}
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: Checks if a local route (not using nodes) between vStart
//			and vEnd exists using the moveType
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CheckStaleRoute(const Vector &vStart, const Vector &vEnd, int moveTypes)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_CheckStaleRoute );

	// -------------------------------------------------------------------
	// First try to go there directly
	// -------------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_GROUND) 
	{
		if (CheckStaleNavTypeRoute( NAV_GROUND, vStart, vEnd ))
			return true;
	}

	// -------------------------------------------------------------------
	// First try to go there directly
	// -------------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_FLY) 
	{
		if (CheckStaleNavTypeRoute( NAV_FLY, vStart, vEnd ))
			return true;
	}

	// --------------------------------------------------------------
	//  Try to jump if we can jump to a node
	// --------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_JUMP)
	{
		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit( NAV_JUMP, vStart, vEnd, MASK_NPCSOLID, NULL, &moveTrace);
		if (!IsMoveBlocked(moveTrace))
		{
			return true;
		}
		else
		{
			// Can't tell jump up from jump down at this point
			GetOuter()->GetMoveProbe()->MoveLimit( NAV_JUMP, vEnd, vStart, MASK_NPCSOLID, NULL, &moveTrace);
			if (!IsMoveBlocked(moveTrace))
				return true;
		}
	}

	// --------------------------------------------------------------
	//  Try to climb if we can climb to a node
	// --------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_CLIMB)
	{
		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit( NAV_CLIMB, vStart, vEnd, MASK_NPCSOLID, NULL, &moveTrace);
		if (!IsMoveBlocked(moveTrace))
		{	
			return true;
		}
	}

	// Man do we suck! Couldn't get there by any route
	return false;
}

//-----------------------------------------------------------------------------

#define MAX_NODE_TRIES 4
#define MAX_TRIANGULATIONS 2

class CPathfindNearestNodeFilter : public INearestNodeFilter
{
public:
	CPathfindNearestNodeFilter( CAI_Pathfinder *pPathfinder, const Vector &vGoal, bool bToNode, int buildFlags, float goalTolerance )
	 :	m_pPathfinder( pPathfinder ),
		m_nTries(0),
		m_vGoal( vGoal ),
		m_bToNode( bToNode ),
		m_goalTolerance( goalTolerance ),
		m_moveTypes( buildFlags & ( bits_BUILD_GROUND | bits_BUILD_FLY | bits_BUILD_JUMP | bits_BUILD_CLIMB ) ),
		m_pRoute( NULL )
	{
		// Cast to int in order to indicate that we are intentionally comparing different
		// enum types, to suppress gcc warnings.
		COMPILE_TIME_ASSERT( bits_BUILD_GROUND == (int)bits_CAP_MOVE_GROUND && bits_BUILD_FLY == (int)bits_CAP_MOVE_FLY && bits_BUILD_JUMP == (int)bits_CAP_MOVE_JUMP && bits_BUILD_CLIMB == (int)bits_CAP_MOVE_CLIMB );
	}

	bool IsValid( CAI_Node *pNode )
	{
		int nStaleLinks = 0;
		if ( !m_pPathfinder->m_bIgnoreStaleLinks )
		{
			int hull = m_pPathfinder->GetOuter()->GetHullType();
			for ( int i = 0; i < pNode->NumLinks(); i++ )
			{
				CAI_Link *pLink = pNode->GetLinkByIndex( i );
				if ( pLink->m_LinkInfo & ( bits_LINK_STALE_SUGGESTED | bits_LINK_OFF ) )
				{
					nStaleLinks++;
				}
				else if ( ( pLink->m_iAcceptedMoveTypes[hull] & m_moveTypes ) == 0 )
				{
					nStaleLinks++;
				}
			}
		}

		if ( nStaleLinks && nStaleLinks == pNode->NumLinks() )
			return false;

		int buildFlags = ( m_nTries < MAX_TRIANGULATIONS ) ? ( bits_BUILD_IGNORE_NPCS | bits_BUILD_TRIANG ) : bits_BUILD_IGNORE_NPCS;

		if ( m_bToNode )
			m_pRoute = m_pPathfinder->RouteToNode( m_vGoal, buildFlags, pNode->GetId(), m_goalTolerance );
		else
			m_pRoute = m_pPathfinder->RouteFromNode( m_vGoal, buildFlags, pNode->GetId(), m_goalTolerance );

		m_nTries++;

		return ( m_pRoute != NULL );
	}

	bool ShouldContinue()
	{
		return ( !m_pRoute && m_nTries < MAX_NODE_TRIES );
	}

	CAI_Pathfinder *m_pPathfinder;
	int				m_nTries;
	Vector			m_vGoal;
	bool			m_bToNode;
	float			m_goalTolerance;
	int				m_moveTypes;

	AI_Waypoint_t *	m_pRoute;
};


AI_Waypoint_t *CAI_Pathfinder::BuildNearestNodeRoute( const Vector &vGoal, bool bToNode, int buildFlags, float goalTolerance, int *pNearestNode )
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildNearestNodeRoute );

	CPathfindNearestNodeFilter filter( this, vGoal, bToNode, buildFlags, goalTolerance );
	*pNearestNode  = GetNetwork()->NearestNodeToPoint( GetOuter(), vGoal, true, &filter );

	return filter.m_pRoute;
}

//-----------------------------------------------------------------------------
// Purpose: Attemps to build a node route between vStart and vEnd
// Input  :
// Output : Returns a route if sucessful or NULL if no node route was possible
//-----------------------------------------------------------------------------

AI_Waypoint_t *CAI_Pathfinder::BuildNodeRoute(const Vector &vStart, const Vector &vEnd, int buildFlags, float goalTolerance)
{
	AI_PROFILE_SCOPE( CAI_Pathfinder_BuildNodeRoute );

	// ----------------------------------------------------------------------
	//  Make sure network has nodes
	// ----------------------------------------------------------------------
	if (GetNetwork()->NumNodes() == 0)
		return NULL;

	// ----------------------------------------------------------------------
	//	Find the nearest source node
	// ----------------------------------------------------------------------
	int srcID;
	AI_Waypoint_t *srcRoute = BuildNearestNodeRoute( vStart, true, buildFlags, goalTolerance, &srcID );
	if ( !srcRoute )
	{
		DbgNavMsg1( GetOuter(), "Node pathfind failed, no route to source %d\n", srcID );
		return NULL;
	}

	// ----------------------------------------------------------------------
	//	Find the nearest destination node
	// ----------------------------------------------------------------------
	int destID;
	AI_Waypoint_t *destRoute = BuildNearestNodeRoute( vEnd, false, buildFlags, goalTolerance, &destID );
	if ( !destRoute )
	{
		DeleteAll( srcRoute );
		DbgNavMsg1( GetOuter(), "Node pathfind failed, no route to dest %d\n", destID );
		return NULL;
	}

	// ----------------------------------------------------------------------
	// If source and destination are the same, we can bypass finding a route
	// ----------------------------------------------------------------------
	if (destID == srcID)
	{
		AddWaypointLists(srcRoute,destRoute);
		DbgNavMsg(  GetOuter(), "Node pathfind succeeded: dest == source\n");
		return srcRoute;
	}

	// If nodes are not connected by network graph, no route is possible
	if (!GetNetwork()->IsConnected(srcID, destID))
		return NULL;

	AI_Waypoint_t *path = FindBestPath(srcID, destID);

	if (!path)
	{
		DeleteAll(srcRoute);
		DeleteAll(destRoute);
		DbgNavMsg2( GetOuter(), "Node pathfind failed, no route between %d and %d\n", srcID, destID );
		return NULL;
	}

	// Now put all the pieces together to form our route
	AddWaypointLists(srcRoute,path);
	AddWaypointLists(srcRoute,destRoute);

	DbgNavMsg(  GetOuter(), "Node pathfind succeeded\n");
	return srcRoute;
}

//-----------------------------------------------------------------------------
// Test the triangulation route...
//-----------------------------------------------------------------------------
#ifdef _WIN32
#pragma warning (disable:4701)
#endif

bool CAI_Pathfinder::TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
	const Vector &vecApex, const Vector &vecEnd, const CBaseEntity *pTargetEnt, AIMoveTrace_t *pStartTrace )
{
	AIMoveTrace_t endTrace;
	endTrace.fStatus = AIMR_OK;	// just to make the debug overlay code easy

	// Check the triangulation
	CAI_MoveProbe *pMoveProbe = GetOuter()->GetMoveProbe();

	bool bPathClear = false;

	// See if we can get from the start point to the triangle apex
	if ( pMoveProbe->MoveLimit(navType, vecStart, vecApex, MASK_NPCSOLID, pTargetEnt, pStartTrace ) )
	{
		// Ok, we got from the start to the triangle apex, now try
		// the triangle apex to the end
		if ( pMoveProbe->MoveLimit(navType, vecApex, vecEnd, MASK_NPCSOLID, pTargetEnt, &endTrace ) )
		{
			bPathClear = true;
		}
	}

	// Debug mode: display the tested path...
	if (GetOuter()->m_debugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
		m_TriDebugOverlay.AddTriOverlayLines( vecStart, vecApex, vecEnd, *pStartTrace, endTrace, bPathClear);

	return bPathClear;
}

#ifdef _WIN32
#pragma warning (default:4701)
#endif


//-----------------------------------------------------------------------------
// Purpose: tries to overcome local obstacles by triangulating a path around them.
// Input  : flDist is is how far the obstruction that we are trying
//			to triangulate around is from the npc
// Output :
//-----------------------------------------------------------------------------

// FIXME: this has no concept that the blocker may not be exactly along the vecStart, vecEnd vector.
// FIXME: it should take a position (and size?) to avoid
// FIXME: this does the position checks in the same order as GiveWay() so they tend to fight each other when both are active
#define MAX_TRIAGULATION_DIST (32*12)
bool CAI_Pathfinder::Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEndIn, 
	float flDistToBlocker, const CBaseEntity *pTargetEnt, Vector *pApex )
{
	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	Assert( pApex );

	AI_PROFILE_SCOPE( CAI_Pathfinder_Triangulate );

	Vector vecForward, vecUp, vecPerpendicular;
	VectorSubtract( vecEndIn, vecStart, vecForward );
	float flTotalDist = VectorNormalize( vecForward );

	Vector vecEnd;

	// If we're walking, then don't try to triangulate over large distances
	if ( navType != NAV_FLY && flTotalDist > MAX_TRIAGULATION_DIST)
	{
		vecEnd = vecForward * MAX_TRIAGULATION_DIST;
		flTotalDist = MAX_TRIAGULATION_DIST;
		if ( !GetOuter()->GetMoveProbe()->MoveLimit(navType, vecEnd, vecEndIn, MASK_NPCSOLID, pTargetEnt) )
		{
			return false;
		}

	}
	else
		vecEnd = vecEndIn;

	// Compute a direction vector perpendicular to the desired motion direction
	if ( 1.0f - fabs(vecForward.z) > 1e-3 )
	{
		vecUp.Init( 0, 0, 1 );
		CrossProduct( vecForward, vecUp, vecPerpendicular );	// Orthogonal to facing
	}
	else
	{
		vecUp.Init( 0, 1, 0 );
		vecPerpendicular.Init( 1, 0, 0 ); 
	}

	// Grab the size of the navigation bounding box
	float sizeX = 0.5f * NAI_Hull::Length(GetHullType());
	float sizeZ = 0.5f * NAI_Hull::Height(GetHullType());

	// start checking right about where the object is, picking two equidistant
	// starting points, one on the left, one on the right. As we progress 
	// through the loop, we'll push these away from the obstacle, hoping to 
	// find a way around on either side. m_vecSize.x is added to the ApexDist 
	// in order to help select an apex point that insures that the NPC is 
	// sufficiently past the obstacle before trying to turn back onto its original course.

	if (GetOuter()->m_debugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
	{
		m_TriDebugOverlay.FadeTriOverlayLines();
	}

	float flApexDist = flDistToBlocker + sizeX;
	if (flApexDist > flTotalDist) 
	{
		flApexDist = flTotalDist;
	}

	// Compute triangulation apex points (NAV_FLY attempts vertical triangulation too)
	Vector vecDelta[2];
	Vector vecApex[4];
	float pApexDist[4];

	Vector vecCenter;
	int nNumToTest = 2;
	VectorMultiply( vecPerpendicular, sizeX, vecDelta[0] );

	VectorMA( vecStart, flApexDist, vecForward, vecCenter );
	VectorSubtract( vecCenter, vecDelta[0], vecApex[0] );
	VectorAdd( vecCenter, vecDelta[0], vecApex[1] );
 	vecDelta[0] *= 2.0f;
	pApexDist[0] = pApexDist[1] = flApexDist;

	if (navType == NAV_FLY)
	{
		VectorMultiply( vecUp, 3.0f * sizeZ, vecDelta[1] );
		VectorSubtract( vecCenter, vecDelta[1], vecApex[2] );
		VectorAdd( vecCenter, vecDelta[1], vecApex[3] );
		pApexDist[2] = pApexDist[3] = flApexDist;
		nNumToTest = 4;
	}

	AIMoveTrace_t moveTrace;
	for (int i = 0; i < 2; ++i )
	{
		// NOTE: Do reverse order so fliers try to move over the top first 
		for (int j = nNumToTest; --j >= 0; )
		{
			if (TestTriangulationRoute(navType, vecStart, vecApex[j], vecEnd, pTargetEnt, &moveTrace))
			{
				*pApex  = vecApex[j];
				return true;
			}

			// Here, the starting half of the triangle was blocked. Lets
			// pull back the apex toward the start...
			if (IsMoveBlocked(moveTrace))
			{
				Vector vecStartToObstruction;
				VectorSubtract( moveTrace.vEndPosition, vecStart, vecStartToObstruction );
				float flDistToObstruction = DotProduct( vecStartToObstruction, vecForward );

				float flNewApexDist = pApexDist[j];
				if (pApexDist[j] > flDistToObstruction)
					flNewApexDist = flDistToObstruction;

				VectorMA( vecApex[j], flNewApexDist - pApexDist[j], vecForward, vecApex[j] );
				pApexDist[j] = flNewApexDist;
			}

			// NOTE: This has to occur *after* the code above because
			// the above code uses vecApex for some distance computations
			if (j & 0x1)
				vecApex[j] += vecDelta[j >> 1];
			else
				vecApex[j] -= vecDelta[j >> 1];
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Triangulation debugging 
//-----------------------------------------------------------------------------

void CAI_Pathfinder::DrawDebugGeometryOverlays(int npcDebugOverlays) 
{
	m_TriDebugOverlay.Draw(npcDebugOverlays);
}

void CAI_Pathfinder::CTriDebugOverlay::Draw(int npcDebugOverlays) 
{
	if (m_debugTriOverlayLine) 
	{
		if ( npcDebugOverlays & OVERLAY_NPC_TRIANGULATE_BIT) 
		{
			for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
			{
				if (m_debugTriOverlayLine[i]->draw)
				{
					NDebugOverlay::Line(m_debugTriOverlayLine[i]->origin,
										 m_debugTriOverlayLine[i]->dest,
										 m_debugTriOverlayLine[i]->r,
										 m_debugTriOverlayLine[i]->g,
										 m_debugTriOverlayLine[i]->b,
										 m_debugTriOverlayLine[i]->noDepthTest,
										 0);
				}
			}
		}
		else
		{
			ClearTriOverlayLines();
		}
	}
}

void CAI_Pathfinder::CTriDebugOverlay::AddTriOverlayLines( const Vector &vecStart, const Vector &vecApex, const Vector &vecEnd, const AIMoveTrace_t &startTrace, const AIMoveTrace_t &endTrace, bool bPathClear )
{
	static unsigned char s_TriangulationColor[2][3] = 
	{
		{ 255,   0, 0 },
		{   0, 255, 0 }
	};

	unsigned char *c = s_TriangulationColor[bPathClear];

	AddTriOverlayLine(vecStart, vecApex, c[0],c[1],c[2], false);
	AddTriOverlayLine(vecApex, vecEnd, c[0],c[1],c[2], false);

	// If we've blocked, draw an X where we were blocked...
	if (IsMoveBlocked(startTrace.fStatus))
	{
		Vector pt1, pt2;
		pt1 = pt2 = startTrace.vEndPosition;

		pt1.x -= 10; pt1.y -= 10;
		pt2.x += 10; pt2.y += 10;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);

		pt1.x += 20;
		pt2.x -= 20;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);
	}

	if (IsMoveBlocked(endTrace.fStatus))
	{
		Vector pt1, pt2;
		pt1 = pt2 = endTrace.vEndPosition;

		pt1.x -= 10; pt1.y -= 10;
		pt2.x += 10; pt2.y += 10;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);

		pt1.x += 20;
		pt2.x -= 20;
		AddTriOverlayLine(pt1, pt2, c[0],c[1],c[2], false);
	}
}

void CAI_Pathfinder::CTriDebugOverlay::ClearTriOverlayLines(void) 
{
	if (m_debugTriOverlayLine)
	{
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i]->draw = false;
		}
	}
}
void CAI_Pathfinder::CTriDebugOverlay::FadeTriOverlayLines(void) 
{
	if (m_debugTriOverlayLine)
	{
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i]->r *= 0.5;
			m_debugTriOverlayLine[i]->g *= 0.5;				
			m_debugTriOverlayLine[i]->b *= 0.5;				
		}
	}
}
void CAI_Pathfinder::CTriDebugOverlay::AddTriOverlayLine(const Vector &origin, const Vector &dest, int r, int g, int b, bool noDepthTest)
{
	if (!m_debugTriOverlayLine)
	{
		m_debugTriOverlayLine = new OverlayLine_t*[NUM_NPC_DEBUG_OVERLAYS];
		for (int i=0;i<NUM_NPC_DEBUG_OVERLAYS;i++)
		{
			m_debugTriOverlayLine[i] = new OverlayLine_t;
		}
	}
	static int overCounter = 0;

	if (overCounter >= NUM_NPC_DEBUG_OVERLAYS)
	{
		overCounter = 0;
	}
	
	m_debugTriOverlayLine[overCounter]->origin			= origin;
	m_debugTriOverlayLine[overCounter]->dest			= dest;
	m_debugTriOverlayLine[overCounter]->r				= r;
	m_debugTriOverlayLine[overCounter]->g				= g;
	m_debugTriOverlayLine[overCounter]->b				= b;
	m_debugTriOverlayLine[overCounter]->noDepthTest		= noDepthTest;
	m_debugTriOverlayLine[overCounter]->draw			= true;
	overCounter++;

}

//-----------------------------------------------------------------------------
