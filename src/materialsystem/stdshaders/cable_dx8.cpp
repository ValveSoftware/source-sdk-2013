//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A wet version of base * lightmap
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#include "BaseVSShader.h"

#include "cable.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DEFINE_FALLBACK_SHADER( Cable, Cable_DX8 )

BEGIN_VS_SHADER( Cable_DX8, 
			  "Help for Cable shader" )
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "cable/cablenormalmap", "bumpmap texture" )
		SHADER_PARAM( MINLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.1", "Minimum amount of light (0-1 value)" )
		SHADER_PARAM( MAXLIGHT, SHADER_PARAM_TYPE_FLOAT, "0.3", "Maximum amount of light" )
	END_SHADER_PARAMS

	SHADER_FALLBACK
	{
		if ( IsPC() && !g_pHardwareConfig->SupportsVertexAndPixelShaders())
		{
			return "UnlitGeneric_DX6";
		}
		return 0;
	}

	SHADER_INIT
	{
		LoadBumpMap( BUMPMAP );
		LoadTexture( BASETEXTURE );
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			// Enable blending?
			if ( IS_FLAG_SET( MATERIAL_VAR_TRANSLUCENT ) )
			{
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}

			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			if ( g_pHardwareConfig->GetDXSupportLevel() >= 90)
			{
				pShaderShadow->EnableSRGBRead( SHADER_SAMPLER1, true );
			}
			
			int tCoordDimensions[] = {2,2};
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_COLOR | VERTEX_TANGENT_S | VERTEX_TANGENT_T, 
				2, tCoordDimensions, 0 );

			cable_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "Cable", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "Cable" );

			// we are writing linear values from this shader.
			// This is kinda wrong.  We are writing linear or gamma depending on "IsHDREnabled" below.
			// The COLOR really decides if we are gamma or linear.  
			if ( g_pHardwareConfig->GetDXSupportLevel() >= 90)
			{
				pShaderShadow->EnableSRGBWrite( true );
			}

			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BUMPMAP );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE );
			

			// The dot product with the light is remapped from the range 
			// [-1,1] to [MinLight, MaxLight].

			// Given:
			//	-A + B = MinLight
			//	 A + B = MaxLight
			// then A = (MaxLight - MinLight) / 2
			// and  B = (MaxLight + MinLight) / 2

			// So here, we multiply the light direction by A to scale the dot product.
			// Then in the pixel shader we add by B.
			float flMinLight = params[MINLIGHT]->GetFloatValue();
			float flMaxLight = params[MAXLIGHT]->GetFloatValue();
			
			float A = (flMaxLight - flMinLight) * 0.5f;
			float B = (flMaxLight + flMinLight) * 0.5f;

			float b4[4] = {B,B,B,B};
			if( g_pHardwareConfig->GetDXSupportLevel() >= 90)
			{
				SetPixelShaderConstantGammaToLinear( 0, b4 );
			}
			else
			{
				pShaderAPI->SetPixelShaderConstant( 0, b4 );
			}
			
			// This is the light direction [0,1,0,0] * A * 0.5
			float lightDir[4] = {0, A*0.5, 0, 0};
			if( g_pHardwareConfig->GetDXSupportLevel() >= 90)
			{
				SetVertexShaderConstantGammaToLinear( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lightDir );
			}
			else
			{
				pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, lightDir );
			}

			cable_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
