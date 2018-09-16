/****************************************************************************
* Reco.Cpp *
*----------*
*   Description:
*       This sample program is a basic speech recognition test tool.  It is a
*   dialog based Win32 application that uses ATL to help manage COM interface
*   pointers.  A basic understanding of the ATL CComPtr object will be useful
*   when looking at this code.
*
*       This tool allows the user to load and activate command and control
*   grammars, activate a dictation grammar, turn the microphone and audio
*   retention on and off, and examine the results of recognitions.
*Copyright (c) Microsoft Corporation. All rights reserved.
*
****************************************************************************/

#include "stdafx.h"
#define FT64(filetime) (*((LONGLONG*)&(filetime)))

/****************************************************************************
* WinMain *
*---------*
*   Description:
*       Main entry point for a Win32 application.  This program simply initializes
*   COM, constructs a CRecoDlg class object on the stack, and displays the dialog
*   by calling DialogBoxParam.    
*
*   Returns:
*       Always returns 0
*
****************************************************************************/

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
        // NOTE:  Extra scope provided so DlgClass is destroyed before CoUninitialize is called.
        {
            CRecoDlgClass DlgClass(hInstance);
            ::DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_RECODLG), NULL, (DLGPROC)CRecoDlgClass::DlgProc, (LPARAM)&DlgClass);
        }
        ::CoUninitialize();
    }
    return 0;
}


/****************************************************************************
*   Implementation of CRecoDlgClass
****************************************************************************/


/****************************************************************************
* CRecoDlgClass::DlgProc *
*------------------------*
*   Description:
*       This static member function is the message handler for the main appliation
*   dialog.  When the dialog is initialized via WM_INITDIALOG, the pointer to the
*   CRecoDlgClass object is passed to this function in the lParam.  This is stored
*   in the USERDATA window long and used for subsequent message processing.
*
*   Returns:
*       Appropriate LRESULT for give window message
*
****************************************************************************/

