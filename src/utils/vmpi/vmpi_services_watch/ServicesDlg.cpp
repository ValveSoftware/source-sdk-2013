//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// ServicesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ServicesDlg.h"
#include "vmpi.h"
#include "bitbuf.h"
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "patchtimeout.h"
#include "SetPasswordDlg.h"
#include "vmpi_browser_helpers.h"
#include <io.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SERVICE_OFF_TIMEOUT					(20*1000)	// If we haven't heard from a service in this long,
														// then we assume the service is off.

#define SERVICES_PING_INTERVAL				(3*1000)	// ping the services every so often

#define SERVICE_MAX_UPDATE_INTERVAL			(8*1000)	// Update each service in the listbox at least this often.



int V_AfxMessageBox( int mbType, const char *pFormat, ... )
{
	char msg[4096];
	va_list marker;
	va_start( marker, pFormat );
	_vsnprintf( msg, sizeof( msg ), pFormat, marker );
	va_end( marker );
	
	return AfxMessageBox( msg, mbType );
}


bool CServiceInfo::IsOff() const
{
	return (Plat_MSTime() - m_LastPingTimeMS) > SERVICE_OFF_TIMEOUT;
}


// Returns the argument following pName.
// If pName is the last argument on the command line, returns pEndArgDefault.
// Returns NULL if there is no argument with pName.
const char* FindArg( const char *pName, const char *pEndArgDefault="" )
{
	for ( int i=0; i < __argc; i++ )
	{
		if ( stricmp( pName, __argv[i] ) == 0 )
		{
			if ( (i+1) < __argc )
				return __argv[i+1];
			else
				return pEndArgDefault;
		}
	}
	return NULL;
}

void AppendStr( char *dest, int destSize, const char *pFormat, ... )
{
	char str[4096];
	va_list marker;
	va_start( marker, pFormat );
	_vsnprintf( str, sizeof( str ), pFormat, marker );
	va_end( marker );
	str[sizeof( str ) - 1] = 0;
	
	V_strncat( dest, str, destSize );
}	


char* GetLastErrorString()
{
	static char err[2048];
	
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	strncpy( err, (char*)lpMsgBuf, sizeof( err ) );
	LocalFree( lpMsgBuf );

	err[ sizeof( err ) - 1 ] = 0;

	return err;
}

const char* GetStatusString( CServiceInfo *pInfo )
{
	if ( pInfo->IsOff() )
		return  "off";
	else if ( pInfo->m_iState == VMPI_STATE_BUSY )
		return  "busy";
	else if ( pInfo->m_iState == VMPI_STATE_PATCHING )
		return "patching";
	else if ( pInfo->m_iState == VMPI_STATE_DISABLED )
		return "disabled";
	else if ( pInfo->m_iState == VMPI_STATE_SCREENSAVER_DISABLED )
		return "disabled (screensaver)";
	else if ( pInfo->m_iState == VMPI_STATE_DOWNLOADING )
		return "downloading";
	else
		return "idle";
}


// --------------------------------------------------------------------------------------------------------- //
// Column sort functions.
// --------------------------------------------------------------------------------------------------------- //
typedef int (CALLBACK *ServicesSortFn)( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam );

static int CALLBACK SortByName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return strcmp( pInfo1->m_ComputerName, pInfo2->m_ComputerName );
}

static int CALLBACK SortByStatus( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( GetStatusString( pInfo2 ), GetStatusString( pInfo1 ) );
}

static int CALLBACK SortByRunningTime( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	int v1 = pInfo1->m_LiveTimeMS / (1000*60);	// Sort on minutes so it doesn't constantly resort the list.
	int v2 = pInfo2->m_LiveTimeMS / (1000*60);
	if ( v2 > v1 )
		return 1;
	else if ( v2 < v1 )
		return -1;
	else
		return 0;
}

static int CALLBACK SortByWorkerAppRunningTime( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	int v1 = pInfo1->m_WorkerAppTimeMS / (1000*60); // Sort on minutes so it doesn't constantly resort the list.
	int v2 = pInfo2->m_WorkerAppTimeMS / (1000*60);
	if ( v2 > v1 )
		return 1;
	else if ( v2 < v1 )
		return -1;
	else
		return 0;
}

static int CALLBACK SortByMasterName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( pInfo2->m_MasterName, pInfo1->m_MasterName );
}

static int CALLBACK SortByProtocol( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return pInfo1->m_ProtocolVersion > pInfo2->m_ProtocolVersion;
}

static int CALLBACK SortByPassword( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( pInfo2->m_Password, pInfo1->m_Password );
}

static int CALLBACK SortByServiceVersion( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( pInfo2->m_ServiceVersion, pInfo1->m_ServiceVersion );
}

static int CALLBACK SortByExe( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( pInfo2->m_ExeName, pInfo1->m_ExeName );
}

static int CALLBACK SortByMap( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	return -strcmp( pInfo2->m_MapName, pInfo1->m_MapName );
}

static int CALLBACK SortByCPUPercentage( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	if ( pInfo2->m_CPUPercentage > pInfo1->m_CPUPercentage )
		return 1;
	else if ( pInfo2->m_CPUPercentage < pInfo1->m_CPUPercentage )
		return -1;														   
	else
		return 0;
}
				
