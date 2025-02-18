//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "treeleaf_ps20.inc"
#include "treeleaf_ps20b.inc"
#include "treeleaf_vs20.inc"

BEGIN_VS_SHADER_FLAGS( TreeLeaf, "Help for TreeLeaf", SHADER_NOT_EDITABLE )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( LEAFCENTER, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "Center of leaf cluster for lighting" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
	}

	SHADER_FALLBACK
	{
		if ( g_pHardwareConfig->GetDXSupportLevel() < 90 )
		{
			return "Wireframe";
		}
		return 0;
	}

	SHADER_INIT
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_VERTEX_LIT );

		if ( params[BASETEXTURE]->IsDefined() )
		{
			LoadTexture( BASETEXTURE );
		}
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			pShaderShadow->EnableAlphaTest( true );
			pShaderShadow->AlphaFunc( SHADER_ALPHAFUNC_GREATER, 0.5f );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			int numTexCoords = 1;
			pShaderShadow->VertexShaderVertexFormat( flags, numTexCoords, 0, 0 );

			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_STATIC_VERTEX_SHADER( treeleaf_vs20 );
			SET_STATIC_VERTEX_SHADER_COMBO( HALFLAMBERT, true );
			SET_STATIC_VERTEX_SHADER_COMBO( USE_STATIC_CONTROL_FLOW, bUseStaticControlFlow );
			SET_STATIC_VERTEX_SHADER( treeleaf_vs20 );

			if( g_pHardwareConfig->SupportsPixelShaders_2_b() )
			{
				DECLARE_STATIC_PIXEL_SHADER( treeleaf_ps20b );
				SET_STATIC_PIXEL_SHADER( treeleaf_ps20b );
			}
			else
			{
				DECLARE_STATIC_PIXEL_SHADER( treeleaf_ps20 );
				SET_STATIC_PIXEL_SHADER( treeleaf_ps20 );
			}

			// we are writing linear values from this shader.
			// This is kinda wrong.  We are writing linear or gamma depending on "IsHDREnabled" below.
			// The COLOR really decides if we are gamma or linear.  
			pShaderShadow->EnableSRGBWrite( false );
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );

			// We need the view matrix
			LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_VIEWMODEL );

			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, params[ LEAFCENTER ]->GetVecValue() );

			LightState_t lightState;
			pShaderAPI->GetDX9LightState( &lightState );

			bool bUseStaticControlFlow = g_pHardwareConfig->SupportsStaticControlFlow();

			DECLARE_DYNAMIC_VERTEX_SHADER( treeleaf_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( DYNAMIC_LIGHT, lightState.HasDynamicLight() );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( STATIC_LIGHT, lightState.m_bStaticLightVertex ? 1 : 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( NUM_LIGHTS, bUseStaticControlFlow ? 0 : lightState.m_nNumLights );
			SET_DYNAMIC_VERTEX_SHADER( treeleaf_vs20 );
		}
		Draw( );
	}
END_SHADER
