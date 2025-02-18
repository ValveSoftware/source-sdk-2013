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
#include "tier1/utlvector.h"
#include "vmpi.h"
#include "mathlib/anorms.h"
#include "map_utils.h"
#include "mathlib/halton.h"
#include "imagepacker.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlbuffer.h"
#include "bitmap/tgawriter.h"
#include "mathlib/quantize.h"
#include "bitmap/imageformat.h"
#include "coordsize.h"

enum
{
	AMBIENT_ONLY = 0x1,
	NON_AMBIENT_ONLY = 0x2,
};

#define SMOOTHING_GROUP_HARD_EDGE	0xff000000

//==========================================================================//
// CNormalList.
//==========================================================================//

// This class keeps a list of unique normals and provides a fast 
class CNormalList
{
public:
	CNormalList();
	
	// Adds the normal if unique. Otherwise, returns the normal's index into m_Normals.
	int FindOrAddNormal( Vector const &vNormal );


public:
	
	CUtlVector<Vector>	m_Normals;


private:

	// This represents a grid from (-1,-1,-1) to (1,1,1).
	enum {NUM_SUBDIVS = 8};
	CUtlVector<int>	m_NormalGrid[NUM_SUBDIVS][NUM_SUBDIVS][NUM_SUBDIVS];
};


int g_iCurFace;
edgeshare_t	edgeshare[MAX_MAP_EDGES];

Vector	face_centroids[MAX_MAP_EDGES];

int vertexref[MAX_MAP_VERTS];
int *vertexface[MAX_MAP_VERTS];
faceneighbor_t faceneighbor[MAX_MAP_FACES];

static directlight_t *gSkyLight = NULL;
static directlight_t *gAmbient = NULL;

//==========================================================================//
// CNormalList implementation.
//==========================================================================//

CNormalList::CNormalList() : m_Normals( 128 )
{
	for( int i=0; i < sizeof(m_NormalGrid)/sizeof(m_NormalGrid[0][0][0]); i++ )
	{
		(&m_NormalGrid[0][0][0] + i)->SetGrowSize( 16 );
	}
}

int CNormalList::FindOrAddNormal( Vector const &vNormal )
{
	int gi[3];

	// See which grid element it's in.
	for( int iDim=0; iDim < 3; iDim++ )
	{
		gi[iDim] = (int)( ((vNormal[iDim] + 1.0f) * 0.5f) * NUM_SUBDIVS - 0.000001f );
		gi[iDim] = min( gi[iDim], NUM_SUBDIVS );
		gi[iDim] = max( gi[iDim], 0 );
	}

	// Look for a matching vector in there.
	CUtlVector<int> *pGridElement = &m_NormalGrid[gi[0]][gi[1]][gi[2]];
	for( int i=0; i < pGridElement->Size(); i++ )
	{
		int iNormal = pGridElement->Element(i);

		Vector *pVec = &m_Normals[iNormal];
		//if( pVec->DistToSqr(vNormal) < 0.00001f )
		if( *pVec == vNormal )
			return iNormal;
	}

	// Ok, add a new one.
	pGridElement->AddToTail( m_Normals.Size() );
	return m_Normals.AddToTail( vNormal );
}

// FIXME: HACK until the plane normals are made more happy
void GetBumpNormals( const float* sVect, const float* tVect, const Vector& flatNormal, 
					 const Vector& phongNormal, Vector bumpNormals[NUM_BUMP_VECTS] )
{
	Vector stmp( sVect[0], sVect[1], sVect[2] );
	Vector ttmp( tVect[0], tVect[1], tVect[2] );
	GetBumpNormals( stmp, ttmp, flatNormal, phongNormal, bumpNormals );
}

int EdgeVertex( dface_t *f, int edge )
{
	int k;

	if (edge < 0)
		edge += f->numedges;
	else if (edge >= f->numedges)
		edge = edge % f->numedges;

	k = dsurfedges[f->firstedge + edge];
	if (k < 0)
	{
		// Msg("(%d %d) ", dedges[-k].v[1], dedges[-k].v[0] );
		return dedges[-k].v[1];
	}
	else
	{
		// Msg("(%d %d) ", dedges[k].v[0], dedges[k].v[1] );
		return dedges[k].v[0];
	}
}


/*
  ============
  PairEdges
  ============
*/
void PairEdges (void)
{
	int		        i, j, k, n, m;
	dface_t	        *f;
	int             numneighbors;
    int             tmpneighbor[64];
	faceneighbor_t  *fn;

	// count number of faces that reference each vertex
	for (i=0, f = g_pFaces; i<numfaces ; i++, f++)
	{
        for (j=0 ; j<f->numedges ; j++)
        {
            // Store the count in vertexref
            vertexref[EdgeVertex(f,j)]++;
        }
	}

	// allocate room
	for (i = 0; i < numvertexes; i++)
	{
		// use the count from above to allocate a big enough array
		vertexface[i] = ( int* )calloc( vertexref[i], sizeof( vertexface[0] ) );
		// clear the temporary data
		vertexref[i] = 0;
	}

	// store a list of every face that uses a particular vertex
	for (i=0, f = g_pFaces ; i<numfaces ; i++, f++)
	{
        for (j=0 ; j<f->numedges ; j++)
        {
            n = EdgeVertex(f,j);
            
            for (k = 0; k < vertexref[n]; k++)
            {
                if (vertexface[n][k] == i)
                    break;
            }
            if (k >= vertexref[n])
            {
                // add the face to the list
                vertexface[n][k] = i;
                vertexref[n]++;
            }
        }
	}

	// calc normals and set displacement surface flag
	for (i=0, f = g_pFaces; i<numfaces ; i++, f++)
	{
		fn = &faceneighbor[i];

		// get face normal
		VectorCopy( dplanes[f->planenum].normal, fn->facenormal );

		// set displacement surface flag
		fn->bHasDisp = false;
		if( ValidDispFace( f ) )
		{
			fn->bHasDisp = true;
		}
	}

	// find neighbors
	for (i=0, f = g_pFaces ; i<numfaces ; i++, f++)
	{
		numneighbors = 0;
		fn = &faceneighbor[i];

        // allocate room for vertex normals
        fn->normal = ( Vector* )calloc( f->numedges, sizeof( fn->normal[0] ) );
     		
        // look up all faces sharing vertices and add them to the list
        for (j=0 ; j<f->numedges ; j++)
        {
            n = EdgeVertex(f,j);
            
            for (k = 0; k < vertexref[n]; k++)
            {
                double	cos_normals_angle;
                Vector  *pNeighbornormal;
                
                // skip self
                if (vertexface[n][k] == i)
                    continue;

				// if this face doens't have a displacement -- don't consider displacement neighbors
				if( ( !fn->bHasDisp ) && ( faceneighbor[vertexface[n][k]].bHasDisp ) )
					continue;

                pNeighbornormal = &faceneighbor[vertexface[n][k]].facenormal;
                cos_normals_angle = DotProduct( *pNeighbornormal, fn->facenormal );
					
				// add normal if >= threshold or its a displacement surface (this is only if the original
				// face is a displacement)
				if ( fn->bHasDisp )
				{
					// Always smooth with and against a displacement surface.
					VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
				}
				else
				{
					// No smoothing - use of method (backwards compatibility).
					if ( ( f->smoothingGroups == 0 ) && ( g_pFaces[vertexface[n][k]].smoothingGroups == 0 ) )
					{
						if ( cos_normals_angle >= smoothing_threshold )
						{
							VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
						}
						else
						{
							// not considered a neighbor
							continue;
						}
					}
					else
					{
						unsigned int smoothingGroup = ( f->smoothingGroups & g_pFaces[vertexface[n][k]].smoothingGroups );

						// Hard edge.
						if ( ( smoothingGroup & SMOOTHING_GROUP_HARD_EDGE ) != 0 )
							continue;

						if ( smoothingGroup != 0 )
						{
							VectorAdd( fn->normal[j], *pNeighbornormal, fn->normal[j] );
						}
						else
						{
							// not considered a neighbor
							continue;
						}
					}
				}

				// look to see if we've already added this one
				for (m = 0; m < numneighbors; m++)
				{
					if (tmpneighbor[m] == vertexface[n][k])
						break;
				}
				
				if (m >= numneighbors)
				{
					// add to neighbor list
					tmpneighbor[m] = vertexface[n][k];
					numneighbors++;
					if ( numneighbors > ARRAYSIZE(tmpneighbor) )
					{
						Error("Stack overflow in neighbors\n");
					}
				}
            }
        }

        if (numneighbors)
        {
            // copy over neighbor list
            fn->numneighbors = numneighbors;
            fn->neighbor = ( int* )calloc( numneighbors, sizeof( fn->neighbor[0] ) );
            for (m = 0; m < numneighbors; m++)
            {
                fn->neighbor[m] = tmpneighbor[m];
            }
        }
        
		// fixup normals
        for (j = 0; j < f->numedges; j++)
        {
            VectorAdd( fn->normal[j], fn->facenormal, fn->normal[j] );
            VectorNormalize( fn->normal[j] );
        }
    }
}


void SaveVertexNormals( void )
{
	faceneighbor_t *fn;
	int i, j;
	dface_t *f;
	CNormalList normalList;

	g_numvertnormalindices = 0;

	for( i = 0 ;i<numfaces ; i++ )
	{
		fn = &faceneighbor[i];
		f = &g_pFaces[i];

		for( j = 0; j < f->numedges; j++ )
		{
			Vector vNormal; 
			if( fn->normal )
			{
				vNormal = fn->normal[j];
			}
			else
			{
				// original faces don't have normals
				vNormal.Init( 0, 0, 0 );
			}
			
			if( g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES )
			{
				Error( "g_numvertnormalindices == MAX_MAP_VERTNORMALINDICES" );
			}
			
			g_vertnormalindices[g_numvertnormalindices] = (unsigned short)normalList.FindOrAddNormal( vNormal );
			g_numvertnormalindices++;
		}
	}

	if( normalList.m_Normals.Size() > MAX_MAP_VERTNORMALS )
	{
		Error( "g_numvertnormals > MAX_MAP_VERTNORMALS" );
	}

	// Copy the list of unique vert normals into g_vertnormals.
	g_numvertnormals = normalList.m_Normals.Size();
	memcpy( g_vertnormals, normalList.m_Normals.Base(), sizeof(g_vertnormals[0]) * normalList.m_Normals.Size() );
}

/*
  =================================================================

  LIGHTMAP SAMPLE GENERATION

  =================================================================
*/


//-----------------------------------------------------------------------------
// Purpose: Spits out an error message with information about a lightinfo_t.
// Input  : s - Error message string.
//			l - lightmap info struct.
//-----------------------------------------------------------------------------
void ErrorLightInfo(const char *s, lightinfo_t *l)
{
	texinfo_t *tex = &texinfo[l->face->texinfo];
	winding_t *w = WindingFromFace(&g_pFaces[l->facenum], l->modelorg);

	//
	// Show the face center and material name if possible.
	//
	if (w != NULL)
	{
		// Don't exit, we'll try to recover...
		Vector vecCenter;
		WindingCenter(w, vecCenter);
//		FreeWinding(w);

		Warning("%s at (%g, %g, %g)\n\tmaterial=%s\n", s, (double)vecCenter.x, (double)vecCenter.y, (double)vecCenter.z, TexDataStringTable_GetString( dtexdata[tex->texdata].nameStringTableID ) );
	}
	//
	// If not, just show the material name.
	//
	else
	{
		Warning("%s at (degenerate face)\n\tmaterial=%s\n", s, TexDataStringTable_GetString( dtexdata[tex->texdata].nameStringTableID ));
	}
}



void CalcFaceVectors(lightinfo_t *l)
{
	texinfo_t	*tex;
	int			i, j;

	tex = &texinfo[l->face->texinfo];
	
    // move into lightinfo_t
	for (i=0 ; i<2 ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			l->worldToLuxelSpace[i][j] = tex->lightmapVecsLuxelsPerWorldUnits[i][j];
		}
	}

	//Solve[ { x * w00 + y * w01 + z * w02 - s == 0, x * w10 + y * w11 + z * w12 - t == 0, A * x + B * y + C * z + D == 0 }, { x, y, z } ]
	//Rule(x,(  C*s*w11 - B*s*w12 + B*t*w02 - C*t*w01 + D*w02*w11 - D*w01*w12) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 )),
	//Rule(y,(  A*s*w12 - C*s*w10 + C*t*w00 - A*t*w02 + D*w00*w12 - D*w02*w10) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 )),
	//Rule(z,(  B*s*w10 - A*s*w11 + A*t*w01 - B*t*w00 + D*w01*w10 - D*w00*w11) / (+ A*w01*w12 - A*w02*w11 + B*w02*w10 - B*w00*w12 + C*w00*w11 - C*w01*w10 ))))

	Vector luxelSpaceCross;

	luxelSpaceCross[0] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][1] * tex->lightmapVecsLuxelsPerWorldUnits[0][2] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][2] * tex->lightmapVecsLuxelsPerWorldUnits[0][1];
	luxelSpaceCross[1] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][2] * tex->lightmapVecsLuxelsPerWorldUnits[0][0] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][0] * tex->lightmapVecsLuxelsPerWorldUnits[0][2];
	luxelSpaceCross[2] = 
		tex->lightmapVecsLuxelsPerWorldUnits[1][0] * tex->lightmapVecsLuxelsPerWorldUnits[0][1] - 
		tex->lightmapVecsLuxelsPerWorldUnits[1][1] * tex->lightmapVecsLuxelsPerWorldUnits[0][0];

	float det = -DotProduct( l->facenormal, luxelSpaceCross );
	if ( fabs( det ) < 1.0e-20 )
	{
		Warning(" warning - face vectors parallel to face normal. bad lighting will be produced\n" );
		l->luxelOrigin = vec3_origin;
	}
	else
	{
		// invert the matrix
		l->luxelToWorldSpace[0][0]	= (l->facenormal[2] * l->worldToLuxelSpace[1][1] - l->facenormal[1] * l->worldToLuxelSpace[1][2]) / det;
		l->luxelToWorldSpace[1][0]	= (l->facenormal[1] * l->worldToLuxelSpace[0][2] - l->facenormal[2] * l->worldToLuxelSpace[0][1]) / det;
		l->luxelOrigin[0]			= -(l->facedist * luxelSpaceCross[0]) / det;
		l->luxelToWorldSpace[0][1]  = (l->facenormal[0] * l->worldToLuxelSpace[1][2] - l->facenormal[2] * l->worldToLuxelSpace[1][0]) / det; 
		l->luxelToWorldSpace[1][1]  = (l->facenormal[2] * l->worldToLuxelSpace[0][0] - l->facenormal[0] * l->worldToLuxelSpace[0][2]) / det; 
		l->luxelOrigin[1]			= -(l->facedist * luxelSpaceCross[1]) / det;
		l->luxelToWorldSpace[0][2]  = (l->facenormal[1] * l->worldToLuxelSpace[1][0] - l->facenormal[0] * l->worldToLuxelSpace[1][1]) / det; 
		l->luxelToWorldSpace[1][2]  = (l->facenormal[0] * l->worldToLuxelSpace[0][1] - l->facenormal[1] * l->worldToLuxelSpace[0][0]) / det; 
		l->luxelOrigin[2]			= -(l->facedist * luxelSpaceCross[2]) / det;

		// adjust for luxel offset
		VectorMA( l->luxelOrigin, -tex->lightmapVecsLuxelsPerWorldUnits[0][3], l->luxelToWorldSpace[0], l->luxelOrigin );
		VectorMA( l->luxelOrigin, -tex->lightmapVecsLuxelsPerWorldUnits[1][3], l->luxelToWorldSpace[1], l->luxelOrigin );
	}
	// compensate for org'd bmodels
	VectorAdd (l->luxelOrigin, l->modelorg, l->luxelOrigin);
}



