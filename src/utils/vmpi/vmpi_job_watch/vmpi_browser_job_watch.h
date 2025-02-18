//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vmpi_browser_job_watch.h : main header file for the VMPI_BROWSER_JOB_WATCH application
//

#if !defined(AFX_VMPI_BROWSER_JOB_WATCH_H__1DF22047_F615_4799_913A_222E3701BE5E__INCLUDED_)
#define AFX_VMPI_BROWSER_JOB_WATCH_H__1DF22047_F615_4799_913A_222E3701BE5E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVMPIBrowserJobWatchApp:
// See vmpi_browser_job_watch.cpp for the implementation of this class
//

class CVMPIBrowserJobWatchApp : public CWinApp
{
public:
	CVMPIBrowserJobWatchApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVMPIBrowserJobWatchApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVMPIBrowserJobWatchApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VMPI_BROWSER_JOB_WATCH_H__1DF22047_F615_4799_913A_222E3701BE5E__INCLUDED_)
