/******************************************************************************
*   dictpad.cpp 
*       This file contains the entry point for the Dictpad application
*       and some of the definitions for methods of CDictationPad, the
*       main object.  
*       dictpad_sapi.cpp contains the methods of CDictationPad that 
*       pertain to the SAPI interfaces used in this app.
*
*   Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
******************************************************************************/

#include "stdafx.h"
#include <richedit.h>
#include <commctrl.h>
#include <commdlg.h>
#include "dictpad.h"

// Stream names for saving and opening Dictpad files
#define DICTPAD_TEXT         L"dictpad_text"
#define DICTPAD_RECORESULTS  L"dictpad_recoresults"

/************************************************************************
* WinMain() *
*-----------*
*   Description:
*       Main entry point for the application
**************************************************************************/
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow )
{
    int iRet = 0;
    HRESULT hr = CoInitialize( NULL );
    if( SUCCEEDED( hr ) )
    {
        CDictationPad *pDictPad = new CDictationPad( hInstance );
        if( pDictPad )
        {
            if ( pDictPad->Initialize( nCmdShow, lpCmdLine ) )
            {
                iRet = pDictPad->Run();
            }

            delete pDictPad;
        }
        CoUninitialize();
    }

    return iRet;
}   /* WinMain */

/****************************************************************************
* CDictationPad::CDictationPad() *
*--------------------------------*
*   Description:
*       Constructor
*****************************************************************************/   
CDictationPad::CDictationPad( HINSTANCE hInst ) :
    m_hAccelTable( NULL ),
    m_hInst( hInst ),
    m_hRtfLib( NULL ),
    m_hEdit( NULL ),
    m_hToolBar( NULL ),
    m_hStatusBar( NULL ),
    m_hFont( NULL ),
    m_hAltsButton( NULL ),
    m_dwFlags( DP_SHARED_RECOGNIZER ),
    m_ullDictInterest( 
#ifdef _DEBUG
                                      // Dictpad doesn't actually need SOUND_START
                                      // or SOUND_END
                                      SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
#endif
                                      SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) |
                                      SPFEI(SPEI_RECO_OTHER_CONTEXT) |
                                      SPFEI(SPEI_FALSE_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) | 
                                      SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_STATE_CHANGE) ),
    m_ullCCInterest( SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
                                      SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) | 
                                      SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_STATE_CHANGE) ),
    m_pCandidateList( NULL ),
    m_pszFile( NULL )
{
    // Zero out the various STRUCTs we are using
    memset( (void *) &m_LastSelInfo, 0, sizeof( SELINFO ) );
    memset( (void *) &m_CurSelInfo, 0, sizeof( SELINFO ) );
    memset( (void *) &m_SpeakInfo, 0, sizeof( SPEAKINFO ) );
}

/****************************************************************************
* CDictationPad::~CDictationPad() *
*---------------------------------*
*   Description:
*       Destructor
*****************************************************************************/   
CDictationPad::~CDictationPad()
{
    // Release the richedit/TOM objects.  These need to be released before
    // the RTF library gets freed.
    m_cpRichEdit = NULL;
    m_cpTextDoc = NULL;
    m_cpTextSel = NULL;

    // Delete objects we have new-ed
    if ( m_pTextRunList )
    {
        delete m_pTextRunList;
    }
    if ( m_pCandidateList )
    {
        delete m_pCandidateList;
    }
    if ( m_pRecoEventMgr )
    {
        delete m_pRecoEventMgr;
    }

    if( m_hFont )
    {
        ::DeleteObject( m_hFont );
    }
    if( m_hRtfLib )
    {
        ::FreeLibrary( m_hRtfLib );
    }
}

/****************************************************************************
* CDictationPad::Initialize *
*---------------------------*
*   Description:
*       Set up the windows, as well as the RichEdit objects.
*       Calls InitializeSAPIObjs, which in initializes the SAPI objects
*       and sets up notification for SAPI events.
*****************************************************************************/   
BOOL CDictationPad::Initialize( int nCmdShow, LPSTR lpCmdLine )
{
    TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
    TCHAR szWindowClass[MAX_LOADSTRING];
    BOOL bRet = false;                      // assume failure

    // Initialize window strings
    ::LoadString( m_hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING );
    ::LoadString( m_hInst, IDC_DICTPAD, szWindowClass, MAX_LOADSTRING );

    // register our window class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = (WNDPROC)WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = m_hInst;
    wcex.hIcon = ::LoadIcon(m_hInst, (LPCTSTR)IDI_DICTPAD);
    wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = (LPCTSTR)IDC_DICTPAD;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = ::LoadIcon( wcex.hInstance, (LPCTSTR)IDI_SMALL );
    ATOM atomRet = ::RegisterClassEx( &wcex );

    HRESULT hr = E_FAIL;
    if( atomRet )
    {
        // Set up the text run list and the reco event manager
        m_pTextRunList = new CTextRunList();
        m_pRecoEventMgr = new CRecoEventMgr( m_hInst );
        if ( !m_pTextRunList || !m_pRecoEventMgr )
        {
            return false;
        }

        // Load DLL for Rich Edit 3.0
        m_hRtfLib = LoadLibrary( _T("RICHED20.DLL") );

        // Create the main application window
        m_hClient = ::CreateWindow( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInst, this );

        // The richedit window (m_hEdit) will have been created by the WM_CREATE
        // message-handling in the WNDPROC for the main window
        if( m_hClient && m_hEdit )
        {
            // Set interest in various events from the richedit control
            ::SendMessage( m_hEdit, EM_SETEVENTMASK, 0, 
                ENM_KEYEVENTS | ENM_MOUSEEVENTS | ENM_CHANGE | ENM_UPDATE | ENM_SELCHANGE | ENM_SCROLL );

            // Show the window if it is supposed to be shown upon startup
            ::ShowWindow( m_hClient, nCmdShow );
            
            // Update the window so that the window is drawn without having to 
            // wait for all of the SAPI initialization
            ::UpdateWindow( m_hClient );

            // Accelerators
            m_hAccelTable = ::LoadAccelerators( m_hInst, (LPCTSTR)IDC_DICTPAD );

            // Get the TOM objects (an ITextDocument and an ITextSelection)
            // from the richedit control
            ::SendMessage( m_hEdit, EM_GETOLEINTERFACE, 0, (LPARAM)(LPVOID FAR *)&m_cpRichEdit );
            if ( !m_cpRichEdit )
            {
                ::MessageBox( m_hClient, _T("Error getting the RichEdit interface"), NULL, MB_OK );
                return false;
            }
            hr = m_cpRichEdit->QueryInterface( IID_ITextDocument, (void**)&m_cpTextDoc );
            if( SUCCEEDED( hr ) )
            {
                // Hand the pointer to the ITextDocument for this document to the
                // data members that need that pointer.
                m_pTextRunList->SetTextDoc( m_cpTextDoc );
                
                hr = m_cpTextDoc->GetSelection( &m_cpTextSel );  
            }
            else
            {
                ::MessageBox( m_hClient, _T("Error getting ITextDocument"), NULL, MB_OK );
            }

            if ( SUCCEEDED( hr ) )
            {
                // Hand the pointer to the TOM objects for this document to the
                // data members that need that pointer.
                m_pRecoEventMgr->SetTextSel( m_cpTextSel );
                m_pCandidateList->SetTextSel( m_cpTextSel );
            }
            else
            {
                ::MessageBox( m_hClient, _T("Error getting ITextSelection"), NULL, MB_OK );
            }


            // Do the initialization of SAPI objects
            if ( SUCCEEDED( hr ) )
            {
                hr = InitializeSAPIObjs();
            }
        }
    }

    if ( FAILED( hr ) )
    {
        // Bail, since some initialization did not happen correctly
        ::MessageBox( m_hClient, _T("Cannot initialize Dictpad; exiting."), NULL, MB_OK );
        ::DestroyWindow( m_hClient );
    }

    if ( SUCCEEDED( hr ) && lpCmdLine && *lpCmdLine )
    {
        // lpInitialFileName will come surrounded by quotation marks,
        // which we need to strip off
        USES_CONVERSION;
        TCHAR *pszFileName = _tcsdup( A2T(lpCmdLine) );
        if ( !pszFileName )
        {
            return E_OUTOFMEMORY;
        }
        TCHAR *pchBeginQuote = _tcschr( pszFileName, _T('\"') );
        if ( pchBeginQuote )
        {
            // Copy everything after the first quote
            _tcscpy( pszFileName, pchBeginQuote + 1 );

            TCHAR *pchEndQuote = _tcschr( pszFileName, _T('\"') );
            if ( pchEndQuote )
            {
                // Cut it off at the end quote
                *pchEndQuote = 0;
            }
        }

        hr = DoFileOpen( pszFileName );

        free( pszFileName );
    }

    return SUCCEEDED( hr );
}   /* CDictationPad::Initialize */

