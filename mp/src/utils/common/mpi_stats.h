//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MPI_STATS_H
#define MPI_STATS_H
#ifdef _WIN32
#pragma once
#endif


// The VMPI stats module reports a bunch of statistics to a MySQL server, and the
// stats can be used to trace and graph a compile session.
//

// Call this as soon as possible to initialize spew hooks.
void VMPI_Stats_InstallSpewHook();

//
// pDBServerName is the hostname (or dotted IP address) of the MySQL server to connect to.
// pBSPFilename is the last argument on the command line.
// pMachineIP is the dotted IP address of this machine.
// jobID is an 8-byte unique identifier for this job.
//
bool VMPI_Stats_Init_Master( const char *pHostName, const char *pDBName, const char *pUserName, const char *pBSPFilename, unsigned long *pDBJobID );
bool VMPI_Stats_Init_Worker( const char *pHostName, const char *pDBName, const char *pUserName, unsigned long DBJobID );
void VMPI_Stats_Term();

// Add a generic text event to the database.
void VMPI_Stats_AddEventText( const char *pText );

class CDBInfo
{
public:
	char			m_HostName[128];
	char			m_DBName[128];
	char			m_UserName[128];
};

// If you're the master, this loads pDBInfoFilename, sends that info to the workers, and 
// connects to the database.
//
// If you're a worker, this waits for the DB info, then connects to the database.
void StatsDB_InitStatsDatabase( 
	int argc, 
	char **argv, 
	const char *pDBInfoFilename );

// The database gives back a unique ID for the job.
unsigned long StatsDB_GetUniqueJobID();

// Get the worker ID (used for the JobWorkerID fields in the database).
unsigned long VMPI_Stats_GetJobWorkerID();


#endif // MPI_STATS_H
