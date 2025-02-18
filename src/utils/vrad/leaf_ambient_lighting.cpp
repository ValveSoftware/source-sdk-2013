//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "vrad.h"
#include "leaf_ambient_lighting.h"
#include "bsplib.h"
#include "vraddetailprops.h"
#include "mathlib/anorms.h"
#include "pacifier.h"
#include "coordsize.h"
#include "vstdlib/random.h"
#include "bsptreedata.h"
#include "messbuf.h"
#include "vmpi.h"
#include "vmpi_distribute_work.h"

static TableVector g_BoxDirections[6] = 
{
	{  1,  0,  0 }, 
	{ -1,  0,  0 },
	{  0,  1,  0 }, 
	{  0, -1,  0 }, 
	{  0,  0,  1 }, 
	{  0,  0, -1 }, 
};



static void ComputeAmbientFromSurface( dface_t *surfID, dworldlight_t* pSkylight, 
									   Vector& radcolor )
{
	if ( !surfID )
		return;

	texinfo_t *pTexInfo = &texinfo[surfID->texinfo];

	// If we hit the sky, use the sky ambient
	if ( pTexInfo->flags & SURF_SKY )
	{
		if ( pSkylight )
		{
			// add in sky ambient
			VectorCopy( pSkylight->intensity, radcolor );
		}
	}
	else
	{
		Vector reflectivity = dtexdata[pTexInfo->texdata].reflectivity;
		VectorMultiply( radcolor, reflectivity, radcolor );
	}
}


// TODO: it's CRAZY how much lighting code we share with the engine. It should all be shared code.
float Engine_WorldLightAngle( const dworldlight_t *wl, const Vector& lnormal, const Vector& snormal, const Vector& delta )
{
	float dot, dot2;

	Assert( wl->type == emit_surface );

	dot = DotProduct( snormal, delta );
	if (dot < 0)
		return 0;

	dot2 = -DotProduct (delta, lnormal);
	if (dot2 <= ON_EPSILON/10)
		return 0; // behind light surface

	return dot * dot2;
}


// TODO: it's CRAZY how much lighting code we share with the engine. It should all be shared code.
float Engine_WorldLightDistanceFalloff( const dworldlight_t *wl, const Vector& delta )
{
	Assert( wl->type == emit_surface );

	// Cull out stuff that's too far
	if (wl->radius != 0)
	{
		if ( DotProduct( delta, delta ) > (wl->radius * wl->radius))
			return 0.0f;
	}

	return InvRSquared(delta);
}


void AddEmitSurfaceLights( const Vector &vStart, Vector lightBoxColor[6] )
{
	fltx4 fractionVisible;

	FourVectors vStart4, wlOrigin4;
	vStart4.DuplicateVector ( vStart );

	for ( int iLight=0; iLight < *pNumworldlights; iLight++ )
	{
		dworldlight_t *wl = &dworldlights[iLight];

		// Should this light even go in the ambient cubes?
		if ( !( wl->flags & DWL_FLAGS_INAMBIENTCUBE ) )
			continue;

		Assert( wl->type == emit_surface );

		// Can this light see the point?
		wlOrigin4.DuplicateVector ( wl->origin );
		TestLine ( vStart4, wlOrigin4, &fractionVisible );
		if ( !TestSignSIMD ( CmpGtSIMD ( fractionVisible, Four_Zeros ) ) )
			continue;

		// Add this light's contribution.
		Vector vDelta = wl->origin - vStart;
		float flDistanceScale = Engine_WorldLightDistanceFalloff( wl, vDelta );

		Vector vDeltaNorm = vDelta;
		VectorNormalize( vDeltaNorm );
		float flAngleScale = Engine_WorldLightAngle( wl, wl->normal, vDeltaNorm, vDeltaNorm );

		float ratio = flDistanceScale * flAngleScale * SubFloat ( fractionVisible, 0 );
		if ( ratio == 0 )
			continue;

		for ( int i=0; i < 6; i++ )
		{
			float t = DotProduct( g_BoxDirections[i], vDeltaNorm );
			if ( t > 0 )
			{
				lightBoxColor[i] += wl->intensity * (t * ratio);
			}
		}
	}	
}


