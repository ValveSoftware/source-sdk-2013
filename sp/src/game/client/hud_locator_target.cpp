//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: See header file
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud_locator_target.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGUI.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include "iinput.h"
#include "view.h"
#include "hud.h"
#include "hudelement.h"
#include "vgui_int.h"

#include "hud_macros.h"
#include "iclientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define ICON_SIZE			0.04f	// Icons are ScreenWidth() * ICON_SIZE wide.
#define ICON_GAP			5		// Number of pixels between the icon and the text

#define OFFSCREEN_ICON_POSITION_RADIUS 100
#define BUTTON_FONT_HANDLE				m_hCaptionFont

#define ICON_DIST_TOO_FAR	(60.0f * 12.0f)

#define MIN_ICON_ALPHA 0.5
#define MAX_ICON_ALPHA 1

ConVar locator_icon_min_size_non_ss( "locator_icon_min_size_non_ss", "1.0", FCVAR_NONE, "Minimum scale of the icon on the screen" );
ConVar locator_icon_max_size_non_ss( "locator_icon_max_size_non_ss", "1.5", FCVAR_NONE, "Maximum scale of the icon on the screen" );

#define MIN_ICON_SCALE			locator_icon_min_size_non_ss.GetFloat()
#define MAX_ICON_SCALE			locator_icon_max_size_non_ss.GetFloat()

#define LOCATOR_OCCLUSION_TEST_RATE 0.25f

enum
{
	DRAW_ARROW_NO = 0,
	DRAW_ARROW_UP,
	DRAW_ARROW_DOWN,
	DRAW_ARROW_LEFT,
	DRAW_ARROW_RIGHT
};

ConVar locator_fade_time( "locator_fade_time", "0.3", FCVAR_NONE, "Number of seconds it takes for a lesson to fully fade in/out." );
ConVar locator_lerp_speed( "locator_lerp_speed", "5.0f", FCVAR_NONE, "Speed that static lessons move along the Y axis." );
ConVar locator_lerp_rest( "locator_lerp_rest", "2.25f", FCVAR_NONE, "Number of seconds before moving from the center." );
ConVar locator_lerp_time( "locator_lerp_time", "1.75f", FCVAR_NONE, "Number of seconds to lerp before reaching final destination" );
ConVar locator_pulse_time( "locator_pulse_time", "1.0f", FCVAR_NONE, "Number of seconds to pulse after changing icon or position" );
ConVar locator_start_at_crosshair( "locator_start_at_crosshair", "0", FCVAR_NONE, "Start position at the crosshair instead of the top middle of the screen." );

ConVar locator_topdown_style( "locator_topdown_style", "0", FCVAR_NONE, "Topdown games set this to handle distance and offscreen location differently." );

ConVar locator_background_style( "locator_background_style", "0", FCVAR_NONE, "Setting this to 1 will show rectangle backgrounds behind the items word-bubble pointers." );
ConVar locator_background_color( "locator_background_color", "255 255 255 5", FCVAR_NONE, "The default color for the background." );
ConVar locator_background_border_color( "locator_background_border_color", "255 255 255 15", FCVAR_NONE, "The default color for the border." );
ConVar locator_background_thickness_x( "locator_background_thickness_x", "8", FCVAR_NONE, "How many pixels the background borders the left and right." );
ConVar locator_background_thickness_y( "locator_background_thickness_y", "0", FCVAR_NONE, "How many pixels the background borders the top and bottom." );
ConVar locator_background_shift_x( "locator_background_shift_x", "3", FCVAR_NONE, "How many pixels the background is shifted right." );
ConVar locator_background_shift_y( "locator_background_shift_y", "1", FCVAR_NONE, "How many pixels the background is shifted down." );
ConVar locator_background_border_thickness( "locator_background_border_thickness", "3", FCVAR_NONE, "How many pixels the background borders the left and right." );

ConVar locator_target_offset_x( "locator_target_offset_x", "0", FCVAR_NONE, "How many pixels to offset the locator from the target position." );
ConVar locator_target_offset_y( "locator_target_offset_y", "0", FCVAR_NONE, "How many pixels to offset the locator from the target position." );

ConVar locator_text_drop_shadow( "locator_text_drop_shadow", "1", FCVAR_NONE, "If enabled, a drop shadow is drawn behind caption text.  PC only." );
ConVar locator_text_glow( "locator_text_glow", "0", FCVAR_NONE, "If enabled, a glow is drawn behind caption text" );
ConVar locator_text_glow_color( "locator_text_glow_color", "255 255 255 255", FCVAR_NONE, "Color of text glow" );

ConVar locator_split_maxwide_percent( "locator_split_maxwide_percent", "0.80f", FCVAR_CHEAT );
ConVar locator_split_len( "locator_split_len", "0.5f", FCVAR_CHEAT );

#ifdef MAPBASE
extern ConVar gameinstructor_default_bindingcolor;
#endif


//------------------------------------
CLocatorTarget::CLocatorTarget( void )
{
	Deactivate( true );

	PrecacheMaterial("vgui/hud/icon_arrow_left");
	PrecacheMaterial("vgui/hud/icon_arrow_right");
	PrecacheMaterial("vgui/hud/icon_arrow_up");
	PrecacheMaterial("vgui/hud/icon_arrow_down");
	PrecacheMaterial("vgui/hud/icon_arrow_plain");
}

//------------------------------------
void CLocatorTarget::Activate( int serialNumber )
{ 
	m_serialNumber		= serialNumber; 
	m_frameLastUpdated	= gpGlobals->framecount;
	m_isActive			= true;

	m_bVisible			= true;
	m_bOnscreen			= true;
	m_alpha				= 0;
	m_fadeStart			= gpGlobals->curtime;

	m_offsetX			= m_offsetY = 0;

	int iStartX = ScreenWidth() / 2;
	int iStartY = ScreenHeight() / 4;

	// We want to start lessons at the players crosshair, cause that's where they're looking!
	if ( locator_start_at_crosshair.GetBool() )
		vgui::input()->GetCursorPos( iStartX, iStartY );

	m_lastXPos = iStartX;
	m_lastYPos = iStartY;

	m_drawArrowDirection	= DRAW_ARROW_NO;
	m_lerpStart				= gpGlobals->curtime;
	m_pulseStart			= gpGlobals->curtime;
	m_declutterIndex		= 0;
	m_lastDeclutterIndex	= 0;

	AddIconEffects(LOCATOR_ICON_FX_FADE_IN);

#ifdef MAPBASE
	// Mods are capable of using a custom binding color
	CSplitString colorValues( gameinstructor_default_bindingcolor.GetString(), "," );

	int r,g,b;
	r = g = b = 0;

	if (colorValues.Count() == 3)
	{
		r = atoi( colorValues[0] );
		g = atoi( colorValues[1] );
		b = atoi( colorValues[2] );
	}

	m_bindingColor.SetColor( r, g, b, 255 );
#endif
}

//------------------------------------
void CLocatorTarget::Deactivate( bool bNoFade )						
{ 
	if ( bNoFade || m_alpha == 0 || 
		 ( m_bOccluded && !( m_iEffectsFlags & LOCATOR_ICON_FX_FORCE_CAPTION ) ) || 
		 ( !m_bOnscreen && ( m_iEffectsFlags & LOCATOR_ICON_FX_NO_OFFSCREEN ) ) )
	{
		m_bOriginInScreenspace = false;

		m_serialNumber			= -1;
		m_isActive				= false;
		m_frameLastUpdated		= 0;
		m_pIcon_onscreen		= NULL;
		m_pIcon_offscreen		= NULL;
		m_bDrawControllerButton = false;
		m_bDrawControllerButtonOffscreen = false;
		m_iEffectsFlags			= LOCATOR_ICON_FX_NONE;
		m_captionWide			= 0;

		m_pchDrawBindingName	= NULL;
		m_pchDrawBindingNameOffscreen = NULL;
		m_widthScale_onscreen	= 1.0f;
		m_bOccluded				= false;
		m_alpha					= 0;
		m_bIsDrawing			= false;
		m_bVisible				= false;

		m_szVguiTargetName		= "";
		m_szVguiTargetLookup	= "";
		m_hVguiTarget			= NULL;
		m_nVguiTargetEdge		= vgui::Label::a_northwest;

		m_szBinding				= "";
		m_iBindingTick			= 0;
		m_flNextBindingTick		= 0.0f;
		m_flNextOcclusionTest	= 0.0f;
		m_iBindingChoicesCount	= 0;

		m_wszCaption.RemoveAll();
		m_wszCaption.AddToTail( (wchar_t)0 );
	}
	else if ( !( m_iEffectsFlags & LOCATOR_ICON_FX_FADE_OUT ) )
	{
		// Determine home much time it would have spent fading to reach the current alpha
		float flAssumedFadeTime;
		flAssumedFadeTime = ( 1.0f - static_cast<float>( m_alpha ) / 255.0f ) * locator_fade_time.GetFloat();

		// Set the fade
		m_fadeStart = gpGlobals->curtime - flAssumedFadeTime;
		AddIconEffects( LOCATOR_ICON_FX_FADE_OUT );
		RemoveIconEffects( LOCATOR_ICON_FX_FADE_IN );
	}
}

//------------------------------------
void CLocatorTarget::Update()							
{
	m_frameLastUpdated = gpGlobals->framecount;

	if ( m_bVisible && ( m_iEffectsFlags & LOCATOR_ICON_FX_FADE_OUT ) )
	{
		// Determine home much time it would have spent fading to reach the current alpha
		float flAssumedFadeTime;
		flAssumedFadeTime = ( 1.0f - static_cast<float>( m_alpha ) / 255.0f ) * locator_fade_time.GetFloat();

		// Set the fade
		m_fadeStart = gpGlobals->curtime - flAssumedFadeTime;
		AddIconEffects( LOCATOR_ICON_FX_FADE_OUT );
		RemoveIconEffects( LOCATOR_ICON_FX_FADE_OUT );
	}
}

int CLocatorTarget::GetIconX( void )
{
	return m_iconX + ( IsOnScreen() ? locator_target_offset_x.GetInt()+m_offsetX : 0 );
}

int CLocatorTarget::GetIconY( void )
{
	return m_iconY + ( IsOnScreen() ? locator_target_offset_y.GetInt()+m_offsetY : 0 );
}

