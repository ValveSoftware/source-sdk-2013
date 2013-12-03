//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Common collision utility methods
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef COLLISIONUTILS_H
#define COLLISIONUTILS_H

#include "tier0/platform.h"

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/ssemath.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
struct Ray_t;
class Vector;
class Vector2D;
class Vector4D;
struct cplane_t;
class QAngle;
class CBaseTrace;
struct matrix3x4_t;


//-----------------------------------------------------------------------------
//
// IntersectRayWithTriangle
//
// Intersects a ray with a triangle, returns distance t along ray.
// t will be less than zero if no intersection occurred
// oneSided will cull collisions which approach the triangle from the back
// side, assuming the vertices are specified in counter-clockwise order
// The vertices need not be specified in that order if oneSided is not used
//
//-----------------------------------------------------------------------------
float IntersectRayWithTriangle( const Ray_t& ray, 
		                        const Vector& v1, const Vector& v2, const Vector& v3, 
								bool oneSided );

//-----------------------------------------------------------------------------
//
// ComputeIntersectionBarycentricCoordinates
//
// Figures out the barycentric coordinates (u,v) where a ray hits a 
// triangle. Note that this will ignore the ray extents, and it also ignores
// the ray length. Note that the edge from v1->v2 represents u (v2: u = 1), 
// and the edge from v1->v3 represents v (v3: v = 1). It returns false
// if the ray is parallel to the triangle (or when t is specified if t is less
// than zero).
//
//-----------------------------------------------------------------------------
bool ComputeIntersectionBarycentricCoordinates( const Ray_t& ray, 
		const Vector& v1, const Vector& v2, const Vector& v3, float& u, float& v,
		float *t = 0 );

//-----------------------------------------------------------------------------
//
// IntersectRayWithRay
//
// Returns whether or not there was an intersection.  The "t" paramter is the
// distance along ray0 and the "s" parameter is the distance along ray1.  If 
// the two lines to not intersect the "t" and "s" represent the closest approach.
// "t" and "s" will not change if the rays are parallel.
//
//-----------------------------------------------------------------------------
bool IntersectRayWithRay( const Ray_t &ray0, const Ray_t &ray1, float &t, float &s );


//-----------------------------------------------------------------------------
//
// IntersectRayWithSphere
//
// Returns whether or not there was an intersection. Returns the two intersection points.
// NOTE: The point of closest approach can be found at the average t value.
//
//-----------------------------------------------------------------------------
bool IntersectRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 );


//-----------------------------------------------------------------------------
//
// IntersectInfiniteRayWithSphere
//
// Returns whether or not there was an intersection of a sphere against an infinitely
// extending ray. 
// Returns the two intersection points
//
//-----------------------------------------------------------------------------
bool IntersectInfiniteRayWithSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float *pT1, float *pT2 );


// returns true if the sphere and cone intersect
// NOTE: cone sine/cosine are the half angle of the cone
bool IsSphereIntersectingCone( const Vector &sphereCenter, float sphereRadius, const Vector &coneOrigin, const Vector &coneNormal, float coneSine, float coneCosine );

//-----------------------------------------------------------------------------
//
// IntersectRayWithPlane
//
// Intersects a ray with a plane, returns distance t along ray.
// t will be less than zero the intersection occurs in the opposite direction of the ray.
//
//-----------------------------------------------------------------------------
float IntersectRayWithPlane( const Ray_t& ray, const cplane_t& plane ); 
float IntersectRayWithPlane( const Vector& org, const Vector& dir, const cplane_t& plane );
float IntersectRayWithPlane( const Vector& org, const Vector& dir, const Vector& normal, float dist );

// This version intersects a ray with an axis-aligned plane
float IntersectRayWithAAPlane( const Vector& vecStart, const Vector& vecEnd, int nAxis, float flSign, float flDist );


