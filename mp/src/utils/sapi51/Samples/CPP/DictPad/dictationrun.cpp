/******************************************************************************
*   DictationRun.cpp 
*       This module contains the implementation details of the CDictationRun
*       class which handles the dictation-specfic issues for a dictation
*       text run.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include "DictationRun.h"

/******************************************************************************
* CDictationRun::CDictationRun *
*------------------------------*
*   Description:
*       Constructor.
*
*   Return:
*       <none>
******************************************************************************/
CDictationRun::CDictationRun() : m_ulStartElement( 0 ),
                                    m_cElements( 0 ),
                                    m_fAltsGotten( false ),
                                    m_pPhraseReplacement( NULL ),
                                    m_pulElementOffsets( NULL ),
                                    m_pResultContainer( NULL )
{}  /* CDictationRun::CDictationRun */

/******************************************************************************
* CDictationRun::~CDictationRun *
*-------------------------------*
*   Description:
*       Destructor.
*
*   Return:
*       <none>
******************************************************************************/
CDictationRun::~CDictationRun( void )
{
    // See CDictationRun::SetTextRange(), where this is allocated
    delete[] m_pulElementOffsets;

    // Tell the result object (which might be shared with other DictationRuns)
    // that we no longer need it.
    if ( m_pResultContainer )
    {
        m_pResultContainer->DeleteOwner( *this );
    }

    // The CPhraseReplacement object is going to need to stay around 
    // for as long as the CResultContainer does; thus when all 
    // CDictationRuns depending on this result object are deleted,
    // the associated CPhraseReplacement object will also be deleted.

}   /* CDictationRun::~CDictationRun */

/******************************************************************************
* CDictationRun::Initialize *
*---------------------------*
*   Description:
*       Constructor.
*       This is the initialization routine that is called when a DictationRun
*       is being created from scratch, either from a serialized DictationRun 
*       (if the optional param pDictHdr is NULL) or from newly-dictated text.
*       Initializes the phrase element information and sets up the 
*       phrase data.
*
*   Return:
*       S_OK
*       E_OUTOFMEMORY
*       Return value of CPhraseReplacement::Initialize()
******************************************************************************/
HRESULT CDictationRun::Initialize( ISpRecoResult &rRecoResult, DICTHEADER *pDictHdr ) 
{
    // Get the phrase element information from the serialized header, if available
    if ( pDictHdr )
    {
        m_ulStartElement = pDictHdr->ulStartElement;
        m_cElements = pDictHdr->cElements;
    }
    
    // If this is a new DictationRun (not recreated from a serialized DictationRun),
    // the element count will get properly initialized
    // when the text range is set.

    // Create and initialize the phrase replacement info
    m_pPhraseReplacement = new CPhraseReplacement();
    if ( !m_pPhraseReplacement )
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = m_pPhraseReplacement->Initialize( rRecoResult );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Create a CResultContainer to hold the result object
    m_pResultContainer = new CResultContainer( rRecoResult, *this, 
        *m_pPhraseReplacement );
    if ( !m_pResultContainer )
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;

}   /* CDictationRun::Initialize */

/******************************************************************************
* CDictationRun::Initialize *
*---------------------------*
*   Description:
*       Constructor.
*       This is the initialization routine that is called when a DictationRun
*       is being created from the contents of another DictationRun 
*       (i.e. a DictationRun is being split).
*       All of the phrase information is already contained in the 
*       CResultContainer.
*
*   Return:
*       S_OK
******************************************************************************/
HRESULT CDictationRun::Initialize( CResultContainer &rResultContainer )
{
    // Get the phrase element and result object information from the
    // CResultContainer
    m_pResultContainer = &rResultContainer;
    m_pPhraseReplacement = rResultContainer.GetPhraseReplacement();

    // The caller needs to call rResultContainer.AddOwner().

    return S_OK;
}   /* CDictationRun::Initialize */

/******************************************************************************
* CDictationRun::SetTextRange *
*-----------------------------*
*   Description:
*       Stores the ITextRange * for this run.
*       Creates the array of element offsets and initializes 
*       the entries.
*       The two cases in which this function can be called are
*       *   New DictationRun, in which presumably all of
*           the elements will be found
*       *   A DictationRun formed by being split off an old
*           DictationRun, in which case not all of the elements
*           will be present
*   Return:
*       S_OK
*       E_POINTER
*       E_OUTOFMEMORY 
*       Return value of ITextRange::FindTextStart()
******************************************************************************/
HRESULT CDictationRun::SetTextRange( ITextRange *pTextRange )
{
    if ( !pTextRange )
    {
        return E_POINTER;
    }
    m_cpTextRange = pTextRange;

    if ( !m_pPhraseReplacement || !m_pResultContainer )
    {
        return E_OUTOFMEMORY;
    }

    // If we do not already know how many phrase elements we have, then 
    // this is a new run, so get it from the phrase replacement object
    if ( !m_cElements )
    {
        m_cElements = m_pPhraseReplacement->GetNumReplacementElements();
    }

    // Allocate enough space in our element offsets to hold information
    // for all the elements.
    // Each entry in this array will indicate where the given element
    // starts (given as an offset from the start of the range)
    m_pulElementOffsets = 
        new ULONG[ m_pPhraseReplacement->GetNumReplacementElements() ];
    if ( !m_pulElementOffsets )
    {
        return E_OUTOFMEMORY;
    }

    // Fill the array with an impossible value, right now the length of
    // this range (since nothing can have an offset tha big)
    const ULONG ulBogusVal = GetEnd() - GetStart();
    ULONG ulElement;
    for ( ulElement = 0; 
        ulElement < m_pPhraseReplacement->GetNumReplacementElements(); 
        ulElement++ )
    {
        m_pulElementOffsets[ ulElement ] = ulBogusVal;
    }

    // Loop through the elements finding the character offset in the range
    // for each one, if it is there.
    // We will use a duplicate ITextRange and ITextRange::FindTextStart()
    BSTR bstrElement;
    long lStart = GetStart();
    long lElementStart;
    CComPtr<ITextRange> cpRangeDup;
    HRESULT hr = pTextRange->GetDuplicate( &cpRangeDup );
    if ( !cpRangeDup )
    {
        return E_OUTOFMEMORY;
    }
    for ( ulElement = 0; 
        SUCCEEDED( hr ) && 
            ( ulElement < m_pPhraseReplacement->GetNumReplacementElements() ); 
        ulElement++ )
    {
        // Get the element text
        BYTE bDisplayAttributes;
        bstrElement = ::SysAllocString( 
            m_pPhraseReplacement->GetDisplayText( ulElement, &bDisplayAttributes ) );
        if ( !bstrElement )
        {
            return E_OUTOFMEMORY;
        }

        // Look for the element.
        long lLength;
        hr = cpRangeDup->FindTextStart( bstrElement,  0, 0, &lLength );

        ::SysFreeString( bstrElement );

        if ( lLength )
        {
            // The element was found in cpRangeDup

            // The start of the cpRangeDup is now the start of the element
            cpRangeDup->GetStart( &lElementStart );

            // Give the offset relative to the start of the range
            m_pulElementOffsets[ulElement] = lElementStart - lStart;

            // Advance the start past this word
            cpRangeDup->MoveStart( tomCharacter, lLength, NULL ); 
        }
    }

    return hr;
}   /* CDictationRun::SetTextRange */

