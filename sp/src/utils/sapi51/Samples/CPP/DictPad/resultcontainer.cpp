/******************************************************************************
*   resultcontainer.cpp 
*       This module contains the definition of CResultContainer.  
*       CResultContainer makes all of the recognition-object-
*       specific SAPI5 calls
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "resultcontainer.h"

/**********************************************************************
* CResultContainer::CResultContainer *
*------------------------------------*
*	Description:  
*       Constructor for the CResultContainer class.
*       A CResultContainer is created by the first CDictationRun
*       to be created for a given result object.
**********************************************************************/
CResultContainer::CResultContainer( ISpRecoResult &rResult,
                                   CDictationRun &rFirstOwner,
                                   CPhraseReplacement &rPhraseReplacement ) :
                            m_cpRecoResult( &rResult ),
                            m_pPhraseReplacement( &rPhraseReplacement ),
                            m_pOwnerListHead( NULL ),
                            m_cLastRequestedAltsReturned( 0 ),
                            m_ulStartOfLastRequestedAlts( 0 ),
                            m_cElementsInLastRequestedAlts( 0 )
{
    // The owner should go onto the list
    m_pOwnerListHead = new OWNERNODE;
    if ( m_pOwnerListHead )
    {
        m_pOwnerListHead->pOwner = &rFirstOwner;
        m_pOwnerListHead->pNext = NULL;
    }
    // If the allocation failed, all calls that need it 
    // will return E_OUTOFMEMORY

    // Zero out the requested phrase alternates array
    memset( (void *) m_apLastRequestedPhraseAlts, 0, 
        ALT_REQUEST_COUNT * sizeof( ISpPhraseAlt *) );
}   /* CResultContainer::CResultContainer */

/**********************************************************************
* CResultContainer::~CResultContainer *
*-------------------------------------*
*	Description:  
*       Destructor for the CResultContainer class.
*       This object gets destroyed when there are no longer
*       any CDictationRuns referencing it.
**********************************************************************/
CResultContainer::~CResultContainer()
{
    _ASSERTE( !m_pOwnerListHead );

    if (  m_pPhraseReplacement )
    {
        delete m_pPhraseReplacement;
    }

    // Release any ISpPhraseAlt's that are still around from the 
    // last call to ISpPhrase::GetAlternates()
    for (int i = 0; i < ALT_REQUEST_COUNT; i++)
    {
        if (m_apLastRequestedPhraseAlts[i])
        {
            m_apLastRequestedPhraseAlts[i]->Release();
        }        
    }
}   /* CResultContainer::~CResultContainer */

