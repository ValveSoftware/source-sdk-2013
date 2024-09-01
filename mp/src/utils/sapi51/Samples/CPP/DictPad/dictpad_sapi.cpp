/******************************************************************************
*   dictpad_sapi.cpp 
*       This file contains the methods of CDictationPad that 
*       pertain to the SAPI interfaces used in this app.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/
#include "stdafx.h"
#include "dictpad.h"
#include "cmdmode.h"
#include "dictmode.h"
#include <commctrl.h>
#include <winuser.h>

#define SAPIINITERR _T("SAPI Inititalization Error")

#ifndef _DEBUG
    #define DUMP_EVENT_NAME(x)
#else
    void DumpEventName(int id);     // forward definition
    #define DUMP_EVENT_NAME(x) DumpEventName(x)
#endif

/****************************************************************************
* CDictationPad::InitializeSAPIObjs() *
*-------------------------------------*
*   Description:
*       "Unplugs" any SAPI objects that we may currently have around.
*       Set up the SR and TTS objects and obtains the relevant information
*       about them.
*       If any errors occur in the initialization, compains.
*   Return: 
*       S_OK
*       failed HRESULTs of the various SAPI initialization routines
*****************************************************************************/   
HRESULT CDictationPad::InitializeSAPIObjs()
{
    HRESULT hr = S_OK;

    // If we're waiting for a recognition, give up on it.
    if ( m_pRecoEventMgr->IsProcessingPhrase()  && (m_dwFlags & DP_DICTATION_MODE) )
    {
        m_pRecoEventMgr->FalseRecognition();
        ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY, 
            MAKELONG( TBSTATE_ENABLED, 0 ) );
        if ( m_pCandidateList )
        {
            m_pCandidateList->ShowButton( true );
        }

        // This will unleash any WM_COMMANDs that were waiting
        m_pRecoEventMgr->DoneProcessingPhrase();
    }
    SetGrammarState( FALSE );


    // Before releasing the dictation reco context, serialize all of the current
    // reco results that depend on this reco context.
    // All of the active reco results live in the text-run list.
    CComPtr<IStream> cpTextRunListStream;
    if ( m_cpDictRecoCtxt )
    {
        if ( !m_pTextRunList )
        {
            _ASSERTE( false );
            return E_UNEXPECTED;
        }

        // Create an IStream for the text-run list and write the serialized
        // text-run list to that stream
        hr = ::CreateStreamOnHGlobal( NULL, TRUE, &cpTextRunListStream );
        if ( SUCCEEDED( hr ) )
        {
            hr = m_pTextRunList->Serialize( 
                cpTextRunListStream, m_cpDictRecoCtxt );
        }

        // Commit the changes to the stream so that we can read them later
        if ( SUCCEEDED( hr ) )
        {
            hr = cpTextRunListStream->Commit( STGC_DEFAULT );
        }

        if ( FAILED( hr ) )
        {
#ifdef _DEBUG
            ::MessageBox( m_hClient, _T("Error serializing the text run list"), SAPIINITERR, MB_OK );
#endif
            return hr;
        }
    }

    // Release the SAPI object in the reverse order in which they
    // were created.  This will ensure that everything really does 
    // get released
    if ( m_cpVoice )
    {
        m_cpVoice.Release();
    }
    if ( m_cpCCRecoCtxt )
    {
        m_cpCCRecoCtxt.Release();
    }
    if ( m_cpDictRecoCtxt )
    {
        m_cpDictRecoCtxt.Release();
    }
    if ( m_cpRecoEngine )
    {
        m_cpRecoEngine.Release();
    }

    // Set up the SR engine
    hr = S_OK;
    HMENU hMenu = ::GetMenu( m_hClient );
    if ( ::GetMenuState( hMenu, IDM_SHAREDENGINE, MF_BYCOMMAND ) == MF_CHECKED )
    {
        // Shared reco engine.
        // For a shared reco engine, the audio gets setup automatically
        hr = m_cpRecoEngine.CoCreateInstance( CLSID_SpSharedRecognizer );
    }
    else
    {
        // Inproc reco engine.
        hr = m_cpRecoEngine.CoCreateInstance( CLSID_SpInprocRecognizer );
    
        // For an inproc reco engine, we need to call ISpRecognizer::SetInput() 
        // ourselves.
        CComPtr<ISpObjectToken> cpAudioToken;
        if (SUCCEEDED(hr))
        {
            // Get a token for the default audio input
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpAudioToken);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_cpRecoEngine->SetInput(cpAudioToken, TRUE);
        }
    }
    
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error creating reco engine"), SAPIINITERR, MB_OK );
#endif
        return hr;
    }

    // Create the recognition contexts (one for dictation mode and one for command mode).
    // An ISpRecoContext is created off an ISpRecognizer
    hr = m_cpRecoEngine->CreateRecoContext( &m_cpDictRecoCtxt );
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error creating dictation reco context"), SAPIINITERR, MB_OK );
#endif
        return hr;
    }
    hr = m_cpRecoEngine->CreateRecoContext( &m_cpCCRecoCtxt );
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error creating C&C reco context"), SAPIINITERR, MB_OK );
#endif
        return hr;
    }

    // Now that there is a recognition context, deserialize the text-run list 
    // (serialized before any pre-existing recognition context was released)
    // using the new reco context
    if ( cpTextRunListStream )
    {
        // Move the seek pointer in the stream to the beginning, since
        // that is where we want to start reading from
        LARGE_INTEGER li0;
        ::memset( (void *) &li0, 0, sizeof( li0 ) );
        hr = cpTextRunListStream->Seek( li0, STREAM_SEEK_SET, NULL );

        // Deserialize the text-run list using the new reco context
        if ( SUCCEEDED( hr ) )
        {
            hr = m_pTextRunList->Deserialize( cpTextRunListStream, m_cpDictRecoCtxt );
        }

        if ( FAILED( hr ) )
        {
#ifdef _DEBUG
            ::MessageBox( m_hClient, _T("Error deserializing the text run list"), SAPIINITERR, MB_OK );
#endif
            return hr;
        }
    }

    // Create an ISpVoice for TTS
    // This is the voice associated with our recognition context
    hr = m_cpDictRecoCtxt->GetVoice(&m_cpVoice);
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error creating voice"), SAPIINITERR, MB_OK );
#endif
        return hr;
    }

    // Set up the reco context for retaining audio in the dictation reco context.
    // Get the stream format for 8kHz, 8-bit mono
    GUID guidFormatId = GUID_NULL;
    WAVEFORMATEX *pWaveFormatEx = NULL;
    hr = SpConvertStreamFormatEnum(SPSF_8kHz8BitMono, &guidFormatId, &pWaveFormatEx);
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error converting stream format"), SAPIINITERR, MB_OK );
#endif
    }
    else
    {
        // Tell the reco contexts to retain its audio in this format
        hr = m_cpDictRecoCtxt->SetAudioOptions( SPAO_RETAIN_AUDIO, &guidFormatId, pWaveFormatEx );
    }
#ifdef _DEBUG
    if ( FAILED( hr ) )
    {
        ::MessageBox( m_hClient, _T("Error setting retained audio data option for dictation reco context"), SAPIINITERR, MB_OK );
    }
#endif
    ::CoTaskMemFree(pWaveFormatEx);
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Now that we have a reco engine, we can get its LangID and give that to
    // the candidate list UI, which needs it for locale information
    if ( m_pCandidateList )
    {
        SPRECOGNIZERSTATUS stat;
        ZeroMemory( &stat, sizeof( stat ) );
        hr = m_cpRecoEngine->GetStatus( &stat );
        m_pCandidateList->SetLangID( stat.aLangID[0] );
        if ( FAILED( hr ) )
        {
#ifdef _DEBUG
            ::MessageBox( m_hClient, _T("Error getting the status of the reco engine"), SAPIINITERR, MB_OK );
#endif
            return hr;
        }
    }
    else
    {
        return E_OUTOFMEMORY;
    }

    // This allows the main window to be notified of SAPI events
    hr = InitSAPICallback( m_hClient );
    if ( FAILED( hr ) )
    {
#ifdef _DEBUG
        ::MessageBox( m_hClient, _T("Error setting up SAPI event notification"), SAPIINITERR, MB_OK );
#endif
        return hr;
    }

    // Load the appropriate grammars
    hr = LoadGrammars();
    if ( FAILED( hr ) )
    {
        HRESULT hr2 = ERROR_RESOURCE_LANG_NOT_FOUND;
        if (( SPERR_UNSUPPORTED_LANG == hr ) || ( ERROR_RESOURCE_LANG_NOT_FOUND == (0xffff & hr)))
        {
            MessageBoxFromResource( m_hClient, IDS_UNSUPPORTEDLANG, SAPIINITERR, MB_ICONEXCLAMATION );
        }
#ifdef _DEBUG
        else
        {
            ::MessageBox( m_hClient, _T("Error loading the grammars"), SAPIINITERR, MB_OK );
        }
#endif
        return hr;
    }

    // Update menu items that are engine specific
    // Errors in this realm are not horrible, so we carry on happily afterward
    BOOL  fSupported;

    // User training UI
    HRESULT hrUISupport = m_cpRecoEngine->IsUISupported( SPDUI_UserTraining, NULL, 0, &fSupported);
