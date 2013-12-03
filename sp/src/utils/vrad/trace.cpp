//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
// trace.c

//=============================================================================

#include "vrad.h"
#include "trace.h"
#include "Cmodel.h"
#include "mathlib/vmatrix.h"


//=============================================================================

class CToolTrace : public CBaseTrace
{
public:
	CToolTrace() {}

	Vector		mins;
	Vector		maxs;
	Vector		extents;

	texinfo_t	*surface;

	qboolean	ispoint;

private:
	 CToolTrace( const CToolTrace& );
};


// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

// JAYHL2: This used to be -1, but that caused lots of epsilon issues
// around slow sloping planes.  Perhaps Quake2 limited maps to a certain
// slope / angle on walkable ground.  It has to be a negative number
// so that the tests work out.
#define		NEVER_UPDATED		-9999

//=============================================================================

bool DM_RayDispIntersectTest( CVRADDispColl *pTree, Vector& rayStart, Vector& rayEnd, CToolTrace *pTrace );
void DM_ClipBoxToBrush( CToolTrace *trace, const Vector & mins, const Vector & maxs, const Vector& p1, const Vector& p2, dbrush_t *brush );

//=============================================================================

float TraceLeafBrushes( int leafIndex, const Vector &start, const Vector &end, CBaseTrace &traceOut )
{
	dleaf_t *pLeaf = dleafs + leafIndex;
	CToolTrace trace;
	memset( &trace, 0, sizeof(trace) );
	trace.ispoint = true;
	trace.startsolid = false;
	trace.fraction = 1.0;

	for ( int i = 0; i < pLeaf->numleafbrushes; i++ )
	{
		int brushnum = dleafbrushes[pLeaf->firstleafbrush+i];
		dbrush_t *b = &dbrushes[brushnum];
		if ( !(b->contents & MASK_OPAQUE))
			continue;

		Vector zeroExtents = vec3_origin;
		DM_ClipBoxToBrush( &trace, zeroExtents, zeroExtents, start, end, b);
		if ( trace.fraction != 1.0 || trace.startsolid )
		{
			if ( trace.startsolid )
				trace.fraction = 0.0f;
			traceOut = trace;
			return trace.fraction;
		}
	}
	traceOut = trace;
	return 1.0f;
}

DispTested_t s_DispTested[MAX_TOOL_THREADS+1];

// this just uses the average coverage for the triangle
class CCoverageCount : public ITransparentTriangleCallback
{
public:
	CCoverageCount()
	{
		m_coverage = Four_Zeros;
	}

	virtual bool VisitTriangle_ShouldContinue( const TriIntersectData_t &triangle, const FourRays &rays, fltx4 *pHitMask, fltx4 *b0, fltx4 *b1, fltx4 *b2, int32 hitID )
	{
		float color = g_RtEnv.GetTriangleColor( hitID ).x;
		m_coverage = AddSIMD( m_coverage, AndSIMD ( *pHitMask, ReplicateX4 ( color ) ) );
		m_coverage = MinSIMD( m_coverage, Four_Ones );

		fltx4 onesMask = CmpEqSIMD( m_coverage, Four_Ones );

		// we should continue if the ones that hit the triangle have onesMask set to zero
		// so hitMask & onesMask != hitMask
		// so hitMask & onesMask == hitMask means we're done
		// so ts(hitMask & onesMask == hitMask) != 0xF says go on
		return 0xF != TestSignSIMD ( CmpEqSIMD ( AndSIMD( *pHitMask, onesMask ), *pHitMask ) );
	}

	fltx4 GetCoverage()
	{
		return m_coverage;
	}

	fltx4 GetFractionVisible()
	{
		return SubSIMD ( Four_Ones, m_coverage );
	}

	fltx4 m_coverage;
};

