/*******************************************************************************
* SPUIHelp.h *
*------------*
*   Description:
*       This is the header file for user-interface helper functions.  Note that
*       unlike SpHelper.H, this file requires the use of ATL.
*-------------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*******************************************************************************/

#ifndef SPUIHelp_h
#define SPUIHelp_h

#ifndef __sapi_h__
#include <sapi.h>
#endif

#ifndef SPError_h
#include <SPError.h>
#endif

#ifndef SPDebug_h
#include <SPDebug.h>
#endif

#ifndef SPHelper_h
#include <SPHelper.h>
#endif

#ifndef __ATLBASE_H__
#include <ATLBASE.h>
#endif

#ifndef __ATLCONV_H__
#include <ATLCONV.H>
#endif

/****************************************************************************
* 
*
*
********************************************************************* RAL ***/

//
//  Dont call this function directly.  Use SpInitTokenComboBox or SpInitTokenListBox.
//
inline HRESULT SpInitTokenList(UINT MsgAddString, UINT MsgSetItemData, UINT MsgSetCurSel,
                               HWND hwnd, const WCHAR * pszCatName,
                               const WCHAR * pszRequiredAttrib, const WCHAR * pszOptionalAttrib)
{
    HRESULT hr;
    ISpObjectToken * pToken;        // NOTE:  Not a CComPtr!  Be Careful.
    CComPtr<IEnumSpObjectTokens> cpEnum;
    hr = SpEnumTokens(pszCatName, pszRequiredAttrib, pszOptionalAttrib, &cpEnum);
    if (hr == S_OK)
    {
        bool fSetDefault = false;
        while (cpEnum->Next(1, &pToken, NULL) == S_OK)
        {
            CSpDynamicString dstrDesc;
            hr = SpGetDescription(pToken, &dstrDesc);
            if (SUCCEEDED(hr))
            {
                USES_CONVERSION;
                LRESULT i = ::SendMessage(hwnd, MsgAddString, 0, (LPARAM)W2T(dstrDesc));
                if (i == CB_ERR || i == CB_ERRSPACE)    // Note:  CB_ and LB_ errors are identical values...
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    ::SendMessage(hwnd, MsgSetItemData, i, (LPARAM)pToken);
                    if (!fSetDefault)
                    {
                        ::SendMessage(hwnd, MsgSetCurSel, i, 0);
                        fSetDefault = true;
                    }
                }
            }
            if (FAILED(hr))
            {
                pToken->Release();
            }
        }
    }
    else
    {
        hr = SPERR_NO_MORE_ITEMS;
    }
    return hr;
}

inline HRESULT SpInitTokenComboBox(HWND hwnd, const WCHAR * pszCatName,
                                   const WCHAR * pszRequiredAttrib = NULL, const WCHAR * pszOptionalAttrib = NULL)
{
    return SpInitTokenList(CB_ADDSTRING, CB_SETITEMDATA, CB_SETCURSEL, hwnd, pszCatName, pszRequiredAttrib, pszOptionalAttrib);
}

inline HRESULT SpInitTokenListBox(HWND hwnd, const WCHAR * pszCatName,
                                   const WCHAR * pszRequiredAttrib = NULL, const WCHAR * pszOptionalAttrib = NULL)
{
    return SpInitTokenList(LB_ADDSTRING, LB_SETITEMDATA, LB_SETCURSEL, hwnd, pszCatName, pszRequiredAttrib, pszOptionalAttrib);
}

//
//  Dont call this function directly.  Use SpDestoyTokenComboBox or SpDestroyTokenListBox.
//
inline void SpDestroyTokenList(UINT MsgGetCount, UINT MsgGetItemData, HWND hwnd)
{
    LRESULT c = ::SendMessage(hwnd, MsgGetCount, 0, 0);
    for (LRESULT i = 0; i < c; i++)
    {
        IUnknown * pUnkObj = (IUnknown *)::SendMessage(hwnd, MsgGetItemData, i, 0);
        if (pUnkObj)
        {
            pUnkObj->Release();
        }
    }
}

inline void SpDestroyTokenComboBox(HWND hwnd)
{
    SpDestroyTokenList(CB_GETCOUNT, CB_GETITEMDATA, hwnd);
}

inline void SpDestroyTokenListBox(HWND hwnd)
{
    SpDestroyTokenList(LB_GETCOUNT, LB_GETITEMDATA, hwnd);
}


inline ISpObjectToken * SpGetComboBoxToken(HWND hwnd, WPARAM Index)
{
    return (ISpObjectToken *)::SendMessage(hwnd, CB_GETITEMDATA, Index, 0);
}

inline ISpObjectToken * SpGetListBoxToken(HWND hwnd, WPARAM Index)
{
    return (ISpObjectToken *)::SendMessage(hwnd, LB_GETITEMDATA, Index, 0);
}

inline ISpObjectToken * SpGetCurSelComboBoxToken(HWND hwnd)
{
    LRESULT i = ::SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    return (i == CB_ERR) ? NULL : SpGetComboBoxToken(hwnd, i);
}

