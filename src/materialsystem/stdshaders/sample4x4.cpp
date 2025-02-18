//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"
#include "common_hlsl_cpp_consts.h"


BEGIN_VS_SHADER_FLAGS( Sample4x4, "Help for Sample4x4", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( PIXSHADER, SHADER_PARAM_TYPE_STRING, "sample4x4_ps20", "Name of the pixel shader to use" )
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
			
			pShaderShadow->SetVertexShader( "Downsample_vs20", 0 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				const char *szPixelShader = params[PIXSHADER]->GetStringValue();
				size_t iLength = Q_strlen( szPixelShader );

				if( (iLength > 5) && (Q_stricmp( &szPixelShader[iLength - 5], "_ps20" ) == 0) ) //detect if it's trying to load a ps20 shader
				{
					//replace it with the ps20b shader
					char *szNewName = (char *)stackalloc( sizeof( char ) * (iLength + 2) );
					memcpy( szNewName, szPixelShader, sizeof( char ) * iLength );
					szNewName[iLength] = 'b';
					szNewName[iLength + 1] = '\0';
					pShaderShadow->SetPixelShader( szNewName, 0 );
				}
				else
				{
					pShaderShadow->SetPixelShader( params[PIXSHADER]->GetStringValue(), 0 );
				}
			}
			else
			{
				pShaderShadow->SetPixelShader( params[PIXSHADER]->GetStringValue(), 0 );
			}

// 			if ( IsAlphaModulating() )
// 			{
// 				pShaderShadow->EnableBlending( true );
// 				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA,
// 										  SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
// 			}
// 			else
// 			{
// 				pShaderShadow->EnableBlending( true );
// 				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA,
// 										  SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
// //				pShaderShadow->EnableBlending( false );
// 			}
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			ITexture *src_texture=params[BASETEXTURE]->GetTextureValue();

			int width=src_texture->GetActualWidth();
			int height=src_texture->GetActualHeight();

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

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );

			// store the ALPHA material var into c0
			v[0] = ALPHA;
			pShaderAPI->SetPixelShaderConstant( 0, v, 1 );
			
		}
		Draw();
	}
END_SHADER