/**********************************************************************
* CDictationRun::Split *
*----------------------*
*	Description:  
*		Splits up a DictationRun so that this text run now ends at 
*       lFirstEnd and a second dictation run (*ppTextRun) begins at 
*       lSecondBegin.
*       "This" will now be a shorter range (it will end sooner),
*       and *ppTextRun will point to the new text run for which 
*       space will be allocated here.
*       In general, the phrase element information for the two
*       DictationRuns will NOT be correct after exiting this
*       function.  The caller must call CorrectPhraseElementsAndRange 
*       to ensure that the phrase element information correctly reflects
*       the new range, which it in general will not after this function.
* 	Return:
*		S_OK 
*       E_POINTER
*       E_INVALIDARG: *plFirstEnd, *plSecondBegin out of bounds or
*           in the wrong order
*       E_OUTOFMEMORY 
*       Return value of CDictationRun::Initialize()
*       Return value of ITextDocument::Range()
*       Return value of CResultContainer::AddOwner()
**********************************************************************/
HRESULT CDictationRun::Split( long *plFirstEnd, 
                            long *plSecondBegin, 
                            ITextDocument *cpTextDoc,
                            CTextRun **ppTextRun )
{
    if ( !plFirstEnd || !plSecondBegin || !cpTextDoc || !ppTextRun )
    {
        return E_POINTER;
    }
    if ( !m_cpTextRange || !m_pResultContainer )
    {
        return E_UNEXPECTED;
    }

    long lFirstEnd = *plFirstEnd;
    long lSecondBegin = *plSecondBegin;

    if ( !WithinRange( lFirstEnd ) || (lFirstEnd > lSecondBegin) )
    {
        // Cannot split somewhere that is not in the range
        return E_INVALIDARG;
    }

    if ( (GetStart() == lSecondBegin) || (GetEnd() == lFirstEnd) || 
        (lSecondBegin > GetEnd()) )
    {
        // Don't need to do anything, since we are trying to split on a 
        // run boundary
        *ppTextRun = NULL;
        return S_OK;
    }

    // Create a new dictation run to hold the text from lSecondBegin until
    // the end of the original range.
    // We create this new DictationRun off the same result object
    // in m_pResultContainer
    *ppTextRun = new CDictationRun(); 
    CDictationRun *pDictRun = (CDictationRun *) *ppTextRun;
    if ( !pDictRun )
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = pDictRun->Initialize( *m_pResultContainer );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // This DictationRun should be on the owner list for this result container
    hr = m_pResultContainer->AddOwner( *pDictRun );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Adjust the latter range so that it starts at lSecondBegin 
    // and ends where "this" used to end
    long lEnd = GetEnd();
    CComPtr<ITextRange> cpLatterRange;
    hr = cpTextDoc->Range( lSecondBegin, lEnd, &cpLatterRange );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    hr = pDictRun->SetTextRange( cpLatterRange );

    // Adjust the range of "this" so that it ends at lFirstEnd
    m_cpTextRange->SetEnd( lFirstEnd );

    // Just copy over the phrase element information, which will 
    // in general be incorrect until we call CorrectPhraseEltsAndRange()
    pDictRun->m_ulStartElement = m_ulStartElement;
    pDictRun->m_cElements = m_cElements;

    return hr;

}   /* CDictationRun::Split */

/**********************************************************************
* CDictationRun::Concatenate *
*----------------------------*
*   Description:
*       If bConcatenateAfter is true, pTextRun's range is appended
*       to "this"'s range; else it is prepended.
*       Concatenation happens according to the following rules:
*       *   If pTextRun is a DictationRun that refers to the same 
*           result object as "this" and its phrase elements adjoin
*           ours, then concatenation is possible.  
*       *   If pTextRun is a plain TextRun, then we try 
*           concatenating the whole thing on and seeing if 
*           some of that text can be appropriated as phrase elements
*   Return:
*       E_NOMERGE if no merging was possible
*       E_PARTIALMERGE if some but not all of pTextRun's range
*           could be appropriated by this
*       E_FULLMERGE if all of pTextRun's range was appropriated
*       E_LESSTEXT if by joining pText onto "this", we reduce
*           the number of phrase elements that "this" has
*           (e.g. pTextRun's text runs onto the end of "this"'s
*           last phrase element, thus making that last word no 
*           longer a true phrase element)
***********************************************************************/
MERGERESULT CDictationRun::Concatenate( CTextRun *pTextRun, 
                                bool fConcatAfter )
{
    // Validate params
    if ( !pTextRun || !m_cpTextRange || !m_pResultContainer )
    {
        _ASSERTE( false );
        return E_NOMERGE;
    }

    // Check that the runs are indeed adjacent in the text
    if (( fConcatAfter && ( GetEnd() != pTextRun->GetStart() ) ) ||
        ( !fConcatAfter && ( GetStart() != pTextRun->GetEnd() ) ))
    {
        // Non-consecutive runs can't be concatenated
        _ASSERTE( false );
        return E_NOMERGE;
    }

    if ( pTextRun->IsDict() )
    {
        // pTextRun is a dictation run.
        // This merge is easy to do: Check that the two DictationRuns
        // share the same result object, then check if e.g. the 
        // first one has elements 2 through 4 and the second one
        // has elements 5 through 8.

        // We know this is a DictationRun so do the cast
        CDictationRun *pDictRun = (CDictationRun *) pTextRun;

        // Check to see if the result objects are the same (if they
        // are then they'll point to the same CResultContainer
        if ( pDictRun->m_pResultContainer != m_pResultContainer )
        {
            // Dictation runs with different result objects 
            // cannot be concatenated
            return E_NOMERGE;
        }
        
        // See if phrase elements are consecutive
        if ( fConcatAfter )
        {
            // pDictRun's elements need to follow "this"'s

            if ( (m_ulStartElement + m_cElements) == 
                                pDictRun->m_ulStartElement )
            {
                // These can be merged, since pDictRun picks 
                // up where we leave off

                // Store the start indices so that we can update 
                // the element offsets
                long lThisStart = GetStart();
                long lDictRunStart = pDictRun->GetStart();

                // Adjust the ranges so that "this" contains 
                // all of pDictRun's text and so that
                // pDictRun will be degenerate
                SetEnd( pDictRun->GetEnd() );
                pDictRun->SetStart( pDictRun->GetEnd() );

                // Update the element offsets for the new elements introduced by
                // pDictRun.  
                // We will need to update every element accounted for
                // by pDictRun to be relative to the start of this run rather
                // than the start of pDictRun
                for ( ULONG ulElement = pDictRun->m_ulStartElement; 
                    ulElement < (pDictRun->m_ulStartElement + pDictRun->m_cElements); 
                    ulElement++ )
                {
                    // Shift the offset of the new element.
                    // Add lDictRunStart to get an absolute offset (within the document),
                    // then subtract lThisStart for an offset relative to "this"
                    m_pulElementOffsets[ulElement] = 
                        pDictRun->m_pulElementOffsets[ulElement] 
                            + lDictRunStart - lThisStart;
                }

                // Adjust the phrase element info so that "this" contains all of
                // pDictRun's phrase elements
                m_cElements += pDictRun->m_cElements;

                // pDictRun should now contain no elements
                pDictRun->m_ulStartElement = m_ulStartElement + m_cElements;
                pDictRun->m_cElements = 0;

                return E_FULLMERGE;
            }
        }
        else
        {
            // Same as above, just in the opposite direction.
            // Here pDictRun is being prepended to this run.

            if ( m_ulStartElement == 
                (pDictRun->m_ulStartElement + pDictRun->m_cElements) )
            {
                // Store the start indices so that we can update 
                // the element offsets
                long lThisStart = GetStart();
                long lDictRunStart = pDictRun->GetStart();

                // Adjust the ranges so that "this" contains 
                // all of pDictRun's text and so that
                // pDictRun will be degenerate
                SetStart( pDictRun->GetStart() );
                pDictRun->SetEnd( pDictRun->GetStart() );

                // Update the element offsets for the new elements introduced by
                // pDictRun.
                // The offsets in pDictRun are correct, since they are relative
                // to the earlier start.
                // The offsets for "this" need to be updated
                ULONG ulElement;
                for ( ulElement = pDictRun->m_ulStartElement;
                    ulElement < 
                        (pDictRun->m_ulStartElement + pDictRun->m_cElements);
                    ulElement++ )
                {
                    // Copy over element offsets from pDictRun
                    m_pulElementOffsets[ulElement] = 
                        pDictRun->m_pulElementOffsets[ulElement];
                }
                
                for ( ulElement = m_ulStartElement;
                    ulElement < (m_ulStartElement + m_cElements);
                    ulElement++ )
                {
                    // Shift the offset of the element.
                    // Add lThisStart to get an absolute offset (within the document),
                    // then subtract lDictRunStart (the new start of the text range)
                    // for an offset relative to the new start
                    m_pulElementOffsets[ulElement] += 
                        lThisStart - lDictRunStart;
                }

                // Adjust the phrase element info so that "this" contains all of
                // pDictRun's phrase elements
                m_cElements += pDictRun->m_cElements;
                m_ulStartElement = pDictRun->m_ulStartElement;
                
                // pDictRun should now contain no elements
                pDictRun->m_cElements = 0;

                return E_FULLMERGE;
            }
        }

        // If we got here, then both runs are CDictationRuns pointing to the same
        // CResultContainer but they did not contain adjacent elements
        return E_NOMERGE;
    }
    

    // Merging a text run onto "this"

    // To do this, we first "guess" that the entire text run should go
    // with "this".  CorrectPhraseEltsAndRange() corrects this assumption
    // if the entire phrase cannot in fact be used.
    
    if ( fConcatAfter )
    {
        // Add on all of the text in pTextRun to the end of "this"

        // For determining how much of a merge will have taken place
        long lOldBorder = pTextRun->GetStart();

        // Temporarily set the range of "this" to the end of pTextRun,
        // thus incorporating all of pTextRun's text into "this"
        SetEnd( pTextRun->GetEnd() );

        // Cut out whatever is not a phrase element.
        // This may push back the end of "this"'s range
        bool fCorrectionResult;
        HRESULT hr = CorrectPhraseEltsAndRange( true, &fCorrectionResult );
        if ( FAILED( hr ) )
        {
            return E_NOMERGE;
        }

        // Now have pTextRun pick up where "this" leaves off,
        // so that it includes everything that could not be
        // merged into "this"
        pTextRun->SetStart( GetEnd() );

        if ( fCorrectionResult )
        {
            // The entire range of pTextRun was appropriated by this
            _ASSERTE( pTextRun->IsDegenerate() );
            return E_FULLMERGE;
        }
        else
        {
            if ( pTextRun->GetStart() == lOldBorder )
            {
                // None of pTextRun's range was appropriated
                return E_NOMERGE;
            }
            else
            {
                // If pTextRun starts earlier than it did before,
                // then concatenating pTextRun onto "this"
                // reduced the number of phrase elements it
                // contained.
                // If pTextRun starts later than it did before,
                // then "this" appropriated  some part of it as 
                // phrase elements.
                return (pTextRun->GetStart() < lOldBorder) ? 
                    E_LESSTEXT : E_PARTIALMERGE;
            }
        }
    }
    else
    {
        // Same as above, only in the reverse direction (try to merge
        // pTextRun onto the beginning of "this"

        // For determining how much of a merge will have taken place
        long lOldBorder = pTextRun->GetEnd();

        // Temporarily set the range of "this" to include pTextRun,
        // thus incorporating all of pTextRun's text into "this"
        SetStart( pTextRun->GetStart() );

        // Cut out whatever is not a phrase element.
        // This may push forward the start of "this"'s range
        bool fCorrectionResult;
        HRESULT hr = CorrectPhraseEltsAndRange( false, &fCorrectionResult );
        if ( FAILED( hr ) )
        {
            return E_NOMERGE;
        }

        // Now have pTextRun end where "this" starts,
        // so that it includes everything that could not be
        // merged into "this"
        pTextRun->SetEnd( GetStart() );

        if ( fCorrectionResult )
        {
            // The entire range of pTextRun was appropriated by this
            _ASSERTE( pTextRun->IsDegenerate() );
            return E_FULLMERGE;
        }
        else
        {
            if ( pTextRun->GetEnd() == lOldBorder )
            {
                // None of pTextRun's range was appropriated
                return E_NOMERGE;
            }
            else
            {
                // If pTextRun ends later than it did before,
                // then concatenating pTextRun onto "this"
                // reduced the number of phrase elements it
                // contained.
                // If pTextRun ends earlier than it did before,
                // then "this" appropriated  some part of it as 
                // phrase elements.
                return (pTextRun->GetEnd() > lOldBorder) ? 
                    E_LESSTEXT : E_PARTIALMERGE;
            }
        }
    }
}   /* CDictationRun::Concatenate */      
        

