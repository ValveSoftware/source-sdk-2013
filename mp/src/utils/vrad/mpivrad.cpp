//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// mpivrad.cpp
//

#include <windows.h>
#include <conio.h>
#include "vrad.h"
#include "physdll.h"
#include "lightmap.h"
#include "tier1/strtools.h"
#include "radial.h"
#include "utlbuffer.h"
#include "pacifier.h"
#include "messbuf.h"
#include "bsplib.h"
#include "consolewnd.h"
#include "vismat.h"
#include "vmpi_filesystem.h"
#include "vmpi_dispatch.h"
#include "utllinkedlist.h"
#include "vmpi.h"
#include "mpi_stats.h"
#include "vmpi_distribute_work.h"
#include "vmpi_tools_shared.h"




CUtlVector<char> g_LightResultsFilename;


extern int total_transfer;
extern int max_transfer;

extern void BuildVisLeafs(int);
extern void BuildPatchLights( int facenum );


// Handle VRAD packets.
bool VRAD_DispatchFn( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	switch( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_PLIGHTDATA_RESULTS:
		{
			const char *pFilename = &pBuf->data[2];
			g_LightResultsFilename.CopyArray( pFilename, strlen( pFilename ) + 1 );
			return true;
		}
		
		default:		
			return false;
	}
}
CDispatchReg g_VRADDispatchReg( VMPI_VRAD_PACKET_ID, VRAD_DispatchFn ); // register to handle the messages we want
CDispatchReg g_DistributeWorkReg( VMPI_DISTRIBUTEWORK_PACKETID, DistributeWorkDispatch );



void VRAD_SetupMPI( int &argc, char **&argv )
{
	CmdLib_AtCleanup( VMPI_Stats_Term );

	//
	// Preliminary check -mpi flag
	//
	if ( !VMPI_FindArg( argc, argv, "-mpi", "" ) && !VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Worker ), "" ) )
		return;

	// Force local mode?
	VMPIRunMode mode;
	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Local ), "" ) )
		mode = VMPI_RUN_LOCAL;
	else
		mode = VMPI_RUN_NETWORKED;

	VMPI_Stats_InstallSpewHook();

	//
	//  Extract mpi specific arguments
	//
	Msg( "Initializing VMPI...\n" );
	if ( !VMPI_Init( 
		argc, 
		argv, 
		"dependency_info_vrad.txt", 
		HandleMPIDisconnect,
		mode
		) )
	{
		Error( "MPI_Init failed." );
	}

	StatsDB_InitStatsDatabase( argc, argv, "dbinfo_vrad.txt" );
}


//-----------------------------------------
//
// Run BuildFaceLights across all available processing nodes
// and collect the results.
//

CCycleCount g_CPUTime;


template<class T> void WriteValues( MessageBuffer *pmb, T const *pSrc, int nNumValues)
{
	pmb->write(pSrc, sizeof( pSrc[0]) * nNumValues );
}

template<class T> int ReadValues( MessageBuffer *pmb, T *pDest, int nNumValues)
{
	return pmb->read( pDest, sizeof( pDest[0]) * nNumValues );
}


//--------------------------------------------------
// Serialize face data
void SerializeFace( MessageBuffer * pmb, int facenum )
{
	int i, n;

	dface_t     * f  = &g_pFaces[facenum];
	facelight_t * fl = &facelight[facenum];

	pmb->write(f, sizeof(dface_t));
	pmb->write(fl, sizeof(facelight_t));

	WriteValues( pmb, fl->sample, fl->numsamples);

	//
	// Write the light information
	// 
	for (i=0; i<MAXLIGHTMAPS; ++i) {
		for (n=0; n<NUM_BUMP_VECTS+1; ++n) {
			if (fl->light[i][n])
			{
				WriteValues( pmb, fl->light[i][n], fl->numsamples);
			}
		}
	}

	if (fl->luxel)
		WriteValues( pmb, fl->luxel, fl->numluxels);
	
	if (fl->luxelNormals) 
		WriteValues( pmb, fl->luxelNormals, fl->numluxels);
}

