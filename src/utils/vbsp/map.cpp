//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "vbsp.h"
#include "map_shared.h"
#include "disp_vbsp.h"
#include "tier1/strtools.h"
#include "builddisp.h"
#include "tier0/icommandline.h"
#include "KeyValues.h"
#include "materialsub.h"
#include "fgdlib/fgdlib.h"
#include "manifest.h"

#ifdef VSVMFIO
#include "VmfImport.h"
#endif // VSVMFIO


// undefine to make plane finding use linear sort
#define	USE_HASHING

#define	RENDER_NORMAL_EPSILON	0.00001
#define	RENDER_DIST_EPSILON	    0.01f

#define BRUSH_CLIP_EPSILON	0.01f			// this should probably be the same
                                            // as clip epsilon, but it is 0.1f and I
											// currently don't know how that number was 
											// come to (cab) - this is 0.01 of an inch
											// for clipping brush solids
struct LoadSide_t
{
	mapbrush_t *pBrush;
	side_t *pSide;
	int nSideIndex;
	int nBaseFlags;
	int nBaseContents;
	Vector planepts[3];
	brush_texture_t	td;
};


extern qboolean onlyents;


CUtlVector< CMapFile * >	g_Maps;
CMapFile					*g_MainMap = NULL;
CMapFile					*g_LoadingMap = NULL;

char CMapFile::m_InstancePath[ MAX_PATH ] = "";
int	CMapFile::m_InstanceCount = 0;
int	CMapFile::c_areaportals = 0;

void CMapFile::Init( void )
{
	entity_num = 0;
	num_entities = 0;

	nummapplanes = 0;
	memset( mapplanes, 0, sizeof( mapplanes ) );

	nummapbrushes = 0;
	memset( mapbrushes, 0, sizeof( mapbrushes ) );

	nummapbrushsides = 0;
	memset( brushsides, 0, sizeof( brushsides ) );

	memset( side_brushtextures, 0, sizeof( side_brushtextures ) );

	memset( planehash, 0, sizeof( planehash ) );

	m_ConnectionPairs = NULL;

	m_StartMapOverlays = g_aMapOverlays.Count();
	m_StartMapWaterOverlays = g_aMapWaterOverlays.Count();

	c_boxbevels = 0;
	c_edgebevels = 0;
	c_clipbrushes = 0;
	g_ClipTexinfo = -1;
}


// All the brush sides referenced by info_no_dynamic_shadow entities.
CUtlVector<int> g_NoDynamicShadowSides;


void TestExpandBrushes (void);

ChunkFileResult_t LoadDispDistancesCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispDistancesKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispInfoCallback(CChunkFile *pFile, mapdispinfo_t **ppMapDispInfo );
ChunkFileResult_t LoadDispInfoKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispNormalsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispNormalsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispOffsetsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispOffsetsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispAlphasCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispAlphasKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispTriangleTagsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispTriangleTagsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);

#ifdef VSVMFIO
ChunkFileResult_t LoadDispOffsetNormalsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo);
ChunkFileResult_t LoadDispOffsetNormalsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo);
#endif // VSVMFIO

ChunkFileResult_t LoadEntityCallback(CChunkFile *pFile, int nParam);
ChunkFileResult_t LoadEntityKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity);

ChunkFileResult_t LoadConnectionsCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity);
ChunkFileResult_t LoadConnectionsKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity);

ChunkFileResult_t LoadSolidCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity);
ChunkFileResult_t LoadSolidKeyCallback(const char *szKey, const char *szValue, mapbrush_t *pLoadBrush);

ChunkFileResult_t LoadSideCallback(CChunkFile *pFile, LoadSide_t *pSideInfo);
ChunkFileResult_t LoadSideKeyCallback(const char *szKey, const char *szValue, LoadSide_t *pSideInfo);



/*
=============================================================================

PLANE FINDING

=============================================================================
*/


/*
=================
PlaneTypeForNormal
=================
*/
int	PlaneTypeForNormal (Vector& normal)
{
	vec_t	ax, ay, az;
	
// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] == 1.0 || normal[0] == -1.0)
		return PLANE_X;
	if (normal[1] == 1.0 || normal[1] == -1.0)
		return PLANE_Y;
	if (normal[2] == 1.0 || normal[2] == -1.0)
		return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);
	
	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

/*
================
PlaneEqual
================
*/
qboolean	PlaneEqual (plane_t *p, Vector& normal, vec_t dist, float normalEpsilon, float distEpsilon)
{
#if 1
	if (
	   fabs(p->normal[0] - normal[0]) < normalEpsilon
	&& fabs(p->normal[1] - normal[1]) < normalEpsilon
	&& fabs(p->normal[2] - normal[2]) < normalEpsilon
	&& fabs(p->dist - dist) < distEpsilon )
		return true;
#else
	if (p->normal[0] == normal[0]
		&& p->normal[1] == normal[1]
		&& p->normal[2] == normal[2]
		&& p->dist == dist)
		return true;
#endif
	return false;
}

/*
================
AddPlaneToHash
================
*/
void CMapFile::AddPlaneToHash (plane_t *p)
{
	int		hash;

	hash = (int)fabs(p->dist) / 8;
	hash &= (PLANE_HASHES-1);

	p->hash_chain = planehash[hash];
	planehash[hash] = p;
}

/*
================
CreateNewFloatPlane
================
*/
int CMapFile::CreateNewFloatPlane (Vector& normal, vec_t dist)
{
	plane_t	*p, temp;

	if (VectorLength(normal) < 0.5)
		g_MapError.ReportError ("FloatPlane: bad normal");
	// create a new plane
	if (nummapplanes+2 > MAX_MAP_PLANES)
		g_MapError.ReportError ("MAX_MAP_PLANES");

	p = &mapplanes[nummapplanes];
	VectorCopy (normal, p->normal);
	p->dist = dist;
	p->type = (p+1)->type = PlaneTypeForNormal (p->normal);

	VectorSubtract (vec3_origin, normal, (p+1)->normal);
	(p+1)->dist = -dist;

	nummapplanes += 2;

	// allways put axial planes facing positive first
	if (p->type < 3)
	{
		if (p->normal[0] < 0 || p->normal[1] < 0 || p->normal[2] < 0)
		{
			// flip order
			temp = *p;
			*p = *(p+1);
			*(p+1) = temp;

			AddPlaneToHash (p);
			AddPlaneToHash (p+1);
			return nummapplanes - 1;
		}
	}

	AddPlaneToHash (p);
	AddPlaneToHash (p+1);
	return nummapplanes - 2;
}


