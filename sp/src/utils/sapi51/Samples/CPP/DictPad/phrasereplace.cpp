/******************************************************************************
*	phrasereplace.cpp 
*       Implementation details for the CPhraseReplacement object which 
*       does the bookkeeping to translate between a phrase as 
*       displayed with ITN replacements and a phrase as seen
*       by the associated RecoResult.
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#include "stdafx.h"
#include "phrasereplace.h"

/*****************************************************************************
* CPhraseReplacement::CPhraseReplacement *
*----------------------------------------*
*   Description:
*       Constructor for the CPhraseReplacement class
*******************************************************************************/
CPhraseReplacement::CPhraseReplacement() :  m_fUseMaps( false ),
                                            m_fSuccessfulSetup( false ),
                                            m_pPhrase( NULL ),
                                            m_cReplacementElements( 0 ),
                                            m_pulPreToPostTable( NULL ),
                                            m_pulPostToPreTable( NULL ),
                                            m_pulPostToReplacementTable( NULL )
{}  /* CPhraseReplacement::CPhraseReplacement */

/*****************************************************************************
* CPhraseReplacement::~CPhraseReplacement *
*-----------------------------------------*
*   Description:
*       Destructor for the CPhraseReplacement class
*       Deletes the tables and frees the SPPHRASE.
*******************************************************************************/
CPhraseReplacement::~CPhraseReplacement()
{
    // If memory was allocated for these, then free
    delete[] m_pulPreToPostTable;
    delete[] m_pulPostToPreTable;
    delete[] m_pulPostToReplacementTable;

    if ( m_pPhrase )
    {
        // The SPPHRASE was CoTaskMemAlloc()ed by CPhraseReplacement::Initialize()
        // so it needs to be freed
        ::CoTaskMemFree( m_pPhrase );
    }
}   /* CPhraseRepalcement::~CPhraseReplacement */


/*****************************************************************************
* CPhraseReplacement::Initialize *
*--------------------------------*
*   Description:
*       Gets the phrase from the ISpPhrase object and calls SetUpMaps().
*       This initialization routine can be called either from a
*       client with a new CPhraseReplacement object or  
*       when an alternate is committed (or the recoresult has
*       otherwise changed).
*   Return:
*       Return value of ISpRecoResult::GetPhrase()
*       Return value of CPhraseReplacement::SetUpMaps()
*******************************************************************************/
HRESULT CPhraseReplacement::Initialize( ISpPhrase &rPhrase )
{
    // Get (or re-get) the phrase
    if ( m_pPhrase )
    {
        ::CoTaskMemFree( m_pPhrase );
    }
    HRESULT hr = rPhrase.GetPhrase( &m_pPhrase );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = SetUpMaps();
    m_fSuccessfulSetup = SUCCEEDED( hr );
    m_fUseMaps = (S_OK == hr);

    return hr;
}   /* CPhraseReplacement::Initialize */