winding_t *LightmapCoordWindingForFace( lightinfo_t *l )
{
	int			i;
	winding_t	*w;

	w = WindingFromFace( l->face, l->modelorg );

	for (i = 0; i < w->numpoints; i++)
	{
		Vector2D coord;
		WorldToLuxelSpace( l, w->p[i], coord );
		w->p[i].x = coord.x;
		w->p[i].y = coord.y;
		w->p[i].z = 0;
	}

	return w;
}


void WriteCoordWinding (FILE *out, lightinfo_t *l, winding_t *w, Vector& color )
{
	int			i;
	Vector		pos;

	fprintf (out, "%i\n", w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		LuxelSpaceToWorld( l, w->p[i][0], w->p[i][1], pos );
		fprintf (out, "%5.2f %5.2f %5.2f %5.3f %5.3f %5.3f\n",
				 pos[0],
				 pos[1],
				 pos[2],
				 color[ 0 ] / 256,
				 color[ 1 ] / 256,
				 color[ 2 ] / 256 );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void DumpFaces( lightinfo_t *pLightInfo, int ndxFace )
{
	static	FileHandle_t out;

	// get face data
	faceneighbor_t *fn = &faceneighbor[ndxFace];
	Vector &centroid = face_centroids[ndxFace];

	// disable threading (not a multi-threadable function!)
	ThreadLock();
	
	if( !out )
	{
		// open the file
		out = g_pFileSystem->Open( "face.txt", "w" );
		if( !out )
			return;
	}
	
	//
	// write out face
	//
	for( int ndxEdge = 0; ndxEdge < pLightInfo->face->numedges; ndxEdge++ )
	{
//		int edge = dsurfedges[pLightInfo->face->firstedge+ndxEdge];

		Vector p1, p2;
		VectorAdd( dvertexes[EdgeVertex( pLightInfo->face, ndxEdge )].point, pLightInfo->modelorg, p1 );
		VectorAdd( dvertexes[EdgeVertex( pLightInfo->face, ndxEdge+1 )].point, pLightInfo->modelorg, p2 );
		
		Vector &n1 = fn->normal[ndxEdge];
		Vector &n2 = fn->normal[(ndxEdge+1)%pLightInfo->face->numedges];
		
		CmdLib_FPrintf( out, "3\n");
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", p1[0], p1[1], p1[2], n1[0] * 0.5 + 0.5, n1[1] * 0.5 + 0.5, n1[2] * 0.5 + 0.5 );
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", p2[0], p2[1], p2[2], n2[0] * 0.5 + 0.5, n2[1] * 0.5 + 0.5, n2[2] * 0.5 + 0.5 );
		
		CmdLib_FPrintf(out, "%f %f %f %f %f %f\n", centroid[0] + pLightInfo->modelorg[0], 
					   centroid[1] + pLightInfo->modelorg[1], 
					   centroid[2] + pLightInfo->modelorg[2], 
					   fn->facenormal[0] * 0.5 + 0.5, 
					   fn->facenormal[1] * 0.5 + 0.5, 
					   fn->facenormal[2] * 0.5 + 0.5 );
		
	}
	
	// enable threading
	ThreadUnlock();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFacesamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// ratio of world area / lightmap area
	texinfo_t *pTex = &texinfo[pLightInfo->face->texinfo];
	pFaceLight->worldAreaPerLuxel = 1.0 / ( sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[0], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[0] ) ) * 
											sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[1], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[1] ) ) );

	//
	// quickly create samples and luxels (copy over samples)
	//
	pFaceLight->numsamples = width * height;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		return false;

	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	if( !pFaceLight->luxel )
		return false;

	sample_t *pSamples = pFaceLight->sample;
	Vector	 *pLuxels = pFaceLight->luxel;

	for( int t = 0; t < height; t++ )
	{
		for( int s = 0; s < width; s++ )
		{
			pSamples->s = s;
			pSamples->t = t;
			pSamples->coord[0] = s;
			pSamples->coord[1] = t;
			// unused but initialized anyway
			pSamples->mins[0] = s - 0.5;
			pSamples->mins[1] = t - 0.5;
			pSamples->maxs[0] = s + 0.5;
			pSamples->maxs[1] = t + 0.5;
			pSamples->area = pFaceLight->worldAreaPerLuxel;
			LuxelSpaceToWorld( pLightInfo, pSamples->coord[0], pSamples->coord[1], pSamples->pos );
			VectorCopy( pSamples->pos, *pLuxels );

			pSamples++;
			pLuxels++;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildSamplesAndLuxels_DoFast( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build samples for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFacesamplesAndLuxels_DoFast( pLightInfo, pFaceLight );
	}
	// build samples for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispSamplesAndLuxels_DoFast( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFacesamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// ratio of world area / lightmap area
	texinfo_t *pTex = &texinfo[pLightInfo->face->texinfo];
	pFaceLight->worldAreaPerLuxel = 1.0 / ( sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[0], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[0] ) ) * 
											sqrt( DotProduct( pTex->lightmapVecsLuxelsPerWorldUnits[1], 
															  pTex->lightmapVecsLuxelsPerWorldUnits[1] ) ) );

	// allocate a large number of samples for creation -- get copied later!
	CUtlVector<sample_t> sampleData;
	sampleData.SetCount( SINGLE_BRUSH_MAP * 2 );
	sample_t *samples = sampleData.Base();
	sample_t *pSamples = samples;

	// lightmap space winding
	winding_t *pLightmapWinding = LightmapCoordWindingForFace( pLightInfo ); 

	//
	// build vector pointing along the lightmap cutting planes
	//
	Vector sNorm( 1.0f, 0.0f, 0.0f );
	Vector tNorm( 0.0f, 1.0f, 0.0f );

	// sample center offset
	float sampleOffset = ( do_centersamples ) ? 0.5 : 1.0;

	//
	// clip the lightmap "spaced" winding by the lightmap cutting planes
	//
	winding_t *pWindingT1, *pWindingT2;
	winding_t *pWindingS1, *pWindingS2;
	float dist;

	for( int t = 0; t < height && pLightmapWinding; t++ )
	{
		dist = t + sampleOffset;
		
		// lop off a sample in the t dimension
		// hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
		ClipWindingEpsilon( pLightmapWinding, tNorm, dist, ON_EPSILON / 16.0f, &pWindingT1, &pWindingT2 );

		for( int s = 0; s < width && pWindingT2; s++ )
		{
			dist = s + sampleOffset;

			// lop off a sample in the s dimension, and put it in ws2
			// hack - need a separate epsilon for lightmap space since ON_EPSILON is for texture space
			ClipWindingEpsilon( pWindingT2, sNorm, dist, ON_EPSILON / 16.0f, &pWindingS1, &pWindingS2 );

			//
			// s2 winding is a single sample worth of winding
			//
			if( pWindingS2 )
			{
				// save the s, t positions
				pSamples->s = s;
				pSamples->t = t;

				// get the lightmap space area of ws2 and convert to world area
				// and find the center (then convert it to 2D)
				Vector center;
				pSamples->area = WindingAreaAndBalancePoint(  pWindingS2, center ) * pFaceLight->worldAreaPerLuxel;
				pSamples->coord[0] = center.x; 
				pSamples->coord[1] = center.y;

				// find winding bounds (then convert it to 2D)
				Vector minbounds, maxbounds;
				WindingBounds( pWindingS2, minbounds, maxbounds );
				pSamples->mins[0] = minbounds.x; 
				pSamples->mins[1] = minbounds.y;
				pSamples->maxs[0] = maxbounds.x; 
				pSamples->maxs[1] = maxbounds.y;

				// convert from lightmap space to world space
				LuxelSpaceToWorld( pLightInfo, pSamples->coord[0], pSamples->coord[1], pSamples->pos );

				if (g_bDumpPatches || (do_extra && pSamples->area < pFaceLight->worldAreaPerLuxel - EQUAL_EPSILON))
				{
					//
					// convert the winding from lightmaps space to world for debug rendering and sub-sampling
					//
					Vector worldPos;
					for( int ndxPt = 0; ndxPt < pWindingS2->numpoints; ndxPt++ )
					{
						LuxelSpaceToWorld( pLightInfo, pWindingS2->p[ndxPt].x, pWindingS2->p[ndxPt].y, worldPos );
						VectorCopy( worldPos, pWindingS2->p[ndxPt] );
					}
					pSamples->w = pWindingS2;
				}
				else
				{
					// winding isn't needed, free it.
					pSamples->w = NULL;
					FreeWinding( pWindingS2 );
				}

				pSamples++;
			}

			//
			// if winding T2 still exists free it and set it equal S1 (the rest of the row minus the sample just created)
			//
			if( pWindingT2 )
			{
				FreeWinding( pWindingT2 );
			}

			// clip the rest of "s"
			pWindingT2 = pWindingS1;
		}

		//
		// if the original lightmap winding exists free it and set it equal to T1 (the rest of the winding not cut into samples) 
		//
		if( pLightmapWinding )
		{
			FreeWinding( pLightmapWinding );
		}

		if( pWindingT2 )
		{
			FreeWinding( pWindingT2 );
		}

		pLightmapWinding = pWindingT1;
	}

	//
	// copy over samples
	//
	pFaceLight->numsamples = pSamples - samples;
	pFaceLight->sample = ( sample_t* )calloc( pFaceLight->numsamples, sizeof( *pFaceLight->sample ) );
	if( !pFaceLight->sample )
		return false;

	memcpy( pFaceLight->sample, samples, pFaceLight->numsamples * sizeof( *pFaceLight->sample ) );

	// supply a default sample normal (face normal - assumed flat)
	for( int ndxSample = 0; ndxSample < pFaceLight->numsamples; ndxSample++ )
	{
		Assert ( VectorLength ( pLightInfo->facenormal ) > 1.0e-20);
		pFaceLight->sample[ndxSample].normal = pLightInfo->facenormal;
	}

	// statistics - warning?!
	if( pFaceLight->numsamples == 0 )
	{
		Msg( "no samples %d\n", pLightInfo->face - g_pFaces );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Free any windings used by this facelight. It's currently assumed they're not needed again
//-----------------------------------------------------------------------------
void FreeSampleWindings( facelight_t *fl )
{
	int i;
	for (i = 0; i < fl->numsamples; i++)
	{
		if (fl->sample[i].w)
		{
			FreeWinding( fl->sample[i].w );
			fl->sample[i].w = NULL;
		}
	}
}



//-----------------------------------------------------------------------------
// Purpose: build the sample data for each lightmapped primitive type
//-----------------------------------------------------------------------------
bool BuildSamples( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build samples for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFacesamples( pLightInfo, pFaceLight );
	}
	// build samples for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispSamples( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool BuildFaceLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight )
{
	// lightmap size
	int width = pLightInfo->face->m_LightmapTextureSizeInLuxels[0]+1;
	int height = pLightInfo->face->m_LightmapTextureSizeInLuxels[1]+1;

	// calcuate actual luxel points
	pFaceLight->numluxels = width * height;
	pFaceLight->luxel = ( Vector* )calloc( pFaceLight->numluxels, sizeof( *pFaceLight->luxel ) );
	if( !pFaceLight->luxel )
		return false;

	for( int t = 0; t < height; t++ )
	{
		for( int s = 0; s < width; s++ )
		{
			LuxelSpaceToWorld( pLightInfo, s, t, pFaceLight->luxel[s+t*width] );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: build the luxels (find the luxel centers) for each lightmapped
//          primitive type
//-----------------------------------------------------------------------------
bool BuildLuxels( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// build luxels for a "face"
	if( pLightInfo->face->dispinfo == -1 )
	{
		return BuildFaceLuxels( pLightInfo, pFaceLight );
	}
	// build luxels for a "displacement"
	else
	{
		return StaticDispMgr()->BuildDispLuxels( pLightInfo, pFaceLight, ndxFace );
	}
}


//-----------------------------------------------------------------------------
// Purpose: for each face, find the center of each luxel; for each texture
//          aligned grid point, back project onto the plane and get the world
//          xyz value of the sample point
// NOTE: ndxFace = facenum
//-----------------------------------------------------------------------------
void CalcPoints( lightinfo_t *pLightInfo, facelight_t *pFaceLight, int ndxFace )
{
	// debugging!
	if( g_bDumpPatches ) 
	{
		DumpFaces( pLightInfo, ndxFace );
	}

	// quick and dirty!
	if( do_fast )
	{
		if( !BuildSamplesAndLuxels_DoFast( pLightInfo, pFaceLight, ndxFace ) )
		{
			Msg( "Face %d: (Fast)Error Building Samples and Luxels\n", ndxFace );
		}
		return;
	}

	// build the samples
	if( !BuildSamples( pLightInfo, pFaceLight, ndxFace ) )
	{
		Msg( "Face %d: Error Building Samples\n", ndxFace );
	}

	// build the luxels
	if( !BuildLuxels( pLightInfo, pFaceLight, ndxFace ) )
	{
		Msg( "Face %d: Error Building Luxels\n", ndxFace );
	}
}


//==============================================================

directlight_t	*activelights;
directlight_t	*freelights;

facelight_t		facelight[MAX_MAP_FACES];
int				numdlights;

/*
  ==================
  FindTargetEntity
  ==================
*/
entity_t *FindTargetEntity (char *target)
{
	int		i;
	char	*n;

	for (i=0 ; i<num_entities ; i++)
	{
		n = ValueForKey (&entities[i], "targetname");
		if (!strcmp (n, target))
			return &entities[i];
	}

	return NULL;
}



/*
  =============
  AllocDLight
  =============
*/

int GetVisCache( int lastoffset, int cluster, byte *pvs );
void SetDLightVis( directlight_t *dl, int cluster );
void MergeDLightVis( directlight_t *dl, int cluster );

directlight_t *AllocDLight( Vector& origin, bool bAddToList )
{
	directlight_t *dl;

	dl = ( directlight_t* )calloc(1, sizeof(directlight_t));
	dl->index = numdlights++;

	VectorCopy( origin, dl->light.origin );

	dl->light.cluster = ClusterFromPoint(dl->light.origin);
	SetDLightVis( dl, dl->light.cluster );

	dl->facenum = -1;

	if ( bAddToList )
	{
		dl->next = activelights;
		activelights = dl;
	}

	return dl;
}

void AddDLightToActiveList( directlight_t *dl )
{
	dl->next = activelights;
	activelights = dl;
}

void FreeDLights()
{
	gSkyLight = NULL;
	gAmbient = NULL;

	directlight_t *pNext;
	for( directlight_t *pCur=activelights; pCur; pCur=pNext )
	{
		pNext = pCur->next;
		free( pCur );
	}
	activelights = 0;
}


void SetDLightVis( directlight_t *dl, int cluster )
{
	if (dl->pvs == NULL)
	{
		dl->pvs = (byte *)calloc( 1, (dvis->numclusters / 8) + 1 );
	}

	GetVisCache( -1, cluster, dl->pvs );
}

void MergeDLightVis( directlight_t *dl, int cluster )
{
	if (dl->pvs == NULL)
	{
		SetDLightVis( dl, cluster );
	}
	else
	{
		byte		pvs[MAX_MAP_CLUSTERS/8];
		GetVisCache( -1, cluster, pvs );

		// merge both vis graphs
		for (int i = 0; i < (dvis->numclusters / 8) + 1; i++)
		{
			dl->pvs[i] |= pvs[i];
		}
	}
}


/*
  =============
  LightForKey
  =============
*/
int LightForKey (entity_t *ent, char *key, Vector& intensity )
{
	char *pLight;

	pLight = ValueForKey( ent, key );

	return LightForString( pLight, intensity );
}

int LightForString( char *pLight, Vector& intensity )
{
	double r, g, b, scaler;
	int argCnt;

	VectorFill( intensity, 0 );

	// scanf into doubles, then assign, so it is vec_t size independent
	r = g = b = scaler = 0;
	double r_hdr,g_hdr,b_hdr,scaler_hdr;
	argCnt = sscanf ( pLight, "%lf %lf %lf %lf %lf %lf %lf %lf", 
					  &r, &g, &b, &scaler, &r_hdr,&g_hdr,&b_hdr,&scaler_hdr );

	if (argCnt==8) 											// 2 4-tuples
	{
		if (g_bHDR)
		{
			r=r_hdr;
			g=g_hdr;
			b=b_hdr;
			scaler=scaler_hdr;
		}
		argCnt=4;
	}

	// make sure light is legal
	if( r < 0.0f || g < 0.0f || b < 0.0f || scaler < 0.0f )
	{
		intensity.Init( 0.0f, 0.0f, 0.0f );
		return false;
	}

	intensity[0] = pow( r / 255.0, 2.2 ) * 255;				// convert to linear
	
	switch( argCnt)
	{
		case 1:
			// The R,G,B values are all equal.
			intensity[1] = intensity[2] = intensity[0]; 
			break;
			
		case 3:
		case 4:
			// Save the other two G,B values.
			intensity[1] = pow( g / 255.0, 2.2 ) * 255;
			intensity[2] = pow( b / 255.0, 2.2 ) * 255;
			
			// Did we also get an "intensity" scaler value too?
			if ( argCnt == 4 )
			{
				// Scale the normalized 0-255 R,G,B values by the intensity scaler
				VectorScale( intensity, scaler / 255.0, intensity );
			}
			break;

		default:
			printf("unknown light specifier type - %s\n",pLight);
			return false;
	}
	// scale up source lights by scaling factor
	VectorScale( intensity, lightscale, intensity );
	return true;
}

//-----------------------------------------------------------------------------
// Various parsing methods
//-----------------------------------------------------------------------------

static void ParseLightGeneric( entity_t *e, directlight_t *dl )
{
	entity_t		*e2;
	char	        *target;
	Vector	        dest;

	dl->light.style = (int)FloatForKey (e, "style");
	
	// get intenfsity
	if( g_bHDR && LightForKey( e, "_lightHDR", dl->light.intensity ) ) 
	{
	}
	else
	{
		LightForKey( e, "_light", dl->light.intensity );
	}
	
	// check angle, targets
	target = ValueForKey (e, "target");
	if (target[0])
	{	// point towards target
		e2 = FindTargetEntity (target);
		if (!e2)
			Warning("WARNING: light at (%i %i %i) has missing target\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
		else
		{
			GetVectorForKey (e2, "origin", dest);
			VectorSubtract (dest, dl->light.origin, dl->light.normal);
			VectorNormalize (dl->light.normal);
		}
	}
	else
	{	
		// point down angle
		Vector angles;
		GetVectorForKey( e, "angles", angles );
		float pitch = FloatForKey (e, "pitch");
		float angle = FloatForKey (e, "angle");
		SetupLightNormalFromProps( QAngle( angles.x, angles.y, angles.z ), angle, pitch, dl->light.normal );
	}
	if ( g_bHDR )
		VectorScale( dl->light.intensity, 
					 FloatForKeyWithDefault( e, "_lightscaleHDR", 1.0 ),
					 dl->light.intensity );
}

static void SetLightFalloffParams( entity_t * e, directlight_t * dl )
{
	float d50=FloatForKey( e, "_fifty_percent_distance" );
	dl->m_flStartFadeDistance = 0;
	dl->m_flEndFadeDistance = - 1;
	dl->m_flCapDist = 1.0e22;
	if ( d50 )
	{
		float d0 = FloatForKey( e, "_zero_percent_distance" );
		if ( d0 < d50 )
		{
			Warning( "light has _fifty_percent_distance of %f but _zero_percent_distance of %f\n", d50, d0);
			d0 = 2.0 * d50;
		}
		float a = 0, b = 1, c = 0;
		if ( ! SolveInverseQuadraticMonotonic( 0, 1.0, d50, 2.0, d0, 256.0, a, b, c ))
		{
			Warning( "can't solve quadratic for light %f %f\n", d50, d0 );
		}
		// it it possible that the parameters couldn't be used because of enforing monoticity. If so, rescale so at
		// least the 50 percent value is right
//		printf("50 percent=%f 0 percent=%f\n",d50,d0);
// 		printf("a=%f b=%f c=%f\n",a,b,c);
		float v50 = c + d50 * ( b + d50 * a );
		float scale = 2.0 / v50;
		a *= scale;
		b *= scale;
		c *= scale;
// 		printf("scaled=%f a=%f b=%f c=%f\n",scale,a,b,c);
// 		for(float d=0;d<1000;d+=20)
// 			printf("at %f, %f\n",d,1.0/(c+d*(b+d*a)));
		dl->light.quadratic_attn = a;
		dl->light.linear_attn = b;
		dl->light.constant_attn = c;



		if ( IntForKey(e, "_hardfalloff" ) )
		{
			dl->m_flEndFadeDistance = d0;
			dl->m_flStartFadeDistance = 0.75 * d0 + 0.25 * d50;		// start fading 3/4 way between 50 and 0. could allow adjust
		}
		else
		{
			// now, we will find the point at which the 1/x term reaches its maximum value, and
			// prevent the light from going past there. If a user specifes an extreme falloff, the
			// quadratic will start making the light brighter at some distance. We handle this by
			// fading it from the minimum brightess point down to zero at 10x the minimum distance
			if ( fabs( a ) > 0. )
			{
				float flMax = b / ( - 2.0 * a );				// where f' = 0
				if ( flMax > 0.0 )
				{
					dl->m_flCapDist = flMax;
					dl->m_flStartFadeDistance = flMax;
					dl->m_flEndFadeDistance = 10.0 * flMax;
				}
			}
		}
	}
	else
	{
		dl->light.constant_attn = FloatForKey (e, "_constant_attn" );
		dl->light.linear_attn = FloatForKey (e, "_linear_attn" );
		dl->light.quadratic_attn = FloatForKey (e, "_quadratic_attn" );

		dl->light.radius = FloatForKey (e, "_distance");

		// clamp values to >= 0
		if ( dl->light.constant_attn < EQUAL_EPSILON )
			dl->light.constant_attn = 0;

		if ( dl->light.linear_attn < EQUAL_EPSILON )
			dl->light.linear_attn = 0;

		if ( dl->light.quadratic_attn < EQUAL_EPSILON )
			dl->light.quadratic_attn = 0;

		if ( dl->light.constant_attn < EQUAL_EPSILON && dl->light.linear_attn < EQUAL_EPSILON && dl->light.quadratic_attn < EQUAL_EPSILON )
			dl->light.constant_attn = 1;

		// scale intensity for unit 100 distance
		float ratio = ( dl->light.constant_attn + 100 * dl->light.linear_attn + 100 * 100 * dl->light.quadratic_attn );
		if ( ratio > 0 )
		{
			VectorScale( dl->light.intensity, ratio, dl->light.intensity );
		}
	}
}

static void ParseLightSpot( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, true );

	ParseLightGeneric( e, dl );

	dl->light.type = emit_spotlight;

	dl->light.stopdot = FloatForKey (e, "_inner_cone");
	if (!dl->light.stopdot)
		dl->light.stopdot = 10;

	dl->light.stopdot2 = FloatForKey (e, "_cone");
	if (!dl->light.stopdot2) 
		dl->light.stopdot2 = dl->light.stopdot;
	if (dl->light.stopdot2 < dl->light.stopdot)
		dl->light.stopdot2 = dl->light.stopdot;

	// This is a point light if stop dots are 180...
	if ((dl->light.stopdot == 180) && (dl->light.stopdot2 == 180))
	{
		dl->light.stopdot = dl->light.stopdot2 = 0;
		dl->light.type = emit_point;
		dl->light.exponent = 0;
	}
	else
	{
		// Clamp to 90, that's all DX8 can handle! 
		if (dl->light.stopdot > 90)
		{
			Warning("WARNING: light_spot at (%i %i %i) has inner angle larger than 90 degrees! Clamping to 90...\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
			dl->light.stopdot = 90;
		}

		if (dl->light.stopdot2 > 90)
		{
			Warning("WARNING: light_spot at (%i %i %i) has outer angle larger than 90 degrees! Clamping to 90...\n",
					(int)dl->light.origin[0], (int)dl->light.origin[1], (int)dl->light.origin[2]);
			dl->light.stopdot2 = 90;
		}

		dl->light.stopdot2 = (float)cos(dl->light.stopdot2/180*M_PI);
		dl->light.stopdot = (float)cos(dl->light.stopdot/180*M_PI);
		dl->light.exponent = FloatForKey (e, "_exponent");
	}

	SetLightFalloffParams(e,dl);
}

// NOTE: This is just a heuristic.  It traces a finite number of rays to find sky
// NOTE: Full vis is necessary to make this 100% correct.
bool CanLeafTraceToSky( int iLeaf )
{
	// UNDONE: Really want a point inside the leaf here.  Center is a guess, may not be in the leaf
	// UNDONE: Clip this to each plane bounding the leaf to guarantee
	Vector center = vec3_origin;
	for ( int i = 0; i < 3; i++ )
	{
		center[i] = ( (float)(dleafs[iLeaf].mins[i] + dleafs[iLeaf].maxs[i]) ) * 0.5f;
	}

	FourVectors center4, delta;
	fltx4 fractionVisible;
	for ( int j = 0; j < NUMVERTEXNORMALS; j+=4 )
	{
		// search back to see if we can hit a sky brush
		delta.LoadAndSwizzle( g_anorms[j], g_anorms[min( j+1, NUMVERTEXNORMALS-1 )],
			g_anorms[min( j+2, NUMVERTEXNORMALS-1 )], g_anorms[min( j+3, NUMVERTEXNORMALS-1 )] );
		delta *= -MAX_TRACE_LENGTH;
		delta += center4;

		// return true if any hits sky
		TestLine_DoesHitSky ( center4, delta, &fractionVisible );
		if ( TestSignSIMD ( CmpGtSIMD ( fractionVisible, Four_Zeros ) ) )
			return true;
	}

	return false;
}

void BuildVisForLightEnvironment( void )
{
	// Create the vis.
	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		dleafs[iLeaf].flags &= ~( LEAF_FLAGS_SKY | LEAF_FLAGS_SKY2D );
		unsigned int iFirstFace = dleafs[iLeaf].firstleafface;
		for ( int iLeafFace = 0; iLeafFace < dleafs[iLeaf].numleaffaces; ++iLeafFace )
		{
			unsigned int iFace = dleaffaces[iFirstFace+iLeafFace];
			
			texinfo_t &tex = texinfo[g_pFaces[iFace].texinfo];
			if ( tex.flags & SURF_SKY )
			{
				if ( tex.flags & SURF_SKY2D )
				{
					dleafs[iLeaf].flags |= LEAF_FLAGS_SKY2D;
				}
				else
				{
					dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
				}
				MergeDLightVis( gSkyLight, dleafs[iLeaf].cluster );
				MergeDLightVis( gAmbient, dleafs[iLeaf].cluster );
				break;
			}
		}
	}

	// Second pass to set flags on leaves that don't contain sky, but touch leaves that
	// contain sky.
	byte pvs[MAX_MAP_CLUSTERS / 8];

	int nLeafBytes = (numleafs >> 3) + 1;
	unsigned char *pLeafBits = (unsigned char *)stackalloc( nLeafBytes * sizeof(unsigned char) );
	unsigned char *pLeaf2DBits = (unsigned char *)stackalloc( nLeafBytes * sizeof(unsigned char) );
	memset( pLeafBits, 0, nLeafBytes );
	memset( pLeaf2DBits, 0, nLeafBytes );

	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		// If this leaf has light (3d skybox) in it, then don't bother
		if ( dleafs[iLeaf].flags & LEAF_FLAGS_SKY )
			continue;

		// Don't bother with this leaf if it's solid
		if ( dleafs[iLeaf].contents & CONTENTS_SOLID )
			continue;

		// See what other leaves are visible from this leaf
		GetVisCache( -1, dleafs[iLeaf].cluster, pvs );

		// Now check out all other leaves
		int nByte = iLeaf >> 3;
		int nBit = 1 << ( iLeaf & 0x7 );
		for ( int iLeaf2 = 0; iLeaf2 < numleafs; ++iLeaf2 )
		{
			if ( iLeaf2 == iLeaf )
				continue;

			if ( !(dleafs[iLeaf2].flags & ( LEAF_FLAGS_SKY | LEAF_FLAGS_SKY2D ) ) )
				continue;

			// Can this leaf see into the leaf with the sky in it?
			if ( !PVSCheck( pvs, dleafs[iLeaf2].cluster ) )
				continue;

			if ( dleafs[iLeaf2].flags & LEAF_FLAGS_SKY2D )
			{
				pLeaf2DBits[ nByte ] |= nBit;
			}
			if ( dleafs[iLeaf2].flags & LEAF_FLAGS_SKY )
			{
				pLeafBits[ nByte ] |= nBit;

				// As soon as we know this leaf needs to draw the 3d skybox, we're done
				break;
			}
		}
	}

	// Must set the bits in a separate pass so as to not flood-fill LEAF_FLAGS_SKY everywhere
	// pLeafbits is a bit array of all leaves that need to be marked as seeing sky
	for ( int iLeaf = 0; iLeaf < numleafs; ++iLeaf )
	{
		// If this leaf has light (3d skybox) in it, then don't bother
		if ( dleafs[iLeaf].flags & LEAF_FLAGS_SKY )
			continue;

		// Don't bother with this leaf if it's solid
		if ( dleafs[iLeaf].contents & CONTENTS_SOLID )
			continue;

		// Check to see if this is a 2D skybox leaf
		if ( pLeaf2DBits[ iLeaf >> 3 ] & (1 << ( iLeaf & 0x7 )) )
		{
			dleafs[iLeaf].flags |= LEAF_FLAGS_SKY2D;
		}

		// If this is a 3D skybox leaf, then we don't care if it was previously a 2D skybox leaf
		if ( pLeafBits[ iLeaf >> 3 ] & (1 << ( iLeaf & 0x7 )) )
		{
			dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
			dleafs[iLeaf].flags &= ~LEAF_FLAGS_SKY2D;
		}
		else
		{
			// if radial vis was used on this leaf some of the portals leading
			// to sky may have been culled.  Try tracing to find sky.
			if ( dleafs[iLeaf].flags & LEAF_FLAGS_RADIAL )
			{
				if ( CanLeafTraceToSky(iLeaf) )
				{
					// FIXME: Should make a version that checks if we hit 2D skyboxes.. oh well.
					dleafs[iLeaf].flags |= LEAF_FLAGS_SKY;
				}
			}
		}
	}
}

static char *ValueForKeyWithDefault (entity_t *ent, char *key, char *default_value = NULL)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
			return ep->value;
	return default_value;
}

static void ParseLightEnvironment( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, false );

	ParseLightGeneric( e, dl );

	char *angle_str=ValueForKeyWithDefault( e, "SunSpreadAngle" );
	if (angle_str)
	{
		g_SunAngularExtent=atof(angle_str);
		g_SunAngularExtent=sin((M_PI/180.0)*g_SunAngularExtent);
		printf("sun extent from map=%f\n",g_SunAngularExtent);
	}
	if ( !gSkyLight )
	{
		// Sky light.
		gSkyLight = dl;
		dl->light.type = emit_skylight;

		// Sky ambient light.
		gAmbient = AllocDLight( dl->light.origin, false );
		gAmbient->light.type = emit_skyambient;
		if( g_bHDR && LightForKey( e, "_ambientHDR", gAmbient->light.intensity ) )
		{
			// we have a valid HDR ambient light value
		}
		else if ( !LightForKey( e, "_ambient", gAmbient->light.intensity ) )
		{
			VectorScale( dl->light.intensity, 0.5, gAmbient->light.intensity );
		}
		if ( g_bHDR )
		{
			VectorScale( gAmbient->light.intensity, 
						 FloatForKeyWithDefault( e, "_AmbientScaleHDR", 1.0 ), 
						 gAmbient->light.intensity );
		}
		
		BuildVisForLightEnvironment();
 
		// Add sky and sky ambient lights to the list.
		AddDLightToActiveList( gSkyLight );
		AddDLightToActiveList( gAmbient );
	}
}

static void ParseLightPoint( entity_t* e, directlight_t* dl )
{
	Vector dest;
	GetVectorForKey (e, "origin", dest );
	dl = AllocDLight( dest, true );

	ParseLightGeneric( e, dl );

	dl->light.type = emit_point;

	SetLightFalloffParams(e,dl);
}

/*
  =============
  CreateDirectLights
  =============
*/
#define DIRECT_SCALE (100.0*100.0)
void CreateDirectLights (void)
{
	unsigned        i;
	CPatch	        *p = NULL;
	directlight_t	*dl = NULL;
	entity_t	    *e = NULL;
	char	        *name;
	Vector	        dest;

	numdlights = 0;

	FreeDLights();

	//
	// surfaces
	//
	unsigned int uiPatchCount = g_Patches.Count();
	for (i=0; i< uiPatchCount; i++)
	{
		p = &g_Patches.Element( i );

		// skip parent patches
		if (p->child1 != g_Patches.InvalidIndex() )
			continue;

		if (p->basearea < 1e-6)
			continue;

		if( VectorAvg( p->baselight ) >= dlight_threshold )
		{
			dl = AllocDLight( p->origin, true );

			dl->light.type = emit_surface;
			VectorCopy (p->normal, dl->light.normal);
			Assert( VectorLength( p->normal ) > 1.0e-20 );
			// scale intensity by number of texture instances
			VectorScale( p->baselight, lightscale * p->area * p->scale[0] * p->scale[1] / p->basearea, dl->light.intensity );

			// scale to a range that results in actual light
			VectorScale( dl->light.intensity, DIRECT_SCALE, dl->light.intensity );
		}
	}
	
	//
	// entities
	//
	for (i=0 ; i<(unsigned)num_entities ; i++)
	{
		e = &entities[i];
		name = ValueForKey (e, "classname");
		if (strncmp (name, "light", 5))
			continue;

		// Light_dynamic is actually a real entity; not to be included here...
		if (!strcmp (name, "light_dynamic"))
			continue;

		if (!strcmp (name, "light_spot"))
		{
			ParseLightSpot( e, dl );
		}
		else if (!strcmp(name, "light_environment")) 
		{
			ParseLightEnvironment( e, dl );
		}
		else if (!strcmp(name, "light")) 
		{
			ParseLightPoint( e, dl );
		}
		else
		{
			qprintf( "unsupported light entity: \"%s\"\n", name );
		}
	}

	qprintf ("%i direct lights\n", numdlights);
	// exit(1);
}

/*
  =============
  ExportDirectLightsToWorldLights
  =============
*/

void ExportDirectLightsToWorldLights()
{
	directlight_t		*dl;

	// In case the level has already been VRADed.
	*pNumworldlights = 0;

	for (dl = activelights; dl != NULL; dl = dl->next )
	{
		dworldlight_t *wl = &dworldlights[(*pNumworldlights)++];

		if (*pNumworldlights > MAX_MAP_WORLDLIGHTS)
		{
			Error("too many lights %d / %d\n", *pNumworldlights, MAX_MAP_WORLDLIGHTS );
		}

		wl->cluster	= dl->light.cluster;
		wl->type	= dl->light.type;
		wl->style	= dl->light.style;
		VectorCopy( dl->light.origin, wl->origin );
		// FIXME: why does vrad want 0 to 255 and not 0 to 1??
		VectorScale( dl->light.intensity, (1.0 / 255.0), wl->intensity );
		VectorCopy( dl->light.normal, wl->normal );
		wl->stopdot	= dl->light.stopdot;
		wl->stopdot2 = dl->light.stopdot2;
		wl->exponent = dl->light.exponent;
		wl->radius = dl->light.radius;
		wl->constant_attn = dl->light.constant_attn;
		wl->linear_attn = dl->light.linear_attn;
		wl->quadratic_attn = dl->light.quadratic_attn;
		wl->flags = 0;
	}
}

/*
  =============
  GatherSampleLight
  =============
*/
#define NORMALFORMFACTOR	40.156979 // accumuated dot products for hemisphere

#define CONSTANT_DOT (.7/2)

#define NSAMPLES_SUN_AREA_LIGHT 30							// number of samples to take for an
                                                            // non-point sun light

// Helper function - gathers light from sun (emit_skylight)
void GatherSampleSkyLightSSE( SSE_sampleLightOutput_t &out, directlight_t *dl, int facenum, 
							 FourVectors const& pos, FourVectors *pNormals, int normalCount, int iThread,
							 int nLFlags, int static_prop_index_to_ignore,
							 float flEpsilon )
{
	bool bIgnoreNormals = ( nLFlags & GATHERLFLAGS_IGNORE_NORMALS ) != 0;
	bool force_fast = ( nLFlags & GATHERLFLAGS_FORCE_FAST ) != 0;

	fltx4 dot;

	if ( bIgnoreNormals )
		dot = ReplicateX4( CONSTANT_DOT );
	else
		dot = NegSIMD( pNormals[0] * dl->light.normal );

	dot = MaxSIMD( dot, Four_Zeros );
	int zeroMask = TestSignSIMD ( CmpEqSIMD( dot, Four_Zeros ) );
	if (zeroMask == 0xF)
		return;

	int nsamples = 1;
	if ( g_SunAngularExtent > 0.0f )
	{
		nsamples = NSAMPLES_SUN_AREA_LIGHT;
		if ( do_fast || force_fast )
			nsamples /= 4;
	}

	fltx4 totalFractionVisible = Four_Zeros;
	fltx4 fractionVisible = Four_Zeros;

	DirectionalSampler_t sampler;

	for ( int d = 0; d < nsamples; d++ )
	{
		// determine visibility of skylight
		// serach back to see if we can hit a sky brush
		Vector delta;
		VectorScale( dl->light.normal, -MAX_TRACE_LENGTH, delta );
		if ( d )
		{
			// jitter light source location
			Vector ofs = sampler.NextValue();
			ofs *= MAX_TRACE_LENGTH * g_SunAngularExtent;
			delta += ofs;
		}
		FourVectors delta4;
		delta4.DuplicateVector ( delta );
		delta4 += pos;

		TestLine_DoesHitSky ( pos, delta4, &fractionVisible, true, static_prop_index_to_ignore );

		totalFractionVisible = AddSIMD ( totalFractionVisible, fractionVisible );
	}

	fltx4 seeAmount = MulSIMD ( totalFractionVisible, ReplicateX4 ( 1.0f / nsamples ) );
	out.m_flDot[0] = MulSIMD ( dot, seeAmount );
	out.m_flFalloff = Four_Ones;
	out.m_flSunAmount = MulSIMD ( seeAmount, ReplicateX4( 10000.0f ) );
	for ( int i = 1; i < normalCount; i++ )
	{
		if ( bIgnoreNormals )
			out.m_flDot[i] = ReplicateX4 ( CONSTANT_DOT );
		else
		{
			out.m_flDot[i] = NegSIMD( pNormals[i] * dl->light.normal );
			out.m_flDot[i] = MulSIMD( out.m_flDot[i], seeAmount );
		}
	}
}

// Helper function - gathers light from ambient sky light
void GatherSampleAmbientSkySSE( SSE_sampleLightOutput_t &out, directlight_t *dl, int facenum, 
							   FourVectors const& pos, FourVectors *pNormals, int normalCount, int iThread,
							   int nLFlags, int static_prop_index_to_ignore,
							   float flEpsilon )
{

	bool bIgnoreNormals = ( nLFlags & GATHERLFLAGS_IGNORE_NORMALS ) != 0;
	bool force_fast = ( nLFlags & GATHERLFLAGS_FORCE_FAST ) != 0;

	fltx4 sumdot = Four_Zeros;
	fltx4 ambient_intensity[NUM_BUMP_VECTS+1];
	fltx4 possibleHitCount[NUM_BUMP_VECTS+1];
	fltx4 dots[NUM_BUMP_VECTS+1];

	for ( int i = 0; i < normalCount; i++ )
	{
		ambient_intensity[i] = Four_Zeros;
		possibleHitCount[i] = Four_Zeros;
	}

	DirectionalSampler_t sampler;
	int nsky_samples = NUMVERTEXNORMALS;
	if (do_fast || force_fast )
		nsky_samples /= 4;
	else
		nsky_samples *= g_flSkySampleScale;

	for (int j = 0; j < nsky_samples; j++)
	{
		FourVectors anorm;
		anorm.DuplicateVector( sampler.NextValue() );

		if ( bIgnoreNormals )
			dots[0] = ReplicateX4( CONSTANT_DOT );
		else
			dots[0] = NegSIMD( pNormals[0] * anorm );

		fltx4 validity = CmpGtSIMD( dots[0], ReplicateX4( EQUAL_EPSILON ) );

		// No possibility of anybody getting lit
		if ( !TestSignSIMD( validity ) )
			continue;

		dots[0] = AndSIMD( validity, dots[0] );
		sumdot = AddSIMD( dots[0], sumdot );
		possibleHitCount[0] = AddSIMD( AndSIMD( validity, Four_Ones ), possibleHitCount[0] );

		for ( int i = 1; i < normalCount; i++ )
		{
			if ( bIgnoreNormals )
				dots[i] = ReplicateX4( CONSTANT_DOT );
			else
				dots[i] = NegSIMD( pNormals[i] * anorm );
			fltx4 validity2 = CmpGtSIMD( dots[i], ReplicateX4 ( EQUAL_EPSILON ) );
			dots[i] = AndSIMD( validity2, dots[i] );
			possibleHitCount[i] = AddSIMD( AndSIMD( AndSIMD( validity, validity2 ), Four_Ones ), possibleHitCount[i] );
		}

		// search back to see if we can hit a sky brush
		FourVectors delta = anorm;
		delta *= -MAX_TRACE_LENGTH;
		delta += pos;
		FourVectors surfacePos = pos;
		FourVectors offset = anorm;
		offset *= -flEpsilon;
		surfacePos -= offset;

		fltx4 fractionVisible = Four_Ones;
		TestLine_DoesHitSky( surfacePos, delta, &fractionVisible, true, static_prop_index_to_ignore );
		for ( int i = 0; i < normalCount; i++ )
		{
			fltx4 addedAmount = MulSIMD( fractionVisible, dots[i] );
			ambient_intensity[i] = AddSIMD( ambient_intensity[i], addedAmount );
		}

	}

	out.m_flFalloff = Four_Ones;
	for ( int i = 0; i < normalCount; i++ )
	{
		// now scale out the missing parts of the hemisphere of this bump basis vector
		fltx4 factor = ReciprocalSIMD( possibleHitCount[0] );
		factor = MulSIMD( factor, possibleHitCount[i] );
		out.m_flDot[i] = MulSIMD( factor, sumdot );
		out.m_flDot[i] = ReciprocalSIMD( out.m_flDot[i] );
		out.m_flDot[i] = MulSIMD( ambient_intensity[i], out.m_flDot[i] );
	}

}

// Helper function - gathers light from area lights, spot lights, and point lights
void GatherSampleStandardLightSSE( SSE_sampleLightOutput_t &out, directlight_t *dl, int facenum, 
								  FourVectors const& pos, FourVectors *pNormals, int normalCount, int iThread,
								  int nLFlags, int static_prop_index_to_ignore,
								  float flEpsilon )
{
	bool bIgnoreNormals = ( nLFlags & GATHERLFLAGS_IGNORE_NORMALS ) != 0;

	FourVectors src;
	src.DuplicateVector( vec3_origin );

	if (dl->facenum == -1)
	{
		src.DuplicateVector( dl->light.origin );
	}

	// Find light vector
	FourVectors delta;
	delta = src;
	delta -= pos;
	fltx4 dist2 = delta.length2();
	fltx4 rpcDist = ReciprocalSqrtSIMD( dist2 );
	delta *= rpcDist;
	fltx4 dist = SqrtEstSIMD( dist2 );//delta.VectorNormalize();

	// Compute dot
	fltx4 dot = ReplicateX4( (float) CONSTANT_DOT );
	if ( !bIgnoreNormals )
		dot = delta * pNormals[0];
	dot = MaxSIMD( Four_Zeros, dot );

	// Affix dot to zero if past fade distz
	bool bHasHardFalloff = ( dl->m_flEndFadeDistance > dl->m_flStartFadeDistance );
	if ( bHasHardFalloff )
	{
		fltx4 notPastFadeDist = CmpLeSIMD ( dist, ReplicateX4 ( dl->m_flEndFadeDistance ) );
		dot = AndSIMD( dot, notPastFadeDist );  // dot = 0 if past fade distance
		if ( !TestSignSIMD ( notPastFadeDist ) )
			return;
	}

	dist = MaxSIMD( dist, Four_Ones );
	fltx4 falloffEvalDist = MinSIMD( dist, ReplicateX4( dl->m_flCapDist ) );

	fltx4 constant, linear, quadratic;
	fltx4 dot2, inCone, inFringe, mult;
	FourVectors offset;

	switch (dl->light.type)
	{
	case emit_point:
		constant  = ReplicateX4( dl->light.constant_attn );
		linear    = ReplicateX4( dl->light.linear_attn );
		quadratic = ReplicateX4( dl->light.quadratic_attn );

		out.m_flFalloff = MulSIMD( falloffEvalDist, falloffEvalDist );
		out.m_flFalloff = MulSIMD( out.m_flFalloff, quadratic );
		out.m_flFalloff = AddSIMD( out.m_flFalloff, MulSIMD( linear, falloffEvalDist ) );
		out.m_flFalloff = AddSIMD( out.m_flFalloff, constant );
		out.m_flFalloff = ReciprocalSIMD( out.m_flFalloff );
		break;

	case emit_surface:
		dot2 = delta * dl->light.normal;
		dot2 = NegSIMD( dot2 );

		// Light behind surface yields zero dot
		dot2 = MaxSIMD( Four_Zeros, dot2 );
		if ( TestSignSIMD( CmpEqSIMD( Four_Zeros, dot ) ) == 0xF )
			return;

		out.m_flFalloff = ReciprocalSIMD ( dist2 );
		out.m_flFalloff = MulSIMD( out.m_flFalloff, dot2 );

		// move the endpoint away from the surface by epsilon to prevent hitting the surface with the trace
		offset.DuplicateVector ( dl->light.normal );
		offset *= DIST_EPSILON;
		src += offset;
		break;

	case emit_spotlight:
		dot2 = delta * dl->light.normal;
		dot2 = NegSIMD( dot2 );

		// Affix dot2 to zero if outside light cone
		inCone = CmpGtSIMD( dot2, ReplicateX4( dl->light.stopdot2 ) );
		if ( !TestSignSIMD ( inCone ) )
			return;
		dot = AndSIMD( inCone, dot );

		constant  = ReplicateX4( dl->light.constant_attn );
		linear    = ReplicateX4( dl->light.linear_attn );
		quadratic = ReplicateX4( dl->light.quadratic_attn );

		out.m_flFalloff = MulSIMD( falloffEvalDist, falloffEvalDist );
		out.m_flFalloff = MulSIMD( out.m_flFalloff, quadratic );
		out.m_flFalloff = AddSIMD( out.m_flFalloff, MulSIMD( linear, falloffEvalDist ) );
		out.m_flFalloff = AddSIMD( out.m_flFalloff, constant );
		out.m_flFalloff = ReciprocalSIMD( out.m_flFalloff );
		out.m_flFalloff = MulSIMD( out.m_flFalloff, dot2 );

		// outside the inner cone
		inFringe = CmpLeSIMD( dot2, ReplicateX4( dl->light.stopdot ) );
		mult = ReplicateX4( dl->light.stopdot - dl->light.stopdot2 );
		mult = ReciprocalSIMD( mult );
		mult = MulSIMD( mult, SubSIMD( dot2, ReplicateX4( dl->light.stopdot2 ) ) );
		mult = MinSIMD( mult, Four_Ones );
		mult = MaxSIMD( mult, Four_Zeros );

		// pow is fixed point, so this isn't the most accurate, but it doesn't need to be
		if ( (dl->light.exponent != 0.0f ) && ( dl->light.exponent != 1.0f ) )
			mult = PowSIMD( mult, dl->light.exponent );

		// if not in between inner and outer cones, mult by 1
		mult = AndSIMD( inFringe, mult );
		mult = AddSIMD( mult, AndNotSIMD( inFringe, Four_Ones ) );
		out.m_flFalloff = MulSIMD( mult, out.m_flFalloff );
		break;

	}

	// we may be in the fade region - modulate lighting by the fade curve
	//float t = ( dist - dl->m_flStartFadeDistance ) / 
	//	( dl->m_flEndFadeDistance - dl->m_flStartFadeDistance );
	if ( bHasHardFalloff )
	{
		fltx4 t = ReplicateX4( dl->m_flEndFadeDistance - dl->m_flStartFadeDistance );
		t = ReciprocalSIMD( t );
		t = MulSIMD( t, SubSIMD( dist, ReplicateX4( dl->m_flStartFadeDistance ) ) );

		// clamp t to [0...1]
		t = MinSIMD( t, Four_Ones );
		t = MaxSIMD( t, Four_Zeros );
		t = SubSIMD( Four_Ones, t );

		// Using QuinticInterpolatingPolynomial, SSE-ified
		// t * t * t *( t * ( t* 6.0 - 15.0 ) + 10.0 )
		mult = SubSIMD( MulSIMD( ReplicateX4( 6.0f ), t ), ReplicateX4( 15.0f ) );
		mult = AddSIMD( MulSIMD( mult, t ), ReplicateX4( 10.0f ) );
		mult = MulSIMD( MulSIMD( t, t), mult );
		mult = MulSIMD( t, mult );
		out.m_flFalloff = MulSIMD( mult, out.m_flFalloff );
	}

	// Raytrace for visibility function
	fltx4 fractionVisible = Four_Ones;
	TestLine( pos, src, &fractionVisible, static_prop_index_to_ignore);
	dot = MulSIMD( fractionVisible, dot );
	out.m_flDot[0] = dot;

	for ( int i = 1; i < normalCount; i++ )
	{
		if ( bIgnoreNormals )
			out.m_flDot[i] = ReplicateX4( (float) CONSTANT_DOT );
		else
		{
			out.m_flDot[i] = pNormals[i] * delta;
			out.m_flDot[i] = MaxSIMD( Four_Zeros, out.m_flDot[i] );
		}
	}
}

// returns dot product with normal and delta
// dl - light
// pos - position of sample
// normal - surface normal of sample
// out.m_flDot[] - returned dot products with light vector and each normal
// out.m_flFalloff - amount of light falloff
void GatherSampleLightSSE( SSE_sampleLightOutput_t &out, directlight_t *dl, int facenum, 
					   FourVectors const& pos, FourVectors *pNormals, int normalCount, int iThread,
					   int nLFlags,
					   int static_prop_index_to_ignore,
					   float flEpsilon )
{
	for ( int b = 0; b < normalCount; b++ )
		out.m_flDot[b] = Four_Zeros;
	out.m_flFalloff = Four_Zeros;
	out.m_flSunAmount = Four_Zeros;
	Assert( normalCount <= (NUM_BUMP_VECTS+1) );

	// skylights work fundamentally differently than normal lights
	switch( dl->light.type )
	{
	case emit_skylight:
		GatherSampleSkyLightSSE( out, dl, facenum, pos, pNormals, normalCount,
		                         iThread, nLFlags, static_prop_index_to_ignore, flEpsilon );
		break;
	case emit_skyambient:
		GatherSampleAmbientSkySSE( out, dl, facenum, pos, pNormals, normalCount,
		                           iThread, nLFlags, static_prop_index_to_ignore, flEpsilon );
		break;
	case emit_point:
	case emit_surface:
	case emit_spotlight:
		GatherSampleStandardLightSSE( out, dl, facenum, pos, pNormals, normalCount,
		                              iThread, nLFlags, static_prop_index_to_ignore, flEpsilon );
		break;
	default:
		Error ("Bad dl->light.type");
		return;
	}

	// NOTE: Notice here that if the light is on the back side of the face
	// (tested by checking the dot product of the face normal and the light position)
	// we don't want it to contribute to *any* of the bumped lightmaps. It glows
	// in disturbing ways if we don't do this.
	out.m_flDot[0] = MaxSIMD ( out.m_flDot[0], Four_Zeros );
	fltx4 notZero = CmpGtSIMD( out.m_flDot[0], Four_Zeros );
	for ( int n = 1; n < normalCount; n++ )
	{
		out.m_flDot[n] = MaxSIMD( out.m_flDot[n], Four_Zeros );
		out.m_flDot[n] = AndSIMD( out.m_flDot[n], notZero );
	}

}

/*
  =============
  AddSampleToPatch

  Take the sample's collected light and
  add it back into the apropriate patch
  for the radiosity pass.
  =============
*/
void AddSampleToPatch (sample_t *s, LightingValue_t& light, int facenum)
{
	CPatch	*patch;
	Vector	mins, maxs;
	int		i;

	if (numbounce == 0)
		return;
	if( VectorAvg( light.m_vecLighting ) < 1)
		return;

	//
	// fixed the sample position and normal -- need to find the equiv pos, etc to set up 
	// patches
	//
	if( g_FacePatches.Element( facenum ) == g_FacePatches.InvalidIndex() )
		return;

	float radius = sqrt( s->area ) / 2.0;

	CPatch *pNextPatch = NULL;
	for( patch = &g_Patches.Element( g_FacePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != g_Patches.InvalidIndex() )
		{
			pNextPatch = &g_Patches.Element( patch->ndxNext );
		}

		if (patch->sky)
			continue;

		// skip patches with children
		if ( patch->child1 != g_Patches.InvalidIndex() )
		 	continue;

		// see if the point is in this patch (roughly)
		WindingBounds (patch->winding, mins, maxs);

		for (i=0 ; i<3 ; i++)
		{
			if (mins[i] > s->pos[i] + radius)
				goto nextpatch;
			if (maxs[i] < s->pos[i] - radius)
				goto nextpatch;
		}

		// add the sample to the patch
		patch->samplearea += s->area;
		VectorMA( patch->samplelight, s->area, light.m_vecLighting, patch->samplelight );

 nextpatch:;
	}
	// don't worry if some samples don't find a patch
}


void GetPhongNormal( int facenum, Vector const& spot, Vector& phongnormal )
{
	int	j;
	dface_t		*f = &g_pFaces[facenum];
//	dplane_t	*p = &dplanes[f->planenum];
	Vector		facenormal, vspot;

	VectorCopy( dplanes[f->planenum].normal, facenormal );
	VectorCopy( facenormal, phongnormal );

	if ( smoothing_threshold != 1 )
	{
		faceneighbor_t *fn = &faceneighbor[facenum];

		// Calculate modified point normal for surface
		// Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
		// Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
		// Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
		// Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

		for (j=0 ; j<f->numedges ; j++)
		{
			Vector	v1, v2;
			//int e = dsurfedges[f->firstedge + j];
			//int e1 = dsurfedges[f->firstedge + ((j+f->numedges-1)%f->numedges)];
			//int e2 = dsurfedges[f->firstedge + ((j+1)%f->numedges)];

			//edgeshare_t	*es = &edgeshare[abs(e)];
			//edgeshare_t	*es1 = &edgeshare[abs(e1)];
			//edgeshare_t	*es2 = &edgeshare[abs(e2)];
			// dface_t	*f2;
			float		a1, a2, aa, bb, ab;
			int			vert1, vert2;

			Vector& n1 = fn->normal[j];
			Vector& n2 = fn->normal[(j+1)%f->numedges];

			/*
			  if (VectorCompare( n1, fn->facenormal ) 
			  && VectorCompare( n2, fn->facenormal) )
			  continue;
			*/

			vert1 = EdgeVertex( f, j );
			vert2 = EdgeVertex( f, j+1 );

			Vector& p1 = dvertexes[vert1].point;
			Vector& p2 = dvertexes[vert2].point;

			// Build vectors from the middle of the face to the edge vertexes and the sample pos.
			VectorSubtract( p1, face_centroids[facenum], v1 );
			VectorSubtract( p2, face_centroids[facenum], v2 );
			VectorSubtract( spot, face_centroids[facenum], vspot );
			aa = DotProduct( v1, v1 );
			bb = DotProduct( v2, v2 );
			ab = DotProduct( v1, v2 );
			a1 = (bb * DotProduct( v1, vspot ) - ab * DotProduct( vspot, v2 )) / (aa * bb - ab * ab);
			a2 = (DotProduct( vspot, v2 ) - a1 * ab) / bb;

			// Test center to sample vector for inclusion between center to vertex vectors (Use dot product of vectors)
			if ( a1 >= 0.0 && a2 >= 0.0)
			{
				// calculate distance from edge to pos
				Vector	temp;
				float scale;
				
				// Interpolate between the center and edge normals based on sample position
				scale = 1.0 - a1 - a2;
				VectorScale( fn->facenormal, scale, phongnormal );
				VectorScale( n1, a1, temp );
				VectorAdd( phongnormal, temp, phongnormal );
				VectorScale( n2, a2, temp );
				VectorAdd( phongnormal, temp, phongnormal );
				Assert( VectorLength( phongnormal ) > 1.0e-20 );
				VectorNormalize( phongnormal );

				/*
				  if (a1 > 1 || a2 > 1 || a1 + a2 > 1)
				  {
				  Msg("\n%.2f %.2f\n", a1, a2 );
				  Msg("%.2f %.2f %.2f\n", v1[0], v1[1], v1[2] );
				  Msg("%.2f %.2f %.2f\n", v2[0], v2[1], v2[2] );
				  Msg("%.2f %.2f %.2f\n", vspot[0], vspot[1], vspot[2] );
				  exit(1);

				  a1 = 0;
				  }
				*/
				/*
				  phongnormal[0] = (((j + 1) & 4) != 0) * 255;
				  phongnormal[1] = (((j + 1) & 2) != 0) * 255;
				  phongnormal[2] = (((j + 1) & 1) != 0) * 255;
				*/
				return;
			}
		}
	}
}

void GetPhongNormal( int facenum, FourVectors const& spot, FourVectors& phongnormal )
{
	int	j;
	dface_t		*f = &g_pFaces[facenum];
	//	dplane_t	*p = &dplanes[f->planenum];
	Vector		facenormal;
	FourVectors vspot;

	VectorCopy( dplanes[f->planenum].normal, facenormal );
	phongnormal.DuplicateVector( facenormal );

	FourVectors faceCentroid;
	faceCentroid.DuplicateVector( face_centroids[facenum] );

	if ( smoothing_threshold != 1 )
	{
		faceneighbor_t *fn = &faceneighbor[facenum];

		// Calculate modified point normal for surface
		// Use the edge normals iff they are defined.  Bend the surface towards the edge normal(s)
		// Crude first attempt: find nearest edge normal and do a simple interpolation with facenormal.
		// Second attempt: find edge points+center that bound the point and do a three-point triangulation(baricentric)
		// Better third attempt: generate the point normals for all vertices and do baricentric triangulation.

		for ( j = 0; j < f->numedges; ++j )
		{
			Vector	v1, v2;
			fltx4		a1, a2;
			float		aa, bb, ab;
			int			vert1, vert2;

			Vector& n1 = fn->normal[j];
			Vector& n2 = fn->normal[(j+1)%f->numedges];

			vert1 = EdgeVertex( f, j );
			vert2 = EdgeVertex( f, j+1 );

			Vector& p1 = dvertexes[vert1].point;
			Vector& p2 = dvertexes[vert2].point;

			// Build vectors from the middle of the face to the edge vertexes and the sample pos.
			VectorSubtract( p1, face_centroids[facenum], v1 );
			VectorSubtract( p2, face_centroids[facenum], v2 );
			//VectorSubtract( spot, face_centroids[facenum], vspot );
			vspot = spot;
			vspot -= faceCentroid;
			aa = DotProduct( v1, v1 );
			bb = DotProduct( v2, v2 );
			ab = DotProduct( v1, v2 );
			//a1 = (bb * DotProduct( v1, vspot ) - ab * DotProduct( vspot, v2 )) / (aa * bb - ab * ab);
			a1 = ReciprocalSIMD( ReplicateX4( aa * bb - ab * ab ) );
			a1 = MulSIMD( a1, SubSIMD( MulSIMD( ReplicateX4( bb ), vspot * v1 ), MulSIMD( ReplicateX4( ab ), vspot * v2 ) ) );
			//a2 = (DotProduct( vspot, v2 ) - a1 * ab) / bb;
			a2 = ReciprocalSIMD( ReplicateX4( bb ) );
			a2 = MulSIMD( a2, SubSIMD( vspot * v2, MulSIMD( a1, ReplicateX4( ab ) ) ) );

			fltx4 resultMask = AndSIMD( CmpGeSIMD( a1, Four_Zeros ), CmpGeSIMD( a2, Four_Zeros ) );
			
			if ( !TestSignSIMD( resultMask ) )
				continue;

			// Store the old phong normal to avoid overwriting already computed phong normals
			FourVectors oldPhongNormal = phongnormal;

			// calculate distance from edge to pos
			FourVectors	temp;
			fltx4 scale;

			// Interpolate between the center and edge normals based on sample position
			scale = SubSIMD( SubSIMD( Four_Ones, a1 ), a2 );
			phongnormal.DuplicateVector( fn->facenormal );
			phongnormal *= scale;
			temp.DuplicateVector( n1 );
			temp *= a1;
			phongnormal += temp;
			temp.DuplicateVector( n2 );
			temp *= a2;
			phongnormal += temp;

			// restore the old phong normals
			phongnormal.x = AddSIMD( AndSIMD( resultMask, phongnormal.x ), AndNotSIMD( resultMask, oldPhongNormal.x ) );
			phongnormal.y = AddSIMD( AndSIMD( resultMask, phongnormal.y ), AndNotSIMD( resultMask, oldPhongNormal.y ) );
			phongnormal.z = AddSIMD( AndSIMD( resultMask, phongnormal.z ), AndNotSIMD( resultMask, oldPhongNormal.z ) );
		}

		phongnormal.VectorNormalize();
	}
}



int GetVisCache( int lastoffset, int cluster, byte *pvs )
{
	// get the PVS for the pos to limit the number of checks
    if ( !visdatasize )
    {       
        memset (pvs, 255, (dvis->numclusters+7)/8 );
        lastoffset = -1;
    }
    else 
    {
		if (cluster < 0)
		{
			// Error, point embedded in wall
			// sampled[0][1] = 255;
			memset (pvs, 255, (dvis->numclusters+7)/8 );
			lastoffset = -1;
		}
		else
		{
			int thisoffset = dvis->bitofs[ cluster ][DVIS_PVS];
			if ( thisoffset != lastoffset )
			{ 
				if ( thisoffset == -1 )
				{
					Error ("visofs == -1");
				}

				DecompressVis (&dvisdata[thisoffset], pvs);
			}
			lastoffset = thisoffset;
		}
    }
	return lastoffset;
}


void BuildPatchLights( int facenum );

void DumpSamples( int ndxFace, facelight_t *pFaceLight )
{
	ThreadLock();

	dface_t *pFace = &g_pFaces[ndxFace];
	if( pFace )
	{
		bool bBumpped = ( ( texinfo[pFace->texinfo].flags & SURF_BUMPLIGHT ) != 0 );

		for( int iStyle = 0; iStyle < 4; ++iStyle )
		{
			if( pFace->styles[iStyle] != 255 )
			{
				for ( int iBump = 0; iBump < 4; ++iBump )
				{
					if ( iBump == 0 || ( iBump > 0 && bBumpped ) )
					{
						for( int iSample = 0; iSample < pFaceLight->numsamples; ++iSample )
						{
							sample_t *pSample = &pFaceLight->sample[iSample];
							WriteWinding( pFileSamples[iStyle][iBump], pSample->w, pFaceLight->light[iStyle][iBump][iSample].m_vecLighting );
							if( bDumpNormals )
							{
								WriteNormal( pFileSamples[iStyle][iBump], pSample->pos, pSample->normal, 15.0f, pSample->normal * 255.0f );
							}
						}
					}
				}
			}
		}
	}

	ThreadUnlock();
}


//-----------------------------------------------------------------------------
// Allocates light sample data
//-----------------------------------------------------------------------------
static inline void AllocateLightstyleSamples( facelight_t* fl, int styleIndex, int numnormals )
{
	for (int n = 0; n < numnormals; ++n)
	{
		fl->light[styleIndex][n] = ( LightingValue_t* )calloc( fl->numsamples, sizeof(LightingValue_t ) );
	}
}


//-----------------------------------------------------------------------------
// Used to find an existing lightstyle on a face
//-----------------------------------------------------------------------------
static inline int FindLightstyle( dface_t* f, int lightstyle )
{
 	for (int k = 0; k < MAXLIGHTMAPS; k++)
	{
		if (f->styles[k] == lightstyle)
			return k;
	}

	return -1;
}

static int FindOrAllocateLightstyleSamples( dface_t* f, facelight_t	*fl, int lightstyle, int numnormals )
{
	// Search the lightstyles associated with the face for a match
	int k;
 	for (k = 0; k < MAXLIGHTMAPS; k++)
	{
		if (f->styles[k] == lightstyle)
			break;

		// Found an empty entry, we can use it for a new lightstyle
		if (f->styles[k] == 255)
		{
			AllocateLightstyleSamples( fl, k, numnormals );
			f->styles[k] = lightstyle;
			break;
		}
	}

	// Check for overflow
	if (k >= MAXLIGHTMAPS)
		return -1;

	return k;
}


//-----------------------------------------------------------------------------
// Compute the illumination point + normal for the sample
//-----------------------------------------------------------------------------
static void ComputeIlluminationPointAndNormalsSSE( lightinfo_t const& l, FourVectors const &pos, FourVectors const &norm, SSE_SampleInfo_t* pInfo, int numSamples )
{

	Vector v[4];

	pInfo->m_Points = pos;
	bool computeNormals = ( pInfo->m_NormalCount > 1 && ( pInfo->m_IsDispFace || !l.isflat ) );

	// FIXME: move sample point off the surface a bit, this is done so that
	// light sampling will not be affected by a bug	where raycasts will
	// intersect with the face being lit. We really should just have that
	// logic in GatherSampleLight
	FourVectors faceNormal;
	faceNormal.DuplicateVector( l.facenormal );
	pInfo->m_Points += faceNormal;

	if ( pInfo->m_IsDispFace )
	{
		pInfo->m_PointNormals[0] = norm;
	}
	else if ( !l.isflat )
	{
		// If the face isn't flat, use a phong-based normal instead
		FourVectors modelorg;
		modelorg.DuplicateVector( l.modelorg );
		FourVectors vecSample = pos;
		vecSample -= modelorg;
		GetPhongNormal( pInfo->m_FaceNum, vecSample, pInfo->m_PointNormals[0] );
	}

	if ( computeNormals )
	{
		Vector bv[4][NUM_BUMP_VECTS];
		for ( int i = 0; i < 4; ++i )
		{
			// TODO: using Vec may slow things down a bit
			GetBumpNormals( pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[0],
			                pInfo->m_pTexInfo->textureVecsTexelsPerWorldUnits[1],
			                l.facenormal, pInfo->m_PointNormals[0].Vec( i ), bv[i] );
		}
		for ( int b = 0; b < NUM_BUMP_VECTS; ++b )
		{
			pInfo->m_PointNormals[b+1].LoadAndSwizzle ( bv[0][b], bv[1][b], bv[2][b], bv[3][b] );
		}
	}

	// TODO: this may slow things down a bit ( using Vec )
	for ( int i = 0; i < 4; ++i )
		pInfo->m_Clusters[i] = ClusterFromPoint( pos.Vec( i ) );
}

//-----------------------------------------------------------------------------
// Iterates over all lights and computes lighting at up to 4 sample points
//-----------------------------------------------------------------------------
static void GatherSampleLightAt4Points( SSE_SampleInfo_t& info, int sampleIdx, int numSamples )
{
	SSE_sampleLightOutput_t out;

	// Iterate over all direct lights and add them to the particular sample
	for (directlight_t *dl = activelights; dl != NULL; dl = dl->next)
	{	    
		// is this lights cluster visible?
		fltx4 dotMask = Four_Zeros;
		bool skipLight = true;
		for( int s = 0; s < numSamples; s++ )
		{
			if( PVSCheck( dl->pvs, info.m_Clusters[s] ) )
			{
				dotMask = SetComponentSIMD( dotMask, s, 1.0f );
				skipLight = false;
			}
		}
		if ( skipLight )
			continue;

		GatherSampleLightSSE( out, dl, info.m_FaceNum, info.m_Points, info.m_PointNormals, info.m_NormalCount, info.m_iThread );
		
		// Apply the PVS check filter and compute falloff x dot
		fltx4 fxdot[NUM_BUMP_VECTS + 1];
		skipLight = true;
		for ( int b = 0; b < info.m_NormalCount; b++ )
		{
			fxdot[b] = MulSIMD( out.m_flDot[b], dotMask );
			fxdot[b] = MulSIMD( fxdot[b], out.m_flFalloff );
			if ( !IsAllZeros( fxdot[b] ) )
			{
				skipLight = false;
			}
		}
		if ( skipLight )
			continue;

		// Figure out the lightstyle for this particular sample
		int lightStyleIndex = FindOrAllocateLightstyleSamples( info.m_pFace, info.m_pFaceLight, 
			dl->light.style, info.m_NormalCount );
		if (lightStyleIndex < 0)
		{
			if (info.m_WarnFace != info.m_FaceNum)
			{
				Warning ("\nWARNING: Too many light styles on a face at (%f, %f, %f)\n",
					info.m_Points.x.m128_f32[0], info.m_Points.y.m128_f32[0], info.m_Points.z.m128_f32[0] );
				info.m_WarnFace = info.m_FaceNum;
			}
			continue;
		}

		// pLightmaps is an array of the lightmaps for each normal direction,
		// here's where the result of the sample gathering goes
		LightingValue_t** pLightmaps = info.m_pFaceLight->light[lightStyleIndex];

		// Incremental lighting only cares about lightstyle zero
		if( g_pIncremental && (dl->light.style == 0) )
		{
			for ( int i = 0; i < numSamples; i++ )
			{
				g_pIncremental->AddLightToFace( dl->m_IncrementalID, info.m_FaceNum, sampleIdx + i, 
					info.m_LightmapSize, SubFloat( fxdot[0], i ), info.m_iThread );
			}
		}

		for( int n = 0; n < info.m_NormalCount; ++n )
		{
			for ( int i = 0; i < numSamples; i++ )
			{
				pLightmaps[n][sampleIdx + i].AddLight( SubFloat( fxdot[n], i ), dl->light.intensity, SubFloat( out.m_flSunAmount, i ) );
			}
		}
	}
}



//-----------------------------------------------------------------------------
// Iterates over all lights and computes lighting at a sample point
//-----------------------------------------------------------------------------
static void ResampleLightAt4Points( SSE_SampleInfo_t& info, int lightStyleIndex, int flags, LightingValue_t pLightmap[4][NUM_BUMP_VECTS+1] )
{
	SSE_sampleLightOutput_t out;

	// Clear result
	for ( int i = 0; i < 4; ++i )
	{
		for ( int n = 0; n < info.m_NormalCount; ++n )
		{
			pLightmap[i][n].Zero();
		}
	}

	// Iterate over all direct lights and add them to the particular sample
	for (directlight_t *dl = activelights; dl != NULL; dl = dl->next)
	{
		if ((flags & AMBIENT_ONLY) && (dl->light.type != emit_skyambient))
			continue;

		if ((flags & NON_AMBIENT_ONLY) && (dl->light.type == emit_skyambient))
			continue;

		// Only add contributions that match the lightstyle 
		Assert( lightStyleIndex <= MAXLIGHTMAPS );
		Assert( info.m_pFace->styles[lightStyleIndex] != 255 );
		if (dl->light.style != info.m_pFace->styles[lightStyleIndex])
			continue;

		// is this lights cluster visible?
		fltx4 dotMask = Four_Zeros;
		bool skipLight = true;
		for( int s = 0; s < 4; s++ )
		{
			if( PVSCheck( dl->pvs, info.m_Clusters[s] ) )
			{
				dotMask = SetComponentSIMD( dotMask, s, 1.0f );
				skipLight = false;
			}
		}
		if ( skipLight )
			continue;

		// NOTE: Notice here that if the light is on the back side of the face
		// (tested by checking the dot product of the face normal and the light position)
		// we don't want it to contribute to *any* of the bumped lightmaps. It glows
		// in disturbing ways if we don't do this.
		GatherSampleLightSSE( out, dl, info.m_FaceNum, info.m_Points, info.m_PointNormals, info.m_NormalCount, info.m_iThread );

		// Apply the PVS check filter and compute falloff x dot
		fltx4 fxdot[NUM_BUMP_VECTS + 1];
		for ( int b = 0; b < info.m_NormalCount; b++ )
		{
			fxdot[b] = MulSIMD( out.m_flFalloff, out.m_flDot[b] );
			fxdot[b] = MulSIMD( fxdot[b], dotMask );
		}

		// Compute the contributions to each of the bumped lightmaps
		// The first sample is for non-bumped lighting.
		// The other sample are for bumpmapping.
		for( int i = 0; i < 4; ++i )
		{
			for( int n = 0; n < info.m_NormalCount; ++n )
			{
				pLightmap[i][n].AddLight( SubFloat( fxdot[n], i ), dl->light.intensity, SubFloat( out.m_flSunAmount, i ) );
			}
		}
	}
}

bool PointsInWinding ( FourVectors const & point, winding_t *w, int &invalidBits )
{
	FourVectors edge, toPt, cross, testCross, p0, p1;
	fltx4 invalidMask;

	//
	// get the first normal to test
	//
	p0.DuplicateVector( w->p[0] );
	p1.DuplicateVector( w->p[1] );
	toPt = point;
	toPt -= p0;
	edge = p1;
	edge -= p0;
	testCross = edge ^ toPt;
	testCross.VectorNormalizeFast();

	for( int ndxPt = 1; ndxPt < w->numpoints; ndxPt++ )
	{
		p0.DuplicateVector( w->p[ndxPt] );
		p1.DuplicateVector( w->p[(ndxPt+1)%w->numpoints] );
		toPt = point;
		toPt -= p0;
		edge = p1;
		edge -= p0;
		cross = edge ^ toPt;
		cross.VectorNormalizeFast();

		fltx4 dot = cross * testCross;
		invalidMask = OrSIMD( invalidMask, CmpLtSIMD( dot, Four_Zeros ) );

		invalidBits = TestSignSIMD ( invalidMask );
		if ( invalidBits == 0xF )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Perform supersampling at a particular point
//-----------------------------------------------------------------------------
static int SupersampleLightAtPoint( lightinfo_t& l, SSE_SampleInfo_t& info, 
									int sampleIndex, int lightStyleIndex, LightingValue_t *pLight, int flags )
{
	sample_t& sample = info.m_pFaceLight->sample[sampleIndex];

	// Get the position of the original sample in lightmapspace
	Vector2D temp;
	WorldToLuxelSpace( &l, sample.pos, temp );
	Vector sampleLightOrigin( temp[0], temp[1], 0.0f );

	// Some parameters related to supersampling
	float sampleWidth = ( flags & NON_AMBIENT_ONLY ) ? 4 : 2;
	float cscale = 1.0f / sampleWidth;
	float csshift = -((sampleWidth - 1) * cscale) / 2.0;

	// Clear out the light values
	for (int i = 0; i < info.m_NormalCount; ++i )
		pLight[i].Zero();

	int subsampleCount = 0;

	FourVectors superSampleNormal;
	superSampleNormal.DuplicateVector( sample.normal );

	FourVectors superSampleLightCoord;
	FourVectors superSamplePosition;

	if ( flags & NON_AMBIENT_ONLY )
	{
		float aRow[4];
		for ( int coord = 0; coord < 4; ++coord )
			aRow[coord] = csshift + coord * cscale;
		fltx4 sseRow = LoadUnalignedSIMD( aRow );

		for (int s = 0; s < 4; ++s)
		{
			// make sure the coordinate is inside of the sample's winding and when normalizing
			// below use the number of samples used, not just numsamples and some of them
			// will be skipped if they are not inside of the winding
			superSampleLightCoord.DuplicateVector( sampleLightOrigin );
			superSampleLightCoord.x = AddSIMD( superSampleLightCoord.x, ReplicateX4( aRow[s] ) );
			superSampleLightCoord.y = AddSIMD( superSampleLightCoord.y, sseRow );

			// Figure out where the supersample exists in the world, and make sure
			// it lies within the sample winding
			LuxelSpaceToWorld( &l, superSampleLightCoord[0], superSampleLightCoord[1], superSamplePosition );

			// A winding should exist only if the sample wasn't a uniform luxel, or if g_bDumpPatches is true.
			int invalidBits = 0;
			if ( sample.w && !PointsInWinding( superSamplePosition, sample.w, invalidBits ) )
				continue;

			// Compute the super-sample illumination point and normal
			// We're assuming the flat normal is the same for all supersamples
			ComputeIlluminationPointAndNormalsSSE( l, superSamplePosition, superSampleNormal, &info, 4 );

			// Resample the non-ambient light at this point...
			LightingValue_t result[4][NUM_BUMP_VECTS+1];
			ResampleLightAt4Points( info, lightStyleIndex, NON_AMBIENT_ONLY, result );

			// Got more subsamples
			for ( int i = 0; i < 4; i++ )
			{
				if ( !( ( invalidBits >> i ) & 0x1 ) )
				{
					for ( int n = 0; n < info.m_NormalCount; ++n )
					{
						pLight[n].AddLight( result[i][n] );
					}
					++subsampleCount;
				}
			}
		}
	}
	else
	{
		FourVectors superSampleOffsets;
		superSampleOffsets.LoadAndSwizzle( Vector( csshift, csshift, 0 ), Vector( csshift, csshift + cscale, 0),
		                                   Vector( csshift + cscale, csshift, 0 ), Vector( csshift + cscale, csshift + cscale, 0 ) );
		superSampleLightCoord.DuplicateVector( sampleLightOrigin );
		superSampleLightCoord += superSampleOffsets;

		LuxelSpaceToWorld( &l, superSampleLightCoord[0], superSampleLightCoord[1], superSamplePosition );

		int invalidBits = 0;
		if ( sample.w && !PointsInWinding( superSamplePosition, sample.w, invalidBits ) )
			return 0;

		ComputeIlluminationPointAndNormalsSSE( l, superSamplePosition, superSampleNormal, &info, 4 );

		LightingValue_t result[4][NUM_BUMP_VECTS+1];
		ResampleLightAt4Points( info, lightStyleIndex, AMBIENT_ONLY, result );

		// Got more subsamples
		for ( int i = 0; i < 4; i++ )
		{
			if ( !( ( invalidBits >> i ) & 0x1 ) )
			{
				for ( int n = 0; n < info.m_NormalCount; ++n )
				{
					pLight[n].AddLight( result[i][n] );
				}
				++subsampleCount;
			}
		}
	}

	return subsampleCount;
}


//-----------------------------------------------------------------------------
// Compute gradients of a lightmap
//-----------------------------------------------------------------------------
static void ComputeLightmapGradients( SSE_SampleInfo_t& info, bool const* pHasProcessedSample, 
									  float* pIntensity, float* gradient )
{
	int w = info.m_LightmapWidth;
	int h = info.m_LightmapHeight;
	facelight_t* fl = info.m_pFaceLight;

	for (int i=0 ; i<fl->numsamples ; i++)
	{
		// Don't supersample the same sample twice
		if (pHasProcessedSample[i])
			continue;

		gradient[i] = 0.0f;
		sample_t& sample = fl->sample[i];

		// Choose the maximum gradient of all bumped lightmap intensities
		for ( int n = 0; n < info.m_NormalCount; ++n )
		{
			int j = n * info.m_LightmapSize + sample.s + sample.t * w;

			if (sample.t > 0)
			{
				if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1-w] ) );
				gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-w] ) );
				if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1-w] ) );
			}
			if (sample.t < h-1)
			{
				if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1+w] ) );
				gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+w] ) );
				if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1+w] ) );
			}
			if (sample.s > 0)   gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j-1] ) );
			if (sample.s < w-1) gradient[i] = max( gradient[i], fabs( pIntensity[j] - pIntensity[j+1] ) );
		}
	}
}