void ComputeAmbientFromSphericalSamples( int iThread, const Vector &vStart, Vector lightBoxColor[6] )
{
	// Figure out the color that rays hit when shot out from this position.
	Vector radcolor[NUMVERTEXNORMALS];
	float tanTheta = tan(VERTEXNORMAL_CONE_INNER_ANGLE);

	for ( int i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		Vector vEnd = vStart + g_anorms[i] * (COORD_EXTENT * 1.74);

		// Now that we've got a ray, see what surface we've hit
		Vector lightStyleColors[MAX_LIGHTSTYLES];
		lightStyleColors[0].Init();	// We only care about light style 0 here.
		CalcRayAmbientLighting( iThread, vStart, vEnd, tanTheta, lightStyleColors );
	
		radcolor[i] = lightStyleColors[0];
	}

	// accumulate samples into radiant box
	for ( int j = 6; --j >= 0; )
	{
		float t = 0;

		lightBoxColor[j].Init();

		for (int i = 0; i < NUMVERTEXNORMALS; i++)
		{
			float c = DotProduct( g_anorms[i], g_BoxDirections[j] );
			if (c > 0)
			{
				t += c;
				lightBoxColor[j] += radcolor[i] * c;
			}
		}
		
		lightBoxColor[j] *= 1/t;
	}

	// Now add direct light from the emit_surface lights. These go in the ambient cube because
	// there are a ton of them and they are often so dim that they get filtered out by r_worldlightmin.
	AddEmitSurfaceLights( vStart, lightBoxColor );
}


bool IsLeafAmbientSurfaceLight( dworldlight_t *wl )
{
	static const float g_flWorldLightMinEmitSurface = 0.005f;
	static const float g_flWorldLightMinEmitSurfaceDistanceRatio = ( InvRSquared( Vector( 0, 0, 512 ) ) );

	if ( wl->type != emit_surface )
		return false;

	if ( wl->style != 0 )
		return false;

	float intensity = max( wl->intensity[0], wl->intensity[1] );
	intensity = max( intensity, wl->intensity[2] );
	
	return (intensity * g_flWorldLightMinEmitSurfaceDistanceRatio) < g_flWorldLightMinEmitSurface;
}


class CLeafSampler
{
public:
	CLeafSampler( int iThread ) : m_iThread(iThread) {}

	// Generate a random point in the leaf's bounding volume
	// reject any points that aren't actually in the leaf
	// do a couple of tracing heuristics to eliminate points that are inside detail brushes 
	// or underneath displacement surfaces in the leaf
	// return once we have a valid point, use the center if one can't be computed quickly
	void GenerateLeafSamplePosition( int leafIndex, const CUtlVector<dplane_t> &leafPlanes, Vector &samplePosition )
	{
		dleaf_t *pLeaf = dleafs + leafIndex;

		float dx = pLeaf->maxs[0] - pLeaf->mins[0];
		float dy = pLeaf->maxs[1] - pLeaf->mins[1];
		float dz = pLeaf->maxs[2] - pLeaf->mins[2];
		bool bValid = false;
		for ( int i = 0; i < 1000 && !bValid; i++ )
		{
			samplePosition.x = pLeaf->mins[0] + m_random.RandomFloat(0, dx);
			samplePosition.y = pLeaf->mins[1] + m_random.RandomFloat(0, dy);
			samplePosition.z = pLeaf->mins[2] + m_random.RandomFloat(0, dz);
			bValid = true;

			for ( int j = leafPlanes.Count(); --j >= 0 && bValid; )
			{
				float d = DotProduct(leafPlanes[j].normal, samplePosition) - leafPlanes[j].dist;
				if ( d < DIST_EPSILON )
				{
					// not inside the leaf, try again 
					bValid = false;
					break;
				}
			}
			if ( !bValid )
				continue;

			for ( int j = 0; j < 6; j++ )
			{
				Vector start = samplePosition;
				int axis = j%3;
				start[axis] = (j<3) ? pLeaf->mins[axis] : pLeaf->maxs[axis];
				float t;
				Vector normal;
				CastRayInLeaf( m_iThread, samplePosition, start, leafIndex, &t, &normal );
				if ( t == 0.0f )
				{
					// inside a func_detail, try again.
					bValid = false;
					break;
				}
				if ( t != 1.0f )
				{
					Vector delta = start - samplePosition;
					if ( DotProduct(delta, normal) > 0 )
					{
						// hit backside of displacement, try again.
						bValid = false;
						break;
					}
				}
			}
		}
		if ( !bValid )
		{
			// didn't generate a valid sample point, just use the center of the leaf bbox
			samplePosition = ( Vector( pLeaf->mins[0], pLeaf->mins[1], pLeaf->mins[2] ) + Vector( pLeaf->maxs[0], pLeaf->maxs[1], pLeaf->maxs[2] ) ) * 0.5f;
		}
	}

private:
	int		m_iThread;
	CUniformRandomStream m_random;
};