/**********************************************************************
* CDictationRun::CorrectPhraseEltsAndRange *
*------------------------------------------*
*	Description:  
*		Ensures that the m_ulStartElement and m_cElements correctly
*       reflect the text range.  If not, adjusts the phrase element
*       info and the range so that they agree.
*
*       Depending on whether fForward is true, either goes forwards
*       from m_ulStartElement or backwards from 
*       m_ulStartElement + m_cElements in matching up the phrases.
*       If fForward is true, then includes any between-word space 
*       after the last element it matches, if that space was 
*       included in the original range.
*
*       If fForward is true, the end of "this"'s range may be
*       pushed back if there is some text at the end of the 
*       range that does not match phrase elements.
*       If fForward is false, the start of "this"'s range may be
*       pushed forward.
*  
*       Note that the range will never be expanded here, only 
*       possibly contracted.
*
*       *pfCorrectionResult will be true iff the phrase element info
*       and range did not have to be changed
* 	Return:
*       S_OK
*       E_POINTER
*       Return value of ITextRange::GetDuplicate()
**********************************************************************/
HRESULT CDictationRun::CorrectPhraseEltsAndRange( bool fForward, 
                                                 bool *pfCorrectionResult )
{   
    // Make sure current state is what we expect
    _ASSERTE( m_pPhraseReplacement && m_pResultContainer );
       
    // Store range and phrase element info.
    // We will use these values later on to determine what has changed
    const long lOldStart = GetStart();
    const long lOldEnd = GetEnd();
    const ULONG ulOldStartElement = m_ulStartElement;
    const ULONG cOldElements = m_cElements;

#ifdef _DEBUG
    BSTR bstrRangeText;
    m_cpTextRange->GetText( &bstrRangeText );
#endif

    // Degenerate range case: Number of elements should be 0.
    if ( IsDegenerate() )
    {
        if ( m_cElements )
        {
            // Phrase information is incorrect since it says
            // that we contain some phrase elements
            m_cElements = 0;
            
            if ( pfCorrectionResult )
            {
                *pfCorrectionResult = false;
            }
        }
        else
        {
            if ( pfCorrectionResult )
            {
                *pfCorrectionResult = true;
            }
        }
        return S_OK;
    }
    
    // Nondegenerate case

    // Create a duplicate range.  
    // This range will correspond to text we skip during
    // our search for phrase elements
    CComPtr<ITextRange> pSkippedRange;
    HRESULT hr = m_cpTextRange->GetDuplicate( &pSkippedRange );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Depending on the direction, we will either want to start
    // at the earliest element supposedly contained in this range
    // (if fForward == true) or at the last element.
    const ULONG ulFirstElement = fForward ? m_ulStartElement : 
                                (m_ulStartElement + m_cElements - 1);
    ULONG ulElement = ulFirstElement;
    
    long cch, lOldBound;
    BSTR bstrElement, bstrSkippedText;

    // For each phrase element, find it in the range.
    // We will use the start (resp. end) of "this"'s range
    // in the calls to ITextRange::FindTextStart()
    // which will move the start (or end) of our range just past 
    // each element that is found
    while (ulElement < m_pPhraseReplacement->GetNumReplacementElements())
    {
        // Get the next phrase element that we are expecting
        BYTE bDisplayAttributes = 0;
        bstrElement = ::SysAllocString( 
            m_pPhraseReplacement->GetDisplayText( ulElement, &bDisplayAttributes ) );

        // Remember where "this"'s range started (resp. ended)
        lOldBound = fForward ? GetStart() : GetEnd();

        // Either move the start to right before this word
        // or move the end to right after this word.  
        // Note that we want to match entire words only if the display attributes 
        // don't tell us to consume leading spaces (like a comma or something)
        if ( GetStart() == GetEnd() )
        {
            cch = 0;
        }
        else
        {
            fForward ? 
                m_cpTextRange->FindTextStart( 
                    bstrElement, 
                    0, 
                    (bDisplayAttributes & SPAF_CONSUME_LEADING_SPACES) ? 0 : tomMatchWord, 
                    &cch )
                : m_cpTextRange->FindTextEnd( 
                    bstrElement, 
                    0, 
                    (bDisplayAttributes & SPAF_CONSUME_LEADING_SPACES) ? 0 : tomMatchWord, 
                    &cch );
        }

        
        ::SysFreeString( bstrElement );

#ifdef _DEBUG
        ::SysFreeString( bstrRangeText );
        m_cpTextRange->GetText( &bstrRangeText );
#endif

        // Set pSkippedRange to include whatever text is between
        // the old start (resp end) of m_cpTextRange (which is at the 
        // end (resp start) of the last phrase element we found) 
        // and the new start (resp end) of m_cpTextRange (which is at the start
        // (resp end) of the phrase element we just found, if we did
        // indeed find one)
        pSkippedRange->SetStart( fForward ? lOldBound : GetEnd() );
        pSkippedRange->SetEnd( fForward ? GetStart() : lOldBound );
        
        // If this skipped range contains any text that is not whitespace,
        // then there was something between the last phrase element and
        // this phrase element that we just found.
        pSkippedRange->GetText( &bstrSkippedText );
        if ( ContainsNonWhitespace( bstrSkippedText ) )
        {
            // Contains some non-spaces, so this element should not
            // count as found, since there is some text between 
            // where we are and the phrase element
            cch = 0;

            // Set the start (resp. end) to the old bound, since we
            // don't want to include this text (it does not consist
            // solely of phrase elements
            fForward ? SetStart( lOldBound ) : SetEnd( lOldBound );
        }
        ::SysFreeString( bstrSkippedText );

        if ( !cch )
        {
            // Phrase element not found:
            // Break out of the loop
            break;
        }

        // The phrase element was found; update the element
        // offset array.
        // For the time being, these are absolute indices
        // (since if fForward is FALSE, then we don't yet know
        // what the start of the range is.
        // Later in the function they will be adjusted
        // to be relative indices.
        if ( fForward )
        {
            // Because of ITextRange::FindTextStart(), 
            // the start of this range is the start of this element
            m_pulElementOffsets[ ulElement ] = GetStart();
        }
        else
        {
            // Because of ITextRange::FindTextEnd(), 
            // the end of this range is the end of this element.
            // Subtract cch since cch is the length of the element
            m_pulElementOffsets[ ulElement ] = GetEnd() - cch;
        }

        // Move the start (resp. the end) past the phrase element
        // (whose length is cch)
        fForward ? m_cpTextRange->MoveStart( tomCharacter, cch, NULL ) : 
                    m_cpTextRange->MoveEnd( tomCharacter, -cch, NULL );


#ifdef _DEBUG
        ::SysFreeString( bstrRangeText );
        m_cpTextRange->GetText( &bstrRangeText );
#endif

        // Increment (decrement) ulElement to look for the next element
        fForward ? ulElement++ : ulElement--;
    }   // (while loop)

#ifdef _DEBUG
    ::SysFreeString( bstrRangeText );
#endif

    if ( fForward )
    {
        // If all of the elements were there, ulElement is equal
        // to the old start element plus the old number of elements.
        // Otherwise, ulElement is equal to the first phrase element
        // after ulFirstElement that was not found.

        // Adjust the phrase element information to reflect
        // what we found
        m_cElements = ulElement - ulFirstElement;
        
        // Adjust the end so that only phrase elements are in this range.
        // This is where the start of the range is right now (since we
        // just moved the start past the last phrase element
        // that was found.
        SetEnd( GetStart() );
        
        // Reset the start to its original location (it was moved around
        // by ITextRange::FindTextStart())
        SetStart( lOldStart );

        // Now try to include any between-word space that can be included. 
        // up until the old end limit.
        // We do not want to do this if there are no phrase elements found 
        // at all (in this case, the range should be degenerate).
        if ( m_cElements )
        {
            m_cpTextRange->MoveEnd( tomWord, 1, NULL );
            if ( GetEnd() > lOldEnd )
            {
                // We overshot the old end, so we should just end at the old end
                SetEnd( lOldEnd );
            }
        }
    }
    else
    {
        // If all of the elements were there, ulElement is equal
        // to the old start element - 1.
        // Otherwise, ulElement is equal to the first phrase element
        // going backwards from ulFirstElement that was not found.

        // Must adjust both the start element and the number of elements
        m_ulStartElement = ulElement + 1;
        m_cElements = ulFirstElement - ulElement;

        // Adjust the start so that only phrase elements are in this range.
        // This is where the end of the range is right now (since we
        // just moved the end before the last phrase element
        // that was found.
        SetStart( GetEnd() );
        
        // Reset the end to its original location (it was moved around
        // by ITextRange::FindTextEnd())
        SetEnd( lOldEnd );

        // Do not include spaces leading up to this phrase
    }

    // If no elements were found in the range, then set the range to be degenerate
    if ( !m_cElements )
    {
        fForward ? SetEnd( lOldStart ) : SetStart( lOldEnd );
    }

#ifdef _DEBUG
    // Fill the array with the bogus phrase element offset value wherever
    // there is an element that did not fall into the "found" range
    // of elements
    const ULONG ulBogusVal = GetEnd() - GetStart();
    for ( ulElement = 0; 
        ulElement < m_pPhraseReplacement->GetNumReplacementElements(); 
        ulElement++ )
    {
        if ( (ulElement < m_ulStartElement) || 
            (ulElement >= (m_ulStartElement + m_cElements)) )
        {
            m_pulElementOffsets[ ulElement ] = ulBogusVal;
        }
    }
#endif

    // All elements in the range should have had their offsets updated above
    // to be absolute locations within the document.
    // Since we now know the start of the range, we can update the offsets
    // of elements we have found relative to that.
    long lStart = GetStart();
    for ( ulElement = m_ulStartElement; 
        ulElement < (m_ulStartElement + m_cElements); 
        ulElement++ )
    {
        // Element is in the range; make its index
        // relative to the start index of the range
        m_pulElementOffsets[ ulElement ] -= lStart;
    }

    // Should return true iff nothing has changed (i.e. everything was already
    // correct)
    if ( pfCorrectionResult )
    {
        *pfCorrectionResult = (( lOldStart == GetStart() ) && 
                            ( lOldEnd == GetEnd() ) && 
                            ( ulOldStartElement == m_ulStartElement ) && 
                            ( cOldElements == m_cElements ));
    }
    return S_OK;

}   /* CDictationRun::CorrectPhraseEltsAndRange */

