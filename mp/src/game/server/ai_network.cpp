//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_network.h"
#include "ai_node.h"
#include "ai_basenpc.h"
#include "ai_link.h"
#include "ai_navigator.h"
#include "world.h"
#include "ai_moveprobe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ai_no_node_cache( "ai_no_node_cache", "0" );

extern float MOVE_HEIGHT_EPSILON;

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// For now we just have one AINetwork called "BigNet".  At some
// later point we will probabaly have multiple AINetworkds per level
CAI_Network*		g_pBigAINet;			

//-----------------------------------------------------------------------------

abstract_class INodeListFilter
{
public:
	virtual bool	NodeIsValid( CAI_Node &node ) = 0;
	virtual float	NodeDistanceSqr( CAI_Node &node ) = 0;
};

//-------------------------------------
// Purpose: Filters nodes for an NPC
//-------------------------------------

class CNodeFilter : public INodeListFilter
{
public:
	CNodeFilter( CAI_BaseNPC *pNPC, const Vector &pos ) : m_pNPC(pNPC), m_pos(pos) 
	{
		if ( m_pNPC )
			m_capabilities = m_pNPC->CapabilitiesGet();
	}

	CNodeFilter( const Vector &pos ) : m_pNPC(NULL), m_pos(pos) 
	{
	}

	virtual bool NodeIsValid( CAI_Node &node )
	{
		if ( node.GetType() == NODE_DELETED )
			return false;

		if ( !m_pNPC )
			return true;

		if ( m_pNPC->GetNavType() == NAV_FLY && node.GetType() != NODE_AIR )
			return false;

		// Check that node is of proper type for this NPC's navigation ability
		if ((node.GetType() == NODE_AIR    && !(m_capabilities & bits_CAP_MOVE_FLY))		||
			(node.GetType() == NODE_GROUND && !(m_capabilities & bits_CAP_MOVE_GROUND))	)
			return false;
			
		if ( m_pNPC->IsUnusableNode( node.GetId(), node.GetHint() ) )
			return false;

		return true;
	}

	virtual float	NodeDistanceSqr( CAI_Node &node )
	{
		// UNDONE: This call to Position() really seems excessive here.  What is the real
		// error % relative to 800 units (MAX_NODE_LINK_DIST) ?
		if ( m_pNPC )
			return (node.GetPosition(m_pNPC->GetHullType()) - m_pos).LengthSqr();
		else
			return (node.GetOrigin() - m_pos).LengthSqr();
	}

	const Vector &m_pos;
	CAI_BaseNPC	*m_pNPC;
	int			m_capabilities;	// cache this
};

//-----------------------------------------------------------------------------
// CAI_Network
//-----------------------------------------------------------------------------

// for searching for the nearest node
// PERFORMANCE: Tune this number
#define MAX_NEAR_NODES	10			// Trace to 10 nodes at most

//-----------------------------------------------------------------------------

CAI_Network::CAI_Network()
{
	m_iNumNodes				= 0;		// Number of nodes in this network
	m_pAInode				= NULL;		// Array of all nodes in this network

	m_iNearestCacheNext	= NEARNODE_CACHE_SIZE - 1;
	// Force empty node caches to be rebuild
	for (int node=0;node<NEARNODE_CACHE_SIZE;node++)
	{
		m_NearestCache[node].hull = HULL_NONE;
		m_NearestCache[node].expiration	= FLT_MIN;
	}

#ifdef AI_NODE_TREE
	m_pNodeTree = NULL;
#endif
}

//-----------------------------------------------------------------------------

