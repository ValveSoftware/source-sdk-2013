//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef WADTYPES_H
#define WADTYPES_H
#pragma once

#include "basetypes.h"

// ---------------------------------------------------------
//
// This should include ONLY types (no function prototypes) 
// in or related to WADs that are shared among the engine
// and tools
//
// ---------------------------------------------------------

//===============
//   TYPES
//===============

#define	CMP_NONE		0
#define	CMP_LZSS		1

#define	TYP_NONE		0
#define	TYP_LABEL		1

#define	TYP_LUMPY		64
#define	TYP_PALETTE		64
#define	TYP_COLORMAP	65
#define	TYP_QPIC		66
#define	TYP_MIPTEX		67
#define	TYP_RAW			68
#define	TYP_COLORMAP2	69
#define	TYP_FONT		70
#define	TYP_SOUND		71
#define	TYP_QTEX		72


#define __QPIC_T
typedef struct qpic_s
{
	int			width, height;
	byte		data[4];			// variably sized
} qpic_t;

#define WAD_ID				MAKEID( 'W', 'A', 'D', '5' )
#define WAD_IDNAME			"WAD5"

typedef struct wadinfo_s
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
} wadinfo_t;

#ifndef TEXTURE_NAME_LENGTH
#define TEXTURE_NAME_LENGTH	128
#endif

typedef struct lumpinfo_s
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[TEXTURE_NAME_LENGTH];				// must be null terminated
} lumpinfo_t;

typedef struct
{
	lumpinfo_t	lump;
	int			iTexFile;	// index of the wad this texture is located in
} texlumpinfo_t;

#define MIPLEVELS		4
typedef struct miptex_s					// New WAD4 files
{
	char		name[TEXTURE_NAME_LENGTH];
	unsigned	width, height, view_width, view_height;
	float		reflectivity[3]; 		// Linear palette
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} miptex_t;


#define MAX_TEXTUREWIDTH	1024
#define MAX_TEXTUREHEIGHT	1024
#define MAX_TEXTURESIZE		(MAX_TEXTUREWIDTH*MAX_TEXTUREHEIGHT)
#define MAXLUMP		(((MAX_TEXTURESIZE * (64+16+4+1))/64) + sizeof(miptex_t) + 1024)         // biggest possible lump


#endif		// WADTYPES_H

