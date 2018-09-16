/******************************************************************************
*   TextRunList.cpp 
*       This module contains the implementation details of the CTextRunList
*       class which is responsible for maintaining the collection of all the
*       text runs in the DictPad program as a doubly linked list.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#include "stdafx.h"
#include "TextRunList.h"

/**********************************************************************
* CTextRunList::CTextRunList *
*----------------------------*
*	Description:  
*		Constructor.
**********************************************************************/
CTextRunList::CTextRunList( ITextDocument *pTextDoc ) : 
                m_pHead( NULL ),
                m_pTail( NULL ),
                m_pCurrent( NULL ),
                m_cpTextDoc( pTextDoc )
{
}   /* CTextRunList::CTextRunList */

/**********************************************************************
* CTextRunList::~CTextRunList *
*-----------------------------*
*   Description:  
*       Destructor
*       Deletes everything in the list.
**********************************************************************/
CTextRunList::~CTextRunList()
{
    DeleteAllNodes();
}   /* CTextRunList::~CTextRunList */

/**********************************************************************
* CTextRunList::CreateSimpleList *
*--------------------------------*
*   Description:  
*       Creates a simple list consisting of a single TextRun
*       made up of the entire text of the document.
*   Return:
*       S_OK
*       E_OUTOFMEMORY
*       Return value of ITextDocument::Range()
**********************************************************************/
HRESULT CTextRunList::CreateSimpleList()
{
    _ASSERTE( !m_pHead && m_cpTextDoc );
    if ( m_pHead || !m_cpTextDoc )
    {
        return E_UNEXPECTED;
    }

    CComPtr<ITextRange> cpDocRange;
    HRESULT hr = m_cpTextDoc->Range( 0, 0, &cpDocRange );
    if ( SUCCEEDED( hr ) )
    {
        // Keep moving the end of the range until it reaches the end of 
        // the document in order to get a range with the text of 
        // the entire document
        long lDelta;
        do
        {
            cpDocRange->MoveEnd( tomWord, 100, &lDelta );
        }   while ( lDelta );

        // Create a single CTextRun with this range
        CTextRun *pTextRun = new CTextRun();
        if ( !pTextRun )
        {
            return E_OUTOFMEMORY;
        }
        pTextRun->SetTextRange( cpDocRange );
        pTextRun->IncrementCount();
        
        // Create a single node with that CTextRun
        PTEXTRUNNODE pNode = new TEXTRUNNODE;
        if ( !pNode )
        {
            return E_OUTOFMEMORY;
        }
        pNode->pTextRun = pTextRun;
        pNode->pNext = pNode->pPrev = NULL;

        // That node is the only node in the list
        m_pHead = m_pTail = m_pCurrent = pNode;
    }
    return hr;
}   /* CTextRunList::CreateSimpleList */

/**********************************************************************
* CTextRunList::Insert *
*----------------------*
*   Description:
*       Places pTextRun in the list wherever it should go.
*       Tries to use pCurrent as a hint.
*       Inserting degenerate runs is OK.
*   Return:
*       S_OK
*       E_POINTER 
*       E_OUTOFMEMORY
*       Return value of CTextRunList::AddHead()
*       Return value of CTextRunList::MoveCurrentTo()
*       Return value of CTextRunList::InsertAfter()
*       Return value of CTextRunList::MergeIn()
***********************************************************************/
HRESULT CTextRunList::Insert( CTextRun *pTextRun )
{
    if ( !pTextRun )
    {
        return E_POINTER;
    }

    // Increment the refcount on the text run
    pTextRun->IncrementCount();

    // Create a new node with this CTextRun
    PTEXTRUNNODE pNode = new TEXTRUNNODE;
    if ( !pNode )
    {
        return E_OUTOFMEMORY;
    }
    pNode->pTextRun = pTextRun;

    // This is the node that is going to be added
    m_pNodeToInsert = pNode;

    // Insert the node into the list
    HRESULT hr;
    if (( !m_pHead ) || ( pNode->pTextRun->GetStart() == 0 ))
    {
        // There's nothing in the list already, or this run starts at zero,
        // so add it as the head
        hr = AddHead( pNode );
    }
    else
    {
        // Move the m_pCurrent pointer to the node that will directly precede this run.
        // Nodes will be split here if necessary
        hr = MoveCurrentTo( pNode->pTextRun->GetStart() );

        // Insert the node after where m_pCurrent now is
        if ( SUCCEEDED( hr ) )
        {
            hr = InsertAfter( m_pCurrent, pNode );
        }
    }
    if ( FAILED( hr ) )
    {
        return hr;
    }
 
    // If we replaced or deleted some entire blocks, 
    // then we need to delete those nodes here
    PTEXTRUNNODE p = pNode->pNext;
    PTEXTRUNNODE pNextNode;
    PTEXTRUNNODE pPrevNode;
    while ( p )
    {
        pNextNode = p->pNext;
        if ( p->pTextRun->IsDegenerate() ||
            ( p->pTextRun->GetEnd() <= pNode->pTextRun->GetEnd() ))
        {
            // This node has a degenerate run or a run that ends earlier than ours: 
            // remove it and keep going
            RemoveNode( p );
            p = pNextNode;
        }
        else
        {
            // Stop at the first non-deletable run
            p = NULL;
        }
    }

    // ... And look backwards for the same thing
    p = pNode->pPrev;
    while ( p )
    {
        pPrevNode = p->pPrev;
        if (p->pTextRun->IsDegenerate() ||
            ( p->pTextRun->GetStart() >= pNode->pTextRun->GetStart() ))
        {
            // This node has a degenerate run or a run that starts later than ours: 
            // remove it and keep going
            RemoveNode( p );
            p = pPrevNode;
        }
        else
        {
            // Stop at the first non-deletable run
            p = NULL;
        }
    }

    // Adjust the text ranges of the previous and next nodes 
    // (This is necessary because the text is actually added before this node
    // makes it in, so the TOM has already adjusted the previous and next nodes' ranges to
    // cover this new text)
    if ( pNode->pNext )
    {
        pNode->pNext->pTextRun->SetStart( pNode->pTextRun->GetEnd() );
    }
    if ( pNode->pPrev )
    {
        pNode->pPrev->pTextRun->SetEnd( pNode->pTextRun->GetStart() );
    }

    // Now merge this new run in with the neighbors, if possible.  
    // If pNode is degenerate (or becomes degenerate as a result of merging,
    // it will get deleted.
    bool fMadeItIn = false;
    hr = MergeIn( pNode, &fMadeItIn );
    if ( SUCCEEDED( hr ) ) 
    {
        return fMadeItIn ? S_OK : S_FALSE;
    }
    else
    {
        return hr;
    }
}   /* CTextRunList::Insert */

