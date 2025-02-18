//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	A utility for a discrete job-oriented worker thread.
//
//			The class CThreadPool is both the job queue, and the 
//			worker thread. Except when the main thread attempts to 
//			synchronously execute a job, most of the inter-thread locking 
//			on the queue. 
//
//			The queue threading model uses a manual reset event for optimal 
//			throughput. Adding to the queue is guarded by a semaphore that 
//			will block the inserting thread if the queue has overflown. 
//			This prevents the worker thread from being starved out even if 
//			not running at a higher priority than the master thread.
//
//			The thread function waits for jobs, services jobs, and manages 
//			communication between the worker and master threads. The nature 
//			of the work is opaque to the Executer.
//
//			CJob instances actually do the work. The base class 
//			calls virtual methods for job primitives, so derivations don't 
//			need to worry about threading models. All of the variants of 
//			job and OS can be expressed in this hierarchy. Instances of 
//			CJob are the items placed in the queue, and by 
//			overriding the job primitives they are the manner by which 
//			users of the Executer control the state of the job. 
//
//=============================================================================

#include <limits.h>
#include "tier0/threadtools.h"
#include "tier1/refcount.h"
#include "tier1/utllinkedlist.h"
#include "tier1/utlvector.h"
#include "tier1/functors.h"
#include "tier0/vprof_telemetry.h"

#include "vstdlib/vstdlib.h"

#ifndef JOBTHREAD_H
#define JOBTHREAD_H

#ifdef AddJob  // windows.h print function collisions
#undef AddJob
#undef GetJob
#endif

#ifdef VSTDLIB_DLL_EXPORT
#define JOB_INTERFACE	DLL_EXPORT
#define JOB_OVERLOAD	DLL_GLOBAL_EXPORT
#define JOB_CLASS		DLL_CLASS_EXPORT
#else
#define JOB_INTERFACE	DLL_IMPORT
#define JOB_OVERLOAD	DLL_GLOBAL_IMPORT
#define JOB_CLASS		DLL_CLASS_IMPORT
#endif

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

class CJob;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum JobStatusEnum_t
{
	// Use negative for errors
	JOB_OK,						// operation is successful
	JOB_STATUS_PENDING,			// file is properly queued, waiting for service
	JOB_STATUS_INPROGRESS,		// file is being accessed
	JOB_STATUS_ABORTED,			// file was aborted by caller
	JOB_STATUS_UNSERVICED,		// file is not yet queued
};

typedef int JobStatus_t;

enum JobFlags_t
{
	JF_IO				= ( 1 << 0 ),	// The job primarily blocks on IO or hardware
	JF_BOOST_THREAD		= ( 1 << 1 ),	// Up the thread priority to max allowed while processing task
	JF_SERIAL			= ( 1 << 2 ),	// Job cannot be executed out of order relative to other "strict" jobs
	JF_QUEUE			= ( 1 << 3 ),	// Queue it, even if not an IO job
};

enum JobPriority_t
{
	JP_LOW,
	JP_NORMAL,
	JP_HIGH
};

#define TP_MAX_POOL_THREADS	64
struct ThreadPoolStartParams_t
{
	ThreadPoolStartParams_t( bool bIOThreads = false, unsigned nThreads = -1, int *pAffinities = NULL, ThreeState_t fDistribute = TRS_NONE, unsigned nStackSize = -1, int iThreadPriority = SHRT_MIN )
		: bIOThreads( bIOThreads ), nThreads( nThreads ), fDistribute( fDistribute ), nStackSize( nStackSize ), iThreadPriority( iThreadPriority ), nThreadsMax( -1 )
	{
		bExecOnThreadPoolThreadsOnly = false;

		bUseAffinityTable = ( pAffinities != NULL ) && ( fDistribute == TRS_TRUE ) && ( nThreads != (unsigned)-1 );
		if ( bUseAffinityTable )
		{
			// user supplied an optional 1:1 affinity mapping to override normal distribute behavior
			nThreads = MIN( TP_MAX_POOL_THREADS, nThreads );
			for ( unsigned int i = 0; i < nThreads; i++ )
			{
				iAffinityTable[i] = pAffinities[i];
			}
		}
	}

	int				nThreads;
	int				nThreadsMax;
	ThreeState_t	fDistribute;
	int				nStackSize;
	int				iThreadPriority;
	int				iAffinityTable[TP_MAX_POOL_THREADS];

	bool			bIOThreads : 1;
	bool			bUseAffinityTable : 1;
	bool			bExecOnThreadPoolThreadsOnly : 1;
};

//-----------------------------------------------------------------------------
//
// IThreadPool
//
//-----------------------------------------------------------------------------

typedef bool (*JobFilter_t)( CJob * );

//---------------------------------------------------------
// Messages supported through the CallWorker() method
//---------------------------------------------------------
enum ThreadPoolMessages_t
{
	TPM_EXIT,		// Exit the thread
	TPM_SUSPEND,		// Suspend after next operation
	TPM_RUNFUNCTOR,	// Run functor, reply when done.
};

