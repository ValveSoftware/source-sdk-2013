//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// MessageWatch.h : main header file for the MESSAGEWATCH application
//

#if !defined(AFX_MESSAGEWATCH_H__72A09EC9_2B19_4AC5_A281_5FAD41F6DFCA__INCLUDED_)
#define AFX_MESSAGEWATCH_H__72A09EC9_2B19_4AC5_A281_5FAD41F6DFCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMessageWatchApp:
// See MessageWatch.cpp for the implementation of this class
//

class CMessageWatchApp : public CWinApp
{
public:
	CMessageWatchApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMessageWatchApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMessageWatchApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MESSAGEWATCH_H__72A09EC9_2B19_4AC5_A281_5FAD41F6DFCA__INCLUDED_)
