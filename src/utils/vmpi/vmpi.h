//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VMPI_H
#define VMPI_H
#ifdef _WIN32
#pragma once
#endif


#include "vmpi_defs.h"
#include "messbuf.h"
#include "iphelpers.h"
#include "tier1/utlvector.h"


// These are called to handle incoming messages. 
// Return true if you handled the message and false otherwise.
// Note: the first byte in each message is the packet ID.
typedef bool (*VMPIDispatchFn)( MessageBuffer *pBuf, int iSource, int iPacketID );

typedef void (*VMPI_Disconnect_Handler)( int procID, const char *pReason );


// Which machine is the master.
#define VMPI_MASTER_ID 0

#define VMPI_SEND_TO_ALL	-2
#define VMPI_PERSISTENT		-3	// If this is set as the destination for a packet, it is sent to all
								// workers, and also to new workers that connect.

#define MAX_VMPI_PACKET_IDS		32


#define VMPI_TIMEOUT_INFINITE	0xFFFFFFFF

#define VMPI_PACKET_SIZE	2048

// Instantiate one of these to register a dispatch.
class CDispatchReg
{
public:
	CDispatchReg( int iPacketID, VMPIDispatchFn fn );
};


// Enums for all the command line parameters.
#define VMPI_PARAM_SDK_HIDDEN	0x0001		// Hidden in SDK mode.

#define VMPI_PARAM( paramName, paramFlags, helpText ) paramName,
enum EVMPICmdLineParam
{
	k_eVMPICmdLineParam_FirstParam=0,
	k_eVMPICmdLineParam_VMPIParam,
	#include "vmpi_parameters.h"
	k_eVMPICmdLineParam_LastParam
};
#undef VMPI_PARAM


// Shared by all the tools.
extern bool	g_bUseMPI;
extern bool g_bMPIMaster;	// Set to true if we're the master in a VMPI session.
extern int g_iVMPIVerboseLevel; // Higher numbers make it spit out more data.

extern bool g_bMPI_Stats;			// Send stats to the MySQL database?
extern bool g_bMPI_StatsTextOutput;	// Send text output in the stats?

// These can be watched or modified to check bandwidth statistics.
extern int g_nBytesSent;
extern int g_nMessagesSent;
extern int g_nBytesReceived;
extern int g_nMessagesReceived;

extern int g_nMulticastBytesSent;
extern int g_nMulticastBytesReceived;

extern int g_nMaxWorkerCount;


enum VMPIRunMode
{
	VMPI_RUN_NETWORKED,
	VMPI_RUN_LOCAL						// Just make a local process and have it do the work.
};


enum VMPIFileSystemMode
{
	VMPI_FILESYSTEM_MULTICAST,		// Multicast out, find workers, have them do work.
	VMPI_FILESYSTEM_BROADCAST,		// Broadcast out, find workers, have them do work.
	VMPI_FILESYSTEM_TCP				// TCP filesystem.
};


// If this precedes the dependency filename, then it will transfer all the files in the specified directory.
#define VMPI_DEPENDENCY_DIRECTORY_TOKEN	'*'


// It's good to specify a disconnect handler here immediately. If you don't have a handler
// and the master disconnects, you'll lockup forever inside a dispatch loop because you
// never handled the master disconnecting.
//
// Note: runMode is only relevant for the VMPI master. The worker always connects to the master
// the same way.
bool VMPI_Init( 
	int &argc, 
	char **&argv, 
	const char *pDependencyFilename, 
	VMPI_Disconnect_Handler handler = NULL, 
	VMPIRunMode runMode = VMPI_RUN_NETWORKED, // Networked or local?,
	bool bConnectingAsService = false
	);

inline bool VMPI_IsEnabled() { return g_bUseMPI; }
inline bool VMPI_IsMaster() { return g_bMPIMaster; }
inline bool VMPI_IsWorker() { return !VMPI_IsMaster(); }

// Used when hosting a patch.
void VMPI_Init_PatchMaster( int argc, char **argv );

void VMPI_Finalize();

VMPIRunMode VMPI_GetRunMode();
VMPIFileSystemMode VMPI_GetFileSystemMode();

// Note: this number can change on the master.
int VMPI_GetCurrentNumberOfConnections();
bool VMPI_IsThisWorkerRunningOnMasterMachine();
bool VMPI_IsThisMyIP( CIPAddr testIP );

// Dispatch messages until it gets one with the specified packet ID.
// If subPacketID is not set to -1, then the second byte must match that as well.
//
// Note: this WILL dispatch packets with matching packet IDs and give them a chance to handle packets first.
//
// If bWait is true, then this function either succeeds or Error() is called. If it's false, then if the first available message
// is handled by a dispatch, this function returns false.
bool VMPI_DispatchUntil( MessageBuffer *pBuf, int *pSource, int packetID, int subPacketID = -1, bool bWait = true );

// This waits for the next message and dispatches it.
// You can specify a timeout in milliseconds. If the timeout expires, the function returns false.
bool VMPI_DispatchNextMessage( unsigned long timeout=VMPI_TIMEOUT_INFINITE );

// This should be called periodically in modal loops that don't call other VMPI functions. This will
// check for disconnected sockets and call disconnect handlers so the app can error out if
// it loses all of its connections. 
//
// This can be used in place of a Sleep() call by specifying a timeout value.
void VMPI_HandleSocketErrors( unsigned long timeout=0 );



