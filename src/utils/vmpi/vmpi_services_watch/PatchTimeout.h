//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined(AFX_PATCHTIMEOUT_H__2D87CBF2_AC88_4F23_BB43_CC8A5C248B64__INCLUDED_)
#define AFX_PATCHTIMEOUT_H__2D87CBF2_AC88_4F23_BB43_CC8A5C248B64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PatchTimeout.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CPatchTimeout dialog

class CPatchTimeout : public CDialog
{
// Construction
public:
	CPatchTimeout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPatchTimeout)
	enum { IDD = IDD_TIMEOUT };
	CString m_PatchDirectory;
	CString m_VMPITransferDirectory;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPatchTimeout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPatchTimeout)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATCHTIMEOUT_H__2D87CBF2_AC88_4F23_BB43_CC8A5C248B64__INCLUDED_)
