//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Episodic screen-space effects
//

#include "cbase.h"
#include "ScreenSpaceEffects.h"
#include "rendertexture.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "cdll_client_int.h"
#include "materialsystem/itexture.h"
#include "KeyValues.h"
#include "clienteffectprecachesystem.h"

#include "episodic_screenspaceeffects.h"


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#ifdef _X360
#define STUN_TEXTURE "_rt_FullFrameFB2"
#else
#define STUN_TEXTURE "_rt_WaterRefraction"
#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CStunEffect::Init( void ) 
{
	m_flDuration = 0.0f;
	m_flFinishTime = 0.0f;
	m_bUpdateView = true;

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", STUN_TEXTURE );
	m_EffectMaterial.Init( "__stuneffect", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues );
	m_StunTexture.Init( STUN_TEXTURE, TEXTURE_GROUP_CLIENT_EFFECTS );
}

void CStunEffect::Shutdown( void )
{
	m_EffectMaterial.Shutdown();
	m_StunTexture.Shutdown();
}

//------------------------------------------------------------------------------
// Purpose: Pick up changes in our parameters
//------------------------------------------------------------------------------
void CStunEffect::SetParameters( KeyValues *params )
{
	if( params->FindKey( "duration" ) )
	{
		m_flDuration = params->GetFloat( "duration" );
		m_flFinishTime = gpGlobals->curtime + m_flDuration;
		m_bUpdateView = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CStunEffect::Render( int x, int y, int w, int h )
{
	// Make sure we're ready to play this effect
	if ( m_flFinishTime < gpGlobals->curtime )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	
	// Set ourselves to the proper rendermode
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	// Draw the texture if we're using it
	if ( m_bUpdateView )
	{
		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );
		m_bUpdateView = false;
	}

	float flEffectPerc = ( m_flFinishTime - gpGlobals->curtime ) / m_flDuration;

	float viewOffs = ( flEffectPerc * 32.0f ) * ( cos( gpGlobals->curtime * 40.0f ) * sin( gpGlobals->curtime * 17.0f ) );
	float vX = x + viewOffs;

	if ( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() >= 80 )
	{
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_NONE )
		{
			m_EffectMaterial->ColorModulate( 1.0f, 1.0f, 1.0f );
		}
		else
		{
			// This is a stupid fix, but I don't have time to do a cleaner implementation. Since
			// the introblur.vmt material uses unlit generic, it will tone map, so I need to undo the tone mapping
			// using color modulate.  The proper fix would be to use a different material type that
			// supports alpha blending but not tone mapping, which I don't think exists. Whatever. This works when
			// the tone mapping scalar is less than 1.0, which it is in the cases it's used in game.
			float flUnTonemap = pow( 1.0f / pRenderContext->GetToneMappingScaleLinear().x, 1.0f / 2.2f );
			m_EffectMaterial->ColorModulate( flUnTonemap, flUnTonemap, flUnTonemap );
		}

		// Set alpha blend value
		float flOverlayAlpha = clamp( ( 150.0f / 255.0f ) * flEffectPerc, 0.0f, 1.0f );
		m_EffectMaterial->AlphaModulate( flOverlayAlpha );

		// Draw full screen alpha-blended quad
		pRenderContext->DrawScreenSpaceRectangle( m_EffectMaterial, 0, 0, w, h,
			vX, 0, (m_StunTexture->GetActualWidth()-1)+vX, (m_StunTexture->GetActualHeight()-1), 
			m_StunTexture->GetActualWidth(), m_StunTexture->GetActualHeight() );
	}

	// Save off this pass
	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = w;
	srcRect.height = h;
	pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );

	// Restore our state
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

// ================================================================================================================
//
//  Ep 1. Intro blur
//
// ================================================================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEP1IntroEffect::Init( void ) 
{
	m_flDuration = 0.0f;
	m_flFinishTime = 0.0f;
	m_bUpdateView = true;
	m_bFadeOut = false;

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", STUN_TEXTURE );
	m_EffectMaterial.Init( "__ep1introeffect", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues );
	m_StunTexture.Init( STUN_TEXTURE, TEXTURE_GROUP_CLIENT_EFFECTS );
}

void CEP1IntroEffect::Shutdown( void ) 
{
	m_EffectMaterial.Shutdown();
	m_StunTexture.Shutdown();
}