#ifdef _DEBUG
    if (FAILED( hrUISupport ) )
    {
        ::MessageBox( m_hClient, _T("Querying engine for 'SPDUI_UserTraining' support failed"), SAPIINITERR, MB_OK );
    }
#endif
    ::EnableMenuItem( hMenu, IDM_VOICE_TRAINING, 
        ( (SUCCEEDED(hrUISupport) && fSupported) ? MF_ENABLED: MF_GRAYED ) );

    // Mic training UI
    hrUISupport = m_cpRecoEngine->IsUISupported( SPDUI_MicTraining, NULL, 0, &fSupported );
#ifdef _DEBUG
    if (FAILED( hrUISupport ) )
    {
        ::MessageBox( m_hClient, _T("Querying engine for 'SPDUI_MicTraining' support failed"), SAPIINITERR, MB_OK );
    }
#endif
    ::EnableMenuItem( hMenu, IDM_MICROPHONE_SETUP, 
        ( (SUCCEEDED(hrUISupport) && fSupported) ? MF_ENABLED: MF_GRAYED ) );

    // Add/Remove words UI
    hrUISupport = m_cpRecoEngine->IsUISupported( SPDUI_AddRemoveWord, NULL, 0, &fSupported );
#ifdef _DEBUG
    if (FAILED( hrUISupport ) )
    {
        ::MessageBox( m_hClient, _T("Querying engine for 'SPDUI_AddRemoveWord' support failed"), SAPIINITERR, MB_OK );
    }
#endif
    ::EnableMenuItem( hMenu, IDM_ADDREMOVEWORDS, 
        ( (SUCCEEDED(hrUISupport) && fSupported) ? MF_ENABLED: MF_GRAYED ) );

    return S_OK;
}   /* CDictationPad::InitializeSAPIObjs */


/******************************************************************************************
* CDictationPad::InitSAPICallback() *
*-----------------------------------*
*   Description:
*       Sets up the notification mechanisms for dictation, C&C,
*       and TTS.  We will be using window messages as our
*       notification method
*       Indicates which SR and TTS events we are interested in.
*       Also sets up the dictation and C&C grammars
*   Return value:
*       S_OK
*       Error code from any of the calls to SAPI
********************************************************************************************/
HRESULT CDictationPad::InitSAPICallback( HWND hWnd )
{
    // Set recognition notification for dictation
    CComPtr<ISpNotifyTranslator> cpDictNotify;
    HRESULT hr = cpDictNotify.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpDictNotify->InitWindowMessage(hWnd, WM_DICTRECOEVENT, 0, 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpDictRecoCtxt->SetNotifySink(cpDictNotify);
    }

    // Set recognition notification for C & C
    CComPtr<ISpNotifyTranslator> cpCCNotify;
    hr = cpCCNotify.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpCCNotify->InitWindowMessage(hWnd, WM_CCRECOEVENT, 0, 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpCCRecoCtxt->SetNotifySink(cpCCNotify);
    }

    // Set recognition notification for TTS
    CComPtr<ISpNotifyTranslator> cpTTSNotify;
    hr = cpTTSNotify.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpTTSNotify->InitWindowMessage(hWnd, WM_TTSEVENT, 0, 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpVoice->SetNotifySink(cpTTSNotify);
    }

    // ISpRecoContext::SetInterest() allows the caller to indicate
    // which types of SAPI events it wants to be notified about.
    // Initially set interest in no events for the recognition contexts; 
    // The event interests will be set when the grammars are first activated
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpDictRecoCtxt->SetInterest( m_ullDictInterest, m_ullDictInterest );
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpCCRecoCtxt->SetInterest( m_ullCCInterest, m_ullCCInterest );
    }

    // Set interest for voice events
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice->SetInterest(SPFEI_ALL_TTS_EVENTS, SPFEI_ALL_TTS_EVENTS);
    }

    return hr;
}   /* CDictationPad::InitSAPICallback */

/******************************************************************************************
* CDictationPad::LoadGrammars() *
*-------------------------------*
*   Description:
*       Loads the grammars for dictation and C&C.
*       (Note that the grammars still need to be activated
*       in order for anything to be recognized from them)
*   Return value:
*       S_OK
*       Return value of ISpRecoContext::CreateGrammar()
*       Return value of ISpRecoGrammar::LoadDictation()
*       Return value of ISpRecoGrammar::LoadCmdFromResource()
********************************************************************************************/
HRESULT CDictationPad::LoadGrammars()
{
    // Create the grammar for general dictation, and make it the statistical
    // language model for dictation
    m_cpDictGrammar.Release();
    HRESULT hr = m_cpDictRecoCtxt->CreateGrammar(GID_DICTATION, &m_cpDictGrammar);
    if (SUCCEEDED(hr))
    {
        hr = m_cpDictGrammar->LoadDictation(NULL, SPLO_STATIC);
    }
    if (FAILED(hr))
    {
        m_cpDictGrammar.Release();
    }

    // We need a langid from the engine in order to load the grammars in the correct language
    SPRECOGNIZERSTATUS Stat;
    LANGID langid = 0;
    ::memset( &Stat, 0, sizeof( Stat ) );
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpRecoEngine->GetStatus( &Stat );
    }
    if ( SUCCEEDED( hr ) )
    {
        langid = Stat.aLangID[0];
    }
    

    // Create the grammar for the commands that are available for dictation mode.
    // The compiled C&C grammars are resources in this project
    if( SUCCEEDED( hr ) )
    {
        m_cpDictCCGrammar.Release();
        hr = m_cpDictRecoCtxt->CreateGrammar(GID_DICTATIONCC, &m_cpDictCCGrammar);
        if (SUCCEEDED(hr))
        {
            hr = m_cpDictCCGrammar->LoadCmdFromResource(NULL, (const WCHAR*)MAKEINTRESOURCE(IDR_DICTATION_MODE_CFG),
                                                   L"SRGRAMMAR", langid,
                                                   SPLO_STATIC);
        }
        if (FAILED(hr))
        {
            m_cpDictCCGrammar.Release();
        }
    }

    // Create the grammar for command mode
    if( SUCCEEDED( hr ) )
    {
        m_cpCCGrammar.Release();
        hr = m_cpCCRecoCtxt->CreateGrammar(GID_CC, &m_cpCCGrammar);
        if (SUCCEEDED(hr))
        {
            hr = m_cpCCGrammar->LoadCmdFromResource(NULL, (const WCHAR*)MAKEINTRESOURCE( IDR_COMMAND_MODE_CFG ),
                                                    L"SRGRAMMAR", langid,
                                                    SPLO_STATIC);
        }
        if (FAILED(hr))
        {
            m_cpCCGrammar.Release();
        }
    }

    return hr;
}   /* CDictationPad::LoadGrammars */


