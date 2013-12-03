//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_ROUTEDIST_H
#define AI_ROUTEDIST_H

#include "ai_navtype.h"

#if defined( _WIN32 )
#pragma once
#endif

// ----------------------------------------------------------------------------
// Computes the route distance + route direction based on nav type
// FIXME: Where should this go? 
// ----------------------------------------------------------------------------
inline float ComputePathDistance( Navigation_t navType, const Vector &start, const Vector &end )
{
	if (navType == NAV_GROUND)
	{
		return (end - start).Length2D();
	}
	else
	{
		return (end - start).Length();
	}
}

inline void ComputePathVector( Navigation_t navType, const Vector &start, const Vector &end, Vector *pDelta )
{
	if (navType == NAV_GROUND)
	{
		Vector2DSubtract( end.AsVector2D(), start.AsVector2D(), pDelta->AsVector2D() );
		pDelta->z = 0.0f;
	}
	else
	{
		VectorSubtract( end, start, *pDelta );
	}
}

inline float ComputePathDirection( Navigation_t navType, const Vector &start, const Vector &end, Vector *pDirection )
{
	if (navType == NAV_GROUND)
	{
		VectorSubtract( end, start, *pDirection );
		pDirection->z = 0.0f;
		return Vector2DNormalize( pDirection->AsVector2D() );
	}
	else
	{
		VectorSubtract( end, start, *pDirection );
		return VectorNormalize( *pDirection );
	}
}

#endif // AI_ROUTEDIST_H
