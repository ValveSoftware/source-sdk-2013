// SpAudioPlug.cpp : Implementation of CSimpleaudioApp and DLL registration.

#include "stdafx.h"
#include "simpleaudio.h"
#include "SpAudioPlug.h"

/////////////////////////////////////////////////////////////////////////////
//


template <class T>
void CBasicQueueByArray<T>::_Clear()
{
    m_ulQueueSize = 0;
    m_ulHeader = 0;
    m_ulTail = 0;
    m_ullTotalOut = 0;
    m_ullTotalIn = 0;
    m_ulSpaceNotifySize = 1;
    m_ulDataNotifySize = 1;
    if (m_pCoMemAlloc)
    {
        ::CoTaskMemFree(m_pCoMemAlloc);
        m_pCoMemAlloc = NULL;
    }
}




template <class T>
ULONG CBasicQueueByArray<T>::_SpaceSize()
{
     return (m_ulHeader >= m_ulTail) ? (m_ulQueueSize - (m_ulHeader -  m_ulTail) - 1) : (m_ulTail - m_ulHeader - 1);
}

template <class T>
ULONG CBasicQueueByArray<T>::_DataSize()
{
    return (m_ulHeader >= m_ulTail) ? (m_ulHeader -  m_ulTail) : (m_ulQueueSize - (m_ulTail -  m_ulHeader));
}


template <class T>
CBasicQueueByArray<T>::CBasicQueueByArray() :
m_pCoMemAlloc(NULL),
m_ulQueueSize(0),
m_ulHeader(0),
m_ulTail(0),
m_hSpaceAvailable(0),
m_hDataAvailable(0),
m_ullTotalOut(0),
m_ullTotalIn(0),
m_ulSpaceNotifySize(1),
m_ulDataNotifySize(1)
{
    InitializeCriticalSection(&m_CriticalSection); 
}

template <class T>
CBasicQueueByArray<T>::CBasicQueueByArray(ULONG ulQueueSize, HRESULT *phr) :
m_pCoMemAlloc(NULL),
m_ulQueueSize(0),
m_ulHeader(0),
m_ulTail(0),
m_hSpaceAvailable(0),
m_hDataAvailable(0),
m_ullTotalOut(0),
m_ullTotalIn(0),
m_ulSpaceNotifySize(1),
m_ulDataNotifySize(1)
{
    InitializeCriticalSection(&m_CriticalSection); 
    *phr = Init(ulQueueSize);
}



template <class T>
HRESULT CBasicQueueByArray<T>::Resize(ULONG ulNewQueueSize)
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    HRESULT hr = S_OK;
    if (ulNewQueueSize > m_ulQueueSize-1)
    {
        T* p = (T*)::CoTaskMemRealloc(m_pCoMemAlloc, sizeof(T)*(ulNewQueueSize+1));
        if (p == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_pCoMemAlloc = p;
            if (m_ulHeader >= m_ulTail)
            {
                //We don't need to do anything.
            }
            else
            {
                //data are rounded from end of the queue to the beginning of the queue, we need to move the data to the end of the queue
                memmove(m_pCoMemAlloc + ulNewQueueSize + 1 - (m_ulQueueSize - m_ulTail), m_pCoMemAlloc + m_ulTail, (m_ulQueueSize - m_ulTail)*sizeof(T));
                m_ulTail += ulNewQueueSize + 1 - m_ulQueueSize;
            }
            m_ulQueueSize = ulNewQueueSize + 1;
        }
    }
    else if (ulNewQueueSize < m_ulQueueSize-1)
    {
        if (m_ulQueueSize-1-ulNewQueueSize > _SpaceSize())
        {
            //This could cause loss of data
            hr = E_INVALIDARG;
        }
        else
        {
            T* p = (T*)::CoTaskMemAlloc(sizeof(T) * (ulNewQueueSize+1));
            if (!p)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                if (m_ulHeader > m_ulTail)
                {
                    //continous memory
                    memcpy(p, m_pCoMemAlloc + m_ulTail, (m_ulHeader-m_ulTail)*sizeof(T));
                }
                else if (m_ulHeader < m_ulTail)
                {
                    //copy the memory from m_ulTail to the end of the memory block
                    memcpy(p, m_pCoMemAlloc + m_ulTail, (m_ulQueueSize - m_ulTail)*sizeof(T));
                    if (m_ulHeader)
                    {
                        memcpy(p + (m_ulQueueSize - m_ulTail), m_pCoMemAlloc, m_ulHeader*sizeof(T));
                    }
                }
                m_ulHeader = _DataSize();
                m_ulTail = 0;

                ::CoTaskMemFree(m_pCoMemAlloc);
                m_pCoMemAlloc = p;
                m_ulQueueSize = ulNewQueueSize + 1;
            }
        }
    }

    return hr;
}


