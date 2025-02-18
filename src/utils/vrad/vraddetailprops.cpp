//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//
// This file contains code to allow us to associate client data with bsp leaves.
//
//=============================================================================//

#include "vrad.h"
#include "Bsplib.h"
#include "GameBSPFile.h"
#include "UtlBuffer.h"
#include "utlvector.h"
#include "CModel.h"
#include "studio.h"
#include "pacifier.h"
#include "vraddetailprops.h"
#include "mathlib/halton.h"
#include "messbuf.h"
#include "byteswap.h"

bool LoadStudioModel( char const* pModelName, CUtlBuffer& buf );


//-----------------------------------------------------------------------------
// Purpose: Writes a glview text file containing the collision surface in question
// Input  : *pCollide - 
//			*pFilename - 
//-----------------------------------------------------------------------------
void DumpRayToGlView( Ray_t const& ray, float dist, Vector* pColor, const char *pFilename )
{
	Vector dir =  ray.m_Delta;
	float len = VectorNormalize(dir);
	if (len < 1e-3)
		return;

	Vector up( 0, 0, 1 );
	Vector crossDir;
	if (fabs(DotProduct(up, dir)) - 1.0f < -1e-3 )
	{
		CrossProduct( dir, up, crossDir );
		VectorNormalize(crossDir);
	}
	else
	{
		up.Init( 0, 1, 0 );
		CrossProduct( dir, up, crossDir );
		VectorNormalize(crossDir);
	}

	Vector end;
	Vector start1, start2;
	VectorMA( ray.m_Start, dist, ray.m_Delta, end );
	VectorMA( ray.m_Start, -2, crossDir, start1 );
	VectorMA( ray.m_Start, 2, crossDir, start2 );

	FileHandle_t fp = g_pFileSystem->Open( pFilename, "a" );
	int vert = 0;
	CmdLib_FPrintf( fp, "3\n" );
	CmdLib_FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", start1.x, start1.y, start1.z,
		pColor->x, pColor->y, pColor->z );
	vert++;
	CmdLib_FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", start2.x, start2.y, start2.z,
		pColor->x, pColor->y, pColor->z );
	vert++;
	CmdLib_FPrintf( fp, "%6.3f %6.3f %6.3f %.2f %.2f %.2f\n", end.x, end.y, end.z,
		pColor->x, pColor->y, pColor->z );
	vert++;
	g_pFileSystem->Close( fp );
}


//-----------------------------------------------------------------------------
// This puppy is used to construct the game lumps
//-----------------------------------------------------------------------------
static CUtlVector<DetailPropLightstylesLump_t>	s_DetailPropLightStyleLumpLDR;
static CUtlVector<DetailPropLightstylesLump_t>	s_DetailPropLightStyleLumpHDR;
static CUtlVector<DetailPropLightstylesLump_t> *s_pDetailPropLightStyleLump = &s_DetailPropLightStyleLumpLDR;

//-----------------------------------------------------------------------------
// An amount to add to each model to get to the model center
//-----------------------------------------------------------------------------
CUtlVector<Vector> g_ModelCenterOffset;
CUtlVector<Vector> g_SpriteCenterOffset;

void VRadDetailProps_SetHDRMode( bool bHDR )
{
	if( bHDR )
	{
		s_pDetailPropLightStyleLump = &s_DetailPropLightStyleLumpHDR;
	}
	else
	{
		s_pDetailPropLightStyleLump = &s_DetailPropLightStyleLumpLDR;
	}
}

//-----------------------------------------------------------------------------
// Finds ambient sky lights
//-----------------------------------------------------------------------------
static directlight_t* FindAmbientSkyLight()
{
	static directlight_t *s_pCachedSkylight = NULL;

	// Don't keep searching for the same light.
	if ( !s_pCachedSkylight )
	{
		// find any ambient lights
		directlight_t* dl;
		for (dl = activelights; dl != 0; dl = dl->next)
		{
			if (dl->light.type == emit_skyambient)
			{
				s_pCachedSkylight = dl;
				break;
			}
		}
	}

	return s_pCachedSkylight;
}


//-----------------------------------------------------------------------------
// Compute world center of a prop
//-----------------------------------------------------------------------------
static void ComputeWorldCenter( DetailObjectLump_t& prop, Vector& center, Vector& normal )
{
	// Transform the offset into world space
	Vector forward, right;
	AngleVectors( prop.m_Angles, &forward, &right, &normal );
	VectorCopy( prop.m_Origin, center );

	// FIXME: Take orientation into account?
	switch (prop.m_Type )
	{
	case DETAIL_PROP_TYPE_MODEL:
		VectorMA( center, g_ModelCenterOffset[prop.m_DetailModel].x, forward, center );
		VectorMA( center, -g_ModelCenterOffset[prop.m_DetailModel].y, right, center );
		VectorMA( center, g_ModelCenterOffset[prop.m_DetailModel].z, normal, center );
		break;

	case DETAIL_PROP_TYPE_SPRITE:
		Vector vecOffset;
		VectorMultiply( g_SpriteCenterOffset[prop.m_DetailModel], prop.m_flScale, vecOffset );
		VectorMA( center, vecOffset.x, forward, center );
		VectorMA( center, -vecOffset.y, right, center );
		VectorMA( center, vecOffset.z, normal, center );
		break;
	}
}


