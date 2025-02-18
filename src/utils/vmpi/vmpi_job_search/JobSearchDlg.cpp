//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// JobSearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JobSearchDlg.h"
#include "imysqlwrapper.h"
#include "tier1/strtools.h"
#include "utllinkedlist.h"
#include "vmpi_browser_helpers.h"
#include "vmpi_defs.h"
#include "net_view_thread.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// These are stored with jobs to help with sorting and to remember the job ID.
class CJobInfo
{
public:
	unsigned long	m_JobID;
	CString			m_StartTimeUnformatted;
	CString			m_MachineName;
	CString			m_BSPFilename;
	DWORD			m_RunningTimeMS;
};



/////////////////////////////////////////////////////////////////////////////
// CJobSearchDlg dialog


CJobSearchDlg::CJobSearchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJobSearchDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CJobSearchDlg)
	//}}AFX_DATA_INIT
	m_pSQL = NULL;
	m_hMySQLDLL = NULL;
}


CJobSearchDlg::~CJobSearchDlg()
{
	if ( m_pSQL )
	{
		m_pSQL->Release();
	}

	if ( m_hMySQLDLL )
	{
		Sys_UnloadModule( m_hMySQLDLL );
	}
}


void CJobSearchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJobSearchDlg)
	DDX_Control(pDX, IDC_WORKER_LIST, m_WorkerList);
	DDX_Control(pDX, IDC_USER_LIST, m_UserList);
	DDX_Control(pDX, IDC_JOBS_LIST, m_JobsList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJobSearchDlg, CDialog)
	//{{AFX_MSG_MAP(CJobSearchDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_JOBS_LIST, OnDblclkJobsList)
	ON_LBN_DBLCLK(IDC_USER_LIST, OnDblclkUserList)
	ON_LBN_DBLCLK(IDC_WORKER_LIST, OnDblclkWorkerList)
	ON_BN_CLICKED(IDC_QUIT, OnQuit)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


int CJobSearchDlg::GetSelectedJobIndex()
{
	POSITION pos = m_JobsList.GetFirstSelectedItemPosition();
	if ( pos )
		return m_JobsList.GetNextSelectedItem( pos );
	else
		return -1;
}


/////////////////////////////////////////////////////////////////////////////
// CJobSearchDlg message handlers

void CJobSearchDlg::OnDblclkJobsList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int iItem = GetSelectedJobIndex();
	if ( iItem != -1 )
	{
		CJobInfo *pInfo = (CJobInfo*)m_JobsList.GetItemData( iItem );

		CString cmdLine;
		cmdLine.Format( "vmpi_job_watch -JobID %d -dbname \"%s\" -hostname \"%s\" -username \"%s\"", 
			pInfo->m_JobID, (const char*)m_DBName, (const char*)m_HostName, (const char*)m_UserName );

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
			CString errStr;
			errStr.Format( "Error launching '%s'", cmdLine.GetBuffer() );
			MessageBox( errStr, "Error", MB_OK );
		}
	}
	
	*pResult = 0;
}


static int CALLBACK JobsSortFn( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CJobInfo *pInfo1 = (CJobInfo*)iItem1;
	CJobInfo *pInfo2 = (CJobInfo*)iItem2;

	return strcmp( pInfo2->m_StartTimeUnformatted, pInfo1->m_StartTimeUnformatted );
}


void CJobSearchDlg::ClearJobsList()
{
	// First, delete all the JobInfo structures we have in it.
	int nItems = m_JobsList.GetItemCount();
	for ( int i=0; i < nItems; i++ )
	{
		CJobInfo *pInfo = (CJobInfo*)m_JobsList.GetItemData( i );
		delete pInfo;
	}

	m_JobsList.DeleteAllItems();
}		   


