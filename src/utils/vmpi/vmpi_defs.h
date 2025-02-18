//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VMPI_DEFS_H
#define VMPI_DEFS_H
#ifdef _WIN32
#pragma once
#endif


// This goes in front of all packets.
#define VMPI_PROTOCOL_VERSION		5

// This represents the protocol between the service and its UI.
#define VMPI_SERVICE_UI_PROTOCOL_VERSION	1

// NOTE: the service version (embedded in vmpi_service.exe as a resource) is the version
// that is used to apply patches.
#define VMPI_SERVICE_IDS_VERSION_STRING	102	// This matches IDS_VERSION_STRING in vmpi_service.exe.

// Known packet IDs in various systems.
#define VMPI_PACKETID_FILESYSTEM	0	// The file system reserves this packet ID.
										// All application traffic must set its first byte to something other
										// than this value.
#define VMPI_SHARED_PACKET_ID		10


// This value is put in the RunningTimeMS until the job is finished. This is how
// the job_search app knows if a job never finished.
#define RUNNINGTIME_MS_SENTINEL		0xFEDCBAFD



#define VMPI_SERVICE_NAME_INTERNAL	"VMPI"
#define VMPI_SERVICE_NAME			"Valve MPI Service"

// Stuff in the registry goes under here (in HKEY_LOCAL_MACHINE).
#define VMPI_SERVICE_KEY				"Software\\Valve\\VMPI"
#define SERVICE_INSTALL_LOCATION_KEY	"InstallLocation"

// The VMPI service listens on one of these ports to talk to the UI.
#define VMPI_SERVICE_FIRST_UI_PORT	23300
#define VMPI_SERVICE_LAST_UI_PORT	23310

// Port numbers that the master will use to broadcast unless -mpi_port is used.
#define VMPI_MASTER_FIRST_PORT		23311
#define VMPI_MASTER_LAST_PORT		23330


// Packet IDs for vmpi_service to talk to UI clients.
#define VMPI_SERVICE_TO_UI_CONSOLE_TEXT		0	// Print some text to the UI's console.
#define VMPI_SERVICE_TO_UI_STATE			1	// Updates state reflecting whether it's idle, busy, etc.
#define VMPI_SERVICE_TO_UI_PATCHING			2	// Updates state reflecting whether it's idle, busy, etc.
#define VMPI_SERVICE_TO_UI_EXIT				3	// Updates state reflecting whether it's idle, busy, etc.
	
	// Application state.. these are communicated between the service and the UI.
	enum
	{
		VMPI_SERVICE_STATE_IDLE=0,
		VMPI_SERVICE_STATE_BUSY,
		VMPI_SERVICE_STATE_DISABLED
	};
#define VMPI_SERVICE_DISABLE			2	// Stop waiting for jobs..
#define VMPI_SERVICE_ENABLE				3
#define VMPI_SERVICE_UPDATE_PASSWORD	4	// New password.
#define VMPI_SERVICE_EXIT				5	// User chose "exit" from the menu. Kill the service.
#define VMPI_SERVICE_SKIP_CSX_JOBS		6
#define VMPI_SERVICE_SCREENSAVER_MODE	7
#define VMPI_SERVICE_PIPE_STDOUT		8	//enable piping of stdout of vmpi jobs to the vmpi console. Otherwise all you get is stderr


// The worker service waits on this range of ports.
#define VMPI_SERVICE_PORT			23397
#define VMPI_LAST_SERVICE_PORT		(VMPI_SERVICE_PORT + 15)


#define VMPI_WORKER_PORT_FIRST		22340
#define VMPI_WORKER_PORT_LAST		22350

// The VMPI service downloader is still a worker but it uses this port range so the 
// master knows it's just downloading the exes.
#define VMPI_SERVICE_DOWNLOADER_PORT_FIRST		22351
#define VMPI_SERVICE_DOWNLOADER_PORT_LAST		22360

// Give it a small range so they can have multiple masters running.
#define VMPI_MASTER_PORT_FIRST		21140
#define VMPI_MASTER_PORT_LAST		21145
#define VMPI_MASTER_FILESYSTEM_BROADCAST_PORT	21146


// VMPI Registry server settings
#define VMPI_REGISTRY_WEB_SERVICE_HOST "vmpi-registry.valvesoftware.com"
#define VMPI_REGISTRY_WEB_SERVICE_PORT 80
#define VMPI_REGISTRY_REGISTRATION_UPDATE_INTERVAL 600.0	// vmpi_service worker machines advertise themselves with the registry every 10 minutes


// Protocol.

// The message format is:
// - VMPI_PROTOCOL_VERSION
// - null-terminated password string (or VMPI_PASSWORD_OVERRIDE followed by a zero to process it regardless of pw).
// - packet ID
// - payload


#define VMPI_PASSWORD_OVERRIDE		-111


#define VMPI_MESSAGE_BASE			71


// This is the broadcast message from the main (rank 0) process looking for workers.
#define VMPI_LOOKING_FOR_WORKERS		(VMPI_MESSAGE_BASE+0)

// This is so an app can find out what machines are running the service.
#define VMPI_PING_REQUEST				(VMPI_MESSAGE_BASE+2)
#define VMPI_PING_RESPONSE				(VMPI_MESSAGE_BASE+3)

// This tells the service to quit.
#define VMPI_STOP_SERVICE			(VMPI_MESSAGE_BASE+6)

// This tells the service to kill any process it has running.
#define VMPI_KILL_PROCESS			(VMPI_MESSAGE_BASE+7)

// This tells the service to patch itself.
#define VMPI_SERVICE_PATCH			(VMPI_MESSAGE_BASE+8)

// Sent back to the master via UDP to tell it if its job has started and ended.
#define VMPI_NOTIFY_START_STATUS	(VMPI_MESSAGE_BASE+9)
#define VMPI_NOTIFY_END_STATUS		(VMPI_MESSAGE_BASE+10)

#define VMPI_FORCE_PASSWORD_CHANGE	(VMPI_MESSAGE_BASE+11)

#define VMPI_ENABLE_SCREENSAVER_MODE	(VMPI_MESSAGE_BASE+12)
#define VMPI_DISABLE_SCREENSAVER_MODE	(VMPI_MESSAGE_BASE+13)

#define VMPI_LOG_FILE_REQUEST		(VMPI_MESSAGE_BASE+14)

// These states are sent from the service to the services browser.
#define VMPI_STATE_IDLE					0
#define VMPI_STATE_BUSY					1
#define VMPI_STATE_PATCHING				2
#define VMPI_STATE_DISABLED				3
#define VMPI_STATE_SCREENSAVER_DISABLED	4
#define VMPI_STATE_DOWNLOADING			5


#endif // VMPI_DEFS_H