//---------------------------------------------------------

abstract_class IThreadPool : public IRefCounted
{
public:
	virtual ~IThreadPool() {};

	//-----------------------------------------------------
	// Thread functions
	//-----------------------------------------------------
	virtual bool Start( const ThreadPoolStartParams_t &startParams = ThreadPoolStartParams_t() ) = 0;
	virtual bool Stop( int timeout = TT_INFINITE ) = 0;

	//-----------------------------------------------------
	// Functions for any thread
	//-----------------------------------------------------
	virtual unsigned GetJobCount() = 0;
	virtual int NumThreads() = 0;
	virtual int NumIdleThreads() = 0;

	//-----------------------------------------------------
	// Pause/resume processing jobs
	//-----------------------------------------------------
	virtual int SuspendExecution() = 0;
	virtual int ResumeExecution() = 0;

	//-----------------------------------------------------
	// Offer the current thread to the pool
	//-----------------------------------------------------
	virtual int YieldWait( CThreadEvent **pEvents, int nEvents, bool bWaitAll = true, unsigned timeout = TT_INFINITE ) = 0;
	virtual int YieldWait( CJob **, int nJobs, bool bWaitAll = true, unsigned timeout = TT_INFINITE ) = 0;
	virtual void Yield( unsigned timeout ) = 0;

	bool YieldWait( CThreadEvent &event, unsigned timeout = TT_INFINITE );
	bool YieldWait( CJob *, unsigned timeout = TT_INFINITE );

	//-----------------------------------------------------
	// Add a native job to the queue (master thread)
	//-----------------------------------------------------
	virtual void AddJob( CJob * ) = 0;

	//-----------------------------------------------------
	// All threads execute pFunctor asap. Thread will either wake up
	//  and execute or execute pFunctor right after completing current job and
	//  before looking for another job.
	//-----------------------------------------------------
	virtual void ExecuteHighPriorityFunctor( CFunctor *pFunctor ) = 0;

	//-----------------------------------------------------
	// Add an function object to the queue (master thread)
	//-----------------------------------------------------
	virtual void AddFunctor( CFunctor *pFunctor, CJob **ppJob = NULL, const char *pszDescription = NULL, unsigned flags = 0 ) { AddFunctorInternal( RetAddRef( pFunctor ), ppJob, pszDescription, flags ); }

	//-----------------------------------------------------
	// Change the priority of an active job
	//-----------------------------------------------------
	virtual void ChangePriority( CJob *p, JobPriority_t priority ) = 0;

	//-----------------------------------------------------
	// Bulk job manipulation (blocking)
	//-----------------------------------------------------
	int ExecuteAll( JobFilter_t pfnFilter = NULL )	{ return ExecuteToPriority( JP_LOW, pfnFilter ); }
	virtual int ExecuteToPriority( JobPriority_t toPriority, JobFilter_t pfnFilter = NULL  ) = 0;
	virtual int AbortAll() = 0;

	//-----------------------------------------------------
	virtual void Reserved1() = 0;

	//-----------------------------------------------------
	// Add an arbitrary call to the queue (master thread) 
	//
	// Avert thy eyes! Imagine rather:
	//
	// CJob *AddCall( <function>, [args1, [arg2,]...]
	// CJob *AddCall( <object>, <function>, [args1, [arg2,]...]
	// CJob *AddRefCall( <object>, <function>, [args1, [arg2,]...]
	// CJob *QueueCall( <function>, [args1, [arg2,]...]
	// CJob *QueueCall( <object>, <function>, [args1, [arg2,]...]
	//-----------------------------------------------------

	#define DEFINE_NONMEMBER_ADD_CALL(N) \
		template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_CONST_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_REF_COUNTING_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *AddRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			if ( !NumIdleThreads() ) \
			{ \
				pJob = GetDummyJob(); \
				FunctorDirectCall( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ); \
			} \
			else \
			{ \
				AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob ); \
			} \
			\
			return pJob; \
		}

	//-----------------------------------------------------------------------------

	#define DEFINE_NONMEMBER_QUEUE_CALL(N) \
		template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_CONST_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			return pJob; \
		}

	//-------------------------------------

	#define DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL(N) \
		template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
		CJob *QueueRefCall(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
			CJob *pJob; \
			AddFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ), &pJob, NULL, JF_QUEUE ); \
			\
			return pJob; \
		}

	FUNC_GENERATE_ALL( DEFINE_NONMEMBER_ADD_CALL );
	FUNC_GENERATE_ALL( DEFINE_MEMBER_ADD_CALL );
	FUNC_GENERATE_ALL( DEFINE_CONST_MEMBER_ADD_CALL );
	FUNC_GENERATE_ALL( DEFINE_REF_COUNTING_MEMBER_ADD_CALL );
	FUNC_GENERATE_ALL( DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL );
	FUNC_GENERATE_ALL( DEFINE_NONMEMBER_QUEUE_CALL );
	FUNC_GENERATE_ALL( DEFINE_MEMBER_QUEUE_CALL );
	FUNC_GENERATE_ALL( DEFINE_CONST_MEMBER_QUEUE_CALL );
	FUNC_GENERATE_ALL( DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL );
	FUNC_GENERATE_ALL( DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL );

	#undef DEFINE_NONMEMBER_ADD_CALL
	#undef DEFINE_MEMBER_ADD_CALL
	#undef DEFINE_CONST_MEMBER_ADD_CALL
	#undef DEFINE_REF_COUNTING_MEMBER_ADD_CALL
	#undef DEFINE_REF_COUNTING_CONST_MEMBER_ADD_CALL
	#undef DEFINE_NONMEMBER_QUEUE_CALL
	#undef DEFINE_MEMBER_QUEUE_CALL
	#undef DEFINE_CONST_MEMBER_QUEUE_CALL
	#undef DEFINE_REF_COUNTING_MEMBER_QUEUE_CALL
	#undef DEFINE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL

private:
	virtual void AddFunctorInternal( CFunctor *, CJob ** = NULL, const char *pszDescription = NULL, unsigned flags = 0 ) = 0;

	//-----------------------------------------------------
	// Services for internal use by job instances
	//-----------------------------------------------------
	friend class CJob;

	virtual CJob *GetDummyJob() = 0;

public:
	virtual void Distribute( bool bDistribute = true, int *pAffinityTable = NULL ) = 0;

	virtual bool Start( const ThreadPoolStartParams_t &startParams, const char *pszNameOverride ) = 0;
};

//-----------------------------------------------------------------------------

JOB_INTERFACE IThreadPool *CreateThreadPool();
JOB_INTERFACE void DestroyThreadPool( IThreadPool *pPool );

//-------------------------------------

JOB_INTERFACE void RunThreadPoolTests();

//-----------------------------------------------------------------------------

JOB_INTERFACE IThreadPool *g_pThreadPool;

//-----------------------------------------------------------------------------
// Class to combine the metadata for an operation and the ability to perform
// the operation. Meant for inheritance. All functions inline, defers to executor
//-----------------------------------------------------------------------------
DECLARE_POINTER_HANDLE( ThreadPoolData_t );
#define JOB_NO_DATA ((ThreadPoolData_t)-1)

class CJob : public CRefCounted1<IRefCounted, CRefCountServiceMT>
{
public:
	CJob( JobPriority_t priority = JP_NORMAL )
	  : m_status( JOB_STATUS_UNSERVICED ),
		m_ThreadPoolData( JOB_NO_DATA ),
		m_priority( priority ),
		m_flags( 0 ),
		m_pThreadPool( NULL ),
		m_CompleteEvent( true ),
		m_iServicingThread( -1 )
	{
		m_szDescription[ 0 ] = 0;
	}

	//-----------------------------------------------------
	// Priority (not thread safe)
	//-----------------------------------------------------
	void SetPriority( JobPriority_t priority )		{ m_priority = priority; }
	JobPriority_t GetPriority() const				{ return m_priority; }

	//-----------------------------------------------------

	void SetFlags( unsigned flags )					{ m_flags = flags; }
	unsigned GetFlags() const						{ return m_flags; }

	//-----------------------------------------------------

	void SetServiceThread( int iServicingThread )	{ m_iServicingThread = (char)iServicingThread; }
	int GetServiceThread() const					{ return m_iServicingThread; }
	void ClearServiceThread()						{ m_iServicingThread = -1; }

	//-----------------------------------------------------
	// Fast queries
	//-----------------------------------------------------
	bool Executed() const							{ return ( m_status == JOB_OK );	}
	bool CanExecute() const							{ return ( m_status == JOB_STATUS_PENDING || m_status == JOB_STATUS_UNSERVICED ); }
	bool IsFinished() const							{ return ( m_status != JOB_STATUS_PENDING && m_status != JOB_STATUS_INPROGRESS && m_status != JOB_STATUS_UNSERVICED ); }
	JobStatus_t GetStatus() const					{ return m_status; }

	/// Slam the status to a particular value.  This is named "slam" instead of "set,"
	/// to warn you that it should only be used in unusual situations.  Otherwise, the
	/// job manager really should manage the status for you, and you should not manhandle it.
	void SlamStatus(JobStatus_t s) { m_status = s; }
	
	//-----------------------------------------------------
	// Try to acquire ownership (to satisfy). If you take the lock, you must either execute or abort.
	//-----------------------------------------------------
	bool TryLock()										{ return m_mutex.TryLock(); }
	void Lock()											{ m_mutex.Lock(); }
	void Unlock()										{ m_mutex.Unlock(); }