void CJobSearchDlg::RepopulateJobsList()
{
	// It's assumed coming into this routine that the caller just executed a query that we can iterate over.
	ClearJobsList();

	CUtlLinkedList<CJobInfo*, int> jobInfos;
	while ( GetMySQL()->NextRow() )
	{
		CJobInfo *pInfo = new CJobInfo;
		pInfo->m_StartTimeUnformatted = GetMySQL()->GetColumnValue( "StartTime" ).String();
		pInfo->m_JobID = GetMySQL()->GetColumnValue( "JobID" ).Int32();
		pInfo->m_MachineName = GetMySQL()->GetColumnValue( "MachineName" ).String();
		pInfo->m_BSPFilename = GetMySQL()->GetColumnValue( "BSPFilename" ).String();
		pInfo->m_RunningTimeMS = GetMySQL()->GetColumnValue( "RunningTimeMS" ).Int32();

		jobInfos.AddToTail( pInfo );
	}


	FOR_EACH_LL( jobInfos, j )
	{
		CJobInfo *pInfo = jobInfos[j];

		// Add the item.
		int iItem = m_JobsList.InsertItem( 0, "", NULL );
		
		// Associate it with the job structure.
		m_JobsList.SetItemData( iItem, (DWORD)pInfo );
		
		char dateStr[128];
		const char *pDate = pInfo->m_StartTimeUnformatted;
		if ( strlen( pDate ) == 14 ) // yyyymmddhhmmss
		{
			Q_snprintf( dateStr, sizeof( dateStr ), "%c%c/%c%c %c%c:%c%c:00", 
				pDate[4], pDate[5],
				pDate[6], pDate[7],
				pDate[8], pDate[9],
				pDate[10], pDate[11] );
		}

		m_JobsList.SetItemText( iItem, 0, dateStr );
		m_JobsList.SetItemText( iItem, 1, pInfo->m_MachineName );
		m_JobsList.SetItemText( iItem, 2, pInfo->m_BSPFilename );

		char timeStr[512];
		if ( pInfo->m_RunningTimeMS == RUNNINGTIME_MS_SENTINEL )
		{
			Q_strncpy( timeStr, "?", sizeof( timeStr ) );
		}
		else
		{
			FormatTimeString( pInfo->m_RunningTimeMS / 1000, timeStr, sizeof( timeStr ) );
		}
		m_JobsList.SetItemText( iItem, 3, timeStr );

		char jobIDStr[512];
		Q_snprintf( jobIDStr, sizeof( jobIDStr ), "%d", pInfo->m_JobID );
		m_JobsList.SetItemText( iItem, 4, jobIDStr );
	}
	
	m_JobsList.SortItems( JobsSortFn, (LPARAM)&m_JobsList );
}


void CJobSearchDlg::OnDblclkUserList() 
{
	int sel = m_UserList.GetCurSel();
	if ( sel != LB_ERR )
	{
		CString computerName;
		m_UserList.GetText( sel, computerName );

		// Look for jobs that this user initiated.
		char query[4096];
		Q_snprintf( query, sizeof( query ), "select RunningTimeMS, JobID, BSPFilename, StartTime, MachineName from job_master_start where MachineName=\"%s\"", (const char*)computerName );
		GetMySQL()->Execute( query );
		
		RepopulateJobsList();
	}
}

void CJobSearchDlg::OnDblclkWorkerList() 
{
	int sel = m_WorkerList.GetCurSel();
	if ( sel != LB_ERR )
	{
		CString computerName;
		m_WorkerList.GetText( sel, computerName );

		// This query does:
		// 1. Take the workers with the specified MachineName.
		// 2. Only use IsMaster = 0.
		// 3. Now get all the job_master_start records with the same JobID.
		char query[4096];
		Q_snprintf( query, sizeof( query ), "select job_master_start.RunningTimeMS, job_master_start.JobID, job_master_start.BSPFilename, job_master_start.StartTime, job_master_start.MachineName "
			"from job_master_start, job_worker_start "
			"where job_worker_start.MachineName = \"%s\" and "
			"IsMaster = 0 and "
			"job_master_start.JobID = job_worker_start.JobID",
			(const char*)computerName );
		GetMySQL()->Execute( query );
		
		RepopulateJobsList();
	}
}

bool ReadStringFromFile( FILE *fp, char *pStr, int strSize )
{
	int i=0;
	for ( i; i < strSize-2; i++ )
	{
		if ( fread( &pStr[i], 1, 1, fp ) != 1 ||
			pStr[i] == '\n' )
		{
			break;
		}
	}

	pStr[i] = 0;
	return i != 0;
}

const char* FindArg( const char *pArgName, const char *pDefault="" )
{
	for ( int i=1; i < __argc; i++ )
	{
		if ( Q_stricmp( pArgName, __argv[i] ) == 0 )
		{
			if ( (i+1) < __argc )
				return __argv[i+1];
			else
				return pDefault;
		}
	}
	return NULL;
}