//-----------------------------------------------------------------------------
// Computes max direct lighting for a single detal prop
//-----------------------------------------------------------------------------
static void ComputeMaxDirectLighting( DetailObjectLump_t& prop, Vector* maxcolor, int iThread )
{
	// The max direct lighting must be along the direction to one
	// of the static lights....

	Vector origin, normal;
	ComputeWorldCenter( prop, origin, normal );

	if ( !origin.IsValid() || !normal.IsValid() )
	{
		static bool s_Warned = false;
		if ( !s_Warned )
		{
			Warning("WARNING: Bogus detail props encountered!\n" );
			s_Warned = true;
		}

		// fill with debug color
		for ( int i = 0; i < MAX_LIGHTSTYLES; ++i)
		{
			maxcolor[i].Init(1,0,0);
		}
		return;
	}

	int cluster = ClusterFromPoint(origin);

	Vector delta;
	CUtlVector< directlight_t* >	lights;
	CUtlVector< Vector >			directions;

	directlight_t* dl;
	for (dl = activelights; dl != 0; dl = dl->next)
	{
		// skyambient doesn't affect dlights..
		if (dl->light.type == emit_skyambient)
			continue;

		// is this lights cluster visible?
		if ( PVSCheck( dl->pvs, cluster ) )
		{
			lights.AddToTail(dl);
			VectorSubtract( dl->light.origin, origin, delta );
			VectorNormalize( delta );
			directions.AddToTail( delta );
		}
	}

	// Find the max illumination
	int i;
	for ( i = 0; i < MAX_LIGHTSTYLES; ++i)
	{
		maxcolor[i].Init(0,0,0);
	}

	// NOTE: See version 10 for a method where we choose a normal based on whichever
	// one produces the maximum possible illumination. This appeared to work better on
	// e3_town, so I'm trying it now; hopefully it'll be good for all cases.
	int j;
	for ( j = 0; j < lights.Count(); ++j)
	{
		dl = lights[j];

		SSE_sampleLightOutput_t out;
		FourVectors origin4;
		FourVectors normal4;
		origin4.DuplicateVector( origin );
		normal4.DuplicateVector( normal );

		GatherSampleLightSSE ( out, dl, -1, origin4, &normal4, 1, iThread );
		VectorMA( maxcolor[dl->light.style], out.m_flFalloff.m128_f32[0] * out.m_flDot[0].m128_f32[0], dl->light.intensity, maxcolor[dl->light.style] );
	}
}


//-----------------------------------------------------------------------------
// Computes the ambient term from a particular surface
//-----------------------------------------------------------------------------

static void ComputeAmbientFromSurface( dface_t* pFace, directlight_t* pSkylight, 
									   Vector& radcolor )
{
	texinfo_t* pTex = &texinfo[pFace->texinfo];
	if (pTex)
	{
		// If we hit the sky, use the sky ambient
		if (pTex->flags & SURF_SKY)
		{
			if (pSkylight)
			{
				// add in sky ambient
				VectorDivide( pSkylight->light.intensity, 255.0f, radcolor ); 
			}
		}
		else
		{
			VectorMultiply( radcolor, dtexdata[pTex->texdata].reflectivity, radcolor );
		}
	}
}


//-----------------------------------------------------------------------------
// Computes the lightmap color at a particular point
//-----------------------------------------------------------------------------

static void ComputeLightmapColorFromAverage( dface_t* pFace, directlight_t* pSkylight, float scale, Vector pColor[MAX_LIGHTSTYLES] )
{
	texinfo_t* pTex = &texinfo[pFace->texinfo];
	if (pTex->flags & SURF_SKY)
	{
		if (pSkylight)
		{
			// add in sky ambient
			Vector amb = pSkylight->light.intensity / 255.0f; 
			pColor[0] += amb * scale;
		}
		return;
	}

	for (int maps = 0 ; maps < MAXLIGHTMAPS && pFace->styles[maps] != 255 ; ++maps)
	{
		ColorRGBExp32* pAvgColor = dface_AvgLightColor( pFace, maps );

		// this code expects values from [0..1] not [0..255]
		Vector color;
		color[0] = TexLightToLinear( pAvgColor->r, pAvgColor->exponent );
		color[1] = TexLightToLinear( pAvgColor->g, pAvgColor->exponent );
		color[2] = TexLightToLinear( pAvgColor->b, pAvgColor->exponent );

		ComputeAmbientFromSurface( pFace, pSkylight, color );

		int style = pFace->styles[maps];
		pColor[style] += color * scale;
	}
}


