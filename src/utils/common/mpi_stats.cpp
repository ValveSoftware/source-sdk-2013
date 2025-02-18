//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// Nasty headers!
#include "MySqlDatabase.h"
#include "tier1/strtools.h"
#include "vmpi.h"
#include "vmpi_dispatch.h"
#include "mpi_stats.h"
#include "cmdlib.h"
#include "imysqlwrapper.h"
#include "threadhelpers.h"
#include "vmpi_tools_shared.h"
#include "tier0/icommandline.h"

/*

-- MySQL code to create the databases, create the users, and set access privileges.
-- You only need to ever run this once.

create database vrad;

use mysql;

create user vrad_worker;
create user vmpi_browser;

-- This updates the "user" table, which is checked when someone tries to connect to the database.
grant select,insert,update on vrad.* to vrad_worker;
grant select on vrad.* to vmpi_browser;
flush privileges;

/*

-- SQL code to (re)create the tables.

-- Master generates a unique job ID (in job_master_start) and sends it to workers.
-- Each worker (and the master) make a job_worker_start, link it to the primary job ID, 
--     get their own unique ID, which represents that process in that job.
-- All JobWorkerID fields link to the JobWorkerID field in job_worker_start. 

-- NOTE: do a "use vrad" or "use vvis" first, depending on the DB you want to create.


use vrad;


drop table job_master_start;
create table job_master_start ( 
	JobID					INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,	index id( JobID, MachineName(5) ),
	BSPFilename				TINYTEXT NOT NULL,
	StartTime				TIMESTAMP NOT NULL,
	MachineName				TEXT NOT NULL,
	RunningTimeMS			INTEGER UNSIGNED NOT NULL,
	NumWorkers				INTEGER UNSIGNED NOT NULL default 0
	);

drop table job_master_end;
create table job_master_end (
	JobID					INTEGER UNSIGNED NOT NULL,					PRIMARY KEY ( JobID ),
	NumWorkersConnected		SMALLINT UNSIGNED NOT NULL,
	NumWorkersDisconnected	SMALLINT UNSIGNED NOT NULL,
	ErrorText				TEXT NOT NULL
	);

drop table job_worker_start;
create table job_worker_start (
	JobWorkerID				INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,	
	index index_jobid( JobID ),
	index index_jobworkerid( JobWorkerID ),
	
	JobID					INTEGER UNSIGNED NOT NULL,				-- links to job_master_start::JobID
	IsMaster				BOOL NOT NULL,							-- Set to 1 if this "worker" is the master process.
	RunningTimeMS			INTEGER UNSIGNED NOT NULL default 0,
	MachineName				TEXT NOT NULL,
	WorkerState				SMALLINT UNSIGNED NOT NULL default 0,	-- 0 = disconnected, 1 = connected
	NumWorkUnits			INTEGER UNSIGNED NOT NULL default 0,	-- how many work units this worker has completed
	CurrentStage			TINYTEXT NOT NULL,				-- which compile stage is it on
	Thread0WU				INTEGER NOT NULL default 0,		-- which WU thread 0 is on
	Thread1WU				INTEGER NOT NULL default 0,		-- which WU thread 1 is on
	Thread2WU				INTEGER NOT NULL default 0,		-- which WU thread 2 is on
	Thread3WU				INTEGER NOT NULL default 0		-- which WU thread 3 is on
	);

drop table text_messages;
create table text_messages (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID, MessageIndex ),
	MessageIndex			INTEGER UNSIGNED NOT NULL,
	Text					TEXT NOT NULL
	);

drop table graph_entry;
create table graph_entry (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID ),
	MSSinceJobStart			INTEGER UNSIGNED NOT NULL,
	BytesSent				INTEGER UNSIGNED NOT NULL,
	BytesReceived			INTEGER UNSIGNED NOT NULL
	);

drop table events;
create table events (
	JobWorkerID				INTEGER UNSIGNED NOT NULL,					index id( JobWorkerID ),
	Text					TEXT NOT NULL
	);
*/



// Stats set by the app.
int		g_nWorkersConnected = 0;
int		g_nWorkersDisconnected = 0;


DWORD	g_StatsStartTime;

CMySqlDatabase	*g_pDB = NULL;

IMySQL			*g_pSQL = NULL;
CSysModule		*g_hMySQLDLL = NULL;