// this will sample the texture to get a coverage at the ray intersection point
class CCoverageCountTexture : public CCoverageCount
{
public:
	virtual bool VisitTriangle_ShouldContinue( const TriIntersectData_t &triangle, const FourRays &rays, fltx4 *pHitMask, fltx4 *b0, fltx4 *b1, fltx4 *b2, int32 hitID )
	{
		int sign = TestSignSIMD( *pHitMask );
		float addedCoverage[4];
		for ( int s = 0; s < 4; s++)
		{
			addedCoverage[s] = 0.0f;
			if ( ( sign >> s) & 0x1 )
			{
				addedCoverage[s] = ComputeCoverageFromTexture( b0->m128_f32[s], b1->m128_f32[s], b2->m128_f32[s], hitID );
			}
		}
		m_coverage = AddSIMD( m_coverage, LoadUnalignedSIMD( addedCoverage ) );
		m_coverage = MinSIMD( m_coverage, Four_Ones );
		fltx4 onesMask = CmpEqSIMD( m_coverage, Four_Ones );

		// we should continue if the ones that hit the triangle have onesMask set to zero
		// so hitMask & onesMask != hitMask
		// so hitMask & onesMask == hitMask means we're done
		// so ts(hitMask & onesMask == hitMask) != 0xF says go on
		return 0xF != TestSignSIMD ( CmpEqSIMD ( AndSIMD( *pHitMask, onesMask ), *pHitMask ) );
	}
};

void TestLine( const FourVectors& start, const FourVectors& stop,
               fltx4 *pFractionVisible, int static_prop_index_to_ignore )
{
	FourRays myrays;
	myrays.origin = start;
	myrays.direction = stop;
	myrays.direction -= myrays.origin;
	fltx4 len = myrays.direction.length();
	myrays.direction *= ReciprocalSIMD( len );

	RayTracingResult rt_result;
	CCoverageCountTexture coverageCallback;

	g_RtEnv.Trace4Rays(myrays, Four_Zeros, len, &rt_result, TRACE_ID_STATICPROP | static_prop_index_to_ignore, g_bTextureShadows ? &coverageCallback : 0 );

	// Assume we can see the targets unless we get hits
	float visibility[4];
	for ( int i = 0; i < 4; i++ )
	{
		visibility[i] = 1.0f;
		if ( ( rt_result.HitIds[i] != -1 ) &&
		     ( rt_result.HitDistance.m128_f32[i] < len.m128_f32[i] ) )
		{
			visibility[i] = 0.0f;
		}
	}
	*pFractionVisible = LoadUnalignedSIMD( visibility );
	if ( g_bTextureShadows )
		*pFractionVisible = MinSIMD( *pFractionVisible, coverageCallback.GetFractionVisible() );
}



