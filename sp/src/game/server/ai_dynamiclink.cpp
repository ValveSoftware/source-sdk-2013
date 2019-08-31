//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		A link that can be turned on and off.  Unlike normal links
//				dyanimc links must be entities so they can receive messages.
//				They update the state of the actual links.  Allows us to save
//				a lot of memory by not making all links into entities
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "collisionutils.h"
#include "ai_dynamiclink.h"
#include "ai_node.h"
#include "ai_link.h"
#include "ai_network.h"
#include "ai_networkmanager.h"
#ifdef MAPBASE
#include "ai_hint.h"
#include "ai_basenpc.h"
#include "filters.h"
#include "point_template.h"
#include "TemplateEntities.h"
#include "mapentities.h"
#endif
#include "saverestore_utlvector.h"
#include "editor_sendcommand.h"
#include "bitstring.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(info_node_link_controller, CAI_DynamicLinkController);

BEGIN_DATADESC( CAI_DynamicLinkController )

	DEFINE_KEYFIELD( m_nLinkState, FIELD_INTEGER, "initialstate" ),
	DEFINE_KEYFIELD( m_strAllowUse, FIELD_STRING, "AllowUse" ),
	DEFINE_KEYFIELD( m_bInvertAllow, FIELD_BOOLEAN, "InvertAllow" ),
	DEFINE_KEYFIELD( m_bUseAirLinkRadius, FIELD_BOOLEAN, "useairlinkradius" ),
	//				 m_ControlledLinks (rebuilt)

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetAllowed", InputSetAllowed ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetInvert", InputSetInvert ),

END_DATADESC()

void CAI_DynamicLinkController::GenerateLinksFromVolume()
{
	Assert( m_ControlledLinks.Count() == 0 );

	int nNodes = g_pBigAINet->NumNodes();
	CAI_Node **ppNodes = g_pBigAINet->AccessNodes();

	float MinDistCareSq = 0;
	if (m_bUseAirLinkRadius)
	{
		 MinDistCareSq = Square(MAX_AIR_NODE_LINK_DIST + 0.1);
	}
	else
	{
		 MinDistCareSq = Square(MAX_NODE_LINK_DIST + 0.1);
	}

	const Vector &origin = WorldSpaceCenter();
	Vector vAbsMins, vAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vAbsMins, &vAbsMaxs );
	vAbsMins -= Vector( 1, 1, 1 );
	vAbsMaxs += Vector( 1, 1, 1 );

	for ( int i = 0; i < nNodes; i++ )
	{
		CAI_Node *pNode = ppNodes[i];
		const Vector &nodeOrigin = pNode->GetOrigin();
		if ( origin.DistToSqr(nodeOrigin) < MinDistCareSq )
		{
			int nLinks = pNode->NumLinks();
			for ( int j = 0; j < nLinks; j++ )
			{
				CAI_Link *pLink = pNode->GetLinkByIndex( j );
				int iLinkDest = pLink->DestNodeID( i );
				if ( iLinkDest > i )
				{
					const Vector &originOther = ppNodes[iLinkDest]->GetOrigin();
					if ( origin.DistToSqr(originOther) < MinDistCareSq )
					{
						if ( IsBoxIntersectingRay( vAbsMins, vAbsMaxs, nodeOrigin, originOther - nodeOrigin ) )
						{
							Assert( IsBoxIntersectingRay( vAbsMins, vAbsMaxs, originOther, nodeOrigin - originOther ) );

							CAI_DynamicLink *pLink = (CAI_DynamicLink *)CreateEntityByName( "info_node_link" );
							pLink->m_nSrcID = i;
							pLink->m_nDestID = iLinkDest;
							pLink->m_nSrcEditID = g_pAINetworkManager->GetEditOps()->GetWCIdFromNodeId( pLink->m_nSrcID );
							pLink->m_nDestEditID = g_pAINetworkManager->GetEditOps()->GetWCIdFromNodeId( pLink->m_nDestID );
							pLink->m_nLinkState = m_nLinkState;
							pLink->m_strAllowUse = m_strAllowUse;
							pLink->m_bInvertAllow = m_bInvertAllow;
							pLink->m_bFixedUpIds = true;
							pLink->m_bNotSaved = true;

							pLink->Spawn();
							m_ControlledLinks.AddToTail( pLink );
						}
					}
				}
			}
		}
	}
}

