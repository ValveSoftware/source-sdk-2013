//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Lightmap only shader
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#include "shaderlib/cshader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// FIXME!!  Need to make dx9 version so that "CENTROID" flags can be added!
BEGIN_SHADER( LightmappedTwoTexture,
			  "Help for LightmappedTwoTexture" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/Detail", "detail" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "detail scale" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "Frame for animated envmap" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );
		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}
	}

	SHADER_INIT
	{
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{					 
			LoadTexture( DETAIL );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
				LoadCubeMap( ENVMAP );
			else
				LoadTexture( ENVMAP );

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
				LoadTexture( ENVMAPMASK );
		}
	}

	SHADER_FALLBACK
	{
		// The only thing we can't do here is masked env-mapped
		if (g_pHardwareConfig->GetSamplerCount() < 2)
		{
			return "LightmappedTwoTexture_DX5";
		}
		return 0;
	}

	/*
	// FIXME: I didn't remove this because it's modulating against the detail alpha!
	// This seems very strange and isn't what FixedFunctionBaseTimesDetailPass does.
	// Should it!?!? If so, I want to move this to the FixedFunctionBaseTimesDetailPass.

	void DrawBaseTimesDetail( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Base
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending, enable alpha blending if the detail texture is translucent
			bool detailIsTranslucent = TextureIsTranslucent( DETAIL, false );
			if ( detailIsTranslucent )
			{
				if ( IS_FLAG_SET( MATERIAL_VAR_ADDITIVE ) )
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
				else
					EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				SetDefaultBlendingShadowState( BASETEXTURE, true );
			}

			// independently configure alpha and color
			pShaderShadow->EnableAlphaPipe( true );

			// Here's the color	states (NOTE: SHADER_DRAW_COLOR == use Vertex Color)
			pShaderShadow->EnableConstantColor( IsColorModulating() );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE0, true );
			pShaderShadow->EnableTexture( SHADER_TEXTURE_STAGE1, true );

			// Here's the alpha states
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, TextureIsTranslucent(BASETEXTURE, true) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, detailIsTranslucent );

			int flags = SHADER_DRAW_POSITION | SHADER_DRAW_TEXCOORD0 | SHADER_DRAW_SECONDARY_TEXCOORD1;
			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
				flags |= SHADER_DRAW_COLOR;
			pShaderShadow->DrawFlags( flags );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			SetFixedFunctionTextureScale( MATERIAL_TEXTURE1, DETAILSCALE );
			BindTexture( SHADER_TEXTURE_STAGE0, BASETEXTURE, FRAME );
			BindTexture( SHADER_TEXTURE_STAGE1, DETAIL );
			SetModulationDynamicState();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaPipe( false );
		}
	}
	*/

	SHADER_DRAW
	{
		// Pass 1 : Base + detail
		FixedFunctionBaseTimesDetailPass(
			BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );

		// Pass 2 : * lightmap
		FixedFunctionMultiplyByLightmapPass( BASETEXTURE, FRAME, BASETEXTURETRANSFORM );

		// Pass 3 : + env
		bool envDefined = params[ENVMAP]->IsTexture();
		if (envDefined)
		{
			FixedFunctionAdditiveMaskedEnvmapPass(
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
	}
END_SHADER
