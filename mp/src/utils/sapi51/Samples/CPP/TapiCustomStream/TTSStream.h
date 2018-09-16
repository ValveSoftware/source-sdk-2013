// TTSStream.h : Declaration of the CTTSStream

#ifndef __TTSSTREAM_H_
#define __TTSSTREAM_H_

#include "resource.h"       // main symbols
#include "sapi.h"

#include <windows.h>
#include <atlbase.h>
#include "SAPI.h"
#include "sphelper.h"
#include <tapi3.h>
#include <mmsystem.h>
#include <amstream.h>
#include <strmif.h>
#include <stdio.h>
#include <assert.h>
#include <crtdbg.h>
#include <vfw.h>

/////////////////////////////////////////////////////////////////////////////
// CTTSStream
class ATL_NO_VTABLE CTTSStream : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTTSStream, &CLSID_TTSStream>,
	public ISpStreamFormat,
	public IDispatchImpl<ITTSStream, &IID_ITTSStream, &LIBID_STCUSTOMSTREAMLib>
{
public:
	CTTSStream()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TTSSTREAM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTTSStream)
	COM_INTERFACE_ENTRY(ITTSStream)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISpStreamFormat)
	COM_INTERFACE_ENTRY(IStream)
END_COM_MAP()

// ITTSStream
public:

	STDMETHOD(InitTTSCaptureStream)(IUnknown *pCaptureTerminal);
	STDMETHOD(WaitUntilReady)(DWORD WaitTime);
	void FinalRelease();
    

	//
    //  IStream
    //
 
    STDMETHODIMP Read(void * pv, ULONG cb, ULONG * pcbRead)
	{
        return STG_E_ACCESSDENIED;
    }
    STDMETHODIMP Write(const void * pv, ULONG cb, ULONG * pcbWritten);
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
   
 
public:

	CSpStreamFormat         m_StreamFormat;

private:

    CComAutoCriticalSection m_hCritSec;
    CComPtr<IMediaStream>   m_cpTTSMediaStream;
	BOOL					m_bFlag:1;

};

#endif //__TTSSTREAM_H_
