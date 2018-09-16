/////////////////////////////////////////////////////////////////////////
//  DlgMain.CPP
//
//  This module handles the main dialog box, and it's controls.
//
//  The main dialog is actually the application's main window class, there
//  is no conventional "Window class", only this dialog class which is
//  registered in WinMain.
//
//  Revision History:
//      06/06/99    Created
//Copyright (c) Microsoft Corporation. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////

#include "globals.h"

//
// Other local functions
//
inline void TTSAppStatusMessage( HWND hWnd, TCHAR* szMessage );

// ---------------------------------------------------------------------------
// CTTSApp::DlgProcMain
// ---------------------------------------------------------------------------
// Description:         Main window procedure.
// Arguments:
//  HWND [in]           Window handle.
//  UINT [in]           Message identifier.
//  WPARAM [in]         Depends on message.
//  LPARAM [in]         Depends on message.
// Returns:
//  LPARAM              Depends on message.
LRESULT CALLBACK CTTSApp::DlgProcMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTTSApp* pThis = (CTTSApp *)::GetWindowLong( hwnd, GWL_USERDATA );

    // Call the appropriate function to handle the messages
    switch(uMsg)                                                    
    {
        case WM_INITDIALOG:
            ::SetWindowLong( hwnd, GWL_USERDATA, lParam );
            pThis = (CTTSApp *)lParam;
            return pThis->OnInitDialog( hwnd );

        case WM_TTSAPPCUSTOMEVENT:
            pThis->MainHandleSynthEvent();
            return TRUE;

        case WM_HSCROLL:
            pThis->HandleScroll( (HWND) lParam );
            return TRUE;

        case WM_COMMAND:
            if( pThis )
            {
                pThis->MainHandleCommand( (int)(LOWORD(wParam)), (HWND)lParam, 
                                 (UINT)HIWORD(wParam) );
            }
            return TRUE;

		case WM_CLOSE:
            pThis->MainHandleClose();
			return TRUE;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////
HRESULT CTTSApp::InitSapi()
/////////////////////////////////////////////////////////////////
{
    HRESULT                 hr;
    
    hr = m_cpVoice.CoCreateInstance( CLSID_SpVoice );

    return hr;
}

/////////////////////////////////////////////////////////////////
HIMAGELIST CTTSApp::InitImageList()
/////////////////////////////////////////////////////////////////
{
    HIMAGELIST hListBmp;
    HBITMAP hBmp;

    hListBmp = ImageList_Create( CHARACTER_WIDTH, CHARACTER_HEIGHT, ILC_COLOR32 | ILC_MASK, 1, 0 );
    if (hListBmp)
    {
        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICFULL ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH2 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH3 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH4 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH5 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH6 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH7 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH8 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH9 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH10 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH11 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH12 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICMOUTH13 ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICEYESNAR ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        hBmp = LoadBitmap( m_hInst, MAKEINTRESOURCE( IDB_MICEYESCLO ) );
        ImageList_AddMasked( hListBmp, hBmp, 0xff00ff );
        DeleteObject( hBmp );

        ImageList_SetOverlayImage( hListBmp, 1, 1 );
        ImageList_SetOverlayImage( hListBmp, 2, 2 );
        ImageList_SetOverlayImage( hListBmp, 3, 3 );
        ImageList_SetOverlayImage( hListBmp, 4, 4 );
        ImageList_SetOverlayImage( hListBmp, 5, 5 );
        ImageList_SetOverlayImage( hListBmp, 6, 6 );
        ImageList_SetOverlayImage( hListBmp, 7, 7 );
        ImageList_SetOverlayImage( hListBmp, 8, 8 );
        ImageList_SetOverlayImage( hListBmp, 9, 9 );
        ImageList_SetOverlayImage( hListBmp, 10, 10 );
        ImageList_SetOverlayImage( hListBmp, 11, 11 );
        ImageList_SetOverlayImage( hListBmp, 12, 12 );
        ImageList_SetOverlayImage( hListBmp, 13, 13 );
        ImageList_SetOverlayImage( hListBmp, 14, WEYESNAR );
        ImageList_SetOverlayImage( hListBmp, 15, WEYESCLO );
    }
    return hListBmp;
}

/////////////////////////////////////////////////////////////////
BOOL CTTSApp::OnInitDialog( HWND hWnd )
/////////////////////////////////////////////////////////////////
{
    HRESULT                         hr          = S_OK;
        
    // Store this as the "Main Dialog"
    m_hWnd  = hWnd;

    // Add some default text to the main edit control
    SetDlgItemText( hWnd, IDE_EDITBOX, _T("Enter text you wish spoken here.") );

    // Set the event mask in the rich edit control so that it notifies us when text is
    // changed in the control
    SendMessage( GetDlgItem( hWnd, IDE_EDITBOX ), EM_SETEVENTMASK, 0, ENM_CHANGE );

    // Initialize the Output Format combo box
    for( int i=0; i<NUM_OUTPUTFORMATS; i++ )
    {
        SendDlgItemMessage( hWnd, IDC_COMBO_OUTPUT, CB_ADDSTRING, 0,
                    (LPARAM)g_aszOutputFormat[i] );

        SendDlgItemMessage( hWnd, IDC_COMBO_OUTPUT, CB_SETITEMDATA, i, 
                    (LPARAM)g_aOutputFormat[i] );
    }

    if ( !m_cpVoice )
    {
        hr = E_FAIL;
    }

    // Set the default output format as the current selection.
    if( SUCCEEDED( hr ) )
    {
        CComPtr<ISpStreamFormat> cpStream;
        HRESULT hrOutputStream = m_cpVoice->GetOutputStream(&cpStream);

        if (hrOutputStream == S_OK)
        {
            CSpStreamFormat Fmt;
            hr = Fmt.AssignFormat(cpStream);
            if (SUCCEEDED(hr))
            {
                SPSTREAMFORMAT eFmt = Fmt.ComputeFormatEnum();
                for( i=0; i<NUM_OUTPUTFORMATS; i++ )
                {
                    if( g_aOutputFormat[i] == eFmt )
                    {
                        m_DefaultFormatIndex = i;
                        SendDlgItemMessage( hWnd, IDC_COMBO_OUTPUT, CB_SETCURSEL, m_DefaultFormatIndex, 0 );
                    }
                }
            }
        }
        else
        {
            SendDlgItemMessage( hWnd, IDC_COMBO_OUTPUT, CB_SETCURSEL, 0, 0 );
        }
    }

    // Use the SAPI5 helper function in sphelper.h to initialize the Voice combo box.
    if ( SUCCEEDED( hr ) )
    {
        hr = SpInitTokenComboBox( GetDlgItem( hWnd, IDC_COMBO_VOICES ), SPCAT_VOICES );
    }
    
	if ( SUCCEEDED( hr ) )
	{
		SpCreateDefaultObjectFromCategoryId( SPCAT_AUDIOOUT, &m_cpOutAudio );
	}

    // Set default voice data 
    VoiceChange();

    // Set Range for Skip Edit Box...
    SendDlgItemMessage( hWnd, IDC_SKIP_SPIN, UDM_SETRANGE, TRUE, MAKELONG( 50, -50 ) );

	// Set the notification message for the voice
	if ( SUCCEEDED( hr ) )
	{
		m_cpVoice->SetNotifyWindowMessage( hWnd, WM_TTSAPPCUSTOMEVENT, 0, 0 );
	}

    // We're interested in all TTS events
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice->SetInterest( SPFEI_ALL_TTS_EVENTS, SPFEI_ALL_TTS_EVENTS );
    }

    // Get default rate and volume
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice->GetRate( &m_DefaultRate );
        // initialize sliders and edit boxes with default rate
		if ( SUCCEEDED( hr ) )
		{
        	SendDlgItemMessage( hWnd, IDC_RATE_SLIDER, TBM_SETRANGE, TRUE, MAKELONG( SPMIN_RATE, SPMAX_RATE ) );
        	SendDlgItemMessage( hWnd, IDC_RATE_SLIDER, TBM_SETPOS, TRUE, m_DefaultRate );
        	SendDlgItemMessage( hWnd, IDC_RATE_SLIDER, TBM_SETPAGESIZE, TRUE, 5 );
		}
    }

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice->GetVolume( &m_DefaultVolume );
        // initialize sliders and edit boxes with default volume
		if ( SUCCEEDED( hr ) )
		{
	        SendDlgItemMessage( hWnd, IDC_VOLUME_SLIDER, TBM_SETRANGE, TRUE, MAKELONG( SPMIN_VOLUME, SPMAX_VOLUME ) );
	        SendDlgItemMessage( hWnd, IDC_VOLUME_SLIDER, TBM_SETPOS, TRUE, m_DefaultVolume );
	        SendDlgItemMessage( hWnd, IDC_VOLUME_SLIDER, TBM_SETPAGESIZE, TRUE, 10 );
		}
    }

    // If any SAPI initialization failed, shut down!
    if( FAILED( hr ) )
    {
        MessageBox( NULL, _T("Error initializing speech objects. Shutting down."), _T("Error"), MB_OK );
        SendMessage( hWnd, WM_CLOSE, 0, 0 );
		return(FALSE);
		
    }
    else
    {
        //
        // Create the child windows to which we'll blit our result
        //
        HWND hCharWnd = GetDlgItem(hWnd, IDC_CHARACTER);
        RECT rc;

        GetClientRect(hCharWnd, &rc);
        rc.left = (rc.right - CHARACTER_WIDTH) / 2;
        rc.top = (rc.bottom - CHARACTER_HEIGHT) / 2;
        m_hChildWnd = CreateWindow( CHILD_CLASS, NULL, 
                            WS_CHILDWINDOW | WS_VISIBLE,
                            rc.left, rc.top,
                            rc.left + CHARACTER_WIDTH, rc.top + CHARACTER_HEIGHT,
                            hCharWnd, NULL, m_hInst, NULL );

        if ( !m_hChildWnd )
        {
            MessageBox( hWnd, _T("Error initializing speech objects. Shutting down."), _T("Error"), MB_OK );
            SendMessage( hWnd, WM_CLOSE, 0, 0 );
			return(FALSE);
			
        }
        else
        {
            // Load Mouth Bitmaps and use and ImageList since we'll blit the mouth
            // and eye positions over top of the full image
            g_hListBmp = InitImageList();
        }
    }
	return(TRUE);
	
}