/*
==============
SnapVector
==============
*/
bool SnapVector (Vector& normal)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(normal[i] - 1) < RENDER_NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = 1;
			return true;
		}

		if ( fabs(normal[i] - -1) < RENDER_NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = -1;
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Snaps normal to axis-aligned if it is within an epsilon of axial.
//			Rounds dist to integer if it is within an epsilon of integer.
// Input  : normal - Plane normal vector (assumed to be unit length).
//			dist - Plane constant.
//-----------------------------------------------------------------------------
void SnapPlane(Vector &normal, vec_t &dist)
{
	SnapVector(normal);

	if (fabs(dist - RoundInt(dist)) < RENDER_DIST_EPSILON)
	{
		dist = RoundInt(dist);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Snaps normal to axis-aligned if it is within an epsilon of axial.
//			Recalculates dist if the normal was snapped. Rounds dist to integer
//			if it is within an epsilon of integer.
// Input  : normal - Plane normal vector (assumed to be unit length).
//			dist - Plane constant.
//			p0, p1, p2 - Three points on the plane.
//-----------------------------------------------------------------------------
void SnapPlane(Vector &normal, vec_t &dist, const Vector &p0, const Vector &p1, const Vector &p2)
{
	if (SnapVector(normal))
	{
		//
		// Calculate a new plane constant using the snapped normal. Use the
		// centroid of the three plane points to minimize error. This is like
		// rotating the plane around the centroid.
		//
		Vector p3 = (p0 + p1 + p2) / 3.0f;
		dist = normal.Dot(p3);
		if ( g_snapAxialPlanes )
		{
			dist = RoundInt(dist);
		}
	}

	if (fabs(dist - RoundInt(dist)) < RENDER_DIST_EPSILON)
	{
		dist = RoundInt(dist);
	}
}


/*
=============
FindFloatPlane

=============
*/
#ifndef USE_HASHING
int CMapFile::FindFloatPlane (Vector& normal, vec_t dist)
{
	int		i;
	plane_t	*p;

	SnapPlane(normal, dist);
	for (i=0, p=mapplanes ; i<nummapplanes ; i++, p++)
	{
		if (PlaneEqual (p, normal, dist, RENDER_NORMAL_EPSILON, RENDER_DIST_EPSILON))
			return i;
	}

	return CreateNewFloatPlane (normal, dist);
}
#else
int	CMapFile::FindFloatPlane (Vector& normal, vec_t dist)
{
	int		i;
	plane_t	*p;
	int		hash, h;

	SnapPlane(normal, dist);
	hash = (int)fabs(dist) / 8;
	hash &= (PLANE_HASHES-1);

	// search the border bins as well
	for (i=-1 ; i<=1 ; i++)
	{
		h = (hash+i)&(PLANE_HASHES-1);
		for (p = planehash[h] ; p ; p=p->hash_chain)
		{
			if (PlaneEqual (p, normal, dist, RENDER_NORMAL_EPSILON, RENDER_DIST_EPSILON))
				return p-mapplanes;
		}
	}

	return CreateNewFloatPlane (normal, dist);
}
#endif


//-----------------------------------------------------------------------------
// Purpose: Builds a plane normal and distance from three points on the plane.
//			If the normal is nearly axial, it will be snapped to be axial. Looks
//			up the plane in the unique planes.
// Input  : p0, p1, p2 - Three points on the plane.
// Output : Returns the index of the plane in the planes list.
//-----------------------------------------------------------------------------
int CMapFile::PlaneFromPoints(const Vector &p0, const Vector &p1, const Vector &p2)
{
	Vector	t1, t2, normal;
	vec_t	dist;
	
	VectorSubtract (p0, p1, t1);
	VectorSubtract (p2, p1, t2);
	CrossProduct (t1, t2, normal);
	VectorNormalize (normal);

	dist = DotProduct (p0, normal);

	SnapPlane(normal, dist, p0, p1, p2);

	return FindFloatPlane (normal, dist);
}


/*
===========
BrushContents
===========
*/
int	BrushContents (mapbrush_t *b)
{
	int			contents;
	int			unionContents = 0;
	side_t		*s;
	int			i;

	s = &b->original_sides[0];
	contents = s->contents;
	unionContents = contents;
	for (i=1 ; i<b->numsides ; i++, s++)
	{
		s = &b->original_sides[i];

		unionContents |= s->contents;
#if 0
		if (s->contents != contents)
		{
			Msg("Brush %i: mixed face contents\n", b->id);
			break;
		}
#endif
	}

	// NOTE: we're making slime translucent	so that it doesn't block lighting on things floating on its surface
	int transparentContents = unionContents & (CONTENTS_WINDOW|CONTENTS_GRATE|CONTENTS_WATER|CONTENTS_SLIME);
	if ( transparentContents )
	{
		contents |= transparentContents | CONTENTS_TRANSLUCENT;
		contents &= ~CONTENTS_SOLID;
	}

	return contents;
}


//============================================================================

bool IsAreaPortal( char const *pClassName )
{
	// If the class name starts with "func_areaportal", then it's considered an area portal.
	char const *pBaseName = "func_areaportal";
	char const *pCur = pBaseName;
	while( *pCur && *pClassName )
	{
		if( *pCur != *pClassName )
			break;

		++pCur;
		++pClassName;
	}

	return *pCur == 0;
}


/*
=================
AddBrushBevels

Adds any additional planes necessary to allow the brush to be expanded
against axial bounding boxes
=================
*/
void CMapFile::AddBrushBevels (mapbrush_t *b)
{
	int		axis, dir;
	int		i, j, k, l, order;
	side_t	sidetemp;
	brush_texture_t	tdtemp;
	side_t	*s, *s2;
	Vector	normal;
	float	dist;
	winding_t	*w, *w2;
	Vector	vec, vec2;
	float	d;

	//
	// add the axial planes
	//
	order = 0;
	for (axis=0 ; axis <3 ; axis++)
	{
		for (dir=-1 ; dir <= 1 ; dir+=2, order++)
		{
			// see if the plane is allready present
			for (i=0, s=b->original_sides ; i<b->numsides ; i++,s++)
			{
				if (mapplanes[s->planenum].normal[axis] == dir)
					break;
			}

			if (i == b->numsides)
			{	// add a new side
				if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
					g_MapError.ReportError ("MAX_MAP_BRUSHSIDES");
				nummapbrushsides++;
				b->numsides++;
				VectorClear (normal);
				normal[axis] = dir;
				if (dir == 1)
					dist = b->maxs[axis];
				else
					dist = -b->mins[axis];
				s->planenum = FindFloatPlane (normal, dist);
				s->texinfo = b->original_sides[0].texinfo;
				s->contents = b->original_sides[0].contents;
				s->bevel = true;
				c_boxbevels++;
			}

			// if the plane is not in it canonical order, swap it
			if (i != order)
			{
				sidetemp = b->original_sides[order];
				b->original_sides[order] = b->original_sides[i];
				b->original_sides[i] = sidetemp;

				j = b->original_sides - brushsides;
				tdtemp = side_brushtextures[j+order];
				side_brushtextures[j+order] = side_brushtextures[j+i];
				side_brushtextures[j+i] = tdtemp;
			}
		}
	}

	//
	// add the edge bevels
	//
	if (b->numsides == 6)
		return;		// pure axial

	// test the non-axial plane edges
	for (i=6 ; i<b->numsides ; i++)
	{
		s = b->original_sides + i;
		w = s->winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			k = (j+1)%w->numpoints;
			VectorSubtract (w->p[j], w->p[k], vec);
			if (VectorNormalize (vec) < 0.5)
				continue;
			SnapVector (vec);
			for (k=0 ; k<3 ; k++)
				if ( vec[k] == -1 || vec[k] == 1)
					break;	// axial
			if (k != 3)
				continue;	// only test non-axial edges

			// try the six possible slanted axials from this edge
			for (axis=0 ; axis <3 ; axis++)
			{
				for (dir=-1 ; dir <= 1 ; dir+=2)
				{
					// construct a plane
					VectorClear (vec2);
					vec2[axis] = dir;
					CrossProduct (vec, vec2, normal);
					if (VectorNormalize (normal) < 0.5)
						continue;
					dist = DotProduct (w->p[j], normal);

					// if all the points on all the sides are
					// behind this plane, it is a proper edge bevel
					for (k=0 ; k<b->numsides ; k++)
					{
						// if this plane has allready been used, skip it
						// NOTE: Use a larger tolerance for collision planes than for rendering planes
						if ( PlaneEqual(&mapplanes[b->original_sides[k].planenum], normal, dist, 0.01f, 0.01f ) )
							break;

						w2 = b->original_sides[k].winding;
						if (!w2)
							continue;
						for (l=0 ; l<w2->numpoints ; l++)
						{
							d = DotProduct (w2->p[l], normal) - dist;
							if (d > 0.1)
								break;	// point in front
						}
						if (l != w2->numpoints)
							break;
					}

					if (k != b->numsides)
						continue;	// wasn't part of the outer hull
					// add this plane
					if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
						g_MapError.ReportError ("MAX_MAP_BRUSHSIDES");
					nummapbrushsides++;
					s2 = &b->original_sides[b->numsides];
					s2->planenum = FindFloatPlane (normal, dist);
					s2->texinfo = b->original_sides[0].texinfo;
					s2->contents = b->original_sides[0].contents;
					s2->bevel = true;
					c_edgebevels++;
					b->numsides++;
				}
			}
		}
	}
}

/*
================
MakeBrushWindings

makes basewindigs for sides and mins / maxs for the brush
================
*/
qboolean CMapFile::MakeBrushWindings (mapbrush_t *ob)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane;

	ClearBounds (ob->mins, ob->maxs);

	for (i=0 ; i<ob->numsides ; i++)
	{
		plane = &mapplanes[ob->original_sides[i].planenum];
		w = BaseWindingForPlane (plane->normal, plane->dist);
		for (j=0 ; j<ob->numsides && w; j++)
		{
			if (i == j)
				continue;
			if (ob->original_sides[j].bevel)
				continue;
			plane = &mapplanes[ob->original_sides[j].planenum^1];
//			ChopWindingInPlace (&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
			// adding an epsilon here, due to precision issues creating complex
			// displacement surfaces (cab)
			ChopWindingInPlace( &w, plane->normal, plane->dist, BRUSH_CLIP_EPSILON );
		}

		side = &ob->original_sides[i];
		side->winding = w;
		if (w)
		{
			side->visible = true;
			for (j=0 ; j<w->numpoints ; j++)
				AddPointToBounds (w->p[j], ob->mins, ob->maxs);
		}
	}

	for (i=0 ; i<3 ; i++)
	{
		if (ob->mins[i] < MIN_COORD_INTEGER || ob->maxs[i] > MAX_COORD_INTEGER)
			Msg("Brush %i: bounds out of range\n", ob->id);
		if (ob->mins[i] > MAX_COORD_INTEGER || ob->maxs[i] < MIN_COORD_INTEGER)
			Msg("Brush %i: no visible sides on brush\n", ob->id);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Takes all of the brushes from the current entity and adds them to the
//			world's brush list. Used by func_detail and func_areaportal.
//			THIS ROUTINE MAY ONLY BE USED DURING ENTITY LOADING.
// Input  : mapent - Entity whose brushes are to be moved to the world.
//-----------------------------------------------------------------------------
void CMapFile::MoveBrushesToWorld( entity_t *mapent )
{
	int			newbrushes;
	int			worldbrushes;
	mapbrush_t	*temp;
	int			i;

	// this is pretty gross, because the brushes are expected to be
	// in linear order for each entity

	newbrushes = mapent->numbrushes;
	worldbrushes = entities[0].numbrushes;

	temp = (mapbrush_t *)malloc(newbrushes*sizeof(mapbrush_t));
	memcpy (temp, mapbrushes + mapent->firstbrush, newbrushes*sizeof(mapbrush_t));

#if	0		// let them keep their original brush numbers
	for (i=0 ; i<newbrushes ; i++)
		temp[i].entitynum = 0;
#endif

	// make space to move the brushes (overlapped copy)
	memmove (mapbrushes + worldbrushes + newbrushes,
		mapbrushes + worldbrushes,
		sizeof(mapbrush_t) * (nummapbrushes - worldbrushes - newbrushes) );

	// copy the new brushes down
	memcpy (mapbrushes + worldbrushes, temp, sizeof(mapbrush_t) * newbrushes);

	// fix up indexes
	entities[0].numbrushes += newbrushes;
	for (i=1 ; i<num_entities ; i++)
		entities[i].firstbrush += newbrushes;
	free (temp);

	mapent->numbrushes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Takes all of the brushes from the current entity and adds them to the
//			world's brush list. Used by func_detail and func_areaportal.
// Input  : mapent - Entity whose brushes are to be moved to the world.
//-----------------------------------------------------------------------------
void CMapFile::MoveBrushesToWorldGeneral( entity_t *mapent )
{
	int			newbrushes;
	int			worldbrushes;
	mapbrush_t	*temp;
	int			i;

	for( i = 0; i < nummapdispinfo; i++ )
	{
		if ( mapdispinfo[ i ].entitynum == ( mapent - entities ) )
		{
			mapdispinfo[ i ].entitynum = 0;
		}
	}

	// this is pretty gross, because the brushes are expected to be
	// in linear order for each entity
	newbrushes = mapent->numbrushes;
	worldbrushes = entities[0].numbrushes;

	temp = (mapbrush_t *)malloc(newbrushes*sizeof(mapbrush_t));
	memcpy (temp, mapbrushes + mapent->firstbrush, newbrushes*sizeof(mapbrush_t));

#if	0		// let them keep their original brush numbers
	for (i=0 ; i<newbrushes ; i++)
		temp[i].entitynum = 0;
#endif

	// make space to move the brushes (overlapped copy)
	memmove (mapbrushes + worldbrushes + newbrushes,
		mapbrushes + worldbrushes,
		sizeof(mapbrush_t) * (mapent->firstbrush - worldbrushes) );


	// wwwxxxmmyyy

	// copy the new brushes down
	memcpy (mapbrushes + worldbrushes, temp, sizeof(mapbrush_t) * newbrushes);

	// fix up indexes
	entities[0].numbrushes += newbrushes;
	for (i=1 ; i<num_entities ; i++)
	{
		if ( entities[ i ].firstbrush < mapent->firstbrush ) // if we use <=, then we'll remap the passed in ent, which we don't want to
		{
			entities[ i ].firstbrush += newbrushes;
		}
	}
	free (temp);

	mapent->numbrushes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Iterates the sides of brush and removed CONTENTS_DETAIL from each side
// Input  : *brush - 
//-----------------------------------------------------------------------------
void RemoveContentsDetailFromBrush( mapbrush_t *brush )
{
	// Only valid on non-world brushes
	Assert( brush->entitynum != 0 );

	side_t		*s;
	int			i;

	s = &brush->original_sides[0];
	for ( i=0 ; i<brush->numsides ; i++, s++ )
	{
		if ( s->contents & CONTENTS_DETAIL )
		{
			s->contents &= ~CONTENTS_DETAIL;
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Iterates all brushes in an entity and removes CONTENTS_DETAIL from all brushes
// Input  : *mapent - 
//-----------------------------------------------------------------------------
void CMapFile::RemoveContentsDetailFromEntity( entity_t *mapent )
{
	int i;
	for ( i = 0; i < mapent->numbrushes; i++ )
	{
		int brushnum = mapent->firstbrush + i;

		mapbrush_t *brush = &mapbrushes[ brushnum ];
		RemoveContentsDetailFromBrush( brush );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispDistancesCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispDistancesKeyCallback, pMapDispInfo));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispDistancesKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pMapDispInfo->power) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext = strtok(szBuf, " ");
		int nIndex = nRow * nCols;

		while (pszNext != NULL)
		{
			pMapDispInfo->dispDists[nIndex] = (float)atof(pszNext);
			pszNext = strtok(NULL, " ");
			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: load in the displacement info "chunk" from the .map file into the
//          vbsp map displacement info data structure
// Output : return the index of the map displacement info
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispInfoCallback(CChunkFile *pFile, mapdispinfo_t **ppMapDispInfo )
{
    //
    // check to see if we exceeded the maximum displacement info list size
    //
    if (nummapdispinfo > MAX_MAP_DISPINFO)
	{
        g_MapError.ReportError( "ParseDispInfoChunk: nummapdispinfo > MAX_MAP_DISPINFO" );
	}

    // get a pointer to the next available displacement info slot
    mapdispinfo_t *pMapDispInfo = &mapdispinfo[nummapdispinfo];
    nummapdispinfo++;

	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("normals", (ChunkHandler_t)LoadDispNormalsCallback, pMapDispInfo);
	Handlers.AddHandler("distances", (ChunkHandler_t)LoadDispDistancesCallback, pMapDispInfo);
	Handlers.AddHandler("offsets", (ChunkHandler_t)LoadDispOffsetsCallback, pMapDispInfo);
	Handlers.AddHandler("alphas", (ChunkHandler_t)LoadDispAlphasCallback, pMapDispInfo);
	Handlers.AddHandler("triangle_tags", (ChunkHandler_t)LoadDispTriangleTagsCallback, pMapDispInfo);

#ifdef VSVMFIO
	Handlers.AddHandler("offset_normals", (ChunkHandler_t)LoadDispOffsetNormalsCallback, pMapDispInfo);
#endif // VSVMFIO

	//
	// Read the displacement chunk.
	//
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadDispInfoKeyCallback, pMapDispInfo);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		// return a pointer to the displacement info
		*ppMapDispInfo = pMapDispInfo;
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*mapent - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispInfoKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!stricmp(szKey, "power"))
	{
		CChunkFile::ReadKeyValueInt(szValue, pMapDispInfo->power);
	}
#ifdef VSVMFIO
	else if (!stricmp(szKey, "elevation"))
	{
		CChunkFile::ReadKeyValueFloat(szValue, pMapDispInfo->m_elevation);
	}
#endif // VSVMFIO
	else if (!stricmp(szKey, "uaxis"))
	{
		CChunkFile::ReadKeyValueVector3(szValue, pMapDispInfo->uAxis);
	}
	else if (!stricmp(szKey, "vaxis"))
	{
		CChunkFile::ReadKeyValueVector3(szValue, pMapDispInfo->vAxis);
	}
	else if( !stricmp( szKey, "startposition" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pMapDispInfo->startPosition );
	}
	else if( !stricmp( szKey, "flags" ) )
	{
		CChunkFile::ReadKeyValueInt( szValue, pMapDispInfo->flags );
	}
#if 0 // old data
	else if (!stricmp( szKey, "alpha" ) )
	{
		CChunkFile::ReadKeyValueVector4( szValue, pMapDispInfo->alphaValues );
	}
#endif
	else if (!stricmp(szKey, "mintess"))
	{
	    CChunkFile::ReadKeyValueInt(szValue, pMapDispInfo->minTess);
	}
	else if (!stricmp(szKey, "smooth"))
	{
		CChunkFile::ReadKeyValueFloat(szValue, pMapDispInfo->smoothingAngle);
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispNormalsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispNormalsKeyCallback, pMapDispInfo));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispNormalsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pMapDispInfo->power) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext0 = strtok(szBuf, " ");
		char *pszNext1 = strtok(NULL, " ");
		char *pszNext2 = strtok(NULL, " ");

		int nIndex = nRow * nCols;

		while ((pszNext0 != NULL) && (pszNext1 != NULL) && (pszNext2 != NULL))
		{
			pMapDispInfo->vectorDisps[nIndex][0] = (float)atof(pszNext0);
			pMapDispInfo->vectorDisps[nIndex][1] = (float)atof(pszNext1);
			pMapDispInfo->vectorDisps[nIndex][2] = (float)atof(pszNext2);

			pszNext0 = strtok(NULL, " ");
			pszNext1 = strtok(NULL, " ");
			pszNext2 = strtok(NULL, " ");

			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispOffsetsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispOffsetsKeyCallback, pMapDispInfo));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispOffsetsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pMapDispInfo->power) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext0 = strtok(szBuf, " ");
		char *pszNext1 = strtok(NULL, " ");
		char *pszNext2 = strtok(NULL, " ");

		int nIndex = nRow * nCols;

		while ((pszNext0 != NULL) && (pszNext1 != NULL) && (pszNext2 != NULL))
		{
			pMapDispInfo->vectorOffsets[nIndex][0] = (float)atof(pszNext0);
			pMapDispInfo->vectorOffsets[nIndex][1] = (float)atof(pszNext1);
			pMapDispInfo->vectorOffsets[nIndex][2] = (float)atof(pszNext2);

			pszNext0 = strtok(NULL, " ");
			pszNext1 = strtok(NULL, " ");
			pszNext2 = strtok(NULL, " ");

			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}


#ifdef VSVMFIO
ChunkFileResult_t LoadDispOffsetNormalsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispOffsetNormalsKeyCallback, pMapDispInfo));
}


