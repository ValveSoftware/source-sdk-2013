//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Force feeback OS level handlers
//
//=============================================================================
#include <windows.h>
#include "basehandle.h"
#include "utlvector.h"
#include "usercmd.h"
#include "cdll_client_int.h"
#include "cdll_util.h"
#include "input.h"
#include "convar.h"
#include "tier0/icommandline.h"
#include "forcefeedback.h"
#include "filesystem.h"

#define DIRECTINPUT_VERSION 0x0800

#include "dinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar ff_autocenter( "ff_autocenter", "1", FCVAR_ARCHIVE, "Enable auto-centering of forcefeedback joystick." );

struct ForceFeedbackParams_t
{
	bool					m_bForceFeedbackAvailable;

	LPDIRECTINPUT8			m_pIInput;
	LPDIRECTINPUTDEVICE8	m_pIJoystick;
	bool					m_bPaused;
};

typedef CUtlVector< LPDIRECTINPUTEFFECT	 > vecEffectPtr_t;

class EffectMap_t
{
public:
	FORCEFEEDBACK_t			effect;
	char const				*effectfile;
	vecEffectPtr_t			*pVecEffectPtr;
	bool					m_bDownloaded;
};

static EffectMap_t g_EffectMap[] =
{
	{ FORCE_FEEDBACK_SHOT_SINGLE, "scripts/forcefeedback/singleshot.ffe" },
	{ FORCE_FEEDBACK_SHOT_DOUBLE, "scripts/forcefeedback/doubleshot.ffe" },
	{ FORCE_FEEDBACK_TAKEDAMAGE, "scripts/forcefeedback/takedamage.ffe" },
	{ FORCE_FEEDBACK_SCREENSHAKE, "scripts/forcefeedback/screenshake.ffe" },
	{ FORCE_FEEDBACK_SKIDDING, "scripts/forcefeedback/skidding.ffe" },
	{ FORCE_FEEDBACK_BREAKING, "scripts/forcefeedback/breaking.ffe" },
};

static void InitEffectMap()
{
	int c = ARRAYSIZE( g_EffectMap );
	for ( int i = 0; i < c; ++i )
	{
		g_EffectMap[ i ].pVecEffectPtr = new vecEffectPtr_t();
	}
}
static void ShutdownEffectMap()
{
	int c = ARRAYSIZE( g_EffectMap );
	for ( int i = 0; i < c; ++i )
	{
		if ( g_EffectMap[ i ].pVecEffectPtr )
		{
			g_EffectMap[ i ].pVecEffectPtr->RemoveAll();
			delete g_EffectMap[ i ].pVecEffectPtr;
		}
		g_EffectMap[ i ].pVecEffectPtr = NULL;
	}
}

static void ReportCap( int flags, int bits, char const *desc )
{
	if ( flags & bits )
	{
		DevMsg( "%s\n", desc );
	}
}

