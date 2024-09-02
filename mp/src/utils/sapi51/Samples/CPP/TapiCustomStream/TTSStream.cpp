// TTSStream.cpp : Implementation of CTTSStream
#include "stdafx.h"
#include "STCustomStream.h"
#include "TTSStream.h"

#include "Spddkhlp.h"
#include <uuids.h>
#include <amstream.h>

/////////////////////////////////////////////////////////////////////////////
// CTTSStream

class CStreamSampleQueue;

// contains an IStreamSample ptr and is doubly-linked in
// a CStreamSampleQueue
class CQueueElem
{
public:

    friend CStreamSampleQueue;

    CQueueElem(
        IN IStreamSample    *pStreamSample,
        IN CQueueElem        *pPrev,
        IN CQueueElem        *pNext
        )
        : m_pStreamSample(pStreamSample),
          m_pPrev(pPrev),
          m_pNext(pNext)
    {
    }

protected:

    IStreamSample *m_pStreamSample;
    CQueueElem *m_pPrev;
    CQueueElem *m_pNext;
};

// queues CQueueElem instances (FIFO)
// keeps the instances in a doubly-linked list
class CStreamSampleQueue
{
public:

    CStreamSampleQueue()
        : m_Head(NULL, &m_Head, &m_Head)
    {}

    IStreamSample *Dequeue()
    {
        if (m_Head.m_pNext == &m_Head)    return NULL;

        CQueueElem *TargetQueueElem = m_Head.m_pNext;
        m_Head.m_pNext = m_Head.m_pNext->m_pNext;
        m_Head.m_pNext->m_pNext->m_pPrev = &m_Head;

        IStreamSample *ToReturn = TargetQueueElem->m_pStreamSample;
        delete TargetQueueElem;

        return ToReturn;
    }

    BOOL Enqueue(
        IN IStreamSample *pStreamSample
        )
    {
        CQueueElem *TargetQueueElem =
            new CQueueElem(pStreamSample, m_Head.m_pPrev, &m_Head);
        if (NULL == TargetQueueElem)    return FALSE;

        m_Head.m_pPrev->m_pNext = TargetQueueElem;
        m_Head.m_pPrev = TargetQueueElem;

        return TRUE;
    }


protected:

    CQueueElem    m_Head;
};

//////////////////////////////////////////////////////////////////////////////
// Query the media stream interface and get the stream format from the capture 
// terminal. This stream object is used for the H.323 connections. If
// you want to use it over the voice modem or other voice boards, you need to 
// set the media stream format by using ITAMMediaFormat::put_MediaFormat(), 
// which is mandatory or the terminal will not be able to connect.

