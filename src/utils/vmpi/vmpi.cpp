//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This module implements the subset of MPI that VRAD and VVIS use.
//
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include <io.h>
#include <conio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <direct.h>
#include "iphelpers.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "vmpi.h"
#include "bitbuf.h"
#include "tier1/strtools.h"
#include "threadhelpers.h"
#include "ithreadedtcpsocket.h"
#include "vstdlib/random.h"
#include "vmpi_distribute_work.h"
#include "filesystem.h"
#include "checksum_md5.h"
#include "tslist.h"
#include "vmpi_logfile.h"
#include "tier0/icommandline.h"
#include "tier0/fasttimer.h"
#include "tier1/strtools.h"
#include "tier1/netadr.h"
#include "tier1/utlstring.h"
#include "curl/curl.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define USE_VMPI_REGISTRY 1

#define DEFAULT_MAX_WORKERS	32	// Unless they specify -mpi_MaxWorkers, it will stop accepting workers after it gets this many.
int g_nMaxWorkerCount = DEFAULT_MAX_WORKERS;

#define VMPI_INTERNAL_PACKET_ID	27
	#define VMPI_INTERNAL_SUBPACKET_MACHINE_NAME				1
	#define VMPI_INTERNAL_SUBPACKET_COMMAND_LINE				2
	#define VMPI_INTERNAL_SUBPACKET_WAITING_FOR_COMMAND_LINE	3
	#define VMPI_INTERNAL_SUBPACKET_GROUPED_PACKET				4
	#define VMPI_INTERNAL_SUBPACKET_TIMING_WAIT_DONE			5
	#define VMPI_INTERNAL_SUBPACKET_VERIFY_EXE_NAME				6
	#define VMPI_INTERNAL_SUBPACKET_PRINT_ON_MASTER				7

VMPI_REGISTER_PACKET_ID( VMPI_INTERNAL_PACKET_ID );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_MACHINE_NAME );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_COMMAND_LINE );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_WAITING_FOR_COMMAND_LINE );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_GROUPED_PACKET );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_TIMING_WAIT_DONE );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_VERIFY_EXE_NAME );
VMPI_REGISTER_SUBPACKET_ID( VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_PRINT_ON_MASTER );


typedef CUtlVector<char> PersistentPacket;

CVMPICriticalSection g_PersistentPacketsCS;
CUtlLinkedList<PersistentPacket*> g_PersistentPackets;

// Command-line parameters list.
#define VMPI_PARAM( paramName, paramFlags, helpText ) {paramName, paramFlags, "-"#paramName, helpText},
class CVMPIParam
{
public:
	EVMPICmdLineParam m_eParam;
	int m_ParamFlags;
	const char *m_pName;
	const char *m_pHelpText;
};
static CVMPIParam g_VMPIParams[] =
{
	{k_eVMPICmdLineParam_FirstParam, 0, "k_eVMPICmdLineParam_FirstParam", "unused"},
	{k_eVMPICmdLineParam_VMPIParam, 0, "mpi", "Enable VMPI."},
#include "vmpi_parameters.h"
};
#undef VMPI_PARAM


// ---------------------------------------------------------------------------------------- //
// Globals.
// ---------------------------------------------------------------------------------------- //

class CVMPIConnection;

//
// WARNING: This should only ever be on for hardcore debugging!
//
bool g_bSuperSpewEnabled = false;


// Used by -mpi_AutoRestart.
CUtlVector<char*> g_OriginalCommandLineParameters;

// This queues up all the incoming VMPI messages.
CVMPICriticalSection g_VMPIMessagesCS;
CUtlLinkedList< CTCPPacket*, int > g_VMPIMessages;
CEvent g_VMPIMessagesEvent;	// This is set when there are messages in the queue.

// These are used to notify the main thread when some socket had OnError() called on it.
CUtlLinkedList<CVMPIConnection*,int> g_ErrorSockets;
CEvent g_ErrorSocketsEvent;
CVMPICriticalSection g_ErrorSocketsCS;
bool g_bTimingWaitDone = false;
bool g_bGroupPackets = false;

#define MAX_VMPI_CONNECTIONS 4096
CThreadMutex g_ConnectionMutexes[MAX_VMPI_CONNECTIONS];
CVMPIConnection *g_Connections[MAX_VMPI_CONNECTIONS];
int g_nConnections = 0;
CVMPICriticalSection g_ConnectionsCS;

// If true, then it will set certain thread priorities low.
bool g_bSetThreadPriorities = true;

VMPIDispatchFn g_VMPIDispatch[MAX_VMPI_PACKET_IDS];
CTSList<MessageBuffer*> g_DispatchBuffers;

VMPIRunMode g_VMPIRunMode = VMPI_RUN_NETWORKED;
VMPIFileSystemMode g_VMPIFileSystemMode = VMPI_FILESYSTEM_TCP;

static char g_GroupedPacketHeader[] = { VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_GROUPED_PACKET };
static unsigned long g_LastFlushGroupedPacketsTime = 0;

// Set to true if we're running under the SDK (i.e. vmpi_transfer.exe is not found).
bool g_bVMPISDKMode = false;
bool g_bVMPISDKModeSet = false;	// If g_bVMPISDKMode has not been set, then VMPI_IsSDKMode just looks for VMPI_Transfer (and doesn't check the command line).
bool g_bIsRunningVMPITransfer = false;

int g_nBytesSent = 0;
int g_nMessagesSent = 0;
int g_nBytesReceived = 0;
int g_nMessagesReceived = 0;

int g_nMulticastBytesSent = 0;
int g_nMulticastBytesReceived = 0;


CUtlLinkedList<VMPI_Disconnect_Handler,int> g_DisconnectHandlers;

bool g_bUseMPI = false;
int g_iVMPIVerboseLevel = 0;
bool g_bMPIMaster = false;

bool g_bMPI_Stats = false;
bool g_bMPI_StatsTextOutput = false;

char g_CurrentStageString[128] = "";
CVMPICriticalSection g_CurrentStageCS;

char g_MasterExeName[MAX_PATH];
bool g_bReceivedMasterExeName = false;


// Change our window text.
HINSTANCE g_hKernel32DLL = NULL;
typedef HWND (*GetConsoleWndFn)();
GetConsoleWndFn g_pConsoleWndFn = NULL;


static CVMPIPacketIDReg *g_pVMPIPacketIDRegHead = NULL;


// ---------------------------------------------------------------------------------------- //
// Classes.
// ---------------------------------------------------------------------------------------- //

// This class is used while discovering what files the workers need.
class CDependencyInfo
{
public:
	class CDependencyFile
	{
	public:
		char m_Name[MAX_PATH];
	};


	// This is the directory where the dependency files live (i.e. all the binaries that the workers need to run the job).
	char m_DependencyFilesDir[MAX_PATH];

	// "vrad.exe", "vvis.exe", etc.
	char m_OriginalExeFilename[MAX_PATH];

	CUtlVector<CDependencyFile*> m_Files;


public:

	CDependencyFile* FindFile( const char *pFilename )
	{
		for ( int i=0; i < m_Files.Count(); i++ )
		{
			if ( stricmp( pFilename, m_Files[i]->m_Name ) == 0 )
				return m_Files[i];
		}
		return NULL;
	}
};

class CVMPIConnectionCreator : public IHandlerCreator
{
public:
	virtual ITCPSocketHandler* CreateNewHandler();

	static void ReleaseHandle( int nConnection );
};


class CVMPIConnection : public ITCPSocketHandler
{
	friend class CVMPIConnectionCreator;

public:
	CVMPIConnection( int iConnection )
	{
		m_iConnection = iConnection;
		m_pSocket = NULL;
		m_bIsAService = false;

		char str[512];
		V_snprintf( str, sizeof( str ), "%d", iConnection );
		SetMachineName( str );
		m_JobWorkerID = 0xFFFFFFFF;

		m_bNameSet = false;
		m_bInErrorState = false;
	}


private:

	virtual ~CVMPIConnection()
	{
		// NOTE that only CVMPIConnectionCreator can delete us. Use DeleteSelf() to delete the connection properly.
		Assert( !m_pSocket );
	}


public:
	
	void DeleteSelf()
	{
		if ( m_pSocket )
		{
			// If we have a socket, then the socket will delete us as it shuts down.
			IThreadedTCPSocket *pSocket = m_pSocket;
			m_pSocket = NULL;
			pSocket->Release();
		}
		else
		{
			delete this;
		}
	}

	void HandleDisconnect()
	{
		if ( m_pSocket )
		{
			// Copy out the error string.
			CVMPICriticalSectionLock csLock( &g_ErrorSocketsCS );
			csLock.Lock();
				char str[512];
				V_strncpy( str, m_ErrorString.Base(), sizeof( str ) );
			csLock.Unlock();

			// Tell the app.
			FOR_EACH_LL( g_DisconnectHandlers, i )
				g_DisconnectHandlers[i]( m_iConnection, str );
			
			// Free our socket.
			m_pSocket->Release();
			m_pSocket = NULL;
		}
	}

	
	IThreadedTCPSocket* GetSocket()
	{
		return m_pSocket;
	}

	
	void SetMachineName( const char *pName )
	{
		m_MachineName.CopyArray( pName, V_strlen( pName ) + 1 );
		m_bNameSet = true;
	}
	
	const char* GetMachineName()
	{
		return m_MachineName.Base();
	}

	bool HasMachineNameBeenSet()
	{
		return m_bNameSet;
	}


// ITCPSocketHandler implementation (thread-safe stuff).
public:

	virtual void Init( IThreadedTCPSocket *pSocket )
	{
		m_pSocket = pSocket;
	}

	virtual void OnPacketReceived( CTCPPacket *pPacket )
	{
		// Set who this message came from.
		pPacket->SetUserData( m_iConnection );
		Assert( m_iConnection >= 0 && m_iConnection < 2048 );

		// Store it in the global list.
		CVMPICriticalSectionLock csLock( &g_VMPIMessagesCS );
		csLock.Lock();
			
			g_VMPIMessages.AddToTail( pPacket );

			if ( g_VMPIMessages.Count() == 1 )
				g_VMPIMessagesEvent.SetEvent();
	}

	virtual void OnError( int errorCode, const char *pErrorString )
	{
		if ( !g_bMPIMaster )
		{
			Msg( "%s - CVMPIConnection::OnError( %s )\n", GetMachineName(), pErrorString );
		}

		CVMPICriticalSectionLock csLock( &g_ErrorSocketsCS );
		csLock.Lock();

			if ( m_bInErrorState == true )
			{	// we already are in an error state, so don't add to the list
				return;
			}

			m_bInErrorState = true;

			m_ErrorString.CopyArray( pErrorString, V_strlen( pErrorString ) + 1 );
			
			g_ErrorSockets.AddToTail( this );
			
			// Notify the main thread that a socket is in trouble!
			g_ErrorSocketsEvent.SetEvent();

		// Make sure the main thread picks up this error soon.
		InterlockedIncrement( &m_ErrorSignal );	
	}

	virtual void Release( bool bForce )
	{
		if ( m_bInErrorState == true && bForce == false )
		{
			return;
		}

		m_pSocket = NULL;		// we are being released inside of the socket, so don't recurse

		CVMPIConnectionCreator::ReleaseHandle( m_iConnection );
	}

	virtual int GetConnectionID( ) { return m_iConnection; }

public:

	unsigned long m_JobWorkerID;
	bool m_bIsAService;				// If true, then this is just a service getting the files. Don't count it as an active worker.

	CUtlVector<int> m_GroupedChunkLengths;
	CUtlVector<void*> m_GroupedChunks;


private:
	
	CUtlVector<char> m_MachineName;
	CUtlVector<char> m_ErrorString;
	long m_ErrorSignal;
	int m_iConnection;
	IThreadedTCPSocket *m_pSocket;
	bool m_bNameSet;
	bool m_bInErrorState;
};


ITCPSocketHandler *CVMPIConnectionCreator::CreateNewHandler()
{
	for( int i = 0; i < g_nConnections; i++ )
	{
		if ( g_Connections[ i ] == NULL )
		{
			CVMPIConnection *pRet = new CVMPIConnection( i );
			g_Connections[ i ] = pRet;

			return pRet;
		}
	}

	Assert( g_nConnections < MAX_VMPI_CONNECTIONS );
	CVMPIConnection *pRet = new CVMPIConnection( g_nConnections );
	g_Connections[ g_nConnections ] = pRet;
	g_nConnections++;

	return pRet;
}

void CVMPIConnectionCreator::ReleaseHandle( int nConnection )
{
	delete g_Connections[ nConnection ];
	g_Connections[ nConnection ] = NULL;
}



CVMPIPacketIDReg::CVMPIPacketIDReg( int nPacketID, int nSubPacketID, const char *pName )
{
	m_nPacketID = nPacketID;
	m_nSubPacketID = nSubPacketID;
	m_pName = pName;
	m_pNext = g_pVMPIPacketIDRegHead;
	g_pVMPIPacketIDRegHead = this;
}