inline ISpObjectToken * SpGetCurSelListBoxToken(HWND hwnd)
{
    LRESULT i = ::SendMessage(hwnd, LB_GETCURSEL, 0, 0);
    return (i == LB_ERR) ? NULL : SpGetListBoxToken(hwnd, i);
}

//
//  Don't call this directly.  Use SpUpdateCurSelComboBoxToken or SpUpdateCurSelListBoxToken
//
inline HRESULT SpUpdateCurSelToken(UINT MsgDelString, UINT MsgInsertString, UINT MsgGetItemData, UINT MsgSetItemData, UINT MsgGetCurSel, UINT MsgSetCurSel,
                                   HWND hwnd)
{
    HRESULT hr = S_OK;
    LRESULT i = ::SendMessage(hwnd, MsgGetCurSel, 0, 0);
    if (i != CB_ERR)
    {
        ISpObjectToken * pToken = (ISpObjectToken *)::SendMessage(hwnd, MsgGetItemData, i, 0);
        CSpDynamicString dstrDesc;
        hr = SpGetDescription(pToken, &dstrDesc);
        if (SUCCEEDED(hr))
        {
            USES_CONVERSION;
            ::SendMessage(hwnd, MsgDelString, i, 0);
            ::SendMessage(hwnd, MsgInsertString, i, (LPARAM)W2T(dstrDesc));
            ::SendMessage(hwnd, MsgSetItemData, i, (LPARAM)pToken);
            ::SendMessage(hwnd, MsgSetCurSel, i, 0);
        }
    }
    return hr;
}

inline HRESULT SpUpdateCurSelComboBoxToken(HWND hwnd)
{
    return SpUpdateCurSelToken(CB_DELETESTRING, CB_INSERTSTRING, CB_GETITEMDATA, CB_SETITEMDATA, CB_GETCURSEL, CB_SETCURSEL, hwnd);
}

inline HRESULT SpUpdateCurSelListBoxToken(HWND hwnd)
{
    return SpUpdateCurSelToken(LB_DELETESTRING, LB_INSERTSTRING, LB_GETITEMDATA, LB_SETITEMDATA, LB_GETCURSEL, LB_SETCURSEL, hwnd);
}

inline HRESULT SpAddTokenToList(UINT MsgAddString, UINT MsgSetItemData, UINT MsgSetCurSel, HWND hwnd, ISpObjectToken * pToken)
{
    CSpDynamicString dstrDesc;
    HRESULT hr = SpGetDescription(pToken, &dstrDesc);
    if (SUCCEEDED(hr))
    {
        USES_CONVERSION;
        LRESULT i = ::SendMessage(hwnd, MsgAddString, 0, (LPARAM)W2T(dstrDesc));
        if (i == CB_ERR || i == CB_ERRSPACE)    // Note:  CB_ and LB_ errors are identical values...
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            ::SendMessage(hwnd, MsgSetItemData, i, (LPARAM)pToken);
            ::SendMessage(hwnd, MsgSetCurSel, i, 0);
            pToken->AddRef();
        }
    }
    return hr;
}

inline HRESULT SpAddTokenToComboBox(HWND hwnd, ISpObjectToken * pToken)
{
    return SpAddTokenToList(CB_ADDSTRING, CB_SETITEMDATA, CB_SETCURSEL, hwnd, pToken);
}

inline HRESULT SpAddTokenToListBox(HWND hwnd, ISpObjectToken * pToken)
{
    return SpAddTokenToList(LB_ADDSTRING, LB_SETITEMDATA, LB_SETCURSEL, hwnd, pToken);
}


inline HRESULT SpDeleteCurSelToken(UINT MsgGetCurSel, UINT MsgSetCurSel, UINT MsgGetItemData, UINT MsgDeleteString, HWND hwnd)
{
    HRESULT hr = S_OK;
    LRESULT i = ::SendMessage(hwnd, MsgGetCurSel, 0, 0);
    if (i == CB_ERR)
    {
        hr = S_FALSE;
    }
    else
    {
        ISpObjectToken * pToken = (ISpObjectToken *)::SendMessage(hwnd, MsgGetItemData, i, 0);
        if (pToken)
        {
            pToken->Release();
        }
        ::SendMessage(hwnd, MsgDeleteString, i, 0);
        ::SendMessage(hwnd, MsgSetCurSel, i, 0);
    }
    return hr;
}

inline HRESULT SpDeleteCurSelComboBoxToken(HWND hwnd)
{
    return SpDeleteCurSelToken(CB_GETCURSEL, CB_SETCURSEL, CB_GETITEMDATA, CB_DELETESTRING, hwnd);
}

inline HRESULT SpDeleteCurSelListBoxToken(HWND hwnd)
{
    return SpDeleteCurSelToken(LB_GETCURSEL, CB_SETCURSEL, LB_GETITEMDATA, LB_DELETESTRING, hwnd);
}

#endif /* #ifndef SPUIHelp_h -- This must be the last line in the file */