//--------------------------------------------------
// UnSerialize face data
//
void UnSerializeFace( MessageBuffer * pmb, int facenum, int iSource )
{
	int i, n;

	dface_t     * f  = &g_pFaces[facenum];
	facelight_t * fl = &facelight[facenum];

	if (pmb->read(f, sizeof(dface_t)) < 0) 
		Error("UnSerializeFace - invalid dface_t from %s (mb len: %d, offset: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset() );

	if (pmb->read(fl, sizeof(facelight_t)) < 0) 
		Error("UnSerializeFace - invalid facelight_t from %s (mb len: %d, offset: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset() );

	fl->sample = (sample_t *) calloc(fl->numsamples, sizeof(sample_t));
	if (pmb->read(fl->sample, sizeof(sample_t) * fl->numsamples) < 0) 
		Error("UnSerializeFace - invalid sample_t from %s (mb len: %d, offset: %d, fl->numsamples: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset(), fl->numsamples );

	//
	// Read the light information
	// 
	for (i=0; i<MAXLIGHTMAPS; ++i) {
		for (n=0; n<NUM_BUMP_VECTS+1; ++n) {
			if (fl->light[i][n])
			{
				fl->light[i][n] = (LightingValue_t *) calloc( fl->numsamples, sizeof(LightingValue_t ) );
				if ( ReadValues( pmb, fl->light[i][n], fl->numsamples) < 0)
					Error("UnSerializeFace - invalid fl->light from %s (mb len: %d, offset: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset() );
			}
		}
	}

	if (fl->luxel) {
		fl->luxel = (Vector *) calloc(fl->numluxels, sizeof(Vector));
		if (ReadValues( pmb, fl->luxel, fl->numluxels) < 0)
			Error("UnSerializeFace - invalid fl->luxel from %s (mb len: %d, offset: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset() );
	}

	if (fl->luxelNormals) {
		fl->luxelNormals = (Vector *) calloc(fl->numluxels, sizeof( Vector ));
		if ( ReadValues( pmb, fl->luxelNormals, fl->numluxels) < 0)
			Error("UnSerializeFace - invalid fl->luxelNormals from %s (mb len: %d, offset: %d)", VMPI_GetMachineName( iSource ), pmb->getLen(), pmb->getOffset() );
	}

}


void MPI_ReceiveFaceResults( uint64 iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	UnSerializeFace( pBuf, iWorkUnit, iWorker );
}


void MPI_ProcessFaces( int iThread, uint64 iWorkUnit, MessageBuffer *pBuf )
{
	// Do BuildFacelights on the face.
	CTimeAdder adder( &g_CPUTime );

	BuildFacelights( iThread, iWorkUnit );

	// Send the results.
	if ( pBuf )
	{
		SerializeFace( pBuf, iWorkUnit );
	}
}


void RunMPIBuildFacelights()
{
	g_CPUTime.Init();

    Msg( "%-20s ", "BuildFaceLights:" );
	if ( g_bMPIMaster )
	{
		StartPacifier("");
	}

	VMPI_SetCurrentStage( "RunMPIBuildFaceLights" );
	double elapsed = DistributeWork( 
		numfaces, 
		VMPI_DISTRIBUTEWORK_PACKETID,
		MPI_ProcessFaces, 
		MPI_ReceiveFaceResults );

	if ( g_bMPIMaster )
	{
		EndPacifier(false);
		Msg( " (%d)\n", (int)elapsed );
	}

	if ( g_bMPIMaster )
	{
		//
		// BuildPatchLights is normally called from BuildFacelights(),
		// but in MPI mode we have the master do the calculation
		// We might be able to speed this up by doing while the master
		// is idling in the above loop. Wouldn't want to slow down the
		// handing out of work - maybe another thread?
		//
		for ( int i=0; i < numfaces; ++i )
		{
			BuildPatchLights(i);
		}
	}
	else
	{
		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "\n\n%.1f%% CPU utilization during BuildFaceLights\n\n", ( g_CPUTime.GetSeconds() * 100 / elapsed ) );
	}
}


//-----------------------------------------
//
// Run BuildVisLeafs across all available processing nodes
// and collect the results.
//

// This function is called when the master receives results back from a worker.
void MPI_ReceiveVisLeafsResults( uint64 iWorkUnit, MessageBuffer *pBuf, int iWorker )
{
	int patchesInCluster = 0;
	
	pBuf->read(&patchesInCluster, sizeof(patchesInCluster));
	
	for ( int k=0; k < patchesInCluster; ++k )
	{
		int patchnum = 0;
		pBuf->read(&patchnum, sizeof(patchnum));
		
		CPatch * patch = &g_Patches[patchnum];
		int numtransfers;
		pBuf->read( &numtransfers, sizeof(numtransfers) );
		patch->numtransfers = numtransfers;
		if (numtransfers) 
		{
			patch->transfers = new transfer_t[numtransfers];
			pBuf->read(patch->transfers, numtransfers * sizeof(transfer_t));
		}
		
		total_transfer += numtransfers;
		if (max_transfer < numtransfers) 
			max_transfer = numtransfers;
	}
}


// Temporary variables used during callbacks. If we're going to be threadsafe, these 
// should go in a structure and get passed around.
class CVMPIVisLeafsData
{
public:
	MessageBuffer *m_pVisLeafsMB;
	int m_nPatchesInCluster;
	transfer_t *m_pBuildVisLeafsTransfers;
};

CVMPIVisLeafsData g_VMPIVisLeafsData[MAX_TOOL_THREADS+1];



// This is called by BuildVisLeafs_Cluster every time it finishes a patch.
// The results are appended to g_VisLeafsMB and sent back to the master when all clusters are done.
void MPI_AddPatchData( int iThread, int patchnum, CPatch *patch )
{
	CVMPIVisLeafsData *pData = &g_VMPIVisLeafsData[iThread];
	if ( pData->m_pVisLeafsMB )
	{
		// Add in results for this patch
		++pData->m_nPatchesInCluster;
		pData->m_pVisLeafsMB->write(&patchnum, sizeof(patchnum));
		pData->m_pVisLeafsMB->write(&patch->numtransfers, sizeof(patch->numtransfers));
		pData->m_pVisLeafsMB->write( patch->transfers, patch->numtransfers * sizeof(transfer_t) );
	}
}


// This handles a work unit sent by the master. Each work unit here is a 
// list of clusters.
void MPI_ProcessVisLeafs( int iThread, uint64 iWorkUnit, MessageBuffer *pBuf )
{
	CTimeAdder adder( &g_CPUTime );

	CVMPIVisLeafsData *pData = &g_VMPIVisLeafsData[iThread];
	int iCluster = iWorkUnit;

	// Start this cluster.
	pData->m_nPatchesInCluster = 0;
	pData->m_pVisLeafsMB = pBuf;

	// Write a temp value in there. We overwrite it later.
	int iSavePos = 0;
	if ( pBuf )
	{
		iSavePos = pBuf->getLen();
		pBuf->write( &pData->m_nPatchesInCluster, sizeof(pData->m_nPatchesInCluster) );
	}

	// Collect the results in MPI_AddPatchData.
	BuildVisLeafs_Cluster( iThread, pData->m_pBuildVisLeafsTransfers, iCluster, MPI_AddPatchData );

	// Now send the results back..
	if ( pBuf )
	{
		pBuf->update( iSavePos, &pData->m_nPatchesInCluster, sizeof(pData->m_nPatchesInCluster) );
		pData->m_pVisLeafsMB = NULL;
	}
}


void RunMPIBuildVisLeafs()
{
    g_CPUTime.Init();
    
    Msg( "%-20s ", "BuildVisLeafs  :" );
	if ( g_bMPIMaster )
	{
		StartPacifier("");
	}

	memset( g_VMPIVisLeafsData, 0, sizeof( g_VMPIVisLeafsData ) );
	if ( !g_bMPIMaster || VMPI_GetActiveWorkUnitDistributor() == k_eWorkUnitDistributor_SDK )
	{
		// Allocate space for the transfers for each thread.
		for ( int i=0; i < numthreads; i++ )
		{
			g_VMPIVisLeafsData[i].m_pBuildVisLeafsTransfers = BuildVisLeafs_Start();
		}
	}

	//
	// Slaves ask for work via GetMPIBuildVisLeafWork()
	// Results are returned in BuildVisRow()
	//
	VMPI_SetCurrentStage( "RunMPIBuildVisLeafs" );
	
	double elapsed = DistributeWork( 
		dvis->numclusters, 
		VMPI_DISTRIBUTEWORK_PACKETID,
		MPI_ProcessVisLeafs, 
		MPI_ReceiveVisLeafsResults );

	// Free the transfers from each thread.
	for ( int i=0; i < numthreads; i++ )
	{
		if ( g_VMPIVisLeafsData[i].m_pBuildVisLeafsTransfers )
			BuildVisLeafs_End( g_VMPIVisLeafsData[i].m_pBuildVisLeafsTransfers );
	}

	if ( g_bMPIMaster )
	{
		EndPacifier(false);
		Msg( " (%d)\n", (int)elapsed );
	}
	else
	{
		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "%.1f%% CPU utilization during PortalFlow\n", (g_CPUTime.GetSeconds() * 100.0f / elapsed) / numthreads );
	}
}

void VMPI_DistributeLightData()
{
	if ( !g_bUseMPI )
		return;

	if ( g_bMPIMaster )
	{
		const char *pVirtualFilename = "--plightdata--";
		
		CUtlBuffer lightFaceData;

		// write out the light data
		lightFaceData.EnsureCapacity( pdlightdata->Count() + (numfaces * (MAXLIGHTMAPS+sizeof(int))) );
		Q_memcpy( lightFaceData.PeekPut(), pdlightdata->Base(), pdlightdata->Count() );
		lightFaceData.SeekPut( CUtlBuffer::SEEK_HEAD, pdlightdata->Count() );

		// write out the relevant face info into the stream
		for ( int i = 0; i < numfaces; i++ )
		{
			for ( int j = 0; j < MAXLIGHTMAPS; j++ )
			{
				lightFaceData.PutChar(g_pFaces[i].styles[j]);
			}
			lightFaceData.PutInt(g_pFaces[i].lightofs);
		}
		VMPI_FileSystem_CreateVirtualFile( pVirtualFilename, lightFaceData.Base(), lightFaceData.TellMaxPut() );

		char cPacketID[2] = { VMPI_VRAD_PACKET_ID, VMPI_SUBPACKETID_PLIGHTDATA_RESULTS };
		VMPI_Send2Chunks( cPacketID, sizeof( cPacketID ), pVirtualFilename, strlen( pVirtualFilename ) + 1, VMPI_PERSISTENT );
	}
	else
	{
		VMPI_SetCurrentStage( "VMPI_DistributeLightData" );

		// Wait until we've received the filename from the master.
		while ( g_LightResultsFilename.Count() == 0 )
		{
			VMPI_DispatchNextMessage();
		}

		// Open 
		FileHandle_t fp = g_pFileSystem->Open( g_LightResultsFilename.Base(), "rb", VMPI_VIRTUAL_FILES_PATH_ID );
		if ( !fp )
			Error( "Can't open '%s' to read lighting info.", g_LightResultsFilename.Base() );

		int size = g_pFileSystem->Size( fp );
		int faceSize = (numfaces*(MAXLIGHTMAPS+sizeof(int)));

		if ( size > faceSize )
		{
			int lightSize = size - faceSize;
			CUtlBuffer faceData;
			pdlightdata->EnsureCount( lightSize );
			faceData.EnsureCapacity( faceSize );

			g_pFileSystem->Read( pdlightdata->Base(), lightSize, fp );
			g_pFileSystem->Read( faceData.Base(), faceSize, fp );
			g_pFileSystem->Close( fp );

			faceData.SeekPut( CUtlBuffer::SEEK_HEAD, faceSize );

			// write out the face data
			for ( int i = 0; i < numfaces; i++ )
			{
				for ( int j = 0; j < MAXLIGHTMAPS; j++ )
				{
					g_pFaces[i].styles[j] = faceData.GetChar();
				}
				g_pFaces[i].lightofs = faceData.GetInt();
			}
		}
	}
}


