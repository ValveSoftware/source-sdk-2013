//******************************************************************************
// Copyright (c) Microsoft Corporation.  All rights reserved.
// ttscomp.h
//
//******************************************************************************

#ifndef __TTSCOMP_H__
#define __TTSCOMP_H__

#include <windows.h>

#include <spdebug.h>
#define ATLASSERT(expr) SPDBG_ASSERT(expr)
// Replace ATLASSERT with SPDBG_ASSERT.
// If the compiler says macro redefinition at this line, you need to remove any
// #include <atlbase.h> before include this file
#include <atlbase.h>

#include <tchar.h>
#include <math.h>
#include <time.h>
#include <kato.h>
#include <tux.h>
#include "sapi.h"
#include "sapiddk.h"
#include <sperror.h>
#include <sphelper.h>
#include "spgeterrormsg.h"
#include "resource.h"
#include <wchar.h>
#include <string>


// Added for logging across .cpp files
extern CKato *g_pKato;
extern SPS_SHELL_INFO *g_pShellInfo;

//************* Helper function prototypes *************
HRESULT GetWStrFromRes( UINT id, WCHAR* szwStr, int	cchBufferMax = MAX_PATH);
HRESULT RegRecursiveCopyKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, HKEY hkeyDest, LPCTSTR szSubKeyDest, BOOL fClearDest = FALSE);
HRESULT RegRecursiveDeleteKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, BOOL fKeepEmptyRoot = FALSE);
void CleanupVoiceAndEngine();
HRESULT GetFreqFromSamples (ISpStream * pStream, int sampFreq, ULONG &length, ULONG * pAverage );
int ComputeF0 (float* data, int nData, int sampFreq, int minF0, int maxF0, int feaSampFreq, 
           float** f0, int* nF0, float **pRms, int *nRms);
HRESULT SpGetLanguageIdFromDefaultVoice(LANGID *plangid);
HRESULT SPCreateStreamOnHGlobal(HGLOBAL hGlobal, BOOL fDeleteOnRelease, REFGUID rguidFormatId,
								const WAVEFORMATEX * pwfex, ISpStream ** ppStream );

#define TTS_WAITTIME 30000 //30 seconds
#define TTS_WAITTIME_LONG 90000 //90 seconds

// Useful macros and definitions
#define ZeroStruct(structvar) memset(&(structvar),0,sizeof(structvar))
#define SP_IS_BAD_WRITE_PTR(p)     ( SPIsBadWritePtr( p, sizeof(*(p)) ))

inline BOOL SPIsBadWritePtr( void* pMem, UINT Size )
{
#ifdef _DEBUG
    BOOL bIsBad = ::IsBadWritePtr( pMem, Size );
    SPDBG_ASSERT(!bIsBad);
    return bIsBad;
#else
    return ::IsBadWritePtr( pMem, Size );
#endif
}

template <class T>
class CAutoReleaseComPtr: public CComPtr<T>
{
public:
	T ** operator&()
	{
        Release();
        return CComPtr<T>::operator &();
	}
};

#define DOCHECKHRGOTO(statement) statement CHECKHRGOTO(hr, tpr, #statement);
#define DOCHECKHR(statement) statement CHECKHR(hr, tpr, #statement);

inline LPCSTR CatMsg( LPCSTR psz1, LPCSTR psz2 = NULL, LPCSTR psz3 = NULL, LPCSTR psz4 = NULL, LPCSTR psz5 = NULL)
{
   static std::string s_str;

    { // make a new copy so that it works for CatMsg(CatMsg(str1, ...), str2, ...);
        std::string str = psz1;
        if(psz2) str += psz2;
        if(psz3) str += psz3;
        if(psz4) str += psz4;
        if(psz5) str += psz5;
        s_str = str;
    }

    return s_str.c_str();
}

