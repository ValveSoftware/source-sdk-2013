/******************************************************************************
*	textrun.cpp 
*       Implementation details for the CTextRun object which is our base class
*       for tracking all the text (be it dictated or typed) in our product.
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#include "stdafx.h"
#include "textrun.h"

/**********************************************************************
* CTextRun::CTextRun *
*--------------------*
*	Description:  
*		CTextRun constructor 
**********************************************************************/
CTextRun::CTextRun() :  m_cpTextRange( NULL ),
                        m_dwRefCount( NULL )
{
}   /* CTextRun::CTextRun */


/**********************************************************************
* CTextRun::~CTextRun *
*---------------------*
*	Description:  
*		CTextRun destructor 
**********************************************************************/
CTextRun::~CTextRun()
{
}   /* CTextRun::~CTextRun */

/**********************************************************************
* CTextRun::IncrementCount *
*--------------------------*
*	Description:  
*		Increments a reference count on this text run. 
*
* 	Return:
*		The current count
**********************************************************************/
DWORD CTextRun::IncrementCount()
{
    m_dwRefCount++;
    return m_dwRefCount;
}   /* CTextRun::IncrementCount */

/**********************************************************************
* CTextRun::DecrementCount *
*--------------------------*
*	Description:  
*		Decrements a reference count on this text run. If the
*       count drops to zero, the text run is deleted.
*
* 	Return:
*		The current count
**********************************************************************/
DWORD CTextRun::DecrementCount()
{
    _ASSERTE( m_dwRefCount );

    DWORD dwRefCount = --m_dwRefCount;
    if( !m_dwRefCount )
    {
        delete this;
    }
    return dwRefCount;
}   /* CTextRun::DecrementCount */

/******************************************************************************
* CTextRun::SetTextRange *
*------------------------*
*   Description:
*       Stores the text range interface pointer for this run
*   Return:
*       S_OK
*       E_POINTER if pTextRange is NULL
******************************************************************************/
HRESULT CTextRun::SetTextRange( ITextRange *pTextRange )
{
    if ( !pTextRange )
    {
        return E_POINTER;
    }

    m_cpTextRange = pTextRange;
    
    return S_OK;
}   /* CTextRun::SetTextRange */

/**********************************************************************
* CTextRun::Split *
*-----------------*
*	Description:  
*		Splits up a TextRun so that this text run now ends at lFirstEnd
*       and the second text run begins at lSecondBegin.
*       "This" will now be a shorter range (it will end sooner),
*       and *ppTextRun will point to the new text run 
*       (space will be allocated here for *ppTextRun) 
*       to be inserted in the list.
*
* 	Return:
*		S_OK 
*       E_INVALIDARG 
*       E_OUTOFMEMORY 
*       Return value of ITextDocument::Range()
**********************************************************************/
HRESULT CTextRun::Split( long *plFirstEnd, long *plSecondBegin, 
                        ITextDocument *cpTextDoc,
                        CTextRun **ppTextRun )
{
    if ( !plFirstEnd || !plSecondBegin || !cpTextDoc || !ppTextRun )
    {
        return E_INVALIDARG;
    }
    _ASSERTE( m_cpTextRange );
    if ( !m_cpTextRange )
    {
        return E_UNEXPECTED;
    }

    // These values won't be changing, since this run has no associated
    // RecoResult. 
    // We can chop this block right at these positions.
    long lFirstEnd = *plFirstEnd;
    long lSecondBegin = *plSecondBegin;

    if ( !WithinRange( lFirstEnd ) || (lFirstEnd > lSecondBegin) )
    {
        return E_INVALIDARG;
    }

    if ( (GetStart() == lSecondBegin) || (GetEnd() == lFirstEnd) || (lSecondBegin > GetEnd()) )
    {
        // Don't need to do anything, since we are asking for both of the 
        // cuts to be made on already-existing TextRun boundaries
        *ppTextRun = NULL;
        return S_OK;
    }

    *ppTextRun = new CTextRun();
    if ( !(*ppTextRun) )
    {
        return E_OUTOFMEMORY;
    }

    // The latter range will start at lSecondBegin and end where "this" ended
    long lEnd = GetEnd();
    CComPtr<ITextRange> pLatterRange;
    HRESULT hr = cpTextDoc->Range( lSecondBegin, lEnd, &pLatterRange );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    (*ppTextRun)->SetTextRange( pLatterRange );

    // Adjust the end of "this"'s range; it will end at lFirstEnd
    m_cpTextRange->SetEnd( lFirstEnd );

    return S_OK;

} /* CTextRun::Split */