STDMETHODIMP CTTSStream::InitTTSCaptureStream( IUnknown *pCaptureTerminal)
{

	HRESULT hr = S_OK;
	CComQIPtr<ITTerminal> cpTerminal(pCaptureTerminal);

	m_cpTTSMediaStream.Release();


	//Query the IMediaStream interface from the terminal
	if ( cpTerminal )
	{
	   hr = cpTerminal->QueryInterface(IID_IMediaStream, (void**)&m_cpTTSMediaStream);
      
	}
    else
    {
        hr = E_INVALIDARG;
		cpTerminal.Release();
		return hr;
    }

	//Get the ITAMMediaFormat and then retrieve the format
	ITAMMediaFormat *pITFormat;
	hr = cpTerminal->QueryInterface(IID_ITAMMediaFormat, (void **)&pITFormat);
	
	if (FAILED(hr)) 
	{ 
		cpTerminal.Release();  
		return hr; 
	}

	AM_MEDIA_TYPE       *pmt;
	if(SUCCEEDED(hr))
    {
		hr = pITFormat->get_MediaFormat(&pmt);
	}

	//Store the wave format
	if(SUCCEEDED(hr))
    {
		hr = m_StreamFormat.AssignFormat (SPDFID_WaveFormatEx, ( WAVEFORMATEX * )pmt->pbFormat );
       ::CoTaskMemFree(pmt);
    }

	//Release the objects
	pITFormat->Release();
	cpTerminal.Release();
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//Release the objects
//
void CTTSStream::FinalRelease()
{ 
    m_cpTTSMediaStream.Release();
   
}

//////////////////////////////////////////////////////////////////////////////
// TTS engines call this method, Write(), through SAPI whenever the audio data
// is ready.
//
STDMETHODIMP CTTSStream::Write(const void * pv, ULONG cb, ULONG * pcbWritten)
{
  
    m_hCritSec.Lock();
    HRESULT hr = S_OK;
	ULONG lWritten = 0;
	ULONG ulPos =0;
    BYTE *  pbData = (BYTE *)pv;

       
    if (SPIsBadReadPtr(pv, cb) || SP_IS_BAD_OPTIONAL_WRITE_PTR(pcbWritten))
    {
        hr = STG_E_INVALIDPOINTER;
    }

	//wait for the media stream to be active. Maximum is 6 seconds
	hr = WaitUntilReady(6000);
	m_bFlag = 0;

	//
    // create a queue (STL deque) that will hold all the samples that we ever 
    // submitted to media streaming terminal
    //
    // we need this so when we are finished reading the data, we can go through 
    // the list of all the samples that we have submitted and make sure mst 
    // is finished processing them
    //
    
    CStreamSampleQueue DestSampleQ;

    //
    // keep reading samples from the buffer and sending them.
    //
    while ( SUCCEEDED ( hr ) )
    {
       
        //
        // allocate a sample on the terminal's media stream
        //
        // Note: the call to AllocateSample() will block if we filled all the 
        // samples with data, and there are no more samples for us to fill 
        // (waiting for media streaming terminal to process samples we have
        // submitted). When MST is is done with at least one sample, the call 
        // will return. This logic will ensure that MST always has work and is
        // never starved for samples.
        //

        IStreamSample *pStreamSample = NULL;

        hr = m_cpTTSMediaStream->AllocateSample(0, &pStreamSample);

        if (FAILED(hr))
        {
            break;
        }

        //
        // get IMemoryData on the sample so we can get to the sample's memory 
        // data
        //

        IMemoryData *pSampleMemoryData = NULL;

        hr = pStreamSample->QueryInterface(IID_IMemoryData, 
                                           (void**)&pSampleMemoryData);

        if (FAILED(hr))
        {
            pStreamSample->Release();
            pStreamSample = NULL;

            break;

        }

        //
        // get to the sample's memory buffer
        //

        ULONG nBufferSize = 0;

        BYTE *pBuffer = NULL;

        hr = pSampleMemoryData->GetInfo(&nBufferSize, &pBuffer, NULL);

        if (FAILED(hr))
        {
            pStreamSample->Release();
            pStreamSample = NULL;

            pSampleMemoryData->Release();
            pSampleMemoryData = NULL;

            break;

        }

        //
        // read ISptream into memory buffer provided by the sample
        //

		nBufferSize = min ( nBufferSize, cb - ulPos);
        memcpy ( pBuffer, (BYTE *)(pbData+ ulPos), nBufferSize);
 
		ulPos += nBufferSize;
       
        //
        // tell the sample how many useful bytes are in the sample's buffer
        //
        
        hr = pSampleMemoryData->SetActual(nBufferSize);

        pSampleMemoryData->Release();
        pSampleMemoryData = NULL;

        if (FAILED(hr))
        {
            pStreamSample->Release();
            pStreamSample = NULL;
            break;
        }

        
        //
        // we are done with the sample. now let media streaming terminal 
        // process it asynchronously. when the terminal is finished with 
        // the sample, this sample will be returned to us from the call 
        // to AllocateSample()
        //
	
		hr = pStreamSample->Update(SSUPDATE_ASYNC, NULL, NULL, 0);

        //
        // with some MSPs, starting the stream can be done asynchronously, so
        // there may be a delay between the time when terminal is selected
        // (or call connected) and the time when the stream becomes usable.
        //
        // attempting to use the stream before the stream is active would
        // result in the Update() returning error VFW_E_NOT_COMMITTED.
        //
        // Usually an application would not start using the stream until 
        // it gets media event CME_STREAM_ACTIVE. This requires the app
        // to register a callback interface by calling 
        // ITTAPI::RegisterCallNotifications. Refer to documentation and other 
        // samples for more details on how this is done.
        //

		//Break the loop when we reach the end of the buffer or errors
        if ( FAILED(hr) || ulPos == cb )
        {
            pStreamSample->Release();
            pStreamSample = NULL;

            break;
		}
 

        //
        // keep the sample we have just submitted. on exit, we will wait 
        // for it to be processed by mst
        //

        if (!DestSampleQ.Enqueue(pStreamSample)) 
        { 
            break; 
        }

    } // IStream reading/sample-filling loop
    
	if ( SUCCEEDED ( hr ) )
	{
		//There is no error here, so set the hr to S_OK in case that some engines
		//stop calling Write() when hr is not S_OK.
		hr = S_OK;

		//
		// walk through the list of all the samples we have submitted and wait for 
		// each sample to be done
		//
		IStreamSample *pStreamSample = DestSampleQ.Dequeue();
		while (NULL != pStreamSample)
		{
			// ignore any error values
			pStreamSample->CompletionStatus(COMPSTAT_WAIT, INFINITE);
			pStreamSample->Release();
			pStreamSample = NULL;

			pStreamSample = DestSampleQ.Dequeue();
		}
		if ( pStreamSample )
		{        
			pStreamSample->Release();
			pStreamSample = NULL;
		}
	}

	m_hCritSec.Unlock();   
	return hr;
};

////////////////////////////////////////////////////////////////////////
// Wait until the stream is active
//
HRESULT	CTTSStream::WaitUntilReady( DWORD WaitTime)
{
	HRESULT hr = S_OK;
	IStreamSample *pStreamSample = NULL;
  
	//only checks if the stream is active at the first time per connection
	if ( !m_bFlag )
	{
		return hr;
	}
 
    hr = m_cpTTSMediaStream->AllocateSample(0, &pStreamSample);

	if ( SUCCEEDED ( hr ) )
	{
		//0x80040211 is the error code of VFW_E_NOT_COMMITTED in DirectShow
		//VFW_E_NOT_COMMITTED error means:
		//"Cannot allocate a sample when the allocator is not active"

		DWORD dwStart = GetTickCount();
		do 
		{
			hr = pStreamSample->Update(SSUPDATE_ASYNC, NULL, NULL, 0);

		}while  ( (0x80040211 == hr) && ((GetTickCount() - dwStart) < WaitTime) );
	}
   
	if (pStreamSample)
	{
		pStreamSample->Release();
		pStreamSample = NULL;
	}
        
	return hr;
}

////////////////////////////////////////////////////////////////////////
// This method is used by SAPI
//
STDMETHODIMP CTTSStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	// We only accept queries for the current stream position
	if (STREAM_SEEK_CUR != dwOrigin || dlibMove.QuadPart)
	{
		return E_INVALIDARG;
    }

	// Validate the OUT parameter
	if (SPIsBadWritePtr(plibNewPosition, sizeof(ULARGE_INTEGER)))
	{
		return E_POINTER;
	}

    m_hCritSec.Lock();

    plibNewPosition->QuadPart = (LONG)dlibMove.LowPart;

    m_hCritSec.Unlock();

	return S_OK;
}

////////////////////////////////////////////////////////////////////////
// This method is used by SAPI
//
STDMETHODIMP CTTSStream::GetFormat(GUID * pFmtId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    m_hCritSec.Lock();

    HRESULT hr = S_OK;

	hr = m_StreamFormat.ParamValidateCopyTo( pFmtId, ppCoMemWaveFormatEx );


    m_hCritSec.Unlock();

    return hr;
}
