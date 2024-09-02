//////////////////////////////////////////////////////////
// SIMPLETELEPHONY.EXE
//
// Sample application that handles incoming TAPI calls
// and does some speech recognition and text-to-speech.
//
// In order to receive incoming calls, the application must
// implement and register the outgoing ITCallNotification
// interface.
//
// This application will register to receive calls on
// all addresses that support at least the audio media type.
//
// NOTE:  This application is limited to working with one
// call at at time, and will not work correctly if multiple
// calls are present at the same time.
//
// NOTE: The call-handling sequence in this application is
// short, so it is fine to make it all the way through to
// the end of the call even if the caller hung up in the 
// middle.  If your telephony app involves a lengthy 
// call-handling sequence, you need a separate thread to
// listen for TAPI's CS_DISCONNECT message; otherwise your
// app will have no way of being notified of the hang-up,
// and your app will appear to have hung.
//////////////////////////////////////////////////////////

/****************************************************************************
*	Operator.cpp
*		Implementation of the COperator class for the simpletelephony
*       project.
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/

#include <windows.h>
#include <atlbase.h>
#include <sapi.h>
#include <sperror.h>
#include <sphelper.h>
#include <tapi3.h>      // In general, you need to have the
                        // Microsoft Platform SDK installed
                        // in order to have this file
#include "operator.h"
#include "resource.h"

#define CALLER_TIMEOUT  8000    // Wait 8 seconds before giving up on a caller

/**********************************************************
* WinMain *
*---------*
*   Description:
*       Main entry point for the application
***********************************************************/
int WINAPI WinMain(
        HINSTANCE hInst,
        HINSTANCE hPrevInst,
        LPSTR lpCmdLine,
        int nCmdShow
       )
{
    // Initialize COM.
    if (FAILED(::CoInitialize( NULL )))
    {
        return 0;
    }

    // Create an instance of our main call-handling class
    COperator *pOperator = new COperator( hInst );
    if ( !pOperator )
    {
        return 0;
    }

    // Does initialization of SAPI and TAPI objects
    HRESULT hr = pOperator->Initialize();
    if ( FAILED( hr ) )
    {
        return 0;
    }

    // everything is initialized, so
    // start the main dialog box
    ::DialogBoxParam( hInst,
              MAKEINTRESOURCE(IDD_MAINDLG),
              NULL,
              MainDialogProc,
              (LPARAM) pOperator
             );

    // This does the cleanup of SAPI and TAPI objects
    delete pOperator;

    ::CoUninitialize();

    return 1;
}

/**********************************************************
* COperator::~COperator *
*-----------------------*
*   Description:
*       Destructor for the COperator class.
*       Cleans up SAPI and TAPI objects.
***********************************************************/
COperator::~COperator()
{
    ShutdownSapi();
    ShutdownTapi();
}   /* COperator::~COperator */

/**********************************************************
* COperator::Initialize *
*-----------------------*
*   Description:
*       Initializes SAPI and TAPI
*   Return:
*       S_OK if both SAPI and TAPI initialized successfully
***********************************************************/
HRESULT COperator::Initialize()
{
    HRESULT hr = InitializeSapi();
    if ( SUCCEEDED( hr ) )
    {
        hr = InitializeTapi();
    }
    return hr;
}   /* COperator::Initialize */

/**********************************************************
* COperator::InitializeSapi *
*---------------------------*
*   Description:
*       Various SAPI initializations.
*   Return:
*       S_OK if SAPI initialized successfully
*       Return values of failed SAPI initialization 
*           functions
***********************************************************/
HRESULT COperator::InitializeSapi()
{
    // Create a voice for speaking on this machine
    HRESULT hr = m_cpLocalVoice.CoCreateInstance( CLSID_SpVoice );
    if ( FAILED( hr ) )
    {
        DoMessage( L"Could not create a TTS voice on the local machine" );
        return hr;
    }

    // Create a reco engine for recognizing speech over the phone
    // This is an inproc recognizer since it will likely be
    // using a format other than the default
    hr = m_cpIncomingRecognizer.CoCreateInstance( CLSID_SpInprocRecognizer );
    if ( FAILED(hr) )
    {
        DoMessage(L"CoCreateInstance on inproc reco engine failed");
        return hr;
    }

    // Create a reco context for this engine
    hr = m_cpIncomingRecognizer->CreateRecoContext( &m_cpIncomingRecoCtxt );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not create recognition context");
        return hr;
    }

    // Set interest only in PHRASE_START, RECOGNITION, FALSE_RECOGNITION
    const ULONGLONG ullInterest = SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) |
                                  SPFEI(SPEI_FALSE_RECOGNITION);
    hr = m_cpIncomingRecoCtxt->SetInterest( ullInterest, ullInterest );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not set interest in SAPI events");
        return hr;
    }

    // Retain recognized audio
    hr = m_cpIncomingRecoCtxt->SetAudioOptions( SPAO_RETAIN_AUDIO, NULL, NULL );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not set audio options to retain recognized audio");
        return hr;
    }

    // Create a dictation grammar and load it
    hr = m_cpIncomingRecoCtxt->CreateGrammar( 0, &m_cpDictGrammar );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not create dictation grammar");
        return hr;
    }

    hr = m_cpDictGrammar->LoadDictation( NULL, SPLO_STATIC );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not load dictation");
        return hr;
    }

    // Create a voice for talking on the phone.
    hr = m_cpIncomingRecoCtxt->GetVoice( &m_cpOutgoingVoice );
    if ( FAILED(hr) )
    {
        DoMessage(L"Could not create a TTS voice for speaking over the phone");
        return hr;
    }

    return S_OK;
}   /* COperator::InitializeSapi */
    