//-----------------------------------------------------------------------------
// ComputeLuxelIntensity...
//-----------------------------------------------------------------------------
static inline void ComputeLuxelIntensity( SSE_SampleInfo_t& info, int sampleIdx, 
										  LightingValue_t **ppLightSamples, float* pSampleIntensity )
{
	// Compute a separate intensity for each
	sample_t& sample = info.m_pFaceLight->sample[sampleIdx];
	int destIdx = sample.s + sample.t * info.m_LightmapWidth;
	for (int n = 0; n < info.m_NormalCount; ++n)
	{
		float intensity = ppLightSamples[n][sampleIdx].Intensity();

		// convert to a linear perception space
		pSampleIntensity[n * info.m_LightmapSize + destIdx] = pow( intensity / 256.0, 1.0 / 2.2 );
	}
}

//-----------------------------------------------------------------------------
// Compute the maximum intensity based on all bumped lighting
//-----------------------------------------------------------------------------
static void ComputeSampleIntensities( SSE_SampleInfo_t& info, LightingValue_t **ppLightSamples, float* pSampleIntensity )
{
	for (int i=0; i<info.m_pFaceLight->numsamples; i++)
	{
		ComputeLuxelIntensity( info, i, ppLightSamples, pSampleIntensity );
	}
}

//-----------------------------------------------------------------------------
// Perform supersampling on a particular lightstyle
//-----------------------------------------------------------------------------
static void BuildSupersampleFaceLights( lightinfo_t& l, SSE_SampleInfo_t& info, int lightstyleIndex )
{
	LightingValue_t pAmbientLight[NUM_BUMP_VECTS+1];
	LightingValue_t pDirectLight[NUM_BUMP_VECTS+1];

	// This is used to make sure we don't supersample a light sample more than once
	int processedSampleSize = info.m_LightmapSize * sizeof(bool);
	bool* pHasProcessedSample = (bool*)stackalloc( processedSampleSize );
	memset( pHasProcessedSample, 0, processedSampleSize );

	// This is used to compute a simple gradient computation of the light samples
	// We're going to store the maximum intensity of all bumped samples at each sample location
	float* pGradient = (float*)stackalloc( info.m_pFaceLight->numsamples * sizeof(float) );
	float* pSampleIntensity = (float*)stackalloc( info.m_NormalCount * info.m_LightmapSize * sizeof(float) );

	// Compute the maximum intensity of all lighting associated with this lightstyle
	// for all bumped lighting
	LightingValue_t **ppLightSamples = info.m_pFaceLight->light[lightstyleIndex];
	ComputeSampleIntensities( info, ppLightSamples, pSampleIntensity );

	Vector *pVisualizePass = NULL;
	if (debug_extra)
	{
		int visualizationSize = info.m_pFaceLight->numsamples * sizeof(Vector);
		pVisualizePass = (Vector*)stackalloc( visualizationSize );
		memset( pVisualizePass, 0, visualizationSize ); 
	}

	// What's going on here is that we're looking for large lighting discontinuities
	// (large light intensity gradients) as a clue that we should probably be supersampling
	// in that area. Because the supersampling operation will cause lighting changes,
	// we've found that it's good to re-check the gradients again and see if any other
	// areas should be supersampled as a result of the previous pass. Keep going
	// until all the gradients are reasonable or until we hit a max number of passes
	bool do_anotherpass = true;
	int pass = 1;
	while (do_anotherpass && pass <= extrapasses)
	{
		// Look for lighting discontinuities to see what we should be supersampling
		ComputeLightmapGradients( info, pHasProcessedSample, pSampleIntensity, pGradient );

		do_anotherpass = false;

		// Now check all of the samples and supersample those which we have
		// marked as having high gradients
		for (int i=0 ; i<info.m_pFaceLight->numsamples; ++i)
		{
			// Don't supersample the same sample twice
			if (pHasProcessedSample[i])
				continue;

			// Don't supersample if the lighting is pretty uniform near the sample
			if (pGradient[i] < 0.0625)
				continue;

			// Joy! We're supersampling now, and we therefore must do another pass
			// Also, we need never bother with this sample again
			pHasProcessedSample[i] = true;
			do_anotherpass = true;

			if (debug_extra)
			{
				// Mark the little visualization bitmap with a color indicating
				// which pass it was updated on.
				pVisualizePass[i][0] = (pass & 1) * 255;
				pVisualizePass[i][1] = (pass & 2) * 128;
				pVisualizePass[i][2] = (pass & 4) * 64;
			}

			// Supersample the ambient light for each bump direction vector
			int ambientSupersampleCount = SupersampleLightAtPoint( l, info, i, lightstyleIndex, pAmbientLight, AMBIENT_ONLY );

			// Supersample the non-ambient light for each bump direction vector
			int directSupersampleCount = SupersampleLightAtPoint( l, info, i, lightstyleIndex, pDirectLight, NON_AMBIENT_ONLY );

			// Because of sampling problems, small area triangles may have no samples.
			// In this case, just use what we already have
			if ( ambientSupersampleCount > 0 && directSupersampleCount > 0 )
			{
				// Add the ambient + directional terms together, stick it back into the lightmap
				for (int n = 0; n < info.m_NormalCount; ++n)
				{
					ppLightSamples[n][i].Zero();
					ppLightSamples[n][i].AddWeighted( pDirectLight[n],1.0f / directSupersampleCount );
					ppLightSamples[n][i].AddWeighted( pAmbientLight[n], 1.0f / ambientSupersampleCount );
				}

				// Recompute the luxel intensity based on the supersampling
				ComputeLuxelIntensity( info, i, ppLightSamples, pSampleIntensity );
			}

		}

		// We've finished another pass
		pass++;
	}

	if (debug_extra)
	{
		// Copy colors representing which supersample pass the sample was messed with
		// into the actual lighting values so we can visualize it
		for (int i=0 ; i<info.m_pFaceLight->numsamples ; ++i)
		{
			for (int j = 0; j <info.m_NormalCount; ++j)
			{
				VectorCopy( pVisualizePass[i], ppLightSamples[j][i].m_vecLighting ); 
			}
		}
	}
}