/*****************************************************************************
* CPhraseReplacement::SetUpMaps *
*-------------------------------*
*   Description:
*       Initializes the various maps in the CPhraseReplacement object.
*       Called from within the initialization routines.
*   Return:
*       S_OK
*       S_FALSE if no maps necessary
*       E_OUTOFMEMORY
*******************************************************************************/
HRESULT CPhraseReplacement::SetUpMaps()
{
    _ASSERTE( m_pPhrase );
    if ( !m_pPhrase )
    {
        return E_UNEXPECTED;
    }

    if ( !m_pPhrase->cReplacements )
    {
        // No phrase replacements; nothing needs to be done for this phrase
        m_cReplacementElements = m_pPhrase->Rule.ulCountOfElements;
        return S_FALSE;
    }

    // Temporary place to keep replacement info:
    // This array will indicate which elements in the original (non-replaced)
    // phrase start off a replacement.
    // For instance, in the phrase "I have three dollars", pulPreToReplacementTable[2]
    // would indicate that "three" starts off a replacement; there is one replacement
    // ($3), so pulPreToReplacementTable[2] would have the value 0.
    ULONG *pulPreToReplacementTable = 
        new ULONG[ m_pPhrase->Rule.ulCountOfElements ];
    if ( !pulPreToReplacementTable )
    {
        return E_OUTOFMEMORY;
    }

    // Set the value to that will indicate that no replacement starts with this element: 
    // Since there are m_pPhrase->cReplacements replacements, 
    // no replacement is going to have the index m_pPhrase->cReplacements
    m_ulNoReplacementValue = m_pPhrase->cReplacements;

    // Initialize this array to indicate that no elements kick off replacements 
    for ( ULONG ul = 0; ul < m_pPhrase->Rule.ulCountOfElements; ul++ )
    {
        pulPreToReplacementTable[ul] = m_ulNoReplacementValue;
    }

    // Fill in the temporary array that maps pre-replacement elements to the 
    // replacements they start (if any).
    // Count up the number of elements that get replaced.
    ULONG cReplacedElements = 0;
    for ( ULONG ulReplacement = 0; 
        ulReplacement < m_pPhrase->cReplacements;
        ulReplacement++ )
    {
        // Record the replacement's index in the ulFirstElement'th position in the
        // temporary array
        pulPreToReplacementTable[(m_pPhrase->pReplacements[ulReplacement]).ulFirstElement] 
            = ulReplacement;
        
        // Bump the number of replaced elements
        cReplacedElements += 
            (m_pPhrase->pReplacements[ulReplacement]).ulCountOfElements;
    }
  
    // Calculate how many phrase elements there will be after replacement.
    // Note that each replacement will be counted as only one element ("$3" is 
    // one element, as is "June 28, 1977")
    m_cReplacementElements = m_pPhrase->Rule.ulCountOfElements 
        + m_pPhrase->cReplacements - cReplacedElements ;

    // Allocate the appropriate amount of space for the tables.  
    // Note that a CPhraseReplace object may be initialized more than once
    // in its lifetime, so we may need to free up memory previously
    // allocated for these tables.
    delete[] m_pulPreToPostTable;
    delete[] m_pulPostToPreTable;
    delete[] m_pulPostToReplacementTable;

    // This table maps from pre-replacement elements to post-replacement elements
    // There are ulCountOfElements elements in the pre-replacement phrase
    m_pulPreToPostTable = new ULONG[ m_pPhrase->Rule.ulCountOfElements ];
    
    // This table maps from post-replacement elements to pre-replacement elements
    // There are m_cReplacementElements elements in the post-replacement phrase
    m_pulPostToPreTable = new ULONG[ m_cReplacementElements ];

    // This table maps from post-replacement elements to the replacement (if any)
    // to which they correspond.
    m_pulPostToReplacementTable = new ULONG[ m_cReplacementElements];

    // Check to make sure we haven't run into any memory issues
    if ( !m_pulPreToPostTable || !m_pulPostToPreTable || !m_pulPostToReplacementTable )
    {
        delete[] pulPreToReplacementTable;

        return E_OUTOFMEMORY;
    }

    // Fill in the tables.
    // There will be two counters
    // ulPreReplElement which walks through the elements of the original phrase;
    // ulPostReplElement which walks through the elements of the post-replacement phrase.
    // Usually with replacements, a post-replacement elements ($3) corresponds to more
    // than one pre-replacement element ("three dollars")
    ULONG ulPreReplElement = 0;
    for ( ULONG ulPostReplElement = 0; 
        ulPostReplElement < m_cReplacementElements; 
        ulPostReplElement++ )
    {
        // Right now, ulPreReplElement and upPostReplElement are referring to the 
        // same element (though they may have different values, of course)
        m_pulPostToPreTable[ulPostReplElement] = ulPreReplElement;
        m_pulPostToReplacementTable[ulPostReplElement] = 
            pulPreToReplacementTable[ulPreReplElement];

        if ( m_pulPostToReplacementTable[ulPostReplElement] < m_ulNoReplacementValue )
        {
            // This is a replaced element; so several of the pre-replacement 
            // elements will correspond to this post-replacement element.
            
            // Use the element-to-replacement info to determine which one, so that
            // we know how many pre-replacement elements to grab.
            const SPPHRASEREPLACEMENT *pRepl = m_pPhrase->pReplacements + 
                m_pulPostToReplacementTable[ulPostReplElement];
            
            // For each element covered by the replacement, make another pre-replacement
            // element map to this post-replacement element and increment
            // ulPreReplElement.
            // (i.e. both "three" and "dollars" would map to "$3")
            for ( ULONG ul = 0; ul < pRepl->ulCountOfElements; ul++, ulPreReplElement++ )
            {
                _ASSERTE( ulPreReplElement < m_pPhrase->Rule.ulCountOfElements );
                m_pulPreToPostTable[ulPreReplElement] = ulPostReplElement;
            }
        }
        else
        {
            // This is not a replaced element, so there is a 1-1 relationship
            // between it and the ulPreReplElement'th pre-replacement element
            _ASSERTE( ulPreReplElement < m_pPhrase->Rule.ulCountOfElements );
            m_pulPreToPostTable[ulPreReplElement++] = ulPostReplElement;
        }

    }

    // Free up temporary array
    delete[] pulPreToReplacementTable;

    return S_OK;
}   /* CPhraseReplacement::SetUpMaps */

