//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// JobWatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "JobWatchDlg.h"
#include "tier1/strtools.h"
#include "consolewnd.h"
#include "vmpi_browser_helpers.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define IMPLEMENT_SORT_NUMBER_FN( FnName, VarName ) \
	static int CALLBACK FnName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam ) \
	{ \
		CWorkerInfo *pInfo1 = (CWorkerInfo*)iItem1; \
		CWorkerInfo *pInfo2 = (CWorkerInfo*)iItem2; \
		return pInfo1->VarName > pInfo2->VarName; \
	}

static int CALLBACK SortByName( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CWorkerInfo *pInfo1 = (CWorkerInfo*)iItem1;
	CWorkerInfo *pInfo2 = (CWorkerInfo*)iItem2;

	return strcmp( pInfo1->m_ComputerName, pInfo2->m_ComputerName );
}

static int CALLBACK SortByCurrentStage( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam )
{
	CWorkerInfo *pInfo1 = (CWorkerInfo*)iItem1;
	CWorkerInfo *pInfo2 = (CWorkerInfo*)iItem2;

	return strcmp( pInfo1->m_CurrentStage, pInfo2->m_CurrentStage );
}

IMPLEMENT_SORT_NUMBER_FN( SortByConnected, m_bConnected )
IMPLEMENT_SORT_NUMBER_FN( SortByWorkUnitsDone, m_nWorkUnitsDone );
IMPLEMENT_SORT_NUMBER_FN( SortByRunningTime, m_RunningTimeMS );
IMPLEMENT_SORT_NUMBER_FN( SortByThread0WU, m_ThreadWUs[0] );
IMPLEMENT_SORT_NUMBER_FN( SortByThread1WU, m_ThreadWUs[1] );
IMPLEMENT_SORT_NUMBER_FN( SortByThread2WU, m_ThreadWUs[2] );
IMPLEMENT_SORT_NUMBER_FN( SortByThread3WU, m_ThreadWUs[3] );

typedef int (CALLBACK *ServicesSortFn)( LPARAM iItem1, LPARAM iItem2, LPARAM lpParam );

struct
{
	char			*pText;
	int				width;
	ServicesSortFn	sortFn;
} g_ColumnInfos[] =
{
	{"Computer Name", 150, SortByName},
	{"Connected", 70, SortByConnected},
	{"Work Units Done", 100, SortByWorkUnitsDone},
	{"Running Time", 80, SortByRunningTime},
	{"Current Stage", 180, SortByCurrentStage},
	{"Thread 0", 70, SortByThread0WU},
	{"Thread 1", 70, SortByThread1WU},
	{"Thread 2", 70, SortByThread2WU},
	{"Thread 3", 70, SortByThread3WU}
};

#define COLUMN_COMPUTER_NAME	0
#define COLUMN_CONNECTED		1
#define COLUMN_WORK_UNITS_DONE	2
#define COLUMN_RUNNING_TIME		3
#define COLUMN_CURRENT_STAGE	4
#define COLUMN_THREAD0_WU		5
#define COLUMN_THREAD1_WU		6
#define COLUMN_THREAD2_WU		7
#define COLUMN_THREAD3_WU		8

int g_iSortColumn = 0;


/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg dialog


CJobWatchDlg::CJobWatchDlg(CWnd* pParent /*=NULL*/)
	: CIdleDialog(CJobWatchDlg::IDD, pParent)
{
	m_CurMessageIndex = 0;
	m_CurGraphTime = 0;
	m_pSQL = NULL;
	m_hMySQLDLL = NULL;
	m_CurWorkerTextToken = 0;
	m_LastQueryTime = 0;

	//{{AFX_DATA_INIT(CJobWatchDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


CJobWatchDlg::~CJobWatchDlg()
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


void CJobWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJobWatchDlg)
	DDX_Control(pDX, IDC_WORKERS, m_Workers);
	DDX_Control(pDX, IDC_TEXTOUTPUT, m_TextOutput);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJobWatchDlg, CIdleDialog)
	//{{AFX_MSG_MAP(CJobWatchDlg)
	ON_LBN_SELCHANGE(IDC_WORKERS, OnSelChangeWorkers)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ODSTATECHANGED, IDC_WORKERS, OnOdstatechangedWorkers)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_WORKERS, OnItemchangedWorkers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJobWatchDlg message handlers

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