static int CALLBACK SortByMemUsage( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CServiceInfo *pInfo1 = (CServiceInfo*)iItem1;
	CServiceInfo *pInfo2 = (CServiceInfo*)iItem2;

	if ( pInfo2->m_MemUsageMB > pInfo1->m_MemUsageMB )
		return 1;
	else if ( pInfo2->m_MemUsageMB < pInfo1->m_MemUsageMB )
		return -1;														   
	else
		return 0;
}
				
// --------------------------------------------------------------------------------------------------------- //
// Column information.
// --------------------------------------------------------------------------------------------------------- //

struct
{
	char			*pText;
	int				width;
	ServicesSortFn	sortFn;
} g_ColumnInfos[] =
{
	{"Computer Name", 150, SortByName},
	{"Status", 95, SortByStatus},
	{"Service Run Time", 100, SortByRunningTime},
	{"Worker App Run Time", 125, SortByWorkerAppRunningTime},
	{"Master", 150, SortByMasterName},
	{"Password", 60, SortByPassword},
	{"Protocol", 60, SortByProtocol},
	{"Version", 50, SortByServiceVersion},
	{"CPU", 50, SortByCPUPercentage},
	{"Memory (MB)", 80, SortByMemUsage},
	{"Exe", 80, SortByExe},
	{"Map", 90, SortByMap},
};
#define COLUMN_COMPUTER_NAME				0
#define COLUMN_STATUS						1
#define COLUMN_RUNNING_TIME					2
#define COLUMN_WORKER_APP_RUNNING_TIME		3
#define COLUMN_MASTER_NAME					4
#define COLUMN_PASSWORD						5
#define COLUMN_PROTOCOL_VERSION				6						 
#define COLUMN_SERVICE_VERSION				7
#define COLUMN_CPU_PERCENTAGE				8
#define COLUMN_MEM_USAGE					9
#define COLUMN_EXE_NAME						10
#define COLUMN_MAP_NAME						11


// Used to sort all the columns. 
// When they click on a column, we add that index to entry 0 in here.
// Then when sorting, if 2 columns are equal, we move to the next sort function.
CUtlVector<int> g_SortColumns;	

static void PushSortColumn( int iColumn )
{
	// First, get rid of all entries with this same index.
	int iPrev = g_SortColumns.Find( iColumn );
	if ( iPrev != g_SortColumns.InvalidIndex() )
		g_SortColumns.Remove( iPrev );
	
	// Now add this one.
	g_SortColumns.AddToTail( iColumn );
}	

static int CALLBACK MainSortFn( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	int curStatus = 0;
	for ( int i = (int)g_SortColumns.Count()-1; i >= 0; i-- )
	{
		curStatus = g_ColumnInfos[g_SortColumns[i]].sortFn( iItem1, iItem2, lpParam );
		if ( curStatus != 0 )
			break;
	}
	return curStatus;
}
																		 

/////////////////////////////////////////////////////////////////////////////
// CServicesDlg dialog


CServicesDlg::CServicesDlg(CWnd* pParent /*=NULL*/)
	: CIdleDialog(CServicesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServicesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pServicesPingSocket = NULL;
}


CServicesDlg::~CServicesDlg()
{
	m_Services.PurgeAndDeleteElements();
}


void CServicesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicesDlg)
	DDX_Control(pDX, IDC_NUM_SERVICES, m_NumServicesControl);
	DDX_Control(pDX, IDC_NUM_DISABLED_SERVICES, m_NumDisabledServicesControl);
	DDX_Control(pDX, IDC_NUM_WORKING_SERVICES, m_NumWorkingServicesControl);
	DDX_Control(pDX, IDC_NUM_WAITING_SERVICES, m_NumWaitingServicesControl);
	DDX_Control(pDX, IDC_NUM_OFF_SERVICES, m_NumOffServicesControl);
	DDX_Control(pDX, IDC_CURRENT_PASSWORD, m_PasswordDisplay);
	DDX_Control(pDX, IDC_SERVICES_LIST, m_ServicesList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServicesDlg, CIdleDialog)
	//{{AFX_MSG_MAP(CServicesDlg)
	ON_BN_CLICKED(ID_PATCH_SERVICES, OnPatchServices)
	ON_BN_CLICKED(ID_STOP_SERVICES, OnStopServices)
	ON_BN_CLICKED(ID_STOP_JOBS, OnStopJobs)
	ON_BN_CLICKED(ID_FILTER_BY_PASSWORD, OnFilterByPassword)
	ON_BN_CLICKED(ID_FORCE_PASSWORD, OnForcePassword)
	ON_BN_CLICKED(ID_COPY_TO_CLIPBOARD, OnCopyToClipboard)
	ON_NOTIFY(NM_DBLCLK, IDC_SERVICES_LIST, OnDblclkServicesList)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED( ID_ENABLE_MACHINES, &CServicesDlg::OnBnClickedEnableMachines )
	ON_BN_CLICKED( ID_REQUEST_LOG, &CServicesDlg::OnBnClickedRequestLog )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServicesDlg message handlers