/****************************************************************************************
* CDictationPad::Run() *
*----------------------*
*   Description:
*       Contains the message loop for the application
*****************************************************************************************/
int CDictationPad::Run()
{
    // Main message loop:
    MSG msg;
    while( ::GetMessage( &msg, NULL, 0, 0 ) ) 
    {
        if( !::TranslateAccelerator( m_hClient, m_hAccelTable, &msg ) ) 
        {
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }
    }

    return msg.wParam;
}   /* CDictationPad::Run */


/*****************************************************************************************
* CDictationPad::WndProc() *
*--------------------------*
*   Description:
*       Main message handler for Dictpad.
******************************************************************************************/
LRESULT CALLBACK CDictationPad::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    int wmId, wmEvent;
    HRESULT hr;
    HMENU hMenu;

    // The CDictationPad object was set to be the USERDATA in the window long
    // associated with this window when WM_CREATE was called
    CDictationPad * pThis = (CDictationPad *)::GetWindowLong( hWnd, GWL_USERDATA );
        
    switch( message ) 
    {
        case WM_CREATE :
        {
            // store pointer to the object that created this window
            pThis = (CDictationPad *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
            _ASSERTE(pThis);
            ::SetWindowLong(hWnd, GWL_USERDATA, (LONG)pThis);

            // This call draws the toolbar buttons, gets the richedit control, etc.
            if ( !( pThis->InitializeWindow( hWnd ) ) )
            {
                ::DestroyWindow( hWnd );
                break;
            }

            ::SetFocus( pThis->m_hEdit );

            // Indicate whether whole words are being selected
            hMenu = ::GetMenu( hWnd );
            ::CheckMenuItem( hMenu, IDM_WHOLE_WORDS, 
                (pThis->m_dwFlags & DP_WHOLE_WORD_SEL) ? MF_CHECKED : MF_UNCHECKED );

            // Set the appropriate menu info for the type of engine
            ::CheckMenuItem( hMenu, IDM_SHAREDENGINE, 
                (pThis->m_dwFlags & DP_SHARED_RECOGNIZER) ? MF_CHECKED : MF_UNCHECKED );
            
            //--- get us into dictation mode
            ::SendMessage( hWnd, WM_COMMAND, IDM_DICTATION_MODE, 0 );

            break;
        }
        
        case WM_SIZE:
        {
            // If the frame changes size, change the size of the edit control,
            // taking the toolbar & status bar into consideration
            RECT r;
            ::GetWindowRect( pThis->m_hToolBar, &r );
            LONG lToolBarHeight = r.bottom - r.top;

            ::GetWindowRect( pThis->m_hStatusBar, &r );
            LONG lStatusBarHeight = r.bottom - r.top;

            ::SetWindowPos( pThis->m_hToolBar, hWnd, 0, 0, LOWORD(lParam), 
                lToolBarHeight, SWP_NOMOVE | SWP_NOZORDER );
            ::SetWindowPos( pThis->m_hEdit, hWnd, 0, 0, LOWORD(lParam), 
                HIWORD(lParam) - lToolBarHeight - lStatusBarHeight, 
                SWP_NOMOVE | SWP_NOZORDER );
            ::SetWindowPos( pThis->m_hStatusBar, hWnd, 0, 
                HIWORD(lParam) - lStatusBarHeight, LOWORD(lParam), lStatusBarHeight, 
                SWP_NOZORDER );
            break;
        }

        case WM_COMMAND:
            // If the SR engine is currently cranking, then save this message for later
            if ( pThis->m_pRecoEventMgr->IsProcessingPhrase() )
            {
                // The message will get handled later, when the result comes back
                pThis->m_pRecoEventMgr->QueueCommand( hWnd, message, wParam, lParam );
                break;
            }

            wmId    = LOWORD(wParam); 
            wmEvent = HIWORD(wParam); 
 
            // Look for notifications from the edit control about gaining and losing
            // input focus
            if ( ((HWND) lParam) == pThis->m_hEdit )
            {
                switch ( wmEvent )
                {
                   case EN_KILLFOCUS:
                        if ( pThis->m_dwFlags & DP_DICTATION_MODE )
                        {
                            // Turn off the "mic"
                            pThis->SetGrammarState( false );
                        }
                        break;

                    case EN_SETFOCUS:
                        if ( pThis->m_dwFlags & DP_DICTATION_MODE )
                        {
                            // Restore the "mic" to its state when we last lost focus
                            pThis->SetGrammarState( pThis->m_dwFlags & DP_MICROPHONE_ON );
                        }

                        // Trigger an update so that the alternates button appears
                        // if appropriate
                        pThis->m_hAltsButton = pThis->m_pCandidateList->Update( 
                            pThis->m_pTextRunList );
                        break;
                }
                break;
            }
                    

            // Handle menu item selections
            switch (wmId)
            {
                //----------------------
                // FILE
                //----------------------
                case ID_FILE_NEW:
                {
                    hr = pThis->DoFileNew();
                    if ( FAILED( hr ) )
                    {
                        MessageBoxFromResource( hWnd, IDS_CANNOTFILENEW, NULL, MB_ICONEXCLAMATION );
                        ::DestroyWindow( hWnd );
                    }

                    break;
                }
                case ID_FILE_OPEN:
                {
                    hr = pThis->DoFileOpen( NULL );

                    if ( FAILED( hr ) )
                    {
                        MessageBoxFromResource( hWnd, IDS_CANNOTOPEN, NULL, MB_ICONEXCLAMATION );

                        // Open a new file instead
                        ::SendMessage( pThis->m_hClient, WM_COMMAND, ID_FILE_NEW, 0 );
                    }

                    break;
                }

                case ID_FILE_SAVE:
                case ID_FILE_SAVEAS:
                {
                    if ( (ID_FILE_SAVE == wmId) && pThis->m_pszFile )
                    {
                        // The file already has a name.
                        // Just save under the existing name
                        hr = pThis->DoFileSave();
                    }
                    else
                    {
                        // Need to get a name
                        hr = pThis->DoFileSaveAs();
                    }

                    if (FAILED(hr))
                    {
                        TCHAR pszCaptionText[ MAX_LOADSTRING ];
                        ::LoadString( pThis->m_hInst, IDS_CANNOTSAVE, 
                            pszCaptionText, MAX_LOADSTRING );
                        
                        if (STG_E_ACCESSDENIED == hr)
                        {
                            // The user has a readonly file open
                            MessageBoxFromResource( hWnd, IDS_ACCESSDENIED, pszCaptionText, 
                                MB_ICONEXCLAMATION );

                            // Ask user to save under a different name
                            ::SendMessage( pThis->m_hClient, WM_COMMAND, ID_FILE_SAVEAS, 0 );
                        }
                        else
                        {
                            MessageBoxFromResource( hWnd, IDS_ERRORSAVING, pszCaptionText, 
                                MB_ICONEXCLAMATION );
                        }
                    }

                    // Return 1 only if we saved the file
                    return (S_OK == hr);
                    
                    break;
                }              

                case IDM_EXIT:
                    ::SendMessage( hWnd, WM_CLOSE, 0, 0 );
                    break;

                case ID_EDIT_CUT:
                    // Cut the selected text to the clipboard
                    pThis->m_cpTextSel->Cut( NULL );
                    break;

                case ID_EDIT_COPY:
                    // Copy the selected text to the clipboard
                    pThis->m_cpTextSel->Copy( NULL );
                    break;

                case ID_EDIT_PASTE:
                    // Paste the text from the clipboard into the document
                    
                    // This flag will be used when the selection changed is processed.
                    // It will indicate that there is new text present, even if the 
                    // length of the text in the document has not changed.
                    pThis->m_dwFlags |= DP_JUST_PASTED_TEXT;
                    pThis->m_cpTextSel->Paste( NULL, 0 );
                    pThis->m_dwFlags &= ~DP_JUST_PASTED_TEXT;
                    break;

                case IDM_FONT:
                    {
                        LOGFONT lf;
                        CHOOSEFONT cf;
                        ZeroMemory( &cf, sizeof(cf) );
                        cf.lStructSize = sizeof(cf);
                        cf.lpLogFont = &lf;
                        cf.Flags = CF_SCREENFONTS;
                        if( ::ChooseFont( &cf ) )     // Display the choose font dialog
                        {
                            ::DeleteObject( (HGDIOBJ)pThis->m_hFont );
                            pThis->m_hFont = ::CreateFontIndirect( &lf );
                            ::SendMessage( pThis->m_hEdit, WM_SETFONT, (WPARAM)pThis->m_hFont, MAKELPARAM(true, 0) );
                            ::SetFocus( pThis->m_hEdit );
                        }
                    }
                    break;

                case IDM_DICTATION_MODE:
                case IDM_COMMAND_MODE:
                    // Choose between dictation mode and command mode
                    hr = pThis->SetMode( IDM_DICTATION_MODE == wmId );
                    if (FAILED(hr))
                    {
                        MessageBoxFromResource( hWnd, IDS_CANNOTSWITCHMODES, NULL, 
                            MB_ICONEXCLAMATION );
                    }
                    break;

                case IDM_VOICE_TRAINING:
                    {
                        // Brings up the SR-engine-specific user training UI
                        pThis->m_cpRecoEngine->DisplayUI(hWnd, NULL, SPDUI_UserTraining, NULL, 0);
                    }
                    break;

                case IDM_MICROPHONE_SETUP:
                    {
                        // Brings up the SR-engine-specific mic training UI
                        pThis->m_cpRecoEngine->DisplayUI(hWnd, NULL, SPDUI_MicTraining, NULL, 0);
                    }
                    break;

                case IDM_ADDREMOVEWORDS:
                    // Brings up the SR-engine-specific Add/Remove Words UI
                    pThis->RunAddDeleteUI();
                    break;
                
                case IDM_MODE_TOGGLE:
                    // Toggles between dictation mode and command mode
                    hMenu = ::GetMenu( hWnd );
                    if( pThis->m_dwFlags & DP_DICTATION_MODE )
                    {
                        ::SendMessage( hWnd, WM_COMMAND, IDM_COMMAND_MODE, 0 );
                    }
                    else
                    {
                        ::SendMessage( hWnd, WM_COMMAND, IDM_DICTATION_MODE, 0 );
                    }
                    break;

                case IDM_WHOLE_WORDS:
                    // Toggles between selecting whole words and normal selection
                    hMenu = ::GetMenu( hWnd );
                    if( pThis->m_dwFlags & DP_WHOLE_WORD_SEL )
                    {
                        pThis->m_dwFlags &= ~DP_WHOLE_WORD_SEL;
                        ::CheckMenuItem( hMenu, IDM_WHOLE_WORDS, MF_UNCHECKED );
                    }
                    else
                    {
                        pThis->m_dwFlags |= DP_WHOLE_WORD_SEL;
                        ::CheckMenuItem( hMenu, IDM_WHOLE_WORDS, MF_CHECKED );
                    }
                    break;

               case IDM_SHAREDENGINE:
                   // Toggles between inproc reco engine and shared reco engine
                    hMenu = ::GetMenu( hWnd );
                    if ( pThis->m_dwFlags & DP_SHARED_RECOGNIZER )
                    {
                        pThis->m_dwFlags &= ~DP_SHARED_RECOGNIZER;
                        ::CheckMenuItem( hMenu, IDM_SHAREDENGINE, MF_UNCHECKED );
                    }
                    else
                    {
                        pThis->m_dwFlags |= DP_SHARED_RECOGNIZER;
                        ::CheckMenuItem( hMenu, IDM_SHAREDENGINE, MF_CHECKED );
                    }

                    // When the reco engine changes, all of the SAPI objects need
                    // to be unplugged and reinitialized.
                    if ( FAILED(pThis->InitializeSAPIObjs()) )
                    {
                        // New SAPI objects couldn't get set up: need to bail
                        ::DestroyWindow( hWnd );
                    }
                    break;
                
               case IDM_PLAY:
                    // if we're already speaking then stop
                    if( pThis->m_dwFlags & DP_IS_SPEAKING )
                    {
                        pThis->EndSpeaking();
                    }
                    else
                    {
                        pThis->DoPlay();
                    }
                    break;
                
                case IDM_MIC_TOGGLE:
                {
                    DWORD dwOldFlags = pThis->m_dwFlags;
                    if ( !(pThis->m_dwFlags & DP_GRAMMARS_ACTIVE) )
                    {
                        // If the user sees the microphone button not pressed and tries
                        // to press it, the grammars should be activated regardless
                        // of what the state is
                        pThis->m_dwFlags |= DP_MICROPHONE_ON;
                    }
                    else
                    {
                        // Flip the microphone flag
                        pThis->m_dwFlags ^= DP_MICROPHONE_ON;
                    }

                    if ( pThis->m_dwFlags & DP_MICROPHONE_ON )
                    {
                        // Since the grammars are active, we want the dictation
                        // window to have the input focus
                        ::SetFocus( pThis->m_hEdit );
                    }

                    // Set the microphone accordingly
                    hr = pThis->SetGrammarState( pThis->m_dwFlags & DP_MICROPHONE_ON );

                    if ( FAILED( hr ) )
                    {
                        // Couldn't do this: switch the microphone flag back
                        pThis->m_dwFlags = dwOldFlags;
                    }

                    break;
                }

                case IDM_ABOUT:
                    ::DialogBox( pThis->m_hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About );
                    break;

                default:
                    return ::DefWindowProc( hWnd, message, wParam, lParam );
            }

            break;

        case WM_KILLFOCUS:
            OutputDebugString( _T("Kill focus\r\n") );

            if ( pThis->m_dwFlags & DP_DICTATION_MODE )
            {
                // No focus, no microphone
                pThis->SetGrammarState( false );
            }
            break;

        case WM_CLOSE:
            if ( S_OK == pThis->DoFileClose() )
            {
                ::DestroyWindow( hWnd );
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage( 0 );
            break;

        // The following three window messages were defined by the call to 
        // ISpNotifyTranslator::InitWindowMessage() in 
        // CDictationPad::InitSAPICallback (see dictpad_sapi.cpp) to be
        // the messages this window receives whenever SAPI wants to 
        // notify us of one of the events in which we said we were interested
        
        case WM_DICTRECOEVENT:
        {
            // Something happened with the dictation recognition context
            bool fSuccess = pThis->SRDictEventHandler();
            
            // We expect this to succeed; otherwise exit
            _ASSERTE( fSuccess );
            if ( !fSuccess )
            {
                MessageBoxFromResource( hWnd, IDS_UPDATEERROR,
                    NULL, MB_ICONEXCLAMATION );
                ::SendMessage( hWnd, WM_CLOSE, 0, 0 );
            }
            break;
        }

        case WM_CCRECOEVENT:
            // Something happened with the command-and-control reco context
            pThis->SRCCEventHandler();
            break;

        case WM_TTSEVENT:
            // Some TTS event happened
            pThis->TTSEventHandler();
            break;

        // The following two messages have to do with "updating", which is
        // Dictpad's way of dealing with selection changes in the richedit 
        // control (which is how Dictpad knows that the text in the document
        // has changed

        case WM_STOPUPDATE:
            pThis->m_dwFlags |= DP_SKIP_UPDATE;
            break;

        case WM_STARTUPDATE:
            pThis->m_dwFlags &= ~DP_SKIP_UPDATE;
            break;

        case WM_UPDATEALTSBUTTON:
            pThis->m_hAltsButton = pThis->m_pCandidateList->Update( pThis->m_pTextRunList );
            break;

        // Notifications from the richedit control
        case WM_NOTIFY:

            // Set the tooltips if that is what this notification is about
            pThis->SetTooltipText( lParam );
            
            switch ( ((LPNMHDR)lParam)->code)
            {
                case EN_MSGFILTER:
                {
                    MSGFILTER *pMsgFilter = (MSGFILTER*)lParam;
                    pThis->ProcessMsgFilter( pMsgFilter );
                    break;
                }

                case EN_SELCHANGE:
                    // Selection has changed; Dictpad needs to update 
                    // its state
                    hr = pThis->ProcessSelChange( (SELCHANGE *) lParam );

                    // We expect this to succeed; otherwise exit
                    _ASSERTE( SUCCEEDED(hr) );
                    if ( FAILED( hr ) )
                    {
                        MessageBoxFromResource( hWnd, IDS_UPDATEERROR,
                            NULL, MB_ICONEXCLAMATION );
                        ::SendMessage( hWnd, WM_CLOSE, 0, 0 );
                    }
                    break;
            }
            break;

        
        default:
            return ::DefWindowProc( hWnd, message, wParam, lParam );
            break;
   }
   return 0;
} /* CDictationPad::WndProc */


/****************************************************************************************
* CDictationPad::InitializeWindow() *
*-----------------------------------*
*   Description:
*       Sets up and creates the application window, along with the toolbar,
*       status bar, and richedit control.
*       In order for the alternates list UI to trap window messages from
*       the richedit control, we subclass the richedit control so that it
*       actually uses the WndProc in candidatelist.cpp
*   Return:
*       TRUE iff successful
*****************************************************************************************/
BOOL CDictationPad::InitializeWindow( HWND hWnd )
{
    // Create a toolbar
    m_hToolBar = ::CreateToolbarEx( hWnd, WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_TOOLTIPS, 
        IDR_TOOLBAR, 5, m_hInst, IDR_TOOLBAR, NULL, 0, 0, 0, 0, 0, sizeof(TBBUTTON) );
    if ( !m_hToolBar )
    {
        return FALSE;
    }

    // add the buttons
    TBBUTTON tbButtons[5];

    tbButtons[0].iBitmap = 0;
    tbButtons[0].idCommand = IDM_DICTATION_MODE;
    tbButtons[0].fsState = TBSTATE_ENABLED;
    tbButtons[0].fsStyle = TBSTYLE_BUTTON;
    tbButtons[0].dwData = 0;
    tbButtons[0].iString = 0;

    tbButtons[1].iBitmap = 1;
    tbButtons[1].idCommand = IDM_COMMAND_MODE;
    tbButtons[1].fsState = TBSTATE_ENABLED;
    tbButtons[1].fsStyle = TBSTYLE_BUTTON;
    tbButtons[1].dwData = 0;
    tbButtons[1].iString = 0;

    tbButtons[2].iBitmap = 0;
    tbButtons[2].idCommand = 0;
    tbButtons[2].fsState = TBSTATE_ENABLED;
    tbButtons[2].fsStyle = TBSTYLE_SEP;
    tbButtons[2].dwData = 0;
    tbButtons[2].iString = 0;
    
    tbButtons[3].iBitmap = 2;
    tbButtons[3].idCommand = IDM_PLAY;
    tbButtons[3].fsState = TBSTATE_ENABLED;
    tbButtons[3].fsStyle = TBSTYLE_BUTTON;
    tbButtons[3].dwData = 0;
    tbButtons[3].iString = 0;

    tbButtons[4].iBitmap = 3;
    tbButtons[4].idCommand = IDM_MIC_TOGGLE;
    tbButtons[4].fsState = TBSTATE_ENABLED;
    tbButtons[4].fsStyle = TBSTYLE_BUTTON;
    tbButtons[4].dwData = 0;
    tbButtons[4].iString = 0;


    ::SendMessage( m_hToolBar, TB_ADDBUTTONS, 5, (LONG)tbButtons );
    ::SendMessage( m_hToolBar, TB_AUTOSIZE, 0, 0 );

    // Create a status bar
    m_hStatusBar = ::CreateStatusWindow( WS_CHILD | WS_VISIBLE, NULL, hWnd, IDC_STATUSBAR );
    if ( !m_hStatusBar )
    {
        return FALSE;
    }

    // Set up the candidate list UI manager.
    // This will register a window class to which our richedit control should belong
    m_pCandidateList = new CCandidateList( hWnd, *m_pRecoEventMgr );
    if ( !m_pCandidateList )
    {
        return FALSE;
    }

    // Create a rich edit control from the candidate UI object's registered parent class
    
    // The richedit control should fill the client area, taking the toolbar & status bar 
    // into consideration
    POINT ptToolBar;
    RECT r;
    LONG lToolBarHeight;
    LONG lStatusBarHeight;
    ::GetWindowRect( m_hToolBar, &r );
    lToolBarHeight = r.bottom - r.top;
    ptToolBar.x = r.left;
    ptToolBar.y = r.bottom;
    ::ScreenToClient( hWnd, &ptToolBar );

    ::GetWindowRect( m_hStatusBar, &r );
    lStatusBarHeight = r.bottom - r.top;
    
    ::GetClientRect (hWnd, &r);

    // Get the modified richedit window class registered by m_pCandidateList
    const WNDCLASS *pwcEditWindowClass = m_pCandidateList->GetParentClass();
    if ( pwcEditWindowClass )
    {
        // Create the modified richedit control.
        // We pass the candidate UI manager in as lParam
        m_hEdit = ::CreateWindowEx( WS_EX_CLIENTEDGE, 
            pwcEditWindowClass->lpszClassName,
            _T(""), 
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL,
            0, 
            ptToolBar.y, r.right, 
            ( r.bottom - ( lToolBarHeight + lStatusBarHeight ) ), 
                                   
            hWnd, 
            (HMENU)1, 
            m_hInst,  
            (LPVOID) m_pCandidateList );
    }

    if ( !m_hEdit )
    {
        return FALSE;
    }

    return TRUE;
}   /* CDictationPad::InitializeWindow() */

/****************************************************************************************
* CDictationPad::SetTooltipText() *
*---------------------------------*
*   Description:
*       Puts the appropriate tooltip text into the LTOOLTIPTEXT struct pointed to
*       by lParam.
******************************************************************************************/
void CDictationPad::SetTooltipText( LPARAM lParam ) 
{ 
    LPTOOLTIPTEXT pToolTipText; 
    static TCHAR szBuffer[64]; 
 
    pToolTipText = (LPTOOLTIPTEXT)lParam; 
    if( pToolTipText->hdr.code == TTN_NEEDTEXT )
    { 
        ::LoadString( m_hInst, pToolTipText->hdr.idFrom, szBuffer, sizeof(szBuffer) ); 
        pToolTipText->lpszText = szBuffer; 
    } 
} /* CDictationPad::SetTooltipText */
 
/***************************************************************************************
* CDictationPad::UpdateList *
*---------------------------*
*   Description:
*       Updates the TextRunList by inserting a TextRun that has the given range.
*       This is called when the RichEdit control notifies us of some change to the 
*       text.  The node added to the TextRunList is to be a TextRun -- not a 
*       DictationRun -- since the case of dictated text is handled separately in 
*       ProcessDictation()
*   Return:
*       Return value of CTextRunList::Insert()
****************************************************************************************/
HRESULT CDictationPad::UpdateList( long lStart, long lEnd )
{
    _ASSERTE( m_pTextRunList );
   
    // Create a new CTextRun for the text in the range lStart->lEnd
    CTextRun *pTextRun = new CTextRun();
    if ( !pTextRun )
    {
        return E_OUTOFMEMORY;
    }

    // Create a range for that text.
    CComPtr<ITextRange> pTextRange;
    HRESULT hr = m_cpTextDoc->Range( lStart, lEnd, &pTextRange );
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Hand the range to the pTextRun
    pTextRun->SetTextRange( pTextRange );

    // Any text entering the TextRunList should be the normal font.
    // This is necessary in the case of typing and hypotheses occurring
    // in the same place at the same time in the text.
    CComPtr<ITextFont> cpFont;
    hr = pTextRange->GetFont( &cpFont );
    if ( SUCCEEDED( hr ) )
    {
        cpFont->SetForeColor( tomAutoColor );
        pTextRange->SetFont( cpFont );
    }

    // Insert the new CTextRun into the TextRunList
    hr = m_pTextRunList->Insert( pTextRun );

    // The alternate UI needs to be notified of this change
    if ( SUCCEEDED( hr ) && m_pCandidateList )
    {
        m_hAltsButton = m_pCandidateList->Update( m_pTextRunList );
    }

    return hr;
}   /* CDictationPad::UpdateList */

/****************************************************************************************
* CDictationPad::ProcessMsgFilter() *
*-----------------------------------*
*   Description:
*       Called whenever one of the events that we set in our event mask
*       for the edit window happened
******************************************************************************************/
void CDictationPad::ProcessMsgFilter( MSGFILTER *pMsgFilter )
{
    switch( pMsgFilter->msg )
    {
    case WM_CHAR:
        {
            switch( pMsgFilter->wParam )
            {
                case VK_BACK:
                    if( m_dwFlags & DP_WHOLE_WORD_SEL )
                    {
                        // Delete whole words at a time

                        // Use the information on the last selection
                        // to see whether a selected range was being deleted 
                        // or whether the selection was an IP
                        long lStart = m_LastSelInfo.selchange.chrg.cpMin;
                        long lEnd = m_LastSelInfo.selchange.chrg.cpMax;
                        if ( lStart == lEnd )
                        {
                            // Select and then delete the current word
                            m_cpTextSel->MoveStart( tomWord, -1, NULL );
                            m_cpTextSel->MoveEnd( tomWord, 1, NULL );
                            m_cpTextSel->Delete(tomWord, 0, NULL);
                        }
                    }
                    break;

                case VK_ESCAPE:
                    if ( m_dwFlags & DP_IS_SPEAKING )
                    {
                        // Esc stops a playback
                        EndSpeaking();
                    }
                    break;

                default:
                    break;
            }
            break;
        }
    case WM_LBUTTONDOWN:
        {
            if( m_dwFlags & DP_WHOLE_WORD_SEL )
            {
                // Mouse capture will expand the text selection to 
                // include whole words when the left button is back up
                ::SetCapture( m_hClient );
            }

            else if ( m_dwFlags & DP_IS_SPEAKING )
            {
                // Clicking anywhere in the document should kill the speak
                EndSpeaking();
            }

            break;
        }
    case WM_LBUTTONUP:
        {
            if( m_dwFlags & DP_WHOLE_WORD_SEL )
            {
                // Get the selection, expanded to whole words
                ::ReleaseCapture();
                m_cpTextSel->Expand( tomWord, NULL );
            }

            break;
        }
    default:
        break;
    }
}   /* CDictationPad::ProcessMsgFilter */

/****************************************************************************************
* CDictationPad::ProcessSelChange() *
*-----------------------------------*
*   Description:
*       Called whenever there is an EN_SELCHANGE notification.  Updates the state
*       info and processes new non-dictated text.  
*       New text is detected by comparing the current state of the document
*       against the state when this function was last called.
*   Return:
*       S_OK
*       E_POINTER
*       return value of CDictationPad::UpdateList()
******************************************************************************************/
HRESULT CDictationPad::ProcessSelChange( SELCHANGE *pSelChange )
{
    _ASSERTE( m_cpTextSel );
    if ( !pSelChange )
    {
        return E_POINTER;
    }

    // Store the previous selection info into m_LastSelInfo
    // and the previous text length into lLastTextLen
    m_LastSelInfo = m_CurSelInfo;
    const long lLastTextLen = m_LastSelInfo.lTextLen;

    // Get the current selection info and document length
    
    m_CurSelInfo.selchange = *pSelChange;
    
    long lCurrentTextLen;
    GETTEXTLENGTHEX gtl;
    gtl.flags = GTL_PRECISE | GTL_NUMCHARS;
    gtl.codepage = CP_ACP;
    lCurrentTextLen = ::SendMessage( m_hEdit, 
        EM_GETTEXTLENGTHEX, (WPARAM) &gtl, 0 );
    m_CurSelInfo.lTextLen = lCurrentTextLen;
    
    // These variables are here for the purpose of clarity
    const SELCHANGE lastSelChange = m_LastSelInfo.selchange;
    const SELCHANGE curSelChange = m_CurSelInfo.selchange;
    
    // Move the alternates button to the new location
    if ( !( m_dwFlags & DP_SKIP_UPDATE ) )
    {
        m_hAltsButton = m_pCandidateList->Update( m_pTextRunList );
    }                            

    // The DP_SKIP_UPDATE flag is set when Dictpad is elsewhere making
    // changes to the document (or moving around the selection)
    // but handling the changes itself.
    if (( m_dwFlags & DP_SKIP_UPDATE ) || ( m_dwFlags & DP_IS_SPEAKING ))
    {
        // Nothing needs to be done
        return S_OK;
    }
    
    // Tell the recoevent manager if the selection is being
    // moved around, because it is possible that if an utterance
    // is currently being processed that we may want it to appear 
    // here.
    if ( lCurrentTextLen == lLastTextLen )
    {
        m_dwFlags |= DP_SKIP_UPDATE;
        HRESULT hr = m_pRecoEventMgr->SelNotify( *m_cpTextSel );
        m_dwFlags &= ~DP_SKIP_UPDATE;
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    // Find out if this selection is editable (it is not if there is
    // hypothesis text here)
    CComPtr<ITextRange> cpNextEditableRange;
    if ( !(m_pRecoEventMgr->IsEditable( m_cpTextSel, &cpNextEditableRange )) )
    {
        // Move the selection to an IP at the next point that can be edited
        long lStart;
        long lEnd;
        if ( cpNextEditableRange )
        {
            cpNextEditableRange->GetStart( &lStart );
            cpNextEditableRange->GetEnd( &lEnd );
        }
        else
        {
            // This is an error: means IsEditable() ran out of memory
            // (see recomgr.cpp)
            lStart = lEnd = m_pTextRunList->GetTailEnd();
        }
        ::SendMessage( m_hEdit, EM_SETSEL, lStart, lEnd );

        // We can return now, since there is no way that text could have
        // been added to a non-editable range
        return S_OK;
    }

    // The following works backwards to determine what must have happened since
    // the last time this function was called and whether there is new text
    // to deal with (or text that has been deleted).

    // Consider all combinations of {nondegenerate, degenerate} among the current 
    // selection and the last selection
    HRESULT hr = E_FAIL;
    if ( lastSelChange.chrg.cpMin == lastSelChange.chrg.cpMax )
    {
        // Last selection was degenerate (an IP).
        if ( lCurrentTextLen < lLastTextLen )
        {
             // Something was deleted; insert the current (degenerate) range
            long lStart;
            long lEnd;
            lStart = lEnd = curSelChange.chrg.cpMax;
            hr = UpdateList( lStart, lEnd );
        }
        else if ( lCurrentTextLen > m_LastSelInfo.lTextLen )
        {
            // Something has been inserted.  The start of the new text
            // is the start of the old text selection, 
            // and the end of the new text is the end of the current 
            // text selection
            long lStart = lastSelChange.chrg.cpMin;;
            long lEnd = curSelChange.chrg.cpMax;
            hr = UpdateList( lStart, lEnd );
        }
        else
        {
            // If the text length has not changed, then no text was changed
            hr = S_OK;
        }
    }
    else
    {
        // Last selection was not degenerate.

        // The new text starts at the start of the last selection and ends at the 
        // end of the current selection (think about what happens when you
        // highlight some text and type over it)
        long lStart = lastSelChange.chrg.cpMin;
        long lEnd = curSelChange.chrg.cpMax;

        // If the text length has stayed the same, unless we just did a paste, 
        // there is no new text to deal with
        if (( lLastTextLen == lCurrentTextLen ) && !(m_dwFlags & DP_JUST_PASTED_TEXT) )
        {
            return S_OK;
        }

        // Something changed, so we need to update
        hr = UpdateList( lStart, lEnd );
    }

    return hr;
}   /* CDictationPad::ProcessSelChange */

/***************************************************************************************
* CDictationPad::DoPlay *
*-----------------------*
*   Description:
*       Does a playback according to the following rules.
*       If the selection is an IP:
*           If it's at the end of the document, speak the entire document
*           Otherwise speak from the IP until the end of the document.
*       Otherwise speak the selection.
*   Return:
*       S_OK
*       S_FALSE if there is nothing to speak
*       Return value of CDictationRun::StartSpeaking()
*       Return value of CTextRunList::Speak()
****************************************************************************************/
HRESULT CDictationPad::DoPlay()
{
    _ASSERTE( m_cpTextSel && m_pTextRunList );

    if ( m_pTextRunList->GetTailEnd() == 0 )
    {
        // Nothing to speak
        return S_FALSE;
    }

    // Save the info on the selection so that we can restore it after 
    // tracking the spoken text.
    // This will occur in CDictationPad::EndSpeaking()
    long lSpeakStart = 0;
    long lSpeakEnd = 0;
    m_cpTextSel->GetStart( &lSpeakStart );
    m_cpTextSel->GetEnd( &lSpeakEnd );
    m_SpeakInfo.lSelStart = lSpeakStart;
    m_SpeakInfo.lSelEnd = lSpeakEnd;

    if( lSpeakStart == lSpeakEnd )    // denotes insertion point
    {
        if ( lSpeakStart >= m_pTextRunList->GetTailEnd() )
        {
            // IP is at the end of the document:
            // Speak from beginning to end 
            lSpeakStart = 0;
        }
        // Else lSpeakStart is already correct

        // Speak all the way to the end
        lSpeakEnd = m_pTextRunList->GetTailEnd();
    }

    // This will deactivate the grammars and set things up for 
    // a playback
    HRESULT hr = StartSpeaking( lSpeakStart, lSpeakEnd );
    if ( S_OK != hr )
    {
        EndSpeaking();
        return hr;
    }

    // Do the playback.
    // If we are in the middle of a word or a dictated phrase element,
    // the endpoints of the speak will be expanded
    hr = m_pTextRunList->Speak( *m_cpVoice, &lSpeakStart, &lSpeakEnd );
    if (S_OK != hr)
    {
        // Something went wrong, or there was nothing to speak; bail out
        EndSpeaking();
    }
    else
    {
        // If the start and end have been tweaked by m_pTextRunList->Speak(),
        // then adjust them so that text tracking looks right
        _ASSERTE( m_SpeakInfo.pSpeakRange );
        m_SpeakInfo.pSpeakRange->SetStart( lSpeakStart );
        m_SpeakInfo.pSpeakRange->SetEnd( lSpeakEnd );
    }

    return hr;

}   /* CDictationPad::DoPlay */

/***************************************************************************************
* CDictationPad::StartSpeaking *
*------------------------------*
*   Description:
*       Called when a playback is about to begin.
*       Initializes the speak info.
*   Return:
*       S_OK
*       Return value of ITextDocument::Range()
*       Return value of CDictationPad::SetGrammarState()
*       Return value of ITextRange::Collapse()
****************************************************************************************/
HRESULT CDictationPad::StartSpeaking( long lStartSpeakRange, long lEndSpeakRange )
{
    // Stop being interested in SR events for the time being
    SetSREventInterest( false );

    // Cancel any current IME stuff
    HIMC himc = ::ImmGetContext( m_hClient );
    ::ImmNotifyIME( himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0 );

    // Hide the alternates UI for the duration of the speak
    m_pCandidateList->ShowButton( false );

    // Tell the candidate list UI to turn off character input
    m_pCandidateList->StartPlayback();

    // Set the range for the playback state info
    _ASSERTE( !m_SpeakInfo.pSpeakRange );
    HRESULT hr = m_cpTextDoc->Range( lStartSpeakRange, lEndSpeakRange, 
        &(m_SpeakInfo.pSpeakRange) );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Set the flag and the toolbar buttons
    m_dwFlags |= DP_IS_SPEAKING;
    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY, 
        MAKELONG( TBSTATE_ENABLED | TBSTATE_PRESSED, 0 ) );
    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_MIC_TOGGLE,
        MAKELONG( 0, 0 ) );

    // Disable all the menu items
    HMENU hMenu = ::GetMenu( m_hClient );
    int nItems = ::GetMenuItemCount( hMenu );
    for ( int i=0; i < nItems; i++ )
    {
        ::EnableMenuItem( hMenu, i, MF_BYPOSITION | MF_GRAYED );
    }
    ::DrawMenuBar( m_hClient );

    // The focus will remain in the edit window.
    // If the user types anything, it will just appear at the beginning
    // of the spoken text.

    // Deactivate the grammars
    hr = SetGrammarState( false );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Set the speak info
    m_SpeakInfo.ulCurrentStream = 1;
    m_SpeakInfo.pCurrentNode = 
        m_pTextRunList->Find( lStartSpeakRange );
    
    _ASSERTE( !(lStartSpeakRange) || m_SpeakInfo.pCurrentNode );
    if ( lStartSpeakRange && !m_SpeakInfo.pCurrentNode )
    {
        return E_UNEXPECTED;
    }
    
    // Move the selection to an IP at the beginning of the selection for the 
    // time being (so that the highlighting can still be seen if the selection is
    // nondegenerate.
    m_cpTextSel->Collapse( tomStart );

    return S_OK;
}   /* CDictationPad::StartSpeaking */