template <class T>
HRESULT CBasicQueueByArray<T>::Init(ULONG ulQueueSize, HANDLE hSpaceAvailable, HANDLE hDataAvailable, ULONG ulSpaceNotifySize, ULONG ulDataNotifySize)
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    HRESULT hr = S_OK;
    if (ulSpaceNotifySize + ulDataNotifySize >= ulQueueSize)
    {
        //This could cause deadlock between client thread and SAPI read/write thread
        return E_INVALIDARG;
    }
    _Clear();
    if (ulQueueSize)
    {
        //We allocated additional dummy space is to deal with the case that m_ulHeader catches up from behind to m_ulTail.
        //We shouldn't use the dummy space to store any value
        m_pCoMemAlloc = (T*)::CoTaskMemAlloc(sizeof(T)*(ulQueueSize + 1)); 
        if (m_pCoMemAlloc)
        {
            m_ulQueueSize = ulQueueSize + 1;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    m_hSpaceAvailable = hSpaceAvailable;
    m_hDataAvailable = hDataAvailable;

    m_ulSpaceNotifySize = ulSpaceNotifySize;
    m_ulDataNotifySize = ulDataNotifySize;

    return hr;
}



template <class T>
ULONG CBasicQueueByArray<T>::QueueSize()
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    return m_ulQueueSize ? m_ulQueueSize -1 : 0;
}


template <class T>
void CBasicQueueByArray<T>::InsertHead(T* pElements, ULONG ulCount, ULONG * pulReturnCount)
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    ULONG ulDataBeforeInsert= _DataSize(); //get the available data before we do the update

    ULONG ulEmpty = _SpaceSize();
    ULONG ulPushCount = (ulEmpty >= ulCount) ? ulCount : ulEmpty;

    if (ulPushCount <= (m_ulQueueSize - m_ulHeader))
    {
        memcpy(m_pCoMemAlloc + m_ulHeader, pElements, sizeof(T)*ulPushCount);
    }
    else
    {
        memcpy(m_pCoMemAlloc + m_ulHeader, pElements, sizeof(T)*(m_ulQueueSize - m_ulHeader));
        memcpy(m_pCoMemAlloc, pElements + m_ulQueueSize - m_ulHeader, sizeof(T)*(ulPushCount - m_ulQueueSize + m_ulHeader));
    }

    m_ulHeader = (m_ulHeader + ulPushCount) % m_ulQueueSize;
    if (pulReturnCount)
    {
        *pulReturnCount = ulPushCount;
    }

    m_ullTotalIn += ulPushCount;

    if (ulDataBeforeInsert < m_ulDataNotifySize && m_ulDataNotifySize <= _DataSize())
    {
        ::SetEvent(m_hDataAvailable);
    }
}