/********************************************************************************
* CPhraseReplacement::GetNumNoReplacementElements *
*-------------------------------------------------*
*   Description:
*       Gets the number of elements as seen from the recoresult's point of view
*       (i.e., without replacement)
**********************************************************************************/
ULONG CPhraseReplacement::GetNumNoReplacementElements()
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return 0;
    }

    return m_pPhrase->Rule.ulCountOfElements;
}   /* CPhraseReplacement::GetNumReplacementElements */


/********************************************************************************
* CPhraseReplacement::GetNumReplacementElements *
*-----------------------------------------------*
*   Description:
*       Gets the number of elements as seen from the caller's point of view
*       (i.e., with replacement)
**********************************************************************************/
ULONG CPhraseReplacement::GetNumReplacementElements()
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return 0;
    }

    if ( m_fUseMaps )
    {
        // There are replacements
        return m_cReplacementElements;
    }
    else
    {
        // There are no replacements
        return m_pPhrase->Rule.ulCountOfElements;
    }
}   /* CPhraseReplacement::GetNumReplacementElements */

/********************************************************************************
* CPhraseReplacement::GetDisplayText *
*------------------------------------*
*   Description:
*       Hands back the text associated with the given phrase "element"
*       If bUseReplacedElements is false or this phrase has no replacements,
*       simply returns the display text for the appropriate element.
*       Otherwise, consults the element mappings and returns  the 
*       replacement text or the display text for the ulElement'th pre-
*       replacement element, as appropriate.
*   Return: 
*       Pointer to a text buffer containing the display text
*       NULL if failed
**********************************************************************************/
const WCHAR * CPhraseReplacement::GetDisplayText( ULONG ulElement, 
                                                 BYTE *pbDisplayAttributes,
                                                 bool bUseReplacedElements )
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return NULL;
    }

    const WCHAR *pwszRetText = NULL;
    BYTE bDisplayAttributes = 0;

    if ( !m_fUseMaps || !bUseReplacedElements )
    {
        // Do not map; just hand back the display text for the requested
        // element
        if ( ulElement < m_pPhrase->Rule.ulCountOfElements )
        {
            bDisplayAttributes = 
                m_pPhrase->pElements[ulElement].bDisplayAttributes;
            pwszRetText = m_pPhrase->pElements[ulElement].pszDisplayText;
        }
    }
    else
    {
        // Caller is using a post-replacement index and wants the text from that
        // replacement
        if ( ulElement < m_cReplacementElements )
        {
            if ( m_pulPostToReplacementTable[ulElement] < m_pPhrase->cReplacements )
            {
                // This is a replaced element; hand back the replacement text
                // and attributes
                const SPPHRASEREPLACEMENT *pRepl = m_pPhrase->pReplacements + 
                    m_pulPostToReplacementTable[ulElement];
                pwszRetText = pRepl->pszReplacementText;
                bDisplayAttributes = pRepl->bDisplayAttributes;
            }
            else
            {
                // This element has not been replaced; hand back the text
                // and attributes corresponding to the appropriate 
                // pre-replacement element
                ULONG ulPreReplElement = m_pulPostToPreTable[ulElement];
                pwszRetText = m_pPhrase->pElements[ulPreReplElement].pszDisplayText;
                bDisplayAttributes = 
                    m_pPhrase->pElements[ulPreReplElement].bDisplayAttributes;
            }
        }
    }

    if ( pbDisplayAttributes )
    {
        *pbDisplayAttributes = bDisplayAttributes;
    }

    // In case of error, this remains NULL
    return pwszRetText;

}   /* CPhraseReplacement::GetDisplayText */

