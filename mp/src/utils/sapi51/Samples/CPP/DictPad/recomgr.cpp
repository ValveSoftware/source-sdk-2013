/****************************************************************************
*	recomgr.cpp
*		Support for queuing insertion points and correctly placing 
*       recognized text
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include <richedit.h>
#include "recomgr.h"

/****************************************************************************
* CRecoEventMgr::CRecoEventMgr *
*------------------------------*
*   Description:
*       Constructor for the recoevent manager
*****************************************************************************/
CRecoEventMgr::CRecoEventMgr( HINSTANCE hInstance ) :   
                            m_hInst( hInstance ), 
                            m_fPhraseStarted( false ),
                            m_pTextSel( NULL ),
                            m_pHeadLP( NULL ),
                            m_pHeadWM( NULL ),
                            m_pTailWM( NULL )
{
}   /* CRecoEventMgr::CRecoEventMgr */

/****************************************************************************
* CRecoEventMgr::~CRecoEventMgr *
*-------------------------------*
*   Description:
*       Destructor for the recoevent manager
*****************************************************************************/
CRecoEventMgr::~CRecoEventMgr()
{
    CleanUp();
}   /* CRecoEventMgr::~CRecoEventMgr */


/****************************************************************************
* CRecoEventMgr::PhraseStart *
*----------------------------*
*   Description:
*       Sets the flag to indicate that a phrase has been started.
*       Drops a listen point by calling SelNotify()
*   Return:
*       S_OK
*       E_OUTOFMEMORY
*       Return value of CRecoEventMgr::SelNotify()
*****************************************************************************/
HRESULT CRecoEventMgr::PhraseStart( ITextRange &rSelRange )
{
    // There should not be any phrases in progress or any listen points
    _ASSERTE( !m_fPhraseStarted && !m_pHeadLP );
    m_fPhraseStarted = true;

    // Should add a listening point right now
    HRESULT hr = SelNotify( rSelRange );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( !m_pHeadLP )
    {
        return E_OUTOFMEMORY;
    }

    m_pHeadLP->fFromPhraseStart = true;

    return S_OK;
}   /* CRecoEventMgr::PhraseStart */

/****************************************************************************
* CRecoEventMgr::SelNotify *
*--------------------------*
*   Description:
*       Called whenever the selection changes.
*       Drops a new listening point onto the list of listening points for
*       this phrase if there is a phrase currently being processed
*       being listened to.
*       Hands back the range to be eventually replaced by recognized
*       text (or deleted).
*   Return:
*       S_OK
*       S_FALSE if nothing had to be done
*       E_OUTOFMEMORY
*       Return value of ITextRange::GetDuplicate()
*       Return value of ITextRange::Collapse()
*****************************************************************************/
HRESULT CRecoEventMgr::SelNotify( ITextRange &rSelRange )
{
    // Only want to queue this listen if we are listening to a phrase
    HRESULT hr = S_FALSE;
    if ( m_fPhraseStarted )
    {
        // Get the time now
        FILETIME ftNow;
        ::CoFileTimeNow( &ftNow );

        // Does this range overlap any of the existing ranges in the list?
        LISTENPOINT *p;
        for ( p = m_pHeadLP; 
            p && AreDisjointRanges( &rSelRange, p->cpRangeToReplace ); 
            p = p->pNext )
            ;

        // If p is not NULL, that means that there is already some listen point
        // that is adjacent to or overlaps this range, so another listen point
        // is not necessary
        if ( !p )
        {
            // Add a new listen point
            LISTENPOINT *pNewPoint = new LISTENPOINT;
            if ( !pNewPoint )
            {
                return E_OUTOFMEMORY;
            }

            // Get the ranges
            hr = rSelRange.GetDuplicate( &(pNewPoint->cpRangeToReplace) );

            // Get the timestamp
            pNewPoint->ftTime = ftNow;

            // This flag will be set by PhraseStart() if appropriate
            pNewPoint->fFromPhraseStart = false;

            // No hypothesis text here yet
            pNewPoint->fHasHypothesisText = false;

            // Put this new listenpoint onto the head of the list
            pNewPoint->pNext = m_pHeadLP;
            m_pHeadLP = pNewPoint;

            long lEndOfRangeToReplace;
            pNewPoint->cpRangeToReplace->GetEnd( &lEndOfRangeToReplace );

            // The selection should be forced to the end of the range to replace;
            // this keeps the selection out of ranges that might be replaced
            // by recognized text
            m_pTextSel->SetRange( 
                    lEndOfRangeToReplace, lEndOfRangeToReplace );

            // If we got here, we were successful
            hr = S_OK;
        }
    }
    return hr;
}   /* CRecoEventMgr::SelNotify */

