/****************************************************************************
*   textrun.h
*       This module contains the definition support for CTextRun which is
*       our base class for holding onto all the text with DictPad.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/
#pragma once

// Enumerated return type for CTextRun merging functions
typedef enum MERGERESULT
{
    E_NOMERGE,          // No merging could be done at all
    E_PARTIALMERGE,     // Part but not all of the range could be merged in
    E_LESSTEXT,         // Some of the range had to be given up
    E_FULLMERGE         // All of the range was merged
};

// Header for serialized CTextRuns
typedef struct RUNHEADER
{
    bool bResultFollows;        // true iff this is a dictation run with the
                                // associated reco result following in the stream
    long lStart;                // Start index of the associated text
    long lEnd;                  // End index of the associated text
}   RUNHEADER;

/******************************************************************************
* CTextRun *
*----------*
*   Description:
*       This class handles a consecutive run of text in the document.
********************************************************************************/
class CTextRun
{
    public:
        // Constructor / destructor
        CTextRun();
        virtual ~CTextRun();

        // Refcounting methods
        DWORD IncrementCount();
        DWORD DecrementCount();

        // Initialization method
        virtual HRESULT SetTextRange( ITextRange *pTextRange );

        // Phrase element-related methods
        virtual HRESULT Split( long *plFirstEnd, long *plSecondBegin,
                                ITextDocument *cpTextDoc, CTextRun **ppTextRun );
        virtual MERGERESULT Concatenate( CTextRun *pTextRun, bool bConcatAfter );
        virtual HRESULT CorrectPhraseEltsAndRange( bool bForward, bool *pfCorrectionResult = NULL ) 
        { 
            if ( pfCorrectionResult ) 
            {
                *pfCorrectionResult = true;
            }
            return S_OK; 
        };

        // Playback methods
        virtual HRESULT Speak( ISpVoice &rVoice );  // Speak a whole block
        virtual HRESULT Speak( ISpVoice &rVoice, long *plStart, long *plEnd );
                                                    // Speak part of a block

        // Serialization method
        virtual HRESULT Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt = NULL );
                                                    // Serialize and write to stream
        
        // Display attributes methods
        virtual HRESULT IsConsumeLeadingSpaces( const long lPos, bool *pfConsumeLeadingSpaces );
        virtual HRESULT HowManySpacesAfter( const long lPos, UINT *puiSpaces );

        virtual bool IsDict() { return FALSE; };

        // Endpoints-related methods
        long GetStart();
        void SetStart( long lStart );
        long GetEnd();
        void SetEnd( long lEnd );
        bool WithinRange( long lCursorPos );
        bool IsDegenerate();

    protected:
        CComPtr<ITextRange> m_cpTextRange;

    private:
        DWORD               m_dwRefCount;
};  /* class CTextRun */