/////////////////////////////////////////////////////////////////
void CTTSApp::Stop()
/////////////////////////////////////////////////////////////////
// 
// Resets global audio state to stopped, updates the Pause/Resume button
// and repaints the mouth in a closed position
//
{
    // Stop current rendering with a PURGEBEFORESPEAK...
    HRESULT hr = m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, 0 );

    if( FAILED( hr ) )
    {
    	TTSAppStatusMessage( m_hWnd, _T("Stop error\r\n") );
    }

    SetWindowText( GetDlgItem( m_hWnd, IDB_PAUSE ), _T("Pause") );
    m_bPause = FALSE;
    m_bStop = TRUE;             
    // Mouth closed
    g_iBmp = 0;
    SendMessage( m_hChildWnd, WM_PAINT, 0, 0 );
    InvalidateRect( m_hChildWnd, NULL, FALSE );
}

/////////////////////////////////////////////////////////////////
void CTTSApp::MainHandleCommand( int id, HWND hWndControl, UINT codeNotify )
/////////////////////////////////////////////////////////////////
//
// Handle each of the WM_COMMAND messages that come in, and deal with
// them appropriately
//
{
    UINT                cNumChar = 0;
    HRESULT             hr = S_OK;
    TCHAR               szAFileName[NORM_SIZE] = _T("");
	static BOOL			bIsUnicode = FALSE;
    BOOL                bWavFileOpened = FALSE;
    int                 iFormat;
    CComPtr<ISpStream>  cpWavStream;
    CComPtr<ISpStreamFormat>    cpOldStream;
    HWND                hwndEdit;
	BOOL                bFileOpened = FALSE;

    // Get handle to the main edit box
    hwndEdit = GetDlgItem( m_hWnd, IDE_EDITBOX );

    switch(id)
    {
		// About Box display
	    case IDC_ABOUT:
            ::DialogBox( m_hInst, (LPCTSTR)IDD_ABOUT, m_hWnd, (DLGPROC)About );
			break;

        // Any change to voices is sent to VoiceChange() function
        case IDC_COMBO_VOICES:
            if( codeNotify == CBN_SELCHANGE )
            {
                hr = VoiceChange();
            }

            if( FAILED( hr ) )
            {
            	TTSAppStatusMessage( m_hWnd, _T("Error changing voices\r\n") );
            }

            break;

        // If user wants to speak a file pop the standard windows open file
        // dialog box and load the text into a global buffer (m_pszwFileText)
        // which will be used when the user hits speak.
        case IDB_OPEN:
            bFileOpened = CallOpenFileDialog( szAFileName,
                        _T("TXT (*.txt)\0*.txt\0XML (*.xml)\0*.xml\0All Files (*.*)\0*.*\0") );
            if( bFileOpened )
            {
				DWORD	dwFileSize = 0;
				
                USES_CONVERSION;
                wcscpy( m_szWFileName, T2W( szAFileName ) );
				ReadTheFile( szAFileName, &bIsUnicode, &m_pszwFileText );
				
				if( bIsUnicode )
				{
					// Unicode source
					UpdateEditCtlW( m_pszwFileText );
				}
				else
				{
					// MBCS source
#ifdef _UNICODE
					TCHAR *pszFileText = _tcsdup( m_pszwFileText );
#else
					// We're compiling ANSI, so we need to convert the string to MBCS
					// Note that a W2T may not be good here, since this string might 
					// be very big
                    TCHAR *pszFileText = NULL;
                    int iNeeded = ::WideCharToMultiByte( CP_ACP, 0, m_pszwFileText, -1, NULL, 0, NULL, NULL );
                    pszFileText = (TCHAR *) ::malloc( sizeof( TCHAR ) * ( iNeeded + 1 ) );
                    ::WideCharToMultiByte( CP_ACP, 0, m_pszwFileText, -1, pszFileText, iNeeded + 1, NULL, NULL );
#endif
                    if ( pszFileText )
                    {
					    SetDlgItemText( m_hWnd, IDE_EDITBOX, pszFileText );
                        free( pszFileText );
                    }

				}
            }
            else
            {
                wcscpy( m_szWFileName, L"" );
            }
            // Always SetFocus back to main edit window so text highlighting will work
            SetFocus( hwndEdit );
            break;
        
        // Handle speak
        case IDB_SPEAK:
			HandleSpeak();
			break;

        case IDB_PAUSE:
            if( !m_bStop )
            {
                if( !m_bPause )
                {
                    SetWindowText( GetDlgItem( m_hWnd, IDB_PAUSE ), _T("Resume") );
                    // Pause the voice...
                    m_cpVoice->Pause();
                    m_bPause = TRUE;
                    TTSAppStatusMessage( m_hWnd, _T("Pause\r\n") );
                }
                else
                {
                    SetWindowText( GetDlgItem( m_hWnd, IDB_PAUSE ), _T("Pause") );
                    m_cpVoice->Resume();
                    m_bPause = FALSE;
                }
            }
            SetFocus( hwndEdit );
            break;

        case IDB_STOP:
            TTSAppStatusMessage( m_hWnd, _T("Stop\r\n") );
            // Set the global audio state to stop
            Stop();
            SetFocus( hwndEdit );
            break;

        case IDB_SKIP:
            {
                SetFocus( hwndEdit );
                int fSuccess = false;
                int SkipNum = GetDlgItemInt( m_hWnd, IDC_SKIP_EDIT, &fSuccess, true );
                ULONG ulGarbage = 0;
                WCHAR szGarbage[] = L"Sentence";
                if ( fSuccess )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Skip\r\n") );
                    hr = m_cpVoice->Skip( szGarbage, SkipNum, &ulGarbage );
                }
                else
                {
                    TTSAppStatusMessage( m_hWnd, _T("Skip failed\r\n") );
                }
                break;
            }

        case IDE_EDITBOX:
            // Set the global audio state to stop if user has changed contents of edit control
            if( codeNotify == EN_CHANGE )
            {
                Stop();
            }
            break;

        case IDB_SPEAKWAV:
            bWavFileOpened = CallOpenFileDialog( szAFileName,
                         _T("WAV (*.wav)\0*.wav\0All Files (*.*)\0*.*\0") );
            // Speak the wav file using SpeakStream
            if( bWavFileOpened )
            {
                CComPtr<ISpStream>       cpWavStream;
                WCHAR                       szwWavFileName[NORM_SIZE] = L"";;

                USES_CONVERSION;
                wcscpy( szwWavFileName, T2W( szAFileName ) );

                // User helper function found in sphelper.h to open the wav file and
                // get back an IStream pointer to pass to SpeakStream
                hr = SPBindToFile( szwWavFileName, SPFM_OPEN_READONLY, &cpWavStream );

                if( SUCCEEDED( hr ) )
                {
                    hr = m_cpVoice->SpeakStream( cpWavStream, SPF_ASYNC, NULL );
                }

                if( FAILED( hr ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Speak error\r\n") );
                }
            }
            break;

        // Reset all values to defaults
        case IDB_RESET:
            TTSAppStatusMessage( m_hWnd, _T("Reset\r\n") );
            SendDlgItemMessage( m_hWnd, IDC_VOLUME_SLIDER, TBM_SETPOS, TRUE, m_DefaultVolume );
            SendDlgItemMessage( m_hWnd, IDC_RATE_SLIDER, TBM_SETPOS, TRUE, m_DefaultRate );
            SendDlgItemMessage( m_hWnd, IDC_SAVETOWAV, BM_SETCHECK, BST_UNCHECKED, 0 );
            SendDlgItemMessage( m_hWnd, IDC_EVENTS, BM_SETCHECK, BST_UNCHECKED, 0 );
            SetDlgItemText( m_hWnd, IDE_EDITBOX, _T("Enter text you wish spoken here.") );

            // reset output format
            SendDlgItemMessage( m_hWnd, IDC_COMBO_OUTPUT, CB_SETCURSEL, m_DefaultFormatIndex, 0 );
            SendMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(IDC_COMBO_OUTPUT, CBN_SELCHANGE), 0 );

            // Change the volume and the rate to reflect what the UI says
            HandleScroll( ::GetDlgItem( m_hWnd, IDC_VOLUME_SLIDER ) );
            HandleScroll( ::GetDlgItem( m_hWnd, IDC_RATE_SLIDER ) );

            SetFocus( hwndEdit );
            break;

        case IDC_COMBO_OUTPUT:
            if( codeNotify == CBN_SELCHANGE )
            {
                // Get the audio output format and set it's GUID
                iFormat  = SendDlgItemMessage( m_hWnd, IDC_COMBO_OUTPUT, CB_GETCURSEL, 0, 0 );
                SPSTREAMFORMAT eFmt = (SPSTREAMFORMAT)SendDlgItemMessage( m_hWnd, IDC_COMBO_OUTPUT,
                                                        CB_GETITEMDATA, iFormat, 0 );
                CSpStreamFormat Fmt;
                Fmt.AssignFormat(eFmt);
				if ( m_cpOutAudio )
				{
					hr = m_cpOutAudio->SetFormat( Fmt.FormatId(), Fmt.WaveFormatExPtr() );
				}
				else
				{
					hr = E_FAIL;
				}

                if( SUCCEEDED( hr ) )
                {
                    hr = m_cpVoice->SetOutput( m_cpOutAudio, FALSE );
                }

                if( FAILED( hr ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Format rejected\r\n") );
                }

                EnableSpeakButtons( SUCCEEDED( hr ) );
            }
            break;

        case IDC_SAVETOWAV:
		{
			USES_CONVERSION;

			TCHAR szFileName[256];
			_tcscpy(szFileName, _T("\0"));

			BOOL bFileOpened = CallSaveFileDialog( szFileName,
                        _T("WAV (*.wav)\0*.wav\0All Files (*.*)\0*.*\0") );

			if (bFileOpened == FALSE) break;

			wcscpy( m_szWFileName, T2W(szFileName) );

			CSpStreamFormat OriginalFmt;
			hr = m_cpVoice->GetOutputStream( &cpOldStream );
			if (hr == S_OK)
			{
				hr = OriginalFmt.AssignFormat(cpOldStream);
			}
			else
			{
				hr = E_FAIL;
			}
			// User SAPI helper function in sphelper.h to create a wav file
			if (SUCCEEDED(hr))
			{
				hr = SPBindToFile( m_szWFileName, SPFM_CREATE_ALWAYS, &cpWavStream, &OriginalFmt.FormatId(), OriginalFmt.WaveFormatExPtr() ); 
			}
			if( SUCCEEDED( hr ) )
			{
				// Set the voice's output to the wav file instead of the speakers
				hr = m_cpVoice->SetOutput(cpWavStream, TRUE);
			}

            if ( SUCCEEDED( hr ) )
            {
			    // Do the Speak
			    HandleSpeak();
            }

			// Set output back to original stream
			// Wait until the speak is finished if saving to a wav file so that
			// the smart pointer cpWavStream doesn't get released before its
			// finished writing to the wav.
			m_cpVoice->WaitUntilDone( INFINITE );
			cpWavStream.Release();
			
            // Reset output
			m_cpVoice->SetOutput( cpOldStream, FALSE );
			
			TCHAR   szTitle[MAX_PATH];
            TCHAR   szConfString[MAX_PATH];
            if ( SUCCEEDED( hr ) )
            {
                LoadString( m_hInst, IDS_SAVE_NOTIFY, szConfString, MAX_PATH );
                LoadString( m_hInst, IDS_NOTIFY_TITLE, szTitle, MAX_PATH );
                MessageBox( m_hWnd, szConfString, szTitle, MB_OK | MB_ICONINFORMATION );
            }
            else
            {
                LoadString( m_hInst, IDS_SAVE_ERROR, szConfString, MAX_PATH );
                MessageBox( m_hWnd, szConfString, NULL, MB_ICONEXCLAMATION );
            }

			break;
		}
    }
    
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
void CTTSApp::HandleSpeak()
/////////////////////////////////////////////////////////////////////////////////////////////
{
    HWND                hwndEdit;
	HRESULT             hr = S_OK;
    WCHAR               *szWTextString = NULL;

    // Get handle to the main edit box
    hwndEdit = GetDlgItem( m_hWnd, IDE_EDITBOX );

	TTSAppStatusMessage( m_hWnd, _T("Speak\r\n") );
	SetFocus( hwndEdit );
	m_bStop = FALSE;
	
	// only get the string if we're not paused
	if( !m_bPause )
	{
        // Find the length of the string in the buffer
        GETTEXTLENGTHEX gtlx;
        gtlx.codepage = 1200;
        gtlx.flags = GTL_DEFAULT;
        LONG lTextLen = SendDlgItemMessage( m_hWnd, IDE_EDITBOX, EM_GETTEXTLENGTHEX, (WPARAM) &gtlx, 0 );
        szWTextString = new WCHAR[ lTextLen + 1 ];

		GETTEXTEX	GetText;
		
		GetText.cb			  = (lTextLen + 1) * sizeof( WCHAR );
		GetText.codepage	  = 1200;
		GetText.flags		  = GT_DEFAULT;
		GetText.lpDefaultChar = NULL;
		GetText.lpUsedDefChar = NULL;
		
		// Get the string in a unicode buffer
		SendDlgItemMessage( m_hWnd, IDE_EDITBOX, EM_GETTEXTEX, (WPARAM)&GetText, (LPARAM)szWTextString );

		// do we speak or interpret the XML
		hr = m_cpVoice->Speak( szWTextString, SPF_ASYNC | ((IsDlgButtonChecked( m_hWnd, IDC_SPEAKXML )) ? SPF_IS_XML : SPF_IS_NOT_XML), 0 );

        delete[] szWTextString;

		if( FAILED( hr ) )
		{
			TTSAppStatusMessage( m_hWnd, _T("Speak error\r\n") );
		}
	}
	m_bPause = FALSE;
	SetWindowText( GetDlgItem( m_hWnd, IDB_PAUSE ), _T("Pause") );
	SetFocus( hwndEdit );
	// Set state to run
	hr = m_cpVoice->Resume();            

	if( FAILED( hr ) )
	{
		TTSAppStatusMessage( m_hWnd, _T("Speak error\r\n") );
	}
}

/////////////////////////////////////////////////////////////////
void CTTSApp::MainHandleSynthEvent()
/////////////////////////////////////////////////////////////////
//
// Handles the WM_TTSAPPCUSTOMEVENT application defined message and all
// of it's appropriate SAPI5 events.
//
{

    CSpEvent        event;  // helper class in sphelper.h for events that releases any 
                            // allocated memory in it's destructor - SAFER than SPEVENT
    SPVOICESTATUS   Stat;
    WPARAM          nStart;
    LPARAM          nEnd;
    int             i = 0;
    HRESULT 		hr = S_OK;

    while( event.GetFrom(m_cpVoice) == S_OK )
    {
        switch( event.eEventId )
        {
            case SPEI_START_INPUT_STREAM:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("StartStream event\r\n") );
                }
                break; 

            case SPEI_END_INPUT_STREAM:
                // Set global boolean stop to TRUE when finished speaking
                m_bStop = TRUE; 
                // Highlight entire text
                nStart = 0;
                nEnd = SendDlgItemMessage( m_hWnd, IDE_EDITBOX, WM_GETTEXTLENGTH, 0, 0 );
                SendDlgItemMessage( m_hWnd, IDE_EDITBOX, EM_SETSEL, nStart, nEnd );
                // Mouth closed
                g_iBmp = 0;
                InvalidateRect( m_hChildWnd, NULL, FALSE );
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("EndStream event\r\n") );
                }
                break;     
                
            case SPEI_VOICE_CHANGE:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Voicechange event\r\n") );
                }
                break;

            case SPEI_TTS_BOOKMARK:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    // Get the string associated with the bookmark
                    // and add the null terminator.
                    USES_CONVERSION;
                    TCHAR szBuff2[MAX_PATH] = _T("Bookmark event: ");

                    WCHAR *pwszEventString = new WCHAR[ wcslen( event.String() ) + 1];
                    if ( pwszEventString )
					{
                        wcscpy( pwszEventString, event.String() );
						_tcscat( szBuff2, W2T(pwszEventString) );
						free( pwszEventString );
					}

                    _tcscat( szBuff2, _T("\r\n") );
                    TTSAppStatusMessage( m_hWnd, szBuff2 );
                }
                break;

            case SPEI_WORD_BOUNDARY:
                hr = m_cpVoice->GetStatus( &Stat, NULL );
                if( FAILED( hr ) )
                {
                	TTSAppStatusMessage( m_hWnd, _T("Voice GetStatus error\r\n") );
                }

                // Highlight word
                nStart = (LPARAM)( Stat.ulInputWordPos / sizeof(char) );
                nEnd = nStart + Stat.ulInputWordLen;
                SendDlgItemMessage( m_hWnd, IDE_EDITBOX, EM_SETSEL, nStart, nEnd );
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    
                    TTSAppStatusMessage( m_hWnd, _T("Wordboundary event\r\n") );
                }
                break;

            case SPEI_PHONEME:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Phoneme event\r\n") );
                }
                break;

            case SPEI_VISEME:
                // Get the current mouth viseme position and map it to one of the 
                // 7 mouth bitmaps. 
                g_iBmp = g_aMapVisemeToImage[event.Viseme()]; // current viseme

                InvalidateRect( m_hChildWnd, NULL, FALSE );
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Viseme event\r\n") );
                }
                break;

            case SPEI_SENTENCE_BOUNDARY:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Sentence event\r\n") );
                }
                break;

            case SPEI_TTS_AUDIO_LEVEL:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    USES_CONVERSION;
                    WCHAR wszBuff[MAX_PATH];
                    swprintf(wszBuff, L"Audio level: %d\r\n", (ULONG)event.wParam);
                    TTSAppStatusMessage( m_hWnd, W2T(wszBuff) );
                }
                break;

            case SPEI_TTS_PRIVATE:
                if( IsDlgButtonChecked( m_hWnd, IDC_EVENTS ) )
                {
                    TTSAppStatusMessage( m_hWnd, _T("Private engine event\r\n") );
                }
                break;

            default:
                TTSAppStatusMessage( m_hWnd, _T("Unknown message\r\n") );
                break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////