LRESULT CALLBACK CRecoDlgClass::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRecoDlgClass * pThis = (CRecoDlgClass *)::GetWindowLong(hDlg, GWL_USERDATA);
    LRESULT lr = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            ::SetWindowLong(hDlg, GWL_USERDATA, lParam);
            pThis = (CRecoDlgClass *)lParam;
            lr = pThis->InitDialog(hDlg);
            break;

        case WM_RECOEVENT:
            pThis->RecoEvent();
            lr = TRUE;
            break;

        case WM_HELP:
            ::DialogBoxParam(pThis->m_hInstance, (LPCTSTR)IDD_DIALOG_BETAHELP, hDlg, (DLGPROC)BetaHelpDlgProc, NULL);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_CHECK_CREATE)
            {
                pThis->UpdateRecoCtxtState();
            }
            else if (LOWORD(wParam) == IDC_CHECK_MIC)
            {
                BOOL bIsChecked = (::SendDlgItemMessage( hDlg, IDC_CHECK_MIC, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
                if (bIsChecked)
                {
                    pThis->m_cpRecognizer->SetRecoState( SPRST_ACTIVE );
                }
                else
                {
                    pThis->m_cpRecognizer->SetRecoState( SPRST_INACTIVE );
                }
            }
            else if (LOWORD(wParam) == IDM_FILE_EXIT || LOWORD(wParam) == IDCANCEL)
            {
                pThis->Cleanup();
                EndDialog(hDlg, 0);
                lr = TRUE;
            }
            else if (LOWORD(wParam) == IDM_HELP)
            {
                ::DialogBoxParam(pThis->m_hInstance, (LPCTSTR)IDD_DIALOG_BETAHELP, hDlg, (DLGPROC)BetaHelpDlgProc, NULL);
            }
            else if (LOWORD(wParam) == IDM_CFG_LOAD_GRAMMAR)
            {
                pThis->SpecifyCAndCGrammar();
            }
            else if (LOWORD(wParam) == IDM_CFG_SET_WORD_SEQUENCE_DATA)
            {
                pThis->SetWordSequenceData();
            }
            else if (LOWORD(wParam) == IDC_EDIT_PARSETEXT)
            {
                BOOL bEnableSubmit = ::SendDlgItemMessage(hDlg, IDC_EDIT_PARSETEXT, WM_GETTEXTLENGTH, 0, 0);
                ::EnableWindow(::GetDlgItem(hDlg, IDC_BUTTON_SUBMIT), bEnableSubmit);
            }
            else if (LOWORD(wParam) == IDC_BUTTON_SUBMIT)
            {
                TCHAR tszText[MAX_PATH];
                ::SendDlgItemMessage(hDlg, IDC_EDIT_PARSETEXT, WM_GETTEXT, sp_countof(tszText), (LPARAM)tszText);
                if (tszText[0] != 0)
                {
                    USES_CONVERSION;
                    pThis->EmulateRecognition(T2W(tszText));
                }
            }
            else if( LOWORD(wParam) == IDC_BUTTON_ALTERNATES )
            {
                LONG item = ::SendDlgItemMessage(hDlg, IDC_LIST_PHRASES, LB_GETCURSEL, 0, 0);
                CRecoDlgListItem * pli = NULL;
                if (item != LB_ERR)
                {
                    pli = (CRecoDlgListItem*)::SendDlgItemMessage(hDlg, IDC_LIST_PHRASES, LB_GETITEMDATA, item, 0);
                }
                if( pli )
                {
                    CAlternatesDlgClass AltDlg;
                    AltDlg.m_pResult = pli->GetRecoResult();
                    if (AltDlg.m_pResult)
                    {
                        int RetVal = ::DialogBoxParam( pThis->m_hInstance, (LPCTSTR)IDD_ALTS_DIALOG, hDlg,
                                                      (DLGPROC)CAlternatesDlgClass::AlternatesDlgProc,
                                                      (LPARAM)&AltDlg );

                        //--- Draw the new item
                        if( RetVal ==  IDOK )
                        {
                            CSpDynamicString dstrText;
                            AltDlg.m_pResult->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE,
                                                       TRUE, &dstrText, NULL );
                            CRecoDlgListItem * pNewItem =
                                new CRecoDlgListItem( AltDlg.m_pResult, dstrText, pli->IsHypothesis() );
                            int iNewPhrase = ::SendDlgItemMessage( hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pNewItem);
                            ::SendDlgItemMessage( hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                            ::SendDlgItemMessage( hDlg, IDC_LIST_PHRASES, LB_DELETESTRING, iNewPhrase-1, 0);
                            pThis->UpdatePropWindow( pNewItem );
                        }
                    }
                }
                lr = TRUE;
            }
            else if (LOWORD(wParam) == IDM_CFG_ADD_DYNAMIC_RULE && pThis->m_cpCFGGrammar)
            {
                CDynGrammarDlgClass DlgClass(pThis, &pThis->m_ItemList);
                ::DialogBoxParam(pThis->m_hInstance, (LPCTSTR)IDD_DYNGRAMMARDLG, hDlg, (DLGPROC)CDynGrammarDlgClass::DlgProc, (LPARAM)&DlgClass);
                lr = TRUE;
            }
            else if (LOWORD(wParam) == IDM_SLM_ADAPT_FROM_FILE)
            {
                lr = pThis->FeedDocumentFromFile();
            }
            else if (LOWORD(wParam) == IDM_SLM_ADAPT_FROM_CLIPBOARD)
            {
                lr = pThis->FeedDocumentFromClipboard();
            }
            else if (LOWORD(wParam) == IDM_SLM_TRAIN_FROM_FILE)
            {
                lr = pThis->TrainFromFile();
            }
            else if (LOWORD(wParam) == IDC_RADIO_SHARED ||
                     LOWORD(wParam) == IDC_RADIO_INPROC)
            {
                pThis->UpdateRecoCtxtState();
            }
            else if (pThis->m_cpRecoCtxt)
            {
                HRESULT hr;

                if (HIWORD(wParam) == BN_CLICKED && 
                    (LOWORD(wParam) == IDC_CHECK_CFG         ||
                     LOWORD(wParam) == IDC_CHECK_DICTATION   ||
                     LOWORD(wParam) == IDC_CHECK_DICTATION_ACTIVE ||
                     LOWORD(wParam) == IDC_CHECK_CFG_ACTIVE ||
                     LOWORD(wParam) == IDC_CHECK_SPELLING   ||
                     LOWORD(wParam) == IDC_CHECK_SPELLING_ACTIVE))
                {
                    hr = pThis->UpdateGrammarState(LOWORD(wParam));
                }
                else if (LOWORD(wParam) == IDC_CHECK_RETAIN_AUDIO)
                {
                    SPAUDIOOPTIONS Opts = SPAO_NONE;
                    if (::SendDlgItemMessage( hDlg, IDC_CHECK_RETAIN_AUDIO, BM_GETCHECK, 0, 0 ) == BST_CHECKED)
                    {
                        Opts = SPAO_RETAIN_AUDIO;
                    }
                    pThis->m_cpRecoCtxt->SetAudioOptions(Opts, NULL, NULL);
                }
                else if (LOWORD(wParam) == IDC_LIST_PHRASES)
                {
                    WORD wNotify = HIWORD(wParam);
                    LONG item = ::SendDlgItemMessage(hDlg, IDC_LIST_PHRASES, LB_GETCURSEL, 0, 0);
                    CRecoDlgListItem * pli = NULL;
                    if (item != LB_ERR)
                    {
                        pli = (CRecoDlgListItem*)::SendDlgItemMessage(hDlg, IDC_LIST_PHRASES, LB_GETITEMDATA, item, 0);
                    }
                    if (wNotify == LBN_SELCHANGE)
                    {
                        pThis->UpdatePropWindow(pli);
                    }
                    else if (wNotify == LBN_DBLCLK && pli)
                    {
                        ISpRecoResult * pResult = pli->GetRecoResult();
                        if (pResult)
                        {
                            if (FAILED(pli->GetRecoResult()->SpeakAudio(NULL, 0, SPF_ASYNC, NULL)))
                            {
                                CComPtr<ISpVoice> cpVoice;
                                pThis->m_cpRecoCtxt->GetVoice(&cpVoice);
                                if (cpVoice)
                                {
                                    cpVoice->Speak(pli->GetText(), SPF_ASYNC, NULL);
                                }
                            }
                        }
                    }
                }
            }
            break;

        case WM_DRAWITEM:
            if (wParam == IDC_LIST_PHRASES)
            {
                LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;

                const CRecoDlgListItem * pli = (const CRecoDlgListItem *)pdis->itemData;

                if (pli && pli->GetText())
                {
                    HGDIOBJ hfontOld = pThis->m_hfont ? SelectObject( pdis->hDC, pThis->m_hfont ) : NULL;

                    CComPtr<ISpRecoResult> cpResult;
                    SPRECORESULTTIMES times;
                    UINT oldTextAlign = GetTextAlign(pdis->hDC);
                    UINT options = ETO_OPAQUE | ETO_CLIPPED;

                    cpResult = pli->GetRecoResult();
                    if (cpResult && cpResult->GetResultTimes(&times) == S_OK)
                    {
                        FILETIME localtime;
                        SYSTEMTIME systime;

                        if (FileTimeToLocalFileTime(&times.ftStreamTime, &localtime) &&
                            FileTimeToSystemTime(&localtime, &systime))
                        {
                            TCHAR sztime[18];
                            wsprintf(sztime, _T("[%2d:%02d:%02d.%03d] -"),
                                        systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
                            SetTextAlign(pdis->hDC, TA_UPDATECP);
                            MoveToEx(pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, NULL);
                            ExtTextOut(pdis->hDC,
                                        pdis->rcItem.left, pdis->rcItem.top,
                                        options,
                                        &pdis->rcItem,
                                        sztime, 16,
                                        NULL);
                            options = ETO_CLIPPED;
                        }
                    }

                    ExtTextOutW(pdis->hDC,
                                pdis->rcItem.left, pdis->rcItem.top,
                                options,
                                &pdis->rcItem,
                                pli->GetText(), pli->GetTextLength(),
                                NULL);

                    if (pli->IsHypothesis())
                    {
                        const WCHAR *pszHypothesis = L" [Hypothesis]";
                        ExtTextOutW(pdis->hDC,
                                pdis->rcItem.left, pdis->rcItem.top,
                                options,
                                &pdis->rcItem,
                                pszHypothesis, wcslen(pszHypothesis),
                                NULL);
                    }

                    SetTextAlign(pdis->hDC, oldTextAlign);

                    if (hfontOld)
                    {
                        SelectObject(pdis->hDC, hfontOld);
                    }
                }
            }
            break;

        case WM_DELETEITEM:
            if (wParam == IDC_LIST_PHRASES)
            {
                CRecoDlgListItem * pli = (CRecoDlgListItem *)((DELETEITEMSTRUCT *)lParam)->itemData;
                delete pli;
                lr = TRUE;
            }
            break;

    }

    return lr;
}

/****************************************************************************
* CRecoDlgClass::ConstructRuleDisplay *
*-------------------------------------*
*   Description:
*       Given the SPPHRASERULE obtained from the parsed result object,
*       writes the tree for the rules into dstr.
*       Calls itself recursively for sub-rules.
*   Returns:
*       S_OK
****************************************************************************/

HRESULT CRecoDlgClass::ConstructRuleDisplay(const SPPHRASERULE *pRule, CSpDynamicString &dstr, ULONG ulLevel)
{
    SPDBG_FUNC("CRecoDlgClass::ConstructRuleDisplay");
    HRESULT hr = S_OK;

    USES_CONVERSION;
    TCHAR szText[256];

    // construct indent
    while(SUCCEEDED(hr) && pRule)
    {
        wsprintf(szText, _T(" [%2d, %2d] "), pRule->ulFirstElement, pRule->ulFirstElement + pRule->ulCountOfElements);
        for (ULONG i = 0; i < ulLevel; i++)
        {
            dstr.Append(L"\t");
        }
        if (pRule->pszName)
        {
            if (wcslen(pRule->pszName) > 240)
            {
                dstr.Append(L"\"(Rule name too long)\" = \"");
            }
            else
            {
				WCHAR *pwszRuleName = wcsdup( pRule->pszName );
				if ( pwszRuleName )
				{
					wsprintf(szText, _T("%s \"%s\" (%d)"), szText, W2T(pwszRuleName), pRule->ulId);
					dstr.Append(T2W(szText));

					free( pwszRuleName );
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
            }
        }
        else
        {
            dstr.Append(L"\"(UNK)\" = \"");
        }
        dstr.Append(L"\r\n");
        if (pRule->pFirstChild && SUCCEEDED(hr))
        {
            hr = ConstructRuleDisplay(pRule->pFirstChild, dstr, ulLevel + 1);
        }
        pRule = pRule->pNextSibling;
    }
    return hr;
}

/****************************************************************************
* CRecoDlgClass::ConstructPropertyDisplay *
*-----------------------------------------*
*   Description:
*       Given the SPPHRASEPROPERTY obtained from the parsed result object,
*       writes the property tree into dstr.
*       Calls itself recursively for sub-properties.
*   Returns:
*       S_OK
****************************************************************************/

HRESULT CRecoDlgClass::ConstructPropertyDisplay(const SPPHRASEELEMENT *pElem, const SPPHRASEPROPERTY *pProp, 
                                                CSpDynamicString & dstr, ULONG ulLevel)
{
    SPDBG_FUNC("CRecoDlgClass::ConstructPropertyDisplay");
    HRESULT hr = S_OK;
    USES_CONVERSION;
    TCHAR szText[256];

    // construct indent
    while(SUCCEEDED(hr) && pProp)
    {
        wsprintf(szText, _T(" [%2d, %2d] "), pProp->ulFirstElement, pProp->ulCountOfElements ? pProp->ulFirstElement + pProp->ulCountOfElements-1 : 0);
        dstr.Append(T2W(szText));
        for (ULONG i = 0; i < ulLevel; i++)
        {
            dstr.Append(L"\t");
        }
        if (pProp->pszName)
        {
            if (wcslen(pProp->pszName) > 240)
            {
                dstr.Append(L"\"(Rule name too long)\" = \"");
            }
            else
            {
				WCHAR *pwszPropName = wcsdup( pProp->pszName );
				if ( pwszPropName )
				{
					wsprintf(szText, _T("\"%s\" = \""), W2T(pwszPropName));
					dstr.Append(T2W(szText));

					free( pwszPropName );
				}
				else
				{
					dstr.Append(L"Out of memory" );
				}
            }
        }
        else
        {
            dstr.Append(L"\"(UNK)\" = \"");
        }
        if (!pProp->pszValue)
        {
            // construct the value from the elements!
            ULONG ulEndElement = pProp->ulFirstElement + pProp->ulCountOfElements;
            for (ULONG j = pProp->ulFirstElement; j < ulEndElement; j++)
            {
                if (j+1 < ulEndElement)
                {
                    dstr.Append2(pElem[j].pszDisplayText, L" ");
                }
                else
                {
                    dstr.Append(pElem[j].pszDisplayText);
                }
            }
        }
        else
        {
            dstr.Append(pProp->pszValue);
        }

        if (pProp->vValue.vt != VT_EMPTY)
        {
            CComVariant cv = pProp->vValue;
            cv.ChangeType(VT_BSTR);
            wsprintf(szText, _T("\" (%d = %s)"), pProp->ulId, W2T(cv.bstrVal));
        }
        else
        {
            wsprintf(szText, _T("\" (%d)"), pProp->ulId);
        }
        dstr.Append2(T2W(szText), L"\r\n");
        if (pProp->pFirstChild)
        {
            hr = ConstructPropertyDisplay(pElem, pProp->pFirstChild, dstr, ulLevel + 1);
        }
        pProp = pProp->pNextSibling;
    }
    return hr;
}


/****************************************************************************
* CRecoDlgClass::UpdatePropWindow *
*---------------------------------*
*   Description:
*       Updates the contents of the property status window to contain a full
*   text dump of the result list item pointed to by pli.  If a NULL is passed
*   to this method, the property window will be cleared.
*
*   Returns:
*       TRUE if property window was updated
*       FALSE if it was not
*
****************************************************************************/

BOOL CRecoDlgClass::UpdatePropWindow(const CRecoDlgListItem * pli)
{
    BOOL fOK = FALSE;
    
    HWND hwndStatus = ::GetDlgItem(m_hDlg, IDC_STATUS);
    ISpPhrase * pPhrase = pli ? pli->GetRecoResult() : NULL;
    SPPHRASE *pElements;

    if (pli && pPhrase && SUCCEEDED(pPhrase->GetPhrase(&pElements)))
    {
        USES_CONVERSION;
        TCHAR szText[256];

        CSpDynamicString dstr;

        if (pElements->Rule.pszName || pElements->Rule.ulId)
        {
			WCHAR *pwszRuleName = NULL;
            if ( pElements->Rule.pszName )
            {
                pwszRuleName = wcsdup( pElements->Rule.pszName );
            }
            else
            {
                pwszRuleName = wcsdup( L"<no name>" );
            }

			if ( pwszRuleName )
			{
				wsprintf(szText, _T("RULE=\"%s\" (%05d)\r\n\r\n"), W2T(pwszRuleName), pElements->Rule.ulId);
				dstr.Append(T2W(szText));
				dstr.Append(pli->GetText());

				free( pwszRuleName );
			}
			else
			{
				dstr.Append( L"Out of memory" );
			}
        }
        else
        {
            dstr = L"DICTATION\r\n\r\n";
            CSpDynamicString dstrText;
            pPhrase->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
            dstr.Append(dstrText);
        }
        dstr.Append(L"\r\n");
        // Display the overall confidence. This is specified on a per-rule basis.
        // However, the top rule contains everything and hence is the phrase confidence.
        wsprintf(szText, _T("\r\nOverall confidence = %i.\r\n"), pElements->Rule.Confidence);
        dstr.Append(T2W(szText));
        if (pElements->pProperties)
        {
            dstr.Append(L"\r\nPROPERTIES:\r\n");
            ConstructPropertyDisplay(pElements->pElements, pElements->pProperties, dstr, 0);
            dstr.Append(L"\r\n");
        }

        dstr.Append(L"\r\nPARSE TREE:\r\n");
        ConstructRuleDisplay(&pElements->Rule, dstr, 0);

        dstr.Append(L"\r\nELEMENTS:\r\n");
        for (ULONG i = 0; i < pElements->Rule.ulCountOfElements; i++)
        {
            WCHAR    wszIntPhone[SP_MAX_PRON_LENGTH];
            wszIntPhone[0] = L'\0';
            if ( pElements->pElements[i].pszPronunciation )
            {
                m_cpPhoneConv->IdToPhone(pElements->pElements[i].pszPronunciation, wszIntPhone);
            }

			WCHAR *pwszDisplayText = wcsdup( pElements->pElements[i].pszDisplayText );
            wsprintf(szText, _T(" <%u - %u> \"%s\" %c(%c) {%S}\r\n"), 
                pElements->pElements[i].ulAudioStreamOffset,
                pElements->pElements[i].ulAudioStreamOffset + pElements->pElements[i].ulAudioSizeBytes,
                pwszDisplayText ? W2T(pwszDisplayText) : _T("(out of memory)"), 
                ConfidenceGroupChar(pElements->pElements[i].ActualConfidence),
                ConfidenceGroupChar(pElements->pElements[i].RequiredConfidence),
                wszIntPhone);
			if ( pwszDisplayText )
			{
				free( pwszDisplayText );
			}
            dstr.Append(T2W(szText));
        }

        ::CoTaskMemFree(pElements);

        SPRECORESULTTIMES times;
        pli->GetRecoResult()->GetResultTimes(&times);

        DWORD   dwTickEnd = times.dwTickCount + (DWORD)(times.ullLength / 10000);
        DWORD   dwTickNow = GetTickCount();

        wsprintf(szText, _T("GetTickCount latency = (%u - %u) = %dms\r\n"), dwTickNow, dwTickEnd, dwTickNow - dwTickEnd);
        ATLTRACE(szText);
        dstr.Append(T2W(szText));

        ATLTRACE(L"\r\n");

        ::SendMessage(hwndStatus, WM_SETTEXT, 0, (LPARAM)W2T(dstr));

        fOK = TRUE;
    }
    else
    {
        ::SendMessage(hwndStatus, WM_SETTEXT, 0, (LPARAM)TEXT(""));
    }

    return fOK;
}


/****************************************************************************
* CRecoDlgClass::UpdateGrammarState *
*-----------------------------------*
*   Description:
*       This method is called whenever a check box state has changed to update
*   the state of grammars.  It will create or release m_cpCFGGrammar and 
*   m_cpDictationGrammar and activate or deactivate either of them.
*
*       If at any point during this function, a SAPI call fails, the state of
*   the application will be reset.
*
*   Returns:
*       S_OK, or failed HRESULT from SAPI calls made below
*
****************************************************************************/

HRESULT CRecoDlgClass::UpdateGrammarState(WORD wIdChanged)
{
    #ifdef _DEBUG
    static BOOL fInUpdate = FALSE;
    _ASSERTE(!fInUpdate);
    fInUpdate = TRUE;
    #endif // _DEBUG
    
    HRESULT hr = S_OK;

    if (m_cpRecoCtxt)           // only if there is a recognition context
    {
        BOOL fActivateCFG = ::SendDlgItemMessage(m_hDlg, IDC_CHECK_CFG_ACTIVE, BM_GETCHECK, 0, 0);
        BOOL fActivateDictation = ::SendDlgItemMessage(m_hDlg, IDC_CHECK_DICTATION_ACTIVE, BM_GETCHECK, 0, 0);
        BOOL fLoadDictation = ::SendDlgItemMessage( m_hDlg, IDC_CHECK_DICTATION, BM_GETCHECK, 0, 0 );
        BOOL fLoadCFG = ::SendDlgItemMessage( m_hDlg, IDC_CHECK_CFG, BM_GETCHECK, 0, 0 );
        BOOL fActivateSpelling = ::SendDlgItemMessage(m_hDlg, IDC_CHECK_SPELLING_ACTIVE, BM_GETCHECK, 0, 0);
        BOOL fLoadSpelling = ::SendDlgItemMessage( m_hDlg, IDC_CHECK_SPELLING, BM_GETCHECK, 0, 0 );
        SPSTATEHANDLE   hDynRule = NULL;

        // Force the pair of checks to be in sync.  If the ID is 0 then no checkbox
        // has changed state, so we don't have to do anything.
        WORD wIdToUpdate = 0;  
        WPARAM NewState;
        switch (wIdChanged)
        {
        case IDC_CHECK_CFG:
            if ((!fLoadCFG) && fActivateCFG) 
            {
                fActivateCFG = FALSE;
                wIdToUpdate = IDC_CHECK_CFG_ACTIVE;
                NewState = BST_UNCHECKED;
            }
            break;
        case IDC_CHECK_DICTATION:
            if ((!fLoadDictation) && fActivateDictation) 
            {
                fActivateDictation = FALSE;
                wIdToUpdate = IDC_CHECK_DICTATION_ACTIVE;
                NewState = BST_UNCHECKED;
            }
            break;
        case IDC_CHECK_SPELLING:
            if ((!fLoadSpelling) && fActivateSpelling) 
            {
                fActivateSpelling = FALSE;
                wIdToUpdate = IDC_CHECK_SPELLING_ACTIVE;
                NewState = BST_UNCHECKED;
            }
            break;
        case IDC_CHECK_CFG_ACTIVE:
            if (fActivateCFG && (!fLoadCFG))
            {
                fLoadCFG = TRUE;
                wIdToUpdate = IDC_CHECK_CFG;
                NewState = BST_CHECKED;
            }
            break;
        case IDC_CHECK_DICTATION_ACTIVE:
            if (fActivateDictation && (!fLoadDictation))
            {
                fLoadDictation = TRUE;
                wIdToUpdate = IDC_CHECK_DICTATION;
                NewState = BST_CHECKED;
            }
            break;
        case IDC_CHECK_SPELLING_ACTIVE:
            if (fActivateSpelling && (!fLoadSpelling))
            {
                fLoadSpelling = TRUE;
                wIdToUpdate = IDC_CHECK_SPELLING;
                NewState = BST_CHECKED;
            }
            break;
        }
        if (wIdToUpdate)
        {
            ::SendDlgItemMessage(m_hDlg, wIdToUpdate, BM_SETCHECK, NewState, 0);
        }

        //
        //  Now load the appropriate grammar(s)...
        //
        UINT uiErrorMessageID = 0;
        if (fLoadCFG)
        {
            if (!m_cpCFGGrammar)
            {
                //
                //  If the first char of m_szGrammarFile is 0 then load the solitaire grammar
                //  from our resource.  Otherwise, load the grammar from the file.
                //
                hr = m_cpRecoCtxt->CreateGrammar(MYGRAMMARID, &m_cpCFGGrammar);

                if (*m_szGrammarFile)
                {
                    USES_CONVERSION;
                    if (SUCCEEDED(hr))
                    {
                        hr = m_cpCFGGrammar->LoadCmdFromFile(T2W(m_szGrammarFile), SPLO_DYNAMIC);
                    }
                }
                else
                {
                // Need a method of determining the language id of the grammar resource we
                // want to load. This is dependent solely on the primary language of the engine the
                // user has selected. Hence, the resources are labelled 'Primary', SUBLANG_NEUTRAL
                // and we use the primary language id of the engine to do the LoadCmdFromResource.
                    hr = m_cpCFGGrammar->LoadCmdFromResource(NULL, MAKEINTRESOURCEW(IDR_SOL_CFG),
                                L"SRGRAMMAR", MAKELANGID(PRIMARYLANGID(m_langid), SUBLANG_NEUTRAL), SPLO_DYNAMIC);
                }

                // Load dynamic rules, if any.
                if (SUCCEEDED(hr) && m_ItemList.GetHead())
                {
                    hr = m_cpCFGGrammar->GetRule(g_szDynRuleName, NULL, SPRAF_TopLevel | SPRAF_Dynamic, TRUE, &hDynRule);
                    if (SUCCEEDED(hr))
                    {
                        CDynItem *pItem = m_ItemList.GetHead();
                        while (pItem && SUCCEEDED(hr))
                        {
                            hr = m_cpCFGGrammar->AddWordTransition(hDynRule, NULL, pItem->m_dstr, L" ", SPWT_LEXICAL, 1.0, NULL);
                            pItem = m_ItemList.GetNext(pItem);
                        }

                        if (SUCCEEDED(hr))
                            hr = m_cpCFGGrammar->Commit(0);
                    }
                }
            }
        }
        else
        {
            ::SendDlgItemMessage(m_hDlg, IDC_CHECK_CFG_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0);
            m_cpCFGGrammar.Release();
            m_cpCFGVoice.Release();
        }

        if (SUCCEEDED(hr))
        {
            if (fLoadDictation)
            {
                if (!m_cpDictationGrammar)
                {
                    hr = m_cpRecoCtxt->CreateGrammar(MYGRAMMARID, &m_cpDictationGrammar);

                    if (SUCCEEDED(hr))
                    {
                        hr = m_cpDictationGrammar->LoadDictation(NULL, SPLO_STATIC);
                    }
                }
            }
            else
            {
                ::SendDlgItemMessage(m_hDlg, IDC_CHECK_DICTATION_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0);
                m_cpDictationGrammar.Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            if (fLoadSpelling)
            {
                if (!m_cpSpellingGrammar)
                {
                    hr = m_cpRecoCtxt->CreateGrammar(MYGRAMMARID, &m_cpSpellingGrammar);
                    if (SUCCEEDED(hr))
                    {
                        hr = m_cpSpellingGrammar->LoadDictation(SPTOPIC_SPELLING, SPLO_STATIC);
                        if (FAILED(hr))
                        {
                            m_cpSpellingGrammar.Release();
                        }
                    }
                }
            }
            else
            {
                ::SendDlgItemMessage(m_hDlg, IDC_CHECK_SPELLING_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0);
                m_cpSpellingGrammar.Release();
            }
        }
        
        // If we've failed by now, it's a grammar load failure
        if ( FAILED( hr ) )
        {
            uiErrorMessageID = IDS_GRAMMAR_LOAD_FAIL;
        }

        if (SUCCEEDED(hr) && m_cpCFGGrammar)
        {
            hr = m_cpCFGGrammar->SetRuleState(NULL, NULL, fActivateCFG ? SPRS_ACTIVE : SPRS_INACTIVE);

            if (SUCCEEDED(hr) && hDynRule)
            {
                hr = m_cpCFGGrammar->SetRuleState(g_szDynRuleName, NULL, fActivateCFG ? SPRS_ACTIVE : SPRS_INACTIVE);
            }
        }

        if (SUCCEEDED(hr) && m_cpDictationGrammar)
        {
            hr = m_cpDictationGrammar->SetDictationState(fActivateDictation ? SPRS_ACTIVE : SPRS_INACTIVE);
        }

        if (SUCCEEDED(hr) && m_cpSpellingGrammar)
        {
            hr = m_cpSpellingGrammar->SetDictationState(fActivateSpelling ? SPRS_ACTIVE : SPRS_INACTIVE);
        }

        // If we have failed here, but the grammar load succeeded, then it's a 
        // SetRuleState/SetDictationState failure
        if ( FAILED( hr ) && (0 == uiErrorMessageID) )
        {
            if ( SPERR_DEVICE_BUSY == hr )
            {
                uiErrorMessageID = IDS_DEVICE_BUSY;
            }
            else
            {
                uiErrorMessageID = IDS_RULESTATE_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            HMENU hMenu = ::GetMenu(m_hDlg);
            ::EnableMenuItem(hMenu, IDM_CFG_LOAD_GRAMMAR, fLoadCFG ? MF_GRAYED : MF_ENABLED);
            ::EnableMenuItem(hMenu, IDM_CFG_ADD_DYNAMIC_RULE, fLoadCFG ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_CFG_SET_WORD_SEQUENCE_DATA, fLoadCFG ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_SLM_TRAIN_FROM_FILE, m_cpRecognizer ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_SLM_ADAPT_FROM_FILE, m_cpDictationGrammar ? MF_ENABLED : MF_GRAYED);
            ::EnableMenuItem(hMenu, IDM_SLM_ADAPT_FROM_CLIPBOARD, m_cpDictationGrammar ? MF_ENABLED : MF_GRAYED);
        }
        else
        {
            MessageBoxFromResource( uiErrorMessageID );
            Reset();
        }
    }

    #ifdef _DEBUG
    fInUpdate = FALSE;
    #endif // _DEBUG
    
    return hr;
}


/****************************************************************************
* CRecoDlgClass::UpdateRecoCtxtState *
*------------------------------------*
*   Description:
*       Creates or releases the m_cpRecoContext member based on the state of the
*   ICD_CHECK_CREATE checkbox.  This method also enables or disables various controls
*   based on weather or not there is a m_cpRecoContext object.  If for any reason
*   one of the SAPI calls fails, the application state will be reset.
*
*   Returns:
*       S_OK
*       Return value of CRecoDlgClass::CreateRecoCtxt()
*       Return value of CRecoDlgClass::UpdateGrammarState()
*
****************************************************************************/

HRESULT CRecoDlgClass::UpdateRecoCtxtState()
{
    HRESULT hr = S_OK;
    BOOL bCreateChecked = (::SendDlgItemMessage(m_hDlg, IDC_CHECK_CREATE, BM_GETCHECK, 0, 0 ) == BST_CHECKED);
    if (bCreateChecked)
    {
        if (!m_cpRecoCtxt)
        {
            BOOL bShared = (::SendDlgItemMessage(m_hDlg, IDC_RADIO_SHARED, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (bShared)
            {
                hr = CreateRecoCtxt(0);
            }
            else
            {
                HWND hwndList = ::GetDlgItem(m_hDlg, IDC_COMBO_ENGINES);
                LONG i = ::SendMessage(hwndList, CB_GETCURSEL, 0, 0);
                hr = CreateRecoCtxt(::SendMessage(hwndList, CB_GETITEMDATA, i, 0));
            }
        }

        if( SUCCEEDED(hr) )
        {
            hr = UpdateGrammarState(0);
            if (FAILED(hr))
            {
                Reset();
            }
        }
    }
    else
    {
        Reset();
    }
    BOOL bEnable = (m_cpRecoCtxt != NULL);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_LIST_PHRASES), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_CFG), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_CFG_ACTIVE), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_MIC), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_DICTATION), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_DICTATION_ACTIVE), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_SPELLING), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_SPELLING_ACTIVE), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_RETAIN_AUDIO), bEnable);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_RADIO_SHARED), m_cpRecoCtxt == NULL);
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_RADIO_INPROC), m_cpRecoCtxt == NULL);
    BOOL bEnableCombo = (m_cpRecoCtxt == NULL);
    if (bEnableCombo)
    {
        bEnableCombo = (::SendDlgItemMessage(m_hDlg, IDC_RADIO_INPROC, BM_GETCHECK, 0, 0) == BST_CHECKED);
    }
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_COMBO_ENGINES), bEnableCombo);
    if (!bEnable)
    {
        UpdatePropWindow(NULL);
    }
    return hr;
}