char	g_BSPFilename[256];

bool			g_bMaster = false;
unsigned long	g_JobPrimaryID = 0;	// This represents this job, but doesn't link to a particular machine.
unsigned long	g_JobWorkerID = 0;	// A unique key in the DB that represents this machine in this job.
char			g_MachineName[MAX_COMPUTERNAME_LENGTH+1] = {0};

unsigned long	g_CurrentMessageIndex = 0;


HANDLE	g_hPerfThread = NULL;
DWORD	g_PerfThreadID = 0xFEFEFEFE;
HANDLE	g_hPerfThreadExitEvent = NULL;

// These are set by the app and they go into the database.
extern uint64 g_ThreadWUs[4];

extern uint64 VMPI_GetNumWorkUnitsCompleted( int iProc );


// ---------------------------------------------------------------------------------------------------- //
// This is a helper class to build queries like the stream IO.
// ---------------------------------------------------------------------------------------------------- //

class CMySQLQuery
{
friend class CMySQL;

public:
	// This is like a sprintf, but it will grow the string as necessary.
	void Format( const char *pFormat, ... );

	int Execute( IMySQL *pDB );

private:
	CUtlVector<char>	m_QueryText;
};


void CMySQLQuery::Format( const char *pFormat, ... )
{
	#define QUERYTEXT_GROWSIZE	1024

	// This keeps growing the buffer and calling _vsnprintf until the buffer is 
	// large enough to hold all the data.
	m_QueryText.SetSize( QUERYTEXT_GROWSIZE );
	while ( 1 )
	{
		va_list marker;
		va_start( marker, pFormat );
		int ret = _vsnprintf( m_QueryText.Base(), m_QueryText.Count(), pFormat, marker );
		va_end( marker );

		if ( ret < 0 )
		{
			m_QueryText.SetSize( m_QueryText.Count() + QUERYTEXT_GROWSIZE );
		}
		else
		{
			m_QueryText[ m_QueryText.Count() - 1 ] = 0;
			break;
		}
	}
}


int CMySQLQuery::Execute( IMySQL *pDB )
{
	int ret = pDB->Execute( m_QueryText.Base() );
	m_QueryText.Purge();
	return ret;
}



// ---------------------------------------------------------------------------------------------------- //
// This inserts the necessary backslashes in front of backslashes or quote characters.
// ---------------------------------------------------------------------------------------------------- //

char* FormatStringForSQL( const char *pText )
{
	// First, count the quotes in the string. We need to put a backslash in front of each one.
	int nChars = 0;
	const char *pCur = pText;
	while ( *pCur != 0 )
	{
		if ( *pCur == '\"' || *pCur == '\\' )
			++nChars;
		
		++pCur;
		++nChars;
	}

	pCur = pText;
	char *pRetVal = new char[nChars+1];
	for ( int i=0; i < nChars; )
	{
		if ( *pCur == '\"' || *pCur == '\\' )
			pRetVal[i++] = '\\';
		
		pRetVal[i++] = *pCur;
		++pCur;
	}
	pRetVal[nChars] = 0;

	return pRetVal;
}



// -------------------------------------------------------------------------------- //
// Commands to add data to the database.
// -------------------------------------------------------------------------------- //
class CSQLDBCommandBase : public ISQLDBCommand
{
public:
	virtual ~CSQLDBCommandBase()
	{
	}

	virtual void deleteThis()
	{
		delete this;
	}
};

class CSQLDBCommand_WorkerStats : public CSQLDBCommandBase
{
public:
	virtual int RunCommand()
	{
		int nCurConnections = VMPI_GetCurrentNumberOfConnections();


		// Update the NumWorkers entry.
		char query[2048];
		Q_snprintf( query, sizeof( query ), "update job_master_start set NumWorkers=%d where JobID=%lu",
			nCurConnections,
			g_JobPrimaryID );
		g_pSQL->Execute( query );

		
		// Update the job_master_worker_stats stuff.
		for ( int i=1; i < nCurConnections; i++ )
		{
			unsigned long jobWorkerID = VMPI_GetJobWorkerID( i );

			if ( jobWorkerID != 0xFFFFFFFF )
			{
				Q_snprintf( query, sizeof( query ), "update "
					"job_worker_start set WorkerState=%d, NumWorkUnits=%d where JobWorkerID=%lu",
					VMPI_IsProcConnected( i ), 
					(int) VMPI_GetNumWorkUnitsCompleted( i ),
					VMPI_GetJobWorkerID( i )
					); 
				g_pSQL->Execute( query );
			}
		}
		return 1;
	}
};