void CTTSApp::HandleScroll( HWND hCtl )
/////////////////////////////////////////////////////////////////////////
{
    static long         hpos = 1;
    HWND                hVolume, hRate;
    HRESULT				hr = S_OK;

    // Get the current position of the slider
    hpos = SendMessage( hCtl, TBM_GETPOS, 0, 0 );

    // Get the Handle for the scroll bar so it can be associated with an edit box
    hVolume = GetDlgItem( m_hWnd, IDC_VOLUME_SLIDER );
    hRate = GetDlgItem( m_hWnd, IDC_RATE_SLIDER );
    
    if( hCtl == hVolume )
    {
        hr = m_cpVoice->SetVolume((USHORT)hpos);
    }
    else if( hCtl == hRate )
    {
		hr = m_cpVoice->SetRate(hpos);
    }
    
    if( FAILED( hr ) )
    {
    	TTSAppStatusMessage( m_hWnd, _T("Error setting volume / rate\r\n") );
    }

    return;
}

/////////////////////////////////////////////////////////////////
void CTTSApp::MainHandleClose()
/////////////////////////////////////////////////////////////////
{
    // Call helper functions from sphelper.h to destroy combo boxes
    // created with SpInitTokenComboBox
    SpDestroyTokenComboBox( GetDlgItem( m_hWnd, IDC_COMBO_VOICES ) );
    
    // Terminate the app
    PostQuitMessage(0);

    // Return success
    return;
}