/**********************************************************
* COperator::InitializeTapi *
*---------------------------*
*   Description:
*       Various TAPI initializations
*   Return:
*       S_OK iff TAPI initialized successfully
*       Return values of failed TAPI initialization 
*           functions
***********************************************************/
HRESULT COperator::InitializeTapi()
{
    // Cocreate the TAPI object
    HRESULT hr = CoCreateInstance(
                          CLSID_TAPI,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITTAPI,
                          (LPVOID *)&m_pTapi
                         );

    if ( FAILED(hr) )
    {
        DoMessage(L"CoCreateInstance on TAPI failed");
        return hr;
    }

    // call ITTAPI::Initialize().  this must be called before
    // any other tapi functions are called.
    hr = m_pTapi->Initialize();

    if ( FAILED(hr) )
    {
        DoMessage(L"TAPI failed to initialize");
        m_pTapi->Release();
        m_pTapi = NULL;
        return hr;
    }

    // Create our own event notification object and register it
    // See callnot.h and callnot.cpp
    m_pTAPIEventNotification = new CTAPIEventNotification;
    if ( NULL == m_pTAPIEventNotification )
    {
        return E_OUTOFMEMORY;
    }
    hr = m_pTAPIEventNotification->Initialize();
    
    if ( SUCCEEDED( hr ) )
    {
        hr = RegisterTapiEventInterface();
    }

    // Set the Event filter to only give us only the events we process
    if ( SUCCEEDED( hr ) )
    {
        hr = m_pTapi->put_EventFilter(TE_CALLNOTIFICATION | TE_CALLSTATE);
    }
    
    if ( FAILED( hr ) )
    {
        DoMessage( L"Could not set up TAPI event notifications" );
        return hr;
    }
    

    // Find all address objects that we will use to listen for calls on
    hr = ListenOnAddresses();

    if ( FAILED(hr) )
    {
        DoMessage(L"Could not find any addresses to listen on");

        m_pTapi->Release();
        m_pTapi = NULL;

        return hr;
    }

    return S_OK;
}   /* COperator::InitializeTapi */

/**********************************************************
* COperator::ShutdownSapi *
*-------------------------*
*   Description:
*       Releases all SAPI objects
***********************************************************/
void COperator::ShutdownSapi()
{
    m_cpMMSysAudioIn.Release();
    m_cpMMSysAudioOut.Release();
    m_cpDictGrammar.Release();
    m_cpIncomingRecoCtxt.Release();
    m_cpIncomingRecognizer.Release();
    m_cpLocalVoice.Release();
    m_cpOutgoingVoice.Release();
}   /* COperator::ShutdownSapi */

/**********************************************************
* COperator::ShutdownTapi *
*-------------------------*
*   Description:
*       Releases all TAPI objects
***********************************************************/
void COperator::ShutdownTapi()
{
    // if there is still a call, release it
    if (NULL != m_pCall)
    {
        m_pCall->Release();
        m_pCall = NULL;
    }

    // release main object.
    if (NULL != m_pTapi)
    {
        m_pTapi->Shutdown();
        m_pTapi->Release();
    }
}   /* COperator::ShutdownTapi */


