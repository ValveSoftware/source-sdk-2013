//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_WAYPOINT_H
#define AI_WAYPOINT_H

#if defined( _WIN32 )
#pragma once
#endif

#include <mempool.h>

// ----------------------------------------------------------------------------
// Forward declarations
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Flags used in the flags field in AI_Waypoint_T
// ----------------------------------------------------------------------------
enum WaypointFlags_t 
{
	// The type of waypoint
	bits_WP_TO_DETOUR =			0x01, // move to detour point.
	bits_WP_TO_PATHCORNER =		0x02, // move to a path corner
	bits_WP_TO_NODE =			0x04, // move to a node
	bits_WP_TO_GOAL =			0x08, // move to an arbitrary point
	bits_WP_TO_DOOR =			0x10, // move to position to open a door

	// Other flags for waypoint
	bits_WP_DONT_SIMPLIFY =		0x20, // Don't let the route code simplify this waypoint
};

// ----------------------------------------------------------------------------
// Purpose: Waypoints that make up an NPC's route. 
// ----------------------------------------------------------------------------
struct AI_Waypoint_t
{
public:
	AI_Waypoint_t();
	AI_Waypoint_t( const Vector &vecPosition, float flYaw, Navigation_t navType, int fWaypointFlags, int nNodeID );
	AI_Waypoint_t( const AI_Waypoint_t &from )
	{
		memcpy( this, &from, sizeof(*this) );
		flPathDistGoal = -1;
		pNext = pPrev = NULL;
	}

	AI_Waypoint_t &operator=( const AI_Waypoint_t &from )
	{
		memcpy( this, &from, sizeof(*this) );
		flPathDistGoal = -1;
		pNext = pPrev = NULL;
		return *this;
	}

	~AI_Waypoint_t()
	{
		AssertValid();
		if ( pNext )
		{
			pNext->AssertValid();
			pNext->pPrev = pPrev;
		}
		if ( pPrev )
		{
			pPrev->AssertValid();
			pPrev->pNext = pNext;
		}
	}

	//---------------------------------

	void AssertValid() const
	{
#ifdef DEBUG
		Assert( !pNext || pNext->pPrev == this );
		Assert( !pPrev || pPrev->pNext == this );
#endif
	}


	//---------------------------------
	
	int					Flags() const;
	Navigation_t		NavType() const;

	// Flag modification method
	void				ModifyFlags( int fFlags, bool bEnable );

	bool				IsReducible() { return (pNext && m_iWPType == pNext->m_iWPType && !(m_fWaypointFlags & (bits_WP_TO_GOAL | bits_WP_TO_PATHCORNER | bits_WP_DONT_SIMPLIFY)) ); }

	//---------------------------------
	
	void				SetNext( AI_Waypoint_t *p );
	AI_Waypoint_t *		GetNext()					{ return pNext; }
	const AI_Waypoint_t *GetNext() const			{ return pNext; }
	
	AI_Waypoint_t *		GetPrev()					{ return pPrev; }
	const AI_Waypoint_t *GetPrev() const			{ return pPrev; }
	
	AI_Waypoint_t *		GetLast();

	//---------------------------------
	
	const Vector &		GetPos() const				{ return vecLocation; }
	void 				SetPos(const Vector &newPos) { vecLocation = newPos; }

	EHANDLE				GetEHandleData() { return m_hData; }
	
	//---------------------------------
	//
	// Basic info
	//
	Vector			vecLocation;
	float			flYaw;				// Waypoint facing dir 
	int				iNodeID;			// If waypoint is a node, which one
	
	//---------------------------------
	//
	// Precalculated distances
	//
	float			flPathDistGoal;

	//---------------------------------
	//
	// If following a designer laid path, the path-corner entity (if any)
	//
	EHANDLE			hPathCorner;

	// Data specific to the waypoint type:
	//
	// PATHCORNER:	The path corner entity.
	// DOOR:		If moving to position to open a door, the handle of the door to open.
	EHANDLE			m_hData;

private:
	int				m_fWaypointFlags;	// See WaypointFlags_t
	Navigation_t	m_iWPType;			// The type of waypoint

	AI_Waypoint_t *pNext;
	AI_Waypoint_t *pPrev;

	DECLARE_FIXEDSIZE_ALLOCATOR(AI_Waypoint_t);

public:
	DECLARE_SIMPLE_DATADESC();
};


// ----------------------------------------------------------------------------
// Inline methods associated with AI_Waypoint_t
// ----------------------------------------------------------------------------
inline int AI_Waypoint_t::Flags() const
{
	return m_fWaypointFlags;
}

inline Navigation_t AI_Waypoint_t::NavType() const
{
	return m_iWPType;
}

inline void AI_Waypoint_t::ModifyFlags( int fFlags, bool bEnable )
{
	if (bEnable)
		m_fWaypointFlags |= fFlags;
	else
		m_fWaypointFlags &= ~fFlags;
}

inline void AI_Waypoint_t::SetNext( AI_Waypoint_t *p )	
{ 
	if (pNext) 
	{
		pNext->pPrev = NULL; 
	}

	pNext = p; 

	if ( pNext ) 
	{
		if ( pNext->pPrev )
			pNext->pPrev->pNext = NULL;

		pNext->pPrev = this; 
	}
}


// ----------------------------------------------------------------------------
// Purpose: Holds an maintains a chain of waypoints

class CAI_WaypointList
{
public:
	CAI_WaypointList()
	 :	m_pFirstWaypoint( NULL )
	{
	}

	CAI_WaypointList( AI_Waypoint_t *pFirstWaypoint)
	 :	m_pFirstWaypoint( pFirstWaypoint )
	{
	}
	
	void			Set(AI_Waypoint_t* route);

	void 			PrependWaypoints( AI_Waypoint_t *pWaypoints );
	void 			PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags, float flYaw = 0 );
	
	bool 			IsEmpty() const				{ return ( m_pFirstWaypoint == NULL ); }
	
	AI_Waypoint_t *		 GetFirst()				{ return m_pFirstWaypoint; }
	const AI_Waypoint_t *GetFirst() const	{ return m_pFirstWaypoint; }

	AI_Waypoint_t *		 GetLast();
	const AI_Waypoint_t *GetLast() const;

	void 			RemoveAll();

private:

	AI_Waypoint_t*	m_pFirstWaypoint;					// Linked list of waypoints
};

// ----------------------------------------------------------------------------
#ifdef DEBUG
void AssertRouteValid( AI_Waypoint_t* route );
#else
#define AssertRouteValid( route ) ((void)0)
#endif

// ----------------------------------------------------------------------------
// Utilities

void DeleteAll( AI_Waypoint_t *pWaypointList );

// ------------------------------------

inline void DeleteAll( AI_Waypoint_t **ppWaypointList )
{
	DeleteAll( *ppWaypointList );
	*ppWaypointList = NULL;
}

// ------------------------------------

void AddWaypointLists(AI_Waypoint_t *pLeft, AI_Waypoint_t *pRight);

// ----------------------------------------------------------------------------



#endif // AI_WAYPOINT_H