BOOL CJobWatchDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();


	m_Workers.SetExtendedStyle( LVS_EX_FULLROWSELECT );
	
	// Setup the headers.
	for ( int i=0; i < ARRAYSIZE( g_ColumnInfos ); i++ )
	{
		m_Workers.InsertColumn( i, g_ColumnInfos[i].pText, LVCFMT_LEFT, g_ColumnInfos[i].width, i );
	}


	m_GraphControl.SubclassDlgItem( IDC_GRAPH_AREA, this );


	CString str;

	// Get all our startup info from the command line.
	const char *pJobID = FindArg( "-JobID", NULL );
	const char *pDBName = FindArg( "-dbname", NULL );
	const char *pHostName = FindArg( "-hostname", NULL );
	const char *pUserName = FindArg( "-username", NULL );

	if ( !pJobID )
	{
		str.Format( "Missing a command line parameter (-JobID or -dbname or -hostname or -username)" );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 1 );
		return FALSE;
	}

	char hostName[512], dbName[512], userName[512];
	if ( !pDBName || !pHostName || !pUserName )
	{
		char errString[512];

		// If they don't specify the DB info, get it from where 
		const char *pFilename = "dbinfo_job_search.txt";
		FILE *fp = fopen( pFilename, "rt" );
		if ( !fp )
		{
			Q_snprintf( errString, sizeof( errString ), "Can't open '%s' for database info.", pFilename );
			MessageBox( errString, "Error", MB_OK );
			EndDialog( 0 );
			return FALSE;
		}

		if ( !ReadStringFromFile( fp, hostName, sizeof( hostName ) ) ||
			 !ReadStringFromFile( fp, dbName, sizeof( dbName ) ) || 
			 !ReadStringFromFile( fp, userName, sizeof( userName ) ) 
			 )
		{
			fclose( fp );
			Q_snprintf( errString, sizeof( errString ), "'%s' has invalid format.", pFilename );
			MessageBox( errString, "Error", MB_OK );
			EndDialog( 0 );
			return FALSE;
		}

		pDBName = dbName;
		pHostName = hostName;
		pUserName = userName;

		fclose( fp );
	}

	m_JobID = atoi( pJobID );

	// Get the mysql interface.
	IMySQL *pSQL;
	if ( !Sys_LoadInterface( "mysql_wrapper", MYSQL_WRAPPER_VERSION_NAME, &m_hMySQLDLL, (void**)&pSQL ) )
		return false;
	
	if ( !pSQL->InitMySQL( pDBName, pHostName, pUserName ) )
	{
		pSQL->Release();
		str.Format( "Can't init MYSQL db (db = '%s', host = '%s', user = '%s')", pDBName, pHostName, pUserName );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 0 );
		return FALSE;
	}

	m_pSQL = CreateMySQLAsync( pSQL );
	if ( !m_pSQL )
	{
		pSQL->Release();
		str.Format( "Can't create IMySQLAsync" );
		MessageBox( str, "Error", MB_OK );
		EndDialog( 0 );
		return FALSE;
	}
	
	
	memset( m_bQueriesInProgress, 0, sizeof( m_bQueriesInProgress ) );


	// (Init the idle processor so we can update text and graphs).
	StartIdleProcessing( 100 );

	// Fill in the command line control.
	char cmdLine[2048];
	Q_snprintf( cmdLine, sizeof( cmdLine ), "vmpi_job_watch -JobID %s -hostname %s -dbname %s -username %s",
		pJobID, pHostName, pDBName, pUserName );
	SetDlgItemText( IDC_COMMAND_LINE, cmdLine );

	// Setup anchors.
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKERS_PANEL ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_WORKERS ), ANCHOR_LEFT, ANCHOR_TOP, ANCHOR_WIDTH_PERCENT, ANCHOR_HEIGHT_PERCENT );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_TEXT_OUTPUT_PANEL ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM );
	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_TEXTOUTPUT ), ANCHOR_LEFT, ANCHOR_HEIGHT_PERCENT, ANCHOR_WIDTH_PERCENT, ANCHOR_BOTTOM );

	m_AnchorMgr.AddAnchor( this, GetDlgItem( IDC_GRAPHS_PANEL ), ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );
	m_AnchorMgr.AddAnchor( this, &m_GraphControl, ANCHOR_WIDTH_PERCENT, ANCHOR_TOP, ANCHOR_RIGHT, ANCHOR_HEIGHT_PERCENT );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


