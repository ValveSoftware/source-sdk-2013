//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include "vmpi.h"
#include "vmpi_distribute_work.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "vmpi_dispatch.h"
#include "pacifier.h"
#include "vstdlib/random.h"
#include "mathlib/mathlib.h"
#include "threadhelpers.h"
#include "threads.h"
#include "tier1/strtools.h"
#include "tier1/utlmap.h"
#include "tier1/smartptr.h"
#include "tier0/icommandline.h"
#include "cmdlib.h"
#include "vmpi_distribute_tracker.h"
#include "vmpi_distribute_work_internal.h"


// To catch some bugs with 32-bit vs 64-bit and etc.
#pragma warning( default : 4244 )
#pragma warning( default : 4305 )
#pragma warning( default : 4267 )
#pragma warning( default : 4311 )
#pragma warning( default : 4312 )


const int MAX_DW_CALLS = 255;
extern bool g_bSetThreadPriorities;


// Subpacket IDs owned by DistributeWork.
#define DW_SUBPACKETID_MASTER_READY		0
#define DW_SUBPACKETID_WORKER_READY		1
#define DW_SUBPACKETID_MASTER_FINISHED	2
#define DW_SUBPACKETID_WU_RESULTS		4
#define DW_SUBPACKETID_WU_STARTED		6	// A worker telling the master it has started processing a work unit.
// NOTE VMPI_DISTRIBUTE_WORK_EXTRA_SUBPACKET_BASE is where the IWorkUnitDistributorX classes start their subpackets.

VMPI_REGISTER_PACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID );
VMPI_REGISTER_SUBPACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID, DW_SUBPACKETID_MASTER_READY );
VMPI_REGISTER_SUBPACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID, DW_SUBPACKETID_WORKER_READY );
VMPI_REGISTER_SUBPACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID, DW_SUBPACKETID_MASTER_FINISHED );
VMPI_REGISTER_SUBPACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID, DW_SUBPACKETID_WU_RESULTS );
VMPI_REGISTER_SUBPACKET_ID( VMPI_DISTRIBUTEWORK_PACKETID, DW_SUBPACKETID_WU_STARTED );

IWorkUnitDistributorCallbacks *g_pDistributeWorkCallbacks = NULL;


bool DistributeWorkDispatch( MessageBuffer *pBuf, int iSource, int iPacketID );
static CDispatchReg g_DistributeWorkReg( VMPI_DISTRIBUTEWORK_PACKETID, DistributeWorkDispatch );


static CDSInfo g_DSInfo;
static unsigned short g_iCurDSInfo = (unsigned short)-1;	// This is incremented each time DistributeWork is called.
static int g_iMasterFinishedDistributeWorkCall = -1;		// The worker stores this to know which DistributeWork() calls the master has finished.
static int g_iMasterReadyForDistributeWorkCall = -1;

// This is only valid if we're a worker and if the worker currently has threads chewing on work units.
static CDSInfo *g_pCurWorkerThreadsInfo = NULL;

static CUtlVector<uint64> g_wuCountByProcess;
static uint64 g_totalWUCountByProcess[512];

static uint64 g_nWUs;				// How many work units there were this time around.
static uint64 g_nCompletedWUs;		// How many work units completed.
static uint64 g_nDuplicatedWUs;	// How many times a worker sent results for a work unit that was already completed.

// Set to true if Error() is called and we want to exit early. vrad and vvis check for this in their
// thread functions, so the workers quit early when the master is done rather than finishing up
// potentially time-consuming work units they're working on.
bool g_bVMPIEarlyExit = false;

static bool g_bMasterDistributingWork = false;

static IWorkUnitDistributorWorker *g_pCurDistributorWorker = NULL;
static IWorkUnitDistributorMaster *g_pCurDistributorMaster = NULL;

// For the stats database.
WUIndexType g_ThreadWUs[4] = { ~0ull, ~0ull, ~0ull, ~0ull };

class CMasterWorkUnitCompletedList
{
public:
	CUtlVector<WUIndexType> m_CompletedWUs;
};
static CCriticalSectionData<CMasterWorkUnitCompletedList> g_MasterWorkUnitCompletedList;


