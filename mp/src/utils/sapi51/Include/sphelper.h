/*******************************************************************************
* SPHelper.h *
*------------*
*   Description:
*       This is the header file for core helper functions implementation.
*-------------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*******************************************************************************/
#ifndef SPHelper_h
#define SPHelper_h

#ifndef _INC_MALLOC
#include <malloc.h>
#endif

#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif

#ifndef __sapi_h__
#include <sapi.h>
#endif

#ifndef __sapiddk_h__
#include <sapiddk.h>
#endif

#ifndef SPError_h
#include <SPError.h>
#endif

#ifndef SPDebug_h
#include <SPDebug.h>
#endif

#ifndef _INC_LIMITS
#include <limits.h>
#endif

#ifndef _INC_MMSYSTEM
#include <mmsystem.h>
#endif

#ifndef __comcat_h__
#include <comcat.h>
#endif

#ifndef _INC_MMREG
#include <mmreg.h>
#endif

#ifndef __ATLBASE_H__
#include <atlbase.h>
#endif

//=== Constants ==============================================================
#define sp_countof(x) ((sizeof(x) / sizeof(*(x))))

/*** CSpDynamicString helper class
*
*/
class CSpDynamicString 
{
public:

    WCHAR *     m_psz;
    CSpDynamicString()
    {
        m_psz = NULL;
    }
    CSpDynamicString(ULONG cchReserve)
    {
        m_psz = (WCHAR *)::CoTaskMemAlloc(cchReserve * sizeof(WCHAR));
    }
    WCHAR * operator=(const CSpDynamicString& src)
    {
        if (m_psz != src.m_psz)
        {
            ::CoTaskMemFree(m_psz);
            m_psz = src.Copy();
        }
        return m_psz;
    }
    WCHAR * operator=(const WCHAR * pSrc)
    {
        Clear();
        if (pSrc)
        {
            ULONG cbNeeded = (wcslen(pSrc) + 1) * sizeof(WCHAR);
            m_psz = (WCHAR *)::CoTaskMemAlloc(cbNeeded);
            SPDBG_ASSERT(m_psz);
            if (m_psz)
            {
                memcpy(m_psz, pSrc, cbNeeded);    
            }
        }
        return m_psz;
    }

    WCHAR * operator=(const char * pSrc)
    {
        Clear();
        if (pSrc)
        {
            ULONG cbNeeded = (lstrlenA(pSrc) + 1) * sizeof(WCHAR);
            m_psz = (WCHAR *)::CoTaskMemAlloc(cbNeeded);
            SPDBG_ASSERT(m_psz);
            if (m_psz)
            {
                ::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, m_psz, cbNeeded/sizeof(WCHAR));
            }
        }
        return m_psz;
    }

    WCHAR * operator=(REFGUID rguid)
    {
        Clear();
        ::StringFromCLSID(rguid, &m_psz);
        return m_psz;
    }


    /*explicit*/ CSpDynamicString(const WCHAR * pSrc)
    {
        m_psz = NULL;
        operator=(pSrc);
    }
    /*explicit*/ CSpDynamicString(const char * pSrc)
    {
        m_psz = NULL;
        operator=(pSrc);
    }
    /*explicit*/ CSpDynamicString(const CSpDynamicString& src)
    {
        m_psz = src.Copy();
    }
    /*explicit*/ CSpDynamicString(REFGUID rguid)
    {
        ::StringFromCLSID(rguid, &m_psz);
    }


    ~CSpDynamicString()
    {
        ::CoTaskMemFree(m_psz);
    }
    unsigned int Length() const
    {
        return (m_psz == NULL)? 0 : wcslen(m_psz);
    }

    operator WCHAR * () const
    {
        return m_psz;
    }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the m_psz member explicitly.
    WCHAR ** operator&()
    {
        SPDBG_ASSERT(m_psz == NULL);
        return &m_psz;
    }

    WCHAR * Append(const WCHAR * pszSrc)
    {
        if (pszSrc)
        {
            ULONG lenSrc = wcslen(pszSrc);
            if (lenSrc)
            {
                ULONG lenMe = Length();
                WCHAR *pszNew = (WCHAR *)::CoTaskMemAlloc((lenMe + lenSrc + 1) * sizeof(WCHAR));
                if (pszNew)
                {
                    if (m_psz)  // Could append to an empty string so check...
                    {
                        if (lenMe)
                        {
                            memcpy(pszNew, m_psz, lenMe * sizeof(WCHAR));
                        }
                        ::CoTaskMemFree(m_psz);
                    }
                    memcpy(pszNew + lenMe, pszSrc, (lenSrc + 1) * sizeof(WCHAR));
                    m_psz = pszNew;
                }
                else
                {
                    SPDBG_ASSERT(FALSE);
                }
            }
        }
        return m_psz;
    }

    WCHAR * Append(const WCHAR * pszSrc, const ULONG lenSrc)
    {
        if (pszSrc && lenSrc)
        {
            ULONG lenMe = Length();
            WCHAR *pszNew = (WCHAR *)::CoTaskMemAlloc((lenMe + lenSrc + 1) * sizeof(WCHAR));
            if (pszNew)
            {
                if (m_psz)  // Could append to an empty string so check...
                {
                    if (lenMe)
                    {
                        memcpy(pszNew, m_psz, lenMe * sizeof(WCHAR));
                    }
                    ::CoTaskMemFree(m_psz);
                }
                memcpy(pszNew + lenMe, pszSrc, lenSrc * sizeof(WCHAR));
                *(pszNew + lenMe + lenSrc) = L'\0';
                m_psz = pszNew;
            }
            else
            {
                SPDBG_ASSERT(FALSE);
            }
        }
        return m_psz;
    }

    WCHAR * Append2(const WCHAR * pszSrc1, const WCHAR * pszSrc2)
    {
        ULONG lenSrc1 = pszSrc1 ? wcslen(pszSrc1) : 0;
        ULONG lenSrc2 = pszSrc2 ? wcslen(pszSrc2) : 0;

        if (lenSrc1 || lenSrc2)
        {
            ULONG lenMe = Length();
            WCHAR *pszNew = (WCHAR *)::CoTaskMemAlloc((lenMe + lenSrc1 + lenSrc2 + 1) * sizeof(WCHAR));
            if (pszNew)
            {
                if (m_psz)  // Could append to an empty string so check...
                {
                    if (lenMe)
                    {
                        memcpy(pszNew, m_psz, lenMe * sizeof(WCHAR));
                    }
                    ::CoTaskMemFree(m_psz);
                }
                // In both of these cases, we copy the trailing NULL so that we're sure it gets
                // there (if lenSrc2 is 0 then we better copy it from pszSrc1).
                if (lenSrc1)
                {
                    memcpy(pszNew + lenMe, pszSrc1, (lenSrc1 + 1) * sizeof(WCHAR));
                }
                if (lenSrc2)
                {
                    memcpy(pszNew + lenMe + lenSrc1, pszSrc2, (lenSrc2 + 1) * sizeof(WCHAR));
                }
                m_psz = pszNew;
            }
            else
            {
                SPDBG_ASSERT(FALSE);
            }
        }
        return m_psz;
    }
    WCHAR * Copy() const
    {
        if (m_psz)
        {
            CSpDynamicString szNew(m_psz);
            return szNew.Detach();
        }
        return NULL;
    }
    CHAR * CopyToChar() const
    {
        if (m_psz)
        {
            CHAR* psz;
            ULONG cbNeeded = ::WideCharToMultiByte(CP_ACP, 0, m_psz, -1, NULL, NULL, NULL, NULL);
            psz = (CHAR *)::CoTaskMemAlloc(cbNeeded);
            SPDBG_ASSERT(psz);
            if (psz)
            {
                ::WideCharToMultiByte(CP_ACP, 0, m_psz, -1, psz, cbNeeded/sizeof(CHAR), NULL, NULL);
            }
            return psz;
        }
        return NULL;
    }
    void Attach(WCHAR * pszSrc)
    {
        SPDBG_ASSERT(m_psz == NULL);
        m_psz = pszSrc;
    }
    WCHAR * Detach()
    {
        WCHAR * s = m_psz;
        m_psz = NULL;
        return s;
    }
    void Clear()
    {
        ::CoTaskMemFree(m_psz);
        m_psz = NULL;
    }
    bool operator!() const
    {
        return (m_psz == NULL);
    }
    HRESULT CopyToBSTR(BSTR * pbstr)
    {
        if (m_psz)
        {
            *pbstr = ::SysAllocString(m_psz);
            if (*pbstr == NULL)
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            *pbstr = NULL;
        }
        return S_OK;
    }
    void TrimToSize(ULONG ulNumChars)
    {
        if (m_psz && ulNumChars < Length())
        {
            m_psz[ulNumChars] = 0;
        }
    }
    WCHAR * Compact()
    {
        if (m_psz)
        {
            ULONG cch = wcslen(m_psz);
            m_psz = (WCHAR *)::CoTaskMemRealloc(m_psz, (cch + 1) * sizeof(WCHAR));
        }
        return m_psz;
    }
    WCHAR * ClearAndGrowTo(ULONG cch)
    {
        if (m_psz)
        {
            Clear();
        }
        m_psz = (WCHAR *)::CoTaskMemAlloc(cch * sizeof(WCHAR));
        return m_psz;
    }
    WCHAR * LTrim()
    {
        if (m_psz)
        {
            WCHAR * pszRead = m_psz;
            while (iswspace(*pszRead))
            {
                pszRead++;
            }
            if (pszRead != m_psz)
            {
                WCHAR * pszWrite = m_psz;
                while (*pszRead)
                {
                    *pszWrite++ = *pszRead++;
                }
                *pszWrite = '\0';
            }
        }
        return m_psz;
    }
    WCHAR * RTrim()
    {
        if (m_psz)
        {
            WCHAR * pszTail = m_psz + wcslen(m_psz);
            WCHAR * pszZeroTerm = pszTail;
            while (pszZeroTerm > m_psz && iswspace(pszZeroTerm[-1]))
            {
                pszZeroTerm--;
            }
            if (pszZeroTerm != pszTail)
            {
                *pszZeroTerm = '\0';
            }
        }
        return m_psz;        
    }
    WCHAR * TrimBoth()
    {
        RTrim();
        return LTrim();
    }
};



//
//  Simple inline function converts a ulong to a hex string.
//
inline void SpHexFromUlong(WCHAR * psz, ULONG ul)
{
    const static WCHAR szHexChars[] = L"0123456789ABCDEF";
    if (ul == 0)
    {
        psz[0] = L'0';
        psz[1] = 0;
    }
    else
    {
        ULONG ulChars = 1;
        psz[0] = 0;
        while (ul)
        {
            memmove(psz + 1, psz, ulChars * sizeof(WCHAR));
            psz[0] = szHexChars[ul % 16];
            ul /= 16;
            ulChars++;
        }
    }
}


