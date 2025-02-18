//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines and structures for the BSP file format.
//
// $NoKeywords: $
//=============================================================================//

#ifndef BSPFILE_H
#define BSPFILE_H
#pragma once

#ifndef MATHLIB_H
#include "mathlib/mathlib.h"
#endif

#include "datamap.h"
#include "mathlib/bumpvects.h"
#include "mathlib/compressed_light_cube.h"

// little-endian "VBSP"
#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'V')		

// MINBSPVERSION is the minimum acceptable version.  The engine will load MINBSPVERSION through BSPVERSION
#define MINBSPVERSION 19
#define BSPVERSION 20


// This needs to match the value in gl_lightmap.h
// Need to dynamically allocate the weights and light values in radial_t to make this variable.
#define MAX_BRUSH_LIGHTMAP_DIM_WITHOUT_BORDER 32
// This is one more than what vbsp cuts for to allow for rounding errors
#define MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER	35

// We can have larger lightmaps on displacements
#define MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER	125
#define MAX_DISP_LIGHTMAP_DIM_INCLUDING_BORDER	128


// This is the actual max.. (change if you change the brush lightmap dim or disp lightmap dim
#define MAX_LIGHTMAP_DIM_WITHOUT_BORDER		MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER
#define MAX_LIGHTMAP_DIM_INCLUDING_BORDER	MAX_DISP_LIGHTMAP_DIM_INCLUDING_BORDER

#define	MAX_LIGHTSTYLES	64


// upper design bounds
#define MIN_MAP_DISP_POWER		2	// Minimum and maximum power a displacement can be.
#define MAX_MAP_DISP_POWER		4	

// Max # of neighboring displacement touching a displacement's corner.
#define MAX_DISP_CORNER_NEIGHBORS	4

#define NUM_DISP_POWER_VERTS(power)	( ((1 << (power)) + 1) * ((1 << (power)) + 1) )
#define NUM_DISP_POWER_TRIS(power)	( (1 << (power)) * (1 << (power)) * 2 )

#if !defined( BSP_USE_LESS_MEMORY )
// Common limits
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS					1024
#define	MAX_MAP_BRUSHES					8192
#define	MAX_MAP_ENTITIES				8192
#define	MAX_MAP_TEXINFO					12288
#define MAX_MAP_TEXDATA					2048
#define MAX_MAP_DISPINFO				2048
#define MAX_MAP_DISP_VERTS				( MAX_MAP_DISPINFO * ((1<<MAX_MAP_DISP_POWER)+1) * ((1<<MAX_MAP_DISP_POWER)+1) )
#define MAX_MAP_DISP_TRIS				( (1 << MAX_MAP_DISP_POWER) * (1 << MAX_MAP_DISP_POWER) * 2 )
#define MAX_DISPVERTS					NUM_DISP_POWER_VERTS( MAX_MAP_DISP_POWER )
#define MAX_DISPTRIS					NUM_DISP_POWER_TRIS( MAX_MAP_DISP_POWER )
#define	MAX_MAP_AREAS					256
#define MAX_MAP_AREA_BYTES				(MAX_MAP_AREAS/8)
#define	MAX_MAP_AREAPORTALS				1024
// Planes come in pairs, thus an even number.
#define	MAX_MAP_PLANES					65536
#define	MAX_MAP_NODES					65536
#define	MAX_MAP_BRUSHSIDES				65536
#define	MAX_MAP_LEAFS					65536
#define	MAX_MAP_VERTS					65536
#define MAX_MAP_VERTNORMALS				256000
#define MAX_MAP_VERTNORMALINDICES		256000
#define	MAX_MAP_FACES					65536
#define	MAX_MAP_LEAFFACES				65536
#define	MAX_MAP_LEAFBRUSHES 			65536
#define	MAX_MAP_PORTALS					65536
#define MAX_MAP_CLUSTERS				65536
#define MAX_MAP_LEAFWATERDATA			32768
#define MAX_MAP_PORTALVERTS				128000
#define	MAX_MAP_EDGES					256000
#define	MAX_MAP_SURFEDGES				512000
#define	MAX_MAP_LIGHTING				0x1000000
#define	MAX_MAP_VISIBILITY				0x1000000			// increased BSPVERSION 7
#define	MAX_MAP_TEXTURES				1024
#define MAX_MAP_WORLDLIGHTS				8192
#define MAX_MAP_CUBEMAPSAMPLES			1024
#define MAX_MAP_OVERLAYS				512 
#define MAX_MAP_WATEROVERLAYS			16384
#define MAX_MAP_TEXDATA_STRING_DATA		256000
#define MAX_MAP_TEXDATA_STRING_TABLE	65536
// this is stuff for trilist/tristrips, etc.
#define MAX_MAP_PRIMITIVES				32768
#define MAX_MAP_PRIMVERTS				65536
#define MAX_MAP_PRIMINDICES				65536