int SortByWUCount( const void *elem1, const void *elem2 )
{
	uint64 a = g_wuCountByProcess[ *((const int*)elem1) ];
	uint64 b = g_wuCountByProcess[ *((const int*)elem2) ];
	if ( a < b )
		return 1;
	else if ( a == b )
		return 0;
	else	   
		return -1;
}


void PrepareDistributeWorkHeader( MessageBuffer *pBuf, unsigned char cSubpacketID )
{
	char cPacketID[2] = { g_DSInfo.m_cPacketID, cSubpacketID };
	pBuf->write( cPacketID, 2 );
	pBuf->write( &g_iCurDSInfo, sizeof( g_iCurDSInfo ) );
}


void ShowMPIStats( 
	double flTimeSpent,
	unsigned long nBytesSent, 
	unsigned long nBytesReceived, 
	unsigned long nMessagesSent, 
	unsigned long nMessagesReceived )
{
	double flKSent = (nBytesSent + 511) / 1024;
	double flKRecv = (nBytesReceived + 511) / 1024;

	bool bShowOutput = VMPI_IsParamUsed( mpi_ShowDistributeWorkStats );

	bool bOldSuppress = g_bSuppressPrintfOutput;
	g_bSuppressPrintfOutput = !bShowOutput;

		Msg( "\n\n--------------------------------------------------------------\n");
		Msg( "Total Time       : %.2f\n", flTimeSpent );
		Msg( "Total Bytes Sent : %dk (%.2fk/sec, %d messages)\n", (int)flKSent, flKSent / flTimeSpent, nMessagesSent );
		Msg( "Total Bytes Recv : %dk (%.2fk/sec, %d messages)\n", (int)flKRecv, flKRecv / flTimeSpent, nMessagesReceived );
		if ( g_bMPIMaster )
		{
			Msg( "Duplicated WUs   : %I64u (%.1f%%)\n", g_nDuplicatedWUs, (float)g_nDuplicatedWUs * 100.0f / g_nWUs );

			Msg( "\nWU count by proc:\n" );

			int nProcs = VMPI_GetCurrentNumberOfConnections();
			
			CUtlVector<int> sortedProcs;
			sortedProcs.SetSize( nProcs );
			int nRealProcs = 0;
			for ( int i=0; i < nProcs; i++ )
			{
				if ( VMPI_IsProcValid( i ) == true )
				{
					sortedProcs[ nRealProcs ] = i;
					nRealProcs++;
				}
			}

			qsort( sortedProcs.Base(), nRealProcs, sizeof( int ), SortByWUCount );

			for ( int i=0; i < nRealProcs; i++ )
			{
				const char *pMachineName = VMPI_GetMachineName( sortedProcs[i] );
				Msg( "%s", pMachineName );
				
				char formatStr[512];
				Q_snprintf( formatStr, sizeof( formatStr ), "%%%ds %I64u\n", 30 - strlen( pMachineName ), g_wuCountByProcess[ sortedProcs[i] ] );
				Msg( formatStr, ":" );
			}
		}
		Msg( "--------------------------------------------------------------\n\n ");

	g_bSuppressPrintfOutput = bOldSuppress;
}


void VMPI_DistributeWork_DisconnectHandler( int procID, const char *pReason )
{
	if ( g_bMasterDistributingWork )
	{
		// Show the disconnect in the database but not on the screen.
		bool bOldSuppress = g_bSuppressPrintfOutput;
		g_bSuppressPrintfOutput = true;
		Msg( "VMPI_DistributeWork_DisconnectHandler( %d )\n", procID );
		g_bSuppressPrintfOutput = bOldSuppress;
	
		// Redistribute the WUs from this guy's partition to another worker.
		g_pCurDistributorMaster->DisconnectHandler( procID );
	}
}


uint64 VMPI_GetNumWorkUnitsCompleted( int iProc )
{
	Assert( iProc >= 0 && iProc <= ARRAYSIZE( g_totalWUCountByProcess ) );
	return g_totalWUCountByProcess[iProc];
}


