//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"

#include "Downsample_ps20.inc"
#include "Downsample_ps20b.inc"

BEGIN_VS_SHADER_FLAGS( sfm_downsample_shader, "Help for Downsample", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadTexture( BASETEXTURE );
	}
	
	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if (!g_pHardwareConfig->SupportsVertexAndPixelShaders())
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
			pShaderShadow->EnableAlphaWrites( true );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			pShaderShadow->SetVertexShader( "downsample_vs20", 0 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( downsample_ps20b );
				SET_STATIC_PIXEL_SHADER( downsample_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( downsample_ps20 );
				SET_STATIC_PIXEL_SHADER( downsample_ps20 );
			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );

			ITexture *src_texture=params[BASETEXTURE]->GetTextureValue();
			int width  = src_texture->GetActualWidth();
			int height = src_texture->GetActualHeight();

			float v[4];
			float dX = 1.0f / width;
			float dY = 1.0f / height;

			v[0] = -dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, v, 1 );
			v[0] = -dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, v, 1 );
			v[0] = dX;
			v[1] = -dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, v, 1 );
			v[0] = dX;
			v[1] = dY;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_3, v, 1 );

			// Setup luminance threshold (all values are scaled down by max luminance)
//			v[0] = 1.0f / MAX_HDR_OVERBRIGHT;
			v[0] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( downsample_ps20b );
				SET_DYNAMIC_PIXEL_SHADER( downsample_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( downsample_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( downsample_ps20 );
			}
		}
		Draw();
	}
END_SHADER