/**********************************************************************
* CDictationRun::GetAlternatesText *
*----------------------------------*
*	Description:  
*       Called to get the alternates for the given range.
*       Since pRangeForAlts does not necessarily line up
*       with element boundaries, this method will get the 
*       alternates for the closest range it can get that
*       contains whole elements.
*       plAltStart and plAltEnd will point to the start
*       and end of the range for which alternates are 
*       actually being returned.
*       The text for the alternates is given in an array
*       of CoTaskMemAlloced WCHAR *'s, and pcPhrasesReturned
*       points to a count of how many of them there are.
*       apfFitsInRun will be an array of bools indicating
*       which alternates actually do fit in "this"'s range
*       (since some may cover more phrase elements than 
*       are actually in our range right now)
*   Return:
*       S_OK
*       E_POINTER 
*       E_INVALIDARG
*       Return value of CDictationRun::FindNearestWholeElementRange()
*       Return value of CResultContainer::GetAlternatesText()
**********************************************************************/
HRESULT CDictationRun::GetAlternatesText( ITextRange *pRangeForAlts, 
                                         ULONG ulRequestCount,
                                         long *plAltStart,
                                         long *plAltEnd,
                                         WCHAR **ppszCoMemText,
                                         bool *apfFitsInRun,
                                         ULONG *pcPhrasesReturned )
{
    if ( !pRangeForAlts || !plAltStart || !plAltEnd || 
        !ppszCoMemText || !pcPhrasesReturned || !apfFitsInRun )
    {
        return E_POINTER;
    }

    if ( !m_pResultContainer )
    {
        _ASSERTE( false );
        return E_UNEXPECTED;
    }

    // Validate the pRangeForAlts param
    long lRangeStart;
    long lRangeEnd;
    pRangeForAlts->GetStart( &lRangeStart );
    pRangeForAlts->GetEnd( &lRangeEnd );
    if ( !( WithinRange( lRangeStart ) && WithinRange( lRangeEnd ) ) )
    {
        // Asking for alternates from an invalid range
        return E_INVALIDARG;
    }

    // Expand the range to the smallest range containing it that
    // contains whole elements
    ULONG ulStartElement;
    ULONG cElements;
    HRESULT hr = FindNearestWholeElementRange( lRangeStart, lRangeEnd, 
        plAltStart, plAltEnd, &ulStartElement, &cElements );
    
    *pcPhrasesReturned = 0;
    if ( SUCCEEDED( hr ) )
    {
        // Get the alternates text from the result object
        hr = m_pResultContainer->GetAlternatesText( 
            ulStartElement, cElements, ulRequestCount, ppszCoMemText, 
            pcPhrasesReturned );
    }

    if ( SUCCEEDED( hr ) )
    {
        // Remember what these are alternates for
        m_fAltsGotten = true;
        m_ulStartAltElt = ulStartElement;
        m_cAltElt = cElements;
    }

    // For each alternate, mark whether or not it fits in the run
    ULONG ulStartInParent;
    ULONG cEltsInParent;
    HRESULT hrAltInfo;
    for ( ULONG ulAlt = 0; ulAlt < *pcPhrasesReturned; ulAlt++ )
    {
        // Find out which elements this alternate covers
        hrAltInfo = m_pResultContainer->GetAltInfo( ulStartElement, cElements, ulAlt, 
            &ulStartInParent, &cEltsInParent );

        // Check if it falls within our m_ulStartElement and m_cElements
        apfFitsInRun[ ulAlt ] = SUCCEEDED(hrAltInfo) && 
            ( ulStartInParent >= m_ulStartElement ) && 
            ( (ulStartInParent + cEltsInParent) <= (m_ulStartElement + m_cElements) );

        // See if we got a NULL alternate back, if we did then mark it so it gets ignored.
        if ( ppszCoMemText[ ulAlt ] == NULL )
        {
            apfFitsInRun[ ulAlt ] = false;
        }
    }
    
    return hr;
}   /* CDictationRun::GetAlternatesText */

