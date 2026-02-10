//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#include "cmdlib.h"
#include "mathlib/mathlib.h"
#include "bsplib.h"
#include "zip_utils.h"
#include "scriplib.h"
#include "utllinkedlist.h"
#include "bsptreedata.h"
#include "cmodel.h"
#include "gamebspfile.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/hardwareverts.h"
#include "utlbuffer.h"
#include "utlrbtree.h"
#include "utlsymbol.h"
#include "utlstring.h"
#include "checksum_crc.h"
#include "physdll.h"
#include "tier0/dbg.h"
#include "lumpfiles.h"
#include "vtf/vtf.h"
#include "lzma/lzma.h"
#include "tier1/lzmaDecoder.h"

#include "tier0/memdbgon.h"

//=============================================================================

// Boundary each lump should be aligned to
#define LUMP_ALIGNMENT	4

// Data descriptions for byte swapping - only needed
// for structures that are written to file for use by the game.
BEGIN_BYTESWAP_DATADESC( dheader_t )
	DEFINE_FIELD( ident, FIELD_INTEGER ),
	DEFINE_FIELD( version, FIELD_INTEGER ),
	DEFINE_EMBEDDED_ARRAY( lumps, HEADER_LUMPS ),
	DEFINE_FIELD( mapRevision, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( lump_t )
	DEFINE_FIELD( fileofs, FIELD_INTEGER ),
	DEFINE_FIELD( filelen, FIELD_INTEGER ),
	DEFINE_FIELD( version, FIELD_INTEGER ),
	DEFINE_FIELD( uncompressedSize, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dflagslump_t )
	DEFINE_FIELD( m_LevelFlags, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dplane_t )
	DEFINE_FIELD( normal, FIELD_VECTOR ),
	DEFINE_FIELD( dist, FIELD_FLOAT ),
	DEFINE_FIELD( type, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dleaf_version_0_t )
	DEFINE_FIELD( contents, FIELD_INTEGER ),
	DEFINE_FIELD( cluster, FIELD_SHORT ),
	DEFINE_BITFIELD( bf, FIELD_SHORT, 16 ),
	DEFINE_ARRAY( mins, FIELD_SHORT, 3 ),
	DEFINE_ARRAY( maxs, FIELD_SHORT, 3 ),
	DEFINE_FIELD( firstleafface, FIELD_SHORT ),
	DEFINE_FIELD( numleaffaces, FIELD_SHORT ),
	DEFINE_FIELD( firstleafbrush, FIELD_SHORT ),
	DEFINE_FIELD( numleafbrushes, FIELD_SHORT ),
	DEFINE_FIELD( leafWaterDataID, FIELD_SHORT ),
	DEFINE_EMBEDDED( m_AmbientLighting ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dleaf_t )
	DEFINE_FIELD( contents, FIELD_INTEGER ),
	DEFINE_FIELD( cluster, FIELD_SHORT ),
	DEFINE_BITFIELD( bf, FIELD_SHORT, 16 ),
	DEFINE_ARRAY( mins, FIELD_SHORT, 3 ),
	DEFINE_ARRAY( maxs, FIELD_SHORT, 3 ),
	DEFINE_FIELD( firstleafface, FIELD_SHORT ),
	DEFINE_FIELD( numleaffaces, FIELD_SHORT ),
	DEFINE_FIELD( firstleafbrush, FIELD_SHORT ),
	DEFINE_FIELD( numleafbrushes, FIELD_SHORT ),
	DEFINE_FIELD( leafWaterDataID, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CompressedLightCube )	// array of 6 ColorRGBExp32 (3 bytes and 1 char)
	DEFINE_ARRAY( m_Color, FIELD_CHARACTER, 6 * sizeof(ColorRGBExp32) ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dleafambientindex_t )
	DEFINE_FIELD( ambientSampleCount, FIELD_SHORT ),
	DEFINE_FIELD( firstAmbientSample, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dleafambientlighting_t )	// array of 6 ColorRGBExp32 (3 bytes and 1 char)
	DEFINE_EMBEDDED( cube ),
	DEFINE_FIELD( x, FIELD_CHARACTER ),
	DEFINE_FIELD( y, FIELD_CHARACTER ),
	DEFINE_FIELD( z, FIELD_CHARACTER ),
	DEFINE_FIELD( pad, FIELD_CHARACTER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dvertex_t )
	DEFINE_FIELD( point, FIELD_VECTOR ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dnode_t )
	DEFINE_FIELD( planenum, FIELD_INTEGER ),
	DEFINE_ARRAY( children, FIELD_INTEGER, 2 ),
	DEFINE_ARRAY( mins, FIELD_SHORT, 3 ),
	DEFINE_ARRAY( maxs, FIELD_SHORT, 3 ),
	DEFINE_FIELD( firstface, FIELD_SHORT ),
	DEFINE_FIELD( numfaces, FIELD_SHORT ),
	DEFINE_FIELD( area, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( texinfo_t )
	DEFINE_ARRAY( textureVecsTexelsPerWorldUnits, FIELD_FLOAT, 2 * 4 ),
	DEFINE_ARRAY( lightmapVecsLuxelsPerWorldUnits, FIELD_FLOAT, 2 * 4 ),
	DEFINE_FIELD( flags, FIELD_INTEGER ),
	DEFINE_FIELD( texdata, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dtexdata_t )
	DEFINE_FIELD( reflectivity, FIELD_VECTOR ),
	DEFINE_FIELD( nameStringTableID, FIELD_INTEGER ),
	DEFINE_FIELD( width, FIELD_INTEGER ),
	DEFINE_FIELD( height, FIELD_INTEGER ),
	DEFINE_FIELD( view_width, FIELD_INTEGER ),
	DEFINE_FIELD( view_height, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( ddispinfo_t )
	DEFINE_FIELD( startPosition, FIELD_VECTOR ),
	DEFINE_FIELD( m_iDispVertStart, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDispTriStart, FIELD_INTEGER ),
	DEFINE_FIELD( power, FIELD_INTEGER ),
	DEFINE_FIELD( minTess, FIELD_INTEGER ),
	DEFINE_FIELD( smoothingAngle, FIELD_FLOAT ),
	DEFINE_FIELD( contents, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMapFace, FIELD_SHORT ),
	DEFINE_FIELD( m_iLightmapAlphaStart, FIELD_INTEGER ),
	DEFINE_FIELD( m_iLightmapSamplePositionStart, FIELD_INTEGER ),
	DEFINE_EMBEDDED_ARRAY( m_EdgeNeighbors, 4 ),
	DEFINE_EMBEDDED_ARRAY( m_CornerNeighbors, 4 ),
	DEFINE_ARRAY( m_AllowedVerts, FIELD_INTEGER, ddispinfo_t::ALLOWEDVERTS_SIZE ),	// unsigned long
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CDispNeighbor )
	DEFINE_EMBEDDED_ARRAY( m_SubNeighbors, 2 ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CDispCornerNeighbors )
	DEFINE_ARRAY( m_Neighbors, FIELD_SHORT, MAX_DISP_CORNER_NEIGHBORS ),
	DEFINE_FIELD( m_nNeighbors, FIELD_CHARACTER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CDispSubNeighbor )
	DEFINE_FIELD( m_iNeighbor, FIELD_SHORT ),
	DEFINE_FIELD( m_NeighborOrientation, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Span, FIELD_CHARACTER ),
	DEFINE_FIELD( m_NeighborSpan, FIELD_CHARACTER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CDispVert )
	DEFINE_FIELD( m_vVector, FIELD_VECTOR ),
	DEFINE_FIELD( m_flDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_flAlpha, FIELD_FLOAT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CDispTri )
	DEFINE_FIELD( m_uiTags, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( CFaceMacroTextureInfo )
	DEFINE_FIELD( m_MacroTextureNameID, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dprimitive_t )
	DEFINE_FIELD( type, FIELD_CHARACTER ),
	DEFINE_FIELD( firstIndex, FIELD_SHORT ),
	DEFINE_FIELD( indexCount, FIELD_SHORT ),
	DEFINE_FIELD( firstVert, FIELD_SHORT ),
	DEFINE_FIELD( vertCount, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dprimvert_t )
	DEFINE_FIELD( pos, FIELD_VECTOR ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dface_t )
	DEFINE_FIELD( planenum, FIELD_SHORT ),
	DEFINE_FIELD( side, FIELD_CHARACTER ),
	DEFINE_FIELD( onNode, FIELD_CHARACTER ),
	DEFINE_FIELD( firstedge, FIELD_INTEGER ),
	DEFINE_FIELD( numedges, FIELD_SHORT ),
	DEFINE_FIELD( texinfo, FIELD_SHORT ),
	DEFINE_FIELD( dispinfo, FIELD_SHORT ),
	DEFINE_FIELD( surfaceFogVolumeID, FIELD_SHORT ),
	DEFINE_ARRAY( styles, FIELD_CHARACTER, MAXLIGHTMAPS ),
	DEFINE_FIELD( lightofs, FIELD_INTEGER ),
	DEFINE_FIELD( area, FIELD_FLOAT ),
	DEFINE_ARRAY( m_LightmapTextureMinsInLuxels, FIELD_INTEGER, 2 ),
	DEFINE_ARRAY( m_LightmapTextureSizeInLuxels, FIELD_INTEGER, 2 ),
	DEFINE_FIELD( origFace, FIELD_INTEGER ),
	DEFINE_FIELD( m_NumPrims, FIELD_SHORT ),
	DEFINE_FIELD( firstPrimID, FIELD_SHORT ),
	DEFINE_FIELD( smoothingGroups, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dfaceid_t )
	DEFINE_FIELD( hammerfaceid, FIELD_SHORT ),
END_BYTESWAP_DATADESC()


BEGIN_BYTESWAP_DATADESC( dbrush_t )
	DEFINE_FIELD( firstside, FIELD_INTEGER ),
	DEFINE_FIELD( numsides, FIELD_INTEGER ),
	DEFINE_FIELD( contents, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dbrushside_t )
	DEFINE_FIELD( planenum, FIELD_SHORT ),
	DEFINE_FIELD( texinfo, FIELD_SHORT ),
	DEFINE_FIELD( dispinfo, FIELD_SHORT ),
	DEFINE_FIELD( bevel, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dedge_t )
	DEFINE_ARRAY( v, FIELD_SHORT, 2 ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dmodel_t )
	DEFINE_FIELD( mins, FIELD_VECTOR ),
	DEFINE_FIELD( maxs, FIELD_VECTOR ),
	DEFINE_FIELD( origin, FIELD_VECTOR ),
	DEFINE_FIELD( headnode, FIELD_INTEGER ),
	DEFINE_FIELD( firstface, FIELD_INTEGER ),
	DEFINE_FIELD( numfaces, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dphysmodel_t )
	DEFINE_FIELD( modelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( dataSize, FIELD_INTEGER ),
	DEFINE_FIELD( keydataSize, FIELD_INTEGER ),
	DEFINE_FIELD( solidCount, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dphysdisp_t )
	DEFINE_FIELD( numDisplacements, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( darea_t )
	DEFINE_FIELD( numareaportals, FIELD_INTEGER ),
	DEFINE_FIELD( firstareaportal, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dareaportal_t )
	DEFINE_FIELD( m_PortalKey, FIELD_SHORT ),
	DEFINE_FIELD( otherarea, FIELD_SHORT ),
	DEFINE_FIELD( m_FirstClipPortalVert, FIELD_SHORT ),
	DEFINE_FIELD( m_nClipPortalVerts, FIELD_SHORT ),
	DEFINE_FIELD( planenum, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dworldlight_t )
	DEFINE_FIELD( origin, FIELD_VECTOR ),
	DEFINE_FIELD( intensity, FIELD_VECTOR ),
	DEFINE_FIELD( normal, FIELD_VECTOR ),
	DEFINE_FIELD( cluster, FIELD_INTEGER ),
	DEFINE_FIELD( type, FIELD_INTEGER ),	// enumeration
	DEFINE_FIELD( style, FIELD_INTEGER ),
	DEFINE_FIELD( stopdot, FIELD_FLOAT ),
	DEFINE_FIELD( stopdot2, FIELD_FLOAT ),
	DEFINE_FIELD( exponent, FIELD_FLOAT ),
	DEFINE_FIELD( radius, FIELD_FLOAT ),
	DEFINE_FIELD( constant_attn, FIELD_FLOAT ),
	DEFINE_FIELD( linear_attn, FIELD_FLOAT ),
	DEFINE_FIELD( quadratic_attn, FIELD_FLOAT ),
	DEFINE_FIELD( flags, FIELD_INTEGER ),
	DEFINE_FIELD( texinfo, FIELD_INTEGER ),
	DEFINE_FIELD( owner, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dleafwaterdata_t )
	DEFINE_FIELD( surfaceZ, FIELD_FLOAT ),
	DEFINE_FIELD( minZ, FIELD_FLOAT ),
	DEFINE_FIELD( surfaceTexInfoID, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( doccluderdata_t )
	DEFINE_FIELD( flags, FIELD_INTEGER ),
	DEFINE_FIELD( firstpoly, FIELD_INTEGER ),
	DEFINE_FIELD( polycount, FIELD_INTEGER ),
	DEFINE_FIELD( mins, FIELD_VECTOR ),
	DEFINE_FIELD( maxs, FIELD_VECTOR ),
	DEFINE_FIELD( area, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( doccluderpolydata_t )
	DEFINE_FIELD( firstvertexindex, FIELD_INTEGER ),
	DEFINE_FIELD( vertexcount, FIELD_INTEGER ),
	DEFINE_FIELD( planenum, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dcubemapsample_t )
	DEFINE_ARRAY( origin, FIELD_INTEGER, 3 ),
	DEFINE_FIELD( size, FIELD_CHARACTER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( doverlay_t )
	DEFINE_FIELD( nId, FIELD_INTEGER ),
	DEFINE_FIELD( nTexInfo, FIELD_SHORT ),
	DEFINE_FIELD( m_nFaceCountAndRenderOrder, FIELD_SHORT ),
	DEFINE_ARRAY( aFaces, FIELD_INTEGER, OVERLAY_BSP_FACE_COUNT ),
	DEFINE_ARRAY( flU, FIELD_FLOAT, 2 ),
	DEFINE_ARRAY( flV, FIELD_FLOAT, 2 ),
	DEFINE_ARRAY( vecUVPoints, FIELD_VECTOR, 4 ),
	DEFINE_FIELD( vecOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( vecBasisNormal, FIELD_VECTOR ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dwateroverlay_t )
	DEFINE_FIELD( nId, FIELD_INTEGER ),
	DEFINE_FIELD( nTexInfo, FIELD_SHORT ),
	DEFINE_FIELD( m_nFaceCountAndRenderOrder, FIELD_SHORT ),
	DEFINE_ARRAY( aFaces, FIELD_INTEGER, WATEROVERLAY_BSP_FACE_COUNT ),
	DEFINE_ARRAY( flU, FIELD_FLOAT, 2 ),
	DEFINE_ARRAY( flV, FIELD_FLOAT, 2 ),
	DEFINE_ARRAY( vecUVPoints, FIELD_VECTOR, 4 ),
	DEFINE_FIELD( vecOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( vecBasisNormal, FIELD_VECTOR ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( doverlayfade_t )
	DEFINE_FIELD( flFadeDistMinSq, FIELD_FLOAT ),
	DEFINE_FIELD( flFadeDistMaxSq, FIELD_FLOAT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dgamelumpheader_t )
	DEFINE_FIELD( lumpCount, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( dgamelump_t )
	DEFINE_FIELD( id, FIELD_INTEGER ),	// GameLumpId_t
	DEFINE_FIELD( flags, FIELD_SHORT ),
	DEFINE_FIELD( version, FIELD_SHORT ),
	DEFINE_FIELD( fileofs, FIELD_INTEGER ),
	DEFINE_FIELD( filelen, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

// From gamebspfile.h
BEGIN_BYTESWAP_DATADESC( StaticPropDictLump_t )
	DEFINE_ARRAY( m_Name, FIELD_CHARACTER, STATIC_PROP_NAME_LENGTH ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( StaticPropLump_t )
	DEFINE_FIELD( m_Origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_Angles, FIELD_VECTOR ),	// QAngle
	DEFINE_FIELD( m_PropType, FIELD_SHORT ),
	DEFINE_FIELD( m_FirstLeaf, FIELD_SHORT ),
	DEFINE_FIELD( m_LeafCount, FIELD_SHORT ),
	DEFINE_FIELD( m_Solid, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Skin, FIELD_INTEGER ),
	DEFINE_FIELD( m_FadeMinDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_FadeMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_LightingOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_flForcedFadeScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_nMinDXLevel, FIELD_SHORT ),
	DEFINE_FIELD( m_nMaxDXLevel, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( StaticPropLumpV4_t )
	DEFINE_FIELD( m_Origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_Angles, FIELD_VECTOR ),	// QAngle
	DEFINE_FIELD( m_PropType, FIELD_SHORT ),
	DEFINE_FIELD( m_FirstLeaf, FIELD_SHORT ),
	DEFINE_FIELD( m_LeafCount, FIELD_SHORT ),
	DEFINE_FIELD( m_Solid, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Skin, FIELD_INTEGER ),
	DEFINE_FIELD( m_FadeMinDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_FadeMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_LightingOrigin, FIELD_VECTOR ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( StaticPropLumpV5_t )
	DEFINE_FIELD( m_Origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_Angles, FIELD_VECTOR ),	// QAngle
	DEFINE_FIELD( m_PropType, FIELD_SHORT ),
	DEFINE_FIELD( m_FirstLeaf, FIELD_SHORT ),
	DEFINE_FIELD( m_LeafCount, FIELD_SHORT ),
	DEFINE_FIELD( m_Solid, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Flags, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Skin, FIELD_INTEGER ),
	DEFINE_FIELD( m_FadeMinDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_FadeMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( m_LightingOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( m_flForcedFadeScale, FIELD_FLOAT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( StaticPropLeafLump_t )
	DEFINE_FIELD( m_Leaf, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( DetailObjectDictLump_t )
	DEFINE_ARRAY( m_Name, FIELD_CHARACTER, DETAIL_NAME_LENGTH ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( DetailObjectLump_t )
	DEFINE_FIELD( m_Origin, FIELD_VECTOR ),
	DEFINE_FIELD( m_Angles, FIELD_VECTOR ),			// QAngle
	DEFINE_FIELD( m_DetailModel, FIELD_SHORT ),
	DEFINE_FIELD( m_Leaf, FIELD_SHORT ),
	DEFINE_ARRAY( m_Lighting, FIELD_CHARACTER, 4 ),	// ColorRGBExp32
	DEFINE_FIELD( m_LightStyles, FIELD_INTEGER ),
	DEFINE_FIELD( m_LightStyleCount, FIELD_CHARACTER ),
	DEFINE_FIELD( m_SwayAmount, FIELD_CHARACTER ),
	DEFINE_FIELD( m_ShapeAngle, FIELD_CHARACTER ),
	DEFINE_FIELD( m_ShapeSize, FIELD_CHARACTER ),
	DEFINE_FIELD( m_Orientation, FIELD_CHARACTER ),
	DEFINE_ARRAY( m_Padding2, FIELD_CHARACTER, 3 ),
	DEFINE_FIELD( m_Type, FIELD_CHARACTER ),
	DEFINE_ARRAY( m_Padding3, FIELD_CHARACTER, 3 ),
	DEFINE_FIELD( m_flScale, FIELD_FLOAT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( DetailSpriteDictLump_t )
	DEFINE_FIELD( m_UL, FIELD_VECTOR2D ),
	DEFINE_FIELD( m_LR, FIELD_VECTOR2D ),
	DEFINE_FIELD( m_TexUL, FIELD_VECTOR2D ),
	DEFINE_FIELD( m_TexLR, FIELD_VECTOR2D ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( DetailPropLightstylesLump_t )
	DEFINE_ARRAY( m_Lighting, FIELD_CHARACTER, 4 ),	// ColorRGBExp32
	DEFINE_FIELD( m_Style, FIELD_CHARACTER ),
END_BYTESWAP_DATADESC()

// From vradstaticprops.h
namespace HardwareVerts
{
BEGIN_BYTESWAP_DATADESC( MeshHeader_t )
	DEFINE_FIELD( m_nLod, FIELD_INTEGER ),
	DEFINE_FIELD( m_nVertexes, FIELD_INTEGER ),
	DEFINE_FIELD( m_nOffset, FIELD_INTEGER ),
	DEFINE_ARRAY( m_nUnused, FIELD_INTEGER, 4 ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( FileHeader_t )
	DEFINE_FIELD( m_nVersion, FIELD_INTEGER ),
	DEFINE_FIELD( m_nChecksum, FIELD_INTEGER ),
	DEFINE_FIELD( m_nVertexFlags, FIELD_INTEGER ),
	DEFINE_FIELD( m_nVertexSize, FIELD_INTEGER ),
	DEFINE_FIELD( m_nVertexes, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMeshes, FIELD_INTEGER ),
	DEFINE_ARRAY( m_nUnused, FIELD_INTEGER, 4 ),
END_BYTESWAP_DATADESC()
} // end namespace

static const char *s_LumpNames[] = {
	"LUMP_ENTITIES",						// 0
	"LUMP_PLANES",							// 1
	"LUMP_TEXDATA",							// 2
	"LUMP_VERTEXES",						// 3
	"LUMP_VISIBILITY",						// 4
	"LUMP_NODES",							// 5
	"LUMP_TEXINFO",							// 6
	"LUMP_FACES",							// 7
	"LUMP_LIGHTING",						// 8
	"LUMP_OCCLUSION",						// 9
	"LUMP_LEAFS",							// 10
	"LUMP_FACEIDS",							// 11
	"LUMP_EDGES",							// 12
	"LUMP_SURFEDGES",						// 13
	"LUMP_MODELS",							// 14
	"LUMP_WORLDLIGHTS",						// 15
	"LUMP_LEAFFACES",						// 16
	"LUMP_LEAFBRUSHES",						// 17
	"LUMP_BRUSHES",							// 18
	"LUMP_BRUSHSIDES",						// 19
	"LUMP_AREAS",							// 20
	"LUMP_AREAPORTALS",						// 21
	"LUMP_UNUSED0",							// 22
	"LUMP_UNUSED1",							// 23
	"LUMP_UNUSED2",							// 24
	"LUMP_UNUSED3",							// 25
	"LUMP_DISPINFO",						// 26
	"LUMP_ORIGINALFACES",					// 27
	"LUMP_PHYSDISP",						// 28
	"LUMP_PHYSCOLLIDE",						// 29
	"LUMP_VERTNORMALS",						// 30
	"LUMP_VERTNORMALINDICES",				// 31
	"LUMP_DISP_LIGHTMAP_ALPHAS",			// 32
	"LUMP_DISP_VERTS",						// 33
	"LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS",	// 34
	"LUMP_GAME_LUMP",						// 35
	"LUMP_LEAFWATERDATA",					// 36
	"LUMP_PRIMITIVES",						// 37
	"LUMP_PRIMVERTS",						// 38
	"LUMP_PRIMINDICES",						// 39
	"LUMP_PAKFILE",							// 40
	"LUMP_CLIPPORTALVERTS",					// 41
	"LUMP_CUBEMAPS",						// 42
	"LUMP_TEXDATA_STRING_DATA",				// 43
	"LUMP_TEXDATA_STRING_TABLE",			// 44
	"LUMP_OVERLAYS",						// 45
	"LUMP_LEAFMINDISTTOWATER",				// 46
	"LUMP_FACE_MACRO_TEXTURE_INFO",			// 47
	"LUMP_DISP_TRIS",						// 48
	"LUMP_PHYSCOLLIDESURFACE",				// 49
	"LUMP_WATEROVERLAYS",					// 50
	"LUMP_LEAF_AMBIENT_INDEX_HDR",			// 51
	"LUMP_LEAF_AMBIENT_INDEX",				// 52
	"LUMP_LIGHTING_HDR",					// 53
	"LUMP_WORLDLIGHTS_HDR",					// 54
	"LUMP_LEAF_AMBIENT_LIGHTING_HDR",		// 55
	"LUMP_LEAF_AMBIENT_LIGHTING",			// 56
	"LUMP_XZIPPAKFILE",						// 57
	"LUMP_FACES_HDR",						// 58
	"LUMP_MAP_FLAGS",						// 59
	"LUMP_OVERLAY_FADES",					// 60
};

const char *GetLumpName( unsigned int lumpnum )
{
	if ( lumpnum >= ARRAYSIZE( s_LumpNames ) )
	{
		return "UNKNOWN";
	}
	return s_LumpNames[lumpnum];
}

// "-hdr" tells us to use the HDR fields (if present) on the light sources.  Also, tells us to write
// out the HDR lumps for lightmaps, ambient leaves, and lights sources.
bool g_bHDR = false;

// Set to true to generate Xbox360 native output files
static bool g_bSwapOnLoad = false;
static bool g_bSwapOnWrite = false;

VTFConvertFunc_t	g_pVTFConvertFunc;
VHVFixupFunc_t		g_pVHVFixupFunc;
CompressFunc_t		g_pCompressFunc;

CUtlVector< CUtlString >	g_StaticPropNames;
CUtlVector< int >			g_StaticPropInstances;

CByteswap	g_Swap;

uint32 g_LevelFlags = 0;

int			nummodels;
dmodel_t	dmodels[MAX_MAP_MODELS];

int			visdatasize;
byte		dvisdata[MAX_MAP_VISIBILITY];
dvis_t		*dvis = (dvis_t *)dvisdata;

CUtlVector<byte> dlightdataHDR;
CUtlVector<byte> dlightdataLDR;
CUtlVector<byte> *pdlightdata = &dlightdataLDR;

CUtlVector<char> dentdata;

int			numleafs;
#if !defined( BSP_USE_LESS_MEMORY )
dleaf_t		dleafs[MAX_MAP_LEAFS];
#else
dleaf_t		*dleafs;
#endif

CUtlVector<dleafambientindex_t> g_LeafAmbientIndexLDR;
CUtlVector<dleafambientindex_t> g_LeafAmbientIndexHDR;
CUtlVector<dleafambientindex_t> *g_pLeafAmbientIndex = NULL;
CUtlVector<dleafambientlighting_t> g_LeafAmbientLightingLDR;
CUtlVector<dleafambientlighting_t> g_LeafAmbientLightingHDR;
CUtlVector<dleafambientlighting_t> *g_pLeafAmbientLighting = NULL;

unsigned short  g_LeafMinDistToWater[MAX_MAP_LEAFS];

int			numplanes;
dplane_t	dplanes[MAX_MAP_PLANES];

int			numvertexes;
dvertex_t	dvertexes[MAX_MAP_VERTS];

int				g_numvertnormalindices;	// dfaces reference these. These index g_vertnormals.
unsigned short	g_vertnormalindices[MAX_MAP_VERTNORMALS];

int				g_numvertnormals;	
Vector			g_vertnormals[MAX_MAP_VERTNORMALS];

int			numnodes;
dnode_t		dnodes[MAX_MAP_NODES];

CUtlVector<texinfo_t> texinfo( MAX_MAP_TEXINFO );

int			numtexdata;
dtexdata_t	dtexdata[MAX_MAP_TEXDATA];

//
// displacement map bsp file info: dispinfo
//
CUtlVector<ddispinfo_t> g_dispinfo;
CUtlVector<CDispVert> g_DispVerts;
CUtlVector<CDispTri> g_DispTris;
CUtlVector<unsigned char> g_DispLightmapSamplePositions; // LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS

int         numorigfaces;
dface_t     dorigfaces[MAX_MAP_FACES];

int				g_numprimitives = 0;
dprimitive_t	g_primitives[MAX_MAP_PRIMITIVES];

int				g_numprimverts = 0;
dprimvert_t		g_primverts[MAX_MAP_PRIMVERTS];

int				g_numprimindices = 0;
unsigned short	g_primindices[MAX_MAP_PRIMINDICES];

int			numfaces;
dface_t		dfaces[MAX_MAP_FACES];

int			numfaceids;
CUtlVector<dfaceid_t>	dfaceids;


int			numfaces_hdr;
dface_t		dfaces_hdr[MAX_MAP_FACES];

int			numedges;
dedge_t		dedges[MAX_MAP_EDGES];

int			numleaffaces;
unsigned short		dleaffaces[MAX_MAP_LEAFFACES];

int			numleafbrushes;
unsigned short		dleafbrushes[MAX_MAP_LEAFBRUSHES];

int			numsurfedges;
int			dsurfedges[MAX_MAP_SURFEDGES];

int			numbrushes;
dbrush_t	dbrushes[MAX_MAP_BRUSHES];

int			numbrushsides;
dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

int			numareas;
darea_t		dareas[MAX_MAP_AREAS];

int			numareaportals;
dareaportal_t	dareaportals[MAX_MAP_AREAPORTALS];

int			numworldlightsLDR;
dworldlight_t dworldlightsLDR[MAX_MAP_WORLDLIGHTS];

int			numworldlightsHDR;
dworldlight_t dworldlightsHDR[MAX_MAP_WORLDLIGHTS];

int			*pNumworldlights = &numworldlightsLDR;
dworldlight_t *dworldlights = dworldlightsLDR;

int			numleafwaterdata = 0;
dleafwaterdata_t dleafwaterdata[MAX_MAP_LEAFWATERDATA]; 

CUtlVector<CFaceMacroTextureInfo>	g_FaceMacroTextureInfos;

Vector				g_ClipPortalVerts[MAX_MAP_PORTALVERTS];
int					g_nClipPortalVerts;

dcubemapsample_t	g_CubemapSamples[MAX_MAP_CUBEMAPSAMPLES];
int					g_nCubemapSamples = 0;

int					g_nOverlayCount;
doverlay_t			g_Overlays[MAX_MAP_OVERLAYS];
doverlayfade_t		g_OverlayFades[MAX_MAP_OVERLAYS];

int					g_nWaterOverlayCount;
dwateroverlay_t		g_WaterOverlays[MAX_MAP_WATEROVERLAYS];

CUtlVector<char>	g_TexDataStringData;
CUtlVector<int>		g_TexDataStringTable;

byte				*g_pPhysCollide = NULL;
int					g_PhysCollideSize = 0;
int					g_MapRevision = 0;

byte				*g_pPhysDisp = NULL;
int					g_PhysDispSize = 0;

CUtlVector<doccluderdata_t>	g_OccluderData( 256, 256 );
CUtlVector<doccluderpolydata_t>	g_OccluderPolyData( 1024, 1024 );
CUtlVector<int>	g_OccluderVertexIndices( 2048, 2048 );
 
template <class T> static void WriteData( T *pData, int count = 1 );
template <class T> static void WriteData( int fieldType, T *pData, int count = 1 );
template< class T > static void AddLump( int lumpnum, T *pData, int count, int version = 0 );
template< class T > static void AddLump( int lumpnum, CUtlVector<T> &data, int version = 0 );

dheader_t		*g_pBSPHeader;
FileHandle_t	g_hBSPFile;

struct Lump_t
{
	void	*pLumps[HEADER_LUMPS];
	int		size[HEADER_LUMPS];
	bool	bLumpParsed[HEADER_LUMPS];
} g_Lumps;

CGameLump	g_GameLumps;

static IZip *s_pakFile = 0;

//-----------------------------------------------------------------------------
// Keep the file position aligned to an arbitrary boundary.
// Returns updated file position.
//-----------------------------------------------------------------------------
static unsigned int AlignFilePosition( FileHandle_t hFile, int alignment )
{
	unsigned int currPosition = g_pFileSystem->Tell( hFile );

	if ( alignment >= 2 )
	{
		unsigned int newPosition = AlignValue( currPosition, alignment );
		unsigned int count = newPosition - currPosition;
		if ( count )
		{
			char *pBuffer;
			char smallBuffer[4096];
			if ( count > sizeof( smallBuffer ) )
			{
				pBuffer = (char *)malloc( count );
			}
			else
			{
				pBuffer = smallBuffer;
			}

			memset( pBuffer, 0, count );
			SafeWrite( hFile, pBuffer, count );

			if ( pBuffer != smallBuffer )
			{
				free( pBuffer );
			}

			currPosition = newPosition;
		}
	}

	return currPosition;
}

//-----------------------------------------------------------------------------
// Purpose: // Get a pakfile instance
// Output : IZip*
//-----------------------------------------------------------------------------
IZip* GetPakFile( void )
{
	if ( !s_pakFile )
	{
		s_pakFile = IZip::CreateZip();
	}
	return s_pakFile;
}

//-----------------------------------------------------------------------------
// Purpose: Free the pak files
//-----------------------------------------------------------------------------
void ReleasePakFileLumps( void )
{
	// Release the pak files
	IZip::ReleaseZip( s_pakFile );
	s_pakFile = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Set the sector alignment for all subsequent zip operations
//-----------------------------------------------------------------------------
void ForceAlignment( IZip *pak, bool bAlign, bool bCompatibleFormat, unsigned int alignmentSize )
{
	pak->ForceAlignment( bAlign, bCompatibleFormat, alignmentSize );
}

//-----------------------------------------------------------------------------
// Purpose: Store data back out to .bsp file
//-----------------------------------------------------------------------------
static void WritePakFileLump( void )
{
	CUtlBuffer buf( 0, 0 );
	GetPakFile()->ActivateByteSwapping( IsX360() );
	GetPakFile()->SaveToBuffer( buf );

	// must respect pak file alignment
	// pad up and ensure lump starts on same aligned boundary
	AlignFilePosition( g_hBSPFile, GetPakFile()->GetAlignment() );
	
	// Now store final buffers out to file
	AddLump( LUMP_PAKFILE, (byte*)buf.Base(), buf.TellPut() );
}

//-----------------------------------------------------------------------------
// Purpose: Remove all entries
//-----------------------------------------------------------------------------
void ClearPakFile( IZip *pak )
{
	pak->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Add file from disk to .bsp PAK lump
// Input  : *relativename - 
//			*fullpath - 
//-----------------------------------------------------------------------------
void AddFileToPak( IZip *pak, const char *relativename, const char *fullpath, IZip::eCompressionType compressionType )
{
	DevMsg( "Adding file to pakfile [ %s ]\n", fullpath );
	pak->AddFileToZip( relativename, fullpath, compressionType );
}

//-----------------------------------------------------------------------------
// Purpose: Add buffer to .bsp PAK lump as named file
// Input  : *relativename - 
//			*data - 
//			length - 
//-----------------------------------------------------------------------------
void AddBufferToPak( IZip *pak, const char *pRelativeName, void *data, int length, bool bTextMode, IZip::eCompressionType compressionType )
{
	pak->AddBufferToZip( pRelativeName, data, length, bTextMode, compressionType );
}

//-----------------------------------------------------------------------------
// Purpose: Add entire directory to .bsp PAK lump as named file
// Input  : *relativename - 
//			*data - 
//			length - 
//-----------------------------------------------------------------------------
void AddDirToPak( IZip *pak, const char *pDirPath, const char *pPakPrefix )
{
	if ( !g_pFullFileSystem->IsDirectory( pDirPath ) )
	{
		Warning( "Passed non-directory to AddDirToPak [ %s ]\n", pDirPath );
		return;
	}

	DevMsg( "Adding directory to pakfile [ %s ]\n", pDirPath );

	// Enumerate dir
	char szEnumerateDir[MAX_PATH] = { 0 };
	V_snprintf( szEnumerateDir, sizeof( szEnumerateDir ), "%s/*.*", pDirPath );
	V_FixSlashes( szEnumerateDir );

	FileFindHandle_t handle;
	const char *szFindResult = g_pFullFileSystem->FindFirst( szEnumerateDir, &handle );
	do
	{
		if ( szFindResult[0] != '.' )
		{
			char szPakName[MAX_PATH] = { 0 };
			char szFullPath[MAX_PATH] = { 0 };
			if ( pPakPrefix )
			{
				V_snprintf( szPakName, sizeof( szPakName ), "%s/%s", pPakPrefix, szFindResult );
			}
			else
			{
				V_strncpy( szPakName, szFindResult, sizeof( szPakName ) );
			}
			V_snprintf( szFullPath, sizeof( szFullPath ), "%s/%s", pDirPath, szFindResult );
			V_FixDoubleSlashes( szFullPath );
			V_FixDoubleSlashes( szPakName );

			if ( g_pFullFileSystem->FindIsDirectory( handle ) )
			{
				// Recurse
				AddDirToPak( pak, szFullPath, szPakName );
			}
			else
			{
				// Just add this file
				AddFileToPak( pak, szPakName, szFullPath );
			}
		}
		szFindResult = g_pFullFileSystem->FindNext( handle );
	} while ( szFindResult);
}

//-----------------------------------------------------------------------------
// Purpose: Check if a file already exists in the pack file.
// Input  : *relativename - 
//-----------------------------------------------------------------------------
bool FileExistsInPak( IZip *pak, const char *pRelativeName )
{
	return pak->FileExistsInZip( pRelativeName );
}


//-----------------------------------------------------------------------------
// Read a file from the pack file
//-----------------------------------------------------------------------------
bool ReadFileFromPak( IZip *pak, const char *pRelativeName, bool bTextMode, CUtlBuffer &buf )
{
	return pak->ReadFileFromZip( pRelativeName, bTextMode, buf );
}


//-----------------------------------------------------------------------------
// Purpose: Remove file from .bsp PAK lump
// Input  : *relativename - 
//-----------------------------------------------------------------------------
void RemoveFileFromPak( IZip *pak, const char *relativename )
{
	pak->RemoveFileFromZip( relativename );
}


//-----------------------------------------------------------------------------
// Purpose: Get next filename in directory
// Input  : id, -1 to start, returns next id, or -1 at list conclusion 
//-----------------------------------------------------------------------------
int GetNextFilename( IZip *pak, int id, char *pBuffer, int bufferSize, int &fileSize )
{
	return pak->GetNextFilename( id, pBuffer, bufferSize, fileSize );
}

//-----------------------------------------------------------------------------
// Convert four-CC code to a handle	+ back
//-----------------------------------------------------------------------------
GameLumpHandle_t CGameLump::GetGameLumpHandle( GameLumpId_t id )
{
	// NOTE: I'm also expecting game lump id's to be four-CC codes
	Assert( id > HEADER_LUMPS );

	FOR_EACH_LL(m_GameLumps, i)
	{
		if (m_GameLumps[i].m_Id == id)
			return i;
	}

	return InvalidGameLump();
}

GameLumpId_t CGameLump::GetGameLumpId( GameLumpHandle_t handle )
{
	return m_GameLumps[handle].m_Id;
}

int	CGameLump::GetGameLumpFlags( GameLumpHandle_t handle )
{
	return m_GameLumps[handle].m_Flags;
}

int	CGameLump::GetGameLumpVersion( GameLumpHandle_t handle )
{
	return m_GameLumps[handle].m_Version;
}


//-----------------------------------------------------------------------------
// Game lump accessor methods 
//-----------------------------------------------------------------------------

void*	CGameLump::GetGameLump( GameLumpHandle_t id )
{
	return m_GameLumps[id].m_Memory.Base();
}

int		CGameLump::GameLumpSize( GameLumpHandle_t id )
{
	return m_GameLumps[id].m_Memory.NumAllocated();
}


//-----------------------------------------------------------------------------
// Game lump iteration methods 
//-----------------------------------------------------------------------------

GameLumpHandle_t	CGameLump::FirstGameLump()
{
	return (m_GameLumps.Count()) ? m_GameLumps.Head() : InvalidGameLump();
}

GameLumpHandle_t	CGameLump::NextGameLump( GameLumpHandle_t handle )
{

	return (m_GameLumps.IsValidIndex(handle)) ? m_GameLumps.Next(handle) : InvalidGameLump();
}

GameLumpHandle_t	CGameLump::InvalidGameLump()
{
	return 0xFFFF;
}


//-----------------------------------------------------------------------------
// Game lump creation/destruction method
//-----------------------------------------------------------------------------

GameLumpHandle_t	CGameLump::CreateGameLump( GameLumpId_t id, int size, int flags, int version )
{
	Assert( GetGameLumpHandle(id) == InvalidGameLump() );
	GameLumpHandle_t handle = m_GameLumps.AddToTail();
	m_GameLumps[handle].m_Id = id;
	m_GameLumps[handle].m_Flags = flags;
	m_GameLumps[handle].m_Version = version;
	m_GameLumps[handle].m_Memory.EnsureCapacity( size );
	return handle;
}

void	CGameLump::DestroyGameLump( GameLumpHandle_t handle )
{
	m_GameLumps.Remove( handle );
}

void	CGameLump::DestroyAllGameLumps()
{
	m_GameLumps.RemoveAll();
}

//-----------------------------------------------------------------------------
// Compute file size and clump count
//-----------------------------------------------------------------------------

void CGameLump::ComputeGameLumpSizeAndCount( int& size, int& clumpCount )
{
	// Figure out total size of the client lumps
	size = 0;
	clumpCount = 0;
	GameLumpHandle_t h;
	for( h = FirstGameLump(); h != InvalidGameLump(); h = NextGameLump( h ) )
	{
		++clumpCount;
		size += GameLumpSize( h );
	}

	// Add on headers
	size += sizeof( dgamelumpheader_t ) + clumpCount * sizeof( dgamelump_t );
}


void CGameLump::SwapGameLump( GameLumpId_t id, int version, byte *dest, byte *src, int length )
{
	int count = 0;
	switch( id )
	{
	case GAMELUMP_STATIC_PROPS:
		// Swap the static prop model dict
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		g_Swap.SwapFieldsToTargetEndian( (StaticPropDictLump_t*)dest, (StaticPropDictLump_t*)src, count );
		src += sizeof(StaticPropDictLump_t) * count;
		dest += sizeof(StaticPropDictLump_t) * count;

		// Swap the leaf list
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		g_Swap.SwapFieldsToTargetEndian( (StaticPropLeafLump_t*)dest, (StaticPropLeafLump_t*)src, count );
		src += sizeof(StaticPropLeafLump_t) * count;
		dest += sizeof(StaticPropLeafLump_t) * count;

		// Swap the models
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		// The one-at-a-time swap is to compensate for these structures 
		// possibly being misaligned, which crashes the Xbox 360.
		if ( version == 4 )
		{
			StaticPropLumpV4_t lump;
			for ( int i = 0; i < count; ++i )
			{
				Q_memcpy( &lump, src, sizeof(StaticPropLumpV4_t) );
				g_Swap.SwapFieldsToTargetEndian( &lump, &lump );
				Q_memcpy( dest, &lump, sizeof(StaticPropLumpV4_t) );
				src += sizeof( StaticPropLumpV4_t );
				dest += sizeof( StaticPropLumpV4_t );
			}
		}
		else if ( version == 5 )
		{
			StaticPropLumpV5_t lump;
			for ( int i = 0; i < count; ++i )
			{
				Q_memcpy( &lump, src, sizeof(StaticPropLumpV5_t) );
				g_Swap.SwapFieldsToTargetEndian( &lump, &lump );
				Q_memcpy( dest, &lump, sizeof(StaticPropLumpV5_t) );
				src += sizeof( StaticPropLumpV5_t );
				dest += sizeof( StaticPropLumpV5_t );
			}
		}
		else
		{
			if ( version != 6 )
			{
				Error( "Unknown Static Prop Lump version %d didn't get swapped!\n", version );
			}

			StaticPropLump_t lump;
			for ( int i = 0; i < count; ++i )
			{
				Q_memcpy( &lump, src, sizeof(StaticPropLump_t) );
				g_Swap.SwapFieldsToTargetEndian( &lump, &lump );
				Q_memcpy( dest, &lump, sizeof(StaticPropLump_t) );
				src += sizeof( StaticPropLump_t );
				dest += sizeof( StaticPropLump_t );
			}
		}
		break;

	case GAMELUMP_DETAIL_PROPS:
		// Swap the detail prop model dict
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		g_Swap.SwapFieldsToTargetEndian( (DetailObjectDictLump_t*)dest, (DetailObjectDictLump_t*)src, count );
		src += sizeof(DetailObjectDictLump_t) * count;
		dest += sizeof(DetailObjectDictLump_t) * count;

		if ( version == 4 )
		{
			// Swap the detail sprite dict
			count = *(int*)src;
			g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
			count = g_bSwapOnLoad ? *(int*)dest : count;
			src += sizeof(int);
			dest += sizeof(int);

			DetailSpriteDictLump_t spritelump;
			for ( int i = 0; i < count; ++i )
			{
				Q_memcpy( &spritelump, src, sizeof(DetailSpriteDictLump_t) );
				g_Swap.SwapFieldsToTargetEndian( &spritelump, &spritelump );
				Q_memcpy( dest, &spritelump, sizeof(DetailSpriteDictLump_t) );
				src += sizeof(DetailSpriteDictLump_t);
				dest += sizeof(DetailSpriteDictLump_t);
			}

			// Swap the models
			count = *(int*)src;
			g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
			count = g_bSwapOnLoad ? *(int*)dest : count;
			src += sizeof(int);
			dest += sizeof(int);

			DetailObjectLump_t objectlump;
			for ( int i = 0; i < count; ++i )
			{
				Q_memcpy( &objectlump, src, sizeof(DetailObjectLump_t) );
				g_Swap.SwapFieldsToTargetEndian( &objectlump, &objectlump );
				Q_memcpy( dest, &objectlump, sizeof(DetailObjectLump_t) );
				src += sizeof(DetailObjectLump_t);
				dest += sizeof(DetailObjectLump_t);
			}
		}
		break;

	case GAMELUMP_DETAIL_PROP_LIGHTING:
		// Swap the LDR light styles
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		g_Swap.SwapFieldsToTargetEndian( (DetailPropLightstylesLump_t*)dest, (DetailPropLightstylesLump_t*)src, count );
		src += sizeof(DetailObjectDictLump_t) * count;
		dest += sizeof(DetailObjectDictLump_t) * count;
		break;

	case GAMELUMP_DETAIL_PROP_LIGHTING_HDR:
		// Swap the HDR light styles
		count = *(int*)src;
		g_Swap.SwapBufferToTargetEndian( (int*)dest, (int*)src );
		count = g_bSwapOnLoad ? *(int*)dest : count;
		src += sizeof(int);
		dest += sizeof(int);

		g_Swap.SwapFieldsToTargetEndian( (DetailPropLightstylesLump_t*)dest, (DetailPropLightstylesLump_t*)src, count );
		src += sizeof(DetailObjectDictLump_t) * count;
		dest += sizeof(DetailObjectDictLump_t) * count;
		break;

	default:
		char idchars[5] = {0};
		Q_memcpy( idchars, &id, 4 );
		Warning( "Unknown game lump '%s' didn't get swapped!\n", idchars );
		memcpy ( dest, src, length);
		break;
	}
}

//-----------------------------------------------------------------------------
// Game lump file I/O
//-----------------------------------------------------------------------------
void CGameLump::ParseGameLump( dheader_t* pHeader )
{
	g_GameLumps.DestroyAllGameLumps();

	g_Lumps.bLumpParsed[LUMP_GAME_LUMP] = true;

	int length = pHeader->lumps[LUMP_GAME_LUMP].filelen;
	int ofs = pHeader->lumps[LUMP_GAME_LUMP].fileofs;
	
	if (length > 0)
	{
		// Read dictionary...
		dgamelumpheader_t* pGameLumpHeader = (dgamelumpheader_t*)((byte *)pHeader + ofs);
		if ( g_bSwapOnLoad )
		{
			g_Swap.SwapFieldsToTargetEndian( pGameLumpHeader );
		}
		dgamelump_t* pGameLump = (dgamelump_t*)(pGameLumpHeader + 1);
		for (int i = 0; i < pGameLumpHeader->lumpCount; ++i )
		{
			if ( g_bSwapOnLoad )
			{
				g_Swap.SwapFieldsToTargetEndian( &pGameLump[i] );
			}

			int length = pGameLump[i].filelen;
			GameLumpHandle_t lump = g_GameLumps.CreateGameLump( pGameLump[i].id, length, pGameLump[i].flags, pGameLump[i].version );
			if ( g_bSwapOnLoad )
			{
				SwapGameLump( pGameLump[i].id, pGameLump[i].version, (byte*)g_GameLumps.GetGameLump(lump), (byte *)pHeader + pGameLump[i].fileofs, length );
			}
			else
			{
				memcpy( g_GameLumps.GetGameLump(lump), (byte *)pHeader + pGameLump[i].fileofs, length );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// String table methods
//-----------------------------------------------------------------------------
const char *TexDataStringTable_GetString( int stringID )
{
	return &g_TexDataStringData[g_TexDataStringTable[stringID]];
}

int	TexDataStringTable_AddOrFindString( const char *pString )
{
	int i;
	// garymcthack: Make this use an RBTree!
	for( i = 0; i < g_TexDataStringTable.Count(); i++ )
	{
		if( stricmp( pString, &g_TexDataStringData[g_TexDataStringTable[i]] ) == 0 )
		{
			return i;
		}
	}

	int len = V_strlen( pString );
	int outOffset = g_TexDataStringData.AddMultipleToTail( len+1, pString );
	int outIndex = g_TexDataStringTable.AddToTail( outOffset );
	return outIndex;
}

//-----------------------------------------------------------------------------
// Adds all game lumps into one big block
//-----------------------------------------------------------------------------

static void AddGameLumps( )
{
	// Figure out total size of the client lumps
	int size, clumpCount;
	g_GameLumps.ComputeGameLumpSizeAndCount( size, clumpCount );

	// Set up the main lump dictionary entry
	g_Lumps.size[LUMP_GAME_LUMP] = 0;	// mark it written

	lump_t* lump = &g_pBSPHeader->lumps[LUMP_GAME_LUMP];
	
	lump->fileofs = g_pFileSystem->Tell( g_hBSPFile );
	lump->filelen = size;

	// write header
	dgamelumpheader_t header;
	header.lumpCount = clumpCount;
	WriteData( &header );

	// write dictionary
	dgamelump_t dict;
	int offset = lump->fileofs + sizeof(header) + clumpCount * sizeof(dgamelump_t);
	GameLumpHandle_t h;
	for( h = g_GameLumps.FirstGameLump(); h != g_GameLumps.InvalidGameLump(); h = g_GameLumps.NextGameLump( h ) )
	{
		dict.id = g_GameLumps.GetGameLumpId(h);
		dict.version = g_GameLumps.GetGameLumpVersion(h);
		dict.flags = g_GameLumps.GetGameLumpFlags(h);
		dict.fileofs = offset;
		dict.filelen = g_GameLumps.GameLumpSize( h );
		offset += dict.filelen;

		WriteData( &dict );
	}

	// write lumps..
	for( h = g_GameLumps.FirstGameLump(); h != g_GameLumps.InvalidGameLump(); h = g_GameLumps.NextGameLump( h ) )
	{
		unsigned int lumpsize = g_GameLumps.GameLumpSize(h);
		if ( g_bSwapOnWrite )
		{
			g_GameLumps.SwapGameLump( g_GameLumps.GetGameLumpId(h), g_GameLumps.GetGameLumpVersion(h), (byte*)g_GameLumps.GetGameLump(h), (byte*)g_GameLumps.GetGameLump(h), lumpsize );
		}
		SafeWrite( g_hBSPFile, g_GameLumps.GetGameLump(h), lumpsize );
	}

	// align to doubleword
	AlignFilePosition( g_hBSPFile, 4 );
}


//-----------------------------------------------------------------------------
// Adds the occluder lump...
//-----------------------------------------------------------------------------
static void AddOcclusionLump( )
{
	g_Lumps.size[LUMP_OCCLUSION] = 0;	// mark it written

	int nOccluderCount = g_OccluderData.Count();
	int nOccluderPolyDataCount = g_OccluderPolyData.Count();
	int nOccluderVertexIndices = g_OccluderVertexIndices.Count();

	int nLumpLength = nOccluderCount * sizeof(doccluderdata_t) +
		nOccluderPolyDataCount * sizeof(doccluderpolydata_t) +
		nOccluderVertexIndices * sizeof(int) +
		3 * sizeof(int);

	lump_t *lump = &g_pBSPHeader->lumps[LUMP_OCCLUSION];

	lump->fileofs = g_pFileSystem->Tell( g_hBSPFile );
	lump->filelen = nLumpLength;
	lump->version = LUMP_OCCLUSION_VERSION;
	lump->uncompressedSize = 0;

	// Data is swapped in place, so the 'Count' variables aren't safe to use after they're written
	WriteData( FIELD_INTEGER, &nOccluderCount );
	WriteData( (doccluderdata_t*)g_OccluderData.Base(), g_OccluderData.Count() );
	WriteData( FIELD_INTEGER, &nOccluderPolyDataCount );
	WriteData( (doccluderpolydata_t*)g_OccluderPolyData.Base(), g_OccluderPolyData.Count() );
	WriteData( FIELD_INTEGER, &nOccluderVertexIndices );
	WriteData( FIELD_INTEGER, (int*)g_OccluderVertexIndices.Base(), g_OccluderVertexIndices.Count() );
}


//-----------------------------------------------------------------------------
// Loads the occluder lump...
//-----------------------------------------------------------------------------
static void UnserializeOcclusionLumpV2( CUtlBuffer &buf )
{
	int nCount = buf.GetInt();
	if ( nCount )
	{
		g_OccluderData.SetCount( nCount );
		buf.GetObjects( g_OccluderData.Base(), nCount );
	}

	nCount = buf.GetInt();
	if ( nCount )
	{
		g_OccluderPolyData.SetCount( nCount );
		buf.GetObjects( g_OccluderPolyData.Base(), nCount );
	}

	nCount = buf.GetInt();
	if ( nCount )
	{
		if ( g_bSwapOnLoad )
		{
			g_Swap.SwapBufferToTargetEndian( (int*)buf.PeekGet(), (int*)buf.PeekGet(), nCount );
		}
		g_OccluderVertexIndices.SetCount( nCount );
		buf.Get( g_OccluderVertexIndices.Base(), nCount * sizeof(g_OccluderVertexIndices[0]) );
	}
}


static void LoadOcclusionLump()
{
	g_OccluderData.RemoveAll();
	g_OccluderPolyData.RemoveAll();
	g_OccluderVertexIndices.RemoveAll();

	int		length, ofs;

	g_Lumps.bLumpParsed[LUMP_OCCLUSION] = true;

	length = g_pBSPHeader->lumps[LUMP_OCCLUSION].filelen;
	ofs = g_pBSPHeader->lumps[LUMP_OCCLUSION].fileofs;
	
	CUtlBuffer buf( (byte *)g_pBSPHeader + ofs, length, CUtlBuffer::READ_ONLY );
	buf.ActivateByteSwapping( g_bSwapOnLoad );
	switch ( g_pBSPHeader->lumps[LUMP_OCCLUSION].version )
	{
	case 2:
		UnserializeOcclusionLumpV2( buf );
		break;

	case 0:
		break;

	default:
		Error("Unknown occlusion lump version!\n");
		break;
	}
}


/*
===============
CompressVis

===============
*/
int CompressVis (byte *vis, byte *dest)
{
	int		j;
	int		rep;
	int		visrow;
	byte	*dest_p;
	
	dest_p = dest;
//	visrow = (r_numvisleafs + 7)>>3;
	visrow = (dvis->numclusters + 7)>>3;
	
	for (j=0 ; j<visrow ; j++)
	{
		*dest_p++ = vis[j];
		if (vis[j])
			continue;

		rep = 1;
		for ( j++; j<visrow ; j++)
			if (vis[j] || rep == 255)
				break;
			else
				rep++;
		*dest_p++ = rep;
		j--;
	}
	
	return dest_p - dest;
}


/*
===================
DecompressVis
===================
*/
void DecompressVis (byte *in, byte *decompressed)
{
	int		c;
	byte	*out;
	int		row;

//	row = (r_numvisleafs+7)>>3;	
	row = (dvis->numclusters+7)>>3;	
	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		if (!c)
			Error ("DecompressVis: 0 repeat");
		in += 2;
		if ((out - decompressed) + c > row)
		{
			c = row - (out - decompressed);
			Warning( "warning: Vis decompression overrun\n" );
		}
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
}

//-----------------------------------------------------------------------------
//	Lump-specific swap functions
//-----------------------------------------------------------------------------
struct swapcollideheader_t
{
	DECLARE_BYTESWAP_DATADESC();
	int		size;
	int		vphysicsID;
	short	version;
	short	modelType;
};

struct swapcompactsurfaceheader_t : swapcollideheader_t
{
	DECLARE_BYTESWAP_DATADESC();
	int		surfaceSize;
	Vector	dragAxisAreas;
	int		axisMapSize;
};

struct swapmoppsurfaceheader_t : swapcollideheader_t
{
	DECLARE_BYTESWAP_DATADESC();
	int moppSize;
};

BEGIN_BYTESWAP_DATADESC( swapcollideheader_t )
	DEFINE_FIELD( size, FIELD_INTEGER ),
	DEFINE_FIELD( vphysicsID, FIELD_INTEGER ),
	DEFINE_FIELD( version, FIELD_SHORT ),
	DEFINE_FIELD( modelType, FIELD_SHORT ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC_( swapcompactsurfaceheader_t, swapcollideheader_t )
	DEFINE_FIELD( surfaceSize, FIELD_INTEGER ),
	DEFINE_FIELD( dragAxisAreas, FIELD_VECTOR ),
	DEFINE_FIELD( axisMapSize, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC_( swapmoppsurfaceheader_t, swapcollideheader_t )
	DEFINE_FIELD( moppSize, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()


static void SwapPhyscollideLump( byte *pDestBase, byte *pSrcBase, unsigned int &count )
{
	IPhysicsCollision *physcollision = NULL;
	CSysModule *pPhysicsModule = g_pFullFileSystem->LoadModule( "vphysics.dll" );
	if ( pPhysicsModule )
	{
		CreateInterfaceFn physicsFactory = Sys_GetFactory( pPhysicsModule );
		if ( physicsFactory )
		{
			physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL );
		}
	}

	if ( !physcollision )
	{
		Warning("!!! WARNING: Can't swap the physcollide lump!\n" );
		return;
	}

	// physics data is variable length.  The last physmodel is a NULL pointer
	// with modelIndex -1, dataSize -1
	dphysmodel_t *pPhysModel;
	byte *pSrc = pSrcBase;

	// first the src chunks have to be aligned properly
	// swap increases size, allocate enough expansion room
	byte *pSrcAlignedBase = (byte*)malloc( 2*count );
	byte *basePtr = pSrcAlignedBase;
	byte *pSrcAligned = pSrcAlignedBase;

	do
	{
		if ( g_bSwapOnLoad )
		{
			g_Swap.SwapFieldsToTargetEndian( (dphysmodel_t*)pSrcAligned, (dphysmodel_t*)pSrc );
		}
		else
		{
			Q_memcpy( pSrcAligned, pSrc, sizeof(dphysmodel_t) );
		}
		pPhysModel = (dphysmodel_t*)pSrcAligned;

		pSrc += sizeof(dphysmodel_t);
		pSrcAligned += sizeof(dphysmodel_t);

		if ( pPhysModel->dataSize > 0 )
		{		
			// Align the collide headers
			for ( int i = 0; i < pPhysModel->solidCount; ++i )
			{
				// Get data size
				int size;
				Q_memcpy( &size, pSrc, sizeof(int) );
				if ( g_bSwapOnLoad )
					size = SwapLong( size );

				// Fixup size
				int padBytes = 0;
				if ( size % 4 != 0 )
				{
					padBytes = ( 4 - size % 4 );
					count += padBytes;
					pPhysModel->dataSize += padBytes;
				}

				// Copy data and size into alligned buffer
				int newsize = size + padBytes;
				if ( g_bSwapOnLoad )
					newsize = SwapLong( newsize );

				Q_memcpy( pSrcAligned, &newsize, sizeof(int) );
				Q_memcpy( pSrcAligned + sizeof(int), pSrc + sizeof(int), size );
				pSrcAligned += size + padBytes + sizeof(int);
				pSrc += size + sizeof(int);
			}

			int padBytes = 0;
			int dataSize = pPhysModel->dataSize + pPhysModel->keydataSize;
			Q_memcpy( pSrcAligned, pSrc, pPhysModel->keydataSize );
			pSrc += pPhysModel->keydataSize;
			pSrcAligned += pPhysModel->keydataSize;
			if ( dataSize % 4 != 0 )
			{
				// Next chunk will be unaligned
				padBytes = ( 4 - dataSize % 4 );
				pPhysModel->keydataSize += padBytes;
				count += padBytes;
				Q_memset( pSrcAligned, 0, padBytes );
				pSrcAligned += padBytes;
			}
		}
	} while ( pPhysModel->dataSize > 0 );

	// Now the data can be swapped properly
	pSrcBase = pSrcAlignedBase;
	pSrc = pSrcBase;
	byte *pDest = pDestBase;

	do
	{
		// src headers are in native format
		pPhysModel = (dphysmodel_t*)pSrc;
		if ( g_bSwapOnWrite )
		{
			g_Swap.SwapFieldsToTargetEndian( (dphysmodel_t*)pDest, (dphysmodel_t*)pSrc );
		}
		else
		{
			Q_memcpy( pDest, pSrc, sizeof(dphysmodel_t) );
		}

		pSrc += sizeof(dphysmodel_t);
		pDest += sizeof(dphysmodel_t);

		pSrcBase = pSrc;
		pDestBase = pDest;

		if ( pPhysModel->dataSize > 0 )
		{		
			vcollide_t collide = {0};
			int dataSize = pPhysModel->dataSize + pPhysModel->keydataSize;

			if ( g_bSwapOnWrite )
			{
				// Load the collide data
				physcollision->VCollideLoad( &collide, pPhysModel->solidCount, (const char *)pSrc, dataSize, false );
			}

			int *offsets = new int[ pPhysModel->solidCount ];

			// Swap the collision data headers
			for ( int i = 0; i < pPhysModel->solidCount; ++i )
			{
				int headerSize = 0;
				swapcollideheader_t *baseHdr = (swapcollideheader_t*)pSrc;
				short modelType = baseHdr->modelType;
				if ( g_bSwapOnLoad )
				{
					g_Swap.SwapBufferToTargetEndian( &modelType );
				}

				if ( modelType == 0 ) // COLLIDE_POLY
				{
					headerSize = sizeof(swapcompactsurfaceheader_t);
					swapcompactsurfaceheader_t swapHdr;
					Q_memcpy( &swapHdr, pSrc, headerSize );
					g_Swap.SwapFieldsToTargetEndian( &swapHdr, &swapHdr );
					Q_memcpy( pDest, &swapHdr, headerSize );
				}
				else if ( modelType == 1 ) // COLLIDE_MOPP
				{
					// The PC still unserializes these, but we don't support them 
					if ( g_bSwapOnWrite )
					{
						collide.solids[i] = NULL;
					}

					headerSize = sizeof(swapmoppsurfaceheader_t);
					swapmoppsurfaceheader_t swapHdr;
					Q_memcpy( &swapHdr, pSrc, headerSize );
					g_Swap.SwapFieldsToTargetEndian( &swapHdr, &swapHdr );
					Q_memcpy( pDest, &swapHdr, headerSize );

				}
				else
				{
					// Shouldn't happen
					Assert( 0 );
				}

				if ( g_bSwapOnLoad )
				{
					// src needs the native header data to load the vcollides
					Q_memcpy( pSrc, pDest, headerSize );
				}
				// HACK: Need either surfaceSize or moppSize - both sit at the same offset in the structure
				swapmoppsurfaceheader_t *hdr = (swapmoppsurfaceheader_t*)pSrc;
				pSrc += hdr->size + sizeof(int);
				pDest += hdr->size + sizeof(int);
				offsets[i] = hdr->size;
			}

			pSrc = pSrcBase;
			pDest = pDestBase;
			if ( g_bSwapOnLoad )
			{
				physcollision->VCollideLoad( &collide, pPhysModel->solidCount, (const char *)pSrc, dataSize, true );
			}

			// Write out the ledge tree data
			for ( int i = 0; i < pPhysModel->solidCount; ++i )
			{
				if ( collide.solids[i] )
				{
					// skip over the size member
					pSrc += sizeof(int);
					pDest += sizeof(int);
					int offset = physcollision->CollideWrite( (char*)pDest, collide.solids[i], g_bSwapOnWrite );
					pSrc += offset;
					pDest += offset;
				}
				else
				{
					pSrc += offsets[i] + sizeof(int);
					pDest += offsets[i] + sizeof(int);
				}
			}

			// copy the keyvalues data
			Q_memcpy( pDest, pSrc, pPhysModel->keydataSize );
			pDest += pPhysModel->keydataSize;
			pSrc += pPhysModel->keydataSize;

			// Free the memory
			physcollision->VCollideUnload( &collide );
			delete [] offsets;
		}

		// avoid infinite loop on badly formed file
		if ( (pSrc - basePtr) > count )
			break;

	} while ( pPhysModel->dataSize > 0 );

	free( pSrcAlignedBase );
}


// UNDONE: This code is not yet tested.
static void SwapPhysdispLump( byte *pDest, byte *pSrc, int count )
{
	// the format of this lump is one unsigned short dispCount, then dispCount unsigned shorts of sizes
	// followed by an array of variable length (each element is the length of the corresponding entry in the
	// previous table) byte-stream data structure of the displacement collision models
	// these byte-stream structs are endian-neutral because each element is byte-sized
	unsigned short dispCount = *(unsigned short*)pSrc;
	if ( g_bSwapOnLoad )
	{
		g_Swap.SwapBufferToTargetEndian( &dispCount );
	}
	g_Swap.SwapBufferToTargetEndian( (unsigned short*)pDest, (unsigned short*)pSrc, dispCount + 1 );

	const int nBytes = (dispCount + 1) * sizeof( unsigned short );
	pSrc += nBytes;
	pDest += nBytes;
	count -= nBytes;

	g_Swap.SwapBufferToTargetEndian( pDest, pSrc, count );
}


static void SwapVisibilityLump( byte *pDest, byte *pSrc, int count )
{
	int firstInt = *(int*)pSrc;
	if ( g_bSwapOnLoad )
	{
		g_Swap.SwapBufferToTargetEndian( &firstInt );
	}
	int intCt = firstInt * 2 + 1;
	const int hdrSize = intCt * sizeof(int);
	g_Swap.SwapBufferToTargetEndian( (int*)pDest, (int*)pSrc, intCt );
	g_Swap.SwapBufferToTargetEndian( pDest + hdrSize, pSrc + hdrSize, count - hdrSize  );
}

//=============================================================================
void Lumps_Init( void )
{
	memset( &g_Lumps, 0, sizeof(g_Lumps) );
}

int LumpVersion( int lump )
{
	return g_pBSPHeader->lumps[lump].version;
}

bool HasLump( int lump )
{
	return g_pBSPHeader->lumps[lump].filelen > 0;
}

void ValidateLump( int lump, int length, int size, int forceVersion )
{
	if ( length % size )
	{
		Error( "ValidateLump: odd size for lump %d", lump );
	}

	if ( forceVersion >= 0 && forceVersion != g_pBSPHeader->lumps[lump].version )
	{
		Error( "ValidateLump: old version for lump %d in map!", lump );
	}
}

//-----------------------------------------------------------------------------
//	Add Lumps of integral types without datadescs
//-----------------------------------------------------------------------------
template< class T >
int CopyLumpInternal( int fieldType, int lump, T *dest, int forceVersion )
{
	g_Lumps.bLumpParsed[lump] = true;

	// Vectors are passed in as floats
	int fieldSize = ( fieldType == FIELD_VECTOR ) ? sizeof(Vector) : sizeof(T);
	unsigned int length = g_pBSPHeader->lumps[lump].filelen;
	unsigned int ofs = g_pBSPHeader->lumps[lump].fileofs;

	// count must be of the integral type
	unsigned int count = length / sizeof(T);
	
	ValidateLump( lump, length, fieldSize, forceVersion );

	if ( g_bSwapOnLoad )
	{
		switch( lump )
		{
		case LUMP_VISIBILITY:
			SwapVisibilityLump( (byte*)dest, ((byte*)g_pBSPHeader + ofs), count );
			break;
		
		case LUMP_PHYSCOLLIDE:
			// SwapPhyscollideLump may change size
			SwapPhyscollideLump( (byte*)dest, ((byte*)g_pBSPHeader + ofs), count );
			length = count;
			break;

		case LUMP_PHYSDISP:
			SwapPhysdispLump( (byte*)dest, ((byte*)g_pBSPHeader + ofs), count );
			break;

		default:
			g_Swap.SwapBufferToTargetEndian( dest, (T*)((byte*)g_pBSPHeader + ofs), count );
			break;
		}
	}
	else
	{
		memcpy( dest, (byte*)g_pBSPHeader + ofs, length );
	}

	// Return actual count of elements
	return length / fieldSize;
}

template< class T >
int CopyLump( int fieldType, int lump, T *dest, int forceVersion = -1 )
{
	return CopyLumpInternal( fieldType, lump, dest, forceVersion );
}

template< class T >
void CopyLump( int fieldType, int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	Assert( fieldType != FIELD_VECTOR ); // TODO: Support this if necessary
	dest.SetSize( g_pBSPHeader->lumps[lump].filelen / sizeof(T) );
	CopyLumpInternal( fieldType, lump, dest.Base(), forceVersion );
}

template< class T >
void CopyOptionalLump( int fieldType, int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	// not fatal if not present
	if ( !HasLump( lump ) )
		return;

	dest.SetSize( g_pBSPHeader->lumps[lump].filelen / sizeof(T) );
	CopyLumpInternal( fieldType, lump, dest.Base(), forceVersion );
}

template< class T >
int CopyVariableLump( int fieldType, int lump, void **dest, int forceVersion = -1 )
{
	int length = g_pBSPHeader->lumps[lump].filelen;
	*dest = malloc( length );

	return CopyLumpInternal<T>( fieldType, lump, (T*)*dest, forceVersion );
}

//-----------------------------------------------------------------------------
//	Add Lumps of object types with datadescs
//-----------------------------------------------------------------------------
template< class T >
int CopyLumpInternal( int lump, T *dest, int forceVersion )
{
	g_Lumps.bLumpParsed[lump] = true;

	unsigned int length = g_pBSPHeader->lumps[lump].filelen;
	unsigned int ofs = g_pBSPHeader->lumps[lump].fileofs;
	unsigned int count = length / sizeof(T);
	
	ValidateLump( lump, length, sizeof(T), forceVersion );

	if ( g_bSwapOnLoad )
	{
		g_Swap.SwapFieldsToTargetEndian( dest, (T*)((byte*)g_pBSPHeader + ofs), count );
	}
	else
	{
		memcpy( dest, (byte*)g_pBSPHeader + ofs, length );
	}

	return count;
}

template< class T >
int CopyLump( int lump, T *dest, int forceVersion = -1 )
{
	return CopyLumpInternal( lump, dest, forceVersion );
}

template< class T >
void CopyLump( int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	dest.SetSize( g_pBSPHeader->lumps[lump].filelen / sizeof(T) );
	CopyLumpInternal( lump, dest.Base(), forceVersion );
}

template< class T >
void CopyOptionalLump( int lump, CUtlVector<T> &dest, int forceVersion = -1 )
{
	// not fatal if not present
	if ( !HasLump( lump ) )
		return;

	dest.SetSize( g_pBSPHeader->lumps[lump].filelen / sizeof(T) );
	CopyLumpInternal( lump, dest.Base(), forceVersion );
}

template< class T >
int CopyVariableLump( int lump, void **dest, int forceVersion = -1 )
{
	int length = g_pBSPHeader->lumps[lump].filelen;
	*dest = malloc( length );

	return CopyLumpInternal<T>( lump, (T*)*dest, forceVersion );
}

//-----------------------------------------------------------------------------
//	Add/Write unknown lumps
//-----------------------------------------------------------------------------
void Lumps_Parse( void )
{
	int i;

	for ( i = 0; i < HEADER_LUMPS; i++ )
	{
		if ( !g_Lumps.bLumpParsed[i] && g_pBSPHeader->lumps[i].filelen )
		{
			g_Lumps.size[i] = CopyVariableLump<byte>( FIELD_CHARACTER, i, &g_Lumps.pLumps[i], -1 );
			Msg( "Reading unknown lump #%d (%d bytes)\n", i, g_Lumps.size[i] );
		}
	}
}

void Lumps_Write( void )
{
	int i;

	for ( i = 0; i < HEADER_LUMPS; i++ )
	{
		if ( g_Lumps.size[i] )
		{
			Msg( "Writing unknown lump #%d (%d bytes)\n", i, g_Lumps.size[i] );
			AddLump( i, (byte*)g_Lumps.pLumps[i], g_Lumps.size[i] );
		}
		if ( g_Lumps.pLumps[i] )
		{
			free( g_Lumps.pLumps[i] );
			g_Lumps.pLumps[i] = NULL;
		}
	}
}

int LoadLeafs( void )
{
#if defined( BSP_USE_LESS_MEMORY )
	dleafs = (dleaf_t*)malloc( g_pBSPHeader->lumps[LUMP_LEAFS].filelen );
#endif

	switch ( LumpVersion( LUMP_LEAFS ) )
	{
	case 0:
		{
			g_Lumps.bLumpParsed[LUMP_LEAFS] = true;
			int length = g_pBSPHeader->lumps[LUMP_LEAFS].filelen;
			int size = sizeof( dleaf_version_0_t );
			if ( length % size )
			{
				Error( "odd size for LUMP_LEAFS\n" );
			}
			int count = length / size;

			void *pSrcBase = ( ( byte * )g_pBSPHeader + g_pBSPHeader->lumps[LUMP_LEAFS].fileofs );
			dleaf_version_0_t *pSrc = (dleaf_version_0_t *)pSrcBase;
			dleaf_t *pDst = dleafs;

			// version 0 predates HDR, build the LDR
			g_LeafAmbientLightingLDR.SetCount( count );
			g_LeafAmbientIndexLDR.SetCount( count );

			dleafambientlighting_t *pDstLeafAmbientLighting = &g_LeafAmbientLightingLDR[0];
			for ( int i = 0; i < count; i++ )
			{
				g_LeafAmbientIndexLDR[i].ambientSampleCount = 1;
				g_LeafAmbientIndexLDR[i].firstAmbientSample = i;
		
				if ( g_bSwapOnLoad )
				{
					g_Swap.SwapFieldsToTargetEndian( pSrc );
				}
				// pDst is a subset of pSrc;
				*pDst = *( ( dleaf_t * )( void * )pSrc );
				pDstLeafAmbientLighting->cube = pSrc->m_AmbientLighting;
				pDstLeafAmbientLighting->x = pDstLeafAmbientLighting->y = pDstLeafAmbientLighting->z = pDstLeafAmbientLighting->pad = 0;
				pDst++;
				pSrc++;
				pDstLeafAmbientLighting++;
			}
			return count;
		}

	case 1:
		return CopyLump( LUMP_LEAFS, dleafs );

	default:
		Assert( 0 );
		Error( "Unknown LUMP_LEAFS version\n" );
		return 0;
	}
}

void LoadLeafAmbientLighting( int numLeafs )
{
	if ( LumpVersion( LUMP_LEAFS ) == 0 )
	{
		// an older leaf version already built the LDR ambient lighting on load
		return;
	}

	// old BSP with ambient, or new BSP with no lighting, convert ambient light to new format or create dummy ambient
	if ( !HasLump( LUMP_LEAF_AMBIENT_INDEX ) )
	{
		// a bunch of legacy maps, have these lumps with garbage versions
		// expect them to be NOT the current version
		if ( HasLump(LUMP_LEAF_AMBIENT_LIGHTING) )
		{
			Assert( LumpVersion( LUMP_LEAF_AMBIENT_LIGHTING ) != LUMP_LEAF_AMBIENT_LIGHTING_VERSION );
		}
		if ( HasLump(LUMP_LEAF_AMBIENT_LIGHTING_HDR) )
		{
			Assert( LumpVersion( LUMP_LEAF_AMBIENT_LIGHTING_HDR ) != LUMP_LEAF_AMBIENT_LIGHTING_VERSION );
		}

		void *pSrcBase = ( ( byte * )g_pBSPHeader + g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].fileofs );
		CompressedLightCube *pSrc = NULL;
		if ( HasLump( LUMP_LEAF_AMBIENT_LIGHTING ) )
		{
			pSrc = (CompressedLightCube*)pSrcBase;
		}
		g_LeafAmbientIndexLDR.SetCount( numLeafs );
		g_LeafAmbientLightingLDR.SetCount( numLeafs );

		void *pSrcBaseHDR = ( ( byte * )g_pBSPHeader + g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING_HDR].fileofs );
		CompressedLightCube *pSrcHDR = NULL;
		if ( HasLump( LUMP_LEAF_AMBIENT_LIGHTING_HDR ) )
		{
			pSrcHDR = (CompressedLightCube*)pSrcBaseHDR;
		}
		g_LeafAmbientIndexHDR.SetCount( numLeafs );
		g_LeafAmbientLightingHDR.SetCount( numLeafs );

		for ( int i = 0; i < numLeafs; i++ )
		{
			g_LeafAmbientIndexLDR[i].ambientSampleCount = 1;
			g_LeafAmbientIndexLDR[i].firstAmbientSample = i;
			g_LeafAmbientIndexHDR[i].ambientSampleCount = 1;
			g_LeafAmbientIndexHDR[i].firstAmbientSample = i;

			Q_memset( &g_LeafAmbientLightingLDR[i], 0, sizeof(g_LeafAmbientLightingLDR[i]) );
			Q_memset( &g_LeafAmbientLightingHDR[i], 0, sizeof(g_LeafAmbientLightingHDR[i]) );

			if ( pSrc )
			{
				if ( g_bSwapOnLoad )
				{
					g_Swap.SwapFieldsToTargetEndian( &pSrc[i] );
				}
				g_LeafAmbientLightingLDR[i].cube = pSrc[i];
			}
			if ( pSrcHDR )
			{
				if ( g_bSwapOnLoad )
				{
					g_Swap.SwapFieldsToTargetEndian( &pSrcHDR[i] );
				}
				g_LeafAmbientLightingHDR[i].cube = pSrcHDR[i];
			}
		}

		g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_LIGHTING] = true;
		g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_INDEX] = true;
		g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_LIGHTING_HDR] = true;
		g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_INDEX_HDR] = true;
	}
	else
	{
		CopyOptionalLump( LUMP_LEAF_AMBIENT_LIGHTING, g_LeafAmbientLightingLDR );
		CopyOptionalLump( LUMP_LEAF_AMBIENT_INDEX, g_LeafAmbientIndexLDR );
		CopyOptionalLump( LUMP_LEAF_AMBIENT_LIGHTING_HDR, g_LeafAmbientLightingHDR );
		CopyOptionalLump( LUMP_LEAF_AMBIENT_INDEX_HDR, g_LeafAmbientIndexHDR );
	}
}

void ValidateHeader( const char *filename, const dheader_t *pHeader )
{
	if ( pHeader->ident != IDBSPHEADER )
	{
		Error ("%s is not a IBSP file", filename);
	}
	if ( pHeader->version < MINBSPVERSION || pHeader->version > BSPVERSION )
	{
		Error ("%s is version %i, not %i", filename, pHeader->version, BSPVERSION);
	}
}

//-----------------------------------------------------------------------------
//	Low level BSP opener for external parsing. Parses headers, but nothing else.
//	You must close the BSP, via CloseBSPFile().
//-----------------------------------------------------------------------------
void OpenBSPFile( const char *filename )
{
	Lumps_Init();

	// load the file header
	LoadFile( filename, (void **)&g_pBSPHeader );

	if ( g_bSwapOnLoad )
	{
		g_Swap.ActivateByteSwapping( true );
		g_Swap.SwapFieldsToTargetEndian( g_pBSPHeader );
	}

	ValidateHeader( filename, g_pBSPHeader );

	g_MapRevision = g_pBSPHeader->mapRevision;
}

//-----------------------------------------------------------------------------
//	CloseBSPFile
//-----------------------------------------------------------------------------
void CloseBSPFile( void )
{
	free( g_pBSPHeader );
	g_pBSPHeader = NULL;
}

//-----------------------------------------------------------------------------
//	LoadBSPFile
//-----------------------------------------------------------------------------
void LoadBSPFile( const char *filename )
{
	OpenBSPFile( filename );

	nummodels = CopyLump( LUMP_MODELS, dmodels );
	numvertexes = CopyLump( LUMP_VERTEXES, dvertexes );
	numplanes = CopyLump( LUMP_PLANES, dplanes );
	numleafs = LoadLeafs();
	numnodes = CopyLump( LUMP_NODES, dnodes );
	CopyLump( LUMP_TEXINFO, texinfo );
	numtexdata = CopyLump( LUMP_TEXDATA, dtexdata );
    
	CopyLump( LUMP_DISPINFO, g_dispinfo );
    CopyLump( LUMP_DISP_VERTS, g_DispVerts );
	CopyLump( LUMP_DISP_TRIS, g_DispTris );
    CopyLump( FIELD_CHARACTER, LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, g_DispLightmapSamplePositions );
	CopyLump( LUMP_FACE_MACRO_TEXTURE_INFO, g_FaceMacroTextureInfos );
	
	numfaces = CopyLump(LUMP_FACES, dfaces, LUMP_FACES_VERSION);
	if ( HasLump( LUMP_FACES_HDR ) )
		numfaces_hdr = CopyLump( LUMP_FACES_HDR, dfaces_hdr, LUMP_FACES_VERSION );
	else
		numfaces_hdr = 0;

	CopyOptionalLump( LUMP_FACEIDS, dfaceids );


	g_numprimitives = CopyLump( LUMP_PRIMITIVES, g_primitives );
	g_numprimverts = CopyLump( LUMP_PRIMVERTS, g_primverts );
	g_numprimindices = CopyLump( FIELD_SHORT, LUMP_PRIMINDICES, g_primindices );
    numorigfaces = CopyLump( LUMP_ORIGINALFACES, dorigfaces );   // original faces
	numleaffaces = CopyLump( FIELD_SHORT, LUMP_LEAFFACES, dleaffaces );
	numleafbrushes = CopyLump( FIELD_SHORT, LUMP_LEAFBRUSHES, dleafbrushes );
	numsurfedges = CopyLump( FIELD_INTEGER, LUMP_SURFEDGES, dsurfedges );
	numedges = CopyLump( LUMP_EDGES, dedges );
	numbrushes = CopyLump( LUMP_BRUSHES, dbrushes );
	numbrushsides = CopyLump( LUMP_BRUSHSIDES, dbrushsides );
	numareas = CopyLump( LUMP_AREAS, dareas );
	numareaportals = CopyLump( LUMP_AREAPORTALS, dareaportals );

	visdatasize = CopyLump ( FIELD_CHARACTER, LUMP_VISIBILITY, dvisdata );
	CopyOptionalLump( FIELD_CHARACTER, LUMP_LIGHTING, dlightdataLDR, LUMP_LIGHTING_VERSION );
	CopyOptionalLump( FIELD_CHARACTER, LUMP_LIGHTING_HDR, dlightdataHDR, LUMP_LIGHTING_VERSION );

	LoadLeafAmbientLighting( numleafs );

	CopyLump( FIELD_CHARACTER, LUMP_ENTITIES, dentdata );
	numworldlightsLDR = CopyLump( LUMP_WORLDLIGHTS, dworldlightsLDR );
	numworldlightsHDR = CopyLump( LUMP_WORLDLIGHTS_HDR, dworldlightsHDR );
	
	numleafwaterdata = CopyLump( LUMP_LEAFWATERDATA, dleafwaterdata );
	g_PhysCollideSize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PHYSCOLLIDE, (void**)&g_pPhysCollide );
	g_PhysDispSize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PHYSDISP, (void**)&g_pPhysDisp );

	g_numvertnormals = CopyLump( FIELD_VECTOR, LUMP_VERTNORMALS, (float*)g_vertnormals );
	g_numvertnormalindices = CopyLump( FIELD_SHORT, LUMP_VERTNORMALINDICES, g_vertnormalindices );

	g_nClipPortalVerts = CopyLump( FIELD_VECTOR, LUMP_CLIPPORTALVERTS, (float*)g_ClipPortalVerts );
	g_nCubemapSamples = CopyLump( LUMP_CUBEMAPS, g_CubemapSamples );	

	CopyLump( FIELD_CHARACTER, LUMP_TEXDATA_STRING_DATA, g_TexDataStringData );
	CopyLump( FIELD_INTEGER, LUMP_TEXDATA_STRING_TABLE, g_TexDataStringTable );

	// It's assumed by other code that the data lump is filled with C strings. We need to make sure that 
	// the buffer as a whole ends with a '\0'.
	if ( g_TexDataStringData.Count() > 0 && g_TexDataStringData.Tail() != 0 )
		Error( "Cannot load corrupted bsp file %s", filename );

	g_nOverlayCount = CopyLump( LUMP_OVERLAYS, g_Overlays );
	g_nWaterOverlayCount = CopyLump( LUMP_WATEROVERLAYS, g_WaterOverlays );
	CopyLump( LUMP_OVERLAY_FADES, g_OverlayFades );
	
	dflagslump_t flags_lump;
	
	if ( HasLump( LUMP_MAP_FLAGS ) )
		CopyLump ( LUMP_MAP_FLAGS, &flags_lump );
	else
		memset( &flags_lump, 0, sizeof( flags_lump ) );			// default flags to 0

	g_LevelFlags = flags_lump.m_LevelFlags;

	LoadOcclusionLump();

	CopyLump( FIELD_SHORT, LUMP_LEAFMINDISTTOWATER, g_LeafMinDistToWater );

	/*
	int junk;
	for( junk = 0; junk < g_nBSPStringTable; junk++ )
	{
		Msg( "stringtable %d", ( int )junk );
		Msg( " %d:",  ( int )g_BSPStringTable[junk] );
		puts( &g_BSPStringData[g_BSPStringTable[junk]] );
		puts( "\n" );
	}
	*/
		
	// Load PAK file lump into appropriate data structure
	byte *pakbuffer = NULL;
	int paksize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PAKFILE, ( void ** )&pakbuffer );
	if ( paksize > 0 )
	{
		GetPakFile()->ActivateByteSwapping( IsX360() );
		GetPakFile()->ParseFromBuffer( pakbuffer, paksize );
	}
	else
	{
		GetPakFile()->Reset();
	}

	free( pakbuffer );

	g_GameLumps.ParseGameLump( g_pBSPHeader );

	// NOTE: Do NOT call CopyLump after Lumps_Parse() it parses all un-Copied lumps
	// parse any additional lumps
	Lumps_Parse();

	// everything has been copied out
	CloseBSPFile();

	g_Swap.ActivateByteSwapping( false );
}

//-----------------------------------------------------------------------------
// Reset any state.
//-----------------------------------------------------------------------------
void UnloadBSPFile()
{
	nummodels = 0;
	numvertexes = 0;
	numplanes = 0;

	numleafs = 0;
#if defined( BSP_USE_LESS_MEMORY )
	if ( dleafs )
	{ 
		free( dleafs );
		dleafs = NULL;
	}
#endif

	numnodes = 0;
	texinfo.Purge();
	numtexdata = 0;

	g_dispinfo.Purge();
	g_DispVerts.Purge();
	g_DispTris.Purge();

	g_DispLightmapSamplePositions.Purge();
	g_FaceMacroTextureInfos.Purge();

	numfaces = 0;
	numfaces_hdr = 0;

	dfaceids.Purge();


	g_numprimitives = 0;
	g_numprimverts = 0;
	g_numprimindices = 0;
	numorigfaces = 0;
	numleaffaces = 0;
	numleafbrushes = 0;
	numsurfedges = 0;
	numedges = 0;
	numbrushes = 0;
	numbrushsides = 0;
	numareas = 0;
	numareaportals = 0;

	visdatasize = 0;
	dlightdataLDR.Purge();
	dlightdataHDR.Purge();

	g_LeafAmbientLightingLDR.Purge();
	g_LeafAmbientLightingHDR.Purge();
	g_LeafAmbientIndexHDR.Purge();
	g_LeafAmbientIndexLDR.Purge();

	dentdata.Purge();
	numworldlightsLDR = 0;
	numworldlightsHDR = 0;

	numleafwaterdata = 0;

	if ( g_pPhysCollide )
	{
		free( g_pPhysCollide );
		g_pPhysCollide = NULL;
	}
	g_PhysCollideSize = 0;

	if ( g_pPhysDisp )
	{
		free( g_pPhysDisp );
		g_pPhysDisp = NULL;
	}
	g_PhysDispSize = 0;

	g_numvertnormals = 0;
	g_numvertnormalindices = 0;

	g_nClipPortalVerts = 0;
	g_nCubemapSamples = 0;	

	g_TexDataStringData.Purge();
	g_TexDataStringTable.Purge();

	g_nOverlayCount = 0;
	g_nWaterOverlayCount = 0;

	g_LevelFlags = 0;

	g_OccluderData.Purge();
	g_OccluderPolyData.Purge();
	g_OccluderVertexIndices.Purge();

	g_GameLumps.DestroyAllGameLumps();

	for ( int i = 0; i < HEADER_LUMPS; i++ )
	{
		if ( g_Lumps.pLumps[i] )
		{
			free( g_Lumps.pLumps[i] );
			g_Lumps.pLumps[i] = NULL;
		}
	}

	ReleasePakFileLumps();
}

//-----------------------------------------------------------------------------
//	LoadBSPFileFilesystemOnly
//-----------------------------------------------------------------------------
void LoadBSPFile_FileSystemOnly( const char *filename )
{
	Lumps_Init();

	//
	// load the file header
	//
	LoadFile( filename, (void **)&g_pBSPHeader );

	ValidateHeader( filename, g_pBSPHeader );

	// Load PAK file lump into appropriate data structure
	byte *pakbuffer = NULL;
	int paksize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PAKFILE, ( void ** )&pakbuffer, 1 );
	if ( paksize > 0 )
	{
		GetPakFile()->ParseFromBuffer( pakbuffer, paksize );
	}
	else
	{
		GetPakFile()->Reset();
	}

	free( pakbuffer );

	// everything has been copied out
	free( g_pBSPHeader );
	g_pBSPHeader = NULL;
}

void ExtractZipFileFromBSP( char *pBSPFileName, char *pZipFileName )
{
	Lumps_Init();

	//
	// load the file header
	//
	LoadFile( pBSPFileName, (void **)&g_pBSPHeader);

	ValidateHeader( pBSPFileName, g_pBSPHeader );

	byte *pakbuffer = NULL;
	int paksize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PAKFILE, ( void ** )&pakbuffer );
	if ( paksize > 0 )
	{
		FILE *fp;
		fp = fopen( pZipFileName, "wb" );
		if( !fp )
		{
			fprintf( stderr, "can't open %s\n", pZipFileName );
			return;
		}

		fwrite( pakbuffer, paksize, 1, fp );
		fclose( fp );
	}
	else
	{		
		fprintf( stderr, "zip file is zero length!\n" );
	}
}

/*
=============
LoadBSPFileTexinfo

Only loads the texinfo lump, so qdata can scan for textures
=============
*/
void LoadBSPFileTexinfo( const char *filename )
{
	FILE		*f;
	int		length, ofs;

	g_pBSPHeader = (dheader_t*)malloc( sizeof(dheader_t) );

	f = fopen( filename, "rb" );
	fread( g_pBSPHeader, sizeof(dheader_t), 1, f);

	ValidateHeader( filename, g_pBSPHeader );

	length = g_pBSPHeader->lumps[LUMP_TEXINFO].filelen;
	ofs = g_pBSPHeader->lumps[LUMP_TEXINFO].fileofs;

	int nCount = length / sizeof(texinfo_t);

	texinfo.Purge();
	texinfo.AddMultipleToTail( nCount );

	fseek( f, ofs, SEEK_SET );
	fread( texinfo.Base(), length, 1, f );
	fclose( f );

	// everything has been copied out
	free( g_pBSPHeader );
	g_pBSPHeader = NULL;
}

static void AddLumpInternal( int lumpnum, void *data, int len, int version )
{
	lump_t *lump;

	g_Lumps.size[lumpnum] = 0;	// mark it written

	lump = &g_pBSPHeader->lumps[lumpnum];

	lump->fileofs = g_pFileSystem->Tell( g_hBSPFile );
	lump->filelen = len;
	lump->version = version;
	lump->uncompressedSize = 0;

	SafeWrite( g_hBSPFile, data, len );

	// pad out to the next dword
	AlignFilePosition( g_hBSPFile, 4 );
}

template< class T >
static void SwapInPlace( T *pData, int count )
{
	if ( !pData )
		return;

	// use the datadesc to swap the fields in place
	g_Swap.SwapFieldsToTargetEndian<T>( (T*)pData, pData, count );
}

template< class T >
static void SwapInPlace( int fieldType, T *pData, int count )
{
	if ( !pData )
		return;

	// swap the data in place
	g_Swap.SwapBufferToTargetEndian<T>( (T*)pData, (T*)pData, count );
}

//-----------------------------------------------------------------------------
//	Add raw data chunk to file (not a lump)
//-----------------------------------------------------------------------------
template< class T >
static void WriteData( int fieldType, T *pData, int count )
{
	if ( g_bSwapOnWrite )
	{
		SwapInPlace( fieldType, pData, count );
	}
	SafeWrite( g_hBSPFile, pData, count * sizeof(T) );
}

template< class T >
static void WriteData( T *pData, int count )
{
	if ( g_bSwapOnWrite )
	{
		SwapInPlace( pData, count );
	}
	SafeWrite( g_hBSPFile, pData, count * sizeof(T) );
}

//-----------------------------------------------------------------------------
//	Add Lump of object types with datadescs
//-----------------------------------------------------------------------------
template< class T >
static void AddLump( int lumpnum, T *pData, int count, int version )
{
	AddLumpInternal( lumpnum, pData, count * sizeof(T), version );
}

template< class T >
static void AddLump( int lumpnum, CUtlVector<T> &data, int version )
{
	AddLumpInternal( lumpnum, data.Base(), data.Count() * sizeof(T), version );
}

/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void WriteBSPFile( const char *filename, char *pUnused )
{		
	if ( texinfo.Count() > MAX_MAP_TEXINFO )
	{
		Error( "Map has too many texinfos (has %d, can have at most %d)\n", texinfo.Count(), MAX_MAP_TEXINFO );
		return;
	}

	dheader_t outHeader;
	g_pBSPHeader = &outHeader;
	memset( g_pBSPHeader, 0, sizeof( dheader_t ) );

	g_pBSPHeader->ident = IDBSPHEADER;
	g_pBSPHeader->version = BSPVERSION;
	g_pBSPHeader->mapRevision = g_MapRevision;

	g_hBSPFile = SafeOpenWrite( filename );
	WriteData( g_pBSPHeader );	// overwritten later

	AddLump( LUMP_PLANES, dplanes, numplanes );
	AddLump( LUMP_LEAFS, dleafs, numleafs, LUMP_LEAFS_VERSION );
	AddLump( LUMP_LEAF_AMBIENT_LIGHTING, g_LeafAmbientLightingLDR, LUMP_LEAF_AMBIENT_LIGHTING_VERSION );
	AddLump( LUMP_LEAF_AMBIENT_INDEX, g_LeafAmbientIndexLDR );
	AddLump( LUMP_LEAF_AMBIENT_INDEX_HDR, g_LeafAmbientIndexHDR );
	AddLump( LUMP_LEAF_AMBIENT_LIGHTING_HDR, g_LeafAmbientLightingHDR, LUMP_LEAF_AMBIENT_LIGHTING_VERSION );

	AddLump( LUMP_VERTEXES, dvertexes, numvertexes );
	AddLump( LUMP_NODES, dnodes, numnodes );
	AddLump( LUMP_TEXINFO, texinfo );
	AddLump( LUMP_TEXDATA, dtexdata, numtexdata );    

    AddLump( LUMP_DISPINFO, g_dispinfo );
    AddLump( LUMP_DISP_VERTS, g_DispVerts );
	AddLump( LUMP_DISP_TRIS, g_DispTris );
    AddLump( LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS, g_DispLightmapSamplePositions );
	AddLump( LUMP_FACE_MACRO_TEXTURE_INFO, g_FaceMacroTextureInfos );
 
	AddLump( LUMP_PRIMITIVES, g_primitives, g_numprimitives );
	AddLump( LUMP_PRIMVERTS, g_primverts, g_numprimverts );
	AddLump( LUMP_PRIMINDICES, g_primindices, g_numprimindices );
    AddLump( LUMP_FACES, dfaces, numfaces, LUMP_FACES_VERSION );
    if (numfaces_hdr)
		AddLump( LUMP_FACES_HDR, dfaces_hdr, numfaces_hdr, LUMP_FACES_VERSION );
	AddLump ( LUMP_FACEIDS, dfaceids, numfaceids );


	AddLump( LUMP_ORIGINALFACES, dorigfaces, numorigfaces );     // original faces lump
	AddLump( LUMP_BRUSHES, dbrushes, numbrushes );
	AddLump( LUMP_BRUSHSIDES, dbrushsides, numbrushsides );
	AddLump( LUMP_LEAFFACES, dleaffaces, numleaffaces );
	AddLump( LUMP_LEAFBRUSHES, dleafbrushes, numleafbrushes );
	AddLump( LUMP_SURFEDGES, dsurfedges, numsurfedges );
	AddLump( LUMP_EDGES, dedges, numedges );
	AddLump( LUMP_MODELS, dmodels, nummodels );
	AddLump( LUMP_AREAS, dareas, numareas );
	AddLump( LUMP_AREAPORTALS, dareaportals, numareaportals );

	AddLump( LUMP_LIGHTING, dlightdataLDR, LUMP_LIGHTING_VERSION );
	AddLump( LUMP_LIGHTING_HDR, dlightdataHDR, LUMP_LIGHTING_VERSION );
	AddLump( LUMP_VISIBILITY, dvisdata, visdatasize );
	AddLump( LUMP_ENTITIES, dentdata );
	AddLump( LUMP_WORLDLIGHTS, dworldlightsLDR, numworldlightsLDR );
	AddLump( LUMP_WORLDLIGHTS_HDR, dworldlightsHDR, numworldlightsHDR );
	AddLump( LUMP_LEAFWATERDATA, dleafwaterdata, numleafwaterdata );

	AddOcclusionLump();

	dflagslump_t flags_lump;
	flags_lump.m_LevelFlags = g_LevelFlags;
	AddLump( LUMP_MAP_FLAGS, &flags_lump, 1 );

	// NOTE: This is just for debugging, so it is disabled in release maps
#if 0
	// add the vis portals to the BSP for visualization
	AddLump( LUMP_PORTALS, dportals, numportals );
	AddLump( LUMP_CLUSTERS, dclusters, numclusters );
	AddLump( LUMP_PORTALVERTS, dportalverts, numportalverts );
	AddLump( LUMP_CLUSTERPORTALS, dclusterportals, numclusterportals );
#endif

	AddLump( LUMP_CLIPPORTALVERTS, (float*)g_ClipPortalVerts, g_nClipPortalVerts * 3 );
	AddLump( LUMP_CUBEMAPS, g_CubemapSamples, g_nCubemapSamples );
	AddLump( LUMP_TEXDATA_STRING_DATA, g_TexDataStringData );
	AddLump( LUMP_TEXDATA_STRING_TABLE, g_TexDataStringTable );
	AddLump( LUMP_OVERLAYS, g_Overlays, g_nOverlayCount );
	AddLump( LUMP_WATEROVERLAYS, g_WaterOverlays, g_nWaterOverlayCount );
	AddLump( LUMP_OVERLAY_FADES, g_OverlayFades, g_nOverlayCount );

	if ( g_pPhysCollide )
	{
		AddLump( LUMP_PHYSCOLLIDE, g_pPhysCollide, g_PhysCollideSize );
	}

	if ( g_pPhysDisp )
	{
		AddLump ( LUMP_PHYSDISP, g_pPhysDisp, g_PhysDispSize );
	}

	AddLump( LUMP_VERTNORMALS, (float*)g_vertnormals, g_numvertnormals * 3 );
	AddLump( LUMP_VERTNORMALINDICES, g_vertnormalindices, g_numvertnormalindices );

	AddLump( LUMP_LEAFMINDISTTOWATER, g_LeafMinDistToWater, numleafs );

	AddGameLumps();

	// Write pakfile lump to disk
	WritePakFileLump();

	// NOTE: Do NOT call AddLump after Lumps_Write() it writes all un-Added lumps
	// write any additional lumps
	Lumps_Write();

	g_pFileSystem->Seek( g_hBSPFile, 0, FILESYSTEM_SEEK_HEAD );
	WriteData( g_pBSPHeader );
	g_pFileSystem->Close( g_hBSPFile );
}

// Generate the next clear lump filename for the bsp file
bool GenerateNextLumpFileName( const char *bspfilename, char *lumpfilename, int buffsize )
{
	for (int i = 0; i < MAX_LUMPFILES; i++)
	{
		GenerateLumpFileName( bspfilename, lumpfilename, buffsize, i );
	
		if ( !g_pFileSystem->FileExists( lumpfilename ) )
			return true;
	}

	return false;
}

void WriteLumpToFile( char *filename, int lump )
{
	if ( !HasLump(lump) )
		return;

	char lumppre[MAX_PATH];	
	if ( !GenerateNextLumpFileName( filename, lumppre, MAX_PATH ) )
	{
		Warning( "Failed to find valid lump filename for bsp %s.\n", filename );
		return;
	}

	// Open the file
	FileHandle_t lumpfile = g_pFileSystem->Open(lumppre, "wb");
	if ( !lumpfile )
	{
		Error ("Error opening %s! (Check for write enable)\n",filename);
		return;
	}

	int ofs = g_pBSPHeader->lumps[lump].fileofs;
	int length = g_pBSPHeader->lumps[lump].filelen;

	// Write the header
	lumpfileheader_t lumpHeader;
	lumpHeader.lumpID = lump;
	lumpHeader.lumpVersion = LumpVersion(lump);
	lumpHeader.lumpLength = length;
	lumpHeader.mapRevision = LittleLong( g_MapRevision );
	lumpHeader.lumpOffset = sizeof(lumpfileheader_t);	// Lump starts after the header
	SafeWrite (lumpfile, &lumpHeader, sizeof(lumpfileheader_t));

	// Write the lump
	SafeWrite (lumpfile, (byte *)g_pBSPHeader + ofs, length);
}

void	WriteLumpToFile( char *filename, int lump, int nLumpVersion, void *pBuffer, size_t nBufLen )
{
	char lumppre[MAX_PATH];	
	if ( !GenerateNextLumpFileName( filename, lumppre, MAX_PATH ) )
	{
		Warning( "Failed to find valid lump filename for bsp %s.\n", filename );
		return;
	}

	// Open the file
	FileHandle_t lumpfile = g_pFileSystem->Open(lumppre, "wb");
	if ( !lumpfile )
	{
		Error ("Error opening %s! (Check for write enable)\n",filename);
		return;
	}

	// Write the header
	lumpfileheader_t lumpHeader;
	lumpHeader.lumpID = lump;
	lumpHeader.lumpVersion = nLumpVersion;
	lumpHeader.lumpLength = (int)nBufLen;
	lumpHeader.mapRevision = LittleLong( g_MapRevision );
	lumpHeader.lumpOffset = sizeof(lumpfileheader_t);	// Lump starts after the header
	SafeWrite( lumpfile, &lumpHeader, sizeof(lumpfileheader_t));

	// Write the lump
	SafeWrite( lumpfile, pBuffer, (int)nBufLen );

	g_pFileSystem->Close( lumpfile );
}


//============================================================================
#define ENTRIES(a)		(sizeof(a)/sizeof(*(a)))
#define ENTRYSIZE(a)	(sizeof(*(a)))

int ArrayUsage( const char *szItem, int items, int maxitems, int itemsize )
{
	float	percentage = maxitems ? items * 100.0 / maxitems : 0.0;

    Msg("%-17.17s %8i/%-8i %8i/%-8i (%4.1f%%) ", 
		   szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage );
	if ( percentage > 80.0 )
		Msg( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		Msg( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		Msg( "SIZE OVERFLOW!!!\n" );
	else
		Msg( "\n" );
	return items * itemsize;
}

int GlobUsage( const char *szItem, int itemstorage, int maxstorage )
{
	float	percentage = maxstorage ? itemstorage * 100.0 / maxstorage : 0.0;
    Msg("%-17.17s     [variable]    %8i/%-8i (%4.1f%%) ", 
		   szItem, itemstorage, maxstorage, percentage );
	if ( percentage > 80.0 )
		Msg( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		Msg( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		Msg( "SIZE OVERFLOW!!!\n" );
	else
		Msg( "\n" );
	return itemstorage;
}

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void PrintBSPFileSizes (void)
{
	int	totalmemory = 0;

//	if (!num_entities)
//		ParseEntities ();

	Msg("\n");
	Msg( "%-17s %16s %16s %9s \n", "Object names", "Objects/Maxobjs", "Memory / Maxmem", "Fullness" );
	Msg( "%-17s %16s %16s %9s \n",  "------------", "---------------", "---------------", "--------" );

	totalmemory += ArrayUsage( "models",		nummodels,		ENTRIES(dmodels),		ENTRYSIZE(dmodels) );
	totalmemory += ArrayUsage( "brushes",		numbrushes,		ENTRIES(dbrushes),		ENTRYSIZE(dbrushes) );
	totalmemory += ArrayUsage( "brushsides",	numbrushsides,	ENTRIES(dbrushsides),	ENTRYSIZE(dbrushsides) );
	totalmemory += ArrayUsage( "planes",		numplanes,		ENTRIES(dplanes),		ENTRYSIZE(dplanes) );
	totalmemory += ArrayUsage( "vertexes",		numvertexes,	ENTRIES(dvertexes),		ENTRYSIZE(dvertexes) );
	totalmemory += ArrayUsage( "nodes",			numnodes,		ENTRIES(dnodes),		ENTRYSIZE(dnodes) );
	totalmemory += ArrayUsage( "texinfos",		texinfo.Count(),MAX_MAP_TEXINFO,		sizeof(texinfo_t) );
	totalmemory += ArrayUsage( "texdata",		numtexdata,		ENTRIES(dtexdata),		ENTRYSIZE(dtexdata) );
    
	totalmemory += ArrayUsage( "dispinfos",     g_dispinfo.Count(),			0,			sizeof( ddispinfo_t ) );
    totalmemory += ArrayUsage( "disp_verts",	g_DispVerts.Count(),		0,			sizeof( g_DispVerts[0] ) );
    totalmemory += ArrayUsage( "disp_tris",		g_DispTris.Count(),			0,			sizeof( g_DispTris[0] ) );
    totalmemory += ArrayUsage( "disp_lmsamples",g_DispLightmapSamplePositions.Count(),0,sizeof( g_DispLightmapSamplePositions[0] ) );
	
	totalmemory += ArrayUsage( "faces",			numfaces,		ENTRIES(dfaces),		ENTRYSIZE(dfaces) );
	totalmemory += ArrayUsage( "hdr faces",     numfaces_hdr,	ENTRIES(dfaces_hdr),	ENTRYSIZE(dfaces_hdr) );
    totalmemory += ArrayUsage( "origfaces",     numorigfaces,   ENTRIES(dorigfaces),    ENTRYSIZE(dorigfaces) );    // original faces
	totalmemory += ArrayUsage( "leaves",		numleafs,		ENTRIES(dleafs),		ENTRYSIZE(dleafs) );
	totalmemory += ArrayUsage( "leaffaces",		numleaffaces,	ENTRIES(dleaffaces),	ENTRYSIZE(dleaffaces) );
	totalmemory += ArrayUsage( "leafbrushes",	numleafbrushes,	ENTRIES(dleafbrushes),	ENTRYSIZE(dleafbrushes) );
	totalmemory += ArrayUsage( "areas",	numareas,	ENTRIES(dareas),	ENTRYSIZE(dareas) );
	totalmemory += ArrayUsage( "surfedges",		numsurfedges,	ENTRIES(dsurfedges),	ENTRYSIZE(dsurfedges) );
	totalmemory += ArrayUsage( "edges",			numedges,		ENTRIES(dedges),		ENTRYSIZE(dedges) );
	totalmemory += ArrayUsage( "LDR worldlights",	numworldlightsLDR,	ENTRIES(dworldlightsLDR),	ENTRYSIZE(dworldlightsLDR) );
	totalmemory += ArrayUsage( "HDR worldlights",	numworldlightsHDR,	ENTRIES(dworldlightsHDR),	ENTRYSIZE(dworldlightsHDR) );

	totalmemory += ArrayUsage( "leafwaterdata",	numleafwaterdata,ENTRIES(dleafwaterdata),	ENTRYSIZE(dleafwaterdata) );
	totalmemory += ArrayUsage( "waterstrips",	g_numprimitives,ENTRIES(g_primitives),	ENTRYSIZE(g_primitives) );
	totalmemory += ArrayUsage( "waterverts",	g_numprimverts,	ENTRIES(g_primverts),	ENTRYSIZE(g_primverts) );
	totalmemory += ArrayUsage( "waterindices",	g_numprimindices,ENTRIES(g_primindices),ENTRYSIZE(g_primindices) );
	totalmemory += ArrayUsage( "cubemapsamples", g_nCubemapSamples,ENTRIES(g_CubemapSamples),ENTRYSIZE(g_CubemapSamples) );
	totalmemory += ArrayUsage( "overlays",      g_nOverlayCount, ENTRIES(g_Overlays),   ENTRYSIZE(g_Overlays) );
	
	totalmemory += GlobUsage( "LDR lightdata",		dlightdataLDR.Count(),	0 );
	totalmemory += GlobUsage( "HDR lightdata",	dlightdataHDR.Count(),	0 );
	totalmemory += GlobUsage( "visdata",		visdatasize,	sizeof(dvisdata) );
	totalmemory += GlobUsage( "entdata",		dentdata.Count(), 384*1024 );	// goal is <384K

	totalmemory += ArrayUsage( "LDR ambient table", g_LeafAmbientIndexLDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientIndexLDR[0] ) );
	totalmemory += ArrayUsage( "HDR ambient table", g_LeafAmbientIndexHDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientIndexHDR[0] ) );
	totalmemory += ArrayUsage( "LDR leaf ambient lighting", g_LeafAmbientLightingLDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientLightingLDR[0] ) );
	totalmemory += ArrayUsage( "HDR leaf ambient lighting", g_LeafAmbientLightingHDR.Count(), MAX_MAP_LEAFS, sizeof( g_LeafAmbientLightingHDR[0] ) );

	totalmemory += ArrayUsage( "occluders",     g_OccluderData.Count(),	0, sizeof( g_OccluderData[0] ) );
    totalmemory += ArrayUsage( "occluder polygons",	g_OccluderPolyData.Count(),	0, sizeof( g_OccluderPolyData[0] ) );
    totalmemory += ArrayUsage( "occluder vert ind",g_OccluderVertexIndices.Count(),0, sizeof( g_OccluderVertexIndices[0] ) );

	GameLumpHandle_t h = g_GameLumps.GetGameLumpHandle( GAMELUMP_DETAIL_PROPS );
	if (h != g_GameLumps.InvalidGameLump())
		totalmemory += GlobUsage( "detail props",	1,	g_GameLumps.GameLumpSize(h) );
	h = g_GameLumps.GetGameLumpHandle( GAMELUMP_DETAIL_PROP_LIGHTING );
	if (h != g_GameLumps.InvalidGameLump())
		totalmemory += GlobUsage( "dtl prp lght",	1,	g_GameLumps.GameLumpSize(h) );
	h = g_GameLumps.GetGameLumpHandle( GAMELUMP_DETAIL_PROP_LIGHTING_HDR );
	if (h != g_GameLumps.InvalidGameLump())
		totalmemory += GlobUsage( "HDR dtl prp lght",	1,	g_GameLumps.GameLumpSize(h) );
	h = g_GameLumps.GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	if (h != g_GameLumps.InvalidGameLump())
		totalmemory += GlobUsage( "static props",	1,	g_GameLumps.GameLumpSize(h) );

	totalmemory += GlobUsage( "pakfile",		GetPakFile()->EstimateSize(), 0 );
	// HACKHACK: Set physics limit at 4MB, in reality this is totally dynamic
	totalmemory += GlobUsage( "physics",		g_PhysCollideSize, 4*1024*1024 );
	totalmemory += GlobUsage( "physics terrain",		g_PhysDispSize, 1*1024*1024 );

	Msg( "\nLevel flags = %x\n", g_LevelFlags );

	Msg( "\n" );

	int triangleCount = 0;

	for ( int i = 0; i < numfaces; i++ )
	{
		// face tris = numedges - 2
		triangleCount += dfaces[i].numedges - 2;
	}
	Msg("Total triangle count: %d\n", triangleCount );

	// UNDONE: 
	// areaportals, portals, texdata, clusters, worldlights, portalverts
}

/*
=============
PrintBSPPackDirectory

Dumps a list of files stored in the bsp pack.
=============
*/
void PrintBSPPackDirectory( void )
{
	GetPakFile()->PrintDirectory();	
}


//============================================

int			num_entities;
entity_t	entities[MAX_MAP_ENTITIES];

void StripTrailing (char *e)
{
	char	*s;

	s = e + strlen(e)-1;
	while (s >= e && *s <= 32)
	{
		*s = 0;
		s--;
	}
}

/*
=================
ParseEpair
=================
*/
epair_t *ParseEpair (void)
{
	epair_t	*e;

	e = (epair_t*)malloc (sizeof(epair_t));
	memset (e, 0, sizeof(epair_t));
	
	if (strlen(token) >= MAX_KEY-1)
		Error ("ParseEpar: token too long");
	e->key = copystring(token);

	GetToken (false);
	if (strlen(token) >= MAX_VALUE-1)
		Error ("ParseEpar: token too long");
	e->value = copystring(token);

	// strip trailing spaces
	StripTrailing (e->key);
	StripTrailing (e->value);

	return e;
}


/*
================
ParseEntity
================
*/
qboolean	ParseEntity (void)
{
	epair_t		*e;
	entity_t	*mapent;

	if (!GetToken (true))
		return false;

	if (Q_stricmp (token, "{") )
		Error ("ParseEntity: { not found");
	
	if (num_entities == MAX_MAP_ENTITIES)
		Error ("num_entities == MAX_MAP_ENTITIES");

	mapent = &entities[num_entities];
	num_entities++;

	do
	{
		if (!GetToken (true))
			Error ("ParseEntity: EOF without closing brace");
		if (!Q_stricmp (token, "}") )
			break;
		e = ParseEpair ();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while (1);
	
	return true;
}

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void ParseEntities (void)
{
	num_entities = 0;
	ParseFromMemory (dentdata.Base(), dentdata.Count());

	while (ParseEntity ())
	{
	}	
}


/*
================
UnparseEntities

Generates the dentdata string from all the entities
================
*/
void UnparseEntities (void)
{
	epair_t	*ep;
	char	line[2048 + 16];
	int		i;
	char	key[1024], value[1024];

	CUtlBuffer buffer( 0, 0, CUtlBuffer::TEXT_BUFFER );
	buffer.EnsureCapacity( 256 * 1024 );
	
	for (i=0 ; i<num_entities ; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;	// ent got removed
		
		buffer.PutString( "{\n" );
				
		for (ep = entities[i].epairs ; ep ; ep=ep->next)
		{
			strcpy (key, ep->key);
			StripTrailing (key);
			strcpy (value, ep->value);
			StripTrailing (value);
				
			V_sprintf_safe(line, "\"%s\" \"%s\"\n", key, value);
			buffer.PutString( line );
		}
		buffer.PutString("}\n");
	}
	int entdatasize = buffer.TellPut()+1;

	dentdata.SetSize( entdatasize );
	memcpy( dentdata.Base(), buffer.Base(), entdatasize-1 );
	dentdata[entdatasize-1] = 0;
}

void PrintEntity (entity_t *ent)
{
	epair_t	*ep;
	
	Msg ("------- entity %p -------\n", ent);
	for (ep=ent->epairs ; ep ; ep=ep->next)
	{
		Msg ("%s = %s\n", ep->key, ep->value);
	}

}

void SetKeyValue(entity_t *ent, const char *key, const char *value)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!Q_stricmp (ep->key, key) )
		{
			free (ep->value);
			ep->value = copystring(value);
			return;
		}
	ep = (epair_t*)malloc (sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = copystring(key);
	ep->value = copystring(value);
}

char 	*ValueForKey (entity_t *ent, char *key)
{
	for (epair_t *ep=ent->epairs ; ep ; ep=ep->next)
		if (!Q_stricmp (ep->key, key) )
			return ep->value;
	return "";
}

vec_t	FloatForKey (entity_t *ent, char *key)
{
	char *k = ValueForKey (ent, key);
	return atof(k);
}

vec_t	FloatForKeyWithDefault (entity_t *ent, char *key, float default_value)
{
	for (epair_t *ep=ent->epairs ; ep ; ep=ep->next)
		if (!Q_stricmp (ep->key, key) )
			return atof( ep->value );
	return default_value;
}



int		IntForKey (entity_t *ent, char *key)
{
	char *k = ValueForKey (ent, key);
	return atol(k);
}

int		IntForKeyWithDefault(entity_t *ent, char *key, int nDefault )
{
	char *k = ValueForKey (ent, key);
	if ( !k[0] )
		return nDefault;
	return atol(k);
}

void 	GetVectorForKey (entity_t *ent, char *key, Vector& vec)
{

	char *k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	double	v1, v2, v3;
	v1 = v2 = v3 = 0;
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
}

void 	GetVector2DForKey (entity_t *ent, char *key, Vector2D& vec)
{
	double	v1, v2;

	char *k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = 0;
	sscanf (k, "%lf %lf", &v1, &v2);
	vec[0] = v1;
	vec[1] = v2;
}

void 	GetAnglesForKey (entity_t *ent, char *key, QAngle& angle)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	angle[0] = v1;
	angle[1] = v2;
	angle[2] = v3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void BuildFaceCalcWindingData( dface_t *pFace, int *points )
{
	for( int i = 0; i < pFace->numedges; i++ )
	{
		int eIndex = dsurfedges[pFace->firstedge+i];
		if( eIndex < 0 )
		{
			points[i] = dedges[-eIndex].v[1];
		}
		else
		{
			points[i] = dedges[eIndex].v[0];
		}
	}
}


void TriStripToTriList( 
	unsigned short const *pTriStripIndices,
	int nTriStripIndices,
	unsigned short **pTriListIndices,
	int *pnTriListIndices )
{
	int nMaxTriListIndices = (nTriStripIndices - 2) * 3;
	*pTriListIndices = new unsigned short[ nMaxTriListIndices ];
	*pnTriListIndices = 0;

	for( int i=0; i < nTriStripIndices - 2; i++ )
	{
		if( pTriStripIndices[i]   == pTriStripIndices[i+1] || 
			pTriStripIndices[i]   == pTriStripIndices[i+2] ||
			pTriStripIndices[i+1] == pTriStripIndices[i+2] )
		{
		}
		else
		{
			// Flip odd numbered tris..
			if( i & 1 )
			{
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+2];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+1];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i];
			}
			else
			{
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+1];
				(*pTriListIndices)[(*pnTriListIndices)++] = pTriStripIndices[i+2];
			}
		}
	}
}


void CalcTextureCoordsAtPoints(
	float const texelsPerWorldUnits[2][4],
	int const subtractOffset[2],
	Vector const *pPoints,
	int const nPoints,
	Vector2D *pCoords )
{
	for( int i=0; i < nPoints; i++ )
	{
		for( int iCoord=0; iCoord < 2; iCoord++ )
		{
			float *pDestCoord = &pCoords[i][iCoord];

			*pDestCoord = 0;
			for( int iDot=0; iDot < 3; iDot++ )
				*pDestCoord += pPoints[i][iDot] * texelsPerWorldUnits[iCoord][iDot];

			*pDestCoord += texelsPerWorldUnits[iCoord][3];
			*pDestCoord -= subtractOffset[iCoord];
		}
	}
}


/*
================
CalcFaceExtents

Fills in s->texmins[] and s->texsize[]
================
*/
void CalcFaceExtents(dface_t *s, int lightmapTextureMinsInLuxels[2], int lightmapTextureSizeInLuxels[2])
{
	vec_t	    mins[2], maxs[2], val=0;
	int		    i,j, e=0;
	dvertex_t	*v=NULL;
	texinfo_t	*tex=NULL;
	
	mins[0] = mins[1] = 1e24;
	maxs[0] = maxs[1] = -1e24;

	tex = &texinfo[s->texinfo];
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = dsurfedges[s->firstedge+i];
		if (e >= 0)
			v = dvertexes + dedges[e].v[0];
		else
			v = dvertexes + dedges[-e].v[1];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->point[0] * tex->lightmapVecsLuxelsPerWorldUnits[j][0] + 
				  v->point[1] * tex->lightmapVecsLuxelsPerWorldUnits[j][1] + 
				  v->point[2] * tex->lightmapVecsLuxelsPerWorldUnits[j][2] + 
				  tex->lightmapVecsLuxelsPerWorldUnits[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	int nMaxLightmapDim = (s->dispinfo == -1) ? MAX_LIGHTMAP_DIM_WITHOUT_BORDER : MAX_DISP_LIGHTMAP_DIM_WITHOUT_BORDER;
	for (i=0 ; i<2 ; i++)
	{	
		mins[i] = ( float )floor( mins[i] );
		maxs[i] = ( float )ceil( maxs[i] );

		lightmapTextureMinsInLuxels[i] = ( int )mins[i];
		lightmapTextureSizeInLuxels[i] = ( int )( maxs[i] - mins[i] );
		if( lightmapTextureSizeInLuxels[i] > nMaxLightmapDim + 1 )
		{
			Vector point = vec3_origin;
			for (int j=0 ; j<s->numedges ; j++)
			{
				e = dsurfedges[s->firstedge+j];
				v = (e<0)?dvertexes + dedges[-e].v[1] : dvertexes + dedges[e].v[0];
				point += v->point;
				Warning( "Bad surface extents point: %f %f %f\n", v->point.x, v->point.y, v->point.z );
			}
			point *= 1.0f/s->numedges;
			Error( "Bad surface extents - surface is too big to have a lightmap\n\tmaterial %s around point (%.1f %.1f %.1f)\n\t(dimension: %d, %d>%d)\n", 
				TexDataStringTable_GetString( dtexdata[texinfo[s->texinfo].texdata].nameStringTableID ), 
				point.x, point.y, point.z,
				( int )i,
				( int )lightmapTextureSizeInLuxels[i],
				( int )( nMaxLightmapDim + 1 )
				);
		}
	}
}


void UpdateAllFaceLightmapExtents()
{
	for( int i=0; i < numfaces; i++ )
	{
		dface_t *pFace = &dfaces[i];

		if ( texinfo[pFace->texinfo].flags & (SURF_SKY|SURF_NOLIGHT) )
			continue;		// non-lit texture

		CalcFaceExtents( pFace, pFace->m_LightmapTextureMinsInLuxels, pFace->m_LightmapTextureSizeInLuxels );
	}
}


//-----------------------------------------------------------------------------
//
// Helper class to iterate over leaves, used by tools
//
//-----------------------------------------------------------------------------

#define TEST_EPSILON	(0.03125)


class CToolBSPTree : public ISpatialQuery
{
public:
	// Returns the number of leaves
	int LeafCount() const;

	// Enumerates the leaves along a ray, box, etc.
	bool EnumerateLeavesAtPoint( Vector const& pt, ISpatialLeafEnumerator* pEnum, intp context );
	bool EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, ISpatialLeafEnumerator* pEnum, intp context );
	bool EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, intp context );
	bool EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, intp context );
};


//-----------------------------------------------------------------------------
// Returns the number of leaves
//-----------------------------------------------------------------------------

int CToolBSPTree::LeafCount() const
{
	return numleafs;
}


//-----------------------------------------------------------------------------
// Enumerates the leaves at a point
//-----------------------------------------------------------------------------

bool CToolBSPTree::EnumerateLeavesAtPoint( Vector const& pt, 
									ISpatialLeafEnumerator* pEnum, intp context )
{
	int node = 0;
	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if (DotProduct( pPlane->normal, pt ) <= pPlane->dist)
		{
			node = pNode->children[1];
		}
		else
		{
			node = pNode->children[0];
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}


//-----------------------------------------------------------------------------
// Enumerates the leaves in a box
//-----------------------------------------------------------------------------

static bool EnumerateLeavesInBox_R( int node, Vector const& mins, 
				Vector const& maxs, ISpatialLeafEnumerator* pEnum, intp context )
{
	Vector cornermin, cornermax;

	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		// Arbitrary split plane here
		for (int i = 0; i < 3; ++i)
		{
			if (pPlane->normal[i] >= 0)
			{
				cornermin[i] = mins[i];
				cornermax[i] = maxs[i];
			}
			else
			{
				cornermin[i] = maxs[i];
				cornermax[i] = mins[i];
			}
		}

		if ( (DotProduct( pPlane->normal, cornermax ) - pPlane->dist) <= -TEST_EPSILON )
		{
			node = pNode->children[1];
		}
		else if ( (DotProduct( pPlane->normal, cornermin ) - pPlane->dist) >= TEST_EPSILON )
		{
			node = pNode->children[0];
		}
		else
		{
			if (!EnumerateLeavesInBox_R( pNode->children[0], mins, maxs, pEnum, context ))
			{
				return false;
			}

			return EnumerateLeavesInBox_R( pNode->children[1], mins, maxs, pEnum, context );
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesInBox( Vector const& mins, Vector const& maxs, 
									ISpatialLeafEnumerator* pEnum, intp context )
{
	return EnumerateLeavesInBox_R( 0, mins, maxs, pEnum, context );
}

//-----------------------------------------------------------------------------
// Enumerate leaves within a sphere
//-----------------------------------------------------------------------------

static bool EnumerateLeavesInSphere_R( int node, Vector const& origin, 
				float radius, ISpatialLeafEnumerator* pEnum, intp context )
{
	while( node >= 0 )
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if (DotProduct( pPlane->normal, origin ) + radius - pPlane->dist <= -TEST_EPSILON )
		{
			node = pNode->children[1];
		}
		else if (DotProduct( pPlane->normal, origin ) - radius - pPlane->dist >= TEST_EPSILON )
		{
			node = pNode->children[0];
		}
		else
		{
			if (!EnumerateLeavesInSphere_R( pNode->children[0], 
					origin, radius, pEnum, context ))
			{
				return false;
			}

			return EnumerateLeavesInSphere_R( pNode->children[1],
				origin, radius, pEnum, context );
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesInSphere( Vector const& center, float radius, ISpatialLeafEnumerator* pEnum, intp context )
{
	return EnumerateLeavesInSphere_R( 0, center, radius, pEnum, context );
}


//-----------------------------------------------------------------------------
// Enumerate leaves along a ray
//-----------------------------------------------------------------------------

static bool EnumerateLeavesAlongRay_R( int node, Ray_t const& ray, 
	Vector const& start, Vector const& end, ISpatialLeafEnumerator* pEnum, intp context )
{
	float front,back;

	while (node >= 0)
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if ( pPlane->type <= PLANE_Z )
		{
			front = start[pPlane->type] - pPlane->dist;
			back = end[pPlane->type] - pPlane->dist;
		}
		else
		{
			front = DotProduct(start, pPlane->normal) - pPlane->dist;
			back = DotProduct(end, pPlane->normal) - pPlane->dist;
		}

		if (front <= -TEST_EPSILON && back <= -TEST_EPSILON)
		{
			node = pNode->children[1];
		}
		else if (front >= TEST_EPSILON && back >= TEST_EPSILON)
		{
			node = pNode->children[0];
		}
		else
		{
			// test the front side first
			bool side = front < 0;

			// Compute intersection point based on the original ray
			float splitfrac;
			float denom = DotProduct( ray.m_Delta, pPlane->normal );
			if ( denom == 0.0f )
			{
				splitfrac = 1.0f;
			}
			else
			{
				splitfrac = (	pPlane->dist - DotProduct( ray.m_Start, pPlane->normal ) ) / denom;
				if (splitfrac < 0)
					splitfrac = 0;
				else if (splitfrac > 1)
					splitfrac = 1;
			}

			// Compute the split point
			Vector split;
			VectorMA( ray.m_Start, splitfrac, ray.m_Delta, split );

			bool r = EnumerateLeavesAlongRay_R (pNode->children[side], ray, start, split, pEnum, context );
			if (!r)
				return r;
			return EnumerateLeavesAlongRay_R (pNode->children[!side], ray, split, end, pEnum, context);
		}
	}

	return pEnum->EnumerateLeaf( - node - 1, context );
}

bool CToolBSPTree::EnumerateLeavesAlongRay( Ray_t const& ray, ISpatialLeafEnumerator* pEnum, intp context )
{
	if (!ray.m_IsSwept)
	{
		Vector mins, maxs;
		VectorAdd( ray.m_Start, ray.m_Extents, maxs );
		VectorSubtract( ray.m_Start, ray.m_Extents, mins );

		return EnumerateLeavesInBox_R( 0, mins, maxs, pEnum, context );
	}

	// FIXME: Extruded ray not implemented yet
	Assert( ray.m_IsRay );

	Vector end;
	VectorAdd( ray.m_Start, ray.m_Delta, end );
	return EnumerateLeavesAlongRay_R( 0, ray, ray.m_Start, end, pEnum, context );
}


//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------

ISpatialQuery* ToolBSPTree()
{
	static CToolBSPTree s_ToolBSPTree;
	return &s_ToolBSPTree;
}



//-----------------------------------------------------------------------------
// Enumerates nodes in front to back order...
//-----------------------------------------------------------------------------

// FIXME: Do we want this in the IBSPTree interface?

static bool EnumerateNodesAlongRay_R( int node, Ray_t const& ray, float start, float end,
	IBSPNodeEnumerator* pEnum, intp context )
{
	float front, back;
	float startDotN, deltaDotN;

	while (node >= 0)
	{
		dnode_t* pNode = &dnodes[node];
		dplane_t* pPlane = &dplanes[pNode->planenum];

		if ( pPlane->type <= PLANE_Z )
		{
			startDotN = ray.m_Start[pPlane->type];
			deltaDotN = ray.m_Delta[pPlane->type];
		}
		else
		{
			startDotN = DotProduct( ray.m_Start, pPlane->normal );
			deltaDotN = DotProduct( ray.m_Delta, pPlane->normal );
		}

		front = startDotN + start * deltaDotN - pPlane->dist;
		back = startDotN + end * deltaDotN - pPlane->dist;

		if (front <= -TEST_EPSILON && back <= -TEST_EPSILON)
		{
			node = pNode->children[1];
		}
		else if (front >= TEST_EPSILON && back >= TEST_EPSILON)
		{
			node = pNode->children[0];
		}
		else
		{
			// test the front side first
			bool side = front < 0;

			// Compute intersection point based on the original ray
			float splitfrac;
			if ( deltaDotN == 0.0f )
			{
				splitfrac = 1.0f;
			}
			else
			{
				splitfrac = ( pPlane->dist - startDotN ) / deltaDotN;
				if (splitfrac < 0.0f)
					splitfrac = 0.0f;
				else if (splitfrac > 1.0f)
					splitfrac = 1.0f;
			}

			bool r = EnumerateNodesAlongRay_R (pNode->children[side], ray, start, splitfrac, pEnum, context );
			if (!r)
				return r;

			// Visit the node...
			if (!pEnum->EnumerateNode( node, ray, splitfrac, context ))
				return false;

			return EnumerateNodesAlongRay_R (pNode->children[!side], ray, splitfrac, end, pEnum, context);
		}
	}

	// Visit the leaf...
	return pEnum->EnumerateLeaf( - node - 1, ray, start, end, context );
}


bool EnumerateNodesAlongRay( Ray_t const& ray, IBSPNodeEnumerator* pEnum, intp context )
{
	Vector end;
	VectorAdd( ray.m_Start, ray.m_Delta, end );
	return EnumerateNodesAlongRay_R( 0, ray, 0.0f, 1.0f, pEnum, context );
}


//-----------------------------------------------------------------------------
// Helps us find all leaves associated with a particular cluster
//-----------------------------------------------------------------------------
CUtlVector<clusterlist_t> g_ClusterLeaves;

void BuildClusterTable( void )
{
	int i, j;
	int leafCount;
	int	leafList[MAX_MAP_LEAFS];

	g_ClusterLeaves.SetCount( dvis->numclusters );
	for ( i = 0; i < dvis->numclusters; i++ )
	{
		leafCount = 0;
		for ( j = 0; j < numleafs; j++ )
		{
			if ( dleafs[j].cluster == i )
			{
				leafList[ leafCount ] = j;
				leafCount++;
			}
		}

		g_ClusterLeaves[i].leafCount = leafCount;
		if ( leafCount )
		{
			g_ClusterLeaves[i].leafs.SetCount( leafCount );
			memcpy( g_ClusterLeaves[i].leafs.Base(), leafList, sizeof(int) * leafCount );
		}
	}
}

// There's a version of this in checksum_engine.cpp!!! Make sure that they match.
static bool CRC_MapFile(CRC32_t *crcvalue, const char *pszFileName)
{
	byte chunk[1024];
	lump_t *curLump;

	FileHandle_t fp = g_pFileSystem->Open( pszFileName, "rb" );
	if ( !fp )
		return false;

	// CRC across all lumps except for the Entities lump
	for ( int l = 0; l < HEADER_LUMPS; ++l )
	{
		if (l == LUMP_ENTITIES)
			continue;

		curLump = &g_pBSPHeader->lumps[l];
		unsigned int nSize = curLump->filelen;

		g_pFileSystem->Seek( fp, curLump->fileofs, FILESYSTEM_SEEK_HEAD );

		// Now read in 1K chunks
		while ( nSize > 0 )
		{
			int nBytesRead = 0;

			if ( nSize > 1024 )
				nBytesRead = g_pFileSystem->Read( chunk, 1024, fp );
			else
				nBytesRead = g_pFileSystem->Read( chunk, nSize, fp );

			// If any data was received, CRC it.
			if ( nBytesRead > 0 )
			{
				nSize -= nBytesRead;
				CRC32_ProcessBuffer( crcvalue, chunk, nBytesRead );
			}
			else
			{
				g_pFileSystem->Close( fp );
				return false;
			}
		}	
	}
	
	g_pFileSystem->Close( fp );
	return true;
}


void SetHDRMode( bool bHDR )
{
	g_bHDR = bHDR;
	if ( bHDR )
	{
		pdlightdata = &dlightdataHDR;		
		g_pLeafAmbientLighting = &g_LeafAmbientLightingHDR;
		g_pLeafAmbientIndex = &g_LeafAmbientIndexHDR;
		pNumworldlights = &numworldlightsHDR;
		dworldlights = dworldlightsHDR;
#ifdef VRAD
		extern void VRadDetailProps_SetHDRMode( bool bHDR );
		VRadDetailProps_SetHDRMode( bHDR );
#endif
	}
	else
	{
		pdlightdata = &dlightdataLDR;		
		g_pLeafAmbientLighting = &g_LeafAmbientLightingLDR;
		g_pLeafAmbientIndex = &g_LeafAmbientIndexLDR;
		pNumworldlights = &numworldlightsLDR;
		dworldlights = dworldlightsLDR;
#ifdef VRAD
		extern void VRadDetailProps_SetHDRMode( bool bHDR );
		VRadDetailProps_SetHDRMode( bHDR );
#endif
	}
}

bool SwapVHV( void *pDestBase, void *pSrcBase )
{
	byte *pDest = (byte*)pDestBase;
	byte *pSrc = (byte*)pSrcBase;

	HardwareVerts::FileHeader_t *pHdr = (HardwareVerts::FileHeader_t*)( g_bSwapOnLoad ? pDest : pSrc );
	g_Swap.SwapFieldsToTargetEndian<HardwareVerts::FileHeader_t>( (HardwareVerts::FileHeader_t*)pDest, (HardwareVerts::FileHeader_t*)pSrc );
	pSrc += sizeof(HardwareVerts::FileHeader_t);
	pDest += sizeof(HardwareVerts::FileHeader_t);

	// This swap is pretty format specific
	Assert( pHdr->m_nVersion == VHV_VERSION );
	if ( pHdr->m_nVersion != VHV_VERSION )
		return false;

	HardwareVerts::MeshHeader_t *pSrcMesh = (HardwareVerts::MeshHeader_t*)pSrc;
	HardwareVerts::MeshHeader_t *pDestMesh = (HardwareVerts::MeshHeader_t*)pDest;
	HardwareVerts::MeshHeader_t *pMesh = (HardwareVerts::MeshHeader_t*)( g_bSwapOnLoad ? pDest : pSrc );
	for ( int i = 0; i < pHdr->m_nMeshes; ++i, ++pMesh, ++pSrcMesh, ++pDestMesh )
	{
		g_Swap.SwapFieldsToTargetEndian( pDestMesh, pSrcMesh );

		pSrc = (byte*)pSrcBase + pMesh->m_nOffset;
		pDest = (byte*)pDestBase + pMesh->m_nOffset;

		// Swap as a buffer of integers 
		// (source is bgra for an Intel swap to argb. PowerPC won't swap, so we need argb source. 
		g_Swap.SwapBufferToTargetEndian<int>( (int*)pDest, (int*)pSrc, pMesh->m_nVertexes );
	}
	return true;
}

const char *ResolveStaticPropToModel( const char *pPropName )
{
	// resolve back to static prop
	int iProp = -1;

	// filename should be sp_???.vhv or sp_hdr_???.vhv
	if ( V_strnicmp( pPropName, "sp_", 3 ) )
	{
		return NULL;
	}
	const char *pPropNumber = V_strrchr( pPropName, '_' );
	if ( pPropNumber )
	{
		sscanf( pPropNumber+1, "%d.vhv", &iProp );
	}
	else
	{
		return NULL;
	}

	// look up the prop to get to the actual model
	if ( iProp < 0 || iProp >= g_StaticPropInstances.Count() )
	{
		// prop out of range
		return NULL;
	}
	int iModel = g_StaticPropInstances[iProp];
	if ( iModel < 0 || iModel >= g_StaticPropNames.Count() )
	{
		// model out of range
		return NULL;
	}

	return g_StaticPropNames[iModel].String();
}

//-----------------------------------------------------------------------------
// Iterate files in pak file, distribute to converters
// pak file will be ready for serialization upon completion
//-----------------------------------------------------------------------------
void ConvertPakFileContents( const char *pInFilename )
{
	IZip *newPakFile = IZip::CreateZip( NULL );

	CUtlBuffer sourceBuf;
	CUtlBuffer targetBuf;
	bool bConverted;
	CUtlVector< CUtlString > hdrFiles;

	int id = -1;
	int fileSize;
	while ( 1 )
	{
		char relativeName[MAX_PATH];
		id = GetNextFilename( GetPakFile(), id, relativeName, sizeof( relativeName ), fileSize );
		if ( id == -1)
			break;

		bConverted = false;
		sourceBuf.Purge();
		targetBuf.Purge();

		const char* pExtension = V_GetFileExtension( relativeName );
		const char* pExt = 0;

		bool bOK = ReadFileFromPak( GetPakFile(), relativeName, false, sourceBuf );
		if ( !bOK )
		{
			Warning( "Failed to load '%s' from lump pak for conversion or copy in '%s'.\n", relativeName, pInFilename );
			continue;
		}

		if ( pExtension && !V_stricmp( pExtension, "vtf" ) )
		{
			bOK = g_pVTFConvertFunc( relativeName, sourceBuf, targetBuf, g_pCompressFunc );
			if ( !bOK )
			{
				Warning( "Failed to convert '%s' in '%s'.\n", relativeName, pInFilename );
				continue;
			}
	
			bConverted = true;
			pExt = ".vtf";
		}
		else if ( pExtension && !V_stricmp( pExtension, "vhv" ) )
		{			
			CUtlBuffer tempBuffer;
			if ( g_pVHVFixupFunc )
			{
				// caller supplied a fixup
				const char *pModelName = ResolveStaticPropToModel( relativeName );
				if ( !pModelName )
				{
					Warning( "Static Prop '%s' failed to resolve actual model in '%s'.\n", relativeName, pInFilename );
					continue;
				}

				// output temp buffer may shrink, must use TellPut() to determine size
				bOK = g_pVHVFixupFunc( relativeName, pModelName, sourceBuf, tempBuffer );
				if ( !bOK )
				{
					Warning( "Failed to convert '%s' in '%s'.\n", relativeName, pInFilename );
					continue;
				}
			}
			else
			{
				// use the source buffer as-is
				tempBuffer.EnsureCapacity( sourceBuf.TellMaxPut() );
				tempBuffer.Put( sourceBuf.Base(), sourceBuf.TellMaxPut() );
			}

			// swap the VHV
			targetBuf.EnsureCapacity( tempBuffer.TellPut() );
			bOK = SwapVHV( targetBuf.Base(), tempBuffer.Base() );
			if ( !bOK )
			{
				Warning( "Failed to swap '%s' in '%s'.\n", relativeName, pInFilename );
				continue;
			}
			targetBuf.SeekPut( CUtlBuffer::SEEK_HEAD, tempBuffer.TellPut() );

			if ( g_pCompressFunc )
			{
				CUtlBuffer compressedBuffer;
				targetBuf.SeekGet( CUtlBuffer::SEEK_HEAD, sizeof( HardwareVerts::FileHeader_t ) );
				bool bCompressed = g_pCompressFunc( targetBuf, compressedBuffer );
				if ( bCompressed )
				{
					// copy all the header data off
					CUtlBuffer headerBuffer;
					headerBuffer.EnsureCapacity( sizeof( HardwareVerts::FileHeader_t ) );
					headerBuffer.Put( targetBuf.Base(), sizeof( HardwareVerts::FileHeader_t ) );

					// reform the target with the header and then the compressed data
					targetBuf.Clear();
					targetBuf.Put( headerBuffer.Base(), sizeof( HardwareVerts::FileHeader_t ) );
					targetBuf.Put( compressedBuffer.Base(), compressedBuffer.TellPut() );
				}

				targetBuf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
			}

			bConverted = true;
			pExt = ".vhv";
		}

		if ( !bConverted )
		{
			// straight copy
			AddBufferToPak( newPakFile, relativeName, sourceBuf.Base(), sourceBuf.TellMaxPut(), false, IZip::eCompressionType_None );
		}
		else
		{
			// converted filename
			V_StripExtension( relativeName, relativeName, sizeof( relativeName ) );
			V_strcat( relativeName, ".360", sizeof( relativeName ) );
			V_strcat( relativeName, pExt, sizeof( relativeName ) );
			AddBufferToPak( newPakFile, relativeName, targetBuf.Base(), targetBuf.TellMaxPut(), false, IZip::eCompressionType_None );
		}

		if ( V_stristr( relativeName, ".hdr" ) || V_stristr( relativeName, "_hdr" ) )
		{
			hdrFiles.AddToTail( relativeName );
		}

		DevMsg( "Created '%s' in lump pak in '%s'.\n", relativeName, pInFilename );
	}

	// strip ldr version of hdr files
	for ( int i=0; i<hdrFiles.Count(); i++ )
	{
		char ldrFileName[MAX_PATH];

		strcpy( ldrFileName, hdrFiles[i].String() );

		char *pHDRExtension = V_stristr( ldrFileName, ".hdr" );
		if ( !pHDRExtension )
		{
			pHDRExtension = V_stristr( ldrFileName, "_hdr" );
		}

		if ( pHDRExtension )
		{
			// strip .hdr or _hdr to get ldr filename
			memcpy( pHDRExtension, pHDRExtension+4, strlen( pHDRExtension+4 )+1 );

			DevMsg( "Stripping LDR: %s\n", ldrFileName );
			newPakFile->RemoveFileFromZip( ldrFileName );
		}
	}

	// discard old pak in favor of new pak
	IZip::ReleaseZip( s_pakFile );
	s_pakFile = newPakFile;
}

void SetAlignedLumpPosition( int lumpnum, int alignment = LUMP_ALIGNMENT )
{
	g_pBSPHeader->lumps[lumpnum].fileofs = AlignFilePosition( g_hBSPFile, alignment );
}

template< class T >
int SwapLumpToDisk( int fieldType, int lumpnum )
{
	if ( g_pBSPHeader->lumps[lumpnum].filelen == 0 )
		return 0;

	DevMsg( "Swapping %s\n", GetLumpName( lumpnum ) );

	// lump swap may expand, allocate enough expansion room
	void *pBuffer = malloc( 2*g_pBSPHeader->lumps[lumpnum].filelen );

	// CopyLumpInternal will handle the swap on load case
	unsigned int fieldSize = ( fieldType == FIELD_VECTOR ) ? sizeof(Vector) : sizeof(T);
	unsigned int count = CopyLumpInternal<T>( fieldType, lumpnum, (T*)pBuffer, g_pBSPHeader->lumps[lumpnum].version );
	g_pBSPHeader->lumps[lumpnum].filelen = count * fieldSize;

	if ( g_bSwapOnWrite )
	{
		// Swap the lump in place before writing
		switch( lumpnum )
		{
		case LUMP_VISIBILITY:
			SwapVisibilityLump( (byte*)pBuffer, (byte*)pBuffer, count );
			break;
		
		case LUMP_PHYSCOLLIDE:
			// SwapPhyscollideLump may change size
			SwapPhyscollideLump( (byte*)pBuffer, (byte*)pBuffer, count );
			g_pBSPHeader->lumps[lumpnum].filelen = count;
			break;

		case LUMP_PHYSDISP:
			SwapPhysdispLump( (byte*)pBuffer, (byte*)pBuffer, count );
			break;

		default:
			g_Swap.SwapBufferToTargetEndian( (T*)pBuffer, (T*)pBuffer, g_pBSPHeader->lumps[lumpnum].filelen / sizeof(T) );
			break;
		}
	}

	SetAlignedLumpPosition( lumpnum );
	SafeWrite( g_hBSPFile, pBuffer, g_pBSPHeader->lumps[lumpnum].filelen );

	free( pBuffer );

	return g_pBSPHeader->lumps[lumpnum].filelen;
}

template< class T >
int SwapLumpToDisk( int lumpnum )
{
	if ( g_pBSPHeader->lumps[lumpnum].filelen == 0 || g_Lumps.bLumpParsed[lumpnum] )
		return 0;

	DevMsg( "Swapping %s\n", GetLumpName( lumpnum ) );

	// lump swap may expand, allocate enough room
	void *pBuffer = malloc( 2*g_pBSPHeader->lumps[lumpnum].filelen );

	// CopyLumpInternal will handle the swap on load case
	int count = CopyLumpInternal<T>( lumpnum, (T*)pBuffer, g_pBSPHeader->lumps[lumpnum].version );
	g_pBSPHeader->lumps[lumpnum].filelen = count * sizeof(T);

	if ( g_bSwapOnWrite )
	{
		// Swap the lump in place before writing
		g_Swap.SwapFieldsToTargetEndian( (T*)pBuffer, (T*)pBuffer, count );
	}

	SetAlignedLumpPosition( lumpnum );
	SafeWrite( g_hBSPFile, pBuffer, g_pBSPHeader->lumps[lumpnum].filelen );
	free( pBuffer );

	return g_pBSPHeader->lumps[lumpnum].filelen;
}

void SwapLeafAmbientLightingLumpToDisk()
{
	if ( HasLump( LUMP_LEAF_AMBIENT_INDEX ) || HasLump( LUMP_LEAF_AMBIENT_INDEX_HDR ) )
	{
		// current version, swap in place
		if ( HasLump( LUMP_LEAF_AMBIENT_INDEX_HDR ) )
		{
			// write HDR
			SwapLumpToDisk< dleafambientlighting_t >( LUMP_LEAF_AMBIENT_LIGHTING_HDR );
			SwapLumpToDisk< dleafambientindex_t >( LUMP_LEAF_AMBIENT_INDEX_HDR );

			// cull LDR			
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].filelen = 0;
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX].filelen = 0;
		}
		else
		{
			// no HDR, keep LDR version
			SwapLumpToDisk< dleafambientlighting_t >( LUMP_LEAF_AMBIENT_LIGHTING );
			SwapLumpToDisk< dleafambientindex_t >( LUMP_LEAF_AMBIENT_INDEX );
		}
	}
	else
	{
		// older ambient lighting version (before index)
		// load older ambient lighting into memory and build ambient/index
		// an older leaf version would have already built the new LDR leaf ambient/index
		int numLeafs = g_pBSPHeader->lumps[LUMP_LEAFS].filelen / sizeof( dleaf_t );
		LoadLeafAmbientLighting( numLeafs );

		if ( HasLump( LUMP_LEAF_AMBIENT_LIGHTING_HDR ) )
		{
			DevMsg( "Swapping %s\n", GetLumpName( LUMP_LEAF_AMBIENT_LIGHTING_HDR ) );
			DevMsg( "Swapping %s\n", GetLumpName( LUMP_LEAF_AMBIENT_INDEX_HDR ) );

			// write HDR
			if ( g_bSwapOnWrite )
			{
				g_Swap.SwapFieldsToTargetEndian( g_LeafAmbientLightingHDR.Base(), g_LeafAmbientLightingHDR.Count() );
				g_Swap.SwapFieldsToTargetEndian( g_LeafAmbientIndexHDR.Base(), g_LeafAmbientIndexHDR.Count() );
			}

			SetAlignedLumpPosition( LUMP_LEAF_AMBIENT_LIGHTING_HDR );
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING_HDR].version = LUMP_LEAF_AMBIENT_LIGHTING_VERSION;
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING_HDR].filelen = g_LeafAmbientLightingHDR.Count() * sizeof( dleafambientlighting_t );
			SafeWrite( g_hBSPFile, g_LeafAmbientLightingHDR.Base(), g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING_HDR].filelen );

			SetAlignedLumpPosition( LUMP_LEAF_AMBIENT_INDEX_HDR );
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX_HDR].filelen = g_LeafAmbientIndexHDR.Count() * sizeof( dleafambientindex_t );
			SafeWrite( g_hBSPFile, g_LeafAmbientIndexHDR.Base(), g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX_HDR].filelen );

			// mark as processed
			g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_LIGHTING_HDR] = true;
			g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_INDEX_HDR] = true;

			// cull LDR
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].filelen = 0;
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX].filelen = 0;
		}
		else
		{
			// no HDR, keep LDR version
			DevMsg( "Swapping %s\n", GetLumpName( LUMP_LEAF_AMBIENT_LIGHTING ) );
			DevMsg( "Swapping %s\n", GetLumpName( LUMP_LEAF_AMBIENT_INDEX ) );

			if ( g_bSwapOnWrite )
			{
				g_Swap.SwapFieldsToTargetEndian( g_LeafAmbientLightingLDR.Base(), g_LeafAmbientLightingLDR.Count() );
				g_Swap.SwapFieldsToTargetEndian( g_LeafAmbientIndexLDR.Base(), g_LeafAmbientIndexLDR.Count() );
			}

			SetAlignedLumpPosition( LUMP_LEAF_AMBIENT_LIGHTING );
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].version = LUMP_LEAF_AMBIENT_LIGHTING_VERSION;
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].filelen = g_LeafAmbientLightingLDR.Count() * sizeof( dleafambientlighting_t );
			SafeWrite( g_hBSPFile, g_LeafAmbientLightingLDR.Base(), g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_LIGHTING].filelen );

			SetAlignedLumpPosition( LUMP_LEAF_AMBIENT_INDEX );
			g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX].filelen = g_LeafAmbientIndexLDR.Count() * sizeof( dleafambientindex_t );
			SafeWrite( g_hBSPFile, g_LeafAmbientIndexLDR.Base(), g_pBSPHeader->lumps[LUMP_LEAF_AMBIENT_INDEX].filelen );

			// mark as processed
			g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_LIGHTING] = true;
			g_Lumps.bLumpParsed[LUMP_LEAF_AMBIENT_INDEX] = true;
		}

		g_LeafAmbientLightingLDR.Purge();
		g_LeafAmbientIndexLDR.Purge();
		g_LeafAmbientLightingHDR.Purge();
		g_LeafAmbientIndexHDR.Purge();
	}
}

void SwapLeafLumpToDisk( void )
{
	DevMsg( "Swapping %s\n", GetLumpName( LUMP_LEAFS ) );

	// load the leafs
	int count = LoadLeafs();
	if ( g_bSwapOnWrite )
	{
		g_Swap.SwapFieldsToTargetEndian( dleafs, count );
	}

	bool bOldLeafVersion = ( LumpVersion( LUMP_LEAFS ) == 0 );
	if ( bOldLeafVersion )
	{
		// version has been converted in the load process
		// not updating the version ye, SwapLeafAmbientLightingLumpToDisk() can detect
		g_pBSPHeader->lumps[LUMP_LEAFS].filelen = count * sizeof( dleaf_t );
	}

	SetAlignedLumpPosition( LUMP_LEAFS );
	SafeWrite( g_hBSPFile, dleafs, g_pBSPHeader->lumps[LUMP_LEAFS].filelen );

	SwapLeafAmbientLightingLumpToDisk();

	if ( bOldLeafVersion )
	{
		// version has been converted in the load process
		// can now safely change
		g_pBSPHeader->lumps[LUMP_LEAFS].version = 1;
	}

#if defined( BSP_USE_LESS_MEMORY )
	if ( dleafs )
	{
		free( dleafs );
		dleafs = NULL;
	}
#endif
}

void SwapOcclusionLumpToDisk( void )
{
	DevMsg( "Swapping %s\n", GetLumpName( LUMP_OCCLUSION ) );

	LoadOcclusionLump();
	SetAlignedLumpPosition( LUMP_OCCLUSION );
	AddOcclusionLump();
}

void SwapPakfileLumpToDisk( const char *pInFilename )
{
	DevMsg( "Swapping %s\n", GetLumpName( LUMP_PAKFILE ) );

	byte *pakbuffer = NULL;
	int paksize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PAKFILE, ( void ** )&pakbuffer );
	if ( paksize > 0 )
	{
		GetPakFile()->ActivateByteSwapping( IsX360() );
		GetPakFile()->ParseFromBuffer( pakbuffer, paksize );

		ConvertPakFileContents( pInFilename );
	}
	free( pakbuffer );

	SetAlignedLumpPosition( LUMP_PAKFILE, XBOX_DVD_SECTORSIZE );
	WritePakFileLump();

	ReleasePakFileLumps();
}

void SwapGameLumpsToDisk( void )
{
	DevMsg( "Swapping %s\n", GetLumpName( LUMP_GAME_LUMP ) );

	g_GameLumps.ParseGameLump( g_pBSPHeader );
	SetAlignedLumpPosition( LUMP_GAME_LUMP );
	AddGameLumps();
}

//-----------------------------------------------------------------------------
// Generate a table of all static props, used for resolving static prop lighting
// files back to their actual mdl.
//-----------------------------------------------------------------------------
void BuildStaticPropNameTable()
{
	g_StaticPropNames.Purge();
	g_StaticPropInstances.Purge();

	g_GameLumps.ParseGameLump( g_pBSPHeader );

	GameLumpHandle_t hGameLump = g_GameLumps.GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	if ( hGameLump != g_GameLumps.InvalidGameLump() )
	{
		int nVersion = g_GameLumps.GetGameLumpVersion( hGameLump );
		if ( nVersion < 4 )
		{
			// old unsupported version
			return;
		}

		if ( nVersion != 4 && nVersion != 5 && nVersion != 6 )
		{
			Error( "Unknown Static Prop Lump version %d!\n", nVersion );
		}

		byte *pGameLumpData = (byte *)g_GameLumps.GetGameLump( hGameLump );
		if ( pGameLumpData && g_GameLumps.GameLumpSize( hGameLump ) )
		{
			// get the model dictionary
			int count = ((int *)pGameLumpData)[0];
			pGameLumpData += sizeof( int );
			StaticPropDictLump_t *pStaticPropDictLump = (StaticPropDictLump_t *)pGameLumpData;
			for ( int i = 0; i < count; i++ )
			{
				g_StaticPropNames.AddToTail( pStaticPropDictLump[i].m_Name );
			}
			pGameLumpData += count * sizeof( StaticPropDictLump_t );

			// skip the leaf list
			count = ((int *)pGameLumpData)[0];
			pGameLumpData += sizeof( int );
			pGameLumpData += count * sizeof( StaticPropLeafLump_t );

			// get the instances
			count = ((int *)pGameLumpData)[0];
			pGameLumpData += sizeof( int );
			for ( int i = 0; i < count; i++ )
			{
				int propType;
				if ( nVersion == 4 )
				{
					propType = ((StaticPropLumpV4_t *)pGameLumpData)->m_PropType;
					pGameLumpData += sizeof( StaticPropLumpV4_t );
				}
				else if ( nVersion == 5 )
				{
					propType = ((StaticPropLumpV5_t *)pGameLumpData)->m_PropType;
					pGameLumpData += sizeof( StaticPropLumpV5_t );
				}
				else
				{
					propType = ((StaticPropLump_t *)pGameLumpData)->m_PropType;
					pGameLumpData += sizeof( StaticPropLump_t );
				}
				g_StaticPropInstances.AddToTail( propType );
			}
		}
	}

	g_GameLumps.DestroyAllGameLumps();
}

int AlignBuffer( CUtlBuffer &buffer, int alignment )
{
	unsigned int newPosition = AlignValue( buffer.TellPut(), alignment );
	int padLength = newPosition - buffer.TellPut();
	for ( int i = 0; i<padLength; i++ )
	{
		buffer.PutChar( '\0' );
	}
	return buffer.TellPut();
}

struct SortedLump_t
{
	int		lumpNum;
	lump_t	*pLump;
};

int SortLumpsByOffset( const SortedLump_t *pSortedLumpA, const SortedLump_t *pSortedLumpB ) 
{
	int fileOffsetA = pSortedLumpA->pLump->fileofs;
	int fileOffsetB = pSortedLumpB->pLump->fileofs;

	int fileSizeA = pSortedLumpA->pLump->filelen;
	int fileSizeB = pSortedLumpB->pLump->filelen;

	// invalid or empty lumps get sorted together
	if ( !fileSizeA )
	{
		fileOffsetA = 0;
	}
	if ( !fileSizeB )
	{
		fileOffsetB = 0;
	}

	// compare by offset, want ascending
	if ( fileOffsetA < fileOffsetB )
	{
		return -1;
	}
	else if ( fileOffsetA > fileOffsetB )
	{
		return 1;
	}

	return 0;
}

bool CompressGameLump( dheader_t *pInBSPHeader, dheader_t *pOutBSPHeader, CUtlBuffer &outputBuffer, CompressFunc_t pCompressFunc )
{
	CByteswap	byteSwap;

	dgamelumpheader_t* pInGameLumpHeader = (dgamelumpheader_t*)(((byte *)pInBSPHeader) + pInBSPHeader->lumps[LUMP_GAME_LUMP].fileofs);
	dgamelump_t* pInGameLump = (dgamelump_t*)(pInGameLumpHeader + 1);

	if ( IsX360() )
	{
		byteSwap.ActivateByteSwapping( true );
		byteSwap.SwapFieldsToTargetEndian( pInGameLumpHeader );
		byteSwap.SwapFieldsToTargetEndian( pInGameLump, pInGameLumpHeader->lumpCount );
	}

	unsigned int newOffset = outputBuffer.TellPut();
	// Make room for gamelump header and gamelump structs, which we'll write at the end
	outputBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, sizeof( dgamelumpheader_t ) );
	outputBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, pInGameLumpHeader->lumpCount * sizeof( dgamelump_t ) );

	// Start with input lumps, and fixup
	dgamelumpheader_t sOutGameLumpHeader = *pInGameLumpHeader;
	CUtlBuffer sOutGameLumpBuf;
	sOutGameLumpBuf.Put( pInGameLump, pInGameLumpHeader->lumpCount * sizeof( dgamelump_t ) );
	dgamelump_t *sOutGameLump = (dgamelump_t *)sOutGameLumpBuf.Base();

	// add a dummy terminal gamelump
	// purposely NOT updating the .filelen to reflect the compressed size, but leaving as original size
	// callers use the next entry offset to determine compressed size
	sOutGameLumpHeader.lumpCount++;
	dgamelump_t dummyLump = { 0 };
	outputBuffer.Put( &dummyLump, sizeof( dgamelump_t ) );

	for ( int i = 0; i < pInGameLumpHeader->lumpCount; i++ )
	{
		CUtlBuffer inputBuffer;
		CUtlBuffer compressedBuffer;

		sOutGameLump[i].fileofs = AlignBuffer( outputBuffer, 4 );

		if ( pInGameLump[i].filelen )
		{
			if ( pInGameLump[i].flags & GAMELUMPFLAG_COMPRESSED )
			{
				byte *pCompressedLump = ((byte *)pInBSPHeader) + pInGameLump[i].fileofs;
				if ( CLZMA::IsCompressed( pCompressedLump ) )
				{
					inputBuffer.EnsureCapacity( CLZMA::GetActualSize( pCompressedLump ) );
					unsigned int outSize = CLZMA::Uncompress( pCompressedLump, (unsigned char *)inputBuffer.Base() );
					inputBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, outSize );
					if ( outSize != CLZMA::GetActualSize( pCompressedLump ) )
					{
						Warning( "Decompressed size differs from header, BSP may be corrupt\n" );
					}
				}
				else
				{
					Assert( CLZMA::IsCompressed( pCompressedLump ) );
					Warning( "Unsupported BSP: Unrecognized compressed game lump\n" );
				}

			}
			else
			{
				inputBuffer.SetExternalBuffer( ((byte *)pInBSPHeader) + pInGameLump[i].fileofs,
				                               pInGameLump[i].filelen, pInGameLump[i].filelen );
			}

			bool bCompressed = pCompressFunc ? pCompressFunc( inputBuffer, compressedBuffer ) : false;
			if ( bCompressed )
			{
				sOutGameLump[i].flags |= GAMELUMPFLAG_COMPRESSED;

				outputBuffer.Put( compressedBuffer.Base(), compressedBuffer.TellPut() );
				compressedBuffer.Purge();
			}
			else
			{
				// as is, clear compression flag from input lump
				sOutGameLump[i].flags &= ~GAMELUMPFLAG_COMPRESSED;
				outputBuffer.Put( inputBuffer.Base(), inputBuffer.TellPut() );
			}
		}
	}

	// fix the dummy terminal lump
	int lastLump = sOutGameLumpHeader.lumpCount-1;
	sOutGameLump[lastLump].fileofs = outputBuffer.TellPut();

	if ( IsX360() )
	{
		// fix the output for 360, swapping it back
		byteSwap.SwapFieldsToTargetEndian( sOutGameLump, sOutGameLumpHeader.lumpCount );
		byteSwap.SwapFieldsToTargetEndian( &sOutGameLumpHeader );
	}

	pOutBSPHeader->lumps[LUMP_GAME_LUMP].fileofs = newOffset;
	pOutBSPHeader->lumps[LUMP_GAME_LUMP].filelen = outputBuffer.TellPut() - newOffset;
	// We set GAMELUMPFLAG_COMPRESSED and handle compression at the sub-lump level, this whole lump is not
	// decompressable as a block.
	pOutBSPHeader->lumps[LUMP_GAME_LUMP].uncompressedSize = 0;

	// Rewind to start and write lump headers
	unsigned int endOffset = outputBuffer.TellPut();
	outputBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, newOffset );
	outputBuffer.Put( &sOutGameLumpHeader, sizeof( dgamelumpheader_t ) );
	outputBuffer.Put( sOutGameLumpBuf.Base(), sOutGameLumpBuf.TellPut() );
	outputBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, endOffset );

	return true;
}