#else

// Xbox 360 - Force static arrays to be very small
#define	MAX_MAP_MODELS					2
#define	MAX_MAP_BRUSHES					2
#define	MAX_MAP_ENTITIES				2
#define	MAX_MAP_TEXINFO					2
#define MAX_MAP_TEXDATA					2
#define MAX_MAP_DISPINFO				2
#define MAX_MAP_DISP_VERTS				( MAX_MAP_DISPINFO * ((1<<MAX_MAP_DISP_POWER)+1) * ((1<<MAX_MAP_DISP_POWER)+1) )
#define MAX_MAP_DISP_TRIS				( (1 << MAX_MAP_DISP_POWER) * (1 << MAX_MAP_DISP_POWER) * 2 )
#define MAX_DISPVERTS					NUM_DISP_POWER_VERTS( MAX_MAP_DISP_POWER )
#define MAX_DISPTRIS					NUM_DISP_POWER_TRIS( MAX_MAP_DISP_POWER )
#define	MAX_MAP_AREAS					2
#define MAX_MAP_AREA_BYTES				2
#define	MAX_MAP_AREAPORTALS				2
#define	MAX_MAP_PLANES					2
#define	MAX_MAP_NODES					2
#define	MAX_MAP_BRUSHSIDES				2
#define	MAX_MAP_LEAFS					2
#define	MAX_MAP_VERTS					2
#define MAX_MAP_VERTNORMALS				2
#define MAX_MAP_VERTNORMALINDICES		2
#define	MAX_MAP_FACES					2
#define	MAX_MAP_LEAFFACES				2
#define	MAX_MAP_LEAFBRUSHES				2
#define	MAX_MAP_PORTALS					2
#define MAX_MAP_CLUSTERS				2
#define MAX_MAP_LEAFWATERDATA			2
#define MAX_MAP_PORTALVERTS				2
#define	MAX_MAP_EDGES					2
#define	MAX_MAP_SURFEDGES				2
#define	MAX_MAP_LIGHTING				2
#define	MAX_MAP_VISIBILITY				2
#define	MAX_MAP_TEXTURES				2
#define MAX_MAP_WORLDLIGHTS				2
#define MAX_MAP_CUBEMAPSAMPLES			2
#define MAX_MAP_OVERLAYS				2 
#define MAX_MAP_WATEROVERLAYS			2
#define MAX_MAP_TEXDATA_STRING_DATA		2
#define MAX_MAP_TEXDATA_STRING_TABLE	2
#define MAX_MAP_PRIMITIVES				2
#define MAX_MAP_PRIMVERTS				2
#define MAX_MAP_PRIMINDICES				2

#endif // BSP_USE_LESS_MEMORY

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024


// ------------------------------------------------------------------------------------------------ //
// Displacement neighbor rules
// ------------------------------------------------------------------------------------------------ //
//
// Each displacement is considered to be in its own space:
//
//               NEIGHBOREDGE_TOP
//
//                   1 --- 2
//                   |     |
// NEIGHBOREDGE_LEFT |     | NEIGHBOREDGE_RIGHT
//                   |     |
//                   0 --- 3
//
//   			NEIGHBOREDGE_BOTTOM
//
//
// Edge edge of a displacement can have up to two neighbors. If it only has one neighbor
// and the neighbor fills the edge, then SubNeighbor 0 uses CORNER_TO_CORNER (and SubNeighbor 1
// is undefined).
//
// CORNER_TO_MIDPOINT means that it spans [bottom edge,midpoint] or [left edge,midpoint] depending
// on which edge you're on.
//
// MIDPOINT_TO_CORNER means that it spans [midpoint,top edge] or [midpoint,right edge] depending
// on which edge you're on.
//
// Here's an illustration (where C2M=CORNER_TO_MIDPOINT and M2C=MIDPOINT_TO_CORNER
//
//
//				 C2M			  M2C
//
//       1 --------------> x --------------> 2
//
//       ^                                   ^
//       |                                   |
//       |                                   |
//  M2C  |                                   |	M2C
//       |                                   |
//       |                                   |
//
//       x                 x                 x 
//
//       ^                                   ^
//       |                                   |
//       |                                   |
//  C2M  |                                   |	C2M
//       |                                   |
//       |                                   |
// 
//       0 --------------> x --------------> 3
//
//               C2M			  M2C
//
//
// The CHILDNODE_ defines can be used to refer to a node's child nodes (this is for when you're
// recursing into the node tree inside a displacement):
//
// ---------
// |   |   |
// | 1 | 0 |
// |   |   |
// |---x---|
// |   |   |
// | 2 | 3 |
// |   |   |
// ---------
// 
// ------------------------------------------------------------------------------------------------ //