	//-----------------------------------------------------
	// Thread event support (safe for NULL this to simplify code )
	//-----------------------------------------------------
	bool WaitForFinish( uint32 dwTimeout = TT_INFINITE ) { if (!this) return true; return ( !IsFinished() ) ? g_pThreadPool->YieldWait( this, dwTimeout ) : true; }
	bool WaitForFinishAndRelease( uint32 dwTimeout = TT_INFINITE ) { if (!this) return true; bool bResult = WaitForFinish( dwTimeout); Release(); return bResult; }
	CThreadEvent *AccessEvent()						{ return &m_CompleteEvent; }

	//-----------------------------------------------------
	// Perform the job
	//-----------------------------------------------------
	JobStatus_t Execute();
	JobStatus_t TryExecute();
	JobStatus_t ExecuteAndRelease()					{ JobStatus_t status = Execute(); Release(); return status;	}
	JobStatus_t TryExecuteAndRelease()				{ JobStatus_t status = TryExecute(); Release(); return status;	}

	//-----------------------------------------------------
	// Terminate the job, discard if partially or wholly fulfilled
	//-----------------------------------------------------
	JobStatus_t Abort( bool bDiscard = true );

	virtual char const *Describe()					{ return m_szDescription[ 0 ] ? m_szDescription : "Job"; }
	virtual void SetDescription( const char *pszDescription )
	{
		if( pszDescription )
		{
			Q_strncpy( m_szDescription, pszDescription, sizeof( m_szDescription ) );
		}
		else
		{
			m_szDescription[ 0 ] = 0;
		}
	}

private:
	//-----------------------------------------------------
	friend class CThreadPool;

	JobStatus_t			m_status;
	JobPriority_t		m_priority;
	CThreadMutex		m_mutex;
	unsigned char		m_flags;
	char				m_iServicingThread;
	short				m_reserved;
	ThreadPoolData_t	m_ThreadPoolData;
	IThreadPool *		m_pThreadPool;
	CThreadEvent		m_CompleteEvent;
	char				m_szDescription[ 32 ];

private:
	//-----------------------------------------------------
	CJob( const CJob &fromRequest );
	void operator=(const CJob &fromRequest );

	virtual JobStatus_t DoExecute() = 0;
	virtual JobStatus_t DoAbort( bool bDiscard ) { return JOB_STATUS_ABORTED; }
	virtual void DoCleanup() {}
};

//-----------------------------------------------------------------------------

class CFunctorJob : public CJob
{
public:
	CFunctorJob( CFunctor *pFunctor, const char *pszDescription = NULL )
		: m_pFunctor( pFunctor )
	{
		if ( pszDescription )
		{
			Q_strncpy( m_szDescription, pszDescription, sizeof(m_szDescription) );
		}
		else
		{
			m_szDescription[0] = 0;
		}
	}

	virtual JobStatus_t DoExecute()
	{
		(*m_pFunctor)();
		return JOB_OK;
	}

	const char *Describe()
	{
		return m_szDescription;
	}

private:
	CRefPtr<CFunctor> m_pFunctor;
	char m_szDescription[16];
};

//-----------------------------------------------------------------------------
// Utility for managing multiple jobs
//-----------------------------------------------------------------------------

class CJobSet
{
public:
	CJobSet( CJob *pJob = NULL )
	{
		if ( pJob )
		{
			m_jobs.AddToTail( pJob );
		}
	}

	CJobSet( CJob **ppJobs, int nJobs )
	{
		if ( ppJobs )
		{
			m_jobs.AddMultipleToTail( nJobs, ppJobs );
		}
	}

	~CJobSet()
	{
		for ( int i = 0; i < m_jobs.Count(); i++ )
		{
			m_jobs[i]->Release();
		}
	}

	void operator+=( CJob *pJob )
	{
		m_jobs.AddToTail( pJob );
	}

	void operator-=( CJob *pJob )
	{
		m_jobs.FindAndRemove( pJob );
	}

	void Execute( bool bRelease = true )
	{
		for ( int i = 0; i < m_jobs.Count(); i++ )
		{
			m_jobs[i]->Execute();
			if ( bRelease )
			{
				m_jobs[i]->Release();
			}
		}

		if ( bRelease )
		{
			m_jobs.RemoveAll();
		}
	}

	void Abort( bool bRelease = true )
	{
		for ( int i = 0; i < m_jobs.Count(); i++ )
		{
			m_jobs[i]->Abort();
			if ( bRelease )
			{
				m_jobs[i]->Release();
			}
		}

		if ( bRelease )
		{
			m_jobs.RemoveAll();
		}
	}

	void WaitForFinish( bool bRelease = true )
	{
		for ( int i = 0; i < m_jobs.Count(); i++ )
		{
			m_jobs[i]->WaitForFinish();
			if ( bRelease )
			{
				m_jobs[i]->Release();
			}
		}

		if ( bRelease )
		{
			m_jobs.RemoveAll();
		}
	}

	void WaitForFinish( IThreadPool *pPool, bool bRelease = true )
	{
		pPool->YieldWait( m_jobs.Base(), m_jobs.Count() );

		if ( bRelease )
		{
			for ( int i = 0; i < m_jobs.Count(); i++ )
			{
				m_jobs[i]->Release();
			}

			m_jobs.RemoveAll();
		}
	}

private:
	CUtlVectorFixed<CJob *, 16> m_jobs;
};

