/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    callnot.cpp

Abstract:

    Implementation of the ITTAPIEventNotification interface.

    This is an outgoing interface that is defined by TAPI 3.0.  This
    is basically a callback function that TAPI 3.0 calls to inform
    the application of events related to calls (on a specific address)

    Please refer to COM documentation for information on outgoing
    interfaces.

    An application must implement and register this interface in order
    to receive calls and events related to calls

--*/

#include <tapi3.h>
#include "callnot.h"
                
///////////////////////////////////////////////////////////////////
// CallEventNotification
//
// The only method in the ITCallEventNotification interface.  This gets
// called by TAPI 3.0 when there is a call event to report. This just
// posts the message to our UI thread, so that we do as little as
// possible on TAPI's callback thread.
//
///////////////////////////////////////////////////////////////////

HRESULT
STDMETHODCALLTYPE
CTAPIEventNotification::Event(
                              TAPI_EVENT TapiEvent,
                              IDispatch * pEvent
                             )
{
    //
    // Addref the event so it doesn't go away.
    //

    pEvent->AddRef();

    //
    // Post a message to our own UI thread.
    //

    PostMessage( m_hWnd,
                WM_PRIVATETAPIEVENT,
                (WPARAM) TapiEvent,
                (LPARAM) pEvent
               );

    return S_OK;
}

