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

#include "basetypes.h"
#include "bsptreedata.h"
#include "utllinkedlist.h"
#include "utlvector.h"
#include "tier0/dbg.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// The BSP tree leaf data system
//-----------------------------------------------------------------------------
class CBSPTreeData : public IBSPTreeData, public ISpatialLeafEnumerator
{
public:
	// constructor, destructor
	CBSPTreeData();
	virtual ~CBSPTreeData();

	// Methods of IBSPTreeData
	void Init( ISpatialQuery* pBSPTree );
	void Shutdown();

	BSPTreeDataHandle_t Insert( int userId, Vector const& mins, Vector const& maxs );
	void Remove( BSPTreeDataHandle_t handle );
	void ElementMoved( BSPTreeDataHandle_t handle, Vector const& mins, Vector const& maxs );

	// Enumerate elements in a particular leaf
	bool EnumerateElementsInLeaf( int leaf, IBSPTreeDataEnumerator* pEnum, int context );

	// For convenience, enumerates the leaves along a ray, box, etc.
	bool EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context );
	bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context );

	// methods of IBSPLeafEnumerator
	bool EnumerateLeaf( int leaf, int context );

	// Is the element in any leaves at all?
	bool IsElementInTree( BSPTreeDataHandle_t handle ) const;

private:
	// Creates a new handle
	BSPTreeDataHandle_t NewHandle( int userId );

	// Adds a handle to the list of handles
	void AddHandleToLeaf( int leaf, BSPTreeDataHandle_t handle );

	// insert, remove handles from leaves
	void InsertIntoTree( BSPTreeDataHandle_t handle, Vector const& mins, Vector const& maxs );
	void RemoveFromTree( BSPTreeDataHandle_t handle );

	// Returns the number of elements in a leaf
	int CountElementsInLeaf( int leaf );

private:
	// All the information associated with a particular handle
	struct HandleInfo_t
	{
		int					m_UserId;		// Client-defined id
		unsigned short		m_LeafList;		// What leafs is it in?
	};

	// The leaf contains an index into a list of elements
	struct Leaf_t
	{
		unsigned short	m_FirstElement;
	};

	// The handle knows about the leaves it lies in
	struct HandleInLeaf_t
	{
		int				m_Leaf;				// what leaf is the handle in?
		unsigned short	m_LeafElementIndex;	// what's the m_LeafElements index of the entry?
	};

	// Stores data associated with each leaf.
	CUtlVector< Leaf_t >	m_Leaf;

	// Stores all unique handles
	CUtlLinkedList< HandleInfo_t, unsigned short >		m_Handles;

	// Maintains the list of all handles in a particular leaf
	CUtlLinkedList< BSPTreeDataHandle_t, unsigned short, true >	m_LeafElements;

	// Maintains the list of all leaves a particular handle spans
	CUtlLinkedList< HandleInLeaf_t, unsigned short, true >	m_HandleLeafList;

	// Interface to BSP tree
	ISpatialQuery*	m_pBSPTree;
};


//-----------------------------------------------------------------------------
// Class factory
//-----------------------------------------------------------------------------

IBSPTreeData* CreateBSPTreeData()
{
	return new CBSPTreeData;
}