/////////////////////////////////////////////////////////////////
HRESULT CTTSApp::VoiceChange()
/////////////////////////////////////////////////////////////////
//
// This function is called during initialization and whenever the 
// selection for the voice combo box changes. 
// It gets the token pointer associated with the voice.
// If the new voice is different from the one that's currently 
// selected, it first stops any synthesis that is going on and
// sets the new voice on the global voice object. 
//
{
    HRESULT         hr = S_OK;
    GUID*           pguidAudioFormat = NULL;
    int             iFormat = 0;

    // Get the token associated with the selected voice
    ISpObjectToken* pToken = SpGetCurSelComboBoxToken( GetDlgItem( m_hWnd, IDC_COMBO_VOICES ) );
    
    //Determine if it is the current voice
    CComPtr<ISpObjectToken> pOldToken;
    hr = m_cpVoice->GetVoice( &pOldToken );

    if (SUCCEEDED(hr))
    {
        if (pOldToken != pToken)
        {        
            // Stop speaking. This is not necesary, for the next call to work,
            // but just to show that we are changing voices.
            hr = m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, 0);
            // And set the new voice on the global voice object
            if (SUCCEEDED (hr) )
            {
                hr = m_cpVoice->SetVoice( pToken );
            }
        }
    }

    EnableSpeakButtons( SUCCEEDED( hr ) );

    return hr;
}