// These can be used to index g_ChildNodeIndexMul.
enum
{
	CHILDNODE_UPPER_RIGHT=0,
	CHILDNODE_UPPER_LEFT=1,
	CHILDNODE_LOWER_LEFT=2,
	CHILDNODE_LOWER_RIGHT=3
};


// Corner indices. Used to index m_CornerNeighbors.
enum
{
	CORNER_LOWER_LEFT=0,
	CORNER_UPPER_LEFT=1,
	CORNER_UPPER_RIGHT=2,
	CORNER_LOWER_RIGHT=3
};


// These edge indices must match the edge indices of the CCoreDispSurface.
enum
{
	NEIGHBOREDGE_LEFT=0,
	NEIGHBOREDGE_TOP=1,
	NEIGHBOREDGE_RIGHT=2,
	NEIGHBOREDGE_BOTTOM=3
};


// These denote where one dispinfo fits on another.
// Note: tables are generated based on these indices so make sure to update
//       them if these indices are changed.
typedef enum
{
	CORNER_TO_CORNER=0,
	CORNER_TO_MIDPOINT=1,
	MIDPOINT_TO_CORNER=2
} NeighborSpan;


// These define relative orientations of displacement neighbors.
typedef enum
{
	ORIENTATION_CCW_0=0,
	ORIENTATION_CCW_90=1,
	ORIENTATION_CCW_180=2,
	ORIENTATION_CCW_270=3
} NeighborOrientation;


//=============================================================================

enum
{
	LUMP_ENTITIES					= 0,	// *
	LUMP_PLANES						= 1,	// *
	LUMP_TEXDATA					= 2,	// *
	LUMP_VERTEXES					= 3,	// *
	LUMP_VISIBILITY					= 4,	// *
	LUMP_NODES						= 5,	// *
	LUMP_TEXINFO					= 6,	// *
	LUMP_FACES						= 7,	// *
	LUMP_LIGHTING					= 8,	// *
	LUMP_OCCLUSION					= 9,
	LUMP_LEAFS						= 10,	// *
	LUMP_FACEIDS					= 11,
	LUMP_EDGES						= 12,	// *
	LUMP_SURFEDGES					= 13,	// *
	LUMP_MODELS						= 14,	// *
	LUMP_WORLDLIGHTS				= 15,	// 
	LUMP_LEAFFACES					= 16,	// *
	LUMP_LEAFBRUSHES				= 17,	// *
	LUMP_BRUSHES					= 18,	// *
	LUMP_BRUSHSIDES					= 19,	// *
	LUMP_AREAS						= 20,	// *
	LUMP_AREAPORTALS				= 21,	// *
	LUMP_UNUSED0					= 22,
	LUMP_UNUSED1					= 23,
	LUMP_UNUSED2					= 24,
	LUMP_UNUSED3					= 25,
	LUMP_DISPINFO					= 26,
	LUMP_ORIGINALFACES				= 27,
	LUMP_PHYSDISP					= 28,
	LUMP_PHYSCOLLIDE				= 29,
	LUMP_VERTNORMALS				= 30,
	LUMP_VERTNORMALINDICES			= 31,
	LUMP_DISP_LIGHTMAP_ALPHAS		= 32,
	LUMP_DISP_VERTS					= 33,		// CDispVerts
	LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,	// For each displacement
												//     For each lightmap sample
												//         byte for index
												//         if 255, then index = next byte + 255
												//         3 bytes for barycentric coordinates
	// The game lump is a method of adding game-specific lumps
	// FIXME: Eventually, all lumps could use the game lump system
	LUMP_GAME_LUMP					= 35,
	LUMP_LEAFWATERDATA				= 36,
	LUMP_PRIMITIVES					= 37,
	LUMP_PRIMVERTS					= 38,
	LUMP_PRIMINDICES				= 39,
	// A pak file can be embedded in a .bsp now, and the file system will search the pak
	//  file first for any referenced names, before deferring to the game directory 
	//  file system/pak files and finally the base directory file system/pak files.
	LUMP_PAKFILE					= 40,
	LUMP_CLIPPORTALVERTS			= 41,
	// A map can have a number of cubemap entities in it which cause cubemap renders
	// to be taken after running vrad.
	LUMP_CUBEMAPS					= 42,
	LUMP_TEXDATA_STRING_DATA		= 43,
	LUMP_TEXDATA_STRING_TABLE		= 44,
	LUMP_OVERLAYS					= 45,
	LUMP_LEAFMINDISTTOWATER			= 46,
	LUMP_FACE_MACRO_TEXTURE_INFO	= 47,
	LUMP_DISP_TRIS					= 48,
	LUMP_PHYSCOLLIDESURFACE			= 49,	// deprecated.  We no longer use win32-specific havok compression on terrain
	LUMP_WATEROVERLAYS              = 50,
	LUMP_LEAF_AMBIENT_INDEX_HDR		= 51,	// index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
	LUMP_LEAF_AMBIENT_INDEX         = 52,	// index of LUMP_LEAF_AMBIENT_LIGHTING