//-----------------------------------------------------------------------------
// Returns true if the surface has bumped lightmaps
//-----------------------------------------------------------------------------

static bool SurfHasBumpedLightmaps( dface_t *pSurf )
{
	bool hasBumpmap = false;
	if( ( texinfo[pSurf->texinfo].flags & SURF_BUMPLIGHT ) && 
		( !( texinfo[pSurf->texinfo].flags & SURF_NOLIGHT ) ) )
	{
		hasBumpmap = true;
	}
	return hasBumpmap;
}

//-----------------------------------------------------------------------------
// Computes the lightmap color at a particular point
//-----------------------------------------------------------------------------

static void ComputeLightmapColorPointSample( dface_t* pFace, directlight_t* pSkylight, Vector2D const& luv, float scale, Vector pColor[MAX_LIGHTSTYLES] )
{
	// face unaffected by light
	if (pFace->lightofs == -1 )
		return;

	int smax = ( pFace->m_LightmapTextureSizeInLuxels[0] ) + 1;
	int tmax = ( pFace->m_LightmapTextureSizeInLuxels[1] ) + 1;

	// luv is in the space of the accumulated lightmap page; we need to convert
	// it to be in the space of the surface
	int ds = clamp( (int)luv.x, 0, smax-1 );
	int dt = clamp( (int)luv.y, 0, tmax-1 );

	int offset = smax * tmax;
	if ( SurfHasBumpedLightmaps( pFace ) )
		offset *= ( NUM_BUMP_VECTS + 1 );

	ColorRGBExp32* pLightmap = (ColorRGBExp32*)&pdlightdata->Base()[pFace->lightofs];
	pLightmap += dt * smax + ds;
	for (int maps = 0 ; maps < MAXLIGHTMAPS && pFace->styles[maps] != 255 ; ++maps)
	{
		int style = pFace->styles[maps];

		Vector color;
		color[0] = TexLightToLinear( pLightmap->r, pLightmap->exponent );
		color[1] = TexLightToLinear( pLightmap->g, pLightmap->exponent );
		color[2] = TexLightToLinear( pLightmap->b, pLightmap->exponent );

		ComputeAmbientFromSurface( pFace, pSkylight, color );
		pColor[style] += color * scale;

		pLightmap += offset;
	}
}


//-----------------------------------------------------------------------------
// Tests a particular node
//-----------------------------------------------------------------------------

class CLightSurface : public IBSPNodeEnumerator
{
public:
	CLightSurface(int iThread) : m_pSurface(0), m_HitFrac(1.0f), m_bHasLuxel(false), m_iThread(iThread) {}

	// call back with a node and a context
	bool EnumerateNode( int node, Ray_t const& ray, float f, int context )
	{
		dface_t* pSkySurface = 0;

		// Compute the actual point
		Vector pt;
		VectorMA( ray.m_Start, f, ray.m_Delta, pt );

		dnode_t* pNode = &dnodes[node];
		dface_t* pFace = &g_pFaces[pNode->firstface];
		for (int i=0 ; i < pNode->numfaces ; ++i, ++pFace)
		{
			// Don't take into account faces that are int a leaf
			if ( !pFace->onNode )
				continue;

			// Don't test displacement faces
			if ( pFace->dispinfo != -1 )
				continue;

			texinfo_t* pTex = &texinfo[pFace->texinfo];

			// Don't immediately return when we hit sky; 
			// we may actually hit another surface
			if (pTex->flags & SURF_SKY)
			{
				if (TestPointAgainstSkySurface( pt, pFace ))
				{
					pSkySurface = pFace;
				}

				continue;
			}

			if (TestPointAgainstSurface( pt, pFace, pTex ))
			{
				m_HitFrac = f;
				m_pSurface = pFace;
				m_bHasLuxel = true;
				return false;
			}
		}

		// if we hit a sky surface, return it
		m_pSurface = pSkySurface;
		return (m_pSurface == 0);
	}