#define CHECKHRALTId( hr, althr, tpr, id )						\
{                                                               \
    if( FAILED( hr ) && (hr != althr) )							\
    {                                                           \
        {                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if(SUCCEEDED(GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
			g_pKato->Comment(10, SpGetErrorMsg(hr));			\
        }                                                       \
        tpr = TPR_FAIL;                                         \
    }                                                           \
}

#define CHECKHRALTIdEx( hr, althr, tpr, id, topic )				\
{                                                               \
																\
    if( FAILED( hr ) && (hr != althr) )							\
    {                                                           \
        {                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
			g_pKato->Comment(10, CatMsg(SpGetErrorMsg(hr), " in ", topic));	\
        }                                                       \
        tpr = TPR_FAIL;                                         \
    }                                                           \
}

#define CHECKHRALTGOTOIdEx( hr, althr, tpr, id, topic )				\
{                                                               \
																\
    if( FAILED( hr ) && (hr != althr) )							\
    {                                                           \
        {                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
			g_pKato->Comment(10, CatMsg(SpGetErrorMsg(hr), " in ", topic));	\
        }                                                       \
        tpr = TPR_ABORT;                                        \
		goto EXIT;                                              \
    }                                                           \
}
// for more general asserts.
// will not output error message based on hr
#define CHECKASSERTGOTOId( cond, tpr, id )						\
{                                                               \
																\
    if( !(cond) )												\
    {                                                           \
	    {                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg("Condition: ", #cond, " Failed"));	\
        tpr = TPR_ABORT;                                         \
		goto EXIT;                                              \
    }                                                           \
}

#define CHECKASSERTGOTOIdEx( cond, tpr, id, topic )				\
{                                                               \
																\
    if( !(cond) )												\
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg("Condition: ", #cond, " Failed in ", topic));	\
        tpr = TPR_ABORT;                                         \
		goto EXIT;                                              \
    }                                                           \
}

#define CHECKASSERTId( cond, tpr, id )							\
{                                                               \
																\
    if( !(cond) )												\
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg("Condition: ", #cond, " Failed"));	\
        tpr = TPR_FAIL;                                         \
    }                                                           \
}

#define CHECKISSUPPORTEDId( cond, tpr, id )							\
{                                                               \
																\
    if( !(cond) )												\
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg("Condition: ", #cond, " Failed"));	\
        tpr = TPR_UNSUPPORTED;                                         \
    }                                                           \
}

#define CHECKASSERTIdEx( cond, tpr, id, topic )					\
{                                                               \
																\
    if( !(cond) )												\
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg("Condition: ", #cond, " Failed in ", topic));	\
        tpr = TPR_FAIL;                                         \
    }                                                           \
}
// Useful macros and definitions
#define CHECKHRId( hr, tpr, id )								\
{                                                               \
    if( FAILED( hr ) )                                          \
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
			g_pKato->Comment(10, SpGetErrorMsg(hr));			\
        }                                                       \
        tpr = TPR_FAIL;                                         \
    }                                                           \
}

#define CHECKHRIdEx( hr, tpr, id, topic )						\
{                                                               \
																\
    if( FAILED( hr ) )                                          \
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg(SpGetErrorMsg(hr), " in ", topic ));	\
        tpr = TPR_FAIL;                                         \
    }                                                           \
}
	
#define CHECKHRGOTOId( hr, tpr, id)								\
{																\
    if( FAILED( hr ) )                                          \
    {                                                           \
		{                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, SpGetErrorMsg(hr));				\
        tpr = TPR_ABORT;                                        \
        goto EXIT;                                              \
    }                                                           \
}

