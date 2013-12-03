//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IVRADDLL_H
#define IVRADDLL_H
#ifdef _WIN32
#pragma once
#endif


#include "interface.h"
#include "bspfile.h"


#define VRAD_INTERFACE_VERSION "vraddll_1"


class CBSPInfo
{
public:
	byte			*dlightdata;
	int				lightdatasize;

	dface_t			*dfaces;
	unsigned char	*m_pFacesTouched;	// If non-null, then this has 1 byte for each face and
										// tells which faces had their lightmaps updated.										
	int				numfaces;
	
	dvertex_t		*dvertexes;
	int				numvertexes;

	dedge_t			*dedges;
	int				numedges;

	int				*dsurfedges;
	int				numsurfedges;

	texinfo_t		*texinfo;
	int				numtexinfo;

	dtexdata_t		*dtexdata;
	int				numtexdata;

	ddispinfo_t		*g_dispinfo;
	int				g_numdispinfo;

	char				*texDataStringData;
	int					nTexDataStringData;

	int					*texDataStringTable;
	int					nTexDataStringTable;
};


// This is the DLL interface to VRAD.
class IVRadDLL
{
public:
	// All vrad.exe does is load the VRAD DLL and run this.
	virtual int			main( int argc, char **argv ) = 0;
	
	
	// Load the BSP file into memory.
	virtual bool		Init( char const *pFilename ) = 0;

	// You must call this if you call Init(), to free resources.
	virtual void		Release() = 0;

	// Get some data from the BSP file that's in memory.
	virtual void		GetBSPInfo( CBSPInfo *pInfo ) = 0;

	// Incrementally relight the BSP file in memory given the new entity 
	// descriptions in pVMFFile. pVMFFile should only contain light entities.
	//
	// Returns true only if the lightmaps are updated. If the process is 
	// interrupted or there is an error, false is returned.
	virtual bool		DoIncrementalLight( char const *pVMFFile ) = 0;

	// Calling DoIncrementalLight doesn't actually write anything to disk.
	// Calling this will write the incremental light file out and will write the
	// current in-memory light data into the BSP.
	// NOTE: if DoIncrementalLight never finished, this will do nothing and return false.
	virtual bool		Serialize() = 0;

	// Returns a 0-1 value telling how close it is to completing the task.
	// This can be called from a separate thread than DoIncrementLight.
	virtual float		GetPercentComplete() = 0;

	// This can be called from a separate thread than the DoIncrementalLight thread.
	// It asynchronously tells DoIncrementalLight to stop as soon as possible and exit.
	virtual void		Interrupt() = 0;
};


#endif // IVRADDLL_H