BOOL CServicesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Initially, just sort by computer name.
	PushSortColumn( COLUMN_COMPUTER_NAME );

	HICON hIcon = LoadIcon( AfxGetInstanceHandle(), MAKEINTRESOURCE( IDR_MAINFRAME ) );
	SetIcon( hIcon, true );

	m_ServicesList.SetExtendedStyle( LVS_EX_FULLROWSELECT );
	
	// Setup the headers.
	for ( int i=0; i < ARRAYSIZE( g_ColumnInfos ); i++ )
	{
		m_ServicesList.InsertColumn( i, g_ColumnInfos[i].pText, LVCFMT_LEFT, g_ColumnInfos[i].width, i );
	}
	
	m_pServicesPingSocket = CreateIPSocket();
	if ( m_pServicesPingSocket )
	{
		m_pServicesPingSocket->BindToAny( 0 );
	}

	m_dwLastServicesPing = GetTickCount() - SERVICES_PING_INTERVAL;
	StartIdleProcessing( 100 );	// get idle messages every half second

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_DISABLED_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_DISABLED_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_WORKING_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_WORKING_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_WAITING_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_WAITING_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_OFF_SERVICES_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_NUM_OFF_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_CURRENT_PASSWORD_LABEL ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_CURRENT_PASSWORD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_PATCH_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_STOP_SERVICES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_STOP_JOBS ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_FORCE_PASSWORD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_ENABLE_MACHINES ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_REQUEST_LOG ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_FILTER_BY_PASSWORD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( ID_COPY_TO_CLIPBOARD ), ANCHOR_LEFT, ANCHOR_BOTTOM, ANCHOR_LEFT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_SERVICES_LIST ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_BOTTOM );

	// Unless they specify admin mode, hide all the controls that can mess with the services.
	if ( !FindArg( "-Admin" ) )
	{
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_PATCH_SERVICES ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_STOP_SERVICES ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_STOP_JOBS ), SW_HIDE );
		::ShowWindow( ::GetDlgItem( m_hWnd, ID_FORCE_PASSWORD ), SW_HIDE );
	}

	m_NetViewThread.Init();
	m_VMPIRegistryQueryThread.Init();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CServicesDlg::BuildVMPIPingPacket( CUtlVector<char> &out, char cPacketID, unsigned char protocolVersion, bool bIgnorePassword )
{
	out.Purge();
	out.AddToTail( protocolVersion );
	
	const char *pPassword = m_Password;
	if ( pPassword[0] == 0 || bIgnorePassword )
	{
		// If they haven't set a password to filter by, then we want all services to report in.
		out.AddToTail( VMPI_PASSWORD_OVERRIDE );
		out.AddToTail( 0 );
	}
	else
	{
		out.AddMultipleToTail( strlen( pPassword ) + 1, pPassword );	// password.
	}
	
	out.AddToTail( cPacketID );
}


void CServicesDlg::UpdateServicesFromNetMessages()
{
	while ( 1 )
	{
		char in[1024];
		CIPAddr ipFrom;

		int len = m_pServicesPingSocket->RecvFrom( in, sizeof( in ), &ipFrom );
		if ( len < 4 )
			break;

		bf_read buf( in, len );
		unsigned char protocolVersion = buf.ReadByte();
		if ( protocolVersion == 4 || protocolVersion == VMPI_PROTOCOL_VERSION ) // Protocol version 4 is almost the same.
		{
			int packetID = buf.ReadByte();
			if ( packetID == VMPI_PING_RESPONSE )
			{
				int iState = buf.ReadByte();
				unsigned long liveTimeMS = (unsigned long)buf.ReadLong();

				int iPort = buf.ReadLong();
				char computerName[512];
				buf.ReadString( computerName, sizeof( computerName ) );

				char masterName[512];
				if ( buf.GetNumBitsLeft() )
					buf.ReadString( masterName, sizeof( masterName ) );
				else
					masterName[0] = 0;

				unsigned long workerAppTimeMS = 0;
				if ( buf.GetNumBitsLeft() )
					workerAppTimeMS = buf.ReadLong();

				char password[512] = {0};
				if ( protocolVersion == VMPI_PROTOCOL_VERSION )
					buf.ReadString( password, sizeof( password ) );

				char serviceVersion[32];
				if ( protocolVersion == VMPI_PROTOCOL_VERSION )
					buf.ReadString( serviceVersion, sizeof( serviceVersion ) );
				else
					V_strncpy( serviceVersion, "old", sizeof( serviceVersion ) );

				int cpuPercentage = -1;
				if ( buf.GetNumBytesLeft() >= 1 )
					cpuPercentage = buf.ReadByte();
				
				char exeName[512];
				if ( buf.GetNumBytesLeft() >= 1 )
					buf.ReadString( exeName, sizeof( exeName ) );
				else
					V_strncpy( exeName, "-", sizeof( exeName ) );

				short memUsage = -1;
				if ( buf.GetNumBytesLeft() >= 2 )
					memUsage = buf.ReadShort();

				char mapName[256];
				if ( buf.GetNumBytesLeft() >= 1 )
					buf.ReadString( mapName, sizeof( mapName ) );
				else
					V_strncpy( mapName, "-", sizeof( mapName ) );
										
	
				CServiceInfo *pInfo = FindServiceByComputerName( computerName );
				if ( !pInfo )
				{
					pInfo = new CServiceInfo;
					m_Services.AddToTail( pInfo );
					pInfo->m_ComputerName = computerName;

					pInfo->m_pLastStatusText = NULL;
					
					pInfo->m_Addr.port = iPort;
					pInfo->m_LastPingTimeMS = Plat_MSTime();
					pInfo->m_MasterName = "?";

					int iItem = m_ServicesList.InsertItem( COLUMN_COMPUTER_NAME, pInfo->m_ComputerName, NULL );
					m_ServicesList.SetItemData( iItem, (DWORD)pInfo );

					// Update the display of # of services.
					UpdateServiceCountDisplay();
				}
				
				pInfo->m_ProtocolVersion = protocolVersion;
				V_strncpy( pInfo->m_ServiceVersion, serviceVersion, sizeof( pInfo->m_ServiceVersion ) );
				pInfo->m_Addr = ipFrom;
				pInfo->m_CPUPercentage = cpuPercentage;
				pInfo->m_MemUsageMB = memUsage;
				pInfo->m_ExeName = exeName;
				pInfo->m_MapName = mapName;
				pInfo->m_MasterName = masterName;
				pInfo->m_LiveTimeMS = liveTimeMS;
				pInfo->m_WorkerAppTimeMS = workerAppTimeMS;
				pInfo->m_iState = iState;
				pInfo->m_LastPingTimeMS = Plat_MSTime();
				pInfo->m_Password = password;

				UpdateServiceInListbox( pInfo );
			}
		}
	}
}