/******************************************************************************
* CTextRunList::Speak *
*---------------------*
*   Description:
*       Starts speaking at the position *plStartSpeaking and ends at 
*       *plEndSpeaking, unless *plEndSpeaking is -1, in which case it 
*       speaks until the end of the document has been reached.
*       Adjusts *plStartSpeaking and *plEndSpeaking to reflect the 
*       start and endpoints at which we actually will be speaking
*       since they will have to be expanded if they fall in the middle
*       of words or phrase elements.
*
*   Return:
*       S_OK
*       S_FALSE if there was nothing to speak
*       E_POINTER 
*       E_INVALIDARG if the speaking limits are not within range
*       Return value of CTextRun::Speak()
******************************************************************************/
HRESULT CTextRunList::Speak( ISpVoice &rVoice, 
                            long *plStartSpeaking, 
                            long *plEndSpeaking )
{
    _ASSERTE( GetTailEnd() != 0 );

    // Check arguments
    if ( !plStartSpeaking || !plEndSpeaking )
    {
        return E_POINTER;
    }
    if ((( *plEndSpeaking >= 0 ) && ( *plEndSpeaking < *plStartSpeaking )) || 
        (*plStartSpeaking < 0) )
    {
        return E_INVALIDARG;
    }

    if ( *plEndSpeaking == *plStartSpeaking )
    {
        // nothing to speak
        return S_FALSE;
    }

    // Get p to the right spot for starting
    PTEXTRUNNODE p;
    p = Find( *plStartSpeaking );
    
    // If the beginning was not found (i.e. it is at the end), then speak the entire TextRunList.
    if ( !p )
    {
        *plStartSpeaking = 0;
        *plEndSpeaking = GetTailEnd();
        return Speak( rVoice, plStartSpeaking, plEndSpeaking );
    }
    
    if ( p->pTextRun->WithinRange( *plEndSpeaking ) )
    {
        // This block is the only one that needs speaking, since both the start and the end
        // limits for speaking are found within this run.
        // CTextRun::Speak() will put the appropriate values in *plStartSpeaking and
        // *plEndSpeaking.
        return p->pTextRun->Speak( rVoice, plStartSpeaking, plEndSpeaking );
    }

    // Speak from *plStartSpeaking to the end of the first block
    long lFirstBlockEnd = -1;
    HRESULT hr = p->pTextRun->Speak( rVoice, plStartSpeaking, &lFirstBlockEnd );

    // Spin through the list until the final node is reached or until the end of the 
    // TextRunList has been reached
    for ( p = p->pNext; 
            SUCCEEDED(hr) && p && 
                (( *plEndSpeaking < 0 ) || ( p->pTextRun->GetEnd() < *plEndSpeaking ));
            p = p->pNext )
    {
        hr = p->pTextRun->Speak( rVoice );
    }

    // Stop here if something has gone wrong
    if (FAILED(hr))
    {
        return hr;
    }

    // If we were not supposed to speak to the end of the document,
    // speak from the start of the final node until the end limit.
    if ( p && (*plEndSpeaking >= 0) )
    {
        if ( p->pTextRun->GetStart() < *plEndSpeaking )
        {
            // There is something left to speak in the final block,
            // so speak from the beginning to *plEndSpeaking
            long lFinalBlockStart = -1;
            hr = p->pTextRun->Speak( rVoice, &lFinalBlockStart, plEndSpeaking );
        }
        else
        {
            // There is nothing to speak in the final block:
            // The end of the speak is the start of the final block
            *plEndSpeaking = p->pTextRun->GetStart();
        }
    }
    else
    {
        // The end of the speak is the end of the document
        *plEndSpeaking = GetTailEnd();
    }

    return hr;
}   /* CTextRunList::Speak */

