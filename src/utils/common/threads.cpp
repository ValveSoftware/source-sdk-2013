//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#define	USED

#include "cmdlib.h"
#define NO_THREAD_NAMES
#include "threads.h"
#include "pacifier.h"

#define	MAX_THREADS	16

#if !defined(WIN32) && !defined(POSIX)
#error "threads.cpp is not supported on this platform"
#endif

#ifdef POSIX
#include <sys/sysinfo.h>
#endif

class CRunThreadsData
{
public:
	int m_iThread;
	void *m_pUserData;
	RunThreadsFn m_Fn;
};

CRunThreadsData g_RunThreadsData[MAX_THREADS];


int		dispatch;
int		workcount;
qboolean		pacifier;

qboolean	threaded;
bool g_bLowPriorityThreads = false;

#ifdef WIN32
HANDLE g_ThreadHandles[MAX_THREADS];
#elif defined(POSIX)
pthread_t g_ThreadHandles[MAX_THREADS];
#endif



/*
=============
GetThreadWork

=============
*/
int	GetThreadWork (void)
{
	int	r;

	ThreadLock ();

	if (dispatch == workcount)
	{
		ThreadUnlock ();
		return -1;
	}

	UpdatePacifier( (float)dispatch / workcount );

	r = dispatch;
	dispatch++;
	ThreadUnlock ();

	return r;
}


ThreadWorkerFn workfunction;

void ThreadWorkerFunction( int iThread, void *pUserData )
{
	int		work;

	while (1)
	{
		work = GetThreadWork ();
		if (work == -1)
			break;
		workfunction( iThread, work );
	}
}

void RunThreadsOnIndividual (int workcnt, qboolean showpacifier, ThreadWorkerFn func)
{
	if (numthreads == -1)
		ThreadSetDefault ();
	workfunction = func;
	RunThreadsOn (workcnt, showpacifier, ThreadWorkerFunction);
}


/*
===================================================================

WIN32

===================================================================
*/

int		numthreads = -1;
CThreadMutex mutex;
static int enter;

void SetLowPriority()
{
#ifdef WIN32
	SetPriorityClass( GetCurrentProcess(), IDLE_PRIORITY_CLASS );
#elif defined(POSIX)
	struct sched_param p {};
	p.sched_priority = 0;
	sched_setscheduler ( 0, SCHED_IDLE, &p );
#endif
}


void ThreadSetDefault (void)
{

	if (numthreads == -1)	// not set manually
	{
#ifdef WIN32
		SYSTEM_INFO info;
		GetSystemInfo (&info);
		numthreads = info.dwNumberOfProcessors;
#elif defined(POSIX)
		numthreads = get_nprocs();
#endif
		if (numthreads < 1 || numthreads > 32)
			numthreads = 1;
	}

	Msg ("%i threads\n", numthreads);
}


void ThreadLock (void)
{
	if (!threaded)
		return;
	mutex.Lock();
	if (enter)
		Error ("Recursive ThreadLock\n");
	enter = 1;
}

void ThreadUnlock (void)
{
	if (!threaded)
		return;
	if (!enter)
		Error ("ThreadUnlock without lock\n");
	enter = 0;
	mutex.Unlock();
}

// This runs in the thread and dispatches a RunThreadsFn call.
#ifdef WIN32
DWORD WINAPI InternalRunThreadsFn( LPVOID pParameter )
#elif defined(POSIX)
void *InternalRunThreadsFn( void *pParameter )
#endif
{
	CRunThreadsData *pData = (CRunThreadsData*)pParameter;
	pData->m_Fn( pData->m_iThread, pData->m_pUserData );
	return 0;
}


void RunThreads_Start( RunThreadsFn fn, void *pUserData, ERunThreadsPriority ePriority )
{
	Assert( numthreads > 0 );
	threaded = true;

	if ( numthreads > MAX_TOOL_THREADS )
		numthreads = MAX_TOOL_THREADS;

	for ( int i=0; i < numthreads ;i++ )
	{
		g_RunThreadsData[i].m_iThread = i;
		g_RunThreadsData[i].m_pUserData = pUserData;
		g_RunThreadsData[i].m_Fn = fn;
#ifdef WIN32
		DWORD dwDummy;
		g_ThreadHandles[i] = CreateThread(
		   NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		   0,		// DWORD cbStack,
		   InternalRunThreadsFn,	// LPTHREAD_START_ROUTINE lpStartAddr,
		   &g_RunThreadsData[i],	// LPVOID lpvThreadParm,
		   0,			// DWORD fdwCreate,
		   &dwDummy );

		if ( ePriority == k_eRunThreadsPriority_UseGlobalState )
		{
			if( g_bLowPriorityThreads )
				SetThreadPriority( g_ThreadHandles[i], THREAD_PRIORITY_LOWEST );
		}
		else if ( ePriority == k_eRunThreadsPriority_Idle )
		{
			SetThreadPriority( g_ThreadHandles[i], THREAD_PRIORITY_IDLE );
		}
#elif defined(POSIX)
		pthread_create( &g_ThreadHandles[i], nullptr, InternalRunThreadsFn, &g_RunThreadsData[i] );
#endif
	}
}


void RunThreads_End()
{
#ifdef WIN32
	WaitForMultipleObjects( numthreads, g_ThreadHandles, TRUE, INFINITE );
	for ( int i=0; i < numthreads; i++ )
		CloseHandle( g_ThreadHandles[i] );
#elif defined(POSIX)
	for ( int i=0; i < numthreads; i++ )
		pthread_join(g_ThreadHandles[i], nullptr);
#endif
	threaded = false;
}

/*
=============
RunThreadsOn
=============
*/
void RunThreadsOn( int workcnt, qboolean showpacifier, RunThreadsFn fn, void *pUserData )
{
	int		start, end;

	start = Plat_FloatTime();
	dispatch = 0;
	workcount = workcnt;
	StartPacifier("");
	pacifier = showpacifier;

#ifdef _PROFILE
	threaded = false;
	(*func)( 0 );
	return;
#endif

	
	RunThreads_Start( fn, pUserData );
	RunThreads_End();


	end = Plat_FloatTime();
	if (pacifier)
	{
		EndPacifier(false);
		printf (" (%i)\n", end-start);
	}
}