/////////////////////////////////////////////////////////////////
BOOL CTTSApp::CallOpenFileDialog( TCHAR* szFileName, TCHAR* szFilter )  
/////////////////////////////////////////////////////////////////
//
// Display the open dialog box to retrieve the user-selected 
// .txt or .xml file for synthisizing
{
    OPENFILENAME    ofn;
    BOOL            bRetVal     = TRUE;
    LONG            lRetVal;
    HKEY            hkResult;
    TCHAR           szPath[256]       = _T("");
    DWORD           size = 256;

    // Open the last directory used by this app (stored in registry)
    lRetVal = RegCreateKeyEx( HKEY_CURRENT_USER, 
                        _T("SOFTWARE\\Microsoft\\Speech\\AppData\\TTSApp"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegQueryValueEx( hkResult, _T("TTSFiles"), NULL, NULL, (PBYTE)szPath, &size );
    }

	size_t ofnsize = (BYTE*)&ofn.lpTemplateName + sizeof(ofn.lpTemplateName) - (BYTE*)&ofn;
    ZeroMemory( &ofn, ofnsize);


    ofn.lStructSize       = ofnsize;
    ofn.hwndOwner         = m_hWnd;    
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;    
    ofn.nFilterIndex      = 1;    
    ofn.lpstrInitialDir   = szPath;
    ofn.lpstrFile         = szFileName;  
    ofn.nMaxFile          = 256;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;    
    ofn.lpstrDefExt       = NULL;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_READONLY | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    // Pop the dialog
    bRetVal = GetOpenFileName( &ofn );

    // Write the directory path you're in to the registry
    TCHAR   pathstr[256] = _T("");
    _tcscpy( pathstr, szFileName );

    int i=0; 
    while( pathstr[i] != NULL )
    {
        i++;
    }
    while( pathstr[i] != '\\' )
    {
        i --;
    }
    pathstr[i] = NULL;

    // Now write the string to the registry
    RegSetValueEx( hkResult, _T("TTSFiles"), NULL, REG_EXPAND_SZ, (PBYTE)pathstr, _tcslen(pathstr)+1 );

    RegCloseKey( hkResult );

    return bRetVal;
}

/////////////////////////////////////////////////////////////////
BOOL CTTSApp::CallSaveFileDialog( TCHAR* szFileName, TCHAR* szFilter )  
/////////////////////////////////////////////////////////////////
//
// Display the save dialog box to save the wav file
{
    OPENFILENAME    ofn;
    BOOL            bRetVal     = TRUE;
    LONG            lRetVal;
    HKEY            hkResult;
    TCHAR           szPath[256]       = _T("");
    DWORD           size = 256;

    // Open the last directory used by this app (stored in registry)
    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("PathTTSDataFiles"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegQueryValueEx( hkResult, _T("TTSFiles"), NULL, NULL, (PBYTE)szPath, &size );
    
        RegCloseKey( hkResult );
    }

	size_t ofnsize = (BYTE*)&ofn.lpTemplateName + sizeof(ofn.lpTemplateName) - (BYTE*)&ofn;
    ZeroMemory( &ofn, ofnsize);

    ofn.lStructSize       = ofnsize;
    ofn.hwndOwner         = m_hWnd;    
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;    
    ofn.nFilterIndex      = 1;    
    ofn.lpstrInitialDir   = szPath;
    ofn.lpstrFile         = szFileName;  
    ofn.nMaxFile          = 256;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;    
    ofn.lpstrDefExt       = _T("wav");
    ofn.Flags             = OFN_OVERWRITEPROMPT;

    // Pop the dialog
    bRetVal = GetSaveFileName( &ofn );

	if ( ofn.Flags & OFN_EXTENSIONDIFFERENT )
	{
		_tcscat( szFileName, _T(".wav") );
	}

    // Write the directory path you're in to the registry
    TCHAR   pathstr[256] = _T("");
    _tcscpy( pathstr, szFileName );

    int i=0; 
    while( pathstr[i] != NULL )
    {
        i++;
    }
    while( pathstr[i] != '\\' )
    {
        i --;
    }
    pathstr[i] = NULL;

    // Now write the string to the registry
    lRetVal = RegCreateKeyEx( HKEY_CLASSES_ROOT, _T("PathTTSDataFiles"), 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkResult, NULL );

    if( lRetVal == ERROR_SUCCESS )
    {
        RegSetValueEx( hkResult, _T("TTSFiles"), NULL, REG_EXPAND_SZ, (PBYTE)pathstr, _tcslen(pathstr)+1 );
    
        RegCloseKey( hkResult );
    }

    return bRetVal;
}

/////////////////////////////////////////////////////
HRESULT CTTSApp::ReadTheFile( TCHAR* szFileName, BOOL* bIsUnicode, WCHAR** ppszwBuff )
/////////////////////////////////////////////////////
//
// This file opens and reads the contents of a file. It
// returns a pointer to the string.
// Warning, this function allocates memory for the string on 
// the heap so the caller must free it with 'delete'.
//
{
    // Open up the file and copy it's contents into a buffer to return
	HRESULT		hr = 0;
    HANDLE		hFile;
	DWORD		dwSize = 0;
	DWORD		dwBytesRead = 0;
	    
    // First delete any memory previously allocated by this function
    if( m_pszwFileText )
    {
        delete m_pszwFileText;
    }

    hFile = CreateFile( szFileName, GENERIC_READ,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
		*ppszwBuff = NULL;
		hr = E_FAIL;
    }

	if( SUCCEEDED( hr ) )
	{
		dwSize = GetFileSize( hFile, NULL );
		if( dwSize == 0xffffffff )
		{
			*ppszwBuff = NULL;
			hr = E_FAIL;        
		}
	}
 
	if( SUCCEEDED( hr ) )
	{
		// Read the file contents into a wide buffer and then determine
		// if it's a unicode or ascii file
		WCHAR	Signature = 0;

		ReadFile( hFile, &Signature, 2, &dwBytesRead, NULL );
			
		// Check to see if its a unicode file by looking at the signature of the first character.
		if( 0xFEFF == Signature )
		{
			*ppszwBuff = new WCHAR [dwSize/2];

			*bIsUnicode = TRUE;
			ReadFile( hFile, *ppszwBuff, dwSize-2, &dwBytesRead, NULL );
			(*ppszwBuff)[dwSize/2-1] = NULL;

			CloseHandle( hFile );
		}			
		else  // MBCS source
		{
			char*	pszABuff = new char [dwSize+1];
			*ppszwBuff = new WCHAR [dwSize+1];

			*bIsUnicode = FALSE;
			SetFilePointer( hFile, NULL, NULL, FILE_BEGIN );
			ReadFile( hFile, pszABuff, dwSize, &dwBytesRead, NULL );
			pszABuff[dwSize] = NULL;
            ::MultiByteToWideChar( CP_ACP, 0, pszABuff, -1, *ppszwBuff, dwSize + 1 );

			delete( pszABuff );
			CloseHandle( hFile );
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////
void CTTSApp::UpdateEditCtlW( WCHAR* pszwText )
/////////////////////////////////////////////////////////////////////////
{
	CComPtr<IRichEditOle>		cpRichEdit;
	CComPtr<ITextDocument>		cpTextDocument;
	ITextServices*				pTextServices=NULL;
	HRESULT						hr = S_OK;

	// Use rich edit control interface pointers to update text
	if( SendDlgItemMessage( m_hWnd, IDE_EDITBOX, EM_GETOLEINTERFACE, 0, (LPARAM)(LPVOID FAR *)&cpRichEdit ) )
	{
		hr = cpRichEdit.QueryInterface( &cpTextDocument );
		
		if( SUCCEEDED( hr ) )
		{
			hr = cpTextDocument->QueryInterface( IID_ITextServices, (void**)&pTextServices );
		}

		if (SUCCEEDED(hr))
		{
			BSTR bstr = SysAllocString( pszwText );
            
			hr = pTextServices->TxSetText( bstr );

			pTextServices->Release();

			SysFreeString( bstr );
		}
	}

	// Add text the old fashon way by converting it to ansi. Note information
	// loss will occur because of the WC2MB conversion.
	if( !cpRichEdit || FAILED( hr ) )  
	{
        USES_CONVERSION;
        TCHAR *pszFileText = _tcsdup( W2T(pszwText) );
		SetDlgItemText( m_hWnd, IDE_EDITBOX, pszFileText );
        free( pszFileText );
	}
}

/////////////////////////////////////////////////////////////////////////
void CTTSApp::EnableSpeakButtons( BOOL fEnable )
/////////////////////////////////////////////////////////////////////////
{
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDB_SPEAK ), fEnable );
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDB_PAUSE ), fEnable );
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDB_STOP ), fEnable );
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDB_SKIP ), fEnable );
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDB_SPEAKWAV ), fEnable );
    ::EnableWindow( ::GetDlgItem( m_hWnd, IDC_SAVETOWAV ), fEnable );
}

