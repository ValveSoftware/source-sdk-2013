//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		AI Utility classes for building the initial AI Networks
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_node.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_initutils.h"
#include "ai_networkmanager.h"

// to help eliminate node clutter by level designers, this is used to cap how many other nodes
// any given node is allowed to 'see' in the first stage of graph creation "LinkVisibleNodes()".

#include "ai_network.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( info_hint,			CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node,			CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_hint,		CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_air,		CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_air_hint,	CNodeEnt );	
LINK_ENTITY_TO_CLASS( info_node_climb,		CNodeEnt );
LINK_ENTITY_TO_CLASS( aitesthull, CAI_TestHull );

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_TestHull*	CAI_TestHull::pTestHull			= NULL;

#ifdef CSTRIKE_DLL
#define PLAYER_MODEL "models/player/ct_urban.mdl"
#else
#define PLAYER_MODEL "models/player.mdl"
#endif

//-----------------------------------------------------------------------------
// Purpose: Make sure we have a "player.mdl" hull to test with
//-----------------------------------------------------------------------------
void CAI_TestHull::Precache()
{
	BaseClass::Precache();
	PrecacheModel( PLAYER_MODEL );
}

//=========================================================
// CAI_TestHull::Spawn
//=========================================================
void CAI_TestHull::Spawn(void)
{
	Precache();

	SetModel( PLAYER_MODEL );

	// Set an initial hull size (this will change later)
	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_STEP );
	m_iHealth			= 50;

	bInUse				= false;

	// Make this invisible
	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: Get the test hull (create if none)
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_TestHull* CAI_TestHull::GetTestHull(void)
{
	if (!CAI_TestHull::pTestHull)
	{
		CAI_TestHull::pTestHull = CREATE_ENTITY( CAI_TestHull, "aitesthull" );
		CAI_TestHull::pTestHull->Spawn();
		CAI_TestHull::pTestHull->AddFlag( FL_NPC );
	}

	if (CAI_TestHull::pTestHull->bInUse == true)
	{
		DevMsg("WARNING: TestHull used and never returned!\n");
		Assert( 0 );
	}

	CAI_TestHull::pTestHull->RemoveSolidFlags( FSOLID_NOT_SOLID );
	CAI_TestHull::pTestHull->bInUse = true;

	return CAI_TestHull::pTestHull;
}

//-----------------------------------------------------------------------------
// Purpose: Get the test hull (create if none)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_TestHull::ReturnTestHull(void)
{
	CAI_TestHull::pTestHull->bInUse = false;
	CAI_TestHull::pTestHull->AddSolidFlags( FSOLID_NOT_SOLID );
	UTIL_SetSize(CAI_TestHull::pTestHull, vec3_origin, vec3_origin);

	UTIL_RemoveImmediate( pTestHull );
	pTestHull = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &startPos - 
//			&endPos - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_TestHull::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 1024.0f;
	const float MAX_JUMP_DISTANCE	= 1024.0f;
	const float MAX_JUMP_DROP		= 1024.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DISTANCE, MAX_JUMP_DROP );
}
//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_TestHull::~CAI_TestHull(void)
{
	CAI_TestHull::pTestHull = NULL;
}

//###########################################################
//	> CNodeEnt
//
// nodes start out as ents in the world. As they are spawned,
// the node info is recorded then the ents are discarded.
//###########################################################

//----------------------------------------------------
//  Static vars
//----------------------------------------------------
int CNodeEnt::m_nNodeCount = 0;

// -------------
// Data table
// -------------
BEGIN_SIMPLE_DATADESC( HintNodeData )

	DEFINE_FIELD(	 strEntityName,		FIELD_STRING ),
	//	DEFINE_FIELD(	 vecPosition,		FIELD_VECTOR ),		// Don't save
	DEFINE_KEYFIELD( nHintType,			FIELD_SHORT,	"hinttype" ),
	DEFINE_KEYFIELD( strGroup,			FIELD_STRING,	"Group" ),
	DEFINE_KEYFIELD( iDisabled,			FIELD_INTEGER,	"StartHintDisabled" ),
	DEFINE_FIELD(	 nNodeID,			FIELD_INTEGER ),
	DEFINE_KEYFIELD( iszActivityName,	FIELD_STRING,	"hintactivity" ),
    DEFINE_KEYFIELD( nTargetWCNodeID,	FIELD_INTEGER, "TargetNode" ),
	DEFINE_KEYFIELD( nWCNodeID,			FIELD_INTEGER,	"nodeid" ),
	DEFINE_KEYFIELD( fIgnoreFacing,		FIELD_INTEGER,	"IgnoreFacing" ),
	DEFINE_KEYFIELD( minState,			FIELD_INTEGER,	"MinimumState" ),
	DEFINE_KEYFIELD( maxState,			FIELD_INTEGER,	"MaximumState" ),

END_DATADESC()

// -------------
// Data table
// -------------
BEGIN_DATADESC( CNodeEnt )

	DEFINE_EMBEDDED( m_NodeData ),

END_DATADESC()

