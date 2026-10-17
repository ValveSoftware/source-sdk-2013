#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <vector>
#include "AudioOutputWatchWin.h"

// Game-thread-only state. A failed unregister leaves ownership uncertain: do
// not register another observer after quarantining the first one.
static bool s_NotificationShutdownFailed = false;
static const char s_NotificationModuleAnchor = 0;

// Callbacks never touch SDK objects, allocate, wait, or execute commands.
class CEndpointNotifications : public IMMNotificationClient
{
public:
 CEndpointNotifications() : references(1), dirty(1) {}
 STDMETHODIMP QueryInterface( REFIID id, void **out )
 {
  if (!out) return E_POINTER;
  *out=NULL;
  if (id==__uuidof(IUnknown) || id==__uuidof(IMMNotificationClient))
  { *out=static_cast<IMMNotificationClient *>(this); AddRef(); return S_OK; }
  return E_NOINTERFACE;
 }
 STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&references); }
 STDMETHODIMP_(ULONG) Release() { ULONG n=InterlockedDecrement(&references); if(!n) delete this; return n; }
 STDMETHODIMP OnDeviceStateChanged(LPCWSTR,DWORD) { InterlockedExchange(&dirty,1); return S_OK; }
 STDMETHODIMP OnDeviceAdded(LPCWSTR) { InterlockedExchange(&dirty,1); return S_OK; }
 STDMETHODIMP OnDeviceRemoved(LPCWSTR) { InterlockedExchange(&dirty,1); return S_OK; }
 STDMETHODIMP OnDefaultDeviceChanged(EDataFlow,ERole,LPCWSTR) { return S_OK; }
 STDMETHODIMP OnPropertyValueChanged(LPCWSTR,const PROPERTYKEY) { return S_OK; }
 bool Poll() { return InterlockedExchange(&dirty,0)!=0; }
private:
 volatile LONG references, dirty;
};

struct CAudioOutputWatchWin::State
{
 IMMDeviceEnumerator *enumerator;
 CEndpointNotifications *notifications;
 HMODULE module;
 bool balanceCOM;
};
CAudioOutputWatchWin::CAudioOutputWatchWin() : m_State(NULL) {}
CAudioOutputWatchWin::~CAudioOutputWatchWin() { Stop(); }
bool CAudioOutputWatchWin::Start()
{
 if(m_State) return true;
 if(s_NotificationShutdownFailed) return false;
 HRESULT hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);
 if(FAILED(hr) && hr!=RPC_E_CHANGED_MODE) return false;
 bool balance=SUCCEEDED(hr);
 IMMDeviceEnumerator *enumerator=NULL;
 hr=CoCreateInstance(__uuidof(MMDeviceEnumerator),NULL,CLSCTX_INPROC_SERVER,
  __uuidof(IMMDeviceEnumerator),reinterpret_cast<void **>(&enumerator));
 if(FAILED(hr)) { if(balance) CoUninitialize(); return false; }
 // Hold the callback's code as well as its object until unregistration succeeds.
 // Acquire before registration so failure containment cannot itself fail later.
 HMODULE module=NULL;
 if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
  &s_NotificationModuleAnchor,&module))
 { enumerator->Release(); if(balance) CoUninitialize(); return false; }
 CEndpointNotifications *notifications=new CEndpointNotifications;
 hr=enumerator->RegisterEndpointNotificationCallback(notifications);
 if(FAILED(hr)) { notifications->Release(); enumerator->Release(); if(balance) CoUninitialize(); FreeLibrary(module); return false; }
 // Registration does not own a reference; retain ours until unregister.
 m_State=new State;
 m_State->enumerator=enumerator; m_State->notifications=notifications; m_State->balanceCOM=balance;
 m_State->module=module;
 return true;
}
void CAudioOutputWatchWin::Stop()
{
 if(!m_State) return;
 State *state=m_State;
 m_State=NULL;
 HRESULT hr=state->enumerator->UnregisterEndpointNotificationCallback(state->notifications);
 if(FAILED(hr))
 {
  // No failure result guarantees that callbacks have stopped. Retain the
  // callback, enumerator, apartment initialization and module reference until
  // process exit. Callbacks only touch their own atomic fields, never this
  // owner. Intentionally do not retry or uninitialize COM on this path.
  s_NotificationShutdownFailed=true;
  return;
 }
 state->enumerator->Release();
 state->notifications->Release();
 bool balance=state->balanceCOM;
 HMODULE module=state->module;
 delete state;
 if(balance) CoUninitialize();
 // Stop is called by the game while its client-module reference is still held.
 FreeLibrary(module);
}
bool CAudioOutputWatchWin::Poll() { return m_State && m_State->notifications->Poll(); }
bool CAudioOutputWatchWin::Query(const char *id,bool &active)
{
 if(!m_State || !id || !*id) return false;
 int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,id,-1,NULL,0);
 if(!size) return false;
 std::vector<wchar_t> wide(size);
 if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,id,-1,&wide[0],size)!=size) return false;
 IMMDevice *device=NULL;
 HRESULT hr=m_State->enumerator->GetDevice(&wide[0],&device);
 if(hr==HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) { active=false; return true; }
 if(FAILED(hr)) return false;
 IMMEndpoint *endpoint=NULL;
 hr=device->QueryInterface(__uuidof(IMMEndpoint),reinterpret_cast<void **>(&endpoint));
 if(FAILED(hr)) { device->Release(); return false; }
 EDataFlow flow=eAll; hr=endpoint->GetDataFlow(&flow); endpoint->Release();
 if(FAILED(hr) || flow!=eRender) { device->Release(); return false; }
 DWORD state=0; hr=device->GetState(&state); device->Release();
 if(FAILED(hr)) return false;
 active=state==DEVICE_STATE_ACTIVE;
 return true;
}
#endif