void CVMPIPacketIDReg::Lookup( int nPacketID, int nSubPacketID, char *pPacketIDString, int nPacketIDStringSize, char *pSubPacketIDString, int nSubPacketIDStringSize )
{
	// First find the packet ID.
	CVMPIPacketIDReg *pCur;
	for ( pCur = g_pVMPIPacketIDRegHead; pCur; pCur = pCur->m_pNext )
	{
		if ( pCur->m_nPacketID == nPacketID && pCur->m_nSubPacketID == -1 )
		{
			V_strncpy( pPacketIDString, pCur->m_pName, nPacketIDStringSize );
			break;
		}
	}

	// Didn't find it? Just print the number.
	if ( !pCur )
	{
		V_snprintf( pPacketIDString, nPacketIDStringSize, "(%d)", nPacketID );
	}

	// Now find the subpacket ID.
	for ( pCur = g_pVMPIPacketIDRegHead; pCur; pCur = pCur->m_pNext )
	{
		if ( pCur->m_nPacketID == nPacketID && pCur->m_nSubPacketID == nSubPacketID )
		{
			V_strncpy( pSubPacketIDString, pCur->m_pName, nSubPacketIDStringSize );
			break;
		}
	}

	// Didn't find it? Just print the number.
	if ( !pCur )
	{
		V_snprintf( pSubPacketIDString, nSubPacketIDStringSize, "(%d)", nSubPacketID );
	}
}



// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

void VMPI_SuperSpew( const char *pMsg, ... )
{
	// This is usually used in conjunction with the log file to trace out protocol errors.
	if ( g_bSuperSpewEnabled )
	{
		va_list marker;
		va_start( marker, pMsg );
		char str[2048];
		V_vsnprintf( str, sizeof( str ), pMsg, marker );
		va_end( marker );

		Msg( "%s", str );
	}
}

const char* VMPI_FindArg( int argc, char **argv, const char *pName, const char *pDefault )
{
	for ( int i=0; i < argc; i++ )
	{
		if ( stricmp( argv[i], pName ) == 0 )
		{
			if ( (i+1) < argc )
				return argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}


void ParseOptions( int argc, char **argv )
{
	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_NoTimeout ) ) )
		ThreadedTCP_EnableTimeouts( false );

	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_DontSetThreadPriorities ) ) )
	{
		Msg( "%s found.\n", VMPI_GetParamString( mpi_DontSetThreadPriorities ) );
		g_bSetThreadPriorities = false;
		ThreadedTCP_SetTCPSocketThreadPriorities( false );
	}

	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_GroupPackets ) ) )
	{
		Msg( "%s found.\n", VMPI_GetParamString( mpi_GroupPackets ) );
		g_bGroupPackets = true;
	}	

	const char *pTransmitRate = VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_FileTransmitRate ), "1" );
	if ( pTransmitRate )
	{
		extern int MULTICAST_TRANSMIT_RATE;
		MULTICAST_TRANSMIT_RATE = atoi( pTransmitRate ) * 1024;
	}

	const char *pVerbose = VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Verbose ), "1" );
	if ( pVerbose )
	{
		if ( pVerbose[0] == '1' )
			g_iVMPIVerboseLevel = 1;
		else if ( pVerbose[0] == '2' )
			g_iVMPIVerboseLevel = 2;
	}

	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Stats ) ) )
		g_bMPI_Stats = true;
	
	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Stats_TextOutput ) ) )
		g_bMPI_StatsTextOutput = true;
}


void SetupDependencyFilename( CDependencyInfo *pInfo, const char *pPatchDirectory )
{
	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "GetModuleFileName failed." );
	
	// If they're in patch mode, then the dependency files come out of a directory they've passed in.
	// Otherwise, the files come from the same exe dir we're in (like c:\valve\game\bin).
	if ( pPatchDirectory ) 
	{
		V_strncpy( pInfo->m_DependencyFilesDir, pPatchDirectory, sizeof( pInfo->m_DependencyFilesDir ) );
	}
	else
	{
		V_strncpy( pInfo->m_DependencyFilesDir, baseExeFilename, sizeof( pInfo->m_DependencyFilesDir ) );
		V_StripLastDir( pInfo->m_DependencyFilesDir, sizeof( pInfo->m_DependencyFilesDir ) );
	}

	// Get the exe filename.	
	V_strncpy( pInfo->m_OriginalExeFilename, V_UnqualifiedFileName( baseExeFilename ), sizeof( pInfo->m_OriginalExeFilename ) );
}


bool ReadString( char *pOut, int maxLen, FILE *fp )
{
	if ( !fgets( pOut, maxLen, fp ) || pOut[0] == 0 )
		return false;

	int len = strlen( pOut );
	if ( pOut[len - 1] == '\n' )
		pOut[len - 1] = 0;

	return true;
}


void ParseDependencyFile( CDependencyInfo *pInfo, const char *pDepFilename )
{
	FILE *fp = fopen( pDepFilename, "rt" );
	if ( !fp )
		Error( "Can't find %s.", pDepFilename );

	const char *pOptionalPrefix = "optional ";

	char tempStr[MAX_PATH];
	while ( ReadString( tempStr, sizeof( tempStr ), fp ) )
	{
		CDependencyInfo::CDependencyFile *pFile = new CDependencyInfo::CDependencyFile;
		bool bOptional = false;
		if ( strstr( tempStr, "optional " ) == tempStr )
		{
			bOptional = true;
			Q_strncpy( pFile->m_Name, tempStr + strlen( pOptionalPrefix ), sizeof( pFile->m_Name ) );
		}
		else
		{
			Q_strncpy( pFile->m_Name, tempStr, sizeof( pFile->m_Name ) );
		}

		// Now get the file info.
		char fullFilename[MAX_PATH];
		V_ComposeFileName( pInfo->m_DependencyFilesDir, pFile->m_Name, fullFilename, sizeof( fullFilename ) );
		
		if ( _access( fullFilename, 0 ) == 0 )
		{
			pInfo->m_Files.AddToTail( pFile );
		}
		else
		{
			delete pFile;

			if ( !bOptional )
				Error( "Can't find %s (listed in %s).", fullFilename, pDepFilename );
		}
	}

	fclose( fp );
}


void SetupDependenciesForPatch( CDependencyInfo *pInfo, const char *pPatchDirectory )
{
	char searchStr[MAX_PATH];
	V_ComposeFileName( pPatchDirectory, "*.*", searchStr, sizeof( searchStr ) );
	
	_finddata_t data;
	long handle = _findfirst( searchStr, &data );
	if ( handle != -1 )
	{
		do
		{
			if ( data.name[0] == '.' || (data.attrib & _A_SUBDIR) != 0 )
				continue;
				
			CDependencyInfo::CDependencyFile *pFile = new CDependencyInfo::CDependencyFile;
			V_strncpy( pFile->m_Name, data.name, sizeof( pFile->m_Name ) );
			pInfo->m_Files.AddToTail( pFile );
		} while( _findnext( handle, &data ) == 0 );
	
		_findclose( handle );
	}
}


void SetupDependencyInfo( CDependencyInfo *pInfo, const char *pDependencyFilename, bool bPatchMode )
{
	if ( bPatchMode )
	{
		const char *pPatchDirectory = pDependencyFilename;
		
		SetupDependencyFilename( pInfo, pPatchDirectory );
		SetupDependenciesForPatch( pInfo, pPatchDirectory );
	}
	else
	{
		SetupDependencyFilename( pInfo, NULL );
		
		// Parse the dependency file.
		char depFilename[MAX_PATH];
		V_ComposeFileName( pInfo->m_DependencyFilesDir, pDependencyFilename, depFilename, sizeof( depFilename ) );
		ParseDependencyFile( pInfo, depFilename );
	}
}


int GetCurMicrosecondsAndSleep( int sleepLen )
{
	Sleep( sleepLen );

	CCycleCount cnt;
	cnt.Sample();
	return cnt.GetMicroseconds();
}


void CountActiveConnections( int *nRegularWorkers, int *nServiceDownloaders )
{
	*nRegularWorkers = *nServiceDownloaders = 0;
	int nTotalConnections = g_nConnections;
	for ( int i=0; i < nTotalConnections; i++ )
	{
		if ( VMPI_IsProcConnected( i ) )
		{
			if ( VMPI_IsProcAService( i ) )
				(*nServiceDownloaders)++;
			else
				(*nRegularWorkers)++;
		}
	}
}

// In this function, we update the window text to tell how many active workers there are.
void UpdateActiveConnectionsText()
{
	if ( !g_bMPIMaster || !g_pConsoleWndFn )
		return;

	HWND hWnd = g_pConsoleWndFn();
	if ( !hWnd )
		return;

	int nRegularWorkers, nDownloaders;
	CountActiveConnections( &nRegularWorkers, &nDownloaders );

	char str[512];
	if ( g_bVMPISDKMode )
	{
		V_snprintf( str, sizeof( str ), "VMPI (SDK) - Workers: %d", nRegularWorkers );
	}
	else
	{
		V_snprintf( str, sizeof( str ), "VMPI - Workers: %d, Downloaders: %d", nRegularWorkers, nDownloaders );
	}
	SetWindowText( hWnd, str );
}


void VMPI_SendMachineNameTo( int iProc )
{
	const char *pMyName = VMPI_GetLocalMachineName();

	VMPI_SuperSpew( "Sent SUBPACKET_MACHINE_NAME (%s).\n", pMyName );

	unsigned char packetData[VMPI_PACKET_SIZE];
	packetData[0] = VMPI_INTERNAL_PACKET_ID;
	packetData[1] = VMPI_INTERNAL_SUBPACKET_MACHINE_NAME;
	V_strncpy( (char*)&packetData[2], pMyName, sizeof( packetData ) - 2 );
	VMPI_SendData( packetData, 2 + V_strlen( pMyName ) + 1, iProc );
}

static CVMPIConnection* FindConnectionBySocket( IThreadedTCPSocket *pSocket, bool bLockConnections )
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	if ( bLockConnections )
		connectionsLock.Lock();

	for ( int i=0; i < g_nConnections; i++ )
		if ( g_Connections[ i ] != NULL && g_Connections[ i ]->GetSocket() == pSocket )
			return g_Connections[i];
	
	return NULL;
}

static char* CopyString( const char *pStr )
{
	int len = V_strlen( pStr ) + 1;
	char *pArg = new char[len];
	V_strncpy( pArg, pStr, len );
	return pArg;
}

// ---------------------------------------------------------------------------------------- //
// Internal VMPI dispatch..
// ---------------------------------------------------------------------------------------- //

void VMPI_SetMachineName( int iProc, const char *pName );

CUtlVector<char*> g_WorkerCommandLine;
bool g_bReceivedWorkerCommandLine = false;


bool VMPI_InternalDispatchFn( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	if ( pBuf->getLen() >= 2 )
	{
		if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_MACHINE_NAME )
		{
			if ( pBuf->getLen() >= 3 )
			{
				VMPI_SuperSpew( "Received SUBPACKET_MACHINE_NAME (%s).\n", &pBuf->data[2] );
				VMPI_SetMachineName( iSource, &pBuf->data[2] );
				return true;
			}
			else
			{
				VMPI_SuperSpew( "Received invalid SUBPACKET_MACHINE_NAME packet (not long enough!)\n" );
			}
		}
		else if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_WAITING_FOR_COMMAND_LINE )
		{
			if ( !VMPI_IsSDKMode() )
			{
				Warning( "Worker %d is running in SDK mode (and the master is not)!\n", iSource );
			}
			return true;
		}
		else if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_COMMAND_LINE )
		{
			pBuf->setOffset( 2 );
			
			int nArgs;
			pBuf->read( &nArgs, sizeof( nArgs ) );
			for ( int i=0; i < nArgs; i++ )
			{
				char str[4096];
				if ( pBuf->ReadString( str, sizeof( str ) ) == -1 )
					Error( "Error in ReadString() while reading command line." );
				
				g_WorkerCommandLine.AddToTail( CopyString( str ) );
			}
			
			g_bReceivedWorkerCommandLine = true;
			return true;
		}
		else if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_VERIFY_EXE_NAME )
		{
			pBuf->setOffset( 2 );
			
			if ( pBuf->ReadString( g_MasterExeName, sizeof( g_MasterExeName ) ) == -1 )
				Error( "Error in ReadString() while reading VMPI_INTERNAL_SUBPACKET_VERIFY_EXE_NAME." );

			VMPI_SuperSpew( "Received SUBPACKET_VERIFY_EXE_NAME (%s).\n", g_MasterExeName );
			g_bReceivedMasterExeName = true;
			return true;
		}
		else if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_PRINT_ON_MASTER )
		{
			pBuf->setOffset( 2 );

			char str[2048];
			if ( pBuf->ReadString( str, sizeof( str ) ) == -1 )
				Plat_FatalError( "Error in ReadString() while reading VMPI_INTERNAL_SUBPACKET_PRINT_ON_MASTER." );

			Msg( "\nWorker %d (%s) message: %s\n", iSource, VMPI_GetMachineName( iSource ), str );
			return true;
		}
		else if ( pBuf->data[1] == VMPI_INTERNAL_SUBPACKET_TIMING_WAIT_DONE )
		{
			g_bTimingWaitDone = true;
			return true;
		}
	}

	return false;
}
CDispatchReg g_VMPIInternalDispatchReg( VMPI_INTERNAL_PACKET_ID, VMPI_InternalDispatchFn ); // register to handle the messages we want


