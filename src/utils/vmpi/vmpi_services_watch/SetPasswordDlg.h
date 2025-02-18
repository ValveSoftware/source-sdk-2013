//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined(AFX_SETPASSWORDDLG_H__A349B943_49B8_4C7E_863B_BF1929AD5443__INCLUDED_)
#define AFX_SETPASSWORDDLG_H__A349B943_49B8_4C7E_863B_BF1929AD5443__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SetPasswordDlg.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg dialog

class CSetPasswordDlg : public CDialog
{
// Construction
public:
	CSetPasswordDlg(int dlgID, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSetPasswordDlg)
	CString	m_Password;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetPasswordDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetPasswordDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETPASSWORDDLG_H__A349B943_49B8_4C7E_863B_BF1929AD5443__INCLUDED_)