#define CHECKHRGOTOIdEx( hr, tpr, id, topic )					\
{																\
    if( FAILED( hr ) )                                          \
    {                                                           \
        {                                                       \
			WCHAR szwDebug[MAX_PATH]=L"";						\
			if (SUCCEEDED (GetWStrFromRes( id, szwDebug )))		\
			{													\
				USES_CONVERSION;								\
				g_pKato->Comment(5, W2T(szwDebug));				\
			}													\
		}														\
		g_pKato->Comment(10, CatMsg (SpGetErrorMsg(hr), " in ", topic));	\
        tpr = TPR_ABORT;                                        \
        goto EXIT;                                              \
    }                                                           \
}
// Useful macros and definitions
#define CHECKHR( hr, tpr, szError )                             \
{                                                               \
    if( FAILED( hr ) )                                          \
    {                                                           \
        if( szError)                                   \
        {                                                       \
            g_pKato->Comment(5, szError);						\
			g_pKato->Comment(10, SpGetErrorMsg(hr));			\
        }                                                       \
        tpr = TPR_FAIL;                                         \
    }                                                           \
}

#define CHECKHRGOTO( hr, tpr, szError )                         \
{                                                               \
    if( FAILED( hr ) )                                          \
    {                                                           \
        if( szError )                                   \
        {                                                       \
            g_pKato->Comment(5, szError);						\
			g_pKato->Comment(10, SpGetErrorMsg(hr));			\
        }                                                       \
        tpr = TPR_ABORT;                                        \
        goto EXIT;                                              \
    }                                                           \
}

// Get a random number between min and max
#define GET_RANDOM( min, max ) ((rand() % (int)(((max) + 1) - (min))) + (min))


/***** Helper Classes *****/
class CEventingTest : public ISpNotifyCallback
{
  /*=== Methods =======*/
  public:
    STDMETHODIMP NotifyCallback(WPARAM wParam, LPARAM lParam) { m_bCallbackNotified = true; return S_OK; }
    
    BOOL    m_bCallbackNotified;

    CEventingTest() { m_bCallbackNotified = false; }
};

//speak two streams: stream1 and stream2, and return back samples: sample1 and sample2
inline HRESULT t_SpeakTwoStreams(ISpVoice *cpVoice, WCHAR *stream1,
					 WCHAR *stream2, ULONG &sample1, ULONG &sample2)
{	

	HRESULT							hr = S_OK;
	CComPtr<ISpStream>			    cpStream1;
    CComPtr<ISpStream>				cpStream2;
    STATSTG							Stat;

	CSpStreamFormat					InFormat(SPSF_22kHz16BitMono, &hr); 

	// Create stream #1
	if( SUCCEEDED( hr ) )
	{
        hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream1 );
	}   
    // Set the default output format
	if( SUCCEEDED( hr ) )
	{
		hr = cpVoice->SetOutput( cpStream1, TRUE);
	}

	if( SUCCEEDED( hr ) )
	{
		hr = cpVoice->Speak(stream1, 0, 0);
	}              

	if( SUCCEEDED( hr ) )
    {
		hr = cpStream1->Stat( &Stat, STATFLAG_NONAME );
		sample1 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);
	}
  
	// Create stream #2
	if( SUCCEEDED( hr ) )
    {
        hr = SPCreateStreamOnHGlobal( NULL, true, InFormat.FormatId(), InFormat.WaveFormatExPtr(), &cpStream2 );
	}
     
    // Set the default output format
   	if( SUCCEEDED( hr ) )
    {
		hr = cpVoice->SetOutput( cpStream2, TRUE);
	}

	if( SUCCEEDED( hr ) )
    {
		hr = cpVoice->Speak(stream2, 0, 0);
	}

	if( SUCCEEDED( hr ) )
    {
		hr = cpStream2->Stat( &Stat, STATFLAG_NONAME );
		sample2 = (ULONG)Stat.cbSize.QuadPart / sizeof(WCHAR);
	}

	return hr;
}