/**************************************************************************************
* CDictationPad::SRDictEventHandler() *
*-------------------------------------*
*   Description:
*       Called whenever there is an SR event from the dictation
*       reco context.  
*       Processes the event appropriately.
*   Return:
*       true iff successful
****************************************************************************************/
bool CDictationPad::SRDictEventHandler()
{
    // There may be multiple events that come through in one notification, so we must
    // look for all of them
    CSpEvent event;
    while ( event.GetFrom(m_cpDictRecoCtxt) == S_OK )
    {
        DUMP_EVENT_NAME(event.eEventId);
        
        switch (event.eEventId)
        {
            // PHRASE_START: The engine heard the start of sounds that it thinks
            // is recognizable speech.
            // This event is guaranteed to be followed eventually by either an
            // SPEI_RECOGNITION or an SPEI_FALSE_RECOGNITION
            case SPEI_PHRASE_START:
            {    
                // We don't expect SR events during a playback
                _ASSERTE( !(m_dwFlags & DP_IS_SPEAKING) );

                HIMC himc = ::ImmGetContext( m_hClient );
                ::ImmNotifyIME( himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0 );

                // Throw out this event unless our grammars are active
                // and we are in dictation mode.
                // This will get rid of PHRASE_STARTs from other reco contexts
                if ( !(m_dwFlags & DP_GRAMMARS_ACTIVE) || !(m_dwFlags & DP_DICTATION_MODE) )
                {
                    break;
                }
        
                // Alternates UI and playback should be disabled
                ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY, MAKELONG(TBSTATE_INDETERMINATE, 0) );
                if ( m_pCandidateList )
                {
                    m_pCandidateList->ShowButton( false );
                }

                // Tell the recoevent manager about what the selection is now,
                // and move the selection to an IP at the end of the "waiting" text (...)
                // This move should not trigger an update
                m_dwFlags |= DP_SKIP_UPDATE;
                HRESULT hr = m_pRecoEventMgr->PhraseStart( *m_cpTextSel );
                m_dwFlags &= ~DP_SKIP_UPDATE;
                if ( FAILED( hr ) )
                {
                    return false;
                }

                // Status bar update
                {
                    USES_CONVERSION;
                    CSpDynamicString dstr;
                    dstr = L"Dictation mode";
                    ::SendMessage( m_hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)W2T( dstr ) );
                }
                break;
            }

            // SPEI_RECO_STATE_CHANGE: For whatever reason, SAPI found it necessary to change the
            // reco state.
            // This can happen e.g. if a shared recognizer is running and some other context using 
            // that recognizer changes its RecoState
            case SPEI_RECO_STATE_CHANGE:
                if (event.RecoState() == SPRST_INACTIVE)
                    SetGrammarState( (event.RecoState() != SPRST_INACTIVE) );
                break;

            // FALSE_RECOGNITION: The engine thought this utterance might be recognizable speech,
            // but it turned out not to be
            // RECO_OTHER_CONTEXT: This will happen in the shared case.  If some other app
            // is using the engine at the same time, and it gets a recognition, then Dictpad
            // will get this message to indicate that the utterance was recognized, just for 
            // someone else
            case SPEI_FALSE_RECOGNITION:
            case SPEI_RECO_OTHER_CONTEXT:
                // Throw out this event unless our grammars are active
                // and we are in dictation mode.
                // This will get rid of PHRASE_STARTs from other reco contexts
                if ( !(m_dwFlags & DP_GRAMMARS_ACTIVE) || !(m_dwFlags & DP_DICTATION_MODE) )
                {
                    break;
                }

                m_pRecoEventMgr->FalseRecognition();
                ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY, 
                    MAKELONG( TBSTATE_ENABLED, 0 ) );
                if ( m_pCandidateList )
                {
                    m_pCandidateList->ShowButton( m_pCandidateList->FHasAlternates() );
                }

                // This will unleash any WM_COMMANDs that were waiting
                m_pRecoEventMgr->DoneProcessingPhrase();
                
                break;

            // HYPOTHESIS: One of a set of ongoing "guesses" that the engine makes.
            //              Any number of these can precede an SPEI_RECOGNITION
            // RECOGNITION: The engine is done processing the utterance and has a result
            case SPEI_HYPOTHESIS:
            case SPEI_RECOGNITION:
            {
                if ( m_dwFlags & DP_IS_SPEAKING )
                {
                    // Don't handle reco notifications if a playback is going on
                    break;
                }

                ISpRecoResult *pResult = event.RecoResult();
                if ( !pResult )
                {
                    // We expect these events to come with reco results
                    return false;
                }

                // The result can be either from the dictation grammar
                // or from the dictation-mode C&C grammar
                SPPHRASE * pPhrase = NULL;
                HRESULT hr = pResult->GetPhrase( &pPhrase );
                
                bool fSuccess = false;
                if ( SUCCEEDED( hr ) )
                {
                    switch( pPhrase->ullGrammarID )
                    {
                    case GID_DICTATION:

                        // Put the hypotesis or recognition into the edit window
                        fSuccess = ProcessDictation( *pResult, event.eEventId );
                        break;

                    case GID_DICTATIONCC:
                        
                        // Don't handle hypotheses, since this is just a command
                        if ( event.eEventId == SPEI_HYPOTHESIS )
                        {
                            fSuccess = true;
                            break;
                        }

                        // Carry out the command
                        fSuccess = ProcessDictationModeCommands( *pResult );
                        break;
                        
                    default:
                        // We should not be seeing any events from other grammars
                        _ASSERTE( false );
                        fSuccess = false;
                        break;
                    }
                    ::CoTaskMemFree(pPhrase);
                }
                
                if ( SPEI_RECOGNITION == event.eEventId )
                {
                    // The playback option will have been disabled by the 
                    // PHRASE_START event for this utterance.
                    // Since we are done processing this utterance,
                    // playback can now be re-enabled
                    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_PLAY, 
                        MAKELONG( TBSTATE_ENABLED, 0 ) );
                }

                if ( !fSuccess || FAILED( hr ) )
                {
                    // Bail: something went wrong
                    return false;
                }

                break;
            }

            default:
                break;
        }
    }

    return true;
}   /* CDictationPad::SRDictEventHandler */


/**************************************************************************************
* CDictationPad::SRCCEventHandler() *
*-----------------------------------*
*   Description:
*       Called whenever there is an SR or a TTS event.  
*       Processes the event appropriately.
*   Return:
*       true iff successful
****************************************************************************************/
bool CDictationPad::SRCCEventHandler()
{
    // Numerous events may have come at once, hence the loop
    CSpEvent event;
    while (S_OK == event.GetFrom(m_cpCCRecoCtxt))
    {
        DUMP_EVENT_NAME(event.eEventId);

        if ( SPEI_RECOGNITION == event.eEventId )
        {  
            // Get the reco result from the event
            ISpRecoResult *pResult;
            pResult = event.RecoResult();
            if ( !pResult )
            {
                return false;
            }

            bool fSuccess = ProcessCommandModeCommands( *pResult );
          
            if ( !fSuccess )
            {
                // We really expect to succeed
                _ASSERTE( false );
                return false;
            }
        }
        else if ( SPEI_RECO_STATE_CHANGE == event.eEventId )
        {
            // SPEI_RECO_STATE_CHANGE: For whatever reason, SAPI found it necessary to change the
            // reco state
            if (event.RecoState() == SPRST_INACTIVE)
                SetGrammarState( (event.RecoState() != SPRST_INACTIVE) );
        }
    }

    return true;
}   /* CDictationPad::SRCCEventHandler */


/******************************************************************************
* CDictationPad::TTSEventHandler *
*--------------------------------*
*   Description:
*       This method gets called when we receive a TTS event.
*       We use TTS events mostly for the "follow-the-bouncing-
*       ball" text tracking during a playback.
*   Return:
*       none.  If this function fails, the text tracking will be
*       wrong, which is not serious.
******************************************************************************/
void CDictationPad::TTSEventHandler()
{
    CSpEvent event;

    // Process the TTS events only if we are speaking,
    // otherwise just take them off the queue
    if ( !(m_dwFlags & DP_IS_SPEAKING) )
    {
        // Empty the queue of all waiting events
        while ( S_OK == event.GetFrom( m_cpVoice ) )
        {
        }

        return;
    }

    // CDictationPad::StartSpeaking() should have been called before the playback
    // begins.  This would have set m_SpeakInfo.pCurrentNode to the appropriate
    // start node in the TextRunList
    _ASSERTE( m_SpeakInfo.pCurrentNode );
    if ( !m_SpeakInfo.pCurrentNode )
    {
        // Error: bail
        return;
    }
    
    // There might be numerous events coming at once, hence the loop
    SPVOICESTATUS Stat;
    HRESULT hr = m_cpVoice->GetStatus( &Stat, NULL );
    while ( SUCCEEDED( hr ) && (event.GetFrom(m_cpVoice) == S_OK) )
    {
        switch( event.eEventId )
        {
            // Each TextRun in the TextRunList gets its own call to
            // ISpVoice::Speak() (or ISpVoice::SpeakAudio()), and
            // thus each one will generate its own SPEI_START_INPUT_STREAM
            // and SPEI_END_INPUT_STREAM

            case SPEI_START_INPUT_STREAM:

                // If the node is a dictation node, highlight the whole thing
                if ( m_SpeakInfo.pCurrentNode->pTextRun->IsDict() )
                {
                    // Find out where the speaking range starts and ends
                    // (since these might be somewhere within this TextRun
                    long lSpeakRangeStart, lSpeakRangeEnd;
                    m_SpeakInfo.pSpeakRange->GetStart( &lSpeakRangeStart );
                    m_SpeakInfo.pSpeakRange->GetEnd( &lSpeakRangeEnd );

                    // The highlighting should start at the beginning of the speaking range
                    // and end at the end of pCurrentNode's run or the end of the speaking
                    // range, whichever comes first
                    HighlightAndBringIntoView( *m_cpTextDoc, lSpeakRangeStart,
                        __min( m_SpeakInfo.pCurrentNode->pTextRun->GetEnd(), lSpeakRangeEnd ) );
                }

                break;

            case SPEI_END_INPUT_STREAM:

                // Stat.ulLastStreamQueued is the index of the last TextRun we have asked
                // to speak (starting with the first one in the TextRunList that we asked
                // to speak).
                if ( m_SpeakInfo.ulCurrentStream >= Stat.ulLastStreamQueued )
                {
                    // We just got to the end of the final input stream
                    // for this speak.
                    EndSpeaking();
                }
                else
                {
                    // We have moved on to the next stream, so move the pCurrentNode along
                    m_SpeakInfo.pCurrentNode = m_SpeakInfo.pCurrentNode->pNext;
                    m_SpeakInfo.ulCurrentStream++;
                }

                break;

            // SPEI_WORD_BOUNDARY events are generated from calls to ISpVoice::Speak(), 
            // and the status of the voice indicates offsets in the text input to that
            // call to tell us where the voice is.
            // ISpVoice::SpeakAudio() does not generate these events
            case SPEI_WORD_BOUNDARY:
            { 
                // Highlight the word being spoken
            
                // The voice's status will tell us where the word is relative
                // to the start of the current run (in characters)
                // and how long it is (in bytes)
                ULONG ulWordPos = Stat.ulInputWordPos;
                ULONG ulWordLen = Stat.ulInputWordLen / sizeof( char );

                // Find out where the speak started to determine whether
                // it started somewhere in this run
                long lSpeakRangeStart;
                m_SpeakInfo.pSpeakRange->GetStart( &lSpeakRangeStart );
                if (m_SpeakInfo.pCurrentNode->pTextRun->WithinRange( lSpeakRangeStart ))
                {
                    // This run is the first run we are speaking, 
                    // and thus the start of the speak range may not
                    // be the same as the start of this run.
                    // The position of the word is relative to the start of the 
                    // speak range
                    ulWordPos += lSpeakRangeStart;
                }
                else
                {
                    // This is not the first textrun in this speaking range.
                    // The word position is relative to the start of this run.
                    ulWordPos += m_SpeakInfo.pCurrentNode->pTextRun->GetStart();
                }

                // The highlighting will end at the end of this word
                long lHighlightEnd;
                lHighlightEnd = ulWordPos + ulWordLen;
        
                // Highlight from the beginning of the speak range to the end of
                // this word
                HighlightAndBringIntoView( *m_cpTextDoc, lSpeakRangeStart, lHighlightEnd );
                
                break;
            }
        
            default:
                break;
        }
    }
}   /* CDictationPad::TTSEventHandler */