/**********************************************************************
* CDictationRun::GetAltEndpoints *
*--------------------------------*
*	Description:  
*       Returns the start and end points of the text in the document
*       that the alternate (from the last call to GetAlternatesText())
*       would replace
*   Return:
*       S_OK
*       return value of CResultContainer::GetAltInfo()
**********************************************************************/
HRESULT CDictationRun::GetAltEndpoints( ULONG ulAlt,
                                       long *plReplaceStart,
                                       long *plReplaceEnd )
{
    _ASSERTE( m_fAltsGotten );
    _ASSERTE( m_pResultContainer );
    if ( !m_fAltsGotten || !m_pResultContainer )
    {
        return E_UNEXPECTED;
    }

    // Find out which elements this alternate covers
    ULONG ulReplaceStartElt;
    ULONG cReplacedElts;
    HRESULT hr = m_pResultContainer->GetAltInfo( 
        m_ulStartAltElt, m_cAltElt, ulAlt,
        &ulReplaceStartElt, &cReplacedElts );
    if ( FAILED(hr) )
    {
        return hr;
    }
    _ASSERTE( (ulReplaceStartElt + cReplacedElts) <= 
        m_pPhraseReplacement->GetNumReplacementElements() );

    if ( plReplaceStart )
    {
        // Get the start position of the first element that would be replaced
        *plReplaceStart = GetStart() + m_pulElementOffsets[ ulReplaceStartElt ];
    }

    if ( plReplaceEnd )
    {
        // The range that would be replaced ends where the first element that 
        // won't be replaced starts.
        // Get the start position of the first element that would not be
        // replaced (the endpoint), or the end of the run if the last element
        // would be replaced.
        if ( (ulReplaceStartElt + cReplacedElts) == (m_ulStartElement + m_cElements) )
        {
            // The last element in the run would be replaced
            *plReplaceEnd = GetEnd();
        }
        else
        {
            // The last element in the run would not be replaced
            *plReplaceEnd = GetStart() +
                m_pulElementOffsets[ ulReplaceStartElt + cReplacedElts ];
        }
    }
    return S_OK;
}   /* CDictationRun::GetAltEndpoints */

/**********************************************************************
* CDictationRun::ChooseAlternate *
*--------------------------------*
*	Description:  
*       Called when the user wishes to commit an alternate
*       from the last group obtained via GetAlternatesText()
*   Return:
*       S_OK
*       return value of CResultContainer::ChooseAlternate
**********************************************************************/
HRESULT CDictationRun::ChooseAlternate( ULONG ulAlt ) 
{
    _ASSERTE( m_fAltsGotten && m_pResultContainer );
    if ( !m_fAltsGotten || !m_pResultContainer )
    {
        return E_UNEXPECTED;
    }

    // Find out which elements will be replaced
    ULONG ulActualAltStart;
    ULONG cActualAltElts;
    HRESULT hr = m_pResultContainer->GetAltInfo( 
        m_ulStartAltElt, m_cAltElt, ulAlt, &ulActualAltStart, &cActualAltElts );

    // Get the text for the alternate that will replace these elements
    CSpDynamicString dstrText;
    BYTE bDisplayAttributes = 0;
    if (SUCCEEDED(hr))
    {
        hr = m_pResultContainer->GetAltText( m_ulStartAltElt, m_cAltElt,
            ulAlt, &dstrText, &bDisplayAttributes );
    }
    
    // cpRangeToReplace will cover the text of the elements to replace
    CComPtr<ITextRange> cpRangeToReplace;
    if (SUCCEEDED(hr))
    {
        hr = m_cpTextRange->GetDuplicate( &cpRangeToReplace );
    }

    // Set the range of cpRangeToReplace to contain the correct text
    if (SUCCEEDED(hr))
    {
        // The start is the start of the first element covered by the alternate
        cpRangeToReplace->SetStart( GetStart() + m_pulElementOffsets[ulActualAltStart] );
        
        // The end is the start of the first element not covered by the alternate,
        // or the end of this DictationRun's range, if the last element in the run
        // is covered by the alternate
        if ( (ulActualAltStart + cActualAltElts) == (m_ulStartElement + m_cElements) )
        {
            // The last element in the run is covered
            cpRangeToReplace->SetEnd( GetEnd() );
        }
        else
        {
            // The last element in the run is not covered
            cpRangeToReplace->SetEnd( GetStart() + 
                m_pulElementOffsets[ulActualAltStart + cActualAltElts] );
        }
    }

    // Deal with display attributes:

    // pwszTextToInsert will contain the actual text to insert, with the 
    // correct spaces
    // At most, it will need space for the recognized text, a terminating nul,
    // two spaces before, and two spaces after
    WCHAR *pwszTextToInsert = new WCHAR[ wcslen( dstrText ) + 5 ];
    if ( !pwszTextToInsert )
    {
        return E_OUTOFMEMORY;
    }
    pwszTextToInsert[0] = 0;

    // Consume spaces before the alternate if necessary,
    // or add spaces before the alternate.
    // To be safe, we will not consume leading spaces outside of
    // this run
    if ( (ulActualAltStart > m_ulStartElement) && 
        (SPAF_CONSUME_LEADING_SPACES & bDisplayAttributes) )
    {
        // Move the start of the range back until is is
        // right after a non-space character
        HRESULT hrMove;
        long lFirstChar;
        do
        {
            hrMove = cpRangeToReplace->MoveStart( -1, tomCharacter, NULL );
            
            // Look at the first character that is now in the range
            lFirstChar = 0;
            cpRangeToReplace->GetChar( &lFirstChar );

        }   while (( S_OK == hrMove ) && ( L' ' == ((WCHAR) lFirstChar) ));
        
        if ( S_OK == hrMove )
        {
            // We have found a non-space character to the left of the start, 
            // move just past it
            cpRangeToReplace->MoveStart( 1, tomCharacter, NULL );
        }
    }
    else
    {
        // If the previous element calls for trailing spaces and there aren't 
        // any, we need to add them
        UINT uiNumLeadingSpacesNeeded = 1;
        BYTE bPreviousDisplayAttributes = 0;
        if (( ulActualAltStart > 0 ) && ( ulActualAltStart > m_ulStartElement ))
        {
            m_pPhraseReplacement->GetDisplayText( ulActualAltStart - 1,
                &bPreviousDisplayAttributes );
            if ( bPreviousDisplayAttributes & SPAF_ONE_TRAILING_SPACE )
            {
                uiNumLeadingSpacesNeeded = 1;
            }
            else if ( bPreviousDisplayAttributes & SPAF_TWO_TRAILING_SPACES )
            {
                uiNumLeadingSpacesNeeded = 2;
            }
            else
            {
                uiNumLeadingSpacesNeeded = 0;
            }
        }

        if ( uiNumLeadingSpacesNeeded > 0 )
        {
            // Now look back to previous characters
            CComPtr<ITextRange> cpDupRange;
            if ( FAILED( cpRangeToReplace->GetDuplicate( &cpDupRange ) ) )
            {
                hr = E_OUTOFMEMORY;
            }
            long lDupRangeStart = 0;
            cpDupRange->GetStart( &lDupRangeStart );

            if ( lDupRangeStart > 0 )
            {
                cpDupRange->MoveStart( tomCharacter, -1, NULL );
                lDupRangeStart--;

                long lFirstChar = 0;
                cpDupRange->GetChar( &lFirstChar );
                if ( !iswspace( (WCHAR) lFirstChar ) )
                {
                    wcscat( pwszTextToInsert, 
                        (1 == uiNumLeadingSpacesNeeded) ? L" " : L"  " );
                }
                else if ( 2 == uiNumLeadingSpacesNeeded )
                {
                    // If two spaces are called for but only one is there,
                    // we need an additional space
                    if ( lDupRangeStart > 0 )
                    {
                        cpDupRange->GetChar( &lFirstChar );
                        if ( !iswspace( (WCHAR) lFirstChar ) )
                        {
                            wcscat( pwszTextToInsert, L" " );
                        }
                    }
                }
            }
        }
    }

    // Get the display attributes for the element to follow.
    // (If the alt covers the last element, CPhraseReplacement::GetDisplayText()
    // will return NULL)
    BYTE bFollowingDisplayAttributes = 0;
    const WCHAR *pwszText = m_pPhraseReplacement->GetDisplayText( 
        ulActualAltStart + cActualAltElts,
        &bFollowingDisplayAttributes );
    bool fConsumeLeadingSpacesAfterAlt = 
        pwszText && (bFollowingDisplayAttributes & SPAF_CONSUME_LEADING_SPACES);

    // Append spaces if necessary: That is, if the attribute itself
    // calls for trailing spaces and the next element is not 
    // CONSUME_LEADING_SPACES
    if ( !fConsumeLeadingSpacesAfterAlt )
    {
        if ( SPAF_TWO_TRAILING_SPACES & bDisplayAttributes )
        {
            dstrText.Append( L"  " );
        }
        else if ( SPAF_ONE_TRAILING_SPACE & bDisplayAttributes )
        {
            dstrText.Append( L" " );
        }
    }
    
    // Get the recognized text and any trailing spaces
    wcscat( pwszTextToInsert, dstrText );

    if ( SUCCEEDED( hr ) )
    {
        BSTR bstrText = ::SysAllocString( pwszTextToInsert );

        // Change the text
        hr = cpRangeToReplace->SetText( bstrText );

        ::SysFreeString( bstrText );
    }

    delete[] pwszTextToInsert;

    if ( SUCCEEDED( hr ) )
    {
        // Notify the common reco result so that all runs sharing
        // this result can adjust their offset maps if necessary
        // (as the indices of elements may have changed)
        BYTE b;
        hr = m_pResultContainer->ChooseAlternate( 
            m_ulStartAltElt, m_cAltElt, ulAlt, &b );
    }

    // At this point the alternate has been committed to the phrase,
    // and our PhraseReplacement object knows about the committed alternate
    
    if ( SUCCEEDED( hr ) )
    {
        // This will get us the new offsets for the phrase elements we 
        // now have
        hr = CorrectPhraseEltsAndRange( true );
    }

    return hr;
}   /* CDictationRun::ChooseAlternate */


