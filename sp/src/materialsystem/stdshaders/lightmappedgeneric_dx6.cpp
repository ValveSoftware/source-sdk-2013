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

DEFINE_FALLBACK_SHADER( LightmappedGeneric, LightmappedGeneric_DX6 )
 
BEGIN_SHADER( LightmappedGeneric_DX6,
			  "Help for LightmappedGeneric_DX6" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint" )
		SHADER_PARAM( DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "detail texture" )
		SHADER_PARAM( DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture" )
		SHADER_PARAM( ENVMAP, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_env", "envmap" )
		SHADER_PARAM( ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "shadertest/shadertest_envmask", "envmap mask" )
		SHADER_PARAM( ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "" )
		SHADER_PARAM( ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1", "envmap mask scale" )
		SHADER_PARAM( ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "envmap tint" )
		SHADER_PARAM( ENVMAPOPTIONAL, SHADER_PARAM_TYPE_BOOL, "90", "Do specular pass only on dxlevel or higher (ie.80, 81, 90)" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		// FLASHLIGHTFIXME
		params[FLASHLIGHTTEXTURE]->SetStringValue( "effects/flashlight001" );

		if( params[BASETEXTURE]->IsDefined() )
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_MULTIPASS) )
			{
				params[ENVMAP]->SetUndefined();
				params[ENVMAPMASK]->SetUndefined();
			}
		}

		if( !params[ENVMAPMASKSCALE]->IsDefined() )
			params[ENVMAPMASKSCALE]->SetFloatValue( 1.0f );

		if( !params[ENVMAPTINT]->IsDefined() )
			params[ENVMAPTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[SELFILLUMTINT]->IsDefined() )
			params[SELFILLUMTINT]->SetVecValue( 1.0f, 1.0f, 1.0f );

		if( !params[DETAILSCALE]->IsDefined() )
			params[DETAILSCALE]->SetFloatValue( 4.0f );

		// No texture means no self-illum or env mask in base alpha
		if ( !params[BASETEXTURE]->IsDefined() )
		{
			CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
			CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
		}

		// If in decal mode, no debug override...
		if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
		{
			SET_FLAGS( MATERIAL_VAR_NO_DEBUG_OVERRIDE );
		}

		SET_FLAGS2( MATERIAL_VAR2_LIGHTING_LIGHTMAP );

		// Get rid of the envmap if it's optional for this dx level.
		if( params[ENVMAPOPTIONAL]->IsDefined() && params[ENVMAPOPTIONAL]->GetIntValue() )
		{
			params[ENVMAP]->SetUndefined();
		}

		// If mat_specular 0, then get rid of envmap
		if( !g_pConfig->UseSpecular() && params[ENVMAP]->IsDefined() && params[BASETEXTURE]->IsDefined() )
		{
			params[ENVMAP]->SetUndefined();
		}

		SET_FLAGS2( MATERIAL_VAR2_NEEDS_FIXED_FUNCTION_FLASHLIGHT );
	}

	SHADER_INIT
	{
		LoadTexture( FLASHLIGHTTEXTURE );
		if (params[BASETEXTURE]->IsDefined())
		{
			LoadTexture( BASETEXTURE );

			if (!params[BASETEXTURE]->GetTextureValue()->IsTranslucent())
			{
				if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM))
					CLEAR_FLAGS( MATERIAL_VAR_SELFILLUM );
				if (IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
					CLEAR_FLAGS( MATERIAL_VAR_BASEALPHAENVMAPMASK );
			}
		}

		if (params[DETAIL]->IsDefined())
		{
			LoadTexture( DETAIL );
		}

		// Don't alpha test if the alpha channel is used for other purposes
		if (IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK) )
			CLEAR_FLAGS( MATERIAL_VAR_ALPHATEST );
			
		if (params[ENVMAP]->IsDefined())
		{
			if( !IS_FLAG_SET(MATERIAL_VAR_ENVMAPSPHERE) )
			{
				LoadCubeMap( ENVMAP );
			}
			else
			{
				LoadTexture( ENVMAP );
			}

			if( !g_pHardwareConfig->SupportsCubeMaps() )
			{
				SET_FLAGS( MATERIAL_VAR_ENVMAPSPHERE );
			}

			if (params[ENVMAPMASK]->IsDefined())
			{
				LoadTexture( ENVMAPMASK );
			}
		}
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	int GetDrawFlagsPass1(IMaterialVar** params)
	{
		int flags = SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD0;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			flags |= SHADER_DRAW_COLOR;
		if (params[BASETEXTURE]->IsTexture())
			flags |= SHADER_DRAW_TEXCOORD1;
		return flags;
	}

	void DrawLightmapOnly( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		SHADOW_STATE
		{
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE0, OVERBRIGHT );

			SetModulationShadowState();
			SetDefaultBlendingShadowState( );
			pShaderShadow->DrawFlags( GetDrawFlagsPass1( params ) );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_LIGHTMAP );
			SetModulationDynamicState();
		}
		Draw();
	}

	void DrawBaseTimesLightmap( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Base times lightmap
		SHADOW_STATE
		{
			// alpha test
 			pShaderShadow->EnableAlphaTest( IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) );

			// Alpha blending
			SetDefaultBlendingShadowState( BASETEXTURE, true );

			// Independenly configure alpha and color
			pShaderShadow->EnableAlphaPipe( true );

			// color channel
			
			// base
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			// lightmap
			pShaderShadow->EnableTexture( SHADER_SAMPLER1, true );
			pShaderShadow->OverbrightValue( SHADER_TEXTURE_STAGE1, OVERBRIGHT );

			pShaderShadow->EnableConstantColor( IsColorModulating() );

			// alpha channel
			pShaderShadow->EnableConstantAlpha( IsAlphaModulating() );
			if ((! IsColorModulating()) && ( ! IsAlphaModulating()))
				pShaderShadow->EnableVertexAlpha( IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE0, TextureIsTranslucent(BASETEXTURE, true) );
			pShaderShadow->EnableTextureAlpha( SHADER_TEXTURE_STAGE1, false );

			int flags = SHADER_DRAW_POSITION | SHADER_DRAW_LIGHTMAP_TEXCOORD1;
			if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
			{
				if ( (! IsColorModulating()) && ( ! IsAlphaModulating()))
					flags |= SHADER_DRAW_COLOR;
			
			}
			if (params[BASETEXTURE]->IsTexture())
			{
				flags |= SHADER_DRAW_TEXCOORD0;
			}

			pShaderShadow->DrawFlags( flags );
			DefaultFog();
		}
		DYNAMIC_STATE
		{
			pShaderAPI->BindStandardTexture( SHADER_SAMPLER1, TEXTURE_LIGHTMAP );
			BindTexture( SHADER_SAMPLER0, BASETEXTURE, FRAME );
			SetFixedFunctionTextureTransform( MATERIAL_TEXTURE0, BASETEXTURETRANSFORM );
			SetModulationDynamicState();
		}
		Draw();

		SHADOW_STATE
		{
			pShaderShadow->EnableAlphaPipe( false );
		}
	}

	void DrawMode1( IMaterialVar** params, IShaderDynamicAPI *pShaderAPI, IShaderShadow* pShaderShadow )
	{
		// Pass 1 : Base * lightmap or just lightmap
		if ( params[BASETEXTURE]->IsTexture() )
		{
			DrawBaseTimesLightmap( params, pShaderAPI, pShaderShadow );
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );

			// Draw the selfillum pass
			if ( IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) )
			{
				FixedFunctionSelfIlluminationPass( 
					SHADER_SAMPLER0, BASETEXTURE, FRAME, BASETEXTURETRANSFORM, SELFILLUMTINT );
			}
		}
		else
		{
			DrawLightmapOnly( params, pShaderAPI, pShaderShadow );
			FixedFunctionMultiplyByDetailPass(
				BASETEXTURE, FRAME, BASETEXTURETRANSFORM, DETAIL, DETAILSCALE );
		}

		// Pass 2 : Masked environment map
		if (params[ENVMAP]->IsTexture())
		{
			FixedFunctionAdditiveMaskedEnvmapPass( 
				ENVMAP, ENVMAPMASK, BASETEXTURE,
				ENVMAPFRAME, ENVMAPMASKFRAME, FRAME, 
				BASETEXTURETRANSFORM, ENVMAPMASKSCALE, ENVMAPTINT );
		}
	}

	SHADER_DRAW
	{
		bool hasFlashlight = UsingFlashlight( params );

		if( hasFlashlight )
		{
			DrawFlashlight_dx70( params, pShaderAPI, pShaderShadow, FLASHLIGHTTEXTURE, FLASHLIGHTTEXTUREFRAME );
		}
		else
		{
			// Base * Lightmap + env
			DrawMode1( params, pShaderAPI, pShaderShadow );
		}
	}
END_SHADER


//-----------------------------------------------------------------------------
// This allows us to use a block labelled 'Water_DX60' in the water materials
//-----------------------------------------------------------------------------
BEGIN_INHERITED_SHADER( Water_DX60, LightmappedGeneric_DX6, "Help for Water_DX60" )
END_INHERITED_SHADER

