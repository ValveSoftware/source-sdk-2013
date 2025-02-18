//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "stdafx.h"
#include "net_view_thread.h"


char* CopyAlloc( const char *pStr )
{
	char *pRet = new char[ strlen( pStr ) + 1];
	strcpy( pRet, pStr );
	return pRet;
}


CNetViewThread::CNetViewThread()
{
	m_hThread = NULL;
	m_hThreadExitEvent = NULL;
	InitializeCriticalSection( &m_ComputerNamesCS );
}


CNetViewThread::~CNetViewThread()
{
	Term();
	DeleteCriticalSection( &m_ComputerNamesCS );
}


void CNetViewThread::Init()
{
	Term();

	m_hThreadExitEvent = CreateEvent( NULL, false, false, NULL );

	DWORD dwThreadID = 0;
	m_hThread = CreateThread( 
		NULL,
		0,
		&CNetViewThread::StaticThreadFn,
		this,
		0,
		&dwThreadID );
}


void CNetViewThread::Term()
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

	m_ComputerNames.PurgeAndDeleteElements();
}


void CNetViewThread::GetComputerNames( CUtlVector<char*> &computerNames, bool bRemoveFromList )
{
	EnterCriticalSection( &m_ComputerNamesCS );

	computerNames.Purge();
	for ( int i=0; i < m_ComputerNames.Count(); i++ )
	{
		computerNames.AddToTail( CopyAlloc( m_ComputerNames[i] ) );
	}

	if ( bRemoveFromList )
	{
		m_ComputerNames.PurgeAndDeleteElements();
	}

	LeaveCriticalSection( &m_ComputerNamesCS );
}


void CNetViewThread::UpdateServicesFromNetView()
{
    HANDLE hChildStdoutRd, hChildStdoutWr;
    
	// Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES saAttr; 
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

	if( CreatePipe( &hChildStdoutRd, &hChildStdoutWr, &saAttr, 0 ) )
	{
		STARTUPINFO si;
		memset(&si, 0, sizeof si);
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = hChildStdoutWr;
		
		PROCESS_INFORMATION pi;

		if( CreateProcess( 
			NULL, 
			"net view", 
			NULL,	// lpProcessAttributes
			NULL,	// lpThreadAttributes
			TRUE,	// bInheritHandls
			DETACHED_PROCESS,	// dwCreationFlags
			NULL,				// lpEnvironment
			NULL,				// lpCurrentDirectory
			&si,				// lpStartupInfo
			&pi					// lpProcessInformation
			) )
		{
			// read from pipe..
			#define BUFFER_SIZE 8192
			char buffer[BUFFER_SIZE];
			BOOL bDone = FALSE;
			CUtlVector<char> totalBuffer;
			
			while(1)
			{
				DWORD dwCount = 0;
				DWORD dwRead = 0;
				
				// read from input handle
				PeekNamedPipe(hChildStdoutRd, NULL, NULL, NULL, &dwCount, NULL);
				if (dwCount)
				{
					dwCount = min (dwCount, (DWORD)BUFFER_SIZE - 1);
					ReadFile(hChildStdoutRd, buffer, dwCount, &dwRead, NULL);
				}
				if(dwRead)
				{
					buffer[dwRead] = 0;
					totalBuffer.AddMultipleToTail( dwRead, buffer );
				}
				// check process termination
				else if( WaitForSingleObject( pi.hProcess, 1000 ) != WAIT_TIMEOUT )
				{
					if ( bDone )
						break;
					
					bDone = TRUE;	// next time we get it
				}
			}

			// Now parse the output.
			totalBuffer.AddToTail( 0 );
			ParseComputerNames( totalBuffer.Base() );
		}

		CloseHandle( hChildStdoutRd );
		CloseHandle( hChildStdoutWr );
	}
}


void CNetViewThread::ParseComputerNames( const char *pNetViewOutput )
{
	EnterCriticalSection( &m_ComputerNamesCS );

	m_ComputerNames.PurgeAndDeleteElements();
	
	const char *pCur = pNetViewOutput;
	while ( *pCur != 0 )
	{
		// If we get a \\, then it's a computer name followed by whitespace.
		if ( pCur[0] == '\\' && pCur[1] == '\\' )
		{
			char curComputerName[512];
			char *pOutPos = curComputerName;

			pCur += 2;
			while ( *pCur && !V_isspace( *pCur ) && (pOutPos-curComputerName < 510) )
			{
				*pOutPos++ = *pCur++;
			}
			*pOutPos = 0;

			m_ComputerNames.AddToTail( CopyAlloc( curComputerName ) );
		}
		++pCur;
	}

	LeaveCriticalSection( &m_ComputerNamesCS );
}


DWORD CNetViewThread::ThreadFn()
{
	// Update the services list every 30 seconds.
	do
	{
		UpdateServicesFromNetView();
	} while ( WaitForSingleObject( m_hThreadExitEvent, 30000 ) != WAIT_OBJECT_0 );

	return 0;
}


DWORD CNetViewThread::StaticThreadFn( LPVOID lpParameter )
{
	return ((CNetViewThread*)lpParameter)->ThreadFn();
}


