//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef ISPATIALPARTITION_H
#define ISPATIALPARTITION_H

#include "interface.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------

class Vector;
struct Ray_t;
class IHandleEntity;


#define INTERFACEVERSION_SPATIALPARTITION	"SpatialPartition001"

//-----------------------------------------------------------------------------
// These are the various partition lists. Note some are server only, some
// are client only
//-----------------------------------------------------------------------------

enum
{
	PARTITION_ENGINE_SOLID_EDICTS		= (1 << 0),		// every edict_t that isn't SOLID_TRIGGER or SOLID_NOT (and static props)
	PARTITION_ENGINE_TRIGGER_EDICTS		= (1 << 1),		// every edict_t that IS SOLID_TRIGGER
	PARTITION_CLIENT_SOLID_EDICTS		= (1 << 2),
	PARTITION_CLIENT_RESPONSIVE_EDICTS	= (1 << 3),		// these are client-side only objects that respond to being forces, etc.
	PARTITION_ENGINE_NON_STATIC_EDICTS	= (1 << 4),		// everything in solid & trigger except the static props, includes SOLID_NOTs
	PARTITION_CLIENT_STATIC_PROPS		= (1 << 5),
	PARTITION_ENGINE_STATIC_PROPS		= (1 << 6),
	PARTITION_CLIENT_NON_STATIC_EDICTS	= (1 << 7),		// everything except the static props
};

// Use this to look for all client edicts.
#define PARTITION_ALL_CLIENT_EDICTS	(		\
	PARTITION_CLIENT_NON_STATIC_EDICTS |	\
	PARTITION_CLIENT_STATIC_PROPS |			\
	PARTITION_CLIENT_RESPONSIVE_EDICTS |	\
	PARTITION_CLIENT_SOLID_EDICTS			\
	)


// These are the only handles in the spatial partition that the game is controlling (everything but static props)
// These masks are used to handle updating the dirty spatial partition list in each game DLL
#define PARTITION_CLIENT_GAME_EDICTS (PARTITION_ALL_CLIENT_EDICTS & ~PARTITION_CLIENT_STATIC_PROPS)
#define PARTITION_SERVER_GAME_EDICTS (PARTITION_ENGINE_SOLID_EDICTS|PARTITION_ENGINE_TRIGGER_EDICTS|PARTITION_ENGINE_NON_STATIC_EDICTS)

//-----------------------------------------------------------------------------
// Clients that want to know about all elements within a particular
// volume must inherit from this
//-----------------------------------------------------------------------------

enum IterationRetval_t
{
	ITERATION_CONTINUE = 0,
	ITERATION_STOP,
};


typedef unsigned short SpatialPartitionHandle_t;

// A combination of the PARTITION_ flags above.
typedef int SpatialPartitionListMask_t;	

typedef int SpatialTempHandle_t;


//-----------------------------------------------------------------------------
// Any search in the CSpatialPartition must use this to filter out entities it doesn't want.
// You're forced to use listMasks because it can filter by listMasks really fast. Any other
// filtering can be done by EnumElement.
//-----------------------------------------------------------------------------

class IPartitionEnumerator
{
public:
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity ) = 0;
};


//-----------------------------------------------------------------------------
// Installs a callback to call right before a spatial partition query occurs
//-----------------------------------------------------------------------------
class IPartitionQueryCallback
{
public:
	virtual void OnPreQuery_V1() = 0;
	virtual void OnPreQuery( SpatialPartitionListMask_t listMask ) = 0;
	virtual void OnPostQuery( SpatialPartitionListMask_t listMask ) = 0;
};


//-----------------------------------------------------------------------------
// This is the spatial partition manager, groups objects into buckets
//-----------------------------------------------------------------------------
enum
{
	PARTITION_INVALID_HANDLE = (SpatialPartitionHandle_t)~0
};


