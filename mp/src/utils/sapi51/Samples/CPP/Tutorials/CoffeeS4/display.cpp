/******************************************************************************
*   Display.cpp 
*       This module contains the UI specifc code for the coffee application
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "sphelper.h"
#include "resource.h"
#include "common.h"

// Static variables for this module
static HBITMAP      s_hBmp;         // Handle to background bitmap
static HBRUSH       s_hBackBrush;   // Pointer to background brush
static HFONT        s_hDrawingFont; // Pointer to our font

// Shared variables
extern HINSTANCE    g_hInst;		// current instance

/******************************************************************************
* MyRegisterClass *
*-----------------*
*   Description:
*       Register our window class.
*
******************************************************************************/
ATOM MyRegisterClass(HINSTANCE hInstance, WNDPROC WndProc)
{
	WNDCLASSEX wcex;
    TCHAR szWindowClass[NORMAL_LOADSTRING];

    s_hBmp = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP1 ) );
    s_hDrawingFont = NULL;
	LoadString(hInstance, IDC_COFFEE, szWindowClass, NORMAL_LOADSTRING);

    wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_COFFEE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

/******************************************************************************
* InitInstance *
*--------------*
*   Description:
*       Save the instance handle in a global variable and create and display 
*       the main program window.
*
******************************************************************************/
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;
    TCHAR szTitle[NORMAL_LOADSTRING];
    TCHAR szWindowClass[NORMAL_LOADSTRING];

    g_hInst = hInstance; 

	// Initialize label strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, NORMAL_LOADSTRING);
	LoadString(hInstance, IDC_COFFEE, szWindowClass, NORMAL_LOADSTRING);

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, MINMAX_WIDTH, MINMAX_HEIGHT, NULL, NULL, hInstance, NULL);
    
    if (!hWnd)
    {
        return FALSE;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    return TRUE;
}

/******************************************************************************
* Erase Background *
*------------------*
*   Description:
*       Erase the screen and paint with background bitmap.
*
******************************************************************************/
void EraseBackground( HDC hDC )
{
    HDC hMemDC = CreateCompatibleDC( hDC );
    HBITMAP hOldBmp = (HBITMAP) SelectObject( hMemDC, s_hBmp );
    int i = 0;
    int j = 0;
    
    while ( i < MINMAX_WIDTH )
    {
        j = 0;
        while ( j  < MINMAX_HEIGHT )
        {
            BitBlt( hDC, i, j, i + 128, j + 128, hMemDC, 0, 0, SRCCOPY );
            j += 128;
        }
        i += 128;
    }
    
    if ( !s_hDrawingFont )
    {
        LOGFONT lf;
        
        lf.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);; 
        lf.lfWidth = 0; 
        lf.lfEscapement = 0; 
        lf.lfOrientation = 0; 
        lf.lfWeight = 0; 
        lf.lfItalic = 0; 
        lf.lfUnderline = 0; 
        lf.lfStrikeOut = 0; 
        lf.lfCharSet = DEFAULT_CHARSET; 
        lf.lfOutPrecision = OUT_TT_ONLY_PRECIS; 
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS; 
        lf.lfQuality = DEFAULT_QUALITY; 
        lf.lfPitchAndFamily = VARIABLE_PITCH | FF_MODERN; 
        _tcscpy( lf.lfFaceName, _T("Kristen ITC") );
        s_hDrawingFont = CreateFontIndirect( &lf );
        if (!s_hDrawingFont )
        {
            s_hDrawingFont = (HFONT) GetStockObject( DEFAULT_GUI_FONT );
        }
        
    }
    SelectObject( hMemDC, hOldBmp );
    DeleteDC( hMemDC );
}

/******************************************************************************
* CleanupGDIObjects *
*-------------------*
*   Description:
*       Cleanup any GDI objects we may have created.
*
******************************************************************************/
void CleanupGDIObjects( void )
{
    if ( s_hDrawingFont )
    {
        DeleteObject( s_hDrawingFont );
    }
    if ( s_hBmp )
    {
        DeleteObject( s_hBmp );
    }
}

