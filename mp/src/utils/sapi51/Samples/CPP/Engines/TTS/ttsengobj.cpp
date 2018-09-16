/*******************************************************************************
* TtsEngObj.cpp *
*---------------*
*   Description:
*       This module is the main implementation file for the CTTSEngObj class.
*-------------------------------------------------------------------------------
*  Creation Date: 03/24/99
*  Copyright (c) Microsoft Corporation. All rights reserved.
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "TtsEngObj.h"

//--- Local

/*****************************************************************************
* CTTSEngObj::FinalConstruct *
*----------------------------*
*   Description:
*       Constructor
*****************************************************************************/
HRESULT CTTSEngObj::FinalConstruct()
{
    SPDBG_FUNC( "CTTSEngObj::FinalConstruct" );
    HRESULT hr = S_OK;

    //--- Init vars
    m_hVoiceData = NULL;
    m_pVoiceData = NULL;
    m_pWordList  = NULL;
    m_ulNumWords = 0;

    return hr;
} /* CTTSEngObj::FinalConstruct */

/*****************************************************************************
* CTTSEngObj::FinalRelease *
*--------------------------*
*   Description:
*       destructor
*****************************************************************************/
void CTTSEngObj::FinalRelease()
{
    SPDBG_FUNC( "CTTSEngObj::FinalRelease" );

    delete m_pWordList;

    if( m_pVoiceData )
    {
        ::UnmapViewOfFile( (void*)m_pVoiceData );
    }

    if( m_hVoiceData )
    {
        ::CloseHandle( m_hVoiceData );
    }

} /* CTTSEngObj::FinalRelease */

/*****************************************************************************
* CTTSEngObj::MapFile *
*---------------------*
*   Description:
*       Helper function used by SetObjectToken to map file.  This function
*   assumes that m_cpToken has been initialized.
*****************************************************************************/
HRESULT CTTSEngObj::MapFile( const WCHAR * pszTokenVal,  // Value that contains file path
                            HANDLE * phMapping,          // Pointer to file mapping handle
                            void ** ppvData )            // Pointer to the data
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    CSpDynamicString dstrFilePath;
    hr = m_cpToken->GetStringValue( pszTokenVal, &dstrFilePath );
    if ( SUCCEEDED( hr ) )
    {
        bool fWorked = false;
        *phMapping = NULL;
        *ppvData = NULL;
        HANDLE hFile;
#ifdef _WIN32_WCE
        hFile = CreateFileForMapping( dstrFilePath, GENERIC_READ,
                                      FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, NULL );
#else
        hFile = CreateFile( W2T(dstrFilePath), GENERIC_READ,
                            FILE_SHARE_READ, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL );
#endif
        if (hFile != INVALID_HANDLE_VALUE)
        {
            *phMapping = ::CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
            if (*phMapping)
            {
                *ppvData = ::MapViewOfFile( *phMapping, FILE_MAP_READ, 0, 0, 0 );
                if (*ppvData)
                {
                    fWorked = true;
                }
            }
            ::CloseHandle( hFile );
        }
        if (!fWorked)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            if (*phMapping)
            {
                ::CloseHandle(*phMapping);
                *phMapping = NULL;
            }
        }
    }
    return hr;
} /* CTTSEngObj::MapFile */

//
//=== ISpObjectWithToken Implementation ======================================
//

