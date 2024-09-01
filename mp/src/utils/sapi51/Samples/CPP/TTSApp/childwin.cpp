/////////////////////////////////////////////////////////////////////////
//  ChidWin.CPP
//
//  This module handles all actions that take place in the child window.
//  The child window is used for displaying the mouth bitmaps.
//
//  Revision History:
//      07/18/99    Created
//Copyright (c) Microsoft Corporation. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////

#include "globals.h"


// ---------------------------------------------------------------------------
// ChildWndProc
// ---------------------------------------------------------------------------
// Description:         Main window procedure.
// Arguments:
//  HWND [in]           Window handle.
//  UINT [in]           Message identifier.
//  WPARAM [in]         Depends on message.
//  LPARAM [in]         Depends on message.
// Returns:
//  LPARAM              Depends on message.
LPARAM CALLBACK ChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC                 hdc=NULL, hmemDC=NULL;
    PAINTSTRUCT         ps;
	HBRUSH				hbrBkGnd=NULL;
    RECT                rect;

    // Call the appropriate message handler
    switch(uMsg)                                                    
    {
    case WM_PAINT:
        {
            // Get the Display Context so we have someplace to blit.
            hdc = BeginPaint( hWnd, &ps );

		    // Create a compatible DC.
            hmemDC = CreateCompatibleDC( hdc );

            GetClientRect( hWnd, &rect );
            // Create a bitmap big enough for our client rectangle.
            HBITMAP hmemBMP = CreateCompatibleBitmap( hdc,
                                        rect.right - rect.left,
                                        rect.bottom - rect.top );

            // Select the bitmap into the off-screen DC.
            HGDIOBJ hobjOld = SelectObject( hmemDC, hmemBMP );

		    // Erase the background.
		    hbrBkGnd = CreateSolidBrush( GetSysColor(COLOR_3DFACE) );
		    FillRect( hmemDC, &rect, hbrBkGnd );
		    DeleteObject( hbrBkGnd );            

            // Draw into memory DC
            ImageList_Draw( g_hListBmp, 0, hmemDC, 0, 0, INDEXTOOVERLAYMASK(g_iBmp) ); 
            if( g_iBmp%6 == 2 )
            {
                ImageList_Draw( g_hListBmp, WEYESNAR, hmemDC, 0, 0, 0 ); 
            }
            if( g_iBmp%6 == 5 )
            {
                ImageList_Draw( g_hListBmp, WEYESCLO, hmemDC, 0, 0, 0 ); 
            }
        
        
            // Blit to window DC
            StretchBlt( hdc, 0, 0, rect.right, rect.bottom, 
                        hmemDC, 0, 0, rect.right, rect.bottom, SRCCOPY );

            // Replace the previous GDI object to the DC
            SelectObject( hmemDC, hobjOld );

            // Done with off-screen bitmap and DC.
            DeleteObject( hmemBMP );

		    DeleteDC( hmemDC );

            DeleteDC( hdc );

            // Clean up and get outta here
            EndPaint( hWnd, &ps );
        }
        break;

    case WM_DESTROY:
        // Delete Mouth Bitmaps
        ImageList_Destroy( g_hListBmp );
        break;

        
    }

    // Call the default message handler
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