// gets a list of the planes pointing into a leaf
void GetLeafBoundaryPlanes( CUtlVector<dplane_t> &list, int leafIndex )
{
	list.RemoveAll();
	int nodeIndex = leafparents[leafIndex];
	int child = -(leafIndex + 1);
	while ( nodeIndex >= 0 )
	{
		dnode_t *pNode = dnodes + nodeIndex;
		dplane_t *pNodePlane = dplanes + pNode->planenum;
		if ( pNode->children[0] == child )
		{
			// front side
			list.AddToTail( *pNodePlane );
		}
		else
		{
			// back side
			int plane = list.AddToTail();
			list[plane].dist = -pNodePlane->dist;
			list[plane].normal = -pNodePlane->normal;
			list[plane].type = pNodePlane->type;
		}
		child = nodeIndex;
		nodeIndex = nodeparents[child];
	}
}

// this stores each sample of the ambient lighting
struct ambientsample_t
{
	Vector pos;
	Vector cube[6];
};

// add the sample to the list.  If we exceed the maximum number of samples, the worst sample will
// be discarded.  This has the effect of converging on the best samples when enough are added.
void AddSampleToList( CUtlVector<ambientsample_t> &list, const Vector &samplePosition, Vector *pCube )
{
	const int MAX_SAMPLES = 16;

	int index = list.AddToTail();
	list[index].pos = samplePosition;
	for ( int i = 0; i < 6; i++ )
	{
		list[index].cube[i] = pCube[i];
	}

	if ( list.Count() <= MAX_SAMPLES )
		return;

	int nearestNeighborIndex = 0;
	float nearestNeighborDist = FLT_MAX;
	float nearestNeighborTotal = 0;
	for ( int i = 0; i < list.Count(); i++ )
	{
		int closestIndex = 0;
		float closestDist = FLT_MAX;
		float totalDC = 0;
		for ( int j = 0; j < list.Count(); j++ )
		{
			if ( j == i )
				continue;
			float dist = (list[i].pos - list[j].pos).Length();
			float maxDC = 0;
			for ( int k = 0; k < 6; k++ )
			{
				// color delta is computed per-component, per cube side
				for (int s = 0; s < 3; s++ )
				{
					float dc = fabs(list[i].cube[k][s] - list[j].cube[k][s]);
					maxDC = max(maxDC,dc);
				}
				totalDC += maxDC;
			}
			// need a measurable difference in color or we'll just rely on position
			if ( maxDC < 1e-4f )
			{
				maxDC = 0;
			}
			else if ( maxDC > 1.0f )
			{
				maxDC = 1.0f;
			}
			// selection criteria is 10% distance, 90% color difference
			// choose samples that fill the space (large distance from each other)
			// and have largest color variation
			float distanceFactor = 0.1f + (maxDC * 0.9f);
			dist *= distanceFactor;

			// find the "closest" sample to this one
			if ( dist < closestDist )
			{
				closestDist = dist;
				closestIndex = j;
			}
		}
		// the sample with the "closest" neighbor is rejected
		if ( closestDist < nearestNeighborDist || (closestDist == nearestNeighborDist && totalDC < nearestNeighborTotal) )
		{
			nearestNeighborDist = closestDist;
			nearestNeighborIndex = i;
		}
	}
	list.FastRemove( nearestNeighborIndex );
}

