//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MYSQL_ASYNC_H
#define MYSQL_ASYNC_H
#ifdef _WIN32
#pragma once
#endif


#include "tier0/fasttimer.h"


class IMySQL;


class CQueryResults
{
public:
	IMySQLRowSet *m_pResults;
	void *m_pUserData;			// This is the value passed to Execute.
	CCycleCount m_ExecuteTime;	// How long it took to execute this query in MySQL.
	CCycleCount m_QueueTime;	// How long it spent in the queue.
};


// This provides a way to do asynchronous MySQL queries. They are executed in a thread,
// then you can pop the results off as they arrive.
class IMySQLAsync
{
public:
	
	virtual void Release() = 0;

	// After finishing the current query, if there is one, this immediately executes 
	// the specified query and returns its results.
	virtual IMySQLRowSet* ExecuteBlocking( const char *pStr ) = 0;

	// Queue up a query.
	virtual void Execute( const char *pStr, void *pUserData ) = 0;
	
	// Poll this to pick up results from Execute().
	// NOTE: if this returns true, but pResults is set to NULL, then that query had an error.
	virtual bool GetNextResults( CQueryResults &results ) = 0;
};

// Create an async mysql interface. Note: if this call returns a non-null value, 
// then after this call, you do NOT own pSQL anymore and you shouldn't ever call
// a function in it again.
IMySQLAsync* CreateMySQLAsync( IMySQL *pSQL );


#endif // MYSQL_ASYNC_H