	// call back with a leaf and a context
	virtual bool EnumerateLeaf( int leaf, Ray_t const& ray, float start, float end, int context )
	{
		bool hit = false;
		dleaf_t* pLeaf = &dleafs[leaf];
		for (int i=0 ; i < pLeaf->numleaffaces ; ++i)
		{
			Assert( pLeaf->firstleafface + i < numleaffaces );
			Assert( dleaffaces[pLeaf->firstleafface + i] < numfaces );
			dface_t* pFace = &g_pFaces[dleaffaces[pLeaf->firstleafface + i]];

			// Don't test displacement faces; we need to check another list
			if ( pFace->dispinfo != -1 )
				continue;

			// Don't take into account faces that are on a node
			if ( pFace->onNode )
				continue;

			// Find intersection point against detail brushes
			texinfo_t* pTex = &texinfo[pFace->texinfo];

			dplane_t* pPlane = &dplanes[pFace->planenum];

			// Backface cull...
			if (DotProduct( pPlane->normal, ray.m_Delta ) > 0)
				continue;

			float startDotN = DotProduct( ray.m_Start, pPlane->normal );
			float deltaDotN = DotProduct( ray.m_Delta, pPlane->normal );

			float front = startDotN + start * deltaDotN - pPlane->dist;
			float back = startDotN + end * deltaDotN - pPlane->dist;
			
			int side = front < 0;

			// Blow it off if it doesn't split the plane...
			if ( (back < 0) == side )
				continue;

			// Don't test a surface that is farther away from the closest found intersection
			float f = front / (front-back);
			float mid = start * (1.0f - f) + end * f;
			if (mid >= m_HitFrac)
				continue;

			Vector pt;
			VectorMA( ray.m_Start, mid, ray.m_Delta, pt );

			if (TestPointAgainstSurface( pt, pFace, pTex ))
			{
				m_HitFrac = mid;
				m_pSurface = pFace;
				hit = true;
				m_bHasLuxel = true;
			}
		}

		// Now try to clip against all displacements in the leaf
		float dist;
		Vector2D luxelCoord;
		dface_t *pDispFace;
		StaticDispMgr()->ClipRayToDispInLeaf( s_DispTested[m_iThread], ray, leaf, dist, pDispFace, luxelCoord );
		if (dist < m_HitFrac)
		{
			m_HitFrac = dist;
			m_pSurface = pDispFace;
			Vector2DCopy( luxelCoord, m_LuxelCoord );
			hit = true;
			m_bHasLuxel = true;
		}
		return !hit;
	}

	bool FindIntersection( Ray_t const& ray )
	{
		StaticDispMgr()->StartRayTest( s_DispTested[m_iThread] );
		return !EnumerateNodesAlongRay( ray, this, 0 );
	}

private:
	bool TestPointAgainstSurface( Vector const& pt, dface_t* pFace, texinfo_t* pTex )
	{
		// no lightmaps on this surface? punt...
		// FIXME: should be water surface?
		if (pTex->flags & SURF_NOLIGHT)
			return false;	
		
		// See where in lightmap space our intersection point is 
		float s, t;
		s = DotProduct (pt.Base(), pTex->lightmapVecsLuxelsPerWorldUnits[0]) + 
			pTex->lightmapVecsLuxelsPerWorldUnits[0][3];
		t = DotProduct (pt.Base(), pTex->lightmapVecsLuxelsPerWorldUnits[1]) + 
			pTex->lightmapVecsLuxelsPerWorldUnits[1][3];

		// Not in the bounds of our lightmap? punt...
		if( s < pFace->m_LightmapTextureMinsInLuxels[0] || t < pFace->m_LightmapTextureMinsInLuxels[1] )
			return false;	
		
		// assuming a square lightmap (FIXME: which ain't always the case),
		// lets see if it lies in that rectangle. If not, punt...
		float ds = s - pFace->m_LightmapTextureMinsInLuxels[0];
		float dt = t - pFace->m_LightmapTextureMinsInLuxels[1];
		if( ds > pFace->m_LightmapTextureSizeInLuxels[0] || dt > pFace->m_LightmapTextureSizeInLuxels[1] )
			return false;	

		m_LuxelCoord.x = ds;
		m_LuxelCoord.y = dt;

		return true;
	}

	bool TestPointAgainstSkySurface( Vector const &pt, dface_t *pFace )
	{
		// Create sky face winding.
		winding_t *pWinding = WindingFromFace( pFace, Vector( 0.0f, 0.0f, 0.0f ) );

		// Test point in winding. (Since it is at the node, it is in the plane.)
		bool bRet = PointInWinding( pt, pWinding );

		FreeWinding( pWinding );

		return bRet;
	}


public:
	int	m_iThread;
	dface_t* m_pSurface;
	float	m_HitFrac;
	Vector2D	m_LuxelCoord;
	bool	m_bHasLuxel;
};