/****************************************************************************
* CRecoDlgClass::InitDialog *
*---------------------------*
*   Description:
*       Enumerates the available SR engines and populates the combo box.  
*   Initializes the application to the default starting state.
*
*   Returns:
*       TRUE if successful, else FALSE
*
****************************************************************************/

BOOL CRecoDlgClass::InitDialog(HWND hDlg)
{
    HRESULT hr = S_OK;
    m_hDlg = hDlg;

    //
    //  Initialize the list of engines.  The shared context will be indicated by a NULL
    //  GUID pointer (a 0 item data).  The default inproc engine will be GUID_NULL, and all
    //  other engines will have a GUID allocated for them.
    //
    CComPtr<IEnumSpObjectTokens> cpEnum;
    if (SUCCEEDED(hr))
    {
        hr = SpEnumTokens(SPCAT_RECOGNIZERS, NULL, NULL, &cpEnum);
    }
    if (FAILED(hr))
    {
        return FALSE;
    }

    CComPtr<ISpObjectToken> cpDefaultRecoToken;
    hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpDefaultRecoToken);

    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrDefaultName;
        if (SUCCEEDED(SpGetDescription(cpDefaultRecoToken, &dstrDefaultName)))
        {
            CSpDynamicString dstrDesc(L"Shar&ed:  ");
            dstrDesc.Append(dstrDefaultName);
            USES_CONVERSION;
            ::SendDlgItemMessage(hDlg, IDC_RADIO_SHARED, WM_SETTEXT, 0, (LPARAM)W2T(dstrDesc));
        }
    }

    //
    //  Now set up the list.
    //
    HWND hwndList = ::GetDlgItem(hDlg, IDC_COMBO_ENGINES);

    ISpObjectToken * pEngineToken;

    while (cpEnum->Next(1, &pEngineToken, NULL) == S_OK)
    {
        CSpDynamicString dstrName;
        if (SUCCEEDED(SpGetDescription(pEngineToken, &dstrName)))
        {
            USES_CONVERSION;
            LRESULT NewItem = ::SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)W2T(dstrName));
            ::SendMessage(hwndList, CB_SETITEMDATA, NewItem, (LPARAM)pEngineToken);
        }
        else
        {
            pEngineToken->Release();
        }
    }

    ::SendMessage(hwndList, CB_SETCURSEL, 0, 0);
    ::SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)::LoadIconA(m_hInstance, MAKEINTRESOURCEA(IDI_RECO)));

    ::SendDlgItemMessage( m_hDlg, IDC_CHECK_MIC, BM_SETCHECK, BST_CHECKED, 0 );
    ::SendDlgItemMessage( m_hDlg, IDC_RADIO_SHARED, BM_SETCHECK, BST_CHECKED, 0 );

    UpdateRecoCtxtState();
    UpdateGrammarStatusWindow();

    //
    // Phrase window is owner draw, in order to prevent dataloss in thunking
    // We keep the Unicode string in the CRecoDlgListItem and call ExtTextOutW.
    //

    HWND hwndPhrase = GetDlgItem(hDlg, IDC_LIST_PHRASES);
    LONG l = GetWindowLong(hwndPhrase, GWL_STYLE);
    SetWindowLong(hwndPhrase, GWL_STYLE, l & ~LBS_HASSTRINGS);

    return TRUE;
}