void VMPI_SendCommandLine( int argc, char **argv )
{
	MessageBuffer mb;
	
	char cPacketHeader[2] = {VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_COMMAND_LINE};
	mb.write( cPacketHeader, sizeof( cPacketHeader ) );
	mb.write( &argc, sizeof( argc ) );
	for ( int i=0; i < argc; i++ )
		mb.WriteString( argv[i] );
	
	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
}

void VMPI_ReceiveCommandLine()
{
	// For verification purposes, tell the master we're trying to get the command line.
	unsigned char chData[2] = {VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_WAITING_FOR_COMMAND_LINE};
	VMPI_SendData( chData, sizeof( chData ), VMPI_MASTER_ID );
	
	double startTime = Plat_FloatTime();
	while ( !g_bReceivedWorkerCommandLine )
	{
		if ( Plat_FloatTime() - startTime > 30 )
			Error( "VMPI_ReceiveCommandLine: timeout. Is the master running in SDK mode?" );

		VMPI_DispatchNextMessage( 10 * 1000 );
	}
}


void VMPI_SendExeName()
{
	MessageBuffer mb;
	
	char cPacketHeader[2] = {VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_VERIFY_EXE_NAME};
	mb.write( cPacketHeader, sizeof( cPacketHeader ) );

	char baseExeFilename[MAX_PATH], fileBase[MAX_PATH];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "VMPI_CheckSDKMode -> GetModuleFileName failed." );

	V_FileBase( baseExeFilename, fileBase, sizeof( fileBase ) );
	mb.WriteString( fileBase );
	
	VMPI_SuperSpew( "Sending SUBPACKET_VERIFY_EXE_NAME (%s).\n", fileBase );
	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
}

void VMPI_ReceiveExeName()
{
	double startTime = Plat_FloatTime();
	while ( !g_bReceivedMasterExeName )
	{
		if ( Plat_FloatTime() - startTime > 30 )
			Error( "VMPI_ReceiveExeName: timeout." );

		VMPI_DispatchNextMessage( 10 * 1000 );
	}
	
	// Now compare the exe name we got with our own.
	char baseExeFilename[MAX_PATH], fileBase[MAX_PATH];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "VMPI_CheckSDKMode -> GetModuleFileName failed." );

	// Unless we're a vmpi_transfer.. vmpi_transfer can always connect.
	V_FileBase( baseExeFilename, fileBase, sizeof( fileBase ) );
	if ( V_stricmp( fileBase, "vmpi_transfer" ) != 0 )
	{
		if ( V_stricmp( fileBase, g_MasterExeName ) != 0 )
		{
			Error( "VMPI_ReceiveExeName: mismatched exe names (master: %s, me: %s).\nThis usually just means the master finished"
				" a job like vvis really fast and started a vrad immediately, and an old vvis worker connected to the new vrad job.",
				g_MasterExeName, fileBase );
		}
	}
}


// ---------------------------------------------------------------------------------------- //
// CMasterBroadcaster
// This class broadcasts messages looking for workers. The app updates it as often as possible
// and it'll add workers as necessary.
// ---------------------------------------------------------------------------------------- //

#define MASTER_BROADCAST_INTERVAL 600 // Send every N milliseconds.
#define MASTER_WORKER_QUERY_INTERVAL 120.0	// Query the VMPI registry for more workers at most every 2 minutes, otherwise reuse our current list of active workers

class CMasterBroadcaster
{
public:
	
	CMasterBroadcaster();
	~CMasterBroadcaster();

	bool Init( int argc, char **argv, const char *pDependencyFilename, int nMaxWorkers, VMPIRunMode runMode, bool bPatchMode );
	void Term();

	// What port is it listening on?
	int GetListenPort() const;
	
	// These can be used to allow more workers on or filter who's able to connect
	int GetMaxWorkers() const;
	void IncreaseMaxWorkers( int count );
	void SetPassword( const char *pPassword );
	void SetNoTimeoutOption();


private:

	void GetPatchWorkerList( int argc, char **argv );
	bool AskSomeWorkersToConnect();

private:

	class CMasterBroadcastInfo
	{
	public:
		int m_JobID[4];
		char m_Password[256];
		char m_WorkerExeFilename[MAX_PATH];
		CUtlVector<char*> m_Args;
		char m_PatchVersion[32];	// 0 if not patching.
	};

	void ThreadFn();
	static DWORD WINAPI StaticThreadFn( LPVOID lpParameter );

	bool Update();
	void BuildBroadcastPacket( bf_write &buf );


private:

	ITCPConnectSocket *m_pListenSocket;
	ITCPConnectSocket *m_pDownloaderListenSocket;
	ISocket *m_pSocket;

	DWORD m_LastSendTime;
	CMasterBroadcastInfo m_BroadcastInfo;
	CUtlVector<CIPAddr> m_PatchWorkerIPs;	// If in patch mode, these are the IPs we send the job request to (instead of broadcasting).
	bool m_bPatching;

	CVMPIConnectionCreator m_ConnectionCreator;
	int m_nMaxWorkers;

	HANDLE m_hThread;
	CEvent m_hShutdownEvent;
	CEvent m_hShutdownReply;
	
	VMPIRunMode m_RunMode;
	int m_iListenPort;
	int m_iDownloaderListenPort;

	double m_flLastWorkerQueryTime;
	CUtlVector<VMPIWorkerInfo_t> m_registeredWorkers;
	int m_nRegisteredWorkerInviteIndex;
};


CMasterBroadcaster::CMasterBroadcaster()
{
	m_pListenSocket = NULL;
	m_pDownloaderListenSocket = NULL;
	m_pSocket = NULL;
	m_iListenPort = -1;
	m_iDownloaderListenPort = -1;
	m_flLastWorkerQueryTime = -1.0f;
	m_nRegisteredWorkerInviteIndex = -1;
}

CMasterBroadcaster::~CMasterBroadcaster()
{
	Term();
}


void CMasterBroadcaster::GetPatchWorkerList( int argc, char **argv )
{
	m_PatchWorkerIPs.Purge();
	for ( int i=0; i < argc-1; i++ )
	{
		if ( V_stricmp( argv[i], "-mpi_PatchWorkers" ) == 0 )
		{
			int workerCount = atoi( argv[i+1] );
			for ( int iWorker=0; iWorker < workerCount; iWorker++ )
			{
				int iArg = i+2 + iWorker;
				if ( iArg >= argc )
					Error( "-mpi_PatchWorkers: %d specified for count, but not enough IPs following.\n", workerCount );
			
				int a, b, c, d;
				const char *pArg = argv[iArg];
				sscanf( pArg, "%d.%d.%d.%d", &a, &b, &c, &d );
				
				CIPAddr addr;
				addr.Init( a, b, c, d, 0 );
				m_PatchWorkerIPs.AddToTail( addr );				
			}
			return;
		}
	}
}

bool CMasterBroadcaster::Init( 
	int argc, 
	char **argv, 
	const char *pDependencyFilename, 
	int nMaxWorkers, 
	VMPIRunMode runMode,
	bool bPatchMode )
{
	m_RunMode = runMode;
	m_nMaxWorkers = nMaxWorkers;

	// Open the file that tells us which binaries we depend on.
	CDependencyInfo dependencyInfo;
	if ( m_RunMode == VMPI_RUN_NETWORKED && !g_bVMPISDKMode )
	{
		SetupDependencyInfo( &dependencyInfo, pDependencyFilename, bPatchMode );
	}

	m_pListenSocket = NULL;
	m_pDownloaderListenSocket = NULL;

	const char *pPortStr = VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Port ) );
	if ( pPortStr )
	{
		m_iListenPort = atoi( pPortStr );
		m_iDownloaderListenPort = m_iListenPort + 1;
		m_pListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, m_iListenPort );
		if ( !g_bVMPISDKMode )
		{
			m_pDownloaderListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, m_iDownloaderListenPort );
		}
	}
	else
	{
		// Create a socket to listen on.
		CCycleCount cnt;
		cnt.Sample();
		int iTime = (int)cnt.GetMicroseconds();
		srand( (unsigned)iTime );

		for ( int iTest=VMPI_MASTER_FIRST_PORT; iTest <= VMPI_MASTER_LAST_PORT; iTest++ )
		{
			m_iListenPort = iTest;
			m_pListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, m_iListenPort );
			if ( m_pListenSocket )
				break;				   
		}
		// No need to create the downloader in SDK mode.
		if ( m_pListenSocket && !g_bVMPISDKMode )
		{
			for ( int iTest=m_iListenPort+1; iTest <= VMPI_MASTER_LAST_PORT; iTest++ )
			{
				m_iDownloaderListenPort = iTest;
				if ( m_iDownloaderListenPort == m_iListenPort )
					continue;
					
				m_pDownloaderListenSocket = ThreadedTCP_CreateListener( &m_ConnectionCreator, m_iDownloaderListenPort );
				if ( m_pDownloaderListenSocket )
					break;
			}
		}
	}

	if ( !m_pListenSocket || (!g_bVMPISDKMode && !m_pDownloaderListenSocket) )
	{
		Error( "Can't bind a listen socket in port range [%d, %d].", VMPI_MASTER_PORT_FIRST, VMPI_MASTER_PORT_LAST );
	}


	// Create a socket to broadcast from unless we're in the SDK in which case we don't broadcast.
	m_bPatching = false;
	if ( m_RunMode == VMPI_RUN_NETWORKED && !g_bVMPISDKMode )
	{
		m_pSocket = CreateIPSocket();
		if ( !m_pSocket->BindToAny( 0 ) )
			Error( "MPI_Init_Master: can't bind a socket" );

		if ( bPatchMode )
		{
			m_bPatching = true;

			const char *pArg = VMPI_FindArg( argc, argv, "-mpi_PatchVersion", "0" );
			float iPatchVersion = atof( pArg );
			if ( iPatchVersion <= 0 || iPatchVersion >= ((1 << 15) - 1) )
			{
				Error( "-mpi_PatchVersion <val> -  val must be between 1.0 and 32767.0" );
			}
			
			V_strncpy( m_BroadcastInfo.m_PatchVersion, pArg, sizeof( m_BroadcastInfo.m_PatchVersion ) );
		}
		else
		{
			m_BroadcastInfo.m_PatchVersion[0] = 0;
		}

		// Come up with a unique job ID.
		m_BroadcastInfo.m_JobID[0] = GetCurMicrosecondsAndSleep( 1 );
		m_BroadcastInfo.m_JobID[1] = GetCurMicrosecondsAndSleep( 1 );
		m_BroadcastInfo.m_JobID[2] = GetCurMicrosecondsAndSleep( 1 );
		m_BroadcastInfo.m_JobID[3] = GetCurMicrosecondsAndSleep( 1 );

		const char *pPassword = VMPI_FindArg( argc, argv, "-mpi_pw", "" );
		V_strncpy( m_BroadcastInfo.m_Password, pPassword ? pPassword : "", sizeof( m_BroadcastInfo.m_Password ) );
		V_strncpy( m_BroadcastInfo.m_WorkerExeFilename, dependencyInfo.m_OriginalExeFilename, sizeof( m_BroadcastInfo.m_WorkerExeFilename ) );

		// Store the command-line args.
		m_BroadcastInfo.m_Args.Purge();
		for ( int i=1; i < argc; i++ )
		{
			m_BroadcastInfo.m_Args.AddToTail( CopyString( argv[i] ) );
		}
		// 0th arg is the exe name.
		m_BroadcastInfo.m_Args.InsertBefore( 0, CopyString( m_BroadcastInfo.m_WorkerExeFilename ) );
		
		// Now add arguments for each file they need to transmit. The service will use this to get all the files from the master before it starts the app.
		for ( int i=0; i < dependencyInfo.m_Files.Count(); i++ )
		{
			m_BroadcastInfo.m_Args.InsertAfter( 0, "-mpi_file" );
			m_BroadcastInfo.m_Args.InsertAfter( 1, CopyString( dependencyInfo.m_Files[i]->m_Name ) );
		}
		
		// Add -mpi_filebase so it can use absolute paths with the filesystem so we get the exact right set of files.
		m_BroadcastInfo.m_Args.InsertAfter( 0, "-mpi_filebase" );
		m_BroadcastInfo.m_Args.InsertAfter( 1, CopyString( dependencyInfo.m_DependencyFilesDir ) );

		if ( bPatchMode )
		{
			GetPatchWorkerList( argc, argv );
		}
	}
	

	// Add ourselves as the first process (rank 0).
	m_ConnectionCreator.CreateNewHandler();

	// Initiate as many connections as we can for a few seconds.
	m_LastSendTime = Plat_MSTime() - MASTER_BROADCAST_INTERVAL*2;
	
	
	m_hShutdownEvent.Init( false, false );
	m_hShutdownReply.Init( false, false );

	DWORD dwThreadID = 0;
	m_hThread = CreateThread( 
		NULL,
		0,
		&CMasterBroadcaster::StaticThreadFn,
		this,
		0,
		&dwThreadID );

	if ( m_hThread )
	{
		SetThreadPriority( m_hThread, THREAD_PRIORITY_HIGHEST );
		return true;
	}
	else
	{
		return false;
	}
}


