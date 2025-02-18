//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include "imysqlwrapper.h"
#include "mysql_async.h"
#include "utllinkedlist.h"


static char* CopyString( const char *pStr )
{
	char *pRet = new char[ strlen( pStr ) + 1 ];
	strcpy( pRet, pStr );
	return pRet;
}


class CMySQLAsync : public IMySQLAsync
{
public:
	
	CMySQLAsync()
	{
		m_hThread = NULL;
		m_pSQL = NULL;

		m_hExitEvent = CreateEvent( NULL, true, false, NULL );	// Use manual reset because we want it to cascade out without
																// resetting the event if it gets set.
		m_hPendingQueryEvent = CreateEvent( NULL, false, false, NULL );
		m_hQueryResultsEvent = CreateEvent( NULL, false, false, NULL );
		
		InitializeCriticalSection( &m_ExecuteQueryCS );
		InitializeCriticalSection( &m_PendingQueryCS );
	}

	~CMySQLAsync()
	{
		Term();
		
		CloseHandle( m_hExitEvent );
		CloseHandle( m_hPendingQueryEvent );
		CloseHandle( m_hQueryResultsEvent );

		DeleteCriticalSection( &m_ExecuteQueryCS );
		DeleteCriticalSection( &m_PendingQueryCS );
	}

	virtual void Release()
	{
		delete this;
	}

	virtual IMySQLRowSet* ExecuteBlocking( const char *pStr )
	{
		IMySQLRowSet *pRet;
		
		EnterCriticalSection( &m_ExecuteQueryCS );
			m_pSQL->Execute( pStr );
			pRet = m_pSQL->DuplicateRowSet();
		LeaveCriticalSection( &m_ExecuteQueryCS );

		return pRet;
	}

	virtual void Execute( const char *pStr, void *pUserData )
	{
		EnterCriticalSection( &m_PendingQueryCS );

			CPendingQuery query;
			query.m_pStr = CopyString( pStr );
			query.m_pUserData = pUserData;
			query.m_Timer.Start();

			m_PendingQueries.AddToTail( query );
			SetEvent( m_hPendingQueryEvent );

		LeaveCriticalSection( &m_PendingQueryCS );
	}
	
	virtual bool GetNextResults( CQueryResults &results )
	{
		results.m_pResults = NULL;

		if ( WaitForSingleObject( m_hQueryResultsEvent, 0 ) == WAIT_OBJECT_0 )
		{
			EnterCriticalSection( &m_PendingQueryCS );
			
				Assert( m_QueryResults.Count() > 0 );
				int iHead = m_QueryResults.Head();
				results = m_QueryResults[iHead];
				m_QueryResults.Remove( iHead );

				if ( m_QueryResults.Count() > 0 )
					SetEvent( m_hQueryResultsEvent );
			
			LeaveCriticalSection( &m_PendingQueryCS );
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Init( IMySQL *pSQL )
	{
		Term();

		DWORD dwThreadID;
		m_hThread = CreateThread( NULL, 0, &CMySQLAsync::StaticThreadFn, this, 0, &dwThreadID );
		if ( m_hThread )
		{										
			m_pSQL = pSQL;
			return true;
		}
		else
		{
			return false;
		}
	}

	void Term()
	{
		// Stop the thread.
		if ( m_hThread )
		{
			// Delete all our queries.
			SetEvent( m_hExitEvent );
			WaitForSingleObject( m_hThread, INFINITE );
			CloseHandle( m_hThread );
			m_hThread = NULL;
		}

		// Delete leftover queries.
		FOR_EACH_LL( m_PendingQueries, iPendingQuery )
		{
			delete [] m_PendingQueries[iPendingQuery].m_pStr;
		}
		m_PendingQueries.Purge();
		
		FOR_EACH_LL( m_QueryResults, i )
		{
			m_QueryResults[i].m_pResults->Release();
		}
		m_QueryResults.Purge();

		if ( m_pSQL )
		{
			m_pSQL->Release();
			m_pSQL = NULL;
		}
	}


private:

	DWORD ThreadFn()
	{
		HANDLE hEvents[2] = { m_hExitEvent, m_hPendingQueryEvent };

		// 
		while ( 1 )
		{
			int ret = WaitForMultipleObjects( ARRAYSIZE( hEvents ), hEvents, false, INFINITE );
			if ( ret == WAIT_OBJECT_0 )
				break;

			if ( ret == WAIT_OBJECT_0+1 )
			{
				// A new string has been queued up for us to execute.
				EnterCriticalSection( &m_PendingQueryCS );
					
					Assert( m_PendingQueries.Count() > 0 );
					int iHead = m_PendingQueries.Head();
					
					CPendingQuery pending = m_PendingQueries[iHead];
					m_PendingQueries.Remove( iHead );

					// Set the pending query event if there are more queries waiting to run.
					if ( m_PendingQueries.Count() > 0 )
						SetEvent( m_hPendingQueryEvent );
					
				LeaveCriticalSection( &m_PendingQueryCS );

							
				// Run the query.
				EnterCriticalSection( &m_ExecuteQueryCS );

					CQueryResults results;
					results.m_pResults = NULL;
					results.m_pUserData = pending.m_pUserData;
					results.m_ExecuteTime.Init();
					pending.m_Timer.End();
					results.m_QueueTime = pending.m_Timer.GetDuration();
					
					CFastTimer executeTimer;
					executeTimer.Start();

					if ( m_pSQL->Execute( pending.m_pStr ) == 0 )
					{			
						executeTimer.End();
						results.m_ExecuteTime = executeTimer.GetDuration();
						results.m_pResults = m_pSQL->DuplicateRowSet();
					}

					delete pending.m_pStr;

				LeaveCriticalSection( &m_ExecuteQueryCS );


				// Store the results.
				EnterCriticalSection( &m_PendingQueryCS );

					m_QueryResults.AddToTail( results );
					SetEvent( m_hQueryResultsEvent );

				LeaveCriticalSection( &m_PendingQueryCS );
			}
		}
		
		return 0;
	}

	static DWORD WINAPI StaticThreadFn( LPVOID lpParameter )
	{
		return ((CMySQLAsync*)lpParameter)->ThreadFn();
	}

private:
	
	HANDLE m_hThread;
	HANDLE m_hExitEvent;
	HANDLE m_hPendingQueryEvent;	// Signaled when a new query is added.
	HANDLE m_hQueryResultsEvent;

	IMySQL *m_pSQL;

	CRITICAL_SECTION m_PendingQueryCS;
	CRITICAL_SECTION m_ExecuteQueryCS;

	
	// Outgoing query results. New ones are added to the tail.
	CUtlLinkedList<CQueryResults, int> m_QueryResults;

	
	// New ones added to the tail.
	class CPendingQuery
	{
	public:
		char *m_pStr;
		void *m_pUserData;
		CFastTimer m_Timer;	// Times how long this query is in the queue.
	};

	CUtlLinkedList<CPendingQuery,int> m_PendingQueries;
};


IMySQLAsync* CreateMySQLAsync( IMySQL *pSQL )
{
	CMySQLAsync *pRet = new CMySQLAsync;
	if ( pRet->Init( pSQL ) )
	{
		return pRet;
	}
	else
	{
		delete pRet;
		return NULL;
	}
}