//-----------------------------------------------------------------------------
// IntersectRayWithBox
//
// Purpose: Computes the intersection of a ray with a box (AABB)
// Output : Returns true if there is an intersection + trace information
//-----------------------------------------------------------------------------
bool IntersectRayWithBox( const Vector &rayStart, const Vector &rayDelta, const Vector &boxMins, const Vector &boxMaxs, float epsilon, CBaseTrace *pTrace, float *pFractionLeftSolid = NULL );
bool IntersectRayWithBox( const Ray_t &ray, const Vector &boxMins, const Vector &boxMaxs, float epsilon, CBaseTrace *pTrace, float *pFractionLeftSolid = NULL );

//-----------------------------------------------------------------------------
// Intersects a ray against a box
//-----------------------------------------------------------------------------
struct BoxTraceInfo_t
{
	float t1;
	float t2;
	int	hitside;
	bool startsolid;
};

bool IntersectRayWithBox( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const Vector &boxMins, const Vector &boxMaxs, float flTolerance, BoxTraceInfo_t *pTrace );


//-----------------------------------------------------------------------------
// IntersectRayWithOBB
//
// Purpose: Computes the intersection of a ray with a oriented box (OBB)
// Output : Returns true if there is an intersection + trace information
//-----------------------------------------------------------------------------
bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Ray_t &ray, const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, BoxTraceInfo_t *pTrace );

//-----------------------------------------------------------------------------
// 
// IsSphereIntersectingSphere
//
// returns true if there's an intersection between sphere and sphere
//
//-----------------------------------------------------------------------------
bool IsSphereIntersectingSphere( const Vector& center1, float radius1, 
								 const Vector& center2, float radius2 );


//-----------------------------------------------------------------------------
// 
// IsBoxIntersectingSphere
//
// returns true if there's an intersection between box and sphere
//
//-----------------------------------------------------------------------------
bool IsBoxIntersectingSphere( const Vector& boxMin, const Vector& boxMax, 
						      const Vector& center, float radius );

bool IsBoxIntersectingSphereExtents( const Vector& boxCenter, const Vector& boxHalfDiag, 
									const Vector& center, float radius );

//-----------------------------------------------------------------------------
// returns true if there's an intersection between ray and sphere
//-----------------------------------------------------------------------------
bool IsRayIntersectingSphere( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecSphereCenter, float flRadius, float flTolerance = 0.0f );


//-----------------------------------------------------------------------------
// 
// IsCircleIntersectingRectangle
//
// returns true if there's an intersection between rectangle and circle
//
//-----------------------------------------------------------------------------
bool IsCircleIntersectingRectangle( const Vector2D& boxMin, const Vector2D& boxMax, 
						      const Vector2D& center, float radius );


//-----------------------------------------------------------------------------
// 
// IsBoxIntersectingBox
//
// returns true if there's an intersection between two boxes
//
//-----------------------------------------------------------------------------
bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
						   const Vector& boxMin2, const Vector& boxMax2 );

bool IsBoxIntersectingBoxExtents( const Vector& boxCenter1, const Vector& boxHalfDiagonal1, 
						   const Vector& boxCenter2, const Vector& boxHalfDiagonal2 );


#ifdef _X360
// inline version:
#include "mathlib/ssemath.h"
inline bool IsBoxIntersectingBoxExtents( const fltx4 boxCenter1, const fltx4 boxHalfDiagonal1, 
								 const fltx4 boxCenter2, const fltx4 boxHalfDiagonal2 );
#endif

//-----------------------------------------------------------------------------
// 
// IsOBBIntersectingOBB
//
// returns true if there's an intersection between two OBBs
//
//-----------------------------------------------------------------------------
bool IsOBBIntersectingOBB( const Vector &vecOrigin1, const QAngle &vecAngles1, const Vector& boxMin1, const Vector& boxMax1, 
						   const Vector &vecOrigin2, const QAngle &vecAngles2, const Vector& boxMin2, const Vector& boxMax2, float flTolerance = 0.0f );


//-----------------------------------------------------------------------------
// 
// IsBoxIntersectingRay
//
// returns true if there's an intersection between box and ray
//
//-----------------------------------------------------------------------------

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& delta, float flTolerance = 0.0f );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Ray_t& ray, float flTolerance = 0.0f );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& delta,
									const Vector& invDelta, float flTolerance = 0.0f );


