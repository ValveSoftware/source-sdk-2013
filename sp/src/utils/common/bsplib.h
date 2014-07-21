//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BSPLIB_H
#define BSPLIB_H

#ifdef _WIN32
#pragma once
#endif


#include "bspfile.h"
#include "utlvector.h"
#include "utlstring.h"
#include "utllinkedlist.h"
#include "byteswap.h"
#ifdef ENGINE_DLL
#include "zone.h"
#endif

#ifdef ENGINE_DLL
typedef CUtlVector<unsigned char, CHunkMemory<unsigned char> > CDispLightmapSamplePositions;
#else
typedef CUtlVector<unsigned char> CDispLightmapSamplePositions;
#endif

class ISpatialQuery;
struct Ray_t;
class Vector2D;
struct portal_t;
class CUtlBuffer;
class IZip;

// this is only true in vrad
extern bool g_bHDR;

// default width/height of luxels in world units.
#define DEFAULT_LUXEL_SIZE ( 16.0f )

#define	SINGLE_BRUSH_MAP	(MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER*MAX_BRUSH_LIGHTMAP_DIM_INCLUDING_BORDER)
#define	SINGLEMAP			(MAX_LIGHTMAP_DIM_INCLUDING_BORDER*MAX_LIGHTMAP_DIM_INCLUDING_BORDER)

struct entity_t
{
	Vector		origin;
	int			firstbrush;
	int			numbrushes;
	epair_t		*epairs;

	// only valid for func_areaportals
	int			areaportalnum;
	int			portalareas[2];
	portal_t	*m_pPortalsLeadingIntoAreas[2];	// portals leading into portalareas
};

extern	int				num_entities;
extern	entity_t		entities[MAX_MAP_ENTITIES];

extern	int			    nummodels;
extern	dmodel_t	    dmodels[MAX_MAP_MODELS];

extern	int			    visdatasize;
extern	byte		    dvisdata[MAX_MAP_VISIBILITY];
extern	dvis_t		    *dvis;

extern	CUtlVector<byte> dlightdataHDR;
extern	CUtlVector<byte> dlightdataLDR;
extern	CUtlVector<byte> *pdlightdata;
extern	CUtlVector<char> dentdata;

extern	int			    numleafs;
#if !defined( _X360 )
extern	dleaf_t			dleafs[MAX_MAP_LEAFS];
#else
extern	dleaf_t			*dleafs;
#endif
extern	CUtlVector<dleafambientlighting_t> *g_pLeafAmbientLighting;
extern	CUtlVector<dleafambientindex_t> *g_pLeafAmbientIndex;
extern	unsigned short  g_LeafMinDistToWater[MAX_MAP_LEAFS];

extern	int			    numplanes;
extern	dplane_t	    dplanes[MAX_MAP_PLANES];

extern	int			    numvertexes;
extern	dvertex_t	    dvertexes[MAX_MAP_VERTS];

extern	int				g_numvertnormalindices;	// dfaces reference these. These index g_vertnormals.
extern	unsigned short	g_vertnormalindices[MAX_MAP_VERTNORMALS];

extern	int				g_numvertnormals;	
extern	Vector			g_vertnormals[MAX_MAP_VERTNORMALS];

extern	int			    numnodes;
extern	dnode_t		    dnodes[MAX_MAP_NODES];

extern  CUtlVector<texinfo_t> texinfo;

extern	int			    numtexdata;
extern	dtexdata_t	    dtexdata[MAX_MAP_TEXDATA];

// displacement map .bsp file info
extern  CUtlVector<ddispinfo_t>		g_dispinfo;
extern  CUtlVector<CDispVert>		g_DispVerts;
extern  CUtlVector<CDispTri>		g_DispTris;
extern  CDispLightmapSamplePositions g_DispLightmapSamplePositions; // LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS

extern  int             numorigfaces;
extern  dface_t         dorigfaces[MAX_MAP_FACES];

extern	int				g_numprimitives;
extern	dprimitive_t	g_primitives[MAX_MAP_PRIMITIVES];

