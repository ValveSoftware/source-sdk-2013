/******************************************************************************
* mkvoice.cpp *
*-------------*
*   This application assembles a simple voice font for the sample TTS engine.
*Copyright (c) Microsoft Corporation. All rights reserved.
*
******************************************************************************/
#include "stdafx.h"
#include <ttseng_i.c>
#include <direct.h>

int wmain(int argc, WCHAR* argv[])
{
    USES_CONVERSION;
    static const DWORD dwVersion = { 1 };
    ULONG ulNumWords = 0;
    HRESULT hr = S_OK;

    //--- Check args
    if( argc != 4 )
    {
        printf( "%s", "Usage: > mkvoice [[in]word list file] [[out]voice file] [voice name]\n" );
        hr = E_INVALIDARG;
    }
    else
    {
        ::CoInitialize( NULL );

        //--- Open word list file and create output voice file
        //--- _wfopen is not supported on Win9x, so use fopen.
        FILE* hWordList  = fopen( W2A(argv[1]), "r" );
        FILE* hVoiceFile = fopen( W2A(argv[2]), "wb" );

        if( hWordList && hVoiceFile )
        {
            //--- Write file version and leave space for word count
            if( !fwrite( &dwVersion, sizeof(dwVersion), 1, hVoiceFile ) ||
                 fseek( hVoiceFile, 4, SEEK_CUR ) )
            {
                hr = E_FAIL;
            }

            //--- Get each entry
            WCHAR WordFileName[MAX_PATH];
            while( SUCCEEDED( hr ) && fgetws( WordFileName, MAX_PATH, hWordList ) )
            {
                ULONG ulTextLen = wcslen( WordFileName );
                if( WordFileName[ulTextLen-1] == '\n' )
                {
                    WordFileName[--ulTextLen] = NULL;
                }
                //--- Include NULL character when writing to the file
                ulTextLen = (ulTextLen+1) * sizeof(WCHAR);

                if( fwrite( &ulTextLen, sizeof(ulTextLen), 1, hVoiceFile ) &&
                    fwrite( WordFileName, ulTextLen, 1, hVoiceFile ) )
                {
                    ++ulNumWords;
                    //--- Open the wav data
                    ISpStream* pStream;
                    wcscat( WordFileName, L".wav" );
                    hr = SPBindToFile( WordFileName, SPFM_OPEN_READONLY, &pStream );
                    if( SUCCEEDED( hr ) )
                    {
                        CSpStreamFormat Fmt;
                        Fmt.AssignFormat(pStream);
                        if( Fmt.ComputeFormatEnum() == SPSF_11kHz16BitMono )
                        {
                            STATSTG Stat;
                            hr = pStream->Stat( &Stat, STATFLAG_NONAME );
                            ULONG ulNumBytes = Stat.cbSize.LowPart;

                            //--- Write the number of audio bytes
                            if( SUCCEEDED( hr ) &&
                                fwrite( &ulNumBytes, sizeof(ulNumBytes), 1, hVoiceFile ) )
                            {
                                BYTE* Buff = (BYTE*)alloca( ulNumBytes );
                                if( SUCCEEDED( hr = pStream->Read( Buff, ulNumBytes, NULL ) ) )
                                {
                                    //--- Write the audio samples
                                    if( !fwrite( Buff, 1, ulNumBytes, hVoiceFile ) )
                                    {
                                        hr = E_FAIL;
                                    }
                                }
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                        }
                        else
                        {
                            printf( "Input file: %s has wrong wav format.", WordFileName );
                        }
                        pStream->Release();
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }

        //--- Write word count
        if( SUCCEEDED( hr ) )
        {
            if( fseek( hVoiceFile, sizeof(dwVersion), SEEK_SET ) ||
                !fwrite( &ulNumWords, sizeof(ulNumWords), 1, hVoiceFile ) )
            {
                hr = E_FAIL;
            }
        }

        //--- Register the new voice file
        //    The section below shows how to programatically create a token for
        //    the new voice and set its attributes.
        if( SUCCEEDED( hr ) )
        {
            CComPtr<ISpObjectToken> cpToken;
            CComPtr<ISpDataKey> cpDataKeyAttribs;
            hr = SpCreateNewTokenEx(
                    SPCAT_VOICES, 
                    argv[3], 
                    &CLSID_SampleTTSEngine, 
                    L"Sample TTS Voice", 
                    0x409, 
                    L"Sample TTS Voice", 
                    &cpToken,
                    &cpDataKeyAttribs);

            //--- Set additional attributes for searching and the path to the
            //    voice data file we just created.
            if (SUCCEEDED(hr))
            {
                hr = cpDataKeyAttribs->SetStringValue(L"Gender", L"Male");
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Name", L"SampleTTSVoice");
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Language", L"409");
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Age", L"Adult");
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Microsoft");
                }

                //--- _wfullpath is not supported on Win9x, so use _fullpath.
                CHAR    szFullPath[MAX_PATH * 2];
                if (SUCCEEDED(hr) && _fullpath(szFullPath, W2A(argv[2]), sizeof(szFullPath)/sizeof(szFullPath[0])) == NULL)
                {
                    hr = SPERR_NOT_FOUND;
                }

                if (SUCCEEDED(hr))
                {
                    USES_CONVERSION;
                    hr = cpToken->SetStringValue(L"VoiceData", A2W(szFullPath));
                }
            }
        }

        //--- Cleanup
        if( hWordList  )
        {
            fclose( hWordList );
        }
        if( hVoiceFile )
        {
            fclose( hVoiceFile );
        }
        ::CoUninitialize();
    }
    return FAILED( hr );
}