//-----------------------------------------------------------------------------
// Job helpers
//-----------------------------------------------------------------------------

#define ThreadExecute g_pThreadPool->QueueCall
#define ThreadExecuteRef g_pThreadPool->QueueRefCall

#define BeginExecuteParallel()	do { CJobSet jobSet
#define EndExecuteParallel()	jobSet.WaitForFinish( g_pThreadPool ); } while (0)

#define ExecuteParallel			jobSet += g_pThreadPool->QueueCall
#define ExecuteRefParallel		jobSet += g_pThreadPool->QueueCallRef


//-----------------------------------------------------------------------------
// Work splitting: array split, best when cost per item is roughly equal
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4389)
#pragma warning(disable:4018)
#pragma warning(disable:4701)
#endif

#define DEFINE_NON_MEMBER_ITER_RANGE_PARALLEL(N) \
	template <typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N, typename ITERTYPE1, typename ITERTYPE2> \
	void IterRangeParallel(FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( ITERTYPE1, ITERTYPE2 FUNC_ADDL_TEMPLATE_FUNC_PARAMS_##N ), ITERTYPE1 from, ITERTYPE2 to FUNC_ARG_FORMAL_PARAMS_##N ) \
	{ \
		const int MAX_THREADS = 16; \
		int nIdle = g_pThreadPool->NumIdleThreads(); \
		ITERTYPE1 range = to - from; \
		int nThreads = MIN( nIdle + 1, range ); \
		if ( nThreads > MAX_THREADS ) \
		{ \
			nThreads = MAX_THREADS; \
		} \
		if ( nThreads < 2 ) \
		{ \
			FunctorDirectCall( pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
		} \
		else \
		{ \
			ITERTYPE1 nIncrement = range / nThreads; \
			\
			CJobSet jobSet; \
			while ( --nThreads ) \
			{ \
				ITERTYPE2 thisTo = from + nIncrement; \
				jobSet += g_pThreadPool->AddCall( pfnProxied, from, thisTo FUNC_FUNCTOR_CALL_ARGS_##N ); \
				from = thisTo; \
			} \
			FunctorDirectCall( pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
			jobSet.WaitForFinish( g_pThreadPool ); \
		} \
		\
	}

FUNC_GENERATE_ALL( DEFINE_NON_MEMBER_ITER_RANGE_PARALLEL );

#define DEFINE_MEMBER_ITER_RANGE_PARALLEL(N) \
	template <typename OBJECT_TYPE, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N, typename ITERTYPE1, typename ITERTYPE2> \
	void IterRangeParallel(OBJECT_TYPE *pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( ITERTYPE1, ITERTYPE2 FUNC_ADDL_TEMPLATE_FUNC_PARAMS_##N ), ITERTYPE1 from, ITERTYPE2 to FUNC_ARG_FORMAL_PARAMS_##N ) \
	{ \
		const int MAX_THREADS = 16; \
		int nIdle = g_pThreadPool->NumIdleThreads(); \
		ITERTYPE1 range = to - from; \
		int nThreads = MIN( nIdle + 1, range ); \
		if ( nThreads > MAX_THREADS ) \
		{ \
			nThreads = MAX_THREADS; \
		} \
		if ( nThreads < 2 ) \
		{ \
			FunctorDirectCall( pObject, pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
		} \
		else \
		{ \
			ITERTYPE1 nIncrement = range / nThreads; \
			\
			CJobSet jobSet; \
			while ( --nThreads ) \
			{ \
				ITERTYPE2 thisTo = from + nIncrement; \
				jobSet += g_pThreadPool->AddCall( pObject, pfnProxied, from, thisTo FUNC_FUNCTOR_CALL_ARGS_##N ); \
				from = thisTo; \
			} \
			FunctorDirectCall( pObject, pfnProxied, from, to FUNC_FUNCTOR_CALL_ARGS_##N ); \
			jobSet.WaitForFinish( g_pThreadPool ); \
		} \
		\
	}

FUNC_GENERATE_ALL( DEFINE_MEMBER_ITER_RANGE_PARALLEL );

//-----------------------------------------------------------------------------
// Work splitting: competitive, best when cost per item varies a lot
//-----------------------------------------------------------------------------

template <typename T>
class CJobItemProcessor
{
public:
	typedef T ItemType_t;
	void Begin() {}
	// void Process( ItemType_t & ) {}
	void End() {}
};

template <typename T>
class CFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init(void (*pfnProcess)( T & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL )
	{
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	//CFuncJobItemProcessor(OBJECT_TYPE_PTR pObject, void (FUNCTION_CLASS::*pfnProcess)( ITEM_TYPE & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL );
	void Begin()						{ if ( m_pfnBegin ) (*m_pfnBegin)(); }
	void Process( T &item )	{ (*m_pfnProcess)( item ); }
	void End()							{ if ( m_pfnEnd ) (*m_pfnEnd)(); }

protected:
	void (*m_pfnProcess)( T & );
	void (*m_pfnBegin)();
	void (*m_pfnEnd)();
};

template <typename T, class OBJECT_TYPE, class FUNCTION_CLASS = OBJECT_TYPE >
class CMemberFuncJobItemProcessor : public CJobItemProcessor<T>
{
public:
	void Init( OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)( T & ), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL )
	{
		m_pObject = pObject;
		m_pfnProcess = pfnProcess;
		m_pfnBegin = pfnBegin;
		m_pfnEnd = pfnEnd;
	}

	void Begin()						{ if ( m_pfnBegin ) ((*m_pObject).*m_pfnBegin)(); }
	void Process( T &item )	            { ((*m_pObject).*m_pfnProcess)( item ); }
	void End()							{ if ( m_pfnEnd ) ((*m_pObject).*m_pfnEnd)(); }

protected:
	OBJECT_TYPE *m_pObject;
	void (FUNCTION_CLASS::*m_pfnProcess)( T & );
	void (FUNCTION_CLASS::*m_pfnBegin)();
	void (FUNCTION_CLASS::*m_pfnEnd)();
};

template <typename ITEM_TYPE, class ITEM_PROCESSOR_TYPE>
class CParallelProcessor
{
public:
	CParallelProcessor( const char *pszDescription )
	{
		m_pItems = m_pLimit= 0;
		m_szDescription = pszDescription;
	}

	void Run( ITEM_TYPE *pItems, unsigned nItems, int nMaxParallel = INT_MAX, IThreadPool *pThreadPool = NULL )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "Run %s %d", m_szDescription, nItems );

		if ( nItems == 0 )
			return;

		if ( !pThreadPool )
		{
			pThreadPool = g_pThreadPool;
		}

		m_pItems = pItems;
		m_pLimit = pItems + nItems;

		int nJobs = nItems - 1;

		if ( nJobs > nMaxParallel )
		{
			nJobs = nMaxParallel;
		}

		if (! pThreadPool )									// only possible on linux
		{
			DoExecute( );
			return;
		}

		int nThreads = pThreadPool->NumThreads();
		if ( nJobs > nThreads )
		{
			nJobs = nThreads;
		}

		if ( nJobs > 1 )
		{
			CJob **jobs = (CJob **)stackalloc( nJobs * sizeof(CJob **) );
			int i = nJobs;

			while( i-- )
			{
				jobs[i] = pThreadPool->QueueCall( this, &CParallelProcessor<ITEM_TYPE, ITEM_PROCESSOR_TYPE>::DoExecute );
				jobs[i]->SetDescription( m_szDescription );
			}

			DoExecute();

			for ( i = 0; i < nJobs; i++ )
			{
				jobs[i]->Abort(); // will either abort ones that never got a thread, or noop on ones that did
				jobs[i]->Release();
			}
		}
		else
		{
			DoExecute();
		}
	}

	ITEM_PROCESSOR_TYPE m_ItemProcessor;

private:
	void DoExecute()
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "DoExecute %s", m_szDescription );

		if ( m_pItems < m_pLimit )
		{
			m_ItemProcessor.Begin();

			ITEM_TYPE *pLimit = m_pLimit;

			for (;;)
			{
				ITEM_TYPE *pCurrent = m_pItems++;
				if ( pCurrent < pLimit )
				{
					m_ItemProcessor.Process( *pCurrent );
				}
				else
				{
					break;
				}
			}

			m_ItemProcessor.End();
		}
	}
	CInterlockedPtr<ITEM_TYPE>	m_pItems;
	ITEM_TYPE *					m_pLimit;
	const char *				m_szDescription;
};

template <typename ITEM_TYPE> 
inline void ParallelProcess( const char *pszDescription, ITEM_TYPE *pItems, unsigned nItems, void (*pfnProcess)( ITEM_TYPE & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL, int nMaxParallel = INT_MAX )
{
	CParallelProcessor<ITEM_TYPE, CFuncJobItemProcessor<ITEM_TYPE> > processor( pszDescription );
	processor.m_ItemProcessor.Init( pfnProcess, pfnBegin, pfnEnd );
	processor.Run( pItems, nItems, nMaxParallel );

}

template <typename ITEM_TYPE, typename OBJECT_TYPE, typename FUNCTION_CLASS > 
inline void ParallelProcess( const char *pszDescription, ITEM_TYPE *pItems, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)( ITEM_TYPE & ), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX )
{
	CParallelProcessor<ITEM_TYPE, CMemberFuncJobItemProcessor<ITEM_TYPE, OBJECT_TYPE, FUNCTION_CLASS> > processor( pszDescription );
	processor.m_ItemProcessor.Init( pObject, pfnProcess, pfnBegin, pfnEnd );
	processor.Run( pItems, nItems, nMaxParallel );
}

// Parallel Process that lets you specify threadpool
template <typename ITEM_TYPE> 
inline void ParallelProcess( const char *pszDescription, IThreadPool *pPool, ITEM_TYPE *pItems, unsigned nItems, void (*pfnProcess)( ITEM_TYPE & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL, int nMaxParallel = INT_MAX )
{
	CParallelProcessor<ITEM_TYPE, CFuncJobItemProcessor<ITEM_TYPE> > processor( pszDescription );
	processor.m_ItemProcessor.Init( pfnProcess, pfnBegin, pfnEnd );
	processor.Run( pItems, nItems, nMaxParallel, pPool );
}


template <class ITEM_PROCESSOR_TYPE>
class CParallelLoopProcessor
{
public:
	CParallelLoopProcessor( const char *pszDescription )
	{
		m_lIndex = m_lLimit= 0;
		m_nActive = 0;
		m_szDescription = pszDescription;
	}

	void Run( long lBegin, long nItems, int nMaxParallel = INT_MAX )
	{
		if ( nItems )
		{
			m_lIndex = lBegin;
			m_lLimit = lBegin + nItems;
			int i = g_pThreadPool->NumIdleThreads();

			if ( nMaxParallel < i)
			{
				i = nMaxParallel;
			}

			while( i-- )
			{
				++m_nActive;
				ThreadExecute( this, &CParallelLoopProcessor<ITEM_PROCESSOR_TYPE>::DoExecute )->Release();
			}

			++m_nActive;
			DoExecute();

			while ( m_nActive )
			{
				ThreadPause();
			}
		}
	}

	ITEM_PROCESSOR_TYPE m_ItemProcessor;

private:
	void DoExecute()
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "DoExecute %s", m_szDescription );

		m_ItemProcessor.Begin();

		long lLimit = m_lLimit;

		for (;;)
		{
			long lIndex = m_lIndex ++;
			if ( lIndex < lLimit )
			{
				m_ItemProcessor.Process( lIndex );
			}
			else
			{
				break;
			}
		}

		m_ItemProcessor.End();

		--m_nActive;
	}
	CInterlockedInt				m_lIndex;
	long						m_lLimit;
	CInterlockedInt				m_nActive;
	const char *				m_szDescription;
};

inline void ParallelLoopProcess( const char *szDescription, long lBegin, unsigned nItems, void (*pfnProcess)( long const & ), void (*pfnBegin)() = NULL, void (*pfnEnd)() = NULL, int nMaxParallel = INT_MAX )
{
	CParallelLoopProcessor< CFuncJobItemProcessor< long const > > processor( szDescription );
	processor.m_ItemProcessor.Init( pfnProcess, pfnBegin, pfnEnd );
	processor.Run( lBegin, nItems, nMaxParallel );

}

template < typename OBJECT_TYPE, typename FUNCTION_CLASS > 
inline void ParallelLoopProcess( const char *szDescription, long lBegin, unsigned nItems, OBJECT_TYPE *pObject, void (FUNCTION_CLASS::*pfnProcess)( long const & ), void (FUNCTION_CLASS::*pfnBegin)() = NULL, void (FUNCTION_CLASS::*pfnEnd)() = NULL, int nMaxParallel = INT_MAX )
{
	CParallelLoopProcessor< CMemberFuncJobItemProcessor<long const, OBJECT_TYPE, FUNCTION_CLASS> > processor( szDescription );
	processor.m_ItemProcessor.Init( pObject, pfnProcess, pfnBegin, pfnEnd );
	processor.Run( lBegin, nItems, nMaxParallel );
}


template <class Derived>
class CParallelProcessorBase
{
protected:
	typedef CParallelProcessorBase<Derived> ThisParallelProcessorBase_t;
	typedef Derived ThisParallelProcessorDerived_t;

public:
	CParallelProcessorBase()
	{
		m_nActive = 0;
		m_szDescription = NULL;
	}
	void SetDescription( const char *pszDescription )
	{
		m_szDescription = pszDescription;
	}

protected:
	void Run( int nMaxParallel = INT_MAX, int threadOverride = -1 )
	{
		int i = g_pThreadPool->NumIdleThreads();

		if ( nMaxParallel < i)
		{
			i = nMaxParallel;
		}

		while( i -- > 0 )
		{
			if (  threadOverride == -1 || i == threadOverride - 1 )
			{
				++ m_nActive;
				ThreadExecute( this, &ThisParallelProcessorBase_t::DoExecute )->Release();
			}
		}

		if (  threadOverride == -1 || threadOverride == 0 )
		{
			++ m_nActive;
			DoExecute();
		}

		while ( m_nActive )
		{
			ThreadPause();
		}
	}

protected:
	void OnBegin() {}
	bool OnProcess() { return false; }
	void OnEnd() {}

private:
	void DoExecute()
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "DoExecute %s", m_szDescription );

		static_cast<Derived *>( this )->OnBegin();

		while ( static_cast<Derived *>( this )->OnProcess() )
			continue;

		static_cast<Derived *>(this)->OnEnd();

		-- m_nActive;
	}

	CInterlockedInt				m_nActive;
	const char *				m_szDescription;
};




//-----------------------------------------------------------------------------
// Raw thread launching
//-----------------------------------------------------------------------------

inline uintp FunctorExecuteThread( void *pParam )
{
	CFunctor *pFunctor = (CFunctor *)pParam;
	(*pFunctor)();
	pFunctor->Release();
	return 0;
}

inline ThreadHandle_t ThreadExecuteSoloImpl( CFunctor *pFunctor, const char *pszName = NULL )
{
	ThreadHandle_t hThread;
	ThreadId_t threadId;
	hThread = CreateSimpleThread( FunctorExecuteThread, pFunctor, &threadId );
	if ( pszName )
	{
		ThreadSetDebugName( threadId, pszName );
	}
	return hThread;
}

inline ThreadHandle_t ThreadExecuteSolo( CJob *pJob ) { return ThreadExecuteSoloImpl( CreateFunctor( pJob, &CJob::Execute ), pJob->Describe()  ); }

template <typename T1> 																								
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1 ), pszName  ); }

