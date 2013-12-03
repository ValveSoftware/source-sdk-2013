//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// The BSP tree leaf data system
//
//=============================================================================//

#include "tier0/platform.h"

#if !defined( BSPTREEDATA )
#define BSPTREEDATA
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

class Vector;
struct Ray_t;


//-----------------------------------------------------------------------------
// Handle to an renderable in the client leaf system
//-----------------------------------------------------------------------------

typedef unsigned short BSPTreeDataHandle_t;

enum
{
	TREEDATA_INVALID_HANDLE = (BSPTreeDataHandle_t)~0
};


//-----------------------------------------------------------------------------
// Interface needed by tree data to do its job
// 
// Note that anything that has convex spatial regions with leaves identified
// by indices can implement the ISpatialQuery. All you have to do is to implement
// a class that can answer the 5 questions in the Query interface about the
// spatial subdivision. For example, a K-D tree or a BSP tree could implement
// this interface
//
//-----------------------------------------------------------------------------

abstract_class ISpatialLeafEnumerator
{
public:
	// call back with a leaf and a context
	// The context is completely user defined; it's passed into the enumeration
	// function of ISpatialQuery.
	// This gets called	by the enumeration methods with each leaf
	// that passes the test; return true to continue enumerating,
	// false to stop

	virtual bool EnumerateLeaf( int leaf, int context ) = 0;
};

abstract_class ISpatialQuery
{
public:
	// Returns the number of leaves
	virtual int LeafCount() const = 0;

	// Enumerates the leaves along a ray, box, etc.
	virtual bool EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context ) = 0;
};


//-----------------------------------------------------------------------------
// Data associated with leaves.
//
// This is a parasitic class that attaches data to the leaves specified by the
// ISpatialQuery sent in to the initialization function. It can't exist without
// a spatial partition of some sort to hold onto.
//-----------------------------------------------------------------------------

abstract_class IBSPTreeDataEnumerator
{
public:
	// call back with a userId and a context
	virtual bool FASTCALL EnumerateElement( int userId, int context ) = 0;
};

abstract_class IBSPTreeData
{
public:
	// Add a virtual destructor so that the derived class destructors will
	// be called.
	virtual ~IBSPTreeData() {}

	// Initializes, shuts down
	virtual void Init( ISpatialQuery* pBSPTree ) = 0;
	virtual void Shutdown() = 0;

	// Adds and removes data from the leaf lists
	virtual BSPTreeDataHandle_t Insert( int userId, Vector const& mins, Vector const& maxs ) = 0;
	virtual void Remove( BSPTreeDataHandle_t handle ) = 0;

	// Call this when a element moves
	virtual void ElementMoved( BSPTreeDataHandle_t handle, Vector const& mins, Vector const& maxs ) = 0;

	// Enumerate elements in a particular leaf
	virtual bool EnumerateElementsInLeaf( int leaf, IBSPTreeDataEnumerator* pEnum, int context ) = 0;

	// Is the element in any leaves at all?
	virtual bool IsElementInTree( BSPTreeDataHandle_t handle ) const = 0;

	// NOTE: These methods call through to the functions in the attached
	// ISpatialQuery
	// For convenience, enumerates the leaves along a ray, box, etc.
	virtual bool EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context ) = 0;
	virtual bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context ) = 0;
};

//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------

IBSPTreeData* CreateBSPTreeData();
void DestroyBSPTreeData( IBSPTreeData* pTreeData );


#endif	// BSPTREEDATA