/**********************************************************************
* CResultContainer::AddOwner *
*----------------------------*
*	Description:  
*       Called whenever a new CDictationRun is created to use this
*       same RecoResult.  Adds that CDictationRun to the front of 
*       the list of owners.
*   Return:
*       S_OK
*       E_OUTOFMEMORY
**********************************************************************/
HRESULT CResultContainer::AddOwner( CDictationRun &rNewOwner )
{
    if ( !m_pOwnerListHead )
    {
        _ASSERTE( false );
        return E_UNEXPECTED;
    }

    OWNERNODE *pNode = new OWNERNODE;
    if ( pNode )
    {
        pNode->pNext = m_pOwnerListHead;
        pNode->pOwner = &rNewOwner;
        m_pOwnerListHead = pNode;
        
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}   /* CResultContainer::AddOwner */

/**********************************************************************
* CResultContainer::DeleteOwner *
*-------------------------------*
*	Description:  
*       Called whenever a DictationRun using this RecoResult
*       has been deleted.  Removes the CDictationRun from the 
*       list and deletes itself if that was the last owner
**********************************************************************/
void CResultContainer::DeleteOwner( CDictationRun &rOldOwner )
{
    _ASSERTE( m_pOwnerListHead );
    if ( !m_pOwnerListHead )
    {
        return;
    }

    OWNERNODE **ppNode;
    for ( ppNode = &m_pOwnerListHead; 
        (*ppNode) && ((*ppNode)->pOwner != &rOldOwner); 
        ppNode = &((*ppNode)->pNext) )
        ;

    if ( *ppNode )
    {
        // Found: Delete it
        OWNERNODE *pNodeToDelete = *ppNode;
        *ppNode = (*ppNode)->pNext;
        delete pNodeToDelete;
    }
    else
    {
        // Should be on the list!
        _ASSERTE( false );
    }

    if ( !m_pOwnerListHead )
    {
        // There are no more CDictationRuns referencing this
        delete this;
    }
}   /* CResultContainer::DeleteOwner */

/**********************************************************************
* CResultContainer::SpeakAudio *
*------------------------------*
*	Description:  
*       Calls ISpRecoResult::SpeakAudio() on the recoresult.
*       Converts the arguments to pre-replacement elements
*   Return:
*       S_OK
*       S_FALSE if no elements are to be spoken.
*       Return value of CPhraseReplacement conversion routines
*       Return value of ISpRecoResult::SpeakAudio()
**********************************************************************/
HRESULT CResultContainer::SpeakAudio( ULONG ulStartElement,
                                     ULONG cElements )
{
    if (!cElements)
    {
        return S_FALSE;
    }

    // Convert the start and end to phrase element indices 
    // as seen from the result object's point of view
    ULONG ulPreReplStart;
    ULONG ulPreReplEnd;
    ULONG cPreReplElts;
    HRESULT hr = m_pPhraseReplacement->PostToPreReplacementIndex(
        ulStartElement, &ulPreReplStart );
    if (SUCCEEDED(hr))
    {
        hr = m_pPhraseReplacement->PostToPreReplacementIndex(
            ulStartElement + cElements, &ulPreReplEnd );
    }

    // Call ISpRecoResult::SpeakAudio()
    if (SUCCEEDED(hr))
    {
        cPreReplElts = ulPreReplEnd - ulPreReplStart;
        hr = m_cpRecoResult->SpeakAudio( 
            ulPreReplStart, cPreReplElts, SPF_ASYNC, NULL );
    }

    return hr;
}   /* CResultContainer::SpeakAudio */

/**********************************************************************
* CResultContainer::GetAlternatesText *
*-------------------------------------*
*	Description:  
*       Called to get the text of the alternates for 
*       a specific range of elements in this result.
*       ppszCoMemText is expected to point to a buffer
*       of size at least sizeof( WCHAR * ) * ulRequestCount,
*       and the text in it will be CoTaskMemAlloced.
*   Return:
*       S_OK
*       S_FALSE if there are no alternates
*       E_POINTER
*       return value of CResultContainer::GetAlternates()
*       return value of CResultContainer::GetAltText()
**********************************************************************/
HRESULT CResultContainer::GetAlternatesText( ULONG ulStartElement,
                                            ULONG cElements, 
                                            ULONG ulRequestCount,
                                            WCHAR **ppszCoMemText,
                                            ULONG *pcPhrasesReturned )
{
    SPDBG_FUNC("CResultContainer::GetAlternatesText");
    if ( !ppszCoMemText || !pcPhrasesReturned )
    {
        return E_POINTER;
    }

    // Get as many alternates as we can 
    ULONG cAvailableAlts;
    HRESULT hr = GetAlternates( ulStartElement, cElements, ulRequestCount,
                                &cAvailableAlts );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Sanity check: We didn't get more than we asked for
    _ASSERTE( cAvailableAlts <= ulRequestCount );
    
    if ( !cAvailableAlts )
    {
        // No alternates
        *pcPhrasesReturned = 0;
        *ppszCoMemText = NULL;
        return S_FALSE;
    }

    // Null out the pointers to text buffers first
    memset(ppszCoMemText, 0, ulRequestCount * sizeof(WCHAR *));

    // Get the text
    for ( ULONG ulAlt = 0; 
        SUCCEEDED(hr) && (ulAlt < cAvailableAlts); 
        ulAlt++ )
    {
        // ppszCoMemText[ulAlt] is the ulAlt'th (WCHAR *)
        // in the array.  
        // Its address will give us a WCHAR ** which will
        // point to a buffer that is to be CoTaskMemAlloced
        // by GetAltText()
        hr = GetAltText( ulStartElement, cElements, ulAlt, 
            &(ppszCoMemText[ulAlt]), NULL );
    }
    *pcPhrasesReturned = cAvailableAlts;

    return hr;
}   /* CResultContainer::GetAlternatesText */

/**********************************************************************
* CResultContainer::GetAlternates *
*---------------------------------*
*	Description:  
*       Gets the ISpPhraseAlts for this range of elements 
*       (indices are given taking replacement into account).
*       The phrase alternates are pointed to by 
*       m_apLastRequestedPhraseAlts.
*       It is very likely that GetAlternates() would be called
*       numerous times consecutively for the same set of 
*       alternates.  Thus, these are cached between calls.
*   Return:
*       S_OK
*       E_INVALIDARG: Out-of-bounds elements
*       Return value of CPhraseReplacement conversion routines
*       Return value of ISpRecoResult::GetAlternates
**********************************************************************/
HRESULT CResultContainer::GetAlternates( ULONG ulStartElement,
                                        ULONG cElements,
                                        ULONG ulRequestCount, 
                                        ULONG *pcPhrasesReturned )
{
    // First check to see if we have these alternates cached.
    // We check that there is at least one alternate cached,
    // that the alternates are for the same set of phrase elements,
    // and that there are enough.
    if ( m_apLastRequestedPhraseAlts[0] && 
        ( m_ulStartOfLastRequestedAlts == ulStartElement ) &&
        ( m_cElementsInLastRequestedAlts == cElements ) &&
        ( m_cLastRequestedAltsReturned >= ulRequestCount) )
    {
        if ( pcPhrasesReturned )
        {
            // We already have these alternates, so don't bother
            // getting them again
            *pcPhrasesReturned = __min( 
                ulRequestCount, m_cLastRequestedAltsReturned );
        }
        return S_OK;
    }

    if ( (ulStartElement + cElements) > 
        m_pPhraseReplacement->GetNumReplacementElements() )
    {
        // Out of bounds
        return E_INVALIDARG;
    }
    
    // Convert the start and end to phrase element indices 
    // as seen from the result object's point of view
    ULONG ulPreReplStart;
    ULONG ulPreReplLast;
    HRESULT hr = m_pPhraseReplacement->PostToPreReplacementIndex( 
        ulStartElement, &ulPreReplStart );
    if ( SUCCEEDED(hr) )
    {
        hr = m_pPhraseReplacement->PostToPreReplacementIndex(
            ulStartElement + cElements, &ulPreReplLast );
    }
    ULONG cPreReplElements = ulPreReplLast - ulPreReplStart;

    // Release any ISpPhraseAlts that are around from last time
    for (int i = 0; i < ALT_REQUEST_COUNT; i++)
    {
        if ( m_apLastRequestedPhraseAlts[i] )
        {
            m_apLastRequestedPhraseAlts[i]->Release();
            m_apLastRequestedPhraseAlts[i] = 0;
        }
    }

    // Get as many alternates as we can by calling 
    // ISpRecoResult::GetAlternates()
    ULONG cPhrasesReturned;
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoResult->GetAlternates( ulPreReplStart,
            cPreReplElements, ulRequestCount, m_apLastRequestedPhraseAlts,
            &cPhrasesReturned );
    }
    if ( pcPhrasesReturned )
    {
        *pcPhrasesReturned = cPhrasesReturned;
    }

    // Cache information about this alternate request
    m_ulStartOfLastRequestedAlts = ulStartElement;
    m_cElementsInLastRequestedAlts = cElements;
    m_cLastRequestedAltsReturned = cPhrasesReturned;

    return hr;
}   /* CResultContainer::GetAlternates */

