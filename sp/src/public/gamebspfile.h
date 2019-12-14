//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines game-specific data
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef GAMEBSPFILE_H
#define GAMEBSPFILE_H

#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"
#include "basetypes.h"


//-----------------------------------------------------------------------------
// This enumerations defines all the four-CC codes for the client lump names
//-----------------------------------------------------------------------------
// TODO: We may have some endian considerations here!
#define GAMELUMP_MAKE_CODE(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d) << 0)
enum
{
	GAMELUMP_DETAIL_PROPS = GAMELUMP_MAKE_CODE('d', 'p', 'r', 'p'),
	GAMELUMP_DETAIL_PROP_LIGHTING = GAMELUMP_MAKE_CODE('d', 'p', 'l', 't'),
	GAMELUMP_STATIC_PROPS = GAMELUMP_MAKE_CODE('s', 'p', 'r', 'p'),
	GAMELUMP_DETAIL_PROP_LIGHTING_HDR = GAMELUMP_MAKE_CODE('d', 'p', 'l', 'h'),
};

// Versions...
enum
{
	GAMELUMP_DETAIL_PROPS_VERSION = 4,
	GAMELUMP_DETAIL_PROP_LIGHTING_VERSION = 0,
	GAMELUMP_STATIC_PROPS_VERSION = 6,
	GAMELUMP_STATIC_PROP_LIGHTING_VERSION = 0,
	GAMELUMP_DETAIL_PROP_LIGHTING_HDR_VERSION = 0,
};


//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_DETAIL_PROPS lump
//-----------------------------------------------------------------------------
#define DETAIL_NAME_LENGTH 128

enum DetailPropOrientation_t
{
	DETAIL_PROP_ORIENT_NORMAL = 0,
	DETAIL_PROP_ORIENT_SCREEN_ALIGNED,
	DETAIL_PROP_ORIENT_SCREEN_ALIGNED_VERTICAL,
};

// NOTE: If DetailPropType_t enum changes, change CDetailModel::QuadsToDraw
// in detailobjectsystem.cpp
enum DetailPropType_t
{
	DETAIL_PROP_TYPE_MODEL = 0,
	DETAIL_PROP_TYPE_SPRITE,
	DETAIL_PROP_TYPE_SHAPE_CROSS,
	DETAIL_PROP_TYPE_SHAPE_TRI,
};

//-----------------------------------------------------------------------------
// Model index when using studiomdls for detail props
//-----------------------------------------------------------------------------
struct DetailObjectDictLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	char	m_Name[DETAIL_NAME_LENGTH];		// model name
};

//-----------------------------------------------------------------------------
// Information about the sprite to render
//-----------------------------------------------------------------------------
struct DetailSpriteDictLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	// NOTE: All detail prop sprites must lie in the material detail/detailsprites
	Vector2D	m_UL;		// Coordinate of upper left 
	Vector2D	m_LR;		// Coordinate of lower right
	Vector2D	m_TexUL;	// Texcoords of upper left
	Vector2D	m_TexLR;	// Texcoords of lower left
};

struct DetailObjectLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_DetailModel;		// either index into DetailObjectDictLump_t or DetailPropSpriteLump_t
	unsigned short	m_Leaf;
	ColorRGBExp32	m_Lighting;
	unsigned int	m_LightStyles; 
	unsigned char	m_LightStyleCount;
	unsigned char   m_SwayAmount;		// how much do the details sway
	unsigned char	m_ShapeAngle;		// angle param for shaped sprites
	unsigned char   m_ShapeSize;		// size param for shaped sprites
	unsigned char	m_Orientation;		// See DetailPropOrientation_t
	unsigned char	m_Padding2[3];		// FIXME: Remove when we rev the detail lump again..
	unsigned char	m_Type;				// See DetailPropType_t
	unsigned char	m_Padding3[3];		// FIXME: Remove when we rev the detail lump again..
	float			m_flScale;			// For sprites only currently
};

//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_DETAIL_PROP_LIGHTING lump
//-----------------------------------------------------------------------------
struct DetailPropLightstylesLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	ColorRGBExp32	m_Lighting;
	unsigned char	m_Style;
};

//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_STATIC_PROPS lump
//-----------------------------------------------------------------------------
enum
{
	STATIC_PROP_NAME_LENGTH  = 128,

	// Flags field
	// These are automatically computed
	STATIC_PROP_FLAG_FADES	= 0x1,
	STATIC_PROP_USE_LIGHTING_ORIGIN	= 0x2,
	STATIC_PROP_NO_DRAW = 0x4,	// computed at run time based on dx level

	// These are set in WC
	STATIC_PROP_IGNORE_NORMALS	= 0x8,
	STATIC_PROP_NO_SHADOW	= 0x10,
	STATIC_PROP_SCREEN_SPACE_FADE	= 0x20,

	STATIC_PROP_NO_PER_VERTEX_LIGHTING = 0x40,				// in vrad, compute lighting at
															// lighting origin, not for each vertex
	
	STATIC_PROP_NO_SELF_SHADOWING = 0x80,					// disable self shadowing in vrad

	STATIC_PROP_WC_MASK		= 0xd8,							// all flags settable in hammer (?)
};

struct StaticPropDictLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	char	m_Name[STATIC_PROP_NAME_LENGTH];		// model name
};

struct StaticPropLumpV4_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned short	m_FirstLeaf;
	unsigned short	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
};

struct StaticPropLumpV5_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned short	m_FirstLeaf;
	unsigned short	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
	float			m_flForcedFadeScale;
//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
};

struct StaticPropLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	Vector			m_Origin;
	QAngle			m_Angles;
	unsigned short	m_PropType;
	unsigned short	m_FirstLeaf;
	unsigned short	m_LeafCount;
	unsigned char	m_Solid;
	unsigned char	m_Flags;
	int				m_Skin;
	float			m_FadeMinDist;
	float			m_FadeMaxDist;
	Vector			m_LightingOrigin;
	float			m_flForcedFadeScale;
	unsigned short	m_nMinDXLevel;
	unsigned short	m_nMaxDXLevel;
	//	int				m_Lighting;			// index into the GAMELUMP_STATIC_PROP_LIGHTING lump
};

struct StaticPropLeafLump_t
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short	m_Leaf;
};

//-----------------------------------------------------------------------------
// This is the data associated with the GAMELUMP_STATIC_PROP_LIGHTING lump
//-----------------------------------------------------------------------------
struct StaticPropLightstylesLump_t
{
	ColorRGBExp32	m_Lighting;
};

#endif // GAMEBSPFILE_H