/**********************************************************
* MainDialogProc *
*----------------*
*   Description:
*       Main dialog proc for this sample
***********************************************************/
BOOL CALLBACK MainDialogProc(
               HWND hDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam
              )
{
    // This is set to be the window long in WM_INITDIALOG
    COperator *pThis = (COperator *) ::GetWindowLong( hDlg, GWL_USERDATA );

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Get the pointer to the COperator object from the lParam.
            // Store it in the window long as user data
            pThis = (COperator *) lParam;
            ::SetWindowLong( hDlg, GWL_USERDATA, (LPARAM) pThis );

            // Hand the window handle to the event notification object
            // so that this window will get window messages about incoming calls
            pThis->m_pTAPIEventNotification->SetHWND( hDlg );
            
            // Get the window handle
            pThis->m_hDlg = hDlg;
            
            // Wait for a call
            ::EnableWindow( ::GetDlgItem( hDlg, IDC_ANSWER ), FALSE );
            pThis->SetStatusMessage( L"Waiting for a call..." );

            return 0;
        }

        case WM_PRIVATETAPIEVENT:
        {
            // This message is received whenever a TAPI event occurs
            pThis->OnTapiEvent( (TAPI_EVENT) wParam,
                        (IDispatch *) lParam );

            return 0;
        }

        case WM_COMMAND:
        {
            if ( LOWORD(wParam) == IDCANCEL )
            {
                // Quit
                EndDialog( hDlg, 0 );

                return 1;
            }

            switch ( LOWORD(wParam) )
            {
                case IDC_AUTOANSWER:
                {
                    // Auto answer check box state was changed
                    pThis->m_fAutoAnswer = !pThis->m_fAutoAnswer;
                    return 1;
                }

                case IDC_ANSWER:
                {
                    // Answer the call

                    pThis->SetStatusMessage(L"Answering...");

                    if ( S_OK == pThis->AnswerTheCall() )
                    {
                        pThis->SetStatusMessage(L"Connected");

                        ::EnableWindow( ::GetDlgItem( hDlg, IDC_ANSWER ), FALSE );

                        // Connected: Talk to the caller

                        // PLEASE NOTE: This is a single-threaded app, so if the caller
                        // hangs up after the call-handling sequence has started, the 
                        // app will not be notified until after the entire call sequence 
                        // has finished.  
                        // If you want to be able to cut the call-handling short because
                        // the caller hung up, you need to have a separate thread listening
                        // for  TAPI's CS_DISCONNECT notification.
                        HRESULT hrHandleCall = pThis->HandleCall();
                        if ( FAILED( hrHandleCall ) )
                        {
                            if ( TAPI_E_DROPPED == hrHandleCall )
                            {
                                pThis->SetStatusMessage( L"Caller hung up prematurely" );
                            }
                            else
                            {
                                pThis->DoMessage( L"Error encountered handling the call" );
                            }
                        }

                        // Hang up if we still need to
                        if ( NULL != pThis->m_pCall )
                        {
                            // The caller is still around; hang up on him
                            pThis->SetStatusMessage(L"Disconnecting...");
                            if (S_OK != pThis->DisconnectTheCall())
                            {
                                pThis->DoMessage(L"Disconnect failed");
                            }
                        }
                    }
                    else
                    {
                        ::EnableWindow( ::GetDlgItem( hDlg, IDC_ANSWER ), FALSE );
                        pThis->DoMessage(L"Answer failed");
                    }

                    // Waiting for the next call...
                    pThis->SetStatusMessage(L"Waiting for a call...");

                    return 1;
                }

                case IDC_DISCONNECTED:
                {
                    // This message is sent from OnTapiEvent()
                    // Disconnected notification -- release the call
                    pThis->ReleaseTheCall();

                    ::EnableWindow( ::GetDlgItem( hDlg, IDC_ANSWER ), FALSE );

                    pThis->SetStatusMessage(L"Waiting for a call...");
                    
                    return 1;
                }
                default:

                    return 0;
            }
        }
        default:

            return 0;
    }
}   /* MainDialogProc */

///////////////////////////////////////////////////////////
// TAPI actions
///////////////////////////////////////////////////////////

/**********************************************************
* COperator::RegisterTapiEventInterface *
*---------------------------------------*
*   Description:
*       Get a unique identifier (m_ulAdvice) for 
*       this connection point.
*   Return:
*       S_OK
*       Failed HRESULTs from QI(), 
*           IConnectionPointContainer::FindConnectionPoint(),
*           or IConnectionPoint::Advise()
***********************************************************/
HRESULT COperator::RegisterTapiEventInterface()
{
    HRESULT                       hr = S_OK;
    IConnectionPointContainer   * pCPC;
    IConnectionPoint            * pCP;
    

    hr = m_pTapi->QueryInterface( IID_IConnectionPointContainer,
                                (void **)&pCPC );

    if ( SUCCEEDED( hr ) )
    {
        hr = pCPC->FindConnectionPoint( IID_ITTAPIEventNotification,
                                       &pCP );
        pCPC->Release();
    }
        
    if ( SUCCEEDED( hr ) )
    {
        hr = pCP->Advise( m_pTAPIEventNotification,
                          &m_ulAdvise );

        pCP->Release();
    }
    return hr;
}   /* COperator::RegisterTapiEventInterface */

