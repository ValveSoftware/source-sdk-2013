//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERNETWORKABLE_H
#define ISERVERNETWORKABLE_H
#ifdef _WIN32
#pragma once
#endif


#include "ihandleentity.h"
#include "basetypes.h"
#include "bitvec.h"
#include "const.h"
#include "bspfile.h"



// Entities can span this many clusters before we revert to a slower area checking algorithm
#define	MAX_FAST_ENT_CLUSTERS	4
#define	MAX_ENT_CLUSTERS	64
#define MAX_WORLD_AREAS		8


class ServerClass;
class SendTable;
struct edict_t;
class CBaseEntity;
class CSerialEntity;
class CBaseNetworkable;


class CCheckTransmitInfo
{
public:
	edict_t	*m_pClientEnt;	// pointer to receiver edict
	byte	m_PVS[PAD_NUMBER( MAX_MAP_CLUSTERS,8 ) / 8];
	int		m_nPVSSize;		// PVS size in bytes

	CBitVec<MAX_EDICTS>	*m_pTransmitEdict;	// entity n is already marked for transmission
	CBitVec<MAX_EDICTS>	*m_pTransmitAlways; // entity n is always sent even if not in PVS (HLTV and Replay only)
	
	int 	m_AreasNetworked; // number of networked areas 
	int		m_Areas[MAX_WORLD_AREAS]; // the areas
	
	// This is used to determine visibility, so if the previous state
	// is the same as the current state (along with pvs and areas networked),
	// then the parts of the map that the player can see haven't changed.
	byte	m_AreaFloodNums[MAX_MAP_AREAS];
	int		m_nMapAreas;
};

//-----------------------------------------------------------------------------
// Stores information necessary to perform PVS testing.
//-----------------------------------------------------------------------------
struct PVSInfo_t
{
	// headnode for the entity's bounding box
	short		m_nHeadNode;			

	// number of clusters or -1 if too many
	short		m_nClusterCount;		

	// cluster indices
	unsigned short *m_pClusters;	

	// For dynamic "area portals"
	short		m_nAreaNum;
	short		m_nAreaNum2;

	// current position
	float		m_vCenter[3];

private:
	unsigned short m_pClustersInline[MAX_FAST_ENT_CLUSTERS];

	friend class CVEngineServer;
};


// IServerNetworkable is the interface the engine uses for all networkable data.
class IServerNetworkable
{
// These functions are handled automatically by the server_class macros and CBaseNetworkable.
public:
	// Gets at the entity handle associated with the collideable
	virtual IHandleEntity	*GetEntityHandle() = 0;

	// Tell the engine which class this object is.
	virtual ServerClass*	GetServerClass() = 0;

	virtual edict_t			*GetEdict() const = 0;

	virtual const char*		GetClassName() const = 0;
	virtual void			Release() = 0;

	virtual int				AreaNum() const = 0;

	// In place of a generic QueryInterface.
	virtual CBaseNetworkable* GetBaseNetworkable() = 0;
	virtual CBaseEntity*	GetBaseEntity() = 0; // Only used by game code.
	virtual PVSInfo_t*		GetPVSInfo() = 0; // get current visibilty data

protected:
	// Should never call delete on this! 
	virtual					~IServerNetworkable() {}
};


#endif // ISERVERNETWORKABLE_H
