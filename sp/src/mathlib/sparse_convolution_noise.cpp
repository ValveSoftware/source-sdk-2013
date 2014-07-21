//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: noise() primitives.
//
//=====================================================================================//

#include <math.h>
#include "basetypes.h"
#include <memory.h>
#include "tier0/dbg.h"
#include "mathlib/mathlib.h"
#include "mathlib/vector.h"
#include "mathlib/noise.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// generate high quality noise based upon "sparse convolution". HIgher quality than perlin noise,
// and no direcitonal artifacts.

#include "noisedata.h"

#define N_IMPULSES_PER_CELL 5
#define NORMALIZING_FACTOR 1.0

//(0.5/N_IMPULSES_PER_CELL)

static inline int LatticeCoord(float x)
{
	return ((int) floor(x)) & 0xff;
}

static inline int Hash4D(int ix, int iy, int iz, int idx)
{
	int ret=perm_a[ix];
	ret=perm_b[(ret+iy) & 0xff];
	ret=perm_c[(ret+iz) & 0xff];
	ret=perm_d[(ret+idx) & 0xff];
	return ret;
}

#define SQ(x) ((x)*(x))

static float CellNoise( int ix, int iy, int iz, float xfrac, float yfrac, float zfrac,
						float (*pNoiseShapeFunction)(float) )
{
	float ret=0;
	for(int idx=0;idx<N_IMPULSES_PER_CELL;idx++)
	{
		int coord_idx=Hash4D( ix, iy, iz, idx );
		float dsq=SQ(impulse_xcoords[coord_idx]-xfrac)+
			SQ(impulse_ycoords[coord_idx]-yfrac)+
			SQ(impulse_zcoords[coord_idx]-zfrac);
		dsq = sqrt( dsq );
		if (dsq < 1.0 )
		{
			ret += (*pNoiseShapeFunction)( 1-dsq );
		}
	}
	return ret;
}


float SparseConvolutionNoise( Vector const &pnt )
{
	return SparseConvolutionNoise( pnt, QuinticInterpolatingPolynomial );
}

float FractalNoise( Vector const &pnt, int n_octaves)
{
	float scale=1.0;
	float iscale=1.0;
	float ret=0;
	float sumscale=0;
	for(int o=0;o<n_octaves;o++)
	{
		Vector p1=pnt;
		p1 *= scale;
		ret+=iscale * SparseConvolutionNoise( p1 );
		sumscale += iscale;
		scale *= 2.0;
		iscale *= 0.5;
	}
	return ret * ( 1.0/sumscale );
}

float Turbulence( Vector const &pnt, int n_octaves)
{
	float scale=1.0;
	float iscale=1.0;
	float ret=0;
	float sumscale=0;
	for(int o=0;o<n_octaves;o++)
	{
		Vector p1=pnt;
		p1 *= scale;
		ret+=iscale * fabs ( 2.0*( SparseConvolutionNoise( p1 )-.5 ) );
		sumscale += iscale;
		scale *= 2.0;
		iscale *= 0.5;
	}
	return ret * ( 1.0/sumscale );
}

#ifdef MEASURE_RANGE
float fmin1=10000000.0;
float fmax1=-1000000.0;
#endif

float SparseConvolutionNoise(Vector const &pnt, float (*pNoiseShapeFunction)(float) )
{
	// computer integer lattice point
	int ix=LatticeCoord(pnt.x);
	int iy=LatticeCoord(pnt.y);
	int iz=LatticeCoord(pnt.z);

	// compute offsets within unit cube
	float xfrac=pnt.x-floor(pnt.x);
	float yfrac=pnt.y-floor(pnt.y);
	float zfrac=pnt.z-floor(pnt.z);

	float sum_out=0.;

	for(int ox=-1; ox<=1; ox++)
		for(int oy=-1; oy<=1; oy++)
			for(int oz=-1; oz<=1; oz++)
			{
				sum_out += CellNoise( ix+ox, iy+oy, iz+oz,
									  xfrac-ox, yfrac-oy, zfrac-oz,
									  pNoiseShapeFunction );
			}
#ifdef MEASURE_RANGE
	fmin1=min(sum_out,fmin1);
	fmax1=max(sum_out,fmax1);
#endif
	return RemapValClamped( sum_out, .544487, 9.219176, 0.0, 1.0 );
}


// Improved Perlin Noise
// The following code is the c-ification of Ken Perlin's new noise algorithm
// "JAVA REFERENCE IMPLEMENTATION OF IMPROVED NOISE - COPYRIGHT 2002 KEN PERLIN"
// as available here: http://mrl.nyu.edu/~perlin/noise/

float NoiseGradient(int hash, float x, float y, float z)
{
	int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
	float u = h<8 ? x : y;                  // INTO 12 GRADIENT DIRECTIONS.
	float v = h<4 ? y : (h==12||h==14 ? x : z);
	return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

int NoiseHashIndex( int i )
{
	static int s_permutation[] = 
	{
		151,160,137,91,90,15,
			131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
			190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
			88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
			77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
			102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
			135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
			5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
			223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
			129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
			251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
			49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
			138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};

	return s_permutation[ i & 0xff ];
}

float ImprovedPerlinNoise( Vector const &pnt )
{
	float fx = floor(pnt.x);
	float fy = floor(pnt.y);
	float fz = floor(pnt.z);

	int X = (int)fx & 255;								// FIND UNIT CUBE THAT
	int Y = (int)fy & 255;								// CONTAINS POINT.
	int Z = (int)fz & 255;

	float x = pnt.x - fx;								// FIND RELATIVE X,Y,Z
	float y = pnt.y - fy;								// OF POINT IN CUBE.
	float z = pnt.z - fz;

	float u = QuinticInterpolatingPolynomial(x);		// COMPUTE FADE CURVES
	float v = QuinticInterpolatingPolynomial(y);		// FOR EACH OF X,Y,Z.
	float w = QuinticInterpolatingPolynomial(z);

	int A  = NoiseHashIndex( X ) + Y;					// HASH COORDINATES OF
	int AA = NoiseHashIndex( A ) + Z;					// THE 8 CUBE CORNERS,
	int AB = NoiseHashIndex( A + 1 ) + Z;
	int B  = NoiseHashIndex( X + 1 ) + Y;
	int BA = NoiseHashIndex( B ) + Z;
	int BB = NoiseHashIndex( B + 1 ) + Z;

	float g0 = NoiseGradient(NoiseHashIndex(AA  ), x  , y  , z   );
	float g1 = NoiseGradient(NoiseHashIndex(BA  ), x-1, y  , z   );
	float g2 = NoiseGradient(NoiseHashIndex(AB  ), x  , y-1, z   );
	float g3 = NoiseGradient(NoiseHashIndex(BB  ), x-1, y-1, z   );
	float g4 = NoiseGradient(NoiseHashIndex(AA+1), x  , y  , z-1 );
	float g5 = NoiseGradient(NoiseHashIndex(BA+1), x-1, y  , z-1 );
	float g6 = NoiseGradient(NoiseHashIndex(AB+1), x  , y-1, z-1 );
	float g7 = NoiseGradient(NoiseHashIndex(BB+1), x-1, y-1, z-1 );

	// AND ADD BLENDED RESULTS FROM 8 CORNERS OF CUBE
	float g01 = Lerp( u, g0, g1 );
	float g23 = Lerp( u, g2, g3 );
	float g45 = Lerp( u, g4, g5 );
	float g67 = Lerp( u, g6, g7 );
	float g0123 = Lerp( v, g01, g23 );
	float g4567 = Lerp( v, g45, g67 );

	return Lerp( w, g0123,g4567 );
}
