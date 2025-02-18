//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef GC_JOBMGR_H
#define GC_JOBMGR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/fasttimer.h"
#include "tier1/utlpriorityqueue.h"
#include "gcsdk_types.h"
#include "job.h"
#include "workthreadpool.h"
class GCConVar;

#include "tier0/memdbgon.h"

namespace GCSDK
{

#if defined(_DEBUG)
// this is restricted to debug builds due to the performance cost
// that could be changed by removing the expensive sm_listAllJobs.Find() command
#define DEBUG_JOB_LIST
#endif // defined(_DEBUG)

struct JobStats_t
{
	uint m_cJobsCurrent;
	uint m_cJobsTotal;
	uint m_cJobsFailed;
	uint64 m_cJobsTimedOut;		// # of jobs timed out ever
	double m_flSumJobTimeMicrosec;
	double m_flSumSqJobTimeMicrosec;
	uint64 m_unMaxJobTimeMicrosec;

	uint m_cTimeslices;

	JobStats_t()
	{
		memset( this, 0, sizeof(JobStats_t) );
	}
};

struct JobStatsBucket_t
{
	JobStatsBucket_t()
	{
		memset( this, 0, sizeof(JobStatsBucket_t) );
	}
	char m_rgchName[64];
	uint64 m_cCompletes;
	uint64 m_u64RunTimeMax;
	uint64 m_cTimeoutNetMsg;
	uint64 m_cLongInterYieldTime;
	uint64 m_cLocksAttempted;
	uint64 m_cLocksWaitedFor;
	uint64 m_cLocksFailed;
	uint64 m_cLocksLongHeld;
	uint64 m_cLocksLongWait;
	uint64 m_cWaitTimeout;
	uint64 m_u64JobDuration;
	uint64 m_cJobsPaused;
	uint64 m_cJobsFailed;
	uint64 m_u64RunTime;
	// use by ListJobs
	uint64 m_cPauseReasonNetworkMsg;
	uint64 m_cPauseReasonSleepForTime;
	uint64 m_cPauseReasonWaitingForLock;
	uint64 m_cPauseReasonYield;
	uint64 m_cPauseReasonSQL;
	uint64 m_cPauseReasonWorkItem;

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName )
	{
		VALIDATE_SCOPE();
	}
#endif
};

enum EJobProfileAction 
{
	k_EJobProfileAction_ErrorReport = 0,
	k_EJobProfileAction_Start = 1,
	k_EJobProfileAction_Stop = 2,
	k_EJobProfileAction_Dump = 3,
	k_EJobProfileAction_Clear = 4,
};

enum EJobProfileSortOrder 
{
	k_EJobProfileSortOrder_Alpha = 0,
	k_EJobProfileSortOrder_Count = 1,
	k_EJobProfileSortOrder_TotalRuntime = 2,
};

struct JobProfileStats_t
{
	int m_iJobProfileSort;
	CUtlMap< uint32, JobStatsBucket_t, int > *pmapStatsBucket;
};

//-----------------------------------------------------------------------------
// Purpose: This keeps track of all jobs that belong to a given hub.
//			It's primarily used for routing incoming messages to jobs.
//-----------------------------------------------------------------------------
class CJobMgr
{ 
public:
	// Constructors & destructors
	CJobMgr();
	~CJobMgr();

	// gets the next available job ID
	JobID_t GetNewJobID();

	// Set the thread count for the internal thread pool(s)
	void SetThreadPoolSize( uint cThreads );

	// Run any sleeping jobs who's wakeup time has arrived and check for timeouts
	bool BFrameFuncRunSleepingJobs( CLimitTimer &limitTimer );

	// Run any yielding jobs, even low priority ones
	bool BFrameFuncRunYieldingJobs( CLimitTimer &limitTimer );

	// Route this message to an existing Job, or create a new one if that JobID does not exist
	bool BRouteMsgToJob( void *pParent, IMsgNetPacket *pNetPacket, const JobMsgInfo_t &jobMsgInfo );

	// Adds a new Job to the mgr and generates a JobID for it.
	void InsertJob( CJob &job );

	// Removes a Job from the mgr (the caller is still responsible for freeing it)
	void RemoveJob( CJob &job );

	//called by a job that has just been started to place itself on the yield queue instead of running
	void AddDelayedJobToYieldList( CJob &job );


	// returns true if we're running any jobs of the specified name
	// slow to call if lots of jobs are running, should only be used by tests
	bool BIsJobRunning( const char *pchJobName );

	// passes a network msg directly to the specified job
	void PassMsgToJob( CJob &job, IMsgNetPacket *pNetPacket, const JobMsgInfo_t &jobMsgInfo );

	// yields until a network message is received
	bool BYieldingWaitForMsg( CJob &job );

	// yields for a set amount of time
	bool BYieldingWaitTime( CJob &job, uint32 cMicrosecondsToSleep );

	// simple yield until Run() called again
	bool BYield( CJob &job );

	// Yield only if job manager decides we need to
	bool BYieldIfNeeded( CJob &job, bool *pbYielded );

