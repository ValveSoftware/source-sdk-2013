//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined(AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_)
#define AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// JobWatchDlg.h : header file
//


#include "idle_dialog.h"
#include "resource.h"
#include "utlvector.h"
#include "imysqlwrapper.h"
#include "GraphControl.h"
#include "window_anchor_mgr.h"
#include "mysql_async.h"


class CWorkerInfo
{
public:
	CWorkerInfo()
	{
		m_bConnected = false;
		m_nWorkUnitsDone = 0;
		m_JobWorkerID = 0xFFFFFFFF;
		m_RunningTimeMS = 0;
		m_ThreadWUs[0] = m_ThreadWUs[1] = m_ThreadWUs[2] = m_ThreadWUs[3] = -1;
	}
	
	CString m_ComputerName;
	int m_bConnected;
	int m_nWorkUnitsDone;
	unsigned long m_JobWorkerID;
	unsigned long m_RunningTimeMS;
	CString m_CurrentStage;
	int m_ThreadWUs[4];
};



/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg dialog

class CJobWatchDlg : public CIdleDialog
{
// Construction
public:
	CJobWatchDlg( CWnd* pParent = NULL);   // standard constructor
	virtual ~CJobWatchDlg();

// Dialog Data
	//{{AFX_DATA(CJobWatchDlg)
	enum { IDD = IDD_JOB_WATCH };
	CListCtrl	m_Workers;
	CEdit	m_TextOutput;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobWatchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	virtual void OnIdle();
	void RefreshWorkerStats();
	CWorkerInfo* FindWorkerByID( unsigned long jobWorkerID );
	CWorkerInfo* FindWorkerByMachineName( const char *pMachineName );
	void SetWorkerListItemInt( int nIndex, int iColumn, int value );
	void UpdateWorkersList();
	void ResortItems();

	// Query IDs.
	enum
	{
		QUERY_TEXT=0,
		QUERY_GRAPH,
		QUERY_WORKER_STATS,
		NUM_QUERIES
	};	

	void ProcessQueryResults_Graph( IMySQLRowSet *pSet );
	void ProcessQueryResults_Text( IMySQLRowSet *pSet );
	void ProcessQueryResults_WorkerStats( IMySQLRowSet *pSet );

	bool m_bQueriesInProgress[NUM_QUERIES];

	// This is our connection to the mysql database.
	IMySQLAsync *m_pSQL;
	CSysModule *m_hMySQLDLL;

	CWindowAnchorMgr	m_AnchorMgr;


	bool GetCurJobWorkerID( unsigned long &id );

	CGraphControl	m_GraphControl;
	unsigned long	m_JobID;
	int				m_CurGraphTime;

	int				m_CurMessageIndex;
	int				m_CurWorkerTextToken; // used to let it ignore old text in the thread's queue

	DWORD			m_LastQueryTime;	// Last time we made a query.

	// Generated message map functions
	//{{AFX_MSG(CJobWatchDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeWorkers();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOdstatechangedWorkers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedWorkers(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOBWATCHDLG_H__761BDEEF_D549_4F10_817C_1C1FAF9FCA47__INCLUDED_)