extern	int				g_numprimverts;
extern	dprimvert_t		g_primverts[MAX_MAP_PRIMVERTS];

extern	int				g_numprimindices;
extern	unsigned short	g_primindices[MAX_MAP_PRIMINDICES];

extern	int			    numfaces;
extern	dface_t		    dfaces[MAX_MAP_FACES];

extern	int				numfaceids;
extern	CUtlVector<dfaceid_t>	dfaceids;

extern	int			    numfaces_hdr;
extern	dface_t		    dfaces_hdr[MAX_MAP_FACES];

extern	int			    numedges;
extern	dedge_t		    dedges[MAX_MAP_EDGES];

extern	int			    numleaffaces;
extern	unsigned short	dleaffaces[MAX_MAP_LEAFFACES];

extern	int			    numleafbrushes;
extern	unsigned short	dleafbrushes[MAX_MAP_LEAFBRUSHES];

extern	int			    numsurfedges;
extern	int			    dsurfedges[MAX_MAP_SURFEDGES];

extern	int			    numareas;
extern	darea_t		    dareas[MAX_MAP_AREAS];

extern	int			    numareaportals;
extern	dareaportal_t	dareaportals[MAX_MAP_AREAPORTALS];

extern	int			    numbrushes;
extern	dbrush_t	    dbrushes[MAX_MAP_BRUSHES];

extern	int			    numbrushsides;
extern	dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

extern  int			    *pNumworldlights;
extern  dworldlight_t   *dworldlights;

extern Vector			g_ClipPortalVerts[MAX_MAP_PORTALVERTS];
extern int				g_nClipPortalVerts;

extern dcubemapsample_t	g_CubemapSamples[MAX_MAP_CUBEMAPSAMPLES];
extern int				g_nCubemapSamples;

extern int				g_nOverlayCount;
extern doverlay_t		g_Overlays[MAX_MAP_OVERLAYS];
extern doverlayfade_t	g_OverlayFades[MAX_MAP_OVERLAYS];	// Parallel array of fade info in a separate lump to avoid breaking backwards compat

extern int				g_nWaterOverlayCount;
extern dwateroverlay_t	g_WaterOverlays[MAX_MAP_WATEROVERLAYS];

extern CUtlVector<char>	g_TexDataStringData;
extern CUtlVector<int>	g_TexDataStringTable;

extern	int					numleafwaterdata;
extern	dleafwaterdata_t	dleafwaterdata[MAX_MAP_LEAFWATERDATA]; 

extern CUtlVector<CFaceMacroTextureInfo>	g_FaceMacroTextureInfos;

extern CUtlVector<doccluderdata_t>		g_OccluderData;
extern CUtlVector<doccluderpolydata_t>	g_OccluderPolyData;
extern CUtlVector<int>					g_OccluderVertexIndices;

// level flags - see LVLFLAGS_xxx in bspfile.h
extern uint32 g_LevelFlags;	

// physics collision data
extern	byte		*g_pPhysCollide;
extern	int			g_PhysCollideSize;
extern byte			*g_pPhysDisp;
extern int			g_PhysDispSize;

// Embedded pack/pak file
IZip				*GetPakFile( void );
IZip				*GetSwapPakFile( void );
void				ClearPakFile( IZip *pak );
void				AddFileToPak( IZip *pak, const char *pRelativeName, const char *fullpath );
void				AddBufferToPak( IZip *pak, const char *pRelativeName, void *data, int length, bool bTextMode );
bool				FileExistsInPak( IZip *pak, const char *pRelativeName );
bool				ReadFileFromPak( IZip *pak, const char *pRelativeName, bool bTextMode, CUtlBuffer &buf );
void				RemoveFileFromPak( IZip *pak, const char *pRelativeName );
int					GetNextFilename( IZip *pak, int id, char *pBuffer, int bufferSize, int &fileSize );
void				ForceAlignment( IZip *pak, bool bAlign, bool bCompatibleFormat, unsigned int alignmentSize );