/**********************************************************************
* CTextRun::Concatenate *
*-----------------------*
*	Description:  
*		If possible, concatenates pNext (pPrev if fConcatAfter is false)
*       onto the end of this.
*       Another CTextRun can always be concatenated on, unless it 
*       contains dictation.
*
* 	Return:
*		E_NOMERGE if could not be merged (because pTextRun is dictation)
*       E_FULLMERGE
**********************************************************************/
MERGERESULT CTextRun::Concatenate( CTextRun *pTextRun, bool fConcatAfter )
{
    if ( !pTextRun || !m_cpTextRange )
    {
        return E_NOMERGE;
    }

    // Check for compatibility: In this case, neither mergee can be 
    // a dict run
    if ( IsDict() || pTextRun->IsDict() )
    {
        return E_NOMERGE;
    }

    // lNewBound will be the new end (resp. start) of the run, if the 
    // concatenation is successful
    long lNewBound;
   
    // Concatenation is possible iff one run ends exactly where the other
    // begins.
    // If concatenation is possible, do it.
    if ( fConcatAfter )
    {
        // Will be concatenating pTextRun onto the end of this
        if ( GetEnd() != pTextRun->GetStart() )
        {
            // They are not consecutive runs
            return E_NOMERGE;
        }

        // lNewBound will be the new end of the run, if the 
        // concatenation is successful
        lNewBound = pTextRun->GetEnd();
        
        // Swallow up pTextRun by setting our end to its end
        SetEnd( lNewBound );

        // Make pTextRun degenerate
        pTextRun->SetStart( lNewBound );
        
    }
    else
    {
        // Will be concatenating pTextRun onto the beginning of this
        if ( GetStart() != pTextRun->GetEnd() )
        {
            return E_NOMERGE;
        }

        // lNewBound will be the new start of the run, if the 
        // concatenation is successful
        lNewBound = pTextRun->GetStart();

        // Swallow up pTextRun by setting our start to its start
        SetStart( lNewBound );

        // Make pTextRun degenerate
        pTextRun->SetEnd( lNewBound );
    }

    return E_FULLMERGE;
}   /* CTextRun::Concatenate */

/**********************************************************************
* CTextRun::Speak *
*-----------------*
*	Description:  
*		Speaks the text associated with this CTextRun using TTS.
*
* 	Return:
*		S_OK
*       Return value of ITextRange::GetText()
*       Return value of ISpVoice::Speak()
**********************************************************************/
HRESULT CTextRun::Speak( ISpVoice &rVoice )
{
    _ASSERTE( m_cpTextRange );
    if ( !m_cpTextRange )
    {
        return E_UNEXPECTED;
    }

    // Get the text and speak it.
    BSTR bstrText;
    HRESULT hr = m_cpTextRange->GetText( &bstrText );
    if( SUCCEEDED( hr ) )
    {
        hr = rVoice.Speak( bstrText, SPF_ASYNC, NULL );

        ::SysFreeString( bstrText );
    }
    return hr;
} /* CTextRun::Speak */

/**********************************************************************
* CTextRun::Speak() *
*-------------------*
*	Description:  
*       Uses *plStart and *plEnd to find the nearest start and 
*       endpoints for speaking (to the nearest word).
*       Returns these values in plStart and plEnd.
*		Speaks the text associated with this CTextRun from *plStart 
*       to *plEnd.  
*       
*       If *plStart is not within the range, then 
*       start at the beginning.  If lEnd is not within range, then
*       end at the end.  
*
* 	Return:
*       S_OK
*       E_POINTER
*		Return value of CTextRun::Speak()
**********************************************************************/
HRESULT CTextRun::Speak( ISpVoice &rVoice, 
                               long *plStart,
                               long *plEnd )
{
    if ( !plStart || !plEnd )
    {
        return E_POINTER;
    }
    _ASSERTE( m_cpTextRange );
    if ( !m_cpTextRange )
    {
        return E_UNEXPECTED;
    }

    // Save the old range
    long lOldStart = GetStart();
    long lOldEnd = GetEnd();

    // Out of range start or end means we start speaking from the start
    // or end (resp.) of the text range.
    
    if ( WithinRange( *plStart ) )
    {
        // The start needs to be moved
        SetStart( *plStart );
    }
    else
    {
        *plStart = GetStart();
    }

    if ( WithinRange( *plEnd ) )
    {
        // The end needs to be moved
        SetEnd( *plEnd );
    }
    else
    {
        *plEnd = GetEnd();
    }

    // Expand to include whole words
    m_cpTextRange->Expand( tomWord, NULL );

    // Get the new start and end so that we can pass them back
    m_cpTextRange->GetStart( plStart );
    m_cpTextRange->GetEnd( plEnd );

    // We should never speak past the end of this run, even if expanding to include
    // whole words caused extra text to be included 
    // (e.g. if you typed "This is a sentence" and dictated some text that 
    // consumed leading spaces right afterwards)
    *plStart = __max( *plStart, lOldStart );
    *plEnd = __min( *plEnd, lOldEnd );
    SetStart( *plStart );
    SetEnd( *plEnd );

    // Pass to the CTextRun::Speak() that speaks an entire run
    HRESULT hr = Speak( rVoice );

    // Restore the old range limits
    SetStart( lOldStart );
    SetEnd( lOldEnd );

    return hr;
}   /* CTextRun::Speak */

