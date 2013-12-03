//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// wad2lib.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/file.h>
#include <stdarg.h>

#ifdef NeXT
#include <libc.h>
#endif
#include "cmdlib.h"
#include "wadlib.h"
#include "commonmacros.h"

/*
============================================================================

							WAD READING

============================================================================
*/


lumpinfo_t		*lumpinfo;		// location of each lump on disk
int				numlumps;

wadinfo_t		header;
FILE			*wadhandle;


/*
====================
W_OpenWad
====================
*/
void W_OpenWad (char *filename)
{
	lumpinfo_t		*lump_p;
	unsigned		i;
	int				length;
	
//
// open the file and add to directory
//	
	wadhandle = SafeOpenRead (filename);
	SafeRead (wadhandle, &header, sizeof(header));

	if (!STRING_MATCHES_ID(header.identification,WAD_ID))
		Error ("Wad file %s doesn't have %s identifier\n",filename, WAD_IDNAME);
		
	header.numlumps = LittleLong(header.numlumps);
	header.infotableofs = LittleLong(header.infotableofs);

	numlumps = header.numlumps;

	length = numlumps*sizeof(lumpinfo_t);
	lumpinfo = malloc (length);
	lump_p = lumpinfo;
	
	fseek (wadhandle, header.infotableofs, SEEK_SET);
	SafeRead (wadhandle, lumpinfo, length);

//
// Fill in lumpinfo
//
	
	for (i=0 ; i<numlumps ; i++,lump_p++)
	{
		lump_p->filepos = LittleLong(lump_p->filepos);
		lump_p->size = LittleLong(lump_p->size);
	}
}



void CleanupName (char *in, char *out)
{
	int		i;
	
	for (i=0 ; i<sizeof( ((lumpinfo_t *)0)->name ) ; i++ )
	{
		if (!in[i])
			break;
			
		out[i] = toupper(in[i]);
	}
	
	for ( ; i<sizeof( ((lumpinfo_t *)0)->name ); i++ )
		out[i] = 0;
}


/*
====================
W_CheckNumForName

Returns -1 if name not found
====================
*/
int	W_CheckNumForName (char *name)
{
	char	cleanname[TEXTURE_NAME_LENGTH];
	int		v1,v2, v3, v4;
	int		i;
	lumpinfo_t	*lump_p;
	
	CleanupName (name, cleanname);
	
// make the name into four integers for easy compares

	v1 = *(int *)cleanname;
	v2 = *(int *)&cleanname[4];
	v3 = *(int *)&cleanname[8];
	v4 = *(int *)&cleanname[12];

// find it

	lump_p = lumpinfo;
	for (i=0 ; i<numlumps ; i++, lump_p++)
	{
		if ( *(int *)lump_p->name == v1
		&& *(int *)&lump_p->name[4] == v2
		&& *(int *)&lump_p->name[8] == v3
		&& *(int *)&lump_p->name[12] == v4
		&& !strcmp( lump_p->name, cleanname ) )
			return i;
	}

	return -1;
}


/*
====================
W_GetNumForName

Calls W_CheckNumForName, but bombs out if not found
====================
*/
int	W_GetNumForName (char *name)
{
	int	i;

	i = W_CheckNumForName (name);
	if (i != -1)
		return i;

	Error ("W_GetNumForName: %s not found!",name);
	return -1;
}


/*
====================
W_LumpLength

Returns the buffer size needed to load the given lump
====================
*/
int W_LumpLength (int lump)
{
	if (lump >= numlumps)
		Error ("W_LumpLength: %i >= numlumps",lump);
	return lumpinfo[lump].size;
}


/*
====================
W_ReadLumpNum

Loads the lump into the given buffer, which must be >= W_LumpLength()
====================
*/
void W_ReadLumpNum (int lump, void *dest)
{
	lumpinfo_t	*l;
	
	if (lump >= numlumps)
		Error ("W_ReadLump: %i >= numlumps",lump);
	l = lumpinfo+lump;
	
	fseek (wadhandle, l->filepos, SEEK_SET);
	SafeRead (wadhandle, dest, l->size);
}



/*
====================
W_LoadLumpNum
====================
*/
void	*W_LoadLumpNum (int lump)
{
	void	*buf;
	
	if ((unsigned)lump >= numlumps)
		Error ("W_CacheLumpNum: %i >= numlumps",lump);
		
	buf = malloc (W_LumpLength (lump));
	W_ReadLumpNum (lump, buf);
	
	return buf;
}


/*
====================
W_LoadLumpName
====================
*/
void	*W_LoadLumpName (char *name)
{
	return W_LoadLumpNum (W_GetNumForName(name));
}


/*
===============================================================================

						WAD CREATION

===============================================================================
*/

FILE		*outwad;

lumpinfo_t	outinfo[4096];
int			outlumps;

short	(*wadshort) (short l);
int		(*wadlong) (int l);

/*
===============
NewWad
===============
*/

void NewWad (char *pathname, qboolean bigendien)
{
	outwad = SafeOpenWrite (pathname);
	fseek (outwad, sizeof(wadinfo_t), SEEK_SET);
	memset (outinfo, 0, sizeof(outinfo));
	
	if (bigendien)
	{
		wadshort = BigShort;
		wadlong = BigLong;
	}
	else
	{
		wadshort = LittleShort;
		wadlong = LittleLong;
	}
	
	outlumps = 0;
}


/*
===============
AddLump
===============
*/

void	AddLump (char *name, void *buffer, int length, int type, int compress)
{
	lumpinfo_t	*info;
	int			ofs;
	
	info = &outinfo[outlumps];
	outlumps++;

	memset (info,0,sizeof(info));
	
	strcpy (info->name, name);
	Q_strupr (info->name);
	
	ofs = ftell(outwad);
	info->filepos = wadlong(ofs);
	info->size = info->disksize = wadlong(length);
	info->type = type;
	info->compression = compress;
	
// FIXME: do compression

	SafeWrite (outwad, buffer, length);
}


/*
===============
WriteWad
===============
*/

void WriteWad (int wad3)
{
	wadinfo_t	header;
	int			ofs;
	
// write the lumpingo
	ofs = ftell(outwad);

	SafeWrite (outwad, outinfo, outlumps*sizeof(lumpinfo_t) );
		
// write the header

// a program will be able to tell the ednieness of a wad by the id
	ID_TO_STRING( WAD_ID, header.identification );
	
	header.numlumps = wadlong(outlumps);
	header.infotableofs = wadlong(ofs);
		
	fseek (outwad, 0, SEEK_SET);
	SafeWrite (outwad, &header, sizeof(header));
	fclose (outwad);
}


