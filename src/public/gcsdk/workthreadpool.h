//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A thread pool implementation.  You give it CWorkItems,
// it processes them asynchronously, and hands them back to you when they've 
// been completed.
// 
// To declare a queue, provide the implementation of a CWorkItem subtype, 
// the thread name prefix for threads in the pool, and the number of work 
// threads you want.
//
// CNet uses this class to offload encryption to a separate thread, 
// so that's a good place to start looking for usage examples.
//
//=============================================================================

#ifndef WORKTHREADPOOL_H
#define WORKTHREADPOOL_H
#ifdef _WIN32
#pragma once
#endif

#include <refcount.h>
#include <reliabletimer.h>
#include "jobtime.h"

// forward declaration for CTSQueue which we can't statically allocate as our member
// because of alignment issues on Win64
template <class T, bool bTestOptimizer>
class CTSQueue;

namespace GCSDK {

// forward declarations
class CWorkThread;
class CJobMgr;


// these functions return pointers to fixed string in the code section. We need this for VPROF nodes
#define DECLARE_WORK_ITEM( classname ) \
	virtual const char* GetDispatchCompletedName() const { return #classname"::DispatchCompleted"; } \
	virtual const char* GetThreadProcessName() const { return #classname"::ThreadProcess"; }


//-----------------------------------------------------------------------------
// Purpose: Work item base class.  Derive from this for specific work item types.
//			The derived type ideally should be self-contained with all data it
//			needs to perform the work.  
//-----------------------------------------------------------------------------
class CWorkItem : public CRefCount
{
public:
	CWorkItem()
	:	m_JobID( k_GIDNil ),
		m_bRunning( false ),
		m_bResubmit( false ),
		m_bCanceled( false ),
		m_ulSequenceNumber( 0 )
	{
		m_jobtimeTimeout.SetLTime( 0 );
		m_jobtimeQueued.SetToJobTime();
	}
	
	CWorkItem( JobID_t jobID )	
	:	m_JobID( jobID ),
		m_bRunning( false ),
		m_bResubmit( false ),
		m_bCanceled( false ),
		m_ulSequenceNumber( 0 )
	{
		m_jobtimeTimeout.SetLTime( 0 );
		m_jobtimeQueued.SetToJobTime();
	}

	CWorkItem( JobID_t jobID, int64 cTimeoutMicroseconds )
	:	m_JobID( jobID ),
		m_bRunning( false ),
		m_bResubmit( false ),
		m_bCanceled( false ),
		m_ulSequenceNumber( 0 )
	{
		SetPreExecuteTimeout( cTimeoutMicroseconds );
		m_jobtimeQueued.SetToJobTime();
	}

	void SetJobID( JobID_t jobID )	
	{ 
		Assert(jobID != k_GIDNil) ;
		m_JobID = jobID; 
	}
	JobID_t GetJobID() const		{ return m_JobID; }

	bool HasTimedOut() const							{ return m_jobtimeTimeout.LTime() != 0 && m_jobtimeTimeout.CServerMicroSecsPassed() > 0; }
	int64 WaitingTime() const							{ return m_jobtimeQueued.CServerMicroSecsPassed(); }
	void SetPreExecuteTimeout( int64 cMicroSeconds )	{ m_jobtimeTimeout.SetFromJobTime( cMicroSeconds ); }
	bool BPreExecuteTimeoutSet( ) const					{ return m_jobtimeTimeout.LTime() != 0; }
	void ForceTimeOut()									{ m_jobtimeTimeout.SetFromJobTime( -1 );}
	bool BIsRunning() const								{ return m_bRunning; } // true if running right now
	bool WasCancelled() const							{ return m_bCanceled; }
	void SetCycleCount( CCycleCount& cycleCount )		{ m_CycleCount = cycleCount ; }
	CCycleCount GetCycleCount()							{ return m_CycleCount; }
	uint64 GetSequenceNumber()							{ return m_ulSequenceNumber; }

	// Work threads can call this to force a work item to be reprocessed (added to the end of the process queue)
	void SetResubmit( bool bResubmit )			{ m_bResubmit = bResubmit; }

	// these functions return pointers to fixed string in the code section. 
	// We need this for VPROF nodes, you must use the DECLARE_WORK_ITEM macro
	virtual const char* GetDispatchCompletedName() const = 0; 
	virtual const char* GetThreadProcessName() const = 0;

	// Return false if your operation failed in some way that you would want to know about
	// The CWorkThreadPool will count the failures.
	virtual bool ThreadProcess( CWorkThread *pThread ) = 0; // called by the worker thread
	virtual bool DispatchCompletedWorkItem( CJobMgr *jobMgr ); // called by main loop after item completed

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName ) {}		// Validate our internal structures
#endif

protected:
	// note: destructor is private.  This is a ref-counted object, private destructor ensures callers can't accidentally delete
	// directly, or declare on stack
	virtual ~CWorkItem() { }

	friend class CWorkThread;
	friend class CWorkThreadPool;
	uint64		m_ulSequenceNumber; // Sequence number for the work item, used when enforcing output ordering as matching input order
	CCycleCount	m_CycleCount;		// A record of how long it took to execute this particular work item !

private:
	bool		m_bResubmit;		// true if the item should be resubmitted after last run
	volatile bool m_bRunning;		// true if the work item is running right now
	bool		m_bCanceled;		// true if the work was canceled due to timeout
	CJobTime	m_jobtimeTimeout;	// time at which this result is no longer valid, so it shouldn't start to be processed
	CJobTime	m_jobtimeQueued;
	JobID_t		m_JobID;
};

// forward decl
class CWorkThreadPool;

//-----------------------------------------------------------------------------
// Purpose: Generic work thread implementation, to be specialized if necessary
//-----------------------------------------------------------------------------
class CWorkThread : public CThread
{
public:
	CWorkThread( CWorkThreadPool *pThreadPool );
	CWorkThread( CWorkThreadPool *pThreadPool, const char *pszName );

	virtual ~CWorkThread()
	{
	}

	virtual int Run();

	virtual void Cancel()
	{
	}

protected:
	CWorkThreadPool *m_pThreadPool;	// parent pool
	volatile bool m_bExitThread;	// set by CWorkThreadPool::StopWorkerThreads and possibly by subclasses of CWorkThread
	volatile bool m_bFinished;		// set by CWorkThread::Run [note: must still check IsThreadRunning, and/or call Join]
	virtual void OnStart() { }
	virtual void OnExit() { }

#ifdef DBGFLAG_VALIDATE
public:
	virtual void Validate( CValidator &validator, const char *pchName )	
	{
		VALIDATE_SCOPE();
	};
#endif // DBGFLAG_VALIDATE

friend class CWorkThreadPool;
};


//-----------------------------------------------------------------------------
// callback class to create work threads
//-----------------------------------------------------------------------------
class IWorkThreadFactory
{
public:
	virtual CWorkThread *CreateWorkerThread( class CWorkThreadPool *pWorkThreadPool ) = 0;
};


//-----------------------------------------------------------------------------
// reusable trivial implementation of IWorkThreadFactory
//-----------------------------------------------------------------------------
template<class T>
class CWorkThreadFactory : public IWorkThreadFactory
{
public:
	virtual CWorkThread *CreateWorkerThread( class CWorkThreadPool *pWorkThreadPool )
	{
		return new T( pWorkThreadPool );
	}
};


//-----------------------------------------------------------------------------
// Purpose: interface class for object that the WorkThreadPool can signal when
//			there are completed work items to process
//-----------------------------------------------------------------------------
class IWorkThreadPoolSignal
{
public:
	virtual void Signal() = 0;
};


//-----------------------------------------------------------------------------
// Purpose: pool of work threads.
//-----------------------------------------------------------------------------
class CWorkThreadPool
{
	friend class CWorkThread;
public:

	static void SetWorkItemCompletedSignal( IWorkThreadPoolSignal *pObject )
	{
		sm_pWorkItemsCompletedSignal = pObject;
	}


	CWorkThreadPool( const char *pszThreadNamePfx );

	// eventually it might be nice to be able to resize these pools via console command
	// in that case, we'd want a constructor like this, and a PoolSize accessor/mutator pair
	// it makes this class much more complicated, however (growing the pool is easy, shrinking it
	// is less easy) so we'll punt for now.
	/* CWorkThreadPool( const char *pszName = "unnamed thread" ) : CWorkThreadPool( pszName, -1 ); */
	
	virtual ~CWorkThreadPool();

	// Setting this will ensure that items of the same priority complete and get dispatched in the same order
	// they are added to the threadpool.  This has a small additional locking overhead and can increase latency
	// as items that are actually completed out-of-order have to queue waiting on earlier items.
	void SetEnsureOutputOrdering( bool bEnsureOutputOrdering ) { m_bEnsureOutputOrdering = bEnsureOutputOrdering; }

	void AllowTimeouts( bool bMayHaveJobTimeouts ) { m_bMayHaveJobTimeouts = bMayHaveJobTimeouts; }

	int AddWorkThread( CWorkThread *pThread );
	void StartWorkThreads();										// gentlemen, start your engines
	void StopWorkThreads();											// stop work threads
	bool HasWorkItemsToProcess() const;

	// sets it to use dynamic worker thread construction
	// if pWorkThreadControl is NULL, just creates a standard CWorkThread object
	void SetWorkThreadAutoConstruct( int cMaxThreads, IWorkThreadFactory *pWorkThreadConstructor );

	bool AddWorkItem( CWorkItem *pWorkItem );						// add a work item to the queue to process
	CWorkItem *GetNextCompletedWorkItem( );							// get next completed work item and it's priority if needed
	const char *GetThreadNamePrefix() const { return m_szThreadNamePfx; }

	void SetNeverSetEventOnAdd( bool bNeverSet ); 
	bool BNeverSetEventOnAdd() { return m_bNeverSetOnAdd; }

	// get count of completed work items 
	// can't be inline because of m_TSQueueCompleted type
	int GetCompletedWorkItemCount() const;

	// get count of work items to process
	// can't be inline because of m_TSQueueToProcess type
	int GetWorkItemToProcessCount() const;

	uint64 GetLastUsedSequenceNumber( ) const
	{
		return m_ulLastUsedSequenceNumber;
	}

	uint64 GetLastCompletedSequenceNumber( ) const
	{
		return m_ulLastCompletedSequenceNumber;
	}

	uint64 GetLastDispatchedSequenceNumber( ) const
	{
		return m_ulLastDispatchedSequenceNumber;
	}

#if 0
	uint64 GetAveExecutionTime() const
	{
		return m_StatExecutionTime.GetUlAvg();
	}
	uint64 GetAveWaitTime() const
	{
		return m_StatWaitTime.GetUlAvg();
	}
	uint64 GetCurrentBacklogTime() const;
#endif

	int CountCompletedSuccess() const { return m_cSuccesses; }
	int CountRetries() const { return m_cRetries; }
	int CountCompletedFailed() const { return m_cFailures; }

	bool BDispatchCompletedWorkItems( const CLimitTimer &limitTimer, CJobMgr *pJobMgr );
	bool BExiting() const { return m_bExiting; }

	int GetWorkerCount() const { return m_WorkThreads.Count(); }

	uint GetActiveThreadCount() const { return m_cActiveThreads; }

	// make sure you lock before using this
	const CWorkThread *GetWorkThread( int iIndex ) const
	{
		Assert( iIndex >= 0 && iIndex < m_WorkThreads.Count() );
		return m_WorkThreads[iIndex];
	}

protected:

	// STATICS
	static IWorkThreadPoolSignal *sm_pWorkItemsCompletedSignal;

	// MEMBERS
	CWorkItem *GetNextWorkItemToProcess( );
	void StartWorkThread( CWorkThread *pWorkThread, int iName );

	// meaningful thread name prefix
	char m_szThreadNamePfx[32];
	// have we actually initialized the threadpool?
	bool m_bThreadsInitialized;

	// Incoming queue: queue of all work items to process
	// must be dynamically allocated for alignment requirements on Win64
	CTSQueue< CWorkItem *, false > *m_pTSQueueToProcess;

	// Outgoing queues: queue of all completed work items
	// must be dynamically allocated for alignment requirements on Win64
	CTSQueue< CWorkItem *, false > *m_pTSQueueCompleted;

	// Vectors of completed, but out of order and waiting work items, only used when bEnsureOutputOrdering == true
	CThreadMutex m_MutexOnItemCompletedOrdered;
	CUtlVector< CWorkItem * > m_vecCompletedAndWaiting;

	// Should we emit work items in the same order they are received (on a per priority basis)
	bool m_bEnsureOutputOrdering;

	// Sequence numbers
	uint64 m_ulLastUsedSequenceNumber;
	uint64 m_ulLastCompletedSequenceNumber;
	uint64 m_ulLastDispatchedSequenceNumber;

	bool m_bMayHaveJobTimeouts;
	CUtlVector< CWorkThread * > m_WorkThreads; 
	CThreadMutex m_WorkThreadMutex;
	CInterlockedUInt m_cThreadsRunning;	// how many threads are running 
	volatile bool m_bExiting;			// are we exiting
	CThreadEvent m_EventNewWorkItem;	// event set when a new work item is available to process
	CInterlockedInt m_cActiveThreads;
	volatile bool m_bNeverSetOnAdd;

	bool m_bAutoCreateThreads;
	int m_cMaxThreads;
	IWorkThreadFactory *m_pWorkThreadConstructor;

	// override this method if you want to do any special handling of completed work items.  Default implementation puts
	// work items in our completed item queue.
	virtual void OnWorkItemCompleted( CWorkItem *pWorkItem );

	bool BTryDeleteExitedWorkerThreads();

	int m_cSuccesses;
	int m_cFailures;
	int m_cRetries;
#if 0
	CStat m_StatExecutionTime;
	CStat m_StatWaitTime;
#endif
	CLimitTimer m_LimitTimerCreateNewThreads;

#ifdef DBGFLAG_VALIDATE
public:
	void Validate( CValidator &validator, const char *pchName );
#endif
};

} // namespace GCSDK


#endif // WORKTHREAD_H
