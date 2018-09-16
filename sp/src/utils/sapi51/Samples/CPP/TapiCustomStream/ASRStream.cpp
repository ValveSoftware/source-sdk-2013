// ASRStream.cpp : Implementation of CASRStream
#include "stdafx.h"
#include "STCustomStream.h"
#include "ASRStream.h"
#include "Spddkhlp.h"

const WAIT_TIME_OUT = 20000; // 20 seconds
/////////////////////////////////////////////////////////////////////////////
// CASRStream

////////////////////////////////////////////////////////////////////////
HRESULT CASRStream::FinalConstruct()
{
	
    HRESULT hr = S_OK;

	//Initialize data members
    m_ulBufferSize  = 0;
    m_ulActualRead  = 0;
    m_pnDataBuffer  = NULL;
	m_ulLeftOver    = 0;
	m_ulLeftOverPos = 0;
	m_nNumberOfSamples = 0;
	m_pSampleReadyEvents = NULL;
	m_ppStreamSamples = NULL;		
    m_pnLeftOverBuffer  = NULL;

	m_bFlag = FALSE;
	m_bPurgeFlag = FALSE;

	return hr;
}

////////////////////////////////////////////////////////////////////////
void CASRStream::FinalRelease()
{

	//Release objects
	CleanUp();
 
    m_cpSRMediaStream.Release();
   
}