void CServicesDlg::UpdateServicesFromVMPIRegistry()
{
	CUtlVector<CServiceInfo> services;
	m_VMPIRegistryQueryThread.GetRegisteredServices( services );

	if ( services.Count() > 0 )
	{
		// Remember the item that was in the middle so we can restore it after we insert all the computer names.
		CServiceInfo *pRestoreItem = NULL;
		int nRestoreIndex = m_ServicesList.GetTopIndex() + m_ServicesList.GetCountPerPage() - 1;
		if ( nRestoreIndex >= 0 && nRestoreIndex < m_ServicesList.GetItemCount() )
		{
			pRestoreItem = (CServiceInfo*)m_ServicesList.GetItemData( nRestoreIndex );
		}

		for ( int i = 0; i < services.Count(); i++ )
		{
			CServiceInfo *pInfo = FindServiceByComputerName( services[ i ].m_ComputerName );
			if ( !pInfo )
			{
				pInfo = new CServiceInfo;
				m_Services.AddToTail( pInfo );
				*pInfo = services[ i ];

				int iItem = m_ServicesList.InsertItem( COLUMN_COMPUTER_NAME, pInfo->m_ComputerName, NULL );
				m_ServicesList.SetItemData( iItem, (DWORD_PTR)pInfo );

				// Update the display of # of services.
				UpdateServiceCountDisplay();
			}
			else
			{
				pInfo->m_Addr = services[ i ].m_Addr;	// Make sure we have the most current address
			}

			UpdateServiceInListbox( pInfo );
		}

		// Make the previous middle item item visible.
		if ( pRestoreItem )
		{
			LVFINDINFO info;
			info.flags = LVFI_PARAM;
			info.lParam = (LPARAM)pRestoreItem;
			int nItem = m_ServicesList.FindItem( &info );
			m_ServicesList.EnsureVisible( nItem, true );
		}
	}
}


void CServicesDlg::UpdateServicesFromNetView()
{
    CUtlVector<char*> computerNames;
	m_NetViewThread.GetComputerNames( computerNames, true );

	if ( computerNames.Count() > 0 )
	{
		// Remember the item that was in the middle so we can restore it after we insert all the computer names.
		CServiceInfo *pRestoreItem = NULL;
		int nRestoreIndex = m_ServicesList.GetTopIndex() + m_ServicesList.GetCountPerPage() - 1;
		if ( nRestoreIndex >= 0 && nRestoreIndex < m_ServicesList.GetItemCount() )
		{
			pRestoreItem = (CServiceInfo*)m_ServicesList.GetItemData( nRestoreIndex );
		}

		for ( int i=0; i < computerNames.Count(); i++ )
		{
			CServiceInfo *pInfo = FindServiceByComputerName( computerNames[i] );
			if ( !pInfo )
			{
				pInfo = new CServiceInfo;
				m_Services.AddToTail( pInfo );
				pInfo->m_ComputerName = computerNames[i];
				pInfo->m_LastPingTimeMS = Plat_MSTime() - SERVICE_OFF_TIMEOUT*2; // so it's marked as "off"
				pInfo->m_LastLiveTimeMS = Plat_MSTime();
				pInfo->m_LiveTimeMS = 0;
				pInfo->m_WorkerAppTimeMS = 0;
				pInfo->m_ProtocolVersion = 0;
				pInfo->m_ServiceVersion[0] = 0;
				pInfo->m_CPUPercentage = -1;
				pInfo->m_MemUsageMB = -1;

				int iItem = m_ServicesList.InsertItem( COLUMN_COMPUTER_NAME, pInfo->m_ComputerName, NULL );
				m_ServicesList.SetItemData( iItem, (DWORD)pInfo );

				// Update the display of # of services.
				UpdateServiceCountDisplay();
			
				UpdateServiceInListbox( pInfo );
			}
		}

		// Make the previous middle item item visible.
		if ( pRestoreItem )
		{
			LVFINDINFO info;
			info.flags = LVFI_PARAM;
			info.lParam = (LPARAM)pRestoreItem;
			int nItem = m_ServicesList.FindItem( &info );
			m_ServicesList.EnsureVisible( nItem, true );
		}

		computerNames.PurgeAndDeleteElements();
	}
}