void CAI_DynamicLinkController::InputTurnOn( inputdata_t &inputdata )
{
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->InputTurnOn( inputdata );
	}

	m_nLinkState = LINK_ON;
}

void CAI_DynamicLinkController::InputTurnOff( inputdata_t &inputdata )
{
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->InputTurnOff( inputdata );
	}

	m_nLinkState = LINK_OFF;
}

void CAI_DynamicLinkController::InputSetAllowed( inputdata_t &inputdata )
{
	m_strAllowUse = inputdata.value.StringID();
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->m_strAllowUse = m_strAllowUse;
	}
}

void CAI_DynamicLinkController::InputSetInvert( inputdata_t &inputdata )
{
	m_bInvertAllow = inputdata.value.Bool();
	for ( int i = 0; i < m_ControlledLinks.Count(); i++ )
	{
		if ( m_ControlledLinks[i] == NULL )
		{
			m_ControlledLinks.FastRemove(i);
			if ( i >= m_ControlledLinks.Count() )
				break;
		}
		m_ControlledLinks[i]->m_bInvertAllow = m_bInvertAllow;
	}
}

#ifdef MAPBASE
//=============================================================================
//	>> CAI_CustomLinkController
//	Uses the specified link class
//=============================================================================
class CAI_CustomLinkController : public CAI_DynamicLinkController
{
	DECLARE_CLASS( CAI_CustomLinkController, CAI_DynamicLinkController );
public:
	CAI_CustomLinkController();

	void GenerateLinksFromVolume();
	int GetReferenceLinkIndex();
	
	string_t					m_iszReferenceLinkTemplate;
	int							m_iReferenceLink;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(info_template_link_controller, CAI_CustomLinkController);

BEGIN_DATADESC( CAI_CustomLinkController )

	DEFINE_KEYFIELD( m_iszReferenceLinkTemplate, FIELD_STRING, "ReferenceTemplate" ),
	//DEFINE_FIELD( m_iReferenceLink, FIELD_INTEGER ), // I don't know if this should be saved. It's only a cached variable, so not saving it shouldn't hurt anything.

END_DATADESC()

CAI_CustomLinkController::CAI_CustomLinkController()
{
	m_iReferenceLink = -1;
}

int CAI_CustomLinkController::GetReferenceLinkIndex()
{
	if (m_iReferenceLink != -1)
		return m_iReferenceLink;

	CBaseEntity *pEnt =  gEntList.FindEntityByName(NULL, STRING(m_iszReferenceLinkTemplate), this);
	if (CPointTemplate *pTemplate = dynamic_cast<CPointTemplate*>(pEnt))
	{
		Assert(pTemplate->GetTemplateEntity(0));

		m_iReferenceLink = pTemplate->GetTemplateIndexForTemplate(0);
		return m_iReferenceLink;
	}

	return -1;
}

void CAI_CustomLinkController::GenerateLinksFromVolume()
{
	Assert( m_ControlledLinks.Count() == 0 );

	int nNodes = g_pBigAINet->NumNodes();
	CAI_Node **ppNodes = g_pBigAINet->AccessNodes();

	float MinDistCareSq = 0;
	if (m_bUseAirLinkRadius)
	{
		 MinDistCareSq = Square(MAX_AIR_NODE_LINK_DIST + 0.1);
	}
	else
	{
		 MinDistCareSq = Square(MAX_NODE_LINK_DIST + 0.1);
	}

	const Vector &origin = WorldSpaceCenter();
	Vector vAbsMins, vAbsMaxs;
	CollisionProp()->WorldSpaceAABB( &vAbsMins, &vAbsMaxs );
	vAbsMins -= Vector( 1, 1, 1 );
	vAbsMaxs += Vector( 1, 1, 1 );

	int iReference = GetReferenceLinkIndex();
	if (iReference == -1)
	{
		Warning("WARNING! %s reference link is invalid!\n", GetDebugName());
		return;
	}

	// Get the map data before the loop
	char *pMapData = (char*)STRING( Templates_FindByIndex( iReference ) );

	// Make sure the entity is a dynamic link before doing anything
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, pMapData, NULL );
	if ( !dynamic_cast<CAI_DynamicLink*>(pEntity) )
	{
		Warning("WARNING! %s reference link is not a node link!\n", GetDebugName());
		UTIL_RemoveImmediate(pEntity);
		return;
	}

