//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef CMODEL_H
#define CMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "trace.h"
#include "tier0/dbg.h"
#include "basehandle.h"

struct edict_t;
struct model_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "bspflags.h"
//#include "mathlib/vector.h"

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

#include "vcollide.h"

struct cmodel_t
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	int			headnode;

	vcollide_t	vcollisionData;
};

struct csurface_t
{
	const char	*name;
	short		surfaceProps;
	unsigned short	flags;		// BUGBUG: These are declared per surface, not per material, but this database is per-material now
};

//-----------------------------------------------------------------------------
// A ray...
//-----------------------------------------------------------------------------

struct Ray_t
{
	VectorAligned  m_Start;	// starting point, centered within the extents
	VectorAligned  m_Delta;	// direction + length of the ray
	VectorAligned  m_StartOffset;	// Add this to m_Start to get the actual ray start
	VectorAligned  m_Extents;	// Describes an axis aligned box extruded along a ray
	bool	m_IsRay;	// are the extents zero?
	bool	m_IsSwept;	// is delta != 0?

	void Init( Vector const& start, Vector const& end )
	{
		Assert( &end );
		VectorSubtract( end, start, m_Delta );

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		VectorClear( m_Extents );
		m_IsRay = true;

		// Offset m_Start to be in the center of the box...
		VectorClear( m_StartOffset );
		VectorCopy( start, m_Start );
	}

	void Init( Vector const& start, Vector const& end, Vector const& mins, Vector const& maxs )
	{
		Assert( &end );
		VectorSubtract( end, start, m_Delta );

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		VectorSubtract( maxs, mins, m_Extents );
		m_Extents *= 0.5f;
		m_IsRay = (m_Extents.LengthSqr() < 1e-6);

		// Offset m_Start to be in the center of the box...
		VectorAdd( mins, maxs, m_StartOffset );
		m_StartOffset *= 0.5f;
		VectorAdd( start, m_StartOffset, m_Start );
		m_StartOffset *= -1.0f;
	}

	// compute inverse delta
	Vector InvDelta() const
	{
		Vector vecInvDelta;
		for ( int iAxis = 0; iAxis < 3; ++iAxis )
		{
			if ( m_Delta[iAxis] != 0.0f )
			{
				vecInvDelta[iAxis] = 1.0f / m_Delta[iAxis];
			}
			else
			{
				vecInvDelta[iAxis] = FLT_MAX;
			}
		}
		return vecInvDelta;
	}

private:
};


#endif // CMODEL_H

	
#include "gametrace.h"