//When ulCount == 0, the caller wants to get the remaining 
template <class T>
void CBasicQueueByArray<T>::RemoveTail(T* pElements, ULONG ulCount, ULONG * pulReturnCount)
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    ULONG ulSpaceBeforeRemove = _SpaceSize(); //get the available space before we do the update

    ULONG ulOccupied = _DataSize();
    ULONG ulPopCount = (ulOccupied >= ulCount && ulCount) ? ulCount : ulOccupied;

    if (ulPopCount <= (m_ulQueueSize - m_ulTail))
    {
        memcpy(pElements, m_pCoMemAlloc + m_ulTail, sizeof(T)*ulPopCount);
    }
    else
    {
        memcpy(pElements, m_pCoMemAlloc + m_ulTail, sizeof(T)*(m_ulQueueSize - m_ulTail));
        memcpy(pElements + m_ulQueueSize - m_ulTail, m_pCoMemAlloc, sizeof(T)*(ulPopCount - m_ulQueueSize + m_ulTail));
    }

    m_ulTail = (m_ulTail + ulPopCount) % m_ulQueueSize;
    if (pulReturnCount)
    {
        *pulReturnCount = ulPopCount;
    }

    m_ullTotalOut += ulPopCount;

    if (ulSpaceBeforeRemove < m_ulSpaceNotifySize && m_ulSpaceNotifySize <= _SpaceSize())
    {
        ::SetEvent(m_hSpaceAvailable);
    }
}



template <class T>
void CBasicQueueByArray<T>::ResetPos()
{
    CRITICAL_SECTION_AUTO_LOCK csl(this);
    m_ulHeader = 0;
    m_ulTail = 0;
    m_ullTotalOut = 0;
    m_ullTotalIn = 0;
}


/****************************************************************************
* SpAudioPlug::Init *
*------------------------------*
*   Description:  
****************************************************************************/
HRESULT SpAudioPlug::Init(VARIANT_BOOL fWrite,  SpeechAudioFormatType FormatType)
{
    SPAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;
    if (fWrite == VARIANT_TRUE)
    {
        m_fWrite = TRUE;
    }
    else
    {
        m_fWrite = FALSE;
    }

    SPSTREAMFORMAT eFormat = (SPSTREAMFORMAT)FormatType;

    if (eFormat == SPSF_Text)
    {
        return E_INVALIDARG;
    }

    if (!m_autohAPIEvent)
    {
        m_autohAPIEvent = CreateEvent(NULL, TRUE, m_fWrite, NULL);
    }

    if (m_State != SPAS_CLOSED)
    {
        hr = SPERR_DEVICE_BUSY;
    }
    else
    {
        if (eFormat != SPSF_NoAssignedFormat && m_Format.ComputeFormatEnum() != eFormat)
        {
           hr = m_Format.AssignFormat(eFormat);
            if (SUCCEEDED(hr))
            {
                static const SPAUDIOBUFFERINFO BuffInfo = {50, 500, 0};
                hr = SetBufferInfo(&BuffInfo);
            }
        }

        m_fautohAPIEventSet = m_fWrite;
    }

    return hr;
}

/****************************************************************************
* SpAudioPlug::SetData *
*------------------------------*
*   Description:  
****************************************************************************/
HRESULT SpAudioPlug::SetData(VARIANT vData, long * pWritten)
{
    HRESULT hr = S_OK;

    //The method can only be called when the audio object is set to be input device.
    if (m_fWrite || m_State != SPAS_RUN)
    {
        return STG_E_ACCESSDENIED;
    }
    else
    {
        bool  fByRef = false;
        switch (vData.vt)
        {
        case (VT_ARRAY | VT_BYREF | VT_UI1):
            fByRef = true;
            break;
        case (VT_ARRAY | VT_UI1):
            break;
        default:
            return E_INVALIDARG;
        }

        BYTE *pArray = NULL;
        ULONG ulDataSize = 0;

        hr = SafeArrayAccessData( fByRef ? *vData.pparray : vData.parray,
            (void **)&pArray );
        if( SUCCEEDED( hr ) )
        {
            ulDataSize = fByRef ? 
                (*vData.pparray)->rgsabound[0].cElements : 
            vData.parray->rgsabound[0].cElements;
        }

        ULONG cbRemaining = ulDataSize;
        ULONG ulWrite = 0;
        BYTE * pHeader = pArray;

	    if (pWritten)
	    {
	        *pWritten = 0;
	    }

        while(cbRemaining)
        {
            m_Queue.InsertHead(pHeader, cbRemaining, &ulWrite);
            _ProcessEvent();

            cbRemaining -= ulWrite;
            pHeader += ulWrite;
            if (pWritten)
            {
                *pWritten += ulWrite;
            }

            if (ulWrite == 0)
            {
                DWORD dwReturn = ::WaitForSingleObject(m_hQueueHasSpaceEvent, INFINITE);
                if (dwReturn == WAIT_OBJECT_0)
                {
                    //It could be signalled by SetState thread, or Read thread
                    if (m_State == SPAS_CLOSED)
                    {
                        //Signalled by SetState thread
                        break;
                    }
                    else
                    {
                        //Signalled by Read thread
                    }
                }
            }
        }

        SafeArrayUnaccessData( fByRef ? *vData.pparray : vData.parray);

    }

    return hr;
}

