//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// Class for sending idle messages to a window

#include "stdafx.h"
#include "win_idle.h"

// Stub function to get into the object's main thread loop
DWORD WINAPI CWinIdle::ThreadStub(LPVOID pIdle)
{
	return ((CWinIdle *)pIdle)->RunIdle();
}

CWinIdle::CWinIdle() :
	m_hIdleThread(NULL),
	m_hIdleEvent(NULL),
	m_hStopEvent(NULL),
	m_hWnd(0),
	m_uMsg(0),
	m_dwDelay(0)
{
}

CWinIdle::~CWinIdle()
{
	EndIdle();
}

DWORD CWinIdle::RunIdle()
{
	// Set up an event list
	HANDLE aEvents[2];

	aEvents[0] = m_hStopEvent;
	aEvents[1] = m_hIdleEvent;

	// Wait for a stop or idle event
	while (WaitForMultipleObjects(2, aEvents, FALSE, INFINITE) != WAIT_OBJECT_0)
	{
		// Send an idle message
		PostMessage(m_hWnd, m_uMsg, m_wParam, m_lParam);
		// Wait for a bit...
		Sleep(m_dwDelay);
	}

	return 0;
}

BOOL CWinIdle::StartIdle(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam, DWORD dwDelay)
{
	// Make sure it's not already running
	if (m_hIdleThread)
		return FALSE;

	// Make sure they send in a valid handle..
	if (!hWnd)
		return FALSE;

	// Create the events
	m_hIdleEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// Make sure the events got created
	if ((!m_hIdleEvent) || (!m_hStopEvent))
		return FALSE;

	// Create the thread
	DWORD dwThreadID;
	m_hIdleThread = CreateThread(NULL, 0, CWinIdle::ThreadStub, (void *)this, 0, &dwThreadID);

	if (m_hIdleThread)
	{
		SetThreadPriority(m_hIdleThread, THREAD_PRIORITY_IDLE);

		m_hWnd = hWnd;
		m_uMsg = uMessage;
		m_wParam = wParam;
		m_lParam = lParam;

		m_dwDelay = dwDelay;
	}

	return m_hIdleThread != 0;
}

BOOL CWinIdle::EndIdle()
{
	// Make sure it's running
	if (!m_hIdleThread)
		return FALSE;

	// Stop the idle thread
	SetEvent(m_hStopEvent);
	WaitForSingleObject(m_hIdleThread, INFINITE);
	CloseHandle(m_hIdleThread);

	// Get rid of the event objects
	CloseHandle(m_hIdleEvent);
	CloseHandle(m_hStopEvent);

	// Set everything back to 0
	m_hIdleEvent = 0;
	m_hStopEvent = 0;
	m_hIdleThread = 0;

	return TRUE;
}

void CWinIdle::NextIdle()
{
	// Make sure the thread's running
	if (!m_hIdleThread)
		return;

	// Signal an idle message
	SetEvent(m_hIdleEvent);
}