	// optional lumps for HDR
	LUMP_LIGHTING_HDR				= 53,
	LUMP_WORLDLIGHTS_HDR			= 54,
	LUMP_LEAF_AMBIENT_LIGHTING_HDR	= 55,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.
	LUMP_LEAF_AMBIENT_LIGHTING		= 56,	// NOTE: this data overrides part of the data stored in LUMP_LEAFS.

	LUMP_XZIPPAKFILE				= 57,   // deprecated. xbox 1: xzip version of pak file
	LUMP_FACES_HDR					= 58,	// HDR maps may have different face data.
	LUMP_MAP_FLAGS                  = 59,   // extended level-wide flags. not present in all levels
	LUMP_OVERLAY_FADES				= 60,	// Fade distances for overlays
};


// Lumps that have versions are listed here
enum
{
	LUMP_LIGHTING_VERSION          = 1,
	LUMP_FACES_VERSION             = 1,
	LUMP_OCCLUSION_VERSION         = 2,
	LUMP_LEAFS_VERSION			   = 1,
	LUMP_LEAF_AMBIENT_LIGHTING_VERSION = 1,
};


#define	HEADER_LUMPS		64

#include "zip_uncompressed.h"

struct lump_t
{
	DECLARE_BYTESWAP_DATADESC();
	int		fileofs, filelen;
	int		version;		// default to zero
	// this field was char fourCC[4] previously, but was unused, favoring the LUMP IDs above instead. It has been
	// repurposed for compression.  0 implies the lump is not compressed.
	int		uncompressedSize; // default to zero
};


struct dheader_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			ident;
	int			version;
	lump_t		lumps[HEADER_LUMPS];
	int			mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)
};

// level feature flags
#define LVLFLAGS_BAKED_STATIC_PROP_LIGHTING_NONHDR 0x00000001	// was processed by vrad with -staticproplighting, no hdr data
#define LVLFLAGS_BAKED_STATIC_PROP_LIGHTING_HDR    0x00000002   // was processed by vrad with -staticproplighting, in hdr

struct dflagslump_t
{
	DECLARE_BYTESWAP_DATADESC();
	uint32 m_LevelFlags;						// LVLFLAGS_xxx
};

struct lumpfileheader_t
{
	int				lumpOffset;
	int				lumpID;
	int				lumpVersion;	
	int				lumpLength;
	int				mapRevision;				// the map's revision (iteration, version) number (added BSPVERSION 6)
};

struct dgamelumpheader_t
{
	DECLARE_BYTESWAP_DATADESC();
	int lumpCount;

	// dgamelump_t follow this
};

// This is expected to be a four-CC code ('lump')
typedef int GameLumpId_t;

// game lump is compressed, filelen reflects original size
// use next entry fileofs to determine actual disk lump compressed size
// compression stage ensures a terminal null dictionary entry
#define GAMELUMPFLAG_COMPRESSED	0x0001

struct dgamelump_t
{
	DECLARE_BYTESWAP_DATADESC();
	GameLumpId_t	id;
	unsigned short	flags;
	unsigned short	version;
	int				fileofs;
	int				filelen;
};

extern int g_MapRevision;

struct dmodel_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector		mins, maxs;
	Vector		origin;					// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};

struct dphysmodel_t
{
	DECLARE_BYTESWAP_DATADESC()
	int			modelIndex;
	int			dataSize;
	int			keydataSize;
	int			solidCount;
};

// contains the binary blob for each displacement surface's virtual hull
struct dphysdisp_t
{
	DECLARE_BYTESWAP_DATADESC()
	unsigned short numDisplacements;
	//unsigned short dataSize[numDisplacements];
};

struct dvertex_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector	point;
};

// planes (x&~1) and (x&~1)+1 are always opposites
struct dplane_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector	normal;
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};

#ifndef BSPFLAGS_H
#include "bspflags.h"
#endif

struct dnode_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
	short			area;		// If all leaves below this node are in the same area, then
								// this is the area index. If not, this is -1.
};

typedef struct texinfo_s
{
	DECLARE_BYTESWAP_DATADESC();
	float		textureVecsTexelsPerWorldUnits[2][4];			// [s/t][xyz offset]
	float		lightmapVecsLuxelsPerWorldUnits[2][4];			// [s/t][xyz offset] - length is in units of texels/area
	int			flags;				// miptex flags + overrides
	int			texdata;			// Pointer to texture name, size, etc.
} texinfo_t;

#define TEXTURE_NAME_LENGTH	 128			// changed from 64 BSPVERSION 8