/****************************************************************************
* SpAudioPlug::GetData *
*------------------------------*
*   Description:  
****************************************************************************/
HRESULT SpAudioPlug::GetData(VARIANT* pData)
{
    HRESULT hr = S_OK;

    //The method can only be called when the audio object is set to be output device.
    if (!m_fWrite)
    {
        return STG_E_ACCESSDENIED;
    }

    //We can lock the queue several times on the same thread without blocking the client thread, this is character of critical section
    //The reason we want to lock the queue explicitly is the possible inconsistent state between 
    m_Queue.Lock();
    ULONG ulDataAvailable = m_Queue.DataSize();
    if (ulDataAvailable)
    {
        BYTE *pArray = NULL;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, ulDataAvailable);
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                m_Queue.RemoveTail(pArray, ulDataAvailable, NULL);
                SafeArrayUnaccessData( psa );
                VariantClear( pData );
                pData->vt     = VT_ARRAY | VT_UI1;
                pData->parray = psa;
            }
            else
            {
                // Free our memory if we failed.
                hr = SafeArrayDestroy(psa);
                VariantClear( pData );    
            }

        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        VariantClear( pData );
        hr = S_FALSE;
    }


    _ProcessEvent();

    m_Queue.Unlock();

    return hr;
}



/****************************************************************************
* SpAudioPlug::SpAudioPlug *
*------------------------------*
*   Description:  
*       ctor
*********************************************************************/
SpAudioPlug::SpAudioPlug() : 
m_State(SPAS_CLOSED),
m_fWrite(TRUE),
m_SpEventSource(this),
m_cbEventBias(0),
m_ulBufferNotifySize(0),
m_hQueueHasDataEvent(NULL),
m_hQueueHasSpaceEvent(NULL),
m_autohAPIEvent(NULL)
{
    InitializeCriticalSection(&m_CriticalSection); 
}

/****************************************************************************
* SpAudioPlug::FinalConstruct *
*-----------------------------*
*   Description:  
*       Called by ATL when our object is constructed. 
************************************************************************/
HRESULT SpAudioPlug::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = m_Format.AssignFormat(SPSF_22kHz16BitMono);

    if (SUCCEEDED(hr))
    {
        m_hQueueHasDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    if (SUCCEEDED(hr))
    {
        m_hQueueHasSpaceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }



    static const SPAUDIOBUFFERINFO BuffInfo = {50, 500, 0};
    hr = SetBufferInfo(&BuffInfo);


    return hr;
}

/****************************************************************************
* SpAudioPlug::FinalRelease *
*-----------------------------*
*   Description:  
*       Called by ATL when our object is going away. 
*********************************************************************/
void SpAudioPlug::FinalRelease()
{
    if (m_hQueueHasDataEvent)
    {
        ::CloseHandle(m_hQueueHasDataEvent);
    }

    if (m_hQueueHasSpaceEvent)
    {
        ::CloseHandle(m_hQueueHasSpaceEvent);
    }

	if (m_autohAPIEvent)
	{
		CloseHandle(m_autohAPIEvent);
	}

    DeleteCriticalSection(&m_CriticalSection);


}



