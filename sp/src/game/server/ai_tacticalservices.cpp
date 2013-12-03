//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================

#include "cbase.h"

#include "bitstring.h"

#include "ai_tacticalservices.h"
#include "ai_basenpc.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_link.h"
#include "ai_moveprobe.h"
#include "ai_pathfinder.h"
#include "ai_navigator.h"
#include "ai_networkmanager.h"
#include "ai_hint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_find_lateral_cover( "ai_find_lateral_cover", "1" );
ConVar ai_find_lateral_los( "ai_find_lateral_los", "1" );

#ifdef _DEBUG
ConVar ai_debug_cover( "ai_debug_cover", "0" );
int g_AIDebugFindCoverNode = -1;
#define DebugFindCover( node, from, to, r, g, b ) \
	if ( !ai_debug_cover.GetBool() || \
		 (g_AIDebugFindCoverNode != -1 && g_AIDebugFindCoverNode != node) || \
		 !GetOuter()->m_bSelected ) \
		; \
	else \
		NDebugOverlay::Line( from, to, r, g, b, false, 1 )

#define DebugFindCover2( node, from, to, r, g, b ) \
	if ( ai_debug_cover.GetInt() < 2 || \
		 (g_AIDebugFindCoverNode != -1 && g_AIDebugFindCoverNode != node) || \
		 !GetOuter()->m_bSelected ) \
		; \
	else \
		NDebugOverlay::Line( from, to, r, g, b, false, 1 )

ConVar ai_debug_tactical_los( "ai_debug_tactical_los", "0" );
int g_AIDebugFindLosNode = -1;
#define ShouldDebugLos( node ) ( ai_debug_tactical_los.GetBool() && ( g_AIDebugFindLosNode == -1 || g_AIDebugFindLosNode == ( node ) ) && GetOuter()->m_bSelected )
#else
#define DebugFindCover( node, from, to, r, g, b ) ((void)0)
#define DebugFindCover2( node, from, to, r, g, b ) ((void)0)
#define ShouldDebugLos( node ) false
#endif

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC(CAI_TacticalServices)
	//						m_pNetwork	(not saved)
	//						m_pPathfinder	(not saved)
	DEFINE_FIELD( m_bAllowFindLateralLos, FIELD_BOOLEAN ),

END_DATADESC();

//-------------------------------------

void CAI_TacticalServices::Init( CAI_Network *pNetwork )
{
	Assert( pNetwork );
	m_pNetwork = pNetwork;
	m_pPathfinder = GetOuter()->GetPathfinder();
	Assert( m_pPathfinder );
}
	
//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam, Vector *pResult)
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLos );

	MARK_TASK_EXPENSIVE();

	int node = FindLosNode( threatPos, threatEyePos, 
											 minThreatDist, maxThreatDist, 
											 blockTime, eFlankType, vecFlankRefPos, flFlankParam );
	
	if (node == NO_NODE)
		return false;

	*pResult = GetNodePos( node );
	return true;
}

//-------------------------------------

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, Vector *pResult)
{
	return FindLos( threatPos, threatEyePos, minThreatDist, maxThreatDist, blockTime, FLANKTYPE_NONE, vec3_origin, 0, pResult );
}

//-------------------------------------

bool CAI_TacticalServices::FindBackAwayPos( const Vector &vecThreat, Vector *pResult )
{
	MARK_TASK_EXPENSIVE();

	Vector vMoveAway = GetAbsOrigin() - vecThreat;
	vMoveAway.NormalizeInPlace();

	if ( GetOuter()->GetNavigator()->FindVectorGoal( pResult, vMoveAway, 10*12, 10*12, true ) )
		return true;

	int node = FindBackAwayNode( vecThreat );
	
	if (node != NO_NODE)
	{
		*pResult = GetNodePos( node );
		return true;
	}

	if ( GetOuter()->GetNavigator()->FindVectorGoal( pResult, vMoveAway, GetHullWidth() * 4, GetHullWidth() * 2, true ) )
		return true;

	return false;
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	return FindCoverPos( GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist, pResult );
}

//-------------------------------------

bool CAI_TacticalServices::FindCoverPos( const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindCoverPos );

	MARK_TASK_EXPENSIVE();

	int node = FindCoverNode( vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist );
	
	if (node == NO_NODE)
		return false;

	*pResult = GetNodePos( node );
	return true;
}