/************************************************************************************
* CDictationPad::SetSREventInterest() *
*-------------------------------------*
*   Description:
*       Sets/unsets interest in SR events for both SR contexts.
*************************************************************************************/
void CDictationPad::SetSREventInterest( bool fOn )
{
    ULONGLONG ullInterest = fOn ? m_ullDictInterest : 0;
    m_cpDictRecoCtxt->SetInterest( ullInterest, ullInterest );

    ullInterest = fOn ? m_ullCCInterest : 0;
    m_cpCCRecoCtxt->SetInterest( ullInterest, ullInterest );
}   /* CDictationPad::SetSREventInterest */

/************************************************************************************
* CDictationPad::ProcessDictationModeCommands() *
*-----------------------------------------------*
*   Description:
*       Processes commands spoken while in dictation mode (i.e. from the 
*       DictCC grammar.
*   Return:
*       true iff successful
*************************************************************************************/
bool CDictationPad::ProcessDictationModeCommands( ISpRecoResult &rResult )
{
    // Get the phrase associated with this reco result.
    // This SPPHRASE gets CoTaskMemAlloced, and we free it below.
    SPPHRASE *pPhrase = NULL;
    HRESULT hr = rResult.GetPhrase( &pPhrase );
    if ( FAILED( hr ) )
    {
        // Couldn't get the phrase
        return false;
    }

    // Set the status bar text
    CSpDynamicString dstr = L"Dictation Mode: ";
    if( pPhrase->Rule.pszName )
    {
        dstr.Append( pPhrase->Rule.pszName );
    }
    else
    {
        dstr.Append( L"<null>" );
    }
    USES_CONVERSION;
    ::SendMessage( m_hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)W2T( dstr ) );

    // We are done processing this utterance
    m_pRecoEventMgr->DoneProcessingPhrase();

    // The Rule.ulId member of an SPPHRASE tells which C&C rule needs to be fired
    switch( pPhrase->Rule.ulId )
    {
    case PID_DictMode: 
        // This rule sends us to command mode
        ::SendMessage( m_hClient, WM_COMMAND, IDM_COMMAND_MODE, 0 );
        break;

    default:
        {    
            // Default - just dump the command to the screen
            DumpCommandToScreen( m_hClient, rResult );
            break;
        }
    }

    ::CoTaskMemFree( pPhrase );

    // Success
    return( true );

}   /* CDictationPad::ProcessDictationModeCommands */

/************************************************************************************
* DATA FOR PROCESSING COMMANDS *
*************************************************************************************/
// This array bundles the responses for all of the voice-enabled menu & toolbar items accessible during command mode.
//    NOTE:  Items that use the WM_NULL uiMessage are place holders.  When the app actually implements those features, 
//           then those lines will need to be updated
static PROPERTYMAP s_aCmdModePropertyMap[] = 
{
    { PID_CmdMenuFile,                  WM_SYSCOMMAND,  SC_KEYMENU,             'f' },
    { PID_CmdMenuEdit,                  WM_SYSCOMMAND,  SC_KEYMENU,             'e' },
    { PID_CmdMenuVoice,                 WM_SYSCOMMAND,  SC_KEYMENU,             'v' },
    { PID_CmdMenuHelp,                  WM_SYSCOMMAND,  SC_KEYMENU,             'h' },
    { PID_CmdNew,                       WM_COMMAND,     ID_FILE_NEW,            0   },
    { PID_CmdOpen,                      WM_COMMAND,     ID_FILE_OPEN,           0   },
    { PID_CmdSave,                      WM_COMMAND,     ID_FILE_SAVE,           0   },
    { PID_CmdSaveAs,                    WM_COMMAND,     ID_FILE_SAVEAS,         0   },
    { PID_CmdExit,                      WM_COMMAND,     IDM_EXIT,               0   },
    { PID_CmdCut,                       WM_COMMAND,        ID_EDIT_CUT,                      0   },
    { PID_CmdCopy,                      WM_COMMAND,        ID_EDIT_COPY,                      0   },
    { PID_CmdPaste,                     WM_COMMAND,        ID_EDIT_PASTE,                      0   },
    { PID_CmdDictationMode,             WM_COMMAND,     IDM_DICTATION_MODE,     0   },
    { PID_CmdCommandMode,               WM_COMMAND,     IDM_COMMAND_MODE,       0   },
    { PID_CmdMicrophone,                WM_COMMAND,     IDM_MIC_TOGGLE,         0   },
    { PID_CmdPlayback,                  WM_COMMAND,     IDM_PLAY,               0   },
    { PID_CmdAddDeleteWords,            WM_COMMAND,     IDM_ADDREMOVEWORDS,     0   },
    { PID_CmdSelectWholeWords,          WM_COMMAND,     IDM_WHOLE_WORDS,        0   },
    { PID_CmdSharedRecoEngine,          WM_COMMAND,     IDM_SHAREDENGINE,       0   },
    { PID_CmdVoiceTraining,             WM_COMMAND,     IDM_VOICE_TRAINING,     0   },
    { PID_CmdMicrophoneSetup,           WM_COMMAND,     IDM_MICROPHONE_SETUP,   0   },
    { PID_CmdAbout,                     WM_COMMAND,     IDM_ABOUT,              0   },
    { PID_CmdEscape,                    WM_KEYDOWN,     VK_ESCAPE,              0x10001}
};  
static const int s_iCmdModePropertyMapSize_c = sizeof( s_aCmdModePropertyMap ) / sizeof( *s_aCmdModePropertyMap );