ChunkFileResult_t LoadDispOffsetNormalsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pMapDispInfo->power) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext0 = strtok(szBuf, " ");
		char *pszNext1 = strtok(NULL, " ");
		char *pszNext2 = strtok(NULL, " ");

		int nIndex = nRow * nCols;

		while ((pszNext0 != NULL) && (pszNext1 != NULL) && (pszNext2 != NULL))
		{
			pMapDispInfo->m_offsetNormals[nIndex][0] = (float)atof(pszNext0);
			pMapDispInfo->m_offsetNormals[nIndex][1] = (float)atof(pszNext1);
			pMapDispInfo->m_offsetNormals[nIndex][2] = (float)atof(pszNext2);

			pszNext0 = strtok(NULL, " ");
			pszNext1 = strtok(NULL, " ");
			pszNext2 = strtok(NULL, " ");

			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}
#endif // VSVMFIO


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispAlphasCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispAlphasKeyCallback, pMapDispInfo));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKey - 
//			*szValue - 
//			*pDisp - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispAlphasKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if (!strnicmp(szKey, "row", 3))
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy(szBuf, szValue);

		int nCols = (1 << pMapDispInfo->power) + 1;
		int nRow = atoi(&szKey[3]);

		char *pszNext0 = strtok(szBuf, " ");

		int nIndex = nRow * nCols;

		while (pszNext0 != NULL)
		{
			pMapDispInfo->alphaValues[nIndex] = (float)atof(pszNext0);
			pszNext0 = strtok(NULL, " ");
			nIndex++;
		}
	}

	return(ChunkFile_Ok);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispTriangleTagsCallback(CChunkFile *pFile, mapdispinfo_t *pMapDispInfo)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadDispTriangleTagsKeyCallback, pMapDispInfo));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadDispTriangleTagsKeyCallback(const char *szKey, const char *szValue, mapdispinfo_t *pMapDispInfo)
{
	if ( !strnicmp( szKey, "row", 3 ) )
	{
		char szBuf[MAX_KEYVALUE_LEN];
		strcpy( szBuf, szValue );

		int nCols = ( 1 << pMapDispInfo->power );
		int nRow = atoi( &szKey[3] );

		char *pszNext = strtok( szBuf, " " );

		int nIndex = nRow * nCols;
		int iTri = nIndex * 2;

		while ( pszNext != NULL ) 
		{
			// Collapse the tags here!
			unsigned short nTriTags = ( unsigned short )atoi( pszNext );

			// Walkable
			bool bWalkable = ( ( nTriTags & COREDISPTRI_TAG_WALKABLE ) != 0 );
			if ( ( ( nTriTags & COREDISPTRI_TAG_FORCE_WALKABLE_BIT ) != 0 ) )
			{
				bWalkable = ( ( nTriTags & COREDISPTRI_TAG_FORCE_WALKABLE_VAL ) != 0 );
			}

			// Buildable
			bool bBuildable = ( ( nTriTags & COREDISPTRI_TAG_BUILDABLE ) != 0 );
			if ( ( ( nTriTags & COREDISPTRI_TAG_FORCE_BUILDABLE_BIT ) != 0 ) )
			{
				bBuildable = ( ( nTriTags & COREDISPTRI_TAG_FORCE_BUILDABLE_VAL ) != 0 );
			}

			nTriTags = 0;
			if ( bWalkable )
			{
				nTriTags |= DISPTRI_TAG_WALKABLE;
			}

			if ( bBuildable )
			{
				nTriTags |= DISPTRI_TAG_BUILDABLE;
			}

			pMapDispInfo->triTags[iTri] = nTriTags;
			pszNext = strtok( NULL, " " );
			iTri++;
		}
	}

	return( ChunkFile_Ok );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : brushSideID - 
// Output : int
//-----------------------------------------------------------------------------
int CMapFile::SideIDToIndex( int brushSideID )
{
	int i;
	for ( i = 0; i < nummapbrushsides; i++ )
	{
		if ( brushsides[i].id == brushSideID )
		{
			return i;
		}
	}
	Assert( 0 );
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *mapent - 
//			*key - 
//-----------------------------------------------------------------------------
void ConvertSideList( entity_t *mapent, char *key )
{
	char *pszSideList = ValueForKey( mapent, key );

	if (pszSideList)
	{
		char *pszTmpList = ( char* )_alloca( strlen( pszSideList ) + 1 );
		strcpy( pszTmpList, pszSideList );

		bool bFirst = true;
		char szNewValue[1024];
		szNewValue[0] = '\0';

		const char *pszScan = strtok( pszTmpList, " " );
		if ( !pszScan )
			return;
		do
		{
			int nSideID;

			if ( sscanf( pszScan, "%d", &nSideID ) == 1 )
			{
				int nIndex = g_LoadingMap->SideIDToIndex(nSideID);
				if (nIndex != -1)
				{
					if (!bFirst)
					{
						strcat( szNewValue, " " );
					}
					else
					{
						bFirst = false;
					}

					char szIndex[15];
					itoa( nIndex, szIndex, 10 );
					strcat( szNewValue, szIndex );
				}
			}
		} while ( ( pszScan = strtok( NULL, " " ) ) );

		SetKeyValue( mapent, key, szNewValue );
	}
}


// Add all the sides referenced by info_no_dynamic_shadows entities to g_NoDynamicShadowSides.
ChunkFileResult_t HandleNoDynamicShadowsEnt( entity_t *pMapEnt )
{
	// Get the list of the sides.
	char *pSideList = ValueForKey( pMapEnt, "sides" );

	// Parse the side list.
	char *pScan = strtok( pSideList, " " );
	if( pScan )
	{
		do
		{
			int brushSideID;
			if( sscanf( pScan, "%d", &brushSideID ) == 1 )
			{
				if ( g_NoDynamicShadowSides.Find( brushSideID ) == -1 )
					g_NoDynamicShadowSides.AddToTail( brushSideID );
			}
		} while( ( pScan = strtok( NULL, " " ) ) );
	}
	
	// Clear out this entity.
	pMapEnt->epairs = NULL;
	return ( ChunkFile_Ok );
}


static ChunkFileResult_t LoadOverlayDataTransitionKeyCallback( const char *szKey, const char *szValue, mapoverlay_t *pOverlay )
{
	if ( !stricmp( szKey, "material" ) )
	{
		// Get the material name.
		const char *pMaterialName = szValue;
		if( g_ReplaceMaterials )
		{
			pMaterialName = ReplaceMaterialName( szValue );
		}

		Assert( strlen( pMaterialName ) < OVERLAY_MAP_STRLEN );
		if ( strlen( pMaterialName ) >= OVERLAY_MAP_STRLEN )
		{
			Error( "Overlay Material Name (%s) > OVERLAY_MAP_STRLEN (%d)", pMaterialName, OVERLAY_MAP_STRLEN );
			return ChunkFile_Fail;
		}
		strcpy( pOverlay->szMaterialName, pMaterialName );	
	}
	else if ( !stricmp( szKey, "StartU") )
	{
		CChunkFile::ReadKeyValueFloat( szValue, pOverlay->flU[0] );
	}
	else if ( !stricmp( szKey, "EndU" ) )
	{
		CChunkFile::ReadKeyValueFloat( szValue, pOverlay->flU[1] );
	}
	else if ( !stricmp( szKey, "StartV" ) )
	{
		CChunkFile::ReadKeyValueFloat( szValue, pOverlay->flV[0] );
	}
	else if ( !stricmp( szKey, "EndV" ) )
	{
		CChunkFile::ReadKeyValueFloat( szValue, pOverlay->flV[1] );
	}
	else if ( !stricmp( szKey, "BasisOrigin" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecOrigin );
	}
	else if ( !stricmp( szKey, "BasisU" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecBasis[0] );
	}
	else if ( !stricmp( szKey, "BasisV" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecBasis[1] );
	}
	else if ( !stricmp( szKey, "BasisNormal" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecBasis[2] );
	}
	else if ( !stricmp( szKey, "uv0" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecUVPoints[0] );
	}
	else if ( !stricmp( szKey, "uv1" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecUVPoints[1] );
	}
	else if ( !stricmp( szKey, "uv2" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecUVPoints[2] );
	}
	else if ( !stricmp( szKey, "uv3" ) )
	{
		CChunkFile::ReadKeyValueVector3( szValue, pOverlay->vecUVPoints[3] );
	}
	else if ( !stricmp( szKey, "sides" ) )
	{
		const char *pSideList = szValue;
		char *pTmpList = ( char* )_alloca( strlen( pSideList ) + 1 );
		strcpy( pTmpList, pSideList );
		const char *pScan = strtok( pTmpList, " " );
		if ( !pScan )
			return ChunkFile_Fail;

		pOverlay->aSideList.Purge();
		pOverlay->aFaceList.Purge();

		do
		{
			int nSideId;
			if ( sscanf( pScan, "%d", &nSideId ) == 1 )
			{
				pOverlay->aSideList.AddToTail( nSideId );
			}
		} while ( ( pScan = strtok( NULL, " " ) ) );
	}

	return ChunkFile_Ok;
}

static ChunkFileResult_t LoadOverlayDataTransitionCallback( CChunkFile *pFile, int nParam )
{
	int iOverlay = g_aMapWaterOverlays.AddToTail();
	mapoverlay_t *pOverlay = &g_aMapWaterOverlays[iOverlay];
	if ( !pOverlay )
		return ChunkFile_Fail;

	pOverlay->nId = ( MAX_MAP_OVERLAYS + 1 ) + g_aMapWaterOverlays.Count() - 1;
	pOverlay->m_nRenderOrder = 0;

	ChunkFileResult_t eResult = pFile->ReadChunk( ( KeyHandler_t )LoadOverlayDataTransitionKeyCallback, pOverlay );
	return eResult;
}

static ChunkFileResult_t LoadOverlayTransitionCallback( CChunkFile *pFile, int nParam )
{
	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "overlaydata", ( ChunkHandler_t )LoadOverlayDataTransitionCallback, 0 );
	pFile->PushHandlers( &Handlers );

	ChunkFileResult_t eResult = pFile->ReadChunk( NULL, NULL );

	pFile->PopHandlers();

	return eResult;
}

//-----------------------------------------------------------------------------
// Purpose: Iterates all brushes in a ladder entity, generates its mins and maxs.
//          These are stored in the object, since the brushes are going to go away.
// Input  : *mapent - 
//-----------------------------------------------------------------------------
void CMapFile::AddLadderKeys( entity_t *mapent )
{
	Vector mins, maxs;
	ClearBounds( mins, maxs );

	int i;
	for ( i = 0; i < mapent->numbrushes; i++ )
	{
		int brushnum = mapent->firstbrush + i;
		mapbrush_t *brush = &mapbrushes[ brushnum ];

		AddPointToBounds( brush->mins, mins, maxs );
		AddPointToBounds( brush->maxs, mins, maxs );
	}

	char buf[16];

	Q_snprintf( buf, sizeof(buf), "%2.2f", mins.x );
	SetKeyValue( mapent, "mins.x", buf );

	Q_snprintf( buf, sizeof(buf), "%2.2f", mins.y );
	SetKeyValue( mapent, "mins.y", buf );

	Q_snprintf( buf, sizeof(buf), "%2.2f", mins.z );
	SetKeyValue( mapent, "mins.z", buf );

	Q_snprintf( buf, sizeof(buf), "%2.2f", maxs.x );
	SetKeyValue( mapent, "maxs.x", buf );

	Q_snprintf( buf, sizeof(buf), "%2.2f", maxs.y );
	SetKeyValue( mapent, "maxs.y", buf );

	Q_snprintf( buf, sizeof(buf), "%2.2f", maxs.z );
	SetKeyValue( mapent, "maxs.z", buf );
}

ChunkFileResult_t LoadEntityCallback(CChunkFile *pFile, int nParam)
{
	return g_LoadingMap->LoadEntityCallback( pFile, nParam );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFile - 
//			ulParam - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFile::LoadEntityCallback(CChunkFile *pFile, int nParam)
{
	if (num_entities == MAX_MAP_ENTITIES)
	{
		// Exits.
		g_MapError.ReportError ("num_entities == MAX_MAP_ENTITIES");
	}

	entity_t *mapent = &entities[num_entities];
	num_entities++;
	memset(mapent, 0, sizeof(*mapent));
	mapent->firstbrush = nummapbrushes;
	mapent->numbrushes = 0;
	//mapent->portalareas[0] = -1;
	//mapent->portalareas[1] = -1;
	
	LoadEntity_t LoadEntity;
	LoadEntity.pEntity = mapent;

	// No default flags/contents
	LoadEntity.nBaseFlags = 0;
	LoadEntity.nBaseContents = 0;

	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("solid", (ChunkHandler_t)::LoadSolidCallback, &LoadEntity);
	Handlers.AddHandler("connections", (ChunkHandler_t)LoadConnectionsCallback, &LoadEntity);
	Handlers.AddHandler( "overlaytransition", ( ChunkHandler_t )LoadOverlayTransitionCallback, 0 );

	//
	// Read the entity chunk.
	//
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadEntityKeyCallback, &LoadEntity);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		GetVectorForKey (mapent, "origin", mapent->origin);

		const char *pMinDXLevelStr = ValueForKey( mapent, "mindxlevel" );
		const char *pMaxDXLevelStr = ValueForKey( mapent, "maxdxlevel" );
		if( *pMinDXLevelStr != '\0' || *pMaxDXLevelStr != '\0' )
		{
			int min = 0;
			int max = 0;
			if( *pMinDXLevelStr )
			{
				min = atoi( pMinDXLevelStr );
			}
			if( *pMaxDXLevelStr )
			{
				max = atoi( pMaxDXLevelStr );
			}

			// Set min and max to default values.
			if( min == 0 )
			{
				min = g_nDXLevel;
			}
			if( max == 0 )
			{
				max = g_nDXLevel;
			}
			if( ( g_nDXLevel != 0 ) && ( g_nDXLevel < min || g_nDXLevel > max ) )
			{
				mapent->numbrushes = 0;
				mapent->epairs = NULL;
				return(ChunkFile_Ok);
			}
		}
		
		// offset all of the planes and texinfo
		if ( mapent->origin[0] || mapent->origin[1] || mapent->origin[2] )
		{
			for (int i=0 ; i<mapent->numbrushes ; i++)
			{
				mapbrush_t *b = &mapbrushes[mapent->firstbrush + i];
				for (int j=0 ; j<b->numsides ; j++)
				{
					side_t *s = &b->original_sides[j];
					vec_t newdist = mapplanes[s->planenum].dist - DotProduct (mapplanes[s->planenum].normal, mapent->origin);
					s->planenum = FindFloatPlane (mapplanes[s->planenum].normal, newdist);
					if ( !onlyents )
					{
						s->texinfo = TexinfoForBrushTexture (&mapplanes[s->planenum], &side_brushtextures[s-brushsides], mapent->origin);
					}
				}
				MakeBrushWindings (b);
			}
		}

		//
		// func_detail brushes are moved into the world entity. The CONTENTS_DETAIL flag was set by the loader.
		//
		const char *pClassName = ValueForKey( mapent, "classname" );

		if ( !strcmp( "func_detail", pClassName ) )
		{
			MoveBrushesToWorld (mapent);
			mapent->numbrushes = 0;
			
			// clear out this entity
			mapent->epairs = NULL;
			return(ChunkFile_Ok);
		}

		// these get added to a list for processing the portal file
		// but aren't necessary to emit to the BSP
		if ( !strcmp( "func_viscluster", pClassName ) )
		{
			AddVisCluster(mapent);
			return(ChunkFile_Ok);
		}

		//
		// func_ladder brushes are moved into the world entity.  We convert the func_ladder to an info_ladder
		// that holds the ladder's mins and maxs, and leave the entity.  This helps the bots figure out ladders.
		//
		if ( !strcmp( "func_ladder", pClassName ) )
		{
			AddLadderKeys( mapent );

			MoveBrushesToWorld (mapent);

			// Convert to info_ladder entity
			SetKeyValue( mapent, "classname", "info_ladder" );

			return(ChunkFile_Ok);
		}

		if( !strcmp( "env_cubemap", pClassName ) )
		{
			if( ( g_nDXLevel == 0 ) || ( g_nDXLevel >= 70 ) )
			{
				const char *pSideListStr = ValueForKey( mapent, "sides" );
				int size;
				size = IntForKey( mapent, "cubemapsize" );
				Cubemap_InsertSample( mapent->origin, size );
				Cubemap_SaveBrushSides( pSideListStr );
			}
			// clear out this entity
			mapent->epairs = NULL;
			return(ChunkFile_Ok);
		}

		if ( !strcmp( "test_sidelist", pClassName ) )
		{
			ConvertSideList(mapent, "sides");
			return ChunkFile_Ok;
		}

		if ( !strcmp( "info_overlay", pClassName ) )
		{
			int iAccessorID = Overlay_GetFromEntity( mapent );

			if ( iAccessorID < 0 )
			{
				// Clear out this entity.
				mapent->epairs = NULL;
			}
			else
			{
				// Convert to info_overlay_accessor entity
				SetKeyValue( mapent, "classname", "info_overlay_accessor" );

				// Remember the id for accessing the overlay
				char buf[16];
				Q_snprintf( buf, sizeof(buf), "%i", iAccessorID );
				SetKeyValue( mapent, "OverlayID", buf );
			}

			return ( ChunkFile_Ok );
		}

		if ( !strcmp( "info_overlay_transition", pClassName ) )
		{
			// Clear out this entity.
			mapent->epairs = NULL;
			return ( ChunkFile_Ok );
		}

		if ( Q_stricmp( pClassName, "info_no_dynamic_shadow" ) == 0 )
		{
			return HandleNoDynamicShadowsEnt( mapent );
		}

		if ( Q_stricmp( pClassName, "func_instance_parms" ) == 0 )
		{
			// Clear out this entity.
			mapent->epairs = NULL;
			return ( ChunkFile_Ok );
		}

		// areaportal entities move their brushes, but don't eliminate
		// the entity
		if( IsAreaPortal( pClassName ) )
		{
			char	str[128];

			if (mapent->numbrushes != 1)
			{
				Error ("Entity %i: func_areaportal can only be a single brush", num_entities-1);
			}

			mapbrush_t *b = &mapbrushes[nummapbrushes-1];
			b->contents = CONTENTS_AREAPORTAL;
			c_areaportals++;
			mapent->areaportalnum = c_areaportals;

			// set the portal number as "portalnumber"
			sprintf (str, "%i", c_areaportals);
			SetKeyValue (mapent, "portalnumber", str);

			MoveBrushesToWorld (mapent);
			return(ChunkFile_Ok);
		}

#ifdef VSVMFIO
		if ( !Q_stricmp( pClassName, "light" ) )
		{
			CVmfImport::GetVmfImporter()->ImportLightCallback(
				ValueForKey( mapent, "hammerid" ),
				ValueForKey( mapent, "origin" ),
				ValueForKey( mapent, "_light" ),
				ValueForKey( mapent, "_lightHDR" ),
				ValueForKey( mapent, "_lightscaleHDR" ),
				ValueForKey( mapent, "_quadratic_attn" ) );
		}

		if ( !Q_stricmp( pClassName, "light_spot" ) )
		{
			CVmfImport::GetVmfImporter()->ImportLightSpotCallback(
				ValueForKey( mapent, "hammerid" ),
				ValueForKey( mapent, "origin" ),
				ValueForKey( mapent, "angles" ),
				ValueForKey( mapent, "pitch" ),
				ValueForKey( mapent, "_light" ),
				ValueForKey( mapent, "_lightHDR" ),
				ValueForKey( mapent, "_lightscaleHDR" ),
				ValueForKey( mapent, "_quadratic_attn" ),
				ValueForKey( mapent, "_inner_cone" ),
				ValueForKey( mapent, "_cone" ),
				ValueForKey( mapent, "_exponent" ) );
		}

		if ( !Q_stricmp( pClassName, "light_dynamic" ) )
		{
			CVmfImport::GetVmfImporter()->ImportLightDynamicCallback(
				ValueForKey( mapent, "hammerid" ),
				ValueForKey( mapent, "origin" ),
				ValueForKey( mapent, "angles" ),
				ValueForKey( mapent, "pitch" ),
				ValueForKey( mapent, "_light" ),
				ValueForKey( mapent, "_quadratic_attn" ),
				ValueForKey( mapent, "_inner_cone" ),
				ValueForKey( mapent, "_cone" ),
				ValueForKey( mapent, "brightness" ),
				ValueForKey( mapent, "distance" ),
				ValueForKey( mapent, "spotlight_radius" ) );
		}

		if ( !Q_stricmp( pClassName, "light_environment" ) )
		{
			CVmfImport::GetVmfImporter()->ImportLightEnvironmentCallback(
				ValueForKey( mapent, "hammerid" ),
				ValueForKey( mapent, "origin" ),
				ValueForKey( mapent, "angles" ),
				ValueForKey( mapent, "pitch" ),
				ValueForKey( mapent, "_light" ),
				ValueForKey( mapent, "_lightHDR" ),
				ValueForKey( mapent, "_lightscaleHDR" ),
				ValueForKey( mapent, "_ambient" ),
				ValueForKey( mapent, "_ambientHDR" ),
				ValueForKey( mapent, "_AmbientScaleHDR" ),
				ValueForKey( mapent, "SunSpreadAngle" ) );
		}

		const char *pModel = ValueForKey( mapent, "model" );
		if ( pModel && Q_strlen( pModel ) )
		{
			CVmfImport::GetVmfImporter()->ImportModelCallback(
				pModel,
				ValueForKey( mapent, "hammerid" ),
				ValueForKey( mapent, "angles" ),
				ValueForKey( mapent, "origin" ),
				MDagPath() );
		}
#endif // VSVMFIO

		// If it's not in the world at this point, unmark CONTENTS_DETAIL from all sides...
		if ( mapent != &entities[ 0 ] )
		{
			RemoveContentsDetailFromEntity( mapent );
		}

		return(ChunkFile_Ok);
	}

	return(eResult);
}


