//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//===========================================================================//

#if !defined( CLIENTLEAFSYSTEM_H )
#define CLIENTLEAFSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "engine/IClientLeafSystem.h"
#include "cdll_int.h"
#include "ivrenderview.h"
#include "tier1/mempool.h"
#include "tier1/refcount.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct WorldListInfo_t;
class IClientRenderable;
class Vector;
class CGameTrace;
typedef CGameTrace trace_t;
struct Ray_t;
class Vector2D;
class CStaticProp;


//-----------------------------------------------------------------------------
// Handle to an renderables in the client leaf system
//-----------------------------------------------------------------------------
enum
{
	DETAIL_PROP_RENDER_HANDLE = (ClientRenderHandle_t)0xfffe
};


class CClientRenderablesList : public CRefCounted<>
{
	DECLARE_FIXEDSIZE_ALLOCATOR( CClientRenderablesList );

public:
	enum
	{
		MAX_GROUP_ENTITIES = 4096
	};

	struct CEntry
	{
		IClientRenderable	*m_pRenderable;
		unsigned short		m_iWorldListInfoLeaf; // NOTE: this indexes WorldListInfo_t's leaf list.
		unsigned short		m_TwoPass;
		ClientRenderHandle_t m_RenderHandle;
	};

	// The leaves for the entries are in the order of the leaves you call CollateRenderablesInLeaf in.
	CEntry		m_RenderGroups[RENDER_GROUP_COUNT][MAX_GROUP_ENTITIES];
	int			m_RenderGroupCounts[RENDER_GROUP_COUNT];
};


//-----------------------------------------------------------------------------
// Used by CollateRenderablesInLeaf
//-----------------------------------------------------------------------------
struct SetupRenderInfo_t
{
	WorldListInfo_t *m_pWorldListInfo;
	CClientRenderablesList *m_pRenderList;
	Vector m_vecRenderOrigin;
	Vector m_vecRenderForward;
	int m_nRenderFrame;
	int m_nDetailBuildFrame;	// The "render frame" for detail objects
	float m_flRenderDistSq;
	bool m_bDrawDetailObjects : 1;
	bool m_bDrawTranslucentObjects : 1;

	SetupRenderInfo_t()
	{
		m_bDrawDetailObjects = true;
		m_bDrawTranslucentObjects = true;
	}
};


//-----------------------------------------------------------------------------
// A handle associated with shadows managed by the client leaf system
//-----------------------------------------------------------------------------
typedef unsigned short ClientLeafShadowHandle_t;
enum
{
	CLIENT_LEAF_SHADOW_INVALID_HANDLE = (ClientLeafShadowHandle_t)~0 
};


//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafShadowEnum
{
public:
	// The user ID is the id passed into CreateShadow
	virtual void EnumShadow( ClientShadowHandle_t userId ) = 0;
};


// subclassed by things which wish to add per-leaf data managed by the client leafsystem
class CClientLeafSubSystemData
{
public:
	virtual ~CClientLeafSubSystemData( void )
	{
	}
};


// defines for subsystem ids. each subsystem id uses up one pointer in each leaf
#define CLSUBSYSTEM_DETAILOBJECTS 0
#define N_CLSUBSYSTEMS 1



//-----------------------------------------------------------------------------
// The client leaf system
//-----------------------------------------------------------------------------
abstract_class IClientLeafSystem : public IClientLeafSystemEngine, public IGameSystemPerFrame
{
public:
	// Adds and removes renderables from the leaf lists
	virtual void AddRenderable( IClientRenderable* pRenderable, RenderGroup_t group ) = 0;

	// This tells if the renderable is in the current PVS. It assumes you've updated the renderable
	// with RenderableChanged() calls
	virtual bool IsRenderableInPVS( IClientRenderable *pRenderable ) = 0;

	virtual void SetSubSystemDataInLeaf( int leaf, int nSubSystemIdx, CClientLeafSubSystemData *pData ) =0;
	virtual CClientLeafSubSystemData *GetSubSystemDataInLeaf( int leaf, int nSubSystemIdx ) =0;


	virtual void SetDetailObjectsInLeaf( int leaf, int firstDetailObject, int detailObjectCount ) = 0;
	virtual void GetDetailObjectsInLeaf( int leaf, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Indicates which leaves detail objects should be rendered from, returns the detais objects in the leaf
	virtual void DrawDetailObjectsInLeaf( int leaf, int frameNumber, int& firstDetailObject, int& detailObjectCount ) = 0;

	// Should we draw detail objects (sprites or models) in this leaf (because it's close enough to the view)
	// *and* are there any objects in the leaf?
	virtual bool ShouldDrawDetailObjectsInLeaf( int leaf, int frameNumber ) = 0;

	// Call this when a renderable origin/angles/bbox parameters has changed
	virtual void RenderableChanged( ClientRenderHandle_t handle ) = 0;

	// Set a render group
	virtual void SetRenderGroup( ClientRenderHandle_t handle, RenderGroup_t group ) = 0;

	// Computes which leaf translucent objects should be rendered in
	virtual void ComputeTranslucentRenderLeaf( int count, const LeafIndex_t *pLeafList, const LeafFogVolume_t *pLeafFogVolumeList, int frameNumber, int viewID ) = 0;

	// Put renderables into their appropriate lists.
	virtual void BuildRenderablesList( const SetupRenderInfo_t &info ) = 0;

	// Put renderables in the leaf into their appropriate lists.
	virtual void CollateViewModelRenderables( CUtlVector< IClientRenderable * >& opaqueList, CUtlVector< IClientRenderable * >& translucentList ) = 0;

	// Call this to deactivate static prop rendering..
	virtual void DrawStaticProps( bool enable ) = 0;

	// Call this to deactivate small object rendering
	virtual void DrawSmallEntities( bool enable ) = 0;

	// The following methods are related to shadows...
	virtual ClientLeafShadowHandle_t AddShadow( ClientShadowHandle_t userId, unsigned short flags ) = 0;
	virtual void RemoveShadow( ClientLeafShadowHandle_t h ) = 0;

	// Project a shadow
	virtual void ProjectShadow( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList ) = 0;

	// Project a projected texture spotlight
	virtual void ProjectFlashlight( ClientLeafShadowHandle_t handle, int nLeafCount, const int *pLeafList ) = 0;

	// Find all shadow casters in a set of leaves
	virtual void EnumerateShadowsInLeaves( int leafCount, LeafIndex_t* pLeaves, IClientLeafShadowEnum* pEnum ) = 0;

	// Fill in a list of the leaves this renderable is in.
	// Returns -1 if the handle is invalid.
	virtual int GetRenderableLeaves( ClientRenderHandle_t handle, int leaves[128] ) = 0;

	// Get leaves this renderable is in
	virtual bool GetRenderableLeaf ( ClientRenderHandle_t handle, int* pOutLeaf, const int* pInIterator = 0, int* pOutIterator = 0 ) = 0;

	// Use alternate translucent sorting algorithm (draw translucent objects in the furthest leaf they lie in)
	virtual void EnableAlternateSorting( ClientRenderHandle_t handle, bool bEnable ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
extern IClientLeafSystem *g_pClientLeafSystem;
inline IClientLeafSystem* ClientLeafSystem()
{
	return g_pClientLeafSystem;
}


#endif	// CLIENTLEAFSYSTEM_H


