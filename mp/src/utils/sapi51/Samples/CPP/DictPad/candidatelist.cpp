/******************************************************************************
*	candidatelist.cpp 
*       Implementation details for the CCandidateList object which is a
*       class that manages the recognized alternatives for dictated
*       text.
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#include "stdafx.h"
#include "candidatelist.h"
#include "dictpad.h"

// Multi-Language Header File
#include <mlang.h>

#define MAX_ALTS_DISPLAYED 10

/*****************************************************************************
* CCandidateList::CCandidateList *
*--------------------------------*
*   Description:
*       Constructor for the CCandidateList class.
*       Modifies the RICHEDIT_CLASS of the main 
*       application window to use our callback function 
*       and registers that window class.
*******************************************************************************/
CCandidateList::CCandidateList( HWND hClient, CRecoEventMgr &rRecoMgr ) :
                                        m_pwcParentClass( NULL ),
                                        m_hMainClientWindow( hClient ),
                                        m_hParent( NULL ),
                                        m_hAltsList( NULL ),
                                        // For now, set the langid to default.
                                        // LangID can be set to something else
                                        // later.
                                        m_langid( ::GetUserDefaultLangID() ),   
                                        m_fMakeUIVisible( true ),
                                        m_fPlaybackInProgress( false ),
                                        m_pCurrentDictRun( NULL ),
                                        m_cpTextSel( NULL ),
                                        m_pRecoMgr( &rRecoMgr ),
                                        m_hFont( NULL ),
                                        m_ulNumAltsDisplayed( 0 )
{
    m_hInst = (HINSTANCE) ::GetWindowLong( m_hMainClientWindow, GWL_HINSTANCE );

    // Register a window class that is like the rich edit control class in every way
    // except that it has space for an extra pointer and calls our wndproc
    WNDCLASS wcModifiedRichEdit;
    ::GetClassInfo( m_hInst, RICHEDIT_CLASS, &wcModifiedRichEdit );

    // Bump the space by the size of a pointer
    m_cbOffset = wcModifiedRichEdit.cbWndExtra;
    int cbExtraSpace = sizeof(CCandidateList *);
    wcModifiedRichEdit.cbWndExtra += cbExtraSpace;

    // Change the wndproc, storing the original wndproc
    m_wpOrigWndProc = wcModifiedRichEdit.lpfnWndProc;
    wcModifiedRichEdit.lpfnWndProc = CandidateUIProc;

    wcModifiedRichEdit.lpszClassName = MODIFIED_RICHEDIT_NAME;
    ATOM atomRet = ::RegisterClass( &wcModifiedRichEdit );

    if ( atomRet )
    {
        m_pwcParentClass = new WNDCLASS;
        *m_pwcParentClass = wcModifiedRichEdit;
    }
    
    GetFontSettings();
}   /* CCandidateList::CCandidateList */

/******************************************************************************
* CCandidateList::~CCandidateList *
*---------------------------------*
*   Description:
*       Destructor for the CCandidateListclass
*******************************************************************************/
CCandidateList::~CCandidateList()
{
    if ( m_pwcParentClass )
    {
        delete m_pwcParentClass;
    }

    if (m_hFont)
    {
        DeleteObject(m_hFont);
    }

    ::DestroyWindow( m_hButton );
    DoneWithAltsList();
}   /* CCandidateList::~CCandidateList */

