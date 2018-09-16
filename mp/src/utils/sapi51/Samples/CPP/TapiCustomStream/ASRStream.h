// ASRStream.h : Declaration of the CASRStream

#ifndef __ASRSTREAM_H_
#define __ASRSTREAM_H_

#include "resource.h"       // main symbols
#include "sapi.h"

#include <windows.h>
#include <atlbase.h>
#include "SAPI.h"
#include "sphelper.h"
#include <tapi3.h>
#include <uuids.h>
#include <mmsystem.h>
#include <amstream.h>
#include <strmif.h>
#include <stdio.h>
#include <assert.h>
#include <crtdbg.h>
#include <vfw.h>

/////////////////////////////////////////////////////////////////////////////
// CASRStream
class ATL_NO_VTABLE CASRStream : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CASRStream, &CLSID_ASRStream>,
	public ISpStreamFormat,
	public IDispatchImpl<IASRStream, &IID_IASRStream, &LIBID_STCUSTOMSTREAMLib>
{
public:
	CASRStream()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ASRSTREAM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CASRStream)
	COM_INTERFACE_ENTRY(IASRStream)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISpStreamFormat)
	COM_INTERFACE_ENTRY(IStream)

END_COM_MAP()

// IASRStream
public:
	STDMETHOD(InitSRRenderStream)(IUnknown *pRenderTerminal);
	STDMETHOD(PurgeStream)();
	STDMETHOD(RestartStream)();
	STDMETHOD(CleanUp)();
	HRESULT FinalConstruct();
	void FinalRelease();
 

	//
    //  IStream
    //
 
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG * pcbRead);
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG * pcbWritten)
    {
        return STG_E_ACCESSDENIED;
    }
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) 
    {
        return STG_E_ACCESSDENIED;
    }
    STDMETHODIMP CopyTo(IStream *pStreamDest, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER __RPC_FAR *pcbWritten)
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP Commit(DWORD grfCommitFlags)
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP Revert(void) 
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) 
    {
        return E_NOTIMPL;  
    }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag)
    {
        return E_NOTIMPL; 
    }
    STDMETHODIMP Clone(IStream ** ppstm)
    {
        return E_NOTIMPL;
    }
    //
    //  ISpStreamFormat
    //
    STDMETHODIMP GetFormat(GUID * pFormatId, WAVEFORMATEX ** ppCoMemWaveFormatEx);


private:

    HRESULT RenderAudioStream(ULONG start);
    HRESULT GetSampleID(DWORD nWaitCode,  DWORD nNumberOfSamples, DWORD *pnSampleID);  
    HRESULT ReadStreamSample(IStreamSample *pStreamSample, ULONG start );
    HRESULT AssociateEventsWithSamples(HANDLE *pSampleReadyEvents,
                                       IStreamSample **ppStreamSamples,
                                       DWORD nNumberOfSamples);
    IStreamSample ** AllocateStreamSamples( IMediaStream *pMediaStream, DWORD nNumberOfSamples);
    void    ReleaseSamples(IStreamSample **ppStreamSamples, DWORD nNumberOfSamples);
    HANDLE* AllocateEvents(DWORD nNumberOfEvents);
    void    ReleaseEvents(HANDLE *pEvents, DWORD nNumberOfEvents );
    HRESULT GetNumberOfSamplesOnStream(IMediaStream *pTerminalMediaStream,DWORD *pnNumberOfSamples);
    void    *AllocateMemory(SIZE_T nMemorySize);
    void    FreeMemory(void *pMemory);

private:

    CComAutoCriticalSection m_hCritSec;
    ULONG                   m_ulBufferSize;
    ULONG                   m_ulActualRead;
    BYTE *                  m_pnDataBuffer;
	ULONG                   m_ulLeftOver;
	ULONG                   m_ulLeftOverPos;
    BYTE *                  m_pnLeftOverBuffer;
	DWORD					m_nNumberOfSamples ;
	BOOL					m_bFlag ;
	BOOL					m_bPurgeFlag;
	HANDLE *				m_pSampleReadyEvents ;
    CComPtr<IMediaStream>   m_cpSRMediaStream;	
	IStreamSample			**m_ppStreamSamples ;
	CSpStreamFormat         m_StreamFormat;
	  
};

#endif //__ASRSTREAM_H_