/****************************************************************************
* CRecoEventMgr::IsEditable *
*---------------------------*
*   Description:
*       Determines whether pRange overlaps any of the current listen points.
*       If it does, and if pNextEditableRange is non-NULL, sets 
*       pNextEditableRange to the next range that can be edited with
*       impunity
*   Return:
*       true iff the RecoEventMgr is okay with this text being edited
*****************************************************************************/
bool CRecoEventMgr::IsEditable( ITextRange *pRange, 
                               ITextRange **ppNextEditableRange )
{
    if ( !pRange )
    {
        return false;
    }
    if ( !m_fPhraseStarted )
    {
        // Always editable if there's no reco computation
        return true;
    }

    // Does this range overlap or abut any of the existing ranges in the list?
    LISTENPOINT *p;
    for ( p = m_pHeadLP; 
        p && AreDisjointRanges( pRange, p->cpRangeToReplace ); 
        p = p->pNext )
        ;

    if ( p )
    {
        // There is a phrase that overlaps or abuts this one.
        // If pRange dovetails onto the end of it, then the 
        // range is editable; otherwise it is not
        long lRangeStart = 0;
        long lLPEnd = 0;
        pRange->GetStart( &lRangeStart );
        p->cpRangeToReplace->GetEnd( &lLPEnd );

        if ( lRangeStart < lLPEnd )
        {
            // If pNextEditableRange is non-NULL, set it a degenerate range
            // at the end of the listen point range
            if ( ppNextEditableRange )
            {
                // Release whatever is already there
                if ( *ppNextEditableRange )
                {
                    (*ppNextEditableRange)->Release();
                }

                // Get a duplicate of the range to replace and collapse at the end
                HRESULT hr = p->cpRangeToReplace->GetDuplicate( ppNextEditableRange );
                if ( SUCCEEDED( hr ) )
                {
                    (*ppNextEditableRange)->Collapse( tomEnd );
                }
                else
                {
                    *ppNextEditableRange = NULL;
                }
            }
            return false;
        }
    }

    // pRange is editable
    // Leave ppNextEditableRange, whatever it is

    return true;
}   /* CRecoEventMgr::IsEditable */

/****************************************************************************
* CRecoEventMgr::QueueCommand *
*-----------------------------*
*   Description:
*       Called when the engine is currently processing a phrase.
*       Puts the command at the tail of the command queue.
*****************************************************************************/
void CRecoEventMgr::QueueCommand( HWND hWnd, UINT message, 
                                 WPARAM wParam, LPARAM lParam )
{
    if ( !m_fPhraseStarted )
    {
        // Don't queue commands unless a phrase is being processed
        return;
    }

    QUEUEDWM *pWM = new QUEUEDWM;
    if ( !pWM )
    {
        // Out of memory: This WM_COMMAND will get dropped
        return;
    }
    pWM->hWnd = hWnd;
    pWM->message = message;
    pWM->wParam = wParam;
    pWM->lParam = lParam;
    pWM->pNext = NULL;

    if ( m_pTailWM )
    {
        // Non-empty queue
        m_pTailWM->pNext = pWM;
        m_pTailWM = pWM;
    }
    else
    {
        // Empty queue
        m_pHeadWM = m_pTailWM = pWM;
    }
}   /* CRecoEventMgr::QueueCommand */
 
