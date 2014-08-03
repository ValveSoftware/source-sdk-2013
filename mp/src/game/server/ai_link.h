//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Base combat character with no AI
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_LINK_H
#define AI_LINK_H
#pragma once

#include "ai_hull.h"	// For num hulls

struct edict_t;

enum Link_Info_t
{
	bits_LINK_STALE_SUGGESTED	=	0x01,		// NPC found this link to be blocked
	bits_LINK_OFF				=	0x02,		// This link has been turned off
};

//=============================================================================
//	>> CAI_Link
//=============================================================================

class CAI_DynamicLink;

#define AI_MOVE_TYPE_BITS ( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_FLY | bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_SWIM | bits_CAP_MOVE_CRAWL )

class CAI_Link
{
public:

	short	m_iSrcID;							// the node that 'owns' this link
	short	m_iDestID;							// the node on the other end of the link. 
	
	int		DestNodeID(int srcID);				// Given the source node ID, returns the destination ID

	byte 	m_iAcceptedMoveTypes[NUM_HULLS];	// Capability_T of motions acceptable for each hull type

	byte	m_LinkInfo;							// other information about this link

	float	m_timeStaleExpires;

	CAI_DynamicLink *m_pDynamicLink;
	
	//edict_t	*m_pLinkEnt;	// the entity that blocks this connection (doors, etc)

	// m_szLinkEntModelname is not necessarily NULL terminated (so we can store it in a more alignment-friendly 4 bytes)
	//char	m_szLinkEntModelname[ 4 ];// the unique name of the brush model that blocks the connection (this is kept for save/restore)

	//float	m_flWeight;		// length of the link line segment

private:
	friend class CAI_Network;
	CAI_Link(void);
};

#endif // AI_LINK_H