//-----------------------------------------------------------------------------
// Compress callback for RepackBSP
//-----------------------------------------------------------------------------
bool RepackBSPCallback_LZMA( CUtlBuffer &inputBuffer, CUtlBuffer &outputBuffer )
{
	if ( !inputBuffer.TellPut() )
	{
		// nothing to do
		return false;
	}

	unsigned int originalSize = inputBuffer.TellPut() - inputBuffer.TellGet();
	unsigned int compressedSize = 0;
	unsigned char *pCompressedOutput = LZMA_Compress( (unsigned char *)inputBuffer.Base() + inputBuffer.TellGet(),
													  originalSize, &compressedSize );
	if ( pCompressedOutput )
	{
		outputBuffer.Put( pCompressedOutput, compressedSize );
		DevMsg( "Compressed bsp lump %u -> %u bytes\n", originalSize, compressedSize );
		free( pCompressedOutput );
		return true;
	}

	return false;
}


bool RepackBSP( CUtlBuffer &inputBuffer, CUtlBuffer &outputBuffer, CompressFunc_t pCompressFunc, IZip::eCompressionType packfileCompression )
{
	dheader_t *pInBSPHeader = (dheader_t *)inputBuffer.Base();
	// The 360 swaps this header to disk. For some reason.
	if ( pInBSPHeader->ident != ( IsX360() ? BigLong( IDBSPHEADER ) : IDBSPHEADER ) )
	{
		Warning( "RepackBSP given invalid input data\n" );
		return false;
	}

	CByteswap	byteSwap;
	if ( IsX360() )
	{
		// bsp is 360, swap the header back
		byteSwap.ActivateByteSwapping( true );
		byteSwap.SwapFieldsToTargetEndian( pInBSPHeader );
	}

	unsigned int headerOffset = outputBuffer.TellPut();
	outputBuffer.Put( pInBSPHeader, sizeof( dheader_t ) );

	// This buffer grows dynamically, don't keep pointers to it around. Write out header at end.
	dheader_t sOutBSPHeader = *pInBSPHeader;

	// must adhere to input lump's offset order and process according to that, NOT lump num
	// sort by offset order
	CUtlVector< SortedLump_t > sortedLumps;
	for ( int i = 0; i < HEADER_LUMPS; i++ )
	{
		int iIndex = sortedLumps.AddToTail();
		sortedLumps[iIndex].lumpNum = i;
		sortedLumps[iIndex].pLump = &pInBSPHeader->lumps[i];
	}
	sortedLumps.Sort( SortLumpsByOffset );

	// iterate in sorted order
	for ( int i = 0; i < HEADER_LUMPS; ++i )
	{
		SortedLump_t *pSortedLump = &sortedLumps[i];
		int lumpNum = pSortedLump->lumpNum;

		// Should be set below, don't copy over old data
		sOutBSPHeader.lumps[lumpNum].fileofs = 0;
		sOutBSPHeader.lumps[lumpNum].filelen = 0;
		// Only set by compressed lumps
		sOutBSPHeader.lumps[lumpNum].uncompressedSize = 0;

		if ( pSortedLump->pLump->filelen ) // Otherwise its degenerate
		{
			int alignment = 4;
			if ( lumpNum == LUMP_PAKFILE )
			{
				alignment = 2048;
			}
			unsigned int newOffset = AlignBuffer( outputBuffer, alignment );

			CUtlBuffer inputBuffer;
			if ( pSortedLump->pLump->uncompressedSize )
			{
				byte *pCompressedLump = ((byte *)pInBSPHeader) + pSortedLump->pLump->fileofs;
				if ( CLZMA::IsCompressed( pCompressedLump ) && pSortedLump->pLump->uncompressedSize == CLZMA::GetActualSize( pCompressedLump ) )
				{
					inputBuffer.EnsureCapacity( CLZMA::GetActualSize( pCompressedLump ) );
					unsigned int outSize = CLZMA::Uncompress( pCompressedLump, (unsigned char *)inputBuffer.Base() );
					inputBuffer.SeekPut( CUtlBuffer::SEEK_CURRENT, outSize );
					if ( outSize != pSortedLump->pLump->uncompressedSize )
					{
						Warning( "Decompressed size differs from header, BSP may be corrupt\n" );
					}
				}
				else
				{
					Assert( CLZMA::IsCompressed( pCompressedLump ) &&
					        pSortedLump->pLump->uncompressedSize == CLZMA::GetActualSize( pCompressedLump ) );
					Warning( "Unsupported BSP: Unrecognized compressed lump\n" );
				}
			}
			else
			{
				// Just use input
				inputBuffer.SetExternalBuffer( ((byte *)pInBSPHeader) + pSortedLump->pLump->fileofs,
				                               pSortedLump->pLump->filelen, pSortedLump->pLump->filelen );
			}

			if ( lumpNum == LUMP_GAME_LUMP )
			{
				// the game lump has to have each of its components individually compressed
				CompressGameLump( pInBSPHeader, &sOutBSPHeader, outputBuffer, pCompressFunc );
			}
			else if ( lumpNum == LUMP_PAKFILE )
			{
				IZip *newPakFile = IZip::CreateZip( NULL );
				IZip *oldPakFile = IZip::CreateZip( NULL );
				oldPakFile->ParseFromBuffer( inputBuffer.Base(), inputBuffer.Size() );

				int id = -1;
				int fileSize;
				while ( 1 )
				{
					char relativeName[MAX_PATH];
					id = GetNextFilename( oldPakFile, id, relativeName, sizeof( relativeName ), fileSize );
					if ( id == -1 )
						break;

					CUtlBuffer sourceBuf;
					CUtlBuffer targetBuf;

					bool bOK = ReadFileFromPak( oldPakFile, relativeName, false, sourceBuf );
					if ( !bOK )
					{
						Error( "Failed to load '%s' from lump pak for repacking.\n", relativeName );
						continue;
					}

					AddBufferToPak( newPakFile, relativeName, sourceBuf.Base(), sourceBuf.TellMaxPut(), false, packfileCompression );

					DevMsg( "Repacking BSP: Created '%s' in lump pak\n", relativeName );
				}

				// save new pack to buffer
				newPakFile->SaveToBuffer( outputBuffer );
				sOutBSPHeader.lumps[lumpNum].fileofs = newOffset;
				sOutBSPHeader.lumps[lumpNum].filelen = outputBuffer.TellPut() - newOffset;
				// Note that this *lump* is uncompressed, it just contains a packfile that uses compression, so we're
				// not setting lumps[lumpNum].uncompressedSize

				IZip::ReleaseZip( oldPakFile );
				IZip::ReleaseZip( newPakFile );
			}
			else
			{
				CUtlBuffer compressedBuffer;
				bool bCompressed = pCompressFunc ? pCompressFunc( inputBuffer, compressedBuffer ) : false;
				if ( bCompressed )
				{
					sOutBSPHeader.lumps[lumpNum].uncompressedSize = inputBuffer.TellPut();
					sOutBSPHeader.lumps[lumpNum].filelen = compressedBuffer.TellPut();
					sOutBSPHeader.lumps[lumpNum].fileofs = newOffset;
					outputBuffer.Put( compressedBuffer.Base(), compressedBuffer.TellPut() );
					compressedBuffer.Purge();
				}
				else
				{
					// add as is
					sOutBSPHeader.lumps[lumpNum].fileofs = newOffset;
					sOutBSPHeader.lumps[lumpNum].filelen = inputBuffer.TellPut();
					outputBuffer.Put( inputBuffer.Base(), inputBuffer.TellPut() );
				}
			}
		}
	}

	if ( IsX360() )
	{
		// fix the output for 360, swapping it back
		byteSwap.SetTargetBigEndian( true );
		byteSwap.SwapFieldsToTargetEndian( &sOutBSPHeader );
	}

	// Write out header
	unsigned int endOffset = outputBuffer.TellPut();
	outputBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, headerOffset );
	outputBuffer.Put( &sOutBSPHeader, sizeof( sOutBSPHeader ) );
	outputBuffer.SeekPut( CUtlBuffer::SEEK_HEAD, endOffset );

	return true;
}