class CSQLDBCommand_JobMasterEnd : public CSQLDBCommandBase
{
public:
	
	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format( "insert into job_master_end values ( %lu, %d, %d, \"no errors\" )", g_JobPrimaryID, g_nWorkersConnected, g_nWorkersDisconnected ); 
		query.Execute( g_pSQL  );

		// Now set RunningTimeMS.
		unsigned long runningTimeMS = GetTickCount() - g_StatsStartTime;
		query.Format( "update job_master_start set RunningTimeMS=%lu where JobID=%lu", runningTimeMS, g_JobPrimaryID );
		query.Execute( g_pSQL );
		return 1;
	}
};


void UpdateJobWorkerRunningTime()
{
	unsigned long runningTimeMS = GetTickCount() - g_StatsStartTime;
	
	char curStage[256];
	VMPI_GetCurrentStage( curStage, sizeof( curStage ) );
	
	CMySQLQuery query;
	query.Format( "update job_worker_start set RunningTimeMS=%lu, CurrentStage=\"%s\", "
		"Thread0WU=%d, Thread1WU=%d, Thread2WU=%d, Thread3WU=%d where JobWorkerID=%lu", 
		runningTimeMS, 
		curStage,
		(int) g_ThreadWUs[0],
		(int) g_ThreadWUs[1],
		(int) g_ThreadWUs[2],
		(int) g_ThreadWUs[3],
		g_JobWorkerID );
	query.Execute( g_pSQL );
}


class CSQLDBCommand_GraphEntry : public CSQLDBCommandBase
{
public:
	
				CSQLDBCommand_GraphEntry( DWORD msTime, DWORD nBytesSent, DWORD nBytesReceived )
				{
					m_msTime = msTime;
					m_nBytesSent = nBytesSent;
					m_nBytesReceived = nBytesReceived;
				}

	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format(	"insert into graph_entry (JobWorkerID, MSSinceJobStart, BytesSent, BytesReceived) "
						"values ( %lu, %lu, %lu, %lu )", 
			g_JobWorkerID, 
			m_msTime, 
			m_nBytesSent, 
			m_nBytesReceived );
		
		query.Execute( g_pSQL );

		UpdateJobWorkerRunningTime();

		++g_CurrentMessageIndex;
		return 1;
	}

	DWORD m_nBytesSent;
	DWORD m_nBytesReceived;
	DWORD m_msTime;
};



class CSQLDBCommand_TextMessage : public CSQLDBCommandBase
{
public:
	
				CSQLDBCommand_TextMessage( const char *pText )
				{
					m_pText = FormatStringForSQL( pText );
				}

	virtual		~CSQLDBCommand_TextMessage()
	{
		delete [] m_pText;
	}

	virtual int RunCommand()
	{
		CMySQLQuery query;
		query.Format( "insert into text_messages (JobWorkerID, MessageIndex, Text) values ( %lu, %lu, \"%s\" )", g_JobWorkerID, g_CurrentMessageIndex, m_pText );
		query.Execute( g_pSQL );

		++g_CurrentMessageIndex;
		return 1;
	}

	char		*m_pText;
};


// -------------------------------------------------------------------------------- //
// Internal helpers.
// -------------------------------------------------------------------------------- //

// This is the spew output before it has connected to the MySQL database.
CVMPICriticalSection g_SpewTextCS;
CUtlVector<char> g_SpewText( 1024 );


void VMPI_Stats_SpewHook( const char *pMsg )
{
	CVMPICriticalSectionLock csLock( &g_SpewTextCS );
	csLock.Lock();

		// Queue the text up so we can send it to the DB right away when we connect.
		g_SpewText.AddMultipleToTail( strlen( pMsg ), pMsg );
}