/****************************************************************************
* CRecoDlgClass::Cleanup *
*------------------------*
*   Description:
*       Called when the application is shutting down.
*
*   Returns:
*       void
*
****************************************************************************/

void CRecoDlgClass::Cleanup()
{
    //
    //  Release all the engine tokens in the combo box
    //
    HWND hwndList = ::GetDlgItem(m_hDlg, IDC_COMBO_ENGINES);
    LONG NumEntries = ::SendMessage(hwndList, CB_GETCOUNT, 0, 0);
    for (LONG i = 0; i < NumEntries; i++)
    {
        LRESULT ItemData = ::SendMessage(hwndList, CB_GETITEMDATA, i, 0);
        if (ItemData)
        {
            ((IUnknown *)ItemData)->Release();
        }
    }

    Reset();

    if (m_hfont)
    {
        ::DeleteObject(m_hfont);
    }
}


/****************************************************************************
* CRecoDlgClass::CreateRecoCtxt *
*-------------------------------*
*   Description:
*       This method is only called from UpdateRecoCtxtState.  It is passed the
*   item data from selected item in the IDC_COMBO_ENGINES combo box.  If the item
*   data is NULL then the "Shared default engine" is selected.  If it is non-NULL
*   then the item data points to an SpObjectToken for the selected engine to
*   load inproc.
*
*   Returns:
*       S_OK, or failed HRESULT of SAPI initialization calls made below
*
****************************************************************************/

