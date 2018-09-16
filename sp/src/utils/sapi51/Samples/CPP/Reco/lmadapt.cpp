/******************************************************************************
*   LMAdapt.cpp 
*       Language Model adaptation routines
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#include "stdafx.h"
#include <commdlg.h>

/****************************************************************************
* CRecoDlgClass::GetTextFile *
*----------------------------*
*
*   Description:    Load the content of a text file into a buffer.  The
*                   buffer is converted into Unicode using the default
*                   engine codepage if it isn't already in Unicode.
*
*   Return:         Buffer and character count
*                   HRESULT hr;
*
****************************************************************************/

HRESULT CRecoDlgClass::GetTextFile(
    WCHAR ** ppwszCoMem,
    ULONG * pcch)
{
    TCHAR pszFileName[_MAX_PATH];
    OPENFILENAME ofn;
    HRESULT hr = S_OK;
    HANDLE hf = INVALID_HANDLE_VALUE;
    DWORD cBytes;
    BOOL fUnicodeFile = FALSE;
    USHORT uTemp;
    WCHAR * pwszCoMem = 0;
    ULONG cch = 0;
    DWORD dwRead;
	size_t ofnsize = (BYTE*)&ofn.lpTemplateName + sizeof(ofn.lpTemplateName) - (BYTE*)&ofn;

    if (SUCCEEDED(hr))
    {
        pszFileName[0] = 0;

        ofn.lStructSize = ofnsize;
        ofn.hwndOwner = m_hDlg;
        ofn.hInstance = m_hInstance;
        ofn.lpstrFilter = _T("Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0");
        ofn.lpstrCustomFilter = NULL;
        ofn.nMaxCustFilter = 0;
        ofn.nFilterIndex = 0;
        ofn.lpstrFile = pszFileName;
        ofn.nMaxFile = _MAX_PATH;
        ofn.nMaxFileTitle = _MAX_PATH;
        ofn.lpstrFileTitle = NULL;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrTitle = NULL;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt = _T("txt");

        hr = GetOpenFileName(&ofn) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        hf = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        hr = (hf != INVALID_HANDLE_VALUE) ? S_OK : HRESULT_FROM_WIN32(CommDlgExtendedError());
    }

    if (SUCCEEDED(hr))
    {
        cBytes = GetFileSize(hf, NULL); // 64K limit

        hr = (cBytes != -1) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        hr = ReadFile(hf, &uTemp, 2, &dwRead, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        fUnicodeFile = uTemp == 0xfeff;

        if (fUnicodeFile)
        {
            cBytes -= 2;

            pwszCoMem = (WCHAR *)CoTaskMemAlloc(cBytes);

            if (pwszCoMem)
            {
                hr = ReadFile(hf, pwszCoMem, cBytes, &dwRead, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

                cch = cBytes / sizeof(WCHAR);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            UINT uiCodePage = SpCodePageFromLcid(MAKELCID(m_langid, SORT_DEFAULT));

            char * pszBuffer = (char *)malloc(cBytes);

            hr = pszBuffer ? S_OK : E_OUTOFMEMORY;

            if (SUCCEEDED(hr))
            {
                SetFilePointer(hf, 0, NULL, FILE_BEGIN); // rewind
        
                hr = ReadFile(hf, pszBuffer, cBytes, &dwRead, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
            }

            if (SUCCEEDED(hr))
            {
                cch = MultiByteToWideChar(uiCodePage, 0, pszBuffer, cBytes, NULL, NULL);

                if (cch)
                {
                    pwszCoMem = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * cch);
                }
                else
                {
                    hr = E_FAIL;
                }
            }

            if (SUCCEEDED(hr))
            {
                MultiByteToWideChar(uiCodePage, 0, pszBuffer, cBytes, pwszCoMem, cch);
            }

            if (pszBuffer)
            {
                free(pszBuffer);
            }
        }
    }

    if (INVALID_HANDLE_VALUE != hf)
    {
        CloseHandle(hf);
    }

    *ppwszCoMem = pwszCoMem;
    *pcch = cch;
    
    return hr;
}

/***************************************************************************************
* CRecoDlgClass::FeedDocumentFromFile *
*-------------------------------------*
*   Description: Open a text file and feed its content to the SR engine.
*   Return:
*       S_OK 
*       E_FAIL if no client window or if unsuccessful at obtaining the file name
****************************************************************************************/

HRESULT CRecoDlgClass::FeedDocumentFromFile()
{
    WCHAR * pwszCoMem = 0;
    ULONG cch = 0;
    
    HRESULT hr = GetTextFile(&pwszCoMem, &cch);

    if (SUCCEEDED(hr))
    {
        hr = FeedDocument(pwszCoMem, cch);
    }

    return hr;
}
   
/***************************************************************************************
* CRecoDlgClass::FeedDocumentFromClipboard *
*------------------------------------------*
*   Description: Take text from the clipboard and feed its content to the SR engine.
*   Return:
*       S_OK 
*       E_FAIL if neither CF_TEXT or CF_UNICODETEXT was available on the clipboard
****************************************************************************************/

HRESULT CRecoDlgClass::FeedDocumentFromClipboard()
{
    HRESULT hr;
    UINT uFormat;
    HANDLE hData;
    BOOL fClipboardOpened = FALSE;
    WCHAR * pwszCoMem;
    ULONG cch;

    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        uFormat = CF_UNICODETEXT;
    }
    else if (IsClipboardFormatAvailable(CF_TEXT))
    {
        uFormat = CF_TEXT;
    }
    else
    {
        uFormat = 0;
    }

    if (uFormat)
    {
        hr = OpenClipboard(m_hDlg) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        fClipboardOpened = TRUE;

        hData = GetClipboardData(uFormat);

        hr = hData ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
 
    if (SUCCEEDED(hr))
    {
        VOID * pData = 0;
        DWORD cBytes;

        if (CF_UNICODETEXT == uFormat)
        {
            cBytes = GlobalSize(hData);

            pwszCoMem = (WCHAR *)CoTaskMemAlloc(cBytes);

            if (pwszCoMem)
            {
                pData = GlobalLock(hData);

                hr = pData ? S_OK : HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                memcpy(pwszCoMem, pData, cBytes);

                cch = cBytes / sizeof(WCHAR);
            }
        }
        else
        {
            UINT uiCodePage = SpCodePageFromLcid(MAKELCID(m_langid, SORT_DEFAULT));

            cBytes = GlobalSize(hData);
            pData = GlobalLock(hData);
            hr = pData ? S_OK : HRESULT_FROM_WIN32(GetLastError());

            if (SUCCEEDED(hr))
            {
                cch = MultiByteToWideChar(uiCodePage, 0, (char *)pData, cBytes, NULL, NULL);

                hr = cch ? S_OK : HRESULT_FROM_WIN32(GetLastError());
            }

            if (SUCCEEDED(hr))
            {
                pwszCoMem = (WCHAR *)CoTaskMemAlloc(cch * sizeof(WCHAR));

                hr = pwszCoMem ? S_OK : E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                MultiByteToWideChar(uiCodePage, 0, (char *)pData, cBytes, pwszCoMem, cch);
            }            
        }

        if (pData)
        {
            GlobalUnlock(hData);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        hr = FeedDocument(pwszCoMem, cch);
    }

    if (fClipboardOpened)
    {
        CloseClipboard();
    }

    return hr;
}

/***************************************************************************************
* CRecoDlgClass::FeedDocument *
*-----------------------------*
*   Description: Common code for FeedDocumentFromFile and FeedDocumentFromClipboard.
*                Feed the text buffer to the SR engine.
*   Return:
*       S_OK 
*       E_* if error in SR engine
****************************************************************************************/

HRESULT CRecoDlgClass::FeedDocument(const WCHAR * pszCoMem, const ULONG cch)
{
    HRESULT hr = S_OK;
    //
    //  NOTE:  This function limits the size of the adaptation data to a maximum of 16K
    //         characters (32K bytes).  This sample does NOT demonstrate the proper use 
    //         of this method.  Applcations that use SetAdaptationData should break the
    //         data into small (1K or less) blocks, call SetAdaptationData, and then wait
    //         for a SPEI_ADAPTATION event before sending the next small block of data.
    //         Note that calling SetAdapataionData with buffers larger than 32K can result
    //         in unpredictable results.  Do NOT call this method with large buffers.
    //
    if (cch <= 16000)
    {
        hr = m_cpRecoCtxt->SetAdaptationData(pszCoMem, cch);
    }

    CoTaskMemFree((void *)pszCoMem);

    return hr;
}

/***************************************************************************************
* CRecoDlgClass::TrainFromFile *
*------------------------------*
*   Description: Load a text file, and launch the User Training wizard using this text.
*
*   Return:
*       S_OK 
*       E_* if error in SR engine
****************************************************************************************/

HRESULT CRecoDlgClass::TrainFromFile()
{
    WCHAR * pwszCoMem = 0;
    ULONG cch = 0;
    
    HRESULT hr = GetTextFile(&pwszCoMem, &cch);

    if (SUCCEEDED(hr))
    {
        hr = m_cpRecognizer->DisplayUI(m_hDlg, NULL, SPDUI_UserTraining, (void *)pwszCoMem, cch * sizeof(WCHAR));

        CoTaskMemFree(pwszCoMem);
    }

    return hr;
}