/****************************************************************************
* CRecoEventMgr::Hypothesis *
*---------------------------*
*   Description:
*       Called whenever a hypothesis comes back to Dictpad.
*       Looks for the latest listening point whose timestamp predates ftRecoTime
*       or for the phrase-start-generated listening point,
*       whichever came later.
*       Hands back the range to be replaced by the recognized text, or NULL
*       if this hypothesis is to be dropped (i.e. did not come between
*       a PHRASE_START and a RECOGNITION/FALSE_RECOGNITION
*   Return:
*       A pointer to an ITextRange in which the hypothesis should go.
*       NULL in case of error.
*****************************************************************************/
ITextRange * CRecoEventMgr::Hypothesis( FILETIME ftRecoTime )
{
    if ( !m_fPhraseStarted || !m_pHeadLP )
    {
        // This hypothesis did not come between a PHRASE_START and a RECOGNITION,
        // so we drop it
        return NULL;
    }

    // Since the listen points are ordered by most recent first, we are
    // looking for the first element whose timestamp predates ftRecoTime
    LISTENPOINT *p = m_pHeadLP;
    for ( p = m_pHeadLP; 
        p && (CompareFiletimes( ftRecoTime, p->ftTime ) < 0) && 
                !(p->fFromPhraseStart);
        p = p->pNext )
        ;
        
    if ( !p )
    {
        // Should not happen!  There should at least be a listen point queued
        // from PhraseStart() on this phrase
        _ASSERTE( false );
        return NULL;
    }

    // This listen point has now been marked to receive hypothesis text
    p->fHasHypothesisText = true;
    
    return p->cpRangeToReplace;
}   /* CRecoEventMgr::Hypothesis */

/****************************************************************************
* CRecoEventMgr::Recognition *
*----------------------------*
*   Description:
*       Called whenever a recognition comes back to Dictpad.
*       Looks for the latest listening point whose timestamp predates 
*       ftRecoTime or for the phrase-start-generated listening point,
*       whichever came later.
*       Deletes all earlier listening points. 
*       Hands back a duplicate of the range to be replaced by the recognized 
*       text.
*   Return:
*       S_OK
*       S_FALSE if the recognition was from some phrase that was started
*           when our grammars were inactive.
*       Return value from ITextRange::GetDuplicate()
*****************************************************************************/
HRESULT CRecoEventMgr::Recognition( FILETIME ftRecoTime, ITextRange **ppRecoRange )
{

    // A SPEI_RECOGNITION without a SPEI_PHRASE_START means that the phrase was
    // started when our grammars were inactive, so we'd like to ignore this one.
    if ( !m_fPhraseStarted || !m_pHeadLP )
    {
        return S_FALSE;
    }

    // Since the listen points are ordered by most recent first, we are
    // looking for the first element whose timestamp predates ftRecoTime
    LISTENPOINT *p = m_pHeadLP;
    for ( p = m_pHeadLP; 
        p && (CompareFiletimes( ftRecoTime, p->ftTime ) < 0) && !(p->fFromPhraseStart);
        p = p->pNext )
        ;
        
    _ASSERTE( p );
    if ( !p )
    {
        // Should not happen!
        return E_UNEXPECTED;
    }

    // Get rid of all subsequent (earlier) listening points
    DeleteAfter( p );

    // Get the range to return.
    // Make a duplicate, since this range will be destroyed before the caller
    // can AddRef it.
    HRESULT hr = p->cpRangeToReplace->GetDuplicate( ppRecoRange );

    // This listen point will now contain real (non-hypothesis) text
    p->fHasHypothesisText = false;

    // p will get cleaned up later when DoneProcessingPhrase() is called

    return hr;
}   /* CRecoEventMgr::Recognition */

/****************************************************************************
* CRecoEventMgr::FalseRecognition *
*---------------------------------*
*   Description:
*       Called whenever a false recognition comes back to Dictpad.
*       Finds the phrase-start-marked listen point
*       and deletes everything after it
*****************************************************************************/
void CRecoEventMgr::FalseRecognition()
{
    if ( !m_fPhraseStarted || !m_pHeadLP )
    {
        // This means that this is a RECO_OTHER_CONTEXT, or a recognition for
        // a grammar other than dictation within our own context, neither of
        // which we care about
        return;
    }

    // Clean up anything that happened before the start of this utterance
    for ( LISTENPOINT *p = m_pHeadLP; p && !(p->fFromPhraseStart); p = p->pNext )
    {
        ;
    }
    if ( p )
    {
        DeleteAfter( p );
    }

    // p will get cleaned up later when DoneProcessingPhrase() is called

}   /* CRecoEventMgr::FalseRecognition */