/**********************************************************************
* CDictationRun::OnAlternateCommit *
*----------------------------------*
*	Description:  
*       Called right before an alternate is committed on this 
*       DictationRun's RecoResult.  
*       If the commit has resulted in an increase in the 
*       number of elements, the offset map will need to be freed 
*       and allocated again.
*       Move the element offset information so that it is correct.
*   Return:
*       S_OK
*       E_OUTOFMEMORY
**********************************************************************/
HRESULT CDictationRun::OnAlternateCommit( ULONG ulStartEltToBeReplaced, 
                                       ULONG cEltsToBeReplaced,
                                       ULONG cElementsInPhraseAfterCommit )
{
    // Determine the change in element count (i.e. whether there will now be 
    // more, fewer, or the same number of elements)
    long lElementShift = cElementsInPhraseAfterCommit 
                        - m_pPhraseReplacement->GetNumReplacementElements();
    
    if ( !lElementShift )
    {
        // Nothing needs to be done
        return S_OK;
    }

    if ( (ulStartEltToBeReplaced + cEltsToBeReplaced) > 
        m_pPhraseReplacement->GetNumReplacementElements() )
    {
        // Out-of-bounds arguments
        _ASSERTE( false );
        return E_UNEXPECTED;
    }

    if ( lElementShift > 0 )
    {
        // The number of elements has increased: need to reallocate
        ULONG *pulNewOffsets = new ULONG[ cElementsInPhraseAfterCommit ];
        if ( !pulNewOffsets )
        {
            return E_OUTOFMEMORY;
        }
        ULONG ulElement; 

#ifdef _DEBUG
        // Fill the element offsets with a bogus value
        const ULONG ulBogusVal = GetEnd() - GetStart();
        for( ulElement = 0; 
             ulElement < cElementsInPhraseAfterCommit; 
             ulElement++ )
        {
            pulNewOffsets[ulElement] = ulBogusVal;
        }
#endif
        
        // Copy over the relevant elements.
        if ( (m_ulStartElement + m_cElements) <= ulStartEltToBeReplaced )
        {
            // The replacement occurs after this run
            // so we can just copy over the entries.
            for ( ulElement = m_ulStartElement; 
                ulElement < (m_ulStartElement + m_cElements); 
                ulElement++ )
            {
                pulNewOffsets[ulElement] = m_pulElementOffsets[ulElement];
            }
        }
        else if ( m_ulStartElement >= (ulStartEltToBeReplaced + cEltsToBeReplaced) )
        {
            // The replacement occurs before this run, so shifting is necessary
            // Shift the entries to the right (recall that lElementShift > 0)
            for ( ulElement = m_ulStartElement; 
                ulElement < (m_ulStartElement + m_cElements); 
                ulElement++ )
            {
                pulNewOffsets[ulElement + lElementShift] = m_pulElementOffsets[ulElement];
            }

            // The start element also needs to be shifted
            m_ulStartElement += lElementShift;
        }

        // If this run is the run in which the replacement will take place,
        // do not do anything here, as this run will later do its own thing to
        // recalculate its element offsets

        // Delete the old map, and make the element offset map point to the new map
        delete[] m_pulElementOffsets;
        m_pulElementOffsets = pulNewOffsets;
    }
    else
    {
        // The number of phrase elements has decreased, so we already have
        // enough space allocated for the map

        // Sanity check: We had better not be shifting back past zero
        if ( (lElementShift < 0) && ((ulStartEltToBeReplaced + cEltsToBeReplaced) < (ULONG)(-lElementShift)) )
        {
            _ASSERTE( false );
            return E_UNEXPECTED;
        }

        // Copy over the offset info if necessary 
        ULONG ulElement;
        if ( m_ulStartElement >= (ulStartEltToBeReplaced + cEltsToBeReplaced) )
        {
            // The replacement occurs before this run.
            // Shift the entries to the left (recall that lElementShift < 0,
            // and that is why we are adding lElementShift, not subtracting)
            for ( ulElement = m_ulStartElement; 
                ulElement < (m_ulStartElement + m_cElements); 
                ulElement++ )
            {
                m_pulElementOffsets[ulElement + lElementShift] = 
                    m_pulElementOffsets[ulElement];
            }

            // The start element also needs to be shifted
            m_ulStartElement += lElementShift;
        }

        // If the replacement occurs after this run, all of the phrase 
        // element information is already correct.

        // If this is the run in which the alternate was committed, do not
        // do anything here, as this run will later do its own thing to
        // recalculate its element offsets
    }

    return S_OK;
}   /* CDictationRun::OnAlternateCommit */

