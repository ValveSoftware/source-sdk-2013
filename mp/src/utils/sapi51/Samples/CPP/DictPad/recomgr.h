/****************************************************************************
*	recomgr.h
*		Support for queuing insertion points and correctly placing 
*       recognized text
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/
#pragma once

#ifndef __RECOMGR_H
#define __RECOMGR_H

// A structure that represents a range waiting for a possible dictation result
typedef struct LISTENPOINT
{
    CComPtr<ITextRange>     cpRangeToReplace;   // The entire range to be replaced
                                                // when a recognition comes back
    FILETIME                ftTime;
    bool                    fFromPhraseStart;   // Indicates that this 
                                                // listening point was added
                                                // because a phrasestart 
                                                // happened
    bool                    fHasHypothesisText; // Indicates that this listen point
                                                // has some hypothesis text associated 
                                                // with it
    LISTENPOINT             *pNext;
}   LISTENPOINT;

// A queued message
typedef struct QUEUEDWM
{
    HWND                    hWnd;
    UINT                    message;
    WPARAM                  wParam;
    LPARAM                  lParam;
    QUEUEDWM                *pNext;
}   QUEUEDWM;


/*****************************************************************************
* CRecoEventMgr *
*---------------*
*   Handles the placement of recognized text according to the timing of
*   the events
******************************************************************************/
class CRecoEventMgr
{
    public:
        CRecoEventMgr( HINSTANCE hInstance );
        ~CRecoEventMgr();

        void SetTextSel( ITextSelection *pTextSel ) { m_pTextSel = pTextSel; }
        bool IsProcessingPhrase() { return m_fPhraseStarted; }
        
        // Methods called by app to notify CRecoEventMgr of events
        HRESULT PhraseStart( ITextRange &rSelRange );  
                                                    // Called when a PHRASE_START
                                                    // notification is received
        HRESULT SelNotify( ITextRange &rSelRange ); // Called whenever the 
                                                    // selection has changed
        bool IsEditable( ITextRange *pRange, ITextRange **ppNextEditableRange );       
                                                    // Returns false iff there is
                                                    // currently a reco computation
                                                    // going on and the current sel
                                                    // overlaps a listen point
        void QueueCommand( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
                                                    // Called when a WM_ messsage
                                                    // is received during a 
                                                    // reco computation
        ITextRange * Hypothesis( FILETIME ftRecoTime );
        HRESULT Recognition( FILETIME ftRecoTime, ITextRange **ppRecoRange );
        void FalseRecognition();

        // Methods called by app when phrase-computation is over
        void DoneProcessingPhrase();                // Turns off m_fPhraseStarted
                                                    // and cleans up the list
    private:
        void CleanUp();                             // Frees up the lists and 
                                                    // unleashes the waiting WM_s
        
        void DeleteAfter( LISTENPOINT *pCutoff );
                                                    // Will delete all points in 
                                                    // in the listening point list
                                                    // after pCutoff
                                                    // the head of the list
        
        HINSTANCE       m_hInst;                    // HINSTANCE for LoadString()
        bool            m_fPhraseStarted;           // true iff there is a phrase being processed
        ITextSelection  *m_pTextSel;                // The selection in dictpad's window
        LISTENPOINT     *m_pHeadLP;                 // List of listening points, most recent first
        QUEUEDWM        *m_pHeadWM;                 // Queue of WM_s
        QUEUEDWM        *m_pTailWM;

};  /* class CRecoEventMgr */

// Helper function
int CompareFiletimes( FILETIME ft1, FILETIME ft2 );
bool AreDisjointRanges( ITextRange *pRange1, ITextRange *pRange2 );
bool AreNonOverlappingRanges( ITextRange *pRange1, ITextRange *pRange2 );

#endif // __RECOMGR_H