/******************************************************************************
* CCandidateList::GetFontSettings *
*---------------------------------*
*   Description:
*       Gets the font settings for the candidate list UI
*******************************************************************************/
void CCandidateList::GetFontSettings()
{
    int iHeight = 0;    // Will cause CreateFont() to use default in case we
                        // don't get the height below
    HDC hdc = GetDC(m_hMainClientWindow);
    HFONT hfontNew = 0;

    // Get the height of the text
    if (hdc)
    {
        TEXTMETRIC tm;
        
        if (GetTextMetrics(hdc, &tm))
        {
            iHeight = tm.tmHeight;
        }
        
        ReleaseDC(m_hMainClientWindow, hdc);
    }

    // Pick an appropriate font.  On Windows 2000, let the system fontlink.
    if (NT5orGreater())
    {
        hfontNew = CreateFont(iHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                              DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY,
                              DEFAULT_PITCH,
                              TEXT("Microsoft Sans Serif"));
    }
    else
    {
        LCID lcid = MAKELCID( m_langid, SORT_DEFAULT );
        UINT uiCodePage = SpCodePageFromLcid( lcid );

        CComPtr<IMultiLanguage> cpMultiLanguage;
        MIMECPINFO MimeCpInfo;

        if (   SUCCEEDED(cpMultiLanguage.CoCreateInstance(CLSID_CMultiLanguage))
            && SUCCEEDED(cpMultiLanguage->GetCodePageInfo(uiCodePage, &MimeCpInfo)))
        {
            USES_CONVERSION;

            hfontNew = CreateFont(iHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                  MimeCpInfo.bGDICharset,
                                  OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY,
                                  DEFAULT_PITCH,
                                  W2T(MimeCpInfo.wszProportionalFont));

        }
    }

    if (hfontNew)
    {
        if (m_hFont)
        {
            DeleteObject(m_hFont);
        }

        m_hFont = hfontNew;
    }            
}   /* CCandidateList::GetFontSettings */

/******************************************************************************
* CCandidateList::SetParent *
*---------------------------*
*   Description:
*       Called after the parent window is created.
*       Sets the parent window member and creates a button for the 
*       alternates UI
*******************************************************************************/
void CCandidateList::SetParent( HWND hParent )
{
    m_hParent = hParent;
    m_hButton = ::CreateWindow( _T("BUTTON"),
                    _T(""),
                    WS_CHILD | BS_DEFPUSHBUTTON,
                    0,
                    0,
                    BUTTON_WIDTH,
                    BUTTON_HEIGHT,
                    m_hParent,
                    NULL,
                    m_hInst,
                    NULL );
}   /* CCandidateList::SetParent */

/******************************************************************************
* CCandidateList::SetLangID *
*---------------------------*
*   Description:
*       Sets the m_langid and calls GetFontSettings()
*******************************************************************************/
void CCandidateList::SetLangID( LANGID langid )
{
    m_langid = langid;
    GetFontSettings();
}   /* CCandidateList::SetLangID */

/******************************************************************************
* CCandidateList::Update *
*------------------------*
*   Description:
*       Called whenever an EN_SELCHANGE notification is received.
*       Shows the alternates button if the entire selection is
*       dictated text from the same phrase.
*   Return:
*       If the entire selection is within a single dictated text range, returns
*       the handle to the button and shows the button.
*       Otherwise returns NULL and hides the button.
********************************************************************************/
HWND CCandidateList::Update( CTextRunList *pTextRunList )
{
    // Clicking off the alternates list should dismiss it
    if ( m_hAltsList )
    {
        DoneWithAltsList();
    }
    
    m_pCurrentDictRun = NULL;

    ::ShowWindow( m_hButton, SW_HIDE );
    ::InvalidateRect( m_hParent, NULL, true );
    
    if ( !m_fMakeUIVisible )
    {
        // The button should not be displayed
        return NULL;
    }

    if ( !m_cpTextSel || !pTextRunList || !m_hParent )
    {
        // error
        return NULL;
    }

    long lStart;
    long lEnd;
    HRESULT hr = m_cpTextSel->GetStart( &lStart );
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpTextSel->GetEnd( &lEnd );
    }

    // Show the alternates UI only if there is no reco computation
    if ( SUCCEEDED( hr ) && m_pRecoMgr && !(m_pRecoMgr->IsProcessingPhrase()) )
    {
        PTEXTRUNNODE pNode = pTextRunList->Find( lStart );
        if ( pNode && pNode->pTextRun->IsDict() && 
            ( lEnd <= pNode->pTextRun->GetEnd() ) )
        {
            // The selection is completely contained within this dictated run.
            // The button should appear at the lower right-hand
            // corner of the selected text
            POINT pt;
            hr = m_cpTextSel->GetPoint( tomEnd | TA_BASELINE | TA_LEFT, 
                &(pt.x), &(pt.y) );
            if ( SUCCEEDED( hr ) )
            {
                // Move the button to the new location
                ::ScreenToClient( m_hParent, &pt );
                ::MoveWindow( m_hButton, pt.x, pt.y, 
                    BUTTON_WIDTH, BUTTON_HEIGHT, true );
                ::ShowWindow( m_hButton, SW_SHOW );

                // We know that this node contains a dictation run (see above)
                m_pCurrentDictRun = 
                    static_cast<CDictationRun *>(pNode->pTextRun);

                return m_hButton;
            }
        }
    }

    // Otherwise, hide the window and return NULL
    return NULL;
}   /* CCandidateList::Update */

