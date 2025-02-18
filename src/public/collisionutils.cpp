//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Common collision utility methods
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#include "collisionutils.h"
#include "cmodel.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "tier0/dbg.h"
#include <float.h>
#include "mathlib/vector4d.h"
#include "trace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define UNINIT		-99999.0

//-----------------------------------------------------------------------------
// Clears the trace
//-----------------------------------------------------------------------------
static void Collision_ClearTrace( const Vector &vecRayStart, const Vector &vecRayDelta, CBaseTrace *pTrace )
{
	pTrace->startpos = vecRayStart;
	pTrace->endpos = vecRayStart;
	pTrace->endpos += vecRayDelta;
	pTrace->startsolid = false;
	pTrace->allsolid = false;
	pTrace->fraction = 1.0f;
	pTrace->contents = 0;
}


//-----------------------------------------------------------------------------
// Compute the offset in t along the ray that we'll use for the collision
//-----------------------------------------------------------------------------
static float ComputeBoxOffset( const Ray_t& ray )
{
	if (ray.m_IsRay)
		return 1e-3f;

	// Find the projection of the box diagonal along the ray...
	float offset = FloatMakePositive(ray.m_Extents[0] * ray.m_Delta[0]) +
					FloatMakePositive(ray.m_Extents[1] * ray.m_Delta[1]) +
					FloatMakePositive(ray.m_Extents[2] * ray.m_Delta[2]);

	// We need to divide twice: Once to normalize the computation above
	// so we get something in units of extents, and the second to normalize
	// that with respect to the entire raycast.
	offset *= InvRSquared( ray.m_Delta );

	// 1e-3 is an epsilon
	return offset + 1e-3;
}


//-----------------------------------------------------------------------------
// Intersects a swept box against a triangle
//-----------------------------------------------------------------------------
float IntersectRayWithTriangle( const Ray_t& ray, 
		const Vector& v1, const Vector& v2, const Vector& v3, bool oneSided )
{
	// This is cute: Use barycentric coordinates to represent the triangle
	// Vo(1-u-v) + V1u + V2v and intersect that with a line Po + Dt
	// This gives us 3 equations + 3 unknowns, which we can solve with
	// Cramer's rule...
	//		E1x u + E2x v - Dx t = Pox - Vox
	// There's a couple of other optimizations, Cramer's rule involves
	// computing the determinant of a matrix which has been constructed
	// by three vectors. It turns out that 
	// det | A B C | = -( A x C ) dot B or -(C x B) dot A
	// which we'll use below..

	Vector edge1, edge2, org;
	VectorSubtract( v2, v1, edge1 );
	VectorSubtract( v3, v1, edge2 );

	// Cull out one-sided stuff
	if (oneSided)
	{
		Vector normal;
		CrossProduct( edge1, edge2, normal );
		if (DotProduct( normal, ray.m_Delta ) >= 0.0f)
			return -1.0f;
	}

	// FIXME: This is inaccurate, but fast for boxes
	// We want to do a fast separating axis implementation here
	// with a swept triangle along the reverse direction of the ray.

	// Compute some intermediary terms
	Vector dirCrossEdge2, orgCrossEdge1;
	CrossProduct( ray.m_Delta, edge2, dirCrossEdge2 );

	// Compute the denominator of Cramer's rule:
	//		| -Dx E1x E2x |
	// det	| -Dy E1y E2y | = (D x E2) dot E1
	//		| -Dz E1z E2z |
	float denom = DotProduct( dirCrossEdge2, edge1 );
	if( FloatMakePositive( denom ) < 1e-6 )
		return -1.0f;
	denom = 1.0f / denom;

	// Compute u. It's gotta lie in the range of 0 to 1.
	//				   | -Dx orgx E2x |
	// u = denom * det | -Dy orgy E2y | = (D x E2) dot org
	//				   | -Dz orgz E2z |
	VectorSubtract( ray.m_Start, v1, org );
	float u = DotProduct( dirCrossEdge2, org ) * denom;
	if ((u < 0.0f) || (u > 1.0f))
		return -1.0f;

	// Compute t and v the same way...
	// In barycentric coords, u + v < 1
	CrossProduct( org, edge1, orgCrossEdge1 );
	float v = DotProduct( orgCrossEdge1, ray.m_Delta ) * denom;
	if ((v < 0.0f) || (v + u > 1.0f))
		return -1.0f;

	// Compute the distance along the ray direction that we need to fudge 
	// when using swept boxes
	float boxt = ComputeBoxOffset( ray );
	float t = DotProduct( orgCrossEdge1, edge2 ) * denom;
	if ((t < -boxt) || (t > 1.0f + boxt))
		return -1.0f;

	return clamp( t, 0.f, 1.f );
}

//-----------------------------------------------------------------------------
// computes the barycentric coordinates of an intersection
//-----------------------------------------------------------------------------

