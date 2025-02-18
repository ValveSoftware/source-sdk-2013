//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#ifdef STDSHADER_DX9_DLL_EXPORT
#include "screenspaceeffect_vs20.inc"
#include "filmdust_ps20.inc"
#else
#include "screenspaceeffect_vs11.inc"
#include "filmdust_ps11.inc"
#endif

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef STDSHADER_DX9_DLL_EXPORT
DEFINE_FALLBACK_SHADER( FilmDust, FilmDust_dx9 )
BEGIN_VS_SHADER_FLAGS( FilmDust_dx9, "Help for FilmDust", SHADER_NOT_EDITABLE )
#else
DEFINE_FALLBACK_SHADER( FilmDust, FilmDust_dx8 )
BEGIN_VS_SHADER_FLAGS( FilmDust_dx8, "Help for FilmDust", SHADER_NOT_EDITABLE )
#endif

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DUST_TEXTURE,   SHADER_PARAM_TYPE_TEXTURE, "0", "Film dust texture" )
		SHADER_PARAM( CHANNEL_SELECT, SHADER_PARAM_TYPE_VEC4,    "",  "Select which color channel to use" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_FALLBACK
	{
#ifdef STDSHADER_DX9_DLL_EXPORT
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90)
		{
			return "FilmDust_DX8";
		}
#else // We're DX8
		// Requires DX8 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80)
		{
			return "FilmDust_DX7";
		}
#endif
		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( DUST_TEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableCulling( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ZERO, SHADER_BLEND_SRC_COLOR );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

#ifdef STDSHADER_DX9_DLL_EXPORT
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( filmdust_ps20 );
			SET_STATIC_PIXEL_SHADER( filmdust_ps20 );
#else
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );

			DECLARE_STATIC_PIXEL_SHADER( filmdust_ps11 );
			SET_STATIC_PIXEL_SHADER( filmdust_ps11 );
#endif
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, DUST_TEXTURE, -1 );
						
			SetPixelShaderConstant( 0, CHANNEL_SELECT );
#ifdef STDSHADER_DX9_DLL_EXPORT
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( filmdust_ps20 );
			SET_DYNAMIC_PIXEL_SHADER( filmdust_ps20 );
#else
			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );

			DECLARE_DYNAMIC_PIXEL_SHADER( filmdust_ps11 );
			SET_DYNAMIC_PIXEL_SHADER( filmdust_ps11 );
#endif
		}
		Draw();
	}
END_SHADER
