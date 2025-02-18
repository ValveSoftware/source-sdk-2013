//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include <windows.h>
#include <conio.h>
#include <io.h>
#include "vmpi.h"
#include "vmpi_distribute_work.h"
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "utlvector.h"
#include "utllinkedlist.h"


#define EVENT_TYPE_SEND_WORK_UNIT	0
#define EVENT_TYPE_WU_STARTED		1
#define EVENT_TYPE_WU_COMPLETED		2

class CWorkUnitEvent
{
public:
	int m_iEventType;	// EVENT_TYPE_ define.
	int m_iWorker;
	double m_flTime;
};


class CWorkUnit
{
public:
	CWorkUnit()
	{
		m_iWorkerCompleted = -1;
	}
	
	int m_iWorkerCompleted;	// Which worker completed this work unit (-1 if not done yet).
	
	CUtlVector<CWorkUnitEvent> m_Events;
};


static CUtlVector<CWorkUnit> g_WorkUnits;
static double g_flJobStartTime;
static bool g_bTrackWorkUnitEvents = false;


static int CountActiveWorkUnits()
{
	int nActive = 0;
	for ( int i=0; i < g_WorkUnits.Count(); i++ )
	{
		if ( g_WorkUnits[i].m_iWorkerCompleted == -1 )
			++nActive;
	}
	return nActive;
}


// ------------------------------------------------------------------------ //
// Graphical functions.
// ------------------------------------------------------------------------ //

static bool g_bUseGraphics = false;
static HWND g_hWnd = 0;

static int g_LastSizeX = 600, g_LastSizeY = 600;

static COLORREF g_StateColors[] =
{
	RGB(50,50,50),
	RGB(100,0,0),
	RGB(150,0,0),
	RGB(0,155,0),
	RGB(0,255,0),
	RGB(0,0,150),
	RGB(0,0,250)
};

static HANDLE g_hCreateEvent = 0;
static HANDLE g_hDestroyWindowEvent = 0;
static HANDLE g_hDestroyWindowCompletedEvent = 0;

static CRITICAL_SECTION g_CS;

class CWUStatus
{
public:
	CWUStatus()
	{
		m_iState = 0;
		memset( &m_Rect, 0, sizeof( m_Rect ) );
	}
	
	RECT m_Rect;
	int m_iState;				// 0 = not sent yet
								// 1 = sent, 2 = sent recently
								// 3 = done, 4 = done recently
								// 5 = started, 6 = started recently
	float m_flTransitionTime;
};
CUtlVector<CWUStatus> g_WUStatus;
int g_nChanges = 0;
int g_nLastDrawnChanges = -1;

static LRESULT CALLBACK TrackerWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_PAINT:
		{
			// Do one pass for each color..
			HBRUSH hStateColors[ARRAYSIZE( g_StateColors )];
			for ( int i=0; i < ARRAYSIZE( hStateColors ); i++ )
				hStateColors[i] = CreateSolidBrush( g_StateColors[i] );

			// Copy the WU statuses.
			CUtlVector<CWUStatus> wuStatus;
			EnterCriticalSection( &g_CS );

			g_nLastDrawnChanges = g_nChanges;
			wuStatus.SetSize( g_WUStatus.Count() );
			memcpy( wuStatus.Base(), g_WUStatus.Base(), wuStatus.Count() * sizeof( wuStatus[0] ) );

			LeaveCriticalSection( &g_CS );

			PAINTSTRUCT ps;
			HDC hDC = BeginPaint( hwnd, &ps );		
			for ( int iState=0; iState < ARRAYSIZE( hStateColors ); iState++ )
			{
				HGDIOBJ hOldObj = SelectObject( hDC, hStateColors[iState] );

				for ( int iWU=0; iWU < wuStatus.Count(); iWU++ )
				{
					if ( wuStatus[iWU].m_iState != iState )
						continue;
					
					RECT &rc = wuStatus[iWU].m_Rect;
					Rectangle( hDC, rc.left, rc.top, rc.right, rc.bottom );
				}
				
				SelectObject( hDC, hOldObj );
				DeleteObject( hStateColors[iState] );
			}
			EndPaint( hwnd, &ps );
		}
		break;
		
		case WM_SIZE:
		{
			int width = LOWORD( lParam );
			int height = HIWORD( lParam );
			
			g_LastSizeX = width;
			g_LastSizeY = height;
			
			// Figure out the rectangles for everything.
			int nWorkUnits = g_WUStatus.Count();
			
			// What is the max width of the grid elements so they will fit in the width and height.
			int testSize;
			for ( testSize=20; testSize > 1; testSize-- )
			{
				int nX = width / testSize;
				int nY = height / testSize;
				if ( nX * nY >= nWorkUnits )
					break;
			}
			static int minTestSize = 3;
			testSize = max( testSize, minTestSize );
			
			int xPos=0, yPos=0;
			for ( int i=0; i < nWorkUnits; i++ )
			{
				g_WUStatus[i].m_Rect.left = xPos;
				g_WUStatus[i].m_Rect.top = yPos;
				g_WUStatus[i].m_Rect.right = xPos + testSize;
				g_WUStatus[i].m_Rect.bottom = yPos + testSize;
				
				xPos += testSize;
				if ( (xPos+testSize) > width )
				{
					yPos += testSize;
					xPos = 0;
				}
			}
		}
		break;
	}
	
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