/**********************************************************************
* CTextRun::Serialize *
*---------------------*
*	Description:  
*		Serializes the text for this run.
*       Simply writes a small header that contains the start
*       and end indices of this run and indicates that
*       no phrase blob follows.
*   Return:
*       S_OK
*       E_INVALIDARG
*       E_UNEXPECTED
*       Return value of ISequentialStream::Write()
**********************************************************************/
HRESULT CTextRun::Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt )
{   
    if ( !pStream )
    {
        return E_INVALIDARG;
    }
    _ASSERTE( m_cpTextRange );
    if ( !m_cpTextRange )
    {
        return E_UNEXPECTED;
    }

    RUNHEADER runHdr;
    runHdr.lStart = GetStart();
    runHdr.lEnd = GetEnd();
    runHdr.bResultFollows = false;

    // Write the header to the stream
    ULONG cbWritten;
    HRESULT hr = pStream->Write( &runHdr, sizeof( RUNHEADER ), &cbWritten );

    return hr;
} /* CTextRun::Serialize */

/**********************************************************************
* CTextRun::IsConsumeLeadingSpaces *
*----------------------------------*
*	Description:  
*       Sets *pfConsumeLeadingSpaces to true iff leading spaces before
*       lPos would need to be consumed.
*       For a text run this value will always be false, since 
*       typed text has no "display attributes"
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
**********************************************************************/
HRESULT CTextRun::IsConsumeLeadingSpaces( const long lPos, 
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
    
    *pfConsumeLeadingSpaces = false;

    return S_OK;
}   /* CTextRun::IsConsumeLeadingSpaces */

/**********************************************************************
* CTextRun::HowManySpacesAfter *
*------------------------------*
*	Description:  
*       Returns the number of spaces that would need to precede text
*       if text were to be inserted at position lPos.
*       This number is in the out param puiSpaces.
*       For a text run this value will always be zero, since 
*       typed text has no "display attributes"
*   Return:
*       S_OK
*       E_POINTER
*       E_INVALIDARG if lPos is out of range
**********************************************************************/
HRESULT CTextRun::HowManySpacesAfter( const long lPos, UINT *puiSpaces )
{
    if ( !puiSpaces )
    {
        return E_POINTER;
    }
    if ( !WithinRange( lPos ) )
    {
        return E_INVALIDARG;
    }
    
    *puiSpaces = 0;

    return S_OK;
}   /* CTextRun::HowManySpacesAfter */

/**********************************************************************
* CTextRun::GetStart *
*--------------------*
*	Description:  
*		Returns the start of the associated TextRange, -1 in case 
*       of error.
**********************************************************************/
long CTextRun::GetStart()
{
    if ( !m_cpTextRange )
    {
        return -1;
    }

    long lRet;
    HRESULT hr = m_cpTextRange->GetStart( &lRet );

    return (SUCCEEDED( hr )) ? lRet : -1;
}   /* CTextRun::GetStart */

/**********************************************************************
* CTextRun::SetStart *
*--------------------*
*	Description:  
*		Sets the start of the associated TextRange.
**********************************************************************/
void CTextRun::SetStart( long lStart )
{
    m_cpTextRange->SetStart( lStart );
}   /* CTextRun::SetStart */

/**********************************************************************
* CTextRun::GetEnd *
*------------------*
*	Description:  
*		Returns the end of the associated TextRange, -1 in case 
*       of error.
**********************************************************************/
long CTextRun::GetEnd()
{
    if ( !m_cpTextRange )
    {
        return -1;
    }

    long lRet;
    HRESULT hr = m_cpTextRange->GetEnd( &lRet );

    return (SUCCEEDED( hr )) ? lRet : -1;
}   /* CTextRun::GetEnd */

/**********************************************************************
* CTextRun::SetEnd *
*------------------*
*	Description:  
*		Sets the end of the associated TextRange
**********************************************************************/
void CTextRun::SetEnd( long lEnd )
{
    m_cpTextRange->SetEnd( lEnd );
}   /* CTextRun::SetEnd */


/*********************************************************************
* CTextRun::WithinRange *
*-----------------------*
*   Description:
*       Determines whether lCursorPos falls within the range of
*       this CTextRun.
*   Return:
*       true iff RunStart <= lCursorPos < RunEnd
**********************************************************************/
bool CTextRun::WithinRange( long lCursorPos )
{
    if ( !m_cpTextRange )
    {
        return false;
    }

    long    lStart= -1;
    long    lEnd= -1;
    
    HRESULT hr = m_cpTextRange->GetStart( &lStart );
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpTextRange->GetEnd( &lEnd );
    }

    return (SUCCEEDED( hr ) && ( lCursorPos >= lStart ) && ( lCursorPos <= lEnd ));
}   /* CTextRun::WithinRange */

/*********************************************************************
* CTextRun::IsDegenerate *
*------------------------*
*   Description:
*       Determines whether "this" has a degenerate range associated
*       with it.
*   Return:
*       true iff start == end
**********************************************************************/
bool CTextRun::IsDegenerate()
{
    return ( GetStart() == GetEnd() );
}   /* CTextRun::IsDegenerate */