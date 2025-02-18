//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef NET_VIEW_THREAD_H
#define NET_VIEW_THREAD_H
#ifdef _WIN32
#pragma once
#endif


#include "utlvector.h"


class CNetViewThread
{
public:
	CNetViewThread();
	~CNetViewThread();

	// This creates the thread that periodically checks "net view" to get the current list of
	// machines out on the network.
	void Init();
	void Term();
	
	// If bRemoveFromList is true, then it'll remove the names that it's returning from its internal
	// list and it won't return those names again.
	void GetComputerNames( CUtlVector<char*> &computerNames, bool bRemoveFromList );

private:

	void UpdateServicesFromNetView();
	void ParseComputerNames( const char *pNetViewOutput );
	
	DWORD ThreadFn();
	static DWORD WINAPI StaticThreadFn( LPVOID lpParameter );

	CUtlVector<char*> m_ComputerNames;
	HANDLE m_hThread;
	HANDLE m_hThreadExitEvent;
	CRITICAL_SECTION m_ComputerNamesCS;
};


#endif // NET_VIEW_THREAD_H