/**********************************************************************
* CDictationRun::Speak *
*----------------------*
*	Description:  
*		Speaks all of the audio associated with this text run.  Uses
*       TTS if there's a problem speaking the audio data.
*
* 	Return:
*		S_OK
*       S_FALSE for a degenerate run
*       Return value of CResultContainer::SpeakAudio()
**********************************************************************/
HRESULT CDictationRun::Speak( ISpVoice &rVoice )
{
    _ASSERTE( m_pResultContainer && m_pPhraseReplacement && m_cpTextRange );
    if ( !m_pResultContainer || !m_pPhraseReplacement || !m_cpTextRange )
    {
        return E_UNEXPECTED;
    }

    if ( !m_cElements )
    {
        // This run is just a place-holder, no need to speak it
        return S_FALSE;
    }

    // Try to speak it as audio
    HRESULT hr = m_pResultContainer->SpeakAudio( 
        m_ulStartElement, m_cElements );

    if( FAILED( hr ) )
    {
        // Speak it as text
        hr = CTextRun::Speak( rVoice );
    }
    return hr;

}   /* CDictationRun::Speak */

/**********************************************************************
* CDictationRun::Speak *
*----------------------*
*	Description: 
*       Finds the smallest range that contains *plStart -> *plEnd  
*       and still contains whole elements.  Returns the start
*       and end of the text it actually spoke in *plStart and *plEnd.
*		Speaks the audio associated with this range within this text run.  
*       Uses TTS if there's a problem speaking the audio data.
*
*       Out-of-bounds *plStart causes the range to be spoken from
*       the start.
*       Out-of-bounds *plEnd causes the range to be spoken all the
*       way to the end.
* 	Return:
*		S_OK 
*       E_POINTER
*       E_OUTOFMEMORY
*       Return value of CDictationRun::FindNearestWholeElementRange()
*       Return value of ITextRange::{Get,Set}{Start,End}()
*       Return value of CDictationRun::Speak()
**********************************************************************/
HRESULT CDictationRun::Speak( ISpVoice &rVoice,
                             long *plStart,
                             long *plEnd )
{
    if ( !plStart || !plEnd )
    {
        return E_POINTER;
    }

    // This function actually temporarily shrinks this DictationRun's
    // range, so we need to store all of its information before 
    // proceeding.

    // Save the old range
    const long lOldStart = GetStart();
    const long lOldEnd = GetEnd();

    // Save the old phrase element information
    const ULONG ulOldStartElement = m_ulStartElement;
    const ULONG cOldElements = m_cElements;

    // Save the element offsets
    const ULONG ulOffsetArraySize = 
        sizeof(ULONG) * m_pPhraseReplacement->GetNumReplacementElements();
    ULONG *pulOldElementOffsets = new ULONG[ ulOffsetArraySize ];
    if ( !pulOldElementOffsets )
    {
        return E_OUTOFMEMORY;
    }
    memcpy( pulOldElementOffsets, m_pulElementOffsets, ulOffsetArraySize );

    // Fix out-of-bounds limits, since these mean to speak the entire run
    if ( !WithinRange( *plStart ) )
    {
        *plStart = lOldStart;
    }
    if ( !WithinRange( *plEnd ) )
    {
        *plEnd = lOldEnd;
    }

    // Find the smallest range containing this range that contains only
    // whole elements
    long lWholeEltStart;
    long lWholeEltEnd;
    HRESULT hr = FindNearestWholeElementRange( 
        *plStart, *plEnd, &lWholeEltStart, &lWholeEltEnd, NULL, NULL );

    // Set the end to the end of the range to speak
    if ( SUCCEEDED( hr ) )
    {
        // The limits of this whole-element range are what we are going to speak 
        *plStart = lWholeEltStart;
        *plEnd = lWholeEltEnd;
 
        hr = m_cpTextRange->SetEnd( *plEnd );
    }

    // Find out what phrase elements are now contained
    if ( SUCCEEDED( hr ) )
    {
        hr = CorrectPhraseEltsAndRange( true );
    }

    if ( SUCCEEDED( hr ) )
    {
        // CDictationRun::CorrectPhraseEltsAndRange() might have moved the end
        hr = m_cpTextRange->GetEnd( plEnd );
    }

        // Set the start to the start of the range to speak
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpTextRange->SetStart( *plStart );
    }

    // Find out what phrase elements are now contained
    if ( SUCCEEDED( hr ) )
    {
        hr = CorrectPhraseEltsAndRange( false ); 
    }

    if ( SUCCEEDED( hr ) )
    {
        // CDictationRun::CorrectPhraseEltsAndRange() might have moved the start
        hr = m_cpTextRange->GetStart( plStart );
    }

    if ( SUCCEEDED( hr ) )
    {
        // Pass to CDictationRun::Speak() in order to speak the entire range,
        // which now consists solely of what we want to speak
        hr = Speak( rVoice );
    }

    // Restore old range and phrase element info
    SetStart( lOldStart );
    SetEnd( lOldEnd );
    m_ulStartElement = ulOldStartElement;
    m_cElements = cOldElements;
    memcpy( m_pulElementOffsets, pulOldElementOffsets, ulOffsetArraySize );
    delete[] pulOldElementOffsets;

    return hr;
}   /* CDictationRun::Speak */

/**********************************************************************
* CDictationRun::Serialize *
*--------------------------*
*	Description:  
*		Serializes the text and audio data for this run.
*       Writes the data to the pStream.
*       There are two headers to precede the seralized run;
*       one containing information about the run as a generic
*       text run, and one containing information about the 
*       run and its dictation-specific information.
*   Return:
*       S_OK
*       E_POINTER
*       Return value of CResultContainer::Serialize()
*       Return value of ISequentialStream::Write()
**********************************************************************/
HRESULT CDictationRun::Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt )
{   
    if ( !pStream  || !pRecoCtxt )
    {
        return E_POINTER;
    }
    _ASSERTE( m_cpTextRange && m_pResultContainer );
    if ( !m_cpTextRange || !m_pResultContainer )
    {
        return E_UNEXPECTED;
    }

    // Make the relevant headers

    // This header deals with the run as a generic text run
    RUNHEADER runHdr;
    runHdr.lStart = GetStart();
    runHdr.lEnd = GetEnd();
    runHdr.bResultFollows = true;

    // This header deals with the run as a CDictationRun
    DICTHEADER dictHdr;
    dictHdr.ulStartElement = m_ulStartElement;
    dictHdr.cElements = m_cElements;

    // Get the phrase blob from the recoresult
    SPSERIALIZEDRESULT * pResultBlock;
    HRESULT hr = m_pResultContainer->Serialize( &pResultBlock );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Get the size of the result block
    dictHdr.cbSize = pResultBlock->ulSerializedSize;

    // Write the headers and the phrase blob to the stream
    ULONG cbWritten;
    hr = pStream->Write( &runHdr, sizeof( RUNHEADER ), &cbWritten );
    if ( SUCCEEDED( hr ) )
    {
        hr = pStream->Write( &dictHdr, sizeof( DICTHEADER ), &cbWritten );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = pStream->Write( pResultBlock, dictHdr.cbSize, &cbWritten );
    }

    return hr;
}   /* CDictationRun::Serialize */

/**********************************************************************
* CDictationRun::IsConsumeLeadingSpaces *
*---------------------------------------*
*	Description:  
*       Sets *pfConsumeLeadingSpaces to true iff any text inserted
*       at lPos would have to consume a space that follows it.
*       This value will be false UNLESS lPos is the first
*       position of a phrase element that has the attribute
*       SPAF_CONSUME_LEADING_SPACES
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
*       return value of CDictationRun::FindNearestWholeElementRange()
*       E_FAIL if the CPhraseReplacement::GetDisplayText() fails
**********************************************************************/
HRESULT CDictationRun::IsConsumeLeadingSpaces( const long lPos,
                                              bool *pfConsumeLeadingSpaces )
{
    if ( !pfConsumeLeadingSpaces )
    {
        return E_POINTER;
    }
    if ( !WithinRange( lPos ) )
    {
        return E_INVALIDARG;
    }
    *pfConsumeLeadingSpaces = false;     // by default

    // Expand lPos to the nearest element
    long lEltStartPos;
    long lEltEndPos;
    ULONG ulStartElt;
    ULONG cElts;
    HRESULT hr = FindNearestWholeElementRange( lPos, lPos, 
        &lEltStartPos, &lEltEndPos, &ulStartElt, &cElts );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Should have found only one element
    _ASSERTE( 1 == cElts );

    // We definitely don't want to consume leading spaces unless
    // this is the beginning of the element
    if ( lPos != lEltStartPos )
    {
        return S_OK;
    }
    
    // Get the text and attributes for that element
    BYTE bDisplayAttributes;
    const WCHAR *pwszEltText = m_pPhraseReplacement->GetDisplayText( 
        ulStartElt, &bDisplayAttributes );   
    if ( !pwszEltText )
    {
        return E_FAIL;
    }

    // Check for the CONSUME_LEADING_SPACES attribute
    if ( SPAF_CONSUME_LEADING_SPACES & bDisplayAttributes )
    {
        *pfConsumeLeadingSpaces = true;
    }
    
    return S_OK;
}   /* CDictationRun::IsConsumeLeadingSpaces */

