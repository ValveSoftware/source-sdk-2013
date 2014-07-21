//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_node.cpp
// AI Navigation Nodes
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include "cbase.h"
#include "nav_node.h"
#include "nav_colors.h"
#include "nav_mesh.h"
#include "tier1/utlhash.h"
#include "tier1/generichash.h"

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


NavDirType Opposite[ NUM_DIRECTIONS ] = { SOUTH, WEST, NORTH, EAST };

CNavNode *CNavNode::m_list = NULL;
unsigned int CNavNode::m_listLength = 0;
unsigned int CNavNode::m_nextID = 1;

extern Vector NavTraceMins;
extern Vector NavTraceMaxs;

//--------------------------------------------------------------------------------------------------------------
// Node hash

class CNodeHashFuncs
{
public:
	CNodeHashFuncs( int ) {}

	bool operator()( const CNavNode *pLhs, const CNavNode *pRhs ) const
	{
		return pRhs->GetPosition()->AsVector2D() == pLhs->GetPosition()->AsVector2D();
	}

	unsigned int operator()( const CNavNode *pItem ) const
	{
		return Hash8( &pItem->GetPosition()->AsVector2D() );	
	}
};

CUtlHash<CNavNode *, CNodeHashFuncs, CNodeHashFuncs> *g_pNavNodeHash;


//--------------------------------------------------------------------------------------------------------------
/**
 * Constructor
 */
CNavNode::CNavNode( const Vector &pos, const Vector &normal, CNavNode *parent, bool isOnDisplacement )
{
	m_pos = pos;
	m_normal = normal;

	m_id = m_nextID++;

	int i;
	for( i=0; i<NUM_DIRECTIONS; ++i )
	{
		m_to[ i ] = NULL;
		m_obstacleHeight[ i ] = 0;
		m_obstacleStartDist[ i ] = 0;
		m_obstacleEndDist[ i ] = 0;
	}

	for ( i=0; i<NUM_CORNERS; ++i )
	{
		m_crouch[ i ] = false;
		m_isBlocked[ i ] = false;
	}

	m_visited = 0;
	m_parent = parent;

	m_next = m_list;
	m_list = this;
	m_listLength++;

	m_isCovered = false;
	m_area = NULL;

	m_attributeFlags = 0;

	m_isOnDisplacement = isOnDisplacement;

	if ( !g_pNavNodeHash )
	{
		g_pNavNodeHash = new CUtlHash<CNavNode *, CNodeHashFuncs, CNodeHashFuncs>( 16*1024 );
	}

	bool bDidInsert;
	UtlHashHandle_t hHash = g_pNavNodeHash->Insert( this, &bDidInsert );
	if ( !bDidInsert )
	{
		CNavNode *pExistingNode = g_pNavNodeHash->Element( hHash );
		m_nextAtXY = pExistingNode;
		g_pNavNodeHash->Element( hHash ) = this;
	}
	else
	{
		m_nextAtXY = NULL;
	}
}

CNavNode::~CNavNode()
{
}


//--------------------------------------------------------------------------------------------------------------
void CNavNode::CleanupGeneration()
{
	delete g_pNavNodeHash;
	g_pNavNodeHash = NULL;

	CNavNode *node, *next;
	for( node = CNavNode::m_list; node; node = next )
	{
		next = node->m_next;
		delete node;
	}
	CNavNode::m_list = NULL;
	CNavNode::m_listLength = 0;
	CNavNode::m_nextID = 1;
}

//--------------------------------------------------------------------------------------------------------------
#if DEBUG_NAV_NODES
ConVar nav_show_nodes( "nav_show_nodes", "0", FCVAR_CHEAT );
ConVar nav_show_node_id( "nav_show_node_id", "0", FCVAR_CHEAT );
ConVar nav_test_node( "nav_test_node", "0", FCVAR_CHEAT );
ConVar nav_test_node_crouch( "nav_test_node_crouch", "0", FCVAR_CHEAT );
ConVar nav_test_node_crouch_dir( "nav_test_node_crouch_dir", "4", FCVAR_CHEAT );
ConVar nav_show_node_grid( "nav_show_node_grid", "0", FCVAR_CHEAT );
#endif // DEBUG_NAV_NODES