inline HRESULT GetFreqFromSamples (ISpStream * pStream, int sampFreq, ULONG &length, ULONG * pAverage )
{
	//this is a help function to get Frequency from the wav stream. The test allocates a 
	// buffer with length * sizeof (BUCKET) bytes.
	
	//Note: The application uses 16 bits wave stream. 

	HRESULT					hr = S_OK;
	ULONG					cbActuallyRead = 0;	
    ULONG                   cSamples = 0;
	ULONG					freq =0;
	BOOL					bPositive = FALSE;

	typedef short BUCKET;   //use 16 bits  
	BUCKET					*buff = NULL, *buffHead = NULL;
	float					*afUnfilteredBuffer = NULL, *afUnfilteredBufferHead = NULL;

	int						feaSampFreq	= 100;  //sample rate
	int						minF0 = 30;	//low freq  Range of frequences interested
	int						maxF0 = 500;			//high freq

	float					*f0 = NULL;
	int						nF0 = 0;

	float					*pRms = NULL;
	int						nRms = 0;
	*pAverage = 0;
	float  *pAverageRms =0;
	int NumRms =0;

	buff = (BUCKET *)::CoTaskMemAlloc ( length * sizeof (BUCKET) );
	if(!buff)
	{
		return E_OUTOFMEMORY;
	}

	afUnfilteredBuffer = (float *)::CoTaskMemAlloc ( length * sizeof ( float) );
	if(!afUnfilteredBuffer)
	{
		return E_OUTOFMEMORY;
	}

	pAverageRms = (float *)::CoTaskMemAlloc ( length * sizeof ( float) );
	if(!pAverageRms)
	{
		return E_OUTOFMEMORY;
	}

	//save the head pointer
	buffHead = buff;
	afUnfilteredBufferHead = afUnfilteredBuffer;

	// Read stream into a buffer until EOF
	if( SUCCEEDED( hr ) )
	{
		hr = pStream->Read( buff, length * sizeof (BUCKET), &cbActuallyRead); 
	}

	cSamples = cbActuallyRead / sizeof(BUCKET);

	// Create a low frequency filter and run the samples through
	if( SUCCEEDED( hr ) )
	{
		for( ULONG i=0; i< cSamples; i++ )
		{
			afUnfilteredBuffer[i] = (float)buff[i];
		}
	}

	if (!ComputeF0 (afUnfilteredBuffer, cSamples, sampFreq, minF0, maxF0, 
		feaSampFreq, &f0, &nF0, &pRms, &nRms))
	{
		hr = E_FAIL;
		return hr;
	}
	*pAverage = 0;
	for( ULONG i=0; i < nF0; i++ )
	{							
		if( f0[i] >0 )
		{
			*pAverage += (ULONG)f0[i];
			freq++;
		}		
	}

	*pAverageRms = 0;
	for( ULONG j=0; j < nRms; j++ )
	{							
		if( pRms[j] >0 )
		{
			*pAverageRms += (ULONG)pRms[j];
			NumRms++;
		}		
	}
	if (NumRms > 0 )
		*pAverageRms /= NumRms;
		
	if (freq > 0 )
		*pAverage /= freq;
	
	::CoTaskMemFree (buffHead);
	::CoTaskMemFree (afUnfilteredBufferHead);
	::CoTaskMemFree (pAverageRms);
	if ( f0 )
	{
		free (f0);
		f0 = NULL;
	}
	if ( pRms )
	{
		free (pRms);
		pRms = NULL;
	}
	return hr;
}

inline HRESULT GetAmpFromSamples (ISpStream * pStream, ULONG * pamp )
{
	//this is a help function to get amplitude from the stream. The test assume that
	// the max of the buffer for Read is 1000 X 2 bytes, i.e. everytime Read gets called, at most
	// 2000 bytes are readed.
	
	//Note: The application uses 16 bits wave stream. 

	HRESULT					hr = S_OK;

	ULONG					cbActuallyRead = 1;
	ULONG					nRead = 1000;  
	ULONG					totalAmp = 0, average = 0;

	typedef short BUCKET;   
	BUCKET					*buff = NULL, *buffHead = NULL;

	buff = (BUCKET *)::CoTaskMemAlloc ( nRead * sizeof (BUCKET) );
	if(!buff)
	{
		return E_OUTOFMEMORY;
	}

	//save the head pointer
	buffHead = buff;

	// Read stream into a buffer until EOF
	while ( cbActuallyRead && SUCCEEDED( hr ) )
	{
		if( SUCCEEDED( hr ) )
		{
			hr = pStream->Read( buff, nRead * sizeof (BUCKET), &cbActuallyRead); 
		}

		// Sum abs value of amplitude
		if( SUCCEEDED( hr ) )
		{
			average += (cbActuallyRead / sizeof(BUCKET));

			for( ULONG i=0; i < cbActuallyRead / sizeof(BUCKET); i++ )
			{
				
				if( buff[i] < 0 )
				{
					buff[i] = -(buff[i]);
				}
				totalAmp += buff[i];
			}
		}
    }
	
	if ( average > 0)
		*pamp = totalAmp/average;

	::CoTaskMemFree (buffHead);
	return hr;
}


