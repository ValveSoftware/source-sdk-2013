//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Namespace for functions having to do with WC Edit mode
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
#include "mathlib/mathlib.h"
#include "player.h"
#include "wcedit.h"
#include "ai_network.h"
#include "ai_initutils.h"
#include "ai_hull.h"
#include "ai_link.h"
#include "ai_node.h"
#include "ai_dynamiclink.h"
#include "ai_networkmanager.h"
#include "ndebugoverlay.h"
#include "editor_sendcommand.h"
#include "movevars_shared.h"
#include "model_types.h"
// UNDONE: Reduce some dependency here!
#include "physics_prop_ragdoll.h"
#include "items.h"
#include "utlsymbol.h"
#include "physobj.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CAI_Node*		FindPickerAINode( CBasePlayer* pPlayer, NodeType_e nNodeType );
extern CAI_Link*		FindPickerAILink( CBasePlayer* pPlayer );
extern float			GetFloorZ(const Vector &origin);

//-----------------------------------------------------------------------------
// Purpose: Make sure the version of the map in WC is the same as the map 
//			that's being edited 
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool NWCEdit::IsWCVersionValid(void)
{
	int status = Editor_CheckVersion(STRING(gpGlobals->mapname), gpGlobals->mapversion, false);
	if (!status)
	{
		return true;
	}
	else if (status == Editor_NotRunning)
	{
		Msg("\nAborting map_edit\nWorldcraft not running...\n\n");
		UTIL_CenterPrintAll( "Worldcraft not running..." );
		engine->ServerCommand("disconnect\n");
	}
	else
	{
		Msg("\nAborting map_edit\nWC/Engine map versions different...\n\n");
		UTIL_CenterPrintAll( "WC/Engine map versions different..." );
		engine->ServerCommand("disconnect\n");
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose : Figure out placement position of air nodes form where player is
//			 looking.  Keep distance from player constant but adjust height
//			 based on viewing angle
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector NWCEdit::AirNodePlacementPosition( void )
{
	CBasePlayer* pPlayer = UTIL_PlayerByIndex(CBaseEntity::m_nDebugPlayer);

	if (!pPlayer) 
	{
		return vec3_origin;
	}

	Vector pForward;
	pPlayer->EyeVectors( &pForward );
	
	Vector	floorVec = pForward;
	floorVec.z = 0;
	VectorNormalize( floorVec );
	VectorNormalize( pForward );

	float cosAngle = DotProduct(floorVec,pForward);

	float lookDist = g_pAINetworkManager->GetEditOps()->m_flAirEditDistance/cosAngle;
	Vector lookPos = pPlayer->EyePosition()+pForward * lookDist;

	return lookPos;
}

//-----------------------------------------------------------------------------
// Purpose: For create nodes in wc edit mode
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NWCEdit::CreateAINode( CBasePlayer *pPlayer )
{
	// -------------------------------------------------------------
	//  Check that WC is running with the right map version
	// -------------------------------------------------------------
	if ( !IsWCVersionValid() || !pPlayer )
		return;

	pPlayer->AddSolidFlags( FSOLID_NOT_SOLID );

	int hullType = g_pAINetworkManager->GetEditOps()->m_iHullDrawNum;

	// -----------------------------------
	//  Get position of node to create
	// -----------------------------------
	Vector vNewNodePos = vec3_origin;
	bool bPositionValid = false;
	if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
	{
		vNewNodePos = NWCEdit::AirNodePlacementPosition();

		// Make sure we can see the node
		trace_t tr;
		UTIL_TraceLine(pPlayer->EyePosition(), vNewNodePos, MASK_NPCSOLID_BRUSHONLY, pPlayer, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction == 1.0)
		{
			bPositionValid = true;
		}
	}
	else
	{
		// Place node by where the player is looking
		Vector forward;
		pPlayer->EyeVectors( &forward );
		Vector	startTrace	= pPlayer->EyePosition();
		Vector	endTrace	= pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH;
		trace_t	tr;
		UTIL_TraceLine(startTrace,endTrace,MASK_NPCSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			// Raise the end position up off the floor, place the node and drop him down
			tr.endpos.z += 48;
			vNewNodePos = tr.endpos;
			bPositionValid = true;
		}
	}

	// -------------------------------------------------------------------------------
	// Now check that this is a valid location for the new node bu using test hull
	// -------------------------------------------------------------------------------
	if (bPositionValid)
	{
		CBaseEntity *testHull = (CBaseEntity*)CAI_TestHull::GetTestHull();

		// Set the size of the test hull
		UTIL_SetSize(testHull, NAI_Hull::Mins(hullType), NAI_Hull::Maxs(hullType));

		// Set origin of test hull
		testHull->SetLocalOrigin( vNewNodePos );

		// -----------------------------------------------------------------------
		// If a ground node, drop to floor and make sure can stand at test postion
		// -----------------------------------------------------------------------
		if (!g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
		{
			UTIL_DropToFloor( testHull, MASK_NPCSOLID );
			vNewNodePos = testHull->GetAbsOrigin();
			CTraceFilterSimple traceFilter( testHull, COLLISION_GROUP_NONE );
			if (!UTIL_CheckBottom(testHull, &traceFilter, sv_stepsize.GetFloat()))
			{
				CAI_TestHull::ReturnTestHull();
				bPositionValid = false;
				goto DoneCreate;
			}
		}

		// -----------------------------------------------------------------------
		// Make sure hull fits at location by seeing if it can move up a fraction
		// -----------------------------------------------------------------------
		Vector vUpBit = testHull->GetAbsOrigin();
		vUpBit.z += 1;
		trace_t tr;
		UTIL_TraceHull( testHull->GetAbsOrigin(), vUpBit, NAI_Hull::Mins(hullType), 
			NAI_Hull::Maxs(hullType), MASK_NPCSOLID, testHull, COLLISION_GROUP_NONE, &tr );
		if (tr.startsolid || tr.fraction != 1.0)
		{
			CAI_TestHull::ReturnTestHull();
			bPositionValid = false;
			goto DoneCreate;
		}

		// <<TEMP>> Round position till DS fixed WC bug
		testHull->SetLocalOrigin( Vector( floor(testHull->GetAbsOrigin().x),
			floor(testHull->GetAbsOrigin().y ), floor(testHull->GetAbsOrigin().z) ) );

		// ---------------------------------------
		//  Send new node to WC
		// ---------------------------------------
		int status;
		if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
		{
			status = Editor_CreateNode("info_node_air", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, testHull->GetLocalOrigin().x, testHull->GetLocalOrigin().y, testHull->GetLocalOrigin().z, false);
		}
		else
		{
			// Create slightly higher in WC so it can be dropped when its loaded again
			Vector origin = testHull->GetLocalOrigin();
			origin.z += 24.0;
			testHull->SetLocalOrigin( origin );
			status = Editor_CreateNode("info_node", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, testHull->GetLocalOrigin().x, testHull->GetLocalOrigin().y, testHull->GetLocalOrigin().z, false);
		}
		if (status == Editor_BadCommand)
		{
			Msg( "Worldcraft failed on creation...\n" );
			CAI_TestHull::ReturnTestHull();
		}
		else if (status == Editor_OK)
		{
			// -----------------------
			// Create a new ai node
			// -----------------------
			CNodeEnt *pNodeEnt;
			if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
			{
				pNodeEnt = (CNodeEnt*)CreateEntityByName("info_node_air");
			}
			else
			{
				pNodeEnt = (CNodeEnt*)CreateEntityByName("info_node");
			}

			// Note this is a new entity being created as part of wc editing
			pNodeEnt->SetLocalOrigin( testHull->GetLocalOrigin() );
			CAI_TestHull::ReturnTestHull();

			pNodeEnt->m_NodeData.nWCNodeID =	g_pAINetworkManager->GetEditOps()->m_nNextWCIndex;

			pNodeEnt->m_debugOverlays |= OVERLAY_WC_CHANGE_ENTITY;
			pNodeEnt->Spawn();
		}	
	}

DoneCreate:
	// ----------------------------------------------------------
	// Flash a red box as a warning that the hull won't fit here
	// ----------------------------------------------------------
	if (!bPositionValid)
	{
		NDebugOverlay::Box(vNewNodePos, NAI_Hull::Mins(hullType), NAI_Hull::Maxs(hullType), 255,0,0,0,0.1);
	}

	// Restore player collidability
	pPlayer->SetSolid( SOLID_BBOX );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NWCEdit::UndoDestroyAINode(void)
{
	// -------------------------------------------------------------
	//  Check that WC is running with the right map version
	// -------------------------------------------------------------
	if (!IsWCVersionValid())
	{
		return;
	}

	if (g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode)
	{
		Vector nodePos = g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode->GetOrigin();

		int status;
		int	nOldWCID	= g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode->GetId()];
		
		if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
		{
			status = Editor_CreateNode("info_node_air", nOldWCID, nodePos.x, nodePos.y, nodePos.z, false);
		}
		else
		{
			status = Editor_CreateNode("info_node", nOldWCID, nodePos.x, nodePos.y, nodePos.z, false);
		}
		if (status == Editor_BadCommand)
		{
			Msg( "Worldcraft failed on creation...\n" );
		}
		else if (status == Editor_OK)
		{
			g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode->SetType( NODE_GROUND );
			//@ tofo g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode->m_pNetwork->BuildNetworkGraph();
			g_pAINetworkManager->BuildNetworkGraph();
			g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For destroying nodes in wc edit mode
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NWCEdit::DestroyAINode( CBasePlayer *pPlayer )
{
	// -------------------------------------------------------------
	//  Check that WC is running with the right map version
	// -------------------------------------------------------------
	if (!IsWCVersionValid())
	{
		return;
	}

	if (!pPlayer)
	{
		return;
	}

	NodeType_e nNodeType = NODE_GROUND;
	if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
	{
		nNodeType = NODE_AIR;
	}

	CAI_Node* pAINode = FindPickerAINode(pPlayer, nNodeType);
	if (pAINode)
	{
		int status = Editor_DeleteNode(g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pAINode->GetId()], false);

		if (status == Editor_BadCommand)
		{
			Msg( "Worldcraft failed on deletion...\n" );
		}
		else if (status == Editor_OK)
		{
			// Mark this node as deleted and changed
			pAINode->SetType( NODE_DELETED );
			pAINode->m_eNodeInfo   |= bits_NODE_WC_CHANGED;

			// Note that network needs to be rebuild
			g_pAINetworkManager->GetEditOps()->SetRebuildFlags();
			g_pAINetworkManager->GetEditOps()->m_pLastDeletedNode	= pAINode;

			// Now go through at delete any dynamic links that were attached to this node
			for (int link = 0; link < pAINode->NumLinks(); link++)
			{
				int nSrcID = pAINode->GetLinkByIndex(link)->m_iSrcID;
				int nDstID = pAINode->GetLinkByIndex(link)->m_iDestID;
				if (CAI_DynamicLink::GetDynamicLink(nSrcID, nDstID))
				{
					int nWCSrcID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[nSrcID];
					int nWCDstID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[nDstID];
					int	status	 = Editor_DeleteNodeLink(nWCSrcID, nWCDstID);

					if (status == Editor_BadCommand)
					{
						Msg( "Worldcraft failed on node link deletion...\n" );
					}
				}				
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For restroring links in WC edit mode.  This actually means
//			destroying links in WC that have been marked as 
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NWCEdit::CreateAILink( CBasePlayer* pPlayer )
{
	// -------------------------------------------------------------
	//  Check that WC is running with the right map version
	// -------------------------------------------------------------
	if (!IsWCVersionValid())
	{
		return;
	}

	CAI_Link* pAILink = FindPickerAILink(pPlayer);
	if (pAILink && (pAILink->m_LinkInfo & bits_LINK_OFF))
	{
		int nWCSrcID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pAILink->m_iSrcID];
		int nWCDstID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pAILink->m_iDestID];
		int	status	 = Editor_DeleteNodeLink(nWCSrcID, nWCDstID, false);

		if (status == Editor_BadCommand)
		{
			Msg( "Worldcraft failed on node link creation...\n" );
		}
		else if (status == Editor_OK)
		{
			// Don't actually destroy the dynamic link while editing.  Just mark the link
			pAILink->m_LinkInfo &= ~bits_LINK_OFF;

			CAI_DynamicLink* pDynamicLink = CAI_DynamicLink::GetDynamicLink(pAILink->m_iSrcID, pAILink->m_iDestID);
			UTIL_Remove(pDynamicLink);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For destroying links in wc edit mode.  Actually have to create
//			a link in WC that is marked as off
// Input  :
// Output :
//-----------------------------------------------------------------------------
void NWCEdit::DestroyAILink( CBasePlayer *pPlayer )
{
	// -------------------------------------------------------------
	//  Check that WC is running with the right map version
	// -------------------------------------------------------------
	if (!IsWCVersionValid())
	{
		return;
	}

	CAI_Link* pAILink = FindPickerAILink(pPlayer);
	if (pAILink)
	{
		int nWCSrcID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pAILink->m_iSrcID];
		int nWCDstID = g_pAINetworkManager->GetEditOps()->m_pNodeIndexTable[pAILink->m_iDestID];
		int	status	 = Editor_CreateNodeLink(nWCSrcID, nWCDstID, false);

		if (status == Editor_BadCommand)
		{
			Msg( "Worldcraft failed on node link creation...\n" );
		}
		else if (status == Editor_OK)
		{
			// Create dynamic link and mark the link
			CAI_DynamicLink* pNewLink	= (CAI_DynamicLink*)CreateEntityByName("info_node_link" );;
			pNewLink->m_nSrcID			= pAILink->m_iSrcID;
			pNewLink->m_nDestID			= pAILink->m_iDestID;
			pNewLink->m_nLinkState		= LINK_OFF;
			pAILink->m_LinkInfo |= bits_LINK_OFF;
		}
	}
}

Vector *g_EntityPositions = NULL;
QAngle *g_EntityOrientations = NULL;
string_t *g_EntityClassnames = NULL;

//-----------------------------------------------------------------------------
// Purpose: Saves the entity's position for future communication with Hammer
//-----------------------------------------------------------------------------
void NWCEdit::RememberEntityPosition( CBaseEntity *pEntity )
{
	if ( !(pEntity->ObjectCaps() & FCAP_WCEDIT_POSITION) )
		return;

	if ( !g_EntityPositions )
	{
		g_EntityPositions = new Vector[NUM_ENT_ENTRIES];
		g_EntityOrientations = new QAngle[NUM_ENT_ENTRIES];
		// have to save these too because some entities change the classname on spawn (e.g. prop_physics_override, physics_prop)
		g_EntityClassnames = new string_t[NUM_ENT_ENTRIES];
	}
	int index = pEntity->entindex();
	g_EntityPositions[index] = pEntity->GetAbsOrigin();
	g_EntityOrientations[index] = pEntity->GetAbsAngles();
	g_EntityClassnames[index] = pEntity->m_iClassname;
}

//-----------------------------------------------------------------------------
// Purpose: Sends Hammer an update to the current position
//-----------------------------------------------------------------------------
void NWCEdit::UpdateEntityPosition( CBaseEntity *pEntity )
{
	const Vector &newPos = pEntity->GetAbsOrigin();
	const QAngle &newAng = pEntity->GetAbsAngles();

	DevMsg( 1, "%s\n   origin %f %f %f\n   angles %f %f %f\n", pEntity->GetClassname(), newPos.x, newPos.y, newPos.z, newAng.x, newAng.y, newAng.z );
	if ( Ragdoll_IsPropRagdoll(pEntity) )
	{
		char tmp[2048];
		Ragdoll_GetAngleOverrideString( tmp, sizeof(tmp), pEntity );
		DevMsg( 1, "pose: %s\n", tmp );
	}

	if ( !(pEntity->ObjectCaps() & FCAP_WCEDIT_POSITION) )
		return;
	
	// can't do this unless in edit mode
	if ( !engine->IsInEditMode() )
		return;

	int entIndex = pEntity->entindex();
	Vector pos = g_EntityPositions[entIndex];
	EditorSendResult_t result = Editor_BadCommand;
	const char *pClassname = STRING(g_EntityClassnames[entIndex]);

	if ( pEntity->GetModel() && modelinfo->GetModelType(pEntity->GetModel()) == mod_brush )
	{
		QAngle xformAngles;
		RotationDelta( g_EntityOrientations[entIndex], newAng, &xformAngles );
		if ( xformAngles.Length() > 1e-4 )
		{
			result = Editor_RotateEntity( pClassname, pos.x, pos.y, pos.z, xformAngles );
		}
		else
		{
			// don't call through for an identity rotation, may just increase error
			result = Editor_OK;
		}
	}
	else
	{
		if ( Ragdoll_IsPropRagdoll(pEntity) )
		{
			char tmp[2048];
			Ragdoll_GetAngleOverrideString( tmp, sizeof(tmp), pEntity );
			result = Editor_SetKeyValue( pClassname, pos.x, pos.y, pos.z, "angleOverride", tmp );
			if ( result != Editor_OK )
				goto error;
		}
		result = Editor_SetKeyValue( pClassname, pos.x, pos.y, pos.z, "angles", CFmtStr("%f %f %f", newAng.x, newAng.y, newAng.z) );
	}
	if ( result != Editor_OK )
		goto error;

	result = Editor_SetKeyValue( pClassname, pos.x, pos.y, pos.z, "origin", CFmtStr("%f %f %f", newPos.x, newPos.y, newPos.z) );
	if ( result != Editor_OK )
		goto error;

	NDebugOverlay::EntityBounds(pEntity, 0, 255, 0, 0 ,5);
	// save the update
	RememberEntityPosition( pEntity );
	return;

error:
	NDebugOverlay::EntityBounds(pEntity, 255, 0, 0, 0 ,5);
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_Create( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode())
	{
		CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

		if (g_pAINetworkManager->GetEditOps()->m_bLinkEditMode)
		{
			NWCEdit::CreateAILink(UTIL_GetCommandClient());
		}
		else
		{
			NWCEdit::CreateAINode(UTIL_GetCommandClient());
		}
	}
}
static ConCommand wc_create("wc_create", CC_WC_Create, "When in WC edit mode, creates a node where the player is looking if a node is allowed at that location for the currently selected hull size (see ai_next_hull)", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_Destroy( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode())
	{
		CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

		// UNDONE: For now just deal with info_nodes
		//CBaseEntity* pEntity = FindEntity( pEdict, ""); - use when generalize this to any class
		//int status = Editor_DeleteEntity("info_node", pEdict->origin.x, pEdict->origin.y, pEdict->origin.z, false);

		if (g_pAINetworkManager->GetEditOps()->m_bLinkEditMode)
		{
			NWCEdit::DestroyAILink(UTIL_GetCommandClient());
		}
		else
		{
			NWCEdit::DestroyAINode(UTIL_GetCommandClient());
		}
	}
}
static ConCommand wc_destroy("wc_destroy", CC_WC_Destroy, "When in WC edit mode, destroys the node that the player is nearest to looking at.  (The node will be highlighted by a red box).", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_DestroyUndo( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode())
	{
		CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();

		NWCEdit::UndoDestroyAINode();
	}
}
static ConCommand wc_destroy_undo("wc_destroy_undo", CC_WC_DestroyUndo, "When in WC edit mode restores the last deleted node", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_AirNodeEdit( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode())
	{
		// Toggle air edit mode state
		if (g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
		{
			g_pAINetworkManager->GetEditOps()->m_bAirEditMode = false;
		}
		else
		{
			g_pAINetworkManager->GetEditOps()->m_bAirEditMode = true;
		}
	}
}
static ConCommand wc_air_node_edit("wc_air_node_edit", CC_WC_AirNodeEdit, "When in WC edit mode, toggles laying down or air nodes instead of ground nodes", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_AirNodeEditFurther( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode() && g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
	{
		g_pAINetworkManager->GetEditOps()->m_flAirEditDistance += 10.0;
	}
}
static ConCommand wc_air_edit_further("wc_air_edit_further", CC_WC_AirNodeEditFurther, "When in WC edit mode and editing air nodes,  moves position of air node crosshair and placement location further away from player", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_AirNodeEditNearer( void )
{
	// Only allowed in wc_edit_mode
	if (engine->IsInEditMode() && g_pAINetworkManager->GetEditOps()->m_bAirEditMode)
	{
		g_pAINetworkManager->GetEditOps()->m_flAirEditDistance -= 10.0;
	}
}
static ConCommand wc_air_edit_nearer("wc_air_edit_nearer", CC_WC_AirNodeEditNearer, "When in WC edit mode and editing air nodes,  moves position of air node crosshair and placement location nearer to from player", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_WC_LinkEdit( void )
{
	// Only allowed in wc_edit_mode 
	if (engine->IsInEditMode())
	{
		// Toggle air edit mode state
		if (g_pAINetworkManager->GetEditOps()->m_bLinkEditMode)
		{
			g_pAINetworkManager->GetEditOps()->m_bLinkEditMode = false;
		}
		// Don't allow link mode if graph outdated
		else if (!(g_pAINetworkManager->GetEditOps()->m_debugNetOverlays & bits_debugNeedRebuild))
		{
			g_pAINetworkManager->GetEditOps()->m_bLinkEditMode = true;
		}
	}
}
static ConCommand wc_link_edit("wc_link_edit", CC_WC_LinkEdit, 0, FCVAR_CHEAT);


/// This is an entity used by the hammer_update_safe_entities command. It allows designers
/// to specify objects that should be ignored. It stores an array of sixteen strings
/// which may correspond to names. Designers may ignore more than sixteen objects by 
/// placing more than one of these in a level.
class CWC_UpdateIgnoreList : public CBaseEntity
{
public:
	DECLARE_CLASS( CWC_UpdateIgnoreList, CBaseEntity );

	enum { MAX_IGNORELIST_NAMES = 16 }; ///< the number of names in the array below

	inline const string_t &GetName( int x ) const { return m_nIgnoredEntityNames[x]; } 

protected:
	// the list of names to ignore
	string_t m_nIgnoredEntityNames[MAX_IGNORELIST_NAMES]; 

public:
	DECLARE_DATADESC();
};


LINK_ENTITY_TO_CLASS( hammer_updateignorelist, CWC_UpdateIgnoreList );

BEGIN_DATADESC( CWC_UpdateIgnoreList )

	// Be still, classcheck!
	//DEFINE_FIELD( m_nIgnoredEntityNames, FIELD_STRING, MAX_IGNORELIST_NAMES ),

	DEFINE_KEYFIELD( m_nIgnoredEntityNames[0], FIELD_STRING, "IgnoredName01" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[1], FIELD_STRING, "IgnoredName02" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[2], FIELD_STRING, "IgnoredName03" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[3], FIELD_STRING, "IgnoredName04" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[4], FIELD_STRING, "IgnoredName05" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[5], FIELD_STRING, "IgnoredName06" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[6], FIELD_STRING, "IgnoredName07" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[7], FIELD_STRING, "IgnoredName08" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[8], FIELD_STRING, "IgnoredName09" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[9], FIELD_STRING, "IgnoredName10" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[10], FIELD_STRING, "IgnoredName11" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[11], FIELD_STRING, "IgnoredName12" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[12], FIELD_STRING, "IgnoredName13" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[13], FIELD_STRING, "IgnoredName14" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[14], FIELD_STRING, "IgnoredName15" ),
	DEFINE_KEYFIELD( m_nIgnoredEntityNames[15], FIELD_STRING, "IgnoredName16" ),

END_DATADESC()



CON_COMMAND( hammer_update_entity, "Updates the entity's position/angles when in edit mode" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		CBasePlayer *pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,
			MASK_SHOT_HULL|CONTENTS_GRATE|CONTENTS_DEBRIS, pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() && !tr.DidHitWorld() )
		{
			NWCEdit::UpdateEntityPosition( tr.m_pEnt );
		}
	}
	else
	{
		CBaseEntity *pEnt = NULL;
		while ((pEnt = gEntList.FindEntityGeneric( pEnt, args[1] ) ) != NULL)
		{
			NWCEdit::UpdateEntityPosition( pEnt );
		}
	}
}

CON_COMMAND( hammer_update_safe_entities, "Updates entities in the map that can safely be updated (don't have parents or are affected by constraints). Also excludes entities mentioned in any hammer_updateignorelist objects in this map." )
{
	int iCount = 0;
	CBaseEntity *pEnt = NULL;

	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	Msg("\n====================================================\nPerforming Safe Entity Update\n" );

	// first look for any exclusion objects -- these are entities that list specific things to be ignored.
	// All the names that are inside them, we store into a hash table (here implemented through a local
	// CUtlSymbolTable)

	CUtlSymbolTable ignoredNames(16,32,true); // grow 16 strings at a time. Case insensitive.
	while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "hammer_updateignorelist" )) != NULL )
	{
		// for each name in each of those strings, add it to the symbol table.
		CWC_UpdateIgnoreList *piglist = static_cast<CWC_UpdateIgnoreList *>(pEnt);
		for (int ii = 0 ; ii < CWC_UpdateIgnoreList::MAX_IGNORELIST_NAMES ; ++ii )
		{
			if (!!piglist->GetName(ii))  // if not null
			{	// add to symtab
				ignoredNames.AddString(piglist->GetName(ii).ToCStr());
			}
		}
	}

	if ( ignoredNames.GetNumStrings() > 0 )
	{
		Msg( "Ignoring %d specified targetnames.\n", ignoredNames.GetNumStrings() );
	}


	// now iterate through everything in the world
	for ( pEnt = gEntList.FirstEnt(); pEnt != NULL; pEnt = gEntList.NextEnt(pEnt) )
	{
		if ( !(pEnt->ObjectCaps() & FCAP_WCEDIT_POSITION) )
			continue;

		// If we have a parent, or any children, we're not safe to update
		if ( pEnt->GetMoveParent() || pEnt->FirstMoveChild() )
			continue;

		IPhysicsObject *pPhysics = pEnt->VPhysicsGetObject();
		if ( !pPhysics )
			continue;
		// If we are affected by any constraints, we're not safe to update
		if ( pPhysics->IsAttachedToConstraint( Ragdoll_IsPropRagdoll(pEnt) )  )
			continue;
		// Motion disabled?
		if ( !pPhysics->IsMoveable() )
			continue;

		// ignore brush models (per bug 61318)
		if ( dynamic_cast<CPhysBox *>(pEnt) )
			continue;

		// explicitly excluded by designer?
		if ( ignoredNames.Find(pEnt->GetEntityName().ToCStr()).IsValid() )
			continue;

		NWCEdit::UpdateEntityPosition( pEnt );
		iCount++;
	}

	Msg("Updated %d entities.\n", iCount);
}