CWorkerInfo* CJobWatchDlg::FindWorkerByID( unsigned long jobWorkerID )
{
	int nIndex = -1;
	while ( ( nIndex = m_Workers.GetNextItem( nIndex, LVNI_ALL ) ) != -1 )
	{
		CWorkerInfo *pInfo = (CWorkerInfo*)m_Workers.GetItemData( nIndex );
		if ( pInfo->m_JobWorkerID == jobWorkerID )
			return pInfo;
	}

	return NULL;
}


CWorkerInfo* CJobWatchDlg::FindWorkerByMachineName( const char *pMachineName )
{
	int nIndex = -1;
	while ( ( nIndex = m_Workers.GetNextItem( nIndex, LVNI_ALL ) ) != -1 )
	{
		CWorkerInfo *pInfo = (CWorkerInfo*)m_Workers.GetItemData( nIndex );
		if ( Q_stricmp( pInfo->m_ComputerName, pMachineName ) == 0 )
			return pInfo;
	}

	return NULL;
}


void CJobWatchDlg::SetWorkerListItemInt( int nIndex, int iColumn, int value )
{
	char str[512];
	Q_snprintf( str, sizeof( str ), "%d", value );
	m_Workers.SetItemText( nIndex, iColumn, str );
}


void CJobWatchDlg::UpdateWorkersList()
{
	int nIndex = -1;
	while ( ( nIndex = m_Workers.GetNextItem( nIndex, LVNI_ALL ) ) != -1 )
	{
		CWorkerInfo *pInfo = (CWorkerInfo*)m_Workers.GetItemData( nIndex );
	
		char *pConnectedStr = pInfo->m_bConnected ? "yes" : "no";
		m_Workers.SetItemText( nIndex, COLUMN_CONNECTED, pConnectedStr );

		SetWorkerListItemInt( nIndex, COLUMN_WORK_UNITS_DONE, pInfo->m_nWorkUnitsDone );

		char timeStr[1024];
		FormatTimeString( pInfo->m_RunningTimeMS / 1000, timeStr, sizeof( timeStr ) );
		m_Workers.SetItemText( nIndex, COLUMN_RUNNING_TIME, timeStr );

		// Current stage.
		SetWorkerListItemInt( nIndex, COLUMN_WORK_UNITS_DONE, pInfo->m_nWorkUnitsDone );
		
		m_Workers.SetItemText( nIndex, COLUMN_CURRENT_STAGE, pInfo->m_CurrentStage );

		SetWorkerListItemInt( nIndex, COLUMN_THREAD0_WU, pInfo->m_ThreadWUs[0] );
		SetWorkerListItemInt( nIndex, COLUMN_THREAD1_WU, pInfo->m_ThreadWUs[1] );
		SetWorkerListItemInt( nIndex, COLUMN_THREAD2_WU, pInfo->m_ThreadWUs[2] );
		SetWorkerListItemInt( nIndex, COLUMN_THREAD3_WU, pInfo->m_ThreadWUs[3] );
	}

	ResortItems();
}


bool CJobWatchDlg::GetCurJobWorkerID( unsigned long &id )
{
	POSITION pos = m_Workers.GetFirstSelectedItemPosition();
	if ( !pos )
		return false;

	int index = m_Workers.GetNextSelectedItem( pos );
	CWorkerInfo *pInfo = (CWorkerInfo*)m_Workers.GetItemData( index );
	id = pInfo->m_JobWorkerID;
	return true;
}


void CJobWatchDlg::OnSelChangeWorkers() 
{
	// Clear the text output and invalidate any old queries for text.
	int nLen = m_TextOutput.SendMessage( EM_GETLIMITTEXT, 0, 0 );
	m_TextOutput.SendMessage( EM_SETSEL, 0, nLen );
	m_TextOutput.SendMessage( EM_REPLACESEL, FALSE, (LPARAM)"" );

	m_CurMessageIndex = 0;
	m_CurWorkerTextToken++;
	m_LastQueryTime = 0; // force a query.

	m_GraphControl.Clear();
	m_CurGraphTime = -1;
}


void CJobWatchDlg::ResortItems()
{
	m_Workers.SortItems( g_ColumnInfos[g_iSortColumn].sortFn, (LPARAM)this );
}