//-----------------------------------------------------------------------------
//  For all lumps in a bsp: Loads the lump from file A, swaps it, writes it to file B.
//  This limits the memory used for the swap process which helps the Xbox 360.
//
//	NOTE: These lumps will be written to the file in exactly the order they appear here,
//	so they can be shifted around if desired for file access optimization.
//-----------------------------------------------------------------------------
bool SwapBSPFile( const char *pInFilename, const char *pOutFilename, bool bSwapOnLoad, VTFConvertFunc_t pVTFConvertFunc, VHVFixupFunc_t pVHVFixupFunc, CompressFunc_t pCompressFunc )
{
	DevMsg( "Creating %s\n", pOutFilename );

	if ( !g_pFileSystem->FileExists( pInFilename ) )
	{
		Warning( "Error! Couldn't open input file %s - BSP swap failed!\n", pInFilename ); 
		return false;
	}

	g_hBSPFile = SafeOpenWrite( pOutFilename );
	if ( !g_hBSPFile )
	{
		Warning( "Error! Couldn't open output file %s - BSP swap failed!\n", pOutFilename ); 
		return false;
	}

	if ( !pVTFConvertFunc )
	{
		Warning( "Error! Missing VTF Conversion function\n" ); 
		return false;
	}
	g_pVTFConvertFunc = pVTFConvertFunc;

	// optional VHV fixup
	g_pVHVFixupFunc = pVHVFixupFunc;

	// optional compression callback
	g_pCompressFunc = pCompressFunc;

	// These must be mutually exclusive
	g_bSwapOnLoad = bSwapOnLoad;
	g_bSwapOnWrite = !bSwapOnLoad;

	g_Swap.ActivateByteSwapping( true );

	OpenBSPFile( pInFilename );

	// CRC the bsp first
	CRC32_t mapCRC;
	CRC32_Init(&mapCRC);
	if ( !CRC_MapFile( &mapCRC, pInFilename ) )
	{
		Warning( "Failed to CRC the bsp\n" );
		return false;
	}

	// hold a dictionary of all the static prop names
	// this is needed to properly convert any VHV files inside the pak lump
	BuildStaticPropNameTable();

	// Set the output file pointer after the header
	dheader_t dummyHeader = { 0 };
	SafeWrite( g_hBSPFile, &dummyHeader, sizeof( dheader_t ) );

	// To allow for alignment fixups, the lumps will be written to the
	// output file in the order they appear in this function.

	// NOTE: Flags for 360 !!!MUST!!! be first	
	SwapLumpToDisk< dflagslump_t >( LUMP_MAP_FLAGS );

	// complex lump swaps first or for later contingent data
	SwapLeafLumpToDisk();
	SwapOcclusionLumpToDisk();
	SwapGameLumpsToDisk();

	// Strip dead or non relevant lumps
	g_pBSPHeader->lumps[LUMP_DISP_LIGHTMAP_ALPHAS].filelen = 0;
	g_pBSPHeader->lumps[LUMP_FACEIDS].filelen = 0;

	// Strip obsolete LDR in favor of HDR
	if ( SwapLumpToDisk<dface_t>( LUMP_FACES_HDR ) )
	{
		g_pBSPHeader->lumps[LUMP_FACES].filelen = 0;
	}
	else
	{
		// no HDR, keep LDR version
		SwapLumpToDisk<dface_t>( LUMP_FACES );
	}

	if ( SwapLumpToDisk<dworldlight_t>( LUMP_WORLDLIGHTS_HDR ) )
	{
		g_pBSPHeader->lumps[LUMP_WORLDLIGHTS].filelen = 0;
	}
	else
	{
		// no HDR, keep LDR version
		SwapLumpToDisk<dworldlight_t>( LUMP_WORLDLIGHTS );
	}

	// Simple lump swaps
	SwapLumpToDisk<byte>( FIELD_CHARACTER, LUMP_PHYSDISP );
	SwapLumpToDisk<byte>( FIELD_CHARACTER, LUMP_PHYSCOLLIDE );
	SwapLumpToDisk<byte>( FIELD_CHARACTER, LUMP_VISIBILITY );
	SwapLumpToDisk<dmodel_t>( LUMP_MODELS );
	SwapLumpToDisk<dvertex_t>( LUMP_VERTEXES );
	SwapLumpToDisk<dplane_t>( LUMP_PLANES );
	SwapLumpToDisk<dnode_t>( LUMP_NODES );
	SwapLumpToDisk<texinfo_t>( LUMP_TEXINFO );
	SwapLumpToDisk<dtexdata_t>( LUMP_TEXDATA );
	SwapLumpToDisk<ddispinfo_t>( LUMP_DISPINFO );
    SwapLumpToDisk<CDispVert>( LUMP_DISP_VERTS );
	SwapLumpToDisk<CDispTri>( LUMP_DISP_TRIS );
    SwapLumpToDisk<char>( FIELD_CHARACTER, LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS );
	SwapLumpToDisk<CFaceMacroTextureInfo>( LUMP_FACE_MACRO_TEXTURE_INFO );
	SwapLumpToDisk<dprimitive_t>( LUMP_PRIMITIVES );
	SwapLumpToDisk<dprimvert_t>( LUMP_PRIMVERTS );
	SwapLumpToDisk<unsigned short>( FIELD_SHORT, LUMP_PRIMINDICES );
    SwapLumpToDisk<dface_t>( LUMP_ORIGINALFACES );
	SwapLumpToDisk<unsigned short>( FIELD_SHORT, LUMP_LEAFFACES );
	SwapLumpToDisk<unsigned short>( FIELD_SHORT, LUMP_LEAFBRUSHES );
	SwapLumpToDisk<int>( FIELD_INTEGER, LUMP_SURFEDGES );
	SwapLumpToDisk<dedge_t>( LUMP_EDGES );
	SwapLumpToDisk<dbrush_t>( LUMP_BRUSHES );
	SwapLumpToDisk<dbrushside_t>( LUMP_BRUSHSIDES );
	SwapLumpToDisk<darea_t>( LUMP_AREAS );
	SwapLumpToDisk<dareaportal_t>( LUMP_AREAPORTALS );
	SwapLumpToDisk<char>( FIELD_CHARACTER, LUMP_ENTITIES );
	SwapLumpToDisk<dleafwaterdata_t>( LUMP_LEAFWATERDATA );
	SwapLumpToDisk<float>( FIELD_VECTOR, LUMP_VERTNORMALS );
	SwapLumpToDisk<short>( FIELD_SHORT, LUMP_VERTNORMALINDICES );
	SwapLumpToDisk<float>( FIELD_VECTOR, LUMP_CLIPPORTALVERTS );
	SwapLumpToDisk<dcubemapsample_t>( LUMP_CUBEMAPS );	
	SwapLumpToDisk<char>( FIELD_CHARACTER, LUMP_TEXDATA_STRING_DATA );
	SwapLumpToDisk<int>( FIELD_INTEGER, LUMP_TEXDATA_STRING_TABLE );
	SwapLumpToDisk<doverlay_t>( LUMP_OVERLAYS );
	SwapLumpToDisk<dwateroverlay_t>( LUMP_WATEROVERLAYS );
	SwapLumpToDisk<unsigned short>( FIELD_SHORT, LUMP_LEAFMINDISTTOWATER );
	SwapLumpToDisk<doverlayfade_t>( LUMP_OVERLAY_FADES );


	// NOTE: this data placed at the end for the sake of 360:
	{
		// NOTE: lighting must be the penultimate lump
		//       (allows 360 to free this memory part-way through map loading)
		if ( SwapLumpToDisk<byte>( FIELD_CHARACTER, LUMP_LIGHTING_HDR ) )
		{
			g_pBSPHeader->lumps[LUMP_LIGHTING].filelen = 0;
		}
		else
		{
			// no HDR, keep LDR version
			SwapLumpToDisk<byte>( FIELD_CHARACTER, LUMP_LIGHTING );
		}
		// NOTE: Pakfile for 360 !!!MUST!!! be last	
		SwapPakfileLumpToDisk( pInFilename );
	}


	// Store the crc in the flags lump version field
	g_pBSPHeader->lumps[LUMP_MAP_FLAGS].version = mapCRC;

	// Pad out the end of the file to a sector boundary for optimal IO
	AlignFilePosition( g_hBSPFile, XBOX_DVD_SECTORSIZE );

	// Warn of any lumps that didn't get swapped
	for ( int i = 0; i < HEADER_LUMPS; ++i )
	{
		if ( HasLump( i ) && !g_Lumps.bLumpParsed[i] )
		{
			// a new lump got added that needs to have a swap function
			Warning( "BSP: '%s', %s has no swap or copy function. Discarding!\n", pInFilename, GetLumpName(i) );

			// the data didn't get copied, so don't reference garbage
			g_pBSPHeader->lumps[i].filelen = 0;
		}
	}

	// Write the updated header
	g_pFileSystem->Seek( g_hBSPFile, 0, FILESYSTEM_SEEK_HEAD );
	WriteData( g_pBSPHeader );
	g_pFileSystem->Close( g_hBSPFile );
	g_hBSPFile = 0;

	// Cleanup
	g_Swap.ActivateByteSwapping( false );

	CloseBSPFile();

	g_StaticPropNames.Purge();
	g_StaticPropInstances.Purge();

	DevMsg( "Finished BSP Swap\n" );

	// caller provided compress func will further compress compatible lumps
	if ( pCompressFunc )
	{
		CUtlBuffer inputBuffer;
		if ( !g_pFileSystem->ReadFile( pOutFilename, NULL, inputBuffer ) )
		{
			Warning( "Error! Couldn't read file %s - final BSP compression failed!\n", pOutFilename ); 
			return false;
		}

		CUtlBuffer outputBuffer;
		if ( !RepackBSP( inputBuffer, outputBuffer, pCompressFunc, IZip::eCompressionType_None ) )
		{
			Warning( "Error! Failed to compress BSP '%s'!\n", pOutFilename );
			return false;
		}

		g_hBSPFile = SafeOpenWrite( pOutFilename );
		if ( !g_hBSPFile )
		{
			Warning( "Error! Couldn't open output file %s - BSP swap failed!\n", pOutFilename ); 
			return false;
		}
		SafeWrite( g_hBSPFile, outputBuffer.Base(), outputBuffer.TellPut() );
		g_pFileSystem->Close( g_hBSPFile );
		g_hBSPFile = 0;			
	}

	return true;
}