bool CastRayInLeaf( int iThread, const Vector &start, const Vector &end, int leafIndex, float *pFraction, Vector *pNormal )
{
	pFraction[0] = 1.0f;

	Ray_t ray;
	ray.Init( start, end, vec3_origin, vec3_origin );
	CBaseTrace trace;
	if ( TraceLeafBrushes( leafIndex, start, end, trace ) != 1.0f )
	{
		pFraction[0] = trace.fraction;
		*pNormal = trace.plane.normal;
	}
	else
	{
		Assert(!trace.startsolid && !trace.allsolid);
	}
	StaticDispMgr()->StartRayTest( s_DispTested[iThread] );
	// Now try to clip against all displacements in the leaf
	float dist;
	Vector normal;
	StaticDispMgr()->ClipRayToDispInLeaf( s_DispTested[iThread], ray, leafIndex, dist, &normal );
	if ( dist < pFraction[0] )
	{
		pFraction[0] = dist;
		*pNormal = normal;
	}
	return pFraction[0] != 1.0f ? true : false;
}

//-----------------------------------------------------------------------------
// Computes ambient lighting along a specified ray.  
// Ray represents a cone, tanTheta is the tan of the inner cone angle
//-----------------------------------------------------------------------------
void CalcRayAmbientLighting( int iThread, const Vector &vStart, const Vector &vEnd, float tanTheta, Vector color[MAX_LIGHTSTYLES] )
{
	Ray_t ray;
	ray.Init( vStart, vEnd, vec3_origin, vec3_origin );

	directlight_t *pSkyLight = FindAmbientSkyLight();

	CLightSurface surfEnum(iThread);
	if (!surfEnum.FindIntersection( ray ))
		return;

	// compute the approximate radius of a circle centered around the intersection point
	float dist = ray.m_Delta.Length() * tanTheta * surfEnum.m_HitFrac;

	// until 20" we use the point sample, then blend in the average until we're covering 40"
	// This is attempting to model the ray as a cone - in the ideal case we'd simply sample all
	// luxels in the intersection of the cone with the surface.  Since we don't have surface 
	// neighbor information computed we'll just approximate that sampling with a blend between
	// a point sample and the face average.
	// This yields results that are similar in that aliasing is reduced at distance while 
	// point samples provide accuracy for intersections with near geometry
	float scaleAvg = RemapValClamped( dist, 20, 40, 0.0f, 1.0f );

	if ( !surfEnum.m_bHasLuxel )
	{
		// don't have luxel UV, so just use average sample
		scaleAvg = 1.0;
	}
	float scaleSample = 1.0f - scaleAvg;

	if (scaleAvg != 0)
	{
		ComputeLightmapColorFromAverage( surfEnum.m_pSurface, pSkyLight, scaleAvg, color );
	}
	if (scaleSample != 0)
	{
		ComputeLightmapColorPointSample( surfEnum.m_pSurface, pSkyLight, surfEnum.m_LuxelCoord, scaleSample, color );
	}
}

//-----------------------------------------------------------------------------
// Compute ambient lighting component at specified position.
//-----------------------------------------------------------------------------
static void ComputeAmbientLightingAtPoint( int iThread, const Vector &origin, Vector radcolor[NUMVERTEXNORMALS], Vector color[MAX_LIGHTSTYLES] )
{
	// NOTE: I'm not dealing with shadow-casting static props here
	// This is for speed, although we can add it if it turns out to
	// be important

	// sample world by casting N rays distributed across a sphere
	Vector upend;

	int j;
	for ( j = 0; j < MAX_LIGHTSTYLES; ++j)
	{
		color[j].Init( 0,0,0 );
	}

	float tanTheta = tan(VERTEXNORMAL_CONE_INNER_ANGLE);
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		VectorMA( origin, COORD_EXTENT * 1.74, g_anorms[i], upend );

		// Now that we've got a ray, see what surface we've hit
		CalcRayAmbientLighting( iThread, origin, upend, tanTheta, color );

//		DumpRayToGlView( ray, surfEnum.m_HitFrac, &color[0], "test.out" );
	}

	for ( j = 0; j < MAX_LIGHTSTYLES; ++j)
	{
		VectorMultiply( color[j], 255.0f / (float)NUMVERTEXNORMALS, color[j] );
	}
}