/***************************************************************************************
* CDictationPad::EndSpeaking *
*----------------------------*
*   Description:
*       Called when a playback has ended (whether it has 
*       ended on its own or by the user interrupting it.
*       Restores the selection to what it was before 
*       the playback started
****************************************************************************************/
void CDictationPad::EndSpeaking()
{
    // This forces the voice to stop speaking
    m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL );

    // Bring back the candidate list UI
    m_pCandidateList->ShowButton( true );

    // Set the flag and the toolbar buttons
    m_dwFlags &= ~DP_IS_SPEAKING;
    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY,
        MAKELONG( TBSTATE_ENABLED, 0 ) );
    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_MIC_TOGGLE,
        MAKELONG( TBSTATE_ENABLED, 0 ) );

    // Re-enable all the menu items
    HMENU hMenu = ::GetMenu( m_hClient );
    int nItems = ::GetMenuItemCount( hMenu );
    for ( int i=0; i < nItems; i++ )
    {
        ::EnableMenuItem( hMenu, i, MF_BYPOSITION | MF_ENABLED );
    }
    ::DrawMenuBar( m_hClient );

    // Restore the default background color
    CComPtr<ITextFont> cpFont;
    HRESULT hr = m_SpeakInfo.pSpeakRange->GetFont( &cpFont );
    if ( SUCCEEDED( hr ) )
    {
        cpFont->SetBackColor( tomAutoColor );
        m_SpeakInfo.pSpeakRange->SetFont( cpFont );
    }

    // Restore the selection to what it was before the playback started
    m_cpTextSel->SetRange( m_SpeakInfo.lSelStart, 
        m_SpeakInfo.lSelEnd );
    
    // If the selection is no longer visible, then scroll it into view
    POINT pt;
    hr = m_cpTextSel->GetPoint( tomEnd | TA_BOTTOM | TA_RIGHT, 
        &(pt.x), &(pt.y) );
    if ( hr == S_FALSE )
    {
        // Out of view
        m_cpTextSel->ScrollIntoView( tomStart );
    }

    // There is no current speaking node
    m_SpeakInfo.pCurrentNode = NULL;

    // Release the current speaking range
    m_SpeakInfo.pSpeakRange->Release();
    m_SpeakInfo.pSpeakRange = NULL;

    // Restore the input focus and place the candidate button in the 
    // appropriate place
    ::SetFocus( m_hEdit );
    m_hAltsButton = m_pCandidateList->Update( m_pTextRunList );

    // Restore the mic if it was on before the playback started
    SetGrammarState( m_dwFlags & DP_MICROPHONE_ON );

    // Re-allow character input
    m_pCandidateList->EndPlayback();

    // Become interested in SR events again
    SetSREventInterest( true );

}   /* CDictationPad::EndSpeaking */

