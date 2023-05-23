// SpAudioPlug.h: Definition of the SpAudioPlug class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SPAUDIOPLUG_H__AD57CF29_3B4A_40A9_BB01_9BE69043E1DF__INCLUDED_)
#define AFX_SPAUDIOPLUG_H__AD57CF29_3B4A_40A9_BB01_9BE69043E1DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols


//The queue object itself is thread safe. Different threads could call the different methods on the same object.
//The queue object uses contiguous memory block for queue operation.

template <class T>
class CCriticalSectionLock
{
private:
T*  m_pObject;

public:
    CCriticalSectionLock(T* pObject)
    {
        m_pObject = pObject;
        m_pObject->Lock();
    }
    ~CCriticalSectionLock()
    {
        m_pObject->Unlock();
    }
};


template <class T>
class CBasicQueueByArray
{
    private:
    typedef CCriticalSectionLock< CBasicQueueByArray<T> > CRITICAL_SECTION_AUTO_LOCK;

    T* m_pCoMemAlloc;
    ULONG m_ulQueueSize; //In terms of the number of element of T in the queue, not the total bytes
    ULONG m_ulHeader; //In terms of the number of element offset from the beginning of memory, range from 0 to m_ulQueueSize - 1
    ULONG m_ulTail; //In terms of the number of element offset from the beginning of memory, range from 0 to m_ulQueueSize - 1
    ULONGLONG m_ullTotalOut; //the total bytes read out of the queue by RemoveTail
    ULONGLONG m_ullTotalIn; //the total bytes put into the queue by InsertHead

    CRITICAL_SECTION m_CriticalSection;
    HANDLE m_hSpaceAvailable; //signalled when the queue has space
    HANDLE m_hDataAvailable; //signalled when the the queue has m_ulNotifySize or more data
    ULONG m_ulSpaceNotifySize; //when the available space is equal or more than this amount, we need to signal m_hSpaceAvailable
    ULONG m_ulDataNotifySize; //when the available data is equal or more than this amount, we need to signal m_hDataAvailable

    void _Clear();

    ULONG _SpaceSize();
    ULONG _DataSize();

    
    public:
        CBasicQueueByArray();
        CBasicQueueByArray(ULONG ulQueueSize, HRESULT *phr);
        ~CBasicQueueByArray()
        {
            DeleteCriticalSection(&m_CriticalSection);
            if (m_pCoMemAlloc)
            {
                ::CoTaskMemFree(m_pCoMemAlloc);
            }
        }

        void Lock()
        {
            EnterCriticalSection(&m_CriticalSection);
        }

        void Unlock()
        {
            LeaveCriticalSection(&m_CriticalSection);
        }


        HRESULT Resize(ULONG ulNewQueueSize);

        HRESULT Init
            (ULONG ulQueueSize, HANDLE hSpaceAvailable, HANDLE hDataAvailable, ULONG ulSpaceNotifySize, ULONG ulDataNotifySize);
        ULONG QueueSize();

        void InsertHead(T* pElements, ULONG ulCount, ULONG * pulReturnCount);
        void RemoveTail(T* pElements, ULONG ulCount, ULONG * pulReturnCount);
        void ResetPos();
        ULONGLONG GetTotalOut()
        {
            CRITICAL_SECTION_AUTO_LOCK csl(this);
            return m_ullTotalOut;
        }
        ULONGLONG GetTotalIn()
        {
            CRITICAL_SECTION_AUTO_LOCK csl(this);
            return m_ullTotalIn;
        }
        ULONG DataSize()
        {
            CRITICAL_SECTION_AUTO_LOCK csl(this);
            return _DataSize();
        }


};

/////////////////////////////////////////////////////////////////////////////
// SpAudioPlug

class SpAudioPlug :
	public ISpAudio,
    public ISpEventSource,
    public ISpEventSink,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<SpAudioPlug,&CLSID_SpAudioPlug>,
	public IDispatchImpl<ISpAudioPlug, &IID_ISpAudioPlug, &LIBID_SIMPLEAUDIOLib>,
	public IDispatchImpl<ISpeechAudio, &IID_ISpeechAudio, &LIBID_SIMPLEAUDIOLib>

{
public:
	SpAudioPlug();
    HRESULT FinalConstruct();
    void FinalRelease();

BEGIN_COM_MAP(SpAudioPlug)
	COM_INTERFACE_ENTRY(ISpAudioPlug)
    COM_INTERFACE_ENTRY(ISequentialStream)
    COM_INTERFACE_ENTRY(IStream)
    COM_INTERFACE_ENTRY(ISpStreamFormat)
	COM_INTERFACE_ENTRY(ISpAudio)
    COM_INTERFACE_ENTRY(ISpNotifySource)
    COM_INTERFACE_ENTRY(ISpEventSource)
    COM_INTERFACE_ENTRY(ISpEventSink)
	COM_INTERFACE_ENTRY2(IDispatch, ISpeechAudio)
	COM_INTERFACE_ENTRY(ISpeechAudio)
	COM_INTERFACE_ENTRY(ISpeechBaseStream)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(SpAudioPlug) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_SpAudioPlug)

