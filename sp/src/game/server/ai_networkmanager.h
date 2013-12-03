//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_NETWORKMANAGER_H
#define AI_NETWORKMANAGER_H

#include "utlvector.h"
#include "bitstring.h"

#if defined( _WIN32 )
#pragma once
#endif

class CAI_NetworkEditTools;
class CAI_Network;
class CAI_Node;
class CAI_Link;
class CAI_TestHull;

//-----------------------------------------------------------------------------
// CAI_NetworkManager
//
// Purpose: The entity in the level responsible for building the network if it
//			isn't there, saving & loading of the network, and holding the 
//			CAI_Network instance.
//
//-----------------------------------------------------------------------------

class CAI_NetworkManager : public CPointEntity
{
public:
	static void		InitializeAINetworks();

	DECLARE_DATADESC();

	DECLARE_CLASS( CAI_NetworkManager, CPointEntity );

public:
	CAI_NetworkManager(void);
	virtual ~CAI_NetworkManager(void);
	
	void			Spawn ();
	virtual	int		ObjectCaps( void ) { return BaseClass::ObjectCaps() | FCAP_DONT_SAVE; }
	void			RebuildNetworkGraph();			// Used in WC edit mode
	void			StartRebuild();			// Used in WC edit mode
	void			LoadNetworkGraph();

	static bool		NetworksLoaded()	{ return gm_fNetworksLoaded; }
	bool			IsInitialized()	{ return m_fInitalized; }

	void			BuildNetworkGraph();

	static void		DeleteAllAINetworks();

	void			FixupHints();
	void			MarkDontSaveGraph();

public:
	CAI_NetworkEditTools *	GetEditOps() { return m_pEditOps; }
	CAI_Network *			GetNetwork() { return m_pNetwork; }
	
private:
	
	void			DelayedInit();
	void			RebuildThink();
	void			SaveNetworkGraph( void) ;	
	static bool		IsAIFileCurrent( const char *szMapName );		
	
	static bool				gm_fNetworksLoaded;							// Have AINetworks been loaded
	
	bool					m_bNeedGraphRebuild;					
	CAI_NetworkEditTools *	m_pEditOps;
	CAI_Network *			m_pNetwork;


	bool m_fInitalized;
	bool	m_bDontSaveGraph;
};

//-----------------------------------------------------------------------------

abstract_class CAI_NetworkBuildHelper : public CLogicalEntity
{
	DECLARE_CLASS( CAI_NetworkBuildHelper, CLogicalEntity );

public:
	virtual void PostInitNodePosition( CAI_Network *pNetwork, CAI_Node *pNode ) = 0;
};

//-----------------------------------------------------------------------------

class CAI_NetworkBuilder
{
public:
	void			Build( CAI_Network *pNetwork );
	void			Rebuild( CAI_Network *pNetwork );

	void			InitNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );

	void			InitZones( CAI_Network *pNetwork );

private:
	void			InitVisibility( CAI_Network *pNetwork, CAI_Node *pNode );
	void			InitNeighbors( CAI_Network *pNetwork, CAI_Node *pNode );
	void			InitClimbNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );
	void			InitGroundNodePosition( CAI_Network *pNetwork, CAI_Node *pNode );
	void			InitLinks( CAI_Network *pNetwork, CAI_Node *pNode );
	void			ForceDynamicLinkNeighbors();
	
	void			FloodFillZone( CAI_Node **ppNodes, CAI_Node *pNode, int zone );

	int				ComputeConnection( CAI_Node *pSrcNode, CAI_Node *pDestNode, Hull_t hull );
	
	void 			BeginBuild();
	void			EndBuild();

	CUtlVector<CVarBitVec>	m_NeighborsTable;
	CVarBitVec				m_DidSetNeighborsTable;
	CAI_TestHull *			m_pTestHull;
};

extern CAI_NetworkBuilder g_AINetworkBuilder;

//-----------------------------------------------------------------------------
// CAI_NetworkEditTools
//
// Purpose: Bridge class to Hammer node editing functionality
//
//-----------------------------------------------------------------------------

class CAI_NetworkEditTools
{
public:
	CAI_NetworkEditTools(CAI_NetworkManager *);
	~CAI_NetworkEditTools();
	// ----------------------
	//  Debug & Edit fields
	// ----------------------
	static CAI_Node *	m_pLastDeletedNode;						// For undo in wc edit mode
	static int			m_iHullDrawNum;							// Which hulls to draw
	static int			m_iVisibilityNode;						// Node I'm showing visibility for
	static int			m_iGConnectivityNode;					// Node I'm showing graph connectivity for
	static bool			m_bAirEditMode;							// Editing Air Nodes
	static bool			m_bLinkEditMode;						// Editing Links
	static float		m_flAirEditDistance;					// Distance editing Air Nodes

	static void			DrawHull(Hull_t eHull);
	static void			DrawNextHull(const char *ainet_name);			// Draws next hull set for the named ai network
	static void			SetDebugBits(const char *ainet_name,int debug_bit);

	static CAI_Node *	FindAINodeNearestFacing( const Vector &origin, const Vector &facing, float threshold, int nNodeType);
	static CAI_Link *	FindAILinkNearestFacing( const Vector &origin, const Vector &facing, float threshold);						

	//---------------
	// WC Editing 
	//---------------
	int					m_nNextWCIndex;				// Next unused index used by WC
	Vector *			m_pWCPosition;				// Array of vectors only used in wc edit mode

	//-----------------
	// Debugging Tools
	//-----------------
	int					m_debugNetOverlays;					// Which network debug overlays to draw
	void				DrawAINetworkOverlay(void);			// Draw network on the client

	void				RecalcUsableNodesForHull(void);		// Used only for debug drawing

	//-----------------
	void 				OnInit();

	int					GetNodeIdFromWCId( int nWCId );
	int					GetWCIdFromNodeId( int nNodeId );
	int *				m_pNodeIndexTable;						// Table of WC Id's to Engine Id's

	void				ClearRebuildFlags();					
	void				SetRebuildFlags();					
	void				DrawEditInfoOverlay();

#ifdef AI_PERF_MON	
	//----------------------
	// Performance stats
	//----------------------
	static	int			m_nPerfStatNN;
	static	int			m_nPerfStatPB;
	static	float		m_fNextPerfStatTime;
#endif

	CAI_NetworkManager *m_pManager;
	CAI_Network *		m_pNetwork;


};

//-----------------------------------------------------------------------------

#endif // AI_NETWORKMANAGER_H