/***************************************************************************************
* CDictationPad::DoFileNew *
*--------------------------*
*   Description:
*       Opens a new file, closing any file that's already open.
*   Return:
*       S_OK
*       S_FALSE if the user cancelled
*       E_OUTOFMEMORY
*       Return value of CDictationPad::DoFileClose()
*       Return value of ITextDocument::New()
****************************************************************************************/
HRESULT CDictationPad::DoFileNew()
{
    HRESULT hr = DoFileClose();
    if ( S_FALSE == hr )
    {
        return S_FALSE;
    }

    if ( S_OK == hr )
    {
        // DoFileClose() better have deleted m_pTextRunList
        // and m_pszFile
        _ASSERTE( !m_pTextRunList && !m_pszFile );

        // Notify m_cpTextDoc that this is a new file,
        // and create a new CTextRunList off of m_cpTextDoc.
        // Note that we do not want the current TextRunList updated, since
        // we threw it away
        m_dwFlags |= DP_SKIP_UPDATE;
        hr = m_cpTextDoc->New();
        m_pTextRunList = new CTextRunList();
        m_dwFlags &= ~DP_SKIP_UPDATE;
        
        if ( !m_pTextRunList )
        {
            return E_OUTOFMEMORY;
        }
    }

    // The TextRunList now uses a different ITextDocument
    if ( SUCCEEDED( hr ) )
    {
        m_pTextRunList->SetTextDoc( m_cpTextDoc );
    }
    
    return hr;

}   /* CDictationPad::DoFileNew */