/*****************************************************************************
* CTTSEngObj::SetObjectToken *
*----------------------------*
*   Description:
*       This function performs the majority of the initialization of the voice.
*   Once the object token has been provided, the filenames are read from the
*   token key and the files are mapped.
*****************************************************************************/
STDMETHODIMP CTTSEngObj::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC( "CTTSEngObj::SetObjectToken" );
    HRESULT hr = SpGenericSetObjectToken(pToken, m_cpToken);

    //--- Map the voice data so it will be shared among all instances
    //  Note: This is a good example of how to memory map and share
    //        your voice data across instances.
    if( SUCCEEDED( hr ) )
    {
        hr = MapFile( L"VoiceData", &m_hVoiceData, &m_pVoiceData );
    }

    //--- Setup word list
    //  Note: This is specific to our example, you probably
    //        don't care much about this logic.
    if( SUCCEEDED( hr ) )
    {
        delete m_pWordList;

        //--- Check version
        UNALIGNED DWORD* pdwPtr = (UNALIGNED DWORD*)m_pVoiceData;
        if( *pdwPtr++ != 1 )
        {
            //--- Bad voice file version
            hr = E_INVALIDARG;
            SPDBG_ASSERT(0);
        }
    
        //--- Get number of words
        if( SUCCEEDED( hr ) )
        {
            m_ulNumWords = *pdwPtr++;
        }

        //--- Build word list
        m_pWordList = new VOICEITEM[m_ulNumWords];
        if( !m_pWordList )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            for( ULONG i = 0; i < m_ulNumWords; ++i )
            {
                ULONG ulTextByteLen = *pdwPtr++;
                m_pWordList[i].pText = (UNALIGNED WCHAR*)pdwPtr;
                m_pWordList[i].ulTextLen = (ulTextByteLen / sizeof(WCHAR)) - 1;
                pdwPtr = (UNALIGNED DWORD*)(((BYTE*)pdwPtr) + ulTextByteLen);
                m_pWordList[i].ulNumAudioBytes = *pdwPtr++;
                m_pWordList[i].pAudio = (BYTE*)pdwPtr;
                pdwPtr = (UNALIGNED DWORD*)(((BYTE*)pdwPtr) + m_pWordList[i].ulNumAudioBytes);
            }
        }
    }

    return hr;
} /* CTTSEngObj::SetObjectToken */

//
//=== ISpTTSEngine Implementation ============================================
//

/*****************************************************************************
* CTTSEngObj::Speak *
*-------------------*
*   Description:
*       This is the primary method that SAPI calls to render text.
*-----------------------------------------------------------------------------
*   Input Parameters
*
*   pUser
*       Pointer to the current user profile object. This object contains
*       information like what languages are being used and this object
*       also gives access to resources like the SAPI master lexicon object.
*
*   dwSpeakFlags
*       This is a set of flags used to control the behavior of the
*       SAPI voice object and the associated engine.
*
*   VoiceFmtIndex
*       Zero based index specifying the output format that should
*       be used during rendering.
*
*   pTextFragList
*       A linked list of text fragments to be rendered. There is
*       one fragement per XML state change. If the input text does
*       not contain any XML markup, there will only be a single fragment.
*
*   pOutputSite
*       The interface back to SAPI where all output audio samples and events are written.
*
*   Return Values
*       S_OK - This should be returned after successful rendering or if
*              rendering was interrupted because *pfContinue changed to FALSE.
*       E_INVALIDARG 
*       E_OUTOFMEMORY
*
*****************************************************************************/
STDMETHODIMP CTTSEngObj::Speak( DWORD dwSpeakFlags,
                                REFGUID rguidFormatId,
                                const WAVEFORMATEX * pWaveFormatEx,
                                const SPVTEXTFRAG* pTextFragList,
                                ISpTTSEngineSite* pOutputSite )
{
    SPDBG_FUNC( "CTTSEngObj::Speak" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_INTERFACE_PTR( pOutputSite ) ||
        SP_IS_BAD_READ_PTR( pTextFragList )  )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Init some vars
        m_pCurrFrag   = pTextFragList;
        m_pNextChar   = m_pCurrFrag->pTextStart;
        m_pEndChar    = m_pNextChar + m_pCurrFrag->ulTextLen;
        m_ullAudioOff = 0;

        //--- Parse
        //    We've supplied a simple word/sentence breaker just to show one
        //    way of walking the fragment list. It obviously doesn't deal with
        //    things like abreviations and expansion of numbers and dates.
        CItemList ItemList;

        while( SUCCEEDED( hr ) && !(pOutputSite->GetActions() & SPVES_ABORT) )
        {
            //--- Do skip?
            if( pOutputSite->GetActions() & SPVES_SKIP )
            {
                long lSkipCnt;
                SPVSKIPTYPE eType;
                hr = pOutputSite->GetSkipInfo( &eType, &lSkipCnt );
                if( SUCCEEDED( hr ) )
                {
                    //--- Notify SAPI how many items we skipped. We're returning zero
                    //    because this feature isn't implemented.
                    hr = pOutputSite->CompleteSkip( 0 );
                }
            }

            //--- Build the text item list
            if( SUCCEEDED( hr ) && (hr = GetNextSentence( ItemList )) != S_OK )
            {
                break;                
            }

            //--- We aren't going to do any part of speech determination,
            //    prosody, or pronunciation determination. If you were, one thing
            //    you will need is access to the SAPI lexicon. You can get that with
            //    the following call.
            //    CComPtr<ISpLexicon> cpLexicon;
            //    hr = pUser->GetLexicon( &cpLexicon );

            if( !(pOutputSite->GetActions() & SPVES_ABORT) )
            {
                //--- Fire begin sentence event
                CSentItem& FirstItem = ItemList.GetHead();
                CSentItem& LastItem  = ItemList.GetTail();
                CSpEvent Event;
                Event.eEventId             = SPEI_SENTENCE_BOUNDARY;
                Event.elParamType          = SPET_LPARAM_IS_UNDEFINED;
                Event.ullAudioStreamOffset = m_ullAudioOff;
                Event.lParam               = (LPARAM)FirstItem.ulItemSrcOffset;
                Event.wParam               = (WPARAM)LastItem.ulItemSrcOffset +
                                                     LastItem.ulItemSrcLen -
                                                     FirstItem.ulItemSrcOffset;
                hr = pOutputSite->AddEvents( &Event, 1 );

                //--- Output
                if( SUCCEEDED( hr ) )
                {
                    hr = OutputSentence( ItemList, pOutputSite );
                }
            }
        }

        //--- S_FALSE just says that we hit the end, return okay
        if( hr == S_FALSE )
        {
            hr = S_OK;
        }
    }

    return hr;
} /* CTTSEngObj::Speak */