/**********************************************************************
* CResultContainer::GetAltInfo *
*------------------------------*
*	Description:  
*       Given the alternates index, gets the info for the 
*       alternate (i.e. which elements it replaces in the 
*       parent and how many elements it has).
*       Returns element indices TAKING REPLACEMENTS INTO
*       ACCOUNT, unless the fReturnPostReplIndices flag has
*       been set to false (it is set to true by default).
*   
*       If the end of the alternate falls in the middle of
*       a replacement, then the result indices are expanded
*       to include the entire replacement.
*   Return:
*       S_OK
*       E_FAIL if we couldn't get ulAlternateIndex alternates
*       Return value of CResultContainer::GetAlternates()
*       Return value of ISpPhraseAlt::GetAltInfo()
*       Return value of 
*           CResultContainer::ExpandToIncludeWholeReplacements()
*       Return value of CPhraseReplacement::Initialize()
*       Return value of CPhraseReplacement conversion routines
**********************************************************************/
HRESULT CResultContainer::GetAltInfo( ULONG ulStartElement,
                                     ULONG cElements,
                                     ULONG ulAlternateIndex, 
                                     ULONG *pulStartInParent,
                                     ULONG *pcEltsInParent,
                                     ULONG *pcEltsInAlt,
                                     bool fReturnPostReplIndices)
{
    // First make sure we actually have this alternate
    ULONG cPhrasesReturned;
    HRESULT hr = GetAlternates( 
        ulStartElement, cElements, ulAlternateIndex + 1, &cPhrasesReturned );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    if ( cPhrasesReturned < (ulAlternateIndex + 1) )
    {
        // There aren't enough alternates to accommodate
        // this one
        return E_FAIL;
    }

    // Sanity check: The pointer to the ISpPhraseAlt for this
    // alternate is non-NULL
    _ASSERTE( m_apLastRequestedPhraseAlts[ ulAlternateIndex ] );

    // Call to ISpPhraseAlt::GetAltInfo()
    ULONG ulStartInParent;
    ULONG cEltsInParent;
    ULONG cEltsInAlt;
    hr = (m_apLastRequestedPhraseAlts[ ulAlternateIndex ])->GetAltInfo(
        NULL, &ulStartInParent, &cEltsInParent, &cEltsInAlt );

#ifdef _DEBUG
    // Debug code for seeing the text of the entire alternate
    // (which covers the entire parent phrase) and for seeing
    // the text for just the part of the alternate that 
    // covers the elements in the parent that we are interested in
    WCHAR * pwszWhole = 0;
    WCHAR * pwszAlt = 0;
    BYTE b;
    m_apLastRequestedPhraseAlts[ ulAlternateIndex ]->GetText( 
        SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, true, &pwszWhole, &b );
    m_apLastRequestedPhraseAlts[ ulAlternateIndex ]->GetText( 
        ulStartInParent, cEltsInAlt, true, &pwszAlt, &b );
    ::CoTaskMemFree( pwszWhole );
    ::CoTaskMemFree( pwszAlt );
#endif

    // If there is replaced (ITNed) text anywhere in this set of
    // elements in the parent, we should be getting alternate
    // text for the entire replacement.
    // That is, if a replacement covers the word "thirty" in the 
    // phrase "I have thirty nine dollars", we should also be
    // showing alternate text for the elements "nine dollars",
    // since it appears to all be one element ("$39.00") to the
    // user
    if ( SUCCEEDED( hr ) )
    {
        const ULONG cEltsInParentBeforeExpansion = cEltsInParent;
        
        // Expand the endpoints to include entire replacements
        hr = m_pPhraseReplacement->ExpandToIncludeWholeReplacements( 
            &ulStartInParent, &cEltsInParent );
        _ASSERTE( SUCCEEDED( hr ) );
        
        // Adjust the number of elements in the alternate accordingly
        // (all of the extra elements we have just included are the 
        // same in the alternate as they are in the parent, so it 
        // will be the same number of extra elements in the alternate
        cEltsInAlt += cEltsInParent - cEltsInParentBeforeExpansion;
    }

    if ( !fReturnPostReplIndices )
    {
        // We're done; no need to convert, since the caller wants
        // output in phrase element indices from the result object's
        // point of view
        if ( pulStartInParent )
        {
            *pulStartInParent = ulStartInParent;
        }
        if ( pcEltsInParent )
        {
            *pcEltsInParent = cEltsInParent;
        }
        if ( pcEltsInAlt )
        {
            *pcEltsInAlt = cEltsInAlt;
        }
        return hr;
    }

    // Convert these element indices to post-replacement indices
    if ( SUCCEEDED( hr ) )
    {    
        // First convert the start element    
        ULONG ulPostReplStartInParent;
        hr = m_pPhraseReplacement->PreToPostReplacementIndex(
            ulStartInParent, &ulPostReplStartInParent );
        if (FAILED(hr))
        {
            return hr;
        }
        if ( pulStartInParent )
        {
            *pulStartInParent = ulPostReplStartInParent;
        }

        // Convert the end element to determine how many post-replacement
        // elements  this alternate replaces
        if ( pcEltsInParent )
        {
            ULONG ulEndInParent = ulStartInParent + cEltsInParent;
            ULONG ulEndPostReplElement;
            hr = m_pPhraseReplacement->PreToPostReplacementIndex(
                ulEndInParent, &ulEndPostReplElement );
            if (FAILED(hr))
            {
                return hr;
            }
            *pcEltsInParent = ulEndPostReplElement - ulPostReplStartInParent;
        }
    }

    // Find the number of elements in the alternate, if that was requested
    if ( SUCCEEDED(hr) && pcEltsInAlt )
    {
        // Since this ISpPhraseAlt is a completely different ISpPhrase
        // from the one associated with m_cpRecoResult, we have to 
        // construct a new CPhraseReplacement object to deal specifically
        // with this phrase.
        CPhraseReplacement newPhraseRepl;
        hr = newPhraseRepl.Initialize(*(m_apLastRequestedPhraseAlts[ ulAlternateIndex ]));
        
        // First convert the start element    
        ULONG ulPostReplStartInAlt;
        if ( SUCCEEDED( hr ) )
        {
            hr = newPhraseRepl.PreToPostReplacementIndex(
                ulStartInParent, &ulPostReplStartInAlt );
        }

        // Convert the end element to determine how many post-replacement
        // elements  this alternate replaces
        ULONG ulEndInAlt = ulStartInParent + cEltsInAlt;
        ULONG ulEndPostReplElement;
        if ( SUCCEEDED(hr) )
        {
            hr = newPhraseRepl.PreToPostReplacementIndex(
                ulEndInAlt, &ulEndPostReplElement );
        }
        *pcEltsInAlt = ulEndPostReplElement - ulPostReplStartInAlt;
    }
    return hr;
}   /* CResultContainer::GetAltInfo */