//-------------------------------------
// Checks lateral cover
//-------------------------------------
bool CAI_TacticalServices::TestLateralCover( const Vector &vecCheckStart, const Vector &vecCheckEnd, float flMinDist )
{
	trace_t	tr;

	if ( (vecCheckStart - vecCheckEnd).LengthSqr() > Square(flMinDist) )
	{
		if (GetOuter()->IsCoverPosition(vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset()))
		{
			if ( GetOuter()->IsValidCover ( vecCheckEnd, NULL ) )
			{
				AIMoveTrace_t moveTrace;
				GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace );
				if (moveTrace.fStatus == AIMR_OK)
				{
					DebugFindCover( NO_NODE, vecCheckEnd + GetOuter()->GetViewOffset(), vecCheckStart, 0, 255, 0 );
					return true;
				}
			}
		}
	}

	DebugFindCover( NO_NODE, vecCheckEnd + GetOuter()->GetViewOffset(), vecCheckStart, 255, 0, 0 );

	return false;
}


//-------------------------------------
// FindLateralCover - attempts to locate a spot in the world
// directly to the left or right of the caller that will
// conceal them from view of pSightEnt
//-------------------------------------

#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks
bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, Vector *pResult )
{
	return FindLateralCover( vecThreat, flMinDist, COVER_CHECKS * COVER_DELTA, COVER_CHECKS, pResult );
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	return FindLateralCover( GetAbsOrigin(), vecThreat, flMinDist, distToCheck, numChecksPerDir, pResult );
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vNearPos, const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLateralCover );

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	int		i;

	if ( TestLateralCover( vecThreat, vNearPos, flMinDist ) )
	{
		*pResult = GetLocalOrigin();
		return true;
	}

	if( !ai_find_lateral_cover.GetBool() )
	{
		// Force the NPC to use the nodegraph to find cover. NOTE: We let the above code run
		// to detect the case where the NPC may already be standing in cover, but we don't 
		// make any additional lateral checks.
		return false;
	}

	Vector right =  vecThreat - vNearPos;
	float temp;

	right.z = 0;
	VectorNormalize( right );
	temp = right.x;
	right.x = -right.y;
	right.y = temp;

	vecStepRight = right * (distToCheck / (float)numChecksPerDir);
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = vNearPos;
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < numChecksPerDir ; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralCover( vecCheckStart, vecLeftTest, flMinDist ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralCover( vecCheckStart, vecRightTest, flMinDist ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}

//-------------------------------------
// Purpose: Find a nearby node that further away from the enemy than the
//			min range of my current weapon if there is one or just futher
//			away than my current location if I don't have a weapon.  
//			Used to back away for attacks
//-------------------------------------

int CAI_TacticalServices::FindBackAwayNode(const Vector &vecThreat )
{
	if ( !CAI_NetworkManager::NetworksLoaded() )
	{
		DevWarning( 2, "Graph not ready for FindBackAwayNode!\n" );
		return NO_NODE;
	}

	int iMyNode			= GetPathfinder()->NearestNodeToNPC();
	int iThreatNode		= GetPathfinder()->NearestNodeToPoint( vecThreat );

	if ( iMyNode == NO_NODE )
	{
		DevWarning( 2, "FindBackAwayNode() - %s has no nearest node!\n", GetEntClassname());
		return NO_NODE;
	}
	if ( iThreatNode == NO_NODE )
	{
		// DevWarning( 2, "FindBackAwayNode() - Threat has no nearest node!\n" );
		iThreatNode = iMyNode;
		// return false;
	}

	// A vector pointing to the threat.
	Vector vecToThreat;
	vecToThreat = vecThreat - GetLocalOrigin();

	// Get my current distance from the threat
	float flCurDist = VectorNormalize( vecToThreat );

	// Check my neighbors to find a node that's further away
	for (int link = 0; link < GetNetwork()->GetNode(iMyNode)->NumLinks(); link++) 
	{
		CAI_Link *nodeLink = GetNetwork()->GetNode(iMyNode)->GetLinkByIndex(link);

		if ( !m_pPathfinder->IsLinkUsable( nodeLink, iMyNode ) )
			continue;

		int destID = nodeLink->DestNodeID(iMyNode);

		float flTestDist = ( vecThreat - GetNetwork()->GetNode(destID)->GetPosition(GetHullType()) ).Length();

		if ( flTestDist > flCurDist )
		{
			// Make sure this node doesn't take me past the enemy's position.
			Vector vecToNode;
			vecToNode = GetNetwork()->GetNode(destID)->GetPosition(GetHullType()) - GetLocalOrigin();
			VectorNormalize( vecToNode );
		
			if( DotProduct( vecToNode, vecToThreat ) < 0.0 )
			{
				return destID;
			}
		}
	}
	return NO_NODE;
}

//-------------------------------------
// FindCover - tries to find a nearby node that will hide
// the caller from its enemy. 
//
// If supplied, search will return a node at least as far
// away as MinDist, but no farther than MaxDist. 
// if MaxDist isn't supplied, it defaults to a reasonable 
// value
//-------------------------------------

int CAI_TacticalServices::FindCoverNode(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	return FindCoverNode(GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist );
}

//-------------------------------------

int CAI_TacticalServices::FindCoverNode(const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	if ( !CAI_NetworkManager::NetworksLoaded() )
		return NO_NODE;

	AI_PROFILE_SCOPE( CAI_TacticalServices_FindCoverNode );

	MARK_TASK_EXPENSIVE();

	DebugFindCover( NO_NODE, GetOuter()->EyePosition(), vThreatEyePos, 0, 255, 255 );

	int iMyNode = GetPathfinder()->NearestNodeToPoint( vNearPos );

	if ( iMyNode == NO_NODE )
	{
		Vector pos = GetOuter()->GetAbsOrigin();
		DevWarning( 2, "FindCover() - %s has no nearest node! (Check near %f %f %f)\n", GetEntClassname(), pos.x, pos.y, pos.z);
		return NO_NODE;
	}

	if ( !flMaxDist )
	{
		// user didn't supply a MaxDist, so work up a crazy one.
		flMaxDist = 784;
	}

	if ( flMinDist > 0.5 * flMaxDist)
	{
		flMinDist = 0.5 * flMaxDist;
	}

	// ------------------------------------------------------------------------------------
	// We're going to search for a cover node by expanding to our current node's neighbors
	// and then their neighbors, until cover is found, or all nodes are beyond MaxDist
	// ------------------------------------------------------------------------------------
	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * GetNetwork()->NumNodes() );
	CNodeList list( pBuffer, GetNetwork()->NumNodes() );
	CVarBitVec wasVisited(GetNetwork()->NumNodes());	// Nodes visited

	// mark start as visited
	list.Insert( AI_NearNode_t(iMyNode, 0) ); 
	wasVisited.Set( iMyNode );
	float flMinDistSqr = flMinDist*flMinDist;
	float flMaxDistSqr = flMaxDist*flMaxDist;

	static int nSearchRandomizer = 0;		// tries to ensure the links are searched in a different order each time;

	// Search until the list is empty
	while( list.Count() )
	{
		// Get the node that is closest in the number of steps and remove from the list
		int nodeIndex = list.ElementAtHead().nodeIndex;
		list.RemoveAtHead();

		CAI_Node *pNode = GetNetwork()->GetNode(nodeIndex);
		Vector nodeOrigin = pNode->GetPosition(GetHullType());

		float dist = (vNearPos - nodeOrigin).LengthSqr();
		if (dist >= flMinDistSqr && dist < flMaxDistSqr)
		{
			Activity nCoverActivity = GetOuter()->GetCoverActivity( pNode->GetHint() );
			Vector vEyePos = nodeOrigin + GetOuter()->EyeOffset(nCoverActivity);

			if ( GetOuter()->IsValidCover( nodeOrigin, pNode->GetHint() ) )
			{
				// Check if this location will block the threat's line of sight to me
				if (GetOuter()->IsCoverPosition(vThreatEyePos, vEyePos))
				{
					// --------------------------------------------------------
					// Don't let anyone else use this node for a while
					// --------------------------------------------------------
					pNode->Lock( 1.0 );

					if ( pNode->GetHint() && ( pNode->GetHint()->HintType() == HINT_TACTICAL_COVER_MED || pNode->GetHint()->HintType() == HINT_TACTICAL_COVER_LOW ) )
					{
						if ( GetOuter()->GetHintNode() )
						{
							GetOuter()->GetHintNode()->Unlock(GetOuter()->GetHintDelay(GetOuter()->GetHintNode()->HintType()));
							GetOuter()->SetHintNode( NULL );
						}

						GetOuter()->SetHintNode( pNode->GetHint() );
					}

					// The next NPC who searches should use a slight different pattern
					nSearchRandomizer = nodeIndex;
					DebugFindCover( pNode->GetId(), vEyePos, vThreatEyePos, 0, 255, 0 );
					return nodeIndex;
				}
				else
				{
					DebugFindCover( pNode->GetId(), vEyePos, vThreatEyePos, 255, 0, 0 );
				}
			}
			else
			{
				DebugFindCover( pNode->GetId(), vEyePos, vThreatEyePos, 0, 0, 255 );
			}
		}

		// Add its children to the search list
		// Go through each link
		// UNDONE: Pass in a cost function to measure each link?
		for ( int link = 0; link < GetNetwork()->GetNode(nodeIndex)->NumLinks(); link++ ) 
		{
			int index = (link + nSearchRandomizer) % GetNetwork()->GetNode(nodeIndex)->NumLinks();
			CAI_Link *nodeLink = GetNetwork()->GetNode(nodeIndex)->GetLinkByIndex(index);

			if ( !m_pPathfinder->IsLinkUsable( nodeLink, iMyNode ) )
				continue;

			int newID = nodeLink->DestNodeID(nodeIndex);

			// If not already on the closed list, add to it and set its distance
			if (!wasVisited.IsBitSet(newID))
			{
				// Don't accept climb nodes or nodes that aren't ready to use yet
				if ( GetNetwork()->GetNode(newID)->GetType() != NODE_CLIMB && !GetNetwork()->GetNode(newID)->IsLocked() )
				{
					// UNDONE: Shouldn't we really accumulate the distance by path rather than
					// absolute distance.  After all, we are performing essentially an A* here.
					nodeOrigin = GetNetwork()->GetNode(newID)->GetPosition(GetHullType());
					dist = (vNearPos - nodeOrigin).LengthSqr();

					// use distance to threat as a heuristic to keep AIs from running toward
					// the threat in order to take cover from it.
					float threatDist = (vThreatPos - nodeOrigin).LengthSqr();

					// Now check this node is not too close towards the threat
					if ( dist < threatDist * 1.5 )
					{
						list.Insert( AI_NearNode_t(newID, dist) );
					}
				}
				// mark visited
				wasVisited.Set(newID);
			}
		}
	}

	// We failed.  Not cover node was found
	// Clear hint node used to set ducking
	GetOuter()->ClearHintNode();
	return NO_NODE;
}