void InitLightinfo( lightinfo_t *pl, int facenum )
{
	dface_t		*f;

	f = &g_pFaces[facenum];

	memset (pl, 0, sizeof(*pl));
	pl->facenum = facenum;

    pl->face = f;

    //
    // rotate plane 
    //
	VectorCopy (dplanes[f->planenum].normal, pl->facenormal);
	pl->facedist = dplanes[f->planenum].dist;

	// get the origin offset for rotating bmodels
	VectorCopy (face_offset[facenum], pl->modelorg);

	CalcFaceVectors( pl );

	// figure out if the surface is flat
	pl->isflat = true;
	if (smoothing_threshold != 1)
	{
		faceneighbor_t *fn = &faceneighbor[facenum];

		for (int j=0 ; j<f->numedges ; j++)
		{
			float dot = DotProduct( pl->facenormal, fn->normal[j] );
			if (dot < 1.0 - EQUAL_EPSILON)
			{
				pl->isflat = false;
				break;
			}
		}
	}
}

static void InitSampleInfo( lightinfo_t const& l, int iThread, SSE_SampleInfo_t& info )
{
	info.m_LightmapWidth  = l.face->m_LightmapTextureSizeInLuxels[0]+1;
	info.m_LightmapHeight = l.face->m_LightmapTextureSizeInLuxels[1]+1;
	info.m_LightmapSize = info.m_LightmapWidth * info.m_LightmapHeight;

	// How many lightmaps are we going to need?
	info.m_pTexInfo = &texinfo[l.face->texinfo];
	info.m_NormalCount = (info.m_pTexInfo->flags & SURF_BUMPLIGHT) ? NUM_BUMP_VECTS + 1 : 1;
	info.m_FaceNum = l.facenum;
	info.m_pFace = l.face;
	info.m_pFaceLight = &facelight[info.m_FaceNum];
	info.m_IsDispFace = ValidDispFace( info.m_pFace );
	info.m_iThread = iThread;
	info.m_WarnFace = -1;

	info.m_NumSamples = info.m_pFaceLight->numsamples;
	info.m_NumSampleGroups = ( info.m_NumSamples & 0x3) ? ( info.m_NumSamples / 4 ) + 1 : ( info.m_NumSamples / 4 );

	// initialize normals if the surface is flat
	if (l.isflat)
	{
		info.m_PointNormals[0].DuplicateVector( l.facenormal );

		// use facenormal along with the smooth normal to build the three bump map vectors
		if( info.m_NormalCount > 1 )
		{
			Vector bumpVects[NUM_BUMP_VECTS];
			GetBumpNormals( info.m_pTexInfo->textureVecsTexelsPerWorldUnits[0], 
				info.m_pTexInfo->textureVecsTexelsPerWorldUnits[1], l.facenormal, 
				l.facenormal, bumpVects );//&info.m_PointNormal[1] );

			for ( int b = 0; b < NUM_BUMP_VECTS; ++b )
			{
				info.m_PointNormals[b + 1].DuplicateVector( bumpVects[b] );
			}
		}
	}
}