/************************************************************************************
* CDictationPad::ProcessCommandModeCommands() *
*---------------------------------------------*
*   Description:
*       Processes commands spoken while in command mode.
*   Return:
*       true iff successful
*************************************************************************************/
bool CDictationPad::ProcessCommandModeCommands( ISpRecoResult &rResult  ) 
{ 
    SPPHRASE *pPhrase = NULL;
    HRESULT hr = rResult.GetPhrase( &pPhrase );
    if ( FAILED( hr ) || pPhrase->ullGrammarID != GID_CC )
    {
        ::CoTaskMemFree(pPhrase);
        return false;
    }
    
    // Set the status bar text
    CSpDynamicString dstr;
    dstr = L"Command Mode: ";
    dstr.Append( pPhrase->Rule.pszName );
    USES_CONVERSION;
    ::SendMessage( m_hStatusBar, SB_SETTEXT, 0, (LPARAM)W2T( dstr ) );
    if( pPhrase->Rule.pszName )
    {
        dstr.Append( pPhrase->Rule.pszName );
    }
    else
    {
        dstr.Append( L"<null>" );
    }
    ::SendMessage( m_hStatusBar, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)W2T( dstr ) );

    switch( pPhrase->Rule.ulId )
    {
    case PID_CmdMenu:
        {
            _ASSERTE( pPhrase->pProperties );
            
            // Spin through the property map array to figure out 
            // which voice-enabled menu item just got triggered
            for( int i = 0;  i < s_iCmdModePropertyMapSize_c;  ++i )
            {
                if( pPhrase->pProperties[0].vValue.ulVal == s_aCmdModePropertyMap[ i ].dwPropertyID )
                {
                    // WM_CANCELMODE will destroy any active popup menus, which is what we want 
                    // if the user selects a menu item
                    if ( WM_SYSCOMMAND != s_aCmdModePropertyMap[ i ].uiMessage )
                    {
                        ::SendMessage( m_hClient, WM_CANCELMODE, 0, 0 );
                    
                        // Turn off grammars while we are processing this command,
                        // unless this is a command to deactivate the grammars
                        if ( IDM_MIC_TOGGLE != s_aCmdModePropertyMap[ i ].wParam )
                        {
                            SetGrammarState( FALSE );
                        }

                    }
                    
                    // When we've discovered which voice-enabled menu item just got triggered, we'll
                    // use our property map to figure out how we simulate that menu item
                    ::SendMessage( m_hClient, 
                                   s_aCmdModePropertyMap[ i ].uiMessage, 
                                   s_aCmdModePropertyMap[ i ].wParam, 
                                   s_aCmdModePropertyMap[ i ].lParam );
    
                    if (( WM_SYSCOMMAND != s_aCmdModePropertyMap[ i ].uiMessage )
                        && ( IDM_MIC_TOGGLE != s_aCmdModePropertyMap[ i ].wParam ))
                    {
                        // Reactivate grammars
                        SetGrammarState( TRUE );
                    }

                    break;
                }
            }

        }
        break;

    case PID_CmdNavigationVertical:
        {
            // Vertical navigation has 3 components: direction, unit of travel & number of units.
            // The grammar was created to allow some flexibility in the way the user says these 3 components.
            // So, the first task is to figure out which components the user actually said - default values will
            // be used for the component not explicitly mentioned.

            // Default values: scroll down 1 page
            long lUnit  = tomScreen;
            long lCount = 1;
            BOOL bDown  = TRUE;
            const SPPHRASEPROPERTY    *pProp = pPhrase->pProperties;

            // Spin thru all of the properties, identifying the individual components
            // It is possible to have the 'direction' specified twice - the last value will be used.
            while( pProp )
            {
                switch( pProp->ulId )
                {
                case PID_CmdDirection:
                    if( PID_CmdUp == pProp->vValue.ulVal )
                    {
                        bDown = FALSE;
                    }
                    else
                    {
                        bDown = TRUE;
                    }
                    break;

                case PID_CmdUnits:
                    switch( pProp->vValue.ulVal )
                    {
                    case PID_CmdPage:
                        lUnit = tomScreen;
                        break;
                        
                    case PID_CmdLine:
                        lUnit = tomLine;
                        break;

                    case PID_CmdParagraph:
                        lUnit = tomParagraph;
                        break;
                    }
                    break;

                case PID_CmdNumber:
                    // Calculate the number.
                    // See cmdmode.xml for details on how this grammar is constructed.
                    // It consists of an optional tens value followed by a ones value.
                    _ASSERTE( pProp->pFirstChild );
                    const SPPHRASEPROPERTY *pPropNum;
                    lCount = 0;
                    for ( pPropNum = pProp->pFirstChild; pPropNum; pPropNum = pPropNum->pNextSibling )
                    {
                        lCount += pPropNum->vValue.uiVal;
                    }
                    break;

                default:
                    _ASSERTE( FALSE );
                    break;
                }

                // Get the next property.  A 'NULL' value terminates our hunt
                pProp = pProp->pNextSibling;
            }

            // Now, actually navigate.
            if( bDown )
            {
                m_cpTextSel->MoveDown( lUnit, lCount, 0, NULL );
            }
            else
            {
                m_cpTextSel->MoveUp( lUnit, lCount, 0, NULL );
            }
        }
        break;

    case PID_CmdNavigationOther:
        // This handles the rule that covers the extraneous navigation
        // not explicitly handled in PID_CmdNavigation1
        if( pPhrase->pProperties )
        {
            switch( pPhrase->pProperties[0].vValue.ulVal )
            {
            case PID_CmdCharacterLeft:
                m_cpTextSel->MoveLeft( tomCharacter, 1, 0, NULL );
                break;
            case PID_CmdCharacterRight:
                m_cpTextSel->MoveRight( tomCharacter, 1, 0, NULL );
                break;
            case PID_CmdWordLeft:
                m_cpTextSel->MoveLeft( tomWord, 1, 0, NULL );
                break;
            case PID_CmdWordRight:
                m_cpTextSel->MoveRight( tomWord, 1, 0, NULL );
                break;

            case PID_CmdLineEnd:
                m_cpTextSel->EndKey( tomLine, 0, NULL );
                break;
            case PID_CmdLineHome:
                m_cpTextSel->HomeKey( tomLine, 0, NULL );
                break;
            case PID_CmdScrollEnd:
                m_cpTextSel->EndKey( tomStory, 0, NULL );
                break;
            case PID_CmdScrollHome:
                m_cpTextSel->HomeKey( tomStory, 0, NULL );
                break;
            }
        }
        else
        {
            _ASSERTE( pPhrase->pProperties );
        }
        break;

    default:
        // Default - just dump the command to the screen
        DumpCommandToScreen( m_hClient, rResult );
        break;
    }

    ::CoTaskMemFree( pPhrase );

    // Success
    return( true );

}   /* CDictationPad::ProcessCommandModeCommands */