void CServicesDlg::OnIdle()
{
	DWORD curTime = GetTickCount();

	if ( !m_pServicesPingSocket )
		return;

	// Broadcast out to all the services?
	if ( curTime - m_dwLastServicesPing >= SERVICES_PING_INTERVAL )
	{
		m_dwLastServicesPing = curTime;
	
		for ( int i=VMPI_SERVICE_PORT; i <= VMPI_LAST_SERVICE_PORT; i++ )
		{
			CUtlVector<char> data;
			BuildVMPIPingPacket( data, VMPI_PING_REQUEST );
			m_pServicesPingSocket->Broadcast( data.Base(), data.Count(), i );

			// Also send out a version 4 one because we understand a version 4 response.
			BuildVMPIPingPacket( data, VMPI_PING_REQUEST, 4 );
			m_pServicesPingSocket->Broadcast( data.Base(), data.Count(), i );
		}
		
		// Try to get a ping response directly from machines we discovered through the registry
		CUtlVector<char> data;
		BuildVMPIPingPacket( data, VMPI_PING_REQUEST );
		FOR_EACH_LL( m_Services, iService )
		{
			CServiceInfo *pInfo = m_Services[ iService ];
			if ( pInfo->m_bFromRegistry )
			{
				m_pServicesPingSocket->SendTo( &pInfo->m_Addr, data.Base(), data.Count() );
			}
		}

		UpdateServiceCountDisplay();
	}

	m_ServicesList.SetRedraw( false );

	// Check for messages from services.
	UpdateServicesFromVMPIRegistry();
	UpdateServicesFromNetMessages();

	// Issue a "net view" command, parse the output, and add any computers it lists.
	// This lets us figure out which PCs on the network are not running the service.
	UpdateServicesFromNetView();


	FOR_EACH_LL( m_Services, iService )
	{
		CServiceInfo *pInfo = m_Services[iService];
		if ( Plat_MSTime() - pInfo->m_LastUpdateTime > SERVICE_MAX_UPDATE_INTERVAL )
		{
			UpdateServiceInListbox( pInfo );
		}
	}

	if ( m_bListChanged )
	{
		ResortItems();
		m_bListChanged = false;
	}

	m_ServicesList.SetRedraw( true );
}


CServiceInfo* CServicesDlg::FindServiceByComputerName( const char *pComputerName )
{
	FOR_EACH_LL( m_Services, i )
	{
		if ( Q_stricmp( m_Services[i]->m_ComputerName, pComputerName ) == 0 )
			return m_Services[i];
	}
	return NULL;
}


void CServicesDlg::SendToSelectedServices( const char *pData, int len )
{
	POSITION pos = m_ServicesList.GetFirstSelectedItemPosition();
	while ( pos )
	{
		int iItem = m_ServicesList.GetNextSelectedItem( pos );

		CServiceInfo *pInfo = (CServiceInfo*)m_ServicesList.GetItemData( iItem );
		m_pServicesPingSocket->SendTo( &pInfo->m_Addr, pData, len );
	}
}


void UpdateItemText( CListCtrl &ctrl, int iItem, int iColumn, const char *pNewVal )
{
	CString str = ctrl.GetItemText( iItem, iColumn );
	if ( V_stricmp( str, pNewVal ) != 0 )
		ctrl.SetItemText( iItem, iColumn, pNewVal );
}


void CServicesDlg::UpdateServiceInListbox( CServiceInfo *pInfo )
{
	// First, find this item in the listbox.
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)pInfo;
	int iItem = m_ServicesList.FindItem( &info );
	if ( iItem != -1 )
	{
		UpdateItemText( m_ServicesList, iItem, COLUMN_COMPUTER_NAME, pInfo->m_ComputerName );
		
		const char *pText = GetStatusString( pInfo );
		UpdateItemText( m_ServicesList, iItem, COLUMN_STATUS, pText );

		char timeStr[512];
		FormatTimeString( pInfo->m_LiveTimeMS / 1000, timeStr, sizeof( timeStr ) );
		UpdateItemText( m_ServicesList, iItem, COLUMN_RUNNING_TIME, timeStr );

		FormatTimeString( pInfo->m_WorkerAppTimeMS / 1000, timeStr, sizeof( timeStr ) );
		UpdateItemText( m_ServicesList, iItem, COLUMN_WORKER_APP_RUNNING_TIME, timeStr );

		UpdateItemText( m_ServicesList, iItem, COLUMN_MASTER_NAME, pInfo->m_MasterName );
		
		char str[512];
		V_snprintf( str, sizeof( str ), "%d", pInfo->m_ProtocolVersion );
		UpdateItemText( m_ServicesList, iItem, COLUMN_PROTOCOL_VERSION, str );

		UpdateItemText( m_ServicesList, iItem, COLUMN_PASSWORD, pInfo->m_Password );
		UpdateItemText( m_ServicesList, iItem, COLUMN_SERVICE_VERSION, pInfo->m_ServiceVersion );

		if ( pInfo->m_CPUPercentage == -1 )
			V_snprintf( str, sizeof( str ), "-" );
		else if ( pInfo->m_CPUPercentage == 101 )
			V_snprintf( str, sizeof( str ), "(err)" );
		else
			V_snprintf( str, sizeof( str ), "%d%%", pInfo->m_CPUPercentage );
		UpdateItemText( m_ServicesList, iItem, COLUMN_CPU_PERCENTAGE, str );

		UpdateItemText( m_ServicesList, iItem, COLUMN_EXE_NAME, pInfo->m_ExeName );
		UpdateItemText( m_ServicesList, iItem, COLUMN_MAP_NAME, pInfo->m_MapName );

		if ( pInfo->m_MemUsageMB == -1 )
			V_snprintf( str, sizeof( str ), "-" );
		else
			V_snprintf( str, sizeof( str ), "%d", pInfo->m_MemUsageMB );
		UpdateItemText( m_ServicesList, iItem, COLUMN_MEM_USAGE, str );

		pInfo->m_pLastStatusText = pText;
		pInfo->m_LastLiveTimeMS = pInfo->m_LiveTimeMS;
		pInfo->m_LastMasterName = pInfo->m_MasterName;
		pInfo->m_LastUpdateTime = Plat_MSTime();

		// Detect changes.
		if ( !m_bListChanged && iItem > 0 )
		{
			CServiceInfo *pPrevItem = (CServiceInfo*)m_ServicesList.GetItemData( iItem-1 );
			if ( pPrevItem && MainSortFn( (LPARAM)pPrevItem, (LPARAM)pInfo, NULL ) > 0 )
				m_bListChanged = true;
		}
			
		if ( !m_bListChanged && (iItem+1) < m_ServicesList.GetItemCount() )
		{
			CServiceInfo *pNextItem = (CServiceInfo*)m_ServicesList.GetItemData( iItem+1 );
			if ( pNextItem && MainSortFn( (LPARAM)pInfo, (LPARAM)pNextItem, NULL ) > 0 )
				m_bListChanged = true;
		}
	}
}