//-----------------------------------------------------------------------------
// Trace hemispherical rays from a vertex, accumulating indirect
// sources at each ray termination.
//-----------------------------------------------------------------------------
void ComputeIndirectLightingAtPoint( Vector &position, Vector &normal, Vector &outColor,
									 int iThread, bool force_fast, bool bIgnoreNormals )
{
	Ray_t			ray;
	CLightSurface	surfEnum(iThread);

	outColor.Init();

	
	int nSamples = NUMVERTEXNORMALS;
	if ( do_fast || force_fast )
		nSamples /= 4;
	else
		nSamples *= g_flSkySampleScale;

	float totalDot = 0;
	DirectionalSampler_t sampler;
	for (int j = 0; j < nSamples; j++)
	{
		Vector samplingNormal = sampler.NextValue();
		float dot;

		if ( bIgnoreNormals )
			dot = (0.7071/2);
		else
			dot = DotProduct( normal, samplingNormal );

		if ( dot <= EQUAL_EPSILON )
		{
			// reject angles behind our plane
			continue;
		}

		totalDot += dot;

		// trace to determine surface
		Vector vEnd;
		VectorScale( samplingNormal, MAX_TRACE_LENGTH, vEnd );
		VectorAdd( position, vEnd, vEnd );

		ray.Init( position, vEnd, vec3_origin, vec3_origin );
		if ( !surfEnum.FindIntersection( ray ) )
			continue;

		// get color from surface lightmap
		texinfo_t* pTex = &texinfo[surfEnum.m_pSurface->texinfo];
		if ( !pTex || pTex->flags & SURF_SKY )
		{
			// ignore contribution from sky
			// sky ambient already accounted for during direct pass
			continue;
		}

		if ( surfEnum.m_pSurface->styles[0] == 255 || surfEnum.m_pSurface->lightofs < 0 )
		{
			// no light affects this face
			continue;
		}


		Vector lightmapColor;
		if ( !surfEnum.m_bHasLuxel )
		{
			ColorRGBExp32* pAvgLightmapColor = dface_AvgLightColor( surfEnum.m_pSurface, 0 );
			ColorRGBExp32ToVector( *pAvgLightmapColor, lightmapColor );
		}
		else
		{
			// get color from displacement
			int smax = ( surfEnum.m_pSurface->m_LightmapTextureSizeInLuxels[0] ) + 1;
			int tmax = ( surfEnum.m_pSurface->m_LightmapTextureSizeInLuxels[1] ) + 1;

			// luxelcoord is in the space of the accumulated lightmap page; we need to convert
			// it to be in the space of the surface
			int ds = clamp( (int)surfEnum.m_LuxelCoord.x, 0, smax-1 );
			int dt = clamp( (int)surfEnum.m_LuxelCoord.y, 0, tmax-1 );

			ColorRGBExp32* pLightmap = (ColorRGBExp32*)&(*pdlightdata)[surfEnum.m_pSurface->lightofs];
			pLightmap += dt * smax + ds;
			ColorRGBExp32ToVector( *pLightmap, lightmapColor );
		}

		float invLengthSqr = 1.0f / (1.0f + ((vEnd - position) * surfEnum.m_HitFrac / 128.0).LengthSqr());
		// Include falloff using invsqrlaw.
		VectorMultiply( lightmapColor, invLengthSqr * dtexdata[pTex->texdata].reflectivity, lightmapColor );
		VectorAdd( outColor, lightmapColor, outColor );
	}

	if ( totalDot )
	{
		VectorScale( outColor, 1.0f/totalDot, outColor );
	}
}

static void ComputeAmbientLighting( int iThread, DetailObjectLump_t& prop, Vector color[MAX_LIGHTSTYLES] )
{
	Vector origin, normal;
	ComputeWorldCenter( prop, origin, normal );

	if ( !origin.IsValid() || !normal.IsValid() )
	{
		static bool s_Warned = false;
		if ( !s_Warned )
		{
			Warning("WARNING: Bogus detail props encountered!\n" );
			s_Warned = true;
		}

		// fill with debug color
		for ( int i = 0; i < MAX_LIGHTSTYLES; ++i)
		{
			color[i].Init(1,0,0);
		}
		return;
	}

	Vector radcolor[NUMVERTEXNORMALS];
	ComputeAmbientLightingAtPoint( iThread, origin, radcolor, color );
}


//-----------------------------------------------------------------------------
// Computes lighting for a single detal prop
//-----------------------------------------------------------------------------