void CMasterBroadcaster::BuildBroadcastPacket( bf_write &buf )
{
	// Broadcast out to tell all the machines we want workers.
	buf.WriteByte( VMPI_PROTOCOL_VERSION );

	buf.WriteString( m_BroadcastInfo.m_Password );
	
	if ( m_BroadcastInfo.m_PatchVersion[0] == 0 )
		buf.WriteByte( VMPI_LOOKING_FOR_WORKERS );
	else
		buf.WriteByte( VMPI_SERVICE_PATCH );

	buf.WriteString( m_BroadcastInfo.m_PatchVersion );
	buf.WriteLong( m_iListenPort );	// Tell the port that we're listening on.
	buf.WriteLong( m_BroadcastInfo.m_JobID[0] );
	buf.WriteLong( m_BroadcastInfo.m_JobID[1] );
	buf.WriteLong( m_BroadcastInfo.m_JobID[2] );
	buf.WriteLong( m_BroadcastInfo.m_JobID[3] );
	buf.WriteWord( m_BroadcastInfo.m_Args.Count() + 2 );

	// Write the alternate exe name.
	buf.WriteString( m_BroadcastInfo.m_WorkerExeFilename );

	// Write the machine name of the master into the command line. It's ignored by the code, but it's useful
	// if a job crashes the workers - by looking at the command line in vmpi_service, you can see who ran the job.
	buf.WriteString( "-mpi_MasterName" );
	buf.WriteString( VMPI_GetLocalMachineName() );
	
	for ( int i=1; i < m_BroadcastInfo.m_Args.Count(); i++ )
		buf.WriteString( m_BroadcastInfo.m_Args[i] );

	// This used to be bForcePatch. Service v4.0.1 and above ignore it and always patch when
	// told to regardless of the version number, so this byte is just for legacy code.
	buf.WriteByte( true );

	buf.WriteShort( m_iDownloaderListenPort );	// Tell the port that we're listening for downloaders on.
}

bool CMasterBroadcaster::Update()
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	// Don't accept any more connections when we've hit the limit.
	int nActiveConnections, nServiceDownloaders;
	CountActiveConnections( &nActiveConnections, &nServiceDownloaders );
	if ( nActiveConnections >= m_nMaxWorkers )
	{
		m_nRegisteredWorkerInviteIndex = -1;	// Stop pinging individual workers to help us do work

		return false;
	}

#if USE_VMPI_REGISTRY
	double flNow = Plat_FloatTime();
	if ( m_flLastWorkerQueryTime < 0.0f ||
		 flNow - m_flLastWorkerQueryTime >= MASTER_WORKER_QUERY_INTERVAL )
	{
		VMPI_QueryRegistryForWorkers( m_registeredWorkers );
		m_flLastWorkerQueryTime = flNow;
	}
#endif

	// Only broadcast our presence so often.
	if ( m_pSocket )
	{									  
		DWORD curTime = Plat_MSTime();
		if ( curTime - m_LastSendTime >= MASTER_BROADCAST_INTERVAL )
		{
			char packetData[VMPI_PACKET_SIZE];
			bf_write packetBuf( "packetBuf", packetData, sizeof( packetData ) );
			BuildBroadcastPacket( packetBuf );

			for ( int iBroadcastPort=VMPI_SERVICE_PORT; iBroadcastPort <= VMPI_LAST_SERVICE_PORT; iBroadcastPort++ )
			{
				if ( m_bPatching )
				{
					// Only send to this specific list of workers if necessary.
					for ( int i=0; i < m_PatchWorkerIPs.Count(); i++ )
					{
						CIPAddr addr = m_PatchWorkerIPs[i];
						addr.port = iBroadcastPort;						
						bool bSent = m_pSocket->SendTo( &addr, packetBuf.GetBasePointer(), packetBuf.GetNumBytesWritten() );
						if ( !bSent )
							VMPI_WriteToLogFile( "Error in SendTo trying to send patch request packet!\n" );

						// Give up a little bit of time to the OS, otherwise it will sometimes refuse to send UDP packets after the first few.
						ThreadSleep( 2 );
					}
				}
#if 1 //!USE_VMPI_REGISTRY
				// Keep broadcasting as well as pinging workers through the registry for the time being.
				else
				{
					m_pSocket->Broadcast( packetBuf.GetBasePointer(), packetBuf.GetNumBytesWritten(), iBroadcastPort );
				}
#endif
			}
			
#if USE_VMPI_REGISTRY
			// Indicate that we should incrementally start pinging workers found in the registry to do work for us
			if ( m_nRegisteredWorkerInviteIndex < 0 )
			{
				m_nRegisteredWorkerInviteIndex = 0;
			}
#endif

			// We don't want them to keep patching over and over.
			m_PatchWorkerIPs.Purge();		 

			m_LastSendTime = curTime;
		}	
	}

	// First look for normal workers.
	IThreadedTCPSocket *pNewConn = NULL;
	bool bRet = m_pListenSocket->Update( &pNewConn, 0 );

	// Now look for downloaders.
	if ( !bRet || !pNewConn )
	{									   
		if ( m_pDownloaderListenSocket )
		{
			int nDownloadersAllowed = (m_nMaxWorkers - nActiveConnections) + 8; // Don't allow too many downloaders.
			if ( nServiceDownloaders < nDownloadersAllowed )
				bRet = m_pDownloaderListenSocket->Update( &pNewConn, 0 );
		}
	}

	if ( bRet && pNewConn )
	{
		// Mark this guy as a downloader if necessary.
		CIPAddr remoteAddr = pNewConn->GetRemoteAddr();
		if ( remoteAddr.port >= VMPI_SERVICE_DOWNLOADER_PORT_FIRST && remoteAddr.port <= VMPI_SERVICE_DOWNLOADER_PORT_LAST )
		{
			CVMPIConnection *pVMPIConnection = FindConnectionBySocket( pNewConn, false );
			if ( pVMPIConnection )
				pVMPIConnection->m_bIsAService = true;
		}
		
		// Send this guy all the persistent packets.
		CVMPICriticalSectionLock csLock( &g_PersistentPacketsCS );
		csLock.Lock();

		int nPersistentPacket = 1;
		ITCPSocketHandler *pHandler = pNewConn->GetHandler();

		FOR_EACH_LL( g_PersistentPackets, iPacket )
		{
			PersistentPacket *pPacket = g_PersistentPackets[iPacket];
			VMPI_SendData( pPacket->Base(), pPacket->Count(), pHandler->GetConnectionID() );

			if ( g_bSuperSpewEnabled )
			{
				if ( pPacket->Count() > 2 )
				{
					const uint8 *pData = (uint8*)pPacket->Base();
					char szPacketID[64], szSubPacketID[64];
					CVMPIPacketIDReg::Lookup( pData[0], pData[1], szPacketID, sizeof( szPacketID ), szSubPacketID, sizeof( szSubPacketID ) );
					VMPI_SuperSpew( "Sending persistent packet %d. PacketID: %s, SubPacketID: %s.\n", nPersistentPacket++, szPacketID, szSubPacketID );
				}
				else
				{
					VMPI_SuperSpew( "Sending a really short persistent packet (%d bytes).\n", pPacket->Count() );
				}
			}
		}

		AskSomeWorkersToConnect();
		UpdateActiveConnectionsText();
		return true;
	}
	else
	{
		return AskSomeWorkersToConnect();
	}
}

bool CMasterBroadcaster::AskSomeWorkersToConnect()
{
	if ( m_bPatching )
	{
		return false;
	}
	
	if ( m_nRegisteredWorkerInviteIndex < 0 )
	{
		// Nothing to do
		return false;
	}

	// The index points to the next worker we should invite
	if ( m_nRegisteredWorkerInviteIndex >= m_registeredWorkers.Count() )
	{
		// Done with this round
		m_nRegisteredWorkerInviteIndex = -1;
		return false;
	}

	char packetData[ 1024 ];
	bf_write packetBuf( "packetBuf", packetData, sizeof( packetData ) );
	BuildBroadcastPacket( packetBuf );

	VMPIWorkerInfo_t &workerInfo = m_registeredWorkers[ m_nRegisteredWorkerInviteIndex ];
	bool bSent = m_pSocket->SendTo( &workerInfo.addr, packetBuf.GetBasePointer(), packetBuf.GetNumBytesWritten() );
	if ( !bSent )
	{
		VMPI_WriteToLogFile( "Error in SendTo trying to send packet!\n" );
	}

	m_nRegisteredWorkerInviteIndex++;
	if ( m_nRegisteredWorkerInviteIndex >= m_registeredWorkers.Count() )
	{
		// Done with this round
		m_nRegisteredWorkerInviteIndex = -1;
	}

	// Give up a little bit of time to the OS, otherwise it will sometimes refuse to send UDP packets after the first few.
	//ThreadSleep( 1 );	// Seems to not be necessary anymore in 2017 on Win10? --Thorsten

	return m_nRegisteredWorkerInviteIndex >= 0;
}


void CMasterBroadcaster::ThreadFn()
{
	// Update every 100ms or until the main thread tells us to go away.
	while ( WaitForSingleObject( m_hShutdownEvent.GetEventHandle(), 20 ) == WAIT_TIMEOUT )
	{
		DWORD startTime = GetTickCount();
		while ( Update() && (GetTickCount() - startTime) < 500 )
		{
		}
	}
	m_hShutdownReply.SetEvent();
}


DWORD CMasterBroadcaster::StaticThreadFn( LPVOID lpParameter )
{
	((CMasterBroadcaster*)lpParameter)->ThreadFn();
	return 0;
}


void CMasterBroadcaster::Term()
{
	// Shutdown the update thread.
	if ( m_hThread )
	{
		m_hShutdownEvent.SetEvent();
		WaitForSingleObject( m_hThread, INFINITE );
		CloseHandle( m_hThread );
		m_hThread = 0;
	}
	
	if ( m_pSocket )
	{
		m_pSocket->Release();
		m_pSocket = NULL;
	}

	if ( m_pListenSocket )
	{
		m_pListenSocket->Release();
		m_pListenSocket = NULL;
	}

	if ( m_pDownloaderListenSocket )
	{
		m_pDownloaderListenSocket->Release();
		m_pDownloaderListenSocket = NULL;
	}

	m_iListenPort = -1;
	m_iDownloaderListenPort = -1;
}


int CMasterBroadcaster::GetListenPort() const
{
	return m_iListenPort;
}


int CMasterBroadcaster::GetMaxWorkers() const
{
	return m_nMaxWorkers;
}


void CMasterBroadcaster::IncreaseMaxWorkers( int count )
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	m_nMaxWorkers = min( MAX_VMPI_CONNECTIONS, m_nMaxWorkers + count );
}
	
void CMasterBroadcaster::SetPassword( const char *pPassword )
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();
	V_strncpy( m_BroadcastInfo.m_Password, pPassword, sizeof( m_BroadcastInfo.m_Password ) );
}

void CMasterBroadcaster::SetNoTimeoutOption()
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	// Don't re-add the option if it's already there.	
	for ( int i=1; i < m_BroadcastInfo.m_Args.Count(); i++ )
	{
		if ( V_stricmp( m_BroadcastInfo.m_Args[i], VMPI_GetParamString( mpi_NoTimeout ) ) == 0 )
			return;
	}
	
	m_BroadcastInfo.m_Args.InsertAfter( 0, (char*)VMPI_GetParamString( mpi_NoTimeout ) );
}


CMasterBroadcaster g_MasterBroadcaster;


// ---------------------------------------------------------------------------------------- //
// CDispatchReg.
// ---------------------------------------------------------------------------------------- //

CDispatchReg::CDispatchReg( int iPacketID, VMPIDispatchFn fn )
{
	Assert( iPacketID >= 0 && iPacketID < MAX_VMPI_PACKET_IDS );
	Assert( !g_VMPIDispatch[iPacketID] );
	g_VMPIDispatch[iPacketID] = fn;
}