/**********************************************************
* COperator::ListenOnAddresses *
*------------------------------*
*   Description:
*       Find all addresses that support audio in and audio
*       out.  Call ListenOnThisAddress to start listening
*   Return:
*       S_OK
*       S_FALSE if there are no addresses to listen on
*       Failed HRESULT ITTAPI::EnumberateAddresses() 
*           or IEnumAddress::Next()
************************************************************/
HRESULT COperator::ListenOnAddresses()
{
    // enumerate the addresses
    IEnumAddress *      pEnumAddress;
    HRESULT hr = m_pTapi->EnumerateAddresses( &pEnumAddress );

    if ( FAILED(hr) )
    {
        return hr;
    }

    ITAddress *         pAddress;
    bool                fAddressExists = false;
    while ( true )
    {
        // get the next address
        hr = pEnumAddress->Next( 1, &pAddress, NULL );
        if (S_OK != hr)
        {
            // Done dealing with all the addresses
            break;
        }

        // Does the address support audio?
        if ( AddressSupportsMediaType(pAddress, TAPIMEDIATYPE_AUDIO) )
        {
            // If it does then we'll listen.
            HRESULT hrListen = ListenOnThisAddress( pAddress );
            
            if ( S_OK == hrListen )
            {
                fAddressExists = true;
            }
        }
        pAddress->Release();

    }
    pEnumAddress->Release();

    if ( !fAddressExists )
    {
        DoMessage( L"Could not find any addresses to listen on" );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }
    else
    {
        return fAddressExists ? S_OK : S_FALSE;
    }
}   /* COperator::ListenOnAddress */

/**********************************************************
* COperator::ListenOnThisAddress *
*--------------------------------*
*   Description:
*       Call RegisterCallNotifications() to inform TAPI 
*       that we want notifications of calls on this 
*       address.  We already registered our notification
*       interface with TAPI, so now we are just telling
*       TAPI that we want calls from this address to 
*       trigger our existing notification interface.
*   Return:
*       Return value of ITTAPI::RegisterCallNotifications()
************************************************************/
HRESULT COperator::ListenOnThisAddress( ITAddress * pAddress )
{
    // RegisterCallNotifications takes a media type set of flags indicating
    // the set of media types we are interested in. We know the
    // address supports audio (see COperator::ListenOnAddress())
    long lMediaTypes = TAPIMEDIATYPE_AUDIO;

    long     lRegister;
    return m_pTapi->RegisterCallNotifications(
                                           pAddress,
                                           VARIANT_TRUE,
                                           VARIANT_TRUE,
                                           lMediaTypes,
                                           m_ulAdvise,
                                           &lRegister );
}   /* COperator::ListenOnAddress */
                    

/**********************************************************
* COperator::AnswerTheCall *
*--------------------------*
*   Description:
*       Called whenever notification is received of an 
*       incoming call and the app wants to answer it.
*       Answers and handles the call.
*   Return:
*       S_OK
*       Failed retval of QI(), ITCallInfo::get_Address(),
*           COperator::SetAudioOutForCall(),
************************************************************/
HRESULT COperator::AnswerTheCall()
{

    if (NULL == m_pCall)
    {
        return E_UNEXPECTED;
    }
    
    // Get the LegacyCallMediaControl interface so that we can 
    // get a device ID to reroute the audio 
    ITLegacyCallMediaControl *pLegacyCallMediaControl;
    HRESULT hr = m_pCall->QueryInterface( IID_ITLegacyCallMediaControl, 
                                        (void**)&pLegacyCallMediaControl );
    
    
    // Set the audio for the SAPI objects so that the call 
    // can be handled
    if ( SUCCEEDED( hr ) )
    {
        hr = SetAudioOutForCall( pLegacyCallMediaControl );
        if ( FAILED( hr ) )
        {
            DoMessage( L"Could not set up audio out for text-to-speech" );
        }
    }
    if ( SUCCEEDED( hr ) )
    {
        hr = SetAudioInForCall( pLegacyCallMediaControl );
        if ( FAILED( hr ) )
        {
            DoMessage( L"Could not set up audio in for speech recognition" );
        }
    }
    pLegacyCallMediaControl->Release();
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Now we can actually answer the call
    hr = m_pCall->Answer();

    return hr;
}   /* COperator::AnswerTheCall */

/**********************************************************
* COperator::DisconnectTheCall *
*------------------------------*
*   Description:
*       Disconnects the call
*   Return:
*       S_OK
*       S_FALSE if there was no call
*       Failed return value of ITCall::Disconnect()
************************************************************/
HRESULT COperator::DisconnectTheCall()
{
    HRESULT hr = S_OK;
    if (NULL != m_pCall)
    {
        // Hang up
        return m_pCall->Disconnect( DC_NORMAL );

        // Do not release the call yet, as that would prevent
        // us from receiving the disconnected notification.
    }
    else
    {
        return S_FALSE;
    }
}   /* COperator::DisconnectTheCall */

/**********************************************************
* COperator::ReleaseTheCall *
*---------------------------*
*   Description:
*       Releases the call
************************************************************/
void COperator::ReleaseTheCall()
{
    if (NULL != m_pCall)
    {
        m_pCall->Release();
        m_pCall = NULL;
    }
    
    // Let go of the SAPI audio in and audio out objects
    if ( m_cpMMSysAudioOut )
    {
        m_cpMMSysAudioOut->SetState( SPAS_STOP, 0 );
    }
    if ( m_cpMMSysAudioIn )
    {
        m_cpMMSysAudioIn->SetState( SPAS_STOP, 0 );
    }

    m_cpMMSysAudioOut.Release();
    m_cpMMSysAudioIn.Release();
}   /* COperator::ReleaseTheCall */