void BuildFacelights (int iThread, int facenum)
{
	int	i, j;

	lightinfo_t	l;
	dface_t *f;
	facelight_t	*fl;
	SSE_SampleInfo_t sampleInfo;
	directlight_t *dl;
	Vector spot;
	Vector v[4], n[4];

	if( g_bInterrupt )
		return;

	// FIXME: Is there a better way to do this? Like, in RunThreadsOn, for instance?
	// Don't pay this cost unless we have to; this is super perf-critical code.
	if (g_pIncremental)
	{
		// Both threads will be accessing this so it needs to be protected or else thread A
		// will load it in and thread B will increment it but its increment will be
		// overwritten by thread A when thread A writes it back.
		ThreadLock();
		++g_iCurFace;
		ThreadUnlock();
	}

	// some surfaces don't need lightmaps
	f = &g_pFaces[facenum];
	f->lightofs = -1;
	for (j=0 ; j<MAXLIGHTMAPS ; j++)
		f->styles[j] = 255;

	// Trivial-reject the whole face?	
	if( !( g_FacesVisibleToLights[facenum>>3] & (1 << (facenum & 7)) ) )
		return;

	if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
		return;		// non-lit texture

	// check for patches for this face.  If none it must be degenerate.  Ignore.
	if( g_FacePatches.Element( facenum ) == g_FacePatches.InvalidIndex() )
		return;

	fl = &facelight[facenum];

	InitLightinfo( &l, facenum );
	CalcPoints( &l, fl, facenum );
	InitSampleInfo( l, iThread, sampleInfo );

	// Allocate sample positions/normals to SSE
	int numGroups = ( fl->numsamples & 0x3) ? ( fl->numsamples / 4 ) + 1 : ( fl->numsamples / 4 );

	// always allocate style 0 lightmap
	f->styles[0] = 0;
	AllocateLightstyleSamples( fl, 0, sampleInfo.m_NormalCount );

	// sample the lights at each sample location
	for ( int grp = 0; grp < numGroups; ++grp )
	{
		int nSample = 4 * grp;

		sample_t *sample = sampleInfo.m_pFaceLight->sample + nSample;
		int numSamples = min ( 4, sampleInfo.m_pFaceLight->numsamples - nSample );

		FourVectors positions;
		FourVectors normals;

		for ( int i = 0; i < 4; i++ )
		{
			v[i] = ( i < numSamples ) ? sample[i].pos : sample[numSamples - 1].pos;
			n[i] = ( i < numSamples ) ? sample[i].normal : sample[numSamples - 1].normal;
		}
		positions.LoadAndSwizzle( v[0], v[1], v[2], v[3] );
		normals.LoadAndSwizzle( n[0], n[1], n[2], n[3] );

		ComputeIlluminationPointAndNormalsSSE( l, positions, normals, &sampleInfo, numSamples );

		// Fixup sample normals in case of smooth faces
		if ( !l.isflat )
		{
			for ( int i = 0; i < numSamples; i++ )
				sample[i].normal = sampleInfo.m_PointNormals[0].Vec( i );
		}

		// Iterate over all the lights and add their contribution to this group of spots
		GatherSampleLightAt4Points( sampleInfo, nSample, numSamples );
	}
	
	// Tell the incremental light manager that we're done with this face.
	if( g_pIncremental )
	{
		for (dl = activelights; dl != NULL; dl = dl->next)
		{
			// Only deal with lightstyle 0 for incremental lighting
			if (dl->light.style == 0)
				g_pIncremental->FinishFace( dl->m_IncrementalID, facenum, iThread );
		}

		// Don't have to deal with patch lights (only direct lighting is used)
		// or supersampling
		return;
	}

	// get rid of the -extra functionality on displacement surfaces
	if (do_extra && !sampleInfo.m_IsDispFace)
	{
		// For each lightstyle, perform a supersampling pass
		for ( i = 0; i < MAXLIGHTMAPS; ++i )
		{
			// Stop when we run out of lightstyles
			if (f->styles[i] == 255)
				break;

			BuildSupersampleFaceLights( l, sampleInfo, i );
		}
	}

	if (!g_bUseMPI) 
	{
		//
		// This is done on the master node when MPI is used
		//
		BuildPatchLights( facenum );
	}

	if( g_bDumpPatches )
	{
		DumpSamples( facenum, fl );
	}
	else
	{
		FreeSampleWindings( fl );
	}

}

