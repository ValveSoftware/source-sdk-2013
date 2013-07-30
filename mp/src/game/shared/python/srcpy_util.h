//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Misc utility code.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_UTIL_H
#define SRCPY_UTIL_H
#ifdef _WIN32
#pragma once
#endif

#include <boost/python.hpp>

#include "collisionutils.h"
#include "util_shared.h"


//-----------------------------------------------------------------------------
// Purpose: Ray_t can't be exposed to python because of VectorAligned
//-----------------------------------------------------------------------------
struct PyRay_t
{
	// Constructors
	PyRay_t() {}
	PyRay_t( const Ray_t &raysrc ) 
	{ 
		m_Start = raysrc.m_Start;
		m_Delta = raysrc.m_Delta;
		m_StartOffset = raysrc.m_StartOffset;
		m_Extents = raysrc.m_Extents;
		m_IsRay = raysrc.m_IsRay;
		m_IsSwept = raysrc.m_IsSwept;
	}

	// To ray
	Ray_t ToRay() const
	{
		Ray_t ray;
		ray.m_Start = m_Start;
		ray.m_Delta = m_Delta;
		ray.m_StartOffset = m_StartOffset;
		ray.m_Extents = m_Extents;
		ray.m_IsRay = m_IsRay;
		ray.m_IsSwept = m_IsSwept;	
		return ray;
	}

	// Copy of Ray_t, but using Vector
	Vector  m_Start;	// starting point, centered within the extents
	Vector  m_Delta;	// direction + length of the ray
	Vector  m_StartOffset;	// Add this to m_Start to get the actual ray start
	Vector  m_Extents;	// Describes an axis aligned box extruded along a ray
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
};

#ifndef CLIENT_DLL
void UTIL_PySetSize( CBaseEntity *ent, const Vector &vecmin, const Vector &vecmax );
void UTIL_PySetModel( CBaseEntity *entity, const char *modelname );
#endif // CLIENT_DLL

// TODO int UTIL_GetModuleIndex( const char *module );
// TODO const char *UTIL_GetModuleNameFromIndex( int index );

boost::python::list UTIL_ListDir( const char *path, const char *pathid=NULL, const char *wildcard="*" );

// Wrappers
#ifdef CLIENT_DLL
	#define DEFAULT_PARTITION_MASK PARTITION_CLIENT_NON_STATIC_EDICTS
#else
	#define DEFAULT_PARTITION_MASK PARTITION_ENGINE_NON_STATIC_EDICTS
#endif // CLIENT_DLL

boost::python::object UTIL_PyEntitiesInBox( int listMax, const Vector &mins, const Vector &maxs, int flagMask, int partitionmask = DEFAULT_PARTITION_MASK );
boost::python::object UTIL_PyEntitiesInSphere( int listMax, const Vector &center, float radius, int flagMask, int partitionmask = DEFAULT_PARTITION_MASK );
boost::python::object UTIL_PyEntitiesAlongRay( int listMax, const PyRay_t &ray, int flagMask, int partitionmask = DEFAULT_PARTITION_MASK );


// Converter IHandleEntity
boost::python::object ConvertIHandleEntity( IHandleEntity *entity );

// Simple trace filter for python
class CPyTraceFilterSimple : public CTraceFilterSimple
{
public:
	CPyTraceFilterSimple( const IHandleEntity *passentity, int collisionGroup ) : CTraceFilterSimple( passentity, collisionGroup ) {}
};

// Collision utils
inline float PyIntersectRayWithTriangle( const PyRay_t& ray, 
							   const Vector& v1, const Vector& v2, const Vector& v3, bool oneSided )
{
	//return IntersectRayWithTriangle( *(ray.ray), v1, v2, v3, oneSided );
	return IntersectRayWithTriangle( ray.ToRay(), v1, v2, v3, oneSided );
}

inline void UTIL_PyTraceRay( const PyRay_t &ray, unsigned int mask, 
						  const CBaseEntity *ignore, int collisionGroup, trace_t *ptr )
{
	CTraceFilterSimple traceFilter( (const IHandleEntity *)ignore, collisionGroup );

	//enginetrace->TraceRay( (*ray.ray), mask, &traceFilter, ptr );
	enginetrace->TraceRay( ray.ToRay(), mask, &traceFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}

inline void UTIL_PyTraceRay( const PyRay_t &ray, unsigned int mask, 
							ITraceFilter &traceFilter, trace_t *ptr )
{
	//enginetrace->TraceRay( (*ray.ray), mask, &traceFilter, ptr );
	enginetrace->TraceRay( ray.ToRay(), mask, &traceFilter, ptr );

	if( r_visualizetraces.GetBool() )
	{
		DebugDrawLine( ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f );
	}
}

// Prediction
CBaseEntity const *GetSuppressHost();

#endif // SRCPY_UTIL_H