void DestroyBSPTreeData( IBSPTreeData* pTreeData )
{
	if (pTreeData)
		delete pTreeData;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

CBSPTreeData::CBSPTreeData()
{
}

CBSPTreeData::~CBSPTreeData()
{
}


//-----------------------------------------------------------------------------
// Level init, shutdown
//-----------------------------------------------------------------------------

void CBSPTreeData::Init( ISpatialQuery* pBSPTree )
{
	Assert( pBSPTree );
	m_pBSPTree = pBSPTree;

	m_Handles.EnsureCapacity( 1024 );
	m_LeafElements.EnsureCapacity( 1024 );
	m_HandleLeafList.EnsureCapacity( 1024 );

	// Add all the leaves we'll need
	int leafCount = m_pBSPTree->LeafCount();
	m_Leaf.EnsureCapacity( leafCount );

	Leaf_t newLeaf;
	newLeaf.m_FirstElement = m_LeafElements.InvalidIndex();
	while ( --leafCount >= 0 )
	{
		m_Leaf.AddToTail( newLeaf );
	}
}

void CBSPTreeData::Shutdown()
{
	m_Handles.Purge();
	m_LeafElements.Purge();
	m_HandleLeafList.Purge();
	m_Leaf.Purge();
}


//-----------------------------------------------------------------------------
// Creates a new handle
//-----------------------------------------------------------------------------

BSPTreeDataHandle_t CBSPTreeData::NewHandle( int userId )
{
	BSPTreeDataHandle_t handle = m_Handles.AddToTail();
	m_Handles[handle].m_UserId = userId;
	m_Handles[handle].m_LeafList = m_HandleLeafList.InvalidIndex();

	return handle;
}

//-----------------------------------------------------------------------------
// Add/remove handle
//-----------------------------------------------------------------------------

BSPTreeDataHandle_t CBSPTreeData::Insert( int userId, Vector const& mins, Vector const& maxs )
{
	BSPTreeDataHandle_t handle = NewHandle( userId );
	InsertIntoTree( handle, mins, maxs );
	return handle;
}

void CBSPTreeData::Remove( BSPTreeDataHandle_t handle )
{
	if (!m_Handles.IsValidIndex(handle))
		return;

	RemoveFromTree( handle );
	m_Handles.Free( handle );
}


//-----------------------------------------------------------------------------
// Adds a handle to a leaf
//-----------------------------------------------------------------------------
void CBSPTreeData::AddHandleToLeaf( int leaf, BSPTreeDataHandle_t handle )
{	
	// Got to a leaf baby! Add the handle to the leaf's list of elements
	unsigned short leafElement = m_LeafElements.Alloc( true );
	if (m_Leaf[leaf].m_FirstElement != m_LeafElements.InvalidIndex() )
		m_LeafElements.LinkBefore( m_Leaf[leaf].m_FirstElement, leafElement );
	m_Leaf[leaf].m_FirstElement = leafElement;
	m_LeafElements[leafElement] = handle;

	// Insert the leaf into the handles's list of leaves
	unsigned short handleElement = m_HandleLeafList.Alloc( true );
	if (m_Handles[handle].m_LeafList != m_HandleLeafList.InvalidIndex() )
		m_HandleLeafList.LinkBefore( m_Handles[handle].m_LeafList, handleElement );
	m_Handles[handle].m_LeafList = handleElement;
	m_HandleLeafList[handleElement].m_Leaf = leaf;
	m_HandleLeafList[handleElement].m_LeafElementIndex = leafElement;
}


//-----------------------------------------------------------------------------
// Inserts an element into the tree
//-----------------------------------------------------------------------------
bool CBSPTreeData::EnumerateLeaf( int leaf, int context )
{
	BSPTreeDataHandle_t handle = (BSPTreeDataHandle_t)context;
	AddHandleToLeaf( leaf, handle );
	return true;
}

void CBSPTreeData::InsertIntoTree( BSPTreeDataHandle_t handle, Vector const& mins, Vector const& maxs )
{
	m_pBSPTree->EnumerateLeavesInBox( mins, maxs, this, handle );
}

//-----------------------------------------------------------------------------
// Removes an element from the tree
//-----------------------------------------------------------------------------

void CBSPTreeData::RemoveFromTree( BSPTreeDataHandle_t handle )
{
	// Iterate over the list of all leaves the handle is in
	unsigned short i = m_Handles[handle].m_LeafList;
	while (i != m_HandleLeafList.InvalidIndex())
	{
		int leaf = m_HandleLeafList[i].m_Leaf;
		unsigned short leafElement = m_HandleLeafList[i].m_LeafElementIndex; 

		// Unhook the handle from the leaf handle list
		if (leafElement == m_Leaf[leaf].m_FirstElement)
			m_Leaf[leaf].m_FirstElement = m_LeafElements.Next(leafElement);
		m_LeafElements.Free(leafElement);

		unsigned short prevNode = i;
		i = m_HandleLeafList.Next(i);
		m_HandleLeafList.Free(prevNode);
	}

	m_Handles[handle].m_LeafList = m_HandleLeafList.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Call this when the element moves
//-----------------------------------------------------------------------------
void CBSPTreeData::ElementMoved( BSPTreeDataHandle_t handle, Vector const& mins, Vector const& maxs )
{
	if (handle != TREEDATA_INVALID_HANDLE)
	{
		RemoveFromTree( handle );
		InsertIntoTree( handle, mins, maxs );
	}
}


//-----------------------------------------------------------------------------
// Is the element in any leaves at all?
//-----------------------------------------------------------------------------
bool CBSPTreeData::IsElementInTree( BSPTreeDataHandle_t handle ) const
{
	return m_Handles[handle].m_LeafList != m_HandleLeafList.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Enumerate elements in a particular leaf
//-----------------------------------------------------------------------------
int CBSPTreeData::CountElementsInLeaf( int leaf )
{
	int i;
	int nCount = 0;
	for( i = m_Leaf[leaf].m_FirstElement; i != m_LeafElements.InvalidIndex(); i = m_LeafElements.Next(i) )
	{
		++nCount;
	}

	return nCount;
}

//-----------------------------------------------------------------------------
// Enumerate elements in a particular leaf
//-----------------------------------------------------------------------------
bool CBSPTreeData::EnumerateElementsInLeaf( int leaf, IBSPTreeDataEnumerator* pEnum, int context )
{
#ifdef DBGFLAG_ASSERT
	// The enumeration method better damn well not change this list...
	int nCount = CountElementsInLeaf(leaf);
#endif

	unsigned short idx = m_Leaf[leaf].m_FirstElement;
	while (idx != m_LeafElements.InvalidIndex())
	{
		BSPTreeDataHandle_t handle = m_LeafElements[idx];
		if (!pEnum->EnumerateElement( m_Handles[handle].m_UserId, context ))
		{
			Assert( CountElementsInLeaf(leaf) == nCount );
			return false;
		}
		idx = m_LeafElements.Next(idx);
	}

	Assert( CountElementsInLeaf(leaf) == nCount );

	return true;
}


//-----------------------------------------------------------------------------
// For convenience, enumerates the leaves along a ray, box, etc.
//-----------------------------------------------------------------------------
bool CBSPTreeData::EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, int context )
{
	return m_pBSPTree->EnumerateLeavesAtPoint( pt, pEnum, context );
}

bool CBSPTreeData::EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, int context )
{
	return m_pBSPTree->EnumerateLeavesInBox( mins, maxs, pEnum, context );
}

bool CBSPTreeData::EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, int context )
{
	return m_pBSPTree->EnumerateLeavesInSphere( center, radius, pEnum, context );
}

bool CBSPTreeData::EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, int context )
{
	return m_pBSPTree->EnumerateLeavesAlongRay( ray, pEnum, context );
}