void CServicesDlg::ResortItems()
{
	m_ServicesList.SortItems( MainSortFn, (LPARAM)this );
}


void CServicesDlg::UpdateServiceCountDisplay()
{
	char str[512];
	Q_snprintf( str, sizeof( str ), "%d", m_Services.Count() );
	m_NumServicesControl.SetWindowText( str );

	// Now count the various types.
	int nDisabled = 0, nWorking = 0, nWaiting = 0, nOff = 0;
	FOR_EACH_LL( m_Services, i )
	{
		if ( m_Services[i]->IsOff() )
		{
			++nOff;
		}
		else if ( m_Services[i]->m_iState == VMPI_STATE_BUSY || m_Services[i]->m_iState == VMPI_STATE_DOWNLOADING )
		{
			++nWorking;
		}
		else if ( m_Services[i]->m_iState == VMPI_STATE_IDLE )
		{
			++nWaiting;
		}
		else
		{
			++nDisabled;
		}
	}

	Q_snprintf( str, sizeof( str ), "%d", nDisabled );
	m_NumDisabledServicesControl.SetWindowText( str );

	Q_snprintf( str, sizeof( str ), "%d", nWorking );
	m_NumWorkingServicesControl.SetWindowText( str );

	Q_snprintf( str, sizeof( str ), "%d", nWaiting );
	m_NumWaitingServicesControl.SetWindowText( str );

	Q_snprintf( str, sizeof( str ), "%d", nOff );
	m_NumOffServicesControl.SetWindowText( str );
}


// This monstrosity is here because of the way they bundle string resources into groups in an exe file.
// See http://support.microsoft.com/kb/q196774/. 
bool FindStringResourceEx( HINSTANCE hinst, UINT uId, UINT langId, char *pStr, int outLen )
{
	// Convert the string ID into a bundle number
	bool bRet = false;
	HRSRC hrsrc = FindResourceEx(hinst, RT_STRING, MAKEINTRESOURCE(uId / 16 + 1), langId);
	if (hrsrc) 
	{
		HGLOBAL hglob = LoadResource(hinst, hrsrc);
		if (hglob) 
		{
			LPCWSTR pwsz = reinterpret_cast<LPCWSTR>( LockResource(hglob) );
			if (pwsz) 
			{
				// okay now walk the string table
				for (UINT i = 0; i < (uId & 15); i++) 
				{
					pwsz += 1 + (UINT)*pwsz;
				}
				
				// First word in the resource is the length and the rest is the data.
				int nChars = min( (int)pwsz[0], outLen-1 );
				++pwsz;
				V_wcstostr( pwsz, nChars, pStr, outLen );
				pStr[nChars] = 0;
				bRet = true;
			}
			FreeResource(hglob);
		}
	}
	return bRet;
}


int CheckServiceVersion( const char *pPatchDir, char *pServiceVersion, int maxServiceVersionLen )
{
	char filename[MAX_PATH];
	V_ComposeFileName( pPatchDir, "vmpi_service.exe", filename, sizeof( filename ) );
	
	int ret = IDCANCEL;
	HINSTANCE hInst = LoadLibrary( filename );
	if ( hInst )
	{
		bool bFound = FindStringResourceEx( hInst, VMPI_SERVICE_IDS_VERSION_STRING, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pServiceVersion, maxServiceVersionLen );
		if ( bFound )
		{
			ret = V_AfxMessageBox( MB_YESNOCANCEL, "Service version in %s is %s.\n\nIs this correct?", filename, pServiceVersion );
		}
		else
		{
			V_AfxMessageBox( MB_OK, "Can't get IDS_VERSION_STRING resource from %s.", filename );
		}
		
		FreeLibrary( hInst );
	}
	else
	{
		V_AfxMessageBox( MB_OK, "Can't load %s to get service version.", filename );
	}
	
	return ret;
}