	UTIL_RemoveImmediate(pEntity);

	for ( int i = 0; i < nNodes; i++ )
	{
		CAI_Node *pNode = ppNodes[i];
		const Vector &nodeOrigin = pNode->GetOrigin();
		if ( origin.DistToSqr(nodeOrigin) < MinDistCareSq )
		{
			int nLinks = pNode->NumLinks();
			for ( int j = 0; j < nLinks; j++ )
			{
				CAI_Link *pLink = pNode->GetLinkByIndex( j );
				int iLinkDest = pLink->DestNodeID( i );
				if ( iLinkDest > i )
				{
					const Vector &originOther = ppNodes[iLinkDest]->GetOrigin();
					if ( origin.DistToSqr(originOther) < MinDistCareSq )
					{
						if ( IsBoxIntersectingRay( vAbsMins, vAbsMaxs, nodeOrigin, originOther - nodeOrigin ) )
						{
							Assert( IsBoxIntersectingRay( vAbsMins, vAbsMaxs, originOther, nodeOrigin - originOther ) );

							CBaseEntity *pEntity = NULL;

							// Create the entity from the mapdata
							MapEntity_ParseEntity( pEntity, pMapData, NULL );
							if ( pEntity == NULL )
							{
								Msg("%s failed to initialize templated link with mapdata: %s\n", GetDebugName(), pMapData );
								return;
							}

							// We already made sure it was an info_node_link template earlier.
							CAI_DynamicLink *pLink = static_cast<CAI_DynamicLink*>(pEntity);

							pLink->m_nSrcID = i;
							pLink->m_nDestID = iLinkDest;
							pLink->m_nSrcEditID = g_pAINetworkManager->GetEditOps()->GetWCIdFromNodeId( pLink->m_nSrcID );
							pLink->m_nDestEditID = g_pAINetworkManager->GetEditOps()->GetWCIdFromNodeId( pLink->m_nDestID );
							pLink->m_nLinkState = m_nLinkState;
							pLink->m_strAllowUse = m_strAllowUse;
							pLink->m_bInvertAllow = m_bInvertAllow;
							pLink->m_bFixedUpIds = true;
							pLink->m_bNotSaved = true;

							pLink->Spawn();
							m_ControlledLinks.AddToTail( pLink );
						}
					}
				}
			}
		}
	}
}
#endif

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS(info_node_link, CAI_DynamicLink);

BEGIN_DATADESC( CAI_DynamicLink )

//								m_pNextDynamicLink
DEFINE_KEYFIELD( m_nLinkState, FIELD_INTEGER, "initialstate" ),
DEFINE_KEYFIELD( m_nSrcEditID,	FIELD_INTEGER, "startnode" ),
DEFINE_KEYFIELD( m_nDestEditID,	FIELD_INTEGER, "endnode" ),
DEFINE_KEYFIELD( m_nLinkType, FIELD_INTEGER, "linktype" ),
DEFINE_FIELD( m_bInvertAllow, FIELD_BOOLEAN ),
//				m_nSrcID (rebuilt)
//				m_nDestID (rebuilt)
DEFINE_KEYFIELD( m_strAllowUse, FIELD_STRING, "AllowUse" ),
//				m_bFixedUpIds (part of rebuild)
//				m_bNotSaved (rebuilt)

DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Init static variables
//-----------------------------------------------------------------------------
CAI_DynamicLink *CAI_DynamicLink::m_pAllDynamicLinks = NULL;
bool CAI_DynamicLink::gm_bInitialized;


//------------------------------------------------------------------------------

void CAI_DynamicLink::GenerateControllerLinks()
{
	CAI_DynamicLinkController *pController = NULL;
	while ( ( pController = gEntList.NextEntByClass( pController ) ) != NULL )
	{
		pController->GenerateLinksFromVolume();
	}

}