/****************************************************************************
* CRecoEventMgr::DoneProcessingPhrase *
*-------------------------------------*
*   Description:
*       Sets m_fPhraseStarted to false.
*       Calls CleanUp() to clean up the listen point list 
*       and WM_COMMAND queue.
*****************************************************************************/
void CRecoEventMgr::DoneProcessingPhrase()
{
    m_fPhraseStarted = false;
    CleanUp();
}   /* CRecoEventMgr::DoneProcessingPhrase */

/****************************************************************************
* CRecoEventMgr::CleanUp *
*------------------------*
*   Description:
*       Deletes all nodes.
*       Resends the queued messages and cleans up the queue
*****************************************************************************/
void CRecoEventMgr::CleanUp()
{
    m_fPhraseStarted = false;
    
    // Remove any hypothesis text
    for ( LISTENPOINT *p = m_pHeadLP; p; p = p->pNext )
    {
        if ( p->fHasHypothesisText && p->cpRangeToReplace )
        {
            p->cpRangeToReplace->SetText( L"" );
        }
    }

    // Clean up the listen point list
    if ( m_pHeadLP )
    {
        DeleteAfter( m_pHeadLP );
        delete m_pHeadLP;
        m_pHeadLP = NULL;
    }

    // Resend the WM_x messages that had been waiting during the computation
    // on the phrase and clean up the queue
    QUEUEDWM *pWM;
    while ( m_pHeadWM )
    {
        pWM = m_pHeadWM;
        ::SendMessage( pWM->hWnd, pWM->message, pWM->wParam, pWM->lParam );

        m_pHeadWM = m_pHeadWM->pNext;
        delete pWM;
    }
    m_pHeadWM = m_pTailWM = NULL;
}   /* CRecoEventMgr::CleanUp */


/****************************************************************************
* CRecoEventMgr::DeleteAfter *
*----------------------------*
*   Description:
*       Deletes all nodes after pCutoff.
*****************************************************************************/
void CRecoEventMgr::DeleteAfter( LISTENPOINT *pCutoff )
{
    if ( pCutoff )
    {
        LISTENPOINT *p, *pNextInLine;
        for ( p = pCutoff->pNext; p; p = pNextInLine )
        {
            pNextInLine = p->pNext;
            delete p;
        }
    }

    pCutoff->pNext = NULL;
}   /* CRecoEventMgr::DeleteAfter */

/****************************************************************************
* CompareFiletimes *
*------------------*
*   Return:
*       -1 if ft1 is earlier than ft2
*       0 if ft1 == ft2
*       1 if ft1 is greater than ft2
*****************************************************************************/
int CompareFiletimes( FILETIME ft1, FILETIME ft2 )
{
    if ( ft1.dwHighDateTime < ft2.dwHighDateTime )
    {
        return -1;
    }
    else if ( ft1.dwHighDateTime > ft2.dwHighDateTime )
    {
        return 1;
    }
    else
    {
        if ( ft1.dwLowDateTime < ft2.dwLowDateTime )
        {
            return -1;
        }
        else if ( ft1.dwLowDateTime > ft2.dwLowDateTime )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}   /* CompareFiletimes */

/****************************************************************************
* AreDisjointRanges *
*-------------------*
*   Description:
*       Suppose Range1 has limits cpMin1, cpMax1.
*       Suppose Range2 has limits cpMin2, cpMax2.
*       Range1 and Range2 are disjoint iff 
*           cpMin2 > cpMax1 OR cpMax2 < cpMin1.
*       That is, they are disjoint iff there is no overlap and they do not
*       dovetail.
*   Return:
*       true iff the two ranges neither overlap nor abut.
*****************************************************************************/
bool AreDisjointRanges( ITextRange *pRange1, ITextRange *pRange2 )
{
    if ( !( pRange1 && pRange2 ) )
    {
        return true;
    }

    long cpMin1, cpMax1, cpMin2, cpMax2;
    pRange1->GetStart( &cpMin1 );
    pRange1->GetEnd( &cpMax1 );
    pRange2->GetStart( &cpMin2 );
    pRange2->GetEnd( &cpMax2 );

    return (( cpMin2 > cpMax1 ) || ( cpMax2 < cpMin1 ));
}   /* AreDisjointRanges */