/***************************************************************************************
* CDictationPad::DoFileOpen *
*---------------------------*
*   Description:
*       Opens the file whose full path is lpFileName, or if lpFileName is NULL
*       gets the name by using the GetOpenFileName common control.
*       Attempts to open the file in the Dictpad format 
*       (an IStorage with two IStreams, one for text, and one with the
*       serialized TextRunList).
*       Failing that, opens the file as text.
*   Return:
*       S_OK
*       S_FALSE if the user cancelled
*       E_FAIL if the GetOpenFileName() function fails
*       Return value of CDictationPad::DoFileClose()
*       Return value of ITextDocument::Open()
****************************************************************************************/
HRESULT CDictationPad::DoFileOpen( LPTSTR lpFileName )
{
    if ( !m_hClient )
    {
        return E_FAIL;
    }

    // Stop listening for dictation, and do not start listening 
    // again unless the user explicitly asks to do so
    if ( m_dwFlags & DP_MICROPHONE_ON )
    {
        ::SendMessage( m_hClient, WM_COMMAND, IDM_MIC_TOGGLE, 0 );
    }

    // Close whatever file is currently open
    HRESULT hr;
    if ( (hr = DoFileClose()) != S_OK )
    {
        // User cancelled
        return hr;
    }
    
    if ( lpFileName )
    {
        m_pszFile = _tcsdup( lpFileName );
    }
    else
    {
		// Start an "Open" dialog box
        TCHAR pszFileName[ MAX_PATH ];
        *pszFileName = 0;
        OPENFILENAME ofn;
		size_t ofnsize = (BYTE*)&ofn.lpTemplateName + sizeof(ofn.lpTemplateName) - (BYTE*)&ofn;
        ZeroMemory( &ofn, ofnsize);
        ofn.lStructSize = ofnsize;
        ofn.hwndOwner = m_hClient;
        ofn.hInstance = m_hInst;
        ofn.lpstrFilter = _T("Dictpad Files (*.dpd;*.txt)\0*.dpd;*.txt\0All Files (*.*)\0*.*\0\0");
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = pszFileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = NULL;
        ofn.Flags = OFN_CREATEPROMPT;
        ofn.lpstrDefExt = _T("dpd");
        BOOL fSuccess = ::GetOpenFileName( &ofn );
    
        if ( !fSuccess )
        {
            // Check what caused fSuccess to be false
            DWORD dwErr = ::CommDlgExtendedError();
        
            if ( 0 == dwErr )
            {
                // User cancelled the open

                // If there isn't a file currently open, then put up a new file
                if ( !m_pszFile )
                {
                    hr = DoFileNew();
                }

                if ( FAILED( hr ) )
                {
                    return hr;
                }
                else
                {
                    return S_FALSE;
                }
            }
            else
            {
                // Error saving
                return E_FAIL;
            }
        }

        // Store the file name
        _ASSERTE( !m_pszFile );
        m_pszFile = _tcsdup( ofn.lpstrFile );
    }
    
    if ( !m_pszFile )
    {
        return E_OUTOFMEMORY;
    }

    // Open the new file into this document

    // We do not want to trigger updates for introducing this new text,
    // since we will take care of constructing the CTextRunList
    // for this file below.
    m_dwFlags |= DP_SKIP_UPDATE;

    // Attempt to open an IStorage from that file
    USES_CONVERSION;
    IStorage *pStorage = NULL;
    hr = ::StgOpenStorage( T2W(m_pszFile), NULL, STGM_READ | STGM_TRANSACTED,
        NULL, 0, &pStorage );

    // Attempt to open an IStream from the IStorage
    IStream *pStream = NULL;
    if ( SUCCEEDED( hr ) && pStorage )
    {
        // Open an IStream off that storage
        hr = pStorage->OpenStream( DICTPAD_TEXT, 0, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, &pStream );
    }

    // Read in from that IStream
    if ( SUCCEEDED( hr ) && pStream )
    {
        // Set up a callback function (defined below) that will accept
        // the text from the IStream and put it into the document
        EDITSTREAM es;
        es.dwCookie = (DWORD) pStream;
        es.pfnCallback = (EDITSTREAMCALLBACK) EditStreamCallbackReadIn;
        ::SendMessage( m_hEdit, EM_STREAMIN, SF_RTF, (LPARAM) &es );

        // Release the stream
        pStream->Release();
        pStream = NULL;
    }
    else
    {
        // Reading in the text in from the IStorage failed; just open this file as text
        CComVariant var;
        USES_CONVERSION;
        BSTR bstrFile = ::SysAllocString( T2W( m_pszFile ) );
        var = bstrFile;
        hr = m_cpTextDoc->Open( &var, tomReadOnly | tomOpenAlways, 0 );
        ::SysFreeString( bstrFile );
    }

    // Create a TextRunList
    _ASSERTE( !m_pTextRunList );
    m_pTextRunList = new CTextRunList();
    m_pTextRunList->SetTextDoc( m_cpTextDoc );

    if ( SUCCEEDED( hr ) && pStorage )
    {
        // We will be here if we could open the DICTPAD_TEXT stream 
        // from pStorage

        // Open the IStream with the serialized result objects
        hr = pStorage->OpenStream( DICTPAD_RECORESULTS, 0, 
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 
            0, &pStream );
    }
    if ( SUCCEEDED( hr ) && pStream )
    {
        // Recreate the TextRunList using this stream
        hr = m_pTextRunList->Deserialize( pStream, m_cpDictRecoCtxt );
    }
    //else
    if ( FAILED( hr ) || !pStream )
    {
        // All of the text in this document will be treated as typed
        // text
        hr = m_pTextRunList->CreateSimpleList();
    }

    // We need to know how long the text run list is starting out
    m_CurSelInfo.lTextLen = m_pTextRunList->GetTailEnd();
    
    if ( pStream )
    {
        pStream->Release();
    }
    if ( pStorage )
    {
        pStorage->Release();
    }

    // Turn updates back on
    m_dwFlags &= ~DP_SKIP_UPDATE;

    // Set the saved flag in m_cpTextDoc to true to indicate that
    // there are no outstanding changes
    m_cpTextDoc->SetSaved( tomTrue );

    return hr;
}   /* CDictationPad::DoFileOpen */