/********************************************************************************
* CPhraseReplacement::PreToPostReplacementIndex *
*-----------------------------------------------*
*   Description:
*       Converts from a pre-replacement index to a with-replacement index.
*       If ulSPPHRASEIndex is equal to the number of (pre-replacement) elements
*       will return the number of post-replacement elements in 
*       *pulReplacedPhraseIndex
*   Return: 
*       S_OK
*       E_POINTER
*       E_FAIL
*       E_INVALIDARG if ulSPPHRASEIndex is out-of-bounds
**********************************************************************************/
HRESULT CPhraseReplacement::PreToPostReplacementIndex( ULONG ulSPPHRASEIndex,
                                                    ULONG *pulReplacedPhraseIndex)
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return E_FAIL;
    }

    if ( !pulReplacedPhraseIndex )
    {
        return E_POINTER;
    }
    if ( ulSPPHRASEIndex > m_pPhrase->Rule.ulCountOfElements )
    {
        // Out of bounds index
        return E_INVALIDARG;
    }

    if ( ulSPPHRASEIndex == m_pPhrase->Rule.ulCountOfElements )
    {
        *pulReplacedPhraseIndex = m_fUseMaps ? 
            m_cReplacementElements : m_pPhrase->Rule.ulCountOfElements;
    }
    else
    {
        // If the maps are valid, do a lookup; 
        // otherwise just return the original index
        *pulReplacedPhraseIndex = m_fUseMaps ? 
            m_pulPreToPostTable[ulSPPHRASEIndex] : ulSPPHRASEIndex;
    }

    return S_OK;
}   /* CPhraseReplacement::PreToPostReplacementIndex */

/********************************************************************************
* CPhraseReplacement::PostToPreReplacementIndex *
*-----------------------------------------------*
*   Description:
*       Converts from a with-replacement index to a pre-replacement index
*       If ulReplacedPhraseIndex is equal to the number of (post-replacement) 
*       elements, will return the number of pre-replacement elements in 
*       *pulSPPHRASEIndex
*   Return: 
*       S_OK
*       E_POINTER
*       E_FAIL
*       E_INVALIDARG if ulReplacedPhraseIndex is out-of-bounds
**********************************************************************************/
HRESULT CPhraseReplacement::PostToPreReplacementIndex( ULONG ulReplacedPhraseIndex,
                                                      ULONG *pulSPPHRASEIndex )
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return E_FAIL;
    }

    if ( !pulSPPHRASEIndex )
    {
        return E_POINTER;
    }
    if ( ulReplacedPhraseIndex > m_cReplacementElements )
    {
        // Out of bounds index
        return E_INVALIDARG;
    }

    if ( ulReplacedPhraseIndex == m_cReplacementElements )
    {
        *pulSPPHRASEIndex = m_pPhrase->Rule.ulCountOfElements;
    }
    else
    {
        // If the maps are valid, do a lookup; 
        //otherwise just return the original index
        *pulSPPHRASEIndex = m_fUseMaps ? 
            m_pulPostToPreTable[ulReplacedPhraseIndex] : ulReplacedPhraseIndex;
    }

    return S_OK;
}   /* CPhraseReplacement::PostToPreReplacementIndex */
    