entity_t* EntityByName( char const *pTestName )
{
	if( !pTestName )
		return 0;

	for( int i=0; i < g_MainMap->num_entities; i++ )
	{
		entity_t *e = &g_MainMap->entities[i];

		const char *pName = ValueForKey( e, "targetname" );
		if( stricmp( pName, pTestName ) == 0 )
			return e;
	}

	return 0;
}


void CMapFile::ForceFuncAreaPortalWindowContents()
{
	// Now go through all areaportal entities and force CONTENTS_WINDOW
	// on the brushes of the bmodels they point at.
	char *targets[] = {"target", "BackgroundBModel"};
	int nTargets = sizeof(targets) / sizeof(targets[0]);

	for( int i=0; i < num_entities; i++ )
	{
		entity_t *e = &entities[i];

		const char *pClassName = ValueForKey( e, "classname" );

		// Don't do this on "normal" func_areaportal entities.  Those are tied to doors
		// and should be opaque when closed.  But areaportal windows (and any other 
		// distance-based areaportals) should be windows because they are normally open/transparent
		if( !IsAreaPortal( pClassName ) || !Q_stricmp( pClassName, "func_areaportal" ) )
			continue;

//		const char *pTestEntName = ValueForKey( e, "targetname" );

		for( int iTarget=0; iTarget < nTargets; iTarget++ )
		{
			char const *pEntName = ValueForKey( e, targets[iTarget] );
			if( !pEntName[0] )
				continue;

			entity_t *pBrushEnt = EntityByName( pEntName );
			if( !pBrushEnt )
				continue;

			for( int iBrush=0; iBrush < pBrushEnt->numbrushes; iBrush++ )
			{
				mapbrushes[pBrushEnt->firstbrush + iBrush].contents &= ~CONTENTS_SOLID;
				mapbrushes[pBrushEnt->firstbrush + iBrush].contents |= CONTENTS_TRANSLUCENT | CONTENTS_WINDOW;
			}
		}
	}
}


