//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		AI Utility classes for building the initial AI Networks
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_INITUTILS_H
#define AI_INITUTILS_H
#ifdef _WIN32
#pragma once
#endif


#include "ai_basenpc.h"
#include "ai_node.h"

//###########################################################
//  >> HintNodeData
//
// This is a chunk of data that's passed to a hint node entity
// when it's created from a CNodeEnt.
//###########################################################
enum HintIgnoreFacing_t
{
	HIF_NO,
	HIF_YES,
	HIF_DEFAULT,
};


struct HintNodeData
{
	string_t	strEntityName;
	Vector		vecPosition;
	short		nHintType;
	int			nNodeID;
	string_t	strGroup;
	int			iDisabled;
	string_t	iszActivityName;
	int			nTargetWCNodeID;
	HintIgnoreFacing_t fIgnoreFacing;
	NPC_STATE	minState;
	NPC_STATE	maxState;

	int			nWCNodeID;			// Node ID assigned by worldcraft (not same as engine!)

	DECLARE_SIMPLE_DATADESC();
};

//###########################################################
//  >> CNodeEnt
//
// This is the entity that is loaded in from worldcraft.
// It is only used to build the network and is deleted
// immediately
//###########################################################
class CNodeEnt : public CServerOnlyPointEntity
{
	DECLARE_CLASS( CNodeEnt, CServerOnlyPointEntity );

public:
	virtual void SetOwnerEntity( CBaseEntity* pOwner ) { BaseClass::SetOwnerEntity( NULL ); }

	static int			m_nNodeCount;

	void	Spawn( void );
	int		Spawn( const char *pMapData );

	DECLARE_DATADESC();

	CNodeEnt(void);

public:
	HintNodeData		m_NodeData;
};

//###########################################################
// >> CAI_TestHull 
//
// a modelless clip hull that verifies reachable nodes by 
// walking from every node to each of it's connections//
//###########################################################
class CAI_TestHull : public CAI_BaseNPC
{
	DECLARE_CLASS( CAI_TestHull, CAI_BaseNPC );
private:
	static CAI_TestHull*	pTestHull;								// Hull for testing connectivity

public:
	static CAI_TestHull*	GetTestHull(void);						// Get the test hull
	static void				ReturnTestHull(void);					// Return the test hull

	bool					bInUse;
	virtual void			Precache();
	void					Spawn(void);
	virtual int				ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~(FCAP_ACROSS_TRANSITION|FCAP_DONT_SAVE); }

	virtual bool			IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const;

	~CAI_TestHull(void);
};


#endif // AI_INITUTILS_H