/************************************************************************************
* CDictationPad::ProcessDictation() *
*-----------------------------------*
*   Description:
*       Processes recognized and hypothesized dictation.
*       Types the dictated text to where the selection was when the text was
*       dictated.  Inserts spaces on either side of the dictated text as
*       necessary (observing the display attributes)
*       
*   Return:
*       true iff successful
*************************************************************************************/
bool CDictationPad::ProcessDictation( ISpRecoResult &rResult, int eEventId ) 
{ 
    _ASSERTE(( eEventId == SPEI_RECOGNITION ) || ( eEventId == SPEI_HYPOTHESIS ));
    if ( eEventId == SPEI_HYPOTHESIS )
    {
        // A separate method to handle hypotheses
        return ProcessDictationHypothesis( rResult );
    }

    // Set the flag to indicate that the next insertion will be a dictation run
    // and so Dictpad does not need to process the next selection change
    m_dwFlags |= DP_SKIP_UPDATE;

    // Get the range to replace from the RecoEventManager.
    // In order to do this, we need to tell m_pRecoEventMgr when this
    // dictation was spoken.  
    // Why we need this: If the user moved the IP around while dictating
    // this phrase, we want to put the text where the IP was when he actually
    // dictated this phrase.
    SPRECORESULTTIMES times;
    rResult.GetResultTimes( &times );
    ITextRange *pRecoRange = NULL;
    HRESULT hr = m_pRecoEventMgr->Recognition( times.ftStreamTime, &pRecoRange ); 
    if ( FAILED( hr ) )
    {
        return false;
    }
    else if ( S_FALSE == hr )
    {
        // For whatever reason, this recognition was thrown out.
        // So there's nothing to display.
        return true;
    }
    else if ( !pRecoRange )
    {
        // unexpected error
        return false;
    }
    
    // Does the current selection overlap or adjoin the range in 
    // which the recognized text will go?
    bool bRecoAndSelDisjoint = AreDisjointRanges( pRecoRange, m_cpTextSel );
    
    ITextRange *pOldSel = NULL;
    if ( bRecoAndSelDisjoint )
    {
        // The recognized text will appear in an entirely different part of 
        // the document from the current selection.
        // Remember where the text selection was.
        // If this call fails, then it is not serious; the text selection
        // will simply not be restored to this location
        m_cpTextSel->GetDuplicate( &pOldSel );
    }

    // Set the text selection to be the range in which dictated
    // text should appear.
    // lStart and lEnd indicate where the dictation should go.
    long lStart, lEnd;
    pRecoRange->GetStart( &lStart );
    pRecoRange->GetEnd( &lEnd );
    m_cpTextSel->SetRange( lStart, lEnd );

    // Determine whether whatever is currently at lEnd wants leading spaces
    // (i.e. the spaces at the end of this newly-dictated text) consumed
    bool fConsumeLeadingSpaces;
    HRESULT hrConsumeSpaces = m_pTextRunList->IsConsumeLeadingSpaces( 
        lEnd, &fConsumeLeadingSpaces );

    // Get the text and display attributes from the result object
    CSpDynamicString dstrText;
    BYTE dwAttributes;
    hr = rResult.GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                                &dstrText, &dwAttributes );
    if ( FAILED( hr ) )
    {
        return false;
    }

    bool bRet = false;

    // Deal with the display attributes of the text: 
    
    // Determine how much space should follow the result text.
    // Space should not follow the text if leading spaces at lEnd are to
    // be consumed
    if ( !( SUCCEEDED( hrConsumeSpaces ) && fConsumeLeadingSpaces ) )
    {
        long lIsDelimiter = 
            ::SendMessage( m_hEdit, EM_FINDWORDBREAK, WB_ISDELIMITER, lEnd );
        if ( !lIsDelimiter )
        {
            // If the dictated text is put here, it will run into whatever
            // follows it.
            // Add as many trailing spaces as the attributes call for
            if ( dwAttributes & SPAF_ONE_TRAILING_SPACE )
            {
                dstrText.Append( L" " );
            }
            else if ( dwAttributes & SPAF_TWO_TRAILING_SPACES )
            {
                dstrText.Append( L"  " );
            }

        }
        else
        {
            // There is at least one space here.  If there isn't a second
            // space and the attributes call for two trailing spaces, 
            // add another space.
            if ( (dwAttributes & SPAF_TWO_TRAILING_SPACES) &&
                !::SendMessage( m_hEdit, EM_FINDWORDBREAK, WB_ISDELIMITER, lEnd+1 ) )
            {
                dstrText.Append( L" " );
            }
        }
    }
    
    // Determine how much space should precede the new text
    UINT uiSpacesNeeded = 0;
    bool fSpacePrepended = false;
    if ( lStart > 0 )
    {
        // This call determines how many spaces would be needed if
        // we inserted text at lStart.
        // If this call fails, we'll get the spacing wrong
        m_pTextRunList->HowManySpacesAfter( lStart, &uiSpacesNeeded );
        _ASSERTE( uiSpacesNeeded <= 2 );
    }
    if ( (dwAttributes & SPAF_CONSUME_LEADING_SPACES) && (lStart > 0) )
    {
        // This result requires that we consume leading space; 
        // move the start back until we have consumed all leading spaces
        
        // Create a degenerate range one space before
        ITextRange *pRange;
        hr = m_cpTextDoc->Range( lStart - 1, lStart - 1, &pRange );

        if ( SUCCEEDED( hr ) )
        {
            // Push the start and pRange back until the first character of 
            // pRange is no longer whitespace.
            long lChar = 0;
            pRange->GetChar( &lChar );
            while ( (lStart >= 0) && (L' ' == ((WCHAR) lChar )) )
            {
                lStart--;
                pRange->MoveStart( tomCharacter, -1, NULL );

                pRange->GetChar( &lChar );
            }

            // The character at lStart is now not space
            m_cpTextSel->SetStart( lStart );

            pRange->Release();
        }
    }
    else
    {
        // Prepend a space, since we must make sure
        // that this does not run into the text preceding it
        // as we are adding it
        fSpacePrepended = true;

        // Temporarily turn off the SkipUpdate flag in order to add a space
        // and have Dictpad handle the new space
        m_dwFlags &= ~DP_SKIP_UPDATE;

        // Insert a space at the beginning of the range.
        // We do this right now even if the space is not needed 
        // so that the dictated text is inserted intact (not running
        // into anything else).
        // If the space is not needed, we get rid of it below
        m_cpTextSel->SetText( L"" );
        BSTR bstrSpace = ::SysAllocString( L" " );
        hr = m_cpTextSel->TypeText( bstrSpace );
        ::SysFreeString( bstrSpace );

        // Restore the SkipUpdate flag
        m_dwFlags |= DP_SKIP_UPDATE;
    }
    if ( FAILED( hr ) )
    {
        return false;
    }

    // lWhereNewTextBegins is the beginning of the new text (not the space
    // that may precede it)
    const long lWhereNewTextBegins = lStart; 

    // Get the text of the reco result
    BSTR bstrText = ::SysAllocString( dstrText );
    if ( !bstrText )
    {
        return false;
    }

    // Create a dictation run to contain the new text
    CDictationRun *pRun = new CDictationRun();
    if ( !pRun )
    {
        return false;
    }
    hr = pRun->Initialize( rResult );
    if ( FAILED( hr ) )
    {
        return false;
    }

    // Type the text into the document
    m_cpTextSel->SetText( L"" );
    hr = m_cpTextSel->TypeText( bstrText );
    ::SysFreeString( bstrText );

    // Get the dictated range (it ends wherever the text selection ends now), 
    // and give it to the new dictation run
    long lDictRunEnd;
    m_cpTextSel->GetEnd( &lDictRunEnd );
    ITextRange *pDictRunRange;
    hr = m_cpTextDoc->Range( lWhereNewTextBegins, lDictRunEnd, &pDictRunRange );
    if ( SUCCEEDED( hr ) )
    {
        hr = pRun->SetTextRange( pDictRunRange );
    }
    if ( FAILED( hr ) )
    {
        return false;
    }

    // Set the font back to normal (in case hypotheses had changed the font
    CComPtr<ITextFont> cpFont;
    hr = pDictRunRange->GetFont( &cpFont );
    if ( SUCCEEDED( hr ) )
    {
        cpFont->SetForeColor( tomAutoColor );
        pDictRunRange->SetFont( cpFont );
    }

    // Release pDictRunRange, since this range is now the run's responsibility
    pDictRunRange->Release();

    // Now that the range is set, can insert it into the TextRunList.
    hr = m_pTextRunList->Insert( pRun );

    if ( SUCCEEDED( hr ) )
    {
        if (( 0 == uiSpacesNeeded ) && fSpacePrepended )
        {
            // Need to get rid of this space, since we prepended a 
            // space but no space is needed.
            // Dictpad should handle the deletion of this space.
            // If the deletion fails, the spacing will be wrong.
            m_cpTextSel->SetRange( lWhereNewTextBegins, lWhereNewTextBegins );
            m_dwFlags &= ~DP_SKIP_UPDATE;
            m_cpTextSel->Delete( tomCharacter, 1, NULL );
            m_dwFlags |= DP_SKIP_UPDATE;

            // Restore the selection, realizing that it is going to be
            // one behind because of the deletion
            m_cpTextSel->SetRange( lDictRunEnd - 1, lDictRunEnd - 1 );
        }
        else if (( uiSpacesNeeded > 0 ) && 
            !(dwAttributes & SPAF_CONSUME_LEADING_SPACES) ) // SPAF_CONSUME_LEADING_SPACES
                                                            // trumps the trailing spaces
                                                            // attribs of previous runs
        {
            // Determine how many more spaces are needed, since we
            // may have already taken care of it if we already prepended
            // a space
            INT iAdditionalSpacesNeeded = uiSpacesNeeded - (fSpacePrepended ? 1 : 0);
            _ASSERTE( (iAdditionalSpacesNeeded >= 0) 
                && (iAdditionalSpacesNeeded <= 2) );

            if ( iAdditionalSpacesNeeded )
            {
                // Type in those spaces where the text begins.
                // Dictpad should handle the insertion of this space.
                // If the insertion fails, the spacing will be wrong
                m_cpTextSel->SetRange( lWhereNewTextBegins, lWhereNewTextBegins );
                m_dwFlags &= ~DP_SKIP_UPDATE;
                BSTR bstrSpaces = ::SysAllocString( 
                    (1 == iAdditionalSpacesNeeded) ? L" " : L"  " );
                m_cpTextSel->TypeText( bstrSpaces );
                ::SysFreeString( bstrSpaces );
                m_dwFlags |= DP_SKIP_UPDATE;

                // Restore the selection, realizing that it's going to be further along
                // than it was because of the spaces
                m_cpTextSel->SetRange( lDictRunEnd + iAdditionalSpacesNeeded, 
                    lDictRunEnd + iAdditionalSpacesNeeded );
            }
        }
    }

    // Success
    bRet = SUCCEEDED(hr);

    // If the recognized text was placed into a range that overlaps or abuts
    // the current selection, then the selection will have been moved to an IP
    // at the end of the recognized text.
    // If the recognized text and the original text selection were disjoint, then
    // move the selection back to the old selection.
    if ( bRecoAndSelDisjoint )
    {
        // Restore the old selection
        pOldSel->GetStart( &lStart );
        pOldSel->GetEnd( &lEnd );
        m_cpTextSel->SetRange( lStart, lEnd );

        pOldSel->Release();
    }

    if ( m_pCandidateList )
    {
        // Show the alternates button and update it with the changed TextRunList
        m_pCandidateList->ShowButton( true );
        m_hAltsButton = m_pCandidateList->Update( m_pTextRunList );
    }

    // This will unleash any WM_COMMANDs that were waiting
    m_pRecoEventMgr->DoneProcessingPhrase();

    // We're done, and Dictpad should now be processing updates again.
    m_dwFlags &= ~DP_SKIP_UPDATE;

    return bRet;

} /* CDictationPad::ProcessDictation */