void BuildPatchLights( int facenum )
{
	int i, k;

	CPatch		*patch;

	dface_t	*f = &g_pFaces[facenum];
	facelight_t	*fl = &facelight[facenum];

	for( k = 0; k < MAXLIGHTMAPS; k++ )
	{
		if (f->styles[k] == 0)
			break;
	}

	if (k >= MAXLIGHTMAPS)
		return;

	for (i = 0; i < fl->numsamples; i++)
	{
		AddSampleToPatch( &fl->sample[i], fl->light[k][0][i], facenum);
	}

	// check for a valid face
	if( g_FacePatches.Element( facenum ) == g_FacePatches.InvalidIndex() )
		return;

	// push up sampled light to parents (children always exist first in the list)
	CPatch *pNextPatch;
	for( patch = &g_Patches.Element( g_FacePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != g_Patches.InvalidIndex() )
		{
			pNextPatch = &g_Patches.Element( patch->ndxNext );
		}

		// skip patches without parents
		if( patch->parent == g_Patches.InvalidIndex() )
//		if (patch->parent == -1)
			continue;

		CPatch *parent = &g_Patches.Element( patch->parent );

		parent->samplearea += patch->samplearea;
		VectorAdd( parent->samplelight, patch->samplelight, parent->samplelight );
	}

	// average up the direct light on each patch for radiosity
	if (numbounce > 0)
	{
		for( patch = &g_Patches.Element( g_FacePatches.Element( facenum ) ); patch; patch = pNextPatch )
		{
			// next patch
			pNextPatch = NULL;
			if( patch->ndxNext != g_Patches.InvalidIndex() )
			{
				pNextPatch = &g_Patches.Element( patch->ndxNext );
			}

			if (patch->samplearea)
			{ 
				float scale;
				Vector v;
				scale = 1.0 / patch->samplearea;

				VectorScale( patch->samplelight, scale, v );
				VectorAdd( patch->totallight.light[0], v, patch->totallight.light[0] );
				VectorAdd( patch->directlight, v, patch->directlight );
			}
		}
	}

	// pull totallight from children (children always exist first in the list)
	for( patch = &g_Patches.Element( g_FacePatches.Element( facenum ) ); patch; patch = pNextPatch )
	{
		// next patch
		pNextPatch = NULL;
		if( patch->ndxNext != g_Patches.InvalidIndex() )
		{
			pNextPatch = &g_Patches.Element( patch->ndxNext );
		}

		if ( patch->child1 != g_Patches.InvalidIndex() )
		{
			float s1, s2;
			CPatch *child1;
			CPatch *child2;

			child1 = &g_Patches.Element( patch->child1 );
			child2 = &g_Patches.Element( patch->child2 );

			s1 = child1->area / (child1->area + child2->area);
			s2 = child2->area / (child1->area + child2->area);

			VectorScale( child1->totallight.light[0], s1, patch->totallight.light[0] );
			VectorMA( patch->totallight.light[0], s2, child2->totallight.light[0], patch->totallight.light[0] );

			VectorCopy( patch->totallight.light[0], patch->directlight );
		}
	}

	bool needsBumpmap = false;
	if( texinfo[f->texinfo].flags & SURF_BUMPLIGHT )
	{
		needsBumpmap = true;
	}

	// add an ambient term if desired
	if (ambient[0] || ambient[1] || ambient[2])
	{
		for( int j=0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++ )
		{
			if ( f->styles[j] == 0 )
			{
				for (i = 0; i < fl->numsamples; i++)
				{
					fl->light[j][0][i].m_vecLighting += ambient;
					if( needsBumpmap )
					{
						fl->light[j][1][i].m_vecLighting += ambient;
						fl->light[j][2][i].m_vecLighting += ambient;
						fl->light[j][3][i].m_vecLighting += ambient;
					}
				}
				break;
			}
		}
	}

	// light from dlight_threshold and above is sent out, but the
	// texture itself should still be full bright

#if 0
	// if( VectorAvg( g_FacePatches[facenum]->baselight ) >= dlight_threshold)	// Now all lighted surfaces glow
 {
	 for( j=0; j < MAXLIGHTMAPS && f->styles[j] != 255; j++ )
	 {
		 if ( f->styles[j] == 0 )
		 {
			 // BUG: shouldn't this be done for all patches on the face?
			 for (i=0 ; i<fl->numsamples ; i++)
			 {
				 // garymctchange
				 VectorAdd( fl->light[j][0][i], g_FacePatches[facenum]->baselight, fl->light[j][0][i] ); 
				 if( needsBumpmap )
				 {
					 for( bumpSample = 1; bumpSample < NUM_BUMP_VECTS + 1; bumpSample++ )
					 {
						 VectorAdd( fl->light[j][bumpSample][i], g_FacePatches[facenum]->baselight, fl->light[j][bumpSample][i] ); 
					 }
				 }
			 }
			 break;
		 }
	 }
 }
#endif
}