//------------------------------------------------------------------------------
// Purpose : Initializes src and dest IDs for all dynamic links
//			 	
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InitDynamicLinks(void)
{
	if (!g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable)
	{
		Warning("ERROR: Trying initialize links with no WC ID table!\n");
		return;
	}

	if ( gm_bInitialized )
		return;

	gm_bInitialized = true;

	bool bUpdateZones = false;

	GenerateControllerLinks();

	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		// -------------------------------------------------------------
		//  First convert this links WC IDs to engine IDs
		// -------------------------------------------------------------
		if ( !pDynamicLink->m_bFixedUpIds )
		{
			int	nSrcID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nSrcEditID );
			if (nSrcID == -1)
			{
				DevMsg( "ERROR: Dynamic link source WC node %d not found\n", pDynamicLink->m_nSrcEditID );
				nSrcID = NO_NODE;
			}

			int	nDestID = g_pAINetworkManager->GetEditOps()->GetNodeIdFromWCId( pDynamicLink->m_nDestEditID );
			if (nDestID == -1)
			{
				DevMsg( "ERROR: Dynamic link dest WC node %d not found\n", pDynamicLink->m_nDestEditID );
				nDestID = NO_NODE;
			}

			pDynamicLink->m_nSrcID  = nSrcID;
			pDynamicLink->m_nDestID  = nDestID;
			pDynamicLink->m_bFixedUpIds = true;
		}

		if ( pDynamicLink->m_nSrcID != NO_NODE && pDynamicLink->m_nDestID != NO_NODE )
		{
			if (  ( pDynamicLink->GetSpawnFlags() & bits_HULL_BITS_MASK ) != 0 )
			{
				CAI_Link *pLink = pDynamicLink->FindLink();
				if ( !pLink )
				{
					CAI_Node *pNode1, *pNode2;

					pNode1 = g_pBigAINet->GetNode( pDynamicLink->m_nSrcID );
					pNode2 = g_pBigAINet->GetNode( pDynamicLink->m_nDestID );

					if ( pNode1 && pNode2 )
					{
						pLink = g_pBigAINet->CreateLink( pDynamicLink->m_nSrcID, pDynamicLink->m_nDestID );
						if ( !pLink )
							DevMsg( "Failed to create dynamic link (%d <--> %d)\n", pDynamicLink->m_nSrcEditID, pDynamicLink->m_nDestEditID );
					}

				}

				if ( pLink )
				{
					bUpdateZones = true;

					int hullBits = ( pDynamicLink->GetSpawnFlags() & bits_HULL_BITS_MASK );
					for ( int i = 0; i < NUM_HULLS; i++ )
					{
						if ( hullBits & ( 1 << i ) )
						{
							pLink->m_iAcceptedMoveTypes[i] = pDynamicLink->m_nLinkType;
						}
					}
				}
			}

			// Now set the link's state
			pDynamicLink->SetLinkState();

			// Go on to the next dynamic link
			pDynamicLink = pDynamicLink->m_pNextDynamicLink;
		}
		else
		{
			CAI_DynamicLink *pBadDynamicLink = pDynamicLink;

			// Go on to the next dynamic link
			pDynamicLink = pDynamicLink->m_pNextDynamicLink;

			UTIL_RemoveImmediate( pBadDynamicLink );
		}

	}

	if ( bUpdateZones )
	{
		g_AINetworkBuilder.InitZones( g_pBigAINet );
	}
}


//------------------------------------------------------------------------------
// Purpose : Goes through each dynamic link and updates the state of all
//			 AINetwork links	
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::ResetDynamicLinks(void)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		// Now set the link's state
		pDynamicLink->SetLinkState();

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}


//------------------------------------------------------------------------------
// Purpose : Goes through each dynamic link and checks to make sure that
//			 there is still a corresponding node link, if not removes it
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::PurgeDynamicLinks(void)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		if (!pDynamicLink->IsLinkValid())
		{
			// Didn't find the link, so remove it
#ifdef _WIN32
			int nWCSrcID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pDynamicLink->m_nSrcID];
			int nWCDstID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pDynamicLink->m_nDestID];
			int	status	 = Editor_DeleteNodeLink(nWCSrcID, nWCDstID, false);
			if (status == Editor_BadCommand)
			{
				DevMsg( "Worldcraft failed in PurgeDynamicLinks...\n" );
			}
#endif
			// Safe to remove it here as this happens only after I leave this function
			UTIL_Remove(pDynamicLink);
		}

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
}