void VMPI_HandleTimingWait_Worker()
{
	if ( VMPI_IsParamUsed( mpi_TimingWait ) )
	{
		Msg( "-mpi_TimingWait specified. Waiting for master to start..." );
		
		// Wait for the signal to go.
		while ( !g_bTimingWaitDone )
		{
			VMPI_DispatchNextMessage( 50 );
		}
		
		Msg( "\n ");
	}
}


void VMPI_HandleTimingWait_Master()
{
	if ( VMPI_IsParamUsed( mpi_TimingWait ) )
	{
		Msg( "-mpi_TimingWait specified. Waiting for a keypress to continue... " );
		getch();
		Msg( "\n" );
		
		unsigned char cPacket[2] = { VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_TIMING_WAIT_DONE };
		VMPI_SendData( cPacket, sizeof( cPacket ), VMPI_PERSISTENT );
	}
}


// ---------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------- //

bool MPI_Init_Worker( int &argc, char **&argv, const CIPAddr &masterAddr, bool bConnectingAsService )
{
	g_bMPIMaster = false;

	// Make a connector to try connect to the master.
	CVMPIConnectionCreator connectionCreator;

	int iFirstPort = VMPI_WORKER_PORT_FIRST;
	int iLastPort =	VMPI_WORKER_PORT_LAST;
	if ( bConnectingAsService )
	{
		iFirstPort = VMPI_SERVICE_DOWNLOADER_PORT_FIRST;
		iLastPort = VMPI_SERVICE_DOWNLOADER_PORT_LAST;
	}

	// Now wait for a connection.
	int nAttempts = 1;
Retry:;

	ITCPConnectSocket *pConnectSocket = NULL;
	int iPort;
	for ( iPort=iFirstPort; iPort <= iLastPort; iPort++ )
	{
		pConnectSocket = ThreadedTCP_CreateConnector(
			masterAddr, 
			CIPAddr( 0, 0, 0, 0, iPort ),
			&connectionCreator );

		if ( pConnectSocket )
			break;
	}
	if ( !pConnectSocket )
	{
		Error( "Can't bind a port in range [%d, %d].", iFirstPort, iLastPort );
	}


	CWaitTimer wait( 3 );
	while ( 1 )
	{
		IThreadedTCPSocket *pSocket = NULL;
		if ( pConnectSocket->Update( &pSocket, 100 ) )
		{
			if ( pSocket )
			{
				// Send the master our machine name.
				VMPI_SendMachineNameTo( VMPI_MASTER_ID );
				
				// Verify that the exe is correct.
				VMPI_ReceiveExeName();

				if ( g_bVMPISDKMode )
				{
					VMPI_ReceiveCommandLine();
				
					CommandLine()->CreateCmdLine( g_WorkerCommandLine.Count(), g_WorkerCommandLine.Base() );
					argc = g_WorkerCommandLine.Count();
					argv = g_WorkerCommandLine.Base();
				}

				ParseOptions( g_WorkerCommandLine.Count(), g_WorkerCommandLine.Base() );
				for ( int i=0; i < g_WorkerCommandLine.Count(); i++ )
				{
					Msg( "arg %d: %s\n", i, g_WorkerCommandLine[i] );
				}

				VMPI_HandleTimingWait_Worker();
				return true;
			}
		}
		else
		{
			pConnectSocket->Release();
			Error( "ITCPConnectSocket::Update() errored out" );
		}
		
		if( wait.ShouldKeepWaiting() )
			Sleep( 100 );
		else
			break;
	};

	// Never made a connection, shucks.
	pConnectSocket->Release();

	if ( VMPI_IsParamUsed( mpi_Retry ) )
	{
		Msg( "%s found. Retrying connection to %d.%d.%d.%d:%d (attempt %d).\n", VMPI_GetParamString( mpi_Retry ), masterAddr.ip[0], masterAddr.ip[1], masterAddr.ip[2], masterAddr.ip[3], masterAddr.port, nAttempts++ );
		goto Retry;
	}

	Warning( "MPI_Init_Worker() failed\n" );
	return false;
}


bool SpawnLocalWorker( int argc, char **argv, int iListenPort, bool bShowConsoleWindow )
{
	char commandLine[4096];
	commandLine[0] = 0;

	// Add the -mpi_worker argument in, then launch the process.
	for ( int i=0; i < 9999999; i++ )
	{
		char argStr[512];

		if ( i == 1 )
		{
			V_snprintf( argStr, sizeof( argStr ), "-mpi_worker 127.0.0.1:%d ", iListenPort );
			V_strncat( commandLine, argStr, sizeof( commandLine ), COPY_ALL_CHARACTERS );
			V_strncat( commandLine, "-allowdebug ", sizeof( commandLine ), COPY_ALL_CHARACTERS );

			// Add -mpi_SDKMode if it's needed. This would mostly only occur in a debugging situation
			// (someone running out of rel using -mpi_AutoLocalWorker).
			if ( VMPI_IsSDKMode() && !VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_SDKMode ), "" ) )
			{
				V_strncat( commandLine, VMPI_GetParamString( mpi_SDKMode ), sizeof( commandLine ), COPY_ALL_CHARACTERS );
			}
		}
		
		if ( i >= argc )
			break;

		V_snprintf( argStr, sizeof( argStr ), "\"%s\" ", argv[i] );
		V_strncat( commandLine, argStr, sizeof( commandLine ), COPY_ALL_CHARACTERS );
	}

	char workingDir[1024];
	if ( !_getcwd( workingDir, sizeof( workingDir ) ) )
	{
		Warning( "_getcwd() failed.\n" );
		return false;
	}

	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,							// security
		NULL,
		TRUE,
		(bShowConsoleWindow ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW) | IDLE_PRIORITY_CLASS,	// flags
		NULL,							// environment
		workingDir,						// current directory (use c:\\ because we don't want it to accidentally share
										// DLLs like vstdlib with us).
		&si,
		&pi ) )
	{
		return true;
	}
	else
	{
		char errStr[1024];
		IP_GetLastErrorString( errStr, sizeof( errStr ) );
		Warning( " - ERROR in CreateProcess (%s)!\n", errStr );
		return false;
	}
}


bool InitMaster( int argc, char **argv, const char *pDependencyFilename, VMPIRunMode runMode, bool bPatchMode )
{
	int nMaxWorkers = -1;
	const char *pProcCount = VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_WorkerCount ) );
	if ( pProcCount )
	{
		nMaxWorkers = atoi( pProcCount );
		Warning( "%s: waiting for %d processes to join.\n", VMPI_GetParamString( mpi_WorkerCount ), nMaxWorkers );
	}
	else
	{
		nMaxWorkers = DEFAULT_MAX_WORKERS;
	}
	nMaxWorkers = clamp( nMaxWorkers, 2, MAX_VMPI_CONNECTIONS );


	g_bMPIMaster = true;
	g_nMaxWorkerCount = nMaxWorkers;

	if ( argc <= 0 )
		Error( "MPI_Init_Master: argc <= 0!" );

	ParseOptions( argc, argv );

	// Send the base filename of the exe we're running. Sometimes if we run vvis followed by vrad
	// really quickly, the old vvis workers can connect to the vrad process and mess with it.
	VMPI_SendExeName();
	
	// In SDK mode, the master sends the command line to the workers since 
	// the workers weren't given a full command line by vmpi_service.
	if ( VMPI_IsSDKMode() )
	{
		VMPI_SendCommandLine( argc, argv );
	}

	if ( !g_MasterBroadcaster.Init( argc, argv, pDependencyFilename, nMaxWorkers, runMode, bPatchMode ) )
		return false;

	bool bRet;
	if ( runMode == VMPI_RUN_LOCAL )
	{
		bRet = SpawnLocalWorker( argc, argv, g_MasterBroadcaster.GetListenPort(), false );
	}
	else
	{
		if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_AutoLocalWorker ), "" ) )
		{
			Msg( "%s found. Spawning a local worker automatically.\n", VMPI_GetParamString( mpi_AutoLocalWorker ) );
			SpawnLocalWorker( 1, argv, g_MasterBroadcaster.GetListenPort(), true );
		}		

		bRet = true;
	}

	VMPI_HandleTimingWait_Master();	
	return bRet;
}


void VMPI_InitGlobals( int argc, char **argv, VMPIRunMode runMode )
{
	g_bUseMPI = true;
	g_VMPIRunMode = runMode;

	// Init event objects.
	g_VMPIMessagesEvent.Init( false, false );
	g_ErrorSocketsEvent.Init( false, false );

	// Load this for GetConsoleWindow().
	g_hKernel32DLL = LoadLibrary( "kernel32.dll" );
	if ( g_hKernel32DLL )
	{
		g_pConsoleWndFn = (GetConsoleWndFn)GetProcAddress( g_hKernel32DLL, "GetConsoleWindow" );
	}

	#if defined( _DEBUG )
		
		for ( int iArg=0; iArg < argc; iArg++ )
		{
			Warning( "%s\n", argv[iArg] );
		}
		
		Warning( "\n" );
	
	#endif
}


bool VMPI_CheckForNonSDKExecutables()
{
	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "VMPI_CheckSDKMode -> GetModuleFileName failed." );
	
	V_StripLastDir( baseExeFilename, sizeof( baseExeFilename ) );
	V_AppendSlash( baseExeFilename, sizeof( baseExeFilename ) );
	V_strncat( baseExeFilename, "mysql_wrapper.dll", sizeof( baseExeFilename ) );
	
	// If vmpi_transfer.exe doesn't exist, then we assume we're in SDK mode.
	return ( _access( baseExeFilename, 0 ) == 0 );
}


bool IsValidSDKBinPath( CUtlVector< char* > &outStrings, int *pError )
{
	*pError = 0;

	// Minimum must have drive:/basedir/steamapps/name/sourcesdk/bin/[ep1|orangebox]/bin/exename
	if ( outStrings.Count() < 9 )
	{
		*pError = 0;
		return false;
	}

	if ( V_stricmp( outStrings[outStrings.Count()-2], "bin" ) != 0 )
	{
		*pError = 1;
		return false;
	}

	if ( V_stricmp( outStrings[outStrings.Count()-5], "sourcesdk" ) != 0 )
	{
		*pError = 2;
		return false;
	}

	if ( V_stricmp( outStrings[outStrings.Count()-7], "steamapps" ) != 0 )
	{
		*pError = 3;
		return false;
	}

	// Check the last-access date on clientregistry.blob
	char baseSteamPath[MAX_PATH];
	V_strncpy( baseSteamPath, outStrings[0], sizeof( baseSteamPath) );
	for ( int i=1; i < outStrings.Count() - 7; i++ )
	{
		V_AppendSlash( baseSteamPath, sizeof( baseSteamPath ) );
		V_strncat( baseSteamPath, outStrings[i], sizeof( baseSteamPath ) );
	}

	char blobPath[MAX_PATH];
	V_ComposeFileName( baseSteamPath, "ClientRegistry.blob", blobPath, sizeof( blobPath ) );
	struct _stat results;
	if ( _stat( blobPath, &results ) != 0 )
	{
		*pError = 4;
		return false;
	}

	time_t curTime;
	time( &curTime );
	int nSecondsSinceLastSteamAccess = curTime - results.st_mtime;
	int nSecondsPerDay = 60 * 60 * 24;
	int nMaxDaysUnaccessed = 10;
	if ( nSecondsSinceLastSteamAccess > nSecondsPerDay*nMaxDaysUnaccessed )
	{
		*pError = 5; // NOTE: don't change this error code because the outer function checks for it.
		return false;
	}

	// Check for some of the files under sourcesdk_content.
	char sourcesdkContentPath[MAX_PATH];
	V_strncpy( sourcesdkContentPath, outStrings[0], sizeof( sourcesdkContentPath ) );
	for ( int i=1; i < outStrings.Count() - 5; i++ )
	{
		V_AppendSlash( sourcesdkContentPath, sizeof( sourcesdkContentPath ) );
		V_strncat( sourcesdkContentPath, outStrings[i], sizeof( sourcesdkContentPath ) );
	}
	V_AppendSlash( sourcesdkContentPath, sizeof( sourcesdkContentPath ) );
	V_strncat( sourcesdkContentPath, "sourcesdk_content", sizeof( sourcesdkContentPath ) );

	char tempFilename[MAX_PATH], mapsrcFilename[MAX_PATH];
	V_snprintf( tempFilename, sizeof( tempFilename ), "cstrike%cmapsrc", CORRECT_PATH_SEPARATOR );
	V_ComposeFileName( sourcesdkContentPath, tempFilename, mapsrcFilename, sizeof( mapsrcFilename ) );
	if ( _access( mapsrcFilename, 0 ) != 0 )
	{
		*pError = 6;
		return false;
	}	

	return true;
}