//-------------------------------------
// Purpose:  Return node ID that has line of sight to target I want to shoot
//
// Input  :	pNPC			- npc that's looking for a place to shoot from
//			vThreatPos		- position of entity/location I'm trying to shoot
//			vThreatEyePos	- eye position of entity I'm trying to shoot. If 
//							  entity has no eye position, just give vThreatPos again
//			flMinThreatDist	- minimum distance that node must be from vThreatPos
//			flMaxThreadDist	- maximum distance that node can be from vThreadPos
//			vThreatFacing	- optional argument.  If given the returned node
//							  will also be behind the given facing direction (flanking)
//			flBlockTime		- how long to block this node from use
// Output :	int				- ID number of node that meets qualifications
//-------------------------------------

int CAI_TacticalServices::FindLosNode(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam )
{
	if ( !CAI_NetworkManager::NetworksLoaded() )
		return NO_NODE;

	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLosNode );

	MARK_TASK_EXPENSIVE();

	int iMyNode	= GetPathfinder()->NearestNodeToNPC();
	if ( iMyNode == NO_NODE )
	{
		Vector pos = GetOuter()->GetAbsOrigin();
		DevWarning( 2, "FindCover() - %s has no nearest node! (Check near %f %f %f)\n", GetEntClassname(), pos.x, pos.y, pos.z);
		return NO_NODE;
	}

	// ------------------------------------------------------------------------------------
	// We're going to search for a shoot node by expanding to our current node's neighbors
	// and then their neighbors, until a shooting position is found, or all nodes are beyond MaxDist
	// ------------------------------------------------------------------------------------
	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * GetNetwork()->NumNodes() );
	CNodeList list( pBuffer, GetNetwork()->NumNodes() );
	CVarBitVec wasVisited(GetNetwork()->NumNodes());	// Nodes visited

	// mark start as visited
	wasVisited.Set( iMyNode );
	list.Insert( AI_NearNode_t(iMyNode, 0) );

	static int nSearchRandomizer = 0;		// tries to ensure the links are searched in a different order each time;

	while ( list.Count() )
	{
		int nodeIndex = list.ElementAtHead().nodeIndex;
		// remove this item from the list
		list.RemoveAtHead();

		const Vector &nodeOrigin = GetNetwork()->GetNode(nodeIndex)->GetPosition(GetHullType());

		// HACKHACK: Can't we rework this loop and get rid of this?
		// skip the starting node, or we probably wouldn't have called this function.
		if ( nodeIndex != iMyNode )
		{
			bool skip = false;

			// See if the node satisfies the flanking criteria.
			switch ( eFlankType )
			{
				case FLANKTYPE_NONE:
					break;
					
				case FLANKTYPE_RADIUS:
				{
					Vector vecDist = nodeOrigin - vecFlankRefPos;
					if ( vecDist.Length() < flFlankParam )
					{
						skip = true;
					}
					
					break;
				}
				
				case FLANKTYPE_ARC:
				{
					Vector vecEnemyToRef = vecFlankRefPos - vThreatPos;
					VectorNormalize( vecEnemyToRef );

					Vector vecEnemyToNode = nodeOrigin - vThreatPos;
					VectorNormalize( vecEnemyToNode );
					
					float flDot = DotProduct( vecEnemyToRef, vecEnemyToNode );
					
					if ( RAD2DEG( acos( flDot ) ) < flFlankParam )
					{
						skip = true;
					}
					
					break;
				}
			}

			// Don't accept climb nodes, and assume my nearest node isn't valid because
			// we decided to make this check in the first place.  Keep moving
			if ( !skip && !GetNetwork()->GetNode(nodeIndex)->IsLocked() &&
				GetNetwork()->GetNode(nodeIndex)->GetType() != NODE_CLIMB )
			{
				// Now check its distance and only accept if in range
				float flThreatDist = ( nodeOrigin - vThreatPos ).Length();

				if ( flThreatDist < flMaxThreatDist &&
					 flThreatDist > flMinThreatDist )
				{
					CAI_Node *pNode = GetNetwork()->GetNode(nodeIndex);
					if ( GetOuter()->IsValidShootPosition( nodeOrigin, pNode, pNode->GetHint() ) )
					{
						if (GetOuter()->TestShootPosition(nodeOrigin,vThreatEyePos))
						{
							// Note when this node was used, so we don't try 
							// to use it again right away.
							GetNetwork()->GetNode(nodeIndex)->Lock( flBlockTime );

#if 0
							if ( GetOuter()->GetHintNode() )
							{
								GetOuter()->GetHintNode()->Unlock(GetOuter()->GetHintDelay(GetOuter()->GetHintNode()->HintType()));
								GetOuter()->SetHintNode( NULL );
							}

							// This used to not be set, why? (kenb)
							// @Note (toml 05-19-04): I think because stomping  the hint can lead to
							// unintended side effects. The hint node is primarily a high level
							// tool, and certain NPCs break if it gets slammed here. If we need
							// this, we should propagate it out and let the schedule selector
							// or task decide to set the hint node
							GetOuter()->SetHintNode( GetNetwork()->GetNode(nodeIndex)->GetHint() );
#endif
							if ( ShouldDebugLos( nodeIndex ) )
							{
								NDebugOverlay::Text( nodeOrigin, CFmtStr( "%d:los!", nodeIndex), false, 1 );
							}

							// The next NPC who searches should use a slight different pattern
							nSearchRandomizer = nodeIndex;
							return nodeIndex;
						}
						else
						{
							if ( ShouldDebugLos( nodeIndex ) )
							{
								NDebugOverlay::Text( nodeOrigin, CFmtStr( "%d:!shoot", nodeIndex), false, 1 );
							}
						}
					}
					else
					{
						if ( ShouldDebugLos( nodeIndex ) )
						{
							NDebugOverlay::Text( nodeOrigin, CFmtStr( "%d:!valid", nodeIndex), false, 1 );
						}
					}
				}
				else
				{
					if ( ShouldDebugLos( nodeIndex ) )
					{
						CFmtStr msg( "%d:%s", nodeIndex, ( flThreatDist < flMaxThreatDist ) ? "too close" : "too far" );
						NDebugOverlay::Text( nodeOrigin, msg, false, 1 );
					}
				}
			}
		}

		// Go through each link and add connected nodes to the list
		for (int link=0; link < GetNetwork()->GetNode(nodeIndex)->NumLinks();link++) 
		{
			int index = (link + nSearchRandomizer) % GetNetwork()->GetNode(nodeIndex)->NumLinks();
			CAI_Link *nodeLink = GetNetwork()->GetNode(nodeIndex)->GetLinkByIndex(index);

			if ( !m_pPathfinder->IsLinkUsable( nodeLink, iMyNode ) )
				continue;

			int newID = nodeLink->DestNodeID(nodeIndex);

			// If not already visited, add to the list
			if (!wasVisited.IsBitSet(newID))
			{
				float dist = (GetLocalOrigin() - GetNetwork()->GetNode(newID)->GetPosition(GetHullType())).LengthSqr();
				list.Insert( AI_NearNode_t(newID, dist) );
				wasVisited.Set( newID );
			}
		}
	}
	// We failed.  No range attack node node was found
	return NO_NODE;
}