inline HRESULT TTSSymToPhoneId(LANGID langid, WCHAR* pszSym, WCHAR* pszPhoneId)
{
	CComPtr<ISpPhoneConverter> cpPhoneConv;
	CComPtr<ISpObjectToken> cpToken;
	CComPtr<IEnumSpObjectTokens> cpEnum;
	HRESULT hr = S_OK;

    if(SUCCEEDED(hr))
    {
        hr = SpCreatePhoneConverter(langid, NULL, NULL, &cpPhoneConv);
    }
	if(SUCCEEDED(hr))
	{
		hr = cpPhoneConv->PhoneToId(pszSym, pszPhoneId);
	}
	return hr;
}

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

inline HRESULT RegRecursiveDeleteKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, BOOL fKeepEmptyRoot)
{
    HRESULT hr = S_OK;
    LONG l;
    HKEY hkey;

    // try to use RegDeleteKey.  On Win95/98 this will be enough.
    l = ::RegDeleteKey(hkeySrc, szSubKeySrc);
    hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
    if(SUCCEEDED(hr)) 
    {
        if(fKeepEmptyRoot)
        {
            HKEY hkey;
            l = ::RegCreateKeyEx(hkeySrc, szSubKeySrc, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkey, NULL);
            hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
            if(SUCCEEDED(hr))
            {
                ::RegCloseKey(hkey);
            }
        }
        return hr;
    }

    l = ::RegOpenKeyEx(hkeySrc, szSubKeySrc, 0, KEY_ALL_ACCESS, &hkey);
    hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
    if(SUCCEEDED(hr)) // open hkey
    {
        TCHAR szName[MAX_PATH];
        DWORD cbName;

        // recursively copy the subkeys
        if(SUCCEEDED(hr))
        {
            for(DWORD i = 0; SUCCEEDED(hr); i++)
            {
                cbName = MAX_PATH;

                // always enum the first subkey because the old one has been deleted
                l = ::RegEnumKeyEx(hkey, 0, szName, &cbName, NULL, NULL, NULL, NULL);
                hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);

                if(SUCCEEDED(hr))
                {
                    hr = RegRecursiveDeleteKey(hkey, szName);
                }
            }
            if(hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS)) hr = S_OK;
        }

        ::RegCloseKey(hkey);
    }

    if(SUCCEEDED(hr) && !fKeepEmptyRoot)
    {
        l = ::RegDeleteKey(hkeySrc, szSubKeySrc);
        hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
    }

    if(hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS)) hr = SPERR_NO_MORE_ITEMS;
    if(hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND)) hr = SPERR_NOT_FOUND;
    return hr;
}