void HandleWorkUnitCompleted( CDSInfo *pInfo, int iSource, WUIndexType iWorkUnit, MessageBuffer *pBuf )
{
	VMPITracker_WorkUnitCompleted( ( int ) iWorkUnit, iSource );
	
	if ( g_pCurDistributorMaster->HandleWorkUnitResults( iWorkUnit ) )
	{
		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "-" );

		++ g_nCompletedWUs;
		++ g_wuCountByProcess[iSource];
		++ g_totalWUCountByProcess[iSource];

		// Let the master process the incoming WU data.
		if ( pBuf )
		{
			pInfo->m_MasterInfo.m_ReceiveFn( iWorkUnit, pBuf, iSource );
		}

		UpdatePacifier( float( g_nCompletedWUs ) / pInfo->m_nWorkUnits );
	}
	else
	{
		// Ignore it if we already got the results for this work unit.
		++ g_nDuplicatedWUs;
		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "*" );
	}
}


bool DistributeWorkDispatch( MessageBuffer *pBuf, int iSource, int iPacketID )
{											  
	unsigned short iCurDistributeWorkCall = *((unsigned short*)&pBuf->data[2]);
	if ( iCurDistributeWorkCall >= MAX_DW_CALLS )
		Error( "Got an invalid DistributeWork packet (id: %d, sub: %d) (iCurDW: %d).", pBuf->data[0], pBuf->data[1], iCurDistributeWorkCall );

	CDSInfo *pInfo = &g_DSInfo;
		
	pBuf->setOffset( 4 );
	
	switch ( pBuf->data[1] )
	{
		case DW_SUBPACKETID_MASTER_READY:
		{
			g_iMasterReadyForDistributeWorkCall = iCurDistributeWorkCall;
			return true;
		}

		case DW_SUBPACKETID_WORKER_READY:
		{
			if ( iCurDistributeWorkCall > g_iCurDSInfo || !g_bMPIMaster )
				Error( "State incorrect on master for DW_SUBPACKETID_WORKER_READY packet from %s.", VMPI_GetMachineName( iSource ) );

			if ( iCurDistributeWorkCall == g_iCurDSInfo )
			{
				// Ok, give this guy some WUs.
				if ( g_pCurDistributorMaster )
					g_pCurDistributorMaster->OnWorkerReady( iSource );
			}

			return true;
		}

		case DW_SUBPACKETID_MASTER_FINISHED:
		{
			g_iMasterFinishedDistributeWorkCall = iCurDistributeWorkCall;
			return true;
		}
		
		// Worker sends this to tell the master it has started on a work unit.
		case DW_SUBPACKETID_WU_STARTED:
		{
			if ( iCurDistributeWorkCall != g_iCurDSInfo )
				return true;

			WUIndexType iWU;
			pBuf->read( &iWU, sizeof( iWU ) );
			VMPITracker_WorkUnitStarted( ( int ) iWU, iSource );
			return true;
		}
		

		case DW_SUBPACKETID_WU_RESULTS:
		{
			// We only care about work results for the iteration we're in.
			if ( iCurDistributeWorkCall != g_iCurDSInfo )
				return true;

			WUIndexType iWorkUnit;
			pBuf->read( &iWorkUnit, sizeof( iWorkUnit ) );
			if ( iWorkUnit >= pInfo->m_nWorkUnits )
			{
				Error( "DistributeWork: got an invalid work unit index (%I64u for WU count of %I64u).", iWorkUnit, pInfo->m_nWorkUnits );
			}

			HandleWorkUnitCompleted( pInfo, iSource, iWorkUnit, pBuf );
			return true;
		}

		default:
		{
			if ( g_pCurDistributorMaster )
				return g_pCurDistributorMaster->HandlePacket( pBuf, iSource, iCurDistributeWorkCall != g_iCurDSInfo );
			else if ( g_pCurDistributorWorker )
				return g_pCurDistributorWorker->HandlePacket( pBuf, iSource, iCurDistributeWorkCall != g_iCurDSInfo );
			else
				return false;
		}
	}
}