struct dtexdata_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector		reflectivity;
	int			nameStringTableID;				// index into g_StringTable for the texture name
	int			width, height;					// source image
	int			view_width, view_height;		//
};


//-----------------------------------------------------------------------------
// Occluders are simply polygons
//-----------------------------------------------------------------------------
// Flags field of doccluderdata_t
enum
{
	OCCLUDER_FLAGS_INACTIVE = 0x1,
};

struct doccluderdata_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			flags;
	int			firstpoly;				// index into doccluderpolys
	int			polycount;
	Vector		mins;
	Vector		maxs;
	int			area;
};

struct doccluderdataV1_t
{
	int			flags;
	int			firstpoly;				// index into doccluderpolys
	int			polycount;
	Vector		mins;
	Vector		maxs;
};

struct doccluderpolydata_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			firstvertexindex;		// index into doccludervertindices
	int			vertexcount;
	int			planenum;
};


// NOTE: see the section above titled "displacement neighbor rules".
struct CDispSubNeighbor
{
public:
	DECLARE_BYTESWAP_DATADESC();
	unsigned short		GetNeighborIndex() const		{ return m_iNeighbor; }
	NeighborSpan		GetSpan() const					{ return (NeighborSpan)m_Span; }
	NeighborSpan		GetNeighborSpan() const			{ return (NeighborSpan)m_NeighborSpan; }
	NeighborOrientation	GetNeighborOrientation() const	{ return (NeighborOrientation)m_NeighborOrientation; }

	bool				IsValid() const				{ return m_iNeighbor != 0xFFFF; }
	void				SetInvalid()				{ m_iNeighbor = 0xFFFF; }


public:
	unsigned short		m_iNeighbor;		// This indexes into ddispinfos.
											// 0xFFFF if there is no neighbor here.

	unsigned char		m_NeighborOrientation;		// (CCW) rotation of the neighbor wrt this displacement.

	// These use the NeighborSpan type.
	unsigned char		m_Span;						// Where the neighbor fits onto this side of our displacement.
	unsigned char		m_NeighborSpan;				// Where we fit onto our neighbor.
};


// NOTE: see the section above titled "displacement neighbor rules".
class CDispNeighbor
{
public:
	DECLARE_BYTESWAP_DATADESC();
	void				SetInvalid()	{ m_SubNeighbors[0].SetInvalid(); m_SubNeighbors[1].SetInvalid(); }
	
	// Returns false if there isn't anything touching this edge.
	bool				IsValid()		{ return m_SubNeighbors[0].IsValid() || m_SubNeighbors[1].IsValid(); }


public:
	// Note: if there is a neighbor that fills the whole side (CORNER_TO_CORNER),
	//       then it will always be in CDispNeighbor::m_Neighbors[0]
	CDispSubNeighbor	m_SubNeighbors[2];
};


class CDispCornerNeighbors
{
public:
	DECLARE_BYTESWAP_DATADESC();
	void			SetInvalid()	{ m_nNeighbors = 0; }


public:
	unsigned short	m_Neighbors[MAX_DISP_CORNER_NEIGHBORS];	// indices of neighbors.
	unsigned char	m_nNeighbors;
};


class CDispVert
{
public:
	DECLARE_BYTESWAP_DATADESC();
	Vector		m_vVector;		// Vector field defining displacement volume.
	float		m_flDist;		// Displacement distances.
	float		m_flAlpha;		// "per vertex" alpha values.
};

#define DISPTRI_TAG_SURFACE			(1<<0)
#define DISPTRI_TAG_WALKABLE		(1<<1)
#define DISPTRI_TAG_BUILDABLE		(1<<2)
#define DISPTRI_FLAG_SURFPROP1		(1<<3)
#define DISPTRI_FLAG_SURFPROP2		(1<<4)
#define DISPTRI_TAG_REMOVE			(1<<5)

class CDispTri
{
public:
	DECLARE_BYTESWAP_DATADESC();
	unsigned short m_uiTags;		// Displacement triangle tags.
};

class ddispinfo_t
{
public:
	DECLARE_BYTESWAP_DATADESC();
	int			NumVerts() const		{ return NUM_DISP_POWER_VERTS(power); }
	int			NumTris() const			{ return NUM_DISP_POWER_TRIS(power); }

public:
	Vector		startPosition;						// start position used for orientation -- (added BSPVERSION 6)
	int			m_iDispVertStart;					// Index into LUMP_DISP_VERTS.
	int			m_iDispTriStart;					// Index into LUMP_DISP_TRIS.

    int         power;                              // power - indicates size of map (2^power + 1)
    int         minTess;                            // minimum tesselation allowed
    float       smoothingAngle;                     // lighting smoothing angle
    int         contents;                           // surface contents