/********************************************************************************
* CPhraseReplacement::ExpandToIncludeWholeReplacements *
*------------------------------------------------------*
*   Description:
*       Takes pre-replacement indices in the in/out params pulPreReplStart and
*       pcPreReplElts and expands them, if necessary, to contain entire 
*       replacements.  
*       Example: "Thirty nine is the first uninteresting number."
*       If *pulPreReplStart = 1 and *pcPreReplElts = 4, then when this
*       function returns *pulPreReplStart = 0 and *pcPreReplElts = 5 (in order
*       to include "thirty" at the beginning)
*   Return: 
*       S_OK
*       E_POINTER
*       E_INVALIDARG if *pulPreReplStart and *pcPreReplElts indicate an
*           out-of-bounds range or if *pcPreReplElts is 0
**********************************************************************************/
HRESULT CPhraseReplacement::ExpandToIncludeWholeReplacements( ULONG *pulPreReplStart,
                                                             ULONG *pcPreReplElts )
{
    if ( !m_fSuccessfulSetup )
    {
        _ASSERTE( false );
        return E_FAIL;
    }

    if ( !pulPreReplStart || !pcPreReplElts )
    {
        return E_POINTER;
    }

    if ( !m_fUseMaps )
    {
        // Don't need to do anything since there are no maps
        return S_OK;
    }

    // Validate params
    if ( (*pulPreReplStart >= GetNumNoReplacementElements()) || 
        (0 == *pcPreReplElts) ||
        ((*pulPreReplStart + *pcPreReplElts) > GetNumNoReplacementElements()) )
    {
        // Out-of-bounds range or degenerate element range
        return E_INVALIDARG;
    }

    // Convert the start to a post-replacement value and see if that
    // element has any replacement associated with it
    ULONG ulPostReplStart;
    HRESULT hr = PreToPostReplacementIndex( *pulPreReplStart, &ulPostReplStart );
    if ( SUCCEEDED( hr ) )
    {
        ULONG ulReplacement = m_pulPostToReplacementTable[ ulPostReplStart ];
        if ( ulReplacement < m_ulNoReplacementValue )
        {
            // There is an associated replacement
            // Move back *pulPreReplStart to the beginning of what that replacement replaces
            const SPPHRASEREPLACEMENT *pRepl = m_pPhrase->pReplacements + ulReplacement;
            *pulPreReplStart = pRepl->ulFirstElement;
        }
    }
    else
    {
        return hr;
    }

    // Do the same for the last element
    ULONG ulPreReplLast = *pulPreReplStart + *pcPreReplElts - 1;
    ULONG ulPostReplLast;
    hr = PreToPostReplacementIndex( ulPreReplLast, &ulPostReplLast );
    if ( SUCCEEDED( hr ) )
    {
        ULONG ulReplacement = m_pulPostToReplacementTable[ ulPostReplLast ];
        if ( ulReplacement < m_ulNoReplacementValue )
        {
            // There is an associated replacement
            // Advance ulPreReplLast to the last element that the replacement replaces
            const SPPHRASEREPLACEMENT *pRepl = m_pPhrase->pReplacements + ulReplacement;
            ulPreReplLast = pRepl->ulFirstElement + pRepl->ulCountOfElements - 1;
        }
    }
    else
    {
        return hr;
    }

    // Adjust the count of elements
    *pcPreReplElts = ulPreReplLast - *pulPreReplStart + 1;
    
    return S_OK;
}   /* CPhraseReplacement::ExpandToIncludeWholeReplacements */