// max number of units in gamma space of per-side delta
int CubeDeltaGammaSpace( Vector *pCube0, Vector *pCube1 )
{
	int maxDelta = 0;
	// do this comparison in gamma space to try and get a perceptual basis for the compare
	for ( int i = 0; i < 6; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			int val0 = LinearToScreenGamma( pCube0[i][j] );
			int val1 = LinearToScreenGamma( pCube1[i][j] );
			int delta = abs(val0-val1);
			if ( delta > maxDelta )
				maxDelta = delta;
		}
	}
	return maxDelta;
}
// reconstruct the ambient lighting for a leaf at the given position in worldspace
// optionally skip one of the entries in the list
void Mod_LeafAmbientColorAtPos( Vector *pOut, const Vector &pos, const CUtlVector<ambientsample_t> &list, int skipIndex )
{
	for ( int i = 0; i < 6; i++ )
	{
		pOut[i].Init();
	}
	float totalFactor = 0;
	for ( int i = 0; i < list.Count(); i++ )
	{
		if ( i == skipIndex )
			continue;
		// do an inverse squared distance weighted average of the samples to reconstruct 
		// the original function
		float dist = (list[i].pos - pos).LengthSqr();
		float factor = 1.0f / (dist + 1.0f);
		totalFactor += factor;
		for ( int j = 0; j < 6; j++ )
		{
			pOut[j] += list[i].cube[j] * factor;
		}
	}
	for ( int i = 0; i < 6; i++ )
	{
		pOut[i] *= (1.0f / totalFactor);
	}
}

// this samples the lighting at each sample and removes any unnecessary samples
void CompressAmbientSampleList( CUtlVector<ambientsample_t> &list )
{
	Vector testCube[6];
	for ( int i = 0; i < list.Count(); i++ )
	{
		if ( list.Count() > 1 )
		{
			Mod_LeafAmbientColorAtPos( testCube, list[i].pos, list, i );
			if ( CubeDeltaGammaSpace(testCube, list[i].cube) < 3 )
			{
				list.FastRemove(i);
				i--;
			}
		}
	}
}

// basically this is an intersection routine that returns a distance between the boxes
float AABBDistance( const Vector &mins0, const Vector &maxs0, const Vector &mins1, const Vector &maxs1 )
{
	Vector delta;
	for ( int i = 0; i < 3; i++ )
	{
		float greatestMin = max(mins0[i], mins1[i]);
		float leastMax = min(maxs0[i], maxs1[i]);
		delta[i] = (greatestMin < leastMax) ? 0 : (leastMax - greatestMin);
	}
	return delta.Length();
}

// build a list of leaves from a query
class CLeafList : public ISpatialLeafEnumerator
{
public:
	virtual bool EnumerateLeaf( int leaf, intp context )
	{
		m_list.AddToTail(leaf);
		return true;
	}

	CUtlVector<int> m_list;
};

// conver short[3] to vector
static void LeafBounds( int leafIndex, Vector &mins, Vector &maxs )
{
	for ( int i = 0; i < 3; i++ )
	{
		mins[i] = dleafs[leafIndex].mins[i];
		maxs[i] = dleafs[leafIndex].maxs[i];
	}
}

// returns the index of the nearest leaf with ambient samples
int NearestNeighborWithLight(int leafID)
{
	Vector mins, maxs;
	LeafBounds( leafID, mins, maxs );
	Vector size = maxs - mins;
	CLeafList leafList;
	ToolBSPTree()->EnumerateLeavesInBox( mins-size, maxs+size, &leafList, 0 );
	float bestDist = FLT_MAX;
	int bestIndex = leafID;
	for ( int i = 0; i < leafList.m_list.Count(); i++ )
	{
		int testIndex = leafList.m_list[i];
		if ( !g_pLeafAmbientIndex->Element(testIndex).ambientSampleCount )
			continue;

		Vector testMins, testMaxs;
		LeafBounds( testIndex, testMins, testMaxs );
		float dist = AABBDistance( mins, maxs, testMins, testMaxs );
		if ( dist < bestDist )
		{
			bestDist = dist;
			bestIndex = testIndex;
		}
	}
	return bestIndex;
}