int CLocatorTarget::GetIconCenterX( void )
{
	return m_centerX + locator_target_offset_x.GetInt() + m_offsetX;
}

int CLocatorTarget::GetIconCenterY( void )
{
	return m_centerY + locator_target_offset_y.GetInt() + m_offsetY;
}

void CLocatorTarget::SetVisible( bool bVisible )
{
	// They are already the same
	if ( m_bVisible == bVisible )
		return;

	m_bVisible = bVisible;

	if ( bVisible )
	{
		// Determine home much time it would have spent fading to reach the current alpha
		float flAssumedFadeTime;
		flAssumedFadeTime = ( static_cast<float>( m_alpha ) / 255.0f ) * locator_fade_time.GetFloat();

		// Set the fade
		m_fadeStart = gpGlobals->curtime - flAssumedFadeTime;
		AddIconEffects( LOCATOR_ICON_FX_FADE_IN );
		RemoveIconEffects( LOCATOR_ICON_FX_FADE_OUT );
	}
	else
	{
		// Determine home much time it would have spent fading to reach the current alpha
		float flAssumedFadeTime;
		flAssumedFadeTime = ( 1.0f - static_cast<float>( m_alpha ) / 255.0f ) * locator_fade_time.GetFloat();

		// Set the fade
		m_fadeStart = gpGlobals->curtime - flAssumedFadeTime;
		AddIconEffects( LOCATOR_ICON_FX_FADE_OUT );
		RemoveIconEffects( LOCATOR_ICON_FX_FADE_IN );
	}
}

bool CLocatorTarget::IsVisible( void )
{
	return m_bVisible;
}

void CLocatorTarget::SetCaptionText( const char *pszText, const char *pszParam )
{
	wchar_t outbuf[ 256 ];
	outbuf[ 0 ] = L'\0';

	if ( pszParam && pszParam[ 0 ] != '\0' )
	{
		wchar_t wszParamBuff[ 128 ];
		wchar_t *pLocalizedParam = NULL;

		if ( pszParam[ 0 ] == '#' )
		{
			pLocalizedParam = g_pVGuiLocalize->Find( pszParam );
		}

		if ( !pLocalizedParam )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszParam, wszParamBuff, sizeof( wszParamBuff ) );
			pLocalizedParam = wszParamBuff;
		}

		wchar_t wszTextBuff[ 128 ];
		wchar_t *pLocalizedText = NULL;

		if ( pszText[ 0 ] == '#' )
			pLocalizedText = g_pVGuiLocalize->Find( pszText );

		if ( !pLocalizedText )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszText, wszTextBuff, sizeof( wszTextBuff ) );
			pLocalizedText = wszTextBuff;
		}

		wchar_t buf[ 256 ];
		g_pVGuiLocalize->ConstructString( buf, sizeof(buf), pLocalizedText, 1, pLocalizedParam );

		UTIL_ReplaceKeyBindings( buf, sizeof( buf ), outbuf, sizeof( outbuf ) );
	}
	else
	{
		wchar_t wszTextBuff[ 128 ];
		wchar_t *pLocalizedText = NULL;

		if ( pszText[ 0 ] == '#' )
		{
			pLocalizedText = g_pVGuiLocalize->Find( pszText );
		}

		if ( !pLocalizedText )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pszText, wszTextBuff, sizeof( wszTextBuff ) );
			pLocalizedText = wszTextBuff;
		}

		wchar_t buf[ 256 ];
		Q_wcsncpy( buf, pLocalizedText, sizeof( buf ) );

		UTIL_ReplaceKeyBindings( buf, sizeof(buf), outbuf, sizeof( outbuf ) );
	}

	int len = wcslen( outbuf ) + 1;
	m_wszCaption.RemoveAll();
	m_wszCaption.EnsureCount( len );
	Q_wcsncpy( m_wszCaption.Base(), outbuf, len * sizeof( wchar_t ) );
}

void CLocatorTarget::SetCaptionColor( const char *pszCaptionColor )
{
	int r,g,b;
	r = g = b = 0;

	CSplitString colorValues( pszCaptionColor, "," );

	if( colorValues.Count() == 3 )
	{
		r = atoi( colorValues[0] );
		g = atoi( colorValues[1] );
		b = atoi( colorValues[2] );

		m_captionColor.SetColor( r,g,b, 255 );
	}
	else
	{
		DevWarning( "caption_color format incorrect. RRR,GGG,BBB expected.\n");
	}
}

bool CLocatorTarget::IsStatic()
{
	return ( ( m_iEffectsFlags & LOCATOR_ICON_FX_STATIC ) || IsPresenting() );
}

bool CLocatorTarget::IsPresenting()
{
	return ( gpGlobals->curtime - m_lerpStart < locator_lerp_rest.GetFloat() );
}

void CLocatorTarget::StartTimedLerp()
{
	if ( gpGlobals->curtime - m_lerpStart > locator_lerp_rest.GetFloat() )
	{
		m_lerpStart = gpGlobals->curtime - locator_lerp_rest.GetFloat();
	}
}

void CLocatorTarget::StartPresent()
{
	m_lerpStart = gpGlobals->curtime;
}


void CLocatorTarget::EndPresent()
{
	if ( gpGlobals->curtime - m_lerpStart < locator_lerp_rest.GetFloat() )
	{
		m_lerpStart = gpGlobals->curtime - locator_lerp_rest.GetFloat();
	}
}

void CLocatorTarget::UpdateVguiTarget( void )
{
	const char *pchVguiTargetName = m_szVguiTargetName.String();

	if ( !pchVguiTargetName || pchVguiTargetName[ 0 ] == '\0' )
	{
		m_hVguiTarget = NULL;
		return;
	}

	// Get the appropriate token based on the binding
	if ( m_iBindingChoicesCount > 0 )
	{
		int nTagetToken = m_iBindChoicesOriginalToken[ m_iBindingTick % m_iBindingChoicesCount ];

		for ( int nToken = 0; nToken < nTagetToken && pchVguiTargetName; ++nToken )
		{
			pchVguiTargetName = strchr( pchVguiTargetName, ';' );

			if ( pchVguiTargetName )
			{
				pchVguiTargetName++;
			}
		}

		if ( !pchVguiTargetName || pchVguiTargetName[ 0 ] == '\0' )
		{
			// There wasn't enough tokens, just use the first
			pchVguiTargetName = m_szVguiTargetName.String();
		}
	}

	m_hVguiTarget = g_pClientMode->GetViewport();
}

void CLocatorTarget::SetVguiTargetName( const char *pchVguiTargetName )
{
	if ( Q_strcmp( m_szVguiTargetName.String(), pchVguiTargetName ) == 0 )
		return;

	m_szVguiTargetName = pchVguiTargetName;

	UpdateVguiTarget();
}

void CLocatorTarget::SetVguiTargetLookup( const char *pchVguiTargetLookup )
{
	m_szVguiTargetLookup = pchVguiTargetLookup;
}

void CLocatorTarget::SetVguiTargetEdge( int nVguiEdge )
{
	m_nVguiTargetEdge = nVguiEdge;
}

vgui::Panel *CLocatorTarget::GetVguiTarget( void )
{
	return (vgui::Panel *)m_hVguiTarget.Get();
}

//------------------------------------
void CLocatorTarget::SetOnscreenIconTextureName( const char *pszTexture )
{
	if ( Q_strcmp( m_szOnscreenTexture.String(), pszTexture ) == 0 )
		return;

	m_szOnscreenTexture = pszTexture;
	m_pIcon_onscreen	= NULL; // Dirty the onscreen icon so that the Locator will look up the new icon by name.
	m_pulseStart		= gpGlobals->curtime;
}

//------------------------------------
void CLocatorTarget::SetOffscreenIconTextureName( const char *pszTexture )
{
	if ( Q_strcmp( m_szOffscreenTexture.String(), pszTexture ) == 0 )
		return;

	m_szOffscreenTexture	= pszTexture;
	m_pIcon_offscreen		= NULL; // Ditto
	m_pulseStart			= gpGlobals->curtime;
}