	unsigned short	m_iMapFace;						// Which map face this displacement comes from.
	
	int			m_iLightmapAlphaStart;				// Index into ddisplightmapalpha.
													// The count is m_pParent->lightmapTextureSizeInLuxels[0]*m_pParent->lightmapTextureSizeInLuxels[1].

	int			m_iLightmapSamplePositionStart;		// Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.

	CDispNeighbor			m_EdgeNeighbors[4];		// Indexed by NEIGHBOREDGE_ defines.
	CDispCornerNeighbors	m_CornerNeighbors[4];	// Indexed by CORNER_ defines.

	enum unnamed { ALLOWEDVERTS_SIZE = PAD_NUMBER( MAX_DISPVERTS, 32 ) / 32 };
	unsigned long	m_AllowedVerts[ALLOWEDVERTS_SIZE];	// This is built based on the layout and sizes of our neighbors
														// and tells us which vertices are allowed to be active.
};


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct dedge_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	v[2];		// vertex numbers
};

#define	MAXLIGHTMAPS	4

enum dprimitive_type
{
	PRIM_TRILIST=0,
	PRIM_TRISTRIP=1,
};

struct dprimitive_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned char type;
	unsigned short	firstIndex;
	unsigned short	indexCount;
	unsigned short	firstVert;
	unsigned short	vertCount;
};

struct dprimvert_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector		pos;
};

struct dface_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	planenum;
	byte		side;	// faces opposite to the node's plane direction
	byte		onNode; // 1 of on node, 0 if in leaf

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;
	// This is a union under the assumption that a fog volume boundary (ie. water surface)
	// isn't a displacement map.
	// FIXME: These should be made a union with a flags or type field for which one it is
	// if we can add more to this.
//	union
//	{
	    short       dispinfo;
		// This is only for surfaces that are the boundaries of fog volumes
		// (ie. water surfaces)
		// All of the rest of the surfaces can look at their leaf to find out
		// what fog volume they are in.
		short		surfaceFogVolumeID;
//	};

	// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
    float       area;

	// TODO: make these unsigned chars?
	int			m_LightmapTextureMinsInLuxels[2];
	int			m_LightmapTextureSizeInLuxels[2];

	int origFace;				// reference the original face this face was derived from


public:

	unsigned short GetNumPrims() const;
	void SetNumPrims( unsigned short nPrims );
	bool AreDynamicShadowsEnabled();
	void SetDynamicShadowsEnabled( bool bEnabled );

	// non-polygon primitives (strips and lists)
private:
	unsigned short m_NumPrims;	// Top bit, if set, disables shadows on this surface (this is why there are accessors).

public:
	unsigned short	firstPrimID; 
	
	unsigned int	smoothingGroups;
};


inline unsigned short dface_t::GetNumPrims() const
{
	return m_NumPrims & 0x7FFF;
}

inline void dface_t::SetNumPrims( unsigned short nPrims )
{
	Assert( (nPrims & 0x8000) == 0 );
	m_NumPrims &= ~0x7FFF;
	m_NumPrims |= (nPrims & 0x7FFF);
}

inline bool dface_t::AreDynamicShadowsEnabled()
{
	return (m_NumPrims & 0x8000) == 0;
}

inline void dface_t::SetDynamicShadowsEnabled( bool bEnabled )
{
	if ( bEnabled )
		m_NumPrims &= ~0x8000;
	else
		m_NumPrims |= 0x8000;
}

struct dfaceid_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	hammerfaceid;
};


// NOTE: Only 7-bits stored!!!
#define LEAF_FLAGS_SKY			0x01		// This leaf has 3D sky in its PVS
#define LEAF_FLAGS_RADIAL		0x02		// This leaf culled away some portals due to radial vis
#define LEAF_FLAGS_SKY2D		0x04		// This leaf has 2D sky in its PVS

#if defined( _X360 )
#pragma bitfield_order( push, lsb_to_msb )
#endif
#pragma warning( disable:4201 )	// C4201: nonstandard extension used: nameless struct/union
struct dleaf_version_0_t
{
	DECLARE_BYTESWAP_DATADESC();
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

	BEGIN_BITFIELD( bf );
	short			area:9;
	short			flags:7;			// Per leaf flags.
	END_BITFIELD();

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water

	// Precaculated light info for entities.
	CompressedLightCube m_AmbientLighting;
};

// version 1
struct dleaf_t
{
	DECLARE_BYTESWAP_DATADESC();
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;

	BEGIN_BITFIELD( bf );
	short			area:9;
	short			flags:7;			// Per leaf flags.
	END_BITFIELD();

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
	short			leafWaterDataID; // -1 for not in water

