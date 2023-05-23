/****************************************************************************
*	SimpleDict.cpp
*		A very simple dictation app.  The user speaks, and whatever the
*       engine recognizes appears in an edit box.
*
*	Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include <sphelper.h>
#include "simpledict.h"

// Foward declarations of functions included in this code module:
LRESULT CALLBACK SimpleDictDlgProc( HWND, UINT, WPARAM, LPARAM );

// Entry point
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
#ifdef _WIN32_WCE
    if (SUCCEEDED(::CoInitializeEx(NULL,COINIT_MULTITHREADED)))
#else
    if (SUCCEEDED(::CoInitialize(NULL)))
#endif
    {
        // NOTE:  Extra scope provided so DlgInfo is destroyed before CoUninitialize is called.
        {
            CSimpleDict SimpleDictClass( hInstance );
            ::DialogBoxParam( hInstance, MAKEINTRESOURCE( IDD_SIMPLEDICTDLG ), NULL, 
                (DLGPROC) CSimpleDict::SimpleDictDlgProc, (LPARAM) &SimpleDictClass );
        }
        ::CoUninitialize();
    }
    return 0;
}



/*********************************************************************************************
* CSimpleDict::SimpleDictDlgProc()
*       Main message handler for SimpleDict dialog box.
*       At initialization time, LPARAM is the CSimpleDict object, to which the
*       window long is set.
**********************************************************************************************/
LRESULT CALLBACK CSimpleDict::SimpleDictDlgProc(HWND hDlg, 
                                                   UINT message, 
                                                   WPARAM wParam, 
                                                   LPARAM lParam)
{
    CSimpleDict *pThis = (CSimpleDict *) ::GetWindowLong( hDlg, GWL_USERDATA );
    switch( message )
    {
        case WM_INITDIALOG:
            ::SetWindowLong( hDlg, GWL_USERDATA, lParam );
            pThis = (CSimpleDict *) lParam;
            if ( !pThis->InitDialog( hDlg ) )
            {
                ::DestroyWindow( hDlg );
            }
            break;

        case WM_RECOEVENT:
            // All recognition events send this message, because that is how we
            // specified we should be notified
            pThis->RecoEvent();
            return TRUE;
            break;

        case WM_COMMAND:
            if ( LOWORD( wParam ) == IDC_BUTTON_EXIT )
            {
                ::EndDialog( hDlg, TRUE );
            }
            break;

        case WM_CLOSE:
            ::EndDialog( hDlg, TRUE );
            break;
         
        case WM_DESTROY:
            // Release the recognition context and the dictation grammar
            pThis->m_cpRecoCtxt.Release();
            pThis->m_cpDictationGrammar.Release();

            break;

        default:
            return FALSE;
            break;
    }

    return TRUE;
}

/*********************************************************************************************
* CSimpleDict::InitDialog()
*   Creates the recognition context and activates the grammar.  
*   Returns TRUE iff successful.
**********************************************************************************************/
bool CSimpleDict::InitDialog( HWND hDlg )
{
    m_hDlg = hDlg;
    
    HRESULT hr = S_OK;
    CComPtr<ISpRecognizer> cpRecoEngine;
    hr = cpRecoEngine.CoCreateInstance(CLSID_SpInprocRecognizer);

    if( SUCCEEDED( hr ) )
    {
        hr = cpRecoEngine->CreateRecoContext( &m_cpRecoCtxt );
    }


    // Set recognition notification for dictation
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoCtxt->SetNotifyWindowMessage( hDlg, WM_RECOEVENT, 0, 0 );
    }
    
    
    if (SUCCEEDED(hr))
    {
        // This specifies which of the recognition events are going to trigger notifications.
        // Here, all we are interested in is the beginning and ends of sounds, as well as
        // when the engine has recognized something
        const ULONGLONG ullInterest = SPFEI(SPEI_RECOGNITION);
        hr = m_cpRecoCtxt->SetInterest(ullInterest, ullInterest);
    }

    // create default audio object
    CComPtr<ISpAudio> cpAudio;
    hr = SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOIN, &cpAudio);

    // set the input for the engine
    hr = cpRecoEngine->SetInput(cpAudio, TRUE);
    hr = cpRecoEngine->SetRecoState( SPRST_ACTIVE );




    
    if (SUCCEEDED(hr))
    {
        // Specifies that the grammar we want is a dictation grammar.
        // Initializes the grammar (m_cpDictationGrammar)
        hr = m_cpRecoCtxt->CreateGrammar( GID_DICTATION, &m_cpDictationGrammar );
    }
    if  (SUCCEEDED(hr))
    {
        hr = m_cpDictationGrammar->LoadDictation(NULL, SPLO_STATIC);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpDictationGrammar->SetDictationState( SPRS_ACTIVE );
    }
    if (FAILED(hr))
    {
        m_cpDictationGrammar.Release();
    }

    return (hr == S_OK);
}

/*****************************************************************************************
* CSimpleDict::RecoEvent()
*   Called whenever the dialog process is notified of a recognition event.
*   Inserts whatever is recognized into the edit box.  
******************************************************************************************/
void CSimpleDict::RecoEvent()
{
    USES_CONVERSION;
    CSpEvent event;

    // Process all of the recognition events
    while (event.GetFrom(m_cpRecoCtxt) == S_OK)
    {
        switch (event.eEventId)
        {
            case SPEI_SOUND_START:
                m_bInSound = TRUE;
                break;

            case SPEI_SOUND_END:
                if (m_bInSound)
                {
                    m_bInSound = FALSE;
                    if (!m_bGotReco)
                    {
                        // The sound has started and ended, 
                        // but the engine has not succeeded in recognizing anything
						const TCHAR szNoise[] = _T("<noise>");
                        ::SendDlgItemMessage( m_hDlg, IDC_EDIT_DICT, 
							EM_REPLACESEL, TRUE, (LPARAM) szNoise );
                    }
                    m_bGotReco = FALSE;
                }
                break;

            case SPEI_RECOGNITION:
                // There may be multiple recognition results, so get all of them
                {
                    m_bGotReco = TRUE;
                    static const WCHAR wszUnrecognized[] = L"<Unrecognized>";

                    CSpDynamicString dstrText;
                    if (FAILED(event.RecoResult()->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, 
                                                            &dstrText, NULL)))
                    {
                        dstrText = wszUnrecognized;
                    }

                    // Concatenate a space onto the end of the recognized word
                    dstrText.Append(L" ");

                    ::SendDlgItemMessage( m_hDlg, IDC_EDIT_DICT, EM_REPLACESEL, TRUE, (LPARAM) W2T(dstrText) );

                }
                break;

        }
    }
} 