//=== Token helpers

inline HRESULT SpGetTokenFromId(
    const WCHAR * pszTokenId, 
    ISpObjectToken ** ppToken,
    BOOL fCreateIfNotExist = FALSE)
{
    SPDBG_FUNC("SpGetTokenFromId");
    HRESULT hr;
    
    CComPtr<ISpObjectToken> cpToken;
    hr = cpToken.CoCreateInstance(CLSID_SpObjectToken);
    
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetId(NULL, pszTokenId, fCreateIfNotExist);
    }
    
    if (SUCCEEDED(hr))
    {
        *ppToken = cpToken.Detach();
    }
    
    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

inline HRESULT SpGetCategoryFromId(
    const WCHAR * pszCategoryId,
    ISpObjectTokenCategory ** ppCategory,
    BOOL fCreateIfNotExist = FALSE)
{
    SPDBG_FUNC("SpGetCategoryFromId");
    HRESULT hr;
    
    CComPtr<ISpObjectTokenCategory> cpTokenCategory;
    hr = cpTokenCategory.CoCreateInstance(CLSID_SpObjectTokenCategory);
    
    if (SUCCEEDED(hr))
    {
        hr = cpTokenCategory->SetId(pszCategoryId, fCreateIfNotExist);
    }
    
    if (SUCCEEDED(hr))
    {
        *ppCategory = cpTokenCategory.Detach();
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpGetDefaultTokenIdFromCategoryId(
    const WCHAR * pszCategoryId,
    WCHAR ** ppszTokenId)
{
    SPDBG_FUNC("SpGetDefaultTokenFromCategoryId");
    HRESULT hr;

    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(pszCategoryId, &cpCategory);
    
    if (SUCCEEDED(hr))
    {
        hr = cpCategory->GetDefaultTokenId(ppszTokenId);
    }

    return hr;
}

inline HRESULT SpSetDefaultTokenIdForCategoryId(
    const WCHAR * pszCategoryId,
    const WCHAR * pszTokenId)
{
    SPDBG_FUNC("SpSetDefaultTokenIdForCategoryId");
    HRESULT hr;

    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(pszCategoryId, &cpCategory);
    
    if (SUCCEEDED(hr))
    {
        hr = cpCategory->SetDefaultTokenId(pszTokenId);
    }

    return hr;
}

inline HRESULT SpGetDefaultTokenFromCategoryId(
    const WCHAR * pszCategoryId,
    ISpObjectToken ** ppToken,
    BOOL fCreateCategoryIfNotExist = TRUE)
{
    SPDBG_FUNC("SpGetDefaultTokenFromCategoryId");
    HRESULT hr;

    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(pszCategoryId, &cpCategory, fCreateCategoryIfNotExist);

    if (SUCCEEDED(hr))
    {
        WCHAR * pszTokenId;
        hr = cpCategory->GetDefaultTokenId(&pszTokenId);
        if (SUCCEEDED(hr))
        {
            hr = SpGetTokenFromId(pszTokenId, ppToken);
            ::CoTaskMemFree(pszTokenId);
        }
    }

    return hr;
}

inline HRESULT SpSetDefaultTokenForCategoryId(
    const WCHAR * pszCategoryId,
    ISpObjectToken * pToken)
{
    SPDBG_FUNC("SpSetDefaultTokenForCategoryId");
    HRESULT hr;

    WCHAR * pszTokenId;
    hr = pToken->GetId(&pszTokenId);

    if (SUCCEEDED(hr))
    {
        hr = SpSetDefaultTokenIdForCategoryId(pszCategoryId, pszTokenId);
        ::CoTaskMemFree(pszTokenId);
    }

    return hr;
}

inline HRESULT SpSetCommonTokenData(
    ISpObjectToken * pToken,
    const CLSID * pclsid,
    const WCHAR * pszLangIndependentName,
    LANGID langid,
    const WCHAR * pszLangDependentName,
    ISpDataKey ** ppDataKeyAttribs)
{
    SPDBG_FUNC("SpSetCommonTokenData");
    HRESULT hr = S_OK;
    
    // Set the new token's CLSID (if specified)
    if (SUCCEEDED(hr) && pclsid)
    {
        CSpDynamicString dstrClsid;
        hr = StringFromCLSID(*pclsid, &dstrClsid);
    
        if (SUCCEEDED(hr))
        {
            hr = pToken->SetStringValue(SPTOKENVALUE_CLSID, dstrClsid);
        }
    }

    // Set the token's lang independent name
    if (SUCCEEDED(hr) && pszLangIndependentName)
    {
        hr = pToken->SetStringValue(NULL, pszLangIndependentName);
    }

    // Set the token's lang dependent name
    if (SUCCEEDED(hr) && pszLangDependentName)
    {
        USES_CONVERSION;
        
        TCHAR szLangId[10];
        wsprintf(szLangId, _T("%x"), langid);

        hr = pToken->SetStringValue(T2W(szLangId), pszLangDependentName);
    }

    // Open the attributes key if requested
    if (SUCCEEDED(hr) && ppDataKeyAttribs)
    {
        hr = pToken->CreateKey(L"Attributes", ppDataKeyAttribs);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpCreateNewToken(
    const WCHAR * pszTokenId,
    ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("SpCreateNewToken");
    HRESULT hr;

    // Forcefully create the token
    hr = SpGetTokenFromId(pszTokenId, ppToken, TRUE);
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpCreateNewToken(
    const WCHAR * pszCategoryId,
    const WCHAR * pszTokenKeyName,
    ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("SpCreateNewToken");
    HRESULT hr;

    // Forcefully create the category
    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(pszCategoryId, &cpCategory, TRUE);

    // Come up with a token key name if one wasn't specified
    CSpDynamicString dstrTokenKeyName;
    if (SUCCEEDED(hr))
    {
        if (pszTokenKeyName == NULL)
        {
            GUID guidTokenKeyName;
            hr = CoCreateGuid(&guidTokenKeyName);

            if (SUCCEEDED(hr))
            {
                hr = StringFromCLSID(guidTokenKeyName, &dstrTokenKeyName);
            }

            if (SUCCEEDED(hr))
            {
                pszTokenKeyName = dstrTokenKeyName;
            }
        }
    }

    // Build the token id
    CSpDynamicString dstrTokenId;
    if (SUCCEEDED(hr))
    {
        dstrTokenId = pszCategoryId;
        dstrTokenId.Append2(L"\\Tokens\\", pszTokenKeyName);
    }

    // Forcefully create the token
    if (SUCCEEDED(hr))
    {
        hr = SpGetTokenFromId(dstrTokenId, ppToken, TRUE);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpCreateNewTokenEx(
    const WCHAR * pszCategoryId,
    const WCHAR * pszTokenKeyName,
    const CLSID * pclsid,
    const WCHAR * pszLangIndependentName,
    LANGID langid,
    const WCHAR * pszLangDependentName,
    ISpObjectToken ** ppToken,
    ISpDataKey ** ppDataKeyAttribs)
{
    SPDBG_FUNC("SpCreateNewTokenEx");
    HRESULT hr;

    // Create the new token
    hr = SpCreateNewToken(pszCategoryId, pszTokenKeyName, ppToken);

    // Now set the extra data
    if (SUCCEEDED(hr))
    {
        hr = SpSetCommonTokenData(
                    *ppToken, 
                    pclsid, 
                    pszLangIndependentName, 
                    langid, 
                    pszLangDependentName, 
                    ppDataKeyAttribs);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpCreateNewTokenEx(
    const WCHAR * pszTokenId,
    const CLSID * pclsid,
    const WCHAR * pszLangIndependentName,
    LANGID langid,
    const WCHAR * pszLangDependentName,
    ISpObjectToken ** ppToken,
    ISpDataKey ** ppDataKeyAttribs)
{
    SPDBG_FUNC("SpCreateNewTokenEx");
    HRESULT hr;

    // Create the new token
    hr = SpCreateNewToken(pszTokenId, ppToken);

    // Now set the extra data
    if (SUCCEEDED(hr))
    {
        hr = SpSetCommonTokenData(
                    *ppToken, 
                    pclsid, 
                    pszLangIndependentName, 
                    langid, 
                    pszLangDependentName, 
                    ppDataKeyAttribs);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpEnumTokens(
    const WCHAR * pszCategoryId, 
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    IEnumSpObjectTokens ** ppEnum)
{
    SPDBG_FUNC("SpEnumTokens");
    HRESULT hr = S_OK;
    
    CComPtr<ISpObjectTokenCategory> cpCategory;
    hr = SpGetCategoryFromId(pszCategoryId, &cpCategory);
    
    if (SUCCEEDED(hr))
    {
        hr = cpCategory->EnumTokens(
                    pszReqAttribs,
                    pszOptAttribs,
                    ppEnum);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

inline HRESULT SpFindBestToken(
    const WCHAR * pszCategoryId, 
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    ISpObjectToken **ppObjectToken)
{
    SPDBG_FUNC("SpFindBestToken");
    HRESULT hr = S_OK;
    
    const WCHAR *pszVendorPreferred = L"VendorPreferred";
    const size_t ulLenVendorPreferred = wcslen(pszVendorPreferred); // no size_t

    // append VendorPreferred to the end of pszOptAttribs to force this preference
    ULONG ulLen = pszOptAttribs ? wcslen(pszOptAttribs) + ulLenVendorPreferred + 1 : ulLenVendorPreferred;
    WCHAR *pszOptAttribsVendorPref = (WCHAR*)_alloca((ulLen+1)*sizeof(WCHAR));
    if (pszOptAttribsVendorPref)
    {
        if (pszOptAttribs)
        {
            wcscpy(pszOptAttribsVendorPref, pszOptAttribs);
            wcscat(pszOptAttribsVendorPref, L";");
            wcscat(pszOptAttribsVendorPref, pszVendorPreferred);
        }
        else
        {
            wcscpy(pszOptAttribsVendorPref, pszVendorPreferred);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    CComPtr<IEnumSpObjectTokens> cpEnum;
    if (SUCCEEDED(hr))
    {
        hr = SpEnumTokens(pszCategoryId, pszReqAttribs, pszOptAttribsVendorPref, &cpEnum);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpEnum->Next(1, ppObjectToken, NULL);
        if (hr == S_FALSE)
        {
            *ppObjectToken = NULL;
            hr = SPERR_NOT_FOUND;
        }
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }
    
    return hr;
}

template<class T>
HRESULT SpCreateObjectFromToken(ISpObjectToken * pToken, T ** ppObject,
                       IUnknown * pUnkOuter = NULL, DWORD dwClsCtxt = CLSCTX_ALL)
{
    SPDBG_FUNC("SpCreateObjectFromToken");
    HRESULT hr;

    hr = pToken->CreateInstance(pUnkOuter, dwClsCtxt, __uuidof(T), (void **)ppObject);
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

template<class T>
HRESULT SpCreateObjectFromTokenId(const WCHAR * pszTokenId, T ** ppObject,
                       IUnknown * pUnkOuter = NULL, DWORD dwClsCtxt = CLSCTX_ALL)
{
    SPDBG_FUNC("SpCreateObjectFromTokenId");
    
    ISpObjectToken * pToken;
    HRESULT hr = SpGetTokenFromId(pszTokenId, &pToken);
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(pToken, ppObject, pUnkOuter, dwClsCtxt);
        pToken->Release();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

template<class T>
HRESULT SpCreateDefaultObjectFromCategoryId(const WCHAR * pszCategoryId, T ** ppObject,
                       IUnknown * pUnkOuter = NULL, DWORD dwClsCtxt = CLSCTX_ALL)
{
    SPDBG_FUNC("SpCreateObjectFromTokenId");
    
    ISpObjectToken * pToken;
    HRESULT hr = SpGetDefaultTokenFromCategoryId(pszCategoryId, &pToken);
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(pToken, ppObject, pUnkOuter, dwClsCtxt);
        pToken->Release();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

template<class T>
HRESULT SpCreateBestObject(
    const WCHAR * pszCategoryId, 
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    T ** ppObject,
    IUnknown * pUnkOuter = NULL, 
    DWORD dwClsCtxt = CLSCTX_ALL)
{
    SPDBG_FUNC("SpCreateBestObject");
    HRESULT hr;
    
    CComPtr<ISpObjectToken> cpToken;
    hr = SpFindBestToken(pszCategoryId, pszReqAttribs, pszOptAttribs, &cpToken);

    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(cpToken, ppObject, pUnkOuter, dwClsCtxt);
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

inline HRESULT SpCreatePhoneConverter(
    LANGID LangID,
    const WCHAR * pszReqAttribs,
    const WCHAR * pszOptAttribs,
    ISpPhoneConverter ** ppPhoneConverter)
{
    SPDBG_FUNC("SpCreatePhoneConverter");
    HRESULT hr;

    if (LangID == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        CSpDynamicString dstrReqAttribs;
        if (pszReqAttribs)
        {
            dstrReqAttribs = pszReqAttribs;
            dstrReqAttribs.Append(L";");
        }

        WCHAR szLang[MAX_PATH];

        SpHexFromUlong(szLang, LangID);

        WCHAR szLangCondition[MAX_PATH];
        wcscpy(szLangCondition, L"Language=");
        wcscat(szLangCondition, szLang);

        dstrReqAttribs.Append(szLangCondition);

        hr = SpCreateBestObject(SPCAT_PHONECONVERTERS, dstrReqAttribs, pszOptAttribs, ppPhoneConverter);
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* SpHrFromWin32 *
*---------------*
*   Description:
*       This inline function works around a basic problem with the macro
*   HRESULT_FROM_WIN32.  The macro forces the expresion in ( ) to be evaluated
*   two times.  By using this inline function, the expression will only be
*   evaluated once.
*
*   Returns:
*       HRESULT of converted Win32 error code
*
*****************************************************************************/

inline HRESULT SpHrFromWin32(DWORD dwErr)
{
    return HRESULT_FROM_WIN32(dwErr);
}


/****************************************************************************
* SpHrFromLastWin32Error *
*------------------------*
*   Description:
*       This simple inline function is used to return a converted HRESULT
*   from the Win32 function ::GetLastError.  Note that using HRESULT_FROM_WIN32
*   will evaluate the error code twice so we don't want to use:
*
*       HRESULT_FROM_WIN32(::GetLastError()) 
*
*   since that will call GetLastError twice.
*   On Win98 and WinMe ::GetLastError() returns 0 for some functions (see MSDN).
*   We therefore check for that and return E_FAIL. This function should only be
*   called in an error case since it will always return an error code!
*
*   Returns:
*       HRESULT for ::GetLastError()
*
*****************************************************************************/

inline HRESULT SpHrFromLastWin32Error()
{
    DWORD dw = ::GetLastError();
    return (dw == 0) ? E_FAIL : SpHrFromWin32(dw);
}


/****************************************************************************
* SpGetUserDefaultUILanguage *
*----------------------------*
*   Description:
*       Returns the default user interface language, using a method 
*       appropriate to the platform (Windows 9x, Windows NT, or Windows 2000)
*
*   Returns:
*       Default UI language
*
*****************************************************************************/

inline LANGID SpGetUserDefaultUILanguage(void) 
{
    HRESULT hr = S_OK;
    LANGID wUILang = 0;

    OSVERSIONINFO Osv ;
    Osv.dwOSVersionInfoSize = sizeof(Osv) ;
    if(!GetVersionEx(&Osv)) 
    {
        hr = SpHrFromLastWin32Error();
    }
    // Get the UI language by one of three methods, depending on the system
    else if(Osv.dwPlatformId != VER_PLATFORM_WIN32_NT) 
    {
        // Case 1: Running on Windows 9x. Get the system UI language from registry:
        CHAR szData[32];
        DWORD dwSize = sizeof(szData) ;
        HKEY hKey;

        long lRet = RegOpenKeyEx(
                        HKEY_USERS, 
                        _T(".Default\\Control Panel\\desktop\\ResourceLocale"), 
                        0, 
                        KEY_READ, 
                        &hKey);

#ifdef _WIN32_WCE_BUG_10655
        if (lRet == ERROR_INVALID_PARAMETER)
        {
            lRet = ERROR_FILE_NOT_FOUND;
        }
#endif // _WIN32_WCE_BUG_10655

        hr = SpHrFromWin32(lRet);

        if (SUCCEEDED(hr))
        {
            lRet = RegQueryValueEx(  
                        hKey, 
                        _T(""), 
                        NULL, 
                        NULL, 
                        (BYTE *)szData, 
                        &dwSize);

#ifdef _WIN32_WCE_BUG_10655
            if(lRet == ERROR_INVALID_PARAMETER)
            {
                lRet = ERROR_FILE_NOT_FOUND;
            }
#endif //_WIN32_WCE_BUG_10655

            hr = SpHrFromWin32(lRet); 
            ::RegCloseKey(hKey) ;
        }
        if (SUCCEEDED(hr))
        {
            // Convert string to number
            wUILang = (LANGID) strtol(szData, NULL, 16) ;
        }
    }
    else if (Osv.dwMajorVersion >= 5.0) 
    {
    // Case 2: Running on Windows 2000 or later. Use GetUserDefaultUILanguage to find 
    // the user's prefered UI language


        HMODULE hMKernel32 = ::LoadLibraryW(L"kernel32.dll") ;
        if (hMKernel32 == NULL)
        {
            hr = SpHrFromLastWin32Error();
        }
        else
        {

            LANGID (WINAPI *pfnGetUserDefaultUILanguage) () = 
                (LANGID (WINAPI *)(void)) 
#ifdef _WIN32_WCE
                    GetProcAddress(hMKernel32, L"GetUserDefaultUILanguage") ;
#else
                    GetProcAddress(hMKernel32, "GetUserDefaultUILanguage") ;
#endif

            if(NULL != pfnGetUserDefaultUILanguage) 
            {
                wUILang = pfnGetUserDefaultUILanguage() ;
            }
            else
            {   // GetProcAddress failed
                hr = SpHrFromLastWin32Error();
            }
            ::FreeLibrary(hMKernel32);
        }
    }
    else {
    // Case 3: Running on Windows NT 4.0 or earlier. Get UI language
    // from locale of .default user in registry:
    // HKEY_USERS\.DEFAULT\Control Panel\International\Locale
        
        WCHAR szData[32]   ;
        DWORD dwSize = sizeof(szData) ;
        HKEY hKey          ;

        LONG lRet = RegOpenKeyEx(HKEY_USERS, 
                                    _T(".DEFAULT\\Control Panel\\International"), 
                                    0, 
                                    KEY_READ, 
                                    &hKey);
#ifdef _WIN32_WCE_BUG_10655
            if(lRet == ERROR_INVALID_PARAMETER)
            {
                lRet = ERROR_FILE_NOT_FOUND;
            }
#endif //_WIN32_WCE_BUG_10655

        hr = SpHrFromWin32(lRet);

        if (SUCCEEDED(hr))
        {
            lRet = RegQueryValueEx(  
                        hKey, 
                        _T("Locale"),
                        NULL, 
                        NULL, 
                        (BYTE *)szData, 
                        &dwSize);

#ifdef _WIN32_WCE_BUG_10655
            if(lRet == ERROR_INVALID_PARAMETER)
            {
                lRet = ERROR_FILE_NOT_FOUND;
            }
#endif //_WIN32_WCE_BUG_10655

        hr = SpHrFromWin32(lRet);
            ::RegCloseKey(hKey);
        }

        if (SUCCEEDED(hr))
        {
            // Convert string to number
            wUILang = (LANGID) wcstol(szData, NULL, 16) ;

            if(0x0401 == wUILang || // Arabic
               0x040d == wUILang || // Hebrew
               0x041e == wUILang    // Thai
               )
            {
                // Special case these to the English UI.
                // These versions of Windows NT 4.0 were enabled only, i.e., the
                // UI was English. However, the registry setting 
                // HKEY_USERS\.DEFAULT\Control Panel\International\Locale was set  
                // to the respective locale for application compatibility.
                wUILang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US) ;
            }
        }
    }

    return (wUILang ? wUILang : ::GetUserDefaultLangID());    // In failure case, try our best!
}


inline HRESULT SpGetDescription(ISpObjectToken * pObjToken, WCHAR ** ppszDescription, LANGID Language = SpGetUserDefaultUILanguage())
{
    WCHAR szLangId[10];
    SpHexFromUlong(szLangId, Language);
    HRESULT hr = pObjToken->GetStringValue(szLangId, ppszDescription);
    if (hr == SPERR_NOT_FOUND)
    {
        hr = pObjToken->GetStringValue(NULL, ppszDescription);
    }
    return hr;
}


inline HRESULT SpSetDescription(ISpObjectToken * pObjToken, const WCHAR * pszDescription, LANGID Language = SpGetUserDefaultUILanguage(), BOOL fSetLangIndependentId = TRUE)
{
    WCHAR szLangId[10];
    SpHexFromUlong(szLangId, Language);
    HRESULT hr = pObjToken->SetStringValue(szLangId, pszDescription);
    if (SUCCEEDED(hr) && fSetLangIndependentId)
    {
        hr = pObjToken->SetStringValue(NULL, pszDescription);
    }
    return hr;
}

/****************************************************************************
* SpConvertStreamFormatEnum *
*---------------------------*
*   Description:
*       This method converts the specified stream format into a wave format
*   structure.
*
*****************************************************************************/
inline HRESULT SpConvertStreamFormatEnum(SPSTREAMFORMAT eFormat, GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    HRESULT hr = S_OK;

    if(pFormatId==NULL || ::IsBadWritePtr(pFormatId, sizeof(*pFormatId))
        || ppCoMemWaveFormatEx==NULL || ::IsBadWritePtr(ppCoMemWaveFormatEx, sizeof(*ppCoMemWaveFormatEx)))
    {
        return E_INVALIDARG;
    }

    const GUID * pFmtGuid = &GUID_NULL;     // Assume failure case
    if( eFormat >= SPSF_8kHz8BitMono && eFormat <= SPSF_48kHz16BitStereo )
    {
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        *ppCoMemWaveFormatEx = pwfex;
        if (pwfex)
        {
            DWORD dwIndex = eFormat - SPSF_8kHz8BitMono;
            BOOL bIsStereo = dwIndex & 0x1;
            BOOL bIs16 = dwIndex & 0x2;
            DWORD dwKHZ = (dwIndex & 0x3c) >> 2;
            static const DWORD adwKHZ[] = { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 };
            pwfex->wFormatTag = WAVE_FORMAT_PCM;
            pwfex->nChannels = pwfex->nBlockAlign = (WORD)(bIsStereo ? 2 : 1);
            pwfex->nSamplesPerSec = adwKHZ[dwKHZ];
            pwfex->wBitsPerSample = 8;
            if (bIs16)
            {
                pwfex->wBitsPerSample *= 2;
                pwfex->nBlockAlign *= 2;
            }
                pwfex->nAvgBytesPerSec = pwfex->nSamplesPerSec * pwfex->nBlockAlign;
            pwfex->cbSize = 0;
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if( eFormat == SPSF_TrueSpeech_8kHz1BitMono )
    {
        int NumBytes = sizeof( WAVEFORMATEX ) + 32;
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc( NumBytes );
        *ppCoMemWaveFormatEx = pwfex;
        if( pwfex )
        {
            memset( pwfex, 0, NumBytes );
            pwfex->wFormatTag      = WAVE_FORMAT_DSPGROUP_TRUESPEECH;
            pwfex->nChannels       = 1;
            pwfex->nSamplesPerSec  = 8000;
            pwfex->nAvgBytesPerSec = 1067;
            pwfex->nBlockAlign     = 32;
            pwfex->wBitsPerSample  = 1;
            pwfex->cbSize          = 32;
            BYTE* pExtra = ((BYTE*)pwfex) + sizeof( WAVEFORMATEX );
            pExtra[0] = 1;
            pExtra[2] = 0xF0;
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if( (eFormat >= SPSF_CCITT_ALaw_8kHzMono    ) &&
             (eFormat <= SPSF_CCITT_ALaw_44kHzStereo ) )
    {
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc( sizeof(WAVEFORMATEX) );
        *ppCoMemWaveFormatEx = pwfex;
        if( pwfex )
        {
            memset( pwfex, 0, sizeof(WAVEFORMATEX) );
            DWORD dwIndex = eFormat - SPSF_CCITT_ALaw_8kHzMono;
            DWORD dwKHZ = dwIndex / 2;
            static const DWORD adwKHZ[] = { 8000, 11025, 22050, 44100 };
            BOOL bIsStereo    = dwIndex & 0x1;
            pwfex->wFormatTag = WAVE_FORMAT_ALAW;
            pwfex->nChannels  = pwfex->nBlockAlign = (WORD)(bIsStereo ? 2 : 1);
            pwfex->nSamplesPerSec  = adwKHZ[dwKHZ];
            pwfex->wBitsPerSample  = 8;
                pwfex->nAvgBytesPerSec = pwfex->nSamplesPerSec * pwfex->nBlockAlign;
            pwfex->cbSize          = 0;
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if( (eFormat >= SPSF_CCITT_uLaw_8kHzMono    ) &&
             (eFormat <= SPSF_CCITT_uLaw_44kHzStereo ) )
    {
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc( sizeof(WAVEFORMATEX) );
        *ppCoMemWaveFormatEx = pwfex;
        if( pwfex )
        {
            memset( pwfex, 0, sizeof(WAVEFORMATEX) );
            DWORD dwIndex = eFormat - SPSF_CCITT_uLaw_8kHzMono;
            DWORD dwKHZ = dwIndex / 2;
            static const DWORD adwKHZ[] = { 8000, 11025, 22050, 44100 };
            BOOL bIsStereo    = dwIndex & 0x1;
            pwfex->wFormatTag = WAVE_FORMAT_MULAW;
            pwfex->nChannels  = pwfex->nBlockAlign = (WORD)(bIsStereo ? 2 : 1);
            pwfex->nSamplesPerSec  = adwKHZ[dwKHZ];
            pwfex->wBitsPerSample  = 8;
                pwfex->nAvgBytesPerSec = pwfex->nSamplesPerSec * pwfex->nBlockAlign;
            pwfex->cbSize          = 0;
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if( (eFormat >= SPSF_ADPCM_8kHzMono    ) &&
             (eFormat <= SPSF_ADPCM_44kHzStereo ) )
    {
        int NumBytes = sizeof( WAVEFORMATEX ) + 32;
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc( NumBytes );
        *ppCoMemWaveFormatEx = pwfex;
        if( pwfex )
        {
            //--- Some of these values seem odd. We used what the codec told us.
            static const DWORD adwKHZ[] = { 8000, 11025, 22050, 44100 };
            static const DWORD BytesPerSec[] = { 4096, 8192, 5644, 11289, 11155, 22311, 22179, 44359 };
            static const DWORD BlockAlign[]  = { 256, 256, 512, 1024 };
            static const BYTE Extra811[32] =
            {
                0xF4, 0x01, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x02, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
                0xC0, 0x00, 0x40, 0x00, 0xF0, 0x00, 0x00, 0x00,
                0xCC, 0x01, 0x30, 0xFF, 0x88, 0x01, 0x18, 0xFF
            };

            static const BYTE Extra22[32] =
            {
                0xF4, 0x03, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x02, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
                0xC0, 0x00, 0x40, 0x00, 0xF0, 0x00, 0x00, 0x00,
                0xCC, 0x01, 0x30, 0xFF, 0x88, 0x01, 0x18, 0xFF
            };

            static const BYTE Extra44[32] =
            {
                0xF4, 0x07, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00,
                0x00, 0x02, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00,
                0xC0, 0x00, 0x40, 0x00, 0xF0, 0x00, 0x00, 0x00,
                0xCC, 0x01, 0x30, 0xFF, 0x88, 0x01, 0x18, 0xFF
            };

            static const BYTE* Extra[4] = { Extra811, Extra811, Extra22, Extra44 };
            memset( pwfex, 0, NumBytes );
            DWORD dwIndex  = eFormat - SPSF_ADPCM_8kHzMono;
            DWORD dwKHZ    = dwIndex / 2;
            BOOL bIsStereo = dwIndex & 0x1;
            pwfex->wFormatTag      = WAVE_FORMAT_ADPCM;
            pwfex->nChannels       = (WORD)(bIsStereo ? 2 : 1);
            pwfex->nSamplesPerSec  = adwKHZ[dwKHZ];
            pwfex->nAvgBytesPerSec = BytesPerSec[dwIndex];
            pwfex->nBlockAlign     = (WORD)(BlockAlign[dwKHZ] * pwfex->nChannels);
            pwfex->wBitsPerSample  = 4;
            pwfex->cbSize          = 32;
            BYTE* pExtra = ((BYTE*)pwfex) + sizeof( WAVEFORMATEX );
            memcpy( pExtra, Extra[dwKHZ], 32 );
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if( (eFormat >= SPSF_GSM610_8kHzMono    ) &&
             (eFormat <= SPSF_GSM610_44kHzMono ) )
    {
        int NumBytes = sizeof( WAVEFORMATEX ) + 2;
        WAVEFORMATEX * pwfex = (WAVEFORMATEX *)::CoTaskMemAlloc( NumBytes );
        *ppCoMemWaveFormatEx = pwfex;
        if( pwfex )
        {
            //--- Some of these values seem odd. We used what the codec told us.
            static const DWORD adwKHZ[] = { 8000, 11025, 22050, 44100 };
            static const DWORD BytesPerSec[] = { 1625, 2239, 4478, 8957 };
            memset( pwfex, 0, NumBytes );
            DWORD dwIndex          = eFormat - SPSF_GSM610_8kHzMono;
            pwfex->wFormatTag      = WAVE_FORMAT_GSM610;
            pwfex->nChannels       = 1;
            pwfex->nSamplesPerSec  = adwKHZ[dwIndex];
            pwfex->nAvgBytesPerSec = BytesPerSec[dwIndex];
            pwfex->nBlockAlign     = 65;
            pwfex->wBitsPerSample  = 0;
            pwfex->cbSize          = 2;
            BYTE* pExtra = ((BYTE*)pwfex) + sizeof( WAVEFORMATEX );
            pExtra[0] = 0x40;
            pExtra[1] = 0x01;
            pFmtGuid = &SPDFID_WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *ppCoMemWaveFormatEx = NULL;
        switch (eFormat)
        {
        case SPSF_NoAssignedFormat:
            break;
        case SPSF_Text:
            pFmtGuid = &SPDFID_Text;
            break;
        default:
            hr = E_INVALIDARG;
            break;
        }
    }
    *pFormatId = *pFmtGuid;
    return hr;
}

class CSpStreamFormat
{
public:
    GUID            m_guidFormatId;
    WAVEFORMATEX  * m_pCoMemWaveFormatEx; 


 static long CoMemCopyWFEX(const WAVEFORMATEX * pSrc, WAVEFORMATEX ** ppCoMemWFEX) // missing long
    {
        ULONG cb = sizeof(WAVEFORMATEX) + pSrc->cbSize;
        *ppCoMemWFEX = (WAVEFORMATEX *)::CoTaskMemAlloc(cb);
        if (*ppCoMemWFEX)
        {
            memcpy(*ppCoMemWFEX, pSrc, cb);
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }


    CSpStreamFormat()
    {
        m_guidFormatId = GUID_NULL;
        m_pCoMemWaveFormatEx = NULL;
    }

    CSpStreamFormat(SPSTREAMFORMAT eFormat, HRESULT * phr)
    {
        *phr = SpConvertStreamFormatEnum(eFormat, &m_guidFormatId, &m_pCoMemWaveFormatEx);
    }

    CSpStreamFormat(const WAVEFORMATEX * pWaveFormatEx, HRESULT * phr)
    {
        SPDBG_ASSERT(pWaveFormatEx);
        *phr = CoMemCopyWFEX(pWaveFormatEx, &m_pCoMemWaveFormatEx);
        m_guidFormatId = SUCCEEDED(*phr) ? SPDFID_WaveFormatEx : GUID_NULL;
    }

    ~CSpStreamFormat()
    {
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
    }

    void Clear()
    {
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
        m_pCoMemWaveFormatEx = NULL;
        memset(&m_guidFormatId, 0, sizeof(m_guidFormatId));
    }

    const GUID & FormatId() const 
    {
        return m_guidFormatId;
    }

    const WAVEFORMATEX * WaveFormatExPtr() const
    {
        return m_pCoMemWaveFormatEx;
    }


    HRESULT AssignFormat(SPSTREAMFORMAT eFormat)
    {
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);    
        return SpConvertStreamFormatEnum(eFormat, &m_guidFormatId, &m_pCoMemWaveFormatEx);
    }

    HRESULT AssignFormat(ISpStreamFormat * pStream)
    {
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
        m_pCoMemWaveFormatEx = NULL;
        return pStream->GetFormat(&m_guidFormatId, &m_pCoMemWaveFormatEx);
    }

    HRESULT AssignFormat(const WAVEFORMATEX * pWaveFormatEx)
    {
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
        HRESULT hr = CoMemCopyWFEX(pWaveFormatEx, &m_pCoMemWaveFormatEx);
        m_guidFormatId = SUCCEEDED(hr) ? SPDFID_WaveFormatEx : GUID_NULL;
        return hr;
    }

    HRESULT AssignFormat(REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx)
    {
        HRESULT hr = S_OK;

        m_guidFormatId = rguidFormatId;
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
        m_pCoMemWaveFormatEx = NULL;

        if (rguidFormatId == SPDFID_WaveFormatEx)
        {
            if (::IsBadReadPtr(pWaveFormatEx, sizeof(*pWaveFormatEx)))
            {
                hr = E_INVALIDARG;
            }
            else 
            {
                hr = CoMemCopyWFEX(pWaveFormatEx, &m_pCoMemWaveFormatEx);
            }

            if (FAILED(hr))
            {
                m_guidFormatId = GUID_NULL;
            }
        }

        return hr;
    }


    BOOL IsEqual(REFGUID rguidFormatId, const WAVEFORMATEX * pwfex) const
    {
        if (rguidFormatId == m_guidFormatId)
        {
            if (m_pCoMemWaveFormatEx)
            {
                if (pwfex &&
                    pwfex->cbSize == m_pCoMemWaveFormatEx->cbSize &&
                    memcmp(m_pCoMemWaveFormatEx, pwfex, sizeof(WAVEFORMATEX) + pwfex->cbSize) == 0)
                {
                    return TRUE;
                }
            }
            else
            {
                return (pwfex == NULL);
            }
        }
        return FALSE;
    }



    HRESULT ParamValidateAssignFormat(REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx, BOOL fRequireWaveFormat = FALSE)
    {
        if ((pWaveFormatEx && (::IsBadReadPtr(pWaveFormatEx, sizeof(*pWaveFormatEx)) || rguidFormatId != SPDFID_WaveFormatEx)) ||
            (fRequireWaveFormat && pWaveFormatEx == NULL))
        {
            return E_INVALIDARG;
        }
        return AssignFormat(rguidFormatId, pWaveFormatEx);
    }

    SPSTREAMFORMAT ComputeFormatEnum()
    {
        if (m_guidFormatId == GUID_NULL)
        {
            return SPSF_NoAssignedFormat;
        }
        if (m_guidFormatId == SPDFID_Text)
        {
            return SPSF_Text;
        }
        if (m_guidFormatId != SPDFID_WaveFormatEx)
        {
            return SPSF_NonStandardFormat;
        }
        //
        //  It is a WAVEFORMATEX.  Now determine which type it is and convert.
        //
        DWORD dwIndex = 0;
        switch (m_pCoMemWaveFormatEx->wFormatTag)
        {
          case WAVE_FORMAT_PCM:
          {
            switch (m_pCoMemWaveFormatEx->nChannels)
            {
              case 1:
                break;
              case 2:
                dwIndex |= 1;
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            switch (m_pCoMemWaveFormatEx->wBitsPerSample)
            {
              case 8:
                break;
              case 16:
                dwIndex |= 2;
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            switch (m_pCoMemWaveFormatEx->nSamplesPerSec)
            {
              case 48000:
                dwIndex += 4;   // Fall through
              case 44100:
                dwIndex += 4;   // Fall through
              case 32000:
                dwIndex += 4;   // Fall through
              case 24000:
                dwIndex += 4;   // Fall through
              case 22050:
                dwIndex += 4;   // Fall through
              case 16000:
                dwIndex += 4;   // Fall through
              case 12000:
                dwIndex += 4;   // Fall through
              case 11025:
                dwIndex += 4;   // Fall through
              case 8000:
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            return static_cast<SPSTREAMFORMAT>(SPSF_8kHz8BitMono + dwIndex);
          }

          case WAVE_FORMAT_DSPGROUP_TRUESPEECH:
          {
            return SPSF_TrueSpeech_8kHz1BitMono;
          }

          case WAVE_FORMAT_ALAW: // fall through
          case WAVE_FORMAT_MULAW:
          case WAVE_FORMAT_ADPCM:
          {
            switch (m_pCoMemWaveFormatEx->nChannels)
            {
              case 1:
                break;
              case 2:
                dwIndex |= 1;
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            if(m_pCoMemWaveFormatEx->wFormatTag == WAVE_FORMAT_ADPCM)
            {
                if(m_pCoMemWaveFormatEx->wBitsPerSample != 4)
                {
                    return SPSF_ExtendedAudioFormat;
                }
            }
            else if(m_pCoMemWaveFormatEx->wBitsPerSample != 8)
            {
                return SPSF_ExtendedAudioFormat;
            }

            switch (m_pCoMemWaveFormatEx->nSamplesPerSec)
            {
              case 44100:
                dwIndex += 2;   // Fall through
              case 22050:
                dwIndex += 2;   // Fall through
              case 11025:
                dwIndex += 2;   // Fall through
              case 8000:
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            switch( m_pCoMemWaveFormatEx->wFormatTag )
            {
              case WAVE_FORMAT_ALAW:
                return static_cast<SPSTREAMFORMAT>(SPSF_CCITT_ALaw_8kHzMono + dwIndex);
              case WAVE_FORMAT_MULAW:
                return static_cast<SPSTREAMFORMAT>(SPSF_CCITT_uLaw_8kHzMono + dwIndex);
              case WAVE_FORMAT_ADPCM:
                return static_cast<SPSTREAMFORMAT>(SPSF_ADPCM_8kHzMono + dwIndex);
            }
          }

          case WAVE_FORMAT_GSM610:
          {
            if( m_pCoMemWaveFormatEx->nChannels != 1 )
            {
                return SPSF_ExtendedAudioFormat;
            }

            switch (m_pCoMemWaveFormatEx->nSamplesPerSec)
            {
              case 44100:
                dwIndex = 3;
                break;
              case 22050:
                dwIndex = 2;
                break;
              case 11025:
                dwIndex = 1;
                break;
              case 8000:
                dwIndex = 0;
                break;
              default:
                return SPSF_ExtendedAudioFormat;
            }

            return static_cast<SPSTREAMFORMAT>(SPSF_GSM610_8kHzMono + dwIndex);
          }

          default:
            return SPSF_ExtendedAudioFormat;
            break;
        }
    }

    void DetachTo(CSpStreamFormat & Other)
    {
        ::CoTaskMemFree(Other.m_pCoMemWaveFormatEx);
        Other.m_guidFormatId = m_guidFormatId;
        Other.m_pCoMemWaveFormatEx = m_pCoMemWaveFormatEx;
        m_pCoMemWaveFormatEx = NULL;
        memset(&m_guidFormatId, 0, sizeof(m_guidFormatId));
    }

    void DetachTo(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
    {
        *pFormatId = m_guidFormatId;
        *ppCoMemWaveFormatEx = m_pCoMemWaveFormatEx;
        m_pCoMemWaveFormatEx = NULL;
        memset(&m_guidFormatId, 0, sizeof(m_guidFormatId));
    }

    HRESULT CopyTo(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWFEX) const
    {
        HRESULT hr = S_OK;
        *pFormatId = m_guidFormatId;
        if (m_pCoMemWaveFormatEx)
        {
            hr = CoMemCopyWFEX(m_pCoMemWaveFormatEx, ppCoMemWFEX);
            if (FAILED(hr))
            {
                memset(pFormatId, 0, sizeof(*pFormatId));
            }
        }
        else
        {
            *ppCoMemWFEX = NULL;
        }
        return hr;
    }

    HRESULT CopyTo(CSpStreamFormat & Other) const
    {
        ::CoTaskMemFree(Other.m_pCoMemWaveFormatEx);
        return CopyTo(&Other.m_guidFormatId, &Other.m_pCoMemWaveFormatEx);
    }
    
    HRESULT AssignFormat(const CSpStreamFormat & Src)
    {
        return Src.CopyTo(*this);
    }


    HRESULT ParamValidateCopyTo(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWFEX) const
    {
        if (::IsBadWritePtr(pFormatId, sizeof(*pFormatId)) ||
            ::IsBadWritePtr(ppCoMemWFEX, sizeof(*ppCoMemWFEX)))
        {
            return E_POINTER;
        }
        return CopyTo(pFormatId, ppCoMemWFEX);
    }

    BOOL operator==(const CSpStreamFormat & Other) const
    {
        return IsEqual(Other.m_guidFormatId, Other.m_pCoMemWaveFormatEx);
    }
    BOOL operator!=(const CSpStreamFormat & Other) const
    {
        return !IsEqual(Other.m_guidFormatId, Other.m_pCoMemWaveFormatEx);
    }

    ULONG SerializeSize() const
    {
        ULONG cb = sizeof(ULONG) + sizeof(m_guidFormatId);
        if (m_pCoMemWaveFormatEx)
        {
            cb += sizeof(WAVEFORMATEX) + m_pCoMemWaveFormatEx->cbSize + 3;  // Add 3 to round up
            cb -= cb % 4;                                                   // Round to DWORD
        }
        return cb;
    }

    ULONG Serialize(BYTE * pBuffer) const
    {
        ULONG cb = SerializeSize();
        *((UNALIGNED ULONG *)pBuffer) = cb;
        pBuffer += sizeof(ULONG);
        *((UNALIGNED GUID *)pBuffer) = m_guidFormatId;
        if (m_pCoMemWaveFormatEx)
        {
            pBuffer += sizeof(m_guidFormatId);
            memcpy(pBuffer, m_pCoMemWaveFormatEx, sizeof(WAVEFORMATEX) + m_pCoMemWaveFormatEx->cbSize);
        }
        return cb;
    }

    HRESULT Deserialize(const BYTE * pBuffer, ULONG * pcbUsed)
    {
        HRESULT hr = S_OK;
        ::CoTaskMemFree(m_pCoMemWaveFormatEx);
        m_pCoMemWaveFormatEx = NULL;
        *pcbUsed = *((UNALIGNED ULONG *)pBuffer);
        pBuffer += sizeof(ULONG);
        // Misaligment exception is generated for SHx platform.
        // Marking pointer as UNALIGNED does not help.
#ifndef _WIN32_WCE
        m_guidFormatId = *((UNALIGNED GUID *)pBuffer);
#else
        memcpy(&m_guidFormatId, pBuffer, sizeof(GUID));
#endif
        if (*pcbUsed > sizeof(GUID) + sizeof(ULONG))
        {
            pBuffer += sizeof(m_guidFormatId);
            hr = CoMemCopyWFEX((const WAVEFORMATEX *)pBuffer, &m_pCoMemWaveFormatEx);
            if (FAILED(hr))
            {
                m_guidFormatId = GUID_NULL;
            }
        }
        return hr;
    }

};



// Return the default codepage given a LCID.
// Note some of the newer locales do not have associated Windows codepages.  For these, we return UTF-8.

inline UINT SpCodePageFromLcid(LCID lcid)
{
    char achCodePage[6];

    return (0 != GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, achCodePage, sizeof(achCodePage))) ? atoi(achCodePage) : 65001;
}


inline HRESULT SPBindToFile( LPCWSTR pFileName, SPFILEMODE eMode, ISpStream ** ppStream,
                            const GUID * pFormatId = NULL, const WAVEFORMATEX * pWaveFormatEx = NULL,
                            ULONGLONG ullEventInterest = SPFEI_ALL_EVENTS)
{
    HRESULT hr = ::CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL, __uuidof(*ppStream), (void **)ppStream);
    if (SUCCEEDED(hr))
    {
        hr = (*ppStream)->BindToFile(pFileName, eMode, pFormatId, pWaveFormatEx, ullEventInterest);
        if (FAILED(hr))
        {
            (*ppStream)->Release();
            *ppStream = NULL;
        }
    }
    return hr;
} /* SPBindToFile */

#ifndef _UNICODE
inline HRESULT SPBindToFile( const TCHAR * pFileName, SPFILEMODE eMode, ISpStream** ppStream, 
                             const GUID * pFormatId = NULL, const WAVEFORMATEX * pWaveFormatEx = NULL,
                             ULONGLONG ullEventInterest = SPFEI_ALL_EVENTS)
{
    WCHAR szWcharFileName[MAX_PATH];
    ::MultiByteToWideChar(CP_ACP, 0, pFileName, -1, szWcharFileName, sp_countof(szWcharFileName));
    return SPBindToFile(szWcharFileName, eMode, ppStream, pFormatId, pWaveFormatEx, ullEventInterest);
}
#endif

/****************************************************************************
* SpClearEvent *
*--------------*
*   Description:
*       Helper function that can be used by clients that do not use the CSpEvent
*   class.
*
*   Returns:
*
*****************************************************************************/

inline void SpClearEvent(SPEVENT * pe)
{
    if( pe->elParamType != SPEI_UNDEFINED)
    {
        if( pe->elParamType == SPET_LPARAM_IS_POINTER ||
            pe->elParamType == SPET_LPARAM_IS_STRING)
        {
            ::CoTaskMemFree((void *)pe->lParam);
        }
        else if (pe->elParamType == SPET_LPARAM_IS_TOKEN ||
               pe->elParamType == SPET_LPARAM_IS_OBJECT)
        {
            ((IUnknown*)pe->lParam)->Release();
        }
    }
    memset(pe, 0, sizeof(*pe));
}

/****************************************************************************
* SpInitEvent *
*-------------*
*   Description:
*
*   Returns:
*
*****************************************************************************/

inline void SpInitEvent(SPEVENT * pe)
{
    memset(pe, 0, sizeof(*pe));
}

/****************************************************************************
* SpEventSerializeSize *
*----------------------*
*   Description:
*       Computes the required size of a buffer to serialize an event.  The caller
*   must specify which type of serialized event is desired -- either SPSERIALIZEDEVENT
*   or SPSERIALIZEDEVENT64.    
*
*   Returns:
*       Size in bytes required to seriailze the event.
*
****************************************************************************/

// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
template <class T>
inline ULONG SpEventSerializeSize(const SPEVENT * pEvent)

{
    ULONG ulSize = sizeof(T);

#else

inline ULONG SpEventSerializeSize(const SPEVENT * pEvent, ULONG ulSize)
{
#endif //_WIN32_WCE

    if( ( pEvent->elParamType == SPET_LPARAM_IS_POINTER ) && pEvent->lParam )
    {
        ulSize += ULONG(pEvent->wParam);
    }
    else if ((pEvent->elParamType == SPET_LPARAM_IS_STRING) && pEvent->lParam != NULL)
    {
        ulSize += (wcslen((WCHAR*)pEvent->lParam) + 1) * sizeof( WCHAR );
    }
    else if( pEvent->elParamType == SPET_LPARAM_IS_TOKEN )
    {
        CSpDynamicString dstrObjectId;
        if( ((ISpObjectToken*)(pEvent->lParam))->GetId( &dstrObjectId ) == S_OK )
        {
            ulSize += (dstrObjectId.Length() + 1) * sizeof( WCHAR );
        }
    }
    // Round up to nearest DWORD
    ulSize += 3;
    ulSize -= ulSize % 4;
    return ulSize;
}

/****************************************************************************
* SpSerializedEventSize *
*-----------------------*
*   Description:
*       Returns the size, in bytes, used by a serialized event.  The caller can
*   pass a pointer to either a SPSERIAILZEDEVENT or SPSERIALIZEDEVENT64 structure.
*
*   Returns:
*       Number of bytes used by serizlied event
*
********************************************************************* RAL ***/

// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
template <class T>
inline ULONG SpSerializedEventSize(const T * pSerEvent)
{
    ULONG ulSize = sizeof(T);

    if( ( pSerEvent->elParamType == SPET_LPARAM_IS_POINTER ) && pSerEvent->SerializedlParam )
    {
        ulSize += ULONG(pSerEvent->SerializedwParam);
    }
    else if ((pSerEvent->elParamType == SPET_LPARAM_IS_STRING || pSerEvent->elParamType == SPET_LPARAM_IS_TOKEN) &&
             pSerEvent->SerializedlParam != NULL)
    {
        ulSize += (wcslen((WCHAR*)(pSerEvent + 1)) + 1) * sizeof( WCHAR );
    }
    // Round up to nearest DWORD
    ulSize += 3;
    ulSize -= ulSize % 4;
    return ulSize;
}

#else //_WIN32_WCE

inline ULONG SpSerializedEventSize(const SPSERIALIZEDEVENT * pSerEvent, ULONG ulSize)
{
    if( ( pSerEvent->elParamType == SPET_LPARAM_IS_POINTER ) && pSerEvent->SerializedlParam )
    {
        ulSize += ULONG(pSerEvent->SerializedwParam);
    }
    else if ((pSerEvent->elParamType == SPET_LPARAM_IS_STRING || pSerEvent->elParamType == SPET_LPARAM_IS_TOKEN) &&
             pSerEvent->SerializedlParam != NULL)
    {
        ulSize += (wcslen((WCHAR*)(pSerEvent + 1)) + 1) * sizeof( WCHAR );
    }
    // Round up to nearest DWORD
    ulSize += 3;
    ulSize -= ulSize % 4;
    return ulSize;
}

inline ULONG SpSerializedEventSize(const SPSERIALIZEDEVENT64 * pSerEvent, ULONG ulSize)
{
    if( ( pSerEvent->elParamType == SPET_LPARAM_IS_POINTER ) && pSerEvent->SerializedlParam )
    {
        ulSize += ULONG(pSerEvent->SerializedwParam);
    }
    else if ((pSerEvent->elParamType == SPET_LPARAM_IS_STRING || pSerEvent->elParamType == SPET_LPARAM_IS_TOKEN) &&
             pSerEvent->SerializedlParam != NULL)
    {
        ulSize += (wcslen((WCHAR*)(pSerEvent + 1)) + 1) * sizeof( WCHAR );
    }
    // Round up to nearest DWORD
    ulSize += 3;
    ulSize -= ulSize % 4;
    return ulSize;
}

#endif //_WIN32_WCE

/*** CSpEvent helper class
*
*/
class CSpEvent : public SPEVENT
{
public:
    CSpEvent()
    {
        SpInitEvent(this);
    }
    ~CSpEvent()
    {
        SpClearEvent(this);
    }
    // If you need to take the address of a CSpEvent that is not const, use the AddrOf() method
    // which will do debug checking of parameters.  If you encounter this problem when calling
    // GetEvents from an event source, you may want to use the GetFrom() method of this class.
    const SPEVENT * operator&()
        {
                return this;
        }
    CSpEvent * AddrOf()
    {
        // Note:  This method does not ASSERT since we assume the caller knows what they are doing.
        return this;
    }
    void Clear()
    {
        SpClearEvent(this);
    }
    HRESULT CopyTo(SPEVENT * pDestEvent) const
    {
        memcpy(pDestEvent, this, sizeof(*pDestEvent));
        if ((elParamType == SPET_LPARAM_IS_POINTER) && lParam)
        {
            SPDBG_ASSERT(wParam && (wParam < 0x100000));    // this is too big!
            pDestEvent->lParam = (LPARAM)::CoTaskMemAlloc(wParam);
            if (pDestEvent->lParam)
            {
                memcpy((void *)pDestEvent->lParam, (void *)lParam, wParam);
            }
            else
            {
                pDestEvent->eEventId = SPEI_UNDEFINED;
                return E_OUTOFMEMORY;
            }
        }
        else if (elParamType == SPET_LPARAM_IS_STRING && lParam != NULL)
        {
            pDestEvent->lParam = (LPARAM)::CoTaskMemAlloc((wcslen((WCHAR*)lParam) + 1) * sizeof(WCHAR));
            if (pDestEvent->lParam)
            {
                wcscpy((WCHAR*)pDestEvent->lParam, (WCHAR*)lParam);
            }
            else
            {
                pDestEvent->eEventId = SPEI_UNDEFINED;
                return E_OUTOFMEMORY;
            }
        }
        else if (elParamType == SPET_LPARAM_IS_TOKEN ||
               elParamType == SPET_LPARAM_IS_OBJECT)
        {
            ((IUnknown*)lParam)->AddRef();
        }
        return S_OK;
    }

    HRESULT GetFrom(ISpEventSource * pEventSrc)
    {
        SpClearEvent(this);
        return pEventSrc->GetEvents(1, this, NULL);
    }
    HRESULT CopyFrom(const SPEVENT * pSrcEvent)
    {
        SpClearEvent(this);
        return static_cast<const CSpEvent *>(pSrcEvent)->CopyTo(this);
    }
    void Detach(SPEVENT * pDestEvent = NULL)
    {
        if (pDestEvent)
        {
            memcpy(pDestEvent, this, sizeof(*pDestEvent));
        }
        memset(this, 0, sizeof(*this));
    }

    template <class T>
    ULONG SerializeSize() const
    {
        return SpEventSerializeSize<T>(this);
    }

    // Call this method with either SPSERIALIZEDEVENT or SPSERIALIZEDEVENT64
    template <class T>
    void Serialize(T * pSerEvent) const
    {
        SPDBG_ASSERT(elParamType != SPET_LPARAM_IS_OBJECT);
        pSerEvent->eEventId = this->eEventId;
        pSerEvent->elParamType = this->elParamType;
        pSerEvent->ulStreamNum = this->ulStreamNum;
        pSerEvent->ullAudioStreamOffset = this->ullAudioStreamOffset;
        pSerEvent->SerializedwParam = static_cast<ULONG>(this->wParam);
        pSerEvent->SerializedlParam = static_cast<LONG>(this->lParam);
        if (lParam)
        {
            switch(elParamType)
            {
            case SPET_LPARAM_IS_POINTER:
                memcpy(pSerEvent + 1, (void *)lParam, wParam);
                pSerEvent->SerializedlParam = sizeof(T);
                break;

            case SPET_LPARAM_IS_STRING:
                wcscpy((WCHAR *)(pSerEvent + 1), (WCHAR*)lParam);
                pSerEvent->SerializedlParam = sizeof(T);
                break;

            case SPET_LPARAM_IS_TOKEN:
                {
                    CSpDynamicString dstrObjectId;
                    if( SUCCEEDED( ((ISpObjectToken*)lParam)->GetId( &dstrObjectId ) ) )
                    {
                        pSerEvent->SerializedwParam = (dstrObjectId.Length() + 1) * sizeof( WCHAR );;
                        memcpy( pSerEvent + 1, (void *)dstrObjectId.m_psz, static_cast<ULONG>(pSerEvent->SerializedwParam) );
                    }
                    pSerEvent->SerializedlParam = sizeof(T);
                }
                break;

            default:
                break;
            }
        }
    }

    template <class T>
    HRESULT Serialize(T ** ppCoMemSerEvent, ULONG * pcbSerEvent) const 
    {
// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
        *pcbSerEvent = SpEventSerializeSize<T>(this);
#else
        *pcbSerEvent = SpEventSerializeSize(this, sizeof(** ppCoMemSerEvent));
#endif
        *ppCoMemSerEvent = (T *)::CoTaskMemAlloc(*pcbSerEvent);
        if (*ppCoMemSerEvent)
        {
            Serialize(*ppCoMemSerEvent);
            return S_OK;
        }
        else
        {
            *pcbSerEvent = 0;
            return E_OUTOFMEMORY;
        }
    }


    // Call this method with either SPSERIALIZEDEVENT or SPSERIALIZEDEVENT64
    template <class T>
    HRESULT Deserialize(const T * pSerEvent, ULONG * pcbUsed = NULL)
    {
        Clear();
        HRESULT hr = S_OK;
        const UNALIGNED T * pTemp = pSerEvent;
        this->eEventId = pTemp->eEventId;
        this->elParamType = pTemp->elParamType;
        this->ulStreamNum = pTemp->ulStreamNum;
        this->ullAudioStreamOffset = pTemp->ullAudioStreamOffset;
        this->wParam = static_cast<WPARAM>(pTemp->SerializedwParam);
        this->lParam = static_cast<LPARAM>(pTemp->SerializedlParam);
        if (pTemp->SerializedlParam)
        {
            ULONG cbAlloc = 0;
            switch (pTemp->elParamType)
            {
            case SPET_LPARAM_IS_POINTER:
                cbAlloc = static_cast<ULONG>(wParam);
                break;

            case SPET_LPARAM_IS_STRING:
                cbAlloc = sizeof(WCHAR) * (1 + wcslen((const WCHAR *)(pTemp + 1)));
                break;

            case SPET_LPARAM_IS_TOKEN:
                {
                    ULONG ulDataOffset = ULONG(lParam);
                    hr = SpGetTokenFromId( (const WCHAR*)(pTemp + 1),
                                                  (ISpObjectToken **)&lParam );
                    wParam = 0;
                }
                break;
            }
            if (cbAlloc)
            {
                void * pvBuff = ::CoTaskMemAlloc(cbAlloc);
                this->lParam = (LPARAM)pvBuff;
                if (pvBuff)
                {
                    memcpy(pvBuff, pTemp + 1, cbAlloc);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        if( SUCCEEDED( hr ) && pcbUsed )
        {
// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
            *pcbUsed = SpEventSerializeSize<T>(this);
#else
            *pcbUsed = SpEventSerializeSize(this, sizeof(*pTemp));
#endif
        }
        return hr;
    }

    //
    //  Helpers for access to events.  Performs run-time checks in debug and casts
    //  data to the appropriate types
    //
    SPPHONEID Phoneme() const 
    {
        SPDBG_ASSERT(eEventId == SPEI_PHONEME);
        return (SPPHONEID)LOWORD(lParam);
    }
    SPVISEMES Viseme() const 
    {
        SPDBG_ASSERT(eEventId == SPEI_VISEME);
        return (SPVISEMES)LOWORD(lParam);
    }
    ULONG InputWordPos() const
    {
        SPDBG_ASSERT(eEventId == SPEI_WORD_BOUNDARY);
        return ULONG(lParam);
    }
    ULONG InputWordLen() const 
    {
        SPDBG_ASSERT(eEventId == SPEI_WORD_BOUNDARY);
        return ULONG(wParam);
    }
    ULONG InputSentPos() const
    {
        SPDBG_ASSERT(eEventId == SPEI_SENTENCE_BOUNDARY);
        return ULONG(lParam);
    }
    ULONG InputSentLen() const 
    {
        SPDBG_ASSERT(eEventId == SPEI_SENTENCE_BOUNDARY);
        return ULONG(wParam);
    }
    ISpObjectToken * ObjectToken() const
    {
        SPDBG_ASSERT(elParamType == SPET_LPARAM_IS_TOKEN);
        return (ISpObjectToken *)lParam;
    }
    ISpObjectToken * VoiceToken() const     // More explicit check than ObjectToken()
    {
        SPDBG_ASSERT(eEventId == SPEI_VOICE_CHANGE);
        return ObjectToken();
    }
    BOOL PersistVoiceChange() const
    {
        SPDBG_ASSERT(eEventId == SPEI_VOICE_CHANGE);
        return (BOOL)wParam;
    }
    IUnknown * Object() const
    {
        SPDBG_ASSERT(elParamType == SPET_LPARAM_IS_OBJECT);
        return (IUnknown*)lParam;
    }
    ISpRecoResult * RecoResult() const
    {
        SPDBG_ASSERT(eEventId == SPEI_RECOGNITION || eEventId == SPEI_FALSE_RECOGNITION || eEventId == SPEI_HYPOTHESIS);
        return (ISpRecoResult *)Object();
    }
    BOOL IsPaused()
    {
        SPDBG_ASSERT(eEventId == SPEI_RECOGNITION || eEventId == SPEI_SR_BOOKMARK);
        return (BOOL)(wParam & SPREF_AutoPause);
    }
    BOOL IsEmulated()
    {
        SPDBG_ASSERT(eEventId == SPEI_RECOGNITION);
        return (BOOL)(wParam & SPREF_Emulated);
    }
    const WCHAR * String() const
    {
        SPDBG_ASSERT(elParamType == SPET_LPARAM_IS_STRING);
        return (const WCHAR*)lParam;
    }
    const WCHAR * BookmarkName() const
    {
        SPDBG_ASSERT(eEventId == SPEI_TTS_BOOKMARK);
        return String();
    }
    const WCHAR * RequestTypeOfUI() const
    {
        SPDBG_ASSERT(eEventId == SPEI_REQUEST_UI);
        return String();
    }
    SPRECOSTATE RecoState() const
    {
        SPDBG_ASSERT(eEventId == SPEI_RECO_STATE_CHANGE);
        return static_cast<SPRECOSTATE>(wParam);
    }
    const WCHAR * PropertyName() const
    {
        SPDBG_ASSERT((eEventId == SPEI_PROPERTY_NUM_CHANGE && elParamType == SPET_LPARAM_IS_STRING) ||
                     (eEventId == SPEI_PROPERTY_STRING_CHANGE && elParamType == SPET_LPARAM_IS_POINTER));
        // Note: Don't use String() method here since in the case of string attributes, the elParamType
        // field specifies LPARAM_IS_POINTER, but the attribute name IS the first string in this buffer
        return (const WCHAR*)lParam;
    }
    const LONG PropertyNumValue() const 
    {
        SPDBG_ASSERT(eEventId == SPEI_PROPERTY_NUM_CHANGE);
        return static_cast<LONG>(wParam);
    }
   const WCHAR * PropertyStringValue() const
   {
       // Search for the first NULL and return pointer to the char past it.
       SPDBG_ASSERT(eEventId == SPEI_PROPERTY_STRING_CHANGE);
       const WCHAR * psz = (const WCHAR *)lParam; // moved this from for init
       for (; *psz; psz++) {}
       return psz + 1;
   }
    SPINTERFERENCE Interference() const
    {
        SPDBG_ASSERT(eEventId == SPEI_INTERFERENCE);
        return static_cast<SPINTERFERENCE>(lParam);
    }
    HRESULT EndStreamResult() const
    {
        SPDBG_ASSERT(eEventId == SPEI_END_SR_STREAM);
        return static_cast<HRESULT>(lParam);
    }
    BOOL InputStreamReleased() const
    {
        SPDBG_ASSERT(eEventId == SPEI_END_SR_STREAM);
        return (wParam & SPESF_STREAM_RELEASED) ? TRUE : FALSE;
    }
};

class CSpPhrasePtr
{
public:
    SPPHRASE    *   m_pPhrase;
    CSpPhrasePtr() : m_pPhrase(NULL) {}
    CSpPhrasePtr(ISpPhrase * pPhraseObj, HRESULT * phr)
    {
        *phr = pPhraseObj->GetPhrase(&m_pPhrase);
    }
    ~CSpPhrasePtr()
    {
        ::CoTaskMemFree(m_pPhrase);
    }
        //The assert on operator& usually indicates a bug.  If this is really
        //what is needed, however, take the address of the m_pPhrase member explicitly.
        SPPHRASE ** operator&()
        {
            SPDBG_ASSERT(m_pPhrase == NULL);
            return &m_pPhrase;
        }
    operator SPPHRASE *() const
    {
        return m_pPhrase;
    }
        SPPHRASE & operator*() const
        {
                SPDBG_ASSERT(m_pPhrase);
                return *m_pPhrase;
        }
    SPPHRASE * operator->() const
    {
        return m_pPhrase;
    }
        bool operator!() const
        {
                return (m_pPhrase == NULL);
        }
    void Clear()
    {
        if (m_pPhrase)
        {
            ::CoTaskMemFree(m_pPhrase);
            m_pPhrase = NULL;
        }
    }
    HRESULT GetFrom(ISpPhrase * pPhraseObj)
    {
        Clear();
        return pPhraseObj->GetPhrase(&m_pPhrase);
    }
};


template <class T>
class CSpCoTaskMemPtr
{
public:
    T       * m_pT;
    CSpCoTaskMemPtr() : m_pT(NULL) {}
    CSpCoTaskMemPtr(void * pv) : m_pT((T *)pv) {}
    CSpCoTaskMemPtr(ULONG cElements, HRESULT * phr)
    {
        m_pT = (T *)::CoTaskMemAlloc(cElements * sizeof(T));
        *phr = m_pT ? S_OK : E_OUTOFMEMORY;
    }
    ~CSpCoTaskMemPtr()
    {
        ::CoTaskMemFree(m_pT);
    }
    void Clear()
    {
        if (m_pT)
        {
            ::CoTaskMemFree(m_pT);
            m_pT = NULL;
        }
    }
    HRESULT Alloc(ULONG cArrayElements = 1)
    {
        m_pT = (T *)::CoTaskMemRealloc(m_pT, sizeof(T) * cArrayElements);
        SPDBG_ASSERT(m_pT);
        return (m_pT ? S_OK : E_OUTOFMEMORY);
    }
    void Attach(void * pv)
    {
        Clear();
        m_pT = (T *)pv;
    }
    T * Detatch()
    {
        T * pT = m_pT;
        m_pT = NULL;
        return pT;
    }
        //The assert on operator& usually indicates a bug.  If this is really
        //what is needed, however, take the address of the m_pT member explicitly.
        T ** operator&()
        {
        SPDBG_ASSERT(m_pT == NULL);
                return &m_pT;
        }
    T * operator->()
    {
        SPDBG_ASSERT(m_pT != NULL);
        return m_pT;
    }
    operator T *()
    {
        return m_pT;
    }
        bool operator!() const
        {
                return (m_pT == NULL);
        }
};

/**** Helper function used to create a new phrase object from an array of
    test words. Each word in the string is converted to a phrase element.
    This is useful to create a phrase to pass to the EmulateRecognition method.
    The method can convert standard words as well as words with the
    "/display_text/lexical_form/pronounciation;" word format.
    You can also specify the DisplayAttributes for each element if desired. 
    If prgDispAttribs is NULL then the DisplayAttribs for each element default to 
    SPAF_ONE_TRAILING_SPACE. ****/
inline HRESULT CreatePhraseFromWordArray(const WCHAR ** ppWords, ULONG cWords,
                             SPDISPLYATTRIBUTES * prgDispAttribs,
                             ISpPhraseBuilder **ppResultPhrase,
                             LANGID LangId = 0,
                             CComPtr<ISpPhoneConverter> cpPhoneConv = NULL)
{
    SPDBG_FUNC("CreatePhraseFromWordArray");
    HRESULT hr = S_OK;

    if ( cWords == 0 || ppWords == NULL || ::IsBadReadPtr(ppWords, sizeof(*ppWords) * cWords ) )
    {
        return E_INVALIDARG;
    }

    if ( prgDispAttribs != NULL && ::IsBadReadPtr(prgDispAttribs, sizeof(*prgDispAttribs) * cWords ) )
    {
        return E_INVALIDARG;
    }

    ULONG    cTotalChars = 0;
    ULONG    i;
    WCHAR** pStringPtrArray = (WCHAR**)::CoTaskMemAlloc( cWords * sizeof(WCHAR *));
    if ( !pStringPtrArray )
    {
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < cWords; i++)
    {
        cTotalChars += wcslen(ppWords[i])+1;
    }

    CSpDynamicString dsText(cTotalChars);
    if(dsText.m_psz == NULL)
    {
        ::CoTaskMemFree(pStringPtrArray);
        return E_OUTOFMEMORY;
    }
    CSpDynamicString dsPhoneId(cTotalChars);
    if(dsPhoneId.m_psz == NULL)
    {
        ::CoTaskMemFree(pStringPtrArray);
        return E_OUTOFMEMORY;
    }
    SPPHONEID* pphoneId = (SPPHONEID*)((WCHAR *)dsPhoneId); // improper casting

    SPPHRASE Phrase;
    memset(&Phrase, 0, sizeof(Phrase));
    Phrase.cbSize = sizeof(Phrase);

    if(LangId == 0)
    {
        LangId = SpGetUserDefaultUILanguage();
    }

    if(cpPhoneConv == NULL)
    {
        hr = SpCreatePhoneConverter(LangId, NULL, NULL, &cpPhoneConv);
        if(FAILED(hr))
        {
            ::CoTaskMemFree(pStringPtrArray);
            return hr;
        }
    }

    SPPHRASEELEMENT *pPhraseElement = new SPPHRASEELEMENT[cWords];
    if(pPhraseElement == NULL)
    {
        ::CoTaskMemFree(pStringPtrArray);
        return E_OUTOFMEMORY;
    }
    memset(pPhraseElement, 0, sizeof(SPPHRASEELEMENT) * cWords); // !!!
    
    WCHAR * pText = dsText;
    for (i = 0; SUCCEEDED(hr) && i < cWords; i++)
    {
        WCHAR *p = pText;
        pStringPtrArray[i] = pText;
        wcscpy( pText, ppWords[i] );
        pText += wcslen( p ) + 1;

        if (*p == L'/')
        {
            //This is a compound word
            WCHAR* pszFirstPart = ++p;
            WCHAR* pszSecondPart = NULL;
            WCHAR* pszThirdPart = NULL;

            while (*p && *p != L'/')
            {
                p++;
            }
            if (*p == L'/')
            {
                //It means we stop at the second '/'
                *p = L'\0';
                pszSecondPart = ++p;
                while (*p && *p != L'/')
                {
                    p++;
                }
                if (*p == L'/')
                {
                    //It means we stop at the third '/'
                    *p = L'\0';
                    pszThirdPart = ++p;
                }
            }

            pPhraseElement[i].pszDisplayText = pszFirstPart;
            pPhraseElement[i].pszLexicalForm = pszSecondPart ? pszSecondPart : pszFirstPart;

            if ( pszThirdPart)
            {
                hr = cpPhoneConv->PhoneToId(pszThirdPart, pphoneId);
                if (SUCCEEDED(hr))
                {
                    pPhraseElement[i].pszPronunciation = pphoneId;
					pphoneId += wcslen((const wchar_t *)pphoneId) + 1; // improper casting
                }
            }
        }
        else
        {
            //It is the simple format, only have one form, use it for everything.
            pPhraseElement[i].pszDisplayText = NULL;
            pPhraseElement[i].pszLexicalForm = p;
            pPhraseElement[i].pszPronunciation = NULL;
        }

        pPhraseElement[i].bDisplayAttributes = (BYTE)(prgDispAttribs ? prgDispAttribs[i] : SPAF_ONE_TRAILING_SPACE);
        pPhraseElement[i].RequiredConfidence = SP_NORMAL_CONFIDENCE;
        pPhraseElement[i].ActualConfidence =  SP_NORMAL_CONFIDENCE;
    }

    Phrase.Rule.ulCountOfElements = cWords;
    Phrase.pElements = pPhraseElement;
    Phrase.LangID = LangId;

    CComPtr<ISpPhraseBuilder> cpPhrase;
    if (SUCCEEDED(hr))
    {
        hr = cpPhrase.CoCreateInstance(CLSID_SpPhraseBuilder);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpPhrase->InitFromPhrase(&Phrase);
    }
    if (SUCCEEDED(hr))
    {
        *ppResultPhrase = cpPhrase.Detach();
    }

    delete pPhraseElement;
    ::CoTaskMemFree(pStringPtrArray);

    return hr;
}

/**** Helper function used to create a new phrase object from a 
    test string. Each word in the string is converted to a phrase element.
    This is useful to create a phrase to pass to the EmulateRecognition method.
    The method can convert standard words as well as words with the
    "/display_text/lexical_form/pronounciation;" word format ****/
inline HRESULT CreatePhraseFromText(const WCHAR *pszOriginalText,
                             ISpPhraseBuilder **ppResultPhrase,
                             LANGID LangId = 0,
                             CComPtr<ISpPhoneConverter> cpPhoneConv = NULL)
{
    SPDBG_FUNC("CreatePhraseFromText");
    HRESULT hr = S_OK;

    //We first trim the input text
    CSpDynamicString dsText(pszOriginalText);
    if(dsText.m_psz == NULL)
    {
        return E_OUTOFMEMORY;
    }
    dsText.TrimBoth();

    ULONG cWords = 0;
    BOOL fInCompoundword = FALSE;

    // Set first array pointer (if *p).
    WCHAR *p = dsText;
    while (*p)
    {
        if( iswspace(*p) && !fInCompoundword)
        {
            cWords++;
            *p++ = L'\0';
            while (*p && iswspace(*p))
            {
                *p++ = L'\0';
            }
            // Add new array pointer.  Use vector.
        }
        else if (*p == L'/' && !fInCompoundword)
        {
            fInCompoundword = TRUE;
        }
        else if (*p == L';' && fInCompoundword)
        {
            fInCompoundword = FALSE;
            *p++ = L'\0';
            // Add new array element.
        }
        else
        {
            p++;
        }
    }

    cWords++;

    WCHAR** pStringPtrArray = (WCHAR**)::CoTaskMemAlloc( cWords * sizeof(WCHAR *));
    if ( !pStringPtrArray )
    {
        hr = E_OUTOFMEMORY;
    }

    if ( SUCCEEDED( hr ) )
    {
        p = dsText;
        for (ULONG i=0; i<cWords; i++)
        {
            pStringPtrArray[i] = p;
            p += wcslen(p)+1;
        }

        hr = CreatePhraseFromWordArray((const WCHAR **)pStringPtrArray, cWords, NULL, ppResultPhrase, LangId, cpPhoneConv);

        ::CoTaskMemFree(pStringPtrArray);
    }
    return hr;
}

#endif /* This must be the last line in the file */