/***************************************************************************************
* CDictationPad::DoFileSave *
*---------------------------*
*   Description:
*       Saves the file in the Dictpad format, unless fTextOnly is set,
*       in which case the file is just saved as text
*   Return:
*       S_OK
*       E_FAIL if there is no file name to save to
*       E_OUTOFMEMORY
*       Return value of ITextDocument::Save()
*       Return value of StgCreateDocfile()
*       Return value of IStorage::CreateStream()
*       Return value of IStream::Commit()
*       Return value of CTextRunList::Serialize()
*       Return value of IStorage::Commit()
****************************************************************************************/
HRESULT CDictationPad::DoFileSave( bool fTextOnly )
{
    if ( !m_pszFile )
    {
        return E_FAIL;
    }

    // Stop listening for dictation, and do not start listening 
    // again unless the user explicitly asks to do so
    if ( m_dwFlags & DP_MICROPHONE_ON )
    {
        ::SendMessage( m_hClient, WM_COMMAND, IDM_MIC_TOGGLE, 0 );
    }

    if ( fTextOnly )
    {
        // Just do a simple save as text

        // Get the file name into a VARIANT for ITextDocument::Save()
        USES_CONVERSION;
        BSTR bstrFile = ::SysAllocString( T2W( m_pszFile ) );
        if ( !bstrFile )
        {
            return E_OUTOFMEMORY;
        }
        CComVariant var = bstrFile;

        HRESULT hr = m_cpTextDoc->Save( &var, tomCreateAlways | tomText, 0 );
        
        ::SysFreeString( bstrFile );
        
        if ( SUCCEEDED( hr ) )
        {
            m_cpTextDoc->SetSaved( tomTrue );
        }

        return hr;
    }

    // Associate an IStorage with this file
    IStorage *pStorage = NULL;
    USES_CONVERSION;
    HRESULT hr = ::StgCreateDocfile( T2W(m_pszFile), 
        STGM_CREATE | STGM_READWRITE | STGM_TRANSACTED,
        0, &pStorage );
    
    // Create a stream for the text of the document in the storage object
    IStream *pStream = NULL;
    if ( SUCCEEDED( hr ) )
    {
        hr = pStorage->CreateStream( DICTPAD_TEXT, 
            STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );
    }

    if ( SUCCEEDED( hr ) )
    {
        // Write the text out to pStream.
        // EditStreamCallbackWriteOut() is a callback function (defined below)
        // that will suck the text into pStream
        EDITSTREAM es;
        es.dwCookie = (DWORD) pStream;
        es.pfnCallback = (EDITSTREAMCALLBACK) EditStreamCallbackWriteOut;
        ::SendMessage( m_hEdit, EM_STREAMOUT, SF_RTF, (LPARAM) &es );
    
        // Commit the changes to the IStream
        hr = pStream->Commit( STGC_DEFAULT );
    }
    
    if ( pStream )
    {
        pStream->Release();
        pStream = NULL;
    }

    if ( SUCCEEDED( hr ) )
    {
        // The text document has been saved (if this doesn't succeed then
        // we will just be prompted to save next time, even if we didn't make 
        // any changes; this is not serious.
        m_cpTextDoc->SetSaved( tomTrue );
    }

    // Serialize the TextRunList into a separate stream
    _ASSERTE( m_pTextRunList );
    if ( SUCCEEDED(hr) )
    {
        hr = pStorage->CreateStream( DICTPAD_RECORESULTS,
            STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pTextRunList->Serialize( pStream, m_cpDictRecoCtxt );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = pStream->Commit( STGC_DEFAULT );
    }

    if ( pStream )
    {
        pStream->Release();
    }

    // Commit the changes to the IStorage
    if ( SUCCEEDED( hr ) )
    {
        hr = pStorage->Commit( STGC_DEFAULT );
    }

    if ( pStorage )
    {
        pStorage->Release();
    }

    return hr;
}   /* CDictationPad::DoFileSave */

/***************************************************************************************
* CDictationPad::DoFileSaveAs *
*-----------------------------*
*   Description:
*       Saves the file, using the GetSaveFileName common control. 
*   Return:
*       S_OK
*       S_FALSE if user cancelled save 
*       E_FAIL if no client window or if unsuccessful at obtaining the file name
*       Return value of CDictationPad::DoFileSave()
****************************************************************************************/
HRESULT CDictationPad::DoFileSaveAs()
{
    if ( !m_hClient )
    {
        return E_FAIL;
    }

    // Stop listening for dictation, and do not start listening 
    // again unless the user explicitly asks to do so
    if ( m_dwFlags & DP_MICROPHONE_ON )
    {
        ::SendMessage( m_hClient, WM_COMMAND, IDM_MIC_TOGGLE, 0 );
    }

    // Launch the Save As... common control
    TCHAR pszFileName[ MAX_PATH ];
    *pszFileName = 0;
    OPENFILENAME ofn;
	size_t ofnsize = (BYTE*)&ofn.lpTemplateName + sizeof(ofn.lpTemplateName) - (BYTE*)&ofn;
    ZeroMemory( &ofn, ofnsize);
    ofn.lStructSize = ofnsize;
    ofn.hwndOwner = m_hClient;
    ofn.hInstance = m_hInst;
    ofn.lpstrFilter = 
        _T("Dictpad Files (*.dpd)\0*.dpd\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0");
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = pszFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.nMaxFileTitle = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = _T("dpd");
    BOOL fSuccess = ::GetSaveFileName( &ofn );

    if ( fSuccess )
    {
        // Get the name of the file to save to
        if ( m_pszFile )
        {
            // If there is already a file name in use, ditch its name
            free( m_pszFile );
        }
        m_pszFile = _tcsdup( ofn.lpstrFile );

        // If the filter index is 2, that means the user wants a ".txt" extension
        return DoFileSave( 2 == ofn.nFilterIndex );
    }
    else
    {
        // Check what caused fSuccess to be false
        DWORD dwErr = ::CommDlgExtendedError();
        
        if ( 0 == dwErr )
        {
            // User cancelled the save
            return S_FALSE;
        }
        else
        {
            // Error saving
            return E_FAIL;
        }
    }
}   /* CDictationPad::DoFileSaveAs */


/***************************************************************************************
* CDictationPad::DoFileClose *
*----------------------------*
*   Description:
*       If there is currently an open file, closes the file (checking to see if 
*       it needs saving).
*       Always frees the file name string if successful
*   Return:
*       S_OK 
*       S_FALSE if the user cancelled the close
*       E_FAIL if there is no associated window
****************************************************************************************/
HRESULT CDictationPad::DoFileClose()
{
    if ( !m_hClient )
    {
        return E_FAIL;
    }

    if ( m_pCandidateList )
    {
        m_pCandidateList->ShowButton( false );
    }

    // Have unsaved changes been made?
    long lSaved = tomFalse;
    m_cpTextDoc->GetSaved( &lSaved );

    // If there is something to save, then confirm the close with the user
    int iResult = IDNO;
    if ( (lSaved != tomTrue) && m_pTextRunList && (m_pTextRunList->GetTailEnd() > 0) )
    {
        // Find out whether the user wants to save the current file
        TCHAR pszCaption[ MAX_LOADSTRING ];
        ::LoadString( m_hInst, IDS_APP_TITLE, pszCaption, MAX_LOADSTRING );
        int iResult = MessageBoxFromResource( m_hClient, IDS_CONFIRMCLOSE, pszCaption, 
            MB_YESNOCANCEL | MB_ICONEXCLAMATION );

        if ( IDCANCEL == iResult )
        {
            return S_FALSE;
        }
        else if ( IDYES == iResult )
        {
            // Save the file before closing it.
            int iSaved = ::SendMessage( m_hClient, WM_COMMAND, ID_FILE_SAVE, 0 );
            if ( !iSaved )
            {
                // Save cancelled or did not work
                return S_FALSE;
            }
        }
        else
        {
            // The user said don't save.
            // If we opened the file via ITextDocument::Open(), TOM will save 
            // the file anyways, unless we set the saved flag, which we do here
            m_cpTextDoc->SetSaved( tomTrue );
        }
    }
    
    // Free the memory associated with this file
    if ( m_pszFile )
    {
        free( m_pszFile );
        m_pszFile = NULL;
    }
    if ( m_pTextRunList )
    {
        delete m_pTextRunList;
        m_pTextRunList = NULL;
    }

    return S_OK;
}   /* CDictationPad::DoFileClose */

/*****************************************************************************************
* EditStreamCallbackReadIn() *
*----------------------------*
*   Description:
*       Repeatedly called by RichEdit when an EM_STREAMIN message is sent.
*       dwCookie is the IStream from which we will be reading
*       pbBuff is the buffer into which we will put the bytes from the IStream
*       cb is the number of bytes we are requested to read, and *pcb is the 
*           number of bytes actually read.
*       This function ceases to be called when it returns a nonzero value or sets
*           *pcb to 0.
*   Return values:
*       0 if successful
*       -1 if error
*******************************************************************************************/
DWORD CALLBACK EditStreamCallbackReadIn( DWORD dwCookie, LPBYTE pbBuff, ULONG cb, ULONG *pcb )
{
    if ( !dwCookie || !pbBuff || !pcb )
    {
        // Error: invalid stream
        return -1;
    }
    IStream *pStream = (IStream *) dwCookie;

    HRESULT hr = pStream->Read( pbBuff, cb, pcb );
    if ( SUCCEEDED( hr ) )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*****************************************************************************************
* EditStreamCallbackWriteOut() *
*------------------------------*
*   Description:
*       Repeatedly called by RichEdit when an EM_STREAMOUT message is sent.
*       dwCookie is the IStream to which we will be writing
*       pbBuff is the buffer from which we will get the bytes to write to the IStream
*       cb is the number of bytes we are requested to write, and *pcb is the 
*           number of bytes actually written.
*       This function ceases to be called when it returns a nonzero value or sets
*           *pcb to 0.
*   Return values:
*       0 if successful
*       -1 if error
*******************************************************************************************/
DWORD CALLBACK EditStreamCallbackWriteOut( DWORD dwCookie, LPBYTE pbBuff, ULONG cb, ULONG *pcb )
{
    if ( !dwCookie || !pbBuff || !pcb )
    {
        // Error: invalid stream
        return -1;
    }
    IStream *pStream = (IStream *) dwCookie;

    HRESULT hr = pStream->Write( pbBuff, cb, pcb );
    if ( SUCCEEDED( hr ) )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*****************************************************************************************
* MessageBoxFromResource() *
*--------------------------*
*   Description:
*       Calls a message box with text loaded from the resource with ID uID.
*   Return values:
*       error value of LoadString() or return value of MessageBox()
*******************************************************************************************/
int MessageBoxFromResource( HWND hWnd, UINT uID, LPCTSTR lpCaption, UINT uType )
{
    TCHAR pszMessageBoxText[ MAX_LOADSTRING ];
    int iRet = ::LoadString( (HINSTANCE) ::GetWindowLong( hWnd, GWL_HINSTANCE ),
        uID, pszMessageBoxText, MAX_LOADSTRING );
    if ( iRet )
    {
        iRet = ::MessageBox( hWnd, pszMessageBoxText, lpCaption, uType );
    }
    return iRet;
}   /* MessageBoxFromResource */


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

            break;
        }
    }
    return FALSE;
}   /* About */

