//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//
#include "BaseVSShader.h"
#include "sky_vs20.inc"
#include "sky_ps20.inc"
#include "sky_ps20b.inc"

#include "convar.h"

BEGIN_VS_SHADER( Sky_DX9, "Help for Sky_DX9 shader" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_VEC3, "[ 1 1 1]", "color multiplier", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "sky_dx6";
		}
		return 0;
	}

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_NOFOG );
		SET_FLAGS( MATERIAL_VAR_IGNOREZ );
	}
	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			ImageFormat fmt = params[BASETEXTURE]->GetTextureValue()->GetImageFormat();
			LoadTexture( BASETEXTURE, (fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616) ? 0 : TEXTUREFLAGS_SRGB );
		}
	}
	SHADER_DRAW
	{
		SHADOW_STATE
		{
			SetInitialShadowState();

//			pShaderShadow->EnableAlphaWrites( true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			ITexture *txtr=params[BASETEXTURE]->GetTextureValue();
			ImageFormat fmt=txtr->GetImageFormat();
			if ((fmt==IMAGE_FORMAT_RGBA16161616F) || (fmt==IMAGE_FORMAT_RGBA16161616))
				pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0,false);
			else
				pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0,true);

			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 1, NULL, 0 );

			DECLARE_STATIC_VERTEX_SHADER( sky_vs20 );
			SET_STATIC_VERTEX_SHADER( sky_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( sky_ps20b );
				SET_STATIC_PIXEL_SHADER( sky_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( sky_ps20 );
				SET_STATIC_PIXEL_SHADER( sky_ps20 );
			}
			// we are writing linear values from this shader.
			pShaderShadow->EnableSRGBWrite( true );

			pShaderShadow->EnableAlphaWrites( true );
		}

		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			float c1[4]={0,0,0,0};
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, c1);

			float c0[4]={1,1,1,1};
			if (params[COLOR]->IsDefined())
			{
				memcpy(c0,params[COLOR]->GetVecValue(),3*sizeof(float));
			}
			ITexture *txtr=params[BASETEXTURE]->GetTextureValue();
			ImageFormat fmt=txtr->GetImageFormat();
			if (
				(fmt==IMAGE_FORMAT_RGBA16161616) ||
				( (fmt==IMAGE_FORMAT_RGBA16161616F) && 
				  (g_pHardwareConfig->GetHDRType()==HDR_TYPE_INTEGER))
				)
			{
				c0[0]*=16.0;
				c0[1]*=16.0;
				c0[2]*=16.0;
			}
			pShaderAPI->SetPixelShaderConstant(0,c0,1);
			DECLARE_DYNAMIC_VERTEX_SHADER( sky_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( sky_vs20 );

			// Texture coord transform
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, BASETEXTURETRANSFORM );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sky_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( WRITE_DEPTH_TO_DESTALPHA, pShaderAPI->ShouldWriteDepthToDestAlpha() );
				SET_DYNAMIC_PIXEL_SHADER( sky_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( sky_ps20 );
				SET_DYNAMIC_PIXEL_SHADER( sky_ps20 );
			}
		}
		Draw( );
	}

END_SHADER

