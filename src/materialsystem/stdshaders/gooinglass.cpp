//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BaseVSShader.h"

#include "bumpmappedenvmap.inc"

#include "lightmappedgeneric_vs11.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME: Need to make a dx9 version so that "CENTROID" works.

BEGIN_VS_SHADER( GooInGlass,
			  "Help for GooInGlass" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM_OVERRIDE( BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/GooInGlass", "Base texture", 0 )
		SHADER_PARAM_OVERRIDE( COLOR, SHADER_PARAM_TYPE_COLOR, "{255 255 255}", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM_OVERRIDE( ALPHA, SHADER_PARAM_TYPE_FLOAT, "1.0", "unused", SHADER_PARAM_NOT_EDITABLE )
		SHADER_PARAM( BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/GooInGlass_normal", "bump map" )
		SHADER_PARAM( BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bump texcoord transform" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/GooInGlass_env", "envmap" )
		SHADER_PARAM( GLASSENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/GooInGlass_envglass", "Glass Envmap" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( GLASSENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( TRANSLUCENTGOO, SHADER_PARAM_TYPE_BOOL, "0", "whether or not goo is translucent" )
	END_SHADER_PARAMS

	SHADER_INIT
	{
		LoadBumpMap( BUMPMAP );
		LoadTexture( BASETEXTURE );
		LoadCubeMap( ENVMAP );
		LoadCubeMap( GLASSENVMAP );
		if( !params[ENVMAPTINT]->IsDefined() )
		{
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[GLASSENVMAPTINT]->IsDefined() )
		{
			params[GLASSENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );
		}
		if( !params[TRANSLUCENTGOO]->IsDefined() )
		{
			params[TRANSLUCENTGOO]->SetIntValue( 0 );
		}
	}

	SHADER_DRAW
	{
		// + MASKED BUMPED CUBEMAP * ENVMAPTINT
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			if( params[TRANSLUCENTGOO]->GetIntValue() )
			{
				pShaderShadow->EnableBlending( true );
				pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
			}
			// FIXME: Remove the normal (needed for tangent space gen)
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0 );

			bumpmappedenvmap_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "BumpmappedEnvmap", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "BumpmappedEnvmap" );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BUMPMAP );
			BindTexture( SHADER_SAMPLER3, ENVMAP );

			float constantColor[4];
			params[ENVMAPTINT]->GetVecValue( constantColor, 3 );
			constantColor[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );

			// handle scrolling of bump texture
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BUMPTRANSFORM );

			bumpmappedenvmap_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
		// glass envmap
		SHADOW_STATE
		{
			SetInitialShadowState( );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER3, true );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_ONE, SHADER_BLEND_ONE );

			// FIXME: Remove the normal (needed for tangent space gen)
			pShaderShadow->VertexShaderVertexFormat( 
				VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TANGENT_S |
				VERTEX_TANGENT_T, 1, 0, 0 );

			bumpmappedenvmap_Static_Index vshIndex;
			pShaderShadow->SetVertexShader( "BumpmappedEnvmap", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "BumpmappedEnvMap" );
			FogToBlack();
		}
		DYNAMIC_STATE
		{
			// fixme: doesn't support camera space envmapping!!!!!!
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_NORMALMAP_FLAT );
			BindTexture( SHADER_SAMPLER3, GLASSENVMAP );

			float constantColor[4];
			params[GLASSENVMAPTINT]->GetVecValue( constantColor, 3 );
			constantColor[3] = 0.0f;
			pShaderAPI->SetPixelShaderConstant( 0, constantColor, 1 );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_4, BUMPTRANSFORM );
			bumpmappedenvmap_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );

			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();

		// BASE TEXTURE * LIGHTMAP
		SHADOW_STATE
		{				
			SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
			SetInitialShadowState( );
			pShaderShadow->EnableBlending( true );
			pShaderShadow->BlendFunc( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION, 2, 0, 0 );

			lightmappedgeneric_vs11_Static_Index vshIndex;
			vshIndex.SetDETAIL( false );
			vshIndex.SetENVMAP( false );
			vshIndex.SetENVMAPCAMERASPACE( false );
			vshIndex.SetENVMAPSPHERE( false );
			vshIndex.SetVERTEXCOLOR( false );
			pShaderShadow->SetVertexShader( "LightmappedGeneric_vs11", vshIndex.GetIndex() );

			pShaderShadow->SetPixelShader( "LightmappedGeneric" );
			FogToFogColor();
		}
		DYNAMIC_STATE
		{
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			SetVertexShaderTextureTransform( VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BASETEXTURETRANSFORM );
			EnablePixelShaderOverbright( true, 0, true );

			lightmappedgeneric_vs11_Dynamic_Index vshIndex;
			vshIndex.SetDOWATERFOG( pShaderAPI->GetSceneFogMode() == MATERIAL_FOG_LINEAR_BELOW_FOG_Z );
			pShaderAPI->SetVertexShaderIndex( vshIndex.GetIndex() );
		}
		Draw();
	}
END_SHADER