// ISpAudioPlug
	STDMETHODIMP Init(VARIANT_BOOL fWrite, SpeechAudioFormatType FormatType);
    STDMETHODIMP SetData(VARIANT vData, long * pWritten);
    STDMETHODIMP GetData(VARIANT* vData);


    //--- ISequentialStream ---
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG *pcbWritten);

    //--- IStream ---
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert(void);
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppstm);

    //--- ISpStreamFormat ---
    STDMETHODIMP GetFormat(GUID * pguidFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);

    //--- ISpAudio ---
    STDMETHODIMP SetState(SPAUDIOSTATE NewState, ULONGLONG ullReserved );
    STDMETHODIMP SetFormat(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx);
    STDMETHODIMP GetStatus(SPAUDIOSTATUS *pStatus);
    STDMETHODIMP SetBufferInfo(const SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetBufferInfo(SPAUDIOBUFFERINFO * pInfo);
    STDMETHODIMP GetDefaultFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);
    STDMETHODIMP_(HANDLE) EventHandle();
	STDMETHODIMP GetVolumeLevel(ULONG *pLevel);
	STDMETHODIMP SetVolumeLevel(ULONG Level);
    STDMETHODIMP GetBufferNotifySize(ULONG *pcbSize);
    STDMETHODIMP SetBufferNotifySize(ULONG cbSize);

    //--- ISpNotifySource ---
    //--- ISpEventSource ---
    CSpEventSource m_SpEventSource;
    DECLARE_SPEVENTSOURCE_METHODS(m_SpEventSource)

    //--- ISpEventSink ---
    STDMETHODIMP AddEvents(const SPEVENT* pEventArray, ULONG ulCount);
    STDMETHODIMP GetEventInterest(ULONGLONG * pullEventInterest);

    //--- ISpeechBaseStream ----------------------------------------
    STDMETHODIMP get_Format(ISpeechAudioFormat** ppStreamFormat) { return E_NOTIMPL; };
    STDMETHODIMP putref_Format(ISpeechAudioFormat* pFormat) { return E_NOTIMPL; };
    STDMETHODIMP Read(VARIANT* pvtBuffer, long NumBytes, long* pRead) { return E_NOTIMPL;}
    STDMETHODIMP Write(VARIANT vtBuffer, long* pWritten) { return E_NOTIMPL;}
    STDMETHODIMP Seek(VARIANT Pos, SpeechStreamSeekPositionType Origin, VARIANT* pNewPosition) { return E_NOTIMPL; };

    //--- ISpeechAudio ----------------------------------
	STDMETHODIMP SetState( SpeechAudioState State ) { return E_NOTIMPL; };
	STDMETHODIMP get_Status( ISpeechAudioStatus** ppStatus ) { return E_NOTIMPL; };
    STDMETHODIMP get_BufferInfo(ISpeechAudioBufferInfo** ppBufferInfo) { return E_NOTIMPL; };
    STDMETHODIMP get_DefaultFormat(ISpeechAudioFormat** ppStreamFormat) { return E_NOTIMPL; };
    STDMETHODIMP get_Volume(long* pVolume) { return E_NOTIMPL; };
    STDMETHODIMP put_Volume(long Volume) { return E_NOTIMPL; };
    STDMETHODIMP get_BufferNotifySize(long* pBufferNotifySize) { return E_NOTIMPL; };
    STDMETHODIMP put_BufferNotifySize(long BufferNotifySize) { return E_NOTIMPL; };
    STDMETHODIMP get_EventHandle(long* pEventHandle) { return E_NOTIMPL; };

        void Lock()
        {
            EnterCriticalSection(&m_CriticalSection);
        }

        void Unlock()
        {
            LeaveCriticalSection(&m_CriticalSection);
        }


private:
        SPAUDIOSTATE m_State;
        CSpStreamFormat m_Format;
        CBasicQueueByArray<BYTE> m_Queue;
        SPAUDIOBUFFERINFO m_BufferInfo;
        ULONG m_cbEventBias;
        BOOL m_fWrite;
        HANDLE m_hQueueHasDataEvent; //The event is signaled by SetData/GetData thread or SAPI SetState thread. Read/Write thread waits for this event.
        HANDLE m_hQueueHasSpaceEvent; //The event is signaled by Read/Write thread or SAPI SetState thread , SetData/GetData thread waits for this event.

        ULONG m_ulBufferNotifySize;
        HANDLE m_autohAPIEvent; //When there are m_ulBufferNotifySize data available
        BOOL m_fautohAPIEventSet;
        CRITICAL_SECTION m_CriticalSection;

        void _ProcessEvent();

};

#define SPAUTO_OBJ_LOCK CCriticalSectionLock< SpAudioPlug > lck(this)

#endif // !defined(AFX_SPAUDIOPLUG_H__AD57CF29_3B4A_40A9_BB01_9BE69043E1DF__INCLUDED_)
