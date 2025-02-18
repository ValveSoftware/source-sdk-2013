//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// PatchTimeout.cpp : implementation file
//

#include "stdafx.h"
#include "PatchTimeout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchTimeout dialog


CPatchTimeout::CPatchTimeout(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchTimeout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatchTimeout)
	//}}AFX_DATA_INIT
}


void CPatchTimeout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchTimeout)
	DDX_Text(pDX, IDC_COMMAND_LINE, m_PatchDirectory);
	DDX_Text(pDX, IDC_VMPI_TRANSFER_DIRECTORY, m_VMPITransferDirectory);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatchTimeout, CDialog)
	//{{AFX_MSG_MAP(CPatchTimeout)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchTimeout message handlers
