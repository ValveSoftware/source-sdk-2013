//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "stdafx.h"
#include "idle_dialog.h"


#define WM_STARTIDLE					(WM_USER + 565)


BEGIN_MESSAGE_MAP(CIdleDialog, CDialog)
	//{{AFX_MSG_MAP(CVMPIBrowserDlg)
	ON_MESSAGE(WM_STARTIDLE, OnStartIdle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CIdleDialog::CIdleDialog( int id, CWnd *pParent )
	: CDialog( id, pParent )
{
}


void CIdleDialog::StartIdleProcessing( DWORD msInterval )
{
	m_cWinIdle.StartIdle( GetSafeHwnd(), WM_STARTIDLE, 0, 0, msInterval );
	m_cWinIdle.NextIdle();
}


LONG CIdleDialog::OnStartIdle( UINT, LONG )
{
	MSG msg;
	
	if ( !PeekMessage( &msg, GetSafeHwnd(), 0,0, PM_NOREMOVE ) )
		OnIdle();
	
	m_cWinIdle.NextIdle();
	return 0;
}


