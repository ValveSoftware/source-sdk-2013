/******************************************************************************
*           coffee.cpp
*               Contains entry point for the coffee tutorial application, as
*               well as implementation of all application features.
*
*
*           Copyright (c) Microsoft Corporation. All rights reserved.
*
*           The source code supplied here is intended as a sample, so some
*           error handling, etc. has been omitted for the sake of clarity.
******************************************************************************/
#include "stdafx.h"
#include <sphelper.h>                           // Contains definitions of SAPI functions
#include "common.h"                             // Contains common defines
#include "coffee.h"                             // Forward declarations and constants
#include "cofgram.h"                            // This header is created by the grammar
                                                // compiler and has our rule ids

/******************************************************************************
* WinMain *
*---------*
*   Description:
*       coffee entry point.
*
*   Return:
*       exit code
******************************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;

    // Register the main window class
	MyRegisterClass(hInstance, WndProc);

    // Initialize pane handler state
    g_fpCurrentPane = EntryPaneProc;

	// Only continue if COM is successfully initialized
    if ( SUCCEEDED( CoInitialize( NULL ) ) )
    {
        // Perform application initialization:
	    if (!InitInstance( hInstance, nCmdShow )) 
	    {
	    	return FALSE;
	    }

	    // Main message loop:
	    while (GetMessage(&msg, NULL, 0, 0)) 
	    {
        	TranslateMessage(&msg);
	    	DispatchMessage(&msg);
	    }
        CoUninitialize();
    }

	return msg.wParam;
}

/******************************************************************************
* WndProc *
*---------*
*   Description:
*       Main window procedure.
*
******************************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
	{
        case WM_CREATE:
            // Try to initialize, quit with error message if we can't
            if ( FAILED( InitSAPI( hWnd ) ) )
            {
                const iMaxTitleLength = 64;
                TCHAR tszBuf[ MAX_PATH ];
                LoadString( g_hInst, IDS_FAILEDINIT, tszBuf, MAX_PATH );
                TCHAR tszName[ iMaxTitleLength ];
                LoadString( g_hInst, IDS_APP_TITLE, tszName, iMaxTitleLength );

                MessageBox( hWnd, tszBuf, tszName, MB_OK|MB_ICONWARNING );
                return( -1 );
            }
            break;
            
        // This is our application defined window message to let us know that a
        // speech recognition event has occurred.
        case WM_RECOEVENT:
            ProcessRecoEvent( hWnd );
            break;

        case WM_ERASEBKGND:
            EraseBackground( (HDC) wParam );
            return ( 1 );

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMM = (LPMINMAXINFO) lParam;

            lpMM->ptMaxSize.x = MINMAX_WIDTH;
            lpMM->ptMaxSize.y = MINMAX_HEIGHT;
            lpMM->ptMinTrackSize.x = MINMAX_WIDTH;
            lpMM->ptMinTrackSize.y = MINMAX_HEIGHT;
            lpMM->ptMaxTrackSize.x = MINMAX_WIDTH;
            lpMM->ptMaxTrackSize.y = MINMAX_HEIGHT;
            return ( 0 );
        }

		// Release remaining SAPI related COM references before application exits
        case WM_DESTROY:
            KillTimer( hWnd, 0 );
            CleanupGDIObjects();
            CleanupSAPI();
			PostQuitMessage(0);
			break;

		default:
        {
            _ASSERTE( g_fpCurrentPane );
            // Send unhandled messages to pane specific procedure for potential action
            LRESULT lRet = (*g_fpCurrentPane)(hWnd, message, wParam, lParam);
            if ( 0 == lRet )
            {
			    lRet = DefWindowProc(hWnd, message, wParam, lParam);
            }
            return ( lRet );
        }
   }
   return ( 0 );
}

/******************************************************************************
* InitSAPI *
*----------*
*   Description:
*       Called once to get SAPI started.
*
******************************************************************************/
HRESULT InitSAPI( HWND hWnd )
{
    HRESULT hr = S_OK;
    CComPtr<ISpAudio> cpAudio;

    while ( 1 )
    {
        // create a recognition engine
        hr = g_cpEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
        if ( FAILED( hr ) )
        {
            break;
        }
       
        // create the command recognition context
        hr = g_cpEngine->CreateRecoContext( &g_cpRecoCtxt );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Let SR know that window we want it to send event information to, and using
        // what message
        hr = g_cpRecoCtxt->SetNotifyWindowMessage( hWnd, WM_RECOEVENT, 0, 0 );
        if ( FAILED( hr ) )
        {
            break;
        }

	    // Tell SR what types of events interest us.  Here we only care about command
        // recognition.
        hr = g_cpRecoCtxt->SetInterest( SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION) );
        if ( FAILED( hr ) )
        {
            break;
        }

        // Load our grammar, which is the compiled form of simple.xml bound into this executable as a
        // user defined ("SRGRAMMAR") resource type.
        hr = g_cpRecoCtxt->CreateGrammar(GRAMMARID1, &g_cpCmdGrammar);
        if (FAILED(hr))
        {
            break;
        }
        hr = g_cpCmdGrammar->LoadCmdFromResource(NULL, MAKEINTRESOURCEW(IDR_CMD_CFG),
                                                 L"SRGRAMMAR", MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                                 SPLO_DYNAMIC);
        if ( FAILED( hr ) )
        {
            break;
        }

        // Set rules to active, we are now listening for commands
        hr = g_cpCmdGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE );
        if ( FAILED( hr ) )
        {
            break;
        }

        break;
    }

    // if we failed and have a partially setup SAPI, close it all down
    if ( FAILED( hr ) )
    {
        CleanupSAPI();
    }

    return ( hr );
}