static void CheckFlashTimers()
{
	double flCurTime = Plat_FloatTime();

	EnterCriticalSection( &g_CS );

	// Check timers for the events that just happened (we show them in a brighter color if they just occurred).
	for ( int iWU=0; iWU < g_WUStatus.Count(); iWU++ )
	{
		CWUStatus &s = g_WUStatus[iWU];
		
		if ( s.m_iState == 2 || s.m_iState == 4 || s.m_iState == 6 )
		{
			if ( flCurTime > s.m_flTransitionTime )
			{
				s.m_iState -= 1;
				++g_nChanges;
			}
		}
	}

	LeaveCriticalSection( &g_CS );
}

static DWORD WINAPI ThreadProc( LPVOID lpParameter )
{
	// Create the window.
	const char *pClassName = "VMPI_Tracker";

	// Register the application
	WNDCLASSEX WndClsEx;
	WndClsEx.cbSize        = sizeof(WNDCLASSEX);
	WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
	WndClsEx.lpfnWndProc   = TrackerWindowProc;
	WndClsEx.cbClsExtra    = 0;
	WndClsEx.cbWndExtra    = 0;
	WndClsEx.hIcon         = NULL;
	WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClsEx.lpszMenuName  = NULL;
	WndClsEx.lpszClassName = pClassName;
	WndClsEx.hInstance     = (HINSTANCE)GetCurrentProcess();
	WndClsEx.hIconSm       = NULL;
	RegisterClassEx(&WndClsEx);
	
	// Create the window.
	g_hWnd = CreateWindow( 
		pClassName,
		"VMPI Tracker",
		WS_OVERLAPPEDWINDOW,
		0, 0, g_LastSizeX, g_LastSizeY,
		NULL, NULL,
		(HINSTANCE)GetCurrentProcess(),
		NULL );

	ShowWindow( g_hWnd, SW_SHOW );
	
	// Tell the main thread we're ready.
	SetEvent( g_hCreateEvent );
	
	// Run our main loop.
	while ( WaitForSingleObject( g_hDestroyWindowEvent, 200 ) != WAIT_OBJECT_0 )
	{
		MSG msg;
		while ( PeekMessage( &msg, g_hWnd, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg); 
		}

		CheckFlashTimers();
				
		if ( g_nChanges != g_nLastDrawnChanges )
			InvalidateRect( g_hWnd, NULL, FALSE );
	}
	
	// Tell the main thread we're done.	
	SetEvent( g_hDestroyWindowCompletedEvent );
	return 0;
}

static void Graphical_Start()
{
	g_bUseGraphics = VMPI_IsParamUsed( mpi_Graphics );
	if ( !g_bUseGraphics )
		return;
	
	// Setup an event so we'll wait until the window is ready.
	if ( !g_hCreateEvent )
	{
		g_hCreateEvent = CreateEvent( 0, 0, 0, 0 );
		g_hDestroyWindowEvent = CreateEvent( 0, 0, 0, 0 );
		g_hDestroyWindowCompletedEvent = CreateEvent( 0, 0, 0, 0 );
		InitializeCriticalSection( &g_CS );
	}
	ResetEvent( g_hCreateEvent );
	ResetEvent( g_hDestroyWindowCompletedEvent );

	g_WUStatus.SetSize( g_WorkUnits.Count() );
	for ( int i=0; i < g_WUStatus.Count(); i++ )
		g_WUStatus[i].m_iState = 0;
	
	// Setup our thread.
	CreateThread( NULL, 0, ThreadProc, NULL, 0, NULL );
	
	// Wait until the event is signaled.
	WaitForSingleObject( g_hCreateEvent, INFINITE );
}