/**********************************************************
* COperator::OnTapiEvent *
*------------------------*
*   Description:
*       This is the real TAPI event handler, called on our
*       UI thread upon receipt of the WM_PRIVATETAPIEVENT
*       message.
*   Return:
*       S_OK
************************************************************/
HRESULT COperator::OnTapiEvent( TAPI_EVENT TapiEvent,
                                IDispatch * pEvent )
{
    HRESULT hr;
    switch ( TapiEvent )
    {
        case TE_CALLNOTIFICATION:
        {
            // TE_CALLNOTIFICATION means that the application is being notified
            // of a new call.
            //
            // Note that we don't answer to call at this point.  The application
            // should wait for a CS_OFFERING CallState message before answering
            // the call.
            //
            ITCallNotificationEvent         * pNotify;
            hr = pEvent->QueryInterface( IID_ITCallNotificationEvent, (void **)&pNotify );
            if (S_OK != hr)
            {
                DoMessage( L"Incoming call, but failed to get the interface");
            }
            else
            {
                CALL_PRIVILEGE          cp;
                ITCallInfo *            pCall;

                // Get the call
                hr = pNotify->get_Call( &pCall );
                pNotify->Release();
                if ( SUCCEEDED(hr) )
                {
                    // Check to see if we own the call
                    hr = pCall->get_Privilege( &cp );
                    if ( FAILED(hr) || (CP_OWNER != cp) )
                    {
                        // Just ignore it if we don't own it
                        pCall->Release();
                        pEvent->Release(); // We addrefed it CTAPIEventNotification::Event()
                        return S_OK;
                    }

                    // Get the ITBasicCallControl interface and save it in our
                    // member variable.
                    hr = pCall->QueryInterface( IID_ITBasicCallControl,
                                                (void**)&m_pCall );
                    pCall->Release();
                
                    if ( SUCCEEDED(hr) )
                    {
                        // Update UI
                        ::EnableWindow( ::GetDlgItem( m_hDlg, IDC_ANSWER ), TRUE );
                        SetStatusMessage(L"Incoming Owner Call");
                    }
                }
            }
            
            break;
        }
        
        case TE_CALLSTATE:
        {
            // TE_CALLSTATE is a call state event.  pEvent is
            // an ITCallStateEvent

            // Get the interface
            ITCallStateEvent * pCallStateEvent;
            hr = pEvent->QueryInterface( IID_ITCallStateEvent, (void **)&pCallStateEvent );
            if ( FAILED(hr) )
            {
                break;
            }

            // Get the CallState that we are being notified of.
            CALL_STATE         cs;
            hr = pCallStateEvent->get_State( &cs );
            pCallStateEvent->Release();
            if ( FAILED(hr) )
            {
                break;
            }

            // If it's offering to be answered, update our UI
            if (CS_OFFERING == cs)
            {
                if (m_fAutoAnswer)
                {
                    ::PostMessage(m_hDlg, WM_COMMAND, IDC_ANSWER, 0); 
                }
                else
                {
                    SetStatusMessage(L"Click the Answer button");
                }
            }
            else if (CS_DISCONNECTED == cs)
            {
                ::PostMessage(m_hDlg, WM_COMMAND, IDC_DISCONNECTED, 0);
            }
            else if (CS_CONNECTED == cs)
            {
                // Nothing to do -- we handle connection synchronously
            }
            break;
        }
        default:
            break;
    }

    pEvent->Release(); // We addrefed it CTAPIEventNotification::Event()
    
    return S_OK;
}

///////////////////////////////////////////////////////////
// SAPI actions
///////////////////////////////////////////////////////////