inline HRESULT RegRecursiveCopyKey(HKEY hkeySrc, LPCTSTR szSubKeySrc, HKEY hkeyDest, LPCTSTR szSubKeyDest, BOOL fClearDest)
{
    HRESULT hr = S_OK;
    LONG l;
    HKEY hkey1, hkey2;

    l = ::RegOpenKeyEx(hkeySrc, szSubKeySrc, 0, KEY_ALL_ACCESS, &hkey1);
    hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);

    if(SUCCEEDED(hr))
    {
        if(fClearDest)
        {
            hr = RegRecursiveDeleteKey(hkeyDest, szSubKeyDest);
            if(hr == SPERR_NOT_FOUND)
            {
                hr = S_OK;
            }
        }

        if(SUCCEEDED(hr))
        {
            l = ::RegCreateKeyEx(hkeyDest, szSubKeyDest, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey2, NULL);
            hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
        }

        if(SUCCEEDED(hr))
        {
            TCHAR szName[MAX_PATH];
            BYTE data[MAX_PATH];
            DWORD cbName, cbData, dwType; 

            // copy the values
            if(SUCCEEDED(hr))
            {
                for(DWORD i = 0; SUCCEEDED(hr); i++)
                {
                    BYTE* _data = data;
                    cbName = cbData = MAX_PATH;
                    l = ::RegEnumValue(hkey1, i, szName, &cbName, NULL, &dwType, data, &cbData);
                    hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
                    if(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)) // buffer is not large enough to hold the data
                    {
                        _data = new BYTE[cbData];
                        if(_data)
                        {
                            l = ::RegEnumValue(hkey1, i, szName, &cbName, NULL, &dwType, _data, &cbData);
                            hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
                        }
                    }
                    if(SUCCEEDED(hr))
                    {
                        l = ::RegSetValueEx(hkey2, szName, NULL, dwType, _data, cbData);
                        hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);
                    }
                    if(_data && _data != data)
                    {
                        delete[] _data;
                    }
                }
                if(hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS)) hr = S_OK;
            }

            // recursively copy the subkeys
            if(SUCCEEDED(hr))
            {
                for(DWORD i = 0; SUCCEEDED(hr); i++)
                {
                    cbName = MAX_PATH;
                    l = ::RegEnumKeyEx(hkey1, i, szName, &cbName, NULL, NULL, NULL, NULL);
                    hr = (l == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(l);

                    if(SUCCEEDED(hr))
                    {
                        hr = RegRecursiveCopyKey(hkey1, szName, hkey2, szName);
                    }
                }
                if(hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS)) hr = S_OK;
            }

            ::RegCloseKey(hkey2);
        }

        ::RegCloseKey(hkey1);
    }

    if(hr == SpHrFromWin32(ERROR_NO_MORE_ITEMS)) hr = SPERR_NO_MORE_ITEMS;
    if(hr == SpHrFromWin32(ERROR_FILE_NOT_FOUND)) hr = SPERR_NOT_FOUND;
    return hr;
}

inline HRESULT RemoveTestAppLexicon()
{

	HRESULT							hr = S_OK;
	CComPtr<IEnumSpObjectTokens>	cpEnum;
	CComPtr<ISpObjectToken>			cpSpObjectToken;
  
	if( SUCCEEDED( hr ) )
	{
		// Get tts test engine voice 
		hr = SpEnumTokens(SPCAT_APPLEXICONS, L"TTSCompliance", NULL, &cpEnum);
    }

	if( hr == S_OK )
	{
		CAutoReleaseComPtr<ISpObjectToken> cpToken;
		hr = cpEnum->Next(1, &cpToken, NULL);
		while(hr == S_OK)
		{
			hr = cpToken->RemoveStorageFileName(CLSID_SpUnCompressedLexicon, L"DataFile", TRUE);
			if(( hr == S_OK ) || (hr == SPERR_NOT_FOUND))
			{
				hr = cpToken->Remove(NULL); // remove the token itself
			}
			if(SUCCEEDED(hr))
			{
				hr = cpEnum->Next(1, &cpToken, NULL);
			}
		}
	}

	return hr;
}