//------------------------------------
void CLocatorTarget::SetBinding( const char *pszBinding )	
{
	int iAllowJoystick = -1;

	/*if ( !IsX360() )
	{
		// Only show joystick binds if it's enabled and non-joystick if it's disabled
		iAllowJoystick = input->ControllerModeActive();
	}*/

	bool bIsControllerNow = ( iAllowJoystick != 0 );

	if ( m_bWasControllerLast == bIsControllerNow )
	{
		// We haven't toggled joystick enabled recently, so if it's the same bind, bail
		if ( Q_strcmp( m_szBinding.String(), pszBinding ) == 0 )
			return;
	}

	m_bWasControllerLast = bIsControllerNow;

	m_szBinding			= pszBinding;
	m_pIcon_onscreen	= NULL; // Dirty the onscreen icon so that the Locator will look up the new icon by name.
	m_pIcon_offscreen	= NULL; // ditto.
	m_flNextBindingTick = gpGlobals->curtime + 0.75f;

	// Get a list of all the keys bound to these actions
	m_iBindingChoicesCount = 0;

	// Tokenize the binding name (could be more than one binding)
	int nOriginalToken		= 0;
	const char	*pchToken	= m_szBinding.String();
	char		szToken[ 128 ];

	pchToken = nexttoken( szToken, pchToken, ';', sizeof( szToken ) );

	while ( pchToken )
	{
		// Get the first parameter
		int iTokenBindingCount = 0;
		const char *pchBinding = engine->Key_LookupBindingExact( szToken );

		while ( m_iBindingChoicesCount < MAX_LOCATOR_BINDINGS_SHOWN && pchBinding )
		{
			m_pchBindingChoices[ m_iBindingChoicesCount ]			= pchBinding;
			m_iBindChoicesOriginalToken[ m_iBindingChoicesCount ]	= nOriginalToken;
			++m_iBindingChoicesCount;
			++iTokenBindingCount;

			pchBinding = engine->Key_LookupBindingExact( szToken );
		}

		nOriginalToken++;
		pchToken = nexttoken( szToken, pchToken, ';', sizeof( szToken ) );
	}

	m_pulseStart = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CLocatorTarget::UseBindingImage( char *pchIconTextureName, size_t bufSize )
{
	if ( m_iBindingChoicesCount <= 0 )
	{
		if ( IsX360() )
		{
			Q_strncpy( pchIconTextureName, "icon_blank", bufSize );
		}
		else
		{
			Q_strncpy( pchIconTextureName, "icon_key_wide", bufSize );
			return "#GameUI_Icons_NONE";
		}

		return NULL;
	}

	// Cycle through the list of binds at a rate of 2 per second
	const char *pchBinding = m_pchBindingChoices[ m_iBindingTick % m_iBindingChoicesCount ];

	// We counted at least one binding... this should not be NULL!
	Assert( pchBinding );

	if ( IsX360() )
	{
		// Use a blank background for the button icons
		Q_strncpy( pchIconTextureName, "icon_blank", bufSize );
		return pchBinding;
	}

	/*if ( input->ControllerModeActive() && 
		 ( Q_strcmp( pchBinding, "A_BUTTON" ) == 0 || 
		   Q_strcmp( pchBinding, "B_BUTTON" ) == 0 || 
		   Q_strcmp( pchBinding, "X_BUTTON" ) == 0 || 
		   Q_strcmp( pchBinding, "Y_BUTTON" ) == 0 || 
		   Q_strcmp( pchBinding, "L_SHOULDER" ) == 0 || 
		   Q_strcmp( pchBinding, "R_SHOULDER" ) == 0 || 
		   Q_strcmp( pchBinding, "L_TRIGGER" ) == 0 || 
		   Q_strcmp( pchBinding, "R_TRIGGER" ) == 0 || 
		   Q_strcmp( pchBinding, "BACK" ) == 0 || 
		   Q_strcmp( pchBinding, "START" ) == 0 || 
		   Q_strcmp( pchBinding, "STICK1" ) == 0 || 
		   Q_strcmp( pchBinding, "STICK2" ) == 0 || 
		   Q_strcmp( pchBinding, "UP" ) == 0 || 
		   Q_strcmp( pchBinding, "DOWN" ) == 0 || 
		   Q_strcmp( pchBinding, "LEFT" ) == 0 || 
		   Q_strcmp( pchBinding, "RIGHT" ) == 0 ) )
	{
		// Use a blank background for the button icons
		Q_strncpy( pchIconTextureName, "icon_blank", bufSize );
		return pchBinding;
	}*/

	if ( Q_strcmp( pchBinding, "MOUSE1" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_mouseLeft", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "MOUSE2" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_mouseRight", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "MOUSE3" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_mouseThree", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "MWHEELUP" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_mouseWheel_up", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "MWHEELDOWN" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_mouseWheel_down", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "UPARROW" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_up", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "LEFTARROW" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_left", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "DOWNARROW" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_down", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "RIGHTARROW" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_right", bufSize );
		return NULL;
	}
	else if ( Q_strcmp( pchBinding, "SEMICOLON" ) == 0 || 
		Q_strcmp( pchBinding, "INS" ) == 0 || 
		Q_strcmp( pchBinding, "DEL" ) == 0 || 
		Q_strcmp( pchBinding, "HOME" ) == 0 || 
		Q_strcmp( pchBinding, "END" ) == 0 || 
		Q_strcmp( pchBinding, "PGUP" ) == 0 || 
		Q_strcmp( pchBinding, "PGDN" ) == 0 || 
		Q_strcmp( pchBinding, "PAUSE" ) == 0 || 
		Q_strcmp( pchBinding, "F10" ) == 0 || 
		Q_strcmp( pchBinding, "F11" ) == 0 || 
		Q_strcmp( pchBinding, "F12" ) == 0 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_generic", bufSize );
		return pchBinding;
	}
	else if ( Q_strlen( pchBinding ) <= 2 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_generic", bufSize );
		return pchBinding;
	}
	else if ( Q_strlen( pchBinding ) <= 6 )
	{
		Q_strncpy( pchIconTextureName, "icon_key_wide", bufSize );
		return pchBinding;
	}
	else
	{
		Q_strncpy( pchIconTextureName, "icon_key_wide", bufSize );
		return pchBinding;
	}

	return pchBinding;
}

//-----------------------------------------------------------------------------
int CLocatorTarget::GetIconWidth( void )
{
	return m_wide;
}

//-----------------------------------------------------------------------------
int CLocatorTarget::GetIconHeight( void )
{
	return m_tall;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLocatorPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CLocatorPanel, vgui::EditablePanel );
public:
	CLocatorPanel( vgui::Panel *parent, const char *name );
	~CLocatorPanel( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void Paint( void );
	void ValidateTexture( int *pTextureID, const char *pszTextureName );
	bool ValidateTargetTextures( CLocatorTarget *pTarget );
	bool IconsAreIntersecting( CLocatorTarget &first, CLocatorTarget &second, int iTolerance );
	virtual void PaintTarget( CLocatorTarget *pTarget );

	void DrawPointerBackground( CLocatorTarget *pTarget, int nPointerX, int nPointerY, int nWide, int nTall, bool bPointer );
	void DrawStaticIcon( CLocatorTarget *pTarget );
	void DrawDynamicIcon( CLocatorTarget *pTarget, bool bDrawCaption, bool bDrawSimpleArrow );
	void DrawIndicatorArrow( int x, int y, int iconWide, int iconTall, int textWidth, int direction );
	void DrawTargetCaption( CLocatorTarget *pTarget, int x, int y, bool bDrawMultiline );
	int  GetScreenWidthForCaption( const wchar_t *pString, vgui::HFont hFont );
	void DrawBindingName( CLocatorTarget *pTarget, const char *pchBindingName, int x, int y, bool bController );
	void ComputeTargetIconPosition( CLocatorTarget *pTarget, bool bSetPosition );
	void CalculateOcclusion( CLocatorTarget *pTarget );

	void DrawSimpleArrow( int x, int y, int iconWide, int iconTall );
	void GetIconPositionForOffscreenTarget( const Vector &vecDelta, float flDist, int *pXPos, int *pYPos );

	CLocatorTarget *GetPointerForHandle( int hTarget );
	int		AddTarget();
	void	RemoveTarget( int hTarget );

	void	GetTargetPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation );

	void	DeactivateAllTargets();
	void	CollectGarbage();

	// Animation
	void	AnimateIconSize( int flags, int *wide, int *tall, float fPulseStart );
	void	AnimateIconPosition( int flags, int *x, int *y );
	void	AnimateIconAlpha( int flags, int *alpha, float fadeStart );

private:

	CPanelAnimationVar( vgui::HFont, m_hCaptionFont, "font", "InstructorTitle" );
	CPanelAnimationVar( vgui::HFont, m_hCaptionFont_ss, "font", "InstructorTitle_ss" );
	CPanelAnimationVar( vgui::HFont, m_hCaptionGlowFont, "font", "InstructorTitleGlow" );
	CPanelAnimationVar( vgui::HFont, m_hCaptionGlowFont_ss, "font", "InstructorTitleGlow_ss" );
	
	CPanelAnimationVar( vgui::HFont, m_hButtonFont, "font", "InstructorButtons" );

	CPanelAnimationVar( vgui::HFont, m_hButtonFont_ss, "font", "InstructorButtons_ss" );
	CPanelAnimationVar( vgui::HFont, m_hKeysFont, "font", "InstructorKeyBindings" );

	
	CPanelAnimationVar( int, m_iShouldWrapStaticLocators, "WrapStaticLocators", "0" );

	static	int		m_serializer;			// Used to issue unique serial numbers to targets, for use as handles
	int				m_textureID_ArrowRight;
	int				m_textureID_ArrowLeft;
	int				m_textureID_ArrowUp;
	int				m_textureID_ArrowDown;
	int				m_textureID_SimpleArrow;

	int				m_staticIconPosition;// Helps us stack static icons

	CLocatorTarget	m_targets[MAX_LOCATOR_TARGETS];
};

//-----------------------------------------------------------------------------
// Local variables
//-----------------------------------------------------------------------------
static CLocatorPanel *s_pLocatorPanel;

inline CLocatorPanel * GetPlayerLocatorPanel()
{
	//if ( !engine->IsLocalPlayerResolvable() )
		//return NULL;

	Assert( s_pLocatorPanel );
	return s_pLocatorPanel;
}


//-----------------------------------------------------------------------------
// Static variable initialization
//-----------------------------------------------------------------------------
int	CLocatorPanel::m_serializer = 1000;		// Serial numbers start at 1000

//-----------------------------------------------------------------------------
// This is the interface function that other systems use to send us targets
//-----------------------------------------------------------------------------
int Locator_AddTarget()
{
	if( s_pLocatorPanel == NULL )
	{
		// Locator has not been used yet. Construct it.
		CLocatorPanel *pLocator = new CLocatorPanel( g_pClientMode->GetViewport(), "LocatorPanel" );
		vgui::SETUP_PANEL(pLocator);
		pLocator->SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
		pLocator->SetPos( 0, 0 );
		pLocator->SetVisible( true );
		vgui::ivgui()->AddTickSignal( pLocator->GetVPanel() );
	}

	Assert( s_pLocatorPanel != NULL );
	return s_pLocatorPanel ? s_pLocatorPanel->AddTarget() : -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Locator_RemoveTarget( int hTarget )
{
	if ( CLocatorPanel *pPanel = GetPlayerLocatorPanel() )
		pPanel->RemoveTarget( hTarget );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CLocatorTarget *Locator_GetTargetFromHandle( int hTarget )
{
	if ( CLocatorPanel *pPanel = GetPlayerLocatorPanel() )
		return pPanel->GetPointerForHandle( hTarget );
	else
		return NULL;
}

void Locator_ComputeTargetIconPositionFromHandle( int hTarget )
{
	if ( CLocatorPanel *pPanel = GetPlayerLocatorPanel() )
	{
		if ( CLocatorTarget *pTarget = pPanel->GetPointerForHandle( hTarget ) )
		{
			if( !( pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_STATIC ) )
			{
				// It's not presenting in the middle of the screen, so figure out it's position
				pPanel->ComputeTargetIconPosition( pTarget, !pTarget->IsPresenting() );
				pPanel->CalculateOcclusion( pTarget );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLocatorPanel::CLocatorPanel( Panel *parent, const char *name ) : EditablePanel(parent,name)
{
	Assert( s_pLocatorPanel == NULL );
	DeactivateAllTargets();

	s_pLocatorPanel				= this;
	m_textureID_ArrowRight		= -1;
	m_textureID_ArrowLeft		= -1;
	m_textureID_ArrowUp			= -1;
	m_textureID_ArrowDown		= -1;
	m_textureID_SimpleArrow		= -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CLocatorPanel::~CLocatorPanel( void )
{
	Assert( s_pLocatorPanel == this );
	s_pLocatorPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CLocatorPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings("resource/UI/Locator.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLocatorPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	vgui::Panel *pPanel = FindChildByName( "LocatorBG" );

	if ( pPanel )
		pPanel->SetPos( (GetWide() - pPanel->GetWide()) * 0.5, (GetTall() - pPanel->GetTall()) * 0.5 );
}

//-----------------------------------------------------------------------------
// Purpose: Given an offscreen target position, compute the 'compass position'
// so that we can draw an icon on the imaginary circle around the crosshair
// that indicates which way the player should turn to bring the target into view.
//-----------------------------------------------------------------------------
void CLocatorPanel::GetTargetPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition	= MainViewOrigin();
	QAngle playerAngles		= MainViewAngles();

	Vector forward, right, up(0,0,1);
	AngleVectors (playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize(forward);
	CrossProduct( up, forward, right );
	float front = DotProduct(vecDelta, forward);
	float side = DotProduct(vecDelta, right);
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2(*xpos,*ypos) + M_PI;
	*flRotation *= 180 / M_PI;

	float yawRadians = -(*flRotation) * M_PI / 180.0f;
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle, squash Y to make an oval rather than a circle
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * 0.6f * ca));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLocatorPanel::DeactivateAllTargets()
{
	for( int i = 0 ; i < MAX_LOCATOR_TARGETS ; i++ )
		m_targets[ i ].Deactivate( true );
}

//-----------------------------------------------------------------------------
// Purpose: Deactivate any target that has not been updated within several frames
//-----------------------------------------------------------------------------
void CLocatorPanel::CollectGarbage()
{
	for( int i = 0 ; i < MAX_LOCATOR_TARGETS ; i++ )
	{
		if( m_targets[ i ].m_isActive )
		{
			if( gpGlobals->framecount - m_targets[ i ].m_frameLastUpdated > 20 )
				m_targets[ i ].Deactivate();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Provide simple animation by modifying the width and height of the
// icon before it is drawn.
//-----------------------------------------------------------------------------
void CLocatorPanel::AnimateIconSize( int flags, int *wide, int *tall, float fPulseStart )
{
	float flScale	 = MIN_ICON_SCALE;
	float scaleDelta = MAX_ICON_SCALE - MIN_ICON_SCALE;

	float newWide = *wide;
	float newTall = *tall;

	if( flags & LOCATOR_ICON_FX_PULSE_SLOW || gpGlobals->curtime - fPulseStart < locator_pulse_time.GetFloat() )
	{
		flScale += scaleDelta * fabs( sin( ( gpGlobals->curtime - fPulseStart ) * M_PI ) );
	}
	else if( flags & LOCATOR_ICON_FX_PULSE_FAST )
	{
		flScale += scaleDelta * fabs( sin( gpGlobals->curtime * 2 * M_PI ) );
	}
	else if( flags & LOCATOR_ICON_FX_PULSE_URGENT )
	{
		flScale += scaleDelta * fabs( sin( gpGlobals->curtime * 4 * M_PI ) );
	}

	if ( newWide > newTall )
	{
		// Get scale to make width change by only the standard height amount of pixels
		int iHeightDelta = (int)(newTall * flScale - newTall);
		flScale = ( newWide + iHeightDelta ) / newWide;
	}

	newWide = newWide * flScale;
	newTall = newTall * flScale;

	*wide = newWide;
	*tall = newTall;
}

//-----------------------------------------------------------------------------
// Purpose: Modify the alpha of the icon before it is drawn.
//-----------------------------------------------------------------------------
void CLocatorPanel::AnimateIconAlpha( int flags, int *alpha, float fadeStart )
{
	float flScale = MIN_ICON_ALPHA;
	float scaleDelta = MAX_ICON_ALPHA - MIN_ICON_ALPHA;

	if( flags & LOCATOR_ICON_FX_ALPHA_SLOW )
	{
		flScale += scaleDelta * fabs( sin( gpGlobals->curtime * 3 ) );
	}
	else if( flags & LOCATOR_ICON_FX_ALPHA_FAST )
	{
		flScale += scaleDelta * fabs( sin( gpGlobals->curtime * 7 ) );
	}
	else if( flags & LOCATOR_ICON_FX_ALPHA_URGENT )
	{
		flScale += scaleDelta * fabs( sin( gpGlobals->curtime * 10 ) );
	}
	else
	{
		flScale = MAX_ICON_ALPHA;
	}

	if ( flags & LOCATOR_ICON_FX_FADE_OUT )
	{
		flScale *= MAX( 0.0f, ( locator_fade_time.GetFloat() - ( gpGlobals->curtime - fadeStart ) ) / locator_fade_time.GetFloat() );
	}
	else if ( flags & LOCATOR_ICON_FX_FADE_IN )
	{
		flScale *= MAX_ICON_ALPHA - MAX( 0.0f, ( locator_fade_time.GetFloat() - ( gpGlobals->curtime - fadeStart ) ) / locator_fade_time.GetFloat() );
	}

	*alpha = static_cast<int>( 255.0f * flScale );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLocatorPanel::AnimateIconPosition( int flags, int *x, int *y )
{
	int newX = *x;
	int newY = *y;

	if( flags & LOCATOR_ICON_FX_SHAKE_NARROW )
	{
		newX += RandomInt( -2, 2 );
		newY += RandomInt( -2, 2 );
	}
	else if( flags & LOCATOR_ICON_FX_SHAKE_WIDE )
	{
		newX += RandomInt( -5, 5 );
		newY += RandomInt( -5, 5 );
	}

	*x = newX;
	*y = newY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLocatorPanel::OnTick( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLocatorPanel::PaintBackground( void )
{
	return;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLocatorPanel::Paint( void )
{
	ValidateTexture( &m_textureID_ArrowLeft, "vgui/hud/icon_arrow_left" );
	ValidateTexture( &m_textureID_ArrowRight, "vgui/hud/icon_arrow_right" );
	ValidateTexture( &m_textureID_ArrowUp, "vgui/hud/icon_arrow_up" );
	ValidateTexture( &m_textureID_ArrowDown, "vgui/hud/icon_arrow_down" );
	ValidateTexture( &m_textureID_SimpleArrow, "vgui/hud/icon_arrow_plain" );

	// reset the static icon position. This is the y position at which the first
	// static icon will be drawn. This value will be incremented by the height of
	// each static icon drawn, which forces the next static icon to be drawn below
	// the previous one, creating a little fixed, vertical stack below the crosshair.
	m_staticIconPosition = ScreenHeight() / 6;

	// Time now to draw the 'dynamic' icons, the icons which help players locate things
	// in actual world space.

	//----------
	// Batch 1
	// Go through all of the active locator targets and compute where to draw the icons
	// that represent each of them. This builds a poor man's draw list by updating the 
	// m_iconX, m_iconY members of each locator target.
	CUtlVectorFixed< CLocatorTarget *, MAX_LOCATOR_TARGETS > vecValid;

	for( int i = 0 ; i < MAX_LOCATOR_TARGETS ; i++ )
	{
		CLocatorTarget *pLocatorTarget = &(m_targets[ i ]);

		// Reset drawing state for this frame... set back to true when it's finally draws
		pLocatorTarget->m_bIsDrawing = false;

		if ( ( !pLocatorTarget->m_bVisible && !pLocatorTarget->m_alpha ) || !pLocatorTarget->m_isActive )
		{
			// Don't want to be visible and have finished fading
			continue;
		}

		vecValid.AddToTail( pLocatorTarget );

		// This prevents an error that if a locator was fading as the map transitioned
		pLocatorTarget->m_fadeStart = fpmin( pLocatorTarget->m_fadeStart, gpGlobals->curtime ); 

		if( !( pLocatorTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_STATIC ) )
		{
			// It's not presenting in the middle of the screen, so figure out it's position
			ComputeTargetIconPosition( pLocatorTarget, !pLocatorTarget->IsPresenting() );
			CalculateOcclusion( pLocatorTarget );

			pLocatorTarget->m_lastDeclutterIndex = pLocatorTarget->m_declutterIndex;
			pLocatorTarget->m_declutterIndex = 0;
		}
	}

	//----------
	// Batch 2
	// Now that we know where each icon _wants_ to be drawn, we grovel through them and 
	// push apart any icons that are too close to one another. This helps to unclutter
	// the display and ensure the maximum number of legible icons and captions. Obviously
	// this process changes where some icons will be drawn. Bubble sort, but tiny data set.
	int iTolerance = 1.25 * (ScreenWidth() * ICON_SIZE);
	int iterations = 0;// Count iterations, don't go infinite in the event of some weird case.
	bool bStillUncluttering = true;
	static int MAX_UNCLUTTER_ITERATIONS = 10;
	while( iterations < MAX_UNCLUTTER_ITERATIONS && bStillUncluttering )
	{
		iterations++;
		bStillUncluttering = false;

		for( int i = 0 ; i < vecValid.Count() ; ++i )
		{
			CLocatorTarget *pLocatorTarget1 = vecValid[ i ];

			for( int j = i + 1 ; j < vecValid.Count() ; ++j )
			{
				CLocatorTarget *pLocatorTarget2 = vecValid[ j ];

				// Don't attempt to declutter icons if one or both is attempting to fade out
				bool bLocatorsFullyActive = !((pLocatorTarget1->GetIconEffectsFlags()|pLocatorTarget2->GetIconEffectsFlags()) & LOCATOR_ICON_FX_FADE_OUT);

				if ( bLocatorsFullyActive && IconsAreIntersecting( *pLocatorTarget1, *pLocatorTarget2, iTolerance ) )
				{
					// Unclutter. Lift whichever icon is highest a bit higher
					if( pLocatorTarget1->m_iconY < pLocatorTarget2->m_iconY )
					{
						pLocatorTarget1->m_iconY = pLocatorTarget2->m_iconY - iTolerance;
						pLocatorTarget1->m_centerY = pLocatorTarget2->m_centerY - iTolerance;
						pLocatorTarget1->m_declutterIndex -= 1;
					}
					else
					{
						pLocatorTarget2->m_iconY = pLocatorTarget1->m_iconY - iTolerance;
						pLocatorTarget2->m_centerY = pLocatorTarget1->m_centerY - iTolerance;
						pLocatorTarget2->m_declutterIndex -= 1;
					}

					bStillUncluttering = true;
				}
			}
		}
	}

	if( iterations == MAX_UNCLUTTER_ITERATIONS )
	{
		DevWarning( "Game instructor hit MAX_UNCLUTTER_ITERATIONS!\n");
	}

	float flLocatorLerpRest = locator_lerp_rest.GetFloat();
	float flLocatorLerpTime = locator_lerp_time.GetFloat();

	//----------
	// Batch 3
	// Draw each of the icons.
	for( int i = 0 ; i < vecValid.Count() ; i++ )
	{
		CLocatorTarget *pLocatorTarget = vecValid[ i ];
		// Back to lerping for these guys
		if ( pLocatorTarget->m_lastDeclutterIndex != pLocatorTarget->m_declutterIndex )
		{
			// It wants to be popped to another position... do it smoothly
			pLocatorTarget->StartTimedLerp();
		}

		// Lerp to the desired position
		float flLerpTime = gpGlobals->curtime - pLocatorTarget->m_lerpStart;

		if ( flLerpTime >= flLocatorLerpRest && flLerpTime < flLocatorLerpRest + flLocatorLerpTime )
		{
			// Lerp slow to fast
			float fInterp = 1.0f - ( ( flLocatorLerpTime - ( flLerpTime - flLocatorLerpRest ) ) / flLocatorLerpTime );

			// Get our desired position
			float iconX = pLocatorTarget->m_iconX;
			float iconY = pLocatorTarget->m_iconY;

			// Get the distance we need to go to reach it
			float diffX = fabsf( pLocatorTarget->m_iconX - pLocatorTarget->m_lastXPos );
			float diffY = fabsf( pLocatorTarget->m_iconY - pLocatorTarget->m_lastYPos );

			// Go from our current position toward the desired position as quick as the interp allows
			pLocatorTarget->m_iconX = static_cast<int>( Approach( iconX, pLocatorTarget->m_lastXPos, diffX * fInterp ) );
			pLocatorTarget->m_iconY = static_cast<int>( Approach( iconY, pLocatorTarget->m_lastYPos, diffY * fInterp ) );

			// Get how much our position changed and apply it to the center values
			int iOffsetX = pLocatorTarget->m_iconX - iconX;
			int iOffsetY = pLocatorTarget->m_iconY - iconY;

			pLocatorTarget->m_centerX += iOffsetX;
			pLocatorTarget->m_centerY += iOffsetY;

			if ( iOffsetX < 3 && iOffsetY < 3 )
			{
				// Near our target! Stop lerping!
				flLerpTime = flLocatorLerpRest + flLocatorLerpTime;
			}
		}

		PaintTarget( pLocatorTarget );
	}

	CollectGarbage();
}

//-----------------------------------------------------------------------------
// Purpose: A helper function to save on typing. Make sure our texture ID's 
//			stay valid.
//-----------------------------------------------------------------------------
void CLocatorPanel::ValidateTexture( int *pTextureID, const char *pszTextureName )
{
	if( *pTextureID == -1 )
	{
		*pTextureID = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( *pTextureID, pszTextureName, true, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame before painting the targets. Ensures that the
//			target's textures are properly cached.
//-----------------------------------------------------------------------------
bool CLocatorPanel::ValidateTargetTextures( CLocatorTarget *pTarget )
{
	bool bBindingTick = false;

	if ( gpGlobals->curtime >= pTarget->m_flNextBindingTick )
	{
		if ( pTarget->m_iBindingChoicesCount > 1 )
		{
			bBindingTick = true;
			pTarget->m_iBindingTick++;
		}

		pTarget->m_flNextBindingTick = gpGlobals->curtime + 0.75f;

		pTarget->UpdateVguiTarget();
	}

	bool bUsesBinding = ( Q_stricmp( pTarget->GetOnscreenIconTextureName(), "use_binding" ) == 0 );

	if( !pTarget->m_pIcon_onscreen || !pTarget->m_pIcon_offscreen || ( bUsesBinding && bBindingTick ) )
	{
		char szIconTextureName[ 256 ];
		if ( bUsesBinding )
		{
			const char *pchDrawBindingName		= pTarget->UseBindingImage( szIconTextureName, sizeof( szIconTextureName ) );
			pTarget->m_bDrawControllerButton		= ( Q_strcmp( szIconTextureName, "icon_blank" ) == 0 );

			pTarget->DrawBindingName( pchDrawBindingName );
		}
		else
		{
			pTarget->m_bDrawControllerButton = false;
			Q_strcpy( szIconTextureName, pTarget->GetOnscreenIconTextureName() );
			pTarget->DrawBindingName( NULL );
		}

		// This target's texture ID is dirty, meaning the target is about to be drawn
		// for the first time, or about to be drawn for the first time since a texture
		// was changed.
		if ( Q_strlen(szIconTextureName) == 0 )
		{
			DevWarning("Locator Target has no onscreen texture name!\n");
			return false;
		}
		else
		{
			pTarget->m_pIcon_onscreen = HudIcons().GetIcon( szIconTextureName );
			if ( pTarget->m_pIcon_onscreen )
			{
				pTarget->m_widthScale_onscreen = static_cast< float >( pTarget->m_pIcon_onscreen->Width() ) / pTarget->m_pIcon_onscreen->Height();
			}
			else
			{
				pTarget->m_widthScale_onscreen = 1.0f;
			}
		}

		if ( Q_stricmp( pTarget->GetOffscreenIconTextureName() , "use_binding" ) == 0 )
		{
			const char *pchDrawBindingName = pTarget->UseBindingImage( szIconTextureName, sizeof( szIconTextureName ) );
			pTarget->m_bDrawControllerButtonOffscreen = ( Q_strcmp( szIconTextureName, "icon_blank" ) == 0 );

			pTarget->DrawBindingNameOffscreen( pchDrawBindingName );
		}
		else
		{
			pTarget->m_bDrawControllerButtonOffscreen = false;
			Q_strcpy( szIconTextureName, pTarget->GetOffscreenIconTextureName() );
			pTarget->DrawBindingNameOffscreen( NULL );
		}

		if( Q_strlen(szIconTextureName) == 0 )
		{
			if( !pTarget->m_pIcon_onscreen )
			{
				DevWarning("Locator Target has no offscreen texture name and can't fall back!\n");
			}
			else
			{
				// The onscreen texture is valid, so default behavior is to use that.
				pTarget->m_pIcon_offscreen = pTarget->m_pIcon_onscreen;
				const char *pchDrawBindingName = pTarget->DrawBindingName();
				pTarget->DrawBindingNameOffscreen( pchDrawBindingName );
			}
		}
		else
		{
			pTarget->m_pIcon_offscreen = HudIcons().GetIcon( szIconTextureName );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Compute where on the screen to draw the icon for this target.
//-----------------------------------------------------------------------------
void CLocatorPanel::ComputeTargetIconPosition( CLocatorTarget *pTarget, bool bSetPosition )
{
	int iconX;
	int iconY;

	// Measure the delta and the dist from this player to this target.
	Vector vecTarget = pTarget->m_vecOrigin;
	Vector vecDelta = vecTarget - MainViewOrigin();

	if ( pTarget->m_bOriginInScreenspace )
	{
		// Coordinates are already in screenspace
		pTarget->m_distFromPlayer = 0.0f;

		iconX = vecTarget.x * ScreenWidth();
		iconY = vecTarget.y * ScreenHeight();
		pTarget->m_targetX = iconX;
		pTarget->m_targetY = iconY;
	}
	else
	{
		pTarget->m_distFromPlayer = VectorNormalize( vecDelta );

		if ( GetVectorInScreenSpace( vecTarget, iconX, iconY ) )
		{
			// NOTE: GetVectorInScreenSpace returns false in an edge case where the 
			// target is very far off screen... just us the old values
			pTarget->m_targetX = iconX;
			pTarget->m_targetY = iconY;
		}
	}

	pTarget->m_drawArrowDirection = DRAW_ARROW_NO;

	float fTitleSafeInset = ScreenWidth() * 0.075f;

	if( iconX < fTitleSafeInset || iconX > ScreenWidth() - fTitleSafeInset )
	{
		// It's off the screen left or right.
		if ( pTarget->m_bOnscreen && !( pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_NO_OFFSCREEN ) )
		{
			// Back to lerping
			pTarget->StartTimedLerp();
			pTarget->m_pulseStart = gpGlobals->curtime;
		}

		if ( bSetPosition )
		{
			pTarget->m_bOnscreen = false;
		}

		GetIconPositionForOffscreenTarget( vecDelta, pTarget->m_distFromPlayer, &iconX, &iconY );

		Vector vCenter = pTarget->m_vecOrigin;
		if( MainViewRight().Dot( vCenter - MainViewOrigin() ) > 0 )
			pTarget->m_drawArrowDirection = DRAW_ARROW_RIGHT;
		else
			pTarget->m_drawArrowDirection = DRAW_ARROW_LEFT;
	}
	else if( iconY < fTitleSafeInset || iconY > ScreenHeight() - fTitleSafeInset )
	{
		// It's off the screen up or down.
		if ( pTarget->m_bOnscreen && !( pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_NO_OFFSCREEN ) )
		{
			// Back to lerping
			pTarget->StartTimedLerp();
			pTarget->m_pulseStart = gpGlobals->curtime;
		}

		if ( bSetPosition )
		{
			pTarget->m_bOnscreen = false;
		}

		GetIconPositionForOffscreenTarget( vecDelta, pTarget->m_distFromPlayer, &iconX, &iconY );

		Vector vCenter = pTarget->m_vecOrigin;
		if( MainViewUp().Dot( vCenter - MainViewOrigin() ) > 0 )
			pTarget->m_drawArrowDirection = DRAW_ARROW_UP;
		else
			pTarget->m_drawArrowDirection = DRAW_ARROW_DOWN;
	}
	else
	{
		if ( !pTarget->m_bOnscreen && !( pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_NO_OFFSCREEN ) )
		{
			// Back to lerping
			pTarget->StartTimedLerp();
			pTarget->m_pulseStart = gpGlobals->curtime;
		}

		pTarget->m_bOnscreen = true;
	}

	if ( bSetPosition )
	{
		int tall = ScreenWidth() * ICON_SIZE;
		int wide = tall * pTarget->m_widthScale_onscreen;

		// Animate the icon
		AnimateIconSize( pTarget->GetIconEffectsFlags(), &wide, &tall, pTarget->m_pulseStart );
		AnimateIconPosition( pTarget->GetIconEffectsFlags(), &iconX, &iconY );
		AnimateIconAlpha( pTarget->GetIconEffectsFlags(), &pTarget->m_alpha, pTarget->m_fadeStart );

		if( pTarget->m_distFromPlayer > ICON_DIST_TOO_FAR && !locator_topdown_style.GetBool() )
		{
			// Make the icon smaller
			wide = wide >> 1;
			tall = tall >> 1;
		}

		pTarget->m_centerX = iconX;
		pTarget->m_centerY = iconY;

		pTarget->m_iconX = pTarget->m_centerX - ( wide >> 1 );
		pTarget->m_iconY = pTarget->m_centerY - ( tall >> 1 );
		pTarget->m_wide = wide;
		pTarget->m_tall = tall;
	}
}

void CLocatorPanel::CalculateOcclusion( CLocatorTarget *pTarget )
{
	if ( gpGlobals->curtime >= pTarget->m_flNextOcclusionTest )
	{
		pTarget->m_flNextOcclusionTest = gpGlobals->curtime + LOCATOR_OCCLUSION_TEST_RATE;

		// Assume the target is not occluded.
		pTarget->m_bOccluded = false;

		if ( pTarget->m_bOriginInScreenspace )
			return;

		trace_t	tr;
		UTIL_TraceLine( pTarget->m_vecOrigin, MainViewOrigin(), (CONTENTS_SOLID|CONTENTS_MOVEABLE), NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction < 1.0f )
		{
			pTarget->m_bOccluded = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This is not valid until after you have computed the onscreen
//			icon position for each target!
//-----------------------------------------------------------------------------
bool CLocatorPanel::IconsAreIntersecting( CLocatorTarget &first, CLocatorTarget &second, int iTolerance )
{
	if( first.m_bOnscreen != second.m_bOnscreen )
	{
		// We only declutter onscreen icons against other onscreen icons and vice-versa.
		return false;
	}

	if( first.IsStatic() || second.IsStatic() )
	{
		// Static icons don't count.
		return false;
	}

	if( abs(first.GetIconY() - second.GetIconY()) < iTolerance )
	{
		// OK, we need the Y-check first. Now we have to see if these icons and their captions overlap.
		int firstWide = iTolerance + first.m_captionWide;
		int secondWide = iTolerance + second.m_captionWide;
		
		if( abs(first.GetIconX() - second.GetIconX()) < (firstWide + secondWide) / 2 )
		{
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Draw this target on the locator.
//
// IF onscreen and visible, draw no icon, draw no arrows
// IF onscreen and occluded, draw icon transparently, draw no arrows
// IF offscreen, draw icon, draw an arrow indicating the direction to the target
//-----------------------------------------------------------------------------
void CLocatorPanel::PaintTarget( CLocatorTarget *pTarget )
{
	bool bNewTexture = ValidateTargetTextures( pTarget );

	if ( bNewTexture )
	{
		// Refigure the width/height for the new texture
		int tall = ScreenWidth() * ICON_SIZE;
		int wide = tall * pTarget->m_widthScale_onscreen;

		AnimateIconSize( pTarget->GetIconEffectsFlags(), &wide, &tall, pTarget->m_pulseStart );

		pTarget->m_wide = wide;
		pTarget->m_tall = tall;
	}

	// A static icon just draws with other static icons in a stack under the crosshair. 
	// Once displayed, they do not move. The are often used for notifiers.
	if( pTarget->IsStatic() )
	{
		DrawStaticIcon( pTarget );
		return;
	}

	if ( !pTarget->m_bOnscreen && ( pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_NO_OFFSCREEN ) )
	{
		// Doesn't draw when offscreen... reset it's alpha so it has to fade in again
		pTarget->m_fadeStart = gpGlobals->curtime;
		pTarget->m_alpha = 0;
	}
	else
	{
		// Save these coordinates for later lerping
		pTarget->m_lastXPos = pTarget->m_iconX;
		pTarget->m_lastYPos = pTarget->m_iconY;

		// Draw when it's on screen or allowed to draw offscreen
		DrawDynamicIcon( pTarget, pTarget->HasCaptionText(), pTarget->m_bOnscreen );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws the caption-like background with word-bubble style pointer
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawPointerBackground( CLocatorTarget *pTarget, int nPointerX, int nPointerY, int nWide, int nTall, bool bPointer )
{
	if ( locator_background_style.GetInt() == 0 || pTarget->m_alpha == 0 )
		return;

	/*
	int nPosX = pTarget->GetIconX() + locator_background_shift_x.GetInt() - locator_background_thickness_x.GetInt() / 2;
	int nPosY = pTarget->GetIconY() + locator_background_shift_y.GetInt() - locator_background_thickness_y.GetInt() / 2;
	int nBackgroundWide = nWide + locator_background_thickness_x.GetInt();
	int nBackgroundTall = nTall + locator_background_thickness_y.GetInt();

	nPointerX = clamp( nPointerX, -0.5f * ScreenWidth(), ScreenWidth() * 1.5f );
	nPointerY = clamp( nPointerY, -0.5f * ScreenHeight(), ScreenHeight() * 1.5f );

	float fAlpha = static_cast<float>( pTarget->m_alpha ) / 255.0f;

	Color rgbaBackground = locator_background_color.GetColor();
	rgbaBackground[ 3 ] *= fAlpha;

	Color rgbaBorder = locator_background_border_color.GetColor();
	rgbaBorder[ 3 ] *= fAlpha;
	*/

	DevMsg("[TODO] vgui::surface()->DrawWordBubble \n");

	//vgui::surface()->DrawWordBubble( nPosX, nPosY, nPosX + nBackgroundWide, nPosY + nBackgroundTall, locator_background_border_thickness.GetInt(),  rgbaBackground, rgbaBorder, bPointer, nPointerX, nPointerY, ScreenWidth() * ICON_SIZE );
}

//-----------------------------------------------------------------------------
// Purpose: Draw an icon with the group of static icons. 
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawStaticIcon( CLocatorTarget *pTarget )
{
	int centerX = ScreenWidth() / 2;
	int centerY = ScreenHeight() / 2;
	centerY += m_staticIconPosition;

	int iconTall = ScreenWidth() * ICON_SIZE;
	int iconWide = iconTall * pTarget->m_widthScale_onscreen;

	pTarget->m_centerX = centerX;
	pTarget->m_centerY = centerY;

	// Animate the icon
	AnimateIconSize( pTarget->GetIconEffectsFlags(), &iconWide, &iconTall, pTarget->m_pulseStart );
	AnimateIconPosition( pTarget->GetIconEffectsFlags(), &centerX, &centerY );
	AnimateIconAlpha( pTarget->GetIconEffectsFlags(), &pTarget->m_alpha, pTarget->m_fadeStart );

	// Figure out the caption width
	pTarget->m_captionWide = GetScreenWidthForCaption( pTarget->GetCaptionText(), m_hCaptionFont );

	bool bDrawMultilineCaption = false;

	if ( m_iShouldWrapStaticLocators > 0 )	// conditionalized in locator.res
	{
		if ( pTarget->m_captionWide > (  ScreenWidth() * locator_split_maxwide_percent.GetFloat() ) )
		{
			// we will double-line this
			pTarget->m_captionWide = pTarget->m_captionWide * locator_split_len.GetFloat();
			bDrawMultilineCaption = true;
		}
	}
	int totalWide = iconWide + ICON_GAP + pTarget->m_captionWide;
	pTarget->m_iconX = centerX - totalWide * 0.5f;
	pTarget->m_iconY = centerY - ( iconTall >> 1 );

	// Lerp by speed on the Y axis
	float iconY = pTarget->m_iconY;

	float diffY = fabsf( pTarget->m_iconY - pTarget->m_lastYPos );

	float flLerpSpeed = gpGlobals->frametime * locator_lerp_speed.GetFloat();
	pTarget->m_iconY = static_cast<int>( Approach( iconY, pTarget->m_lastYPos, MAX( 3.0f, flLerpSpeed * diffY ) ) );
	pTarget->m_centerY += ( pTarget->m_iconY - iconY );

	pTarget->m_lastXPos = pTarget->m_iconX;
	pTarget->m_lastYPos = pTarget->m_iconY;

	pTarget->m_bIsDrawing = true;

	vgui::Panel *pVguiTarget = pTarget->GetVguiTarget();

	if ( pVguiTarget )
	{
		int nPanelX, nPanelY;
		nPanelX = 0;
		nPanelY = 0;

		vgui::Label::Alignment nVguiTargetEdge = (vgui::Label::Alignment)pTarget->GetVguiTargetEdge();

		int nWide = pVguiTarget->GetWide();
		int nTall = pVguiTarget->GetTall();

		/*
		const char *pchLookup = pTarget->GetVguiTargetLookup();
		if ( pchLookup[ 0 ] != '\0' )
		{
			bool bLookupSuccess = false;
			bLookupSuccess = pVguiTarget->LookupElementBounds( pchLookup, nPanelX, nPanelY, nWide, nTall );

			Assert( bLookupSuccess );
		}
		*/

		if ( nVguiTargetEdge == vgui::Label::a_north || 
			 nVguiTargetEdge == vgui::Label::a_center || 
			 nVguiTargetEdge == vgui::Label::a_south )
		{
			nPanelX += nWide / 2;
		}
		else if ( nVguiTargetEdge == vgui::Label::a_northeast || 
			 nVguiTargetEdge == vgui::Label::a_east || 
			 nVguiTargetEdge == vgui::Label::a_southeast )
		{
			nPanelX += nWide;
		}

		if ( nVguiTargetEdge == vgui::Label::a_west || 
			 nVguiTargetEdge == vgui::Label::a_center || 
			 nVguiTargetEdge == vgui::Label::a_east )
		{
			nPanelY += nTall / 2;
		}
		else if ( nVguiTargetEdge == vgui::Label::a_southwest || 
			 nVguiTargetEdge == vgui::Label::a_south || 
			 nVguiTargetEdge == vgui::Label::a_southeast )
		{
			nPanelY += nTall;
		}

		pVguiTarget->LocalToScreen( nPanelX, nPanelY );

		DrawPointerBackground( pTarget, nPanelX, nPanelY, totalWide, iconTall, true );
	}
	else
	{
		DrawPointerBackground( pTarget, pTarget->m_centerX, pTarget->m_centerY, totalWide, iconTall, false );
	}

	if ( pTarget->m_pIcon_onscreen )
	{
		if ( !pTarget->m_bDrawControllerButton )
		{
			// Don't draw the icon if we're on 360 and have a binding to draw
			pTarget->m_pIcon_onscreen->DrawSelf( pTarget->GetIconX(), pTarget->GetIconY(), iconWide, iconTall, Color( 255, 255, 255, pTarget->m_alpha ) );
		}
	}

	DrawTargetCaption( pTarget, pTarget->GetIconX() + iconWide + ICON_GAP, pTarget->GetIconCenterY(), bDrawMultilineCaption );
	if ( pTarget->DrawBindingName() )
	{
		DrawBindingName( pTarget, pTarget->DrawBindingName(), pTarget->GetIconX() + (iconWide>>1), pTarget->GetIconY() + (iconTall>>1), pTarget->m_bDrawControllerButton );
	}

	// Draw the arrow.
	int iArrowSize = ScreenWidth() * ICON_SIZE;	// Always square width
	DrawIndicatorArrow( pTarget->GetIconX(), pTarget->GetIconY(), iArrowSize, iArrowSize, pTarget->m_captionWide + ICON_GAP, pTarget->m_drawArrowDirection );

	pTarget->m_bOnscreen = true;

	// Move the static icon position so the next static icon drawn this frame below this one.
	m_staticIconPosition += iconTall + (iconTall>>2);
	// Move down a little more if this one was multi-line
	if ( bDrawMultilineCaption )
	{
		m_staticIconPosition += (iconTall>>2);
	}
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Position and animate this target's icon on the screen. Based on
//			options, draw the indicator arrows (arrows that point to the 
//			direction the player should turn to see the icon), text caption, 
//			and the 'simple' arrow which just points down to indicate the 
//			item the icon represents.
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawDynamicIcon( CLocatorTarget *pTarget, bool bDrawCaption, bool bDrawSimpleArrow )
{
	int alpha = pTarget->m_alpha;

	if( pTarget->m_bOccluded && !( (pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_FORCE_CAPTION) || locator_topdown_style.GetBool() ) )
	{
		return;
	}

	// Draw the icon!
	vgui::surface()->DrawSetColor( 255, 255, 255, alpha );

	int iWide = pTarget->m_wide;

	if ( !pTarget->m_bOnscreen )
	{
		// Width is always square for offscreen icons
		iWide /= pTarget->m_widthScale_onscreen;
	}

	// Figure out the caption width
	pTarget->m_captionWide = GetScreenWidthForCaption( pTarget->GetCaptionText(), m_hCaptionFont );
	
	bool bDrawMultilineCaption = false;

	if ( m_iShouldWrapStaticLocators > 0 )	// conditionalized in locator.res
	{
		if ( pTarget->m_captionWide > (  ScreenWidth() * locator_split_maxwide_percent.GetFloat() ) )
		{
			// we will double-line this
			pTarget->m_captionWide = pTarget->m_captionWide * locator_split_len.GetFloat();
			bDrawMultilineCaption = true;
		}
	}

	int totalWide = iWide;

	bool bShouldDrawCaption = ( (pTarget->GetIconEffectsFlags() & LOCATOR_ICON_FX_FORCE_CAPTION) || (!pTarget->m_bOccluded && pTarget->m_distFromPlayer <= ICON_DIST_TOO_FAR) || locator_topdown_style.GetBool() );

	if( pTarget->m_bOnscreen && bDrawCaption && bShouldDrawCaption )
	{
		totalWide += ( ICON_GAP + pTarget->m_captionWide );
	}

	pTarget->m_bIsDrawing = true;

	int nTargetX, nTargetY;

	vgui::Panel *pVguiTarget = pTarget->GetVguiTarget();

	if ( pVguiTarget )
	{
		nTargetX = 0;
		nTargetY = 0;

		vgui::Label::Alignment nVguiTargetEdge = (vgui::Label::Alignment)pTarget->GetVguiTargetEdge();

		int nWide = pVguiTarget->GetWide();
		int nTall = pVguiTarget->GetTall();

		/*
		const char *pchLookup = pTarget->GetVguiTargetLookup();
		
		if ( pchLookup[ 0 ] != '\0' )
		{
			bool bLookupSuccess = false;
			bLookupSuccess = pVguiTarget->LookupElementBounds( pchLookup, nTargetX, nTargetY, nWide, nTall );
			Assert( bLookupSuccess );
		}
		*/

		if ( nVguiTargetEdge == vgui::Label::a_north || 
			 nVguiTargetEdge == vgui::Label::a_center || 
			 nVguiTargetEdge == vgui::Label::a_south )
		{
			nTargetX += nWide / 2;
		}
		else if ( nVguiTargetEdge== vgui::Label::a_northeast || 
			 nVguiTargetEdge == vgui::Label::a_east || 
			 nVguiTargetEdge == vgui::Label::a_southeast )
		{
			nTargetX += nWide;
		}

		if ( nVguiTargetEdge == vgui::Label::a_west || 
			 nVguiTargetEdge == vgui::Label::a_center || 
			 nVguiTargetEdge == vgui::Label::a_east )
		{
			nTargetY += nTall / 2;
		}
		else if ( nVguiTargetEdge == vgui::Label::a_southwest || 
			 nVguiTargetEdge == vgui::Label::a_south || 
			 nVguiTargetEdge == vgui::Label::a_southeast )
		{
			nTargetY += nTall;
		}

		pVguiTarget->LocalToScreen( nTargetX, nTargetY );
	}
	else if ( !pTarget->m_bOnscreen )
	{
		nTargetX = pTarget->m_targetX;
		nTargetY = pTarget->m_targetY;
	}
	else
	{
		nTargetX = pTarget->m_centerX;
		nTargetY = pTarget->m_centerY;
	}

	if ( pTarget->m_bOnscreen )
	{
		DrawPointerBackground( pTarget, nTargetX, nTargetY, totalWide, pTarget->m_tall, true );
	}
	else
	{
		// Offscreen we need to point the pointer toward out offscreen target
		DrawPointerBackground( pTarget, nTargetX, nTargetY, totalWide, pTarget->m_tall, true );
	}

	if( pTarget->m_bOnscreen && pTarget->m_pIcon_onscreen )
	{
		if ( !pTarget->m_bDrawControllerButton )
		{
			pTarget->m_pIcon_onscreen->DrawSelf( pTarget->GetIconX(), pTarget->GetIconY(), iWide, pTarget->m_tall, Color( 255, 255, 255, alpha ) );
		}
	}
	else if ( pTarget->m_pIcon_offscreen )
	{
		if ( !pTarget->m_bDrawControllerButtonOffscreen )
		{
			pTarget->m_pIcon_offscreen->DrawSelf( pTarget->GetIconX(), pTarget->GetIconY(), iWide, pTarget->m_tall, Color( 255, 255, 255, alpha ) );
		}
	}

	if( !pTarget->m_bOnscreen )
	{
		if ( pTarget->DrawBindingNameOffscreen() )
		{
			DrawBindingName( pTarget, pTarget->DrawBindingName(), pTarget->GetIconX() + (iWide>>1), pTarget->GetIconY() + (pTarget->m_tall>>1), pTarget->m_bDrawControllerButtonOffscreen );
		}

		if ( locator_background_style.GetInt() == 0 )
		{
			// Draw the arrow.
			DrawIndicatorArrow( pTarget->GetIconX(), pTarget->GetIconY(), iWide, pTarget->m_tall, 0, pTarget->m_drawArrowDirection );
		}
	}
	else if( bShouldDrawCaption )
	{
		if( bDrawCaption )
		{
			//ScreenWidth() *  * pTarget->m_widthScale_onscreen
			DrawTargetCaption( pTarget, pTarget->GetIconCenterX() + ICON_GAP + pTarget->GetIconWidth() * 0.5, pTarget->GetIconCenterY(), bDrawMultilineCaption );
		}
		if ( pTarget->DrawBindingName() )
		{
			DrawBindingName( pTarget, pTarget->DrawBindingName(), pTarget->GetIconX() + (iWide>>1), pTarget->GetIconY() + (pTarget->m_tall>>1), pTarget->m_bDrawControllerButton );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Some targets have text captions. Draw the text.
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawTargetCaption( CLocatorTarget *pTarget, int x, int y, bool bDrawMultiline )
{
	// Draw the caption
	vgui::surface()->DrawSetTextFont( m_hCaptionFont );
	int fontTall = vgui::surface()->GetFontTall( m_hCaptionFont );
	int iCaptionWidth = GetScreenWidthForCaption( pTarget->GetCaptionText(), m_hCaptionFont );

	if ( bDrawMultiline )
	{
		iCaptionWidth *= locator_split_len.GetFloat();
	}

	// Mapbase fixes glow and shadow not working in multiline
	bool bDrawGlow = locator_text_glow.GetBool();
	bool bDrawShadow = !IsConsole() && locator_text_drop_shadow.GetBool(); // Only draw drop shadow on PC because it looks crappy on a TV

	if ( !bDrawMultiline )
	{
		if ( bDrawGlow )
		{
			vgui::surface()->DrawSetTextFont( m_hCaptionGlowFont );
			Color glowColor = locator_text_glow_color.GetColor();
			vgui::surface()->DrawSetTextColor( glowColor.r(), glowColor.g(), glowColor.b(), ( glowColor.a() / 255.0f ) * pTarget->m_alpha );
			vgui::surface()->DrawSetTextPos( x - 1, y - (fontTall >>1) - 1 );
			vgui::surface()->DrawUnicodeString( pTarget->GetCaptionText() );
			vgui::surface()->DrawSetTextFont( m_hCaptionFont );
		}

		if ( bDrawShadow )
		{
			// Draw black text (drop shadow)
			vgui::surface()->DrawSetTextColor( 0,0,0, pTarget->m_alpha );
			vgui::surface()->DrawSetTextPos( x, y - (fontTall >>1) );
			vgui::surface()->DrawUnicodeString( pTarget->GetCaptionText() );
		}

		// Draw text
		vgui::surface()->DrawSetTextColor( pTarget->m_captionColor.r(),pTarget->m_captionColor.g(),pTarget->m_captionColor.b(), pTarget->m_alpha );
		vgui::surface()->DrawSetTextPos( x - 1, y - (fontTall >>1) - 1 );
		vgui::surface()->DrawUnicodeString( pTarget->GetCaptionText() );
	}
	else
	{
		int charX = x-1;
		int charY = y - ( fontTall >> 1 ) - 1;

		int iWidth = 0;

		const wchar_t *pString = pTarget->GetCaptionText();
		int len = Q_wcslen( pString );

		Color glowColor = locator_text_glow_color.GetColor();
		for ( int iChar = 0; iChar < len; ++ iChar )
		{
			int charW = vgui::surface()->GetCharacterWidth( m_hCaptionFont, pString[ iChar ] );
			iWidth += charW;

			if ( iWidth > pTarget->m_captionWide && pString[iChar] == L' ' )
			{
				charY += fontTall;
				charX = x-1;
				iWidth = 0;
			}

			if ( bDrawGlow )
			{
				vgui::surface()->DrawSetTextFont( m_hCaptionGlowFont );
				vgui::surface()->DrawSetTextColor( glowColor.r(), glowColor.g(), glowColor.b(), ( glowColor.a() / 255.0f ) * pTarget->m_alpha );
				vgui::surface()->DrawSetTextPos( charX - 1, charY );
				vgui::surface()->DrawUnicodeChar( pString[iChar] );
				vgui::surface()->DrawSetTextFont( m_hCaptionFont );
			}

			if ( bDrawShadow )
			{
				// Draw black text (drop shadow)
				vgui::surface()->DrawSetTextColor( 0,0,0, pTarget->m_alpha );
				vgui::surface()->DrawSetTextPos( charX, charY + 1 );
				vgui::surface()->DrawUnicodeChar( pString[iChar] );
			}

			// Draw text
			vgui::surface()->DrawSetTextColor( pTarget->m_captionColor.r(),pTarget->m_captionColor.g(),pTarget->m_captionColor.b(), pTarget->m_alpha );
			vgui::surface()->DrawSetTextPos( charX, charY );
			vgui::surface()->DrawUnicodeChar( pString[iChar] );
			charX += charW;
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: Figure out how wide (pixels) a string will be if rendered with this font
// 
//-----------------------------------------------------------------------------
int CLocatorPanel::GetScreenWidthForCaption( const wchar_t *pString, vgui::HFont hFont )
{
	int iWidth = 0;

	for ( int iChar = 0; iChar < Q_wcslen( pString ); ++ iChar )
	{
		iWidth += vgui::surface()->GetCharacterWidth( hFont, pString[ iChar ] );
	}

	return iWidth;
}

//-----------------------------------------------------------------------------
// Purpose: Some targets' captions contain information about key bindings that
//			should be displayed to the player. Do so.
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawBindingName( CLocatorTarget *pTarget, const char *pchBindingName, int x, int y, bool bController )
{
	if ( !bController && !IsConsole() )
	{
		// Draw the caption
		vgui::surface()->DrawSetTextFont( m_hKeysFont );
		int fontTall = vgui::surface()->GetFontTall( m_hKeysFont );

		char szBinding[ 256 ];
		Q_strcpy( szBinding, pchBindingName ? pchBindingName : "" );

		if ( Q_strcmp( szBinding, "SEMICOLON" ) == 0 )
		{
			Q_strcpy( szBinding, ";" );
		}
		else if ( Q_strlen( szBinding ) == 1 && szBinding[ 0 ] >= 'a' && szBinding[ 0 ] <= 'z' )
		{
			// Make single letters uppercase
			szBinding[ 0 ] += ( 'A' - 'a' );
		}

		wchar_t wszCaption[ 64 ];
		g_pVGuiLocalize->ConstructString( wszCaption, sizeof(wchar_t)*64, szBinding, NULL );

		int iWidth = GetScreenWidthForCaption( wszCaption, m_hKeysFont );

#ifdef MAPBASE
		// Mods are capable of choosing a custom color
		vgui::surface()->DrawSetTextColor( pTarget->m_bindingColor.r(), pTarget->m_bindingColor.g(), pTarget->m_bindingColor.b(), pTarget->m_alpha );
#else
		// Draw black text
		vgui::surface()->DrawSetTextColor( 0,0,0, pTarget->m_alpha );
#endif
		vgui::surface()->DrawSetTextPos( x - (iWidth>>1) - 1, y - (fontTall >>1) - 1 );
		vgui::surface()->DrawUnicodeString( wszCaption );
	}
	else
	{
		// Draw the caption
		wchar_t	wszCaption[ 64 ];

		vgui::surface()->DrawSetTextFont( BUTTON_FONT_HANDLE );
		int fontTall = vgui::surface()->GetFontTall( BUTTON_FONT_HANDLE );

		char szBinding[ 256 ];

		// Turn localized string into icon character
		Q_snprintf( szBinding, sizeof( szBinding ), "#GameUI_Icons_%s", pchBindingName );
		g_pVGuiLocalize->ConstructString( wszCaption, sizeof( wszCaption ), g_pVGuiLocalize->Find( szBinding ), 0 );
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszCaption, szBinding, sizeof( szBinding ) );

		int iWidth = GetScreenWidthForCaption( wszCaption, BUTTON_FONT_HANDLE );

		int iLargeIconShift = MAX( 0, iWidth - ( ScreenWidth() * ICON_SIZE + ICON_GAP + ICON_GAP ) );

		// Draw the button
		vgui::surface()->DrawSetTextColor( 255,255,255, pTarget->m_alpha );
		vgui::surface()->DrawSetTextPos( x - (iWidth>>1) - iLargeIconShift, y - (fontTall >>1) );
		vgui::surface()->DrawUnicodeString( wszCaption );

	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw an arrow to indicate that a target is offscreen
//
// iconWide is sent to this function so that the arrow knows how to straddle
// the icon that it is being drawn near.
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawIndicatorArrow( int x, int y, int iconWide, int iconTall, int textWidth, int direction )
{
	int wide = iconWide;
	int tall = iconTall;

	//tall = wide = ScreenWidth() * ICON_SIZE;

	switch( direction )
	{
	case DRAW_ARROW_LEFT:
		vgui::surface()->DrawSetTexture( m_textureID_ArrowLeft );
		x -= wide;
		y += iconTall / 2 - tall / 2;
		vgui::surface()->DrawTexturedRect( x, y, x + wide, y + tall );
		break;

	case DRAW_ARROW_RIGHT:
		vgui::surface()->DrawSetTexture( m_textureID_ArrowRight );
		x += iconWide + textWidth;
		y += iconTall / 2 - tall / 2;
		vgui::surface()->DrawTexturedRect( x, y, x + wide, y + tall );
		break;

	case DRAW_ARROW_UP:
		vgui::surface()->DrawSetTexture( m_textureID_ArrowUp );
		x += iconWide / 2 - wide / 2;
		y -= tall;
		vgui::surface()->DrawTexturedRect( x, y, x + wide, y + tall );
		break;

	case DRAW_ARROW_DOWN:
		vgui::surface()->DrawSetTexture( m_textureID_ArrowDown );
		x += iconWide / 2 - wide / 2;
		y += iconTall;
		vgui::surface()->DrawTexturedRect( x, y, x + wide, y + tall );
		break;

	default:
		// Do not draw.
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draws a very simple arrow that points down.
//-----------------------------------------------------------------------------
void CLocatorPanel::DrawSimpleArrow( int x, int y, int iconWide, int iconTall )
{
	vgui::surface()->DrawSetTexture( m_textureID_SimpleArrow );

	y += iconTall;

	vgui::surface()->DrawTexturedRect( x, y, x + iconWide, y + iconTall );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLocatorPanel::GetIconPositionForOffscreenTarget( const Vector &vecDelta, float flDist, int *pXPos, int *pYPos )
{
	float xpos, ypos;
	float flRotation;
	float flRadius = YRES(OFFSCREEN_ICON_POSITION_RADIUS);

	if ( locator_topdown_style.GetBool() )
	{
		flRadius *= clamp( flDist / 600.0f, 1.75f, 3.0f );
	}

	GetTargetPosition( vecDelta, flRadius, &xpos, &ypos, &flRotation );

	*pXPos = xpos;
	*pYPos = ypos;
}

//-----------------------------------------------------------------------------
// Purpose: Given a handle, return the pointer to the proper locator target.
//-----------------------------------------------------------------------------
CLocatorTarget *CLocatorPanel::GetPointerForHandle( int hTarget )
{
	for( int i = 0 ; i < MAX_LOCATOR_TARGETS ; i++ )
	{
		if( m_targets[ i ].m_isActive && m_targets[ i ].m_serialNumber == hTarget )
		{
			return &m_targets[ i ];
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CLocatorPanel::AddTarget()
{
	for( int i = 0 ; i < MAX_LOCATOR_TARGETS ; i++ )
	{
		if( !m_targets[ i ].m_isActive )
		{
			m_targets[ i ].Activate( m_serializer );
			m_serializer++;

			return m_targets[ i ].m_serialNumber;
		}
	}

	DevWarning( "Locator Panel has no free targets!\n" );
	return -1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CLocatorPanel::RemoveTarget( int hTarget )
{
	CLocatorTarget *pTarget = GetPointerForHandle( hTarget );

	if( pTarget )
	{
		pTarget->Deactivate();
	}
}