static void ReportDevType( DWORD devType )
{
	byte baseType = GET_DIDEVICE_TYPE( devType );
	byte subType = GET_DIDEVICE_SUBTYPE( devType );

	switch ( baseType )
	{
	default:
		DevMsg( "unknown type\n" );
		break;
	case DI8DEVTYPE_DEVICE:
		DevMsg( "DEVICE\n" );
		break;
	case DI8DEVTYPE_MOUSE:
		DevMsg( "MOUSE\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEMOUSE_UNKNOWN:
			DevMsg( "DI8DEVTYPEMOUSE_UNKNOWN\n" );
			break;
		case DI8DEVTYPEMOUSE_TRADITIONAL:
			DevMsg( "DI8DEVTYPEMOUSE_TRADITIONAL\n" );
			break;
		case DI8DEVTYPEMOUSE_FINGERSTICK:
			DevMsg( "DI8DEVTYPEMOUSE_FINGERSTICK\n" );
			break;
		case DI8DEVTYPEMOUSE_TOUCHPAD:
			DevMsg( "DI8DEVTYPEMOUSE_TOUCHPAD\n" );
			break;
		case DI8DEVTYPEMOUSE_TRACKBALL:
			DevMsg( "DI8DEVTYPEMOUSE_TRACKBALL\n" );
			break;
		case DI8DEVTYPEMOUSE_ABSOLUTE:
			DevMsg( "DI8DEVTYPEMOUSE_ABSOLUTE\n" );
			break;
		}
		break;
	case DI8DEVTYPE_KEYBOARD:
		DevMsg( "KEYBOARD\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEKEYBOARD_UNKNOWN:
			DevMsg( "DI8DEVTYPEKEYBOARD_UNKNOWN\n" );
			break;
		case DI8DEVTYPEKEYBOARD_PCXT:
			DevMsg( "DI8DEVTYPEKEYBOARD_PCXT\n" );
			break;
		case DI8DEVTYPEKEYBOARD_OLIVETTI:
			DevMsg( "DI8DEVTYPEKEYBOARD_OLIVETTI\n" );
			break;
		case DI8DEVTYPEKEYBOARD_PCAT:
			DevMsg( "DI8DEVTYPEKEYBOARD_PCAT\n" );
			break;
		case DI8DEVTYPEKEYBOARD_PCENH:
			DevMsg( "DI8DEVTYPEKEYBOARD_PCENH:\n" );
			break;
		case DI8DEVTYPEKEYBOARD_NOKIA1050:
			DevMsg( "DI8DEVTYPEKEYBOARD_NOKIA1050\n" );
			break;
		case DI8DEVTYPEKEYBOARD_NOKIA9140:
			DevMsg( "DI8DEVTYPEKEYBOARD_NOKIA9140\n" );
			break;
		case DI8DEVTYPEKEYBOARD_NEC98:
			DevMsg( "DI8DEVTYPEKEYBOARD_NEC98\n" );
			break;
		case DI8DEVTYPEKEYBOARD_NEC98LAPTOP:
			DevMsg( "DI8DEVTYPEKEYBOARD_NEC98LAPTOP\n" );
			break;
		case DI8DEVTYPEKEYBOARD_NEC98106:
			DevMsg( "DI8DEVTYPEKEYBOARD_NEC98106\n" );
			break;
		case DI8DEVTYPEKEYBOARD_JAPAN106:
			DevMsg( "DI8DEVTYPEKEYBOARD_JAPAN106\n" );
			break;
		case DI8DEVTYPEKEYBOARD_JAPANAX:
			DevMsg( "DI8DEVTYPEKEYBOARD_JAPANAX\n" );
			break;
		case DI8DEVTYPEKEYBOARD_J3100:
			DevMsg( "DI8DEVTYPEKEYBOARD_J3100\n" );
			break;
		}
		break;
	case DI8DEVTYPE_JOYSTICK:
		DevMsg( "JOYSTICK\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEJOYSTICK_LIMITED :
			DevMsg( "DI8DEVTYPEJOYSTICK_LIMITED\n" );
			break;
		case DI8DEVTYPEJOYSTICK_STANDARD:
			DevMsg( "DI8DEVTYPEJOYSTICK_STANDARD\n" );
			break;
		}
		break;
	case DI8DEVTYPE_GAMEPAD:
		DevMsg( "GAMEPAD\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEGAMEPAD_LIMITED:
			DevMsg( "DI8DEVTYPEGAMEPAD_LIMITED\n" );
			break;
		case DI8DEVTYPEGAMEPAD_STANDARD:
			DevMsg( "DI8DEVTYPEGAMEPAD_STANDARD\n" );
			break;
		case DI8DEVTYPEGAMEPAD_TILT:
			DevMsg( "DI8DEVTYPEGAMEPAD_TILT\n" );
			break;
		}
		break;
	case DI8DEVTYPE_DRIVING:
		DevMsg( "DRIVING\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEDRIVING_LIMITED:
			DevMsg( "DI8DEVTYPEDRIVING_LIMITED\n" );
			break;
		case DI8DEVTYPEDRIVING_COMBINEDPEDALS:
			DevMsg( "DI8DEVTYPEDRIVING_COMBINEDPEDALS\n" );
			break;
		case DI8DEVTYPEDRIVING_DUALPEDALS:
			DevMsg( "DI8DEVTYPEDRIVING_DUALPEDALS\n" );
			break;
		case DI8DEVTYPEDRIVING_THREEPEDALS:
			DevMsg( "DI8DEVTYPEDRIVING_THREEPEDALS\n" );
			break;
		case DI8DEVTYPEDRIVING_HANDHELD:
			DevMsg( "DI8DEVTYPEDRIVING_HANDHELD\n" );
			break;
		}
		break;
	case DI8DEVTYPE_FLIGHT:
		DevMsg( "FLIGHT\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEFLIGHT_LIMITED:
			DevMsg( "DI8DEVTYPEFLIGHT_LIMITED\n" );
			break;
		case DI8DEVTYPEFLIGHT_STICK:
			DevMsg( "DI8DEVTYPEFLIGHT_STICK\n" );
			break;
		case DI8DEVTYPEFLIGHT_YOKE:
			DevMsg( "DI8DEVTYPEFLIGHT_YOKE\n" );
			break;
		case DI8DEVTYPEFLIGHT_RC:
			DevMsg( "DI8DEVTYPEFLIGHT_RC\n" );
			break;
		}
		break;
	case DI8DEVTYPE_1STPERSON:
		DevMsg( "1STPERSON\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPE1STPERSON_LIMITED:
			DevMsg( "DI8DEVTYPE1STPERSON_LIMITED\n" );
			break;
		case DI8DEVTYPE1STPERSON_UNKNOWN:
			DevMsg( "DI8DEVTYPE1STPERSON_UNKNOWN\n" );
			break;
		case DI8DEVTYPE1STPERSON_SIXDOF:
			DevMsg( "DI8DEVTYPE1STPERSON_SIXDOF\n" );
			break;
		case DI8DEVTYPE1STPERSON_SHOOTER:
			DevMsg( "DI8DEVTYPE1STPERSON_SHOOTER\n" );
			break;
		}
		break;
	case DI8DEVTYPE_DEVICECTRL:
		DevMsg( "DEVICECTRL\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEDEVICECTRL_UNKNOWN:
			DevMsg( "DI8DEVTYPEDEVICECTRL_UNKNOWN\n" );
			break;
		case DI8DEVTYPEDEVICECTRL_COMMSSELECTION:
			DevMsg( "DI8DEVTYPEDEVICECTRL_COMMSSELECTION\n" );
			break;
		case DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED:
			DevMsg( "DI8DEVTYPEDEVICECTRL_COMMSSELECTION_HARDWIRED\n" );
			break;
		}
		break;
	case DI8DEVTYPE_SCREENPOINTER:
		DevMsg( "SCREENPOINTER\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPESCREENPTR_UNKNOWN:
			DevMsg( "DI8DEVTYPESCREENPTR_UNKNOWN\n" );
			break;
		case DI8DEVTYPESCREENPTR_LIGHTGUN:
			DevMsg( "DI8DEVTYPESCREENPTR_LIGHTGUN\n" );
			break;
		case DI8DEVTYPESCREENPTR_LIGHTPEN:
			DevMsg( "DI8DEVTYPESCREENPTR_LIGHTPEN\n" );
			break;
		case DI8DEVTYPESCREENPTR_TOUCH:
			DevMsg( "DI8DEVTYPESCREENPTR_TOUCH\n" );
			break;
		}
		break;
	case DI8DEVTYPE_REMOTE:
		DevMsg( "REMOTE\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPEREMOTE_UNKNOWN:
			DevMsg( "DI8DEVTYPEREMOTE_UNKNOWN\n" );
			break;
		}
		break;
	case DI8DEVTYPE_SUPPLEMENTAL:
		DevMsg( "SUPPLEMENTAL\n" );
		switch ( subType )
		{
		default:
			break;
		case DI8DEVTYPESUPPLEMENTAL_UNKNOWN:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_UNKNOWN\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_2NDHANDCONTROLLER:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_2NDHANDCONTROLLER\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_HEADTRACKER:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_HEADTRACKER\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_HANDTRACKER:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_HANDTRACKER\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_SHIFTSTICKGATE:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_SHIFTSTICKGATE\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_SHIFTER:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_SHIFTER\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_THROTTLE:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_THROTTLE\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_SPLITTHROTTLE:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_SPLITTHROTTLE\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_COMBINEDPEDALS:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_COMBINEDPEDALS\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_DUALPEDALS:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_DUALPEDALS\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_THREEPEDALS:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_THREEPEDALS\n" );
			break;
		case DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS:
			DevMsg( "DI8DEVTYPESUPPLEMENTAL_RUDDERPEDALS\n" );
			break;
		}
		break;
	}
}

