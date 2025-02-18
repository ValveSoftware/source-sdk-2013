//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "fmtstr.h"
#include "filesystem.h"
#include "filesystem/IQueuedLoader.h"
#include "utlbuffer.h"
#include "utlrbtree.h"
#include "editor_sendcommand.h"

#include "ai_networkmanager.h"
#include "ai_network.h"
#include "ai_node.h"
#include "ai_navigator.h"
#include "ai_link.h"
#include "ai_dynamiclink.h"
#include "ai_initutils.h"
#include "ai_moveprobe.h"
#include "ai_hull.h"
#include "ndebugoverlay.h"
#include "ai_hint.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Increment this to force rebuilding of all networks
#define	 AINET_VERSION_NUMBER	37

//-----------------------------------------------------------------------------

int g_DebugConnectNode1 = -1;
int g_DebugConnectNode2 = -1;
#define DebuggingConnect( node1, node2 ) ( ( node1 == g_DebugConnectNode1 && node2 == g_DebugConnectNode2 ) || ( node1 == g_DebugConnectNode2 && node2 == g_DebugConnectNode1 ) )

inline void DebugConnectMsg( int node1, int node2, const char *pszFormat, ... )
{
	if ( DebuggingConnect( node1, node2 ) )
	{
		char string[ 2048 ];
		va_list argptr;
		va_start( argptr, pszFormat );
		Q_vsnprintf( string, sizeof(string), pszFormat, argptr );
		va_end( argptr );

		DevMsg( "%s", string );
	}
}

CON_COMMAND( ai_debug_node_connect, "Debug the attempted connection between two nodes" )
{
	g_DebugConnectNode1 = atoi( args[1] );
	g_DebugConnectNode2 = atoi( args[2] );
	
	DevMsg( "ai_debug_node_connect: debugging enbabled for %d <--> %d\n", g_DebugConnectNode1, g_DebugConnectNode2 );
}

//-----------------------------------------------------------------------------
// This CVAR allows level designers to override the building
// of node graphs due to date conflicts with the BSP and AIN 
// files. That way they don't have to wait for the node graph
// to rebuild following small only-ents changes. This CVAR
// always defaults to 0 and must be set at the command
// line to properly override the node graph building.

ConVar g_ai_norebuildgraph( "ai_norebuildgraph", "0" );


//-----------------------------------------------------------------------------
// CAI_NetworkManager
//
//-----------------------------------------------------------------------------

CAI_NetworkManager *g_pAINetworkManager;			

//-----------------------------------------------------------------------------

bool CAI_NetworkManager::gm_fNetworksLoaded;

LINK_ENTITY_TO_CLASS(ai_network,CAI_NetworkManager);

BEGIN_DATADESC( CAI_NetworkManager )

	DEFINE_FIELD( m_bNeedGraphRebuild, FIELD_BOOLEAN ),
	//									m_pEditOps
	//									m_pNetwork
	// DEFINE_FIELD( m_bDontSaveGraph, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_fInitalized, FIELD_BOOLEAN ),

	// Function pointers
	DEFINE_FUNCTION( DelayedInit ),
	DEFINE_FUNCTION( RebuildThink ),

END_DATADESC()


//-----------------------------------------------------------------------------

CAI_NetworkManager::CAI_NetworkManager(void)
{
	m_pNetwork = new CAI_Network;
	m_pEditOps = new CAI_NetworkEditTools(this);
	m_bNeedGraphRebuild		= false;
	m_fInitalized = false;
	CAI_DynamicLink::gm_bInitialized = false;

	// ---------------------------------
	//  Add to linked list of networks
	// ---------------------------------
};
         
//-----------------------------------------------------------------------------

CAI_NetworkManager::~CAI_NetworkManager(void)
{
	// ---------------------------------------
	//  Remove from linked list of AINetworks
	// ---------------------------------------
	delete m_pEditOps;
	delete m_pNetwork;
	if ( g_pAINetworkManager == this )
	{
		g_pAINetworkManager = NULL;
	}
}


//------------------------------------------------------------------------------
// Purpose : Think function so we can put message on screen saying we are
//			 going to rebuild the network, before we hang during the rebuild
//------------------------------------------------------------------------------

void CAI_NetworkManager::RebuildThink( void )
{
	SetThink(NULL);

	GetEditOps()->m_debugNetOverlays &= ~bits_debugNeedRebuild;
	StartRebuild( );
}

//------------------------------------------------------------------------------
// Purpose : Delay function so we can put message on screen saying we are
//			 going to rebuild the network, before we hang during the rebuild
//------------------------------------------------------------------------------