void VerifyValidSDKMode()
{
	// Make sure we're running out of the SourceSDK directory and that our SDK directories are filled out.
	char baseExeFilename[MAX_PATH];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "VerifyValidSDKMode: GetModuleFileName failed." );
	V_FixSlashes( baseExeFilename );

	char strSlash[2] = {CORRECT_PATH_SEPARATOR, 0};
	CSplitString outStrings( baseExeFilename, strSlash );

	int err;
	if ( !IsValidSDKBinPath( outStrings, &err ) )
	{
		if ( err == 5 )
			Error( "VMPI running in SDK mode but Steam hasn't been run recently. Please run Steam and retry." );
		else
			Error( "VMPI running in SDK mode but incorrect SDK install detected (error %d).", err );
	}
}

void VMPI_CheckSDKMode( int argc, char **argv )
{
	if ( g_bIsRunningVMPITransfer )
	{
		// Since vmpi_transfer.exe is used for 
		g_bVMPISDKMode = false;
		g_bVMPISDKModeSet = true;
		return;
	}
	else
	{
		g_bVMPISDKMode = !VMPI_CheckForNonSDKExecutables();
		g_bVMPISDKModeSet = true;
	}
		
	// Also check for -mpi_sdkmode (only used in testing).
	if ( !g_bVMPISDKMode )
	{
		if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_SDKMode ), "" ) )
			g_bVMPISDKMode = true;
	}

	if ( g_bVMPISDKMode )
	{
		VerifyValidSDKMode();
	}

	if ( g_bVMPISDKMode )
	{	
		Msg( "VMPI running in SDK mode.\n" );
	}
}


void VMPI_SetupAutoRestartParameters( int argc, char **argv )
{
	if ( VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_AutoRestart ) ) )
	{	
		g_OriginalCommandLineParameters.SetCount( argc );
		for ( int i=0; i < argc; i++ )
		{
			g_OriginalCommandLineParameters[i] = CopyString( argv[i] );
		}
	}
}


bool VMPI_HandleAutoRestart()
{
	if ( g_OriginalCommandLineParameters.Count() == 0 )
		return true;

	Msg( "%s found. Auto-restarting.\n", VMPI_GetParamString( mpi_AutoRestart ) );
	DWORD curPriority = GetPriorityClass( GetCurrentProcess() );

	char commandLine[1024*8];
	commandLine[0] = 0;

	// Add the -mpi_worker argument in, then launch the process.
	for ( int i=0; i < g_OriginalCommandLineParameters.Count(); i++ )
	{
		char argStr[512];
		V_snprintf( argStr, sizeof( argStr ), "\"%s\" ", g_OriginalCommandLineParameters[i] );
		V_strncat( commandLine, argStr, sizeof( commandLine ), COPY_ALL_CHARACTERS );
	}

	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	if ( CreateProcess( 
		NULL, 
		commandLine, 
		NULL,							// security
		NULL,
		TRUE,
		CREATE_NEW_CONSOLE | curPriority,	// flags
		NULL,							// environment
		NULL,
		&si,
		&pi ) )
	{
		g_OriginalCommandLineParameters.Purge();
		return true;
	}
	else
	{
		char errStr[1024];
		IP_GetLastErrorString( errStr, sizeof( errStr ) );
		Warning( " - ERROR in CreateProcess (%s)!\n", errStr );
		return false;
	}
}


bool VMPI_Init( 
	int &argc, 
	char **&argv, 
	const char *pDependencyFilename, 
	VMPI_Disconnect_Handler handler,
	VMPIRunMode runMode,
	bool bConnectingAsService
	)
{
	if ( handler )
		VMPI_AddDisconnectHandler( handler );

	VMPI_SetupAutoRestartParameters( argc, argv );

	VMPI_CheckSDKMode( argc, argv );
	VMPI_InitGlobals( argc, argv, runMode );

	// Init libcurl
	if ( curl_global_init( CURL_GLOBAL_DEFAULT ) != 0 )
	{
		Warning( "Failed to initialize curl library!\n" );
	}

	// Were we launched by the vmpi service as a worker?
	const char *pMasterIP = VMPI_FindArg( argc, argv, VMPI_GetParamString( mpi_Worker ), NULL );
	if ( pMasterIP )
	{
		CIPAddr addr;
		addr.port = VMPI_MASTER_FIRST_PORT;
		if ( !ConvertStringToIPAddr( pMasterIP, &addr ) )
			Error( "Unable to parse or resolve master IP (%s).\n", pMasterIP );
		
		return MPI_Init_Worker( argc, argv, addr, bConnectingAsService );
	}
	else
	{
		if ( !pDependencyFilename )
		{
			Error( "VMPI started as master, but no dependency filename specified.\n" );
			return false;
		}
		
		return InitMaster( argc, argv, pDependencyFilename, runMode, false );
	}
}


void VMPI_Init_PatchMaster( int argc, char **argv )
{
	const char *pPatchDirectory = VMPI_FindArg( argc, argv, "-mpi_PatchDirectory", NULL );
	if ( !pPatchDirectory )
		Error( "-mpi_PatchDirectory <dir> must be specified if using -PatchHost mode." );

	VMPI_InitGlobals( argc, argv, VMPI_RUN_NETWORKED );
		
	InitMaster( argc, argv, pPatchDirectory, VMPI_RUN_NETWORKED, true );
}


void VMPI_Finalize()
{
	g_MasterBroadcaster.Term();

	DistributeWork_Cancel();

	// Get rid of all the sockets.
	for ( int iConn=0; iConn < g_nConnections; iConn++ )
	{
		if ( g_Connections[iConn] != NULL )
			g_Connections[iConn]->DeleteSelf();
	}

	g_nConnections = 0;

	// Get rid of all the packets.
	FOR_EACH_LL( g_VMPIMessages, i )
	{
		g_VMPIMessages[i]->Release();
	}
	g_VMPIMessages.Purge();

	g_PersistentPackets.PurgeAndDeleteElements();
	
	// Get rid of the message buffers
	g_DispatchBuffers.Purge();

	if ( g_hKernel32DLL )
	{
		FreeLibrary( g_hKernel32DLL );
		g_hKernel32DLL = NULL;
	}
	
	g_WorkerCommandLine.PurgeAndDeleteElements();

	curl_global_cleanup();

	VMPI_HandleAutoRestart();
}


VMPIRunMode VMPI_GetRunMode()
{
	return g_VMPIRunMode;
}


VMPIFileSystemMode VMPI_GetFileSystemMode()
{
	return g_VMPIFileSystemMode;
}


int VMPI_GetCurrentNumberOfConnections()
{
	return g_nConnections;
}


void InternalHandleSocketErrors()
{
	// Copy the list of sockets with errors into a local array so we can handle all the errors outside
	// the mutex, thus avoiding potential deadlock if any error handlers call Error().
	CUtlVector<CVMPIConnection*> errorSockets;
	
	CVMPICriticalSectionLock csLock( &g_ErrorSocketsCS );
	csLock.Lock();

		errorSockets.SetCount( g_ErrorSockets.Count() );
		int iCur = 0;
		FOR_EACH_LL( g_ErrorSockets, i )
		{
			errorSockets[iCur++] = g_ErrorSockets[i];
		}

		g_ErrorSockets.Purge();

	csLock.Unlock();

	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	// Handle the errors.
	for ( int i=0; i < errorSockets.Count(); i++ )
	{	
		CVMPIConnection *pConnection = errorSockets[i];
		AUTO_LOCK( g_ConnectionMutexes[ pConnection->GetConnectionID() ] );	// This prevents us from deleting a connection right under the main thread in VMPI_SendData.
		pConnection->HandleDisconnect();
		pConnection->Release( true );
	}
	connectionsLock.Unlock();

	UpdateActiveConnectionsText();
}


void VMPI_HandleSocketErrors( unsigned long timeout )
{
	DWORD ret = WaitForSingleObject( g_ErrorSocketsEvent.GetEventHandle(), timeout );
	if ( ret == WAIT_OBJECT_0 )
	{
		InternalHandleSocketErrors();
	}
}


// If bWait is false, then this function returns false immediately if there are no messages waiting.
bool VMPI_GetNextMessage( MessageBuffer *pBuf, int *pSource, unsigned long startTimeout )
{
	HANDLE handles[2] = { g_ErrorSocketsEvent.GetEventHandle(), g_VMPIMessagesEvent.GetEventHandle() };
	
	DWORD startTime = Plat_MSTime();
	DWORD timeout = startTimeout;

	while ( 1 )
	{
		DWORD ret = WaitForMultipleObjects( ARRAYSIZE( handles ), handles, FALSE, timeout );
		if ( ret == WAIT_TIMEOUT )
		{
			return false;
		}
		else if ( ret == WAIT_OBJECT_0 )
		{
			// A socket had an error. Handle all socket errors.
			InternalHandleSocketErrors();

			// Update the timeout.
			DWORD delta = Plat_MSTime() - startTime;
			if ( delta >= startTimeout )
				return false;

			timeout = startTimeout - delta;
			continue;
		}
		else if ( ret == (WAIT_OBJECT_0 + 1) )
		{
			// Read out the next message.
			CVMPICriticalSectionLock csLock( &g_VMPIMessagesCS );
			csLock.Lock();

GrabNextMessage:;
				int iHead = g_VMPIMessages.Head();
				CTCPPacket *pPacket = g_VMPIMessages[iHead];
				g_VMPIMessages.Remove( iHead );

				// Set the event again if there are more messages waiting.
				const char *pBase = pPacket->GetData();
				if ( pPacket->GetLen() >= 6 && (unsigned char)pBase[0] == VMPI_INTERNAL_PACKET_ID && (unsigned char)pBase[1] == VMPI_INTERNAL_SUBPACKET_GROUPED_PACKET )
				{
					// Ok, this is a grouped packet. Split it out into a bunch of separate packets.
					CUtlVector<CTCPPacket*> groupedPackets;
					int iCurOffset = 2;
					while ( (iCurOffset+4) <= pPacket->GetLen() )
					{
						int curPacketLen = *((int*)&pBase[iCurOffset]);
						if ( iCurOffset + curPacketLen > pPacket->GetLen() )
							Error( "Invalid chunked packet\n" );
							
						iCurOffset += 4;
											 
						CTCPPacket *pChunkPacket = (CTCPPacket*)malloc( sizeof( CTCPPacket ) + curPacketLen - 1 );
						pChunkPacket->m_Len = curPacketLen;
						pChunkPacket->m_UserData = pPacket->m_UserData;
						memcpy( pChunkPacket->m_Data, &pBase[iCurOffset], curPacketLen );
						groupedPackets.AddToTail( pChunkPacket );					
						
						iCurOffset += curPacketLen;
					}
					
					for ( int i=0; i < groupedPackets.Count(); i++ )
					{
						g_VMPIMessages.AddToHead( groupedPackets[groupedPackets.Count() - i - 1] );
					}
					pPacket->Release();
					goto GrabNextMessage;
				}
				else
				{
					if ( g_VMPIMessages.Count() > 0 )
						g_VMPIMessagesEvent.SetEvent();
				}

			csLock.Unlock();

			// Copy it into their message buffer.
			pBuf->setLen( pPacket->GetLen() );
			memcpy( pBuf->data, pPacket->GetData(), pPacket->GetLen() );

			if ( g_bSuperSpewEnabled )
			{
				char szPacketID[64], szSubPacketID[64];
				CVMPIPacketIDReg::Lookup( 
					(pPacket->GetLen() >= 1) ? pBuf->data[0] : -1, 
					(pPacket->GetLen() >= 2) ? pBuf->data[1] : -1, 
					szPacketID, sizeof( szPacketID ), szSubPacketID, sizeof( szSubPacketID ) );

				VMPI_SuperSpew( "Received a packet (packetID %s, subPacketID %s)\n",
					szPacketID, szSubPacketID );
			}

			*pSource = pPacket->GetUserData();
			Assert( *pSource >= 0 && *pSource < g_nConnections );
			
			// Update global stats about how much data we've received.
			++g_nMessagesReceived;
			g_nBytesReceived += pPacket->GetLen() + 4;	// (4 bytes extra for the packet length)
			
			// Free the memory associated with the packet.
			pPacket->Release();
			return true;
		}
		else
		{
			Error( "VMPI_GetNextMessage: WaitForSingleObject returned %lu", ret );
			return false;
		}
	}
}


bool VMPI_InternalDispatch( MessageBuffer *pBuf, int iSource )
{
	if ( pBuf->getLen() >= 1 &&
		pBuf->data[0] >= 0 && pBuf->data[0] < MAX_VMPI_PACKET_IDS &&
		g_VMPIDispatch[pBuf->data[0]] )
	{
		return g_VMPIDispatch[ pBuf->data[0] ]( pBuf, iSource, pBuf->data[0] );
		
	}
	else
	{
		return false;
	}
}

