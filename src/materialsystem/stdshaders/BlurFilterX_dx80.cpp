//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "BaseVSShader.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( BlurFilterX, BlurFilterX_DX80 )

BEGIN_VS_SHADER_FLAGS( BlurFilterX_DX80, "Help for BlurFilterX_DX80", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT
	{
		if( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}
	
	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 80 )
		{
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
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, 0, 0 );

			// Pre-cache shaders
			pShaderShadow->SetVertexShader( "BlurFilter_vs11", 0 );
			pShaderShadow->SetPixelShader( "BlurFilter_ps11", 0 );

			if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
				EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER2, BASETEXTURE, -1 );
			BindTexture( SHADER_SAMPLER3, BASETEXTURE, -1 );

			// The temp buffer is 1/4 back buffer size
			ITexture *src_texture=params[BASETEXTURE]->GetTextureValue();
			int width = src_texture->GetActualWidth();
			float dX = 2.0f / width;

			// 4 Tap offsets, expected from pixel center
			float v[4][4];
			v[0][0] = -1.5f * dX;
			v[0][1] = 0;
			v[1][0] = -0.5f * dX;
			v[1][1] = 0;
			v[2][0] = 0.5f * dX;
			v[2][1] = 0;
			v[3][0] = 1.5f * dX;
			v[3][1] = 0;
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, &v[0][0], 4 );

			v[0][0] = v[0][1] = v[0][2] = v[0][3] = 1.0f;
			pShaderAPI->SetPixelShaderConstant( 1, v[0], 1 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );
		}
		Draw();
	}
END_SHADER