//=========================================================
//=========================================================
void CNodeEnt::Spawn( void )
{
	Spawn( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMapData - 
//-----------------------------------------------------------------------------
int CNodeEnt::Spawn( const char *pMapData )
{
	m_NodeData.strEntityName = GetEntityName();
	m_NodeData.vecPosition = GetAbsOrigin();
	m_NodeData.nNodeID = NO_NODE;
	if ( m_NodeData.minState == NPC_STATE_NONE )
		m_NodeData.minState = NPC_STATE_IDLE;
	if ( m_NodeData.maxState == NPC_STATE_NONE )
		m_NodeData.maxState = NPC_STATE_COMBAT;
	// ---------------------------------------------------------------------------------
	//  If just a hint node (not used for navigation) just create a hint and bail
	// ---------------------------------------------------------------------------------
	if (FClassnameIs( this, "info_hint" ))
	{
		if (m_NodeData.nHintType)
		{
			CAI_HintManager::CreateHint( &m_NodeData, pMapData );
		}
		else
		{
			Warning("info_hint (HammerID: %d, position (%.2f, %.2f, %.2f)) with no hint type.\n", m_NodeData.nWCNodeID, m_NodeData.vecPosition.x, m_NodeData.vecPosition.y, m_NodeData.vecPosition.z );
		}
		UTIL_RemoveImmediate( this );
		return -1;
	}
	
	// ---------------------------------------------------------------------------------
	//  First check if this node has a hint.  If so create a hint entity
	// ---------------------------------------------------------------------------------
	CAI_Hint *pHint = NULL;

	if ( ClassMatches( "info_node_hint" ) || ClassMatches( "info_node_air_hint" ) )
	{
		if ( m_NodeData.nHintType || m_NodeData.strGroup != NULL_STRING || m_NodeData.strEntityName != NULL_STRING )
		{
			m_NodeData.nNodeID = m_nNodeCount;
			pHint = CAI_HintManager::CreateHint( &m_NodeData, pMapData );
			pHint->AddSpawnFlags( GetSpawnFlags() );
		}
	}


	// ---------------------------------------------------------------------------------
	//  If we loaded from disk, we can discard all these node ents as soon as they spawn
	//  unless we are in WC edited mode
	// ---------------------------------------------------------------------------------
	if ( g_pAINetworkManager->NetworksLoaded() && !engine->IsInEditMode())
	{
		// If hint exists for this node, set it
		if (pHint)
		{
			CAI_Node *pNode = g_pBigAINet->GetNode(m_nNodeCount);
			if (pNode)
				pNode->SetHint( pHint );
			else
			{
				DevMsg("AI node graph corrupt\n");
			}
		}
		m_nNodeCount++;
		UTIL_RemoveImmediate( this );
		return -1;
	}	
	else
	{
		m_nNodeCount++;
	}

	// ---------------------------------------------------------------------------------
	//	Add a new node to the network
	// ---------------------------------------------------------------------------------
	// For now just using one big AI network
	CAI_Node *new_node = g_pBigAINet->AddNode( GetAbsOrigin(), GetAbsAngles().y );
	new_node->SetHint( pHint );

	// -------------------------------------------------------------------------
	//  Update table of how each WC id relates to each engine ID	
	// -------------------------------------------------------------------------
	if (g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable)
	{
		g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[new_node->GetId()]	= m_NodeData.nWCNodeID;
	}
	// Keep track of largest index used by WC
	if (g_pAINetworkManager->GetEditOps()->m_nNextWCIndex <= m_NodeData.nWCNodeID)
	{
		g_pAINetworkManager->GetEditOps()->m_nNextWCIndex = m_NodeData.nWCNodeID+1;
	}

	// -------------------------------------------------------------------------
	// If in WC edit mode:
	// 	Remember the original positions of the nodes before
	//	they drop so we can send the undropped positions to wc.
	// -------------------------------------------------------------------------
	if (engine->IsInEditMode())
	{
		if (g_pAINetworkManager->GetEditOps()->m_pWCPosition)
		{
			g_pAINetworkManager->GetEditOps()->m_pWCPosition[new_node->GetId()]		= new_node->GetOrigin();
		}
	}
	
	if (FClassnameIs( this, "info_node_air" ) || FClassnameIs( this, "info_node_air_hint" ))
	{
		new_node->SetType( NODE_AIR );
	}
	else if (FClassnameIs( this, "info_node_climb" ))
	{
		new_node->SetType( NODE_CLIMB );
	}
	else
	{
		new_node->SetType( NODE_GROUND );
	}

	new_node->m_eNodeInfo = ( m_spawnflags << NODE_ENT_FLAGS_SHIFT );

	// If changed as part of WC editing process note that network must be rebuilt
	if (m_debugOverlays & OVERLAY_WC_CHANGE_ENTITY)
	{
		g_pAINetworkManager->GetEditOps()->SetRebuildFlags();
		new_node->m_eNodeInfo			|= bits_NODE_WC_CHANGED;

		// Initialize the new nodes position.  The graph may not be rebuild
		// right away but the node should at least be positioned correctly
		g_AINetworkBuilder.InitNodePosition( g_pBigAINet, new_node );
	}

	UTIL_RemoveImmediate( this );

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CNodeEnt::CNodeEnt( void ) 
{
	m_debugOverlays = 0;
}