/**********************************************************************
* CResultContainer::GetAltText *
*------------------------------*
*	Description:  
*       Given the alternates index, gets the text for the 
*       alternate and display attributes.
*   Return:
*       S_OK
*       Return value of CResultContainer::GetAlternates()
*       Return value of CResultContainer::GetAltInfo()
*       Return value of ISpPhraseAlt::GetText()
**********************************************************************/
HRESULT CResultContainer::GetAltText( ULONG ulStartElement,
                                     ULONG cElements,
                                     ULONG ulAlternateIndex, 
                                     WCHAR **ppszCoMemText,
                                     BYTE *pbDisplayAttributes )
{
    // First make sure we actually have this alternate
    ULONG cPhrasesReturned;
    HRESULT hr = GetAlternates( 
        ulStartElement, cElements, ulAlternateIndex + 1, &cPhrasesReturned );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    if ( cPhrasesReturned < (ulAlternateIndex + 1) )
    {
        // There aren't enough alternates to accommodate
        // this one
        return E_FAIL;
    }

    // Sanity check: The pointer to the ISpPhraseAlt for this
    // alternate is non-NULL
    _ASSERTE( m_apLastRequestedPhraseAlts[ ulAlternateIndex ] );

    // Find out what elements in the parent this alternate replaces
    // Note that we do NOT want text replacements taken into effect,
    // since we are calling ISpPhrase::GetText() directly with
    // these phrase element indices, so we need those indices
    // from the result object's point of view
    ULONG ulStartInParent;
    ULONG cEltsInParent;
    ULONG cEltsInAlt;
    hr = GetAltInfo( ulStartElement, cElements, ulAlternateIndex, 
        &ulStartInParent, &cEltsInParent, &cEltsInAlt, false );
    if ( FAILED(hr) )
    {
        return hr;
    }
 
    // Call to ISpPhraseAlt::GetText()
    return m_apLastRequestedPhraseAlts[ ulAlternateIndex ]->GetText(
        ulStartInParent, cEltsInAlt, true, ppszCoMemText, pbDisplayAttributes );
}   /* CResultContainer::GetAltText */