/******************************************************************************
* CCandidateList::ShowAlternates *
*--------------------------------*
*   Description:
*       Called whenever the alternates button is clicked.
*       Hides the alternates button and displays the 
*       alternates dialog box.
*       When the alternates dialog box is done, shows the button again.
********************************************************************************/
void CCandidateList::ShowAlternates()
{
    _ASSERTE( m_pCurrentDictRun );
    _ASSERTE( m_cpTextSel );
    
    // Get the alternates.  The text for the alternates
    // will have been CoTaskMemAlloced
    WCHAR *apszAltsText[ALT_REQUEST_COUNT];
    bool apfFitsInRun[ ALT_REQUEST_COUNT ];
    long lAltStart;
    long lAltEnd;
    ULONG cAltsReturned = 0;
    HRESULT hr = m_pCurrentDictRun->GetAlternatesText( 
        m_cpTextSel, ALT_REQUEST_COUNT, &lAltStart, &lAltEnd, 
        apszAltsText, apfFitsInRun, &cAltsReturned );

    if ( FAILED(hr) )
    {
        return;
    }

    // Check to make sure that at least one alternate is displayable
    bool fDisplayableAlts = false;
    for ( ULONG ulAlt = 0; !fDisplayableAlts && (ulAlt < cAltsReturned); ulAlt++ )
    {
        fDisplayableAlts = apfFitsInRun[ ulAlt ];
    }
    if ( !fDisplayableAlts )
    {
        // No alternates to display: Won't be doing anything here
        return;
    }

    // Hide and disable the alternates button
    _ASSERTE( m_hParent );
    BOOL fVisible = ::ShowWindow( m_hButton, SW_HIDE );
    _ASSERTE( fVisible );
    ::EnableWindow( m_hButton, false );
    ::InvalidateRect( m_hParent, NULL, true );

    // Create a window for the alternates list.
    // The alternates list should appear at the lower 
    // right-hand corner of the text selection
    POINT pt;
    m_cpTextSel->GetPoint( tomEnd | TA_BASELINE | TA_LEFT, 
        &(pt.x), &(pt.y) );
    ::ScreenToClient( m_hParent, &pt );
    m_hAltsList = ::CreateWindow( _T("LISTBOX"), 
        _T(""), 
        WS_CHILD | WS_DLGFRAME | LBS_OWNERDRAWFIXED | LBS_NOTIFY | WS_VSCROLL, 
        pt.x, pt.y,             // Dimensions will be determined by
                                // number and width of alternates
        0, 0, 
        m_hParent, 
        (HMENU) IDC_LIST_ALTS, 
        m_hInst, 
        NULL ); 

    // Get the font with which to draw the alternates (this is an
    // owner-drawn listbox)
    HDC     hdc = ::GetDC( m_hAltsList );
    HGDIOBJ hfontOld = m_hFont ? SelectObject(hdc, m_hFont) : 0;

    // Populate the alternates list. 
    ULONG   ulAltIndex;
    ULONG   ulNumAltsDisplayed = 0;
    WCHAR   **ppszCoMemText;
    SIZE    size;
    int     cxMaxWidth = 0;
    for ( ulAltIndex = 0, ppszCoMemText = apszAltsText; 
        ulAltIndex < cAltsReturned; 
        ulAltIndex++, ppszCoMemText++ )
    {
        if ( !apfFitsInRun[ ulAltIndex ] )
        {
            // This alt will not be displayed, since the alt covers elements
            // not in this run
            continue;
        }

        // Keep track of which alternate is going into location
        // cAltsListed in the alternates list
        m_aulAltIndices[ ulNumAltsDisplayed ] = ulAltIndex;

        // Keep track of the widest alt so far
        _ASSERTE( *ppszCoMemText );
        ::GetTextExtentPointW( 
            hdc, *ppszCoMemText, wcslen( *ppszCoMemText ), &size );
        cxMaxWidth = max( cxMaxWidth, size.cx );

        // Owner-drawn list box, so the string is stored as item data (as a WCHAR *)
        WCHAR *pwszListItem = wcsdup( *ppszCoMemText );
        ::SendMessage( m_hAltsList, LB_INSERTSTRING, ulNumAltsDisplayed, (LPARAM) pwszListItem );

        ulNumAltsDisplayed++;
    }

    // Keep track of how many alternates there are
    m_ulNumAltsDisplayed = ulNumAltsDisplayed;

    // Get the old font back
    hfontOld ? SelectObject(hdc, hfontOld) : NULL;
    ::ReleaseDC( m_hAltsList, hdc );

    // Bump up the maximum width by the list box border width and the 
    // vertical scroll bar width if necessary
    cxMaxWidth += 2 * GetSystemMetrics( SM_CXDLGFRAME );
    if (ulNumAltsDisplayed > MAX_ALTS_DISPLAYED)
    {
        cxMaxWidth += GetSystemMetrics( SM_CXVSCROLL );
    }
    
    // The alternates text was CoTaskMemAlloced, so we must free
    // it now
    ULONG ul;
    for ( ul = 0, ppszCoMemText = apszAltsText; 
        ul < cAltsReturned; 
        ul++, ppszCoMemText++ )
    {
        if ( *ppszCoMemText )
        {
            ::CoTaskMemFree( *ppszCoMemText );
        }
    }

    // Resize the window to the correct width
    // The alternates list should always go inside the parent window,
    // so if the list is too wide, move it to the left.
    RECT rectButton;
    RECT rectParent;
    POINT ptTopLeft;
    ::GetWindowRect( m_hButton, &rectButton );
    ::GetWindowRect( m_hParent, &rectParent );
    int cyItemHeight = ::SendMessage( m_hAltsList, LB_GETITEMHEIGHT, 0, 0 );
    int cyHeight = __min(((int) ulNumAltsDisplayed + 1) * cyItemHeight, 
                            (MAX_ALTS_DISPLAYED + 1) * cyItemHeight);
    ptTopLeft.x = __min( rectButton.left, rectParent.right - cxMaxWidth );
    ptTopLeft.y = rectButton.top;
    ::ScreenToClient( m_hParent, &ptTopLeft );
    ::MoveWindow( m_hAltsList, 
        ptTopLeft.x,
        ptTopLeft.y,
        cxMaxWidth, 
        cyHeight, 
        true );

        
    // Display the alternates list
    ::ShowWindow( m_hAltsList, SW_SHOW );

    // Highlight the text for the first alternate displayed
    MakeTextSelReflectAlt(0);

    ::SetFocus( m_hAltsList );
}   /* CCandidateList::ShowAlternates */