/******************************************************************************
* EntryPanePaint *
*----------------*
*   Description:
*       Do the paint on the entry pane.
*
******************************************************************************/
void EntryPanePaint( HWND hWnd )
{
    if ( GetUpdateRect( hWnd, NULL, TRUE ) )
    {
        PAINTSTRUCT ps;
        HDC hDC;
        TCHAR tBuf[MAX_LOADSTRING];
        BeginPaint( hWnd, &ps );
        hDC = ps.hdc;
        HFONT hOldFont = (HFONT) SelectObject( hDC, s_hDrawingFont );
        COLORREF sOldColor = SetTextColor( hDC, RGB( 255, 255, 255 ) );
        SetBkMode(hDC, TRANSPARENT);
        
        RECT rc;
        RECT clientRC;
        GetClientRect( hWnd, &clientRC );
        
        rc.left = 0;
        rc.right = 100;
        LoadString( g_hInst, IDS_ENTERSTORE, tBuf, MAX_LOADSTRING );
        int iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        rc.left += 25;
        rc.right += 25;
        rc.top = ( clientRC.bottom - iHeight ) / 2;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );
        
        rc.left = 0;
        rc.right = 100;
        LoadString( g_hInst, IDS_ENTEROFFICE, tBuf, MAX_LOADSTRING );
        iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        rc.left = clientRC.right - 125;
        rc.right = rc.left + 100;
        rc.top = ( clientRC.bottom - iHeight ) / 2;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );

        LoadString( g_hInst, IDS_WELCOME, tBuf, MAX_LOADSTRING );
        rc.left = 0;
        rc.right = 450;
        iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        int iWidth = rc.right - rc.left;
        rc.left = (clientRC.right - iWidth) / 2;
        rc.right = rc.left + iWidth;
        rc.top = 25;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );
        
        SetTextColor( hDC, sOldColor );
        SelectObject( hDC, hOldFont );
        EndPaint( hWnd, &ps );
    }
}

/******************************************************************************
* CounterPanePaint *
*------------------*
*   Description:
*       Do the paint on the counter pane.
*
******************************************************************************/
void CounterPanePaint( HWND hWnd, LPCTSTR szCounterDisplay )
{
    if ( GetUpdateRect( hWnd, NULL, TRUE ) )
    {
        PAINTSTRUCT ps;
        HDC hDC;
        BeginPaint( hWnd, &ps );
        hDC = ps.hdc;
        HFONT hOldFont = (HFONT) SelectObject( hDC, s_hDrawingFont );
        COLORREF sOldColor = SetTextColor( hDC, RGB( 255, 255, 255 ) );
        SetBkMode(hDC, TRANSPARENT);
        
        RECT rc;
        RECT clientRC;
        GetClientRect( hWnd, &clientRC );
        
        rc.left = 0;
        rc.right = 450;
        int iHeight = DrawText( hDC, szCounterDisplay, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        int iWidth = rc.right - rc.left;
        rc.left = (clientRC.right - iWidth) / 2;
        rc.right = rc.left + iWidth;
        rc.top = 100;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, szCounterDisplay, -1, &rc, DT_WORDBREAK );
        
        SetTextColor( hDC, sOldColor );
        SelectObject( hDC, hOldFont );
        EndPaint( hWnd, &ps );
    }

}

/******************************************************************************
* OfficePanePaint  *
*------------------*
*   Description:
*       Do the paint on the office pane.
*
******************************************************************************/
void OfficePanePaint( HWND hWnd )
{
    if ( GetUpdateRect( hWnd, NULL, TRUE ) )
    {
        PAINTSTRUCT ps;
        HDC hDC;
        TCHAR tBuf[MAX_LOADSTRING];
        BeginPaint( hWnd, &ps );
        hDC = ps.hdc;
        HFONT hOldFont = (HFONT) SelectObject( hDC, s_hDrawingFont );
        COLORREF sOldColor = SetTextColor( hDC, RGB( 255, 255, 255 ) );
        SetBkMode(hDC, TRANSPARENT);
        
        RECT rc;
        RECT clientRC;
        GetClientRect( hWnd, &clientRC );
        
        LoadString( g_hInst, IDS_OFFICE, tBuf, MAX_LOADSTRING );
        rc.left = 0;
        rc.right = 450;
        int iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        int iWidth = rc.right - rc.left;
        rc.left = (clientRC.right - iWidth) / 2;
        rc.right = rc.left + iWidth;
        rc.top = 25;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );
        
        rc.left = 0;
        rc.right = 100;
        LoadString( g_hInst, IDS_MANAGEEMPLOYEES, tBuf, MAX_LOADSTRING );
        iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        rc.left = clientRC.right - 125;
        rc.right = rc.left + 100;
        rc.top = ( clientRC.bottom - iHeight ) / 2;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );

        SetTextColor( hDC, sOldColor );
        SelectObject( hDC, hOldFont );
        EndPaint( hWnd, &ps );
    }        
}