bool VMPI_DispatchNextMessage( unsigned long timeout )
{
	MessageBuffer *pBuf = NULL;
	if ( !g_DispatchBuffers.PopItem( &pBuf ) )
	{
		pBuf = new MessageBuffer();
	}

	bool bRetval = true;
	while ( 1 )
	{
		int iSource;
		if ( VMPI_GetNextMessage( pBuf, &iSource, timeout ) )
		{
			if ( VMPI_InternalDispatch( pBuf, iSource ) )
			{
				break;
			}
			else
			{
				// Workers running in service mode don't hook anything except filesystem stuff, so if they happen to be sent something, no problem.
				if ( !VMPI_IsProcAService( iSource ) )
				{
					// Oops! What is this packet?
					Assert( false );
				}
			}
		}
		else
		{
			bRetval = false;
			break;
		}
	}

	g_DispatchBuffers.PushItem( pBuf );
	return bRetval;
}


bool VMPI_DispatchUntil( MessageBuffer *pBuf, int *pSource, int packetID, int subPacketID, bool bWait )
{
	while ( 1 )
	{
		if ( !VMPI_GetNextMessage( pBuf, pSource, bWait ? VMPI_TIMEOUT_INFINITE : 0 ) )
			return false;
		
		if ( !VMPI_InternalDispatch( pBuf, *pSource ) )
		{		
			if ( pBuf->getLen() >= 1 && (unsigned char)pBuf->data[0] == packetID )
			{
				if ( subPacketID == -1 )
					return true;

				if ( pBuf->getLen() >= 2 && (unsigned char)pBuf->data[1] == subPacketID )
					return true;
			}
		
			// Oops! What is this packet?
			// Note: the most common case where this happens is if it finishes a BuildFaceLights run
			// and is in an AppBarrier and one of the workers is still finishing up some work given to it.
			// It'll be waiting for a barrier packet, and it'll get results. In that case, the packet should
			// be discarded like we do here, so maybe this assert won't be necessary.
			//Assert( false );
		}
	}
}


bool VMPI_SendData( void *pData, int nBytes, int iDest, int fVMPISendFlags )
{
	return VMPI_SendChunks( &pData, &nBytes, 1, iDest, fVMPISendFlags );
}


inline bool VMPI_FilterPacketsForServiceDownloader( CVMPIConnection *pConnection, void const * const *pChunks, const int *pChunkLengths, int nChunks )
{
	if ( pConnection->m_bIsAService )
	{
		// Find the first byte and treat that as the packet ID.
		for ( int i=0; i < nChunks; i++ )
		{
			int nChunkLen = pChunkLengths[i];
			if ( pChunkLengths[i] >= 1 )
			{
				unsigned char *pChunkData = (unsigned char*)pChunks[i];
				unsigned char cPacketID = pChunkData[0];

				if ( cPacketID == VMPI_INTERNAL_PACKET_ID || cPacketID == VMPI_SHARED_PACKET_ID || cPacketID == VMPI_PACKETID_FILESYSTEM )
				{
					return false;
				}
				else
				{
					if ( g_bSuperSpewEnabled )
					{
						char szPacketID[64], szSubPacketID[64];
						CVMPIPacketIDReg::Lookup( cPacketID, ( nChunkLen >= 2 ? pChunkData[1] : -1 ), szPacketID, sizeof( szPacketID ), szSubPacketID, sizeof( szSubPacketID ) );
						VMPI_SuperSpew( "VMPI_FilterPacketsForServiceDownloader filtered packetID %s, subPacketID %s\n", szPacketID, szSubPacketID );
					}

					return true;
				}
			}
		}
	}
	return false;
}


void VMPI_GroupPackets( CVMPIConnection *pConn, void const * const *pChunks, const int *pChunkLengths, int nChunks )
{
	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	// First add the header.
	if ( pConn->m_GroupedChunks.Count() == 0 )
	{
		pConn->m_GroupedChunks.AddToTail( g_GroupedPacketHeader );
		pConn->m_GroupedChunkLengths.AddToTail( sizeof( g_GroupedPacketHeader ) );
	}

	// Collate the chunks.
	int nTotalLength = 0;
	for ( int i=0; i < nChunks; i++ )
		nTotalLength += pChunkLengths[i];
	
	char *pOut = new char[nTotalLength + 4];
	*((int*)pOut) = nTotalLength;
	int iOutByte = 4;
	for ( int i=0; i < nChunks; i++ )
	{
		memcpy( &pOut[iOutByte], pChunks[i], pChunkLengths[i] );
		iOutByte += pChunkLengths[i];
	}
	
	pConn->m_GroupedChunks.AddToTail( pOut );
	pConn->m_GroupedChunkLengths.AddToTail( nTotalLength + 4 );
}


void VMPI_FlushGroupedPackets( unsigned long msInterval )
{
	if ( msInterval != 0 )
	{
		unsigned long curTime = Plat_MSTime();
		if ( curTime - g_LastFlushGroupedPacketsTime < msInterval )
			return;
		g_LastFlushGroupedPacketsTime = curTime;
	}

	CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
	connectionsLock.Lock();

	for ( int i=0; i < g_nConnections; i++ )
	{
		CVMPIConnection *pConn = g_Connections[i];
		
		if ( !pConn )
			continue;
		
		IThreadedTCPSocket *pSocket = pConn->GetSocket();
		if ( !pSocket || pConn->m_GroupedChunks.Count() == 0 )
			continue;

		pSocket->SendChunks( pConn->m_GroupedChunks.Base(), pConn->m_GroupedChunkLengths.Base(), pConn->m_GroupedChunks.Count() );
		
		// Free the chunks.
		for ( int j=1; j < pConn->m_GroupedChunks.Count(); j++ )
		{
			free( pConn->m_GroupedChunks[j] );
		}
		pConn->m_GroupedChunks.RemoveAll();
		pConn->m_GroupedChunkLengths.RemoveAll();
	}	
}


bool VMPI_SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks, int iDest, int fVMPISendFlags )
{
	if ( iDest == VMPI_SEND_TO_ALL )
	{
		// Don't want new connections while in here!
		CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
		connectionsLock.Lock();

		for ( int i=0; i < g_nConnections; i++ )
			VMPI_SendChunks( pChunks, pChunkLengths, nChunks, i );

		return true;
	}
	else if ( iDest == VMPI_PERSISTENT )
	{
		// Don't want new connections while in here!
		CVMPICriticalSectionLock connectionsLock( &g_ConnectionsCS );
		connectionsLock.Lock();

		CVMPICriticalSectionLock csLock( &g_PersistentPacketsCS );
		csLock.Lock();

		// Send the packet to everyone.
		for ( int i=0; i < g_nConnections; i++ )
			VMPI_SendChunks( pChunks, pChunkLengths, nChunks, i );

		// Remember to send it to the new workers.
		if ( iDest == VMPI_PERSISTENT )
		{
			PersistentPacket *pNew = new PersistentPacket;
			for ( int i=0; i < nChunks; i++ )
				pNew->AddMultipleToTail( pChunkLengths[i], (const char*)pChunks[i] );

			g_PersistentPackets.AddToTail( pNew );
		}
	
		return true;
	}
	else
	{
		g_nMessagesSent++;
		g_nBytesSent += 4; // for message tag.
		for ( int i=0; i < nChunks; i++ )
			g_nBytesSent += pChunkLengths[i];

		AUTO_LOCK( g_ConnectionMutexes[iDest] );	// This prevents CVMPIConnectionCreator::ReleaseHandle from deleting the connection from under us.
		CVMPIConnection *pConnection = g_Connections[iDest];

		if ( pConnection )
		{
			// If it's a service downloader, only send certain packet IDs.
			if ( VMPI_FilterPacketsForServiceDownloader( pConnection, pChunks, pChunkLengths, nChunks ) )
				return true;

			IThreadedTCPSocket *pSocket = pConnection->GetSocket();
			if ( !pSocket )
				return false;
			
			if ( g_bGroupPackets && (fVMPISendFlags & k_eVMPISendFlags_GroupPackets) )
			{
				VMPI_GroupPackets( pConnection, pChunks, pChunkLengths, nChunks );
				return true;
			}
			else
			{
				return pSocket->SendChunks( pChunks, pChunkLengths, nChunks );
			}
		}
		else
		{
			return false;
		}
	}
}


bool VMPI_Send2Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, int iDest, int fVMPISendFlags )
{
	const void *pChunks[2] = { pChunk1, pChunk2 };
	int len[2] = { chunk1Len, chunk2Len };
	return VMPI_SendChunks( pChunks, len, ARRAYSIZE( pChunks ), iDest, fVMPISendFlags );
}


bool VMPI_Send3Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, const void *pChunk3, int chunk3Len, int iDest, int fVMPISendFlags )
{
	const void *pChunks[3] = { pChunk1, pChunk2, pChunk3 };
	int len[3] = { chunk1Len, chunk2Len, chunk3Len };
	return VMPI_SendChunks( pChunks, len, ARRAYSIZE( pChunks ), iDest, fVMPISendFlags );
}


void VMPI_AddDisconnectHandler( VMPI_Disconnect_Handler handler )
{
	g_DisconnectHandlers.AddToTail( handler );
}


CVMPIConnection* GetConnection( int procID )
{
	Assert( procID >= 0 && procID < g_nConnections );
	return g_Connections[procID];
}


bool VMPI_IsProcValid( int procID )
{
	if ( procID < 0 || procID >= g_nConnections )
	{
		return false;
	}

	if ( g_Connections[ procID ] == NULL )
	{
		return false;
	}

	return true;
}


bool VMPI_IsProcConnected( int procID )
{
	if ( procID < 0 || procID >= g_nConnections )
	{
		Assert( false );
		return false;
	}

	if ( g_Connections[ procID ] == NULL )
	{
		return false;
	}

	return g_Connections[procID]->GetSocket() != NULL;
}

bool VMPI_IsProcAService( int procID )
{
	if ( procID < 0 || procID >= g_nConnections )
	{
		Assert( false );
		return false;
	}

	return g_Connections[procID]->m_bIsAService;
}

void VMPI_Sleep( unsigned long ms )
{
	Sleep( ms );
}


const char* VMPI_GetMachineName( int iProc )
{
	if ( g_bMPIMaster && iProc == VMPI_MASTER_ID )
		return VMPI_GetLocalMachineName();
	
	if ( iProc < 0 || iProc >= g_nConnections )
	{
		Assert( false );
		return "invalid index";
	}

	if ( g_Connections[iProc] == NULL )
	{
		return "invalid index";
	}

	return g_Connections[iProc]->GetMachineName();
}


void VMPI_SetMachineName( int iProc, const char *pName )
{
	if ( iProc < 0 || iProc >= g_nConnections )
	{
		Assert( false );
		return;
	}

	if ( g_Connections[ iProc ] == NULL )
	{
		return;
	}

	g_Connections[iProc]->SetMachineName( pName );
}


bool VMPI_HasMachineNameBeenSet( int iProc )
{
	if ( iProc < 0 || iProc >= g_nConnections )
	{
		Assert( false );
		return false;
	}

	if ( g_Connections[ iProc ] == NULL )
	{
		return false;
	}

	return g_Connections[iProc]->HasMachineNameBeenSet();
}


const char* VMPI_GetLocalMachineName()
{
	static char cName[MAX_COMPUTERNAME_LENGTH+1];
	DWORD len = sizeof( cName );
	if ( GetComputerName( cName, &len ) )
		return cName;
	else
		return "(error in GetComputerName)";
}


unsigned long VMPI_GetJobWorkerID( int iProc )
{
	return GetConnection( iProc )->m_JobWorkerID;
}


void VMPI_SetJobWorkerID( int iProc, unsigned long jobWorkerID )
{
	GetConnection( iProc )->m_JobWorkerID = jobWorkerID;
}


void VMPI_GetCurrentStage( char *pOut, int strLen )
{
	CVMPICriticalSectionLock csLock( &g_CurrentStageCS );
	csLock.Lock();
	V_strncpy( pOut, g_CurrentStageString, strLen );
}
	

void VMPI_SetCurrentStage( const char *pCurStage )
{
	CVMPICriticalSectionLock csLock( &g_CurrentStageCS );
	csLock.Lock();
	V_strncpy( g_CurrentStageString, pCurStage, sizeof( g_CurrentStageString ) );
}


void VMPI_InviteDebugWorkers()
{
	// Only allow workers with password set to debugworker.
	g_MasterBroadcaster.SetPassword( "debugworker" );

	// Disable timeouts so they can sit in the debugger.
	g_MasterBroadcaster.SetNoTimeoutOption();
	ThreadedTCP_EnableTimeouts( false );

	// Let in some more workers.	
	g_MasterBroadcaster.IncreaseMaxWorkers( 25 );
}


bool VMPI_IsSDKMode()
{
	if ( g_bVMPISDKModeSet )
		return g_bVMPISDKMode;
	else
		return !VMPI_CheckForNonSDKExecutables();
}