/******************************************************************************
* CCandidateList::ShowButton *
*----------------------------*
*   Description:
*       Shows/hides the alternates button.
********************************************************************************/
void CCandidateList::ShowButton( bool bShow )
{
    ::ShowWindow( m_hButton, bShow ? SW_SHOW : SW_HIDE );
    ::EnableWindow( m_hButton, bShow );
    ::InvalidateRect( m_hParent, NULL, true );
    m_fMakeUIVisible = bShow;
}   /* CCandidateList::ShowButton */

/******************************************************************************
* CCandidateList::MakeTextSelReflectAlt *
*---------------------------------------*
*   Description:
*       Called when an item in the alternates list is selected.
*       Adjusts the text selection in order to jive with whichever
*       elements that alternate replaces
********************************************************************************/
void CCandidateList::MakeTextSelReflectAlt( ULONG ulAltIndexInList )
{
    _ASSERTE( m_pCurrentDictRun );
    _ASSERTE( m_cpTextSel );
    _ASSERTE( ulAltIndexInList < m_ulNumAltsDisplayed );

    long lSelStart;
    long lSelEnd;

    HRESULT hr = m_pCurrentDictRun->GetAltEndpoints(
        m_aulAltIndices[ ulAltIndexInList ], &lSelStart, &lSelEnd );

    if ( SUCCEEDED(hr) )
    {
        // The WM_STOPUPDATE message tells Dictpad that there is no
        // new text, so it does not need to process this selection change
        ::SendMessage( m_hMainClientWindow, WM_STOPUPDATE, 0, 0 );
        m_cpTextSel->SetStart( lSelStart );
        m_cpTextSel->SetEnd( lSelEnd );
        ::SendMessage( m_hMainClientWindow, WM_STARTUPDATE, 0, 0 );
    }
}   /* CCandidateList::MakeTextSelReflectAlt */