/**********************************************************************
* CResultContainer::ChooseAlternate *
*-----------------------------------*
*	Description:  
*       Called when the caller wishes to commit one of the alternates.
*       If pbDisplayAttributes is non-NULL, hands back the display
*       attributes of the alternate text going in.
*       Adjusts the phrase element mapping in CPhraseReplacement
*       to the new result object.
*   Return:
*       S_OK
*       Return value of CResultContainer::GetAlternates()
*       Return value of CResultContainer::GetAltText()
*       Return value of CResultContainer::NotifyOwnersOfCommit()
*       Return value of ISpPhraseAlt::Commit()
*       Return value of CPhraseReplacement::Initialize()
**********************************************************************/
HRESULT CResultContainer::ChooseAlternate( ULONG ulStartElement,
                                          ULONG cElements, 
                                          ULONG ulAlternateIndex,
                                          BYTE *pbDisplayAttributes )
{
    // First make sure we actually have this alternate
    ULONG cPhrasesReturned;
    HRESULT hr = GetAlternates( 
        ulStartElement, cElements, ulAlternateIndex + 1, &cPhrasesReturned );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    if ( cPhrasesReturned < (ulAlternateIndex + 1) )
    {
        // There aren't enough alternates to accommodate
        // this one
        return E_FAIL;
    }

    // Sanity check: The pointer to the ISpPhraseAlt for this
    // alternate is non-NULL
    _ASSERTE( m_apLastRequestedPhraseAlts[ ulAlternateIndex ] );

    // If there is interest in the display attributes for the alternate, 
    // get them
    if ( pbDisplayAttributes )
    {
        WCHAR *pszCoMemText;
        hr = GetAltText( ulStartElement, cElements, ulAlternateIndex, 
            &pszCoMemText, pbDisplayAttributes );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        ::CoTaskMemFree( pszCoMemText );
    }

    // Notify owners of the alternate committal so that they can adjust
    // their element offset maps
    hr = NotifyOwnersOfCommit( ulStartElement, cElements, ulAlternateIndex );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Call to ISpPhraseAlt::Commit()
    hr = m_apLastRequestedPhraseAlts[ulAlternateIndex]->Commit();
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // The alternate is now "dead"

    // Re-initialize the phrase replacement info, as the element indices
    // and text replacment (ITN) situations are likely to have changed
    return m_pPhraseReplacement->Initialize( *m_cpRecoResult );

}   /* CResultContainer::ChooseAlternate */

