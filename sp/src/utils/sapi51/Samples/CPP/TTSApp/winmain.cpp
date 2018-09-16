#include "globals.h"

/////////////////////////////////////////////////////////////////////////
//  SAPI5 TTSAPP
//
//  Dialog box application used to manually verify SAPI5 TTS methods
//  
//
//  Revision History:
//      06/18/99    Created
//Copyright (c) Microsoft Corporation. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////

//
//  Function prototypes
//
int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow );

// ---------------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------------
// Description:         Program entry point.
// Arguments:
//  HINSTANCE [in]      Program instance handle.
//  HINSTANCE [in]      Unused in Win32.
//  LPSTR [in]          Program command line.
//  int [in]            Command to pass to ShowWindow().
// Returns:
//  int                 0 if all goes well.

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpszCmdLine, int nCmdShow )
{
    HWND                hWnd    = NULL;
    HWND                hChild  = NULL;
    WNDCLASSEX          wc;
    HRESULT             hr      = S_OK;
        
	// Initialize the Win95 control library
	InitCommonControls();

	// Load the library containing the rich edit control
	HMODULE hMod = LoadLibrary( TEXT( "riched20.dll" ) );
	if( !hMod )
	{
		MessageBox( NULL, _T("Couldn't find riched32.dll. Shutting down!"), 
					_T("Error - Missing dll"), MB_OK );
	}

	if ( hMod )
	{
	    // Initialize COM library on the current thread and identify the concurrency model as
		// single-thread apartment (STA).  Applications must initialize the COM library before they
		// can call COM library functions other than CoGetMalloc and memory allocation functions.
		// New application developers may choose to use CoInitializeEx rather than CoInitialize, allowing
		// them to set the concurrency model to apartment or multi-threaded.
	    CoInitialize( NULL );

	    // Register the child window class
	    ZeroMemory( &wc, sizeof( wc ) );
	    wc.cbSize = sizeof( wc );
	    wc.style            = CS_HREDRAW | CS_VREDRAW;
	    wc.lpfnWndProc      = ChildWndProc;
	    wc.hInstance        = hInst;
	    wc.hIcon            = LoadIcon( hInst, (LPCTSTR) IDI_APPICON );
	    wc.hCursor          = LoadCursor( NULL, IDC_CROSS );
	    wc.hbrBackground    = GetSysColorBrush( COLOR_3DFACE );
	    wc.lpszMenuName     = NULL;
	    wc.lpszClassName    = CHILD_CLASS;
	    wc.hIconSm          = LoadIcon( hInst, (LPCTSTR) IDI_APPICON );

	    if( RegisterClassEx( &wc ) )
	    {
	        CTTSApp DlgClass( hInst );
	        hr = DlgClass.InitSapi();
	        
	        if( SUCCEEDED( hr ) )
	        {
	            // Create the main dialog
	            DialogBoxParam( hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)CTTSApp::DlgProcMain, 
	                        (LPARAM)&DlgClass );
	        }
	        else
	        {
	            // Error - shut down
	            MessageBox( NULL, 
	                _T("Error initializing TTSApp. Shutting down."), 
	                _T("Error"), MB_OK );
	        }        
	    }
	
		FreeLibrary( hMod );
	}

	// Unload COM
	CoUninitialize();

	// Return 0
	return 0;
	
}