void CJobWatchDlg::OnIdle()
{
	// Issue any queries that we need to.
	DWORD curTime = GetTickCount();
	if ( curTime - m_LastQueryTime >= 1000 )
	{
		m_LastQueryTime = curTime;
		char query[2048];
		
		unsigned long jobWorkerID;
		bool bJobWorkerIDValid = GetCurJobWorkerID( jobWorkerID );
		
		if ( !m_bQueriesInProgress[QUERY_TEXT] && bJobWorkerIDValid )
		{
			Q_snprintf( query, sizeof( query ), "select * from text_messages where JobWorkerID=%lu and MessageIndex >= %lu", jobWorkerID, m_CurMessageIndex );
			m_pSQL->Execute( query, (void*)(QUERY_TEXT | (m_CurWorkerTextToken << 16)) );	
			m_bQueriesInProgress[QUERY_TEXT] = true;
		}

		if ( !m_bQueriesInProgress[QUERY_GRAPH] && bJobWorkerIDValid )
		{
			Q_snprintf( query, sizeof( query ), "select * from graph_entry where JobWorkerID=%lu", jobWorkerID );
			m_pSQL->Execute( query, (void*)QUERY_GRAPH );	
			m_bQueriesInProgress[QUERY_GRAPH] = true;
		}

		if ( !m_bQueriesInProgress[QUERY_WORKER_STATS] )
		{
			Q_snprintf( query, sizeof( query ), "select JobWorkerID, WorkerState, NumWorkUnits, "
				"RunningTimeMS, CurrentStage, Thread0WU, Thread1WU, Thread2WU, Thread3WU, IsMaster, MachineName "
				" from job_worker_start where JobID=%lu", m_JobID );

			m_pSQL->Execute( query, (void*)QUERY_WORKER_STATS );	
			m_bQueriesInProgress[QUERY_WORKER_STATS] = true;
		}
	}


	// Pickup query results.
	CQueryResults results;
	while ( m_pSQL->GetNextResults( results ) && results.m_pResults )
	{
		int iQueryID = ((int)results.m_pUserData) & 0xFFFF;
		int iExtraData = ((int)results.m_pUserData) >> 16;

		if ( results.m_pResults )
		{
			if ( iQueryID == QUERY_TEXT )
			{
				if ( iExtraData == m_CurWorkerTextToken )
				{
					ProcessQueryResults_Text( results.m_pResults );
				}
			}
			else if ( iQueryID == QUERY_GRAPH )
			{
				ProcessQueryResults_Graph( results.m_pResults );
			}
			else if ( iQueryID == QUERY_WORKER_STATS )
			{
				ProcessQueryResults_WorkerStats( results.m_pResults );
			}
			
			results.m_pResults->Release();
		}

		m_bQueriesInProgress[iQueryID] = false;
	}
}


void CJobWatchDlg::ProcessQueryResults_WorkerStats( IMySQLRowSet *pSet )
{
	bool bChange = false;
	while ( pSet->NextRow() )
	{
		int iColumn = 0;
		int workerID =  pSet->GetColumnValue_Int( iColumn++ );
		int workerState = pSet->GetColumnValue_Int( iColumn++ );
		int nWorkUnits = pSet->GetColumnValue_Int( iColumn++ );
		unsigned long runningTimeMS = pSet->GetColumnValue_Int( iColumn++ );
		const char *pCurrentStage = pSet->GetColumnValue_String( iColumn++ );
		int iThread0WU = pSet->GetColumnValue_Int( iColumn++ );
		int iThread1WU = pSet->GetColumnValue_Int( iColumn++ );
		int iThread2WU = pSet->GetColumnValue_Int( iColumn++ );
		int iThread3WU = pSet->GetColumnValue_Int( iColumn++ );
		int bIsMaster = pSet->GetColumnValue_Int( iColumn++ );
		const char *pMachineName = pSet->GetColumnValue_String( iColumn );

		CWorkerInfo *pInfo = FindWorkerByID( workerID );
		if ( pInfo )
		{
			if ( workerState != pInfo->m_bConnected || 
				nWorkUnits != pInfo->m_nWorkUnitsDone ||
				runningTimeMS != pInfo->m_RunningTimeMS ||
				stricmp( pCurrentStage, pInfo->m_CurrentStage ) != 0 ||
				iThread0WU != pInfo->m_ThreadWUs[0] ||
				iThread1WU != pInfo->m_ThreadWUs[1] ||
				iThread2WU != pInfo->m_ThreadWUs[2] ||
				iThread3WU != pInfo->m_ThreadWUs[3]
				)
			{
				bChange = true;
				pInfo->m_bConnected = workerState;
				pInfo->m_nWorkUnitsDone = nWorkUnits;
				pInfo->m_RunningTimeMS = runningTimeMS;
				pInfo->m_CurrentStage = pCurrentStage;
				pInfo->m_ThreadWUs[0] = iThread0WU;
				pInfo->m_ThreadWUs[1] = iThread1WU;
				pInfo->m_ThreadWUs[2] = iThread2WU;
				pInfo->m_ThreadWUs[3] = iThread3WU;
			}
		}
		else
		{
			// Add a new entry.
			CWorkerInfo *pInfo = new CWorkerInfo;
			pInfo->m_ComputerName = pMachineName;
			pInfo->m_bConnected = false;
			pInfo->m_nWorkUnitsDone = 0;
			pInfo->m_RunningTimeMS = 0;
			pInfo->m_JobWorkerID = workerID;

			int index;
			if ( bIsMaster )
			{
				char tempStr[512];
				Q_snprintf( tempStr, sizeof( tempStr ), "%s [master]", (const char*)pInfo->m_ComputerName );
				index = m_Workers.InsertItem( COLUMN_COMPUTER_NAME, tempStr, NULL );
			}
			else
			{
				index = m_Workers.InsertItem( COLUMN_COMPUTER_NAME, pInfo->m_ComputerName, NULL );
			}

			m_Workers.SetItemData( index, (DWORD)pInfo );
			bChange = true;
		}
	}

	if ( bChange )
	{
		UpdateWorkersList();
	}
}