void CServicesDlg::OnPatchServices() 
{
	// Inquire about the timeout.
	CPatchTimeout dlg;
	dlg.m_PatchDirectory = "\\\\fileserver\\vmpi\\testservice";
	dlg.m_VMPITransferDirectory = dlg.m_PatchDirectory;

TryAgain:;

	if ( dlg.DoModal() == IDOK )
	{
		// Launch the transfer app.
		char commandLine[32 * 1024] = {0};
		
		char transferExe[MAX_PATH];
		V_ComposeFileName( dlg.m_VMPITransferDirectory, "vmpi_transfer.exe", transferExe, sizeof( transferExe ) );
		if ( _access( transferExe, 0 ) != 0 )
		{
			V_AfxMessageBox( MB_OK, "Can't find '%s' to run the patch.", transferExe );
			goto TryAgain;
		}

		char strServiceVersion[64];
		int ret = CheckServiceVersion( dlg.m_PatchDirectory, strServiceVersion, sizeof( strServiceVersion ) );
		if ( ret == IDCANCEL )
			return;
		else if ( ret == IDNO )
			goto TryAgain;

		AppendStr( commandLine, sizeof( commandLine ), "\"%s\" -PatchHost", transferExe );
		AppendStr( commandLine, sizeof( commandLine ), " -mpi_PatchVersion %s", strServiceVersion );
		AppendStr( commandLine, sizeof( commandLine ), " -mpi_PatchDirectory \"%s\"", (const char*)dlg.m_PatchDirectory );
													
		// Collect the list of addresses.
		CUtlVector<CIPAddr> addrs;
		POSITION pos = m_ServicesList.GetFirstSelectedItemPosition();
		while ( pos )
		{
			int iItem = m_ServicesList.GetNextSelectedItem( pos );

			CServiceInfo *pInfo = (CServiceInfo*)m_ServicesList.GetItemData( iItem );
			if ( pInfo->m_Addr.ip[0] != 0 )	// "off" services won't have an IP
				addrs.AddToTail( pInfo->m_Addr );
		}
		if ( addrs.Count() == 0 )
		{
			AfxMessageBox( "No workers selected, or they all are off." );
			return;
		}
		
		AppendStr( commandLine, sizeof( commandLine ), " -mpi_PatchWorkers %d", addrs.Count() );
		for ( int i=0; i < addrs.Count(); i++ )
		{
			AppendStr( commandLine, sizeof( commandLine ), " %d.%d.%d.%d", addrs[i].ip[0], addrs[i].ip[1], addrs[i].ip[2], addrs[i].ip[3] );
		}

		STARTUPINFO si;
		memset( &si, 0, sizeof( si ) );
		si.cb = sizeof( si );
		
		PROCESS_INFORMATION pi;
		memset( &pi, 0, sizeof( pi ) );

		if ( CreateProcess( NULL, commandLine, 
			NULL, NULL, false, 
			0,
			NULL,
			(const char *)dlg.m_PatchDirectory,
			&si,
			&pi ) )
		{
			CloseHandle( pi.hProcess );
			CloseHandle( pi.hThread );

			V_AfxMessageBox( MB_OK, "Patch master successfully started.\nServices patching now.\nClose the patch master console app when finished." );
		}
		else
		{
			V_AfxMessageBox( MB_OK, "Error starting patch master: %s", GetLastErrorString() );
		}		 		
	}
}


void CServicesDlg::OnStopServices() 
{
	if ( MessageBox( "Warning: if you stop these services, you won't be able to control them from this application, and must restart them manually. Contine?", "Warning", MB_YESNO ) == IDYES )
	{
		CUtlVector<char> data;
		BuildVMPIPingPacket( data, VMPI_STOP_SERVICE );
		SendToSelectedServices( data.Base(), data.Count() );
	}
}

void CServicesDlg::OnStopJobs() 
{
	CUtlVector<char> data;
	BuildVMPIPingPacket( data, VMPI_KILL_PROCESS );
	SendToSelectedServices( data.Base(), data.Count() );
}


void CServicesDlg::OnFilterByPassword() 
{
	CSetPasswordDlg dlg( IDD_SET_PASSWORD );
	dlg.m_Password = m_Password;

	if ( dlg.DoModal() == IDOK )
	{
		m_Password = dlg.m_Password;
		m_PasswordDisplay.SetWindowText( m_Password );

		m_Services.PurgeAndDeleteElements();
		m_ServicesList.DeleteAllItems();

		UpdateServiceCountDisplay();

		// Re-ping everyone immediately.
		m_dwLastServicesPing = GetTickCount() - SERVICES_PING_INTERVAL;
	}	
}

// This sets a new password on the selected services.
void CServicesDlg::OnForcePassword() 
{
	CSetPasswordDlg dlg( IDD_FORCE_PASSWORD );
	dlg.m_Password = "password";

	if ( dlg.DoModal() == IDOK )
	{
		CUtlVector<char> data;

		BuildVMPIPingPacket( data, VMPI_FORCE_PASSWORD_CHANGE, VMPI_PROTOCOL_VERSION, true );
		const char *pNewPassword = dlg.m_Password;
		data.AddMultipleToTail( V_strlen( pNewPassword ) + 1, pNewPassword );

		SendToSelectedServices( data.Base(), data.Count() );
	}
}