/****************************************************************************
* SpAudioPlug::AddEvents *
*-----------------------*
*   Description:  
*       ISpEventSink::AddEvents implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::AddEvents(const SPEVENT* pEventArray, ULONG ulCount)
{                                                                               
    HRESULT hr = S_OK;
   SPAUTO_OBJ_LOCK;
   
    if( SPIsBadReadPtr(pEventArray, sizeof(SPEVENT ) * ulCount))
    {                                                                           
        hr = E_INVALIDARG;                                                      
    }                                                                               
    else 
    {
        ULONGLONG ullDevicePosition = m_fWrite ? m_Queue.GetTotalOut() : m_Queue.GetTotalIn();
        hr = m_SpEventSource._AddEvents(pEventArray, ulCount);
        m_SpEventSource._CompleteEvents(ullDevicePosition + m_cbEventBias);
    }
    return hr;
}



/****************************************************************************
* SpAudioPlug::GetEventInterest *
*------------------------------*
*   Description:  
*       ISpEventSink::GetEventInterest implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetEventInterest(ULONGLONG * pullEventInterest)
{
    HRESULT hr = S_OK;
    SPAUTO_OBJ_LOCK;

    if (SP_IS_BAD_WRITE_PTR(pullEventInterest))
    {
        hr = E_POINTER;
    }
    else
    {
        *pullEventInterest = m_SpEventSource.m_ullEventInterest;
    }

    return hr;
}



/****************************************************************************
* SpAudioPlug::_ProcessEvent *
*---------------------*
*   Description:  
*
*   Return:
*********************************************************************/
void SpAudioPlug::_ProcessEvent()
{
    BOOL fSetEvent;
    if (m_fWrite)
    {
        if (m_Queue.DataSize() < m_ulBufferNotifySize)
        {
            fSetEvent = TRUE;
        }
        else
        {
            fSetEvent = FALSE;
        }
    }
    else
    {
        if (m_Queue.DataSize() >=  m_ulBufferNotifySize)
        {
            fSetEvent = TRUE;
        }
        else
        {
            fSetEvent = FALSE;
        }
    }
    if (fSetEvent != m_fautohAPIEventSet)
    {
        if (fSetEvent)
        {
            SetEvent(m_autohAPIEvent);
        }
        else
        {
            ResetEvent(m_autohAPIEvent);
        }
        m_fautohAPIEventSet = fSetEvent;
    }

    ULONGLONG ullDevicePosition = m_fWrite ? m_Queue.GetTotalOut() : m_Queue.GetTotalIn();
    m_SpEventSource._CompleteEvents(ullDevicePosition + m_cbEventBias);

}