static void DescribeFFDevice( const DIDEVCAPS& caps )
{
	ReportCap( caps.dwFlags, DIDC_ALIAS, "  DIDC_ALIAS" );
	ReportCap( caps.dwFlags, DIDC_ATTACHED, "  device is attached" );
	ReportCap( caps.dwFlags, DIDC_DEADBAND, "  device supports deadband" );
	//ReportCap( caps.dwFlags, DIDC_EMULATED, "  device is emulated" );
	ReportCap( caps.dwFlags, DIDC_FFFADE, "  device supports fade" );
	ReportCap( caps.dwFlags, DIDC_FFATTACK, "  device supports attack" );
	ReportCap( caps.dwFlags, DIDC_HIDDEN, "  DIDC_HIDDEN" );
	ReportCap( caps.dwFlags, DIDC_PHANTOM, "  DIDC_PHANTOM" );
	ReportCap( caps.dwFlags, DIDC_POLLEDDATAFORMAT, "  device using polled data format" );
	ReportCap( caps.dwFlags, DIDC_POLLEDDEVICE, "  device is polled" );
	ReportCap( caps.dwFlags, DIDC_POSNEGCOEFFICIENTS, "  device supports two coefficient values for conditions" );
	ReportCap( caps.dwFlags, DIDC_POSNEGSATURATION, "  DIDC_POSNEGSATURATION" );
	ReportCap( caps.dwFlags, DIDC_SATURATION, "  device supports saturation" );
	ReportCap( caps.dwFlags, DIDC_STARTDELAY, "  device supports start delay" );

	ReportDevType( caps.dwDevType );

	DevMsg( "  %u buttons\n", caps.dwButtons );
	DevMsg( "  %u axes\n", caps.dwAxes );
	DevMsg( "  %u POVs\n", caps.dwPOVs );

	DevMsg( "  %.1f msec FF sample period\n", (float)caps.dwFFSamplePeriod/1000.0f );
	DevMsg( "  %.1f msec FF min time resolution period\n", (float)caps.dwFFMinTimeResolution/1000.0f );
}

