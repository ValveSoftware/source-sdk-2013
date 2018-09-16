/****************************************************************************
*   phrasereplace.h
*       This module contains the definition support for CPhraseReplacement,
*       which does the bookkeeping to translate between a phrase as 
*       displayed with ITN replacements and a phrase as thought of
*       by the associated RecoResult
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/
#pragma once

#ifndef __PHRASEREPLACE_H
#define __PHRASEREPLACE_H

class CPhraseReplacement
{
public:
    CPhraseReplacement();
    ~CPhraseReplacement();
    
    HRESULT Initialize( ISpPhrase &rSpPhrase );

    ULONG GetNumNoReplacementElements();
    ULONG GetNumReplacementElements();

    // Gets the text of a given element
    const WCHAR * GetDisplayText( ULONG ulElement, 
        BYTE *pbDisplayAttributes = NULL,
        bool bUseReplacedElements = true );

    // Conversion methods
    HRESULT PreToPostReplacementIndex( ULONG ulSPPHRASEIndex, 
                                        ULONG *pulReplacePhraseIndex );
    HRESULT PostToPreReplacementIndex( ULONG ulReplacedPhraseIndex,
                                        ULONG *pulSPPHRASEIndex );
    HRESULT ExpandToIncludeWholeReplacements( ULONG *pulPreReplStart,
                                        ULONG *pcPreReplElts );

private:
    HRESULT SetUpMaps();                                // Does the real initialization work

    bool                m_fUseMaps;                     // Indicates whether to use the maps
    bool                m_fSuccessfulSetup;             // Indicates the this object is usable

    SPPHRASE            *m_pPhrase;
    ULONG               m_cReplacementElements;         // Number of elements after 
                                                        // replacements have been made
    ULONG               *m_pulPreToPostTable;           // Given the index of an element in the phrase,
                                                        // contains the index of that element with 
                                                        // replacements
    ULONG               *m_pulPostToPreTable;           // Given the index of an element with replacements,
                                                        // contains the index of that element in the 
                                                        // original phrase
    ULONG               *m_pulPostToReplacementTable;   // Given the index of an element with replacements,
                                                        // indicates which replacement (if any) that
                                                        // element corresponds to
    ULONG               m_ulNoReplacementValue;         // Value in tables to indicate that no 
                                                        // replacement corresponds to this element.
};  // class CPhraseReplacement

#endif // __PHRASEREPLACE_H