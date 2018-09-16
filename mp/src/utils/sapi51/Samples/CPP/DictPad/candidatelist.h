/****************************************************************************
*   candidatelist.h
*       This module contains the definition support for CCandidateList,
*       the candidate list UI for Dictpad
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/

#ifndef __CANDIDATELIST_H
#define __CANDIDATELIST_H
#pragma once

#include <richedit.h>
#include <richole.h>
#include "resource.h"
#include "sapi.h"
#include "TextRunList.h"
#include "recomgr.h"

#define BUTTON_WIDTH            16
#define BUTTON_HEIGHT           16
#define MODIFIED_RICHEDIT_NAME  _T("modified_richedit")  
#define MAX_CLASS_NAME          50


/****************************************************************************
* CCandidateList
*       Handles the UI for the alternate list
*****************************************************************************/
class CCandidateList
{
public:
    CCandidateList( HWND hClient, CRecoEventMgr &rRecoMgr );
    ~CCandidateList();

    void GetFontSettings();

    const WNDCLASS *GetParentClass() 
        { return m_pwcParentClass; }  
    void SetParent( HWND hParent );
    void SetTextSel( ITextSelection *pTextSel ) 
        { m_cpTextSel = pTextSel; }
    void SetLangID( LANGID langid );

    HWND Update( CTextRunList *pTextRunList );
    void ShowButton( bool fShow );

    void StartPlayback() {m_fPlaybackInProgress = true;};
    void EndPlayback() {m_fPlaybackInProgress = false;};
    bool IsPlaybackInProgress() { return m_fPlaybackInProgress; };

    friend LRESULT APIENTRY CandidateUIProc( 
        HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

    bool FHasAlternates() { return (m_pCurrentDictRun != NULL) && (m_cpTextSel != NULL); };
    
private:
    void ShowAlternates();
    void MakeTextSelReflectAlt( ULONG ulAltIndexInList );
    void AlternateChosen( ULONG ulChosenAltInList );
    void DoneWithAltsList();

// Data members

    WNDCLASS    *m_pwcParentClass;  // The window class registered by the  
                                    // call to the constructor for a candidate list
                                    // (a modified RichEdit control).
                                    // The parent window will belong to this window class.
    HINSTANCE   m_hInst; 
    HWND        m_hMainClientWindow;// Main application window
    HWND        m_hParent;          // Parent window of the UI (richedit control)
    int         m_cbOffset;         // Offset of the extra space in the parent window 
    WNDPROC     m_wpOrigWndProc;    // The wndproc of the parent window that will be
                                    // subclassed
    HWND        m_hButton;          // The button that triggers the alternates UI
    HWND        m_hAltsList;        // Handle to the alternates list
    LANGID      m_langid;           // Language of dictation
    HFONT       m_hFont;            // Font for the owner-drawn alternates listbox
    bool        m_fMakeUIVisible;   // If false, button and list not shown
    bool        m_fPlaybackInProgress;
                                    // If true, richedit window needs to ignore keystrokes

    CDictationRun   *m_pCurrentDictRun;  
                                    // The last dictation run that the alternates button
                                    // corresponded to

    CComPtr<ITextSelection>     m_cpTextSel;
    CRecoEventMgr               *m_pRecoMgr;
    ULONG                       m_aulAltIndices[ ALT_REQUEST_COUNT ];
    ULONG                       m_ulNumAltsDisplayed;
                                    // A map from indices in the list to alternate indices
};

// Subclassing wndproc for the richedit control (parent window of the candidate UI
LRESULT APIENTRY CandidateUIProc( 
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

// Helper function for locale and font settings
inline BOOL NT5orGreater()
{
    DWORD dwVersion = GetVersion();

    return !(dwVersion & 0x80000000) && LOBYTE(LOWORD(dwVersion)) >= 5;
}



#endif // __CANDIDATELIST_H