void CAI_NetworkManager::RebuildNetworkGraph( void )
{
	if (m_pfnThink != (void (CBaseEntity::*)())&CAI_NetworkManager::RebuildThink)
	{
		UTIL_CenterPrintAll( "Doing partial rebuild of Node Graph...\n" );
		SetThink(&CAI_NetworkManager::RebuildThink);
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose:  Used for WC edit move to rebuild the network around the given
//			 location.  Rebuilding the entire network takes too long
//-----------------------------------------------------------------------------

void CAI_NetworkManager::StartRebuild( void )
{
	CAI_DynamicLink::gm_bInitialized = false;

	g_AINetworkBuilder.Rebuild( m_pNetwork );

	// ------------------------------------------------------------
	// Purge any dynamic links for links that don't exist any more
	// ------------------------------------------------------------
	CAI_DynamicLink::PurgeDynamicLinks();


	// ------------------------
	// Reset all dynamic links
	// ------------------------
	CAI_DynamicLink::ResetDynamicLinks();

	// --------------------------------------------------
	//  Update display of usable nodes for displayed hull
	// --------------------------------------------------
	GetEditOps()->RecalcUsableNodesForHull();

	GetEditOps()->ClearRebuildFlags();
}


//-----------------------------------------------------------------------------
// Purpose: Called by save restore code if no valid load graph was loaded at restore time.
//  Prevents writing out of a "bogus" node graph...
// Input  :  - 
//-----------------------------------------------------------------------------
void CAI_NetworkManager::MarkDontSaveGraph()
{
	m_bDontSaveGraph = true;
}

//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
// Input  :
// Output :
//-----------------------------------------------------------------------------

void CAI_NetworkManager::SaveNetworkGraph( void )
{
	if ( m_bDontSaveGraph )
		return;

	if ( !m_bNeedGraphRebuild )
		return;

	//if ( g_AI_Manager.NumAIs() && m_pNetwork->m_iNumNodes == 0 )
	//{
	//	return;
	//}

	if ( !g_pGameRules->FAllowNPCs() )
	{
		return;
	}

	// -----------------------------
	// Make sure directories have been made
	// -----------------------------
	char	szNrpFilename [MAX_PATH];// text node report filename
	Q_strncpy( szNrpFilename, "maps/graphs" ,sizeof(szNrpFilename));
	
	// Usually adding on the map filename and stripping it does nothing, but if the map is under a subdir,
	// this makes it create the correct subdir under maps/graphs.
	char tempFilename[MAX_PATH];
	Q_snprintf( tempFilename, sizeof( tempFilename ), "%s/%s", szNrpFilename, STRING( gpGlobals->mapname ) );
	
	// Remove the filename.
	int len = strlen( tempFilename );
	for ( int i=0; i < len; i++ )
	{
		if ( tempFilename[len-i-1] == '/' || tempFilename[len-i-1] == '\\' )
		{
			tempFilename[len-i-1] = 0;
			break;
		}
	}
	
	// Make sure the directories we need exist.
	filesystem->CreateDirHierarchy( tempFilename, "DEFAULT_WRITE_PATH" );

	// Now add the real map filename.
	Q_strncat( szNrpFilename, "/", sizeof( szNrpFilename ), COPY_ALL_CHARACTERS  );
	Q_strncat( szNrpFilename, STRING( gpGlobals->mapname ), sizeof( szNrpFilename ), COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, IsX360() ? ".360.ain" : ".ain", sizeof( szNrpFilename ), COPY_ALL_CHARACTERS  );

	CUtlBuffer buf;

	// ---------------------------
	// Save the version number
	// ---------------------------
	buf.PutInt(AINET_VERSION_NUMBER);
	buf.PutInt(gpGlobals->mapversion);

	// -------------------------------
	// Dump all the nodes to the file
	// -------------------------------
	buf.PutInt( m_pNetwork->m_iNumNodes);

	int node;
	int totalNumLinks = 0;
	for ( node = 0; node < m_pNetwork->m_iNumNodes; node++)
	{
		CAI_Node *pNode = m_pNetwork->GetNode(node);
		Assert( pNode->GetZone() != AI_NODE_ZONE_UNKNOWN );

		buf.PutFloat( pNode->GetOrigin().x );
		buf.PutFloat( pNode->GetOrigin().y );
		buf.PutFloat( pNode->GetOrigin().z );
		buf.PutFloat( pNode->GetYaw() );
		buf.Put( pNode->m_flVOffset, sizeof( pNode->m_flVOffset ) );
		buf.PutChar( pNode->GetType() );
		if ( IsX360() )
		{
			buf.SeekPut( CUtlBuffer::SEEK_CURRENT, 3 );
		}
		buf.PutUnsignedShort( pNode->m_eNodeInfo );
		buf.PutShort( pNode->GetZone() );

		for (int link = 0; link < pNode->NumLinks(); link++)
		{
			// Only dump if link source
			if (node == pNode->GetLinkByIndex(link)->m_iSrcID)
			{
				totalNumLinks++;
			}
		}
	}

	// -------------------------------
	// Dump all the links to the file
	// -------------------------------
	buf.PutInt( totalNumLinks );

	for (node = 0; node < m_pNetwork->m_iNumNodes; node++)
	{
		CAI_Node *pNode = m_pNetwork->GetNode(node);

		for (int link = 0; link < pNode->NumLinks(); link++)
		{
			// Only dump if link source
			CAI_Link *pLink = pNode->GetLinkByIndex(link);
			if (node == pLink->m_iSrcID)
			{
				buf.PutShort( pLink->m_iSrcID );
				buf.PutShort( pLink->m_iDestID );
				buf.Put( pLink->m_iAcceptedMoveTypes, sizeof( pLink->m_iAcceptedMoveTypes) );
			}
		}
	}

	// -------------------------------
	// Dump WC lookup table
	// -------------------------------
	CUtlMap<int, int> wcIDs;
	SetDefLessFunc(wcIDs);
	bool bCheckForProblems = false;
	for (node = 0; node < m_pNetwork->m_iNumNodes; node++)
	{
		int iPreviousNodeBinding = wcIDs.Find( GetEditOps()->m_pNodeIndexTable[node] );
		if ( iPreviousNodeBinding != wcIDs.InvalidIndex() )
		{
			if ( !bCheckForProblems )
			{
				DevWarning( "******* MAP CONTAINS DUPLICATE HAMMER NODE IDS! CHECK FOR PROBLEMS IN HAMMER TO CORRECT *******\n" );
				bCheckForProblems = true;
			}
			DevWarning( "   AI node %d is associated with Hammer node %d, but %d is already bound to node %d\n", node, GetEditOps()->m_pNodeIndexTable[node], GetEditOps()->m_pNodeIndexTable[node], wcIDs[iPreviousNodeBinding] );
		}
		else
		{
			wcIDs.Insert( GetEditOps()->m_pNodeIndexTable[node], node );
		}
		buf.PutInt( GetEditOps()->m_pNodeIndexTable[node] );
	}

	// -------------------------------
	// Write the file out
	// -------------------------------

	FileHandle_t fh = filesystem->Open( szNrpFilename, "wb" );
	if ( !fh )
	{
		DevWarning( 2, "Couldn't create %s!\n", szNrpFilename );
		return;
	}

	filesystem->Write( buf.Base(), buf.TellPut(), fh );
	filesystem->Close(fh);
}

/* Keep this around for debugging
//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkManager::SaveNetworkGraph( void )
{
	// -----------------------------
	// Make sure directories have been made
	// -----------------------------
	char	szNrpFilename [MAX_PATH];// text node report filename
	Q_strncpy( szNrpFilename, "maps" ,sizeof(szNrpFilename));
	filesystem->CreateDirHierarchy( szNrpFilename );
	Q_strncat( szNrpFilename, "/graphs", COPY_ALL_CHARACTERS );
	filesystem->CreateDirHierarchy( szNrpFilename );

	Q_strncat( szNrpFilename, "/", COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, STRING( gpGlobals->mapname ), COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, ".ain", COPY_ALL_CHARACTERS );

	FileHandle_t file = filesystem->Open ( szNrpFilename, "w+" );

	// -----------------------------
	// Make sure the file opened ok
	// -----------------------------
	if ( !file )
	{
		DevWarning( 2, "Couldn't create %s!\n", szNrpFilename );
		return;
	}

	// ---------------------------
	// Save the version number
	// ---------------------------
	filesystem->FPrintf(file,"Version	%4d\n",AINET_VERSION_NUMBER);

	// -------------------------------
	// Dump all the nodes to the file
	// -------------------------------
	filesystem->FPrintf ( file, "NumNodes:         %d\n", m_iNumNodes);
	int totalNumLinks = 0;
	for (int node = 0; node < m_iNumNodes; node++)
	{
		filesystem->FPrintf ( file, "Location      %4f,%4f,%4f\n",m_pAInode[node]->GetOrigin().x, m_pAInode[node]->GetOrigin().y, m_pAInode[node]->GetOrigin().z );
		for (int hull =0;hull<NUM_HULLS;hull++)
		{
			filesystem->FPrintf ( file, "Voffset	     %4f\n", m_pAInode[node]->m_flVOffset[hull]);
		}
		filesystem->FPrintf ( file, "HintType:     %4d\n", m_pAInode[node]->m_eHintType );
		filesystem->FPrintf ( file, "HintYaw:      %4f\n", m_pAInode[node]->GetYaw() );
		filesystem->FPrintf ( file, "NodeType      %4d\n",m_pAInode[node]->GetType());
		filesystem->FPrintf ( file, "NodeInfo      %4d\n",m_pAInode[node]->m_eNodeInfo);
		filesystem->FPrintf ( file, "Neighbors     ");
		m_pAInode[node]->m_pNeighborBS->SaveBitString(file);
		filesystem->FPrintf ( file, "Visible	   ");
		m_pAInode[node]->m_pVisibleBS->SaveBitString(file);
		filesystem->FPrintf ( file, "Connected     ");
		m_pAInode[node]->m_pConnectedBS->SaveBitString(file);

		filesystem->FPrintf ( file, "NumLinks      %4d\n",m_pAInode[node]->NumLinks());

		for (int link = 0; link < m_pAInode[node]->NumLinks(); link++)
		{
			// Only dump if link source
			if (node == m_pAInode[node]->GetLinkByIndex(link)->m_iSrcID)
			{
				totalNumLinks++;
			}
		}
	}

	// -------------------------------
	// Dump all the links to the file
	// -------------------------------
	filesystem->FPrintf ( file, "TotalNumLinks      %4d\n",totalNumLinks);

	for (node = 0; node < m_iNumNodes; node++)
	{
		for (int link = 0; link < m_pAInode[node]->NumLinks(); link++)
		{
			// Only dump if link source
			if (node == m_pAInode[node]->GetLinkByIndex(link)->m_iSrcID)
			{
				filesystem->FPrintf ( file, "LinkSrcID       %4d\n", m_pAInode[node]->GetLinkByIndex(link)->m_iSrcID);
				filesystem->FPrintf ( file, "LinkDestID      %4d\n", m_pAInode[node]->GetLinkByIndex(link)->m_iDestID);

				for (int hull =0;hull<NUM_HULLS;hull++)
				{
					filesystem->FPrintf ( file, "Hulls		     %4d\n", m_pAInode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[hull]);
				}
			}
		}
	}

	// -------------------------------
	// Dump WC lookup table
	// -------------------------------
	for (node = 0; node < m_iNumNodes; node++)
	{
		filesystem->FPrintf( file, "%4d\n",m_pNodeIndexTable[node]);
	}

	filesystem->Close(file);
}
*/

//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
//-----------------------------------------------------------------------------

void CAI_NetworkManager::LoadNetworkGraph( void )
{
	// ---------------------------------------------------
	// If I'm in edit mode don't load, always recalculate
	// ---------------------------------------------------
	DevMsg( "Loading AI graph\n" );
	if (engine->IsInEditMode())
	{
		DevMsg( "Not loading AI due to edit mode\n" );
		return;
	}

	if ( !g_pGameRules->FAllowNPCs() )
	{
		DevMsg( "Not loading AI due to games rules\n" );
		return;
	}

	DevMsg( "Step 1 loading\n" );

	// -----------------------------
	// Make sure directories have been made
	// -----------------------------
	char szNrpFilename[MAX_PATH];// text node report filename
	Q_strncpy( szNrpFilename, "maps" ,sizeof(szNrpFilename));
	filesystem->CreateDirHierarchy( szNrpFilename, "DEFAULT_WRITE_PATH" );
	Q_strncat( szNrpFilename, "/graphs", sizeof( szNrpFilename ), COPY_ALL_CHARACTERS );
	filesystem->CreateDirHierarchy( szNrpFilename, "DEFAULT_WRITE_PATH" );

	Q_strncat( szNrpFilename, "/", sizeof( szNrpFilename ), COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, STRING( gpGlobals->mapname ), sizeof( szNrpFilename ), COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, IsX360() ? ".360.ain" : ".ain", sizeof( szNrpFilename ), COPY_ALL_CHARACTERS );

	MEM_ALLOC_CREDIT();

	// Read the file in one gulp
	CUtlBuffer buf;
	bool bHaveAIN = false;
	if ( IsX360() && g_pQueuedLoader->IsMapLoading() )
	{
		// .ain was loaded anonymously by bsp, should be ready
		void *pData;
		int nDataSize;
		if ( g_pQueuedLoader->ClaimAnonymousJob( szNrpFilename, &pData, &nDataSize ) )
		{
			if ( nDataSize != 0 )
			{
				buf.Put( pData, nDataSize );
				bHaveAIN = true;
			}
			filesystem->FreeOptimalReadBuffer( pData );
		}
	}
	


	if ( !bHaveAIN && !filesystem->ReadFile( szNrpFilename, "game", buf ) )
	{
		DevWarning( 2, "Couldn't read %s!\n", szNrpFilename );
		return;
	}

	DevMsg( "Checking version\n" );

	// ---------------------------
	// Check the version number
	// ---------------------------
	if ( buf.GetChar() == 'V' && buf.GetChar() == 'e' && buf.GetChar() == 'r' )
	{
		DevMsg( "AI node graph %s is out of date\n", szNrpFilename );
		return;
	}
	
	DevMsg( "Passed first ver check\n" );


	buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

	int version = buf.GetInt();
	DevMsg( "Got version %d\n", version );

	if ( version != AINET_VERSION_NUMBER)
	{
		DevMsg( "AI node graph %s is out of date\n", szNrpFilename );
		return;
	}

	int mapversion = buf.GetInt();
	DevMsg( "Map version %d\n", mapversion );

	if ( mapversion != gpGlobals->mapversion && !g_ai_norebuildgraph.GetBool() )
	{
		bool bOK = false;
		
		const char *pGameDir = CommandLine()->ParmValue( "-game", "hl2" );		
		char szLoweredGameDir[256];
		Q_strncpy( szLoweredGameDir, pGameDir, sizeof( szLoweredGameDir ) );
		Q_strlower( szLoweredGameDir );

		// hack for shipped ep1 and hl2 maps
		// they were rebuilt a week after they were actually shipped so allow the slightly
		// older node graphs to load for these maps
		if ( !V_stricmp( szLoweredGameDir, "hl2" ) || !V_stricmp( szLoweredGameDir, "episodic" ) )
		{
			bOK = true;
		}
		
		if ( !bOK )
		{
			DevMsg( "AI node graph %s is out of date (map version changed)\n", szNrpFilename );
			return;
		}
	}

	DevMsg( "Done version checks\n" );

	// ----------------------------------------
	// Get the network size and allocate space
	// ----------------------------------------
	int numNodes = buf.GetInt();

	if ( numNodes > MAX_NODES || numNodes < 0 )
	{
		Error( "AI node graph %s is corrupt\n", szNrpFilename );
		DevMsg( "%s", (const char *)buf.Base() );
		DevMsg( "\n" );
		Assert( 0 );
		return;
	}
	
	DevMsg( "Finishing load\n" );


	// ------------------------------------------------------------------------
	// If in wc_edit mode allocate extra space for nodes that might be created
	// ------------------------------------------------------------------------
	if ( engine->IsInEditMode() )
	{
		numNodes = MAX( numNodes, 1024 );
	}

	m_pNetwork->m_pAInode = new CAI_Node*[MAX( numNodes, 1 )];
	memset( m_pNetwork->m_pAInode, 0, sizeof( CAI_Node* ) * MAX( numNodes, 1 ) );

	// -------------------------------
	// Load all the nodes to the file
	// -------------------------------
	int node;
	for ( node = 0; node < numNodes; node++)
	{
		Vector origin;
		float yaw;
		origin.x = buf.GetFloat();
		origin.y = buf.GetFloat();
		origin.z = buf.GetFloat();
		yaw = buf.GetFloat();

		CAI_Node *new_node = m_pNetwork->AddNode( origin, yaw );

		buf.Get( new_node->m_flVOffset, sizeof(new_node->m_flVOffset) );
		new_node->m_eNodeType = (NodeType_e)buf.GetChar();
		if ( IsX360() )
		{
			buf.SeekGet( CUtlBuffer::SEEK_CURRENT, 3 );
		}

		new_node->m_eNodeInfo = buf.GetUnsignedShort();
		new_node->m_zone = buf.GetShort();
	}

	// -------------------------------
	// Load all the links to the fild
	// -------------------------------
	int totalNumLinks = buf.GetInt();

	for (int link = 0; link < totalNumLinks; link++)
	{
		int srcID, destID;

		srcID = buf.GetShort();
		destID = buf.GetShort();

		CAI_Link *pLink = m_pNetwork->CreateLink( srcID, destID );;

		byte ignored[NUM_HULLS];
		byte *pDest = ( pLink ) ? &pLink->m_iAcceptedMoveTypes[0] : &ignored[0];
		buf.Get( pDest, sizeof(ignored) );
	}

	// -------------------------------
	// Load WC lookup table
	// -------------------------------
	delete [] GetEditOps()->m_pNodeIndexTable;
	GetEditOps()->m_pNodeIndexTable	= new int[MAX( m_pNetwork->m_iNumNodes, 1 )];
	memset( GetEditOps()->m_pNodeIndexTable, 0, sizeof( int ) *MAX( m_pNetwork->m_iNumNodes, 1 ) );

	for (node = 0; node < m_pNetwork->m_iNumNodes; node++)
	{
		GetEditOps()->m_pNodeIndexTable[node] = buf.GetInt();
	}

	
#if 1
	CUtlRBTree<int> usedIds;
	CUtlRBTree<int> reportedIds;
	SetDefLessFunc( usedIds );
	SetDefLessFunc( reportedIds );

	bool printedHeader = false;
	
	for (node = 0; node < m_pNetwork->m_iNumNodes; node++)
	{
		int editorId = GetEditOps()->m_pNodeIndexTable[node];
		if ( editorId != NO_NODE )
		{
			if ( usedIds.Find( editorId ) != usedIds.InvalidIndex() )
			{
				if ( !printedHeader )
				{
					Warning( "** Duplicate Hammer Node IDs: " );
					printedHeader = true;
				}

				if ( reportedIds.Find( editorId ) == reportedIds.InvalidIndex() )
				{
					DevMsg( "%d, ", editorId );
					reportedIds.Insert( editorId );
				}
			}
			else
				usedIds.Insert( editorId );
		}
	}

	if ( printedHeader )
		DevMsg( "\n** Should run \"Check For Problems\" on the VMF then verify dynamic links\n" );
#endif

	gm_fNetworksLoaded = true;
	CAI_DynamicLink::gm_bInitialized = false;
}

/* Keep this around for debugging
//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkManager::LoadNetworkGraph( void )
{
	// ---------------------------------------------------
	// If I'm in edit mode don't load, always recalculate
	// ---------------------------------------------------
	if (engine->IsInEditMode())
	{
		return;
	}

	// -----------------------------
	// Make sure directories have been made
	// -----------------------------
	char	szNrpFilename [MAX_PATH];// text node report filename
	Q_strncpy( szNrpFilename, "maps" ,sizeof(szNrpFilename));
	filesystem->CreateDirHierarchy( szNrpFilename );
	Q_strncat( szNrpFilename, "/graphs", COPY_ALL_CHARACTERS );
	filesystem->CreateDirHierarchy( szNrpFilename );

	Q_strncat( szNrpFilename, "/", COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, STRING( gpGlobals->mapname ), COPY_ALL_CHARACTERS );
	Q_strncat( szNrpFilename, ".ain", COPY_ALL_CHARACTERS );

	FileHandle_t file = filesystem->Open ( szNrpFilename, "r" );

	// -----------------------------
	// Make sure the file opened ok
	// -----------------------------
	if ( !file )
	{
		DevWarning( 2, "Couldn't create %s!\n", szNrpFilename );
		return;
	}

	// ---------------------------
	// Check the version number
	// ---------------------------
	char temps[256];
	int version;
	fscanf(file,"%255s",&temps);
	fscanf(file, "%i\n",&version);
	if (version!=AINET_VERSION_NUMBER)
	{
		return;
	}

	// ----------------------------------------
	// Get the network size and allocate space
	// ----------------------------------------
	int numNodes;
	fscanf(file,"%255s",&temps);
	fscanf ( file, "%d\n", &numNodes);

	// ------------------------------------------------------------------------
	// If in wc_edit mode allocate extra space for nodes that might be created
	// ------------------------------------------------------------------------
	if ( engine->IsInEditMode() )
	{
		numNodes = MAX( numNodes, 1024 );
	}

	m_pAInode = new CAI_Node*[numNodes];
	if ( !m_pAInode )
	{
		Warning( "LoadNetworkGraph:  Not enough memory to create %i nodes\n", numNodes );
		Assert(0);
		return;
	}

	// -------------------------------
	// Load all the nodes to the file
	// -------------------------------
	for (int node = 0; node < numNodes; node++)
	{
		CAI_Node *new_node = AddNode();

		Vector origin;
		fscanf(file,"%255s",&temps);
		fscanf(file, "%f,%f,%f\n", &new_node->GetOrigin().x, &new_node->GetOrigin().y, &new_node->GetOrigin().z );
		for (int hull =0;hull<NUM_HULLS;hull++)
		{
			fscanf(file,"%255s",&temps);
			fscanf(file, "%f\n", &new_node->m_flVOffset[hull]);
		}
		fscanf(file,"%255s",&temps);
		fscanf(file, "%d\n", &new_node->m_eHintType );
		fscanf(file,"%255s",&temps);
		fscanf(file, "%f\n", &new_node->GetYaw() );
		fscanf(file,"%255s",&temps);
		fscanf(file, "%d\n",&new_node->GetType());
		fscanf(file,"%255s",&temps);
		fscanf(file, "%d\n",&new_node->m_eNodeInfo);

		fscanf(file,"%255s",&temps);
		new_node->m_pNeighborBS = new CVarBitVec(numNodes);
		new_node->m_pNeighborBS->LoadBitString(file);

		fscanf(file,"%255s",&temps);
		new_node->m_pVisibleBS = new CVarBitVec(numNodes);
		new_node->m_pVisibleBS->LoadBitString(file);

		fscanf(file,"%255s",&temps);
		new_node->m_pConnectedBS = new CVarBitVec(numNodes);
		new_node->m_pConnectedBS->LoadBitString(file);

		fscanf(file,"%255s",&temps);
		int numLinks;
		fscanf (file, "%4d",&numLinks);

		// ------------------------------------------------------------------------
		// If in wc_edit mode allocate extra space for nodes that might be created
		// ------------------------------------------------------------------------
		if ( engine->IsInEditMode() )
		{
			numLinks = AI_MAX_NODE_LINKS;
		}

		//Assert ( numLinks >= 1 );
		new_node->AllocateLinkSpace( numLinks );
	}

	// -------------------------------
	// Load all the links to the fild
	// -------------------------------
	int totalNumLinks;
	fscanf(file,"%255s",&temps);
	fscanf ( file, "%d\n",&totalNumLinks);

	for (int link = 0; link < totalNumLinks; link++)
	{
		CAI_Link *new_link = new CAI_Link;

		fscanf(file,"%255s",&temps);
		fscanf ( file, "%4d\n", &new_link->m_iSrcID);

		fscanf(file,"%255s",&temps);
		fscanf ( file, "%4d\n", &new_link->m_iDestID);

		for (int hull =0;hull<NUM_HULLS;hull++)
		{
			fscanf(file,"%255s",&temps);
			fscanf ( file, "%d\n", &new_link->m_iAcceptedMoveTypes[hull]);
		}
		// Now add link to source and destination nodes
		m_pAInode[new_link->m_iSrcID]->AddLink(new_link);
		m_pAInode[new_link->m_iDestID]->AddLink(new_link);
	}

	// -------------------------------
	// Load WC lookup table
	// -------------------------------
	m_pNodeIndexTable	= new int[m_iNumNodes];

	for (node = 0; node < m_iNumNodes; node++)
	{
		fscanf( file, "%d\n",&m_pNodeIndexTable[node]);
	}

	CAI_NetworkManager::NetworksLoaded() = true;
	fclose(file);
}
*/

//-----------------------------------------------------------------------------
// Purpose:  Deletes all AINetworks from memory
//-----------------------------------------------------------------------------

void CAI_NetworkManager::DeleteAllAINetworks(void) 
{
	CAI_DynamicLink::gm_bInitialized = false;
	gm_fNetworksLoaded = false;
	g_pBigAINet = NULL;
}


//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
//-----------------------------------------------------------------------------

void CAI_NetworkManager::BuildNetworkGraph( void )
{
	if ( m_bDontSaveGraph )
		return;

	CAI_DynamicLink::gm_bInitialized = false;
	g_AINetworkBuilder.Build( m_pNetwork );

	// If I'm loading for the first time save.  Otherwise I'm 
	// doing a wc edit and I don't want to save
	if (!CAI_NetworkManager::NetworksLoaded())
	{
		SaveNetworkGraph();	

		gm_fNetworksLoaded = true;
	}
}

//------------------------------------------------------------------------------
bool g_bAIDisabledByUser = false;

void CAI_NetworkManager::InitializeAINetworks()
{
	// For not just create a single AI Network called "BigNet"
	// At some later point we may have mulitple AI networks
	CAI_NetworkManager *pNetwork;
	g_pAINetworkManager = pNetwork = CREATE_ENTITY( CAI_NetworkManager, "ai_network" );
	pNetwork->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
	g_pBigAINet = pNetwork->GetNetwork();
	pNetwork->SetName( AllocPooledString("BigNet") );
	pNetwork->Spawn();
	if ( engine->IsInEditMode() )
	{
		g_ai_norebuildgraph.SetValue( 0 );
	}
	if ( CAI_NetworkManager::IsAIFileCurrent( STRING( gpGlobals->mapname ) ) )
	{
		pNetwork->LoadNetworkGraph(); 
		if ( !g_bAIDisabledByUser )
		{
			CAI_BaseNPC::m_nDebugBits &= ~bits_debugDisableAI;
		}
	}

	// Reset node counter used during load
	CNodeEnt::m_nNodeCount = 0;

	pNetwork->SetThink( &CAI_NetworkManager::DelayedInit );
	pNetwork->SetNextThink( gpGlobals->curtime );
}

// UNDONE: Where should this be defined?
#ifndef MAX_PATH
#define MAX_PATH	256
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns true if the AINetwork data files are up to date
//-----------------------------------------------------------------------------

bool CAI_NetworkManager::IsAIFileCurrent ( const char *szMapName )
{
	char		szBspFilename[MAX_PATH];
	char		szGraphFilename[MAX_PATH];

	if ( !g_pGameRules->FAllowNPCs() )
	{
		return false;
	}

	if ( IsX360() && ( filesystem->GetDVDMode() == DVDMODE_STRICT ) )
	{
		// dvd build process validates and guarantees correctness, timestamps are allowed to be wrong
		return true;
	}
	
	{
		const char *pGameDir = CommandLine()->ParmValue( "-game", "hl2" );		
		char szLoweredGameDir[256];
		Q_strncpy( szLoweredGameDir, pGameDir, sizeof( szLoweredGameDir ) );
		Q_strlower( szLoweredGameDir );
		
		if ( !V_stricmp( szLoweredGameDir, "hl2" ) || !V_stricmp( szLoweredGameDir, "episodic" ) || !V_stricmp( szLoweredGameDir, "ep2" ) || !V_stricmp( szLoweredGameDir, "portal" ) || !V_stricmp( szLoweredGameDir, "lostcoast" )  || !V_stricmp( szLoweredGameDir, "hl1" ) )
		{
			// we shipped good node graphs for our games
			return true;
		}
	}
	
	Q_snprintf( szBspFilename, sizeof( szBspFilename ), "maps/%s%s.bsp" ,szMapName, GetPlatformExt() );
	Q_snprintf( szGraphFilename, sizeof( szGraphFilename ), "maps/graphs/%s%s.ain", szMapName, GetPlatformExt() );
	
	int iCompare;
	if ( engine->CompareFileTime( szBspFilename, szGraphFilename, &iCompare ) )
	{
		if ( iCompare > 0 )
		{
			// BSP file is newer.
			if ( g_ai_norebuildgraph.GetInt() )
			{
				// The user has specified that they wish to override the 
				// rebuilding of outdated nodegraphs (see top of this file)
				if ( filesystem->FileExists( szGraphFilename ) )
				{
					// Display these messages only if the graph exists, and the 
					// user is asking to override the rebuilding. If the graph does
					// not exist, we're going to build it whether the user wants to or 
					// not. 
					DevMsg( 2, ".AIN File will *NOT* be updated. User Override.\n\n" );
					DevMsg( "\n*****Node Graph Rebuild OVERRIDDEN by user*****\n\n" );
				}
				return true;
			}
			else
			{
				// Graph is out of date. Rebuild at usual.
				DevMsg( 2, ".AIN File will be updated\n\n" );
				return false;
			}
		}
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------

void CAI_NetworkManager::Spawn ( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

//------------------------------------------------------------------------------

void CAI_NetworkManager::DelayedInit( void )
{
	if ( !g_pGameRules->FAllowNPCs() )
	{
		SetThink ( NULL );
		return;
	}

	if ( !g_ai_norebuildgraph.GetInt() )
	{
		// ----------------------------------------------------------
		//  Actually enter DelayedInit twice when rebuilding the 
		//  node graph.  The first time through we just print the
		//  warning message.  We only actually do the rebuild on
		//  the second pass to make sure the message hits the screen
		// ----------------------------------------------------------
		if (m_bNeedGraphRebuild)
		{
			Assert( !m_bDontSaveGraph );

			BuildNetworkGraph();	// For now only one AI Network

			if (engine->IsInEditMode())
			{
				engine->ServerCommand("exec map_edit.cfg\n");
			}

			SetThink ( NULL );
			if ( !g_bAIDisabledByUser )
			{
				CAI_BaseNPC::m_nDebugBits &= ~bits_debugDisableAI;
			}
		}


		// --------------------------------------------
		// If I haven't loaded a network, or I'm in 
		// WorldCraft edit mode rebuild the network
		// --------------------------------------------
		else if ( !m_bDontSaveGraph && ( !CAI_NetworkManager::NetworksLoaded() || engine->IsInEditMode() ) )
		{
#ifdef _WIN32
			// --------------------------------------------------------
			// If in edit mode start WC session and make sure we are
			// running the same map in WC and the engine
			// --------------------------------------------------------
			if (engine->IsInEditMode())
			{
				int status = Editor_BeginSession(STRING(gpGlobals->mapname), gpGlobals->mapversion, false);
				if (status == Editor_NotRunning)
				{
					DevMsg("\nAborting map_edit\nWorldcraft not running...\n\n");
					UTIL_CenterPrintAll( "Worldcraft not running...\n" );
					engine->ServerCommand("disconnect\n");
					SetThink(NULL);
					return;
				}
				else if (status == Editor_BadCommand)
				{
					DevMsg("\nAborting map_edit\nWC/Engine map versions different...\n\n");
					UTIL_CenterPrintAll( "WC/Engine map versions different...\n" );
					engine->ServerCommand("disconnect\n");
					SetThink(NULL);
					return;
				}
				else
				{
					// Increment version number when session begins
					gpGlobals->mapversion++;
				}
			}
#endif

			DevMsg( "Node Graph out of Date. Rebuilding... (%d, %d, %d)\n", (int)m_bDontSaveGraph, (int)!CAI_NetworkManager::NetworksLoaded(), (int) engine->IsInEditMode() );
			UTIL_CenterPrintAll( "Node Graph out of Date. Rebuilding...\n" );
			m_bNeedGraphRebuild = true;
			g_pAINetworkManager->SetNextThink( gpGlobals->curtime + 1 );
			return;
		}	

	}

	// --------------------------------------------
	// Initialize any dynamic links
	// --------------------------------------------
	CAI_DynamicLink::InitDynamicLinks();
	FixupHints();
	
	GetEditOps()->OnInit();

	m_fInitalized = true;

	if ( g_AI_Manager.NumAIs() != 0 && g_pBigAINet->NumNodes() == 0 )
		DevMsg( "WARNING: Level contains NPCs but has no path nodes\n" );
}

//------------------------------------------------------------------------------

void CAI_NetworkManager::FixupHints()
{
	AIHintIter_t iter;
	CAI_Hint *pHint = CAI_HintManager::GetFirstHint( &iter );
	while ( pHint )
	{
		pHint->FixupTargetNode();
		pHint = CAI_HintManager::GetNextHint( &iter );
	}
}

//-----------------------------------------------------------------------------
// CAI_NetworkEditTools
//-----------------------------------------------------------------------------

CAI_Node*		CAI_NetworkEditTools::m_pLastDeletedNode		= NULL;						// For undo in wc edit mode
int				CAI_NetworkEditTools::m_iHullDrawNum			= HULL_HUMAN;				// Which hulls to draw
int				CAI_NetworkEditTools::m_iVisibilityNode		= NO_NODE;
int				CAI_NetworkEditTools::m_iGConnectivityNode	= NO_NODE;
bool			CAI_NetworkEditTools::m_bAirEditMode			= false;
bool			CAI_NetworkEditTools::m_bLinkEditMode		= false;
float			CAI_NetworkEditTools::m_flAirEditDistance	= 300;

#ifdef AI_PERF_MON
	// Performance stats (only for development)
	int				CAI_NetworkEditTools::m_nPerfStatNN			= 0;
	int				CAI_NetworkEditTools::m_nPerfStatPB			= 0;
	float			CAI_NetworkEditTools::m_fNextPerfStatTime	= -1;
#endif

//------------------------------------------------------------------------------

void CAI_NetworkEditTools::OnInit()
{
	// --------------------------------------------
	// If I'm not in edit mode delete WC ID table
	// --------------------------------------------
	if ( !engine->IsInEditMode() )
	{
//		delete[] m_pNodeIndexTable;	// For now only one AI Network called "BigNet"
//		m_pNodeIndexTable = NULL;
	}
}

//------------------------------------------------------------------------------
// Purpose : Given a WorldCraft node ID, return the associated engine ID
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CAI_NetworkEditTools::GetNodeIdFromWCId( int nWCId )
{
	if ( nWCId == -1 )
		return -1;

	if (!m_pNodeIndexTable)
	{
		DevMsg("ERROR: Trying to get WC ID with no table!\n");
		return -1;
	}

	if (!m_pNetwork->NumNodes())
	{
		DevMsg("ERROR: Trying to get WC ID with no network!\n");
		return -1;
	}

	for (int i=0;i<m_pNetwork->NumNodes();i++)
	{
		if (m_pNodeIndexTable[i] == nWCId)
		{
			return i;
		}
	}
	return -1;
}

//-----------------------------------------------------------------------------

int CAI_NetworkEditTools::GetWCIdFromNodeId( int nNodeId )
{
	if ( nNodeId == -1 || nNodeId >= m_pNetwork->NumNodes() )
		return -1;
	return m_pNodeIndexTable[nNodeId];
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest ainode that is faced from the given location
//			and within the angular threshold (ignores worldspawn).
//			DO NOT USE FOR ANY RUN TIME RELEASE CODE
//			Used for selection of nodes in debugging display only!
//			
//-----------------------------------------------------------------------------

CAI_Node *CAI_NetworkEditTools::FindAINodeNearestFacing( const Vector &origin, const Vector &facing, float threshold, int nNodeType)
{
	float bestDot  = threshold;
	CAI_Node *best = NULL;

	CAI_Network* aiNet = g_pBigAINet;

	for (int node =0; node < aiNet->NumNodes();node++)
	{
		if (aiNet->GetNode(node)->GetType() != NODE_DELETED)
		{
			// Pick nodes that are in the current editing type
			if ( nNodeType	== NODE_ANY								|| 
				 nNodeType	== aiNet->GetNode(node)->GetType()  )
			{
				// Make vector to node
				Vector	to_node = (aiNet->GetNode(node)->GetPosition(m_iHullDrawNum) - origin);

				VectorNormalize( to_node );
				float	dot = DotProduct (facing , to_node );
				if (dot > bestDot)
				{
					// Make sure I have a line of sight to it
					trace_t tr;
					AI_TraceLine ( origin, aiNet->GetNode(node)->GetPosition(m_iHullDrawNum), 
						MASK_BLOCKLOS, NULL, COLLISION_GROUP_NONE, &tr );
					if ( tr.fraction == 1.0 )
					{
						bestDot	= dot;
						best	= aiNet->GetNode(node);
					}
				}
			}
		}
	}
	return best;
}


Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint)
{
	Vector	vEndToStart		= (vEndPos - vStartPos);
	Vector	vOrgToStart		= (vPoint  - vStartPos);
	float	fNumerator		= DotProduct(vEndToStart,vOrgToStart);
	float	fDenominator	= vEndToStart.Length() * vOrgToStart.Length();
	float	fIntersectDist	= vOrgToStart.Length()*(fNumerator/fDenominator);
	VectorNormalize( vEndToStart ); 
	Vector	vIntersectPos	= vStartPos + vEndToStart * fIntersectDist;

	return vIntersectPos;
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest ainode that is faced from the given location
//			and within the angular threshold (ignores worldspawn).
//			DO NOT USE FOR ANY RUN TIME RELEASE CODE
//			Used for selection of nodes in debugging display only!
//-----------------------------------------------------------------------------

CAI_Link *CAI_NetworkEditTools::FindAILinkNearestFacing( const Vector &vOrigin, const Vector &vFacing, float threshold)
{
	float bestDot  = threshold;
	CAI_Link *best = NULL;

	CAI_Network* aiNet = g_pBigAINet;

	for (int node =0; node < aiNet->NumNodes();node++)
	{
		if (aiNet->GetNode(node)->GetType() != NODE_DELETED)
		{
			// Pick nodes that are in the current editing type
			if (( m_bAirEditMode && aiNet->GetNode(node)->GetType() == NODE_AIR)	||
				(!m_bAirEditMode && aiNet->GetNode(node)->GetType() == NODE_GROUND))
			{
				// Go through each link
				for (int link=0; link < aiNet->GetNode(node)->NumLinks();link++) 
				{
					CAI_Link *nodeLink = aiNet->GetNode(node)->GetLinkByIndex(link);

					// Find position on link that I am looking
					int		endID		= nodeLink->DestNodeID(node);
					Vector  startPos	= aiNet->GetNode(node)->GetPosition(m_iHullDrawNum);
					Vector	endPos		= aiNet->GetNode(endID)->GetPosition(m_iHullDrawNum);
					Vector  vNearest	= PointOnLineNearestPoint(startPos, endPos, vOrigin);

					// Get angle between viewing dir. and nearest point on line
					Vector	vOriginToNearest = (vNearest - vOrigin);
					float	fNumerator		 = DotProduct(vOriginToNearest,vFacing);
					float	fDenominator	 = vOriginToNearest.Length();
					float	fAngleToNearest	 = acos(fNumerator/fDenominator);

					// If not facing the line reject
					if (fAngleToNearest > 1.57)
					{
						continue;
					}

					// Calculate intersection of facing direction to nearest point
					float	fIntersectDist	 = vOriginToNearest.Length() * tan(fAngleToNearest);
					Vector dir = endPos-startPos;
					float fLineLen = VectorNormalize( dir );
					Vector	vIntersection	 = vNearest + (fIntersectDist * dir);

					// Reject of beyond end of line
					if (((vIntersection - startPos).Length() > fLineLen) ||
						((vIntersection - endPos  ).Length() > fLineLen) )
					{
						continue;
					}

					// Now test dot to the look position
					Vector	toLink	= vIntersection - vOrigin;
					VectorNormalize(toLink);
					float	lookDot = DotProduct (vFacing , toLink);
					if (lookDot > bestDot)
					{
						// Make sure I have a line of sight to it
						trace_t tr;
						AI_TraceLine ( vOrigin, vIntersection, MASK_BLOCKLOS, NULL, COLLISION_GROUP_NONE, &tr );
						if ( tr.fraction == 1.0 )
						{
 							bestDot	= lookDot;
							best	= nodeLink;
						}
					}
				}
			}
		}
	}
	return best;
}


//-----------------------------------------------------------------------------
// Purpose:  Used for WC edit more.  Marks that the network should be 
//			 rebuild and turns of any displays that have been invalidated
//			 as the network is now out of date
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkEditTools::SetRebuildFlags( void )
{
	m_debugNetOverlays |= bits_debugNeedRebuild;
	m_debugNetOverlays &= ~bits_debugOverlayConnections;
	m_debugNetOverlays &= ~bits_debugOverlayGraphConnect;
	m_debugNetOverlays &= ~bits_debugOverlayVisibility;
	m_debugNetOverlays &= ~bits_debugOverlayHulls;

	// Not allowed to edit links when graph outdated
	m_bLinkEditMode = false;
}

//-----------------------------------------------------------------------------
// Purpose:  Used for WC edit more.  After node graph has been rebuild
//			 marks it as so and turns connection display back on
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkEditTools::ClearRebuildFlags( void )
{
	m_debugNetOverlays |=  bits_debugOverlayConnections;

	// ------------------------------------------
	//  Clear all rebuild flags in nodes
	// ------------------------------------------
	for (int i = 0; i < m_pNetwork->NumNodes(); i++)
	{
		m_pNetwork->GetNode(i)->m_eNodeInfo &= ~bits_NODE_WC_CHANGED;
		m_pNetwork->GetNode(i)->ClearNeedsRebuild();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the next hull to draw, or none if at end of hulls
//-----------------------------------------------------------------------------
void CAI_NetworkEditTools::DrawNextHull(const char *ainet_name) 
{
	m_iHullDrawNum++;
	if (m_iHullDrawNum == NUM_HULLS) 
	{
		m_iHullDrawNum = 0;
	}

	// Recalculate usable nodes for current hull
	g_pAINetworkManager->GetEditOps()->RecalcUsableNodesForHull();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_NetworkEditTools::DrawHull(Hull_t eHull) 
{
	m_iHullDrawNum = eHull;
	if (m_iHullDrawNum >= NUM_HULLS) 
	{
		m_iHullDrawNum = 0;
	}

	// Recalculate usable nodes for current hull
	g_pAINetworkManager->GetEditOps()->RecalcUsableNodesForHull();
}


//-----------------------------------------------------------------------------
// Purpose: Used just for debug display, to color nodes grey that the 
//			currently selected hull size can't use.
//-----------------------------------------------------------------------------

void CAI_NetworkEditTools::RecalcUsableNodesForHull(void) 
{
	// -----------------------------------------------------
	//  Use test hull to check hull sizes
	// -----------------------------------------------------
	CAI_TestHull *m_pTestHull = CAI_TestHull::GetTestHull();
	m_pTestHull->GetNavigator()->SetNetwork( g_pBigAINet );
	m_pTestHull->SetHullType((Hull_t)m_iHullDrawNum);
	m_pTestHull->SetHullSizeNormal();

	for (int node=0;node<m_pNetwork->NumNodes();node++) 
	{
		if ( ( m_pNetwork->GetNode(node)->m_eNodeInfo & ( HullToBit( (Hull_t)m_iHullDrawNum ) << NODE_ENT_FLAGS_SHIFT ) )  ||
			 m_pTestHull->GetNavigator()->CanFitAtNode(node))
		{
			m_pNetwork->GetNode(node)->m_eNodeInfo &= ~bits_NODE_WONT_FIT_HULL;
		}
		else
		{
			m_pNetwork->GetNode(node)->m_eNodeInfo |= bits_NODE_WONT_FIT_HULL;
		}
	}
	CAI_TestHull::ReturnTestHull();
}

//-----------------------------------------------------------------------------
// Purpose: Sets debug bits
//-----------------------------------------------------------------------------

void CAI_NetworkEditTools::SetDebugBits(const char *ainet_name,int debug_bit) 
{
	CAI_NetworkEditTools *pEditOps = g_pAINetworkManager->GetEditOps();
	if ( !pEditOps )
		return;

	if (debug_bit & bits_debugOverlayNodes)
	{
		if (pEditOps->m_debugNetOverlays & bits_debugOverlayNodesLev2)
		{
			pEditOps->m_debugNetOverlays &= ~bits_debugOverlayNodes;
			pEditOps->m_debugNetOverlays &= ~bits_debugOverlayNodesLev2;
		}
		else if (pEditOps->m_debugNetOverlays & bits_debugOverlayNodes)
		{
			pEditOps->m_debugNetOverlays |= bits_debugOverlayNodesLev2;
		}
		else 
		{
			pEditOps->m_debugNetOverlays |= bits_debugOverlayNodes;

			// Recalculate usable nodes for current hull
			pEditOps->RecalcUsableNodesForHull();
		}
	}
	else if (pEditOps->m_debugNetOverlays & debug_bit)
	{
		pEditOps->m_debugNetOverlays &= ~debug_bit;
	}
	else
	{
		pEditOps->m_debugNetOverlays |= debug_bit;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws edit display info on screen
//-----------------------------------------------------------------------------

void CAI_NetworkEditTools::DrawEditInfoOverlay(void)
{
	hudtextparms_s tTextParam;
	tTextParam.x			= 0.8;
	tTextParam.y			= 0.8;
	tTextParam.effect		= 0;
	tTextParam.r1			= 255;
	tTextParam.g1			= 255;
	tTextParam.b1			= 255;
	tTextParam.a1			= 255;
	tTextParam.r2			= 255;
	tTextParam.g2			= 255;
	tTextParam.b2			= 255;
	tTextParam.a2			= 255;
	tTextParam.fadeinTime	= 0;
	tTextParam.fadeoutTime	= 0;
	tTextParam.holdTime		= 1;
	tTextParam.fxTime		= 0;
	tTextParam.channel		= 0;
	
	char hullTypeTxt[50];
	char nodeTypeTxt[50];
	char editTypeTxt[50];
	char outTxt[255];

	Q_snprintf(hullTypeTxt,sizeof(hullTypeTxt),"  %s",NAI_Hull::Name(m_iHullDrawNum));
	Q_snprintf(outTxt,sizeof(outTxt),"Displaying:\n%s\n\n", hullTypeTxt);

	if (engine->IsInEditMode())
	{
		char outTxt2[255];
		Q_snprintf(nodeTypeTxt,sizeof(nodeTypeTxt),"  %s (l)", m_bLinkEditMode ? "Links":"Nodes");
		Q_snprintf(editTypeTxt,sizeof(editTypeTxt),"  %s (m)", m_bAirEditMode  ? "Air":"Ground");
		Q_snprintf(outTxt2,sizeof(outTxt2),"Editing:\n%s\n%s", editTypeTxt,nodeTypeTxt);
		Q_strncat(outTxt,outTxt2,sizeof(outTxt), COPY_ALL_CHARACTERS);

		// Print in red if network needs rebuilding
		if (m_debugNetOverlays & bits_debugNeedRebuild)
		{
			tTextParam.g1			= 0;
			tTextParam.b1			= 0;
		}
	}

	UTIL_HudMessageAll( tTextParam, outTxt );


}

//-----------------------------------------------------------------------------
// Purpose: Draws AINetwork on the screen
//-----------------------------------------------------------------------------

void CAI_NetworkEditTools::DrawAINetworkOverlay(void)
{
	// ------------------------------------
	//  If network isn't loaded yet return
	// ------------------------------------
	if (!CAI_NetworkManager::NetworksLoaded())
	{
		return;
	}

	// ----------------------------------------------
	//  So we don't fill up the client message queue
	//  with node drawing messages, only send them
	//  in chuncks
	// ----------------------------------------------
	static int		startDrawNode	= 0;
	static int		endDrawNode		= 0;
	static float	flDrawDuration;
	endDrawNode		= startDrawNode + 20;
	flDrawDuration	= 0.1 * (m_pNetwork->NumNodes()-1)/20;
	if ( flDrawDuration < .1 )
		flDrawDuration = .1;
	if (endDrawNode > m_pNetwork->NumNodes())
	{
		endDrawNode		= m_pNetwork->NumNodes();
	}

	// ---------------------
	// Draw grid
	// ---------------------
	if (m_debugNetOverlays & bits_debugOverlayGrid)
	{
		// Trace a line to where player is looking
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

		if (pPlayer)
		{
			Vector vForward;
			Vector vSource = pPlayer->EyePosition();
			pPlayer->EyeVectors( &vForward );

			trace_t tr;
			AI_TraceLine ( vSource, vSource + vForward * 2048, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

			float dotPr = DotProduct(Vector(0,0,1),tr.plane.normal);
			if (tr.fraction != 1.0 &&  dotPr > 0.5)
			{
				NDebugOverlay::Grid( tr.endpos + Vector(0,0,1) );
			}
		}
	}

	// --------------------
	CAI_Node **pAINode = m_pNetwork->AccessNodes();

	// --------------------
	// Draw the graph connectivity
	// ---------------------
	if (m_debugNetOverlays & bits_debugOverlayGraphConnect) 
	{
		// ---------------------------------------------------
		//  If network needs rebuilding do so before display
		// --------------------------------------------------
		if (m_debugNetOverlays & bits_debugNeedRebuild)
		{
			m_pManager->RebuildNetworkGraph();
		}
		else if (m_iGConnectivityNode != NO_NODE)
		{
			for (int node=0;node<m_pNetwork->NumNodes();node++) 
			{
				if ( m_pNetwork->IsConnected( m_iGConnectivityNode, node) )
				{
					Vector srcPos = pAINode[m_iGConnectivityNode]->GetPosition(m_iHullDrawNum);
					Vector desPos = pAINode[node]->GetPosition(m_iHullDrawNum);
					NDebugOverlay::Line(srcPos, desPos, 255,0,255, false,0);
				}
			}
		}
	}

	// --------------------
	// Draw the hulls
	// ---------------------
	if (m_debugNetOverlays & bits_debugOverlayHulls)
	{
		// ---------------------------------------------------
		//  If network needs rebuilding do so before display
		// --------------------------------------------------
		if (m_debugNetOverlays & bits_debugNeedRebuild)
		{
			m_pManager->RebuildNetworkGraph();
		}
		else
		{
			for (int node=startDrawNode;node<endDrawNode;node++) 
			{
				for (int link=0;link<pAINode[node]->NumLinks();link++) 
				{
					// Only draw link once
					if (pAINode[node]->GetLinkByIndex(link)->DestNodeID(node) < node) 
					{

						Vector srcPos	 = pAINode[pAINode[node]->GetLinkByIndex(link)->m_iSrcID]->GetPosition(m_iHullDrawNum);
						Vector desPos	 = pAINode[pAINode[node]->GetLinkByIndex(link)->m_iDestID]->GetPosition(m_iHullDrawNum);
						Vector direction = desPos - srcPos;
						float length	 = VectorNormalize(direction);
						Vector hullMins = NAI_Hull::Mins(m_iHullDrawNum);
						Vector hullMaxs = NAI_Hull::Maxs(m_iHullDrawNum);
						hullMaxs.x = length + hullMaxs.x;

						if (pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[m_iHullDrawNum] & bits_CAP_MOVE_FLY) 
						{	
							NDebugOverlay::BoxDirection(srcPos, hullMins, hullMaxs, direction, 100,255,255,20,flDrawDuration);
						}
						
						if (pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[m_iHullDrawNum] & bits_CAP_MOVE_CLIMB) 
						{	
							// Display as a vertical slice up the climbing surface unless dismount node
							if (pAINode[pAINode[node]->GetLinkByIndex(link)->m_iSrcID]->GetOrigin() != pAINode[pAINode[node]->GetLinkByIndex(link)->m_iDestID]->GetOrigin())
							{
								hullMaxs.x = hullMaxs.x - length;
								if (srcPos.z < desPos.z)
								{
									hullMaxs.z = length + hullMaxs.z;
								}
								else
								{
									hullMins.z = hullMins.z - length;
								}
								direction = Vector(0,1,0);

							}
							NDebugOverlay::BoxDirection(srcPos, hullMins, hullMaxs, direction, 255,0,255,20,flDrawDuration);
						}

						if (pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[m_iHullDrawNum] & bits_CAP_MOVE_GROUND) 
						{	
							NDebugOverlay::BoxDirection(srcPos, hullMins, hullMaxs, direction, 0,255,50,20,flDrawDuration);
						}

						else if (pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[m_iHullDrawNum] & bits_CAP_MOVE_JUMP) 
						{	
							NDebugOverlay::BoxDirection(srcPos, hullMins, hullMaxs, direction, 0,0,255,20,flDrawDuration);
						}
					}
				}
			}
		}
	}

	// --------------------
	// Draw the hints
	// ---------------------
	if (m_debugNetOverlays & bits_debugOverlayHints)
	{
		CAI_HintManager::DrawHintOverlays(flDrawDuration);
	}

	// -------------------------------
	// Draw the nodes and connections
	// -------------------------------
	if (m_debugNetOverlays & (bits_debugOverlayNodes | bits_debugOverlayConnections)) 
	{
		for (int node=startDrawNode;node<endDrawNode;node++) {

			// This gets expensive, so see if the node is visible to the client
			if (pAINode[node]->GetType() != NODE_DELETED)
			{
				// --------------------
				// Draw the connections
				// ---------------------
				if (m_debugNetOverlays & bits_debugOverlayConnections) 
				{
					// ---------------------------------------------------
					//  If network needs rebuilding do so before display
					// --------------------------------------------------
					if (m_debugNetOverlays & bits_debugNeedRebuild)
					{
						m_pManager->RebuildNetworkGraph();
					}
					else
					{
						for (int link=0;link<pAINode[node]->NumLinks();link++) {

							// Only draw link once
							if (pAINode[node]->GetLinkByIndex(link)->DestNodeID(node) < node)
							{
								int srcID = pAINode[node]->GetLinkByIndex(link)->m_iSrcID;
								int desID = pAINode[node]->GetLinkByIndex(link)->m_iDestID;

								Vector srcPos	 = pAINode[srcID]->GetPosition(m_iHullDrawNum);
								Vector desPos	 = pAINode[desID]->GetPosition(m_iHullDrawNum);

								int srcType = pAINode[pAINode[node]->GetLinkByIndex(link)->m_iSrcID]->GetType();
								int desType = pAINode[pAINode[node]->GetLinkByIndex(link)->m_iDestID]->GetType();

								int linkInfo = pAINode[node]->GetLinkByIndex(link)->m_LinkInfo;
								int moveTypes = pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[m_iHullDrawNum];
			
								// when rendering, raise NODE_GROUND off the floor slighty as they seem to clip too much
								if ( srcType == NODE_GROUND)
								{
									srcPos.z += 1.0;
								}

								if ( desType == NODE_GROUND)
								{
									desPos.z += 1.0;
								}

								// Draw in red if stale link
								if (linkInfo & bits_LINK_STALE_SUGGESTED)
								{
									NDebugOverlay::Line(srcPos, desPos, 255,0,0, false, flDrawDuration);
								}
								// Draw in grey if link turned off
								else if (linkInfo & bits_LINK_OFF)						
								{
									NDebugOverlay::Line(srcPos, desPos, 100,100,100, false, flDrawDuration);
								}
								else if ((m_debugNetOverlays & bits_debugOverlayFlyConnections) && (moveTypes & bits_CAP_MOVE_FLY))
								{	
									NDebugOverlay::Line(srcPos, desPos, 100,255,255, false, flDrawDuration);
								}
								else if (moveTypes & bits_CAP_MOVE_CLIMB) 
								{	
									NDebugOverlay::Line(srcPos, desPos, 255,0,255, false, flDrawDuration);
								}
								else if (moveTypes & bits_CAP_MOVE_GROUND) 
								{	
									NDebugOverlay::Line(srcPos, desPos, 0,255,50, false, flDrawDuration);
								}
								else if ((m_debugNetOverlays & bits_debugOverlayJumpConnections) && (moveTypes & bits_CAP_MOVE_JUMP) )
								{	
									NDebugOverlay::Line(srcPos, desPos, 0,0,255, false, flDrawDuration);
								}
								else  
								{	// Dark red if this hull can't use
									bool isFly = ( srcType == NODE_AIR || desType == NODE_AIR );
									bool isJump = true;
									for ( int i = HULL_HUMAN; i < NUM_HULLS; i++ )
									{
										if ( pAINode[node]->GetLinkByIndex(link)->m_iAcceptedMoveTypes[i] & ~bits_CAP_MOVE_JUMP )
										{
											isJump = false;
											break;
										}
									}
									if ( ( isFly && (m_debugNetOverlays & bits_debugOverlayFlyConnections) ) ||
										 ( isJump && (m_debugNetOverlays & bits_debugOverlayJumpConnections) ) ||
										 ( !isFly && !isJump ) )
									{
										NDebugOverlay::Line(srcPos, desPos, 100,25,25, false, flDrawDuration);
									}
								}
							}
						}
					}
				}
				if (m_debugNetOverlays & bits_debugOverlayNodes) 
				{
					int r  = 255;
					int g  = 0;
					int b  = 0;

					// If checking visibility base color off of visibility info
					if (m_debugNetOverlays & bits_debugOverlayVisibility &&
						m_iVisibilityNode != NO_NODE)
					{
						// ---------------------------------------------------
						//  If network needs rebuilding do so before display
						// --------------------------------------------------
						if (m_debugNetOverlays & bits_debugNeedRebuild)
						{
							m_pManager->RebuildNetworkGraph();
						}
					}

					// If checking graph connectivity base color off of connectivity info
					if (m_debugNetOverlays & bits_debugOverlayGraphConnect &&
						m_iGConnectivityNode != NO_NODE)
					{
						// ---------------------------------------------------
						//  If network needs rebuilding do so before display
						// --------------------------------------------------
						if (m_debugNetOverlays & bits_debugNeedRebuild)
						{
							m_pManager->RebuildNetworkGraph();
						}
						else if (m_pNetwork->IsConnected( m_iGConnectivityNode, node) )
						{
							r  = 0;
							g  = 0;
							b  = 255;
						}
					}
					// Otherwise base color off of node type
					else 
					{
						// If node is new and hasn't been rebuild yet
						if (pAINode[node]->m_eNodeInfo & bits_NODE_WC_CHANGED)
						{
							r = 200;
							g = 200;
							b = 200;
						}

						// If node doesn't fit the current hull size
						else if (pAINode[node]->m_eNodeInfo & bits_NODE_WONT_FIT_HULL)
						{
							r = 255;
							g = 25;
							b = 25;
						}

						else if (pAINode[node]->GetType() == NODE_CLIMB)
						{
							r  = 255;
							g  = 0;
							b  = 255;
						}
						else if (pAINode[node]->GetType() == NODE_AIR)
						{
							r  = 0;
							g  = 255;
							b  = 255;
						}
						else if (pAINode[node]->GetType() == NODE_GROUND)
						{
							r  = 0;
							g  = 255;
							b  = 100;
						}
					}


					Vector nodePos;

					nodePos	 = pAINode[node]->GetPosition(m_iHullDrawNum);

					NDebugOverlay::Box(nodePos, Vector(-5,-5,-5), Vector(5,5,5), r,g,b,0,flDrawDuration);
					
					// If climb node draw line in facing direction
					if (pAINode[node]->GetType() == NODE_CLIMB)
					{
						Vector offsetDir	= 12.0 * Vector(cos(DEG2RAD(pAINode[node]->GetYaw())),sin(DEG2RAD(pAINode[node]->GetYaw())),flDrawDuration);
						NDebugOverlay::Line(nodePos, nodePos+offsetDir, r,g,b,false,flDrawDuration);
					}

					if ( pAINode[node]->GetHint() )
					{
						NDebugOverlay::Box( nodePos, Vector(-7,-7,-7), Vector(7,7,7), 255,255,0,0,flDrawDuration);
					}

					if (m_debugNetOverlays & bits_debugOverlayNodesLev2)
					{
						CFmtStr msg;

						if ( m_pNodeIndexTable )
							msg.sprintf("%i (wc:%i; z:%i)",node,m_pNodeIndexTable[pAINode[node]->GetId()], pAINode[node]->GetZone());
						else
							msg.sprintf("%i (z:%i)",node,pAINode[node]->GetZone());

						Vector loc = nodePos;
						loc.x+=6;
						loc.y+=6;
						loc.z+=6;
						NDebugOverlay::Text( loc, msg, true, flDrawDuration);
						
						// Print the hintgroup if we have one
						if ( pAINode[node]->GetHint() )
						{
							msg.sprintf("%s", STRING( pAINode[node]->GetHint()->GetGroup() ));
							loc.z-=3;
							NDebugOverlay::Text( loc, msg, true, flDrawDuration);
						}
					}
				}
			}
		}
	}

	// -------------------------------
	//  Identify hull being displayed
	// -------------------------------
	if (m_debugNetOverlays & (bits_debugOverlayNodes | bits_debugOverlayConnections | bits_debugOverlayHulls)) 
	{
		DrawEditInfoOverlay();
	}

	// ----------------------------
	//  Increment node draw chunk
	// ----------------------------
	startDrawNode = endDrawNode;
	if (startDrawNode >= m_pNetwork->NumNodes())
	{
		startDrawNode = 0;
	}

	// ----------------------------
	// Output performance stats
	// ----------------------------
#ifdef AI_PERF_MON
		if (m_fNextPerfStatTime < gpGlobals->curtime)
		{
			char temp[512];
			Q_snprintf(temp,sizeof(temp),"%3.2f NN/m\n%3.2f P/m\n",(m_nPerfStatNN/1.0),(m_nPerfStatPB/1.0));
			UTIL_CenterPrintAll(temp);

			m_fNextPerfStatTime = gpGlobals->curtime + 1;
			m_nPerfStatNN		= 0;
			m_nPerfStatPB		= 0;
		}
#endif		
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

CAI_NetworkEditTools::CAI_NetworkEditTools(CAI_NetworkManager *pNetworkManager)
{
	// ----------------------------------------------------------------------------
	// If in wc_edit mode 
	// ----------------------------------------------------------------------------
	if (engine->IsInEditMode())
	{
		// ----------------------------------------------------------------------------
		// Allocate extra space for storing undropped node positions
		// ----------------------------------------------------------------------------
		m_pWCPosition		= new Vector[MAX_NODES];
	}
	else
	{
		m_pWCPosition	= NULL;
	}

	m_pNodeIndexTable		= NULL;
	m_debugNetOverlays		= 0;

	// ----------------------------------------------------------------------------
	// Allocate table of WC Id's. If not in edit mode Deleted after initialization 
	// ----------------------------------------------------------------------------
	m_pNodeIndexTable	= new int[MAX_NODES];
	for ( int i = 0; i < MAX_NODES; i++ )
		m_pNodeIndexTable[i] = NO_NODE;
	m_nNextWCIndex		= 0;

	m_pNetwork = pNetworkManager->GetNetwork(); // @tbd
	m_pManager = pNetworkManager;

	
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------

CAI_NetworkEditTools::~CAI_NetworkEditTools()
{
	// --------------------------------------------------------
	// If in edit mode tell WC I'm ending my session
	// --------------------------------------------------------
#ifdef _WIN32
	Editor_EndSession(false);
#endif
	delete[] m_pNodeIndexTable;
}

//-----------------------------------------------------------------------------
// CAI_NetworkBuilder
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::FloodFillZone( CAI_Node **ppNodes, CAI_Node *pNode, int zone )
{
	pNode->SetZone( zone );

	for (int link = 0; link < pNode->NumLinks(); link++) 
	{
		CAI_Link *pLink = pNode->GetLinkByIndex(link);
		CAI_Node *pLinkedNode = ( pLink->m_iDestID == pNode->GetId()) ? ppNodes[pLink->m_iSrcID] : ppNodes[pLink->m_iDestID];
		if ( pLinkedNode->GetZone() == AI_NODE_ZONE_UNKNOWN )
			FloodFillZone( ppNodes, pLinkedNode, zone );
			
		Assert( pLinkedNode->GetZone() == pNode->GetZone() && pNode->GetZone() == zone );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::InitZones( CAI_Network *pNetwork )
{
	int nNodes = pNetwork->NumNodes();
	CAI_Node **ppNodes = pNetwork->AccessNodes();

	if ( !nNodes )
		return;
		
	int i;
	
	for (i = 0; i < nNodes; i++)
	{	
		ppNodes[i]->SetZone( AI_NODE_ZONE_UNKNOWN );
	}

	// Mark solo nodes
	for (i = 0; i < nNodes; i++)
	{	
		if ( ppNodes[i]->NumLinks() == 0 )
			ppNodes[i]->SetZone( AI_NODE_ZONE_SOLO );
	}
	
	int curZone = AI_NODE_FIRST_ZONE;

	for (i = 0; i < nNodes; i++)
	{	
		if ( ppNodes[i]->GetZone() == AI_NODE_ZONE_UNKNOWN )
		{
			FloodFillZone( (CAI_Node **)ppNodes, ppNodes[i], curZone );
			curZone++;
		}
	}

#ifdef DEBUG
	for (i = 0; i < nNodes; i++)
	{	
		Assert( ppNodes[i]->GetZone() != AI_NODE_ZONE_UNKNOWN );
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose:  Used for WC edit move to rebuild the network around the given
//			 location.  Rebuilding the entire network takes too long
//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::Rebuild( CAI_Network *pNetwork )
{
	int nNodes = pNetwork->NumNodes();
	CAI_Node **ppNodes = pNetwork->AccessNodes();

	if ( !nNodes )
		return;

	BeginBuild();
	
	// ------------------------------------------------------------
	//  First mark all nodes around vecPos as having to be rebuilt
	// ------------------------------------------------------------
	int i;
	for (i = 0; i < nNodes; i++)
	{
		// --------------------------------------------------------------------
		// If changed, mark all nodes that are within the max link distance to
		// the changed node as having to be rebuild
		// --------------------------------------------------------------------
		if (ppNodes[i]->m_eNodeInfo & bits_NODE_WC_CHANGED)
		{
			Vector vRebuildPos			= ppNodes[i]->GetOrigin();
			ppNodes[i]->SetNeedsRebuild();
			ppNodes[i]->SetZone( AI_NODE_ZONE_UNIVERSAL );
			for (int node = 0; node < nNodes; node++)
			{
				if ( ppNodes[node]->GetType() == NODE_AIR )
				{
					if ((ppNodes[node]->GetOrigin() - vRebuildPos).LengthSqr() < MAX_AIR_NODE_LINK_DIST_SQ)
					{
						ppNodes[node]->SetNeedsRebuild();
						ppNodes[node]->SetZone( AI_NODE_ZONE_UNIVERSAL );
					}
				}
				else
				{
					if ((ppNodes[node]->GetOrigin() - vRebuildPos).LengthSqr() < MAX_NODE_LINK_DIST_SQ)
					{
						ppNodes[node]->SetNeedsRebuild();
						ppNodes[node]->SetZone( AI_NODE_ZONE_UNIVERSAL );
					}
				}
			}
		}
	}

	// ---------------------------
	// Initialize node positions
	// ---------------------------
	for (i = 0; i < nNodes; i++)
	{
		if (ppNodes[i]->NeedsRebuild())
		{
			InitNodePosition( pNetwork, ppNodes[i] );
		}
	}
	nNodes = pNetwork->NumNodes(); // InitNodePosition can create nodes

	// ---------------------------
	// Initialize node neighbors
	// ---------------------------
	m_DidSetNeighborsTable.Resize( nNodes );
	m_DidSetNeighborsTable.ClearAll();
	m_NeighborsTable.SetSize( nNodes );
	for (i = 0; i < nNodes; i++)
	{
		m_NeighborsTable[i].Resize( nNodes );
	}
	for (i = 0; i < nNodes; i++)
	{
		// If near point of change recalculate
		if (ppNodes[i]->NeedsRebuild())
		{
			InitNeighbors( pNetwork, ppNodes[i] );
		}
	}

	// ---------------------------
	// Force node neighbors for dynamic links
	// ---------------------------
	ForceDynamicLinkNeighbors();

	// ---------------------------
	// Initialize accepted hulls
	// ---------------------------
	for (i = 0; i < nNodes; i++)
	{
		if (ppNodes[i]->NeedsRebuild())
		{
			ppNodes[i]->ClearLinks();
		}
	}
	for (i = 0; i < nNodes; i++)
	{	
		if (ppNodes[i]->NeedsRebuild())
		{
			InitLinks( pNetwork, ppNodes[i] );
		}
	}

	g_pAINetworkManager->FixupHints();

	EndBuild();
}

//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::BeginBuild()
{
	m_pTestHull = CAI_TestHull::GetTestHull();
}

//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::EndBuild()
{
	m_NeighborsTable.SetSize(0);
	m_DidSetNeighborsTable.Resize(0);
	CAI_TestHull::ReturnTestHull();
}

//-----------------------------------------------------------------------------
// Purpose:  Only called if network has changed since last time level
//			 was loaded
//-----------------------------------------------------------------------------


void CAI_NetworkBuilder::Build( CAI_Network *pNetwork )
{
	int nNodes = pNetwork->NumNodes();
	CAI_Node **ppNodes = pNetwork->AccessNodes();

	if ( !nNodes )
		return;

	CAI_NetworkBuildHelper *pHelper = (CAI_NetworkBuildHelper *)CreateEntityByName( "ai_network_build_helper" );

	VPROF( "AINet" );

	BeginBuild();

	CFastTimer masterTimer;
	CFastTimer timer;
	
	DevMsg( "Building AI node graph...\n");
	masterTimer.Start();
	
	// ---------------------------
	// Initialize node positions
	// ---------------------------
	DevMsg( "Initializing node positions...\n" );
	timer.Start();
	int i;
	for ( i = 0; i < nNodes; i++)
	{
		InitNodePosition( pNetwork, ppNodes[i] );
		if ( pHelper )
			pHelper->PostInitNodePosition( pNetwork, ppNodes[i] );
	}
	nNodes = pNetwork->NumNodes(); // InitNodePosition can create nodes
	timer.End();
	DevMsg( "...done initializing node positions. %f seconds\n", timer.GetDuration().GetSeconds() );

	// ---------------------------
	// Initialize node neighbors
	// ---------------------------
	DevMsg( "Initializing node neighbors...\n" );
	timer.Start();
	m_DidSetNeighborsTable.Resize( nNodes );
	m_DidSetNeighborsTable.ClearAll();
	m_NeighborsTable.SetSize( nNodes );
	for (i = 0; i < nNodes; i++)
	{
		m_NeighborsTable[i].Resize( nNodes );
		m_NeighborsTable[i].ClearAll();
	}
	for (i = 0; i < nNodes; i++)
	{	
		InitNeighbors( pNetwork, ppNodes[i] );
	}
	timer.End();
	DevMsg( "...done initializing node neighbors. %f seconds\n", timer.GetDuration().GetSeconds() );

	// ---------------------------
	// Force node neighbors for dynamic links
	// ---------------------------
	DevMsg( "Forcing dynamic link neighbors...\n" );
	timer.Start();
	ForceDynamicLinkNeighbors();
	timer.End();
	DevMsg( "...done forcing dynamic link neighbors. %f seconds\n", timer.GetDuration().GetSeconds() );

	// ---------------------------
	// Initialize accepted hulls
	// ---------------------------
	DevMsg( "Determining links...\n" );
	timer.Start();
	for (i = 0; i < nNodes; i++)
	{	
		// Make sure all the links are clear
		ppNodes[i]->ClearLinks();
	}
	for (i = 0; i < nNodes; i++)
	{	
		InitLinks( pNetwork, ppNodes[i] );
	}
	timer.End();
	DevMsg( "...done determining links. %f seconds\n", timer.GetDuration().GetSeconds() );

	// ------------------------------
	// Initialize disconnected nodes
	// ------------------------------
	DevMsg( "Determining zones...\n" );
	timer.Start();
	InitZones( pNetwork);
	timer.End();
	masterTimer.End();
	DevMsg( "...done determining zones. %f seconds\n", timer.GetDuration().GetSeconds() );
	DevMsg( "...done building AI node graph, %f seconds\n", masterTimer.GetDuration().GetSeconds() );

	g_pAINetworkManager->FixupHints();

	EndBuild();

	if ( pHelper )
		UTIL_Remove( pHelper );
}

//------------------------------------------------------------------------------
// Purpose : Forces testing of a connection between src and dest IDs for all dynamic links
//			 	
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_NetworkBuilder::ForceDynamicLinkNeighbors(void)
{
	if (!g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable)
	{
		DevMsg("ERROR: Trying initialize links with no WC ID table!\n");
		return;
	}

	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		// -------------------------------------------------------------
		//  First convert this links WC IDs to engine IDs
		// -------------------------------------------------------------
		int	nSrcID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nSrcEditID );
		if (nSrcID == -1)
		{
			DevMsg("ERROR: Dynamic link source WC node %d not found\n", pDynamicLink->m_nSrcEditID );
		}

		int	nDestID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nDestEditID );
		if (nDestID == -1)
		{
			DevMsg("ERROR: Dynamic link dest WC node %d not found\n", pDynamicLink->m_nDestEditID );
		}

		if ( nSrcID != -1 && nDestID != -1 )
		{
			if ( nSrcID < g_pBigAINet->NumNodes() && nDestID < g_pBigAINet->NumNodes() )
			{
				CAI_Node *pSrcNode = g_pBigAINet->GetNode( nSrcID );
				CAI_Node *pDestNode = g_pBigAINet->GetNode( nDestID );

				// -------------------------------------------------------------
				//  Force visibility and neighbor-ness between the nodes
				// -------------------------------------------------------------
				Assert( pSrcNode );
				Assert( pDestNode );

				m_NeighborsTable[pSrcNode->GetId()].Set(pDestNode->GetId());
				m_NeighborsTable[pDestNode->GetId()].Set(pSrcNode->GetId());
			}
		}

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}

CAI_NetworkBuilder g_AINetworkBuilder;


//-----------------------------------------------------------------------------
// Purpose: Initializes position of climb node in the world.  Climb nodes are
//			set to be just above the floor or at the same level at the
//			dismount point for the node
//
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkBuilder::InitClimbNodePosition(CAI_Network *pNetwork, CAI_Node *pNode) 
{
	AI_PROFILE_SCOPE( CAI_Node_InitClimbNodePosition );

	// If this is a node for mounting/dismounting the climb skip it
	if ( pNode->m_eNodeInfo & (bits_NODE_CLIMB_OFF_FORWARD | bits_NODE_CLIMB_OFF_LEFT | bits_NODE_CLIMB_OFF_RIGHT) )
	{
		return;
	}

	// Figure out which directions I can dismount from the climb node

	//float  hullLength	= NAI_Hull::Length(HULL_SMALL);
	//Vector offsetDir	= Vector(cos(DEG2RAD(m_flYaw)),sin(DEG2RAD(m_flYaw)),0);

	// ----------------
	//  Check position
	// ----------------
	trace_t trace;
	Vector posOnLadder		= pNode->GetPosition(HULL_SMALL_CENTERED);
	AI_TraceHull( posOnLadder, posOnLadder + Vector( 0, 0, -37 ), 
		NAI_Hull::Mins(HULL_SMALL_CENTERED), NAI_Hull::Maxs(HULL_SMALL_CENTERED), 
		MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trace );

	// --------------------------------------------------------------------
	// If climb node is right above the floor, we don't need any dismount
	// nodes.  Accept this dropped position and note that this climb node
	// is at the bottom
	// --------------------------------------------------------------------
	if (!trace.startsolid && trace.fraction != 1)
	{
		pNode->m_eNodeInfo		= bits_NODE_CLIMB_BOTTOM;
		InitGroundNodePosition( pNetwork, pNode );
		return;
	}

	// ---------------------------------------------------------------------
	//  If network was already loaded this means we are in wc edit mode
	//  so we shouldn't recreate the added climb nodes
	// ---------------------------------------------------------------------
	if (g_pAINetworkManager->NetworksLoaded())
	{
		return;
	}

	// ---------------------------------------------------------------------
	//	Otherwise we need to create climb nodes for dismounting the climb
	//  and place the height of the climb node at the dismount position
	// ---------------------------------------------------------------------
	int checkNodeTypes[3] = { bits_NODE_CLIMB_OFF_FORWARD, bits_NODE_CLIMB_OFF_LEFT, bits_NODE_CLIMB_OFF_RIGHT };

	int numExits = 0;

	// DevMsg( "testing %f %f %f\n", GetOrigin().x, GetOrigin().y, GetOrigin().z );

	for (int i = 0; i < 3; i++)
	{
		pNode->m_eNodeInfo = checkNodeTypes[i];

		Vector origin = pNode->GetPosition(HULL_SMALL_CENTERED);
		
		// DevMsg( "testing %f %f %f\n", origin.x, origin.y, origin.z );
		// ----------------
		//  Check outward
		// ----------------
		AI_TraceLine ( posOnLadder,
						 origin,
						 MASK_NPCSOLID_BRUSHONLY,
						 NULL,
						 COLLISION_GROUP_NONE, 
						 &trace );

		// DevMsg( "to %f %f %f : %d %f", origin.x, origin.y, origin.z, trace.startsolid, trace.fraction );

		if (!trace.startsolid && trace.fraction == 1.0)
		{
			float floorZ = GetFloorZ(origin); // FIXME: don't use this

			if (abs(pNode->GetOrigin().z - floorZ) < 36)
			{
				CAI_Node *new_node		= pNetwork->AddNode( pNode->GetOrigin(), pNode->m_flYaw );
				new_node->m_pHint			= NULL;
				new_node->m_eNodeType		= NODE_CLIMB;
				new_node->m_eNodeInfo		= pNode->m_eNodeInfo;
				InitGroundNodePosition( pNetwork, new_node );

				// copy over the offsets for the first CLIMB_OFF node
				// FIXME: this method is broken for when the CLIMB_OFF nodes are at different heights
				if (numExits == 0)
				{
					for (int hull = 0; hull < NUM_HULLS; hull++)
					{
						pNode->m_flVOffset[hull] = new_node->m_flVOffset[hull];
					}
				}
				else
				{
					for (int hull = 0; hull < NUM_HULLS; hull++)
					{
						if (fabs(pNode->m_flVOffset[hull] - new_node->m_flVOffset[hull]) > 1)
						{
							DevMsg(2, "Warning: Climb Node %i has different exit heights for hull %s\n", pNode->m_iID, NAI_Hull::Name(hull));
						}
					}
				}

				numExits++;
			}
		}
		// DevMsg( "\n");
	}

	if (numExits == 0)
	{
		DevMsg("ERROR: Climb Node %i has no way off\n",pNode->m_iID);
	}

	// this is a node that can't get gotten to directly
	pNode->m_eNodeInfo = bits_NODE_CLIMB_ON;
}

//-----------------------------------------------------------------------------
// Purpose: Initializes position of the node sitting on the ground.
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkBuilder::InitGroundNodePosition(CAI_Network *pNetwork, CAI_Node *pNode)
{
	AI_PROFILE_SCOPE( CAI_Node_InitGroundNodePosition );

	if ( pNode->m_eNodeInfo & bits_DONT_DROP )
		return;

	// find actual floor for each hull type
	for (int hull = 0; hull < NUM_HULLS; hull++)
	{
		trace_t tr;
		Vector origin = pNode->GetOrigin();
		Vector mins, maxs;

		// turn hull into pancake to avoid problems with ceiling
		mins = NAI_Hull::Mins(hull);
		maxs = NAI_Hull::Maxs(hull);
		maxs.z = mins.z;

		// Add an epsilon for cast
		origin.z += 0.1;

		// shift up so bottom of box is at center of node
		origin.z -= mins.z;

		AI_TraceHull( origin, origin + Vector( 0, 0, -384 ), mins, maxs, MASK_NPCSOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

		if ( !tr.startsolid )
			pNode->m_flVOffset[hull] = tr.endpos.z - pNode->GetOrigin().z + 0.1;
		else
			pNode->m_flVOffset[hull] = -mins.z + 0.1;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Initializes position of the node in the world.  Only called if
//			the network was never initialized
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkBuilder::InitNodePosition(CAI_Network *pNetwork, CAI_Node *pNode) 
{
	AI_PROFILE_SCOPE( CAI_Node_InitNodePosition );

	if (pNode->m_eNodeType == NODE_AIR)
	{
		return;
	}
	else if (pNode->m_eNodeType == NODE_CLIMB)
	{
		InitClimbNodePosition(pNetwork, pNode);
		return;
	}

	// Otherwise mark as a land node and drop to the floor

	else if (pNode->m_eNodeType == NODE_GROUND)
	{
		InitGroundNodePosition( pNetwork, pNode );

		if (pNode->m_flVOffset[HULL_SMALL_CENTERED] < -100)
		{
			Assert( pNetwork == g_pBigAINet );
			DevWarning("ERROR: Node %.0f %.0f %.0f, WC ID# %i, is either too low (fell through floor) or too high (>100 units above floor)\n",
				pNode->GetOrigin().x, pNode->GetOrigin().y, pNode->GetOrigin().z, 
				g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pNode->m_iID]);

			pNode->m_eNodeInfo |= bits_NODE_FALLEN;
		}
		return;
	}
	/*	// If under water, not that the node is in water	<<TODO>>  when we get water
	else if ( UTIL_PointContents(GetOrigin()) & MASK_WATER )
	{
		m_eNodeType |= NODE_WATER;
	}
	*/
	else if (pNode->m_eNodeType != NODE_DELETED)
	{
		DevMsg( "Bad node type!\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the visibility for this node.  (What nodes it can see with a
//			line trace)
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NetworkBuilder::InitVisibility(CAI_Network *pNetwork, CAI_Node *pNode)
{
	AI_PROFILE_SCOPE( CAI_Node_InitVisibility );
	
	// If a deleted node bail
	if (pNode->m_eNodeType == NODE_DELETED)
	{
		return;
	}
	// The actual position of some nodes may be inside geometry as they have
	// hull specific position offsets (e.g. climb nodes).  Get the hull specific 
	// position using the smallest hull to make sure were not in geometry
	Vector srcPos = pNode->GetPosition(HULL_SMALL_CENTERED);

	// Check the visibility on every other node in the network
	for (int testnode = 0; testnode < pNetwork->NumNodes(); testnode++ )
  	{
		CAI_Node *testNode = pNetwork->GetNode( testnode );

		if ( DebuggingConnect( pNode->m_iID, testnode ) )
		{
			DevMsg( " " ); // break here..
		}

		// We know we can view ourself
		if (pNode->m_iID == testnode)
		{
			m_NeighborsTable[pNode->m_iID].Set(testNode->m_iID);
			continue;
		}
		
		// Remove duplicate nodes unless a climb node as they move
		if (testNode->GetOrigin() == pNode->GetOrigin() && testNode->GetType() != NODE_CLIMB)
		{
			testNode->SetType( NODE_DELETED );
			DevMsg( 2, "Probable duplicate node placed at %s\n", VecToString(testNode->GetOrigin()) );
			continue;
		}

		// If a deleted node we don't care about it
		if (testNode->GetType() == NODE_DELETED)
		{
			continue;
		}

		if ( m_DidSetNeighborsTable.IsBitSet( testNode->m_iID ) )
		{
			if ( m_NeighborsTable[testNode->m_iID].IsBitSet(pNode->m_iID))
				m_NeighborsTable[pNode->m_iID].Set(testNode->m_iID);

			continue;
		}

		float flDistToCheckNode = ( testNode->GetOrigin() - pNode->GetOrigin() ).LengthSqr(); 

		if ( testNode->GetType() == NODE_AIR )
		{
			if (flDistToCheckNode > MAX_AIR_NODE_LINK_DIST_SQ) 
				continue;
		}
		else
		{
			if (flDistToCheckNode > MAX_NODE_LINK_DIST_SQ) 
				continue;
		}

		// The actual position of some nodes may be inside geometry as they have
		// hull specific position offsets (e.g. climb nodes).  Get the hull specific 
		// position using the smallest hull to make sure were not in geometry
		Vector destPos = pNetwork->GetNode( testnode )->GetPosition(HULL_SMALL_CENTERED);

		trace_t	tr;
		tr.m_pEnt = NULL;

		// Try several line of sight checks

		bool isVisible = false;

		// ------------------
		//  Bottom to bottom
		// ------------------
		AI_TraceLine ( srcPos, destPos,MASK_NPCWORLDSTATIC,NULL,COLLISION_GROUP_NONE, &tr );
		if (!tr.startsolid && tr.fraction == 1.0)
		{
			isVisible = true;
		}

		// ------------------
		//  Top to top
		// ------------------
		if (!isVisible)
		{
			AI_TraceLine ( srcPos + Vector( 0, 0, 70 ),destPos + Vector( 0, 0, 70 ),MASK_NPCWORLDSTATIC,NULL,COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{	
				isVisible = true;
			}
		}

		// ------------------
		//  Top to Bottom
		// ------------------
		if (!isVisible)
		{
			AI_TraceLine ( srcPos + Vector( 0, 0, 70 ),destPos,MASK_NPCWORLDSTATIC,NULL,COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{	
				isVisible = true;
			}
		}

		// ------------------
		//  Bottom to Top
		// ------------------
		if (!isVisible)
		{
			AI_TraceLine ( srcPos,destPos + Vector( 0, 0, 70 ),MASK_NPCWORLDSTATIC,NULL,COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{	
				isVisible = true;
			}
		}

		// ------------------
		//  Failure
		// ------------------
		if (!isVisible)
		{
			continue;
		}

		/* <<TODO>> may not apply with editable connections.......

		// trace hit a brush ent, trace backwards to make sure that this ent is the only thing in the way.
		if ( tr.fraction != 1.0 )
		{
			pTraceEnt = tr.u.ent;// store the ent that the trace hit, for comparison

			AI_TraceLine ( srcPos,
							 destPos,
							 MASK_NPCSOLID_BRUSHONLY,
							 NULL,
							 &tr );

			
			// there is a solid_bsp ent in the way of these two nodes, so we must record several things about in order to keep
			// track of it in the pathfinding code, as well as through save and restore of the node graph. ANY data that is manipulated 
			// as part of the process of adding a LINKENT to a connection here must also be done in CGraph::SetGraphPointers, where reloaded
			// graphs are prepared for use.
			if ( tr.u.ent == pTraceEnt && !FClassnameIs( tr.u.ent, "worldspawn" ) )
			{
				// get a pointer
				pLinkPool [ cTotalLinks ].m_pLinkEnt = tr.u.ent;

				// record the modelname, so that we can save/load node trees
				memcpy( pLinkPool [ cTotalLinks ].m_szLinkEntModelname, STRING( tr.u.ent->model ), 4 );

				// set the flag for this ent that indicates that it is attached to the world graph
				// if this ent is removed from the world, it must also be removed from the connections
				// that it formerly blocked.
				CBaseEntity *e = CBaseEntity::Instance( tr.u.ent );
				if ( e )
				{
					if ( !(e->GetFlags() & FL_GRAPHED ) )
					{
						e->AddFlag( FL_GRAPHED );
					}
				}
			}
			// even if the ent wasn't there, these nodes couldn't be connected. Skip.
			else
			{
				continue;
			}
		}
*/
		m_NeighborsTable[pNode->m_iID].Set(testNode->m_iID);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Initializes the neighbors list
// Input  :
// Output :
//-----------------------------------------------------------------------------

void CAI_NetworkBuilder::InitNeighbors(CAI_Network *pNetwork, CAI_Node *pNode)
{
	m_NeighborsTable[pNode->m_iID].ClearAll();
	
	// Begin by establishing viewability to limit the number of nodes tested
	InitVisibility( pNetwork, pNode );

	AI_PROFILE_SCOPE_BEGIN( CAI_Node_InitNeighbors );

	// Now check each neighbor against all other neighbors to see if one of
	// them is a redundant connection
	for (int checknode = 0; checknode < pNetwork->NumNodes(); checknode++ )
	{
		if ( DebuggingConnect( pNode->m_iID, checknode ) )
		{
			DevMsg( " " ); // break here..
		}

		// I'm not a neighbor of myself
		if ( pNode->m_iID == checknode )
		{
			m_NeighborsTable[pNode->m_iID].Clear(checknode);
			continue;
		}

		// Only check if already on the neightbor list
		if (!m_NeighborsTable[pNode->m_iID].IsBitSet(checknode)) 
		{
			continue;
		}

		CAI_Node *pCheckNode = pNetwork->GetNode(checknode);

		for (int testnode = 0; testnode < pNetwork->NumNodes(); testnode++ )
		{
			// don't check against itself
			if (( testnode == checknode ) || (testnode == pNode->m_iID))
			{
				continue;
			}

			// Only check if already on the neightbor list
			if (!m_NeighborsTable[pNode->m_iID].IsBitSet(testnode)) 
			{
				continue;
			}

			CAI_Node *pTestNode = pNetwork->GetNode(testnode);

			// ----------------------------------------------------------
			//  Don't check air nodes against nodes of a different types
			// ----------------------------------------------------------
			if ((pCheckNode->GetType() == NODE_AIR && pTestNode->GetType() != NODE_AIR)||
				(pCheckNode->GetType() != NODE_AIR && pTestNode->GetType() == NODE_AIR))
			{
				continue;
			}

			// ----------------------------------------------------------
			// If climb node pairs, don't consider redundancy
			// ----------------------------------------------------------
			if (pNode->GetType() == NODE_CLIMB &&
				(pCheckNode->GetType() == NODE_CLIMB || pTestNode->GetType() == NODE_CLIMB))
			{
				continue;
			}

			// ----------------------------------------------------------
			// If a climb node mounting point is involved, don't consider redundancy
			// ----------------------------------------------------------
			if ( ( pCheckNode->GetOrigin() == pNode->GetOrigin() && pNode->GetType() == NODE_CLIMB && pCheckNode->GetType() == NODE_CLIMB ) ||
				 ( pTestNode->GetOrigin() == pNode->GetOrigin() && pNode->GetType() == NODE_CLIMB && pTestNode->GetType() == NODE_CLIMB ) ||
				 ( pTestNode->GetOrigin() == pCheckNode->GetOrigin() && pCheckNode->GetType() == NODE_CLIMB && pTestNode->GetType() == NODE_CLIMB ) )
			{
				continue;
			}
			
			// @HACKHACK (toml 02-25-04): Ignore redundancy if both nodes are air nodes with
			// hint type "strider node". Really, really should do this in a clean manner
			bool nodeIsStrider = ( pNode->GetHint() && pNode->GetHint()->HintType() == HINT_STRIDER_NODE );
			bool other1IsStrider = ( pCheckNode->GetHint() && pCheckNode->GetHint()->HintType() == HINT_STRIDER_NODE );
			bool other2IsStrider = ( pTestNode->GetHint() && pTestNode->GetHint()->HintType() == HINT_STRIDER_NODE );
			if ( nodeIsStrider && other1IsStrider != other2IsStrider )
			{
				continue;
			}

			Vector	vec2DirToCheckNode = pCheckNode->GetOrigin() - pNode->GetOrigin(); 
			float	flDistToCheckNode  = VectorNormalize( vec2DirToCheckNode );

			Vector	vec2DirToTestNode = ( pTestNode->GetOrigin() - pNode->GetOrigin() ); 
			float	flDistToTestNode  = VectorNormalize( vec2DirToTestNode );

			float	tolerance = 0.92388;	// 45 degrees

			if ( DotProduct ( vec2DirToCheckNode, vec2DirToTestNode ) >= tolerance ) 
			{
				if ( flDistToTestNode < flDistToCheckNode )
				{
					DebugConnectMsg( pNode->m_iID, checknode, "      Revoking neighbor status to to closer redundant link %d\n", testnode );
					m_NeighborsTable[pNode->m_iID].Clear(checknode);
				}
				else
				{
					DebugConnectMsg( pNode->m_iID, testnode, "      Revoking neighbor status to to closer redundant link %d\n", checknode );
					m_NeighborsTable[pNode->m_iID].Clear(testnode);
				}
			}
		}
	}
	
	AI_PROFILE_SCOPE_END();

	m_DidSetNeighborsTable.Set(pNode->m_iID);
}

//-----------------------------------------------------------------------------
// Purpose: For the current node, check its connection to all other nodes
// Input  :
// Output :
//-----------------------------------------------------------------------------

static bool IsInLineForClimb( const Vector &srcPos, const Vector &srcFacing, const Vector &destPos, const Vector &destFacing )
{
#ifdef DEBUG
	Vector normSrcFacing( srcFacing ), normDestFacing( destFacing );

	VectorNormalize( normSrcFacing );
	VectorNormalize( normDestFacing );

	Assert( VectorsAreEqual( srcFacing, normSrcFacing, 0.01 ) && VectorsAreEqual( destFacing, normDestFacing, 0.01 ) );
#endif

	// If they are not facing the same way...
	if ( 1 - srcFacing.Dot( destFacing ) > 0.01 )
		return false;

	// If they aren't in line along the facing...
	if ( CalcDistanceToLine2D( destPos.AsVector2D(), srcPos.AsVector2D(), srcPos.AsVector2D() + srcFacing.AsVector2D() ) > 0.01 )
		return false;
		
	// Check that the angle between them is either staight up, or on at angle of ladder-stairs
	Vector vecDelta = srcPos - destPos;

	VectorNormalize( vecDelta );

	float fabsCos = fabs( srcFacing.Dot( vecDelta ) );

	const float CosAngLadderStairs = 0.4472; // rise 2 & run 1

	if ( fabsCos > 0.05 && fabs( fabsCos - CosAngLadderStairs ) > 0.05 )
		return false;

	// *************************** --------------------------------
	return true;
}

//-------------------------------------

int CAI_NetworkBuilder::ComputeConnection( CAI_Node *pSrcNode, CAI_Node *pDestNode, Hull_t hull )
{
	int srcId = pSrcNode->m_iID;
	int destId = pDestNode->m_iID;
	int result = 0;
	trace_t tr;
	
	// Set the size of the test hull
	if ( m_pTestHull->GetHullType() != hull ) 
	{
		m_pTestHull->SetHullType( hull );
		m_pTestHull->SetHullSizeNormal( true );
	}

	if ( !( m_pTestHull->GetFlags() & FL_ONGROUND ) )
	{
		DevWarning( 2, "OFFGROUND!\n" );
	}
	m_pTestHull->AddFlag( FL_ONGROUND );

	// ==============================================================
	// FIRST CHECK IF HULL CAN EVEN FIT AT THESE NODES
	// ==============================================================
	// @Note (toml 02-10-03): this should be optimized, caching the results of CanFitAtNode() 
	if ( !( pSrcNode->m_eNodeInfo & ( HullToBit( hull ) << NODE_ENT_FLAGS_SHIFT ) ) &&
		 !m_pTestHull->GetNavigator()->CanFitAtNode(srcId,MASK_NPCWORLDSTATIC) )
	{
		DebugConnectMsg( srcId, destId, "      Cannot fit at node %d\n", srcId );
		return 0;
	}
	
	if (  !( pDestNode->m_eNodeInfo & ( HullToBit( hull ) << NODE_ENT_FLAGS_SHIFT ) ) &&
		 !m_pTestHull->GetNavigator()->CanFitAtNode(destId,MASK_NPCWORLDSTATIC) )
	{
		DebugConnectMsg( srcId, destId, "      Cannot fit at node %d\n", destId );
		return 0;
	}
	
	// ==============================================================
	// AIR NODES (FLYING)
	// ==============================================================
	if (pSrcNode->m_eNodeType == NODE_AIR || pDestNode->GetType() == NODE_AIR) 
	{
		AI_PROFILE_SCOPE( CAI_Node_InitLinks_Air );

		// Air nodes only connect to other air nodes and nothing else
		if (pSrcNode->m_eNodeType == NODE_AIR && pDestNode->GetType() == NODE_AIR)
		{
			AI_TraceHull( pSrcNode->GetOrigin(), pDestNode->GetOrigin(), NAI_Hull::Mins(hull),NAI_Hull::Maxs(hull), MASK_NPCWORLDSTATIC, m_pTestHull, COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{
				result |= bits_CAP_MOVE_FLY;
				DebugConnectMsg( srcId, destId, "      Connect by flying\n" );
			}
		}
	}
	// =============================================================================
	// > CLIMBING
	// =============================================================================
	// If both are climb nodes just make sure they are above each other
	// and there is room for the hull to pass between them
	else if ((pSrcNode->m_eNodeType == NODE_CLIMB) && (pDestNode->GetType() == NODE_CLIMB))
	{
		AI_PROFILE_SCOPE( CAI_Node_InitLinks_Climb );

		Vector srcPos	 = pSrcNode->GetPosition(hull);
		Vector destPos	 = pDestNode->GetPosition(hull);
		
		// If a code genereted climb dismount node the two origins will be the same
		if (pSrcNode->GetOrigin() == pDestNode->GetOrigin())
		{
			AI_TraceHull( srcPos, destPos, 
							NAI_Hull::Mins(hull),NAI_Hull::Maxs(hull), 
							MASK_NPCWORLDSTATIC, m_pTestHull, COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{
				result |= bits_CAP_MOVE_CLIMB;
				DebugConnectMsg( srcId, destId, "      Connect by climbing\n" );
			}
		}
		else
		{
			if ( !IsInLineForClimb(srcPos, UTIL_YawToVector( pSrcNode->m_flYaw ), destPos, UTIL_YawToVector( pDestNode->m_flYaw ) ) )
			{
				Assert( !IsInLineForClimb(destPos, UTIL_YawToVector( pDestNode->m_flYaw ), srcPos, UTIL_YawToVector( pSrcNode->m_flYaw ) ) );
				DebugConnectMsg( srcId, destId, "      Not lined up for proper climbing\n" );
				return 0;
			}

			AI_TraceHull( srcPos, destPos, NAI_Hull::Mins(hull),NAI_Hull::Maxs(hull), MASK_NPCWORLDSTATIC, m_pTestHull, COLLISION_GROUP_NONE, &tr );
			if (!tr.startsolid && tr.fraction == 1.0)
			{
				result |= bits_CAP_MOVE_CLIMB;
				DebugConnectMsg( srcId, destId, "      Connect by climbing\n" );
			}
		}
	}
	// ====================================================
	// > TWO LAND NODES
	// =====================================================	
	else if ((pSrcNode->m_eNodeType == NODE_GROUND) || (pDestNode->GetType() == NODE_GROUND))
	{
		// BUG: this could use GroundMoveLimit, except there's no version of world but not brushes (doors open, etc).
		
		// ====================================================
		// > WALKING : walk the space between the nodes
		// =====================================================

		// in this loop we take tiny steps from the current node to the nodes that it links to, one at a time.
		bool fStandFailed = false;
		bool fWalkFailed = true;

		AI_PROFILE_SCOPE_BEGIN( CAI_Node_InitLinks_Ground );

		Vector srcPos	 = pSrcNode->GetPosition(hull);
		Vector destPos	 = pDestNode->GetPosition(hull);

		if (!m_pTestHull->GetMoveProbe()->CheckStandPosition( srcPos, MASK_NPCWORLDSTATIC))
		{
			DebugConnectMsg( srcId, destId, "      Failed to stand at %d\n", srcId );
			fStandFailed = true;
		}

		if (!m_pTestHull->GetMoveProbe()->CheckStandPosition( destPos, MASK_NPCWORLDSTATIC))
		{
			DebugConnectMsg( srcId, destId, "      Failed to stand at %d\n", destId );
			fStandFailed = true;
		}

		//if (hull == 0)
		//	DevMsg("from %.1f %.1f %.1f to %.1f %.1f %.1f\n", srcPos.x, srcPos.y, srcPos.z, destPos.x, destPos.y, destPos.z );

		if ( !fStandFailed )
		{
			fWalkFailed = !m_pTestHull->GetMoveProbe()->TestGroundMove( srcPos, destPos, MASK_NPCWORLDSTATIC, AITGM_IGNORE_INITIAL_STAND_POS, NULL );
			if ( fWalkFailed )
				DebugConnectMsg( srcId, destId, "      Failed to walk between nodes\n" );
		}

		// Add to our list of accepable hulls
		if (!fWalkFailed && !fStandFailed)
		{
			result |= bits_CAP_MOVE_GROUND;
			DebugConnectMsg( srcId, destId, "      Nodes connect for ground movement\n" );
		}
	
		AI_PROFILE_SCOPE_END();

		// =============================================================================
		// > JUMPING : jump the space between the nodes, but only if walk failed
		// =============================================================================
		if (!fStandFailed && fWalkFailed && (pSrcNode->m_eNodeType == NODE_GROUND) && (pDestNode->GetType() == NODE_GROUND))
		{
			AI_PROFILE_SCOPE( CAI_Node_InitLinks_Jump );

			Vector srcPos	 = pSrcNode->GetPosition(hull);
			Vector destPos	 = pDestNode->GetPosition(hull);

			// Jumps aren't bi-directional.  We can jump down further than we can jump up so
			// we have to test for either one
			bool canDestJump = m_pTestHull->IsJumpLegal(srcPos, destPos, destPos);
			bool canSrcJump  = m_pTestHull->IsJumpLegal(destPos, srcPos, srcPos);

			if (canDestJump || canSrcJump) 
			{
				CAI_MoveProbe *pMoveProbe = m_pTestHull->GetMoveProbe();

				bool fJumpLegal = false;
				m_pTestHull->SetGravity(1.0);

				AIMoveTrace_t moveTrace;
				pMoveProbe->MoveLimit( NAV_JUMP, srcPos,destPos, MASK_NPCWORLDSTATIC, NULL, &moveTrace);
				if (!IsMoveBlocked(moveTrace))
				{
					fJumpLegal = true;
				}
				pMoveProbe->MoveLimit( NAV_JUMP, destPos,srcPos, MASK_NPCWORLDSTATIC, NULL, &moveTrace);
				if (!IsMoveBlocked(moveTrace))
				{
					fJumpLegal = true;
				}
				
				// Add to our list of accepable hulls
				if (fJumpLegal)
				{
					result |= bits_CAP_MOVE_JUMP;
					DebugConnectMsg( srcId, destId, "      Nodes connect for jumping\n" );
				}
			}
		}
	}
	return result;
}



//-------------------------------------

void CAI_NetworkBuilder::InitLinks(CAI_Network *pNetwork, CAI_Node *pNode)
{
	AI_PROFILE_SCOPE( CAI_Node_InitLinks );

	// -----------------------------------------------------
	// Get test hull
	// -----------------------------------------------------
	m_pTestHull->GetNavigator()->SetNetwork( pNetwork );

	// -----------------------------------------------------
	// Initialize links to every node 
	// -----------------------------------------------------
	for (int i = 0; i < pNetwork->NumNodes(); i++ )
  	{
		// -------------------------------------------------
		//  Check for redundant link building
		// -------------------------------------------------
		DebugConnectMsg( pNode->m_iID, i, "Testing connection between %d and %d:\n", pNode->m_iID, i );
		
		if (pNode->HasLink(i))
		{
			// A link has been already created when the other node was processed...
			DebugConnectMsg( pNode->m_iID, i, "   Nodes already connected\n" );
			continue;
		}

		// ---------------------------------------------------------------------
		// If link has been already created in other node just share it
		// ---------------------------------------------------------------------
		CAI_Node *pDestNode = pNetwork->GetNode( i );
		
		CAI_Link *pOldLink = pDestNode->HasLink(pNode->m_iID);
		if (pOldLink)
		{
			DebugConnectMsg( pNode->m_iID, i, "   Sharing previously establish connection\n" );
			((CAI_Node *)pNode)->AddLink(pOldLink);
			continue;
		}

		// Only check if the node is a neighbor
		if ( m_NeighborsTable[pNode->m_iID].IsBitSet(pDestNode->m_iID) ) 
		{
			int acceptedMotions[NUM_HULLS];

			bool bAllFailed = true;

			if ( DebuggingConnect( pNode->m_iID, i ) )
			{
				DevMsg( " " ); // break here..
			}

			if ( !(pNode->m_eNodeInfo & bits_NODE_FALLEN) && !(pDestNode->m_eNodeInfo & bits_NODE_FALLEN) )
			{
				for (int hull = 0 ; hull < NUM_HULLS; hull++ )
				{
					DebugConnectMsg( pNode->m_iID, i, "   Testing for hull %s\n", NAI_Hull::Name( (Hull_t)hull  ) );
					
					acceptedMotions[hull] = ComputeConnection( pNode, pDestNode, (Hull_t)hull );
					if ( acceptedMotions[hull] != 0 )
						bAllFailed = false;
				}
			}
			else
				DebugConnectMsg( pNode->m_iID, i, "   No connection: one or both are fallen nodes\n" );

			// If there were any passible hulls create link
			if (!bAllFailed) 
			{
				CAI_Link *pLink = pNetwork->CreateLink( pNode->m_iID, pDestNode->m_iID);
				if ( pLink )
				{
					for (int hull=0;hull<NUM_HULLS;hull++)
					{
						pLink->m_iAcceptedMoveTypes[hull] = acceptedMotions[hull];
					}
					DebugConnectMsg( pNode->m_iID, i, "   Added link\n" );
				}
			}
			else 
			{
				m_NeighborsTable[pNode->m_iID].Clear(pDestNode->m_iID);
				DebugConnectMsg(pNode->m_iID, i, "   NO LINK\n" );
			}
		}
		else
			DebugConnectMsg( pNode->m_iID, i, "   NO LINK (not neighbors)\n" );
	}
}

//-----------------------------------------------------------------------------