HRESULT CRecoDlgClass::CreateRecoCtxt(LRESULT ItemData)
{
    HRESULT hr = S_OK;

    //
    // ItemData is NULL, for shared case.  In the inproc case, we are REQUIRED to set the
    // input so we create the default audio object.
    //
    if (ItemData)
    {
        hr = m_cpRecognizer.CoCreateInstance(CLSID_SpInprocRecognizer);
        if (SUCCEEDED(hr))
        {
            hr = m_cpRecognizer->SetRecognizer((ISpObjectToken *)ItemData);
        }
        if (SUCCEEDED(hr))
        {
            CComPtr<ISpObjectToken> cpAudioToken;
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOIN, &cpAudioToken);
            if (SUCCEEDED(hr))
            {
                hr = m_cpRecognizer->SetInput(cpAudioToken, TRUE);
            }
        }
    }
    else
    {
        hr = m_cpRecognizer.CoCreateInstance(CLSID_SpSharedRecognizer);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecognizer->CreateRecoContext(&m_cpRecoCtxt);
    }
    if (SUCCEEDED(hr))
    {
        hr = m_cpRecoCtxt->SetNotifyWindowMessage(m_hDlg, WM_RECOEVENT, 0, 0);
    }
    if (SUCCEEDED(hr))
    {
        const ULONGLONG ullInterest = SPFEI(SPEI_SOUND_START) | SPFEI(SPEI_SOUND_END) |
                                      SPFEI(SPEI_PHRASE_START) | SPFEI(SPEI_RECOGNITION) |
                                      SPFEI(SPEI_FALSE_RECOGNITION) | SPFEI(SPEI_HYPOTHESIS) |
                                      SPFEI(SPEI_INTERFERENCE) | SPFEI(SPEI_RECO_OTHER_CONTEXT) |
                                      SPFEI(SPEI_REQUEST_UI) | SPFEI(SPEI_RECO_STATE_CHANGE) |
                                      SPFEI(SPEI_PROPERTY_NUM_CHANGE) | SPFEI(SPEI_PROPERTY_STRING_CHANGE);
        hr = m_cpRecoCtxt->SetInterest(ullInterest, ullInterest);
    }

    if( SUCCEEDED(hr) )
    {
        //--- Set the max number of desired alternates
        hr = m_cpRecoCtxt->SetMaxAlternates( 3 );
    }

    SPRECOGNIZERSTATUS stat;

    // Get locale/font settings
    if (SUCCEEDED(hr))
    {
        ZeroMemory(&stat, sizeof(stat));
        hr = m_cpRecognizer->GetStatus(&stat);
    }

    if (SUCCEEDED(hr))
    {
        m_langid = stat.aLangID[0];

        // Pick an appropriate font.  On Windows 2000, let the system fontlink.

        DWORD dwVersion = GetVersion();

        if (   dwVersion >= 0x80000000
            || LOBYTE(LOWORD(dwVersion)) < 5
            || m_langid != MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US))
        {
            TCHAR achCodePage[6];
            UINT uiCodePage;

            if (0 != GetLocaleInfo(MAKELCID(m_langid, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, achCodePage, 6))
            {
                uiCodePage = _ttoi(achCodePage);
            }
            else
            {
                uiCodePage = GetACP();
            }

            CComPtr<IMultiLanguage> cpMultiLanguage;
            MIMECPINFO MimeCpInfo;

            if (   SUCCEEDED(cpMultiLanguage.CoCreateInstance(CLSID_CMultiLanguage))
                && SUCCEEDED(cpMultiLanguage->GetCodePageInfo(uiCodePage, &MimeCpInfo)))
            {
                USES_CONVERSION;

                if (m_hfont)
                {
                    DeleteObject(m_hfont);
                }

                m_hfont = CreateFont(0, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                     MimeCpInfo.bGDICharset,
                                     OUT_DEFAULT_PRECIS,
                                     CLIP_DEFAULT_PRECIS,
                                     DEFAULT_QUALITY,
                                     DEFAULT_PITCH,
                                     W2T(MimeCpInfo.wszProportionalFont));

            }
        }
    }

    if (SUCCEEDED(hr))
    {
        m_cpPhoneConv.Release();
        hr = SpCreatePhoneConverter(m_langid, NULL, NULL, &m_cpPhoneConv);
    }

    if (FAILED(hr))
    {
        MessageBoxFromResource( IDS_RECOCONTEXT_FAIL );
        Reset();
    }
    return hr;
}