//------------------------------------------------------------------------------
// Purpose: Pick up changes in our parameters
//------------------------------------------------------------------------------
void CEP1IntroEffect::SetParameters( KeyValues *params )
{
	if( params->FindKey( "duration" ) )
	{
		m_flDuration = params->GetFloat( "duration" );
		m_flFinishTime = gpGlobals->curtime + m_flDuration;
	}

	if( params->FindKey( "fadeout" ) )
	{
		m_bFadeOut = ( params->GetInt( "fadeout" ) == 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the alpha value depending on various factors and time
//-----------------------------------------------------------------------------
inline unsigned char CEP1IntroEffect::GetFadeAlpha( void )
{
	// Find our percentage between fully "on" and "off" in the pulse range
	float flEffectPerc = ( m_flDuration == 0.0f ) ? 0.0f : ( m_flFinishTime - gpGlobals->curtime ) / m_flDuration;
	flEffectPerc = clamp( flEffectPerc, 0.0f, 1.0f );

	if  ( m_bFadeOut )
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 50.0f * flEffectPerc, 0.0f, 50.0f );

		// Non-HDR
		return (unsigned char) clamp( 64.0f * flEffectPerc, 0.0f, 64.0f );
	}
	else
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 64.0f * flEffectPerc, 50.0f, 64.0f );

		// Non-HDR
		return (unsigned char) clamp( 128.0f * flEffectPerc, 64.0f, 128.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CEP1IntroEffect::Render( int x, int y, int w, int h )
{
	if ( ( m_flFinishTime == 0 ) || ( IsEnabled() == false ) )
		return;

	CMatRenderContextPtr pRenderContext( materials );
	
	// Set ourselves to the proper rendermode
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	// Draw the texture if we're using it
	if ( m_bUpdateView )
	{
		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );
		m_bUpdateView = false;
	}

	byte overlaycolor[4] = { 255, 255, 255, 0 };
	
	// Get our fade value depending on our fade duration
	overlaycolor[3] = GetFadeAlpha();
	if ( g_pMaterialSystemHardwareConfig->UsesSRGBCorrectBlending() )
	{
		// For DX10 cards, alpha blending happens in linear space, so try to adjust by hacking alpha to 50%
		overlaycolor[3] *= 0.7f;
	}

	// Disable overself if we're done fading out
	if ( m_bFadeOut && overlaycolor[3] == 0 )
	{
		// Takes effect next frame (we don't want to hose our matrix stacks here)
		g_pScreenSpaceEffects->DisableScreenSpaceEffect( "episodic_intro" );
		m_bUpdateView = true;
	}

	// Calculate some wavey noise to jitter the view by
	float vX = 2.0f * -fabs( cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 6.0 ) );
	float vY = 2.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 5.0 );

	// Scale percentage
	float flScalePerc = 0.02f + ( 0.01f * cosf( gpGlobals->curtime * 2.0f ) * cosf( gpGlobals->curtime * 0.5f ) );
	
	// Scaled offsets for the UVs (as texels)
	float flUOffset = ( m_StunTexture->GetActualWidth() - 1 ) * flScalePerc * 0.5f;
	float flVOffset = ( m_StunTexture->GetActualHeight() - 1 ) * flScalePerc * 0.5f;
	
	// New UVs with scaling offsets
	float flU1 = flUOffset;
	float flU2 = ( m_StunTexture->GetActualWidth() - 1 ) - flUOffset;
	float flV1 = flVOffset;
	float flV2 = ( m_StunTexture->GetActualHeight() - 1 ) - flVOffset;

	// Draw the "zoomed" overlay
	pRenderContext->DrawScreenSpaceRectangle( m_EffectMaterial, vX, vY, w, h,
		flU1, flV1, 
		flU2, flV2, 
		m_StunTexture->GetActualWidth(), m_StunTexture->GetActualHeight() );

	render->ViewDrawFade( overlaycolor, m_EffectMaterial );

	// Save off this pass
	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = w;
	srcRect.height = h;
	pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );

	// Restore our state
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

// ================================================================================================================
//
//  Ep 2. Groggy-player view
//
// ================================================================================================================

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEP2StunEffect::Init( void ) 
{
	m_flDuration = 0.0f;
	m_flFinishTime = 0.0f;
	m_bUpdateView = true;
	m_bFadeOut = false;

	KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", STUN_TEXTURE );
	m_EffectMaterial.Init( "__ep2stuneffect", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues );
	m_StunTexture.Init( STUN_TEXTURE, TEXTURE_GROUP_CLIENT_EFFECTS );
}

