//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "vrad.h"
#include "lightmap.h"
#include "radial.h"
#include "mathlib/bumpvects.h"
#include "utlrbtree.h"
#include "mathlib/VMatrix.h"
#include "macro_texture.h"


void WorldToLuxelSpace( lightinfo_t const *l, Vector const &world, Vector2D &coord )
{
	Vector pos;

	VectorSubtract( world, l->luxelOrigin, pos );
	coord[0] = DotProduct( pos, l->worldToLuxelSpace[0] ) - l->face->m_LightmapTextureMinsInLuxels[0];
	coord[1] = DotProduct( pos, l->worldToLuxelSpace[1] ) - l->face->m_LightmapTextureMinsInLuxels[1];
}

void LuxelSpaceToWorld( lightinfo_t const *l, float s, float t, Vector &world )
{
	Vector pos;

	s += l->face->m_LightmapTextureMinsInLuxels[0];
	t += l->face->m_LightmapTextureMinsInLuxels[1];
	VectorMA( l->luxelOrigin, s, l->luxelToWorldSpace[0], pos );
	VectorMA( pos, t, l->luxelToWorldSpace[1], world );
}

void WorldToLuxelSpace( lightinfo_t const *l, FourVectors const &world, FourVectors &coord )
{
	FourVectors luxelOrigin;
	luxelOrigin.DuplicateVector ( l->luxelOrigin );

	FourVectors pos = world;
	pos -= luxelOrigin;

	coord.x = pos * l->worldToLuxelSpace[0];
	coord.x = SubSIMD ( coord.x, ReplicateX4 ( l->face->m_LightmapTextureMinsInLuxels[0] ) );
	coord.y = pos * l->worldToLuxelSpace[1];
	coord.y = SubSIMD ( coord.y, ReplicateX4 ( l->face->m_LightmapTextureMinsInLuxels[1] ) );
	coord.z = Four_Zeros;
}

void LuxelSpaceToWorld( lightinfo_t const *l, fltx4 s, fltx4 t, FourVectors &world )
{
	world.DuplicateVector ( l->luxelOrigin );
	FourVectors st;

	s = AddSIMD ( s, ReplicateX4 ( l->face->m_LightmapTextureMinsInLuxels[0] ) );
	st.DuplicateVector ( l->luxelToWorldSpace[0] );
	st *= s;
	world += st;

	t = AddSIMD ( t, ReplicateX4 ( l->face->m_LightmapTextureMinsInLuxels[1] ) );
	st.DuplicateVector ( l->luxelToWorldSpace[1] );
	st *= t;
	world += st;
}



