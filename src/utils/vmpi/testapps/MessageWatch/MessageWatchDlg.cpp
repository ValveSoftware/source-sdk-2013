//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// MessageWatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MessageWatch.h"
#include "MessageWatchDlg.h"
#include "messagemgr.h"
#include "tier1/strtools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define WM_STARTIDLE	(WM_USER + 565)


// --------------------------------------------------------------------------- //
// CSender.
// --------------------------------------------------------------------------- //

CSender::CSender()
{
	m_pSocket = NULL;
	m_pConsoleWnd = NULL;
}

CSender::~CSender()
{
	if ( m_pSocket )
		m_pSocket->Release();
	
	if ( m_pConsoleWnd )
		m_pConsoleWnd->Release();
}


/////////////////////////////////////////////////////////////////////////////
// CMessageWatchDlg dialog

CMessageWatchDlg::CMessageWatchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMessageWatchDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMessageWatchDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_pListenSocket = NULL;
}

CMessageWatchDlg::~CMessageWatchDlg()
{
	// destroy the sender objects.

	if ( m_pListenSocket )
		m_pListenSocket->Release();
}

void CMessageWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessageWatchDlg)
	DDX_Control(pDX, IDC_MACHINES, m_Machines);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMessageWatchDlg, CDialog)
	//{{AFX_MSG_MAP(CMessageWatchDlg)
	ON_MESSAGE(WM_STARTIDLE, OnStartIdle)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_LBN_DBLCLK(IDC_MACHINES, OnDblclkMachines)
	ON_BN_CLICKED(IDSHOWALL, OnShowall)
	ON_BN_CLICKED(IDHIDEALL, OnHideall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageWatchDlg message handlers

BOOL CMessageWatchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// Setup our listen socket and thread.	
	m_pListenSocket = CreateIPSocket();
	m_pListenSocket->BindToAny( MSGMGR_BROADCAST_PORT );

	m_cWinIdle.StartIdle( GetSafeHwnd(), WM_STARTIDLE, 0, 0, 100 );
	m_cWinIdle.NextIdle();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}


LONG CMessageWatchDlg::OnStartIdle( UINT, LONG )
{
	MSG msg;
	if (!PeekMessage(&msg, GetSafeHwnd(), 0,0, PM_NOREMOVE))
		OnIdle();
	m_cWinIdle.NextIdle();
	return 0;
}


void CMessageWatchDlg::OnIdle()
{
	// Kill dead connections.
	int iNext;
	for ( int iSender=m_Senders.Head(); iSender != m_Senders.InvalidIndex(); iSender = iNext )
	{
		iNext = m_Senders.Next( iSender );

		CSender *pSender = m_Senders[iSender];
		if ( pSender->m_pSocket && !pSender->m_pSocket->IsConnected() )
		{
			// Just release the socket so the text stays there.
			pSender->m_pSocket->Release();
			pSender->m_pSocket = NULL;
		}
	}

	// Look for new connections.
	while ( 1 )
	{
		CIPAddr ipFrom;
		char data[16];
		int len = m_pListenSocket->RecvFrom( data, sizeof( data ), &ipFrom );
		if ( len == -1 )
			break;

		if ( data[0] == MSGMGR_PACKETID_ANNOUNCE_PRESENCE &&
			*((int*)&data[1]) == MSGMGR_VERSION )
		{
			int iPort = *((int*)&data[5]);
		
			// See if we have a machine with this info yet.
			CIPAddr connectAddr = ipFrom;
			connectAddr.port = iPort;

			// NOTE: we'll accept connections from machines we were connected to earlier but
			// lost the connection to.
			CSender *pSender = FindSenderByAddr( ipFrom.ip );
			if ( !pSender || !pSender->m_pSocket )
			{
				// 'nitiate the connection.
				ITCPSocket *pNew = CreateTCPSocket();
				if ( pNew->BindToAny( 0 ) && TCPSocket_Connect( pNew, &connectAddr, 1000 ) )
				{
					char nameStr[256];
					char title[512];
					if ( !ConvertIPAddrToString( &ipFrom, nameStr, sizeof( nameStr ) ) )
						Q_snprintf( nameStr, sizeof( nameStr ), "%d.%d.%d.%d", ipFrom.ip[0], ipFrom.ip[1], ipFrom.ip[2], ipFrom.ip[3] );
					
					Q_snprintf( title, sizeof( title ), "%s:%d", nameStr, iPort );

					// If the sender didn't exist yet, add a new one.
					if ( !pSender )
					{
						pSender = new CSender;
						
						IConsoleWnd *pWnd = CreateConsoleWnd(
							AfxGetInstanceHandle(),
							IDD_OUTPUT,
							IDC_DEBUG_OUTPUT,
							false
							);

						pSender->m_pConsoleWnd = pWnd;
						pWnd->SetTitle( title );
						
						Q_strncpy( pSender->m_Name, title, sizeof( pSender->m_Name ) );
						m_Senders.AddToTail( pSender );
						m_Machines.AddString( pSender->m_Name );
					}

					pSender->m_Addr = connectAddr;
					pSender->m_pSocket = pNew;
				}
				else
				{
					pNew->Release();
				}
			}
		}
	}

	
	// Read input from our current connections.
	FOR_EACH_LL( m_Senders, i )
	{
		CSender *pSender = m_Senders[i];

		while ( 1 )
		{
			if ( !pSender->m_pSocket )
				break;

			CUtlVector<unsigned char> data;
			if ( !pSender->m_pSocket->Recv( data ) )
				break;

			if ( data[0] == MSGMGR_PACKETID_MSG )
			{
				char *pMsg = (char*)&data[1];
				pSender->m_pConsoleWnd->PrintToConsole( pMsg );
				OutputDebugString( pMsg );
			}
		}
	}
}


void CMessageWatchDlg::OnDestroy() 
{
	// Stop the idling thread
	m_cWinIdle.EndIdle();
	CDialog::OnDestroy();
}


CSender* CMessageWatchDlg::FindSenderByAddr( const unsigned char ip[4] )
{
	FOR_EACH_LL( m_Senders, i )
	{
		if ( memcmp( m_Senders[i]->m_Addr.ip, ip, 4 ) == 0 )
			return m_Senders[i];
	}
	return NULL;
}


CSender* CMessageWatchDlg::FindSenderByName( const char *pName )
{
	FOR_EACH_LL( m_Senders, i )
	{
		if ( stricmp( pName, m_Senders[i]->m_Name ) == 0 )
			return m_Senders[i];
	}
	return NULL;
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMessageWatchDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMessageWatchDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMessageWatchDlg::OnDblclkMachines() 
{
	int index = m_Machines.GetCurSel();
	if ( index != LB_ERR )
	{
		CString str;
		m_Machines.GetText( index, str );

		CSender *pSender = FindSenderByName( str );
		if ( pSender )
			pSender->m_pConsoleWnd->SetVisible( true );
	}	
}

void CMessageWatchDlg::OnShowall() 
{
	FOR_EACH_LL( m_Senders, i )
	{
		m_Senders[i]->m_pConsoleWnd->SetVisible( true );
	}	
}

void CMessageWatchDlg::OnHideall() 
{
	FOR_EACH_LL( m_Senders, i )
	{
		m_Senders[i]->m_pConsoleWnd->SetVisible( false );
	}	
}
