//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"
#include "SDK_Bloom_ps20.inc"
#include "SDK_Bloom_ps20b.inc"

BEGIN_VS_SHADER_FLAGS( SDK_Bloom, "Help for Bloom", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( FBTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "" )
		SHADER_PARAM( BLURTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_SmallHDR0", "" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[FBTEXTURE]->IsDefined() )
		{
			LoadTexture( FBTEXTURE );
		}
		if( params[BLURTEXTURE]->IsDefined() )
		{
			LoadTexture( BLURTEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			Assert( 0 );
			return "Wireframe";
		}
		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( sdk_bloom_ps20b );
				SET_STATIC_PIXEL_SHADER( sdk_bloom_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( sdk_bloom_ps20 );
				SET_STATIC_PIXEL_SHADER( sdk_bloom_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, FBTEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BLURTEXTURE, -1 );
			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sdk_bloom_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( sdk_bloom_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sdk_bloom_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( sdk_bloom_ps20 );
			}
		}
		Draw();
	}
END_SHADER
