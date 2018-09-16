/*******************************************************************************
* SPEventQ.h *
*------------*
*   Description:
*       This is the header file for the SAPI5 event queue implementation.
*-------------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*******************************************************************************/
#ifndef SPEventQ_h
#define SPEventQ_h

#ifndef SPHelper_h
#include <SPHelper.h>
#endif

#ifndef SPCollec_h
#include <SPCollec.h>
#endif

//=== Inline helpers for copying and deleting events ============================


//=== Class definition ==========================================================

class CSpEventNode : public CSpEvent
{
public:
    CSpEventNode    * m_pNext;
    static LONG Compare(const CSpEventNode * p1, const CSpEventNode *p2)
    {
        // Assumes offsets DO or DO NOT reset when stream number changes
        if (p1->ulStreamNum < p2->ulStreamNum)
        {
            return -1;
        }
        else if (p1->ulStreamNum > p2->ulStreamNum)
        {
            return 1;
        }
        else if (p1->ullAudioStreamOffset < p2->ullAudioStreamOffset)
        {
            return -1;
        }
        else if (p1->ullAudioStreamOffset > p2->ullAudioStreamOffset)
        {
            return 1;
        }
        return 0;
    }
};


typedef CSpBasicQueue<CSpEventNode, TRUE, TRUE> CSpEventList;

#define DECLARE_SPNOTIFYSOURCE_METHODS(T) \
STDMETHODIMP SetNotifySink(ISpNotifySink * pNotifySink) \
{ return T._SetNotifySink(pNotifySink); } \
STDMETHODIMP SetNotifyWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) \
{ return T._SetNotifyWindowMessage(hWnd, Msg, wParam, lParam); } \
STDMETHODIMP SetNotifyCallbackFunction(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam) \
{ return T._SetNotifyCallbackFunction(pfnCallback, wParam, lParam); } \
STDMETHODIMP SetNotifyCallbackInterface(ISpNotifyCallback * pSpCallback, WPARAM wParam, LPARAM lParam) \
{ return T._SetNotifyCallbackInterface(pSpCallback, wParam, lParam); } \
STDMETHODIMP SetNotifyWin32Event() \
{ return T._SetNotifyWin32Event(); } \
STDMETHODIMP WaitForNotifyEvent(DWORD dwMilliseconds) \
{ return T._WaitForNotifyEvent(dwMilliseconds); } \
STDMETHODIMP_(HANDLE) GetNotifyEventHandle() \
{ return T._GetNotifyEventHandle(); } 

#define DECLARE_SPEVENTSOURCE_METHODS(T) \
DECLARE_SPNOTIFYSOURCE_METHODS(T) \
STDMETHODIMP SetInterest(ULONGLONG ullEventInterest, ULONGLONG ullQueuedInterest) \
{ return T._SetInterest(ullEventInterest, ullQueuedInterest); } \
STDMETHODIMP GetEvents(ULONG ulCount, SPEVENT* pEventArray, ULONG * pulFetched) \
{ return T._GetEvents(ulCount, pEventArray, pulFetched); } \
STDMETHODIMP GetInfo(SPEVENTSOURCEINFO *pInfo) \
{ return T._GetInfo(pInfo); }



class CSpEventSource 
{
  public:
    CSpEventSource(CComObjectRootEx<CComMultiThreadModel> * pParent) :
        m_pParent(pParent)
    {
        m_ullEventInterest = 0; m_ullQueuedInterest = 0;
        m_ulStreamNum = 0;
    }
    HRESULT _SetNotifySink(ISpNotifySink * pNotifySink);
    HRESULT _SetNotifyWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    HRESULT _SetNotifyCallbackFunction(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam);
    HRESULT _SetNotifyCallbackInterface(ISpNotifyCallback * pSpCallback, WPARAM wParam, LPARAM lParam);
    HRESULT _SetNotifyWin32Event();
    HRESULT _WaitForNotifyEvent(DWORD dwMilliseconds);
    HANDLE  _GetNotifyEventHandle();