template <typename T1, typename T2> 																				
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2 ), pszName  ); }

template <typename T1, typename T2, typename T3> 																	
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4> 														
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3, a4 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5> 											
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3, a4, a5 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> 							
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3, a4, a5, a6 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> 				
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3, a4, a5, a6, a7 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> 	
inline ThreadHandle_t ThreadExecuteSolo( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8 ) { return ThreadExecuteSoloImpl( CreateFunctor( a1, a2, a3, a4, a5, a6, a7, a8 ), pszName  ); }

template <typename T1, typename T2> 																				
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2 ), pszName  ); }

template <typename T1, typename T2, typename T3> 																	
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4> 														
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3, a4 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5> 											
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3, a4, a5 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6> 							
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7> 				
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6, a7 ), pszName  ); }

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8> 	
inline ThreadHandle_t ThreadExecuteSoloRef( const char *pszName, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8 ) { return ThreadExecuteSoloImpl( CreateRefCountingFunctor(a1, a2, a3, a4, a5, a6, a7, a8 ), pszName  ); }

//-----------------------------------------------------------------------------

inline bool IThreadPool::YieldWait( CThreadEvent &event, unsigned timeout )
{
	CThreadEvent *pEvent = &event;
	return ( YieldWait( &pEvent, 1, true, timeout ) != TW_TIMEOUT );
}