/***************************************************************************************
* CDictationPad::ProcessDictationHypothesis *
*-------------------------------------------*
*   Description:
*       Called when a hypothesis notification is received.
*       Puts the text in the appropriate location
*   Return:
*       true iff successful
****************************************************************************************/
bool CDictationPad::ProcessDictationHypothesis( ISpRecoResult &rResult ) 
{ 
    // Set the flag to indicate that the next insertion will be a dictation run
    m_dwFlags |= DP_SKIP_UPDATE;

    // Get the text
    CSpDynamicString dstrText;
    HRESULT hr = rResult.GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                                &dstrText, NULL );
    if ( FAILED( hr ) )
    {
        return false;
    }

    // Get the range to replace from the RecoEventManager.
    // Just like in CDictationPad::ProcessDictation() above, when the 
    // utterance was occurred determines where it will go.
    SPRECORESULTTIMES times;
    rResult.GetResultTimes( &times );
    ITextRange *pRecoRange = m_pRecoEventMgr->Hypothesis( times.ftStreamTime );
    if ( !pRecoRange )
    {
        // For whatever reason the hypothesis was dropped, and nothing needs
        // to be done with it.
        return true;
    }

    // Put the text in the range.
    // If this fails, then we just won't see the hypothesis.
    BSTR bstrText = ::SysAllocString( dstrText );
    pRecoRange->SetText( bstrText );
    ::SysFreeString( bstrText );
    
    // Check if the selection now interferes with some non-editable range.
    // (A range is not editable if it currently contains hypotheses)
    // If so, move it to the end of that range
    CComPtr<ITextRange> cpNextEditableRange;
    if ( !(m_pRecoEventMgr->IsEditable( m_cpTextSel, &cpNextEditableRange )) )
    {
        if ( !cpNextEditableRange )
        {
            // This indicates an out-of-memory error condition
            // (see recomgr.cpp)
            return false;
        }

        // Move the selection to the next editable phrase
        long lStart, lEnd;
        cpNextEditableRange->GetStart( &lStart );
        cpNextEditableRange->GetEnd( &lEnd );
        m_cpTextSel->SetStart( lStart );
        m_cpTextSel->SetEnd( lEnd );
    }

    // Make the hypothesized text gray
    CComPtr<ITextFont> cpFont;
    pRecoRange->GetFont( &cpFont );
    if ( cpFont )
    {
        cpFont->SetForeColor( PALETTERGB( 128, 128, 128 ) );
        pRecoRange->SetFont( cpFont );
    }

    // Dictpad should resume processing selection changes
    m_dwFlags &= ~DP_SKIP_UPDATE;
    
    return SUCCEEDED( hr );
} /* CDictationPad::ProcessDictationHypothesis */

/****************************************************************************************
* CDictationPad::SetMode() *
*--------------------------*
*   Description:
*       Switches to dictation or to command mode (depending 
*       on the value of fDictationMode).
*       Updates the toolbar/statusbar accordingly.
*       Switching between dictation and command mode involves
*       activating and deactivating grammars
*   Return:
*       S_OK
*       Return value of CDictationPad::SetMode()
*****************************************************************************************/
HRESULT CDictationPad::SetMode( bool fDictationMode )
{
    bool fAlreadyInDictMode = m_dwFlags & DP_DICTATION_MODE;
    if ( fAlreadyInDictMode != fDictationMode )
    {
        // Request to change the mode

        // Deactivate any currently-active grammars
        HRESULT hr = SetGrammarState( false );

        if ( SUCCEEDED( hr ) )
        {
            // Flip the mode flag
            m_dwFlags ^= DP_DICTATION_MODE; 
            
            // Since the flags are now set to the mode that the
            // user wanted to switch to, this will now activate the
            // rules in the correct grammar (if the user had the
            // mic on when he switched grammars
            hr = SetGrammarState( m_dwFlags & DP_MICROPHONE_ON );
        }

        _ASSERTE(SUCCEEDED(hr));
        if ( FAILED(hr) )
        {
            return hr;
        }
    }


    // Keep the toolbar button in sync with the current mode
    ::SendMessage( m_hToolBar, TB_PRESSBUTTON, IDM_DICTATION_MODE, MAKELONG( fDictationMode, 0 ) );
    ::SendMessage( m_hToolBar, TB_PRESSBUTTON, IDM_COMMAND_MODE, MAKELONG( !fDictationMode, 0 ) );
    
    // Keep the menu items in sync with the current mode
    HMENU hMenu = ::GetMenu( m_hClient );
    ::CheckMenuItem( hMenu, IDM_DICTATION_MODE, fDictationMode ? MF_CHECKED : MF_UNCHECKED );
    ::CheckMenuItem( hMenu, IDM_COMMAND_MODE, fDictationMode ? MF_UNCHECKED : MF_CHECKED );
    
    // Keep the status bar in sync with the current mode
    {
        USES_CONVERSION;
        CSpDynamicString dstr;
        dstr = fDictationMode ? L"Dictation mode" : L"Command mode";
        ::SendMessage( m_hStatusBar, SB_SETTEXT, 0, (LPARAM)W2T( dstr ) );
    }

    return S_OK;
}   /* CDictationPad::SetMode */

/****************************************************************************************
* CDictationPad::SetGrammarState() *
*--------------------------------*
*   Description:
*       Sets the rules in the relevant grammars to the desired state
*       (active or inactive).
*       If the edit window does not have the input focus and we are
*       turning the grammars on, set the input focus to the edit
*       window.
*       Sets the microphone button to reflect whether the rules are active
*   Return:
*       Return value of ISpRecognizer::SetRecoState()
*       Return value of ISpRecoGrammar::SetRuleState()
*       Return value of ISpRecoGrammar::SetDictationState()
*****************************************************************************************/
HRESULT CDictationPad::SetGrammarState( BOOL bOn )
{
    // Check the reco state, to make sure that the reco state is active.
    // If not, activate it.
    if ( bOn )
    {
        SPRECOSTATE rs;
        HRESULT hrRecoState = m_cpRecoEngine->GetRecoState( &rs );
        if ( SUCCEEDED( hrRecoState ) && (SPRST_INACTIVE == rs) )
        {
            // Set the reco state to active
            hrRecoState = m_cpRecoEngine->SetRecoState( SPRST_ACTIVE );
        }

        if ( FAILED( hrRecoState ) )
        {
            return hrRecoState;
        }
    }

    // Check to make sure that we're actually making a change
    const BOOL fGrammarsActive = m_dwFlags & DP_GRAMMARS_ACTIVE;
    if ( fGrammarsActive == bOn )
    {
        // Asking for the current state; nothing needs to be done
        return S_OK;
    }

#ifdef _DEBUG
    TCHAR debugstring[100];
    wsprintf( debugstring, _T("Trying to %s grammars..."), bOn ? _T("activate") : _T("deactivate") );
    OutputDebugString( debugstring );
#endif

    HRESULT hr = S_OK;
    if ( m_dwFlags & DP_DICTATION_MODE )
    {
        // We are in dictation mode.

        if ( !m_cpDictCCGrammar || !m_cpDictGrammar )
        {
            return E_FAIL;
        }

        // Dictation and the dictation-mode command rules should be (in)active
        hr = m_cpDictCCGrammar->SetRuleState(NULL, NULL, bOn ? SPRS_ACTIVE : SPRS_INACTIVE);
        if ( SUCCEEDED( hr ) )
        {
            hr = m_cpDictGrammar->SetDictationState(bOn ? SPRS_ACTIVE : SPRS_INACTIVE);
        }
    }
    else
    {
        // We are in command mode

        if ( !m_cpCCGrammar )
        {
            return E_FAIL;
        }
        // Command-mode command rules should be (in)active
        hr = m_cpCCGrammar->SetRuleState(NULL, NULL, bOn ? SPRS_ACTIVE : SPRS_INACTIVE);
    }

    if ( SUCCEEDED( hr ) )
    {
        // Grammars successfully (de)activated, set the flag
        bOn ? ( m_dwFlags |= DP_GRAMMARS_ACTIVE ) 
            : ( m_dwFlags &= ~DP_GRAMMARS_ACTIVE );
#ifdef _DEBUG
        OutputDebugString( _T("success\r\n") );
#endif
    }
    else
    {
        return hr;
    }

    // Update the menu items
    HMENU hMenu = ::GetMenu( m_hClient );
    ::CheckMenuItem( hMenu, IDM_MIC_TOGGLE, bOn ? MF_CHECKED : MF_UNCHECKED );

    // Update the toolbar button
    long lButtonState;
    if ( bOn )
    {
        if ( DP_IS_SPEAKING & m_dwFlags )
        {
            lButtonState = MAKELONG( TBSTATE_PRESSED, 0 );
        }
        else
        {
            lButtonState = MAKELONG( TBSTATE_ENABLED | TBSTATE_PRESSED, 0 );
        }
    }
    else
    {
        if ( DP_IS_SPEAKING & m_dwFlags )
        {
            lButtonState = MAKELONG( 0, 0 );
        }
        else
        {
            lButtonState = MAKELONG( TBSTATE_ENABLED, 0 );
        }
    }
    ::SendMessage( m_hToolBar, TB_SETSTATE, IDM_MIC_TOGGLE, lButtonState );
    
    if ( bOn && ( ::GetFocus() != m_hEdit ) )
    {
        // Asking to turn the microphone on, but the edit window does not have
        // the input focus: Set the input focus to the edit window 
        ::SetFocus( m_hEdit );
    }

    return S_OK;
}   /* CDictationPad::SetGrammarState() */