const char* VMPI_GetParamString( EVMPICmdLineParam eParam )
{
	if ( eParam <= k_eVMPICmdLineParam_FirstParam || eParam >= k_eVMPICmdLineParam_LastParam )
	{
		Assert( false );
		Warning( "Invalid call: VMPI_GetParamString( %d )\n", eParam );
		return "unknown";
	}
	else
	{
		return g_VMPIParams[eParam].m_pName;
	}
}

int VMPI_GetParamFlags( EVMPICmdLineParam eParam )
{
	if ( eParam <= k_eVMPICmdLineParam_FirstParam || eParam >= k_eVMPICmdLineParam_LastParam )
	{
		Assert( false );
		Warning( "Invalid call: VMPI_GetParamString( %d )\n", eParam );
		return 0;
	}
	else
	{
		return g_VMPIParams[eParam].m_ParamFlags;
	}
}

bool VMPI_IsParamUsed( EVMPICmdLineParam eParam )
{
	int iParam = CommandLine()->FindParm( VMPI_GetParamString( eParam ) );
	return iParam != 0;
}

const char* VMPI_GetParamHelpString( EVMPICmdLineParam eParam )
{
	if ( eParam <= k_eVMPICmdLineParam_FirstParam || eParam >= k_eVMPICmdLineParam_LastParam )
	{
		Assert( false );
		Warning( "Invalid call: VMPI_GetParamHelpString( %d )\n", eParam );
		return "unknown vmpi param";
	}
	else
	{
		return g_VMPIParams[eParam].m_pHelpText;
	}
}

void VMPI_PrintMsgOnMaster( const char *pMessage, ... )
{
	char formatted[2048];
	va_list marker;
	va_start( marker, pMessage );
	V_vsnprintf( formatted, sizeof( formatted ), pMessage, marker );
	va_end( marker );

	if ( VMPI_IsMaster() )
	{
		Msg( "%s\n", pMessage );
	}
	else
	{
		MessageBuffer mb;
		
		char cPacketHeader[2] = {VMPI_INTERNAL_PACKET_ID, VMPI_INTERNAL_SUBPACKET_PRINT_ON_MASTER};
		mb.write( cPacketHeader, sizeof( cPacketHeader ) );
		mb.WriteString( formatted );

		VMPI_SendData( mb.data, mb.getLen(), VMPI_MASTER_ID );
	}
}


//-----------------------------------------------------------------------------------------------
// This code is lifted from S2 networksystem. Maybe it should be moved into tier1 but
// this copy minimizes compile churn
//-----------------------------------------------------------------------------------------------

#if defined( IS_WINDOWS_PC )

#include <Iphlpapi.h>

typedef DWORD( WINAPI *PFN_GetAdaptersInfo )(
	_Out_   PIP_ADAPTER_INFO pAdapterInfo,
	_Inout_ PULONG           pOutBufLen
	);

static PFN_GetAdaptersInfo s_pGetAdaptersInfo;

PFN_GetAdaptersInfo RequireGetAdaptersInfo()
{
	if ( !s_pGetAdaptersInfo )
	{
		HMODULE hMod = ::LoadLibrary( "iphlpapi.dll" );
		if ( !hMod )
		{
			Plat_FatalError( "Unable to resolve GetAdaptersInfo\n" );
		}

		s_pGetAdaptersInfo = (PFN_GetAdaptersInfo)GetProcAddress( hMod, "GetAdaptersInfo" );
		if ( !s_pGetAdaptersInfo )
		{
			Plat_FatalError( "Unable to resolve GetAdaptersInfo\n" );
		}
	}

	return s_pGetAdaptersInfo;
}

#endif

#if defined( PLATFORM_POSIX )
#include "poll.h"
#include "ifaddrs.h"
#endif

//-----------------------------------------------------------------------------
// Calculates all the IPs of the current box.
// IPs are returned in host format.
// WARNING: This code is super-slow, and should only be executed once if possible.
//-----------------------------------------------------------------------------
struct SInterfaceAddrDesc
{
	CUtlString m_Name;
	CUtlString m_Desc;
	netadr_t m_Adr;
};

typedef CUtlVector< SInterfaceAddrDesc > InterfaceAddrDescVec_t;

static InterfaceAddrDescVec_t g_hostIPAddressCache;
static bool g_bHasCachedHostIPs = false;

void GetAllHostIPAddresses( InterfaceAddrDescVec_t &iVec )
{
	iVec.Purge();

	if ( g_bHasCachedHostIPs )
	{
		iVec.AddVectorToTail( g_hostIPAddressCache );
		return;
	}

#if defined( PLATFORM_WINDOWS )

	// Enumerate all the adapters on this box
	byte rgchBuf[ 1024 * 10 ];
	IP_ADAPTER_INFO * pIPAdapterInfo = (IP_ADAPTER_INFO *)rgchBuf;
	ULONG ulSize = Q_ARRAYSIZE( rgchBuf );
	DWORD dwRet = ( *RequireGetAdaptersInfo() )( pIPAdapterInfo, &ulSize );

	if ( NO_ERROR != dwRet )
	{
		return;
	}

	// for each adapter...
	while ( pIPAdapterInfo )
	{
		// for each IP on that adapter...
		for ( IP_ADDR_STRING *pIPAddrString = &pIPAdapterInfo->IpAddressList; pIPAddrString; pIPAddrString = pIPAddrString->Next )
		{
			const char * pchIP = pIPAddrString->IpAddress.String;
			if ( pchIP && pchIP[ 0 ] )
			{
				// get the IP address of this adapter.  (use netadr to parse the string they helpfully return us)
				netadr_t netadr( pchIP );

				// don't add dups
				bool bIsDuplicateIP = false;
				for ( int iIPThisBox = 0; iIPThisBox < iVec.Count(); iIPThisBox++ )
				{
					if ( netadr == iVec[ iIPThisBox ].m_Adr )
					{
						bIsDuplicateIP = true;
						break;
					}
				}

				// ignore adapters with an IP of 0, and ones we already have
				if ( netadr.GetIPHostByteOrder() && !bIsDuplicateIP )
				{
					auto &addrDesc = iVec.Element( iVec.AddToTail() );
					addrDesc.m_Adr = netadr;
					addrDesc.m_Name = pIPAdapterInfo->AdapterName;
					addrDesc.m_Desc = pIPAdapterInfo->Description;
				}
			}
		}

		pIPAdapterInfo = pIPAdapterInfo->Next;
	}

#elif defined( PLATFORM_POSIX ) 

	struct ifaddrs *ifaces = NULL;
	int rc = getifaddrs( &ifaces );
	if ( rc )
	{
		return;
	}

	// Get the nic associated with the default route
	char szNicDefault[ 64 ];
#if defined( PLATFORM_LINUX )
	GetDefaultNic( szNicDefault, sizeof( szNicDefault ) );
#else
	szNicDefault[ 0 ] = 0;
#endif

	// Collect ip addresses. Put the default nic first.
	for ( struct ifaddrs *iface = ifaces; iface; iface = iface->ifa_next )
	{
		if ( iface->ifa_addr && iface->ifa_addr->sa_family == AF_INET && ( ( struct sockaddr_in * )iface->ifa_addr )->sin_addr.s_addr != 0 )
		{
			uint ipAddress = ntohl( ( ( struct sockaddr_in * )iface->ifa_addr )->sin_addr.s_addr );
			if ( ipAddress )
			{
				netadr_t adr;
				adr.SetFromSockadr( iface->ifa_addr );
				SInterfaceAddrDesc addrDesc;
				addrDesc.m_Adr = adr;
				addrDesc.m_Name = iface->ifa_name;

				// Get the ip. If it is the default ip, ensure it is first in the list
				if ( iface->ifa_name )
				{
					if ( V_strcmp( iface->ifa_name, szNicDefault ) == 0 )
					{
						iVec.AddToHead( addrDesc );
					}
					else
					{
						iVec.AddToTail( addrDesc );
					}
				}
			}
		}
	}

	freeifaddrs( ifaces );
#else
#error "net_misc.cpp need GetAllHostIPAddresses implementation"
#endif

	g_hostIPAddressCache.AddVectorToTail( iVec );
	g_bHasCachedHostIPs = true;
}
// End copied code
//-----------------------------------------------------------------------------------------------

bool VMPI_IsThisMyIP( CIPAddr testIP )
{
	netadr_t masterIPAddr;
	masterIPAddr.SetIP( testIP.ip[ 0 ], testIP.ip[ 1 ], testIP.ip[ 2 ], testIP.ip[ 3 ] );

	// Now get all IP addresses of this machine and see if the list contains the IP address of the VMPI master
	InterfaceAddrDescVec_t myAddrs;
	GetAllHostIPAddresses( myAddrs );
	for ( int i = 0; i < myAddrs.Count(); i++ )
	{
		if ( masterIPAddr.CompareAdr( myAddrs[ i ].m_Adr, true ) )
		{
			return true;
		}
	}

	return false;
}

bool VMPI_IsThisWorkerRunningOnMasterMachine()
{
	if ( VMPI_IsMaster() )
	{
		return false;
	}

	if ( VMPI_IsProcValid( VMPI_MASTER_ID ) && VMPI_IsProcConnected( VMPI_MASTER_ID ) )
	{
		CVMPIConnection *pConn = GetConnection( VMPI_MASTER_ID );
		CIPAddr tempIPAddr = pConn->GetSocket()->GetRemoteAddr();
		return VMPI_IsThisMyIP( tempIPAddr );
	}

	return false;
}


static size_t CURLDataReceivedCallback( void *buffer, size_t size, size_t nmemb, void *userp )
{
	CUtlBuffer *pResponseBuf = reinterpret_cast<CUtlBuffer*>( userp );
	pResponseBuf->Put( buffer, int( size * nmemb ) );
	return size * nmemb;
}


void VMPI_QueryRegistryForWorkers( CUtlVector<VMPIWorkerInfo_t> &registeredWorkers )
{
	registeredWorkers.RemoveAll();

	CURL *hCurl = curl_easy_init();
	if ( !hCurl )
	{
		Warning( "Failed to get CURL handle\n" );
		return;
	}

	CFmtStr urlStr( "http://%s:%d/find_workers", VMPI_REGISTRY_WEB_SERVICE_HOST, VMPI_REGISTRY_WEB_SERVICE_PORT );
	curl_easy_setopt( hCurl, CURLOPT_URL, urlStr.Get() );
	curl_easy_setopt( hCurl, CURLOPT_NOPROXY, VMPI_REGISTRY_WEB_SERVICE_HOST );	// Prevent going through our web proxy as that obscures the worker's IP address on the registry's end
	CUtlBuffer responseBuf( 0, 0, CUtlBuffer::TEXT_BUFFER );
	curl_easy_setopt( hCurl, CURLOPT_WRITEFUNCTION, CURLDataReceivedCallback );
	curl_easy_setopt( hCurl, CURLOPT_WRITEDATA, &responseBuf );

	char errorString[ CURL_ERROR_SIZE ];
	curl_easy_setopt( hCurl, CURLOPT_ERRORBUFFER, errorString );

	CURLcode resultCode = curl_easy_perform( hCurl );
	responseBuf.PutChar( '\0' );	// Make sure it's 0-terminated
	curl_easy_cleanup( hCurl );

	if ( resultCode != CURLE_OK )
	{
		Warning( "CURL Error for %s\n%s\n", urlStr.Get(), errorString );
		return;
	}

	// Parse result into worker array
	char str[ 1024 ];
	responseBuf.GetLine( str, 1024 );
	int nWorkerCount = -1;
	sscanf( str, "%d", &nWorkerCount );
	if ( nWorkerCount < 0 )
	{
		Warning( "Bad response from registry?\n" );
		return;
	}

	for ( int i = 0; i < nWorkerCount; i++ )
	{
		responseBuf.GetLine( str, 1024 );
		int nIP[ 4 ];
		int nPort = 0;

		CUtlVector<char *> strings;
		V_SplitString( str, ",", strings );
		if ( strings.Count() < 6 )
		{
			Warning( "Malformed VMPI registry data!\n" );
			return;
		}

		int nNumParsed = sscanf( strings[ 0 ], "%d.%d.%d.%d", &nIP[ 0 ], &nIP[ 1 ], &nIP[ 2 ], &nIP[ 3 ] );
		if ( nNumParsed != 4 )
		{
			Warning( "Malformed VMPI registry data!\n" );
			return;
		}
		nPort = V_atoi( strings[ 1 ] );

		VMPIWorkerInfo_t wi;
		V_strcpy_safe( wi.machineName, strings[ 2 ] );
		wi.addr.Init( nIP[ 0 ], nIP[ 1 ], nIP[ 2 ], nIP[ 3 ], nPort );
		wi.nStatus = V_atoi( strings[ 3 ] );
		wi.nProtocolVersion = V_atoi( strings[ 4 ] );
		V_strcpy_safe( wi.vmpiVersion, strings[ 5 ] );
		registeredWorkers.AddToTail( wi );
		strings.PurgeAndDeleteElements();
	}
}