// maps a float to a byte fraction between min & max
static byte Fixed8Fraction( float t, float tMin, float tMax )
{
	if ( tMax <= tMin )
		return 0;

	float frac = RemapValClamped( t, tMin, tMax, 0.0f, 255.0f );
	return byte(frac+0.5f);
}

CUtlVector< CUtlVector<ambientsample_t> > g_LeafAmbientSamples;

void ComputeAmbientForLeaf( int iThread, int leafID, CUtlVector<ambientsample_t> &list )
{
	CUtlVector<dplane_t> leafPlanes;
	CLeafSampler sampler( iThread );

	GetLeafBoundaryPlanes( leafPlanes, leafID );
	list.RemoveAll();
	// this heuristic tries to generate at least one sample per volume (chosen to be similar to the size of a player) in the space
	int xSize = (dleafs[leafID].maxs[0] - dleafs[leafID].mins[0]) / 32;
	int ySize = (dleafs[leafID].maxs[1] - dleafs[leafID].mins[1]) / 32;
	int zSize = (dleafs[leafID].maxs[2] - dleafs[leafID].mins[2]) / 64;
	xSize = max(xSize,1);
	ySize = max(xSize,1);
	zSize = max(xSize,1);
	// generate update 128 candidate samples, always at least one sample
	int volumeCount = xSize * ySize * zSize;
	if ( g_bFastAmbient )
	{
		// save compute time, only do one sample
		volumeCount = 1;
	}
	int sampleCount = clamp( volumeCount, 1, 128 );
	if ( dleafs[leafID].contents & CONTENTS_SOLID )
	{
		// don't generate any samples in solid leaves
		// NOTE: We copy the nearest non-solid leaf sample pointers into this leaf at the end
		return;
	}
	Vector cube[6];
	for ( int i = 0; i < sampleCount; i++ )
	{
		// compute each candidate sample and add to the list
		Vector samplePosition;
		sampler.GenerateLeafSamplePosition( leafID, leafPlanes, samplePosition );
		ComputeAmbientFromSphericalSamples( iThread, samplePosition, cube );
		// note this will remove the least valuable sample once the limit is reached
		AddSampleToList( list, samplePosition, cube );
	}

	// remove any samples that can be reconstructed with the remaining data
	CompressAmbientSampleList( list );
}

static void ThreadComputeLeafAmbient( int iThread, void *pUserData )
{
	CUtlVector<ambientsample_t> list;
	while (1)
	{
		int leafID = GetThreadWork ();
		if (leafID == -1)
			break;
		list.RemoveAll();
		ComputeAmbientForLeaf(iThread, leafID, list);
		// copy to the output array
		g_LeafAmbientSamples[leafID].SetCount( list.Count() );
		for ( int i = 0; i < list.Count(); i++ )
		{
			g_LeafAmbientSamples[leafID].Element(i) = list.Element(i);
		}
	}
}

#ifdef MPI
void VMPI_ProcessLeafAmbient( int iThread, uint64 iLeaf, MessageBuffer *pBuf )
{
	CUtlVector<ambientsample_t> list;
	ComputeAmbientForLeaf(iThread, (int)iLeaf, list);

	VMPI_SetCurrentStage( "EncodeLeafAmbientResults" );

	// Encode the results.
	int nSamples = list.Count();
	pBuf->write( &nSamples, sizeof( nSamples ) );
	if ( nSamples )
	{
		pBuf->write( list.Base(), list.Count() * sizeof( ambientsample_t ) );
	}
}

//-----------------------------------------------------------------------------
// Called on the master when a worker finishes processing a static prop.
//-----------------------------------------------------------------------------
void VMPI_ReceiveLeafAmbientResults( uint64 leafID, MessageBuffer *pBuf, int iWorker )
{
	// Decode the results.
	int nSamples;
	pBuf->read( &nSamples, sizeof( nSamples ) );

	g_LeafAmbientSamples[leafID].SetCount( nSamples );
	if ( nSamples )
	{
		pBuf->read(g_LeafAmbientSamples[leafID].Base(), nSamples * sizeof(ambientsample_t) );
	}
}
#endif