/*****************************************************************************
* CTTSEngObj::OutputSentence *
*----------------------------*
*   This method is used to output an item list.
****************************************************************************/
HRESULT CTTSEngObj::OutputSentence( CItemList& ItemList, ISpTTSEngineSite* pOutputSite )
{
    HRESULT hr = S_OK;
    ULONG WordIndex;

    //--- Lookup words in our voice
    SPLISTPOS ListPos = ItemList.GetHeadPosition();
    while( ListPos && !(pOutputSite->GetActions() & SPVES_ABORT) )
    {
        CSentItem& Item = ItemList.GetNext( ListPos );

        //--- Process sentence items
        switch( Item.pXmlState->eAction )
        {
          //--- Speak some text ---------------------------------------
          case SPVA_Speak:
          {
            //--- We don't say anything for punctuation or control characters
            //    in this sample. 
            if( iswalpha( Item.pItem[0] ) || iswdigit( Item.pItem[0] ) )
            {
                //--- Lookup the word, if we can't find it just use the first one
                for( WordIndex = 0; WordIndex < m_ulNumWords; ++WordIndex )
                {
                    if( ( m_pWordList[WordIndex].ulTextLen == Item.ulItemLen ) &&
                        ( !wcsnicmp( m_pWordList[WordIndex].pText, Item.pItem, Item.ulItemLen )) )
                    {
                        break;
                    }
                }
                if( WordIndex == m_ulNumWords )
                {
                    WordIndex = 0;
                }

                //--- Queue the event
                CSpEvent Event;
                Event.eEventId             = SPEI_WORD_BOUNDARY;
                Event.elParamType          = SPET_LPARAM_IS_UNDEFINED;
                Event.ullAudioStreamOffset = m_ullAudioOff;
                Event.lParam               = Item.ulItemSrcOffset;
                Event.wParam               = Item.ulItemSrcLen;
                hr = pOutputSite->AddEvents( &Event, 1 );

                //--- Queue the audio data
                hr = pOutputSite->Write( m_pWordList[WordIndex].pAudio,
                                         m_pWordList[WordIndex].ulNumAudioBytes,
                                         NULL );

                //--- Update the audio offset
                m_ullAudioOff += m_pWordList[WordIndex].ulNumAudioBytes;
            }
          }
          break;

          //--- Output some silence for a pause -----------------------
          case SPVA_Silence:
          {
            BYTE Buff[1000];
            memset( Buff, 0, 1000 );
            ULONG NumSilenceBytes = Item.pXmlState->SilenceMSecs * 22;

            //--- Queue the audio data in chunks so that we can get
            //    interrupted if necessary.
            while( !(pOutputSite->GetActions() & SPVES_ABORT) )
            {
                if( NumSilenceBytes > 1000 )
                {
                    hr = pOutputSite->Write( Buff, 1000, NULL );
                    NumSilenceBytes -= 1000;
                }
                else
                {
                    hr = pOutputSite->Write( Buff, NumSilenceBytes, NULL );
                    break;
                }
            }

            //--- Update the audio offset
            m_ullAudioOff += NumSilenceBytes;
          }
          break;

          //--- Fire a bookmark event ---------------------------------
          case SPVA_Bookmark:
          {
            //--- The bookmark is NOT a null terminated string in the Item, but we need
            //--- to convert it to one.  Allocate enough space for the string.
            WCHAR * pszBookmark = (WCHAR *)_alloca((Item.ulItemLen + 1) * sizeof(WCHAR));
            memcpy(pszBookmark, Item.pItem, Item.ulItemLen * sizeof(WCHAR));
            pszBookmark[Item.ulItemLen] = 0;
            //--- Queue the event
            SPEVENT Event;
            Event.eEventId             = SPEI_TTS_BOOKMARK;
            Event.elParamType          = SPET_LPARAM_IS_STRING;
            Event.ullAudioStreamOffset = m_ullAudioOff;
            Event.lParam               = (LPARAM)pszBookmark;
            Event.wParam               = _wtol(pszBookmark);
            hr = pOutputSite->AddEvents( &Event, 1 );
          }
          break;

          case SPVA_Pronounce:
            //--- Our sample engine doesn't handle this. If it
            //    did, you would use the associated pronunciation in
            //    the XmlState structure instead of the lexicon.
            break;

          case SPVA_ParseUnknownTag:
            //--- This will reference an XML tag that is unknown to SAPI
            //    if your engine has private tags to control state, you
            //    would examine these tags and see if you recognize it. This
            //    would also be the point that you would make the rendering
            //    state change.
            break;
        }
    }

    return hr;
} /* CTTSEngObj::OutputSentence */