/*
================
DM_ClipBoxToBrush
================
*/
void DM_ClipBoxToBrush( CToolTrace *trace, const Vector& mins, const Vector& maxs, const Vector& p1, const Vector& p2,
						dbrush_t *brush)
{
	dplane_t	*plane, *clipplane;
	float		dist;
	Vector		ofs;
	float		d1, d2;
	float		f;
	dbrushside_t	*side, *leadside;

	if (!brush->numsides)
		return;

	float enterfrac = NEVER_UPDATED;
	float leavefrac = 1.f;
	clipplane = NULL;

	bool getout = false;
	bool startout = false;
	leadside = NULL;

	// Loop interchanged, so we don't have to check trace->ispoint every side.
	if ( !trace->ispoint )
	{
		for (int i=0 ; i<brush->numsides ; ++i)
		{
			side = &dbrushsides[brush->firstside+i];
			plane = dplanes + side->planenum;

			// FIXME: special case for axial

			// general box case
			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			ofs.x = (plane->normal.x < 0) ? maxs.x : mins.x;
			ofs.y = (plane->normal.y < 0) ? maxs.y : mins.y;
			ofs.z = (plane->normal.z < 0) ? maxs.z : mins.z;
//			for (j=0 ; j<3 ; j++)
//			{
				// Set signmask to either 0 if the sign is negative, or 0xFFFFFFFF is the sign is positive:
				//int signmask = (((*(int *)&(plane->normal[j]))&0x80000000) >> 31) - 1;

				//float temp = maxs[j];
				//*(int *)&(ofs[j]) =    (~signmask) & (*(int *)&temp);
				//float temp1 = mins[j];
				//*(int *)&(ofs[j]) |=   (signmask) & (*(int *)&temp1);
//			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;

			d1 = DotProduct (p1, plane->normal) - dist;
			d2 = DotProduct (p2, plane->normal) - dist;

			// if completely in front of face, no intersection
			if (d1 > 0 && d2 > 0)
				return;

			if (d2 > 0)
				getout = true;	// endpoint is not in solid
			if (d1 > 0)
				startout = true;

			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2)
			{	// enter
				f = (d1-DIST_EPSILON) / (d1-d2);
				if (f > enterfrac)
				{
					enterfrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else
			{	// leave
				f = (d1+DIST_EPSILON) / (d1-d2);
				if (f < leavefrac)
					leavefrac = f;
			}
		}
	}
	else
	{
		for (int i=0 ; i<brush->numsides ; ++i)
		{
			side = &dbrushsides[brush->firstside+i];
			plane = dplanes + side->planenum;

			// FIXME: special case for axial

			// special point case
			// don't ray trace against bevel planes
			if( side->bevel == 1 )
				continue;

			dist = plane->dist;
			d1 = DotProduct (p1, plane->normal) - dist;
			d2 = DotProduct (p2, plane->normal) - dist;

			// if completely in front of face, no intersection
			if (d1 > 0 && d2 > 0)
				return;

			if (d2 > 0)
				getout = true;	// endpoint is not in solid
			if (d1 > 0)
				startout = true;

			if (d1 <= 0 && d2 <= 0)
				continue;

			// crosses face
			if (d1 > d2)
			{	// enter
				f = (d1-DIST_EPSILON) / (d1-d2);
				if (f > enterfrac)
				{
					enterfrac = f;
					clipplane = plane;
					leadside = side;
				}
			}
			else
			{	// leave
				f = (d1+DIST_EPSILON) / (d1-d2);
				if (f < leavefrac)
					leavefrac = f;
			}
		}
	}



	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > NEVER_UPDATED && enterfrac < trace->fraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane.dist = clipplane->dist;
			trace->plane.normal = clipplane->normal;
			trace->plane.type = clipplane->type;
			if (leadside->texinfo!=-1)
				trace->surface = &texinfo[leadside->texinfo];
			else
				trace->surface = 0;
			trace->contents = brush->contents;
		}
	}
}

