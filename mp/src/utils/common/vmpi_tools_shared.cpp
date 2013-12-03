//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include <dbghelp.h>
#include "vmpi.h"
#include "cmdlib.h"
#include "vmpi_tools_shared.h"
#include "tier1/strtools.h"
#include "mpi_stats.h"
#include "iphelpers.h"
#include "tier0/minidump.h"


// ----------------------------------------------------------------------------- //
// Globals.
// ----------------------------------------------------------------------------- //

static bool g_bReceivedDirectoryInfo = false;	// Have we gotten the qdir info yet?

static bool g_bReceivedDBInfo = false;
static CDBInfo g_DBInfo;
static unsigned long g_JobPrimaryID;

static int g_nDisconnects = 0;	// Tracks how many remote processes have disconnected ungracefully.


// ----------------------------------------------------------------------------- //
// Shared dispatch code.
// ----------------------------------------------------------------------------- //

bool SharedDispatch( MessageBuffer *pBuf, int iSource, int iPacketID )
{
	char *pInPos = &pBuf->data[2];

	switch ( pBuf->data[1] )
	{
		case VMPI_SUBPACKETID_DIRECTORIES:
		{
			Q_strncpy( gamedir, pInPos, sizeof( gamedir ) );
			pInPos += strlen( pInPos ) + 1;

			Q_strncpy( qdir, pInPos, sizeof( qdir ) );
			
			g_bReceivedDirectoryInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_DBINFO:
		{
			g_DBInfo = *((CDBInfo*)pInPos);
			pInPos += sizeof( CDBInfo );
			g_JobPrimaryID = *((unsigned long*)pInPos);

			g_bReceivedDBInfo = true;
		}
		return true;

		case VMPI_SUBPACKETID_CRASH:
		{
			char const chCrashInfoType = *pInPos;
			pInPos += 2;
			switch ( chCrashInfoType )
			{
			case 't':
				Warning( "\nWorker '%s' dead: %s\n", VMPI_GetMachineName( iSource ), pInPos );
				break;
			case 'f':
				{
					int iFileSize = * reinterpret_cast< int const * >( pInPos );
					pInPos += sizeof( iFileSize );

					// Temp folder
					char const *szFolder = NULL;
					if ( !szFolder ) szFolder = getenv( "TEMP" );
					if ( !szFolder ) szFolder = getenv( "TMP" );
					if ( !szFolder ) szFolder = "c:";

					// Base module name
					char chModuleName[_MAX_PATH], *pModuleName = chModuleName;
					::GetModuleFileName( NULL, chModuleName, sizeof( chModuleName ) / sizeof( chModuleName[0] ) );

					if ( char *pch = strrchr( chModuleName, '.' ) )
						*pch = 0;
					if ( char *pch = strrchr( chModuleName, '\\' ) )
						*pch = 0, pModuleName = pch + 1;

					// Current time
					time_t currTime = ::time( NULL );
					struct tm * pTime = ::localtime( &currTime );

					// Number of minidumps this run
					static int s_numMiniDumps = 0;
					++ s_numMiniDumps;

					// Prepare the filename
					char chSaveFileName[ 2 * _MAX_PATH ] = { 0 };
					sprintf( chSaveFileName, "%s\\vmpi_%s_on_%s_%d%.2d%2d%.2d%.2d%.2d_%d.mdmp",
						szFolder,
						pModuleName,
						VMPI_GetMachineName( iSource ),
						pTime->tm_year + 1900,	/* Year less 2000 */
						pTime->tm_mon + 1,		/* month (0 - 11 : 0 = January) */
						pTime->tm_mday,			/* day of month (1 - 31) */
						pTime->tm_hour,			/* hour (0 - 23) */
						pTime->tm_min,		    /* minutes (0 - 59) */
						pTime->tm_sec,		    /* seconds (0 - 59) */
						s_numMiniDumps
						);

					if ( FILE *fDump = fopen( chSaveFileName, "wb" ) )
					{
						fwrite( pInPos, 1, iFileSize, fDump );
						fclose( fDump );

						Warning( "\nSaved worker crash minidump '%s', size %d byte(s).\n",
							chSaveFileName, iFileSize );
					}
					else
					{
						Warning( "\nReceived worker crash minidump size %d byte(s), failed to save.\n", iFileSize );
					}
				}
				break;
			}
		}
		return true;
	}

	return false;
}

CDispatchReg g_SharedDispatchReg( VMPI_SHARED_PACKET_ID, SharedDispatch );



// ----------------------------------------------------------------------------- //
// Module interfaces.
// ----------------------------------------------------------------------------- //

void SendQDirInfo()
{
	char cPacketID[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DIRECTORIES };

	MessageBuffer mb;
	mb.write( cPacketID, 2 );
	mb.write( gamedir, strlen( gamedir ) + 1 );
	mb.write( qdir, strlen( qdir ) + 1 );

	VMPI_SendData( mb.data, mb.getLen(), VMPI_PERSISTENT );
}


void RecvQDirInfo()
{
	while ( !g_bReceivedDirectoryInfo )
		VMPI_DispatchNextMessage();
}


void SendDBInfo( const CDBInfo *pInfo, unsigned long jobPrimaryID )
{
	char cPacketInfo[2] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_DBINFO };
	const void *pChunks[] = { cPacketInfo, pInfo, &jobPrimaryID };
	int chunkLengths[] = { 2, sizeof( CDBInfo ), sizeof( jobPrimaryID ) };
	
	VMPI_SendChunks( pChunks, chunkLengths, ARRAYSIZE( pChunks ), VMPI_PERSISTENT );
}