void PerfThread_SendSpewText()
{
	// Send the spew text to the database.
	CVMPICriticalSectionLock csLock( &g_SpewTextCS );
	csLock.Lock();
		
		if ( g_SpewText.Count() > 0 )
		{
			g_SpewText.AddToTail( 0 );
			
			if ( g_bMPI_StatsTextOutput )
			{
				g_pDB->AddCommandToQueue( new CSQLDBCommand_TextMessage( g_SpewText.Base() ), NULL );
			}
			else
			{
				// Just show one message in the vmpi_job_watch window to let them know that they need
				// to use a command line option to get the output.
				static bool bFirst = true;
				if ( bFirst )
				{
					char msg[512];
					V_snprintf( msg, sizeof( msg ), "%s not enabled", VMPI_GetParamString( mpi_Stats_TextOutput ) );
					bFirst = false;
					g_pDB->AddCommandToQueue( new CSQLDBCommand_TextMessage( msg ), NULL );
				}
			}
			
			g_SpewText.RemoveAll();
		}

	csLock.Unlock();
}


void PerfThread_AddGraphEntry( DWORD startTicks, DWORD &lastSent, DWORD &lastReceived )
{
	// Send the graph entry with data transmission info.
	DWORD curSent = g_nBytesSent + g_nMulticastBytesSent;
	DWORD curReceived = g_nBytesReceived + g_nMulticastBytesReceived;

	g_pDB->AddCommandToQueue( 
		new CSQLDBCommand_GraphEntry( 
			GetTickCount() - startTicks,
			curSent - lastSent, 
			curReceived - lastReceived ), 
		NULL );

	lastSent = curSent;
	lastReceived = curReceived;
}


// This function adds a graph_entry into the database periodically.
DWORD WINAPI PerfThreadFn( LPVOID pParameter )
{
	DWORD lastSent = 0;
	DWORD lastReceived = 0;
	DWORD startTicks = GetTickCount();

	while ( WaitForSingleObject( g_hPerfThreadExitEvent, 1000 ) != WAIT_OBJECT_0 )
	{
		PerfThread_AddGraphEntry( startTicks, lastSent, lastReceived );

		// Send updates for text output.
		PerfThread_SendSpewText();

		// If we're the master, update all the worker stats.
		if ( g_bMaster )
		{
			g_pDB->AddCommandToQueue( 
				new CSQLDBCommand_WorkerStats, 
				NULL );
		}
	}

	// Add the remaining text and one last graph entry (which will include the current stage info).
	PerfThread_SendSpewText();
	PerfThread_AddGraphEntry( startTicks, lastSent, lastReceived );

	SetEvent( g_hPerfThreadExitEvent );
	return 0;
}


// -------------------------------------------------------------------------------- //
// VMPI_Stats interface.
// -------------------------------------------------------------------------------- //

void VMPI_Stats_InstallSpewHook()
{
	InstallExtraSpewHook( VMPI_Stats_SpewHook );
}


void UnloadMySQLWrapper()
{
	if ( g_hMySQLDLL )
	{
		if ( g_pSQL )
		{
			g_pSQL->Release();
			g_pSQL = NULL;
		}
	
		Sys_UnloadModule( g_hMySQLDLL );
		g_hMySQLDLL = NULL;
	}
}


bool LoadMySQLWrapper(
	const char *pHostName, 
	const char *pDBName, 
	const char *pUserName
	)
{
	UnloadMySQLWrapper();

	// Load the DLL and the interface.
	if ( !Sys_LoadInterface( "mysql_wrapper", MYSQL_WRAPPER_VERSION_NAME, &g_hMySQLDLL, (void**)&g_pSQL ) )
		return false;

	// Try to init the database.
	if ( !g_pSQL->InitMySQL( pDBName, pHostName, pUserName ) )
	{
		UnloadMySQLWrapper();
		return false;
	}

	return true;
}


bool VMPI_Stats_Init_Master( 
	const char *pHostName, 
	const char *pDBName, 
	const char *pUserName,
	const char *pBSPFilename, 
	unsigned long *pDBJobID )
{
	Assert( !g_pDB );

	g_bMaster = true;
	
	// Connect the database.
	g_pDB = new CMySqlDatabase;
	if ( !g_pDB || !g_pDB->Initialize() || !LoadMySQLWrapper( pHostName, pDBName, pUserName ) )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}

	DWORD size = sizeof( g_MachineName );
	GetComputerName( g_MachineName, &size );

	// Create the job_master_start row.
	Q_FileBase( pBSPFilename, g_BSPFilename, sizeof( g_BSPFilename ) );

	g_JobPrimaryID = 0;
	CMySQLQuery query;
	query.Format( "insert into job_master_start ( BSPFilename, StartTime, MachineName, RunningTimeMS ) values ( \"%s\", null, \"%s\", %lu )", g_BSPFilename, g_MachineName, RUNNINGTIME_MS_SENTINEL ); 
	query.Execute( g_pSQL );

	g_JobPrimaryID = g_pSQL->InsertID();
	if ( g_JobPrimaryID == 0 )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}


	// Now init the worker portion.
	*pDBJobID = g_JobPrimaryID;
	return VMPI_Stats_Init_Worker( NULL, NULL, NULL, g_JobPrimaryID );
}