static void ComputeLighting( DetailObjectLump_t& prop, int iThread )
{
	// We're going to take the maximum of the ambient lighting and 
	// the strongest directional light. This works because we're assuming
	// the props will have built-in faked lighting.

	Vector directColor[MAX_LIGHTSTYLES];
	Vector ambColor[MAX_LIGHTSTYLES];

	// Get the max influence of all direct lights
	ComputeMaxDirectLighting( prop, directColor, iThread );

	// Get the ambient lighting + lightstyles	  
	ComputeAmbientLighting( iThread, prop, ambColor );

	// Base lighting
	Vector totalColor;
	VectorAdd( directColor[0], ambColor[0], totalColor );
	VectorToColorRGBExp32( totalColor, prop.m_Lighting );

	bool hasLightstyles = false;
	prop.m_LightStyleCount = 0;
	
	// lightstyles
	for (int i = 1; i < MAX_LIGHTSTYLES; ++i )
	{
		VectorAdd( directColor[i], ambColor[i], totalColor );
		totalColor *= 0.5f;

		if ((totalColor[0] != 0.0f) || (totalColor[1] != 0.0f) ||
			(totalColor[2] != 0.0f) )
		{
			if (!hasLightstyles)
			{
				prop.m_LightStyles = s_pDetailPropLightStyleLump->Size();
				hasLightstyles = true;
			}

			int j = s_pDetailPropLightStyleLump->AddToTail();
			VectorToColorRGBExp32( totalColor, (*s_pDetailPropLightStyleLump)[j].m_Lighting );
			(*s_pDetailPropLightStyleLump)[j].m_Style = i;
			++prop.m_LightStyleCount;
		}
	}
}


//-----------------------------------------------------------------------------
// Unserialization
//-----------------------------------------------------------------------------
static void UnserializeModelDict( CUtlBuffer& buf )
{
	// Get origin offset for each model...
	int count = buf.GetInt();
	while ( --count >= 0 )
	{
		DetailObjectDictLump_t lump;
		buf.Get( &lump, sizeof(DetailObjectDictLump_t) );
		
		int i = g_ModelCenterOffset.AddToTail();

		CUtlBuffer mdlbuf;
		if (LoadStudioModel( lump.m_Name, mdlbuf ))
		{
			studiohdr_t* pHdr = (studiohdr_t*)mdlbuf.Base();
			VectorAdd( pHdr->hull_min, pHdr->hull_max, g_ModelCenterOffset[i] );
			g_ModelCenterOffset[i] *= 0.5f;
		}
		else
		{
			g_ModelCenterOffset[i].Init(0,0,0);
		}
	}
}

static void UnserializeSpriteDict( CUtlBuffer& buf )
{
	// Get origin offset for each model...
	int count = buf.GetInt();
	while ( --count >= 0 )
	{
		DetailSpriteDictLump_t lump;
		buf.Get( &lump, sizeof(DetailSpriteDictLump_t) );
		
		// For these sprites, x goes out the front, y right, z up
		int i = g_SpriteCenterOffset.AddToTail();
		g_SpriteCenterOffset[i].x = 0.0f;
		g_SpriteCenterOffset[i].y = lump.m_LR.x + lump.m_UL.x;
		g_SpriteCenterOffset[i].z = lump.m_LR.y + lump.m_UL.y;
		g_SpriteCenterOffset[i] *= 0.5f;
	}
}


//-----------------------------------------------------------------------------
// Unserializes the detail props
//-----------------------------------------------------------------------------
static int UnserializeDetailProps( DetailObjectLump_t*& pProps )
{
	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle( GAMELUMP_DETAIL_PROPS );

	if (g_GameLumps.GetGameLumpVersion(handle) != GAMELUMP_DETAIL_PROPS_VERSION)
		return 0;

	// Unserialize
	CUtlBuffer buf( g_GameLumps.GetGameLump(handle), g_GameLumps.GameLumpSize( handle ), CUtlBuffer::READ_ONLY );

	UnserializeModelDict( buf );
	UnserializeSpriteDict( buf );

	// Now we're pointing to the detail prop data
	// This actually works because the scope of the game lump data
	// is global and the buf was just pointing to it.
	int count = buf.GetInt();
	if (count)
	{
		pProps = (DetailObjectLump_t*)buf.PeekGet();
	}
	else
	{
		pProps = 0;
	}
	return count;
}


//-----------------------------------------------------------------------------
// Writes the detail lighting lump
//-----------------------------------------------------------------------------
static void WriteDetailLightingLump( int lumpID, int lumpVersion, CUtlVector<DetailPropLightstylesLump_t> &lumpData )
{
	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle(lumpID);
	if (handle != g_GameLumps.InvalidGameLump())
		g_GameLumps.DestroyGameLump(handle);
	int lightsize = lumpData.Size() * sizeof(DetailPropLightstylesLump_t);
	int lumpsize = lightsize + sizeof(int);

	handle = g_GameLumps.CreateGameLump( lumpID, lumpsize, 0, lumpVersion );

	// Serialize the data
	CUtlBuffer buf( g_GameLumps.GetGameLump(handle), lumpsize );
	buf.PutInt( lumpData.Size() );
	if (lightsize)
		buf.Put( lumpData.Base(), lightsize );
}

