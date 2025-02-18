//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A wet version of base * lightmap
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "particlesphere_vs20.inc"
#include "particlesphere_ps20.inc"
#include "particlesphere_ps20b.inc"

#include "cpp_shader_constant_register_map.h"

int GetDefaultDepthFeatheringValue( void ); //defined in spritecard.cpp

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( ParticleSphere, ParticleSphere_DX9 )

BEGIN_VS_SHADER_FLAGS( ParticleSphere_DX9, "Help for BumpmappedEnvMap", SHADER_NOT_EDITABLE  )
			   
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DEPTHBLEND, SHADER_PARAM_TYPE_INTEGER, "0", "fade at intersection boundaries" )
		SHADER_PARAM( DEPTHBLENDSCALE, SHADER_PARAM_TYPE_FLOAT, "50.0", "Amplify or reduce DEPTHBLEND fading. Lower values make harder edges." )
		SHADER_PARAM( USINGPIXELSHADER, SHADER_PARAM_TYPE_BOOL, "0", "Tells to client code whether the shader is using DX8 vertex/pixel shaders or not" )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bumpmap" )
		SHADER_PARAM( LIGHTS, SHADER_PARAM_TYPE_FOURCC, "", "array of lights" )
		SHADER_PARAM( LIGHT_POSITION, SHADER_PARAM_TYPE_VEC3, "0 0 0", "This is the directional light position." )
		SHADER_PARAM( LIGHT_COLOR, SHADER_PARAM_TYPE_VEC3, "1 1 1", "This is the directional light color." )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		if ( !params[DEPTHBLEND]->IsDefined() )
		{
			params[ DEPTHBLEND ]->SetIntValue( GetDefaultDepthFeatheringValue() );
		}
		if ( !g_pHardwareConfig->SupportsPixelShaders_2_b() )
		{
			params[ DEPTHBLEND ]->SetIntValue( 0 );
		}
		if ( !params[DEPTHBLENDSCALE]->IsDefined() )
		{
			params[ DEPTHBLENDSCALE ]->SetFloatValue( 50.0f );
		}
	}

	bool UsePixelShaders( IMaterialVar **params ) const
	{
		return  (!params || params[BUMPMAP]->IsDefined()) && g_pHardwareConfig->SupportsVertexAndPixelShaders();
	}

	SHADER_INIT
	{
		// If this would return false, then we should have fallen back to the DX6 one.
		Assert( UsePixelShaders( params ) );

		params[USINGPIXELSHADER]->SetIntValue( true );
		LoadBumpMap( BUMPMAP );
	}

	SHADER_FALLBACK
	{
		if ( !UsePixelShaders(params) )
		{
			return "UnlitGeneric_DX6";
		}

		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "ParticleSphere_DX8";
		}

		return 0;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			if ( params[DEPTHBLEND]->GetIntValue() )
			{
				pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			}
			
			int tCoordDimensions[] = {2};
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_COLOR, 1, tCoordDimensions, 0 );

			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableDepthWrites( false );

			DECLARE_STATIC_VERTEX_SHADER( particlesphere_vs20 );
			SET_STATIC_VERTEX_SHADER( particlesphere_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( particlesphere_ps20b );
				SET_STATIC_PIXEL_SHADER_COMBO( DEPTHBLEND, params[DEPTHBLEND]->GetIntValue() );
				SET_STATIC_PIXEL_SHADER( particlesphere_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( particlesphere_ps20 );
				SET_STATIC_PIXEL_SHADER( particlesphere_ps20 );
			}

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BUMPMAP );

			if ( params[DEPTHBLEND]->GetIntValue() )
			{
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_FRAME_BUFFER_FULL_DEPTH );
			}

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, params[LIGHT_POSITION]->GetVecValue() );
			
			// Separate the light color into something that has a max value of 1 and a scale
			// so the vertex shader can determine if it's going to overflow the color and scale back
			// if it needs to.
			//
			// (It does this by seeing if the intensity*1/distSqr is > 1. If so, then it scales it so
			// it is equal to 1).
			const float *f = params[LIGHT_COLOR]->GetVecValue();
			Vector vLightColor( f[0], f[1], f[2] );
			float flScale = max( vLightColor.x, max( vLightColor.y, vLightColor.z ) );
			if ( flScale < 0.01f )
				flScale = 0.01f;
			float vScaleVec[3] = { flScale, flScale, flScale };
			vLightColor /= flScale;

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_1, vLightColor.Base() );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_2, vScaleVec );

			pShaderAPI->SetPixelShaderFogParams( PSREG_FOG_PARAMS );

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition( vEyePos_SpecExponent );
			vEyePos_SpecExponent[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1 );

			pShaderAPI->SetDepthFeatheringPixelShaderConstant( 0, params[DEPTHBLENDSCALE]->GetFloatValue() );


			// Compute the vertex shader index.
			DECLARE_DYNAMIC_VERTEX_SHADER( particlesphere_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( FOGTYPE, s_pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			SET_DYNAMIC_VERTEX_SHADER( particlesphere_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( particlesphere_ps20b );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( particlesphere_ps20b );
			}
			else
			{
				DECLARE_DYNAMIC_PIXEL_SHADER( particlesphere_ps20 );
				SET_DYNAMIC_PIXEL_SHADER_COMBO( PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo() );
				SET_DYNAMIC_PIXEL_SHADER( particlesphere_ps20 );
			}
		}
		Draw();
	}
END_SHADER