void CEP2StunEffect::Shutdown( void ) 
{
	m_EffectMaterial.Shutdown();
	m_StunTexture.Shutdown();
}

//------------------------------------------------------------------------------
// Purpose: Pick up changes in our parameters
//------------------------------------------------------------------------------
void CEP2StunEffect::SetParameters( KeyValues *params )
{
	if( params->FindKey( "duration" ) )
	{
		m_flDuration = params->GetFloat( "duration" );
		m_flFinishTime = gpGlobals->curtime + m_flDuration;
	}

	if( params->FindKey( "fadeout" ) )
	{
		m_bFadeOut = ( params->GetInt( "fadeout" ) == 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the alpha value depending on various factors and time
//-----------------------------------------------------------------------------
inline unsigned char CEP2StunEffect::GetFadeAlpha( void )
{
	// Find our percentage between fully "on" and "off" in the pulse range
	float flEffectPerc = ( m_flDuration == 0.0f ) ? 0.0f : ( m_flFinishTime - gpGlobals->curtime ) / m_flDuration;
	flEffectPerc = clamp( flEffectPerc, 0.0f, 1.0f );

	if  ( m_bFadeOut )
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 50.0f * flEffectPerc, 0.0f, 50.0f );

		// Non-HDR
		return (unsigned char) clamp( 64.0f * flEffectPerc, 0.0f, 64.0f );
	}
	else
	{
		// HDR requires us to be more subtle, or we get uber-brightening
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() != HDR_TYPE_NONE )
			return (unsigned char) clamp( 164.0f * flEffectPerc, 128.0f, 164.0f );

		// Non-HDR
		return (unsigned char) clamp( 164.0f * flEffectPerc, 128.0f, 164.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CEP2StunEffect::Render( int x, int y, int w, int h )
{
	if ( ( m_flFinishTime == 0 ) || ( IsEnabled() == false ) )
		return;

	CMatRenderContextPtr pRenderContext( materials );

	// Set ourselves to the proper rendermode
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	if ( m_bUpdateView )
	{
		// Save off this pass
		Rect_t srcRect;
		srcRect.x = x;
		srcRect.y = y;
		srcRect.width = w;
		srcRect.height = h;
		pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );
		m_bUpdateView = false;
	}

	byte overlaycolor[4] = { 255, 255, 255, 0 };

	// Get our fade value depending on our fade duration
	overlaycolor[3] = GetFadeAlpha();

	// Disable overself if we're done fading out
	if ( m_bFadeOut && overlaycolor[3] == 0 )
	{
		// Takes effect next frame (we don't want to hose our matrix stacks here)
		g_pScreenSpaceEffects->DisableScreenSpaceEffect( "ep2_groggy" );
		m_bUpdateView = true;
	}

	// Calculate some wavey noise to jitter the view by
	float vX = 4.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 6.0 );
	float vY = 2.0f * cosf( gpGlobals->curtime ) * cosf( gpGlobals->curtime * 5.0 );

	float flBaseScale = 0.2f + 0.005f * sinf( gpGlobals->curtime * 4.0f );

	// Scale percentage
	float flScalePerc = flBaseScale + ( 0.01f * cosf( gpGlobals->curtime * 2.0f ) * cosf( gpGlobals->curtime * 0.5f ) );

    // Scaled offsets for the UVs (as texels)
	float flUOffset = ( m_StunTexture->GetActualWidth() - 1 ) * flScalePerc * 0.5f;
	float flVOffset = ( m_StunTexture->GetActualHeight() - 1 ) * flScalePerc * 0.5f;

	// New UVs with scaling offsets
	float flU1 = flUOffset;
	float flU2 = ( m_StunTexture->GetActualWidth() - 1 ) - flUOffset;
	float flV1 = flVOffset;
	float flV2 = ( m_StunTexture->GetActualHeight() - 1 ) - flVOffset;

	// Draw the "zoomed" overlay
	pRenderContext->DrawScreenSpaceRectangle( m_EffectMaterial, vX, vY, w, h,
		flU1, flV1, 
		flU2, flV2, 
		m_StunTexture->GetActualWidth(), m_StunTexture->GetActualHeight() );

	render->ViewDrawFade( overlaycolor, m_EffectMaterial );

	// Save off this pass
	Rect_t srcRect;
	srcRect.x = x;
	srcRect.y = y;
	srcRect.width = w;
	srcRect.height = h;
	pRenderContext->CopyRenderTargetToTextureEx( m_StunTexture, 0, &srcRect, NULL );

	// Restore our state
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();
	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}