inline bool IThreadPool::YieldWait( CJob *pJob, unsigned timeout )
{
	return ( YieldWait( &pJob, 1, true, timeout ) != TW_TIMEOUT );
}

//-----------------------------------------------------------------------------

inline JobStatus_t CJob::Execute()
{
	if ( IsFinished() )
	{
		return m_status;
	}

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s %s %d", __FUNCTION__, Describe(), m_status );

	AUTO_LOCK( m_mutex );
	AddRef();

	JobStatus_t result;

	switch ( m_status )
	{
	case JOB_STATUS_UNSERVICED:
	case JOB_STATUS_PENDING:
		{
			// Service it
			m_status = JOB_STATUS_INPROGRESS;
			result = m_status = DoExecute();
			DoCleanup();
			m_CompleteEvent.Set();
			break;
		}

	case JOB_STATUS_INPROGRESS:
		AssertMsg(0, "Mutex Should have protected use while processing");
		// fall through...

	case JOB_OK:
	case JOB_STATUS_ABORTED:
		result = m_status;
		break;

	default:
		AssertMsg( m_status < JOB_OK, "Unknown job state");
		result = m_status;
	}

	Release();

	return result;
}


//---------------------------------------------------------

inline JobStatus_t CJob::TryExecute()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s %s %d", __FUNCTION__, Describe(), m_status );

	// TryLock() would only fail if another thread has entered
	// Execute() or Abort()
	if ( !IsFinished() && TryLock() )
	{
		// ...service the request
		Execute();
		Unlock();
	}
	return m_status;
}

//---------------------------------------------------------

inline JobStatus_t CJob::Abort( bool bDiscard )
{
	if ( IsFinished() )
	{
		return m_status;
	}

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s %s %d", __FUNCTION__, Describe(), m_status );

	AUTO_LOCK( m_mutex );
	AddRef();

	JobStatus_t result;

	switch ( m_status )
	{
	case JOB_STATUS_UNSERVICED:
	case JOB_STATUS_PENDING:
		{
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "CJob::DoAbort" );

			result = m_status = DoAbort( bDiscard );
			if ( bDiscard )
				DoCleanup();
			m_CompleteEvent.Set();
		}
		break;

	case JOB_STATUS_ABORTED:
	case JOB_STATUS_INPROGRESS:
	case JOB_OK:
		result = m_status;
		break;

	default:
		AssertMsg( m_status < JOB_OK, "Unknown job state");
		result = m_status;
	}

	Release();

	return result;
}

//-----------------------------------------------------------------------------

#endif // JOBTHREAD_H
