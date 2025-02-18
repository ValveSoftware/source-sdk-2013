//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#ifdef STDSHADER_DX9_DLL_EXPORT
#include "screenspaceeffect_vs20.inc"
#include "filmgrain_ps20.inc"
#else
#include "screenspaceeffect_vs11.inc"
#include "filmgrain_ps11.inc"
#endif

#include "../materialsystem_global.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef STDSHADER_DX9_DLL_EXPORT
DEFINE_FALLBACK_SHADER( FilmGrain, FilmGrain_dx9 )
BEGIN_VS_SHADER_FLAGS( FilmGrain_dx9, "Help for FilmGrain", SHADER_NOT_EDITABLE )
#else
DEFINE_FALLBACK_SHADER( FilmGrain, FilmGrain_dx8 )
BEGIN_VS_SHADER_FLAGS( FilmGrain_dx8, "Help for FilmGrain", SHADER_NOT_EDITABLE )
#endif

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( GRAIN_TEXTURE,  SHADER_PARAM_TYPE_TEXTURE, "0", "Film grain texture" )
		SHADER_PARAM( NOISESCALE,     SHADER_PARAM_TYPE_VEC4, "", "Strength of film grain" )
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
				return "FilmGrain_dx8";
		}
#else // We're DX8
		// Requires DX8 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80)
		{
			return "FilmGrain_dx7";
		}
#endif

		return 0;
	}

	SHADER_INIT
	{
		LoadTexture( GRAIN_TEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

#ifdef STDSHADER_DX9_DLL_EXPORT
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( filmgrain_ps20 );
			SET_STATIC_PIXEL_SHADER( filmgrain_ps20 );
#else
			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs11 );

			DECLARE_STATIC_PIXEL_SHADER( filmgrain_ps11 );
			SET_STATIC_PIXEL_SHADER( filmgrain_ps11 );
#endif

		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, GRAIN_TEXTURE, -1 );
						
			SetPixelShaderConstant( 0, NOISESCALE );

#ifdef STDSHADER_DX9_DLL_EXPORT
				DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
				SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );

				DECLARE_DYNAMIC_PIXEL_SHADER( filmgrain_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( filmgrain_ps20 );
#else
				DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );
				SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs11 );

				DECLARE_DYNAMIC_PIXEL_SHADER( filmgrain_ps11 );
				SET_DYNAMIC_PIXEL_SHADER( filmgrain_ps11 );
#endif
		}
		Draw();
	}
END_SHADER