/*****************************************************************************
* CTTSEngObj::GetVoiceFormat *
*----------------------------*
*   Description:
*       This method returns the output data format associated with the
*   specified format Index. Formats are in order of quality with the best
*   starting at 0.
*****************************************************************************/
STDMETHODIMP CTTSEngObj::GetOutputFormat( const GUID * pTargetFormatId, const WAVEFORMATEX * pTargetWaveFormatEx,
                                          GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx )
{
    SPDBG_FUNC( "CTTSEngObj::GetVoiceFormat" );
    HRESULT hr = S_OK;

    hr = SpConvertStreamFormatEnum(SPSF_11kHz16BitMono, pDesiredFormatId, ppCoMemDesiredWaveFormatEx);

    return hr;
} /* CTTSEngObj::GetVoiceFormat */

//
//=== This code is just a simplified parser ==================================
//
/*****************************************************************************
* CTTSEngObj::GetNextSentence *
*-----------------------------*
*   This method is used to create a list of items to be spoken.
****************************************************************************/
HRESULT CTTSEngObj::GetNextSentence( CItemList& ItemList )
{
    HRESULT hr = S_OK;

    //--- Clear the destination
    ItemList.RemoveAll();

    //--- Is there any work to do
    if( m_pCurrFrag == NULL )
    {
        hr = S_FALSE;
    }
    else
    {
        BOOL fSentDone = false;
        BOOL fGoToNextFrag = false;

        while( m_pCurrFrag && !fSentDone )
        {
            if( m_pCurrFrag->State.eAction == SPVA_Speak )
            {
                fSentDone = AddNextSentItem( ItemList );

                //--- Advance fragment?
                if( m_pNextChar >= m_pEndChar )
                {
                    fGoToNextFrag = true;
                }
            }
            else
            {
                //--- Add non spoken fragments
                CSentItem Item;
                Item.pItem           = m_pCurrFrag->pTextStart;
                Item.ulItemLen       = m_pCurrFrag->ulTextLen;
                Item.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset;
                Item.ulItemSrcLen    = Item.ulItemLen;
                Item.pXmlState       = &m_pCurrFrag->State;
                ItemList.AddTail( Item );
                fGoToNextFrag = true;
            }

            if( fGoToNextFrag )
            {
                fGoToNextFrag = false;
                m_pCurrFrag = m_pCurrFrag->pNext;
                if( m_pCurrFrag )
                {
                    m_pNextChar = m_pCurrFrag->pTextStart;
                    m_pEndChar  = m_pNextChar + m_pCurrFrag->ulTextLen;
                }
                else
                {
                    m_pNextChar = NULL;
                    m_pEndChar  = NULL;
                }
            }
        } // end while

        if( ItemList.IsEmpty() )
        {
            hr = S_FALSE;
        }
    }
    return hr;
} /* CTTSEngObj::GetNextSentence */