/**********************************************************************
* CDictationRun::HowManySpacesAfter *
*-----------------------------------*
*	Description:  
*       Returns the number of spaces that would need to precede text
*       if text were to be inserted at position lPos.
*       This number goes in the out param puiSpaces.
*       The value returned will be zero UNLESS lPos immediately
*       follows the text of a phrase element that has 
*       SPAF_ONE_TRAILING_SPACE or SPAF_TWO_TRAILINGSPACES attributes
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
*       return value of CDictationRun::FindNearestWholeElementRange()
*       E_FAIL if the CPhraseReplacement::GetDisplayText() fails
**********************************************************************/
HRESULT CDictationRun::HowManySpacesAfter( const long lPos, 
                                          UINT *puiSpaces )
{
    if ( !puiSpaces )
    {
        return E_POINTER;
    }
    if ( !WithinRange( lPos ) )
    {
        return E_INVALIDARG;
    }
    *puiSpaces = 0;     // by default

    // Expand lPos to the nearest element
    long lEltStartPos;
    long lEltEndPos;
    ULONG ulStartElt;
    ULONG cElts;
    HRESULT hr = FindNearestWholeElementRange( lPos, lPos, 
        &lEltStartPos, &lEltEndPos, &ulStartElt, &cElts );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Should have found only one element
    _ASSERTE( 1 == cElts );
    
    // Get the text and attributes for that element
    BYTE bDisplayAttributes;
    const WCHAR *pwszEltText = m_pPhraseReplacement->GetDisplayText( 
        ulStartElt, &bDisplayAttributes );   
    if ( !pwszEltText )
    {
        return E_FAIL;
    }

    // If that element asks for leading spaces to be consumed and
    // lPos is at the beginning of that element, we actually
    // want to be looking at the element for the previous space, 
    // since those two elements may run together.
    // (For instance, if lPos fell between the 'd' and the '.' in 
    // "This sentence ends with a period."  We want attributes for
    // "period", not ".")
    if (( bDisplayAttributes & SPAF_CONSUME_LEADING_SPACES ) 
        && ( lPos == lEltStartPos ) 
        && ( lPos > GetStart() ))
    {
        // Get the element that (lPos - 1) is on
        hr = FindNearestWholeElementRange( lPos - 1, lPos - 1, 
            &lEltStartPos, &lEltEndPos, &ulStartElt, &cElts );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        // Should have found only one element
        _ASSERTE( 1 == cElts );
    
        // Get the text and attributes for that element
        pwszEltText = m_pPhraseReplacement->GetDisplayText( 
            ulStartElt, &bDisplayAttributes );   
        if ( !pwszEltText )
        {
            return E_FAIL;
        }
    }

    // Now look for attributes if lPos falls at the end of this element
    if ( (lEltStartPos + (long) wcslen( pwszEltText )) == lPos )
    {
        // lPos immediately follows the element text,
        // so pay attention to the display attributes
        if ( SPAF_ONE_TRAILING_SPACE & bDisplayAttributes )
        {
            *puiSpaces = 1;
        }
        else if ( SPAF_TWO_TRAILING_SPACES & bDisplayAttributes )
        {
            *puiSpaces = 2;
        }
    }
    else if ( (lEltStartPos + (long) wcslen( pwszEltText ) + 1) == lPos )
    {
        // lPos is the position after the position immediately following
        // the element text.  We care about this situation only if 
        // we want two trailing spaces, in which case we need only one more
        if ( SPAF_TWO_TRAILING_SPACES & bDisplayAttributes )
        {
            *puiSpaces = 1;
        }
    }

    // Otherwise text can be inserted right at lPos

    return S_OK;
}   /* CDictationRun::HowManySpacesAfter */


/**********************************************************************
* CDictationRun::FindNearestWholeElementRange *
*---------------------------------------------*
*	Description:  
*       Finds the start and end of the smallest range containing
*       the range with lStart and lEnd that contains whole elements.
*       Upon return, pulStartElement and pcElements will point 
*       to information about the elements to which the range 
*       best corresponds.
*       Upon return, plResultStart and plResultEnd will point
*       to the start and end indices (in the document) of this
*       element range.
*   Return:
*       S_OK
*       E_POINTER
*       E_FAIL if there are no elements in this range
*       E_INVALIDARG if the range lStart...lEnd is not contained 
*                       within our range
**********************************************************************/
HRESULT CDictationRun::FindNearestWholeElementRange( long lStart,
                                                    long lEnd,
                                                    long *plResultStart,
                                                    long *plResultEnd,
                                                    ULONG *pulStartElement,
                                                    ULONG *pcElements )
{
    if ( !plResultStart || !plResultEnd )
    {
        return E_POINTER;
    }
    _ASSERTE( m_cpTextRange && m_pulElementOffsets );
    if ( !m_cpTextRange || !m_pulElementOffsets )
    {
        return E_UNEXPECTED;
    }

    if ( !m_cElements )
    {
        return E_FAIL;
    }
    
    if ( !( WithinRange( lStart ) && WithinRange( lEnd ) ) )
    {
        // Asking for out-of-range text
        return E_INVALIDARG;
    }

    ULONG ulThisRunStart = (ULONG) GetStart();
    ULONG ulStart = (ULONG)lStart;

    // Find the last element offset that is 
    // less than or equal to lStart.
    // This is accomplished by going backwards through the 
    // elements until we find one with an offset that is <= lStart,
    // or until we hit the start element
    ULONG ulElement;
    for ( ulElement = m_ulStartElement + m_cElements - 1; 
        (ulElement > m_ulStartElement) && ((ulThisRunStart + m_pulElementOffsets[ulElement]) > ulStart); 
        ulElement-- )
        ;
    ULONG ulStartElement = ulElement;
    _ASSERTE( m_pulElementOffsets[ulStartElement] <= ulStart );

    // Get the result into the out params
    *plResultStart = ulThisRunStart + m_pulElementOffsets[ulStartElement];
    if ( pulStartElement )
    {
        *pulStartElement = ulStartElement;
    }

    // Find the first element offset that is 
    // greater than or equal to lEnd.
    // This is the first element that we will not include.
    ULONG ulEnd = (ULONG) lEnd;
    for ( ulElement = ulStartElement; 
        (ulElement < m_ulStartElement + m_cElements) && 
            ((ulThisRunStart + m_pulElementOffsets[ulElement]) < ulEnd); 
        ulElement++ )
        ;
    _ASSERTE( ulElement >= ulStartElement );
    if ( ulElement == ulStartElement )
    {
        // The range was degenerate and at the beginning of a phrase element; 
        // we should include one element 
        ulElement++;
    }

    // Get the result into the out params
    if ( (m_ulStartElement + m_cElements) == ulElement )
    {
        // Our range contains part of the final element,
        // so get the end of the range from the end of 
        // the document
        *plResultEnd = GetEnd();
    }
    else
    {
        // Get the end of the range from the offset of the 
        // first element not included.
        *plResultEnd = ulThisRunStart + m_pulElementOffsets[ulElement];
    }
    if ( pcElements )
    {
        // ulElement is the index of the first element not included.
        // If the last element in the run was included, then ulElement will
        // be one more than that.
        *pcElements = ulElement - ulStartElement;
    }

    return S_OK;
}   /* CDictationRun::FindNearestWholeElementRange */

// Helper function

/*********************************************************************
* ContainsNonWhitespace() *
*-------------------------*
*   Description:
*       Returns true iff some character in the string is
*       not whitespace.
**********************************************************************/
bool ContainsNonWhitespace( const WCHAR *pwsz )
{
    if ( !pwsz )
    {
        return false;
    }

    const WCHAR *pwc;
    for ( pwc = pwsz; *pwc; pwc++ )
    {
        if ( !iswspace( *pwc ) )
        {
            return true;
        }
    }

    // Did not hit a whitespace character
    return false;
}   /* ContainsNonWhitespace */