////////////////////////////////////////////////////////////////////////
// Initialize the SR render stream and store the wave format 
// 
STDMETHODIMP CASRStream::InitSRRenderStream( IUnknown *pRenderTerminal)
{

	HRESULT hr = S_OK;
	CComQIPtr<ITTerminal> cpTerminal(pRenderTerminal);

	//Release the stream, if it exists
    if ( m_cpSRMediaStream )
    {
        m_cpSRMediaStream.Release();
    }

	//Query the IMediaStream interface from the terminal
	if ( cpTerminal )
	{
	   hr = cpTerminal->QueryInterface(IID_IMediaStream, (void**)&m_cpSRMediaStream);
      
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
	pITFormat->Release();

	//Store the wave format
	if(SUCCEEDED(hr))
    {
		hr = m_StreamFormat.AssignFormat (SPDFID_WaveFormatEx, ( WAVEFORMATEX * )pmt->pbFormat );
        ::CoTaskMemFree(pmt);
    }

	cpTerminal.Release();
	return hr;
}

////////////////////////////////////////////////////////////////////////
//  Release the internal objects
// 
STDMETHODIMP CASRStream::CleanUp()
{
	HRESULT hr = S_OK;
	m_hCritSec.Lock();

	//Release objects
	if ( m_pSampleReadyEvents)
	{
		ReleaseEvents(m_pSampleReadyEvents, m_nNumberOfSamples);
		m_pSampleReadyEvents = NULL;
	}

	if ( m_ppStreamSamples )
	{
		ReleaseSamples(m_ppStreamSamples, m_nNumberOfSamples);
		m_ppStreamSamples = NULL;
	}

	if ( m_pnDataBuffer )
    {
        delete[] m_pnDataBuffer;
        m_pnDataBuffer = NULL;
    }
    
	if ( m_pnLeftOverBuffer )
    {
        delete[] m_pnLeftOverBuffer;
        m_pnLeftOverBuffer = NULL;
    }

    m_hCritSec.Unlock();

	return hr;
}

////////////////////////////////////////////////////////////////////////
// Set the purge stream flag to TRUE so that SR engine will get the less 
// audio data than it wants. The SR engine will stop calling Read(), because 
// the SR engine thinks it reaches the end of the stream.
// 
STDMETHODIMP CASRStream::PurgeStream()
{
	HRESULT hr = S_OK;
	m_hCritSec.Lock();

	m_bPurgeFlag = TRUE;
	m_bFlag = FALSE;

    m_hCritSec.Unlock();

	return hr;
}

////////////////////////////////////////////////////////////////////////
// Set the purge stream flag to FALSE. This allows the SR engine to
// to get the data in the next Read() calls.
// Reset the other variables.
// 
STDMETHODIMP CASRStream::RestartStream()
{
	HRESULT hr = S_OK;
	m_hCritSec.Lock();

	hr = CleanUp();

	m_ulBufferSize  = 0;
    m_ulActualRead  = 0;

	m_ulLeftOver    = 0;
	m_ulLeftOverPos = 0;
	m_nNumberOfSamples = 0;

	m_bFlag = FALSE;
	m_bPurgeFlag = FALSE;

    m_hCritSec.Unlock();

	return hr;
}
////////////////////////////////////////////////////////////////////////
// read the terminal's allocator properties to return the number of samples
// the terminal provides
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CASRStream::GetNumberOfSamplesOnStream(IMediaStream *pTerminalMediaStream,
                                   OUT DWORD *pnNumberOfSamples)
{

    HRESULT hr = S_OK;


    //
    // don't return garbage
    //
       
    *pnNumberOfSamples = 0;


    //
    // get allocator properties
    //

    ITAllocatorProperties *pAllocProperites = NULL;

    hr = pTerminalMediaStream->QueryInterface(IID_ITAllocatorProperties,
                                        (void **)&pAllocProperites);

    if (FAILED(hr))
    {
        return hr;
    }

    
    //
    // we want to know the number of samples we will be getting
    //

    ALLOCATOR_PROPERTIES AllocProperties;

    hr = pAllocProperites->GetAllocatorProperties(&AllocProperties);

    pAllocProperites->Release();
    pAllocProperites = NULL;


    if (FAILED(hr))
    {
        return hr;
    }

    *pnNumberOfSamples = AllocProperties.cBuffers;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// close the handles passed into the function and release the array of handles
//
///////////////////////////////////////////////////////////////////////////////

void CASRStream::ReleaseEvents(OUT HANDLE *pEvents,   // array of events to be freed
                                     DWORD nNumberOfEvents  // number of events the array
                                    )
{

    //
    // close all the handles in the array
    //

    for (DWORD i = 0; i < nNumberOfEvents; i++)
    {
        CloseHandle(pEvents[i]);
        pEvents[i] = NULL;
    }

    
    //
    // free the array itself
    //

    FreeMemory(pEvents);
    pEvents = NULL;
}


//////////////////////////////////////////////////////////////////////////////
// allocate the array of events of size nNumberOfSamples
//
// return pointer to the allocated and initialized array if success
// or NULL if failed
//
///////////////////////////////////////////////////////////////////////////////

HANDLE* CASRStream::AllocateEvents(DWORD nNumberOfEvents)
{

    //
    // pointer to an array of event handles
    //

    HANDLE *pSampleReadyEvents = NULL;

    pSampleReadyEvents = 
        (HANDLE*)AllocateMemory(sizeof(HANDLE) * nNumberOfEvents);

    if (NULL == pSampleReadyEvents)
    {
		//Failed to allocate sample ready events.
        
        return NULL;
    }


    //
    // create an event for every allocated handle
    //

    
    for (DWORD i = 0; i < nNumberOfEvents; i++)
    {

        pSampleReadyEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        if (NULL == pSampleReadyEvents[i])
        {

            //
            // close handles we have created already
            //

            for (DWORD j = 0; j< i; j++)
            {
                CloseHandle(pSampleReadyEvents[j]);

                pSampleReadyEvents[j] = NULL;
            }


            FreeMemory(pSampleReadyEvents);
            pSampleReadyEvents = NULL;

            return NULL;

        }

    } // creating events for each sample

    
    //
    // succeded creating events. return the pointer to the array
    //

    return pSampleReadyEvents;

}

//////////////////////////////////////////////////////////////////////////////
// aborts and releases every sample in the array of samples of size 
// nNumberOfSamples and deallocates the array itself
//
// ppStreamSamples becomes invalid when the function returns
//
///////////////////////////////////////////////////////////////////////////////

void CASRStream::ReleaseSamples(IStreamSample **ppStreamSamples,
                    DWORD nNumberOfSamples)
{

    for (DWORD i = 0; i < nNumberOfSamples; i++)
    {
        ppStreamSamples[i]->CompletionStatus(COMPSTAT_WAIT |
                                             COMPSTAT_ABORT,
                                             INFINITE);

        
        //
        // regardless of the error code, release the sample
        //

        ppStreamSamples[i]->Release();
        ppStreamSamples[i] = NULL;
    }

    FreeMemory(ppStreamSamples);
    ppStreamSamples = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// allocate the array of nNumberOfSamples samples, and initialize each sample 
// pointer in the array with samples from the supplied stream.
//
// return pointer to the allocated and initialized array if success
// or NULL if failed
//
///////////////////////////////////////////////////////////////////////////////

IStreamSample ** CASRStream::AllocateStreamSamples(IMediaStream *pMediaStream,
                                      DWORD nNumberOfSamples)
{

    //
    // allocate stream sample array
    //

    IStreamSample **ppStreamSamples = (IStreamSample **)
        AllocateMemory( sizeof(IStreamSample*) * nNumberOfSamples );


    if (NULL == ppStreamSamples)
    {
        return NULL;
    }


    //
    // allocate samples from the stream and put them into the array
    // 

    for (DWORD i = 0; i < nNumberOfSamples; i++)
    {

        HRESULT hr = pMediaStream->AllocateSample(0, &ppStreamSamples[i]);

        if (FAILED(hr))
        {

            for (DWORD j = 0; j < i; j++)
            {
                ppStreamSamples[j]->Release();
                ppStreamSamples[j] = NULL;
            }

            FreeMemory(ppStreamSamples);
            ppStreamSamples = NULL;

            return NULL;
            
        } // failed AllocateSample()

    } // allocating samples on the stream



    //
    // succeeded allocating samples
    //

    return ppStreamSamples;

}


//////////////////////////////////////////////////////////////////////////////
// call Update() on every sample of the array of stream samples to associate it
// with an event from the array of events. The events will be signaled when the
// corresponding sample has data and is ready to render to SAPI
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CASRStream::AssociateEventsWithSamples(HANDLE *pSampleReadyEvents,
                                   IStreamSample **ppStreamSamples,
                                   DWORD nNumberOfSamples)
{

    for (DWORD i = 0; i < nNumberOfSamples; i++)
    {

        //
        // the event passed to Update will be signaled when the sample is 
        // filled with data
        //

        HRESULT hr = 
            ppStreamSamples[i]->Update(0, pSampleReadyEvents[i], NULL, 0);


        if (FAILED(hr))
        {
          
            //
            // abort the samples we have Update()'d
            //

            for (DWORD j = 0; j < i; j++)
            {
      
                //
                // no need to check the return code here -- best effort attempt
                // if failed -- too bad
                //

                ppStreamSamples[j]->CompletionStatus(COMPSTAT_WAIT |
                                                     COMPSTAT_ABORT,
                                                     INFINITE);
            }

            return hr;

        } // Update() failed

    } // Update()'ing all samples


    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// This function writes the data portion of the sample into the SAPI
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CASRStream::ReadStreamSample(IStreamSample *pStreamSample, ULONG start)
{

    //
    // get the sample's IMemoryData interface so we can get to the 
    // sample's data
    //

    IMemoryData *pSampleMemoryData = NULL;

    HRESULT hr = pStreamSample->QueryInterface(IID_IMemoryData,
                                               (void **)&pSampleMemoryData);

    if (FAILED(hr))
    {
        return hr;
    }

    //
    // get to the sample's data buffer
    //
    DWORD nBufferSize = 0;
    BYTE  *pnDataBuffer = NULL;
    DWORD nActualDataSize = 0;

    hr = pSampleMemoryData->GetInfo(&nBufferSize,
                                    &pnDataBuffer,
                                    &nActualDataSize);

 
    if ( SUCCEEDED ( hr ) )
    {
        m_ulActualRead = (ULONG) nActualDataSize;
        if ( nActualDataSize + start <= m_ulBufferSize )
        {   
			//If SR engine wants more data then the media stream contains, copy it directly
			//to the current buffer
            memcpy ( (BYTE *)&m_pnDataBuffer[start], (BYTE *)pnDataBuffer, nActualDataSize * sizeof(BYTE)); 
        }
        else
        { // we have left over data
            memcpy ( (BYTE *)&m_pnDataBuffer[start], (BYTE *)pnDataBuffer, (m_ulBufferSize - start - m_ulLeftOver ) * sizeof(BYTE)); 
 
            ULONG ulLeftOver = nActualDataSize + start + m_ulLeftOver - m_ulBufferSize;
            
            if ( m_pnLeftOverBuffer )
            {
                delete[] m_pnLeftOverBuffer;
                m_pnLeftOverBuffer = NULL;
            }
            
            m_pnLeftOverBuffer = new BYTE[ ulLeftOver ];
            
            if ( m_pnLeftOverBuffer == NULL )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memcpy ( (BYTE *)m_pnLeftOverBuffer, (BYTE *)&pnDataBuffer[m_ulBufferSize - start - m_ulLeftOver], ulLeftOver * sizeof(BYTE)); 
            }       
        }
    }

	pSampleMemoryData->Release();
    pSampleMemoryData = NULL;
    
    return hr;
}




//////////////////////////////////////////////////////////////////////////////
// given the return code fom WaitForMultipleObjects, this function determines
// which sample was signal and returns S_OK and the id of the signaled sample
// or E_FAIL if WaitForMultipleEvents returned an error
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CASRStream::GetSampleID(DWORD nWaitCode, // code from WaitForMultiple...
                    DWORD nNumberOfSamples,		// the total number of samples
                    DWORD *pnSampleID)			// the calculated id of the 
                                                // signaled sample
{


    //
    // event abandoned?
    //

    if ( (nWaitCode >= WAIT_ABANDONED_0) && 
         (nWaitCode < WAIT_ABANDONED_0 + nNumberOfSamples) )
    {
        return E_FAIL;
    }


    //
    // any other error?
    //

    if ( (WAIT_OBJECT_0 + nNumberOfSamples <= nWaitCode) )
    {
        return E_FAIL;
    }


    //
    // which sample was signaled?
    //

    *pnSampleID = nWaitCode - WAIT_OBJECT_0;

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// extract samples from the terminal's media stream and render it to SR engine
//
// returns when the call is disconnected (call disconnect causes media streaming
// terminal to abort the samples
//
///////////////////////////////////////////////////////////////////////////////

HRESULT CASRStream::RenderAudioStream(ULONG start)
{

    HRESULT hr = S_OK;
	DWORD nSampleID = 0;

	if ( !m_bFlag )
	{
		//turn off the flag. Only initialize once for each connection
		m_bFlag = TRUE;

		// get the number of stream samples we will be using
		hr = GetNumberOfSamplesOnStream(m_cpSRMediaStream, &m_nNumberOfSamples);

		if (FAILED(hr))
		{
			return hr;
		}

		//
		// the number of samples directly corresponds the number of events we will 
		// be waiting on later. WaitForMultipleObjects has a limit of 
		// MAXIMUM_WAIT_OBJECTS events.
		//

		if (m_nNumberOfSamples > MAXIMUM_WAIT_OBJECTS)
		{
			return E_FAIL;
		}

   
		//
		// allocate events that will be signaled when each sample is ready to be 
		// read
		//

		m_pSampleReadyEvents = AllocateEvents(m_nNumberOfSamples);

		if (NULL == m_pSampleReadyEvents)
		{
			return E_OUTOFMEMORY;
		}

		//
		// allocate array of stream samples
		//

		m_ppStreamSamples = AllocateStreamSamples(m_cpSRMediaStream, 
												m_nNumberOfSamples);

		if (NULL == m_ppStreamSamples)
		{
			//
			// release events we have allocated
			//

			ReleaseEvents(m_pSampleReadyEvents, m_nNumberOfSamples);
			m_pSampleReadyEvents = NULL;

			return E_FAIL;
		}


		//
		// we have the samples, we have the events. 
		// associate events with samples so events get signaled when the 
		// corresponding samples are ready to be read
		// 
	
		hr = AssociateEventsWithSamples(m_pSampleReadyEvents,
										m_ppStreamSamples,
										m_nNumberOfSamples);

		if (FAILED(hr))
		{
			//
			// release events and samples we have allocated
			//

			ReleaseEvents(m_pSampleReadyEvents, m_nNumberOfSamples);
			m_pSampleReadyEvents = NULL;

			ReleaseSamples(m_ppStreamSamples, m_nNumberOfSamples);
			m_ppStreamSamples = NULL;

			return E_FAIL;
		}

	}
	if ( SUCCEEDED ( hr ) )
    {

        //
        // wait for the events associated with the samples
        // when a samples has data, the corresponding event will be 
        // signaled
        //
    
        DWORD nWaitCode = WaitForMultipleObjects(m_nNumberOfSamples,
                                                 m_pSampleReadyEvents,
                                                 FALSE,
                                                 WAIT_TIME_OUT);

        
        //
        // get the id of the sample that was signaled. fail if Wait returned
        // error
        // 

        hr = GetSampleID(nWaitCode, m_nNumberOfSamples, &nSampleID);

	}


    //
    // we filtered out all invalid error codes. so nSampleID has no 
    // choice but be a valid sample index.
    //

    _ASSERTE(nSampleID < m_nNumberOfSamples);

	if ( SUCCEEDED ( hr ) )
	{
  
        //
        // make sure the sample is ready to be read
        //

        hr = m_ppStreamSamples[nSampleID]->CompletionStatus(COMPSTAT_WAIT, 0);
	}
    
  
    //
    // we have the sample that was signaled and which is now ready to be
	// read. Record the sample.
    //
	if ( SUCCEEDED ( hr ) )
	{
        hr = ReadStreamSample(m_ppStreamSamples[nSampleID], start);
	}

  
    //
    // we are done with this sample. return it to the source stream
    // to be refilled with data
    //
	if ( SUCCEEDED ( hr ) )
	{
  
       hr = m_ppStreamSamples[nSampleID]->Update(0, m_pSampleReadyEvents[nSampleID],NULL,0);

	}

	if (FAILED(hr))
	{
		//
		// release events and samples we have allocated
		//

		ReleaseEvents(m_pSampleReadyEvents, m_nNumberOfSamples);
		m_pSampleReadyEvents = NULL;

		ReleaseSamples(m_ppStreamSamples, m_nNumberOfSamples);
		m_ppStreamSamples = NULL;
	}

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// use win32 heap api to allocate memory on the application's heap
// and zero the allocated memory
//
///////////////////////////////////////////////////////////////////////////////
void *CASRStream::AllocateMemory(SIZE_T nMemorySize)
{    
    //
    // use HeapAlloc to allocate and clear memory
    //

    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nMemorySize);
}


//////////////////////////////////////////////////////////////////////////////
//
// use win32 heap api to free memory previously allocated on the application's
// heap
//
///////////////////////////////////////////////////////////////////////////////
void CASRStream::FreeMemory(void *pMemory)
{
    
    //
    // get size of the allocated memory
    //

    DWORD nMemorySize = HeapSize(GetProcessHeap(), 0, pMemory);


    //
    // fill memory with 0xdd's before freeing, so it is easier to debug 
    // failures caused by using pointer to deallocated memory
    //
    
    if (NULL != pMemory)
    {
        FillMemory(pMemory, nMemorySize, 0xdd);
    }

    
    //
    // use HeapFree to free memory. use return code to log the result, but
    // do not return it to the caller
    //
    
    BOOL bFreeSuccess = HeapFree(GetProcessHeap(), 0, pMemory);

    if (FALSE == bFreeSuccess)
    {
        //
        // if this assertion fires, it is likely there is a problem with the 
        // memory we are trying to deallocate. Was it allocated using heapalloc
        // and on the same heap? Is this a valid pointer?
        //

        _ASSERTE(FALSE);
    }

}

//////////////////////////////////////////////////////////////////////////////
//
// SR engine calls the Read to get the audio through SAPI
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CASRStream::Read(void * pv, ULONG cb, ULONG *pcbRead)
{
    m_hCritSec.Lock();

    ULONG   start;
    LONG    len = cb;
    BYTE *  pnData = (BYTE *)pv;
    HRESULT hr = S_OK;


	if (m_bPurgeFlag)
	{
		//the app wants to stop rendering the stream
		m_bPurgeFlag = FALSE;
		ReleaseEvents(m_pSampleReadyEvents, m_nNumberOfSamples);
		m_pSampleReadyEvents = NULL;

		ReleaseSamples(m_ppStreamSamples, m_nNumberOfSamples);
		m_ppStreamSamples = NULL;
		
		*pcbRead = 0;
		m_hCritSec.Unlock();
		return hr;
	}

	//allocate the buffer
    if ( m_pnDataBuffer == NULL )
    {
        m_ulBufferSize = cb;
        m_pnDataBuffer = new BYTE [ m_ulBufferSize ];
        if ( m_pnDataBuffer == NULL )
        {
			m_hCritSec.Unlock();
            return E_OUTOFMEMORY;
        }
    }
	else if ( m_ulBufferSize != cb)
	{	   
		//allocate a new buffer 
		delete []m_pnDataBuffer;
		m_ulBufferSize = cb;
        m_pnDataBuffer = new BYTE [ m_ulBufferSize ];
        if ( m_pnDataBuffer == NULL )
        {
			m_hCritSec.Unlock();
            return E_OUTOFMEMORY;
        }
    }
	
	//copy the data left from the previous Read().
    if ( m_pnLeftOverBuffer && m_ulLeftOver > 0 )
    {
        memcpy ( pnData, (BYTE *)&m_pnLeftOverBuffer[m_ulLeftOverPos], min(cb, m_ulLeftOver) * sizeof(BYTE) );
    }

	//Read the data from MST
    start = 0;
    len = cb - m_ulLeftOver;

    while ( SUCCEEDED ( hr )  && len > 0 )
    {
        hr = RenderAudioStream( start );        
        start += m_ulActualRead;
        len = cb - m_ulLeftOver - start;
    }
    
    if ( SUCCEEDED ( hr ) )
    {
		// tell SR engine, we have cb bytes available.
        *pcbRead = cb;
       
        // Copy the rest of data outside and set the postion and number of bytes left in
		// the left over data buffer 
		long lRead = *pcbRead - m_ulLeftOver;
        if ( lRead > 0 )
        {
            m_ulLeftOverPos = 0;
            memcpy ( &pnData[m_ulLeftOver], (BYTE *)m_pnDataBuffer, (*pcbRead - m_ulLeftOver) * sizeof(BYTE) );
        }
        else
        {
            m_ulLeftOverPos += cb;
        }
        
        if ( len < 0 )
        {
            m_ulLeftOver = -len;
        }
		else
		{
            m_ulLeftOverPos = 0;
            m_ulLeftOver = 0;
		}
    }

    m_hCritSec.Unlock();

    return hr;
} 

//////////////////////////////////////////////////////////////////////////////
//
// This method is called by SAPI
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CASRStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
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

////////////////////////////////////////////////////////////////////////////////
// This method is called by SAPI
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CASRStream::GetFormat(GUID * pFmtId, WAVEFORMATEX ** ppCoMemWaveFormatEx)
{
    m_hCritSec.Lock();

    HRESULT hr = S_OK;

	hr = m_StreamFormat.ParamValidateCopyTo( pFmtId, ppCoMemWaveFormatEx );


    m_hCritSec.Unlock();

    return hr;
}