BOOL CJobSearchDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_JobsList.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	char str[512];

	// Init the mysql database.
	const char *pDBName = FindArg( "-dbname", NULL );
	const char *pHostName = FindArg( "-hostname", NULL );
	const char *pUserName = FindArg( "-username", NULL );

	if ( pDBName && pHostName && pUserName )
	{
		m_DBName = pDBName;
		m_HostName = pHostName;
		m_UserName = pUserName;
	}
	else
	{
		// Load the dbinfo_browser.txt file to get the database information.
		const char *pFilename = FindArg( "-dbinfo", NULL );
		if ( !pFilename )
			pFilename = "dbinfo_job_search.txt";

		FILE *fp = fopen( pFilename, "rt" );
		if ( !fp )
		{
			Q_snprintf( str, sizeof( str ), "Can't open '%s' for database info.", pFilename );
			MessageBox( str, "Error", MB_OK );
			EndDialog( 0 );
			return FALSE;
		}

		char hostName[512], dbName[512], userName[512];
		if ( !ReadStringFromFile( fp, hostName, sizeof( hostName ) ) ||
			 !ReadStringFromFile( fp, dbName, sizeof( dbName ) ) || 
			 !ReadStringFromFile( fp, userName, sizeof( userName ) ) 
			 )
		{
			fclose( fp );
			Q_snprintf( str, sizeof( str ), "'%s' has invalid format.", pFilename );
			MessageBox( str, "Error", MB_OK );
			EndDialog( 0 );
			return FALSE;
		}

		m_DBName = dbName;
		m_HostName = hostName;
		m_UserName = userName;

		fclose( fp );		
	}

	// Get the mysql interface.
	if ( !Sys_LoadInterface( "mysql_wrapper", MYSQL_WRAPPER_VERSION_NAME, &m_hMySQLDLL, (void**)&m_pSQL ) )
		return false;
	
	if ( !m_pSQL->InitMySQL( m_DBName, m_HostName, m_UserName ) )
	{
		Q_snprintf( str, sizeof( str ), "Can't init MYSQL db (db = '%s', host = '%s', user = '%s')", (const char*)m_DBName, (const char*)m_HostName, (const char*)m_UserName );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 0 );
		return FALSE;
	}

	// Setup the headers for the job info list.
	struct
	{
		char *pText;
		int width;
	} titles[] =
	{
		{"Date", 100},
		{"User", 100},
		{"BSP Filename", 100},
		{"Running Time", 100},
		{"Job ID", 100}
	};
	for ( int i=0; i < ARRAYSIZE( titles ); i++ )
	{
		m_JobsList.InsertColumn( i, titles[i].pText, LVCFMT_LEFT, titles[i].width, i );
	}
	
	
	CUtlVector<char*> computerNames;
	CNetViewThread netView;
	netView.Init();
	DWORD startTime = GetTickCount();
	while ( 1 )
	{
		netView.GetComputerNames( computerNames, true );
		if ( computerNames.Count() > 0 )
			break;

		Sleep( 30 );
		if ( GetTickCount() - startTime > 5000 )
		{
			Q_snprintf( str, sizeof( str ), "Unable to get computer names Can't init MYSQL db (db = '%s', host = '%s', user = '%s')", (const char*)m_DBName, (const char*)m_HostName, (const char*)m_UserName );
			MessageBox( str, "Error", MB_OK );
			EndDialog( 0 );
			return FALSE;
		}
	}

	PopulateWorkerList( computerNames );
	PopulateUserList( computerNames );	


	// Auto-select a worker?
	const char *pSelectWorker = FindArg( "-SelectWorker", NULL );
	if ( pSelectWorker )
	{
		int index = m_WorkerList.FindString( -1, pSelectWorker );
		if ( index != LB_ERR )
		{
			m_WorkerList.SetCurSel( index );
			OnDblclkWorkerList();
		}
	}


	// Setup our anchors.
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_SEARCH_BY_USER_PANEL ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_USER_LIST ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKER_PANEL ), ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKER_LIST ), ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_JOBS_PANEL ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_RIGHT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_JOBS_LIST ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_RIGHT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_QUIT ), ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM, ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CJobSearchDlg::PopulateWorkerList( CUtlVector<char*> &computerNames )
{
	m_WorkerList.ResetContent();
	for ( int i=0; i < computerNames.Count(); i++ )
	{
		m_WorkerList.AddString( computerNames[i] );
	}
}

void CJobSearchDlg::PopulateUserList( CUtlVector<char*> &computerNames )
{
	m_UserList.ResetContent();
	for ( int i=0; i < computerNames.Count(); i++ )
	{
		m_UserList.AddString( computerNames[i] );
	}
}



void CJobSearchDlg::OnQuit() 
{
	EndDialog( 0 );	
}

void CJobSearchDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	m_AnchorMgr.UpdateAnchors( this );	
}
