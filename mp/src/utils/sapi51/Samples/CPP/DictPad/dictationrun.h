/******************************************************************************
*   DictationRun.h 
*       This module contains the definition of CDictationRun.  CDictationRun
*       keeps track of the dictation-specific items that go with runs of test 
*       that were dictated to DictPad.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#pragma once

#include "TextRun.h"
#include "phrasereplace.h"
#include "resultcontainer.h"

// Forward definition
class CResultContainer;

// Header for serialized CDictationRuns
typedef struct DICTHEADER
{
    ULONG   cbSize;             // Size of the reco result to follow
    ULONG   ulStartElement;         
    ULONG   cElements;
}   DICTHEADER;

/******************************************************************************
* CDictationRun *
*---------------*
*   Description:
*       This class inherits from CTextRun (in textrun.h).  It handles
*       a consecutive run of text in the document that was dictated.
*       It can be a full dictated phrase or part of a phrase that 
*       was dictated, but all of the text in a CDictationRun exists
*       exactly as it had originally been dictated.
********************************************************************************/
class CDictationRun : public CTextRun
{
    public:
        CDictationRun();
        virtual ~CDictationRun();

        // Initialization methods
        HRESULT Initialize( ISpRecoResult &rRecoResult, DICTHEADER *pDictHdr = NULL );
        HRESULT Initialize( CResultContainer &rResultContainer );
        HRESULT SetTextRange( ITextRange *pTextRange );

        // Phrase element-related methods
        HRESULT Split( long *plFirstEnd, long *plSecondBegin,
                        ITextDocument *cpTextDoc, CTextRun **ppTextRun );
        MERGERESULT Concatenate( CTextRun *pTextRun, bool fConcatAfter );
        HRESULT CorrectPhraseEltsAndRange( bool fForward, bool *pfCorrectionResult = NULL );
                                                // Adjust the phrase element members
                                                // and the range so that they are 
                                                // consistent

        // Alternates methods
        HRESULT GetAlternatesText( ITextRange *pRangeForAlts, 
                                    ULONG ulRequestCount,
                                    long *plAltStart,
                                    long *plAltEnd, 
                                    WCHAR **ppszCoMemText,
                                    bool *apfFitsInRun,
                                    ULONG *pcPhrasesReturned );
                                                // Get the text of the alternates
                                                // for the closest range possible
        HRESULT GetAltEndpoints( ULONG ulAlt,
                                    long *plReplaceStart,
                                    long *plReplaceEnd );
                                                // Get the start and end of the 
                                                // range that would be replaced by
                                                // alternate ulAlt from the last time
                                                // GetAlternatesText() was called
        HRESULT ChooseAlternate( ULONG ulAlt );
                                                // Choose alternate number ulAlt
                                                // from the last time GetAlternatesText()
                                                // was called
        void DoneWithAlternates() { m_fAltsGotten = false; }
                                                // Called when the last set of alternates
                                                // obtained is no longer needed
        HRESULT OnAlternateCommit( ULONG ulFirstEltToBeReplaced, ULONG cEltsToBeReplaced,
                                ULONG cElementsInPhraseAfterCommit );
                                                // Called right before an alternate
                                                // is committed somewhere in the 
                                                // associated result object's phrase
        
        
        // Playback methods
        HRESULT Speak( ISpVoice &rVoice );      // Speak the entire block
        HRESULT Speak( ISpVoice &rVoice, long *plStart, long *plEnd );
                                                // Speak part of the block

        // Serialization method
        HRESULT Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt );  
                                                // Write the phrase blob to a stream
        
        // Display attributes methods
        HRESULT IsConsumeLeadingSpaces( const long lPos, bool *pfConsumeLeadingSpaces );
                                                // Do leading spaces need to be
                                                // consumed at lPos?
        HRESULT HowManySpacesAfter( const long lPos, UINT *puiSpaces );
                                                // How many spaces would need to be
                                                // at lPos if new text were inserted there?

        bool IsDict() { return true; };
        


    private:
        HRESULT FindNearestWholeElementRange( long lStart,
                                        long lEnd,
                                        long *plResultStart,
                                        long *plResultEnd,
                                        ULONG *pulStartElement,
                                        ULONG *pcElements );
                                                // Finds the nearest range containing
                                                // pRange that contains entire elements

    // data members
    private:

        ULONG m_ulStartElement;   // Where in the RecoResult this dictation run starts
        ULONG m_cElements;        // How many elements are in this run

        bool m_fAltsGotten;       // Whether alternates have been requested for some range
                                  // of elements
        ULONG m_ulStartAltElt;    // These two members indicate the range of elements
        ULONG m_cAltElt;          // for which alternates were last requested

        CPhraseReplacement      *m_pPhraseReplacement;  // Phrase replacement (ITN) info
        ULONG                   *m_pulElementOffsets;   // Where each element starts 
                                                        // relative to the start of the range
        CResultContainer        *m_pResultContainer;    // Holds the result object
};


// Helper functions

bool ContainsNonWhitespace( const WCHAR *pwsz );// Return true iff some
                                                // character in the string
                                                // is not whitespace