/////////////////////////////////////////////////////
inline void TTSAppStatusMessage( HWND hWnd, TCHAR* szMessage )
/////////////////////////////////////////////////////
//
// This function prints debugging messages to the debug edit control
//
{
    static TCHAR            szDebugText[MAX_SIZE]=_T("");
    static int              i = 0;
    
    // Clear out the buffer after 100 lines of text have been written
    // to the debug window since it can only hold 4096 characters.
    if( i == 100 )
    {
        _tcscpy( szDebugText, _T("") );
        i = 0;
    }
    // Attach the new message to the ongoing list of messages
    _tcscat( szDebugText, szMessage );
    SetDlgItemText( hWnd, IDC_DEBUG, szDebugText );

    SendDlgItemMessage( hWnd, IDC_DEBUG, EM_LINESCROLL, 0, i );

    i++;

    return;
}

/*****************************************************************************************
* About() *
*---------*
*   Description:
*       Message handler for the "About" box.
******************************************************************************************/
LRESULT CALLBACK About( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    USES_CONVERSION;

    switch( message )
    {
        case WM_COMMAND:
        {
            WORD wId    = LOWORD(wParam); 
            WORD wEvent = HIWORD(wParam); 
            
            switch( wId )
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog( hDlg, LOWORD(wParam) );
                    return TRUE;
            }
        }
    }
    return FALSE;
}   /* About */