/**********************************************************
* COperator::SetAudioOutForCall *
*-------------------------------*
*   Description:
*       Uses the legacy call media control in TAPI to 
*       get the device IDs for audio out.
*       Uses these device IDs to set up the audio 
*       for text-to-speech
*   Return:
*       S_OK
*       E_INVALIDARG if the pLegacyCallMediaControl is NULL
*       E_OUTOFMEMORY
*       SPERR_DEVICE_NOT_SUPPORTED if no supported
*           formats could be found
*       Failed return value of QI(), 
*           ITLegacyCallMediaControl::GetID(),
*           CoCreateInstance(), 
*           ISpMMSysAudio::SetDeviceID(),
*           ISpMMSysAudio::SetFormat(),
*           ISpVoice::SetOutput()                                
************************************************************/
HRESULT COperator::SetAudioOutForCall( ITLegacyCallMediaControl *pLegacyCallMediaControl )
{
    if (NULL == m_pCall)
    {
        return E_UNEXPECTED;
    }

    if ( NULL == pLegacyCallMediaControl )
    {
        return E_INVALIDARG;
    }

    // Get the device ID from ITLegacyCallMediaControl::GetID()
    UINT *puDeviceID;
    BSTR bstrWavOut = ::SysAllocString( L"wave/out" );
    if ( !bstrWavOut )
    {
        return E_OUTOFMEMORY;
    }
    DWORD dwSize = sizeof( puDeviceID );
    HRESULT hr = pLegacyCallMediaControl->GetID( bstrWavOut, &dwSize, (BYTE**) &puDeviceID );
    ::SysFreeString( bstrWavOut );
    
    // Find out what, if any, formats are supported
    GUID guidWave = SPDFID_WaveFormatEx;
    WAVEFORMATEX *pWaveFormatEx = NULL;
    if ( SUCCEEDED(hr) )
    {
        // Loop through all of the SAPI audio formats and query the wave/out device
        // about whether it supports each one.
        // We will take the first one that we find
        SPSTREAMFORMAT enumFmtId;
        MMRESULT mmr = MMSYSERR_ALLOCATED;
        for ( DWORD dw = 0; (MMSYSERR_NOERROR != mmr) && (dw < SPSF_NUM_FORMATS); dw++ )
        {
            if ( pWaveFormatEx && ( MMSYSERR_NOERROR != mmr ) )
            {
                // No dice: The audio device does not support this format

                // Free up the WAVEFORMATEX pointer
                ::CoTaskMemFree( pWaveFormatEx );
                pWaveFormatEx = NULL;
            }

            // Get the next format from SAPI and convert it into a WAVEFORMATEX
            enumFmtId = (SPSTREAMFORMAT) (SPSF_8kHz8BitMono + dw);
            HRESULT hrConvert = SpConvertStreamFormatEnum( 
                enumFmtId, &guidWave, &pWaveFormatEx );

            if ( SUCCEEDED( hrConvert ) )
            {
                // This call to waveOutOpen() does not actually open the device;
                // it just queries the device whether it supports the given format
                mmr = ::waveOutOpen( NULL, *puDeviceID, pWaveFormatEx, 0, 0, WAVE_FORMAT_QUERY );
            }
        }

        // If we made it all the way through the loop without breaking, that
        // means we found no supported formats
        if ( enumFmtId == SPSF_NUM_FORMATS )
        {
            return SPERR_DEVICE_NOT_SUPPORTED;
        }
    }

    // Cocreate a SAPI audio out object
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpMMSysAudioOut.CoCreateInstance( CLSID_SpMMAudioOut );
    }

    // Give the audio out object the device ID
    if ( SUCCEEDED(hr) )
    {
        hr = m_cpMMSysAudioOut->SetDeviceId( *puDeviceID );
    }

    // Use the format that we found works
    if ( SUCCEEDED( hr ) )
    {
        _ASSERTE( pWaveFormatEx );
        hr = m_cpMMSysAudioOut->SetFormat( guidWave, pWaveFormatEx );
    }

    // We are now done with the wave format pointer
    if ( pWaveFormatEx )
    {
        ::CoTaskMemFree( pWaveFormatEx );
    }

    // Set the appropriate output to the outgoing voice
    if ( SUCCEEDED( hr ) )
    {
        _ASSERTE( m_cpOutgoingVoice );
        hr = m_cpOutgoingVoice->SetOutput( m_cpMMSysAudioOut, FALSE );
    }

    return hr;
}   /* COperator::SetAudioOutForCall */