EWorkUnitDistributor VMPI_GetActiveWorkUnitDistributor()
{
	if ( VMPI_IsParamUsed( mpi_UseSDKDistributor ) )
	{
		Msg( "Found %s.\n", VMPI_GetParamString( mpi_UseSDKDistributor ) );
		return k_eWorkUnitDistributor_SDK;
	}
	else if ( VMPI_IsParamUsed( mpi_UseDefaultDistributor ) )
	{
		Msg( "Found %s.\n", VMPI_GetParamString( mpi_UseDefaultDistributor ) );
		return k_eWorkUnitDistributor_Default;
	}
	else
	{
		if ( VMPI_IsSDKMode() )
			return k_eWorkUnitDistributor_SDK;
		else
			return k_eWorkUnitDistributor_Default;
	}
}


void PreDistributeWorkSync( CDSInfo *pInfo )
{
	if ( g_bMPIMaster )
	{
		// Send a message telling all the workers we're ready to go on this DistributeWork call.
		MessageBuffer mb;
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_MASTER_READY );
		VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
	}
	else
	{
		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "PreDistributeWorkSync: waiting for master\n" );

		// Wait for the master's message saying it's ready to go.
		while ( g_iMasterReadyForDistributeWorkCall < g_iCurDSInfo )
		{
			VMPI_DispatchNextMessage();
		}

		if ( g_iVMPIVerboseLevel >= 1 )
			Msg( "PreDistributeWorkSync: master ready\n" );

		// Now tell the master we're ready.
		MessageBuffer mb;
		PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_WORKER_READY );
		VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID );
	}
}


void DistributeWork_Master( CDSInfo *pInfo, ProcessWorkUnitFn processFn, ReceiveWorkUnitFn receiveFn )
{
	pInfo->m_WorkerInfo.m_pProcessFn = processFn;
	pInfo->m_MasterInfo.m_ReceiveFn = receiveFn;

	VMPITracker_Start( (int) pInfo->m_nWorkUnits );

	g_bMasterDistributingWork = true;
	g_pCurDistributorMaster->DistributeWork_Master( pInfo );
	g_bMasterDistributingWork = false;

	VMPITracker_End();
		
	// Tell all workers to move on.
	MessageBuffer mb;
	PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_MASTER_FINISHED );
	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );

	// Clear the master's local completed work unit list.
	CMasterWorkUnitCompletedList *pList = g_MasterWorkUnitCompletedList.Lock();
	pList->m_CompletedWUs.RemoveAll();	
	g_MasterWorkUnitCompletedList.Unlock();
}


void NotifyLocalMasterCompletedWorkUnit( WUIndexType iWorkUnit )
{
	CMasterWorkUnitCompletedList *pList = g_MasterWorkUnitCompletedList.Lock();
	pList->m_CompletedWUs.AddToTail( iWorkUnit );
	g_MasterWorkUnitCompletedList.Unlock();
}

void CheckLocalMasterCompletedWorkUnits()
{
	CMasterWorkUnitCompletedList *pList = g_MasterWorkUnitCompletedList.Lock();
	
	for ( int i=0; i < pList->m_CompletedWUs.Count(); i++ )
	{
		HandleWorkUnitCompleted( &g_DSInfo, 0, pList->m_CompletedWUs[i], NULL );
	}
	pList->m_CompletedWUs.RemoveAll();	
		
	g_MasterWorkUnitCompletedList.Unlock();
}


void TellMasterThatWorkerStartedAWorkUnit( MessageBuffer &mb, CDSInfo *pInfo, WUIndexType iWU )
{
	mb.setLen( 0 );
	PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_WU_STARTED );
	mb.write( &iWU, sizeof( iWU ) );
	VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID, k_eVMPISendFlags_GroupPackets );
}