	// Thread pool work item
	bool BYieldingWaitForWorkItem( CJob &job, const char *pszWorkItemName = NULL );
	bool BRouteWorkItemCompleted( JobID_t jobID, bool bWorkItemCanceled )	{ return BRouteWorkItemCompletedInternal( jobID, bWorkItemCanceled, /* bShouldExist */ true, /* bResumeImmediately */ true ); }
	bool BRouteWorkItemCompletedIfExists( JobID_t jobID, bool bWorkItemCanceled ) { return BRouteWorkItemCompletedInternal( jobID, bWorkItemCanceled, /* bShouldExist */ false, /* bResumeImmediately */ true ); }
	bool BRouteWorkItemCompletedDelayed( JobID_t jobID, bool bWorkItemCanceled )	{ return BRouteWorkItemCompletedInternal( jobID, bWorkItemCanceled, /* bShouldExist */ true, /* bResumeImmediately */ false ); }
	bool BRouteWorkItemCompletedIfExistsDelayed( JobID_t jobID, bool bWorkItemCanceled ) { return BRouteWorkItemCompletedInternal( jobID, bWorkItemCanceled, /* bShouldExist */ false, /* bResumeImmediately */ false ); }

	void AddThreadedJobWorkItem( CWorkItem *pWorkItem );
	void StopWorkThreads() { m_WorkThreadPool.StopWorkThreads(); }

	static int ProfileSortFunc( void *pCtx, const int *lhs, const int *rhs );

	void ProfileJobs( EJobProfileAction ejobProfileAction, EJobProfileSortOrder iSortOrder = k_EJobProfileSortOrder_Alpha );
	int DumpJobSummary();
	void DumpJob( JobID_t jobID, int nPrintLocksMax = 20 ) const;
	int CountJobs() const;	// counts currently active jobs
	void CheckThreadID(); // make sure we are still in the correct thread
	int CountYieldingJobs() const { return m_ListJobsYieldingRegPri.Count(); } // counts jobs currently in a yielding state
	bool HasOutstandingThreadPoolWorkItems();
	
	void SetIsShuttingDown();
	bool GetIsShuttingDown() const { return m_bIsShuttingDown; }

	void *GetMainMemoryDebugInfo() { return g_memMainDebugInfo.Base(); }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );		// Validate our internal structures
	static void ValidateStatics( CValidator &validator, const char *pchName );
#endif /* DBGFLAG_VALIDATE */

	// wakes up a job that was waiting on a lock
	void WakeupLockedJob( CJob &job );

    // returns true if there is a job active with the specified ID
	bool BJobExists( JobID_t jobID ) const;

	// returns a job
	CJob *GetPJob( JobID_t jobID );
	const CJob *GetPJob( JobID_t jobID ) const;

	JobStats_t& GetJobStats() { return m_JobStats; }

	// Access work thread pool directly
	CWorkThreadPool *AccessWorkThreadPool() { return &m_WorkThreadPool; }

	// Debug helpers
	// dumps a list of all running jobs across ALL job managers
	void DumpJobs( const char *pszJobName, int nMax, int nPrintLocksMax = 1 ) const;
	// cause a debug break in the given job
	static void DebugJob( int iJob );

	// disable/enable yielding for debugging
	void SetPauseAllowed( bool bNewPauseAllowed ) { m_bDebugDisallowPause = !bNewPauseAllowed; }

private:

	bool BRouteWorkItemCompletedInternal( JobID_t jobID, bool bWorkItemCanceled, bool bShouldExist, bool bResumeImmediately );

	// Create a new job for this message
	bool BLaunchJobFromNetworkMsg( void *pParent, const JobMsgInfo_t &jobMsgInfo, IMsgNetPacket *pNetPacket );

	// Internal add to yield list (looks at priority)
	void AddToYieldList( CJob &job );

	// Get an IJob given a job ID and pause reason
	bool BGetIJob( JobID_t jobID, EJobPauseReason eJobPauseReason, bool bShouldExist, int *pIJob );

	// Map containing all of our jobs
	CUtlMap<JobID_t, CJob *, int> m_MapJob;

	// jobs simply waiting until the next Run()
	struct JobYielding_t
	{
		JobID_t m_JobID;
		uint m_nIteration;
	};
	CUtlLinkedList<JobYielding_t, int> m_ListJobsYieldingRegPri;
	bool BResumeYieldingJobs( CLimitTimer &limitTimer );
	bool BResumeYieldingJobsFromList( CUtlLinkedList<JobYielding_t, int> &listJobsYielding, uint nCurrentIteration, CLimitTimer &limitTimer );
	uint m_nCurrentYieldIterationRegPri;

	// jobs waiting on a timer
	struct JobSleeping_t
	{
		JobID_t m_JobID;
		CJobTime m_SWakeupTime;
		CJobTime m_STimeTouched;
	};
	CUtlPriorityQueue<JobSleeping_t> m_QueueJobSleeping;
	bool BResumeSleepingJobs( CLimitTimer &limitTimer );
	static bool JobSleepingLessFunc( JobSleeping_t const &lhs, JobSleeping_t const &rhs );

	// timeout list of jobs, ordered from oldest to newest
	struct JobTimeout_t
	{
		JobID_t m_JobID;
		CJobTime m_STimePaused;
		CJobTime m_STimeTouched;
		uint32 m_cHeartbeatsBeforeTimeout;
	};
	CUtlLinkedList<JobTimeout_t, int> m_ListJobTimeouts;
	CUtlMap<JobID_t, int, int> m_MapJobTimeoutsIndexByJobID;
	void PauseJob( CJob &job, EJobPauseReason eJobPauseReason );
	void CheckForJobTimeouts( CLimitTimer &limitTimer );
	void TimeoutJob( CJob &job );
	bool m_bJobTimedOut;

	// thread pool usage, for running job functions in other threads
	CWorkThreadPool m_WorkThreadPool;

	void AccumulateStatsofJob( CJob &job );
	void RecordOrphanedMessage( MsgType_t eMsg, JobID_t jobIDTarget );

	// stats info
	JobStats_t m_JobStats;

	// static job registration
	static void RegisterJobType( const JobType_t *pJobType ); 
	friend void Job_RegisterJobType( const JobType_t *pJobType );

	JobID_t m_unNextJobID;
	ThreadId_t m_unFrameFuncThreadID; // the thread is JobMgr is working in
	bool m_bProfiling;
	bool m_bIsShuttingDown;
	int m_cErrorsToReport;
	CUtlMap< uint32, JobStatsBucket_t, int > m_mapStatsBucket;
	CUtlMap<MsgType_t, int, int> m_mapOrphanMessages;
	CUtlMemory<unsigned char> g_memMainDebugInfo;