void RecvDBInfo( CDBInfo *pInfo, unsigned long *pJobPrimaryID )
{
	while ( !g_bReceivedDBInfo )
		VMPI_DispatchNextMessage();

	*pInfo = g_DBInfo;
	*pJobPrimaryID = g_JobPrimaryID;
}

// If the file is successfully opened, read and sent returns the size of the file in bytes
// otherwise returns 0 and nothing is sent
int VMPI_SendFileChunk( const void *pvChunkPrefix, int lenPrefix, tchar const *ptchFileName )
{
	HANDLE hFile = NULL;
	HANDLE hMapping = NULL;
	void const *pvMappedData = NULL;
	int iResult = 0;

	hFile = ::CreateFile( ptchFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( !hFile || ( hFile == INVALID_HANDLE_VALUE ) )
		goto done;

	hMapping = ::CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
	if ( !hMapping || ( hMapping == INVALID_HANDLE_VALUE ) )
		goto done;

	pvMappedData = ::MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
	if ( !pvMappedData )
		goto done;

	int iMappedFileSize = ::GetFileSize( hFile, NULL );
	if ( INVALID_FILE_SIZE == iMappedFileSize )
		goto done;

	// Send the data over VMPI
	if ( VMPI_Send3Chunks(
		pvChunkPrefix, lenPrefix,
		&iMappedFileSize, sizeof( iMappedFileSize ),
		pvMappedData, iMappedFileSize,
		VMPI_MASTER_ID ) )
		iResult = iMappedFileSize;

	// Fall-through for cleanup code to execute
done:
	if ( pvMappedData )
		::UnmapViewOfFile( pvMappedData );

	if ( hMapping && ( hMapping != INVALID_HANDLE_VALUE ) )
		::CloseHandle( hMapping );

	if ( hFile && ( hFile != INVALID_HANDLE_VALUE ) )
		::CloseHandle( hFile );

	return iResult;
}

void VMPI_HandleCrash( const char *pMessage, void *pvExceptionInfo, bool bAssert )
{
	static LONG crashHandlerCount = 0;
	if ( InterlockedIncrement( &crashHandlerCount ) == 1 )
	{
		Msg( "\nFAILURE: '%s' (assert: %d)\n", pMessage, bAssert );

		// Send a message to the master.
		char crashMsg[4] = { VMPI_SHARED_PACKET_ID, VMPI_SUBPACKETID_CRASH, 't', ':' };

		VMPI_Send2Chunks( 
			crashMsg, 
			sizeof( crashMsg ), 
			pMessage,
			strlen( pMessage ) + 1,
			VMPI_MASTER_ID );

		// Now attempt to create a minidump with the given exception information
		if ( pvExceptionInfo )
		{
			struct _EXCEPTION_POINTERS *pvExPointers = ( struct _EXCEPTION_POINTERS * ) pvExceptionInfo;
			tchar tchMinidumpFileName[_MAX_PATH] = { 0 };
			bool bSucceededWritingMinidump = WriteMiniDumpUsingExceptionInfo(
				pvExPointers->ExceptionRecord->ExceptionCode,
				pvExPointers,
				( MINIDUMP_TYPE )( MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData ),
				// ( MINIDUMP_TYPE )( MiniDumpWithDataSegs | MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData | MiniDumpWithPrivateReadWriteMemory  ),
				// ( MINIDUMP_TYPE )( MiniDumpNormal ),
				tchMinidumpFileName );
			if ( bSucceededWritingMinidump )
			{
				crashMsg[2] = 'f';
				VMPI_SendFileChunk( crashMsg, sizeof( crashMsg ), tchMinidumpFileName );
				::DeleteFile( tchMinidumpFileName );
			}
		}

		// Let the messages go out.
		Sleep( 500 );
	}

	InterlockedDecrement( &crashHandlerCount );
}


// This is called if we crash inside our crash handler. It just terminates the process immediately.
LONG __stdcall VMPI_SecondExceptionFilter( struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	TerminateProcess( GetCurrentProcess(), 2 );
	return EXCEPTION_EXECUTE_HANDLER; // (never gets here anyway)
}


void VMPI_ExceptionFilter( unsigned long uCode, void *pvExceptionInfo )
{
	// This is called if we crash inside our crash handler. It just terminates the process immediately.
	SetUnhandledExceptionFilter( VMPI_SecondExceptionFilter );

	//DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;

	#define ERR_RECORD( name ) { name, #name }
	struct
	{
		int code;
		char *pReason;
	} errors[] =
	{
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
		ERR_RECORD( EXCEPTION_ARRAY_BOUNDS_EXCEEDED ),
		ERR_RECORD( EXCEPTION_BREAKPOINT ),
		ERR_RECORD( EXCEPTION_DATATYPE_MISALIGNMENT ),
		ERR_RECORD( EXCEPTION_FLT_DENORMAL_OPERAND ),
		ERR_RECORD( EXCEPTION_FLT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_FLT_INEXACT_RESULT ),
		ERR_RECORD( EXCEPTION_FLT_INVALID_OPERATION ),
		ERR_RECORD( EXCEPTION_FLT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_FLT_STACK_CHECK ),
		ERR_RECORD( EXCEPTION_FLT_UNDERFLOW ),
		ERR_RECORD( EXCEPTION_ILLEGAL_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_IN_PAGE_ERROR ),
		ERR_RECORD( EXCEPTION_INT_DIVIDE_BY_ZERO ),
		ERR_RECORD( EXCEPTION_INT_OVERFLOW ),
		ERR_RECORD( EXCEPTION_INVALID_DISPOSITION ),
		ERR_RECORD( EXCEPTION_NONCONTINUABLE_EXCEPTION ),
		ERR_RECORD( EXCEPTION_PRIV_INSTRUCTION ),
		ERR_RECORD( EXCEPTION_SINGLE_STEP ),
		ERR_RECORD( EXCEPTION_STACK_OVERFLOW ),
		ERR_RECORD( EXCEPTION_ACCESS_VIOLATION ),
	};

	int nErrors = sizeof( errors ) / sizeof( errors[0] );
	int i=0;
	char *pchReason = NULL;
	char chUnknownBuffer[32];
	for ( i; ( i < nErrors ) && !pchReason; i++ )
	{
		if ( errors[i].code == uCode )
			pchReason = errors[i].pReason;
	}

	if ( i == nErrors )
	{
		sprintf( chUnknownBuffer, "Error code 0x%08X", uCode );
		pchReason = chUnknownBuffer;
	}
	
	VMPI_HandleCrash( pchReason, pvExceptionInfo, true );

	TerminateProcess( GetCurrentProcess(), 1 );
}


void HandleMPIDisconnect( int procID, const char *pReason )
{
	int nLiveWorkers = VMPI_GetCurrentNumberOfConnections() - g_nDisconnects - 1;

	// We ran into the size limit before and it wasn't readily apparent that the size limit had
	// been breached, so make sure to show errors about invalid packet sizes..
	bool bOldSuppress = g_bSuppressPrintfOutput;
	g_bSuppressPrintfOutput = ( Q_stristr( pReason, "invalid packet size" ) == 0 );

		Warning( "\n\n--- WARNING: lost connection to '%s' (%s).\n", VMPI_GetMachineName( procID ), pReason );
		
		if ( g_bMPIMaster )
		{
			Warning( "%d workers remain.\n\n", nLiveWorkers );

			++g_nDisconnects;
			/*
			if ( VMPI_GetCurrentNumberOfConnections() - g_nDisconnects <= 1 )
			{
				Error( "All machines disconnected!" );
			}
			*/
		}
		else
		{
			VMPI_HandleAutoRestart();
			Error( "Worker quitting." );
		}
	
	g_bSuppressPrintfOutput = bOldSuppress;
}