//--------------------------------------------------------------------------------------------------------------
void CNavNode::Draw( void )
{
#if DEBUG_NAV_NODES

	if ( !nav_show_nodes.GetBool() )
		return;

	int r = 0, g = 0, b = 0;

	if ( m_isCovered )
	{
		if ( GetAttributes() & NAV_MESH_CROUCH )
		{
			b = 255;
		}
		else
		{
			r = 255;
		}
	}
	else
	{
		if ( GetAttributes() & NAV_MESH_CROUCH )
		{
			b = 255;
		}
		g = 255;
	}

	NDebugOverlay::Cross3D( m_pos, 2, r, g, b, true, 0.1f );

	if ( (!m_isCovered && nav_show_node_id.GetBool()) || (m_isCovered && nav_show_node_id.GetInt() < 0) )
	{
		char text[16];
		Q_snprintf( text, sizeof( text ), "%d", m_id );
		NDebugOverlay::Text( m_pos, text, true, 0.1f );
	}

	if ( (unsigned int)(nav_test_node.GetInt()) == m_id )
	{
		TheNavMesh->TestArea( this, 1, 1 );
		nav_test_node.SetValue( 0 );
	}

	if ( (unsigned int)(nav_test_node_crouch.GetInt()) == m_id )
	{
		CheckCrouch();
		nav_test_node_crouch.SetValue( 0 );
	}

	if ( GetAttributes() & NAV_MESH_CROUCH )
	{
		int i;
		for( i=0; i<NUM_CORNERS; i++ )
		{
			if ( m_isBlocked[i] || m_crouch[i] )
			{
				Vector2D dir;
				CornerToVector2D( (NavCornerType)i, &dir );

				const float scale = 3.0f;
				Vector scaled( dir.x * scale, dir.y * scale, 0 );

				if ( m_isBlocked[i] )
				{
					NDebugOverlay::HorzArrow( m_pos, m_pos + scaled, 0.5, 255, 0, 0, 255, true, 0.1f );
				}
				else
				{
					NDebugOverlay::HorzArrow( m_pos, m_pos + scaled, 0.5, 0, 0, 255, 255, true, 0.1f );
				}
			}
		}
	}

	if ( nav_show_node_grid.GetBool() )
	{
		for ( int i = NORTH; i < NUM_DIRECTIONS; i++ )
		{
			CNavNode *nodeNext = GetConnectedNode( (NavDirType) i );
			if ( nodeNext )
			{
				NDebugOverlay::Line( *GetPosition(), *nodeNext->GetPosition(), 255, 255, 0, false, 0.1f );

				float obstacleHeight = m_obstacleHeight[i];
				if ( obstacleHeight > 0 )
				{
					float z = GetPosition()->z + obstacleHeight;
					Vector from = *GetPosition();
					Vector to = from;
					AddDirectionVector( &to, (NavDirType) i, m_obstacleStartDist[i] );
					NDebugOverlay::Line( from, to, 255, 0, 255, false, 0.1f );
					from = to;
					to.z = z;
					NDebugOverlay::Line( from, to, 255, 0, 255, false, 0.1f );
					from = to;
					to = *GetPosition();
					to.z = z;
					AddDirectionVector( &to, (NavDirType) i, m_obstacleEndDist[i] );
					NDebugOverlay::Line( from, to, 255, 0, 255, false, 0.1f );
				}				
			}
		}
	}
	

#endif // DEBUG_NAV_NODES
}


//--------------------------------------------------------------------------------------------------------
// return ground height above node in given corner direction (NUM_CORNERS for highest in any direction)
float CNavNode::GetGroundHeightAboveNode( NavCornerType cornerType ) const
{
	if ( cornerType >= 0 && cornerType < NUM_CORNERS )
		return m_groundHeightAboveNode[ cornerType ];

	float blockedHeight = 0.0f;
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
		blockedHeight = MAX( blockedHeight, m_groundHeightAboveNode[i] );
	}

	return blockedHeight;
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Look up to JumpCrouchHeight in the air to see if we can fit a whole HumanHeight box
 */
bool CNavNode::TestForCrouchArea( NavCornerType cornerNum, const Vector& mins, const Vector& maxs, float *groundHeightAboveNode )
{
	CTraceFilterWalkableEntities filter( NULL, COLLISION_GROUP_PLAYER_MOVEMENT, WALK_THRU_EVERYTHING );
	trace_t tr;

	Vector start( m_pos );
	Vector end( start );
	end.z += JumpCrouchHeight;
	UTIL_TraceHull( start, end, NavTraceMins, NavTraceMaxs, MASK_NPCSOLID_BRUSHONLY, &filter, &tr );

	float maxHeight = tr.endpos.z - start.z;

	Vector realMaxs( maxs );

	for ( float height = 0; height <= maxHeight; height += 1.0f )
	{
		start = m_pos;
		start.z += height;

		realMaxs.z = HumanCrouchHeight;
		UTIL_TraceHull( start, start, mins, realMaxs, MASK_NPCSOLID_BRUSHONLY, &filter, &tr );
		if ( !tr.startsolid )
		{
			*groundHeightAboveNode = start.z - m_pos.z;

			// We found a crouch-sized space.  See if we can stand up.
			realMaxs.z = HumanHeight;
			UTIL_TraceHull( start, start, mins, realMaxs, MASK_NPCSOLID_BRUSHONLY, &filter, &tr );
			if ( !tr.startsolid )
			{
				// We found a crouch-sized space.  See if we can stand up.
#if DEBUG_NAV_NODES
				if ( (unsigned int)(nav_test_node_crouch.GetInt()) == GetID() )
				{
					NDebugOverlay::Box( start, mins, maxs, 0, 255, 255, 100, 100 );
				}
#endif // DEBUG_NAV_NODES
				return true;
			}
#if DEBUG_NAV_NODES
			if ( (unsigned int)(nav_test_node_crouch.GetInt()) == GetID() )
			{
				NDebugOverlay::Box( start, mins, maxs, 255, 0, 0, 100, 100 );
			}
#endif // DEBUG_NAV_NODES

			return false;
		}
	}

	*groundHeightAboveNode = JumpCrouchHeight;
	m_isBlocked[ cornerNum ] = true;
	return false;
}