	// NOTE: removed this for version 1 and moved into separate lump "LUMP_LEAF_AMBIENT_LIGHTING" or "LUMP_LEAF_AMBIENT_LIGHTING_HDR"
	// Precaculated light info for entities.
//	CompressedLightCube m_AmbientLighting;
};
#pragma warning( default:4201 )	// C4201: nonstandard extension used: nameless struct/union
#if defined( _X360 )
#pragma bitfield_order( pop )
#endif

// each leaf contains N samples of the ambient lighting
// each sample contains a cube of ambient light projected on to each axis
// and a sampling position encoded as a 0.8 fraction (mins=0,maxs=255) of the leaf's bounding box
struct dleafambientlighting_t
{
	DECLARE_BYTESWAP_DATADESC();
	CompressedLightCube	cube;
	byte x;		// fixed point fraction of leaf bounds
	byte y;		// fixed point fraction of leaf bounds
	byte z;		// fixed point fraction of leaf bounds
	byte pad;	// unused
};

struct dleafambientindex_t
{
	DECLARE_BYTESWAP_DATADESC();

	unsigned short ambientSampleCount;
	unsigned short firstAmbientSample;
};

struct dbrushside_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	planenum;		// facing out of the leaf
	short			texinfo;
	short			dispinfo;		// displacement info (BSPVERSION 7)
	short			bevel;			// is the side a bevel plane? (BSPVERSION 7)
};

struct dbrush_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			firstside;
	int			numsides;
	int			contents;
};

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PAS	1
struct dvis_t
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dareaportal_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	m_PortalKey;		// Entities have a key called portalnumber (and in vbsp a variable
									// called areaportalnum) which is used
									// to bind them to the area portals by comparing with this value.
	
	unsigned short	otherarea;		// The area this portal looks into.
	
	unsigned short	m_FirstClipPortalVert;	// Portal geometry.
	unsigned short	m_nClipPortalVerts;

	int				planenum;
};


struct darea_t
{
	DECLARE_BYTESWAP_DATADESC();
	int		numareaportals;
	int		firstareaportal;
};

struct dleafwaterdata_t
{
	DECLARE_BYTESWAP_DATADESC();
	float	surfaceZ;
	float	minZ;
	short	surfaceTexInfoID;
};

class CFaceMacroTextureInfo
{
public:
	DECLARE_BYTESWAP_DATADESC();
	// This looks up into g_TexDataStringTable, which looks up into g_TexDataStringData.
	// 0xFFFF if the face has no macro texture.
	unsigned short m_MacroTextureNameID;	
};

// lights that were used to illuminate the world
enum emittype_t
{
	emit_surface,		// 90 degree spotlight
	emit_point,			// simple point light source
	emit_spotlight,		// spotlight with penumbra
	emit_skylight,		// directional light with no falloff (surface must trace to SKY texture)
	emit_quakelight,	// linear falloff, non-lambertian
	emit_skyambient,	// spherical light source with no falloff (surface must trace to SKY texture)
};


// Flags for dworldlight_t::flags
#define DWL_FLAGS_INAMBIENTCUBE		0x0001	// This says that the light was put into the per-leaf ambient cubes.


struct dworldlight_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector		origin;
	Vector		intensity;
	Vector		normal;			// for surfaces and spotlights
	int			cluster;
	emittype_t	type;
    int			style;
	float		stopdot;		// start of penumbra for emit_spotlight
	float		stopdot2;		// end of penumbra for emit_spotlight
	float		exponent;		// 
	float		radius;			// cutoff distance
	// falloff for emit_spotlight + emit_point: 
	// 1 / (constant_attn + linear_attn * dist + quadratic_attn * dist^2)
	float		constant_attn;	
	float		linear_attn;
	float		quadratic_attn;
	int			flags;			// Uses a combination of the DWL_FLAGS_ defines.
	int			texinfo;		// 
	int			owner;			// entity that this light it relative to
};

struct dcubemapsample_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			origin[3];			// position of light snapped to the nearest integer
									// the filename for the vtf file is derived from the position
	unsigned char size;				// 0 - default
									// otherwise, 1<<(size-1)
};

#define OVERLAY_BSP_FACE_COUNT	64

#define OVERLAY_NUM_RENDER_ORDERS		(1<<OVERLAY_RENDER_ORDER_NUM_BITS)
#define OVERLAY_RENDER_ORDER_NUM_BITS	2
#define OVERLAY_RENDER_ORDER_MASK		0xC000	// top 2 bits set

struct doverlay_t
{
	DECLARE_BYTESWAP_DATADESC();
	int			nId;
	short		nTexInfo;

	// Accessors..
	void			SetFaceCount( unsigned short count );
	unsigned short	GetFaceCount() const;

	void			SetRenderOrder( unsigned short order );
	unsigned short	GetRenderOrder() const;

private:
	unsigned short	m_nFaceCountAndRenderOrder;

public:
	int			aFaces[OVERLAY_BSP_FACE_COUNT];
	float		flU[2];
	float		flV[2];
	Vector		vecUVPoints[4];
	Vector		vecOrigin;
	Vector		vecBasisNormal;
};


