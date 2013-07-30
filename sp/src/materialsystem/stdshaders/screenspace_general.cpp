//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "SDK_screenspaceeffect_vs20.inc"

DEFINE_FALLBACK_SHADER( SDK_screenspace_general, SDK_screenspace_general_dx9 )
BEGIN_VS_SHADER_FLAGS( SDK_screenspace_general_dx9, "Help for screenspace_general", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( C0_X,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C0_Y,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C0_Z,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C0_W,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C1_X,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C1_Y,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C1_Z,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C1_W,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C2_X,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C2_Y,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C2_Z,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C2_W,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C3_X,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C3_Y,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C3_Z,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( C3_W,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( PIXSHADER, SHADER_PARAM_TYPE_STRING, "", "Name of the pixel shader to use" )
		SHADER_PARAM( DISABLE_COLOR_WRITES,SHADER_PARAM_TYPE_INTEGER,"0","")
		SHADER_PARAM( ALPHATESTED,SHADER_PARAM_TYPE_FLOAT,"0","")
		SHADER_PARAM( TEXTURE1, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( TEXTURE3, SHADER_PARAM_TYPE_TEXTURE, "", "" )
		SHADER_PARAM( LINEARREAD_BASETEXTURE, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( LINEARREAD_TEXTURE1, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( LINEARREAD_TEXTURE2, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( LINEARREAD_TEXTURE3, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( LINEARWRITE,SHADER_PARAM_TYPE_INTEGER,"0","")
		SHADER_PARAM( X360APPCHOOSER, SHADER_PARAM_TYPE_INTEGER, "0", "Needed for movies in 360 launcher" )
	END_SHADER_PARAMS

    SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
		if ( params[TEXTURE1]->IsDefined() )
		{
			LoadTexture( TEXTURE1 );
		}
		if ( params[TEXTURE2]->IsDefined() )
		{
			LoadTexture( TEXTURE2 );
		}
		if ( params[TEXTURE3]->IsDefined() )
		{
			LoadTexture( TEXTURE3 );
		}
	}
	
	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "screenspace_general_dx8";
		}

		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableDepthWrites( false );
			
			if (params[BASETEXTURE]->IsDefined())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				ITexture *txtr=params[BASETEXTURE]->GetTextureValue();
				ImageFormat fmt=txtr->GetImageFormat();
				if ((fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616))
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0,false);
				else
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, !params[LINEARREAD_BASETEXTURE]->IsDefined() || !params[LINEARREAD_BASETEXTURE]->GetIntValue() );
			}				
			if (params[TEXTURE1]->IsDefined())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
				ITexture *txtr=params[TEXTURE1]->GetTextureValue();
				ImageFormat fmt=txtr->GetImageFormat();
				if ((fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616))
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1,false);
				else
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER1, !params[LINEARREAD_TEXTURE1]->IsDefined() || !params[LINEARREAD_TEXTURE1]->GetIntValue() );
			}				
			if (params[TEXTURE2]->IsDefined())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
				ITexture *txtr=params[TEXTURE2]->GetTextureValue();
				ImageFormat fmt=txtr->GetImageFormat();
				if ((fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616))
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER2,false);
				else
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER2, !params[LINEARREAD_TEXTURE2]->IsDefined() || !params[LINEARREAD_TEXTURE2]->GetIntValue() );
			}				
			if (params[TEXTURE3]->IsDefined())
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
				ITexture *txtr=params[TEXTURE3]->GetTextureValue();
				ImageFormat fmt=txtr->GetImageFormat();
				if ((fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616))
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3,false);
				else
					pShaderShadow->EnableSRGBRead(SHADER_SAMPLER3, !params[LINEARREAD_TEXTURE3]->IsDefined() || !params[LINEARREAD_TEXTURE3]->GetIntValue() );
			}				
			int fmt = VERTEX_POSITION;

			if ( IS_PARAM_DEFINED( X360APPCHOOSER ) && ( params[X360APPCHOOSER]->GetIntValue() ) )
			{
				fmt |= VERTEX_COLOR;
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// maybe convert from linear to gamma on write.
			bool srgb_write=true;
			if (params[LINEARWRITE]->GetFloatValue())
				srgb_write=false;
			pShaderShadow->EnableSRGBWrite( srgb_write );

			// Pre-cache shaders
			DECLARE_STATIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( X360APPCHOOSER, IS_PARAM_DEFINED( X360APPCHOOSER ) ? params[X360APPCHOOSER]->GetIntValue() : 0 );
			vsh_forgot_to_set_static_X360APPCHOOSER = 0; // This is a dirty workaround to the shortcut [= 0] in the fxc
			SET_STATIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );

			if (params[DISABLE_COLOR_WRITES]->GetIntValue())
			{
				pShaderShadow->EnableColorWrites(false);
			}
//			if (params[ALPHATESTED]->GetFloatValue())
			{
				pShaderShadow->EnableAlphaTest(true);
				pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GREATER,0.0);
			}
			if ( IS_FLAG_SET(MATERIAL_VAR_ADDITIVE) )
			{
				EnableAlphaBlending( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			}

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
		}

		DYNAMIC_STATE
		{
			if (params[BASETEXTURE]->IsDefined())
			{
				BindTexture( SHADER_SAMPLER0, BASETEXTURE, -1 );
			}
			if (params[TEXTURE1]->IsDefined())
			{
				BindTexture( SHADER_SAMPLER1, TEXTURE1, -1 );
			}
			if (params[TEXTURE2]->IsDefined())
			{
				BindTexture( SHADER_SAMPLER2, TEXTURE2, -1 );
			}
			if (params[TEXTURE3]->IsDefined())
			{
				BindTexture( SHADER_SAMPLER3, TEXTURE3, -1 );
			}
			float c0[]={
				params[C0_X]->GetFloatValue(),
				params[C0_Y]->GetFloatValue(),
				params[C0_Z]->GetFloatValue(),
				params[C0_W]->GetFloatValue(),
				params[C1_X]->GetFloatValue(),
				params[C1_Y]->GetFloatValue(),
				params[C1_Z]->GetFloatValue(),
				params[C1_W]->GetFloatValue(),
				params[C2_X]->GetFloatValue(),
				params[C2_Y]->GetFloatValue(),
				params[C2_Z]->GetFloatValue(),
				params[C2_W]->GetFloatValue(),
				params[C3_X]->GetFloatValue(),
				params[C3_Y]->GetFloatValue(),
				params[C3_Z]->GetFloatValue(),
				params[C3_W]->GetFloatValue()
			};

			pShaderAPI->SetPixelShaderConstant( 0, c0, ARRAYSIZE(c0)/4 );

			float eyePos[4];
			pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
			pShaderAPI->SetPixelShaderConstant( 10, eyePos, 1 );

			pShaderAPI->SetVertexShaderIndex( 0 );
			pShaderAPI->SetPixelShaderIndex( 0 );

			DECLARE_DYNAMIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( sdk_screenspaceeffect_vs20 );
		}
		Draw();
	}
END_SHADER