/**********************************************************
* COperator::SetAudioInForCall *
*------------------------------*
*   Description:
*       Uses the legacy call media control in TAPI to 
*       get the device IDs for audio input.
*       Uses these device IDs to set up the audio 
*       for speech recognition
*   Return:
*       S_OK
*       E_INVALIDARG if pLegacyCallMediaControl is NULL
*       E_OUTOFMEMORY
*       SPERR_DEVICE_NOT_SUPPORTED if no supported
*           formats could be found
*       Failed return value of QI(), 
*           ITLegacyCallMediaControl::GetID(),
*           CoCreateInstance(), 
*           ISpMMSysAudio::SetDeviceID(),
*           ISpMMSysAudio::SetFormat()
************************************************************/
HRESULT COperator::SetAudioInForCall( ITLegacyCallMediaControl *pLegacyCallMediaControl )
{
    if ( NULL == m_pCall )
    {
        return E_UNEXPECTED;
    }

    if ( NULL == pLegacyCallMediaControl )
    {
        return E_INVALIDARG;
    }

    // Get the device ID
    UINT *puDeviceID;
    BSTR bstrWavIn = ::SysAllocString( L"wave/in" );
    if ( !bstrWavIn )
    {
        return E_OUTOFMEMORY;
    }
    DWORD dwSize = sizeof( puDeviceID );
    HRESULT hr = pLegacyCallMediaControl->GetID( bstrWavIn, &dwSize, (BYTE**) &puDeviceID );
    ::SysFreeString( bstrWavIn );

    // Find out what, if any, formats are supported
    GUID guidWave = SPDFID_WaveFormatEx;
    WAVEFORMATEX *pWaveFormatEx = NULL;
    if ( SUCCEEDED(hr) )
    {
        // Loop through all of the SAPI audio formats and query the wave/out device
        // about each one.
        // We will take the first one that we find
        SPSTREAMFORMAT enumFmtId;
        MMRESULT mmr = MMSYSERR_ALLOCATED;
        for ( DWORD dw = 0; (MMSYSERR_NOERROR != mmr) && (dw < SPSF_NUM_FORMATS); dw++ )
        {
            if ( pWaveFormatEx && ( MMSYSERR_NOERROR != mmr ) )
            {
                // Free up the WAVEFORMATEX pointer
                ::CoTaskMemFree( pWaveFormatEx );
                pWaveFormatEx = NULL;
            }

            // Get the next format from SAPI and convert it into a WAVEFORMATEX
            enumFmtId = (SPSTREAMFORMAT) (SPSF_8kHz8BitMono + dw);
            HRESULT hrConvert = SpConvertStreamFormatEnum( 
                enumFmtId, &guidWave, &pWaveFormatEx );
            if ( SUCCEEDED( hrConvert ) )
            {
                // This call to waveOutOpen() does not actually open the device;
                // it just queries the device whether it supports the given format
                mmr = ::waveInOpen( NULL, *puDeviceID, pWaveFormatEx, 0, 0, WAVE_FORMAT_QUERY );
            }
        }

        // If we made it all the way through the loop without breaking, that
        // means we found no supported formats
        if ( SPSF_NUM_FORMATS == dw )
        {
            return SPERR_DEVICE_NOT_SUPPORTED;
        }
    }

    // Cocreate a SAPI audio in object
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpMMSysAudioIn.CoCreateInstance( CLSID_SpMMAudioIn );
    }

    // Give the audio in object the device ID
    if ( SUCCEEDED(hr) )
    {
        hr = m_cpMMSysAudioIn->SetDeviceId( *puDeviceID );
    }

    // Use the format that we found works
    if ( SUCCEEDED( hr ) )
    {
        _ASSERTE( pWaveFormatEx );
        hr = m_cpMMSysAudioIn->SetFormat( guidWave, pWaveFormatEx );
    }

    // We are now done with the wave format pointer
    if ( pWaveFormatEx )
    {
        ::CoTaskMemFree( pWaveFormatEx );
    }

    // Set this as input to the reco context
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpIncomingRecognizer->SetInput( m_cpMMSysAudioIn, FALSE );
    }

    return hr;
}   /* COperator::SetAudioInForCall */