/****************************************************************************
* CRecoDlgClass::UpdateGrammarStatusWindow *
*------------------------------------------*
*   Description:
*       Called whenever the grammar file has changed to set the text of the
*   grammar status control (a line in the dialog box below the property window).    
*
*   Returns:
*       void
*
****************************************************************************/

void CRecoDlgClass::UpdateGrammarStatusWindow()
{
    const static TCHAR szPrefixText[] = _T("Current C&&C Grammar: ");
    TCHAR szDesc[sp_countof(m_szGrammarFileTitle) + sp_countof(szPrefixText)];
    _tcscpy(szDesc, szPrefixText);
    _tcscat(szDesc, *m_szGrammarFile ? m_szGrammarFileTitle : _T("Solitaire (built-in demo grammar)"));
    ::SendDlgItemMessage(m_hDlg, IDC_GRAMMAR_STATUS, WM_SETTEXT, 0, (LPARAM)szDesc);
}


/****************************************************************************
* CRecoDlgClass::Reset *
*----------------------*
*   Description:
*       Shutdown and release all of the objects associated with an active
*   recognition context and reset UI elements to their initial state.
*
*   Returns:
*       void
*
****************************************************************************/

void CRecoDlgClass::Reset()
{
    UpdatePropWindow(NULL);
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_CFG_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_CFG, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_DICTATION_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_DICTATION, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_SPELLING_ACTIVE, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_SPELLING, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_CREATE, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_RETAIN_AUDIO, BM_SETCHECK, BST_UNCHECKED, 0 );
    ::SendDlgItemMessage (m_hDlg, IDC_CHECK_MIC, BM_SETCHECK, BST_CHECKED, 0 );
    ::EnableWindow(::GetDlgItem(m_hDlg, IDC_CHECK_MIC), 0);

    HMENU hMenu = ::GetMenu(m_hDlg);
    ::EnableMenuItem(hMenu, IDM_CFG_LOAD_GRAMMAR, MF_ENABLED);
    ::EnableMenuItem(hMenu, IDM_CFG_ADD_DYNAMIC_RULE, MF_GRAYED);
    ::EnableMenuItem(hMenu, IDM_CFG_SET_WORD_SEQUENCE_DATA, MF_GRAYED);
    ::EnableMenuItem(hMenu, IDM_SLM_TRAIN_FROM_FILE, MF_GRAYED);
    ::EnableMenuItem(hMenu, IDM_SLM_ADAPT_FROM_FILE, MF_GRAYED);
    ::EnableMenuItem(hMenu, IDM_SLM_ADAPT_FROM_CLIPBOARD, MF_GRAYED);

    ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_RESETCONTENT, 0, 0);

    m_cpCFGGrammar.Release();
    m_cpCFGVoice.Release();
    m_cpRecoCtxt.Release();
    m_cpDictationGrammar.Release();
    m_cpSpellingGrammar.Release();
    m_cpRecognizer.Release();

    m_bInSound = FALSE;
    m_bGotReco = FALSE;
}

/****************************************************************************
* CRecoDlgClass::RecoEvent *
*--------------------------*
*   Description:
*       Whenever a notification is sent from the m_cpRecoCtxt object, this
*   method is called.  When a recognition event occurs, it adds an item to
*   the listbox.  If a sound start / sound end occurs without a recogntion
*   then it inserts a "<noise>" string in the listbox.
*
*   Returns:
*       void
*
****************************************************************************/

void CRecoDlgClass::RecoEvent()
{
    USES_CONVERSION;
    CSpEvent event;
    long iNewPhrase;

    if (m_cpRecoCtxt)
    {
        while (event.GetFrom(m_cpRecoCtxt) == S_OK)
        {
            switch (event.eEventId)
            {
                case SPEI_REQUEST_UI:
                    if (event.RequestTypeOfUI() != NULL)
                    {
                        #ifdef _DEBUG
                        SPRECOCONTEXTSTATUS recostatus;
                        m_cpRecoCtxt->GetStatus(&recostatus);
                        _ASSERTE(wcscmp(recostatus.szRequestTypeOfUI, event.RequestTypeOfUI()) == 0);
                        #endif // _DEBUG

                        m_cpRecognizer->DisplayUI(m_hDlg, L"Basic Speech Recognition", event.RequestTypeOfUI() , NULL, 0);
                    }
                    else
                    {
                        #ifdef _DEBUG
                        SPRECOCONTEXTSTATUS recostatus;
                        m_cpRecoCtxt->GetStatus(&recostatus);
                        _ASSERTE(recostatus.szRequestTypeOfUI[0] == '\0');
                        #endif // _DEBUG
                    }
                    break;
                    
            case SPEI_INTERFERENCE: 
                { 
                     USES_CONVERSION;
                     CSpDynamicString dstr;
                     switch(event.Interference())
                     {
                     case SPINTERFERENCE_NONE:
                         dstr = L"Interference - None";
                         break;
                     case SPINTERFERENCE_NOISE:
                         dstr = L"Interference - Noise";
                         break;
                     case SPINTERFERENCE_NOSIGNAL:
                         dstr = L"Interference - No signal";
                         break;
                     case SPINTERFERENCE_TOOLOUD:
                         dstr = L"Interference - Too loud";
                         break;
                     case SPINTERFERENCE_TOOQUIET:
                         dstr = L"Interference - Too quiet";
                         break;
                     case SPINTERFERENCE_TOOFAST:
                         dstr = L"Dictation mode: Interference - Too fast";
                         break;
                     case SPINTERFERENCE_TOOSLOW:
                         dstr = L"Dictation mode: Interference - Too slow";
                         break;
                     default:
                         dstr = L"Unrecognized Interference Event";
                     }
                        CRecoDlgListItem * pli = new CRecoDlgListItem(NULL, dstr, FALSE);
                        if (pli)
                        {
                            iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                            ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                        }
                    }
                    break;

                case SPEI_PROPERTY_NUM_CHANGE:
                    {
                        USES_CONVERSION;
                        TCHAR sz[MAX_PATH * 2];
                        WCHAR *pwszPropertyName = NULL;
                        if ( event.PropertyName() )
                        {
						    pwszPropertyName = wcsdup( event.PropertyName() );
                        }
                        else
                        {
                            pwszPropertyName = wcsdup( L"<no name>" );
                        }
                        wsprintf(sz, _T("Attrib change:  %s=%d"), 
							pwszPropertyName ? W2T(pwszPropertyName) : _T("(out of memory)"), 
							event.PropertyNumValue());
						if ( pwszPropertyName )
						{
							free( pwszPropertyName );
						}
                        CRecoDlgListItem * pli = new CRecoDlgListItem(NULL, T2W(sz), FALSE);
                        iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                        ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                    }
                    break;

                case SPEI_PROPERTY_STRING_CHANGE:
                    {
                        USES_CONVERSION;
                        TCHAR sz[MAX_PATH * 2];
						WCHAR *pwszPropertyName = NULL;
                        if ( event.PropertyName() )
                        {
                            pwszPropertyName = wcsdup( event.PropertyName() );
                        }
                        else
                        {
                            pwszPropertyName = wcsdup( L"<no name>" );
                        }

						WCHAR *pwszPropertyStringValue = NULL;
                        if ( event.PropertyStringValue() )
                        {
                            pwszPropertyStringValue = wcsdup( event.PropertyStringValue() );
                        }
                        else
                        {
                            pwszPropertyStringValue = wcsdup( L"<no string value>" );
                        }
                        wsprintf(sz, _T("Attrib change:  %s=%s"), 
							pwszPropertyName ? W2T(pwszPropertyName) : _T("(out of memory)"), 
							pwszPropertyStringValue ? W2T(pwszPropertyStringValue): _T("(out of memory)") );
                        if ( pwszPropertyName )
						{
							free( pwszPropertyName );
						}
						if ( pwszPropertyStringValue )
						{
							free( pwszPropertyStringValue );
						}
						CRecoDlgListItem * pli = new CRecoDlgListItem(NULL, T2W(sz), FALSE);
                        iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                        ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                    }
                    break;
                
                case SPEI_RECO_STATE_CHANGE:
                    ::SendDlgItemMessage( m_hDlg, IDC_CHECK_MIC, BM_SETCHECK,
                                        (event.RecoState() == SPRST_INACTIVE) ? BST_UNCHECKED : BST_CHECKED, 0 );
                    break;



                case SPEI_SOUND_START:
                    m_bInSound = TRUE;
                    break;

                case SPEI_SOUND_END:
                    if (m_bInSound)
                    {
                        m_bInSound = FALSE;
                        if (!m_bGotReco)
                        {
                            const WCHAR wszNoise[] = L"(Noise without speech)";
                            CRecoDlgListItem * pli = new CRecoDlgListItem(NULL, wszNoise, FALSE);
                            iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                            ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                        }
                        m_bGotReco = FALSE;
                    }
                    break;

                case SPEI_RECO_OTHER_CONTEXT:
                    {
                        m_bGotReco = TRUE;
                        CRecoDlgListItem * pli = new CRecoDlgListItem(NULL, L"(Recognition for other client)", FALSE);
                        iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                        ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                    }
                    break;


                case SPEI_FALSE_RECOGNITION:
                case SPEI_HYPOTHESIS:
                case SPEI_RECOGNITION:
                    {
                        CComPtr<ISpRecoResult> cpResult;
                        cpResult = event.RecoResult();

                        m_bGotReco = TRUE;

                        CRecoDlgListItem * pli;

                        CSpDynamicString dstrText;
                        if (event.eEventId == SPEI_FALSE_RECOGNITION)
                        {
                            dstrText = L"(Unrecognized)";
                        }
                        else
                        {
                            cpResult->GetText(SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, TRUE, &dstrText, NULL);
                        }

                        pli = new CRecoDlgListItem(cpResult, dstrText, event.eEventId == SPEI_HYPOTHESIS);
                        iNewPhrase = ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_ADDSTRING, 0, (LPARAM)pli);
                        ::SendDlgItemMessage(m_hDlg, IDC_LIST_PHRASES, LB_SETCURSEL, iNewPhrase, 0);
                        UpdatePropWindow(pli);
                        
                        cpResult.Release();
                    }
                    break;
            }
        }
    }
}