bool ComputeIntersectionBarycentricCoordinates( const Ray_t& ray, 
		const Vector& v1, const Vector& v2, const Vector& v3, float& u, float& v,
		float *t )
{
	Vector edge1, edge2, org;
	VectorSubtract( v2, v1, edge1 );
	VectorSubtract( v3, v1, edge2 );

	// Compute some intermediary terms
	Vector dirCrossEdge2, orgCrossEdge1;
	CrossProduct( ray.m_Delta, edge2, dirCrossEdge2 );

	// Compute the denominator of Cramer's rule:
	//		| -Dx E1x E2x |
	// det	| -Dy E1y E2y | = (D x E2) dot E1
	//		| -Dz E1z E2z |
	float denom = DotProduct( dirCrossEdge2, edge1 );
	if( FloatMakePositive( denom ) < 1e-6 )
		return false;
	denom = 1.0f / denom;

	// Compute u. It's gotta lie in the range of 0 to 1.
	//				   | -Dx orgx E2x |
	// u = denom * det | -Dy orgy E2y | = (D x E2) dot org
	//				   | -Dz orgz E2z |
	VectorSubtract( ray.m_Start, v1, org );
	u = DotProduct( dirCrossEdge2, org ) * denom;

	// Compute t and v the same way...
	// In barycentric coords, u + v < 1
	CrossProduct( org, edge1, orgCrossEdge1 );
	v = DotProduct( orgCrossEdge1, ray.m_Delta ) * denom;

	// Compute the distance along the ray direction that we need to fudge 
	// when using swept boxes
	if( t )
	{
		float boxt = ComputeBoxOffset( ray );
		*t = DotProduct( orgCrossEdge1, edge2 ) * denom;
		if( ( *t < -boxt ) || ( *t > 1.0f + boxt ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Intersects a plane with a triangle (requires barycentric definition)
//-----------------------------------------------------------------------------

int IntersectTriangleWithPlaneBarycentric( const Vector& org, const Vector& edgeU,
		const Vector& edgeV, const Vector4D& plane, Vector2D* pIntersection )
{
	// This uses a barycentric method, since we need that to determine
	// interpolated points, alphas, and normals
	// Given the plane equation P dot N + d = 0
	// and the barycentric coodinate equation P = Org + EdgeU * u + EdgeV * v
	// Plug em in. Intersection occurs at u = 0 or v = 0 or u + v = 1

	float orgDotNormal = DotProduct( org, plane.AsVector3D() );
	float edgeUDotNormal = DotProduct( edgeU, plane.AsVector3D() );
	float edgeVDotNormal = DotProduct( edgeV, plane.AsVector3D() );

	int ptIdx = 0;

	// u = 0
	if ( edgeVDotNormal != 0.0f )
	{
		pIntersection[ptIdx].x = 0.0f;
		pIntersection[ptIdx].y = - ( orgDotNormal - plane.w ) / edgeVDotNormal;
		if ((pIntersection[ptIdx].y >= 0.0f) && (pIntersection[ptIdx].y <= 1.0f))
			++ptIdx;
	}

	// v = 0
	if ( edgeUDotNormal != 0.0f )
	{
		pIntersection[ptIdx].x = - ( orgDotNormal - plane.w ) / edgeUDotNormal;
		pIntersection[ptIdx].y = 0.0f;
		if ((pIntersection[ptIdx].x >= 0.0f) && (pIntersection[ptIdx].x <= 1.0f))
			++ptIdx;
	}

	// u + v = 1
	if (ptIdx == 2)
		return ptIdx;

	if ( edgeVDotNormal != edgeUDotNormal )
	{
		pIntersection[ptIdx].x = - ( orgDotNormal - plane.w + edgeVDotNormal) / 
			( edgeUDotNormal - edgeVDotNormal);
		pIntersection[ptIdx].y = 1.0f - pIntersection[ptIdx].x;
		if ((pIntersection[ptIdx].x >= 0.0f) && (pIntersection[ptIdx].x <= 1.0f) &&
			 (pIntersection[ptIdx].y >= 0.0f) && (pIntersection[ptIdx].y <= 1.0f))
			++ptIdx;
	}

	Assert( ptIdx < 3 );
	return ptIdx;
}


//-----------------------------------------------------------------------------
// Returns true if a box intersects with a sphere
//-----------------------------------------------------------------------------
bool IsSphereIntersectingSphere( const Vector& center1, float radius1, 
								 const Vector& center2, float radius2 )
{
	Vector delta;
	VectorSubtract( center2, center1, delta );
	float distSq = delta.LengthSqr();
	float radiusSum = radius1 + radius2;
	return (distSq <= (radiusSum * radiusSum));
}


//-----------------------------------------------------------------------------
// Returns true if a box intersects with a sphere
//-----------------------------------------------------------------------------
bool IsBoxIntersectingSphere( const Vector& boxMin, const Vector& boxMax, 
						const Vector& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta;

	// Unrolled the loop.. this is a big cycle stealer...
	if (center[0] < boxMin[0])
	{
		flDelta = center[0] - boxMin[0];
		dmin += flDelta * flDelta;
	}
	else if (center[0] > boxMax[0])
	{
		flDelta = boxMax[0] - center[0];
		dmin += flDelta * flDelta;
	}

	if (center[1] < boxMin[1])
	{
		flDelta = center[1] - boxMin[1];
		dmin += flDelta * flDelta;
	}
	else if (center[1] > boxMax[1])
	{
		flDelta = boxMax[1] - center[1];
		dmin += flDelta * flDelta;
	}

	if (center[2] < boxMin[2])
	{
		flDelta = center[2] - boxMin[2];
		dmin += flDelta * flDelta;
	}
	else if (center[2] > boxMax[2])
	{
		flDelta = boxMax[2] - center[2];
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}

bool IsBoxIntersectingSphereExtents( const Vector& boxCenter, const Vector& boxHalfDiag, 
						const Vector& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta, flDiff;

	// Unrolled the loop.. this is a big cycle stealer...
	flDiff = FloatMakePositive( center.x - boxCenter.x );
	if (flDiff > boxHalfDiag.x)
	{
		flDelta = flDiff - boxHalfDiag.x;
		dmin += flDelta * flDelta;
	}

	flDiff = FloatMakePositive( center.y - boxCenter.y );
	if (flDiff > boxHalfDiag.y)
	{
		flDelta = flDiff - boxHalfDiag.y;
		dmin += flDelta * flDelta;
	}

	flDiff = FloatMakePositive( center.z - boxCenter.z );
	if (flDiff > boxHalfDiag.z)
	{
		flDelta = flDiff - boxHalfDiag.z;
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}


//-----------------------------------------------------------------------------
// Returns true if a rectangle intersects with a circle
//-----------------------------------------------------------------------------
bool IsCircleIntersectingRectangle( const Vector2D& boxMin, const Vector2D& boxMax, 
						      const Vector2D& center, float radius )
{
	// See Graphics Gems, box-sphere intersection
	float dmin = 0.0f;
	float flDelta;

	if (center[0] < boxMin[0])
	{
		flDelta = center[0] - boxMin[0];
		dmin += flDelta * flDelta;
	}
	else if (center[0] > boxMax[0])
	{
		flDelta = boxMax[0] - center[0];
		dmin += flDelta * flDelta;
	}

	if (center[1] < boxMin[1])
	{
		flDelta = center[1] - boxMin[1];
		dmin += flDelta * flDelta;
	}
	else if (center[1] > boxMax[1])
	{
		flDelta = boxMax[1] - center[1];
		dmin += flDelta * flDelta;
	}

	return dmin < radius * radius;
}


//-----------------------------------------------------------------------------
// returns true if there's an intersection between ray and sphere
//-----------------------------------------------------------------------------
bool IsRayIntersectingSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
		const Vector& vecCenter, float flRadius, float flTolerance )
{
	// For this algorithm, find a point on the ray  which is closest to the sphere origin
	// Do this by making a plane passing through the sphere origin
	// whose normal is parallel to the ray. Intersect that plane with the ray.
	// Plane: N dot P = I, N = D (ray direction), I = C dot N = C dot D
	// Ray: P = O + D * t
	// D dot ( O + D * t ) = C dot D
	// D dot O + D dot D * t = C dot D
	// t = (C - O) dot D / D dot D
	// Clamp t to (0,1)
	// Find distance of the point on the ray to the sphere center.
	Assert( flTolerance >= 0.0f );
	flRadius += flTolerance;

	Vector vecRayToSphere;
	VectorSubtract( vecCenter, vecRayOrigin, vecRayToSphere );
	float flNumerator = DotProduct( vecRayToSphere, vecRayDelta );
	
	float t;
	if (flNumerator <= 0.0f)
	{
		t = 0.0f;
	}
	else
	{
		float flDenominator = DotProduct( vecRayDelta, vecRayDelta );
		if ( flNumerator > flDenominator )
			t = 1.0f;
		else
			t = flNumerator / flDenominator;
	}
	
	Vector vecClosestPoint;
	VectorMA( vecRayOrigin, t, vecRayDelta, vecClosestPoint );
	return ( vecClosestPoint.DistToSqr( vecCenter ) <= flRadius * flRadius );

	// NOTE: This in an alternate algorithm which I didn't use because I'd have to use a sqrt
	// So it's probably faster to do this other algorithm. I'll leave the comments here
	// for how to go back if we want to

	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;
	// Valid solutions are possible only if b^2 - 4ac >= 0
	// Therefore, compute that value + see if we got it	
}


//-----------------------------------------------------------------------------
//
// IntersectInfiniteRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points
//
//-----------------------------------------------------------------------------
bool IntersectInfiniteRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 )
{
	// Solve using the ray equation + the sphere equation
	// P = o + dt
	// (x - xc)^2 + (y - yc)^2 + (z - zc)^2 = r^2
	// (ox + dx * t - xc)^2 + (oy + dy * t - yc)^2 + (oz + dz * t - zc)^2 = r^2
	// (ox - xc)^2 + 2 * (ox-xc) * dx * t + dx^2 * t^2 +
	//		(oy - yc)^2 + 2 * (oy-yc) * dy * t + dy^2 * t^2 +
	//		(oz - zc)^2 + 2 * (oz-zc) * dz * t + dz^2 * t^2 = r^2
	// (dx^2 + dy^2 + dz^2) * t^2 + 2 * ((ox-xc)dx + (oy-yc)dy + (oz-zc)dz) t +
	//		(ox-xc)^2 + (oy-yc)^2 + (oz-zc)^2 - r^2 = 0
	// or, t = (-b +/- sqrt( b^2 - 4ac)) / 2a
	// a = DotProduct( vecRayDelta, vecRayDelta );
	// b = 2 * DotProduct( vecRayOrigin - vecCenter, vecRayDelta )
	// c = DotProduct(vecRayOrigin - vecCenter, vecRayOrigin - vecCenter) - flRadius * flRadius;

	Vector vecSphereToRay;
	VectorSubtract(	vecRayOrigin, vecSphereCenter, vecSphereToRay );

	float a = DotProduct( vecRayDelta, vecRayDelta );

	// This would occur in the case of a zero-length ray
	if ( a == 0.0f )
	{
		*pT1 = *pT2 = 0.0f;
		return vecSphereToRay.LengthSqr() <= flRadius * flRadius;
	}

	float b = 2 * DotProduct( vecSphereToRay, vecRayDelta );
	float c = DotProduct( vecSphereToRay, vecSphereToRay ) - flRadius * flRadius;
	float flDiscrim = b * b - 4 * a * c;
	if ( flDiscrim < 0.0f )
		return false;

	flDiscrim = sqrt( flDiscrim );
	float oo2a = 0.5f / a;
	*pT1 = ( - b - flDiscrim ) * oo2a;
	*pT2 = ( - b + flDiscrim ) * oo2a;
	return true;
}



//-----------------------------------------------------------------------------
//
// IntersectRayWithSphere
//
// Returns whether or not there was an intersection. 
// Returns the two intersection points, clamped to (0,1)
//
//-----------------------------------------------------------------------------
bool IntersectRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 )
{
	if ( !IntersectInfiniteRayWithSphere( vecRayOrigin, vecRayDelta, vecSphereCenter, flRadius, pT1, pT2 ) )
		return false;

	if (( *pT1 > 1.0f ) || ( *pT2 < 0.0f ))
		return false;

	// Clamp it!
	if ( *pT1 < 0.0f )
		*pT1 = 0.0f;
	if ( *pT2 > 1.0f )
		*pT2 = 1.0f;

	return true;
}


// returns true if the sphere and cone intersect
// NOTE: cone sine/cosine are the half angle of the cone
bool IsSphereIntersectingCone( const Vector &sphereCenter, float sphereRadius, const Vector &coneOrigin, const Vector &coneNormal, float coneSine, float coneCosine )
{
	Vector backCenter = coneOrigin - (sphereRadius / coneSine) * coneNormal;
	Vector delta = sphereCenter - backCenter;
	float deltaLen = delta.Length();
	if ( DotProduct(coneNormal, delta) >= deltaLen*coneCosine )
	{
		delta = sphereCenter - coneOrigin;
		deltaLen = delta.Length();
		if ( -DotProduct(coneNormal, delta) >= deltaLen * coneSine )
		{
			return ( deltaLen <= sphereRadius ) ? true : false;
		}
		return true;
	}
	return false;
}



//-----------------------------------------------------------------------------
// returns true if the point is in the box
//-----------------------------------------------------------------------------
bool IsPointInBox( const Vector& pt, const Vector& boxMin, const Vector& boxMax )
{
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// on x360, force use of SIMD version.
	if (IsX360())
	{
		return IsPointInBox( LoadUnaligned3SIMD(pt.Base()), LoadUnaligned3SIMD(boxMin.Base()), LoadUnaligned3SIMD(boxMax.Base()) ) ;
	}

	if ( (pt[0] > boxMax[0]) || (pt[0] < boxMin[0]) )
		return false;
	if ( (pt[1] > boxMax[1]) || (pt[1] < boxMin[1]) )
		return false;
	if ( (pt[2] > boxMax[2]) || (pt[2] < boxMin[2]) )
		return false;
	return true;
}


bool IsPointInCone( const Vector &pt, const Vector &origin, const Vector &axis, float cosAngle, float length )
{
	Vector delta = pt - origin;
	float dist = VectorNormalize( delta );
	float dot = DotProduct( delta, axis );
	if ( dot < cosAngle )
		return false;
	if ( dist * dot > length )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// returns true if there's an intersection between two boxes
//-----------------------------------------------------------------------------
bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
						const Vector& boxMin2, const Vector& boxMax2 )
{
	Assert( boxMin1[0] <= boxMax1[0] );
	Assert( boxMin1[1] <= boxMax1[1] );
	Assert( boxMin1[2] <= boxMax1[2] );
	Assert( boxMin2[0] <= boxMax2[0] );
	Assert( boxMin2[1] <= boxMax2[1] );
	Assert( boxMin2[2] <= boxMax2[2] );

	if ( (boxMin1[0] > boxMax2[0]) || (boxMax1[0] < boxMin2[0]) )
		return false;
	if ( (boxMin1[1] > boxMax2[1]) || (boxMax1[1] < boxMin2[1]) )
		return false;
	if ( (boxMin1[2] > boxMax2[2]) || (boxMax1[2] < boxMin2[2]) )
		return false;
	return true;
}

bool IsBoxIntersectingBoxExtents( const Vector& boxCenter1, const Vector& boxHalfDiagonal1, 
						   const Vector& boxCenter2, const Vector& boxHalfDiagonal2 )
{
	Vector vecDelta, vecSize;
	VectorSubtract( boxCenter1, boxCenter2, vecDelta );
	VectorAdd( boxHalfDiagonal1, boxHalfDiagonal2, vecSize );
	return ( FloatMakePositive( vecDelta.x ) <= vecSize.x ) &&
			( FloatMakePositive( vecDelta.y ) <= vecSize.y ) &&
			( FloatMakePositive( vecDelta.z ) <= vecSize.z );
}


//-----------------------------------------------------------------------------
// 
// IsOBBIntersectingOBB
//
// returns true if there's an intersection between two OBBs
//
//-----------------------------------------------------------------------------
bool IsOBBIntersectingOBB( const Vector &vecOrigin1, const QAngle &vecAngles1, const Vector& boxMin1, const Vector& boxMax1, 
						   const Vector &vecOrigin2, const QAngle &vecAngles2, const Vector& boxMin2, const Vector& boxMax2, float flTolerance )
{
	// FIXME: Simple case AABB check doesn't work because the min and max extents are not oriented based on the angle
	// this fast check would only be good for cubes.
	/*if ( vecAngles1 == vecAngles2 )
	{
		const Vector &vecDelta = vecOrigin2 - vecOrigin1;
		Vector vecOtherMins, vecOtherMaxs;
		VectorAdd( boxMin2, vecDelta, vecOtherMins );
		VectorAdd( boxMax2, vecDelta, vecOtherMaxs );
		return IsBoxIntersectingBox( boxMin1, boxMax1, vecOtherMins, vecOtherMaxs );
	}*/

	// OBB test...
	cplane_t plane;
	bool bFoundPlane = ComputeSeparatingPlane( vecOrigin1, vecAngles1, boxMin1, boxMax1, 
		vecOrigin2, vecAngles2, boxMin2, boxMax2, flTolerance, &plane );
	return (bFoundPlane == false);
}

// NOTE: This is only very slightly faster on high end PCs and x360
#define USE_SIMD_RAY_CHECKS 1
//-----------------------------------------------------------------------------
// returns true if there's an intersection between box and ray
//-----------------------------------------------------------------------------
bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& vecDelta, float flTolerance )
{
	
#if USE_SIMD_RAY_CHECKS
	// Load the unaligned ray/box parameters into SIMD registers
	fltx4 start = LoadUnaligned3SIMD(origin.Base());
	fltx4 delta = LoadUnaligned3SIMD(vecDelta.Base());
	fltx4 boxMins = LoadUnaligned3SIMD( boxMin.Base() );
	fltx4 boxMaxs = LoadUnaligned3SIMD( boxMax.Base() );
	fltx4 epsilon = ReplicateX4(flTolerance);
	// compute the mins/maxs of the box expanded by the ray extents
	// relocate the problem so that the ray start is at the origin.
	fltx4 offsetMins = SubSIMD(boxMins, start);
	fltx4 offsetMaxs = SubSIMD(boxMaxs, start);
	fltx4 offsetMinsExpanded = SubSIMD(offsetMins, epsilon);
	fltx4 offsetMaxsExpanded = AddSIMD(offsetMaxs, epsilon);

	// Check to see if both the origin (start point) and the end point (delta) are on the front side
	// of any of the box sides - if so there can be no intersection
	fltx4 startOutMins = CmpLtSIMD(Four_Zeros, offsetMinsExpanded);
	fltx4 endOutMins = CmpLtSIMD(delta,offsetMinsExpanded);
	fltx4 minsMask = AndSIMD( startOutMins, endOutMins );
	fltx4 startOutMaxs = CmpGtSIMD(Four_Zeros, offsetMaxsExpanded);
	fltx4 endOutMaxs = CmpGtSIMD(delta,offsetMaxsExpanded);
	fltx4 maxsMask = AndSIMD( startOutMaxs, endOutMaxs );
	if ( IsAnyNegative(SetWToZeroSIMD(OrSIMD(minsMask,maxsMask))))
		return false;

	// now build the per-axis interval of t for intersections
	fltx4 invDelta = ReciprocalSaturateSIMD(delta);
	fltx4 tmins = MulSIMD( offsetMinsExpanded, invDelta );
	fltx4 tmaxs = MulSIMD( offsetMaxsExpanded, invDelta );
	fltx4 crossPlane = OrSIMD(XorSIMD(startOutMins,endOutMins), XorSIMD(startOutMaxs,endOutMaxs));

	// only consider axes where we crossed a plane
	tmins = MaskedAssign( crossPlane, tmins, Four_Negative_FLT_MAX );
	tmaxs = MaskedAssign( crossPlane, tmaxs, Four_FLT_MAX );

	// now sort the interval per axis
	fltx4 mint = MinSIMD( tmins, tmaxs );
	fltx4 maxt = MaxSIMD( tmins, tmaxs );

	// now find the intersection of the intervals on all axes
	fltx4 firstOut = FindLowestSIMD3(maxt);
	fltx4 lastIn = FindHighestSIMD3(mint);
	// NOTE: This is really a scalar quantity now [t0,t1] == [lastIn,firstOut]
	firstOut = MinSIMD(firstOut, Four_Ones);
	lastIn = MaxSIMD(lastIn, Four_Zeros);

	// If the final interval is valid lastIn<firstOut, check for separation
	fltx4 separation = CmpGtSIMD(lastIn, firstOut);

	return IsAllZeros(separation);
#else
	// On the x360, we force use of the SIMD functions.
#if defined(_X360) 
	if (IsX360())
	{
		fltx4 delta = LoadUnaligned3SIMD(vecDelta.Base());
		return IsBoxIntersectingRay( 
			LoadUnaligned3SIMD(boxMin.Base()), LoadUnaligned3SIMD(boxMax.Base()),
			LoadUnaligned3SIMD(origin.Base()), delta, ReciprocalSIMD(delta), // ray parameters
			ReplicateX4(flTolerance) ///< eg from ReplicateX4(flTolerance)
			);
	}
#endif
	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// FIXME: Surely there's a faster way
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for (int i = 0; i < 3; ++i)
	{
		// Parallel case...
		if (FloatMakePositive(vecDelta[i]) < 1e-8)
		{
			// Check that origin is in the box
			// if not, then it doesn't intersect..
			if ( (origin[i] < boxMin[i] - flTolerance) || (origin[i] > boxMax[i] + flTolerance) )
				return false;

			continue;
		}

		// non-parallel case
		// Find the t's corresponding to the entry and exit of
		// the ray along x, y, and z. The find the furthest entry
		// point, and the closest exit point. Once that is done,
		// we know we don't collide if the closest exit point
		// is behind the starting location. We also don't collide if
		// the closest exit point is in front of the furthest entry point

		float invDelta = 1.0f / vecDelta[i];
		float t1 = (boxMin[i] - flTolerance - origin[i]) * invDelta;
		float t2 = (boxMax[i] + flTolerance - origin[i]) * invDelta;
		if (t1 > t2)
		{
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}
		if (t1 > tmin)
			tmin = t1;
		if (t2 < tmax)
			tmax = t2;
		if (tmin > tmax)
			return false;
		if (tmax < 0)
			return false;
		if (tmin > 1)
			return false;
	}

	return true;
#endif
}

//-----------------------------------------------------------------------------
// returns true if there's an intersection between box and ray
//-----------------------------------------------------------------------------
bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& vecDelta,
									const Vector& vecInvDelta, float flTolerance )
{	
#if USE_SIMD_RAY_CHECKS
	// Load the unaligned ray/box parameters into SIMD registers
	fltx4 start = LoadUnaligned3SIMD(origin.Base());
	fltx4 delta = LoadUnaligned3SIMD(vecDelta.Base());
	fltx4 boxMins = LoadUnaligned3SIMD( boxMin.Base() );
	fltx4 boxMaxs = LoadUnaligned3SIMD( boxMax.Base() );
	// compute the mins/maxs of the box expanded by the ray extents
	// relocate the problem so that the ray start is at the origin.
	boxMins = SubSIMD(boxMins, start);
	boxMaxs = SubSIMD(boxMaxs, start);

	// Check to see if both the origin (start point) and the end point (delta) are on the front side
	// of any of the box sides - if so there can be no intersection
	fltx4 startOutMins = CmpLtSIMD(Four_Zeros, boxMins);
	fltx4 endOutMins = CmpLtSIMD(delta,boxMins);
	fltx4 minsMask = AndSIMD( startOutMins, endOutMins );
	fltx4 startOutMaxs = CmpGtSIMD(Four_Zeros, boxMaxs);
	fltx4 endOutMaxs = CmpGtSIMD(delta,boxMaxs);
	fltx4 maxsMask = AndSIMD( startOutMaxs, endOutMaxs );
	if ( IsAnyNegative(SetWToZeroSIMD(OrSIMD(minsMask,maxsMask))))
		return false;

	// now build the per-axis interval of t for intersections
	fltx4 epsilon = ReplicateX4(flTolerance);
	fltx4 invDelta = LoadUnaligned3SIMD(vecInvDelta.Base());
	boxMins = SubSIMD(boxMins, epsilon);
	boxMaxs = AddSIMD(boxMaxs, epsilon);

	boxMins = MulSIMD( boxMins, invDelta );
	boxMaxs = MulSIMD( boxMaxs, invDelta );

	fltx4 crossPlane = OrSIMD(XorSIMD(startOutMins,endOutMins), XorSIMD(startOutMaxs,endOutMaxs));
	// only consider axes where we crossed a plane
	boxMins = MaskedAssign( crossPlane, boxMins, Four_Negative_FLT_MAX );
	boxMaxs = MaskedAssign( crossPlane, boxMaxs, Four_FLT_MAX );

	// now sort the interval per axis
	fltx4 mint = MinSIMD( boxMins, boxMaxs );
	fltx4 maxt = MaxSIMD( boxMins, boxMaxs );

	// now find the intersection of the intervals on all axes
	fltx4 firstOut = FindLowestSIMD3(maxt);
	fltx4 lastIn = FindHighestSIMD3(mint);
	// NOTE: This is really a scalar quantity now [t0,t1] == [lastIn,firstOut]
	firstOut = MinSIMD(firstOut, Four_Ones);
	lastIn = MaxSIMD(lastIn, Four_Zeros);

	// If the final interval is valid lastIn<firstOut, check for separation
	fltx4 separation = CmpGtSIMD(lastIn, firstOut);

	return IsAllZeros(separation);
#else
	// On the x360, we force use of the SIMD functions.
#if defined(_X360) && !defined(PARANOID_SIMD_ASSERTING)
	if (IsX360())
	{
		return IsBoxIntersectingRay( 
			LoadUnaligned3SIMD(boxMin.Base()), LoadUnaligned3SIMD(boxMax.Base()),
			LoadUnaligned3SIMD(origin.Base()), LoadUnaligned3SIMD(vecDelta.Base()), LoadUnaligned3SIMD(vecInvDelta.Base()), // ray parameters
			ReplicateX4(flTolerance) ///< eg from ReplicateX4(flTolerance)
			);
	}
#endif

	Assert( boxMin[0] <= boxMax[0] );
	Assert( boxMin[1] <= boxMax[1] );
	Assert( boxMin[2] <= boxMax[2] );

	// FIXME: Surely there's a faster way
	float tmin = -FLT_MAX;
	float tmax = FLT_MAX;

	for ( int i = 0; i < 3; ++i )
	{
		// Parallel case...
		if ( FloatMakePositive( vecDelta[i] ) < 1e-8 )
		{
			// Check that origin is in the box, if not, then it doesn't intersect..
			if ( ( origin[i] < boxMin[i] - flTolerance ) || ( origin[i] > boxMax[i] + flTolerance ) )
				return false;

			continue;
		}

		// Non-parallel case
		// Find the t's corresponding to the entry and exit of
		// the ray along x, y, and z. The find the furthest entry
		// point, and the closest exit point. Once that is done,
		// we know we don't collide if the closest exit point
		// is behind the starting location. We also don't collide if
		// the closest exit point is in front of the furthest entry point
		float t1 = ( boxMin[i] - flTolerance - origin[i] ) * vecInvDelta[i];
		float t2 = ( boxMax[i] + flTolerance - origin[i] ) * vecInvDelta[i];
		if ( t1 > t2 )
		{
			float temp = t1;
			t1 = t2;
			t2 = temp;
		}

		if (t1 > tmin)
			tmin = t1;

		if (t2 < tmax)
			tmax = t2;

		if (tmin > tmax)
			return false;

		if (tmax < 0)
			return false;

		if (tmin > 1)
			return false;
	}

	return true;
#endif
}

//-----------------------------------------------------------------------------
// Intersects a ray with a aabb, return true if they intersect
//-----------------------------------------------------------------------------
bool FASTCALL IsBoxIntersectingRay( const Vector& vecBoxMin, const Vector& vecBoxMax, const Ray_t& ray, float flTolerance )
{
	// On the x360, we force use of the SIMD functions.
#if defined(_X360) 
	if (IsX360())
	{
		return IsBoxIntersectingRay( 
			LoadUnaligned3SIMD(vecBoxMin.Base()), LoadUnaligned3SIMD(vecBoxMax.Base()),
			ray, flTolerance);
	}
#endif

	if ( !ray.m_IsSwept )
	{
		Vector rayMins, rayMaxs;
		VectorSubtract( ray.m_Start, ray.m_Extents, rayMins );
		VectorAdd( ray.m_Start, ray.m_Extents, rayMaxs );
		if ( flTolerance != 0.0f )
		{
			rayMins.x -= flTolerance; rayMins.y -= flTolerance; rayMins.z -= flTolerance;
			rayMaxs.x += flTolerance; rayMaxs.y += flTolerance; rayMaxs.z += flTolerance;
		}
		return IsBoxIntersectingBox( vecBoxMin, vecBoxMax, rayMins, rayMaxs );
	}

	Vector vecExpandedBoxMin, vecExpandedBoxMax;
	VectorSubtract( vecBoxMin, ray.m_Extents, vecExpandedBoxMin );
	VectorAdd( vecBoxMax, ray.m_Extents, vecExpandedBoxMax );
	return IsBoxIntersectingRay( vecExpandedBoxMin, vecExpandedBoxMax, ray.m_Start, ray.m_Delta, flTolerance );
}


//-----------------------------------------------------------------------------
// returns true if there's an intersection between box and ray (SIMD version)
//-----------------------------------------------------------------------------


#ifdef _X360
bool FASTCALL IsBoxIntersectingRay( fltx4 boxMin, fltx4 boxMax, 
								    fltx4 origin, fltx4 delta, fltx4 invDelta, // ray parameters
									fltx4 vTolerance ///< eg from ReplicateX4(flTolerance)
									)
#else
bool FASTCALL IsBoxIntersectingRay( const fltx4 &inBoxMin, const fltx4 & inBoxMax, 
								   const fltx4 & origin, const fltx4 & delta, const fltx4 & invDelta, // ray parameters
								   const fltx4 & vTolerance ///< eg from ReplicateX4(flTolerance)
								   )
#endif
{
	// Load the unaligned ray/box parameters into SIMD registers
	// compute the mins/maxs of the box expanded by the ray extents
	// relocate the problem so that the ray start is at the origin.

#ifdef _X360
	boxMin = SubSIMD(boxMin, origin);
	boxMax = SubSIMD(boxMax, origin);
#else
	fltx4 boxMin = SubSIMD(inBoxMin, origin);
	fltx4 boxMax = SubSIMD(inBoxMax, origin);
#endif

	// Check to see if the origin (start point) and the end point (delta) are on the same side
	// of any of the box sides - if so there can be no intersection
	fltx4 startOutMins = AndSIMD( CmpLtSIMD(Four_Zeros, boxMin), CmpLtSIMD(delta,boxMin) );
	fltx4 startOutMaxs = AndSIMD( CmpGtSIMD(Four_Zeros, boxMax), CmpGtSIMD(delta,boxMax) );
	if ( IsAnyNegative(SetWToZeroSIMD(OrSIMD(startOutMaxs,startOutMins))))
		return false;

	// now build the per-axis interval of t for intersections
	boxMin = SubSIMD(boxMin, vTolerance);
	boxMax = AddSIMD(boxMax, vTolerance);

	boxMin = MulSIMD( boxMin, invDelta );
	boxMax = MulSIMD( boxMax, invDelta );

	// now sort the interval per axis
	fltx4 mint = MinSIMD( boxMin, boxMax );
	fltx4 maxt = MaxSIMD( boxMin, boxMax );

	// now find the intersection of the intervals on all axes
	fltx4 firstOut = FindLowestSIMD3(maxt);
	fltx4 lastIn = FindHighestSIMD3(mint);
	// NOTE: This is really a scalar quantity now [t0,t1] == [lastIn,firstOut]
	firstOut = MinSIMD(firstOut, Four_Ones);
	lastIn = MaxSIMD(lastIn, Four_Zeros);

	// If the final interval is valid lastIn<firstOut, check for separation
	fltx4 separation = CmpGtSIMD(lastIn, firstOut);

	return IsAllZeros(separation);
}


bool FASTCALL IsBoxIntersectingRay( const fltx4& boxMin, const fltx4& boxMax, 
								   const Ray_t& ray, float flTolerance )
{
	fltx4 vTolerance = ReplicateX4(flTolerance);
	fltx4 rayStart = LoadAlignedSIMD(ray.m_Start);
	fltx4 rayExtents = LoadAlignedSIMD(ray.m_Extents);
	if ( !ray.m_IsSwept )
	{

		fltx4 rayMins, rayMaxs;
		rayMins = SubSIMD(rayStart, rayExtents);
		rayMaxs = AddSIMD(rayStart, rayExtents);
		rayMins = AddSIMD(rayMins, vTolerance);
		rayMaxs = AddSIMD(rayMaxs, vTolerance);

		VectorAligned vecBoxMin, vecBoxMax, vecRayMins, vecRayMaxs;
		StoreAlignedSIMD( vecBoxMin.Base(), boxMin );
		StoreAlignedSIMD( vecBoxMax.Base(), boxMax );
		StoreAlignedSIMD( vecRayMins.Base(), rayMins );
		StoreAlignedSIMD( vecRayMaxs.Base(), rayMaxs );

		return IsBoxIntersectingBox( vecBoxMin, vecBoxMax, vecRayMins, vecRayMaxs );
	}

	fltx4 rayDelta = LoadAlignedSIMD(ray.m_Delta);
	fltx4 vecExpandedBoxMin, vecExpandedBoxMax;
	vecExpandedBoxMin = SubSIMD( boxMin, rayExtents );
	vecExpandedBoxMax = AddSIMD( boxMax, rayExtents );

	return IsBoxIntersectingRay( vecExpandedBoxMin, vecExpandedBoxMax, rayStart, rayDelta, ReciprocalSIMD(rayDelta), ReplicateX4(flTolerance) );
}


//-----------------------------------------------------------------------------
// Intersects a ray with a ray, return true if they intersect
// t, s = parameters of closest approach (if not intersecting!)
//-----------------------------------------------------------------------------
bool IntersectRayWithRay( const Ray_t &ray0, const Ray_t &ray1, float &t, float &s )
{
	Assert( ray0.m_IsRay && ray1.m_IsRay );

	//
	// r0 = p0 + v0t
	// r1 = p1 + v1s
	//
	// intersection : r0 = r1 :: p0 + v0t = p1 + v1s
	// NOTE: v(0,1) are unit direction vectors
	//
	// subtract p0 from both sides and cross with v1 (NOTE: v1 x v1 = 0)
	//  (v0 x v1)t = ((p1 - p0 ) x v1)
	//
	// dotting  with (v0 x v1) and dividing by |v0 x v1|^2
	//	t = Det | (p1 - p0) , v1 , (v0 x v1) | / |v0 x v1|^2
	//  s = Det | (p1 - p0) , v0 , (v0 x v1) | / |v0 x v1|^2
	//
	//  Det | A B C | = -( A x C ) dot B or -( C x B ) dot A
	// 
	//  NOTE: if |v0 x v1|^2 = 0, then the lines are parallel
	//
	Vector v0( ray0.m_Delta );
	Vector v1( ray1.m_Delta );
	VectorNormalize( v0 );
	VectorNormalize( v1 );

	Vector v0xv1 = v0.Cross( v1 );
	float lengthSq = v0xv1.LengthSqr();
	if( lengthSq == 0.0f )
	{
		t = 0; s = 0;
		return false;		// parallel
	}

	Vector p1p0 = ray1.m_Start - ray0.m_Start;

	Vector AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detT = AxC.Dot( v1 );
	
	AxC = p1p0.Cross( v0xv1 );
	AxC.Negate();
	float detS = AxC.Dot( v0 );

	t = detT / lengthSq;
	s = detS / lengthSq;

	// intersection????
	Vector i0, i1;
	i0 = v0 * t;
	i1 = v1 * s;
	i0 += ray0.m_Start;
	i1 += ray1.m_Start;
	if( i0.x == i1.x && i0.y == i1.y && i0.z == i1.z )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Intersects a ray with a plane, returns distance t along ray.
//-----------------------------------------------------------------------------
float IntersectRayWithPlane( const Ray_t& ray, const cplane_t& plane )
{
	float denom	= DotProduct( ray.m_Delta, plane.normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (plane.dist - DotProduct( ray.m_Start, plane.normal )) * denom;
}

float IntersectRayWithPlane( const Vector& org, const Vector& dir, const cplane_t& plane )
{
	float denom	= DotProduct( dir, plane.normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (plane.dist - DotProduct( org, plane.normal )) * denom;
}

float IntersectRayWithPlane( const Vector& org, const Vector& dir, const Vector& normal, float dist )
{
	float denom	= DotProduct( dir, normal );
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (dist - DotProduct( org, normal )) * denom;
}

float IntersectRayWithAAPlane( const Vector& vecStart, const Vector& vecEnd, int nAxis, float flSign, float flDist )
{
	float denom	= flSign * (vecEnd[nAxis] - vecStart[nAxis]);
	if (denom == 0.0f)
		return 0.0f;

	denom = 1.0f / denom;
	return (flDist - flSign * vecStart[nAxis]) * denom;
}


//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, BoxTraceInfo_t *pTrace )
{
	int			i;
	float		d1, d2;
	float		f;

	pTrace->t1 = -1.0f;
	pTrace->t2 = 1.0f;
	pTrace->hitside = -1;

	// UNDONE: This makes this code a little messy
	pTrace->startsolid = true;

	for ( i = 0; i < 6; ++i )
	{
		if ( i >= 3 )
		{
			d1 = vecRayStart[i-3] - boxMaxs[i-3];
			d2 = d1 + vecRayDelta[i-3];
		}
		else
		{
			d1 = -vecRayStart[i] + boxMins[i];
			d2 = d1 - vecRayDelta[i];
		}

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 > 0)
		{
			// UNDONE: Have to revert this in case it's still set
			// UNDONE: Refactor to have only 2 return points (true/false) from this function
			pTrace->startsolid = false;
			return false;
		}

		// completely inside, check next face
		if (d1 <= 0 && d2 <= 0)
			continue;

		if (d1 > 0)
		{
			pTrace->startsolid = false;
		}

		// crosses face
		if (d1 > d2)
		{
			f = d1 - flTolerance;
			if ( f < 0 )
			{
				f = 0;
			}
			f = f / (d1-d2);
			if (f > pTrace->t1)
			{
				pTrace->t1 = f;
				pTrace->hitside = i;
			}
		}
		else
		{ 
			// leave
			f = (d1 + flTolerance) / (d1-d2);
			if (f < pTrace->t2)
			{
				pTrace->t2 = f;
			}
		}
	}

	return pTrace->startsolid || (pTrace->t1 < pTrace->t2 && pTrace->t1 >= 0.0f);
}


//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid )
{
	Collision_ClearTrace( vecRayStart, vecRayDelta, pTrace );

	BoxTraceInfo_t trace;

	if ( IntersectRayWithBox( vecRayStart, vecRayDelta, boxMins, boxMaxs, flTolerance, &trace ) )
	{
		pTrace->startsolid = trace.startsolid;
		if (trace.t1 < trace.t2 && trace.t1 >= 0.0f)
		{
			pTrace->fraction = trace.t1;
			VectorMA( pTrace->startpos, trace.t1, vecRayDelta, pTrace->endpos );
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.normal = vec3_origin;
			if ( trace.hitside >= 3 )
			{
				trace.hitside -= 3;
				pTrace->plane.dist = boxMaxs[trace.hitside];
				pTrace->plane.normal[trace.hitside] = 1.0f;
				pTrace->plane.type = trace.hitside;
			}
			else
			{
				pTrace->plane.dist = -boxMins[trace.hitside];
				pTrace->plane.normal[trace.hitside] = -1.0f;
				pTrace->plane.type = trace.hitside;
			}
			return true;
		}

		if ( pTrace->startsolid )
		{
			pTrace->allsolid = (trace.t2 <= 0.0f) || (trace.t2 >= 1.0f);
			pTrace->fraction = 0;
			if ( pFractionLeftSolid )
			{
				*pFractionLeftSolid = trace.t2;
			}
			pTrace->endpos = pTrace->startpos;
			pTrace->contents = CONTENTS_SOLID;
			pTrace->plane.dist = pTrace->startpos[0];
			pTrace->plane.normal.Init( 1.0f, 0.0f, 0.0f );
			pTrace->plane.type = 0;
			pTrace->startpos = vecRayStart + (trace.t2 * vecRayDelta);
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Ray_t &ray, const Vector &boxMins, const Vector &boxMaxs, 
						 float flTolerance, CBaseTrace *pTrace, float *pFractionLeftSolid )
{
	if ( !ray.m_IsRay )
	{
		Vector vecExpandedMins = boxMins - ray.m_Extents;
		Vector vecExpandedMaxs = boxMaxs + ray.m_Extents;
		bool bIntersects = IntersectRayWithBox( ray.m_Start, ray.m_Delta, vecExpandedMins, vecExpandedMaxs, flTolerance, pTrace, pFractionLeftSolid );
		pTrace->startpos += ray.m_StartOffset;
		pTrace->endpos += ray.m_StartOffset;
		return bIntersects;
	}
	return IntersectRayWithBox( ray.m_Start, ray.m_Delta, boxMins, boxMaxs, flTolerance, pTrace, pFractionLeftSolid );
}


//-----------------------------------------------------------------------------
// Intersects a ray against an OBB, returns t1 and t2
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, BoxTraceInfo_t *pTrace )
{
	// FIXME: Two transforms is pretty expensive. Should we optimize this?
	Vector start, delta;
	VectorITransform( vecRayStart, matOBBToWorld, start );
	VectorIRotate( vecRayDelta, matOBBToWorld, delta );

	return IntersectRayWithBox( start, delta, vecOBBMins, vecOBBMaxs, flTolerance, pTrace ); 
}



//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, CBaseTrace *pTrace )
{
	Collision_ClearTrace( vecRayStart, vecRayDelta, pTrace );

	// FIXME: Make it work with tolerance
	Assert( flTolerance == 0.0f );

	// OPTIMIZE: Store this in the box instead of computing it here
	// compute center in local space
	Vector vecBoxExtents = (vecOBBMins + vecOBBMaxs) * 0.5; 
	Vector vecBoxCenter;

	// transform to world space
	VectorTransform( vecBoxExtents, matOBBToWorld, vecBoxCenter );

	// calc extents from local center
	vecBoxExtents = vecOBBMaxs - vecBoxExtents;

	// OPTIMIZE: This is optimized for world space.  If the transform is fast enough, it may make more
	// sense to just xform and call UTIL_ClipToBox() instead.  MEASURE THIS.

	// save the extents of the ray along 
	Vector extent, uextent;
	Vector segmentCenter = vecRayStart + vecRayDelta - vecBoxCenter;

	extent.Init();

	// check box axes for separation
	for ( int j = 0; j < 3; j++ )
	{
		extent[j] = vecRayDelta.x * matOBBToWorld[0][j] + vecRayDelta.y * matOBBToWorld[1][j] +	vecRayDelta.z * matOBBToWorld[2][j];
		uextent[j] = fabsf(extent[j]);
		float coord = segmentCenter.x * matOBBToWorld[0][j] + segmentCenter.y * matOBBToWorld[1][j] +	segmentCenter.z * matOBBToWorld[2][j];
		coord = fabsf(coord);

		if ( coord > (vecBoxExtents[j] + uextent[j]) )
			return false;
	}

	// now check cross axes for separation
	float tmp, cextent;
	Vector cross = vecRayDelta.Cross( segmentCenter );
	cextent = cross.x * matOBBToWorld[0][0] + cross.y * matOBBToWorld[1][0] + cross.z * matOBBToWorld[2][0];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[1]*uextent[2] + vecBoxExtents[2]*uextent[1];
	if ( cextent > tmp )
		return false;

	cextent = cross.x * matOBBToWorld[0][1] + cross.y * matOBBToWorld[1][1] + cross.z * matOBBToWorld[2][1];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[0]*uextent[2] + vecBoxExtents[2]*uextent[0];
	if ( cextent > tmp )
		return false;

	cextent = cross.x * matOBBToWorld[0][2] + cross.y * matOBBToWorld[1][2] + cross.z * matOBBToWorld[2][2];
	cextent = fabsf(cextent);
	tmp = vecBoxExtents[0]*uextent[1] + vecBoxExtents[1]*uextent[0];
	if ( cextent > tmp )
		return false;

	// !!! We hit this box !!! compute intersection point and return
	// Compute ray start in bone space
	Vector start;
	VectorITransform( vecRayStart, matOBBToWorld, start );

	// extent is ray.m_Delta in bone space, recompute delta in bone space
	extent *= 2.0f;

	// delta was prescaled by the current t, so no need to see if this intersection
	// is closer
	if ( !IntersectRayWithBox( start, extent, vecOBBMins, vecOBBMaxs, flTolerance, pTrace ) )
		return false;

	// Fix up the start/end pos and fraction
	Vector vecTemp;
	VectorTransform( pTrace->endpos, matOBBToWorld, vecTemp );
	pTrace->endpos = vecTemp;

	pTrace->startpos = vecRayStart;
	pTrace->fraction *= 2.0f;

	// Fix up the plane information
	float flSign = pTrace->plane.normal[ pTrace->plane.type ];
	pTrace->plane.normal[0] = flSign * matOBBToWorld[0][pTrace->plane.type];
	pTrace->plane.normal[1] = flSign * matOBBToWorld[1][pTrace->plane.type];
	pTrace->plane.normal[2] = flSign * matOBBToWorld[2][pTrace->plane.type];
	pTrace->plane.dist = DotProduct( pTrace->endpos, pTrace->plane.normal );
	pTrace->plane.type = 3;

	return true;
}


//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace )
{
	if (angBoxRotation == vec3_angle)
	{
		Vector vecAbsMins, vecAbsMaxs;
		VectorAdd( vecBoxOrigin, vecOBBMins, vecAbsMins );
		VectorAdd( vecBoxOrigin, vecOBBMaxs, vecAbsMaxs );
		return IntersectRayWithBox( vecRayOrigin, vecRayDelta, vecAbsMins, vecAbsMaxs, flTolerance, pTrace ); 
	}

	matrix3x4_t obbToWorld;
	AngleMatrix( angBoxRotation, vecBoxOrigin, obbToWorld );
	return IntersectRayWithOBB( vecRayOrigin, vecRayDelta, obbToWorld, vecOBBMins, vecOBBMaxs, flTolerance, pTrace );
}


//-----------------------------------------------------------------------------
// Box support map
//-----------------------------------------------------------------------------
inline void ComputeSupportMap( const Vector &vecDirection, const Vector &vecBoxMins, 
				const Vector &vecBoxMaxs, float pDist[2] )
{
	int nIndex = (vecDirection.x > 0.0f);
	pDist[nIndex] = vecBoxMaxs.x * vecDirection.x;
	pDist[1 - nIndex] = vecBoxMins.x * vecDirection.x;

	nIndex = (vecDirection.y > 0.0f);
	pDist[nIndex] += vecBoxMaxs.y * vecDirection.y;
	pDist[1 - nIndex] += vecBoxMins.y * vecDirection.y;

	nIndex = (vecDirection.z > 0.0f);
	pDist[nIndex] += vecBoxMaxs.z * vecDirection.z;
	pDist[1 - nIndex] += vecBoxMins.z * vecDirection.z;
}

inline void ComputeSupportMap( const Vector &vecDirection, int i1, int i2, 
	const Vector &vecBoxMins, const Vector &vecBoxMaxs, float pDist[2] )
{
	int nIndex = (vecDirection[i1] > 0.0f);
	pDist[nIndex] = vecBoxMaxs[i1] * vecDirection[i1];
	pDist[1 - nIndex] = vecBoxMins[i1] * vecDirection[i1];

	nIndex = (vecDirection[i2] > 0.0f);
	pDist[nIndex] += vecBoxMaxs[i2] * vecDirection[i2];
	pDist[1 - nIndex] += vecBoxMins[i2] * vecDirection[i2];
}

//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
static int s_ExtIndices[3][2] = 
{
	{ 2, 1 },
	{ 0, 2 },
	{ 0, 1 },
};

static int s_MatIndices[3][2] = 
{
	{ 1, 2 },
	{ 2, 0 },
	{ 1, 0 },
};

bool IntersectRayWithOBB( const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace )
{
	if ( ray.m_IsRay )
	{
		return IntersectRayWithOBB( ray.m_Start, ray.m_Delta, matOBBToWorld, 
			vecOBBMins, vecOBBMaxs, flTolerance, pTrace );
	}

	Collision_ClearTrace( ray.m_Start + ray.m_StartOffset, ray.m_Delta, pTrace );

	// Compute a bounding sphere around the bloated OBB
	Vector vecOBBCenter;
	VectorAdd( vecOBBMins, vecOBBMaxs, vecOBBCenter );
	vecOBBCenter *= 0.5f;
	vecOBBCenter.x += matOBBToWorld[0][3];
	vecOBBCenter.y += matOBBToWorld[1][3];
	vecOBBCenter.z += matOBBToWorld[2][3];

	Vector vecOBBHalfDiagonal;
	VectorSubtract( vecOBBMaxs, vecOBBMins, vecOBBHalfDiagonal );
	vecOBBHalfDiagonal *= 0.5f;

	float flRadius = vecOBBHalfDiagonal.Length() + ray.m_Extents.Length();
	if ( !IsRayIntersectingSphere( ray.m_Start, ray.m_Delta, vecOBBCenter, flRadius, flTolerance ) )
		return false;

	// Ok, we passed the trivial reject, so lets do the dirty deed.
	// Basically we're going to do the GJK thing explicitly. We'll shrink the ray down
	// to a point, and bloat the OBB by the ray's extents. This will generate facet
	// planes which are perpendicular to all of the separating axes typically seen in
	// a standard seperating axis implementation.

	// We're going to create a number of planes through various vertices in the OBB
	// which represent all of the separating planes. Then we're going to bloat the planes
	// by the ray extents.
	
	// We're going to do all work in OBB-space because it's easier to do the
	// support-map in this case

	// First, transform the ray into the space of the OBB
	Vector vecLocalRayOrigin, vecLocalRayDirection;
	VectorITransform( ray.m_Start, matOBBToWorld, vecLocalRayOrigin );
	VectorIRotate( ray.m_Delta, matOBBToWorld, vecLocalRayDirection );

	// Next compute all separating planes
	Vector pPlaneNormal[15];
	float ppPlaneDist[15][2];

	int i;
	for ( i = 0; i < 3; ++i )
	{
		// Each plane needs to be bloated an amount = to the abs dot product of 
		// the ray extents with the plane normal
		// For the OBB planes, do it in world space; 
		// and use the direction of the OBB (the ith column of matOBBToWorld) in world space vs extents
		pPlaneNormal[i].Init( );
		pPlaneNormal[i][i] = 1.0f;

		float flExtentDotNormal = 
			FloatMakePositive( matOBBToWorld[0][i] * ray.m_Extents.x ) +
			FloatMakePositive( matOBBToWorld[1][i] * ray.m_Extents.y ) +
			FloatMakePositive( matOBBToWorld[2][i] * ray.m_Extents.z );

		ppPlaneDist[i][0] = vecOBBMins[i] - flExtentDotNormal; 
		ppPlaneDist[i][1] = vecOBBMaxs[i] + flExtentDotNormal;

		// For the ray-extents planes, they are bloated by the extents
		// Use the support map to determine which
		VectorCopy( matOBBToWorld[i], pPlaneNormal[i+3].Base() ); 
		ComputeSupportMap( pPlaneNormal[i+3], vecOBBMins, vecOBBMaxs, ppPlaneDist[i+3] );
		ppPlaneDist[i+3][0] -= ray.m_Extents[i];
		ppPlaneDist[i+3][1] += ray.m_Extents[i];

		// Now the edge cases... (take the cross product of x,y,z axis w/ ray extent axes
		// given by the rows of the obb to world matrix. 
		// Compute the ray extent bloat in world space because it's easier...

		// These are necessary to compute the world-space versions of
		// the edges so we can compute the extent dot products
		float flRayExtent0 = ray.m_Extents[s_ExtIndices[i][0]];
		float flRayExtent1 = ray.m_Extents[s_ExtIndices[i][1]];
		const float *pMatRow0 = matOBBToWorld[s_MatIndices[i][0]];
		const float *pMatRow1 = matOBBToWorld[s_MatIndices[i][1]];

		// x axis of the OBB + world ith axis
		pPlaneNormal[i+6].Init( 0.0f, -matOBBToWorld[i][2], matOBBToWorld[i][1] );
		ComputeSupportMap( pPlaneNormal[i+6], 1, 2, vecOBBMins, vecOBBMaxs, ppPlaneDist[i+6] );
		flExtentDotNormal = 
			FloatMakePositive( pMatRow0[0] ) * flRayExtent0 +
			FloatMakePositive( pMatRow1[0] ) * flRayExtent1;
		ppPlaneDist[i+6][0] -= flExtentDotNormal;
		ppPlaneDist[i+6][1] += flExtentDotNormal;
		
		// y axis of the OBB + world ith axis
		pPlaneNormal[i+9].Init( matOBBToWorld[i][2], 0.0f, -matOBBToWorld[i][0] );
		ComputeSupportMap( pPlaneNormal[i+9], 0, 2, vecOBBMins, vecOBBMaxs, ppPlaneDist[i+9] );
		flExtentDotNormal = 
			FloatMakePositive( pMatRow0[1] ) * flRayExtent0 +
			FloatMakePositive( pMatRow1[1] ) * flRayExtent1;
		ppPlaneDist[i+9][0] -= flExtentDotNormal;
		ppPlaneDist[i+9][1] += flExtentDotNormal;

		// z axis of the OBB + world ith axis
		pPlaneNormal[i+12].Init( -matOBBToWorld[i][1], matOBBToWorld[i][0], 0.0f );
		ComputeSupportMap( pPlaneNormal[i+12], 0, 1, vecOBBMins, vecOBBMaxs, ppPlaneDist[i+12] );
		flExtentDotNormal = 
			FloatMakePositive( pMatRow0[2] ) * flRayExtent0 +
			FloatMakePositive( pMatRow1[2] ) * flRayExtent1;
		ppPlaneDist[i+12][0] -= flExtentDotNormal;
		ppPlaneDist[i+12][1] += flExtentDotNormal;
	}

	float enterfrac, leavefrac;
	float d1[2], d2[2];
	float f;

	int hitplane = -1;
	int hitside = -1;
	enterfrac = -1.0f;
	leavefrac = 1.0f;

	pTrace->startsolid = true;

	Vector vecLocalRayEnd;
	VectorAdd( vecLocalRayOrigin, vecLocalRayDirection, vecLocalRayEnd );

	for ( i = 0; i < 15; ++i )
	{
		// FIXME: Not particularly optimal since there's a lot of 0's in the plane normals
		float flStartDot = DotProduct( pPlaneNormal[i], vecLocalRayOrigin );
		float flEndDot = DotProduct( pPlaneNormal[i], vecLocalRayEnd );

		// NOTE: Negative here is because the plane normal + dist 
		// are defined in negative terms for the far plane (plane dist index 0)
		d1[0] = -(flStartDot - ppPlaneDist[i][0]);
		d2[0] = -(flEndDot - ppPlaneDist[i][0]);

		d1[1] = flStartDot - ppPlaneDist[i][1];
		d2[1] = flEndDot - ppPlaneDist[i][1];

		int j;
		for ( j = 0; j < 2; ++j )
		{
			// if completely in front near plane or behind far plane no intersection
			if (d1[j] > 0 && d2[j] > 0)
				return false;

			// completely inside, check next plane set
			if (d1[j] <= 0 && d2[j] <= 0)
				continue;

			if (d1[j] > 0)
			{
				pTrace->startsolid = false;
			}

			// crosses face
			float flDenom = 1.0f / (d1[j] - d2[j]);
			if (d1[j] > d2[j])
			{
				f = d1[j] - flTolerance;
				if ( f < 0 )
				{
					f = 0;
				}
				f *= flDenom;
				if (f > enterfrac)
				{
					enterfrac = f;
					hitplane = i;
					hitside = j;
				}
			}
			else
			{ 
				// leave
				f = (d1[j] + flTolerance) * flDenom;
				if (f < leavefrac)
				{
					leavefrac = f;
				}
			}
		}
	}

	if (enterfrac < leavefrac && enterfrac >= 0.0f)
	{
		pTrace->fraction = enterfrac;
		VectorMA( pTrace->startpos, enterfrac, ray.m_Delta, pTrace->endpos );
		pTrace->contents = CONTENTS_SOLID;

		// Need to transform the plane into world space...
		cplane_t temp;
		temp.normal = pPlaneNormal[hitplane];
		temp.dist = ppPlaneDist[hitplane][hitside];
		if (hitside == 0)
		{
			temp.normal *= -1.0f;
			temp.dist *= -1.0f;
		}
		temp.type = 3;

		MatrixITransformPlane( matOBBToWorld, temp, pTrace->plane );
		return true;
	}

	if ( pTrace->startsolid )
	{
		pTrace->allsolid = (leavefrac <= 0.0f) || (leavefrac >= 1.0f);
		pTrace->fraction = 0;
		pTrace->endpos = pTrace->startpos;
		pTrace->contents = CONTENTS_SOLID;
		pTrace->plane.dist = pTrace->startpos[0];
		pTrace->plane.normal.Init( 1.0f, 0.0f, 0.0f );
		pTrace->plane.type = 0;
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Intersects a ray against an OBB
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB( const Ray_t &ray, const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace )
{
	if ( angBoxRotation == vec3_angle )
	{
		Vector vecWorldMins, vecWorldMaxs;
		VectorAdd( vecBoxOrigin, vecOBBMins, vecWorldMins );
		VectorAdd( vecBoxOrigin, vecOBBMaxs, vecWorldMaxs );
		return IntersectRayWithBox( ray, vecWorldMins, vecWorldMaxs, flTolerance, pTrace );
	}

	if ( ray.m_IsRay )
	{
		return IntersectRayWithOBB( ray.m_Start, ray.m_Delta, vecBoxOrigin, angBoxRotation, vecOBBMins, vecOBBMaxs, flTolerance, pTrace );
	}

	matrix3x4_t matOBBToWorld;
	AngleMatrix( angBoxRotation, vecBoxOrigin, matOBBToWorld );
	return IntersectRayWithOBB( ray, matOBBToWorld, vecOBBMins, vecOBBMaxs, flTolerance, pTrace );
}

	
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void GetNonMajorAxes( const Vector &vNormal, Vector2D &axes )
{
	axes[0] = 0;
	axes[1] = 1;

	if( FloatMakePositive( vNormal.x ) > FloatMakePositive( vNormal.y ) )
	{
		if( FloatMakePositive( vNormal.x ) > FloatMakePositive( vNormal.z ) )
		{
			axes[0] = 1;
			axes[1] = 2;
		}
	}
	else
	{
		if( FloatMakePositive( vNormal.y ) > FloatMakePositive( vNormal.z ) )
		{
			axes[0] = 0;
			axes[1] = 2;
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
QuadBarycentricRetval_t QuadWithParallelEdges( const Vector &vecOrigin, 
	const Vector &vecU, float lengthU, const Vector &vecV, float lengthV,
	const Vector &pt, Vector2D &vecUV )
{
	Ray_t rayAxis;
	Ray_t rayPt;

	//
	// handle the u axis
	//
	rayAxis.m_Start = vecOrigin;
	rayAxis.m_Delta = vecU;
	rayAxis.m_IsRay = true;

	rayPt.m_Start = pt;
	rayPt.m_Delta = vecV * -( lengthV * 10.0f );
	rayPt.m_IsRay = true;

	float s, t;
	IntersectRayWithRay( rayAxis, rayPt, t, s );
	vecUV[0] = t / lengthU;

	//
	// handle the v axis
	//
	rayAxis.m_Delta = vecV;

	rayPt.m_Delta = vecU * -( lengthU * 10.0f );

	IntersectRayWithRay( rayAxis, rayPt, t, s );
	vecUV[1] = t / lengthV;

	// inside of the quad??
	if( ( vecUV[0] < 0.0f ) || ( vecUV[0] > 1.0f ) || 
		( vecUV[1] < 0.0f ) || ( vecUV[1] > 1.0f ) )
		return BARY_QUADRATIC_FALSE;

	return BARY_QUADRATIC_TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ResolveQuadratic( double tPlus, double tMinus, 
					   const Vector axisU0, const Vector axisU1,
					   const Vector axisV0, const Vector axisV1,
					   const Vector axisOrigin, const Vector pt,
					   int projU, double &s, double &t )
{
	// calculate the sPlus, sMinus pair(s)
	double sDenomPlus = ( axisU0[projU] * ( 1 - tPlus ) ) + ( axisU1[projU] * tPlus );
	double sDenomMinus = ( axisU0[projU] * ( 1 - tMinus ) ) + ( axisU1[projU] * tMinus );

	double sPlus = UNINIT, sMinus = UNINIT;
	if( FloatMakePositive( sDenomPlus ) >= 1e-5 )
	{
		sPlus = ( pt[projU] - axisOrigin[projU] - ( axisV0[projU] * tPlus ) ) / sDenomPlus; 
	}

	if( FloatMakePositive( sDenomMinus ) >= 1e-5 )
	{
		sMinus = ( pt[projU] - axisOrigin[projU] - ( axisV0[projU] * tMinus ) ) / sDenomMinus; 
	}
	
	if( ( tPlus >= 0.0 ) && ( tPlus <= 1.0 ) && ( sPlus >= 0.0 ) && ( sPlus <= 1.0 ) )
	{
		s = sPlus;
		t = tPlus;
		return;
	}

	if( ( tMinus >= 0.0 ) && ( tMinus <= 1.0 ) && ( sMinus >= 0.0 ) && ( sMinus <= 1.0 ) )
	{
		s = sMinus;
		t = tMinus;
		return;
	}

	double s0, t0, s1, t1;

	s0 = sPlus;
	t0 = tPlus;
	if( s0 >= 1.0 ) { s0 -= 1.0; }
	if( t0 >= 1.0 ) { t0 -= 1.0; }

	s1 = sMinus;
	t1 = tMinus;
	if( s1 >= 1.0 ) { s1 -= 1.0; }
	if( t1 >= 1.0 ) { t1 -= 1.0; }

	s0 = FloatMakePositive( s0 );
	t0 = FloatMakePositive( t0 );
	s1 = FloatMakePositive( s1 );
	t1 = FloatMakePositive( t1 );

	double max0, max1;
	max0 = s0;
	if( t0 > max0 ) { max0 = t0; }
	max1 = s1;
	if( t1 > max1 ) { max1 = t1; }

	if( max0 > max1 )
	{
		s = sMinus;
		t = tMinus;
	}
	else
	{
		s = sPlus;
		t = tPlus;
	}
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

QuadBarycentricRetval_t PointInQuadToBarycentric( const Vector &v1, const Vector &v2, 
	const Vector &v3, const Vector &v4, const Vector &point, Vector2D &uv )
{
#define PIQ_TEXTURE_EPSILON		0.001
#define PIQ_PLANE_EPSILON		0.1
#define PIQ_DOT_EPSILON			0.99f

	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// u0 = v2 - v1
	// u1 = v3 - v4
	// v0 = v4 - v1
	// v1 = v3 - v2
	//
	Vector axisU[2], axisV[2];
	Vector axisUNorm[2], axisVNorm[2];
	axisU[0] = axisUNorm[0] = v2 - v1;
	axisU[1] = axisUNorm[1] = v3 - v4;
	axisV[0] = axisVNorm[0] = v4 - v1;
	axisV[1] = axisVNorm[1] = v3 - v2;

	float lengthU[2], lengthV[2];
	lengthU[0] = VectorNormalize( axisUNorm[0] );
	lengthU[1] = VectorNormalize( axisUNorm[1] );
	lengthV[0] = VectorNormalize( axisVNorm[0] );
	lengthV[1] = VectorNormalize( axisVNorm[1] );

	//
	// check for an early out - parallel opposite edges!
	// NOTE: quad property if 1 set of opposite edges is parallel and equal
	//       in length, then the other set of edges is as well
	//
	if( axisUNorm[0].Dot( axisUNorm[1] ) > PIQ_DOT_EPSILON )
	{
		if( FloatMakePositive( lengthU[0] - lengthU[1] ) < PIQ_PLANE_EPSILON )
		{
			return QuadWithParallelEdges( v1, axisUNorm[0], lengthU[0], axisVNorm[0], lengthV[0], point, uv );
		}
	}

	//
	// since we are solving for s in our equations below we need to ensure that
	// the v axes are non-parallel
	//
	bool bFlipped = false;
	if( axisVNorm[0].Dot( axisVNorm[1] ) > PIQ_DOT_EPSILON )
	{
		Vector tmp[2];
		tmp[0] = axisV[0];
		tmp[1] = axisV[1];
		axisV[0] = axisU[0];
		axisV[1] = axisU[1];
		axisU[0] = tmp[0];
		axisU[1] = tmp[1];
		bFlipped = true;
	}

	//
	// get the "projection" axes
	//
	Vector2D projAxes;
	Vector vNormal = axisU[0].Cross( axisV[0] );
	GetNonMajorAxes( vNormal, projAxes );

	//
	// NOTE: axisU[0][projAxes[0]] < axisU[0][projAxes[1]], 
	//       this is done to decrease error when dividing later
	//
	if( FloatMakePositive( axisU[0][projAxes[0]] ) < FloatMakePositive( axisU[0][projAxes[1]] ) )
	{
		int tmp = projAxes[0];
		projAxes[0] = projAxes[1];
		projAxes[1] = tmp;
	}

	// Here's how we got these equations:
	//
	// Given the points and u,v line segments above...
	//
	// Then:
	//
	// (1.0) PT = P0 + U0 * s + V * t
	//
	// where
	//
	// (1.1) V = V0 + s * (V1 - V0)
	// (1.2) U = U0 + t * (U1 - U0)
	// 
	// Therefore (from 1.1 + 1.0):
	// PT - P0 = U0 * s + (V0 + s * (V1-V0)) * t
	// Group s's:
	// PT - P0 - t * V0 = s * (U0 + t * (V1-V0))
	// Two equations and two unknowns in x and y get you the following quadratic:
	//
	// solve the quadratic
	//
	double s = 0.0, t = 0.0;
	double A, negB, C;

	A = ( axisU[0][projAxes[1]] * axisV[0][projAxes[0]] ) - 
		( axisU[0][projAxes[0]] * axisV[0][projAxes[1]] ) - 
		( axisU[1][projAxes[1]] * axisV[0][projAxes[0]] ) + 
		( axisU[1][projAxes[0]] * axisV[0][projAxes[1]] );
	C = ( v1[projAxes[1]] * axisU[0][projAxes[0]] ) - 
		( point[projAxes[1]] * axisU[0][projAxes[0]] ) - 
		( v1[projAxes[0]] * axisU[0][projAxes[1]] ) + 
		( point[projAxes[0]] * axisU[0][projAxes[1]] );
	negB = C - 
		  ( v1[projAxes[1]] * axisU[1][projAxes[0]] ) + 
		  ( point[projAxes[1]] * axisU[1][projAxes[0]] ) + 
		  ( v1[projAxes[0]] * axisU[1][projAxes[1]] ) - 
		  ( point[projAxes[0]] * axisU[1][projAxes[1]] ) + 
		  ( axisU[0][projAxes[1]] * axisV[0][projAxes[0]] ) - 
		  ( axisU[0][projAxes[0]] * axisV[0][projAxes[1]] );

	if( ( A > -PIQ_PLANE_EPSILON ) && ( A < PIQ_PLANE_EPSILON ) )
	{
		// shouldn't be here -- this should have been take care of in the "early out"
//		Assert( 0 );

		Vector vecUAvg, vecVAvg;
		vecUAvg = ( axisUNorm[0] + axisUNorm[1] ) * 0.5f;
		vecVAvg = ( axisVNorm[0] + axisVNorm[1] ) * 0.5f;
		
		float fLengthUAvg = ( lengthU[0] + lengthU[1] ) * 0.5f;
		float fLengthVAvg = ( lengthV[0] + lengthV[1] ) * 0.5f;

		return QuadWithParallelEdges( v1, vecUAvg, fLengthUAvg, vecVAvg, fLengthVAvg, point, uv );

#if 0
		// legacy code -- kept here for completeness!

		// not a quadratic -- solve linearly
		t = C / negB;

		// See (1.2) above
		float ui = axisU[0][projAxes[0]] + t * ( axisU[1][projAxes[0]] - axisU[0][projAxes[0]] );
		if( FloatMakePositive( ui ) >= 1e-5 )
		{
			// See (1.0) above
			s = ( point[projAxes[0]] - v1[projAxes[0]] - axisV[0][projAxes[0]] * t ) / ui;
		}
#endif
	}
	else
	{
		// (-b +/- sqrt( b^2 - 4ac )) / 2a
		double discriminant = (negB*negB) - (4.0f * A * C);
		if( discriminant < 0.0f )
		{
			uv[0] = -99999.0f;
			uv[1] = -99999.0f;
			return BARY_QUADRATIC_NEGATIVE_DISCRIMINANT;
		}

		double quad = sqrt( discriminant );
		double QPlus = ( negB + quad ) / ( 2.0f * A );
		double QMinus = ( negB - quad ) / ( 2.0f * A );

		ResolveQuadratic( QPlus, QMinus, axisU[0], axisU[1], axisV[0], axisV[1], v1, point, projAxes[0], s, t );
	}

	if( !bFlipped )
	{
		uv[0] = ( float )s;
		uv[1] = ( float )t;
	}
	else
	{
		uv[0] = ( float )t;
		uv[1] = ( float )s;
	}

	// inside of the quad??
	if( ( uv[0] < 0.0f ) || ( uv[0] > 1.0f ) || ( uv[1] < 0.0f ) || ( uv[1] > 1.0f ) )
		return BARY_QUADRATIC_FALSE;
	
	return BARY_QUADRATIC_TRUE;

#undef PIQ_TEXTURE_EPSILON
#undef PIQ_PLANE_EPSILON
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void PointInQuadFromBarycentric( const Vector &v1, const Vector &v2, const Vector &v3, const Vector &v4,
								 const Vector2D &uv, Vector &point )
{
	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// find the ray from v0 edge to v1 edge at v
	//
	Vector vPts[2];
	VectorLerp( v1, v4, uv[1], vPts[0] );
	VectorLerp( v2, v3, uv[1], vPts[1] );
	VectorLerp( vPts[0], vPts[1], uv[0], point ); 
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void TexCoordInQuadFromBarycentric( const Vector2D &v1, const Vector2D &v2, const Vector2D &v3, const Vector2D &v4,
								    const Vector2D &uv, Vector2D &texCoord )
{
	//
	// Think of a quad with points v1, v2, v3, v4 and u, v line segments
	// find the ray from v0 edge to v1 edge at v
	//
	Vector2D vCoords[2];
	Vector2DLerp( v1, v4, uv[1], vCoords[0] );
	Vector2DLerp( v2, v3, uv[1], vCoords[1] );
	Vector2DLerp( vCoords[0], vCoords[1], uv[0], texCoord ); 
}


//-----------------------------------------------------------------------------
// Compute point from barycentric specification
// Edge u goes from v0 to v1, edge v goes from v0 to v2
//-----------------------------------------------------------------------------
void ComputePointFromBarycentric( const Vector& v0, const Vector& v1, const Vector& v2, 
								 float u, float v, Vector& pt )
{
	Vector edgeU, edgeV;
	VectorSubtract( v1, v0, edgeU );
	VectorSubtract( v2, v0, edgeV );
	VectorMA( v0, u, edgeU, pt );
	VectorMA( pt, v, edgeV, pt );
}

void ComputePointFromBarycentric( const Vector2D& v0, const Vector2D& v1, const Vector2D& v2, 
								 float u, float v, Vector2D& pt )
{
	Vector2D edgeU, edgeV;
	Vector2DSubtract( v1, v0, edgeU );
	Vector2DSubtract( v2, v0, edgeV );
	Vector2DMA( v0, u, edgeU, pt );
	Vector2DMA( pt, v, edgeV, pt );
}


//-----------------------------------------------------------------------------
// Compute a matrix that has the correct orientation but which has an origin at
// the center of the bounds
//-----------------------------------------------------------------------------
static void ComputeCenterMatrix( const Vector& origin, const QAngle& angles, 
	const Vector& mins, const Vector& maxs, matrix3x4_t& matrix )
{
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= 0.5f;
	AngleMatrix( angles, matrix );

	Vector worldCentroid;
	VectorRotate( centroid, matrix, worldCentroid );
	worldCentroid += origin;
	MatrixSetColumn( worldCentroid, 3, matrix );
}

static void ComputeCenterIMatrix( const Vector& origin, const QAngle& angles, 
	const Vector& mins, const Vector& maxs, matrix3x4_t& matrix )
{
	Vector centroid;
	VectorAdd( mins, maxs, centroid );
	centroid *= -0.5f;
	AngleIMatrix( angles, matrix );

	// For the translational component here, note that the origin in world space
	// is T = R * C + O, (R = rotation matrix, C = centroid in local space, O = origin in world space)
	// The IMatrix translation = - transpose(R) * T = -C - transpose(R) * 0
	Vector localOrigin;
	VectorRotate( origin, matrix, localOrigin );
	centroid -= localOrigin;
	MatrixSetColumn( centroid, 3, matrix );
}


//-----------------------------------------------------------------------------
// Compute a matrix which is the absolute value of another
//-----------------------------------------------------------------------------
static inline void ComputeAbsMatrix( const matrix3x4_t& in, matrix3x4_t& out )
{
	FloatBits(out[0][0]) = FloatAbsBits(in[0][0]);
	FloatBits(out[0][1]) = FloatAbsBits(in[0][1]);
	FloatBits(out[0][2]) = FloatAbsBits(in[0][2]);
	FloatBits(out[1][0]) = FloatAbsBits(in[1][0]);
	FloatBits(out[1][1]) = FloatAbsBits(in[1][1]);
	FloatBits(out[1][2]) = FloatAbsBits(in[1][2]);
	FloatBits(out[2][0]) = FloatAbsBits(in[2][0]);
	FloatBits(out[2][1]) = FloatAbsBits(in[2][1]);
	FloatBits(out[2][2]) = FloatAbsBits(in[2][2]);
}


//-----------------------------------------------------------------------------
// Compute a separating plane between two boxes (expensive!)
// Returns false if no separating plane exists
//-----------------------------------------------------------------------------
static bool ComputeSeparatingPlane( const matrix3x4_t &worldToBox1, const matrix3x4_t &box2ToWorld, 
	const Vector& box1Size, const Vector& box2Size, float tolerance, cplane_t* pPlane )
{
	// The various separating planes can be either
	// 1) A plane parallel to one of the box face planes
	// 2) A plane parallel to the cross-product of an edge from each box

	// First, compute the basis of second box in the space of the first box
	// NOTE: These basis place the origin at the centroid of each box!
	matrix3x4_t	box2ToBox1;
	ConcatTransforms( worldToBox1, box2ToWorld, box2ToBox1 );

	// We're going to be using the origin of box2 in the space of box1 alot,
	// lets extract it from the matrix....
	Vector box2Origin;
	MatrixGetColumn( box2ToBox1, 3, box2Origin );

	// Next get the absolute values of these entries and store in absbox2ToBox1.
	matrix3x4_t absBox2ToBox1;
	ComputeAbsMatrix( box2ToBox1, absBox2ToBox1 );

	// There are 15 tests to make.  The first 3 involve trying planes parallel
	// to the faces of the first box.

	// NOTE: The algorithm here involves finding the projections of the two boxes
	// onto a particular line. If the projections on the line do not overlap,
	// that means that there's a plane perpendicular to the line which separates 
	// the two boxes; and we've therefore found a separating plane.

	// The way we check for overlay is we find the projections of the two boxes
	// onto the line, and add them up. We compare the sum with the projection
	// of the relative center of box2 onto the same line.

	Vector tmp;
	float boxProjectionSum;
	float originProjection;
	
	// NOTE: For these guys, we're taking advantage of the fact that the ith
	// row of the box2ToBox1 is the direction of the box1 (x,y,z)-axis
	// transformed into the space of box2.

	// First side of box 1
	boxProjectionSum = box1Size.x + MatrixRowDotProduct( absBox2ToBox1, 0, box2Size );
	originProjection = FloatMakePositive( box2Origin.x ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[0], pPlane->normal.Base() );
		return true;
	}
	
	// Second side of box 1
	boxProjectionSum = box1Size.y + MatrixRowDotProduct( absBox2ToBox1, 1, box2Size );
	originProjection = FloatMakePositive( box2Origin.y ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[1], pPlane->normal.Base() );
		return true;
	}
	
	// Third side of box 1
	boxProjectionSum = box1Size.z + MatrixRowDotProduct( absBox2ToBox1, 2, box2Size );
	originProjection = FloatMakePositive( box2Origin.z ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		VectorCopy( worldToBox1[2], pPlane->normal.Base() );
		return true;
	}
	
	// The next three involve checking splitting planes parallel to the
	// faces of the second box.

	// NOTE: For these guys, we're taking advantage of the fact that the 0th
	// column of the box2ToBox1 is the direction of the box2 x-axis
	// transformed into the space of box1.
	// Here, we're determining the distance of box2's center from box1's center
	// by projecting it onto a line parallel to box2's axis
	
	// First side of box 2
	boxProjectionSum = box2Size.x +	MatrixColumnDotProduct( absBox2ToBox1, 0, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 0, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 0, pPlane->normal );	
		return true;
	}
	
	// Second side of box 2
	boxProjectionSum = box2Size.y +	MatrixColumnDotProduct( absBox2ToBox1, 1, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 1, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 1, pPlane->normal );	
		return true;
	}
	
	// Third side of box 2
	boxProjectionSum = box2Size.z +	MatrixColumnDotProduct( absBox2ToBox1, 2, box1Size );
	originProjection = FloatMakePositive( MatrixColumnDotProduct( box2ToBox1, 2, box2Origin ) ) + tolerance;
	if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
	{
		MatrixGetColumn( box2ToWorld, 2, pPlane->normal );	
		return true;
	}
	
	// Next check the splitting planes which are orthogonal to the pairs
	// of edges, one from box1 and one from box2.  As only direction matters,
	// there are 9 pairs since each box has 3 distinct edge directions.
	
	// Here, we take advantage of the fact that the edges from box 1 are all
	// axis aligned; therefore the crossproducts are simplified. Let's walk through
	// the example of b1e1 x b2e1:

	// In this example, the line to check is perpendicular to b1e1 + b2e2
	// we can compute this line by taking the cross-product:
	//
	// [  i  j  k ]
	// [  1  0  0 ] = - ez j + ey k = l1
	// [ ex ey ez ]

	// Where ex, ey, ez is the components of box2's x axis in the space of box 1,
	// which is == to the 0th column of of box2toBox1

	// The projection of box1 onto this line = the absolute dot product of the box size
	// against the line, which =
	// AbsDot( box1Size, l1 ) = abs( -ez * box1.y ) + abs( ey * box1.z )

	// To compute the projection of box2 onto this line, we'll do it in the space of box 2
	//
	// [  i  j  k ]
	// [ fx fy fz ] = fz j - fy k = l2
	// [  1  0  0 ]

	// Where fx, fy, fz is the components of box1's x axis in the space of box 2,
	// which is == to the 0th row of of box2toBox1

	// The projection of box2 onto this line = the absolute dot product of the box size
	// against the line, which =
	// AbsDot( box2Size, l2 ) = abs( fz * box2.y ) + abs ( fy * box2.z )

	// The projection of the relative origin position on this line is done in the 
	// space of box 1:
	//
	// originProjection = DotProduct( <-ez j + ey k>, box2Origin ) =
	//		-ez * box2Origin.y + ey * box2Origin.z

	// NOTE: These checks can be bogus if both edges are parallel. The if
	// checks at the beginning of each block are designed to catch that case
	
	// b1e1 x b2e1
	if ( absBox2ToBox1[0][0] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.y * absBox2ToBox1[2][0] + box1Size.z * absBox2ToBox1[1][0] +
			box2Size.y * absBox2ToBox1[0][2] + box2Size.z * absBox2ToBox1[0][1];
		originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][0] + box2Origin.z * box2ToBox1[1][0] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 0, tmp );	
			CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e1 x b2e2
	if ( absBox2ToBox1[0][1] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.y * absBox2ToBox1[2][1] + box1Size.z * absBox2ToBox1[1][1] +
			box2Size.x * absBox2ToBox1[0][2] + box2Size.z * absBox2ToBox1[0][0];
		originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][1] + box2Origin.z * box2ToBox1[1][1] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 1, tmp );	
			CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e1 x b2e3
	if ( absBox2ToBox1[0][2] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.y * absBox2ToBox1[2][2] + box1Size.z * absBox2ToBox1[1][2] +
			box2Size.x * absBox2ToBox1[0][1] + box2Size.y * absBox2ToBox1[0][0];
		originProjection = FloatMakePositive( -box2Origin.y * box2ToBox1[2][2] + box2Origin.z * box2ToBox1[1][2] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 2, tmp );	
			CrossProduct( worldToBox1[0], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e2 x b2e1
	if ( absBox2ToBox1[1][0] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[2][0] + box1Size.z * absBox2ToBox1[0][0] +
			box2Size.y * absBox2ToBox1[1][2] + box2Size.z * absBox2ToBox1[1][1];
		originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][0] - box2Origin.z * box2ToBox1[0][0] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 0, tmp );	
			CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e2 x b2e2
	if ( absBox2ToBox1[1][1] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[2][1] + box1Size.z * absBox2ToBox1[0][1] +
			box2Size.x * absBox2ToBox1[1][2] + box2Size.z * absBox2ToBox1[1][0];
		originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][1] - box2Origin.z * box2ToBox1[0][1] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 1, tmp );	
			CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e2 x b2e3
	if ( absBox2ToBox1[1][2] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[2][2] + box1Size.z * absBox2ToBox1[0][2] +
			box2Size.x * absBox2ToBox1[1][1] + box2Size.y * absBox2ToBox1[1][0];
		originProjection = FloatMakePositive( box2Origin.x * box2ToBox1[2][2] - box2Origin.z * box2ToBox1[0][2] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 2, tmp );	
			CrossProduct( worldToBox1[1], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e3 x b2e1
	if ( absBox2ToBox1[2][0] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[1][0] + box1Size.y * absBox2ToBox1[0][0] +
			box2Size.y * absBox2ToBox1[2][2] + box2Size.z * absBox2ToBox1[2][1];
		originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][0] + box2Origin.y * box2ToBox1[0][0] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 0, tmp );	
			CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e3 x b2e2
	if ( absBox2ToBox1[2][1] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[1][1] + box1Size.y * absBox2ToBox1[0][1] +
			box2Size.x * absBox2ToBox1[2][2] + box2Size.z * absBox2ToBox1[2][0];
		originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][1] + box2Origin.y * box2ToBox1[0][1] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 1, tmp );	
			CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}

	// b1e3 x b2e3
	if ( absBox2ToBox1[2][2] < 1.0f - 1e-3f )
	{
		boxProjectionSum =
			box1Size.x * absBox2ToBox1[1][2] + box1Size.y * absBox2ToBox1[0][2] +
			box2Size.x * absBox2ToBox1[2][1] + box2Size.y * absBox2ToBox1[2][0];
		originProjection = FloatMakePositive( -box2Origin.x * box2ToBox1[1][2] + box2Origin.y * box2ToBox1[0][2] ) + tolerance;
		if ( FloatBits(originProjection) > FloatBits(boxProjectionSum) )
		{
			MatrixGetColumn( box2ToWorld, 2, tmp );	
			CrossProduct( worldToBox1[2], tmp.Base(), pPlane->normal.Base() ); 
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Compute a separating plane between two boxes (expensive!)
// Returns false if no separating plane exists
//-----------------------------------------------------------------------------
bool ComputeSeparatingPlane( const Vector& org1, const QAngle& angles1, const Vector& min1, const Vector& max1, 
	const Vector& org2, const QAngle& angles2, const Vector& min2, const Vector& max2, 
	float tolerance, cplane_t* pPlane )
{
	matrix3x4_t	worldToBox1, box2ToWorld;
	ComputeCenterIMatrix( org1, angles1, min1, max1, worldToBox1 );
	ComputeCenterMatrix( org2, angles2, min2, max2, box2ToWorld );

	// Then compute the size of the two boxes
	Vector box1Size, box2Size;
	VectorSubtract( max1, min1, box1Size );
	VectorSubtract( max2, min2, box2Size );
	box1Size *= 0.5f;
	box2Size *= 0.5f;

	return ComputeSeparatingPlane( worldToBox1, box2ToWorld, box1Size, box2Size, tolerance, pPlane );
}


//-----------------------------------------------------------------------------
// Swept OBB test
//-----------------------------------------------------------------------------
bool IsRayIntersectingOBB( const Ray_t &ray, const Vector& org, const QAngle& angles, 
						  const Vector& mins, const Vector& maxs )
{
	if ( angles == vec3_angle )
	{
		Vector vecWorldMins, vecWorldMaxs;
		VectorAdd( org, mins, vecWorldMins );
		VectorAdd( org, maxs, vecWorldMaxs );
		return IsBoxIntersectingRay( vecWorldMins, vecWorldMaxs, ray ); 
	}

	if ( ray.m_IsRay )
	{
		matrix3x4_t worldToBox;
		AngleIMatrix( angles, org, worldToBox );

		Ray_t rotatedRay;
		VectorTransform( ray.m_Start, worldToBox, rotatedRay.m_Start );
		VectorRotate( ray.m_Delta, worldToBox, rotatedRay.m_Delta );
		rotatedRay.m_StartOffset = vec3_origin;
		rotatedRay.m_Extents = vec3_origin;
		rotatedRay.m_IsRay = ray.m_IsRay;
		rotatedRay.m_IsSwept = ray.m_IsSwept;

		return IsBoxIntersectingRay( mins, maxs, rotatedRay ); 
	}

	if ( !ray.m_IsSwept )
	{
		cplane_t plane;
		return ComputeSeparatingPlane( ray.m_Start, vec3_angle, -ray.m_Extents, ray.m_Extents,
			org, angles, mins, maxs, 0.0f, &plane ) == false;
	}

	// NOTE: See the comments in ComputeSeparatingPlane to understand this math

	// First, compute the basis of box in the space of the ray
	// NOTE: These basis place the origin at the centroid of each box!
	matrix3x4_t	worldToBox1, box2ToWorld;
	ComputeCenterMatrix( org, angles, mins, maxs, box2ToWorld );

	// Find the center + extents of an AABB surrounding the ray
	Vector vecRayCenter;
	VectorMA( ray.m_Start, 0.5, ray.m_Delta, vecRayCenter );
	vecRayCenter *= -1.0f;
	SetIdentityMatrix( worldToBox1 );
	MatrixSetColumn( vecRayCenter, 3, worldToBox1 );

	Vector box1Size;
	box1Size.x = ray.m_Extents.x + FloatMakePositive( ray.m_Delta.x ) * 0.5f;
	box1Size.y = ray.m_Extents.y + FloatMakePositive( ray.m_Delta.y ) * 0.5f;
	box1Size.z = ray.m_Extents.z + FloatMakePositive( ray.m_Delta.z ) * 0.5f;

	// Then compute the size of the box
	Vector box2Size;
	VectorSubtract( maxs, mins, box2Size );
	box2Size *= 0.5f;

	// Do an OBB test of the box with the AABB surrounding the ray
	cplane_t plane;
	if ( ComputeSeparatingPlane( worldToBox1, box2ToWorld, box1Size, box2Size, 0.0f, &plane ) )
		return false;

	// Now deal with the planes which are the cross products of the ray sweep direction vs box edges
	Vector vecRayDirection = ray.m_Delta;
	VectorNormalize( vecRayDirection );

	// Need a vector between ray center vs box center measured in the space of the ray (world)
	Vector vecCenterDelta;
	vecCenterDelta.x = box2ToWorld[0][3] - ray.m_Start.x;
	vecCenterDelta.y = box2ToWorld[1][3] - ray.m_Start.y;
	vecCenterDelta.z = box2ToWorld[2][3] - ray.m_Start.z;

	// Rotate the ray direction into the space of the OBB
	Vector vecAbsRayDirBox2;
	VectorIRotate( vecRayDirection, box2ToWorld, vecAbsRayDirBox2 );

	// Make abs versions of the ray in world space + ray in box2 space
	VectorAbs( vecAbsRayDirBox2, vecAbsRayDirBox2 );

	// Now do the work for the planes which are perpendicular to the edges of the AABB
	// and the sweep direction edges...

	// In this example, the line to check is perpendicular to box edge x + ray delta
	// we can compute this line by taking the cross-product:
	//
	// [  i  j  k ]
	// [  1  0  0 ] = - dz j + dy k = l1
	// [ dx dy dz ]

	// Where dx, dy, dz is the ray delta (normalized)

	// The projection of the box onto this line = the absolute dot product of the box size
	// against the line, which =
	// AbsDot( vecBoxHalfDiagonal, l1 ) = abs( -dz * vecBoxHalfDiagonal.y ) + abs( dy * vecBoxHalfDiagonal.z )

	// Because the plane contains the sweep direction, the sweep will produce
	// no extra projection onto the line normal to the plane. 
	// Therefore all we need to do is project the ray extents onto this line also:
	// AbsDot( ray.m_Extents, l1 ) = abs( -dz * ray.m_Extents.y ) + abs( dy * ray.m_Extents.z )

	Vector vecPlaneNormal;

	// box x x ray delta
	CrossProduct( vecRayDirection, Vector( box2ToWorld[0][0], box2ToWorld[1][0], box2ToWorld[2][0] ), vecPlaneNormal );
	float flCenterDeltaProjection = FloatMakePositive( DotProduct( vecPlaneNormal, vecCenterDelta ) );
	float flBoxProjectionSum = 
		vecAbsRayDirBox2.z * box2Size.y + vecAbsRayDirBox2.y * box2Size.z +
		DotProductAbs( vecPlaneNormal, ray.m_Extents );
	if ( FloatBits(flCenterDeltaProjection) > FloatBits(flBoxProjectionSum) )
		return false;

	// box y x ray delta
	CrossProduct( vecRayDirection, Vector( box2ToWorld[0][1], box2ToWorld[1][1], box2ToWorld[2][1] ), vecPlaneNormal );
	flCenterDeltaProjection = FloatMakePositive( DotProduct( vecPlaneNormal, vecCenterDelta ) );
	flBoxProjectionSum = 
		vecAbsRayDirBox2.z * box2Size.x + vecAbsRayDirBox2.x * box2Size.z +
		DotProductAbs( vecPlaneNormal, ray.m_Extents );
	if ( FloatBits(flCenterDeltaProjection) > FloatBits(flBoxProjectionSum) )
		return false;

	// box z x ray delta
	CrossProduct( vecRayDirection, Vector( box2ToWorld[0][2], box2ToWorld[1][2], box2ToWorld[2][2] ), vecPlaneNormal );
	flCenterDeltaProjection = FloatMakePositive( DotProduct( vecPlaneNormal, vecCenterDelta ) );
	flBoxProjectionSum = 
		vecAbsRayDirBox2.y * box2Size.x + vecAbsRayDirBox2.x * box2Size.y +
		DotProductAbs( vecPlaneNormal, ray.m_Extents );
	if ( FloatBits(flCenterDeltaProjection) > FloatBits(flBoxProjectionSum) )
		return false;
	
	return true;
}

//--------------------------------------------------------------------------
// Purpose:
//
// NOTE:
//	triangle points are given in clockwise order (aabb-triangle test)
//
//    1				edge0 = 1 - 0
//    | \           edge1 = 2 - 1
//    |  \          edge2 = 0 - 2
//    |   \			.
//    |    \		.
//    0-----2		.
//
//--------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: find the minima and maxima of the 3 given values
//-----------------------------------------------------------------------------
inline void FindMinMax( float v1, float v2, float v3, float &min, float &max )
{
	min = max = v1;
	if ( v2 < min ) { min = v2; }
	if ( v2 > max ) { max = v2; }
	if ( v3 < min ) { min = v3; }
	if ( v3 > max ) { max = v3; }
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX2( float flEdgeZ, float flEdgeY, float flAbsEdgeZ, float flAbsEdgeY,
							     const Vector &p1, const Vector &p3, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialX(1,0,0) x edge ): x = 0.0f, y = edge.z, z = -edge.y
	// Triangle Point Distances: dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	float flDist1 = flEdgeZ * p1.y - flEdgeY * p1.z;
	float flDist3 = flEdgeZ * p3.y - flEdgeY * p3.z;

	// Extents are symmetric: dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	float flDistBox = flAbsEdgeZ * vecExtents.y + flAbsEdgeY * vecExtents.z;

	// Either dist1, dist3 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist1 < flDist3 )
	{
		if ( ( flDist1 > ( flDistBox + flTolerance ) ) || ( flDist3 < -( flDistBox + flTolerance ) ) )
			return false;
	}
	else
	{
		if ( ( flDist3 > ( flDistBox + flTolerance ) ) || ( flDist1 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
// Purpose:
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossX3( float flEdgeZ, float flEdgeY, float flAbsEdgeZ, float flAbsEdgeY,
								 const Vector &p1, const Vector &p2, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialX(1,0,0) x edge ): x = 0.0f, y = edge.z, z = -edge.y 
	// Triangle Point Distances: dist(x) = normal.y * pt(x).y + normal.z * pt(x).z
	float flDist1 = flEdgeZ * p1.y - flEdgeY * p1.z;
	float flDist2 = flEdgeZ * p2.y - flEdgeY * p2.z;

	// Extents are symmetric: dist = abs( normal.y ) * extents.y + abs( normal.z ) * extents.z
	float flDistBox = flAbsEdgeZ * vecExtents.y + flAbsEdgeY * vecExtents.z;

	// Either dist1, dist2 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist1 < flDist2 )
	{
		if ( ( flDist1 > ( flDistBox + flTolerance ) ) || ( flDist2 < -( flDistBox + flTolerance ) ) )
			return false;
	}
	else
	{
		if ( ( flDist2 > ( flDistBox + flTolerance ) ) || ( flDist1 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY2( float flEdgeZ, float flEdgeX, float flAbsEdgeZ, float flAbsEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialY(0,1,0) x edge ): x = -edge.z, y = 0.0f, z = edge.x
	// Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	float flDist1 = -flEdgeZ * p1.x + flEdgeX * p1.z;
	float flDist3 = -flEdgeZ * p3.x + flEdgeX * p3.z;

	// Extents are symmetric: dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	float flDistBox = flAbsEdgeZ * vecExtents.x + flAbsEdgeX * vecExtents.z;

	// Either dist1, dist3 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist1 < flDist3 )
	{
		if ( ( flDist1 > ( flDistBox + flTolerance ) ) || ( flDist3 < -( flDistBox + flTolerance ) ) )
			return false;
	}
	else
	{
		if ( ( flDist3 > ( flDistBox + flTolerance ) ) || ( flDist1 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossY3( float flEdgeZ, float flEdgeX, float flAbsEdgeZ, float flAbsEdgeX,
								 const Vector &p1, const Vector &p2, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialY(0,1,0) x edge ): x = -edge.z, y = 0.0f, z = edge.x
	// Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.z * pt(x).z
	float flDist1 = -flEdgeZ * p1.x + flEdgeX * p1.z;
	float flDist2 = -flEdgeZ * p2.x + flEdgeX * p2.z;

	// Extents are symmetric: dist = abs( normal.x ) * extents.x + abs( normal.z ) * extents.z
	float flDistBox = flAbsEdgeZ * vecExtents.x + flAbsEdgeX * vecExtents.z;

	// Either dist1, dist2 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist1 < flDist2 )
	{
		if ( ( flDist1 > ( flDistBox + flTolerance ) ) || ( flDist2 < -( flDistBox + flTolerance ) ) )
			return false;
	}
	else
	{
		if ( ( flDist2 > ( flDistBox + flTolerance ) ) || ( flDist1 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ1( float flEdgeY, float flEdgeX, float flAbsEdgeY, float flAbsEdgeX,
								 const Vector &p2, const Vector &p3, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialZ(0,0,1) x edge ): x = edge.y, y = -edge.x, z = 0.0f
	// Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	float flDist2 = flEdgeY * p2.x - flEdgeX * p2.y;
	float flDist3 = flEdgeY * p3.x - flEdgeX * p3.y;

	// Extents are symmetric: dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	float flDistBox = flAbsEdgeY * vecExtents.x + flAbsEdgeX * vecExtents.y; 

	// Either dist2, dist3 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist3 < flDist2 )
	{
		if ( ( flDist3 > ( flDistBox + flTolerance ) ) || ( flDist2 < -( flDistBox + flTolerance ) ) ) 
			return false;
	}
	else
	{
		if ( ( flDist2 > ( flDistBox + flTolerance ) ) || ( flDist3 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
inline bool AxisTestEdgeCrossZ2( float flEdgeY, float flEdgeX, float flAbsEdgeY, float flAbsEdgeX,
								 const Vector &p1, const Vector &p3, const Vector &vecExtents,
								 float flTolerance )
{
	// Cross Product( axialZ(0,0,1) x edge ): x = edge.y, y = -edge.x, z = 0.0f
	// Triangle Point Distances: dist(x) = normal.x * pt(x).x + normal.y * pt(x).y
	float flDist1 = flEdgeY * p1.x - flEdgeX * p1.y;
	float flDist3 = flEdgeY * p3.x - flEdgeX * p3.y;

	// Extents are symmetric: dist = abs( normal.x ) * extents.x + abs( normal.y ) * extents.y
	float flDistBox = flAbsEdgeY * vecExtents.x + flAbsEdgeX * vecExtents.y; 

	// Either dist1, dist3 is the closest point to the box, determine which and test of overlap with box(AABB).
	if ( flDist1 < flDist3 )
	{
		if ( ( flDist1 > ( flDistBox + flTolerance ) ) || ( flDist3 < -( flDistBox + flTolerance ) ) ) 
			return false;
	}
	else
	{
		if ( ( flDist3 > ( flDistBox + flTolerance ) ) || ( flDist1 < -( flDistBox + flTolerance ) ) )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Test for an intersection (overlap) between an axial-aligned bounding 
//          box (AABB) and a triangle.
//
// Using the "Separating-Axis Theorem" to test for intersections between
// a triangle and an axial-aligned bounding box (AABB).
// 1. 3 Axis Planes - x, y, z
// 2. 9 Edge Planes Tests - the 3 edges of the triangle crossed with all 3 axial 
//                          planes (x, y, z)
// 3. 1 Face Plane - the triangle plane (cplane_t plane below)
// Output: false = separating axis (no intersection)
//         true = intersection
//-----------------------------------------------------------------------------
bool IsBoxIntersectingTriangle( const Vector &vecBoxCenter, const Vector &vecBoxExtents,
						        const Vector &v1, const Vector &v2, const Vector &v3,
						        const cplane_t &plane, float flTolerance )
{
	// Test the axial planes (x,y,z) against the min, max of the triangle.
	float flMin, flMax;
	Vector p1, p2, p3;

	// x plane
	p1.x = v1.x - vecBoxCenter.x;
	p2.x = v2.x - vecBoxCenter.x;
	p3.x = v3.x - vecBoxCenter.x;
	FindMinMax( p1.x, p2.x, p3.x, flMin, flMax );
	if ( ( flMin > ( vecBoxExtents.x + flTolerance ) ) || ( flMax < -( vecBoxExtents.x + flTolerance ) ) ) 
		return false; 

	// y plane
	p1.y = v1.y - vecBoxCenter.y;
	p2.y = v2.y - vecBoxCenter.y;
	p3.y = v3.y - vecBoxCenter.y;
	FindMinMax( p1.y, p2.y, p3.y, flMin, flMax );
	if ( ( flMin > ( vecBoxExtents.y + flTolerance ) ) || ( flMax < -( vecBoxExtents.y + flTolerance ) ) ) 
		return false; 

	// z plane
	p1.z = v1.z - vecBoxCenter.z;
	p2.z = v2.z - vecBoxCenter.z;
	p3.z = v3.z - vecBoxCenter.z;
	FindMinMax( p1.z, p2.z, p3.z, flMin, flMax );
	if ( ( flMin > ( vecBoxExtents.z + flTolerance ) ) || ( flMax < -( vecBoxExtents.z + flTolerance ) ) ) 
		return false; 

	// Test the 9 edge cases.
	Vector vecEdge, vecAbsEdge;

	// edge 0 (cross x,y,z)
	vecEdge = p2 - p1;
	vecAbsEdge.y = FloatMakePositive( vecEdge.y );
	vecAbsEdge.z = FloatMakePositive( vecEdge.z );
	if ( !AxisTestEdgeCrossX2( vecEdge.z, vecEdge.y, vecAbsEdge.z, vecAbsEdge.y, p1, p3, vecBoxExtents, flTolerance ) ) 
		return false; 

	vecAbsEdge.x = FloatMakePositive( vecEdge.x );
	if ( !AxisTestEdgeCrossY2( vecEdge.z, vecEdge.x, vecAbsEdge.z, vecAbsEdge.x, p1, p3, vecBoxExtents, flTolerance ) ) 
		return false; 

	if ( !AxisTestEdgeCrossZ1( vecEdge.y, vecEdge.x, vecAbsEdge.y, vecAbsEdge.x, p2, p3, vecBoxExtents, flTolerance ) ) 
		return false; 

	// edge 1 (cross x,y,z)
	vecEdge = p3 - p2;
	vecAbsEdge.y = FloatMakePositive( vecEdge.y );
	vecAbsEdge.z = FloatMakePositive( vecEdge.z );
	if ( !AxisTestEdgeCrossX2( vecEdge.z, vecEdge.y, vecAbsEdge.z, vecAbsEdge.y, p1, p2, vecBoxExtents, flTolerance ) ) 
		return false; 

	vecAbsEdge.x = FloatMakePositive( vecEdge.x );
	if ( !AxisTestEdgeCrossY2( vecEdge.z, vecEdge.x, vecAbsEdge.z, vecAbsEdge.x, p1, p2, vecBoxExtents, flTolerance ) ) 
		return false; 

	if ( !AxisTestEdgeCrossZ2( vecEdge.y, vecEdge.x, vecAbsEdge.y, vecAbsEdge.x, p1, p3, vecBoxExtents, flTolerance ) ) 
		return false; 

	// edge 2 (cross x,y,z)
	vecEdge = p1 - p3;
	vecAbsEdge.y = FloatMakePositive( vecEdge.y );
	vecAbsEdge.z = FloatMakePositive( vecEdge.z );
	if ( !AxisTestEdgeCrossX3( vecEdge.z, vecEdge.y, vecAbsEdge.z, vecAbsEdge.y, p1, p2, vecBoxExtents, flTolerance ) ) 
		return false; 

	vecAbsEdge.x = FloatMakePositive( vecEdge.x );
	if ( !AxisTestEdgeCrossY3( vecEdge.z, vecEdge.x, vecAbsEdge.z, vecAbsEdge.x, p1, p2, vecBoxExtents, flTolerance ) ) 
		return false; 

	if ( !AxisTestEdgeCrossZ1( vecEdge.y, vecEdge.x, vecAbsEdge.y, vecAbsEdge.x, p2, p3, vecBoxExtents, flTolerance ) ) 
		return false; 

	// Test against the triangle face plane.
	Vector vecMin, vecMax;
	VectorSubtract( vecBoxCenter, vecBoxExtents, vecMin );
	VectorAdd( vecBoxCenter, vecBoxExtents, vecMax );
	if ( BoxOnPlaneSide( vecMin, vecMax, &plane ) != 3 ) 
		return false; 

	return true;
}

// NOTE: JAY: This is untested code based on Real-time Collision Detection by Ericson
#if 0
Vector CalcClosestPointOnTriangle( const Vector &P, const Vector &v0, const Vector &v1, const Vector &v2 )
{
	Vector e0 = v1 - v0;
	Vector e1 = v2 - v0;
	Vector p0 = P - v0;

	// voronoi region of v0
	float d1 = DotProduct( e0, p0 );
	float d2 = DotProduct( e1, p0 );
	if (d1 <= 0.0f && d2 <= 0.0f)
		return v0;

	// voronoi region of v1
	Vector p1 = P - v1;
	float d3 = DotProduct( e0, p1 );
	float d4 = DotProduct( e1, p1 );
	if (d3 >=0.0f && d4 <= d3)
		return v1;

	// voronoi region of e0 (v0-v1)
	float ve2 = d1*d4 - d3*d2;
	if ( ve2 <= 0.0f && d1 >= 0.0f && d3 <= 0.0f )
	{
		float v = d1 / (d1-d3);
		return v0 + v * e0;
	}
	// voronoi region of v2
	Vector p2 = P - v2;
	float d5 = DotProduct( e0, p2 );
	float d6 = DotProduct( e1, p2 );
	if (d6 >= 0.0f && d5 <= d6)
		return v2;
	// voronoi region of e1
	float ve1 = d5*d2 - d1*d6;
	if (ve1 <= 0.0f && d2 >= 0.0f && d6 >= 0.0f)
	{
		float w = d2 / (d2-d6);
		return v0 + w * e1;
	}
	// voronoi region on e2
	float ve0 = d3*d6 - d5*d4;
	if ( ve0 <= 0.0f && (d4-d3) >= 0.0f && (d5-d6) >= 0.0f )
	{
		float w = (d4-d3)/((d4-d3) + (d5-d6));
		return v1 + w * (v2-v1);
	}
	// voronoi region of v0v1v2 triangle
	float denom = 1.0f / (ve0+ve1+ve2);
	float v = ve1*denom;
	float w = ve2 * denom;
	return v0 + e0 * v + e1 * w;
}
#endif


bool OBBHasFullyContainedIntersectionWithQuad( const Vector &vOBBExtent1_Scaled, const Vector &vOBBExtent2_Scaled, const Vector &vOBBExtent3_Scaled, const Vector &ptOBBCenter,
											  const Vector &vQuadNormal, float fQuadPlaneDist, const Vector &ptQuadCenter,
											  const Vector &vQuadExtent1_Normalized, float fQuadExtent1Length, 
											  const Vector &vQuadExtent2_Normalized, float fQuadExtent2Length )
{
	Vector ptOBB[8]; //this specific ordering helps us web out from a point to its 3 connecting points with some bit math (most importantly, no if's)
	ptOBB[0] = ptOBBCenter - vOBBExtent1_Scaled - vOBBExtent2_Scaled - vOBBExtent3_Scaled;
	ptOBB[1] = ptOBBCenter - vOBBExtent1_Scaled - vOBBExtent2_Scaled + vOBBExtent3_Scaled;
	ptOBB[2] = ptOBBCenter - vOBBExtent1_Scaled + vOBBExtent2_Scaled + vOBBExtent3_Scaled;
	ptOBB[3] = ptOBBCenter - vOBBExtent1_Scaled + vOBBExtent2_Scaled - vOBBExtent3_Scaled;
	ptOBB[4] = ptOBBCenter + vOBBExtent1_Scaled - vOBBExtent2_Scaled - vOBBExtent3_Scaled;
	ptOBB[5] = ptOBBCenter + vOBBExtent1_Scaled - vOBBExtent2_Scaled + vOBBExtent3_Scaled;
	ptOBB[6] = ptOBBCenter + vOBBExtent1_Scaled + vOBBExtent2_Scaled + vOBBExtent3_Scaled;
	ptOBB[7] = ptOBBCenter + vOBBExtent1_Scaled + vOBBExtent2_Scaled - vOBBExtent3_Scaled;

	float fDists[8];
	for( int i = 0; i != 8; ++i )
		fDists[i] = vQuadNormal.Dot( ptOBB[i] ) - fQuadPlaneDist;

	int iSides[8];
	int iSideMask = 0;
	for( int i = 0; i != 8; ++i )
	{
		if( fDists[i] > 0.0f )
		{
			iSides[i] = 1;
			iSideMask |= 1;
		}
		else
		{
			iSides[i] = 2;
			iSideMask |= 2;
		}
	}

	if( iSideMask != 3 ) //points reside entirely on one side of the quad's plane
		return false;

	Vector ptPlaneIntersections[12]; //only have 12 lines, can only possibly generate 12 split points
	int iPlaneIntersectionsCount = 0;

	for( int i = 0; i != 8; ++i )
	{
		if( iSides[i] == 2 ) //point behind the plane
		{
			int iAxisCrossings[3];
			iAxisCrossings[0] = i ^ 4; //upper 4 vs lower 4 crosses vOBBExtent1 axis
			iAxisCrossings[1] = ((i + 1) & 3) + (i & 4); //cycle to the next element while staying within the upper 4 or lower 4, this will cross either vOBBExtent2 or vOBBExtent3 axis, we don't care which
			iAxisCrossings[2] = ((i - 1) & 3) + (i & 4); //cylce to the previous element while staying within the upper 4 or lower 4, this will cross the axis iAxisCrossings[1] didn't cross

			for( int j = 0; j != 3; ++j )
			{
				if( iSides[iAxisCrossings[j]] == 1 ) //point in front of the plane
				{
					//line between ptOBB[i] and ptOBB[iAxisCrossings[j]] intersects the plane, generate a point at the intersection for further testing
					float fTotalDist = fDists[iAxisCrossings[j]] - fDists[i]; //remember that fDists[i] is a negative value
					ptPlaneIntersections[iPlaneIntersectionsCount] = (ptOBB[iAxisCrossings[j]] * (-fDists[i]/fTotalDist)) + (ptOBB[i] * (fDists[iAxisCrossings[j]]/fTotalDist));

					Assert( fabs( ptPlaneIntersections[iPlaneIntersectionsCount].Dot( vQuadNormal ) - fQuadPlaneDist ) < 0.1f ); //intersection point is on plane

					++iPlaneIntersectionsCount;
				}
			}
		}
	}

	Assert( iPlaneIntersectionsCount != 0 );

	for( int i = 0; i != iPlaneIntersectionsCount; ++i )
	{
		//these points are guaranteed to be on the plane, now just check to see if they're within the quad's extents
		Vector vToPointFromQuadCenter = ptPlaneIntersections[i] - ptQuadCenter;

		float fExt1Dist = vQuadExtent1_Normalized.Dot( vToPointFromQuadCenter );
		if( fabs( fExt1Dist ) > fQuadExtent1Length )
			return false; //point is outside boundaries

		//vToPointFromQuadCenter -= vQuadExtent1_Normalized * fExt1Dist; //to handle diamond shaped quads

		float fExt2Dist = vQuadExtent2_Normalized.Dot( vToPointFromQuadCenter );
		if( fabs( fExt2Dist ) > fQuadExtent2Length )
			return false; //point is outside boundaries
	}

	return true; //there were lines crossing the quad plane, and every line crossing that plane had its intersection with the plane within the quad's boundaries
}

//-----------------------------------------------------------------------------
// Compute if the Ray intersects the quad plane, and whether the entire
// Ray/Quad intersection is contained within the quad itself
//
// False if no intersection exists, or if part of the intersection is
// outside the quad's extents
//-----------------------------------------------------------------------------
bool RayHasFullyContainedIntersectionWithQuad( const Ray_t &ray,
											  const Vector &vQuadNormal, float fQuadPlaneDist, const Vector &ptQuadCenter,
											  const Vector &vQuadExtent1_Normalized, float fQuadExtent1Length, 
											  const Vector &vQuadExtent2_Normalized, float fQuadExtent2Length )
{
	Vector ptPlaneIntersections[(12 + 12 + 8)]; //absolute max possible: 12 lines to connect the start box, 12 more to connect the end box, 8 to connect the boxes to eachother
	
	//8 points to make an AABB, 8 lines to connect each point from it's start to end point along the ray, 8 possible intersections
	int iPlaneIntersectionsCount = 0;

	if( ray.m_IsRay )
	{
		//just 1 line
		if( ray.m_IsSwept )
		{
			Vector ptEndPoints[2];
			ptEndPoints[0] = ray.m_Start;
			ptEndPoints[1] = ptEndPoints[0] + ray.m_Delta;

			int i;
			float fDists[2];
			for( i = 0; i != 2; ++i )
				fDists[i] = vQuadNormal.Dot( ptEndPoints[i] ) - fQuadPlaneDist;
	       
			for( i = 0; i != 2; ++i )
			{
				if( fDists[i] <= 0.0f )
				{
					int j = 1-i;
					if( fDists[j] >= 0.0f )
					{
						float fInvTotalDist = 1.0f / (fDists[j] - fDists[i]); //fDists[i] <= 0, ray is swept so no chance that the denom was 0
						ptPlaneIntersections[0] = (ptEndPoints[i] * (fDists[j] * fInvTotalDist)) - (ptEndPoints[j] * (fDists[i] * fInvTotalDist)); //fDists[i] <= 0 
						Assert( fabs( ptPlaneIntersections[iPlaneIntersectionsCount].Dot( vQuadNormal ) - fQuadPlaneDist ) < 0.1f ); //intersection point is on plane
						iPlaneIntersectionsCount = 1;
					}
					else 
					{
						return false;
					}
					break;
				}
			}

			if( i == 2 )
				return false;
		}
		else //not swept, so this is actually a point on quad question
		{
			if( fabs( vQuadNormal.Dot( ray.m_Start ) - fQuadPlaneDist ) < 1e-6 )
			{
				ptPlaneIntersections[0] = ray.m_Start;
				iPlaneIntersectionsCount = 1;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		Vector ptEndPoints[2][8];
		//this specific ordering helps us web out from a point to its 3 connecting points with some bit math (most importantly, no if's)
		ptEndPoints[0][0] = ray.m_Start; ptEndPoints[0][0].x -= ray.m_Extents.x; ptEndPoints[0][0].y -= ray.m_Extents.y; ptEndPoints[0][0].z -= ray.m_Extents.z;
		ptEndPoints[0][1] = ray.m_Start; ptEndPoints[0][1].x -= ray.m_Extents.x; ptEndPoints[0][1].y -= ray.m_Extents.y; ptEndPoints[0][1].z += ray.m_Extents.z;
		ptEndPoints[0][2] = ray.m_Start; ptEndPoints[0][2].x -= ray.m_Extents.x; ptEndPoints[0][2].y += ray.m_Extents.y; ptEndPoints[0][2].z += ray.m_Extents.z;
		ptEndPoints[0][3] = ray.m_Start; ptEndPoints[0][3].x -= ray.m_Extents.x; ptEndPoints[0][3].y += ray.m_Extents.y; ptEndPoints[0][3].z -= ray.m_Extents.z;
		ptEndPoints[0][4] = ray.m_Start; ptEndPoints[0][4].x += ray.m_Extents.x; ptEndPoints[0][4].y -= ray.m_Extents.y; ptEndPoints[0][4].z -= ray.m_Extents.z;
		ptEndPoints[0][5] = ray.m_Start; ptEndPoints[0][5].x += ray.m_Extents.x; ptEndPoints[0][5].y -= ray.m_Extents.y; ptEndPoints[0][5].z += ray.m_Extents.z;
		ptEndPoints[0][6] = ray.m_Start; ptEndPoints[0][6].x += ray.m_Extents.x; ptEndPoints[0][6].y += ray.m_Extents.y; ptEndPoints[0][6].z += ray.m_Extents.z;
		ptEndPoints[0][7] = ray.m_Start; ptEndPoints[0][7].x += ray.m_Extents.x; ptEndPoints[0][7].y += ray.m_Extents.y; ptEndPoints[0][7].z -= ray.m_Extents.z;

		float fDists[2][8];
		int iSides[2][8];
		int iSideMask[2] = { 0, 0 };
		for( int i = 0; i != 8; ++i )
		{
			fDists[0][i] = vQuadNormal.Dot( ptEndPoints[0][i] ) - fQuadPlaneDist;
			if( fDists[0][i] > 0.0f )
			{
				iSides[0][i] = 1;
				iSideMask[0] |= 1;
			}
			else
			{
				iSides[0][i] = 2;
				iSideMask[0] |= 2;
			}
		}

		if( ray.m_IsSwept )
		{
			for( int i = 0; i != 8; ++i )
				ptEndPoints[1][i] = ptEndPoints[0][i] + ray.m_Delta;

			for( int i = 0; i != 8; ++i )
			{
				fDists[1][i] = vQuadNormal.Dot( ptEndPoints[1][i] ) - fQuadPlaneDist;
				if( fDists[1][i] > 0.0f )
				{
					iSides[1][i] = 1;
					iSideMask[1] |= 1;
				}
				else
				{
					iSides[1][i] = 2;
					iSideMask[1] |= 2;
				}
			}
		}

		if( (iSideMask[0] | iSideMask[1]) != 3 )
		{
			//Assert( (iSideMask[0] | iSideMask[1]) != 2 );
			return false; //all points resides entirely on one side of the quad
		}


		//generate intersections for boxes split by the plane at either end of the ray
		for( int k = 0; k != 2; ++k )
		{
			if( iSideMask[k] == 3 ) //box is split by the plane
			{
				for( int i = 0; i != 8; ++i )
				{
					if( iSides[k][i] == 2 ) //point behind the plane
					{
						int iAxisCrossings[3];
						iAxisCrossings[0] = i ^ 4; //upper 4 vs lower 4 crosses X axis
						iAxisCrossings[1] = ((i + 1) & 3) + (i & 4); //cycle to the next element while staying within the upper 4 or lower 4, this will cross either Y or Z axis, we don't care which
						iAxisCrossings[2] = ((i - 1) & 3) + (i & 4); //cylce to the previous element while staying within the upper 4 or lower 4, this will cross the axis iAxisCrossings[1] didn't cross

						for( int j = 0; j != 3; ++j )
						{
							if( iSides[k][iAxisCrossings[j]] == 1 ) //point in front of the plane
							{
								//line between ptEndPoints[i] and ptEndPoints[iAxisCrossings[j]] intersects the plane, generate a point at the intersection for further testing
								float fInvTotalDist = 1.0f / (fDists[k][iAxisCrossings[j]] - fDists[k][i]); //remember that fDists[k][i] is a negative value
								ptPlaneIntersections[iPlaneIntersectionsCount] = (ptEndPoints[k][iAxisCrossings[j]] * (-fDists[k][i] * fInvTotalDist)) + (ptEndPoints[k][i] * (fDists[k][iAxisCrossings[j]] * fInvTotalDist));

								Assert( fabs( ptPlaneIntersections[iPlaneIntersectionsCount].Dot( vQuadNormal ) - fQuadPlaneDist ) < 0.1f ); //intersection point is on plane

								++iPlaneIntersectionsCount;
							}
						}
					}
				}
			}
		}		

		if( ray.m_IsSwept )
		{
			for( int i = 0; i != 8; ++i )
			{
				if( iSides[0][i] != iSides[1][i] )
				{
					int iPosSide, iNegSide;
					if( iSides[0][i] == 1 )
					{
						iPosSide = 0;
						iNegSide = 1;
					}
					else
					{
						iPosSide = 1;
						iNegSide = 0;
					}

					Assert( (fDists[iPosSide][i] >= 0.0f) && (fDists[iNegSide][i] <= 0.0f) );

					float fInvTotalDist = 1.0f / (fDists[iPosSide][i] - fDists[iNegSide][i]); //remember that fDists[iNegSide][i] is a negative value
					ptPlaneIntersections[iPlaneIntersectionsCount] = (ptEndPoints[iPosSide][i] * (-fDists[iNegSide][i] * fInvTotalDist)) + (ptEndPoints[iNegSide][i] * (fDists[iPosSide][i] * fInvTotalDist));

					Assert( fabs( ptPlaneIntersections[iPlaneIntersectionsCount].Dot( vQuadNormal ) - fQuadPlaneDist ) < 0.1f ); //intersection point is on plane

					++iPlaneIntersectionsCount;
				}
			}
		}
	}

	//down here, we should simply have a collection of plane intersections, now we see if they reside within the quad
	Assert( iPlaneIntersectionsCount != 0 );

	for( int i = 0; i != iPlaneIntersectionsCount; ++i )
	{
		//these points are guaranteed to be on the plane, now just check to see if they're within the quad's extents
		Vector vToPointFromQuadCenter = ptPlaneIntersections[i] - ptQuadCenter;

		float fExt1Dist = vQuadExtent1_Normalized.Dot( vToPointFromQuadCenter );
		if( fabs( fExt1Dist ) > fQuadExtent1Length )
			return false; //point is outside boundaries

		//vToPointFromQuadCenter -= vQuadExtent1_Normalized * fExt1Dist; //to handle diamond shaped quads

		float fExt2Dist = vQuadExtent2_Normalized.Dot( vToPointFromQuadCenter );
		if( fabs( fExt2Dist ) > fQuadExtent2Length )
			return false; //point is outside boundaries
	}

	return true; //there were lines crossing the quad plane, and every line crossing that plane had its intersection with the plane within the quad's boundaries
}

#endif // !_STATIC_LINKED || _SHARED_LIB