//-------------------------------------
// Checks lateral LOS
//-------------------------------------
bool CAI_TacticalServices::TestLateralLos( const Vector &vecCheckStart, const Vector &vecCheckEnd )
{
	trace_t	tr;

	// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
	AI_TraceLOS(  vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset(), NULL, &tr );

	if (tr.fraction == 1.0)
	{
		if ( GetOuter()->IsValidShootPosition( vecCheckEnd, NULL, NULL ) )
			{
				if (GetOuter()->TestShootPosition(vecCheckEnd,vecCheckStart))
				{
					AIMoveTrace_t moveTrace;
					GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace );
					if (moveTrace.fStatus == AIMR_OK)
					{
						return true;
					}
				}
		}
	}

	return false;
}


//-------------------------------------

bool CAI_TacticalServices::FindLateralLos( const Vector &vecThreat, Vector *pResult )
{
	AI_PROFILE_SCOPE( CAI_TacticalServices_FindLateralLos );

	if( !m_bAllowFindLateralLos )
	{
		return false;
	}

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	bool	bLookingForEnemy = GetEnemy() && VectorsAreEqual(vecThreat, GetEnemy()->EyePosition(), 0.1f);
	int		i;

	if(  !bLookingForEnemy || GetOuter()->HasCondition(COND_SEE_ENEMY) || GetOuter()->HasCondition(COND_HAVE_ENEMY_LOS) || 
		 GetOuter()->GetTimeScheduleStarted() == gpGlobals->curtime ) // Conditions get nuked before tasks run, assume should try
	{
		// My current position might already be valid.
		if ( TestLateralLos(vecThreat, GetLocalOrigin()) )
		{
			*pResult = GetLocalOrigin();
			return true;
		}
	}

	if( !ai_find_lateral_los.GetBool() )
	{
		// Allows us to turn off lateral LOS at the console. Allow the above code to run 
		// just in case the NPC has line of sight to begin with.
		return false;
	}

	int iChecks = COVER_CHECKS;
	int iDelta = COVER_DELTA;

	// If we're limited in how far we're allowed to move laterally, don't bother checking past it
	int iMaxLateralDelta = GetOuter()->GetMaxTacticalLateralMovement();
	if ( iMaxLateralDelta != MAXTACLAT_IGNORE && iMaxLateralDelta < iDelta )
	{
		iChecks = 1;
		iDelta = iMaxLateralDelta;
	}

	Vector right;
	AngleVectors( GetLocalAngles(), NULL, &right, NULL );
	vecStepRight = right * iDelta;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = GetLocalOrigin();
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < iChecks; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralLos( vecCheckStart, vecLeftTest ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralLos( vecCheckStart, vecRightTest ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}

//-------------------------------------

Vector CAI_TacticalServices::GetNodePos( int node )
{
	return GetNetwork()->GetNode((int)node)->GetPosition(GetHullType());
}

//-----------------------------------------------------------------------------