/****************************************************************************
* CRecoDlgClass::EmulateRecognition *
*-----------------------------------*
*   Description:
*       Calls ISpRecognizer::EmulateRecognition to parse the phrase
*       built out of the text in pszText
*   Returns:
*       void
****************************************************************************/

void CRecoDlgClass::EmulateRecognition(WCHAR *pszText)
{
    SPDBG_FUNC("CRecoDlgClass::EmulateRecognition");
    HRESULT hr = S_OK;

    CComPtr<ISpPhraseBuilder> cpPhrase;
    if (SUCCEEDED(CreatePhraseFromText(pszText, &cpPhrase, m_langid, m_cpPhoneConv)) && m_cpRecognizer)
    {
        hr = m_cpRecognizer->EmulateRecognition(cpPhrase);
    }
}



/****************************************************************************
* CRecoDlgClass::SpecifyCAndCGrammar *
*------------------------------------*
*   Description:
*       This method is called when the "C&C Grammar" button is pressed on the
*   main application dialog.  It opens the standard file dialog so that the user
*   can specify a command and control grammar to be used by the application.  If
*   the user cancels out of the dialog, then this method resets the m_szGrammarFile
*   member so that the built-in grammar will be used.
*
*   Returns:
*       void
*
****************************************************************************/

void CRecoDlgClass::SpecifyCAndCGrammar()
{
    OPENFILENAME OpenFileName;
    const static TCHAR szFilter[] = _T("Grammar files\0*.cfg;*.xml\0All files\0*.*\0");
	size_t ofnsize = (BYTE*)&OpenFileName.lpTemplateName + sizeof(OpenFileName.lpTemplateName) - (BYTE*)&OpenFileName;

    // Fill in the OPENFILENAME structure to support a template and hook.
    OpenFileName.lStructSize       = ofnsize;
    OpenFileName.hwndOwner         = m_hDlg;
    OpenFileName.hInstance         = m_hInstance;
    OpenFileName.lpstrFilter       = szFilter;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter    = 0;
    OpenFileName.nFilterIndex      = 0;
    OpenFileName.lpstrFile         = m_szGrammarFile;
    OpenFileName.nMaxFile          = sp_countof(m_szGrammarFile);
    OpenFileName.lpstrFileTitle    = m_szGrammarFileTitle;
    OpenFileName.nMaxFileTitle     = sp_countof(m_szGrammarFileTitle);
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = _T("Select a grammar file");
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lpstrDefExt       = _T("cfg");
    OpenFileName.lCustData         = 0;
    OpenFileName.lpfnHook          = NULL;
    OpenFileName.lpTemplateName    = NULL;
    OpenFileName.Flags             = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    // Call the common dialog function.
    if (!GetOpenFileName(&OpenFileName))
    {
        // If for any reason, GetOpenFileName returns false, revert to our internal grammar
        m_szGrammarFile[0] = 0;
    }
    UpdateGrammarStatusWindow();
}

/****************************************************************************
* CRecoDlgClass::MessageBoxFromResource *
*---------------------------------------*
*   Description:
*       Display a MessageBox whose text is the requested string resource ID
*
*   Returns:
*       void
*
****************************************************************************/
int CRecoDlgClass::MessageBoxFromResource( UINT uiResource )
{
    TCHAR szMessage[ MAX_LOADSTRING ];
    ::LoadString( m_hInstance, uiResource, szMessage, sizeof( szMessage ) );
    return ::MessageBox( m_hDlg, szMessage, NULL, MB_ICONEXCLAMATION );
}  

/****************************************************************************
* CRecoDlgClass::SetWordSequenceData *
*------------------------------------*
*   Description:
*       This method is called when the "Set Word Sequence Data" menu item is selected from the
*   C&C menu.  It opens the standard file dialog so that the user
*   can specify a document to be used by the application.
*
****************************************************************************/

void CRecoDlgClass::SetWordSequenceData()
{
    WCHAR * pwszCoMem = 0;
    WCHAR * pwszCoMem2 = 0;
    ULONG cch = 0;
    
    HRESULT hr = GetTextFile(&pwszCoMem, &cch);

    if (SUCCEEDED(hr))
    {
        SPTEXTSELECTIONINFO tsi;

        tsi.ulStartActiveOffset = 0;
        tsi.cchActiveChars = cch;
        tsi.ulStartSelection = 0;
        tsi.cchSelection = cch;
    
        pwszCoMem2 = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * (cch + 2));

        if (SUCCEEDED(hr))
        {
            // SetWordSequenceData requires double NULL terminator.
            memcpy(pwszCoMem2, pwszCoMem, sizeof(WCHAR) * cch);
            pwszCoMem2[cch] = L'\0';
            pwszCoMem2[cch+1] = L'\0';

            hr = m_cpCFGGrammar->SetWordSequenceData(pwszCoMem2, cch + 2, &tsi);

            CoTaskMemFree(pwszCoMem2);
        }
        CoTaskMemFree(pwszCoMem);
    }
}


/****************************************************************************
*   Implementation of CDynGrammarDlgClass
****************************************************************************/

/****************************************************************************
* CDynGrammarDlgClass::DlgProc *
*------------------------------*
*   Description:
*       This static member function is the message handler for the dynamic rule
*   dialog.  When the dialog is initialized via WM_INITDIALOG, the pointer to the
*   CDynGrammarDlgClass object is passed to this function in the lParam.  This is stored
*   in the USERDATA window long and used for subsequent message processing.
*
*   Returns:
*       Appropriate LRESULT for give window message
*
****************************************************************************/

LRESULT CALLBACK CDynGrammarDlgClass::DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CDynGrammarDlgClass * pThis = (CDynGrammarDlgClass *)::GetWindowLong(hDlg, GWL_USERDATA);
    switch (message)
    {
        case WM_INITDIALOG:
            ::SetWindowLong(hDlg, GWL_USERDATA, lParam);
            pThis = (CDynGrammarDlgClass *)lParam;
            return pThis->InitDialog(hDlg, pThis->m_pItemList);

        case WM_HELP:
            ::DialogBoxParam(pThis->m_pParent->m_hInstance, (LPCTSTR)IDD_DIALOG_BETAHELP, hDlg, (DLGPROC)BetaHelpDlgProc, NULL);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                pThis->Cleanup();
                EndDialog(hDlg, 0);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDC_BUTTON_ADDITEM)
            {
                pThis->AddItem();
            }
            else if (LOWORD(wParam) == IDC_BUTTON_CLEARALL)
            {
                pThis->ClearAll();
            }
            else if (HIWORD(wParam) == EN_UPDATE && LOWORD(wParam) == IDC_EDIT_NEWITEM)
            {
                //
                //  Only enable the "Add" button if there is some text in the edit control
                //
                BOOL fEnable = ::SendMessage((HWND)lParam, WM_GETTEXTLENGTH, 0, 0);
                ::EnableWindow(::GetDlgItem(hDlg, IDC_BUTTON_ADDITEM), fEnable);
            }
            
            break;

    }
    return FALSE;
}


/****************************************************************************
* CDynGrammarDlgClass::InitDialog *
*---------------------------------*
*   Description:
*       This method is called from the WM_INITDIALOG message.  It initialzes
*   the m_hDlg member and populates the listbox with any items that have already
*   been added to the "DynRule".    
*
*   Returns:
*       TRUE if successful, else FALSE
*
****************************************************************************/