enum VMPISendFlags
{
	k_eVMPISendFlags_GroupPackets = 0x0001
};
	
// Use these to send data to one of the machines.
// If iDest is VMPI_SEND_TO_ALL, then the message goes to all the machines.
// Flags is a combination of the VMPISendFlags enums.
bool VMPI_SendData( void *pData, int nBytes, int iDest, int fVMPISendFlags=0 );
bool VMPI_SendChunks( void const * const *pChunks, const int *pChunkLengths, int nChunks, int iDest, int fVMPISendFlags=0 );
bool VMPI_Send2Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, int iDest, int fVMPISendFlags=0 );	// for convenience..
bool VMPI_Send3Chunks( const void *pChunk1, int chunk1Len, const void *pChunk2, int chunk2Len, const void *pChunk3, int chunk3Len, int iDest, int fVMPISendFlags=0 );

// Flush any groups that were queued with k_eVMPISendFlags_GroupPackets.
// If msInterval is > 0, then it will check a timer and only flush that often (so you can call this a lot, and have it check).
void VMPI_FlushGroupedPackets( unsigned long msInterval=0 ); 

// This registers a function that gets called when a connection is terminated ungracefully.
void VMPI_AddDisconnectHandler( VMPI_Disconnect_Handler handler );

// is this a valid process
bool VMPI_IsProcValid( int procID );

// Returns false if the process has disconnected ungracefully (disconnect handlers
// would have been called for it too).
bool VMPI_IsProcConnected( int procID );

// Returns true if the process is just a service (in which case it should only get file IO traffic).
bool VMPI_IsProcAService( int procID );

// Simple wrapper for Sleep() so people can avoid including windows.h
void VMPI_Sleep( unsigned long ms );

// VMPI sends machine names around first thing.
const char* VMPI_GetLocalMachineName();
const char* VMPI_GetMachineName( int iProc );
bool VMPI_HasMachineNameBeenSet( int iProc );

// Returns 0xFFFFFFFF if the ID hasn't been set.
unsigned long VMPI_GetJobWorkerID( int iProc );
void VMPI_SetJobWorkerID( int iProc, unsigned long jobWorkerID );

// Search a command line to find arguments. Looks for pName, and if it finds it, returns the
// argument following it. If pName is the last argument, it returns pDefault. If it doesn't
// find pName, returns NULL.
const char* VMPI_FindArg( int argc, char **argv, const char *pName, const char *pDefault = "" );

// (Threadsafe) get and set the current stage. This info winds up in the VMPI database.
void VMPI_GetCurrentStage( char *pOut, int strLen );
void VMPI_SetCurrentStage( const char *pCurStage );

// VMPI is always broadcasting this job in the background.
// This changes the password to 'debugworker' and allows more workers in.
// This can be used if workers are dying on certain work units. Then a programmer
// can run vmpi_service with -superdebug and debug the whole thing.
void VMPI_InviteDebugWorkers();

bool VMPI_IsSDKMode();

// Lookup a command line parameter string.
const char* VMPI_GetParamString( EVMPICmdLineParam eParam );
int VMPI_GetParamFlags( EVMPICmdLineParam eParam );
const char* VMPI_GetParamHelpString( EVMPICmdLineParam eParam );
bool VMPI_IsParamUsed( EVMPICmdLineParam eParam ); // Returns true if the specified parameter is on the command line.

// Can be called from error handlers and if -mpi_Restart is used, it'll automatically restart the process.
bool VMPI_HandleAutoRestart();

// Any worker can print a message on the master with this.
void VMPI_PrintMsgOnMaster( PRINTF_FORMAT_STRING const char *pMessage, ... );

struct VMPIWorkerInfo_t
{
	char machineName[256];
	CIPAddr addr;
	int nStatus;
	int nProtocolVersion;
	char vmpiVersion[32];
};

// Ask the central VMPI registry server for a list of registered VMPI workers.
void VMPI_QueryRegistryForWorkers( CUtlVector<VMPIWorkerInfo_t> &registeredWorkers );

// These are optional debug helpers. When VMPI_SuperSpew is enabled, VMPI can spit out messages
// with strings for packet IDs instead of numbers.
#define VMPI_REGISTER_PACKET_ID( idDefine ) static CVMPIPacketIDReg g_VMPIPacketIDReg_##idDefine( idDefine, -1, #idDefine );
#define VMPI_REGISTER_SUBPACKET_ID( idPacketIDDefine, idSubPacketIDDefine ) static CVMPIPacketIDReg g_VMPISubPacketIDReg_##idSubPacketIDDefine( idPacketIDDefine, idSubPacketIDDefine, #idSubPacketIDDefine );

class CVMPIPacketIDReg
{
public:
	CVMPIPacketIDReg( int nPacketID, int nSubPacketID, const char *pName );

	static void Lookup( int nPacketID, int nSubPacketID, char *pPacketIDString, int nPacketIDStringSize, char *pSubPacketIDString, int nSubPacketIDStringSize );

private:
	int m_nPacketID;
	int m_nSubPacketID;
	const char *m_pName;
	CVMPIPacketIDReg *m_pNext;
};



#endif // VMPI_H