/******************************************************************************
* CCandidateList::AlternateChosen *
*---------------------------------*
*   Description:
*       Called when the user selects an alternate from the alternates UI.
*       Notifies the appropriate CDictationRun that the alt has been
*       chosen and changes the text.
*       Dismisses the alternates list, since choosing an alternate means
*       the user is done with the alternates list.
********************************************************************************/
void CCandidateList::AlternateChosen( ULONG ulChosenAltInList )
{
    _ASSERTE( m_hAltsList );
    _ASSERTE( m_pCurrentDictRun );
    _ASSERTE( m_cpTextSel );
    _ASSERTE( ulChosenAltInList < m_ulNumAltsDisplayed );

    ::SendMessage( m_hMainClientWindow, WM_STOPUPDATE, 0, 0 );
    HRESULT hr = m_pCurrentDictRun->ChooseAlternate( 
        m_aulAltIndices[ ulChosenAltInList ] );
    ::SendMessage( m_hMainClientWindow, WM_STARTUPDATE, 0, 0 );

    // The main window should update the alternates button
    ::SendMessage( m_hMainClientWindow, WM_UPDATEALTSBUTTON, 0, 0 );

    DoneWithAltsList();
}   /* CCandidateList::AlternateChosen */

/******************************************************************************
* CCandidateList::DoneWithAltsList *
*----------------------------------*
*   Description:
*       Called when the alternates list no longer need be displayed.
*       Either the user has chosen an alternate or the user has clicked
*       off the list to dismiss it.
********************************************************************************/
void CCandidateList::DoneWithAltsList()
{
    if ( !m_hAltsList )
    {
        return;
    }

    int cItems = ::SendMessage( m_hAltsList, LB_GETCOUNT, 0, 0 );
    for ( int i = 0; i < cItems; i++ )
    {
        // Free the memory used for the strings in the list box
        WCHAR *pwszListItem = (WCHAR *) ::SendMessage(
            m_hAltsList, LB_GETITEMDATA, i, 0 );
        if ( pwszListItem )
        {
            free( pwszListItem );
            ::SendMessage( m_hAltsList, LB_SETITEMDATA, i, NULL );
        }
    }

    m_ulNumAltsDisplayed = 0;

    ::DestroyWindow( m_hAltsList );
    m_hAltsList = 0;
    m_pCurrentDictRun = NULL;

    // Bring the button back
    ::EnableWindow( m_hButton, true );
}   /* CCandidateList::DoneWithAltsList */