/*****************************************************************************
* IsSpace *
*---------*
*   Returns true if the character is a space, tab, carriage return, or line feed.
****************************************************************************/
static BOOL IsSpace( WCHAR wc )
{
    return ( ( wc == 0x20 ) || ( wc == 0x9 ) || ( wc == 0xD  ) || ( wc == 0xA ) );
}

/*****************************************************************************
* SkipWhiteSpace *
*----------------*
*   Returns the position of the next non-whitespace character.
****************************************************************************/
static const WCHAR* SkipWhiteSpace( const WCHAR* pPos )
{
    while( IsSpace( *pPos ) ) ++pPos;
    return pPos;
}

/*****************************************************************************
* FindNextToken *
*---------------*
*   Locates the next space delimited token in the stream
****************************************************************************/
static const WCHAR* 
    FindNextToken( const WCHAR* pStart, const WCHAR* pEnd, const WCHAR*& pNext )
{
    const WCHAR* pPos = SkipWhiteSpace( pStart );
    pNext = pPos;
    if( pNext == pEnd )
    {
        pPos = NULL;
    }
    else
    {
        while( *pNext && !IsSpace( *pNext ) )
        {
            if( ++pNext == pEnd )
            {
                //--- This can happen when a text fragment is
                //    tight up against a following xml tag.
                break;
            }
        }
    }
    return pPos;
} /* FindNextToken */

/*****************************************************************************
* SearchSet *
*-----------*
*   Finds the character in the specified array.
****************************************************************************/
BOOL SearchSet( WCHAR wc, const WCHAR* Set, ULONG Count, ULONG* pIndex )
{
    for( ULONG i = 0; i < Count; ++i )
    {
        if( wc == Set[i] )
        {
            *pIndex = i;
            return true;
        }
    }
    return false;
}