//-----------------------------------------------------------------------------
// Get the pak lump from a BSP
//-----------------------------------------------------------------------------
bool GetPakFileLump( const char *pBSPFilename, void **pPakData, int *pPakSize )
{
	*pPakData = NULL;
	*pPakSize = 0;

	if ( !g_pFileSystem->FileExists( pBSPFilename ) )
	{
		Warning( "Error! Couldn't open file %s!\n", pBSPFilename ); 
		return false;
	}

	// determine endian nature
	dheader_t *pHeader;
	LoadFile( pBSPFilename, (void **)&pHeader );
	bool bSwap = ( pHeader->ident == BigLong( IDBSPHEADER ) );
	free( pHeader );

	g_bSwapOnLoad = bSwap;
	g_bSwapOnWrite = !bSwap;

	OpenBSPFile( pBSPFilename );
	
	if ( g_pBSPHeader->lumps[LUMP_PAKFILE].filelen )
	{
		*pPakSize = CopyVariableLump<byte>( FIELD_CHARACTER, LUMP_PAKFILE, pPakData );
	}

	CloseBSPFile();

	return true;
}

// compare function for qsort below
static int LumpOffsetCompare( const void *pElem1, const void *pElem2 )
{
	int lump1 = *(byte *)pElem1;
	int lump2 = *(byte *)pElem2;

	if ( lump1 != lump2 )
	{
		// force LUMP_MAP_FLAGS to be first, always
		if ( lump1 == LUMP_MAP_FLAGS )
		{
			return -1;
		}
		else if ( lump2 == LUMP_MAP_FLAGS )
		{
			return 1;
		}

		// force LUMP_PAKFILE to be last, always
		if ( lump1 == LUMP_PAKFILE )
		{
			return 1;
		}
		else if ( lump2 == LUMP_PAKFILE )
		{
			return -1;
		}
	}

	int fileOffset1 = g_pBSPHeader->lumps[lump1].fileofs;
	int fileOffset2 = g_pBSPHeader->lumps[lump2].fileofs;

	// invalid or empty lumps will get sorted together
	if ( !g_pBSPHeader->lumps[lump1].filelen )
	{
		fileOffset1 = 0;
	}

	if ( !g_pBSPHeader->lumps[lump2].filelen )
	{
		fileOffset2 = 0;
	}

	// compare by offset
	if ( fileOffset1 < fileOffset2 )
	{
		return -1;
	}
	else if ( fileOffset1 > fileOffset2 )
	{
		return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Replace the pak lump in a BSP
//-----------------------------------------------------------------------------
bool SetPakFileLump( const char *pBSPFilename, const char *pNewFilename, void *pPakData, int pakSize )
{
	if ( !g_pFileSystem->FileExists( pBSPFilename ) )
	{
		Warning( "Error! Couldn't open file %s!\n", pBSPFilename ); 
		return false;
	}

	// determine endian nature
	dheader_t *pHeader;
	LoadFile( pBSPFilename, (void **)&pHeader );
	bool bSwap = ( pHeader->ident == BigLong( IDBSPHEADER ) );
	free( pHeader );

	g_bSwapOnLoad = bSwap;
	g_bSwapOnWrite = bSwap;

	OpenBSPFile( pBSPFilename );

	// save a copy of the old header
	// generating a new bsp is a destructive operation
	dheader_t oldHeader;
	oldHeader = *g_pBSPHeader;

	g_hBSPFile = SafeOpenWrite( pNewFilename );
	if ( !g_hBSPFile )
	{
		return false;
	}

	// placeholder only, reset at conclusion
	WriteData( &oldHeader );

	// lumps must be reserialized in same relative offset order
	// build sorted order table
	int readOrder[HEADER_LUMPS];
	for ( int i=0; i<HEADER_LUMPS; i++ )
	{
		readOrder[i] = i;
	}
	qsort( readOrder, HEADER_LUMPS, sizeof( int ), LumpOffsetCompare );

	for ( int i = 0; i < HEADER_LUMPS; i++ )
	{
		int lump = readOrder[i];

		if ( lump == LUMP_PAKFILE )
		{
			// pak lump always written last, with special alignment
			continue;
		}

		int length = g_pBSPHeader->lumps[lump].filelen;
		if ( length )
		{
			// save the lump data
			int offset = g_pBSPHeader->lumps[lump].fileofs;
			SetAlignedLumpPosition( lump );
			SafeWrite( g_hBSPFile, (byte *)g_pBSPHeader + offset, length );
		}
		else
		{
			g_pBSPHeader->lumps[lump].fileofs = 0;
		}
	}

	// Always write the pak file at the end
	// Pad out the end of the file to a sector boundary for optimal IO
	g_pBSPHeader->lumps[LUMP_PAKFILE].fileofs = AlignFilePosition( g_hBSPFile, XBOX_DVD_SECTORSIZE );
	g_pBSPHeader->lumps[LUMP_PAKFILE].filelen = pakSize;
	SafeWrite( g_hBSPFile, pPakData, pakSize );

	// Pad out the end of the file to a sector boundary for optimal IO
	AlignFilePosition( g_hBSPFile, XBOX_DVD_SECTORSIZE );

	// Write the updated header
	g_pFileSystem->Seek( g_hBSPFile, 0, FILESYSTEM_SEEK_HEAD );
	WriteData( g_pBSPHeader );
	g_pFileSystem->Close( g_hBSPFile );

	CloseBSPFile();
	
	return true;
}

//-----------------------------------------------------------------------------
// Build a list of files that BSP owns, world/cubemap materials, static props, etc.
//-----------------------------------------------------------------------------
bool GetBSPDependants( const char *pBSPFilename, CUtlVector< CUtlString > *pList )
{
	if ( !g_pFileSystem->FileExists( pBSPFilename ) )
	{
		Warning( "Error! Couldn't open file %s!\n", pBSPFilename ); 
		return false;
	}

	// must be set, but exact hdr not critical for dependant traversal	
	SetHDRMode( false );

	LoadBSPFile( pBSPFilename );

	char szBspName[MAX_PATH];
	V_FileBase( pBSPFilename, szBspName, sizeof( szBspName ) );
	V_SetExtension( szBspName, ".bsp", sizeof( szBspName ) );

	// get embedded pak files, and internals
	char szFilename[MAX_PATH];
	int fileSize;
	int fileId = -1;
	for ( ;; )
	{
		fileId = GetPakFile()->GetNextFilename( fileId, szFilename, sizeof( szFilename ), fileSize );
		if ( fileId == -1 )
		{
			break;
		}
		pList->AddToTail( szFilename );
	}

	// get all the world materials
	for ( int i=0; i<numtexdata; i++ )
	{
		const char *pName = TexDataStringTable_GetString( dtexdata[i].nameStringTableID );
		V_ComposeFileName( "materials", pName, szFilename, sizeof( szFilename ) );
		V_SetExtension( szFilename, ".vmt", sizeof( szFilename ) );
		pList->AddToTail( szFilename );
	}

	// get all the static props
	GameLumpHandle_t hGameLump = g_GameLumps.GetGameLumpHandle( GAMELUMP_STATIC_PROPS );
	if ( hGameLump != g_GameLumps.InvalidGameLump() )
	{
		byte *pGameLumpData = (byte *)g_GameLumps.GetGameLump( hGameLump );
		if ( pGameLumpData && g_GameLumps.GameLumpSize( hGameLump ) )
		{
			int count = ((int *)pGameLumpData)[0];
			pGameLumpData += sizeof( int );

			StaticPropDictLump_t *pStaticPropDictLump = (StaticPropDictLump_t *)pGameLumpData;
			for ( int i=0; i<count; i++ )
			{
				pList->AddToTail( pStaticPropDictLump[i].m_Name );
			}
		}
	}

	// get all the detail props
	hGameLump = g_GameLumps.GetGameLumpHandle( GAMELUMP_DETAIL_PROPS );
	if ( hGameLump != g_GameLumps.InvalidGameLump() )
	{
		byte *pGameLumpData = (byte *)g_GameLumps.GetGameLump( hGameLump );
		if ( pGameLumpData && g_GameLumps.GameLumpSize( hGameLump ) )
		{
			int count = ((int *)pGameLumpData)[0];
			pGameLumpData += sizeof( int );

			DetailObjectDictLump_t *pDetailObjectDictLump = (DetailObjectDictLump_t *)pGameLumpData;
			for ( int i=0; i<count; i++ )
			{
				pList->AddToTail( pDetailObjectDictLump[i].m_Name );
			}
			pGameLumpData += count * sizeof( DetailObjectDictLump_t );

			if ( g_GameLumps.GetGameLumpVersion( hGameLump ) == 4 )
			{
				count = ((int *)pGameLumpData)[0];
				pGameLumpData += sizeof( int );
				if ( count )
				{
					// All detail prop sprites must lie in the material detail/detailsprites
					pList->AddToTail( "materials/detail/detailsprites.vmt" );
				}
			}
		}
	}

	UnloadBSPFile();

	return true;
}