void CServicesDlg::OnDblclkServicesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	POSITION pos = m_ServicesList.GetFirstSelectedItemPosition();
	if ( pos )
	{
		int iItem = m_ServicesList.GetNextSelectedItem( pos );
		if ( iItem != -1 )
		{
			CServiceInfo *pInfo = (CServiceInfo*)m_ServicesList.GetItemData( iItem );
			if ( pInfo )
			{
				// Launch vmpi_browser_job_search and have it auto-select this worker.
				char cmdLine[1024];
				Q_snprintf( cmdLine, sizeof( cmdLine ), "vmpi_job_search -SelectWorker %s", (const char*)pInfo->m_ComputerName );

				STARTUPINFO si;
				memset( &si, 0, sizeof( si ) );
				si.cb = sizeof( si );

				PROCESS_INFORMATION pi;
				memset( &pi, 0, sizeof( pi ) );

				if ( !CreateProcess( 
					NULL, 
					(char*)(const char*)cmdLine, 
					NULL,							// security
					NULL,
					TRUE,
					0,			// flags
					NULL,							// environment
					NULL,							// current directory
					&si,
					&pi ) )
				{
					char err[512];
					Q_snprintf( err, sizeof( err ), "Can't run '%s'", (LPCTSTR)cmdLine );
					MessageBox( err, "Error", MB_OK );
				}
			}			
		}
	}
	
	*pResult = 0;
}

void CServicesDlg::OnSize(UINT nType, int cx, int cy) 
{
	CIdleDialog::OnSize(nType, cx, cy);
	
	m_AnchorMgr.UpdateAnchors( this );	
}


BOOL CServicesDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *pHdr = (NMHDR*)lParam;
	if ( pHdr->idFrom == IDC_SERVICES_LIST )
	{
		if ( pHdr->code == LVN_COLUMNCLICK )
		{
			LPNMLISTVIEW pListView = (LPNMLISTVIEW)lParam;

			// Now sort by this column.
			int iSortColumn = max( 0, min( pListView->iSubItem, (int)ARRAYSIZE( g_ColumnInfos ) - 1 ) );
			PushSortColumn( iSortColumn );
			ResortItems();
		}
	}

	return CIdleDialog::OnNotify(wParam, lParam, pResult);
}


void CServicesDlg::BuildClipboardText( CUtlVector<char> &clipboardText )
{
	// Add the header information.
	CHeaderCtrl *pHeader = m_ServicesList.GetHeaderCtrl();
	for ( int i=0; i < pHeader->GetItemCount(); i++ )
	{
		char tempBuffer[512];
		HDITEM item;
		memset( &item, 0, sizeof( item ) );
		item.mask = HDI_TEXT;
		item.pszText = tempBuffer;
		item.cchTextMax = sizeof( tempBuffer ) - 1;

		if ( !pHeader->GetItem( i, &item ) )
			item.pszText = "<bug>";

		clipboardText.AddMultipleToTail( strlen( item.pszText ), item.pszText );
		clipboardText.AddToTail( '\t' );
	}
	clipboardText.AddMultipleToTail( 2, "\r\n" );

	// Now add each line of data.
	int nItem = -1;
	while ( (nItem = m_ServicesList.GetNextItem( nItem, LVNI_ALL )) != -1 )
	{
		char tempBuffer[512];
		LVITEM item;
		memset( &item, 0, sizeof( item ) );
		item.mask = LVIF_TEXT;
		item.iItem = nItem;
		item.pszText = tempBuffer;
		item.cchTextMax = sizeof( tempBuffer ) - 1;

		for ( int i=0; i < pHeader->GetItemCount(); i++ )
		{
			item.iSubItem = i;
			if ( !m_ServicesList.GetItem( &item ) )
			{
				item.pszText = "<bug>";
			}

			clipboardText.AddMultipleToTail( strlen( item.pszText ), item.pszText );
			clipboardText.AddToTail( '\t' );
		}
		clipboardText.AddMultipleToTail( 2, "\r\n" );
	}
	clipboardText.AddToTail( 0 );
}


void CServicesDlg::OnCopyToClipboard()
{
	// Open and clear the clipboard.
	if ( !OpenClipboard() )
		return;

	EmptyClipboard();

	// Setup the clipboard text.
	CUtlVector<char> clipboardText;
	BuildClipboardText( clipboardText );

	// Put the clipboard text into a global memory object.
	HANDLE hMem = GlobalAlloc( GMEM_MOVEABLE, clipboardText.Count() );
	void *ptr = GlobalLock( hMem );
	memcpy( ptr, clipboardText.Base(), clipboardText.Count() );
	GlobalUnlock( hMem );

	// Put it in the clipboard.
	SetClipboardData( CF_TEXT, hMem );

	// Cleanup.
	GlobalFree( hMem );
	CloseClipboard();
}


void CServicesDlg::OnBnClickedEnableMachines()
{
	CUtlVector<char> data;
	BuildVMPIPingPacket( data, VMPI_DISABLE_SCREENSAVER_MODE );
	SendToSelectedServices( data.Base(), data.Count() );
}



void CServicesDlg::OnBnClickedRequestLog()
{
	static CString lastLogOutputDir;

	// MEGA-HACK: Reuse CSetPasswordDlg because it deals with inputting a single string.
	// I don't think I know enough about MFC stuff to replicate this anymore.
	CSetPasswordDlg dlg( IDD_REQUEST_LOG_DIR );
	dlg.m_Password = lastLogOutputDir;

	if ( dlg.DoModal() == IDOK )
	{
		lastLogOutputDir = dlg.m_Password;

		CUtlVector<char> data;

		BuildVMPIPingPacket( data, VMPI_LOG_FILE_REQUEST );
		const char *pPath = dlg.m_Password;
		data.AddMultipleToTail( V_strlen( pPath ) + 1, pPath );

		SendToSelectedServices( data.Base(), data.Count() );
	}
}