//--------------------------------------------------------------------------------------------------------------
void CNavNode::CheckCrouch( void )
{
	// For each direction, trace upwards from our best ground height to VEC_HULL_MAX.z to see if we have standing room.
	for ( int i=0; i<NUM_CORNERS; ++i )
	{
#if DEBUG_NAV_NODES
		if ( nav_test_node_crouch_dir.GetInt() != NUM_CORNERS && i != nav_test_node_crouch_dir.GetInt() )
			continue;
#endif // DEBUG_NAV_NODES

		NavCornerType corner = (NavCornerType)i;
		Vector2D cornerVec;
		CornerToVector2D( corner, &cornerVec );

		// Build a mins/maxs pair for the HumanWidth x HalfHumanWidth box facing the appropriate direction
		Vector mins( 0, 0, 0 );
		Vector maxs( 0, 0, 0 );
		if ( cornerVec.x < 0 )
		{
			mins.x = -HalfHumanWidth;
		}
		else if ( cornerVec.x > 0 )
		{
			maxs.x = HalfHumanWidth;
		}
		if ( cornerVec.y < 0 )
		{
			mins.y = -HalfHumanWidth;
		}
		else if ( cornerVec.y > 0 )
		{
			maxs.y = HalfHumanWidth;
		}
		maxs.z = HumanHeight;

		// now make sure that mins is smaller than maxs
		for ( int j=0; j<3; ++j )
		{
			if ( mins[j] > maxs[j] )
			{
				float tmp = mins[j];
				mins[j] = maxs[j];
				maxs[j] = tmp;
			}
		}

		if ( !TestForCrouchArea( corner, mins, maxs, &m_groundHeightAboveNode[i] ) )
		{
			SetAttributes( NAV_MESH_CROUCH );
			m_crouch[corner] = true;
		}
	}
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Create a connection FROM this node TO the given node, in the given direction
 */
void CNavNode::ConnectTo( CNavNode *node, NavDirType dir, float obstacleHeight, float obstacleStartDist, float obstacleEndDist )
{
	Assert( obstacleStartDist >= 0 && obstacleStartDist <= GenerationStepSize );
	Assert( obstacleEndDist >= 0 && obstacleStartDist <= GenerationStepSize );
	Assert( obstacleStartDist < obstacleEndDist );

	m_to[ dir ] = node;
	m_obstacleHeight[ dir ] = obstacleHeight;
	m_obstacleStartDist[ dir ] = obstacleStartDist;
	m_obstacleEndDist[ dir ] = obstacleEndDist;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return node at given position.
 * @todo Need a hash table to make this lookup fast
 */
CNavNode *CNavNode::GetNode( const Vector &pos )
{
	const float tolerance = 0.45f * GenerationStepSize;			// 1.0f
	CNavNode *pNode = NULL;
	if ( g_pNavNodeHash )
	{
		static CNavNode lookup;
		lookup.m_pos = pos;
		UtlHashHandle_t hNode = g_pNavNodeHash->Find( &lookup );

		if ( hNode != g_pNavNodeHash->InvalidHandle() )
		{
			for( pNode = g_pNavNodeHash->Element( hNode ); pNode; pNode = pNode->m_nextAtXY )
			{
				float dz = fabs( pNode->m_pos.z - pos.z );

				if (dz < tolerance)
				{
					break;
				}
			}
		}
	}

#ifdef DEBUG_NODE_HASH
	CNavNode *pTestNode = NULL;
	for( CNavNode *node = m_list; node; node = node->m_next )
	{
		float dx = fabs( node->m_pos.x - pos.x );
		float dy = fabs( node->m_pos.y - pos.y );
		float dz = fabs( node->m_pos.z - pos.z );

		if (dx < tolerance && dy < tolerance && dz < tolerance)
		{
			pTestNode = node;
			break;
		}
	}
	AssertFatal( pTestNode == pNode );
#endif

	return pNode;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if this node is bidirectionally linked to 
 * another node in the given direction
 */
BOOL CNavNode::IsBiLinked( NavDirType dir ) const
{
	if (m_to[ dir ] && m_to[ dir ]->m_to[ Opposite[dir] ] == this)
	{
		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if this node is the NW corner of a quad of nodes
 * that are all bidirectionally linked.
 */
BOOL CNavNode::IsClosedCell( void ) const
{
	if (IsBiLinked( SOUTH ) &&
		IsBiLinked( EAST ) &&
		m_to[ EAST ]->IsBiLinked( SOUTH ) &&
		m_to[ SOUTH ]->IsBiLinked( EAST ) &&
		m_to[ EAST ]->m_to[ SOUTH ] == m_to[ SOUTH ]->m_to[ EAST ])
	{
		return true;
	}

	return false;
}