void CJobWatchDlg::ProcessQueryResults_Text( IMySQLRowSet *pSet )
{
	CUtlVector<char> text;

	while ( pSet->NextRow() )
	{
		const char *pTextStr = pSet->GetColumnValue( "text" ).String();
		int len = strlen( pTextStr );
		text.AddMultipleToTail( len, pTextStr );

		m_CurMessageIndex = pSet->GetColumnValue( "MessageIndex" ).Int32() + 1;
	}

	text.AddToTail( 0 );
	FormatAndSendToEditControl( m_TextOutput.GetSafeHwnd(), text.Base() );
}


void CJobWatchDlg::ProcessQueryResults_Graph( IMySQLRowSet *pSet )
{
	int iMSTime = pSet->GetColumnIndex( "MSSinceJobStart" );	
	int iBytesSent = pSet->GetColumnIndex( "BytesSent" );
	int iBytesReceived = pSet->GetColumnIndex( "BytesReceived" );

	// See if there's anything new.
	CUtlVector<CGraphEntry> entries;

	int highest = m_CurGraphTime;
	while ( pSet->NextRow() )
	{
		CGraphEntry entry;
		entry.m_msTime = pSet->GetColumnValue( iMSTime ).Int32();
		entry.m_nBytesSent = pSet->GetColumnValue( iBytesSent ).Int32();
		entry.m_nBytesReceived = pSet->GetColumnValue( iBytesReceived ).Int32();
		entries.AddToTail( entry );

		highest = max( highest, entry.m_msTime );
	}

	if ( highest > m_CurGraphTime )
	{
		m_CurGraphTime = highest;
		
		m_GraphControl.Clear();
		m_GraphControl.Fill( entries );
	}
}
 

void CJobWatchDlg::OnSize(UINT nType, int cx, int cy) 
{
	CIdleDialog::OnSize(nType, cx, cy);
	
	m_AnchorMgr.UpdateAnchors( this );	
}


BOOL CJobWatchDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *pHdr = (NMHDR*)lParam;
	if ( pHdr->idFrom == IDC_WORKERS )
	{
		if ( pHdr->code == LVN_COLUMNCLICK )
		{
			LPNMLISTVIEW pListView = (LPNMLISTVIEW)lParam;

			// Now sort by this column.
			g_iSortColumn = max( 0, min( pListView->iSubItem, (int)ARRAYSIZE( g_ColumnInfos ) - 1 ) );
			ResortItems();
		}
	}

	return CIdleDialog::OnNotify(wParam, lParam, pResult);
}

void CJobWatchDlg::OnOdstatechangedWorkers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLVODSTATECHANGE* pStateChanged = (NMLVODSTATECHANGE*)pNMHDR;

	if ( !( pStateChanged->uOldState & LVIS_SELECTED ) && ( pStateChanged->uNewState & LVIS_SELECTED ) )
	{
		OnSelChangeWorkers();
	}
	
	*pResult = 0;
}

void CJobWatchDlg::OnItemchangedWorkers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if ( !( pNMListView->uOldState & LVIS_SELECTED ) && ( pNMListView->uNewState & LVIS_SELECTED ) )
	{
		OnSelChangeWorkers();
	}
	
	*pResult = 0;
}