// On the PC, we can't pass fltx4's in registers like this. On the x360, it is 
// much better if we do.
#ifdef _X360
bool FASTCALL IsBoxIntersectingRay( fltx4 boxMin, fltx4 boxMax, 
								   fltx4 origin, fltx4 delta, fltx4 invDelta, // ray parameters
								   fltx4 vTolerance = LoadZeroSIMD() ///< eg from ReplicateX4(flTolerance)
								   );
#else
bool FASTCALL IsBoxIntersectingRay( const fltx4 &boxMin, const fltx4 &boxMax, 
								   const fltx4 & origin, const fltx4 & delta, const fltx4 & invDelta, // ray parameters
								   const fltx4 & vTolerance = Four_Zeros ///< eg from ReplicateX4(flTolerance)
								   );
#endif

bool inline FASTCALL IsBoxIntersectingRay( const fltx4& boxMin, const fltx4& boxMax, 
								   const fltx4& origin, const fltx4& delta, float flTolerance = 0.0f )
{
	return IsBoxIntersectingRay( boxMin, boxMax, origin, delta, ReciprocalSIMD(delta), ReplicateX4(flTolerance) );
}


bool FASTCALL IsBoxIntersectingRay( const fltx4& boxMin, const fltx4& boxMax, 
								   const Ray_t& ray, float flTolerance = 0.0f );



//-----------------------------------------------------------------------------
// 
// IsPointInBox
//
// returns true if the point is in the box
//
//-----------------------------------------------------------------------------
bool IsPointInBox( const Vector& pt, const Vector& boxMin, const Vector& boxMax );


// SIMD version
FORCEINLINE bool IsPointInBox( const fltx4& pt, const fltx4& boxMin, const fltx4& boxMax )
{
	fltx4 greater = CmpGtSIMD( pt,boxMax );
	fltx4 less = CmpLtSIMD( pt, boxMin );
	return (IsAllZeros(SetWToZeroSIMD(OrSIMD(greater,less))));
}



//-----------------------------------------------------------------------------
// Purpose: returns true if pt intersects the truncated cone
// origin - cone tip, axis unit cone axis, cosAngle - cosine of cone axis to surface angle
//-----------------------------------------------------------------------------
bool IsPointInCone( const Vector &pt, const Vector &origin, const Vector &axis, float cosAngle, float length );

//-----------------------------------------------------------------------------
// Intersects a plane with a triangle (using barycentric definition)
// The return value, in pIntersection, is an array of barycentric coordinates 
// describing at most 2 intersection points. 
// The return value is the number of intersection points
//-----------------------------------------------------------------------------
int IntersectTriangleWithPlaneBarycentric( const Vector& org, const Vector& edgeU, const Vector& edgeV, 
										   const Vector4D& plane, Vector2D* pIntersection );

//-----------------------------------------------------------------------------
//
// PointInQuadBarycentric
//
//	Given a point and a quad in a plane return the u and v (barycentric) positions
//  of the point relative to the quad.  The points (v1,v2,v3,v4) should be given
//  in a counter-clockwise order with v1 acting as the primary corner (u=0, v=0).
//  Thus, u0 = v2 - v1, and v0 = v4 - v1.
//
//-----------------------------------------------------------------------------

enum QuadBarycentricRetval_t
{
	BARY_QUADRATIC_FALSE					= 0,
	BARY_QUADRATIC_TRUE						= 1,
	BARY_QUADRATIC_NEGATIVE_DISCRIMINANT	= 2
};

QuadBarycentricRetval_t PointInQuadToBarycentric( const Vector &v1, const Vector &v2, 
	const Vector &v3, const Vector &v4, const Vector &point, Vector2D &uv );


void PointInQuadFromBarycentric( const Vector &v1, const Vector &v2, const Vector &v3, const Vector &v4,
								 const Vector2D &uv, Vector &point );
void TexCoordInQuadFromBarycentric( const Vector2D &v1, const Vector2D &v2, const Vector2D &v3, const Vector2D &v4,
								    const Vector2D &uv, Vector2D &texCoord );