//------------------------------------------------------------------------------
// Purpose : Returns false if the dynamic link doesn't have a corresponding
//			 node link
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLink::IsLinkValid( void )
{
	CAI_Node *pNode = g_pBigAINet->GetNode(m_nSrcID);

	return ( pNode->GetLink( m_nDestID ) != NULL );
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InputTurnOn( inputdata_t &inputdata )
{
	if (m_nLinkState == LINK_OFF)
	{
		m_nLinkState = LINK_ON;
		CAI_DynamicLink::SetLinkState();
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAI_DynamicLink::InputTurnOff( inputdata_t &inputdata )
{
	if (m_nLinkState == LINK_ON)
	{
		m_nLinkState = LINK_OFF;
		CAI_DynamicLink::SetLinkState();
	}
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CAI_Link *CAI_DynamicLink::FindLink()
{
	CAI_Node *	pSrcNode = g_pBigAINet->GetNode(m_nSrcID, false);
	if ( pSrcNode )
	{
		int	numLinks = pSrcNode->NumLinks();
		for (int i=0;i<numLinks;i++)
		{
			CAI_Link* pLink = pSrcNode->GetLinkByIndex(i);

			if (((pLink->m_iSrcID  == m_nSrcID )&&
				(pLink->m_iDestID == m_nDestID))   ||

				((pLink->m_iSrcID  == m_nDestID)&&
				(pLink->m_iDestID == m_nSrcID ))   )
			{
				return pLink;
			}
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int CAI_DynamicLink::ObjectCaps()
{
	int caps = BaseClass::ObjectCaps();

	if ( m_bNotSaved )
		caps |= FCAP_DONT_SAVE;

	return caps;
}

//------------------------------------------------------------------------------
// Purpose : Updates network link state if dynamic link state has changed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLink::SetLinkState(void)
{
	if ( !gm_bInitialized )
	{
		// Safe to quietly return. Consistency will be enforced when InitDynamicLinks() is called
		return;
	}

	if (m_nSrcID == NO_NODE || m_nDestID == NO_NODE)
	{
		Vector pos = GetAbsOrigin();
		DevWarning("ERROR: Dynamic link at %f %f %f pointing to invalid node ID!!\n", pos.x, pos.y, pos.z);
		return;
	}

	// ------------------------------------------------------------------
	// Now update the node links...
	//  Nodes share links so we only have to find the node from the src 
	//  For now just using one big AINetwork so find src node on that network
	// ------------------------------------------------------------------
	CAI_Node *	pSrcNode = g_pBigAINet->GetNode(m_nSrcID, false);
	if ( pSrcNode )
	{
		CAI_Link* pLink = FindLink();
		if ( pLink )
		{
			pLink->m_pDynamicLink = this;
			if (m_nLinkState == LINK_OFF)
			{
				pLink->m_LinkInfo |=  bits_LINK_OFF;
			}
			else
			{
				pLink->m_LinkInfo &= ~bits_LINK_OFF;
			}
		}
		else
		{
			DevMsg("Dynamic Link Error: (%s) unable to form between nodes %d and %d\n", GetDebugName(), m_nSrcID, m_nDestID );
		}
	}

}

//------------------------------------------------------------------------------
// Purpose : Given two node ID's return the related dynamic link if any or NULL
//			 	
// Input   :
// Output  :
//------------------------------------------------------------------------------
CAI_DynamicLink* CAI_DynamicLink::GetDynamicLink(int nSrcID, int nDstID)
{
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;

	while (pDynamicLink)
	{
		if ((nSrcID == pDynamicLink->m_nSrcID  && nDstID == pDynamicLink->m_nDestID) ||
			(nSrcID == pDynamicLink->m_nDestID && nDstID == pDynamicLink->m_nSrcID ) ) 
		{
			return pDynamicLink;
		}

		// Go on to the next dynamic link
		pDynamicLink = pDynamicLink->m_pNextDynamicLink;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_DynamicLink::CAI_DynamicLink(void)
{
	m_bFixedUpIds		= false;
	m_bNotSaved			= false;
	m_nSrcID			= NO_NODE;
	m_nDestID			= NO_NODE;
	m_nLinkState		= LINK_OFF;
	m_nLinkType			= bits_CAP_MOVE_GROUND;
	m_bInvertAllow		= false;

	// -------------------------------------
	//  Add to linked list of dynamic links
	// -------------------------------------
	m_pNextDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;
	CAI_DynamicLink::m_pAllDynamicLinks = this;
};


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_DynamicLink::~CAI_DynamicLink(void) {

	// ----------------------------------------------
	//  Remove from linked list of all dynamic links
	// ----------------------------------------------
	CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::m_pAllDynamicLinks;
	if (pDynamicLink == this)
	{
		m_pAllDynamicLinks = pDynamicLink->m_pNextDynamicLink;
	}
	else
	{
		while (pDynamicLink)
		{
			if (pDynamicLink->m_pNextDynamicLink == this)
			{
				pDynamicLink->m_pNextDynamicLink = pDynamicLink->m_pNextDynamicLink->m_pNextDynamicLink;
				break;
			}
			pDynamicLink = pDynamicLink->m_pNextDynamicLink;
		}
	}
}

#ifdef MAPBASE
//------------------------------------------------------------------------------
// Purpose : Determines if usage is allowed by a NPC, whether the link is disabled or not.
//			This was created for info_node_link derivatives.
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLink::UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd)
{
	if (!(FindLink()->m_LinkInfo & bits_LINK_OFF))
		return true;

	if ( m_strAllowUse == NULL_STRING )
			return false;

	const char *pszAllowUse = STRING( m_strAllowUse );
	if ( m_bInvertAllow )
	{
		// Exlude only the specified entity name or classname
		if ( !pNPC->NameMatches(pszAllowUse) && !pNPC->ClassMatches( pszAllowUse ) )
			return true;
	}
	else
	{
		// Exclude everything but the allowed entity name or classname
		if ( pNPC->NameMatches( pszAllowUse) || pNPC->ClassMatches( pszAllowUse ) )
			return true;
	}

	return false;
}

//=============================================================================
//	>> CAI_DynanicLinkOneWay
//=============================================================================
class CAI_DynamicLinkOneWay : public CAI_DynamicLink
{
	DECLARE_CLASS( CAI_DynamicLinkOneWay, CAI_DynamicLink );
public:
	virtual bool			UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd);
	//virtual void			SetLinkState( void );

	bool					m_bNormalWhenEnabled;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(info_node_link_oneway, CAI_DynamicLinkOneWay);

BEGIN_DATADESC( CAI_DynamicLinkOneWay )

	DEFINE_KEYFIELD( m_bNormalWhenEnabled, FIELD_BOOLEAN, "Usage" ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Determines if usage is allowed by a NPC.
//			This was created for info_node_link derivatives.
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLinkOneWay::UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd)
{
	if (m_bNormalWhenEnabled)
		return (m_nLinkState == LINK_OFF && bFromEnd) ? BaseClass::UseAllowed(pNPC, bFromEnd) : true;

	if (bFromEnd || m_nLinkState == LINK_OFF)
		return BaseClass::UseAllowed(pNPC, bFromEnd);

	return true;
}

#if 0
//------------------------------------------------------------------------------
// Purpose : Updates network link state if dynamic link state has changed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAI_DynamicLinkOneWay::SetLinkState(void)
{
	if (m_bNormalWhenEnabled)
		return BaseClass::SetLinkState();

	if ( !gm_bInitialized )
	{
		// Safe to quietly return. Consistency will be enforced when InitDynamicLinks() is called
		return;
	}

	if (m_nSrcID == NO_NODE || m_nDestID == NO_NODE)
	{
		Vector pos = GetAbsOrigin();
		DevWarning("ERROR: Dynamic link at %f %f %f pointing to invalid node ID!!\n", pos.x, pos.y, pos.z);
		return;
	}

	CAI_Node *	pSrcNode = g_pBigAINet->GetNode(m_nSrcID, false);
	if ( pSrcNode )
	{
		CAI_Link* pLink = FindLink();
		if ( pLink )
		{
			// One-way always registers as off so it always calls UseAllowed()
			pLink->m_pDynamicLink = this;
			pLink->m_LinkInfo |= bits_LINK_OFF;
		}
		else
		{
			DevMsg("Dynamic Link Error: (%s) unable to form between nodes %d and %d\n", GetDebugName(), m_nSrcID, m_nDestID );
		}
	}
}
#endif

//=============================================================================
//	>> CAI_DynamicLinkFilter
//=============================================================================
class CAI_DynamicLinkFilter : public CAI_DynamicLink
{
	DECLARE_CLASS( CAI_DynamicLinkFilter, CAI_DynamicLink );
public:
	virtual bool			UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd);
	//virtual void			SetLinkState( void );

	bool					m_bNormalWhenEnabled;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(info_node_link_filtered, CAI_DynamicLinkFilter);

BEGIN_DATADESC( CAI_DynamicLinkFilter )

	DEFINE_KEYFIELD( m_bNormalWhenEnabled, FIELD_BOOLEAN, "Usage" ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetLinkFilter", InputSetDamageFilter ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Determines if usage is allowed by a NPC.
//			This was created for info_node_link derivatives.
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLinkFilter::UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd)
{
	if ( !m_hDamageFilter )
	{
		m_hDamageFilter = gEntList.FindEntityByName( NULL, m_iszDamageFilterName );
		if (!m_hDamageFilter)
		{
			Warning("%s (%s) couldn't find filter \"%s\"!\n", GetClassname(), GetDebugName(), STRING(m_iszDamageFilterName));
			return BaseClass::UseAllowed(pNPC, bFromEnd);
		}
	}

	CBaseFilter *pFilter = (CBaseFilter *)(m_hDamageFilter.Get());

	if (m_bNormalWhenEnabled)
		return (m_nLinkState == LINK_OFF) ? (pFilter->PassesFilter(this, pNPC) || BaseClass::UseAllowed(pNPC, bFromEnd)) : true;

	if (m_nLinkState == LINK_OFF)
		return BaseClass::UseAllowed(pNPC, bFromEnd);

	return pFilter->PassesFilter(this, pNPC);
}

//=============================================================================
//	>> CAI_DynamicLinkLogic
//=============================================================================
class CAI_DynamicLinkLogic : public CAI_DynamicLink
{
	DECLARE_CLASS( CAI_DynamicLinkLogic, CAI_DynamicLink );
public:
	virtual bool			UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd);
	virtual bool			FinalUseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd);

	COutputEvent			m_OnUsageAccepted;
	COutputEvent			m_OnUsageAcceptedWhileDisabled;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(info_node_link_logic, CAI_DynamicLinkLogic);

BEGIN_DATADESC( CAI_DynamicLinkLogic )

	DEFINE_OUTPUT( m_OnUsageAccepted, "OnUsageAccepted" ),
	DEFINE_OUTPUT( m_OnUsageAcceptedWhileDisabled, "OnUsageAcceptedWhileDisabled" ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose : Determines if usage is allowed by a NPC.
//			This was created for info_node_link derivatives.
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLinkLogic::UseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd)
{
	// 
	// If the link is off, we want to fire "OnUsageAcceptedWhileDisabled", but we have to make sure
	// the rest of the pathfinding calculations work. Yes, they might do all of this just to find a disabled link,
	// but we have to fire the output somehow.
	// 
	// Links already enabled go through regular usage rules.
	// 
	if (m_nLinkState == LINK_OFF)
		return true;
	else
		return BaseClass::UseAllowed( pNPC, bFromEnd );
}

//------------------------------------------------------------------------------
// Purpose : After nothing else is left, finally determines if usage is allowed by a NPC.
//			This was created for info_node_link derivatives.
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_DynamicLinkLogic::FinalUseAllowed(CAI_BaseNPC *pNPC, bool bFromEnd)
{
	if (m_nLinkState == LINK_ON)
	{
		m_OnUsageAccepted.FireOutput(pNPC, this);
		return true;
	}
	else
	{
		m_OnUsageAcceptedWhileDisabled.FireOutput(pNPC, this);

		// We skipped the usage rules before. Do them now.
		return BaseClass::UseAllowed(pNPC, bFromEnd);
	}
}
#endif

LINK_ENTITY_TO_CLASS(info_radial_link_controller, CAI_RadialLinkController);

BEGIN_DATADESC( CAI_RadialLinkController )
DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
DEFINE_FIELD( m_vecAtRestOrigin, FIELD_POSITION_VECTOR ),
DEFINE_FIELD( m_bAtRest, FIELD_BOOLEAN ),

DEFINE_THINKFUNC( PollMotionThink ),
END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_RadialLinkController::Spawn()
{
	SetSolid( SOLID_NONE );
	AddEffects( EF_NODRAW );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_RadialLinkController::Activate()
{
	BaseClass::Activate();

	m_bAtRest = false;
	m_vecAtRestOrigin = vec3_invalid;

	// Force re-evaluation
	SetThink( &CAI_RadialLinkController::PollMotionThink );

	// Spread think times out.
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.0f, 1.0f) );

	if( GetParent() != NULL )
	{
		float flDist = GetAbsOrigin().DistTo( GetParent()->GetAbsOrigin() );

		if( flDist > 200.0f )
		{
			// Warn at the console if a link controller is far away from its parent. This
			// most likely means that a level designer has copied an entity without researching its hierarchy.
			DevMsg("RadialLinkController (%s) is far from its parent!\n", GetDebugName() );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_RadialLinkController::PollMotionThink()
{
	SetNextThink( gpGlobals->curtime + 0.5f );

	CBaseEntity *pParent = GetParent();

	if( pParent )
	{
		if( pParent->VPhysicsGetObject()->IsAsleep() )
		{
			if( !m_bAtRest )
			{
				m_vecAtRestOrigin = GetAbsOrigin();
				ModifyNodeLinks( true );
				m_bAtRest = true;
				//Msg("At Rest!\n");
			}
		}
		else
		{
			if( m_bAtRest )
			{
				float flDist; 

				flDist = GetAbsOrigin().DistTo(m_vecAtRestOrigin);

				if( flDist < 18.0f )
				{
					// Ignore movement If moved less than 18 inches from the place where we came to rest.
					//Msg("Reject.\n");
					return;
				}
			}

			//Msg("Polling!\n");

			if( m_vecAtRestOrigin != vec3_invalid )
			{
				ModifyNodeLinks( false );
				m_bAtRest = false;
				m_vecAtRestOrigin = vec3_invalid;
			}
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
ConVar ai_radial_max_link_dist( "ai_radial_max_link_dist", "512" );
void CAI_RadialLinkController::ModifyNodeLinks( bool bMakeStale )
{
	int nNodes = g_pBigAINet->NumNodes();
	CAI_Node **ppNodes = g_pBigAINet->AccessNodes();

	VPROF_BUDGET("ModifyLinks", "ModifyLinks");

	const float MinDistCareSq = Square( ai_radial_max_link_dist.GetFloat() + 0.1 );

	for ( int i = 0; i < nNodes; i++ )
	{
		CAI_Node *pNode = ppNodes[i];
		const Vector &nodeOrigin = pNode->GetOrigin();
		if ( m_vecAtRestOrigin.DistToSqr(nodeOrigin) < MinDistCareSq )
		{
			int nLinks = pNode->NumLinks();
			for ( int j = 0; j < nLinks; j++ )
			{
				CAI_Link *pLink = pNode->GetLinkByIndex( j );
				int iLinkDest = pLink->DestNodeID( i );

				if ( iLinkDest > i )
				{
					bool bQualify = true;

					if( ( (pLink->m_iAcceptedMoveTypes[HULL_HUMAN]||pLink->m_iAcceptedMoveTypes[HULL_WIDE_HUMAN]) & bits_CAP_MOVE_GROUND) == 0 )
					{
						// Micro-optimization: Ignore any connection that's not a walking connection for humans.(sjb)
						bQualify = false;
					}

					const Vector &originOther = ppNodes[iLinkDest]->GetOrigin();
					if ( bQualify && m_vecAtRestOrigin.DistToSqr(originOther) < MinDistCareSq )
					{
						if ( IsRayIntersectingSphere(nodeOrigin, originOther - nodeOrigin, m_vecAtRestOrigin, m_flRadius) )
						{
							if( bMakeStale )
							{
								pLink->m_LinkInfo |= bits_LINK_STALE_SUGGESTED;
								pLink->m_timeStaleExpires = FLT_MAX;
							}
							else
							{
								pLink->m_LinkInfo &= ~bits_LINK_STALE_SUGGESTED;
							}
						}
					}
				}
			}
		}
	}
}