//*****************************************************************************/
//Similar to CreateStreamOnHGlobal Win32 API, but allows a stream to be created 
inline HRESULT SPCreateStreamOnHGlobal(
                    HGLOBAL hGlobal,            //Memory handle for the stream object
                    BOOL fDeleteOnRelease,      //Whether to free memory when the object is released
                    REFGUID rguidFormatId,      //Format ID for stream
                    const WAVEFORMATEX * pwfex, //WaveFormatEx for stream
                    ISpStream ** ppStream)      //Address of variable to receive ISpStream pointer
{
    HRESULT hr;
    IStream * pMemStream;
    *ppStream = NULL;
    hr = ::CreateStreamOnHGlobal(hGlobal, fDeleteOnRelease, &pMemStream);
    if (SUCCEEDED(hr))
    {
        hr = ::CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_ALL, __uuidof(*ppStream), (void **)ppStream);
        if (SUCCEEDED(hr))
        {
            hr = (*ppStream)->SetBaseStream(pMemStream, rguidFormatId, pwfex);
            if (FAILED(hr))
            {
                (*ppStream)->Release();
                *ppStream = NULL;
            }
        }
        pMemStream->Release();
    }
    return hr;
}


/************* Test function prototypes (TestProc's) *************/
// ISpTTSEngine Tests
TESTPROCAPI t_ISpTTSEngine_Speak            (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_ISpTTSEngine_Skip             (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_ISpTTSEngine_GetOutputFormat  (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_ISpTTSEngine_SetRate          (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_ISpTTSEngine_SetVolume        (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

// Eventing Tests
TESTPROCAPI t_CheckEventsSAPI               (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_CheckEventsNotRequire			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

// TTS Markup Tests
TESTPROCAPI t_XMLBookmark					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLSilence					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLSpell	    				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLPronounce	    			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLRate						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLVolume						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLPitch						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLNonSapiTagsTest	        (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLEmphTest			        (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLPartOfSpTest		        (UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_XMLContext					(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

// Real Time Rate/Vol Tests
TESTPROCAPI t_RealTimeRateChange			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_RealTimeVolumeChange			(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

//Multiple Instances test
TESTPROCAPI t_MultiInstanceTest				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

// Audio State Tests
TESTPROCAPI t_SpeakStop						(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_SpeakDestroy  				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

// Lexicon test
TESTPROCAPI t_UserLexiconTest				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);
TESTPROCAPI t_AppLexiconTest				(UINT, TPPARAM, LPFUNCTION_TABLE_ENTRY);

//the following are used in Multiple instance tests

TESTPROCAPI t_ISpTTSEngine_Skip_Test(UINT uMsg, 
								TPPARAM tpParam, 
								LPFUNCTION_TABLE_ENTRY lpFTE,
								ISpVoice *cpVoice,
								bool bCalledByMulti=false);
TESTPROCAPI t_ISpTTSEngine_Speak_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti=false);
TESTPROCAPI t_ISpTTSEngine_GetOutputFormat_Test(UINT uMsg, 
								TPPARAM tpParam, 
								LPFUNCTION_TABLE_ENTRY lpFTE,
								ISpVoice *cpVoice,
								bool bCalledByMulti=false);
TESTPROCAPI t_ISpTTSEngine_SetRate_Test(UINT uMsg, 
								TPPARAM tpParam, 
								LPFUNCTION_TABLE_ENTRY lpFTE,
								ISpVoice *cpVoice,
								bool bCalledByMulti=false) ;
TESTPROCAPI t_ISpTTSEngine_SetVolume_Test(UINT uMsg, 
								TPPARAM tpParam, 
								LPFUNCTION_TABLE_ENTRY lpFTE,
								ISpVoice *cpVoice,
								bool bCalledByMulti=false) ;
TESTPROCAPI t_RealTimeRateChange_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_RealTimeVolumeChange_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_SpeakStop_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_CheckEventsSAPI_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti) ;
TESTPROCAPI t_LexiconMulti_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLSAPIMarkup_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLContext_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLBookmark_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLSilence_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLPronounce_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLRate_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLVolume_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLPitch_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);
TESTPROCAPI t_XMLSpell_Test(UINT uMsg, 
								 TPPARAM tpParam, 
								 LPFUNCTION_TABLE_ENTRY lpFTE,
								 ISpVoice *cpVoice,
								 bool bCalledByMulti);

#endif //__TTSCOMP_H__