/*****************************************************************************
* CTTSEngObj::AddNextSentItem *
*-----------------------------*
*   Locates the next sentence item in the stream and adds it to the list.
*   Returns true if the last item added is the end of the sentence.
****************************************************************************/
BOOL CTTSEngObj::AddNextSentItem( CItemList& ItemList )
{
    //--- Get the token
    ULONG ulIndex;
    CSentItem Item;
    Item.pItem = FindNextToken( m_pNextChar, m_pEndChar, m_pNextChar );

    //--- This case can occur when we hit the end of a text fragment.
    //    Returning at this point will cause advancement to the next fragment.
    if( Item.pItem == NULL )
    {
        return false;
    }

    const WCHAR* pTrailChar = m_pNextChar-1;
    ULONG TokenLen = m_pNextChar - Item.pItem;

    //--- Split off leading punction if any
    static const WCHAR LeadItems[] = { L'(', L'\"', L'{', L'\'', L'[' };
    while( TokenLen > 1 )
    {
        if( SearchSet( Item.pItem[0], LeadItems, sp_countof(LeadItems), &ulIndex ) )
        {
            CSentItem LItem;
            LItem.pItem           = Item.pItem;
            LItem.ulItemLen       = 1;
            LItem.pXmlState       = &m_pCurrFrag->State;
            LItem.ulItemSrcLen    = LItem.ulItemLen;
            LItem.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset +
                                    ( LItem.pItem - m_pCurrFrag->pTextStart );
            ItemList.AddTail( LItem );
            ++Item.pItem;
            --TokenLen;
        }
        else
        {
            break;
        }
    }

    //--- Get primary item insert position
    SPLISTPOS ItemPos = ItemList.AddTail( Item );

    //--- Split off trailing punction if any.
    static const WCHAR EOSItems[] = { L'.', L'!', L'?' };
    static const WCHAR TrailItems[] = { L',', L'\"', L';', L':', L')', L'}', L'\'', L']' };
    SPLISTPOS NextPos = NULL;
    BOOL fIsEOS = false;
    while( TokenLen > 1 )
    {
        BOOL fAddTrailItem = false;
        if( SearchSet( *pTrailChar, EOSItems, sp_countof(EOSItems), &ulIndex ) )
        {
            fIsEOS = true;
            fAddTrailItem = true;
        }
        else if( SearchSet( *pTrailChar, TrailItems, sp_countof(TrailItems), &ulIndex ) )
        {
            fAddTrailItem = true;
        }

        if( fAddTrailItem )
        {
            CSentItem TItem;
            TItem.pItem           = pTrailChar;
            TItem.ulItemLen       = 1;
            TItem.pXmlState       = &m_pCurrFrag->State;
            TItem.ulItemSrcLen    = TItem.ulItemLen;
            TItem.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset +
                                    ( TItem.pItem - m_pCurrFrag->pTextStart );
            NextPos = ItemList.InsertAfter( ItemPos, TItem );
            --TokenLen;
            --pTrailChar;
        }
        else
        {
            break;
        }
    }

    //--- Abreviation or sentence end?
    //    If we are at the end of the buffer then EOS is implied.
    if( *m_pNextChar == NULL )
    {
        fIsEOS = true;
        if( !SearchSet( *(m_pNextChar-1), EOSItems, sp_countof(EOSItems), &ulIndex ) )
        {
            //--- Terminate with a period if we are at the end of a buffer
            //    and no end of sentence punction has been added.
            static const WCHAR* pPeriod = L".";
            CSentItem EOSItem;
            EOSItem.pItem           = pPeriod;
            EOSItem.ulItemLen       = 1;
            EOSItem.pXmlState       = &m_pCurrFrag->State;
            EOSItem.ulItemSrcLen    = EOSItem.ulItemLen;
            EOSItem.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset +
                                    ( (m_pNextChar-1) - m_pCurrFrag->pTextStart );
            ItemList.AddTail( EOSItem );
        }
    }
    else if( pTrailChar[1] == L'.' )
    {
        //--- Here is where you would try to prove that it's not EOS
        //    It might be an abreviation. That's a hard problem that
        //    we are not going to attempt here.
    }
    
    //--- Substitute underscore for apostrophe
    for( ULONG i = 0; i < TokenLen; ++i )
    {
        if( Item.pItem[i] == L'\'' )
        {
            ((WCHAR)Item.pItem[i]) = L'_';
        }
    }

    //--- Add the main item
    if( TokenLen > 0 )
    {
        Item.ulItemLen       = TokenLen;
        Item.pXmlState       = &m_pCurrFrag->State;
        Item.ulItemSrcLen    = Item.ulItemLen;
        Item.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset +
                               ( Item.pItem - m_pCurrFrag->pTextStart );
        ItemList.SetAt( ItemPos, Item );
    }

    return fIsEOS;
} /* CTTSEngObj::AddNextSentItem */

