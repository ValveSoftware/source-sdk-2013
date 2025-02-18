//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// MessageWatchDlg.h : header file
//

#if !defined(AFX_MESSAGEWATCHDLG_H__AB9CEAF4_0166_4CCA_9DEC_77C0918F78C4__INCLUDED_)
#define AFX_MESSAGEWATCHDLG_H__AB9CEAF4_0166_4CCA_9DEC_77C0918F78C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "iphelpers.h"
#include "tcpsocket.h"
#include "threadhelpers.h"
#include "consolewnd.h"
#include "win_idle.h"


/////////////////////////////////////////////////////////////////////////////
// CMessageWatchDlg dialog

class CSender
{
public:
				CSender();
				~CSender();

public:

	CIPAddr		m_Addr;
	ITCPSocket	*m_pSocket;
	IConsoleWnd	*m_pConsoleWnd;
	char		m_Name[128];
};

class CMessageWatchDlg : public CDialog
{
// Construction
public:
	CMessageWatchDlg(CWnd* pParent = NULL);	// standard constructor
	~CMessageWatchDlg();


	// Listen for broadcasts on this socket.
	ISocket							*m_pListenSocket;
	
	// Connections we've made.
	CUtlLinkedList<CSender*,int>	m_Senders;

	CCriticalSection				m_SocketsCS;
	CWinIdle						m_cWinIdle;


	CSender*						FindSenderByAddr( const unsigned char ip[4] );
	CSender*						FindSenderByName( const char *pName );


// Dialog Data
	//{{AFX_DATA(CMessageWatchDlg)
	enum { IDD = IDD_MESSAGEWATCH_DIALOG };
	CListBox	m_Machines;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessageWatchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	void			OnIdle();

	// Generated message map functions
	//{{AFX_MSG(CMessageWatchDlg)
	afx_msg void OnDestroy();
	afx_msg LONG OnStartIdle(UINT, LONG);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDblclkMachines();
	afx_msg void OnShowall();
	afx_msg void OnHideall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MESSAGEWATCHDLG_H__AB9CEAF4_0166_4CCA_9DEC_77C0918F78C4__INCLUDED_)