CAI_Network::~CAI_Network()
{
#ifdef AI_NODE_TREE
	if ( m_pNodeTree )
	{
		engine->DestroySpatialPartition( m_pNodeTree );
		m_pNodeTree = NULL;
	}
#endif
	if ( m_pAInode )
	{
		for ( int node = 0; node < m_iNumNodes; node++ )
		{
			CAI_Node *pNode = m_pAInode[node];
			Assert( pNode && pNode->m_iID == node );

			for ( int link = 0; link < pNode->NumLinks(); link++ )
			{
				CAI_Link *pLink = pNode->m_Links[link];
				if ( pLink )
				{
					int destID = pLink->DestNodeID( node );
					Assert( destID > node && destID < m_iNumNodes );
					if ( destID > node && destID < m_iNumNodes )
					{
						CAI_Node *pDestNode = m_pAInode[destID];
						Assert( pDestNode );

						for ( int destLink = 0; destLink < pDestNode->NumLinks(); destLink++ )
						{
							if ( pDestNode->m_Links[destLink] == pLink )
							{
								pDestNode->m_Links[destLink] = NULL;
							}
						}
					}
					delete pLink;
				}
			}
			delete pNode;
		}
	}
	delete[] m_pAInode;
	m_pAInode = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Given an bitString and float array of size array_size, return the 
//			index of the smallest number in the array whose it is set
//-----------------------------------------------------------------------------

int	CAI_Network::FindBSSmallest(CVarBitVec *bitString, float *float_array, int array_size) 
{
	int	  winIndex = -1;
	float winSize  = FLT_MAX;
	for (int i=0;i<array_size;i++) 
	{
		if (bitString->IsBitSet(i) && (float_array[i]<winSize)) 
		{
			winIndex = i;
			winSize  = float_array[i];
		}
	}
	return winIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Build a list of nearby nodes sorted by distance
// Input  : &list - 
//			maxListCount - 
//			*pFilter - 
// Output : int - count of list
//-----------------------------------------------------------------------------

int CAI_Network::ListNodesInBox( CNodeList &list, int maxListCount, const Vector &mins, const Vector &maxs, INodeListFilter *pFilter )
{
	CNodeList result;
	
	result.SetLessFunc( CNodeList::RevIsLowerPriority );
	
	// NOTE: maxListCount must be > 0 or this will crash
	bool full = false;
	float flClosest = 1000000.0 * 1000000;
	int closest = 0;

// UNDONE: Store the nodes in a tree and query the tree instead of the entire list!!!
	for ( int node = 0; node < m_iNumNodes; node++ )
	{
		CAI_Node *pNode = m_pAInode[node];
		const Vector &origin = pNode->GetOrigin();
		// in box?
		if ( origin.x < mins.x || origin.x > maxs.x ||
			 origin.y < mins.y || origin.y > maxs.y ||
			 origin.z < mins.z || origin.z > maxs.z )
			continue;

		if ( !pFilter->NodeIsValid(*pNode) )
			continue;

		float flDist = pFilter->NodeDistanceSqr(*pNode);

		if ( flDist < flClosest )
		{
			closest = node;
			flClosest = flDist;
		}

		if ( !full || (flDist < result.ElementAtHead().dist) )
		{
			if ( full )
				result.RemoveAtHead();

			result.Insert( AI_NearNode_t(node, flDist) );
	
			full = (result.Count() == maxListCount);
		}
	}
	
	list.RemoveAll();
	while ( result.Count() )
	{
		list.Insert( result.ElementAtHead() );
		result.RemoveAtHead();
	}

	return list.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Return ID of node nearest of vecOrigin for pNPC with the given
//			tolerance distance.  If a route is required to get to the node
//			node_route is set.
//-----------------------------------------------------------------------------

int	CAI_Network::NearestNodeToPoint( CAI_BaseNPC *pNPC, const Vector &vecOrigin, bool bCheckVisibility, INearestNodeFilter *pFilter )
{
	AI_PROFILE_SCOPE( CAI_Network_NearestNodeToNPCAtPoint );
	
	// --------------------------------
	//  Check if network has no nodes
	// --------------------------------
	if (m_iNumNodes == 0)
		return NO_NODE;

	// ----------------------------------------------------------------
	//  First check cached nearest node positions
	// ----------------------------------------------------------------
	int cachePos;
	int cachedNode = GetCachedNearestNode( vecOrigin, pNPC, &cachePos );
	if ( cachedNode != NO_NODE )
	{
		if ( bCheckVisibility )
		{
			trace_t tr;

			Vector vTestLoc = ( pNPC ) ? 
								m_pAInode[cachedNode]->GetPosition(pNPC->GetHullType()) + pNPC->GetViewOffset() : 
								m_pAInode[cachedNode]->GetOrigin();

			CTraceFilterNav traceFilter( pNPC, true, pNPC, COLLISION_GROUP_NONE );
			AI_TraceLine ( vecOrigin, vTestLoc, MASK_NPCSOLID_BRUSHONLY, &traceFilter, &tr );

			if ( tr.fraction != 1.0 )
				cachedNode = NO_NODE;
		}

		if ( cachedNode != NO_NODE && ( !pFilter || pFilter->IsValid( m_pAInode[cachedNode] ) ) )
		{
			m_NearestCache[cachePos].expiration	= gpGlobals->curtime + NEARNODE_CACHE_LIFE;
			return cachedNode;
		}
	}

	// ---------------------------------------------------------------
	// First get nodes distances and eliminate those that are beyond 
	// the maximum allowed distance for local movements
	// ---------------------------------------------------------------
	CNodeFilter filter( pNPC, vecOrigin );

#ifdef AI_PERF_MON
		m_nPerfStatNN++;
#endif

	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * MAX_NEAR_NODES );
	CNodeList list( pBuffer, MAX_NEAR_NODES );

	// OPTIMIZE: If not flying, this box should be smaller in Z (2 * height?)
	Vector ext(MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST);
	// If the NPC can fly, check further
	if ( pNPC && ( pNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY ) )
	{
		ext.Init( MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST );
	}

	ListNodesInBox( list, MAX_NEAR_NODES, vecOrigin - ext, vecOrigin + ext, &filter );

	// --------------------------------------------------------------
	//  Now find a reachable node searching the close nodes first
	// --------------------------------------------------------------
	//int smallestVisibleID = NO_NODE;

	for( ;list.Count(); list.RemoveAtHead() )
	{
		int smallest = list.ElementAtHead().nodeIndex;

		// Check not already rejected above
		if ( smallest == cachedNode )
			continue;

		// Check that this node is usable by the current hull size
		if ( pNPC && !pNPC->GetNavigator()->CanFitAtNode(smallest))
			continue;

		if ( bCheckVisibility )
		{
			trace_t tr;

			Vector vTestLoc = ( pNPC ) ? 
								m_pAInode[smallest]->GetPosition(pNPC->GetHullType()) + pNPC->GetNodeViewOffset() : 
								m_pAInode[smallest]->GetOrigin();

			Vector vecVisOrigin = vecOrigin + Vector(0,0,1);

			CTraceFilterNav traceFilter( pNPC, true, pNPC, COLLISION_GROUP_NONE );
			AI_TraceLine ( vecVisOrigin, vTestLoc, MASK_NPCSOLID_BRUSHONLY, &traceFilter, &tr );

			if ( tr.fraction != 1.0 )
				continue;
		}

		if ( pFilter )
		{
			if ( !pFilter->IsValid( m_pAInode[smallest] ) )
			{
				if ( !pFilter->ShouldContinue() )
					break;
				continue;
			}
		}

		SetCachedNearestNode( vecOrigin, smallest, (pNPC) ? pNPC->GetHullType() : HULL_NONE );

		return smallest;
	}

	// Store inability to reach in cache for later use
	SetCachedNearestNode( vecOrigin, NO_NODE, (pNPC) ? pNPC->GetHullType() : HULL_NONE );

	return NO_NODE;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest node to an entity without regard to how whether
//			the node can be reached
//-----------------------------------------------------------------------------

int	CAI_Network::NearestNodeToPoint(const Vector &vPosition, bool bCheckVisibility )
{
	return NearestNodeToPoint( NULL, vPosition, bCheckVisibility );
}
	
//-----------------------------------------------------------------------------
// Purpose: Check nearest node cache for checkPos and return cached nearest
//			node if it exists in the cache.  Doesn't care about reachability,
//			only if the node is visible
//-----------------------------------------------------------------------------
int	CAI_Network::GetCachedNode(const Vector &checkPos, Hull_t nHull, int *pCachePos )
{
	if ( ai_no_node_cache.GetBool() )
		return NOT_CACHED;

	// Walk from newest to oldest.
	int iNewest = m_iNearestCacheNext + 1;
	for ( int i = 0; i < NEARNODE_CACHE_SIZE; i++ )
	{
		int iCurrent = ( iNewest + i ) % NEARNODE_CACHE_SIZE;
		if ( m_NearestCache[iCurrent].hull == nHull && m_NearestCache[iCurrent].expiration > gpGlobals->curtime )
		{
			if ( (m_NearestCache[iCurrent].vTestPosition - checkPos).LengthSqr() < Square(24.0) )
			{
				if ( pCachePos )
					*pCachePos = iCurrent;
				return m_NearestCache[iCurrent].node;
			}
		}
	}


	if ( pCachePos )
		*pCachePos = -1;
	return NOT_CACHED;
}

//-----------------------------------------------------------------------------

int	CAI_Network::GetCachedNearestNode(const Vector &checkPos, CAI_BaseNPC *pNPC, int *pCachePos )
{
	if ( pNPC )
	{
		CNodeFilter filter( pNPC, checkPos );

		int nodeID = GetCachedNode( checkPos, pNPC->GetHullType(), pCachePos );
		if ( nodeID >= 0 )
		{
			if ( filter.NodeIsValid( *m_pAInode[nodeID] ) && pNPC->GetNavigator()->CanFitAtNode(nodeID) )
				return nodeID;
		}
	}
	return NO_NODE;
}

//-----------------------------------------------------------------------------
// Purpose: Update nearest node cache with new data
//			if nHull == HULL_NONE, reachability of this node wasn't checked
//-----------------------------------------------------------------------------

void CAI_Network::SetCachedNearestNode(const Vector &checkPos, int nodeID, Hull_t nHull)
{
	if ( ai_no_node_cache.GetBool() )
		return;

	m_NearestCache[m_iNearestCacheNext].vTestPosition	= checkPos;
	m_NearestCache[m_iNearestCacheNext].node			= nodeID;
	m_NearestCache[m_iNearestCacheNext].hull			= nHull;
	m_NearestCache[m_iNearestCacheNext].expiration		= gpGlobals->curtime + NEARNODE_CACHE_LIFE;

	m_iNearestCacheNext--;
	if ( m_iNearestCacheNext < 0 )
	{
		m_iNearestCacheNext = NEARNODE_CACHE_SIZE - 1;
	}
}

//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( Hull_t hull, int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return vec3_origin;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return m_pAInode[nodeID]->GetPosition( hull );
}

//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( CBaseCombatCharacter *pNPC, int nodeID )
{
	if ( pNPC == NULL )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return GetNodePosition( pNPC->GetHullType(), nodeID );
}

//-----------------------------------------------------------------------------

float CAI_Network::GetNodeYaw( int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return 0.0f;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return 0.0f;
	}

	return m_pAInode[nodeID]->GetYaw();
}

//-----------------------------------------------------------------------------
// Purpose: Adds an ainode to the network
//-----------------------------------------------------------------------------

CAI_Node *CAI_Network::AddNode( const Vector &origin, float yaw )
{
	// ---------------------------------------------------------------------
	// When loaded from a file will know the number of nodes from 
	// the start.  Otherwise init MAX_NODES of them if not initialized
	// ---------------------------------------------------------------------
	if (!m_pAInode || !m_pAInode[0])
	{
		if ( m_pAInode )
			delete [] m_pAInode;
		m_pAInode	= new CAI_Node*[MAX_NODES];
	}

	if (m_iNumNodes >= MAX_NODES)
	{
		DevMsg( "ERROR: too many nodes in map, deleting last node.\n" );
		m_iNumNodes--;
	}

	m_pAInode[m_iNumNodes] = new CAI_Node( m_iNumNodes, origin, yaw );

#ifdef AI_NODE_TREE
	if ( !m_pNodeTree )
	{
		Vector worldMins, worldMaxs;
		GetWorldEntity()->GetWorldBounds( worldMins, worldMaxs );
		m_pNodeTree = engine->CreateSpatialPartition( worldMins, worldMaxs );
	}

	m_pNodeTree->CreateHandle( (IHandleEntity *)m_iNumNodes, PARTITION_NODE, origin, origin ); // ignore result for now as we'll never move or remove
#endif

	m_iNumNodes++;

	return m_pAInode[m_iNumNodes-1];
};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CAI_Link *CAI_Network::CreateLink( int srcID, int destID, CAI_DynamicLink *pDynamicLink )
{
	CAI_Node *pSrcNode = g_pBigAINet->GetNode( srcID );
	CAI_Node *pDestNode = g_pBigAINet->GetNode( destID );

	Assert( pSrcNode && pDestNode && pSrcNode != pDestNode );

	if ( !pSrcNode || !pDestNode )
	{
		DevMsg( "Attempted to create link to node that doesn't exist\n" );
		return NULL;
	}

	if ( pSrcNode == pDestNode )
	{
		DevMsg( "Attempted to link a node to itself\n" );
		return NULL;
	}

	if ( pSrcNode->NumLinks() == AI_MAX_NODE_LINKS )
	{
		DevMsg( "Node %d has too many links\n", srcID );
		return NULL;
	}
		
	if ( pDestNode->NumLinks() == AI_MAX_NODE_LINKS )
	{
		DevMsg( "Node %d has too many links\n", destID );
		return NULL;
	}

	CAI_Link *pLink = new CAI_Link;

	pLink->m_iSrcID = srcID;
	pLink->m_iDestID = destID;
	pLink->m_pDynamicLink = pDynamicLink;

	pSrcNode->AddLink(pLink);
	pDestNode->AddLink(pLink);

	return pLink;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true is two nodes are connected by the network graph
//-----------------------------------------------------------------------------

bool CAI_Network::IsConnected(int srcID, int destID)
{
	if (srcID > m_iNumNodes || destID > m_iNumNodes)
	{
		DevMsg("IsConnected called with invalid node IDs!\n");
		return false;
	}
	
	if ( srcID == destID )
		return true;
	
	int srcZone = m_pAInode[srcID]->GetZone();
	int destZone = m_pAInode[destID]->GetZone();
	
	if ( srcZone == AI_NODE_ZONE_SOLO || destZone == AI_NODE_ZONE_SOLO )
		return false;
		
	if ( srcZone == AI_NODE_ZONE_UNIVERSAL || destZone == AI_NODE_ZONE_UNIVERSAL ) // only happens in WC edit case
		return true;

#ifdef DEBUG
	if ( srcZone == AI_NODE_ZONE_UNKNOWN || destZone == AI_NODE_ZONE_UNKNOWN )
	{
		DevMsg( "Warning: Node found in unknown zone\n" );
		return true;
	}
#endif
		
	return ( srcZone == destZone );
}

//-----------------------------------------------------------------------------

IterationRetval_t CAI_Network::EnumElement( IHandleEntity *pHandleEntity )
{
#ifdef AI_NODE_TREE
	m_GatheredNodes.AddToTail( (int)pHandleEntity );
#endif
	return ITERATION_CONTINUE;
}

//=============================================================================