/*
  =============
  PrecompLightmapOffsets
  =============
*/

void PrecompLightmapOffsets()
{
    int facenum;
    dface_t *f;
    int lightstyles;
    int lightdatasize = 0;
    
    // NOTE: We store avg face light data in this lump *before* the lightmap data itself
	// in *reverse order* of the way the lightstyles appear in the styles array.
    for( facenum = 0; facenum < numfaces; facenum++ )
    {
        f = &g_pFaces[facenum];

        if ( texinfo[f->texinfo].flags & TEX_SPECIAL)
            continue;		// non-lit texture
        
        if ( dlight_map != 0 )
            f->styles[1] = 0;
        
        for (lightstyles=0; lightstyles < MAXLIGHTMAPS; lightstyles++ )
        {
            if ( f->styles[lightstyles] == 255 )
                break;
        }
        
        if ( !lightstyles )
            continue;

        // Reserve room for the avg light color data
		lightdatasize += lightstyles * 4;

        f->lightofs = lightdatasize;

		bool needsBumpmap = false;
		if( texinfo[f->texinfo].flags & SURF_BUMPLIGHT )
		{
			needsBumpmap = true;
		}

		int nLuxels = (f->m_LightmapTextureSizeInLuxels[0]+1) * (f->m_LightmapTextureSizeInLuxels[1]+1);
		if( needsBumpmap )
		{
			lightdatasize += nLuxels * 4 * lightstyles * ( NUM_BUMP_VECTS + 1 );
		}
		else
		{
	        lightdatasize += nLuxels * 4 * lightstyles;
		}
    }

	// The incremental lighting code needs us to preserve the contents of dlightdata
	// since it only recomposites lighting for faces that have lights that touch them.
	if( g_pIncremental && pdlightdata->Count() )
		return;

	pdlightdata->SetSize( lightdatasize );
}