typedef bool (*CompressFunc_t)( CUtlBuffer &inputBuffer, CUtlBuffer &outputBuffer );
typedef bool (*VTFConvertFunc_t)( const char *pDebugName, CUtlBuffer &sourceBuf, CUtlBuffer &targetBuf, CompressFunc_t pCompressFunc );
typedef bool (*VHVFixupFunc_t)( const char *pVhvFilename, const char *pModelName, CUtlBuffer &sourceBuf, CUtlBuffer &targetBuf );

//-----------------------------------------------------------------------------
// Game lump memory storage
//-----------------------------------------------------------------------------
// NOTE: This is not optimal at all; since I expect client lumps to
// not be accessed all that often.

struct GameLump_t
{
	GameLumpId_t	m_Id;
	unsigned short	m_Flags;
	unsigned short	m_Version;
	CUtlMemory< unsigned char >	m_Memory;
};

//-----------------------------------------------------------------------------
// Handle to a game lump
//-----------------------------------------------------------------------------
typedef unsigned short GameLumpHandle_t;

class CGameLump
{
public:
	//-----------------------------------------------------------------------------
	// Convert four-CC code to a handle	+ back
	//-----------------------------------------------------------------------------
	GameLumpHandle_t	GetGameLumpHandle( GameLumpId_t id );
	GameLumpId_t		GetGameLumpId( GameLumpHandle_t handle );
	int					GetGameLumpFlags( GameLumpHandle_t handle );
	int					GetGameLumpVersion( GameLumpHandle_t handle );
	void				ComputeGameLumpSizeAndCount( int& size, int& clumpCount );
	void				ParseGameLump( dheader_t* pHeader );
	void				SwapGameLump( GameLumpId_t id, int version, byte *dest, byte *src, int size );


	//-----------------------------------------------------------------------------
	// Game lump accessor methods 
	//-----------------------------------------------------------------------------
	void*	GetGameLump( GameLumpHandle_t handle );
	int		GameLumpSize( GameLumpHandle_t handle );


	//-----------------------------------------------------------------------------
	// Game lump iteration methods 
	//-----------------------------------------------------------------------------
	GameLumpHandle_t	FirstGameLump();
	GameLumpHandle_t	NextGameLump( GameLumpHandle_t handle );
	GameLumpHandle_t	InvalidGameLump();


	//-----------------------------------------------------------------------------
	// Game lump creation/destruction method
	//-----------------------------------------------------------------------------
	GameLumpHandle_t	CreateGameLump( GameLumpId_t id, int size, int flags, int version );
	void				DestroyGameLump( GameLumpHandle_t handle );
	void				DestroyAllGameLumps();

private:
	CUtlLinkedList< GameLump_t, GameLumpHandle_t >	m_GameLumps;
};

extern CGameLump	g_GameLumps;
extern CByteswap	g_Swap;

//-----------------------------------------------------------------------------
// Helper for the bspzip tool
//-----------------------------------------------------------------------------
void ExtractZipFileFromBSP( char *pBSPFileName, char *pZipFileName );


//-----------------------------------------------------------------------------
// String table methods
//-----------------------------------------------------------------------------
const char *		TexDataStringTable_GetString( int stringID );
int					TexDataStringTable_AddOrFindString( const char *pString );

void	DecompressVis (byte *in, byte *decompressed);
int		CompressVis (byte *vis, byte *dest);

void	OpenBSPFile( const char *filename );
void	CloseBSPFile(void);
void	LoadBSPFile( const char *filename );
void	LoadBSPFile_FileSystemOnly( const char *filename );
void	LoadBSPFileTexinfo( const char *filename );
void	WriteBSPFile( const char *filename, char *pUnused = NULL );
void	PrintBSPFileSizes(void);
void	PrintBSPPackDirectory(void);
void	ReleasePakFileLumps(void);
bool	SwapBSPFile( const char *filename, const char *swapFilename, bool bSwapOnLoad, VTFConvertFunc_t pVTFConvertFunc, VHVFixupFunc_t pVHVFixupFunc, CompressFunc_t pCompressFunc );
bool	GetPakFileLump( const char *pBSPFilename, void **pPakData, int *pPakSize );
bool	SetPakFileLump( const char *pBSPFilename, const char *pNewFilename, void *pPakData, int pakSize );
void	WriteLumpToFile( char *filename, int lump );
void	WriteLumpToFile( char *filename, int lump, int nLumpVersion, void *pBuffer, size_t nBufLen );
bool	GetBSPDependants( const char *pBSPFilename, CUtlVector< CUtlString > *pList );
void	UnloadBSPFile();

