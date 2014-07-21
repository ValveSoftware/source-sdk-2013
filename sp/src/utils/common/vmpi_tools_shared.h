//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef VMPI_TOOLS_SHARED_H
#define VMPI_TOOLS_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Packet IDs.
	#define VMPI_SUBPACKETID_DIRECTORIES	0	// qdir directories.
	#define VMPI_SUBPACKETID_DBINFO			1	// MySQL database info.
	#define VMPI_SUBPACKETID_CRASH			3	// A worker saying it crashed.
	#define VMPI_SUBPACKETID_MULTICAST_ADDR	4	// Filesystem multicast address.


class CDBInfo;
class CIPAddr;


// Send/receive the qdir info.
void SendQDirInfo();
void RecvQDirInfo();

void SendDBInfo( const CDBInfo *pInfo, unsigned long jobPrimaryID );
void RecvDBInfo( CDBInfo *pInfo, unsigned long *pJobPrimaryID );

void SendMulticastIP( const CIPAddr *pAddr );
void RecvMulticastIP( CIPAddr *pAddr );

void VMPI_HandleCrash( const char *pMessage, void *pvExceptionInfo, bool bAssert );

// Call this from an exception handler (set by SetUnhandledExceptionHandler).
// uCode			= ExceptionInfo->ExceptionRecord->ExceptionCode.
// pvExceptionInfo	= ExceptionInfo
void VMPI_ExceptionFilter( unsigned long uCode, void *pvExceptionInfo );

void HandleMPIDisconnect( int procID, const char *pReason );


#endif // VMPI_TOOLS_SHARED_H