static void Graphical_WorkUnitSentToWorker( int iWorkUnit )
{
	if ( !g_bUseGraphics )
		return;
	
	EnterCriticalSection( &g_CS );
	
	CWUStatus &s = g_WUStatus[iWorkUnit];
	if ( s.m_iState != 3 && s.m_iState != 4 && s.m_iState != 5 && s.m_iState != 6 )
	{
		s.m_iState = 2;
		s.m_flTransitionTime = Plat_FloatTime() + 0.1f;
		++g_nChanges;
	}
	
	LeaveCriticalSection( &g_CS );
}

static void Graphical_WorkUnitStarted( int iWorkUnit )
{
	if ( !g_bUseGraphics )
		return;
	
	EnterCriticalSection( &g_CS );
	
	if ( g_WUStatus[iWorkUnit].m_iState != 3 && g_WUStatus[iWorkUnit].m_iState != 4 )
	{
		g_WUStatus[iWorkUnit].m_iState = 6;
		g_WUStatus[iWorkUnit].m_flTransitionTime = Plat_FloatTime() + 0.1f;
		++g_nChanges;
	}
	
	LeaveCriticalSection( &g_CS );
}

static void Graphical_WorkUnitCompleted( int iWorkUnit )
{
	if ( !g_bUseGraphics )
		return;
		
	EnterCriticalSection( &g_CS );
	g_WUStatus[iWorkUnit].m_iState = 4;
	g_WUStatus[iWorkUnit].m_flTransitionTime = Plat_FloatTime() + 0.1f;
	++g_nChanges;
	LeaveCriticalSection( &g_CS );
}

static void Graphical_End()
{
	if ( !g_bUseGraphics )
		return;
		
	SetEvent( g_hDestroyWindowEvent );
	WaitForSingleObject( g_hDestroyWindowCompletedEvent, INFINITE );
}


// ------------------------------------------------------------------------ //
// Interface functions.
// ------------------------------------------------------------------------ //

void VMPITracker_Start( int nWorkUnits )
{
	g_bTrackWorkUnitEvents = (VMPI_IsParamUsed( mpi_TrackEvents ) || VMPI_IsParamUsed( mpi_Graphics ));
	g_flJobStartTime = Plat_FloatTime();
	g_WorkUnits.Purge();

	if ( g_bTrackWorkUnitEvents )
	{
		g_WorkUnits.SetSize( nWorkUnits );
	}

	Graphical_Start();
}


void VMPITracker_WorkUnitSentToWorker( int iWorkUnit, int iWorker )
{
	if ( g_bTrackWorkUnitEvents )
	{
		CWorkUnitEvent event;
		event.m_iEventType = EVENT_TYPE_SEND_WORK_UNIT;
		event.m_iWorker = iWorker;
		event.m_flTime = Plat_FloatTime();
		g_WorkUnits[iWorkUnit].m_Events.AddToTail( event );
	}

	Graphical_WorkUnitSentToWorker( iWorkUnit );
}


void VMPITracker_WorkUnitStarted( int iWorkUnit, int iWorker )
{
	if ( g_bTrackWorkUnitEvents )
	{
		CWorkUnitEvent event;
		event.m_iEventType = EVENT_TYPE_WU_STARTED;
		event.m_iWorker = iWorker;
		event.m_flTime = Plat_FloatTime();
		g_WorkUnits[iWorkUnit].m_Events.AddToTail( event );
	}

	Graphical_WorkUnitStarted( iWorkUnit );
}


void VMPITracker_WorkUnitCompleted( int iWorkUnit, int iWorker )
{
	if ( g_bTrackWorkUnitEvents )
	{
		CWorkUnitEvent event;
		event.m_iEventType = EVENT_TYPE_WU_COMPLETED;
		event.m_iWorker = iWorker;
		event.m_flTime = Plat_FloatTime();
		g_WorkUnits[iWorkUnit].m_Events.AddToTail( event );
		g_WorkUnits[iWorkUnit].m_iWorkerCompleted = iWorker;
	}

	Graphical_WorkUnitCompleted( iWorkUnit );
}


void VMPITracker_End()
{
	g_WorkUnits.Purge();
	Graphical_End();
}