BOOL CDynGrammarDlgClass::InitDialog(HWND hDlg, CSpBasicQueue<CDynItem> *pItemList)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    m_hDlg = hDlg;

    m_pItemList = pItemList;
    ISpRecoGrammar * pGram = m_pParent->m_cpCFGGrammar;

    hr = pGram->GetRule(g_szDynRuleName, NULL, SPRAF_TopLevel | SPRAF_Dynamic | SPRAF_Active, TRUE, &m_hDynRule);
    if (SUCCEEDED(hr))
    {
        HWND hwndList = ::GetDlgItem(hDlg, IDC_LIST_ITEMS);
        CDynItem *pItem = m_pItemList->GetHead();
        while (pItem)
        {
            LRESULT Index = ::SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)W2T(pItem->m_dstr));
            ::SendMessage(hwndList, LB_SETITEMDATA, Index, (LPARAM)W2T(pItem->m_dstr));
            pItem = m_pItemList->GetNext(pItem);
        }
    }
    return TRUE;
}

/****************************************************************************
* CDynGrammarDlgClass::AddItem *
*------------------------------*
*   Description:
*       Called when the "Add" button is pressed.  It calls AddItem to add the
*   text in the edit control to the "DynRule" in the CFG grammar.
*
*   Returns:
*       void
*
****************************************************************************/

void CDynGrammarDlgClass::AddItem()
{
    HRESULT hr = S_OK;
    WCHAR wszItem[MAX_PATH];
    GETTEXTEX gtex = { sp_countof(wszItem), GT_DEFAULT, 1200, NULL, NULL };

    ::SendDlgItemMessage(m_hDlg, IDC_EDIT_NEWITEM, EM_GETTEXTEX, (WPARAM)&gtex, (LPARAM)wszItem);

    hr = m_pParent->m_cpCFGGrammar->AddWordTransition(m_hDynRule, NULL, wszItem, L" ", SPWT_LEXICAL, 1.0, NULL);
    if (SUCCEEDED(hr))
    {
        CDynItem *pItem = new CDynItem(wszItem);
        if (pItem)
        {
            m_pItemList->InsertTail(pItem);
            USES_CONVERSION;
            LRESULT Index = ::SendDlgItemMessage(m_hDlg, IDC_LIST_ITEMS, LB_ADDSTRING, 0, (LPARAM)W2T(wszItem));
        }
        ::SendDlgItemMessage(m_hDlg, IDC_EDIT_NEWITEM, WM_SETTEXT, 0, (LPARAM)_T(""));
        ::SetFocus(::GetDlgItem(m_hDlg, IDC_EDIT_NEWITEM));
    }
}


/****************************************************************************
* CDynGrammarDlgClass::ClearAll *
*-------------------------------*
*   Description:
*       Called when the "Clear" button is pressed.  Clears the contents of the 
*   listbox and clears the "DynRule" in the CFG grammar.
*
*   Returns:
*       void
*
****************************************************************************/

void CDynGrammarDlgClass::ClearAll()
{
    ::SendDlgItemMessage(m_hDlg, IDC_LIST_ITEMS, LB_RESETCONTENT, 0, 0);
    m_pParent->m_cpCFGGrammar->ClearRule(m_hDynRule);
    m_pItemList->Purge();
}


/****************************************************************************
* CDynGrammarDlgClass::Cleanup *
*------------------------------*
*   Description:
*       Called when the dialog is being closed.  Commits the changes to the
*   "DynRule" and sets the activation state of the DynRule to the same state
*   as the current setting of the checkbox in the main dialog.
*
*   Returns:
*       void
*
****************************************************************************/

void CDynGrammarDlgClass::Cleanup()
{
    m_pParent->m_cpCFGGrammar->Commit(0);
    BOOL fActive = ::SendDlgItemMessage(m_pParent->m_hDlg, IDC_CHECK_CFG_ACTIVE, BM_GETCHECK, 0, 0);
    m_pParent->m_cpCFGGrammar->SetRuleState(g_szDynRuleName, NULL, fActive ? SPRS_ACTIVE : SPRS_INACTIVE);
}

/****************************************************************************
*   Implemenation of temporary help dialog for pre-release tool
****************************************************************************/

const TCHAR g_szBetaHelpText[] =
_T("INTRODUCTION:\r\n")
_T("This tool can be used by engine vendors and CFG grammar developers to perform basic functionality ")
_T("testing for recognition. The tool can select a specific engine, load a specified command and control grammar, add items to ")
_T("a dynamic rule, enable a dictation grammar, and turn the microphone state on and off.\r\n\r\n")
_T("BASIC USE:\r\n")
_T("Select the speech recognition engine you want to use and then check Create Recognition Context. ")
_T("Once the engine has loaded, you can enable a dictation grammar by checking Activate Dictation. ")
_T("For debugging purposes, the operations of loading and then activating a specific grammar have been separated so that ")
_T("engine developers can debug the process of loading a grammar prior to the grammar activation.\r\n\r\n")
_T("COMMAND AND CONTROL GRAMMARS\r\n")
_T("Recognition has a built-in grammar containing the basic commands for solitaire. Try saying \"Play the King of Diamonds\" or ")
_T("\"Put the Jack of Clubs on the Queen please.\" If you want to load your own grammar, click Load C&&C. Use GramComp.Exe ")
_T("to compile the grammar for use with this program. If you wish to test dynamic lists, author a grammar with a rule named \"DynRule\" ")
_T("and then reference that rule from a top-level rule in your grammar. When you add items to the grammar, click Dynamic Rules. ")
_T("The items will be added to the dynamic rule in your grammar. If the grammar does not contain a rule named \"DynRule\", then the ")
_T("items added in the Dynamic Rules dialog will become top-level rules.\r\n\r\n")
_T("OTHER FEATURES:\r\n")
_T("When a command is recognized, the text is displayed in the list box at the top of the dialog box, and a dump of the full result is displayed ")
_T("in the status window towards the bottom of the dialog. To examine the result of a particular utterance, simply click it in the list box, and ")
_T("the result status window will be updated. To play back the audio from an utterance, double click the item. Note that if you have not ")
_T("checked Retain Reco Audio, the text will be played back using synthesized speech. If you have enabled audio retention, the ")
_T("original utterance will be played back.  To emulate recognition, you can type a phrase you want parsed into the edit box ")
_T("at the bottom of the dialog and click \"Submit\"");

LRESULT CALLBACK BetaHelpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
            ::SendDlgItemMessage(hDlg, IDC_BETA_HELP, WM_SETTEXT, 0, (LPARAM)g_szBetaHelpText);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, 0);
                return TRUE;
            }

    }
    return FALSE;
}

/****************************************************************************
* AlternatesDlgProc *
*-------------------*
*   Description:
*       Dlgproc for the alternates dialog, which displays the 
*       alternates for the current phrase and allows the user to 
*       choose (commit) one of them.
****************************************************************************/
LRESULT CALLBACK CAlternatesDlgClass::
    AlternatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    CAlternatesDlgClass* pThis = (CAlternatesDlgClass *)::GetWindowLong(hDlg, GWL_USERDATA);

    switch( message )
    {
      case WM_INITDIALOG:
      {
        ::SetWindowLong(hDlg, GWL_USERDATA, lParam);
        pThis = (CAlternatesDlgClass *)lParam;

        //--- Ask for up to 5 alts
        hr = pThis->m_pResult->GetAlternates( 0, SPRR_ALL_ELEMENTS, NUM_ALTS,
                                              pThis->m_Alts, &pThis->m_ulNumAltsReturned );
        //--- Show user choices
        if( SUCCEEDED( hr ) && pThis->m_ulNumAltsReturned )
        {
            for( ULONG i = 0; i < pThis->m_ulNumAltsReturned; ++i )
            {
                WCHAR* pComemText = NULL;
                BYTE Attrs;
                hr = pThis->m_Alts[i]->GetText( SP_GETWHOLEPHRASE, SP_GETWHOLEPHRASE, true,
					                            &pComemText, &Attrs );
                if( SUCCEEDED( hr ) )
                {
                    int Err = ::SendDlgItemMessage( hDlg, IDC_ALTS_LIST, LB_ADDSTRING, 0, (LPARAM)W2T(pComemText) );
                    ::CoTaskMemFree( pComemText );
                }
            }

            //--- Pick the first alternate by default
            ::SendDlgItemMessage( hDlg, IDC_ALTS_LIST, LB_SETCURSEL, 0, 0 );
        }
        break;
      }

      case WM_COMMAND:
      {
        if( LOWORD(wParam) == IDOK )
        {
            ULONG Index = ::SendDlgItemMessage( hDlg, IDC_ALTS_LIST, LB_GETCURSEL, 0, 0 );
            if( Index <= pThis->m_ulNumAltsReturned )
            {
                pThis->m_Alts[Index]->Commit();
            }
        }

        if( ( LOWORD(wParam) == IDOK ) || LOWORD(wParam) == IDCANCEL )
        {
            EndDialog(hDlg, LOWORD(wParam) );
            return TRUE;
        }
        break;
      }
    }
    return FALSE;
} 

