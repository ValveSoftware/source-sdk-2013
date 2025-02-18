//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// vmpi_browser_services.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "vmpi_browser_services.h"
#include "servicesdlg.h"
#include "curl/curl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVMPIBrowserServicesApp

BEGIN_MESSAGE_MAP(CVMPIBrowserServicesApp, CWinApp)
	//{{AFX_MSG_MAP(CVMPIBrowserServicesApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVMPIBrowserServicesApp construction

CVMPIBrowserServicesApp::CVMPIBrowserServicesApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVMPIBrowserServicesApp object

CVMPIBrowserServicesApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVMPIBrowserServicesApp initialization

BOOL CVMPIBrowserServicesApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

								// Init libcurl
	if ( curl_global_init( CURL_GLOBAL_DEFAULT ) != 0 )
	{
		Warning( "Failed to initialize curl library!\n" );
	}

	CServicesDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	curl_global_cleanup();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