bool VMPITracker_WriteDebugFile( const char *pFilename )
{
	FILE *fp = fopen( pFilename, "wt" );
	if ( fp )
	{
		fprintf( fp, "# work units: %d\n", g_WorkUnits.Count() );
		fprintf( fp, "# active work units: %d\n", CountActiveWorkUnits() );
		
		fprintf( fp, "\n" );
		fprintf( fp, "--- Events ---" );
		fprintf( fp, "\n" );
		fprintf( fp, "\n" );
		
		for ( int i=0; i < g_WorkUnits.Count(); i++ )
		{
			CWorkUnit *wu = &g_WorkUnits[i];
			
			if ( wu->m_iWorkerCompleted != -1 )
				continue;

			fprintf( fp, "  work unit %d\n", i );
			fprintf( fp, "\n" );
						
			if ( wu->m_Events.Count() == 0 )
			{
				fprintf( fp, "    *no events*\n" );
			}
			else
			{
				for ( int iEvent=0; iEvent < wu->m_Events.Count(); iEvent++ )
				{
					CWorkUnitEvent *pEvent = &wu->m_Events[iEvent];
					
					if ( pEvent->m_iEventType == EVENT_TYPE_WU_STARTED )
					{
						fprintf( fp, "   started (by worker %s) %.1f seconds ago\n", 
							VMPI_GetMachineName( wu->m_Events[iEvent].m_iWorker ),
							Plat_FloatTime() - wu->m_Events[iEvent].m_flTime );
					}
					else if ( pEvent->m_iEventType == EVENT_TYPE_SEND_WORK_UNIT )
					{
						fprintf( fp, "      sent (to worker %s) %.1f seconds ago\n", 
							VMPI_GetMachineName( wu->m_Events[iEvent].m_iWorker ),
							Plat_FloatTime() - wu->m_Events[iEvent].m_flTime );
					}
					else if ( pEvent->m_iEventType == EVENT_TYPE_WU_COMPLETED )
					{
						fprintf( fp, " completed (by worker %s) %.1f seconds ago\n", 
							VMPI_GetMachineName( wu->m_Events[iEvent].m_iWorker ),
							Plat_FloatTime() - wu->m_Events[iEvent].m_flTime );
					}
				}
			}
			fprintf( fp, "\n" );
		}
		
		fclose( fp );
		return true;
	}
	else
	{
		return false;
	}	
}


void VMPITracker_HandleDebugKeypresses()
{
	if ( !g_bTrackWorkUnitEvents )
		return;
	
	if ( !kbhit() )
		return;

	static int iState = 0;

	int key = toupper( getch() );
	if ( iState == 0 )
	{
		if ( key == 'D' )
		{
			iState = 1;
			Warning("\n\n"
				"----------------------\n"
				"1. Write debug file (ascending filenames).\n"
				"2. Write debug file (c:\\vmpi_tracker_0.txt).\n"
				"3. Invite debug workers (password: 'debugworker').\n"
				"\n"
				"0. Exit menu.\n"
				"----------------------\n"
				"\n"
				);
		}
	}
	else if ( iState == 1 )
	{
		if ( key == '1' )
		{
			iState = 0;
			
			int nMaxTries = 128;
			char filename[512];
			int iFile = 1;
			for ( iFile; iFile < nMaxTries; iFile++ )
			{
				Q_snprintf( filename, sizeof( filename ), "c:\\vmpi_tracker_%d.txt", iFile );
				if ( _access( filename, 0 ) != 0 )
					break;
			}
			if ( iFile == nMaxTries )
			{
				Warning( "** Please delete c:\\vmpi_tracker_*.txt and try again.\n" );
			}
			else
			{
				if ( VMPITracker_WriteDebugFile( filename ) )
					Warning( "Wrote %s successfully.\n", filename );
				else
					Warning( "Failed to write %s successfully.\n", filename );
			}
		}
		else if ( key == '2' )
		{
			iState = 0;

			const char *filename = "c:\\vmpi_tracker_0.txt";
			if ( VMPITracker_WriteDebugFile( filename ) )
				Warning( "Wrote %s successfully.\n", filename );
			else
				Warning( "Failed to write %s successfully.\n", filename );
		}
		else if ( key == '3' )
		{
			iState = 0;
			Warning( "\nInviting debug workers with password 'debugworker'...\nGo ahead and connect them.\n" );
			VMPI_InviteDebugWorkers();
		}
		else if ( key == '0' )
		{
			iState = 0;
			Warning( "\n\nExited menu.\n\n" );
		}
	}
}


