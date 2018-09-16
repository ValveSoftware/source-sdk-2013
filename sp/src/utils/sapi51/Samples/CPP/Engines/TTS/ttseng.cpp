// TtsEng.cpp : Implementation of DLL Exports.
//Copyright (c) Microsoft Corporation. All rights reserved.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f msttsdrvps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "TtsEng.h"
#include "TtsEng_i.c"
#include "TtsEngObj.h"



CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY( CLSID_SampleTTSEngine   , CTTSEngObj    )
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

#ifdef _WIN32_WCE
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID)
#else
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
#endif
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, (HINSTANCE)hInstance, &LIBID_SAMPLETTSENGLib);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}