// Clamp the three values for bumped lighting such that we trade off directionality for brightness.
static void ColorClampBumped( Vector& color1, Vector& color2, Vector& color3 )
{
	Vector maxs;
	Vector *colors[3] = { &color1, &color2, &color3 };
	maxs[0] = VectorMaximum( color1 );
	maxs[1] = VectorMaximum( color2 );
	maxs[2] = VectorMaximum( color3 );

	// HACK!  Clean this up, and add some else statements
#define CONDITION(a,b,c) do { if( maxs[a] >= maxs[b] && maxs[b] >= maxs[c] ) { order[0] = a; order[1] = b; order[2] = c; } } while( 0 )
	
	int order[3];
	CONDITION(0,1,2);
	CONDITION(0,2,1);
	CONDITION(1,0,2);
	CONDITION(1,2,0);
	CONDITION(2,0,1);
	CONDITION(2,1,0);

	int i;
	for( i = 0; i < 3; i++ )
	{
		float max = VectorMaximum( *colors[order[i]] );
		if( max <= 1.0f )
		{
			continue;
		}
		// This channel is too bright. . take half of the amount that we are over and 
		// add it to the other two channel.
		float factorToRedist = ( max - 1.0f ) / max;
		Vector colorToRedist = factorToRedist * *colors[order[i]];
		*colors[order[i]] -= colorToRedist;
		colorToRedist *= 0.5f;
		*colors[order[(i+1)%3]] += colorToRedist;
		*colors[order[(i+2)%3]] += colorToRedist;
	}

	ColorClamp( color1 );
	ColorClamp( color2 );
	ColorClamp( color3 );
	
	if( color1[0] < 0.f ) color1[0] = 0.f;
	if( color1[1] < 0.f ) color1[1] = 0.f;
	if( color1[2] < 0.f ) color1[2] = 0.f;
	if( color2[0] < 0.f ) color2[0] = 0.f;
	if( color2[1] < 0.f ) color2[1] = 0.f;
	if( color2[2] < 0.f ) color2[2] = 0.f;
	if( color3[0] < 0.f ) color3[0] = 0.f;
	if( color3[1] < 0.f ) color3[1] = 0.f;
	if( color3[2] < 0.f ) color3[2] = 0.f;
}

static void LinearToBumpedLightmap(
	const float		*linearColor, 
	const float		*linearBumpColor1,
	const float		*linearBumpColor2, 
	const float		*linearBumpColor3,
	unsigned char	*ret, 
	unsigned char	*retBump1,
	unsigned char	*retBump2, 
	unsigned char	*retBump3 )
{
	const Vector &linearBump1 = *( ( const Vector * )linearBumpColor1 );
	const Vector &linearBump2 = *( ( const Vector * )linearBumpColor2 );
	const Vector &linearBump3 = *( ( const Vector * )linearBumpColor3 );

	Vector gammaGoal;
	// gammaGoal is premultiplied by 1/overbright, which we want
	gammaGoal[0] = LinearToVertexLight( linearColor[0] );
	gammaGoal[1] = LinearToVertexLight( linearColor[1] );
	gammaGoal[2] = LinearToVertexLight( linearColor[2] );
	Vector bumpAverage = linearBump1;
	bumpAverage += linearBump2;
	bumpAverage += linearBump3;
	bumpAverage *= ( 1.0f / 3.0f );
	
	Vector correctionScale;
	if( *( int * )&bumpAverage[0] != 0 && *( int * )&bumpAverage[1] != 0 && *( int * )&bumpAverage[2] != 0 )
	{
		// fast path when we know that we don't have to worry about divide by zero.
		VectorDivide( gammaGoal, bumpAverage, correctionScale );
//			correctionScale = gammaGoal / bumpSum;
	}
	else
	{
		correctionScale.Init( 0.0f, 0.0f, 0.0f );
		if( bumpAverage[0] != 0.0f )
		{
			correctionScale[0] = gammaGoal[0] / bumpAverage[0];
		}
		if( bumpAverage[1] != 0.0f )
		{
			correctionScale[1] = gammaGoal[1] / bumpAverage[1];
		}
		if( bumpAverage[2] != 0.0f )
		{
			correctionScale[2] = gammaGoal[2] / bumpAverage[2];
		}
	}
	Vector correctedBumpColor1;
	Vector correctedBumpColor2;
	Vector correctedBumpColor3;
	VectorMultiply( linearBump1, correctionScale, correctedBumpColor1 );
	VectorMultiply( linearBump2, correctionScale, correctedBumpColor2 );
	VectorMultiply( linearBump3, correctionScale, correctedBumpColor3 );

	Vector check = ( correctedBumpColor1 + correctedBumpColor2 + correctedBumpColor3 ) / 3.0f;

	ColorClampBumped( correctedBumpColor1, correctedBumpColor2, correctedBumpColor3 );

	ret[0] = RoundFloatToByte( gammaGoal[0] * 255.0f );
	ret[1] = RoundFloatToByte( gammaGoal[1] * 255.0f );
	ret[2] = RoundFloatToByte( gammaGoal[2] * 255.0f );
	retBump1[0] = RoundFloatToByte( correctedBumpColor1[0] * 255.0f );
	retBump1[1] = RoundFloatToByte( correctedBumpColor1[1] * 255.0f );
	retBump1[2] = RoundFloatToByte( correctedBumpColor1[2] * 255.0f );
	retBump2[0] = RoundFloatToByte( correctedBumpColor2[0] * 255.0f );
	retBump2[1] = RoundFloatToByte( correctedBumpColor2[1] * 255.0f );
	retBump2[2] = RoundFloatToByte( correctedBumpColor2[2] * 255.0f );
	retBump3[0] = RoundFloatToByte( correctedBumpColor3[0] * 255.0f );
	retBump3[1] = RoundFloatToByte( correctedBumpColor3[1] * 255.0f );
	retBump3[2] = RoundFloatToByte( correctedBumpColor3[2] * 255.0f );
}

//-----------------------------------------------------------------------------
// Convert a RGBExp32 to a RGBA8888
// This matches the engine's conversion, so the lighting result is consistent.
//-----------------------------------------------------------------------------
void ConvertRGBExp32ToRGBA8888( const ColorRGBExp32 *pSrc, unsigned char *pDst )
{
	Vector		linearColor;
	Vector		vertexColor;

	// convert from ColorRGBExp32 to linear space
	linearColor[0] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->r, ((ColorRGBExp32 *)pSrc)->exponent );
	linearColor[1] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->g, ((ColorRGBExp32 *)pSrc)->exponent );
	linearColor[2] = TexLightToLinear( ((ColorRGBExp32 *)pSrc)->b, ((ColorRGBExp32 *)pSrc)->exponent );

	// convert from linear space to lightmap space
	// cannot use mathlib routine directly because it doesn't match
	// the colorspace version found in the engine, which *is* the same sequence here
	vertexColor[0] = LinearToVertexLight( linearColor[0] );
	vertexColor[1] = LinearToVertexLight( linearColor[1] );
	vertexColor[2] = LinearToVertexLight( linearColor[2] );

	// this is really a color normalization with a floor
	ColorClamp( vertexColor );

	// final [0..255] scale
	pDst[0] = RoundFloatToByte( vertexColor[0] * 255.0f );
	pDst[1] = RoundFloatToByte( vertexColor[1] * 255.0f );
	pDst[2] = RoundFloatToByte( vertexColor[2] * 255.0f );
	pDst[3] = 255;
}