/***************************************************************************************
* CDictationPad::RunAddDeleteUI *
*-------------------------------*
*   Description:
*       Gets the first word in the current selection, if the selection
*       contains any words.
*       Displays the UI for adding and deleting words with the word as 
*       a parameter.
*   Return:
*       S_OK
*       Return value of CDictationPad::SetGrammarState()
*       Return value of ISpRecognizer::DisplayUI()
****************************************************************************************/
HRESULT CDictationPad::RunAddDeleteUI()
{
    HRESULT hr;
    
    // Stop listening
    hr = SetGrammarState( false );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    // Get the word nearest the beginning of the selection
    // If these calls fail, we just won't get the word.

    CComPtr<ITextRange> cpFirstWordRange;
    BSTR bstrFirstWordSelected = NULL;
    long lStart = m_pTextRunList->GetTailEnd();
    hr = m_cpTextSel->GetDuplicate( &cpFirstWordRange );
    if ( SUCCEEDED(hr) )
    {
        hr = cpFirstWordRange->Collapse( true );
        cpFirstWordRange->GetStart( &lStart );
    }
    if ( SUCCEEDED(hr) )
    {
        hr = cpFirstWordRange->Expand( tomWord, NULL );
    }
    // If lStart is equal to m_pTextRunList, we don't get useful
    // text in bstrFirstWordSelected
    if ( SUCCEEDED(hr) && (lStart < m_pTextRunList->GetTailEnd()) )
    {
        hr = cpFirstWordRange->GetText( &bstrFirstWordSelected );
    }
    
    WCHAR *pwszNewWord = NULL;
    if ( bstrFirstWordSelected )
    {
        pwszNewWord = wcsdup( bstrFirstWordSelected );
        ::SysFreeString( bstrFirstWordSelected );
    }

    ULONG ulDataSize = pwszNewWord ? 
        (sizeof(WCHAR) * wcslen( pwszNewWord )) : 0;

    USES_CONVERSION;
    hr = m_cpRecoEngine->DisplayUI( 
        m_hClient, NULL, SPDUI_AddRemoveWord, pwszNewWord, ulDataSize );
        
    if ( pwszNewWord )
    {
        free(pwszNewWord);
    }

    return hr;
}   /* CDictationPad::RunAddDeleteUI */



/******************************************************************************
* HighlightAndBringIntoView *
*---------------------------*
*   Description:
*       Highlight the given text in the document and bring it into view
*   Return:
*       none.  If this function fails, the text tracking will be
*       wrong, which is not serious.
******************************************************************************/
void HighlightAndBringIntoView( ITextDocument &rTextDoc, long lStart, long lEnd )
{
    CComPtr<ITextRange> cpWordRange;
    HRESULT hr = rTextDoc.Range( lStart, lEnd, &cpWordRange );

    CComPtr<ITextFont> cpHighlightFont;
    if ( SUCCEEDED( hr ) )
    {
        hr = cpWordRange->GetFont( &cpHighlightFont );
    }
    if ( SUCCEEDED( hr ) )
    {
        cpHighlightFont->SetBackColor( PALETTERGB( 255, 255, 0 ) );
        cpWordRange->SetFont( cpHighlightFont );
    }

    // Bring the most recently-spoken text into view
    POINT pt;
    hr = cpWordRange->GetPoint( tomEnd | TA_BOTTOM | TA_RIGHT, 
        &(pt.x), &(pt.y) );
    if ( hr == S_FALSE )
    {
        // An S_FALSE return value from ITextRange::GetPoint() means that 
        // the requested point is not visible
        cpWordRange->ScrollIntoView( tomEnd );
    }
}   /* HighlightAndBringIntoView */

/******************************************************************************
* DumpCommandToScreen *
*---------------------*
*   Description:
*       Dumps the command whose spoken text is in the phrase object
*       rPhrase to the screen
*   Return:
*       none.  If this function fails, the command will not be dumped.
******************************************************************************/
void DumpCommandToScreen( HWND hwndClient, ISpPhrase &rPhrase )
{
    // Get the rule name
    SPPHRASE *pPhrase = NULL;
    HRESULT hr = rPhrase.GetPhrase( &pPhrase );
    if ( FAILED( hr ) )
    {
        return;
    }
    CSpDynamicString dstr = L"Rule: \"";
    if( pPhrase->Rule.pszName )
    {
        dstr.Append( pPhrase->Rule.pszName );
    }
    ::CoTaskMemFree( pPhrase );

    // Now get the text
    WCHAR *pwszSpokenText = NULL;
    BYTE b;
    hr = rPhrase.GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, true,
        &pwszSpokenText, &b );
    if ( FAILED( hr ) )
    {
        return;
    }
    dstr.Append( L"\"\nSpoken Text: \"" );
    _ASSERTE( pwszSpokenText );
    if( pwszSpokenText )
    {
        dstr.Append( pwszSpokenText );
    }
    dstr.Append( L"\"" );
    ::CoTaskMemFree( pwszSpokenText );

    USES_CONVERSION;
    ::MessageBox( hwndClient, W2T( dstr ), _T("Command Mode"), MB_OK );

}   /* DumpCommandToScreen */

/******************************************************************************
* DEBUG CODE *
******************************************************************************/
#ifdef _DEBUG
const char * apszTtsEventNames[] =
{
    "SPEI_START_INPUT_STREAM", //  = 1
    "SPEI_END_INPUT_STREAM", //    = 2
    "SPEI_VOICE_CHANGE", //        = 3
    "SPEI_BOOKMARK", //            = 4
    "SPEI_WORDBOUNDARY", //        = 5
    "SPEI_PHONEME", //             = 6
    "SPEI_SENTENCEBOUNDARY", //    = 7
    "SPEI_VISEME", //              = 8
    "SPEI_TTS_UNDEFINED_0", //     = 9
    "SPEI_TTS_UNDEFINED_1", //     = 10
    "SPEI_TTS_UNDEFINED_2", //     = 11
    "SPEI_TTS_UNDEFINED_3", //     = 12
    "SPEI_TTS_UNDEFINED_4", //     = 13
    "SPEI_TTS_UNDEFINED_5", //     = 14
    "SPEI_TTS_UNDEFINED_6", //     = 15
};

const char * apszSrEventNames[] =
{
    "SPEI_END_SR_STREAM", //       = 34
    "SPEI_SOUNDSTART", //          = 35
    "SPEI_SOUNDEND", //            = 36
    "SPEI_PHRASESTART", //         = 37
    "SPEI_RECOGNITION", //         = 38
    "SPEI_HYPOTHESIS", //          = 39
    "SPEI_ATTRIBCHANGED", //       = 40
    "SPEI_SR_BOOKMARK", //         = 41
    "SPEI_ASYNC_COMPLETED", //     = 42
    "SPEI_FALSERECOGNITION", //    = 43
    "SPEI_INTERFERENCE", //        = 44
    "SPEI_REQUESTUI", //           = 45
    "SPEI_RECO_STATE_CHANGE", //   = 46
    "SPEI_SR_UNDEFINED_1", //      = 47
};

void DumpEventName(int id)
{
    SPDBG_FUNC( "DictPad - DumpEventName" );

    const char * pszEventName;

    if (id >= SPEI_MIN_SR && id <= SPEI_MAX_SR)
    {
        pszEventName = apszSrEventNames[id - SPEI_MIN_SR];
    }
    else if (id >= SPEI_MIN_TTS && id <= SPEI_MAX_TTS)
    {
        pszEventName = apszTtsEventNames[id - SPEI_MIN_TTS];
    }
    else
    {
        pszEventName = NULL;
    }

    if (pszEventName)
    {
        SPDBG_DMSG1("dictpad: event = %s\r\n", pszEventName);
    }
    else
    {
        SPDBG_DMSG1("dictpad: event = #%d\r\n", id);
    }
}
#endif
