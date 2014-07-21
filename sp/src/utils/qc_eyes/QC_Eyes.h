//========= Copyright Valve Corporation, All rights reserved. ============//
// QC_Eyes.h : main header file for the QC_EYES application
//

#if !defined(AFX_QC_EYES_H__398BAF8D_D3C0_4326_BEF0_5129884EE1A3__INCLUDED_)
#define AFX_QC_EYES_H__398BAF8D_D3C0_4326_BEF0_5129884EE1A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CQC_EyesApp:
// See QC_Eyes.cpp for the implementation of this class
//

class CQC_EyesApp : public CWinApp
{
public:
	CQC_EyesApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQC_EyesApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CQC_EyesApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QC_EYES_H__398BAF8D_D3C0_4326_BEF0_5129884EE1A3__INCLUDED_)
