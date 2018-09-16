#include <windows.h>
#include <sapi.h>
#include <stdio.h>
#include <string.h>
#include <atlbase.h>
#include "sphelper.h"
//Copyright (c) Microsoft Corporation. All rights reserved.

inline HRESULT BlockForResult(ISpRecoContext * pRecoCtxt, ISpRecoResult ** ppResult)
{
    HRESULT hr = S_OK;
	CSpEvent event;

    while (SUCCEEDED(hr) &&
           SUCCEEDED(hr = event.GetFrom(pRecoCtxt)) &&
           hr == S_FALSE)
    {
        hr = pRecoCtxt->WaitForNotifyEvent(INFINITE);
    }

    *ppResult = event.RecoResult();
    if (*ppResult)
    {
        (*ppResult)->AddRef();
    }

    return hr;
}

const WCHAR * StopWord()
{
    const WCHAR * pchStop;
    
    LANGID LangId = ::SpGetUserDefaultUILanguage();

    switch (LangId)
    {
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT):
            pchStop = L"\x7d42\x4e86\\\x30b7\x30e5\x30fc\x30ea\x30e7\x30fc/\x3057\x3085\x3046\x308a\x3087\x3046";;
            break;

        default:
            pchStop = L"Stop";
            break;
    }

    return pchStop;
}
            
int main(int argc, char* argv[])
{
    HRESULT hr = E_FAIL;
    bool fUseTTS = true;            // turn TTS play back on or off
    bool fReplay = true;            // turn Audio replay on or off

    // Process optional arguments
    if (argc > 1)
    {
        int i;

        for (i = 1; i < argc; i++)
        {
            if (_stricmp(argv[i], "-noTTS") == 0)
            {
                fUseTTS = false;
                continue;
            }
            if (_stricmp(argv[i], "-noReplay") == 0)
            {
                fReplay = false;
                continue;
            }       
            printf ("Usage: %s [-noTTS] [-noReplay] \n", argv[0]);
            return hr;
        }
    }

    if (SUCCEEDED(hr = ::CoInitialize(NULL)))
    {
        {
            CComPtr<ISpRecoContext> cpRecoCtxt;
            CComPtr<ISpRecoGrammar> cpGrammar;
            CComPtr<ISpVoice> cpVoice;
            hr = cpRecoCtxt.CoCreateInstance(CLSID_SpSharedRecoContext);
            if(SUCCEEDED(hr))
            {
                hr = cpRecoCtxt->GetVoice(&cpVoice);
            }
           
            if (cpRecoCtxt && cpVoice &&
                SUCCEEDED(hr = cpRecoCtxt->SetNotifyWin32Event()) &&
                SUCCEEDED(hr = cpRecoCtxt->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION))) &&
                SUCCEEDED(hr = cpRecoCtxt->SetAudioOptions(SPAO_RETAIN_AUDIO, NULL, NULL)) &&
                SUCCEEDED(hr = cpRecoCtxt->CreateGrammar(0, &cpGrammar)) &&
                SUCCEEDED(hr = cpGrammar->LoadDictation(NULL, SPLO_STATIC)) &&
                SUCCEEDED(hr = cpGrammar->SetDictationState(SPRS_ACTIVE)))
            {
                USES_CONVERSION;
                            
                const WCHAR * const pchStop = StopWord();
                CComPtr<ISpRecoResult> cpResult;

                printf( "I will repeat everything you say.\nSay \"%s\" to exit.\n", W2A(pchStop) );

                while (SUCCEEDED(hr = BlockForResult(cpRecoCtxt, &cpResult)))
                {
                    cpGrammar->SetDictationState( SPRS_INACTIVE );

                    CSpDynamicString dstrText;

                    if (SUCCEEDED(cpResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, 
                                                    TRUE, &dstrText, NULL)))
                    {
                        printf("I heard:  %s\n", W2A(dstrText));

                        if (fUseTTS)
                        {
                            cpVoice->Speak( L"I heard", SPF_ASYNC, NULL);
                            cpVoice->Speak( dstrText, SPF_ASYNC, NULL );
                        }

                        if (fReplay)
                        {
                            if (fUseTTS)
                                cpVoice->Speak( L"when you said", SPF_ASYNC, NULL);
                            else
                                printf ("\twhen you said...\n");
                            cpResult->SpeakAudio(NULL, 0, NULL, NULL);
                       }

                       cpResult.Release();
                    }
                    if (_wcsicmp(dstrText, pchStop) == 0)
                    {
                        break;
                    }
                    
                    cpGrammar->SetDictationState( SPRS_ACTIVE );
                } 
            }
        }
        ::CoUninitialize();
    }
    return hr;
}