/******************************************************************************
* CandidateUIProc() *
*-------------------*
*   Description:
*       Subclassing procedure for the richedit control so that it can process
*       messages from the candidate list UI.
*******************************************************************************/
LRESULT APIENTRY CandidateUIProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    static int                      cbOffset = 0;   // Where the data to be used by this 
                                                    // wndproc starts
    // The window long points to the associated instance of the candidate list UI
    CCandidateList *pCandidateList = 
        ( CCandidateList * ) ::GetWindowLong( hWnd, cbOffset );

    switch( message )
    {
        case WM_NCCREATE:
        {
            // lParam points to a CREATESTRUCT with the CCandidateList * object 
            // as its lpCreateParams
            pCandidateList = (CCandidateList *)
                                ((LPCREATESTRUCT) lParam)->lpCreateParams;
    
            // Get the class info and find the offset that will give us 
            // the very end of the extra space
            WNDCLASS wc;
            TCHAR pszClassName[ MAX_CLASS_NAME ];
            ::GetClassName( hWnd, pszClassName, MAX_CLASS_NAME );
            ::GetClassInfo( (HINSTANCE) ::GetWindowLong( hWnd, GWL_HINSTANCE ),
                pszClassName, &wc );
            _ASSERTE( wc.cbWndExtra >= sizeof( CCandidateList *) );
            if ( wc.cbWndExtra < sizeof( CCandidateList * ) )
            {
                // No space for the CCandidateList * in the window long
                return -1;
            }
            cbOffset = wc.cbWndExtra - sizeof( CCandidateList *);
            
            // Set the window long
            ::SetWindowLong( hWnd, cbOffset, (long) pCandidateList );

            // Tell the candidate list about the parent window
            pCandidateList->SetParent( hWnd );

            break;
        }

        case WM_KEYDOWN:
        case WM_CHAR: 
            // Ignore keystrokes as a recognition is being processed
            if ( pCandidateList->m_pRecoMgr->IsProcessingPhrase() 
                || ( pCandidateList->IsPlaybackInProgress() && ( VK_ESCAPE != wParam )) )
            {
                // Dropping these messages
                return 0;
            }
            break;
        
        case WM_IME_STARTCOMPOSITION:
            if ( pCandidateList->m_pRecoMgr->IsProcessingPhrase() 
                || ( pCandidateList->IsPlaybackInProgress() && ( VK_ESCAPE != wParam )) )
            {
                HIMC himc = ::ImmGetContext( hWnd );
                ::ImmNotifyIME( himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0 );
            }
            break;

        case WM_COMMAND:
            switch ( HIWORD( wParam ) )
            {
                case BN_CLICKED:
                    // Clicking on the alternates button
                    pCandidateList->ShowAlternates();
                    break;

                case LBN_SELCHANGE:
                    // Selecting a different alternate in the alternates list
                    pCandidateList->MakeTextSelReflectAlt(
                        ::SendMessage( pCandidateList->m_hAltsList, LB_GETCURSEL, 0, 0 ) );
                    break;

                case LBN_DBLCLK:
                    // Choosing an alternate
                    pCandidateList->AlternateChosen( 
                        ::SendMessage( pCandidateList->m_hAltsList, LB_GETCURSEL, 0, 0 ) );
                    break;

                case LBN_SETFOCUS:
                    // When the alternates list first appears, we give it the input 
                    // focus.  The first alternate should start out selected
                    ::SendMessage( pCandidateList->m_hAltsList, LB_SETCURSEL, 0, 0 );
                    break;

                default:
                    break;
            }
            break;

        case WM_DRAWITEM:
            // Since we have an owner-draw list box we need to process this
            // message
            if (wParam == IDC_LIST_ALTS)
            {
                LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

                HGDIOBJ hfontOld = pCandidateList->m_hFont ? 
                    SelectObject( pdis->hDC, pCandidateList->m_hFont ) : NULL;
                UINT oldTextAlign = GetTextAlign(pdis->hDC);

                UINT options = ETO_OPAQUE | ETO_CLIPPED;

                // Strings are stored as item data
                HWND hwndList = pCandidateList->m_hAltsList;
                WCHAR *pwszItemText = (WCHAR *) ::SendMessage( hwndList,
                    LB_GETITEMDATA, pdis->itemID, 0 );
              
                int cStringLen = wcslen( pwszItemText );

                SetTextAlign(pdis->hDC, TA_UPDATECP);
                MoveToEx(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, NULL);
                ExtTextOutW(pdis->hDC,
                            pdis->rcItem.left, pdis->rcItem.top,
                            options,
                            &pdis->rcItem,
                            pwszItemText, 
                            cStringLen,
                            NULL);


                SetTextAlign(pdis->hDC, oldTextAlign);

                if (hfontOld)
                {
                    SelectObject(pdis->hDC, hfontOld);
                }
            }
            break;

        default:
            break;

    }

    // Call the original WndProc
    return ::CallWindowProc( pCandidateList->m_wpOrigWndProc, 
        hWnd, message, wParam, lParam );
}   /* CandidateUIProc */

