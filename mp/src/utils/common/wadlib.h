//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

// wadlib.h

//
// wad reading
//

#define	CMP_NONE		0
#define	CMP_LZSS		1

#define	TYP_NONE		0
#define	TYP_LABEL		1
#define	TYP_LUMPY		64				// 64 + grab command number

#ifndef WADTYPES_H
#include "wadtypes.h"
#endif

extern	lumpinfo_t		*lumpinfo;		// location of each lump on disk
extern	int				numlumps;
extern	wadinfo_t		header;

void	W_OpenWad (char *filename);
int		W_CheckNumForName (char *name);
int		W_GetNumForName (char *name);
int		W_LumpLength (int lump);
void	W_ReadLumpNum (int lump, void *dest);
void	*W_LoadLumpNum (int lump);
void	*W_LoadLumpName (char *name);

void CleanupName (char *in, char *out);

//
// wad creation
//
void	NewWad (char *pathname, qboolean bigendien);
void	AddLump (char *name, void *buffer, int length, int type, int compress);
void	WriteWad (int wad3);