static void WriteDetailLightingLumps( void )
{
	WriteDetailLightingLump( GAMELUMP_DETAIL_PROP_LIGHTING, GAMELUMP_DETAIL_PROP_LIGHTING_VERSION, s_DetailPropLightStyleLumpLDR );
	WriteDetailLightingLump( GAMELUMP_DETAIL_PROP_LIGHTING_HDR, GAMELUMP_DETAIL_PROP_LIGHTING_HDR_VERSION, s_DetailPropLightStyleLumpHDR );
}

// need to do this so that if we are building HDR data, the LDR data is intact, and vice versa.s
void UnserializeDetailPropLighting( int lumpID, int lumpVersion, CUtlVector<DetailPropLightstylesLump_t> &lumpData )
{
	GameLumpHandle_t handle = g_GameLumps.GetGameLumpHandle( lumpID );

	if( handle == g_GameLumps.InvalidGameLump() )
	{
		return;
	}

	if (g_GameLumps.GetGameLumpVersion(handle) != lumpVersion)
		return;

	// Unserialize
	CUtlBuffer buf( g_GameLumps.GetGameLump(handle), g_GameLumps.GameLumpSize( handle ), CUtlBuffer::READ_ONLY );

	int count = buf.GetInt();
	if( !count )
	{
		return;
	}
	lumpData.SetCount( count );
	int lightsize = lumpData.Size() * sizeof(DetailPropLightstylesLump_t);
	buf.Get( lumpData.Base(), lightsize );
}

DetailObjectLump_t *g_pMPIDetailProps = NULL;

#ifdef MPI
void VMPI_ProcessDetailPropWU( int iThread, int iWorkUnit, MessageBuffer *pBuf )
{
	CUtlVector<DetailPropLightstylesLump_t> *pDetailPropLump = s_pDetailPropLightStyleLump;

	DetailObjectLump_t& prop = g_pMPIDetailProps[iWorkUnit];
	ComputeLighting( prop, iThread );

	// Send the results back...	
	pBuf->write( &prop.m_Lighting, sizeof( prop.m_Lighting ) );
	pBuf->write( &prop.m_LightStyleCount, sizeof( prop.m_LightStyleCount ) );
	pBuf->write( &prop.m_LightStyles, sizeof( prop.m_LightStyles ) );
	
	for ( int i=0; i < prop.m_LightStyleCount; i++ )
	{
		DetailPropLightstylesLump_t *l = &pDetailPropLump->Element( i + prop.m_LightStyles );
		pBuf->write( &l->m_Lighting, sizeof( l->m_Lighting ) );
		pBuf->write( &l->m_Style, sizeof( l->m_Style ) );
	}
}


void VMPI_ReceiveDetailPropWU( int iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	CUtlVector<DetailPropLightstylesLump_t> *pDetailPropLump = s_pDetailPropLightStyleLump;

	DetailObjectLump_t& prop = g_pMPIDetailProps[iWorkUnit];

	pBuf->read( &prop.m_Lighting, sizeof( prop.m_Lighting ) );
	pBuf->read( &prop.m_LightStyleCount, sizeof( prop.m_LightStyleCount ) );
	pBuf->read( &prop.m_LightStyles, sizeof( prop.m_LightStyles ) );
	
	pDetailPropLump->EnsureCount( prop.m_LightStyles + prop.m_LightStyleCount );
	
	for ( int i=0; i < prop.m_LightStyleCount; i++ )
	{
		DetailPropLightstylesLump_t *l = &pDetailPropLump->Element( i + prop.m_LightStyles );
		pBuf->read( &l->m_Lighting, sizeof( l->m_Lighting ) );
		pBuf->read( &l->m_Style, sizeof( l->m_Style ) );
	}
}
#endif
	
//-----------------------------------------------------------------------------
// Computes lighting for the detail props
//-----------------------------------------------------------------------------
void ComputeDetailPropLighting( int iThread )
{
	// illuminate them all
	DetailObjectLump_t* pProps;
	int count = UnserializeDetailProps( pProps );
	if (!count)
		return;

	// unserialize the lump that we aren't computing.
	if( g_bHDR )
	{
		UnserializeDetailPropLighting( GAMELUMP_DETAIL_PROP_LIGHTING, GAMELUMP_DETAIL_PROP_LIGHTING_VERSION, s_DetailPropLightStyleLumpLDR );
	}
	else
	{
		UnserializeDetailPropLighting( GAMELUMP_DETAIL_PROP_LIGHTING_HDR, GAMELUMP_DETAIL_PROP_LIGHTING_HDR_VERSION, s_DetailPropLightStyleLumpHDR );
	}

	StartPacifier("Computing detail prop lighting : ");

	for (int i = 0; i < count; ++i)
	{
		UpdatePacifier( (float)i / (float)count );
		ComputeLighting( pProps[i], iThread );
	}

	// Write detail prop lightstyle lump...
	WriteDetailLightingLumps();
	EndPacifier( true );
}