struct LoadEffectContext_t
{
	LPDIRECTINPUTDEVICE8	device;
	EffectMap_t				*map;
};

static BOOL CALLBACK EnumEffectsInFileProc(LPCDIFILEEFFECT lpdife, LPVOID pvRef)

{
 	LoadEffectContext_t *ctx = ( LoadEffectContext_t * )pvRef;

    EffectMap_t *map = ctx->map;


	vecEffectPtr_t *vecPtr = map->pVecEffectPtr;
	Assert( vecPtr );

	int idx = vecPtr->AddToTail( NULL );

    HRESULT     hr;
	hr = ctx->device->CreateEffect
		(
			lpdife->GuidEffect, 
            lpdife->lpDiEffect,
            &(*vecPtr)[ idx ], 
            NULL
		);

    if ( FAILED ( hr ) )
    {
        // Error handling
		Msg( "EnumEffectsInFileProc during effect loading for %s\n", map->effectfile );
    }

	return DIENUM_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : device - 
//			&map - 
// Output : static void
//-----------------------------------------------------------------------------
static void LoadEffectFile( LPDIRECTINPUTDEVICE8 device, EffectMap_t &map )
{
	LoadEffectContext_t context;
	context.device = device;
	context.map = &map;

	// Pull out of .gcf if needed
	filesystem->GetLocalCopy( map.effectfile );

	char fullpath[ 512 ];
	filesystem->GetLocalPath( map.effectfile, fullpath, sizeof( fullpath ) );

	HRESULT hr = device->EnumEffectsInFile
		( fullpath, 
          EnumEffectsInFileProc,
          (LPVOID)&context, 
          DIFEF_MODIFYIFNEEDED );

	if ( FAILED( hr ) )
	{
		Msg( "EnumEffectsInFile failed for %s\n", map.effectfile );
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : device - 
// Output : static void
//-----------------------------------------------------------------------------
static void LoadEffectFiles( LPDIRECTINPUTDEVICE8 device )
{
	int c = ARRAYSIZE( g_EffectMap );
	for ( int i = 0; i < c; ++i )
	{
		EffectMap_t& map = g_EffectMap[ i ];

		LoadEffectFile( device, map );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Init_ForceFeedback 
//-----------------------------------------------------------------------------
void CInput::Init_ForceFeedback() 
{ 
	// abort startup if user requests no joystick
	if ( CommandLine()->FindParm("-noff" ) ) 
	{
		return; 
	}
 
	Assert( !m_pFF );

	m_pFF = new ForceFeedbackParams_t;
	Assert( m_pFF );
	Q_memset( m_pFF, 0, sizeof( *m_pFF ) );

	HRESULT hr = DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, 
        IID_IDirectInput8, (void**)&m_pFF->m_pIInput, NULL ); 

	if ( FAILED( hr ) )
	{
		return;
	}

	hr = m_pFF->m_pIInput->CreateDevice(GUID_Joystick, &m_pFF->m_pIJoystick, NULL );
	if ( FAILED( hr ) )
	{
		return;
	}

	hr = m_pFF->m_pIJoystick->SetDataFormat(&c_dfDIJoystick2 );

	if ( FAILED( hr ) )
	{
		return;
	}
	
	HWND mainWnd = (HWND)g_pEngineWindow->GetWindowHandle();

	hr = m_pFF->m_pIJoystick->SetCooperativeLevel( mainWnd, DISCL_BACKGROUND | DISCL_EXCLUSIVE );

	if ( FAILED( hr ) )
	{
		return;
	}

	DIPROPDWORD dwd;
	//The following code turns the center spring off
	dwd.diph.dwSize       = sizeof(DIPROPDWORD);
	dwd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dwd.diph.dwObj        = 0;
	dwd.diph.dwHow        = DIPH_DEVICE;
	dwd.dwData            = FALSE;

	if ( !ff_autocenter.GetBool() )
	{
		hr = m_pFF->m_pIJoystick->SetProperty( DIPROP_AUTOCENTER, &dwd.diph );
		if ( FAILED( hr ) )
		{
			return;
		}
	}

    // Acquire the device
	hr = m_pFF->m_pIJoystick->Acquire();

    if( FAILED( hr ) )
	{
        return;
	}

	DIDEVCAPS diDevCaps;
	Q_memset( &diDevCaps, 0, sizeof( diDevCaps ) );
	diDevCaps.dwSize = sizeof( diDevCaps );

	hr = m_pFF->m_pIJoystick->GetCapabilities( &diDevCaps );

	if ( FAILED( hr ) )
	{
		DevMsg( "GetCapabilities failed\n" );
		return;
	}

	if ( !( diDevCaps.dwFlags & DIDC_FORCEFEEDBACK ) )
	{
		// Doesn't support FF
		return;
	}

	DIDEVICEINSTANCE diDI;
	Q_memset( &diDI, 0, sizeof( diDI ) );
	diDI.dwSize = sizeof( diDI );

	hr = m_pFF->m_pIJoystick->GetDeviceInfo( &diDI );
	if ( FAILED( hr ) )
	{
		DevMsg( "GetDeviceInfo failed\n" );
		return;
	}

	DevMsg( "Forcefeedback device found:\n" ); 

	//DevMsg( "  device '%s'\n", diDI.tszInstanceName );
	DevMsg( "  product '%s'\n", diDI.tszProductName );

	DescribeFFDevice( diDevCaps );
	
	InitEffectMap();

	LoadEffectFiles( m_pFF->m_pIJoystick );

	m_pFF->m_bForceFeedbackAvailable = true; 
}

//-----------------------------------------------------------------------------
// Purpose: Shutdown_ForceFeedback
//-----------------------------------------------------------------------------
void CInput::Shutdown_ForceFeedback()
{
	HRESULT hr;

	Assert( m_pFF );

	if ( !m_pFF )
	{
		return;
	}

	if ( m_pFF->m_bForceFeedbackAvailable )
	{
		m_pFF->m_bForceFeedbackAvailable = false;

		ShutdownEffectMap();

		// Do cleanup
		if ( m_pFF->m_pIJoystick )
		{
			hr = m_pFF->m_pIJoystick->Unacquire();
			if ( FAILED( hr ) )
			{
				DevMsg( "Forcefeedback Unacquire failed\n" );
			}

			hr = m_pFF->m_pIJoystick->Release();
			if ( FAILED( hr ) )
			{
				DevMsg( "Forcefeedback Release failed\n" );
			}
		}

		if ( m_pFF->m_pIInput )
		{
			hr = m_pFF->m_pIInput->Release();
			if ( FAILED( hr ) )
			{
				DevMsg( "DirectInput Release failed\n" );
			}
		}
	}

	delete m_pFF;
	m_pFF = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Reaquire()
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	HRESULT hr = m_pFF->m_pIJoystick->Acquire();
	if ( FAILED( hr ) )
	{
		DevMsg( "ForceFeedback_Reaquire failed\n" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Certain devices require polling periodically
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Think()
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	HRESULT hr = m_pFF->m_pIJoystick->Poll();
	if ( FAILED( hr ) )
	{
		if ( hr == DIERR_INPUTLOST ||
			 hr == DIERR_NOTACQUIRED )
		{
			ForceFeedback_Reaquire();
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_StopAll()
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	HRESULT hr = m_pFF->m_pIJoystick->SendForceFeedbackCommand( DISFFC_STOPALL );
	if ( hr == DIERR_INPUTLOST )
	{
		ForceFeedback_Reaquire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Pause()
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	m_pFF->m_bPaused = true;
	HRESULT hr = m_pFF->m_pIJoystick->SendForceFeedbackCommand( DISFFC_PAUSE );
	if ( hr == DIERR_INPUTLOST )
	{
		ForceFeedback_Reaquire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Resume()
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	if ( !m_pFF->m_bPaused )
		return;

	m_pFF->m_bPaused = false;
	HRESULT hr = m_pFF->m_pIJoystick->SendForceFeedbackCommand( DISFFC_CONTINUE );
	if ( hr == DIERR_INPUTLOST )
	{
		ForceFeedback_Reaquire();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : effectnum - 
//			params - 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Start( int effectnum, const FFBaseParams_t& params )
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;

	// Unpause system...
	if ( m_pFF->m_bPaused )
	{
		ForceFeedback_Resume();
	}

	// look up the effect
	FORCEFEEDBACK_t effect = (FORCEFEEDBACK_t)effectnum;

	if ( effect < 0 || effect >= NUM_FORCE_FEEDBACK_PRESETS )
	{
		Assert( !"ForceFeedback_Start with bogus effectnum" );
		return;
	}

	EffectMap_t *map = &g_EffectMap[ effectnum ];

	vecEffectPtr_t *effects = map->pVecEffectPtr;


	// Play the effects on the device
	int c = effects->Count();
	for ( int i = 0; i < c; ++i )
	{
		LPDIRECTINPUTEFFECT pEffect = (*effects)[ i ];

		if ( !map->m_bDownloaded )
		{
			pEffect->Download();
			map->m_bDownloaded = true;
		}

		DWORD flags = DIEP_DIRECTION | DIEP_GAIN | DIEP_DURATION;

		LONG            rglDirection[2] = { 0, 100 };

		// Fill in parameters
		DIEFFECT effect;
		Q_memset( &effect, 0, sizeof( effect ) );
		effect.dwSize = sizeof( effect );
		effect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
		effect.rglDirection = rglDirection;
		effect.cAxes = 2;

		HRESULT hr = pEffect->GetParameters( &effect, flags );
		if ( !FAILED( hr ) )
		{
			// If params.m_flDuration == 0.0f then that means use the duration in the file
			if ( params.m_flDuration <= -0.999f )
			{
				effect.dwDuration = INFINITE;
			}
			else if( params.m_flDuration >= 0.001f )
			{
				// Convert to microsseconds
				effect.dwDuration = (DWORD)( params.m_flDuration * 1000000.0f );
			}

			effect.dwGain = params.m_flGain * 10000.0f;
			effect.rglDirection[ 0 ] = 100.0f * params.m_flDirection;
			effect.rglDirection[ 1 ] = 0;

			hr = pEffect->SetParameters( &effect, flags );
			if ( !FAILED( hr ) )
			{
				pEffect->Start( 1, params.m_bSolo ? DIES_SOLO : 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : effectnum - 
//-----------------------------------------------------------------------------
void CInput::ForceFeedback_Stop( int effectnum )
{
	if ( !m_pFF || !m_pFF->m_bForceFeedbackAvailable )
		return;


	// look up the effect
	FORCEFEEDBACK_t effect = (FORCEFEEDBACK_t)effectnum;

	if ( effect < 0 || effect >= NUM_FORCE_FEEDBACK_PRESETS )
	{
		Assert( !"ForceFeedback_Stop with bogus effectnum" );
		return;
	}

	EffectMap_t *map = &g_EffectMap[ effectnum ];

	vecEffectPtr_t *effects = map->pVecEffectPtr;

	// Stop the effects on the device
	int c = effects->Count();
	for ( int i = 0; i < c; ++i )
	{
		LPDIRECTINPUTEFFECT pEffect = (*effects)[ i ];
		pEffect->Stop();
	}
}