void ComputePerLeafAmbientLighting()
{
	// Figure out which lights should go in the per-leaf ambient cubes.
	int nInAmbientCube = 0;
	int nSurfaceLights = 0;
	for ( int i=0; i < *pNumworldlights; i++ )
	{
		dworldlight_t *wl = &dworldlights[i];
		
		if ( IsLeafAmbientSurfaceLight( wl ) )
			wl->flags |= DWL_FLAGS_INAMBIENTCUBE;
		else
			wl->flags &= ~DWL_FLAGS_INAMBIENTCUBE;
	
		if ( wl->type == emit_surface )
			++nSurfaceLights;

		if ( wl->flags & DWL_FLAGS_INAMBIENTCUBE )
			++nInAmbientCube;
	}

	Msg( "%d of %d (%d%% of) surface lights went in leaf ambient cubes.\n", nInAmbientCube, nSurfaceLights, nSurfaceLights ? ((nInAmbientCube*100) / nSurfaceLights) : 0 );

	g_LeafAmbientSamples.SetCount(numleafs);

#ifdef MPI
	if ( g_bUseMPI )
	{
		// Distribute the work among the workers.
		VMPI_SetCurrentStage( "ComputeLeafAmbientLighting" );
		DistributeWork( numleafs, VMPI_ProcessLeafAmbient, VMPI_ReceiveLeafAmbientResults );
	}
	else
#endif
	{
		RunThreadsOn(numleafs, true, ThreadComputeLeafAmbient);
	}

	// now write out the data
	Msg("Writing leaf ambient...");
	g_pLeafAmbientIndex->RemoveAll();
	g_pLeafAmbientLighting->RemoveAll();
	g_pLeafAmbientIndex->SetCount( numleafs );
	g_pLeafAmbientLighting->EnsureCapacity( numleafs*4 );
	for ( int leafID = 0; leafID < numleafs; leafID++ )
	{
		const CUtlVector<ambientsample_t> &list = g_LeafAmbientSamples[leafID];
		g_pLeafAmbientIndex->Element(leafID).ambientSampleCount = list.Count();
		if ( !list.Count() )
		{
			g_pLeafAmbientIndex->Element(leafID).firstAmbientSample = 0;
		}
		else
		{
			g_pLeafAmbientIndex->Element(leafID).firstAmbientSample = g_pLeafAmbientLighting->Count();
			// compute the samples in disk format.  Encode the positions in 8-bits using leaf bounds fractions
			for ( int i = 0; i < list.Count(); i++ )
			{
				int outIndex = g_pLeafAmbientLighting->AddToTail();
				dleafambientlighting_t &light = g_pLeafAmbientLighting->Element(outIndex);

				light.x = Fixed8Fraction( list[i].pos.x, dleafs[leafID].mins[0], dleafs[leafID].maxs[0] );
				light.y = Fixed8Fraction( list[i].pos.y, dleafs[leafID].mins[1], dleafs[leafID].maxs[1] );
				light.z = Fixed8Fraction( list[i].pos.z, dleafs[leafID].mins[2], dleafs[leafID].maxs[2] );
				light.pad = 0;
				for ( int side = 0; side < 6; side++ )
				{
					VectorToColorRGBExp32( list[i].cube[side], light.cube.m_Color[side] );
				}
			}
		}
	}
	for ( int i = 0; i < numleafs; i++ )
	{
		// UNDONE: Do this dynamically in the engine instead.  This will allow us to sample across leaf
		// boundaries always which should improve the quality of lighting in general
		if ( g_pLeafAmbientIndex->Element(i).ambientSampleCount == 0 )
		{
			if ( !(dleafs[i].contents & CONTENTS_SOLID) )
			{
				Msg("Bad leaf ambient for leaf %d\n", i );
			}

			int refLeaf = NearestNeighborWithLight(i);
			g_pLeafAmbientIndex->Element(i).ambientSampleCount = 0;
			g_pLeafAmbientIndex->Element(i).firstAmbientSample = refLeaf;
		}
	}
	Msg("done\n");
}

