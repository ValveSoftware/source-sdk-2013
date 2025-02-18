//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vmpi_browser_services.h : main header file for the VMPI_BROWSER_SERVICES application
//

#if !defined(AFX_VMPI_BROWSER_SERVICES_H__B03E2165_4E70_48AC_A991_EB0289A3471E__INCLUDED_)
#define AFX_VMPI_BROWSER_SERVICES_H__B03E2165_4E70_48AC_A991_EB0289A3471E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVMPIBrowserServicesApp:
// See vmpi_browser_services.cpp for the implementation of this class
//

class CVMPIBrowserServicesApp : public CWinApp
{
public:
	CVMPIBrowserServicesApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVMPIBrowserServicesApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVMPIBrowserServicesApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VMPI_BROWSER_SERVICES_H__B03E2165_4E70_48AC_A991_EB0289A3471E__INCLUDED_)
