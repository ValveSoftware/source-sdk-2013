//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// srenginecompliance.h
// 
//******************************************************************************
#ifndef _srenginecompliance_h_
#define _srenginecompliance_h_
#include "sapi.h"

#include <vector>
typedef std::vector<CComPtr<ISpRecoResult> > RESULTVECTOR;

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0400

#define WILDCARDTEXT L"..."
#define WAIT_TIME           25000
#define TEST_CASES				20
#define NUMOFRECOCONTEXTS 2
#define INTERATIONS		1
#define TESTEVENT_WAIT     250

template <const int i = MAX_PATH>
class CSpTString
{
#ifdef _UNICODE
private:
    WCHAR    m_wString[i];    
public:
    CSpTString(const WCHAR * psz){
        wcscpy(m_wString, psz);
    }
    operator const WCHAR *() { return m_wString; }
    WCHAR * operator =(const WCHAR * psz) {
        wcscpy(m_wString, psz);
        return m_wString;
    } 
#else
private:
    char   m_aString[i];
public:
    CSpTString(const WCHAR * psz)
    {
        ::WideCharToMultiByte(CP_ACP, 0, psz, -1, m_aString, i, NULL, NULL);
    }
    operator const char *() { return m_aString; }
    const char * operator =(const WCHAR * psz)
    {
        ::WideCharToMultiByte(CP_ACP, 0, psz, -1, m_aString, i, NULL, NULL);
        return m_aString;
    }
#endif
};


HRESULT RegRecursiveCopyKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, HKEY hkeyDest, LPCTSTR szSubKeyDest, BOOL fClearDest = FALSE);
HRESULT RegRecursiveDeleteKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, BOOL fKeepEmptyRoot = FALSE);

LANGID GetDefaultEnginePrimaryLangId();

struct PropertyNum
{
	const WCHAR* pPropertyNumNames;
	LONG ulLowLimit;
	LONG ulHighLimit;
};

const PropertyNum PropertyNumTable[] =
{
	{SPPROP_RESOURCE_USAGE, 0, 100},
	{SPPROP_HIGH_CONFIDENCE_THRESHOLD, 0, 100},
	{SPPROP_NORMAL_CONFIDENCE_THRESHOLD, 0, 100},
	{SPPROP_LOW_CONFIDENCE_THRESHOLD, 0, 100},
	{SPPROP_RESPONSE_SPEED, 0, 10000},
	{SPPROP_COMPLEX_RESPONSE_SPEED, 0, 10000},
	{SPPROP_ADAPTATION_ON, 0, 1}
};



inline HRESULT CreateAppLexicon(
	const WCHAR * pszLangIndependentName, 
	LANGID langid, 
	const WCHAR * pszLangDependentName, 
	WCHAR* pwszAttributes, 
	ISpLexicon** ppLexicon)
{
    HRESULT hr = S_OK;

    CComPtr<ISpObjectToken> cpToken;
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    if(SUCCEEDED(hr))
    {
        hr = SpCreateNewTokenEx(SPCAT_APPLEXICONS, pszLangIndependentName, &CLSID_SpUnCompressedLexicon, pszLangIndependentName, langid, pszLangDependentName, &cpToken, &cpDataKeyAttribs);
    }
    if(SUCCEEDED(hr))
    {
        CSpDynamicString str(pwszAttributes); // make a copy of the pwszAttributes
        WCHAR* p = wcstok(str, L";");
        while(SUCCEEDED(hr) && p)
        {
            if(WCHAR* pe = wcschr(p, L'='))
            {
                *(pe++) = L'\0';
                hr = cpDataKeyAttribs->SetStringValue(p, pe);
            }
            else
            {
                hr = cpDataKeyAttribs->SetStringValue(p, L"");
            }
            p = wcstok(NULL, L";");
        }
    }
    if(SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(cpToken, ppLexicon);
    }

	return hr;
}



inline HRESULT RemoveTestAppLexicon(WCHAR* pwszAttributes)
{

	HRESULT							hr = S_OK;
	CComPtr<IEnumSpObjectTokens>				 cpEnum;

	CComPtr<ISpObjectToken>			cpSpObjectToken;


	if( SUCCEEDED( hr ) )
	{
		// Get tts test engine voice 
		hr = SpEnumTokens(SPCAT_APPLEXICONS, pwszAttributes, NULL, &cpEnum);
    }
	if( hr == S_OK )
	{
		hr = cpEnum->Next(1, &cpSpObjectToken, NULL);
	}
	if( hr == S_OK )
	{
		hr = cpSpObjectToken->Remove(NULL);

	}

	return hr;
}


#endif // _srenginecompliance_h_