/******************************************************************************
* ManageEmployeesPanePaint  *
*---------------------------*
*   Description:
*       Do the paint on the manage employees pane.
*
******************************************************************************/
void ManageEmployeesPanePaint( HWND hWnd, ULONG ulNumTokens, CSpDynamicString* ppcDesciptionString,
                               ULONG ulCurToken)
{
    USES_CONVERSION;
    
    if ( GetUpdateRect( hWnd, NULL, TRUE ) )
    {
        PAINTSTRUCT ps;
        HDC hDC;
        TCHAR tBuf[MAX_LOADSTRING];
        BeginPaint( hWnd, &ps );
        hDC = ps.hdc;
        HFONT hOldFont = (HFONT) SelectObject( hDC, s_hDrawingFont );
        COLORREF sOldColor = SetTextColor( hDC, RGB( 255, 255, 255 ) );
        SetBkMode(hDC, TRANSPARENT);
        
        RECT rc;
        RECT clientRC;
        GetClientRect( hWnd, &clientRC );
        
        LoadString( g_hInst, IDS_MANAGEEMPLOYEES, tBuf, MAX_LOADSTRING );
        rc.left = 0;
        rc.right = 450;
        int iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        int iWidth = rc.right - rc.left;
        rc.left = (clientRC.right - iWidth) / 2;
        rc.right = rc.left + iWidth;
        rc.top = 25;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );
        
        rc.left = 0;
        rc.right = 200;
        LoadString( g_hInst, IDS_EMPLOYEEDUTY, tBuf, MAX_LOADSTRING );
        iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        rc.left = clientRC.right - 275;
        rc.right = rc.left + 200;
        rc.top = ( clientRC.bottom - iHeight ) / 4;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );

        if ( 8 < ulNumTokens )
        {
            ulNumTokens = 8;
        }
        ULONG ulIndex = 0;
        rc.bottom +=5;
        while( ulIndex < ulNumTokens )
        {
            TCHAR * pszTitle = W2T( ppcDesciptionString[ulIndex] );
            LONG lLastBottom = rc.bottom;
            rc.left = 0;
            rc.right = 200;
            iHeight = DrawText( hDC, pszTitle, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
            rc.left = clientRC.right - 275;
            rc.right = rc.left + 200;
            rc.top = lLastBottom + 4;
            rc.bottom = rc.top + iHeight + 1;
            if ( ulIndex != ulCurToken )
            {
                DrawText( hDC, pszTitle, -1, &rc, DT_WORDBREAK );
            }
            else
            {
                COLORREF clrTmp = SetTextColor( hDC, RGB(255, 0, 0) );
                DrawText( hDC, pszTitle, -1, &rc, DT_WORDBREAK );
                SetTextColor( hDC, clrTmp );
            }

            ulIndex++;          

        }

        LoadString( g_hInst, IDS_HEARTHEM, tBuf, MAX_LOADSTRING );
        rc.left = 0;
        rc.right = 450;
        iHeight = DrawText( hDC, tBuf, -1, &rc, DT_CALCRECT | DT_WORDBREAK );
        iWidth = rc.right - rc.left;
        rc.left = (clientRC.right - iWidth) / 2;
        rc.right = rc.left + iWidth;
        rc.top = ( clientRC.bottom - iHeight ) - 100;
        rc.bottom = rc.top + iHeight + 1;
        DrawText( hDC, tBuf, -1, &rc, DT_WORDBREAK );

        SetTextColor( hDC, sOldColor );
        SelectObject( hDC, hOldFont );
        EndPaint( hWnd, &ps );
    }        
}