    HRESULT _SetInterest(ULONGLONG ullEventInterest , ULONGLONG ullQueuedInterest);
    HRESULT _GetEvents( ULONG ulCount, SPEVENT* pEventArray, ULONG * pulFetched );
    HRESULT _GetInfo(SPEVENTSOURCEINFO *pInfo );

    /*--- Non interface methods ---*/
    HRESULT _CompleteEvents( ULONGLONG ullPos = 0xFFFFFFFFFFFFFFFF );
    inline void _MoveAllToFreeList(CSpEventList * pList);
    inline void _RemoveAllEvents();
    inline HRESULT _AddEvent(const SPEVENT & Event);
    inline HRESULT _AddEvents(const SPEVENT* pEventArray, ULONG ulCount);
    inline HRESULT _DeserializeAndAddEvent(const BYTE * pBuffer, ULONG * pcbUsed);
    inline HRESULT _GetStreamNumber(const ULONGLONG ullAudioOffset, ULONG *pulStreamNum);
    //=== Data members ==============================
  public:
    ULONGLONG                   m_ullEventInterest;
    ULONGLONG                   m_ullQueuedInterest;
    ULONG                       m_ulStreamNum;
    CSpEventList                m_PendingList;
    CSpEventList                m_CompletedList;
    CSpEventList                m_FreeList;
    CComPtr<ISpNotifySink>      m_cpNotifySink;
    CComPtr<ISpNotifyTranslator> m_cpEventTranslator;   // If non-NULL then Win32 events being used
    CComObjectRootEx<CComMultiThreadModel> * m_pParent;
    CComAutoCriticalSection     m_NotifyObjChangeCrit;  // Critical section used to make sure that
                                                        // the notify object (m_cpNotifySink) not changed
                                                        // while waiting on it.
                                                       
};


//
//=== Inlines =========================================================
//

//
//  WARNING:  If this logic changes, you will need to change the logic in SetNotifyWin32Event also.
//
inline HRESULT CSpEventSource::_SetNotifySink(ISpNotifySink * pNotifySink)
{
    if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pNotifySink))
    {
        return E_INVALIDARG;
    }
    else
    {
        m_pParent->Lock();
        m_NotifyObjChangeCrit.Lock();
        m_cpEventTranslator.Release();
        m_cpNotifySink = pNotifySink;
        if (m_cpNotifySink && m_CompletedList.GetHead())
        {
            m_cpNotifySink->Notify();
        }
        m_NotifyObjChangeCrit.Unlock();
        m_pParent->Unlock();
        return S_OK;
    }
}

