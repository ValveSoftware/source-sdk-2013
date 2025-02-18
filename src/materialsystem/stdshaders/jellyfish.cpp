//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "jellyfish.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_VS_SHADER( JellyFish, 
			  "Help for JellyFish" )
			  
	BEGIN_SHADER_PARAMS
		SHADER_PARAM( GRADIENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "1D texture for silhouette glowy bits" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/cubemap", "envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmaptint" )
		SHADER_PARAM( PULSERATE, SHADER_PARAM_TYPE_FLOAT, "1", "blah" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_INIT
	{
		LoadTexture( GRADIENTTEXTURE );
		LoadTexture( BASETEXTURE );
		if( params[ENVMAP]->IsDefined() )
		{
			LoadCubeMap( ENVMAP );
		}
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[PULSERATE]->IsDefined() )
		{
			params[PULSERATE]->SetFloatValue( 0.0f );
		}
	}

	SHADER_DRAW
	{
		// fixme
		if( !g_pHardwareConfig->SupportsVertexAndPixelShaders() )
		{
			return;
		}

		SHADOW_STATE
		{				
			pShaderShadow->EnableDepthWrites( false );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL, 1, 0, 0 );
			jellyfish_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "JellyFish", vshIndex.GetIndex() );
			pShaderShadow->SetPixelShader( "JellyFish" );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			float time[4];
			time[0] = pShaderAPI->CurrentTime() * params[PULSERATE]->GetFloatValue();
			BindTexture( SHADER_SAMPLER0, GRADIENTTEXTURE );
			BindTexture( SHADER_SAMPLER1, BASETEXTURE );
			pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_5, time, 1 );
			LoadViewMatrixIntoVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0 );

			jellyfish_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			vshIndex.SetSKINNING( pShaderAPI->GetCurrentNumBones() > 0 );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();

		if( params[ENVMAP]->IsTexture() )
		{
			SHADOW_STATE
			{
				SetInitialShadowState( );
				pShaderShadow->EnableDepthWrites( false );
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );
				pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
				pShaderShadow->TexGen( SHADER_TEXTURE_STAGE0, SHADER_TEXGENPARAM_CAMERASPACEREFLECTIONVECTOR );
				pShaderShadow->EnableTexGen( SHADER_TEXTURE_STAGE0, true );
				pShaderShadow->DrawFlags( SHADER_DRAW_POSITION | SHADER_DRAW_NORMAL );
				pShaderShadow->EnableConstantColor( true );
				FogToFogColor();
			}
			DYNAMIC_STATE
			{
				SetColorState( ENVMAPTINT );
				LoadCameraToWorldTransform( MATERIAL_TEXTURE0 );
				BindTexture( SHADER_SAMPLER0, ENVMAP );
			}
			Draw();
		}
	}
END_SHADER