/******************************************************************************
* CleanupSAPI *
*----------------*
*   Description:
*       Called to close down SAPI COM objects we have stored away.
*
******************************************************************************/
void CleanupSAPI( void )
{
    // Release grammar, if loaded
    if ( g_cpCmdGrammar )
    {
        g_cpCmdGrammar.Release();
    }
    // Release recognition context, if created
    if ( g_cpRecoCtxt )
    {
        g_cpRecoCtxt->SetNotifySink(NULL);
        g_cpRecoCtxt.Release();
    }
    // Release recognition engine instance, if created
	if ( g_cpEngine )
	{
		g_cpEngine.Release();
	}
}

/******************************************************************************
* ProcessRecoEvent *
*------------------*
*   Description:
*       Called to when reco event message is sent to main window procedure.
*       In the case of a recognition, it extracts result and calls ExecuteCommand.
*
******************************************************************************/
void ProcessRecoEvent( HWND hWnd )
{
    CSpEvent event;  // Event helper class

    // Loop processing events while there are any in the queue
    while (event.GetFrom(g_cpRecoCtxt) == S_OK)
    {
        // Look at recognition event only
        switch (event.eEventId)
        {
            case SPEI_RECOGNITION:
                ExecuteCommand(event.RecoResult(), hWnd);
                break;

        }
    }
}

/******************************************************************************
* ExecuteCommand *
*----------------*
*   Description:
*       Called to Execute commands that have been identified by the speech engine.
*
******************************************************************************/
void ExecuteCommand(ISpPhrase *pPhrase, HWND hWnd)
{
    SPPHRASE *pElements;

    // Get the phrase elements, one of which is the rule id we specified in
    // the grammar.  Switch on it to figure out which command was recognized.
    if (SUCCEEDED(pPhrase->GetPhrase(&pElements)))
    {        
        switch ( pElements->Rule.ulId )
        {
            case VID_EspressoDrinks:
            {
                ID_TEXT *pulIds = new ID_TEXT[MAX_ID_ARRAY];  // This memory will be freed when the WM_ESPRESSOORDER
                                                              // message is processed
                const SPPHRASEPROPERTY *pProp = NULL;
                const SPPHRASERULE *pRule = NULL;
                ULONG ulFirstElement, ulCountOfElements;
                int iCnt = 0;

                if ( pulIds )
                {
                    ZeroMemory( pulIds, sizeof( ID_TEXT[MAX_ID_ARRAY] ) );
                    pProp = pElements->pProperties;
                    pRule = pElements->Rule.pFirstChild;
                    // Fill in an array with the drink properties received
                    while ( pProp && iCnt < MAX_ID_ARRAY )
                    {
                        // Fill out a structure with all the property ids received as well
                        // as their corresponding text
                        pulIds[iCnt].ulId = static_cast< ULONG >(pProp->pFirstChild->vValue.ulVal);
                        // Get the count of elements from the rule ref, not the actual leaf
                        // property
                        if ( pRule )
                        {
                            ulFirstElement = pRule->ulFirstElement;
                            ulCountOfElements = pRule->ulCountOfElements;
                        }
                        else
                        {
                            ulFirstElement = 0;
                            ulCountOfElements = 0;
                        }
                        // This is the text corresponding to property iCnt - it must be
                        // released when we are done with it
                        pPhrase->GetText( ulFirstElement, ulCountOfElements,
                                          FALSE, &(pulIds[iCnt].pwstrCoMemText), NULL);
                        // Loop through all properties
                        pProp = pProp->pNextSibling;
                        // Loop through rulerefs corresponding to properties
                        if ( pRule )
                        {
                            pRule = pRule->pNextSibling;
                        }
                        iCnt++;
                    }
                    PostMessage( hWnd, WM_ESPRESSOORDER, NULL, (LPARAM) pulIds );                   
                }
            }
            break;
            case VID_Navigation:
            {
                switch( pElements->pProperties->vValue.ulVal )
                {
                    case VID_Counter:
                        PostMessage( hWnd, WM_GOTOCOUNTER, NULL, NULL );                        
                    break;
                }
            }
            break;
        }
        // Free the pElements memory which was allocated for us
        ::CoTaskMemFree(pElements);
    }

}

