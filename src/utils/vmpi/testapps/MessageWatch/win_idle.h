//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// WinIdle.h - Defines a class for sending idle messages to a window from a secondary thread

#ifndef __WINIDLE_H__
#define __WINIDLE_H__


class CWinIdle
{
protected:
	HANDLE m_hIdleEvent, m_hStopEvent;

	HWND m_hWnd;
	UINT m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;

	DWORD m_dwDelay;

	HANDLE m_hIdleThread;

	// The thread calling stub
	static DWORD WINAPI ThreadStub(LPVOID pIdle);
	// The actual idle loop
	virtual DWORD RunIdle();
	
public:
	CWinIdle();
	virtual ~CWinIdle();

	inline DWORD	GetDelay()				{return m_dwDelay;}
	inline void		SetDelay(DWORD delay)	{m_dwDelay = delay;}

	// Member access
	virtual HANDLE GetThreadHandle() const { return m_hIdleThread; };

	// Start idling, and define the message and window to use
	//	Returns TRUE on success
	virtual BOOL StartIdle(HWND hWnd, UINT uMessage, WPARAM wParam = 0, LPARAM lParam = 0, DWORD dwDelay = 0);
	// Stop idling
	//	Returns TRUE on success
	virtual BOOL EndIdle();
	// Notify the idle process that the message was received.
	//	Note : If this function is not called, the idle thread will not send any messages
	virtual void NextIdle();
};


// Used to slow down the idle thread while dialogs are up.
class IdleChanger
{
public:
		IdleChanger(CWinIdle *pIdle, DWORD msDelay)
		{
			m_pIdle = pIdle;
			m_OldDelay = pIdle->GetDelay();
			pIdle->SetDelay(msDelay);
		}

		~IdleChanger()
		{
			m_pIdle->SetDelay(m_OldDelay);
		}

	CWinIdle		*m_pIdle;
	DWORD			m_OldDelay;
};



#endif //__WINIDLE_H__