/*****************************************************************************
* CTextRunList::Serialize *
*-------------------------*
*   Description:
*       Serializes the information in the TextRunList and writes 
*       it to pStream.
*   Return:
*       S_OK
*       E_POINTER: pStream or pRecoCtxt is NULL
*       Return value of CTextRun::Serialize()
*       Return value of IStream::Write()
******************************************************************************/
HRESULT CTextRunList::Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt )
{
    if ( !pStream || !pRecoCtxt )
    {
        return E_POINTER;
    }

    // Walk the list, serializing each CTextRun
    PTEXTRUNNODE pNode;
    HRESULT hr = S_OK;
    for ( pNode = m_pHead; pNode; pNode = pNode->pNext )
    {
        // Serialize the node
        hr = pNode->pTextRun->Serialize( pStream, pRecoCtxt );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    // Write an "end-of-list" header to the stream
    RUNHEADER endHdr;
    ULONG cb;
    endHdr.lStart = endHdr.lEnd = -1;
    hr = pStream->Write( &endHdr, sizeof( RUNHEADER ), &cb );

    return hr;
}   /* CTextRunList::Serialize */

/*****************************************************************************
* CTextRunList::Deserialize *
*---------------------------*
*   Description:
*       Uses an IStream to recreate a previously-serialized TextRunList
*   Return:
*       S_OK
*       E_POINTER: pStream or pRecoCtxt is NULL
*       E_FAIL: TextRunList was not empty to begin with or the associated
*               ITextDocument is invalid or the TextRunList contains 
*               invalid offsets
*       E_OUTOFMEMORY
*       Return value of IStream::Read()
*       Return value of ISpRecoContext::DeserializeResult()
*       Return value of ITextDocument::Range()
*       Return value of CTextRun::SetTextRange()
******************************************************************************/
HRESULT CTextRunList::Deserialize( IStream *pStream, ISpRecoContext *pRecoCtxt )
{
    if ( !pStream || !pRecoCtxt )
    {
        return E_POINTER;
    }
    if ( !m_cpTextDoc )
    {
        return E_FAIL;
    }

    // Since we're going to be creating this CTextRunList from a stream,
    // clear out any nodes that are already there
    if ( m_pHead )
    {
        DeleteAllNodes();
    }

    // Read in the first header; the end will be indicated by a header
    // with lStart < 0
    RUNHEADER runHdr;
    ULONG cbRead = 0;
    HRESULT hr = pStream->Read( &runHdr, sizeof( RUNHEADER ), &cbRead );
  
    // Keep reading until the end-of-stream RUNHEADER is encountered
    // (that RUNHEADER will have -1 as its lStart)
    long lNextPosToRead = 0;      // To check consistency of serialized list
    while ( SUCCEEDED( hr ) && ( cbRead > 0 ) && ( runHdr.lStart >= 0 ) )
    {
        // Consistency check: Make sure this run starts where the 
        // last run left off
        if ( lNextPosToRead == runHdr.lStart )
        {
            // Good, update next position
            lNextPosToRead = runHdr.lEnd;
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        CTextRun *pNewRun = NULL;

        if ( runHdr.bResultFollows )
        {
            // This run is a serialized CDictationRun

            // Serialized CDictationRuns are preceded by a DICTHEADER
            DICTHEADER dictHdr;
            if ( SUCCEEDED( hr ) )
            {
                hr = pStream->Read( &dictHdr, sizeof( DICTHEADER ), &cbRead );
            }
            /*
            if ( FAILED( hr ) )
            {
                return hr;
            }
            */

            // Allocate the appropriate amount of space for the serialized
            // result object
            void *pv = NULL;
            if ( SUCCEEDED( hr ) )
            {
                pv = ::CoTaskMemAlloc( dictHdr.cbSize );
                if ( !pv )
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Read in the serialized result object
            if ( SUCCEEDED( hr ) )
            {
                hr = pStream->Read( pv, dictHdr.cbSize, &cbRead );
            }
            SPSERIALIZEDRESULT *pResultBlob = static_cast<SPSERIALIZEDRESULT *> (pv);
            CComPtr<ISpRecoResult> cpResult;
            if ( SUCCEEDED( hr ) )
            {
                hr = pRecoCtxt->DeserializeResult( pResultBlob, &cpResult );
            }

            if ( pv )
            {
                ::CoTaskMemFree( pv );
            }

            /*
            if ( FAILED( hr ) )
            {
                return hr;
            }
*/
            // Create a CDictationRun with which to associate the phrase blob
            if ( SUCCEEDED( hr ) )
            {
                pNewRun = new CDictationRun;
                if ( !pNewRun )
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Initialize it with the deserialized result object
            if ( SUCCEEDED( hr ) )
            {
                hr = ((CDictationRun *) pNewRun)->Initialize( 
                    *cpResult, &dictHdr );

                if ( FAILED( hr ) )            
                {
                    // Punt and make it a TextRun
                    delete pNewRun;
                    pNewRun = new CTextRun();

                    if ( pNewRun )
                    {
                        // Proceed with a simple CTextRun
                        hr = S_OK;
                    }
                }
            }

        }
        else
        {
            // The RUNHEADER has indicated that this run is not a CDictationRun,
            // so create a CTextRun
            pNewRun = new CTextRun();
        }

        if ( !pNewRun )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pNewRun->IncrementCount();
        }

        // Create a text range for this run as specified by the RUNHEADER
        CComPtr<ITextRange> cpRange;
        if ( SUCCEEDED( hr ) )
        {
            hr = m_cpTextDoc->Range( runHdr.lStart, runHdr.lEnd, &cpRange );
        }

        // Set the run to use that text range
        if (SUCCEEDED( hr ) )
        {
            hr = pNewRun->SetTextRange( cpRange );
        }

        // Put the node on the list and continue
        if ( pNewRun )
        {
            // Link in the new node at the tail
            PTEXTRUNNODE pNewNode = new TEXTRUNNODE;
            pNewNode->pTextRun = pNewRun;
            AddTail( pNewNode );
        }
        
        if ( SUCCEEDED( hr ) )
        {
            // Read in the next header
            hr = pStream->Read( &runHdr, sizeof( RUNHEADER ), &cbRead );
        }
    }

    if ( FAILED( hr ) )
    {
        // Roll back: Delete all of the nodes that were created
        DeleteAllNodes();
    }

    // Start out with the "hint" pointer pointing to the head
    m_pCurrent = m_pHead;
    return hr;
}   /* CTextRunList::Deserialize */

/*****************************************************************************
* CTextRunList::IsConsumeLeadingSpaces *
*--------------------------------------*
*   Description:
*       Sets *pfConsumeLeadingSpaces to true iff whatever starts at lPos
*       has a display attribute to consume leading spaces
*   Return:
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
*       Return value of CTextRun::IsConsumeLeadingSpaces()
******************************************************************************/
HRESULT CTextRunList::IsConsumeLeadingSpaces( long lPos, 
                                            bool *pfConsumeLeadingSpaces )
{
    if ( !pfConsumeLeadingSpaces )
    {
        return E_POINTER;
    }
    
    // Find the node containing this position
    PTEXTRUNNODE pNode = Find( lPos );
    if ( !pNode )
    {
        // we were given an out-of-range position
        return E_INVALIDARG;
    }

    _ASSERTE( pNode->pTextRun );

    return pNode->pTextRun->IsConsumeLeadingSpaces( lPos, 
        pfConsumeLeadingSpaces );
}   /* CTextRunList::ConsumeLeadingSpaces */

/*****************************************************************************
* CTextRunList::HowManySpacesAfter *
*----------------------------------*
*   Description:
*       Returns the number of spaces that would need to precede text
*       if text were to be inserted at position lPos.
*       This number is in the out param puiSpaces
*   Return:
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
*       Return value of CTextRun::HowManySpacesAfter()
******************************************************************************/
HRESULT CTextRunList::HowManySpacesAfter( long lPos, UINT *puiSpaces )
{
    if ( !puiSpaces )
    {
        return E_POINTER;
    }
    
    // Find the node containing this position
    PTEXTRUNNODE pNode = Find( lPos );
    if ( !pNode )
    {
        // we were given an out-of-range position
        return E_INVALIDARG;
    }

    _ASSERTE( pNode->pTextRun );
    
    return pNode->pTextRun->HowManySpacesAfter( lPos, puiSpaces );
}   /* CTextRunList::HowManySpacesAfter */

/*****************************************************************************
* CTextRunList::GetTailEnd *
*--------------------------*
*   Description:
*       Returns the end position of the tail.  This gives us the length of
*       the document
******************************************************************************/
long CTextRunList::GetTailEnd()
{
    if ( m_pTail )
    {
        return m_pTail->pTextRun->GetEnd();
    }
    else
    {
        return 0;
    }
}   /* CTextRunList::GetTailEnd */

/*****************************************************************************
* CTextRunList::Find *
*--------------------*
*   Description: Returns a pointer to the node containing position lDest.
*   Return:
*       NULL if lDest is out of bounds
*       Pointer to a node with cpMin <= lDest < cpMax
*       OR a pointer to the tail if lDest is the last position in the list
******************************************************************************/
PTEXTRUNNODE CTextRunList::Find( long lDest )
{
    if ( !m_pHead || ( lDest < 0 ) || ( lDest > GetTailEnd() ))
    {
        return NULL;
    }

    // Try to use m_pCurrent as a hint
    if ( m_pCurrent && 
        ( m_pCurrent->pTextRun->GetStart() <= lDest ) && 
        ( lDest < m_pCurrent->pTextRun->GetEnd()) )
    {
        return m_pCurrent;
    }

    // Check for the start
    if ( lDest == 0 )
    {
        return m_pHead;
    }

    // Check for the end
    if ( lDest == GetTailEnd() )
    {
        return m_pTail;
    }
    
    // Find whose start is closest to lDest: the head, the tail, or m_pCurrent.
    // Note that the distance from the start (head) is just lDest, since the 
    // start position is always 0.
    long lDistFromEnd = labs( lDest - m_pTail->pTextRun->GetStart() );
    long lDistFromCurrent;
    if ( m_pCurrent )
    {
        lDistFromCurrent = labs( lDest - m_pCurrent->pTextRun->GetStart() );
    }
    else
    {
        // m_pCurrent isn't pointing anywhere.
        // "Sabotage" lDistFromCurrent so it will never beat searching from the start
        lDistFromCurrent = lDest + 1;
    }
    bool bSearchForward;
    PTEXTRUNNODE pStartSearch;
    if (( lDistFromCurrent < lDest ) && 
        ( lDistFromCurrent < lDistFromEnd ))
    {
        // m_pCurrent is closer than both the head and the tail
        // Search from m_pCurrent
        pStartSearch = m_pCurrent;
    }
    else
    {
        if ( lDest < lDistFromEnd )
        {
            // Head is closer than tail
            pStartSearch = m_pHead;
        }
        else
        {
            pStartSearch = m_pTail;
        }
    }

    bSearchForward = (lDest >= pStartSearch->pTextRun->GetStart());

    // Walk either forwards or backwards from the closest reference node
    // looking for a node that meets cpMin <= lDest < cpMax
    PTEXTRUNNODE p;
    if ( bSearchForward )
    {
        // Forward search
        for ( p = pStartSearch;
            p && (p->pTextRun->GetEnd() <= lDest);
            p = p->pNext )
            ;

        _ASSERTE( p );
        if ( p->pTextRun->GetStart() > lDest )
        {
            // lDest is in a gap preceding node p
            _ASSERTE( false );
            p->pTextRun->SetStart( lDest );
        }
    }
    else
    {
        // Backwards search
        for ( p = pStartSearch;
            p && (p->pTextRun->GetStart() > lDest);
            p = p->pPrev )
            ;

        _ASSERTE( p );
        if ( p->pTextRun->GetEnd() <= lDest )
        {
            // lDest is in a gap following node p
            _ASSERTE( false );
            p->pTextRun->SetEnd( lDest );
        }
    }

    return p;
}   /* CTextRunList::Find */

/*****************************************************************************
* CTextRunList::MoveCurrentTo *
*-----------------------------*
*   Description:
*       Moves the m_pCurrent to the TextRun node directly preceding 
*       the specified location (so that the next time we try to
*       insert a text run, hopefully we can insert right after
*       m_pCurrent.
*       If lDest falls in the middle of a text run, splits that text
*       run.  
*       First tries using its old value as a hint.
*       Postcondition: The end of m_pCurrent == lDest
*   Return:
*       S_OK
*       E_INVALIDARG if lDest is out of the range of the document
******************************************************************************/
HRESULT CTextRunList::MoveCurrentTo( LONG lDest )
{
    if ( m_pCurrent && (lDest == m_pCurrent->pTextRun->GetEnd()) )
    {
        // All set, nothing more to do; m_pCurrent was already right
        return S_OK;
    }

    // If lDest is 0, then set m_pCurrent to NULL (since there is no node preceding the 
    // head
    if ( !lDest )
    {
        m_pCurrent = NULL;
        return S_OK;
    }
    
    // Find() will return pNode such that cpMin <= lDest < cpMax
    PTEXTRUNNODE pNode = Find( lDest );

    if ( !pNode )
    {
        // lDest was too big, so it wasn't found
        return E_INVALIDARG;
    }

    // Make sure lDest is somewhere in pNode's range
    _ASSERTE( pNode->pTextRun->WithinRange( lDest ) || (lDest == pNode->pTextRun->GetEnd()) );

    HRESULT hr = S_OK;
    if ( pNode->pTextRun->GetStart() == lDest )
    {
        // The node we found begins exactly at lDest, so no splitting is necessary;
        // we just need to have m_pCurrent point to the previous node
        m_pCurrent = pNode->pPrev;
    }
    else
    {
        // lDest occurs in the middle of pNode's range, so we need to split pNode
        hr = SplitNode( pNode );
        
        if ( SUCCEEDED( hr ) )
        {
            // pNode still starts at the same place, except now it ends at the start
            // of m_pNodeToInsert (see CTextRunList::Insert())
            m_pCurrent = pNode;
        }
    }
    return hr;
}   /* CTextRunList::MoveCurrentTo */

/******************************************************************************
* CTextRunList::SplitNode *
*-------------------------*
*   Description:
*       Called BEFORE m_pNodeToInsert is inserted into the list, in 
*       order to split up pNode to accommodate the new node.
*       
*       Splits the given node to form a node that ends at the start position 
*       of m_pNodeToInsert and a node that starts at the end position of
*       m_pNodeToInsert.
*
*       Creates a new node to bridge the gap if necessary.
*       See CTextRun::Split() and CDictationRun::Split()
*   Return:
*       S_OK 
*       E_POINTER if pNode is NULL
*       E_INVALIDARG if lCursorPos does not fall in the range of pNode
*       E_OUTOFMEMORY
*       Return value of CTextRun::Split()
*       Return value of CTextRunList::InsertAfter()
******************************************************************************/
HRESULT CTextRunList::SplitNode( PTEXTRUNNODE pNode )
{
    if ( !pNode )
    {
        return E_POINTER;
    }
    _ASSERTE( m_pNodeToInsert );
    if ( !m_pNodeToInsert )
    {
        return E_UNEXPECTED;
    }

    if ( m_pNodeToInsert->pTextRun->GetStart() == pNode->pTextRun->GetEnd() )
    {
        // No need to split, since the new node is at the end of this node
        return S_OK;
    }

    if ( !(pNode->pTextRun->WithinRange( m_pNodeToInsert->pTextRun->GetStart() )) )
    {
        // Start of m_pNodeToInsert is out of bounds
        return E_INVALIDARG;
    }

    CTextRun *pLatterRun = NULL;
    
    // Remember the start and end of m_pNodeToInsert
    const long lStart = m_pNodeToInsert->pTextRun->GetStart();
    const long lEnd = m_pNodeToInsert->pTextRun->GetEnd();
    
    // Call CTextRun::Split().  This might change the split locations lNewStart and lNewEnd 
    // and will set pLatterRun to a non-NULL value if the split actually did occur.
    long lNewStart = lStart;
    long lNewEnd = lEnd;
    HRESULT hr = pNode->pTextRun->Split( &lNewStart, 
        &lNewEnd, m_cpTextDoc, &pLatterRun );
    _ASSERTE( SUCCEEDED( hr ) );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    // Adjust the range on m_pNodeToInsert if it has changed
    if (( lStart != lNewStart ) || ( lEnd != lNewEnd ))
    {
        m_pNodeToInsert->pTextRun->SetStart( lNewStart );
        m_pNodeToInsert->pTextRun->SetEnd( lNewEnd );
    }
    
    if ( pLatterRun )
    {
        // A split did occur
        // Create the new node and insert it right after pNode
        PTEXTRUNNODE pNewNode = new TEXTRUNNODE;
        if ( !pNewNode )
        {
            return E_OUTOFMEMORY;
        }
        pNewNode->pTextRun = pLatterRun;
        pNewNode->pTextRun->IncrementCount();
        pNewNode->pNext = pNewNode->pPrev = NULL;
        
        hr = InsertAfter( pNode, pNewNode );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        // InsertAfter() would have moved m_pCurrent to the latter run (the 
        // one associated with pNewNode).  But m_pCurrent should stay at pNode.  
        m_pCurrent = pNode;
   }

    return S_OK;
}   /* CTextRunList::SplitNode */

/******************************************************************************
* CTextRunList::MergeIn *
*-----------------------*
*   Description:
*       For new DictationRuns, passes them along to the separate merging
*       method for DictationRuns below.
*       handled by a separate merging method below).
*
*       First tries to merge with each neighbor if it is a non-dictated
*       node.
*       
*       Next tries to merge with dictation nodes on either side by 
*       temporarily including the node in the dictation node's range
*       and seeing if it is a match for any of the missing phrase elements
*       in that run.  The neighboring dictation runs are allowed to
*       appropriate whatever portion of its range that they can.      
*
*   Return:
*       S_OK
*       E_POINTER
*       Return value of CTextRunList::MergeInDictRun()
******************************************************************************/
HRESULT CTextRunList::MergeIn( PTEXTRUNNODE pNode, bool *pfNodeMadeItIn )
{
    if ( !pfNodeMadeItIn )
    {
        return E_POINTER;
    }

    *pfNodeMadeItIn = false;

    if ( pNode->pTextRun->IsDict() )
    {
        // Dictation runs are handled by a separate merging method.
        // Assuming CTextRunList::MergeInDictRun() went off successfully,
        // the run will have made it onto the list
        HRESULT hr = MergeInDictRun( pNode );
        if ( SUCCEEDED( hr ) )
        {
            *pfNodeMadeItIn = true;
        }
        return hr;
    }
    
    // This run is not a CDictationRun

    // Do the easy merge with the neighbors if they are text nodes
    // (that is, two adjacent non-dictated runs can always be merged)
    MERGERESULT mr;
    HRESULT hr;
    if ( pNode->pPrev && !(pNode->pPrev->pTextRun->IsDict()) )
    {
        mr = pNode->pTextRun->Concatenate( pNode->pPrev->pTextRun, false );
        
        // Both nodes are nondictated text, so this better have worked
        _ASSERTE( mr == E_FULLMERGE );
        _ASSERTE( pNode->pPrev->pTextRun->IsDegenerate() );
        if (( mr != E_FULLMERGE ) || !pNode->pPrev->pTextRun->IsDegenerate() )
        {
            return E_UNEXPECTED;
        }

        // Prev node can now be removed
        hr = RemoveNode( pNode->pPrev );
        if ( FAILED( hr ) )
        {
            _ASSERTE( false );
            return E_UNEXPECTED;
        }
    }
    if ( pNode->pNext && !(pNode->pNext->pTextRun->IsDict()) )
    {
        mr = pNode->pTextRun->Concatenate( pNode->pNext->pTextRun, true );

        // Both nodes are nondictated text, so this better have worked
        _ASSERTE( mr == E_FULLMERGE );
        _ASSERTE( pNode->pNext->pTextRun->IsDegenerate() );
        if (( mr != E_FULLMERGE ) || !pNode->pNext->pTextRun->IsDegenerate() )
        {
            return E_UNEXPECTED;
        }

        // Next node can now be removed
        hr = RemoveNode( pNode->pNext );
        if ( FAILED( hr ) )
        {
            _ASSERTE( false );
            return E_UNEXPECTED;
        }
    }

    // pNode should be surrounded by dictation nodes now, since there should never be 
    // two consecutive text nodes

    // The concatenation for pNext must happen before that for pPrev.
    // That is because it is a lot easier to include trailing spaces before the
    // next word in a range than it is to include preceding spaces.  
    // Thus by concatenating onto pPrev LAST we are doing the forward Concatenate second, 
    // which is more effective in gobbling up pNode's range (our goal is to get
    // pNode's range as small as possible.
    if ( pNode->pNext )
    {
        _ASSERTE( pNode->pNext->pTextRun->IsDict() );
        _ASSERTE( pNode->pNext->pTextRun->GetStart() == pNode->pTextRun->GetEnd() );

        // This concatenation may change the end of pNode's text range or 
        // the start of pNext's text range
        pNode->pNext->pTextRun->Concatenate( pNode->pTextRun, false );
    }
    if ( pNode->pPrev )
    {
        _ASSERTE( pNode->pPrev->pTextRun->IsDict() );
        _ASSERTE( pNode->pPrev->pTextRun->GetEnd() == pNode->pTextRun->GetStart() );

        // This concatenation may change the end of pPrev's text range or 
        // the start of pNode's text range
        pNode->pPrev->pTextRun->Concatenate( pNode->pTextRun, true );
    }

    // If pNode now has nothing left, then remove it and try to concatenate its previous and 
    // next neighbors together
    if ( pNode->pTextRun->IsDegenerate() )
    {
        // Hang on to the prev and next nodes
        PTEXTRUNNODE pPrev = pNode->pPrev;
        PTEXTRUNNODE pNext = pNode->pNext;

        // Remove pNode
        HRESULT hr = RemoveNode( pNode );
        _ASSERTE( SUCCEEDED(hr) );
        if ( FAILED( hr ) )
        {
            return E_UNEXPECTED;
        }

        // Concatenate pPrev and pNext.  If the pNext node is completely consumed, then 
        // remove it as well
        if ( pPrev && pNext )
        {
            _ASSERTE( pPrev->pTextRun->GetEnd() == pNext->pTextRun->GetStart() );
            
            pPrev->pTextRun->Concatenate( pNext->pTextRun, true );
            
            if ( pNext->pTextRun->IsDegenerate() )
            {
                // pNext was entirely subsumed by pPrev, so remove it
                hr = RemoveNode( pNext );
                _ASSERTE( SUCCEEDED(hr) );
                if ( FAILED( hr ) )
                {
                    return E_UNEXPECTED;
                }
            }
        }

        // Do not set *pfNodeMadeItIn to true, since the node did not make it in
        return S_OK;
    }
    else
    {
        // If we are here, the node made it onto the list successfully
        *pfNodeMadeItIn = true;
        return S_OK;
    }
}   /* CTextRunList::MergeIn */

/******************************************************************************
* CTextRunList::MergeInDictRun *
*------------------------------*
*   Description:
*       Merges in a dictation run.  
*       Since it is a new dictation run, it is complete and its range
*       will not change.  If either neighbor is a dictation run,
*       we need to check if the phrase elements are correct
*       (since, of course, the new dictation run may be splitting up
*       already-existing dictation runs).  
*       If not, we may need to insert text runs on either side.
*   Return:
*       S_OK
*       E_OUTOFMEMORY
*       Return value of CTextRun::CorrectPhraseEltsAndRange()
*       Return value of ITextDocument::Range()
*       Return value of CTextRun::SetTextRange()
*       Return value of CTextRunList::InsertAfter()
*       Return value of CTextRunList::MergeIn()
*******************************************************************************/
HRESULT CTextRunList::MergeInDictRun( PTEXTRUNNODE pNode )
{
    HRESULT hr;
    PTEXTRUNNODE pNewTextRunNode = NULL;
    CTextRun *pNewTextRun = NULL;
    
    // Make sure that the previous node still has accurate information
    if ( pNode->pPrev )
    {
        PTEXTRUNNODE pPrev = pNode->pPrev;

        // Ensure that the previous node's phrase elements reflect reality (it
        // may be a newly-broken dictation run.)
        // The range of pPrev will be adjusted so that only full phrase elements
        // are included in pPrev's range
        hr = pPrev->pTextRun->CorrectPhraseEltsAndRange( true );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        if ( pPrev->pTextRun->GetEnd() < pNode->pTextRun->GetStart() )
        {
            // There is a gap between the neighboring run and this run.
            // The gap contains a fragment of a phrase element, which will
            // now be downgraded to the status of non-dictated text.

            // A new text run must be created to cover that area
            pNewTextRun = new CTextRun();
            if ( !pNewTextRun )
            {
                return E_OUTOFMEMORY;
            }
            pNewTextRun->IncrementCount();

            // That new run will cover the gap 
            CComPtr<ITextRange> pPrevTextRange;
            hr = m_cpTextDoc->Range( pPrev->pTextRun->GetEnd(), 
                pNode->pTextRun->GetStart(), &pPrevTextRange );
            if ( SUCCEEDED( hr ) )
            {
                hr = pNewTextRun->SetTextRange( pPrevTextRange );
            }
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            // Make a new TEXTRUNNODE for the list
            pNewTextRunNode = new TEXTRUNNODE;
            if ( !pNewTextRunNode )
            {
                return E_OUTOFMEMORY;
            }
            pNewTextRunNode->pNext = pNewTextRunNode->pPrev = NULL;
            pNewTextRunNode->pTextRun = pNewTextRun;
            
            // Put it right where the gap is
            hr = InsertAfter( pPrev, pNewTextRunNode );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        if ( pPrev->pTextRun->IsDegenerate() )
        {
            // pPrev now has no complete phrase elements,
            // so get rid of it
            hr = RemoveNode( pPrev );
            _ASSERTE( SUCCEEDED(hr) );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        if ( pNewTextRunNode )
        {
            // We are adding a CTextRun to bridge the gap between pPrev and pNode

            // Merge in the fragment of the phrase element as text
            bool fMadeItIn;
            hr = MergeIn( pNewTextRunNode, &fMadeItIn );
            if ( FAILED( hr ) )
            {
                return hr;
            }

            // Reset pNewTextRunNode NULL so this will work for processing pNext
            pNewTextRunNode = NULL;
        }
    }

    // Make sure that the next node still has accurate information
    if ( pNode->pNext )
    {
        PTEXTRUNNODE pNext = pNode->pNext;

        // Ensure that the next node's phrase elements reflect reality (it
        // may be a newly-broken dictation run.
        // The range of pPrev will be adjusted so that only full phrase elements
        // are included in pNext's range
        hr = pNext->pTextRun->CorrectPhraseEltsAndRange( false );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        if ( pNext->pTextRun->GetStart() > pNode->pTextRun->GetEnd() )
        {
            // There is a gap between the neighboring run and this run.
            // The gap contains a fragment of a phrase element, which will
            // now be downgraded to the status of non-dictated text.

            // A new text run must be created to cover that area
            pNewTextRun = new CTextRun();
            if ( !pNewTextRun )
            {
                return E_OUTOFMEMORY;
            }
            pNewTextRun->IncrementCount();

            // That new run will cover the gap 
            CComPtr<ITextRange> pNextTextRange;
            hr = m_cpTextDoc->Range( pNode->pTextRun->GetEnd(), 
                pNext->pTextRun->GetStart(), &pNextTextRange );
            if ( SUCCEEDED( hr ) )
            {    
                hr = pNewTextRun->SetTextRange( pNextTextRange );
            }
            if ( FAILED( hr ) )
            {
                return hr;
            }

            // Make a new TEXTRUNNODE for the list
            pNewTextRunNode = new TEXTRUNNODE;
            if ( !pNewTextRunNode )
            {
                return E_OUTOFMEMORY;
            }
            pNewTextRunNode->pNext = pNewTextRunNode->pPrev = NULL;
            pNewTextRunNode->pTextRun = pNewTextRun;

            // Put it right where the gap is
            hr = InsertAfter( pNode, pNewTextRunNode );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        if ( pNext->pTextRun->IsDegenerate() )
        {
            // pNext now has no complete phrase elements,
            // so get rid of it
            hr = RemoveNode( pNext );
            _ASSERTE( SUCCEEDED(hr) );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        if ( pNewTextRunNode )
        {
            // We are adding a CTextRun to bridge the gap between pNode and pNext

            // Merge in the fragment of the phrase element as text
            bool fMadeItIn;
            hr = MergeIn( pNewTextRunNode, &fMadeItIn );
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }

    // If we got here, then no errors were encountered
    return S_OK;
}   /*CTextRunList::MergeInDictRun */

/**********************************************************************
* CTextRunList::InsertAfter *
*---------------------------*
*	Description:  
*		Inserts pNodeToInsert in the list after pCurrent.
*       If pCurrent is NULL, adds it onto the head.
*
* 	Return:
*		E_POINTER if pNodeToInsert is NULL, otherwise S_OK.
**********************************************************************/
HRESULT CTextRunList::InsertAfter( PTEXTRUNNODE pCurrent,          
                                   PTEXTRUNNODE pNodeToInsert )    
{
    HRESULT hr;
    if( !pNodeToInsert )
    {
        hr = E_POINTER;
    }
    else if ( !pCurrent )
    {
        hr = AddHead( pNodeToInsert );
    }
    else
    {
        // check if pCurrent is the tail of the list
        if( pCurrent == m_pTail )
        {
            hr = AddTail( pNodeToInsert );
        }
        else
        {
            if( pCurrent->pNext == NULL )
            {
                hr = E_UNEXPECTED;  // only the tail can have a NULL pNext
            }
            else
            {
                // Link in the new node
                PTEXTRUNNODE pLast = pCurrent->pNext;
                pCurrent->pNext = pNodeToInsert;
                pLast->pPrev = pNodeToInsert;
                pNodeToInsert->pPrev = pCurrent;
                pNodeToInsert->pNext = pLast;

                // Update m_pCurrent
                m_pCurrent = pNodeToInsert;

                hr = S_OK;
            }
        }
    }
    return hr;
} /* CTextRunList::InsertAfter */

/**********************************************************************
* CTextRunList::RemoveNode *
*--------------------------*
*	Description:  
*		Removes a node from the list.
*       Decrements the text run reference counts.
*
* 	Return:
*		E_POINTER if pNode is NULL, otherwise S_OK.
**********************************************************************/
HRESULT CTextRunList::RemoveNode( PTEXTRUNNODE pNode ) 
{
    HRESULT hr;
    if( !pNode )
    {
        hr = E_POINTER;
    }
    else
    {
        if( pNode == m_pHead )
        {
            RemoveHead();
        }
        else if( pNode == m_pTail )
        {
            RemoveTail();
        }
        else
        {
            // Link the two neighbors together
            PTEXTRUNNODE pLeft = pNode->pPrev;
            PTEXTRUNNODE pRight = pNode->pNext;
            pLeft->pNext = pRight;
            pRight->pPrev = pLeft;
            
            // Update current node if we're removing it
            if( pNode == m_pCurrent )
            {
                m_pCurrent = pLeft;
            }
            
            // Decrement the refcount of this pNode's TextRun, then delete it
            pNode->pTextRun->DecrementCount();
            delete pNode;
            
        }
        hr = S_OK;
    }
    return hr;
}   /* CTextRunList::RemoveNode */

/**********************************************************************
* CTextRunList::AddHead *
*-----------------------*
*	Description:  
*		Adds a node to the head of the list.  Sets the current
*       pointer to the head.
*
* 	Return:
*		E_POINTER if pHead is NULL, otherwise S_OK.
**********************************************************************/
HRESULT CTextRunList::AddHead( PTEXTRUNNODE pHead )    
{
    HRESULT hr;
    if( !pHead )
    {
        hr = E_POINTER;
    }
    else
    {
        // Check for the first item in the list
        if( !m_pHead )
        {
            // This will be the only item on the list
            _ASSERTE( m_pTail == NULL );
            pHead->pNext = pHead->pPrev = NULL;
            m_pHead = m_pTail = m_pCurrent = pHead;
        }
        else
        {
            // Put this node ahead of the existing head
            m_pHead->pPrev = pHead;
            pHead->pNext = m_pHead;
            pHead->pPrev = NULL;
            m_pHead = pHead;
            m_pCurrent = pHead;
        }
        hr = S_OK;
    }
    
    return hr;
}   /* CTextRunList::AddHead */

/**********************************************************************
* CTextRunList::AddTail *
*-----------------------*
*	Description:  
*		Adds a node to the tail of the list.  Sets the current
*       pointer to the tail.
*
* 	Return:
*		E_POINTER if pTail is NULL, otherwise S_OK.
**********************************************************************/
HRESULT CTextRunList::AddTail( PTEXTRUNNODE pTail )   
{
    HRESULT hr;
    if( !pTail )
    {
        hr = E_POINTER;
    }
    else
    {
        // Check for the last item in the list
        if( !m_pTail )
        {
            // This will be the only item on the list
            _ASSERTE( m_pHead == NULL );
            pTail->pNext = pTail->pPrev = NULL;
            m_pHead = m_pTail = pTail;
        }
        else
        {
            // Put this node after of the existing tail
            m_pTail->pNext = pTail;
            pTail->pPrev = m_pTail;
            pTail->pNext = NULL;
            m_pTail = pTail;
            m_pCurrent = pTail;
        }
        hr = S_OK;
    }
    
    return hr;
}   /* CTextRunList::AddTail */

/**********************************************************************
* CTextRunList::RemoveHead *
*--------------------------*
*	Description:  
*		Removes the head node from the list and decrements the
*       text run's reference count.
**********************************************************************/
void CTextRunList::RemoveHead()
{
    _ASSERTE( m_pHead );
    if( m_pHead )
    {
        if( m_pCurrent == m_pHead )
        {
            // m_pCurrent would usually fall back one, but here
            // we are removing the head
            m_pCurrent = NULL;
        }
        m_pHead->pTextRun->DecrementCount();

        // Update m_pHead
        PTEXTRUNNODE pNewHead = m_pHead->pNext;
        delete m_pHead;
        m_pHead = pNewHead;

        // If we have deleted the only element, then make the tail NULL too
        if ( !m_pHead )
        {
            m_pTail = NULL;
        }
        else
        {
            // Head should have no previous node
            m_pHead->pPrev = NULL;
        }
    }
}   /* CTextRunList::RemoveHead */

/**********************************************************************
* CTextRunList::RemoveTail *
*--------------------------*
*	Description:  
*		Removes the tail node from the list and decrements the
*       text run's reference count.
**********************************************************************/
void CTextRunList::RemoveTail()
{
    if( m_pTail )
    {
        if( m_pCurrent == m_pTail )
        {
            m_pCurrent = m_pTail->pPrev;
        }
        m_pTail->pTextRun->DecrementCount();
        
        // Update m_pTail
        PTEXTRUNNODE pNewTail = m_pTail->pPrev;
        delete m_pTail;
        m_pTail = pNewTail;
       
        // If we have deleted the last element, then make the head NULL too
        if ( !m_pTail )
        {
            m_pHead = NULL;
        }
        else
        {
            // Tail should have no next node
            m_pTail->pNext = NULL;
        }
    }
}   /* CTextRunList::RemoveTail */

/*****************************************************************************
* CTextRunList::DeleteAllNodes *
*------------------------------*
*   Description:
*       Deletes a doubly linked list for text run objects
*       and decrements the text run reference counts.
******************************************************************************/
void CTextRunList::DeleteAllNodes()
{
    // Spin through our list, deleting as we go
    while( m_pHead )
    {
        m_pCurrent = m_pHead;

        m_pHead = m_pHead->pNext;

        // Decrement a count on the TextRun associated with m_pCurrent
        // This should delete the TextRun when its refcount hits 0.
        m_pCurrent->pTextRun->DecrementCount();

        delete m_pCurrent;
    }

    // The list is now empty
    m_pHead = m_pTail = m_pCurrent = NULL;
}   /* CTextRunList::DeleteAllNodes */