/****************************************************************************
* CSpEventSource::_SetNotifyWindowMessage *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CSpEventSource::_SetNotifyWindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CSpEventSource::_SetNotifyWindowMessage");
    HRESULT hr = S_OK;
    CComPtr<ISpNotifyTranslator> cpTranslator;
    hr = cpTranslator.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpTranslator->InitWindowMessage(hWnd, Msg, wParam, lParam);
    }
    if (SUCCEEDED(hr))
    {
        hr = _SetNotifySink(cpTranslator);
    }
    return hr;
}
/****************************************************************************
* CSpEventSource::_SetNotifyCallbackFunction *
*--------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CSpEventSource::_SetNotifyCallbackFunction(SPNOTIFYCALLBACK * pfnCallback, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CSpEventSource::_SetNotifyCallbackFunction");
    HRESULT hr = S_OK;
    CComPtr<ISpNotifyTranslator> cpTranslator;
    hr = cpTranslator.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpTranslator->InitCallback(pfnCallback, wParam, lParam);
    }
    if (SUCCEEDED(hr))
    {
        hr = _SetNotifySink(cpTranslator);
    }
    return hr;
}
/****************************************************************************
* CSpEventSource::_SetNotifyCallbackInterface *
*---------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CSpEventSource::_SetNotifyCallbackInterface(ISpNotifyCallback * pSpCallback, WPARAM wParam, LPARAM lParam)
{
    SPDBG_FUNC("CSpEventSource::_SetNotifyCallbackInterface");
    HRESULT hr = S_OK;
    CComPtr<ISpNotifyTranslator> cpTranslator;
    hr = cpTranslator.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpTranslator->InitSpNotifyCallback(pSpCallback, wParam, lParam);
    }
    if (SUCCEEDED(hr))
    {
        hr = _SetNotifySink(cpTranslator);
    }
    return hr;
}
/****************************************************************************
* CSpEventSource::_SetNotifyWin32Event *
*--------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CSpEventSource::_SetNotifyWin32Event(void)
{
    SPDBG_FUNC("CSpEventSource::_SetNotifyWin32Event");
    HRESULT hr = S_OK;
    CComPtr<ISpNotifyTranslator> cpTranslator;
    hr = cpTranslator.CoCreateInstance(CLSID_SpNotifyTranslator);
    if (SUCCEEDED(hr))
    {
        hr = cpTranslator->InitWin32Event(NULL, TRUE);
    }
    if (SUCCEEDED(hr))
    {
        //
        //  In this case we do NOT call _SetNotify sink since we want to set the cpEventTranslator
        //
        m_pParent->Lock();
        m_NotifyObjChangeCrit.Lock();
        m_cpEventTranslator = cpTranslator;
        m_cpNotifySink = cpTranslator;
        if (m_cpNotifySink && m_CompletedList.GetHead())
        {
            m_cpNotifySink->Notify();
        }
        m_NotifyObjChangeCrit.Unlock();
        m_pParent->Unlock();
    }
    return hr;
}
/****************************************************************************
* CSpEventSource::_WaitForNotifyEvent *
*-------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HRESULT CSpEventSource::_WaitForNotifyEvent(DWORD dwMilliseconds)
{
    SPDBG_FUNC("CSpEventSource::_WaitForNotifyEvent");
    HRESULT hr = S_OK;
    m_NotifyObjChangeCrit.Lock();
    if (m_cpEventTranslator)
    {
        hr = m_cpEventTranslator->Wait(dwMilliseconds);
    }
    else
    {
        if (m_cpNotifySink)
        {
            hr = SPERR_ALREADY_INITIALIZED;
        }
        else
        {
            hr = _SetNotifyWin32Event();
            if (SUCCEEDED(hr))
            {
                hr = m_cpEventTranslator->Wait(dwMilliseconds);
            }
        }
    }
    m_NotifyObjChangeCrit.Unlock();
    return hr;
}
/****************************************************************************
* CSpEventSource::_GetNotifyEventHandle *
*---------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

inline HANDLE CSpEventSource::_GetNotifyEventHandle()
{
    HANDLE h = NULL;
    SPDBG_FUNC("CSpEventSource::_GetNotifyEventHandle");
    m_NotifyObjChangeCrit.Lock();
    if (!m_cpNotifySink)
    {
        _SetNotifyWin32Event();
    }
    if (m_cpEventTranslator)
    {
        h = m_cpEventTranslator->GetEventHandle();
    }
    m_NotifyObjChangeCrit.Unlock();
    return h;
}


inline HRESULT CSpEventSource::_SetInterest( ULONGLONG ullEventInterest, ULONGLONG ullQueuedInterest )
{
    HRESULT hr = S_OK;
    m_pParent->Lock();

    if(ullEventInterest && SPFEI_FLAGCHECK != (ullEventInterest & SPFEI_FLAGCHECK))
    {
        hr = E_INVALIDARG;
    }
    else if(ullQueuedInterest && SPFEI_FLAGCHECK != (ullQueuedInterest & SPFEI_FLAGCHECK))
    {
        hr = E_INVALIDARG;
    }
    else if ((ullQueuedInterest | ullEventInterest) != ullEventInterest)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_ullEventInterest = ullEventInterest;
        m_ullQueuedInterest = ullQueuedInterest;
    }
    m_pParent->Unlock();
    return hr;
}


//
//  Same as AddEvents except:  No param validation, and caller must take the critical section
//  prior to calling.
//
inline HRESULT CSpEventSource::_AddEvents( const SPEVENT* pEventArray, ULONG ulCount )
{
    HRESULT hr = S_OK;
    for( ULONG i = 0; i < ulCount && SUCCEEDED(hr = _AddEvent(pEventArray[i])); ++i ) {}
    return hr;
}

inline HRESULT CSpEventSource::_AddEvent(const SPEVENT & Event)
{
    SPDBG_ASSERT(Event.eEventId < 64);
    SPDBG_ASSERT(Event.elParamType == SPET_LPARAM_IS_UNDEFINED ||
                 Event.elParamType == SPET_LPARAM_IS_TOKEN ||
                 Event.elParamType == SPET_LPARAM_IS_OBJECT ||
                 Event.elParamType == SPET_LPARAM_IS_POINTER ||
                 Event.elParamType == SPET_LPARAM_IS_STRING);
#ifdef _DEBUG
    if (Event.eEventId == SPEI_VOICE_CHANGE)
    {
        SPDBG_ASSERT(Event.elParamType == SPET_LPARAM_IS_TOKEN);
    }
    else if (Event.eEventId == SPEI_RECOGNITION || Event.eEventId == SPEI_FALSE_RECOGNITION || Event.eEventId == SPEI_HYPOTHESIS)
    {
        SPDBG_ASSERT(Event.elParamType == SPET_LPARAM_IS_OBJECT);
    }
    else if (Event.eEventId ==SPEI_REQUEST_UI || Event.eEventId == SPEI_TTS_BOOKMARK)
    {
        SPDBG_ASSERT(Event.elParamType == SPET_LPARAM_IS_STRING);
    }
#endif

    if ( (1i64 << Event.eEventId) & m_ullEventInterest )
    {
        CSpEventNode *pNode = m_FreeList.RemoveHead();
        if (pNode == NULL)
        {
            pNode = new CSpEventNode();
            if (pNode == NULL)
            {
                return E_OUTOFMEMORY;
            }
        }
        pNode->CopyFrom(&Event);
        m_PendingList.InsertSorted(pNode);
    }
    return S_OK;
}

inline HRESULT CSpEventSource::
    _DeserializeAndAddEvent(const BYTE *pBuffer, ULONG * pcbUsed)
{
    HRESULT hr = S_OK;
    const SPEVENT * pSrcEvent = (const SPEVENT *)pBuffer;
    SPDBG_ASSERT(pSrcEvent->eEventId < 64);
    if ( (1i64 << pSrcEvent->eEventId) & m_ullEventInterest )
    {
        CSpEventNode *pNode = m_FreeList.RemoveHead();
        if (pNode == NULL)
        {
            pNode = new CSpEventNode();
            if (pNode == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = pNode->Deserialize(((const SPSERIALIZEDEVENT64 *)(pBuffer)), pcbUsed);
            if (SUCCEEDED(hr))
            {
                m_PendingList.InsertSorted(pNode);
            }
            else
            {
                m_FreeList.InsertHead(pNode);
            }
        }
    }
    else
    {
// WCE compiler does not work propertly with template
#ifndef _WIN32_WCE
        *pcbUsed = SpEventSerializeSize<SPSERIALIZEDEVENT64>(pSrcEvent);
#else
        *pcbUsed = SpEventSerializeSize(pSrcEvent, sizeof(SPSERIALIZEDEVENT64));
#endif
    }
    return hr;
}

inline HRESULT CSpEventSource::_GetEvents( ULONG ulCount, SPEVENT* pEventArray, ULONG *pulFetched )
{
    HRESULT hr = S_OK;
    m_pParent->Lock();
    if( SPIsBadWritePtr( pEventArray, sizeof( SPEVENT ) * ulCount ) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pulFetched) )
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        ULONG ulCopied = 0;
        ULONG ulRemaining = ulCount;
        CSpEventNode * pCur = m_CompletedList.m_pHead;
        CSpEventNode * pLastCopied = NULL;
        while (ulRemaining && pCur)
        {
            pCur->Detach(pEventArray + ulCopied);
            pLastCopied = pCur;
            ulCopied++;
            pCur = pCur->m_pNext;
            ulRemaining--;
        }
        if (ulCopied)
        {
            if (m_FreeList.m_pHead == NULL)
            {
                m_FreeList.m_pTail = pLastCopied;
            }
            pLastCopied->m_pNext = m_FreeList.m_pHead;
            m_FreeList.m_pHead = m_CompletedList.m_pHead;
            m_CompletedList.m_pHead = pCur;
            m_CompletedList.m_cElements -= ulCopied;
            m_FreeList.m_cElements += ulCopied;
        }
        if (ulCopied < ulCount)
        {
            hr = S_FALSE;
        }
        if (pulFetched) 
        {
            *pulFetched = ulCopied;
        }
    }
    m_pParent->Unlock();
    return hr;
}


inline HRESULT CSpEventSource::_GetInfo( SPEVENTSOURCEINFO * pInfo )
{
    HRESULT hr = S_OK;
    m_pParent->Lock();    
    if( SP_IS_BAD_WRITE_PTR( pInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        pInfo->ulCount = m_CompletedList.GetCount();
        pInfo->ullEventInterest = m_ullEventInterest;
        pInfo->ullQueuedInterest= m_ullQueuedInterest;
    }
    m_pParent->Unlock();
    return hr;
}



//
//  The caller must call this function with the critical section owned
//
inline HRESULT CSpEventSource::_CompleteEvents( ULONGLONG ullPos )
{
    HRESULT hr = S_OK;

    if (m_PendingList.m_pHead && m_PendingList.m_pHead->ullAudioStreamOffset <= ullPos)
    {
        BOOL bNotify = FALSE;
        while (m_PendingList.m_pHead &&
               m_PendingList.m_pHead->ullAudioStreamOffset <= ullPos)
        {
            CSpEventNode *pNode = m_PendingList.RemoveHead();
            if(pNode->ulStreamNum != m_ulStreamNum)
            {
                m_ulStreamNum = pNode->ulStreamNum;
            }
            if ( (1i64 << pNode->eEventId) & m_ullEventInterest )
            {
                bNotify = TRUE;
                //
                //  NOTE:  If we're forwarding events to an event sink then we'll only
                //  pay attention to the Interest flags.  If we're going to notify, then
                //  we'll only queue completed events that the user has explicitly asked
                //  us to store as completed events.
                //
                if ( (1i64 << pNode->eEventId) & m_ullQueuedInterest )
                {
                    m_CompletedList.InsertSorted(pNode);
                }
                else
                {
                    pNode->Clear();
                    m_FreeList.InsertHead(pNode);
                }
            }
            else
            {
                pNode->Clear();
                m_FreeList.InsertHead(pNode);
            }
        }    
        if (bNotify && m_cpNotifySink)
        {
            hr = m_cpNotifySink->Notify();
        }
    }
    return hr;
};


inline void CSpEventSource::_MoveAllToFreeList(CSpEventList * pList)
{
    CSpEventNode * pNode;
    while ((pNode = pList->RemoveHead()) != NULL)
    {
        pNode->Clear();
        m_FreeList.InsertHead(pNode);
    }
}
inline void CSpEventSource::_RemoveAllEvents( )
{
    m_pParent->Lock();

    _MoveAllToFreeList(&m_CompletedList);
    _MoveAllToFreeList(&m_PendingList);
    m_pParent->Unlock();
}

inline HRESULT CSpEventSource::_GetStreamNumber(const ULONGLONG ullAudioOffset, ULONG *pulStreamNum)
{
    CSpEventNode *pNode = m_PendingList.m_pHead;
    *pulStreamNum = m_ulStreamNum;
    for(;pNode && pNode->ullAudioStreamOffset <= ullAudioOffset; pNode = pNode->m_pNext)
    {
        *pulStreamNum = pNode->ulStreamNum;
    }
    return S_OK;
}



#endif //--- This must be the last line in this file
