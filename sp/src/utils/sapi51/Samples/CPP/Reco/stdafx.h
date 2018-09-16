// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif WIN32_LEAN_AND_MEAN


// Windows Header Files:
#include <windows.h>
#include <commdlg.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// ATL Header Files
#include <atlbase.h>

// Multi-Language Header File
#include <mlang.h>

// RichEdit 2.0
#include <richedit.h>

#ifdef _WIN32_WCE
#include<WinCEStub.h>
#define ICON_BIG            1
#endif

// SAPI Header Files
#include <sphelper.h>
#include <spddkhlp.h>

// Local Header Files
#include "reco.h"
#include "resource.h"


#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
