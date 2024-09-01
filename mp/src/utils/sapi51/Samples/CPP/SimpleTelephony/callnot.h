/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    callnot.h

Abstract:
    
    Declaration of the CTAPIEventNotification object

--*/

#ifndef __TAPIEventNotification_H__
#define __TAPIEventNotification_H__

#define WM_PRIVATETAPIEVENT   WM_USER+101

/////////////////////////////////////////////////////////////////////////////
// CTAPIEventNotification
class CTAPIEventNotification : public ITTAPIEventNotification
{
private:
    // Refcount for COM
    LONG       m_dwRefCount;

    // Member data
    HWND       m_hWnd;

public:

    // CTAPIEventNotification implements ITTAPIEventNotification
    //  Declare ITTAPIEventNotification methods here
    HRESULT STDMETHODCALLTYPE Event(
                                    TAPI_EVENT TapiEvent,
                                    IDispatch * pEvent
                                   );

    
// other COM stuff:
public:

    // constructor
    CTAPIEventNotification() {}
    // destructor
    ~CTAPIEventNotification(){}

    // initialization function
    // this stuff could also be done in the
    // constructor
    HRESULT Initialize( )
    {
        m_dwRefCount = 1;
        return S_OK;
    }

    void Shutdown()
    {
    }

    void SetHWND( HWND hWnd )
    {
        m_hWnd = hWnd;
    }

    // IUnknown implementation
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
    {
        if (iid == IID_ITTAPIEventNotification)
        {
            AddRef();
            *ppvObject = (void *)this;
            return S_OK;
        }

        if (iid == IID_IUnknown)
        {
            AddRef();
            *ppvObject = (void *)this;
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    //
    // reference counting needs to be thread safe
    //
    
    ULONG STDMETHODCALLTYPE AddRef()
    {
        ULONG l = InterlockedIncrement(&m_dwRefCount);
        return l;
    }
    
	ULONG STDMETHODCALLTYPE Release()
    {
        ULONG l = InterlockedDecrement(&m_dwRefCount);

        if ( 0 == l)
        {
            delete this;
        }
        
        return l;
    }


};

#endif //__TAPIEventNotification_H__