void VMPI_WorkerThread( int iThread, void *pUserData )
{
	CDSInfo *pInfo = (CDSInfo*)pUserData;
	CWorkerInfo *pWorkerInfo = &pInfo->m_WorkerInfo;

	
	// Get our index for running work units
	uint64 idxRunningWorkUnit = (uint64) iThread;
	{
		CVMPICriticalSectionLock csLock( &pWorkerInfo->m_WorkUnitsRunningCS );
		csLock.Lock();
		pWorkerInfo->m_WorkUnitsRunning.ExpandWindow( idxRunningWorkUnit, ~0ull );
		csLock.Unlock();
	}


	MessageBuffer mb;
	PrepareDistributeWorkHeader( &mb, DW_SUBPACKETID_WU_RESULTS );

	MessageBuffer mbStartedWorkUnit;	// Special messagebuffer used to tell the master when we started a work unit.

	while ( g_iMasterFinishedDistributeWorkCall < g_iCurDSInfo && !g_bVMPIEarlyExit )
	{
		WUIndexType iWU;

		// Quit out when there are no more work units.
		if ( !g_pCurDistributorWorker->GetNextWorkUnit( &iWU ) )
		{
			// Wait until there are some WUs to do. This should probably use event handles.
			VMPI_Sleep( 10 );
			continue;
		}
			
		CVMPICriticalSectionLock csLock( &pWorkerInfo->m_WorkUnitsRunningCS );
		csLock.Lock();

			// Check if this WU is not running
			WUIndexType const *pBegin = &pWorkerInfo->m_WorkUnitsRunning.Get( 0ull ), *pEnd = pBegin + pWorkerInfo->m_WorkUnitsRunning.PastVisibleIndex();
			WUIndexType const *pRunningWu = GenericFind( pBegin, pEnd, iWU );
			if ( pRunningWu != pEnd )
				continue;

			// We are running it
			pWorkerInfo->m_WorkUnitsRunning.Get( idxRunningWorkUnit ) = iWU;
	
		csLock.Unlock();


		// Process this WU and send the results to the master.
		mb.setLen( 4 );
		mb.write( &iWU, sizeof( iWU ) );

		// Set the current WU for the stats database.
		if ( iThread >= 0 && iThread < 4 )
		{
			g_ThreadWUs[iThread] = iWU;
		}

		// Tell the master we're starting on this WU.
		TellMasterThatWorkerStartedAWorkUnit( mbStartedWorkUnit, pInfo, iWU );
		

		pWorkerInfo->m_pProcessFn( iThread, iWU, &mb );
		g_pCurDistributorWorker->NoteLocalWorkUnitCompleted( iWU );
		
		VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID, /*k_eVMPISendFlags_GroupPackets*/0 );

		// Flush grouped packets every once in a while.
		//VMPI_FlushGroupedPackets( 1000 );
	}

	if ( g_iVMPIVerboseLevel >= 1 )
		Msg( "Worker thread exiting.\n" );
}


void DistributeWork_Worker( CDSInfo *pInfo, ProcessWorkUnitFn processFn )
{
	if ( g_iVMPIVerboseLevel >= 1 )
		Msg( "VMPI_DistributeWork call %d started.\n", g_iCurDSInfo+1 );

	CWorkerInfo *pWorkerInfo = &pInfo->m_WorkerInfo;
	pWorkerInfo->m_pProcessFn = processFn;

	g_pCurWorkerThreadsInfo = pInfo;
	g_pCurDistributorWorker->Init( pInfo );

	// Start a couple threads to do the work.
	RunThreads_Start( VMPI_WorkerThread, pInfo, g_bSetThreadPriorities ? k_eRunThreadsPriority_Idle : k_eRunThreadsPriority_UseGlobalState );
	if ( g_iVMPIVerboseLevel >= 1 )
		Msg( "RunThreads_Start finished successfully.\n" );

	if ( VMPI_IsSDKMode() )
	{
		Msg( "\n" );
		while ( g_iMasterFinishedDistributeWorkCall < g_iCurDSInfo )
		{
			VMPI_DispatchNextMessage( 300 );
			
			Msg( "\rThreads status: " );
			for ( int i=0; i < ARRAYSIZE( g_ThreadWUs ); i++ )
			{
				if ( g_ThreadWUs[i] != ~0ull )
					Msg( "%d: WU %5d  ", i, (int)g_ThreadWUs[i] );
			}

			VMPI_FlushGroupedPackets();
		}
		Msg( "\n" );
	}
	else
	{
		while ( g_iMasterFinishedDistributeWorkCall < g_iCurDSInfo )
		{
			VMPI_DispatchNextMessage();
		}
	}
	

	// Close the threads.
	g_pCurWorkerThreadsInfo = NULL;
	RunThreads_End();

	if ( g_iVMPIVerboseLevel >= 1 )
		Msg( "VMPI_DistributeWork call %d finished.\n", g_iCurDSInfo+1 );
}


