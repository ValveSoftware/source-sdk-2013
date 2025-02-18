//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined(AFX_JOBSEARCHDLG_H__833A83FF_35CC_42B5_AEB3_AE31C7FDF492__INCLUDED_)
#define AFX_JOBSEARCHDLG_H__833A83FF_35CC_42B5_AEB3_AE31C7FDF492__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// JobSearchDlg.h : header file
//

#include "resource.h"
#include "imysqlwrapper.h"
#include "window_anchor_mgr.h"


/////////////////////////////////////////////////////////////////////////////
// CJobSearchDlg dialog

class CJobSearchDlg : public CDialog
{
// Construction
public:
	CJobSearchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CJobSearchDlg();

// Dialog Data
	//{{AFX_DATA(CJobSearchDlg)
	enum { IDD = IDD_VMPI_JOB_SEARCH };
	CListBox	m_WorkerList;
	CListBox	m_UserList;
	CListCtrl	m_JobsList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJobSearchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void ClearJobsList();
	void RepopulateJobsList();

	void PopulateWorkerList( CUtlVector<char*> &computerNames );
	void PopulateUserList( CUtlVector<char*> &computerNames );

	int GetSelectedJobIndex();

	
	// Info on how we connected to the database so we can pass it to apps we launch.
	CString	m_DBName, m_HostName, m_UserName;
	
	
	IMySQL*	GetMySQL()	{ return m_pSQL; }
	IMySQL	*m_pSQL;
	CSysModule *m_hMySQLDLL;

	CWindowAnchorMgr	m_AnchorMgr;


	// Generated message map functions
	//{{AFX_MSG(CJobSearchDlg)
	afx_msg void OnDblclkJobsList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkUserList();
	afx_msg void OnDblclkWorkerList();
	virtual BOOL OnInitDialog();
	afx_msg void OnQuit();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOBSEARCHDLG_H__833A83FF_35CC_42B5_AEB3_AE31C7FDF492__INCLUDED_)
