//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "MySqlDatabase.h"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMySqlDatabase::CMySqlDatabase()
{
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//			blocks until db process thread has stopped
//-----------------------------------------------------------------------------
CMySqlDatabase::~CMySqlDatabase()
{
	// flag the thread to stop
	m_bRunThread = false;

	// pulse the thread to make it run
	::SetEvent(m_hEvent);

	// make sure it's done
	::EnterCriticalSection(&m_csThread);
	::LeaveCriticalSection(&m_csThread);
}

//-----------------------------------------------------------------------------
// Purpose: Thread access function
//-----------------------------------------------------------------------------
static DWORD WINAPI staticThreadFunc(void *param)
{
	((CMySqlDatabase *)param)->RunThread();
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Establishes connection to the database and sets up this object to handle db command
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CMySqlDatabase::Initialize()
{
	// prepare critical sections
	//!! need to download SDK and replace these with InitializeCriticalSectionAndSpinCount() calls
	::InitializeCriticalSection(&m_csThread);
	::InitializeCriticalSection(&m_csInQueue);
	::InitializeCriticalSection(&m_csOutQueue);
	::InitializeCriticalSection(&m_csDBAccess);

	// initialize wait calls
	m_hEvent = ::CreateEvent(NULL, false, true, NULL);

	// start the DB-access thread
	m_bRunThread = true;

	unsigned long threadID;
	::CreateThread(NULL, 0, staticThreadFunc, this, 0, &threadID);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Main thread loop
//-----------------------------------------------------------------------------
void CMySqlDatabase::RunThread()
{
	::EnterCriticalSection(&m_csThread);
	while (m_bRunThread)
	{
		if (m_InQueue.Count() > 0)
		{
			// get a dispatched DB request
			::EnterCriticalSection(&m_csInQueue);

			// pop the front of the queue
			int headIndex = m_InQueue.Head();
			msg_t msg = m_InQueue[headIndex];
			m_InQueue.Remove(headIndex);

			::LeaveCriticalSection(&m_csInQueue);

			::EnterCriticalSection(&m_csDBAccess);
			
			// run sqldb command
			msg.result = msg.cmd->RunCommand();

			::LeaveCriticalSection(&m_csDBAccess);

			if (msg.replyTarget)
			{
				// put the results in the outgoing queue
				::EnterCriticalSection(&m_csOutQueue);
				m_OutQueue.AddToTail(msg);
				::LeaveCriticalSection(&m_csOutQueue);

				// wake up out queue
				msg.replyTarget->WakeUp();
			}
			else
			{
				// there is no return data from the call, so kill the object now
				msg.cmd->deleteThis();
			}
		}
		else
		{
			// nothing in incoming queue, so wait until we get the signal
			::WaitForSingleObject(m_hEvent, INFINITE);
		}

		// check the size of the outqueue; if it's getting too big, sleep to let the main thread catch up
		if (m_OutQueue.Count() > 50)
		{
			::Sleep(2);
		}
	}
	::LeaveCriticalSection(&m_csThread);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a database command to the queue, and wakes the db thread
//-----------------------------------------------------------------------------
void CMySqlDatabase::AddCommandToQueue(ISQLDBCommand *cmd, ISQLDBReplyTarget *replyTarget, int returnState)
{
	::EnterCriticalSection(&m_csInQueue);

	// add to the queue
	msg_t msg = { cmd, replyTarget, 0, returnState };
	m_InQueue.AddToTail(msg);

	::LeaveCriticalSection(&m_csInQueue);

	// signal the thread to start running
	::SetEvent(m_hEvent);
}

//-----------------------------------------------------------------------------
// Purpose: Dispatches responses to SQLDB queries
//-----------------------------------------------------------------------------
bool CMySqlDatabase::RunFrame()
{
	bool doneWork = false;

	while (m_OutQueue.Count() > 0)
	{
		::EnterCriticalSection(&m_csOutQueue);

		// pop the first item in the queue
		int headIndex = m_OutQueue.Head();
		msg_t msg = m_OutQueue[headIndex];
		m_OutQueue.Remove(headIndex);

		::LeaveCriticalSection(&m_csOutQueue);

		// run result
		if (msg.replyTarget)
		{
			msg.replyTarget->SQLDBResponse(msg.cmd->GetID(), msg.returnState, msg.result, msg.cmd->GetReturnData());

			// kill command
			// it would be a good optimization to be able to reuse these
			msg.cmd->deleteThis();
		}

		doneWork = true;
	}

	return doneWork;
}

//-----------------------------------------------------------------------------
// Purpose: load info - returns the number of sql db queries waiting to be processed
//-----------------------------------------------------------------------------
int CMySqlDatabase::QueriesInOutQueue()
{
	// the queue names are from the DB point of view, not the server - thus the reversal
	return m_InQueue.Count();
}

//-----------------------------------------------------------------------------
// Purpose: number of queries finished processing, waiting to be responded to
//-----------------------------------------------------------------------------
int CMySqlDatabase::QueriesInFinishedQueue()
{
	return m_OutQueue.Count();
}
