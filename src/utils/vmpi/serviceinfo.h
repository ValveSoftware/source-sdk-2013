//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#pragma once

class CServiceInfo
{
public:

	bool IsOff() const;	// Returns true if the time since we've heard from this guy is too long.
	CServiceInfo() : m_bFromRegistry( false ) {}

public:

	CString	m_ComputerName;
	CString	m_MasterName;
	CString m_Password;
	int		m_iState;

	// Since the live time is always changing, we only update it every 10 seconds or so.
	DWORD	m_LiveTimeMS;			// How long the service has been running (in milliseconds).

	DWORD	m_WorkerAppTimeMS;		// How long the worker app has been running (0 if it's not running).

	DWORD	m_LastPingTimeMS;		// Last time we heard from this machine. Used to detect if the service
									// is off or not.

									// Used to detect if we need to re-sort the list.
	const char *m_pLastStatusText;
	DWORD		m_LastLiveTimeMS;
	CString		m_LastMasterName;

	int			m_CPUPercentage;
	CString		m_ExeName;
	CString		m_MapName;
	int			m_MemUsageMB;

	// Last time we updated the service in the listbox.. used to make sure we update its on/off status
	// every once in a while.
	DWORD		m_LastUpdateTime;

	int			m_ProtocolVersion;		// i.e. the service's VMPI_SERVICE_PROTOCOL_VERSION.
	char		m_ServiceVersion[ 32 ];	// Version string.

	CIPAddr	m_Addr;
	bool		m_bFromRegistry;
};