inline void doverlay_t::SetFaceCount( unsigned short count )
{
	m_nFaceCountAndRenderOrder &= OVERLAY_RENDER_ORDER_MASK;
	m_nFaceCountAndRenderOrder |= (count & ~OVERLAY_RENDER_ORDER_MASK);
}

inline unsigned short doverlay_t::GetFaceCount() const
{
	return m_nFaceCountAndRenderOrder & ~OVERLAY_RENDER_ORDER_MASK;
}

inline void doverlay_t::SetRenderOrder( unsigned short order )
{
	m_nFaceCountAndRenderOrder &= ~OVERLAY_RENDER_ORDER_MASK;
	m_nFaceCountAndRenderOrder |= (order << (16 - OVERLAY_RENDER_ORDER_NUM_BITS));	// leave 2 bits for render order.
}

inline unsigned short doverlay_t::GetRenderOrder() const
{
	return (m_nFaceCountAndRenderOrder >> (16 - OVERLAY_RENDER_ORDER_NUM_BITS));
}


struct doverlayfade_t
{
	DECLARE_BYTESWAP_DATADESC();

	float flFadeDistMinSq;
	float flFadeDistMaxSq;
};


#define WATEROVERLAY_BSP_FACE_COUNT				256
#define WATEROVERLAY_RENDER_ORDER_NUM_BITS		2
#define WATEROVERLAY_NUM_RENDER_ORDERS			(1<<WATEROVERLAY_RENDER_ORDER_NUM_BITS)
#define WATEROVERLAY_RENDER_ORDER_MASK			0xC000	// top 2 bits set
struct dwateroverlay_t
{
	DECLARE_BYTESWAP_DATADESC();
	int				nId;
	short			nTexInfo;

	// Accessors..
	void			SetFaceCount( unsigned short count );
	unsigned short	GetFaceCount() const;
	void			SetRenderOrder( unsigned short order );
	unsigned short	GetRenderOrder() const;

private:

	unsigned short	m_nFaceCountAndRenderOrder;

public:

	int				aFaces[WATEROVERLAY_BSP_FACE_COUNT];
	float			flU[2];
	float			flV[2];
	Vector			vecUVPoints[4];
	Vector			vecOrigin;
	Vector			vecBasisNormal;
};

inline void dwateroverlay_t::SetFaceCount( unsigned short count )
{
	m_nFaceCountAndRenderOrder &= WATEROVERLAY_RENDER_ORDER_MASK;
	m_nFaceCountAndRenderOrder |= (count & ~WATEROVERLAY_RENDER_ORDER_MASK);
}

inline unsigned short dwateroverlay_t::GetFaceCount() const
{
	return m_nFaceCountAndRenderOrder & ~WATEROVERLAY_RENDER_ORDER_MASK;
}

inline void dwateroverlay_t::SetRenderOrder( unsigned short order )
{
	m_nFaceCountAndRenderOrder &= ~WATEROVERLAY_RENDER_ORDER_MASK;
	m_nFaceCountAndRenderOrder |= ( order << ( 16 - WATEROVERLAY_RENDER_ORDER_NUM_BITS ) );	// leave 2 bits for render order.
}

inline unsigned short dwateroverlay_t::GetRenderOrder() const
{
	return ( m_nFaceCountAndRenderOrder >> ( 16 - WATEROVERLAY_RENDER_ORDER_NUM_BITS ) );
}

#ifndef _DEF_BYTE_
#define _DEF_BYTE_
typedef unsigned char	byte;
typedef unsigned short	word;
#endif


#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


//===============


struct epair_t
{
	epair_t	*next;
	char	*key;
	char	*value;
};

// finalized page of surface's lightmaps
#define MAX_LIGHTMAPPAGE_WIDTH	256
#define MAX_LIGHTMAPPAGE_HEIGHT	128
typedef struct nameForDatadesc_dlightmappage_t // unnamed structs collide in the datadesc macros
{
	DECLARE_BYTESWAP_DATADESC();
	byte	data[MAX_LIGHTMAPPAGE_WIDTH*MAX_LIGHTMAPPAGE_HEIGHT];
	byte	palette[256*4];
} dlightmappage_t;

typedef struct nameForDatadesc_dlightmappageinfo_t // unnamed structs collide in the datadesc macros
{
	DECLARE_BYTESWAP_DATADESC();
	byte			page;			// lightmap page [0..?]
	byte			offset[2];		// offset into page (s,t)
	byte			pad;			// unused
	ColorRGBExp32	avgColor;		// average used for runtime lighting calcs
} dlightmappageinfo_t;

#endif // BSPFILE_H