// ============ Instancing ============

// #define MERGE_INSTANCE_DEBUG_INFO	1

#define INSTANCE_VARIABLE_KEY			"replace"

static GameData	GD;

//-----------------------------------------------------------------------------
// Purpose: this function will read in a standard key / value file
// Input  : pFilename - the absolute name of the file to read
// Output : returns the KeyValues of the file, NULL if the file could not be read.
//-----------------------------------------------------------------------------
static KeyValues *ReadKeyValuesFile( const char *pFilename )
{
	// Read in the gameinfo.txt file and null-terminate it.
	FILE *fp = fopen( pFilename, "rb" );
	if ( !fp )
		return NULL;
	CUtlVector<char> buf;
	fseek( fp, 0, SEEK_END );
	buf.SetSize( ftell( fp ) + 1 );
	fseek( fp, 0, SEEK_SET );
	fread( buf.Base(), 1, buf.Count()-1, fp );
	fclose( fp );
	buf[buf.Count()-1] = 0;

	KeyValues *kv = new KeyValues( "" );
	if ( !kv->LoadFromBuffer( pFilename, buf.Base() ) )
	{
		kv->deleteThis();
		return NULL;
	}

	return kv;
}


//-----------------------------------------------------------------------------
// Purpose: this function will set a secondary lookup path for instances.
// Input  : pszInstancePath - the secondary lookup path
//-----------------------------------------------------------------------------
void CMapFile::SetInstancePath( const char *pszInstancePath )
{
	strcpy( m_InstancePath, pszInstancePath );
	V_strlower( m_InstancePath );
	V_FixSlashes( m_InstancePath );
}


