/******************************************************************************
*   TextRunList.h 
*       This module contains the definition details of CTextRunList which is 
*       a container class that keeps track of all of the individual text runs
*       in DictPad
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#pragma once

#include "TextRun.h"
#include "DictationRun.h"

// Nodes for the CTextRunList
typedef struct TEXTRUNNODE
{
    CTextRun *pTextRun;         // Points to the actual test run information
    struct TEXTRUNNODE *pPrev;  // Points to the previous node in our list
    struct TEXTRUNNODE *pNext;  // Points to the next node in our list
} TEXTRUNNODE, *PTEXTRUNNODE;

/******************************************************************************
*   CTextRunList
*       This class keeps track of all the CTextRun objects that exist to 
*       represent the entire document being edited within DictPad
******************************************************************************/
class CTextRunList
{
    public:
        // Constructor/destructor
        CTextRunList( ITextDocument *pTextDoc = NULL);
        ~CTextRunList();

        // Initialization methods
        void SetTextDoc( ITextDocument *cpTextDoc ) { m_cpTextDoc = cpTextDoc; }
        HRESULT CreateSimpleList();             // Create a one-node list consisting of a single TextRun

        // Method for new text
        HRESULT Insert( CTextRun *pTextRun );   // Takes care of figuring out where the new node should go

        // Playback method
        HRESULT Speak( ISpVoice &rVoice, long *plStartSpeaking, long *plEndSpeaking );       
                                                // Speak starting and ending at specified locations

        // Serialization methods
        HRESULT Serialize( IStream *pStream, ISpRecoContext *pRecoCtxt );
                                                // Write the contents of the TextRunList to pStream
        HRESULT Deserialize( IStream *pStream, ISpRecoContext *pRecoCtxt );
                                                // Recreate the TextRunList from the stream

        // Display attributes methods
        HRESULT IsConsumeLeadingSpaces( long lPos, bool *pfConsumeLeadingSpaces );
        HRESULT HowManySpacesAfter( long lPos, UINT *puiSpaces );   
                                                // How many spaces would need to precede text 
                                                // that would start at lPos

        // Methods that deal with positions in the text 
        long GetTailEnd();                      // length of the document
        PTEXTRUNNODE Find( long lDest );        // Pointer to the node that contains this position


    private:
        // Methods for manipulation of adjacent CTextRuns
        HRESULT MoveCurrentTo( LONG lDest );        // Move m_pCurrent to a node that ends at 
                                                    // lDest, splitting nodes if necessary
        HRESULT SplitNode( PTEXTRUNNODE pNode );    // Split the given node for m_pNodeToInsert
        HRESULT MergeIn( PTEXTRUNNODE pNode, bool *pfNodeMadeItIn );
                                                    // Try to merge a new node in with neighbors
        HRESULT MergeInDictRun( PTEXTRUNNODE pNode );
                                                    // Handle newly-dictated text in the list

        // List manipulation methods
        HRESULT InsertAfter( PTEXTRUNNODE pCurrent, PTEXTRUNNODE pNodeToInsert );
        HRESULT RemoveNode( PTEXTRUNNODE pNode );
        HRESULT AddHead( PTEXTRUNNODE pHead );
        HRESULT AddTail( PTEXTRUNNODE pTail );
        void RemoveHead();
        void RemoveTail();

        // Clean-up method
        void DeleteAllNodes();                  

    private:
        // These cached nodes help us to add new nodes to our list 
        // somewhat optimally.
        PTEXTRUNNODE m_pHead;       // Starting point (necessary reguardless of strategy)
        PTEXTRUNNODE m_pTail;       // Last node in our list
        PTEXTRUNNODE m_pCurrent;    // Most recently added node.  We cache this one under the
                                    // assumption that nodes will often be added in groups;
                                    // remembering where the last one went is a good clue 
                                    // as to where we should insert the next one.

        PTEXTRUNNODE m_pNodeToInsert;    
                                    // Node that we are about to add.  This information is necessary for 
                                    // calling CTextRun::Split()

        CComPtr<ITextDocument> m_cpTextDoc;     // Pointer to the ITextDocument interface to which
                                                // we will need access for splitting nodes in order
                                                // to retrieve ITextRange pointers.
};  /* class CTextRunList */