void AddDirectToRadial( radial_t *rad, 
						Vector const &pnt, 
						Vector2D const &coordmins, Vector2D const &coordmaxs, 
						LightingValue_t const light[NUM_BUMP_VECTS+1],
						bool hasBumpmap, bool neighborHasBumpmap  )
{
	int     s_min, s_max, t_min, t_max;
	Vector2D  coord;
	int	    s, t;
	float   ds, dt;
	float   r;
	float	area;
	int		bumpSample;

	// convert world pos into local lightmap texture coord
	WorldToLuxelSpace( &rad->l, pnt, coord );

	s_min = ( int )( coordmins[0] );
	t_min = ( int )( coordmins[1] );
	s_max = ( int )( coordmaxs[0] + 0.9999f ) + 1; // ????
	t_max = ( int )( coordmaxs[1] + 0.9999f ) + 1;

	s_min = max( s_min, 0 );
	t_min = max( t_min, 0 );
	s_max = min( s_max, rad->w );
	t_max = min( t_max, rad->h );

	for( s = s_min; s < s_max; s++ )
	{
		for( t = t_min; t < t_max; t++ )
		{
			float s0 = max( coordmins[0] - s, -1.0 );
			float t0 = max( coordmins[1] - t, -1.0 );
			float s1 = min( coordmaxs[0] - s, 1.0 );
			float t1 = min( coordmaxs[1] - t, 1.0 );

			area = (s1 - s0) * (t1 - t0);

			if (area > EQUAL_EPSILON)
			{
				ds = fabs( coord[0] - s );
				dt = fabs( coord[1] - t );

				r = max( ds, dt );

				if (r < 0.1)
				{
					r = area / 0.1;
				}
				else
				{
					r = area / r;
				}

				int i = s+t*rad->w;

				if( hasBumpmap )
				{
					if( neighborHasBumpmap )
					{
						for( bumpSample = 0; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
						{
							rad->light[bumpSample][i].AddWeighted( light[bumpSample], r );
						}
					}
					else
					{
						rad->light[0][i].AddWeighted(light[0],r );
						for( bumpSample = 1; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
						{
							rad->light[bumpSample][i].AddWeighted( light[0], r * OO_SQRT_3 );
						}
					}
				}
				else
				{
					rad->light[0][i].AddWeighted( light[0], r );
				}
				
				rad->weight[i] += r;
			}
		}
	}
}



void AddBouncedToRadial( radial_t *rad, 
						 Vector const &pnt, 
						 Vector2D const &coordmins, Vector2D const &coordmaxs, 
						 Vector const light[NUM_BUMP_VECTS+1],
						 bool hasBumpmap, bool neighborHasBumpmap  )
{
	int     s_min, s_max, t_min, t_max;
	Vector2D  coord;
	int	    s, t;
	float   ds, dt;
	float   r;
	int		bumpSample;

	// convert world pos into local lightmap texture coord
	WorldToLuxelSpace( &rad->l, pnt, coord );

	float dists, distt;

	dists = (coordmaxs[0] - coordmins[0]);
	distt = (coordmaxs[1] - coordmins[1]);

	// patches less than a luxel in size could be mistakeningly filtered, so clamp.
	dists = max( 1.0, dists );
	distt = max( 1.0, distt );

	// find possible domain of patch influence
  	s_min = ( int )( coord[0] - dists * RADIALDIST );
  	t_min = ( int )( coord[1] - distt * RADIALDIST );
  	s_max = ( int )( coord[0] + dists * RADIALDIST + 1.0f );
  	t_max = ( int )( coord[1] + distt * RADIALDIST + 1.0f );

	// clamp to valid luxel
	s_min = max( s_min, 0 );
	t_min = max( t_min, 0 );
	s_max = min( s_max, rad->w );
	t_max = min( t_max, rad->h );

	for( s = s_min; s < s_max; s++ )
	{
		for( t = t_min; t < t_max; t++ )
		{
			// patch influence is based on patch size
  			ds = ( coord[0] - s ) / dists;
  			dt = ( coord[1] - t ) / distt;
  
  			r = RADIALDIST2 - (ds * ds + dt * dt);

			int i = s+t*rad->w;
   
  			if (r > 0)
			{
				if( hasBumpmap )
				{
					if( neighborHasBumpmap )
					{
						for( bumpSample = 0; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
						{
							rad->light[bumpSample][i].AddWeighted( light[bumpSample], r );
						}
					}
					else
					{
						rad->light[0][i].AddWeighted( light[0], r );
						for( bumpSample = 1; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
						{
							rad->light[bumpSample][i].AddWeighted( light[0], r * OO_SQRT_3 );
						}
					}
				}
				else
				{
					rad->light[0][i].AddWeighted( light[0], r );
				}
				
				rad->weight[i] += r;
			}
		}
	}
}

void PatchLightmapCoordRange( radial_t *rad, int ndxPatch, Vector2D &mins, Vector2D &maxs )
{
	winding_t	*w;
	int i;
	Vector2D coord;

	mins.Init( 1E30, 1E30 );
	maxs.Init( -1E30, -1E30 );

	CPatch *patch = &g_Patches.Element( ndxPatch );
	w = patch->winding;

	for (i = 0; i < w->numpoints; i++)
	{
		WorldToLuxelSpace( &rad->l, w->p[i], coord );
		mins[0] = min( mins[0], coord[0] );
		maxs[0] = max( maxs[0], coord[0] );
		mins[1] = min( mins[1], coord[1] );
		maxs[1] = max( maxs[1], coord[1] );
	}
}

radial_t *AllocateRadial( int facenum )
{
	radial_t *rad;

	rad = ( radial_t* )calloc( 1, sizeof( *rad ) );

	rad->facenum = facenum;
	InitLightinfo( &rad->l, facenum );

	rad->w = rad->l.face->m_LightmapTextureSizeInLuxels[0]+1;
	rad->h = rad->l.face->m_LightmapTextureSizeInLuxels[1]+1;

	return rad;
}

void FreeRadial( radial_t *rad )
{
	if (rad)
		free( rad );
}


radial_t *BuildPatchRadial( int facenum )
{
	int             j;
	radial_t        *rad;
	CPatch	        *patch;
	faceneighbor_t  *fn;
	Vector2D        mins, maxs;
	bool			needsBumpmap, neighborNeedsBumpmap;

	needsBumpmap = texinfo[g_pFaces[facenum].texinfo].flags & SURF_BUMPLIGHT ? true : false;

	rad = AllocateRadial( facenum );
	
	fn = &faceneighbor[ rad->facenum ];

	CPatch *pNextPatch;

	if( g_FacePatches.Element( rad->facenum ) != g_FacePatches.InvalidIndex() )
	{
		for( patch = &g_Patches.Element( g_FacePatches.Element( rad->facenum ) ); patch; patch = pNextPatch )
		{
			// next patch
			pNextPatch = NULL;
			if( patch->ndxNext != g_Patches.InvalidIndex() )
			{
				pNextPatch = &g_Patches.Element( patch->ndxNext );
			}
			
			// skip patches with children
			if (patch->child1 != g_Patches.InvalidIndex() )
				continue;
			
			// get the range of patch lightmap texture coords
			int ndxPatch = patch - g_Patches.Base();
			PatchLightmapCoordRange( rad, ndxPatch, mins, maxs );
			
			if (patch->numtransfers == 0)
			{
				// Error, using patch that was never evaluated or has no samples
				// patch->totallight[1] = 255;
			}
			
			//
			// displacement surface patch origin position and normal vectors have been changed to
			// represent the displacement surface position and normal -- for radial "blending"
			// we need to get the base surface patch origin!
			//
			if( ValidDispFace( &g_pFaces[facenum] ) )
			{
				Vector patchOrigin;
				WindingCenter (patch->winding, patchOrigin );
				AddBouncedToRadial( rad, patchOrigin, mins, maxs, patch->totallight.light,  
					needsBumpmap, needsBumpmap );			
			}
			else
			{
				AddBouncedToRadial( rad, patch->origin, mins, maxs, patch->totallight.light, 
					needsBumpmap, needsBumpmap );
			}
		}
	}

	for (j=0 ; j<fn->numneighbors; j++)
	{
		if( g_FacePatches.Element( fn->neighbor[j] ) != g_FacePatches.InvalidIndex() )
		{
			for( patch = &g_Patches.Element( g_FacePatches.Element( fn->neighbor[j] ) ); patch; patch = pNextPatch )
			{
				// next patch
				pNextPatch = NULL;
				if( patch->ndxNext != g_Patches.InvalidIndex() )
				{
					pNextPatch = &g_Patches.Element( patch->ndxNext );
				}
				
				// skip patches with children
				if (patch->child1 != g_Patches.InvalidIndex() )
					continue;
				
				// get the range of patch lightmap texture coords
				int ndxPatch = patch - g_Patches.Base();
				PatchLightmapCoordRange( rad, ndxPatch, mins, maxs  );
				
				neighborNeedsBumpmap = texinfo[g_pFaces[facenum].texinfo].flags & SURF_BUMPLIGHT ? true : false;
				
				//
				// displacement surface patch origin position and normal vectors have been changed to
				// represent the displacement surface position and normal -- for radial "blending"
				// we need to get the base surface patch origin!
				//
				if( ValidDispFace( &g_pFaces[fn->neighbor[j]] ) )
				{
					Vector patchOrigin;
					WindingCenter (patch->winding, patchOrigin );
					AddBouncedToRadial( rad, patchOrigin, mins, maxs, patch->totallight.light, 
						needsBumpmap, needsBumpmap );			
				}
				else
				{
					AddBouncedToRadial( rad, patch->origin, mins, maxs, patch->totallight.light,
						needsBumpmap, needsBumpmap );
				}
			}
		}
	}

	return rad;
}


radial_t *BuildLuxelRadial( int facenum, int style )
{
	LightingValue_t light[NUM_BUMP_VECTS + 1];

	facelight_t *fl = &facelight[facenum];
	faceneighbor_t *fn = &faceneighbor[facenum];

	radial_t *rad = AllocateRadial( facenum );

	bool needsBumpmap = texinfo[g_pFaces[facenum].texinfo].flags & SURF_BUMPLIGHT ? true : false;

	for (int k=0 ; k<fl->numsamples ; k++)
	{
		if( needsBumpmap )
		{
			for( int bumpSample = 0; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
			{
				light[bumpSample] = fl->light[style][bumpSample][k];
			}
		}
		else
		{
			light[0] = fl->light[style][0][k];
		}

		AddDirectToRadial( rad, fl->sample[k].pos, fl->sample[k].mins, fl->sample[k].maxs, light, needsBumpmap, needsBumpmap );
	}

	for (int j = 0; j < fn->numneighbors; j++)
	{
		fl = &facelight[fn->neighbor[j]];

		bool neighborHasBumpmap = false;
		
		if( texinfo[g_pFaces[fn->neighbor[j]].texinfo].flags & SURF_BUMPLIGHT )
		{
			neighborHasBumpmap = true;
		}

		int nstyle = 0;

		// look for style that matches
		if (g_pFaces[fn->neighbor[j]].styles[nstyle] != g_pFaces[facenum].styles[style])
		{
			for (nstyle = 1; nstyle < MAXLIGHTMAPS; nstyle++ )
				if ( g_pFaces[fn->neighbor[j]].styles[nstyle] == g_pFaces[facenum].styles[style] )
					break;

			// if not found, skip this neighbor
			if (nstyle >= MAXLIGHTMAPS)
				continue;
		}

		lightinfo_t l;

		InitLightinfo( &l, fn->neighbor[j] );

		for (int k=0 ; k<fl->numsamples ; k++)
		{
			if( neighborHasBumpmap )
			{
				for( int bumpSample = 0; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
				{
					light[bumpSample] = fl->light[nstyle][bumpSample][k];
				}
			}
			else
			{
				light[0]=fl->light[nstyle][0][k];
			}

			Vector tmp;
			Vector2D mins, maxs;

			LuxelSpaceToWorld( &l, fl->sample[k].mins[0], fl->sample[k].mins[1], tmp );
			WorldToLuxelSpace( &rad->l, tmp, mins );
			LuxelSpaceToWorld( &l, fl->sample[k].maxs[0], fl->sample[k].maxs[1], tmp );
			WorldToLuxelSpace( &rad->l, tmp, maxs );

			AddDirectToRadial( rad, fl->sample[k].pos, mins, maxs, light,
				         needsBumpmap, neighborHasBumpmap );
		}
	}

	return rad;
}


//-----------------------------------------------------------------------------
// Purpose: returns the closest light value for a given point on the surface
//			this is normally a 1:1 mapping
//-----------------------------------------------------------------------------
bool SampleRadial( radial_t *rad, Vector& pnt, LightingValue_t light[NUM_BUMP_VECTS + 1], int bumpSampleCount )
{
	int bumpSample;
	Vector2D coord;

	WorldToLuxelSpace( &rad->l, pnt, coord );
	int u = ( int )( coord[0] + 0.5f );
	int v = ( int )( coord[1] + 0.5f );
	int i = u + v * rad->w;

	if (u < 0 || u > rad->w || v < 0 || v > rad->h)
	{
		static bool warning = false;
		if ( !warning )
		{
			// punting over to KenB
			// 2d coord indexes off of lightmap, generation of pnt seems suspect
			Warning( "SampleRadial: Punting, Waiting for fix\n" );
			warning = true;
		}
		for( bumpSample = 0; bumpSample < bumpSampleCount; bumpSample++ )
		{
			light[bumpSample].m_vecLighting.Init( 2550, 0, 0 );
		}
		return false;
	}

	bool baseSampleOk = true;
	for( bumpSample = 0; bumpSample < bumpSampleCount; bumpSample++ )
	{
		light[bumpSample].Zero();

		if (rad->weight[i] > WEIGHT_EPS)
		{
			light[bumpSample]= rad->light[bumpSample][i];
			light[bumpSample].Scale( 1.0 / rad->weight[i] );
		}
		else
		{
			if ( bRed2Black )
			{
				// Error, luxel has no samples
				light[bumpSample].m_vecLighting.Init( 0, 0, 0 );
			}
			else
			{
				// Error, luxel has no samples
				// Yes, it actually should be 2550
				light[bumpSample].m_vecLighting.Init( 2550, 0, 0 );
			}

			if (bumpSample == 0)
				baseSampleOk = false;
		}
	}

	return baseSampleOk;
}

bool FloatLess( float const& src1, float const& src2 )
{
	return src1 < src2;
}


//-----------------------------------------------------------------------------
// Debugging!
//-----------------------------------------------------------------------------
void GetRandomColor( unsigned char *color )
{
	static bool firstTime = true;
				
	if( firstTime )
	{
		firstTime = false;
		srand( 0 );
	}
	
	color[0] = ( unsigned char )( rand() * ( 255.0f / VALVE_RAND_MAX ) ); 
	color[1] = ( unsigned char )( rand() * ( 255.0f / VALVE_RAND_MAX ) ); 
	color[2] = ( unsigned char )( rand() * ( 255.0f / VALVE_RAND_MAX ) ); 
}


#if 0
// debugging! -- not accurate!
void DumpLuxels( facelight_t *pFaceLight, Vector *luxelColors, int ndxFace )
{
	static FileHandle_t pFpLuxels = NULL;

	ThreadLock();

	if( !pFpLuxels )
	{
		pFpLuxels = g_pFileSystem->Open( "luxels.txt", "w" );
	}

	dface_t *pFace = &g_pFaces[ndxFace];
	bool bDisp = ( pFace->dispinfo != -1 );

	for( int ndx = 0; ndx < pFaceLight->numluxels; ndx++ )
	{
		WriteWinding( pFpLuxels, pFaceLight->sample[ndx].w, luxelColors[ndx] );
		if( bDumpNormals && bDisp )
		{
			WriteNormal( pFpLuxels, pFaceLight->luxel[ndx], pFaceLight->luxelNormals[ndx], 15.0f, Vector( 255, 255, 0 ) );
		}
	}

	ThreadUnlock();
}
#endif


static FileHandle_t pFileLuxels[4] = { NULL, NULL, NULL, NULL };

void DumpDispLuxels( int iFace, Vector &color, int iLuxel, int nBump )
{
	// Lock the thread and dump the luxel data.
	ThreadLock();

	// Get the face and facelight data.
	facelight_t *pFaceLight = &facelight[iFace];

	// Open the luxel files.
	char szFileName[512];
	for ( int iBump = 0; iBump < ( NUM_BUMP_VECTS+1 ); ++iBump )
	{
		if ( pFileLuxels[iBump] == NULL )
		{
			sprintf( szFileName, "luxels_bump%d.txt", iBump );
			pFileLuxels[iBump] = g_pFileSystem->Open( szFileName, "w" );
		}
	}

	WriteWinding( pFileLuxels[nBump], pFaceLight->sample[iLuxel].w, color );

	ThreadUnlock();
}

void CloseDispLuxels()
{
	for ( int iBump = 0; iBump < ( NUM_BUMP_VECTS+1 ); ++iBump )
	{
		if ( pFileLuxels[iBump] )
		{
			g_pFileSystem->Close( pFileLuxels[iBump] );
		}
	}
}

/*
=============
FinalLightFace

Add the indirect lighting on top of the direct
lighting and save into final map format
=============
*/
void FinalLightFace( int iThread, int facenum )
{
	dface_t	        *f;
	int		        i, j, k;
	facelight_t	    *fl;
	float		    minlight;
	int			    lightstyles;
	LightingValue_t lb[NUM_BUMP_VECTS + 1], v[NUM_BUMP_VECTS + 1];
	unsigned char   *pdata[NUM_BUMP_VECTS + 1];
	int				bumpSample;
	radial_t	    *rad = NULL;
	radial_t	    *prad = NULL;

   	f = &g_pFaces[facenum];

    // test for non-lit texture
    if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
        return;		

	fl = &facelight[facenum];


	for (lightstyles=0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
	{
		if ( f->styles[lightstyles] == 255 )
			break;
	}
	if ( !lightstyles )
		return;

	
	//
	// sample the triangulation
	//
	minlight = FloatForKey (face_entity[facenum], "_minlight") * 128;

	bool needsBumpmap = ( texinfo[f->texinfo].flags & SURF_BUMPLIGHT ) ? true : false;
	int bumpSampleCount = needsBumpmap ? NUM_BUMP_VECTS + 1 : 1;

	bool bDisp = ( f->dispinfo != -1 );

//#define RANDOM_COLOR

#ifdef RANDOM_COLOR
	unsigned char randomColor[3];
	GetRandomColor( randomColor );
#endif
	

	// NOTE: I'm using these RB trees to sort all the illumination values
	// to compute median colors. Turns out that this is a somewhat better
	// method that using the average; usually if there are surfaces
	// with a large light intensity variation, the extremely bright regions
	// have a very small area and tend to influence the average too much.
	CUtlRBTree< float, int >	m_Red( 0, 256, FloatLess );
	CUtlRBTree< float, int >	m_Green( 0, 256, FloatLess );
	CUtlRBTree< float, int >	m_Blue( 0, 256, FloatLess );

	for (k=0 ; k < lightstyles; k++ )
	{
		m_Red.RemoveAll();
		m_Green.RemoveAll();
		m_Blue.RemoveAll();

		if (!do_fast)
		{
			if( !bDisp )
			{
				rad = BuildLuxelRadial( facenum, k );
			}
			else
			{
				rad = StaticDispMgr()->BuildLuxelRadial( facenum, k, needsBumpmap );
			}
		}

		if (numbounce > 0 && k == 0)
		{
			// currently only radiosity light non-displacement surfaces!
			if( !bDisp )
			{
				prad = BuildPatchRadial( facenum );
			}
			else
			{
				prad = StaticDispMgr()->BuildPatchRadial( facenum, needsBumpmap );
			}
		}

		// pack the nonbump texture and the three bump texture for the given 
		// lightstyle right next to each other.
		// NOTE: Even though it's building positions for all bump-mapped data,
		// it isn't going to use those positions (see loop over bumpSample below)
		// The file offset is correctly computed to only store space for 1 set
		// of light data if we don't have bumped lighting.
		for( bumpSample = 0; bumpSample < bumpSampleCount; ++bumpSample )
		{
			pdata[bumpSample] = &(*pdlightdata)[f->lightofs + (k * bumpSampleCount + bumpSample) * fl->numluxels*4]; 
		}

		// Compute the average luxel color, but not for the bump samples
		Vector avg( 0.0f, 0.0f, 0.0f );
		int avgCount = 0;

		for (j=0 ; j<fl->numluxels; j++)
		{
			// garymct - direct lighting
			bool baseSampleOk = true;

			if (!do_fast)
			{
				if( !bDisp )
				{
					baseSampleOk = SampleRadial( rad, fl->luxel[j], lb, bumpSampleCount );
				}
				else
				{
					baseSampleOk = StaticDispMgr()->SampleRadial( facenum, rad, fl->luxel[j], j, lb, bumpSampleCount, false );
				}
			}
			else
			{
				for ( int iBump = 0 ; iBump < bumpSampleCount; iBump++ )
				{
					lb[iBump] = fl->light[0][iBump][j];
				}
			}

			if (prad)
			{
				// garymct - bounced light
				// v is indirect light that is received on the luxel.
				if( !bDisp )
				{
					SampleRadial( prad, fl->luxel[j], v, bumpSampleCount );
				}
				else
				{
					StaticDispMgr()->SampleRadial( facenum, prad, fl->luxel[j], j, v, bumpSampleCount, true );
				}

				for( bumpSample = 0; bumpSample < bumpSampleCount; ++bumpSample )
				{
					lb[bumpSample].AddLight( v[bumpSample] );
				}
			}

			if ( bDisp && g_bDumpPatches )
			{
				for( bumpSample = 0; bumpSample < bumpSampleCount; ++bumpSample )
				{
					DumpDispLuxels( facenum, lb[bumpSample].m_vecLighting, j, bumpSample );
				}
			}

			if (fl->numsamples == 0)
			{
				for( i = 0; i < bumpSampleCount; i++ )
				{
					lb[i].Init( 255, 0, 0 );
				}
				baseSampleOk = false;
			}

			int bumpSample;
			for( bumpSample = 0; bumpSample < bumpSampleCount; bumpSample++ )
			{
				// clip from the bottom first
				// garymct: minlight is a per entity minimum light value?
				for( i=0; i<3; i++ )
				{
					lb[bumpSample].m_vecLighting[i] = max( lb[bumpSample].m_vecLighting[i], minlight );
				}
				
				// Do the average light computation, I'm assuming (perhaps incorrectly?)
				// that all luxels in a particular lightmap have the same area here. 
				// Also, don't bother doing averages for the bump samples. Doing it here 
				// because of the minlight clamp above + the random color testy thingy. 
				// Also have to do it before Vec3toColorRGBExp32 because it 
				// destructively modifies lb[bumpSample] (Feh!)
				if ((bumpSample == 0) && baseSampleOk)
				{
					++avgCount;

					ApplyMacroTextures( facenum, fl->luxel[j], lb[0].m_vecLighting );

					// For median computation
					m_Red.Insert( lb[bumpSample].m_vecLighting[0] );
					m_Green.Insert( lb[bumpSample].m_vecLighting[1] );
					m_Blue.Insert( lb[bumpSample].m_vecLighting[2] );
				}

#ifdef RANDOM_COLOR
				pdata[bumpSample][0] = randomColor[0] / ( bumpSample + 1 );
				pdata[bumpSample][1] = randomColor[1] / ( bumpSample + 1 );
				pdata[bumpSample][2] = randomColor[2] / ( bumpSample + 1 );
				pdata[bumpSample][3] = 0;
#else
				// convert to a 4 byte r,g,b,signed exponent format
				VectorToColorRGBExp32( Vector( lb[bumpSample].m_vecLighting.x, lb[bumpSample].m_vecLighting.y,
											   lb[bumpSample].m_vecLighting.z ), *( ColorRGBExp32 *)pdata[bumpSample] );
#endif

				pdata[bumpSample] += 4;
			}
		}
		FreeRadial( rad );
		if (prad)
		{
			FreeRadial( prad );
			prad = NULL;
		}

		// Compute the median color for this lightstyle
		// Remember, the data goes *before* the specified light_ofs, in *reverse order*
		ColorRGBExp32 *pAvgColor = dface_AvgLightColor( f, k );
		if (avgCount == 0)
		{
			Vector median( 0, 0, 0 );
			VectorToColorRGBExp32( median, *pAvgColor );
		}
		else
		{
			unsigned int r, g, b;
			r = m_Red.FirstInorder();
			g = m_Green.FirstInorder();
			b = m_Blue.FirstInorder();
			avgCount >>= 1;
			while (avgCount > 0)
			{
				r = m_Red.NextInorder(r);
				g = m_Green.NextInorder(g);
				b = m_Blue.NextInorder(b);
				--avgCount;
			}

			Vector median( m_Red[r], m_Green[g], m_Blue[b] );
			VectorToColorRGBExp32( median, *pAvgColor );
		}
	}
}