/******************************************************************************
* EntryPaneProc *
*---------------*
*   Description:
*       Handles messages specifically for the entry pane.
*
******************************************************************************/
LRESULT EntryPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
        case WM_GOTOCOUNTER:
            // Set the right message handler and repaint
            g_fpCurrentPane = CounterPaneProc;
            PostMessage( hWnd, WM_INITPANE, NULL, NULL );
            InvalidateRect( hWnd, NULL, TRUE );
            return ( 1 );

        case WM_PAINT:
            EntryPanePaint( hWnd );
            return ( 1 );
    }
    return ( 0 );

}

/******************************************************************************
* CounterPaneProc *
*-----------------*
*   Description:
*       Handles messages specifically for the counter (order) pane.
*
******************************************************************************/
LRESULT CounterPaneProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    USES_CONVERSION;
    
    switch ( message )
    {
        case WM_ESPRESSOORDER:
        {
            _ASSERTE( lParam );
            KillTimer( hWnd, 0 );
            ID_TEXT *pulIds = (ID_TEXT *) lParam;
            int i = 0, ilen = 0;
            TCHAR szTempBuf[NORMAL_LOADSTRING];
            TCHAR szSpace[] =  _T(" ");
            int iTemplen;
            
            g_szCounterDisplay[0] = '\0';

            // Sort the array
            while ( 0 != pulIds[i].ulId )
            {
                i++;
            }
            for ( int j = 0; j < i; j++ )
            {
                int iminIndex = j;               
                for ( int k = j; k < i; k++ )
                {
                    if ( pulIds[iminIndex].ulId > pulIds[k].ulId )
                    {
                        iminIndex = k;
                    }
                }
                ULONG ulId = pulIds[iminIndex].ulId;
                WCHAR *pwstr = pulIds[iminIndex].pwstrCoMemText;
                pulIds[iminIndex].pwstrCoMemText = pulIds[j].pwstrCoMemText;
                pulIds[j].pwstrCoMemText = pwstr;
                pulIds[iminIndex].ulId = pulIds[j].ulId;
                pulIds[j].ulId = ulId;
            }
            
            i = 0;
            // Put in the first order words if we actually have an order
            if ( 0 != pulIds[0].ulId )
            {
                iTemplen = LoadString( g_hInst, IDS_ORDERBEGIN, szTempBuf, NORMAL_LOADSTRING );                
                lstrcat( g_szCounterDisplay + ilen, szTempBuf );
                ilen += iTemplen;
            }
            while ( i < MAX_ID_ARRAY && 0 != pulIds[i].ulId )
            {
                TCHAR *pTempStr = W2T( pulIds[i].pwstrCoMemText );

                iTemplen = lstrlen( pTempStr );
                // We'll quit now so we dont overrun the buffer
                if ( ilen + iTemplen >= MAX_LOADSTRING )
                {
                    break;
                }
                if ( i > 0 )
                {
                    lstrcat( g_szCounterDisplay + ilen, szSpace );
                    ilen += 1;
                }
                lstrcat( g_szCounterDisplay, pTempStr );
                ilen += iTemplen;
                i++;
            }
            // Put the thank you on this order
            if ( 0 < i )
            {
                iTemplen = LoadString( g_hInst, IDS_ORDEREND, szTempBuf, NORMAL_LOADSTRING );                
                if ( ilen + iTemplen < MAX_LOADSTRING )
                {
                    lstrcat( g_szCounterDisplay + ilen, szTempBuf );
                    ilen += iTemplen;
                }
            }

            InvalidateRect( hWnd, NULL, TRUE );
            SetTimer( hWnd, 0, TIMEOUT, NULL );

            // Delete the CoTaskMem we were given initially by ISpPhrase->GetText
            i = 0;
            while ( i < MAX_ID_ARRAY && 0 != pulIds[i].ulId )
            {
                CoTaskMemFree( pulIds[i].pwstrCoMemText );
                i++;
            }
            delete [] pulIds;
            return ( 1 );
        }
        case WM_PAINT:
            CounterPanePaint( hWnd, g_szCounterDisplay );
            return ( 1 );

        case WM_INITPANE:
            LoadString( g_hInst, IDS_PLEASEORDER, g_szCounterDisplay, MAX_LOADSTRING );
            return ( 1 );

        case WM_TIMER:
            // Revert back to 'go ahead and order' message
            LoadString( g_hInst, IDS_PLEASEORDER, g_szCounterDisplay, MAX_LOADSTRING );
            InvalidateRect( hWnd, NULL, TRUE );
            KillTimer( hWnd, 0 );
            return ( 1 );
    }
    return ( 0 );
}
