//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// SetPasswordDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SetPasswordDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg dialog


CSetPasswordDlg::CSetPasswordDlg(int dlgID, CWnd* pParent /*=NULL*/)
	: CDialog(dlgID, pParent)
{
	//{{AFX_DATA_INIT(CSetPasswordDlg)
	m_Password = _T("");
	//}}AFX_DATA_INIT
}


void CSetPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetPasswordDlg)
	DDX_Text(pDX, IDC_PASSWORD, m_Password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetPasswordDlg, CDialog)
	//{{AFX_MSG_MAP(CSetPasswordDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetPasswordDlg message handlers