/****************************************************************************
* SpAudioPlug::Read *
*---------------------*
*   Description:  
*       ISequentialStream::Read implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Read(void * pv, ULONG cb, ULONG *pcbRead)
{
    SPDBG_FUNC("SpAudioPlug::Read");
    HRESULT hr = S_OK;
    
    if (SPIsBadWritePtr(pv, cb) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbRead))
    {
        return E_POINTER;
    }

    if (m_fWrite)
    {
        return STG_E_ACCESSDENIED;
    }

    if (pcbRead)
    {
        *pcbRead = 0;
    }

    ULONG cbRemaining = cb;
    BYTE *pTail = (BYTE*)pv;
    ULONG  ulRead = 0;

    while(cbRemaining)
    {

        m_Queue.RemoveTail(pTail, cbRemaining, &ulRead);
        _ProcessEvent();



        if (ulRead)
        {
            cbRemaining -= ulRead;
            pTail += ulRead;

            if (pcbRead)
            {
                *pcbRead += ulRead;
            }
        }
        else
        {
            if (m_State == SPAS_RUN)
            {
                DWORD dwReturn = ::WaitForSingleObject(m_hQueueHasDataEvent, 1000);

                if (dwReturn == WAIT_OBJECT_0)
                {
                    //Signalled by SetData thread
                }
                else if (dwReturn == WAIT_TIMEOUT)
                {
                    hr = SPERR_AUDIO_BUFFER_UNDERFLOW;
                    break;
                }

            }
            else
            {
                break;
            }
        }

    }

    
    SPDBG_REPORT_ON_FAIL(hr);
    
    return hr;
}

/****************************************************************************
* SpAudioPlug::Write *
*----------------------*
*   Description:  
*       ISequentialStream::Write implementation.
*
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Write(const void * pv, ULONG cb, ULONG *pcb)
{
    SPDBG_FUNC("SpAudioPlug::Write");
    HRESULT hr = S_OK;
    
    if (!m_fWrite)
    {
        return STG_E_ACCESSDENIED;
    }
    else if (m_State != SPAS_RUN && m_State != SPAS_PAUSE)
    {
        return SPERR_AUDIO_STOPPED;
    }
    else
    {
        ULONG cbRemaining = cb;
        ULONG ulWrite = 0;
        BYTE * pHeader = (BYTE*)pv;
        while(cbRemaining)
        {
            m_Queue.InsertHead(pHeader, cbRemaining, &ulWrite);
            _ProcessEvent();


            cbRemaining -= ulWrite;
            pHeader += ulWrite;
            if (pcb)
            {
                *pcb += ulWrite;
            }

            if (ulWrite == 0)
            {
                DWORD dwReturn = ::WaitForSingleObject(m_hQueueHasSpaceEvent, INFINITE);
                if (dwReturn == WAIT_OBJECT_0)
                {
                    //It could be signalled by SetState thread, or GetData thread
                    if (m_State == SPAS_CLOSED)
                    {
                        //Signalled by SetState thread
                        break;
                    }
                    else
                    {
                        //Signalled by GetData thread
                    }
                }
            }
        }

    }
    
        
    return hr;
}

/****************************************************************************
* SpAudioPlug::Seek *
*---------------------*
*   Description:  
*       IStream::Seek implementation. It can only be used to retrieve the current seek position
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
{
    SPDBG_FUNC("SpAudioPlug::Seek");
    SPAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    if (dwOrigin != STREAM_SEEK_CUR || dlibMove.QuadPart)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (SPIsBadWritePtr(plibNewPosition, sizeof(*plibNewPosition)))
        {
            hr = E_POINTER;
        }
        else
        {
            ULONGLONG ullSeekPosition = m_fWrite ? m_Queue.GetTotalIn() : m_Queue.GetTotalOut();
            plibNewPosition->QuadPart = ullSeekPosition;
        }
    }

    return hr;
}

/****************************************************************************
* SpAudioPlug::SetSize *
*------------------------*
*   Description:  
*       IStream::SetSize implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetSize(ULARGE_INTEGER libNewSize)
{
    SPDBG_FUNC("SpAudioPlug::SetSize");
    SPAUTO_OBJ_LOCK;

    return m_Queue.Resize(libNewSize.LowPart);
}

/****************************************************************************
* SpAudioPlug::CopyTo *
*-----------------------*
*   Description:  
*       IStream::CopyTo implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    SPDBG_FUNC("SpAudioPlug::CopyTo");
    SPAUTO_OBJ_LOCK;
    if (m_fWrite)
    {
        return STG_E_ACCESSDENIED;
    }
    else
    {
        return E_NOTIMPL;
    }
}

/****************************************************************************
* SpAudioPlug::Commit *
*-----------------------*
*   Description:  
*       IStream::Commit implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Commit(DWORD grfCommitFlags)
{
    SPDBG_FUNC("SpAudioPlug::Commit");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    if (m_fWrite && m_State == SPAS_RUN)
    {
        _ProcessEvent();              // Call this to clear the event if we're writing
    }

            
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::Revert *
*-----------------------*
*   Description:  
*       IStream::Revert implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Revert(void)
{
    SPDBG_FUNC("SpAudioPlug::Revert");
    
    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL;
}

/****************************************************************************
* SpAudioPlug::LockRegion *
*---------------------------*
*   Description:  
*       IStream::LockRegion implementation. Delegate to the actual audio 
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("SpAudioPlug::LockRegion");
    
    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL;
}

/****************************************************************************
* SpAudioPlug::UnlockRegion *
*-----------------------------*
*   Description:  
*       IStream::UnlockRegion implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    SPDBG_FUNC("SpAudioPlug::UnlockRegion");
    
    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL;
}

/****************************************************************************
* SpAudioPlug::Stat *
*---------------------*
*   Description:  
*       IStream::Stat implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    SPDBG_FUNC("SpAudioPlug::Stat");
    
    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL;
}

/****************************************************************************
* SpAudioPlug::Clone *
*----------------------*
*   Description:  
*       IStream::Clone implementation. Delegate to the actual audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::Clone(IStream **ppstm)
{
    SPDBG_FUNC("SpAudioPlug::Clone");
    
    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL;
}

/****************************************************************************
* SpAudioPlug::GetFormat *
*--------------------------*
*   Description:  
*       ISpStreamFormat::GetFormat implementation.
*       GetFormat is called for input device
*
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("SpAudioPlug::GetFormat");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (SP_IS_BAD_WRITE_PTR(pguidFormatId) || 
             SP_IS_BAD_WRITE_PTR(ppCoMemWaveFormatEx))
    {
        return E_POINTER;
    }

    //m_guidFormatId and m_pCoMemWaveFormatEx need to be initialized by ISpAudioPlug::Init
    if (m_Format.FormatId() == GUID_NULL || m_Format.WaveFormatExPtr() == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else
    {
        hr = m_Format.CopyTo(pguidFormatId, ppCoMemWaveFormatEx);
    }
            
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::SetState *
*-------------------------*
*   Description:  
*       ISpAudio::SetState implementation.
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved )
{
    SPDBG_FUNC("SpAudioPlug::SetState");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (m_State != NewState)
    {
        m_State = NewState;

        if (NewState != SPAS_RUN)
        {
            if (NewState == SPAS_STOP || NewState == SPAS_CLOSED)
            {
                m_Queue.ResetPos();
            }
            SetEvent(m_hQueueHasDataEvent); //Signal the read/getdata thread it exit
            SetEvent(m_hQueueHasSpaceEvent); //Signal the write/setdata thread it exit
        }
    }


    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::SetFormat *
*--------------------------*
*   Description:  
*       ISpAudio::SetFormat implementation. We don't allow setting the format
*       to anything other than the input format.
*       We'll let the format converter do the right thing for us for
*       the SR engine.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx)
{
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (!m_Format.IsEqual(rguidFmtId, pWaveFormatEx))
    {
        hr = SPERR_UNSUPPORTED_FORMAT;
    }

    return hr;
}

/****************************************************************************
* SpAudioPlug::GetStatus *
*--------------------------*
*   Description:  
*       ISpAudio::GetStatus implementation.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetStatus(SPAUDIOSTATUS *pStatus)
{
    SPDBG_FUNC("SpAudioPlug::GetStatus");
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(pStatus))
    {
        hr = E_POINTER;
    }
    else
    {
        //Because there are more than one operation on m_Queue, we need to lock the queue explicitly
        m_Queue.Lock();

        if (m_State == SPAS_RUN)
        {
            long lDataInQueue = (long)(m_Queue.DataSize());
            pStatus->cbFreeBuffSpace = ((long)m_Queue.QueueSize()) - lDataInQueue;

            if (m_fWrite)
            {
                pStatus->cbNonBlockingIO = pStatus->cbFreeBuffSpace;
            }
            else
            {
                pStatus->cbNonBlockingIO = lDataInQueue;
            }
        }
        else
        {
            pStatus->cbFreeBuffSpace = 0;
            pStatus->cbNonBlockingIO = 0;
        }
        pStatus->CurSeekPos = m_fWrite ? m_Queue.GetTotalIn() : m_Queue.GetTotalOut();
        pStatus->State = m_State;
        pStatus->CurDevicePos = m_fWrite ? m_Queue.GetTotalOut() : m_Queue.GetTotalIn();;
        pStatus->dwReserved1 = 0;
        pStatus->dwReserved2 = 0;

        m_Queue.Unlock();
    }

            
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::SetBufferInfo *
*------------------------------*
*   Description:  
*       ISpAudio::SetBufferInfo implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo)
{
    SPDBG_FUNC("SpAudioPlug::SetBufferInfo");
    SPAUTO_OBJ_LOCK;

    HRESULT hr = S_OK;

    
    if (m_State != SPAS_CLOSED)
    {
        hr = SPERR_DEVICE_BUSY;
    }
    else
    {
        m_BufferInfo = *pInfo;
        if (m_fWrite)
        {
            m_Queue.Init(m_Format.WaveFormatExPtr()->nAvgBytesPerSec * pInfo->ulMsBufferSize / 1000,
                m_hQueueHasSpaceEvent,
                m_hQueueHasDataEvent,
                m_Format.WaveFormatExPtr()->nAvgBytesPerSec *pInfo->ulMsMinNotification / 1000,
                1);
        }
        else
        {
            m_Queue.Init(m_Format.WaveFormatExPtr()->nAvgBytesPerSec * pInfo->ulMsBufferSize / 1000,
                m_hQueueHasSpaceEvent,
                m_hQueueHasDataEvent,
                1,
                1);
        }

        m_cbEventBias = (m_Format.WaveFormatExPtr()->nAvgBytesPerSec * pInfo->ulMsEventBias) / 1000;
    }

            
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::GetBufferInfo *
*------------------------------*
*   Description:  
*       ISpAudio::GetBufferInfo implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetBufferInfo(SPAUDIOBUFFERINFO * pInfo)
{
    SPDBG_FUNC("SpAudioPlug::GetBufferInfo");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;

    *pInfo = m_BufferInfo;
            
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::GetDefaultFormat *
*---------------------------------*
*   Description:  
*       ISpAudio::GetDefaultFormat implementation.
*       GetDefaultFormat is called for output device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    SPDBG_FUNC("SpAudioPlug::GetDefaultFormat");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    if (SP_IS_BAD_WRITE_PTR(pFormatId) || 
             SP_IS_BAD_WRITE_PTR(ppCoMemWaveFormatEx))
    {
        return E_POINTER;
    }

    //m_guidFormatId and m_pCoMemWaveFormatEx need to be initialized by ISpAudioPlug::Init
    if (m_Format.FormatId() == GUID_NULL || m_Format.WaveFormatExPtr() == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else
    {
        hr = m_Format.CopyTo(pFormatId, ppCoMemWaveFormatEx);
    }
    
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::EventHandle *
*----------------------------*
*   Description:  
*       ISpAudio::EventHandle implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP_(HANDLE) SpAudioPlug::EventHandle()
{
    SPDBG_FUNC("SpAudioPlug::EventHandle");
    
    SPAUTO_OBJ_LOCK;
    
    return m_autohAPIEvent;
}

/****************************************************************************
* SpAudioPlug::GetVolumeLevel *
*-------------------------------*
*   Description:  
*       ISpAudio:GetVolumeLevel implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetVolumeLevel(ULONG *pLevel)
{
    SPDBG_FUNC("SpAudioPlug::GetVolumeLevel");
    
    SPAUTO_OBJ_LOCK;
        
    return E_NOTIMPL ;
}

/****************************************************************************
* SpAudioPlug::SetVolumeLevel *
*-------------------------------*
*   Description:  
*       ISpAudio::SetVolumeLevel implementation. Delegate to the actual audio
*       device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetVolumeLevel(ULONG Level)
{
    SPDBG_FUNC("SpAudioPlug::SetVolumeLevel");

    SPAUTO_OBJ_LOCK;
            
    return E_NOTIMPL ;
}

/****************************************************************************
* SpAudioPlug::GetBufferNotifySize *
*------------------------------------*
*   Description:  
*       ISpAudio::GetBufferNotifySize implementation. Delegate to the actual
*       audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::GetBufferNotifySize(ULONG *pcbSize)
{
    SPDBG_FUNC("SpAudioPlug::GetBufferNotifySize");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    *pcbSize = m_ulBufferNotifySize;
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* SpAudioPlug::SetBufferNotifySize *
*------------------------------------*
*   Description:  
*       ISpAudio::SetBufferNotifySize implementation. Delegate to the actual
*       audio device.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
*********************************************************************/
STDMETHODIMP SpAudioPlug::SetBufferNotifySize(ULONG cbSize)
{
    SPDBG_FUNC("SpAudioPlug::SetBufferNotifySize");
    HRESULT hr = S_OK;
    
    SPAUTO_OBJ_LOCK;
    
    m_ulBufferNotifySize = cbSize;
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}