// This is called by VMPI_Finalize in case it's shutting down due to an Error() call.
// In this case, it's important that the worker threads here are shut down before VMPI shuts
// down its sockets.
void DistributeWork_Cancel()
{
	if ( g_pCurWorkerThreadsInfo )
	{
		Msg( "\nDistributeWork_Cancel saves the day!\n" );
		g_pCurWorkerThreadsInfo->m_bMasterFinished = true;
		g_bVMPIEarlyExit = true;
		RunThreads_End();
	}
}


// Returns time it took to finish the work.
double DistributeWork( 
	uint64 nWorkUnits,				// how many work units to dole out
	ProcessWorkUnitFn processFn,	// workers implement this to process a work unit and send results back
	ReceiveWorkUnitFn receiveFn		// the master implements this to receive a work unit
	)
{
	++g_iCurDSInfo;

	if ( g_iCurDSInfo == 0 )
	{
		// Register our disconnect handler so we can deal with it if clients bail out.
		if ( g_bMPIMaster )
		{
			VMPI_AddDisconnectHandler( VMPI_DistributeWork_DisconnectHandler );
		}
	}
	else if ( g_iCurDSInfo >= MAX_DW_CALLS )
	{
		Error( "DistributeWork: called more than %d times.\n", MAX_DW_CALLS );
	}

	CDSInfo *pInfo = &g_DSInfo;
	
	pInfo->m_cPacketID = VMPI_DISTRIBUTEWORK_PACKETID;
	pInfo->m_nWorkUnits = nWorkUnits;

	// Make all the workers wait until the master is ready.
	PreDistributeWorkSync( pInfo );

	g_nWUs = nWorkUnits;
	g_nCompletedWUs = 0ull;
	g_nDuplicatedWUs = 0ull;

	// Setup stats info.
	double flMPIStartTime = Plat_FloatTime();
	g_wuCountByProcess.SetCount( 512 );
	memset( g_wuCountByProcess.Base(), 0, sizeof( int ) * g_wuCountByProcess.Count() );
	
	unsigned long nBytesSentStart = g_nBytesSent;
	unsigned long nBytesReceivedStart = g_nBytesReceived;
	unsigned long nMessagesSentStart = g_nMessagesSent;
	unsigned long nMessagesReceivedStart = g_nMessagesReceived;

	EWorkUnitDistributor eWorkUnitDistributor = VMPI_GetActiveWorkUnitDistributor();
	if ( g_bMPIMaster )
	{
		Assert( !g_pCurDistributorMaster );
		g_pCurDistributorMaster = ( eWorkUnitDistributor == k_eWorkUnitDistributor_SDK ? CreateWUDistributor_SDKMaster() : CreateWUDistributor_DefaultMaster() );
		
		DistributeWork_Master( pInfo, processFn, receiveFn );
		
		g_pCurDistributorMaster->Release();
		g_pCurDistributorMaster = NULL;
	}
	else 
	{
		Assert( !g_pCurDistributorWorker );
		g_pCurDistributorWorker = ( eWorkUnitDistributor == k_eWorkUnitDistributor_SDK ? CreateWUDistributor_SDKWorker() : CreateWUDistributor_DefaultWorker() );

		DistributeWork_Worker( pInfo, processFn );
		
		g_pCurDistributorWorker->Release();
		g_pCurDistributorWorker = NULL;
	}

	double flTimeSpent = Plat_FloatTime() - flMPIStartTime;
	ShowMPIStats( 
		flTimeSpent,
		g_nBytesSent - nBytesSentStart,
		g_nBytesReceived - nBytesReceivedStart,
		g_nMessagesSent - nMessagesSentStart,
		g_nMessagesReceived - nMessagesReceivedStart
		);

	// Mark that the threads aren't working on anything at the moment.
	for ( int i=0; i < ARRAYSIZE( g_ThreadWUs ); i++ )
		g_ThreadWUs[i] = ~0ull;
	
	return flTimeSpent;
}