abstract_class ISpatialPartition
{
public:
	// Add a virtual destructor to silence the clang warning.
	// This is harmless but not important since the only derived class
	// doesn't have a destructor.
	virtual ~ISpatialPartition() {}

	// Create/destroy a handle for this dude in our system. Destroy
	// will also remove it from all lists it happens to be in
	virtual SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity ) = 0;

	// A fast method of creating a handle + inserting into the tree in the right place
	virtual SpatialPartitionHandle_t CreateHandle( IHandleEntity *pHandleEntity,
		SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs ) = 0; 

	virtual void DestroyHandle( SpatialPartitionHandle_t handle ) = 0;

	// Adds, removes an handle from a particular spatial partition list
	// There can be multiple partition lists; each has a unique id
	virtual void Insert( SpatialPartitionListMask_t listMask, 
		SpatialPartitionHandle_t handle ) = 0;
	virtual void Remove( SpatialPartitionListMask_t listMask, 
		SpatialPartitionHandle_t handle ) = 0;

	// Same as calling Remove() then Insert(). For performance-sensitive areas where you want to save a call.
	virtual void RemoveAndInsert( SpatialPartitionListMask_t removeMask, SpatialPartitionListMask_t insertMask, 
		SpatialPartitionHandle_t handle ) = 0;

	// This will remove a particular handle from all lists
	virtual void Remove( SpatialPartitionHandle_t handle ) = 0;

	// Call this when an entity moves...
	virtual void ElementMoved( SpatialPartitionHandle_t handle, 
		const Vector& mins, const Vector& maxs ) = 0;

	// A fast method to insert + remove a handle from the tree...
	// This is used to suppress collision of a single model..
	virtual SpatialTempHandle_t HideElement( SpatialPartitionHandle_t handle ) = 0;
	virtual void UnhideElement( SpatialPartitionHandle_t handle, SpatialTempHandle_t tempHandle ) = 0;
	
	// Installs callbacks to get called right before a query occurs
	virtual void InstallQueryCallback_V1( IPartitionQueryCallback *pCallback ) = 0;
	virtual void RemoveQueryCallback( IPartitionQueryCallback *pCallback ) = 0;

	// Gets all entities in a particular volume...
	// if coarseTest == true, it'll return all elements that are in
	// spatial partitions that intersect the box
	// if coarseTest == false, it'll return only elements that truly intersect
	virtual void EnumerateElementsInBox(
		SpatialPartitionListMask_t listMask,  
		const Vector& mins, 
		const Vector& maxs, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsInSphere(
		SpatialPartitionListMask_t listMask, 
		const Vector& origin, 
		float radius, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsAlongRay(
		SpatialPartitionListMask_t listMask, 
		const Ray_t& ray, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator 
		) = 0;

	virtual void EnumerateElementsAtPoint( 
		SpatialPartitionListMask_t listMask, 
		const Vector& pt, 
		bool coarseTest, 
		IPartitionEnumerator* pIterator
		) = 0;

	// For debugging.... suppress queries on particular lists
	virtual void SuppressLists( SpatialPartitionListMask_t nListMask, bool bSuppress ) = 0;
	virtual SpatialPartitionListMask_t GetSuppressedLists() = 0;

	virtual void RenderAllObjectsInTree( float flTime ) = 0;
	virtual void RenderObjectsInPlayerLeafs( const Vector &vecPlayerMin, const Vector &vecPlayerMax, float flTime ) = 0;
	virtual void RenderLeafsForRayTraceStart( float flTime ) = 0;
	virtual void RenderLeafsForRayTraceEnd( void ) = 0;
	virtual void RenderLeafsForHullTraceStart( float flTime ) = 0;
	virtual void RenderLeafsForHullTraceEnd( void ) = 0;
	virtual void RenderLeafsForBoxStart( float flTime ) = 0;
	virtual void RenderLeafsForBoxEnd( void ) = 0;
	virtual void RenderLeafsForSphereStart( float flTime ) = 0;
	virtual void RenderLeafsForSphereEnd( void ) = 0;

	virtual void RenderObjectsInBox( const Vector &vecMin, const Vector &vecMax, float flTime ) = 0;
	virtual void RenderObjectsInSphere( const Vector &vecCenter, float flRadius, float flTime ) = 0;
	virtual void RenderObjectsAlongRay( const Ray_t& ray, float flTime ) = 0;

	virtual void ReportStats( const char *pFileName ) = 0;

	virtual void InstallQueryCallback( IPartitionQueryCallback *pCallback ) = 0;
};

#endif