void TestLine_DoesHitSky( FourVectors const& start, FourVectors const& stop,
	fltx4 *pFractionVisible, bool canRecurse, int static_prop_to_skip, bool bDoDebug )
{
	FourRays myrays;
	myrays.origin = start;
	myrays.direction = stop;
	myrays.direction -= myrays.origin;
	fltx4 len = myrays.direction.length();
	myrays.direction *= ReciprocalSIMD( len );
	RayTracingResult rt_result;
	CCoverageCountTexture coverageCallback;

	g_RtEnv.Trace4Rays(myrays, Four_Zeros, len, &rt_result, TRACE_ID_STATICPROP | static_prop_to_skip, g_bTextureShadows? &coverageCallback : 0);

	if ( bDoDebug )
	{
		WriteTrace( "trace.txt", myrays, rt_result );
	}

	float aOcclusion[4];
	for ( int i = 0; i < 4; i++ )
	{
		aOcclusion[i] = 0.0f;
		if ( ( rt_result.HitIds[i] != -1 ) &&
		     ( rt_result.HitDistance.m128_f32[i] < len.m128_f32[i] ) )
		{
			int id = g_RtEnv.OptimizedTriangleList[rt_result.HitIds[i]].m_Data.m_IntersectData.m_nTriangleID;
			if ( !( id & TRACE_ID_SKY ) )
				aOcclusion[i] = 1.0f;
		}
	}
	fltx4 occlusion = LoadUnalignedSIMD( aOcclusion );
	if (g_bTextureShadows)
		occlusion = MaxSIMD ( occlusion, coverageCallback.GetCoverage() );

	bool fullyOccluded = ( TestSignSIMD( CmpGeSIMD( occlusion, Four_Ones ) ) == 0xF );

	// if we hit sky, and we're not in a sky camera's area, try clipping into the 3D sky boxes
	if ( (! fullyOccluded) && canRecurse && (! g_bNoSkyRecurse ) )
	{
		FourVectors dir = stop;
		dir -= start;
		dir.VectorNormalize();

		int leafIndex = -1;
		leafIndex = PointLeafnum( start.Vec( 0 ) );
		if ( leafIndex >= 0 )
		{
			int area = dleafs[leafIndex].area;
			if (area >= 0 && area < numareas)
			{
				if (area_sky_cameras[area] < 0)
				{
					int cam;
					for (cam = 0; cam < num_sky_cameras; ++cam)
					{
						FourVectors skystart, skytrans, skystop;
						skystart.DuplicateVector( sky_cameras[cam].origin );
						skystop = start;
						skystop *= sky_cameras[cam].world_to_sky;
						skystart += skystop;

						skystop = dir;
						skystop *= MAX_TRACE_LENGTH;
						skystop += skystart;
						TestLine_DoesHitSky ( skystart, skystop, pFractionVisible, false, static_prop_to_skip, bDoDebug );
						occlusion = AddSIMD ( occlusion, Four_Ones );
						occlusion = SubSIMD ( occlusion, *pFractionVisible );
					}
				}
			}
		}
	}

	occlusion = MaxSIMD( occlusion, Four_Zeros );
	occlusion = MinSIMD( occlusion, Four_Ones );
	*pFractionVisible = SubSIMD( Four_Ones, occlusion );
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int PointLeafnum_r( const Vector &point, int ndxNode )
{
	// while loop here is to avoid recursion overhead
	while( ndxNode >= 0 )
	{
		dnode_t *pNode = dnodes + ndxNode;
		dplane_t *pPlane = dplanes + pNode->planenum;

		float dist;
		if( pPlane->type < 3 )
		{
			dist = point[pPlane->type] - pPlane->dist;
		}
		else
		{
			dist = DotProduct( pPlane->normal, point ) - pPlane->dist;
		}

		if( dist < 0.0f )
		{
			ndxNode = pNode->children[1];
		}
		else
		{
			ndxNode = pNode->children[0];
		}
	}

	return ( -1 - ndxNode );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int PointLeafnum( const Vector &point )
{
	return PointLeafnum_r( point, 0 );
}

// this iterates the list of entities looking for _vradshadows 1
// each brush entity containing this key is added to the raytracing environment
// as a triangle soup model.

dmodel_t *BrushmodelForEntity( entity_t *pEntity )
{
	const char *pModelname = ValueForKey( pEntity, "model" );
	if ( Q_strlen(pModelname) > 1 )
	{
		int modelIndex = atol( pModelname + 1 );
		if ( modelIndex > 0 && modelIndex < nummodels )
		{
			return &dmodels[modelIndex];
		}
	}
	return NULL;
}

void AddBrushToRaytraceEnvironment( dbrush_t *pBrush, const VMatrix &xform )
{
	if ( !( pBrush->contents & MASK_OPAQUE ) )
		return;

	Vector v0, v1, v2;
	for (int i = 0; i < pBrush->numsides; i++ )
	{
		dbrushside_t *side = &dbrushsides[pBrush->firstside + i];
		dplane_t *plane = &dplanes[side->planenum];
		texinfo_t *tx = &texinfo[side->texinfo];
		winding_t *w = BaseWindingForPlane (plane->normal, plane->dist);

		if ( tx->flags & SURF_SKY || side->dispinfo )
			continue;

		for (int j=0 ; j<pBrush->numsides && w; j++)
		{
			if (i == j)
				continue;
			dbrushside_t *pOtherSide = &dbrushsides[pBrush->firstside + j];
			if (pOtherSide->bevel)
				continue;
			plane = &dplanes[pOtherSide->planenum^1];
			ChopWindingInPlace (&w, plane->normal, plane->dist, 0);
		}
		if ( w )
		{
			for ( int j = 2; j < w->numpoints; j++ )
			{
				v0 = xform.VMul4x3(w->p[0]);
				v1 = xform.VMul4x3(w->p[j-1]);
				v2 = xform.VMul4x3(w->p[j]);
				Vector fullCoverage;
				fullCoverage.x = 1.0f;
				g_RtEnv.AddTriangle(TRACE_ID_OPAQUE, v0, v1, v2, fullCoverage);
			}
			FreeWinding( w );
		}
	}
}


// recurse the bsp and build a list of brushes at the leaves under this node
void GetBrushes_r( int node, CUtlVector<int> &list )
{
	if ( node < 0 )
	{
		int leafIndex = -1 - node;
		// Add the solids in the leaf
		for ( int i = 0; i < dleafs[leafIndex].numleafbrushes; i++ )
		{
			int brushIndex = dleafbrushes[dleafs[leafIndex].firstleafbrush + i];
			if ( list.Find(brushIndex) < 0 )
			{
				list.AddToTail( brushIndex );
			}
		}
	}
	else
	{
		// recurse
		dnode_t *pnode = dnodes + node;

		GetBrushes_r( pnode->children[0], list );
		GetBrushes_r( pnode->children[1], list );
	}
}


void AddBrushes( dmodel_t *pModel, const VMatrix &xform )
{
	if ( pModel )
	{
		CUtlVector<int> brushList;
		GetBrushes_r( pModel->headnode, brushList );
		for ( int i = 0; i < brushList.Count(); i++ )
		{
			int ndxBrush = brushList[i];
			AddBrushToRaytraceEnvironment( &dbrushes[ndxBrush], xform );
		}
	}
}


// Adds the brush entities that cast shadows to the raytrace environment
void ExtractBrushEntityShadowCasters()
{
	for ( int i = 0; i < num_entities; i++ )
	{
		if ( IntForKey( &entities[i], "vrad_brush_cast_shadows" ) != 0 )
		{
			Vector origin;
			QAngle angles;
			GetVectorForKey( &entities[i], "origin", origin );
			GetAnglesForKey( &entities[i], "angles", angles );
			VMatrix xform;
			xform.SetupMatrixOrgAngles( origin, angles );
			AddBrushes( BrushmodelForEntity( &entities[i] ), xform );
		}
	}
}

void AddBrushesForRayTrace( void )
{
	if ( !nummodels )
		return;

	VMatrix identity;
	identity.Identity();
	
	CUtlVector<int> brushList;
	GetBrushes_r ( dmodels[0].headnode, brushList );

	for ( int i = 0; i < brushList.Size(); i++ )
	{
		dbrush_t *brush = &dbrushes[brushList[i]];
		AddBrushToRaytraceEnvironment ( brush, identity );
	}

	for ( int i = 0; i < dmodels[0].numfaces; i++ )
	{
		int ndxFace = dmodels[0].firstface + i;
		dface_t *face = &g_pFaces[ndxFace];

		texinfo_t *tx = &texinfo[face->texinfo];
		if ( !( tx->flags & SURF_SKY ) )
			continue;

		Vector points[MAX_POINTS_ON_WINDING];

		for ( int j = 0; j < face->numedges; j++ )
		{
			if ( j >= MAX_POINTS_ON_WINDING )
				Error( "***** ERROR! MAX_POINTS_ON_WINDING reached!" );

			if ( face->firstedge + j >= ARRAYSIZE( dsurfedges ) )
				Error( "***** ERROR! face->firstedge + j >= ARRAYSIZE( dsurfedges )!" );

			int surfEdge = dsurfedges[face->firstedge + j];
			unsigned short v;

			if (surfEdge < 0)
				v = dedges[-surfEdge].v[1];
			else
				v = dedges[surfEdge].v[0];

			if ( v >= ARRAYSIZE( dvertexes ) )
				Error( "***** ERROR! v(%u) >= ARRAYSIZE( dvertexes(%d) )!", ( unsigned int )v, ARRAYSIZE( dvertexes ) );

			dvertex_t *dv = &dvertexes[v];
			points[j] = dv->point;
		}

		for ( int j = 2; j < face->numedges; j++ )
		{
			Vector fullCoverage;
			fullCoverage.x = 1.0f;
			g_RtEnv.AddTriangle ( TRACE_ID_SKY, points[0], points[j - 1], points[j], fullCoverage );
		}
	}
}