#ifdef DEBUG_JOB_LIST
	// static job debug list
	static CUtlLinkedList<CJob *, int> sm_listAllJobs;
#endif

	bool m_bDebugDisallowPause;
};


//-----------------------------------------------------------------------------
// Purpose: passthrough function just so the CJob internal data can be kept private
//-----------------------------------------------------------------------------
inline void Job_RegisterJobType( const JobType_t *pJobType )
{
	CJobMgr::RegisterJobType( pJobType );
}


//-----------------------------------------------------------------------------
// Purpose: passthrough function just so the CJob internal data can be kept private
//-----------------------------------------------------------------------------
inline void Job_SetJobType( CJob &job, const JobType_t *pJobType )
{
	job.m_pJobType = pJobType;
}


//-----------------------------------------------------------------------------
// Purpose: job registration macro
//-----------------------------------------------------------------------------
#define GC_REG_JOB( parentclass, jobclass, jobname, msg, servertype ) \
	GCSDK::CJob *CreateJob_##jobclass( parentclass *pParent, void * pvStartParam ); \
	static const GCSDK::JobType_t g_JobType_##jobclass = { jobname, (GCSDK::MsgType_t)msg, servertype, (GCSDK::JobCreationFunc_t)CreateJob_##jobclass }; \
	GCSDK::CJob *CreateJob_##jobclass( parentclass *pParent, void * pvStartParam ) \
	{ \
		GCSDK::CJob *job = GCSDK::CJob::AllocateJob<jobclass>( pParent ); \
		if ( job ) \
		{ \
			Job_SetJobType( *job, &g_JobType_##jobclass ); \
			if ( pvStartParam ) job->SetStartParam( pvStartParam ); \
		} \
		else \
		{ \
			AssertMsg( job, "CJob::AllocateJob<" #jobclass "> returned NULL!" ); \
		} \
		return job; \
	} \
	static class CRegJob_##jobclass \
	{ \
	public: CRegJob_##jobclass() \
		{ \
		Job_RegisterJobType( &g_JobType_##jobclass ); \
		} \
	} g_RegJob_##jobclass;


//-----------------------------------------------------------------------------
// Purpose: job registration macro for job triggered by web api request
//-----------------------------------------------------------------------------
#define REG_WEBAPI_JOB( parentclass, jobclass, jobname, servertype ) \
	CJob *CreateJob_##jobclass( parentclass *pParent, void * pvStartParam ); \
	static const JobType_t g_JobType_##jobclass = { jobname, k_EGCMsgInvalid, servertype, (JobCreationFunc_t)CreateJob_##jobclass }; \
	CJob *CreateJob_##jobclass( parentclass *pParent, void * pvStartParam ) \
{ \
	CJob *job = CJob::AllocateJob<jobclass>( pParent ); \
	if ( job ) \
	{ \
		Job_SetJobType( *job, &g_JobType_##jobclass ); \
		if ( pvStartParam ) job->SetStartParam( pvStartParam ); \
	} \
	else \
	{ \
		AssertMsg( job, "CJob::AllocateJob<" #jobclass "> returned NULL!" ); \
	} \
	return job; \
} \
	static class CRegJob_##jobclass \
{ \
public: CRegJob_##jobclass() \
{ \
	Job_RegisterJobType( &g_JobType_##jobclass ); \
} \
} g_RegJob_##jobclass;



} // namespace GCSDK

#include "tier0/memdbgoff.h"

#endif // GC_JOBMGR_H