//-----------------------------------------------------------------------------
// Purpose: This function will attempt to find a full path given the base and relative names.
// Input  : pszBaseFileName - the base file that referenced this instance
//			pszInstanceFileName - the relative file name of this instance
// Output : Returns true if it was able to locate the file
//			pszOutFileName - the full path to the file name if located
//-----------------------------------------------------------------------------
bool CMapFile::DeterminePath( const char *pszBaseFileName, const char *pszInstanceFileName, char *pszOutFileName )
{
	char		szInstanceFileNameFixed[ MAX_PATH ];
	const char *pszMapPath = "\\maps\\";

	strcpy( szInstanceFileNameFixed, pszInstanceFileName );
	V_SetExtension( szInstanceFileNameFixed, ".vmf", sizeof( szInstanceFileNameFixed ) );
	V_FixSlashes( szInstanceFileNameFixed );

	// first, try to find a relative location based upon the Base file name
	strcpy( pszOutFileName, pszBaseFileName );
	V_StripFilename( pszOutFileName );

	strcat( pszOutFileName, "\\" );
	strcat( pszOutFileName, szInstanceFileNameFixed );

	if ( g_pFullFileSystem->FileExists( pszOutFileName ) )
	{
		return true;
	}

	// second, try to find the master 'maps' directory and make it relative from that
	strcpy( pszOutFileName, pszBaseFileName );
	V_StripFilename( pszOutFileName );
	V_RemoveDotSlashes( pszOutFileName );
	V_FixDoubleSlashes( pszOutFileName );
	V_strlower( pszOutFileName );
	strcat( pszOutFileName, "\\" );

	char *pos = strstr( pszOutFileName, pszMapPath );
	if ( pos )
	{
		pos += strlen( pszMapPath );
		*pos = 0;
		strcat( pszOutFileName, szInstanceFileNameFixed );

		if ( g_pFullFileSystem->FileExists( pszOutFileName ) )
		{
			return true;
		}
	}

	if ( m_InstancePath[ 0 ] != 0 )
	{
		sprintf( szInstanceFileNameFixed, "%s%s", m_InstancePath, pszInstanceFileName );

		if ( g_pFullFileSystem->FileExists( szInstanceFileNameFixed, "GAME" ) )
		{
			char FullPath[ MAX_PATH ];
			g_pFullFileSystem->RelativePathToFullPath( szInstanceFileNameFixed, "GAME", FullPath, sizeof( FullPath ) );
			strcpy( pszOutFileName, FullPath );

			return true;
		}
	}

	pszOutFileName[ 0 ] = 0;

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: this function will check the main map for any func_instances.  It will
//			also attempt to load in the gamedata file for instancing remapping help.
// Input  : none
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::CheckForInstances( const char *pszFileName )
{
	if ( this != g_MainMap )
	{	// all sub-instances will be appended to the main map master list as they are read in
		// so the main loop below will naturally get to the appended ones.
		return;
	}

	char	GameInfoPath[ MAX_PATH ];

	g_pFullFileSystem->RelativePathToFullPath( "gameinfo.txt", "MOD", GameInfoPath, sizeof( GameInfoPath ) );
	KeyValues *GameInfoKV = ReadKeyValuesFile( GameInfoPath );
	if ( !GameInfoKV )
	{
		Msg( "Could not locate gameinfo.txt for Instance Remapping at %s\n", GameInfoPath );
		return;
	}

	const char *InstancePath = GameInfoKV->GetString( "InstancePath", NULL );
	if ( InstancePath )
	{
		CMapFile::SetInstancePath( InstancePath );
	}

	const char *GameDataFile = GameInfoKV->GetString( "GameData", NULL );
	if ( !GameDataFile )
	{
		Msg( "Could not locate 'GameData' key in %s\n", GameInfoPath );
		return;
	}

	char	FDGPath[ MAX_PATH ];
	if ( !g_pFullFileSystem->RelativePathToFullPath( GameDataFile, "EXECUTABLE_PATH", FDGPath, sizeof( FDGPath ) ) )
	{
		if ( !g_pFullFileSystem->RelativePathToFullPath( GameDataFile, NULL, FDGPath, sizeof( FDGPath ) ) )
		{
			Msg( "Could not locate GameData file %s\n", GameDataFile );
		}
	}

	GD.Load( FDGPath );

	// this list will grow as instances are merged onto it.  sub-instances are merged and 
	// automatically done in this processing.
	for ( int i = 0; i < num_entities; i++ )
	{
		char *pEntity = ValueForKey( &entities[ i ], "classname" );
		if ( !strcmp( pEntity, "func_instance" ) )
		{
			char *pInstanceFile = ValueForKey( &entities[ i ], "file" );
			if ( pInstanceFile[ 0 ] )
			{
				char	InstancePath[ MAX_PATH ];
				bool	bLoaded = false;

				if ( DeterminePath( pszFileName, pInstanceFile, InstancePath ) )
				{
					if ( LoadMapFile( InstancePath ) )
					{
						MergeInstance( &entities[ i ], g_LoadingMap );
						delete g_LoadingMap;
						bLoaded = true;
					}
				}

				if ( bLoaded == false )
				{
					Color red( 255, 0, 0, 255 );

					ColorSpewMessage( SPEW_ERROR, &red, "Could not open instance file %s\n", pInstanceFile );
				}
			}

			entities[ i ].numbrushes = 0;
			entities[ i ].epairs = NULL;
		}
	}

	g_LoadingMap = this;
}


//-----------------------------------------------------------------------------
// Purpose: this function will do all of the necessary work to merge the instance
//			into the main map.
// Input  : pInstanceEntity - the entity of the func_instance
//			Instance - the map file of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergeInstance( entity_t *pInstanceEntity, CMapFile *Instance )
{
	matrix3x4_t	mat;
	QAngle		angles;
	Vector		OriginOffset = pInstanceEntity->origin;

	m_InstanceCount++;

	GetAnglesForKey( pInstanceEntity, "angles", angles );
	AngleMatrix( angles, OriginOffset, mat );

#ifdef MERGE_INSTANCE_DEBUG_INFO
	Msg( "Instance Remapping: O:( %g, %g, %g ) A:( %g, %g, %g )\n", OriginOffset.x, OriginOffset.y, OriginOffset.z, angles.x, angles.y, angles.z );
#endif // #ifdef MERGE_INSTANCE_DEBUG_INFO
	MergePlanes( pInstanceEntity, Instance, OriginOffset, angles, mat );
	MergeBrushes( pInstanceEntity, Instance, OriginOffset, angles, mat );
	MergeBrushSides( pInstanceEntity, Instance, OriginOffset, angles, mat );
	MergeEntities( pInstanceEntity, Instance, OriginOffset, angles, mat );
	MergeOverlays( pInstanceEntity, Instance, OriginOffset, angles, mat );
}


//-----------------------------------------------------------------------------
// Purpose: this function will merge in the map planes from the instance into
//			the main map.
// Input  : pInstanceEntity - the entity of the func_instance
//			Instance - the map file of the instance
//			InstanceOrigin - the translation of the instance
//			InstanceAngle - the rotation of the instance
//			InstanceMatrix - the translation / rotation matrix of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergePlanes( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix )
{
	// Each pair of planes needs to be added to the main map
	for ( int i = 0; i < Instance->nummapplanes; i += 2 )
	{
		FindFloatPlane( Instance->mapplanes[i].normal, Instance->mapplanes[i].dist );
	}
}


//-----------------------------------------------------------------------------
// Purpose: this function will merge in the map brushes from the instance into
//			the main map.
// Input  : pInstanceEntity - the entity of the func_instance
//			Instance - the map file of the instance
//			InstanceOrigin - the translation of the instance
//			InstanceAngle - the rotation of the instance
//			InstanceMatrix - the translation / rotation matrix of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergeBrushes( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix )
{
	int		max_brush_id = 0;

	for( int i = 0; i < nummapbrushes; i++ )
	{
		if ( mapbrushes[ i ].id > max_brush_id )
		{
			max_brush_id = mapbrushes[ i ].id;
		}
	}

	for( int i = 0; i < Instance->nummapbrushes; i++ )
	{
		mapbrushes[ nummapbrushes + i ] = Instance->mapbrushes[ i ];

		mapbrush_t	*brush = &mapbrushes[ nummapbrushes + i ];
		brush->entitynum += num_entities;
		brush->brushnum += nummapbrushes;

		if ( i < Instance->entities[ 0 ].numbrushes || ( brush->contents & CONTENTS_LADDER ) != 0 )
		{	// world spawn brushes as well as ladders we physically move
			Vector	minsIn = brush->mins;
			Vector	maxsIn = brush->maxs;

			TransformAABB( InstanceMatrix, minsIn, maxsIn, brush->mins, brush->maxs );
		}
		else
		{
		}
		brush->id += max_brush_id;
		
		int index = brush->original_sides - Instance->brushsides;
		brush->original_sides = &brushsides[ nummapbrushsides + index ];
	}

	nummapbrushes += Instance->nummapbrushes;
}


//-----------------------------------------------------------------------------
// Purpose: this function will merge in the map sides from the instance into
//			the main map.
// Input  : pInstanceEntity - the entity of the func_instance
//			Instance - the map file of the instance
//			InstanceOrigin - the translation of the instance
//			InstanceAngle - the rotation of the instance
//			InstanceMatrix - the translation / rotation matrix of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergeBrushSides( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix )
{
	int		max_side_id = 0;

	for( int i = 0; i < nummapbrushsides; i++ )
	{
		if ( brushsides[ i ].id > max_side_id )
		{
			max_side_id = brushsides[ i ].id;
		}
	}

	for( int i = 0; i < Instance->nummapbrushsides; i++ )
	{
		brushsides[ nummapbrushsides + i ] = Instance->brushsides[ i ];

		side_t	*side = &brushsides[ nummapbrushsides + i ];
		// The planes got merged & remapped.  So you need to search for the output plane index on each side
		// NOTE: You could optimize this by saving off an index map in MergePlanes
		side->planenum = FindFloatPlane( Instance->mapplanes[side->planenum].normal, Instance->mapplanes[side->planenum].dist );
		side->id += max_side_id; 

		// this could be pre-processed into a list for quicker checking
		bool	bNeedsTranslation = ( side->pMapDisp && side->pMapDisp->entitynum == 0 );
		if ( !bNeedsTranslation )
		{	// check for sides that are part of the world spawn - those need translating
			for( int j = 0; j < Instance->entities[ 0 ].numbrushes; j++ )
			{
				int loc = Instance->mapbrushes[ j ].original_sides - Instance->brushsides;

				if ( i >= loc && i < ( loc + Instance->mapbrushes[ j ].numsides ) )
				{
					bNeedsTranslation = true;
					break;
				}
			}
		}
		if ( !bNeedsTranslation )
		{	// sides for ladders are outside of the world spawn, but also need translating
			for( int j = Instance->entities[ 0 ].numbrushes; j < Instance->nummapbrushes; j++ )
			{
				int loc = Instance->mapbrushes[ j ].original_sides - Instance->brushsides;

				if ( i >= loc && i < ( loc + Instance->mapbrushes[ j ].numsides ) && ( Instance->mapbrushes[ j ].contents & CONTENTS_LADDER ) != 0 )
				{
					bNeedsTranslation = true;
					break;
				}
			}
		}
		if ( bNeedsTranslation )
		{	// we only want to do the adjustment on world spawn brushes, not entity brushes
			if ( side->winding )
			{
				for( int point = 0; point < side->winding->numpoints; point++ )
				{
					Vector	inPoint = side->winding->p[ point ];
					VectorTransform( inPoint, InstanceMatrix, side->winding->p[ point ] );
				}
			}

			int		planenum = side->planenum;
			cplane_t inPlane, outPlane;
			inPlane.normal = mapplanes[ planenum ].normal;
			inPlane.dist = mapplanes[ planenum ].dist; 

			MatrixTransformPlane( InstanceMatrix, inPlane, outPlane );
			planenum = FindFloatPlane( outPlane.normal, outPlane.dist );
			side->planenum = planenum;

			brush_texture_t	bt = Instance->side_brushtextures[ i ];

			VectorRotate( Instance->side_brushtextures[ i ].UAxis, InstanceMatrix, bt.UAxis );
			VectorRotate( Instance->side_brushtextures[ i ].VAxis, InstanceMatrix, bt.VAxis );
			bt.shift[ 0 ] -= InstanceOrigin.Dot( bt.UAxis ) / bt.textureWorldUnitsPerTexel[ 0 ];
			bt.shift[ 1 ] -= InstanceOrigin.Dot( bt.VAxis ) / bt.textureWorldUnitsPerTexel[ 1 ];

			if ( !onlyents )
			{
				side->texinfo = TexinfoForBrushTexture ( &mapplanes[ side->planenum ], &bt, vec3_origin );
			}
		}

		if ( side->pMapDisp )
		{
			mapdispinfo_t	*disp = side->pMapDisp;
				
			disp->brushSideID = side->id;
			Vector	inPoint = disp->startPosition;
			VectorTransform( inPoint, InstanceMatrix, disp->startPosition );

			disp->face.originalface = side;
			disp->face.texinfo = side->texinfo;
			disp->face.planenum = side->planenum;
			disp->entitynum += num_entities; 

			for( int point = 0; point < disp->face.w->numpoints; point++ )
			{
				Vector	inPoint = disp->face.w->p[ point ];
				VectorTransform( inPoint, InstanceMatrix, disp->face.w->p[ point ] );
			}

		}
	}

	nummapbrushsides += Instance->nummapbrushsides;
}


//-----------------------------------------------------------------------------
// Purpose: this function will look for replace parameters in the function instance
//			to see if there is anything in the epair that should be replaced.
// Input  : pPair - the epair with the value
//			pInstanceEntity - the func_instance that may ahve replace keywords
// Output : pPair - the value field may be updated
//-----------------------------------------------------------------------------
void CMapFile::ReplaceInstancePair( epair_t *pPair, entity_t *pInstanceEntity )
{
	char	Value[ MAX_KEYVALUE_LEN ], NewValue[ MAX_KEYVALUE_LEN ];
	bool	Overwritten = false;

	strcpy( NewValue, pPair->value );
	for ( epair_t *epInstance = pInstanceEntity->epairs; epInstance != NULL; epInstance = epInstance->next )
	{
		if ( strnicmp( epInstance->key, INSTANCE_VARIABLE_KEY, strlen( INSTANCE_VARIABLE_KEY ) ) == 0 )
		{
			char InstanceVariable[ MAX_KEYVALUE_LEN ];

			strcpy( InstanceVariable, epInstance->value );

			char *ValuePos = strchr( InstanceVariable, ' ' );
			if ( !ValuePos )
			{
				continue;
			}
			*ValuePos = 0;
			ValuePos++;

			strcpy( Value, NewValue );
			if ( !V_StrSubst( Value, InstanceVariable, ValuePos, NewValue, sizeof( NewValue ), false ) )
			{
				Overwritten = true;
				break;
			}
		}
	}

	if ( !Overwritten && strcmp( pPair->value, NewValue ) != 0 )
	{
		free( pPair->value );
		pPair->value = copystring( NewValue );
	}
}


//-----------------------------------------------------------------------------
// Purpose: this function will merge in the entities from the instance into
//			the main map.
// Input  : pInstanceEntity - the entity of the func_instance
//			Instance - the map file of the instance
//			InstanceOrigin - the translation of the instance
//			InstanceAngle - the rotation of the instance
//			InstanceMatrix - the translation / rotation matrix of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergeEntities( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix )
{
	int						max_entity_id = 0;
	char					temp[ 2048 ];
	char					NameFixup[ 128 ];
	entity_t				*WorldspawnEnt = NULL;
	GameData::TNameFixup	FixupStyle;

	char *pTargetName = ValueForKey( pInstanceEntity, "targetname" );
	char *pName = ValueForKey( pInstanceEntity, "name" );
	if ( pTargetName[ 0 ] )
	{
		sprintf( NameFixup, "%s", pTargetName );
	}
	else if ( pName[ 0 ] )
	{
		sprintf( NameFixup, "%s", pName );
	}
	else
	{
		sprintf( NameFixup, "InstanceAuto%d", m_InstanceCount );
	}

	for( int i = 0; i < num_entities; i++ )
	{
		char *pID = ValueForKey( &entities[ i ], "hammerid" );
		if ( pID[ 0 ] )
		{
			int value = atoi( pID );
			if ( value > max_entity_id )
			{
				max_entity_id = value;
			}
		}
	}

	FixupStyle = ( GameData::TNameFixup )( IntForKey( pInstanceEntity, "fixup_style" ) );

	for( int i = 0; i < Instance->num_entities; i++ )
	{
		entities[ num_entities + i ] = Instance->entities[ i ];

		entity_t *entity = &entities[ num_entities + i ];
		entity->firstbrush += ( nummapbrushes - Instance->nummapbrushes );

		char *pID = ValueForKey( entity, "hammerid" );
		if ( pID[ 0 ] )
		{
			int value = atoi( pID );
			value += max_entity_id;
			sprintf( temp, "%d", value );

			SetKeyValue( entity, "hammerid", temp );
		}

		char *pEntity = ValueForKey( entity, "classname" );
		if ( strcmpi( pEntity, "worldspawn" ) == 0 )
		{
			WorldspawnEnt = entity;
		}
		else
		{
			Vector	inOrigin = entity->origin;
			VectorTransform( inOrigin, InstanceMatrix, entity->origin );

			// search for variables coming from the func_instance to replace inside of the instance
			// this is done before entity fixup, so fixup may occur on the replaced value.  Not sure if this is a desired order of operation yet.
			for ( epair_t *ep = entity->epairs; ep != NULL; ep = ep->next )
			{
				ReplaceInstancePair( ep, pInstanceEntity );
			}

#ifdef MERGE_INSTANCE_DEBUG_INFO
			Msg( "Remapping class %s\n", pEntity );
#endif // #ifdef MERGE_INSTANCE_DEBUG_INFO
			GDclass *EntClass = GD.BeginInstanceRemap( pEntity, NameFixup, InstanceOrigin, InstanceAngle );
			if ( EntClass )
			{
				for( int i = 0; i < EntClass->GetVariableCount(); i++ )
				{
					GDinputvariable *EntVar = EntClass->GetVariableAt( i );
					char *pValue = ValueForKey( entity, ( char * )EntVar->GetName() );
					if ( GD.RemapKeyValue( EntVar->GetName(), pValue, temp, FixupStyle ) )
					{
#ifdef MERGE_INSTANCE_DEBUG_INFO
						Msg( "   %d. Remapped %s: from %s to %s\n", i, EntVar->GetName(), pValue, temp );
#endif // #ifdef MERGE_INSTANCE_DEBUG_INFO
						SetKeyValue( entity, EntVar->GetName(), temp );
					}
					else
					{
#ifdef MERGE_INSTANCE_DEBUG_INFO
						Msg( "   %d. Ignored %s: %s\n", i, EntVar->GetName(), pValue );
#endif // #ifdef MERGE_INSTANCE_DEBUG_INFO
					}
				}
			}

			if ( strcmpi( pEntity, "func_simpleladder" ) == 0 )
			{	// hate having to do this, but the key values are so screwed up
				AddLadderKeys( entity );
/*				Vector	vInNormal, vOutNormal;

				vInNormal.x = FloatForKey( entity, "normal.x" );
				vInNormal.y = FloatForKey( entity, "normal.y" );
				vInNormal.z = FloatForKey( entity, "normal.z" );
				VectorRotate( vInNormal, InstanceMatrix, vOutNormal );

				Q_snprintf( temp, sizeof( temp ), "%f", vOutNormal.x );
				SetKeyValue( entity, "normal.x", temp );

				Q_snprintf( temp, sizeof( temp ), "%f", vOutNormal.y );
				SetKeyValue( entity, "normal.y", temp );

				Q_snprintf( temp, sizeof( temp ), "%f", vOutNormal.z );
				SetKeyValue( entity, "normal.z", temp );*/
			}
		}

#ifdef MERGE_INSTANCE_DEBUG_INFO
		Msg( "Instance Entity %d remapped to %d\n", i, num_entities + i );
		Msg( "   FirstBrush: from %d to %d\n", Instance->entities[ i ].firstbrush, entity->firstbrush );
		Msg( "   KV Pairs:\n" );
		for ( epair_t *ep = entity->epairs; ep->next != NULL; ep = ep->next )
		{
			Msg( "      %s %s\n", ep->key, ep->value );
		}
#endif // #ifdef MERGE_INSTANCE_DEBUG_INFO
	}

	// search for variables coming from the func_instance to replace inside of the instance
	// this is done before connection fix up, so fix up may occur on the replaced value.  Not sure if this is a desired order of operation yet.
	for( CConnectionPairs *Connection = Instance->m_ConnectionPairs; Connection; Connection = Connection->m_Next )
	{
		ReplaceInstancePair( Connection->m_Pair, pInstanceEntity );
	}

	for( CConnectionPairs *Connection = Instance->m_ConnectionPairs; Connection; Connection = Connection->m_Next )
	{
		char	*newValue, *oldValue;
		char	origValue[ 4096 ];
		int		extraLen = 0;

		oldValue = Connection->m_Pair->value;
		strcpy( origValue, oldValue );
		char chDelim = VMF_IOPARAM_STRING_DELIMITER;
		if (!strchr(origValue, VMF_IOPARAM_STRING_DELIMITER))
		{
			chDelim = ',';
		}

		char *pos = strchr( origValue, chDelim );
		if ( pos )
		{	// null terminate the first field
			*pos = NULL;
			extraLen = strlen( pos + 1) + 1;	// for the comma we just null'd
		}

		if ( GD.RemapNameField( origValue, temp, FixupStyle ) )
		{
			newValue = new char [ strlen( temp ) + extraLen + 1 ];
			strcpy( newValue, temp );
			if ( pos )
			{
				char szDelim[ 2 ];
				sprintf( szDelim, "%c", VMF_IOPARAM_STRING_DELIMITER );

				strcat( newValue, szDelim );
				strcat( newValue, pos + 1 );
			}

			Connection->m_Pair->value = newValue;
			delete oldValue;
		}
	}

	num_entities += Instance->num_entities;

	MoveBrushesToWorldGeneral( WorldspawnEnt );
	WorldspawnEnt->numbrushes = 0;
	WorldspawnEnt->epairs = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: this function will translate overlays from the instance into
//			the main map.
// Input  : InstanceEntityNum - the entity number of the func_instance
//			Instance - the map file of the instance
//			InstanceOrigin - the translation of the instance
//			InstanceAngle - the rotation of the instance
//			InstanceMatrix - the translation / rotation matrix of the instance
// Output : none
//-----------------------------------------------------------------------------
void CMapFile::MergeOverlays( entity_t *pInstanceEntity, CMapFile *Instance, Vector &InstanceOrigin, QAngle &InstanceAngle, matrix3x4_t &InstanceMatrix )
{
	for( int i = Instance->m_StartMapOverlays; i < g_aMapOverlays.Count(); i++ )
	{
		Overlay_Translate( &g_aMapOverlays[ i ], InstanceOrigin, InstanceAngle, InstanceMatrix );
	}
	for( int i = Instance->m_StartMapWaterOverlays; i < g_aMapWaterOverlays.Count(); i++ )
	{
		Overlay_Translate( &g_aMapWaterOverlays[ i ], InstanceOrigin, InstanceAngle, InstanceMatrix );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Loads a VMF or MAP file. If the file has a .MAP extension, the MAP
//			loader is used, otherwise the file is assumed to be in VMF format.
// Input  : pszFileName - Full path of the map file to load.
//-----------------------------------------------------------------------------
bool LoadMapFile( const char *pszFileName )
{
	bool				bLoadingManifest = false;
	CManifest			*pMainManifest = NULL;
	ChunkFileResult_t	eResult;
	
	//
	// Dummy this up for the texture handling. This can be removed when old .MAP file
	// support is removed.
	//
	g_nMapFileVersion = 400;

	const char *pszExtension =V_GetFileExtension( pszFileName );
	if ( pszExtension && strcmpi( pszExtension, "vmm" ) == 0 )
	{
		pMainManifest = new CManifest();
		if ( pMainManifest->LoadVMFManifest( pszFileName ) )
		{
			eResult = ChunkFile_Ok;
			pszFileName = pMainManifest->GetInstancePath();
		}
		else
		{
			eResult = ChunkFile_Fail;
		}
		bLoadingManifest = true;
	}
	else
	{
		//
		// Open the file.
		//
		CChunkFile File;
		eResult = File.Open(pszFileName, ChunkFile_Read);

		//
		// Read the file.
		//
		if (eResult == ChunkFile_Ok)
		{
			int index = g_Maps.AddToTail( new CMapFile() );
			g_LoadingMap = g_Maps[ index ];
			if ( g_MainMap == NULL )
			{
				g_MainMap = g_LoadingMap;
			}

			if ( g_MainMap == g_LoadingMap || verbose )
			{
				Msg( "Loading %s\n", pszFileName );
			}


			// reset the displacement info count
	//		nummapdispinfo = 0;

			//
			// Set up handlers for the subchunks that we are interested in.
			//
			CChunkHandlerMap Handlers;
			Handlers.AddHandler("world", (ChunkHandler_t)LoadEntityCallback, 0);
			Handlers.AddHandler("entity", (ChunkHandler_t)LoadEntityCallback, 0);

			File.PushHandlers(&Handlers);

			//
			// Read the sub-chunks. We ignore keys in the root of the file.
			//
			while (eResult == ChunkFile_Ok)
			{
				eResult = File.ReadChunk();
			}

			File.PopHandlers();
		}
		else
		{
			Error("Error opening %s: %s.\n", pszFileName, File.GetErrorText(eResult));
		}
	}

	if ((eResult == ChunkFile_Ok) || (eResult == ChunkFile_EOF))
	{
		// Update the overlay/side list(s).
		Overlay_UpdateSideLists( g_LoadingMap->m_StartMapOverlays );
		OverlayTransition_UpdateSideLists( g_LoadingMap->m_StartMapWaterOverlays );

		g_LoadingMap->CheckForInstances( pszFileName );

		if ( pMainManifest )
		{
			pMainManifest->CordonWorld();
		}

		ClearBounds (g_LoadingMap->map_mins, g_LoadingMap->map_maxs);
		for (int i=0 ; i<g_MainMap->entities[0].numbrushes ; i++)
		{
			// HLTOOLS: Raise map limits
			if (g_LoadingMap->mapbrushes[i].mins[0] > MAX_COORD_INTEGER)
			{
				continue;	// no valid points
			}

			AddPointToBounds (g_LoadingMap->mapbrushes[i].mins, g_LoadingMap->map_mins, g_LoadingMap->map_maxs);
			AddPointToBounds (g_LoadingMap->mapbrushes[i].maxs, g_LoadingMap->map_mins, g_LoadingMap->map_maxs);
		}

		qprintf ("%5i brushes\n", g_LoadingMap->nummapbrushes);
		qprintf ("%5i clipbrushes\n", g_LoadingMap->c_clipbrushes);
		qprintf ("%5i total sides\n", g_LoadingMap->nummapbrushsides);
		qprintf ("%5i boxbevels\n", g_LoadingMap->c_boxbevels);
		qprintf ("%5i edgebevels\n", g_LoadingMap->c_edgebevels);
		qprintf ("%5i entities\n", g_LoadingMap->num_entities);
		qprintf ("%5i planes\n", g_LoadingMap->nummapplanes);
		qprintf ("%5i areaportals\n", g_LoadingMap->c_areaportals);
		qprintf ("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", g_LoadingMap->map_mins[0],g_LoadingMap->map_mins[1],g_LoadingMap->map_mins[2],
			g_LoadingMap->map_maxs[0],g_LoadingMap->map_maxs[1],g_LoadingMap->map_maxs[2]);

		//TestExpandBrushes();
		
		// Clear the error reporting
		g_MapError.ClearState();
	}

	if ( g_MainMap == g_LoadingMap )
	{
		num_entities = g_MainMap->num_entities;
		memcpy( entities, g_MainMap->entities, sizeof( g_MainMap->entities ) );
	}
	g_LoadingMap->ForceFuncAreaPortalWindowContents();

	return ( ( eResult == ChunkFile_Ok ) || ( eResult == ChunkFile_EOF ) );
}

ChunkFileResult_t LoadSideCallback(CChunkFile *pFile, LoadSide_t *pSideInfo)
{
	return g_LoadingMap->LoadSideCallback( pFile, pSideInfo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pParent - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFile::LoadSideCallback(CChunkFile *pFile, LoadSide_t *pSideInfo)
{
	if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
	{
		g_MapError.ReportError ("MAX_MAP_BRUSHSIDES");
	}

	pSideInfo->pSide = &brushsides[nummapbrushsides];

	side_t *side = pSideInfo->pSide;
	mapbrush_t *b = pSideInfo->pBrush;
	g_MapError.BrushSide( pSideInfo->nSideIndex++ );

	// initialize the displacement info
	pSideInfo->pSide->pMapDisp = NULL;

	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler( "dispinfo", ( ChunkHandler_t )LoadDispInfoCallback, &side->pMapDisp );

	//
	// Read the side chunk.
	//
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadSideKeyCallback, pSideInfo);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		side->contents |= pSideInfo->nBaseContents;
		side->surf |= pSideInfo->nBaseFlags;
		pSideInfo->td.flags |= pSideInfo->nBaseFlags;

		if (side->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
		{
			side->contents |= CONTENTS_DETAIL;
		}

		if (fulldetail )
		{
			side->contents &= ~CONTENTS_DETAIL;
		}
		
		if (!(side->contents & (ALL_VISIBLE_CONTENTS | CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP)  ) )
		{
			side->contents |= CONTENTS_SOLID;
		}

		// hints and skips are never detail, and have no content
		if (side->surf & (SURF_HINT|SURF_SKIP) )
		{
			side->contents = 0;
		}

		//
		// find the plane number
		//
		int planenum = PlaneFromPoints(pSideInfo->planepts[0], pSideInfo->planepts[1], pSideInfo->planepts[2]);
		if  (planenum != -1)
		{
			//
			// See if the plane has been used already.
			//
			int k;
			for ( k = 0; k < b->numsides; k++)
			{
				side_t *s2 = b->original_sides + k;
				if (s2->planenum == planenum)
				{
					g_MapError.ReportWarning("duplicate plane");
					break;
				}
				if ( s2->planenum == (planenum^1) )
				{
					g_MapError.ReportWarning("mirrored plane");
					break;
				}
			}

			//
			// If the plane hasn't been used already, keep this side.
			//
			if (k == b->numsides)
			{
				side = b->original_sides + b->numsides;
				side->planenum = planenum;
				if ( !onlyents )
				{
					side->texinfo = TexinfoForBrushTexture (&mapplanes[planenum], &pSideInfo->td, vec3_origin);
				}
        
				// save the td off in case there is an origin brush and we
				// have to recalculate the texinfo
				if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
					g_MapError.ReportError ("MAX_MAP_BRUSHSIDES");
				side_brushtextures[nummapbrushsides] = pSideInfo->td;
				nummapbrushsides++;
				b->numsides++;

#ifdef VSVMFIO
				// Tell Maya We Have Another Side
				if ( CVmfImport::GetVmfImporter() )
				{
					CVmfImport::GetVmfImporter()->AddSideCallback(
						b, side, pSideInfo->td,
						pSideInfo->planepts[ 0 ], pSideInfo->planepts[ 1 ], pSideInfo->planepts[ 2 ] );
				}
#endif // VSVMFIO

			}
		}
		else
		{
			g_MapError.ReportWarning("plane with no normal");
		}
	}

	return(eResult);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : szKey - 
//			szValue - 
//			pSideInfo - 
// Output : 
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadSideKeyCallback(const char *szKey, const char *szValue, LoadSide_t *pSideInfo)
{
	if (!stricmp(szKey, "plane"))
	{
		int nRead = sscanf(szValue, "(%f %f %f) (%f %f %f) (%f %f %f)",
			&pSideInfo->planepts[0][0], &pSideInfo->planepts[0][1], &pSideInfo->planepts[0][2],
			&pSideInfo->planepts[1][0], &pSideInfo->planepts[1][1], &pSideInfo->planepts[1][2],
			&pSideInfo->planepts[2][0],  &pSideInfo->planepts[2][1],  &pSideInfo->planepts[2][2]);

		if (nRead != 9)
		{
			g_MapError.ReportError("parsing plane definition");
		}
	}
	else if (!stricmp(szKey, "material"))
	{
		// Get the material name.
		if( g_ReplaceMaterials )
		{
			szValue = ReplaceMaterialName( szValue );
		}

		strcpy(pSideInfo->td.name, szValue);
		g_MapError.TextureState(szValue);

		// Find default flags and values for this material.
		int mt = FindMiptex(pSideInfo->td.name);
        pSideInfo->td.flags = textureref[mt].flags;
		pSideInfo->td.lightmapWorldUnitsPerLuxel = textureref[mt].lightmapWorldUnitsPerLuxel;

		pSideInfo->pSide->contents = textureref[mt].contents;
		pSideInfo->pSide->surf = pSideInfo->td.flags;
	}
	else if (!stricmp(szKey, "uaxis"))
	{
		int nRead = sscanf(szValue, "[%f %f %f %f] %f", &pSideInfo->td.UAxis[0], &pSideInfo->td.UAxis[1], &pSideInfo->td.UAxis[2], &pSideInfo->td.shift[0], &pSideInfo->td.textureWorldUnitsPerTexel[0]);
		if (nRead != 5)
		{
			g_MapError.ReportError("parsing U axis definition");
		}
	}
	else if (!stricmp(szKey, "vaxis"))
	{
		int nRead = sscanf(szValue, "[%f %f %f %f] %f", &pSideInfo->td.VAxis[0], &pSideInfo->td.VAxis[1], &pSideInfo->td.VAxis[2], &pSideInfo->td.shift[1], &pSideInfo->td.textureWorldUnitsPerTexel[1]);
		if (nRead != 5)
		{
			g_MapError.ReportError("parsing V axis definition");
		}
	}
	else if (!stricmp(szKey, "lightmapscale"))
	{
		pSideInfo->td.lightmapWorldUnitsPerLuxel = atoi(szValue);
		if (pSideInfo->td.lightmapWorldUnitsPerLuxel == 0.0f)
		{
			g_MapError.ReportWarning("luxel size of 0");
			pSideInfo->td.lightmapWorldUnitsPerLuxel = g_defaultLuxelSize; 
		}
		pSideInfo->td.lightmapWorldUnitsPerLuxel *= g_luxelScale;
		if (pSideInfo->td.lightmapWorldUnitsPerLuxel < g_minLuxelScale)
		{
			pSideInfo->td.lightmapWorldUnitsPerLuxel = g_minLuxelScale;
		}
	}
	else if (!stricmp(szKey, "contents"))
	{
		pSideInfo->pSide->contents |= atoi(szValue);
	}
	else if (!stricmp(szKey, "flags"))
	{
		pSideInfo->td.flags |= atoi(szValue);
		pSideInfo->pSide->surf = pSideInfo->td.flags;
	}
	else if (!stricmp(szKey, "id"))
	{
		pSideInfo->pSide->id = atoi( szValue );
	}
	else if (!stricmp(szKey, "smoothing_groups"))
	{
		pSideInfo->pSide->smoothingGroups = atoi( szValue );
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: Reads the connections chunk of the entity.
// Input  : pFile - Chunk file to load from.
//			pLoadEntity - Structure to receive loaded entity information.
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadConnectionsCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity)
{
	return(pFile->ReadChunk((KeyHandler_t)LoadConnectionsKeyCallback, pLoadEntity));
}


//-----------------------------------------------------------------------------
// Purpose: Parses a key/value pair from the entity connections chunk.
// Input  : szKey - Key indicating the name of the entity output.
//			szValue - Comma delimited fields in the following format:
//				<target>,<input>,<parameter>,<delay>,<times to fire>
//			pLoadEntity - Structure to receive loaded entity information.
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadConnectionsKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity)
{
	return g_LoadingMap->LoadConnectionsKeyCallback( szKey, szValue, pLoadEntity );
}

ChunkFileResult_t CMapFile::LoadConnectionsKeyCallback(const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity)
{
	//
	// Create new input and fill it out.
	//
	epair_t *pOutput = new epair_t;

	pOutput->key = new char [strlen(szKey) + 1];
	pOutput->value = new char [strlen(szValue) + 1];

	strcpy(pOutput->key, szKey);
	strcpy(pOutput->value, szValue);

	m_ConnectionPairs = new CConnectionPairs( pOutput, m_ConnectionPairs );
	
	//
	// Append it to the end of epairs list.
	//
	pOutput->next = NULL;

	if (!pLoadEntity->pEntity->epairs)
	{
		pLoadEntity->pEntity->epairs = pOutput;
	}
	else
	{
		epair_t *ep;
		for ( ep = pLoadEntity->pEntity->epairs; ep->next != NULL; ep = ep->next )
		{
		}
		ep->next = pOutput;
	}

	return(ChunkFile_Ok);
}


ChunkFileResult_t LoadSolidCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity)
{
	return g_LoadingMap->LoadSolidCallback( pFile, pLoadEntity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			pParent - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t CMapFile::LoadSolidCallback(CChunkFile *pFile, LoadEntity_t *pLoadEntity)
{
	if (nummapbrushes == MAX_MAP_BRUSHES)
	{
		g_MapError.ReportError ("nummapbrushes == MAX_MAP_BRUSHES");
	}

	mapbrush_t *b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = num_entities-1;
	b->brushnum = nummapbrushes - pLoadEntity->pEntity->firstbrush;

	LoadSide_t SideInfo;
	SideInfo.pBrush = b;
	SideInfo.nSideIndex = 0;
	SideInfo.nBaseContents = pLoadEntity->nBaseContents;
	SideInfo.nBaseFlags = pLoadEntity->nBaseFlags;

	//
	// Set up handlers for the subchunks that we are interested in.
	//
	CChunkHandlerMap Handlers;
	Handlers.AddHandler("side", (ChunkHandler_t)::LoadSideCallback, &SideInfo);

	//
	// Read the solid chunk.
	//
	pFile->PushHandlers(&Handlers);
	ChunkFileResult_t eResult = pFile->ReadChunk((KeyHandler_t)LoadSolidKeyCallback, b);
	pFile->PopHandlers();

	if (eResult == ChunkFile_Ok)
	{
		// get the content for the entire brush
		b->contents = BrushContents (b);

		// allow detail brushes to be removed 
		if (nodetail && (b->contents & CONTENTS_DETAIL) && !HasDispInfo( b ) )
		{
			b->numsides = 0;
			return(ChunkFile_Ok);
		}

		// allow water brushes to be removed
		if (nowater && (b->contents & MASK_WATER) )
		{
			b->numsides = 0;
			return(ChunkFile_Ok);
		}

		// create windings for sides and bounds for brush
		MakeBrushWindings (b);

		//
		// brushes that will not be visible at all will never be
		// used as bsp splitters
		//
		// only do this on the world entity
		//
		if ( b->entitynum == 0 )
		{
			if (b->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
			{
				if ( g_ClipTexinfo < 0 )
				{
					g_ClipTexinfo = b->original_sides[0].texinfo;
				}
				c_clipbrushes++;
				for (int i=0 ; i<b->numsides ; i++)
				{
					b->original_sides[i].texinfo = TEXINFO_NODE;
				}
			}
		}

		//
		// origin brushes are removed, but they set
		// the rotation origin for the rest of the brushes
		// in the entity.  After the entire entity is parsed,
		// the planenums and texinfos will be adjusted for
		// the origin brush
		//
		if (b->contents & CONTENTS_ORIGIN)
		{
			char	string[32];
			Vector	origin;

			if (num_entities == 1)
			{
				Error("Brush %i: origin brushes not allowed in world", b->id);
			}

			VectorAdd (b->mins, b->maxs, origin);
			VectorScale (origin, 0.5, origin);

			sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
			SetKeyValue (&entities[b->entitynum], "origin", string);

			VectorCopy (origin, entities[b->entitynum].origin);

			// don't keep this brush
			b->numsides = 0;

			return(ChunkFile_Ok);
		}

#ifdef VSVMFIO
		if ( CVmfImport::GetVmfImporter() )
		{
			CVmfImport::GetVmfImporter()->MapBrushToMayaCallback( b );
		}
#endif // VSVMFIO

		//
		// find a map brushes with displacement surfaces and remove them from the "world"
		//
		if( HasDispInfo( b ) )
		{
			// add the base face data to the displacement surface
			DispGetFaceInfo( b );			

			// don't keep this brush
			b->numsides = 0;

			return( ChunkFile_Ok );
		}

		AddBrushBevels (b);

		nummapbrushes++;
		pLoadEntity->pEntity->numbrushes++;		
	}
	else
	{
		return eResult;
	}

	return(ChunkFile_Ok);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pFile - 
//			parent - 
// Output : ChunkFileResult_t
//-----------------------------------------------------------------------------
ChunkFileResult_t LoadSolidKeyCallback(const char *szKey, const char *szValue, mapbrush_t *pLoadBrush)
{
	if (!stricmp(szKey, "id"))
	{
		pLoadBrush->id = atoi(szValue);
		g_MapError.BrushState(pLoadBrush->id);
	}

	return ChunkFile_Ok;
}


/*
================
TestExpandBrushes

Expands all the brush planes and saves a new map out
================
*/
void CMapFile::TestExpandBrushes (void)
{
	FILE	*f;
	side_t	*s;
	int		i, j, bn;
	winding_t	*w;
	char	*name = "expanded.map";
	mapbrush_t	*brush;
	vec_t	dist;

	Msg ("writing %s\n", name);
	f = fopen (name, "wb");
	if (!f)
		Error ("Can't write %s\b", name);

	fprintf (f, "{\n\"classname\" \"worldspawn\"\n");
	fprintf( f, "\"mapversion\" \"220\"\n\"sounds\" \"1\"\n\"MaxRange\" \"4096\"\n\"mapversion\" \"220\"\n\"wad\" \"vert.wad;dev.wad;generic.wad;spire.wad;urb.wad;cit.wad;water.wad\"\n" );


	for (bn=0 ; bn<nummapbrushes ; bn++)
	{
		brush = &mapbrushes[bn];
		fprintf (f, "{\n");
		for (i=0 ; i<brush->numsides ; i++)
		{
			s = brush->original_sides + i;
			dist = mapplanes[s->planenum].dist;
			for (j=0 ; j<3 ; j++)
				dist += fabs( 16 * mapplanes[s->planenum].normal[j] );

			w = BaseWindingForPlane (mapplanes[s->planenum].normal, dist);

			fprintf (f,"( %i %i %i ) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]);

			fprintf (f, "%s [ 0 0 1 -512 ] [ 0 -1 0 -256 ] 0 1 1 \n", 
				TexDataStringTable_GetString( GetTexData( texinfo[s->texinfo].texdata )->nameStringTableID ) );

			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

	Error ("can't proceed after expanding brushes");
}


//-----------------------------------------------------------------------------
// Purpose: load in the displacement info "chunk" from the .map file into the
//          vbsp map displacement info data structure
//  Output: return the pointer to the displacement map
//-----------------------------------------------------------------------------
mapdispinfo_t *ParseDispInfoChunk( void )
{
    int             i, j;
    int             vertCount;
    mapdispinfo_t   *pMapDispInfo;

    //
    // check to see if we exceeded the maximum displacement info list size
    //
    if( nummapdispinfo > MAX_MAP_DISPINFO )
        g_MapError.ReportError( "ParseDispInfoChunk: nummapdispinfo > MAX_MAP_DISPINFO");

    // get a pointer to the next available displacement info slot
    pMapDispInfo = &mapdispinfo[nummapdispinfo];
    nummapdispinfo++;

    //
    // get the chunk opener - "{"
    //
    GetToken( false );
    if( strcmp( token, "{" ) )
        g_MapError.ReportError( "ParseDispInfoChunk: Illegal Chunk! - {" );

    //
    //
    // get the displacement info attribs
    //
    //

    // power
    GetToken( true );
    pMapDispInfo->power = atoi( token );

    // u and v mapping axes
    for( i = 0; i < 2; i++ )
    {
        GetToken( false );
        if( strcmp( token, "[" ) )
            g_MapError.ReportError( "ParseDispInfoChunk: Illegal Chunk! - [" );

        for( j = 0; j < 3; j++ )
        {
            GetToken( false );

            if( i == 0 )
            {
                pMapDispInfo->uAxis[j] = atof( token );
            }
            else
            {
                pMapDispInfo->vAxis[j] = atof( token );
            }
        }

        GetToken( false );
        if( strcmp( token, "]" ) )
            g_MapError.ReportError( "ParseDispInfoChunk: Illegal Chunk! - ]" );
    }

    // max displacement value
   	if( g_nMapFileVersion < 350 )
	{
		GetToken( false );
		pMapDispInfo->maxDispDist = atof( token );
	}

    // minimum tesselation value
    GetToken( false );
    pMapDispInfo->minTess = atoi( token );

    // light smoothing angle
    GetToken( false );
    pMapDispInfo->smoothingAngle = atof( token );

    //
    // get the displacement info displacement normals
    //
    GetToken( true );
    pMapDispInfo->vectorDisps[0][0] = atof( token );
    GetToken( false );
    pMapDispInfo->vectorDisps[0][1] = atof( token );
    GetToken( false );
    pMapDispInfo->vectorDisps[0][2] = atof( token );

    vertCount = ( ( ( 1 << pMapDispInfo->power ) + 1 ) * ( ( 1 << pMapDispInfo->power ) + 1 ) );
    for( i = 1; i < vertCount; i++ )
    {
        GetToken( false );
        pMapDispInfo->vectorDisps[i][0] = atof( token );
        GetToken( false );
        pMapDispInfo->vectorDisps[i][1] = atof( token );
        GetToken( false );
        pMapDispInfo->vectorDisps[i][2] = atof( token );
    }

    //
    // get the displacement info displacement values
    //
    GetToken( true );
    pMapDispInfo->dispDists[0] = atof( token );

    for( i = 1; i < vertCount; i++ )
    {
        GetToken( false );
        pMapDispInfo->dispDists[i] = atof( token );
    }

    //
    // get the chunk closer - "}"
    //
    GetToken( true );
    if( strcmp( token, "}" ) )
        g_MapError.ReportError( "ParseDispInfoChunk: Illegal Chunk! - }" );
    
    // return the index of the displacement info slot
    return pMapDispInfo;
}