//-----------------------------------------------------------------------------
// Compute point from barycentric specification
// Edge u goes from v0 to v1, edge v goes from v0 to v2
//-----------------------------------------------------------------------------
void ComputePointFromBarycentric( const Vector& v0, const Vector& v1, const Vector& v2, 
								 float u, float v, Vector& pt );
void ComputePointFromBarycentric( const Vector2D& v0, const Vector2D& v1, const Vector2D& v2, 
								 float u, float v, Vector2D& pt );


//-----------------------------------------------------------------------------
// Swept OBB test
//-----------------------------------------------------------------------------
bool IsRayIntersectingOBB( const Ray_t &ray, const Vector& org, const QAngle& angles, 
						  const Vector& mins, const Vector& maxs );


//-----------------------------------------------------------------------------
// Compute a separating plane between two boxes (expensive!)
// Returns false if no separating plane exists
//-----------------------------------------------------------------------------
bool ComputeSeparatingPlane( const Vector& org1, const QAngle& angles1, const Vector& min1, const Vector& max1, 
	const Vector& org2, const QAngle& angles2, const Vector& min2, const Vector& max2, 
	float tolerance, cplane_t* pPlane );

//-----------------------------------------------------------------------------
// IsBoxIntersectingTriangle
//
// Test for an intersection (overlap) between an axial-aligned bounding 
// box (AABB) and a triangle.
//
// Triangle points are in counter-clockwise order with the normal facing "out."
//
// Using the "Separating-Axis Theorem" to test for intersections between
// a triangle and an axial-aligned bounding box (AABB).
// 1. 3 Axis Plane Tests - x, y, z
// 2. 9 Edge Planes Tests - the 3 edges of the triangle crossed with all 3 axial 
//                          planes (x, y, z)
// 3. 1 Face Plane Test - the plane the triangle resides in (cplane_t plane)
//-----------------------------------------------------------------------------
bool IsBoxIntersectingTriangle( const Vector &vecBoxCenter, const Vector &vecBoxExtents,
				   		        const Vector &v1, const Vector &v2, const Vector &v3,
						        const cplane_t &plane, float flTolerance );


Vector CalcClosestPointOnTriangle( const Vector &P, const Vector &v0, const Vector &v1, const Vector &v2 );


//-----------------------------------------------------------------------------
// Compute if the OBB intersects the quad plane, and whether the entire
// OBB/Quad intersection is contained within the quad itself
//
// False if no intersection exists, or if part of the intersection is
// outside the quad's extents
//-----------------------------------------------------------------------------
bool OBBHasFullyContainedIntersectionWithQuad( const Vector &vOBBExtent1_Scaled, const Vector &vOBBExtent2_Scaled, const Vector &vOBBExtent3_Scaled, const Vector &ptOBBCenter,
											  const Vector &vQuadNormal, float fQuadPlaneDist, const Vector &ptQuadCenter,
											  const Vector &vQuadExtent1_Normalized, float fQuadExtent1Length, 
											  const Vector &vQuadExtent2_Normalized, float fQuadExtent2Length );


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
											  const Vector &vQuadExtent2_Normalized, float fQuadExtent2Length );



//-----------------------------------------------------------------------------
// INLINES
//-----------------------------------------------------------------------------


#ifdef _X360
inline bool IsBoxIntersectingBoxExtents( const fltx4 boxCenter1, const fltx4 boxHalfDiagonal1, 
								 const fltx4 boxCenter2, const fltx4 boxHalfDiagonal2 )
{
	fltx4 vecDelta, vecSize;

	vecDelta = SubSIMD(boxCenter1, boxCenter2);
	vecSize = AddSIMD(boxHalfDiagonal1, boxHalfDiagonal2);

	uint condition;
	XMVectorInBoundsR(&condition, vecDelta, vecSize);
	// we want the top three words to be all 1's ; that means in bounds


	return XMComparisonAllInBounds( condition );
}
#endif


#endif // COLLISIONUTILS_H
