//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

//#include <strstrea.h>
#include "vraddll.h"
#include "bsplib.h"
#include "vrad.h"
#include "map_shared.h"
#include "lightmap.h"
#include "threads.h"


static CUtlVector<unsigned char> g_LastGoodLightData;
static CUtlVector<unsigned char> g_FacesTouched;


static CVRadDLL g_VRadDLL;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVRadDLL, IVRadDLL, VRAD_INTERFACE_VERSION, g_VRadDLL );
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CVRadDLL, ILaunchableDLL, LAUNCHABLE_DLL_INTERFACE_VERSION, g_VRadDLL );


// ---------------------------------------------------------------------------- //
// temporary static array data size tracking
// original data size = 143 megs
// - converting ddispindices, ddispverts, g_dispinfo, and dlightdata to CUtlVector
//		- 51 megs
// ---------------------------------------------------------------------------- //

class dat
{
public:
	char *name;
	int size;
};
#define DATENTRY(name) {#name, sizeof(name)}

dat g_Dats[] =
{
	DATENTRY(dmodels),
	DATENTRY(dvisdata),
	DATENTRY(dlightdataLDR),
	DATENTRY(dlightdataHDR),
	DATENTRY(dentdata),
	DATENTRY(dleafs),
	DATENTRY(dplanes),
	DATENTRY(dvertexes),
	DATENTRY(g_vertnormalindices),
	DATENTRY(g_vertnormals),
	DATENTRY(texinfo),
	DATENTRY(dtexdata),
	DATENTRY(g_dispinfo),
	DATENTRY(dorigfaces),
	DATENTRY(g_primitives),
	DATENTRY(g_primverts),
	DATENTRY(g_primindices),
	DATENTRY(dfaces),
	DATENTRY(dedges),
	DATENTRY(dleaffaces),
	DATENTRY(dleafbrushes),
	DATENTRY(dsurfedges),
	DATENTRY(dbrushes),
	DATENTRY(dbrushsides),
	DATENTRY(dareas),
	DATENTRY(dareaportals),
	DATENTRY(dworldlights),
	DATENTRY(dleafwaterdata),
	DATENTRY(g_ClipPortalVerts),
	DATENTRY(g_CubemapSamples),
	DATENTRY(g_TexDataStringData),
	DATENTRY(g_TexDataStringTable),
	DATENTRY(g_Overlays)
};

int CalcDatSize()
{
	int ret = 0;
	int count = sizeof( g_Dats ) / sizeof( g_Dats[0] );
	
	int i;
	for( i=1; i < count; i++ )
	{
		if( g_Dats[i-1].size > g_Dats[i].size )
		{
			dat temp = g_Dats[i-1];
			g_Dats[i-1] = g_Dats[i];
			g_Dats[i] = temp;
			
			if( i > 1 )
				i -= 2;
			else
				i -= 1;
		}
	}

	for( i=0; i < count; i++ )
		ret += g_Dats[i].size;
	
	return ret;
}

int g_TotalDatSize = CalcDatSize();




int CVRadDLL::main( int argc, char **argv )
{
	return VRAD_Main( argc, argv );
}


bool CVRadDLL::Init( char const *pFilename )
{
	VRAD_Init();
	
	// Set options and run vrad startup code.
	do_fast = true;
	g_bLowPriorityThreads = true;
	g_pIncremental = GetIncremental();

	VRAD_LoadBSP( pFilename );
	return true;
}


void CVRadDLL::Release()
{
}


void CVRadDLL::GetBSPInfo( CBSPInfo *pInfo )
{
	pInfo->dlightdata = pdlightdata->Base();
	pInfo->lightdatasize = pdlightdata->Count();

	pInfo->dfaces = dfaces;
	pInfo->m_pFacesTouched = g_FacesTouched.Base();
	pInfo->numfaces = numfaces;
	
	pInfo->dvertexes = dvertexes;
	pInfo->numvertexes = numvertexes;

	pInfo->dedges = dedges;
	pInfo->numedges = numedges;

	pInfo->dsurfedges = dsurfedges;
	pInfo->numsurfedges = numsurfedges;

	pInfo->texinfo = texinfo.Base();
	pInfo->numtexinfo = texinfo.Count();

	pInfo->g_dispinfo = g_dispinfo.Base();
	pInfo->g_numdispinfo = g_dispinfo.Count();

	pInfo->dtexdata = dtexdata;
	pInfo->numtexdata = numtexdata;

	pInfo->texDataStringData = g_TexDataStringData.Base();
	pInfo->nTexDataStringData = g_TexDataStringData.Count();

	pInfo->texDataStringTable = g_TexDataStringTable.Base();
	pInfo->nTexDataStringTable = g_TexDataStringTable.Count();
}


bool CVRadDLL::DoIncrementalLight( char const *pVMFFile )
{
	char tempPath[MAX_PATH], tempFilename[MAX_PATH];
	GetTempPath( sizeof( tempPath ), tempPath );
	GetTempFileName( tempPath, "vmf_entities_", 0, tempFilename );

	FileHandle_t fp = g_pFileSystem->Open( tempFilename, "wb" );
	if( !fp )
		return false;

	g_pFileSystem->Write( pVMFFile, strlen(pVMFFile)+1, fp );
	g_pFileSystem->Close( fp );

	// Parse the new entities.
	if( !LoadEntsFromMapFile( tempFilename ) )
		return false;

	// Create lights.
	CreateDirectLights();

	// set up sky cameras
	ProcessSkyCameras();
		
	g_bInterrupt = false;
	if( RadWorld_Go() )
	{
		// Save off the last finished lighting results for the BSP.
		g_LastGoodLightData.CopyArray( pdlightdata->Base(), pdlightdata->Count() );
		if( g_pIncremental )
			g_pIncremental->GetFacesTouched( g_FacesTouched );

		return true;
	}
	else
	{
		g_iCurFace = 0;
		return false;
	}
}


bool CVRadDLL::Serialize()
{
	if( !g_pIncremental )
		return false;

	if( g_LastGoodLightData.Count() > 0 )
	{
		pdlightdata->CopyArray( g_LastGoodLightData.Base(), g_LastGoodLightData.Count() );

		if( g_pIncremental->Serialize() )
		{
			// Delete this so it doesn't keep re-saving it.
			g_LastGoodLightData.Purge();
			return true;
		}
	}

	return false;
}


float CVRadDLL::GetPercentComplete()
{
	return (float)g_iCurFace / numfaces;
}


void CVRadDLL::Interrupt()
{
	g_bInterrupt = true;
}


