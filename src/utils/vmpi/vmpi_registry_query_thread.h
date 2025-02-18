//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once

#include "iphelpers.h"
#include "serviceinfo.h"
#include "tier1/utlvector.h"


class CVMPIRegistryQueryThread
{
public:
	CVMPIRegistryQueryThread();
	~CVMPIRegistryQueryThread();

	// This creates the thread that periodically checks "net view" to get the current list of
	// machines out on the network.
	void Init();
	void Term();
	
	void GetRegisteredServices( CUtlVector<CServiceInfo> &services );

private:

	void UpdateServicesFromRegistry();
	
	DWORD ThreadFn();
	static DWORD WINAPI StaticThreadFn( LPVOID lpParameter );

	CUtlVector<CServiceInfo> m_RegisteredServices;
	HANDLE m_hThread;
	HANDLE m_hThreadExitEvent;
	CRITICAL_SECTION m_ComputerNamesCS;
};