/**********************************************************
* COperator::HandleCall *
*-----------------------*
*   Description:
*       Deals with the call
*   Return:
*       S_OK
*       Failed return value of ISpMMSysAudio::SetState(),
*           ISpVoice::Speak()
************************************************************/
HRESULT COperator::HandleCall()
{
    // PLEASE NOTE: This is a single-threaded app, so if the caller
    // hangs up after the call-handling sequence has started, the 
    // app will not be notified until after the entire call sequence 
    // has finished.  
    // If you want to be able to cut the call-handling short because
    // the caller hung up, you need to have a separate thread listening
    // for  TAPI's CS_DISCONNECT notification.

    _ASSERTE( m_cpMMSysAudioOut );
    HRESULT hr = S_OK;
      
    // Now that the call is connected, we can start up the audio output
    hr = m_cpOutgoingVoice->Speak( L"Hello, please say something to me", 0, NULL );

    // Start listening
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpDictGrammar->SetDictationState( SPRS_ACTIVE );
    }

    // We are expecting a PHRASESTART followed by either a RECOGNITION or a 
    // FALSERECOGNITION

    // Wait for the PHRASE_START
    CSpEvent event;
    WORD eLastEventID = SPEI_FALSE_RECOGNITION;
    hr = m_cpIncomingRecoCtxt->WaitForNotifyEvent(CALLER_TIMEOUT);
    if ( SUCCEEDED( hr ) )
    {
        hr = event.GetFrom( m_cpIncomingRecoCtxt );
    }

    // Enter this block only if we have not timed out (the user started speaking)
    if ( ( S_OK == hr ) && ( SPEI_PHRASE_START == event.eEventId ) )
    {
        // Caller has started to speak, block "forever" until the 
        // result (or lack thereof) comes back.
        // This is all right, since every PHRASE_START is guaranteed
        // to be followed up by a RECOGNITION or FALSE_RECOGNITION
        hr = m_cpIncomingRecoCtxt->WaitForNotifyEvent(INFINITE);

        if ( S_OK == hr )
        {
            // Get the RECOGNITION or FALSE_RECOGNITION 
            hr = event.GetFrom( m_cpIncomingRecoCtxt );
            eLastEventID = event.eEventId;

            // This had better be either a RECOGNITION or FALSERECOGNITION!
            _ASSERTE( (SPEI_RECOGNITION == event.eEventId) || 
                            (SPEI_FALSE_RECOGNITION == event.eEventId) );
        }
    }


    // Make sure a recognition result was actually received (as opposed to a false recognition
    // or timeout on the caller)
    WCHAR *pwszCoMemText = NULL;
    ISpRecoResult *pResult = NULL;
    if ( SUCCEEDED( hr ) && ( SPEI_RECOGNITION == event.eEventId ) )
    {
        // Get the text of the result
        pResult = event.RecoResult();

        BYTE bDisplayAttr;
        hr = pResult->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, FALSE, &pwszCoMemText, &bDisplayAttr );
    }
    if ( SUCCEEDED( hr ) && pResult )
    {
        // Speak the result back locally
        m_cpLocalVoice->Speak( L"I think the person on the phone said", SPF_ASYNC, 0 );
        m_cpLocalVoice->Speak( pwszCoMemText, SPF_ASYNC, 0 );
        m_cpLocalVoice->Speak( L"when he said", SPF_ASYNC, 0 );
        
        // Get the audio so that the local voice can speak it back
        CComPtr<ISpStreamFormat> cpStreamFormat;
        HRESULT hrAudio = pResult->GetAudio( 0, 0, &cpStreamFormat );
        if ( SUCCEEDED( hrAudio ) )
        {
            m_cpLocalVoice->SpeakStream( cpStreamFormat, SPF_ASYNC, 0 );
        }
        else
        {
            m_cpLocalVoice->Speak( L"no audio was available", SPF_ASYNC, 0 );
        }
    }

    // Stop listening
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpDictGrammar->SetDictationState( SPRS_INACTIVE );
    }

    // Close the audio input so that we can open the audio output
    // (half-duplex device)
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpMMSysAudioIn->SetState( SPAS_CLOSED, 0 );
    }

    // The caller may have hung up on us, in which case we don't want to do 
    // the following
    if ( m_pCall )
    {
        if ( pResult )
        {
            // There's a result to playback
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpOutgoingVoice->Speak( L"I think I heard you say", 0, 0 );
            }
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpOutgoingVoice->Speak( pwszCoMemText, 0, 0 );
            }
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpOutgoingVoice->Speak( L"when you said", 0, 0 );
            }
            if ( SUCCEEDED( hr ) )
            {
                hr = pResult->SpeakAudio( NULL, 0, NULL, NULL );
            }
        }
        else
        {
            // Caller didn't say anything
            if ( SUCCEEDED( hr ) )
            {
                hr = m_cpOutgoingVoice->Speak( L"I don't believe you said anything!", 0, 0 );
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = m_cpOutgoingVoice->Speak( L"OK bye now", 0, 0 );
        }
    }
    else
    {
        m_cpLocalVoice->Speak( L"Prematurely terminated call", 0, 0 );
    }

    if ( pwszCoMemText )
    {
        ::CoTaskMemFree( pwszCoMemText );
    }

    return m_pCall ? hr : TAPI_E_DROPPED;
}   /* COperator::HandleCall */

///////////////////////////////////////////////////////////
// Miscellany
///////////////////////////////////////////////////////////

/**********************************************************
* COperator::DoMessage *
*----------------------*
*   Description:
*       Displays a MessageBox
************************************************************/
void COperator::DoMessage( LPWSTR pszMessage )
{
    USES_CONVERSION;
    ::MessageBox( m_hDlg, W2T(pszMessage), W2T((WCHAR *) gszTelephonySample), MB_OK );
}   /* COperator::DoMessage */


/**********************************************************
* COperator::SetStatusMessage *
*-----------------------------*
*   Description:
*       Displays a status message
************************************************************/
void COperator::SetStatusMessage( LPWSTR pszMessage )
{
    USES_CONVERSION;
    ::SetDlgItemText( m_hDlg, IDC_STATUS, W2T(pszMessage) );
}   /* COperator::SetStatusMessage */

/*************************************************************
* AddressSupportsMediaType *
*--------------------------*
*   Return:
*       TRUE iff the given address supports the given media 
**************************************************************/
BOOL AddressSupportsMediaType( ITAddress * pAddress,
                                long lMediaType )
{
    VARIANT_BOOL     bSupport = VARIANT_FALSE;
    ITMediaSupport * pMediaSupport;
    
    if ( SUCCEEDED( pAddress->QueryInterface( IID_ITMediaSupport,
                                              (void **)&pMediaSupport ) ) )
    {
        // does it support this media type?
        pMediaSupport->QueryMediaType( lMediaType,
                                      &bSupport );
    
        pMediaSupport->Release();
    }
    return (bSupport == VARIANT_TRUE);
}   /* AddressSupportsMediaType */
