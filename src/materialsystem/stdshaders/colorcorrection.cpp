//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "BaseVSShader.h"

#include "screenspaceeffect_vs20.inc"
#include "colorcorrection_ps20.inc"
#include "colorcorrection_ps20b.inc"

#include "../materialsystem_global.h"


BEGIN_VS_SHADER_FLAGS( ColorCorrection, "Help for ColorCorrection", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( WEIGHT_DEFAULT, SHADER_PARAM_TYPE_FLOAT,   "1", "Volume Texture Default Weight" )
		SHADER_PARAM( WEIGHT0,		  SHADER_PARAM_TYPE_FLOAT,   "0", "Volume Texture Weight 0" )
		SHADER_PARAM( WEIGHT1,		  SHADER_PARAM_TYPE_FLOAT,   "0", "Volume Texture Weight 1" )
		SHADER_PARAM( WEIGHT2,		  SHADER_PARAM_TYPE_FLOAT,   "0", "Volume Texture Weight 2" )
		SHADER_PARAM( WEIGHT3,		  SHADER_PARAM_TYPE_FLOAT,   "0", "Volume Texture Weight 3" )
		SHADER_PARAM( NUM_LOOKUPS,    SHADER_PARAM_TYPE_INTEGER, "0", "Number of lookup maps" )
		SHADER_PARAM( USE_FB_TEXTURE, SHADER_PARAM_TYPE_BOOL,    "0", "Use frame buffer texture as input" )
		SHADER_PARAM( INPUT_TEXTURE,  SHADER_PARAM_TYPE_TEXTURE, "0", "Input texture" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if( !params[ WEIGHT_DEFAULT ]->IsDefined() )
		{
			params[ WEIGHT_DEFAULT ]->SetFloatValue( 1.0f) ;
		}
		if( !params[ WEIGHT0 ]->IsDefined() )
		{
			params[ WEIGHT0 ]->SetFloatValue( 1.0f );
		}
		if( !params[ WEIGHT1 ]->IsDefined() )
		{
			params[ WEIGHT1 ]->SetFloatValue( 1.0f );
		}
		if( !params[ WEIGHT2 ]->IsDefined() )
		{
			params[ WEIGHT2 ]->SetFloatValue( 1.0f );
		}
		if( !params[ WEIGHT3 ]->IsDefined() )
		{
			params[ WEIGHT3 ]->SetFloatValue( 1.0f );
		}
		if( !params[ NUM_LOOKUPS ]->IsDefined() )
		{
			params[ NUM_LOOKUPS ]->SetIntValue( 0 );
		}
		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE );
	}

	SHADER_FALLBACK
	{
		// Requires DX9 + above
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "wireframe";
		}
		return 0;
	}

	SHADER_INIT
	{
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER2, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER4, true );

			int fmt = VERTEX_POSITION;
			pShaderShadow->VertexShaderVertexFormat( fmt, 1, 0, 0 );

			// Render targets are always sRGB on OSX GL
			bool bForceSRGBWrite = IsOSX() && g_pHardwareConfig->CanDoSRGBReadFromRTs();
			pShaderShadow->EnableSRGBWrite( bForceSRGBWrite );

			DECLARE_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_STATIC_VERTEX_SHADER( screenspaceeffect_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( colorcorrection_ps20b );
				SET_STATIC_PIXEL_SHADER( colorcorrection_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( colorcorrection_ps20 );
				SET_STATIC_PIXEL_SHADER( colorcorrection_ps20 );
			}
		}
		DYNAMIC_STATE
		{
			if( params[ USE_FB_TEXTURE ]->GetIntValue() )
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0 );
			else
				BindTexture( SHADER_SAMPLER0, INPUT_TEXTURE, -1 );

			for( int i=0;i<params[NUM_LOOKUPS]->GetIntValue();i++ )
			{
				pShaderAPI->BindStandardTexture( (Sampler_t)(SHADER_SAMPLER1+i), (StandardTextureId_t)(TEXTURE_COLOR_CORRECTION_VOLUME_0+i) );
			}

			float default_weight = params[ WEIGHT_DEFAULT ]->GetFloatValue();
			float weights[4] = { params[ WEIGHT0 ]->GetFloatValue(),
								 params[ WEIGHT1 ]->GetFloatValue(),
								 params[ WEIGHT2 ]->GetFloatValue(),
								 params[ WEIGHT3 ]->GetFloatValue()  };

			pShaderAPI->SetPixelShaderConstant( 0, &default_weight );
			pShaderAPI->SetPixelShaderConstant( 1, &weights[0] );
			pShaderAPI->SetPixelShaderConstant( 2, &weights[1] );
			pShaderAPI->SetPixelShaderConstant( 3, &weights[2] );
			pShaderAPI->SetPixelShaderConstant( 4, &weights[3] );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( colorcorrection_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LOOKUPS, params[ NUM_LOOKUPS ]->GetIntValue() );
				SET_DYNAMIC_PIXEL_SHADER( colorcorrection_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( colorcorrection_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( NUM_LOOKUPS, params[ NUM_LOOKUPS ]->GetIntValue() );
				SET_DYNAMIC_PIXEL_SHADER( colorcorrection_ps20 );
			}

			DECLARE_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( screenspaceeffect_vs20 );
		}
		Draw();
	}
END_SHADER
