#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include "AudioOutputDevicesWin.h"

#include "tier0/memdbgon.h"

namespace
{
template <class T> class CComOwner
{
public:
	CComOwner() : p( NULL ) {}
	~CComOwner() { if ( p ) p->Release(); }
	T *p;
private:
	CComOwner( const CComOwner & );
	CComOwner &operator=( const CComOwner & );
};

class CApartment
{
public:
	CApartment() : result( CoInitializeEx( NULL, COINIT_MULTITHREADED ) ) {}
	~CApartment() { if ( SUCCEEDED( result ) ) CoUninitialize(); }
	// An existing STA is usable too. Never change its mode or balance a failed call.
	bool IsReady() const { return SUCCEEDED( result ) || result == RPC_E_CHANGED_MODE; }
private:
	HRESULT result;
};

bool CopyUTF8( const wchar_t *text, CUtlString &copy )
{
	if ( !text ) return false;
	int size = WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, NULL, 0, NULL, NULL );
	if ( !size ) return false;
	CUtlVector<char> bytes;
	bytes.SetCount( size );
	if ( WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, bytes.Base(), size, NULL, NULL ) != size )
		return false;
	copy = bytes.Base();
	return true;
}

bool ReadEndpoint( IMMDevice *device, AudioOutputChoice_t &choice )
{
	LPWSTR id = NULL;
	HRESULT hr = device->GetId( &id );
	bool valid = SUCCEEDED( hr ) && id && *id && CopyUTF8( id, choice.m_OverrideValue );
	CoTaskMemFree( id );
	if ( !valid ) return false;
	CComOwner<IPropertyStore> properties;
	if ( FAILED( device->OpenPropertyStore( STGM_READ, &properties.p ) ) ) return false;
	PROPVARIANT name;
	PropVariantInit( &name );
	hr = properties.p->GetValue( PKEY_Device_FriendlyName, &name );
	valid = SUCCEEDED( hr ) && name.vt == VT_LPWSTR && name.pwszVal &&
		*name.pwszVal && CopyUTF8( name.pwszVal, choice.m_DisplayName );
	PropVariantClear( &name );
	if ( !valid ) return false;
	DWORD state = 0;
	if ( FAILED( device->GetState( &state ) ) || state != DEVICE_STATE_ACTIVE ) return false;
	choice.m_bAvailable = true;
	return true;
}
}

bool EnumerateAudioOutputDevicesWin( void *, CUtlVector<AudioOutputChoice_t> &choices )
{
	choices.RemoveAll();
	CApartment apartment;
	if ( !apartment.IsReady() ) return false;
	CComOwner<IMMDeviceEnumerator> enumerator;
	if ( FAILED( CoCreateInstance( __uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>( &enumerator.p ) ) ) ) return false;
	CComOwner<IMMDeviceCollection> devices;
	if ( FAILED( enumerator.p->EnumAudioEndpoints( eRender, DEVICE_STATE_ACTIVE, &devices.p ) ) ) return false;
	UINT count = 0;
	if ( FAILED( devices.p->GetCount( &count ) ) ) return false;
	CUtlVector<AudioOutputChoice_t> snapshot;
	for ( UINT i = 0; i < count; ++i )
	{
		CComOwner<IMMDevice> device;
		AudioOutputChoice_t choice;
		if ( FAILED( devices.p->Item( i, &device.p ) ) || !ReadEndpoint( device.p, choice ) ) return false;
		FOR_EACH_VEC( snapshot, j )
			if ( !V_strcmp( snapshot[j].m_OverrideValue.Get(), choice.m_OverrideValue.Get() ) ) return false;
		snapshot.AddToTail( choice );
	}
	// Duplicate names are disambiguated for display only, using the full opaque ID.
	// Never mutate an identifier or rely on the enumeration order for selection.
	FOR_EACH_VEC( snapshot, i )
	{
		AudioOutputChoice_t choice = snapshot[i];
		FOR_EACH_VEC( snapshot, j )
		{
			if ( i != j && !V_strcmp( snapshot[i].m_DisplayName.Get(), snapshot[j].m_DisplayName.Get() ) )
			{
				choice.m_DisplayName += " [";
				choice.m_DisplayName += choice.m_OverrideValue.Get();
				choice.m_DisplayName += "]";
				break;
			}
		}
		choices.AddToTail( choice );
	}
	// Retail build 11138802: empty override clears the explicit preference, but
	// may not reroute until sound is restarted. Do not equate this policy with
	// a particular endpoint or promise live following of the Windows default.
	AudioOutputChoice_t systemDefault;
	systemDefault.m_OverrideValue = "";
	systemDefault.m_DisplayName = "#GameUI_AudioOutputDefault";
	systemDefault.m_bAvailable = count != 0;
	choices.AddToTail( systemDefault );
	return true;
}
#endif