bool VMPI_Stats_Init_Worker( const char *pHostName, const char *pDBName, const char *pUserName, unsigned long DBJobID )
{
	g_StatsStartTime = GetTickCount();
	
	// If pDBServerName is null, then we're the master and we just want to make the job_worker_start entry.
	if ( pHostName )
	{
		Assert( !g_pDB );
		
		// Connect the database.
		g_pDB = new CMySqlDatabase;
		if ( !g_pDB || !g_pDB->Initialize() || !LoadMySQLWrapper( pHostName, pDBName, pUserName ) )
		{
			delete g_pDB;
			g_pDB = NULL;
			return false;
		}
		
		// Get our machine name to store in the database.
		DWORD size = sizeof( g_MachineName );
		GetComputerName( g_MachineName, &size );
	}


	g_JobPrimaryID = DBJobID;
	g_JobWorkerID = 0;

	CMySQLQuery query;
	query.Format( "insert into job_worker_start ( JobID, CurrentStage, IsMaster, MachineName ) values ( %lu, \"none\", %d, \"%s\" )",
		g_JobPrimaryID, g_bMaster, g_MachineName );
	query.Execute( g_pSQL );
			
	g_JobWorkerID = g_pSQL->InsertID();
	if ( g_JobWorkerID == 0 )
	{
		delete g_pDB;
		g_pDB = NULL;
		return false;
	}

	// Now create a thread that samples perf data and stores it in the database.
	g_hPerfThreadExitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_hPerfThread = CreateThread(
		NULL,
		0,
		PerfThreadFn,
		NULL,
		0,
		&g_PerfThreadID );

	return true;	
}


void VMPI_Stats_Term()
{
	if ( !g_pDB )
		return;

	// Stop the thread.
	SetEvent( g_hPerfThreadExitEvent );
	WaitForSingleObject( g_hPerfThread, INFINITE );
	
	CloseHandle( g_hPerfThreadExitEvent );
	g_hPerfThreadExitEvent = NULL;

	CloseHandle( g_hPerfThread );
	g_hPerfThread = NULL;

	if ( g_bMaster )
	{
		// (Write a job_master_end entry here).
		g_pDB->AddCommandToQueue( new CSQLDBCommand_JobMasterEnd, NULL );
	}

	// Wait for up to a second for the DB to finish writing its data.
	DWORD startTime = GetTickCount();
	while ( GetTickCount() - startTime < 1000 )
	{
		if ( g_pDB->QueriesInOutQueue() == 0 )
			break;
	}

	delete g_pDB;
	g_pDB = NULL;

	UnloadMySQLWrapper();
}


static bool ReadStringFromFile( FILE *fp, char *pStr, int strSize )
{
	int i=0;
	for ( i; i < strSize-2; i++ )
	{
		if ( fread( &pStr[i], 1, 1, fp ) != 1 ||
			pStr[i] == '\n' )
		{
			break;
		}
	}

	pStr[i] = 0;
	return i != 0;
}