void	ParseEntities (void);
void	UnparseEntities (void);
void	PrintEntity (entity_t *ent);

void 	SetKeyValue (entity_t *ent, const char *key, const char *value);
char 	*ValueForKey (entity_t *ent, char *key);
// will return "" if not present
int		IntForKey (entity_t *ent, char *key);
int		IntForKeyWithDefault(entity_t *ent, char *key, int nDefault );
vec_t	FloatForKey (entity_t *ent, char *key);
vec_t	FloatForKeyWithDefault (entity_t *ent, char *key, float default_value);
void 	GetVectorForKey (entity_t *ent, char *key, Vector& vec);
void 	GetVector2DForKey (entity_t *ent, char *key, Vector2D& vec);
void 	GetAnglesForKey (entity_t *ent, char *key, QAngle& vec);
epair_t *ParseEpair (void);
void StripTrailing (char *e);

// Build a list of the face's vertices (index into dvertexes).
// points must be able to hold pFace->numedges indices.
void BuildFaceCalcWindingData( dface_t *pFace, int *points );

// Convert a tristrip to a trilist.
// Removes degenerates.
// Fills in pTriListIndices and pnTriListIndices.
// You must free pTriListIndices with delete[].
void TriStripToTriList( 
	unsigned short const *pTriStripIndices,
	int nTriStripIndices,
	unsigned short **pTriListIndices,
	int *pnTriListIndices );

// Calculates the lightmap coordinates at a given set of positions given the 
// lightmap basis information.
void CalcTextureCoordsAtPoints(
	float const texelsPerWorldUnits[2][4],
	int const subtractOffset[2],
	Vector const *pPoints,
	int const nPoints,
	Vector2D *pCoords );

// Figure out lightmap extents on all (lit) faces.
void UpdateAllFaceLightmapExtents();


//-----------------------------------------------------------------------------
// Gets at an interface for the tree for enumeration of leaves in volumes.
//-----------------------------------------------------------------------------
ISpatialQuery* ToolBSPTree();

class IBSPNodeEnumerator
{
public:
	// call back with a node and a context
	virtual bool EnumerateNode( int node, Ray_t const& ray, float f, int context ) = 0;

	// call back with a leaf and a context
	virtual bool EnumerateLeaf( int leaf, Ray_t const& ray, float start, float end, int context ) = 0;
};

//-----------------------------------------------------------------------------
// Enumerates nodes + leafs in front to back order...
//-----------------------------------------------------------------------------
bool EnumerateNodesAlongRay( Ray_t const& ray, IBSPNodeEnumerator* pEnum, int context );


//-----------------------------------------------------------------------------
// Helps us find all leaves associated with a particular cluster
//-----------------------------------------------------------------------------
struct clusterlist_t
{
	int				leafCount;
	CUtlVector<int> leafs;
};

extern CUtlVector<clusterlist_t> g_ClusterLeaves;

// Call this to build the mapping from cluster to leaves
void BuildClusterTable( );

void GetPlatformMapPath( const char *pMapPath, char *pPlatformMapPath, int dxlevel, int maxLength );

void SetHDRMode( bool bHDR );

// ----------------------------------------------------------------------------- //
// Helper accessors for the various structures.
// ----------------------------------------------------------------------------- //

inline ColorRGBExp32* dface_AvgLightColor( dface_t *pFace, int nLightStyleIndex ) 
{ 
	return (ColorRGBExp32*)&(*pdlightdata)[pFace->lightofs - (nLightStyleIndex+1) * 4];
}

inline const char* TexInfo_TexName( int iTexInfo )
{
	return TexDataStringTable_GetString( dtexdata[texinfo[iTexInfo].texdata].nameStringTableID );
}


#endif // BSPLIB_H
