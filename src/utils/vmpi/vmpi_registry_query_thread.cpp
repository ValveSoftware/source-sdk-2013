//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "stdafx.h"
#include "vmpi_registry_query_thread.h"
#include "tier1/utlstring.h"
#include "curl/curl.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"
#include "vmpi_defs.h"
#include "vmpi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CVMPIRegistryQueryThread::CVMPIRegistryQueryThread()
{
	m_hThread = NULL;
	m_hThreadExitEvent = NULL;
	InitializeCriticalSection( &m_ComputerNamesCS );
}


CVMPIRegistryQueryThread::~CVMPIRegistryQueryThread()
{
	Term();
	DeleteCriticalSection( &m_ComputerNamesCS );
}


void CVMPIRegistryQueryThread::Init()
{
	Term();

	m_hThreadExitEvent = CreateEvent( NULL, false, false, NULL );

	DWORD dwThreadID = 0;
	m_hThread = CreateThread(
		NULL,
		0,
		&CVMPIRegistryQueryThread::StaticThreadFn,
		this,
		0,
		&dwThreadID );
}


void CVMPIRegistryQueryThread::Term()
{
	if ( m_hThread )
	{
		SetEvent( m_hThreadExitEvent );
		WaitForSingleObject( m_hThread, INFINITE );
		CloseHandle( m_hThread );
		m_hThread = NULL;
	}

	if ( m_hThreadExitEvent )
	{
		CloseHandle( m_hThreadExitEvent );
		m_hThreadExitEvent = NULL;
	}

	m_RegisteredServices.Purge();
}

void CVMPIRegistryQueryThread::GetRegisteredServices( CUtlVector<CServiceInfo> &services )
{
	EnterCriticalSection( &m_ComputerNamesCS );

	services.Purge();
	services.AddVectorToTail( m_RegisteredServices );
	m_RegisteredServices.RemoveAll();

	LeaveCriticalSection( &m_ComputerNamesCS );
}


static size_t CURLDataReceivedCallback( void *buffer, size_t size, size_t nmemb, void *userp )
{
	CUtlBuffer *pResponseBuf = reinterpret_cast<CUtlBuffer*>( userp );
	pResponseBuf->Put( buffer, int( size * nmemb ) );
	return size * nmemb;
}

void CVMPIRegistryQueryThread::UpdateServicesFromRegistry()
{
	CUtlVector<VMPIWorkerInfo_t> registeredWorkers;
	// MEGA-HACK: There's an inconsistency between the VMPI_ func below using dlmalloc to alloc memory for registeredWorkers, but this function will call the
	// std library free() to free it, causing a mismatch and crash. Here I'm pre-allocating "enough" space to avoid VMPI_QueryRegistryForWorkers() doing any allocations on the vector.
	registeredWorkers.EnsureCapacity( 10000 );
														
	VMPI_QueryRegistryForWorkers( registeredWorkers );

	// Convert results
	EnterCriticalSection( &m_ComputerNamesCS );
	m_RegisteredServices.RemoveAll();
	for ( int i = 0; i < registeredWorkers.Count(); i++ )
	{
		CServiceInfo si;
		si.m_Addr = registeredWorkers[ i ].addr;
		si.m_ComputerName.SetString( registeredWorkers[i].machineName );
		si.m_iState = registeredWorkers[ i ].nStatus;
		si.m_LiveTimeMS = 0;
		si.m_WorkerAppTimeMS = 0;
		si.m_LastPingTimeMS = Plat_MSTime();
		si.m_pLastStatusText = "";
		si.m_LastLiveTimeMS = Plat_MSTime();
		si.m_CPUPercentage = -1;
		si.m_MemUsageMB = -1;
		si.m_LastUpdateTime = 0;
		si.m_ProtocolVersion = registeredWorkers[ i ].nProtocolVersion;
		si.m_bFromRegistry = true;
		V_strcpy_safe( si.m_ServiceVersion, registeredWorkers[i].vmpiVersion );
		m_RegisteredServices.AddToTail( si );
	}
	LeaveCriticalSection( &m_ComputerNamesCS );
}


DWORD CVMPIRegistryQueryThread::ThreadFn()
{
	// Update the services list every 30 seconds.
	do
	{
		UpdateServicesFromRegistry();
	} while ( WaitForSingleObject( m_hThreadExitEvent, 30000 ) != WAIT_OBJECT_0 );

	return 0;
}


DWORD CVMPIRegistryQueryThread::StaticThreadFn( LPVOID lpParameter )
{
	return ((CVMPIRegistryQueryThread*)lpParameter)->ThreadFn();
}