// This looks for pDBInfoFilename in the same path as pBaseExeFilename.
// The file has 3 lines: machine name (with database), database name, username
void GetDBInfo( const char *pDBInfoFilename, CDBInfo *pInfo )
{
	char baseExeFilename[512];
	if ( !GetModuleFileName( GetModuleHandle( NULL ), baseExeFilename, sizeof( baseExeFilename ) ) )
		Error( "GetModuleFileName failed." );
	
	// Look for the info file in the same directory as the exe.
	char dbInfoFilename[512];
	Q_strncpy( dbInfoFilename, baseExeFilename, sizeof( dbInfoFilename ) );
	Q_StripFilename( dbInfoFilename );

	if ( dbInfoFilename[0] == 0 )
		Q_strncpy( dbInfoFilename, ".", sizeof( dbInfoFilename ) );

	Q_strncat( dbInfoFilename, "/", sizeof( dbInfoFilename ), COPY_ALL_CHARACTERS );
	Q_strncat( dbInfoFilename, pDBInfoFilename, sizeof( dbInfoFilename ), COPY_ALL_CHARACTERS );

	FILE *fp = fopen( dbInfoFilename, "rt" );
	if ( !fp )
	{
		Error( "Can't open %s for database info.\n", dbInfoFilename );
	}

	if ( !ReadStringFromFile( fp, pInfo->m_HostName, sizeof( pInfo->m_HostName ) ) ||
		 !ReadStringFromFile( fp, pInfo->m_DBName, sizeof( pInfo->m_DBName ) ) || 
		 !ReadStringFromFile( fp, pInfo->m_UserName, sizeof( pInfo->m_UserName ) ) 
		 )
	{
		Error( "%s is not a valid database info file.\n", dbInfoFilename );
	}

	fclose( fp );
}


void RunJobWatchApp( char *pCmdLine )
{
	STARTUPINFO si;
	memset( &si, 0, sizeof( si ) );
	si.cb = sizeof( si );

	PROCESS_INFORMATION pi;
	memset( &pi, 0, sizeof( pi ) );

	// Working directory should be the same as our exe's directory.
	char dirName[512];
	if ( GetModuleFileName( NULL, dirName, sizeof( dirName ) ) != 0 )
	{
		char *s1 = V_strrchr( dirName, '\\' );
		char *s2 = V_strrchr( dirName, '/' );
		if ( s1 || s2 )
		{
			// Get rid of the last slash.
			s1 = max( s1, s2 );
			s1[0] = 0;
		
			if ( !CreateProcess( 
				NULL, 
				pCmdLine, 
				NULL,							// security
				NULL,
				TRUE,
				0,			// flags
				NULL,							// environment
				dirName,							// current directory
				&si,
				&pi ) )
			{
				Warning( "%s - error launching '%s'\n", VMPI_GetParamString( mpi_Job_Watch ), pCmdLine );
			}
		}
	}
}


void StatsDB_InitStatsDatabase( 
	int argc, 
	char **argv, 
	const char *pDBInfoFilename )
{
	// Did they disable the stats database?
	if ( !g_bMPI_Stats && !VMPI_IsParamUsed( mpi_Job_Watch ) )
		return;

	unsigned long jobPrimaryID;

	// Now open the DB.
	if ( g_bMPIMaster )
	{
		CDBInfo dbInfo;
		GetDBInfo( pDBInfoFilename, &dbInfo );

		if ( !VMPI_Stats_Init_Master( dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, argv[argc-1], &jobPrimaryID ) )
		{
			Warning( "VMPI_Stats_Init_Master( %s, %s, %s ) failed.\n", dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName );

			// Tell the workers not to use stats.
			dbInfo.m_HostName[0] = 0; 
		}

		char cmdLine[2048];
		Q_snprintf( cmdLine, sizeof( cmdLine ), "vmpi_job_watch -JobID %d", jobPrimaryID );
		
		Msg( "\nTo watch this job, run this command line:\n%s\n\n", cmdLine );
		
		if ( VMPI_IsParamUsed( mpi_Job_Watch ) )
		{
			// Convenience thing to automatically launch the job watch for this job.
			RunJobWatchApp( cmdLine );
		}

		// Send the database info to all the workers.
		SendDBInfo( &dbInfo, jobPrimaryID );
	}
	else
	{
		// Wait to get DB info so we can connect to the MySQL database.
		CDBInfo dbInfo;
		unsigned long jobPrimaryID;
		RecvDBInfo( &dbInfo, &jobPrimaryID );
		
		if ( dbInfo.m_HostName[0] != 0 )
		{
			if ( !VMPI_Stats_Init_Worker( dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, jobPrimaryID ) )
				Error( "VMPI_Stats_Init_Worker( %s, %s, %s, %d ) failed.\n", dbInfo.m_HostName, dbInfo.m_DBName, dbInfo.m_UserName, jobPrimaryID );
		}
	}
}


unsigned long StatsDB_GetUniqueJobID()
{
	return g_JobPrimaryID;
}


unsigned long VMPI_Stats_GetJobWorkerID()
{
	return g_JobWorkerID;
}