/**********************************************************************
* CResultContainer::NotifyOwnersOfCommit *
*----------------------------------------*
*	Description:  
*       Called when an alternate is about to be committed BEFORE 
*       the commit happens.
*       Notifies all DictationRuns that hold onto this result
*       that their phrase element offset maps will need to change
*   Return:
*       S_OK
*       E_OUTOFMEMORY if the head of the owner list wasn't
*           allocated at construction time
*       Return value of CResultContainer::GetAlternates()
*       Return value of CPhraseReplacement::Initialize()
*       Return value of CResultContainer::GetAltInfo()
**********************************************************************/
HRESULT CResultContainer::NotifyOwnersOfCommit( ULONG ulStartElement,
                                               ULONG cElements, 
                                               ULONG ulAlternateIndex )
                                               //ISpPhraseAlt *pChosenAlternate )
{
    if ( !m_pOwnerListHead )
    {
        return E_OUTOFMEMORY;
    }

    // First make sure we actually have this alternate
    ULONG cPhrasesReturned;
    HRESULT hr = GetAlternates( 
        ulStartElement, cElements, ulAlternateIndex + 1, &cPhrasesReturned );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    if ( cPhrasesReturned < (ulAlternateIndex + 1) )
    {
        // There aren't enough alternates to accommodate
        // this one
        return E_FAIL;
    }

    // Sanity check: The pointer to the ISpPhraseAlt for this
    // alternate is non-NULL
    _ASSERTE( m_apLastRequestedPhraseAlts[ ulAlternateIndex ] );

    // We need to determine how many post-replacement elements the new
    // phrase will have, but the alternate has not yet been committed to
    // m_cpRecoResult, so we need to construct a different CPhraseReplacement
    // to get that information
    CPhraseReplacement newPhraseRepl;
    hr = newPhraseRepl.Initialize( *(m_apLastRequestedPhraseAlts[ ulAlternateIndex ]) );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    ULONG cPostReplElementsAfterCommit = newPhraseRepl.GetNumReplacementElements();

    // Find out which (post-replacement) elements in the parent 
    // this alternate will replace
    ULONG ulStartInParent;
    ULONG cEltsInParent;
    hr = GetAltInfo( ulStartElement, cElements, ulAlternateIndex, 
        &ulStartInParent, &cEltsInParent );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    // Loop through the owner list, and let each one adjust its maps
    for ( OWNERNODE *pNode = m_pOwnerListHead; pNode; pNode = pNode->pNext )
    {
        hr = pNode->pOwner->OnAlternateCommit( ulStartInParent, 
            cEltsInParent, cPostReplElementsAfterCommit );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return S_OK;
}   /* CResultContainer::NotifyOwnersOfCommit */

